/* sdl2/sdl_snd.c - SDL2 audio backend for EGL
 * Implements a DMA-style audio backend using SDL2 audio callback.
 */

#include "../client/snd_local.h"
#include "../unix/unix_local.h"
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>

/* Sound cvars - normally in unix_snd_main.c but we need them here for SDL2 build */
cVar_t *s_bits;
cVar_t *s_speed;
cVar_t *s_channels;

static cVar_t *s_sdl_device;
static cVar_t *s_sdl_buffer_ms; /* desired buffer size in milliseconds */

static SDL_AudioDeviceID s_sdl_dev = 0;
static SDL_AudioSpec s_sdl_want, s_sdl_have;

/* Local ring state */
static int s_sdl_buf_bytes = 0;
static int s_sdl_read_pos = 0; /* byte offset read by callback */

/* Callback: fill stream from snd_audioDMA.buffer */
static void SDLAudioCallback (void *userdata, Uint8 *stream, int len)
{
    if (!snd_audioDMA.buffer || snd_audioDMA.samples == 0) {
        SDL_memset (stream, 0, len);
        return;
    }

    int bufsize = s_sdl_buf_bytes;
    int read = s_sdl_read_pos;

    while (len > 0) {
        int chunk = bufsize - read;
        if (chunk > len) chunk = len;
        memcpy (stream, snd_audioDMA.buffer + read, chunk);
        stream += chunk;
        len -= chunk;
        read += chunk;
        if (read >= bufsize) read = 0;
    }

    s_sdl_read_pos = read;

    /* advance sample pos (mono samples) */
    if (snd_audioDMA.sampleBits/8 > 0) {
        int bytes_per_sample = (snd_audioDMA.sampleBits/8) * snd_audioDMA.channels;
        snd_audioDMA.samplePos = s_sdl_read_pos / bytes_per_sample;
    }
}

qBool Snd_SDL_Init (void)
{
    /* Register sound cvars if not already done */
    if (!s_bits) s_bits = Cvar_Register ("s_bits", "16", CVAR_ARCHIVE);
    if (!s_speed) s_speed = Cvar_Register ("s_speed", "44100", CVAR_ARCHIVE);
    if (!s_channels) s_channels = Cvar_Register ("s_channels", "2", CVAR_ARCHIVE);

    int bits = s_bits ? s_bits->intVal : 16;
    int channels = s_channels ? s_channels->intVal : 2;
    int speed = 0;

    /* Register cvars */
    if (!s_sdl_device) s_sdl_device = Cvar_Register ("s_sdl_device", "", CVAR_ARCHIVE);
    if (!s_sdl_buffer_ms) s_sdl_buffer_ms = Cvar_Register ("s_sdl_buffer_ms", "200", CVAR_ARCHIVE);

    /* determine speed from s_khz or s_speed */
    if (s_khz && s_khz->intVal) {
        switch (s_khz->intVal) {
        case 48: speed = 48000; break;
        case 44: speed = 44100; break;
        case 22: speed = 22050; break;
        default: speed = 11025; break;
        }
    }
    if (!speed && s_speed && s_speed->intVal) speed = s_speed->intVal;
    if (!speed) speed = 44100;

    snd_audioDMA.sampleBits = bits;
    snd_audioDMA.channels = channels;
    snd_audioDMA.speed = speed;

    /* allocate ring buffer based on buffer_ms */
    int buf_ms = s_sdl_buffer_ms ? s_sdl_buffer_ms->intVal : 200;
    if (buf_ms < 50) buf_ms = 50;

    int mono_samples = (speed * buf_ms) / 1000;
    snd_audioDMA.samples = mono_samples;
    snd_audioDMA.submissionChunk = 1024; /* conservative chunk */

    int bytes_per_sample = (bits/8) * channels;
    s_sdl_buf_bytes = mono_samples * bytes_per_sample;

    if (snd_audioDMA.buffer) Mem_Free (snd_audioDMA.buffer);
    snd_audioDMA.buffer = (byte *) Mem_Alloc (s_sdl_buf_bytes);
    memset (snd_audioDMA.buffer, 0, s_sdl_buf_bytes);
    snd_audioDMA.samplePos = 0;

    /* Initialize SDL audio subsystem if needed */
    if (SDL_WasInit (SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem (SDL_INIT_AUDIO) != 0) {
            Com_Printf (PRNT_ERROR, "Snd_SDL_Init: SDL Init audio failed: %s\n", SDL_GetError ());
            return qFalse;
        }
    }

    SDL_zero (s_sdl_want);
    s_sdl_want.freq = speed;
    s_sdl_want.format = (bits == 16) ? AUDIO_S16SYS : AUDIO_U8;
    s_sdl_want.channels = channels;
    s_sdl_want.samples = 1024; /* callback buffer size */
    s_sdl_want.callback = SDLAudioCallback;
    s_sdl_want.userdata = NULL;

    s_sdl_dev = SDL_OpenAudioDevice (s_sdl_device && s_sdl_device->string[0] ? s_sdl_device->string : NULL, 0, &s_sdl_want, &s_sdl_have, 0);
    if (s_sdl_dev == 0) {
        Com_Printf (PRNT_ERROR, "Snd_SDL_Init: Failed to open audio: %s\n", SDL_GetError ());
        return qFalse;
    }

    /* Start playback */
    SDL_PauseAudioDevice (s_sdl_dev, 0);
    s_sdl_read_pos = 0;

    Com_Printf (0, "Snd_SDL_Init: audio %d Hz, %d-bit, %d channels, buffer %d ms\n", speed, bits, channels, buf_ms);

    return qTrue;
}

void Snd_SDL_Shutdown (void)
{
    if (s_sdl_dev) {
        SDL_CloseAudioDevice (s_sdl_dev);
        s_sdl_dev = 0;
    }

    if (snd_audioDMA.buffer) {
        Mem_Free (snd_audioDMA.buffer);
        snd_audioDMA.buffer = NULL;
    }

    /* Quit audio subsystem if not used by others */
    if (SDL_WasInit (SDL_INIT_AUDIO))
        SDL_QuitSubSystem (SDL_INIT_AUDIO);
}

int Snd_SDL_GetDMAPos (void)
{
    /* Return current sample position (mono samples) */
    return snd_audioDMA.samplePos;
}

void Snd_SDL_BeginPainting (void)
{
    if (s_sdl_dev)
        SDL_LockAudioDevice (s_sdl_dev);
}

void Snd_SDL_Submit (void)
{
    if (s_sdl_dev)
        SDL_UnlockAudioDevice (s_sdl_dev);
}

qBool SndImp_Init (void)
{
    return Snd_SDL_Init ();
}

void SndImp_Shutdown (qBool full)
{
    (void)full;
    Snd_SDL_Shutdown ();
}

int SndImp_GetDMAPos (void)
{
    return Snd_SDL_GetDMAPos ();
}

void SndImp_BeginPainting (void)
{
    Snd_SDL_BeginPainting ();
}

void SndImp_Submit (void)
{
    Snd_SDL_Submit ();
}
