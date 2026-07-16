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
| TX-1 | <= 100 us | `TransactionBenchmarkFixture/CommitLatency` |
| TX-2 | <= 5 ms | `TransactionBenchmarkFixture/CommitLatency` |
| TX-3 | > 6 k/s | `TransactionBenchmarkFixture/WriteOnlyTransaction` |
| TX-4 | <= 5 ms | `TransactionBenchmarkFixture/MixedTransaction` |
| TX-5 | <= 20 ms | `SagaBenchmarkFixture/DatabaseWriteCompensation` |
| TX-6 | <= 1 % overhead (vs. 5 % baseline) | `TransactionBenchmarkFixture/ReadOnlyTransaction` |
| TX-7 | < 5 % | `TransactionBenchmarkFixture/OccReadVersionAndUpdate` |
| TX-8 | > 90 % | `TransactionBenchmarkFixture/OccOptimisticPut` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.

## Sourcecode Verification (Module: transaction/performance)

- Gepruefte Benchmark-Quellen:
  - `benchmarks/bench_transaction_throughput.cpp`
  - `benchmarks/bench_saga_compensation.cpp`
- Gepruefte Ziel-Fall-Zuordnung:
  - Commit, read-only, write-only, mixed, OCC paths in `TransactionBenchmarkFixture`
  - Compensation path in `SagaBenchmarkFixture`
- Ergebnis:
  - Die referenzierten Benchmark-Faelle sind im aktuellen Benchmark-Source vorhanden.
  - Release-Gates bleiben an reproduzierbare Messlaeufe im Release-Profil gebunden.
