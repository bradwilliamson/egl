#!/usr/bin/env bash
# tools/check_environment.sh - verify build environment on Unix-like systems
# Usage: ./tools/check_environment.sh

echo "Checking environment..."
missing=0
check_cmd() {
  if command -v "$1" >/dev/null 2>&1; then
    echo "OK: $1 -> $(command -v $1)"
  else
    echo "MISSING: $1"
    missing=1
  fi
}

check_cmd cc
check_cmd make
check_cmd pkg-config
check_cmd tar
check_cmd unzip

# Verify dev libraries using pkg-config when available
pkg-config --exists x11 || { echo "MISSING: X11 dev headers (install libx11-dev or equivalent)"; missing=1; }
pkg-config --exists gl || { echo "MISSING: OpenGL dev headers (install libgl-dev or equivalent)"; missing=1; }
pkg-config --exists zlib || { echo "MISSING: zlib (install zlib1g-dev or equivalent)"; missing=1; }
pkg-config --exists minizip || { echo "MISSING: minizip (install libminizip-dev or equivalent)"; missing=1; }

if [ "$missing" -eq 0 ]; then
  echo "Environment looks OK. Try: make -f Makefile.unix all"
  exit 0
else
  echo "Some dependencies are missing. See docs/ENVIRONMENT.md for install instructions."
  exit 1
fi
