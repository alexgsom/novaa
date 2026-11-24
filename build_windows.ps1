# Windows PowerShell entry point. MSVC from a Developer PowerShell or g++ in PATH.
[CmdletBinding()]
param([ValidateSet('Auto','MSVC','MinGW')][string]$Compiler='Auto')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $MyInvocation.MyCommand.Path
$bin=Join-Path $root 'bin'
if(-not [IO.Directory]::Exists($bin)){[IO.Directory]::CreateDirectory($bin)|Out-Null}
$out=Join-Path $bin 'nova-native.exe'
$cpp=Join-Path $root 'src\nova_native.cpp'
if($Compiler -eq 'Auto'){
  if(Get-Command cl.exe -ErrorAction SilentlyContinue){$Compiler='MSVC'}
  elseif(Get-Command g++.exe -ErrorAction SilentlyContinue){$Compiler='MinGW'}
  else{throw 'No Windows C++17 compiler in PATH. Open a VS Developer PowerShell or install MSYS2 MinGW-w64.'}
}
$gui=Join-Path $bin 'nova-windowed.exe'
if($Compiler -eq 'MSVC'){
  & cl.exe /nologo /EHsc /std:c++17 /utf-8 /O2 /MT /W4 "/Fe:$out" $cpp /link /SUBSYSTEM:CONSOLE user32.lib gdi32.lib shell32.lib comdlg32.lib winhttp.lib
  if($LASTEXITCODE -ne 0){throw "C++ console compilation failed: $LASTEXITCODE"}
  & cl.exe /nologo /EHsc /std:c++17 /utf-8 /O2 /MT /W4 /DNOVA_PORTABLE_GUI "/Fe:$gui" $cpp /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib shell32.lib comdlg32.lib winhttp.lib
}else{
  & g++.exe -std=c++17 -O2 -Wall -Wextra -Wpedantic $cpp -o $out -luser32 -lgdi32 -lshell32 -lcomdlg32 -lwinhttp
  if($LASTEXITCODE -ne 0){throw "C++ console compilation failed: $LASTEXITCODE"}
  & g++.exe -std=c++17 -O2 -DNOVA_PORTABLE_GUI -mwindows $cpp -o $gui -luser32 -lgdi32 -lshell32 -lcomdlg32 -lwinhttp
}
if($LASTEXITCODE -ne 0){throw "C++ GUI compilation failed: $LASTEXITCODE"}
Write-Output "Built console runtime: $out"
Write-Output "Built Windows GUI runner: $gui"

# Nova Studio 1.0: optional Python-free native source editor.
$studio=Join-Path $root 'studio\nova_studio.cpp'
$studioOut=Join-Path $bin 'NovaStudio.exe'
if($Compiler -eq 'MSVC'){
  & cl.exe /nologo /EHsc /std:c++17 /utf-8 /O2 /MT /W4 "/Fe:$studioOut" $studio /link /SUBSYSTEM:WINDOWS user32.lib comdlg32.lib shell32.lib
}else{
  & g++.exe -std=c++17 -O2 -Wall -Wextra -Wpedantic -municode -mwindows $studio -o $studioOut -luser32 -lcomdlg32 -lshell32
}
if($LASTEXITCODE -ne 0){throw "Nova Studio Windows compilation failed: $LASTEXITCODE"}
Write-Output "Built native Nova Studio: $studioOut"
