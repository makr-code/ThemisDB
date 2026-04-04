# ThemisDB Docker Build - Best Practices Guide

## 🎯 Überblick

Dieses Repository enthält eine optimierte Docker Build-Konfiguration für ThemisDB, die Best Practices für Produktions-Deployments implementiert.

### ✨ Features

- ✅ **Multi-Stage Build** - Minimale Runtime-Image-Größe
- ✅ **BuildKit Caching** - Schnelle Rebuilds (2-5 Min statt 30+ Min)
- ✅ **vcpkg Binary Cache** - Wiederverwendung kompilierter Pakete
- ✅ **aria2 Downloads** - 10-16x schnellere vcpkg Downloads
- ✅ **Multi-Architecture** - AMD64, ARM64 Support
- ✅ **Health Checks** - Automatische Container-Überwachung
- ✅ **Non-Root User** - Sicherheit nach Best Practices
- ✅ **Edition Support** - MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER
- ✅ **Debug Image** - Separates Image mit Debug-Tools

---

## 🚀 Quick Start

### Voraussetzungen

- Docker Desktop 20.10+ mit BuildKit aktiviert
- Windows mit WSL2 (empfohlen) oder Linux
- 16GB RAM (8GB minimal)
- 50GB freier Speicher

### Einfacher Build

```powershell
# COMMUNITY Edition (Standard)
docker buildx build -t themisdb:community .

# Mit Docker Compose
docker compose up -d
```

### PowerShell Build-Skript

```powershell
# Standard Build (COMMUNITY Edition)
.\build-docker.ps1

# ENTERPRISE Edition mit LLM
.\build-docker.ps1 -Edition ENTERPRISE -EnableLLM

# Mit GPU Support
.\build-docker.ps1 -EnableLLM -EnableGPU

# Build und Push zu Registry
.\build-docker.ps1 -Registry ghcr.io/yourorg -Push
```

---

## 📦 Docker Images

### Runtime Image (Produktion)

Minimales Image für Produktions-Deployments:

```dockerfile
FROM themisdb:community AS runtime
# Größe: ~500MB
# User: themis (non-root)
# Ports: 8080 (HTTP), 8081 (gRPC), 9090 (Metrics)
```

**Build:**
```powershell
docker buildx build --target runtime -t themisdb:community .
```

### Debug Image (Entwicklung)

Erweitertes Image mit Debug-Tools:

```dockerfile
FROM themisdb:community AS debug
# Zusätzliche Tools: gdb, valgrind, strace, htop, vim
# Source Code: /src
```

**Build:**
```powershell
docker buildx build --target debug -t themisdb:community-debug .
```

---

## 🏗️ Build-Architektur

### Multi-Stage Build Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│ Stage 1: base                                               │
│ - Ubuntu 24.04                                              │
│ - Build tools (gcc, cmake, ninja)                           │
│ - vcpkg bootstrap                                           │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ Stage 2: deps                                               │
│ - vcpkg manifest selection (edition-specific)               │
│ - Dependency installation mit aria2                         │
│ - BuildKit cache mounts                                     │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ Stage 3: llama (parallel)                                   │
│ - llama.cpp compilation (wenn LLM=ON)                       │
│ - Shared library build                                      │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ Stage 4: build                                              │
│ - ThemisDB compilation                                      │
│ - CMake + Ninja (matching Windows config)                  │
│ - Binary: /src/build/themis_server                         │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ Stage 5: runtime (Produktion)                              │
│ - Minimale Ubuntu 24.04                                     │
│ - Nur themis_server Binary + Runtime-Libs                  │
│ - Non-root user                                             │
│ - Health checks                                             │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ Stage 6: debug (Optional)                                   │
│ - Runtime + Debug-Tools                                     │
│ - Source code für Debugging                                 │
└─────────────────────────────────────────────────────────────┘
```

---

## ⚙️ Build-Optionen

### Build Arguments

| Argument | Standard | Beschreibung |
|----------|---------|--------------|
| `THEMIS_EDITION` | `COMMUNITY` | Edition: MINIMAL, COMMUNITY, ENTERPRISE, HYPERSCALER |
| `ENABLE_LLM` | `ON` | LLM-Integration mit llama.cpp |
| `ENABLE_GPU` | `OFF` | GPU-Acceleration (Vulkan/CUDA) |
| `FORCE_CPU_ONLY` | `ON` | CPU-only Build (Docker-kompatibel) |
| `BUILD_TESTS` | `OFF` | Tests kompilieren |
| `BUILD_BENCHMARKS` | `OFF` | Benchmarks kompilieren |
| `TARGETARCH` | `amd64` | Zielarchitektur (amd64, arm64) |

### Beispiele

```powershell
# MINIMAL Edition (kleinste Größe)
docker buildx build --build-arg THEMIS_EDITION=MINIMAL -t themisdb:minimal .

# ENTERPRISE mit LLM + Tests
docker buildx build \
  --build-arg THEMIS_EDITION=ENTERPRISE \
  --build-arg ENABLE_LLM=ON \
  --build-arg BUILD_TESTS=ON \
  -t themisdb:enterprise .

# ARM64 Build
docker buildx build \
  --platform linux/arm64 \
  --build-arg TARGETARCH=arm64 \
  -t themisdb:arm64 .
```

---

## 🚄 Caching-Strategien

### 1. BuildKit Layer Cache

```powershell
# Cache aktivieren (Standard)
docker buildx build --cache-from type=local,src=.docker-cache \
                    --cache-to type=local,dest=.docker-cache \
                    -t themisdb:community .
