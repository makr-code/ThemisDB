# ThemisDB Docker RAID Comprehensive Benchmark Suite

**Status:** ✅ Production-Ready  
**Created:** January 3, 2026  
**Runtime:** 1+ hour (configurable)  
**File:** `bench_docker_raid_comprehensive.cpp`

## Executive Summary

Umfassende C++ Google Benchmark-Suite für Docker RAID-Performance-Testing in Multi-Container-Umgebungen. Entwickelt nach Best Practices von Google Benchmark und RocksDB, inspiriert von der ThemisDB Python RAID-Suite.

### Hauptmerkmale

- ✅ **10 umfassende Benchmark-Suites** (125-140 min Gesamtlaufzeit)
- ✅ **Alle RAID-Level:** RAID0, RAID1, RAID5, RAID6, RAID10
- ✅ **3-12 Docker Container** pro Test-Konfiguration
- ✅ **Variable Wiederholungen** für statistische Signifikanz
- ✅ **Realistische Workloads:** Small/Medium/Large Documents, Blobs
- ✅ **Failover-Szenarien** mit automatischer Recovery
- ✅ **Concurrent Operations** mit Multi-Threading
- ✅ **JSON/CSV Export** für Analyse

## Benchmark-Suites im Detail

### Suite 1: Small Document Write (5-10 min)
- **Dokumentgröße:** 10 KB
- **Szenarien:** 100-1000 Dokumente pro Batch
- **RAID-Level:** 0, 1, 5, 10
- **Container:** 3-6
- **Min Time:** 10s pro Konfiguration

```cpp
// Beispiel-Konfigurationen:
->Args({3, 0, 100})      // 3 containers, RAID0, 100 docs
->Args({6, 1, 1000})     // 6 containers, RAID1, 1000 docs
```

### Suite 2: Medium Document Write (10-15 min)
- **Dokumentgröße:** 100 KB
- **Szenarien:** 50-500 Dokumente pro Batch
- **Metriken:** Durchsatz (MB/s), Latenz (ms)
- **Min Time:** 15s

### Suite 3: Large Blob Write (15-20 min)
- **Blob-Größe:** 10 MB
- **Szenarien:** 5-20 Blobs
- **Metriken:** Blobs/Sekunde, Gesamtdurchsatz
- **Min Time:** 20s
- **Anwendungsfall:** Video/Bild-Storage

### Suite 4: Random Read (10-15 min)
- **Pre-Population:** 1000 Dokumente
- **Reads:** 100-500 pro Iteration
- **Verteilung:** Uniform Random
- **Min Time:** 12s

### Suite 5: Container Failover & Recovery (10-15 min)
- **Szenario:** Container-Ausfall während Betrieb
- **Messung:** Recovery-Zeit, Daten-Verfügbarkeit
- **RAID-Level:** 1, 5, 6, 10 (redundante Konfigurationen)
- **Iterations:** 10 Failover-Zyklen

```cpp
// Failover-Metriken:
state.counters["recovery_time_ms"] = recovery_ms;
state.counters["failed_containers"] = controller_->getNumFailedContainers();
```

### Suite 6: Concurrent Operations (15-20 min)
- **Threads:** 4-16
- **Operationen:** 50 Writes pro Thread
- **Synchronisation:** Lock-free striping
- **Min Time:** 15s
- **UseRealTime:** Wand-Zeit statt CPU-Zeit

### Suite 7: Mixed Read/Write Workload (15-20 min)
- **Read/Write Ratios:** 90/10, 70/30, 50/50
- **Dokumente:** 1000 pre-populated
- **Operationen:** 100 pro Iteration
- **Realistischer Produktions-Workload**

### Suite 8: Synchronization Latency (10-15 min)
- **Messung:** Replikations-Latenz zwischen Containern
- **Netzwerk-Simulation:** 100 µs lokales Netzwerk
- **RAID-Level:** 1, 5, 6 (mit Redundanz)
- **Metriken:** Durchschnittliche Sync-Latenz (µs)

### Suite 9: Cross-Container Query (10-15 min)
- **Szenario:** Queries über alle Container hinweg
- **Verteilung:** 200 Dokumente pro Container
- **Queries:** 50-100 pro Iteration
- **Scatter-Gather-Pattern**

### Suite 10: Dynamic Rebalancing (15-20 min)
- **Szenario:** Container hinzufügen/entfernen
- **Initial Load:** 500 Dokumente
- **Rebalancing:** 10% der Daten
- **Iterations:** 5 Rebalancing-Zyklen
- **Metriken:** Rebalancing-Zeit (ms)

## Verwendung

### Kompilierung

```bash
# Mit CMake
cd build-msvc
cmake --build . --target bench_docker_raid_comprehensive --config Release

# Oder direkt mit g++/clang++
g++ -std=c++17 -O3 -DNDEBUG \
    bench_docker_raid_comprehensive.cpp \
    -lbenchmark -lpthread \
    -o bench_docker_raid_comprehensive
```

### Ausführung

