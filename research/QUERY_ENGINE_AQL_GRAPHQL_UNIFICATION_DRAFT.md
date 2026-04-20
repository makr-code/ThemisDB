# Query Engine Unification in ThemisDB: AQL and GraphQL on a Shared IR and Cost Model

**Status**: Draft  
**Version**: 0.2  
**Last Updated**: 2026-04-19  
**Target Venue**: VLDB, SIGMOD, ICDE

---

## I. Abstract

ThemisDB already shows inherent AQL/GraphQL embedding in production code: language frontends feed into overlapping query infrastructure, and integration hooks between GraphQL and AQL-oriented planning paths are present. This paper reframes the objective from feasibility to consolidation quality: formalize and optimize an integration that already exists via a shared intermediate representation (IR) and harmonized cost behavior. The contribution is a repository-grounded findings report plus an evaluation design that quantifies latency, plan stability, and compatibility trade-offs in the QueryEngine.

Primary focus is QueryEngine-level AQL/GraphQL unification. Inline INFER and RAG are treated as optional extension operators evaluated only after core language unification results are established.

## II. Problem Statement

Supporting two query paradigms in one database is strategically valuable. In ThemisDB, AQL and GraphQL are already partially embedded into a common query substrate, but this embedding is not yet fully formalized as an explicit architectural contract.

This creates practical risks:

- duplicated optimizer logic,
- inconsistent cardinality and selectivity behavior,
- harder cross-language tuning,
- and elevated maintenance burden.

The core research problem is therefore not whether unification is possible, but how to systematize and optimize an integration that already exists in code and documentation without degrading semantics or SLOs.

## III. Research Questions and Hypotheses

### Contributions

1. A source-grounded finding that AQL and GraphQL are already inherently embedded in ThemisDB query infrastructure.
2. A consolidation design that makes this embedding explicit through shared IR and unified costing contracts.
3. An extension path showing how inline INFER and RAG can be attached to the same optimizer surface after core unification is validated.

RQ1: To what extent are AQL and GraphQL already embedded in shared query infrastructure in the current codebase?

RQ2: Does a unified cost model improve plan quality and p95/p99 latency for mixed-language workloads?

RQ3: What is the compatibility impact on existing AQL and GraphQL query corpora?

RQ4: Which operator classes (joins, traversals, nested projections) benefit most from shared QueryEngine optimization rules?

H1: Existing code paths already provide an inherent AQL/GraphQL embedding baseline that can be measured and strengthened through explicit IR/cost-model contracts.

H2: Shared optimization rules deliver measurable gains for join/traversal-heavy workloads without unacceptable regression in language-specific edge cases.

## IV. Repository Evidence Registry

| Evidence ID | File/Area | Scope | What It Supports |
|-------------|-----------|-------|------------------|
| E1 | src/query/query_engine.cpp | Shared query core with AQL parser/translator includes and multi-model execution | Common execution substrate anchor |
| E2 | src/query/aql_runner.cpp | AQL parse/translate/execute path plus SQL-to-AQL transpile path | Language normalization into shared AQL pipeline |
| E3 | src/api/graphql.cpp | Production GraphQL path with documented AQL-GraphQL integration/cost bridge revision | GraphQL-to-query integration anchor |
| E4 | src/query/gremlin_parser.cpp | Gremlin-to-AQL translation path | Existing cross-language embedding via AQL |
| E5 | src/query/ARCHITECTURE.md | Query architecture centered on shared AQL execution components | Documentation-level shared substrate evidence |
| E6 | ARCHITECTURE.md | System-level layered architecture (API + query integration) | Cross-module integration context |
| E7 | research/QUERY_ENGINE_AQL_GRAPHQL_UNIFICATION_DRAFT.md | This manuscript scope | Consolidation and measurement plan |
| E8 | src/cache/ARCHITECTURE.md | Multi-level and semantic cache design | Cache behavior implications for unified planner |
| E9 | src/sharding/ARCHITECTURE.md | Adaptive shard routing and distributed transaction stack | Federation/distributed execution implications |
| E10 | src/transaction/ARCHITECTURE.md | MVCC/SAGA/2PC transaction semantics | Consistency constraints for cross-language plans |
| E11 | src/governance/ARCHITECTURE.md | Policy enforcement and audit integration | Governance/compliance constraints in query execution |
| E12 | src/observability/ARCHITECTURE.md | Query profiling, SLO, tracing and anomaly analysis | Measurement and SLO reporting requirements |
| E13 | src/rag/ARCHITECTURE.md | Hybrid retrieval and multi-judge RAG evaluation | Inline RAG operator and quality-evaluation scope |
| E14 | PERFORMANCE_EXPECTATIONS.md | SLO and reproducible benchmark framework | Baseline and reporting rigor requirements |

