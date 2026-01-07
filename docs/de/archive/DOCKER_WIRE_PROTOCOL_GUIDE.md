# ThemisDB Wire Protocol Docker Deployment Guide

## Überblick
Diese Anleitung erklärt, wie Sie ThemisDB mit Wire Protocol Server in Docker packieren und ausführen.

## Voraussetzungen

- Docker installiert
- `build-wsl/themis_server` Binary mit Wire Protocol vorhanden
- `config/config.json` Konfigurationsdatei

## Build-Prozess

### 1. Binary mit Wire Protocol bauen (WSL)

```bash
cd /mnt/c/VCC/themis
export VCPKG_ROOT=/mnt/c/VCC/themis/vcpkg

# CMake-Konfiguration mit Wire Protocol
cmake -S . -B build-wsl -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=/mnt/c/VCC/themis/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DTHEMIS_BUILD_WIRE_PROTOCOL=ON

# Build themis_server
cmake --build build-wsl --target themis_server -j8
```

### 2. Docker Image bauen

**Option A: Mit Bash-Script (Linux/WSL)**
```bash
bash ./docker-build-wire-protocol.sh
```

**Option B: Mit Batch-Script (Windows)**
```cmd
docker-build-wire-protocol.bat
```

**Option C: Manuell**
```bash
docker build \
  -t themis-db:wire-protocol-latest \
  -f Dockerfile.prebuilt \
  .
```

## Ausführung

### Mit Docker Compose (Empfohlen)

```bash
docker-compose -f docker-compose.wire-protocol.yml up -d
```

### Mit Docker Run

```bash
docker run -d \
  -p 8765:8765 \
  -p 8766:8766 \
  -v themis_data:/data \
  --name themis-server \
  themis-db:wire-protocol-latest
```

## Ports

- **8765**: HTTP REST API (Standard ThemisDB)
- **8766**: Wire Protocol (Binary TCP für Native Clients)

## Überprüfung

```bash
# Container-Status prüfen
docker ps -a

# Logs anschauen
docker logs -f themis-server

# In Container gehen
docker exec -it themis-server bash

# Wire Protocol Port prüfen (von außen)
netstat -tnl | grep 8766
# oder
ss -tnl | grep 8766
```

## Cleanup

```bash
# Container stoppen
docker stop themis-server

# Container löschen
docker rm themis-server

# Volume löschen (WARNUNG: Löscht alle Daten!)
docker volume rm themis_data

# Image löschen
docker rmi themis-db:wire-protocol-latest
```

## Performance für Benchmarks

Der Wire Protocol Server ist optimiert für:
- Faire Benchmarks gegen PostgreSQL/MongoDB
- Hoher Durchsatz (parallel Requests)
- Niedrige Latenz (dedizierte Thread Pools)
- Sichere Connections (Rate Limiting, Connection Limits)

## Features

✅ Wire Protocol Server (8766)
✅ HTTP REST API (8765)
✅ Rate Limiting pro IP
✅ Connection Limits
✅ Timeout Protection
✅ Security Features aktiviert

## Troubleshooting

**Problem: Port 8766 wird nicht gehört**
- Prüfe Container-Status: `docker ps -a`
- Logs: `docker logs themis-server`
- Firewall-Regeln checken

**Problem: Build fehlt schlägt fehl**
- Stelle sicher, dass `build-wsl/themis_server` existiert
- Führe den WSL-Build aus: `wsl bash -c "cd /mnt/c/VCC/themis && ..."`

**Problem: Daten-Persistenz**
- Docker Volume `themis_data` wird automatisch erstellt
- Backup: `docker run --rm -v themis_data:/data -v $(pwd):/backup ubuntu tar czf /backup/themis_backup.tar.gz -C /data .`

## Monitoring

Für Produktionsumgebungen:
```bash
docker stats themis-server
```

Für detaillierte Metriken:
```bash
docker top themis-server
```
