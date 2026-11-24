# Real Win32 UI callback test, not a source-only or bytecode-only check.
# Requires a Windows runner with an interactive window station/desktop.
[CmdletBinding()]
param([string]$NovaRoot = '')
$ErrorActionPreference = 'Stop'
if (-not $NovaRoot) { $NovaRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path) }
$NovaRoot = (Resolve-Path -LiteralPath $NovaRoot).ProviderPath
$nova = Join-Path $NovaRoot 'nova.ps1'
$source = Join-Path $NovaRoot 'tests/windows_gui_click.nova'
if (-not (Test-Path -LiteralPath $source)) { throw "GUI test source missing: $source" }
$code = @'
using System;
using System.Runtime.InteropServices;
public static class NovaWin32Test {
  [DllImport("user32.dll", EntryPoint="FindWindowW", CharSet=CharSet.Unicode, ExactSpelling=true)]
  public static extern IntPtr FindWindow(string cls, string caption);
  [DllImport("user32.dll", EntryPoint="FindWindowExW", CharSet=CharSet.Unicode, ExactSpelling=true)]
  public static extern IntPtr FindWindowEx(IntPtr parent, IntPtr childAfter, string cls, string caption);
  [DllImport("user32.dll", EntryPoint="SendMessageTimeoutW", CharSet=CharSet.Unicode, ExactSpelling=true, SetLastError=true)]
  public static extern IntPtr SendMessageTimeout(IntPtr handle, uint msg, IntPtr wparam, IntPtr lparam, uint flags, uint timeout, out IntPtr result);
}
'@
if (-not ('NovaWin32Test' -as [type])) { Add-Type -TypeDefinition $code }
$tmp = Join-Path ([IO.Path]::GetTempPath()) ('nova-click-Łódź-' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($tmp) | Out-Null
$proc = $null
try {
  $exe = Join-Path $tmp 'NovaClick.exe'
  & $nova build -Source $source -Output $exe -Windowed
  if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $exe)) { throw 'Failed to build windowed self-hosted Nova GUI EXE' }
  $proc = Start-Process -FilePath $exe -WorkingDirectory $tmp -PassThru
  $window = [IntPtr]::Zero
  $button = [IntPtr]::Zero
  $deadline = [DateTime]::UtcNow.AddSeconds(20)
  do {
    if ($proc.HasExited) { throw "GUI process exited before click with code $($proc.ExitCode)" }
    $window = [NovaWin32Test]::FindWindow('NovaNativeWindow03', 'Nova Win32 Click CI')
    if ($window -ne [IntPtr]::Zero) {
      $button = [NovaWin32Test]::FindWindowEx($window, [IntPtr]::Zero, 'Button', 'CI_CLICK_ME')
    }
    if ($button -ne [IntPtr]::Zero) { break }
    Start-Sleep -Milliseconds 200
  } while ([DateTime]::UtcNow -lt $deadline)
  if ($button -eq [IntPtr]::Zero) { throw 'Native Win32 window/button not found; CI desktop or GUI runtime unavailable' }
  # BM_CLICK (0xF5): exercise the real Windows WM_COMMAND/BN_CLICKED callback.
  $reply = [IntPtr]::Zero
  $sent = [NovaWin32Test]::SendMessageTimeout($button, [uint32]0x00F5, [IntPtr]::Zero, [IntPtr]::Zero, [uint32]0x0002, [uint32]8000, [ref]$reply)
  if ($sent -eq [IntPtr]::Zero) { throw 'Win32 BM_CLICK timed out or failed' }
  if (-not $proc.WaitForExit(12000)) { throw 'Nova UI callback did not close the window' }
  if ($proc.ExitCode -ne 0) { throw "Nova GUI callback failed (exit $($proc.ExitCode))" }
  $marker = Join-Path $tmp 'nova_ci_click.txt'
  if (-not (Test-Path -LiteralPath $marker)) { throw 'Nova callback did not create marker file' }
  $actual = [IO.File]::ReadAllText($marker, [Text.Encoding]::UTF8)
  if ($actual -ne 'CLICKED_OK_Київ_Łódź') { throw "Wrong callback marker: $actual" }
  Write-Output 'PASS: Win32 EXE received real button click -> Nova VM callback -> UTF-8 storage -> closed'
} finally {
  if ($null -ne $proc) {
    $proc.Refresh()
    if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue }
    $proc.Dispose()
  }
  Remove-Item -LiteralPath $tmp -Force -Recurse -ErrorAction SilentlyContinue
}
