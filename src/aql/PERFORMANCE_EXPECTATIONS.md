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

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
