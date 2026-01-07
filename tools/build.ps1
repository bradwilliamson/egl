param(
	[Parameter(ValueFromRemainingArguments = $true)]
	[string[]]$MakeArgs
)

$ErrorActionPreference = 'Stop'

# Ensure we run from the repo root (where the Makefile is), even if the user
# launches this script from inside the tools/ directory.
$repoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
Set-Location $repoRoot

$msys2Root = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { 'C:\msys64' }
$mingwBin = Join-Path $msys2Root 'mingw64\bin'

if (-not (Test-Path (Join-Path $mingwBin 'gcc.exe'))) {
	throw "gcc.exe not found at '$mingwBin'. Set MSYS2_ROOT or install MSYS2 MinGW-w64 x86_64 toolchain."
}

# Ensure MinGW-w64 toolchain is on PATH so 'make' can spawn gcc/ld.
$env:PATH = "$mingwBin;" + $env:PATH

& make @MakeArgs
exit $LASTEXITCODE