```

### 2. vcpkg Binary Cache

Automatisch aktiviert via Dockerfile:
- Downloads: `/opt/vcpkg/downloads` (BuildKit mount)
- Packages: `/opt/vcpkg/packages` (BuildKit mount)
- Buildtrees: `/opt/vcpkg/buildtrees` (BuildKit mount)

**Effekt:** vcpkg install ~2-5 Min statt ~15-30 Min

### 3. Registry Cache

Für CI/CD-Pipelines:

```powershell
docker buildx build \
  --cache-from type=registry,ref=ghcr.io/yourorg/themisdb:buildcache \
  --cache-to type=registry,ref=ghcr.io/yourorg/themisdb:buildcache,mode=max \
  -t themisdb:community .
```

---

## 🐳 Docker Compose

### Standard-Konfiguration

```yaml
# docker-compose.yml
services:
  themis:
    build: .
    ports:
      - "8080:8080"
    volumes:
      - themis-data:/data
    environment:
      - THEMIS_LOG_LEVEL=info
```

### Starten

```powershell
# Services starten
docker compose up -d

# Logs anzeigen
docker compose logs -f themis

# Services stoppen
docker compose down

# Mit Volume-Löschung
docker compose down -v
```

---

## 🔍 Debugging

### Debug-Container starten

```powershell
docker buildx build --target debug -t themisdb:debug .
docker run -it --rm themisdb:debug /bin/bash
```

### GDB Debugging

```powershell
docker run -it --rm --cap-add=SYS_PTRACE themisdb:debug gdb /opt/themis/bin/themis_server
```

### Live-Debugging

```powershell
# Container mit Debug-Image starten
docker run -it --rm -p 8080:8080 themisdb:debug

# In separatem Terminal: Attach
docker exec -it <container_id> bash
```

---

## 📊 Performance

### Build-Zeiten (Intel i7, 16GB RAM, SSD)

| Szenario | Ohne Cache | Mit BuildKit Cache | Mit vcpkg Binary Cache |
|----------|------------|--------------------|-----------------------|
| **First Build** | ~30-45 Min | ~30-45 Min | ~25-35 Min |
| **Rebuild (Code-Änderung)** | ~30-45 Min | ~2-5 Min | ~2-3 Min |
| **Rebuild (Dependency-Änderung)** | ~30-45 Min | ~15-20 Min | ~5-10 Min |

### Image-Größen

| Edition | Runtime Image | Debug Image |
|---------|--------------|-------------|
| MINIMAL | ~300 MB | ~450 MB |
| COMMUNITY | ~500 MB | ~700 MB |
| ENTERPRISE | ~700 MB | ~950 MB |
| HYPERSCALER | ~900 MB | ~1.2 GB |

---

## 🔒 Sicherheit

### Non-Root User

Runtime-Container läuft als User `themis` (UID 1000):

```dockerfile
USER themis
```

### Read-Only Root Filesystem

Für erhöhte Sicherheit:

```yaml
services:
  themis:
    read_only: true
    tmpfs:
      - /tmp
      - /var/tmp
```

### Security Scanning

```powershell
# Trivy scan
docker run --rm -v /var/run/docker.sock:/var/run/docker.sock \
  aquasec/trivy image themisdb:community
```

---

## 🛠️ Troubleshooting

### Build schlägt fehl

**Problem:** vcpkg install timeout
```
ERROR: vcpkg install failed after 3 attempts
```

**Lösung:** Cache löschen und neu bauen
```powershell
docker buildx prune -a -f
.\build-docker.ps1 -NoCache
```

### Container startet nicht

**Problem:** Port bereits belegt
```
Error: bind: address already in use
```

**Lösung:** Port ändern
```yaml
ports:
  - "8888:8080"  # Statt 8080:8080
```

### OOM (Out of Memory)

**Lösung:** Docker Desktop RAM erhöhen (Settings → Resources → Memory)
- Minimum: 8GB
- Empfohlen: 16GB

---

## 📚 Best Practices

### ✅ DO

- ✅ Use BuildKit caching für schnelle Rebuilds
- ✅ Use multi-stage builds für kleine Images
- ✅ Use non-root user im Runtime-Image
- ✅ Use health checks für Container-Überwachung
- ✅ Use `.dockerignore` für Build-Context-Optimierung
- ✅ Pin vcpkg baseline für reproduzierbare Builds
- ✅ Use aria2 für schnelle vcpkg Downloads

### ❌ DON'T

- ❌ Don't commit `.docker-cache/` zu Git
- ❌ Don't run as root in production
- ❌ Don't include unnecessary files in context
- ❌ Don't rebuild dependencies on every code change
- ❌ Don't use `latest` tags in production

---

## 🔗 Verwandte Dokumentation

- [WINDOWS_BUILD_SETUP.md](WINDOWS_BUILD_SETUP.md) - Windows CMake Configuration
- [docker/DOCKER_BUILD_STRATEGY_QUICKREF.md](docker/DOCKER_BUILD_STRATEGY_QUICKREF.md) - Docker Build Strategy
- [docker/VCPKG_ARIA2_STRATEGY.md](docker/VCPKG_ARIA2_STRATEGY.md) - vcpkg Download Optimization

---

## 📝 Version History

- **v1.4.0** (2026-01-29) - Best-Practice Docker Build basierend auf Windows CMake Config
- **v1.3.0** (2026-01-10) - Multi-Stage Build mit vcpkg aria2
- **v1.2.0** (2025-12) - Edition-basierte Builds

---

## 🤝 Contributing

Bei Problemen oder Verbesserungsvorschlägen:
1. Issue erstellen mit `[Docker]` Prefix
2. Build-Logs anhängen (`docker-build.log`)
3. System-Info angeben (OS, Docker Version, RAM)

---

## 📄 Lizenz

MIT License - See [LICENSE](LICENSE)
