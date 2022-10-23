#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#define CONSTANT_SIZE_STR 310
#define MAX_ANNOUNCMENTS 3
#define BUFFER_WORD_LENGTH 256
#define MAX_BUFFER_WORDS 500
#define NULL ((void *)0)
#define OPEN "Open"
#define CLOSED "Closed"
#define IN_NEGOTIATION "Negotiation"
#define SEND_SALE "Send_Sale"

int itoa(int value, char *ptr)
{
    int count = 0, temp;
    if (ptr == NULL)
        return 0;
    if (value == 0)
    {
        *ptr = '0';
        return 1;
    }

    if (value < 0)
    {
        value *= (-1);
        *ptr++ = '-';
        count++;
    }
    for (temp = value; temp > 0; temp /= 10, ptr++)
        ;
    *ptr = '\0';
    for (temp = value; temp > 0; temp /= 10)
    {
        *--ptr = temp % 10 + '0';
        count++;
    }
    return count;
}
int strcmpnl(const char *s1, const char *s2)
{
    char s1c;
    char s2c;
    if (abs(strlen(s1) - strlen(s2)) > 2){}
        do
        {
            s1c = *(s1++);
            s2c = *(s2++);
            if (s1c == '\n')
                s1c = 0;
            if (s2c == '\n')
                s2c = 0;
            if (s1c != s2c)
                return 1;
        } while (s1c);

    return 0;
}
struct SaleSuggestion
{
    int port;
    // char* seller_name;
    char* sale_name;
    char* state;
    int seller_fd;
};

struct Seller_SaleSuggestion
{
    int port;
    int server_fd;
    int current_client_fd;
    // char* seller_name;
    char* sale_name;
    char* state;
    char * last_buyer_message;
};
char **cli_tokenized(char *input)
{
    int action = 0;
    char **tokens = malloc(MAX_BUFFER_WORDS * sizeof(char *));
    char *p = strtok(input," ");
    int token_count = 1;
    tokens[0] = malloc(BUFFER_WORD_LENGTH * sizeof(char));
    strcpy(tokens[0], p);
    while (p != NULL)
    {
        tokens[token_count] = malloc(BUFFER_WORD_LENGTH * sizeof(char));
        p = strtok(NULL," ");
        if (p != NULL)
            tokens[token_count] = p;
        token_count++;
    }
    return tokens;
}
void return_ssg_struct(char *str_input, struct Seller_SaleSuggestion *new_sale_suggestion)
{

    char *input_string = strdup(str_input);
    char *p = strtok(input_string, ",");
    int token_count = 1;
    char tokens[4][256]={0};
    strcpy(tokens[0], p);
    while (p != NULL)
    {
        p = strtok(NULL, ",");
        if (p != NULL)
            strcpy(tokens[token_count], p);
        token_count++;
    }
    new_sale_suggestion->port = atoi(tokens[0]);
    new_sale_suggestion->sale_name = strdup(tokens[1]);
    // new_sale_suggestion->seller_name = strdup(tokens[2]);
    new_sale_suggestion->state = OPEN;
    new_sale_suggestion->current_client_fd=-1; //temp descriptor
    new_sale_suggestion->server_fd=-1;
}
void return_sg_struct(char *str_input,struct SaleSuggestion* new_sale_suggestion)
{

    char *input_string = strdup(str_input);
    char *p = strtok(input_string, ",");
    int token_count = 1;
    char tokens[4][256]={0};
    strcpy(tokens[0], p);
    while (p != NULL)
    {
        p = strtok(NULL, ",");
        if (p != NULL)
            strcpy(tokens[token_count], p);
        token_count++;
    }
    new_sale_suggestion->port = atoi(tokens[0]);
    new_sale_suggestion->sale_name = strdup(tokens[1]);
    // new_sale_suggestion->seller_name=strdup(tokens[2]);
    new_sale_suggestion->state = strdup(tokens[2]);
    new_sale_suggestion->seller_fd=-1;
}