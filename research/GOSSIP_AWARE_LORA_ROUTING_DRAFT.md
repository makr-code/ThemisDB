# Gossip-Aware LoRA Routing: Federated Fine-Tuning at Scale

**Status**: Draft  
**Version**: 0.2  
**Last Updated**: 2026-04-19  
**Target Venue**: Middleware, ICDCS, SoCC

---

## I. Abstract

Centralized LoRA-adapter registries create bottlenecks and failure concentration in distributed inference fabrics. This paper proposes a gossip-aware routing architecture where adapter capability and freshness metadata propagate via epidemic exchange, and query routing decisions jointly optimize specialization quality and tail-latency constraints. The contribution is a reproducible systems model with explicit evaluation matrices for convergence, quality-latency trade-offs, and fault tolerance. The design is anchored to ThemisDB routing and gossip components and includes claim boundaries that separate repository-validated building blocks from pending large-scale empirical validation.

## II. Problem Statement

Large clusters with domain-specialized LoRA adapters require routing policies that are both freshness-aware and resilient to partial failures. Pure hash routing ignores specialization; centralized registries reduce routing ambiguity but add control-plane fragility.

We target a decentralized alternative with four properties: low-latency local decisions, fast convergence of capability metadata, bounded stale-routing risk, and predictable behavior under node churn and partition-like delays.

The central deployment decision examined in this paper is adapter placement strategy: (a) distribute LoRA adapters broadly across many shards to maximize local hit probability, or (b) keep domain-specialized shards with concentrated adapter portfolios and route requests by affinity.

## III. Research Questions and Hypotheses

RQ1: How quickly does adapter availability and capability information converge under gossip fanout and round-duration settings?

RQ2: How much domain-quality improvement does gossip-aware routing provide versus hash-only and centralized-registry baselines?

RQ3: What is the p95/p99 latency impact of freshness-aware remote forwarding under realistic workload drift?

RQ4: How robust is routing quality under stale metadata, cache eviction pressure, and partial node failures?

RQ5: Under which workload and infrastructure conditions should operators prefer broad adapter distribution over domain-specialized shard placement?

H1: Gossip-aware routing increases domain-hit rate and domain-quality delta while keeping routing overhead within a bounded latency budget.

H2: A stable parameter region exists where convergence is fast enough to keep stale-route incidence low without excessive control-plane traffic.

H3: Broad adapter distribution reduces remote-forward frequency and p99 latency for highly mixed workloads, while domain-specialized shards provide higher domain-quality gains under skewed or stable domain demand.

## IV. Placement Strategies Under Study

We compare two primary strategies and one hybrid fallback:

- Strategy S1 (Distributed Adapters): frequently used adapters are replicated to many shards, prioritizing local execution probability and reduced forwarding overhead.
- Strategy S2 (Domain-Specialized Shards): adapters are concentrated on designated domain shards, prioritizing specialization quality and controlled memory usage.
- Strategy S3 (Hybrid): domain-specialized default with selective replication for top-k high-traffic adapters.

The policy-selection objective is evaluated as:

score = alpha * quality_gain - beta * p99_latency - gamma * replication_cost - delta * stale_route_risk.

Where replication_cost captures memory and transfer overhead from adapter fanout.

## V. Repository Evidence Registry

| Evidence ID | File | Scope | What It Supports |
|-------------|------|-------|------------------|
| E1 | include/sharding/adaptive_shard_router.h | Adaptive routing interfaces | Feasibility of capability-aware route selection |
| E2 | include/distributed_knowledge/adapter_capability_announcement.h | Capability announcement schema | Gossip metadata propagation model |
| E3 | include/sharding/remote_executor.h | Cross-node dispatch path | Remote forwarding and failover execution |
| E4 | tests/test_sharding_gossip.cpp | Gossip/sharding tests | Baseline behavior validation |
| E5 | tests/test_gossip_custom_handler.cpp | Gossip extensibility tests | Handler-level consistency handling |
| E6 | research/GOSSIP_DRIVEN_LORA_DOMAIN_ROUTING_DRAFT.md | Companion design draft | Domain-routing alignment and assumptions |
| E7 | research/RAID_SHARDING_LLM_DISTRIBUTED_INFERENCE.md | Cluster integration context | Compatibility with distributed inference fabric |

Rule: Every major claim in Sections II-VII maps to one or more evidence IDs.

## VI. Experimental Methodology

### A. Setup

We evaluate 4-64 node topologies with configurable gossip fanout, round duration, and TTL. Each node maintains a bounded adapter cache with explicit eviction policy and freshness tracking.

### B. Placement Variants

P1: S1 distributed adapters with low/medium/high replication factor.  
P2: S2 domain-specialized shards with strict affinity routing.  
P3: S3 hybrid policy with adaptive top-k replication.

### C. Workloads

W1: Stationary mixed-domain query distribution with stable adapter popularity.  
W2: Drift workload where dominant domains shift over time.  
W3: Failure workload with node unavailability, stale-announcement injection, and delayed gossip rounds.

### D. Metrics

Primary metrics: domain-hit rate, domain-quality delta, p95/p99 latency, and routing overhead.  
Reliability metrics: stale-route incidence, misroute rate, timeout rate, and failover success rate.  
Control-plane metrics: gossip bandwidth, convergence rounds, cache-churn rate, replication traffic, and memory footprint per shard.

## VII. Results Plan

### A. Reporting Tables and Figure Plan

Table GA1. Routing quality and latency comparison by placement strategy.

