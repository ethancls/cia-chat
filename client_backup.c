#include "./gtk_ui.h"
#include "./tcp_network.h"

/* Variables globales */
int sock;
user_t *user;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        usage(argv[0]);
        return 1;
    }

    printf("Connecting to %s\n", argv[1]);
    sock = creer_connecter_sock(argv[1], PORT_WCP);

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getpeername(sock, (struct sockaddr *)&addr, &addrlen) == -1)
    {
        perror("getpeername failed");
        close(sock);
        return 1;
    }

    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN) == NULL)
    {
        perror("inet_ntop failed");
        close(sock);
        return 1;
    }

    printf("Connected to %s\n", ip_str);

    gtk_init(&argc, &argv);

    open_login_window();

    gtk_main();

    return 0;
}

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

GHashTable *conversation_buttons;

void login(GtkWidget *widget, gpointer data)
{
    update = FALSE;
    char *username = gtk_entry_get_text(GTK_ENTRY(username_entry));
    char *password = gtk_entry_get_text(GTK_ENTRY(password_entry));

    user = malloc(sizeof(user_t));
    user->nb_conv = 0;
    user->u_pseudo = username;
    user->password = password;
    user->conversation = malloc(sizeof(convo_t *) * CONTENT_MAX_NB);
    for (int i = 0; i < CONTENT_MAX_NB; i++)
    {
        user->conversation[i] = malloc(sizeof(convo_t));
        user->conversation[i]->id_deconv = malloc(CONTENT_MAX_SIZE);
        user->conversation[i]->nom = malloc(32);
    }

    printf("Logging in as %s\n", user->u_pseudo);
    printf("Password: %s\n", user->password);

    query_t *q = malloc(sizeof(query_t));
    char **dataBuffer = malloc(sizeof(char *) * CONTENT_MAX_NB * 2);

    *q = construire_message(LOG, user->u_pseudo, user->password);
    printf("Built login query\n");
    envoyer_query(sock, q);
    printf("Sent login query\n");
    int reponse = interpreter_message(sock, dataBuffer);

    if (reponse == -1)
    {
        gtk_label_set_text(GTK_LABEL(error_label), "Invalid username or password");
        free(dataBuffer);
        return;
    }
    else
    {
        printf("Logged in\n");
        strncpy(user_name, username, sizeof(user_name) - 1);
        gtk_widget_hide(login_window);
        GHashTable *conversation_buttons = NULL;
        open_chat_window();

        printf("*************************LOADING DATA***************************************\n");
        if (dataBuffer[0] == NULL)
        {
            printf("No data loaded\n");
            user->nb_conv = 0;
        }
        else
        {
            int incr = 0;
            for (int i = 0; i < CONTENT_MAX_NB; i = i + 2)
            {
                if (dataBuffer[i] == NULL)
                {
                    user->nb_conv = incr;
                    break;
                }
                printf("id %d: %s ", i, dataBuffer[i]);
                // user->conversation[i]->id_deconv = malloc(strlen(dataBuffer[i]) + 1); // Allocation pour id_deconv
                strcpy(user->conversation[incr]->id_deconv, dataBuffer[i]);
                // printf("data loades in conversation %d: %s \n",i,user->conversation[incr]->id_deconv);
                strcpy(user->conversation[incr]->nom, dataBuffer[i + 1]);
                printf("name %d: %s \n", i + 1, dataBuffer[i + 1]);
                incr++;
            }
        }
        printf("*************************DATA LOADED***************************************\n");
        update = TRUE;
    }
    free(dataBuffer);
}

void logout(GtkWidget *widget, gpointer data)
{
    gtk_entry_set_text(GTK_ENTRY(username_entry), "");
    gtk_entry_set_text(GTK_ENTRY(password_entry), "");
    gtk_label_set_text(GTK_LABEL(error_label), "");

    gtk_widget_hide(chat_window);

    open_login_window();

    update = FALSE;
}

void on_contact_clicked(GtkWidget *widget, gpointer data)
{
    char *contact_name = gtk_button_get_label(GTK_BUTTON(widget));

    strncpy(actual_conversation, contact_name, sizeof(actual_conversation) - 1);

    load_chat_history(actual_conversation);
}

gboolean reload_messages(gpointer user_data)
{
    if (update == TRUE)
    {
        load_chat_history(actual_conversation);
        load_contacts();
    }
    return TRUE;
}

// Fonction pour démarrer la temporisation
void start_message_reload_timer()
{
    g_timeout_add(2000, reload_messages, NULL);
}

