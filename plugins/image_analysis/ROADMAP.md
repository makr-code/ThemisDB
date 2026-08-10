# Image Analysis Plugin – Roadmap

## Current Status

**Status:** ✅ Production-ready

Entry-point: `plugins/image_analysis/onnx_clip/CMakeLists.txt` (compatibility shim) · implementation: `src/onnx_clip/` · public API: `include/plugins/image_analysis/onnx_clip_plugin.h`

| Plugin | Status |
|--------|--------|
| ONNX CLIP (embedding) | ✅ Production |
| YOLOv8 ONNX (object detection) | 🟡 Phase 1 delivered – awaiting ONNX Runtime integration tests |
| Tesseract OCR | 🟡 Phase 1 delivered – awaiting real Tesseract fixture tests |

---

## In Progress

- [~] Integration tests for ONNX CLIP with a pre-downloaded model fixture
- [~] Documentation of all build-flag combinations (CPU / CUDA / DirectML / TensorRT) and prerequisites

## Planned Features

- [x] **Object detection plugin** (YOLOv8 via ONNX) – bounding boxes + class labels (Target: Q3 2026)
- [x] **OCR plugin** (Tesseract / EasyOCR) – extract text from images (Target: Q3 2026)
- [ ] **Captioning plugin** (BLIP-2) – generate natural-language descriptions (Target: Q3 2026)
- [ ] Batch-processing API: multiple images in a single plugin call (Target: Q3 2026)
- [ ] Support 768-dim and 1024-dim CLIP variants (Target: Q3 2026)
- [ ] **Video analysis** – frame-level embedding + scene change detection (Target: Q4 2026)
- [ ] **Multi-modal retrieval** – combined text + image query via joint embedding space (Target: Q4 2026)
- [ ] **Model hot-swap** – replace ONNX model file without restarting ThemisDB (Target: 2027)
- [ ] **Fine-tuned CLIP** – load domain-specific CLIP variants (Target: 2027)

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

## Implementation Phases

### Phase 1 – Object Detection & OCR Plugins
- [x] Implement `YOLOv8OnnxPlugin` using ONNX Runtime; output bounding boxes + confidence scores
  - `include/plugins/yolov8_onnx_plugin.h` + `src/image_analysis/yolov8_onnx_plugin.cpp`
  - COCO 80-class NMS post-processing; CPU/CUDA EP fallback; per-call latency counters
- [x] Implement `TesseractOCRPlugin` wrapping Tesseract C API; output text + layout regions
  - `include/plugins/tesseract_ocr_plugin.h` + `src/image_analysis/tesseract_ocr_plugin.cpp`
  - Per-word bounding boxes + confidence; full page text via `getLastOcrResult()`
- [x] Unit tests: IMP-YOL-01..08 + IMP-OCR-01..08 in `tests/image_analysis/test_image_analysis_phase1_focused.cpp`
- [x] Inference latency histogram metrics exported via `getStatistics()` for both plugins

### Phase 2 – Captioning & Batch API
- [ ] Implement `BLIP2CaptioningPlugin` (ONNX export of BLIP-2 encoder-decoder)
- [ ] Batch-processing API: `analyzeImages(std::vector<ImageData>)` with configurable batch size
- [ ] Support 768-dim and 1024-dim CLIP model variants via `plugin.json` configuration

### Phase 3 – Video Analysis & Multi-Modal Retrieval
- [ ] Video frame extractor: configurable FPS sampling, scene-change detection
- [ ] Per-frame embedding pipeline feeding into ThemisDB vector index
- [ ] Multi-modal retrieval: combined text + image AQL query using joint embedding space

### Phase 4 – Model Hot-Swap & Fine-Tuning
- [ ] Model hot-swap: atomic model file replacement without service restart
- [ ] Fine-tuned CLIP variant loading: validate model signature before swap
- [ ] CUDA / DirectML inference backend integration tests in CI (with GPU runner)

---

## Dependencies

- ONNX Runtime (already in ThemisDB vcpkg)
- Optional: CUDA Runtime, DirectML, TensorRT
- ThemisDB `IImageAnalysisBackend` (`include/plugins/image_analysis_interface.h`)

## Open Questions

- [ ] Should model files be bundled with the plugin or downloaded on first use?
- [ ] What image formats (JPEG, PNG, WebP, AVIF) must be supported as input?

---

## Production Readiness Checklist

| Item | Status |
|------|--------|
| ONNX CLIP (512-dim) embedding | ✅ Ready |
| Object detection (YOLOv8) | ❌ Not implemented |
| OCR plugin (Tesseract) | ❌ Not implemented |
| Captioning plugin (BLIP-2) | ❌ Not implemented |
| Video analysis | ❌ Not implemented |
| Integration tests with model fixture in CI | ❌ Pending (model file not in CI) |
| CUDA / DirectML inference tested in CI | ❌ Not tested |

## Known Issues & Limitations

- Integration tests require a pre-downloaded ONNX model file that is not included in CI; tests are skipped or manual only
- CUDA and DirectML inference backends have not been tested in CI; correctness on those paths is unverified
- Only 512-dim CLIP is supported; 768-dim and 1024-dim variants require additional work

---

*See also: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)*
