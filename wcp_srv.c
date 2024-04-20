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
#include <semaphore.h>

#define PORT_WCP 4321

#define RECEIVED_CLIENT 10

const char PATH_USER ="./database/users";

const char PATH_CONV = "./database/conversations";


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

typedef struct dymTab{
	void * tab;
	uint16_t size;
	size_t type;
} dymTab_t;


typedef struct UserData{
	char * userID;
	char ** conversationID;
	int nbConv;

}UserData_t;

typedef struct query_{
	char content[2048];//taille temporaire
	uint16_t size;
} query_t;


typedef struct masterDb{
	UserData_t ** Users;
	int nbUser;
}masterDb_t;


int read_until_nl(int fd, char *buf)
{
	int numChar = 0;
	char *readChar = malloc(sizeof(char));// malloc obligatoire pour pouvoir utiliser read(read ne peut pas ecrire dans le stack ?)
	int n;
	while((n = read(fd,readChar,sizeof(char))) > 0){
		*(buf + numChar) = *readChar;// on ajoute un char dans notre buffer
		numChar++;
		if(*readChar == '\n'){ // quand on arrive a '0' on retourne
			*(buf + numChar) = '\0';// obligatoire sino buffer overflow
			return numChar;
		}
	}
	if(n < 0){
		perror("read");
	}
	
	//printf("Error : no \\n or read error ");
	*buf = '\0';
	free(readChar);// on libere notre malloc
	return numChar;
}

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
	char query_content[2048];
	while(n != 666){
		
		int size_query = read_until_nl(args->fd,query_content);
		query_t q = {.content = query_content, .size = size_query};
		serv_inteprerte(&q, dbd);
	}
	pthread_exit(EXIT_SUCCESS);
}

query_t serv_construire_message(request_e inst, char * content){

	query_t query;
	
	switch (inst)
	{
	case CONV:
		write_query_end(&query,"CONV\\");
		char * conv = strtok(content,"\\");
		char * participant = strtok(NULL,"\\");
		char * contenueConv = strtok(NULL,"\\");
		//encryption
		write_query_end(&query,conv);
		write_query_end(&query,"\\");
		write_query_end(&query,participant);
		write_query_end(&query,"\\");
		write_query_end(&query,contenueConv);
		write_query_end(&query,"\\");

		break;


		break;
	case LOG_OK:
		write_query_end(&query,"LOG_OK\\");
	
		break;
	case LOG_FAILED:
		write_query_end(&query,"LOG_FAILED\\");
		break;
	case DENIED:
		write_query_end(&query,"DENIED\\");
	
	case OKS:
		write_query_end(&query,"OKS\\");
		break;
	default:
		printf("invalide request");
		break;
	}

	return query;
}

