@echo off
setlocal

cd /d "%~dp0"

where pwsh >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    set "AMBER_POWERSHELL=pwsh"
) else (
    set "AMBER_POWERSHELL=powershell"
)

%AMBER_POWERSHELL% -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Format\Format-Code.ps1" %*
exit /b %ERRORLEVEL%
