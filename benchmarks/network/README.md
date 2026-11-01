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

# Network Benchmarks

`CMakeLists.txt` discovers `bench_*.cpp` files when
`THEMIS_BUILD_BENCHMARKS=ON` and Google Benchmark is available. It registers
each executable as a CTest benchmark target and installs it under
`bin/benchmarks`.

The Linux release preset disables benchmarks by default. Configure and build
the current gate executables with:

```sh
cmake --preset linux-release -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build-gcc-linux-release \
  --target bench_network_bench_network_release_gates \
           bench_network_bench_network_routing_gates \
           bench_network_bench_network_protocol_path
```

The release and routing gates exercise in-process helper paths; the protocol
path cases are explicitly stubs. These targets do not establish live transport
capacity or production latency. See [`../../src/network/RESEARCH.md`](../../src/network/RESEARCH.md)
for measurement limits and interpretation.
