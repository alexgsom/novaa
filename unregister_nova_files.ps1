# Undo only the Nova Studio context menu installed by register_nova_files.ps1.
[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$key='HKCU:\Software\Classes\SystemFileAssociations\.nova\shell\NovaStudio'
if (Test-Path -LiteralPath $key) {
  Remove-Item -LiteralPath $key -Recurse -Force
  Write-Output 'Removed Nova Studio context menu.'
} else { Write-Output 'Nova Studio context menu is not installed.' }
