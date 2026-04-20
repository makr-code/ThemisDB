# ACID-Constrained Database-Native RAG with Integrated Evaluation in ThemisDB

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-04-19  
**Target Venue**: arXiv (cs.DB / cs.CL)

---

## Abstract

Retrieval-augmented generation (RAG) is often deployed as a loosely coupled stack where retrieval consistency and generation quality are optimized independently. This paper studies a database-native RAG architecture in ThemisDB that keeps retrieval, transactional guarantees, and evaluation in one execution environment. The core contribution is an ACID-constrained RAG pipeline with integrated quality instrumentation (faithfulness, relevancy, context precision/recall) and contention-aware latency analysis. We define a methodology that jointly measures answer quality and systems behavior under concurrent writes, showing how isolation policies affect factual grounding and tail latency. The design is grounded in existing repository components for RAG execution, LLM monitoring, and transaction control. We provide an implementation-oriented research protocol, explicit claim boundaries, and a reproducibility plan for benchmark-grade experiments. The paper does not claim final production-optimal tuning; instead, it contributes a measurable systems framing for quality-consistent RAG in transactional multi-model databases.

## I. Introduction

Production RAG deployments are usually assembled from loosely coupled components: a retriever, a generation model, and a separate quality pipeline. This separation improves engineering modularity, but it also introduces a critical systems weakness: retrieval consistency and generation quality are often optimized independently. In mutable enterprise corpora, this can produce a silent mismatch between what is currently committed in storage and what is surfaced to the model during response generation.

The central research gap is that most RAG evaluation studies report quality metrics under static or weakly controlled data conditions, while transactional database constraints are treated as infrastructure details rather than first-class experimental variables. For real production systems, this is insufficient. Under concurrent writes, isolation choices can change what context is visible at retrieval time, which in turn affects faithfulness, relevance, and answer stability.

This paper frames RAG as a joint quality-and-systems problem. We study an ACID-constrained, database-native RAG execution path in ThemisDB and define an evaluation methodology that couples quality metrics with latency and contention behavior. Instead of presenting a model-only improvement, we provide a systems-grounded design and measurement protocol that can be reproduced and extended for deployment-grade decision making.

### Contributions

1. A database-native RAG architecture that explicitly models transaction isolation in retrieval-generation loops.
2. A contention-aware evaluation protocol connecting quality metrics with p99 latency and abort behavior.
3. A repository-grounded evidence map for reproducible implementation and benchmark execution.

### Research Questions and Hypotheses

RQ1: How strongly do isolation-policy choices affect RAG faithfulness and answer relevancy under concurrent writes?

RQ2: Which isolation policy yields the best quality-latency operating point for each contention regime?

RQ3: How much of observed quality degradation is attributable to visibility semantics versus retrieval parameter settings?

H1: Under moderate and high contention, stricter isolation policies produce statistically higher faithfulness than weaker visibility policies.

H2: For each workload class, a non-trivial Pareto frontier exists where quality gains from stricter isolation are eventually dominated by p99 latency growth.

## II. Related Work

RAG was established as a retrieval-conditioned generation paradigm in which external knowledge is injected into model context at inference time. Follow-up work has improved retrieval quality, adaptive retrieval behavior, and practical deployment patterns. In parallel, evaluation frameworks such as RAGAS operationalized quality dimensions including faithfulness and answer relevance, making large-scale automatic assessment practical.

However, the dominant line of RAG evaluation still assumes largely static corpora or pipeline boundaries where transactional semantics are not modeled explicitly. This contrasts with classic database systems literature, where isolation, serialization anomalies, and contention behavior are treated as fundamental determinants of correctness.

Our work is positioned at this intersection: we keep the RAG quality lens, but we explicitly model isolation policy and concurrent updates as causal factors for answer quality. The novelty delta is therefore architectural and methodological: we evaluate RAG quality as an outcome of database execution semantics, not only as a function of retrieval/model quality.

## III. System Model / Architecture

The system model contains three tightly coupled planes. First, the retrieval plane resolves context using hybrid operators (vector and lexical, optionally graph-aware expansion) and assembles prompt context windows. Second, the inference-and-monitoring plane executes generation and records quality and runtime telemetry. Third, the transaction plane governs visibility through isolation and concurrency control policies.

The architectural co-location claim in this section is grounded in repository evidence E1-E4.

We assume a single logical query path per user request, but we vary background write pressure to model realistic update-heavy environments. This setup allows direct observation of how data mutation and isolation policy alter retrieved evidence and downstream response behavior.

The failure model includes three practical classes. The first is visibility drift, where weaker isolation can expose context states that reduce answer faithfulness. The second is resource-pressure failure, where contention increases timeout and abort risk. The third is quality degradation risk, where reduced context precision or stale reads increase hallucination probability even if base model behavior is unchanged.

## IV. Method / Design

Our method uses a dual-workload protocol. Workload W_q issues RAG question-answer requests, while workload W_u generates concurrent writes and updates against the same or overlapping document sets. We run both workloads under controlled concurrency and sweep isolation policies to produce quality-latency trade-off curves.