> **Build:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`


#### Standard-Run (1+ Stunde)
```bash
./bench_docker_raid_comprehensive
```

#### Extended Run (2+ Stunden mit Wiederholungen)
```bash
./bench_docker_raid_comprehensive \
    --benchmark_min_time=120 \
    --benchmark_repetitions=3 \
    --benchmark_report_aggregates_only=true \
    --benchmark_out=raid_results_2h.json \
    --benchmark_out_format=json
```

#### Nur bestimmte RAID-Level testen
```bash
# Nur RAID1
./bench_docker_raid_comprehensive \
    --benchmark_filter=".*RAID1.*"

# Nur Failover-Tests
./bench_docker_raid_comprehensive \
    --benchmark_filter=".*Failover.*"

# Nur Small Documents
./bench_docker_raid_comprehensive \
    --benchmark_filter="SmallDocument.*"
```

#### Schneller Smoke-Test
```bash
./bench_docker_raid_comprehensive \
    --benchmark_min_time=2.0 \
    --benchmark_filter="SmallDocumentWrite.*RAID0"
```

### Ergebnis-Analyse

#### JSON-Export
```bash
./bench_docker_raid_comprehensive \
    --benchmark_out=results.json \
    --benchmark_out_format=json
```

#### CSV-Export (Excel-kompatibel)
```bash
./bench_docker_raid_comprehensive \
    --benchmark_out=results.csv \
    --benchmark_out_format=csv
```

#### Console-Output mit Aggregaten
```bash
./bench_docker_raid_comprehensive \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true
```

## Konfiguration

### Runtime-Parameter

| Parameter | Beschreibung | Standard | Empfehlung für 1h+ |
|-----------|--------------|----------|-------------------|
| `--benchmark_min_time` | Min. Zeit pro Benchmark (Sekunden) | variabel | 60-120 |
| `--benchmark_repetitions` | Anzahl Wiederholungen | 1 | 3-5 |
| `--benchmark_filter` | Regex-Filter | alle | siehe Beispiele |
| `--benchmark_enable_random_interleaving` | Zufällige Reihenfolge | false | true |

### Environment Variables

```bash
# Docker-Konfiguration
export DOCKER_HOST=unix:///var/run/docker.sock
export THEMIS_RAID_IMAGE=themisdb:latest
export THEMIS_NETWORK_NAME=themis_raid_network

# Performance-Tuning
export BENCHMARK_NUM_CONTAINERS=6
export BENCHMARK_MAX_THREADS=16
```

## Metriken & Counters

### Automatische Metriken
- `iterations` - Anzahl der Iterationen
- `real_time` - Wand-Zeit (Sekunden)
- `cpu_time` - CPU-Zeit (Sekunden)
- `time_unit` - Zeiteinheit (ms, µs, s)

### Custom Counters

```cpp
// Durchsatz
state.counters["throughput_mbps"] = bytes_processed / time_seconds / 1e6;

// Container-Statistiken
state.counters["containers"] = num_containers;
state.counters["raid_level"] = static_cast<double>(raid_level);

// Failover-Metriken
state.counters["recovery_time_ms"] = recovery_milliseconds;
state.counters["data_loss_bytes"] = bytes_lost;

// Concurrency
state.counters["concurrency"] = num_threads;
state.counters["ops_per_sec"] = ops / time_seconds;
```

## Beispiel-Output

### Console
```
------------------------------------------------------------------------
Benchmark                              Time       CPU   Iterations
------------------------------------------------------------------------
SmallDocumentWrite/3/0/100          2.45 ms   2.43 ms        286
  containers                             3
  raid_level                             0
  total_bytes_written              2.86e+08

MediumDocumentWrite/4/5/500        124 ms    123 ms          6
  throughput_mbps                     405.2

ContainerFailover/3/1/0             89.2 ms   88.1 ms        8
  recovery_time_ms                     156
  
ConcurrentWrites/6/10/8            45.3 ms   362 ms         15
  concurrency                           8
  ops_per_sec                        8832.5
