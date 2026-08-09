# Stable Diffusion Benchmark Baseline

Last Updated: 2026-08-09

## Scope

Benchmark target: `bench_stable_diffusion_release_gates`

## Published Baseline (current)

| Case | Backend | Status | Notes |
|---|---|---|---|
| `BM_SD_TimeToPng_Stub512` | Stub | Pending run | Requires benchmark execution in release profile |
| `BM_SD_TimeToPng_InMemoryProxy512` | In-memory proxy | Pending run | Requires benchmark execution in release profile |
| `BM_SD_ParallelGenerate_Stability` | In-memory proxy | Pending run | Validate threads {1,4} without errors |
| `SDPluginRealBackendE2ETests` | Real backend | Gate target present | Runtime evidence requires `THEMIS_SD_E2E_MODEL_PATH` |

## Publication Rules

- Update this file whenever release-gate benchmark values change.
- Include command line, hardware profile, and timestamp with each published result block.
- Keep both stub/proxy and real-backend evidence in sync with:
  - `src/stable_diffusion/ROADMAP.md`
  - `src/stable_diffusion/PRODUCTION_REQUIREMENTS.md`
  - `src/stable_diffusion/PERFORMANCE_EXPECTATIONS.md`
