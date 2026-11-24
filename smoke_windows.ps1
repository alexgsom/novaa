# Run in Windows PowerShell 5.1+ after compiling; checks Unicode file paths and EXE packaging.
[CmdletBinding()]
param([string] $Exe = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
if (-not $Exe) { $Exe = Join-Path $root 'bin\nova-native.exe' }
if (-not (Test-Path -LiteralPath $Exe)) { throw "Missing executable: $Exe" }
$temp = Join-Path ([IO.Path]::GetTempPath()) ('Nova Україна Żółw ' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($temp) | Out-Null
try {
  $source = Join-Path $temp 'Привіт-żółw.nova'
  [IO.File]::WriteAllText($source, "import storage`r`nshow storage.write(`"запис.txt`", `"Україна Żółw`")`r`nshow storage.read(`"запис.txt`")`r`n", [Text.UTF8Encoding]::new($true))
  Push-Location $temp
  try {
    $result = & $Exe run $source 2>&1
    if ($LASTEXITCODE -ne 0) { throw "Unicode Windows run failed: $result" }
    if (($result -join "`n") -notmatch 'Україна Żółw') { throw "Unexpected Unicode output: $result" }
    $built = Join-Path $temp 'Нотатник Nova.exe'
    & (Join-Path $root 'build_app_windows.ps1') -Source $source -Output $built
    if ($LASTEXITCODE -ne 0) { throw 'Embedded EXE build failed' }
    Remove-Item -LiteralPath $source -Force
    $standalone = & $built 2>&1
    if ($LASTEXITCODE -ne 0) { throw "Standalone EXE failed: $standalone" }
    if (($standalone -join "`n") -notmatch 'Україна Żółw') { throw "Standalone Unicode mismatch: $standalone" }
    $guiScript = Join-Path $root 'examples\windows_ui_smoke.nova'
    $guiSmoke = & $Exe run $guiScript 2>&1
    if ($LASTEXITCODE -ne 0 -or (($guiSmoke -join "`n") -notmatch 'Win32 UI smoke OK')) {
      throw "Win32 GUI smoke failed: $guiSmoke"
    }
    # A GUI-subsystem EXE must run without Python or the .nova file.
    $windowed = Join-Path $temp 'Win32 Smoke Windowed.exe'
    & (Join-Path $root 'build_app_windows.ps1') -Source $guiScript -Output $windowed -Windowed
    if ($LASTEXITCODE -ne 0) { throw 'Windowed EXE build failed' }
    $marker = Join-Path $temp 'nova_win32_smoke_ok.txt'
    if (Test-Path -LiteralPath $marker) { Remove-Item -LiteralPath $marker -Force }
    $proc = Start-Process -FilePath $windowed -WorkingDirectory $temp -PassThru
    if (-not $proc.WaitForExit(15000)) {
      $proc.Kill()
      throw 'Windowed EXE failed to exit within 15 seconds'
    }
    if ($proc.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $marker)) {
      throw "Windowed EXE did not finish correctly (exit code $($proc.ExitCode))"
    }
    if ([IO.File]::ReadAllText($marker) -ne 'ok') { throw 'Windowed EXE wrote unexpected marker' }
    # Nested user modules are bundled into a standalone EXE on Windows, too.
    $project = Join-Path $temp 'Multifile'
    [IO.Directory]::CreateDirectory((Join-Path $project 'lib')) | Out-Null
    [IO.File]::WriteAllText((Join-Path $project 'main.nova'), "import `"lib/helper.nova`" as h`nshow h.greet()`n", [Text.UTF8Encoding]::new($false))
    [IO.File]::WriteAllText((Join-Path $project 'lib/helper.nova'), "function greet()`n return `"Привіт Windows`"`nend`n", [Text.UTF8Encoding]::new($false))
    $package = Join-Path $temp 'Multifile Nova.exe'
    & (Join-Path $root 'build_app_windows.ps1') -Source (Join-Path $project 'main.nova') -Output $package -Compiler MSVC
    if ($LASTEXITCODE -ne 0) { throw 'Multifile Windows EXE build failed' }
    Remove-Item -LiteralPath $project -Recurse -Force
    $packOutput = & $package 2>&1
    if ($LASTEXITCODE -ne 0 -or (($packOutput -join "`n") -notmatch 'Привіт Windows')) {
      throw "Bundled Windows module failed: $packOutput"
    }
    Write-Output 'Windows smoke checks passed: Unicode, embedded EXE, Win32 UI, windowed EXE, bundled nested modules'

  } finally { Pop-Location }
} finally { Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue }
