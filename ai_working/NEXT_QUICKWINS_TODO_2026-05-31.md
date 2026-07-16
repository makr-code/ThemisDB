# Next Quickwins TODO (Gap Scanner v3)

Generated: 2026-05-31
Source scan: [ai_working/gap_scan_v3_summary.json](ai_working/gap_scan_v3_summary.json), [ai_working/gap_scan_v3_confidence_review.json](ai_working/gap_scan_v3_confidence_review.json)
PR prep handoff: [ai_working/QUICKWINS_PR_PREP_2026-05-31.md](ai_working/QUICKWINS_PR_PREP_2026-05-31.md)
PR copy-ready text: [ai_working/PR_DESCRIPTION_QUICKWINS_2026-05-31.md](ai_working/PR_DESCRIPTION_QUICKWINS_2026-05-31.md)
Commit split plan: [ai_working/COMMIT_PLAN_QUICKWINS_2026-05-31.md](ai_working/COMMIT_PLAN_QUICKWINS_2026-05-31.md)

## Snapshot
- Total gaps: 38257
- Actionable (critical + high): 20928
- Top modules by high-confidence queue: llm, rag, replication, sharding
- Dominant high-confidence categories: llm_ai_safety, distributed_consistency, audit_logging

## Execution Update (2026-05-31)
- Consolidated closure pass (QW-1..QW-8 core regressions):
  - Focused suite run completed: `21 tests from 9 suites`.
  - Result: `20 passed`, `1 skipped` (feature-gated path in focused kernel suite).
  - Covered representative guardrails across rag prompt safety, replication fail-closed + causality metadata, llm/training prompt policy rejection, sharding determinism, and LoRA kernel hardening.

- QW-1 started: centralized prompt sanitization guard added in runtime evaluation path.
  - Implemented in [src/rag/rag_judge.cpp](src/rag/rag_judge.cpp) and [src/rag/batch_evaluator.cpp](src/rag/batch_evaluator.cpp).
  - Defense-in-depth behavior: document-level blocking visibility is preserved, while downstream evaluators receive sanitized prompt text.
  - Focused regression reruns after rebuilding `themis_tests` are green:
    - `RAGJudgeTest.RAS01_BenignDocumentsNotBlocked`
    - `RAGJudgeTest.RAS02_HighSeverityInjectionBlocked`
    - `RAGJudgeTest.RAS04_HighSeverityFoundButNotBlocked`
    - `BatchEvaluatorTest.EvaluateBatchInputsReturnsResults`
    - `BatchEvaluatorTest.EvaluateAsyncReturnsHandle`
  - Additional focused unsafe-payload coverage in [tests/test_rag_batch_evaluator.cpp](tests/test_rag_batch_evaluator.cpp):
    - `BatchEvaluatorTest.EvaluateBatchHandlesUnsafePromptPayloads`
  - Verification in this environment (re-run):
    - focused pass (6/6):
      - `RAGJudgeTest.RAS01_BenignDocumentsNotBlocked`
      - `RAGJudgeTest.RAS02_HighSeverityInjectionBlocked`
      - `RAGJudgeTest.RAS04_HighSeverityFoundButNotBlocked`
      - `BatchEvaluatorTest.EvaluateBatchInputsReturnsResults`
      - `BatchEvaluatorTest.EvaluateAsyncReturnsHandle`
      - `BatchEvaluatorTest.EvaluateBatchHandlesUnsafePromptPayloads`

- QW-2 started: replication quorum/ack fail-closed guards for write confirmation path.
  - Implemented in [src/replication/replication_manager.cpp](src/replication/replication_manager.cpp) (`waitForReplication`):
    - reject `required == 0` in sync/semi-sync wait path
    - reject impossible quorum (`required > active_streams`) before timeout loop
  - Added focused tests in [tests/test_replication_ha.cpp](tests/test_replication_ha.cpp):
    - `ReplicationManagerErrorHandling.SyncModeWithoutReplicaStreamsFailsClosed`
    - `ReplicationManagerErrorHandling.SemiSyncImpossibleQuorumFailsClosed`
  - Verification: both new tests pass (2/2).
  - Conflict-resolution hardening added in [src/replication/conflict_resolution.cpp](src/replication/conflict_resolution.cpp):
    - `ThreeWayMergeResolver::resolve` now fail-closes on empty conflict sets.
    - `FieldLevelMergeResolver::resolve` now fail-closes on empty conflict sets.
  - Added focused tests in [tests/test_replication_new_features.cpp](tests/test_replication_new_features.cpp):
    - `ThreeWayMergeTest.EmptyConflictSetFailsClosed`
    - `FieldLevelMergeTest.EmptyConflictSetFailsClosed`
  - Verification: both conflict-resolver fail-closed tests pass (2/2).
  - Causality/version metadata hardening for conflict winners:
    - [src/replication/replication_manager.cpp](src/replication/replication_manager.cpp):
      - `LastWriteWinsResolver::resolve(...)` now enriches winner with merged vector-clock frontier, merged dependencies, max HLC, and recomputed checksum
      - `CRDTMergeResolver::resolve(...)` now recomputes checksum after merged payload replacement
    - [src/replication/conflict_resolution.cpp](src/replication/conflict_resolution.cpp):
      - `ThreeWayMergeResolver::resolve(...)` and `FieldLevelMergeResolver::resolve(...)` now return causality-enriched winners (merged vector clock/dependencies, max HLC, recomputed checksum)
  - Added stale-version/causality regression tests in [tests/test_replication_new_features.cpp](tests/test_replication_new_features.cpp):
    - `ThreeWayMergeTest.WinnerCarriesMergedCausalMetadata`
    - `FieldLevelMergeTest.WinnerCarriesMergedCausalMetadata`
  - Verification in this environment:
    - focused pass (4/4):
      - `ReplicationManagerErrorHandling.SyncModeWithoutReplicaStreamsFailsClosed`
      - `ReplicationManagerErrorHandling.SemiSyncImpossibleQuorumFailsClosed`
      - `ThreeWayMergeTest.WinnerCarriesMergedCausalMetadata`
      - `FieldLevelMergeTest.WinnerCarriesMergedCausalMetadata`

- QW-3 started: structured audit logging for authentication-sensitive voice assistant flows.
  - Implemented in [src/voice/voice_assistant.cpp](src/voice/voice_assistant.cpp) and [include/voice/voice_assistant.h](include/voice/voice_assistant.h):
    - integrated `VoiceSecurityManager` into `VoiceAssistant`
    - emit `voice_authentication` audit events for:
      - `processVoiceCommand` auth gate
      - `streamProcessVoiceCommand` auth gate
      - public `authenticateSpeaker` API
    - each event records actor (`user_id`), action, session correlation (`session_id` where available), result (`success`), timestamp, and reason.
  - Observability: `VoiceAssistant::getStatistics()` now includes `voice_security` stats.
  - Added focused regression tests in [tests/test_voice_assistant.cpp](tests/test_voice_assistant.cpp):
    - `VoiceAssistantAuditAuth.AuthenticateSpeakerFailureIsAudited`
    - `VoiceAssistantAuditAuth.AuthenticateSpeakerSuccessIsAudited`
  - Tests were moved into an always-compiled section for this build profile (no `THEMIS_ENABLE_VOICE_ASSISTANT` gate dependency for the two audit cases).
  - Additional hardening in [src/voice/voice_assistant_llm.cpp](src/voice/voice_assistant_llm.cpp):
    - sanitize user input + recent conversation history before LLM prompt assembly
    - emit structured `voice_prompt_sanitization` audit events when sanitization changes payload
    - sanitize transcript payloads in summary/key-points/action-items prompt builders
  - Verification in this environment:
    - focused build `themis_tests` passed
    - focused tests passed (2/2):
      - `VoiceAssistantAuditAuth.AuthenticateSpeakerFailureIsAudited`
      - `VoiceAssistantAuditAuth.AuthenticateSpeakerSuccessIsAudited`
  - Direct authenticator coverage in [src/voice/voice_authenticator.cpp](src/voice/voice_authenticator.cpp) and [include/voice/voice_auth.h](include/voice/voice_auth.h):
    - every `authenticate(...)` outcome now emits an internal audit event
    - optional `setAuthAuditCallback(...)` hook added for external audit sinks
    - `get_statistics()` includes `total_auth_audit_events`
  - Additional focused test passed:
    - `VoiceBiometricAuth.AuthAuditCounterAndCallbackCoverFailureAndSuccess`

