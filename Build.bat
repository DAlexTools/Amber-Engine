@echo off
setlocal

cd /d "%~dp0"

where pwsh >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    set "AMBER_POWERSHELL=pwsh"
) else (
    set "AMBER_POWERSHELL=powershell"
)

%AMBER_POWERSHELL% -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Build.ps1" %*
set "AMBER_BUILD_EXIT_CODE=%ERRORLEVEL%"

echo.
if not "%AMBER_BUILD_NO_PAUSE%"=="1" (
    pause
)

exit /b %AMBER_BUILD_EXIT_CODE%
