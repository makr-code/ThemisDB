---
Author: unknown (original author not verified)
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: AI documentation assistant
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# `benchmarks/chimera/`

**Purpose / audience:** directory map for maintainers.

## Checked-in files

- `bench_chimera_adapter.cpp` — C++ CHM-01..06 adapter microbenchmarks.
- `bench_chimera_dedicated_gates.cpp` — C++ CH-BM-01..04 synthetic stub cases.
- `benchmark_config_schema.yaml` — workload/schema description that currently differs from the Python runner's defaults and workload shapes; see [`CHIMERA_README.md`](CHIMERA_README.md).
- `demo_reports/benchmark_comparison.csv` — example comparison data, not current project benchmark evidence.
- `CMakeLists.txt` — standard benchmark registration recipe when benchmark support is enabled.

The separate Python CI runner is `external/chimera/run_ci_benchmarks.py`; its harness implementation is `benchmarks/chimera.py`. The Python runner executes synthetic operations, not the C++ adapter benchmarks.

`THEMIS_BUILD_CHIMERA` defaults to `OFF`; C++ benchmark target registration is managed separately by benchmark CMake configuration. No build or benchmark execution was performed for issue #7336.

## Quickstart references

The Python harness is invoked from the repository root with `python3 external/chimera/run_ci_benchmarks.py` (or its documented `--output`, `--warmup`, and `--iterations` arguments). The C++ targets can be built only when present in the selected benchmark configuration, for example `cmake --build <build-dir> --target bench_chimera_adapter`. Neither command was run for this issue.

Issue/milestone: #7336 / DOC-WEEKLY-2026-44.
