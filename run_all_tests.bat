@echo off
setlocal enabledelayedexpansion
set FAILED=0

REM Run each test executable individually
set TEST_LIST=test_mathlib test_byteswap test_serverbrowser_ui test_favorites_persistence test_config_write test_net_msg test_string test_crc test_infostrings test_sdl_input test_cvar test_cmd test_keys test_com_parse test_resolution test_sound_spatialization test_sound_channel test_net_stringtoaddr test_netchan test_netchan_oob test_memory test_playerstate test_inventory test_pmove_clipvelocity test_pmove_friction test_pmove_accelerate test_pmove_airaccelerate test_pmove_quantization test_bounds test_plane

echo ========================================
echo Running all unit tests...
echo ========================================

REM Guardrails that catch prior regressions early
powershell -NoProfile -ExecutionPolicy Bypass -File tests\check_no_joy_rumble_injection.ps1
if errorlevel 1 set FAILED=1

powershell -NoProfile -ExecutionPolicy Bypass -File tests\check_download_options_menu.ps1
if errorlevel 1 set FAILED=1

pushd tests

REM Build tests if make is available (needed for test_sdl_input, etc.)
powershell -NoProfile -ExecutionPolicy Bypass -Command "$m = Get-Command make -ErrorAction SilentlyContinue; $root = $env:EGL_MSYS2_ROOT; if (-not $root) { $root = 'C:\msys64' }; $prefix = $env:EGL_MSYS2_PREFIX; if (-not $prefix) { $prefix = 'mingw64' }; $bin = Join-Path $root (Join-Path $prefix 'bin'); $gcc = Join-Path $bin 'gcc.exe'; if ($null -ne $m -and (Test-Path $gcc)) { Write-Host ('Building unit tests with ' + $gcc); $env:PATH = $bin + ';' + $env:PATH; & make clean all CC=$gcc; exit $LASTEXITCODE } else { Write-Host ('SKIP: make or MSYS2 gcc not found (' + $gcc + ')'); exit 0 }"
if errorlevel 1 set FAILED=1

for %%t in (test_mathlib test_byteswap test_serverbrowser_ui test_favorites_persistence test_config_write test_net_msg test_string test_crc test_infostrings test_sdl_input test_cvar test_cmd test_keys test_com_parse test_resolution test_sound_spatialization test_sound_channel test_net_stringtoaddr test_netchan test_netchan_oob test_memory test_playerstate test_inventory test_pmove_clipvelocity test_pmove_friction test_pmove_accelerate test_pmove_airaccelerate test_pmove_quantization test_bounds test_plane) do (
    echo.
    echo Running %%t ...
    if exist .\%%t.exe (
        .\%%t.exe
        if errorlevel 1 set FAILED=1
    ) else if exist .\%%t (
        .\%%t
        if errorlevel 1 set FAILED=1
    ) else (
        echo SKIP: %%t not built
    )
)

echo.
if %FAILED%==0 (
    echo =======================================
    echo All test suites passed!
    echo =======================================
) else (
    echo =======================================
    echo SOME TESTS FAILED
    echo =======================================
    exit /b 1
)

popd
endlocal
