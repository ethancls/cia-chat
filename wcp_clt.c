/* Timothée M'BASSIDJE 12104516
Je déclare qu'il s'agit de mon propre travail. */
/* fichiers de la bibliothèque standard */
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
#include <arpa/inet.h> /* inet_pton */

#define PORT_WCP 4321 // DEBIND PORT MACOS : sudo lsof -P -i :PORT and kill -9 <PID>

typedef struct query
{
	char *content; // taille temporaire
	uint16_t size;
} query_t;

typedef struct utilisateur
{
	uint32_t u_Id; // doit etre le meme entre serveur et le client
	char *u_pseudo;
} user_t;

typedef struct conversation
{
	char *c_Id; // doit etre le meme entre serveur et le client
	user_t c_users[30];
	char *contenue; // contient le texte qui compose la conversation
} convo_t;

typedef struct feed
{
	convo_t f_conv[1024];
	uint32_t f_nbConv;
} feed_t;

typedef enum tokens
{
	LOG,
	SIGNIN, // A faire
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

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s addr_ipv4\n"
					"client pour TCP\n"
					"Exemple: %s 208.97.177.124\n",
			nom_prog, nom_prog);
}

/** Retourne (en cas de succès) le descripteur de fichier d'une socket
 *  TCP/IPv4 connectée au processus écoutant sur port sur la machine d'adresse
 *  addr_ipv4 */
int creer_connecter_sock(char *addr_ipv4, uint16_t port);

void write_query_end(query_t *q, char *wr);

void envoyer_query(int fd, query_t *q);

query_t nouvelle_conversation(uint32_t myId);

query_t login(char *username, char *password);

int interpreter_message(int fd); // bloquant

query_t construire_message(tokens_t inst, char *username, char *content);

void envoyer_message(int fd, char *message);

int read_until_nl(int fd, char *buf)
{
	int numChar = 0;
	char *readChar = malloc(sizeof(char)); // malloc obligatoire pour pouvoir utiliser read(read ne peut pas ecrire dans le stack ?)
	int n;
	int dansguillemet = 0;
	while ((n = read(fd, readChar, sizeof(char))) > 0)
	{
		if (*readChar == '\"')
		{
			dansguillemet = !dansguillemet; // ignorer les \n si dans les guillemets
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

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		usage(argv[0]);
		return 1;
	}

	uint16_t snd = 0;

	// Create and connect socket
	int sock = creer_connecter_sock(argv[1], PORT_WCP);
	if (sock == -1)
	{
		perror("Failed to connect to server");
		return 1;
	}

	// Get the IP address of the connected server
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	if (getpeername(sock, (struct sockaddr *)&addr, &addrlen) == -1)
	{
		perror("getpeername failed");
		close(sock);
		return 1;
	}

	// Convert binary IP address to human-readable format
	char ip_str[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN) == NULL)
	{
		perror("inet_ntop failed");
		close(sock);
		return 1;
	}

	printf("Connected to server IP: %s\n", ip_str);

	query_t q;
	q = construire_message(SIGNIN, "admin", "123");
	envoyer_query(sock, &q);
	interpreter_message(sock);
	q = construire_message(CREATE, "allseing", "admin:dave:ethan:jesse:maverick:neji:");
	envoyer_query(sock, &q);
/*	char *message = "Hello World !\n";
	char *convers = "bkJB4iMLlufosedz3buGeeSxPW7Ma1xCFN9t";
	char *size_message = malloc(sizeof(char) * 4);
	char *m = malloc(sizeof(char) * 128);
	sprintf(size_message, "%d", (int)strlen(message));
	snprintf(m, 132, "%s:%s:", convers, size_message);
	q = construire_message(SEND, "ethan", m);
	envoyer_query(sock, &q);
	write(sock, message, strlen(message));
	interpreter_message(sock);
	free(size_message);
	free(m);*/
	q = construire_message(LOG, "admin", "123");
	envoyer_query(sock, &q);
	interpreter_message(sock);

	close(sock);
	printf("Program terminated without complications\n");
	return 0;
}

tokens_t convert_to_request(const char *str)
{
	if (strcmp(str, "CONV") == 0)
	{
		return CONV;
	}
	else if (strcmp(str, "USERID") == 0)
	{
		return USERID;
	}
	else if (strcmp(str, "LOG_OK") == 0)
	{
		return LOG_OK;
	}
	else if (strcmp(str, "LOG_FAILED") == 0)
	{
		return LOG_FAILED;
	}
	else if (strcmp(str, "DENIED") == 0)
	{
		return DENIED;
	}
	else if (strcmp(str, "OK") == 0)
	{
		return OK;
	}
	else if (strcmp(str, "SENDING_TRAFFIC") == 0)
	{
		return SENDING_TRAFFIC;
	}
	else
	{
		return -1;
	}
}

void write_query_end(query_t *q, char *wr)
{
	for (int i = 0; i < strlen(wr); i++)
	{
		if (q->size >= 1024)
		{
			perror("buffer overflow");
			exit(1);
		}
		q->content[q->size] = wr[i];
		q->size++;
	}
	return;
}

int creer_connecter_sock(char *addr_ipv4, uint16_t port)
{
	// init socket comme monter dans le cours :
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror("socket");
		exit(0);
	}
	struct sockaddr_in sa = {.sin_family = AF_INET,
							 .sin_port = htons(port)};

	if (inet_pton(AF_INET, addr_ipv4, &sa.sin_addr) != 1)
	{
		perror("socket");
	}
	socklen_t sl = sizeof(sa);
	if (connect(sock, (struct sockaddr *)&sa, sl) < 0)
	{
		perror("connect");
		exit(3);
	}
	return sock;
}