void add_to_conv(GtkWidget *widget, gpointer data)
{
    char *contact_name = gtk_entry_get_text(GTK_ENTRY(contact_entry));
    g_strstrip(contact_name);
    if (contact_name[0] == '\0')
        return;
    printf("Adding %s to conversation %s\n", contact_name, actual_conversation);
    fflush(stdout);
    char *conv_id = malloc(37);
    if (get_conversation_id(actual_conversation, conv_id) == FALSE)
    {
        printf("Conversation not found\n");
        return;
    }

    char *payload = malloc(1032);
    snprintf(payload, 1032, "%s:%s:", conv_id, contact_name);

    /*query_t q = construire_message(ADD, user->u_pseudo, payload);
    envoyer_query(sock, &q);

    char **content = malloc(sizeof(char *) * CONTENT_MAX_NB);
    int rep = interpreter_message(sock, content);
    if (rep == -1)
    {
        printf("Serveur had a problem : message ignored \n");
        return;
    }
    printf("Serveur : ACK, participant added to conversation : %s\n", content[0]);*/

    load_contacts();
}

void create_new_conversation(GtkWidget *widget, gpointer data)
{
    char *conv_name = gtk_entry_get_text(GTK_ENTRY(conv_name_entry));

    char *contact_name = gtk_entry_get_text(GTK_ENTRY(contact_entry));

    g_strstrip(contact_name);
    g_strstrip(conv_name);
    if (contact_name[0] == '\0' || conv_name[0] == '\0')
        return;

    strncpy(actual_conversation, conv_name, sizeof(actual_conversation) - 1);

    char **content = malloc(sizeof(char *) * CONTENT_MAX_NB);
    char *payload = malloc(1056);
    snprintf(payload, 1056, "%s:%s:", contact_name, user->u_pseudo);
    update = FALSE;
    query_t q = construire_message(CREATE, conv_name, payload);
    envoyer_query(sock, &q);
    int rep = interpreter_message(sock, content);
    if (rep == -1)
    {
        printf("failed to create : %s\n", content[0]);
        return;
    }
    printf("success created conversation : %s", content[0]);
    gtk_entry_set_text(GTK_ENTRY(conv_name_entry), "");
    gtk_entry_set_text(GTK_ENTRY(contact_entry), "");
    update = TRUE;
    load_contacts();

    free(content);
}

void maj()
{
    if (update == FALSE)
    {
        return;
    }
    else
    {
        printf("******MAJ******\n");
        char **content = malloc(sizeof(char *) * MAX_CONVERSATIONS_PER_USER * 2);
        query_t q = construire_message(LOG, user->u_pseudo, user->password);
        envoyer_query(sock, &q);
        int reponse = interpreter_message(sock, content);
        user->nb_conv = 0;
        if (reponse == -1)
        {
            printf("failed to update");
            return;
        }
        int incr = 0;
        for (int i = 0; i < MAX_CONVERSATIONS_PER_USER * 2; i = i + 2)
        {
            if (content[i] == NULL)
            {
                user->nb_conv = incr;
                break;
            }
            printf("data %d: %s \n", i, content[i]);
            strcpy(user->conversation[incr]->id_deconv, content[i]);
            strcpy(user->conversation[incr]->nom, content[i + 1]);
            incr++;
        }
        printf("*******MAJ FINISHED*******\n");
        free(content);
    }
}

void load_contacts()
{
    // Create a new CSS provider and load CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "./data/css/home.css", NULL);

    maj();

    if (conversation_buttons == NULL)
    {
        conversation_buttons = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    }

    GtkWidget *button;

    for (int i = 0; i < user->nb_conv; i++)
    {
        const char *current_name = user->conversation[i]->nom;

        button = g_hash_table_lookup(conversation_buttons, current_name);
        if (button == NULL)
        {
            button = gtk_button_new_with_label(current_name);
            gtk_list_box_insert(GTK_LIST_BOX(listbox), button, -1);
            g_signal_connect(button, "clicked", G_CALLBACK(on_contact_clicked), NULL);
            g_hash_table_insert(conversation_buttons, g_strdup(current_name), button);
            apply_css(listbox, GTK_STYLE_PROVIDER(provider));
        }
    }

    // Show all widgets in the window
    gtk_widget_show_all(chat_window);
}

