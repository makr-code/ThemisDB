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

# rpc_grpc Tests

The module tests are registered from `tests/rpc_grpc/CMakeLists.txt` when the
corresponding source files are present:

| Test source | Coverage | CMake tier |
|---|---|---|
| `test_rpc_grpc_circuit_breaker_focused.cpp` | Circuit-breaker states, probes, reset, stats, and callback | unit |
| `test_rpc_grpc_contract_hardening_focused.cpp` | Contract constants, enums, descriptors, and fail-closed predicate | unit |
| `test_rpc_grpc_integration_focused.cpp` | Plugin/server lifecycle and loopback listener behavior when gRPC is enabled | integration |
| `test_rpc_grpc_highcardinality_stress.cpp` | Test-local stub registry/stream/reload stress; not a live `GRPCServer` stress test | unit |

## Installation

These are in-tree test targets; no separate test package is installed. Configure
with the repository's test-enabled CMake configuration. The gRPC
plugin/integration cases require the gRPC build option and package; the
circuit-breaker focused test does not require a live gRPC service.

## Usage

Use the configured build directory to run the module tests:

```sh
ctest --test-dir <build-dir> -L rpc_grpc --output-on-failure
```

Additional stream-adapter and observability test sources are registered from
`tests/CMakeLists.txt`, outside this mirrored module directory.