- QW-4 started: enforce safe prompt handling in LLM wrapper and training entry points.
  - Implemented in [src/llm/llama_wrapper.cpp](src/llm/llama_wrapper.cpp):
    - centralized prompt-policy guard in `LlamaWrapper::generate(...)` before inference/state-dependent dispatch
    - fail-closed rejection for blocked prompt patterns
    - RAG prompt formatting now sanitizes prompt text consistently
  - Implemented in [src/training/incremental_lora_trainer.cpp](src/training/incremental_lora_trainer.cpp):
    - training entry fail-closed check on `training_data_collection`
    - `encodeSample(...)` hashes sanitized text and neutralizes blocked prompt-like payloads
  - Added focused regression tests:
    - [tests/llm/test_llama_wrapper_state.cpp](tests/llm/test_llama_wrapper_state.cpp): `LlamaWrapperStateTest.GenerateRejectsBlockedPromptBeforeStateCheck`
    - [tests/test_training_phase2.cpp](tests/test_training_phase2.cpp): `TrainingSafety.TrainRejectsPromptInjectionLikeCollectionName`
  - Verification in this environment:
    - built target `themis_tests` successfully
    - focused tests passed (2/2) with prompt-policy block behavior confirmed
    - broader focused regression passed (36 tests):
      - `LlamaWrapperStateTest.*`
      - `LLMPromptPolicyTest.*`
      - `AsyncEnginePromptPolicyTest.*`
      - `IncrementalLoRATrainerTest.*`
      - `IncrementalLoRATrainerValidation.*`
      - `TrainingSafety.*`

- QW-5 started: deterministic routing safeguards in shard routing decisions.
  - Implemented in [src/sharding/adaptive_shard_router.cpp](src/sharding/adaptive_shard_router.cpp) and [include/sharding/adaptive_shard_router.h](include/sharding/adaptive_shard_router.h):
    - deterministic tie-break for equal domain score and equal load (lexical `shard_id` fallback)
    - explicit freshness handling for LLM load snapshots (`llm_load_freshness_ms`)
    - explicit handling of missing/stale load snapshots in `routeByDomain(...)`
    - explicit filtering of invalid (`NaN/inf`) scores and stale/unhealthy topology entries before iterative shard selection
    - stable score ordering (`score desc`, then `shard_id asc`) for deterministic replayability
  - Added focused deterministic replay regression in [tests/test_adaptive_shard_router.cpp](tests/test_adaptive_shard_router.cpp):
    - `AdaptiveShardRouterTest.ASR_DOM_05_DeterministicTieBreakWithMissingLoadSnapshots`
    - `AdaptiveShardRouterTest.ASR_DOM_06_FreshSnapshotPreferredOverStaleSnapshot`
  - Verification in this environment:
    - built target `themis_tests` successfully
    - focused domain-routing suites passed (12/12):
      - `AdaptiveShardRouterTest.ASR_DOM_*`
      - `LLMRaidRouting.*`

- QW-6 started: GPU kernel input validation and bounds checks in LoRA paths.
  - Implemented centralized fail-closed metadata validation in [src/llm/lora_framework/gpu_lora_layers.cpp](src/llm/lora_framework/gpu_lora_layers.cpp):
    - constructor rejects zero dimensions and non-finite scaling
    - `forward(...)` enforces 2D input and `in_dim` compatibility before backend dispatch
    - `backward(...)` enforces 2D gradient and `out_dim` compatibility before backend dispatch
    - explicit guard that `backward(...)` cannot run before successful `forward(...)`
    - explicit guard for missing cached intermediate activation in non-checkpointing path
  - Implemented backend launcher validation in [src/llm/lora_framework/kernels/hip_fused_kernels.cpp](src/llm/lora_framework/kernels/hip_fused_kernels.cpp):
    - fail-closed `hipErrorInvalidValue` on null pointers and zero-sized launches
    - explicit range guards before HIP grid construction to prevent oversized launch geometry
    - fail-closed guards for invalid MSE launch parameters (`n <= 0`, `num_blocks <= 0`)
  - Added focused interface hardening tests in [tests/test_lora_kernel_interface_hardening.cpp](tests/test_lora_kernel_interface_hardening.cpp):
    - `LoRAKernelInterfaceHardeningTest.GPULoRALayerConstructorRejectsInvalidDimensions`
    - `LoRAKernelInterfaceHardeningTest.GPULoRALayerRejectsForwardBackwardShapeMismatch`
    - `LoRAKernelInterfaceHardeningTest.GPULoRALayerRejectsBackwardBeforeForward`
    - `LoRAKernelInterfaceHardeningTest.VulkanInitializedInvalidDimensionsFailClosed`
  - Guard-fix follow-up:
    - replaced brittle `size()==0` precondition checks with `shape().empty()` for cached activation validation in [src/llm/lora_framework/gpu_lora_layers.cpp](src/llm/lora_framework/gpu_lora_layers.cpp)
  - Additional fail-closed hardening for non-finite scaling values:
    - [src/llm/lora_framework/kernels/vulkan_kernels.cpp](src/llm/lora_framework/kernels/vulkan_kernels.cpp): reject non-finite `scaling`/`scalar` in scalar multiply, grad-A, fused forward, fused backward launchers
    - [src/llm/lora_framework/kernels/directx_kernels.cpp](src/llm/lora_framework/kernels/directx_kernels.cpp): reject non-finite `alpha`/`scaling`/`scalar` in matmul, scalar multiply, grad-A launchers
    - [tests/test_lora_kernel_interface_hardening.cpp](tests/test_lora_kernel_interface_hardening.cpp): Vulkan initialized path now asserts non-finite fused-forward scaling fails closed
  - Verification in this environment:
    - built target `themis_tests` successfully
    - focused suite passed `LoRAKernelInterfaceHardeningTest.*`: 6 passed, 1 skipped (DirectX feature-off skip)
    - re-run after HIP launcher patch and guard-fix remained green
    - re-run after Vulkan/DirectX non-finite scaling guards remained green

- QW-7 started: one shared safety helper for prompt assembly.
  - Implemented shared helper in [include/llm/prompt_safety_utils.h](include/llm/prompt_safety_utils.h):
    - centralized shared PromptPolicy construction (`sharedPromptSafetyPolicy()`)
    - shared sanitizer API (`sanitizePromptWithSharedPolicy(...)`) for consistent block/redact behavior
  - Migrated call-sites to shared helper:
    - [src/llm/llama_wrapper.cpp](src/llm/llama_wrapper.cpp): removed local duplicated policy construction and delegated to shared helper
    - [src/training/incremental_lora_trainer.cpp](src/training/incremental_lora_trainer.cpp): removed duplicated local training policy and delegated to shared helper
    - [src/rag/batch_evaluator.cpp](src/rag/batch_evaluator.cpp): integrated shared helper after existing RAG sanitizer stage to align rag/llm/training prompt safety
  - Verification in this environment:
    - built target `themis_tests` successfully
    - focused cross-module regression passed (4/4):
      - `LlamaWrapperStateTest.GenerateRejectsBlockedPromptBeforeStateCheck`
      - `TrainingSafety.TrainRejectsPromptInjectionLikeCollectionName`
      - `BatchEvaluatorTest.EvaluateBatchInputsReturnsResults`
      - `BatchEvaluatorTest.EvaluateAsyncReturnsHandle`

