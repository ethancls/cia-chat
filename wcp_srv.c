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
#include <errno.h>
/* spécifique à internet */
#include <arpa/inet.h> /* inet_pton */
/* spécifique aux comptines */

#include <pthread.h>

#define PORT_WCP 4321

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s repertoire_comptines\n"
			"serveur pour WCP (Wikicomptine Protocol)\n"
			"Exemple: %s comptines\n", nom_prog, nom_prog);
}
/** Retourne en cas de succès le descripteur de fichier d'une socket d'écoute
 *  attachée au port port et à toutes les adresses locales. */
int creer_configurer_sock_ecoute(uint16_t port);

/** Écrit dans le fichier de desripteur fd la liste des comptines présents dans
 *  le catalogue c comme spécifié par le protocole WCP, c'est-à-dire sous la
 *  forme de plusieurs lignes terminées par '\n' :
 *  chaque ligne commence par le numéro de la comptine (son indice dans le
 *  catalogue) commençant à 0, écrit en décimal, sur 6 caractères
 *  suivi d'un espace
 *  puis du titre de la comptine
 *  une ligne vide termine le message */
void envoyer_liste(int fd, struct catalogue *c);

/** Lit dans fd un entier sur 2 octets écrit en network byte order
 *  retourne : cet entier en boutisme machine. */
uint16_t recevoir_num_comptine(int fd);

/** Écrit dans fd la comptine numéro ic du catalogue c dont le fichier est situé
 *  dans le répertoire dirname comme spécifié par le protocole WCP, c'est-à-dire :
 *  chaque ligne du fichier de comptine est écrite avec son '\n' final, y
 *  compris son titre, deux lignes vides terminent le message */
void envoyer_comptine(int fd, const char *dirname, struct catalogue *c, uint16_t ic);

struct work_args{//structure utiliser pour les threads
	int fd;
	struct catalogue *cat;
};
// Serveur multi-threadé la logique est ecrite dans le thread
void *thread_worker(void *arg) {
	printf("Nous sommes dans le thread.\n");
	struct work_args *args = (struct work_args*)arg;// structure qui contient un descripteur fd et un catalogue
	
	
	
	uint16_t n = 0;
	while(n != 666){
		envoyer_liste(args->fd,args->cat);// envoyer au client des la connection
		n = recevoir_num_comptine(args->fd);// attente de la reponse
		if(n == 666){
			break;
		}
		envoyer_comptine(args->fd,"comptines",args->cat,n);//on passe comptine ici mais on devrait ajouter un buf dans work_args qui prend argv(1): non implémenter par manque de temps
	}
	close(args->fd);
	printf("fin threadworker\n");
	pthread_exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[])
{
	int kill = 0;
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}
	int sock = creer_configurer_sock_ecoute(PORT_WCP);//creation socket listen
	struct catalogue *cat = creer_catalogue(argv[1]);
	while(1){
		struct sockaddr_in sa_clt;
		socklen_t sl = sizeof(sa_clt);
		printf("en attente de connection\n");

		int sctl = accept(sock, (struct sockaddr *) &sa_clt, &sl);// connection d'un client
		struct work_args wa = {.cat = cat, .fd = sctl};
		pthread_t threadw;
		printf("Avant la création du thread.\n");
		// Création du thread
		pthread_create(&threadw, NULL, thread_worker,&wa);// lancement d'un thread pour un client
		printf("Après la création du thread.\n");
		pthread_detach(threadw);//le systeme peut recuperer les ressource quand le thread est fermer 
	}
	close(sock);
	liberer_catalogue(cat);


	return 0;
}

int creer_configurer_sock_ecoute(uint16_t port)
{
	//init socket comme monter dans le cours :
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock < 0 ){
		perror("socket");
		exit(0);
	}
	struct sockaddr_in sa ={ .sin_family = AF_INET,
		.sin_port = htons(port), .sin_addr.s_addr = htonl(INADDR_ANY)};


	socklen_t sl = sizeof(sa);
	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
	if (bind(sock, (struct sockaddr *) &sa, sl) < 0) {
		perror("bind");
		exit(3);
	}

	if (listen(sock, 128) < 0) { perror("listen"); exit(2); }
	return sock;

	
}

void envoyer_liste(int fd, struct catalogue *c)
{	
	char endnl = '\n';// ajouter a la fin de la transmision
	u_int16_t index;
	for(; index < c->nb; index++){
		printf("%d\n", index);
		int n = dprintf(fd,"%d %s",index,c->tab[index]->titre);// dprint utiliser pour mettre un int avant d'une chaine
		if(n < 0){
			perror("dprintd");
			close(fd);
			exit(1);
		}
	}
	//ajout ligne vide
	 int n = dprintf(fd,"%c",endnl);
		if(n < 0){
			perror("dprintd");
			close(fd);
			exit(1);
		}
	return;
}

uint16_t recevoir_num_comptine(int fd)
{
	u_int16_t nc;
	
	if(read(fd,&nc,sizeof(u_int16_t)) < 0){
		perror("write");
		close(fd);
		exit(1);
	}
	printf("recu le uint_16 :\n");
	nc = ntohs(nc);// conversion network to machine/host
	printf("conversion ntohs :\n");
	return nc;
}

void envoyer_comptine(int fd, const char *dir_name, struct catalogue *c, uint16_t ic)
{	
	printf("envoie comptines %d :\n",ic);
	struct comptine *cp = init_cpt_depuis_fichier(dir_name,c->tab[ic]->nom_fichier);
	printf("comptine instancier :\n");
	//On doit creer le chemin pour ouvrir le fichier
	if(cp == NULL){
		perror("init_cpt");
		close(fd);
		exit(1);
	}
	char *path = malloc(sizeof(char) * 512);
	int cur= 0;
	while(*(dir_name + cur) != '\0'){
		*(path + cur) = *(dir_name + cur);
		cur++;
	}

	*(path + cur) = '/';
	cur++;
	int temp = 0;
	
	while(*(cp->nom_fichier + temp) != '\0'){
		*(path + cur) = *(cp->nom_fichier + temp);
		cur++;
		temp++;
	}
	*(path + cur) ='\0';//a ne pas oublier
	//fin construction chemin
	printf("%s\n",path);
	int n = open(path,O_RDONLY);
	if(n < 0){
		perror("open");
		close(fd);
		exit(1);
	}
	printf("fichier ouvert avec succes\n");
	// boucle qui envois la comptine au client 
	//Les comptine sont ecrite ligne par ligne dans la socket
	//et la fonction s'arrete apres deux ligne vide
	char *buf = malloc(sizeof(char)*50);
	
			do
			{	
			
				int tst = read_until_nl(n,buf);
				printf("ligne lue\n");
				printf("%s",buf);
				if(tst < 0){
					perror("failed reading liste of comptine\n");
					exit(1);
				}

				if(tst <= 2){
					printf("on est la\n");
					if(write(fd,buf,strlen(buf)) < 0){
						perror("write");
						exit(1);
					}
					int tst = read_until_nl(n,buf);
					printf("2eme\n");
					printf("ligne lue\n");
					printf("%s",buf);
					if(tst < 0){
						perror("failed reading liste of comptine");
						exit(1);
				
					}
					if(tst <= 2){
						strcpy(buf,"\n\n");
						write(fd,buf,sizeof(char)*2);//probleme de detection de de fin chez le client ajout de deux ligne "vide"
						break;
					}
				}	
					

				
				if(write(fd,buf,strlen(buf)) < 0){
					perror("write");
					exit(1);
				}
				

			} while (1);
			
	free(buf);
	printf("sortie avec succes\n");
}
