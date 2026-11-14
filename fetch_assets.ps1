# Windows entry point of fetch_assets.sh (runs it in MSYS2's MINGW64 environment, whose Python
# has Pillow). Usage:
#     powershell.exe -ExecutionPolicy Bypass -File .\fetch_assets.ps1 -Rom "C:\path\to\emerald.gba"
# build_all.ps1 does this and the rest of the build in one step.
param([Parameter(Mandatory = $true)][string]$Rom)
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $scriptDir

$bashExe = "C:\msys64\usr\bin\bash.exe"
if (-not (Test-Path -LiteralPath $bashExe)) {
    Write-Host "MSYS2 not found in C:\msys64. Run build_all.ps1, which installs it, or see docs/BUILD_YOUR_OWN.md." -ForegroundColor Red
    exit 1
}
if (-not (Test-Path -LiteralPath $Rom -PathType Leaf)) {
    Write-Host "ROM not found: $Rom" -ForegroundColor Red
    exit 1
}

# Paths go through environment variables: Windows PowerShell 5.1 does not pass double quotes
# inside an argument on to programs, so the command below has none. With word splitting
# (IFS) and globbing (set -f) off, the unquoted paths stay whole whatever they contain.
$env:FETCH_DIR = $scriptDir
$env:FETCH_ROM = (Get-Item -LiteralPath $Rom).FullName
$env:MSYSTEM = "MINGW64"
$env:CHERE_INVOKING = "1"
& $bashExe -lc 'set -f; IFS=; cd $(cygpath -u $FETCH_DIR) && ./fetch_assets.sh $(cygpath -u $FETCH_ROM)'
exit $LASTEXITCODE
