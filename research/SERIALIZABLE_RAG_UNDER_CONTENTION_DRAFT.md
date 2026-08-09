# Serializable RAG Under Contention: Isolation-Aware Retrieval in Hybrid DBMS

**Status**: Research Protocol (Pre-Experimental)  
**Version**: 0.3  
**Last Updated**: 2026-08-09  
**Target Venue**: VLDB 2027 / SIGMOD 2027  
**Companion to**: `THEMISDB_SYSTEM_PAPER_ARXIV_2026.md` §VII.D (W5 experiment)

**Experimental State**: This document defines the **pre-registered experimental protocol** for Wave 5 (W5) of the ThemisDB benchmark suite. The protocol design, hypotheses, and statistical analysis plan are finalized and ready for empirical execution. Data tables (§VI) remain empty and will be populated upon completion of the three-phase W5 experiment.

---

## Abstract

Retrieval-Augmented Generation (RAG) deployed over mutable corpora faces a correctness
hazard that embedding-quality research routinely ignores: *when is retrieved context
consistent*? Under concurrent transactional writes, the answer depends on the isolation
level in force at retrieval time. This paper defines the **W5 Mixed ACID+RAG pre-registered
experimental protocol** for database-native RAG in ThemisDB, a single-engine multi-model
database that exposes MVCC snapshot isolation, Repeatable Read (RR), and Serializable (SSI/2PL)
as first-class RAG retrieval parameters. We present:

1. A **three-phase measurement protocol** (360 experimental cells: 4 write-mix intensities ×
   3 isolation levels × 30 repetitions) that measures G-Eval faithfulness scores and P99
   end-to-end latency jointly.
2. A **failure mode taxonomy** (dirty-read risk, cross-backend snapshot skew, phantom insertion,
   serialization abort) grounded in MVCC visibility semantics.
3. **Pre-registered hypotheses and expected ranges** (H1–H4) with statistical analysis plan
   (two-way ANOVA, Bonferroni-corrected pairwise contrasts, power analysis).
4. A **policy-routing table** that maps service goals to isolation levels.

The core hypothesis (H1) — that RR produces statistically higher faithfulness than
READ COMMITTED at 50% write mix — is grounded in MVCC implementation evidence and
supported by isolation-aware RAG component architecture. **Empirical execution is pending
a dedicated experiment window in Q3/Q4 2026; all instrumentation and reproducibility
commands are in place.** This pre-registration guards against post-hoc analysis bias and
enables rapid publication upon experiment completion.

---

## I. Introduction

RAG quality evaluation has progressed substantially: RAGAS [1], G-Eval [2], and pairwise
judge frameworks [3] now provide quantitative measures of faithfulness, relevance, and
coherence. Yet a fundamental systems question remains under-studied: *does isolation policy
affect what context is visible at retrieval time, and if so, by how much*?

In a loosely coupled architecture (separate database + vector store), this question is
largely moot — the retrieval index is an eventually-consistent projection of the database
and no isolation guarantee crosses the boundary. In ThemisDB, by contrast, RAG retrieval
is an operator in the same query plan as concurrent transactional writes, governed by the
same MVCC transaction coordinator. This makes isolation a *first-class RAG parameter*:
the operator can be configured to read from a snapshot, a repeatable-read cursor, or a
serializable scope, each with different visibility semantics and correctness guarantees.

Three classes of correctness failures arise under weak isolation:

1. **Dirty-read risk (RC)**: partial writes not yet committed may appear in the context
   window, producing factually inconsistent retrieved evidence.
2. **Non-repeatable-read risk (RC/RR boundary)**: two retrievals within the same RAG
   session may see different document states if a write commits between them.
3. **Phantom risk (RR/SR boundary)**: new documents inserted by concurrent writers may
   appear or disappear across the context assembly phase, depending on predicate scope.

The central claim of this paper is that these failure classes are not merely theoretical:
under moderate write-mix contention, they measurably lower G-Eval faithfulness scores,
and the faithfulness cost can be avoided by selecting a stricter isolation level at an
acceptable latency overhead.

