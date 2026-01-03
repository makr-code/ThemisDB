# ✅ Prometheus Metrics Integration - Checkliste

## Status: VOLLSTÄNDIG KONFIGURIERT ✅

Alle notwendigen Konfigurationen sind abgeschlossen. Der Docker Image Rebuild ist der nächste Schritt.

---

## 📋 Was wurde erledigt?

### ✅ Dateien erstellt/modifiziert:

1. **Dockerfile.themis-metrics-enabled** ✅
   - [x] Linux-basierter Multi-Stage Build
   - [x] CMake + vcpkg Unterstützung
   - [x] prometheus-cpp Integration
   - [x] Port 8080 für REST API /metrics
   - [x] Ubuntu 24.04 Base Image
   - [x] Health Check auf /health
   - **Status:** Bereit zum Build

2. **docker-compose-sharding.yml** ✅
   - [x] Port 8080 zu allen 9 RAID-Shards hinzugefügt
   - [x] Korrekte Port-Mappings: 8080→8080, 8081→8080, etc.
   - [x] `THEMIS_ENABLE_METRICS: "true"` in allen Shards
   - [x] `THEMIS_METRICS_PORT: "9090"` definiert
   - **Status:** Aktualisiert und bereit

3. **prometheus.yml** ✅
   - [x] Alle Targets auf Port 8080 (REST API)
   - [x] `metrics_path: '/metrics'` explizit gesetzt
   - [x] Scrape Interval: 15s
   - [x] 3 Jobs: raid0-stripe, raid1-mirror, raid5-parity
   - [x] Alle 9 Shards konfiguriert
   - **Status:** Korrekt konfiguriert

4. **themis_raid_benchmark_dashboard.json** ✅
   - [x] Kopiert zu `docker/compose/grafana/dashboards/`
   - [x] Grafana v39 Schema validiert
   - [x] 4 Panels mit PromQL Queries
   - [x] Variablen (datasource, raid_group)
   - [x] Automatisch durch dashboards.yml provisioniert
   - **Status:** Einsatzbereit

5. **Dokumentation** ✅
   - [x] **PROMETHEUS_METRICS_BUILD.md** - Detaillierte Build-Anleitung
   - [x] **PROMETHEUS_INTEGRATION_COMPLETE.md** - Technische Übersicht
   - [x] **docker/compose/README.md** - Umfassende Dokumentation
   - [x] **docker/compose/QUICK_START.md** - Schritt-für-Schritt Guide
   - [x] **docker/compose/CONFIGURATION_CHECKLIST.md** - Diese Datei
   - **Status:** Komplett

---

## 🚀 Nächste Schritte (MANUELL)

### Schritt 1: Docker Image bauen (15-25 Min)

```powershell
cd c:\VCC\themis

# Baue neues Image mit prometheus-cpp
docker build -f Dockerfile.themis-metrics-enabled `
    -t themisdb/themisdb:metrics-enabled .
```

**Was zu sehen ist:**
- `Step 1/X : FROM ubuntu:24.04` - Base Image lädt
- `RUN apt-get install ...` - Dependencies installiert
- `RUN cmake ...` - Build konfiguriert
- `RUN cmake --build` - Kompilierung läuft (längster Teil)
- `Sending build context to Docker daemon`
- `Successfully tagged themisdb/themisdb:metrics-enabled`

**Troubleshooting:**
- Wenn Docker nicht läuft: `docker ps` → sollte Fehler zeigen
- Wenn Netzwerk-Fehler: VPN prüfen oder offline Dockerfile verwenden
- Wenn Speicher-Fehler: `docker system prune` und erneut versuchen

---

### Schritt 2: Container Stack neustarten (1 Min)

```powershell
cd c:\VCC\themis\docker\compose

# Alte Container stoppen
docker-compose -f docker-compose-sharding.yml down

# Neue Container mit metrics-enabled Image starten
docker-compose -f docker-compose-sharding.yml up -d
```

**Was zu sehen ist:**
- `Stopping themis-prometheus ... done`
- `Stopping themis-raid0-shard1 ... done`
- `Creating network themis-network`
- `Creating themis-prometheus ... done`
- `Creating themis-raid0-shard1 ... done`
- ...weitere Container...
- Alle 11 Container sollten starten

---

### Schritt 3: Auf vollständigen Start warten (30 Sec)

```powershell
# Warte mindestens 30 Sekunden
Start-Sleep -Seconds 30

