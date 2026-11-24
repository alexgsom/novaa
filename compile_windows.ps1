# Build the Nova-authored stage-1 compiler once, then compile Nova source to .nbc.
[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$Source,
  [Parameter(Mandatory=$true)][string]$Output,
  [ValidateSet('Auto','MSVC','MinGW')][string]$Compiler='Auto'
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $MyInvocation.MyCommand.Path
$novac=Join-Path $root 'bin\novac.exe'
if (-not (Test-Path -LiteralPath $novac)) {
  & (Join-Path $root 'build_app_windows.ps1') -Source (Join-Path $root 'compiler\novac.nova') -Output $novac -Compiler $Compiler
  if ($LASTEXITCODE -ne 0) { throw 'Unable to build Nova stage-1 compiler' }
}
& $novac $Source $Output
if ($LASTEXITCODE -ne 0) { throw "Nova stage-1 compilation failed: $LASTEXITCODE" }
