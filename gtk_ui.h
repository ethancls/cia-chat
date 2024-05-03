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

// gcc -Wall `pkg-config --cflags gtk+-3.0` client.c -o cia-chat `pkg-config --libs gtk+-3.0` -lssl -lcrypto

#define NB_LIGNES_INFOS 2

// Déclarations des fonctions
void apply_css(GtkWidget *widget, GtkStyleProvider *provider);
void hash_password(const char *password, char *hashed_password_hex);
gboolean validate_login(const gchar *username, const gchar *password);
void login(GtkWidget *widget, gpointer data);
void logout(GtkWidget *widget, gpointer data);
void on_contact_clicked(GtkWidget *widget, gpointer data);
gboolean is_contact_valid(const gchar *contact_name);
gboolean reload_messages(gpointer user_data);
void start_message_reload_timer();
void create_new_conversation(GtkWidget *widget, gpointer data);
void load_contacts();
void open_chat_window();
void to_hex_string(unsigned char *hash, char *output, size_t length);
void write_login_to_file(const char *username, const char *password);
void submit_signin(GtkWidget *widget, gpointer data);
void signin(GtkWidget *widget, gpointer data);
void open_login_window();
char rand_char();
void generate_random_id(char *id, size_t size);
void create_new_conversation_file(const char *conv_name);
void send_message(GtkWidget *widget, gpointer data);
void append_to_text_view(const gchar *text);
gboolean scroll_to_bottom(gpointer text_view);
gboolean file_exists(const char *filename);
gboolean get_conversation_id(char *partner_name, char *conversation_id);
void load_chat_history();

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
GtkWidget *conv_name_entry;
GtkWidget *contact_entry;
gchar actual_conversation[64];
gchar user_name[64];
gboolean update;
