#include <stdio.h>

#include "app/app.h"
#include "audio/player.h"
#include "library/library.h"
#include "ui/ui.h"

/* Ordner einlesen, Player + UI starten */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Verwendung: %s <ordner>\n", argv[0]);
        return 1;
    }

    library lib;
    if (library_load(&lib, argv[1]) != 0) {
        printf("Ordner \"%s\" konnte nicht geöffnet werden\n", argv[1]);
        return 1;
    }
    if (lib.count == 0) {
        printf("Keine MP3-Dateien in \"%s\" gefunden\n", argv[1]);
        library_free(&lib);
        return 1;
    }

    if (player_init() != 0) {
        printf("Audioausgabe konnte nicht gestartet werden\n");
        library_free(&lib);
        return 1;
    }

    ui_init();
    app_run(&lib);
    ui_close();

    player_close();
    library_free(&lib);
    return 0;
}
