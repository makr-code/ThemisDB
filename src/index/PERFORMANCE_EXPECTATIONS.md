# PERFORMANCE_EXPECTATIONS — src/index

## Scope
- Modul: `src/index`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_vector_search.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| I-VectorInsert | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_VectorSearch_efSearch` |
| I-L2Distance | Siehe Zielbeschreibung: L2Distance 1000×512 | `BM_L2Distance_1000_512` |
| I-CosineDistance | Siehe Zielbeschreibung: CosineDistance 1000×512 | `BM_CosineDistance_1000_512` |
| I-TopK | Siehe Zielbeschreibung: TopK 5000×50 | `BM_TopK_5000_50` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| IG-1 | >= 120000 vec/s (VectorInsert Throughput) | mean aus `BM_VectorSearch_efSearch` |
| IG-2 | <= 2.5 ms (L2Distance Latenz P95) | p95 aus `BM_L2Distance_1000_512` |
| IG-3 | <= 15 ms (TopK Latenz P99) | p99 aus `BM_TopK_5000_50` |
| IG-4 | Regression <= 7 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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