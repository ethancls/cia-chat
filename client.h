#pragma once
#include <gtk/gtk.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Déclarations des fonctions
gboolean validate_login(const gchar *username, const gchar *password);
void login(GtkWidget *widget, gpointer data);
void send_file(GtkWidget *widget, gpointer data);
GdkPixbuf *get_pixbuf_from_file_resized(const char *filename, int width, int height);
void logout(GtkWidget *widget, gpointer data);
void open_chat_window();
void write_login_to_file(const char *username, const char *password);
void submit_signin(GtkWidget *widget, gpointer data);
void signin(GtkWidget *widget, gpointer data);
void open_login_window();
void send_message(GtkWidget *widget, gpointer data);
gboolean scroll_to_bottom(gpointer text_view);
void append_to_text_view(const gchar *text);
gboolean file_exists(const char *filename);
void load_chat_history();
gboolean get_conversation_id(const char* user_name, const char* partner_name, char* conversation_id);
void load_contacts_from_file();

// Déclarations des variables globales
GtkWidget *login_window;
GtkWidget *username_entry;
GtkWidget *password_entry;
GtkWidget *login_button;
GtkWidget *chat_window;
GtkWidget *chat_view;
GtkWidget *chat_entry;
GtkWidget *error_label;
GtkWidget *signin_window;
GtkWidget *signin_username_entry;
GtkWidget *signin_password_entry;
GtkWidget *signin_firstname_entry;
GtkWidget *signin_lastname_entry;
GtkWidget *listbox;
gchar actual_conversation[64];
gchar user_name[64];
