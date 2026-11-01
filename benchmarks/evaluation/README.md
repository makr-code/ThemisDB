---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "OpenAI ChatGPT"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

AI provenance: Exact model backend not verifiable from final document text alone.

# Evaluation module benchmark source

This directory contains `bench_evaluation_dedicated_gates.cpp`. The local
`CMakeLists.txt` only configures benchmark targets when
`THEMIS_BUILD_BENCHMARKS` is enabled and discovers `bench_*.cpp` files through
the repository's standard benchmark helper.

The source's EV-BM-01 through EV-BM-04 cases measure in-process stub functions
for a synthetic metric calculation, aggregation, harness invocation and a
deterministic judge stub. They do not call the evaluation metric, planner,
retrieval, or external judge implementations. Thresholds written in the source
are benchmark criteria, not captured results or proof that a gate passed.
The benchmark source does not calculate p50, p95 or p99 percentiles; it cannot
establish that those latency targets were achieved.

EPIC 2 planner and matrix benchmarks are under
`benchmarks/epic2_evaluation/` and use separate CMake registration conditions.
In particular, the `artifact_staleness_bench.cc` source is not registered by
that directory's current CMake file. See
[`../../src/evaluation/PERFORMANCE_EXPECTATIONS.md`](../../src/evaluation/PERFORMANCE_EXPECTATIONS.md)
for the distinction between proposed guardrails and measured evidence. No
benchmark was run as part of this documentation-only refresh.
