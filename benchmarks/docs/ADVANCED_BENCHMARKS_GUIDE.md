> **Aktueller Build-Flow:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`

# 🚀 Advanced Benchmarks Suite - OOP & Best-Practices Edition

## Executive Summary

Umfassende Benchmark-Suite nach OOP-Prinzipien und Google Benchmark Best-Practices, die spezialisierte Tests für:
- **Read/Write Ratio Patterns** (100R, 80R/20W, 50R/50W, 20R/80W, 100W)
- **Parallelität & Skalierung** (1, 4, 8, 16, 32 Threads)
- **Self-Protection Mechanismen** (Sustained Load, Burst Load, Connection Exhaustion)
- **Best-Practice vs Anti-Pattern Vergleiche**
- **Gap-Analyse gegen Dokumentation & Internetstandards**

---

## Architecture

### OOP Design Patterns

```
DatabaseFixture (RAII Pattern)
├── Automatic lifecycle management
├── Path generation & cleanup
└── RocksDBWrapper encapsulation

RandomGenerator (Singleton + Thread-Local)
├── Deterministic random generation
├── reusable across all benchmarks
└── Type-safe APIs

ParallelExecutor (Template-based)
├── Generic work distribution
├── Flexible thread scaling
└── Composable with any work

Benchmark Fixtures (Inheritance Hierarchy)
├── ReadWriteRatioBench (base + helpers)
├── ParallelityBench (concurrency testing)
├── SelfProtectionBench (resilience)
├── BestPracticeBench (pattern comparison)
└── GapAnalysisBench (standard validation)
```

### Best-Practices Implemented

✅ **RAII for Resource Management**
```cpp
class DatabaseFixture {
    explicit DatabaseFixture(const std::string& name);
    ~DatabaseFixture();  // Automatic cleanup
};
```

✅ **Fixture Hierarchy with Inheritance**
```cpp
class ReadWriteRatioBench : public benchmark::Fixture {
    void populateDataset(int count);
    void performRead(int entity_id);
    void performWrite(int entity_id);
};
```

✅ **Template Metaprogramming**
```cpp
template<typename Callable>
void execute(Callable&& work, int iterations_per_thread);
```

✅ **Thread-Local State**
```cpp
static RandomGenerator& instance() {
    static thread_local RandomGenerator gen;
    return gen;
}
```

✅ **Custom Counters**
```cpp
state.counters["write_ratio"] = benchmark::Counter(
    80.0, benchmark::Counter::kAvgIterations
);
```

---

## Benchmark Categories

### 1. Read/Write Ratio Tests (5 Tests)

| Test | Ratio | Zweck |
|------|-------|-------|
| `WriteHeavy_80W_20R` | 80% Write | DB unter Schreiblast |
| `Balanced_50W_50R` | 50% Write | Normaler OLTP |
| `ReadHeavy_20W_80R` | 20% Write | Reporting Workload |
| `ReadOnly_0W_100R` | 0% Write | Pure Analytics |
| `WriteOnly_100W_0R` | 100% Write | Ingest Pipeline |

**Gap-Analyse Ziele:**
- RocksDB kann ~1M writes/sec (documented) → Welche Ratios sind optimal?
- Read-heavy sollte 5-10x schneller sein → Validierung?
- Write-only vs Balanced - Lock-Contention-Overhead?

### 2. Parallelity Tests (5 Tests)

Scalability von 1 → 4 → 8 → 16 → 32 Threads

| Threads | Erwartung | Test |
|---------|-----------|------|
| 1 | Baseline | `ParallelInserts_1Thread` |
| 4 | 3-3.5x speedup | `ParallelInserts_4Threads` |
| 8 | 6-7x speedup | `ParallelInserts_8Threads` |
| 16 | 12-14x speedup | `ParallelInserts_16Threads` |
| 32 | 24-28x speedup* | `ParallelInserts_32Threads` |

*Sublinear bei Hardware-Limits

**Gap-Analyse:**
- Ideale Parallelität = CPU-cores. 8-core System → 8 sollte optimal sein
- 16+ Threads → Context-switch Overhead
- Lock contention Pattern erkennbar?

### 3. Self-Protection Tests (5 Tests)

| Test | Szenario | Was wird gemessen |
|------|----------|-------------------|
| `SustainedLoad_70W_30R` | Konstante hohe Last | Throughput-Stabilität |
| `BurstLoad_NormalThen10xSpike` | Plötzliche Last-Spitze | Recovery-Verhalten |
| `ConcurrentConnections_32Threads` | 32 gleichzeitige Verbindungen | Connection Pooling Limits |
| `MemoryPressure_100KB_Documents` | Große Dokumente | Memory Management |

**Gap-Analyse:**
- Welche Burst-Größe bricht die DB?
- Memory-Cleanup nach Pressure?
- Connection-Limits erreicht?

### 4. Best-Practice Comparison (3 Tests)

| Anti-Pattern | Best-Practice | Speedup-Erwartung |
|--------------|---------------|------------------|
| `AntiPattern_NewIndex_PerOperation` | `BestPractice_ReuseIndex_Manager` | 10-100x |
| Individual Puts | `BestPractice_Batch_1000Items` | 2-5x (mit true batch) |

**Gap-Analyse:**
- Realistische Performance Gaps zwischen Patterns?
- Dokumentation warnt davor?

### 5. Gap-Analysis Tests (7 Tests)

| Test | Dokumentiert | Actual | Gap |
|------|-------------|--------|-----|
| `Gap_RocksDB_SequentialWrites_vs_Expected` | 1M /sec | ? | ? |
| `Gap_RandomVsSequential_AccessPattern` | 50% throughput | ? | ? |
| `Gap_ConcurrencyScaling_8Threads` | Linear | ? | ? |
| `Gap_TransactionOverhead_MultiOp` | <5% | ? | ? |
| `Gap_IndexCreation_NewIndexCost` | O(1) | ? | ? |

---

## OOP Design Decisions

### 1. DatabaseFixture RAII Pattern
**Warum:** Vermeidet manuelles Cleanup, prevents resource leaks
```cpp
~DatabaseFixture() {
    db_.reset();
    fs::remove_all(db_path_);  // Automatic cleanup
}
```

### 2. RandomGenerator Singleton + Thread-Local
**Warum:** Thread-safe, deterministic, keine Locks
```cpp
static thread_local RandomGenerator gen;  // Per-thread state
```

### 3. ParallelExecutor Template
**Warum:** Generisch, composable, kein Copy-Paste
```cpp
template<typename Callable>
void execute(Callable&& work, int iterations_per_thread);
```

### 4. Fixture Inheritance Hierarchy
**Warum:** Code-Reuse, consistent setup/teardown

```
benchmark::Fixture
├── ReadWriteRatioBench (base + helpers)
├── ParallelityBench (concurrent ops)
├── SelfProtectionBench (stress testing)
├── BestPracticeBench (patterns)
└── GapAnalysisBench (standards)
```

### 5. Custom State Counters
**Warum:** Metadata für bessere Auswertung
```cpp
state.counters["write_ratio"] = benchmark::Counter(80.0);
```

---

## Google Benchmark Best-Practices Applied

✅ **Fixture-based Tests**
- Proper SetUp/TearDown isolation
- Per-test database instances

✅ **Iteration Scaling**
- Auto-scaling durch Google Benchmark
- Benchmark library determines run count

✅ **DoNotOptimize Calls**
- Compiler kann Benchmarks nicht optimieren
- Realistische Messungen

✅ **Counter Usage**
- Custom metrics neben throughput
- JSON export für CI/CD

✅ **Multiple Runs**
- Statistics: mean, median, std-dev
- Outlier detection

✅ **JSON Export Format**
- Maschinenlesbar für Dashboards
- CI/CD Integration ready

---

## Compilation & Execution

### Build
```bash
cd C:\VCC\themis\build-msvc
cmake --build . --target bench_advanced_patterns --config Release
```

### Run All
```bash
.\Release\bench_advanced_patterns.exe
```

### Run Specific Category
```bash
# Read/Write Ratios
.\Release\bench_advanced_patterns.exe --benchmark_filter="ReadWrite"

