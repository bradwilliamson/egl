@echo off
setlocal enabledelayedexpansion
set FAILED=0

REM Run each test executable individually
set TEST_LIST=test_mathlib test_byteswap test_net_msg test_string test_crc test_infostrings test_sdl_input test_cvar test_cmd test_keys test_com_parse test_resolution test_sound_spatialization test_sound_channel test_net_stringtoaddr test_netchan test_netchan_oob test_memory test_playerstate test_inventory test_pmove_clipvelocity test_pmove_friction test_pmove_accelerate test_pmove_airaccelerate test_pmove_quantization test_bounds test_plane

echo ========================================
echo Running all unit tests...
echo ========================================

pushd tests

for %%t in (test_mathlib test_byteswap test_net_msg test_string test_crc test_infostrings test_sdl_input test_cvar test_cmd test_keys test_com_parse test_resolution test_sound_spatialization test_sound_channel test_net_stringtoaddr test_netchan test_netchan_oob test_memory test_playerstate test_inventory test_pmove_clipvelocity test_pmove_friction test_pmove_accelerate test_pmove_airaccelerate test_pmove_quantization test_bounds test_plane) do (
    echo.
    echo Running %%t ...
    .\%%t
    if errorlevel 1 set FAILED=1
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
