# Prometheus Metrics Integration - Buildanleitung

## Überblick

Der ThemisDB Server muss **mit aktivierten Prometheus-Metriken** in allen Editionen (Community bis Hyperscaler) kompiliert werden.

Status: **❌ NICHT AKTIVIERT** in aktuellem Docker Image

## Problem

Die aktuellen Docker-Images (`themisdb/themisdb:latest`) exponieren die Prometheus-Metriken **nicht** auf Port 9090 oder auf `/metrics` Endpunkt:

```bash
# ❌ Funktioniert nicht
curl http://localhost:9090/metrics        # Port nicht offen
curl http://localhost:8080/metrics        # Endpunkt nicht implementiert
docker exec themis-raid0-shard1 curl http://localhost:9090/metrics  # Connection refused
```

## Root Cause

1. **Alte Dockerfile (`Dockerfile.themis-server`)** kopiert einen **Windows-Binary** (`build-msvc/Release/themis_server.exe`)
2. **Kein Linux-Build** des themis_server mit prometheus-cpp Unterstützung
3. **HTTP /metrics Endpunkt** ist im Code vorhanden (Zeile 1297-1304 in `src/server/http_server.cpp`), aber wird nicht vom Docker-Image exponiert

## Lösung

### Option 1: Neue Dockerfile mit Linux-Build (EMPFOHLEN)

Verwende `Dockerfile.themis-metrics-enabled`:

```bash
docker build -f Dockerfile.themis-metrics-enabled -t themisdb/themisdb:metrics-enabled .
```

**Features:**
- ✅ Linux-Build mit CMAKE
- ✅ Prometheus-cpp integration
- ✅ Port 9090 exponiert für Prometheus Metriken
- ✅ Port 8080 für REST API (/metrics Endpunkt)
- ✅ Health Check auf /health

### Option 2: CMake Feature Flag

Stelle sicher, dass Prometheus-Metriken im Build aktiviert sind:

```bash
# Windows/MSVC Build
cmake -S . -B build-msvc \
    -G "Visual Studio 17 2022" \
    -DCMAKE_TOOLCHAIN_FILE="C:/VCC/themis/vcpkg/scripts/buildsystems/vcpkg.cmake"

# Linux Build
cmake -S . -B build-linux \
    -DCMAKE_TOOLCHAIN_FILE="/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-linux --target themis_server
```

**Wichtig:** Es gibt **keine CMake-Option zum Deaktivieren** von Prometheus-Metriken. Sie sind **immer aktiviert** wenn das themis_server binärer gebaut wird.

## Endpunkte nach Aktivierung

### Port 8080 (REST API)
```bash
GET /health           # Health Check
GET /metrics          # Prometheus Metriken (Text Format)
POST /entities        # Create Entity
GET /entities/:key    # Get Entity
```

### Port 9090 (Optional: Separater Prometheus HTTP Server)

Aktuell exponiert themis_server auf **Port 8080** den `/metrics` Endpunkt.

Wenn **separater** Prometheus-HTTP-Server auf Port 9090 gewünscht:
- `THEMIS_METRICS_PORT=9090` setzen (Umgebungsvariable)
- Implementierung in `src/llm/grafana_metrics.cpp` (MetricsServer)

## Docker Compose Integration

```yaml
services:
  themis-raid0-shard1:
    image: themisdb/themisdb:metrics-enabled
    environment:
      THEMIS_ENABLE_METRICS: "true"      # Redundant, immer an
      THEMIS_PORT: "18765"
      THEMIS_RAID_MODE: "stripe"
      THEMIS_RAID_GROUP: "raid0"
    ports:
      - "18765:18765"    # Wire Protocol
      - "8080:8080"      # REST API + Metriken auf /metrics
      - "9090:9090"      # (Optional) Falls separater Prometheus Server
```

## Prometheus Scrape Config

```yaml
scrape_configs:
  - job_name: 'themisdb-raid'
    scrape_interval: 15s
    scrape_timeout: 10s
    static_configs:
      - targets: 
        - 'themis-raid0-shard1:8080'     # NEW: Port 8080 instead of 9090
        - 'themis-raid0-shard2:8080'
        - 'themis-raid0-shard3:8080'
      labels:
        raid_mode: 'stripe'
        raid_group: 'raid0'
    metrics_path: '/metrics'             # Explizit setzen
```

## Verfügbare Metriken

Nach Aktivierung verfügbar:

- `themis_raid_io_bytes_total` - RAID I/O Bytes
- `themis_operation_duration_seconds_bucket` - Operation Latenz
- `themis_io_operations_total` - Operationen pro Sekunde
- `themis_shard_health_status` - Shard Gesundheit
- `themis_routing_requests_total` - Routing Requests
- `themis_cluster_size` - Cluster Größe
- ... (siehe `include/sharding/prometheus_metrics.h`)

## Gradle/CMake Build Flags

**NICHT nötig** - Prometheus ist immer aktiviert.

Falls jedoch selective builds gewünscht (nicht empfohlen):

```cpp
// src/main_server.cpp Zeile 560
#ifdef THEMIS_ENABLE_METRICS
    // Sharding metrics registrieren
    themis::sharding::PrometheusMetrics::Config metrics_cfg;
    auto sharding_metrics = std::make_shared<themis::sharding::PrometheusMetrics>(metrics_cfg);
    themis::sharding::ShardingMetricsRegistry::instance().registerMetrics(sharding_metrics);
#endif
```

## Verifikation

Nach Docker-Build testen:

```bash
# Container starten
docker run -d --name themis-test \
    -p 8080:8080 \
    -p 9090:9090 \
    themisdb/themisdb:metrics-enabled

# Metriken überprüfen
sleep 5
curl -s http://localhost:8080/metrics | head -20

# Sollte folgende Metriken zeigen:
# # HELP themis_raid_io_bytes_total ThemisDB RAID I/O Bytes
# # TYPE themis_raid_io_bytes_total counter
# themis_raid_io_bytes_total{job="raid0"} 0
```

## Häufige Probleme

### Problem: "Connection refused" auf Port 9090
- **Ursache:** Themis-Server exponiert nur auf Port 8080
- **Lösung:** Prometheus sollte auf Port 8080 scrapen, nicht 9090
- **Fix in prometheus.yml:** `targets: ['themis-shard:8080']`

### Problem: /metrics gibt 404 zurück
- **Ursache:** Alter Binary ohne Metrics-Implementation
- **Lösung:** Neues Docker-Image mit `Dockerfile.themis-metrics-enabled` bauen

### Problem: Metriken sind leer
- **Ursache:** Zu früh nach Start (< 30s) abgefragt
- **Lösung:** `curl http://localhost:8080/metrics` nach 30+ Sekunden versuchen

## Next Steps

1. **Baue neues Docker Image:**
   ```bash
   docker build -f Dockerfile.themis-metrics-enabled -t themisdb/themisdb:metrics-enabled .
   ```

2. **Update docker-compose.yml:** Verwende neues Image

3. **Update prometheus.yml:** 
   ```yaml
   targets: ['themis-raid0-shard1:8080']  # Port 8080, nicht 9090
   ```

4. **Neustart der Container:**
   ```bash
   docker-compose -f docker-compose-sharding.yml down
   docker-compose -f docker-compose-sharding.yml up -d
   ```

5. **Verifiziere Metriken:**
   ```bash
   curl http://localhost:8080/metrics
   ```

---

**Status:** Community Edition - Prometheus Metrics sollten in ALLEN Builds aktiviert sein (keine separaten Builds nötig).
