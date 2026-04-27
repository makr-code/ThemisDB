# Cost-Aware Hybrid Retrieval Planning in AQL

**Status**: Draft  
**Version**: 0.2  
**Last Updated**: 2026-04-27  
**Target Venue**: SIGMOD 2027 / VLDB 2027 / EDBT 2027  
**Companion to**: `THEMISDB_SYSTEM_PAPER_ARXIV_2026.md` §IV.D (IndexAdvisor), `HYBRID_ANN_RETRIEVAL_SYSTEMS_PAPER_DRAFT.md`

---

## Abstract

Hybrid retrieval pipelines that combine lexical (BM25), dense vector (HNSW/IVF+PQ), and
graph-traversal operators incur query-plan costs that vary by orders of magnitude across
query classes, corpus distributions, and hardware profiles. Yet current implementations
select retrieval operator combinations via static heuristics that ignore these cost
gradients. We present the first cost-aware hybrid retrieval planner for AQL — ThemisDB's
native query language — that models each retrieval operator as a plan node with
measurable cost functions (CPU cycles, I/O pages, ANN distance computations, graph edge
expansions) and selects plans by minimising a Pareto-optimal (latency, recall) objective.
We define a three-workload evaluation protocol (W-HR-1: fact lookup, W-HR-2: semantic
expansion, W-HR-3: graph-constrained retrieval), pre-register expected plan rankings and
recall targets, and supply a repository-grounded evidence registry. The study answers four
research questions: which cost components explain plan quality; when hybrid beats
partial-pipeline plans; how stable plan selections are under distribution shift; and how
well estimated costs correlate with measured runtime and recall. Empirical execution is
deferred pending instrumented cost accounting in `bench_rag_hybrid_retriever.cpp`.

---

## I. Introduction

### A. Motivation

Modern document retrieval for RAG pipelines is rarely single-operator. A typical
administrative-law compliance query in ThemisDB may involve:
- BM25 inverted-index scan (term precision for statutory keywords)
- HNSW dense vector search (semantic similarity for concept expansion)
- Graph traversal via `KnowledgeGraphRetriever` (citation and precedent linkage)
- Optional spatial filter (geo-sensitive jurisdiction queries)

Executing all three in full is expensive; executing only one degrades recall for queries
that cross semantic-lexical boundaries. The correct plan depends on the *query type* and
*corpus statistics* — neither of which is known statically.

Existing approaches either always execute the full pipeline (high latency) or hard-code
operator order in AQL statement templates (low adaptability). Neither adapts to runtime
conditions such as HNSW segment eviction from VRAM, graph index cold starts, or
distribution shift from new document domains.

### B. Contributions

1. A formal cost model for ThemisDB's four retrieval operators (BM25, HNSW, IVF+PQ,
   GraphTraversal) parameterised by corpus statistics and hardware profiles.
2. A Pareto-optimisation framework for (latency, recall) plan selection in AQL.
3. A three-workload evaluation protocol with pre-registered expected rankings.
4. A distribution-shift robustness analysis for the planner's stability guarantees.

---

## II. Related Work

**Cost-based query optimisation**: System R's cost model [1] established selectivity and
I/O cost as primary plan-selection signals. Orca [2] extended this to distributed
execution. ThemisDB's `GraphQueryOptimizer` adapts cost-based rewriting to graph and
multi-model query plans.

**ANN retrieval cost modelling**: Jégou et al. [3] quantified IVF+PQ recall–speed
trade-offs. Malkov & Yashunin [4] characterised HNSW graph construction and query
complexity. These studies treat ANN in isolation; we integrate them into a joint
retrieval plan cost model.

**Hybrid retrieval**: Lin et al. [5] (RRF) and Ma et al. [6] (hybrid BM25+dense) show
that fusion improves recall but at cost of double index reads. Neither models the
*planning* decision for when to activate each index. ThemisDB's `HybridRetriever`
(E3) implements RRF fusion but does not currently select which indexes to activate.

**Learned query optimisers**: Bao [7] (bandit-based hint selection) and DNN-based
cardinality estimators [8] show that ML can improve upon hand-tuned cost models. We
treat the learned approach as a Phase 2 extension; Phase 1 uses analytic cost functions.

---

## III. System Model

### A. Retrieval Operator Inventory

| Operator | Implementation | Cost Class | Primary Cost Driver |
|---|---|---|---|
| BM25 | `src/rag/hybrid_retriever.cpp::bm25Score()` | O(|posting|) | Posting list I/O + term scoring |
| HNSW | `src/acceleration/hnsw_gpu_manager.cpp` | O(ef · d) | Distance computations × ef-search |
| IVF+PQ | `src/index/product_quantizer.cpp` | O(nprobe · m · n_codes) | ADC table lookups × nprobe |
| GraphTraversal | `src/graph/graph_index.cpp` | O(e · E_k) | Edge expansions × branching factor |

### B. Cost Model

Let `C_op(q, D, H)` be the cost of operator `op` for query `q`, corpus `D`, hardware `H`:

```
C_BM25(q, D, H)  = α · |posting(q)| + β · tf_idf_ops(q, D)
C_HNSW(q, D, H)  = γ · ef_search · dim + δ · VRAM_pressure(H)
C_IVF(q, D, H)   = ε · nprobe · m · n_codes / batch_size
C_Graph(q, D, H) = ζ · avg_degree(D) · max_hops + η · graph_cold_start(H)
```

Parameters α–η are calibrated per hardware profile from Phase 2 benchmarks. The total
plan cost is the sum of activated operators plus the RRF fusion overhead:

```
C_plan(P, q, D, H) = Σ_{op ∈ P} C_op(q, D, H) + C_RRF(|P|)
```

### C. Pareto Optimisation Objective

Given `k` candidate plans {P_1 ... P_k}:

```
P* = argmin_{P} λ · C_plan(P) + (1-λ) · (1 - Recall@10(P))
    subject to C_plan(P) ≤ latency_SLO
```

The trade-off parameter `λ ∈ [0, 1]` is a per-request configuration (default 0.5 for
interactive RAG; 0.9 for batch summarisation where latency matters less than recall).

### D. Integration with AQL Planner

The cost-aware planner is inserted between AQL statement parsing and execution:

```
AQL PARSE → LOGICAL PLAN → [CostAwareHybridPlanner] → PHYSICAL PLAN → EXECUTE
```

`CostAwareHybridPlanner::selectPlan(query, stats, hardware_profile)` emits a
`HybridRetrievalPlan` struct specifying which operators to activate, their parameter
settings (ef_search, nprobe, max_hops), and the RRF weight vector.

---

## IV. Experimental Methodology

### A. Workloads

| Workload | Query Type | Expected Best Plan | Notes |
|---|---|---|---|
| W-HR-1 | Fact lookup (exact term match) | BM25-only | Dense retrieval adds noise for exact queries |
| W-HR-2 | Semantic expansion (conceptual) | HNSW+BM25 | Concept-crossing queries need vector proximity |
| W-HR-3 | Graph-constrained (citation chain) | HNSW+Graph | Requires structural link traversal |
| W-HR-4 | Mixed jurisdiction (all operators) | Full hybrid | Maximum recall at latency cost |

**Corpus**: NaturalQuestions (500 queries) + ThemisDB administrative-law test set
(200 queries, proprietary). Corpus: 10 000 background documents.

**Distribution-shift extension**: W-HR-5 introduces 500 documents from a new domain
(medical) after initial calibration to test planner robustness.

### B. Parameter Sweep

| Parameter | Values |
|---|---|
| BM25 k1 | {0.9, 1.2, 1.5} |
| HNSW ef_search | {50, 100, 200} |
| IVF nprobe | {8, 16, 32} |
| Graph max_hops | {1, 2, 3, 5} |
| λ (quality-latency weight) | {0.3, 0.5, 0.7, 0.9} |
| Repetitions | 30 per cell |

### C. Metrics

**Primary**:
- Recall@10, NDCG@10 per workload
- P50/P95/P99 end-to-end retrieval latency (ms)
- Plan-selection accuracy (fraction of queries where selected plan ≥ recall of best plan)

