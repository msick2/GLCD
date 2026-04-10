@echo off
rem Configure and build the Pico project in build/.
rem Usage:
rem   build.cmd         - incremental build
rem   build.cmd clean   - wipe build/ and reconfigure

setlocal
set "SCRIPT_DIR=%~dp0"
call "%SCRIPT_DIR%pico-env.cmd" || exit /b 1

if /i "%~1"=="clean" (
  if exist "%SCRIPT_DIR%build" rmdir /s /q "%SCRIPT_DIR%build"
)

if not exist "%SCRIPT_DIR%build" mkdir "%SCRIPT_DIR%build"
pushd "%SCRIPT_DIR%build"
cmake -G Ninja .. || (popd & exit /b 1)
ninja || (popd & exit /b 1)
popd
echo.
echo Build artifacts:
dir /b "%SCRIPT_DIR%build\*.uf2" 2>nul
endlocal
