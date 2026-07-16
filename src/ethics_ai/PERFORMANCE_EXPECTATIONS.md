# PERFORMANCE_EXPECTATIONS - src/ethics_ai

## Scope

- Module: src/ethics_ai
- This file defines measurable ethics_ai module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_ethics_ai_plugin.cpp

## Specific Expectations — Current Production (SELECTION_ONLY)

| Target ID | Expectation | Benchmark case |
|---|---|---|
| EAIP-1 | single-school decision path remains within release baseline budget | BM_DiscourseEngine_MakeDecisionSingleSchool |
| EAIP-2 | five-school decision path remains bounded under benchmark profile | BM_DiscourseEngine_MakeDecisionFiveSchools |
| EAIP-3 | context assembly and batch context paths remain bounded | BM_RAGContextEngine_BuildContext, BM_RAGContextEngine_BuildContextBatch10 |
| EAIP-4 | vector semantic lookup path remains bounded at configured input scale | BM_RAGContextEngine_VectorSemanticSearch512 |
| EAIP-5 | evaluator decision recording path remains bounded and stable | BM_EthicsEvaluator_RecordDecision |
| EAIP-6 | debate continuation path remains bounded under round progression | BM_DiscourseEngine_ContinueDebateRound |

## Specific Expectations — Layered Discourse Model (LDM, planned Q1–Q2 2027)

### Ebene-1: Parallel Equal-Weight Initial Scoring

| Target ID | Expectation | Measurement |
|---|---|---|
| LDM-E1-1 | Ebene-1 for N=22 schools completes in P95 ≤ 200 ms (fully parallel LLM batch) | BM_LDM_Ebene1_22Schools (to be added) |
| LDM-E1-2 | ABSTAIN failsafe on LLM timeout adds ≤ 10 ms overhead per school | BM_LDM_Ebene1_TimeoutFailsafe |
| LDM-E1-3 | DiscourseOrchestratorPlan generation (school → cluster assignment) ≤ 5 ms | BM_LDM_PlanGeneration |

### Ebene-2: Cluster Discourse

| Target ID | Expectation | Measurement |
|---|---|---|
| LDM-E2-1 | LAYERED_FULL Ebene-2 (6 clusters, 3 rounds, ≈10 parallel batches) P95 ≤ 6 s | BM_LDM_Ebene2_FullClusters |
| LDM-E2-2 | LAYERED_FAST Ebene-2 (axis-1 only) P95 ≤ 800 ms | BM_LDM_Ebene2_FastAxis1 |
| LDM-E2-3 | intra-cluster consolidation (4 schools → ClusterPosition) P95 ≤ 400 ms | BM_LDM_ClusterConsolidation |

### Ebene-3: MetaVerdict + Legal Grounding

| Target ID | Expectation | Measurement |
|---|---|---|
| LDM-E3-1 | convergence-counting MetaVerdict assembly ≤ 50 ms | BM_LDM_MetaVerdictAssembly |
| LDM-E3-2 | legal-DB norm lookup (KnowledgeGraphRetriever) ≤ 50 ms | BM_LDM_LegalGrounding |
| LDM-E3-3 | full Ebene-3 P95 ≤ 500 ms | BM_LDM_Ebene3_Full |

### Mirror-School-Modus

| Target ID | Expectation | Measurement |
|---|---|---|
| LDM-MS-1 | mirror-school output (position_abstract + strongest_tension) per school ≤ 200 ms | BM_LDM_MirrorSchool_Single |
| LDM-MS-2 | 4 mirror schools parallel ≤ 200 ms total | BM_LDM_MirrorSchool_Parallel4 |

### End-to-End Targets

| Target ID | Mode | P95 Target |
|---|---|---|
| LDM-ETE-1 | LAYERED_FULL (N=22, K=6, R=3) | ≤ 8 s |
| LDM-ETE-2 | LAYERED_FAST (N=22, axis-1+domain cluster) | ≤ 1.2 s |
| LDM-ETE-3 | Efficiency vs naive full discourse (N=22, N×N×R) | factor ≥ 30× faster |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| EAIG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| EAIG-2 | ethics hot-path p99 <= release threshold | p99 from mapped ethics_ai benchmark cases |
| EAIG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |
| EAIG-4 (LDM) | LAYERED_FULL P95 ≤ 8 s end-to-end | BM_LDM end-to-end suite |
| EAIG-5 (LDM) | LAYERED_FAST P95 ≤ 1.2 s end-to-end | BM_LDM_Fast end-to-end suite |

## Validation

- Existing expectations (EAIP-*) are met when mapped benchmarks run reproducibly in release
  profile and remain inside configured thresholds.
- LDM expectations (LDM-*) become gates once corresponding benchmark targets are added to
  `benchmarks/bench_ethics_ai_plugin.cpp` or a new `benchmarks/ethics_ai/bench_ldm.cpp`.
- Mapping should be expanded as additional ethics_ai benchmark scenarios are introduced.

## Sourcecode Verification (Module: ethics_ai/performance)

- Verified benchmark sources:
  - benchmarks/bench_ethics_ai_plugin.cpp
- Planned benchmark sources (LDM):
  - benchmarks/ethics_ai/bench_ldm.cpp (to be created with LDM implementation)
- Verified mapping surfaces:
  - discourse single/five-school decision benchmark cases
  - context build and vector semantic benchmark cases
  - evaluator record and debate continuation benchmark paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.