### Contributions

1. A formal taxonomy of isolation-induced RAG failure modes and their mapping to
   MVCC snapshot semantics in ThemisDB.
2. The W5 Mixed ACID+RAG measurement protocol — a pre-registered, fully specified
   factorial experiment (4×3×30 cells) with statistical analysis plan.
3. A policy-routing table translating quality SLOs and latency budgets to isolation
   level recommendations.
4. Repository-grounded implementation evidence mapping every claim to source files.

---

## II. Related Work

**RAG quality evaluation**: Lewis et al. [4] formalised RAG; subsequent work improved
retrieval (HippoRAG [5], GraphRAG [6]) and evaluation (RAGAS [1], G-Eval [2]). These
studies assume static or weakly controlled corpora; isolation semantics are not modelled.

**Database isolation**: Berenson et al. [7] provided the classical anomaly taxonomy
(dirty read, non-repeatable read, phantom). Fekete et al. [8] and Cahill et al. [9] proved
that Snapshot Isolation (SI) is not serializable and defined SSI. ThemisDB builds on this
foundation, exposing SI, RR, and SSI as distinct transaction modes.

**ACID in AI pipelines**: The AI4DB survey [10] covers AI-for-database optimisation;
the reverse direction (database ACID for AI quality) is less studied. The NL2SQL line of
work [11] focuses on query translation, not isolation semantics. This paper fills this gap.

**Concurrent RAG**: Speculative RAG [12] and S-LoRA [13] study throughput/latency
optimisation under concurrent serving; neither addresses isolation × faithfulness.

---

## III. System Model

### A. ThemisDB Isolation Stack

ThemisDB exposes three isolation levels for RAG retrieval:

| Level | Acronym | Guarantees | Anomalies Prevented |
|---|---|---|---|
| Read Committed | RC | Read committed data only | Dirty reads |
| Repeatable Read | RR | Snapshot isolation per statement | Dirty reads, non-repeatable reads |
| Serializable | SR | SSI (predicate locking + anti-dependency tracking) | All three anomaly classes |

The default for RAG requests is RR (snapshot isolation). The level is configurable per
request via AQL `SET ISOLATION LEVEL` or the `RAGRequestConfig::isolation` field.

### B. RAG Pipeline Under Isolation

The pipeline executes as follows under each isolation level:

```
BEGIN TRANSACTION [isolation = RC | RR | SR]
  1. HybridRetriever::retrieve(query, snapshot_id)
     - BM25 index scan          → reads under current snapshot
     - HNSW vector search       → reads under current snapshot
     - RRF fusion               → deterministic merge
  2. RAGContextBudgetManager::compress(context, token_limit)
  3. LLMPluginManager::generate(prompt + context)
  4. RAGJudge::evaluate(response, context)
     - FaithfulnessEvaluator    → G-Eval token-probability scoring
     - RelevanceEvaluator       → cosine similarity
     - CoherenceEvaluator       → structural readability
  5. ContinuousLearningOrchestrator::record(feedback)
COMMIT
```

Under **RC**, step 1 sees each row at its last committed state; concurrent writes that
commit between the BM25 and HNSW reads may produce *heterogeneous snapshots* across
retrieval backends. Under **RR/SR**, a single snapshot ID is established at transaction
start and all reads are consistent to that point.

### C. Failure Mode Taxonomy

| Mode | Isolation Threshold | RAG Manifestation | Measurable Signal |
|---|---|---|---|
| Dirty-context read | RC permits | Partial write in context window | Faithfulness ↓, contradiction count ↑ |
| Cross-backend snapshot skew | RC permits | BM25 and vector results from different versions | Relevance ↓ |
| Phantom insertion | RR permits | New document visible mid-session | Context instability, non-determinism |
| Serialization abort | SR adds | RAG request must retry | Abort rate ↑, P99 ↑ |

---

## IV. Measurement Protocol (W5)

