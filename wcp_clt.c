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

typedef struct query_{
	char content[2048];//taille temporaire
	uint16_t size;
} query_t;


typedef struct utilisateur{
	uint32_t u_Id;//doit etre le meme entre serveur et le client
	char * u_pseudo;
}user_t;

typedef struct conversation_{
	uint32_t c_Id;//doit etre le meme entre serveur et le client
	user_t c_users[30];
	char * contenue;// contient le texte qui compose la conversation
}convo_t;

typedef struct feed{
	convo_t f_conv[1024];
	uint32_t f_nbConv;
}feed_t;

typedef enum{
	LOG,
	SEND,
	UPDATE,
	CREATE,
	ADD,
	OKR,

}request_e;

typedef enum{
	CONV,
	USERID,
	LOG_OK,
	LOG_FAILED,
	DENIED,
	OKS,

}serveur_e;

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s addr_ipv4\n"
			"client pour WCP (Wikicomptine Protocol)\n"
			"Exemple: %s 208.97.177.124\n", nom_prog, nom_prog);
}

/** Retourne (en cas de succès) le descripteur de fichier d'une socket
 *  TCP/IPv4 connectée au processus écoutant sur port sur la machine d'adresse
 *  addr_ipv4 */
int creer_connecter_sock(char *addr_ipv4, uint16_t port);

void write_query_end(query_t * q,char * wr);

query_t nouvelle_conversation(uint32_t myId);

query_t login(char * username,char * password);

void interpreter_message(int fd);//bloquant

query_t construire_message(request_e inst, char * content,convo_t * conv, user_t * user);

void envoyer_message(int fd, char * message);

/** Affiche la comptine arrivant dans fd sur le terminal */
void afficher_comptine(int fd);

int main(int argc, char *argv[])
{
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}
	uint16_t snd = 0;
	printf("connection au serveur..\n\n");
	int sock = creer_connecter_sock(argv[1],PORT_WCP);// socket avec ip passer en parametre du terminal
	
	
	while (snd >= 0)// si snd est negatif sortir
	{
		
	}
	
	close(sock);
	printf("le programme se ferme sans compliquation\n");
	return 0;
}

int creer_connecter_sock(char *addr_ipv4, uint16_t port)
{
	//init socket comme monter dans le cours :
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock < 0 ){
		perror("socket");
		exit(0);
	}
	struct sockaddr_in sa ={ .sin_family = AF_INET,
		.sin_port = htons(port)};

	if(inet_pton(AF_INET,addr_ipv4,&sa.sin_addr)!= 1){
		perror("socket");
	}
	socklen_t sl = sizeof(sa);
	if(connect(sock, (struct sockaddr *) &sa,sl )< 0){
		perror("connect");
		exit(3);
	}
	return sock;  
}


query_t construire_message(request_e inst, char * content,convo_t * conv, user_t * user){

	query_t query;
	
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

		break;
	case SEND:
		write_query_end(&query,"SEND\\");
		char * idconvo = &(conv->c_Id);//tres experimental sa vas peut etre changer//A FAIRE dprintf existe :(
		char * parser = malloc(5);
		for(int i = 0; i < 4; i++){
			parser[i] = idconvo[i];
		}
		parser[4] = '\0';
		write_query_end(&query,parser);
		free(parser);
		write_query_end(&query,"\\");
		write_query_end(&query,user->u_pseudo);// Il sera preferable d'utiliser l'id au lieu du pseudo, mais sa fera la faire pour le moment
		write_query_end(&query,"\\");
		write_query_end(&query,content);
		write_query_end(&query,"\\");

		break;
	case UPDATE:
		write_query_end(&query,"UPDATE\\");
		char * idconvo = &(conv->c_Id);//tres experimental sa vas peut etre changer//A FAIRE dprintf existe :(
		char * parser = malloc(5);
		for(int i = 0; i < 4; i++){
			parser[i] = idconvo[i];
		}
		parser[4] = '\0';
		write_query_end(&query,parser);
		free(parser);
		write_query_end(&query,"\\");
		break;
	case CREATE:
		write_query_end(&query,"CREATE\\");
		
		char * nom = strtok(content,"\\");
		write_query_end(&query,nom);
		write_query_end(&query,"\\");
		while((nom = strtok(content,"\\")) != NULL){
			write_query_end(&query,nom);
			write_query_end(&query,"\\");
		}
		break;
	case ADD:
		write_query_end(&query,"ADD\\");
		char * nom = strtok(content,"\\");
		write_query_end(&query,nom);
		write_query_end(&query,"\\");
		break;
	case OKR:
		write_query_end(&query,"OK\\");
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
