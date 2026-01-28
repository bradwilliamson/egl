# Audio Timing Trace: snd_dmaSoundTime Computation

Tracing `snd_dmaSoundTime` and the variables `fullSamples`, `samplePos`, and `channels` for each backend. Shows what `SndImp_GetDMAPos` returns (units) and exact expressions where `samplePos` is computed and consumed.

---

## Summary: Units Flow

| Backend | `SndImp_GetDMAPos` returns | Units | Conversion to `samplePos` |
|---------|---------------------------|-------|---------------------------|
| **Win32 DS** | Mono samples | `[0, snd_audioDMA.samples)` | Direct (already mono samples) |
| **OSS** | Mono samples | `[0, snd_audioDMA.samples)` | Direct (stored in `snd_audioDMA.samplePos`) |
| **SDL2** | Mono samples | `[0, snd_audioDMA.samples)` | Direct (converted from bytes) |

**Final computation:** `snd_dmaSoundTime = buffers * fullSamples + samplePos / channels`  
- **`fullSamples`**: frames per buffer = `snd_audioDMA.samples / snd_audioDMA.channels`  
- **`samplePos`**: mono samples (from `SndImp_GetDMAPos`)  
- **`samplePos / channels`**: converts mono samples → frames  
- **`snd_dmaSoundTime`**: **frames** (stereo frames for stereo, mono frames for mono)

---

## 1. Win32 DirectSound (`win32/win_snd.c`)

### 1.1 `SndImp_GetDMAPos` implementation

**File:** `win32/win_snd.c`  
**Function:** `SndImp_GetDMAPos()` (line 421)

**Expression:**
```c
int SndImp_GetDMAPos (void)
{
    int     s;
    MMTIME  mmtime;
    DWORD   dwWrite;

    if (!snd_win.initialized)
        return 0;

    mmtime.wType = TIME_SAMPLES;
    snd_win.pDSBuf->lpVtbl->GetCurrentPosition (snd_win.pDSBuf, &mmtime.u.sample, &dwWrite);
    s = ((mmtime.u.sample - snd_win.mmStartTime.u.sample) >> snd_win.sample16) & (snd_audioDMA.samples-1);

    return s;
}
```

**Units:**
- **`mmtime.u.sample`**: DirectSound `GetCurrentPosition` with `TIME_SAMPLES` returns **bytes** (playback position in bytes).
- **`snd_win.mmStartTime.u.sample`**: Initial position in **bytes** (captured at buffer start, line 225).
- **`snd_win.sample16`**: `(snd_audioDMA.sampleBits/8) - 1` (line 232). For 16-bit: `(16/8) - 1 = 1`.
- **`>> snd_win.sample16`**: Right-shift by 1 (for 16-bit) converts **bytes → samples** (divide by 2).
- **`& (snd_audioDMA.samples-1)`**: Wrap mask (power-of-two buffer).

**Returns:** **Mono samples** (range `[0, snd_audioDMA.samples)`).

**Evidence:** `win_snd.c` 421–435, 225 (mmStartTime capture), 232 (sample16 = (sampleBits/8)-1).

---

## 2. OSS (`unix/unix_snd_oss.c`)

### 2.1 `OSS_GetDMAPos` implementation

**File:** `unix/unix_snd_oss.c`  
**Function:** `OSS_GetDMAPos()` (line 270)

**Expression:**
```c
int OSS_GetDMAPos (void)
{
    struct count_info count;

    if (!oss_initialized)
        return 0;

    if (ioctl (oss_audioFD, SNDCTL_DSP_GETOPTR, &count) == -1) {
        perror (oss_curDevice);
        Com_Printf (PRNT_ERROR, "SndImp_GetDMAPos: Uh, sound dead.\n");
        close (oss_audioFD);
        oss_initialized = qFalse;
        return 0;
    }

    snd_audioDMA.samplePos = count.ptr / (snd_audioDMA.sampleBits / 8);

    return snd_audioDMA.samplePos;
}
```

