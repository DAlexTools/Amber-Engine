@echo off
setlocal

cd /d "%~dp0"

where pwsh >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    set "AMBER_POWERSHELL=pwsh"
) else (
    set "AMBER_POWERSHELL=powershell"
)

%AMBER_POWERSHELL% -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Clean.ps1" %*
set "AMBER_CLEAN_EXIT_CODE=%ERRORLEVEL%"

echo.
if "%AMBER_BUILD_NO_PAUSE%"=="1" goto skip_pause
if "%AMBER_CLEAN_NO_PAUSE%"=="1" goto skip_pause
pause

:skip_pause

exit /b %AMBER_CLEAN_EXIT_CODE%
