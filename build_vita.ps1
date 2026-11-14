# Builds build-vita/pokeemerald.vpk from PowerShell by running build_vita.sh in MSYS2.
# Needs the game files and the converted assets (build_all.ps1 does everything in one step).
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $scriptDir

$bashExe = "C:\msys64\usr\bin\bash.exe"
if (-not (Test-Path -LiteralPath $bashExe)) {
    Write-Host "MSYS2 not found in C:\msys64. Run build_all.ps1, which installs it, or see docs/BUILD.md." -ForegroundColor Red
    exit 1
}

Write-Host "=== Building pokeemerald-vita in $scriptDir ===" -ForegroundColor Cyan

# The path goes through an environment variable (no double quotes in the command: Windows
# PowerShell 5.1 does not pass them on to programs)
$env:BUILD_VITA_DIR = $scriptDir
$env:CHERE_INVOKING = "1"
& $bashExe -lc 'set -f; IFS=; cd $(cygpath -u $BUILD_VITA_DIR) && ./build_vita.sh'
if ($LASTEXITCODE -eq 0) {
    Write-Host "=== Done: build-vita\pokeemerald.vpk ===" -ForegroundColor Green
} else {
    Write-Host "The build failed (exit code $LASTEXITCODE)." -ForegroundColor Red
}
exit $LASTEXITCODE
