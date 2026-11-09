---
Author: "ThemisDB Team"
Created: "2026-11-09"
Last Updated: "2026-11-09"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Wave D Execution Evidence Blockers

**Review date:** 2026-11-09
**Source revision reviewed:** `74945e87`
**Disposition:** No qualifying current-revision D2 or soak execution evidence was available. This record documents blockers; it is not test evidence or a gate pass.

## D2 observability

| Area | Source review | Gate status |
| --- | --- | --- |
| Distributed tracing | `tests/observability/test_otel_exporter_stress.cpp` includes cases constructing spans with a null exporter; span creation alone does not prove pipeline delivery or recovery. | Pending current-revision integration and performance results. |
| Metrics | `tests/observability/test_high_cardinality_metrics.cpp` uses estimated manager memory, and its purported 50k case repeats a smaller set of shard labels rather than exercising 50k unique dimensions. | Pending measured cardinality, process resource, throughput, and latency evidence. |
| Exporter reliability | `tests/observability/test_otel_exporter_stress.cpp` simulates delay for recovery and counts constructed spans in throughput cases; it does not demonstrate a real network failure/recovery, exporter flush, or delivered-span loss rate. | Pending real exporter execution and archived telemetry artifacts. |

The Phase 2A/2B/2C verification documents are plans or unverified estimates, not records of successful current-target execution. No current run URL, artifact bundle, or human review was attached at this review.

## Four required soak scenarios

| Scenario/source | Source-level limitation | Gate status |
| --- | --- | --- |
| `benchmarks/wave9/test_wave9_otel_24h_soak.cpp` | Default rate is 500 spans/sec, below the 1,000 spans/sec target; recovery is assigned as a placeholder rather than verified against an exporter. | Not executed; source is not acceptance evidence. |
| `benchmarks/wave9/test_wave9_replication_48h_soak.cpp` | Simulates lag and failover timing; does not execute WAL shipping or verify replicated data consistency. | Not executed; source is not acceptance evidence. |
| `benchmarks/wave9/test_wave9_sharding_topology_soak.cpp` | Uses sleeps in place of distributed writes/topology rebalancing; exactness validation is a placeholder. | Not executed; source is not acceptance evidence. |
| `benchmarks/wave9/test_wave9_mixed_acceleration_soak.cpp` | Uses sleeps in place of GPU kernels and fallback; cannot establish real fallback latency, CPU degradation, or GPU resource-leak behavior. | Not executed; source is not acceptance evidence. |

All four 24/48-hour runs require production-representative workloads, full-duration measurements, injected-failure outcomes, and archived logs/metrics/traces. A shortened smoke run or a PASS emitted by these sources cannot satisfy those criteria.

## Closure prerequisites

- Replace or complete the source workloads and assertions so they exercise the real exporter, WAL replication, sharding, and GPU/CPU fallback paths.
- Execute D2 on the target revision and attach run URL/ID, commit SHA, configuration, measurements, failure/recovery results, and archived artifacts.
- Execute each soak for its required 24/48-hour duration on representative infrastructure and record all scenario acceptance metrics.
- Keep Wave D pending until D1/RUN-01 drill evidence, approved recovery objectives, operations and architecture reviews, authorized release approval, and upstream Wave A–C gates are also satisfied.

The proposals in `audit/ROADMAP.md` for WS8 RPO/RTO remain proposals and are not acceptance gates without explicit maintainer approval.
