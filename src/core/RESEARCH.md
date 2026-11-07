---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-06"
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

## Research Basis for v2.5.0 and v3.0.0

The following standards and research inform the tests and acceptance method in [`ROADMAP.md`](ROADMAP.md#v250-v300-research-and-wave-plan). They provide engineering guidance, not evidence that the current Core implementation already conforms or meets a performance target.

| Source | Finding applied to Core planning |
|---|---|
| Kalibera, T. and Jones, R. (2013), [*Rigorous Benchmarking in Reasonable Time*](https://doi.org/10.1145/2464157.2464160), ISMM '13. | Benchmark results should account for experimental variation. The plan requires repeated comparable runs, raw output, and confidence intervals rather than a single best result. |
| W3C (2021), [Trace Context Recommendation](https://www.w3.org/TR/2021/REC-trace-context-1-20211123/). | `traceparent`/`tracestate` are standardized propagation fields; test valid and invalid context across the actual supported boundaries and do not let malformed external context fail requests. |
| NIST (2022), [SP 800-218, Secure Software Development Framework v1.1](https://csrc.nist.gov/pubs/sp/800/218/final); [MITRE CWE-829](https://cwe.mitre.org/data/definitions/829.html) and [CWE-427](https://cwe.mitre.org/data/definitions/427.html). | Plugin delivery and loading need an explicit trusted-source and path threat model. A digest check alone does not establish signer identity or protect an attacker-writable trust path. |
| SLSA, [Build Provenance specification v1.2](https://slsa.dev/spec/v1.2/); Sigstore, [Cosign verification documentation](https://docs.sigstore.dev/cosign/verifying/verify/). | Signed provenance and signer identity can be release/admission evidence when rooted in an explicit trust policy; neither proves plugin behavior is safe. |
| Prometheus, [Instrumentation guidance](https://prometheus.io/docs/practices/instrumentation/); OpenTelemetry, [Metrics SDK specification](https://opentelemetry.io/docs/specs/otel/metrics/sdk/). | Metric-label cardinality consumes time-series and memory resources. Exercise cardinality bounds and make overflow/drop behavior observable; select Core-specific limits from measured memory budgets rather than importing another system's default. |
| Basiri et al. (2016), [“Chaos Engineering,” *IEEE Software* 33(3), 35–41](https://doi.org/10.1109/MS.2016.60). | Failure experiments need a stated steady-state hypothesis, controlled blast radius, measurable stop conditions, and recovery verification. |

### Mapping to the Current Code Evidence

- `include/core/concerns/concerns_context.h` returns owning `shared_ptr` snapshots from `resolve<T>()`, while named accessors return references. Concurrency validation should test the supported snapshot/replacement contract rather than assume every access form is safe across replacement.
- `include/core/concerns/context_propagation.h` captures thread-local context for `std::async`; `include/core/concerns/i_tracer.h` exposes header propagation methods. The end-to-end set of transports remains consumer-dependent and is an explicit assumption for Wave 2.
- `include/core/concerns/adapter_registry.h` currently defaults to `AdapterTrustPolicy::kTrustAll`; `src/core/TRUST.md` says the optional adjacent SHA-256 file does not authenticate a publisher. This supports a trust-policy design and test gap, not a claim that a particular deployment is compromised.
- `tests/core/test_core_highcardinality_stress.cpp` tests local `StubConfigStore`, `StubModuleLifecycle`, and `StubSpanExporter`; those tests are useful harness checks but do not measure production Core adapter behavior.
- `benchmarks/core/bench_core_performance.cpp` is registered by `benchmarks/core/CMakeLists.txt`, but its cases cover indexes, query/storage, graph, and timeseries rather than Core concerns. Thus no Core-specific baseline or numerical SLO is asserted here.

The general guidance that time-series cardinality grows with label combinations should not be converted directly into a Core limit. Likewise, the 24-hour v2.5.0 and 72-hour v3.0.0 soak durations and the proposed 10% improvement / 5% regression gates in the roadmap are project acceptance proposals, not thresholds established by these publications; baseline and human approval are prerequisites.