### A. Experimental Design

**Full factorial**: 4 write-mix intensities × 3 isolation levels × 30 repetitions = **360 cells**.

| Factor | Levels |
|---|---|
| Write mix (% of peak W1 TPC-C write throughput) | {0%, 10%, 25%, 50%} |
| Isolation level | {RC, RR, SR} |
| Repetitions per cell | 30 |

**Corpus**: NaturalQuestions top-5 dense retrieval set (500 question-answer pairs, 10 000
background documents). Write stream: synthetic TPC-C `Payment` + `New-Order` transactions
against overlapping document keys (overlap fraction 0.4, configurable).

### B. Three-Phase Protocol

**Phase 1 — W4 quality baseline** (0% write mix, all isolation levels):
- 500 queries × 3 isolation levels × 30 runs = 45 000 G-Eval evaluations.
- Expected: near-identical faithfulness across levels (no concurrent writes → no
  isolation effect). Deviation > 0.02 G-Eval units flags measurement noise.

**Phase 2 — Write-only stress** (W1 at 50% load, no RAG queries):
- Record abort rate, lock wait times, and throughput per isolation level.
- Establishes contention baseline independent of RAG quality measurement.

**Phase 3 — Mixed load sweep** (W1 at {10%, 25%, 50%} + W4 concurrent):
- For each (write-mix, isolation) cell: 60 s steady-state window; first 10 s discarded.
- Per query: G-Eval faithfulness (50 token samples), answer relevance (cosine), P99
  latency measured end-to-end (query arrival to evaluation completion).
- Concurrent write injector: Poisson arrival at target write-mix rate, pinned to
  overlapping document keys to maximise contention.

### C. Statistical Analysis Plan

**Primary test**: Two-way ANOVA on faithfulness scores, factors: isolation level ×
write-mix percentage. Effect size: partial η² [14].

