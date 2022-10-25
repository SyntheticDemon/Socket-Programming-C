#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include "utils.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#define LIST_COMMAND "list"
#define START_NEGOTIATE "negotiate"
#define TIMER 60
// Endpoint Serialization and Deserialization is done by csv
// Each struct is serialized like this

int close_connection = 0;

int current_client_after_neg_fd = -1;

int search_and_update_received_sale_suggestions(struct SaleSuggestion **sale_suggestions,
                                                struct SaleSuggestion *new_suggestion)
{

    // returns -1 if not foudn ()
    for (int i = 0; i < MAX_ANNOUNCMENTS_TO_KEEP; i++)
    {
        if (sale_suggestions[i] == NULL)
        {
            sale_suggestions[i] = new_suggestion;
            return i;
        }
        if (!strcmpnl(sale_suggestions[i]->sale_name, new_suggestion->sale_name))
        {

            char temp[BUFFER_WORD_LENGTH];
            sprintf(temp, "Sale Update Received Seller name :%s\n", sale_suggestions[i]->sale_name);
            write(0, temp, strlen(temp));
            sale_suggestions[i] = new_suggestion;
            return i;
        }
    }
    return -1;
}
void **print_current_announcments(struct SaleSuggestion **sale_suggestions)
{
    for (int i = 0; i < MAX_ANNOUNCMENTS_TO_KEEP; i++)
    {
        if (sale_suggestions[i] != NULL)
        {
            char temp[BUFFER_WORD_LENGTH];
            sprintf(temp, "Sale Descrption\n Seller Name: %s,Port:%d,Sale Name:%s,Status:%s ======\n",
                    sale_suggestions[i]->seller_name,sale_suggestions[i]->port, sale_suggestions[i]->sale_name, sale_suggestions[i]->state);
            write(0, temp, strlen(temp));
        }
        else
        {
            if (i == 0)
            {
                writeto_stdout_vanilla("%s\n", "No sales");
            }
            break;
        }
    }
}

int search_for_suggestion(struct SaleSuggestion **sale_suggestions,
                          int port)
{

    for (int i = 0; i < MAX_ANNOUNCMENTS_TO_KEEP; i++)
    {

        if (sale_suggestions[i] != NULL)
        {
            if (sale_suggestions[i]->port == port)
            {
                return i;
            }
        }
    }
    return -1;
}
int connectServer(int port)
{
    int fd;
    struct sockaddr_in server_address;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        write(fd, "Error in connecting to server\n", strlen("Error in connecting to server\n"));
    }

    return fd;
}
void alarm_handler(int sig)
{
    shutdown(current_client_after_neg_fd, SHUT_RDWR);
    writeto_stdout_vanilla("%s", "No Response Received cutting the connection out\n");
}

int main(int argc, char const *argv[])
{
    int sock, broadcast = 1, opt = 1;
    
    char socket_buffer[1024] = {0};
    char terminal_buffer[1024] = {0};
    
    char buyer_name[BUFFER_WORD_LENGTH];
    strcpy(buyer_name,argv[2]);
    
    int announcments_count_up_until_now;
    
    struct SaleSuggestion *sale_suggestions[MAX_ANNOUNCMENTS_TO_KEEP] = {0};
    struct sockaddr_in bc_address;
    
    int server_fd, max_sd;
    fd_set master_set, working_set;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    // bc_address.sin_port = htons(atoi(argv[1]));
    bc_address.sin_port = htons(atoi(argv[1]));
    bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");
    FD_ZERO(&master_set);
    max_sd = sock;
    FD_SET(sock, &master_set);
    FD_SET(0, &master_set);

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));
    int *in_negotiation = malloc(sizeof(int));
    *in_negotiation = 0;
    while (1)
    {

        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int i = 0; i <= max_sd; i++)
        {

            if (FD_ISSET(i, &working_set) | i == current_client_after_neg_fd)
            {

                if (i == sock)
                { // new message
                    memset(socket_buffer, 0, 1024);
                    recv(sock, socket_buffer, 1024, 0);
                    struct SaleSuggestion *new_sale_suggestion = malloc(sizeof(new_sale_suggestion));
                    return_sg_struct(socket_buffer, new_sale_suggestion);
                    search_and_update_received_sale_suggestions(sale_suggestions, new_sale_suggestion);
                    FD_SET(sock, &master_set);
                    if (sock > max_sd)
                        max_sd = sock;
                    char temp[BUFFER_WORD_LENGTH];
                    sprintf(temp, "New Message Received State might be updated fd: %d \n", sock);
                    write(0, temp, strlen(temp));
                }
                if (i == 0)
                { // stdin
                    memset(terminal_buffer, 0, 1024);
                    read(0, terminal_buffer, 1024);
                    char **tokens = cli_tokenized(terminal_buffer);
                    if (!strcmpnl(tokens[0], LIST_COMMAND))
                    {
                        print_current_announcments(sale_suggestions);
                    }
                    else
                    {
                        if (*in_negotiation == 0)
                        {
                            if (!strcmpnl(tokens[0], START_NEGOTIATE))
                            {
                                int seller_port = atoi(tokens[1]);
                                server_fd = connectServer(seller_port);
                                char temp[BUFFER_WORD_LENGTH];
                                char negotiation_message[BUFFER_WORD_LENGTH]={0};
                                strcat(negotiation_message, "Name: ");
                                strcat(negotiation_message, buyer_name);
                                strcat(negotiation_message, " Negotiation Message:");
                                strcat(negotiation_message, tokens[2]);   
                                sprintf(temp, "Sending Negotiation to seller on port %d and server_fd %d \n", seller_port, server_fd);
                                write(0, temp, strlen(temp));
                                send(server_fd, negotiation_message, strlen(negotiation_message), 0); // tokens [2] is the buyer message
                                FD_SET(server_fd, &master_set);
                                if (server_fd > max_sd)
                                {
                                    max_sd = server_fd;
                                }
                                int returned_index = search_for_suggestion(sale_suggestions, seller_port);
                                sale_suggestions[returned_index]->seller_fd = server_fd;
                                current_client_after_neg_fd = server_fd;
                            }
                        }

                        else
                        {
                            char temp[BUFFER_WORD_LENGTH];
                            writeto_stdout_vanilla("%s", "You are already negotiating\n");
                        }
                    }
                }
                for (int t = 0; t < MAX_ANNOUNCMENTS_TO_KEEP; t++)
                {
                    if (sale_suggestions[t] != NULL)
                    {

                        if (sale_suggestions[t]->seller_fd == i)
                        {
                            char seller_msg_buff[BUFFER_WORD_LENGTH];
                            signal(SIGALRM, alarm_handler);
                            alarm(TIMER);

                            int bytes_received = recv(server_fd, seller_msg_buff, BUFFER_WORD_LENGTH, 0);
                            if (bytes_received == 0)
                            {
                                *in_negotiation = 0;
                                close(server_fd);
                                FD_CLR(i, &master_set);
                                alarm(0);
                            }
                            else
                            {
                                char temp[BUFFER_WORD_LENGTH];
                                if (strcmpnl(seller_msg_buff, "Accept") == 0)
                                {
                                    sprintf(temp, "Buy Request was Accepted\n");
                                    write(0, temp, strlen(temp));
                                }
                                else if (strcmp(seller_msg_buff, "Reject") == 0)
                                {
                                    sprintf(temp, "Buy Request was Rejected\n");
                                    write(0, temp, strlen(temp));
                                }
                                alarm(0);
                                *in_negotiation = 0;
                                close(server_fd);

                                FD_CLR(i, &master_set);
                            }
                            current_client_after_neg_fd = -1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
