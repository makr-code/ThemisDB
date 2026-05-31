# Core Quick-Win Plan (Next Blocks)

Date: 2026-05-31
Source: Package F completion state, focused RAG validation, and the remaining RAG test gaps
Scope: rag, llm, server, query, observability, tests

## Goal

Close the next small, production-relevant RAG blocks without reopening broad architecture work. The focus is on keeping the live RAG path stable while removing the last ingestion and budget-management mismatches surfaced by the focused suites.

## Current Status

- Package F is complete: live `/api/v1/llm/rag` wiring, dedicated `LLMApiHandler::handleRAG`, and focused RAG validation are in place.
- Package I has progressed with contract-level coverage:
  - `/api/v1/llm/rag` success responses now include effective telemetry fields (`collection_effective`, `rag_mode_effective`, `retrieval_attempted`, `documents_rejected`, `top_k_effective`, `max_context_tokens_effective`, `response_budget_tokens_effective`).
  - Connector-mode live tests were extended to assert these fields when the endpoint is reachable.
  - Non-live `themisctl` contract tests now validate both success-shape and fail-closed error behavior (including explicit 400/503 scenarios and structured error payload checks).
- Focused verification is green for the newly added RAG contract/fail-closed `themisctl` cases.
- Package G has progressed with bridge hydration hardening:
  - `enrichRetrievedDocuments(...)` canonicalises retrieval metadata (`content` + `source`) for valid documents.
  - `indexDocument(...)` now keeps indexing available via a minimal fallback entity/chunk path when workflow execution is unavailable.
  - `RAGIngestionBridgeTest.*` is green (30/30) after the change set.
- Package H has progressed with deterministic context assembly:
  - `RAGContextAssembler::assemble(...)` now applies deterministic tie-breaking for equal relevance scores (`chunk_id` -> `source` -> `content`).
  - `RagContextAssemblerFocusedTests.*` is green (32/32), including deterministic tie-break and `computeMaxTokens` overflow clamp coverage.
  - `AdaptiveRetrieval` now clamps invalid/overflow-prone config inputs at ingress (`base_top_k`, `max_top_k`, `complexity_scaling`, similarity thresholds incl. non-finite values).
  - `AdaptiveRetrievalFocusedTests.*` is green (16/16), including invalid-config clamp coverage.
  - `AgenticRAG` now sanitises `max_session_tokens` at ingress to keep budget-overflow sentinels representable (`SIZE_MAX` -> `SIZE_MAX-1`).
  - `ARG_BUD.*` remains green (6/6), including max-size budget sanitisation coverage.
  - `ARG_BUD` focused execution no longer pays per-test setup cost: shared-agent fixture wiring plus suite-level warm-up keep heavy initialization outside the assertion path, and the focused tests now run at 0 ms each after warm-up.
  - `MultiStepRAGOrchestrator` now sanitises budget-relevant ingress config (`model_context_tokens`, `min_response_tokens`, `max_response_tokens`, `max_map_steps`) before orchestration.
  - `MultiStepRAGFocusedTests.*` remains green (16/16), including invalid-budget sanitisation coverage.
  - `LlamaCppPlugin::generateRAG(...)` now derives response max-tokens from the same effective context-window override used during assembly, avoiding plugin-side budget drift when `rag_context.max_context_tokens` is set.
  - `LlamaCppPluginFocusedTests.E*` is green (4/4), including explicit context-window override coverage.
- Remaining gaps are now primarily:
  - Package G deeper normalization follow-ups across downstream consumers.
  - Package H cross-component budget consistency hardening.
  - Live connector regression execution in environments where `127.0.0.1:8765` is reachable.

## In Progress / Planned Features

- [~] Package G: RAG ingestion bridge stabilization.
  - [ ] Normalize `RAGIngestionBridge::indexDocument(...)`, `enrichRetrievedDocuments(...)`, and `buildEntityContext(...)` so retrieved items survive the full index -> context -> response round trip.
  - [ ] Fail closed on missing source/content metadata instead of producing partially hydrated documents.
  - [ ] Align bridge output with the document shape already used by `DocsAssistant` and the live RAG handlers.
  - Primary files: `src/rag/rag_ingestion_bridge.cpp`, `src/llm/docs_assistant.cpp`, `src/server/llm_api_handler.cpp`, `tests/test_rag_ingestion_bridge.cpp`.

- [~] Package H: budget-aware context assembly and retrieval control.
  - [~] Align `RAGContextAssembler`, `MultiStepRAG`, `AdaptiveRetrieval`, and the `AgenticRAGBudget` path to the same token-budget math and truncation rules (plugin-side response-budget drift for explicit context overrides is now closed in `LlamaCppPlugin::generateRAG(...)`).
  - [~] Clamp invalid or overflow-prone budgets at ingress and preserve deterministic selection order when chunk scores tie (context tie-break + adaptive clamp + agentic session-budget overflow guard + multistep budget sanitisation + central `computeMaxTokens` int-overflow guard done; remaining cross-component budget ingress hardening pending).
  - [x] Reduce timeout sensitivity in the focused budget suite by separating heavy fixture work from the assertion path where needed (ARG_BUD now uses shared fixture wiring plus suite-level warm-up; focused assertion cases run without embedded warm-up cost).
  - Primary files: `src/rag/rag_context_assembler.cpp`, `src/rag/multi_step_rag.cpp`, `src/rag/adaptive_retrieval.cpp`, `tests/test_agentic_rag_budget.cpp`, `tests/test_rag_context_assembler.cpp`, `tests/test_rag_adaptive_retrieval.cpp`, `tests/test_multi_step_rag.cpp`.

