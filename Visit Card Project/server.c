#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sqlite3.h>
#define PORT 2024
#define RESPONSE_SIZE 4096
extern int errno;
sqlite3 *db;
char *err_msg = 0;
const char *passwd = "rc2024";
int checkpass(const char *pass)
{
    if (strcmp(passwd, pass) == 0)
        return 1;
    return 0;
}
int callback(void *data, int num_fields, char **fields, char **col_names)
{
    char *response = (char *)data;
    char line[200];

    for (int i = 0; i < num_fields; i++)
    {
        snprintf(line, sizeof(line), "%s: %s\n", col_names[i], fields[i] ? fields[i] : "NULL");
        // verific daca e suficient loc pe buffer
        if (strlen(response) + strlen(line) < RESPONSE_SIZE)
        {
            strncat(response, line, sizeof(line) - strlen(response) - 1);
        }
        else
        {
            // daca bufferul e plin ma opresc
            strcat(response, "\n... Lista i prea lunga \n");
            return 0; 
        }
    }
    strcat(response, "\n");
    return 0;
}
void reset_autoincrement(sqlite3 *db)
{
    const char *sql = "DELETE FROM sqlite_sequence WHERE name='Cards';";
    char *err_msg = 0;

    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
}

void search_cards(sqlite3 *db, const char *searchQuery, char *response)
{
    char sql[1024];
    snprintf(sql, sizeof(sql), "SELECT * FROM Cards WHERE Name LIKE '%%%s%%' OR Address LIKE '%%%s%%' OR Email LIKE '%%%s%%';", searchQuery, searchQuery, searchQuery);
    sqlite3_stmt *stmt;
    char localResponse[RESPONSE_SIZE] = "";
    if (sqlite3_exec(db, sql, callback, localResponse, &err_msg) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        snprintf(response, RESPONSE_SIZE, "Eroare la interogare: %s", err_msg);
        sqlite3_free(err_msg);
    }
    else
    {
        if (strlen(localResponse) == 0)
        {
            snprintf(response, RESPONSE_SIZE, "Nu exista asemenea carduri!");
        }
        else
        {
            strncpy(response, localResponse, RESPONSE_SIZE);
        }
    }
}

void get_cards(sqlite3 *db, char *response)
{
    const char *sql = "SELECT * FROM Cards";
    char localResponse[RESPONSE_SIZE] = ""; 

    if (sqlite3_exec(db, sql, callback, localResponse, &err_msg) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        snprintf(response, RESPONSE_SIZE, "Eroare la interogare: %s", err_msg);
        sqlite3_free(err_msg);
    }
    else
    {
        strncpy(response, localResponse, RESPONSE_SIZE);
    }
}
void get_card(sqlite3 *db, int id, char *response)
{
    char sql[200];
    snprintf(sql, sizeof(sql), "SELECT * FROM Cards WHERE ID = %d;", id);

    char *err_msg = 0;
    char localResponse[1024] = "";

    if (sqlite3_exec(db, sql, callback, localResponse, &err_msg) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        snprintf(response, 1024, "Eroare la interogare: %s", err_msg);
        sqlite3_free(err_msg);
    }
    else
    {
        if (strlen(localResponse) == 0)
        {
            snprintf(response, 1024, "Nu s-a gasit cardul cu ID-ul %d.", id);
        }
        else
        {
            strncpy(response, localResponse, 1024);
        }
    }
}

void new_card(sqlite3 *db, const char *card_details, char *response)
{
    char *token;
    char name[50], address[100], email[50];

    token = strtok((char *)card_details, ",");
    if (token == NULL)
    {
        snprintf(response, 1024, "Format incorect. Utilizati: new_card,<Nume>,<Adresa>,<Email>");
        return;
    }
    token = strtok(NULL, ",");

    if (token == NULL)
    {
        snprintf(response, 1024, "Format incorect. Utilizati: new_card,<Nume>,<Adresa>,<Email>");
        return;
    }
    strncpy(name, token, sizeof(name));

    token = strtok(NULL, ",");
    if (token == NULL)
    {
        snprintf(response, 1024, "Format incorect. Utilizati: new_card,<Nume>,<Adresa>,<Email>");
        return;
    }
    strncpy(address, token, sizeof(address));

    token = strtok(NULL, ",");
    if (token == NULL)
    {
        snprintf(response, 1024, "Format incorect. Utilizati: new_card,<Nume>,<Adresa>,<Email>");
        return;
    }
    strncpy(email, token, sizeof(email));

    char sql[1024];
    snprintf(sql, sizeof(sql), "INSERT INTO Cards (Name, Address, Email) VALUES ('%s', '%s', '%s');", name, address, email);

    if (sqlite3_exec(db, sql, NULL, 0, &err_msg) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        snprintf(response, 1024, "Eroare SQL: %s", err_msg);
        sqlite3_free(err_msg);
    }
    else
    {
        snprintf(response, 1024, "Card adaugat cu succes.");
    }
}
void edit_card(sqlite3 *db, int id, const char *name, const char *address, const char *email, char *response)
{
    char sql[512];
    snprintf(sql, sizeof(sql), "UPDATE Cards SET Name = '%s', Address = '%s', Email = '%s' WHERE ID = %d;", name, address, email, id);
    sqlite3_stmt *stmt;
    char *err_msg = 0;
    if (sqlite3_exec(db, sql, NULL, 0, &err_msg) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        snprintf(response, 1024, "Eroare SQL: %s", err_msg);
        sqlite3_free(err_msg);
    }
    else if (sqlite3_changes(db) == 0)
    {
        snprintf(response, 1024, "Nu exista cardul cu ID-ul %d", id);
    }
    else
    {
        snprintf(response, 1024, "Card cu ID-ul %d a fost actualizat cu succes.", id);
    }
}

