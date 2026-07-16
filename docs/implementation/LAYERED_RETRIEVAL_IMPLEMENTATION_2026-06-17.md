# Layered Retrieval Implementation Report (2026-06-17)

## Scope
This report documents the implementation and stabilization work that completed the
layered retrieval path in ThemisDB:

- ANN Frontdoor
- Tensor Mid-Layer
- Graph Truth validation
- Final LLM/LoRA orchestration

The work includes code integration, modular-build linker fixes, focused tests, and
artifact documentation updates.

## Implemented Architecture Integration

### 1. Final-layer orchestration in runtime pipeline
- Added explicit final-layer hook in Tensor RAG pipeline:
  - `TensorRAGPipeline::setFinalLayerOrchestrator(...)`
- Extended pipeline configuration and decision payload for final-layer resolution:
  - package id, base model/version, metadata
  - `RAGDecision.final_layer_resolution`
- Implemented cross-layer policy/telemetry decision fields:
  - `correlation_id`
  - `routing_reason_code`
  - `confidence_policy_version`
  - `confidence_threshold_key`
  - `fallback_mode`
  - `fallback_reason_code`
  - `escalation_source_layer`
- Propagated telemetry into upper layers:
  - `GraphTruthValidationResult` now exposes `correlation_id`,
    `routing_reason_code`, `fallback_mode`, `fallback_reason_code`,
    `escalation_source_layer`.
  - `FinalLayerResolution` now exposes `correlation_id`,
    `routing_reason_code`, `confidence_policy_version`,
    `confidence_threshold_key`, `fallback_mode`, `fallback_reason_code`,
    `escalation_source_layer`.
- Runtime behavior:
  - If retrieval triggers and an orchestrator is attached, the pipeline resolves
    package/model/adapter selection through `FinalLayerOrchestrator::resolve(...)`.
  - Routing reason and confidence policy metadata are attached on every step.
  - Embedding backend exceptions are marked fail-closed with explicit reason code.

Primary files:
- `include/rag/tensor_rag_pipeline.h`
- `src/rag/tensor_rag_pipeline.cpp`

### 2. Final-layer API and implementation stabilization
- Resolved parser issue in final-layer header by making JSON type visibility explicit.
- Kept implementation signature and header declaration aligned.

Primary files:
- `include/llm/final_layer_orchestrator.h`
- `src/llm/final_layer_orchestrator.cpp`
- `tests/model/test_final_layer_orchestrator.cpp`

## Modular Build and Linker Fixes

### 1. Storage module source-set consistency
Problem:
- `tensor_mid_layer.cpp` in `THEMIS_STORAGE_SOURCES` referenced symbols from
  `adapter_repository.cpp` and `tensor_fingerprint_graph.cpp` that were not part of
  the same module source-set.

Fix:
- Added missing sources to storage module:
  - `../src/tensor/adapter_repository.cpp`
  - `../src/tensor/tensor_fingerprint_graph.cpp`

### 2. Query/content linker dependency for graph-truth path
Problem:
- `GraphTruthValidator` called `OntologyAwareRetriever::retrieve(...)`, but
  `ontology_aware_retriever.cpp` was not included in the modular query source-set,
  causing unresolved externals while linking `themis_content.dll`.

Fix:
- Added:
  - `../src/rag/ontology_aware_retriever.cpp`

### 3. Module-boundary-safe ANN fallback behavior
Adjustment:
- Reduced cross-module linker coupling by removing direct storage-side dependence on
  `VectorIndexManager::searchKnn(...)` in `AnnFrontdoor` fallback path.
- Added robust search-layer fallback in `HybridSearch` to call legacy vector search
  if AnnFrontdoor returns no candidates on fallback strategies.
- Added ANN observability/policy metadata in `AnnFrontdoorResult`:
  - `correlation_id`, `routing_reason_code`, `confidence_policy_version`,
    `confidence_threshold_key`, `fallback_mode`, `fallback_reason_code`.
- Introduced a shared reason-code registry:
  - `include/observability/reason_codes.h`
  - Used by ANN/Tensor/Graph/Final-Layer code paths and focused tests to
    prevent string drift across layers.
- Introduced a shared telemetry-key registry:
  - `include/observability/telemetry_keys.h`
  - Centralizes telemetry field names, layer-name values, default correlation
    identifiers, and metadata keys (for example `reasoning_chain`).
  - Applied in ANN/Tensor/Graph runtime paths to reduce key drift risk.