Rule: major claims in Sections II-VIII map to one or more evidence IDs.

## V. Codebase Findings: Inherent Embedding Already Present

Finding F1: The query core is shared. The production query engine already ties AQL parser/translator and multi-model execution in one central path (E1).

Finding F2: Multiple language frontends normalize into AQL-centered execution paths. AQL runner already executes SQL-transpiled queries through the same pipeline (E2).

Finding F3: GraphQL integration is not isolated from query optimization concerns; production GraphQL code includes explicit AQL-GraphQL integration/cost bridge revision evidence (E3).

Finding F4: Translation-based language embedding into AQL is an established pattern in src/query (for example Gremlin-to-AQL), reinforcing inherent embedding already in place (E4, E5).

## VI. System Design

### A. Shared Intermediate Representation

Both frontends compile into a common operator algebra with core operators:

- SCAN
- FILTER
- PROJECT
- JOIN
- TRAVERSE
- AGGREGATE
- SORT
- LIMIT

### B. Normalization Layer

GraphQL field-resolution chains are normalized into relational/graph operators. AQL statements map directly to the same algebra, enabling equivalence analysis and shared rule application.

### C. Unified Cost Model

A shared estimator consumes cardinality and selectivity signals from both language paths, with language-specific correction factors where required.

### D. Backward Compatibility Contract

The execution engine enforces language-level semantic invariants through frontend-specific validation passes before and after optimization.

### E. Formal Unified Objective with Inline INFER/RAG

For plan candidate $p$, we optimize a multi-objective score that includes classical DB costs and AI-operator costs:

$$
J(p) = \alpha \cdot \text{latency}_{p99}(p)
+ \beta \cdot \text{cpu\_io\_cost}(p)
+ \gamma \cdot \text{semantic\_equivalence\_risk}(p)
+ \delta \cdot \text{infer\_rag\_cost}(p)
$$

where $\text{infer\_rag\_cost}(p)$ captures retrieval depth, embedding lookups, and model inference budget for inline INFER/RAG operators. This objective explicitly places API-shape planning and AI-augmented query planning in one optimizer surface.

## VII. Experimental Methodology

### A. Setup

Three configurations are compared:

- C1: current split pipelines,
- C2: shared IR with partial shared costing,
- C3: shared IR plus unified cost model and shared optimization rules.

### B. Workloads

W1: AQL-heavy transactional and analytical mix.  
W2: GraphQL-heavy API-style nested traversal workload.  
W3: Mixed AQL+GraphQL workload with semantically equivalent query sets.

### C. Metrics

Primary metrics: p50/p95/p99 latency, throughput, optimizer planning time, and plan-stability score.  
Correctness metrics: semantic-equivalence pass rate and regression count versus baseline outputs.  
Operational metrics: memory overhead and rule-application count.

### E. Secondary Boundary Conditions from Knowledge Base

These dimensions are explicitly secondary to the core QueryEngine study (RQ1-RQ4). They are activated once core AQL/GraphQL unification outcomes are stable.

