#include "./tcp_srv.h"

int main(int argc, char *argv[])
{
	sem_t sem;
	sem_init(&sem, 0, 1);
	printf("Serveur TCP\n");
	masterDb_t *dbd = malloc(sizeof(masterDb_t));
	dbd->User = malloc(sizeof(UserData_t *) * 128);
	for (int i = 0; i < 128; i++)
	{
		dbd->User[i] = malloc(sizeof(UserData_t));
	}
	int kill = 0;
	printf("socket\n");
	int sock = creer_configurer_sock_ecoute(PORT_WCP); // creation socket listen

	reload_database(dbd);
	print_master(dbd);
	while (1)
	{
		struct sockaddr_in *sa_clt = malloc(sizeof(struct sockaddr_in));
		socklen_t sl = sizeof(struct sockaddr_in);
		printf("en attente de connection\n");
		int sctl = accept(sock, (struct sockaddr *)sa_clt, &sl); // connection d'un client
		struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
		socklen_t addrlen = sizeof(struct sockaddr_in);
		getpeername(sctl, (struct sockaddr *)addr, &addrlen);
		char *ip_str = malloc(INET_ADDRSTRLEN);
		inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN);
		printf("Connection from %s\n", ip_str);
		work_args *wa = malloc(sizeof(work_args));
		wa->dbd = dbd;
		wa->fd = sctl;
		pthread_t threadw;
		// Création du thread
		pthread_create(&threadw, NULL, thread_worker, wa); // lancement d'un thread pour un client
		pthread_detach(threadw);						   // le systeme peut recuperer les ressource quand le thread est fermer
		free(sa_clt);
		free(ip_str);
	}
	close(sock);
	return 0;
}

tokens_t convert_to_request(const char *str)
{
	if (strcmp(str, "LOG") == 0)
	{
		return LOG;
	}
	else if (strcmp(str, "SIGNIN") == 0)
	{
		return SIGNIN;
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
	else if (strcmp(str, "OK") == 0)
	{
		return OK;
	}
	else if (strcmp(str, "CONV") == 0)
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
	else if (strcmp(str, "SENDING_TRAFFIC") == 0)
	{
		return SENDING_TRAFFIC;
	}
	else
	{
		return -1; // Indicate an error or handle unknown values accordingly
	}
}

int is_contact_valid(char *contact_name)
{
	char *line = malloc(sizeof(char) * 256);
	char *file_username;
	char *saveptr;

	FILE *file = fopen("./database/login.txt", "r");
	if (file == NULL)
	{
		return -1;
	}
	while (fgets(line, sizeof(line), file) != NULL)
	{
		line[strcspn(line, "\r\n")] = 0;

		file_username = strtok_r(line, ";", &saveptr);

		if (file_username != NULL && strcmp(contact_name, file_username) == 0)
		{
			return 0;
			break;
		}
	}
	fclose(file);
	return -1;
}

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
			printf("dans guillemet\n");
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
	*(buf + numChar) = '\0';
	free(readChar); // on libere notre malloc
	return numChar;
}

void usage(char *nom_prog)
{
	fprintf(stderr, "Usage: %s \n",nom_prog);
}

