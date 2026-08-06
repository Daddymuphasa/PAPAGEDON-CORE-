@echo off
title PAPAGEDON CORE
cd /d "%~dp0"
echo.
echo   Starting PAPAGEDON...
echo.
build\apps\player\papagedon-player.exe %*
echo.
if errorlevel 1 (
    echo   PAPAGEDON exited with an error.
) else (
    echo   PAPAGEDON closed.
)
echo.
pause
