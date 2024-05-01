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
#define CONTENT_MAX_SIZE 64
#define CONTENT_MAX_NB 32

typedef struct query
{
	char *content; // taille temporaire
	uint16_t size;
} query_t;



typedef struct conversation
{
	char *nom; // doit etre le meme entre serveur et le client
	char *id_deconv; // contient le texte qui compose la conversation
} convo_t;

typedef struct feed
{
	convo_t f_conv[1024];
	uint32_t f_nbConv;
} feed_t;

typedef struct utilisateur
{
	// doit etre le meme entre serveur et le client
	char *u_pseudo;
	char * password;
	convo_t ** conversation;
	int nb_conv;
} user_t;

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

int interpreter_message(int fd,char ** data); // bloquant

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

void envoyerMessage(user_t * u,int sock,char ** content){
	int choix = 0;
	printf("\n choissez une conversation : \n");
	for(int i = 0; i < u->nb_conv;i++){
		printf("conv number : %d ,name : %s\n",i,u->conversation[i]->nom);
	}
	printf("choix : ");
	scanf("%d", &choix);
	if(choix >= u->nb_conv || choix <0){
		printf("\n inavlide conv number : returning to menu\n");
		return;
	}
	printf("conversation : %s : id %s\n",u->conversation[choix]->nom,u->conversation[choix]->id_deconv);
	printf("\nmessage : ");
	char * message = malloc(1024);
	char * sizem = malloc(5);
	char * payload = malloc(1032);
	read_until_nl(STDIN_FILENO,message);
	printf("you typed : %s\n",message);
	sprintf(sizem,"%d",(int)strlen(message));
	snprintf(payload,1032,"%s:%s:",u->conversation[choix]->id_deconv,sizem);
	printf("payload : %s\n",payload);
	free(sizem);
	query_t q;
	q = construire_message(SEND,u->u_pseudo,payload);
	
	envoyer_query(sock,&q);
	write(sock,message,(int)strlen(message));
	
	free(message);
	free(payload);

	int rep = interpreter_message(sock,content);
	if(rep == -1){
		printf("Serveur had a problem : message ignored \n");
		return;
	}
	printf("Serveur : ACK, message saved on serveur : %s\n",content[0]);
	return;

}
 
void voirConversation(user_t * u, int sock,char ** content){
	int choix = 0;
	printf("\n choissez une conversation : \n");
	for(int i = 0; i < u->nb_conv;i++){
		printf("conv number : %d ,name : %s\n",i,u->conversation[i]->nom);
	}
	printf("choix : ");
	scanf("%d", &choix);
	if(choix >= u->nb_conv || choix <0){
		printf("\n inavlide conv number : returning to menu\n");
		return;
	}
	printf("conversation : %s : id %s\n",u->conversation[choix]->nom,u->conversation[choix]->id_deconv);
	query_t q;
	q = construire_message(UPDATE,u->u_pseudo,u->conversation[choix]->id_deconv);
	envoyer_query(sock,&q);
	int rep = interpreter_message(sock,content);
	if(rep == -1){
		printf("failed to get reponse : maybe convo doesnt exist? \n");
		return;
	}
	printf("contenue conv :\n*****************************************************************************\n\n%s\n",content[0]);
	printf("press any key to get back to menu");
	char dis;
	scanf("%c",&dis);
	return;
}

void creerConversation(user_t * u, int sock,char ** content){
	printf("nom nouvelle convo : ");
	char nom[32];
	scanf("%31s",nom);
	printf("participant ( format nomPar1:nomPar2:...:nomParn:)  : ");
	char participant[1026];
	scanf("%1000s",participant);
	char payload[1056];
	snprintf(payload,1056,"%s:%s:",participant,u->u_pseudo);
	query_t q;
	q = construire_message(CREATE,nom,payload);
	envoyer_query(sock,&q);
	int rep = interpreter_message(sock,content);
	if(rep == -1){
		printf("failed to create : %s\n",content[0]);
		return;
	}
	printf("success created conversation : %s", content[0]);

}
 
void miseAJourServeur(user_t * user, int sock,char ** content){
	query_t q;
	q = construire_message(LOG,user->u_pseudo,user->password); 
	envoyer_query(sock,&q);
	int reponse = interpreter_message(sock,content);
	
	if(reponse == -1){
		printf("failed to update : mauvais nom utilisateur");
		return;
	}
	int incr = 0;
	for(int i = 0; i < CONTENT_MAX_NB; i = i + 2){
		if(content[i] == NULL){
			user->nb_conv = incr;
			break;
		}
		printf("data %d: %s \n",i,content[i]);
		strcpy(user->conversation[incr]->id_deconv, content[i]);
		strcpy(user->conversation[incr]->nom, content[i + 1]);
		incr++;
	}
	printf("convo loaded\n");
	printf("\n***********************STATE UPDATED***********************************\n");
	return;
}

