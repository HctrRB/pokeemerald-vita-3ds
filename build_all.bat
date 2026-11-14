@echo off
rem Double-click to build EmeraldVita.vpk: runs build_all.ps1 with the right options and
rem keeps the window open at the end so the result (or the error) can be read.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_all.ps1" %*
set BUILD_ALL_EXIT=%ERRORLEVEL%
echo.
pause
exit /b %BUILD_ALL_EXIT%
