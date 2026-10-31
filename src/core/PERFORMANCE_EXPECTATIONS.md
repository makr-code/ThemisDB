---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

# Core Performance Evidence

## Scope and current evidence

No measured performance result or release threshold for the Core concern APIs is
established by the current scoped benchmark source.

`benchmarks/core/CMakeLists.txt` registers `bench_core_performance` from
`benchmarks/core/bench_core_performance.cpp`. Despite its directory and target
name, the source contains vector-index, secondary-index, direct-key/query,
graph-index, and timeseries cases. It does not register `ConcernsContext`,
logging, metrics, cache, or adapter-registry performance cases.

The previously referenced `benchmarks/bench_di_logging.cpp` is absent from the
current repository. Its former benchmark names and associated thresholds are
therefore not evidence-backed and are not listed as current expectations here.

## Evidence boundary

- Benchmark source registration is not evidence that a benchmark was built or run.
- No benchmark execution or measured output is asserted by this document.
- Do not make Core latency, throughput, memory, or scalability claims from the
  unrelated cases in `bench_core_performance.cpp`.

## Follow-up gap

If Core performance thresholds are required, add direct benchmark cases for the
specific concern APIs, define reproducible workloads and acceptance thresholds,
and record run configuration plus raw results. Until that evidence exists, Core
performance expectations remain unspecified.
