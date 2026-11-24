@echo off
setlocal
set "ROOT=%~dp0"
if /I "%~1"=="build" goto scripted
if /I "%~1"=="compile" goto scripted
if /I "%~1"=="doctor" goto scripted
if /I "%~1"=="inspect" goto scripted
if /I "%~1"=="unpack" goto scripted
"%ROOT%bin\nova-native.exe" %*
exit /b %errorlevel%
:scripted
powershell.exe -NoProfile -File "%ROOT%nova.ps1" %*
exit /b %errorlevel%
