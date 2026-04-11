@echo off
rem SDK 2.x wrapper. Points CMake at Pico SDK 2.1.1 in E:\pico-sdk-2\sdk
rem and uses the new picotool 2.x from E:\pico-sdk-2\tools\picotool.
rem
rem The toolchain (ARM GCC 10.3), CMake, Ninja, and Python still come from
rem the original pico-setup-windows v1.5.1 install, so we just call that
rem first and then override the SDK-specific paths.

set "PICO_INSTALL_PATH=C:\Program Files\Raspberry Pi\Pico SDK v1.5.1"
call "%PICO_INSTALL_PATH%\pico-env.cmd" || exit /b 1

rem --- Override with SDK 2.x locations ---
set "PICO_SDK_PATH=E:\pico-sdk-2\sdk"
set "PICO_SDK_VERSION=2.1.1"

rem Put picotool 2.x first so CMake's find_program picks it up instead of 1.1.2
set "PATH=E:\pico-sdk-2\tools\picotool;%PATH%"
set "picotool_DIR=E:\pico-sdk-2\tools\picotool"

echo.
echo === Pico SDK 2.x environment active ===
echo PICO_SDK_PATH=%PICO_SDK_PATH%
picotool version 2>nul | findstr /C:"picotool"