void open_chat_window()
{
    // Crée un provider CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "./data/css/home.css", NULL);

    // Création de la fenêtre de chat
    chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    char title[128];
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

    // Création de la boîte horizontale contenant l'entrée pour le nom de la conversation, l'entrée pour les participants, et le bouton de création
    GtkWidget *hbox_conv = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(hbox_conv), FALSE);

    // Champ de saisie pour le nom de la conversation
    conv_name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(conv_name_entry), "Enter name...");
    gtk_box_pack_start(GTK_BOX(hbox_conv), conv_name_entry, FALSE, FALSE, 0);

    // Champ de saisie pour les participants
    contact_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(contact_entry), "Enter participants...");
    gtk_box_pack_start(GTK_BOX(hbox_conv), contact_entry, TRUE, TRUE, 0);

    // Bouton pour créer une nouvelle conversation
    GtkWidget *new_conv_button = gtk_button_new_with_label("Create");
    g_signal_connect(new_conv_button, "clicked", G_CALLBACK(create_new_conversation), NULL);
    // Reduce the width allocation by not allowing expansion
    gtk_box_pack_start(GTK_BOX(hbox_conv), new_conv_button, FALSE, FALSE, 0);

    g_signal_connect(contact_entry, "activate", G_CALLBACK(create_new_conversation), NULL);

    // Ajouter hbox_conv à vbox_left
    gtk_box_pack_start(GTK_BOX(vbox_left), hbox_conv, FALSE, FALSE, 0);

    // Création du menu défilant à gauche
    GtkWidget *scroll_menu = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_menu), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll_menu, 50, -1);
    gtk_box_pack_start(GTK_BOX(vbox_left), scroll_menu, TRUE, TRUE, 0);

    listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scroll_menu), listbox);

    update = TRUE;

    load_contacts();

    // Création de la zone principale de chat à droite
    GtkWidget *chat_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), chat_area, TRUE, TRUE, 0);

    // Zone de texte du chat avec défilement
    chat_view = gtk_text_view_new();
    gtk_widget_override_background_color(chat_view, GTK_STATE_FLAG_NORMAL, &(GdkRGBA){0.0, 0.0, 0.0, 1.0});
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

    // Création du bouton pour addParticipant
    GtkWidget *addParticipant = gtk_button_new_with_label("Add");
    g_signal_connect(addParticipant, "clicked", G_CALLBACK(add_to_conv), NULL);
    gtk_box_pack_start(GTK_BOX(hbox2), addParticipant, FALSE, FALSE, 0);

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
    gtk_widget_set_name(conv_name_entry, "conv_name_entry");
    gtk_widget_set_name(send_button, "send_button");
    gtk_widget_set_name(new_conv_button, "new_button");
    gtk_widget_set_name(logout_button, "logout_button");
    gtk_widget_set_name(addParticipant, "addParticipant");

    // Appliquer le CSS à la fenêtre de connexion
    apply_css(chat_window, GTK_STYLE_PROVIDER(provider));

    // Affichage de tous les widgets
    gtk_widget_show_all(chat_window);

    start_message_reload_timer();
}

void submit_signin(GtkWidget *widget, gpointer data) // envoyer
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
    update = FALSE;
    query_t q = construire_message(SIGNIN, trimmed_username, trimmed_password);
    envoyer_query(sock, &q);
    char **content = malloc(sizeof(char *) * CONTENT_MAX_NB);
    int reponse = interpreter_message(sock, content);
    printf("reponse : %d\n", reponse);
    if (reponse == -1)
    {
        gtk_label_set_text(GTK_LABEL(error_label), "Username already exists");

        return;
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(error_label), "Account created successfully");
        return;
    }
    g_free(trimmed_username);
    g_free(trimmed_password);
    g_free(trimmed_firstname);
    g_free(trimmed_lastname);
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

void send_message(GtkWidget *widget, gpointer data)
{
    char *message = g_strdup(gtk_entry_get_text(GTK_ENTRY(chat_entry)));
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
    char *conv_id = malloc(37);
    if (get_conversation_id(actual_conversation, conv_id) == FALSE)
    {
        printf("Conversation not found\n");
        return;
    }
    char *sizem = malloc(5);
    char *payload = malloc(1032);
    sprintf(sizem, "%d", (int)strlen(message) + 1);
    snprintf(payload, 1032, "%s:%s:", conv_id, sizem);

    query_t q;
    char **content = malloc(sizeof(char *) * CONTENT_MAX_NB);
    printf("Sending message to %s\n", actual_conversation);
    printf("Message: %s\n", message);
    q = construire_message(SEND, user->u_pseudo, payload);
    envoyer_query(sock, &q);
    write(sock, message, (int)strlen(message) + 1);

    int rep = interpreter_message(sock, content);
    if (rep == -1)
    {
        printf("Serveur had a problem : message ignored \n");
        return;
    }
    printf("Serveur : ACK, message saved on serveur : %s\n", content[0]);

    load_chat_history(actual_conversation);
    gtk_entry_set_text(GTK_ENTRY(chat_entry), "");
    free(message);
    free(payload);
    free(sizem);
}

