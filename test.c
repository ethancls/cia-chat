#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_MSG 100  // Nombre maximal de messages à afficher

void init_colors() {
    if (has_colors() == FALSE) {
        endwin();
        printf("Votre terminal ne supporte pas les couleurs\n");
        exit(1);
    }
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
}

int main() {
    char messages[MAX_MSG][80]; // Stockage des messages
    int num_messages = 0;

    initscr();          // Initialiser la fenêtre ncurses
    cbreak();           // Désactiver le buffering des lignes, passer Ctrl-C à l'appli
    noecho();           // Ne pas afficher les entrées du clavier à l'écran
    keypad(stdscr, TRUE); // Activer les touches du clavier

    init_colors();

    // Diviser la fenêtre principale en deux sous-fenêtres
    int mid_y = LINES - 3;
    WINDOW *topwin = newwin(mid_y, COLS, 0, 0);
    WINDOW *botwin = newwin(3, COLS, mid_y, 0);
    box(topwin, 0, 0);
    box(botwin, 0, 0);

    scrollok(topwin, TRUE);

    // Boucle principale de la messagerie
    while (1) {
        // Affichage des messages
        werase(topwin);
        box(topwin, 0, 0);
        for (int i = 0; i < num_messages; i++) {
            wmove(topwin, i + 1, 1);
            wprintw(topwin, "%s", messages[i]);
        }
        wrefresh(topwin);

        // Saisie de l'utilisateur
        werase(botwin);
        box(botwin, 0, 0);
        mvwprintw(botwin, 1, 1, "Message: ");
        wgetnstr(botwin, messages[num_messages], 78);
        
        // Quitter si l'utilisateur tape "exit"
        if (strcmp(messages[num_messages], "exit") == 0) {
            break;
        }

        if (num_messages < MAX_MSG - 1) {
            num_messages++;
        }
        wrefresh(botwin);
    }

    // Nettoyage et fermeture
    delwin(topwin);
    delwin(botwin);
    endwin();

    return 0;
}
