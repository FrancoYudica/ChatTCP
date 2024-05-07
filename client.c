// gcc client.c utils.h -o client -pthread
#include <arpa/inet.h>
#include "utils.h"
#include "pthread.h"

char* server_ip;
char* username;
int server_port = 8000;
int sockfd;
struct sockaddr_in server_addr;
socklen_t server_len = sizeof(struct sockaddr_in);

/// @brief Wrapper function that sends a message directly to the server
void send_to_server(const char* message)
{
    
    if (send(sockfd, message, strlen(message), 0) == -1) {
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }
}

/// @brief Process user input
/// @return If the input should be sent to the server
int process_command(const char* buffer)
{
    // In case user wants to disconnect
    if (!strcmp(buffer, COMMAND_DISCONNECT))
    {
        send_to_server(buffer);
        printf("Disconnecting...\n");
        exit(EXIT_SUCCESS);
    }

    else if (!strcmp(buffer, COMMAND_CONNECTED))
    {
        printf("Asking for connected users...\n");
        send_to_server(buffer);
    }

    else if (!strcmp(buffer, COMMAND_LOGOUT))
    {
        send_to_server(buffer);
        printf("Logging out in server\n");
        free((void*)username);
        username = NULL;
    }
    else if (prefix(buffer, COMMAND_USERNAME))
    {
        char* u_name = remove_prefix(strlen("/username "), buffer);
        if (u_name != NULL)
            username = u_name;

        send_to_server(buffer);
    }

    else if (!strcmp(buffer, COMMAND_HELP))
    {
        printf(
            "Commands:\n"
            "   /disconnect:             Disconnects from the server and stops application\n"
            "   /username:               Asks for current username in server\n"
            "   /username new_name:      Sets new name\n"
            "   /connected:              Asks server for currenly connected clients\n"
        );
    }
    else
    {
        printf("Unrecognized command.\n");
    }

}

void* write_task()
{
    char write_buffer[BUFFER_SIZE];
    while(1)
    {
        fgets(write_buffer, BUFFER_SIZE, stdin);

        if (strlen(write_buffer) == 0)
            continue;

        write_buffer[strlen(write_buffer) - 1] = '\0';

        if (is_command(write_buffer))
            process_command(write_buffer);
        else
            send_to_server(write_buffer);
    }
    return NULL;

}




int main(int argc, char** argv)
{

    // Loads arguments
    int i = 0;
    while (++i < argc - 1)
    {
        char* parameter = argv[i++];

        if (!strcmp(parameter, "--port"))
        {
            char* port_str = argv[i];
            server_port = atoi(port_str);
        }

        else if (!strcmp(parameter, "--server_ip"))
        {
            server_ip = argv[i];
        }

        else
        {
            printf("Unrecognized parameters (%s, %s)", parameter, argv[i]);
        }
    }

    if (server_ip == NULL)
    {
        printf("Should set '--server_ip server_ip' parameter");
        exit(EXIT_FAILURE);
    }

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    // Establishes connection with server
    int status = connect(sockfd, (const struct sockaddr*)&server_addr, sizeof(const struct sockaddr_in));
    if (status == -1)
    {
        perror("Unable to connect");
        exit(EXIT_FAILURE);
    }

    // Creates write thread
    pthread_t write_thread;
    pthread_create(&write_thread, NULL, &write_task, NULL);

    char receive_buffer[BUFFER_SIZE];
    // Continuous loop to send and receive messages
    while (1) {
        

        // Receive response from server
        int bytes_read = read(sockfd, receive_buffer, BUFFER_SIZE);

        if (bytes_read < 0)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }

        // Connection closed by the peer
        if (bytes_read == 0)
        {
            printf("Connection closed by server\n");
            exit(EXIT_SUCCESS);
        }
        // Data successfully read from the socket
        else
        {
            receive_buffer[bytes_read] = '\0'; // Null-terminate the received data
            printf("%s\n", receive_buffer);
            receive_buffer[0] = '\0';
        }

    }

    // Close the socket
    close(sockfd);

    return 0;
}
