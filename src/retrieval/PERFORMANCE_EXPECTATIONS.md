---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "README.md"
  - "CMakeLists.txt"
  - "../../benchmarks/retrieval/README.md"
  - "../../benchmarks/retrieval/bench_retrieval_dedicated_gates.cpp"
---

Exact model backend not verifiable from final document text alone.

# Retrieval performance evidence

## Current evidence

No production retrieval latency, throughput, memory, CPU, GPU, p95, or p99
baseline is established by the current module.

The only benchmark source under `benchmarks/retrieval/` measures synthetic
key-to-shard hashing and in-process vector-summing functions. The function
named as a GPU path is explicitly a stub and performs no GPU work. These
results are not retrieval-engine performance measurements or release gates.

The tests under `tests/retrieval/` also include local mocks and routing/shard
stubs; their timings and stress-loop completion do not establish production
performance.

## Configuration and interpretation

The retrieval target has no runtime performance/configuration controls. Its
module-specific CMake option is `THEMIS_BUILD_RETRIEVAL` (default `ON`); this
only enables or disables building the artifact library.

Until a real retrieval execution path and representative benchmark workload
are identified, do not publish numerical performance expectations or infer
hardware break-even, scaling, or operational capacity from the synthetic
gates. The required workload and owner for future production measurements are
not established by the current code.
