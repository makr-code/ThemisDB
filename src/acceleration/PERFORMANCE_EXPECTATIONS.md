# PERFORMANCE_EXPECTATIONS — src/acceleration

<!-- Status: current | validated: 2026-05-10 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Scope

- Modul: `src/acceleration`
- Diese Datei dokumentiert die modulspezifischen, messbaren Performance-Erwartungswerte (Latenz,
  Throughput, Speedup) für Release-Gates im Acceleration-Modul.
- Primärquelle: `benchmarks/bench_cuda_vs_cpu.cpp`; Baselines in
  `benchmarks/baselines/acceleration/baseline.json`.

## Benchmark-Bezug

Relevante Benchmark-Dateien für dieses Modul:

- `benchmarks/bench_cuda_vs_cpu.cpp` — CUDA vs. CPU ANN-Latenz/Throughput (JSON-Output)
- `benchmarks/multi_gpu_bench.cpp` — Multi-GPU-Skalierungseffizienz (NCCL/RCCL)
- CI-Regression-Gate: `.github/workflows/acceleration-benchmark-ci.yml`
  (minor 5 %, major 10 %, critical 20 % Regression-Schwellen)

## Spezifische Erwartungswerte

| Ziel-ID | Beschreibung | Erwartungswert | Benchmark-Fall |
|---|---|---|---|
| ACC-1 | CUDA ANN-Latenz (1M × 128-dim float32 L2, single GPU) | < 8 ms auf RTX 3090 | `BenchCudaVsCpu_L2_1M_128dim` |
| ACC-2 | CUDA ANN-Throughput ggü. CPU AVX2 | ≥ 10× Speedup | `BenchCudaVsCpu_Throughput` |
| ACC-3 | GPU-Speicherbedarf (10M × 128-dim Vektoren) | < 2 GB device memory | `BenchCudaVsCpu_MemoryFootprint` |
| ACC-4 | CPU-Fallback-Latenz ggü. SIMD-Baseline | ≤ 2× Overhead | `BenchCudaVsCpu_CpuFallback` |
| ACC-5 | CUDA HNSW k=1024 single-pass (dynamisches Shared Memory) | SM-Nutzung ≤ 32 KB; keine Regression ggü. k=256 | `BenchCudaHnsw_LargeK` |
| ACC-6 | Multi-GPU ANN p99-Latenz (100M × 128-dim, 4× A100 80 GB, k=100) | < 15 ms | `BenchMultiGpu_p99_4xA100` |
| ACC-7 | `mergeTopK`-Overhead (worldSize=4, k=100, NVLink-3) | < 500 µs | `BenchMultiGpu_MergeTopK` |
| ACC-8 | Multi-GPU lineare Skalierungseffizienz (1→4 GPUs) | ≥ 75 % | `BenchMultiGpu_ScalingEfficiency` |
| ACC-9 | `VLLMResourceManager::getStats()` Aufruflatenz (Linux) | < 2 ms | `BenchVllmResourceStats` |
| ACC-10 | CI-Regressionsschwelle Throughput | ≤ 5 % (minor) / ≤ 10 % (major) / ≤ 20 % (critical) | `acceleration-benchmark-ci.yml` |

## Validierung

- Erwartungswerte gelten als erfüllt, wenn die zugeordneten Benchmarks im Release-Profil
  (`cmake --preset linux-release`) reproduzierbar laufen und die Zielwerte erreichen.
- ACC-6 bis ACC-8 sind Hardware-abhängig (4× A100); bei fehlender Hardware werden diese
  Benchmarks als `SKIPPED` markiert; die Zielwerte bleiben verbindlich für Produktions-Releases.
- ACC-5 und ACC-10 sind CI-pflichtig und blockieren bei Überschreitung der Schwellen.

## Quellen

- `src/acceleration/FUTURE_ENHANCEMENTS.md` — Abschnitt "CUDA Kernel Completion" und
  "NCCL/RCCL Distributed mergeTopK", "CUDA HNSW Kernel: Remove Silent k > kMaxK Clamping"
- `src/acceleration/ROADMAP.md` — Phase 5: Performance/Hardening
- `src/acceleration/ARCHITECTURE.md` — Abschnitt 7 (Performance Architecture)
