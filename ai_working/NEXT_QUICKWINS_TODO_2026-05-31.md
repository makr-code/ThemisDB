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
