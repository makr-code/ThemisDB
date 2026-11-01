---
Author: "ThemisDB Team"
Created: "2026-08-08"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Utils Benchmark Baseline — Historical Record

The measurements formerly recorded here were attributed to the Phase A.3
benchmark runner on 2026-08-08. Inspection of
[`phase_a3_benchmark_runner.cpp`](phase_a3_benchmark_runner.cpp) shows that its
privacy, compression, rate-limiter, thread-pool, HKDF, and key-rotation cases
execute synthetic loops rather than the corresponding utils implementations.
The runner is not registered in the current `benchmarks/utils/CMakeLists.txt`.

**These measurements are historical and are not a production-path baseline,
release gate, or evidence that the named subsystem meets a performance
target.** They are intentionally not repeated here as current measurements.

The only benchmark target registered by the directory today is
`bench_utils_release_gates`; it measures configuration-object operations and
small enum-dispatch cases. See
[`../../src/utils/PERFORMANCE_EXPECTATIONS.md`](../../src/utils/PERFORMANCE_EXPECTATIONS.md)
for the current scope and the open work required to establish representative
performance targets.

## Reproducibility status

- Historical report date: 2026-08-08.
- Historic environment and result values have not been independently
  reproduced against current source.
- No production workload baseline or regression threshold is established by
  this document.