// Serveur multi-threadé la logique est ecrite dans le thread
void *thread_worker(void *arg)
{
	printf("Thread started.\n");
	work_args *args = (work_args *)arg;

	query_t q;
	uint16_t n = 0;
	while (n != 666)
	{
		reload_database(args->dbd);
		int size_query;
		char *query_content = malloc(sizeof(char) * 2048);
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
		// printf("string is %s\n", query_content);
		// printf("\n");
		q.content = query_content;
		q.size = size_query;
		printf("\nQuery received: %s\n", q.content);
		serv_interpreter(&q, args->dbd, args->fd);
		free(query_content);
		n++;
	}
	close(args->fd); // Close the client connection
	printf("Thread exiting.\n");
	pthread_exit(EXIT_SUCCESS);
}
void to_hex_string(unsigned char *hash, char *output, size_t length)
{
	for (size_t i = 0; i < length; i++)
	{
		sprintf(output + (i * 2), "%02x", hash[i]);
	}
	output[length * 2] = '\0';
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

query_t serv_construire_message(tokens_t token, char *info, char *content)
{

	query_t query;
	query.content = malloc(sizeof(char) * 2048);
	query.size = 0;
	switch (token)
	{
	case CONV: // CONV <IdConv> <NomParticipants>

		write_query_end(&query, "CONV ");
		write_query_end(&query, info);
		write_query_end(&query, " ");
		write_query_end(&query, content);
		write_query_end(&query, "\n");
		break;

	case LOG_OK: // LOG_OK <NomUtilisateur> <ListeConversations>

		write_query_end(&query, "LOG_OK ");
		write_query_end(&query, info);
		write_query_end(&query, " ");
		write_query_end(&query, content);
		write_query_end(&query, " ");
		write_query_end(&query, "\n");
		break;

	case LOG_FAILED: // LOG_FAILED <NomUtilisateur>

		write_query_end(&query, "LOG_FAILED ");
		write_query_end(&query, info); // pq envoyer quand log failed ??? juste envoyer token
		write_query_end(&query, " ");
		write_query_end(&query, content);
		write_query_end(&query, " ");
		write_query_end(&query, "\n");
		break;

	case DENIED: // DENIED <Raison> <CodeErreur>

		write_query_end(&query, "DENIED ");
		write_query_end(&query, info);
		write_query_end(&query, " ");
		write_query_end(&query, content);
		write_query_end(&query, " ");
		write_query_end(&query, "\n");
		break;

	case OK: // OK <Info>

		write_query_end(&query, "OK ");
		write_query_end(&query, info);
		write_query_end(&query, " ");
		write_query_end(&query, content);
		write_query_end(&query, " ");
		write_query_end(&query, "\n");
		break;

	case SENDING_TRAFFIC: // SENDING_TRAFFIC <Taille> <Contenu>

		write_query_end(&query, "SENDING_TRAFFIC ");
		write_query_end(&query, info);
		write_query_end(&query, " ");
		write_query_end(&query, content);
		write_query_end(&query, " ");
		write_query_end(&query, "\n");
		break;
		
	default:
		printf("invalid request");
		write_query_end(&query, "ERROR_INVALID_REQUEST\n");
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
	char *payload = malloc(sizeof(char) * 1024);
	printf("content : %s\n", q->content);
	sscanf(q->content, "%s %s %s", TOK, username, payload);
	printf("*********INTERPRETER***********\n");
	printf("%s\n", TOK);
	printf("%s\n", username);
	printf("%s\n", payload);
	query_t rep;
	tokens_t inst = convert_to_request(TOK);

	switch (inst)
	{

		int check;

	case LOG:
		printf("@LOG\n");
		printf("username : %s payload : %s\n", username, payload);
		if (!validate_login(username, payload))
		{
			printf("login failed\n");
			rep = serv_construire_message(LOG_FAILED, username, username);
			envoyer_query(socket, &rep);
			break;
		}
		printf("login success\n");
		int userIndex = 0;
		check = 0;
		char *content = malloc(sizeof(char) * MAX_USERS * 2 * MAX_INFO_SIZE);
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
			rep = serv_construire_message(LOG_FAILED, username, NULL);
			envoyer_query(socket, &rep);
			break;
		}
		for (int i = 0; i < master->User[userIndex]->nbConv; i++)
		{
			content = strcat(content, master->User[userIndex]->conversationID[i]);
			content = strcat(content, ":");
			content = strcat(content, master->User[userIndex]->conversationName[i]);
			content = strcat(content, ":");
		}
		printf("content : %s\n", content);
		rep = serv_construire_message(LOG_OK, username, content);
		envoyer_query(socket, &rep);
		break;

	case SEND:
		printf("@SEND\n");
		UserData_t **tabUser = master->User;
		size_t size = master->nbUser;
		check = 0;

		for (int i = 0; i < size; i++)
		{
			if (!strcmp(username, tabUser[i]->userID))
			{
				printf("user found\n");
				char *conv = strtok(payload, ":");
				printf("conv : %s\n", conv);
				for (int j = 0; j < tabUser[i]->nbConv; j++)
				{
					if (!strcmp(conv, tabUser[i]->conversationID[j]))
					{
						printf("conversation found\n");
						check = 1;
						char *size = strtok(NULL, ":");

						char *filePath = malloc(sizeof(char) * 256);
						snprintf(filePath, 256, "%s/%s.txt", PATH_CONV, conv);
						printf("path : %s\n", filePath);
						// sem_wait
						int sz = atoi(size) + 1;
						char *message = malloc(sizeof(char) * sz);
						read(socket, message, sz);
						char *formated = malloc(sizeof(char) * (sz + MAX_USER_NAME_LENGH));
						snprintf(formated, (sz + MAX_USER_NAME_LENGH), "%s : %s\n", username, message);
						int fdconv = open(filePath, O_WRONLY | O_APPEND);
						if (fdconv == -1)
						{
							perror("open");
							printf("PANIC A");
							exit(1);
						}
						printf("messaje written\n");
						write(fdconv, formated, (int)strlen(formated));
						close(fdconv);
						// sem_post
						break;
					}
				}
			}
		}

		if (check)
		{
			rep = serv_construire_message(OK, username, "accepted");
		}
		else
		{
			rep = serv_construire_message(DENIED, username, "no_user_found");
		}
		envoyer_query(socket, &rep);
		break;

	case UPDATE:

		printf("@UPDATE\n");
		// update conversation
		printf("ouvrir conversation\n");
		char *filePath = malloc(sizeof(char) * 256);
		snprintf(filePath, 256, "%s/%s.txt", PATH_CONV, payload);
		printf("path : %s\n", filePath);
		int fdconv = open(filePath, O_RDONLY);
		if (fdconv == -1)
		{
			perror("open");
			printf("PANIC AHHHHHHHHHHHHHHHHHHHH no file\n");
			rep = serv_construire_message(DENIED, username, "no_convo_found");
			envoyer_query(socket, &rep);
			return -1;
		}
		// taille de la discusion
		int sz = lseek(fdconv, 0, SEEK_END);
		lseek(fdconv, 0, SEEK_SET);
		char *rawtext = malloc(sizeof(char) * sz);
		read(fdconv, rawtext, sz);
		char *l_size = malloc(4);
		sprintf(l_size, "%d", sz + 3);
		char *sendtext = malloc(sizeof(char) * (sz + 3)); //+2 pour guillemet
		snprintf(sendtext, sizeof(char) * (sz + 3), "%s\n", rawtext);
		rep = serv_construire_message(SENDING_TRAFFIC, l_size, l_size); // envoie taille
		envoyer_query(socket, &rep);
		write(socket, sendtext, sz + 3); // envoie du text de la conv
		close(fdconv);
		break;

	case CREATE:

		printf("@CREATE\n");
		char *user = strtok(payload, ":");
		char *conversation = create_new_conversation_file(user);
		if (addParticipant(conversation, username, user) == -1)
		{
			rep = serv_construire_message(DENIED, username, "failed_to_create_new_converstion_or_invalid_participant");
			envoyer_query(socket, &rep);
			break;
		}
		while ((user = strtok(NULL, ":")) != NULL)
		{
			if(addParticipant(conversation, username, user))
			{
				rep = serv_construire_message(DENIED, username, "failed_to_create_new_converstion_or_invalid_participant_%s");
				envoyer_query(socket, &rep);
				break;
			}
		}
		rep = serv_construire_message(OK, conversation, "conversation_created");
		envoyer_query(socket, &rep);
		break;

	case SIGNIN:

		printf("@SIGNIN\n");
		printf("is_contact_valid : %d\n", is_contact_valid(username));
		if (is_contact_valid(username) == -1)
		{
			printf("Username already exists\n"),
				rep = serv_construire_message(DENIED, "Username already exists", "E_12");
			envoyer_query(socket, &rep);
		}
		else
		{
			printf("Creating account\n");
			write_login_to_file(username, payload);
			rep = serv_construire_message(OK, username, "Account created successfully");
			envoyer_query(socket, &rep);
		}
		break;

	case OK:

		break;

	default:

		printf("@INVALID_REQUEST\n");
		rep = serv_construire_message(DENIED, "Request has been denied", "E_13");
		envoyer_query(socket, &rep);
		break;
	}
	free(TOK);
	free(username);
	free(payload);
	return 0;
}

void write_login_to_file(char *username, char *password)
{
	char *hex_string = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
	char *hashed_password_hex = malloc(sizeof(char) * (SHA256_DIGEST_LENGTH * 2 + 1));

	// Hacher le mot de passe fourni
	hash_password(password, hashed_password_hex);

	// Écrire le nom d'utilisateur et le hash du mot de passe dans le fichier
	FILE *file = fopen("./database/login.txt", "a");
	if (file != NULL)
	{
		fprintf(file, "%s;%s\n", username, hashed_password_hex);
		fclose(file);
	}
	else
	{
		printf("Erreur lors de l'ouverture du fichier.\n");
	}

	char *filename = malloc(64);
	snprintf(filename, 64, "./database/users/%s.txt", username);

	file = fopen(filename, "w");
	if (file != NULL)
	{
		fprintf(file, "Firstname: %s\n", username);
		fprintf(file, "Lastname: %s\n", username);
		fclose(file);
	}
	else
	{
		printf("Error: Unable to create file\n");
	}
}

char rand_char()
{
	char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	size_t charset_size = sizeof(charset) - 1;
	return charset[rand() % charset_size];
}

void generate_random_id(char *id, size_t size)
{
	srand(time(NULL));
	for (size_t i = 0; i < size; ++i)
	{
		id[i] = rand_char();
	}
	id[size - 1] = '\0'; // Null-terminate the string
}

char *create_new_conversation_file(char *conv_name)
{
	char *chat_id = malloc(37);
	generate_random_id(chat_id, 37);

	char *filePath = malloc(256);
	snprintf(filePath, 256, "./database/conversations/%s.txt", chat_id);

	FILE *file = fopen(filePath, "w");
	if (file != NULL)
	{
		fprintf(file, "cia:\n"); // evite le chat vide
		fclose(file);
	}
	else
	{
		printf("Error creating conversation file: %s\n", filePath);
	}
	return chat_id;
}

int addParticipant(char *convId, char *nomconv, char *participant)
{
	if (is_contact_valid(participant) == 0)
	{

		char filename[64];
		snprintf(filename, 64, "./database/users/%s.txt", participant);

		int file = open(filename, O_WRONLY | O_APPEND);
		if (file == -1)
		{
			perror("open");
			return -1;
		}

		char *temp = malloc(64);
		snprintf(temp, 64, "%s;%s\n", nomconv, convId);
		write(file, temp, strlen(temp));
		free(temp);
		close(file);
		return 0;
	} else {
		printf("Participant not found in login.txt\n");
		return -1;
	}
}

void reload_database(masterDb_t *dbd)
{
	DIR *d = opendir("./database/users");

	if (d == NULL)
	{
		perror("no dir");
		return;
	}

	// check si le fichier est bien dans le dossier
	struct dirent *entry;
	int index = 0;
	printf("Ouverture du dossier\n");
	dbd->nbUser = 0;
	while ((entry = readdir(d)) != NULL)
	{
		printf("Ouverture du fichier: %s %hhu\n", entry->d_name, entry->d_type);
		if (entry->d_type == DT_REG)
		{
			// Check if the file extension is .txt
			char *extension = strstr(entry->d_name, ".txt");
			if (extension != NULL && strcmp(extension, ".txt") == 0)
			{
				char *filePath = malloc(sizeof(char) * 256);
				snprintf(filePath, 256, "%s/%s", PATH_USER, entry->d_name);
				// printf("path : %s\n", filePath);
				// fflush(stdout);
				int fd = open(filePath, O_RDONLY); // ouverture du fichier avec path
				if (fd == -1)
				{
					perror("open");
					exit(1);
				}
				char *buffer = malloc(sizeof(char) * 128);
				sscanf(entry->d_name, "%[^.].txt", buffer);
				dbd->User[index]->userID = buffer;
				for (int p = 0; p < NB_LIGNES_INFOS; p++)
				{
					char *discard = (char *)malloc(sizeof(char) * 128);
					read_until_nl(fd, discard);
					free(discard);
				}
				char **convName = malloc(sizeof(char *) * 64);
				char **conv = malloc(sizeof(char *) * 64);
				for (int i = 0; i < 64; i++)
				{
					conv[i] = malloc(sizeof(char) * 64);
				}
				for (int i = 0; i < 64; i++)
				{
					convName[i] = malloc(sizeof(char) * 64);
				}
				int j = 0;
				char **content = malloc(sizeof(char *) * 64);
				for (int i = 0; i < 64; i++)
				{
					content[i] = malloc(sizeof(char) * 64);
				}
				while (1 < read_until_nl(fd, content[j]))
				{
					sscanf(content[j], "%[^;];%s", convName[j], conv[j]);
					j++;
				}
				close(fd);
				dbd->User[index]->nbConv = j;
				dbd->User[index]->conversationID = conv;
				dbd->User[index]->conversationName = convName;
				index++;
			}
		}
	}
	dbd->nbUser = index;
	// printf("nbUser : %d\n", dbd->nbUser);
	closedir(d);
	// print_master(dbd);
}

void hash_password(char *password, char *hashed_password_hex)
{
	unsigned char hash[SHA256_DIGEST_LENGTH]; // Buffer for the binary hash
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, password, strlen(password));
	SHA256_Final(hash, &sha256);

	// Convert the binary hash to a hexadecimal string
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf(hashed_password_hex + (i * 2), "%02x", hash[i]);
	}
	hashed_password_hex[SHA256_DIGEST_LENGTH * 2] = 0; // Null-terminate the string
}

int validate_login(char *username, char *password)
{
	char *line = malloc(sizeof(char) * 256);
	char *file_username;
	char *file_password_hash;
	char *saveptr; // Pour strtok_r
	char *hashed_password_hex = malloc(sizeof(char) * (SHA256_DIGEST_LENGTH * 2 + 1));

	// Hacher le mot de passe fourni
	hash_password(password, hashed_password_hex);

	FILE *file = fopen("./database/login.txt", "r");
	if (file == NULL)
	{
		return 0;
	}

	while (fgets(line, sizeof(char) * 256, file) != NULL)
	{
		// printf("validate_login : iter\n");
		//  Supprime le saut de ligne à la fin si présent
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
			printf("Conv %s : %s\n", master->User[i]->conversationName[j], master->User[i]->conversationID[j]);
		}
	}
	printf("\n***************************\n");
}
