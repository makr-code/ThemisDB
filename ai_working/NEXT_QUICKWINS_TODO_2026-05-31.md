# Next Quickwins TODO (Gap Scanner v3)

Generated: 2026-05-31
Source scan: [ai_working/gap_scan_v3_summary.json](ai_working/gap_scan_v3_summary.json), [ai_working/gap_scan_v3_confidence_review.json](ai_working/gap_scan_v3_confidence_review.json)

## Snapshot
- Total gaps: 38201
- Actionable (critical + high): 20875
- Top modules by high-confidence queue: llm, rag, replication, sharding
- Dominant high-confidence categories: llm_ai_safety, distributed_consistency, audit_logging

## Execution Update (2026-05-31)
- QW-1 started: centralized prompt sanitization guard added in runtime evaluation path.
  - Implemented in [src/rag/rag_judge.cpp](src/rag/rag_judge.cpp) and [src/rag/batch_evaluator.cpp](src/rag/batch_evaluator.cpp).
  - Defense-in-depth behavior: document-level blocking visibility is preserved, while downstream evaluators receive sanitized prompt text.
  - Focused regression reruns after rebuilding `themis_tests` are green:
    - `RAGJudgeTest.RAS01_BenignDocumentsNotBlocked`
    - `RAGJudgeTest.RAS02_HighSeverityInjectionBlocked`
    - `RAGJudgeTest.RAS04_HighSeverityFoundButNotBlocked`
    - `BatchEvaluatorTest.EvaluateBatchInputsReturnsResults`
    - `BatchEvaluatorTest.EvaluateAsyncReturnsHandle`

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

## Prioritized Quickwins

- [ ] QW-1: Add prompt input sanitization guard rails in RAG judge and batch evaluator (Target: Next Sprint)
  - Scope: [src/rag/rag_judge.cpp](src/rag/rag_judge.cpp), [src/rag/batch_evaluator.cpp](src/rag/batch_evaluator.cpp)
  - Why now: High concentration of CRITICAL llm_ai_safety findings in top review queue.
  - Acceptance:
    - All user-provided prompt fragments pass through one centralized sanitizer/validator path.
    - Reject or neutralize common injection patterns before LLM dispatch.
    - Add focused unit tests for safe and unsafe prompt payloads.

- [ ] QW-2: Harden replication write/ack path with explicit consensus checks (Target: Next Sprint)
  - Scope: [src/replication/replication_manager.cpp](src/replication/replication_manager.cpp), [src/replication/conflict_resolution.cpp](src/replication/conflict_resolution.cpp)
  - Why now: Highest file-level count in confidence review and high distributed_consistency risk.
  - Acceptance:
    - Write paths fail closed when quorum/ack requirements are not met.
    - Conflict resolution records version/causal metadata before commit.
    - Add regression tests for partial-ack and stale-version scenarios.

- [ ] QW-3: Add missing security audit logs for authentication-sensitive voice flows (Target: Next Sprint)
  - Scope: [src/voice/voice_authenticator.cpp](src/voice/voice_authenticator.cpp), [src/voice/voice_assistant.cpp](src/voice/voice_assistant.cpp), [src/voice/voice_assistant_llm.cpp](src/voice/voice_assistant_llm.cpp)
  - Why now: Multiple CRITICAL missing_audit_log findings with very high confidence.
  - Acceptance:
    - Authentication and authorization outcomes emit structured audit events.
    - Audit logs include actor, action, result, and correlation id.
    - Negative tests verify no silent auth-path completion without log emission.

- [ ] QW-4: Enforce safe prompt handling in LLM wrapper and training entry points (Target: Next Sprint)
  - Scope: [src/llm/llama_wrapper.cpp](src/llm/llama_wrapper.cpp), [src/training/incremental_lora_trainer.cpp](src/training/incremental_lora_trainer.cpp)
  - Why now: LLM module dominates high-confidence queue volume.
  - Acceptance:
    - Prompt construction uses sanitized, bounded, and escaped user input.
    - Unsafe control-token patterns are blocked or escaped consistently.
    - Add tests for injection-like payloads in both inference and training APIs.

- [ ] QW-5: Add deterministic routing safeguards in shard router decisions (Target: Next Sprint)
  - Scope: [src/sharding/shard_router.cpp](src/sharding/shard_router.cpp)
  - Why now: Sharding is top-4 module in high-confidence queue, impacts correctness under load.
  - Acceptance:
    - Stable tie-break ordering for equivalent shard scores.
    - Explicit handling for missing metrics and stale state.
    - Add deterministic replay test with fixed seed and repeated runs.

- [ ] QW-6: Add GPU kernel input validation and bounds checks in LoRA kernels (Target: Next Sprint)
  - Scope: [src/llm/lora_framework/kernels/vulkan_kernels.cpp](src/llm/lora_framework/kernels/vulkan_kernels.cpp), [src/llm/lora_framework/kernels/directx_kernels.cpp](src/llm/lora_framework/kernels/directx_kernels.cpp), [src/llm/lora_framework/kernels/hip_fused_kernels.cpp](src/llm/lora_framework/kernels/hip_fused_kernels.cpp), [src/llm/lora_framework/gpu_lora_layers.cpp](src/llm/lora_framework/gpu_lora_layers.cpp)
  - Why now: Concentrated gpu_memory_safety findings in top files.
  - Acceptance:
    - Validate tensor dimensions/strides before kernel dispatch.
    - Guard against out-of-range buffer offsets and zero-sized launches.
    - Add focused tests for malformed tensor metadata.

- [ ] QW-7: Introduce one shared safety helper for LLM prompt assembly (Target: Next Sprint)
  - Scope: [src/llm](src/llm), [src/rag](src/rag), [src/training](src/training)
  - Why now: Reduces duplicated safety logic across the most affected modules.
  - Acceptance:
    - New helper used by at least rag + llm + training entry points.
    - Consistent sanitizer policy and telemetry tags.
    - Remove duplicate ad-hoc sanitization branches replaced by helper.

- [ ] QW-8: Add a high-confidence triage gate in CI preflight report (Target: Next Sprint)
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
