# ThemisDB - Official Docker Image

[![Release](https://img.shields.io/github/v/release/makr-code/ThemisDB?include_prereleases&sort=semver&color=blue)](https://github.com/makr-code/ThemisDB/releases)
[![License](https://img.shields.io/badge/license-MIT-green)](https://github.com/makr-code/ThemisDB/blob/main/LICENSE)
[![Platform](https://img.shields.io/badge/platform-linux%2Famd64%20%7C%20linux%2Farm64-brightgreen)](https://github.com/makr-code/ThemisDB)
[![Build](https://img.shields.io/badge/build-multi--stage--docker-success)](https://github.com/makr-code/ThemisDB/blob/main/Dockerfile)
[![Docker](https://img.shields.io/badge/Docker-✓-blue?logo=docker)](https://hub.docker.com/r/themisdb/themisdb)
[![GitHub Stars](https://img.shields.io/github/stars/makr-code/ThemisDB?style=social)](https://github.com/makr-code/ThemisDB)

ThemisDB ist ein High-Performance Multi-Modell-Datenbanksystem auf Basis von LSM-Tree-Architektur mit nativer Unterstützung für Vektorsuche, Graphoperationen, Geospatial-Abfragen und Volltextsuche.

**✨ NEU in v1.3.0:** Optionale native LLM-Integration mit llama.cpp - Führen Sie AI/LLM-Workloads direkt in Ihrer Datenbank aus!

**Aktuelle Version:** v1.3.0 (Dezember 2025)  
**Registry:** `docker.io/themisdb/themisdb`  
**Build-Status:** ✅ Erfolgreich (21.12.2025)

---

## Quick Start

```bash
# Image herunterladen
docker pull themisdb/themisdb:1.3.0

# ThemisDB starten (mit Named Volume - Best Practice)
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -p 4318:4318 \
  -v themisdb_data:/var/lib/themisdb \
  themisdb/themisdb:1.3.0

# Verifyieren Sie den Start
curl http://localhost:8080/health
```

**Ports:**

**Core Ports (Always Available):**
- `8080` - REST API & GraphQL Interface (HTTP/1.1, HTTP/2 if enabled)
- `18765` - Binary Protocol (gRPC, Wire Protocol) - Updated from 8765 for consistency
- `4318` - OpenTelemetry/Prometheus Metrics (OTLP)

**Optional Protocol Ports (Require Build Flags):**
- `1883` - MQTT (plain) - Requires `-DTHEMIS_ENABLE_MQTT=ON`
- `8883` - MQTT over TLS - Requires `-DTHEMIS_ENABLE_MQTT=ON`
- `8083` - MQTT over WebSocket - Requires `-DTHEMIS_ENABLE_MQTT=ON`
- `5432` - PostgreSQL Wire Protocol - Requires `-DTHEMIS_ENABLE_POSTGRES_WIRE=ON`
- `3000` - MCP (Model Context Protocol) - Requires `-DTHEMIS_ENABLE_MCP=ON`

**📖 Complete Port Reference:** See [docs/deployment/PORT_REFERENCE.md](../docs/deployment/PORT_REFERENCE.md) for detailed documentation.

**Volume:**
- `/var/lib/themisdb` - Datenbankdateien (PostgreSQL-Style: persistent und als Named Volume!)

**Schnelle Überprüfung:**
```bash
docker logs themis        # Logs anschauen
docker ps | grep themis   # Container-Status prüfen
curl http://localhost:8080/health
```

---

## Umgebungsvariablen (Environment Variables)

ThemisDB unterstützt umfangreiche Konfiguration über Umgebungsvariablen (PostgreSQL-Style):

### Core Configuration
- `THEMIS_DATA_DIR` - Hauptdatenverzeichnis (default: `/var/lib/themisdb`, ähnlich wie `PGDATA`)
- `THEMIS_PORT` - Server Port (default: `18765`)
- `THEMIS_HOST` - Server Host (default: `0.0.0.0`)
- `THEMIS_WORKER_THREADS` - Worker Threads (default: `8`)

### Storage Configuration
- `THEMIS_ROCKSDB_PATH` - RocksDB Pfad (default: `${THEMIS_DATA_DIR}/data`)
- `THEMIS_VECTOR_INDEX_PATH` - Vektorindex Pfad (default: `${THEMIS_DATA_DIR}/vector_indexes`)
- `THEMIS_MEMTABLE_SIZE_MB` - Memtable Größe (default: `256`)
- `THEMIS_BLOCK_CACHE_SIZE_MB` - Block Cache Größe (default: `1024`)

### Feature Flags
- `THEMIS_ENABLE_TRACING` - OpenTelemetry Tracing (default: `false`)
- `THEMIS_ENABLE_SEMANTIC_CACHE` - Semantic Cache (default: `true`)
- `THEMIS_ENABLE_LLM_STORE` - LLM Store (default: `true`)
- `THEMIS_ENABLE_CDC` - Change Data Capture (default: `true`)
- `THEMIS_ENABLE_TIMESERIES` - Timeseries Support (default: `true`)

**📖 Vollständige ENV-Referenz:** Siehe [../docs/DOCKER_ENV_VARIABLES.md](../docs/DOCKER_ENV_VARIABLES.md)

### Beispiel mit ENV-Variablen
```bash
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 18765:18765 \
  -e THEMIS_WORKER_THREADS=16 \
  -e THEMIS_MEMTABLE_SIZE_MB=512 \
  -e THEMIS_ENABLE_TRACING=true \
  -e THEMIS_OTLP_ENDPOINT=http://jaeger:4318 \
  -v themisdb_data:/var/lib/themisdb \
  themisdb/themisdb:1.3.0
```

---

## Verfügbare Tags

| Tag | Architektur | Base Image | Einsatz |
|-----|-------------|------------|---------|
| `latest` | amd64, arm64 | Ubuntu 22.04 | **Empfohlen** - Neueste stabile Version |
| `1.3.0` | amd64, arm64 | Ubuntu 22.04 | Stabile Release (Dez 2025) - LLM-Support |
| `1.3` | amd64, arm64 | Ubuntu 22.04 | Minor Version Track |
| `1.2.0` | amd64, arm64 | Ubuntu 22.04 | Vorherige stabile Version |
| `qnap` | amd64 | Ubuntu 20.04 | **QNAP NAS** optimiert (SSE4.2 Baseline) |
| `1.3.0-qnap` | amd64 | Ubuntu 20.04 | QNAP v1.3.0 Release |

**Multi-Architektur-Unterstützung:**
- `linux/amd64` - Intel/AMD x64 Prozessoren
- `linux/arm64` - ARM v8 (Raspberry Pi, Apple Silicon, AWS Graviton)

Docker wählt automatisch die passende Architektur für Ihre Plattform.

---

## Features

✅ **Multi-Modell Datenbank**
- Key-Value Store
- Document Store (JSON, BSON)
- Vector Search (Embeddings, Ähnlichkeitssuche mit HNSW)
- Graph Database (Vertices, Edges, Traversals)
- Geospatial (Points, Polygons, Spatial Indexes)
- Full-Text Search (Tokenization, Stemming, Ranking)
- **NEU: Optional LLM Integration** (v1.3.0) - Native AI Inference mit llama.cpp

✅ **Enterprise Features**
- ACID-Transaktionen mit Snapshot Isolation
- Horizontale Skalierung & Sharding
- Multi-Master Replication
- GPU-Beschleunigung (CUDA, Vulkan, ROCm)
- Real-Time Analytics (Complex Event Processing, OLAP)
- Client SDKs (Python, JavaScript, Rust, Go, Java, C#, Swift)
- **NEU: Protocol Support** (v1.3.0) - HTTP/2, WebSocket, MQTT, PostgreSQL Wire, MCP

✅ **Production-Ready**
- ~3.8 GB Docker Image (komprimiert ~150 MB)
- Integrierte Health-Checks
- OpenTelemetry Instrumentation
- DSGVO/GDPR Compliance Features
- Automatisierte Backups & Recovery
- Non-Root Container Security

---

## Docker Compose

### Basis-Setup

```yaml
version: '3.8'

services:
  themis:
    image: themisdb/themisdb:1.3.0
    container_name: themis
    ports:
      # Core ports (always available)
      - "8080:8080"      # HTTP REST API, GraphQL, HTTP/2 (if enabled)
      - "18765:18765"    # Binary Protocol (Wire Protocol, gRPC)
      - "4318:4318"      # OpenTelemetry OTLP (Prometheus metrics)
      
      # Optional protocol ports (uncomment when build flags are enabled):
      # - "1883:1883"    # MQTT plain (requires -DTHEMIS_ENABLE_MQTT=ON)
      # - "8883:8883"    # MQTT over TLS (requires -DTHEMIS_ENABLE_MQTT=ON)
      # - "8083:8083"    # MQTT over WebSocket (requires -DTHEMIS_ENABLE_MQTT=ON)
      # - "5432:5432"    # PostgreSQL Wire Protocol (requires -DTHEMIS_ENABLE_POSTGRES_WIRE=ON)
      # - "3000:3000"    # MCP server (requires -DTHEMIS_ENABLE_MCP=ON)
    volumes:
      - themis_data:/data
      - ./config.json:/etc/themis/config.json:ro
    environment:
      THEMIS_PORT: "18765"
      THEMIS_CONFIG_PATH: "/etc/themis/config.json"
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8080/health"]
      interval: 30s
      timeout: 5s
      retries: 3
      start_period: 10s

volumes:
  themis_data:
    driver: local
```

**Starten:**
```bash
docker-compose up -d
docker-compose logs -f themis
```

### Production-Setup mit Monitoring

```yaml
version: '3.8'

services:
  themis:
    image: themisdb/themisdb:1.3.0
    ports:
      - "8080:8080"      # HTTP REST API
      - "8765:8765"      # Binary Protocol (Wire Protocol, gRPC)
      - "4318:4318"      # OpenTelemetry OTLP
    volumes:
      - themis_data:/data
      - ./config.json:/etc/themis/config.json:ro
    environment:
      THEMIS_PORT: "8765"
      THEMIS_CONFIG_PATH: "/etc/themis/config.json"
    deploy:
      resources:
        limits:
          cpus: '4'
          memory: 8G
        reservations:
          cpus: '2'
          memory: 4G
    restart: unless-stopped
    networks:
      - themis_net

  prometheus:
    image: prom/prometheus:latest
    ports:
      - "9090:9090"      # Prometheus UI & API
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
      - prometheus_data:/prometheus
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'
      - '--storage.tsdb.path=/prometheus'
    networks:
      - themis_net
    restart: unless-stopped

  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"      # Grafana UI
    environment:
      GF_SECURITY_ADMIN_PASSWORD: "admin"
      GF_INSTALL_PLUGINS: "grafana-clock-panel,grafana-simple-json-datasource"
    volumes:
      - grafana_data:/var/lib/grafana
    networks:
      - themis_net
    restart: unless-stopped

networks:
  themis_net:
    driver: bridge

volumes:
  themis_data:
    driver: local
  prometheus_data:
    driver: local
  grafana_data:
    driver: local
```

---

## Konfiguration

### Umgebungsvariablen

| Variable | Standard | Beschreibung |
|----------|----------|--------------|
| `THEMIS_CONFIG_PATH` | `/etc/themis/config.json` | Pfad zur Konfigurationsdatei |
| `THEMIS_PORT` | `8765` | Interner Server-Port (Binary Protocol) |
| `LD_LIBRARY_PATH` | `/usr/local/lib/themisdb:/usr/local/lib` | Pfad für Runtime-Bibliotheken |

### Custom Configuration

```bash
# Eigene Konfiguration einbinden
docker run -d \
  -v /path/to/config.json:/etc/themis/config.json:ro \
  themisdb/themisdb:1.3.0
```

**Konfiguration Beispiel:**
```json
{
  "server": {
    "port": 8765,
    "max_connections": 1000,
    "thread_pool_size": 8,
    "host": "0.0.0.0"
  },
  "storage": {
    "data_dir": "/data",
    "cache_size_mb": 512,
    "compression": "zstd"
  },
  "tracing": {
    "enabled": true,
    "service_name": "themis-server",
    "otlp_endpoint": "http://localhost:4318"
  },
  "features": {
    "vector_search": true,
    "graph_engine": true,
    "geo_spatial": true,
    "llm_engine": false
  }
}
```

---

## Plattform-spezifische Verwendung

### QNAP NAS

```bash
# QNAP-optimiertes Image herunterladen
docker pull themisdb/themisdb:qnap

# Starten (mit Port 8765)
docker run -d \
  --name themis \
  -p 8765:8765 \
  -v /share/Container/themis/data:/data \
  themisdb/themisdb:qnap
```

**QNAP Hinweise:**
- Ubuntu 20.04 Base (GLIBC 2.31)
- SSE4.2 CPU Baseline (erweiterte Kompatibilität)
- Optimiert für x86_64 QNAP NAS-Geräte
- Erfordert QTS 5.0+ oder QuTS hero h5.0+

### Raspberry Pi / ARM

```bash
# Wählt automatisch ARM64 Image aus
docker pull themisdb/themisdb:1.3.0
docker run -d \
  --name themis \
  -p 8080:8080 \
  -p 8765:8765 \
  -p 4318:4318 \
  -v themis_data:/data \
  themisdb/themisdb:1.3.0
```

### macOS (Docker Desktop)

```bash
# Funktioniert auf Intel und Apple Silicon
docker pull themisdb/themisdb:1.3.0
docker run -d \
  -p 8080:8080 \
  -p 8765:8765 \
  -p 4318:4318 \
  -v themis_data:/data \
  themisdb/themisdb:1.3.0
```

---

## Volumes & Persistence

### Data Volume

```bash
# Named Volume erstellen
docker volume create themis_data

# Mit Volume starten
docker run -d -v themis_data:/data themisdb/themisdb:1.3.0

# Volume sichern
docker run --rm -v themis_data:/data -v $(pwd):/backup \
  ubuntu tar czf /backup/themis_backup.tar.gz /data
```

### Wichtige Verzeichnisse

| Pfad | Zweck | Mount |
|------|-------|--------|
| `/data` | Datenbankdateien | ✅ Erforderlich |
| `/etc/themis` | Konfiguration | Optional |
| `/var/log/themis` | Anwendungs-Logs | Optional |

---

## Health Checks & Monitoring

### Built-in Health Check

```bash
# Container-Zustand prüfen
docker inspect --format='{{.State.Health.Status}}' themis

# Health Logs anschauen
docker inspect --format='{{json .State.Health}}' themis | jq
```

### Health Endpoint

```bash
curl http://localhost:8080/health
# Response: {"status":"ok","uptime":3600,"version":"1.3.0"}
```

### Ressourcen-Monitoring

```bash
# Live Statistiken
docker stats themis

# Detaillierte Ressourcennutzung
docker inspect themis | jq '.[0].HostConfig.Memory'
```

---

## Troubleshooting

### Container startet nicht

```bash
# Logs anschauen
docker logs themis

# Häufige Lösungen:
# 1. Port-Konflikt - Port-Mapping ändern
docker run -p 8081:8080 -p 18766:18765 ...

# 2. Berechtigungsproblem - Volume-Berechtigungen prüfen
docker run --user 0 ...  # Temporär als root starten

# 3. Ressourcen-Limit - Memory erhöhen
docker run --memory 4g ...
```

### Performance-Probleme

```bash
# Ressourcen erhöhen
docker update --memory 8g --cpus 4 themis
docker restart themis

# Ressourcennutzung prüfen
docker stats themis

# Für Production optimieren
docker run -d \
  --cpus="4" \
  --memory="8g" \
  --memory-swap="10g" \
  themisdb/themisdb:1.3.0
```

### Library Path Probleme

```bash
# Bibliotheken überprüfen
docker exec themis ldd /usr/local/bin/themis_server

# Umgebungsvariablen prüfen
docker exec themis printenv LD_LIBRARY_PATH
```

---

## Advanced Usage

### Ressourcen Limits

```bash
docker run -d \
  --cpus="4" \              # 4 CPU Cores
  --memory="8g" \           # 8GB RAM
  --memory-swap="10g" \     # 10GB insgesamt mit Swap
  --pids-limit=1000 \       # Prozess-Limit
  themisdb/themisdb:1.3.0
```

### Logging Konfiguration

```bash
# JSON file driver mit Rotation
docker run -d \
  --log-driver json-file \
  --log-opt max-size=10m \
  --log-opt max-file=3 \
  themisdb/themisdb:1.3.0

# Syslog driver
docker run -d \
  --log-driver syslog \
  --log-opt syslog-address=udp://localhost:514 \
  themisdb/themisdb:1.3.0
```

### Netzwerk Modi

```bash
# Host network (bessere Performance, weniger Isolation)
docker run -d --network host themisdb/themisdb:1.3.0

# Custom bridge network
docker network create themis_net
docker run -d --network themis_net themisdb/themisdb:1.3.0
```

---

## Eigenes Image bauen

```bash
# Repository klonen
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Standard Image bauen
docker build -t themis:custom .

# QNAP-optimiertes Image bauen
docker build -f docker/Dockerfile.qnap -t themis:qnap .

# Multi-Architektur Build (erfordert buildx)
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t themis:multiarch \
  --push .

# Mit lokaler LLM-Integration (llama.cpp)
docker build \
  --build-arg ENABLE_LLM=ON \
  --build-context llama=llama.cpp \
  -t themis:llm .
```

---

## Support & Ressourcen

**Dokumentation:**
- [ThemisDB GitHub](https://github.com/makr-code/ThemisDB)
- [Docker Deployment Guide](https://github.com/makr-code/ThemisDB/blob/main/docs/deployment/DOCKER_DEPLOYMENT.md)
- [API Reference](https://github.com/makr-code/ThemisDB/tree/main/openapi)

**Hilfe bekommen:**
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- GitHub Discussions: https://github.com/makr-code/ThemisDB/discussions

**Quellcode:**
- Repository: https://github.com/makr-code/ThemisDB
- Lizenz: [Siehe LICENSE](https://github.com/makr-code/ThemisDB/blob/main/LICENSE)

---

## Image Details

**Image Größe:** 
- Komprimiert: ~150 MB
- Unkomprimiert: ~400 MB
- Docker Umgebung: ~3.8 GB

**Build Info:**
- Builder: Docker Buildx (Multi-Stage)
- Compiler: GCC 11+ / Clang 14+
- C++ Standard: C++20
- vcpkg: Package Manager für Dependencies

**Sicherheit:**
- Non-Root User standardmäßig
- Minimale Angriffsfläche
- Regelmäßige Security Updates
- SBOM (Software Bill of Materials) vorhanden

**Inhalte der Runtime Image:**
- `themis_server` Binary (30 MB)
- Runtime-Bibliotheken (vcpkg Dependencies)
- Konfigurationen & Schemas
- Dokumentation & OpenAPI Specs
- Client SDKs & Beispiele
- Plugin-Signer Tools

---

## Lizenz

ThemisDB ist unter den Bedingungen der [LICENSE](https://github.com/makr-code/ThemisDB/blob/main/LICENSE) Datei lizenziert.

---

**Letztes Update:** 21. Dezember 2025  
**Betreuer:** ThemisDB Team  
**Docker Hub:** https://hub.docker.com/r/themisdb/themisdb  
**GitHub:** https://github.com/makr-code/ThemisDB
