---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# `distributed_knowledge` Benchmarks

The module CMake file registers these benchmark targets when
`THEMIS_BUILD_BENCHMARKS` is enabled:

| Target | Source | Coverage |
|---|---|---|
| `bench_distributed_knowledge` | `bench_distributed_knowledge.cpp` | LoRA aggregation, federated RAG merge, feedback dedup throughput, and feedback publish latency |
| `bench_distributed_knowledge_or` | `bench_distributed_knowledge_or.cpp` | Aggregation without timeout, feedback backpressure, timed-out-shard merge, coordinator erase, and low-risk feedback checks |
| `bench_dk_release_gates` | `bench_dk_release_gates.cpp` | DKRG-01..DKRG-06 local in-memory/mock cases for entity, graph, LWW, federation-union, and serialization helpers |

The DKRG cases exercise local helpers and contract functions; they are not
measurements of a live shard topology. Benchmark source/target registration does
not establish that a benchmark was run or met its declared threshold. No
benchmark was executed as part of this documentation audit.
