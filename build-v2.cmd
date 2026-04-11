@echo off
rem Build wrapper for Pico SDK 2.x. Uses build-v2/ as the CMake output dir
rem so it does not collide with the SDK 1.5.1 build/ directory.
rem
rem Usage:
rem   build-v2.cmd               - incremental build (default PICO_BOARD=pico)
rem   build-v2.cmd clean          - wipe build-v2/ and reconfigure
rem   build-v2.cmd pico2          - build for Raspberry Pi Pico 2 (RP2350)
rem   build-v2.cmd pico2 clean    - clean build for Pico 2
rem   build-v2.cmd pico           - build for Pico 1 (RP2040) with SDK 2.x

setlocal
set "SCRIPT_DIR=%~dp0"
call "%SCRIPT_DIR%pico-env-v2.cmd" || exit /b 1

set "BOARD=pico"
set "DO_CLEAN=0"

:parse
if "%~1"=="" goto after_parse
if /i "%~1"=="pico"  set "BOARD=pico"  & shift & goto parse
if /i "%~1"=="pico2" set "BOARD=pico2" & shift & goto parse
if /i "%~1"=="clean" set "DO_CLEAN=1"  & shift & goto parse
echo Unknown argument: %~1
exit /b 1
:after_parse

set "BUILD_DIR=%SCRIPT_DIR%build-v2-%BOARD%"

if "%DO_CLEAN%"=="1" (
  if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
pushd "%BUILD_DIR%"
cmake -G Ninja -DPICO_BOARD=%BOARD% .. || (popd & exit /b 1)
ninja || (popd & exit /b 1)
popd
echo.
echo Build artifacts (%BOARD%):
dir /b "%BUILD_DIR%\*.uf2" 2>nul
endlocal
