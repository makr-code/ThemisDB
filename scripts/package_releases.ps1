param(
    [switch]$SkipStrip
)

$ErrorActionPreference = 'Stop'

Write-Host "=== Package ThemisDB Release Artefakte ===" -ForegroundColor Cyan

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = Split-Path -Parent $root
Set-Location $repo

# Ensure dist folder
$dist = Join-Path $repo 'dist'
New-Item -ItemType Directory -Force -Path $dist | Out-Null

function Sha256($path) {
    (Get-FileHash -Algorithm SHA256 -Path $path).Hash.ToLower()
}

# Helper: run a bash command in the qnap builder (for linux strip/tar)
function Run-Docker([string]$command) {
    & docker run --rm -v "${repo}:/src" -w /src themisdb-qnap-builder:latest bash -lc $command
    if ($LASTEXITCODE -ne 0) { throw "Docker command failed: $command" }
}

# Package QNAP Linux (build-qnap)
$binQnap = Join-Path $repo 'build-qnap/themis_server'
if (Test-Path $binQnap) {
    Write-Host "[QNAP] Erstelle vollständiges QNAP Release-Package" -ForegroundColor Yellow
    $outTar = Join-Path $dist 'themisdb-1.0.0-qnap-x64.tar.gz'
    $cmd = @()
    $cmd += 'set -e'
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/bin'
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/config'
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/docs'
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/examples'
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/tools'
    
    # Binary
    $cmd += 'cp /src/build-qnap/themis_server /src/dist/themisdb-1.0.0/bin/'
    if (-not $SkipStrip) { $cmd += 'strip /src/dist/themisdb-1.0.0/bin/themis_server' }
    
    # Copy shared libraries
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/lib'
    $cmd += 'if [ -d /src/vcpkg_installed/x64-linux/lib ]; then'
    $cmd += '  find /src/vcpkg_installed/x64-linux/lib -name \"*.so*\" -exec cp -v {} /src/dist/themisdb-1.0.0/lib/ \;'
    $cmd += 'fi'
    
    # Config
    $cmd += 'cp /src/config/config.qnap.json /src/dist/themisdb-1.0.0/config/config.json'
    $cmd += 'cp /src/config/*.yaml /src/dist/themisdb-1.0.0/config/'
    $cmd += 'cp /src/config/policies.json /src/dist/themisdb-1.0.0/config/'
    $cmd += 'test -d /src/config/processors && cp -r /src/config/processors /src/dist/themisdb-1.0.0/config/ || true'
    $cmd += 'test -d /src/config/schemas && cp -r /src/config/schemas /src/dist/themisdb-1.0.0/config/ || true'
    
    # Docs
    $cmd += 'cp /src/README.md /src/LICENSE /src/dist/themisdb-1.0.0/'
    $cmd += 'test -f /src/CHANGELOG.md && cp /src/CHANGELOG.md /src/dist/themisdb-1.0.0/ || true'
    $cmd += 'test -f /src/SECURITY.md && cp /src/SECURITY.md /src/dist/themisdb-1.0.0/ || true'
    $cmd += 'test -f /src/docs/ThemisDB-Documentation.pdf && cp /src/docs/ThemisDB-Documentation.pdf /src/dist/themisdb-1.0.0/docs/ || true'
    $cmd += 'test -f /src/docs/deployment/QNAP_CPU_COMPATIBILITY.md && cp /src/docs/deployment/QNAP_CPU_COMPATIBILITY.md /src/dist/themisdb-1.0.0/docs/ || true'
    
    # OpenAPI & Client SDKs
    $cmd += 'test -d /src/openapi && cp -r /src/openapi /src/dist/themisdb-1.0.0/ || true'
    $cmd += 'test -d /src/clients && cp -r /src/clients /src/dist/themisdb-1.0.0/ || true'
    
    # Examples & Tools
    $cmd += 'test -d /src/examples && cp -r /src/examples/* /src/dist/themisdb-1.0.0/examples/ || true'
    $cmd += 'test -d /src/tools/plugin_signer && cp -r /src/tools/plugin_signer /src/dist/themisdb-1.0.0/tools/ || true'
    
    # Install Script
    $cmd += 'cat > /src/dist/themisdb-1.0.0/install.sh << \"EOF\"'
    $cmd += '#!/bin/bash'
    $cmd += 'set -e'
    $cmd += 'INSTALL_DIR=/opt/themisdb'
    $cmd += 'echo \"Installing ThemisDB to $INSTALL_DIR...\"'
    $cmd += 'mkdir -p $INSTALL_DIR'
    $cmd += 'cp -r bin config docs examples tools lib $INSTALL_DIR/'
    $cmd += 'chmod +x $INSTALL_DIR/bin/themis_server'
    $cmd += 'mkdir -p /var/lib/themisdb/data'
    $cmd += 'mkdir -p /var/log/themisdb'
    $cmd += 'echo \"export LD_LIBRARY_PATH=$INSTALL_DIR/lib:\\$LD_LIBRARY_PATH\" >> ~/.bashrc'
    $cmd += 'echo \"Installation complete. Start with:\"'
    $cmd += 'echo \"  export LD_LIBRARY_PATH=$INSTALL_DIR/lib:\\$LD_LIBRARY_PATH\"'
    $cmd += 'echo \"  $INSTALL_DIR/bin/themis_server --config $INSTALL_DIR/config/config.json\"'
    $cmd += 'EOF'
    $cmd += 'chmod +x /src/dist/themisdb-1.0.0/install.sh'
    
    # Tar
    $cmd += 'tar -C /src/dist -czf /src/dist/themisdb-1.0.0-qnap-x64.tar.gz themisdb-1.0.0'
    $cmd += 'rm -rf /src/dist/themisdb-1.0.0'
    
    Run-Docker ("$($cmd -join '; ')")
    $hash = Sha256 $outTar
    "$hash  $(Split-Path -Leaf $outTar)" | Out-File -FilePath (Join-Path $dist 'SHA256SUMS') -Append -Encoding ASCII
    Write-Host "  → $outTar" -ForegroundColor Green
}

