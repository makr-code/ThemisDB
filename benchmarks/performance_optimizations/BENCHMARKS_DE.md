> ⚠️ **Historischer Statusreport** – Dieser Bericht beschreibt den Implementierungsstand zum Zeitpunkt der Erstellung.
> Für den aktuellen Stand: Quellcode und aktuelle [`benchmarks/README.md`](../README.md) prüfen.

# Google Benchmark C++ Performance Tests

Dieses Verzeichnis enthält C++ Google Benchmark-basierte Performance-Tests für alle Phase 1 Optimierungen.

## Anforderungen

- **Google Benchmark** Bibliothek
- **CMake** 3.15+
- **C++20** Compiler
- **GoogleTest** (für Unit Tests)

### Google Benchmark Installieren

```bash
# Via vcpkg (empfohlen)
vcpkg install benchmark

# Oder via apt (Ubuntu/Debian)
sudo apt-get install libbenchmark-dev

# Oder from source bauen
git clone https://github.com/google/benchmark.git
cd benchmark
cmake -B build -S . -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on
cmake --build build
sudo cmake --install build
```

## Benchmarks Bauen

```bash
# Mit Benchmarks konfigurieren
cmake -B build -S . \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_HUGE_PAGES=ON \
  -DTHEMIS_ENABLE_RCU_INDEX=ON

# Bauen
cmake --build build

# Benchmarks befinden sich in: build/benchmarks/
```

## Benchmarks Ausführen

### Alle Benchmarks

```bash
# Mimalloc Allocator Benchmark
./build/benchmarks/benchmark_mimalloc

# Huge Pages Memory Benchmark
./build/benchmarks/benchmark_huge_pages

# RCU Index Benchmark
./build/benchmarks/benchmark_rcu_index
```

### Mit Optionen

```bash
# Für bestimmte Zeit ausführen
./build/benchmarks/benchmark_mimalloc --benchmark_min_time=5.0

# Spezifischen Benchmark ausführen
./build/benchmarks/benchmark_rcu_index --benchmark_filter=ConcurrentReads

# Als JSON ausgeben
./build/benchmarks/benchmark_mimalloc --benchmark_format=json > results.json

# Mit verschiedenen Threads
./build/benchmarks/benchmark_rcu_index --benchmark_threads=8

# Statistiken anzeigen
./build/benchmarks/benchmark_mimalloc --benchmark_repetitions=10
```

## Benchmark-Übersicht

### 1. Mimalloc Allocator (`benchmark_mimalloc.cpp`)

**Testet**: Speicherallokations-Performance

**Benchmarks**:
- `BM_SimpleAllocation` - Einfache Allokation/Deallokation (64B - 16KB)
- `BM_MultipleAllocations` - Mehrfach-Allokationen (10-1000 Objekte)
- `BM_RandomSizeAllocations` - Zufällige Größen (64B - 4KB)
- `BM_AlignedAllocation` - Ausgerichtete Allokationen (64-byte alignment)
- `BM_AllocationWithUsage` - Allokation + Nutzung (memset)
- `BM_ThreadLocalPattern` - Thread-lokale Muster (1-8 Threads)

**Erwartete Verbesserung**: +10-20% vs System-Allocator

**Beispiel-Output**:
```
=================================================================
Mimalloc Allocator Benchmark
=================================================================
Allocator: mimalloc
Mimalloc enabled: YES

Expected improvement: +10-20% vs system allocator
Best performance: Multi-threaded, many small allocations
=================================================================

----------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
----------------------------------------------------------------
BM_SimpleAllocation/64        153 ns          153 ns      4568234
BM_SimpleAllocation/1024      201 ns          201 ns      3483921
BM_MultipleAllocations/100   15.2 µs         15.2 µs       46013
BM_ThreadLocalPattern/threads:4  12.3 µs    49.2 µs       14234
```

### 2. Huge Pages Memory (`benchmark_huge_pages.cpp`)

**Testet**: Speicherzugriff-Performance

