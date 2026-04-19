# Performance Optimization Testing & Benchmarking Guide

## Übersicht

ThemisDB verfügt über eine umfassende Test- und Benchmark-Abdeckung:

| Kategorie | Dateien | Funktionen | Beschreibung |
|-----------|---------|-----------|-------------|
| **C++ Benchmarks** | **122** | **1.108** | Google Benchmark – in `benchmarks/` |
| **C++ Tests** | **732** | **10.436** | Google Test – in `tests/` |
| **Go Client Tests** | **4** | **40** | Unit/Integration – in `clients/go/` |
| **Go Client Benchmarks** | **2** | **25** | Benchmarks – in `clients/go/` |
| **Python Tools/Scripts** | **9+** | – | Tools, Analyse-Skripte |

Vollständige Inventur: [TEST_AND_BENCHMARK_INVENTORY.md](ARCHIVED/implementation-summaries/TEST_AND_BENCHMARK_INVENTORY.md)

---

### C++ Benchmark-Kategorien

| Kategorie | Benchmarks (Auswahl) |
|-----------|---------------------|
| Core CRUD & Storage | `bench_crud.cpp`, `bench_insert_profiling.cpp`, `bench_storage_performance.cpp` |
| OLAP & Analytics | `bench_olap_performance.cpp`, `bench_tpcc.cpp`, `bench_tpch.cpp`, `bench_ycsb.cpp` |
| LLM & KI | `bench_llm_infrastructure.cpp`, `bench_llm_inference_performance.cpp`, `bench_embedded_llm.cpp` |
| LoRA & Training | `bench_lora_inline.cpp`, `bench_qlora_gpu_kernels.cpp`, `bench_multi_gpu_lora_advanced.cpp` |
| GPU & Beschleunigung | `bench_gpu_backends.cpp`, `bench_fused_kernels.cpp`, `bench_flash_attention.cpp` |
| Vektor-Suche | `bench_mmdb.cpp`, `bench_vector_search.cpp`, `bench_mixed_precision_perf.cpp` |
| PostgreSQL Wire | `bench_postgres_e2e.cpp`, `bench_postgres_transactions.cpp` |
| Graph | `bench_graph_traversal.cpp`, `bench_pagerank.cpp` |
| Sharding | `bench_shard_routing.cpp`, `bench_sharding_performance.cpp` |
| Sicherheit | `bench_encryption.cpp`, `bench_ethics_ai_plugin.cpp` |
| Content & Medien | `bench_text_extraction.cpp`, `bench_image_analysis.cpp`, `bench_video_processor.cpp` |

### Go Client Tests (`clients/go/`)

| Datei | Typ | Beschreibung |
|-------|-----|-------------|
| `client_test.go` | Unit Test | Client-Basis-Funktionalität |
| `wire_protocol_test.go` | Unit Test | Wire Protocol |
| `tls_config_test.go` | Unit Test | TLS-Konfiguration |
| `client_rest_test.go` | Integration Test | REST API |
| `client_bench_test.go` | Benchmark | Client-Performance |
| `wire_protocol_bench_test.go` | Benchmark | Wire-Protocol-Performance |

```bash
# Go Tests ausführen
cd clients/go
go test ./...
go test -bench=. -benchmem ./...
```

### Python-Werkzeuge

| Datei | Pfad | Zweck |
|-------|------|-------|
| `statistics.py` | `benchmarks/chimera/` | Statistische Analyse |
| `reporter.py` | `benchmarks/chimera/` | CHIMERA-Reporting |
| `ingest.py` | `tools/` | Data Ingestion |
| `train_failure_model.py` | `scripts/` | Failure Prediction ML |
| `link-check.py` | `scripts/` | Link-Validierung |

---

### Phase 1 Performance-Optimierungen

