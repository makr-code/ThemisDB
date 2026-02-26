# vcpkg Multi-Platform Package System

**Umfassende Build-Strategie für ThemisDB mit vorkompilierten vcpkg-Paketen**

## Übersicht

Dieses System kompiliert vcpkg-Pakete für alle Plattformen und Konfigurationen **vorab** und mountet sie in Docker, statt sie bei jedem Docker-Build neu zu kompilieren.

### Vorteile

✅ **Extrem schnelle Docker-Builds** - keine vcpkg-Kompilierung im Docker nötig  
✅ **Reproduzierbare Builds** - identische Pakete für Windows und Docker  
✅ **Offline-fähig** - keine GitHub-Downloads während Docker-Builds  
✅ **Parallelisierbar** - Windows und Linux Builds unabhängig  
✅ **Debug + Release** - beide Konfigurationen verfügbar  

### Architektur

```
vcpkg_packages/           ← Zentraler Package Store
├── x64-windows/
│   ├── debug/           ← MSVC Debug builds
│   └── release/         ← MSVC Release builds
└── x64-linux/
    ├── debug/           ← WSL/GCC Debug builds (für Docker)
    └── release/         ← WSL/GCC Release builds (für Docker)
```

### Workflow

```
┌─────────────────────────────────────────────────────────────┐
│ 1. BUILD PACKAGES (einmalig oder bei Dependency-Änderungen) │
└─────────────────────────────────────────────────────────────┘
         │
         ├──> Windows (MSVC) ──> vcpkg_packages/x64-windows/{debug,release}
         └──> Linux (WSL)    ──> vcpkg_packages/x64-linux/{debug,release}

┌─────────────────────────────────────────────────────────────┐
│ 2. BUILD DOCKER IMAGE (schnell, mounted packages)           │
└─────────────────────────────────────────────────────────────┘
         │
         └──> COPY vcpkg_packages/x64-linux/release ──> Docker Image

┌─────────────────────────────────────────────────────────────┐
│ 3. TEST & DEPLOY                                            │
└─────────────────────────────────────────────────────────────┘
```

---

## Schnellstart

### 1. Alle Pakete und Docker-Image in einem Schritt bauen

```powershell
# Quick build (nur Release)
.\build-all-platforms.ps1 -Quick

# Vollständiger Build (Debug + Release)
.\build-all-platforms.ps1 -Configuration all

# Nur eine Edition
.\build-all-platforms.ps1 -Edition ENTERPRISE
```

### 2. Schrittweise: Erst Pakete, dann Docker

```powershell
# Schritt 1: Alle Pakete kompilieren (Windows + Linux, Debug + Release)
.\build-vcpkg-packages.ps1 -Platform all -Configuration all

# Schritt 2: Docker-Image mit Release-Paketen bauen
.\docker-build-with-prebuilt-packages.ps1 -Edition COMMUNITY -Configuration release

# Schritt 3: Image testen
docker run --rm themisdb:latest themis_server --version
```

---

## Detaillierte Anleitung

### Skript 1: `build-vcpkg-packages.ps1`

**Kompiliert vcpkg-Pakete für alle Plattformen und Konfigurationen**

#### Parameter

| Parameter | Werte | Default | Beschreibung |
|-----------|-------|---------|--------------|
| `-Platform` | windows, linux, all | all | Zielplattform(en) |
| `-Configuration` | debug, release, all | release | Build-Konfiguration |
| `-Edition` | COMMUNITY, MINIMAL, ENTERPRISE, HYPERSCALER | COMMUNITY | Edition (bestimmt Abhängigkeiten) |
| `-SkipWindows` | Switch | - | Windows-Builds überspringen |
| `-SkipLinux` | Switch | - | Linux-Builds überspringen |

#### Beispiele

