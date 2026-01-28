# Audio Subsystem Map

Where **Snd_Init** chooses backends, what backends exist (file + init), how **mixing/buffering** works, and where **timing** comes from. Then a **modernization** plan that fits this architecture (no new libraries).

---

## 1. Where Snd_Init chooses backend(s)

**File:** `client/snd_main.c`  
**Function:** `Snd_Init()` (line 868)

**Flow:**

1. **Cvar `s_initSound`** (default: `"1"` Windows, `"2"` Unix) controls which path to try:
   - **`s_initSound == 2`:** Try **OpenAL** first via `ALSnd_Init()`. If it fails, fall through to DMA.
   - **`s_initSound == 1`:** Skip OpenAL; use **DMA** only.

2. **OpenAL path:**  
   `ALSnd_Init()` (`client/snd_openal.c` 901). On success, `snd_isAL = qTrue`. No `SndImp_*` / DMA used.

3. **DMA path:**  
   `DMASnd_Init()` (`client/snd_dma.c` 948) → `SndImp_Init()`. On success, `snd_isDMA = qTrue`.

4. **`SndImp_Init`** is **platform-specific**:
   - **Windows (no SDL2):** `win32/win_snd.c` `SndImp_Init()` (DirectSound).
   - **Unix (no SDL2):** `unix/unix_snd_main.c` `SndImp_Init()` → chooses ALSA / OSS / SDL via **`s_system`** cvar.
   - **SDL2 build (Win or Unix):** `sdl2/sdl_snd.c` `SndImp_Init()` → `Snd_SDL_Init()`. No `win_snd` / `unix_snd_main` in the link.

**Evidence:** `snd_main.c` 891–941 (`s_initSound`, `ALSnd_Init`, `DMASnd_Init`); `snd_dma.c` 948–959; `unix_snd_main.c` 76–124 (`s_system`, `OSS_Init`, `Snd_SDL_Init`); `win_snd.c` 285; `sdl_snd.c` 229–231. Makefile(s): SDL2 client uses `sdl_snd`, excludes `win_snd` / `unix_snd_main`.

---

## 2. Backends (file + init function)

| Backend | File | Init | Notes |
|--------|------|------|-------|
| **OpenAL** | `client/snd_openal.c` | `ALSnd_Init()` | 3D API; AL sources/buffers/listener. No DMA buffer, no `SndImp_*`. |
| **DMA / DirectSound** | `win32/win_snd.c` | `SndImp_Init()` | Win32 non-SDL2. Fills `snd_audioDMA`, DS secondary buffer. |
| **DMA / OSS** | `unix/unix_snd_oss.c` | `OSS_Init()` | Unix non-SDL2, via `unix_snd_main` when `s_system` = `"oss"`. mmap'd `/dev/dsp`. |
| **DMA / ALSA** | — | (FIXME in `unix_snd_main`) | `s_system` = `"alsa"`; not implemented. |
| **DMA / SDL2** | `sdl2/sdl_snd.c` | `SndImp_Init()` → `Snd_SDL_Init()` | USE_SDL2 build. Ring buffer + SDL callback. |

**Platform dispatch:**

- **Win:** `SndImp_*` = `win_snd` (non-SDL2) or `sdl_snd` (SDL2).
- **Unix:** `SndImp_*` = `unix_snd_main` (ALSA/OSS/SDL) or `sdl_snd` (SDL2). `unix_snd_main` delegates to `OSS_Init` / `Snd_SDL_Init` per `s_system`.

**Evidence:** `snd_openal.c` 898–901; `win_snd.c` 279–285; `unix_snd_oss.c` 47–50; `unix_snd_main.c` 73–124; `sdl_snd.c` 82, 229–231.

---

## 3. Mixing and buffering

### 3.1 OpenAL path

- No shared DMA buffer. **`ALSnd_Update(refDef_t *)`** (`snd_openal.c` 766) runs each frame: updates listener (position, velocity, orientation), gain, distance model, loop sounds, issues playsounds, spatializes channels.  
- **Evidence:** `snd_main.c` 1298–1306 (`Snd_Update` → `ALSnd_Update` when `snd_isAL`); `snd_openal.c` 766–885.

### 3.2 DMA path (shared layout)

**`audioDMA_t snd_audioDMA`** (`snd_local.h` 174–183, `snd_dma.c`):

- **`channels`**, **`sampleBits`**, **`speed`**: format.
- **`samples`**: mono sample count in the buffer (i.e. frames × channels). Power-of-two for wrap mask.
- **`submissionChunk`**: minimum mix block (e.g. 1024); `endTime` aligned to this.
- **`buffer`**: base of the DMA/ring buffer. Filled by mixer, consumed by backend (or device).
- **`samplePos`**: maintained by backend; used for **timing** (see below).

**Evidence:** `snd_local.h` 174–183; `snd_dma.c` 32, 119–132, 152–162, 430–445, 886–932.

### 3.3 DMA mix pipeline

**Entry:** `DMASnd_Update(refDef_t *)` (`snd_dma.c` 823). Called from `Snd_Update()` when `snd_isDMA`.

