# RAID LoRA Orchestration - File Index & Quicklinks

## 📚 Dokumentation (Start hier!)

### 🚀 Getting Started
- **[IMPLEMENTATION_COMPLETE.md](IMPLEMENTATION_COMPLETE.md)** - Überblick was implementiert wurde (5 Min Lesen)
- **[RAID_ORCHESTRATION_QUICKREF.md](RAID_ORCHESTRATION_QUICKREF.md)** - Quick Reference Card (1 Min Lesen)

### 📖 Vollständige Guides
- **[ORCHESTRATION_QUICKSTART.md](docker/compose/ORCHESTRATION_QUICKSTART.md)** - Kompletter Leitfaden mit Beispielen (10 Min Lesen)
- **[RAID_ORCHESTRATION_SUMMARY.md](RAID_ORCHESTRATION_SUMMARY.md)** - Technischer Deep Dive (15 Min Lesen)
- **[RAID_ORCHESTRATION_ARCHITECTURE.md](RAID_ORCHESTRATION_ARCHITECTURE.md)** - Architektur & Diagramme (10 Min Lesen)

---

## 🎯 Code-Dateien

### Python Orchestrator
**Datei:** `docker/raid_lora_orchestrator.py` (480 Zeilen)

```python
class RAIDLoRAPipelineOrchestrator:
    # Daten in RAID pushen
    push_test_data_to_shards()        # HTTP POST, round-robin distribution
    
    # Metriken sammeln
    collect_metrics_baseline()        # Vor Tests
    collect_metrics_after()           # Nach Tests
    
    # Tests orchestrieren
    run_tests(test_type)              # C++ Test Execution
    
    # Ergebnisse validieren
    validate_results()                # XML/JSON Parsing
    
    # Report generieren
    generate_report()                 # HTML Output
    
    # Hauptpipeline
    run_full_pipeline(test_type)      # Alles zusammen
```

**Verwendung:**
```bash
python3 docker/raid_lora_orchestrator.py all
python3 docker/raid_lora_orchestrator.py pipeline
python3 docker/raid_lora_orchestrator.py inline
```

### C++ Header: Data Pusher
**Datei:** `include/raid_data_pusher.h` (280 Zeilen)

```cpp
class RAIDDataPusher {
    // Konfiguration
    initializeShards()                     // 9 Shards setup
    
    // Health Check
    waitForShardsHealthy(timeout=120)     // Block bis ready
    
    // Daten Push
    pushTestData(collection, num_records) // HTTP POST
    
    // Metriken
    getShardMetrics(shard)                // Curl Prometheus
    collectMetricsBaseline()
    collectMetricsAfter()
    
    // Output
    generateReport()                      // JSON Report
};
```

**Verwendung:**
```cpp
RAIDDataPusher pusher;
pusher.waitForShardsHealthy();
auto result = pusher.pushTestData("test_collection", 1000);
pusher.collectMetricsBaseline();
pusher.collectMetricsAfter();
auto report = pusher.generateReport();
```

### C++ Tests
**Datei:** `tests/test_llm_raid_data_push.cpp` (480 Zeilen)

**Unit Tests (7):**
- `PushSmallDataset` - 100 Records
- `PushLargeDataset` - 1000 Records
- `HandlePushFailures` - Fehlerbehandlung
- `MetricsCollectionBeforeAfter` - Before/After
- `VerifyRoundRobinDistribution` - RAID0 Striping
- `UnhealthyShardHandling` - Resilience
- `DataDistributionBalance` - Balance Check

**Benchmarks (3):**
- `BM_Push100Records` - Durchsatz
- `BM_Push1000Records` - Scale Test
- `BM_MetricsCollection` - Metrik Latenz

**Verwendung:**
```bash
# Kompilieren
cmake --build build-msvc --target test_llm_raid_data_push

# Ausführen
./build-msvc/test_llm_raid_data_push

# Mit Google Benchmark
./build-msvc/test_llm_raid_data_push --benchmark
```

---

## 🐳 Docker & Orchestration

### Docker Compose (Orchestrated)
**Datei:** `docker/compose/docker-compose-llm-raid-tests-orchestrated.yml` (350 Zeilen)

**Services:**
```yaml
Services (11):
  themis-raid0-shard1/2/3       # RAID0 Striping
  themis-raid1-primary/mirror   # RAID1 Mirroring
  themis-raid5-shard1/2/3       # RAID5 Parity
  themis-llm-raid-tests         # Orchestrator
  prometheus                     # Metrics
  grafana                        # Dashboards

Ports:
  REST API: 8080-8087
  Prometheus: 9090-9097 (per shard)
  Prometheus Server: 9090
  Grafana: 3000
```

