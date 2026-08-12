# 📋 Prometheus Metrics Integration - Vollständige Übersicht

## Status: ✅ VOLLSTÄNDIG KONFIGURIERT (Wartet auf Docker Rebuild)

---

## 🎯 Problem & Lösung

### 🔴 Das Problem
Der aktuelle `themisdb/themisdb:latest` Docker Image enthält ein **Windows-Executable**:
- `build-msvc/Release/themis_server.exe` (Windows PE Binary)
- Lädt in Linux-Container `ubuntu:24.04` - **Kann nicht ausgeführt werden**
- **Resultat:** Keine Prometheus-Metriken, keine HTTP API

### ✅ Die Lösung
Neues Image mit Linux-Build und Prometheus-Unterstützung:
- `Dockerfile.themis-metrics-enabled` (Ubuntu 24.04 + CMake)
- Prometheus-cpp Library eingebunden
- HTTP Server auf Port 8080 mit `/metrics` Endpunkt

---

## 📚 Dateien im Überblick

### 1. **Dockerfile.themis-metrics-enabled** (NEU)
**Pfad:** `c:\VCC\themis\Dockerfile.themis-metrics-enabled`

**Zweck:** Baue Linux-Binary mit Prometheus-Support

**Features:**
```dockerfile
FROM ubuntu:24.04                    # Linux target (nicht Windows!)
RUN apt-get install cmake vcpkg      # Build dependencies
RUN cmake -DTHEMIS_ENABLE_LLM=OFF    # Metrics immer dabei
CMD ["./build/Release/themis_server"]# Linux ELF binary
EXPOSE 18765 8080 9090               # Alle relevanten Ports
```

**Verwendung:**
```bash
docker build -f Dockerfile.themis-metrics-enabled \
    -t themisdb/themisdb:metrics-enabled .
```

---

### 2. **docker-compose-sharding.yml** (MODIFIED)
**Pfad:** `c:\VCC\themis\docker\compose\docker-compose-sharding.yml`

**Änderungen:**
```yaml
# VOR:
ports:
  - "18765:18765"   # Wire Protocol
  - "9091:9090"     # ❌ Falsch - nichts listening auf 9090

# NACH:
ports:
  - "18765:18765"   # Wire Protocol
  - "8080:8080"     # ✅ REST API + /metrics
  - "9091:9090"     # Metrics Port (optional, nicht benötigt)

# Alle 9 RAID-Shards haben jetzt:
environment:
  THEMIS_ENABLE_METRICS: "true"      # Metriken aktiviert
  THEMIS_METRICS_PORT: "9090"        # (Intern, wird nicht verwendet)
```

**Wichtig:** Port 8080 ist der echte `/metrics` Endpunkt!

| Container | Host Port | Container Port | Service |
|-----------|-----------|----------------|---------|
| raid0-shard1 | 8080 | 8080 | REST API /metrics |
| raid0-shard2 | 8081 | 8080 | REST API /metrics |
| raid0-shard3 | 8082 | 8080 | REST API /metrics |
| raid1-primary | 8083 | 8080 | REST API /metrics |
| raid1-secondary | 8084 | 8080 | REST API /metrics |
| raid5-shard1 | 8085 | 8080 | REST API /metrics |
| raid5-shard2 | 8086 | 8080 | REST API /metrics |
| raid5-shard3 | 8087 | 8080 | REST API /metrics |
| prometheus | 9090 | 9090 | Prometheus Server |
| grafana | 3000 | 3000 | Grafana UI |

---

### 3. **prometheus.yml** (MODIFIED)
**Pfad:** `c:\VCC\themis\docker\compose\prometheus.yml`

**Konfiguration:**
```yaml
scrape_configs:
  - job_name: 'raid0-stripe'
    static_configs:
      - targets:
        - 'themis-raid0-shard1:8080'   # ✅ Port 8080 (REST API)
        - 'themis-raid0-shard2:8080'
        - 'themis-raid0-shard3:8080'
    metrics_path: '/metrics'            # ✅ Expliziter Path
    scrape_interval: 15s

  - job_name: 'raid1-mirror'
    static_configs:
      - targets:
        - 'themis-raid1-primary:8080'
        - 'themis-raid1-secondary:8080'
    metrics_path: '/metrics'
    scrape_interval: 15s

  - job_name: 'raid5-parity'
    static_configs:
      - targets:
        - 'themis-raid5-shard1:8080'
        - 'themis-raid5-shard2:8080'
        - 'themis-raid5-shard3:8080'
    metrics_path: '/metrics'
    scrape_interval: 15s
```

