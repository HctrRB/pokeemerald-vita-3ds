# One step from your Pokémon Emerald dump to an installable VPK (Windows). Usage:
#     powershell.exe -ExecutionPolicy Bypass -File .\build_all.ps1 -Rom "C:\path\to\emerald.gba"
# The ROM is only needed the first time (to get the game files). Runs build_all.sh in MSYS2;
# the result is EmeraldVita.vpk next to this script. Work in progress: please report bugs as
# GitHub issues.
param([string]$Rom = "")
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

$bashExe = "C:\msys64\usr\bin\bash.exe"
if (-not (Test-Path $bashExe)) {
    Write-Host "MSYS2 is not installed in C:\msys64." -ForegroundColor Red
    Write-Host "Install it from https://www.msys2.org (or: winget install MSYS2.MSYS2), then run this again."
    exit 1
}

function To-Unix([string]$path) {
    $full = (Resolve-Path $path).Path
    return "/" + $full.Substring(0, 1).ToLower() + "/" + $full.Substring(3).Replace('\', '/')
}

$unixDir = To-Unix $scriptDir
$unixRom = ""
if ($Rom -ne "") {
    if (-not (Test-Path $Rom)) { Write-Error "ROM not found: $Rom"; exit 1 }
    $unixRom = To-Unix $Rom
}

# MINGW64 environment: the host tools use its gcc, the generators its Python with Pillow
$env:MSYSTEM = "MINGW64"
$env:CHERE_INVOKING = "1"
& $bashExe -lc "cd '$unixDir' && ./build_all.sh '$unixRom'"
if ($LASTEXITCODE -eq 0) {
    Write-Host "VPK: $scriptDir\EmeraldVita.vpk" -ForegroundColor Green
} else {
    Write-Host "The build failed (exit code $LASTEXITCODE). See the messages above and docs/BUILD_YOUR_OWN.md." -ForegroundColor Red
}
exit $LASTEXITCODE
