# Cost-Aware Hybrid ANN Retrieval in ThemisDB: HNSW, FAISS, and Multi-Model Query Planning

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-04-19  
**Target Venue**: arXiv (cs.DB / cs.IR)

---

## Abstract

Approximate nearest neighbor (ANN) retrieval has become a core primitive for modern database-assisted AI workloads, yet practical deployments still struggle with planner-level decisions across vector, lexical, and graph operators. This paper presents a cost-aware hybrid retrieval design in ThemisDB, combining HNSW and FAISS-backed vector search with multi-model query planning. Our contribution is not a new ANN index algorithm; instead, we provide a systems-level formulation for selecting retrieval plans under latency, memory, and quality constraints. The paper is repository-grounded: architecture definitions and implementation-influence records already map ANN methods to concrete modules. We define an experimental protocol covering Recall@k, nDCG, QPS, tail latency, build cost, and memory footprint under production-like workloads. The draft contributes a reproducible benchmark blueprint and claim boundaries for fair comparison against single-path retrieval strategies.

## I. Introduction

AI-native database workloads increasingly require retrieval stacks that are both high-recall and latency-bounded. In practice, operators rarely execute pure ANN retrieval in isolation. They combine lexical filters, vector search, and sometimes graph expansion in one end-to-end query path. The quality and performance observed by users are therefore dominated by planner behavior, not by any single index implementation.

Most prior evaluations compare ANN engines under controlled standalone settings. While useful, this misses the systems question that production teams face: which retrieval path should be selected for a given query class under a fixed latency, memory, and quality budget? We argue that this planner-level decision is the primary optimization surface in multi-model database deployments.

This paper introduces a cost-aware hybrid retrieval formulation for ThemisDB. Instead of proposing a new ANN algorithm, we focus on integrated plan selection across lexical, vector, and graph operators and provide a benchmark methodology that reports quality and systems cost together.

### Contributions

1. A hybrid retrieval planning model for vector + lexical + graph operators in one DB runtime.
2. A reproducible benchmark design for quality-latency-memory trade-offs.
3. Evidence-backed mapping from research influences to concrete ThemisDB modules.

### Research Questions and Hypotheses

RQ1: Under which query/workload classes does hybrid planning outperform single-path retrieval on quality-latency trade-offs?

RQ2: How do planner-objective weights shift the Pareto frontier between Recall@k and p99 latency?

RQ3: Which index-parameter regions maximize quality gains without violating memory and build-cost budgets?

H1: Hybrid plans provide statistically significant Recall@k improvements versus vector-only and lexical-only baselines in mixed workloads.

H2: The quality gain of hybrid planning diminishes beyond a workload-specific parameter region due to latency and memory overhead.

## II. Related Work

ANN research has produced strong methods and implementations, including graph-based structures such as HNSW and highly optimized toolchains such as FAISS. Parallel work in hybrid retrieval and rank fusion has shown that lexical and dense signals are often complementary, particularly in noisy or domain-specific corpora.

Database query optimization literature provides rich tools for cost-based operator selection, but many practical retrieval systems still treat ANN configuration as a mostly independent subsystem. As a result, global query costs across lexical, vector, and graph stages are frequently under-optimized.

Our novelty is therefore not an index innovation but a planner-centric systems perspective: we explicitly optimize retrieval path choice in a multi-model DB runtime and evaluate outcomes with both IR quality metrics and systems metrics.

## III. System Model / Architecture

The architecture contains three decision-critical components. The vector index layer exposes ANN alternatives with different latency-memory-recall trade-offs. The hybrid coordinator combines candidate sets across lexical and vector channels (and optionally graph-derived candidates). The query planner computes plan choices based on estimated cost and expected quality.

The capability and implementation scope claimed here is bounded by evidence E1-E4.

Execution follows a staged pipeline: candidate generation, candidate fusion, and optional reranking. The planner can choose one dominant path or a combined path depending on query features and budget constraints.

The primary failure modes are quality collapse under aggressive latency constraints and memory-pressure-induced degradation when index parameters are tuned beyond hardware limits. We treat these not as corner cases but as first-order operating regimes in production.

## IV. Method / Design

For each query class, we estimate expected plan latency, memory cost, and quality risk. The planner selects the plan minimizing a weighted objective that reflects deployment priorities. This objective can be tuned per workload profile, enabling one system to support both latency-critical and quality-critical modes.

We operationalize plan selection as:

objective = alpha * latency + beta * memory + gamma * quality_penalty.

Quality penalty is derived from target recall floors and historical miss behavior for the same query class. In this way, the planner learns not only cost but also risk of unacceptable quality outcomes.

