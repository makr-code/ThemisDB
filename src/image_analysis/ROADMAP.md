# Image Analysis Module Roadmap

<!-- Status: IN_PROGRESS | plugin-backed implementation | validated: 2026-10-07 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · CMakeLists.txt -->

## Current Status

The live implementation is in progress and source-first. The module is not a standalone `include/image_analysis/` API tree; the actual contract is defined under `include/plugins/`, while the dependency-gated implementations remain in `src/image_analysis/` and are built by `src/image_analysis/CMakeLists.txt`.

**Current status:** OCR and YOLOv8 plugins are present and behave gracefully when optional dependencies are absent, but the module still requires explicit release-gate validation and documentation alignment before it should be treated as fully production-ready.

- [x] Tesseract OCR plugin implementation exists and is wired through the module build (`src/image_analysis/tesseract_ocr_plugin.cpp`)
- [x] YOLOv8 ONNX plugin implementation exists and is wired through the module build (`src/image_analysis/yolov8_onnx_plugin.cpp`)
- [~] Plugin lifecycle and fallback semantics are source-backed but still need release-evidence alignment and targeted validation
- [ ] Representative benchmark and release-gate evidence for OCR/detection performance should be refreshed under the module's actual plugin contract

## In Progress

- [~] align module docs with the live plugin interface and remove stale `include/image_analysis/*` references (Target: Q4 2026)
- [~] validate optional-dependency fallback paths under the real build configuration used by tests and CI (Target: Q4 2026)
- [ ] add focused regression coverage for Tesseract and YOLOv8 graceful-failure paths (Target: Q4 2026)
- [ ] refresh representative benchmark evidence for OCR and detection latency against release profile (Target: Q4 2026)

## Implementation Phases

### Phase 1: Design / API Contract

- [x] Define the shared plugin contract in `include/plugins/image_analysis_interface.h`
- [x] Define plugin discovery and compatibility flow in `include/plugins/image_analysis_manager.h`
- [x] Document the build-time gating pattern in `src/image_analysis/CMakeLists.txt`
- [ ] Confirm `PluginConfig` and runtime result contracts are fully reflected in module documentation and examples

### Phase 2: Core Implementation

- [x] Implement Tesseract OCR plugin (`src/image_analysis/tesseract_ocr_plugin.cpp`)
- [x] Implement YOLOv8 ONNX detection plugin (`src/image_analysis/yolov8_onnx_plugin.cpp`)
- [x] Wire plugin builds and optional dependency checks in `src/image_analysis/CMakeLists.txt`
- [ ] Confirm final runtime behaviour and default config values against the current test matrix

### Phase 3: Error Handling & Edge Cases

- [x] Validate graceful-failure behaviour when `HAVE_TESSERACT` or `HAVE_ONNXRUNTIME` is absent
- [x] Return structured error states instead of crashing the host process
- [ ] Add direct negative tests for malformed input and unsupported image payloads
- [ ] Capture timeout and fallback policy in a module-specific integration test

### Phase 4: Tests

- [x] Existing focused and stress tests are present under `tests/image_analysis/` and `tests/integration/`
- [ ] Validate the exact plugin fallback mode under local CI/build presets, not only static source review
- [ ] Extend the module test matrix to cover capability selection and manager routing edge cases

### Phase 5: Performance / Hardening

- [~] Keep performance expectations documented in `PERFORMANCE_EXPECTATIONS.md`
- [ ] Benchmark release-profile latency and throughput for OCR and detection under real model paths
- [ ] Reconcile internal performance targets with actual runner evidence before signing off GA readiness

### Phase 6: Documentation & Acceptance

- [x] Restore governance docs (`AUDIT.md`, `CHANGELOG.md`, `FUTURE_ENHANCEMENTS.md`, `MODULE_GAPS.md`, `PERFORMANCE_EXPECTATIONS.md`, `PRODUCTION_REQUIREMENTS.md`, `SECURITY.md`)
- [x] Align module README/architecture and roadmap to the source-backed plugin model
- [ ] Run the final module acceptance gate once the benchmark and fallback evidence is captured in the current build environment

## Production Readiness Checklist

- [x] Shared plugin contract is present and source-backed
- [x] Core OCR and detection plugin implementations exist
- [x] Graceful fallback behaviour is documented and implemented
- [x] Module governance docs are present and aligned with the source tree
- [ ] Focused release benchmark evidence is complete for the current build environment
- [ ] Final GA sign-off remains pending until representative validation is recorded

## Known Issues & Limitations

1. **Dependency gating** – both plugins operate as optional feature builds and may report a safe failure if their native dependency is not installed.
2. **No settled release benchmark** – module-level performance targets exist, but they are not yet backed by a fresh release-validated run in this environment.
3. **No fully frozen standalone API** – the canonical public contract is plugin-facing, not a dedicated `include/image_analysis/` library tree.
4. **Documentation drift risk** – stale path references and legacy production claims must be kept out of the module narrative.

## Breaking Changes

None known at the current source baseline. The active contract remains the shared plugin interface under `include/plugins/`.

## Module Statistics

- **Implementation files:** `src/image_analysis/tesseract_ocr_plugin.cpp`, `src/image_analysis/yolov8_onnx_plugin.cpp`, `src/image_analysis/CMakeLists.txt`
- **Plugin contract:** `include/plugins/image_analysis_interface.h`, `include/plugins/image_analysis_manager.h`
- **Runtime backends:** OCR (Tesseract) + detection (YOLOv8 ONNX)
- **Current module posture:** in progress, dependency-gated, and documentation-aligned to source evidence

## Program Execution Model — Wave Context

This module remains a contributing image-vision capability within ThemisDB's broader multi-wave execution model. It should be treated as operationally relevant but not release-final until the module-specific benchmark and fallback evidence is refreshed against the active build profile.

See [`../../ROADMAP.md`](../../ROADMAP.md) for the broader wave model and exit criteria.