**Verwendung:**
```bash
# Start
docker-compose -f docker/compose/docker-compose-llm-raid-tests-orchestrated.yml up -d

# Status
docker-compose -f docker/compose/docker-compose-llm-raid-tests-orchestrated.yml ps

# Stop
docker-compose -f docker/compose/docker-compose-llm-raid-tests-orchestrated.yml down -v
```

### Docker Entry Point
**Datei:** `docker/test-entrypoint-orchestrated.sh` (40 Zeilen)

```bash
#!/bin/bash
# Startet Orchestrator mit Test-Typ
python3 /opt/themis/bin/raid_lora_orchestrator.py $TEST_TYPE
```

**Environment Variables:**
```bash
TEST_TYPE=all                    # Test Mode
LOG_LEVEL=INFO
RAID0_SHARDS=shard1,shard2,shard3
RAID1_SHARDS=primary,mirror
RAID5_SHARDS=shard1,shard2,shard3
```

### Prometheus Config
**Datei:** `docker/compose/prometheus.yml` (85 Zeilen)

**Jobs:**
```yaml
raid0-shard1/2/3    Port: 9090-9092
raid1-primary       Port: 9093
raid1-mirror        Port: 9094
raid5-shard1/2/3    Port: 9095-9097
prometheus          Localhost
```

**Metriken-Path:** `/metrics`
**Scrape Interval:** 10s
**Timeout:** 5s

---

## 🔧 Build & Automation

### Makefile für Tests
**Datei:** `Makefile.raid-tests` (180 Zeilen)

**Alle Targets:**
```makefile
build               # Docker Image bauen
up                  # Services starten
down                # Services stoppen
test                # Default tests (pipeline)
test-inline         # LoRA Inline Tests
test-pipeline       # RAID Pipeline Tests
test-bench          # Alle Benchmarks
test-all            # Tests + Benchmarks
logs                # Test Logs anschauen
health              # Cluster Status
results             # Ergebnisse kopieren
metrics             # Prometheus Metriken
grafana             # Grafana öffnen
prometheus          # Prometheus öffnen
clean               # Cleanup
restart             # Restart
ci-test             # CI/CD Test Run
```

**Verwendung:**
```bash
# Complete workflow
make -f Makefile.raid-tests build up test-all results clean

# Single operations
make -f Makefile.raid-tests test
make -f Makefile.raid-tests logs
make -f Makefile.raid-tests health
```

---

## 📊 Output & Results

### HTML Report
**Datei:** `test_results/test_report.html` (Auto-generated)

**Inhalt:**
- Phase 1: Data Push Status
- Phase 2: Tests Executed
- Phase 3: Metrics & Validation
- Summary: PASSED/FAILED

**Öffnen:**
```bash
# Lokal
cat test_results/test_report.html

# Browser
open test_results/test_report.html
```

### JSON Results
**Datei:** `test_results/orchestrator_results.json` (Auto-generated)

**Struktur:**
```json
{
  "phases": {
    "data_push": { "status": "success", "pushed_records": 1000 },
    "test_llm_raid_data_push": { "status": "passed" }
  },
  "metrics": {
    "baseline": { "documents": 0, "disk_usage": 1073741824 },
    "after_push": { "documents": 1000, "per_shard": 111 }
  },
  "validation": {
    "result_files": { "count": 8 },
    "passed_checks": 12
  }
}
```

**Verwendung:**
```bash
# Pretty Print
cat test_results/orchestrator_results.json | jq .

# Extract Phase
jq '.phases' test_results/orchestrator_results.json
```

---

## 🌐 Web Dashboards

### Prometheus
**URL:** http://localhost:9090

**Features:**
- Query Builder
- Metrics Explorer
- Targets View
- Graph Visualization

**Nützliche Queries:**
```promql
themis_documents_total
themis_disk_usage_bytes
themis_lora_cache_hits
rate(themis_documents_total[1m])
```

### Grafana
**URL:** http://localhost:3000  
**User:** admin  
**Pass:** themis

**Dashboards:**
- RAID Cluster Status
- LoRA Cache Performance
- Inference Metrics
- Data Distribution

---

## 🎯 Quick Start Flows

### Flow 1: Test ausführen (2 Minuten)
```bash
# Terminal 1
cd C:\VCC\themis
make -f Makefile.raid-tests test

# Terminal 2
make -f Makefile.raid-tests results

# Öffne
cat test_results/test_report.html
```

### Flow 2: Complete Setup (10 Minuten)
```bash
make -f Makefile.raid-tests build         # 5 min
make -f Makefile.raid-tests up            # 2 min
make -f Makefile.raid-tests test-all      # 1 min
make -f Makefile.raid-tests results       # instant
```