**Cost model validation**:
- Pearson r(estimated cost, measured latency) — expected ≥ 0.85
- Mean Absolute Percentage Error (MAPE) of cost estimates — expected ≤ 25%

**Stability under distribution shift**:
- Recall drift (W-HR-5 vs. W-HR-2) for BM25-only plan vs. hybrid
- Plan-switch rate under W-HR-5 (fraction of queries where planner changes plan vs. W-HR-2)

### D. Statistical Analysis Plan

**Primary tests** (pairwise, Wilcoxon signed-rank, Bonferroni α' = 0.05/6 = 0.0083):
1. Hybrid vs. BM25-only on W-HR-2 (semantic)
2. Hybrid vs. HNSW-only on W-HR-1 (fact lookup)
3. Cost-aware vs. static hybrid on P99 latency at same recall
4. Estimated cost vs. measured latency (Pearson test, H₀: ρ = 0)
5. Plan stability W-HR-2 → W-HR-5 (shift)

---

## V. Pre-Registered Expected Results

| Hypothesis | Expected Range | Test |
|---|---|---|
| H1: BM25-only dominates W-HR-1 recall@10 | BM25 ≥ hybrid + 0.05 | Wilcoxon (p < 0.0083) |
| H2: Hybrid dominates W-HR-2 recall@10 | Hybrid ≥ BM25 + 0.08 | Wilcoxon (p < 0.0083) |
| H3: Cost-aware ≤ latency vs. full hybrid at λ=0.7 | −20% to −40% P99 reduction | t-test (p < 0.05) |
| H4: Cost model correlation with measured latency | ρ ≥ 0.85 | Pearson test |
| H5: Plan-switch rate under domain shift ≤ 30% | ≤ 30% queries change plan | Proportion test |

---

## VI. Implementation Evidence

| ID | File | Scope | Claim |
|----|------|-------|-------|
| E1 | `aql/README.md` | AQL query language | AQL plan execution framework exists |
| E2 | `benchmarks/bench_rag_hybrid_retriever.cpp` | Hybrid retrieval bench | Benchmark harness exists for W-HR-1..4 |
| E3 | `src/rag/hybrid_retriever.cpp` | HybridRetriever::retrieve() | BM25+HNSW+RRF implemented |
| E4 | `src/index/product_quantizer.cpp` | IVF+PQ | ADC + compression path implemented |
| E5 | `src/graph/graph_index.cpp` | GraphIndex traversal | Graph edge expansion implemented |
| E6 | `include/index/index_analysis_advisor.h` | IIndexAnalysisAdvisor | Adaptive index advisor interface defined |
| E7 | `compendium/docs/chapter_34_query_optimization.md` | Query optimization | Cost-based optimizer documented |
| E8 | `benchmarks/ann/README.md` | ANN benchmark protocol | Recall@k + latency framework defined |
| E9 | `research/ACID_CONSTRAINED_RAG_DRAFT.md` | ACID+RAG integration | Retrieval isolation model (companion paper) |
| E10 | `research/QUERY_ENGINE_AQL_GRAPHQL_UNIFICATION_DRAFT.md` | AQL/GraphQL unification | Plan execution layer context |

---

## VII. Results Schema (Pre-defined)

### Table HR-1: Recall@10 × Workload × Plan

| Workload | Plan | Recall@10 | NDCG@10 | N |
|---|---|---|---|---|
| W-HR-1 | BM25-only | *pending* | *pending* | 30 |
| W-HR-1 | HNSW-only | *pending* | *pending* | 30 |
| W-HR-1 | Full hybrid | *pending* | *pending* | 30 |
| W-HR-1 | Cost-aware | *pending* | *pending* | 30 |
| W-HR-2 | BM25-only | *pending* | *pending* | 30 |
| W-HR-2 | HNSW-only | *pending* | *pending* | 30 |
| W-HR-2 | Full hybrid | *pending* | *pending* | 30 |
| W-HR-2 | Cost-aware | *pending* | *pending* | 30 |
| W-HR-3 | Full hybrid | *pending* | *pending* | 30 |
| W-HR-3 | HNSW+Graph | *pending* | *pending* | 30 |
| W-HR-3 | Cost-aware | *pending* | *pending* | 30 |

### Table HR-2: Latency × Workload × Plan

| Workload | Plan | P50 (ms) | P95 (ms) | P99 (ms) | Plan-Sel. Acc. |
|---|---|---|---|---|---|
| W-HR-1 | Cost-aware (λ=0.5) | *pending* | *pending* | *pending* | *pending* |
| W-HR-2 | Cost-aware (λ=0.5) | *pending* | *pending* | *pending* | *pending* |
| W-HR-3 | Cost-aware (λ=0.5) | *pending* | *pending* | *pending* | *pending* |
| W-HR-4 | Full hybrid | *pending* | *pending* | *pending* | — |

### Table HR-3: Cost Model Validation

| Operator | Estimated Cost (μ) | Measured Latency (μ ms) | Pearson ρ | MAPE |
|---|---|---|---|---|
| BM25 | *pending* | *pending* | *pending* | *pending* |
| HNSW | *pending* | *pending* | *pending* | *pending* |
| IVF+PQ | *pending* | *pending* | *pending* | *pending* |
| GraphTraversal | *pending* | *pending* | *pending* | *pending* |

---

## VIII. Discussion

### A. Cost Model Calibration Strategy

Accurate cost estimation requires per-hardware calibration of the α–η parameters. We
propose a **one-time offline calibration run** (`bench_cost_calibration`) that:
1. Executes 100 BM25/HNSW/IVF/Graph queries with monotonically increasing corpus sizes.
2. Fits a linear regression C_op = a · feature_vector + b for each operator.
3. Persists calibration coefficients to `config/cost_model_<hw_profile>.json`.

This calibration is rerun automatically when a hardware change is detected (GPU model
change, RAM reduction) via the `IIndexAnalysisAdvisor::detectHardwareChange()` hook.

### B. Distribution-Shift Robustness

The planner maintains a sliding-window corpus statistics tracker (default: 10 000
most-recent queries). When the BM25 term-frequency distribution drifts by more than
`distribution_shift_threshold` (KL divergence > 0.15), the planner triggers a partial
re-calibration of the BM25 cost coefficient β. This is designed to handle the W-HR-5
domain-shift scenario without a full cold start.

### C. λ Policy Recommendations

Based on H3 pre-registered ranges and expected operating points:

| Use Case | Recommended λ | Rationale |
|---|---|---|
| Interactive chat (P99 ≤ 200 ms SLO) | 0.7–0.9 | Prioritise latency |
| Batch compliance audit | 0.2–0.4 | Maximise recall |
| Mixed tenant workload | 0.5 (default) | Balanced |
| High-precision citation research | 0.1–0.3 | Recall-first, latency budget 2 s |

### D. Threats to Validity

**Internal validity**: cost model is analytic; actual HNSW lookup cost depends on graph
fragmentation (not modelled in Phase 1). Mitigated by MAPE monitoring and re-calibration
trigger.

**Construct validity**: recall@10 may not correlate with downstream RAG faithfulness;
we cross-reference with G-Eval faithfulness in a subset of cells.

**External validity**: corpus statistics from NaturalQuestions may not transfer to
German administrative-law domain; we explicitly include the ThemisDB admin-law test set
as a second corpus.

---

## IX. Reproducibility & Artifact

```bash
# Step 1 — build
cmake --preset linux-release
cmake --build --preset linux-release

# Step 2 — cost model calibration (one-time per hardware profile)
./build/linux-release/benchmarks/bench_cost_calibration \
  --corpus testdata/nq_10k.jsonl \
  --output config/cost_model_linux_x64.json

# Step 3 — W-HR-1..4 hybrid retrieval benchmark
./build/linux-release/benchmarks/bench_rag_hybrid_retriever \
  --plan bm25,hnsw,ivf,graph,hybrid,cost-aware \
  --workloads W-HR-1,W-HR-2,W-HR-3,W-HR-4 \
  --reps 30 --lambda 0.3,0.5,0.7,0.9 \
  --output artifacts/hr/

# Step 4 — distribution-shift run (W-HR-5)
./build/linux-release/benchmarks/bench_rag_hybrid_retriever \
  --plan cost-aware --workloads W-HR-5 \
  --reps 30 --output artifacts/hr/shift/

# Step 5 — analysis
python scripts/analyze_hr.py artifacts/hr/
```

**Expected runtime**: calibration ≈ 5 min; W-HR-1..4 ≈ 30 min; W-HR-5 ≈ 10 min.

---

## X. Limitations, Risk, Ethics

- **Cost model freshness**: stale calibration coefficients can cause suboptimal plan
  selection; production deployments must schedule weekly re-calibration.
- **HNSW non-determinism**: graph-based ANN results vary slightly across runs; recall
  measurements report 30-run means with 95% CI.
- **Legal recall sensitivity**: for compliance queries where a missed document is a legal
  risk, operators should set λ ≤ 0.2 regardless of latency cost.

---

## XI. Conclusion

This paper specifies the cost-aware hybrid retrieval planning framework for ThemisDB AQL.
The core claim — that static hybrid pipelines waste 20–40% of retrieval latency on
unnecessary operator activations without recall benefit — is testable via the H1–H5
hypotheses. Pre-registered expected ranges and a λ policy table provide actionable
guidance for production configuration. Upon experimental execution, result tables HR-1
through HR-3 will be filled and this paper upgraded to v0.3.

---

## References

[1] Selinger, P. G., Astrahan, M. M., Chamberlin, D. D., Lorie, R. A., & Price, T. G.
(1979). Access path selection in a relational database management system. *SIGMOD 1979*.

[2] Soliman, M. A., Antova, L., Raghavan, V., El-Helw, A., Gu, Z., Shen, E., … &
Graefe, G. (2014). Orca: A modular query optimizer architecture for big data. *SIGMOD 2014*.

[3] Jégou, H., Douze, M., & Schmid, C. (2011). Product Quantization for Nearest Neighbor
Search. *IEEE TPAMI, 33*(1), 117–128.

[4] Malkov, Y. A., & Yashunin, D. A. (2020). Efficient and Robust Approximate Nearest
Neighbor Search Using Hierarchical Navigable Small World Graphs. *IEEE TPAMI, 42*(4),
824–836.

[5] Cormack, G. V., Clarke, C. L., & Buettcher, S. (2009). Reciprocal rank fusion
outperforms condorcet and individual rank learning methods. *SIGIR 2009*.

[6] Ma, X., et al. (2021). PROP: Pre-training with Representative Words Prediction for
Ad-hoc Retrieval. *WSDM 2021*.

[7] Marcus, R., et al. (2021). Bao: Making Learned Query Optimization Practical.
*SIGMOD 2021*.

[8] Dutt, A., Wang, C., Nazi, A., Kandula, S., Narasayya, V., & Chaudhuri, S. (2019).
Selectivity estimation for range predicates using lightweight models. *VLDB 2019*.

---

## Appendix A. Submission Readiness Checklist

- [x] Research questions formally stated
- [x] System model and cost functions defined (§III)
- [x] All four workloads specified with expected plan rankings (§IV)
- [x] Statistical analysis plan pre-registered (§IV.D)
- [x] Hypotheses H1–H5 with expected ranges (§V)
- [x] Implementation evidence registry E1–E10 (§VI)
- [x] Result table schemas pre-defined (§VII)
- [x] λ policy recommendations specified (§VIII.C)
- [x] Reproducibility commands provided (§IX)
- [x] Limitations and threats documented (§VIII.D, §X)
- [ ] Cost model calibration executed and coefficients published
- [ ] Tables HR-1 through HR-3 filled with experimental data
- [ ] Distribution-shift W-HR-5 results inserted

## Appendix B. Claim-to-Evidence Traceability

| Claim | Evidence IDs |
|-------|-------------|
| AQL plan execution framework exists | E1 |
| HybridRetriever (BM25+HNSW+RRF) implemented | E3 |
| IVF+PQ ADC path implemented | E4 |
| Graph edge expansion implemented | E5 |
| Index advisor interface defined | E6 |
| Benchmark harness for workloads W-HR-1..4 | E2, E8 |
