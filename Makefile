# Basic Makefile for EGL Quake 2 on MinGW (Windows)
CC ?= gcc
RC ?= windres
RCFLAGS ?=

# Some Windows environments set CC=cc by default, but don't actually provide a 'cc' executable.
# If the caller wants a specific compiler, they can still override CC on the command line.
ifeq ($(CC),cc)
CC = gcc
endif

# Renderer build configuration (default: legacy-only)
EGL_LEGACY ?= 1
EGL_MODERN ?= 0

# Optional SDL2 backend (set USE_SDL2=1 to enable)
ifdef USE_SDL2
# Allow overriding MSYS2 location/prefix via env or make vars.
MSYS2_ROOT ?= $(or $(EGL_MSYS2_ROOT),C:/msys64)
MSYS2_PREFIX ?= $(or $(EGL_MSYS2_PREFIX),mingw64)

SDL2_INC_DIR = $(MSYS2_ROOT)/$(MSYS2_PREFIX)/include/SDL2
SDL2_LIB_DIR = $(MSYS2_ROOT)/$(MSYS2_PREFIX)/lib
SDL2_DLL_SRC = $(MSYS2_ROOT)/$(MSYS2_PREFIX)/bin/SDL2.dll

CFLAGS += -DHAVE_SDL2 -I"$(SDL2_INC_DIR)"
LDFLAGS += -L"$(SDL2_LIB_DIR)" -lSDL2
SDL2_SOURCES = sdl2/sdl_glimp.c sdl2/sdl_input.c sdl2/sdl_snd.c sdl2/sdl_main.c
else
SDL2_SOURCES =
endif
CFLAGS += -DWIN32 -DEGL_LEGACY_RENDERER=$(EGL_LEGACY) -DEGL_MODERN_RENDERER=$(EGL_MODERN) -m64 -O2 -Wall -Wno-deprecated-declarations -Wno-unused-function -I. -I./include -I./shared -I./renderer -I./client -I./cgame -I./game -I./server -I./win32
LDFLAGS += -m64 -mwindows -lopengl32 -lglu32 -lgdi32 -luser32 -lkernel32 -lwinmm -lws2_32 -lwinhttp -lole32 -luuid -lwindowscodecs -lz -lminizip -ldbghelp

# Optional debug build (symbols + no optimization) for diagnosing crashes.
DBG_CFLAGS = -DWIN32 -DEGL_LEGACY_RENDERER=$(EGL_LEGACY) -DEGL_MODERN_RENDERER=$(EGL_MODERN) -m64 -O0 -g3 -fno-omit-frame-pointer -Wall -Wno-deprecated-declarations -Wno-unused-function -I. -I./include -I./shared -I./renderer -I./client -I./cgame -I./game -I./server -I./win32
DBG_LDFLAGS = -m64 -mwindows -Wl,--pdb=egl.pdb -lopengl32 -lglu32 -lgdi32 -luser32 -lkernel32 -lwinmm -lws2_32 -lwinhttp -lole32 -luuid -lwindowscodecs -lz -lminizip -ldbghelp

# Dedicated server build (no renderer/client). Uses separate objects to avoid flag collisions.
DED_OBJDIR = build/dedicated
DED_CFLAGS = -DWIN32 -DDEDICATED_ONLY -m64 -O2 -Wall -Wno-deprecated-declarations -Wno-unused-function -I. -I./include -I./shared -I./server -I./win32
DED_LDFLAGS = -m64 -mwindows -lgdi32 -luser32 -lkernel32 -lwinmm -lws2_32 -lwinhttp -lole32 -luuid -lz -lminizip -ldbghelp

# Module naming follows win32/win_main.c (LIBARCH = x64 for this toolchain)
LIBARCH = x64
CGAME_DLL = eglcgame$(LIBARCH).dll
GAME_DLL = game$(LIBARCH).dll
DLL_LDFLAGS = -m64 -shared
WIN_RC_SRC = win32/EGL.rc
WIN_RC_OBJ = win32/EGL.res.o

