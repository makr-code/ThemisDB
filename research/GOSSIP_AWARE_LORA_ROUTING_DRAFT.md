# Gossip-Aware LoRA Routing in ThemisDB: Repository-Grounded Review and Evaluation Plan

**Status**: In Review
**Version**: 1.0
**Last Updated**: 2026-05-18
**Scope**: ThemisDB open-source repository state at review time

---

## Abstract / Zusammenfassung

This document reviews the technical feasibility of gossip-aware LoRA routing in ThemisDB and separates implemented capabilities from open validation work. The repository already contains production code for capability announcements (`AdapterCapabilityAnnouncement`), domain-aware route selection (`AdaptiveShardRouter::routeByDomain`), and AQL-side domain-hint propagation in `LLMAQLHandler`. Unit and integration tests cover core control-flow behavior (handler dispatch, capability updates, domain routing, and fallback behavior). However, no dedicated benchmark currently quantifies convergence, stale-route incidence, or quality/latency trade-offs for gossip-aware LoRA routing. Therefore, this manuscript claims implementation readiness of core plumbing, but not production performance superiority.

## Introduction / Einleitung

ThemisDB positions itself as a multi-model database with native AI/LLM integration and strong transactional guarantees (MVCC, SSI, 2PC, SAGA, HLC-based ordering) [R1]. In this architecture, routing requests to shards with better domain-specialized LoRA adapters is a plausible optimization, but only if routing metadata can be propagated and consumed without creating a centralized control-plane bottleneck.

This review focuses on one concrete question: **Does the current repository already implement the technical building blocks needed for gossip-aware LoRA routing, and what is still missing for publication-grade empirical claims?**

Narrative structure of this review:
1. Problem and implemented approach (Methodology)
2. Current validation evidence and measurable gaps (Evaluation)
3. Explicit boundaries and risks (Limitations)

## Methodology / Ansatz

### 1) Repository evidence and fact-checked implementation status

| ID | Artifact | Fact-checked statement |
|---|---|---|
| E1 | `include/distributed_knowledge/adapter_capability_announcement.h` | Defines the gossip payload schema for adapter capability announcements (domain type, accuracy/latency deltas, timestamps). |
| E2 | `src/distributed_knowledge/adapter_capability_announcement.cpp` | Publishes messages tagged `message_type = "adapter_capability"`. |
| E3 | `include/sharding/adaptive_shard_router.h`, `src/sharding/adaptive_shard_router.cpp` | Implements `updateAdapterCapability`, `routeByDomain`, and tie-breaking by pending LLM queue load. |
| E4 | `src/aql/llm_aql_handler.cpp` | AQL inference path parses `domain_hint` options and can route via `AdaptiveShardRouter` with minimum accuracy-delta gating. |
| E5 | `tests/test_gossip_custom_handler.cpp` | Verifies custom gossip handler dispatch and membership-gate behavior. |
| E6 | `tests/test_adaptive_shard_router.cpp` | Verifies adaptive router config validation, fallback behavior, iteration behavior, and stats output. |
| E7 | `tests/test_distributed_knowledge_integration.cpp` | End-to-end integration scenario includes domain-aware routing via capability announcements. |
| E8 | `tests/test_sharding_gossip.cpp` | Verifies baseline gossip configuration defaults and enable/disable semantics. |
| E9 | `benchmarks/bench_shard_routing.cpp` | Benchmarks general shard-routing and consistent-hash performance, but not gossip-aware LoRA routing convergence/quality. |

### 2) Terminology normalization used in this document

- **AQL**: Themis query layer interface (query layer in architecture documentation) [R1].
- **Multi-model**: relational + graph + vector + document + geospatial + time-series in one engine [R1].
- **Consistency/transaction model**: ACID with MVCC/SSI/2PC/SAGA/HLC components as documented by repository root docs [R1].
- **Gossip-aware LoRA routing** (this review): routing decisions influenced by `AdapterCapabilityAnnouncement` data propagated through gossip.
- **Fallback routing**: local/default behavior when no qualifying domain route exists (e.g., low accuracy delta or no match).

### 3) Claim model used for review-readiness

**Supported now (repository-backed):**
- Capability announcement schema and publisher/consumer integration points exist (E1-E3).
- AQL inference flow can carry domain hints into routing decisions (E4).
- Core behavior is covered by unit/integration tests for routing and gossip mechanics (E5-E8).

**Not yet supported as empirical result:**
- Quantified superiority of gossip-aware routing versus baselines for quality, p95/p99 latency, or convergence.
- Production-grade threshold recommendations that generalize across workloads and cluster sizes.

