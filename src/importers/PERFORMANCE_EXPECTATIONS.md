# PERFORMANCE_EXPECTATIONS — src/importers

## Scope
- Modul: `src/importers`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Dieses Modul nutzt die Ziel-ID-Matrix des Parent-Moduls `ingestion` als Referenzpfad.
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

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| IMPG-1 | >= 45000 records/s (Batch Import Throughput) | mean aus `IngestionBenchFixture_BatchIngest` |
| IMPG-2 | <= 45 ms (Single Import P95) | p95 aus `IngestionBenchFixture_SingleIngest` |
| IMPG-3 | <= 80 ms (Importer E2E P99) | p99 aus `IngestionBenchFixture_BatchIngest` |
| IMPG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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