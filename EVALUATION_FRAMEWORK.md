# EVALUATION_FRAMEWORK.md

# ThemisDB Evaluation Framework
## For Hybrid Knowledge Retrieval Architecture

**Status:** Draft  
**Date:** 2026-06-01

---

## 1. Purpose

This document defines the evaluation framework for ThemisDB's target layered retrieval architecture.

The framework is intended to evaluate:

- ANN retrieval quality,
- tensor-based compression and summaries,
- graph evidence validation,
- LLM / LoRA grounded generation,
- distributed cross-shard behavior,
- approximation boundaries and trade-offs.

---

## 2. Why Evaluation Must Be First-Class

The target architecture introduces multiple interacting retrieval layers. This means success cannot be measured using a single metric such as recall@k.

Instead, the architecture must be evaluated as a system across:

- quality,
- latency,
- memory,
- token cost,
- evidence quality,
- provenance correctness,
- distributed efficiency.

---

## 3. Evaluation Axes

## 3.1 Retrieval Quality
- recall@k
- precision@k
- MRR / ranking quality
- candidate reduction effectiveness

## 3.2 Evidence Quality
- evidence coverage
- evidence completeness
- evidence relevance
- multi-hop support quality

## 3.3 Provenance Quality
- provenance fidelity
- source traceability
- trust signal correctness

## 3.4 Compression Quality
- tensor summary compression ratio
- information preservation
- redundancy reduction
- approximation error bounds

## 3.5 LLM Answer Quality
- faithfulness
- groundedness
- hallucination reduction
- prompt size reduction
- answer completeness

## 3.6 Distributed / Shard Efficiency
- shard fan-out
- bytes transferred
- summary-first retrieval efficiency
- selective exact load efficiency

---

## 4. Core Metrics

### Retrieval Metrics
- Recall@k
- Precision@k
- NDCG
- Candidate Reduction Ratio

### Evidence Metrics
- Evidence Coverage Rate
- Evidence Precision
- Multi-hop Support Score

### Provenance Metrics
- Provenance Fidelity Score
- Source Attribution Completeness

### Compression Metrics
- Summary Compression Ratio
- Approximation Loss
- Redundancy Elimination Rate

### LLM / LoRA Metrics
- Faithfulness Score
- Hallucination Rate
- Prompt Token Count
- Answer Support Density

### Distributed Metrics
- Cross-shard Requests per Query
- Cross-shard Bytes per Query
- Summary-first Shard Selectivity

---

## 5. Evaluation Scenarios

## 5.1 ANN-only Baseline
- embedding retrieval only
- no tensor layer
- minimal graph involvement

## 5.2 ANN + Graph Baseline
- semantic retrieval followed by graph filter/validation

## 5.3 ANN + Tensor
- semantic retrieval followed by tensor compression/routing

## 5.4 ANN + Tensor + Graph
- target layered retrieval path

## 5.5 Federated Summary Scenario
- shard summaries first
- selective exact loading

## 5.6 Model / Adapter Scenario
- adapter/package similarity retrieval
- compatibility and rebuild routing support

---

## 6. Ablation Strategy

To evaluate the layered architecture rigorously, ablation studies should compare:

- with vs without tensor summaries
- with vs without graph validation
- HNSW vs DiskANN
- summary-first vs direct retrieval
- provenance-aware vs provenance-agnostic ranking
- approximate-only vs approximate + exact validation

---

## 7. Evaluation Principles

1. Approximation must always be compared to exact reference paths.
2. Faithfulness and evidence quality are as important as latency.
3. Distributed efficiency is a first-class metric in federated scenarios.
4. Prompt reduction is useful only if answer support remains strong.
5. Any tensor-based improvement must demonstrate measurable benefit, not just architectural elegance.

---

## 8. Recommended Deliverables

- benchmark corpus and test suites
- evaluation dashboards
- layered retrieval benchmark scenarios
- approximation-vs-exact comparison reports
- model-switch and adapter-search benchmark scenarios

---

## 9. Executive Statement

**ThemisDB's future architecture must be benchmarked as a layered system. Success depends not only on faster retrieval, but on evidence quality, provenance fidelity, compression utility, cross-shard efficiency, and grounded answer generation.**