# Parallelity
.\Release\bench_advanced_patterns.exe --benchmark_filter="Parallel"

# Self-Protection
.\Release\bench_advanced_patterns.exe --benchmark_filter="SelfProtection"

# Best-Practices
.\Release\bench_advanced_patterns.exe --benchmark_filter="BestPractice"

# Gap Analysis
.\Release\bench_advanced_patterns.exe --benchmark_filter="Gap"
```

### Export Results
```bash
# JSON (for automation)
.\Release\bench_advanced_patterns.exe --benchmark_format=json \
    --benchmark_out=results.json

# CSV (for spreadsheets)
.\Release\bench_advanced_patterns.exe --benchmark_format=csv \
    --benchmark_out=results.csv
```

---

## Expected Outputs

### Success Criteria

| Test Category | Erwartete Ergebnisse | Gap-Schwelle |
|---------------|---------------------|-------------|
| Read/Write | Alle 5 Tests mit messbaren Daten | < 20% Variation |
| Parallelity | Linear scaling bis CPU-cores | > 0.7 speedup/thread |
| Self-Protection | Alle Tests completen ohne Fehler | No crash/hang |
| Best-Practice | Mindestens 2x difference Pattern | >1.5x gap erkennbar |
| Gap-Analysis | Messwerte vs. documented | Abweichungen dokumentieren |

### Performance Expectations

```
Read/Write Ratios:
├── Read-only: 500k-1M ops/s (best case)
├── Balanced: 300k-500k ops/s
├── Write-only: 200k-400k ops/s (lock overhead)

