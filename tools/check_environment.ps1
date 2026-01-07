# tools/check_environment.ps1
# Verify build environment on Windows (MSYS2 / MinGW-w64)
# Usage: Open a MinGW/MSYS2 shell or PowerShell with MinGW in PATH and run:
#   .\tools\check_environment.ps1

function Check-Command($cmd) {
    $found = Get-Command $cmd -ErrorAction SilentlyContinue
    if ($null -ne $found) {
        Write-Host "OK: $cmd -> $($found.Path)"
        return $true
    } else {
        Write-Host "MISSING: $cmd"
        return $false
    }
}

$allOK = $true
$allOK = (Check-Command gcc) -and $allOK
$allOK = (Check-Command make) -and $allOK
$allOK = (Check-Command pacman) -and $allOK
$allOK = (Check-Command tar) -and $allOK
$allOK = (Check-Command pkg-config) -and $allOK
$allOK = (Check-Command unzip) -and $allOK

Write-Host ""
if ($allOK) {
    Write-Host "Environment looks OK. Try: .\tools\build.ps1"
    exit 0
} else {
    Write-Host "Some required tools are missing. Install MSYS2 and the packages shown below (run in MSYS2 MINGW64 shell):"
    Write-Host "  pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-zlib mingw-w64-x86_64-pkg-config mingw-w64-x86_64-minizip"
    exit 1
}