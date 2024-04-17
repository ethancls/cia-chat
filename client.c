#include "./client.h"

void apply_css(GtkWidget *widget, GtkStyleProvider *provider)
{
    gtk_style_context_add_provider(gtk_widget_get_style_context(widget),
                                   provider,
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    if (GTK_IS_CONTAINER(widget))
    {
        gtk_container_forall(GTK_CONTAINER(widget), (GtkCallback)apply_css, provider);
    }
}

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

void login(GtkWidget *widget, gpointer data)
{
    const gchar *username = gtk_entry_get_text(GTK_ENTRY(username_entry));
    const gchar *password = gtk_entry_get_text(GTK_ENTRY(password_entry));

    if (validate_login(username, password))
    {
        strncpy(user_name, username, sizeof(user_name) - 1);
        gtk_widget_hide(login_window);
        open_chat_window();
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(error_label), "Invalid username or password");
    }
}

void send_file(GtkWidget *widget, gpointer data)
{
    printf("Sended file\n");
    return;
}

void logout(GtkWidget *widget, gpointer data)
{
    // Cache la fenêtre de chat
    gtk_widget_hide(chat_window);

    // Réinitialise les champs de saisie de l'écran de connexion
    gtk_entry_set_text(GTK_ENTRY(username_entry), "");
    gtk_entry_set_text(GTK_ENTRY(password_entry), "");

    // Affiche la fenêtre de connexion
    gtk_widget_show_all(login_window);
}

void on_contact_clicked(GtkWidget *widget, gpointer data)
{
    const gchar *contact_name = gtk_button_get_label(GTK_BUTTON(widget));

    strncpy(actual_conversation, contact_name, sizeof(actual_conversation) - 1);

    load_chat_history();
}

gboolean is_contact_valid(const gchar *contact_name)
{
    char line[256];
    gboolean contact_found = FALSE;
    gchar *file_username;
    gchar *saveptr;

    FILE *file = fopen("./database/login.txt", "r");
    if (file == NULL)
    {
        return FALSE;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        line[strcspn(line, "\r\n")] = 0;

        file_username = strtok_r(line, ";", &saveptr);

        if (strcmp(contact_name, file_username) == 0)
        {
            contact_found = TRUE;
            break;
        }
    }
    fclose(file);

    // Check already a conversation with this contact
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "./database/users/%s.txt", user_name);
    file = fopen(filePath, "r");
    if (file != NULL)
    {
        char line[512];
        int line_count = 0;
        while (fgets(line, sizeof(line), file) != NULL)
        {
            if (line_count >= 2)
            {
                char *file_partner_name = strtok(line, ";");
                if (strcmp(contact_name, file_partner_name) == 0)
                {
                    contact_found = FALSE;
                    break;
                }
            }
            line_count++;
        }
        fclose(file);
    }

    if (strcmp(user_name, contact_name) == 0)
    {
        contact_found = FALSE;
    }

    return contact_found;
}

// Déclarez la fonction de temporisation
gboolean reload_messages(gpointer user_data)
{
    load_chat_history();
    return TRUE;
}

// Fonction pour démarrer la temporisation
void start_message_reload_timer()
{
    // Spécifiez l'intervalle en millisecondes
    const guint interval_milliseconds = 100;

    // Ajoutez une temporisation périodique avec l'intervalle spécifié
    g_timeout_add(interval_milliseconds, reload_messages, NULL);
}

void create_new_conversation(GtkWidget *widget, gpointer data)
{
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "./data/css/home.css", NULL);
    gchar *contact_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(data)));
    g_strstrip(contact_name);
    if (contact_name[0] == '\0')
        return;

    // Vérifier si le contact est valide
    if (!is_contact_valid(contact_name))
    {
        printf("Invalid contact name.\n");
        gtk_entry_set_text(GTK_ENTRY(data), "");
        return;
    }

    strncpy(actual_conversation, contact_name, sizeof(actual_conversation) - 1);

    GtkWidget *listbox_item = gtk_button_new_with_label(contact_name);
    g_signal_connect(listbox_item, "clicked", G_CALLBACK(on_contact_clicked), NULL);
    gtk_list_box_insert(GTK_LIST_BOX(listbox), listbox_item, -1);

    gtk_entry_set_text(GTK_ENTRY(data), "");

    apply_css(chat_window, GTK_STYLE_PROVIDER(provider));

    gtk_widget_show_all(chat_window);
}

