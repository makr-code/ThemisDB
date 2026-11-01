---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1; SOT domain: module-behavior. Canonical references: `benchmarks/benchmark_target_mapping.json`, `benchmarks/transaction/bench_transaction_throughput.cpp`, `benchmarks/transaction/bench_transaction_phase4.cpp`, `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`, and `benchmarks/maintenance/bench_saga_compensation.cpp`. Related issue: #7390.

# PERFORMANCE_EXPECTATIONS — src/transaction

## Scope
- Modul: `src/transaction`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall). Die Werte sind Zielvorgaben aus dem Mapping, keine hier bestätigten Messergebnisse.

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/transaction/bench_transaction_throughput.cpp`
  - `benchmarks/maintenance/bench_saga_compensation.cpp` (Mapping source; not registered in `benchmarks/transaction/CMakeLists.txt`)
  - `benchmarks/transaction/bench_transaction_phase4.cpp`
  - `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`

## Spezifische Erwartungswerte
Parametrisierte Benchmark-Fälle werden mit dem exakten Argument-Suffix angegeben,
wenn ein SLO nur für eine Workload-Größe gilt. So prüft der Comparator nicht
versehentlich Geschwisterfälle mit anderen Größen.

| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| TX-1 | <= 100 us | `TransactionBenchmarkFixture/CommitLatency/1` |
| TX-2 | <= 5 ms | `TransactionBenchmarkFixture/CommitLatency/100` |
| TX-3 | > 6 k/s | `TransactionBenchmarkFixture/WriteOnlyTransaction` |
| TX-4 | <= 5 ms | `TransactionBenchmarkFixture/MixedTransaction` |
| TX-5 | <= 20 ms | `SagaBenchmarkFixture/DatabaseWriteCompensation` |
| TX-6 | <= 1 % overhead (vs. 5 % baseline) | `DeadlockDetectionFixture/DeadlockDetectionOverhead_Predict` |
| TX-7 | < 5 % | `TransactionBenchmarkFixture/OccReadVersionAndUpdate` |
| TX-8 | > 90 % | `TransactionBenchmarkFixture/OccOptimisticPut` |

## Validierung
- Erwartungswerte sind erst erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar ausgeführt wurden und die Zielwerte erreichen; dieser Audit hat keine Benchmarks ausgeführt.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.

## Sourcecode Verification (Module: transaction/performance)

- Gepruefte Benchmark-Quellen:
  - `benchmarks/transaction/bench_transaction_throughput.cpp`
  - `benchmarks/bench_saga_compensation.cpp`
- Gepruefte Ziel-Fall-Zuordnung:
  - Commit, read-only, write-only, mixed, OCC paths in `TransactionBenchmarkFixture`
  - Compensation path in `SagaBenchmarkFixture`
- Ergebnis:
  - The target mapping names cases in the current throughput source, but TX-5 maps to a benchmark outside the transaction benchmark CMake directory.
  - Registered transaction targets are declared in `benchmarks/transaction/CMakeLists.txt`; baseline execution and target attainment remain unverified.