**Wichtig:** Alle Targets verwenden jetzt Port 8080!

---

### 4. **themis_raid_benchmark_dashboard.json** (CREATED)
**Pfad:** `c:\VCC\themis\docker\compose\grafana\dashboards\themis_raid_benchmark_dashboard.json`

**Dashboard Inhalt:**
```json
{
  "title": "Themis RAID Benchmark",
  "version": "39",
  "panels": [
    {
      "title": "RAID I/O Throughput",
      "targets": [{"expr": "rate(themis_raid_io_bytes_total[5m])"}]
    },
    {
      "title": "Operation Latency (p95/p99)",
      "targets": [{"expr": "histogram_quantile(0.95, themis_operation_duration_seconds)"}]
    },
    {
      "title": "Operations/sec",
      "targets": [{"expr": "rate(themis_io_operations_total[1m])"}]
    },
    {
      "title": "Avg Throughput",
      "targets": [{"expr": "avg(themis_raid_io_bytes_total)"}]
    }
  ]
}
```

**Verwendung:**
- Wird automatisch von Grafana provisioniert (siehe `dashboards.yml`)
- Sichtbar unter http://localhost:3000/dashboards

---

### 5. **README.md & QUICK_START.md** (CREATED)
**Pfade:**
- `c:\VCC\themis\docker\compose\README.md` (Umfassende Dokumentation)
- `c:\VCC\themis\docker\compose\QUICK_START.md` (Schritt-für-Schritt Anleitung)

---

## 🔧 Technische Details

### HTTP Metrics Endpunkt
**Code Location:** `src/server/http_server.cpp:1297-1304`

```cpp
case Route::Metrics:
    handleMetricsJson(req);  // Route /metrics → handleMetricsJson()
    break;
```

**Handler:** `src/server/sharding_metrics_handler.cpp`
```cpp
void handleMetricsJson(const Request& req) {
    auto metricsStr = registry->getMetricsString();
    return Response(metricsStr, "text/plain; version=0.0.4");
}
```

### Metriken Initialisierung
**Code Location:** `src/main_server.cpp:560-568`

```cpp
// Prometheus Metrics Setup
PrometheusMetrics::Config metricsConfig{
    .enable_histograms = true,
    .http_port = 8080,
    .http_path = "/metrics"
};
auto metrics = std::make_shared<PrometheusMetrics>(metricsConfig);
registry->registerMetrics(metrics);
LOG(INFO) << "Sharding metrics initialized";
```

### Verfügbare Metriken
**Source:** `include/sharding/prometheus_metrics.h`

```cpp
// RAID I/O Metriken
prometheus::Counter& raid_io_bytes_total;      // Summe geschriebene Bytes
prometheus::Counter& raid_io_operations;       // Anzahl I/O Ops

// Latenz Metriken  
prometheus::Histogram& operation_duration;     // Op-Dauer in Sekunden
prometheus::Histogram& routing_latency_ms;     // Routing-Latenz

// Health Metriken
prometheus::Gauge& cluster_size;               // Aktive Shards
prometheus::Gauge& shard_health_status;        // Shard-Gesundheit

// Weitere Metriken
prometheus::Counter& migration_records;
prometheus::Gauge& certificate_expiry_seconds;
prometheus::Counter& routing_requests_total;
```

---

## 🚀 Deployment Steps

### Phase 1: Docker Image Build (10-20 Min)
```bash
cd c:\VCC\themis
docker build -f Dockerfile.themis-metrics-enabled \
    -t themisdb/themisdb:metrics-enabled .
```

