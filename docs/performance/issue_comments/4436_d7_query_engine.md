## 🔍 Tiefenanalyse-Update (2026-04-07): KRITISCH — Benchmark ist No-Op, kein echtes Performance-Gap

### `QueryEngineBench/SimpleEvaluation` misst die Query Engine **nicht**

```cpp
// bench_core_performance.cpp:221–229
BENCHMARK_F(QueryEngineBench, SimpleEvaluation)(benchmark::State& state) {
    for (auto _ : state) {
        double val = 42.0;              // ← kein QueryEngine-Aufruf!
        benchmark::DoNotOptimize(val);
    }
    state.SetItemsProcessed(state.iterations());
}
```

Die 814.5 M items/s messen den Overhead des Benchmark-Frameworks (leere Iteration + `DoNotOptimize`). **Kein einziger Aufruf von `QueryEngine::executeAql()` oder äquivalent.**

### Konsequenzen

| Annahme im Issue | Realität |
|-----------------|----------|
| "814.5 M items/s ist die Query-Engine-Performance" | 814.5 M/s = Benchmark-Framework-Overhead |
| "47% Gap vs. ClickHouse" | Bedeutungsloser Vergleich — beide messen unterschiedliche Dinge |
| "−16% Regression (v1.3.0→v1.3.4)" | Regression im Benchmark-Overhead, nicht in Query-Performance |
| Code-Snippet aus `src/query/executor.cpp` | Fiktiv — diese Datei und dieser Code existieren nicht |

### Echte Query-Engine-Performance (PERFORMANCE_EXPECTATIONS.md §2)

| Benchmark | Ergebnis | Ziel | Status |
|-----------|----------|------|--------|
| Simple AQL WHERE | 3.43 M ops/s @ ~0.3 µs | ≥ 10.000 Queries/s | ✅ |
| Complex WHERE | 3.35 M ops/s | ≥ 1 M ops/s | ✅ |
| JOIN (Users-Posts) | 10.2 M ops/s | ≥ 5 M ops/s | ✅ |

**Es gibt kein echtes Performance-Gap bei der Query Engine.**

### `QueryCompiler` JIT: Lambda-Closure, kein nativer Code

`src/query/query_compiler.cpp:308–375`: Die "JIT-Compilation" baut eine `std::function`-Closure, die den Interpreter mit direktem Capture aufruft. `THEMIS_HAS_LLVM_JIT` ist ein Extension-Point-Kommentar — **nicht implementiert**.

### Empfehlung

**Option A (bevorzugt):** Dieses Issue schließen. Kein echtes Gap.

**Option B:** Issue umformulieren zu:
> *"Erstelle einen echten Query-Engine-Benchmark als Ersatz für den No-Op `SimpleEvaluation`"*
> - Benchmark: `VectorizedExecutionEngine::execute()` auf vorbereiteten `ColumnBatch`-Daten
> - Realistischer Vergleich gegen ClickHouse Columnar-Filter-Kernel
> - Potenzieller echter Kandidat: `ColumnarExecutionEngine` in `src/analytics/columnar_engine.cpp`