```

### JSON (Auszug)
```json
{
  "context": {
    "date": "2026-01-03 10:30:00",
    "num_cpus": 16,
    "mhz_per_cpu": 3600,
    "cpu_scaling_enabled": false,
    "caches": []
  },
  "benchmarks": [
    {
      "name": "SmallDocumentWrite/3/0/100",
      "run_type": "iteration",
      "iterations": 286,
      "real_time": 2.45,
      "cpu_time": 2.43,
      "time_unit": "ms",
      "containers": 3,
      "raid_level": 0,
      "total_bytes_written": 286000000
    }
  ]
}
```

## Performance-Erwartungen

### RAID0 (Striping, maximale Performance)
- **Write Throughput:** 800-1200 MB/s (bei 6 Containern)
- **Read Throughput:** 1000-1500 MB/s
- **Latenz:** <5 ms (kleine Dokumente)
- **Skalierung:** Linear mit Container-Anzahl

### RAID1 (Mirroring)
- **Write Throughput:** 400-600 MB/s (halbe von RAID0)
- **Read Throughput:** 1200-1800 MB/s (parallelisiert)
- **Latenz:** 5-10 ms (Sync-Overhead)
- **Redundanz:** 100%

### RAID5 (Parity)
- **Write Throughput:** 500-700 MB/s
- **Read Throughput:** 800-1200 MB/s
- **Latenz:** 8-15 ms (Parity-Berechnung)
- **Kapazität:** (n-1)/n

### RAID6 (Dual Parity)
- **Write Throughput:** 400-550 MB/s
- **Read Throughput:** 700-1000 MB/s
- **Latenz:** 10-20 ms
- **Kapazität:** (n-2)/n
- **Fault Tolerance:** 2 Container-Ausfälle

### RAID10 (Striped Mirrors)
- **Write Throughput:** 600-900 MB/s
- **Read Throughput:** 1100-1600 MB/s
- **Latenz:** 6-12 ms
- **Balance:** Beste Kombination aus Performance und Redundanz

## Vergleich mit Python-Suite

| Feature | Python Suite | C++ Suite | Vorteil |
|---------|--------------|-----------|---------|
| **Performance** | Interpreter-Overhead | Native Code | C++ 10-50x schneller |
| **Präzision** | µs-Genauigkeit | ns-Genauigkeit | C++ |
| **Integration** | Separate Scripts | Google Benchmark | C++ |
| **Statistik** | Manuell | Automatisch (Google) | C++ |
| **Docker Control** | Python SDK | System Calls | Python (einfacher) |
| **Test Complexity** | 881 Zeilen | 1300 Zeilen | Ähnlich |

## Troubleshooting

### Problem: Docker-Container starten nicht
```bash
# Prüfen ob Docker läuft
docker ps

# Prüfen ob Image existiert
docker images | grep themisdb

# Container manuell starten (Test)
docker run -d --name test_raid themisdb:latest
```

### Problem: Benchmark zu langsam
```bash
# Reduziere Min-Time
--benchmark_min_time=5.0

# Filtere Benchmarks
--benchmark_filter="SmallDocument.*"

# Deaktiviere bestimmte RAID-Level
--benchmark_filter="^((?!RAID6).)*$"  # Ohne RAID6
```

### Problem: Out of Memory
```bash
# Reduziere Container-Anzahl
# Ändere in Code: MAX_CONTAINERS = 6

# Reduziere Datengrößen
# Ändere in Code: LARGE_DOCUMENT_SIZE = 512 * KB

# Reduziere Concurrent Threads
--benchmark_filter=".*ConcurrentWrites.*/[1-4]$"
```

## Erweiterungen

### Eigene Benchmarks hinzufügen

```cpp
BENCHMARK_DEFINE_F(DockerRAIDBenchmarkBase, MyCustomBenchmark)
(benchmark::State& state) {
    // Setup
    int my_param = state.range(2);
    
    for (auto _ : state) {
        // Zu messender Code
        auto data = TestDataGenerator::generateData(1024, 0);
        controller_->writeData(data, 0);
    }
    
    // Metriken
    state.SetItemsProcessed(state.iterations());
    state.counters["my_metric"] = calculate_metric();
}

BENCHMARK_REGISTER_F(DockerRAIDBenchmarkBase, MyCustomBenchmark)
    ->Args({3, 0, 100})
    ->MinTime(10.0)
    ->Unit(benchmark::kMillisecond);
```

### Netzwerk-Latenz-Simulation anpassen

```cpp
// In readRAID1() oder ähnlichen Funktionen:
std::this_thread::sleep_for(
    std::chrono::microseconds(CROSS_DC_LATENCY_US));  // 50ms
```

### RAID-Level hinzufügen (z.B. RAID50)

```cpp
enum class RAIDLevel {
    // ... existing
    RAID50 = 50,  // RAID5 + RAID0
};

// Implementiere writeRAID50() und readRAID50()
```

## Best Practices

1. **Lange Runs verwenden:** `--benchmark_min_time=60+` für stabile Ergebnisse
2. **Wiederholungen:** `--benchmark_repetitions=3-5` für Statistik
3. **Filtern:** Nur relevante Tests ausführen mit `--benchmark_filter`
4. **Exportieren:** Immer JSON/CSV exportieren für spätere Analyse
5. **System-Load:** Tests auf dedizierter Hardware ohne andere Last
6. **Docker Resources:** Ausreichend RAM/CPU für Container bereitstellen

## Referenzen

- [Google Benchmark Documentation](https://github.com/google/benchmark)
- [RocksDB Benchmark Examples](https://github.com/facebook/rocksdb/tools)
- [ThemisDB Python RAID Suite](../../benchmarks/raid_sharding_test_suite.py)
- [ThemisDB Sharding Benchmarks](../bench_sharding_performance.cpp)

## Lizenz

Copyright © 2026 ThemisDB Team  
Distributed under Apache License 2.0

## Support

Bei Fragen oder Issues:
- GitHub: https://github.com/themisdb/themis
- Dokumentation: https://docs.themisdb.org/benchmarks
- Email: benchmarks@themisdb.org
