param(
	[Parameter(ValueFromRemainingArguments = $true)]
	[string[]]$MakeArgs
)

$ErrorActionPreference = 'Stop'

# Ensure we run from the repo root (where the Makefile is), even if the user
# launches this script from inside the tools/ directory.
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
Set-Location $repoRoot

# MSYS2 root: MSYS2_ROOT or EGL_MSYS2_ROOT, else default.
$msys2Root = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } elseif ($env:EGL_MSYS2_ROOT) { $env:EGL_MSYS2_ROOT } else { 'C:\msys64' }

# Toolchain prefix: EGL_MSYS2_PREFIX or MSYS2_PREFIX, else mingw64.
# Use mingw32 for 32-bit (i686), mingw64 for 64-bit (x86_64). Also ucrt64, clang64.
$prefix = if ($env:EGL_MSYS2_PREFIX) { $env:EGL_MSYS2_PREFIX } elseif ($env:MSYS2_PREFIX) { $env:MSYS2_PREFIX } else { 'mingw64' }
$mingwBin = Join-Path $msys2Root "$prefix\bin"

if (-not (Test-Path (Join-Path $mingwBin 'gcc.exe'))) {
	throw "gcc.exe not found at '$mingwBin'. Set MSYS2_ROOT (or EGL_MSYS2_ROOT) and EGL_MSYS2_PREFIX (e.g. mingw64, mingw32). Install the matching MSYS2 toolchain (e.g. mingw-w64-x86_64-gcc or mingw-w64-i686-gcc)."
}
if ($prefix -eq 'mingw32') {
	Write-Host "Note: mingw32 is 32-bit. The Makefile uses -m64 by default; use mingw64 for 64-bit builds." -ForegroundColor Yellow
}

# Ensure MinGW toolchain is on PATH so 'make' can spawn gcc/ld.
$env:PATH = "$mingwBin;" + $env:PATH
$env:EGL_MSYS2_PREFIX = $prefix
$env:MSYS2_PREFIX = $prefix
if (-not $env:MSYS2_ROOT) { $env:MSYS2_ROOT = $msys2Root }
if (-not $env:EGL_MSYS2_ROOT) { $env:EGL_MSYS2_ROOT = $msys2Root }

# Renderer build configuration (default: legacy-only)
$eglLegacy = if ($env:EGL_LEGACY) { $env:EGL_LEGACY } else { '1' }
$eglModern = if ($env:EGL_MODERN) { $env:EGL_MODERN } else { '0' }

# Determine SDL2 status
$sdl2Status = if ($env:USE_SDL2 -eq '1') { '1' } else { '0' }

# Print build configuration banner
Write-Host "Build: SDL2=$sdl2Status LEGACY=$eglLegacy MODERN=$eglModern" -ForegroundColor Cyan

# Pass renderer config to make
$makeArgsWithConfig = @("EGL_LEGACY=$eglLegacy", "EGL_MODERN=$eglModern") + $MakeArgs

& make @makeArgsWithConfig
exit $LASTEXITCODE
