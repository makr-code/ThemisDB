---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Analytics benchmark build and execution

This guide covers the targets registered by
[`CMakeLists.txt`](./CMakeLists.txt). This directory does not define a GitHub
Actions workflow. Do not treat the sample workflow, S3 baseline, or gate scripts
in the superseded version of this document as current CI configuration.

## Configure and build

From the repository root, configure a build with benchmarks enabled and build a
specific target:

```bash
cmake -S . -B build-analytics-bench \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-analytics-bench --target bench_analytics_operability_paths
```

Other registered target names are listed in [`INDEX.md`](./INDEX.md). Runtime
binaries are written to `<build-dir>/bin/`, so the executable above is
`build-analytics-bench/bin/bench_analytics_operability_paths`.

## Run and capture output

Google Benchmark options can be passed directly to a built executable:

```bash
./build-analytics-bench/bin/bench_analytics_operability_paths --benchmark_list_tests
./build-analytics-bench/bin/bench_analytics_operability_paths \
  --benchmark_filter='BM_AO01_ExportJsonToString' \
  --benchmark_format=json \
  --benchmark_out=analytics-results.json
```

Record the commit, compiler/build configuration, operating system, CPU model,
and benchmark command alongside any result used for comparison. Compare only
like-for-like runs on the same benchmark case and workload.

## Interpretation and limitations

- Several benchmark sources contain local simulations or stub workloads and do
  not execute the production analytics implementation. See
  [`BENCH_GAP_CLOSURE_REPORT.md`](./BENCH_GAP_CLOSURE_REPORT.md) and inspect
  each source before attributing a result to production behavior.
- A benchmark threshold in a source or helper script is not proof that the
  threshold was measured or passed. Store execution output and hardware context
  as evidence before making a release claim.
- `benchmark_runner.py` exposes `--build-dir` and `--output`, but currently
  resolves binaries directly beneath the supplied build directory. The standard
  CMake runtime location is `<build-dir>/bin/`; verify or update the helper
  before relying on it with a normal CMake build.
- The repository-level workflow configuration is authoritative for CI
  participation. No analytics benchmark workflow reference was found in the
  checked workflow tree on 2026-10-31.

## Validation checklist

- Build only the benchmark targets required for the change.
- List cases with `--benchmark_list_tests` before selecting a filter.
- Run on representative hardware when using results for performance decisions.
- Save JSON output and configuration metadata; do not infer pass/fail from
  benchmark availability alone.
- For behavior changes, run the focused analytics tests as well; benchmarks do
  not replace correctness tests.
