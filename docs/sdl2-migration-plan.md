# SDL2 Migration Plan (3 Phases)

This plan lists **platform entry points** (file + function) to replace with SDL2, and which **SDL2 equivalents** own them. For each area we cite **what currently implements it** and **what is already SDL2** when `USE_SDL2=1`.

**Builds:** Windows `Makefile` uses `USE_SDL2=1`; Unix `Makefile.unix` uses `USE_SDL2=1`.  
**Evidence:** `Makefile` 11–25, 47–52; `Makefile.unix` 57–70.

---

## Phase 1: Window + GL context

**Goal:** SDL2 is the single owner of window creation and OpenGL context. Remove platform-specific window/GL code from the client path.

### 1.1 Window + GL — already SDL2 (when `USE_SDL2=1`)

| Entry point | File | Function | SDL2 owner | SDL2 API |
|-------------|------|----------|------------|----------|
| Window + GL init | `sdl2/sdl_glimp.c` | `GLimp_Init()` | same | `SDL_Init(SDL_INIT_VIDEO\|EVENTS)`, `SDL_CreateWindow`, `SDL_GL_CreateContext` |
| Window + GL init | `sdl2/sdl_glimp.c` | `VID_Init()` | same | `VID_CheckChanges` → `GLimp_Init` (indirect), window cvars |
| Shutdown | `sdl2/sdl_glimp.c` | `GLimp_Shutdown()`, `VID_Shutdown()` | same | `SDL_GL_DeleteContext`, `SDL_DestroyWindow`, `SDL_Quit` |
| Swap / vsync | `sdl2/sdl_glimp.c` | `SDL_GLimp_SetSwapInterval` (internal) | same | `SDL_GL_SetSwapInterval` |
| Gamma | `sdl2/sdl_glimp.c` | `GLimp_GetGammaRamp`, `GLimp_SetGammaRamp` | same | `SDL_GetWindowGammaRamp`, `SDL_SetWindowGammaRamp` |

**Evidence:** `sdl2/sdl_glimp.c` 103–139 (`GLimp_Init`), 141–153 (`GLimp_Shutdown`), 336–361 (`VID_Init`, `VID_Shutdown`), 88–101 (swap), 364–393 (gamma).

### 1.2 Replaced (not used when `USE_SDL2=1`)

| Entry point | File | Function | Replaced by |
|-------------|------|----------|-------------|
| Window + GL init | `win32/win_glimp.c` | `GLimp_Init()` | `sdl2/sdl_glimp.c` `GLimp_Init` |
| Window + mode | `win32/win_vid.c` | `VID_Init()`, `VID_CheckChanges()`, `VID_Shutdown()` | `sdl2/sdl_glimp.c` `VID_Init`, `VID_Shutdown` |
| Window + GL | `unix/unix_glimp.c` | `VID_Init()`, `GLimp_Init()` | `sdl2/sdl_glimp.c` |
| Window + events | `unix/x11_main.c` | X11 window creation, `XCreateWindow`, etc. | SDL window in `sdl_glimp`; no X11 window in SDL2 build |

**Evidence:** `Makefile` 47–52: `USE_SDL2` → `SDL2_SOURCES` including `sdl_glimp.c`; `win32/*` excludes `win_vid.c`, `win_glimp.c`. `Makefile.unix` 57–70: `USE_SDL2` → `sdl2/sdl_glimp.c`; excludes `unix_glimp.c`, `x11_main.c`.

### 1.3 Phase 1 tasks

- **Windows:** Keep using `sdl_glimp` for client; ensure `win_vid` / `win_glimp` are never linked in SDL2 client build. **Already so** per Makefile.
- **Unix:** Ditto for `unix_glimp` / `x11_main` window path. **Already so** per Makefile.unix.
- **Optional:** Remove or guard `win_vid` / `win_glimp` / `unix_glimp` / `x11` window code so they are clearly dead in SDL2 builds; reduce `#ifdef HAVE_SDL2` in `win_vid` (e.g. 381–406) to a single “SDL2 build” guard if both branches stay.

---

## Phase 2: Input events + Audio init

**Goal:** SDL2 owns all input (keyboard, mouse, gamepad) and audio backend init. Platform-specific input and sound init are dropped from the client path.

### 2.1 Input — already SDL2 (when `USE_SDL2=1`)

| Entry point | File | Function | SDL2 owner | SDL2 API |
|-------------|------|----------|------------|----------|
| Input init | `sdl2/sdl_input.c` | `IN_Init()` | same | `SDL_InitSubSystem(GAMECONTROLLER\|JOYSTICK\|HAPTIC)`, cvars |
| Input per-frame | `sdl2/sdl_input.c` | `IN_Frame()` | same | `SDL2_PollInputEvents()` → `SDL_PollEvent` |
| Input shutdown | `sdl2/sdl_input.c` | `IN_Shutdown()` | same | `SDL2_CloseController`, cmd remove |
| Key state | `sdl2/sdl_input.c` | `In_GetKeyState()` | same | `SDL_GetModState` |
| Mouse grab | `sdl2/sdl_input.c` | `SDL2_SetMouseGrab()` (used by `sdl_glimp`) | same | `SDL_SetRelativeMouseMode`, `SDL_SetWindowGrab`, `SDL_ShowCursor` |

