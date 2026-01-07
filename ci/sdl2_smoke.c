#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>

int main(void) {
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init(SDL_INIT_AUDIO) failed: %s\n", SDL_GetError());
        return 2;
    }

    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = NULL;

    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0) {
        fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 3;
    }

    printf("Opened audio device (freq=%d channels=%d)\n", have.freq, have.channels);
    SDL_CloseAudioDevice(dev);
    SDL_Quit();
    return 0;
}
