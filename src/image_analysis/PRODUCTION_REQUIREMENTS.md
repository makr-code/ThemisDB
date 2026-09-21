# Image Analysis Module - Production Requirements

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Purpose and Scope

This document defines the canonical mandatory production requirements for the `image_analysis` module.
It covers minimum operational behavior, resource safety, and backend lifecycle requirements for deployments that use the OCR, object detection, and feature extraction capabilities.

## Document Boundary (Canonical Split)

- **`src/image_analysis/PRODUCTION_REQUIREMENTS.md` (this document):** mandatory production requirements (MUST/MUST NOT), resource limits, backend lifecycle constraints.
- **`src/image_analysis/README.md`:** module overview, scope, quick-start.
- **`src/image_analysis/ARCHITECTURE.md`:** design rationale, data flow, component relationships.
- **`src/image_analysis/ROADMAP.md`:** delivery phases, open/closed features, readiness checklist.
- **`src/image_analysis/FUTURE_ENHANCEMENTS.md`:** planned extensions.

## Mandatory Backend Requirements

- **MUST:** All backend calls must be bounded by a configurable per-operation timeout. Operations that exceed their deadline must return `E6203` without hanging the caller.
- **MUST:** Backend initialization failures must return `E6202` and degrade gracefully; they must not prevent the indexing pipeline from starting.
- **MUST NOT:** Backend unavailability must not propagate as a hard failure to callers. Partial results must be returned with explicit error flags.
- **MUST:** Image format validation must be performed before backend dispatch. Unsupported or corrupted images must return `E6200` or `E6201` without invoking native backend code.

## Mandatory Resource Requirements

- **MUST:** Memory usage during single-image processing must remain below 50 MB (see PERFORMANCE_EXPECTATIONS.md IAR-1).
- **MUST:** Model cache overhead must remain at or below 200 MB for the Tesseract + YOLOv8 models combined (IAR-2).
- **MUST:** Cache entry overhead must remain at or below 100 KB per cached result (IAR-3).
- **MUST NOT:** The module must not hold unbounded memory across requests. Cache eviction (LRU) must be active with a configured capacity limit.

## Mandatory Caching Requirements

- **MUST:** The `ImageCache` must be keyed by image content hash (SHA-256) to prevent stale results from mismatched file paths.
- **MUST:** TTL-based expiration must be active and configurable. Expired entries must not be returned to callers.
- **MUST NOT:** Cache failures must not propagate as hard errors; on cache miss or failure, the backend must reprocess the image.

## Mandatory Error Code Usage

| Code | Condition | Required Behavior |
|---|---|---|
| E6200 | Unsupported image format | Return error; do not invoke backend |
| E6201 | Image corrupted or invalid | Return error; do not invoke backend |
| E6202 | Backend initialization failed | Return error; degrade gracefully |
| E6203 | Processing timeout exceeded | Return partial result with timeout flag |
| E6204 | Insufficient memory for processing | Return error; free all allocated resources |

## Minimal Production Checklist (Audit-Capable)

- [ ] Timeout configured per backend (OCR and detection) with non-zero value
- [ ] Cache capacity limit configured (not unlimited)
- [ ] Cache TTL configured (not unlimited)
- [ ] Error taxonomy E6200–E6299 enforced; no silent failures
- [ ] Backend graceful degradation active (partial result on backend failure)
- [ ] Memory limit per image enforced (< 50 MB)
- [ ] Image hash-based cache keying active
- [ ] Build flags `HAVE_TESSERACT` / `HAVE_OPENCV` validated against actual deployment libraries

## Sourcecode Verification (Module: image_analysis/production)

- Verified files:
  - src/image_analysis/tesseract_ocr_plugin.cpp
  - src/image_analysis/yolov8_onnx_plugin.cpp
  - include/image_analysis/image_processor.h
  - include/image_analysis/image_cache.h
- Verified controls:
  - Timeout enforcement documented in source headers and ARCHITECTURE.md.
  - Error taxonomy E6200–E6299 defined in ROADMAP.md §Phase 1 and ARCHITECTURE.md §Error Handling.
  - Graceful degradation paths documented in ARCHITECTURE.md §Graceful Degradation.
  - `HAVE_TESSERACT` / `HAVE_OPENCV` compile guards prevent unsafe native calls in constrained environments.