**Evidence:** `sdl2/sdl_input.c` 893 (`IN_Init`), 866 (`IN_Frame`), 933 (`IN_Shutdown`), 612–891 (`SDL2_PollInputEvents`), 989 (`In_GetKeyState`). `client/cl_main.c` 1455 calls `IN_Frame`; `cl_cgapi.c` 787 passes `Sys_SendKeyEvents` to cgame.

### 2.2 Input — replaced (not used when `USE_SDL2=1`)

| Entry point | File | Function | Replaced by |
|-------------|------|----------|-------------|
| Input init | `win32/win_input.c` | `IN_Init()` | `sdl2/sdl_input.c` `IN_Init` |
| Input per-frame | `win32/win_input.c` | `IN_Frame()` | `sdl2/sdl_input.c` `IN_Frame` |
| Input init | `unix/x11_main.c` | `IN_Init()` | `sdl2/sdl_input.c` `IN_Init` |
| Input per-frame | `unix/x11_main.c` | `IN_Frame()` | `sdl2/sdl_input.c` `IN_Frame` |

**Evidence:** `Makefile` 49: SDL2 build omits `win_input`; `Makefile.unix` 61–62: SDL2 build uses `sdl_input`, omits `x11_main`.

### 2.3 Audio init — already SDL2 (when `USE_SDL2=1`)

| Entry point | File | Function | SDL2 owner | SDL2 API |
|-------------|------|----------|------------|----------|
| DMA backend init | `sdl2/sdl_snd.c` | `SndImp_Init()` → `Snd_SDL_Init()` | same | `SDL_InitSubSystem(SDL_INIT_AUDIO)`, `SDL_OpenAudioDevice` |
| DMA shutdown | `sdl2/sdl_snd.c` | `SndImp_Shutdown()` → `Snd_SDL_Shutdown()` | same | `SDL_CloseAudioDevice` |
| DMA position | `sdl2/sdl_snd.c` | `SndImp_GetDMAPos()` → `Snd_SDL_GetDMAPos()` | same | ring buffer |
| Paint lock | `sdl2/sdl_snd.c` | `SndImp_BeginPainting` / `SndImp_Submit` | same | `SDL_LockAudioDevice` / `Unlock` |

**Evidence:** `sdl2/sdl_snd.c` 82–169 (`Snd_SDL_Init`), 229–262 (`SndImp_*`). `client/snd_dma.c` 948–959 (`DMASnd_Init` calls `SndImp_Init`).

### 2.4 Audio init — replaced (not used when `USE_SDL2=1`)

| Entry point | File | Function | Replaced by |
|-------------|------|----------|-------------|
| DMA backend | `win32/win_snd.c` | `SndImp_Init()` (DirectSound) | `sdl2/sdl_snd.c` `SndImp_Init` |
| DMA backend | `unix/unix_snd_main.c` | `SndImp_Init()` (ALSA/OSS/SDL) | `sdl2/sdl_snd.c` `SndImp_Init` |

**Evidence:** `Makefile` 49: SDL2 client uses `win_snd_cd.c` only (CD), not `win_snd.c`; `sdl_snd` provides `SndImp_*`. `Makefile.unix` 61–62: SDL2 uses `sdl_snd`, omits `unix_snd_main` (and other `unix_snd_*`).

### 2.5 Phase 2 tasks

- **Input:** Confirm `IN_Frame` → `SDL2_PollInputEvents` is the sole event pump in SDL2 client builds. Unix `Sys_SendKeyEvents` (`unix_main.c` 412) only does `CL_UpdateFrameTime`; no X11 pump. **Already correct** for SDL2.
- **Audio:** Ensure `Snd_Init` (snd_main) → `DMASnd_Init` → `SndImp_Init` uses `sdl_snd` in SDL2 builds. **Already so** via build selection of `sdl_snd` and exclusion of `win_snd` / `unix_snd_main`.
- **Optional:** Add `HAVE_SDL2` branch in `client/snd_main.c` (see 1097) if you want to prefer SDL2 audio when available, or doc the current DMA → `SndImp_*` flow.

---

## Phase 3: Timing (+ system init); Net unchanged

**Goal:** Use SDL2 for timing where the SDL2 client runs; centralize SDL init. **Net stays platform-specific** (no SDL_Net).

### 3.1 Timing — not SDL2 today

| Entry point | File | Function | Current implementation |
|-------------|------|----------|-------------------------|
| Millisecond timer | `win32/win_main.c` | `Sys_Milliseconds()` | `timeGetTime()` |
| Millisecond timer | `unix/unix_main.c` | `Sys_Milliseconds()` | `gettimeofday` → ms |

**Evidence:** `win32/win_main.c` 313–316; `unix/unix_main.c` 344–347. `common/common.c` uses `Sys_Milliseconds` indirectly; `client/cl_main.c` 1384, 2255; `sdl2/sdl_input.c` uses `Sys_Milliseconds` in `Key_Event` calls (e.g. 640, 661, 673).

