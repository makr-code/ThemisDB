# PERFORMANCE_EXPECTATIONS — src/aql

## Scope
- Modul: `src/aql`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Dieses Modul nutzt die Ziel-ID-Matrix des Parent-Moduls `query` als Referenzpfad.
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_query.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| Q-SimpleWhere | Siehe Zielbeschreibung: Simple AQL WHERE P99 | `BM_SimpleWhere` |
| Q-ComplexWhere | Siehe Zielbeschreibung: Complex WHERE P99 | `BM_ComplexWhere` |
| Q-JoinUsersPosts | Siehe Zielbeschreibung: JOIN (Users-Posts) P99 | `BM_JoinUsersPosts` |
| Q-Pagination-Offset | Siehe Zielbeschreibung: Pagination Offset P99 | `BM_Pagination_Offset` |
| Q-Pagination-Cursor | Siehe Zielbeschreibung: Pagination Cursor P99 | `BM_Pagination_Cursor` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| AQLG-1 | <= 22 ms (Simple/Complex WHERE P95) | p95 aus `BM_SimpleWhere` und `BM_ComplexWhere` |
| AQLG-2 | <= 45 ms (Join P99) | p99 aus `BM_JoinUsersPosts` |
| AQLG-3 | >= 24000 ops/s (Pagination Throughput) | mean aus `BM_Pagination_Offset` und `BM_Pagination_Cursor` |
| AQLG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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