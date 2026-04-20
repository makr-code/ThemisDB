# ThemisDB as a Distributed ACID Multi-Model AI Database: Architecture, Trade-offs, and Evaluation Plan

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-04-19  
**Target Venue**: arXiv (cs.DB / cs.DC)

---

## Abstract

Modern enterprise AI stacks frequently separate transactional data management and model-driven processing into distinct systems, creating consistency and operational complexity gaps. This paper presents a systems study of ThemisDB as a distributed ACID multi-model database with integrated AI/LLM capabilities. The contribution is an architecture-centric and measurement-driven framework for analyzing trade-offs across consistency, scalability, and AI workload performance. We focus on consensus-backed sharding, transactional semantics (including MVCC and SAGA patterns), and native support for vector/graph/retrieval-aware AI paths. The paper is repository-grounded and provides explicit evidence mapping to architecture and capability definitions. Rather than claiming final benchmark-optimal behavior, we define reproducible evaluation protocols and claim boundaries to support rigorous comparison with decoupled data-plus-serving architectures.

## I. Introduction

Enterprise AI systems often split transactional storage and model serving into separate stacks. This architectural split enables independent scaling, but it also creates consistency, latency, and operational complexity gaps at integration boundaries.

For regulated or mission-critical workloads, these trade-offs are increasingly problematic: systems must provide strict transactional correctness while also supporting retrieval and LLM-assisted processing at production latency.

Existing literature typically emphasizes one side of this boundary, either distributed transaction correctness or AI-serving performance. This paper evaluates their co-existence in one runtime model by studying ThemisDB as a distributed ACID multi-model AI database.

### Contributions

1. A unified systems model for distributed ACID + multi-model + AI-native execution.
2. A trade-off framework connecting consistency choices with AI workload performance.
3. Repository-grounded evidence and reproducibility roadmap for benchmark-driven validation.

### Research Questions and Hypotheses

RQ1: How do consistency policies affect AI-path latency and throughput under mixed distributed workloads?

RQ2: Which shard/replication configurations provide the best balance between fault resilience and end-to-end performance?

RQ3: How much additional overhead is introduced during transition windows after failures compared with steady-state operation?

H1: Mixed transaction+AI workloads can preserve strict correctness while meeting latency SLOs within a bounded coordination-overhead regime.

H2: Transition-window overhead after failures dominates steady-state overhead and must be modeled explicitly for realistic SLO planning.

## II. Related Work

Distributed systems research established robust foundations for consensus, replication, and transactional correctness. Multi-model database work expanded operator diversity across relational, graph, and document/vector paradigms. In parallel, AI-serving research improved throughput and latency for model inference.

Despite this progress, end-to-end evaluations that jointly model strict transactional guarantees and integrated AI paths remain limited. Most practical architectures still rely on cross-system composition, where correctness and AI performance are tuned separately.

Our novelty is a unified systems treatment: we analyze correctness, scalability, and AI-path performance as interacting dimensions in a single distributed runtime.

## III. System Model / Architecture

The model is composed of four layers. The distributed layer handles consensus, replication, and shard placement. The transaction layer provides MVCC and distributed transaction orchestration. The multi-model layer executes relational, graph, and vector operators. The AI layer adds retrieval-aware LLM paths.

The co-presence and capability claims in this section are anchored to evidence E1-E4.

We assume heterogeneous workloads and node capabilities, reflecting realistic cluster operation. Queries can traverse multiple layers in one request path, making cross-layer contention an expected behavior rather than an exception.

The failure model covers node loss, partial network partition, and contention bursts. We evaluate both steady-state behavior and degraded regimes, because many practical incidents are dominated by transition dynamics rather than static throughput limits.

## IV. Method / Design

The design principle is shared primitives: data and AI workloads use the same storage, query, and coordination substrate. This removes cross-system synchronization boundaries but introduces explicit trade-offs between correctness guarantees and tail-latency objectives.

Decision policies map workload class and risk profile to consistency and routing strategy. For example, stricter policies are applied to correctness-critical operations, while latency-sensitive analytical paths may use less restrictive strategies when acceptable.

Scaling analysis characterizes both horizontal shard gains and coordination overhead growth. Edge-case design includes deterministic failover handling and compensation workflows for multi-step distributed operations.

## V. Implementation Evidence (Repository-Grounded)

