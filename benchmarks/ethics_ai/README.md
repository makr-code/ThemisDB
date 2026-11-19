---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-19
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# benchmarks/ethics_ai

Module benchmark sources for `src/ethics_ai`.

These benchmarks measure only the in-tree implementation path. Root `ROADMAP.md` identifies an external plugin submodule as the canonical implementation; no performance result for that external code is claimed here.

- The root `benchmarks/CMakeLists.txt` conditionally defines `bench_ethics_ai_plugin` and separately adds this directory. The plugin benchmark source contains loader, store, RAG, discourse-engine, and evaluator cases. Its creation block checks for `${CMAKE_CURRENT_SOURCE_DIR}/ethics_ai/bench_ethics_ai_plugin.cpp` and compiles that same nested source, so the checked-in source path matches the guard. Target generation requires `THEMIS_BUILD_BENCHMARKS=ON` and Google Benchmark; CTest registration is conditional on target creation. The current environment does not have Google Benchmark discoverable, so generated target presence and compilation have not been verified here.
- `bench_ldm.cpp` contains eight LDM/audit-log cases and uses a deterministic mock registry and stub inference. It is considered only when the root build has `THEMIS_BUILD_BENCHMARKS=ON` and Google Benchmark is found; `bench_ethics_ai_ldm` is created only when both `benchmark::benchmark` and `themis_core` targets exist.

Benchmark case names, source-comment targets, registration conditions, and LDM fixture limits are mapped in [`../../src/ethics_ai/PERFORMANCE_EXPECTATIONS.md`](../../src/ethics_ai/PERFORMANCE_EXPECTATIONS.md). Checked-in source and CMake registration do not establish measured results or CI enforcement of numerical targets.
