# ACID-Constrained Database-Native RAG with Integrated Evaluation in ThemisDB

**Status**: SUBMISSION_CANDIDATE  
**Version**: 0.3  
**Last Updated**: 2026-08-10  
**Target Venue**: VLDB 2027 (deadline Q4 2026)  
**Portfolio Cluster**: `research/manuscripts/retrieval_rag/`  
**Predecessor**: `research/DB_NATIVE_RAG_EVALUATION_PAPER_DRAFT.md` (SUPERSEDED_DRAFT — canonical location now this file)

---

## Abstract

Retrieval-augmented generation (RAG) is often deployed as a loosely coupled stack where retrieval consistency and generation quality are optimized independently. This paper reframes that setup as a database-systems problem and studies a database-native RAG path in ThemisDB. The contribution is twofold: (1) a repository-grounded architecture analysis connecting RAG quality control and transactional visibility semantics, and (2) a reproducible evaluation protocol for measuring quality metrics (faithfulness, answer relevancy, context precision, context recall) together with systems metrics (latency percentiles, throughput, abort/timeout behavior) under concurrent writes. We separate currently supported claims (code and benchmark evidence present in-repo) from deferred quantitative claims that require dedicated contention runs. The result is a review-ready, evidence-traceable paper draft aligned with the current ThemisDB repository state.

## I. Introduction

Production RAG systems are frequently assembled from independently tuned components (retriever, generator, evaluator). This separation can hide a key operational risk: the retrieval context exposed to the model may drift from transactionally committed state under concurrent updates.

ThemisDB is a multi-model database with native AI/LLM capabilities and explicit transaction controls. This combination makes it possible to treat RAG quality and isolation behavior as a joint measurement problem rather than two disconnected optimization tracks.

This paper addresses the following gap: RAG quality studies usually assume static corpora or weakly modeled update pressure, while database literature treats isolation and contention as first-class correctness variables. We unify these views for a database-native RAG setting.

### Contributions

1. A repository-grounded architecture mapping for database-native RAG in a multi-model ACID-capable system.
2. A reproducible methodology that evaluates quality and performance jointly under controlled write contention.
3. A claim-boundary model that distinguishes evidence-backed statements from deferred comparative claims.

### Research Questions and Hypotheses

- **RQ1:** How do isolation-policy choices affect faithfulness and answer relevancy when retrieval-visible data changes concurrently?
- **RQ2:** Which isolation setting provides the best quality-latency operating point for each contention regime?
- **RQ3:** How much degradation is caused by visibility semantics versus retrieval configuration changes?

- **H1:** Under moderate/high contention, stricter visibility semantics improve faithfulness compared with weaker semantics.
- **H2:** A Pareto frontier emerges where stricter consistency eventually increases p99 latency beyond practical SLO budgets.

Expected directional outcome: quality improvements should be strongest in high-overlap/high-contention workloads (W3), while latency costs should grow from W1 to W3 as visibility guarantees tighten.

## II. Related Work

RAG foundations (Lewis et al.) and dense retrieval systems (Karpukhin et al.) define the quality-centric retrieval-generation paradigm. Evaluation frameworks such as RAGAS make automated faithfulness/relevancy analysis practical at scale. Active retrieval work (Jiang et al.) further motivates adaptive retrieval under changing context.

In parallel, classic transaction-isolation work (Fekete et al.; Cahill et al.) shows that visibility semantics are central to correctness in concurrent systems. Our novelty is methodological: RAG quality is evaluated as a function of database execution semantics, not only model/retriever quality.

## III. ThemisDB System Context (Repository-Grounded)

ThemisDB documents native support for multi-model storage, AQL query processing, ACID transactions, and integrated AI/LLM workflows at architecture and module levels. The RAG module documents hybrid retrieval, multi-dimensional evaluation (including faithfulness/relevance dimensions), and benchmark targets for evaluation latency modes. The transaction module documents isolation-level behavior, MVCC-based execution, and atomic multi-layer updates.

This paper therefore treats ThemisDB as a suitable platform for studying quality/consistency coupling in RAG:

