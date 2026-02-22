# Image Analysis Plugin – Roadmap

## Current Status

**Status:** ✅ Production-ready

Entry-point: `plugins/image_analysis/onnx_clip/`

| Plugin | Status |
|--------|--------|
| ONNX CLIP (embedding) | ✅ Production |

---

## Short-term Goals (next 1–2 sprints)

- [ ] Add integration tests for ONNX CLIP with a pre-downloaded model fixture.
- [ ] Document all build-flag combinations (CPU / CUDA / DirectML / TensorRT) and their prerequisites.
- [ ] Expose inference latency histogram and batch size metrics to Prometheus.

## Mid-term Goals (1–3 months)

- [ ] **Object detection plugin** (e.g., YOLOv8 via ONNX) – bounding boxes + class labels.
- [ ] **OCR plugin** (Tesseract / EasyOCR) – extract text from images.
- [ ] **Captioning plugin** (BLIP-2 or similar) – generate natural-language descriptions.
- [ ] Batch-processing API: accept multiple images in a single plugin call.
- [ ] Support 768-dim and 1024-dim CLIP variants (currently defaults to 512).

## Long-term Goals (3–12 months)

- [ ] **Video analysis** – frame-level embedding + scene change detection.
- [ ] **Multi-modal retrieval** – combined text + image query via joint embedding space.
- [ ] **Model hot-swap** – replace the ONNX model file without restarting ThemisDB.
- [ ] **Fine-tuned CLIP** – allow users to load domain-specific CLIP variants.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| Object detection plugin MVP | TODO | 🔲 Planned |
| OCR plugin | TODO | 🔲 Planned |
| Video frame analysis | TODO | 🔲 Planned |

## Dependencies

- ONNX Runtime (already in ThemisDB vcpkg)
- Optional: CUDA Runtime, DirectML, TensorRT
- ThemisDB `IImageAnalysisBackend` (`include/plugins/image_analysis_interface.h`)

## Open Questions

- [ ] Should model files be bundled with the plugin or downloaded on first use?
- [ ] What image formats (JPEG, PNG, WebP, AVIF) must be supported as input?

---

*See also: [`future_enhancements.md`](future_enhancements.md)*