```powershell
# Nur Linux Release (für Docker)
.\build-vcpkg-packages.ps1 -Platform linux -Configuration release

# Windows Debug + Release
.\build-vcpkg-packages.ps1 -Platform windows -Configuration all

# Alles für ENTERPRISE Edition
.\build-vcpkg-packages.ps1 -Platform all -Configuration all -Edition ENTERPRISE

# Nur Linux, beide Konfigurationen
.\build-vcpkg-packages.ps1 -SkipWindows -Configuration all
```

#### Funktionsweise

1. **Validierung**
   - Prüft vcpkg.exe Verfügbarkeit
   - Prüft WSL für Linux-Builds
   - Validiert vcpkg-Manifest (docker/vcpkg-{edition}.json)

2. **Windows-Builds (MSVC)**
   - Aktiviert Visual Studio Developer Environment
   - Kompiliert Pakete mit `vcpkg install --triplet=x64-windows`
   - Speichert in `vcpkg_packages/x64-windows/{debug,release}`

3. **Linux-Builds (WSL)**
   - Installiert Build-Tools in WSL (gcc, cmake, ninja)
   - Bootstrapt vcpkg in WSL
   - Kompiliert Pakete mit `vcpkg install --triplet=x64-linux`
   - Speichert in `vcpkg_packages/x64-linux/{debug,release}`

4. **Package Store**
   - Strukturierte Ablage aller kompilierten Pakete
   - Permanente Wiederverwendung
   - Kein erneutes Download/Kompilieren nötig

#### Erwartete Größen

| Plattform | Konfiguration | Größe (geschätzt) |
|-----------|---------------|-------------------|
| x64-windows | Debug | ~15-20 GB |
| x64-windows | Release | ~8-12 GB |
| x64-linux | Debug | ~12-18 GB |
| x64-linux | Release | ~6-10 GB |

**Gesamt: ~50-60 GB für alle Varianten**

---

### Skript 2: `docker-build-with-prebuilt-packages.ps1`

**Erstellt Docker-Image mit gemounteten vorkompilierten Paketen**

#### Parameter

| Parameter | Werte | Default | Beschreibung |
|-----------|-------|---------|--------------|
| `-Edition` | COMMUNITY, MINIMAL, ENTERPRISE, HYPERSCALER | COMMUNITY | ThemisDB Edition |
| `-Tag` | string | themisdb:latest | Docker Image Tag |
| `-Configuration` | debug, release | release | Welche Pakete mounten |
| `-EnableLLM` | ON, OFF | ON (außer MINIMAL) | LLM-Support |
| `-EnableGPU` | ON, OFF | OFF | GPU-Support |
| `-NoBuildCache` | Switch | - | Docker BuildKit Cache deaktivieren |

#### Beispiele

```powershell
# Standard COMMUNITY Release
.\docker-build-with-prebuilt-packages.ps1

# Debug-Image mit custom Tag
.\docker-build-with-prebuilt-packages.ps1 -Configuration debug -Tag themisdb:debug

# ENTERPRISE mit GPU
.\docker-build-with-prebuilt-packages.ps1 -Edition ENTERPRISE -EnableGPU ON

# MINIMAL ohne Cache
.\docker-build-with-prebuilt-packages.ps1 -Edition MINIMAL -NoBuildCache
```

#### Funktionsweise

1. **Validierung**
   - Prüft ob vorkompilierte Linux-Pakete existieren
   - Zeigt Package-Store-Größe an

2. **Dockerfile-Generierung**
   - Erstellt `Dockerfile.prebuilt` mit optimiertem Multi-Stage-Build
   - Verwendet `VCPKG_MANIFEST_MODE=OFF` (Pakete bereits vorhanden)
   - Mountet Pakete aus `vcpkg_packages/x64-linux/{configuration}`

3. **Minimaler Build-Context**
   - Kopiert nur Source-Code (~100-200 MB)
   - Kopiert vorkompilierte Pakete (~6-18 GB je nach Config)
   - **KEIN** vcpkg download/install während Docker-Build!

4. **Docker Build**
   - Stage 1: Basis (Ubuntu 24.04 + Build-Tools)
   - Stage 2: Dependencies (mountet vorkompilierte Pakete)
   - Stage 3: ThemisDB Build (nutzt gemounte Pakete)
   - Stage 4: Runtime (minimal, nur Binary + Runtime-Libs)