- **Retrieval/evaluation plane:** `src/rag/README.md`, `src/rag/ARCHITECTURE.md`
- **Transaction/visibility plane:** `src/transaction/README.md`
- **System-level context:** `README.md`, `ARCHITECTURE.md`

## IV. Methodology / Approach

### A. Experimental Design

We define a dual-workload experiment:

- **W_q (query workload):** RAG question-answer requests with fixed query sets.
- **W_u (update workload):** concurrent writes to document subsets with configurable overlap to retrieved contexts.

Each workload profile is executed under identical query batches while sweeping isolation settings supported by the Transaction module (`src/transaction/README.md` documents `ReadCommitted` and `Snapshot`).

### B. Metrics

- **Quality:** faithfulness, answer relevancy, context precision, context recall.
- **Systems:** p50/p95/p99 latency, throughput.
- **Reliability:** abort rate, timeout rate, retry amplification.

### C. Statistical Protocol

- Fixed seeds for workload generation.
- Warm-up phase before measurements.
- Repeated runs with confidence intervals.
- Explicit reporting of negative regimes (quality-neutral but latency-costly isolation choices).

### D. Failure and Edge-Case Handling

The protocol includes bounded retries for transient failures, explicit timeout accounting, and empty-context safeguards (no unsupported factual claims when retrieval context is insufficient).

## V. Evidence Map and Claim Policy

| Evidence ID | File | What It Supports |
|-------------|------|------------------|
| E1 | `research/implementation_influence/by_paper.md` (Lewis et al. entry) | RAG scientific lineage mapped to `src/rag/` |
| E2 | `research/implementation_influence/by_paper.md` (Es et al. / RAGAS entry) | Evaluation lineage mapped to implemented monitoring/evaluation capabilities |
| E3 | `src/rag/README.md` | Implemented RAG components and evaluation dimensions |
| E4 | `src/transaction/README.md` | Transaction/isolation semantics and ACID execution model |
| E5 | `benchmarks/bench_rag_evaluation.cpp` | Existing benchmark harness for RAG evaluation paths |
| E6 | `PERFORMANCE_EXPECTATIONS.md` | Current measured root baselines and open performance gaps |
| E7 | `README.md`, `ARCHITECTURE.md` | Multi-model + AI/LLM + transaction co-location at system level |

**Claim policy used in this draft:**

- Every central claim must map to at least one evidence item.
- Comparative superiority claims are out of scope unless backed by dedicated, reproducible run artifacts.

## VI. Evaluation / Experiments

### A. Workload Matrix

- **W1 (read-mostly):** low write overlap baseline.
- **W2 (moderate contention):** stable query rate, moderate update overlap.
- **W3 (bursty contention):** high overlap and bursty writes.

Each workload is evaluated across transaction visibility configurations and retrieval settings (top-k, context budget).

### B. Reporting Tables (Schema)

**Note:** In the current repository snapshot, dedicated contention experiment runs for W1-W3 have not been executed yet.

Table R1. Quality metrics by workload and isolation policy.

| Workload | Isolation Policy | Faithfulness | Answer Relevancy | Context Precision | Context Recall | N | 95% CI |
|----------|------------------|--------------|------------------|-------------------|----------------|---|--------|
| W1 | ReadCommitted/Snapshot sweep planned | n/a | n/a | n/a | n/a | n/a | n/a |
| W2 | ReadCommitted/Snapshot sweep planned | n/a | n/a | n/a | n/a | n/a | n/a |
| W3 | ReadCommitted/Snapshot sweep planned | n/a | n/a | n/a | n/a | n/a | n/a |

Table R2. Systems and reliability metrics by workload and isolation policy.

| Workload | Isolation Policy | p50 (ms) | p95 (ms) | p99 (ms) | Throughput | Abort Rate | Timeout Rate | Retry Amplification |
|----------|------------------|----------|----------|----------|------------|------------|--------------|---------------------|
| W1 | ReadCommitted/Snapshot sweep planned | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| W2 | ReadCommitted/Snapshot sweep planned | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| W3 | ReadCommitted/Snapshot sweep planned | n/a | n/a | n/a | n/a | n/a | n/a | n/a |

### C. Baseline Anchors Already Available