| Evidence ID | File | Scope | What It Proves | Status |
|-------------|------|-------|----------------|--------|
| E1 | `README.md` | capability matrix | ACID, distributed, and AI/LLM capabilities co-declared | ready |
| E2 | `ARCHITECTURE.md` | Distributed & Sharding section | Raft/Paxos/Gossip and distributed coordination are architectural core | ready |
| E3 | `ARCHITECTURE.md` | Transaction section | MVCC and SAGA are explicit transaction features | ready |
| E4 | `ARCHITECTURE.md` | Index/LLM/RAG sections | Multi-model plus AI integration within one architecture | ready |
| E5 | `PERFORMANCE_EXPECTATIONS.md` | v1.8.2 abstract + benchmark summary | Root-level measured throughput/latency and explicit open performance gaps | ready |
| E6 | `ARCHITECTURE.md` | single-node benchmark table + scalability section | Baseline and scalability targets for distributed systems discussion | ready |

Rules:
- Every major claim in Sections III-VII must map to >=1 evidence ID.
- Prefer tests/benchmarks over comments as claim support.

## VI. Experimental Methodology

### A. Setup
Evaluation uses 3-9 node cluster profiles with controlled heterogeneity scenarios (uniform nodes versus mixed-capability nodes). Hardware and network metadata are recorded per run to interpret failover and coordination effects correctly.

Software state is pinned by commit hash, cluster configuration bundle, and workload-generator version. Dataset design combines transactional records with retrieval-oriented content so cross-layer request paths are exercised in one benchmark protocol.

Reproducibility controls include deterministic workload generation, repeated trials, fixed failure-injection schedules, and synchronized clock/telemetry collection across nodes.

### B. Workloads
W1 is transaction-heavy and establishes correctness and throughput baselines for distributed ACID paths. W2 is retrieval/AI-heavy and stresses vector/graph/LLM-associated operators. W3 combines both workloads under failure injection (node loss, delayed links, transient partitions).

All workloads are run across shard-count and replication-policy sweeps to quantify scaling and coordination overhead.

### C. Metrics
Core metrics include p50/p95/p99 latency, throughput, and tail-amplification under mixed load. AI-path quality metrics include retrieval quality and response quality checks. Reliability metrics include failover time, abort ratio, recovery convergence time, and detected consistency violations.

We report both steady-state and transition-window statistics because many production failures are dominated by transient behavior.

## VII. Results

### A. Primary Results
Repository baselines confirm strong single-node module performance and therefore provide a robust anchor for distributed extrapolation. Query, graph, and time-series values indicate that core primitives are not the primary bottleneck in isolation.

At the same time, open throughput gaps in index insert and query peak performance highlight where distributed coordination overhead could compound existing pressure points. This informs where to prioritize deep instrumentation during mixed-workload runs.

Result schema is predefined: Table D1 reports throughput/latency by shard count and workload class; Table D2 reports failover and recovery metrics under fault injection; Figure D1 plots consistency-latency trade-offs across policy settings.

### D. Reporting Tables and Figure Plan

Table D1. Throughput and latency by shard count and workload class.

| Shard Count | Workload | Throughput | p50 (ms) | p95 (ms) | p99 (ms) | Abort Rate |
|-------------|----------|------------|----------|----------|----------|------------|
| 3 | W1/W2/W3 | pending | pending | pending | pending | pending |
| 6 | W1/W2/W3 | pending | pending | pending | pending | pending |
| 9 | W1/W2/W3 | pending | pending | pending | pending | pending |

Table D2. Fault-injection reliability outcomes.

| Fault Scenario | Consistency Policy | Failover Time | Recovery Convergence | Consistency Violations | AI-Path p99 Delta |
|----------------|--------------------|---------------|----------------------|------------------------|-------------------|
| Node Loss | pending | pending | pending | pending | pending |
| Link Delay | pending | pending | pending | pending | pending |
| Partial Partition | pending | pending | pending | pending | pending |

Figure D1. Consistency-latency trade-off frontier across workload classes and fault scenarios.

### B. Ablations / Sensitivity
Sensitivity sweeps include shard count, replication policy, and isolation settings, with emphasis on interaction effects during fault transitions.

### C. Negative Results
Negative findings will explicitly report conditions where stricter consistency substantially harms AI-path latency without commensurate correctness benefits for the target workload class.

## VIII. Discussion

Practical implications: integrated architecture is viable but requires policy-driven adaptation.

Operational constraints: balancing correctness guarantees with AI-serving SLOs requires explicit policy selection by workload class and risk level.

Measurement scope note: current measured values are strong single-node and module-level baselines; full distributed mixed-workload wave measurements remain the decisive next step for final claims.

### Threats to Validity

Internal validity: distributed measurements are sensitive to failure-injection timing and background infrastructure noise; we mitigate with repeated synchronized trials and transition-window isolation.

Construct validity: consistency and quality objectives can conflict under mixed load; we therefore report both correctness and AI-path performance metrics in the same result matrix.

