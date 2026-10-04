@echo off
title PAPAGEDON CORE
cd /d "%~dp0"
echo.
echo   Starting PAPAGEDON...
echo.
where cmake >nul 2>nul
if errorlevel 1 (
    echo   CMake is needed to rebuild the current source before launch.
    pause
    exit /b 1
)
cmake --build build --target papagedon-player --parallel 4
if errorlevel 1 (
    echo   Build failed. Close any running PAPAGEDON player and retry.
    pause
    exit /b 1
)
build\apps\player\papagedon-player.exe %*
echo.
if errorlevel 1 (
    echo   PAPAGEDON exited with an error.
) else (
    echo   PAPAGEDON closed.
)
echo.
pause
