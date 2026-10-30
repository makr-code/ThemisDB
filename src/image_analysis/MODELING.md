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

# Image Analysis Modeling

## Purpose and scope

This document describes the model-side contract implemented by the OCR and
detection plugins. It is for maintainers selecting model assets or diagnosing
output mismatches. It does not claim that a particular model file or quality
level has been validated. See [RESEARCH.md](RESEARCH.md) for evaluation
evidence and [IMPLEMENTATION.md](IMPLEMENTATION.md) for build-time behavior.

## OCR model/data path

`TesseractOCRPlugin` initializes `TessBaseAPI` with `tessdata_path` (empty
means pass a null/default data directory) and language `eng` by default.
`language` may select another Tesseract language or a combination such as
`eng+deu`; the corresponding language data must be present at runtime.
Optional configuration includes `page_seg_mode` (default 3), `dpi` (default
70), `whitelist_chars`, and `min_confidence` (default 0.0). The plugin
extracts page text and per-word confidence/rectangles. The public OCR word
confidence is on Tesseract's 0–100 scale; the common detection box confidence
is divided by 100.

There is a configuration-scale mismatch to preserve during integration:
`tesseract_ocr_plugin.h` describes `min_confidence` on a 0–100 scale, but the
implementation reads the configured value unchanged and compares it with
`word_confidence / 100` (0–1). Until the source contract is reconciled, the
runtime comparison behaves as a 0–1 threshold; do not infer conversion from
the header wording.

OCR is not a learned image embedding. Changing language data, segmentation
mode, DPI, or character whitelist can change recognition results and must be
treated as a behavior change in evaluation.

## YOLOv8 detection model contract

`YOLOv8OnnxPlugin` requires an ONNX model path in builds where ONNX Runtime is
compiled in. Default input width and height are 640; defaults are confidence
threshold 0.25, NMS IoU threshold 0.45, and maximum 100 returned boxes. An
optional labels file supplies one label per line; if it is absent or unreadable,
the implementation keeps its built-in 80 COCO labels.

The postprocessor expects the conventional 80-class tensor layout
`[1, 84, 8400]` for 640×640 input: four center-format box channels followed by
80 class scores, channel-major with 8,400 anchors. It accepts a flattened
buffer of at least `84 * 8400` floats; it does not validate the full tensor
shape. It chooses the best class score for each anchor, filters below the
effective threshold, converts and clips box coordinates to 0–1, sorts by
confidence, suppresses overlapping boxes of the same class above the IoU
threshold, and caps returned detections. A custom export with different
dimensions, class count, output ordering, or score semantics is not
automatically compatible.

The image is resized to the configured square dimensions (not letterboxed).
That is a modeling trade-off: simple preprocessing, but aspect-ratio distortion
can affect detections. Labels must correspond exactly to the model's class
index order. No model-specific accuracy or calibration evidence is included
here.

## Capability boundary

The broad `IImageAnalysisBackend` interface declares embedding, captioning,
segmentation, generation, and visual-QA operations. That is a shared contract,
not proof that these plugins implement those models. Both concrete plugins
report detection capability only. Their `generateEmbedding()` methods return
an explicit unsupported error; this module does not currently implement image
embeddings. See [EMBEDDING.md](EMBEDDING.md).

## Reproducible model evaluation

For any model or data update, preserve: model file identity and provenance,
labels-file identity, configuration values, dependency/build flags, input
corpus and labels, runtime/provider, and hardware. Measure detection quality
and confidence calibration on task-specific labeled samples, and check
coordinate correctness across image aspect ratios and dimensions. Run the
focused plugin test and a release-profile benchmark. Targets in
[PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) are not measured
results; do not report them as achieved without run artifacts.

## Source references

* [`yolov8_onnx_plugin.h`](../../include/plugins/yolov8_onnx_plugin.h)
* [`tesseract_ocr_plugin.h`](../../include/plugins/tesseract_ocr_plugin.h)
* [`image_analysis_interface.h`](../../include/plugins/image_analysis_interface.h)
* [Future model-registry and validation work](FUTURE_ENHANCEMENTS.md)
