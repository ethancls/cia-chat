/* Timothée M'BASSIDJE && Ethan NICOLAS -- 2024*/

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
#include <time.h> 

#define NB_LIGNES_INFOS 2

/* Function prototypes*/
int main(int argc, char *argv[]);
void open_login_window(void);
void open_chat_window(void);

/* Function prototypes for login and signup */
void login(GtkWidget *widget, gpointer data);
void signin(GtkWidget *widget, gpointer data);
void submit_signin(GtkWidget *widget, gpointer data);
void exit_signin(GtkWidget *widget, gpointer data);
void logout(GtkWidget *widget, gpointer data);

/* Function prototypes for chat interactions */
void send_message(GtkWidget *widget, gpointer data);
void create_new_conversation(GtkWidget *widget, gpointer data);
void load_contacts(void);
void load_chat_history(char *contact_name);
gboolean get_conversation_id(char *partner_name, char *conversation_id);
void apply_css(GtkWidget *widget, GtkStyleProvider *provider);
void on_contact_clicked(GtkWidget *widget, gpointer data);
gboolean reload_messages(gpointer user_data);
void start_message_reload_timer(void);
void append_to_text_view(const gchar *text);
void maj(void);

/* Global Variables for GTK */
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
