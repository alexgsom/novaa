# Compile Nova .nbc bytecode into a standalone native Windows EXE (no Python).
[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$Source,
  [Parameter(Mandatory=$true)][string]$Output,
  [ValidateSet('Auto','MSVC','MinGW')][string]$Compiler = 'Auto'
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $MyInvocation.MyCommand.Path
$inputPath=(Resolve-Path -LiteralPath $Source).ProviderPath
if (-not $inputPath.EndsWith('.nbc', [StringComparison]::OrdinalIgnoreCase)) { throw 'Source must end with .nbc' }
[byte[]]$bytes=[IO.File]::ReadAllBytes($inputPath)
if ($bytes.Length -gt 4MB) { throw 'Bytecode exceeds 4 MiB' }
$outputPath=[IO.Path]::GetFullPath($Output)
if (-not $outputPath.EndsWith('.exe',[StringComparison]::OrdinalIgnoreCase)) { $outputPath+='.exe' }
$outputDir=[IO.Path]::GetDirectoryName($outputPath)
if (-not [IO.Directory]::Exists($outputDir)) { [IO.Directory]::CreateDirectory($outputDir)|Out-Null }
$temp=Join-Path ([IO.Path]::GetTempPath()) ('nova-bc-'+[guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($temp)|Out-Null
try {
  $values=if ($bytes.Length) { ($bytes|ForEach-Object { $_.ToString([Globalization.CultureInfo]::InvariantCulture) }) -join ',' } else { '0' }
  $header="static const unsigned char NOVA_BYTECODE_BYTES[] = {$values};`nstatic const size_t NOVA_BYTECODE_LENGTH = $($bytes.Length);`n"
  [IO.File]::WriteAllText((Join-Path $temp 'nova_embedded_bytecode.hpp'),$header,[Text.Encoding]::ASCII)
  if ($Compiler -eq 'Auto') {
    if (Get-Command cl.exe -ErrorAction SilentlyContinue) { $Compiler='MSVC' }
    elseif (Get-Command g++.exe -ErrorAction SilentlyContinue) { $Compiler='MinGW' }
    else { throw 'C++ compiler not found (use Developer PowerShell for Visual Studio or MinGW-w64)' }
  }
  $cpp=Join-Path $root 'src\nova_native.cpp'
  if ($Compiler -eq 'MSVC') {
    & cl.exe /nologo /EHsc /std:c++17 /utf-8 /O2 /DNOVA_EMBEDDED_BYTECODE_APP "/I$temp" "/Fo:$temp\nova_bytecode.obj" "/Fe:$outputPath" $cpp /link /SUBSYSTEM:CONSOLE user32.lib gdi32.lib shell32.lib comdlg32.lib winhttp.lib
  } else {
    & g++.exe -std=c++17 -O2 -DNOVA_EMBEDDED_BYTECODE_APP "-I$temp" $cpp -o $outputPath -luser32 -lgdi32 -lshell32 -lcomdlg32 -lwinhttp
  }
  if ($LASTEXITCODE -ne 0) { throw "C++ build failed: $LASTEXITCODE" }
  if (-not (Test-Path -LiteralPath $outputPath)) { throw 'Compiler did not produce an EXE' }
  Write-Output "Built native Nova bytecode EXE: $outputPath"
} finally { Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue }
