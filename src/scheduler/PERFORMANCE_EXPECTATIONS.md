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

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
