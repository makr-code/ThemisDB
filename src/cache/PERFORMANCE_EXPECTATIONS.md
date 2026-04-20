# PERFORMANCE_EXPECTATIONS — src/cache

## Scope
- Modul: `src/cache`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_embedding_cache_performance.cpp`
  - `benchmarks/bench_adaptive_query_cache.cpp`
  - `benchmarks/bench_api_endpoints.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| C-1 | Siehe Zielbeschreibung: L1 Hit-Path | `BM_EmbeddingCache_Query_WithIndex` |
| C-2 | Siehe Zielbeschreibung: L2 Hit-Path | `BM_EmbeddingCache_Store_WithIndex` |
| C-3 | Siehe Zielbeschreibung: L3 Hit-Path P99 | `BM_EmbeddingCache_Query_NoIndex` |
| C-4 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_Cache_L1_Put` |
| C-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_GraphQL_Parse_Simple_Uncached` |
| C-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_Cache_L1_Get_Hit` |
| C-7 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_Cache_Mixed_ReadWrite` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
