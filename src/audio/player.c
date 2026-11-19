#include "audio/player.h"

#include <fcntl.h>
#include <portaudio.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>

#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include "minimp3_ex.h"

#define OUTPUT_LATENCY 0.1 // Sekunden, mit weniger ruckelt es unter PipeWire

static mp3dec_ex_t decoder;
static int file_open = 0;
static PaStream *stream = NULL;
static player_state state = PLAYER_STOPPED;

// werden auch im Audio-Thread benutzt, deshalb atomic
static atomic_ullong position = 0; // in Samples
static atomic_int volume = 100;    // 0 - 100

/* PortAudio ruft das auf, wenn neue Samples gebraucht werden */
static int audio_callback(const void *input, void *output, unsigned long frames,
                          const PaStreamCallbackTimeInfo *time_info,
                          PaStreamCallbackFlags flags, void *user_data) {
    (void)input;
    (void)time_info;
    (void)flags;
    (void)user_data;

    mp3d_sample_t *out = output;
    size_t wanted = frames * decoder.info.channels;
    size_t got = mp3dec_ex_read(&decoder, out, wanted);

    int vol = volume;
    for (size_t i = 0; i < got; i++)
        out[i] = out[i] * vol / 100;

    // Rest mit Stille fuellen
    memset(out + got, 0, (wanted - got) * sizeof(mp3d_sample_t));
    position = decoder.cur_sample;

    if (got < wanted)
        return paComplete; // Song zu Ende
    return paContinue;
}

/* PortAudio starten, die ALSA Warnungen dabei nach /dev/null */
int player_init(void) {
    int saved_stderr = dup(STDERR_FILENO);
    int devnull = open("/dev/null", O_WRONLY);
    dup2(devnull, STDERR_FILENO);
    close(devnull);

    PaError err = Pa_Initialize();

    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);

    if (err != paNoError)
        return -1;
    return 0;
}

/* aktuellen Stream und Datei schliessen */
static void close_song(void) {
    if (stream != NULL) {
        Pa_AbortStream(stream);
        Pa_CloseStream(stream);
        stream = NULL;
    }
    if (file_open) {
        mp3dec_ex_close(&decoder);
        file_open = 0;
    }
    state = PLAYER_STOPPED;
    position = 0;
}

/* beim Beenden aufrufen */
void player_close(void) {
    close_song();
    Pa_Terminate();
}

/* Datei laden und von vorne abspielen, -1 bei Fehler */
int player_play(const char *path) {
    close_song();

    if (mp3dec_ex_open(&decoder, path, MP3D_SEEK_TO_SAMPLE) != 0)
        return -1;
    file_open = 1;

    // kein einziger mp3 frame gefunden -> keine echte mp3
    if (decoder.samples == 0) {
        close_song();
        return -1;
    }

    PaStreamParameters params;
    params.device = Pa_GetDefaultOutputDevice();
    params.channelCount = decoder.info.channels;
    params.sampleFormat = paInt16;
    params.suggestedLatency = OUTPUT_LATENCY;
    params.hostApiSpecificStreamInfo = NULL;

    if (params.device == paNoDevice) {
        close_song();
        return -1;
    }

    PaError err = Pa_OpenStream(&stream, NULL, &params, decoder.info.hz,
                                paFramesPerBufferUnspecified, paNoFlag, audio_callback, NULL);
    if (err != paNoError) {
        stream = NULL;
        close_song();
        return -1;
    }

    if (Pa_StartStream(stream) != paNoError) {
        close_song();
        return -1;
    }

    state = PLAYER_PLAYING;
    return 0;
}

/* Pause / weiter, nach Stopp wieder von vorne */
void player_toggle_pause(void) {
    if (stream == NULL)
        return;

    if (state == PLAYER_PLAYING) {
        // StopStream spielt den Puffer noch fertig, Position bleibt also genau
        Pa_StopStream(stream);
        state = PLAYER_PAUSED;
    } else {
        Pa_StartStream(stream);
        state = PLAYER_PLAYING;
    }
}

/* sofort anhalten, zurueck auf 0:00 */
void player_stop(void) {
    if (stream == NULL)
        return;

    Pa_AbortStream(stream);
    mp3dec_ex_seek(&decoder, 0);
    state = PLAYER_STOPPED;
    position = 0;
}

/* regelmaessig aufrufen, merkt wenn der Song fertig ist */
void player_update(void) {
    if (state == PLAYER_PLAYING && Pa_IsStreamActive(stream) == 0) {
        Pa_StopStream(stream);
        mp3dec_ex_seek(&decoder, 0);
        state = PLAYER_STOPPED;
        position = 0;
    }
}

/* um seconds vor (+) oder zurueck (-) spulen, nicht ueber Anfang/Ende hinaus */
void player_seek(double seconds) {
    if (state == PLAYER_STOPPED)
        return;

    // in Frames rechnen, ein Frame = ein Sample pro Kanal
    long long frame = (long long)((player_get_position() + seconds) * decoder.info.hz);
    long long last_frame = decoder.samples / decoder.info.channels;
    if (frame < 0)
        frame = 0;
    if (frame > last_frame)
        frame = last_frame;

    // waehrend dem Abspielen darf der Callback nicht gleichzeitig lesen
    if (state == PLAYER_PLAYING)
        Pa_AbortStream(stream);

    mp3dec_ex_seek(&decoder, frame * decoder.info.channels);
    position = frame * decoder.info.channels;

    if (state == PLAYER_PLAYING)
        Pa_StartStream(stream);
}

/* Lautstaerke 0 - 100 */
void player_set_volume(int new_volume) {
    if (new_volume < 0)
        new_volume = 0;
    if (new_volume > 100)
        new_volume = 100;
    volume = new_volume;
}

/* aktueller Zustand */
player_state player_get_state(void) {
    return state;
}

/* aktuelle Lautstaerke */
int player_get_volume(void) {
    return volume;
}

/* Position in Sekunden */
double player_get_position(void) {
    if (!file_open)
        return 0;
    return (double)position / decoder.info.hz / decoder.info.channels;
}

/* Laenge vom Song in Sekunden */
double player_get_duration(void) {
    if (!file_open)
        return 0;
    return (double)decoder.samples / decoder.info.hz / decoder.info.channels;
}
