# PERFORMANCE_EXPECTATIONS — src/temporal

## Scope
- Modul: `src/temporal`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_temporal_queries.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| TM-1 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_BiTemporalTable_Insert` |
| TM-2 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_BiTemporalTable_QueryBiTemporal` |
| TM-3 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_BiTemporalTable_QueryCurrentByValidTime` |
| TM-4 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_BiTemporalTable_Delete` |
| TM-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_BiTemporalTable_Update` |
| TM-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_BiTemporalTable_GetHistory` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| TMG-1 | >= 50000 ops/s (BiTemporal Insert Throughput) | mean aus `BM_BiTemporalTable_Insert` |
| TMG-2 | <= 30 ms (BiTemporal Query P95) | p95 aus `BM_BiTemporalTable_QueryBiTemporal` |
| TMG-3 | <= 35 ms (History Query P99) | p99 aus `BM_BiTemporalTable_GetHistory` |
| TMG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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