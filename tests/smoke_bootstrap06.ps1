# Windows/MSVC smoke test for the compiler authored in Nova and the stage-1 VM.
# The outputs from source runtime, compiled VM and a standalone native EXE must match.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$native = Join-Path $root 'bin\nova-native.exe'
$novac  = Join-Path $root 'bin\novac.exe'
if (-not (Test-Path -LiteralPath $native) -or -not (Test-Path -LiteralPath $novac)) {
  throw 'Build nova-native.exe and novac.exe before running this test'
}
$demo = Join-Path $root 'examples\bootstrap\functions_and_lists.nova'
$temp = Join-Path ([IO.Path]::GetTempPath()) ('Nova-Żółw-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
try {
  $src = Join-Path $temp 'Україна.nova'
  $bytecode = Join-Path $temp 'compiled.nbc'
  $exe = Join-Path $root 'bin\NovaBootstrap06Demo.exe'
  Copy-Item -LiteralPath $demo -Destination $src
  $reference = & $native run $src
  if ($LASTEXITCODE -ne 0) { throw 'Stage-0 source interpreter failed' }
  & $novac $src $bytecode | Out-Null
  if ($LASTEXITCODE -ne 0) { throw 'Nova-authored compiler failed' }
  $compiled = & $native exec $bytecode
  if ($LASTEXITCODE -ne 0) { throw 'Stage-1 bytecode execution failed' }
  if (($reference -join "`n") -ne ($compiled -join "`n")) { throw 'Stage-0 vs Stage-1 output mismatch' }
  & (Join-Path $root 'build_bytecode_windows.ps1') -Source $bytecode -Output $exe -Compiler MSVC
  if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $exe)) { throw 'Standalone Windows EXE build failed' }
  Remove-Item -LiteralPath $src, $bytecode -Force
  $standalone = & $exe
  if ($LASTEXITCODE -ne 0) { throw 'Standalone Windows executable failed' }
  if (($reference -join "`n") -ne ($standalone -join "`n")) { throw 'Standalone EXE output mismatch' }
  Write-Output 'OK: Nova-authored recursive functions + for/list compiler -> bytecode -> standalone Windows EXE'
} finally {
  Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