1. **View/origin:** If `rd`, copy `rd->viewOrigin` and `rd->rightVec` into `snd_dmaOrigin` / `snd_dmaRightVec` for spatialization; else clear.
2. **Focus:** If `cls.disableScreen` or `!snd_isActive`, `DMASnd_ClearBuffer()` and return.
3. **Volume:** If `s_volume->modified`, `DMASnd_ScaleTableInit()`.
4. **Spatialize:** Update L/R volumes for active channels via `DMASnd_SpatializeChannel`; clear fully muted channels.
5. **Loop sounds:** `DMASnd_AddLoopSounds()`.
6. **Timing (see §4):** `samplePos = SndImp_GetDMAPos()`, wrap detection,  
   `snd_dmaSoundTime = buffers * fullSamples + samplePos / channels`,  
   `endTime = snd_dmaSoundTime + s_mixahead * speed`, clamped and alignment-adjusted.
7. **Paint:** `SndImp_BeginPainting()` → `DMASnd_PaintChannels(endTime)` → `SndImp_Submit()`.

**`DMASnd_PaintChannels`** (636):

- **`snd_dmaPaintBuffer`**: stereo pairs, size `SND_PBUFFER` (2048). Per-block mix target.
- Loop over blocks until `snd_dmaPaintedTime >= endTime`:
  - **Issue playsounds** (`DMASnd_IssuePlaysounds`): assign pending `playSound_t` to channels when `beginTime <= snd_dmaPaintedTime`.
  - **Raw samples:** Copy from `snd_dmaRawSamples` ring into paint buffer (or clear).
  - **Paint channels:** For each active channel, resample from `sfxCache_t` into `snd_dmaPaintBuffer` (`DMASnd_PaintChannelFrom8` / `From16`), then **transfer** to `snd_audioDMA.buffer` via `DMASnd_TransferPaintBuffer` (stereo16 fast path or generic 8/16).
- Advance `snd_dmaPaintedTime` per block.

**Evidence:** `snd_dma.c` 823–933, 636–723, 369–417, 152–212, 430–445.

### 3.4 Backend buffer semantics

| Backend | `snd_audioDMA.buffer` | `SndImp_BeginPainting` / `Submit` | `SndImp_GetDMAPos` |
|--------|------------------------|-----------------------------------|---------------------|
| **Win32 DS** | Locked region of DS secondary buffer | Lock entire buffer; Unlock on Submit | DS `GetCurrentPosition` → mono sample offset |
| **OSS** | mmap'd device buffer | No-op (write directly) | `SNDCTL_DSP_GETOPTR` → `count.ptr` → mono samples |
| **SDL2** | Engine-owned ring buffer | `SDL_LockAudioDevice` / `Unlock` | Callback read position `s_sdl_play_pos` (bytes) → mono samples |

**Evidence:** `win_snd.c` 421–435, 445–505; `unix_snd_oss.c` 271–289, 298–310; OSS mmap 201–210; `sdl_snd.c` 51–81, 196–215, 217–227, 239–251.

---

## 4. Timing

### 4.1 DMA path (playback position)

**Source:** playback position from the backend, **not** `Sys_Milliseconds`.

- **`SndImp_GetDMAPos()`** returns current **mono sample** position in the buffer (hardware play cursor or callback read position).
- **Wrap:** `samplePos < oldSamplePos` → buffer wrapped; increment `buffers`.
- **`fullSamples`** = `snd_audioDMA.samples / snd_audioDMA.channels` (frames per buffer).
- **`snd_dmaSoundTime`** = `buffers * fullSamples + samplePos / channels` (in **stereo frames** for the stereo path).
- **Mix-ahead:** `endTime = snd_dmaSoundTime + s_mixahead->floatVal * snd_audioDMA.speed` (cvar `s_mixahead`, default 0.2 s). Mixing runs **ahead** of playback to avoid underruns.
- **Overflow:** If `snd_dmaPaintedTime < snd_dmaSoundTime`, we dropped samples; reset `snd_dmaPaintedTime = snd_dmaSoundTime`.

**Evidence:** `snd_dma.c` 895–921, 50–51, 124–132; `snd_main.c` 897 (`s_mixahead`).

### 4.2 OpenAL path

- Uses **`snd_audioAL.frameCount`** (incremented each `ALSnd_Update`), not DMA position.  
- **Evidence:** `snd_openal.c` 777, 849–856.

### 4.3 Play sound scheduling (DMA)

- **`beginTime`** on `playSound_t` is in the same **sample (frame) space** as `snd_dmaPaintedTime` / `snd_dmaSoundTime`.  
- **Evidence:** `snd_main.c` 1148–1166 (`beginTime` from `cl.frame.serverTime` and `snd_dmaPaintedTime`); `snd_dma.c` 379–381 (`DMASnd_IssuePlaysounds`).

---

## 5. Who calls what

