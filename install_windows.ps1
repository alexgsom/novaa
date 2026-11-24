# Install Nova Native for current Windows user; no administrator rights required.
# PATH modification is opt-in. Does not register a .nova file association.
[CmdletBinding()]
param([switch] $AddToPath, [switch] $StartMenu)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $MyInvocation.MyCommand.Path
$source=Join-Path $root 'bin\nova-native.exe'
if(-not (Test-Path -LiteralPath $source -PathType Leaf)){
  throw 'Please run build_windows.bat or build_windows.ps1 first: bin\nova-native.exe is missing'
}
if($StartMenu -and -not (Test-Path -LiteralPath (Join-Path $root 'bin\NovaStudio.exe') -PathType Leaf)){
  throw 'Cannot add Start Menu shortcut: first build bin\NovaStudio.exe'
}
$target=Join-Path $env:LOCALAPPDATA 'Programs\NovaNative'
[IO.Directory]::CreateDirectory($target)|Out-Null
[IO.Directory]::CreateDirectory((Join-Path $target 'bin'))|Out-Null
Copy-Item -LiteralPath $source -Destination (Join-Path $target 'nova-native.exe') -Force
Copy-Item -LiteralPath $source -Destination (Join-Path $target 'bin\nova-native.exe') -Force
$studio=Join-Path $root 'bin\NovaStudio.exe'
if(Test-Path -LiteralPath $studio -PathType Leaf){Copy-Item -LiteralPath $studio -Destination (Join-Path $target 'bin\NovaStudio.exe') -Force}
$gui=Join-Path $root 'bin\nova-windowed.exe'
if(Test-Path -LiteralPath $gui -PathType Leaf){Copy-Item -LiteralPath $gui -Destination (Join-Path $target 'bin\nova-windowed.exe') -Force}
$compiler=Join-Path $root 'compiler\novac_selfhost.nbc'
if(Test-Path -LiteralPath $compiler -PathType Leaf){
  [IO.Directory]::CreateDirectory((Join-Path $target 'compiler'))|Out-Null
  Copy-Item -LiteralPath $compiler -Destination (Join-Path $target 'compiler\novac_selfhost.nbc') -Force
}
# Optional Explorer integration: installer copies scripts but does NOT change file associations.
foreach($script in @('register_nova_files.ps1','unregister_nova_files.ps1','uninstall_windows.ps1')){
  $file=Join-Path $root $script
  if(Test-Path -LiteralPath $file -PathType Leaf){Copy-Item -LiteralPath $file -Destination (Join-Path $target $script) -Force}
}
$portable=Join-Path $root 'nova.ps1'
if(Test-Path -LiteralPath $portable -PathType Leaf){Copy-Item -LiteralPath $portable -Destination (Join-Path $target 'nova.ps1') -Force}

# Optional native SQLite backend: copy a trusted official sqlite3.dll if the user supplied it.
$sqlite=Join-Path $root 'bin\sqlite3.dll'
if(Test-Path -LiteralPath $sqlite -PathType Leaf){
  Copy-Item -LiteralPath $sqlite -Destination (Join-Path $target 'sqlite3.dll') -Force
  Copy-Item -LiteralPath $sqlite -Destination (Join-Path $target 'bin\sqlite3.dll') -Force
}
Copy-Item -LiteralPath (Join-Path $root 'nova.cmd') -Destination (Join-Path $target 'nova.cmd') -Force
if($AddToPath){
  $current=[Environment]::GetEnvironmentVariable('Path','User')
  $items=@($current -split ';' | Where-Object { $_.Trim() -ne '' })
  if(-not ($items | Where-Object { $_.TrimEnd('\\') -ieq $target.TrimEnd('\\') })){
    [Environment]::SetEnvironmentVariable('Path',(($items+@($target)) -join ';'),'User')
    Write-Output 'User PATH updated; open a new Terminal to use the nova command.'
  }
}
Write-Output "Installed Nova Native to: $target"
Write-Output "Run: `"$target\nova.cmd`" run examples\hello_trilingual.nova"

if($StartMenu){
  $studioExe=Join-Path $target 'bin\NovaStudio.exe'
  if(-not (Test-Path -LiteralPath $studioExe -PathType Leaf)){throw 'Cannot create Start Menu entry: NovaStudio.exe was not installed.'}
  $menu=Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Nova Studio.lnk'
  $shell=New-Object -ComObject WScript.Shell
  $shortcut=$shell.CreateShortcut($menu)
  $shortcut.TargetPath=$studioExe
  $shortcut.WorkingDirectory=$target
  $shortcut.Description='Nova Studio native Windows IDE'
  $shortcut.Save()
  Write-Output "Start Menu shortcut: $menu"
}
