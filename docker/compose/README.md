# ThemisDB RAID Cluster mit Prometheus & Grafana Monitoring

## 📊 Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Docker Compose Network                    │
│                      (themis-network)                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  RAID 0 (Striping)          RAID 1 (Mirror)                │
│  ┌─────────────────┐       ┌─────────────────┐             │
│  │ themis-raid0-1  │       │ themis-raid1-p  │             │
│  │ :8080 /metrics  │       │ :8080 /metrics  │             │
│  │ :18765 Proto    │       │ :18765 Proto    │             │
│  │ :9091 ⚠️ UNUSED │       │ :9094 ⚠️ UNUSED │             │
│  └─────────────────┘       └─────────────────┘             │
│  ┌─────────────────┐       ┌─────────────────┐             │
│  │ themis-raid0-2  │       │ themis-raid1-s  │             │
│  │ :8080 /metrics  │       │ :8080 /metrics  │             │
│  │ :18766 Proto    │       │ :18769 Proto    │             │
│  │ :9092 ⚠️ UNUSED │       │ :9095 ⚠️ UNUSED │             │
│  └─────────────────┘       └─────────────────┘             │
│  ┌─────────────────┐       RAID 5 (Parity)                │
│  │ themis-raid0-3  │       ┌─────────────────┐             │
│  │ :8080 /metrics  │       │ themis-raid5-1  │             │
│  │ :18767 Proto    │       │ :8080 /metrics  │             │
│  │ :9093 ⚠️ UNUSED │       │ :18770 Proto    │             │
│  └─────────────────┘       │ :9096 ⚠️ UNUSED │             │
│                             └─────────────────┘             │
│                             ┌─────────────────┐             │
│                             │ themis-raid5-2  │             │
│                             │ :8080 /metrics  │             │
│                             │ :18771 Proto    │             │
│                             │ :9097 ⚠️ UNUSED │             │
│                             └─────────────────┘             │
│                             ┌─────────────────┐             │
│                             │ themis-raid5-3  │             │
│                             │ :8080 /metrics  │             │
│                             │ :18772 Proto    │             │
│                             │ :9098 ⚠️ UNUSED │             │
│                             └─────────────────┘             │
│                                                              │
│  ┌──────────────┐            ┌──────────────┐              │
│  │ Prometheus   │◄───────────┤  Grafana     │              │
│  │ :9090        │ scrapes    │ :3000        │              │
│  │ (Scraper)    │            │ (Dashboards) │              │
│  └──────────────┘            └──────────────┘              │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## 🚀 Quick Start

### 1. Docker Image mit Metrics aktivieren

```bash
# Baue neues Image mit Prometheus-cpp support
docker build -f Dockerfile.themis-metrics-enabled \
    -t themisdb/themisdb:metrics-enabled .

# Oder verwende das neueste Image
# docker pull themisdb/themisdb:metrics-enabled
```

### 2. Container starten

```bash
cd docker/compose

# Alte Container stoppen
docker-compose -f docker-compose-sharding.yml down

# Neue Container mit Metrics starten
docker-compose -f docker-compose-sharding.yml up -d
```

### 3. Metriken überprüfen

```bash
# Warte 30+ Sekunden für vollständigen Start
sleep 30

# Prüfe ob Prometheus Daten sammelt
curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | {job: .labels.job, health}'

# Sollte folgende Ausgabe zeigen:
# {
#   "job": "raid0-stripe",
#   "health": "up"
# }
# ... für alle RAID Targets
```

### 4. Grafana Dashboard öffnen

```bash
# Browser öffnen: http://localhost:3000
# Login: admin / admin
# Dashboard: "Themis RAID Benchmark" sollte verfügbar sein
```

## 📋 Dateistruktur

```
docker/compose/
├── docker-compose-sharding.yml      # RAID + Prometheus + Grafana
├── prometheus.yml                   # Prometheus Scrape Config
│                                    # (Targets: raid*:8080/metrics)
├── grafana/
│   ├── datasources.yml              # Prometheus datasource config
│   ├── dashboards.yml               # Dashboard provisioning
│   └── dashboards/
│       ├── themis-raid-overview.json        # Old dashboard
│       └── themis_raid_benchmark_dashboard.json  # NEW
└── README.md                        # Diese Datei
```

