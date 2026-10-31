---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Distributed Tensor Benchmarks

`CMakeLists.txt` adds benchmark sources matching `bench_*.cpp` only when
`THEMIS_BUILD_BENCHMARKS` is enabled. The current source is
`bench_distributed_tensor_dedicated_gates.cpp`, registered as the
`bench_distributed_tensor_dedicated_gates` build target. The parent
`benchmarks/CMakeLists.txt` also registers it with CTest under the `tensor`
module and `release_critical` label.

Build the target after configuring a benchmark-enabled repository build:

```sh
cmake --build <build-dir> --target bench_distributed_tensor_dedicated_gates
ctest --test-dir <build-dir> -R '^bench_distributed_tensor_dedicated_gates$'
```

This source uses in-process stubs for dispatch, synchronization, and routing;
it does not call distributed tensor runtime components or exercise network/GPU
paths. Its source-comment thresholds are gate definitions, not measured
production performance results. The separate
`../epic3_distributed_tensor/` directory contains additional workload
benchmarks and has its own build configuration.
