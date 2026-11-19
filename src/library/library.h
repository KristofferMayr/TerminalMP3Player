#ifndef LIBRARY_H
#define LIBRARY_H

#include "library/id3.h"

typedef struct {
    char *path;       // kompletter Pfad zur Datei
    const char *name; // Dateiname, zeigt in path
    id3_tags tags;
} song;

typedef struct {
    song *songs;
    int count;
} library;

int library_load(library *lib, const char *dir);
void library_free(library *lib);

#endif
