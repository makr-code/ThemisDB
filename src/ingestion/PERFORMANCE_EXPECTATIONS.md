# PERFORMANCE_EXPECTATIONS — src/ingestion

## Scope
- Modul: `src/ingestion`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_ingestion_kv.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| ING-1 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `IngestionBenchFixture_BatchIngest` |
| ING-2 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `IngestionBenchFixture_SingleIngest` |
| ING-3 | Siehe Zielbeschreibung: Kafka → Document E2E P99 | `IngestionBenchFixture_BatchIngest` |
| ING-4 | Siehe Zielbeschreibung: S3 Concurrent Download | `IngestionBenchFixture_SingleIngest` |
| ING-5 | Siehe Zielbeschreibung: Quarantine Queue Scan (100k) | `IngestionBenchFixture_SingleIngest` |

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
