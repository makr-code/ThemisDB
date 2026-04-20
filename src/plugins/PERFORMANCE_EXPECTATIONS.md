# PERFORMANCE_EXPECTATIONS — src/plugins

## Scope
- Modul: `src/plugins`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Dieses Modul nutzt die Ziel-ID-Matrix des Parent-Moduls `api` als Referenzpfad.
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_api_endpoints.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| API-1 | Siehe Zielbeschreibung: GraphQL Parse+Execute P99 | `BM_GraphQL_Execute_MockResolver` |
| API-2 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_Json_Serialize_SingleDocument` |
| API-3 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_GraphQL_Parse_Complex_Uncached` |
| API-4 | Siehe Zielbeschreibung: Bulk Insert (10k Docs) | `BM_GraphQL_Parse_Simple_Uncached` |
| API-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_CorrelationId_Generate_UUIDv4` |
| API-6 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_CorrelationId_Header_Check` |
| API-7 | Siehe Zielbeschreibung: OTLP Flush (64 Spans) | `BM_GraphQL_Parse_Simple_Cached` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
