param(
  [string]$Version = (Get-Content -Path (Join-Path $PSScriptRoot "..\VERSION") -ErrorAction SilentlyContinue).Trim(),
  [ValidateSet('windows','linux')]
  [string]$Platform = 'windows',
  [string]$OutputDir = (Join-Path $PSScriptRoot "..\release"),
  [string]$BinaryPath = ''
)

$ErrorActionPreference = 'Stop'

if (-not $Version) {
  throw "Version konnte nicht ermittelt werden. Bitte via -Version angeben oder VERSION-Datei pflegen."
}

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

switch ($Platform) {
  'windows' {
    if (-not $BinaryPath) { $BinaryPath = Join-Path $root "build-msvc\Release\themis_server.exe" }
    if (-not (Test-Path $BinaryPath)) { throw "Windows-Binary nicht gefunden: $BinaryPath" }
    $pkgName = "themis-v$Version-windows-x64"
    $pkgDir = Join-Path $OutputDir $pkgName

    if (Test-Path $pkgDir) { Remove-Item $pkgDir -Recurse -Force }
    New-Item -ItemType Directory -Path $pkgDir | Out-Null

    Copy-Item $BinaryPath (Join-Path $pkgDir "themis_server.exe")
    # Falls dynamisch gebaut: DLL beilegen
    $dll = Join-Path $root "build-msvc\Release\themis_core.dll"
    if (Test-Path $dll) { Copy-Item $dll (Join-Path $pkgDir "themis_core.dll") }
    Copy-Item (Join-Path $root "README.md") $pkgDir -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $root "LICENSE") $pkgDir -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $root "license.md") $pkgDir -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $root "config") $pkgDir -Recurse -ErrorAction SilentlyContinue

    $zipPath = Join-Path $OutputDir ("$pkgName.zip")
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
    Compress-Archive -Path $pkgDir -DestinationPath $zipPath
    Write-Host "✓ Windows ZIP erstellt: $zipPath" -ForegroundColor Green
  }
  'linux' {
    if (-not $BinaryPath) { $BinaryPath = Join-Path $root "build-linux\themis_server" }
    if (-not (Test-Path $BinaryPath)) { throw "Linux-Binary nicht gefunden: $BinaryPath" }
    $pkgName = "themisdb-v$Version-linux-x64"
    $pkgDir = Join-Path $OutputDir $pkgName

    if (Test-Path $pkgDir) { Remove-Item $pkgDir -Recurse -Force }
    New-Item -ItemType Directory -Path $pkgDir | Out-Null

    Copy-Item $BinaryPath (Join-Path $pkgDir "themis_server")
    # Falls dynamisch gebaut: .so beilegen
    $so = Join-Path $root "build-linux/libthemis_core.so"
    if (Test-Path $so) { Copy-Item $so (Join-Path $pkgDir "libthemis_core.so") }
    Copy-Item (Join-Path $root "README.md") $pkgDir -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $root "LICENSE") $pkgDir -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $root "license.md") $pkgDir -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $root "config") $pkgDir -Recurse -ErrorAction SilentlyContinue

    $zipPath = Join-Path $OutputDir ("$pkgName.zip")
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
    Compress-Archive -Path $pkgDir -DestinationPath $zipPath
    Write-Host "✓ Linux ZIP erstellt: $zipPath" -ForegroundColor Green
  }
}