Dieses Dokument beschreibt außerdem die Tests und Benchmarks für die Phase 1 Performance-Optimierungen:
- **Mimalloc**: Schneller Speicher-Allocator (+10-20%)
- **Huge Pages**: Große Speicherseiten (+15-30%)
- **RCU Index**: Lock-free Lesezugriffe (+200-500%)

---

## 1. Funktionstests (Unit Tests)

### 1.1 Test-Dateien

| Test-Datei | Optimierung | Test-Fälle | Beschreibung |
|------------|-------------|------------|--------------|
| `tests/test_performance_feature_flags.cpp` | Infrastructure | 8 | Feature-Flag-System |
| `tests/test_performance_allocator.cpp` | Mimalloc | 7 | Speicherallokation |
| `tests/test_huge_pages.cpp` | Huge Pages | 9 | Huge Page-Allokation |
| `tests/test_rcu_index.cpp` | RCU Index | 12 | RCU und Lock-free Hash |

**Gesamt**: 36 Unit Tests

### 1.2 Tests Ausführen

```bash
# Alle Tests
./build/tests/themis_tests

# Nur Performance-Tests
./build/tests/themis_tests --gtest_filter="Performance*"

# Spezifische Optimierung
./build/tests/themis_tests --gtest_filter="RCU*"
./build/tests/themis_tests --gtest_filter="*Allocator*"
./build/tests/themis_tests --gtest_filter="HugePages*"
```

### 1.3 Erwartete Ausgabe

```
[==========] Running 36 tests from 4 test suites.
[----------] 8 tests from PerformanceFeatureFlagsTest
[ RUN      ] PerformanceFeatureFlagsTest.Initialization
[       OK ] PerformanceFeatureFlagsTest.Initialization
...
[----------] 8 tests from PerformanceFeatureFlagsTest (45 ms total)

[----------] 7 tests from PerformanceAllocatorTest
...
[----------] 7 tests from PerformanceAllocatorTest (156 ms total)

[----------] 9 tests from HugePagesTest
...
[----------] 9 tests from HugePagesTest (234 ms total)

[----------] 12 tests from RCUTest and RCUHashTableTest
...
[----------] 12 tests from RCUTest (1832 ms total)

[==========] 36 tests from 4 test suites ran. (2267 ms total)
[  PASSED  ] 36 tests.
```

---

## 2. Performance-Benchmarks

### 2.1 Benchmark-Skripte

| Benchmark | Datei | Messung | Ziel |
|-----------|-------|---------|------|
| Mimalloc | `benchmark_mimalloc.py` | Allokations-Performance | +10-20% |
| Huge Pages | `benchmark_huge_pages.py` | Memory-Access | +15-30% |
| RCU Index | `benchmark_rcu_index.py` | Read-Throughput | +200-500% |

### 2.2 Einzelne Benchmarks Ausführen

#### Mimalloc Benchmark

```bash
cd benchmarks/performance_optimizations

python3 benchmark_mimalloc.py \
  --iterations 10 \
  --min-improvement 10
```

**Erwartete Ausgabe:**
```
MIMALLOC ALLOCATOR BENCHMARK
============================================================

Phase 1: Baseline (no mimalloc)
Running allocation benchmark: mimalloc=OFF
Iterations: 10

Iteration 1/10... 102.34µs
Iteration 2/10... 98.76µs
...

Phase 2: Optimized (with mimalloc)
Running allocation benchmark: mimalloc=ON
Iterations: 10

Iteration 1/10... 85.23µs
Iteration 2/10... 82.45µs
...

Phase 3: Validation
============================================================
MIMALLOC BENCHMARK RESULTS
============================================================

Baseline (no mimalloc):
  Mean: 100.00µs
  StdDev: 2.45µs
  Range: 96.00 - 103.00µs

Optimized (with mimalloc):
  Mean: 83.50µs
  StdDev: 2.12µs
  Range: 80.00 - 87.00µs

Performance Improvement: +16.50%
Required Minimum: 10.00%

✅ VALIDATION PASSED
   Improvement 16.50% >= 10.00%
============================================================
```

