#ifndef ID3_H
#define ID3_H

#define ID3_TEXT_SIZE 128

typedef struct {
    char title[ID3_TEXT_SIZE];
    char artist[ID3_TEXT_SIZE];
    char album[ID3_TEXT_SIZE];
} id3_tags;

void id3_read(const char *path, id3_tags *tags);

#endif
