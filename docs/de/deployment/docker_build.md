# ThemisDB Docker Build und Deployment

**Stand:** 22. Dezember 2025  
**Version:** v1.3.0  
**Kategorie:** 🚀 Deployment

---

## 📑 Inhaltsverzeichnis

- [Build-Strategie](#build-strategie-hybrid-pre-built-binary-monolithisch)
- [Build Scripts](#unified-docker-build-scripts)
- [Unterstützte Plattformen](#unterstützte-plattformen)
- [Docker Tags](#docker-tags)
- [Voraussetzungen](#voraussetzungen)
- [Container starten](#container-starten)

## Build-Strategie: Hybrid Pre-built Binary (Monolithisch)

Der empfohlene Ansatz für Docker-Builds ist der **Hybrid Pre-built Binary** Workflow mit **monolithischem (statischem) Linking**:

1. **Binary lokal bauen** (einmalig, ~30-40 Minuten mit vcpkg, `-DTHEMIS_STATIC_BUILD=ON`)
2. **Docker-Image erstellen** mit `Dockerfile.simple` (schnell, ~30 Sekunden)
3. **Ergebnis**: Kleine Images (~100-200 MB), 100% offline-fähig, maximale Portabilität

### Vorteile
- ✅ Schnelle Build-Zeiten (Sekunden statt Minuten)
- ✅ Kleine Image-Größe (~100-200 MB)
- ✅ 100% Offline-fähig
- ✅ Monolithische Binary (keine Library-Abhängigkeiten)
- ✅ Health-Check für Container-Orchestrierung

## Unified Docker Build Scripts

### PowerShell (Windows/WSL)

```powershell
# Standard Build mit existierender Binary
.\docker-build.ps1

# Binary in WSL bauen, dann Docker-Image erstellen
.\docker-build.ps1 -BuildBinary

# QNAP-Variante
.\docker-build.ps1 -Variant qnap

# Build und Push zu Registry
.\docker-build.ps1 -Push

# Alle Optionen
.\docker-build.ps1 -Version 1.0.1 -Registry themisdb -Variant standard -Push
```

### Bash (Linux/macOS)

```bash
# Standard Build mit existierender Binary
./docker-build.sh

# Binary bauen, dann Docker-Image erstellen
./docker-build.sh --build-binary

# QNAP-Variante
./docker-build.sh -b qnap

# Build und Push zu Registry
./docker-build.sh --push
```

## Unterstützte Plattformen

| Plattform | Architektur | Linking | Use Case |
|-----------|-------------|---------|----------|
| `linux/amd64` | x86_64 | Statisch | Server, Desktop, QNAP NAS |
| `linux/arm64` | ARM64 | Statisch | Raspberry Pi 4/5, ARM Server |

## Docker Tags

| Tag | Beschreibung |
|-----|--------------|
| `themisdb/themisdb:latest` | Neueste stabile Version |
| `themisdb/themisdb:1.0.0` | Spezifische Version |
| `themisdb/themisdb:qnap` | QNAP NAS optimiert |
| `themisdb/themisdb:1.0.0-qnap` | QNAP spezifische Version |

## Voraussetzungen

### Binary vorbereiten (monolithisch/statisch)

Die Binary muss vor dem Docker-Build vorhanden sein:

```bash
# Option 1: Mit dem Script
./docker-build.sh --build-binary  # Linux/macOS
.\docker-build.ps1 -BuildBinary   # Windows/WSL

# Option 2: Manuell in WSL/Linux (mit statischem Build)
cd ~/themis-build-release
cmake -S /path/to/ThemisDB -B . -DCMAKE_BUILD_TYPE=Release -DTHEMIS_STATIC_BUILD=ON
cmake --build . --target themis_server -j$(nproc)
cp themis_server /path/to/ThemisDB/build/
```

### Verzeichnisstruktur
```
ThemisDB/
├── build/
│   └── themis_server          # Pre-built Binary (monolithisch)
├── docker/
│   └── entrypoint.sh
├── config/
│   └── config.qnap.json
├── Dockerfile.simple          # Verwendet für Hybrid-Build
├── docker-build.ps1
└── docker-build.sh
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