void menu(user_t * u,int sock,char ** content) {
    int choix;

    do {
		
        printf("\nMenu :\n");
        printf("1. Envoyer un message\n");
        printf("2. Voir la conversation\n");
        printf("3. Créer une conversation\n");
        printf("4. Mettre à jour le serveur\n");
        printf("0. Quitter\n");
        printf("Choisissez une option : ");
        scanf("%d", &choix);

        switch (choix) {
            case 1:
                envoyerMessage(u,sock,content);
                break;
            case 2:
                voirConversation(u,sock,content);
                break;
            case 3:
                creerConversation(u,sock,content);
                break;
            case 4:
                miseAJourServeur(u,sock,content);
                break;
            case 0:
                printf("Au revoir !\n");
                break;
            default:
                printf("Option invalide. Veuillez réessayer.\n");
        }
    } while (choix != 0);
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
	int reponse = 0;
	user_t * user = malloc(sizeof(user_t));
	user->conversation = malloc(sizeof(convo_t*) *CONTENT_MAX_NB);
	for(int i =0; i < CONTENT_MAX_NB; i++){
		user->conversation[i] = malloc(sizeof(convo_t));
		user->conversation[i]->id_deconv = malloc(CONTENT_MAX_SIZE);
		user->conversation[i]->nom = malloc(32);
	}
	query_t * q = malloc(sizeof(query_t));
	char ** dataBuffer = malloc(sizeof(char*) * CONTENT_MAX_NB * 2);
	//for(int i =0; i < CONTENT_MAX_NB; i++){
	//	dataBuffer[i] = malloc(CONTENT_MAX_SIZE);
	//}
	printf("Connected to server IP: %s\n", ip_str);
	printf("Se connecter : 1\nCreer un compte : 2\n");
	int sc = 0;
	scanf("%d",&sc);
	char * nom = malloc(32);
	char * mdp = malloc(32);
	if(sc == 2){
		printf("********************NOUVELLE UTILISATEUR****************************\n");
		printf("taper nom utilisateur et mdp :\n");
		printf("nom : ");
	
		scanf("%s",nom); 
		printf("\n");
		printf("mdp : ");
		scanf("%s",mdp);
		*q = construire_message(SIGNIN,nom,mdp);
		envoyer_query(sock,q);
		reponse = interpreter_message(sock,dataBuffer);
		if(reponse == -1){
			printf("n'a pas pu creer l'utilisateur\n");
		}
	}
	printf("taper nom utilisateur et mdp :\n");
	printf("nom : ");
	
	scanf("%s",nom); 
	printf("\n");
	printf("mdp : ");
	scanf("%s",mdp);
	printf("\n");
	user->u_pseudo = nom;
	user->password = mdp;
	printf("user : %s , mdp : %s\n",user->u_pseudo,user->password);

	*q = construire_message(LOG,user->u_pseudo,user->password); 
	envoyer_query(sock,q);
	reponse = interpreter_message(sock,dataBuffer);
	if(reponse == -1){
		printf("mauvais nom utilisateur\n");
		close(sock);
		exit(0);
	}
	printf("*************************LOADING DATA***************************************\n");
	int incr = 0;
	for(int i = 0; i < CONTENT_MAX_NB; i = i + 2){
		if(dataBuffer[i] == NULL){
			user->nb_conv = incr;
			break;
		}
		printf("data %d: %s \n",i,dataBuffer[i]);
		//user->conversation[i]->id_deconv = malloc(strlen(dataBuffer[i]) + 1); // Allocation pour id_deconv
        strcpy(user->conversation[incr]->id_deconv, dataBuffer[i]);
		printf("data loades in conversation %d: %s \n",i,user->conversation[incr]->id_deconv);
		strcpy(user->conversation[incr]->nom, dataBuffer[i + 1]);
		incr++;
	}


	printf("convo loaded");

	menu(user,sock,dataBuffer);
	
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

int interpreter_message(int fd,char ** dataRet)
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
		
		char *idConv = strtok(payload, ":");
		char * nomConv = strtok(NULL, ":") ;
		printf("1ER conv = %s\n",idConv);
		printf("1ER nom = %s\n",nomConv);
		int indexConv = 0;
		dataRet[indexConv] = idConv;
		indexConv++;
		dataRet[indexConv] = nomConv;
		while ((idConv = strtok(NULL, ":")) != NULL)
		{
			indexConv++;
			printf("idConv = %s\n", idConv);
			dataRet[indexConv] = idConv;
			indexConv++;
			nomConv = strtok(NULL, ":");
			printf("nom = %s\n",nomConv);
			dataRet[indexConv] = nomConv;
			
		}
		indexConv++;
		dataRet[indexConv] = NULL;
		
		return 0;
		break;
	case LOG_FAILED:
		printf("@LOGIN_FAILED\n");
		dataRet[0] = "couldnt log in";
		return -1;
		break;
	case SENDING_TRAFFIC:
		printf("@SENDING_TRAFFIC\n");
		int size_buffer = atoi(info);
		printf("size_buffer = %d\n", size_buffer);
		char *data = malloc(size_buffer);
		read(fd, data, size_buffer);
		printf("%s\n", data);
		dataRet[0] = data;

		break;
	case CONV: // DEPRECATED
		printf("@CONV\n");
		return 2;
		break;
	case DENIED:
		printf("@DENIED_RECEIVED\n");
		dataRet[0] = payload;
		return -1;
	case OK:
		printf("@OK_RECEIVED\n");
		dataRet[0] = payload;
		return 0;
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
		break;
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