- Cache sensitivity: L1/L2/L3 hit rates and semantic-cache hit behavior under cross-language equivalent queries (E8).
- Distributed/federation behavior: cross-shard routing overhead, rebalancing sensitivity, and federation plan stability (E9).
- Transaction semantics: equivalence and latency under MVCC/SAGA/2PC envelopes for mixed AQL/GraphQL flows (E10).
- Governance/compliance: policy decision overhead and audit completeness under unified execution (E11).
- Observability/SLO: per-operator tracing coverage, p99 burn-rate behavior, and anomaly detectability (E12, E14).
- AI quality path: RAG-judge quality dimensions when inline INFER/RAG appears in unified plans (E13).

### D. Baselines and Ablations

Baselines:

- B1: Current split execution paths (status quo).
- B2: Shared IR only (no unified AI-aware costing).
- B3: Shared IR + unified cost model without INFER/RAG-aware terms.

Ablations:

- A1: remove language-specific correction factors.
- A2: disable inline INFER/RAG terms from unified objective.
- A3: disable cross-language rule sharing for join/traverse operators.

These ablations isolate which gains come from pure IR consolidation versus AI-aware cost integration.

## VIII. Results Plan

### A. Reporting Tables and Figure Plan

Table Q1. Performance by configuration and workload.

| Config | Workload | p50 (ms) | p95 (ms) | p99 (ms) | Throughput | Planning Time (ms) |
|--------|----------|----------|----------|----------|------------|--------------------|
| C1 split | W1/W2/W3 | pending | pending | pending | pending | pending |
| C2 shared IR | W1/W2/W3 | pending | pending | pending | pending | pending |
| C3 shared IR+cost | W1/W2/W3 | pending | pending | pending | pending | pending |

Table Q2. Correctness and compatibility outcomes.

| Config | Equivalence Pass Rate | Regression Count | Rule Coverage | Notes |
|--------|-----------------------|------------------|---------------|-------|
| C1 split | pending | pending | pending | pending |
| C2 shared IR | pending | pending | pending | pending |
| C3 shared IR+cost | pending | pending | pending | pending |

Figure Q1. Latency and plan-stability trade-off across C1/C2/C3.

Table Q3. Inline INFER/RAG impact under unified planning.

| Config | Workload | Retrieval Depth | INFER/RAG Budget Fit | p99 Delta vs Non-AI Plan | Quality Proxy | Notes |
|--------|----------|-----------------|----------------------|--------------------------|---------------|-------|
| C1 split | W1/W2/W3 | pending | pending | pending | pending | pending |
| C2 shared IR | W1/W2/W3 | pending | pending | pending | pending | pending |
| C3 shared IR+cost | W1/W2/W3 | pending | pending | pending | pending | pending |

Figure Q2. Cost-quality-latency frontier with and without INFER/RAG-aware objective terms.

Table Q4. Secondary system impacts (post-core QueryEngine validation).

| Dimension | Metric Family | Baseline (C1) | Unified (C3) | Acceptance Target |
|-----------|---------------|---------------|--------------|-------------------|
| Cache | L1/L2/L3 + semantic hit rates | pending | pending | no regression in hit efficiency |
| Distributed/Federation | cross-shard/federated p99 | pending | pending | <= 15% overhead vs baseline |
| Transaction | MVCC/SAGA/2PC compatibility pass | pending | pending | >= 99.5% pass |
| Governance/Audit | policy+audit completeness | pending | pending | 100% decision traceability |
| Observability/SLO | tracing/SLO burn-rate coverage | pending | pending | full operator coverage |
| RAG Quality | faithfulness/relevance/coherence proxies | pending | pending | non-inferior vs baseline |

### B. Expected Negative Results

We expect some nested GraphQL edge cases to require language-specific correction factors even under a shared cost model.

## IX. Claim Boundaries

**Supported claims:**

- AQL and GraphQL are already inherently embedded through shared query infrastructure and translation-based normalization patterns (E1-E6).
- A shared-IR/cost-model consolidation is technically feasible on top of existing integration anchors (E1-E7).

**Deferred claims:**

