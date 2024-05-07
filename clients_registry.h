#ifndef __CLIENTS_REGISTRY_H__
#define __CLIENTS_REGISTRY_H__
#include "arpa/inet.h"
#include "utils.h"

#define MAX_CLIENTS 10

typedef struct
{
    int id;
    int sockfd;
    char* ip;
    char* name;
    struct sockaddr_in address;
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

int is_logged(const Client* client)
{
    return client->ip != 0;
}

/// @return Returns a free client pointer
Client* get_free_client_slot()
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].ip == 0)
            return &clients[i];
    }
    return NULL;
}

/// @brief Regusters client in clients array;
Client* create_client(const struct sockaddr_in* client_address, int client_sock_fd)
{
    Client* client = get_free_client_slot();

    if (client == NULL)
    {
        printf("Max client limit exceeded");
        exit(EXIT_FAILURE);
    }

    char* ip = inet_ntoa(client_address->sin_addr);
    // Sets client data
    client->address = *client_address;
    client->ip = ip;
    client->sockfd = client_sock_fd;
    client->name = malloc(32);
    sprintf(client->name, "Unnamed-%i", client_count);

    client->id = client_count++;
    return client;
}

void update_client(const struct sockaddr_in* client_address, Client* client)
{
    client->address = *client_address;
}

void unregister_client(Client* client)
{
    memset(client, 0, sizeof(Client));
}

void initialize_clients()
{
    memset(clients, 0, sizeof(clients));
}


#endif