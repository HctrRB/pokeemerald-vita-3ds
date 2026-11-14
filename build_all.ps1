# One step from your Pokemon Emerald dump to an installable VPK (Windows). Usage:
#     powershell.exe -ExecutionPolicy Bypass -File .\build_all.ps1 -Rom "C:\path\to\emerald.gba"
# Without -Rom, a window asks for the ROM when it is needed. The ROM is only needed the first
# time (to get the game files). Installs MSYS2 in C:\msys64 when it is missing, then runs
# build_all.sh in it; the result is EmeraldVita.vpk next to this script. Work in progress:
# please report bugs as GitHub issues.
param([string]$Rom = "")
$ErrorActionPreference = "Stop"

function Fail([string]$msg) {
    Write-Host $msg -ForegroundColor Red
    exit 1
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $scriptDir

# make cannot build in a folder whose path has spaces or other special characters
if ($scriptDir -notmatch '^[A-Za-z]:\\[A-Za-z0-9_.\-\\]*$') {
    Fail ("This folder's path has spaces or special characters, which the build tools cannot handle:`n" +
          "    $scriptDir`n" +
          "Move the whole folder to a simple path, for example C:\Emerald\pokeemerald-vita-3ds, and run this again.")
}

# ---------------------------------------------------------------- the ROM (only the first time)
$assetsReady = (Test-Path -LiteralPath (Join-Path $scriptDir ".assets_ok")) -and
               (Test-Path -LiteralPath (Join-Path $scriptDir "graphics\pokemon\bulbasaur\front.png")) -and
               (Test-Path -LiteralPath (Join-Path $scriptDir "assets\ui2\ui_atlas.bin"))
if ($Rom -eq "" -and -not $assetsReady) {
    Add-Type -AssemblyName System.Windows.Forms
    $dialog = New-Object System.Windows.Forms.OpenFileDialog
    $dialog.Title = "Choose your Pokemon Emerald dump (.gba)"
    $dialog.Filter = "GBA ROM (*.gba)|*.gba|All files (*.*)|*.*"
    if ($dialog.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {
        Fail "No ROM chosen. Run this again and choose your Pokemon Emerald .gba file."
    }
    $Rom = $dialog.FileName
}
if ($Rom -ne "") {
    if (-not (Test-Path -LiteralPath $Rom -PathType Leaf)) { Fail "ROM not found: $Rom" }
    $Rom = (Get-Item -LiteralPath $Rom).FullName
}

# ---------------------------------------------------------------- MSYS2
$msysRoot = "C:\msys64"
$bashExe = Join-Path $msysRoot "usr\bin\bash.exe"

# Runs a command in MSYS2 and returns its exit code; its output goes straight to the console.
# (No double quotes in $command: Windows PowerShell 5.1 does not pass them on to programs.)
function Invoke-Msys([string]$command, [string]$system = "MSYS") {
    $env:MSYSTEM = $system
    $env:CHERE_INVOKING = "1"
    & $bashExe -lc $command | Out-Host
    return $LASTEXITCODE
}

if (-not (Test-Path -LiteralPath $bashExe)) {
    Write-Host "=== Installing MSYS2 in $msysRoot (once, a few minutes) ===" -ForegroundColor Cyan
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $installer = Join-Path $env:TEMP "msys2-x86_64-latest.exe"
    $url = "https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64-latest.exe"
    try {
        $ProgressPreference = "SilentlyContinue"   # the progress bar makes the download very slow
        Invoke-WebRequest -Uri $url -OutFile $installer -UseBasicParsing
    } catch {
        Fail "Could not download MSYS2 ($url): $($_.Exception.Message)`nInstall it by hand from https://www.msys2.org in C:\msys64 and run this again."
    }
    & $installer in --confirm-command --accept-messages --root "C:/msys64" | Out-Host
    Remove-Item -LiteralPath $installer -ErrorAction SilentlyContinue
    if (-not (Test-Path -LiteralPath $bashExe)) {
        Fail "MSYS2 did not install. Install it by hand from https://www.msys2.org in C:\msys64 and run this again."
    }
    # First start (creates the user's home), then update it. The first update can close the
    # shell when it updates MSYS2 itself, so it runs twice.
    $null = Invoke-Msys "true"
    $null = Invoke-Msys "pacman -Syu --noconfirm"
    $null = Invoke-Msys "pacman -Syu --noconfirm"
}

# ---------------------------------------------------------------- the build
# The ROM goes through an environment variable (build_all.sh reads BUILD_ALL_ROM), so that any
# character in its path is safe. The folder's path was checked above: no spaces.
$env:BUILD_ALL_DIR = $scriptDir
$env:BUILD_ALL_ROM = $Rom
# MINGW64 environment: the host tools use its gcc, the generators its Python with Pillow
$code = Invoke-Msys 'cd $(cygpath -u $BUILD_ALL_DIR) && ./build_all.sh' "MINGW64"
if ($code -eq 0) {
    Write-Host "VPK: $scriptDir\EmeraldVita.vpk" -ForegroundColor Green
} else {
    Write-Host "The build failed (exit code $code). See the messages above and docs/BUILD_YOUR_OWN.md." -ForegroundColor Red
}
exit $code
