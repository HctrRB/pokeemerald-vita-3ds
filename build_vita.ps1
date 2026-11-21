# Builds build-vita/pokeemerald.vpk from PowerShell by running build_vita.sh in MSYS2.
# Needs the game files and the converted assets (build_all.ps1 does everything in one step).
# -Probe builds the measuring build (GPU probes + automatic bench) into build-vita-probe\.
# -Locale <code> builds in that language (tools/locales/languages.json) into build-vita-<code>\;
# a language made from its official ROM also needs -Rom <your ROM of that language>.
param([switch]$Probe, [string]$Locale = "en", [string]$Rom = "")
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $scriptDir

$bashExe = "C:\msys64\usr\bin\bash.exe"
if (-not (Test-Path -LiteralPath $bashExe)) {
    Write-Host "MSYS2 not found in C:\msys64. Run build_all.ps1, which installs it, or see docs/guides/BUILD.md." -ForegroundColor Red
    exit 1
}

Write-Host "=== Building pokeemerald-vita in $scriptDir ===" -ForegroundColor Cyan

# The path goes through an environment variable (no double quotes in the command: Windows
# PowerShell 5.1 does not pass them on to programs)
$env:BUILD_VITA_DIR = $scriptDir
$env:CHERE_INVOKING = "1"
# BUILD_DIR / EXTRA_CMAKE_ARGS (read by build_vita.sh) pick a second build folder and its options
if ($Probe) {
    $env:BUILD_DIR = "build-vita-probe"
    $env:EXTRA_CMAKE_ARGS = "-DVITA_GPU_PROBE=ON"
}
if ($Locale -ne "en") {
    $env:VITA_LOCALE = $Locale
}
if ($Rom -ne "") {
    # MSYS2 path of the ROM, passed through the environment (no quotes in the command)
    $env:VITA_ROM = (& "C:\msys64\usr\bin\cygpath.exe" -u (Resolve-Path -LiteralPath $Rom).Path)
}
$outDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } elseif ($Locale -ne "en") { "build-vita-$Locale" } else { "build-vita" }
& $bashExe -lc 'set -f; IFS=; cd $(cygpath -u $BUILD_VITA_DIR) && ./build_vita.sh'
$buildExit = $LASTEXITCODE
if ($Probe) {
    Remove-Item Env:BUILD_DIR, Env:EXTRA_CMAKE_ARGS -ErrorAction SilentlyContinue
}
if ($Locale -ne "en") {
    Remove-Item Env:VITA_LOCALE -ErrorAction SilentlyContinue
}
if ($Rom -ne "") {
    Remove-Item Env:VITA_ROM -ErrorAction SilentlyContinue
}
if ($buildExit -eq 0) {
    Write-Host "=== Done: $outDir\pokeemerald.vpk ===" -ForegroundColor Green
} else {
    Write-Host "The build failed (exit code $buildExit)." -ForegroundColor Red
}
exit $buildExit
