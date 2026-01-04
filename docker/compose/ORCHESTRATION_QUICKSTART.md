# ThemisDB LLM RAID Orchestration Tests

## Überblick

Das Test-Orchestration-System automatisiert die komplette Validierungs-Pipeline:

```
1. Daten pushen        → 100-1000 Records in RAID Shards
2. Metriken sammeln    → Baseline vor Tests
3. Tests ausführen     → C++ Unit Tests + Benchmarks
4. Metriken sammeln    → Nach Tests vergleichen
5. Validieren          → Ergebnisse prüfen
6. Report generieren   → HTML + JSON
```

## Architektur

### Test-Server (themis-llm-raid-tests)

```
┌─────────────────────────────────────────────────────────┐
│  Orchestrator (Python)                                  │
│  - Daten pushen                                         │
│  - Metriken sammeln                                     │
│  - Tests starten                                        │
│  - Ergebnisse validieren                                │
│  - Reports generieren                                   │
└────────┬──────────────────────────────────────────┬─────┘
         │                                          │
         ▼                                          ▼
   ┌─────────────┐                          ┌──────────────┐
   │ C++ Tests   │                          │ Metriken     │
   │ - Inlining  │                          │ - Prometheus │
   │ - Pipeline  │                          │ - Grafana    │
   │ - Data Push │                          │              │
   └─────────────┘                          └──────────────┘
         │
         └─────────────────────────┬─────────────────────────┐
                                   ▼
                        ┌──────────────────────┐
                        │  RAID Cluster (9x)   │
                        │  - 3x RAID0 Striping │
                        │  - 2x RAID1 Mirroring│
                        │  - 3x RAID5 Parity   │
                        └──────────────────────┘
```

## Komponenten

### 1. Orchestrator Service (raid_lora_orchestrator.py)

**Aufgaben:**
- Wartet auf RAID Shards (healthcheck)
- Pusht Test-Daten (100-1000 Records)
- Sammelt Metriken vor/nach
- Startet C++ Tests
- Validiert Ergebnisse
- Generiert HTML Report

**Verwendung:**
```bash
python3 raid_lora_orchestrator.py [test_type]
```

**Test Types:**
- `pipeline` - RAID Pipeline Tests
- `inline` - LoRA Inline Tests
- `bench_lora` - LoRA Benchmarks
- `bench_pipeline` - Pipeline Benchmarks
- `all_tests` - Alle Unit Tests
- `all_bench` - Alle Benchmarks
- `all` - Alles (Tests + Benchmarks)

### 2. Data Push Service (raid_data_pusher.h)

**Klasse: RAIDDataPusher**

```cpp
// Warte auf Shards
bool waitForShardsHealthy(int timeout_seconds = 120);

// Pushe Test-Daten
PushResult pushTestData(
    const std::string& collection_name, 
    int num_records
);

// Sammle Metriken
MetricsSnapshot getShardMetrics(const ShardConfig& shard);

// Baseline + Post-Push Metriken
void collectMetricsBaseline();
void collectMetricsAfter();

// Generiere Report
json generateReport() const;
```

### 3. Test Suite (test_llm_raid_data_push.cpp)

**Unit Tests:**
- `PushSmallDataset` - 100 Records
- `PushLargeDataset` - 1000 Records
- `HandlePushFailures` - Fehlerbehandlung
- `MetricsCollectionBeforeAfter` - Metrik-Vergleiche
- `VerifyRoundRobinDistribution` - Verteilung prüfen
- `UnhealthyShardHandling` - Ausfallsicherheit
- `DataDistributionBalance` - Balance-Check

**Benchmarks:**
- `BM_Push100Records` - Durchsatz für 100
- `BM_Push1000Records` - Durchsatz für 1000
- `BM_MetricsCollection` - Metrik-Sammlung Latenz

## Docker Compose Setup

### Schnellstart

```bash
# 1. Repository zum Image builden
cd C:\VCC\themis
docker build -f docker/Dockerfile.llm-raid-tests \
  -t themisdb/themis-llm-raid-tests:latest .

# 2. RAID Cluster + Test-Service starten
cd docker/compose
docker-compose -f docker-compose-llm-raid-tests-orchestrated.yml up -d

# 3. Warten bis alle Shards healthy sind (60-90s)
docker-compose -f docker-compose-llm-raid-tests-orchestrated.yml ps

# 4. Tests ausführen (verschiedene Modi)
docker-compose -f docker-compose-llm-raid-tests-orchestrated.yml \
  run --rm -e TEST_TYPE=all themis-llm-raid-tests
```

### Shard Health Monitoring

```bash
# Alle Shards prüfen
docker-compose -f docker-compose-llm-raid-tests-orchestrated.yml ps

# Shard-Logs
docker logs themis-raid0-shard1

# RAID Cluster Status
curl http://localhost:8080/health
curl http://localhost:8081/health
# ... etc für alle Shards (8080-8087)
```

## Test-Modi

### Modus: Pipeline

```bash
docker-compose run --rm -e TEST_TYPE=pipeline themis-llm-raid-tests
```

- Führt: `test_llm_raid_pipeline` aus
- Testet: 6-Phase Pipeline (Daten → RAID Dist → LoRA → Inference)
- Output: `pipeline_results.xml`

### Modus: Inline

```bash
docker-compose run --rm -e TEST_TYPE=inline themis-llm-raid-tests
```

- Führt: `test_llm_lora_inline` aus
- Testet: LoRA Load/Unload, Switching, Export/Import
- Output: `inline_results.xml`

### Modus: All Tests