# Dann prüfe Container Status
docker-compose -f docker-compose-sharding.yml ps
```

**Erwartete Ausgabe:**
```
NAME                    STATUS              
themis-prometheus       Up 20s (healthy)    
themis-grafana          Up 20s              
themis-raid0-shard1     Up 25s (healthy)    
themis-raid0-shard2     Up 25s (healthy)    
themis-raid0-shard3     Up 25s (healthy)    
themis-raid1-primary    Up 25s (healthy)    
themis-raid1-secondary  Up 25s (healthy)    
themis-raid5-shard1     Up 25s (healthy)    
themis-raid5-shard2     Up 25s (healthy)    
themis-raid5-shard3     Up 25s (healthy)    
```

---

### Schritt 4: Metriken verifizieren

```powershell
# Test 1: Prometheus Targets Status
curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | {job: .labels.job, instance: .labels.instance, health}'

# Erwartete Ausgabe: health sollte "up" sein für alle
```

```powershell
# Test 2: Metriken direkt abrufen
curl -s http://localhost:8080/metrics | Select-Object -First 20

# Sollte Prometheus Format zeigen:
# # HELP themis_raid_io_bytes_total Total bytes read/written
# # TYPE themis_raid_io_bytes_total counter
# themis_raid_io_bytes_total{...} 0
```

```powershell
# Test 3: Prometheus API Query
curl -s 'http://localhost:9090/api/v1/query?query=themis_raid_io_bytes_total' | jq '.data.result | length'

# Sollte > 0 sein (mehrere Metriken gefunden)
```

---

### Schritt 5: Grafana Dashboard öffnen

```
Browser: http://localhost:3000
Login: admin / admin
Navigation: Dashboards → "Themis RAID Benchmark"
```

**Was zu sehen ist:**
- 4 Panels mit Metriken
- Daten sollten nach 30-60 Sekunden sichtbar sein
- Wenn noch keine Daten: Kurz warten und F5 drücken

---

## ✅ Verifikations-Checkliste

Nachdem Sie alle Schritte erledigt haben, prüfen Sie:

- [ ] **Docker Build erfolgreich:** `docker image ls | grep metrics-enabled`
  - Sollte zeigen: `themisdb/themisdb  metrics-enabled  ...`

- [ ] **Alle 11 Container laufen:** `docker-compose ps`
  - 9 RAID Shards + Prometheus + Grafana = 11 Container

- [ ] **Prometheus Targets "up":** `curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets | length'`
  - Sollte zeigen: `9` (9 RAID-Shards)

- [ ] **Metriken verfügbar:** `curl -s http://localhost:8080/metrics | wc -l`
  - Sollte zeigen: `> 100` (viele Metriken)

- [ ] **Grafana Dashboard existiert:** http://localhost:3000/d/themis-raid-benchmark
  - Dashboard sollte laden

- [ ] **Grafana zeigt Daten:** Die 4 Panels sollten nicht leere Charts zeigen
  - Wenn leer: F5 drücken und 30 Sekunden warten

---

## 📞 Hilfe bei Problemen

### Problem: Docker Build schlägt fehl

**Symptom:**
```
ERROR: failed to solve: ...
```

**Lösungen:**
1. Docker neu starten: `Restart-Service docker` (Admin PowerShell)
2. Dockerfile prüfen: `cat Dockerfile.themis-metrics-enabled | head -20`
3. Build Output speichern: `docker build ... 2>&1 | Tee-Object build.log`

### Problem: Container starten nicht

**Symptom:**
```
docker-compose: error: No such service: themis-raid0-shard1
```

**Lösungen:**
1. Docker-Compose Datei prüfen: `Test-Path docker\compose\docker-compose-sharding.yml`
2. Container Logs lesen: `docker logs themis-raid0-shard1`
3. Network prüfen: `docker network ls | grep themis`

### Problem: Prometheus zeigt "down"

**Symptom:**
```
curl http://localhost:9090/api/v1/targets
# "health": "down" für alle targets
```