# Package Linux GCC Release (build-linux-gcc-release) – optional
$binLinuxGcc = Join-Path $repo 'build-linux-gcc-release/themis_server'
if (Test-Path $binLinuxGcc) {
    Write-Host "[Linux] Erstelle vollständiges Linux Release-Package" -ForegroundColor Yellow
    $outTar2 = Join-Path $dist 'themisdb-1.0.0-linux-x64.tar.gz'
    $cmd = @()
    $cmd += 'set -e'
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/bin'
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/config'
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/docs'
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/examples'
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/tools'
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/systemd'
    
    # Binary
    $cmd += 'cp /src/build-linux-gcc-release/themis_server /src/dist/themisdb-1.0.0/bin/'
    if (-not $SkipStrip) { $cmd += 'strip /src/dist/themisdb-1.0.0/bin/themis_server' }
    
    # Copy shared libraries
    $cmd += 'mkdir -p /src/dist/themisdb-1.0.0/lib'
    $cmd += 'if [ -d /src/vcpkg_installed/x64-linux/lib ]; then'
    $cmd += '  find /src/vcpkg_installed/x64-linux/lib -name \"*.so*\" -exec cp -v {} /src/dist/themisdb-1.0.0/lib/ \;'
    $cmd += 'fi'
    
    # Config
    $cmd += 'cp /src/config/config.json /src/dist/themisdb-1.0.0/config/'
    $cmd += 'cp /src/config/*.yaml /src/dist/themisdb-1.0.0/config/'
    $cmd += 'cp /src/config/policies.json /src/dist/themisdb-1.0.0/config/'
    $cmd += 'test -d /src/config/processors && cp -r /src/config/processors /src/dist/themisdb-1.0.0/config/ || true'
    $cmd += 'test -d /src/config/schemas && cp -r /src/config/schemas /src/dist/themisdb-1.0.0/config/ || true'
    
    # Docs
    $cmd += 'cp /src/README.md /src/LICENSE /src/dist/themisdb-1.0.0/'
    $cmd += 'test -f /src/CHANGELOG.md && cp /src/CHANGELOG.md /src/dist/themisdb-1.0.0/ || true'
    $cmd += 'test -f /src/SECURITY.md && cp /src/SECURITY.md /src/dist/themisdb-1.0.0/ || true'
    $cmd += 'test -f /src/docs/ThemisDB-Documentation.pdf && cp /src/docs/ThemisDB-Documentation.pdf /src/dist/themisdb-1.0.0/docs/ || true'
    
    # OpenAPI & Client SDKs
    $cmd += 'test -d /src/openapi && cp -r /src/openapi /src/dist/themisdb-1.0.0/ || true'
    $cmd += 'test -d /src/clients && cp -r /src/clients /src/dist/themisdb-1.0.0/ || true'
    
    # Examples & Tools
    $cmd += 'test -d /src/examples && cp -r /src/examples/* /src/dist/themisdb-1.0.0/examples/ || true'
    $cmd += 'test -d /src/tools/plugin_signer && cp -r /src/tools/plugin_signer /src/dist/themisdb-1.0.0/tools/ || true'
    
    # Systemd Service
    $cmd += 'test -f /src/release/deb-package/themisdb-1.0.0/lib/systemd/system/themisdb.service && cp /src/release/deb-package/themisdb-1.0.0/lib/systemd/system/themisdb.service /src/dist/themisdb-1.0.0/systemd/ || true'
    
    # Install Script
    $cmd += 'cat > /src/dist/themisdb-1.0.0/install.sh << \"EOF\"'
    $cmd += '#!/bin/bash'
    $cmd += 'set -e'
    $cmd += 'INSTALL_DIR=/opt/themisdb'
    $cmd += 'echo \"Installing ThemisDB to $INSTALL_DIR...\"'
    $cmd += 'sudo mkdir -p $INSTALL_DIR'
    $cmd += 'sudo cp -r bin config docs examples tools lib $INSTALL_DIR/'
    $cmd += 'sudo chmod +x $INSTALL_DIR/bin/themis_server'
    $cmd += 'sudo mkdir -p /var/lib/themisdb/data'
    $cmd += 'sudo mkdir -p /var/log/themisdb'
    $cmd += 'sudo mkdir -p /etc/themisdb'
    $cmd += 'sudo cp config/*.json config/*.yaml /etc/themisdb/ || true'
    $cmd += '# Install libraries to system'
    $cmd += 'sudo cp lib/*.so* /usr/local/lib/ 2>/dev/null || true'
    $cmd += 'sudo ldconfig'
    $cmd += 'if [ -f systemd/themisdb.service ]; then'
    $cmd += '  echo \"Installing systemd service...\"'
    $cmd += '  sudo cp systemd/themisdb.service /etc/systemd/system/'
    $cmd += '  sudo systemctl daemon-reload'
    $cmd += '  echo \"Service installed. Enable with: sudo systemctl enable themisdb\"'
    $cmd += 'fi'
    $cmd += 'echo \"Installation complete. Start with:\"'
    $cmd += 'echo \"  $INSTALL_DIR/bin/themis_server --config /etc/themisdb/config.json\"'
    $cmd += 'echo \"Or with systemd: sudo systemctl start themisdb\"'
    $cmd += 'EOF'
    $cmd += 'chmod +x /src/dist/themisdb-1.0.0/install.sh'
    
    # Tar
    $cmd += 'tar -C /src/dist -czf /src/dist/themisdb-1.0.0-linux-x64.tar.gz themisdb-1.0.0'
    $cmd += 'rm -rf /src/dist/themisdb-1.0.0'
    
    Run-Docker ("$($cmd -join '; ')")
    $hash = Sha256 $outTar2
    "$hash  $(Split-Path -Leaf $outTar2)" | Out-File -FilePath (Join-Path $dist 'SHA256SUMS') -Append -Encoding ASCII
    Write-Host "  → $outTar2" -ForegroundColor Green
}

