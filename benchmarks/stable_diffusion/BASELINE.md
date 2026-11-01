---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone. Original creation dates
> could not be verified from available history; `Created` records this AI-assisted revision.

# Stable Diffusion Benchmark Baseline

Last Updated: 2026-10-31

## Scope

Benchmark target: `bench_stable_diffusion_release_gates`

## Baseline Status (no measurements published)

| Case | Backend | Status | Notes |
|---|---|---|---|
| `BM_SD_TimeToPng_Stub512` | Stub | Pending run | Benchmark target exists; local build blocked outside module by storage/RocksDB failures |
| `BM_SD_TimeToPng_InMemoryProxy512` | In-memory proxy | Pending run | Benchmark target exists; local build blocked outside module by storage/RocksDB failures |
| `BM_SD_ParallelGenerate_Stability` | In-memory proxy | Pending run | Benchmark target exists; local build blocked outside module by storage/RocksDB failures |

`SDPluginRealBackendE2ETests` is a separate opt-in test suite, not a benchmark case. Runtime
evidence requires `THEMIS_SD_E2E_MODEL_PATH`.

## Historical Validation Attempt (2026-08-09; not revalidated here)

- Configure succeeded with:
  `cmake --preset community-release-allow-missing-rocksdb -DCMAKE_TOOLCHAIN_FILE= -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_BUILD_BENCHMARKS=ON -DTHEMIS_ENABLE_GPU=OFF`
- Focused suites passed:
  - `test_sd_plugin` → 62/62
  - `test_sd_plugin_registrar` → 12/12
- Benchmark build command:
  `cmake --build build-community-debug-allow-missing-rocksdb --target bench_stable_diffusion_release_gates --parallel 4`
- Current blocker:
  - `src/storage/backup_manager.cpp`: `rocksdb/db.h` missing
  - `src/storage/rocksdb_wrapper.cpp`: existing compile error before benchmark target can link

## Publication Rules

- Update this file whenever release-gate benchmark values change.
- Include command line, hardware profile, and timestamp with each published result block.
- Keep both stub/proxy and real-backend evidence in sync with:
  - `src/stable_diffusion/ROADMAP.md`
  - `src/stable_diffusion/PRODUCTION_REQUIREMENTS.md`
  - `src/stable_diffusion/PERFORMANCE_EXPECTATIONS.md`
