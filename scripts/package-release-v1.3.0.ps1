param(
    [switch]$SkipStrip,
    [string]$Version = "1.3.0"
)

$ErrorActionPreference = 'Stop'

Write-Host "=== Package ThemisDB v$Version Release Artefakte ===" -ForegroundColor Cyan

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = Split-Path -Parent $root
Set-Location $repo

# Ensure release folder
$releaseDir = Join-Path $repo "release"
New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null

function Sha256($path) {
    (Get-FileHash -Algorithm SHA256 -Path $path).Hash.ToLower()
}

# Clear SHA256SUMS
$checksumFile = Join-Path $releaseDir 'SHA256SUMS.txt'
if (Test-Path $checksumFile) { Remove-Item $checksumFile -Force }

Write-Host "`n=== Windows MSVC Release Package ===" -ForegroundColor Yellow

# Package Windows MSVC Release
$binWin = Join-Path $repo 'build-msvc\Release\themis_server.exe'
if (-not (Test-Path $binWin)) {
    $binWin = Join-Path $repo 'build-msvc-ninja-release\themis_server.exe'
}

if (Test-Path $binWin) {
    Write-Host "[Windows] Erstelle Release-Package für v$Version" -ForegroundColor Cyan
    $outZip = Join-Path $releaseDir "themisdb-v$Version-windows-x64.zip"
    $stage = Join-Path $releaseDir "themisdb-$Version"
    
    # Erstelle Package-Struktur
    New-Item -ItemType Directory -Force -Path "$stage\bin" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stage\config" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stage\docs" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stage\examples" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stage\tools" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stage\scripts" | Out-Null
    New-Item -ItemType Directory -Force -Path "$stage\models" | Out-Null
    
    # Binary
    Write-Host "  Kopiere Binary: $binWin" -ForegroundColor DarkGray
    Copy-Item $binWin -Destination "$stage\bin\themis_server.exe" -Force
    
    # Check for LLM DLLs
    $buildDir = Split-Path -Parent $binWin
    $llamaDll = Join-Path $buildDir "llama.dll"
    if (Test-Path $llamaDll) {
        Write-Host "  Kopiere llama.dll (LLM support)" -ForegroundColor DarkGray
        Copy-Item $llamaDll -Destination "$stage\bin\" -Force
    }
    
    # Copy GGML DLLs
    Get-ChildItem "$buildDir\ggml*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
        Write-Host "  Kopiere $($_.Name) (GGML backend)" -ForegroundColor DarkGray
        Copy-Item $_.FullName -Destination "$stage\bin\" -Force
    }
    
    # Copy vcpkg DLLs
    $vcpkgLibDir = Join-Path $repo "vcpkg_installed\x64-windows\bin"
    if (Test-Path $vcpkgLibDir) {
        Write-Host "  Kopiere vcpkg DLLs..." -ForegroundColor DarkGray
        Get-ChildItem "$vcpkgLibDir\*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
            Copy-Item $_.FullName -Destination "$stage\bin\" -Force
        }
    }
    
    # Config-Dateien
    Write-Host "  Kopiere Konfigurationsdateien..." -ForegroundColor DarkGray
    Copy-Item "$repo\config\*.json" -Destination "$stage\config\" -Force -ErrorAction SilentlyContinue
    Copy-Item "$repo\config\*.yaml" -Destination "$stage\config\" -Force -ErrorAction SilentlyContinue
    if (Test-Path "$repo\config\processors") {
        Copy-Item "$repo\config\processors" -Destination "$stage\config\" -Recurse -Force
    }
    if (Test-Path "$repo\config\schemas") {
        Copy-Item "$repo\config\schemas" -Destination "$stage\config\" -Recurse -Force
    }
    
    # LLM Config Template
    @"
# ThemisDB v$Version - LLM Configuration
llm:
  enabled: true
  plugin: llamacpp
  model:
    path: ./models/mistral-7b-instruct-v0.2.Q4_K_M.gguf
    n_gpu_layers: 32  # GPU offload layers (set to 0 for CPU-only)
    n_ctx: 4096       # Context window
    n_batch: 512      # Batch size
  cache:
    max_models: 3
    max_vram_mb: 24576  # 24 GB
    ttl_minutes: 30
