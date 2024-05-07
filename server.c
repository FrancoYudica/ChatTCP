// gcc server.c utils.h clients_registry.h -o server -pthread
#include <arpa/inet.h>
#include "utils.h"
#include "clients_registry.h"
#include "pthread.h"

int server_port = 8000;
int sockfd;
pthread_mutex_t thread_mutex;

void send_to_client(const char* buffer, const Client* client)
{
    if (send(client->sockfd, buffer, strlen(buffer), 0) == -1) {
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }
}

/// @brief Sends message to all clients, except for the sender
void broadcast_message(const char* message, const Client* sender, int echo_to_sender)
{
    // Broadscasts the message to all clients, except sender
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        Client* client = &clients[i];
        if (is_logged(client))
        {
            if (echo_to_sender || client != sender)
                send_to_client(message, client);
        }
    }
}

void on_client_disconnect(Client* client)
{
    char message[BUFFER_SIZE];
    sprintf(message, "SERVER: Client (%s) (%s) disconnected\n", client->name, client->ip);
    printf("%s\n", message);
    broadcast_message(message, client, 0);
    unregister_client(client);
}

void process_command(const char* buffer, Client* client)
{

    // Client disconnected
    if (prefix(buffer, COMMAND_DISCONNECT))
    {
        on_client_disconnect(client);
    }
    else if (prefix(buffer, COMMAND_USERNAME))
    {
        if (!strcmp(buffer, COMMAND_USERNAME))
        {
            char msg[256];  
            sprintf(msg, "SERVER: Your name is %s", client->name);
            send_to_client(msg, client);
        }
        else
        {
            // Changes name
            char* new_name = remove_prefix(strlen("/username "), buffer);

            char message[BUFFER_SIZE];
            sprintf(message, "SERVER: Client (%s) (%s) changed name to (%s)", client->name, client->ip, new_name);
            client->name = new_name;

            // Broadcasts to all clients
            broadcast_message(message, client, 1);
            printf("%s\n", message);
        }
    }
    else if(prefix(buffer, "/connected"))
    {
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            Client* other_client = &clients[i];
            if (is_logged(other_client))
            {
                char client_name[BUFFER_SIZE];
                sprintf(client_name, "SERVER: Client (%s) (%s) connected\n", other_client->name, other_client->ip);
                send_to_client(client_name, client);
            }
        }
    }
    else
        printf("SERVER: Tried to use unrecognized command in message: %s", buffer);
}


void* handle_client(void* args)
{
    Client* client = (Client*)args;
    printf("Handling client %s\n    ", client->name);
    char read_buffer[BUFFER_SIZE];
    while(1)
    {
        int bytes_read = read(client->sockfd, read_buffer, BUFFER_SIZE);
        pthread_mutex_lock(&thread_mutex);

        if (bytes_read < 0)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }

        // Connection closed by the peer
        if (bytes_read == 0)
        {
            on_client_disconnect(client);
            pthread_mutex_unlock(&thread_mutex);
            return NULL;
        }
        // Data successfully read from the socket
        else
        {

            read_buffer[bytes_read] = '\0';

            DEBUG_LOG("SERVER: Client (%i)  (%s) with IP (%s) sent %i bytes\n", client->id, client->name, client->ip, bytes_read);


            // Command
            if (is_command(read_buffer))
                process_command(read_buffer, client);

            // Usual message, broadcasted
            else
            {
                char formatted_message[BUFFER_SIZE + 128];
                sprintf(formatted_message, "%s-: %s", client->name, read_buffer);
                broadcast_message(formatted_message, client, 0);
            }

            read_buffer[0] = '\0';
        }

        pthread_mutex_unlock(&thread_mutex);
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

        else
        {
            printf("Unrecognized parameters (%s, %s)\n", parameter, argv[i]);
        }
    }

    initialize_clients();

    struct sockaddr_in server_addr;
    socklen_t client_len = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;   // Sets the server ip to local ip
    server_addr.sin_port = htons(server_port);

    // Bind socket to the server address
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Sets socket in passive mode, allowing future connections
    // in the previously bound port
    if (listen(sockfd, MAX_CLIENTS) == -1)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("SERVER: Listening. Waiting for client connections...\n");

    char welcome_message[] = "Welcome! You successfully connected to the server";


    // Continuous loop to receive and send messages
    while (1) {

        struct sockaddr_in client_address;

        // Accepts connection
        int client_sock_fd = accept(
            sockfd, 
            (struct sockaddr*)&client_address, 
            &client_len
        );

        if (client_sock_fd == -1) {
            perror("recvfrom failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Gets client
        Client* client = create_client((const struct sockaddr_in*)&client_address, client_sock_fd);

        // Welcomes client
        send_to_client(welcome_message, client);

        // Tells other clients that a new client connected
        {
            char clients_notification[BUFFER_SIZE];
            sprintf(clients_notification, "SERVER: Client (%s) (%s) connected", client->name, client->ip);
            broadcast_message(clients_notification, client, 0);
            printf("%s\n", clients_notification);
        }


        // Creates thread to handle client connection
        pthread_t thread;
        int rc = pthread_create(&thread, NULL, &handle_client, client);
        if (rc)
        {
            perror("Thread create");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

    }

    // Close the socket
    close(sockfd);
    pthread_mutex_destroy(&thread_mutex);

    return 0;
}
