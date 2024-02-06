#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

extern int errno;
int port;

int main(int argc, char *argv[])
{
    int sd;
    struct sockaddr_in server;
    char message[1024];

    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket().\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Eroare la connect().\n");
        return errno;
    }

    while (1)
    {
        bzero(message, sizeof(message));
        printf("[client] Lista comenzi: <help>,<exit>,<new_card>,<delete_card>,<edit_card>,<search_cards>,<get_card>,<get_cards> \n"
               "Introduceti comanda: \n");
        fflush(stdout);
        fgets(message, sizeof(message), stdin);

        message[strcspn(message, "\n")] = 0;

        if (strncmp(message, "exit", 4) == 0)
        {
            close(sd);
            return 0;
        }

        // salvez in needspassword daca comanda are nevoie de parola pt a face comunicarea in cazul asta mai tarziu
        int needsPassword = (strncmp(message, "new_card", 8) == 0 || strncmp(message, "delete_card", 11) == 0 || strncmp(message, "edit_card", 9) == 0);

        // trimit comanda la server
        if (write(sd, message, strlen(message) + 1) <= 0)
        {
            perror("[client]Eroare la write() spre server.\n");
            return errno;
        }

        // astept raspunsu serverului
        bzero(message, sizeof(message));
        if (read(sd, message, sizeof(message) - 1) < 0)
        {
            perror("[client]Eroare la read() de la server.\n");
            return errno;
        }

        if (strlen(message) > 0)
        {
            printf("[client] %s\n", message);
        }

        // daca am comanda cu parola fac procesarea si comunicarea in cazu asta
        if (needsPassword)
        {
            printf("Introduceti parola: ");
            fgets(message, sizeof(message), stdin);
            message[strcspn(message, "\n")] = 0;

            if (write(sd, message, strlen(message) + 1) <= 0)
            {
                perror("[client]Eroare la write() spre server.\n");
                return errno;
            }

            // astept raspunsu serverului
            bzero(message, sizeof(message));
            if (read(sd, message, sizeof(message) - 1) < 0)
            {
                perror("[client]Eroare la read() de la server.\n");
                return errno;
            }

            if (strlen(message) > 0)
            {
                printf("[client] %s\n", message);
            }
        }
    }
    close(sd);
    return 0;
}