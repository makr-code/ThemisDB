---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-11-01"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "src/performance/*.cpp"
  - "include/performance/*.h"
  - "CMakeLists.txt"
  - "cmake/CMakeLists.txt"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Architecture — Performance Module

## Boundary and composition

The module contains independent measurement/export, optimization, memory/cache,
algorithm, and optional hardware/I/O components. It does not impose a single
module-wide execution pipeline: calling modules choose and compose the public
interfaces declared under [`../../include/performance`](../../include/performance).
`performance_api_contract.h` supplies shared result/error types for some
interfaces; individual APIs also use exceptions, booleans, or errno results.

## Dataflows

### Measurement and export

When enabled by compile definitions, cycle macros use the cycle-counter
interface and the collector records operation measurements into per-thread
buffers. The asynchronous collector periodically drains those buffers into a
bounded retained history; the Prometheus and CHIMERA serializers consume
collected entries. Disabled macros expand to no-ops where specified in
`cycle_metrics_config.h`. A missing export is not a measured zero.

### Workload adaptation and feedback

Callers supply workload observations to `WorkloadPredictor` or query observations
to `WorkloadAdaptiveOptimizer` and `phase3::PerQueryCostModel`. These return
forecasts, strategy choices, or calibration inputs. Applying a suggested
thread-pool/cache size or optimizer calibration is a caller responsibility;
the prediction is not evidence that an external resource was resized or that
performance improved.

### Optional memory and hardware paths

Callers check compile-time support, runtime controls, and host/OS capabilities
before using NUMA allocation, accelerator dispatch, io_uring, PMU, or
persistent-memory facilities. A capability check, explicit error, or documented
fallback is API-specific. Do not equate requested configuration with use of an
accelerated path.

## Build and verification boundary

Measurement and optional-I/O CMake options are declared in root
`CMakeLists.txt`; allocator/cache options are also declared in
`cmake/CMakeLists.txt`. Header comments mention additional feature names, but
not every such name is verified as a configured CMake option or active target
path. See [`../../include/performance/README.md`](../../include/performance/README.md)
for the verified option inventory.

Focused test registration is described in
[`../../tests/performance/README.md`](../../tests/performance/README.md). The
module test CMake file includes root-level tests and recursively registers
phase3/phase4 sources with path-derived names.
Benchmark registration is described in
[`../../benchmarks/performance/README.md`](../../benchmarks/performance/README.md).

## Direct consumer examples

Verified direct references include the cache allocator header, the index
adapter's DiskANN wrapper, query optimizer cost-model attachment, storage
prefetch hints, and server/RAG includes of adaptive interfaces. These are
integration points, not proof that every optional runtime path is enabled in a
given build.

Exact model backend not verifiable from final document text alone.
