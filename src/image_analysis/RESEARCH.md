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

# Image Analysis Research Notes

## Purpose and evidence boundary

This note is for engineers evaluating the OCR and object-detection methods in
`image_analysis`. It records the implemented method and what the available
validation does—and does not—establish. The plugin sources and public headers
are the behavioral evidence; the wiki inventory dated 2026-10-26 is not a fresh
validation. The root [roadmap](../../ROADMAP.md) aggregates
`image_analysis` as a production candidate, while the module
[roadmap](ROADMAP.md) still requires current release-profile benchmarks and
acceptance evidence. These statements describe different maturity views, not
proof that performance gates passed.

## Methods and assumptions

* **OCR:** `TesseractOCRPlugin` calls Tesseract's C++ API, requests full UTF-8
  page text and iterates word-level results. Word confidence is normalized to
  0–1 for the common detection result; each word's rectangle is normalized to
  image coordinates. OCR is exposed through `detectObjects()` as labeled
  regions, while `getLastOcrResult()` exposes the transcript and OCR-specific
  details. It is CPU-only.
* **Detection:** `YOLOv8OnnxPlugin` runs an ONNX Runtime session, preprocesses
  to a configured tensor size (640×640 by default), selects the highest class
  score per candidate, applies confidence filtering and per-class greedy NMS,
  then returns normalized boxes. The postprocessor assumes the standard
  80-class YOLOv8 output with 8,400 anchors; it is not a general ONNX detector
  adapter.
* **Input assumption:** callers supply image bytes. OpenCV, when found at
  configure time, decodes those bytes. The no-OpenCV paths have important
  caveats documented in [IMPLEMENTATION.md](IMPLEMENTATION.md); do not infer
  that any encoded format is safely decoded in every build.
* **Model assumption:** model files, Tesseract language data, and their
  compatibility are deployment inputs. The repository code does not establish
  accuracy for a particular model/data package.

## Evaluation and what is not measured

`tests/image_analysis/test_image_analysis_phase1_focused.cpp` checks plugin
metadata, lifecycle, result shape, unsupported embedding behavior, and
statistics. Its ordinary compile path does not define `HAVE_ONNXRUNTIME` or
`HAVE_TESSERACT`, so these checks exercise dependency-absent behavior unless a
test build explicitly adds those definitions. The stress and integration soak
sources use local stub functions; their operation counts are not OCR/detection
throughput or model-quality evidence.

`PERFORMANCE_EXPECTATIONS.md` lists target P99s, throughput, memory, and cache
expectations. They are targets, not measured results; its release gates remain
pending fresh evidence. No accuracy dataset, precision/recall result, model
parity result, or representative production benchmark is established by the
sources inspected for this note.

## Trade-offs and validation path

The small plugin boundary permits optional native dependencies and structured
failure results. Conversely, dependency availability changes the behavior
behind an otherwise loadable plugin, and image decoding/model-output
assumptions make configuration significant. OCR's single Tesseract API
instance is serialized; separate instances consume more resources. YOLO's
fixed output assumptions constrain model exports, and resize-to-square may
distort aspect ratio.

Before making a quality or speed claim, record the exact build flags and
dependency versions, model/tessdata identity, image corpus and preprocessing,
hardware, and release profile. Run correctness measurements on labeled data
(including empty, malformed, varied-size, and non-COCO samples), then run
latency/throughput and memory benchmarks against the gates in
`PERFORMANCE_EXPECTATIONS.md`. Preserve raw run evidence and do not mark a
target achieved from a stub-only test.

## Related documents

* [MODELING.md](MODELING.md) — model and output assumptions
* [EMBEDDING.md](EMBEDDING.md) — implemented embedding status
* [RUNBOOK.md](RUNBOOK.md) — diagnosis when a backend cannot infer
* [Module roadmap](ROADMAP.md) — outstanding release evidence