### Flow 3: Monitoring (Parallel)
```bash
# Terminal 1
watch docker-compose -f docker/compose/docker-compose-llm-raid-tests-orchestrated.yml ps

# Terminal 2
docker logs -f themis-llm-raid-tests

# Terminal 3
open http://localhost:3000  # Grafana
open http://localhost:9090  # Prometheus
```

### Flow 4: CI/CD Integration
```bash
make -f Makefile.raid-tests ci-test
# Builds image, runs tests, cleans up
```

---

## 🔍 Debugging & Inspection

### Logs
```bash
# Test Container
docker logs -f themis-llm-raid-tests

# Shard Logs
docker logs -f themis-raid0-shard1

# Compose Services
docker-compose -f docker/compose/docker-compose-llm-raid-tests-orchestrated.yml logs
```

### Inspection
```bash
# Health Check
curl http://localhost:8080/health

# Shard Status
curl http://localhost:8080/api/v1/collections

# Metrics
curl http://localhost:9090/api/v1/targets
curl http://localhost:9090/api/v1/query?query=up

# Copy Results
docker cp themis-llm-raid-tests:/test_results/. ./results/
```

### Cleanup
```bash
# Stop
docker-compose down

# Remove Volumes
docker-compose down -v

# Complete Cleanup
docker system prune -a
```

---

## 📋 File Structure Overview

```
C:\VCC\themis\
├─ 📄 IMPLEMENTATION_COMPLETE.md           [Summary]
├─ 📄 RAID_ORCHESTRATION_QUICKREF.md       [Quick Ref]
├─ 📄 RAID_ORCHESTRATION_SUMMARY.md        [Technical]
├─ 📄 RAID_ORCHESTRATION_ARCHITECTURE.md   [Design]
├─ 📄 THIS_FILE.md                         [Index]
│
├─ 🐍 docker/
│  ├─ raid_lora_orchestrator.py            [Main Orchestrator]
│  ├─ test-entrypoint-orchestrated.sh      [Docker Entry]
│  └─ compose/
│     ├─ docker-compose-llm-raid-tests-orchestrated.yml
│     ├─ prometheus.yml
│     └─ ORCHESTRATION_QUICKSTART.md
│
├─ 🔧 include/
│  └─ raid_data_pusher.h                   [C++ Header]
│
├─ 🧪 tests/
│  └─ test_llm_raid_data_push.cpp          [C++ Tests]
│
├─ 🎯 Makefile.raid-tests                  [Build Automation]
│
└─ 📊 test_results/                        [Output Directory]
   ├─ test_report.html
   ├─ orchestrator_results.json
   ├─ *_results.xml
   └─ *_results.json
```

---

## 🚀 Empfohlener Start

1. **Lesen:** `IMPLEMENTATION_COMPLETE.md` (5 min)
2. **Review:** `RAID_ORCHESTRATION_QUICKREF.md` (2 min)
3. **Setup:** `make -f Makefile.raid-tests build` (5 min)
4. **Run:** `make -f Makefile.raid-tests up test-all` (2 min)
5. **Inspect:** `make -f Makefile.raid-tests results` (instant)

---

## ✅ Checklist vor erstem Test

- [ ] Docker Desktop läuft
- [ ] WSL2 configured (if on Windows)
- [ ] C++ Compiler available (MSVC/GCC/Clang)
- [ ] Python3 installed
- [ ] pip packages: `requests`, `prometheus-client`
- [ ] 16+ GB RAM verfügbar
- [ ] 50+ GB Disk Space verfügbar
- [ ] Keine Ports 8080-8087, 9090-9097, 3000 blockiert

---

## 🆘 Häufige Fragen

**Q: Wie lange dauert der komplette Test?**  
A: ~45-60 Sekunden (nach Image build und startup)

**Q: Wo finde ich Ergebnisse?**  
A: `/test_results/` oder `docker cp themis-llm-raid-tests:/test_results/ ./`

**Q: Wie starte ich nur einen Test-Modus?**  
A: `make test-inline` oder `make test-pipeline`

**Q: Wie debugge ich fehlerhafte Tests?**  
A: `docker logs -f themis-llm-raid-tests` oder XML-Datei in `/test_results/`

**Q: Wie benutze ich Grafana?**  
A: http://localhost:3000 (admin/themis)

**Q: Wie stoppe ich alles?**  
A: `make -f Makefile.raid-tests clean`

---

**Status:** ✅ Complete & Ready  
**Next Step:** Read IMPLEMENTATION_COMPLETE.md
