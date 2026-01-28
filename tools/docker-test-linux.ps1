param(
  [string]$Image = $env:EGL_DOCKER_IMAGE
)

if ([string]::IsNullOrWhiteSpace($Image)) { $Image = 'ubuntu:22.04' }

$root = (Resolve-Path -LiteralPath '.').Path

docker run --rm -t `
  -v "${root}:/src" `
  -w /src `
  $Image `
  sh -lc "apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential pkg-config ca-certificates libcurl4-openssl-dev && make -C tests ci"
