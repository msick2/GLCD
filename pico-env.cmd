@echo off
rem Local wrapper: sets PICO_INSTALL_PATH then calls the installed pico-env.cmd.
rem Use this from any terminal to get CMake, ARM GCC, Ninja, Python, picotool on PATH.
set "PICO_INSTALL_PATH=C:\Program Files\Raspberry Pi\Pico SDK v1.5.1"
call "%PICO_INSTALL_PATH%\pico-env.cmd"