**Units:**
- **`count.ptr`**: OSS `SNDCTL_DSP_GETOPTR` returns the current write pointer in **bytes** (offset into the mmap'd buffer).
- **`snd_audioDMA.sampleBits / 8`**: Bytes per sample (e.g., 2 for 16-bit).
- **`count.ptr / (snd_audioDMA.sampleBits / 8)`**: Converts **bytes → samples**.

**Returns:** **Mono samples** (stored in `snd_audioDMA.samplePos`, range `[0, snd_audioDMA.samples)`).

**Evidence:** `unix_snd_oss.c` 270–288. OSS `count_info.ptr` is documented as byte offset; division by `sampleBits/8` converts to samples.

---

## 3. SDL2 (`sdl2/sdl_snd.c`)

### 3.1 `Snd_SDL_GetDMAPos` implementation

**File:** `sdl2/sdl_snd.c`  
**Function:** `Snd_SDL_GetDMAPos()` (line 196)

**Expression:**
```c
int Snd_SDL_GetDMAPos (void)
{
    /*
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
```

**Units:**
- **`s_sdl_play_pos`**: **Bytes** (comment at line 32: "byte offset being played (read by callback)"). Updated by `SDLAudioCallback` (line 79) as it reads from the ring buffer.
- **`bytes_per_sample`**: `snd_audioDMA.sampleBits / 8` (e.g., 2 for 16-bit).
- **`s_sdl_play_pos / bytes_per_sample`**: Converts **bytes → mono samples**.
- **`sample_pos % snd_audioDMA.samples`**: Wrap to buffer size (power-of-two mask).

**Returns:** **Mono samples** (range `[0, snd_audioDMA.samples)`).

**Evidence:** `sdl_snd.c` 196–215, 32 (s_sdl_play_pos comment), 51–80 (SDLAudioCallback updates s_sdl_play_pos in bytes).

---

## 4. Consumption in `snd_dma.c`

### 4.1 Where `samplePos` is computed

**File:** `client/snd_dma.c`  
**Function:** `DMASnd_Update()` (line 823)

**Expression (line 901):**
```c
samplePos = SndImp_GetDMAPos ();
```

**Units:** `samplePos` is in **mono samples** (from all backends).

**Evidence:** `snd_dma.c` 901.

---

### 4.2 Where `fullSamples` is computed

**File:** `client/snd_dma.c`  
**Function:** `DMASnd_Update()` (line 823)

**Expression (line 895):**
```c
fullSamples = snd_audioDMA.samples / snd_audioDMA.channels;
```

**Units:** **Frames per buffer** (stereo frames for stereo, mono frames for mono).

**Explanation:**
- **`snd_audioDMA.samples`**: Total **mono samples** in the buffer (frames × channels).
- **`snd_audioDMA.channels`**: 1 (mono) or 2 (stereo).
- **`fullSamples`**: Frames per buffer wrap (e.g., for stereo: `samples/2`).

**Evidence:** `snd_dma.c` 895.

---

### 4.3 Where `snd_dmaSoundTime` is computed

**File:** `client/snd_dma.c`  
**Function:** `DMASnd_Update()` (line 823)

**Expression (line 914):**
```c
snd_dmaSoundTime = buffers*fullSamples + samplePos/snd_audioDMA.channels;
```

**Units:** **Frames** (stereo frames for stereo, mono frames for mono).

**Breakdown:**
- **`buffers`**: Static counter (line 830), incremented on wrap (line 903). Number of buffer wraps since init.
- **`buffers * fullSamples`**: Total frames from completed buffer wraps.
- **`samplePos`**: Current position in **mono samples** (from `SndImp_GetDMAPos`).
- **`samplePos / snd_audioDMA.channels`**: Converts **mono samples → frames** (divide by 2 for stereo, by 1 for mono).
- **`snd_dmaSoundTime`**: Total frames played since init (absolute time in frame space).

**Evidence:** `snd_dma.c` 914, 50 (comment: "sample PAIRS" for stereo), 830 (buffers static), 902–903 (wrap detection).

---

### 4.4 Where `snd_dmaSoundTime` is consumed

**File:** `client/snd_dma.c`  
**Function:** `DMASnd_Update()` (line 823)

**Expressions:**

1. **Overflow check (line 917):**
   ```c
   if (snd_dmaPaintedTime < snd_dmaSoundTime) {
       Com_DevPrintf (PRNT_WARNING, "Snd_Update: overflow\n");
       snd_dmaPaintedTime = snd_dmaSoundTime;
   }
   ```
   **Units:** Both in **frames**. If mixer fell behind playback, resync.

2. **Mix-ahead calculation (line 923):**
   ```c
   endTime = snd_dmaSoundTime + s_mixahead->floatVal * snd_audioDMA.speed;
   ```
   **Units:** `snd_dmaSoundTime` in **frames**, `s_mixahead * speed` in **samples/sec × seconds = samples**, but `snd_audioDMA.speed` is samples/sec (mono), so this adds **mono samples**. Wait—this is inconsistent. Let me check...

   Actually, `s_mixahead` is in **seconds** (default 0.2), and `snd_audioDMA.speed` is **samples/sec** (mono). So `s_mixahead * speed` = **mono samples**. But `snd_dmaSoundTime` is in **frames**. This means the addition is mixing units.

   Correction: Looking at line 923, `endTime` is used in `DMASnd_PaintChannels(endTime)` which expects **frames** (see line 129: `lpos = lpaintedtime & ((snd_audioDMA.samples>>1)-1)` where `lpaintedtime` is in frames for stereo). So `endTime` must be in **frames**.

   The expression `snd_dmaSoundTime + s_mixahead * speed` adds:
   - `snd_dmaSoundTime`: **frames**
   - `s_mixahead * speed`: **mono samples** = `(seconds) * (mono samples/sec)`

   For stereo: `speed` is mono samples/sec, so `s_mixahead * speed` = mono samples. To convert to frames: `(s_mixahead * speed) / channels`. But the code doesn't divide by channels here.

   Wait, let me re-check. `snd_audioDMA.speed` is the sample rate in **samples/sec** (mono). For stereo at 48kHz, `speed = 48000` (mono samples/sec). `s_mixahead = 0.2` seconds. So `0.2 * 48000 = 9600` **mono samples**. For stereo, that's `9600 / 2 = 4800` **frames**.

   But the code does `snd_dmaSoundTime + s_mixahead * speed` directly. If `snd_dmaSoundTime` is in frames and `s_mixahead * speed` is in mono samples, this is wrong unless...

   Actually, I think the code treats `snd_dmaSoundTime` as if it were in **mono samples** in this context, or there's an implicit conversion. Let me check `DMASnd_PaintChannels`.

   Looking at line 129: `lpos = lpaintedtime & ((snd_audioDMA.samples>>1)-1)`, `lpaintedtime` is compared to `endTime` (line 127: `while (lpaintedtime < endTime)`), and `lpaintedtime` starts as `snd_dmaPaintedTime` (line 125), which is in **frames** (comment line 51: "sample PAIRS").

   So `endTime` must be in **frames**. But `snd_dmaSoundTime` is computed as frames (line 914), and `s_mixahead * speed` is mono samples. The addition is wrong unless `speed` is interpreted differently.

   Correction: I need to check if `snd_audioDMA.speed` is actually frames/sec or samples/sec. Looking at init code:
   - Win32: `snd_audioDMA.speed = 48000` (line 298–307 in win_snd.c) — this is samples/sec (mono).
   - OSS: `snd_audioDMA.speed = 48000` (line 127 in unix_snd_oss.c) — samples/sec.
   - SDL2: `snd_audioDMA.speed = speed` where `speed = 48000` (line 111 in sdl_snd.c) — samples/sec.

   So `speed` is **mono samples/sec**. For stereo, frames/sec = `speed / channels = 48000 / 2 = 24000` frames/sec.

   The expression `snd_dmaSoundTime + s_mixahead * speed` adds frames + (seconds × mono samples/sec) = frames + mono samples. This is a unit mismatch.

   **Actual behavior:** The code works because `snd_dmaSoundTime` is stored/compared in a way that treats it as if it were in mono samples in some contexts, or there's an implicit conversion. But the comment says "sample PAIRS" (frames for stereo).

   **Most likely:** The code has a subtle unit inconsistency. `snd_dmaSoundTime` is computed in frames (line 914), but when used with `s_mixahead * speed` (mono samples), it's treated as if both are in the same unit. This works numerically if `channels = 1` (mono), but for stereo it's off by a factor of 2.

   **Evidence:** `snd_dma.c` 923, 50 (comment), 129 (lpaintedtime in frames), 125 (lpaintedtime = snd_dmaPaintedTime).

3. **Clamp check (line 928):**
   ```c
   if (endTime - snd_dmaSoundTime > samples)
       endTime = snd_dmaSoundTime + samples;
   ```
   **Units:** Both in **frames** (or treated as such). `samples` is `snd_audioDMA.samples >> (channels-1)` = frames per buffer (line 927).

**Evidence:** `snd_dma.c` 917–920, 923, 927–929.

---

## 5. Summary Table

| Variable | Units | Where computed | Expression |
|----------|-------|----------------|------------|
| **`SndImp_GetDMAPos()` (Win32)** | Mono samples | `win_snd.c:432` | `((mmtime.u.sample - mmStartTime.u.sample) >> sample16) & (samples-1)` |
| **`SndImp_GetDMAPos()` (OSS)** | Mono samples | `unix_snd_oss.c:285` | `count.ptr / (sampleBits/8)` |
| **`SndImp_GetDMAPos()` (SDL2)** | Mono samples | `sdl_snd.c:211` | `(s_sdl_play_pos / bytes_per_sample) % samples` |
| **`samplePos`** | Mono samples | `snd_dma.c:901` | `SndImp_GetDMAPos()` |
| **`fullSamples`** | Frames | `snd_dma.c:895` | `snd_audioDMA.samples / snd_audioDMA.channels` |
| **`snd_dmaSoundTime`** | Frames | `snd_dma.c:914` | `buffers * fullSamples + samplePos / channels` |
| **`endTime`** | Frames (intended) | `snd_dma.c:923` | `snd_dmaSoundTime + s_mixahead * speed` (unit mismatch: frames + mono samples) |

---

## 6. Potential Issue: Unit Mismatch in Mix-Ahead

**Line 923:** `endTime = snd_dmaSoundTime + s_mixahead->floatVal * snd_audioDMA.speed;`

- **`snd_dmaSoundTime`**: **Frames** (from line 914).
- **`s_mixahead * speed`**: **Mono samples** (seconds × mono samples/sec).

**For stereo:** This adds frames + mono samples, which is incorrect. Should be:
```c
endTime = snd_dmaSoundTime + (s_mixahead->floatVal * snd_audioDMA.speed) / snd_audioDMA.channels;
```

**For mono:** Works correctly (frames = mono samples when channels = 1).

**Evidence:** `snd_dma.c` 923, 914, 50 (comment: "sample PAIRS" for stereo), 129 (lpaintedtime in frames).

**Note:** This may be a latent bug that only manifests under certain conditions, or the code may rely on implicit conversions elsewhere. The comment at line 50 says `snd_dmaSoundTime` is in "sample PAIRS" (frames for stereo), confirming the intended unit.
