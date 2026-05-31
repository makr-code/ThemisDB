# PERFORMANCE_EXPECTATIONS - src/process

## Scope

- Module: src/process
- This file defines measurable process module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_process_import_retrieval.cpp
  - benchmarks/bench_process_mining.cpp
  - benchmarks/bench_process_retrieval.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| PRCP-1 | process import/export and descriptor/prompt assembly paths remain bounded | BM_BpmnImport_NodeCount, BM_BpmnExport, BM_EpkImport_EventCount, ProcessManagerFixture/List_Scan, BM_LlmDescriptor_Generate, BM_BuildSystemPrompt, BM_BuildConformancePrompt, BM_SummarizeList |
| PRCP-2 | process mining analytics and export paths remain bounded | BM_ProcessMining_AlphaMiner, BM_ProcessMining_HeuristicMiner, BM_ProcessMining_InductiveMiner, BM_ProcessMining_EventLogExtraction, BM_ProcessMining_LargeLogProcessing, BM_ProcessMining_DFGCreation, BM_ProcessMining_DFGWithPerformance, BM_ProcessMining_VariantAnalysis, BM_ProcessMining_VariantClustering, BM_ProcessMining_ConformanceChecking, BM_ProcessMining_BPMNExport, BM_ProcessMining_PNMLExport |
| PRCP-3 | process retrieval, embedding, and state-change paths remain bounded | BM_ProcessModelImport, BM_ProcessEmbeddingGenerate, BM_ProcessEmbeddingPersist, BM_ProcessFullTextSearch, BM_ProcessHnswRetrieve, BM_ProcessStateChangeEmbed |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| PRCG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| PRCG-2 | process hot-path p99 <= release threshold | p99 from mapped process benchmark cases |
| PRCG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional process benchmark scenarios are introduced.

## Sourcecode Verification (Module: process/performance)

- Verified benchmark sources:
  - benchmarks/bench_process_import_retrieval.cpp
  - benchmarks/bench_process_mining.cpp
  - benchmarks/bench_process_retrieval.cpp
- Verified mapping surfaces:
  - process import/export, mining, and retrieval/embedding paths
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.