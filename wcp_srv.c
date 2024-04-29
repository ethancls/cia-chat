/* Timothée M'BASSIDJE 12104516
Je déclare qu'il s'agit de mon propre travail. */
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
/* spécifique aux comptines */

// MACOS : gcc wcp_srv.c -o wcp_srv -I/opt/homebrew/opt/openssl@3/include -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto
// LINUX : gcc wcp_srv.c -o wcp_srv -lssl -lcrypto

#include <pthread.h>
#include <semaphore.h>

#define PORT_WCP 4321

#define NB_LIGNES_INFOS 2

#define RECEIVED_CLIENT 10

const char *PATH_USER = "./database/users";

const char *PATH_CONV = "./database/conversations";

typedef enum
{
	LOG,
	SEND,
	UPDATE,
	CREATE,
	ADD,
	OKR,

} request_e;

typedef enum
{
	CONV,
	USERID,
	LOG_OK,
	LOG_FAILED,
	SENDING_TRAFFIC
	DENIED,
	OKS,

} serveur_e;

typedef struct dymTab
{
	void *tab;
	uint16_t size;
	size_t type;
} dymTab_t;

typedef struct UserData
{
	char *userID;
	char **conversationID;
	int nbConv;

} UserData_t;

typedef struct query_
{
	char content[2048]; // taille temporaire
	uint16_t size;
} query_t;

typedef struct masterDb
{
	UserData_t **User;
	int nbUser;
} masterDb_t;

// structure utiliser pour les threads
typedef struct work_args
{
	int fd;
	masterDb_t *dbd;
} work_args;

void print_master(masterDb_t *master);
int serv_interpreter(query_t *q, masterDb_t *master, int socket);
int validate_login(char *username, char *password);

request_e convert_to_request(const char *str)
{
	if (strcmp(str, "LOG") == 0)
	{
		return LOG;
	}
	else if (strcmp(str, "SEND") == 0)
	{
		return SEND;
	}
	else if (strcmp(str, "UPDATE") == 0)
	{
		return UPDATE;
	}
	else if (strcmp(str, "CREATE") == 0)
	{
		return CREATE;
	}
	else if (strcmp(str, "ADD") == 0)
	{
		return ADD;
	}
	else if (strcmp(str, "OKR") == 0)
	{
		return OKR;
	}
	else
	{
		return -1; // Indicate an error or handle unknown values accordingly
	}
}

int read_until_nl(int fd, char *buf)
{
	int numChar = 0;
	char *readChar = malloc(sizeof(char)); // malloc obligatoire pour pouvoir utiliser read(read ne peut pas ecrire dans le stack ?)
	int n;
	int dansguillemet = 0;
	while ((n = read(fd, readChar, sizeof(char))) > 0)
	{	
		if(readChar == '\"'){
			dansguillemet = !dansguillemet; //ignorer les \n si dans les guillemets
		}

		if (*readChar == '\n' && !dansguillemet)
		{							 // quand on arrive a '0' on retourne
			*(buf + numChar) = '\0'; // obligatoire sino buffer overflow
			return numChar;
		}
		*(buf + numChar) = *readChar; // on ajoute un char dans notre buffer
		numChar++;
	}
	if (n < 0)
	{
		perror("read");
	}

	// printf("Error : no \\n or read error ");
	*buf = '\0';
	free(readChar); // on libere notre malloc
	return numChar;
}

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s repertoire_comptines\n"
					"serveur pour WCP (Wikicomptine Protocol)\n"
					"Exemple: %s comptines\n",
			nom_prog, nom_prog);
}

/** Retourne en cas de succès le descripteur de fichier d'une socket d'écoute
 *  attachée au port port et à toutes les adresses locales. */
int creer_configurer_sock_ecoute(uint16_t port);

/* Écrit dans le fichier de desripteur fd la liste des comptines présents dans
 *  le catalogue c comme spécifié par le protocole WCP, c'est-à-dire sous la
 *  forme de plusieurs lignes terminées par '\n' :
 *  chaque ligne commence par le numéro de la comptine (son indice dans le
 *  catalogue) commençant à 0, écrit en décimal, sur 6 caractères
 *  suivi d'un espace
 *  puis du titre de la comptine
 *  une ligne vide termine le message */
// void envoyer_liste(int fd, struct catalogue *c);

/* Lit dans fd un entier sur 2 octets écrit en network byte order
 *  retourne : cet entier en boutisme machine. */
// uint16_t recevoir_num_comptine(int fd);