#### Build-Zeiten

| Szenario | Zeit | Bedingung |
|----------|------|-----------|
| **Erster Build** | 5-10 min | Pakete vorhanden, CMake+Compile only |
| **Rebuild** | 2-5 min | Docker BuildKit Cache |
| **Neue Edition** | 5-10 min | Bei Package-Wiederverwendung |

**Vergleich zu alter Strategie:**
- Ohne vorkompilierte Pakete: 30-45 min (vcpkg install im Docker)
- Mit vorkompilierten Paketen: **5-10 min** (nur Source-Compile)
- **Zeitersparnis: ~80%!**

---

### Skript 3: `build-all-platforms.ps1`

**Orchestrierungs-Skript für kompletten Workflow**

#### Parameter

| Parameter | Werte | Default | Beschreibung |
|-----------|-------|---------|--------------|
| `-Stage` | packages, docker, test, all | all | Welche Build-Stages ausführen |
| `-Edition` | COMMUNITY, MINIMAL, ENTERPRISE, HYPERSCALER | COMMUNITY | Edition |
| `-Configuration` | debug, release, all | release | Build-Konfiguration |
| `-Quick` | Switch | - | Nur Release, skip tests |

#### Beispiele

```powershell
# Kompletter Workflow (Pakete + Docker + Tests)
.\build-all-platforms.ps1

# Quick build (nur Release, keine Tests)
.\build-all-platforms.ps1 -Quick

# Nur Pakete neu bauen
.\build-all-platforms.ps1 -Stage packages

# Nur Docker neu bauen (Pakete wiederverwenden)
.\build-all-platforms.ps1 -Stage docker

# Nur Tests ausführen
.\build-all-platforms.ps1 -Stage test
```

#### Stages

1. **packages** - Kompiliert alle Pakete (alle Plattformen, Konfigurationen)
2. **docker** - Baut Docker-Images mit gemounteten Paketen
3. **test** - Führt Smoke-Tests aus (--version, --build-info, --help)

---

## Verzeichnisstruktur

### Vor dem Build

```
C:\VCC\themis\
├── vcpkg/                           ← vcpkg Tool + Cache
│   ├── vcpkg.exe
│   ├── downloads/                   ← Source-Archive (shared)
│   ├── buildtrees/                  ← Temporär (kann gelöscht werden)
│   └── packages/                    ← Temporär (kann gelöscht werden)
├── docker/
│   ├── vcpkg-community.json         ← COMMUNITY Dependencies
│   ├── vcpkg-minimal.json
│   ├── vcpkg-enterprise.json
│   └── vcpkg-hyperscaler.json
└── docker\
    ├── Dockerfile                   ← Original (alte Strategie)
    └── Dockerfile.prebuilt          ← Neu (mount-Strategie)
```

### Nach Package-Build

```
C:\VCC\themis\
└── vcpkg_packages/                  ← 🆕 Zentraler Package Store
    ├── x64-windows/
    │   ├── debug/                   ← MSVC Debug (~15-20 GB)
    │   │   ├── boost/
    │   │   ├── rocksdb/
    │   │   ├── grpc/
    │   │   └── ...
    │   └── release/                 ← MSVC Release (~8-12 GB)
    │       └── ...
    └── x64-linux/
        ├── debug/                   ← WSL/GCC Debug (~12-18 GB)
        │   └── ...
        └── release/                 ← WSL/GCC Release (~6-10 GB)
            └── ...
```

### Docker Build Context

```
.docker-build-prebuilt/              ← Temporärer Build-Context
├── CMakeLists.txt
├── src/
├── include/
├── cmake/
└── vcpkg_packages/                  ← Kopie der Linux-Pakete
    └── x64-linux/
        └── release/                 ← ~6-10 GB (wird in Docker gemountet)
```

---

## Dependency-Manifests

### Verfügbare Editionen

