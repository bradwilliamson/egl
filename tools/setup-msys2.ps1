param(
    [ValidateSet('auto','mingw64','ucrt64','clang64')]
    [string]$Prefix = 'auto',
    [string]$Msys2Root = $env:MSYS2_ROOT
)

$ErrorActionPreference = 'Stop'

if (-not $Msys2Root -or -not (Test-Path -LiteralPath $Msys2Root)) {
    $Msys2Root = 'C:\msys64'
}

$bash = Join-Path $Msys2Root 'usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $bash)) {
    throw "MSYS2 bash not found at '$bash'. Install MSYS2 first, or pass -Msys2Root <path>."
}

function Pick-Prefix {
    param([string]$Root, [string]$Wanted)

    if ($Wanted -ne 'auto') { return $Wanted }

    foreach ($p in @('mingw64','ucrt64','clang64')) {
        if (Test-Path -LiteralPath (Join-Path $Root ($p + '\\bin'))) {
            return $p
        }
    }

    return 'mingw64'
}

$Prefix = Pick-Prefix -Root $Msys2Root -Wanted $Prefix

switch ($Prefix) {
    'mingw64' { $toolchainPkg = 'mingw-w64-x86_64-toolchain'; $sdlPkg = 'mingw-w64-x86_64-SDL2' }
    'ucrt64'  { $toolchainPkg = 'mingw-w64-ucrt-x86_64-toolchain'; $sdlPkg = 'mingw-w64-ucrt-x86_64-SDL2' }
    'clang64' { $toolchainPkg = 'mingw-w64-clang-x86_64-toolchain'; $sdlPkg = 'mingw-w64-clang-x86_64-SDL2' }
}

Write-Host "Using MSYS2 root: $Msys2Root" -ForegroundColor Cyan
Write-Host "Using prefix: $Prefix" -ForegroundColor Cyan
Write-Host "Installing: $toolchainPkg, $sdlPkg" -ForegroundColor Cyan

# Sync package DB and install needed packages.
& $bash -lc "pacman -Sy --noconfirm"
& $bash -lc "pacman -S --needed --noconfirm $toolchainPkg $sdlPkg"

Write-Host "Done. To use gcc/make from this PowerShell session:" -ForegroundColor Green
Write-Host "  . .\\tools\\msys2-env.ps1 -Prefix $Prefix" -ForegroundColor Green
