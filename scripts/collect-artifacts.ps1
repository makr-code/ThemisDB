param(
  [string]$Version = (Get-Content -Path (Join-Path $PSScriptRoot "..\VERSION") -ErrorAction SilentlyContinue | Select-Object -First 1).Trim(),
  [string]$OutputDir = (Join-Path $PSScriptRoot "..\release")
)

$ErrorActionPreference = 'Stop'
if (-not $Version) { throw "Version konnte nicht ermittelt werden. Bitte VERSION pflegen oder -Version angeben." }

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

# Windows Artefakt
$winBin = Join-Path $root "build-msvc\Release\themis_server.exe"
if (Test-Path $winBin) {
  $dst = Join-Path $OutputDir ("themisdb-v$Version-windows-x64.exe")
  Copy-Item $winBin $dst -Force
  Write-Host "✓ Windows Artefakt gesammelt: $dst" -ForegroundColor Green
  $winDll = Join-Path $root "build-msvc\Release\themis_core.dll"
  if (Test-Path $winDll) {
    $dllDst = Join-Path $OutputDir ("themisdb-v$Version-windows-x64-themis_core.dll")
    Copy-Item $winDll $dllDst -Force
    Write-Host "✓ Windows DLL gesammelt: $dllDst" -ForegroundColor Green
  }
}

# Linux Artefakt
$linuxBin = Join-Path $root "build-linux\themis_server"
if (Test-Path $linuxBin) {
  $dst = Join-Path $OutputDir ("themisdb-v$Version-linux-x64")
  Copy-Item $linuxBin $dst -Force
  Write-Host "✓ Linux Artefakt gesammelt: $dst" -ForegroundColor Green
  $linuxSo = Join-Path $root "build-linux/libthemis_core.so"
  if (Test-Path $linuxSo) {
    $soDst = Join-Path $OutputDir ("themisdb-v$Version-linux-x64-libthemis_core.so")
    Copy-Item $linuxSo $soDst -Force
    Write-Host "✓ Linux .so gesammelt: $soDst" -ForegroundColor Green
  }
}