#### Huge Pages Benchmark

```bash
python3 benchmark_huge_pages.py \
  --iterations 10 \
  --min-improvement 15
```

**Erwartete Ausgabe:**
```
HUGE PAGES MEMORY BENCHMARK
============================================================

Phase 1: Baseline (standard 4KB pages)
Running memory benchmark: huge_pages=OFF
Iterations: 10

Iteration 1/10... 205.67µs
Iteration 2/10... 198.23µs
...

Phase 2: Optimized (2MB huge pages)
Running memory benchmark: huge_pages=ON
Iterations: 10

Iteration 1/10... 160.45µs
Iteration 2/10... 155.78µs
...

Phase 3: Validation
============================================================
HUGE PAGES BENCHMARK RESULTS
============================================================

Baseline (4KB pages):
  Mean: 200.00µs
  StdDev: 4.23µs
  Range: 195.00 - 208.00µs
  TLB Misses: HIGH (many 4KB pages)

Optimized (2MB huge pages):
  Mean: 158.00µs
  StdDev: 3.45µs
  Range: 153.00 - 164.00µs
  TLB Misses: LOW (fewer 2MB pages)

Performance Improvement: +21.00%
Required Minimum: 15.00%

✅ VALIDATION PASSED
   Improvement 21.00% >= 15.00%
   TLB miss reduction confirmed!
============================================================
```

#### RCU Index Benchmark

```bash
python3 benchmark_rcu_index.py \
  --iterations 10 \
  --min-improvement 200 \
  --read-ratio 0.95
```

**Erwartete Ausgabe:**
```
RCU INDEX BENCHMARK
Read-heavy workload optimization
============================================================

Phase 1: Baseline (traditional locking)
Running RCU index benchmark: rcu=OFF
Read ratio: 95%
Iterations: 10

Iteration 1/10... 152.34µs
Iteration 2/10... 148.76µs
...

Phase 2: Optimized (RCU lock-free reads)
Running RCU index benchmark: rcu=ON
Read ratio: 95%
Iterations: 10

Iteration 1/10... 42.34µs
Iteration 2/10... 40.87µs
...

Phase 3: Validation
============================================================
RCU INDEX BENCHMARK RESULTS
============================================================

Workload: 95% reads, 5% writes

Baseline (with locks):
  Mean: 150.00µs
  StdDev: 3.45µs
  Range: 145.00 - 155.00µs
  Lock overhead: YES (all operations)

Optimized (with RCU):
  Mean: 41.50µs
  StdDev: 1.89µs
  Range: 39.00 - 44.00µs
  Lock overhead: NO (lock-free reads!)

Performance Improvement: +261.45%
Throughput Multiplier: 3.61x
Required Minimum: 200.00%

✅ VALIDATION PASSED
   Improvement 261.45% >= 200.00%
   RCU delivers 3.61x throughput!
============================================================
```

### 2.3 Alle Benchmarks Ausführen

```bash
cd benchmarks/performance_optimizations

python3 run_all_validations.py --iterations 10
```

