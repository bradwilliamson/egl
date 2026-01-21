/* sdl2/sdl_snd.c - SDL2 audio backend for EGL
 * Implements a DMA-style audio backend using SDL2 audio callback.
 */

#include "../client/snd_local.h"
#include "../unix/unix_local.h"
#if defined(__has_include)
#  if __has_include(<SDL2/SDL.h>)
#    include <SDL2/SDL.h>
#  else
#    include <SDL.h>
#  endif
#else
#  include <SDL2/SDL.h>
#endif
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

/* Local ring state - position in BYTES */
static volatile int s_sdl_buf_bytes = 0;
static volatile int s_sdl_play_pos = 0; /* byte offset being played (read by callback) */

static int Snd_SDL_NextPow2 (int x)
{
    int p;

    if (x <= 0)
        return 1;

    p = 1;
    while (p < x && p > 0)
        p <<= 1;

    return (p > 0) ? p : x;
}

/* Callback: fill stream from snd_audioDMA.buffer
 * This runs in a separate audio thread - must be careful about shared state.
 */
static void SDLAudioCallback (void *userdata, Uint8 *stream, int len)
{
    byte *buf = snd_audioDMA.buffer;
    int bufsize = s_sdl_buf_bytes;
    
    if (!buf || bufsize == 0 || snd_audioDMA.samples == 0) {
        SDL_memset (stream, 0, len);
        return;
    }

    int pos = s_sdl_play_pos;
    Uint8 *out = stream;
    int remaining = len;
    
    while (remaining > 0) {
        int avail = bufsize - pos;
        int chunk = (avail < remaining) ? avail : remaining;
        
        memcpy (out, buf + pos, chunk);
        
        out += chunk;
        remaining -= chunk;
        pos += chunk;
        
        if (pos >= bufsize) 
            pos = 0;
    }

    s_sdl_play_pos = pos;
}

qBool Snd_SDL_Init (void)
{
    /* Register sound cvars if not already done */
    if (!s_bits) s_bits = Cvar_Register ("s_bits", "16", CVAR_ARCHIVE);
    if (!s_speed) s_speed = Cvar_Register ("s_speed", "48000", CVAR_ARCHIVE);
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

    /*
     * IMPORTANT: snd_audioDMA.samples is defined as MONO samples in the buffer
     * (i.e. frames * channels). The DMA mixer assumes snd_audioDMA.samples is
     * a power-of-two and uses (samples - 1) as a wrap mask.
     */
    {
        const int bytes_per_mono_sample = bits / 8;
        int desired_frames = (speed * buf_ms) / 1000;
        int desired_mono_samples = desired_frames * channels;

        /* Ensure a power-of-two ring size for mask-based wrap. */
        snd_audioDMA.samples = Snd_SDL_NextPow2 (desired_mono_samples);

        /* Keep submissionChunk a power-of-two for alignment masking. */
        snd_audioDMA.submissionChunk = 1024;
        if (snd_audioDMA.submissionChunk > snd_audioDMA.samples)
            snd_audioDMA.submissionChunk = snd_audioDMA.samples;

        s_sdl_buf_bytes = snd_audioDMA.samples * bytes_per_mono_sample;
    }

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
    s_sdl_play_pos = 0;

    Com_Printf (0, "Snd_SDL_Init: audio %d Hz, %d-bit, %d channels, buffer %d ms (%d bytes)\n", 
                speed, bits, channels, buf_ms, s_sdl_buf_bytes);

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
    /* Return current play position in MONO SAMPLES (individual samples, not frames).
     * The DMA engine divides this by channels to get sample frames.
     * snd_audioDMA.samples is the total mono samples in the buffer.
     * 
     * The code in snd_dma.c does:
     *   snd_dmaSoundTime = buffers*fullSamples + samplePos/snd_audioDMA.channels;
     * where fullSamples = snd_audioDMA.samples / snd_audioDMA.channels
     * So samplePos must be in mono samples, range [0, snd_audioDMA.samples).
     */
    if (!snd_audioDMA.buffer || snd_audioDMA.samples == 0)
        return 0;
    
    int bytes_per_sample = snd_audioDMA.sampleBits / 8;
    if (bytes_per_sample <= 0)
        return 0;
    
    /* Convert byte position to mono samples */
    int sample_pos = s_sdl_play_pos / bytes_per_sample;
    
    /* Ensure it's within bounds */
    return sample_pos % snd_audioDMA.samples;
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

void SndImp_Shutdown (void)
{
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

void SndImp_Activate (qBool active)
{
    // Keep the engine's global active flag and AL state updated.
    Snd_Activate (active);

    // For the SDL DMA backend, pausing the device prevents needless callback churn.
    if (s_sdl_dev)
        SDL_PauseAudioDevice (s_sdl_dev, active ? 0 : 1);
}