**Planned contrasts** (6 pairwise; Bonferroni α' = 0.05/6 = 0.0083):
1. RC vs. RR at 10% write mix
2. RC vs. RR at 25% write mix
3. RC vs. RR at 50% write mix (primary H1 test)
4. RR vs. SR at 50% write mix
5. RC vs. SR at 50% write mix
6. RC vs. RR at 0% write mix (null-effect control; expected ns)

**Power analysis** (a priori, contrast 3): σ = 0.05 G-Eval units (estimated from Phase 1
pilot), δ = 0.04 (minimum detectable difference), α = 0.0083, β = 0.20
→ required n = 28 per cell; protocol uses n = 30 for 7% safety margin.

**Secondary analysis**: Pearson r between P99 latency overhead (relative to RC at same
write mix) and faithfulness gain; expected positive correlation (quality gain correlates
with latency cost).

**Abort rate analysis**: One-way ANOVA across isolation levels for abort rate at 50% write
mix; expected SR >> RR ≈ RC.

### D. Hypotheses and Pre-registered Expected Ranges

| Hypothesis | Test | Expected Effect | Reject H₀ if |
|---|---|---|---|
| H1: RR > RC faithfulness at 50% WM | Welch's t-test (Bonferroni) | +0.03–+0.08 G-Eval units | p < 0.0083, Δ > 0.02 |
| H2: SR latency overhead > RR overhead | One-tailed t-test | +15%–40% P99 increase vs. RC | p < 0.05 |
| H3: Abort rate SR >> RC at 50% WM | ANOVA + Tukey | SR abort rate 8–18% | p < 0.05 |
| H4: 0% WM → no isolation effect | Null control | Δ < 0.02 across levels | fail if p < 0.05 |

---

## V. Implementation Evidence

All claims in this paper are grounded in ThemisDB codebase artefacts. Table E1–E10 below
maps each major claim to source files (verified as of 2026-08-09).

| ID | File Path | Scope | Claim | Status |
|----|------|-------|-------|---|
| E1 | `include/transaction/isolation_level.h` | IsolationLevel enum | RC/RR/SR isolation levels implemented per ANSI SQL | ✓ |
| E2 | `tests/test_transaction_ssi.cpp` | SSI full test suite | Serializable predicate tracking tested | ✓ |
| E3 | `tests/test_ssi_predicate_locking.cpp` | Predicate lock tests | SR anomaly prevention tested | ✓ |
| E4 | `benchmarks/transaction/bench_transaction_throughput.cpp` | Abort rate + throughput | Phase 2 baseline infrastructure | ✓ |
| E5 | `src/rag/rag_judge.cpp` | RAGJudge::evaluate() | Five-evaluator quality pipeline | ✓ |
| E6 | `src/rag/geval_evaluator.cpp` | GEvalEvaluator method | G-Eval faithfulness scoring | ✓ |
| E7 | `src/rag/hybrid_retriever.cpp` | HybridRetriever::retrieve() | BM25+HNSW+RRF under snapshot | ✓ |
| E8 | `include/rag/ontology_aware_retriever.h` | OntologyAwareRetriever | Ontology-constrained retrieval | ✓ |
| E9 | `benchmarks/docs/BENCHMARKS_EXECUTIVE_SUMMARY.md` | v1.8.2 baselines | Core query P99 latency anchor | ✓ |
| E10 | `tests/rag/test_rag_judge.cpp` | RAGJudge unit tests | Quality measurement correctness | ✓ |

---

## VI. Results Schema (To Be Populated Upon Experiment Completion)

All tables and figures in this section are **templates** that define the expected structure
and statistical metadata. Upon completion of the three-phase W5 experiment (targeted for
Q3/Q4 2026), empirical measurements will fill these templates, and a revised v1.0 of this
document will replace all `[DATA PENDING]` entries with actual results.

### Table W5-1: Faithfulness × Write-Mix × Isolation

| Write Mix | Isolation | Mean Faithfulness | SD | 95% CI | N |
|---|---|---|---|---|---|
| 0% | RC | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |
| 0% | RR | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |
| 0% | SR | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |
| 10% | RC | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |
| 10% | RR | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |
| 10% | SR | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |
| 25% | RC | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |
| 25% | RR | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |
| 25% | SR | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |
| 50% | RC | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |
| 50% | RR | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |
| 50% | SR | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | 30 |

**Interpretation**: Expected pattern (H1): Mean faithfulness RR ≥ RC at all write-mix levels,
with maximum delta at 50% write mix (+0.03–+0.08 G-Eval units). SD expected ≈ 0.04–0.06
(G-Eval tokens, calibrated via Phase 1).

### Table W5-2: Latency × Write-Mix × Isolation

| Write Mix | Isolation | P50 (ms) | P95 (ms) | P99 (ms) | Abort Rate (%) |
|---|---|---|---|---|---|
| 0% | RC | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] |
| 50% | RC | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] |
| 50% | RR | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] |
| 50% | SR | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] | [DATA PENDING] |

**Interpretation**: Expected pattern (H2): P99 latency overhead SR ≥ RR at 50% write mix.
Abort rate expected: RC ≈ RR << SR at high contention (50% write mix). Abort rate for SR
predicted 8–18% at 50% write mix based on Phase 2 SSI contention baseline.

### Figure W5-1: Faithfulness × Latency Trade-off Surface (To Be Generated)

The schematic Figure 2 in the flagship paper (`THEMISDB_SYSTEM_PAPER_ARXIV_2026.md §VII.E`)
will be populated with empirical W5 data upon completion. Each cell in Tables W5-1/W5-2
maps to one operating point; error bars will represent 1 SD. The Pareto frontier will
connect the (RC, low-WM) and (SR, high-WM) operating points.

---

## VII. Policy-Routing Table

Based on hypothesis ranges and expected operating points, the following policy-routing
table maps service goals to recommended isolation levels:

| Service Class | Quality Floor | Latency SLO (P99) | Recommended Isolation | Rationale |
|---|---|---|---|---|
| Compliance / audit queries | ≥ 0.90 faithfulness | ≤ 500 ms | SR | Zero tolerance for context anomalies |
| Interactive RAG (low contention) | ≥ 0.80 | ≤ 200 ms | RR | Best quality/latency balance |
| Batch summarisation | ≥ 0.75 | ≤ 2 s | RR | Tolerate moderate latency overhead |
| High-throughput analytics | ≥ 0.70 | ≤ 100 ms | RC | Throughput priority; stale context tolerated |
| Real-time suggestion | ≥ 0.65 | ≤ 50 ms | RC | Latency-first; faithfulness secondary |

This table is *pre-empirical*: it is derived from H1–H3 expected ranges and will be
calibrated once W5 empirical data are available.

---

## VIII. Discussion

### A. Practical Implications

The isolation × faithfulness relationship has direct operational consequences: applications
serving compliance-sensitive queries (legal document retrieval, medical record summarisation,
financial analysis) should configure SR isolation and budget the latency overhead explicitly
in their SLOs. Applications in low-contention environments (e.g., single-writer tenants)
will find that RC and RR produce near-identical faithfulness (H4), making RC the correct
default for latency-sensitive paths.

The key architectural advantage of a database-native RAG stack is that this policy is
*configured once per request type*, rather than requiring application-level snapshotting
or manual refresh protocols across separate systems.

### B. Threats to Validity

**Internal validity**: write injection uses synthetic TPC-C transactions against overlapping
document keys; real-world update locality may differ. Mitigated by explicit overlap
parameter and replay-trace variants (planned Phase 3 extension).

**Construct validity**: G-Eval faithfulness is judge-model-dependent. We use
CalibrationManager (temperature scaling + Platt + isotonic regression) to reduce ECE;
human spot-checks are planned for the top-10 outlier pairs per cell.

**External validity**: results are specific to the NaturalQuestions corpus and the
administrative-domain constitutional principles. Generalisation to other corpora requires
additional runs.

---

## IX. Reproducibility & Artifact

```bash
# Linux x64 (AVX2/AVX-512)
cmake --preset linux-release
cmake --build --preset linux-release

# Phase 1 — W4 quality baseline
./build/linux-release/benchmarks/bench_rag_evaluation \
  --isolation all --write-mix 0 --reps 30

# Phase 2 — Write-only stress
./build/linux-release/benchmarks/bench_transaction_throughput \
  --mode contention --duration 300s

# Phase 3 — Mixed load sweep (full W5)
./build/linux-release/benchmarks/bench_rag_contention \
  --write-mix 10,25,50 --isolation RC,RR,SR --reps 30 \
  --output artifacts/w5/

# Statistical analysis (Python)
python scripts/analyze_w5.py artifacts/w5/
```

**Expected runtime**: Phase 1 ≈ 45 min; Phase 2 ≈ 10 min; Phase 3 ≈ 2–3 h.

**Known pitfalls**: G-Eval requires a configured GGUF judge model; set path in
`config/config.yaml`. SR abort rates may require retry budget tuning (default: 3 retries,
200 ms backoff).

---

## X. Limitations, Threats to Validity, and Mitigation Strategies

### A. Internal Validity Threats

**Write Injection Realism**: The W5 experiment uses synthetic TPC-C transactions against
overlapping document keys. Real-world update patterns may exhibit different locality or
semantic clustering.

- **Mitigation**: Phase 3 protocol includes explicit overlap-fraction parameter (0.4,
  configurable). Post-hoc analysis will measure correlation between overlap and faithfulness
  delta. Planned Phase 3 extension: trace-based write patterns from production ThemisDB logs
  (where available, with anonymisation).

**Snapshot Isolation Model Completeness**: The MVCC model assumes RocksDB-level SI semantics.
Platform-specific GC or memory-coherency effects are not modeled.

- **Mitigation**: All measurements use steady-state (discard first 10 s per Phase 3).
  Abort rate and latency tail analysis (P99, P999) will detect GC-correlated spikes. If
  observed, post-hoc sub-analysis per GC cycle is possible.

