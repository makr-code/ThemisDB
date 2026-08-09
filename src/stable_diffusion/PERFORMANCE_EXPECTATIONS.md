# PERFORMANCE_EXPECTATIONS — src/stable_diffusion

## Scope

- Module: `src/stable_diffusion`
- This file defines release-gate performance expectations for the Stable Diffusion plugin.
- Primary benchmark target: `bench_stable_diffusion_release_gates`.

## Benchmark Coverage

Current benchmark cases in `benchmarks/stable_diffusion/bench_stable_diffusion_release_gates.cpp`:

- `BM_SD_TimeToPng_Stub512`
- `BM_SD_TimeToPng_InMemoryProxy512`
- `BM_SD_ParallelGenerate_Stability`

## Release Gate Targets

| Gate ID | Expectation | Measurement |
|---|---|---|
| SD-BENCH-01 | Stub 512x512 time-to-PNG p95 <= 200 ms | `BM_SD_TimeToPng_Stub512` (`UseRealTime`) |
| SD-BENCH-02 | In-memory proxy 512x512 time-to-PNG p95 <= 250 ms | `BM_SD_TimeToPng_InMemoryProxy512` (`UseRealTime`) |
| SD-BENCH-03 | Parallel stability: 0 benchmark-time errors for threads {1,4} | `BM_SD_ParallelGenerate_Stability` |
| SD-BENCH-04 | Regression <= 10% against last published baseline | `(current - baseline) / baseline` |

## Baseline Publication

- Baseline artifact path: `benchmarks/stable_diffusion/BASELINE.md`
- Mandatory publication includes:
  - stub path metrics
  - in-memory proxy metrics
  - real-backend metrics (when `THEMIS_ENABLE_STABLE_DIFFUSION=ON` and model fixture available)

## Validation

- Run benchmarks in release profile.
- Use at least 5 repetitions for p95/p99 comparisons when publishing baseline updates.
- If real-backend fixture is unavailable in CI, keep the gate status at `[~]` in module roadmap and production requirements until evidence is published.
