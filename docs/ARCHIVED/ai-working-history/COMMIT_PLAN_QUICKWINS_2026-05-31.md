# Commit Plan: Quickwins Wave (2026-05-31)

## Goal
Create review-friendly commits that separate production code changes, test coverage, generated scanner artifacts, and documentation/handover notes.

## Recommended Commit Split

### Commit 1: Replication + kernel hardening (production code)
Files:
- src/replication/replication_manager.cpp
- src/replication/conflict_resolution.cpp
- src/llm/lora_framework/kernels/vulkan_kernels.cpp
- src/llm/lora_framework/kernels/directx_kernels.cpp

Suggested message:
- feat(hardening): fail-closed replication conflict metadata and LoRA kernel scaling guards

Suggested commands:
```powershell
git add src/replication/replication_manager.cpp src/replication/conflict_resolution.cpp src/llm/lora_framework/kernels/vulkan_kernels.cpp src/llm/lora_framework/kernels/directx_kernels.cpp
git commit -m "feat(hardening): fail-closed replication conflict metadata and LoRA kernel scaling guards"
```

### Commit 2: Quickwin regression tests
Files:
- tests/test_replication_new_features.cpp
- tests/test_lora_kernel_interface_hardening.cpp
- tests/test_rag_batch_evaluator.cpp

Suggested message:
- test(quickwins): add focused regressions for prompt safety, replication causality, and kernel guards

Suggested commands:
```powershell
git add tests/test_replication_new_features.cpp tests/test_lora_kernel_interface_hardening.cpp tests/test_rag_batch_evaluator.cpp
git commit -m "test(quickwins): add focused regressions for prompt safety, replication causality, and kernel guards"
```

### Commit 3: Scanner outputs (generated artifacts)
Files:
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
- ai_working/gap_scan_v3_preflight_actionable_queue.json
- ai_working/gap_scan_v3_preflight_summary.md

Suggested message:
- chore(scanner): refresh gap-scan artifacts and preflight triage outputs

Suggested commands:
```powershell
git add ai_working/gap_scan_v3_aggregate.json ai_working/gap_scan_v3_confidence_by_category.json ai_working/gap_scan_v3_confidence_review.json ai_working/gap_scan_v3_llm.json ai_working/gap_scan_v3_rag.json ai_working/gap_scan_v3_replication.json ai_working/gap_scan_v3_sharding.json ai_working/gap_scan_v3_summary.json ai_working/gap_scan_v3_training.json ai_working/gap_scan_v3_voice.json ai_working/gap_scan_v3_preflight_actionable_queue.json ai_working/gap_scan_v3_preflight_summary.md
git commit -m "chore(scanner): refresh gap-scan artifacts and preflight triage outputs"
```

### Commit 4: Documentation and handover notes
Files:
- ai_working/NEXT_QUICKWINS_TODO_2026-05-31.md
- ai_working/QUICKWINS_PR_PREP_2026-05-31.md
- ai_working/PR_DESCRIPTION_QUICKWINS_2026-05-31.md
- ai_working/COMMIT_PLAN_QUICKWINS_2026-05-31.md

Suggested message:
- docs(quickwins): finalize execution tracker, PR prep, and commit plan

Suggested commands:
```powershell
git add ai_working/NEXT_QUICKWINS_TODO_2026-05-31.md ai_working/QUICKWINS_PR_PREP_2026-05-31.md ai_working/PR_DESCRIPTION_QUICKWINS_2026-05-31.md ai_working/COMMIT_PLAN_QUICKWINS_2026-05-31.md
git commit -m "docs(quickwins): finalize execution tracker, PR prep, and commit plan"
```

## Validation Before Push
Run this focused suite once after commit 2 (or once after all commits):

```powershell
Set-Location C:/Projects/ThemisDB/build-msvc-windows-release/bin ; ./themis_tests.exe --gtest_filter="RAGJudgeTest.RAS01_BenignDocumentsNotBlocked:RAGJudgeTest.RAS02_HighSeverityInjectionBlocked:RAGJudgeTest.RAS04_HighSeverityFoundButNotBlocked:BatchEvaluatorTest.EvaluateBatchInputsReturnsResults:BatchEvaluatorTest.EvaluateAsyncReturnsHandle:BatchEvaluatorTest.EvaluateBatchHandlesUnsafePromptPayloads:ReplicationManagerErrorHandling.SyncModeWithoutReplicaStreamsFailsClosed:ReplicationManagerErrorHandling.SemiSyncImpossibleQuorumFailsClosed:ThreeWayMergeTest.WinnerCarriesMergedCausalMetadata:FieldLevelMergeTest.WinnerCarriesMergedCausalMetadata:LlamaWrapperStateTest.GenerateRejectsBlockedPromptBeforeStateCheck:TrainingSafety.TrainRejectsPromptInjectionLikeCollectionName:AdaptiveShardRouterTest.ASR_DOM_05_DeterministicTieBreakWithMissingLoadSnapshots:AdaptiveShardRouterTest.ASR_DOM_06_FreshSnapshotPreferredOverStaleSnapshot:LoRAKernelInterfaceHardeningTest.*" --gtest_brief=1
```

Expected summary:
- 21 tests from 9 suites
- 20 passed
- 1 skipped (feature-gated kernel path)

## Optional Cleanup Notes
- CRLF/LF warnings are expected in this workspace and do not indicate content regressions.
- Keep generated scanner artifacts in their own commit to simplify reviewer focus on code logic.