From `PERFORMANCE_EXPECTATIONS.md`, root-level measured anchors include:

- Query p99 latency: **9.67 ms** (below documented 50 ms target).
- Graph edge throughput: **1.177 M ops/s** (above documented target).
- Time-series ingest: **61.0 M points/s** (above documented target).

The same report also documents current gaps (e.g., secondary index insert throughput and query peak throughput). We use these as context for interpreting contention experiments, not as proof of RAG quality superiority.

## VII. Results Interpretation Framework

The interpretation logic for this paper is intentionally conservative:

- **Supported now:** The repository contains integrated building blocks for RAG execution, quality evaluation, and transaction control (E1-E7).
- **Deferred until dedicated runs:** Quantitative superiority claims against external RAG stacks or fixed isolation defaults.

A result is considered actionable only if quality gains and latency/reliability costs are reported together for the same workload/isolation configuration.

## VIII. Limitations / Known Issues

1. This repository snapshot includes architecture and benchmark harness evidence, but not finalized contention-matrix result artifacts for the proposed R1/R2 tables.
2. Baseline values in root performance documentation are system-level anchors and do not substitute for dedicated RAG-under-contention quality experiments.
3. Hardware heterogeneity (CPU/GPU profile differences) can shift absolute latency and throughput; all comparative claims require hardware metadata.
4. Automatic quality metrics can miss domain-specific answer defects; manual error analysis remains necessary for high-risk deployments.

## IX. Reproducibility Notes

**Windows (repository default preset example):**

```powershell
cmake --preset windows-release
cmake --build --preset windows-release --parallel 4
ctest --preset windows-release --output-on-failure -j 1 --timeout 60
```

**Linux (benchmark harness target example):**

```bash
cmake --build --preset linux-release --target bench_rag_evaluation
```

Publication finalization checklist:

- [ ] Record commit hash used for experiments.
- [ ] Archive exact benchmark command lines.
- [ ] Archive workload manifests/config snapshots.
- [ ] Export and retain raw result files.

## X. Conclusion

This revised draft is aligned with the current ThemisDB repository state and cleanly separates evidence-backed claims from deferred experimental claims. The paper is therefore review-ready as a repository-grounded methodology and evaluation framework for ACID-constrained, database-native RAG. The next milestone is execution and archival of the full contention experiment matrix.

## References

1. Lewis, P. et al. (2020). *Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks*. NeurIPS. URL: https://arxiv.org/abs/2005.11401
2. Karpukhin, V. et al. (2020). *Dense Passage Retrieval for Open-Domain Question Answering*. EMNLP. URL: https://doi.org/10.18653/v1/2020.emnlp-main.550
3. Es, S. et al. (2023). *RAGAS: Automated Evaluation of Retrieval Augmented Generation*. URL: https://arxiv.org/abs/2309.15217
4. Jiang, Z. et al. (2023). *Active Retrieval Augmented Generation*. EMNLP. URL: https://arxiv.org/abs/2305.06983
5. Fekete, A. et al. (2005). *Making Snapshot Isolation Serializable*. ACM TODS. URL: https://dl.acm.org/doi/10.1145/1071610.1071615
6. Cahill, M. J., Rohm, U., & Fekete, A. (2008). *Serializable Isolation for Snapshot Databases*. SIGMOD. URL: https://dl.acm.org/doi/10.1145/1376616.1376690
7. Hu, E. J. et al. (2022). *LoRA: Low-Rank Adaptation of Large Language Models*. ICLR. URL: https://arxiv.org/abs/2106.09685
8. ThemisDB Contributors (2026). *ThemisDB Repository*. URL: https://github.com/makr-code/ThemisDB

---

## Appendix A. Claim-to-Evidence Traceability

| Claim ID | Claim Summary | Evidence IDs |
|----------|---------------|--------------|
| C1 | ThemisDB integrates RAG execution/evaluation with transaction capabilities in one system context. | E3, E4, E7 |
| C2 | A concrete benchmark harness and measurable system baselines exist for grounded evaluation planning. | E5, E6 |
| C3 | External superiority claims remain deferred until dedicated contention experiments are executed and published. | E6 |
