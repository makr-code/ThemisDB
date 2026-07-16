# PERFORMANCE_EXPECTATIONS — src/onnx_clip

## Scope
- Modul: `src/onnx_clip`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Ops/s, Latenz, Throughput) für Release-Gates.
- Primärquelle: `benchmarks/benchmark_target_mapping.json` (Ziel-ID ↔ Benchmark-Fall).

## Benchmark-Bezug
- Relevante Benchmark-Dateien:
  - `benchmarks/bench_image_analysis.cpp`

## Spezifische Erwartungswerte
| Ziel-ID | Erwartungswert | Benchmark-Fall |
|---|---|---|
| OC-1 | Siehe Zielbeschreibung: Batched Inference (Batch 64) | `BM_ImageEmbedding_Batch` |
| OC-2 | Siehe Zielbeschreibung: ViT-B/32 CUDA (Batch 64) | `BM_ImageEmbedding_BackendComparison` |
| OC-3 | Siehe Zielbeschreibung: ViT-B/32 CPU (Batch 16) | `BM_ImageEmbedding_Batch` |
| OC-4 | Siehe Zielbeschreibung: Text Encoding P95 (CPU) | `BM_ImageCaptioning` |
| OC-5 | Keine absolute Zielzahl dokumentiert; Throughput-Regression <= 10 % und P95-Regression <= 15 % ggü. Baseline | `BM_Plugin_Initialization` |

## Modulspezifische harte Grenzwerte (v1.9.0)

| Gate-ID | Erwartungswert | Messregel |
|---|---|---|
| ONXG-1 | >= 300 img/s (Batched Inference Throughput) | mean aus `BM_ImageEmbedding_Batch` |
| ONXG-2 | <= 45 ms (Image Captioning P95) | p95 aus `BM_ImageCaptioning` |
| ONXG-3 | <= 70 ms (Backend Comparison P99) | p99 aus `BM_ImageEmbedding_BackendComparison` |
| ONXG-4 | Regression <= 8 % gegen letzte Release-Baseline | `(current - baseline) / baseline` |

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