# Source files (adjust paths as needed)
SHARED_SRC = $(wildcard shared/*.c)
COMMON_SRC = $(filter-out common/zlib_stubs.c, $(wildcard common/*.c)) $(SHARED_SRC)
ifdef USE_SDL2
# SDL2 build: use SDL2 for video/input/sound, but still need core Win32 platform code
WIN32_SYS_SRC = win32/win_main.c win32/win_console.c win32/win_sock.c win32/win_snd_cd.c
CLIENT_SRC = $(wildcard client/*.c) $(WIN32_SYS_SRC) $(SDL2_SOURCES)
else
CLIENT_SRC = $(wildcard client/*.c) $(wildcard win32/*.c)
endif
CGAME_SRC = $(wildcard cgame/*.c) $(wildcard cgame/menu/*.c) $(wildcard cgame/ui/*.c)
GAME_SRC = $(wildcard game/*.c)
RENDERER_SRC = $(wildcard renderer/*.c) renderer/glad/glad.c
# Conditionally add modern renderer sources if EGL_MODERN=1
ifeq ($(EGL_MODERN),1)
RENDERER_SRC += $(wildcard renderer/modern/*.c)
endif
SERVER_SRC = $(filter-out server/server_stubs.c, $(wildcard server/*.c))

# Dedicated build uses only the Win32 system/console/net layer.
DED_SYS_SRC = win32/win_main.c win32/win_console.c win32/win_sock.c

# Object files
COMMON_OBJ = $(COMMON_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
CGAME_OBJ = $(CGAME_SRC:.c=.o)
GAME_OBJ = $(GAME_SRC:.c=.o)
RENDERER_OBJ = $(RENDERER_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

SHARED_OBJ = $(SHARED_SRC:.c=.o)

DED_COMMON_OBJ = $(addprefix $(DED_OBJDIR)/,$(COMMON_SRC:.c=.o))
DED_SERVER_OBJ = $(addprefix $(DED_OBJDIR)/,$(SERVER_SRC:.c=.o))
DED_SYS_OBJ = $(addprefix $(DED_OBJDIR)/,$(DED_SYS_SRC:.c=.o))

# Targets
# Default to client + modules. Dedicated server can be built explicitly.
all: egl.exe modules

debug:
	$(MAKE) clean
	$(MAKE) all CFLAGS="$(DBG_CFLAGS)" LDFLAGS="$(DBG_LDFLAGS)"

egl.exe: $(COMMON_OBJ) $(CLIENT_OBJ) $(RENDERER_OBJ) $(SERVER_OBJ) $(WIN_RC_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)
ifdef USE_SDL2
	-powershell -NoProfile -Command "if (Test-Path -LiteralPath '$(SDL2_DLL_SRC)') { Copy-Item -Force -LiteralPath '$(SDL2_DLL_SRC)' -Destination 'SDL2.dll' } else { throw 'SDL2.dll not found at $(SDL2_DLL_SRC). Install MSYS2 SDL2 for your prefix (e.g. mingw-w64-x86_64-SDL2) or set EGL_MSYS2_ROOT/EGL_MSYS2_PREFIX.' }"
endif

$(CGAME_DLL): $(CGAME_OBJ) $(SHARED_OBJ) cgame/exports.def
	$(CC) -o $@ $(DLL_LDFLAGS) $(CGAME_OBJ) $(SHARED_OBJ) cgame/exports.def

$(GAME_DLL): $(GAME_OBJ) $(SHARED_OBJ) game/exports.def
	$(CC) -o $@ $(DLL_LDFLAGS) $(GAME_OBJ) $(SHARED_OBJ) game/exports.def

modules: $(CGAME_DLL) $(GAME_DLL)
	-powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path baseq2 | Out-Null"
	-powershell -NoProfile -Command "Copy-Item -Force $(CGAME_DLL) baseq2\\$(CGAME_DLL)"
	-powershell -NoProfile -Command "Copy-Item -Force $(GAME_DLL) baseq2\\$(GAME_DLL)"

egl-dedicated.exe: $(COMMON_OBJ) $(SERVER_OBJ) $(GAME_OBJ)
	@echo "Deprecated target: use 'eglded.exe'"
	@$(MAKE) eglded.exe
	@powershell -NoProfile -Command "Copy-Item -Force eglded.exe egl-dedicated.exe"

eglded.exe: $(DED_COMMON_OBJ) $(DED_SERVER_OBJ) $(DED_SYS_OBJ) $(WIN_RC_OBJ)
	$(CC) -o $@ $^ $(DED_LDFLAGS)

$(WIN_RC_OBJ): $(WIN_RC_SRC) win32/resource.h favicon.ico
	$(RC) $(RCFLAGS) -I./win32 -O coff -o $@ $<

dedicated: egl-dedicated.exe

$(DED_OBJDIR)/%.o: %.c
	-powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(subst /,\\,$(dir $@))' | Out-Null"
	$(CC) $(DED_CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-powershell -NoProfile -Command "Get-Process egl -ErrorAction SilentlyContinue | Stop-Process -Force"
	-powershell -NoProfile -Command "Remove-Item -Force -ErrorAction SilentlyContinue *.exe"
	-powershell -NoProfile -Command "Get-ChildItem -Recurse -Include *.o -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue"
	-powershell -NoProfile -Command "Remove-Item -Recurse -Force -ErrorAction SilentlyContinue build\\dedicated"

.PHONY: all clean dedicated debug