- [~] Package I: live regression and telemetry coverage.
  - [~] Extend the connector-mode RAG checks so they verify effective retrieval counts, effective budgets, and explicit failure reasons where the endpoint is available.
  - [x] Add narrow checks for empty retrieval, missing query engine, invalid collection, and zero-document cases so the contract stays fail-closed (non-live `themisctl` coverage added).
  - [x] Keep the live `/api/v1/llm/rag` response shape aligned with the handler contract after any bridge or budget changes.
  - Primary files: `tests/test_connector_mode_api.cpp`, `src/server/http_server.cpp`, `src/server/llm_api_handler.cpp`.

## Implementation Phases

### Phase 1: Design / API Contract

- [~] Define the canonical RAG document shape for bridge output, including `content`, `source`, `metadata`, and relevance score handling.
- [~] Lock the budget propagation contract across assembler, adaptive retrieval, and the two HTTP RAG entry points.
- [x] Identify which failures should return `400` versus `503` so the live path stays fail-closed and predictable.

### Phase 2: Core Implementation

- [ ] Update the RAG ingestion bridge to emit fully hydrated documents and stable enrichment metadata.
- [~] Align context assembly and retrieval control paths with the same budget and truncation rules.
- [x] Reuse the existing live handler contract instead of adding alternate RAG response shapes.

### Phase 3: Error Handling & Edge Cases

- [ ] Reject empty or malformed bridge inputs before they reach prompt assembly.
- [x] Clamp invalid token limits and handle zero-document retrieval explicitly.
- [~] Keep missing-engine and invalid-collection cases observable instead of silently falling back to empty context.

### Phase 4: Tests

- [ ] Repair the RAG ingestion bridge regression coverage around the failing RI cases.
- [~] Add focused assertions for budget propagation, deterministic chunk selection, and adaptive retrieval decisions.
- [~] Re-run the live connector checks when the local endpoint is available.

### Phase 5: Performance / Hardening

- [ ] Remove avoidable copies in bridge-to-context conversion.
- [ ] Preserve deterministic ordering in retrieval selection and context assembly.
- [ ] Keep budget checks cheap enough that they can run in the hot path without extra allocations.

### Phase 6: Documentation & Acceptance

- [~] Update the affected module docs and inventory notes if the bridge contract or budget contract changes.
- [ ] Mark the block complete only after the focused RAG suites are green and the live contract checks are stable.
- [x] Record any remaining environment-only limitations separately from source-level gaps.

## Production Readiness Checklist

- [ ] `themis_server` builds cleanly in `windows-release`
- [x] `RAGPromptBuilderFocusedTests` and `MultiStepRAGFocusedTests` remain green
- [x] `test_rag_ingestion_bridge.cpp` passes after bridge hydration changes
- [~] `test_agentic_rag_budget.cpp`, `test_rag_context_assembler.cpp`, `test_rag_adaptive_retrieval.cpp`, and `test_multi_step_rag.cpp` pass or are split into stable focused slices (focused slices for context/adaptive/multi-step are green, incl. deterministic tie-break plus adaptive/multistep invalid-config clamps and central `computeMaxTokens` overflow guard; budget suite remains a stabilization item)
- [~] connector-mode RAG assertions pass when the local endpoint is available (assertions are implemented; execution remains environment-dependent on `127.0.0.1:8765`)
- [x] no new null-dereference, input-validation, or uncaught-exception regressions are introduced in the touched files (validated via targeted `test_themisctl_focused` runs incl. fail-closed TRQ cases)
- [x] non-live RAG contract and fail-closed behavior are covered in `test_themisctl_focused` (effective fields + structured `400/503` error payload checks)
- [x] RAG ingestion bridge round-trip regressions RI01-RI30 are green in `themis_tests`
- [ ] docs inventory is updated if the canonical module references change

## Known Issues & Limitations

- `AgenticRAGBudgetFocusedTests` can still be dominated by heavyweight model loading and may need a narrower fixture boundary before it is fully stable in the preset timeout.
- Live connector coverage is environment-dependent because `127.0.0.1:8765` is not guaranteed to be reachable in every workspace.
- Broader docs canonical-reference gaps still exist outside the RAG-adjacent modules; this plan only targets the next runtime blocks, not the full documentation cleanup set.

## Breaking Changes

- If the bridge document shape is normalized, downstream code that relied on fallback keys will need to switch to the canonical `content`/`source`/`metadata` fields.
- If budget clamping becomes stricter, oversized requests may start failing earlier with an explicit error instead of being silently truncated later in the pipeline.