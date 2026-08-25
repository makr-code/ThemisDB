# PR Title
Quickwins Wave: close QW-1..QW-8 (RAG safety, replication hardening, LoRA guards, scanner preflight)

# PR Body
## Summary
This PR closes the current quickwin wave (QW-1..QW-8) and consolidates guardrails across RAG prompt safety, replication fail-closed behavior, conflict-resolution causality metadata, LoRA kernel validation, and scanner preflight triage artifacts.

## What changed
- QW-1 RAG prompt safety hardening
  - Sanitization guardrails are enforced across RAG judge and batch evaluator paths.
  - Added focused unsafe payload coverage.
- QW-2 Replication hardening
  - Fail-closed quorum/ack behavior for sync and semi-sync paths.
  - Conflict winners now carry merged causal/version metadata and consistent checksum refresh.
- QW-3 Voice auth audit hardening
  - Structured audit behavior was completed in this wave (tracked in quickwins TODO).
- QW-4 LLM/training prompt policy hardening
  - Block/neutralize unsafe prompt patterns at inference/training entry points.
- QW-5 Sharding determinism/freshness
  - Deterministic tie-breaks and stale/missing snapshot handling.
- QW-6 LoRA kernel hardening
  - Additional fail-closed checks for invalid dimensions and non-finite scaling parameters in Vulkan/DirectX launch paths.
- QW-7 Shared prompt safety helper
  - Common helper aligned rag/llm/training sanitization behavior.
- QW-8 Scanner preflight triage artifacts
  - Added actionable preflight JSON queue + markdown summary artifact with net-new high-confidence CRITICAL reporting.

## Key files
- src/rag/rag_judge.cpp
- src/rag/batch_evaluator.cpp
- src/replication/replication_manager.cpp
- src/replication/conflict_resolution.cpp
- src/llm/lora_framework/kernels/vulkan_kernels.cpp
- src/llm/lora_framework/kernels/directx_kernels.cpp
- tests/test_rag_batch_evaluator.cpp
- tests/test_replication_new_features.cpp
- tests/test_lora_kernel_interface_hardening.cpp
- tools/gap_scanner_v3.py
- ai_working/gap_scan_v3_preflight_actionable_queue.json
- ai_working/gap_scan_v3_preflight_summary.md

## Validation
Consolidated focused regression run:

```powershell
Set-Location C:/Projects/ThemisDB/build-msvc-windows-release/bin ; ./themis_tests.exe --gtest_filter="RAGJudgeTest.RAS01_BenignDocumentsNotBlocked:RAGJudgeTest.RAS02_HighSeverityInjectionBlocked:RAGJudgeTest.RAS04_HighSeverityFoundButNotBlocked:BatchEvaluatorTest.EvaluateBatchInputsReturnsResults:BatchEvaluatorTest.EvaluateAsyncReturnsHandle:BatchEvaluatorTest.EvaluateBatchHandlesUnsafePromptPayloads:ReplicationManagerErrorHandling.SyncModeWithoutReplicaStreamsFailsClosed:ReplicationManagerErrorHandling.SemiSyncImpossibleQuorumFailsClosed:ThreeWayMergeTest.WinnerCarriesMergedCausalMetadata:FieldLevelMergeTest.WinnerCarriesMergedCausalMetadata:LlamaWrapperStateTest.GenerateRejectsBlockedPromptBeforeStateCheck:TrainingSafety.TrainRejectsPromptInjectionLikeCollectionName:AdaptiveShardRouterTest.ASR_DOM_05_DeterministicTieBreakWithMissingLoadSnapshots:AdaptiveShardRouterTest.ASR_DOM_06_FreshSnapshotPreferredOverStaleSnapshot:LoRAKernelInterfaceHardeningTest.*" --gtest_brief=1
```

Result:
- 21 tests from 9 suites
- 20 passed
- 1 skipped (feature-gated kernel path in current profile)

## Reviewer notes
- Large ai_working JSON diffs are expected due to scanner reruns; review code changes separately from generated artifacts.
- One focused kernel path remains skipped in this profile and should also be exercised in a feature-on lane.
