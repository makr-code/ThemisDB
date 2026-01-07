# 🚀 ThemisDB RAID Monitoring - Quick Start Guide

## ⚠️ Kritische Voraussetzung

Der aktuelle Docker Image (`themisdb/themisdb:latest`) enthält einen **Windows-Binary** (`build-msvc/Release/themis_server.exe`), der in Linux-Containern nicht ausgeführt werden kann.

**Dies ist die Ursache:** Keine Prometheus-Metriken verfügbar ❌

## ✅ Lösung: Docker Image mit Linux Build neu erstellen

### Schritt 1: Neues Image bauen

```bash
cd c:\VCC\themis

# Baue neues Image mit prometheus-cpp Support
docker build -f Dockerfile.themis-metrics-enabled \
    -t themisdb/themisdb:metrics-enabled .
```

**Dauer:** ca. 10-20 Minuten (CMake + vcpkg + prometheus-cpp Kompilierung)

### Schritt 2: docker-compose Stack aktualisieren

```bash
cd docker/compose

# Alte Container stoppen
docker-compose -f docker-compose-sharding.yml down

# Stack mit neuem Image starten
docker-compose -f docker-compose-sharding.yml up -d
```

**Warte:** ~30 Sekunden für vollständigen Start

### Schritt 3: Metriken verifizieren

```bash
# Überprüfe ob Prometheus Daten sammelt
curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | {job: .labels.job, health}'

# Erwartete Ausgabe:
# {
#   "job": "raid0-stripe",
#   "health": "up"
# }
# {
#   "job": "raid1-mirror",
#   "health": "up"
# }
# {
#   "job": "raid5-parity",
#   "health": "up"
# }
```

### Schritt 4: Grafana Dashboard öffnen

```
Browser: http://localhost:3000
Login: admin / admin
Dashboard: "Themis RAID Benchmark"
```

Das Dashboard sollte jetzt Daten anzeigen! 📊

---

## 📊 Exposed Metrics Ports

| Service | Port (Host) | Port (Container) | Endpunkt |
|---------|------------|-----------------|----------|
| raid0-shard1 | 8080 | 8080 | `/metrics` |
| raid0-shard2 | 8081 | 8080 | `/metrics` |
| raid0-shard3 | 8082 | 8080 | `/metrics` |
| raid1-primary | 8083 | 8080 | `/metrics` |
| raid1-secondary | 8084 | 8080 | `/metrics` |
| raid5-shard1 | 8085 | 8080 | `/metrics` |
| raid5-shard2 | 8086 | 8080 | `/metrics` |
| raid5-shard3 | 8087 | 8080 | `/metrics` |
| Prometheus | 9090 | 9090 | `/api/v1/targets` |
| Grafana | 3000 | 3000 | Web UI |

---

## 🔍 Metriken testen (von Host)

```bash
# Einzelne Metriken abrufen
curl -s http://localhost:8080/metrics | grep themis_raid

# Prometheus Abfrage
curl -s 'http://localhost:9090/api/v1/query?query=themis_raid_io_bytes_total' | jq
```

---

## 🐛 Troubleshooting

### Problem: "docker build" fehlgeschlagen

**Lösung:** Prüfe ob Docker running ist
```bash
docker ps
# Sollte bestehende Container zeigen
```

### Problem: Prometheus zeigt "down" für Targets

**Überprüfe:**
```bash
# Container Status
docker-compose -f docker/compose/docker-compose-sharding.yml ps

# Container Logs
docker logs themis-raid0-shard1

# Manuell testen
curl -s http://localhost:8080/metrics | head -20
```

### Problem: Grafana zeigt keine Daten

1. **Prüfe ob Prometheus Daten hat:**
   ```bash
   curl -s 'http://localhost:9090/api/v1/query?query=themis_raid_io_bytes_total' | jq '.data.result | length'
   # Sollte > 0 sein
   ```

2. **Dashboard reload:** `Ctrl+Shift+R` im Browser

3. **Datasource prüfen:**
   - Grafana → Configuration → Data Sources
   - "Prometheus" sollte "green" sein

---

## 📝 Dateien die geändert wurden

- ✅ **Dockerfile.themis-metrics-enabled** (NEU)
  - Linux-basierter Multi-Stage Build
  - CMake + vcpkg Integration
  - prometheus-cpp Support

- ✅ **docker/compose/docker-compose-sharding.yml** (MODIFIED)
  - Port 8080 hinzugefügt für alle 9 RAID-Shards
  - Korrekte Port-Mappings: 8080→8080, 8081→8080, etc.

- ✅ **docker/compose/prometheus.yml** (MODIFIED)
  - Targets zeigen auf Port 8080 (nicht 9090)
  - Explizit: `metrics_path: '/metrics'`

- ✅ **docker/compose/grafana/dashboards/themis_raid_benchmark_dashboard.json** (CREATED)
  - Dashboard mit 4 Monitoring-Panels
  - Richtigen Prometheus Queries

- ✅ **docker/compose/README.md** (CREATED)
  - Ausführliche Dokumentation

---

## ✅ Checkliste

- [ ] Docker Image gebaut: `docker build -f Dockerfile.themis-metrics-enabled ...`
- [ ] `docker-compose up -d` ausgeführt
- [ ] 30 Sekunden gewartet
- [ ] `curl -s http://localhost:9090/api/v1/targets` zeigt "up" für alle Targets
- [ ] `curl -s http://localhost:8080/metrics` gibt Prometheus-Daten zurück
- [ ] Grafana Dashboard öffnet (http://localhost:3000)
- [ ] Dashboard zeigt Daten in allen 4 Panels

---

**Status:** ✅ Ready für Rebuild  
**Estimated Time:** 15-25 Minuten (Docker build + Container Start)