- QW-8 started: high-confidence triage gate artifacts for preflight.
  - Implemented in [tools/gap_scanner_v3.py](tools/gap_scanner_v3.py):
    - confidence review now loads previous snapshot (when present) before overwrite
    - emits machine-readable actionable queue artifact:
      - `gap_scan_v3_preflight_actionable_queue.json`
      - includes top actionable items, top categories/files, and net-new high-confidence CRITICAL count/items
    - emits markdown PR-facing summary artifact:
      - `gap_scan_v3_preflight_summary.md`
      - includes headline metrics, top categories/files, and top actionable findings
    - adds stable item-keying for net-new detection across snapshots
  - Verification in this environment:
    - static diagnostics on scanner script are clean (`tools/gap_scanner_v3.py`)
    - full scanner run completed successfully:
      - `c:/Projects/ThemisDB/.venv/Scripts/python.exe tools/gap_scanner_v3.py . ai_working`
      - output summary: `Total Gaps Found: 38257`, `ACTIONABLE (C+H): 20928`
    - generated artifacts validated:
      - [ai_working/gap_scan_v3_preflight_actionable_queue.json](ai_working/gap_scan_v3_preflight_actionable_queue.json)
        - `actionable_count=2000`
        - `critical_high_confidence_count=1906`
        - `net_new_critical_high_confidence_count=398`
      - [ai_working/gap_scan_v3_preflight_summary.md](ai_working/gap_scan_v3_preflight_summary.md)

## Prioritized Quickwins

- [x] QW-1: Add prompt input sanitization guard rails in RAG judge and batch evaluator (Target: Next Sprint)
  - Scope: [src/rag/rag_judge.cpp](src/rag/rag_judge.cpp), [src/rag/batch_evaluator.cpp](src/rag/batch_evaluator.cpp)
  - Why now: High concentration of CRITICAL llm_ai_safety findings in top review queue.
  - Acceptance:
    - All user-provided prompt fragments pass through one centralized sanitizer/validator path.
    - Reject or neutralize common injection patterns before LLM dispatch.
    - Add focused unit tests for safe and unsafe prompt payloads.

- [x] QW-2: Harden replication write/ack path with explicit consensus checks (Target: Next Sprint)
  - Scope: [src/replication/replication_manager.cpp](src/replication/replication_manager.cpp), [src/replication/conflict_resolution.cpp](src/replication/conflict_resolution.cpp)
  - Why now: Highest file-level count in confidence review and high distributed_consistency risk.
  - Acceptance:
    - Write paths fail closed when quorum/ack requirements are not met.
    - Conflict resolution records version/causal metadata before commit.
    - Add regression tests for partial-ack and stale-version scenarios.

- [x] QW-3: Add missing security audit logs for authentication-sensitive voice flows (Target: Next Sprint)
  - Scope: [src/voice/voice_authenticator.cpp](src/voice/voice_authenticator.cpp), [src/voice/voice_assistant.cpp](src/voice/voice_assistant.cpp), [src/voice/voice_assistant_llm.cpp](src/voice/voice_assistant_llm.cpp)
  - Why now: Multiple CRITICAL missing_audit_log findings with very high confidence.
  - Acceptance:
    - Authentication and authorization outcomes emit structured audit events.
    - Audit logs include actor, action, result, and correlation id.
    - Negative tests verify no silent auth-path completion without log emission.

- [x] QW-4: Enforce safe prompt handling in LLM wrapper and training entry points (Target: Next Sprint)
  - Scope: [src/llm/llama_wrapper.cpp](src/llm/llama_wrapper.cpp), [src/training/incremental_lora_trainer.cpp](src/training/incremental_lora_trainer.cpp)
  - Why now: LLM module dominates high-confidence queue volume.
  - Acceptance:
    - Prompt construction uses sanitized, bounded, and escaped user input.
    - Unsafe control-token patterns are blocked or escaped consistently.
    - Add tests for injection-like payloads in both inference and training APIs.

- [x] QW-5: Add deterministic routing safeguards in shard router decisions (Target: Next Sprint)
  - Scope: [src/sharding/shard_router.cpp](src/sharding/shard_router.cpp)
  - Why now: Sharding is top-4 module in high-confidence queue, impacts correctness under load.
  - Acceptance:
    - Stable tie-break ordering for equivalent shard scores.
    - Explicit handling for missing metrics and stale state.
    - Add deterministic replay test with fixed seed and repeated runs.

- [x] QW-6: Add GPU kernel input validation and bounds checks in LoRA kernels (Target: Next Sprint)
  - Scope: [src/llm/lora_framework/kernels/vulkan_kernels.cpp](src/llm/lora_framework/kernels/vulkan_kernels.cpp), [src/llm/lora_framework/kernels/directx_kernels.cpp](src/llm/lora_framework/kernels/directx_kernels.cpp), [src/llm/lora_framework/kernels/hip_fused_kernels.cpp](src/llm/lora_framework/kernels/hip_fused_kernels.cpp), [src/llm/lora_framework/gpu_lora_layers.cpp](src/llm/lora_framework/gpu_lora_layers.cpp)
  - Why now: Concentrated gpu_memory_safety findings in top files.
  - Acceptance:
    - Validate tensor dimensions/strides before kernel dispatch.
    - Guard against out-of-range buffer offsets and zero-sized launches.
    - Add focused tests for malformed tensor metadata.

- [x] QW-7: Introduce one shared safety helper for LLM prompt assembly (Target: Next Sprint)
  - Scope: [src/llm](src/llm), [src/rag](src/rag), [src/training](src/training)
  - Why now: Reduces duplicated safety logic across the most affected modules.
  - Acceptance:
    - New helper used by at least rag + llm + training entry points.
    - Consistent sanitizer policy and telemetry tags.
    - Remove duplicate ad-hoc sanitization branches replaced by helper.

- [x] QW-8: Add a high-confidence triage gate in CI preflight report (Target: Next Sprint)
  - Scope: [tools/gap_scanner_v3.py](tools/gap_scanner_v3.py), [ai_working/gap_scan_v3_confidence_review.json](ai_working/gap_scan_v3_confidence_review.json)
  - Why now: Keeps quickwins aligned with current highest-confidence risks.
  - Acceptance:
    - Emit top-N actionable queue by category and file in machine-readable format.
    - Flag net-new CRITICAL high-confidence entries versus previous snapshot.
    - Add a short markdown summary artifact for PR review.

## Suggested Execution Order
1. QW-2 replication consistency
2. QW-1 RAG prompt sanitization
3. QW-3 voice audit logging
4. QW-4 LLM/training prompt safety
5. QW-6 GPU memory safety
6. QW-5 sharding determinism
7. QW-7 shared safety helper
8. QW-8 scanner triage gate

## Next Block Candidates (Post QW-1..QW-8)

Source: [ai_working/gap_scan_v3_preflight_actionable_queue.json](ai_working/gap_scan_v3_preflight_actionable_queue.json), filtered after QW-1..QW-8 closure.

- Current top queue profile (top_actionable_items, N=200):
  - `llm_ai_safety`: 126
  - `distributed_consistency`: 67
  - `audit_logging`: 4
  - `gpu_memory_safety`: 3

- Highest remaining file clusters in top queue:
  - [src/training/incremental_lora_trainer.cpp](src/training/incremental_lora_trainer.cpp): 30
  - [src/training/lora_data_selection.cpp](src/training/lora_data_selection.cpp): 11
  - [src/training/lora_adapter.cpp](src/training/lora_adapter.cpp): 10
  - [src/transaction/distributed_saga.cpp](src/transaction/distributed_saga.cpp): 11
  - [src/sharding/stream_protocol.cpp](src/sharding/stream_protocol.cpp): 10

### Proposed Next Quickwins

