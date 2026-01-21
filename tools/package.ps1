param(
    [string]$OutDir = "out\\egl",
    [switch]$SkipBuild,
    [switch]$SkipDedicated,
    [switch]$NoPkzCopy,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$MakeArgs
)

$ErrorActionPreference = 'Stop'

function Find-Msys2Root {
    param(
        [string[]]$Candidates
    )

    foreach ($c in $Candidates) {
        if (-not $c) { continue }
        try {
            $root = (Resolve-Path -LiteralPath $c -ErrorAction Stop).Path
        } catch {
            continue
        }
        if (Test-Path -LiteralPath (Join-Path $root 'mingw64\bin\gcc.exe')) {
            return $root
        }
    }

    return $null
}

function Find-Msys2ToolchainBin {
    param(
        [string]$Msys2Root
    )

    if (-not $Msys2Root) { return $null }

    $bins = @(
        'mingw64\bin',
        'ucrt64\bin',
        'clang64\bin'
    )

    foreach ($b in $bins) {
        $binPath = Join-Path $Msys2Root $b
        if (Test-Path -LiteralPath (Join-Path $binPath 'gcc.exe')) {
            return $binPath
        }
    }

    return $null
}

function Get-RepoRoot {
    (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
}

$root = Get-RepoRoot
Set-Location $root

# Ensure MinGW toolchain is available when running from a fresh PowerShell.
$candidateRoots = @(
    $env:MSYS2_ROOT,
    $env:MSYS2ROOT,
    'C:\msys64',
    'D:\msys64',
    $(if ($env:RUNNER_TEMP) { Join-Path $env:RUNNER_TEMP 'msys64' } else { $null }),
    $(if ($env:TEMP) { Join-Path $env:TEMP 'msys64' } else { $null }),
    'C:\tools\msys64'
)

$msys2Root = Find-Msys2Root -Candidates $candidateRoots
if (-not $msys2Root) {
    $msys2Root = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { 'C:\msys64' }
}

$toolchainBin = Find-Msys2ToolchainBin -Msys2Root $msys2Root
if (-not $toolchainBin) {
    $expected = @(
        (Join-Path $msys2Root 'mingw64\bin\gcc.exe'),
        (Join-Path $msys2Root 'ucrt64\bin\gcc.exe'),
        (Join-Path $msys2Root 'clang64\bin\gcc.exe')
    ) -join "\n  - "
    throw "Could not find gcc.exe under detected MSYS2 root '$msys2Root'. Expected one of:\n  - $expected\n\nInstall MSYS2 + a toolchain (e.g. mingw-w64-x86_64-toolchain or mingw-w64-ucrt-x86_64-toolchain), or set MSYS2_ROOT to your MSYS2 install folder."
}

if (-not ($env:PATH -split ';' | Where-Object { $_ -ieq $toolchainBin })) {
    $env:PATH = "$toolchainBin;" + $env:PATH
}

if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) {
    throw "gcc is still not discoverable after updating PATH with '$toolchainBin'. Current PATH does not allow running the toolchain."
}

# If USE_SDL2=1 was requested, sanity-check that SDL2 headers exist under the detected MSYS2 root.
if ($MakeArgs -and ($MakeArgs | Where-Object { $_ -match '^(?:USE_SDL2=1|USE_SDL2\s*=\s*1)$' })) {
    $sdlHeader = Join-Path $msys2Root 'mingw64\include\SDL2\SDL.h'
    if (-not (Test-Path -LiteralPath $sdlHeader)) {
        throw "SDL2 headers not found at '$sdlHeader'. Ensure MSYS2 SDL2 is installed (package: mingw-w64-x86_64-SDL2) and MSYS2_ROOT points at the correct MSYS2 install."
    }
}

$outRootPath = Join-Path $root $OutDir
New-Item -ItemType Directory -Force -Path $outRootPath | Out-Null
$outRoot = (Resolve-Path -LiteralPath $outRootPath).Path

$outBase = Join-Path $outRoot 'baseq2'
New-Item -ItemType Directory -Force -Path $outBase | Out-Null

if (-not $SkipBuild) {
    if (-not $MakeArgs) { $MakeArgs = @() }

    # Avoid stale objects when switching build flags (e.g. USE_SDL2 on/off).
    & make clean
    if ($LASTEXITCODE -ne 0) { throw "make clean failed with exit code $LASTEXITCODE" }

    & make @MakeArgs all
    if ($LASTEXITCODE -ne 0) { throw "make all failed with exit code $LASTEXITCODE" }

    if (-not $SkipDedicated) {
        & make @MakeArgs dedicated
        if ($LASTEXITCODE -ne 0) { throw "make dedicated failed with exit code $LASTEXITCODE" }
    }
}

# Stage executables
$clientExe = Join-Path $root 'egl.exe'
if (-not (Test-Path -LiteralPath $clientExe)) {
    throw "Missing egl.exe. Run without -SkipBuild or build it first." 
}
Copy-Item -Force -Path $clientExe -Destination (Join-Path $outRoot 'egl.exe')

# If USE_SDL2=1 was requested, stage SDL2.dll next to the exe so it runs without MSYS2 on PATH.
if ($MakeArgs -and ($MakeArgs | Where-Object { $_ -match '^(?:USE_SDL2=1|USE_SDL2\s*=\s*1)$' })) {
    $sdlDll = Join-Path $toolchainBin 'SDL2.dll'
    if (-not (Test-Path -LiteralPath $sdlDll)) {
        throw "SDL2.dll not found at '$sdlDll'. Ensure MSYS2 SDL2 runtime is installed for your prefix (e.g. mingw-w64-x86_64-SDL2)."
    }
    Copy-Item -Force -Path $sdlDll -Destination (Join-Path $outRoot 'SDL2.dll')
}

# Stage required runtime DLLs.
# - App deps: minizip/zlib/bzip2
# - Toolchain deps (needed on machines without MSYS2): libgcc/libstdc++/winpthread
$runtimeDlls = @(
    'libminizip-1.dll',
    'zlib1.dll',
    'libbz2-1.dll',
    'libgcc_s_seh-1.dll',
    'libstdc++-6.dll',
    'libwinpthread-1.dll'
)
foreach ($dll in $runtimeDlls) {
    $dllPath = Join-Path $toolchainBin $dll
    if (Test-Path -LiteralPath $dllPath) {
        Copy-Item -Force -Path $dllPath -Destination (Join-Path $outRoot $dll)
    }
}

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
