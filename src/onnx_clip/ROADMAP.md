---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

> **Status:** current | source-checked: 2026-10-31

<!-- Status: current | source-checked: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · PRODUCTION_REQUIREMENTS.md · SECURITY.md · TRUST.md · FUTURE_ENHANCEMENTS.md -->

# ONNX CLIP Plugin Roadmap

## Current Status

- [x] `ONNXClipPlugin` is present in the current source tree and matches the active `IImageAnalysisBackend` contract in `src/onnx_clip/onnx_clip_plugin.h`.
- [x] The current implementation generates deterministic reference vectors; it does not load CLIP weights or execute ONNX Runtime.
- [x] Batch requests are processed sequentially in configured chunks; chunk size is clamped to at least 1, but total request size and configured upper bounds are not enforced here.
- [x] Optional digest checking and read-only mmap hooks exist; mmap bytes are not used by an inference session, and no-verifier non-OpenSSL builds skip digest checking.
- [x] The module's developer documentation set describes the current source behavior and identifies the model-backed gaps below.
- [x] Historical Phase 3B/4B reports and generated gap artifacts are retained only under `src/onnx_clip/archive/` and are excluded from the active production governance set.

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
- [~] Ensure non-OpenSSL builds fail closed when no `ModelHashFn` is registered (current source skips the check)
- [x] Publish a validated new implementation before draining old-snapshot requests; a drain timeout returns false without rolling back that publication
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
| Configuration handling | [~] | Defaults and dimension/batch minimums are applied; general schema validation and upper bounds are not implemented |
| Batch handling | [x] | `generateEmbeddingBatch()` processes bounded sub-batches |
| Threading | [x] | Request serialization and in-flight drain guard are implemented |
| Integrity checks | [~] | OpenSSL or injected verifier checks configured digest; non-OpenSSL builds without callback skip it |
| Hot swap | [x] | New snapshot is published before old requests drain; a drain timeout returns false while new state remains active |
| mmap support | [~] | Read-only mapping is best-effort but mapped bytes are not used for inference |
| Benchmark coverage | [ ] | Pending fixture-backed evidence when real model assets are available |
| Documentation | [x] | Canonical governance docs are present and aligned |

## Known Issues & Limitations

- [ ] Real-model golden embedding fixtures remain unavailable in the default test environment; broader provider-specific validation depends on asset availability.
- [ ] Provider acceleration paths are intentionally not treated as mandatory for the portable build contract.
- [ ] Additional benchmark evidence should be captured before claiming broader production throughput targets.
- [ ] Integrate and validate actual ONNX inference, image preprocessing, and CLIP tokenization before claiming semantic embeddings (Target: after model/runtime design approval)
- [ ] Add model-backed cross-modal retrieval and provider benchmarks before production semantic use (Target: after inference integration)

## Breaking Changes

- [ ] No breaking API changes are planned for the current public interface; any future provider-specific expansion will require doc and migration updates in the same change.