- universal superiority of unified planning for all query shapes,
- production-grade calibration of correction factors,
- full backward-compatibility guarantees for all legacy workloads.

## X. Discussion

The expected strategic value of this paper is high because it addresses a platform-level capability: one optimizer and one execution logic across two major query paradigms.

### A. Scientific Discourse Context (DB Query Languages)

The broader discourse has evolved from SQL-centric relational optimization toward heterogeneous query execution where API-driven graph access and analytical operators coexist [1], [2]. GraphQL established a client-driven graph query interface for API ecosystems, while production databases retained cost-based planning and index-centric execution as core performance mechanisms [3], [4].

Modern system design therefore faces a language-fragmentation challenge: application teams prefer GraphQL ergonomics, while database kernels require optimizer-visible algebraic structure for predictable performance [2], [7]. ThemisDB's AQL/GraphQL embedding can be interpreted as a practical answer to this tension: preserve API ergonomics while converging execution into a shared optimizer substrate.

### B. Symbiosis Value for ThemisDB

The AQL/GraphQL symbiosis yields three system-level advantages:

1. Unified optimization surface: semantically equivalent API and analytical queries can share planning logic, reducing divergence in p95/p99 behavior.
2. Operational simplification: one query-core telemetry and tuning layer replaces parallel language-specific optimization silos.
3. Multi-model consistency: relational, graph, vector, and document operators can be costed in a single plan context rather than stitched across separate execution domains.

### C. Inline INFER and RAG as Material Extension

Inline INFER and RAG elevate the language symbiosis from query compatibility to AI-native query semantics. In this model, inference and retrieval are not external post-processing stages but explicit operators in the logical plan [5], [6], [8].

For ThemisDB this adds concrete capabilities:

1. Transaction-aware retrieval/inference: model context can follow the same visibility and consistency envelope as core query execution.
2. Cost-aware arbitration: optimizer can trade off retrieval depth, inference path, and latency budget in one planning loop.
3. Auditability and governance alignment: AI outputs remain tied to query plans and data lineage already present in database observability paths.

This is the key context frame for the paper: AQL/GraphQL QueryEngine unification is the main contribution, while inline INFER/RAG is a secondary extension path.

### D. Knowledge-Base Gap Analysis: Aspects to Include

The current manuscript should include six cross-cutting aspects as secondary constraints around the QueryEngine core:

1. Cache-aware planning assumptions (multi-level plus semantic cache) and their impact on cross-language plan selection (E8).
2. Federation and cross-shard behavior as first-class workload dimensions, not optional appendix scenarios (E9).
3. Transaction-envelope guarantees (MVCC/SAGA/2PC) for language-equivalent plans under write contention (E10).
4. Governance and policy-evaluation overhead in the critical path for regulated deployments (E11).
5. Observability and SLO instrumentation as publication-grade evidence requirements (E12, E14).
6. Inline RAG quality-evaluation coupling (faithfulness/relevance/coherence) when INFER/RAG operators are optimizer-visible (E13).

These additions do not change the primary scope; they harden external validity around the QueryEngine core.

### Threats to Validity

Internal validity: benchmark corpora may over-represent canonical query patterns and under-represent pathological resolver chains.

Construct validity: semantic-equivalence metrics may miss subtle client-observable differences unless output-shape checks are strict.

External validity: results may vary with data-shape diversity, index layouts, and deployment hardware.

## XI. Next Milestones

- M0: Freeze and document codebase findings F1-F4 as baseline architecture facts.
- M1: Define and prototype shared operator IR for a subset of AQL and GraphQL queries.
- M2: Implement normalization layer and equivalence test harness.
- M3: Integrate shared cost model with correction-factor hooks.
- M4: Run C1/C2/C3 benchmark matrix and publish first quantitative results.
- M5: Execute secondary knowledge-base dimension wave (cache, federation, transaction, governance, observability, RAG quality) and integrate Q4 results.

## XII. Implementation Roadmap and Quantitative Targets

### A. Four-Phase Plan

