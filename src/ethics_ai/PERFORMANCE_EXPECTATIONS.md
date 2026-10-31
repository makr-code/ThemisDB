---
Author: ThemisDB Team
Created: 2026-07-28
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# Ethics AI Performance Expectations

## Purpose and evidence boundary

This file maps named benchmark cases to the `ethics_ai` paths they exercise. A benchmark target or target value is not a measured result or a release gate unless a recorded run and threshold-enforcing job establish that separately.

## Runtime benchmark cases

Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`. The root `benchmarks/CMakeLists.txt` has a separate conditional creation/CTest-registration block for `bench_ethics_ai_plugin`; it checks for `bench_ethics_ai_plugin.cpp` at the `benchmarks/` root before compiling the nested source. With the checked-in paths that existence guard is false, so the block skips this target. The benchmark suite itself is added only when `THEMIS_BUILD_BENCHMARKS=ON` and Google Benchmark is found.

| Path | Source-defined benchmark case(s) |
|---|---|
| Profile loader | `BM_PhilosophyLoader_LoadSingleProfile`, `BM_PhilosophyLoader_GetProfile`, `BM_PhilosophyLoader_HasProfile`, `BM_PhilosophyLoader_ListSchools` |
| Argument store | `BM_ArgumentStore_StoreArgument`, `BM_ArgumentStore_GetArgument`, `BM_ArgumentStore_GetArgumentsByPhilosophy`, `BM_ArgumentStore_StoreDecision`, `BM_ArgumentStore_StoreDebateRound`, `BM_ArgumentStore_GetDebateTranscript` |
| RAG context | `BM_RAGContextEngine_BuildContext`, `BM_RAGContextEngine_BuildContextBatch10`, `BM_RAGContextEngine_FindSimilarDilemmas`, `BM_RAGContextEngine_TraverseArgumentChain`, `BM_RAGContextEngine_VectorSemanticSearch512` |
| Discourse engine | `BM_DiscourseEngine_InitializeDebate`, `BM_DiscourseEngine_MakeDecisionSingleSchool`, `BM_DiscourseEngine_MakeDecisionFiveSchools`, `BM_DiscourseEngine_MakeDecisionWithRAG`, `BM_DiscourseEngine_ContinueDebateRound` |
| Evaluator | `BM_EthicsEvaluator_EvaluateDecision`, `BM_EthicsEvaluator_EvaluateDecisionNoArgs`, `BM_EthicsEvaluator_EvaluateDecisionManyArgs`, `BM_EthicsEvaluator_RecordDecision`, `BM_EthicsEvaluator_GetMetricsText` |

These cases are present in source, but no current target registration, result, or release baseline is asserted here.

## LDM helper benchmark cases

Source: `benchmarks/ethics_ai/bench_ldm.cpp`; the root benchmark CMake file adds this subdirectory when its `CMakeLists.txt` exists. The CMake target `bench_ethics_ai_ldm` is created only when `benchmark::benchmark` and `themis_core` targets are available, and registered with CTest only when present. The suite also requires the root benchmark build to be enabled and Google Benchmark to be found. Cases use a deterministic mock registry and stub inference, not a live LLM or legal database.

| Benchmark case | Source-comment target | Measured scope |
|---|---:|---|
| `BM_LDM_PlanGeneration` | `< 5 ms` | plan generation |
| `BM_LDM_Ebene1_22Schools` | `P95 ≤ 200 ms` | parallel helper with default deterministic inference |
| `BM_LDM_Ebene1_TimeoutFailsafe` | `≤ 10 ms` overhead | timeout bookkeeping with fast stub |
| `BM_LDM_MetaVerdictAssembly` | `≤ 50 ms` | MetaVerdict assembly on supplied mock outputs |
| `BM_LDM_MirrorSchool_Parallel4` | `≤ 200 ms` | four mirror helper calls with default stub |
| `BM_LDM_EndToEnd_LAYERED_FAST` | `P95 ≤ 1.2 s` | Ebene-1 stub plus MetaVerdict assembly; no legal lookup |
| `BM_LDM_AuditLog_Append` | `≤ 5%` overhead is stated as a gate goal | in-memory append and export |
| `BM_LDM_AuditLog_ExportOnly` | `≤ 5%` overhead is stated as a gate goal | in-memory export of a pre-populated log |

The source does not define `BM_LDM_EndToEnd_Full`, an Ebene-2/full-cluster benchmark, a legal-retrieval benchmark, or the previously documented single-mirror/cluster-consolidation/efficiency-ratio cases. The benchmark comments name gate goals, but this mapping does not establish that numerical thresholds are measured or enforced in CI.

## Validation status

- Benchmark source identifiers and CMake registration conditions were inspected on 2026-10-31. The plugin benchmark block's current root-path guard does not match its nested source path; the LDM target is dependency-gated.
- No benchmark binary was run for this documentation update; no hardware baseline is claimed.
- For current implementation limitations, see `ARCHITECTURE.md` and `ROADMAP.md`.