For decision support, we define a policy selector that maps service goals to execution policy. Given a target quality floor and latency SLO, the selector chooses the strictest isolation level that satisfies quality constraints while keeping p99 latency within budget. This converts abstract consistency settings into operationally actionable policy choices.

From a scaling perspective, end-to-end throughput is constrained by three terms: retrieval operator cost, generation cost, and concurrency-control overhead (validation, lock coordination, retries). The relative weight of each term changes with contention intensity and corpus update rate, making joint measurement mandatory.

Edge-case handling is explicit in the protocol: bounded retries for transient failures, timeout fallbacks for overloaded intervals, and empty-context safeguards that prevent unsupported factual claims. These safeguards are part of the evaluated system behavior rather than post-hoc operational guidance.

## V. Implementation Evidence (Repository-Grounded)

| Evidence ID | File | Scope | What It Proves | Status |
|-------------|------|-------|----------------|--------|
| E1 | `docs/research/implementation_influence/by_paper.md` | Lewis et al. (RAG) entry | RAG influence mapped to implemented module | ready |
| E2 | `docs/research/implementation_influence/by_paper.md` | Es et al. (RAGAS) entry | Evaluation/monitoring influence mapped to implementation | ready |
| E3 | `ARCHITECTURE.md` | `rag/` and transaction sections | Co-location of RAG and transaction capabilities | ready |
| E4 | `README.md` | capability matrix | Product-level statement of AI/LLM + ACID functionality | ready |
| E5 | `PERFORMANCE_EXPECTATIONS.md` | v1.8.2 abstract + benchmark summary | Root-level measured system baselines and known performance gaps | ready |
| E6 | `ARCHITECTURE.md` | single-node benchmark table | Cross-check latency/throughput baseline for query/vector/graph paths | ready |

Rules:
- Every major claim in Sections III-VII must map to >=1 evidence ID.
- Prefer tests/benchmarks over comments as claim support.

## VI. Experimental Methodology

### A. Setup
Experiments are executed on 1-4 node profiles with CPU-only and CPU+GPU variants to separate retrieval pressure from generation acceleration effects. Each run records exact hardware metadata (CPU model, core count, memory size, GPU type/VRAM, storage class) so that latency and contention results can be normalized across environments.

Software state is pinned by repository commit, build preset, and dependency lockfile snapshot. For each measurement block we persist configuration manifests and run identifiers in artifact JSON files.

The corpus is mutable by design. We combine a question set with controlled document update streams to model realistic write intensity. Reproducibility controls include fixed random seeds, mandatory warm-up intervals, and repeated trials with confidence-interval reporting.

### B. Workloads
Workload W1 establishes a quality reference under read-mostly conditions. Workload W2 introduces moderate write contention while preserving stable query arrival rates. Workload W3 applies bursty updates and higher overlap between write targets and retrieval candidates.

For each workload we run identical question batches under multiple isolation policies and compare quality and latency distributions. This controlled sweep isolates the causal impact of visibility semantics from pure model variance.

### C. Metrics
Primary systems metrics are p50/p95/p99 latency and end-to-end throughput. Primary quality metrics are faithfulness, answer relevancy, context precision, and context recall. Reliability metrics include abort rate, timeout rate, retry amplification, and stale-answer indicators.

All reported values are summarized per workload and isolation setting. Statistical reporting uses repeated runs with mean, standard deviation, and confidence intervals.

## VII. Results

### A. Primary Results
Current repository-grounded baselines establish that core query and data paths are fast enough to support meaningful contention studies: query p99 is 9.67 ms, graph edge throughput reaches 1.177 M ops/s, and time-series ingest reaches 61.0 M points/s. Single-node cross-checks further anchor expected write/read and vector-search behavior.

At the same time, two open gap signals remain important for this paper's interpretation: secondary index insert throughput is below target, and query peak throughput remains below its target bound. These gaps can amplify under mixed RAG+write contention and must therefore be modeled explicitly in analysis.

RAG-specific quality-under-contention results are the central pending block. We predefine the result schema as follows: Table R1 reports quality metrics by isolation level and workload; Table R2 reports latency/abort distributions for the same matrix; Figure R1 plots quality-latency frontiers to expose operating regions.

### D. Reporting Tables and Figure Plan

Table R1. Quality metrics by workload and isolation policy.

| Workload | Isolation Policy | Faithfulness | Answer Relevancy | Context Precision | Context Recall | N | 95% CI |
|----------|------------------|--------------|------------------|-------------------|----------------|---|--------|
| W1 | pending | pending | pending | pending | pending | pending | pending |
| W2 | pending | pending | pending | pending | pending | pending | pending |
| W3 | pending | pending | pending | pending | pending | pending | pending |

Table R2. Systems and reliability metrics by workload and isolation policy.

| Workload | Isolation Policy | p50 (ms) | p95 (ms) | p99 (ms) | Throughput | Abort Rate | Timeout Rate | Retry Amplification |
|----------|------------------|----------|----------|----------|------------|------------|--------------|---------------------|
| W1 | pending | pending | pending | pending | pending | pending | pending | pending |
| W2 | pending | pending | pending | pending | pending | pending | pending | pending |
| W3 | pending | pending | pending | pending | pending | pending | pending | pending |

