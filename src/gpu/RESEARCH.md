---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# GPU Research

## Purpose and scope

This page records how to evaluate GPU acceleration and resource governance in
ThemisDB. It is for developers proposing a backend, dispatch, memory-policy, or
accelerated-query change. It does not claim that a particular benchmark has
already demonstrated a speedup.

The module owns device discovery, allocation policy, dispatch orchestration,
fallback reporting, and telemetry. Workload algorithms and end-user embedding
semantics remain with their consuming modules; see [EMBEDDING.md](EMBEDDING.md).

## Questions and assumptions

Evaluate these independently:

1. Does a candidate backend support the operation and data layout required by
   the caller?
2. Does dispatch and transfer overhead leave an end-to-end benefit at the
   intended batch size?
3. Do admission limits, tenant accounting, and the CPU path preserve the
   caller's correctness and service objective during pressure or device loss?

The measured result depends on the enabled backend, device/driver, edition
limits, feature flags, workload shape, and whether data is already resident on
the device. A CPU-only or stub-safe test is evidence for its tested control
path, not for hardware throughput or GPU/CPU numerical parity.

## Evaluation method

Use the benchmark mapping in
[PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md). It covers memory
manager and pool paths, policy/config/metrics/admin paths, backend comparisons,
vector search, training, topology/load balancing, and erasure coding. For a
change, select the benchmark case that matches the actual path and record:

- device model, driver/runtime, build options, edition, and enabled features;
- input shape, batch size, warm-up, repetitions, and whether transfers are
  included;
- latency distribution (including p99 where release gates require it),
  throughput, and memory use;
- a CPU baseline using the same inputs and output validation;
- fallback, quota-denial, and device-unavailable outcomes as separate cases.

The documented release targets are regression of at most 10% against the
release baseline, the mapped GPU hot-path p99 threshold, and complete mapped
benchmark coverage. The numeric p99 threshold is defined by the release
baseline/configuration, not by this page. Do not substitute synthetic timing
for those release measurements.

## Evidence and limitations

`tests/gpu/` contains focused tests for configuration, policy, discovery,
dispatch, memory, fallback, and integrations. These establish the behavior
asserted by each test and do not by themselves establish performance on physical
hardware. Hardware-dependent results must identify the tested backend and
device. If no compatible device is available, report the test as not run rather
than as a GPU pass.

The current benchmark and gate definitions are in
[PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md); execution and
resource boundaries are in [ARCHITECTURE.md](ARCHITECTURE.md) and
[include/gpu/GPU_CONTRACT.md](../../include/gpu/GPU_CONTRACT.md). Keep new
measurements with the benchmark/release evidence; do not convert an unmeasured
hypothesis into a module capability claim.