Phase 1: Consolidation contract
- Freeze F1-F4 findings as architecture contract.
- Define canonical shared IR operator schema and mapping tests.

Phase 2: Optimizer integration
- Implement unified objective with correction-factor hooks.
- Add INFER/RAG-aware cost terms and budget constraints.

Phase 3: Compatibility hardening
- Run AQL/GraphQL semantic-equivalence suite.
- Gate regressions with strict output-shape and cardinality checks.

Phase 4: Performance wave
- Execute C1/C2/C3 and B1/B2/B3 matrix.
- Publish ablation outcomes A1/A2/A3 and Pareto curves.

### B. Quantitative Targets

| Metric | Target |
|--------|--------|
| Mixed-workload p99 improvement (C3 vs C1) | >= 15% |
| Semantic-equivalence pass rate | >= 99.5% |
| Planning-time overhead (C3 vs C1) | <= 10% |
| Regression count in compatibility suite | 0 critical, <= 0.5% minor |
| INFER/RAG budget-fit adherence (extension track) | >= 95% queries within configured budget |

## XIII. IEEE Positioning and Research Scope

From an IEEE-style systems perspective, this work is positioned as QueryEngine architecture-and-evaluation research with three layers of contribution:

1. Empirical finding: AQL and GraphQL are already inherently embedded in source-level execution paths.
2. Systems design: explicit consolidation into shared IR/cost behavior for predictable cross-language performance.
3. AI-query extension (secondary): inline INFER/RAG integration as a query-language evolution path after core QueryEngine unification.

The main novelty claim is therefore centered on QueryEngine consolidation: not inventing a new standalone query language, but demonstrating how AQL/GraphQL symbiosis can be engineered as one coherent database kernel capability, with AI-native operators as optional follow-on.

## References

[1] M. Stonebraker et al., "MapReduce and Parallel DBMSs: Friends or Foes?," Communications of the ACM, 2010.

[2] S. Melnik et al., "Dremel: Interactive Analysis of Web-Scale Datasets," PVLDB, 2010.

[3] L. Byron and N. Schrock, "GraphQL Specification," GraphQL Foundation, 2021.

[4] ArangoDB GmbH, "AQL: ArangoDB Query Language," Documentation, 2026.

[5] P. Lewis et al., "Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks," NeurIPS, 2020.

[6] S. Es et al., "RAGAS: Automated Evaluation of Retrieval Augmented Generation," 2023.

[7] V. Sanca and A. Ailamaki, "Analytical Engines With Context-Rich Processing: Towards Efficient Next-Generation Analytics," ICDE, 2023.

[8] ThemisDB Contributors, "ThemisDB," GitHub repository, 2026.

## Appendix A. Claim-to-Evidence Traceability

| Claim ID | Claim Summary | Evidence IDs |
|----------|---------------|--------------|
| C1 | ThemisDB already contains inherent AQL/GraphQL embedding through shared query infrastructure. | E1, E2, E3, E5, E6 |
| C2 | Translation-based embedding into AQL-centered execution is an existing pattern. | E2, E4, E5 |
| C3 | Shared IR and unified costing is a feasible consolidation target above current integration anchors. | E1, E2, E3, E4, E5, E6, E7 |
| C4 | Final compatibility and superiority claims require full benchmark and regression evidence. | E7 |
| C5 | Publication-grade evaluation must include cache, distributed, transaction, governance, observability, and RAG-quality dimensions. | E8, E9, E10, E11, E12, E13, E14 |

## Appendix B. Submission Readiness Checklist

- [x] Problem and contribution are technically scoped
- [x] Research questions and hypotheses are explicit
- [x] Evidence registry is documented
- [x] Methodology and results schema are defined
- [x] Threats to validity are documented
- [x] Claim-to-evidence traceability is documented
- [x] Implementation roadmap and quantitative targets are defined
- [ ] C1/C2/C3 benchmark results inserted
- [ ] Artifact manifest and commit hash frozen
