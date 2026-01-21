# EGL, an enhanced Quake II Engine
Originally written by Echon.
A backup of EGL 0.3.2, modernized from the original 0.3.1 QuakeSrc snapshot (circa 2007).

## Modernization status

**January 2026:** this repo is being actively modernized.
The short-term focus is keeping builds working on Linux + Windows (MSYS2/MinGW64), especially via the SDL2 backend.
Expect the build system and platform backends to continue evolving.

## Notes (renderer + platform fixes)

- `gl_coloredlightmaps`: older EGL builds could have this cvar set via configs, but it was not actually implemented in the renderer. This repo now implements it for Quake II BSP lightmaps.
- SDL2 + `vid_restart`: the SDL2 backend now restores mouse grab/relative mode and hides the OS cursor after a restart.

## Running
EGL is an engine; it expects Quake II base game data to be present.

- Put your legally-obtained Quake II `pak0.pak` into `baseq2/` (and any other `pak*.pak` you have).
- Keep `egl.pkz` in `baseq2/` as well.

Without `pak0.pak`, you'll see missing media warnings (e.g. `sound/misc/menu1.wav`, `pics/conchars.*`).

## Download (release binaries)

On the GitHub Releases page, download the ZIP for your OS and extract it into your Quake II folder.
Then copy your legally-obtained `pak0.pak` into the extracted `baseq2/` directory.

The release packages include the engine binaries, the `baseq2` game modules, and `egl.pkz`.
They do not include Quake II game data.

## Building on Windows (PowerShell + MSYS2)

If you run `make` from PowerShell and see:

`make (e=2): The system cannot find the file specified.`

it usually means `gcc` isn't on your `PATH`.

Use the helper:

`powershell -NoProfile -ExecutionPolicy Bypass -File tools\build.ps1 -j`

SDL2 backend build:

`powershell -NoProfile -ExecutionPolicy Bypass -File tools\build.ps1 -j USE_SDL2=1`

## Packaging a portable folder (client + dedicated + data)

To stage an installer-like folder layout (portable build) under `out\egl\`:

`powershell -NoProfile -ExecutionPolicy Bypass -File tools\package.ps1`

This will:

- Build `egl.exe` and the x64 game modules (`baseq2\gamex64.dll`, `baseq2\eglcgamex64.dll`)
- Build `egl-dedicated.exe`
- Copy `data\egl.pkz` into `out\egl\baseq2\egl.pkz`

You still need to provide your legally-obtained Quake II data (at minimum `pak0.pak`) in `out\egl\baseq2\`.

## CI

GitHub Actions runs an SDL2 build on Linux and Windows (MSYS2/MinGW64) in `.github/workflows/ci-sdl2.yml`.
The workflow also compiles a small Vulkan smoke test (compile/link on Linux; compile-only on Windows).

## Building on Linux (Ubuntu / openSUSE)

This repo includes an X11/GLX Unix platform layer under `unix/`, but the default `Makefile` is Windows-focused.
For Linux builds, use `Makefile.unix`.

Typical dependencies:

- A C compiler + make (`build-essential` on Ubuntu)
- X11 + OpenGL development headers (`libx11-dev`, `libgl1-mesa-dev`)
- zlib + minizip development packages (`zlib1g-dev`, `libminizip-dev`)

Build:

`make -f Makefile.unix -j all`

This produces:

- `./egl` (client)
- `./baseq2/game.so` and `./baseq2/eglcgame.so` (modules)

Dedicated server:

`make -f Makefile.unix -j dedicated`

### Linux portable folder output

To stage a portable folder under `out/egl-linux/`:

`bash tools/package.sh`

Then copy your Quake II `pak0.pak` into `out/egl-linux/baseq2/`.

## macOS status

There is no native macOS platform layer in this repo right now (the Unix code here targets X11/GLX).
To support modern macOS, you’ll likely want an SDL2 (or Cocoa) window/input layer and an OpenGL (or Metal) renderer backend.
Linux can also benefit from SDL2 (Wayland) long-term, but the current `Makefile.unix` targets X11.

## Effects assets (modern overrides)

EGL’s particle effects are largely driven by textures under `egl/parts/*.tga`.
You can override these without code changes by placing a higher-priority `.pkz` in `baseq2/addons/`.

See [docs/effects-assets.md](docs/effects-assets.md).