- Integrated a shared structured layer-decision JSON emitter:
  - `include/observability/layer_decision_log.h`
  - Event schema: `layer_handoff_decision`
  - Standardized keys include `event`, `layer_name`, `correlation_id`,
    `routing_reason_code`, `confidence_policy_version`,
    `confidence_threshold_key`, `fallback_mode`, `fallback_reason_code`,
    `escalation_source_layer`, and `resolved`.
  - Runtime callsites wired in all four layers before return:
    `AnnFrontdoor::search(...)`, `TensorRAGPipeline::step(...)`,
    `GraphTruthValidator::validate(...)`,
    `FinalLayerOrchestrator::resolve(...)`.
- Normalized fallback reason codes for missing backend paths and fixed
  strategy planning edge cases (no backend => FLAT_BRUTE_FORCE; hot scoped
  backend can remain HNSW-planned without requiring a global VIM handle).

Primary files:
- `cmake/ModularBuild.cmake`
- `src/index/ann_frontdoor.cpp`
- `src/search/hybrid_search.cpp`

## Test Integration and Validation

### 1. Correct focused test source alignment
Observation:
- Focused target `test_tensor_phase3_focused` compiles `tests/test_tensor_phase3.cpp`
  (not `tests/tensor/test_tensor_phase3.cpp`).

Action:
- Added new integration test in the compiled test file:
  - `TensorRAGPipeline.TRPL14_final_layer_resolution_attached_when_orchestrator_set`
- Added policy/telemetry regression tests:
  - `TensorRAGPipeline.TRPL15_policy_metadata_present_for_flare_trigger`
  - `TensorRAGPipeline.TRPL16_fail_closed_when_embedding_backend_throws`
- Added end-to-end propagation regression test:
  - `TensorRAGPipeline.TRPL17_correlation_and_reason_codes_propagate_to_graph_and_final_layer`
- Added structured logging schema regression test:
  - `TensorRAGPipeline.TRPL18_structured_layer_handoff_json_log_emitted`
  - Validates that a `layer_handoff_decision` JSON payload is emitted with
    canonical schema keys (`event`, `layer_name`, `correlation_id`, `resolved`).
- Corrected assertion to current final-layer API field:
  - `primary_adapter_id`

Primary file:
- `tests/test_tensor_phase3.cpp`

### 2. Duplicate test-file drift reduction
- Synchronized the same API field fix in:
  - `tests/tensor/test_tensor_phase3.cpp`

## Executed Validation Runs

All commands executed in `build-msvc-windows-release` context unless noted.

### Build verification
- `ninja -j1 themis_content`
  - Result: success (Exit 0)
- `ninja -j1 test_tensor_phase3_focused`
  - Result: success (Exit 0)

### Test verification
- `test_tensor_phase3_focused.exe --gtest_filter=TensorRAGPipeline.*`
  - Result: 16/16 passed
- `test_tensor_phase3_focused.exe --gtest_filter=TensorRAGPipeline.TRPL14_final_layer_resolution_attached_when_orchestrator_set`
  - Result: 1/1 passed
- `test_tensor_phase3_focused.exe --gtest_filter=TensorRAGPipeline.TRPL15_*:TensorRAGPipeline.TRPL16_*`
  - Result: 2/2 passed
- `test_tensor_phase3_focused.exe --gtest_filter=TensorRAGPipeline.TRPL17_*`
  - Result: 1/1 passed
- `module_index_test_ann_frontdoor_focused.exe`
  - Result: 29/29 passed

## Updated Documentation Artifacts

- Gap state reflects closure of section 3.4 and updated cross-cutting focus:
  - `GAP_ANALYSIS.md`
- This implementation report:
  - `docs/implementation/LAYERED_RETRIEVAL_IMPLEMENTATION_2026-06-17.md`

## Follow-up Recommendations

1. Decide canonical ownership for phase-3 tensor test source to avoid future drift
   between `tests/test_tensor_phase3.cpp` and `tests/tensor/test_tensor_phase3.cpp`.
2. Add CI coverage for TRPL14 in focused test workflows to protect final-layer
   pipeline integration.
3. Extend observability around final-layer routing decisions (package/model/adapter)
   for production diagnostics.
