#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <netinet/in.h>

#define PORT 8080

void start_server(int port);
void handle_client(int client_socket, sqlite3 *db);
int execute_sqlite_query(sqlite3 *db, const char *sql, int (*callback)(void*,int,char**,char**), void *data);

int main() {
    start_server(PORT);
    return 0;
}

void start_server(int port) {

    sqlite3 *db;
    sqlite3_stmt *stmt;
    sqlite3_open("login_database.db", &db);

    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Préparer la requête SQL d'insertion
    const char *sql = "INSERT INTO users (username, password) VALUES (?, ?);";

    // Préparer l'instruction
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    // Lier les valeurs à la requête
    const char *username = "admin";
    const char *password = "admin"; // Dans une application réelle, ce devrait être un hachage du mot de passe

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    // Exécuter la requête
    rc = sqlite3_step(stmt);
    if(rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to insert data: %s\n", sqlite3_errmsg(db));
    } else {
        fprintf(stdout, "Data inserted successfully\n");
    }

    printf("Listening on port %d...\n", port);
    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            continue;
        }

        handle_client(client_socket, db);
        close(client_socket);
    }
}

// Prototype pour une fonction callback utilisée par SQLite
static int callback(void *data, int argc, char **argv, char **azColName) {
    int *auth = (int *)data;
    *auth = 1; // Si la requête retourne au moins une ligne, l'authentification est un succès.
    return 0;
}

void handle_client(int client_socket, sqlite3 *db) {
    char buffer[1024] = {0};
    int bytes_read = read(client_socket , buffer, 1024);
    printf("Client sent: %s\n", buffer);

    char *token;
    char *delimiter = ";";
    token = strtok(buffer, delimiter);
    char *username = strtok(NULL, delimiter);
    char *password = strtok(NULL, delimiter);

    if (username != NULL && password != NULL) {
        char sql[1024];
        int auth = 0; // 0 signifie échec de l'authentification par défaut

        // Préparer la requête SQL pour vérifier l'existence de l'utilisateur avec le bon mot de passe
        sprintf(sql, "SELECT * FROM users WHERE username = '%s' AND password = '%s'", username, password);

        // Exécution de la requête
        execute_sqlite_query(db, sql, callback, &auth);

        if (auth) {
            char *message = "Authentication successful\n";
            send(client_socket, message, strlen(message), 0);
        } else {
            char *message = "Authentication failed\n";
            send(client_socket, message, strlen(message), 0);
        }
    } else {
        char *message = "Invalid request\n";
        send(client_socket, message, strlen(message), 0);
    }
}

int execute_sqlite_query(sqlite3 *db, const char *sql, int (*callback)(void*,int,char**,char**), void *data) {
    char *errmsg = 0;
    int rc = sqlite3_exec(db, sql, callback, data, &errmsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
    }
    return rc;
}
