#include "./client.h"

// Fonction pour charger le fichier CSS
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

gboolean validate_login(const gchar *username, const gchar *password)
{
    gboolean valid = FALSE;
    gchar line[256];
    gchar *file_username;
    gchar *file_password;
    gchar *saveptr; // Pour strtok_r

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
        file_password = strtok_r(NULL, ";", &saveptr);

        if (file_username && file_password &&
            strcmp(username, file_username) == 0 &&
            strcmp(password, file_password) == 0)
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
}

GdkPixbuf *get_pixbuf_from_file_resized(const char *filename, int width, int height)
{
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    if (!pixbuf)
    {
        g_printerr("Error loading file: %s\n", error->message);
        g_error_free(error);
        return NULL;
    }

    GdkPixbuf *resized_pixbuf = gdk_pixbuf_scale_simple(pixbuf, width, height, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf); // Libère la mémoire de l'ancien GdkPixbuf

    return resized_pixbuf; // Retourne le nouveau GdkPixbuf redimensionné
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

void open_chat_window()
{
    // Crée un provider CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "./data/css/signin.css", NULL);

    // Création de la fenêtre de chat
    chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(chat_window), "CIA Chat");

    gtk_window_set_default_size(GTK_WINDOW(chat_window), 1920 * 0.5, 1200 * 0.5);
    gtk_window_set_position(GTK_WINDOW(chat_window), GTK_WIN_POS_CENTER);
    g_signal_connect(chat_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_window_set_keep_above(GTK_WINDOW(chat_window), TRUE); // Garde la fenêtre au premier plan

    // Création d'une boîte horizontale pour organiser les widgets
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(chat_window), hbox);

    // Création du menu défilant à gauche
    GtkWidget *scroll_menu = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_name(scroll_menu, "scrollable_msg");
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_menu), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scroll_menu, 100, -1); // Ajustement de la largeur minimale du menu
    gtk_box_pack_start(GTK_BOX(hbox), scroll_menu, FALSE, TRUE, 0);

    GtkWidget *listbox = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(scroll_menu), listbox);

    // Ajouter des éléments au menu défilant (remplacer par votre propre logique)
    GtkWidget *item1 = gtk_button_new_with_label("Marcus");
    GtkWidget *item2 = gtk_button_new_with_label("Tom");
    GtkWidget *item3 = gtk_button_new_with_label("Jake");

    // Ajoutez les éléments à la liste
    gtk_container_add(GTK_CONTAINER(listbox), item1);
    gtk_container_add(GTK_CONTAINER(listbox), item2);
    gtk_container_add(GTK_CONTAINER(listbox), item3);

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
    GtkWidget *logout_button = gtk_button_new();
    GdkPixbuf *resized_logout = get_pixbuf_from_file_resized("./exit-light.png", 24, 24); // Redimensionne l'image
    if (resized_logout)
    {
        GtkWidget *logout_icon = gtk_image_new_from_pixbuf(resized_logout);
        gtk_button_set_image(GTK_BUTTON(logout_button), logout_icon);
        g_object_unref(resized_logout); // Libère la mémoire du GdkPixbuf redimensionné
    }
    gtk_button_set_always_show_image(GTK_BUTTON(logout_button), TRUE);
    g_signal_connect(logout_button, "clicked", G_CALLBACK(logout), NULL);
    gtk_box_pack_start(GTK_BOX(hbox2), logout_button, FALSE, FALSE, 0);

    // Création du bouton d'envoi de fichier avec une icône
    GtkWidget *file_button = gtk_button_new();
    GdkPixbuf *resized_pixbuf = get_pixbuf_from_file_resized("./attach-hover-dark.png", 24, 24); // Redimensionne l'image
    if (resized_pixbuf)
    {
        GtkWidget *file_icon = gtk_image_new_from_pixbuf(resized_pixbuf);
        gtk_button_set_image(GTK_BUTTON(file_button), file_icon);
        g_object_unref(resized_pixbuf); // Libère la mémoire du GdkPixbuf redimensionné
    }
    gtk_button_set_always_show_image(GTK_BUTTON(file_button), TRUE);
    g_signal_connect(file_button, "clicked", G_CALLBACK(send_file), NULL);
    gtk_box_pack_start(GTK_BOX(hbox2), file_button, FALSE, FALSE, 0);

    // Champ de saisie pour les messages
    chat_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(chat_entry), "Type your message here...");
    gtk_box_pack_start(GTK_BOX(hbox2), chat_entry, TRUE, TRUE, 0);

    g_signal_connect(chat_entry, "activate", G_CALLBACK(send_message), NULL);

    // Bouton d'envoi
    GtkWidget *send_button = gtk_button_new();
    GdkPixbuf *resized_send_icon = get_pixbuf_from_file_resized("./send-hover-dark.png", 24, 24); // Redimensionne l'image
    if (resized_send_icon)
    {
        GtkWidget *send_icon = gtk_image_new_from_pixbuf(resized_send_icon);
        gtk_button_set_image(GTK_BUTTON(send_button), send_icon);
        g_object_unref(resized_send_icon); // Libère la mémoire du GdkPixbuf redimensionné
    }
    gtk_button_set_always_show_image(GTK_BUTTON(send_button), TRUE);
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_message), NULL);
    gtk_box_pack_start(GTK_BOX(hbox2), send_button, FALSE, FALSE, 0);

    // Appliquer le CSS à la fenêtre de connexion
    apply_css(chat_window, GTK_STYLE_PROVIDER(provider));

    // Affichage de tous les widgets
    gtk_widget_show_all(chat_window);

    // Charger l'historique de la discussion
    load_chat_history();
}

