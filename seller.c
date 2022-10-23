#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "utils.c"
#include <stdio.h>
#include "string.h"
#define ACCEPT "Accept"
#define REJECT "Reject"
int setupServer(int port)
{
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));

    listen(server_fd, 4);

    return server_fd;
}

int acceptClient(int server_fd)
{
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t *)&address_len);

    return client_fd;
}
int update_add_suggestions(struct Seller_SaleSuggestion **sale_suggestions,
                           struct Seller_SaleSuggestion *new_suggestion)
{
    // Returns a file descriptor if it sets up a server , otherwise returns -1
    //  returns -1 if not foudn ()
    for (int i = 0; i < MAX_ANNOUNCMENTS; i++)
    {
        if (sale_suggestions[i] == NULL)
        {
            printf("New Port: %d\n", new_suggestion->port);
            new_suggestion->server_fd = setupServer(new_suggestion->port);
            sale_suggestions[i] = new_suggestion;
            return i;
        }
        if (sale_suggestions[i]->port == new_suggestion->port)
        {
            printf("Update With Port :%d\n", new_suggestion->port);
            sale_suggestions[i] = new_suggestion;
            return i;
        }
    }
    return -1;
}
int search_for_suggestion(struct Seller_SaleSuggestion **sale_suggestions,
                          char *suggestion_name)
{

    for (int i = 0; i < MAX_ANNOUNCMENTS; i++)
    {

        if (sale_suggestions[i] != NULL)
        {
            if (!strcmpnl(suggestion_name, sale_suggestions[i]->sale_name))
            {
                return i;
            }
        }
    }
    return -1;
}
void serialize_suggestion(struct Seller_SaleSuggestion *sale_suggestion, char *message)
{

    char str[6] = {0};
    itoa(sale_suggestion->port, str);
    strcat(message, str);
    strcat(message, ",");
    strcat(message, sale_suggestion->sale_name);
    strcat(message, ",");
    // strcat(message,sale_suggestion->seller_name);
    strcat(message, sale_suggestion->state);
}
int main(int argc, char const *argv[])
{
    struct sockaddr_in bc_address;
    struct Seller_SaleSuggestion *sent_sale_suggestions[MAX_ANNOUNCMENTS] = {0};

    int broadcast = 1, opt = 1;
    int new_sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(new_sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(new_sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(8082);
    // bc_address.sin_port = htons(8082);
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");
    bind(new_sock, (struct sockaddr *)&bc_address, sizeof(bc_address));
    char buffer[1024] = {0};
    int max_sd;
    fd_set master_set, working_set;
    max_sd = new_sock;
    FD_ZERO(&master_set);
    FD_SET(new_sock, &master_set);
    FD_SET(0, &master_set);
    while (1)
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);
        for (int i = 0; i <= max_sd; i++)
        {
            if (FD_ISSET(i, &working_set))
            {
                if (i == 0)
                {

                    memset(buffer, 0, 1024);
                    read(0, buffer, 1024);
                    char **tokens = cli_tokenized(buffer);
                    if (!strcmpnl(tokens[0], SEND_SALE))
                    {
                        //Send Comma Seperated Value
                        char sending_message[BUFFER_WORD_LENGTH]={0};
                        strcat(sending_message,tokens[1]);
                        strcat(sending_message,",");
                        strcat(sending_message,OPEN);

                        int a = sendto(new_sock, sending_message, strlen(sending_message), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                        struct Seller_SaleSuggestion *new_sale_suggestion = malloc(sizeof(new_sale_suggestion));
                        return_ssg_struct(sending_message, new_sale_suggestion);
                        int server_fd_index = update_add_suggestions(sent_sale_suggestions, new_sale_suggestion);
                        int server_fd = sent_sale_suggestions[server_fd_index]->server_fd;
                        FD_SET(server_fd, &master_set);
                        if (max_sd < server_fd)
                        {
                            max_sd = server_fd;
                        }
                    }
                    if (strcmp(tokens[0], ACCEPT) == 0 | strcmp(tokens[0],REJECT) == 0)
                    {
                        int suggestion_index = search_for_suggestion(sent_sale_suggestions, tokens[1]);
                        if (suggestion_index == -1)
                        {
                            printf("Sale was not found with sale name:Typo?\n");
                        }
                        else
                        {
                            // Send update
                            if (sent_sale_suggestions[suggestion_index]->current_client_fd == -1)
                            {
                                printf("No one is even negotiating to accept\n");
                            }
                            else
                            {
                                if (strcmpnl(sent_sale_suggestions[suggestion_index]->state, IN_NEGOTIATION) != 0)
                                {
                                    printf("The order is not in negotiation mode\n");
                                }
                                else
                                {
                                    if (strcmp(tokens[0],ACCEPT) == 0)
                                    {
                                        sent_sale_suggestions[suggestion_index]->state = CLOSED;
                                    }
                                    else
                                    {
                                        sent_sale_suggestions[suggestion_index]->state = OPEN;
                                    }
                                    char message[BUFFER_WORD_LENGTH] = {0};
                                    serialize_suggestion(sent_sale_suggestions[suggestion_index], message);
                                    int a = sendto(new_sock, message, strlen(message), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                                    // SEND UPDATE DIRECTCLY
                                    send(sent_sale_suggestions[suggestion_index]->current_client_fd, tokens[0], strlen(tokens[0]), 0);
                                    sent_sale_suggestions[suggestion_index]->current_client_fd = -1;
                                    printf("Sent out latest order Status update to everyone and sent negotiation end\n");
                                }
                            }
                        }
                    }
                }
                for (int j = 0; j < MAX_ANNOUNCMENTS; j++)
                {

                    if (sent_sale_suggestions[j] != NULL & i != 1)
                    {

                        if (sent_sale_suggestions[j]->server_fd == i)
                        {

                            // new client on fd
                            // Setup alaram for 60 seconds inactivity here
                            int current_client_fd = sent_sale_suggestions[j]->current_client_fd;
                            int *initial_opening = malloc(sizeof(int));
                            int *new_current_client_fd = malloc(sizeof(int));
                            if ((!strcmpnl(sent_sale_suggestions[j]->state, OPEN)) & current_client_fd == -1)
                            {
                                printf("Negotiation Initializing\n");
                                int new_fd = acceptClient(sent_sale_suggestions[j]->server_fd);
                                printf("Initial Negotiation beginning with consumer fd %d on port %d\n", new_fd, sent_sale_suggestions[j]->port);
                                char message[BUFFER_WORD_LENGTH] = {0};
                                sent_sale_suggestions[j]->state=IN_NEGOTIATION;
                                serialize_suggestion(sent_sale_suggestions[j], message);
                                int a = sendto(new_sock, message, strlen(message), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                                sent_sale_suggestions[j]->current_client_fd = current_client_fd;
                                *initial_opening = 1;
                                *new_current_client_fd = new_fd;
                            }

                            if (*initial_opening)
                            {
                                sent_sale_suggestions[j]->current_client_fd = *new_current_client_fd;
                                FD_SET(*new_current_client_fd, &master_set);
                                if (max_sd < *new_current_client_fd)
                                {
                                    max_sd = *new_current_client_fd;
                                }
                                printf("A New client on port was discovered: %d with server fd:%d with client fd: %d\n", sent_sale_suggestions[j]->port,
                                       sent_sale_suggestions[j]->server_fd, *new_current_client_fd);
                            }
                            else
                            {
                                // CLOSE AND TEST THE RESPECTVIE FILD DESCRIPTOR
                                printf("Contention on one order,Seller is already negotiating with buyer fd: %d  port: %d\n ", sent_sale_suggestions[j]->current_client_fd, sent_sale_suggestions[j]->port);
                                int reject_fd = acceptClient(sent_sale_suggestions[j]->server_fd);
                                close(reject_fd);
                            }
                        }
                    }
                }

                for (int z = 0; z < MAX_ANNOUNCMENTS; z++)
                {

                    if (sent_sale_suggestions[z] != NULL)
                    {

                        if (sent_sale_suggestions[z]->current_client_fd == i &
                            sent_sale_suggestions[z]->current_client_fd != -1 & sent_sale_suggestions[z]->current_client_fd != 1)
                        {

                            char bw[BUFFER_WORD_LENGTH];
                            int bytes_received;
                            bytes_received = recv(i, bw, 1024, 0);
                            if (bytes_received == 0)
                            { // End of file from consumer
                                printf("client fd : %d Closed it's connection abruptly in negotiation\n", i);
                                sent_sale_suggestions[z]->state=OPEN;
                                char message[BUFFER_WORD_LENGTH] = {0};
                                serialize_suggestion(sent_sale_suggestions[z], message);
                                int a = sendto(new_sock, message, strlen(message), 0, (struct sockaddr *)&bc_address, sizeof(bc_address));
                                close(i);
                                FD_CLR(i, &master_set);
                                sent_sale_suggestions[z]->current_client_fd = -1; // no active client fd
                                continue;
                            }

                            printf("A client message discovered with fd %d message was :%s\n", sent_sale_suggestions[z]->current_client_fd, bw);
                            // TODO Log better here
                        }
                    }
                }
            }
        }
    }

    return 0;
}