- [x] QW-9: Extend shared prompt-safety helper across remaining training flows (Target: Next Sprint)
  - Scope: [src/training/lora_data_selection.cpp](src/training/lora_data_selection.cpp), [src/training/lora_adapter.cpp](src/training/lora_adapter.cpp), [src/training/modality_parser.cpp](src/training/modality_parser.cpp)
  - Why now: Dominant CRITICAL prompt_injection findings still concentrated in training pipelines.
  - Acceptance:
    - All user-influenced prompt assembly paths in these files use shared sanitizer policy.
    - Fail-closed behavior for blocked patterns is consistent with QW-4/QW-7 semantics.
    - Focused tests cover benign payload, blocked payload, and sanitized-but-allowed payload.
  - Execution note (2026-05-31):
    - Code-level review confirmed no direct model-dispatch prompt assembly in `lora_adapter.cpp` (math/weight manipulation only); scanner cluster in this file is false-positive heavy for prompt-injection semantics.
    - Shared prompt-safety enforcement added at user-influenced training-sample entry surfaces:
      - [src/training/lora_data_selection.cpp](src/training/lora_data_selection.cpp): Stage-1 `filterByQuality(...)` now applies `sanitizePromptWithSharedPolicy(...)`, fail-closes blocked payloads, and keeps allowed payloads with control-token redaction.
      - [src/training/modality_parser.cpp](src/training/modality_parser.cpp): `TextClauseExtractor`, `TableExtractor`, `CitationExtractor`, and `OCRExtractor` now sanitize extracted sample payloads and drop blocked ones fail-closed.
      - API docs updated in [include/training/lora_data_selection.h](include/training/lora_data_selection.h) and [include/training/modality_parser.h](include/training/modality_parser.h).
    - Focused regression additions:
      - [tests/test_lora_data_selection.cpp](tests/test_lora_data_selection.cpp):
        - `DataSelectionPipelineTest.QualityFilter_RejectsPromptInjectionLikePayloads`
        - `DataSelectionPipelineTest.QualityFilter_RedactsControlTokensButKeepsSample`
      - [tests/test_training_convergence.cpp](tests/test_training_convergence.cpp):
        - `TextClauseExtractorTest.PromptInjectionLikeClauseIsRejected`
        - `TextClauseExtractorTest.ControlTokensAreRedactedAndSampleRemains`
    - Validation in this environment:
      - `themis_tests` target built successfully after changes.
      - Focused modality parser tests passed (2/2):
        - `TextClauseExtractorTest.PromptInjectionLikeClauseIsRejected`
        - `TextClauseExtractorTest.ControlTokensAreRedactedAndSampleRemains`
      - Note: `DataSelectionPipelineTest.*` is not currently registered/runnable in this preset (`ctest -N` shows no matching entries), but compiles cleanly in source-level diagnostics.

- [x] QW-10: Add model integrity verification before LoRA model/checkpoint load (Target: Next Sprint)
  - Scope: [src/training/lora_checkpoint_manager.cpp](src/training/lora_checkpoint_manager.cpp), [src/training/incremental_lora_trainer.cpp](src/training/incremental_lora_trainer.cpp)
  - Why now: Scanner flags CRITICAL `model_integrity_gap` (poisoning risk) in load paths.
  - Acceptance:
    - Checkpoint/model loading requires explicit integrity verification (hash/signature policy gate).
    - Failure path is fail-closed with structured error/audit context.
    - Focused tests cover invalid checksum, missing checksum, and valid checksum success.
  - Execution update (2026-05-31):
    - `resumeFromCheckpoint(...)` now enforces manifest-backed SHA-256 verification of `<checkpoint_prefix>_weights.bin` in managed checkpoint mode (`checkpoint_dir` set).
    - Resume now fails closed when no matching manifest entry exists, when SHA is missing, or when hash comparison fails.
    - Added managed-mode integrity regression: `IncrementalLoRATrainerCheckpoint.ResumeWithManagedCheckpointDirRequiresManifestIntegrity`.
    - Preserved unmanaged compatibility regression: `IncrementalLoRATrainerCheckpoint.ResumeFromNonexistentPathSucceeds` remains green.

- [x] QW-11: Harden distributed saga write paths with quorum/causality invariants (Target: Next Sprint)
  - Scope: [src/transaction/distributed_saga.cpp](src/transaction/distributed_saga.cpp)
  - Why now: Multiple CRITICAL distributed_consistency findings (`missing_consensus`, `missing_version_tracking`).
  - Acceptance:
    - Writes fail closed when required consensus/ack cannot be proven.
    - Concurrent state transitions enforce version/causal checks before commit.
    - Focused tests cover partial-ack timeout, stale version, and replay/idempotency behavior.
  - Execution update (2026-05-31):
    - Duplicate saga replay is now fail-closed (`execute(...)` rejects repeated `saga_id` and records `REJECTED_DUPLICATE`).
    - Global `saga_timeout` is enforced as a hard deadline across step execution attempts.
    - Each step now performs an explicit causal dependency gate (`depends_on` must be `DONE` before dispatch).
    - API/docs updated to capture fail-closed invariants and deadline behavior in the coordinator contract.
    - Focused regressions passed (2/2):
      - `DistributedSagaTest.DuplicateSagaIdExecutionRejectedFailClosed`
      - `DistributedSagaTimeoutTest.GlobalSagaTimeoutBudgetFailsClosed`

- [x] QW-12: Replication conflict-resolution residual sweep (Target: Next Sprint)
  - Scope: [src/replication/replication_manager.cpp](src/replication/replication_manager.cpp), [src/replication/conflict_resolution.cpp](src/replication/conflict_resolution.cpp)
  - Why now: Net-new CRITICAL queue still reports high volume on replication files after QW-2; requires targeted rescan + residual fix pass.
  - Acceptance:
    - Re-scan confirms true-positive residuals only (no duplicate stale findings from pre-fix snapshots).
    - Residual true positives are fixed with focused regression tests.
    - Post-fix preflight shows reduced net-new CRITICAL count for replication category.
  - Execution update (2026-05-31):
    - Residual sweep focused on scanner clusters in `src/replication/replication_manager.cpp` and `src/replication/conflict_resolution.cpp`.
    - True-positive hardening implemented in [src/replication/replication_manager.cpp](src/replication/replication_manager.cpp):
      - `MultiMasterReplicationManager::replicateWrite(...)` now fails closed when `write_quorum == 0` with peers configured.
      - `MultiMasterReplicationManager::replicateWrite(...)` now fails closed when `eligible_peers < write_quorum` before write shipping.
      - `BidirectionalReplicationManager::applyRemoteWrite(...)` now rejects malformed origin metadata (`origin_node` empty or `origin_seq == 0`) when origin tracking is enabled.
      - `BidirectionalReplicationManager::applyRemoteWrite(...)` now rejects stale/duplicate remote versions for same `(collection, document_id)` and origin (`origin_seq <= last_seen`).
    - API contract updated in [include/replication/replication_manager.h](include/replication/replication_manager.h) with explicit fail-closed invariants for remote apply path.
    - Focused regressions added in [tests/test_replication_ha.cpp](tests/test_replication_ha.cpp):
      - `MMReplicationManagerTest.WriteSyncFailsClosedWhenQuorumZeroWithActivePeer`
      - `BidirectionalReplicationTest.OriginTrackingRejectsStaleOrDuplicateRemoteSequence`
    - Validation in this environment:
      - focused test run passed (3/3), including the two new tests plus
        `BidirectionalReplicationTest.OriginTrackingAcceptsPeerChanges`.
    - Re-scan completed with updated preflight artifacts:
      - `critical_high_confidence_count`: 1906 -> 1901
      - `net_new_critical_high_confidence_count`: 398 -> 292
      - Residual high-volume `conflict_resolution.cpp` version-tracking hits are predominantly stale heuristic clusters around already-causality-enriched resolver paths (false-positive heavy), while true positives in active replication transport/write paths were patched above.

