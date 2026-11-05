#include "ui/ui.h"

#include <locale.h>
#include <ncurses.h>

static int first_visible = 0; // fuers Scrollen in der Liste

/* ncurses einrichten */
void ui_init(void) {
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100); // getch wartet max. 100 ms
}

/* Terminal wieder normal */
void ui_close(void) {
    endwin();
}

/* Text fuer die Statuszeile */
static const char *state_text(player_state state) {
    switch (state) {
    case PLAYER_PLAYING:
        return "Wiedergabe";
    case PLAYER_PAUSED:
        return "Pausiert";
    default:
        return "Gestoppt";
    }
}

/* Songliste, Status und Tastenhilfe zeichnen */
void ui_draw(const library *lib, int selected, int playing, player_state state,
             const char *message) {
    erase();

    mvprintw(0, 1, "TerminalMP3Player - %d Songs", lib->count);

    int list_top = 2;
    int list_height = LINES - 6;
    if (list_height < 1)
        list_height = 1;

    // Auswahl muss sichtbar bleiben
    if (selected < first_visible)
        first_visible = selected;
    if (selected >= first_visible + list_height)
        first_visible = selected - list_height + 1;

    for (int row = 0; row < list_height; row++) {
        int i = first_visible + row;
        if (i >= lib->count)
            break;

        if (i == selected)
            attron(A_REVERSE);
        mvaddstr(list_top + row, 1, i == playing ? "> " : "  ");
        addnstr(lib->songs[i].name, COLS - 4);
        if (i == selected)
            attroff(A_REVERSE);
    }

    if (message[0] != '\0')
        mvaddnstr(LINES - 3, 1, message, COLS - 2);

    mvprintw(LINES - 2, 1, "Status: %s", state_text(state));
    if (playing >= 0) {
        int space = COLS - getcurx(stdscr) - 4;
        if (space > 0) {
            addstr(" - ");
            addnstr(lib->songs[playing].name, space);
        }
    }

    mvaddnstr(LINES - 1, 1, "Enter: Abspielen   Leertaste: Pause/Weiter   s: Stopp   q: Beenden",
              COLS - 2);

    refresh();
}
