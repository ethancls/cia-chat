#include "./client.h"

void hash_password(const char *password, char *hashed_password_hex)
{
    unsigned char hashed_password[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char *)password, strlen(password), hashed_password);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(hashed_password_hex + (i * 2), "%02x", hashed_password[i]);
    }
    hashed_password_hex[SHA256_DIGEST_LENGTH * 2] = '\0';
}

gboolean validate_login(const gchar *username, const gchar *password)
{
    gboolean valid = FALSE;
    gchar line[256];
    gchar *file_username;
    gchar *file_password_hash;
    gchar *saveptr; // Pour strtok_r
    gchar hashed_password_hex[SHA256_DIGEST_LENGTH * 2 + 1];

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
            valid = TRUE;
            break;
        }
    }

    fclose(file);
    return valid;
}

void to_hex_string(unsigned char *hash, char *output, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[length * 2] = '\0';
}

void write_login_to_file(const char *username, const char *password)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char hex_string[SHA256_DIGEST_LENGTH * 2 + 1];

    // Hachage du mot de passe
    if (!SHA256((const unsigned char *)password, strlen(password), hash))
    {
        g_print("Erreur lors du hachage du mot de passe.\n");
        return;
    }

    // Convertir le hash en chaîne hexadécimale
    to_hex_string(hash, hex_string, sizeof(hash));

    // Écrire le nom d'utilisateur et le hash du mot de passe dans le fichier
    FILE *file = fopen("./database/login.txt", "a");
    if (file != NULL)
    {
        fprintf(file, "%s;%s\n", username, hex_string);
        fclose(file);
    }
    else
    {
        g_print("Erreur lors de l'ouverture du fichier.\n");
    }
}

char rand_char()
{
    const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const size_t charset_size = sizeof(charset) - 1;
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

void create_new_conversation_file(const char *conv_name)
{
    char chat_id[37];
    generate_random_id(chat_id, sizeof(chat_id));

    char filePath[256];
    snprintf(filePath, sizeof(filePath), "./database/conversations/%s.txt", chat_id);

    FILE *file = fopen(filePath, "w");
    if (file != NULL)
    {
        fclose(file);
        char filePath[256];
        printf("Creating conversation file: %s, %s\n", conv_name, user_name);
        snprintf(filePath, sizeof(filePath), "./database/users/%s.txt", user_name);

        FILE *file = fopen(filePath, "a");
        if (file != NULL)
        {
            fprintf(file, "%s;%s\n", conv_name, chat_id);
            fclose(file);
        }
        else
        {
            printf("Error appending data to %s\n", filePath);
        }

        snprintf(filePath, sizeof(filePath), "./database/users/%s.txt", conv_name);

        file = fopen(filePath, "a");
        if (file != NULL)
        {
            fprintf(file, "%s;%s\n", user_name, chat_id);
            fclose(file);
        }
        else
        {
            printf("Error appending data to %s\n", filePath);
        }
    }
    else
    {
        printf("Error creating conversation file: %s\n", filePath);
    }
}

bool file_exists(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file)
    {
        fclose(file);
        return TRUE;
    }
    return FALSE;
}