@echo off
setlocal
cd /d "%~dp0"
if not exist "bin" mkdir "bin"
where cl.exe >nul 2>nul
if not errorlevel 1 goto build_msvc
where g++.exe >nul 2>nul
if not errorlevel 1 goto build_mingw
echo ERROR: No C++17 compiler found.
echo Install Visual Studio Build Tools (Developer Command Prompt), or MSYS2 MinGW-w64.
echo Users of prebuilt Windows binaries do NOT need this script or a C++ compiler.
exit /b 2
:build_msvc
cl /nologo /EHsc /std:c++17 /utf-8 /O2 /MT /W4 "src\nova_native.cpp" /Fe:"bin\nova-native.exe" /link /SUBSYSTEM:CONSOLE user32.lib gdi32.lib shell32.lib comdlg32.lib winhttp.lib
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++17 /utf-8 /O2 /MT /DNOVA_PORTABLE_GUI "src\nova_native.cpp" /Fe:"bin\nova-windowed.exe" /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib shell32.lib comdlg32.lib winhttp.lib
if errorlevel 1 exit /b 1
echo Built: bin\nova-native.exe and bin\nova-windowed.exe
exit /b 0
:build_mingw
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic "src\nova_native.cpp" -o "bin\nova-native.exe" -luser32 -lgdi32 -lshell32 -lcomdlg32 -lwinhttp
if errorlevel 1 exit /b 1
g++ -std=c++17 -O2 -DNOVA_PORTABLE_GUI -mwindows "src\nova_native.cpp" -o "bin\nova-windowed.exe" -luser32 -lgdi32 -lshell32 -lcomdlg32 -lwinhttp
if errorlevel 1 exit /b 1
echo Built: bin\nova-native.exe and bin\nova-windowed.exe
exit /b 0
