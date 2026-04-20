# PERFORMANCE_EXPECTATIONS — src/toolbox

## Scope
- Modul: `src/toolbox`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Dieses Modul nutzt die Ziel-ID-Matrix des Parent-Moduls `search` als Referenzpfad.
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_rag_hybrid_retriever.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| SE-1 | Siehe Zielbeschreibung: Hybrid Search P99 (10M-Doc-Index) | `BM_RRF_Balanced` |
| SE-2 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_RRF_BM25Only` |
| SE-3 | Siehe Zielbeschreibung: Facet Counting (1k distinct, 100k Docs) | `BM_RRF_VectorOnly` |
| SE-4 | Siehe Zielbeschreibung: LTR Re-Ranking (Top-100) | `BM_Linear_Balanced` |
| SE-5 | Siehe Zielbeschreibung: Autocomplete P99 (1M-Term-Dict) | `BM_ConfigConstruction` |
| SE-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_RRF_Disjoint` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
