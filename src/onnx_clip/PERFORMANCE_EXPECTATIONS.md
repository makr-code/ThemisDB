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

## Validierung
- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil reproduzierbar laufen und die Zielwerte erreichen.
- Bei `proxy`/`not_measurable`-Ziel-IDs ist ein dedizierter Messpfad als Folgeaufgabe zu tracken; bis dahin gilt das dokumentierte Proxy-Ziel.