**Erwartete Ausgabe:**
```
######################################################################
# PERFORMANCE OPTIMIZATION VALIDATION
# Testing Phase 1: Mimalloc + Huge Pages + RCU Index
######################################################################

######################################################################
# PHASE 1: UNIT TESTS
######################################################################

======================================================================
Running: Unit Tests
Command: ./build/tests/themis_tests
======================================================================

[==========] Running 36 tests from 4 test suites.
...
[  PASSED  ] 36 tests.

✅ Unit Tests: PASSED

######################################################################
# PHASE 2: PERFORMANCE BENCHMARKS
######################################################################

======================================================================
Running: Mimalloc Benchmark
Command: python3 benchmarks/.../benchmark_mimalloc.py --iterations 10 --min-improvement 10
======================================================================

... [Mimalloc benchmark output] ...

✅ Mimalloc Benchmark: PASSED

======================================================================
Running: Huge Pages Benchmark
Command: python3 benchmarks/.../benchmark_huge_pages.py --iterations 10 --min-improvement 15
======================================================================

... [Huge Pages benchmark output] ...

✅ Huge Pages Benchmark: PASSED

======================================================================
Running: RCU Index Benchmark
Command: python3 benchmarks/.../benchmark_rcu_index.py --iterations 10 --min-improvement 200 --read-ratio 0.95
======================================================================

... [RCU benchmark output] ...

✅ RCU Index Benchmark: PASSED

######################################################################
# VALIDATION SUMMARY
######################################################################

Unit Tests: ✅ PASSED

Performance Benchmarks:
  Mimalloc: ✅ PASSED
  Huge Pages: ✅ PASSED
  Rcu Index: ✅ PASSED

======================================================================
✅ ALL VALIDATIONS PASSED
   Phase 1 optimizations are working as expected!
======================================================================
```

---

## 3. Validierungs-Checkliste

### 3.1 Vor der Validierung

- [ ] Build-Umgebung aufgesetzt
- [ ] CMake konfiguriert mit gewünschten Flags
- [ ] Projekt erfolgreich gebaut
- [ ] Huge Pages auf dem System konfiguriert (optional)
- [ ] Mimalloc via vcpkg installiert (optional)

### 3.2 Validierungs-Schritte

1. **Unit Tests ausführen**
   ```bash
   ./build/tests/themis_tests
   ```
   - Alle 36 Tests sollten bestehen
   - Keine Segfaults oder Crashes

2. **Mimalloc Benchmark**
   ```bash
   python3 benchmark_mimalloc.py --iterations 10
   ```
   - Erwartete Verbesserung: ≥10%
   - Tatsächliche Verbesserung: 10-20%

3. **Huge Pages Benchmark**
   ```bash
   python3 benchmark_huge_pages.py --iterations 10
   ```
   - Erwartete Verbesserung: ≥15%
   - Tatsächliche Verbesserung: 15-30%
   - Hinweis: System muss Huge Pages unterstützen

4. **RCU Index Benchmark**
   ```bash
   python3 benchmark_rcu_index.py --iterations 10 --read-ratio 0.95
   ```
   - Erwartete Verbesserung: ≥200%
   - Tatsächliche Verbesserung: 200-500%
   - Am besten bei 90%+ Reads

5. **Alle Validierungen zusammen**
   ```bash
   python3 run_all_validations.py --iterations 10
   ```
   - Alle Tests und Benchmarks in einem Durchlauf

### 3.3 Nach der Validierung

- [ ] Alle Unit Tests bestanden
- [ ] Alle Benchmarks zeigen erwartete Verbesserungen
- [ ] Keine Performance-Regressionen
- [ ] Dokumentation aktualisiert
- [ ] Ergebnisse in Status-Report dokumentiert

---

## 4. Troubleshooting

### 4.1 Unit Tests Schlagen Fehl

**Problem**: Tests kompilieren nicht
```bash
# Lösung: C++20 Standard sicherstellen
cmake -B build -S . -DCMAKE_CXX_STANDARD=20
cmake --build build
```

**Problem**: Huge Pages Tests werden übersprungen
```
GTEST_SKIP() << "Huge pages not available on this system"
```
```bash
# Lösung: Huge Pages konfigurieren (Linux)
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
```

### 4.2 Benchmarks Zeigen Keine Verbesserung

**Mimalloc zeigt <10% Verbesserung**
- Mimalloc möglicherweise nicht installiert
- Feature Flag nicht aktiviert beim Build
```bash
cmake -B build -S . -DTHEMIS_ENABLE_MIMALLOC=ON
```

**Huge Pages zeigen <15% Verbesserung**
- Huge Pages nicht auf System konfiguriert
- System unterstützt keine Huge Pages
```bash
# Check Huge Pages Status
cat /proc/meminfo | grep Huge
```

