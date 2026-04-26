# Gossip-Driven LoRA Domain Routing for Distributed Inference Fabrics

**Status**: Draft  
**Version**: 0.2  
**Last Updated**: 2026-04-19  
**Target Venue**: Middleware, ICDCS, SoCC

---

## I. Abstract

Distributed inference clusters increasingly host domain-specialized LoRA adapters on different shards. Static hash-based routing ignores these specialization gradients and can degrade both answer quality and latency SLOs. This paper proposes a gossip-driven domain-routing model in which shards continuously publish adapter capability signals and routers select destinations using a joint quality-latency objective. The contribution is a reproducible systems design and evaluation plan, grounded in ThemisDB repository components for adaptive routing, capability announcements, and distributed request execution. We define measurable hypotheses, result-table schemas, and explicit claim boundaries to separate validated implementation anchors from pending large-scale empirical results.

## II. Problem Statement

Static sharding and topology-only routing assume homogeneous model quality across shards. In practice, shards accumulate different LoRA specializations and therefore exhibit domain-dependent quality and latency behavior.

A production-grade routing policy must satisfy four constraints: low control-plane overhead, rapid capability convergence, graceful degradation under stale information and failures, and transparent trade-offs between quality gains and tail-latency risk. We target these constraints without introducing centralized coordination dependencies.

## III. Research Questions and Hypotheses

RQ1: How quickly does adapter capability knowledge converge under gossip dissemination for cluster sizes from 4 to 64 shards?

RQ2: What quality and latency gains does domain-aware routing provide compared with hash-only routing under mixed-domain workloads?

RQ3: How robust is the policy under stale announcements, partial failures, and shifting workload distributions?

RQ4: How sensitive are outcomes to TTL and score-threshold configurations?

H1: Domain-aware gossip routing improves domain-hit rate and quality metrics versus hash-only routing while keeping routing overhead bounded.

H2: There exists a stable parameter region where capability staleness remains below tolerance and p99 latency remains within SLO budget despite partial failures.

## IV. Repository Evidence Registry

| Evidence ID | File | Scope | What It Supports |
|-------------|------|-------|------------------|
| E1 | include/sharding/adaptive_shard_router.h | Routing policy and scoring hooks | Domain-aware routing implementation anchor |
| E2 | include/distributed_knowledge/adapter_capability_announcement.h | Capability message schema | Gossip-propagated adapter metadata |
| E3 | include/sharding/remote_executor.h | Cross-shard invocation path | Distributed request dispatch feasibility |
| E4 | tests/test_sharding_gossip.cpp | Gossip routing tests | Baseline correctness and behavior checks |
| E5 | tests/test_gossip_custom_handler.cpp | Custom gossip handler tests | Extensibility and propagation handling |
| E6 | research/GOSSIP_AWARE_LORA_ROUTING_DRAFT.md | Prior design context | Conceptual lineage and assumptions |
| E7 | research/RAID_SHARDING_LLM_DISTRIBUTED_INFERENCE.md | Integration architecture context | Compatibility with broader distributed inference design |

Rule: Every major claim in Sections II-VII maps to one or more evidence IDs.

## V. Experimental Methodology

### A. Setup

We evaluate 4-64 shard configurations with synthetic and replay-based mixed-domain traffic. Capability announcements are emitted at configurable intervals with explicit TTL, and router decisions are logged with per-request metadata.

### B. Workloads

W1: Stationary mixed-domain workload with balanced domain shares.  
W2: Drift workload where dominant domain changes over time.  
W3: Failure workload with stale announcements, shard unavailability, and intermittent partition-like delays.

### C. Metrics

Primary metrics: domain-hit rate, quality delta per domain, p95/p99 end-to-end latency, routing overhead, and fallback rate.  
Reliability metrics: stale-route incidence, misroute rate, failover success rate, and timeout rate.

## VI. Results Plan

### A. Reporting Tables and Figure Plan

Table G1. Routing quality and latency by workload.

| Policy | Workload | Domain-Hit Rate | Quality Delta | p95 (ms) | p99 (ms) | Routing Overhead (ms) |
|--------|----------|-----------------|---------------|----------|----------|-----------------------|
| Hash-only | W1/W2/W3 | pending | pending | pending | pending | pending |
| Domain-aware gossip | W1/W2/W3 | pending | pending | pending | pending | pending |

Table G2. Robustness under stale and failure conditions.

| Scenario | TTL | Threshold Set | Misroute Rate | Failover Success Rate | Timeout Rate | Notes |
|----------|-----|---------------|---------------|-----------------------|--------------|-------|
| Stale announcements | pending | pending | pending | pending | pending | pending |
| Partial shard failure | pending | pending | pending | pending | pending | pending |
| Partition-like delay | pending | pending | pending | pending | pending | pending |

Figure G1. Convergence time versus cluster size and gossip fanout.

### B. Expected Negative Results

We explicitly report regimes where aggressive thresholding improves quality but causes unacceptable p99 inflation, and regimes where short TTL improves freshness but increases control-plane churn.

## VII. Claim Boundaries

**Supported claims:**
- Router and capability announcement structures exist and are testable (E1-E5).
- Integration with distributed inference architecture is conceptually aligned (E6-E7).

**Deferred claims:**
- Large-scale convergence evidence for cluster sizes above 32 shards.
- Production-like multi-fault campaign outcomes.
- Final threshold recommendations for all workload classes.

## VIII. Discussion

Practical implication: domain-aware routing should be treated as a dynamic control policy, not a one-time configuration.

Operational constraint: capability freshness and routing stability are coupled through TTL and threshold parameters.

### Threats to Validity

Internal validity: synthetic workload composition may under-represent real tenant skew; we mitigate with replay traces and drift scenarios.

Construct validity: domain-hit rate alone does not ensure semantic quality; we pair it with quality deltas and failure-case audits.

External validity: cluster-network and hardware heterogeneity can affect convergence and tail latency; we therefore report environment metadata per run.

## IX. Next Milestones

- M1: Parameter study for TTL and threshold sets.
- M2: Stale-information and failover experiment suite.
- M3: Joint quality-versus-latency budget calibration.
- M4: v0.3 with calibrated decision function and initial large-scale results.

## Appendix A. Claim-to-Evidence Traceability

| Claim ID | Claim Summary | Evidence IDs |
|----------|---------------|--------------|
| C1 | ThemisDB exposes the required structures for capability-aware routing decisions. | E1, E2 |
| C2 | Cross-shard routing execution and test anchors already exist in the repository. | E3, E4, E5 |
| C3 | The routing approach is consistent with broader distributed inference architecture assumptions. | E6, E7 |
| C4 | Production-level threshold recommendations are deferred pending full-scale benchmarks. | E4, E5 |

## Appendix B. Submission Readiness Checklist

- [x] Problem and contribution are technically scoped
- [x] Research questions and hypotheses are explicit
- [x] Repository evidence registry is present
- [x] Results schema (tables/figure plan) is defined
- [x] Threats to validity are documented
- [x] Claim-to-evidence traceability is documented
- [ ] Large-scale benchmark results inserted
- [ ] Final artifact manifest and commit hash frozen