void load_contacts_from_file()
{
    gchar filePath[256];
    snprintf(filePath, sizeof(filePath), "./database/users/%s.txt", user_name);
    FILE *file = fopen(filePath, "r");
    if (file != NULL)
    {
        gchar line[512];
        gint line_count = 1;
        while (fgets(line, sizeof(line), file) != NULL)
        {
            if (line_count > NB_LIGNES_INFOS)
            {
                gchar *contact_name = strtok(line, ";");
                if (line_count == NB_LIGNES_INFOS + 1)
                {
                    strncpy(actual_conversation, contact_name, sizeof(actual_conversation) - 1);
                }
                GtkWidget *button = gtk_button_new_with_label(contact_name);
                gtk_list_box_insert(GTK_LIST_BOX(listbox), button, -1);
                g_signal_connect(button, "clicked", G_CALLBACK(on_contact_clicked), NULL);
            }
            line_count++;
        }

        fclose(file);
    }
    else
    {
        printf("Error opening file %s\n", filePath);
    }
}

void open_chat_window()
{
    // Crée un provider CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "./data/css/home.css", NULL);

    // Création de la fenêtre de chat
    chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    char title[64];
    snprintf(title, sizeof(title), "Welcome Agent %s !", user_name);
    gtk_window_set_title(GTK_WINDOW(chat_window), title);

    gtk_window_set_default_size(GTK_WINDOW(chat_window), 1920 * 0.5, 1200 * 0.5);
    gtk_window_set_position(GTK_WINDOW(chat_window), GTK_WIN_POS_CENTER);
    g_signal_connect(chat_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Création d'une boîte horizontale pour organiser les widgets
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(chat_window), hbox);

    // Création d'une boîte verticale pour organiser les widgets du côté gauche
    GtkWidget *vbox_left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), vbox_left, FALSE, FALSE, 0);

    // Création de la boîte horizontale contenant l'entrée et le bouton
    GtkWidget *hbox_entry = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(hbox_entry), FALSE);

    // Champ de saisie pour le nom du contact
    GtkWidget *contact_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(contact_entry), "Enter contact name...");
    gtk_box_pack_start(GTK_BOX(hbox_entry), contact_entry, TRUE, TRUE, 0);
    g_signal_connect(contact_entry, "activate", G_CALLBACK(create_new_conversation), contact_entry);

    // Bouton pour créer une nouvelle conversation
    GtkWidget *new_conv_button = gtk_button_new_with_label("New");
    g_signal_connect(new_conv_button, "clicked", G_CALLBACK(create_new_conversation), contact_entry);
    gtk_box_pack_start(GTK_BOX(hbox_entry), new_conv_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox_left), hbox_entry, FALSE, FALSE, 0);

    // Création du menu défilant à gauche
    GtkWidget *scroll_menu = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_menu), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll_menu, 100, -1);                  // Ajustement de la largeur minimale du menu
    gtk_box_pack_start(GTK_BOX(vbox_left), scroll_menu, TRUE, TRUE, 0); // Ajout de la propriété expand à TRUE

    listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scroll_menu), listbox);

    load_contacts_from_file();

    // Création de la zone principale de chat à droite
    GtkWidget *chat_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), chat_area, TRUE, TRUE, 0);

    // Zone de texte du chat avec défilement
    chat_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(chat_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(chat_view), FALSE);
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), chat_view);
    gtk_box_pack_start(GTK_BOX(chat_area), scrolled_window, TRUE, TRUE, 0);

    // Zone de saisie pour les messages avec boutons dans une boîte horizontale
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(hbox2), FALSE);           // Pour que les widgets ne soient pas de la même taille
    gtk_container_set_border_width(GTK_CONTAINER(hbox2), 10); // Ajout d'une marge autour de hbox
    gtk_box_pack_start(GTK_BOX(chat_area), hbox2, FALSE, TRUE, 0);

    // Création du bouton pour quitter
    GtkWidget *logout_button = gtk_button_new_with_label("Exit");
    g_signal_connect(logout_button, "clicked", G_CALLBACK(logout), NULL);
    gtk_box_pack_start(GTK_BOX(hbox2), logout_button, FALSE, FALSE, 0);

    // Création du bouton d'envoi de fichier
    GtkWidget *file_button = gtk_button_new_with_label("Load File");
    g_signal_connect(file_button, "clicked", G_CALLBACK(send_file), NULL);
    gtk_box_pack_start(GTK_BOX(hbox2), file_button, FALSE, FALSE, 0);

    // Champ de saisie pour les messages
    chat_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(chat_entry), "Type your message here...");
    gtk_box_pack_start(GTK_BOX(hbox2), chat_entry, TRUE, TRUE, 0);

    g_signal_connect(chat_entry, "activate", G_CALLBACK(send_message), chat_entry);

    // Bouton d'envoi
    GtkWidget *send_button = gtk_button_new_with_label("Send");
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_message), NULL);
    gtk_box_pack_start(GTK_BOX(hbox2), send_button, FALSE, FALSE, 0);

    // Définir des identifiants CSS pour les widgets
    gtk_widget_set_name(hbox, "chat_area");
    gtk_widget_set_name(contact_entry, "contact_entry");
    gtk_widget_set_name(chat_window, "chat_window");
    gtk_widget_set_name(listbox, "listbox");
    gtk_widget_set_name(chat_area, "chat_area");
    gtk_widget_set_name(chat_entry, "chat_entry");
    gtk_widget_set_name(scrolled_window, "chat_scrolled_window");
    gtk_widget_set_name(send_button, "send_button");
    gtk_widget_set_name(new_conv_button, "new_button");
    gtk_widget_set_name(logout_button, "logout_button");
    gtk_widget_set_name(file_button, "file_button");

    // Appliquer le CSS à la fenêtre de connexion
    apply_css(chat_window, GTK_STYLE_PROVIDER(provider));

    // Affichage de tous les widgets
    gtk_widget_show_all(chat_window);

    start_message_reload_timer();
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