Scaling analysis covers both offline and online dimensions: index build/refresh cost as corpus size grows, and online query latency under varying concurrency. Edge handling includes lexical-only fallback when vector services are unavailable and bounded degradation policies to preserve service continuity.

## V. Implementation Evidence (Repository-Grounded)

| Evidence ID | File | Scope | What It Proves | Status |
|-------------|------|-------|----------------|--------|
| E1 | `docs/research/implementation_influence/by_paper.md` | ADR-001 (HNSW over FAISS) | Architectural choice is accepted and tracked | ready |
| E2 | `docs/research/implementation_influence/by_paper.md` | Malkov/Yashunin entry | HNSW influence mapped to implemented modules | ready |
| E3 | `ARCHITECTURE.md` | Index & Vector layer section | HNSW, FAISS, and hybrid search are architecture-level capabilities | ready |
| E4 | `README.md` | capability matrix | Vector search support communicated at product level | ready |
| E5 | `PERFORMANCE_EXPECTATIONS.md` | v1.8.2 abstract + benchmark summary | Root-level measured performance baseline and index/query gaps | ready |
| E6 | `ARCHITECTURE.md` | single-node benchmark table | CPU/GPU vector-search baseline values for ANN discussion | ready |

Rules:
- Every major claim in Sections III-VII must map to >=1 evidence ID.
- Prefer tests/benchmarks over comments as claim support.

## VI. Experimental Methodology

### A. Setup
We evaluate on CPU-only and GPU-accelerated profiles to separate planner effects from accelerator effects. Hardware metadata is captured per run so latency and throughput comparisons remain interpretable across environments.

Software is pinned by commit and benchmark configuration snapshot. Datasets include text and embedding corpora with multiple dimensionalities and filter selectivities to reflect realistic mixed retrieval conditions.

Reproducibility controls include fixed random seeds, repeated trials, warm-up phases, and strict separation of index-build and query-time measurement windows.

### B. Workloads
W1 stresses vector-dominant semantic retrieval. W2 emphasizes lexical filtering followed by semantic reranking. W3 combines lexical, vector, and graph-aware expansion in one pipeline.

Each workload is executed under multiple planner objectives so we can compare policy behavior across latency-critical and quality-critical modes.

### C. Metrics
Primary system metrics are p50/p95/p99 latency and throughput. Retrieval quality is measured with Recall@k, nDCG@k, and MRR. Reliability metrics include timeout rate, fallback rate, and degraded-mode utilization.

We also report index build time and memory footprint per plan family to expose offline-online trade-offs.

## VII. Results

### A. Primary Results
Repository baselines provide clear control anchors: vector search shows 5,000 queries/s (p99 15 ms) on CPU and 25,000 queries/s (p99 3 ms) on GPU; overall query p99 baseline is 9.67 ms. These values bound plausible gains for planner-level optimization.

The root report also shows index-ingest and peak-query gaps relative to targets. For hybrid ANN planning, these are not peripheral details, because stale or delayed index refresh can alter quality-cost trade-offs at query time.

Paper-specific result packaging is pre-defined: Table H1 compares plan families by Recall@k and p99; Table H2 reports memory and build costs; Figure H1 plots Pareto fronts across quality and latency objectives. This ensures fair comparison against single-path baselines.

### D. Reporting Tables and Figure Plan

Table H1. Retrieval quality and latency by plan family.

| Plan Family | Workload | Recall@10 | nDCG@10 | MRR | p50 (ms) | p95 (ms) | p99 (ms) | QPS |
|-------------|----------|-----------|---------|-----|----------|----------|----------|-----|
| Vector-only | W1/W2/W3 | pending | pending | pending | pending | pending | pending | pending |
| Lexical-only | W1/W2/W3 | pending | pending | pending | pending | pending | pending | pending |
| Hybrid | W1/W2/W3 | pending | pending | pending | pending | pending | pending | pending |

Table H2. Build and memory cost by configuration.

| Index Config | Build Time | Memory Footprint | Refresh Cost | Timeout Rate | Fallback Rate |
|--------------|------------|------------------|--------------|---------------|----------------|
| Config A | pending | pending | pending | pending | pending |
| Config B | pending | pending | pending | pending | pending |
| Config C | pending | pending | pending | pending | pending |

Figure H1. Pareto frontier of retrieval quality versus p99 latency across planner objectives.

### B. Ablations / Sensitivity
We perform parameter sweeps over efSearch, nprobe, fusion weights, and top-k, including cross-effects with filter selectivity and query difficulty classes.

