# PERFORMANCE_EXPECTATIONS — src/geo

## Scope
- Modul: `src/geo`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_hybrid_vector_geo.cpp`
  - `benchmarks/bench_spatial_index.cpp`
  - `benchmarks/bench_geo_cpu_gpu.cpp`
  - `benchmarks/bench_spatial_join.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| GEO-1 | >= 20 M/s | `BM_GeoDistance_Haversine` |
| GEO-2 | >= 30 M/s | `BM_RTree_Contains` |
| GEO-3 | <= 5 ms (R-Tree) | `BM_RTree_Intersects` |
| GEO-4 | <= 3 s | `BM_RTree_BulkLoad` |
| GEO-5 | <= 200 ms/Core | `BM_GeoCPUExact_StBuffer` |
| GEO-6 | <= 500 ms | `BM_SpatialJoin_First1000` |
| GEO-7 | <= 2 s | `n/a` |
| GEO-8 | <= 50 ms | `BM_GeoGPU_BatchIntersects` |
| GEO-9 | > 100x vs. CPU | `n/a` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