# Package Windows MSVC Release (build-msvc-ninja-release)
$binWin = Join-Path $repo 'build-msvc-ninja-release/themis_server.exe'
if (-not (Test-Path $binWin)) {
    # Fallback für Multi-Config VS Generator (Release Unterordner)
    $binWinRelease = Join-Path $repo 'build-msvc-ninja-release/Release/themis_server.exe'
    if (Test-Path $binWinRelease) {
        Write-Host "[Windows] Fallback: kopiere Release/themis_server.exe ins Root" -ForegroundColor Yellow
        Copy-Item $binWinRelease $binWin -Force
    }
}
if (Test-Path $binWin) {
    Write-Host "[Windows] Erstelle vollständiges Release-Package" -ForegroundColor Yellow
    $outZip = Join-Path $dist 'themisdb-1.0.0-windows-x64.zip'
    $stage = Join-Path $dist 'themisdb-1.0.0'
    
    # Erstelle Package-Struktur
    New-Item -ItemType Directory -Force -Path "$stage\bin" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stage\config" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stage\docs" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stage\examples" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stage\tools" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stage\scripts" | Out-Null
    
    # Binary
    Copy-Item $binWin -Destination "$stage\bin\themis_server.exe" -Force
    
    # Copy DLLs from vcpkg
    $vcpkgLibDir = Join-Path $repo "vcpkg_installed\x64-windows\bin"
    if (Test-Path $vcpkgLibDir) {
        Write-Host "  Copying DLLs from vcpkg..." -ForegroundColor Cyan
        Get-ChildItem "$vcpkgLibDir\*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
            Copy-Item $_.FullName -Destination "$stage\bin\" -Force
            Write-Host "    → $($_.Name)" -ForegroundColor DarkGray
        }
    } else {
        Write-Warning "  vcpkg bin directory not found: $vcpkgLibDir"
    }
    
    # Config-Dateien
    Copy-Item "$repo\config\*.json" -Destination "$stage\config\" -Force
    Copy-Item "$repo\config\*.yaml" -Destination "$stage\config\" -Force
    if (Test-Path "$repo\config\processors") {
        Copy-Item "$repo\config\processors" -Destination "$stage\config\" -Recurse -Force
    }
    if (Test-Path "$repo\config\schemas") {
        Copy-Item "$repo\config\schemas" -Destination "$stage\config\" -Recurse -Force
    }
    
    # Dokumentation
    Copy-Item "$repo\README.md" -Destination "$stage\" -Force
    Copy-Item "$repo\LICENSE" -Destination "$stage\" -Force
    Copy-Item "$repo\CHANGELOG.md" -Destination "$stage\" -Force -ErrorAction SilentlyContinue
    Copy-Item "$repo\SECURITY.md" -Destination "$stage\" -Force -ErrorAction SilentlyContinue
    if (Test-Path "$repo\docs\ThemisDB-Documentation.pdf") {
        Copy-Item "$repo\docs\ThemisDB-Documentation.pdf" -Destination "$stage\docs\" -Force
    }
    
    # OpenAPI specification
    if (Test-Path "$repo\openapi") {
        Copy-Item "$repo\openapi\*" -Destination "$stage\openapi\" -Recurse -Force
    }
    
    # Client libraries (SDKs)
    if (Test-Path "$repo\clients") {
        Copy-Item "$repo\clients\*" -Destination "$stage\clients\" -Recurse -Force
    }
    
    # Beispiele
    if (Test-Path "$repo\examples") {
        Copy-Item "$repo\examples\*" -Destination "$stage\examples\" -Recurse -Force
    }
    
    # Tools
    if (Test-Path "$repo\tools\plugin_signer") {
        Copy-Item "$repo\tools\plugin_signer" -Destination "$stage\tools\" -Recurse -Force
    }
    Copy-Item "$repo\tools\sign_*.py" -Destination "$stage\tools\" -Force -ErrorAction SilentlyContinue
    
    # Scripts
    Copy-Item "$repo\scripts\*.ps1" -Destination "$stage\scripts\" -Force -ErrorAction SilentlyContinue
    
    # README für Installation
    @"
