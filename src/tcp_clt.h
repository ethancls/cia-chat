/* Timothée M'BASSIDJE && Ethan NICOLAS -- 2024*/

/* Fichier d'entête pour le client TCP */

#pragma once // permet d'éviter les inclusions multiples

/* bibliothèques standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
/* bibliothèque standard unix */
#include <unistd.h> /* close, read, write */
#include <sys/types.h>
#include <sys/socket.h>
/* spécifique à internet */
#include <arpa/inet.h>  /* inet_pton */
#include <netinet/in.h> /* struct sockaddr_in */

/* DEBIND PORT MACOS : sudo lsof -P -i :PORT and kill -9 <PID> */

/* Définitions des constantes */
#define PORT_WCP 4321
#define CONTENT_MAX_SIZE 64
#define MAX_CONVERSATIONS_PER_USER 32
#define CONTENT_MAX_NB 32
#define VIGINERE_KEY "BAYOFPIGS"

/* Définition des structures de données */
typedef struct query
{
    char *content;
    uint16_t size;
} query_t;

typedef struct conversation
{
    char *nom;
    char *id_deconv;
} convo_t;

typedef struct feed
{
    convo_t f_conv[1024];
    uint32_t f_nbConv;
} feed_t;

typedef struct utilisateur
{
    char *u_pseudo;
    char *password;
    convo_t **conversation;
    int nb_conv;
} user_t;

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
    SENDING_TRAFFIC,
} tokens_t;

/* Prototypes de fonctions */

void usage(char *nom_prog);
void maj();
int creer_connecter_sock(char *addr_ipv4, uint16_t port);
void write_query_end(query_t *q, char *wr);
void envoyer_query(int fd, query_t *q);
query_t nouvelle_conversation(uint32_t myId);
int interpreter_message(int fd, char **data);
query_t construire_message(tokens_t inst, char *username, char *content);
void envoyer_message(int fd, char *message);
int read_until_nl(int fd, char *buf);
void envoyerMessage(user_t *u, int sock, char **content);
void voirConversation(user_t *u, int sock, char **content);
void creerConversation(user_t *u, int sock, char **content);
void miseAJourServeur(user_t *user, int sock, char **content);
void menu(user_t *u, int sock, char **content);
tokens_t convert_to_request(const char *str);
void vigenere(char *text, char *key, int encrypt);