Parallelity:
├── 1 thread:   100k ops/s (baseline)
├── 4 threads:  ~350k (3.5x)
├── 8 threads:  ~700k (7x)
├── 16 threads: ~900k (9x - sublinear)
├── 32 threads: ~1000k (10x - context switch overhead)

Self-Protection:
├── Sustained: No degradation >5 min
├── Burst: Recovery <1 sec
├── 32 conns: All complete without error
├── 100KB docs: <10% slowdown vs 1KB

Best-Practice Gap:
├── Anti-pattern: ~1M ops/sec (baseline)
├── Best-practice: >10M ops/sec (10x+)
├── Realistic gap: 2-5x observable

Gap-Analysis:
├── Sequential writes: ~500k-1M ops/s (vs 1M documented)
├── Random vs sequential: ~20-50% throughput
├── Concurrency scaling: Near-linear <8 cores
├── Transaction overhead: 1-3% actual
├── Index creation: ~10k indices/sec
```

---

## Integration Points

### CI/CD Pipeline
```bash
# Run benchmarks
./bench_advanced_patterns.exe --benchmark_format=json \
    --benchmark_out=bench_results.json

# Parse results
jq '.benchmarks[] | {name, real_time, items_per_second}' \
    bench_results.json > metrics.txt

# Compare with baseline
diff baseline.txt metrics.txt > regressions.txt

# Alert if >10% regression
grep "^>" regressions.txt && echo "ALERT: Performance Regression"
```

### Performance Dashboard
```json
{
  "suite": "advanced_patterns",
  "run_date": "2025-12-18",
  "categories": {
    "read_write_ratios": [
      {"test": "WriteHeavy_80W_20R", "throughput": "X ops/s"},
      ...
    ],
    "parallelity": [
      {"threads": 1, "throughput": "Y ops/s"},
      ...
    ],
    "gaps": [
      {"test": "Gap_RocksDB_SequentialWrites", "gap_pct": "Z%"},
      ...
    ]
  }
}
```

---

## Files

| File | Lines | Zweck |
|------|-------|-------|
| [benchmarks/bench_advanced_patterns.cpp](benchmarks/bench_advanced_patterns.cpp) | ~750 | Implementation |
| [CMakeLists.txt](CMakeLists.txt#L1225) | Target definition | Build config |
| Results: `C:\tmp\advanced_bench_results.json` | Auto-generated | Performance data |

---

## Next Steps

1. **Ergebnisse Analysieren**
   - [ ] Welche Gaps sind größer als erwartet?
   - [ ] Welche Read/Write Ratio ist optimal?
   - [ ] Wann tritt Lock-Contention auf?

2. **Best-Practices Dokumentieren**
   - [ ] Performance-Gaps quantifizieren
   - [ ] Empfehlungen basierend auf Gaps
   - [ ] Patterns für verschiedene Workloads

3. **Optimierungen Identifizieren**
   - [ ] Bottlenecks in Parallelity?
   - [ ] Self-Protection triggern?
   - [ ] Transaction overhead messbar?

4. **CI/CD Integration**
   - [ ] Regression-Tester einbauen
   - [ ] Dashboard-Export
   - [ ] Alerting bei Problemen

---

## References

- **Google Benchmark**: https://github.com/google/benchmark
- **RocksDB Performance**: https://github.com/facebook/rocksdb/wiki/Performance-Tuning
- **C++ OOP Patterns**: Modern C++ by Andrei Alexandrescu
- **Concurrent Programming**: Data Structures and Program Design by Niklaus Wirth

---

**Status**: ✅ Ready for Execution  
**Execution Time**: ~5-10 minutes (all 20 benchmarks)  
**Output Location**: C:\tmp\advanced_bench_results.json  
**Maintainer**: ThemisDB Performance Team  
**Last Updated**: 2026-04-06