Figure R1. Quality-latency frontier by isolation policy under increasing write contention.

### B. Ablations / Sensitivity
Sensitivity analysis will sweep context window size, retrieval top-k, and isolation configuration. We also include interaction terms (for example top-k x write intensity) to detect nonlinear degradation regimes.

### C. Negative Results
We explicitly report negative regimes where stricter isolation increases tail latency without statistically significant quality improvements. These outcomes are operationally valuable because they define ceilings for consistency-overhead trade-offs.

## VIII. Discussion

Practical implications: choose isolation by quality risk profile, not only by throughput target.

Operational constraints: GPU/CPU heterogeneity and monitoring overhead can shift absolute latency numbers and policy thresholds.

Measurement scope note: the root performance report confirms strong core-system latency/throughput baselines, while distributed and AI/ML module wave runs are explicitly marked as pending; this paper therefore treats current values as baseline anchors, not final RAG-quality evidence.

### Threats to Validity

Internal validity: concurrent-write generators may not perfectly reproduce production update locality; to reduce bias we use controlled overlap profiles and repeated runs.

Construct validity: automated quality metrics can miss domain-specific answer defects; we therefore pair metric results with explicit failure-case inspection and stale-context indicators.

External validity: results from selected corpora and hardware profiles may not transfer directly to all deployment environments; we mitigate this with multi-profile runs and transparent artifact metadata.

The above interpretation statements are constrained to E5-E6 for measured baselines and E1-E4 for integration scope.

### Claim Boundaries

**Supported claims:**
- The repository contains integrated building blocks for RAG + evaluation + transaction control (E1-E4).

**Deferred claims:**
- Quantitative superiority over external RAG stacks requires full benchmark completion.

## IX. Reproducibility & Artifact

The final version will freeze a specific commit hash and benchmark manifest. To support deterministic reruns, we standardize the execution flow below.

```powershell
# Configure and build
cmake --preset msvc-ninja-release
cmake --build --preset build-msvc-ninja-release --target themis_tests

# Optional: run test binary with local DLL path hints on Windows
$env:PATH = "C:\Projects\ThemisDB\build-msvc-ninja-release\bin;C:\Projects\ThemisDB\build-msvc-ninja-release\cmake;" + $env:PATH
.\build-msvc-ninja-release\bin\themis_tests.exe --gtest_color=yes
```

Benchmark artifacts are anchored by root reports and JSON outputs in `artifacts/perf_nv/targeted_validation/` and `artifacts/perf_nv/repro_validation_20260412_211053/`. Expected runtime for full sweeps is 2-8 hours depending on contention level and repetition count. Known pitfalls are warm-up sensitivity, noisy cluster timing, and policy-dependent retry amplification.

## X. Limitations, Risk, Ethics

- Misuse risks: over-trusting generated answers in safety-critical settings.
- Safety/compliance: require audit trails and policy controls for regulated domains.
- Boundary conditions: high-update corpora may need adaptive policy switching.

## XI. Conclusion

This draft defines a concrete research path for ACID-constrained, database-native RAG with integrated evaluation. The architecture and evidence anchors are already present in the repository; the next milestone is full benchmark execution and statistically robust reporting.

## References

1. P. Lewis et al., "Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks," NeurIPS 2020. URL: https://arxiv.org/abs/2005.11401
2. V. Karpukhin et al., "Dense Passage Retrieval for Open-Domain Question Answering," EMNLP 2020. URL: https://arxiv.org/abs/2004.04906
3. S. Es et al., "RAGAS: Automated Evaluation of Retrieval Augmented Generation," 2023. URL: https://arxiv.org/abs/2309.15217
4. Z. Jiang et al., "Active Retrieval Augmented Generation," EMNLP 2023. URL: https://arxiv.org/abs/2305.06983
5. E. J. Hu et al., "LoRA: Low-Rank Adaptation of Large Language Models," ICLR 2022. URL: https://arxiv.org/abs/2106.09685
6. A. Fekete et al., "Making Snapshot Isolation Serializable," ACM TODS, 2005. URL: https://dl.acm.org/doi/10.1145/1071610.1071615
7. M. J. Cahill, U. Rohm, and A. Fekete, "Serializable Isolation for Snapshot Databases," SIGMOD 2008. URL: https://dl.acm.org/doi/10.1145/1376616.1376690
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

1. Run contention-aware RAG benchmark harness.
2. Fill Sections VI-VII with measured results.
3. Freeze commit hash and artifact metadata.
4. Finalize references and figure set.

## Appendix C. Claim-to-Evidence Traceability

| Claim ID | Claim Summary | Evidence IDs |
|----------|---------------|--------------|
| C1 | ThemisDB integrates RAG execution, monitoring, and transactional control in one architecture. | E1, E2, E3, E4 |
| C2 | Current system baselines support contention-aware RAG evaluation feasibility. | E5, E6 |
| C3 | Final quality-under-contention superiority claims are deferred pending dedicated runs. | E5 |
