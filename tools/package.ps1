param(
    [string]$OutDir = "out\\egl",
    [switch]$SkipBuild,
    [switch]$SkipDedicated,
    [switch]$NoPkzCopy
)

$ErrorActionPreference = 'Stop'

function Get-RepoRoot {
    (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
}

$root = Get-RepoRoot
Set-Location $root

# Ensure MinGW toolchain is available when running from a fresh PowerShell.
$msys2Root = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { 'C:\msys64' }
$mingwBin = Join-Path $msys2Root 'mingw64\bin'
if (-not ($env:PATH -split ';' | Where-Object { $_ -ieq $mingwBin })) {
    $env:PATH = "$mingwBin;" + $env:PATH
}

$outRootPath = Join-Path $root $OutDir
New-Item -ItemType Directory -Force -Path $outRootPath | Out-Null
$outRoot = (Resolve-Path -LiteralPath $outRootPath).Path

$outBase = Join-Path $outRoot 'baseq2'
New-Item -ItemType Directory -Force -Path $outBase | Out-Null

if (-not $SkipBuild) {
    & make all
    if ($LASTEXITCODE -ne 0) { throw "make all failed with exit code $LASTEXITCODE" }

    if (-not $SkipDedicated) {
        & make dedicated
        if ($LASTEXITCODE -ne 0) { throw "make dedicated failed with exit code $LASTEXITCODE" }
    }
}

# Stage executables
$clientExe = Join-Path $root 'egl.exe'
if (-not (Test-Path -LiteralPath $clientExe)) {
    throw "Missing egl.exe. Run without -SkipBuild or build it first." 
}
Copy-Item -Force -Path $clientExe -Destination (Join-Path $outRoot 'egl.exe')

$dedExe = Join-Path $root 'eglded.exe'
if (-not $SkipDedicated -and (Test-Path -LiteralPath $dedExe)) {
    Copy-Item -Force -Path $dedExe -Destination (Join-Path $outRoot 'eglded.exe')
    # Compatibility alias with previous target naming
    Copy-Item -Force -Path $dedExe -Destination (Join-Path $outRoot 'egl-dedicated.exe')
}

# Stage DLL modules (64-bit toolchain output)
$dlls = @(
    'gamex64.dll',
    'eglcgamex64.dll'
)
foreach ($dll in $dlls) {
    $src = Join-Path $root $dll
    if (Test-Path -LiteralPath $src) {
        Copy-Item -Force -Path $src -Destination (Join-Path $outBase $dll)
    }
    else {
        Write-Warning "Did not find $dll; output may not be playable without it."
    }
}

# Stage data pack
if (-not $NoPkzCopy) {
    $pkz = Join-Path $root 'data\\egl.pkz'
    if (Test-Path -LiteralPath $pkz) {
        Copy-Item -Force -Path $pkz -Destination (Join-Path $outBase 'egl.pkz')
    }
    else {
        Write-Warning "Did not find data\\egl.pkz; output will not be directly playable without it."
    }
}

Write-Host "Packaged portable build to: $outRoot" -ForegroundColor Green
Write-Host "Run: `"$outRoot\\egl.exe`"" -ForegroundColor Green
