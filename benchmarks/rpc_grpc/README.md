---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# rpc_grpc Benchmarks

`benchmarks/rpc_grpc/CMakeLists.txt` registers these targets when
`THEMIS_BUILD_BENCHMARKS` is enabled:

| Target | Source | Scope |
|---|---|---|
| `bench_rpc_grpc_release_gates` | `bench_rpc_grpc_release_gates.cpp` | Contract-type microbenchmarks (enum conversion/dispatch, descriptors, and predicates); labels include proposed thresholds. |
| `bench_rpc_grpc_dedicated_gates` | `bench_rpc_grpc_dedicated_gates.cpp` | In-process stub operations for registration, stream send, credential reload, and service calls; does not start a gRPC server or call the plugin runtime. |

These sources define benchmark workloads, not evidence that a threshold passed.
The dedicated-gate cases are simulations and their timings must not be reported
as end-to-end gRPC performance. Record the build configuration, benchmark
output, machine, and baseline before making performance claims.

## Installation

These are in-tree benchmark executables; no standalone package is installed by
this directory. Enable `THEMIS_BUILD_BENCHMARKS` in the repository CMake
configuration to register the targets.

## Usage

Build the desired target from the benchmark-enabled build tree with
`cmake --build <build-dir> --target <target-name>`, then run the resulting
executable from that build tree. The repository benchmark registration macros
provide the executable and runtime dependencies.
