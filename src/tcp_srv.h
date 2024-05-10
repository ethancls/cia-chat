/* Timothée M'BASSIDJE && Ethan NICOLAS -- 2024*/

/* TCP Server HEADER */

#pragma once

/* Standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
/* UNIX Standard */
#include <unistd.h> /* close, read, write */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <errno.h>
/* Internet */
#include <arpa/inet.h> /* inet_pton */
/* Threads - Signal */
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>

/* Constants */
#define PORT_WCP 4321
#define NB_LIGNES_INFOS 2
#define MAX_USER_NAME_LENGH 32
#define MAX_CONVERSATIONS_PER_USER 32
#define MAX_USERS 128
#define MAX_INFO_SIZE 38

/* FILEPATHS */
const char *PATH_USER = "./database/users";
const char *PATH_CONV = "./database/conversations";

/* TOKENS */
typedef enum tokens
{
    LOG,
    SIGNIN,
    SEND,
    UPDATE,
    CREATE,
    ADD,
    CONV,
    USERID,
    LOG_OK,
    LOG_FAILED,
    DENIED,
    OK,
    SENDING_TRAFFIC
} tokens_t;

/* Data structures */
typedef struct
{
    void *tab;
    uint16_t size;
    size_t type;
} dymTab_t;

typedef struct
{
    char *userID;
    char **conversationID;
    char **conversationName;
    int nbConv;
} UserData_t;

typedef struct
{
    char *content;
    uint16_t size;
} query_t;

typedef struct
{
    UserData_t **User;
    int nbUser;
} masterDb_t;

typedef struct
{
    int fd;
    masterDb_t *dbd;
} work_args;

/* Main server functions */
int main(int argc, char *argv[]);
void *thread_worker(void *arg);

/* Utility functions */
void usage(char *nom_prog);
tokens_t convert_to_request(const char *str);
int read_until_nl(int fd, char *buf);
void to_hex_string(unsigned char *hash, char *output, size_t length);

/* Database functions */
void hash_password(char *password, char *hashed_password_hex);
int validate_login(char *username, char *password);
void write_login_to_file(char *username, char *password);
char rand_char();
void generate_random_id(char *id, size_t size);
char *create_new_conversation_file(char *conv_name);
int addParticipant(char *convId, char *nomconv, char *participant);
void reload_database(masterDb_t *dbd);
void flushDatabase(masterDb_t *dbd);
void print_master(masterDb_t *master);

/* Network communication functions */
query_t serv_construire_message(tokens_t token, char *info, char *content);
void envoyer_query(int fd, query_t *q);
void write_query_end(query_t *q, char *wr);
int creer_configurer_sock_ecoute(uint16_t port);

/* Interpreter */
int serv_interpreter(query_t *q, masterDb_t *master, int socket);

/* Validation */
int is_contact_valid(char *contact_name);