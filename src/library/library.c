#include "library/library.h"

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

/* endet der Name auf .mp3? (gross/klein egal) */
static int is_mp3(const char *filename) {
    size_t len = strlen(filename);
    return len > 4 && strcasecmp(filename + len - 4, ".mp3") == 0;
}

/* fuer qsort, alphabetisch nach Dateiname */
static int compare_songs(const void *a, const void *b) {
    const song *sa = a;
    const song *sb = b;
    return strcasecmp(sa->name, sb->name);
}

/* alle mp3 Dateien aus dir einlesen, -1 wenn Ordner nicht lesbar */
int library_load(library *lib, const char *dir) {
    lib->songs = NULL;
    lib->count = 0;

    DIR *d = opendir(dir);
    if (d == NULL)
        return -1;

    int capacity = 0;
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (!is_mp3(entry->d_name))
            continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

        // Ordner die auf .mp3 enden usw. ueberspringen
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode))
            continue;

        if (lib->count == capacity) {
            capacity = capacity == 0 ? 16 : capacity * 2;
            song *bigger = realloc(lib->songs, capacity * sizeof(song));
            if (bigger == NULL)
                break;
            lib->songs = bigger;
        }

        song *s = &lib->songs[lib->count];
        s->path = strdup(path);
        s->name = strrchr(s->path, '/') + 1;

        id3_read(s->path, &s->tags);
        if (s->tags.title[0] == '\0') // keine Tags -> Dateiname ohne .mp3
            snprintf(s->tags.title, ID3_TEXT_SIZE, "%.*s", (int)strlen(s->name) - 4, s->name);

        lib->count++;
    }
    closedir(d);

    if (lib->count > 0)
        qsort(lib->songs, lib->count, sizeof(song), compare_songs);
    return 0;
}

/* Speicher wieder freigeben */
void library_free(library *lib) {
    for (int i = 0; i < lib->count; i++)
        free(lib->songs[i].path);
    free(lib->songs);
    lib->songs = NULL;
    lib->count = 0;
}
