# Experiment Protocol: Hybrid Graph-Process-Vector Retrieval

**Manuscript**: `research/manuscripts/verticals/PROCESS_GRAPH_VECTOR_AI_HYBRID_PAPER_DRAFT.md`  
**Status**: PROTOCOL_DRAFT  
**Last Updated**: 2026-08-10

---

## Objective

Measure recall@K, latency, and G-Eval faithfulness for hybrid (graph + vector + event-log) vs. single-modal retrieval on process intelligence workloads.

---

## Dataset

- OCEL 2.0 event log: BPI Challenge 2017 or synthetic equivalent (50K events, 10K cases)
- Vector index: HNSW over process-segment embeddings (768-dim, Sentence-BERT)
- Graph: SCC + Louvain over process flow graph
- Ground truth: manually labeled relevant segments per query (50 queries, human-annotated)

---

## Experiment Suite

### Suite H1 — Single-Modal Baselines

| ID | Channel | Query set |
|---|---|---|
| H1-01 | Graph only | W1 structural queries |
| H1-02 | Vector only | W2 semantic queries |
| H1-03 | Event-log only | W3 temporal queries |

Metric: recall@K for K ∈ {1, 3, 5, 10}, p95/p99 latency

### Suite H2 — Hybrid Fusion

- All three channels active
- Fusion weights: α=0.4 (graph), β=0.4 (vector), γ=0.2 (event-log) — initial calibration
- Same 50-query ground-truth set from H1

Metric: recall@K, p95/p99 latency, fusion overhead vs. best single-modal baseline

### Suite H3 — LLM Answer Quality

For W4 mixed queries (all three dimensions):
- Context assembled from H1 winner (best single-modal) vs. H2 hybrid
- G-Eval faithfulness and context relevance scored by judge model (`gemma4:latest`)
- Report: mean G-Eval faithfulness improvement (hybrid – single-modal)

### Suite H4 — Fusion Weight Sensitivity

Sweep α ∈ {0.2, 0.4, 0.6} with β + γ = 1 − α, γ = 0.2 fixed.  
Report: recall@5 vs. α for W1, W2, W3, W4 separately.

---

## Environment

- Build: `linux-release`
- LLM judge: Ollama `gemma4:latest` (tool-calling support verified)
- Vector index: HNSW, ef-search=200
- Process engine: `src/process/` OCEL 2.0 tracer + DFMG

---

## Artifact Checklist

- [ ] Ground-truth annotation set (50 queries) committed under `research/experiments/verticals/data/`
- [ ] H1 single-modal recall@K table committed
- [ ] H2 hybrid recall@K and latency table committed
- [ ] H3 G-Eval faithfulness comparison committed
- [ ] H4 weight sensitivity plot data committed
- [ ] Results at `research/experiments/verticals/results/H_<timestamp>.json`