void submit_signin(GtkWidget *widget, gpointer data)
{
    // Obtenez le nom d'utilisateur et le mot de passe des entrées
    const char *username = g_strdup(gtk_entry_get_text(GTK_ENTRY(signin_username_entry)));
    const char *password = g_strdup(gtk_entry_get_text(GTK_ENTRY(signin_password_entry)));
    const char *firstname = g_strdup(gtk_entry_get_text(GTK_ENTRY(signin_firstname_entry)));
    const char *lastname = g_strdup(gtk_entry_get_text(GTK_ENTRY(signin_lastname_entry)));

    // Supprimez les espaces de début et de fin des chaînes de caractères
    gchar *trimmed_username = g_strstrip(g_strdup(username));
    gchar *trimmed_password = g_strstrip(g_strdup(password));
    gchar *trimmed_firstname = g_strstrip(g_strdup(firstname));
    gchar *trimmed_lastname = g_strstrip(g_strdup(lastname));

    // Vérifiez si les champs sont vides après suppression des espaces
    if (strlen(trimmed_username) == 0 || strlen(trimmed_password) == 0 || strlen(trimmed_firstname) == 0 ||
        strlen(trimmed_lastname) == 0)
    {
        // Affiche un message d'erreur dans le label d'erreur
        gtk_label_set_text(GTK_LABEL(error_label), "Please fill in all fields");
        g_free(trimmed_username);
        g_free(trimmed_password);
        g_free(trimmed_firstname);
        g_free(trimmed_lastname);
        return;
    }

    // Écrivez le login dans le fichier
    write_login_to_file(trimmed_username, trimmed_password);

    char filename[64];
    snprintf(filename, sizeof(filename), "./database/users/%s.txt", trimmed_username);

    FILE *file = fopen(filename, "w");
    if (file != NULL)
    {
        fprintf(file, "Firstname: %s\n", trimmed_firstname);
        fprintf(file, "Lastname: %s\n", trimmed_lastname);
        fclose(file);
    }
    else
    {
        printf("Error: Unable to create file\n");
    }

    // Libérer la mémoire allouée pour les chaînes de caractères
    g_free(trimmed_username);
    g_free(trimmed_password);
    g_free(trimmed_firstname);
    g_free(trimmed_lastname);

    // Fermer la fenêtre de Signin
    gtk_widget_hide(signin_window);

    // Afficher à nouveau la fenêtre de login
    open_login_window();
}

