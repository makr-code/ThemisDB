# PERFORMANCE_EXPECTATIONS — src/scheduler

## Scope
- Modul: `src/scheduler`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_task_scheduler.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| SCH-1 | Siehe Zielbeschreibung: Scheduler Loop Tick P99 | `TaskSchedulerBenchFixture_ListTasks` |
| SCH-2 | Siehe Zielbeschreibung: Task Dispatch P99 | `TaskSchedulerBenchFixture_ExecuteTaskNow` |
| SCH-3 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `TaskSchedulerBenchFixture_GetStats` |
| SCH-4 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `TaskSchedulerBenchFixture_ConcurrentRegister` |
| SCH-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `TaskSchedulerBenchFixture_RegisterUnregister` |
| SCH-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `TaskSchedulerBenchFixture_ExecuteTaskNow` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| SCHG-1 | <= 20 ms (Scheduler Tick P95) | p95 aus `TaskSchedulerBenchFixture_ListTasks` |
| SCHG-2 | <= 45 ms (Task Dispatch P99) | p99 aus `TaskSchedulerBenchFixture_ExecuteTaskNow` |
| SCHG-3 | >= 30000 ops/s (Register/Unregister Throughput) | mean aus `TaskSchedulerBenchFixture_RegisterUnregister` |
| SCHG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.

## Numerische Mindestziele (Release Gate)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| NG-1 Latenz P95 | <= 50 ms | p95 aus Benchmark-Run (`--benchmark_repetitions=5`) |
| NG-2 Latenz P99 | <= 100 ms | p99 aus Benchmark-Run (`--benchmark_repetitions=5`) |
| NG-3 Throughput-Stabilitaet | Regression <= 10 % gegen letzte Baseline | `(current - baseline) / baseline` |

Hinweis:
- Diese Mindestziele gelten als moduluebergreifende Release-Grenzen solange kein strengeres, modulspezifisches Ziel hinterlegt ist.
- Bei `proxy` oder `not_measurable` bleibt das Ziel numerisch gueltig, wird aber ueber den dokumentierten Proxy-Pfad verifiziert.