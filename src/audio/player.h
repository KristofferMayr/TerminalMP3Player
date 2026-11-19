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
void player_seek(double seconds);
void player_set_volume(int new_volume);

player_state player_get_state(void);
int player_get_volume(void);
double player_get_position(void);
double player_get_duration(void);

#endif
