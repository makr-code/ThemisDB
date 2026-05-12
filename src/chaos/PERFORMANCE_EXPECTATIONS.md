# PERFORMANCE_EXPECTATIONS — src/chaos

## Scope

- Modul: `src/chaos`
- Diese Datei dokumentiert die modulspezifischen Performance-Erwartungen für den In-Process-Fault-Injector und den Scheduler.
- Primäre Benchmark-Quelle: `benchmarks/bench_chaos_stress.cpp`.

## Benchmark Coverage

- `InjectFault_Throughput`
- `InjectFault_AllTypes`
- `IsFaultActive_Positive` / `IsFaultActive_Negative`
- `RecoverFault_Throughput`
- `GetActiveFaults_HighChurn`
- `ExpiredFaultPruning`
- `BM_CallbackDispatch`
- `BM_ConcurrentStress`
- `BM_ChaosScheduler_Schedule`
- `ActiveFaultCount_Throughput`

## Performance Targets

| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| C-1 | `injectFault` skaliert linear mit Iterationen ohne regressiven Einbruch | `InjectFault_Throughput` |
| C-2 | Kein signifikanter Ausreißer zwischen FaultType-Varianten | `InjectFault_AllTypes` |
| C-3 | Positive/negative Query-Pfade bleiben O(1)-nah für kleine bis mittlere Fault-Mengen | `IsFaultActive_Positive`, `IsFaultActive_Negative` |
| C-4 | Recovery bleibt stabil unter wiederholten inject/recover-Zyklen | `RecoverFault_Throughput` |
| C-5 | Snapshot-Operation bleibt unter Last reproduzierbar für 0..1024 aktive Faults | `GetActiveFaults_HighChurn` |
| C-6 | Expired-Pruning erzeugt keinen ungebundenen Wachstumseffekt | `ExpiredFaultPruning` |
| C-7 | Callback-Dispatch skaliert kontrolliert mit 0/1/5/10 Callbacks | `BM_CallbackDispatch` |
| C-8 | Concurrent-Stress (1..16 Threads) bleibt deadlock-frei und reproduzierbar | `BM_ConcurrentStress` |
| C-9 | Scheduler-Einplanung bleibt reproduzierbar bei hoher Schedule-Rate | `BM_ChaosScheduler_Schedule` |
| C-10 | `activeFaultCount` bleibt stabil bei 128 aktiven Faults | `ActiveFaultCount_Throughput` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| CHAG-1 | >= 70000 ops/s (InjectFault Throughput) | mean aus `InjectFault_Throughput` |
| CHAG-2 | <= 20 ms (RecoverFault P95) | p95 aus `RecoverFault_Throughput` |
| CHAG-3 | <= 35 ms (Concurrent Stress P99) | p99 aus `BM_ConcurrentStress` |
| CHAG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

## Validation

- Erwartungswerte gelten als erfüllt, wenn die Benchmarks im Release-Profil reproduzierbar laufen und keine signifikante Regression gegenüber der letzten Referenzmessung zeigen.
- Harte numerische Schwellwerte werden modulweit erst mit dem verteilten Chaos-Backplane-Feature (`src/chaos/ROADMAP.md`) finalisiert.

## Numerische Mindestziele (Release Gate)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| NG-1 Latenz P95 | <= 50 ms | p95 aus Benchmark-Run (`--benchmark_repetitions=5`) |
| NG-2 Latenz P99 | <= 100 ms | p99 aus Benchmark-Run (`--benchmark_repetitions=5`) |
| NG-3 Throughput-Stabilitaet | Regression <= 10 % gegen letzte Baseline | `(current - baseline) / baseline` |

Hinweis:
- Diese Mindestziele gelten als moduluebergreifende Release-Grenzen solange kein strengeres, modulspezifisches Ziel hinterlegt ist.
- Bei `proxy` oder `not_measurable` bleibt das Ziel numerisch gueltig, wird aber ueber den dokumentierten Proxy-Pfad verifiziert.