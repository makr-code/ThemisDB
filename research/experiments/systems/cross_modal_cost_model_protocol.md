# Experiment Protocol: Cross-Modal Cardinality Estimation and Cost Models

**Manuscript**: `research/manuscripts/systems/CROSS_MODAL_CARDINALITY_COST_MODELS_PAPER_DRAFT.md`  
**Status**: PROTOCOL_DRAFT  
**Last Updated**: 2026-08-10

---

## Objective

Produce a reproducible benchmark matrix comparing cardinality estimator quality and plan outcomes across vector, geo, graph, and temporal modalities in AQL hybrid queries.

---

## Experiment Suite

### Suite C1 — Estimator Error per Modality

| ID | Modality | Query pattern | Metric |
|---|---|---|---|
| C1-01 | Vector | k-NN with HNSW ef-search variation | estimation error (actual vs. estimated cardinality) |
| C1-02 | Geo | Bounding box + radius containment filter | estimation error |
| C1-03 | Graph | Fixed-depth traversal with attribute predicates | estimation error |
| C1-04 | Temporal | Point-in-time range scan + interval overlap | estimation error |

### Suite C2 — Cross-Modal Hybrid Plans

| ID | Combination | Expected challenge |
|---|---|---|
| C2-01 | Vector + Geo | Normalize cosine similarity vs. spatial density |
| C2-02 | Graph + Attribute predicates | Combine structural selectivity with value-level filtering |
| C2-03 | Vector + Geo + Fulltext | Three-modality fusion plan; test plan stability |

### Suite C3 — Plan Regret

For each hybrid workload in C2: compare chosen plan latency vs. optimal plan latency (determined by brute-force enumeration on a small dataset). Report wrong-plan frequency.

---

## Environment

- Build: `linux-release`
- Dataset: synthetic — 100K vector entries (768-dim), 100K geo points, 100K graph nodes, 100K timestamped events
- Benchmark infrastructure: extend `benchmarks/bench_ycsb.cpp` or add `benchmarks/bench_cross_modal.cpp`

---

## Artifact Checklist

- [ ] C1 estimation error table committed
- [ ] C2 hybrid plan latency table committed
- [ ] C3 wrong-plan frequency per workload committed
- [ ] Benchmark source at `benchmarks/bench_cross_modal.cpp`
- [ ] Results JSON at `research/experiments/systems/results/C_<timestamp>.json`
