# Quickwins PR Prep (2026-05-31)

## Scope
This prep summarizes the completed quickwin wave (QW-1..QW-8), key changed files, unified regression evidence, and residual review risks.

## Change Log By Quickwin

### QW-1: RAG prompt sanitization guard rails
- Core:
  - src/rag/rag_judge.cpp
  - src/rag/batch_evaluator.cpp
- Test additions/coverage:
  - tests/test_rag_batch_evaluator.cpp
    - BatchEvaluatorTest.EvaluateBatchHandlesUnsafePromptPayloads

### QW-2: Replication fail-closed quorum + causality metadata hardening
- Core:
  - src/replication/replication_manager.cpp
  - src/replication/conflict_resolution.cpp
- Behavior:
  - waitForReplication fail-closed for impossible/missing quorum paths
  - conflict winners enriched with merged vector-clock/dependency lineage and recomputed checksum
- Tests:
  - tests/test_replication_new_features.cpp
    - ThreeWayMergeTest.WinnerCarriesMergedCausalMetadata
    - FieldLevelMergeTest.WinnerCarriesMergedCausalMetadata

### QW-3: Voice auth audit hardening
- Already completed in prior step set (tracked in NEXT_QUICKWINS_TODO_2026-05-31.md)

### QW-4: Prompt safety in llm/training entry paths
- Already completed in prior step set (tracked in NEXT_QUICKWINS_TODO_2026-05-31.md)

### QW-5: Sharding determinism/freshness safeguards
- Already completed in prior step set (tracked in NEXT_QUICKWINS_TODO_2026-05-31.md)

### QW-6: LoRA GPU kernel input/shape/scaling hardening
- Core:
  - src/llm/lora_framework/kernels/vulkan_kernels.cpp
  - src/llm/lora_framework/kernels/directx_kernels.cpp
- Tests:
  - tests/test_lora_kernel_interface_hardening.cpp

### QW-7: Shared prompt safety helper across rag/llm/training
- Already completed in prior step set (tracked in NEXT_QUICKWINS_TODO_2026-05-31.md)

### QW-8: Scanner preflight triage artifacts
- Core:
  - tools/gap_scanner_v3.py
- Generated artifacts:
  - ai_working/gap_scan_v3_preflight_actionable_queue.json
  - ai_working/gap_scan_v3_preflight_summary.md

## Unified Focused Regression Evidence

Consolidated run command:

```powershell
Set-Location C:/Projects/ThemisDB/build-msvc-windows-release/bin ; ./themis_tests.exe --gtest_filter="RAGJudgeTest.RAS01_BenignDocumentsNotBlocked:RAGJudgeTest.RAS02_HighSeverityInjectionBlocked:RAGJudgeTest.RAS04_HighSeverityFoundButNotBlocked:BatchEvaluatorTest.EvaluateBatchInputsReturnsResults:BatchEvaluatorTest.EvaluateAsyncReturnsHandle:BatchEvaluatorTest.EvaluateBatchHandlesUnsafePromptPayloads:ReplicationManagerErrorHandling.SyncModeWithoutReplicaStreamsFailsClosed:ReplicationManagerErrorHandling.SemiSyncImpossibleQuorumFailsClosed:ThreeWayMergeTest.WinnerCarriesMergedCausalMetadata:FieldLevelMergeTest.WinnerCarriesMergedCausalMetadata:LlamaWrapperStateTest.GenerateRejectsBlockedPromptBeforeStateCheck:TrainingSafety.TrainRejectsPromptInjectionLikeCollectionName:AdaptiveShardRouterTest.ASR_DOM_05_DeterministicTieBreakWithMissingLoadSnapshots:AdaptiveShardRouterTest.ASR_DOM_06_FreshSnapshotPreferredOverStaleSnapshot:LoRAKernelInterfaceHardeningTest.*" --gtest_brief=1
```

Result snapshot:
- 21 tests from 9 suites
- 20 passed
- 1 skipped (feature-gated kernel path in current build profile)

## Residual Risks / Reviewer Watchlist
- Long-running model-loading tests create noisy logs; rely on final gtest summary lines for pass/fail.
- One focused kernel test remains skipped under current feature/profile gating; acceptable but should be exercised in a feature-on lane.
- Large generated JSON deltas in ai_working are expected from scanner reruns; review code changes separately from generated artifacts.

## Changed Files (current working tree)
- ai_working/NEXT_QUICKWINS_TODO_2026-05-31.md
- ai_working/gap_scan_v3_aggregate.json
- ai_working/gap_scan_v3_confidence_by_category.json
- ai_working/gap_scan_v3_confidence_review.json
- ai_working/gap_scan_v3_llm.json
- ai_working/gap_scan_v3_rag.json
- ai_working/gap_scan_v3_replication.json
- ai_working/gap_scan_v3_sharding.json
- ai_working/gap_scan_v3_summary.json
- ai_working/gap_scan_v3_training.json
- ai_working/gap_scan_v3_voice.json
- src/llm/lora_framework/kernels/directx_kernels.cpp
- src/llm/lora_framework/kernels/vulkan_kernels.cpp
- src/replication/conflict_resolution.cpp
- src/replication/replication_manager.cpp
- tests/test_lora_kernel_interface_hardening.cpp
- tests/test_rag_batch_evaluator.cpp
- tests/test_replication_new_features.cpp
- ai_working/gap_scan_v3_preflight_actionable_queue.json (new)
- ai_working/gap_scan_v3_preflight_summary.md (new)