External validity: cluster topology and network characteristics vary across deployments; we include topology manifests and fault-scenario metadata for transferability analysis.

In this section, baseline performance interpretation maps to E5-E6, while integrated-architecture scope maps to E1-E4.

### Claim Boundaries

**Supported claims:**
- Repository evidence confirms the architectural co-presence of distributed, transactional, multi-model, and AI layers (E1-E4).

**Deferred claims:**
- Quantified superiority versus decoupled architectures pending full comparative benchmarks.

## IX. Reproducibility & Artifact

The final artifact bundle will pin commit hash, cluster topology manifests, and failure-injection scripts. Baseline rerun flow:

```powershell
# Configure + build
cmake --preset msvc-ninja-release
cmake --build --preset build-msvc-ninja-release

# Optional: execute core tests before distributed benchmark wave
$env:PATH = "C:\Projects\ThemisDB\build-msvc-ninja-release\bin;C:\Projects\ThemisDB\build-msvc-ninja-release\cmake;" + $env:PATH
.\build-msvc-ninja-release\bin\themis_tests.exe --gtest_color=yes
```

Artifact anchors include root baseline documentation plus JSON validation outputs in `artifacts/perf_nv/targeted_validation/` and `artifacts/perf_nv/repro_validation_20260412_211053/`. End-to-end distributed runs typically require 6-24 hours depending on fault scenario matrix size. Known pitfalls are noisy failover timing and shared-infrastructure interference.

## X. Limitations, Risk, Ethics

- Misuse risk: incorrect policy tuning can sacrifice either consistency or service quality.
- Safety/compliance: auditability and policy transparency required in regulated domains.
- Boundary conditions: extreme-scale deployments may need specialized control-plane tuning.

## XI. Conclusion

This draft establishes a publication-ready structure for evaluating ThemisDB as a distributed ACID multi-model AI database. The architecture and evidence anchors are present; the remaining work is end-to-end comparative benchmarking and artifact hardening.

## References

1. D. Ongaro and J. Ousterhout, "In Search of an Understandable Consensus Algorithm (Raft)," 2014. URL: https://raft.github.io/raft.pdf
2. L. Lamport, "The Part-Time Parliament," ACM TOCS, 1998. URL: https://dl.acm.org/doi/10.1145/279227.279229
3. H. Garcia-Molina and K. Salem, "Sagas," SIGMOD 1987. URL: https://dl.acm.org/doi/10.1145/38713.38742
4. A. Fekete et al., "Making Snapshot Isolation Serializable," ACM TODS, 2005. URL: https://dl.acm.org/doi/10.1145/1071610.1071615
5. M. J. Cahill, U. Rohm, and A. Fekete, "Serializable Isolation for Snapshot Databases," SIGMOD 2008. URL: https://dl.acm.org/doi/10.1145/1376616.1376690
6. D. Karger et al., "Consistent Hashing and Random Trees: Distributed Caching Protocols for Relieving Hot Spots on the World Wide Web," STOC 1997. URL: https://dl.acm.org/doi/10.1145/258533.258660
7. M. Stonebraker et al., "MapReduce and Parallel DBMSs: Friends or Foes?," Communications of the ACM, 2010. URL: https://doi.org/10.1145/1629175.1629198
8. ThemisDB Contributors, "ThemisDB," GitHub repository, 2026. URL: https://github.com/makr-code/ThemisDB

---

## Appendix A. arXiv Submission Readiness Checklist

- [x] Title is specific and technically scoped
- [x] Abstract states measurable contribution
- [x] All headline claims are evidence-backed
- [x] Related work includes closest baselines and novelty delta
- [x] Method and assumptions are explicitly stated
- [ ] Experimental setup is reproducible
- [x] Limitations and threat model are transparent
- [x] Figures/tables are referenced in text
- [x] References are complete and consistent
- [ ] Artifact path and commit hash documented

## Appendix B. Quick Start for ThemisDB Drafts

1. Finalize distributed workload and failure-injection harness.
2. Run mixed workload benchmarks at multiple shard counts.
3. Populate results with statistical reporting.
4. Lock artifact metadata and finalize references.

## Appendix C. Claim-to-Evidence Traceability

| Claim ID | Claim Summary | Evidence IDs |
|----------|---------------|--------------|
| C1 | ThemisDB architecture co-locates distributed ACID, multi-model operators, and AI execution paths. | E1, E2, E3, E4 |
| C2 | Measured baselines provide a credible anchor for distributed mixed-workload evaluation. | E5, E6 |
| C3 | Quantified superiority versus decoupled architectures remains deferred pending full comparative wave runs. | E5 |