void envoyer_message(int fd, char *message)
{
	if (write(fd, message, strlen(message) + 1) < 0) // +1 pour le \0 ??
	{
		perror("write");
		close(fd);
		exit(1);
	}
}

void envoyer_query(int fd, query_t *q)
{
	write(fd, q->content, sizeof(char) * q->size);
}

int interpreter_message(int fd)
{
	char *content = malloc(sizeof(char) * 2048);
	int size = read_until_nl(fd, content);
	printf("query received = %s", content);
	char *TOK = malloc(sizeof(char) * 16);
	char *info = malloc(sizeof(char) * 48);
	char *payload = malloc(sizeof(char) * 2048);
	sscanf(content, "%s %s %s", TOK, info, payload);
	tokens_t r = convert_to_request(TOK);
	printf("token = %d\n", r);

	switch (r)
	{
	case LOG_OK:
		printf("@LOGIN_OK\n");
		char *convs = strtok(payload, ";");
		char *idConv = strtok(NULL, ":");
		while ((convs = strtok(NULL, ";")) != NULL)
		{
			printf("convs = %s\n", convs);
			idConv = strtok(NULL, ":");
			printf("idConv = %s\n", idConv);
		}
		return 1;
		break;
	case LOG_FAILED:
		printf("@LOGIN_FAILED\n");
		return 0;
		break;
	case SENDING_TRAFFIC:
		printf("@SENDING_TRAFFIC\n");
		int size_buffer = atoi(info);
		printf("size_buffer = %d\n", size_buffer);
		char *data = malloc(size_buffer);
		read(fd, data, size_buffer);
		printf("%s\n", data);
		break;
	case CONV:
		printf("@CONV\n");
		while (interpreter_message(fd) != 2)
			continue;
		return 2;
		break;
	case OK:
		printf("@OK_RECEIVED\n");
		return 2;
	default:
		return -1;
		break;
	}
	return 0;
}

query_t construire_message(tokens_t inst, char *username, char *content)
{

	query_t query;
	query.size = 0;
	query.content = malloc(sizeof(char) * 2048);
	char *user_id;
	char *payload;

	// TODO : REFAIRE LE PROTOCOLE COTER CLIENT

	switch (inst)
	{
	case LOG: // LOG <username> <password>

		write_query_end(&query, "LOG ");
		// char *name = strtok(content, "\\");
		// char *pass = strtok(NULL, "\\");
		//  encryption

		break;
	case SEND: // SEND <username> <numCOnv:size>
		write_query_end(&query, "SEND ");

	case SIGNIN: // SIGNIN <username> <password>
		write_query_end(&query, "SIGNIN ");
		break;
	case UPDATE: // UPDATE <username> <numCOnv>
		write_query_end(&query, "UPDATE ");

		break;
	case CREATE: // CREATE <NomCOnv> <nomPartipant:...:nom>
		write_query_end(&query, "CREATE ");
		break;
	case ADD: // ADD <USerAjouter> <IdCOnv>
		write_query_end(&query, "ADD ");

		break;
	case OK: // OK <info>
		write_query_end(&query, "OK ");
		break;
	default:
		printf("invalid request");
		query.size = 0;
		return query;
		break;
	}

	write_query_end(&query, username);
	write_query_end(&query, " ");
	write_query_end(&query, content);
	write_query_end(&query, "\n");

	return query;
}

/*uint16_t recevoir_liste_comptines(int fd){

	u_int16_t ret = 0;
	char *buf = malloc(sizeof(char)*50);
	int tst;
		do
		{

			tst = read_until_nl(fd,buf);

			if(tst < 0){
				perror("failed reading liste of comptine");
				close(fd);
				exit(1);
			}
			printf("%s",buf);
			ret++;

		} while (tst > 2);// une ligne vide est considerer comme un ligne de moins de 3 char : LES LIGNES A UN CHAR NE SONT DONC PAS SUPPORTER
	free(buf);
	printf("fin liste comptine\n");
	return ret - 1;
}

uint16_t saisir_num_comptine(uint16_t nb_comptines)
{
	u_int16_t ret = 0;
	do{

		printf("entrer un valeur entre 0 et %d\n", nb_comptines);
		int n = scanf("%hd", &ret);//scanf peut retourner des valeurs etranges il est peut etre plus judicieux d'utiliser une autre fonction
		if(n < 0){
			perror("scanf");
			exit(1);
		}
	}while (ret >= nb_comptines && ret != 666);// 666 signale d'arret

	return ret;
}

void envoyer_num_comptine(int fd, uint16_t nc)
{
	u_int16_t rnc = htons(nc);// conversion en NBO
	if(write(fd,&rnc,sizeof(u_int16_t)) < 0){
		perror("write");
		close(fd);
		exit(1);
	}

}

void afficher_comptine(int fd)
{
	char *buf = malloc(sizeof(char)*50);// malloc obligatoire a cause de read_until_nl
	if(buf == NULL){
		perror("malloc");
		exit(1);
	}
		do
		{
			int tst = read_until_nl(fd,buf);

			if(tst < 0){
				perror("failed reading liste of comptine");
				exit(1);
			}

			if(tst <= 2){// on teste deux fois meme logique que pour recevoir_liste_comptine
				printf("%s",buf);
				int tst = read_until_nl(fd,buf);
				if(tst < 0){
					perror("failed reading liste of comptine");
					exit(1);
				}

				if(tst <= 2){// deuxieme test
					//printf("%s\n",buf);
					break;
				}

			}

			printf("%s",buf);


		} while (1);

	free(buf);
	//printf("la meme\n");
}*/