/* Écrit dans fd la comptine numéro ic du catalogue c dont le fichier est situé
 *  dans le répertoire dirname comme spécifié par le protocole WCP, c'est-à-dire :
 *  chaque ligne du fichier de comptine est écrite avec son '\n' final, y
 *  compris son titre, deux lignes vides terminent le message */
// void envoyer_comptine(int fd, const char *dirname, struct catalogue *c, uint16_t ic);

/*// Serveur multi-threadé la logique est ecrite dans le thread
void *thread_worker(void *arg) {
	printf("Nous sommes dans le thread.\n");
	struct work_args *args = (struct work_args*)arg;// structure qui contient un descripteur fd et un catalogue

	uint16_t n = 0;
	char query_content[2048];
	while(n != 666){

		int size_query = read_until_nl(args->fd,query_content);
		query_t q = {.content = query_content, .size = size_query};
		serv_interpreter(&q, dbd);
	}
	pthread_exit(EXIT_SUCCESS);
}*/

// Serveur multi-threadé la logique est ecrite dans le thread
void *thread_worker(void *arg)
{
	printf("Thread started.\n");
	work_args *args = (work_args *)arg;

	char * query_content = malloc(sizeof(char) * 2048);
	int size_query;
	uint16_t n = 0;
	while (n != 10)
	{
		size_query = read_until_nl(args->fd, query_content);
		if (size_query <= 0)
		{
			if (size_query == 0)
			{
				printf("Client disconnected.\n");
			}
			else
			{
				perror("Read error");
			}
			break;
		}
		for(int i = 0; i < size_query; i++){
			printf("%c", query_content[i]);
		}
		printf("\n");
		query_t q = {.content = {0}, .size = (uint16_t)size_query};
		memcpy(q.content, query_content, size_query);
		serv_interpreter(&q, args->dbd, args->fd);
		n++;
	}

	close(args->fd); // Close the client connection
	printf("Thread exiting.\n");
	pthread_exit(EXIT_SUCCESS);
}

void write_query_end(query_t *q, char *wr)
{
	for (int i = 0; i < strlen(wr); i++)
	{
		if (q->size >= 2048)
		{
			perror("buffer overflow");
			exit(1);
		}
		q->content[q->size] = wr[i];
		q->size++;
	}
	return;
}

query_t serv_construire_message(serveur_e inst,char * User, char *content,int size)
{

	query_t query;

	switch (inst)
	{
	case CONV:

		write_query_end(&query, "CONV ");
		// char * conv = strtok(content,"\\");
		// char * participant = strtok(NULL,"\\");
		//char *contenueConv = strtok(NULL, "\\");
		// encryption
		// write_query_end(&query,conv);
		// write_query_end(&query,"\\");
		// write_query_end(&query,participant);
		// write_query_end(&query,"\\");
		write_query_end(&query,User);
		write_query_end(&query, " ");
		write_query_end(&query,content);
		write_query_end(&query, "\n");

		break;

		break;
	case LOG_OK:

		write_query_end(&query, "LOG_OK  ");
		write_query_end(&query,User);
		write_query_end(&query, " ");
		write_query_end(&query, content);
		write_query_end(&query, " \\\n");


		break;
	case LOG_FAILED:
		write_query_end(&query, "LOG_FAILED ");
		write_query_end(&query,User);
		write_query_end(&query, " ");
		write_query_end(&query,User);
		write_query_end(&query, " ");
		write_query_end(&query, "\n");

		break;
	case DENIED:
		write_query_end(&query, "DENIED ");
		write_query_end(&query,User);
		write_query_end(&query, " ");
		write_query_end(&query,User);
		write_query_end(&query, " ");
		write_query_end(&query, "\n");

	case OKS:
		write_query_end(&query, "OKS ");
		write_query_end(&query,User);
		write_query_end(&query, " ");
		write_query_end(&query,User);
		write_query_end(&query, " ");
		write_query_end(&query, "\n");

		break;
	case SENDING_TRAFFIC:
		write_query_end(&query, "SENDING_TRAFFIC ");
		write_query_end(&query,User);
		write_query_end(&query, " ");
		write_query_end(&query,User);
		write_query_end(&query, " ");
		write_query_end(&query, "\n");

		break;
	default:
		printf("invalide request");
		write_query_end(&query, "\n");

		break;
	}

	return query;
}

void envoyer_query(int fd, query_t *q)
{
	write(fd, q->content, sizeof(char) * q->size);
}