### 3.2 Timing — SDL2 equivalent (to own it)

| Entry point | File | Function | SDL2 owner | SDL2 API |
|-------------|------|----------|------------|----------|
| Millisecond timer | **new or `sdl2/sdl_main.c`** | `Sys_Milliseconds()` | SDL2 | `SDL_GetTicks()` |

**Proposal:** Add `sdl2/sdl_time.c` (or extend `sdl_main.c`) with `Sys_Milliseconds` implemented as `SDL_GetTicks()`. When `USE_SDL2=1`, link that instead of platform `Sys_Milliseconds`. Ensure `SDL_Init` has been called before any timer use (e.g. after `SDL_Init` in `GLimp_Init` or a shared init path).

### 3.3 System init — SDL2 hooks (currently unused)

| Entry point | File | Function | Notes |
|-------------|------|----------|-------|
| SDL init | `sdl2/sdl_main.c` | `SDL2_InitHook()` | `SDL_Init(VIDEO\|EVENTS\|AUDIO)`. **Not called anywhere.** |
| SDL shutdown | `sdl2/sdl_main.c` | `SDL2_ShutdownHook()` | `SDL_Quit`. **Not called anywhere.** |

**Evidence:** `sdl2/sdl_main.c` 21–35; `grep SDL2_InitHook SDL2_ShutdownHook` shows only definitions.

**Proposal:** Call `SDL2_InitHook` from a single platform-agnostic init path (e.g. early in `Com_Init` when `USE_SDL2`) and `SDL2_ShutdownHook` in shutdown. Today, `SDL_Init` is done inside `GLimp_Init` (`sdl_glimp.c` 104); you could either keep that or move it to `SDL2_InitHook` and have `GLimp_Init` depend on SDL being inited.

### 3.4 Net — unchanged

| Entry point | File | Function | Owner | Notes |
|-------------|------|----------|--------|------|
| UDP init | `win32/win_sock.c` | `NET_Init()` | Win32 | Kept |
| UDP send/recv | `win32/win_sock.c` | `NET_SendPacket()`, `NET_GetPacket()` | Win32 | Kept |
| UDP init | `unix/unix_udp.c` | `NET_Init()` | Unix | Kept |
| UDP send/recv | `unix/unix_udp.c` | `NET_SendPacket()`, `NET_GetPacket()` | Unix | Kept |

**Evidence:** `common/common.c` 806 (`NET_Init`); `common/net_chan.c` uses `NET_SendPacket`; `client/cl_main.c` 1271, `server/sv_main.c` 767 use `NET_GetPacket`. No SDL_Net in tree.

**Conclusion:** Net stays as-is. No SDL2 replacement.

### 3.5 Phase 3 tasks

1. **Timing:** Implement `Sys_Milliseconds` via `SDL_GetTicks` in `sdl2/` and use it in SDL2 client builds; switch platform code to that impl only when `USE_SDL2`.
2. **System init:** Wire `SDL2_InitHook` / `SDL2_ShutdownHook` into startup/shutdown, and ensure `SDL_Init` runs before any SDL usage (including timing).
3. **Net:** No change; keep `win_sock` / `unix_udp`.

---

## Summary

| Area | Currently platform | Already SDL2 when `USE_SDL2`? | Phase | SDL2 owner |
|------|--------------------|-------------------------------|-------|------------|
| Window + GL | `win_vid` / `win_glimp`, `unix_glimp` / `x11` | Yes | 1 | `sdl2/sdl_glimp.c` |
| Input | `win_input`, `x11_main` | Yes | 2 | `sdl2/sdl_input.c` |
| Audio init | `win_snd`, `unix_snd_main` | Yes | 2 | `sdl2/sdl_snd.c` |
| Timing | `win_main`, `unix_main` | No | 3 | New `sdl2` timing (e.g. `SDL_GetTicks`) |
| System init | — | Hooks exist, unused | 3 | `sdl2/sdl_main.c` (`SDL2_InitHook` / `ShutdownHook`) |
| Net | `win_sock`, `unix_udp` | No, unchanged | — | Remain platform |

---

## Quick reference: who calls what

- **`VID_Init`** / **`GLimp_Init`**: `client/cl_main.c` 2249; `renderer/rf_init.c` 1397 (`GLimp_Init`). With SDL2 → `sdl_glimp`.
- **`IN_Init`** / **`IN_Frame`**: `client/cl_main.c` 2261, 1455. With SDL2 → `sdl_input`.
- **`Snd_Init`**: `client/snd_main.c` 868; called from `cl_main` shutdown path and from `sdl_glimp` VID path (314). Uses `DMASnd_Init` → `SndImp_Init`. With SDL2 → `sdl_snd`.
- **`Sys_Milliseconds`**: Used by `cl_main`, `net_chan`, `cl_parse`, `sdl_input`, etc. Stays in `win_main` / `unix_main` until Phase 3.
- **`NET_Init`** / **`NET_SendPacket`** / **`NET_GetPacket`**: `common/common.c`, `net_chan.c`, `cl_main.c`, `sv_main.c`. Always platform.
