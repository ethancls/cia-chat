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

#define PORT_WCP 4321

typedef struct query_
{
	char content[2048]; // taille temporaire
	uint16_t size;
} query_t;

typedef struct utilisateur
{
	uint32_t u_Id; // doit etre le meme entre serveur et le client
	char *u_pseudo;
} user_t;


typedef struct utilisateur{
	uint32_t u_Id;//doit etre le meme entre serveur et le client
	char * u_pseudo;
}user_t;

typedef struct conversation_{
	char * c_Id;//doit etre le meme entre serveur et le client

	user_t c_users[30];
	char *contenue; // contient le texte qui compose la conversation
} convo_t;

typedef struct feed
{
	convo_t f_conv[1024];
	uint32_t f_nbConv;
} feed_t;

typedef enum
{
	LOG,
	SIGNIN, // A faire
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
	DENIED,
	OKS,

} serveur_e;

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s addr_ipv4\n"
					"client pour WCP (Wikicomptine Protocol)\n"
					"Exemple: %s 208.97.177.124\n",
			nom_prog, nom_prog);
}

/** Retourne (en cas de succès) le descripteur de fichier d'une socket
 *  TCP/IPv4 connectée au processus écoutant sur port sur la machine d'adresse
 *  addr_ipv4 */
int creer_connecter_sock(char *addr_ipv4, uint16_t port);

void write_query_end(query_t *q, char *wr);

query_t nouvelle_conversation(uint32_t myId);

query_t login(char *username, char *password);

void interpreter_message(int fd); // bloquant

query_t construire_message(request_e inst, char *content, convo_t *conv, user_t *user);

void envoyer_message(int fd, char *message);

/** Affiche la comptine arrivant dans fd sur le terminal */
void afficher_comptine(int fd);

int read_until_nl(int fd, char *buf)
{
	int numChar = 0;
	char *readChar = malloc(sizeof(char)); // malloc obligatoire pour pouvoir utiliser read(read ne peut pas ecrire dans le stack ?)
	int n;
	while ((n = read(fd, readChar, sizeof(char))) > 0)
	{
		*(buf + numChar) = *readChar; // on ajoute un char dans notre buffer
		numChar++;
		if (*readChar == '\n')
		{							 // quand on arrive a '0' on retourne
			*(buf + numChar) = '\0'; // obligatoire sino buffer overflow
			return numChar;
		}
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

	while (1)
	{
		//travail
	}

	close(sock);
	printf("Program terminated without complications\n");
	return 0;
}

request_e convert_to_request(const char *str)
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
	else if (strcmp(str, "OKS") == 0)
	{
		return OKS;
	}
	else
	{
		return -1; // Indicate an error or handle unknown values accordingly
	}
}

void write_query_end(query_t *q, char *wr)
{
	for (int i = 0; i < strlen(wr)  - 1; i++)
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


int interpreter_message(int fd)
{

	char content[2048];
	int size = read_until_nl(fd,content);
	printf("query received = %s",content);
	const char * TOK = strtok(content,'//');
	request_e r = convert_to_request(TOK);

	switch (r)
	{
	case LOG_OK:
		char * convs = strtok(NULL,'//');
		char * idConv = strtok(convs,':');
		while(idConv != NULL){
			printf("conv : %s \n");
			char * idConv = strtok(convs,':');
		}
		return 1;
		break;
	case LOG_FAILED:
		printf("LOGIN FAILED\n");
		return 0;
		break;

	case CONV:
		while(interpreter_message(fd)!=2)continue;
		return 2;
		break;
	case OKS:
		printf("OKS RECEIVED\n");
		return 2;
	default:
		break;
	}
}



query_t construire_message(request_e inst, char *content, convo_t *conv, user_t *user)
{

	query_t query;
	char *parser;
	char *idconvo;
	char *nom;

	switch (inst)
	{
	case LOG:

		write_query_end(&query,"LOG\\");
		char * name = strtok(content,"\\");
		char * pass = strtok(NULL, "\\");
		//encryption
		write_query_end(&query,name);
		write_query_end(&query,"\\");
		write_query_end(&query,pass);
		write_query_end(&query,"\\");
		write_query_end(&query,"\n");


		break;
	case SEND:
		write_query_end(&query,"SEND\\");
		
		write_query_end(&query,conv->c_Id);
		
		write_query_end(&query,"\\");
		write_query_end(&query,user->u_pseudo);// Il sera preferable d'utiliser l'id au lieu du pseudo, mais sa fera la faire pour le moment
		write_query_end(&query,"\\");
		write_query_end(&query,content);
		write_query_end(&query,"\\");
		write_query_end(&query,"\n");


		break;
	case UPDATE:
		write_query_end(&query,"UPDATE\\");
		write_query_end(&query,conv->c_Id);
		write_query_end(&query,"\\");
		write_query_end(&query,"\n");


		break;
	case CREATE:
		write_query_end(&query, "CREATE\\");

		nom = strtok(content, "\\");
		write_query_end(&query, nom);
		write_query_end(&query, "\\");
		while ((nom = strtok(content, "\\")) != NULL)
		{
			write_query_end(&query, nom);
			write_query_end(&query, "\\");
		}
		break;
	case ADD:
		write_query_end(&query, "ADD\\");
		nom = strtok(content, "\\");
		write_query_end(&query, nom);
		write_query_end(&query, "\\");
		break;
	case OKR:
		write_query_end(&query, "OK\\");
		break;
	default:
		printf("invalide request");
		break;
	}

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