**RCU Index zeigt <200% Verbesserung**
- Read Ratio zu niedrig (sollte >90% sein)
- RCU Feature Flag nicht aktiviert
```bash
cmake -B build -S . -DTHEMIS_ENABLE_RCU_INDEX=ON
```

### 4.3 Python-Skripte Funktionieren Nicht

**Problem**: `python3: command not found`
```bash
# Lösung: Python 3 installieren
sudo apt-get install python3  # Ubuntu/Debian
```

**Problem**: Import-Fehler
```bash
# Lösung: Arbeitsverzeichnis prüfen
cd benchmarks/performance_optimizations
python3 benchmark_mimalloc.py
```

---

## 5. CI/CD Integration

### 5.1 GitHub Actions Workflow

```yaml
name: Performance Validation

on: [push, pull_request]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Install Dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake g++ python3
          
      - name: Configure Huge Pages
        run: |
          echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
          
      - name: Build
        run: |
          cmake -B build -S . \
            -DTHEMIS_ENABLE_MIMALLOC=ON \
            -DTHEMIS_ENABLE_HUGE_PAGES=ON \
            -DTHEMIS_ENABLE_RCU_INDEX=ON
          cmake --build build
          
      - name: Run Validations
        run: |
          cd benchmarks/performance_optimizations
          python3 run_all_validations.py --iterations 5
```

### 5.2 Lokales Pre-Commit Hook

```bash
# .git/hooks/pre-commit
#!/bin/bash

echo "Running performance validation..."
cd benchmarks/performance_optimizations
python3 run_all_validations.py --iterations 3

if [ $? -ne 0 ]; then
    echo "❌ Performance validation failed!"
    exit 1
fi

echo "✅ Performance validation passed!"
```

---

## 6. Reporting

### 6.1 Ergebnis-Format

Alle Benchmarks erzeugen strukturierte Ausgaben mit:
- **Baseline-Metriken**: Mean, StdDev, Min, Max
- **Optimierte Metriken**: Mean, StdDev, Min, Max
- **Verbesserung**: Prozent und Absolut
- **Status**: PASSED/FAILED mit Begründung

### 6.2 Beispiel-Report

```
Performance Optimization Validation Report
Date: 2025-12-24
System: Ubuntu 22.04, 16-core AMD EPYC, 64GB RAM

Phase 1 Results:

1. Mimalloc Allocator:
   - Baseline: 100.0µs ± 2.5µs
   - Optimized: 83.5µs ± 2.1µs
   - Improvement: +16.5% ✅
   - Status: PASSED (target: ≥10%)

2. Huge Pages:
   - Baseline: 200.0µs ± 4.2µs (4KB pages)
   - Optimized: 158.0µs ± 3.5µs (2MB pages)
   - Improvement: +21.0% ✅
   - Status: PASSED (target: ≥15%)

3. RCU Index (95% reads):
   - Baseline: 150.0µs ± 3.5µs (locks)
   - Optimized: 41.5µs ± 1.9µs (lock-free)
   - Improvement: +261.5% ✅
   - Throughput: 3.6x
   - Status: PASSED (target: ≥200%)

Overall: ✅ ALL VALIDATIONS PASSED
Combined Expected Gain: +25-50% memory + 3-5x reads
```

---

## 7. Nächste Schritte

Nach erfolgreicher Validierung:

1. **Staging-Deployment**
   - Optimierungen in Staging-Umgebung aktivieren
   - Real-World-Workloads testen
   - Performance-Metriken sammeln

2. **Production-Rollout**
   - Gradual Rollout (10% → 50% → 100%)
   - A/B Testing mit Feature Flags
   - Monitoring und Alerting

3. **Phase 2 Vorbereitung**
   - LIRS Cache Implementierung
   - Weitere Optimierungen planen
   - Lessons Learned dokumentieren

---

**Letzte Aktualisierung**: 2025-12-24  
**Version**: 1.0  
**Status**: Production Ready
