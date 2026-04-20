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

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