void delete_card(sqlite3 *db, int id, char *response)
{
    char sql[100];
    snprintf(sql, sizeof(sql), "DELETE FROM Cards WHERE ID = %d;", id);

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        snprintf(response, 1024, "Eroare SQL: %s", sqlite3_errmsg(db));
        return;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        snprintf(response, 1024, "Eroare la stergere: %s", sqlite3_errmsg(db));
    }
    else if (sqlite3_changes(db) == 0)
    {
        snprintf(response, 1024, "Nu exista card cu ID-ul %d.", id);
    }
    else
    {
        snprintf(response, 1024, "Card cu ID-ul %d a fost sters cu succes.", id);
    }

    sqlite3_finalize(stmt);
}

int main()
{
    struct sockaddr_in server;
    struct sockaddr_in from;
    char message[1024];
    char responsemsg[1024] = " ";
    int sd;
    int dbmsg= sqlite3_open("server_database.db", &db);
    if (dbmsg!= SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    const char *sql = "CREATE TABLE IF NOT EXISTS Cards(ID INTEGER PRIMARY KEY AUTOINCREMENT, Name TEXT, Address TEXT, Email TEXT);";
    dbmsg= sqlite3_exec(db, sql, 0, 0, &err_msg);
    const char *countQuery = "SELECT COUNT(*) FROM Cards;";
    sqlite3_stmt *res;
    int count = 0;

    dbmsg= sqlite3_prepare_v2(db, countQuery, -1, &res, 0);

    if (dbmsg== SQLITE_OK)
    {
        if (sqlite3_step(res) == SQLITE_ROW)
        {
            count = sqlite3_column_int(res, 0);
        }
    }
    sqlite3_finalize(res);

    if (count == 0) //daca tabelu i gol inserez niste valori default
    {
        reset_autoincrement(db);
        const char *insert1 = "INSERT INTO Cards (Name, Address, Email) VALUES ('Ana Maria', 'Dimitrie Ralet', 'anamaria@gmail.com');";
        const char *insert2 = "INSERT INTO Cards (Name, Address, Email) VALUES ('Damian', 'strada x', 'damian@gmail.com');";

        dbmsg= sqlite3_exec(db, insert1, 0, 0, &err_msg);
        dbmsg= sqlite3_exec(db, insert2, 0, 0, &err_msg);

        if (dbmsg!= SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    }

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    if (listen(sd, 5) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    while (1)
    {
        int client;
        socklen_t length = sizeof(from);

        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        client = accept(sd, (struct sockaddr *)&from, &length);

        if (client < 0)
        {
            perror("[server]Eroare la accept().\n");
            continue;
        }

        int pid;
        if ((pid = fork()) == -1)
        {
            close(client);
            continue;
        }
        else if (pid > 0)
        {
            // parinte
            close(client);
            continue;
        }
        else if (pid == 0)
        { // copil
            close(sd);

            while (1)
            { 
                bzero(message, sizeof(message));
                int read_result = read(client, message, sizeof(message) - 1);
                if (read_result <= 0)
                {
                    if (read_result == 0)
                    {
                        printf("[server]Clientul a inchis conexiunea.\n");
                    }
                    else
                    {
                        perror("[server]Eroare la read() de la client.\n");
                    }
                    close(client); 
                    break;         
                }

                char *end;
                end = strpbrk(message, "\n\r");
                if (end)
                    *end = '\0';

                printf("[server]Comanda primita: %s\n", message);
                bzero(responsemsg, sizeof(responsemsg));

                if (strncmp(message, "exit", 4) == 0)
                {
                    printf("[server]Clientul a solicitat închiderea conexiunii.\n");
                    close(client);
                    break;
                }
                else if (strncmp(message, "new_card", 8) == 0)
                {
                    char cardDetails[1024];
                    strcpy(cardDetails, message); //aici salvez detaliile cardului ca sa nu se piarda cand o sa citesc parola

                    strcpy(responsemsg, "Introduceti parola pentru a adauga cardul:");
                    write(client, responsemsg, strlen(responsemsg) + 1);
                    bzero(responsemsg, sizeof(responsemsg));
                    bzero(message, sizeof(message));
                    read(client, message, sizeof(message) - 1); // citesc parola

                    if (checkpass(message))
                    {
                        char response[1024] = "";
                        new_card(db, cardDetails, response); // adaug cardu nou cu detaliile de le am salvat mai sus
                        snprintf(responsemsg, sizeof(responsemsg), "%s", response);
                    }
                    else
                    {
                        snprintf(responsemsg, sizeof(responsemsg), "Parola incorecta. Cardul nu a fost adaugat.");
                    }
                    bzero(message, sizeof(message));
                }

                else if (strncmp(message, "get_cards", 9) == 0)
                {
                    bzero(responsemsg, sizeof(responsemsg));
                    char response[1024] = "";
                    get_cards(db, response);
                    snprintf(responsemsg, sizeof(responsemsg), "Cardurile sunt:\n%s", response);
                }
                else if (strncmp(message, "search_cards", 12) == 0)
                {
                    char search[100];
                    if (sscanf(message, "search_cards %s", search) == 1)
                    {
                        char response[1024] = "";
                        search_cards(db, search, response);
                        snprintf(responsemsg, sizeof(responsemsg), "%s", response);
                    }
                    else
                        snprintf(responsemsg, sizeof(responsemsg), "Format incorect.Utilizati: search_cards <text>");
                }
                else if (strncmp(message, "get_card", 8) == 0)
                {
                    int id;
                    bzero(responsemsg, sizeof(responsemsg));
                    if (sscanf(message, "get_card %d", &id) == 1)
                    {
                        char response[1024] = "";
                        get_card(db, id, response);
                        snprintf(responsemsg, sizeof(responsemsg), "%s", response);
                    }
                    else
                    {
                        snprintf(responsemsg, sizeof(responsemsg), "Format incorect. Utilizati: get_card<ID>");
                    }
                }
                else if (strncmp(message, "delete_card", 11) == 0)
                {
                    int id;
                    if (sscanf(message, "delete_card %d", &id))
                    {
                        strcpy(responsemsg, "Introduceti parola pentru a sterge cardul:");
                        write(client, responsemsg, strlen(responsemsg) + 1);
                        bzero(responsemsg, sizeof(responsemsg));
                        bzero(message, sizeof(message));
                        read(client, message, sizeof(message) - 1);

                        if (checkpass(message))
                        {
                            char response[1024] = "";
                            delete_card(db, id, response);
                            snprintf(responsemsg, sizeof(responsemsg), "%s", response);
                        }
                        else
                        {
                            snprintf(responsemsg, sizeof(responsemsg), "Parola incorecta");
                        }
                    }
                    else
                    {
                        snprintf(responsemsg, sizeof(responsemsg), "Format incorect!");
                    }
                    // write(client,responsemsg,sizeof(responsemsg));
                    bzero(message, sizeof(message));
                }
                else if (strncmp(message, "edit_card", 9) == 0)
                {
                    int id;
                    char name[50], address[100], email[50];
                    sscanf(message, "edit_card,%d,%49[^,],%99[^,],%49s", &id, name, address, email);
                    strcpy(responsemsg, "Introduceti parola pentru a edita cardul:");
                    write(client, responsemsg, strlen(responsemsg) + 1);
                    bzero(responsemsg, sizeof(responsemsg));
                    bzero(message, sizeof(message));
                    read(client, message, sizeof(message) - 1);

                    if (checkpass(message))
                    {
                        char response[1024] = "";
                        edit_card(db, id, name, address, email, response);
                        snprintf(responsemsg, sizeof(responsemsg), "%s", response);
                    }
                    else
                    {
                        snprintf(responsemsg, sizeof(responsemsg), "Parola gresita!");
                    }
                    // write(client,responsemsg,sizeof(responsemsg));
                    bzero(message, sizeof(message));
                }
                else if (strncmp(message, "help", 4) == 0)
                {
                    snprintf(responsemsg, sizeof(responsemsg), "Pentru a afla informatii despre un card specific dupa id: <get_card> \n"
                                                       "Pentru a afisa toata lista carduri: <get_cards> \n"
                                                       "Pentru a cauta carduri ce contin un anumit cuvant <search_cards> <text>\n"
                                                       "Pentru ajutor: <help> \n"
                                                       "Pentru a sterge: <delete_card> <id> \n"
                                                       "Pentru a modifica: <edit_card>,<id>,<nume>,<adresa>,<email> \n"
                                                       "Pentru a insera card nou: <new_card>,<Nume>,<Adresa>,<Email>");
                }
                else
                {
                    snprintf(responsemsg, sizeof(responsemsg), "Comanda necunoscuta");
                }
                if (write(client, responsemsg, strlen(responsemsg) + 1) <= 0)
                {
                    perror("[server]Eroare la write() catre client.\n");
                    break;
                }
                bzero(message, sizeof(message));
                bzero(responsemsg, sizeof(responsemsg));
            }

            close(client); // inchid socketu
            exit(0);       // inchid copilu
        }
    }
    sqlite3_close(db);
}
