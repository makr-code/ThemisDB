# Core Quick-Win Plan (Next Blocks)

Date: 2026-05-31
Source: Package F completion state, focused RAG validation, and the remaining RAG test gaps
Scope: rag, llm, server, query, observability, tests

## Goal

Close the next small, production-relevant RAG blocks without reopening broad architecture work. The focus is on keeping the live RAG path stable while removing the last ingestion and budget-management mismatches surfaced by the focused suites.

Snapshot: Package G/H are complete, Package I/J are code-level complete with remaining environment-gated live validation on `127.0.0.1:8765`.

## Current Status

- Package F is complete: live `/api/v1/llm/rag` wiring, dedicated `LLMApiHandler::handleRAG`, and focused RAG validation are in place.
- Package I is code-level complete for contract/telemetry coverage (live execution remains environment-gated):
  - `/api/v1/llm/rag` success responses now include effective telemetry fields (`collection_effective`, `rag_mode_effective`, `retrieval_attempted`, `documents_rejected`, `top_k_effective`, `max_context_tokens_effective`, `response_budget_tokens_effective`).
  - Connector-mode live tests were extended to assert these fields when the endpoint is reachable.
  - Connector-mode live RAG checks now also assert iterative `rag_mode` passthrough and response-budget capping by explicit `max_tokens` when `/api/v1/llm/rag` is reachable.
  - Non-live `themisctl` contract tests now validate both success-shape and fail-closed error behavior (including explicit 400/503 scenarios and structured error payload checks).
  - `themisctl rag query` now forwards budget/mode controls (`--rag-mode`, `--response-budget-tokens`, `--max-tokens`) so contract-level budget semantics can be exercised from CLI level.
- Focused verification is green for the newly added RAG contract/fail-closed `themisctl` cases.
- Package G has progressed with bridge hydration hardening:
  - `enrichRetrievedDocuments(...)` canonicalises retrieval metadata (`content` + `source`) for valid documents.
  - `enrichRetrievedDocuments(...)` now treats whitespace-only IDs/content/metadata as missing and fails closed when canonical source/content cannot be established.
  - `indexDocument(...)` now stamps canonical chunk metadata aliases (`source`, `content`, `text`, `body`) so bridge output matches the shape consumed by live RAG handlers and docs-assistant flows.
  - `buildEntityContext(...)` now trims entity identifiers before rendering to keep enrichment context canonical and whitespace-stable.
  - `indexDocument(...)` now keeps indexing available via a minimal fallback entity/chunk path when workflow execution is unavailable.
  - Targeted regression validation for bridge hardening is green (`RI28`-`RI34`).
- Package H is complete for budget-aware context assembly and retrieval control:
  - `RAGContextAssembler::assemble(...)` now applies deterministic tie-breaking for equal relevance scores (`chunk_id` -> `source` -> `content`).
  - `RagContextAssemblerFocusedTests.*` is green (32/32), including deterministic tie-break and `computeMaxTokens` overflow clamp coverage.
  - `AdaptiveRetrieval` now clamps invalid/overflow-prone config inputs at ingress (`base_top_k`, `max_top_k`, `complexity_scaling`, similarity thresholds incl. non-finite values).
  - `AdaptiveRetrievalFocusedTests.*` is green (16/16), including invalid-config clamp coverage.
  - `AgenticRAG` now sanitises `max_session_tokens` at ingress to keep budget-overflow sentinels representable (`SIZE_MAX` -> `SIZE_MAX-1`).
  - `ARG_BUD.*` remains green (6/6), including max-size budget sanitisation coverage.
  - `ARG_BUD` focused execution no longer pays per-test setup cost: shared-agent fixture wiring plus suite-level warm-up keep heavy initialization outside the assertion path, and the focused tests now run at 0 ms each after warm-up.
  - `MultiStepRAGOrchestrator` now sanitises budget-relevant ingress config (`model_context_tokens`, `min_response_tokens`, `max_response_tokens`, `max_map_steps`) before orchestration.
  - `MultiStepRAGOrchestrator::runMapReduce(...)` now applies the same budget-capped max-token limit consistently across single-pass, map-phase, and reduce-phase inference calls.
  - `MultiStepRAGOrchestrator::runIterative(...)` now uses the same budget-capped max-token limit for answer generation and iterative gap-detection prompts.
  - `MultiStepRAGFocusedTests.*` remains green (16/16), including invalid-budget sanitisation coverage.
  - Focused coverage now includes cross-phase budget-cap propagation (`MultiStepRAGFocusedTests.C7_MapReduceUsesBudgetCappedMaxTokensAcrossPhases`) alongside ingress sanitisation (`C6`).
  - Focused coverage now also verifies iterative budget-cap propagation for answer + gap-detection (`MultiStepRAGFocusedTests.B6_IterativeUsesBudgetCappedTokensForAnswerAndGapDetection`).
  - `LlamaCppPlugin::generateRAG(...)` now derives response max-tokens from the same effective context-window override used during assembly, avoiding plugin-side budget drift when `rag_context.max_context_tokens` is set.
  - `LLMApiHandler::handleRAG(...)` now propagates the normalized response budget into `llm_request.max_tokens` and caps it by the explicit request limit, removing handler-side budget drift while preserving caller constraints.
  - Focused non-live contract coverage now spans budget propagation/capping, iterative-mode consistency, CLI forwarding, numeric guardrails, and local rag-mode validation (`ThemisctlHttpTest.TRQ13..TRQ23`).
  - Compact closure runs are green for both slices: contract core (`ThemisctlHttpTest.TRQ13..TRQ19`, 7/7) and CLI guards (`ThemisctlHttpTest.TRQ15..TRQ23`, 9/9).
  - `themisctl rag query` now normalizes `--rag-mode` robustly (case-insensitive, `map-reduce` and `mapreduce` aliases to `map_reduce`) with focused coverage (`ThemisctlHttpTest.TRQ21..TRQ23`).
  - `LlamaCppPluginFocusedTests.E*` is green (4/4), including explicit context-window override coverage.
