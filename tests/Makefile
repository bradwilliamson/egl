# EGL unit test build (portable)
# Usage:
#   make ci      - build + run all tests (CI-friendly)
#   make clean   - remove built test binaries and logs

CC ?= gcc

CFLAGS += -Wall -Wextra -g -I.. -I../shared -I../common -I../unity -DTEST_BUILD
CFLAGS += -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function

# Source files from main project needed for tests
SHARED_SRC = \
	../shared/mathlib.c \
	../shared/m_angles.c \
	../shared/m_plane.c \
	../shared/m_bounds.c \
	../shared/m_mat3.c \
	../shared/m_mat4.c \
	../shared/m_quat.c \
	../shared/byteswap.c \
	../shared/string.c \
	../shared/shared.c

# Stub for Com_Printf (used by mathlib.c and string.c)
COM_STUBS = test_stubs.c

SHARED_SRC_NO_PLANE = \
	../shared/mathlib.c \
	../shared/m_angles.c \
	../shared/m_bounds.c \
	../shared/byteswap.c \
	../shared/string.c \
	../shared/shared.c

SDL2_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL2_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null)

# Fallback for MSYS2/MinGW when pkg-config fails
ifeq ($(SDL2_CFLAGS),)
  ifdef MSYSTEM_PREFIX
    SDL2_CFLAGS := -I$(MSYSTEM_PREFIX)/include/SDL2
    SDL2_LIBS := -L$(MSYSTEM_PREFIX)/lib -lSDL2
  endif
endif

# Only include SDL test if SDL2 is available
SDL_TEST_BIN :=
ifneq ($(SDL2_LIBS),)
  SDL_TEST_BIN := test_sdl_input
endif

TEST_BINS = \
	test_mathlib \
	test_byteswap \
	test_net_msg \
	test_string \
	test_crc \
	test_infostrings \
	$(SDL_TEST_BIN) \
	test_cvar \
	test_cmd \
	test_keys \
	test_com_parse \
	test_resolution \
	test_sound_spatialization \
	test_sound_channel \
	test_net_stringtoaddr \
	test_netchan \
	test_netchan_oob \
	test_memory \
	test_playerstate \
	test_inventory \
	test_pmove_clipvelocity \
	test_pmove_friction \
	test_pmove_accelerate \
	test_pmove_airaccelerate \
	test_pmove_quantization \
	test_bounds \
	test_plane \
	test_mat_quat \
	test_angles

.PHONY: all ci clean

all: $(TEST_BINS)

# CI target - returns proper exit code and writes per-test logs
ci: clean
	@echo "=== Building test binaries ==="
	@echo "SDL2_CFLAGS: $(SDL2_CFLAGS)"
	@echo "SDL2_LIBS: $(SDL2_LIBS)"
	@echo "SDL_TEST_BIN: $(SDL_TEST_BIN)"
	@$(MAKE) $(TEST_BINS)
	@echo "=== Running tests ==="
	@set -e; \
	exit_code=0; \
	for t in $(TEST_BINS); do \
		if [ -f ./$$t ]; then \
			echo "Running $$t..."; \
			./$$t > $$t.log 2>&1 || exit_code=1; \
			cat $$t.log; \
		else \
			echo "SKIP: $$t not built"; \
		fi; \
	done; \
	exit $$exit_code

clean:
	rm -f $(TEST_BINS) *.o *.log test-output.log

# Individual test targets

test_mathlib: test_mathlib.c $(SHARED_SRC) $(COM_STUBS)
	$(CC) $(CFLAGS) -o $@ $< $(SHARED_SRC) $(COM_STUBS) -lm

test_byteswap: test_byteswap.c ../shared/byteswap.c
	$(CC) $(CFLAGS) -o $@ $< ../shared/byteswap.c

test_net_msg: test_net_msg.c ../common/net_msg.c $(SHARED_SRC_NO_PLANE) $(COM_STUBS)
	$(CC) $(CFLAGS) -o $@ $< ../common/net_msg.c $(SHARED_SRC_NO_PLANE) $(COM_STUBS) -lm

test_string: test_string.c ../shared/string.c ../shared/shared.c $(COM_STUBS)
	$(CC) $(CFLAGS) -o $@ $< ../shared/string.c ../shared/shared.c $(COM_STUBS)

test_crc: test_crc.c ../common/crc.c ../shared/shared.c ../shared/string.c $(COM_STUBS)
	$(CC) $(CFLAGS) -o $@ $< ../common/crc.c ../shared/shared.c ../shared/string.c $(COM_STUBS)

test_infostrings: test_infostrings.c ../shared/infostrings.c ../shared/shared.c ../shared/string.c $(COM_STUBS)
	$(CC) $(CFLAGS) -o $@ $< ../shared/infostrings.c ../shared/shared.c ../shared/string.c $(COM_STUBS)

test_sdl_input: test_sdl_input.c
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) -o $@ $< $(SDL2_LIBS)

test_cvar: test_cvar.c
	$(CC) $(CFLAGS) -o $@ $<

test_cmd: test_cmd.c
	$(CC) $(CFLAGS) -o $@ $<

test_keys: test_keys.c
	$(CC) $(CFLAGS) -o $@ $<

test_com_parse: test_com_parse.c
	$(CC) $(CFLAGS) -o $@ $<

test_resolution: test_resolution.c
	$(CC) $(CFLAGS) -o $@ $<

test_sound_spatialization: test_sound_spatialization.c
	$(CC) $(CFLAGS) -o $@ $< -lm

test_sound_channel: test_sound_channel.c
	$(CC) $(CFLAGS) -o $@ $< -lm

test_net_stringtoaddr: test_net_stringtoaddr.c
	$(CC) $(CFLAGS) -o $@ $<

test_netchan: test_netchan.c
	$(CC) $(CFLAGS) -o $@ $<

test_netchan_oob: test_netchan_oob.c
	$(CC) $(CFLAGS) -o $@ $< -lm

test_memory: test_memory.c
	$(CC) $(CFLAGS) -o $@ $<

test_playerstate: test_playerstate.c
	$(CC) $(CFLAGS) -o $@ $<

test_inventory: test_inventory.c
	$(CC) $(CFLAGS) -o $@ $<

test_pmove_clipvelocity: test_pmove_clipvelocity.c
	$(CC) $(CFLAGS) -o $@ $< -lm

test_pmove_friction: test_pmove_friction.c
	$(CC) $(CFLAGS) -o $@ $< -lm

test_pmove_accelerate: test_pmove_accelerate.c
	$(CC) $(CFLAGS) -o $@ $< -lm

test_pmove_airaccelerate: test_pmove_airaccelerate.c
	$(CC) $(CFLAGS) -o $@ $< -lm

test_pmove_quantization: test_pmove_quantization.c
	$(CC) $(CFLAGS) -o $@ $< -lm

test_bounds: test_bounds.c $(SHARED_SRC) $(COM_STUBS)
	$(CC) $(CFLAGS) -o $@ $< $(SHARED_SRC) $(COM_STUBS) -lm

test_plane: test_plane.c $(SHARED_SRC) $(COM_STUBS)
	$(CC) $(CFLAGS) -o $@ $< $(SHARED_SRC) $(COM_STUBS) -lm

test_mat_quat: test_mat_quat.c $(SHARED_SRC) $(COM_STUBS)
	$(CC) $(CFLAGS) -o $@ $< $(SHARED_SRC) $(COM_STUBS) -lm

test_angles: test_angles.c $(SHARED_SRC) $(COM_STUBS)
	$(CC) $(CFLAGS) -o $@ $< $(SHARED_SRC) $(COM_STUBS) -lm