**Benchmarks**:
- `BM_SequentialAccess` - Sequentieller Zugriff (4MB - 64MB)
- `BM_RandomAccess` - Zufälliger Zugriff (zeigt TLB-Vorteil)
- `BM_StridedAccess` - Stride-Zugriff (maximiert TLB-Druck)
- `BM_MemoryInitialization` - Speicher-Initialisierung
- `BM_BufferPoolSimulation` - Datenbank Buffer-Pool Simulation

**Erwartete Verbesserung**: +15-30% für memory-intensive Workloads

**Beispiel-Output**:
```
=================================================================
Huge Pages Memory Benchmark
=================================================================
Status: enabled (2MB pages)
Page size: 2097152 bytes

Expected improvement: +15-30% for memory-intensive workloads
TLB miss reduction: Up to 512x (4KB -> 2MB pages)
=================================================================

----------------------------------------------------------------
Benchmark                            Time             CPU   Iterations
----------------------------------------------------------------
BM_SequentialAccess/4MB           2.34 ms         2.34 ms         299
BM_RandomAccess/16MB               423 µs          423 µs        1654
BM_StridedAccess/64MB              156 µs          156 µs        4483
BM_BufferPoolSimulation/threads:4  234 ns          936 ns      748392
```

### 3. RCU Index (`benchmark_rcu_index.cpp`)

**Testet**: Lock-free Read-Performance

**Benchmarks**:
- `BM_SimpleLookup` - Einfache Lookup-Operationen
- `BM_ReadHeavyWorkload` - Read-heavy (95% Reads, 5% Writes)
- `BM_ConcurrentReads` - Gleichzeitige Reads (1-8 Threads)
- `BM_HotKeyPattern` - Hot-Key-Zugriffsmuster (80/20 Pareto)
- `BM_InsertOperations` - Insert-Performance
- `BM_MixedWorkload` - Gemischter Workload (50-99% Reads)
- `BM_GracePeriodOverhead` - RCU Grace Period Overhead

**Erwartete Verbesserung**: +200-500% für read-heavy Workloads (90%+ Reads)

**Beispiel-Output**:
```
=================================================================
RCU Index Benchmark
=================================================================
RCU enabled: YES

Expected improvement: +200-500% for read-heavy workloads
Read ratio required: 90%+ reads for best results
Key feature: ZERO overhead for readers (lock-free!)
Scalability: Linear with CPU cores
=================================================================

----------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
----------------------------------------------------------------
BM_SimpleLookup                 42 ns           42 ns     16734892
BM_ReadHeavyWorkload            45 ns           45 ns     15562341
BM_ConcurrentReads/threads:1    43 ns           43 ns     16289473
BM_ConcurrentReads/threads:4    12 ns           48 ns     58392847
BM_ConcurrentReads/threads:8     7 ns           56 ns     99283746
BM_MixedWorkload/95             46 ns           46 ns     15234829 read_ratio=0.95
BM_MixedWorkload/99             44 ns           44 ns     15892374 read_ratio=0.99
```

## Ergebnisse Interpretieren

### Zeit-Einheiten
- `ns` = Nanosekunden (10^-9 Sekunden)
- `µs` = Mikrosekunden (10^-6 Sekunden)
- `ms` = Millisekunden (10^-3 Sekunden)

### Wichtige Metriken
- **Time**: Wanduhr-Zeit pro Iteration
- **CPU**: CPU-Zeit pro Iteration
- **Iterations**: Anzahl der Benchmark-Durchläufe
- **Bytes/sec**: Durchsatz für Speicher-Operationen
- **Items/sec**: Operationen pro Sekunde

### Erwartete Performance-Gewinne

| Optimierung | Baseline | Mit Optimierung | Verbesserung |
|-------------|----------|-----------------|--------------|
| Mimalloc | 200 ns/op | 160-180 ns/op | +10-20% |
| Huge Pages | 500 µs | 350-425 µs | +15-30% |
| RCU (95% Reads) | 150 ns/op | 30-50 ns/op | +200-400% |

### Warnzeichen

