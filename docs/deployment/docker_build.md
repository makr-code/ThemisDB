# ThemisDB Docker Build und Deployment

**Stand:** 6. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Deployment

---

## Build-Strategie: Hybrid Pre-built Binary

Der empfohlene Ansatz für Docker-Builds ist der **Hybrid Pre-built Binary** Workflow:

1. **Binary lokal bauen** (einmalig, ~30-40 Minuten mit vcpkg)
2. **Docker-Image erstellen** mit `Dockerfile.simple` (schnell, ~30 Sekunden)
3. **Ergebnis**: Kleine Images (~100-200 MB) und 100% offline-fähig

### Vorteile
- ✅ Schnelle Build-Zeiten (Sekunden statt Minuten)
- ✅ Kleine Image-Größe (~100-200 MB)
- ✅ 100% Offline-fähig
- ✅ Multi-Architektur Unterstützung (amd64, arm64)

## Unified Multi-Arch Build Script

### PowerShell (Windows)

```powershell
# Standard Build mit existierender Binary
.\docker-build-multiarch.ps1

# Binary in WSL bauen, dann Docker-Image erstellen
.\docker-build-multiarch.ps1 -BuildBinary

# QNAP-Variante
.\docker-build-multiarch.ps1 -Variant qnap

# ARM64 für Raspberry Pi
.\docker-build-multiarch.ps1 -Platform linux/arm64

# Build und Push zu Registry
.\docker-build-multiarch.ps1 -Push

# Alle Optionen
.\docker-build-multiarch.ps1 -Version 1.0.1 -Registry themisdb -Variant standard -Push
```

### Bash (Linux/macOS)

```bash
# Standard Build mit existierender Binary
./docker-build-multiarch.sh

# Binary bauen, dann Docker-Image erstellen
./docker-build-multiarch.sh --build-binary

# QNAP-Variante
./docker-build-multiarch.sh -b qnap

# ARM64 für Raspberry Pi
./docker-build-multiarch.sh -p linux/arm64

# Build und Push zu Registry
./docker-build-multiarch.sh --push
```

## Unterstützte Plattformen

| Plattform | Architektur | Use Case |
|-----------|-------------|----------|
| `linux/amd64` | x86_64 | Server, Desktop, QNAP NAS |
| `linux/arm64` | ARM64 | Raspberry Pi 4/5, ARM Server, Apple Silicon |

## Docker Tags

| Tag | Beschreibung |
|-----|--------------|
| `themisdb/themisdb:latest` | Neueste stabile Version |
| `themisdb/themisdb:1.0.0` | Spezifische Version |
| `themisdb/themisdb:qnap` | QNAP NAS optimiert |
| `themisdb/themisdb:1.0.0-qnap` | QNAP spezifische Version |
| `themisdb/themisdb:1.0.0-arm64` | ARM64 spezifische Version |

## Voraussetzungen

### Binary vorbereiten

Die Binary muss vor dem Docker-Build vorhanden sein:

```bash
# Option 1: Mit dem Script (WSL)
.\docker-build-multiarch.ps1 -BuildBinary

# Option 2: Manuell in WSL/Linux
cd ~/themis-build-release
cmake --build . --target themis_server -j$(nproc)
cp themis_server /path/to/ThemisDB/build/
```

### Verzeichnisstruktur
```
ThemisDB/
├── build/
│   └── themis_server          # Pre-built Binary (erforderlich)
├── docker/
│   └── entrypoint.sh
├── config/
│   └── config.qnap.json
├── Dockerfile.simple          # Verwendet für Hybrid-Build
├── docker-build-multiarch.ps1
└── docker-build-multiarch.sh
```

## Container starten

```powershell
# Einfacher Start
docker run -d -p 18765:18765 themisdb/themisdb:latest

# Mit Daten-Volume
docker run -d \
  -p 18765:18765 \
  -v themis-data:/data \
  themisdb/themisdb:latest

# Mit custom Config
docker run -d \
  -p 18765:18765 \
  -v /pfad/zu/config.json:/etc/themis/config.json \
  -v themis-data:/data \
  themisdb/themisdb:latest
```

## Docker Compose (QNAP)

Siehe `docker-compose.qnap.yml` für Production-Setup.

## Abhängigkeiten (Runtime)

Das Runtime-Image enthält nur minimale Abhängigkeiten:
- Ubuntu 24.04 Base
- ca-certificates
- curl
- libstdc++6
- jq (für Config-Verarbeitung)

## Troubleshooting

### Binary nicht gefunden
```
✗ Binary not found at: build/themis_server
```
**Lösung**: Binary zuerst bauen mit `-BuildBinary` oder manuell kopieren.

### Docker nicht verfügbar
```
✗ Docker is not running
```
**Lösung**: Docker Desktop starten.

### Platform-Fehler bei ARM64
```
ERROR: no matching manifest for linux/arm64
```
**Lösung**: Binary muss für ARM64 kompiliert sein (Cross-Compile oder auf ARM-System).
