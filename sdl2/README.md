SDL2 backend (scaffold)

This directory contains a minimal SDL2 backend for EGL providing:
- `sdl_glimp.c` — SDL2-based GL context/window creation and swap
- `sdl_input.c` — SDL2 event polling and forwarding to engine input

How to build (Unix):
  make -f Makefile.unix USE_SDL2=1

Dependencies:
  - SDL2 development headers / libraries (e.g., libSDL2-dev on Debian/Ubuntu)

Notes / Next work:
  - Implement complete mode enumeration and fullscreen handling
  - Integrate sound backend using SDL2 audio (sdl_snd.c)
  - Add macOS build support and test
  - Add a runtime option to switch between SDL2 and native backends
