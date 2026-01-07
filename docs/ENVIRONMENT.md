# Development Environment (Windows, Linux, macOS)

This document explains how to prepare a development environment to build EGL on Windows (MSYS2/MinGW), Linux, and macOS.

## Quick smoke checks
- Windows (PowerShell / MSYS2 MinGW64):
  - Run: `.\tools\check_environment.ps1`
  - Then: `.\tools\build.ps1`

- Linux/macOS (bash):
  - Run: `./tools/check_environment.sh`
  - Then: `make -f Makefile.unix all`

---

## Windows (MSYS2 / MinGW-w64)

1. Install MSYS2: https://www.msys2.org/
2. Open `MSYS2 MinGW 64-bit` shell.
3. Update packages and install dependencies:

```
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-zlib mingw-w64-x86_64-pkg-config mingw-w64-x86_64-minizip
```

4. Ensure `C:\msys64\mingw64\bin` is in your PATH (PowerShell example):

```
$env:PATH = 'C:\msys64\mingw64\bin;' + $env:PATH
```

5. Run the helper script and build:

```
.\tools\check_environment.ps1
.\tools\build.ps1
```

Note: EGL includes a Windows Raw Input toggle cvar `in_rawinput` (default `1`). When enabled, the engine uses Windows Raw Input (`WM_INPUT`) for higher-precision mouse deltas while preserving legacy button events. To temporarily disable: `in_rawinput 0`.

## Linux (Debian/Ubuntu example)

Install required packages:

```
sudo apt update
sudo apt install build-essential libx11-dev libgl-dev libxrandr-dev libxi-dev zlib1g-dev libminizip-dev pkg-config
```

Note: `libxi-dev` (XInput2) provides raw input support; enable it via the `in_xinput2` cvar (default `1`).

SDL2 (optional): To build with the SDL2 backend, install `libsdl2-dev` (Debian/Ubuntu) or equivalent and build with `USE_SDL2=1`:

Audio (SDL2): The SDL2 backend provides audio output controlled by the following cvars:
- `s_sdl_device` (string) — optional device name (default: auto)
- `s_sdl_buffer_ms` (int) — desired audio buffer size in milliseconds (default: 200)

Testing audio with SDL2:
1. Ensure SDL2 dev libs are installed (`sudo apt install libsdl2-dev`).
2. Build with `make -f Makefile.unix USE_SDL2=1 all`.
3. Run EGL and verify sound plays; use `snd_restart` to reinitialize the audio subsystem if needed.
```
# Linux
sudo apt install libsdl2-dev
make -f Makefile.unix USE_SDL2=1 all

# Windows (MSYS2/MinGW)
# Install mingw-w64-x86_64-SDL2 and build with
make USE_SDL2=1 all
```

The SDL2 backend is scaffolded in `sdl2/` and provides `sdl_glimp.c`, `sdl_input.c`, and `sdl_snd.c` as a starting point.
Note: `libxrandr-dev` provides XRandR headers/libs required for modern display mode switching and gamma control.

VSync: EGL can use GLX swap control extensions to enable/disable v-sync. Use the following cvars at runtime:
- `gl_swap_control` (0/1) — enable or disable swap control (default 1)
- `gl_swap_interval` (integer) — interval to use when swap control is enabled (default 1)

If the GL driver supports `GLX_EXT_swap_control`, `GLX_SGI_swap_control`, or `GLX_MESA_swap_control`, EGL will detect it at runtime and attempt to set the requested interval during video initialization.
Then run:

```
./tools/check_environment.sh
make -f Makefile.unix all
```

## macOS (Homebrew)

Install Homebrew and packages:

```
brew install pkg-config gcc glfw glew zlib
```

Notes:
- macOS support is not implemented in the main tree yet; these instructions are preparatory.

---

If checks fail, follow the messages from the helper scripts and install the missing packages. If you want, I can add CI scripts and more detailed platform-specific guidance.
