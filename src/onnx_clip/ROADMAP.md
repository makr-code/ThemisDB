> **Status:** current | validated: 2026-10-08

<!-- Status: current | validated: 2026-10-08 -->
<!-- Links: README.md · ARCHITECTURE.md · PRODUCTION_REQUIREMENTS.md · SECURITY.md · FUTURE_ENHANCEMENTS.md -->

# ONNX CLIP Plugin Roadmap

## Current Status

- [x] `ONNXClipPlugin` is present in the current source tree and matches the active `IImageAnalysisBackend` contract in `src/onnx_clip/onnx_clip_plugin.h`.
- [x] The current implementation is a deterministic portable reference backend with explicit config validation, bounded batch processing, hot-swap support, and optional mmap/hash verification hooks.
- [x] The canonical module docs are aligned with the live implementation and no stale governance doc remains missing.

## In Progress

- [~] Keep the module docs synchronized with the live source as the provider stack and release hardening evolve (Target: Q4 2026)
- [~] Validate additional benchmark and golden-model coverage when model assets are available in CI (Target: Q4 2026)

## Planned Features

- [ ] Extend real-model golden embedding validation with deterministic fixtures when asset availability is stable (Target: Q4 2026)
- [ ] Add or update broader performance coverage for the bounded batching path and hot-swap drain behavior (Target: Q1 2027)
- [ ] Review and tighten any provider-specific acceleration paths only when they are concretely supported by build/test evidence (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Keep `IImageAnalysisBackend` integration stable and source-aligned with the public interface
- [x] Document configuration validation and lifecycle semantics for `initialize()` and `reloadModel()`
- [x] Explicitly scope the portable implementation to deterministic behavior rather than implicit provider assumptions

### Phase 2: Core Implementation
- [x] Complete deterministic embedding generation and text embedding paths
- [x] Add bounded sub-batching via `max_batch_size`
- [x] Maintain request-order preservation and structured error results

### Phase 3: Error Handling & Edge Cases
- [x] Reject empty payloads before processing
- [x] Fail closed on hash mismatch when `model.path` and `model.expected_sha256` are configured
- [x] Preserve the previous implementation snapshot during hot-swap drain failures
- [x] Treat mmap enablement as a best-effort optional optimization rather than a required runtime dependency

### Phase 4: Tests
- [x] Validate focused plugin tests and contract-hardening coverage under the module test harness
- [x] Keep integration and soak coverage aligned with the actual supported runtime surface
- [ ] Add additional test evidence for real-model fixtures when available (Target: Q4 2026)

### Phase 5: Performance / Hardening
- [x] Keep memory churn bounded by capped sub-batching and request serialization
- [x] Track latency and counts through `getStatistics()`
- [ ] Collect benchmark evidence against a representative baseline before broader provider expansion (Target: Q1 2027)

### Phase 6: Documentation & Acceptance
- [x] Maintain README, ARCHITECTURE, PRODUCTION_REQUIREMENTS, SECURITY, and FUTURE_ENHANCEMENTS in sync with the live implementation
- [x] Confirm module docs satisfy the direct Doxygen/governance validation contract for the active source tree
- [ ] Refresh release notes or changelog entries if a provider-specific contract change is introduced (Target: Q1 2027)

## Production Readiness Checklist

| Area | Status | Notes |
|------|--------|-------|
| API contract | [x] | Source-aligned with the current `IImageAnalysisBackend` declaration |
| Validation | [x] | Config validation and structured fail-closed behavior are in place |
| Batch handling | [x] | `generateEmbeddingBatch()` processes bounded sub-batches |
| Threading | [x] | Request serialization and in-flight drain guard are implemented |
| Integrity checks | [x] | Model hash verification is enforced when configured |
| Hot swap | [x] | `reloadModel()` validates and swaps snapshots while draining requests |
| mmap support | [x] | Best-effort and non-blocking when unsupported |
| Benchmark coverage | [ ] | Pending fixture-backed evidence when real model assets are available |
| Documentation | [x] | Canonical governance docs are present and aligned |

## Known Issues & Limitations

- [ ] Real-model golden embedding fixtures remain unavailable in the default test environment; broader provider-specific validation depends on asset availability.
- [ ] Provider acceleration paths are intentionally not treated as mandatory for the portable build contract.
- [ ] Additional benchmark evidence should be captured before claiming broader production throughput targets.

## Breaking Changes

- [ ] No breaking API changes are planned for the current public interface; any future provider-specific expansion will require doc and migration updates in the same change.
