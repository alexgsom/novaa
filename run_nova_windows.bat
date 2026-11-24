@echo off
setlocal
if "%~1"=="" (
  echo Drag a .nova file onto this launcher or run:
  echo   run_nova_windows.bat path\to\my_app.nova
  pause
  exit /b 2
)
if not exist "%~dp0bin\nova-native.exe" (
  echo Nova not built. Run build_windows.bat first.
  pause
  exit /b 2
)
"%~dp0bin\nova-native.exe" run "%~1"
echo.
if errorlevel 1 echo The Nova program reported an error.
pause