int serv_interpreter(query_t *q, masterDb_t *master, int socket)
{

	char *TOK = malloc(sizeof(char) * 128);
	char *username = malloc(sizeof(char) * 128);
	char *payload = malloc(sizeof(char) * 2048);
	sscanf(q->content, "%s %s %s", TOK, username, payload);
	printf("%s\n", TOK);
	query_t rep;
	request_e inst = convert_to_request(TOK);

	switch (inst)
	{
		
		int check;

	case LOG:
		printf("@LOG\n");
		printf("username : %s payload : %s\n", username, payload);
		if (!validate_login(username, payload))
		{
			printf("login failed\n");
			rep = serv_construire_message(LOG_FAILED, NULL,0);
			envoyer_query(socket, &rep);
			break;
		}
		printf("login success\n");
		int userIndex = 0;
		check = 0;
		char *content = malloc(sizeof(char) * 592);
		content[0] = '\0';
		for (; userIndex < master->nbUser; userIndex++)
		{
			if (!strcmp(username, master->User[userIndex]->userID))
			{
				printf("user found\n");
				check = 1;
				break;
			}
		}
		if (!check)
		{
			rep = serv_construire_message(LOG_FAILED,username, NULL,0);
			envoyer_query(socket, &rep);
			break;
		}
		for (int i = 0; i < master->User[userIndex]->nbConv; i++)
		{
			content = strcat(content, master->User[userIndex]->conversationID[i]);
			content = strcat(content, ":");
		}
		printf("content : %s\n", content);
		rep = serv_construire_message(LOG_OK,username, content,0);
		envoyer_query(socket, &rep);
		break;

	case SEND:
		printf("@SEND\n");
		UserData_t **tabUser = master->User;
		size_t size = master->nbUser;
		check = 0;

		for (int i = 0; i < size; i++)
		{
			if (strcmp(username, tabUser[i]->userID))
			{
				char *conv = strtok(content, ":");
				for (int j = 0; j < tabUser[i]->nbConv; i++)
				{
					if (!strcmp(conv, tabUser[i]->conversationID[j]))
					{
						check = 1;
						char *message = strtok(NULL, ":");
						char filePath[256];
						snprintf(filePath, sizeof(filePath), "%s%s.txt", PATH_CONV, conv);
						// sem_wait
						int fdconv = open(filePath, O_WRONLY);
						write(fdconv, message, sizeof(message));
						close(fdconv);
						// sem_post
						break;
					}
				}
			}
		}

		if (check)
		{
			rep = serv_construire_message(OKS,username, NULL,0);
		}
		else
		{
			rep = serv_construire_message(DENIED,username, NULL,0);
		}
		envoyer_query(socket, &rep);

		break;
	case UPDATE:
		printf("@UPDATE\n");

		char * sendtext = malloc(sizeof(char) * (sz + 3));//+2 pour guillemet 
		char * size = malloc(4);
		char * rawtext = malloc(sizeof(char) * sz);
		int sz = 0;
		char filePath[256];

		snprintf(filePath, sizeof(filePath), "%s%s.txt", PATH_CONV, content);
		
		int fdconv = open(filePath, O_RDONLY);
		//taille de la discusion
		fseek(fdconv, 0L, SEEK_END);
		sz = ftell(fdconv);
		rewind(fdconv);
		char * rawtext = malloc(sizeof(char) * sz);
		read(fdconv,rawtext,sz);
		
		sprintf(size,"%d",sz);
		snprintf(sendtext,"\"%s\"\n",size,rawtext);


		rep = serv_construire_message(SENDING_TRAFFIC,size,NULL);//envoie taille
		envoyer_query(socket, &rep);
		write(socket,sendtext,sz);//envoie du text de la conv
		close(fdconv);

	case CREATE:
		// create conversation
		// OKS
		//  renvoyer l'id de la conv au client
		// OKR
		break;

	case OKR:
		return RECEIVED_CLIENT;
		break;
	default:
		printf("invalide request");
		break;
	}
}

/************************************************************************************************************************$****************/