## 🔧 Konfiguration

### docker-compose-sharding.yml

Die folgenden Services werden bereitgestellt:

| Service | Port (intern) | Port (extern) | Beschreibung |
|---------|--------------|---------------|-------------|
| themis-raid0-shard1 | 8080 | 8080 | REST API + Metrics |
| themis-raid0-shard1 | 18765 | 18765 | Wire Protocol |
| themis-raid0-shard1 | 9090 | 9091 | ⚠️ UNUSED (Metrics auf 8080) |
| themis-prometheus | 9090 | 9090 | Prometheus Server |
| themis-grafana | 3000 | 3000 | Grafana UI |

### prometheus.yml

Prometheus scrapet alle RAID-Shards auf **Port 8080**:

```yaml
scrape_configs:
  - job_name: 'raid0-stripe'
    static_configs:
      - targets:
        - 'themis-raid0-shard1:8080'      # ✅ Port 8080 (REST API)
        - 'themis-raid0-shard2:8080'
        - 'themis-raid0-shard3:8080'
    metrics_path: '/metrics'             # ✅ Path zum Metriken-Endpunkt
```

## 📊 Verfügbare Metriken

Nach erfolgreichem Start sollten folgende Metriken verfügbar sein:

```bash
# Von jedem RAID-Shard
curl -s http://localhost:8080/metrics | grep themis

# Ausgabe sollte enthalten:
# themis_raid_io_bytes_total{job="raid0-stripe",shard_id="raid0-1"} 0
# themis_operation_duration_seconds_bucket{...} 0
# themis_io_operations_total{...} 0
# themis_cluster_size{...} 9
# themis_shard_health_status{shard_id="raid0-1",status="healthy"} 1
```

### Metriken-Typen

- **Counter**: `themis_raid_io_bytes_total`, `themis_io_operations_total`
- **Gauge**: `themis_cluster_size`, `themis_shard_health_status`
- **Histogram**: `themis_operation_duration_seconds_bucket`
- **Routing**: `themis_routing_requests_total`, `themis_routing_latency_ms`
- **PKI**: `themis_certificate_expiry_seconds`
- **Migration**: `themis_migration_progress_percent`, `themis_migration_records_total`

## 🐛 Troubleshooting

### Problem: Prometheus zeigt "down" für alle Targets

**Symptom:**
```
curl http://localhost:9090/api/v1/targets
# "health": "down" für alle raid* targets
```

**Ursachen & Lösungen:**

1. **Falscher Port in prometheus.yml**
   - ❌ Target: `themis-raid0-shard1:9090`
   - ✅ Target: `themis-raid0-shard1:8080`
   - Fix: `docker/compose/prometheus.yml` aktualisieren

2. **Alter themis_server Binary ohne Metrics**
   - Lösung: Neues Docker Image bauen:
   ```bash
   docker build -f Dockerfile.themis-metrics-enabled \
       -t themisdb/themisdb:metrics-enabled .
   ```

3. **Container ist noch nicht vollständig gestartet**
   - Lösung: 30+ Sekunden warten
   ```bash
   docker-compose logs themis-raid0-shard1
   # "HTTP Server created" sollte sichtbar sein
   ```

### Problem: curl auf /metrics gibt 404 oder Connection refused

**Test:**
```bash
# Innerhalb docker network
docker exec themis-prometheus curl -s http://themis-raid0-shard1:8080/metrics | head

# Von Host
curl -s http://localhost:18765/metrics  # Falscher Port
curl -s http://localhost:8080/metrics   # Richtig, wenn du den Port exponierst
```

**Lösung:**
- Stelle sicher, dass Port 8080 in docker-compose-sharding.yml exponiert ist:
  ```yaml
  ports:
    - "18765:18765"  # Wire Protocol
    - "8080:8080"    # REST API + Metrics  <- WICHTIG
  ```