void exit_signin(GtkWidget *widget, gpointer data)
{
    gtk_widget_hide(signin_window);
    open_login_window();
}

void signin(GtkWidget *widget, gpointer data)
{
    // Fermer la fenêtre de login actuelle
    gtk_widget_hide(login_window);

    // Crée un provider CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "./data/css/signin.css", NULL);

    // Création de la fenêtre de connexion
    signin_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(signin_window, "auth_screen"); // CSS ID
    gtk_window_set_title(GTK_WINDOW(signin_window), "Sign In");
    gtk_window_set_default_size(GTK_WINDOW(signin_window), 1920 * 0.5, 1200 * 0.5);
    gtk_window_set_position(GTK_WINDOW(signin_window), GTK_WIN_POS_CENTER);
    g_signal_connect(signin_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Crée une grille pour organiser les widgets
    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(signin_window), grid);

    // Crée une boîte verticale pour les entrées et le bouton
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_name(vbox, "login_menu");
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 30); // Ajout d'une marge autour de vbox
    gtk_grid_attach(GTK_GRID(grid), vbox, 0, 0, 1, 1);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(vbox, TRUE); // Pour que la boîte verticale s'étende verticalement
    gtk_widget_set_vexpand(vbox, TRUE); // Pour que la boîte verticale s'étende verticalement
    gtk_widget_set_valign(vbox, GTK_ALIGN_END);

    // Ajoute les entrées
    signin_firstname_entry = gtk_entry_new();
    gtk_widget_set_name(signin_firstname_entry, "firstname_entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(signin_firstname_entry), "First Name");
    gtk_box_pack_start(GTK_BOX(vbox), signin_firstname_entry, FALSE, FALSE, 0);

    signin_lastname_entry = gtk_entry_new();
    gtk_widget_set_name(signin_lastname_entry, "lastname_entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(signin_lastname_entry), "Last Name");
    gtk_box_pack_start(GTK_BOX(vbox), signin_lastname_entry, FALSE, FALSE, 0);

    signin_username_entry = gtk_entry_new();
    gtk_widget_set_name(signin_username_entry, "username_entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(signin_username_entry), "Username");
    gtk_box_pack_start(GTK_BOX(vbox), signin_username_entry, FALSE, FALSE, 0);

    signin_password_entry = gtk_entry_new();
    gtk_widget_set_name(signin_password_entry, "password_entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(signin_password_entry), "Password");
    gtk_entry_set_visibility(GTK_ENTRY(signin_password_entry), FALSE); // Hide password text
    gtk_box_pack_start(GTK_BOX(vbox), signin_password_entry, FALSE, FALSE, 0);

    g_signal_connect(signin_password_entry, "activate", G_CALLBACK(submit_signin), NULL); // Appuyez sur Entrée pour vous connecter

    // Crée une boîte horizontale pour les boutons Login et Signin
    GtkWidget *hbox_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_name(hbox_buttons, "button_box");
    gtk_box_pack_start(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);

    // Ajoute le bouton de connexion
    GtkWidget *exit_button = gtk_button_new_with_label("Exit");
    gtk_widget_set_name(exit_button, "exit_button");
    g_signal_connect(exit_button, "clicked", G_CALLBACK(exit_signin), NULL);
    gtk_widget_set_hexpand(exit_button, TRUE); // Pour étendre la largeur
    gtk_box_pack_start(GTK_BOX(hbox_buttons), exit_button, TRUE, TRUE, 0);

    // Création du bouton Signin
    GtkWidget *signin_button = gtk_button_new_with_label("Sign In");
    gtk_widget_set_name(signin_button, "signin_button");
    g_signal_connect(signin_button, "clicked", G_CALLBACK(submit_signin), NULL);
    gtk_widget_set_hexpand(signin_button, TRUE); // Pour étendre la largeur
    gtk_box_pack_start(GTK_BOX(hbox_buttons), signin_button, TRUE, TRUE, 0);

    // Création de l'étiquette pour afficher le message d'erreur
    error_label = gtk_label_new("");
    gtk_widget_set_name(error_label, "error_label"); // Nom de classe CSS

    // Ajout de l'étiquette à la boîte verticale vbox
    gtk_box_pack_start(GTK_BOX(vbox), error_label, FALSE, FALSE, 0);

    // Appliquer le CSS à la fenêtre de connexion
    apply_css(signin_window, GTK_STYLE_PROVIDER(provider));

    gtk_widget_show_all(signin_window);
}

void open_login_window()
{
    // Crée un provider CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "./data/css/auth.css", NULL);

    // Création de la fenêtre de connexion
    login_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(login_window, "auth_screen"); // CSS ID
    gtk_window_set_title(GTK_WINDOW(login_window), "Login");
    gtk_window_set_default_size(GTK_WINDOW(login_window), 1920 * 0.5, 1200 * 0.5);
    gtk_window_set_position(GTK_WINDOW(login_window), GTK_WIN_POS_CENTER);
    g_signal_connect(login_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Crée une grille pour organiser les widgets
    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(login_window), grid);

    // Crée une boîte verticale pour les entrées et le bouton
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_name(vbox, "login_menu");
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 30); // Ajout d'une marge autour de vbox
    gtk_grid_attach(GTK_GRID(grid), vbox, 0, 0, 1, 1);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(vbox, TRUE); // Pour que la boîte verticale s'étende verticalement
    gtk_widget_set_vexpand(vbox, TRUE); // Pour que la boîte verticale s'étende verticalement
    gtk_widget_set_valign(vbox, GTK_ALIGN_END);

    // Ajoute les entrées pour le nom d'utilisateur et le mot de passe
    username_entry = gtk_entry_new();
    gtk_widget_set_name(username_entry, "username_entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(username_entry), "Username");
    gtk_box_pack_start(GTK_BOX(vbox), username_entry, FALSE, FALSE, 0);

    password_entry = gtk_entry_new();
    gtk_widget_set_name(password_entry, "password_entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(password_entry), "Password");
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE); // Hide password text
    gtk_box_pack_start(GTK_BOX(vbox), password_entry, FALSE, FALSE, 0);
    g_signal_connect(password_entry, "activate", G_CALLBACK(login), NULL); // Appuyez sur Entrée pour vous connecter

    // Crée une boîte horizontale pour les boutons Login et Signin
    GtkWidget *hbox_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_name(hbox_buttons, "button_box");
    gtk_box_pack_start(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);

    // Création du bouton Signin
    GtkWidget *signin_button = gtk_button_new_with_label("Sign In");
    gtk_widget_set_name(signin_button, "signin_button");
    g_signal_connect(signin_button, "clicked", G_CALLBACK(signin), NULL);
    gtk_widget_set_hexpand(signin_button, TRUE); // Pour étendre la largeur
    gtk_box_pack_start(GTK_BOX(hbox_buttons), signin_button, TRUE, TRUE, 0);

    // Ajoute le bouton de connexion
    login_button = gtk_button_new_with_label("Login");
    gtk_widget_set_name(login_button, "login_button");
    g_signal_connect(login_button, "clicked", G_CALLBACK(login), NULL);
    gtk_widget_set_hexpand(login_button, TRUE); // Pour étendre la largeur
    gtk_box_pack_start(GTK_BOX(hbox_buttons), login_button, TRUE, TRUE, 0);

    // Création de l'étiquette pour afficher le message d'erreur
    error_label = gtk_label_new("");
    gtk_widget_set_name(error_label, "error_label"); // Nom de classe CSS

    // Ajout de l'étiquette à la boîte verticale vbox
    gtk_box_pack_start(GTK_BOX(vbox), error_label, FALSE, FALSE, 0);

    // Appliquer le CSS à la fenêtre de connexion
    apply_css(login_window, GTK_STYLE_PROVIDER(provider));

    gtk_widget_show_all(login_window);
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    open_login_window();

    gtk_main();

    return 0;
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

void send_message(GtkWidget *widget, gpointer data)
{
    gchar *message = g_strdup(gtk_entry_get_text(GTK_ENTRY(chat_entry)));
    g_strstrip(message);
    if (message[0] == '\0')
    {
        printf("Empty message\n");
        return;
    }
    if (user_name[0] == '\0' || actual_conversation[0] == '\0')
    {
        printf("Select a conversation\n");
        return;
    }
    // Construction du message avec le nom de l'utilisateur
    char full_message[512];
    snprintf(full_message, sizeof(full_message), "%s: %s\n", user_name, message);

    // Ajouter le message à la vue de chat
    append_to_text_view(full_message);

    char conversation_id[37];
    if (!get_conversation_id(user_name, actual_conversation, conversation_id))
    {
        create_new_conversation_file(actual_conversation);
        return;
    }

    char filePath[256];
    snprintf(filePath, sizeof(filePath), "./database/conversations/%s.txt", conversation_id);

    // Sauvegarder le message dans le fichier
    FILE *file = fopen(filePath, "a");
    if (file != NULL)
    {
        fputs(full_message, file);
        fclose(file);
    }

    // Effacer le champ de saisie
    gtk_entry_set_text(GTK_ENTRY(chat_entry), "");
}

void append_to_text_view(const gchar *text)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat_view));
    GtkTextIter end_iter;

    // Obtient le GtkTextIter pointant à la fin du buffer
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    // Insère le texte au pointeur de fin (ajoute le texte à la fin du buffer)
    gtk_text_buffer_insert(buffer, &end_iter, text, -1);

    // Défilement automatique vers le bas en utilisant g_idle_add
    g_idle_add(scroll_to_bottom, chat_view);
}

gboolean scroll_to_bottom(gpointer data)
{
    GtkTextView *text_view = GTK_TEXT_VIEW(data);
    GtkAdjustment *adjustment = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(text_view));

    // Définit la valeur de l'ajustement à son maximum, ce qui fait défiler vers le bas
    gtk_adjustment_set_value(adjustment, gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));

    // Retourne FALSE pour que g_idle_add() n'appelle pas cette fonction une seconde fois
    return FALSE;
}