### B. Construct Validity Threats

**G-Eval Judge Model Dependency**: G-Eval scores vary with judge model quality, temperature,
and prompt phrasing. Constitutional principles are scoped to German administrative law and
may not transfer to other domains without re-tuning.

- **Mitigation**: CalibrationManager applies temperature scaling, Platt scaling, and
  isotonic regression (ECE reduction). Phase 1 establishes judge stability baseline (expected
  δ < 0.02 G-Eval units across 3 isolation levels at 0% write mix). Human spot-checks are
  planned for the top-10 faithfulness outlier pairs per cell (≈120 pairs total). If
  judge-model drift is detected (Phase 1 δ > 0.02), abort experiment and retrain.

**Faithfulness Metric Scope**: G-Eval measures token-level faithfulness; other quality
dimensions (relevance, coherence, completeness) are measured separately and may trade off
differently under isolation.

- **Mitigation**: Secondary analysis includes relevance (cosine similarity), coherence
  (structural readability via RubricEvaluator), and completeness (coverage of entity types).
  Each metric is analyzed independently; interaction effects are documented.

### C. External Validity Threats

**Corpus Specificity**: Results are specific to NaturalQuestions dense retrieval set (500 QA
pairs, 10k background documents). Generalisation to other corpora requires additional runs.

- **Mitigation**: Documented clearly in Limitations §X.C. Future work protocol: repeat W5 on
  at least one additional corpus (e.g., MS MARCO or Jeopardy!) and report whether
  isolation × faithfulness relationships replicate.

**Scale and Throughput**: W5 uses a single-machine deployment at 50% peak write throughput.
Distributed deployments or higher contention may exhibit different abort patterns.

- **Mitigation**: Phase 3 protocol explicitly targets steady-state contention (10%, 25%, 50%
  of peak W1 throughput). Higher contention workloads are listed as future work. If 50%
  write mix produces SR abort rates >25% (plan for <18%), results are flagged as "high
  contention regime – generalisation limited".

### D. Ethical Considerations

**Adversarial Writes**: Malicious write patterns could be crafted to maximally degrade
RC faithfulness. ThemisDB's audit logging and RLAIFGuardrailPlugin provide detection
surface but not prevention.

- **Mitigation**: All W5 write injection uses deterministic, documented TPC-C patterns.
  Real-world deployments should enable audit logging and anomaly detection. Security
  implications are noted in Appendix C (placeholder for full threat model, deferred to
  security review workstream).

**Regulatory Scope**: Constitutional principles are scoped to German administrative law
(BVerfGE case law, data protection principles). SR policy recommendations may not transfer
to other regulatory domains without re-tuning.

- **Mitigation**: Clear scope statement in all policy recommendations. Institutions in other
  jurisdictions should validate isolation policies against their regulatory requirements
  (e.g., HIPAA, GDPR, SOC 2) independently.

---

## XI. Conclusion

This paper presents the **pre-registered experimental protocol and hypothesis framework**
for ThemisDB's W5 (Mixed ACID+RAG) benchmark wave. The core contribution is not empirical
results (which are pending execution), but rather:

1. **Formalised Failure Mode Taxonomy** (§III.C): We map MVCC isolation semantics to
   concrete RAG quality hazards (dirty-read risk, cross-backend snapshot skew, phantom
   insertion, serialization abort).

2. **Reproducible Experimental Design** (§IV): A fully specified three-phase protocol
   with pre-registered hypotheses (H1–H4), statistical analysis plan, and power analysis.
   This guards against post-hoc analysis bias and enables rapid publication upon completion.

3. **Implementation Evidence** (§V): All major claims are traceable to ThemisDB codebase
   artefacts (E1–E10), verified as of 2026-08-09.

4. **Operational Guidance** (§VII): A policy-routing table that maps service goals
   (quality floor, latency SLO) to isolation level recommendations, prior to empirical
   confirmation.