### Problem: Grafana Dashboard zeigt "No data"

1. **Überprüfe Prometheus Datasource:**
   - Grafana UI → Configuration → Data Sources
   - Sollte auf `prometheus:9090` zeigen (nicht localhost)

2. **Überprüfe PromQL Queries:**
   - Sollten `raid.*` im Job-Filter haben
   - Beispiel: `rate(themis_raid_io_bytes_total{job=~"raid.*"}[5m])`

3. **Überprüfe ob Metriken existieren:**
   ```bash
   curl -s http://localhost:9090/api/v1/query?query=themis_raid_io_bytes_total
   ```

## 📈 Performance Monitoring

### I/O Throughput anschauen

```bash
# Prometheus API
curl 'http://localhost:9090/api/v1/query?query=rate(themis_raid_io_bytes_total%5B5m%5D)' | jq
```

### Latenzen überprüfen

```bash
# p95 Latenz für Operationen
curl 'http://localhost:9090/api/v1/query?query=histogram_quantile(0.95,themis_operation_duration_seconds)' | jq
```

## 🔄 Update & Maintenance

### Docker Image aktualisieren

```bash
# Neuen Build erstellen
docker build -f Dockerfile.themis-metrics-enabled \
    -t themisdb/themisdb:metrics-enabled \
    --no-cache .

# Container neu starten
docker-compose -f docker-compose-sharding.yml down
docker-compose -f docker-compose-sharding.yml up -d
```

### Prometheus Daten löschen

```bash
# Alle Metriken zurücksetzen
docker volume rm compose_prometheus_data

# Container neu starten
docker-compose -f docker-compose-sharding.yml restart themis-prometheus
```

### Grafana neukalibrieren

```bash
# Grafana-Container neu starten
docker-compose -f docker-compose-sharding.yml restart themis-grafana

# Browser Cache löschen
# Reload Dashboard (Ctrl+Shift+R)
```

## 📝 Umgebungsvariablen (Optional)

Diese Variablen werden in docker-compose-sharding.yml aktuell NICHT verwendet, könnten aber hinzugefügt werden:

```yaml
environment:
  THEMIS_PORT: "18765"              # Wire Protocol Port
  THEMIS_RAID_MODE: "stripe"
  THEMIS_RAID_GROUP: "raid0"
  # THEMIS_ENABLE_METRICS: "true"   # Immer aktiviert (New Image)
  # THEMIS_METRICS_PATH: "/metrics" # Standardwert
```

## ✅ Checkliste für vollständiges Setup

- [ ] Dockerfile.themis-metrics-enabled vorhanden
- [ ] Neues Docker Image gebaut: `docker build -f Dockerfile.themis-metrics-enabled ...`
- [ ] docker-compose-sharding.yml mit aktualisiertem Image
- [ ] prometheus.yml mit `:8080` Targets (nicht `:9090`)
- [ ] grafana/datasources.yml mit `prometheus:9090`
- [ ] grafana/dashboards/themis_raid_benchmark_dashboard.json vorhanden
- [ ] Container gestartet: `docker-compose up -d`
- [ ] Gewartet auf vollständigen Start (~30s)
- [ ] Prometheus zeigt "up" für alle Targets
- [ ] Grafana Dashboard zeigt Daten

## 🔗 Weiterführende Ressourcen

- Prometheus Build Dokumentation: [PROMETHEUS_METRICS_BUILD.md](../../PROMETHEUS_METRICS_BUILD.md)
- Sharding Metrics Code: [include/sharding/prometheus_metrics.h](../../include/sharding/prometheus_metrics.h)
- Main Server Setup: [src/main_server.cpp Zeile 560+](../../src/main_server.cpp#L560)
- Dashboard Definition: [benchmarks/monitoring/themis_raid_benchmark_dashboard.json](../../benchmarks/monitoring/themis_raid_benchmark_dashboard.json)

---

**Stand:** 3. Januar 2026 | **Edition:** Community bis Hyperscaler | **Metrics Status:** ✅ Aktiviert
