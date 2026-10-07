# Image Analysis Module - Production Requirements

<!-- Status: current | validated: 2026-10-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Purpose and Scope

This document defines the mandatory production requirements for the `image_analysis` module as it exists in the current source tree. The live implementation is plugin-based and uses `include/plugins/` as the canonical contract; the module-specific production requirements therefore focus on lifecycle, dependency gates, and fail-safe behaviour rather than a non-existent legacy `include/image_analysis/` API.

## Document Boundary (Canonical Split)

- **`src/image_analysis/PRODUCTION_REQUIREMENTS.md` (this document):** mandatory operational requirements, dependency gating, and fail-safe behaviour.
- **`src/image_analysis/README.md`:** module overview and current state.
- **`src/image_analysis/ARCHITECTURE.md`:** design rationale and component relationships.
- **`src/image_analysis/ROADMAP.md`:** delivery phases and remaining acceptance work.
- **`src/image_analysis/FUTURE_ENHANCEMENTS.md`:** planned evolution and risk controls.

## Mandatory Backend Requirements

- **MUST:** Each plugin must expose a clear lifecycle (`initialize`, `shutdown`, `isReady`, `healthCheck`).
- **MUST:** When a native dependency is absent, the plugin must return a structured error result and remain operational rather than crashing the process.
- **MUST:** The manager layer must prefer valid capability-based routing over stale hard-coded assumptions.
- **MUST:** Input handling and backend dispatch must remain explicit and fail-closed when unsupported or malformed input is encountered.

## Mandatory Resource Requirements

- **MUST:** Memory and latency should remain within the explicit thresholds stated in `PERFORMANCE_EXPECTATIONS.md` for the active build profile.
- **MUST:** Module behaviour must remain bounded even when the native runtime dependency is not installed.
- **MUST NOT:** A missing optional backend may silently remove the feature without surfacing an error state to the caller.

## Mandatory Reliability Requirements

- **MUST:** OCR/detection results must be returned as structured result objects with explicit success/error information.
- **MUST:** Dependency or inference failures must be surfaced to the caller without taking down the broader process.
- **MUST NOT:** The module should assume the presence of Tesseract or ONNX Runtime without checking the build and runtime configuration.

## Minimal Production Checklist (Audit-Capable)

- [ ] Native dependencies are guarded by explicit build-time checks
- [ ] Missing dependency paths are surfaced as structured errors
- [ ] Module manager routing is capability-based and explicit
- [ ] No silent crash or undefined-state fallback exists in dependency-missing paths
- [ ] Performance gates are revalidated in the active build profile before a final GA claim

## Sourcecode Verification (Module: image_analysis/production)

- Verified files:
  - `src/image_analysis/tesseract_ocr_plugin.cpp`
  - `src/image_analysis/yolov8_onnx_plugin.cpp`
  - `include/plugins/image_analysis_interface.h`
  - `include/plugins/image_analysis_manager.h`
- Verified controls:
  - Dependency gating is explicit in source.
  - Plugin lifecycle and error handling are implemented in the current source tree.
  - The module contract remains aligned to the actual public API surface instead of a stale `include/image_analysis/` path.
