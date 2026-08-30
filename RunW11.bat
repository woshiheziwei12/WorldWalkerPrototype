@echo off
setlocal

set "W11_SCRIPT=%~dp0Scripts\RunW11.ps1"

if not exist "%W11_SCRIPT%" (
    echo [W11] Script not found: "%W11_SCRIPT%"
    pause
    exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%W11_SCRIPT%" %*
set "W11_EXIT_CODE=%ERRORLEVEL%"

if not "%W11_EXIT_CODE%"=="0" (
    echo.
    echo [W11] Build or launch failed. Exit code: %W11_EXIT_CODE%
    pause
)

exit /b %W11_EXIT_CODE%