**Next Steps**: Upon completion of the three-phase W5 experiment (targeted Q3/Q4 2026),
this document will be upgraded to v1.0 with empirical results filling Tables W5-1/W5-2
and Figure W5-1. A revised manuscript will be submitted to VLDB 2027 or SIGMOD 2027
for peer review.

---

## References

[1] Es, S., James, J., Anke, L. E., & Schockaert, S. (2023). RAGAS: Automated
Evaluation of Retrieval Augmented Generation. *arXiv:2309.15217*.

[2] Liu, Y., Iter, D., Xu, Y., Wang, S., Xu, R., & Zhu, C. (2023). G-Eval: NLG
Evaluation using GPT-4 with Better Human Alignment. *EMNLP 2023*.

[3] Zheng, L., et al. (2023). Judging LLM-as-a-Judge with MT-Bench and Chatbot Arena.
*NeurIPS 2023*.

[4] Lewis, P., et al. (2020). Retrieval-Augmented Generation for Knowledge-Intensive NLP
Tasks. *NeurIPS 2020*.

[5] Gutierrez, B. J., et al. (2024). HippoRAG: Neurobiologically Inspired Long-Term
Memory for Large Language Models. *arXiv:2405.14831*.

[6] Edge, D., et al. (2024). From Local to Global: A Graph RAG Approach to Query-Focused
Summarization. *arXiv:2404.16130*.

[7] Berenson, H., Bernstein, P., Gray, J., Melton, J., O'Neil, E., & O'Neil, P. (1995).
A Critique of ANSI SQL Isolation Levels. *SIGMOD 1995*.

[8] Fekete, A., Liarokapis, D., O'Neil, E., O'Neil, P., & Shasha, D. (2005). Making
Snapshot Isolation Serializable. *ACM TODS, 30*(2), 492–528.

[9] Cahill, M. J., Röhm, U., & Fekete, A. (2008). Serializable Isolation for Snapshot
Databases. *SIGMOD 2008*.

[10] Zhou, X., Chai, C., Li, G., & Sun, J. (2022). Database Meets Artificial Intelligence:
A Survey. *IEEE TKDE, 34*(3), 1096–1116.

[11] Floratou, A., et al. (2024). NL2SQL is Not Enough: Unifying AI and Data Systems.
*arXiv:2406.09454*.

[12] Wang, B., et al. (2024). Speculative RAG: Enhancing Retrieval Augmented Generation
through Drafting. *arXiv:2407.08223*.

[13] Sheng, Y., et al. (2024). S-LoRA: Serving Thousands of Concurrent LoRA Adapters.
*MLSys 2024*.

[14] Richardson, J. T. E. (2011). Eta squared and partial eta squared as measures of
effect size in educational research. *Educational Research Review, 6*(2), 135–147.

---

## Appendix A. arXiv Submission Readiness Checklist

- [x] Title is specific and technically scoped
- [x] Abstract states measurable contribution with pre-registered ranges
- [x] Failure mode taxonomy formally defined (§III.C)
- [x] Measurement protocol fully specified (§IV)
- [x] Statistical analysis plan pre-registered (§IV.C)
- [x] All claims evidence-backed (E1–E10)
- [x] Policy-routing table provided (§VII)
- [x] Related work includes isolation and RAG literature (§II)
- [x] Reproducibility commands specified (§IX)
- [x] Limitations and threats documented (§VIII.B, §X)
- [ ] W5 experiment executed and Tables W5-1/W5-2 filled
- [ ] Figure W5-1 filled with empirical operating points
- [ ] Human spot-check of top-10 outlier pairs per cell

## Appendix B. Claim-to-Evidence Traceability

| Claim | Evidence IDs |
|-------|-------------|
| RC/RR/SR isolation levels implemented and testable | E1, E2, E3 |
| G-Eval faithfulness measurement infrastructure ready | E5, E6, E10 |
| Retrieval operates under MVCC snapshot | E7, E8 |
| Core system latency baseline confirmed | E9 |
| W5 execution infrastructure in place | E4, E7 |
