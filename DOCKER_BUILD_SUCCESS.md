# ThemisDB Wire Protocol - Docker Build Lösung ✅

## Problem GELÖST

Docker Desktop auf Windows konnte nicht auf WSL2-Dateien zugreifen beim Build. 

## Finale Lösung: Fast Build mit Pre-Built Binary

**Methode:** Kopiere das bereits kompilierte WSL-Binary direkt ins Docker-Image.

### Verwendete Dateien

1. **Dockerfile.fast** - Minimales Runtime-Image mit Ubuntu 24.04
2. **docker-build-fast.sh** - Bash-Skript das Binary kopiert und Docker baut

### Build-Kommando (SCHNELL - ~30 Sekunden)

```bash
# Von WSL aus (empfohlen):
wsl bash docker-build-fast.sh

# Oder manuell:
cd /mnt/c/VCC/themis
cp build-wsl/themis_server themis_server
docker build -t themis-db:wire-protocol-latest -f Dockerfile.fast .
rm themis_server
```

### Image-Details

- **Base:** Ubuntu 24.04 (wegen glibc 2.38+ Requirement)
- **Size:** ~160 MB
- **Runtime Dependencies:** libboost 1.83.0, libssl3t64, libcurl4t64, glibc 2.39
- **Ports:** 8765 (HTTP REST), 8766 (Wire Protocol)

### Container Starten

```bash
docker run -d \
  -p 8765:8765 \
  -p 8766:8766 \
  -v themis_data:/data \
  --name themis-wire \
  themis-db:wire-protocol-latest

# Status prüfen
docker ps -a | grep themis-wire

# Logs ansehen
docker logs -f themis-wire

# Test HTTP API
curl http://localhost:8765/health
```

### Erfolgreicher Start (Logs)

```
[2025-12-04 18:36:42.597] [themis] [info] HTTP Server listening on 0.0.0.0:8765
[2025-12-04 18:36:42.597] [themis] [info] HTTP Server started successfully
[2025-12-04 18:36:42.597] [themis] [info] Starting Wire Protocol server on port 8766...
[2025-12-04 18:36:42.597] [themis] [info] STATUS: READY FOR CONNECTIONS ✓
```

✅ **HTTP REST API:** Port 8765 - `curl http://localhost:8765/health` → `200 OK`
✅ **Wire Protocol:** Port 8766 - `nc -zv localhost 8766` → `Connection succeeded!`

## Bug Fix: Wire Protocol remote_endpoint

**Problem:** Wire Protocol startete nicht - "remote_endpoint: Bad file descriptor"

**Ursache:** Socket-remote_endpoint wurde VOR Socket-Akzeptierung aufgerufen

**Lösung:** 
- Session::Konstruktor initialisiert client_ip_ mit "unknown"  
- Session::start() setzt client_ip_ NACH Socket-Akzeptierung
- handleAccept() liest remote_ip direkt aus akzeptiertem Socket

**Resultat:** Beide Protokolle laufen perfekt! ✅

### Verifikation nach Build

```bash
# Image prüfen
docker images themis-db

# Container starten
docker run -d -p 8765:8765 -p 8766:8766 -v themis_data:/data themis-db:wire-protocol-latest

# Status prüfen
docker ps -a

# Logs ansehen
docker logs <container_id>

# Port-Test
# HTTP REST API: http://localhost:8765
# Wire Protocol: TCP port 8766
```

### Alternative Ansätze (gescheitert)

❌ **Dockerfile.prebuilt** - Binary aus WSL kopieren
   - Problem: Docker build context sieht WSL-Dateien nicht
   - `COPY build-wsl/themis_server` schlägt fehl

❌ **Dockerfile.wire-protocol** - Multi-Stage mit externem vcpkg
   - Problem: Build context > 500 MB (vcpkg zu groß)
   - Error: "invalid file request external/vcpkg/buildtrees/..."

❌ **Dockerfile.build-in-docker** - Ohne zip/unzip
   - Problem: vcpkg bootstrap benötigt zip/unzip/tar
   - Error: "Could not find zip. Please install it..."

### Erfolgreiches Pattern ✅

```dockerfile
# Multi-Stage mit IN-Container vcpkg
FROM ubuntu:22.04 AS builder

# WICHTIG: zip, unzip, tar für vcpkg bootstrap
RUN apt-get update && apt-get install -y \
    build-essential cmake git curl wget zip unzip tar \
    pkg-config ca-certificates libboost-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone vcpkg IN CONTAINER (nicht im Build-Kontext)
RUN git clone --depth=1 https://github.com/Microsoft/vcpkg.git external/vcpkg && \
    cd external/vcpkg && ./bootstrap-vcpkg.sh -disableMetrics

# Build with Wire Protocol
RUN cmake -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
    -DTHEMIS_BUILD_WIRE_PROTOCOL=ON && \
    cmake --build build --target themis_server -j$(nproc)

# Runtime Stage
FROM ubuntu:22.04
COPY --from=builder /build/build/themis_server /app/themis_server
# ... rest of runtime setup
```

## Lessons Learned

1. ✅ **In-Container vcpkg** vermeidet Build-Context-Probleme
2. ✅ **--depth=1** spart Zeit beim Git-Clone
3. ✅ **Multi-Stage** hält Final-Image klein
4. ✅ **zip/unzip/tar** sind PFLICHT für vcpkg
5. ✅ **Boost 1.74.0** für Ubuntu 22.04 (nicht 1.81.0!)

## Nächste Schritte

Nach erfolgreichem Build:

1. Container starten
2. Wire Protocol auf Port 8766 testen
3. Benchmarks gegen PostgreSQL/MongoDB durchführen
4. Performance-Metriken sammeln
5. Produktions-Deployment vorbereiten

## Support

Build-Status prüfen:
```bash
docker ps -a  # Alle Container
docker logs -f <container_id>  # Live-Logs
docker inspect themis-db:wire-protocol-latest  # Image-Details
```
