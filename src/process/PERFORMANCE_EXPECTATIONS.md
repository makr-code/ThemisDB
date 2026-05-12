# PERFORMANCE_EXPECTATIONS — src/process

## Scope
- Modul: `src/process`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Direkte Mapping-Einträge fehlen; Fallback auf modulnahe Benchmarks `benchmarks/bench_process*.cpp` und globale Systemziele.

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| MOD-BASELINE | Throughput-Regression <= 10 %, P95-Regression <= 15 %, P99/P50 <= 2.5x, Peak-Memory <= 120 % ggü. Baseline | modulnahe Benchmarks |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| PRCG-1 | <= 70 ms (Process Pipeline P95) | p95 aus modulnahen `bench_process*` Benchmarks |
| PRCG-2 | <= 120 ms (Process Pipeline P99) | p99 aus modulnahen `bench_process*` Benchmarks |
| PRCG-3 | >= 5000 ops/s (Process Throughput) | mean aus modulnahen `bench_process*` Benchmarks |
| PRCG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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