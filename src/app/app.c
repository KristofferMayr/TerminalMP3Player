#include "app/app.h"

#include <ncurses.h>

#include "audio/player.h"
#include "ui/ui.h"

#define SEEK_STEP 5   // Sekunden
#define VOLUME_STEP 5 // Prozent

/* Hauptschleife: Taste lesen, Player steuern, neu zeichnen */
void app_run(const library *lib) {
    int selected = 0;
    int playing = -1; // Index vom geladenen Song, -1 = keiner
    const char *message = "";
    int running = 1;

    while (running) {
        player_update();
        ui_draw(lib, selected, playing, message);

        int key = getch();
        switch (key) {
        case KEY_UP:
            if (selected > 0)
                selected--;
            break;
        case KEY_DOWN:
            if (selected < lib->count - 1)
                selected++;
            break;
        case '\n':
        case KEY_ENTER:
            if (player_play(lib->songs[selected].path) == 0) {
                playing = selected;
                message = "";
            } else {
                playing = -1;
                message = "Fehler: Datei konnte nicht abgespielt werden";
            }
            break;
        case ' ':
            player_toggle_pause();
            break;
        case 's':
            player_stop();
            break;
        case KEY_RIGHT:
            player_seek(SEEK_STEP);
            break;
        case KEY_LEFT:
            player_seek(-SEEK_STEP);
            break;
        case '+':
            player_set_volume(player_get_volume() + VOLUME_STEP);
            break;
        case '-':
            player_set_volume(player_get_volume() - VOLUME_STEP);
            break;
        case 'q':
            running = 0;
            break;
        }
    }
}
