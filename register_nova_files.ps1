# Optional per-user Windows Explorer context-menu action; never changes the default .nova association.
[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $MyInvocation.MyCommand.Path
$studio=Join-Path $root 'bin\NovaStudio.exe'
if (-not (Test-Path -LiteralPath $studio -PathType Leaf)) { throw 'Missing bin\NovaStudio.exe. Install the compiled Nova Studio first.' }
$key='HKCU:\Software\Classes\SystemFileAssociations\.nova\shell\NovaStudio'
$command=Join-Path $key 'command'
New-Item -Path $key -Force | Out-Null
Set-Item -Path $key -Value 'Edit with Nova Studio'
New-Item -Path $command -Force | Out-Null
Set-Item -Path $command -Value ('"'+$studio+'" "%1"')
Write-Output 'Installed per-user "Edit with Nova Studio" context menu for .nova files.'
Write-Output 'Default .nova file association was not changed.'
