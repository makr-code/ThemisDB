---
Author: ThemisDB Team
Created: 2026-10-30
Last Updated: 2026-10-30
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# Image Analysis Module Guide

## Purpose and reader

Use this guide to orient a developer changing or integrating `src/image_analysis`.
It is a plugin implementation for OCR and object detection; it is not a
general-purpose image-model suite. Start with the [module README](README.md)
and [architecture](ARCHITECTURE.md), then use the source-facing documents
linked below.

## Fast path for a developer

1. Read [API.md](API.md) and check whether the intended behavior is actually
   implemented. Both concrete plugins currently implement detection; neither
   provides image embedding, captioning, segmentation, or generation.
2. Inspect `src/image_analysis/CMakeLists.txt` and the target compile
   definitions. Confirm whether the selected plugin and its native runtime
   are enabled; “plugin target built” does not guarantee real inference.
3. Configure `PluginConfig` for the deployed model/data, then check
   `initialize()`, `healthCheck()`, and an actual inference result. In stub
   builds, readiness can be true while inference returns `success=false`.
4. Run the focused unit test target
   `module_image_analysis_test_image_analysis_phase1_focused_focused` (CTest
   case `test_image_analysis_phase1_focused_ImageAnalysisFocusedTests`); its
   normal path primarily validates contract and dependency-absent behavior. Do
   not use stub stress/soak tests as model performance evidence.
5. Before claiming readiness, check pending acceptance items in
   [ROADMAP.md](ROADMAP.md) and benchmark status in
   [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md).

## Integration checklist

* **YOLOv8:** supply an ONNX Runtime-enabled build, a compatible `model_path`,
  input dimensions matching the export assumptions, and labels matching the
  model class order. OpenCV is required for decoding image bytes; no-OpenCV
  preprocessing supplies a zero tensor, not a decoded image.
* **OCR:** supply Tesseract and the requested language data. Set
  `tessdata_path`/`language` explicitly when system defaults are not controlled.
  Encoded JPEG/PNG decoding requires OpenCV; without it the code treats input
  as raw grayscale bytes.
* Consume `DetectionResult` only when `success` is true. OCR labels are text
  data; they are not sanitized for SQL/AQL, HTML, shell, or prompt contexts.
* The plugins do not generate embeddings. Use a distinct embedding-capable
  implementation and verify model-space dimensions before indexing vectors.

## Where to make changes

* `src/image_analysis/*.cpp`: current backend implementation
* `src/image_analysis/CMakeLists.txt`: optional dependency wiring
* `include/plugins/image_analysis_interface.h`: shared API and result schema
* `include/plugins/tesseract_ocr_plugin.h`,
  `include/plugins/yolov8_onnx_plugin.h`: concrete plugin contracts
* `tests/image_analysis/`: focused and stub stress tests

Public API or result-schema changes require matching API, implementation,
security/operations, and release-evidence updates. Keep unimplemented
capabilities and planned work explicit; do not promote roadmap items to
implemented behavior.

## Related docs

* [IMPLEMENTATION.md](IMPLEMENTATION.md) — dependency modes and implementation invariants
* [RUNBOOK.md](RUNBOOK.md) — backend failure diagnosis
* [TRUST.md](TRUST.md) — trust boundaries and incident handling
* [RESEARCH.md](RESEARCH.md) — evidence and evaluation limits