```bash
docker-compose run --rm -e TEST_TYPE=all_tests themis-llm-raid-tests
```

- Führt alle Unit Tests aus
- Output: Multiple XML files

### Modus: All Benchmarks

```bash
docker-compose run --rm -e TEST_TYPE=all_bench themis-llm-raid-tests
```

- Führt alle Benchmarks aus
- Output: JSON benchmark results

### Modus: Vollständig

```bash
docker-compose run --rm -e TEST_TYPE=all themis-llm-raid-tests
```

- Tests + Benchmarks + Orchestration
- Output: HTML Report + JSON Results

## Ergebnisse & Metriken

### Struktur

```
/test_results/
├── orchestrator_results.json    # Orchestrator Metadata
├── test_report.html              # HTML Report
├── pipeline_results.xml           # Test Results
├── inline_results.xml
├── lora_inline_results.json       # Benchmark Results
├── raid_pipeline_results.json
└── metrics/                       # Gesammelte Metriken
    ├── baseline_metrics.json
    └── post_test_metrics.json
```

### Ergebnisse abrufen

```bash
# Ergebnisse aus Container kopieren
docker cp themis-llm-raid-tests:/test_results/. ./local_results/

# Oder via bind mount (docker-compose.yml)
cd docker/compose
cat ../../../test_results/orchestrator_results.json
cat ../../../test_results/test_report.html
```

## Prometheus & Grafana Monitoring

### Prometheus Dashboard

```
http://localhost:9090
```

**Verfügbare Metriken:**
- `themis_documents_total` - Dokumente pro Shard
- `themis_disk_usage_bytes` - Speichernutzung
- `themis_inference_latency_ms` - Inference Latenz
- `themis_lora_cache_hits` - LoRA Cache Hits
- `themis_lora_cache_misses` - LoRA Cache Misses

### Grafana Dashboard

```
http://localhost:3000
Admin: admin / themis
```

**Dashboards:**
- RAID Cluster Status (Shards, Health)
- LoRA Cache Performance (Hits/Misses)
- Inference Performance (Latenz, Durchsatz)
- Data Distribution (RAID0/1/5 Balance)

## Metriken & Reports

### Automatische Metriken-Sammlung

Der Orchestrator sammelt automatisch:

1. **Baseline Metrics** (vor Tests)
   - Documents per Shard
   - Disk Usage per Shard
   - Cache Stats
   - LLM Plugin Status

2. **Post-Test Metrics** (nach Tests)
   - Identisch zu Baseline
   - Ermöglicht Vergleich

3. **Test Performance Metrics**
   - Dauer pro Phase
   - Records gepusht
   - Fehlerrate
   - Durchsatz (records/sec)

### HTML Report Struktur

```html
ThemisDB LLM RAID Pipeline Test Report

Phase 1: Data Push
  ✓ Status: Pushed 1000/1000 records
  - Erfolgsquote: 100%
  - Dauer: 2.5 Sekunden

Phase 2: Tests Executed
  ✓ test_llm_raid_pipeline: PASSED
  ✓ test_llm_lora_inline: PASSED
  ✓ bench_lora_inline: 12 benchmarks

Phase 3: Metrics & Validation
  ✓ 9 Shards: Metriken gesammelt
  - Documents: ~111 pro Shard (RAID0 Striping)
  - Distribution Balance: ✓ OK
```

## Fehlerbehandlung

### Shard nicht erreichbar

```bash
# Prüfe Shard Health
docker logs themis-raid0-shard1

# Restart Shard
docker-compose restart themis-raid0-shard1

# Warte auf Recovery
sleep 30
docker-compose ps
```

### Test-Fehler

```bash
# Logs des Test-Containers abrufen
docker logs themis-llm-raid-tests

# XML Test-Ergebnisse prüfen
docker exec themis-llm-raid-tests cat /test_results/pipeline_results.xml
```

### Metriken nicht gesammelt

```bash
# Prometheus Konfiguration prüfen
curl http://localhost:9090/api/v1/scrape_configs

# Targets prüfen
curl http://localhost:9090/api/v1/targets

# Manuelle Metrik-Query
curl http://localhost:9090/api/v1/query?query=themis_documents_total
```

## Performance Tuning

### Docker Desktop Ressourcen

```
Empfehlungen für 9 Shards + Tests:
- CPU: 8+ Cores
- RAM: 16+ GB
- Disk: 50+ GB
- Swap: 4+ GB
```

### Parallelisierung

```bash
# Tests mit Parallelisierung
TEST_TYPE=all docker-compose run themis-llm-raid-tests

# Einzelne Tests schneller
TEST_TYPE=inline docker-compose run themis-llm-raid-tests
```

### Data Push Optimierungen

```python
# In raid_lora_orchestrator.py anpassbar:
batch_size = 100        # Batch-Größe
num_records = 1000      # Anzahl Test-Records
timeout = 5             # Request Timeout
```

## Cleanup

```bash
# Nur Test-Service stoppen
docker-compose stop themis-llm-raid-tests

# Alle Services stoppen
docker-compose down

# Mit Volume-Cleanup
docker-compose down -v

# Ergebnisse vor Cleanup speichern
docker cp themis-llm-raid-tests:/test_results/. ./backup_results/
```

## Weitere Ressourcen

- [ThemisDB RAID Architecture](../README.md)
- [LLM Plugin Manager](../include/llm_plugin_manager.h)
- [Multi LoRA Manager](../include/multi_lora_manager.h)
- [Prometheus Metrics Guide](../docs/PROMETHEUS_INTEGRATION.md)
