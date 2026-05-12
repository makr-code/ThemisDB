# PERFORMANCE_EXPECTATIONS — src/api

## Scope
- Modul: `src/api`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
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

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| APIG-1 | <= 35 ms (GraphQL Execute P99) | p99 aus `BM_GraphQL_Execute_MockResolver` |
| APIG-2 | >= 22000 ops/s (JSON Serialize Throughput) | mean aus `BM_Json_Serialize_SingleDocument` |
| APIG-3 | <= 20 ms (Correlation Header Check P95) | p95 aus `BM_CorrelationId_Header_Check` |
| APIG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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