void write_login_to_file(const char *username, const char *password)
{
    FILE *file = fopen("./database/login.txt", "a"); // Ouvre le fichier en mode append
    if (file != NULL)
    {
        fprintf(file, "%s;%s\n", username, password);
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
    snprintf(filename, sizeof(filename), "./database/%s.txt", trimmed_username);

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


void exit_signin(GtkWidget *widget, gpointer data){
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

    // Ajoute le bouton de connexion
    login_button = gtk_button_new_with_label("Login");
    gtk_widget_set_name(login_button, "login_button");
    g_signal_connect(login_button, "clicked", G_CALLBACK(login), NULL);
    gtk_widget_set_hexpand(login_button, TRUE); // Pour étendre la largeur
    gtk_box_pack_start(GTK_BOX(hbox_buttons), login_button, TRUE, TRUE, 0);

    // Création du bouton Signin
    GtkWidget *signin_button = gtk_button_new_with_label("Sign In");
    gtk_widget_set_name(signin_button, "signin_button");
    g_signal_connect(signin_button, "clicked", G_CALLBACK(signin), NULL);
    gtk_widget_set_hexpand(signin_button, TRUE); // Pour étendre la largeur
    gtk_box_pack_start(GTK_BOX(hbox_buttons), signin_button, TRUE, TRUE, 0);

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

void send_message(GtkWidget *widget, gpointer data)
{
    gchar *message = g_strdup(gtk_entry_get_text(GTK_ENTRY(chat_entry)));
    g_strstrip(message); // Supprime les espaces de début et de fin
    if (message[0] == '\0')
        return; // Ne rien faire si le message est vide

    // Construction du message avec le nom de l'utilisateur
    char full_message[512];
    snprintf(full_message, sizeof(full_message), "%s: %s\n", user_name, message);

    // Ajouter le message à la vue de chat
    append_to_text_view(full_message);

    // Sauvegarder le message dans le fichier
    FILE *file = fopen("./database/chat_data.txt", "a");
    if (file != NULL)
    {
        fputs(full_message, file);
        fclose(file);
    }

    // Effacer le champ de saisie
    gtk_entry_set_text(GTK_ENTRY(chat_entry), "");
}

gboolean scroll_to_bottom(gpointer text_view);

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

// Fonction de callback pour effectuer le défilement
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

void load_chat_history()
{
    if (!file_exists("./database/chat_data.txt"))
        return;

    FILE *file = fopen("./database/chat_data.txt", "r");
    char line[512];

    while (fgets(line, sizeof(line), file) != NULL)
    {
        append_to_text_view(line);
    }

    fclose(file);
}
