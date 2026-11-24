# Nova 0.8: build Windows .exe from .nova without C++/Visual Studio on the user's computer.
# Requires a PREBUILT Windows bin/nova-native.exe and compiler/novac_selfhost.nbc.
[CmdletBinding()]
param(
  [Parameter(Position=0, Mandatory=$true)]
  [ValidateSet('build','compile','run','inspect','unpack','doctor')][string]$Command,
  [Parameter(Position=1)][string]$Source,
  [Parameter(Position=2)][string]$Output,
  [string]$Runtime,
  [switch]$Windowed,
  [switch]$Force
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $MyInvocation.MyCommand.Path
$cli=Join-Path $root 'bin\nova-native.exe'
if (-not (Test-Path -LiteralPath $cli -PathType Leaf)) {
  throw "Missing prebuilt Windows compiler/packaging runtime: $cli. Download the Windows artifact or build it once on a developer computer."
}
$runner=(Resolve-Path -LiteralPath $cli).ProviderPath
if ([string]::IsNullOrWhiteSpace($Runtime)) {
  if ($Windowed) { $Runtime=Join-Path $root 'bin\nova-windowed.exe' }
  else { $Runtime=$cli }
}
if ($Command -eq 'build' -and -not (Test-Path -LiteralPath $Runtime -PathType Leaf)) { throw "Missing application runner: $Runtime" }
if ($Command -eq 'build' -and $Windowed) { Write-Verbose 'Using native Win32 bytecode GUI runner for .nova UI application.' }
$compiler=Join-Path $root 'compiler\novac_selfhost.nbc'
if (($Command -eq 'build' -or $Command -eq 'compile' -or $Command -eq 'doctor') -and -not (Test-Path -LiteralPath $compiler)) {
  throw "Missing self-hosted Nova compiler bytecode: $compiler"
}
function Run-Nova([string[]]$Arguments) {
  & $runner @Arguments
  if ($LASTEXITCODE -ne 0) { throw "Nova exited with code $LASTEXITCODE" }
}
if ($Command -eq 'doctor') {
  $tmpdir=Join-Path ([IO.Path]::GetTempPath()) ('Nova-Doctor-Łódź-'+[guid]::NewGuid().ToString('N'))
  [IO.Directory]::CreateDirectory($tmpdir)|Out-Null
  try {
    $sourceFile=Join-Path $tmpdir 'hello.nova'
    $bytecode=Join-Path $tmpdir 'hello.nbc'
    $program=Join-Path $tmpdir 'hello.exe'
    [IO.File]::WriteAllText($sourceFile,"show 12`n",[Text.UTF8Encoding]::new($false))
    Run-Nova -Arguments @('exec',$compiler,'--max-steps','15000000','--',$sourceFile,$bytecode) | Out-Null
    $vmOutput=& $runner exec $bytecode
    if($LASTEXITCODE -ne 0 -or ($vmOutput -join "`n").Trim() -ne '12') { throw 'Nova VM check failed' }
    Run-Nova -Arguments @('pack',$bytecode,$program,'--runtime',$runner) | Out-Null
    $exeOutput=& $program
    if($LASTEXITCODE -ne 0 -or ($exeOutput -join "`n").Trim() -ne '12') { throw 'Portable EXE check failed' }
    $info=& $runner inspect $program
    if($LASTEXITCODE -ne 0 -or ($info -join "`n") -notmatch 'Integrity: OK') { throw 'Package integrity check failed' }
    Write-Output 'PASS: Windows native runtime, Nova self-hosted compiler, VM, portable EXE'
    if(-not (Test-Path -LiteralPath (Join-Path $root 'bin\nova-windowed.exe'))) {
      Write-Warning 'Optional GUI runner not installed (nova-windowed.exe).'
    }
  } finally { Remove-Item -LiteralPath $tmpdir -Recurse -Force -ErrorAction SilentlyContinue }
} elseif ($Command -eq 'build' -or $Command -eq 'compile') {
  if ([string]::IsNullOrWhiteSpace($Source)) { throw 'Specify -Source main.nova' }
  $inputPath=(Resolve-Path -LiteralPath $Source).ProviderPath
  if (-not $inputPath.EndsWith('.nova',[StringComparison]::OrdinalIgnoreCase)) { throw 'Source must be a .nova file' }
  if ([string]::IsNullOrWhiteSpace($Output)) {
    $extension=if ($Command -eq 'build') {'.exe'} else {'.nbc'}
    $Output=[IO.Path]::ChangeExtension($inputPath,$extension)
  }
  $dest=[IO.Path]::GetFullPath($Output)
  if ($Command -eq 'build' -and -not $dest.EndsWith('.exe',[StringComparison]::OrdinalIgnoreCase)) { throw 'Windows builds must output .exe' }
  if ($Command -eq 'compile' -and -not $dest.EndsWith('.nbc',[StringComparison]::OrdinalIgnoreCase)) { throw 'Compiler output must end in .nbc' }
  $parent=[IO.Path]::GetDirectoryName($dest)
  if (-not [IO.Directory]::Exists($parent)) {[IO.Directory]::CreateDirectory($parent)|Out-Null}
  if ((Test-Path -LiteralPath $dest) -and -not $Force) { throw 'Output already exists (use -Force)' }
  $tmpdir=Join-Path ([IO.Path]::GetTempPath()) ('nova-build-'+[guid]::NewGuid().ToString('N'))
  [IO.Directory]::CreateDirectory($tmpdir)|Out-Null
  try {
    $bc=Join-Path $tmpdir 'compiled.nbc'
    Run-Nova -Arguments @('exec', $compiler, '--max-steps', '15000000', '--', $inputPath, $bc)
    if ($Command -eq 'compile') {
      if (Test-Path -LiteralPath $dest) {Remove-Item -LiteralPath $dest -Force}
      Move-Item -LiteralPath $bc -Destination $dest
      Write-Output "COMPILED: $dest"
    } else {
      $packArgs=@('pack', $bc, $dest, '--runtime', (Resolve-Path -LiteralPath $Runtime).ProviderPath)
      if ($Force) {$packArgs+='--force'}
      Run-Nova -Arguments $packArgs
    }
  } finally { Remove-Item -LiteralPath $tmpdir -Recurse -Force -ErrorAction SilentlyContinue }
} elseif ($Command -eq 'run') {
  if ([string]::IsNullOrWhiteSpace($Source)) {throw 'Specify -Source file.nova or file.nbc'}
  $src=(Resolve-Path -LiteralPath $Source).ProviderPath
  if ($src.EndsWith('.nbc',[StringComparison]::OrdinalIgnoreCase)) {Run-Nova -Arguments @('exec',$src)}
  else {Run-Nova -Arguments @('run',$src)}
} elseif ($Command -eq 'inspect') {
  if ([string]::IsNullOrWhiteSpace($Source)) {throw 'Specify -Source MyApp.exe'}
  Run-Nova -Arguments @('inspect',(Resolve-Path -LiteralPath $Source).ProviderPath)
} elseif ($Command -eq 'unpack') {
  if ([string]::IsNullOrWhiteSpace($Source) -or [string]::IsNullOrWhiteSpace($Output)) {throw 'Specify -Source MyApp.exe -Output source.nbc'}
  $args=@('unpack',(Resolve-Path -LiteralPath $Source).ProviderPath,[IO.Path]::GetFullPath($Output))
  if ($Force) {$args+='--force'}
  Run-Nova -Arguments $args
}
