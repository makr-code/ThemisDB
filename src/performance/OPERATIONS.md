---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "src/performance/async_metrics_exporter.cpp"
  - "src/performance/prometheus_exporter.cpp"
  - "src/performance/chimera_exporter.cpp"
  - "benchmarks/performance/CMakeLists.txt"
  - "benchmarks/performance/bench_performance_release_gates.cpp"
  - "src/performance/ROADMAP.md"
Related_Issue: "#7366"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Performance Module Operations

## Operational purpose

This page defines how to evaluate and observe the performance module in
development and release operations. It supplements service-specific
deployment/runbooks; it does not define host sizing or production defaults.

## Signals and interpretation

- Cycle metrics are controlled by runtime configuration and sampling. The
  asynchronous collector drains bounded per-thread buffers on its export
  interval; `getDroppedMetrics()` reports dropped entries, and the retained
  aggregate history is capped at 10,000 entries.
- Prometheus and CHIMERA exporters serialize collected measurements. An empty
  string can indicate an export failure and must not be treated as a measured
  zero.
- Workload forecast confidence and optimizer adaptation counts are diagnostic
  inputs. They do not prove that the selected strategy improved the system.
- Accelerator results, NUMA topology/locality statistics, io_uring fallback
  counters, and DiskANN cache/disk statistics help distinguish a requested
  optimization from the path actually used.

Metric and query labels may reveal usage patterns. Limit access and retention
according to the deployment's observability policy and the controls in
[TRUST.md](TRUST.md).

## Release performance gates

The current module benchmark source defines six contract microbenchmarks, but
does not encode a regression percentage, p99 threshold, or baseline comparison.
The benchmark CMake file registers only that executable; a sibling random
access/prefetch benchmark source is present but is not registered there. See
[PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) for the exact cases
and limits of the current evidence. If a release process imposes additional
thresholds, record the owning policy, baseline, and run manifest; do not imply
that these are enforced by the module benchmark itself. The roadmap calls for
representative p95/p99 baseline work and broader coverage in Q1 2027.

## Recovery and change control

For a suspected regression, preserve the current metric and benchmark evidence
before changing flags. Make one approved change at a time, compare with the
same workload, and retain the exact before/after configuration. Roll back the
specific optimization if the regression reproduces or the safe fallback cannot
be verified. For symptom-specific diagnosis and escalation, use
[RUNBOOK.md](RUNBOOK.md).

## Operational limitations

Hardware-specific behavior depends on build options, host capabilities, kernel
support, and permissions. Benchmark measurements are not portable across
hardware without a new baseline. The module roadmap still tracks long-duration
workload-shift validation, broader distributed/high-contention benchmarks,
and operator remediation evidence; these should not be assumed complete from
the existence of local focused tests.

Exact model backend not verifiable from final document text alone.