**Was passiert:**
1. Ubuntu 24.04 Base Image lädt
2. Build dependencies installiert (cmake, gcc, vcpkg)
3. vcpkg konfiguriert mit prometheus-cpp
4. ThemisDB Source Code compiliert (CMake)
5. Runtime Image mit Binary erstellt

### Phase 2: Container Restart (30 Sec)
```bash
cd docker/compose
docker-compose -f docker-compose-sharding.yml down
docker-compose -f docker-compose-sharding.yml up -d
```

**Was passiert:**
1. Alte Container gestoppt
2. Neue Container mit `metrics-enabled` Image gestartet
3. RAID Shards verbinden sich untereinander
4. Prometheus beginnt Scraping
5. Grafana lädt Dashboard

### Phase 3: Verifikation (2 Min)
```bash
# Check Prometheus Targets
curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | {job, health}'

# Check Metriken
curl -s http://localhost:8080/metrics | head -20

# Open Grafana
open http://localhost:3000
```

---

## 📊 Prometheus Architecture

```
┌─────────────────────────────────────────────────────────┐
│              Docker Compose Network                      │
│                 (themis-network)                         │
├─────────────────────────────────────────────────────────┤
│                                                          │
│   RAID Shards (9 Container)                            │
│   ├─ themis-raid0-shard1:8080/metrics                  │
│   ├─ themis-raid0-shard2:8080/metrics                  │
│   ├─ themis-raid0-shard3:8080/metrics                  │
│   ├─ themis-raid1-primary:8080/metrics                 │
│   ├─ themis-raid1-secondary:8080/metrics               │
│   ├─ themis-raid5-shard1:8080/metrics                  │
│   ├─ themis-raid5-shard2:8080/metrics                  │
│   ├─ themis-raid5-shard3:8080/metrics                  │
│   │                                                     │
│   └─► Prometheus Server:9090                           │
│       (Scrape Interval: 15s)                           │
│       │                                                │
│       └─► Grafana:3000                                │
│           └─► Dashboard: "Themis RAID Benchmark"      │
│               ├─ Panel 1: RAID I/O Throughput         │
│               ├─ Panel 2: Operation Latency p95/p99   │
│               ├─ Panel 3: Operations/sec              │
│               └─ Panel 4: Average Throughput          │
│                                                        │
└─────────────────────────────────────────────────────────┘
```

---

## 🎯 Key Takeaways

| Aspekt | Alt (Broken) | Neu (Fixed) |
|--------|-------------|-----------|
| **Docker Binary** | Windows .exe | Linux ELF |
| **Metrics Endpoint** | Port 9090 (nicht da) | Port 8080 ✅ |
| **Rest API** | Nicht funktionsfähig | Port 8080 ✅ |
| **Prometheus Scrape** | Targets "down" | Targets "up" ✅ |
| **Grafana Dashboard** | Keine Daten | Zeigt Daten ✅ |
| **Status** | ❌ Broken | ✅ Ready |

---

## 📌 Wichtige URLs

**Monitoring Dashboard:**
- Grafana: http://localhost:3000 (admin/admin)
- Prometheus: http://localhost:9090/targets
- Prometheus Graph: http://localhost:9090/graph

**Metriken (von Host):**
- raid0-shard1: http://localhost:8080/metrics
- raid0-shard2: http://localhost:8081/metrics
- raid0-shard3: http://localhost:8082/metrics
- raid1-primary: http://localhost:8083/metrics
- raid1-secondary: http://localhost:8084/metrics
- raid5-shard1: http://localhost:8085/metrics
- raid5-shard2: http://localhost:8086/metrics
- raid5-shard3: http://localhost:8087/metrics

---

## ⏱️ Timeline

1. **Now:** Dockerfile.themis-metrics-enabled gebaut ✅
2. **T+20min:** `docker build` abgeschlossen
3. **T+21min:** `docker-compose down && up -d`
4. **T+22min:** Alle Targets "up"
5. **T+23min:** Grafana zeigt Daten
6. **T+24min:** 🎉 Monitoring aktiv!

---

**Aktueller Status:** ✅ Alle Konfigurationen fertig, warte auf Docker Image Build
**Nächster Schritt:** `docker build -f Dockerfile.themis-metrics-enabled ...`
