param(
    [string]$OutDir = "out\\egl",
    [switch]$SkipBuild,
    [switch]$SkipDedicated,
    [switch]$NoPkzCopy,
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$MakeArgs
)

$ErrorActionPreference = 'Stop'

function Get-ObjdumpPath {
    param(
        [string]$ToolchainBin
    )

    if (-not $ToolchainBin) { return $null }

    $candidates = @(
        (Join-Path $ToolchainBin 'objdump.exe'),
        (Join-Path $ToolchainBin 'objdump')
    )
    foreach ($c in $candidates) {
        if ($c -and (Test-Path -LiteralPath $c)) {
            return $c
        }
    }

    return $null
}

function Get-PeImportDllNames {
    param(
        [string]$Objdump,
        [string]$BinaryPath
    )

    if (-not (Test-Path -LiteralPath $BinaryPath)) {
        throw "Dependency check: missing binary '$BinaryPath'"
    }

    $lines = & $Objdump -p $BinaryPath 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $lines) {
        throw "Dependency check: failed to read imports from '$BinaryPath'"
    }

    $names = @()
    foreach ($m in ($lines | Select-String -Pattern '^\s*DLL Name:\s*' -AllMatches)) {
        $name = ($m.Line -replace '^\s*DLL Name:\s*', '').Trim()
        if ($name) { $names += $name }
    }

    $names | Sort-Object -Unique
}

function Test-IsSystemDll {
    param(
        [string]$Name
    )

    if (-not $Name) { return $true }
    $n = $Name.ToLowerInvariant()

    if ($n -like 'api-ms-win-*.dll') { return $true }
    if ($n -like 'ext-ms-*.dll') { return $true }

    $system = @(
        'advapi32.dll',
        'bcrypt.dll',
        'cfgmgr32.dll',
        'comctl32.dll',
        'comdlg32.dll',
        'crypt32.dll',
        'gdi32.dll',
        'hid.dll',
        'imm32.dll',
        'kernel32.dll',
        'msvcrt.dll',
        'ntdll.dll',
        'ole32.dll',
        'oleaut32.dll',
        'opengl32.dll',
        'rpcrt4.dll',
        'secur32.dll',
        'setupapi.dll',
        'shell32.dll',
        'shlwapi.dll',
        'ucrtbase.dll',
        'user32.dll',
        'version.dll',
        'winmm.dll',
        'ws2_32.dll',
        'wsock32.dll'
    )

    return $system -contains $n
}

function Test-StagedDependencies {
    param(
        [string]$Objdump,
        [string]$OutRoot,
        [string]$OutBase
    )

    $searchDirs = @($OutRoot, $OutBase) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }
    if (-not $searchDirs) {
        throw 'Dependency check: output folders do not exist'
    }

    $roots = @(
        (Join-Path $OutRoot 'egl.exe'),
        (Join-Path $OutRoot 'eglded.exe'),
        (Join-Path $OutBase 'gamex64.dll'),
        (Join-Path $OutBase 'eglcgamex64.dll')
    ) | Where-Object { Test-Path -LiteralPath $_ }

    # Also include all staged DLLs next to egl.exe to catch transitive deps.
    $roots += (Get-ChildItem -LiteralPath $OutRoot -File -Filter '*.dll' -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })

    $queue = New-Object System.Collections.Generic.Queue[string]
    $seen = New-Object 'System.Collections.Generic.HashSet[string]'

    foreach ($r in ($roots | Sort-Object -Unique)) {
        $full = (Resolve-Path -LiteralPath $r).Path
        if ($seen.Add($full)) { $queue.Enqueue($full) }
    }

    $missing = New-Object System.Collections.Generic.List[string]

    while ($queue.Count -gt 0) {
        $current = $queue.Dequeue()
        $imports = Get-PeImportDllNames -Objdump $Objdump -BinaryPath $current

        foreach ($imp in $imports) {
            if (Test-IsSystemDll -Name $imp) { continue }

            $foundPath = $null
            foreach ($dir in $searchDirs) {
                $candidate = Join-Path $dir $imp
                if (Test-Path -LiteralPath $candidate) {
                    $foundPath = (Resolve-Path -LiteralPath $candidate).Path
                    break
                }
            }

            if (-not $foundPath) {
                $missing.Add("$imp (required by $(Split-Path -Leaf $current))")
                continue
            }

            if ($seen.Add($foundPath)) {
                $queue.Enqueue($foundPath)
            }
        }
    }

    if ($missing.Count -gt 0) {
        $msg = "Portable build is missing required DLLs:\n- " + (($missing | Sort-Object -Unique) -join "\n- ")
        throw $msg
    }
}

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

$objdump = Get-ObjdumpPath -ToolchainBin $toolchainBin
if (-not $objdump) {
    throw "Could not find objdump.exe under toolchain bin '$toolchainBin'. Ensure binutils is installed with your MSYS2 toolchain."
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

# Fail fast if any staged binary/DLL depends on a non-system DLL that wasn't staged.
Test-StagedDependencies -Objdump $objdump -OutRoot $outRoot -OutBase $outBase

Write-Host "Packaged portable build to: $outRoot" -ForegroundColor Green
Write-Host "Run: `"$outRoot\\egl.exe`"" -ForegroundColor Green