| Caller | Function | When |
|--------|----------|------|
| `cl_main` init / `sdl_glimp` VID path | `Snd_Init()` | Startup; after vid_restart |
| `cl_main` frame | `Snd_Update(NULL)` | Menu/loading (no refDef) |
| `cg_view` | `cgi.Snd_Update(&cg.refDef)` | In-game |
| `snd_main` | `Snd_Update()` → `DMASnd_Update` or `ALSnd_Update` | Per-frame |
| `snd_dma` | `SndImp_BeginPainting` / `GetDMAPos` / `Submit` | During `DMASnd_Update` |
| `win_vid` (Win non-SDL2) / `sdl_input` (SDL2) | `SndImp_Activate(qBool)` | Focus gain/loss |

**Evidence:** `cl_main.c` 2249, 2261, 1472; `sdl_glimp.c` 314; `cg_view.c` 588; `snd_main.c` 1298–1306; `snd_dma.c` 888–932, 443–446; `win_vid.c` 137; `sdl_input.c` 774, 780.

---

## 6. Modernization (fits current architecture, no new libs)

### 6.1 Principles

- Keep **dual backend** (OpenAL vs DMA) and **`SndImp_*`** abstraction.
- No new audio libraries; extend **existing** backends and init flow.
- Prefer **SDL2** as the single DMA backend on platforms where `USE_SDL2` is used (already the case for builds that enable it).

### 6.2 Recommended changes

**1. Unify backend selection (reduce split between `s_initSound` and `s_system`)**

- **Now:** `s_initSound` (1 vs 2) picks AL vs DMA; on Unix non-SDL2, `s_system` picks OSS/ALSA/SDL inside DMA.
- **Change:** Keep `s_initSound` for “OpenAL vs DMA”. When DMA is used and `USE_SDL2` is defined, **always** use `sdl_snd` (no `s_system` for SDL2 client).  
- **Benefit:** Clear, single DMA implementation per build type; less branching.

**2. Prefer SDL2 for DMA when available**

- **Now:** SDL2 client **already** uses `sdl_snd` and skips `win_snd` / `unix_snd_main`.
- **Change:** Document this as the standard. Optionally add a **`HAVE_SDL2`** branch in `Snd_Init` (e.g. in `snd_main.c` around 1097) to **prefer** SDL2 DMA when both SDL2 and another backend exist (e.g. Unix build with SDL2 + OSS). Only switch to another DMA backend if SDL2 init fails.  
- **Integration:** Reuse existing `DMASnd_Init` → `SndImp_Init`; `SndImp_*` already implemented in `sdl_snd.c`. No new APIs.

**3. Centralize `SndImp_Activate`**

- **Now:** `SndImp_Activate` implemented in `win_snd` and `sdl_snd`; Unix OSS/ALSA path has no activate.
- **Change:** Add **`SndImp_Activate`** to `unix_snd_main` (and OSS path) as a small wrapper that calls `Snd_Activate(active)` only, matching SDL2’s non-DS logic. Wire it to focus events where Unix already handles them (or keep no-op if no focus handling).  
- **Benefit:** Same activate contract on all DMA backends; easier to add focus-based behavior later.

**4. Tunables (no structural change)**

- **`s_mixahead`:** Already configurable; consider documenting recommended range (e.g. 0.1–0.3 s) and default.  
- **SDL2:** `s_sdl_buffer_ms` already exists; document interaction with `s_mixahead` and underrun behavior.  
- **`submissionChunk`:** Keep 1024 or make it backend-specific (e.g. SDL2 could derive from buffer size) if you need to reduce latency further.

**5. Phase out or isolate legacy DMA backends**

- **Windows:** With `USE_SDL2=1`, `win_snd` is already unused. Keep it for non-SDL2 builds; add a short comment in `win_snd.c` that it is legacy when SDL2 is enabled.  
- **Unix:** Similarly, when `USE_SDL2=1`, `unix_snd_main` / OSS are unused. For non-SDL2 Unix, keep OSS; mark ALSA as “FIXME” until implemented.  
- **Benefit:** Clear “primary” vs “fallback” backends per platform.

**6. Avoid touching mixing core**

- **`DMASnd_PaintChannels`**, **`DMASnd_TransferPaintBuffer`**, **`DMASnd_Spatialize*`**, and **`snd_dmaSoundTime` / `snd_dmaPaintedTime`** timing are already correct and backend-agnostic.  
- **Recommendation:** Limit changes to init/backend selection, `SndImp_*` wiring, and cvars. Do **not** replace the mix pipeline or timing model.

### 6.3 Summary

| Area | Action |
|------|--------|
| Backend choice | Unify around `s_initSound`; when DMA + USE_SDL2, use `sdl_snd` only. Optionally prefer SDL2 in `Snd_Init` when available. |
| `SndImp_*` | Add `SndImp_Activate` to Unix DMA path; keep Win/SDL2 impls. |
| Mixing / timing | Leave as-is; timing from `SndImp_GetDMAPos` stays the source of truth. |
| Legacy backends | Keep for non-SDL2; document SDL2 as primary where enabled. |
| New libraries | None; stay within current AL + DMA / `SndImp_*` design. |

This keeps the existing architecture, clarifies backend roles, and gently modernizes init/activate and tuning without introducing new dependencies or changing the core mix/timing model.
