#include "ui/ui.h"

#include <locale.h>
#include <ncurses.h>
#include <stdio.h>

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

/* Sekunden als mm:ss */
static void format_time(char *buf, size_t size, double seconds) {
    int s = (int)seconds;
    snprintf(buf, size, "%02d:%02d", s / 60, s % 60);
}

/* Zeile "Label  Wert", Wert wird auf die Breite gekuerzt */
static void draw_field(int y, const char *label, const char *value) {
    mvaddstr(y, 1, label);
    int space = COLS - getcurx(stdscr) - 1;
    if (space > 0)
        addnstr(value[0] != '\0' ? value : "-", space);
}

/* z.B. 01:23 [#######-------] -02:10 */
static void draw_progress(int y) {
    double pos = player_get_position();
    double len = player_get_duration();

    char pos_text[16];
    char rest_text[16];
    format_time(pos_text, sizeof(pos_text), pos);
    format_time(rest_text, sizeof(rest_text), len - pos);

    int width = COLS - 18; // Rest geht fuer die Zeiten drauf
    if (width < 1)
        return;
    int filled = len > 0 ? (int)(width * pos / len) : 0;

    mvprintw(y, 1, "%s [", pos_text);
    for (int i = 0; i < width; i++)
        addch(i < filled ? '#' : '-');
    printw("] -%s", rest_text);
}

/* Songliste, aktueller Song, Status und Tastenhilfe zeichnen */
void ui_draw(const library *lib, int selected, int playing, const char *message) {
    erase();

    mvprintw(0, 1, "TerminalMP3Player - %d Songs", lib->count);

    int list_top = 2;
    int list_height = LINES - 9;
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
        mvaddnstr(LINES - 7, 1, message, COLS - 2);

    if (playing >= 0) {
        const id3_tags *tags = &lib->songs[playing].tags;
        draw_field(LINES - 6, "Titel:     ", tags->title);
        draw_field(LINES - 5, "Künstler:  ", tags->artist);
        draw_field(LINES - 4, "Album:     ", tags->album);
    }
    draw_progress(LINES - 3);

    mvprintw(LINES - 2, 1, "Status: %-10s  Lautstärke: %d%%", state_text(player_get_state()),
             player_get_volume());

    mvaddstr(LINES - 1, 1, "Enter Abspielen | Space Pause | s Stopp | ←→ Spulen | +- Lautstärke | q Ende");

    refresh();
}