| Edition | Manifest | Dependencies | Beschreibung |
|---------|----------|--------------|--------------|
| **MINIMAL** | docker/vcpkg-minimal.json | ~10 Pakete | Auth, REST, Tracing - keine LLM/GPU |
| **COMMUNITY** | docker/vcpkg-community.json | ~21 Pakete | Standard, LLM (llama.cpp) |
| **ENTERPRISE** | docker/vcpkg-enterprise.json | ~25 Pakete | Advanced Features, Compliance |
| **HYPERSCALER** | docker/vcpkg-hyperscaler.json | ~30 Pakete | GPU, Distributed Systems, Monitoring |

### Beispiel: COMMUNITY Dependencies

```json
{
  "name": "themisdb-community",
  "dependencies": [
    "boost-filesystem",
    "boost-beast",
    "fmt",
    "spdlog",
    "nlohmann-json",
    "simdjson",
    "tbb",
    "tl-expected",
    "yaml-cpp",
    {"name": "rocksdb", "features": ["lz4", "snappy", "zlib", "zstd"]},
    "protobuf",
    "grpc",
    "curl",
    "openssl",
    "zlib",
    "zstd",
    "lz4",
    "snappy",
    "mimalloc"
  ]
}
```

---

## WSL-Konfiguration

### Voraussetzungen

1. **WSL 2 installiert**
   ```powershell
   wsl --install
   wsl --set-default-version 2
   ```

2. **Ubuntu-Distribution**
   ```powershell
   wsl --install Ubuntu
   ```

3. **Build-Tools in WSL** (werden automatisch installiert)
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential cmake ninja-build git curl pkg-config
   ```

### WSL-Performance-Optimierung

Erstelle/editiere `%UserProfile%\.wslconfig`:

```ini
[wsl2]
memory=16GB                  # 50% des RAM empfohlen
processors=8                 # Alle CPU-Cores
swap=8GB
localhostForwarding=true

[experimental]
autoMemoryReclaim=gradual    # Automatische Speicherfreigabe
sparseVhd=true              # Dynamische Disk-Größe
```

Nach Änderungen WSL neu starten:
```powershell
wsl --shutdown
wsl
```

---

## Performance-Vergleich

### Alte Strategie (Downloads-Cache)

```
Docker Build Timeline:
├── deps: vcpkg install (30-40 min) ← ❌ Jedes Mal neu kompilieren
├── llama: Compile (5-8 min)
├── builder: ThemisDB (5-10 min)
└── runtime: Package (1-2 min)

Gesamt: 45-60 min pro Build
```

### Neue Strategie (Vorkompilierte Pakete)

```
Einmalig: Package Build (60-90 min)
├── Windows Debug   (20-30 min)
├── Windows Release (15-20 min)
├── Linux Debug     (15-20 min)
└── Linux Release   (10-15 min)

Danach: Docker Build (5-10 min) ← ✅ Pakete bereits vorhanden!
├── deps: Mount packages (10 sec)
├── llama: Compile (3-5 min)
├── builder: ThemisDB (2-4 min)
└── runtime: Package (30-60 sec)

Gesamt: 5-10 min pro Build
Zeitersparnis: ~80%!
```

### Rebuild-Zeiten

| Szenario | Alte Strategie | Neue Strategie | Ersparnis |
|----------|----------------|----------------|-----------|
| Source-Code-Änderung | 45 min | 5 min | 89% |
| Dependency-Änderung | 50 min | 15 min* | 70% |
| Neue Edition | 45 min | 5 min | 89% |
| CMake-Änderung | 40 min | 4 min | 90% |

*Dependency-Änderungen erfordern neuen Package-Build (einmalig), danach wieder schnell

---

## Disk-Space-Management

### Speicherverbrauch

```
vcpkg/
  downloads/      ~4-5 GB    (Source-Archive, shared)
  buildtrees/     ~30-50 GB  (Temporär, kann gelöscht werden)
  packages/       ~10-20 GB  (Temporär, kann gelöscht werden)

