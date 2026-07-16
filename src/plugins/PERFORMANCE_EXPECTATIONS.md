# PERFORMANCE_EXPECTATIONS - src/plugins

## Scope

- Module: src/plugins
- This file defines measurable plugins module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_plugin_system.cpp
  - benchmarks/bench_plugin_hot_plug.cpp
  - benchmarks/bench_ethics_ai_plugin.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| PLGP-1 | core plugin lifecycle and query/scan paths remain bounded | BM_ScanEmptyDirectory, BM_ScanDirectoryWithPlugins, BM_ReScanDirectory, BM_IsPluginLoaded, BM_GetPluginInfo, BM_GetAllPlugins, BM_GetPluginsByType, BM_GetLoadedPlugins, BM_LoadNonexistentPlugin, BM_ManifestParsing, BM_LoadUnloadPlugin, BM_ReloadPlugin, BM_GetMetricsSnapshot, BM_FilterPluginsByType, BM_BatchPluginQueries, BM_TypicalWorkflow |
| PLGP-2 | plugin system concurrency and memory behavior remain bounded | BM_ConcurrentQueries, BM_ConcurrentScans, BM_ConcurrentGetAllPlugins, BM_MemoryOverhead, BM_ManagerDestruction |
| PLGP-3 | hot-plug monitor scenarios remain bounded | HotPlugBenchmarkFixture/MultipleFileCreations, HotPlugBenchmarkFixture/AutoLoadDisabledVsEnabled, HotPlugBenchmarkFixture/MonitorMemoryFootprint |
| PLGP-4 | plugin-adjacent ethics AI plugin execution paths remain bounded | BM_PhilosophyLoader_LoadSingleProfile, BM_PhilosophyLoader_GetProfile, BM_PhilosophyLoader_HasProfile, BM_PhilosophyLoader_ListSchools, BM_ArgumentStore_StoreArgument, BM_ArgumentStore_GetArgument, BM_ArgumentStore_GetArgumentsByPhilosophy, BM_ArgumentStore_StoreDecision, BM_RAGContextEngine_BuildContext, BM_RAGContextEngine_BuildContextBatch10, BM_RAGContextEngine_FindSimilarDilemmas, BM_RAGContextEngine_TraverseArgumentChain, BM_RAGContextEngine_VectorSemanticSearch512, BM_DiscourseEngine_InitializeDebate, BM_DiscourseEngine_MakeDecisionSingleSchool, BM_DiscourseEngine_MakeDecisionFiveSchools, BM_DiscourseEngine_MakeDecisionWithRAG, BM_DiscourseEngine_ContinueDebateRound, BM_ArgumentStore_StoreDebateRound, BM_ArgumentStore_GetDebateTranscript, BM_EthicsEvaluator_EvaluateDecision, BM_EthicsEvaluator_EvaluateDecisionNoArgs, BM_EthicsEvaluator_EvaluateDecisionManyArgs, BM_EthicsEvaluator_RecordDecision, BM_EthicsEvaluator_GetMetricsText |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| PLGG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| PLGG-2 | plugin hot-path p99 <= release threshold | p99 from mapped plugin benchmark cases |
| PLGG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional plugins benchmark scenarios are introduced.

## Sourcecode Verification (Module: plugins/performance)

- Verified benchmark sources:
  - benchmarks/bench_plugin_system.cpp
  - benchmarks/bench_plugin_hot_plug.cpp
  - benchmarks/bench_ethics_ai_plugin.cpp
- Verified mapping surfaces:
  - plugin lifecycle/query, hot-plug, and plugin-adjacent AI execution paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.