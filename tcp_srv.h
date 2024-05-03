/* Timothée M'BASSIDJE && Ethan NICOLAS -- 2024*/

/* Fichier d'entête pour le serveur TCP */

#pragma once // permet d'éviter les inclusions multiples

/* fichiers de la bibliothèque standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
/* bibliothèque standard unix */
#include <unistd.h> /* close, read, write */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <errno.h>
/* spécifique à internet */
#include <arpa/inet.h> /* inet_pton */
/* bibliothèques pour les threads */
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>

/* Définition des constantes */
#define PORT_WCP 4321
#define NB_LIGNES_INFOS 2
#define MAX_USER_NAME_LENGH 32
#define MAX_CONVERSATIONS_PER_USER 64
#define MAX_USERS 128
#define MAX_INFO_SIZE 38

/* Définition des chemins d'accès aux fichiers */
const char *PATH_USER = "./database/users";
const char *PATH_CONV = "./database/conversations";

/* Définitions des types d'énumération pour les tokens */
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

/* Structures de données */
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

/* Prototypes de fonctions pour le serveur */
void usage(char *nom_prog);
int creer_configurer_sock_ecoute(uint16_t port);
void *thread_worker(void *arg);
int validate_login(char *username, char *password);
void hash_password(char *password, char *hashed_password_hex);
void write_login_to_file(char *username, char *password);
char *create_new_conversation_file(char *conv_name);
int addParticipant(char *convId, char *nomconv, char *participant);
int serv_interpreter(query_t *q, masterDb_t *master, int socket);
query_t serv_construire_message(tokens_t token, char *info, char *content);
void envoyer_query(int fd, query_t *q);
int read_until_nl(int fd, char *buf);
void reload_database(masterDb_t *db);
void print_master(masterDb_t *master);
