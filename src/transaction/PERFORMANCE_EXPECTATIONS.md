# PERFORMANCE_EXPECTATIONS — src/transaction

## Scope
- Modul: `src/transaction`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_transaction_throughput.cpp`
  - `benchmarks/bench_saga_compensation.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| TX-1 | ≤ 100 µs | `TransactionBenchmarkFixture_CommitLatency` |
| TX-2 | ≤ 5 ms | `TransactionBenchmarkFixture_CommitLatency` |
| TX-3 | > 6 k/s | `TransactionBenchmarkFixture_WriteOnlyTransaction` |
| TX-4 | ≤ 5 ms | `TransactionBenchmarkFixture_MixedTransaction` |
| TX-5 | ≤ 20 ms | `SagaBenchmarkFixture_DatabaseWriteCompensation` |
| TX-6 | ≤ 1 % overhead (vs. 5 % baseline) | `TransactionBenchmarkFixture_ReadOnlyTransaction` |
| TX-7 | < 5 % | `TransactionBenchmarkFixture_OccReadVersionAndUpdate` |
| TX-8 | > 90 % | `TransactionBenchmarkFixture_OccOptimisticPut` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