void append_to_text_view(const gchar *text)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat_view));
    GtkTextIter end_iter;
    gtk_text_buffer_get_end_iter(buffer, &end_iter);
    const gchar *colors[10] = {"red", "cyan", "lime", "orange", "purple", "cyan", "magenta", "green", "pink", "teal"};

    GHashTable *user_colors = g_hash_table_new(g_str_hash, g_str_equal);

    gtk_text_buffer_create_tag(buffer, "white_text", "foreground", "white", "left_margin", 35, "family", "Arial", "pixels_below_lines", 4, NULL);

    gtk_text_buffer_insert(buffer, &end_iter, "\n", -1);

    gchar *last_username = NULL; // Stocke le dernier nom d'utilisateur

    gchar **lines = g_strsplit(text, "\n", -1);
    for (int i = 0; lines[i] != NULL; i++)
    {
        gchar **parts = g_strsplit(lines[i], ":", 2);
        if (parts[0] && parts[1])
        {
            gchar *color_tag;
            // Vérifier si un tag de couleur existe déjà pour cet utilisateur
            if (!g_hash_table_contains(user_colors, parts[0]))
            {
                // Attribuer une nouvelle couleur si l'utilisateur n'en a pas déjà une
                int color_index = g_hash_table_size(user_colors) % 10;
                color_tag = g_strdup_printf("%s_message", colors[color_index]);
                g_hash_table_insert(user_colors, g_strdup(parts[0]), g_strdup(color_tag));

                // Créer un tag pour le nom d'utilisateur avec cette nouvelle couleur uniquement s'il n'existe pas
                if (gtk_text_tag_table_lookup(gtk_text_buffer_get_tag_table(buffer), color_tag) == NULL)
                {
                    gtk_text_buffer_create_tag(buffer, color_tag, "foreground", colors[color_index], "weight", PANGO_WEIGHT_HEAVY, "left_margin", 20, "right_margin", 10, "family", "Arial", "pixels_below_lines", 4, NULL);
                }
            }
            else
            {
                color_tag = g_hash_table_lookup(user_colors, parts[0]);
            }

            // Si c'est le même utilisateur qui écrit, on n'affiche pas son nom d'utilisateur
            if (last_username == NULL || strcmp(last_username, parts[0]) != 0)
            {
                // Format et insertion du nom d'utilisateur
                gchar *username_formatted = g_strdup_printf("%s\n", parts[0]);
                gtk_text_buffer_insert_with_tags_by_name(buffer, &end_iter, username_formatted, -1, color_tag, NULL);
                g_free(username_formatted);
            }

            // Insertion du message avec la couleur blanche
            gchar *message_formatted = g_strdup_printf("%s\n", parts[1]);
            gtk_text_buffer_insert_with_tags_by_name(buffer, &end_iter, message_formatted, -1, "white_text", NULL);
            g_free(message_formatted);

            // Mise à jour du nom d'utilisateur précédent
            if (last_username != NULL)
                g_free(last_username);
            last_username = g_strdup(parts[0]);
        }
        g_strfreev(parts);
    }
    g_strfreev(lines);

    g_hash_table_destroy(user_colors);
    if (last_username != NULL)
        g_free(last_username);

    // Create a mark at the end of the buffer with right gravity (FALSE)
    GtkTextMark *end_mark = gtk_text_buffer_create_mark(buffer, NULL, &end_iter, FALSE);

    // Get the vertical adjustment to check the scroll position
    GtkAdjustment *adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(chat_view));
    gdouble upper = gtk_adjustment_get_upper(adj);
    gdouble page_size = gtk_adjustment_get_page_size(adj);
    gdouble value = gtk_adjustment_get_value(adj);

    // Check if the user is near the bottom of the text view
    if (value + page_size >= upper - 100)
    { // Adjusting this threshold if needed
        gtk_text_view_scroll_to_mark(chat_view, end_mark, 0.0, FALSE, 1.0, 1.0);
    }

    // Clean up: remove the mark after use
    gtk_text_buffer_delete_mark(buffer, end_mark);
}

gboolean get_conversation_id(char *partner_name, char *conversation_id)
{
    for (int i = 0; i < user->nb_conv; i++)
    {
        if (strcmp(user->conversation[i]->nom, partner_name) == 0)
        {
            strcpy(conversation_id, user->conversation[i]->id_deconv);
            return TRUE;
        }
    }
    return FALSE;
}

void load_chat_history(char *contact_name)
{
    query_t q;
    char **content = malloc(sizeof(char *) * CONTENT_MAX_NB);
    char *conv_id = malloc(37);
    printf("Loading chat history for %s\n", contact_name);
    if (!get_conversation_id(contact_name, conv_id))
    {
        printf("Conversation not found\n");
        return;
    }
    q = construire_message(UPDATE, user->u_pseudo, conv_id);
    envoyer_query(sock, &q);
    int rep = interpreter_message(sock, content);

    if (rep == -1)
    {
        printf("failed to get reponse : maybe convo doesnt exist? \n");
        return;
    }
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat_view));
    gtk_text_buffer_set_text(buffer, "", -1);
    append_to_text_view(content[0]);
    free(content);
    free(conv_id);
}
