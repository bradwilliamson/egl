param(
    [ValidateSet('auto','mingw64','ucrt64','clang64')]
    [string]$Prefix = 'auto',
    [string]$Msys2Root = $env:MSYS2_ROOT
)

$ErrorActionPreference = 'Stop'

if (-not $Msys2Root -or -not (Test-Path -LiteralPath $Msys2Root)) {
    $Msys2Root = 'C:\msys64'
}

function Resolve-Prefix {
    param([string]$Root, [string]$Wanted)

    if ($Wanted -ne 'auto') { return $Wanted }

    foreach ($p in @('mingw64','ucrt64','clang64')) {
        if (Test-Path -LiteralPath (Join-Path $Root ($p + '\\bin\\gcc.exe'))) {
            return $p
        }
    }

    return 'mingw64'
}

$Prefix = Resolve-Prefix -Root $Msys2Root -Wanted $Prefix
$binPath = Join-Path $Msys2Root ($Prefix + '\\bin')

if (-not (Test-Path -LiteralPath (Join-Path $binPath 'gcc.exe'))) {
    throw "gcc.exe not found at '$binPath'. Run tools\\setup-msys2.ps1 first (or install the MSYS2 toolchain)."
}

if (-not ($env:PATH -split ';' | Where-Object { $_ -ieq $binPath })) {
    $env:PATH = "$binPath;" + $env:PATH
}

$env:MSYS2_ROOT = $Msys2Root
$env:EGL_MSYS2_PREFIX = $Prefix

Write-Host "MSYS2 environment configured: root=$Msys2Root prefix=$Prefix" -ForegroundColor Green
Write-Host "gcc: $(Get-Command gcc).Source" -ForegroundColor Green