"@ | Out-File -FilePath "$stage\config\llm_config.yaml" -Encoding UTF8
    
    # Dokumentation
    Write-Host "  Kopiere Dokumentation..." -ForegroundColor DarkGray
    Copy-Item "$repo\README.md" -Destination "$stage\" -Force
    Copy-Item "$repo\LICENSE" -Destination "$stage\" -Force
    Copy-Item "$repo\CHANGELOG.md" -Destination "$stage\" -Force -ErrorAction SilentlyContinue
    Copy-Item "$repo\SECURITY.md" -Destination "$stage\" -Force -ErrorAction SilentlyContinue
    Copy-Item "$repo\RELEASE_NOTES_v$Version.md" -Destination "$stage\" -Force -ErrorAction SilentlyContinue
    
    # LLM Documentation
    if (Test-Path "$repo\docs\llm") {
        Write-Host "  Kopiere LLM-Dokumentation..." -ForegroundColor DarkGray
        New-Item -ItemType Directory -Force -Path "$stage\docs\llm" | Out-Null
        Copy-Item "$repo\docs\llm\*.md" -Destination "$stage\docs\llm\" -Force -ErrorAction SilentlyContinue
    }
    
    # OpenAPI specification
    if (Test-Path "$repo\openapi") {
        Write-Host "  Kopiere OpenAPI specs..." -ForegroundColor DarkGray
        Copy-Item "$repo\openapi" -Destination "$stage\" -Recurse -Force
    }
    
    # Client libraries (SDKs)
    if (Test-Path "$repo\clients") {
        Write-Host "  Kopiere Client SDKs..." -ForegroundColor DarkGray
        Copy-Item "$repo\clients" -Destination "$stage\" -Recurse -Force
    }
    
    # Beispiele
    if (Test-Path "$repo\examples") {
        Write-Host "  Kopiere Beispiele..." -ForegroundColor DarkGray
        Copy-Item "$repo\examples" -Destination "$stage\examples\" -Recurse -Force -ErrorAction SilentlyContinue
    }
    
    # Tools
    if (Test-Path "$repo\tools\plugin_signer") {
        Copy-Item "$repo\tools\plugin_signer" -Destination "$stage\tools\" -Recurse -Force
    }
    
    # Scripts
    Copy-Item "$repo\scripts\*.ps1" -Destination "$stage\scripts\" -Force -ErrorAction SilentlyContinue
    
    # Installation README
    @"
# ThemisDB v$Version - Windows Installation

**Release Date:** 20. Dezember 2025  
**Code Name:** "Keep Your Own Llamas"

## 🚀 Schnellstart

### 1. Extrahieren
Entpacken Sie das ZIP-Archiv in ein Verzeichnis Ihrer Wahl, z.B.:
``````
C:\Program Files\ThemisDB\
``````

### 2. Server starten (ohne LLM)
``````powershell
cd bin
.\themis_server.exe --config ..\config\config.json
``````

### 3. Server mit LLM-Support starten
``````powershell
# Laden Sie ein GGUF-Modell herunter (z.B. Mistral 7B)
# Speichern Sie es in: ..\models\

cd bin
.\themis_server.exe --config ..\config\llm_config.yaml
``````

## 🧠 LLM Features

ThemisDB v$Version enthält **native LLM-Integration** mit llama.cpp:

- ✅ Embedded llama.cpp inference engine
- ✅ GPU acceleration (CUDA)
- ✅ GGUF model support (LLaMA 3, Mistral, Phi-3, etc.)
- ✅ Lazy loading with caching
- ✅ Multi-LoRA adapter support

### Modelle herunterladen

Empfohlene Modelle von HuggingFace:

``````powershell
# Mistral 7B Instruct Q4 (~4GB)
curl -L -o models/mistral-7b-instruct-v0.2.Q4_K_M.gguf \
  https://huggingface.co/TheBloke/Mistral-7B-Instruct-v0.2-GGUF/resolve/main/mistral-7b-instruct-v0.2.Q4_K_M.gguf

# LLaMA 3 8B Instruct Q4 (~4.5GB)
curl -L -o models/llama-3-8b-instruct.Q4_K_M.gguf \
  https://huggingface.co/TheBloke/LLaMA-3-8B-Instruct-GGUF/resolve/main/llama-3-8b-instruct.Q4_K_M.gguf
