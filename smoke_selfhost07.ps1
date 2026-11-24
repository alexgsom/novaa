# Native Nova compiler self-hosting / Windows x64 acceptance test.
# Run after build_windows.ps1 -Compiler MSVC in Developer PowerShell.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$native = Join-Path $root 'bin\nova-native.exe'
$compiler = Join-Path $root 'compiler\novac.nova'
$example = Join-Path $root 'examples\bootstrap\objects_modules.nova'
if (-not (Test-Path -LiteralPath $native)) { throw 'Build nova-native.exe before running self-host smoke test' }
$temp = Join-Path ([IO.Path]::GetTempPath()) ('Nova-selfhost-Україна-'+[guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($temp) | Out-Null
try {
  $a = Join-Path $temp 'novac-A.nbc'
  $b = Join-Path $temp 'novac-B.nbc'
  $sample = Join-Path $temp 'sample.nbc'
  $fromExe = Join-Path $temp 'sample-from-exe.nbc'
  & $native run $compiler --max-steps 15000000 -- $compiler $a
  if ($LASTEXITCODE -ne 0) { throw 'Stage-0 did not compile the compiler' }
  & $native exec $a --max-steps 15000000 -- $compiler $b
  if ($LASTEXITCODE -ne 0) { throw 'Stage-1 compiler could not compile itself' }
  if ((Get-FileHash -Algorithm SHA256 -LiteralPath $a).Hash -ne (Get-FileHash -Algorithm SHA256 -LiteralPath $b).Hash) {
    throw 'Self-hosting identity FAILED: stage-1 and stage-2 bytecode differ'
  }
  & $native exec $b --max-steps 3000000 -- $example $sample
  if ($LASTEXITCODE -ne 0) { throw 'Stage-2 compiler failed on the Nova demo' }
  $aOutput = & $native run $example
  if ($LASTEXITCODE -ne 0) { throw 'Source runtime demo failed' }
  $bOutput = & $native exec $sample
  if ($LASTEXITCODE -ne 0 -or ($bOutput -join "`n") -ne ($aOutput -join "`n")) { throw 'Bytecode execution differs from the source interpreter' }
  $exe = Join-Path $root 'bin\novac-selfhost.exe'
  & (Join-Path $root 'build_bytecode_windows.ps1') -Source $b -Output $exe -Compiler MSVC
  if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $exe)) { throw 'Failed to build a native Windows compiler executable' }
  & $exe --max-steps 3000000 -- $example $fromExe
  if ($LASTEXITCODE -ne 0) { throw 'Standalone self-hosted compiler.exe failed' }
  if ((Get-FileHash -Algorithm SHA256 -LiteralPath $sample).Hash -ne (Get-FileHash -Algorithm SHA256 -LiteralPath $fromExe).Hash) {
    throw 'Standalone compiler generated different bytecode'
  }
  Write-Output 'PASS: Windows Nova self-hosted A -> B -> compiler EXE -> demo, byte-identical output.'
} finally {
  Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
