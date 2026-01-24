param(
    [Parameter(Mandatory = $false)]
    [string]$DumpPath = "EGL.DMP",

    [Parameter(Mandatory = $false)]
    [string]$OutLog = "dump_analysis.txt"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $DumpPath)) {
    throw "Dump not found: $DumpPath"
}

$cdb = Get-Command cdb -ErrorAction SilentlyContinue
if (-not $cdb) {
    Write-Host "cdb.exe not found in PATH." -ForegroundColor Yellow
    Write-Host "Install WinDbg (Preview) or Debugging Tools for Windows, then re-run." -ForegroundColor Yellow
    Write-Host "  WinDbg Preview (Store): https://aka.ms/windbg" -ForegroundColor Yellow
    Write-Host "  Windows SDK (Debugging Tools): https://developer.microsoft.com/windows/downloads/windows-sdk/" -ForegroundColor Yellow
    exit 2
}

$dumpFull = (Resolve-Path -LiteralPath $DumpPath).Path
$outFull = (Resolve-Path -LiteralPath (Split-Path -Parent $dumpFull)).Path + "\\" + $OutLog

# Symbol path: Microsoft public symbols + local folder (for egl.pdb, dll pdbs if present)
$localSym = (Resolve-Path -LiteralPath (Split-Path -Parent $dumpFull)).Path

$commands = @(
    ".symfix",
    ".sympath+ $localSym",
    ".reload /f",
    ".time",
    "|",
    "!analyze -v",
    "~* kp 200",
    "q"
) -join "; "

Write-Host "Analyzing dump: $dumpFull"
Write-Host "Writing log:     $outFull"

& $cdb.Source @(
    "-z", $dumpFull,
    "-logo", $outFull,
    "-c", $commands
)
