# benchmarks/stable_diffusion

Release-gate benchmark suite for `src/stable_diffusion`.

## Targets

- `bench_stable_diffusion_release_gates`
  - `BM_SD_TimeToPng_Stub512`
  - `BM_SD_TimeToPng_InMemoryProxy512`
  - `BM_SD_ParallelGenerate_Stability`

## Gate intent

- Measure 512x512 request latency on stub and in-memory proxy backends.
- Validate multi-threaded request-path stability under benchmark threads.