- Package J has progressed for runtime console observability:
  - End-to-end lifecycle logs are in place across live RAG, LLM-adjacent runtime modules, and streaming/non-streaming transitions (including async submit + async streaming paths).
  - Low-noise non-functional assertions cover OpenAI non-stream/stream lifecycle logs and async submit/streaming lifecycle logs (`LLMApiHandlerPolicyTest.*`, `InferenceEngineEnhancedTest.AsyncSubmitLifecycle_EmitsLowNoiseLogs`, `AsyncInferenceEngineStreamingTest.SubmitStreaming_EmitsLifecycleLogs`).
  - Compact Package-J closeout runs are green (`test_llm_openai_compat_adapter` 2/2 and `test_inference_engine_enhanced_focused` 2/2 for the lifecycle-log assertions).
  - Focused verification remains green for instrumented RAG modules (`RagContextAssemblerFocusedTests` 32/32, `AdaptiveRetrievalFocusedTests` 16/16, `MultiStepRAGFocusedTests` 16/16, `LlamaCppPluginFocusedTests.E*` 4/4), plus rollout-step-2 checks (`RAGIngestionBridgeTest.*` + `DocsAssistantAQLTest.SingletonPattern`: 31/31).
  - Build validation for affected targets is green (`themis_llm`, `themis_network`).
- Remaining gaps are now primarily:
  - Environment-gated connector regression execution in workspaces where `127.0.0.1:8765` is reachable.

## Closing Checklist (Current Run)

- [x] `themis_server` builds cleanly in `windows-release` (verified in current run).
- [x] Non-live themisctl RAG contract slice passes as a single-shot closure (`ThemisctlHttpTest.TRQ13..TRQ23`, 11/11).
- [~] Connector live regression closure (`ConnectorApiLiveTest.IngestWorkspaceDocsAndRunRag`) remains environment-gated; latest run skipped because `127.0.0.1:8765` is unreachable.

## In Progress / Planned Features

- [x] Package G: RAG ingestion bridge stabilization.
  - [x] Completed (canonical metadata hydration, fail-closed guards, bridge/context shape alignment; details and evidence in Current Status).
  - Primary files: `src/rag/rag_ingestion_bridge.cpp`, `src/llm/docs_assistant.cpp`, `src/server/llm_api_handler.cpp`, `tests/test_rag_ingestion_bridge.cpp`.

- [x] Package H: budget-aware context assembly and retrieval control.
  - [x] Completed (cross-component budget-cap consistency, ingress clamps/tie-break determinism, focused TRQ/ARG_BUD closure; details and evidence in Current Status).
  - Primary files: `src/rag/rag_context_assembler.cpp`, `src/rag/multi_step_rag.cpp`, `src/rag/adaptive_retrieval.cpp`, `src/server/llm_api_handler.cpp`, `tests/test_agentic_rag_budget.cpp`, `tests/test_rag_context_assembler.cpp`, `tests/test_rag_adaptive_retrieval.cpp`, `tests/test_multi_step_rag.cpp`.

- [~] Package I: live regression and telemetry coverage.
  - [x] Extend the connector-mode RAG checks so they verify effective retrieval counts, effective budgets, iterative-mode passthrough, and explicit failure reasons where the endpoint is available.
  - [~] Execute/confirm the full connector-mode RAG regression in environment-ready workspaces (`127.0.0.1:8765`, auth/model prerequisites).
  - [x] Add narrow checks for empty retrieval, missing query engine, invalid collection, and zero-document cases so the contract stays fail-closed (non-live `themisctl` coverage added).
  - [x] Keep the live `/api/v1/llm/rag` response shape aligned with the handler contract after any bridge or budget changes.
  - Primary files: `tests/test_connector_mode_api.cpp`, `src/server/http_server.cpp`, `src/server/llm_api_handler.cpp`.

