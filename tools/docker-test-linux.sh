#!/usr/bin/env sh
set -eu

IMAGE=${EGL_DOCKER_IMAGE:-ubuntu:22.04}

# MSYS/Git Bash can rewrite ':'-containing args (like docker -v C:/x:/y) into bogus paths.
# This breaks Docker Desktop volume mounts. Disable conversion for this script.
if [ -n "${MSYSTEM:-}" ] || [ -n "${MSYS:-}" ]; then
	: "${MSYS2_ARG_CONV_EXCL:=*}"
	export MSYS2_ARG_CONV_EXCL
fi

HOST_PWD=${PWD:-""}

# On Windows with Git Bash/MSYS, docker volume mounts need a Windows absolute path.
# Prefer (in order): cygpath -aw, pwd -W, plain pwd.
if command -v cygpath >/dev/null 2>&1; then
	HOST_PWD=$(cygpath -aw .)
elif pwd -W >/dev/null 2>&1; then
	HOST_PWD=$(pwd -W)
else
	HOST_PWD=$(pwd)
fi

# Build and run unit tests inside a Linux container.
DOCKER_CMD="docker"
if [ -n "${MSYSTEM:-}" ] || [ -n "${MSYS:-}" ]; then
	DOCKER_CMD="cmd.exe /c docker"
fi

$DOCKER_CMD run --rm -t \
  -v "${HOST_PWD}:/src" \
  -w /src \
  "${IMAGE}" \
  sh -lc "apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential pkg-config ca-certificates libcurl4-openssl-dev && make -C tests ci"