### C. Negative Results
Negative results will include workloads where hybrid planning adds overhead without sufficient quality gain, as well as regimes where lexical-only or vector-only plans dominate.

## VIII. Discussion

Practical implications: planner policy should be workload-specific, not static.

Operational constraints: memory/latency budget coupling can shift the optimal plan family under real-time load variation.

Measurement scope note: currently available root measurements provide strong vector/query baselines but not yet the full Recall@k/nDCG matrix for every hybrid plan variant in this draft.

### Threats to Validity

Internal validity: plan comparisons can be distorted by warm-cache effects and run-order bias; we mitigate with randomized execution order and explicit warm/cold reporting.

Construct validity: Recall@k and nDCG may not fully represent domain utility; we therefore include workload-specific relevance checks and degraded-mode tracking.

External validity: findings from selected embedding models and corpora may not generalize to all domains; we address this with multiple corpora profiles and parameter-sweep disclosure.

In this section, baseline statements are anchored to E5-E6, while architecture and integration statements map to E1-E4.

### Claim Boundaries

**Supported claims:**
- ThemisDB contains architecture and influence anchors for hybrid ANN retrieval (E1-E4).

**Deferred claims:**
- Superiority margins and generalized recommendations pending full benchmark suite.

## IX. Reproducibility & Artifact

The reproducibility package will pin commit hash, dataset snapshot, and planner-parameter grid. A baseline rerun flow is defined below.

```powershell
# Configure + build benchmark binaries
cmake --preset msvc-ninja-release
cmake --build --preset build-msvc-ninja-release

# Example focused benchmark run (adjust target according to local benchmark setup)
cmake --build build-msvc-ninja-release --target themis_tests
```

Primary artifact anchors are `PERFORMANCE_EXPECTATIONS.md`, `ARCHITECTURE.md`, and validation JSON files such as `artifacts/perf_nv/targeted_validation/bench_vector_search_targeted.json`. Full parameter sweeps typically require 3-12 hours. Known pitfalls are warm-cache bias, run-order effects, and inconsistent GPU occupancy.

## X. Limitations, Risk, Ethics

- Misuse risk: ranking bias from embedding model mismatch.
- Safety/compliance: document model version and retrieval policy for auditability.
- Boundary conditions: low-resource environments may require simplified plans.

## XI. Conclusion

This draft prepares a systems-oriented ANN/hybrid retrieval paper grounded in existing ThemisDB capabilities and clear benchmark methodology. Final contribution strength depends on comprehensive empirical results and reproducible artifact packaging.

## References

1. Y. A. Malkov and D. A. Yashunin, "Efficient and Robust Approximate Nearest Neighbor Search Using Hierarchical Navigable Small World Graphs," IEEE TPAMI, 2020. URL: https://doi.org/10.1109/TPAMI.2018.2889473
2. J. Johnson, M. Douze, and H. Jegou, "Billion-Scale Similarity Search with GPUs," IEEE Transactions on Big Data, 2019. URL: https://arxiv.org/abs/1702.08734
3. M. Aumuller, E. Bernhardsson, and A. Faithfull, "ANN-Benchmarks: A Benchmarking Tool for Approximate Nearest Neighbor Algorithms," SISAP 2017. URL: https://arxiv.org/abs/1807.05614
4. G. V. Cormack, C. L. A. Clarke, and S. Buettcher, "Reciprocal Rank Fusion Outperforms Condorcet and Individual Rank Learning Methods," SIGIR 2009. URL: https://dl.acm.org/doi/10.1145/1571941.1572114
5. C. Chen et al., "ScaNN: Efficient Vector Similarity Search at Scale," 2020. URL: https://arxiv.org/abs/1908.10396
6. Facebook Research, "FAISS," GitHub repository. URL: https://github.com/facebookresearch/faiss
7. ThemisDB Contributors, "ThemisDB," GitHub repository, 2026. URL: https://github.com/makr-code/ThemisDB

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

1. Freeze benchmark datasets and parameter grid.
2. Run plan-comparison experiments.
3. Populate Sections VI-VII with tables and confidence intervals.
4. Finalize artifact and references.

## Appendix C. Claim-to-Evidence Traceability

| Claim ID | Claim Summary | Evidence IDs |
|----------|---------------|--------------|
| C1 | ThemisDB provides architecture-level support for HNSW/FAISS and hybrid retrieval planning. | E1, E2, E3, E4 |
| C2 | Baseline vector/query performance supports planner-level optimization studies. | E5, E6 |
| C3 | Generalized superiority claims across all workloads remain pending full sweep completion. | E5 |
