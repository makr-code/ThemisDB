# Docker Compose Quick Start Guide for LLM RAID Pipeline Tests

## Overview

Das LLM RAID Pipeline Test-Setup in Docker Desktop besteht aus:
- **9-Shard RAID Cluster** (RAID0, RAID1, RAID5)
- **LLM Model + LoRA Services** auf allen Shards
- **Dedicated Test Service** für Inlining, Distribution, Inferencing
- **Monitoring** (Prometheus + Grafana)

## Quick Start

### 1. Build der Test-Images

```powershell
# Basis-Image bauen (falls nicht vorhanden)
cd C:\VCC\themis
docker build -f Dockerfile.themis-server -t themisdb/themisdb:hyperscaler .

# Test-Suite Image bauen
docker build -f docker/Dockerfile.llm-raid-tests -t themisdb/themis-llm-raid-tests:latest .
```

### 2. Cluster starten

```powershell
cd docker/compose

# Nur RAID Cluster (ohne Tests)
docker-compose -f docker-compose-sharding.yml up -d

# RAID Cluster + Test-Service
docker-compose -f docker-compose-llm-raid-tests.yml up -d
```

### 3. Test-Service ausführen

```powershell
# Pipeline Tests ausführen
docker-compose -f docker-compose-llm-raid-tests.yml \
  -e TEST_TYPE=pipeline \
  exec themis-llm-raid-tests test-entrypoint.sh pipeline

# Inline Tests
docker-compose -f docker-compose-llm-raid-tests.yml \
  -e TEST_TYPE=inline \
  exec themis-llm-raid-tests test-entrypoint.sh inline

# Alle Benchmarks
docker-compose -f docker-compose-llm-raid-tests.yml \
  -e TEST_TYPE=all_bench \
  exec themis-llm-raid-tests test-entrypoint.sh all_bench

# Komplettes Paket (Tests + Benchmarks)
docker-compose -f docker-compose-llm-raid-tests.yml \
  -e TEST_TYPE=all \
  exec themis-llm-raid-tests test-entrypoint.sh all
```

## Test-Modi

Der Test-Service unterstützt folgende Modi:

| Modus | Befehl | Beschreibung |
|-------|--------|-------------|
| **pipeline** | `test-entrypoint.sh pipeline` | Full 6-Phase Pipeline Tests |
| **inline** | `test-entrypoint.sh inline` | Inline LoRA Tests |
| **bench_lora** | `test-entrypoint.sh bench_lora` | LoRA Inline Benchmarks |
| **bench_pipeline** | `test-entrypoint.sh bench_pipeline` | RAID Pipeline Benchmarks |
| **all_tests** | `test-entrypoint.sh all_tests` | Alle Unit Tests |
| **all_bench** | `test-entrypoint.sh all_bench` | Alle Benchmarks |
| **all** | `test-entrypoint.sh all` | Tests + Benchmarks |

## Volumes & Daten

Test-Ergebnisse werden in lokale Verzeichnisse gemountet:

```
./test_data/          # Test-Datensätze (werden generiert)
./test_models/        # Base Model (GGUF)
./test_loras/         # LoRA Adapter
./test_results/       # Test-Ausgaben (XML, JSON, Logs)
```

## Monitoring

Nach dem Start:

```
Prometheus: http://localhost:9090
Grafana:    http://localhost:3000 (admin/admin)
```

Shard-Metriken:
- RAID0 Shards: Ports 8080-8082 (REST API), 9091-9093 (Metrics)
- RAID1 Shards: Ports 8083-8084 (REST API), 9094-9095 (Metrics)
- RAID5 Shards: Ports 8085-8087 (REST API), 9096-9098 (Metrics)

## Logs & Debugging

```powershell
# Test-Service Logs
docker-compose -f docker-compose-llm-raid-tests.yml logs -f themis-llm-raid-tests

# Spezifischen Shard
docker-compose -f docker-compose-llm-raid-tests.yml logs -f themis-raid0-shard1

# Test-Ergebnisse ansehen
docker exec themis-llm-raid-tests ls -la /test_results/
docker exec themis-llm-raid-tests cat /test_results/pipeline_results.xml
```

## Cleanup

```powershell
# Container stoppen
docker-compose -f docker-compose-llm-raid-tests.yml down

# Volumes löschen
docker-compose -f docker-compose-llm-raid-tests.yml down -v

# Images entfernen
docker rmi themisdb/themis-llm-raid-tests:latest
```

## Typischer Workflow

```powershell
# 1. RAID Cluster starten
docker-compose -f docker-compose-llm-raid-tests.yml up -d

# 2. Warten bis alle Services healthy sind (60-90 Sekunden)
docker-compose -f docker-compose-llm-raid-tests.yml ps

# 3. Pipeline Tests ausführen
docker-compose -f docker-compose-llm-raid-tests.yml \
  exec themis-llm-raid-tests test-entrypoint.sh all

# 4. Ergebnisse abrufen
docker exec themis-llm-raid-tests ls -lh /test_results/

# 5. Cleanup
docker-compose -f docker-compose-llm-raid-tests.yml down -v
```

## Performance Tuning

Für bessere Benchmark-Ergebnisse:

```powershell
# Docker Desktop mit mehr CPUs konfigurieren
# Settings → Resources → CPU: 8+ Kerne empfohlen

# RAM: 8-16 GB zuweisen
# Settings → Resources → Memory: 8-16 GB
```