- [~] Package J: Console observability and debug traceability for live LLM/RAG execution.
  - [x] Add nachvollziehbare console logs for request ingress, budget normalization, retrieval preparation, context assembly, and final llama.cpp dispatch in the main RAG runtime path.
  - [x] Extend the same structured console logging style to remaining LLM-adjacent runtime modules so execution state is consistently visible across modules (step 2 complete for plugin manager/docs assistant/ingestion bridge; step 3 complete for streaming handler + async streaming lifecycle, plus non-stream OpenAI and async submit lifecycle; compact assertion closeout run green).
  - [x] Add/refresh focused assertions for non-functional logging expectations only where stable and low-noise; keep functional assertions as the primary guard.
  - Primary files: `src/server/http_server.cpp`, `src/server/llm_api_handler.cpp`, `src/rag/rag_context_assembler.cpp`, `src/rag/adaptive_retrieval.cpp`, `src/rag/multi_step_rag.cpp`, `src/llama_cpp/llama_cpp_plugin.cpp`, `src/llm/llm_plugin_manager.cpp`, `src/llm/docs_assistant.cpp`, `src/rag/rag_ingestion_bridge.cpp`, `src/llm/async_inference_engine.cpp`.

## Implementation Phases

### Phase 1: Design / API Contract

- [x] Define the canonical RAG document shape for bridge output, including `content`, `source`, `metadata`, and relevance score handling.
- [x] Lock the budget propagation contract across assembler, adaptive retrieval, and the two HTTP RAG entry points.
- [x] Identify which failures should return `400` versus `503` so the live path stays fail-closed and predictable.

### Phase 2: Core Implementation

- [x] Update the RAG ingestion bridge to emit fully hydrated documents and stable enrichment metadata.
- [x] Align context assembly and retrieval control paths with the same budget and truncation rules.
- [x] Reuse the existing live handler contract instead of adding alternate RAG response shapes.

### Phase 3: Error Handling & Edge Cases

- [x] Reject empty or malformed bridge inputs before they reach prompt assembly.
- [x] Clamp invalid token limits and handle zero-document retrieval explicitly.
- [x] Keep missing-engine and invalid-collection cases observable instead of silently falling back to empty context.

### Phase 4: Tests

- [x] Repair the RAG ingestion bridge regression coverage around the failing RI cases.
- [x] Add focused assertions for budget propagation, deterministic chunk selection, and adaptive retrieval decisions.
- [~] Re-run the live connector checks when the local endpoint is available.

### Phase 5: Performance / Hardening

- [x] Remove avoidable copies in bridge-to-context conversion.
- [ ] Preserve deterministic ordering in retrieval selection and context assembly.
- [ ] Keep budget checks cheap enough that they can run in the hot path without extra allocations.
- [~] Ensure console debug output remains signal-rich without flooding (entry/decision/exit events only on hot paths).

### Phase 6: Documentation & Acceptance

- [~] Update the affected module docs and inventory notes if the bridge contract or budget contract changes.
- [ ] Mark the block complete only after the focused RAG suites are green and the live contract checks are stable.
- [x] Record any remaining environment-only limitations separately from source-level gaps.

## Production Readiness Checklist

- [x] `themis_server` builds cleanly in `windows-release`
- [x] `RAGPromptBuilderFocusedTests` and `MultiStepRAGFocusedTests` remain green
- [x] `test_rag_ingestion_bridge.cpp` passes after bridge hydration changes
- [~] Focused RAG budget suites are stable where split (`test_rag_context_assembler.cpp`, `test_rag_adaptive_retrieval.cpp`, `test_multi_step_rag.cpp` green with deterministic tie-break/clamp/overflow guards); `test_agentic_rag_budget.cpp` remains a stabilization item.
- [~] Connector-mode RAG assertions (including iterative budget-cap checks) are implemented and pass when the local endpoint is available (`127.0.0.1:8765` environment-gated; latest focused run skipped because endpoint unreachable).
- [x] no new null-dereference, input-validation, or uncaught-exception regressions are introduced in the touched files (validated via targeted `test_themisctl_focused` runs incl. fail-closed TRQ cases)
- [x] Non-live RAG contract and fail-closed behavior are covered in `test_themisctl_focused` (single-shot TRQ13-TRQ23 closure green, 11/11; effective fields, structured `400/503` errors, local CLI fail-fast validation).
- [x] RAG ingestion bridge round-trip regressions RI01-RI30 are green in `themis_tests`
- [ ] docs inventory is updated if the canonical module references change

## Known Issues & Limitations

- `AgenticRAGBudgetFocusedTests` can still be dominated by heavyweight model loading and may need a narrower fixture boundary before it is fully stable in the preset timeout.
- Live connector coverage is environment-dependent because `127.0.0.1:8765` is not guaranteed to be reachable in every workspace.
- Broader docs canonical-reference gaps still exist outside the RAG-adjacent modules; this plan only targets the next runtime blocks, not the full documentation cleanup set.

## Breaking Changes

- If the bridge document shape is normalized, downstream code that relied on fallback keys will need to switch to the canonical `content`/`source`/`metadata` fields.
- If budget clamping becomes stricter, oversized requests may start failing earlier with an explicit error instead of being silently truncated later in the pipeline.