| Policy | Placement Strategy | Workload | Domain-Hit Rate | Quality Delta | p95 (ms) | p99 (ms) | Routing Overhead (ms) |
|--------|----------|-----------------|---------------|----------|----------|-----------------------|
| Hash-only | S1/S2/S3 | W1/W2/W3 | pending | pending | pending | pending | pending |
| Central registry | S1/S2/S3 | W1/W2/W3 | pending | pending | pending | pending | pending |
| Gossip-aware | S1/S2/S3 | W1/W2/W3 | pending | pending | pending | pending | pending |

Table GA2. Robustness, convergence, and replication cost outcomes.

| Scenario | Placement Strategy | Fanout | TTL | Convergence Rounds | Stale-Route Incidence | Failover Success Rate | Timeout Rate | Replication Cost |
|----------|--------|-----|--------------------|-----------------------|-----------------------|--------------|
| Stationary | S1/S2/S3 | pending | pending | pending | pending | pending | pending | pending |
| Drift | S1/S2/S3 | pending | pending | pending | pending | pending | pending | pending |
| Failure-injected | S1/S2/S3 | pending | pending | pending | pending | pending | pending | pending |

Figure GA1. Convergence and stale-route trade-off across fanout and TTL configurations.

Figure GA2. Quality-latency-replication Pareto frontier for S1 versus S2 versus S3.

### B. Expected Negative Results

We expect to observe parameter regimes where aggressive freshness thresholds improve quality but increase routing oscillation and tail latency, and regimes where over-replication reduces forwarding overhead but harms memory efficiency.

## VIII. Operational Decision Framework (S1 vs S2 vs S3)

To make placement selection actionable, we define a policy gate that converts measured metrics into a deployment choice.

### A. Decision Inputs

- Domain skew index (DSI): share of top-1 domain traffic over total traffic.
- Remote-forward rate (RFR): fraction of requests requiring cross-shard forwarding.
- Quality gap (QG): domain-quality delta between S2 and S1.
- Replication pressure (RP): memory plus replication bandwidth usage relative to configured budget.

### B. Selection Rules

Use S1 (Distributed Adapters) when DSI is low, RFR dominates p99, and replication pressure is below budget.

Use S2 (Domain-Specialized Shards) when DSI is high, QG is materially positive for specialized shards, and forwarding remains within latency SLO.

Use S3 (Hybrid) when workload drift is frequent or when S1 and S2 alternate superiority across time windows.

### C. Suggested Initial Thresholds

| Signal | Suggest S1 | Suggest S2 | Suggest S3 |
|--------|------------|------------|------------|
| DSI | < 0.45 | > 0.65 | 0.45-0.65 |
| RFR | > 0.30 | < 0.20 | 0.20-0.30 |
| QG (S2-S1) | <= 0.01 | >= 0.03 | 0.01-0.03 |
| RP | <= 0.70 budget | <= 0.85 budget | 0.70-0.85 budget |

These thresholds are initialization values for experiments, not final recommendations.

### D. Online Reconfiguration Cadence

Policy reevaluation window: every 15 minutes for stable workloads, every 5 minutes for drift-heavy workloads.

Guardrail: require two consecutive windows before switching strategy to prevent oscillation.

## IX. Claim Boundaries

**Supported claims:**
- Routing and gossip metadata structures exist and are testable in repository scope (E1-E5).
- Integration assumptions are aligned with broader distributed inference architecture (E6-E7).

**Deferred claims:**
- Large-scale convergence guarantees beyond tested cluster sizes.
- Production-grade fault-campaign outcomes across long-running workloads.
- Universal threshold recommendations independent of workload mix.
- Universal superiority of S1 or S2 independent of domain skew and infrastructure limits.

## X. Discussion

Practical implication: gossip-aware routing should be operated as a dynamic policy surface with periodic recalibration, not as a static routing rule.

Operational constraint: stronger freshness sensitivity can improve quality but may amplify control-plane churn and forwarding variability.

Placement constraint: S1 improves locality but increases memory/replication overhead, while S2 improves specialization efficiency but can increase forwarding under mixed-domain traffic.

### Threats to Validity

Internal validity: synthetic drift and failure patterns may not fully capture production incident distributions; we mitigate via replay traces and scenario diversification.

Construct validity: domain-hit rate is not a complete proxy for semantic quality; we pair it with explicit quality-delta and failure-case analyses.

External validity: hardware/network heterogeneity may alter convergence and latency outcomes; environment metadata must accompany all reported results.

## XI. Next Milestones

- M1: Parameter sweep for fanout, round duration, TTL, and threshold sets.
- M2: Controlled stale-metadata and failover campaign with reproducible scripts.
- M3: S1 vs S2 vs S3 decision study (quality, p99, replication cost).
- M4: v0.3 manuscript with calibrated placement-policy recommendations and initial large-scale data.

## Appendix A. Claim-to-Evidence Traceability

| Claim ID | Claim Summary | Evidence IDs |
|----------|---------------|--------------|
| C1 | ThemisDB exposes adaptive routing and capability announcement structures required for gossip-aware LoRA routing. | E1, E2 |
| C2 | Cross-shard forwarding and gossip behavior can be exercised in current repository test paths. | E3, E4, E5 |
| C3 | The design is compatible with broader distributed-inference architecture assumptions. | E6, E7 |
| C4 | Final production thresholds and convergence guarantees are deferred pending full benchmark campaigns. | E4, E5 |
| C5 | Final decision guidance for distributed adapters vs domain-specialized shards is deferred pending comparative experiments. | E1, E2, E4, E5 |

## Appendix B. Submission Readiness Checklist

- [x] Problem and contribution are technically scoped
- [x] Research questions and hypotheses are explicit
- [x] Evidence registry is documented
- [x] Results schema (tables/figure plan) is defined
- [x] Threats to validity are documented
- [x] Claim-to-evidence traceability is documented
- [ ] Large-scale benchmark results inserted
- [ ] Final artifact manifest and commit hash frozen
