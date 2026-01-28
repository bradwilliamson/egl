#!/usr/bin/env sh
set -eu

# Portable staging for Linux/Unix builds.
# Produces: out/egl-linux/ with ./egl, ./eglded, and baseq2/*

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$ROOT_DIR/out/egl-linux"
BASEQ2_OUT="$OUT_DIR/baseq2"

MAKEFILE="$ROOT_DIR/Makefile.unix"

echo "[egl] Packaging portable Linux folder..."

mkdir -p "$BASEQ2_OUT"

# Build
make -f "$MAKEFILE" -C "$ROOT_DIR" clean
JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
make -f "$MAKEFILE" -C "$ROOT_DIR" -j"$JOBS" USE_SDL2=1 all
make -f "$MAKEFILE" -C "$ROOT_DIR" -j"$JOBS" USE_SDL2=1 dedicated

# Stage binaries
cp -f "$ROOT_DIR/egl" "$OUT_DIR/egl"
cp -f "$ROOT_DIR/eglded" "$OUT_DIR/eglded"

# Stage engine data + modules
cp -f "$ROOT_DIR/data/egl.pkz" "$BASEQ2_OUT/egl.pkz"
cp -f "$ROOT_DIR/baseq2/game.so" "$BASEQ2_OUT/game.so"
cp -f "$ROOT_DIR/baseq2/eglcgame.so" "$BASEQ2_OUT/eglcgame.so"

cat <<EOF

Packaged to: $OUT_DIR

To run:
  $OUT_DIR/egl

You still need to provide your legally-obtained Quake II data:
  copy pak0.pak into: $BASEQ2_OUT/
EOF