``````

## 🔧 Als Windows-Service installieren

``````powershell
# Administrator-PowerShell öffnen
sc.exe create ThemisDB binPath= "C:\Program Files\ThemisDB\bin\themis_server.exe --config C:\Program Files\ThemisDB\config\llm_config.yaml"
sc.exe description ThemisDB "ThemisDB Multi-Model Database with LLM Support"
sc.exe start ThemisDB
``````

## 📂 Verzeichnisstruktur

- ``bin\`` - ThemisDB Server Binary + LLM DLLs
- ``config\`` - Konfigurationsdateien
- ``docs\`` - Dokumentation (inkl. LLM guides)
- ``examples\`` - Code-Beispiele
- ``tools\`` - Admin-Tools
- ``scripts\`` - PowerShell-Scripts
- ``models\`` - GGUF-Modelle (leer, selbst herunterladen)

## 🌐 HTTP API

ThemisDB läuft standardmäßig auf:
- **Core API:** http://localhost:8765
- **LLM API:** http://localhost:8765/api/llm/generate

Beispiel:
``````powershell
curl -X POST http://localhost:8765/api/llm/generate \
  -H "Content-Type: application/json" \
  -d '{
    "prompt": "What is ThemisDB?",
    "max_tokens": 512,
    "temperature": 0.7
  }'
``````

## 📚 Weitere Dokumentation

- **README.md** - Überblick
- **RELEASE_NOTES_v$Version.md** - Was ist neu?
- **docs/llm/** - LLM-Integration Guides
- **docs/deployment/** - Deployment-Strategien

## 🐛 Troubleshooting

### "DLL not found" Fehler
Alle DLLs sollten im ``bin\`` Ordner liegen. Prüfen Sie:
``````powershell
dir bin\*.dll
``````

### GPU wird nicht erkannt
Stellen Sie sicher, dass CUDA installiert ist (falls NVIDIA GPU):
``````powershell
nvidia-smi
``````

Setzen Sie in ``config\llm_config.yaml``:
``````yaml
llm:
  model:
    n_gpu_layers: 0  # Fallback auf CPU
``````

### Weitere Hilfe
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://makr-code.github.io/ThemisDB/

---

**Version:** $Version  
**Build Date:** $((Get-Date).ToString("yyyy-MM-dd"))  
**License:** MIT
"@ | Out-File -FilePath "$stage\INSTALL.txt" -Encoding UTF8
    
    # ZIP erstellen
    Write-Host "  Erstelle ZIP-Archiv..." -ForegroundColor DarkGray
    if (Test-Path $outZip) { Remove-Item $outZip -Force }
    Compress-Archive -Path "$stage\*" -DestinationPath $outZip -CompressionLevel Optimal -Force
    Remove-Item $stage -Recurse -Force
    
    $hash = Sha256 $outZip
    "$hash  $(Split-Path -Leaf $outZip)" | Out-File -FilePath $checksumFile -Append -Encoding ASCII
    
    $size = (Get-Item $outZip).Length / 1MB
    $sizeFormatted = [math]::Round($size, 2)
    Write-Host "  checkmark $outZip ($sizeFormatted MB)" -ForegroundColor Green
} else {
    Write-Warning "Windows Binary nicht gefunden. Erwartet: $binWin"
    Write-Warning "Baue zuerst mit: .\scripts\build-themis-server-llm.ps1"
}

Write-Host "`n=== Release Checksums ===" -ForegroundColor Yellow
if (Test-Path $checksumFile) {
    Get-Content $checksumFile | ForEach-Object {
        Write-Host "  $_" -ForegroundColor Cyan
    }
} else {
    Write-Warning "Keine Checksums generiert (keine Pakete erstellt)"
}

Write-Host "`nFertig! Release-Artefakte in: $releaseDir" -ForegroundColor Green
Write-Host "`nNächste Schritte:" -ForegroundColor Yellow
Write-Host "  1. Git Tag erstellen: git tag -a v$Version -m 'Release v$Version'" -ForegroundColor White
Write-Host "  2. Tag pushen: git push origin v$Version" -ForegroundColor White
Write-Host "  3. GitHub Release erstellen und $releaseDir\* hochladen" -ForegroundColor White