- ⚠️ Verbesserung <5%: Prüfen ob Optimierung aktiviert ist
- ⚠️ Performance verschlechtert sich: Konfiguration und System prüfen
- ⚠️ Hohe Varianz: Mehr Iterationen oder System-Last prüfen

## Konfigurationen Vergleichen

```bash
# Baseline (keine Optimierungen)
cmake -B build-baseline -S . -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build-baseline
./build-baseline/benchmarks/benchmark_mimalloc --benchmark_format=json > baseline.json

# Mit Optimierungen
cmake -B build-opt -S . \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DTHEMIS_ENABLE_MIMALLOC=ON \
  -DTHEMIS_ENABLE_HUGE_PAGES=ON \
  -DTHEMIS_ENABLE_RCU_INDEX=ON
cmake --build build-opt
./build-opt/benchmarks/benchmark_mimalloc --benchmark_format=json > optimized.json

# Vergleichen mit compare.py (Google Benchmark Tool)
python3 compare.py benchmarks baseline.json optimized.json
```

## Unit Tests

Zusätzlich zu Benchmarks gibt es Unit Tests:

```bash
# Alle Performance-Tests
./build/tests/themis_tests --gtest_filter="Performance*"

# Spezifische Tests
./build/tests/themis_tests --gtest_filter="*Allocator*"
./build/tests/themis_tests --gtest_filter="HugePages*"
./build/tests/themis_tests --gtest_filter="RCU*"
```

## CI/CD Integration

### GitHub Actions Beispiel

```yaml
- name: Build mit Benchmarks
  run: |
    cmake -B build -S . \
      -DTHEMIS_BUILD_BENCHMARKS=ON \
      -DTHEMIS_ENABLE_MIMALLOC=ON \
      -DTHEMIS_ENABLE_HUGE_PAGES=ON \
      -DTHEMIS_ENABLE_RCU_INDEX=ON
    cmake --build build

- name: Benchmarks Ausführen
  run: |
    ./build/benchmarks/benchmark_mimalloc --benchmark_format=json > mimalloc.json
    ./build/benchmarks/benchmark_huge_pages --benchmark_format=json > huge_pages.json
    ./build/benchmarks/benchmark_rcu_index --benchmark_format=json > rcu.json

- name: Ergebnisse Hochladen
  uses: actions/upload-artifact@v2
  with:
    name: benchmark-results
    path: '*.json'
```

## Troubleshooting

### Benchmark-Bibliothek nicht gefunden
```
ERROR: Could not find benchmark library
```
**Lösung**: Google Benchmark via vcpkg oder Paketmanager installieren

### Optimierung nicht aktiviert
```
⚠️ Mimalloc NOT enabled - using system allocator
```
**Lösung**: Mit `-DTHEMIS_ENABLE_MIMALLOC=ON` neu bauen

### Huge Pages nicht verfügbar
```
Status: enabled but unavailable (system)
```
**Lösung** (Linux):
```bash
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
```

### Schlechte RCU Performance
```
Erwartet: +200%, Tatsächlich: +50%
```
**Prüfen**:
- Ist Workload wirklich read-heavy (>90% Reads)?
- Ist RCU zur Compile-Zeit aktiviert?
- Werden genug Threads verwendet?

## Dokumentation

Für detaillierte Dokumentation siehe:
- `docs/TESTING_AND_BENCHMARKING_GUIDE.md` - Vollständiger Test-Guide (Deutsch)
- `docs/PHASE1_*_IMPLEMENTATION.md` - Implementierungs-Details pro Optimierung
- [Google Benchmark Dokumentation](https://github.com/google/benchmark/blob/main/docs/user_guide.md)

## Referenzen

- **Google Benchmark**: https://github.com/google/benchmark
- **Benchmark User Guide**: https://github.com/google/benchmark/blob/main/docs/user_guide.md
- **Research Papers**: Siehe individuelle Implementierungs-Docs

---

**Letzte Aktualisierung**: 2025-12-24  
**Status**: Production Ready  
**Abdeckung**: 3 Optimierungen, 20+ Benchmark-Szenarien