**Lösungen:**
1. Container Port prüfen: `docker port themis-raid0-shard1 | grep 8080`
   - Sollte zeigen: `8080/tcp -> 0.0.0.0:8080`
2. Erreichbarkeit testen: `curl http://localhost:8080/health`
   - Sollte 200 OK zurückgeben
3. Prometheus Config prüfen: `cat docker/compose/prometheus.yml | grep -A 5 "raid0"`
   - Sollte zeigen: `targets: ... :8080`

### Problem: Grafana zeigt keine Daten

**Symptom:**
```
Dashboard öffnet, aber Panels sind leer
```

**Lösungen:**
1. Datasource prüfen:
   - Grafana → Configuration → Data Sources
   - "Prometheus" sollte "green" Status haben
2. Metriken existieren?: `curl -s 'http://localhost:9090/api/v1/query?query=themis_raid_io_bytes_total' | jq '.data.result'`
   - Sollte nicht `null` sein
3. Browser Cache: `Ctrl+Shift+Delete` → Cache löschen → Seite neu laden

---

## 📚 Relevante Dateien

**Konfiguration:**
- `Dockerfile.themis-metrics-enabled` - Docker Image Definition
- `docker/compose/docker-compose-sharding.yml` - Container Orchestration
- `docker/compose/prometheus.yml` - Metriken Scraping
- `docker/compose/grafana/datasources.yml` - Grafana Datasource
- `docker/compose/grafana/dashboards.yml` - Dashboard Provisioning

**Dashboard:**
- `docker/compose/grafana/dashboards/themis_raid_benchmark_dashboard.json`

**Code (für Referenz):**
- `src/server/http_server.cpp:1297-1304` - /metrics Route Handler
- `src/main_server.cpp:560-568` - Metriken Initialisierung
- `include/sharding/prometheus_metrics.h` - Metriken Klasse

**Dokumentation:**
- `PROMETHEUS_METRICS_BUILD.md` - Build-Anleitung
- `PROMETHEUS_INTEGRATION_COMPLETE.md` - Technische Details
- `docker/compose/README.md` - Umfassende Dokumentation
- `docker/compose/QUICK_START.md` - Schritt-für-Schritt Guide

---

## 🎯 Was danach?

Nach erfolgreicher Integration:

1. **Metriken testen:** Führe einige RAID-Operationen durch
   ```bash
   # Von außerhalb des Clusters
   curl -X POST http://localhost:18765/api/write \
     -H "Content-Type: application/json" \
     -d '{"shard": "raid0-1", "data": "test"}'
   ```

2. **Grafana Dashboards erweitern:**
   - Weitere Panels hinzufügen
   - Alerts konfigurieren
   - Custom PromQL Queries schreiben

3. **Monitoring Alerting:**
   - Prometheus Alert Rules erstellen
   - Alertmanager konfigurieren
   - Slack/Email Notifications

4. **Performance Tuning:**
   - Scrape Interval anpassen (aktuell 15s)
   - Retention Policy setzen
   - Disk Space prüfen

---

## 📊 Erwartete Metriken nach Start

Nach erfolgreichem Start sollten diese Metriken verfügbar sein:

```
# RAID I/O Metriken
themis_raid_io_bytes_total{raid_group="raid0", shard_id="raid0-1"} 0
themis_io_operations_total{raid_group="raid0", shard_id="raid0-1"} 0

# Operation Latenz
themis_operation_duration_seconds_bucket{...} 0
themis_operation_duration_seconds_sum{...} 0
themis_operation_duration_seconds_count{...} 0

# Cluster Health
themis_cluster_size{job="raid0-stripe"} 3
themis_shard_health_status{shard_id="raid0-1", status="healthy"} 1

# Routing Metriken
themis_routing_requests_total{operation="write"} 0
themis_routing_latency_ms{...} 0

# PKI Metriken
themis_certificate_expiry_seconds{certificate="themis-ca"} ...

# Migration Metriken
themis_migration_progress_percent{...} 0
themis_migration_records_total{...} 0
```

---

**Status:** ✅ Bereit für Docker Build  
**Nächster Schritt:** `docker build -f Dockerfile.themis-metrics-enabled -t themisdb/themisdb:metrics-enabled .`  
**Geschätzte Zeit:** 15-25 Minuten
