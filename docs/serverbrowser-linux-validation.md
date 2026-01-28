# Server Browser Linux Validation

Analysis of whether the client in-game online server browser will work on Linux and whether Docker tests validate it.

---

## Summary

**Status:** ✅ **Should work on Linux**, but ⚠️ **Docker tests do NOT validate HTTP fetching**.

---

## 1. Linux Compatibility

### 1.1 HTTP Fetching Implementation

**File:** `client/cl_httpfetch.c`

- **Windows:** Uses WinHTTP API (`#ifdef _WIN32`, lines 26-252)
- **Linux/Unix:** Uses libcurl (`#ifndef _WIN32`, lines 254-378)

**Linux implementation details:**
- Uses `curl_easy_init()`, `curl_easy_perform()`, etc. (standard libcurl API)
- Handles redirects (up to 5), timeouts (4s connect, 8s total)
- Follows same interface as Windows version
- **Evidence:** `cl_httpfetch.c` 254-378 (`CL_HTTPFetch_Curl`), 380-394 (platform dispatch)

### 1.2 Build Configuration

**File:** `Makefile.unix`

- **Line 25-33:** Links against libcurl:
  ```makefile
  CURL_CFLAGS := $(shell pkg-config --cflags libcurl 2>/dev/null)
  CURL_LIBS := $(shell pkg-config --libs libcurl 2>/dev/null)
  LDFLAGS_CLIENT += $(CURL_LIBS)
  ```
- **Line 50:** `CLIENT_SRC = $(wildcard client/*.c)` includes `cl_httpfetch.c`
- **Evidence:** `Makefile.unix` 25-33, 50

### 1.3 Server Browser Usage

**File:** `client/cl_serverbrowser.c`

- **Line 13:** `#include "cl_httpfetch.h"`
- **Line 155-184:** `SB_QueryHTTPMaster()` calls `CL_HTTPFetch()` for HTTP master servers
- **Line 392-394:** Detects HTTP masters via `SB_IsHTTPMaster()` (checks for `http://` or `https://` prefix)
- **Evidence:** `cl_serverbrowser.c` 13, 155-184, 392-394

**Conclusion:** The server browser **should work on Linux** because:
1. ✅ Linux HTTP implementation exists (libcurl)
2. ✅ Build system links against libcurl
3. ✅ Server browser code is platform-agnostic (uses `CL_HTTPFetch` abstraction)

---

## 2. Docker Test Validation

### 2.1 What Docker Tests Install

**Files:** `tools/docker-test-linux.sh`, `tools/docker-test-linux.ps1`

**Line 35 (shell) / 13 (PowerShell):**
```bash
apt-get install -y build-essential pkg-config ca-certificates libcurl4-openssl-dev
```

- ✅ Installs `libcurl4-openssl-dev` (libcurl development package)
- ✅ Installs `ca-certificates` (for HTTPS)
- **Evidence:** `docker-test-linux.sh` 35, `docker-test-linux.ps1` 13

### 2.2 What Docker Tests Actually Test

**File:** `tests/Makefile`

**Test binaries:**
- **Line 58:** `test_serverbrowser_ui` — tests UI layout
- **Line 59:** `test_serverbrowser_parse` — tests parsing logic (text/binary server lists)

**What `test_serverbrowser_parse` tests:**
- Parsing text server lists (lines like `ip:port` or `quake2://ip:port`)
- Parsing binary server lists (6-byte entries: 4-byte IP + 2-byte port)
- **Does NOT test:** HTTP fetching, network connectivity, libcurl integration

**Evidence:** `tests/Makefile` 58-59, 140-144; `tests/test_serverbrowser_parse.c` (parsing only, no HTTP)

### 2.3 Gap: No HTTP Fetch Test

**Missing validation:**
- ❌ No test that calls `CL_HTTPFetch()` on Linux
- ❌ No test that validates libcurl integration
- ❌ No test that checks HTTP master server fetching works

**Current tests only validate:**
- ✅ Parsing logic (text/binary formats)
- ✅ UI layout calculations
- ❌ **NOT** actual HTTP fetching

---

## 3. Recommendations

### 3.1 Immediate: Verify Build

**Check that `cl_httpfetch.c` compiles on Linux:**
```bash
# In Docker test environment
cd /src
make -f Makefile.unix clean
make -f Makefile.unix USE_SDL2=1 2>&1 | grep -i "cl_httpfetch\|curl\|undefined"
```

**Expected:** No undefined references to `curl_*` functions (libcurl linked correctly).

### 3.2 Add HTTP Fetch Test (Optional)

**Create `tests/test_httpfetch.c`:**
- Mock or use a test HTTP server
- Call `CL_HTTPFetch()` with a known URL
- Validate response parsing
- **Note:** This requires either:
  - A mock HTTP server in the test
  - Network access (may fail in CI)
  - Stubbing libcurl (complex)

### 3.3 Manual Validation

**To fully validate server browser on Linux:**
1. Build Linux client: `make -f Makefile.unix USE_SDL2=1`
2. Run client: `./egl`
3. In console: `sb_refresh`
4. Check console output for:
   - "Fetching server list via HTTP: ..."
   - "HTTP fetch returned X bytes"
   - Server entries appearing in list

---

## 4. Conclusion

| Aspect | Status | Notes |
|--------|--------|-------|
| **Linux HTTP implementation** | ✅ Exists | Uses libcurl (`cl_httpfetch.c` 254-378) |
| **Build system** | ✅ Configured | Links against libcurl (`Makefile.unix` 25-33) |
| **Server browser code** | ✅ Platform-agnostic | Uses `CL_HTTPFetch` abstraction |
| **Docker tests install libcurl** | ✅ Yes | `libcurl4-openssl-dev` installed |
| **Docker tests validate HTTP** | ❌ **No** | Only tests parsing, not fetching |

**Answer:** The server browser **should work on Linux** (code and build are correct), but **Docker tests do NOT validate HTTP fetching** — they only test parsing logic. To fully validate, either:
1. Manually test `sb_refresh` on a Linux build, or
2. Add an HTTP fetch test (requires test HTTP server or network access).
