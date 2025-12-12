# ThemisDB Build System with Automatic Cache Updates

## Overview

Das neue Build-System aktualisiert automatisch den lokalen vcpkg-Cache vor jedem Build. Dies stellt sicher, dass immer die neuesten Paketversionen verwendet werden.

## Workflow

```
┌─────────────────────────────────────────┐
│  1. Update vcpkg Cache                  │
│     (vcpkg\downloads → Latest Sources)  │
└──────────────┬──────────────────────────┘
               │
               ├─→ Windows (MSVC)
               ├─→ Linux (GCC/Ninja)
               └─→ Docker (Multi-Arch)
```

## Skripte

### 1. `update-vcpkg-cache.ps1` (Core Tool)
Aktualisiert den vcpkg-Cache mit den neuesten Paketversionen.

**Funktionen:**
- Git-basiertes Update von vcpkg selbst
- Registry-Baseline aktualisieren
- Source-Archive pre-fetchen für alle Triplets
- Cache-Integrität prüfen

**Verwendung:**
```powershell
# Default: x64-linux, arm64-linux, x64-windows
.\scripts\update-vcpkg-cache.ps1

# Nur spezifische Triplets
.\scripts\update-vcpkg-cache.ps1 -Triplets @("x64-linux", "arm64-linux")

# Force-Update
.\scripts\update-vcpkg-cache.ps1 -Force
```

### 2. `build.ps1` (Main Entry Point)
Orchestriert alle Build-Targets mit automatischer Cache-Aktualisierung.

**Targets:**
- `windows` - MSVC Build (Standard)
- `linux` - GCC/Ninja Build
- `docker` - Docker Multi-Arch Build
- `all` - Alle Targets nacheinander

**Verwendung:**
```powershell
# Windows-Build (mit Cache-Update)
.\scripts\build.ps1

# Docker Multi-Arch Build
.\scripts\build.ps1 -Target docker

# Docker mit Push zu Registry
.\scripts\build.ps1 -Target docker -Push

# Nur AMD64 Docker-Image
.\scripts\build.ps1 -Target docker -Platforms "linux/amd64"

# Alle Builds (ohne Cache-Update)
.\scripts\build.ps1 -Target all -NoCache
```

### 3. `build-windows.ps1`
Windows-spezifischer Build (MSVC + CMake).

**Optionen:**
```powershell
-NoCache       # Cache-Update überspringen
-SkipTests     # Keine Tests kompilieren
-Debug         # Debug-Modus mit Tracing
-Config        # Release (Standard) oder Debug
```

### 4. `build-docker.ps1`
Docker-Build mit buildx.

**Optionen:**
```powershell
-Platforms     # "linux/amd64,linux/arm64" (Standard)
-Push          # Nach Build zu Registry pushen
-NoCache       # Docker Cache nicht verwenden
-Tag           # Image-Tag (Standard: 1.0.1)
-Registry      # Registry-Prefix (Standard: themisdb)
```

### 5. `build-linux.sh`
Linux-spezifischer Build (GCC + Ninja).

**Optionen:**
```bash
--no-cache     # Cache-Update überspringen
--skip-tests   # Keine Tests kompilieren
--debug        # Debug-Modus mit Tracing
```

## Workflow-Beispiele

### Szenario 1: Täglicher Entwicklungs-Build (Windows)
```powershell
# Cache aktualisiert, dann MSVC-Build
.\scripts\build.ps1

# Oder mit Caching-Kontrolle
.\scripts\build.ps1 -NoCache
```

### Szenario 2: Docker Production-Image
```powershell
# Cache aktualisiert, Multi-Arch Build, zu Registry pushen
.\scripts\build.ps1 -Target docker -Push

# Nur amd64 für schnelles Testing
.\scripts\build.ps1 -Target docker -Platforms "linux/amd64"
```

### Szenario 3: Kontinuierliche Integration (CI/CD)
```powershell
# All-in-One: Windows + Linux + Docker
.\scripts\build.ps1 -Target all

# Mit spezifischer Version
.\scripts\build.ps1 -Target docker -Tag "1.0.2" -Push
```

### Szenario 4: Schneller Lokaler Test (kein Cache-Update)
```powershell
# Überspringt 5+ Minuten Cache-Update
.\scripts\build.ps1 -NoCache
```

## Was wird aktualisiert?

