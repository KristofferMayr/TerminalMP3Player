#include "library/id3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 4 Bytes mit je 7 Bit ("syncsafe"), ID3v2 Groessen */
static unsigned read_syncsafe(const unsigned char *b) {
    return (b[0] << 21) | (b[1] << 14) | (b[2] << 7) | b[3];
}

/* 4 Bytes big endian */
static unsigned read_big_endian(const unsigned char *b) {
    return ((unsigned)b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

/* ein Unicode Zeichen als UTF-8 an out haengen, gibt neue Laenge zurueck */
static size_t append_utf8(char *out, size_t size, size_t len, unsigned c) {
    unsigned char buf[4];
    size_t n;

    if (c < 0x80) {
        buf[0] = c;
        n = 1;
    } else if (c < 0x800) {
        buf[0] = 0xC0 | (c >> 6);
        buf[1] = 0x80 | (c & 0x3F);
        n = 2;
    } else if (c < 0x10000) {
        buf[0] = 0xE0 | (c >> 12);
        buf[1] = 0x80 | ((c >> 6) & 0x3F);
        buf[2] = 0x80 | (c & 0x3F);
        n = 3;
    } else {
        buf[0] = 0xF0 | (c >> 18);
        buf[1] = 0x80 | ((c >> 12) & 0x3F);
        buf[2] = 0x80 | ((c >> 6) & 0x3F);
        buf[3] = 0x80 | (c & 0x3F);
        n = 4;
    }

    if (len + n >= size)
        return len; // voll
    memcpy(out + len, buf, n);
    out[len + n] = '\0';
    return len + n;
}

/* ISO-8859-1 -> UTF-8 (Bytewert = Unicode Nummer) */
static void decode_latin1(char *out, size_t size, const unsigned char *data, size_t len) {
    size_t n = 0;
    out[0] = '\0';
    for (size_t i = 0; i < len && data[i] != 0; i++)
        n = append_utf8(out, size, n, data[i]);
}

/* UTF-16 -> UTF-8, big = Byte-Reihenfolge */
static void decode_utf16(char *out, size_t size, const unsigned char *data, size_t len,
                         int big) {
    size_t n = 0;
    out[0] = '\0';

    for (size_t i = 0; i + 1 < len; i += 2) {
        unsigned c = big ? (data[i] << 8 | data[i + 1]) : (data[i + 1] << 8 | data[i]);
        if (c == 0)
            break;

        // Zeichen ueber 0xFFFF (z.B. Emojis) kommen als zwei Teile
        if (c >= 0xD800 && c < 0xDC00 && i + 3 < len) {
            unsigned low = big ? (data[i + 2] << 8 | data[i + 3]) : (data[i + 3] << 8 | data[i + 2]);
            c = 0x10000 + ((c - 0xD800) << 10) + (low - 0xDC00);
            i += 2;
        }
        n = append_utf8(out, size, n, c);
    }
}

/* Inhalt von einem Textframe, das erste Byte sagt die Kodierung */
static void decode_text(char *out, size_t size, const unsigned char *data, size_t len) {
    out[0] = '\0';
    if (len < 1)
        return;

    int encoding = data[0];
    data++;
    len--;

    if (encoding == 0) {
        decode_latin1(out, size, data, len);
    } else if (encoding == 1) {
        // UTF-16 mit BOM am Anfang
        if (len >= 2 && data[0] == 0xFE && data[1] == 0xFF)
            decode_utf16(out, size, data + 2, len - 2, 1);
        else if (len >= 2 && data[0] == 0xFF && data[1] == 0xFE)
            decode_utf16(out, size, data + 2, len - 2, 0);
    } else if (encoding == 2) {
        decode_utf16(out, size, data, len, 1);
    } else if (encoding == 3) {
        size_t n = len < size - 1 ? len : size - 1;
        memcpy(out, data, n);
        out[n] = '\0';
    }
}

/* ID3v2.3 / v2.4 Tag am Dateianfang */
static void read_id3v2(FILE *f, id3_tags *tags) {
    unsigned char header[10];
    if (fread(header, 1, 10, f) != 10 || memcmp(header, "ID3", 3) != 0)
        return;

    int version = header[3];
    if (version != 3 && version != 4)
        return;

    unsigned size = read_syncsafe(header + 6);
    unsigned char *data = malloc(size);
    if (data == NULL)
        return;
    if (fread(data, 1, size, f) != size) {
        free(data);
        return;
    }

    unsigned pos = 0;
    if ((header[5] & 0x40) && size >= 4) // Extended Header ueberspringen
        pos = version == 4 ? read_syncsafe(data) : read_big_endian(data) + 4;

    while (pos + 10 <= size) {
        const unsigned char *frame = data + pos;
        if (frame[0] == 0)
            break; // ab hier nur noch Padding

        unsigned frame_size = version == 4 ? read_syncsafe(frame + 4) : read_big_endian(frame + 4);
        if (frame_size > size - pos - 10)
            break;

        if (memcmp(frame, "TIT2", 4) == 0)
            decode_text(tags->title, ID3_TEXT_SIZE, frame + 10, frame_size);
        else if (memcmp(frame, "TPE1", 4) == 0)
            decode_text(tags->artist, ID3_TEXT_SIZE, frame + 10, frame_size);
        else if (memcmp(frame, "TALB", 4) == 0)
            decode_text(tags->album, ID3_TEXT_SIZE, frame + 10, frame_size);

        pos += 10 + frame_size;
    }

    free(data);
}

/* altes ID3v1 Tag (letzte 128 Bytes), fuellt nur leere Felder */
static void read_id3v1(FILE *f, id3_tags *tags) {
    unsigned char tag[128];
    if (fseek(f, -128, SEEK_END) != 0 || fread(tag, 1, 128, f) != 128 ||
        memcmp(tag, "TAG", 3) != 0)
        return;

    if (tags->title[0] == '\0')
        decode_latin1(tags->title, ID3_TEXT_SIZE, tag + 3, 30);
    if (tags->artist[0] == '\0')
        decode_latin1(tags->artist, ID3_TEXT_SIZE, tag + 33, 30);
    if (tags->album[0] == '\0')
        decode_latin1(tags->album, ID3_TEXT_SIZE, tag + 63, 30);
}

/* Leerzeichen am Ende weg (ID3v1 fuellt damit auf) */
static void trim_end(char *s) {
    size_t len = strlen(s);
    while (len > 0 && s[len - 1] == ' ')
        s[--len] = '\0';
}

/* Titel, Kuenstler, Album lesen, leer wenn nicht vorhanden */
void id3_read(const char *path, id3_tags *tags) {
    tags->title[0] = '\0';
    tags->artist[0] = '\0';
    tags->album[0] = '\0';

    FILE *f = fopen(path, "rb");
    if (f == NULL)
        return;

    read_id3v2(f, tags);
    read_id3v1(f, tags);
    fclose(f);

    trim_end(tags->title);
    trim_end(tags->artist);
    trim_end(tags->album);
}