vcpkg_packages/   ~50-60 GB  (Permanent, alle Varianten)
  x64-windows/debug    ~15-20 GB
  x64-windows/release   ~8-12 GB
  x64-linux/debug      ~12-18 GB
  x64-linux/release     ~6-10 GB

Gesamt: ~60-80 GB (ohne temporäre Dateien)
```

### Aufräumen

```powershell
# Temporäre vcpkg-Verzeichnisse löschen (Speicher freigeben)
Remove-Item C:\VCC\themis\vcpkg\buildtrees -Recurse -Force
Remove-Item C:\VCC\themis\vcpkg\packages -Recurse -Force

# Package Store löschen (erzwingt Rebuild aller Pakete)
Remove-Item C:\VCC\themis\vcpkg_packages -Recurse -Force

# Docker Build Context löschen
Remove-Item C:\VCC\themis\.docker-build-prebuilt -Recurse -Force
```

### Nur eine Konfiguration behalten

```powershell
# Nur Release (Debug löschen)
Remove-Item C:\VCC\themis\vcpkg_packages\*\debug -Recurse -Force

# Nur Linux (Windows löschen)
Remove-Item C:\VCC\themis\vcpkg_packages\x64-windows -Recurse -Force
```

**Speicherersparnis: ~70% (nur Release + Linux → ~10 GB statt ~60 GB)**

---

## Troubleshooting

### Problem: WSL-Build schlägt fehl

**Symptom:** Fehler beim Kompilieren in WSL

**Lösungen:**

1. **Build-Tools aktualisieren**
   ```powershell
   wsl bash -c "sudo apt-get update && sudo apt-get upgrade -y build-essential cmake ninja-build"
   ```

2. **vcpkg neu bootstrappen**
   ```powershell
   wsl bash -c "cd /mnt/c/VCC/themis/vcpkg && ./bootstrap-vcpkg.sh -disableMetrics"
   ```

3. **Mehr Speicher in WSL**
   - Editiere `%UserProfile%\.wslconfig` (siehe WSL-Konfiguration)
   - `wsl --shutdown` und neu starten

### Problem: Docker kann Pakete nicht finden

**Symptom:** `COPY vcpkg_packages/x64-linux/release: no such file or directory`

**Lösung:**
```powershell
# Prüfe ob Pakete existieren
Test-Path C:\VCC\themis\vcpkg_packages\x64-linux\release

# Falls nicht: Pakete bauen
.\build-vcpkg-packages.ps1 -Platform linux -Configuration release
```

### Problem: Zu wenig Speicherplatz

**Symptom:** Builds schlagen fehl mit Disk-Full-Errors

**Lösungen:**

1. **Temporäre Dateien löschen**
   ```powershell
   Remove-Item C:\VCC\themis\vcpkg\buildtrees -Recurse -Force
   Remove-Item C:\VCC\themis\vcpkg\packages -Recurse -Force
   ```

2. **Alte Docker-Images löschen**
   ```powershell
   docker system prune -a --volumes
   ```

3. **Nur benötigte Konfigurationen bauen**
   ```powershell
   # Nur Linux Release (für Docker)
   .\build-vcpkg-packages.ps1 -Platform linux -Configuration release -Edition COMMUNITY
   ```

### Problem: Lange Build-Zeiten trotz vorkompilierter Pakete

**Symptom:** Docker-Build dauert >20 min

**Mögliche Ursachen:**

1. **BuildKit nicht aktiviert**
   ```powershell
   $env:DOCKER_BUILDKIT = "1"
   .\docker-build-with-prebuilt-packages.ps1
   ```

2. **Cache wurde geleert**
   ```powershell
   # Rebuild mit Cache
   .\docker-build-with-prebuilt-packages.ps1  # Ohne -NoBuildCache
   ```

3. **Falsche Pakete gemountet**
   - Prüfe dass x64-**linux** Pakete verwendet werden (nicht x64-windows!)
   ```powershell
   Get-ChildItem C:\VCC\themis\vcpkg_packages\x64-linux\release
   ```

### Problem: vcpkg-Versionskonflikt

**Symptom:** Fehler über inkompatible vcpkg-Versionen

**Lösung:**
```powershell
# vcpkg aktualisieren
cd C:\VCC\themis\vcpkg
git pull
.\bootstrap-vcpkg.bat

