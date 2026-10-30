---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Acceleration research and evaluation

## Purpose and scope

This page records the engineering questions and evidence required to evaluate acceleration backends. It is for maintainers comparing CPU and optional hardware paths; it is not a survey of accelerator hardware or a claim that every backend is faster. Runtime contracts in [ARCHITECTURE.md](./ARCHITECTURE.md), source interfaces, and the benchmark mapping in [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md) take precedence over this evaluation guidance.

The module's research problem is whether capability-aware dispatch improves a specified workload without changing its correctness, error behavior, or fallback guarantees. The relevant workloads include vector distance/TopK, graph and geospatial batches, matrix operations, AI inference dispatch, and multi-device paths. They do not share one performance profile.

## Assumptions to test

- Hardware discovery describes the current process environment, not a universal deployment profile. Driver, build flags, runtime libraries, and device health affect which backend is usable.
- Dispatch overhead, data movement, batch size, precision, and memory placement can dominate kernel time. A kernel-only speedup does not establish an end-to-end workload gain.
- CPU execution is the comparison baseline and fallback, not proof of parity for every specialized operation. Parity must be checked for the operation and precision under test.
- Multi-device throughput and tail latency depend on topology and partial-failure behavior; results from one host do not establish a fleet-wide guarantee.

## Evaluation method

1. Define one operation, input shape, metric, precision, batch size, and result contract. Keep these fixed between the CPU and candidate-backend runs.
2. Record the repository revision, build preset and enabled backend flags, operating system, CPU, accelerator model/count, driver/runtime versions, and whether the test is hardware-backed.
3. Warm up and measure dispatch and operation time separately where the benchmark supports it. Record throughput plus a latency distribution; include allocation/transfer overhead when it is on the production path.
4. Compare outputs and failure behavior with the corresponding CPU path. Include unavailable-device, unhealthy-device, invalid-input, and resource-pressure cases rather than reporting only the successful path.
5. Preserve raw benchmark output and the host profile with the comparison. A result without this context is not a reusable baseline.

## Evidence and interpretation

The module maps release expectations to named benchmark cases and baseline comparisons in [PERFORMANCE_EXPECTATIONS.md](./PERFORMANCE_EXPECTATIONS.md). Its aggregate gates include regression against the approved release baseline, dispatch/backend latency, multi-device latency, and benchmark-manifest completeness. The benchmarks listed there are the evidence sources; this page does not assert that a current run has passed them.

Focused behavior tests are listed in [tests/acceleration/README.md](../../tests/acceleration/README.md), including dispatch, fallback, failure handling, plugin hardening, and performance-gate tests. Those tests establish the tested behavior of their build and host, not a universal performance ratio. Optional hardware tests must report when the required runtime or device is absent rather than being represented as a successful hardware measurement.

## Trade-offs and limits

- A backend may be present in the binary but unavailable at runtime; source compilation alone is not deployment validation.
- A faster path may consume more device memory or contend with inference workloads. Record capacity and resource conditions alongside latency.
- Precision modes and implementation differences can affect output parity. Do not combine measurements from different precision modes into a single speedup claim.
- Until hardware-in-the-loop evidence is attached to a release profile, treat optional-backend performance as unverified for that profile.

## Source basis

- [Backend contract](../../include/acceleration/compute_backend.h)
- [Capability discovery](./device_manager.cpp)
- [AI dispatch](./ai_hardware_dispatcher.cpp)
- [Benchmark expectations](./PERFORMANCE_EXPECTATIONS.md)
- [Research and release follow-up](./FUTURE_ENHANCEMENTS.md)

**Exact model backend not verifiable from final document text alone.**
