---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
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
  - "CMakeLists.txt"
  - "bench_retrieval_dedicated_gates.cpp"
  - "../../src/retrieval/README.md"
---

Exact model backend not verifiable from final document text alone.

# Retrieval benchmark directory

`bench_retrieval_dedicated_gates.cpp` builds the
`bench_retrieval_dedicated_gates` Google Benchmark executable when this
directory is included and the benchmark dependency is available. The file
contains synthetic key-to-shard hashing loops and a CPU/GPU-labeled pair of
in-process vector-summing stubs; the GPU-named function explicitly uses no
GPU. It does not call `themis_retrieval`, an index, a production router, or a
GPU backend.

The RT-BM labels and iteration throughput are not measured retrieval latency,
retrieval-engine throughput, GPU break-even results, or release performance
evidence. Do not use them as production baselines.
