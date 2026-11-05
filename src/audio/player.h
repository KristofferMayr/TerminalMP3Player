#ifndef PLAYER_H
#define PLAYER_H

typedef enum {
    PLAYER_STOPPED,
    PLAYER_PLAYING,
    PLAYER_PAUSED
} player_state;

int player_init(void);
void player_close(void);

int player_play(const char *path);
void player_toggle_pause(void);
void player_stop(void);
void player_update(void);

player_state player_get_state(void);

#endif