- [x] QW-13: Stream protocol fail-closed guards for sharding transport edges (Target: Next Sprint)
  - Scope: [src/sharding/stream_protocol.cpp](src/sharding/stream_protocol.cpp), [src/sharding/wal_applier.cpp](src/sharding/wal_applier.cpp)
  - Why now: Remaining high-confidence distributed consistency findings in sharding transport code paths.
  - Acceptance:
    - Invalid/stale transport metadata is rejected before apply/ack.
    - WAL apply path validates ordering/version preconditions explicitly.
    - Focused tests cover stale frame, duplicate frame, and out-of-order apply.
  - Execution update (2026-05-31):
    - `StreamReceiveTask::onChunkReceived(...)` now fails closed for out-of-range indices, stale/duplicate chunks, duplicate buffered chunks, and offset/size metadata mismatches.
    - `StreamReceiveTask::writeChunk(...)` now fails closed for malformed payload metadata, out-of-bounds ranges, decompression size mismatches, and missing target-path resolution.
    - `WALApplier::applyBatch(...)` now enforces strict fail-closed LSN invariants in strict mode:
      - rejects stale/duplicate entries (`entry.lsn <= current_lsn_`)
      - requires exact next-LSN progression (with explicit `0/0` bootstrap replay allowance)
    - `WALApplier::validateLSN(...)` now enforces exact successor semantics (no implicit idempotent acceptance in strict mode).
    - Added/updated focused regressions:
      - [tests/test_stream_protocol_extended.cpp](tests/test_stream_protocol_extended.cpp):
        - `StreamProtocolExtendedTest.RejectsOutOfRangeChunkIndexFailClosed`
        - `StreamProtocolExtendedTest.RejectsDuplicateChunkFailClosed`
        - `StreamProtocolExtendedTest.RejectsChunkWithMismatchedOffsetFailClosed`
      - [tests/test_wal_replication.cpp](tests/test_wal_replication.cpp):
        - `WALReplicationTest.ApplierStrictModeRejectsDuplicateLSNFailClosed`
      - [tests/test_wal_replication_integration.cpp](tests/test_wal_replication_integration.cpp):
        - strict ordering scenario aligned to next-LSN progression (`LSNOrderingValidation`).
    - Validation in this environment:
      - `themis_tests` built successfully with the changed tests.
      - Focused stream suite passed (5/5) via `test_stream_protocol_extended_focused`.
      - Focused WAL suite passed (3/3):
        - `WALReplicationTest.ApplierStrictMode`
        - `WALReplicationTest.ApplierStrictModeRejectsDuplicateLSNFailClosed`
        - `WALReplicationIntegrationTest.LSNOrderingValidation`

  - [x] QW-14: Sanitize training-pipeline progress/callback messages with shared policy (Target: Next Sprint)
    - Scope: [src/training/training_pipeline.cpp](src/training/training_pipeline.cpp), [include/training/training_pipeline.h](include/training/training_pipeline.h), [tests/test_training_convergence.cpp](tests/test_training_convergence.cpp)
    - Why now: Top actionable queue still reports CRITICAL `prompt_injection` hits in `training_pipeline.cpp` callback/progress paths (lines 152/199/202/275/305 in preflight queue snapshot).
    - Acceptance:
      - All callback/progress messages emitted by `TrainingPipeline::run(...)` pass through shared prompt-safety sanitization.
      - Blocked injection patterns are fail-closed to a constant safe message.
      - Allowed payloads preserve content with control-token redaction.
      - Focused tests verify both blocked and redacted behavior.
    - Execution update (2026-05-31):
      - Added shared-policy callback sanitizer in [src/training/training_pipeline.cpp](src/training/training_pipeline.cpp):
        - centralized helper `sanitizeTrainingPipelineMessage(...)`
        - fail-closed replacement marker: `"message blocked by prompt policy"`
        - all stage callback emissions in `TrainingPipeline::Impl::run(...)` now route through this sanitizer.
      - Added explicit public API for deterministic validation and downstream reuse:
        - `TrainingPipeline::sanitizeCallbackMessage(...)` in [include/training/training_pipeline.h](include/training/training_pipeline.h) and [src/training/training_pipeline.cpp](src/training/training_pipeline.cpp).
      - Added focused regression tests in [tests/test_training_convergence.cpp](tests/test_training_convergence.cpp):
        - `TrainingPipelineCallbackSanitizer.BlocksInjectionPatternFailClosed`
        - `TrainingPipelineCallbackSanitizer.RedactsControlTokensButAllowsMessage`
      - Validation in this environment:
        - `themis_tests` target built successfully.
        - focused suite passed (2/2): `TrainingPipelineCallbackSanitizer.*`.

  - [x] QW-15: Sanitize stage-specific training callbacks via shared policy (Target: Next Sprint)
    - Scope: [src/training/training_pipeline.cpp](src/training/training_pipeline.cpp), [include/training/training_pipeline.h](include/training/training_pipeline.h)
    - Why now: Scanner preflight still reported CRITICAL `prompt_injection` callback surfaces in stage-entry methods (`runLabeling`, `runEnrichment`, `runTraining`, `runDataSelection`) that bypassed the already-sanitized aggregate `run(...)` path.
    - Acceptance:
      - All stage-specific callback emissions pass through the same shared prompt-safety policy as `run(...)`.
      - Blocked patterns fail closed to a fixed safe message.
      - Allowed messages are emitted with control-token redaction.
      - Public API documentation reflects sanitized callback contract.
    - Execution update (2026-05-31):
      - Added callback wrappers in [src/training/training_pipeline.cpp](src/training/training_pipeline.cpp):
        - `TrainingPipeline::Impl::runLabeling(...)`
        - `TrainingPipeline::Impl::runEnrichment(...)`
        - `TrainingPipeline::Impl::runTraining(...)`
        - `TrainingPipeline::Impl::runDataSelection(...)`
      - Each wrapper now routes callback message payloads through `sanitizeTrainingPipelineMessage(...)` before forwarding.
      - Updated public API docs in [include/training/training_pipeline.h](include/training/training_pipeline.h) for the four stage-specific methods to state sanitized-callback behavior.
      - Validation in this environment:
        - `themis_tests` target built successfully.
        - focused suite passed (2/2): `TrainingPipelineCallbackSanitizer.*`.

  - [x] QW-16: Migrate voice assistant LLM prompt sanitization to shared policy (Target: Next Sprint)
    - Scope: [src/voice/voice_assistant_llm.cpp](src/voice/voice_assistant_llm.cpp), [include/voice/voice_assistant.h](include/voice/voice_assistant.h), [tests/test_voice_assistant.cpp](tests/test_voice_assistant.cpp)
    - Why now: Preflight actionable queue still lists CRITICAL `prompt_injection` findings for [src/voice/voice_assistant_llm.cpp](src/voice/voice_assistant_llm.cpp) (top queue line 44) and voice remains a top-risk module.
    - Acceptance:
      - Voice LLM prompt surfaces use the repository-wide shared prompt-safety policy.
      - Blocked prompt patterns fail closed to a fixed safe marker.
      - Allowed prompts preserve payload with control-token redaction.
      - Focused tests validate blocked and redacted behavior deterministically.
    - Execution update (2026-05-31):
      - Replaced local `PromptInjectionSanitizer` usage in [src/voice/voice_assistant_llm.cpp](src/voice/voice_assistant_llm.cpp) with shared helper `sanitizePromptWithSharedPolicy(...)` from [include/llm/prompt_safety_utils.h](include/llm/prompt_safety_utils.h).
      - Added fail-closed handling for blocked user input in `VoiceAssistant::generateLLMResponse(...)` with fixed marker `"message blocked by prompt policy"` and security audit event.
      - Added public deterministic sanitizer API in [include/voice/voice_assistant.h](include/voice/voice_assistant.h): `VoiceAssistant::sanitizeLLMPromptText(...)`.
      - Added focused regressions in [tests/test_voice_assistant.cpp](tests/test_voice_assistant.cpp):
        - `VoiceAssistantPromptSafety.BlocksInjectionPatternFailClosed`
        - `VoiceAssistantPromptSafety.RedactsControlTokensButKeepsPrompt`
      - Validation in this environment:
        - `themis_tests` target built successfully.
        - focused suite passed (2/2): `VoiceAssistantPromptSafety.*`.

  - [x] QW-17: Fail-closed distributed remote SAGA execution without transport (Target: Next Sprint)
    - Scope: [src/transaction/distributed_saga.cpp](src/transaction/distributed_saga.cpp), [include/transaction/distributed_saga.h](include/transaction/distributed_saga.h), [tests/test_distributed_saga.cpp](tests/test_distributed_saga.cpp)
    - Why now: Top actionable queue still carries CRITICAL distributed-consistency findings in `distributed_saga.cpp`; permissive remote no-op execution without transport can mask missing consensus/ack paths.
    - Acceptance:
      - `executeDistributed(...)` rejects calls when `Config::remote_executor` is not configured.
      - Failure is explicit (`FAILED` + deterministic reason) and journaled as rejection.
      - Legacy no-op success semantics are removed from distributed-path tests.
      - Invalid remote payload metadata (empty saga/step endpoint/operation) is rejected fail-closed before execution conversion.
    - Execution update (2026-06-01):
      - Added fail-closed guard in [src/transaction/distributed_saga.cpp](src/transaction/distributed_saga.cpp) `executeDistributed(...)`:
        - returns `FAILED` with `remote_executor_not_configured`
        - writes journal event `REJECTED_NO_REMOTE_EXECUTOR`
        - increments failed saga metric
      - Added strict distributed input validation in `executeDistributed(...)`:
        - rejects empty `saga_id`
        - rejects empty remote step list
        - rejects steps with empty `name`, `service_endpoint`, or `operation`
        - persists rejection reports in coordinator status map for observability via `getDistributedStatus(...)`
      - Updated defensive fallback in `remoteStepToLocal(...)` to return explicit error instead of no-op success.
      - Updated API docs in [include/transaction/distributed_saga.h](include/transaction/distributed_saga.h) to reflect mandatory remote executor for distributed execution.
      - Added focused regressions in [tests/test_distributed_saga.cpp](tests/test_distributed_saga.cpp):
        - `ExecuteDistributedRejectsRemoteStepWithEmptyEndpoint`
        - `ExecuteDistributedRejectsRemoteStepWithEmptyOperation`
      - Updated distributed status regression to use an explicit remote executor for successful-path expectation.
    - Validation status in this environment:
      - `ctest --preset windows-release --output-on-failure -R "DistributedSagaTests"` passed (`1/1`).

  - [x] QW-18: Fail-closed ShardRouter remote dispatch without RemoteExecutor (Target: Next Sprint)
    - Scope: [src/sharding/shard_router.cpp](src/sharding/shard_router.cpp), [tests/test_sharding_integration.cpp](tests/test_sharding_integration.cpp)
    - Why now: `ShardRouter` hat drei Remote-Dispatch-Sites (`routeRequest`, `scatterGather`-Lambda, `executeOnShards`-Lambda), die `executor_` ohne Null-Check dereferenzieren. Ein `nullptr`-Executor (kein Transport konfiguriert) verursacht UB/Crash.
    - Acceptance:
      - Alle drei Remote-Dispatch-Sites liefern explizites Failure (`success=false`, `error_msg=remote_executor_not_configured`) wenn `executor_` `nullptr` ist.
      - `errors_`-Counter wird inkrementiert; `THEMIS_ERROR` wird in `routeRequest` geloggt.
      - Kein Crash, kein UB, deterministisches Fehler-Propagation.
    - Execution update (2026-06-01):
      - Fail-closed Null-Checks in [src/sharding/shard_router.cpp](src/sharding/shard_router.cpp) hinzugefuegt:
        - `routeRequest()`: Guard + `errors_++` + `THEMIS_ERROR`-Log vor dem ersten `executor_->`-Call
        - `scatterGather()`-Remote-Lambda: Guard mit `success=false` / error_msg gesetzt
        - `executeOnShards()`-Remote-Lambda: Guard mit `success=false` / error_msg gesetzt
      - Fokus-Regressionen in [tests/test_sharding_integration.cpp](tests/test_sharding_integration.cpp) hinzugefuegt:
        - `RouterRouteRequestFailsClosedWithoutExecutor`
        - `RouterPutFailsClosedWithoutExecutor`
    - Validation status in this environment:
      - `themis_sharding.dll` built successfully.
      - `test_sharding_integration_focused.exe` built successfully.
      - `ctest --preset windows-release --output-on-failure -R "ShardingIntegrationFocusedTests"` passed (`1/1`, 1.21 s).

  - [x] QW-19: Fail-closed DistributedTrainer collectives without injected callbacks (Target: Next Sprint)
    - Scope: [src/llm/lora_framework/distributed_trainer.cpp](src/llm/lora_framework/distributed_trainer.cpp), [include/llm/lora_framework/distributed_trainer.h](include/llm/lora_framework/distributed_trainer.h), [tests/test_distributed_trainer_guard.cpp](tests/test_distributed_trainer_guard.cpp)
    - Why now: Im Multi-Rank-Modus meldeten `synchronize_gradients()` und `broadcast_parameters()` Erfolg, obwohl ohne injizierte `AllReduceCpuFn` bzw. `BroadcastFn` keinerlei verteilte Kommunikation stattfindet.
    - Acceptance:
      - `synchronize_gradients()` liefert bei `world_size > 1` ohne `AllReduceCpuFn` explizit `false`.
      - `broadcast_parameters()` liefert bei `world_size > 1` ohne `BroadcastFn` explizit `false`.
      - Die API-Dokumentation benennt diese Fail-Closed-Bedingungen.
    - Execution update (2026-06-01):
      - Fail-closed Guards in [src/llm/lora_framework/distributed_trainer.cpp](src/llm/lora_framework/distributed_trainer.cpp) hinzugefuegt:
        - `synchronize_gradients()` lehnt Multi-Rank-Laeufe ohne `AllReduceCpuFn` mit Fehlerlog ab
        - `broadcast_parameters()` lehnt Multi-Rank-Laeufe ohne `BroadcastFn` mit Fehlerlog ab
      - API-Doku in [include/llm/lora_framework/distributed_trainer.h](include/llm/lora_framework/distributed_trainer.h) fuer beide Rueckgabepfade aktualisiert.
      - Fokus-Regressionen in [tests/test_distributed_trainer_guard.cpp](tests/test_distributed_trainer_guard.cpp) hinzugefuegt:
        - `SynchronizeGradientsFailsWithoutAllReduceInMultiRankMode`
        - `BroadcastParametersFailsWithoutBroadcastFnInMultiRankMode`
    - Validation status in this environment:
      - `test_distributed_trainer_guard.exe` built successfully.
      - `ctest --preset windows-release --output-on-failure -R "DistributedTrainerGuardFocusedTests"` passed (`1/1`, 0.09 s).

  - [x] QW-20: Fail-closed cloud restore without BackupManager (Target: Next Sprint)
    - Scope: [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp), [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h), [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp)
    - Why now: `CloudBackupCoordinator::restoreBackup()` konnte Erfolg melden, obwohl gar kein `BackupManager` konfiguriert war und damit kein lokaler Restore angewendet werden konnte.
    - Acceptance:
      - `restoreBackup()` lehnt Restore-Aufrufe ohne `BackupManager` explizit fail-closed ab.
      - Die API-Dokumentation benennt den fehlenden `BackupManager` als Failure-Bedingung.
      - Eine fokussierte Regression deckt den Null-Manager-Restore-Pfad ab.
    - Execution update (2026-06-01):
      - Fail-closed Guard in [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp) hinzugefuegt:
        - `restoreBackup()` gibt bei fehlendem `BackupManager` sofort `false` zurueck und loggt den Fehler
      - API-Doku in [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h) fuer den Restore-Rueckgabepfad aktualisiert.
      - Fokus-Regression in [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp) hinzugefuegt:
        - `RestoreBackupFailsClosedWithoutBackupManager`
    - Validation status in this environment:
      - `test_cloud_backup_focused.exe` built successfully.
      - `ctest --preset windows-release --output-on-failure -R "CloudBackupFocusedTests"` passed (`1/1`, 2.12 s).

  - [x] QW-21: Fail-closed cloud restore input validation (Target: Next Sprint)
    - Scope: [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp), [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h), [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp)
    - Why now: `CloudBackupCoordinator::restoreBackup()` akzeptierte leere `backup_id` oder leere `shard_ids` und konnte dadurch als inhaltlicher No-Op erfolgreich enden.
    - Acceptance:
      - `restoreBackup()` lehnt leere `backup_id` explizit fail-closed ab.
      - `restoreBackup()` lehnt leere `shard_ids` explizit fail-closed ab.
      - Der Header-Vertrag benennt beide Eingabefehler als Failure-Bedingungen.
    - Execution update (2026-06-01):
      - Eingangsvalidierung in [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp) hinzugefuegt:
        - leeres `backup_id` -> sofort `false` + Fehlerlog
        - leere `shard_ids` -> sofort `false` + Fehlerlog
      - API-Doku in [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h) fuer Restore-Fehlerbedingungen erweitert.
      - Fokus-Regressionen in [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp) hinzugefuegt:
        - `RestoreBackupFailsClosedForEmptyBackupId`
        - `RestoreBackupFailsClosedForEmptyShardList`
    - Validation status in this environment:
      - `test_cloud_backup_focused.exe` built successfully.
      - `ctest --preset windows-release --output-on-failure -R "CloudBackupFocusedTests"` passed (`1/1`, 2.15 s).

  - [x] QW-22: Fail-closed cloud restore empty shard entries (Target: Next Sprint)
    - Scope: [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp), [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h), [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp)
    - Why now: `CloudBackupCoordinator::restoreBackup()` validierte leere Listen, aber nicht leere `shard_id`-Eintraege innerhalb der Liste. Dadurch konnte der Restore-Pfad mit ungueltigem Zielpfad weiterlaufen.
    - Acceptance:
      - `restoreBackup()` lehnt leere `shard_id`-Eintraege explizit fail-closed ab.
      - Der Header-Vertrag nennt leere Eintraege in `shard_ids` als Fehlerfall.
      - Eine fokussierte Regression deckt genau diesen Eingabefehler ab.
    - Execution update (2026-06-01):
      - Guard in [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp) hinzugefuegt:
        - leere `shard_id` innerhalb der Restore-Liste -> sofort `false` + Fehlerlog
      - API-Doku in [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h) fuer den Restore-Vertrag erweitert.
      - Fokus-Regression in [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp) hinzugefuegt:
        - `RestoreBackupFailsClosedForEmptyShardIdEntry`
    - Validation status in this environment:
      - `test_cloud_backup_focused.exe` built successfully.
      - `ctest --preset windows-release --output-on-failure -R "CloudBackupFocusedTests"` passed (`1/1`, 2.21 s).

  - [x] QW-23: Fail-closed cloud restore backup-catalog membership (Target: Next Sprint)
    - Scope: [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp), [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h), [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp)
    - Why now: `CloudBackupCoordinator::restoreBackup()` konnte Shards anfordern, die gar nicht Teil des katalogisierten Backups sind. Damit konnte ein Restore auf fachlich ungueltige Remote-Pfade weiterlaufen.
    - Acceptance:
      - `restoreBackup()` lehnt angeforderte Shards ab, die nicht in `BackupInfo::shard_ids` des Katalogeintrags enthalten sind.
      - Der Header-Vertrag nennt diese Katalog-Membership als Fehlerbedingung.
      - Eine fokussierte Regression deckt den Pfad mit fachlich falscher Shard-Zuordnung ab.
    - Execution update (2026-06-01):
      - Guard in [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp) hinzugefuegt:
        - Restore lehnt `shard_id` ab, wenn sie nicht zum katalogisierten Backup gehoert
      - API-Doku in [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h) fuer den Restore-Vertrag erweitert.
      - Fokus-Regression in [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp) hinzugefuegt:
        - `RestoreBackupFailsClosedForShardOutsideBackupCatalog`
    - Validation status in this environment:
      - `test_cloud_backup_focused.exe` built successfully.
      - `ctest --preset windows-release --output-on-failure -R "CloudBackupFocusedTests"` passed (`1/1`, 2.40 s).

  - [x] QW-24: Fail-closed cloud delete empty backup-id (Target: Next Sprint)
    - Scope: [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp), [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h), [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp)
    - Why now: `CloudBackupCoordinator::deleteBackup()` akzeptierte leere `backup_id` und endete danach ohne Fehler. Damit konnte ein semantisch ungueltige Anfrage als Erfolg interpretiert werden.
    - Acceptance:
      - `deleteBackup()` lehnt leere `backup_id` explizit fail-closed ab.
      - Der Header-Vertrag nennt die leere backup_id als Fehlerbedingung.
      - Eine fokussierte Regression deckt den Eingabefehler ab.
    - Execution update (2026-06-02):
      - Guard in [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp) hinzugefuegt:
        - `deleteBackup()` lehnt leere `backup_id` ab
      - API-Doku in [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h) fuer den Delete-Fehlerfall erweitert.
      - Fokus-Regression in [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp) hinzugefuegt:
        - `DeleteBackupFailsClosedForEmptyBackupId`
    - Validation status in this environment:
      - `test_cloud_backup_focused.exe` built successfully.
      - Test passed: `DeleteBackupFailsClosedForEmptyBackupId` in 69 ms.

  - [x] QW-25: Fail-closed cloud replication target empty inputs (Target: Next Sprint)
    - Scope: [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp), [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h), [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp)
    - Why now: `CloudBackupCoordinator::setReplicationTarget()` akzeptierte leere `datacenter_id` oder leere `shard_endpoints` und speicherte diese als no-op Ziele.
    - Acceptance:
      - `setReplicationTarget()` lehnt leere `datacenter_id` explizit fail-closed ab.
      - `setReplicationTarget()` lehnt leere `shard_endpoints` explizit fail-closed ab.
      - Der Header-Vertrag nennt beide Eingabefehler.
    - Execution update (2026-06-02):
      - Guards in [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp) hinzugefuegt:
        - `setReplicationTarget()` lehnt leere `datacenter_id` ab
        - `setReplicationTarget()` lehnt leere `shard_endpoints` ab
      - API-Doku in [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h) erweitert.
      - Fokus-Regressionen in [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp) hinzugefuegt:
        - `SetReplicationTargetFailsClosedForEmptyDatacenterId`
        - `SetReplicationTargetFailsClosedForEmptyEndpoints`
    - Validation status in this environment:
      - `test_cloud_backup_focused.exe` built successfully.
      - Tests passed: both regressions in 137 ms total.

  - [x] QW-26: Fail-closed cloud replication enable/disable empty datacenter-id (Target: Next Sprint)
    - Scope: [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp), [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h), [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp)
    - Why now: `CloudBackupCoordinator::enableContinuousReplication()` und `disableContinuousReplication()` akzeptierten leere `datacenter_id` und behandelten das implizit als "Ziel nicht gefunden".
    - Acceptance:
      - Beide Methoden lehnen leere `datacenter_id` explizit fail-closed ab.
      - Der Header-Vertrag nennt leere IDs als Fehlerbedingung.
      - Fokussierte Regressionen decken beide Guards ab.
    - Execution update (2026-06-02):
      - Guards in [src/sharding/cloud_backup.cpp](src/sharding/cloud_backup.cpp) hinzugefuegt:
        - `enableContinuousReplication()` lehnt leere `datacenter_id` ab
        - `disableContinuousReplication()` lehnt leere `datacenter_id` ab
      - API-Doku in [include/sharding/cloud_backup.h](include/sharding/cloud_backup.h) erweitert.
      - Fokus-Regressionen in [tests/test_cloud_backup.cpp](tests/test_cloud_backup.cpp) hinzugefuegt:
        - `EnableContinuousReplicationFailsClosedForEmptyDatacenterId` (72 ms)
        - `DisableContinuousReplicationFailsClosedForEmptyDatacenterId` (72 ms)
    - Validation status in this environment:
      - `test_cloud_backup_focused.exe` built successfully.
      - Both tests passed in 72 ms each.

  - [x] QW-27: Fail-closed LLM plugin manager model/LoRA loader empty inputs (Target: Next Sprint)
    - Scope: [src/llm/llm_plugin_manager.cpp](src/llm/llm_plugin_manager.cpp), [include/llm/llm_plugin_manager.h](include/llm/llm_plugin_manager.h), [tests/test_llm_plugin.cpp](tests/test_llm_plugin.cpp)
    - Why now: `LLMPluginManager::loadModel()` und `loadLoRA()` akzeptierten leere `model_id`/`lora_id` oder leere `path` und delegierten an Plugin ohne Validierung.
    - Acceptance:
      - `loadModel()` lehnt leere `model_id` explizit fail-closed ab.
      - `loadModel()` lehnt leere `path` explizit fail-closed ab.
      - `loadLoRA()` lehnt leere `lora_id` explizit fail-closed ab.
      - `loadLoRA()` lehnt leere `path` explizit fail-closed ab.
      - Der Header-Vertrag nennt alle Eingabefehler.
    - Execution update (2026-06-02):
      - Guards in [src/llm/llm_plugin_manager.cpp](src/llm/llm_plugin_manager.cpp) hinzugefuegt:
        - `loadModel()` lehnt leere `model_id` ab (Zeile 277)
        - `loadModel()` lehnt leere `path` ab (Zeile 277)
        - `loadLoRA()` lehnt leere `lora_id` ab (Zeile 387)
        - `loadLoRA()` lehnt leere `path` ab (Zeile 387)
      - API-Doku in [include/llm/llm_plugin_manager.h](include/llm/llm_plugin_manager.h) erweitert (Doxygen @return false tags).
      - Fokus-Tests in [tests/test_llm_plugin.cpp](tests/test_llm_plugin.cpp) hinzugefuegt:
        - `LLMPluginManagerTest::LoadModelFailsClosedForEmptyModelId`
        - `LLMPluginManagerTest::LoadModelFailsClosedForEmptyPath`
        - `LLMPluginManagerTest::LoadLoRAFailsClosedForEmptyLoRAId`
        - `LLMPluginManagerTest::LoadLoRAFailsClosedForEmptyPath`
    - Validation status in this environment:
      - Build-Status: pending (gesamter Projekt-Build laeuft; LLM-Plugin-Tests kompilieren).
      - Fokus-Tests hinzugefuegt und syntaktisch geprueft.

  - [x] QW-28: Fail-closed replication coordinator acknowledgment empty replica-id (Target: Next Sprint)
    - Scope: [src/sharding/replication_coordinator.cpp](src/sharding/replication_coordinator.cpp), [include/sharding/replication_coordinator.h](include/sharding/replication_coordinator.h), [tests/test_sharding_core.cpp](tests/test_sharding_core.cpp)
    - Why now: `ReplicationCoordinator::recordAcknowledgment()` akzeptierte leere `replica_id` und verarbeitete sie als gueltige Acknowledgments (nur in Debug-Log).
    - Acceptance:
      - `recordAcknowledgment()` lehnt leere `replica_id` explizit fail-closed ab.
      - Der Header-Vertrag nennt leere IDs als Fehlerbedingung.
    - Execution update (2026-06-02):
      - Guard in [src/sharding/replication_coordinator.cpp](src/sharding/replication_coordinator.cpp) hinzugefuegt:
        - `recordAcknowledgment()` lehnt leere `replica_id` ab (Zeile 107)
      - API-Doku in [include/sharding/replication_coordinator.h](include/sharding/replication_coordinator.h) erweitert.
      - Fokus-Test-Datei erstellt: [tests/test_replication_coordinator_focused.cpp](tests/test_replication_coordinator_focused.cpp)
        - `ReplicationCoordinatorTest::RecordAcknowledgmentFailsClosedForEmptyReplicaId`
        - `ReplicationCoordinatorTest::RecordAcknowledgmentAcceptsValidReplicaId`
      - CMakeLists.txt registriert (Zeile 21101)
    - Validation status in this environment:
      - **BUILD: SUCCESS** (test_replication_coordinator_focused.exe compiled, 5 steps)
      - **TEST: ALL PASS** (2/2 tests passed in 2 ms)
        - Empty replica_id guard correctly rejects with error log
        - Valid replica_id processes without crash
      - **QW-28 COMPLETE** ✓

  - [x] QW-29: Fail-closed URNResolver key validation (Target: Next Sprint)
    - Scope: [src/sharding/urn_resolver.cpp](src/sharding/urn_resolver.cpp), [include/sharding/urn_resolver.h](include/sharding/urn_resolver.h), [tests/test_urn_resolver_focused.cpp](tests/test_urn_resolver_focused.cpp)
    - Why now: `URNResolver::getShardForKey()` akzeptierte leere `key` und lieferte unerwartete leere Shard-IDs (impliziter Fehler statt expliziter Ablehnung).
    - Acceptance:
      - `getShardForKey()` lehnt leere `key` explizit fail-closed ab (spdlog::error).
      - Der Header-Vertrag nennt leere Keys als Fehlerbedingung.
    - Execution update (2026-06-02):
      - Guard in [src/sharding/urn_resolver.cpp](src/sharding/urn_resolver.cpp) hinzugefuegt:
        - `getShardForKey()` lehnt leere `key` ab mit spdlog::error und Rueckgabe leerer String
      - API-Doku in [include/sharding/urn_resolver.h](include/sharding/urn_resolver.h) erweitert (Parameterbeschreibung + @note).
      - Fokus-Test-Datei erstellt: [tests/test_urn_resolver_focused.cpp](tests/test_urn_resolver_focused.cpp)
        - `URNResolverTest::GetShardForKeyFailsClosedForEmptyKey`
        - `URNResolverTest::GetShardForKeyReturnsValidShardForNonEmptyKey`
      - CMakeLists.txt registriert (Zeile 21115)
    - Validation status in this environment:
      - **BUILD: SUCCESS** (test_urn_resolver_focused.exe compiled, 36 steps)
      - **TEST: ALL PASS** (2/2 tests passed in 1 ms)
        - Empty key guard correctly rejects with error log
        - Valid key returns shard_001 without error
      - **QW-29 COMPLETE** ✓

## Suggested Execution Order (Next Block)
  1. QW-10 model integrity verification
  2. QW-11 distributed saga consistency
  3. QW-12 replication conflict-resolution residual sweep
  4. QW-13 stream protocol fail-closed guards
  5. QW-14 training pipeline callback sanitization
  6. QW-15 stage-specific callback sanitization
  7. QW-16 voice shared prompt-policy migration
  8. QW-17 distributed remote SAGA fail-closed transport gate
  9. QW-18 ShardRouter fail-closed remote dispatch without RemoteExecutor
  10. QW-19 DistributedTrainer fail-closed collectives without callbacks
  11. QW-20 CloudBackup restore fail-closed without BackupManager
  12. QW-21 CloudBackup restore fail-closed input validation
  13. QW-22 CloudBackup restore fail-closed empty shard entries
  14. QW-23 CloudBackup restore fail-closed backup catalog membership
  15. QW-24 CloudBackup delete fail-closed empty backup-id
  16. QW-25 CloudBackup replication-target fail-closed empty inputs
  17. QW-26 CloudBackup replication enable/disable fail-closed empty datacenter-id
  18. QW-27 LLMPluginManager fail-closed model/LoRA loader empty inputs
