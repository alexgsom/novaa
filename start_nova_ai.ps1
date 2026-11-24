# Optional launcher: asks for an API key without writing it to the repository or disk.
# The key is visible only to this process and the launched Nova Studio process.
$ErrorActionPreference = 'Stop'
$studio = Join-Path $PSScriptRoot 'bin\NovaStudio.exe'
if (-not (Test-Path -LiteralPath $studio -PathType Leaf)) {
    throw 'NovaStudio.exe not found. Build Windows Nova first or unpack the successful GitHub Actions artifact.'
}
Write-Host 'Nova Studio Next - optional OpenAI connection'
Write-Host 'OpenAI API usage may incur separate charges. Your prompt is sent only when you click Ask.'
$secure = Read-Host 'Paste your OpenAI API key (hidden)' -AsSecureString
$ptr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure)
try {
    $env:OPENAI_API_KEY = [Runtime.InteropServices.Marshal]::PtrToStringBSTR($ptr)
    & $studio
} finally {
    Remove-Item Env:\OPENAI_API_KEY -ErrorAction SilentlyContinue
    [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($ptr)
}