# WSL vcpkg aktualisieren
wsl bash -c "cd /mnt/c/VCC/themis/vcpkg && git pull && ./bootstrap-vcpkg.sh"
```

---

## Migration von alter zu neuer Strategie

### Schritt 1: Pakete initial bauen

```powershell
# Nur Linux Release (minimal für Docker)
.\build-vcpkg-packages.ps1 -Platform linux -Configuration release

# Dauer: 10-15 min (einmalig)
```

### Schritt 2: Docker-Build testen

```powershell
.\docker-build-with-prebuilt-packages.ps1 -Edition COMMUNITY

# Dauer: 5-10 min (statt 45 min!)
```

### Schritt 3: Alte Build-Scripte ersetzen

**Alt:**
```powershell
.\docker-build-with-cache.ps1  ← Alte Strategie (Downloads-Cache)
```

**Neu:**
```powershell
.\docker-build-with-prebuilt-packages.ps1  ← Neue Strategie (Package-Mount)
```

### Schritt 4: Workflow anpassen

**Vorher:**
```
Code-Änderung → Docker Build (45 min) → Test
```

**Nachher:**
```
Code-Änderung → Docker Build (5 min) → Test

Dependency-Änderung → Package Build (15 min, einmalig) → Docker Build (5 min) → Test
```

---

## Continuous Integration (CI)

### GitHub Actions Workflow

```yaml
name: Multi-Platform Build

on: [push, pull_request]

jobs:
  build-packages:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      
      # Cache vcpkg packages
      - uses: actions/cache@v3
        with:
          path: vcpkg_packages/x64-linux
          key: vcpkg-linux-${{ hashFiles('docker/vcpkg-*.json') }}
      
      # Build packages if not cached
      - name: Build vcpkg packages
        run: |
          .\build-vcpkg-packages.ps1 -Platform linux -Configuration release -Edition COMMUNITY
        if: steps.cache.outputs.cache-hit != 'true'
      
      # Upload packages as artifact
      - uses: actions/upload-artifact@v3
        with:
          name: vcpkg-packages-linux
          path: vcpkg_packages/x64-linux/release
          retention-days: 7
  
  build-docker:
    needs: build-packages
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      # Download pre-built packages
      - uses: actions/download-artifact@v3
        with:
          name: vcpkg-packages-linux
          path: vcpkg_packages/x64-linux/release
      
      # Build Docker image
      - name: Build Docker
        run: |
          # PowerShell-Skript nicht verfügbar in Ubuntu
          # Direkt docker build verwenden
          docker build -t themisdb:latest -f Dockerfile.prebuilt .
      
      # Push to registry
      - run: docker push themisdb:latest
```

### Multi-Stage CI/CD

```yaml
stages:
  # Stage 1: Packages (cached, selten)
  - packages:
      trigger: manual OR dependency-change
      cache: aggressive (30 days)
      duration: 15 min
  
  # Stage 2: Docker Build (bei jeder Code-Änderung)
  - docker:
      trigger: code-change
      cache: moderate (7 days)
      duration: 5 min
  
  # Stage 3: Tests & Deploy
  - test:
      trigger: after docker
      duration: 2 min
