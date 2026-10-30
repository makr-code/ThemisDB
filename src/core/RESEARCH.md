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

# Core Research Notes

> Exact model backend not verifiable from final document text alone.

## Purpose and scope

This page records how to evaluate Core's cross-cutting concern infrastructure: adapter selection, lookup and replacement, telemetry integrations, cache behavior, and failure handling. It is for maintainers designing changes or collecting evidence; it is not a report of benchmark results.

Core is an infrastructure layer, not a statistical or machine-learning subsystem. It does not define a model-quality dataset or an embedding evaluation corpus. The applicable research questions are about lifecycle correctness, failure isolation, overhead, and operational behavior.

## Evidence and assumptions

The current contract is defined by [`ConcernsContext`](../../include/core/concerns/concerns_context.h), [`AdapterRegistry`](../../include/core/concerns/adapter_registry.h), [`ConfigValidator`](../../include/core/config_validator.h), and their implementations under `src/core/concerns/`. Focused test sources are registered from [`tests/core/CMakeLists.txt`](../../tests/core/CMakeLists.txt). The wiki and historical evidence are secondary; implementation and tests take precedence.

Assumptions to verify for each evaluation:

- Compare the same concern operation, adapter configuration, build, and workload across variants.
- Record whether the operation uses a no-op, in-process, or external adapter; external I/O must not be conflated with interface dispatch cost.
- Test both steady-state and failure/recovery behavior. A healthy local test does not establish remote exporter or Redis availability.
- Do not infer a production performance target from the illustrative values in older module material; the repository has no current benchmark baseline in this document.

## Evaluation method

1. Establish a no-op or in-process baseline with the same caller and operation.
2. Measure logging, span creation/end, metric update, cache hit/miss, registry resolve, and adapter replacement separately.
3. Capture throughput, p50/p95/p99 latency, allocation and memory use, error count, and configuration. Include environment, build type, adapter/backend, workload size, and test revision with every result.
4. Exercise invalid configuration, absent adapters, exporter/cache outages, high-cardinality metric labels, concurrent resolution, and replacement with outstanding handles.
5. Repeat each run enough to report variation; treat results without reproducible setup and raw output as exploratory rather than acceptance evidence.

Relevant regression suites include `test_concerns_context_focused.cpp`, `test_adapter_registry_focused.cpp`, `test_adapter_signing_focused.cpp`, `test_plugin_loading_focused.cpp`, `test_circuit_breaker_focused.cpp`, and `test_core_highcardinality_stress.cpp`. Their existence is not proof that a particular configuration or performance target has passed; check the current build/test results.

## Trade-offs and open evidence

- No-op adapters reduce external work but suppress the corresponding signals; production mode rejects no-op tracing and metrics.
- External adapters add backend and network failure domains; Core's API abstraction does not make those dependencies available.
- Registry hot-swap replaces the visible adapter before waiting up to 100 ms for old shared references. A timeout is reported as `false`; callers must not interpret it as proof that old calls were cancelled.
- Cache results are only as coherent as the selected implementation. An in-process cache is not cross-node invalidation.

No measured Core benchmark result is asserted here. Add a reproducible benchmark and its result before making latency, throughput, memory, or scalability claims. See [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md), [`MODULE_EVIDENCE.md`](MODULE_EVIDENCE.md), and [`OPERATIONS.md`](OPERATIONS.md).
