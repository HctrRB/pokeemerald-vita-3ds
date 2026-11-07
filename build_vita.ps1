# Build script for pokeemerald-vita on Windows PowerShell
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

$bashExe = "C:\msys64\usr\bin\bash.exe"
if (-not (Test-Path $bashExe)) {
    Write-Error "MSYS2 bash.exe no encontrado en C:\msys64\usr\bin\bash.exe"
    exit 1
}

# Convert Windows path (e.g. A:\foo\bar) to MSYS2 posix path (/a/foo/bar)
$drive = (Get-Item $scriptDir).PSDrive.Name.ToLower()
$restPath = $scriptDir.Substring(3).Replace('\', '/')
$unixDir = "/$drive/$restPath"

Write-Host "=== Iniciando compilacion de pokeemerald-vita ===" -ForegroundColor Cyan
Write-Host "Directorio: $unixDir" -ForegroundColor Gray

& $bashExe -lc "cd '$unixDir' && ./build_vita.sh"
if ($LASTEXITCODE -eq 0) {
    Write-Host "=== Compilacion exitosa: build-vita/pokeemerald.vpk generado ===" -ForegroundColor Green
} else {
    Write-Error "Fallo en la compilacion (codigo de salida $LASTEXITCODE)"
}