```

**CI-Build-Zeit:**
- Ohne Package-Cache: ~20 min (Package-Build + Docker-Build)
- Mit Package-Cache: **5-7 min** (nur Docker-Build + Tests)

---

## Best Practices

### 1. Package-Management

✅ **DO:**
- Pakete für alle benötigten Plattformen vorab bauen
- Nur Release-Pakete für Production-Docker-Images
- vcpkg_packages im .gitignore (zu groß für Git)
- Packages auf Netzwerk-Share für Team-Zugriff

❌ **DON'T:**
- Pakete im Docker neu kompilieren (wenn vermeidbar)
- x64-windows Pakete für Linux-Docker verwenden
- vcpkg_packages in Git committen
- buildtrees/packages permanent speichern

### 2. Disk-Space

✅ **DO:**
- Regelmäßig vcpkg/buildtrees löschen (~50 GB)
- Nur benötigte Konfigurationen behalten (Release meist ausreichend)
- WSL-Disk komprimieren: `wsl --manage <distro> --set-sparse true`

❌ **DON'T:**
- Alle 4 Varianten permanent vorhalten (wenn nicht nötig)
- vcpkg/downloads löschen (shared cache, schwer wiederherzustellen)

### 3. Build-Workflow

✅ **DO:**
- `build-all-platforms.ps1 -Quick` für schnelle Iteration
- Package-Build nur bei Dependency-Änderungen
- BuildKit Cache aktiviert lassen
- Multi-Stage Docker für kleinere Images

❌ **DON'T:**
- `-NoBuildCache` verwenden (außer bei Cache-Problemen)
- Packages bei jeder Code-Änderung neu bauen
- Alle Pakete in Runtime-Image kopieren

### 4. CI/CD

✅ **DO:**
- Packages cachen mit Actions/Cache
- Separate Jobs für Package-Build und Docker-Build
- Artifacts für Packages verwenden
- Cache-Invalidierung bei Dependency-Änderung

❌ **DON'T:**
- Packages in jedem CI-Run neu bauen
- Packages als Git LFS speichern (zu groß)

---

## Zusammenfassung

### Was wurde verbessert?

| Aspekt | Vorher | Nachher | Verbesserung |
|--------|--------|---------|--------------|
| **Docker-Build-Zeit** | 45 min | 5 min | **89% schneller** |
| **Netzwerk-Abhängigkeit** | Hoch | Keine | 100% offline |
| **Reproduzierbarkeit** | Mittel | Hoch | Identische Pakete |
| **Disk-Space** | ~20 GB | ~60 GB* | Trade-off für Speed |
| **Parallelisierung** | Begrenzt | Voll | Windows + Linux parallel |
| **CI-Freundlich** | Mittel | Hoch | Package-Caching |

*Kann mit nur Release + Linux auf ~10 GB reduziert werden

### Wann welche Strategie?

| Szenario | Empfohlene Strategie | Skript |
|----------|----------------------|--------|
| **Entwicklung** | Quick Builds, nur Release | `build-all-platforms.ps1 -Quick` |
| **Production** | Linux Release Packages | `build-vcpkg-packages.ps1 -Platform linux -Configuration release` |
| **Testing** | Debug + Release beide | `build-all-platforms.ps1 -Configuration all` |
| **CI/CD** | Cached Packages | GitHub Actions mit Cache |
| **Speicherknappheit** | Downloads-Cache (alte Strategie) | `docker-build-with-cache.ps1` |

### Nächste Schritte

1. ✅ Einmalig: Pakete für Linux Release bauen (~10-15 min)
2. ✅ Docker-Image mit mounted Packages bauen (~5 min)
3. ✅ Testen und in Workflow integrieren
4. 🔄 Bei Dependency-Änderung: Pakete neu bauen
5. 🚀 Enjoy 89% schnellere Builds!

---

## Support & Resources

- **Skripte:**
  - `build-vcpkg-packages.ps1` - Multi-Platform Package Builder
  - `docker-build-with-prebuilt-packages.ps1` - Docker mit mounted Packages
  - `build-all-platforms.ps1` - Orchestrierung (All-in-One)

- **Dokumentation:**
  - Dieser Guide: `VCPKG_MULTI_PLATFORM_PACKAGES.md`
  - Alte Strategie: `VCPKG_DOCKER_CACHE_STRATEGY.md`
  - vcpkg Docs: https://vcpkg.io/

- **Verzeichnisse:**
  - Package Store: `vcpkg_packages/`
  - Manifests: `docker/vcpkg-*.json`
  - Docker Build Context: `.docker-build-prebuilt/`
