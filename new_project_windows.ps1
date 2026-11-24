# Generate a multi-file Windows Nova project. Requires only PowerShell (not Python).
[CmdletBinding()]
param(
  [Parameter(Mandatory=$true)][string]$Name,
  [ValidateSet('Console','Desktop')][string]$Template='Desktop',
  [string]$Destination='.'
)
$ErrorActionPreference='Stop'
if($Name -notmatch '^[\p{L}\p{N}_-]{1,50}$'){throw 'Project name may contain only letters, digits, _ and - (1..50 characters)'}
$dest=[IO.Path]::GetFullPath((Join-Path $Destination $Name))
if([IO.Directory]::Exists($dest) -or [IO.File]::Exists($dest)){throw "Project already exists: $dest"}
[IO.Directory]::CreateDirectory((Join-Path $dest 'lib')) | Out-Null
$utf8=New-Object System.Text.UTF8Encoding($false)
$helper=@'
function greeting(name)
    return "Привіт, " + name + "!"
end
'@
[IO.File]::WriteAllText((Join-Path $dest 'lib\greeter.nova'),$helper,$utf8)
if($Template -eq 'Console'){
  $main=@'
import "lib/greeter.nova" as greeter
let name = "Nova"
pokaż greeter.greeting(name)
'@
}else{
  $main=@'
import ui
import "lib/greeter.nova" as greeter
let app = ui.window("My Nova App", 520, 320)
let label = ui.label(app, "Твій перший застосунок Nova")
let name = ui.input(app, "Світ")
function say_hello()
    label.set(greeter.greeting(name.get()))
end
ui.button(app, "Привітатися", say_hello)
ui.run(app)
'@
}
[IO.File]::WriteAllText((Join-Path $dest 'main.nova'),$main,$utf8)
$readme="Nova project: $Name`r`nTemplate: $Template`r`nRun: nova-native.exe run main.nova`r`nBuild: build_app_windows.ps1 -Source main.nova -Output $Name.exe"
[IO.File]::WriteAllText((Join-Path $dest 'README.txt'),$readme,$utf8)
Write-Output "Created $Template project: $dest"
Write-Output 'Edit main.nova, then build it with build_app_windows.ps1.'