### vcpkg Cache Update (`update-vcpkg-cache.ps1`)

1. **vcpkg selbst** (via Git)
   - Neueste Portfiles vom vcpkg-Repository
   - Baseline-Updates für Package-Versionen

2. **Source Archives** (`vcpkg\downloads\`)
   - Boost, OpenSSL, RocksDB, SimdJSON, TBB, etc.
   - Arrow, Abseil, Thrift, Libevent (falls optional)
   - ~2GB komprimierte Source-Dateien

3. **Triplet-Support**
   - `x64-linux`: x86_64 Linux
   - `arm64-linux`: ARM64 Linux (Raspberry Pi, Apple Silicon VM)
   - `x64-windows`: Windows x64

## Cache-Struktur

```
vcpkg/
├── downloads/          ← Source-Archive (~2GB)
│   ├── boostorg-*.tar.gz
│   ├── openssl-*.tar.gz
│   └── ...
├── packages/           ← Kompilierte Binäre (NICHT für Docker)
│   ├── *_x64-linux/
│   ├── *_x64-windows/
│   └── ...
├── buildtrees/         ← Temp Build-Dirs (NICHT für Docker)
│   └── ...
└── ports/              ← Port-Definitionen
```

### Was wird in Docker kopiert?
✅ `vcpkg/downloads/` - Source Archives (notwendig)
❌ `vcpkg/packages/` - Nicht nötig (werden in Docker neu gebaut)
❌ `vcpkg/buildtrees/` - Nicht nötig (temporär)

## Tipps & Tricks

### Cache-Größe reduzieren
```powershell
# Nur spezifische Triplets aktualisieren
.\scripts\update-vcpkg-cache.ps1 -Triplets @("x64-linux")
```

### Cache-Probleme beheben
```powershell
# Kompletter Neuaufbau (bricht ab bei Fehler)
Remove-Item vcpkg\downloads -Recurse -Force
.\scripts\update-vcpkg-cache.ps1
```

### Build ohne Netzwerk (Offline)
```powershell
# Cache ist bereits gefüllt → keine Downloads
.\scripts\build.ps1 -NoCache
```

### Performance-Tuning
```powershell
# Windows: Parallelisierung
cmake --build build-msvc --config Release --parallel 8

# Linux/Docker: Ninja parallel jobs
ninja -C build -j 8
```

## Fehlerbehandlung

### Problem: "vcpkg install failed"
```powershell
# Lösung: Cache aktualisieren und neu versuchen
.\scripts\update-vcpkg-cache.ps1 -Force
.\scripts\build.ps1
```

### Problem: "Subprocess aborted" (Netzwerkfehler)
```powershell
# Lösung: Mit Cache arbeiten, keine Online-Downloads
.\scripts\build.ps1 -NoCache
```

### Problem: "Docker buildx build failed"
```powershell
# Lösung: Nur amd64 testen, dann multi-arch
.\scripts\build-docker.ps1 -Platforms "linux/amd64"
```

## CI/CD Integration

### GitHub Actions
```yaml
- name: Update vcpkg cache
  run: .\scripts\update-vcpkg-cache.ps1

- name: Build ThemisDB
  run: .\scripts\build.ps1 -Target docker -Push
```

### GitLab CI
```yaml
cache:
  paths:
    - vcpkg/downloads/

before_script:
  - pwsh -File scripts/update-vcpkg-cache.ps1

build_docker:
  script:
    - pwsh -File scripts/build-docker.ps1 -Push
```

## Änderungen in diesem System

### Vorher (Probleme)
- ❌ Netzwerkabhängig (GitHub-Downloads schlagen fehl)
- ❌ Manuelle vcpkg-Verwaltung
- ❌ Keine konsistente Cache-Nutzung
- ❌ Unterschiedliche Build-Prozesse (Windows/Linux/Docker)

### Nachher (Lösungen)
- ✅ Offline-First mit vcpkg-Cache
- ✅ Automatische Cache-Aktualisierung
- ✅ Einheitliche Build-Schnittstelle
- ✅ Multi-Triplet Support (x64-linux, arm64-linux, x64-windows)
- ✅ Cache-Konsistenz über alle Builds

## Kontakt & Support

Bei Fragen oder Problemen:
```powershell
# Help anzeigen
Get-Help .\scripts\update-vcpkg-cache.ps1
Get-Help .\scripts\build.ps1
```