## Evaluation / Experimente

### A) What is validated today

Current test evidence validates **correctness of control flow**, not cluster-scale performance:
- Gossip handler registration and dispatch paths are tested (E5).
- Domain routing based on announced capability deltas is tested (E6, E7).
- Fallback and guard conditions (no match / low quality) are represented in routing logic and tests (E4, E6).

### B) What is missing for publication-level performance claims

A dedicated benchmark harness for gossip-aware LoRA routing is currently absent. Existing sharding benchmarks focus on hash routing and generic routing throughput/latency (E9), so they cannot substantiate claims such as:
- convergence rounds under fanout/TTL settings,
- stale-route incidence,
- quality-latency Pareto behavior of domain-aware routing vs baseline routing.

### C) Concrete experiment plan required before claiming performance gains

To move from architecture validation to empirical claims, the following minimum campaign is required:

1. **Baseline comparison**
   - Policies: hash-only, centralized registry (if implemented), gossip-aware routing.
   - Metrics: domain-hit rate, p95/p99 latency, routing overhead.

2. **Control-plane robustness**
   - Scenarios: stale announcements, delayed gossip rounds, node churn/failure.
   - Metrics: convergence rounds, stale-route incidence, failover success rate.

3. **Quality/efficiency trade-off**
   - Vary accuracy-delta threshold and TTL.
   - Report replication/memory pressure and latency variance.

Until these measurements exist in benchmark artifacts, this paper must remain explicit that routing-quality improvements are **hypotheses**, not measured outcomes.

## Limitations / Known Issues

1. **Benchmark gap**: No repository benchmark currently isolates gossip-aware LoRA routing quality/latency/convergence behavior (E9).
2. **External validity gap**: Current evidence is mostly unit/integration level; cluster-scale heterogeneity (network, hardware, workload skew) is not yet quantified.
3. **Threshold calibration gap**: The minimum accuracy-delta gate is implemented in code paths, but globally valid threshold recommendations are not yet justified by published benchmark data.
4. **Terminology risk in prior drafts**: Earlier wording mixed implemented behavior with planned evaluation outcomes; this revision separates both explicitly.

## Conclusion

ThemisDB already provides the core implementation hooks required for gossip-aware LoRA routing (announcement schema, router integration, AQL domain-hint path, and test coverage). What is still missing is not basic feasibility, but benchmark-grade empirical validation. Consequently, the technically correct current claim is: **implementation anchors exist and are testable; production-performance superiority remains to be demonstrated with dedicated experiments.**

## References / Quellen

- **[R1]** ThemisDB README (architecture and capability overview). [GitHub](https://github.com/makr-code/ThemisDB/blob/main/README.md)
- **[R2]** ThemisDB `AdaptiveShardRouter` header. [GitHub](https://github.com/makr-code/ThemisDB/blob/main/include/sharding/adaptive_shard_router.h)
- **[R3]** ThemisDB adapter capability announcement header. [GitHub](https://github.com/makr-code/ThemisDB/blob/main/include/distributed_knowledge/adapter_capability_announcement.h)
- **[R4]** ThemisDB AQL LLM handler implementation. [GitHub](https://github.com/makr-code/ThemisDB/blob/main/src/aql/llm_aql_handler.cpp)
- **[R5]** ThemisDB distributed knowledge integration tests. [GitHub](https://github.com/makr-code/ThemisDB/blob/main/tests/test_distributed_knowledge_integration.cpp)
- **[R6]** Hu et al., "LoRA: Low-Rank Adaptation of Large Language Models". arXiv:2106.09685. [arXiv](https://arxiv.org/abs/2106.09685)
- **[R7]** McMahan et al., "Communication-Efficient Learning of Deep Networks from Decentralized Data" (FedAvg). PMLR 54:1273-1282. [PMLR](https://proceedings.mlr.press/v54/mcmahan17a.html)
- **[R8]** Demers et al., "Epidemic Algorithms for Replicated Database Maintenance". DOI: [10.1145/41840.41841](https://doi.org/10.1145/41840.41841)
- **[R9]** Kermarrec et al., "Gossiping in Distributed Systems". DOI: [10.1145/1089733.1089737](https://doi.org/10.1145/1089733.1089737)
- **[R10]** Kwon et al., "Efficient Memory Management for Large Language Model Serving with PagedAttention" (vLLM). arXiv:2309.06180. [arXiv](https://arxiv.org/abs/2309.06180)
