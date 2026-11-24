# PowerShell 5.1+; build a self-contained native Nova application for Windows.
# Supports MSVC cl.exe in Developer PowerShell and MinGW-w64 g++.exe.
[CmdletBinding()]
param(
  [Parameter(Mandatory = $true)][string] $Source,
  [Parameter(Mandatory = $true)][string] $Output,
  [ValidateSet('Auto','MSVC','MinGW')][string] $Compiler = 'Auto',
  [switch] $Windowed
)
$ErrorActionPreference = 'Stop'
if (-not $Source.EndsWith('.nova', [StringComparison]::OrdinalIgnoreCase)) { throw 'Expected a .nova source file' }
$inputPath = (Resolve-Path -LiteralPath $Source).ProviderPath
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$outputPath = [IO.Path]::GetFullPath($Output)
$outputDir = [IO.Path]::GetDirectoryName($outputPath)
if (-not [IO.Directory]::Exists($outputDir)) { [IO.Directory]::CreateDirectory($outputDir) | Out-Null }
if (-not $outputPath.EndsWith('.exe', [StringComparison]::OrdinalIgnoreCase)) { $outputPath += '.exe' }
$temp = Join-Path ([IO.Path]::GetTempPath()) ('nova-' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($temp) | Out-Null
try {
  [byte[]]$bytes = [IO.File]::ReadAllBytes($inputPath)
  # Produces a C++ header, never interpolates source text into compiled C++.
  $values = if ($bytes.Length) { ($bytes | ForEach-Object { $_.ToString([Globalization.CultureInfo]::InvariantCulture) }) -join ',' } else { '0' }
  $header = "static const unsigned char NOVA_EMBEDDED_CODE[] = {$values};`nstatic const size_t NOVA_EMBEDDED_LENGTH = $($bytes.Length);`n"
  # Package all sibling .nova modules, including nested module folders.
  $projectRoot = [IO.Path]::GetDirectoryName($inputPath)
  $modules = @(Get-ChildItem -LiteralPath $projectRoot -File -Recurse -Filter '*.nova' | Where-Object { $_.FullName -ne $inputPath } | Sort-Object FullName)
  if ($bytes.Length -gt 1MB) { throw 'Main source exceeds 1 MiB' }
  if ($modules.Count -gt 128) { throw 'Maximum 128 project modules' }
  $header += "struct NovaEmbeddedModule { const char* name; const unsigned char* bytes; size_t length; };`n"
  $records = @()
  $totalBytes = $bytes.Length
  for ($i = 0; $i -lt $modules.Count; $i++) {
    $module = $modules[$i]
    [byte[]]$modBytes = [IO.File]::ReadAllBytes($module.FullName)
    if ($modBytes.Length -gt 1MB) { throw "Module exceeds 1 MiB: $($module.Name)" }
    $totalBytes += $modBytes.Length
    if ($totalBytes -gt 8MB) { throw 'Project source exceeds 8 MiB' }
    $relative = $module.FullName.Substring($projectRoot.Length + 1).Replace('\', '/')
    [byte[]]$nameBytes = [Text.Encoding]::UTF8.GetBytes($relative)
    $moduleValues = if ($modBytes.Length) { ($modBytes | ForEach-Object { $_.ToString([Globalization.CultureInfo]::InvariantCulture) }) -join ',' } else { '0' }
    $pathValues = (($nameBytes | ForEach-Object { $_.ToString([Globalization.CultureInfo]::InvariantCulture) }) + '0') -join ','
    $header += "static const unsigned char NOVA_MOD_BYTES_${i}[] = {$moduleValues};`n"
    $header += "static const char NOVA_MOD_PATH_${i}[] = {$pathValues};`n"
    $records += "{NOVA_MOD_PATH_$i, NOVA_MOD_BYTES_$i, sizeof(NOVA_MOD_BYTES_$i)}"
  }
  $header += 'static const NovaEmbeddedModule NOVA_EMBEDDED_MODULES[] = {' + ((@($records) + @('{nullptr,nullptr,0}')) -join ',') + '};' + "`n"
  $header += "static const size_t NOVA_EMBEDDED_MODULE_COUNT = $($modules.Count);`n"
  [IO.File]::WriteAllText((Join-Path $temp 'nova_embedded_app.hpp'), $header, [Text.Encoding]::ASCII)
  $cpp = Join-Path $root 'src\nova_native.cpp'
  if ($Compiler -eq 'Auto') {
    if (Get-Command cl.exe -ErrorAction SilentlyContinue) { $Compiler = 'MSVC' }
    elseif (Get-Command g++.exe -ErrorAction SilentlyContinue) { $Compiler = 'MinGW' }
    else { throw 'No compiler found: use a Visual Studio Developer PowerShell or install MSYS2 MinGW-w64' }
  }
  $guiDefine = if ($Windowed) { '/DNOVA_WINDOWS_GUI' } else { '/DNOVA_WINDOWS_CONSOLE' }
  if ($Compiler -eq 'MSVC') {
    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) { throw 'cl.exe not found; open a Developer PowerShell for VS' }
    $subsystem = if ($Windowed) { '/SUBSYSTEM:WINDOWS' } else { '/SUBSYSTEM:CONSOLE' }
    & cl.exe /nologo /EHsc /std:c++17 /utf-8 /O2 /DNOVA_EMBEDDED_APP $guiDefine "/I$temp" "/Fo:$temp\nova_native.obj" "/Fe:$outputPath" $cpp /link $subsystem user32.lib gdi32.lib shell32.lib comdlg32.lib winhttp.lib
  } else {
    if (-not (Get-Command g++.exe -ErrorAction SilentlyContinue)) { throw 'g++.exe not found' }
    $windowFlags = if ($Windowed) { @('-mwindows','-municode') } else { @() }
    $mingwDefine = if ($Windowed) { '-DNOVA_WINDOWS_GUI' } else { '-DNOVA_WINDOWS_CONSOLE' }
    & g++.exe -std=c++17 -O2 -DNOVA_EMBEDDED_APP $mingwDefine "-I$temp" $cpp -o $outputPath @windowFlags -luser32 -lgdi32 -lshell32 -lcomdlg32 -lwinhttp
  }
  if ($LASTEXITCODE -ne 0) { throw "Compilation failed with exit code $LASTEXITCODE" }
  if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf)) { throw 'Compiler exited without producing an .exe' }
  Write-Output "Built standalone Nova Windows program: $outputPath (windowed: $Windowed)"
} finally {
  Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