gboolean file_exists(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file)
    {
        fclose(file);
        return TRUE;
    }
    return FALSE;
}

gboolean get_conversation_id(const char *user_name, const char *partner_name, char *conversation_id)
{
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "./database/users/%s.txt", user_name);
    FILE *file = fopen(filePath, "r");
    if (!file)
    {
        return FALSE;
    }

    char line[512];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *file_partner_name = strtok(line, ";");
        char *file_conversation_id = strtok(NULL, "\n");

        if (strcmp(file_partner_name, partner_name) == 0)
        {
            strcpy(conversation_id, file_conversation_id);
            fclose(file);
            return TRUE;
        }
    }

    fclose(file);
    return FALSE;
}

void load_chat_history()
{
    // Clear the chat view
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat_view));
    gtk_text_buffer_set_text(buffer, "", -1);

    char conversation_id[37];
    if (!get_conversation_id(user_name, actual_conversation, conversation_id))
    {
        return;
    }

    char filePath[256];
    snprintf(filePath, sizeof(filePath), "./database/conversations/%s.txt", conversation_id);

    if (access(filePath, F_OK) == -1)
    {
        printf("File %s not found.\n", filePath);
        return;
    }

    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        printf("Error opening file %s\n", filePath);
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), file) != NULL)
    {
        append_to_text_view(line);
    }

    fclose(file);
}