int serv_inteprerte(query_t * q, masterDb_t master){

	char * TOK = strtok(q->content,'\\');

	request_e inst = converte_to_inst(TOK); 
	
	switch (inst)
	{
	case LOG:
		char * username = strtok(NULL,'\\');
		char * password = strtok(NULL,'\\');
		if(validate_login(username,password) == 'F'){
			query_t rep = serv_construire_message(LOG_FAILED,NULL);
			//envoie
		};
		int userIndex = 0;

		char * content = malloc(sizeof(char)*592){
		content[0] = '\0';
			for(;userIndex < master->nbUser; userIndex++ ){
				if(!strcmp(username,master->User[userIndex]->userId)){
					break;
				}
			}
		}
		for(int i = 0; i < master->User[userIndex]->nbConv;i++){
			content = strcat(content, master->User[userIndex]->conversationID[i]);
			content = strcat(content,":");
		}
		break;
		query_t rep = serv_construire_message(LOG_OK,content);
		//envoie

	case SEND:
		char * username = strtok(NULL,'\\');
		UserData_t ** tabUser = master->Users;
		size_t size = master->Users.size;
		int check = 0;
	
		for(int i = 0; i < size; i++){
			if(strcmp(username,tabUser[i]->userID)){
				char * conv = strtok(NULL,'\\');
				for(int j = 0; j < nbConv; i++ ){
					if(!strcmp(conv,tabUser[i]->conversationID[j])){
						check = 1;
						char * message = strtok(NULL,'\\');
						char filePath[256];
						snprintf(filePath, sizeof(filePath), "%s%s.txt",PATH_CONV,conv);
						//sem_wait
						int fdConv = open(filePath,WR_ONLY);
						write(fdConv,message,sizeof(message));
						close(fdconv);
						//sem_post
						break;

					}
				}
			}
		}

		if(check){
			serv_construire_message(OKS,NULL);
		}
		else{
			serv_construire_message(DENIED,NULL);
		}

		break;
	case UPDATE:
		char * convID = strtok(NULL,'\\');
		char filePath[256];
		snprintf(filePath, sizeof(filePath), "%s%s.txt",PATH_CONV,conv);
		char conv[1024];
		int fdConv = open(filePath,RD_ONLY);
		int size_conv = read(fdConv,conv,sizeof(conv));
		close(fdconv);
		serv_construire_message()
		
	case CREATE:
		//create conversation 
		//OKS
		// renvoyer l'id de la conv au client
		//OKR
		break;
		
	case OKR:
		return RECEIVED_CLIENT;
		break;
	default:
		printf("invalide request");
		break;
	}

	return query;
}



int main(int argc, char *argv[])
{	

	sem_t sem;
	sem_init(&sem,0,1);

	masterDb_t * dbd;
	dbd->Users = malloc(sizeof(UserData_t)*128);

	int kill = 0;
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}
	int sock = creer_configurer_sock_ecoute(PORT_WCP);//creation socket listen

	DIR *d = opendir(PATH_USER);

	if(d == NULL){
		perror("no dir");
		
		return EXIT_FAILURE;
	}	
	//check si le fichier est bien dans le dossier
	struct dirent *entry;
	int i = 0;
	while(entry = readdir(d)){
			char filePath[256];
		 	snprintf(filePath, sizeof(filePath), "%s%s.txt",PATH_USER, user_name);
			int fd = open(filePath,O_RDONLY);// ouverture du fichier avec path
			if(fd == NULL){
				perror("open");{
					exit(1);
				}
			}
			char * buffer = (char *)malloc(sizeof(char) *32);
			read_until_nl(fd,buffer);
			dbd->Users[i]->userID = buffer;  
			char * discard = (char *)malloc(sizeof(char) *32);
			read_until_nl(fd,discard); 
			free(discard);
			
			char ** conv = malloc(sizeof(char*) * 64);
			for(int i = 0; i < 64; i++){
				conv[i] = malloc(sizeof(char)*37);
			}
			int j = 0;
			while(1<read_until_nl(fd,conv[j])){
				i++;
			}
			dbd->Users[i]->nbConv = i;

	}


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

char validate_login(const char *username, const char *password)
{
    char valid = 'F';
    char line[256];
    char *file_username;
    char *file_password_hash;
    char *saveptr; // Pour strtok_r
    char hashed_password_hex[SHA256_DIGEST_LENGTH * 2 + 1];

    // Hacher le mot de passe fourni
    hash_password(password, hashed_password_hex);

    FILE *file = fopen("./database/login.txt", "r");
    if (file == NULL)
    {
        return FALSE;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        // Supprime le saut de ligne à la fin si présent
        line[strcspn(line, "\r\n")] = 0;

        file_username = strtok_r(line, ";", &saveptr);
        file_password_hash = strtok_r(NULL, ";", &saveptr);

        if (file_username && file_password_hash &&
            strcmp(username, file_username) == 0 &&
            strcmp(hashed_password_hex, file_password_hash) == 0)
        {
            valid = 'T';
            break;
        }
	}
}

/*int creer_configurer_sock_ecoute(uint16_t port)
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

*/