# ThemisDB 1.0.0 - Windows Installation

## Schnellstart

1. Binary ausführbar machen:
   ``````powershell
   cd bin
   .\themis_server.exe --config ..\config\config.json
   ``````

2. Als Windows-Service installieren (Administrator):
   ``````powershell
   sc.exe create ThemisDB binPath= "C:\Program Files\ThemisDB\bin\themis_server.exe --config C:\Program Files\ThemisDB\config\config.json"
   sc.exe start ThemisDB
   ``````

3. Konfiguration anpassen:
   - Editiere ``config\config.json``
   - Setze ``rocksdb_path`` auf gewünschten Daten-Pfad
   - Passe ``port`` und ``worker_threads`` an

## Verzeichnisstruktur

- ``bin\`` - ThemisDB Server Binary
- ``config\`` - Konfigurationsdateien
- ``docs\`` - Dokumentation
- ``examples\`` - Code-Beispiele
- ``tools\`` - Admin-Tools
- ``scripts\`` - PowerShell-Scripts

Siehe README.md für vollständige Dokumentation.
"@ | Out-File -FilePath "$stage\INSTALL.txt" -Encoding UTF8
    
    # ZIP erstellen
    if (Test-Path $outZip) { Remove-Item $outZip -Force }
    Compress-Archive -Path "$stage\*" -DestinationPath $outZip -Force
    Remove-Item $stage -Recurse -Force
    
    $hash = Sha256 $outZip
    "$hash  $(Split-Path -Leaf $outZip)" | Out-File -FilePath (Join-Path $dist 'SHA256SUMS') -Append -Encoding ASCII
    Write-Host "  → $outZip" -ForegroundColor Green
} else {
    Write-Warning "Windows Binary nicht gefunden (erwartet build-msvc-ninja-release). Überspringe Packaging."
}

Write-Host "Fertig. Artefakte in: $dist" -ForegroundColor Green
