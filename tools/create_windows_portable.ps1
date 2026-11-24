# Build a relocatable, Python-free x64 Nova installation for GitHub Actions.
[CmdletBinding()]
param([string]$Destination='dist/windows-x64/Nova')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$dest=if([IO.Path]::IsPathRooted($Destination)){$Destination}else{Join-Path $root $Destination}
if(Test-Path -LiteralPath $dest) { throw "Destination already exists: $dest (refusing to overwrite)" }
$files=@(
  'bin/nova-native.exe', 'bin/nova-windowed.exe', 'bin/NovaStudio.exe',
  'compiler/novac_selfhost.nbc',
  'nova.ps1','nova.cmd','install_windows.ps1','uninstall_windows.ps1',
  'register_nova_files.ps1','unregister_nova_files.ps1',
  'tests/windows_gui_click.nova', 'tests/smoke_win32_click.ps1',
  'examples/bootstrap/trilingual.nova',
  'examples/windows09/hello_gui.nova', 'examples/windows09/counter_gui.nova',
  'examples/windows09/notes_gui.nova',
  'README.md','INSTALL_WINDOWS.md','WINDOWS_CI.md','studio/README.md','LICENSE'
)
foreach($relative in $files) {
  if(-not (Test-Path -LiteralPath (Join-Path $root $relative) -PathType Leaf)) {
    throw "Distribution dependency missing: $relative"
  }
}
python (Join-Path $root 'tools/verify_pe.py') `
 (Join-Path $root 'bin/nova-native.exe') `
 (Join-Path $root 'bin/nova-windowed.exe') `
 (Join-Path $root 'bin/NovaStudio.exe') `
 --console nova-native.exe --gui nova-windowed.exe --gui NovaStudio.exe
if ($LASTEXITCODE -ne 0) { throw 'Windows executables failed PE32+ metadata checks' }
[IO.Directory]::CreateDirectory($dest)|Out-Null
foreach($relative in $files) {
  $from=Join-Path $root $relative
  $to=Join-Path $dest $relative
  $parent=Split-Path -Parent $to
  [IO.Directory]::CreateDirectory($parent)|Out-Null
  Copy-Item -LiteralPath $from -Destination $to
}
[IO.File]::WriteAllText((Join-Path $dest 'VERSION.txt'),"Nova Native 1.0 Beta 2`nWindows x64, CI acceptance build`n",[Text.UTF8Encoding]::new($false))
# Hash manifest is for accidental corruption detection, not code signing.
$manifest=@()
foreach($f in Get-ChildItem -LiteralPath $dest -File -Recurse | Sort-Object FullName) {
  $rel=$f.FullName.Substring($dest.Length+1).Replace('\','/')
  $hash=(Get-FileHash -LiteralPath $f.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
  $manifest+="$hash *$rel"
}
[IO.File]::WriteAllLines((Join-Path $dest 'SHA256_FILES.txt'),$manifest,[Text.UTF8Encoding]::new($false))
Write-Output "PASS: created relocatable Windows x64 distribution at $dest"