int main(int argc, char *argv[])
{
	sem_t sem;
	sem_init(&sem, 0, 1);
	printf("Serveur WCP\n");
	masterDb_t *dbd;
	dbd->User = malloc(sizeof(UserData_t*) * 128);
	for(int i = 0; i < 128; i++){
		dbd->User[i] = malloc(sizeof(UserData_t));
	}
	int kill = 0;

	int sock = creer_configurer_sock_ecoute(PORT_WCP); // creation socket listen

	DIR *d = opendir("./database/users");
	
	if (d == NULL)
	{
		perror("no dir");
		return EXIT_FAILURE;
	}

	// check si le fichier est bien dans le dossier
	struct dirent *entry;
	int index = 0;
	printf("Ouverture du dossier\n");
	// load database
	while ((entry = readdir(d)) != NULL)
	{	
		printf("Ouverture du fichier: %s %hhu\n", entry->d_name, entry->d_type);
		if (entry->d_type == DT_REG)
		{
			char * filePath = malloc(sizeof(char) * 256);
			snprintf(filePath, 256, "%s/%s", PATH_USER, entry->d_name);
			printf("path : %s\n", filePath);
			fflush(stdout);
			int fd = open(filePath, O_RDONLY); // ouverture du fichier avec path
			if (fd == -1)
			{
				perror("open");
				exit(1);
			}
			char *buffer = malloc(sizeof(char) * 128);
			sscanf(entry->d_name, "%[^.].txt", buffer);
			dbd->User[index]->userID = buffer;
			for(int p = 0; p < NB_LIGNES_INFOS; p++){
				char *discard = (char *)malloc(sizeof(char) * 128);
				read_until_nl(fd, discard);
				free(discard);
			}
			
			char **conv = malloc(sizeof(char *) * 64);
			for (int i = 0; i < 64; i++)
			{
				conv[i] = malloc(sizeof(char) * 64);
			}
			int j = 0;
			while (1 < read_until_nl(fd, conv[j]))
			{
				j++;
			}
			dbd->User[index]->nbConv = j;
			dbd->User[index]->conversationID = conv;
			index++;
		}
	}
	dbd->nbUser = index;
	closedir(d);
	print_master(dbd);

	while (1)
	{
		struct sockaddr_in sa_clt;
		socklen_t sl = sizeof(sa_clt);
		printf("en attente de connection\n");

		int sctl = accept(sock, (struct sockaddr *)&sa_clt, &sl); // connection d'un client
		struct work_args wa = {.dbd = dbd, .fd = sctl};
		pthread_t threadw;
		printf("Avant la création du thread.\n");
		// Création du thread
		pthread_create(&threadw, NULL, thread_worker, &wa); // lancement d'un thread pour un client
		printf("Après la création du thread.\n");
		pthread_detach(threadw); // le systeme peut recuperer les ressource quand le thread est fermer
	}
	close(sock);
	return 0;
}

void hash_password(const char *password, char *hashed_password_hex) {
    unsigned char hash[SHA256_DIGEST_LENGTH]; // Buffer for the binary hash
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password, strlen(password));
    SHA256_Final(hash, &sha256);

    // Convert the binary hash to a hexadecimal string
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hashed_password_hex + (i * 2), "%02x", hash[i]);
    }
    hashed_password_hex[SHA256_DIGEST_LENGTH * 2] = 0; // Null-terminate the string
}


int validate_login(char *username, char *password)
{
	char * line = malloc(sizeof(char) * 256);
	char * file_username;
	char * file_password_hash;
	char * saveptr; // Pour strtok_r
	char * hashed_password_hex = malloc(sizeof(char) * (SHA256_DIGEST_LENGTH * 2 + 1));
	
	// Hacher le mot de passe fourni
	hash_password(password, hashed_password_hex);


	FILE *file = fopen("./database/login.txt", "r");
	if (file == NULL)
	{
		return 0;
	}

	while (fgets(line, sizeof(char)*256, file) != NULL)
	{
		printf("validate_login : iter\n");
		// Supprime le saut de ligne à la fin si présent
		line[strcspn(line, "\r\n")] = 0;
		file_username = strtok_r(line, ";", &saveptr);
		file_password_hash = strtok_r(NULL, ";", &saveptr);

		if (strcmp(username, file_username) == 0 && strcmp(hashed_password_hex, file_password_hash) == 0)
		{
			return 1;
		}
	}

	return 0;
}

int creer_configurer_sock_ecoute(uint16_t port)
{
	// init socket comme monter dans le cours :
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror("socket");
		exit(0);
	}
	struct sockaddr_in sa = {.sin_family = AF_INET,
							 .sin_port = htons(port),
							 .sin_addr.s_addr = htonl(INADDR_ANY)};

	socklen_t sl = sizeof(sa);
	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
	if (bind(sock, (struct sockaddr *)&sa, sl) < 0)
	{
		perror("bind");
		exit(3);
	}

	if (listen(sock, 128) < 0)
	{
		perror("listen");
		exit(2);
	}
	return sock;
}

void print_master(masterDb_t *master)
{
	printf("*********DATABASE***********\n");
	for (int i = 0; i < master->nbUser; i++)
	{
		printf("User %d : %s\n", i, master->User[i]->userID);
		for (int j = 0; j < master->User[i]->nbConv; j++)
		{
			printf("Conv %d : %s\n", j, master->User[i]->conversationID[j]);
		}
	}
	printf("\n***************************\n");
}
