@echo off
setlocal
cd /d "%~dp0"
if exist "build\launcher\Debug\course_browser.exe" (
  start "" /b "build\launcher\Debug\course_browser.exe"
  exit /b
)
if exist "build\launcher\Release\course_browser.exe" (
  start "" /b "build\launcher\Release\course_browser.exe"
  exit /b
)
cmake --preset launcher
if errorlevel 1 goto failed
cmake --build --preset launcher-debug --parallel 4
if errorlevel 1 goto failed
start "" /b "build\launcher\Debug\course_browser.exe"
exit /b
:failed
echo Tutorial launcher build failed. See README.md for setup.
pause
