# Remove only known Nova Native files from current-user installation.
[CmdletBinding()]
param([switch]$RemoveFromPath, [switch]$RemoveStartMenu, [switch]$RemoveExplorerMenu)
$ErrorActionPreference='Stop'
$target=Join-Path $env:LOCALAPPDATA 'Programs\NovaNative'
if($RemoveExplorerMenu){
  $key='HKCU:\Software\Classes\SystemFileAssociations\.nova\shell\NovaStudio'
  if(Test-Path -LiteralPath $key){Remove-Item -LiteralPath $key -Recurse -Force}
}
if($RemoveStartMenu){
  $shortcut=Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Nova Studio.lnk'
  if(Test-Path -LiteralPath $shortcut){Remove-Item -LiteralPath $shortcut -Force}
}
foreach($relative in @(
  'nova-native.exe','nova.cmd','nova.ps1',
  'register_nova_files.ps1','unregister_nova_files.ps1','uninstall_windows.ps1',
  'bin\nova-native.exe','bin\nova-windowed.exe','bin\NovaStudio.exe',
  'compiler\novac_selfhost.nbc'
)){
  $path=Join-Path $target $relative
  if(Test-Path -LiteralPath $path -PathType Leaf){Remove-Item -LiteralPath $path -Force}
}
foreach($folder in @('compiler','bin')){
  $path=Join-Path $target $folder
  if((Test-Path -LiteralPath $path -PathType Container) -and -not (Get-ChildItem -LiteralPath $path -Force | Select-Object -First 1)){
    Remove-Item -LiteralPath $path -Force
  }
}
if((Test-Path -LiteralPath $target -PathType Container) -and -not (Get-ChildItem -LiteralPath $target -Force | Select-Object -First 1)){
  Remove-Item -LiteralPath $target -Force
}
if($RemoveFromPath){
  $current=[Environment]::GetEnvironmentVariable('Path','User')
  $items=@($current -split ';' | Where-Object {
    $_.Trim() -ne '' -and $_.TrimEnd('\') -ine $target.TrimEnd('\')
  })
  [Environment]::SetEnvironmentVariable('Path',($items -join ';'),'User')
  Write-Output 'User PATH updated; open a new Terminal to see changes.'
}
if(Test-Path -LiteralPath $target){
  Write-Output "Kept user or unrecognized files in: $target"
}else {Write-Output 'Nova Native user-local installation removed.'}
