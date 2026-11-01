# Image Analysis Module Roadmap

**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** active
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Contributors

Exact model backend not verifiable from final document text alone.

<!-- Status: plugin-backed implementation | source review: 2026-10-31; validation not run -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · CMakeLists.txt -->
<!-- Source level: level 1 | SOT domain: module-behavior -->

## Current Status

The source-tracked Phase 1–6 delivery is recorded complete in the root `ROADMAP.md`. This is distinct from current release acceptance: real-runtime tests and performance evidence remain outstanding. The shared contract is under `include/plugins/`; the module-local CMake file defines two optional backend targets, but the [root CMakeLists.txt](../../CMakeLists.txt) and [CMake orchestration](../../cmake/CMakeLists.txt) do not currently add `src/image_analysis/` or declare its plugin options.

**Current status:** Tesseract OCR and YOLOv8 ONNX detection plugin sources exist. Missing inference runtimes return structured errors, but OpenCV is a separate optional dependency: without it, OCR does not decode encoded image bytes and YOLO preprocessing uses a zero-filled tensor. The module is not release-validated by this documentation review.

- [x] Tesseract OCR plugin source exists; a target is defined in the module-local CMake file (`src/image_analysis/tesseract_ocr_plugin.cpp`)
- [x] YOLOv8 ONNX plugin source exists; a target is defined in the module-local CMake file (`src/image_analysis/yolov8_onnx_plugin.cpp`)
- [ ] Integrate or explicitly keep the two plugin targets outside the standard root build; no root `add_subdirectory(src/image_analysis)` or declarations for `THEMIS_PLUGIN_IMAGE_TESSERACT` / `THEMIS_PLUGIN_IMAGE_YOLOV8` were found
- [x] Plugin lifecycle and missing-runtime error paths are present in source
- [ ] Validate the runtime/dependency combinations, including OpenCV-enabled encoded-image inference
- [ ] Capture real-plugin benchmark and release-gate evidence; current benchmark sources use mock/synthetic plugins

## In Progress

- [x] align module docs with the live plugin interface and resolve legacy path drift (Target: Q4 2026)
- [~] validate optional-dependency fallback paths under the real build configuration used by tests and CI (Target: Q4 2026)
- [ ] add focused regression coverage for Tesseract and YOLOv8 graceful-failure paths (Target: Q4 2026)
- [ ] refresh representative benchmark evidence for OCR and detection latency against release profile (Target: Q4 2026)

## Implementation Phases

### Phase 1: Design / API Contract

- [x] Define the shared plugin contract in `include/plugins/image_analysis_interface.h`
- [x] Declare plugin discovery and compatibility APIs in `include/plugins/image_analysis_manager.h`
- [ ] Implement/build manager discovery, loading, and capability routing before treating these declared APIs as available
- [x] Document the build-time gating pattern in `src/image_analysis/CMakeLists.txt`
- [x] Align module documentation to `PluginConfig`, structured result types, and plugin-specific capabilities

### Phase 2: Core Implementation

- [x] Implement Tesseract OCR plugin (`src/image_analysis/tesseract_ocr_plugin.cpp`)
- [x] Implement YOLOv8 ONNX detection plugin (`src/image_analysis/yolov8_onnx_plugin.cpp`)
- [x] Define local plugin targets and optional dependency checks in `src/image_analysis/CMakeLists.txt`
- [ ] Validate runtime behavior and configuration defaults in the intended dependency-enabled build

### Phase 3: Error Handling & Edge Cases

- [x] Implement graceful-failure behavior when `HAVE_TESSERACT` or `HAVE_ONNXRUNTIME` is absent (source-reviewed; execution pending)
- [x] Implement structured error results for missing-runtime paths (source-reviewed; execution pending)
- [ ] Add direct negative tests for malformed input and unsupported image payloads
- [ ] Add malformed/oversized input and timeout/resource-bound tests; no explicit image-size limit or request timeout was found in the inspected plugin sources

### Phase 4: Tests

- [x] Focused and stress test sources exist under `tests/image_analysis/`; [`tests/CMakeLists.txt`](../../tests/CMakeLists.txt) discovers this directory's CMake file and creates focused unit executables when tests are configured
- [ ] Run the focused test target in the intended build profile; source presence is not a passing test result
- [ ] Extend coverage for malformed inputs, optional dependency combinations, and manager routing (the current stress file uses local stubs)

### Phase 5: Performance / Hardening

- [x] Mark unsupported numeric performance expectations as unverified proposals in `PERFORMANCE_EXPECTATIONS.md`
- [ ] Benchmark release-profile latency and throughput for OCR and detection under real model/runtime paths
- [ ] Set numeric release gates only after reproducible runner evidence exists

The three checked-in benchmark programs are all mock/synthetic. As shown in
[`benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt), the two
registered under benchmark module `image_analysis` are
`bench_image_analysis` and `bench_image_analysis_latency`; the third,
`benchmark_image_analysis`, is registered under `benchmark_image`.

### Phase 6: Documentation & Acceptance

- [x] Keep governance docs (`AUDIT.md`, `CHANGELOG.md`, `FUTURE_ENHANCEMENTS.md`, `MODULE_GAPS.md`, `PERFORMANCE_EXPECTATIONS.md`, `PRODUCTION_REQUIREMENTS.md`, `SECURITY.md`) present and reconcile their claims with source evidence
- [x] Align module README/architecture and roadmap to the actual plugin builds, optional dependencies, and evidence scope
- [ ] Run the final module acceptance gate once the benchmark and fallback evidence is captured in the current build environment

## Production Readiness Checklist

- [x] Shared plugin contract is present and source-backed
- [x] Core OCR and detection plugin implementations exist
- [x] Missing-runtime inference paths return structured failure results
- [ ] Encoded-image behavior without OpenCV is safe and explicit
- [x] Module governance docs are present and aligned with the source tree
- [ ] Focused release benchmark evidence is complete for the current build environment
- [ ] Final GA sign-off remains pending until representative validation is recorded

## Known Issues & Limitations

1. **Dependency gating** – both plugins operate as optional feature builds and may report a safe failure if their native dependency is not installed.
2. **No settled release benchmark** – module-level performance targets exist, but they are not yet backed by a fresh release-validated run in this environment.
3. **No manager implementation in this module build** – `image_analysis_manager.h` declares an API, but the module CMake file does not build its implementation.
4. **No real-plugin performance evidence** – available benchmark programs use mock/synthetic plugins.

## Breaking Changes

None known at the current source baseline. The active contract remains the shared plugin interface under `include/plugins/`.

## Module Statistics

- **Implementation files:** `src/image_analysis/tesseract_ocr_plugin.cpp`, `src/image_analysis/yolov8_onnx_plugin.cpp`, `src/image_analysis/CMakeLists.txt`
- **Plugin contract:** `include/plugins/image_analysis_interface.h`, `include/plugins/image_analysis_manager.h`
- **Runtime backends:** OCR (Tesseract) + detection (YOLOv8 ONNX)
- **Current module posture:** source-tracked Phase 1–6 delivery; optional dependencies; release acceptance and real-backend evidence outstanding

## Program Execution Model — Wave Context

This module remains a contributing image-vision capability within ThemisDB's broader multi-wave execution model. The root roadmap's `PRODUCTION_CANDIDATE` classification and Phase 1–6 history do not establish that the image-analysis release checks listed here have passed.

See [`../../ROADMAP.md`](../../ROADMAP.md) for the broader wave model and exit criteria.
