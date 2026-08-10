# Cross-Modal Cardinality Estimation and Cost Models for AQL, Graph, Vector, and Geo Queries

**Status**: ACTIVE_DRAFT  
**Version**: 0.1  
**Last Updated**: 2026-08-10  
**Target Venue**: SIGMOD 2027 / VLDB 2027 / ICDE 2027

---

## Metadata

- **Scientific Delta**: Isolate cross-modal selectivity and cost estimation as a first-class problem in ThemisDB's AQL engine instead of treating hybrid plan choice as a minor optimizer detail.
- **Canonical Evidence Sources**: `src/query/README.md`, `research/implementation_influence/by_module.md`, `COST_AWARE_HYBRID_RETRIEVAL_PLANNING_AQL_DRAFT.md`, `HYBRID_ANN_RETRIEVAL_SYSTEMS_PAPER_DRAFT.md`.
- **Required Experiments**: shared workloads across graph/vector/geo/temporal predicates, estimator error analysis, plan-quality comparison against single-modal heuristics.
- **Open Risks / Claim Boundaries**: current evidence establishes optimizer scope and hybrid-plan intent, but a dedicated cross-modal benchmark matrix still needs to be frozen.
- **Overlap / Successor / Predecessor**: complements the hybrid retrieval planning paper; should focus on estimation/modeling novelty instead of end-to-end retrieval benchmarking.

## Abstract

ThemisDB's query engine already exposes hybrid query strategies spanning vector, geo, graph, and fulltext workloads. Yet the scientific problem of cross-modal cardinality estimation is broader than any single retrieval path: it concerns how heterogeneous selectivity signals can be normalized into a cost model that chooses stable plans across fundamentally different operator families. This manuscript packages that problem as a standalone database research topic. Current repository evidence supports the presence of hybrid operators, optimizer hooks, and explicit cardinality estimation machinery, while the missing piece is a reproducible experiment suite that compares estimator quality and plan outcomes across modalities.

## I. Introduction

Hybrid multi-model queries are easy to advertise but hard to optimize. A DBMS that combines graph traversal, vector similarity, geospatial filtering, and traditional predicates needs a principled way to compare incomparable selectivity signals. ThemisDB's AQL engine already documents these operator classes, making it a strong basis for a cross-modal estimation paper.

### Contributions

1. A problem framing for cross-modal selectivity normalization in AQL.
2. A unified estimator/cost-model design space for graph, vector, geo, and temporal operators.
3. A benchmark plan for plan-quality and estimator-error evaluation across hybrid workloads.

## II. Related Work

- classical cardinality estimation
- learned query optimization and adaptive cost models
- hybrid retrieval planning and ANN-aware query optimization
- novelty delta: treat vector/graph/geo/temporal selectivity interaction as one optimizer problem

## III. System Model / Repository Scope

- query parser and plan execution: `src/query/README.md`
- cost model entry points: `optimizer_cost_model.cpp`, `query_optimizer.cpp`, `adaptive_optimizer.cpp`
- hybrid paths: vector+geo, fulltext+geo, graph traversal with constraints

## IV. Method / Design

- define per-operator selectivity representations
- define calibration and confidence envelopes across modalities
- define plan-choice objective combining latency, throughput, and relevance-sensitive quality proxies

## V. Repository-Grounded Evidence

| Evidence ID | File | Scope | Claim anchor | Status |
|---|---|---|---|---|
| E1 | `src/query/README.md` | Relevant Interfaces / Query Optimizer | explicit cardinality estimation, hybrid strategies, optimizer cost model | ready |
| E2 | `research/implementation_influence/by_module.md` | `src/query/` row | optimizer and cost-model work already tracked as a research-backed capability | ready |
| E3 | `COST_AWARE_HYBRID_RETRIEVAL_PLANNING_AQL_DRAFT.md` | draft scope | hybrid planning problem framing already exists | ready |
| E4 | `HYBRID_ANN_RETRIEVAL_SYSTEMS_PAPER_DRAFT.md` | retrieval systems scope | retrieval-system baselines for companion comparison | ready |

## VI. Experimental Methodology

### A. Setup
- standardized hybrid AQL workload family
- per-modality statistics collection
- plan logging with replay support

### B. Workloads
- W1: vector + geo filters
- W2: graph traversal + attribute predicates
- W3: lexical + vector + geo hybrid retrieval

### C. Metrics
- cardinality estimation error
- plan regret / wrong-plan frequency
- p95/p99 latency and throughput
- relevance-sensitive metrics where retrieval quality matters

## VII. Results

### A. Primary Results
- repository evidence currently supports the optimizer scope and operator diversity claim
- estimator comparison results remain pending

### B. Ablations / Sensitivity
- per-modality vs unified estimator
- static vs adaptive calibration

### C. Negative Results
- no frozen cross-modal benchmark matrix yet

## VIII. Discussion

This paper has strong venue fit because it converts ThemisDB's multi-model surface area into a precise optimizer research question. The main danger is allowing it to collapse back into a generic hybrid retrieval paper.

### Supported claims
- The query engine already exposes multi-modal operator families and cost-model hooks (`E1`, `E2`)
- a planning-oriented manuscript line already exists and can be sharpened into an estimation paper (`E3`, `E4`)

### Deferred claims
- superiority of a specific unified estimator
- robust plan selection across all modalities without additional experiments

## IX. Reproducibility & Artifact

- query module docs provide current scope
- next step: dedicated experiment package under `research/experiments/`

## X. Limitations, Risk, Ethics

- optimizer outcomes depend on workload mix and calibration quality
- relevance-aware cost signals risk hidden benchmark bias if datasets are not standardized

## XI. Conclusion

A dedicated cross-modal cardinality and cost-model manuscript is justified now. The repository already exposes the right optimizer hooks; the missing work is evidence consolidation and benchmark standardization.
