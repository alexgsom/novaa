# Windows acceptance test: run a MOVEABLE distribution outside the source tree.
[CmdletBinding()]
param([string]$Source='dist/windows-x64/Nova', [switch]$SkipGui)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$sourcePath=if([IO.Path]::IsPathRooted($Source)){$Source}else{Join-Path $root $Source}
$sourcePath=(Resolve-Path -LiteralPath $sourcePath).ProviderPath
$tmp=Join-Path ([IO.Path]::GetTempPath()) ('nova-installed-Україна-Łódź-'+[guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($tmp)|Out-Null
try {
  $target=Join-Path $tmp 'Nova Portable'
  Copy-Item -LiteralPath $sourcePath -Destination $target -Recurse -Force
  if(-not (Test-Path -LiteralPath (Join-Path $target 'bin/NovaStudio.exe'))) {throw 'Copied distribution is missing Studio'}
  $manifest=Get-Content -LiteralPath (Join-Path $target 'SHA256_FILES.txt') -Encoding UTF8
  foreach($row in $manifest){
    if ($row -notmatch '^([a-f0-9]{64}) \*(.+)$'){throw "Malformed manifest line: $row"}
    $expected=$Matches[1]
    $relative=$Matches[2]
    $path=Join-Path $target $relative
    if(-not (Test-Path -LiteralPath $path -PathType Leaf)){throw "Package missing hashed file $relative"}
    $actual=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if($actual -ne $expected){throw "SHA256 mismatch: $relative"}
  }
  Push-Location $target
  try {
    & (Join-Path $target 'nova.ps1') doctor
    $studio=Join-Path $target 'bin/NovaStudio.exe'
    foreach($mode in @('--self-test','--gui-smoke')) {
      if($mode -eq '--gui-smoke' -and $SkipGui){continue}
      $proc=Start-Process -FilePath $studio -ArgumentList $mode -PassThru
      try {
        if (-not $proc.WaitForExit(15000)) {throw "Nova Studio $mode timed out"}
        if ($proc.ExitCode -ne 0) {throw "Nova Studio $mode failed with exit $($proc.ExitCode)"}
      } finally {
        if (-not $proc.HasExited) {Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue}
        $proc.Dispose()
      }
    }
    $program=Join-Path $tmp 'Привіт.exe'
    $script=Join-Path $tmp 'Привіт.nova'
    [IO.File]::WriteAllText($script,"show `"Київ Łódź`"`n",[Text.UTF8Encoding]::new($false))
    & (Join-Path $target 'nova.ps1') build -Source $script -Output $program
    if(-not(Test-Path -LiteralPath $program)){throw 'Compilerless EXE build missing'}
    Remove-Item -LiteralPath $script -Force
    $actual=& $program
    if ($LASTEXITCODE -ne 0 -or ($actual -join "`n").Trim() -ne 'Київ Łódź') {throw "Portable program failed: $actual"}
    & (Join-Path $target 'nova.ps1') inspect -Source $program | Out-Null
    if (-not $SkipGui) { & (Join-Path $target 'tests/smoke_win32_click.ps1') -NovaRoot $target }
    Write-Output 'PASS: relocatable Nova installation, doctor, Studio, compilerless EXE and GUI callback'
  } finally {Pop-Location}
} finally {
  Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue
}
