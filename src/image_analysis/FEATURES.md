---
Author: ThemisDB Contributors
Created: 2026-10-18
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
---

> Exact model backend not verifiable from final document text alone.

# Image Analysis Features

This page lists behavior present in the two concrete plugin implementations.
The shared interface declares more operations than these plugins implement.
Both concrete `getInfo()` implementations advertise detection only; neither
advertises image/text embedding or plugin discovery/selection.

## Implemented

| Feature | Implementation | Limits |
|---|---|---|
| OCR | `TesseractOCRPlugin` maps recognized words to detection boxes through `detectObjects()` and exposes the full transcript through `getLastOcrResult()` | Requires a build with Tesseract for OCR. Without OpenCV, input bytes are treated as raw grayscale data, not decoded image files. |
| Object detection | `YOLOv8OnnxPlugin` runs a compatible ONNX Runtime model and returns normalized boxes | Requires ONNX Runtime and a compatible model. Without OpenCV, preprocessing uses a zero-filled tensor rather than decoding the input. |
| Structured results and lifecycle | Both plugins implement initialization, shutdown, readiness, health checks, statistics, and structured detection results | In a build missing the native runtime, initialization/readiness may succeed while inference returns an error. |
| Optional dependency wiring | The module-local CMake file defines the Tesseract and YOLO plugin targets and probes their runtime and OpenCV dependencies | The root build does not currently add `src/image_analysis/` or declare the module's two plugin options; standalone module CMake wiring is not root-build integration. |

## Not implemented by these plugins

- Image embedding generation; both concrete `generateEmbedding()` methods return unsupported-operation errors.
- `generateTextEmbedding()` is unsupported by the shared interface default and neither concrete plugin overrides it.
- Captioning, segmentation, image generation, visual question answering, and real-time video analysis.
- Manager discovery, registration, and capability selection. `ImageAnalysisManager` APIs are declarations in `include/plugins/image_analysis_manager.h`; no implementation is built by this module.
- Batch processing advertised by either concrete backend.

## Test and benchmark evidence boundary

The focused test directory has two source files. Its CMake file is discovered by
`tests/CMakeLists.txt`, compiles plugin sources into standalone test executables,
and does not enable the Tesseract or ONNX Runtime compile paths. The test sources
therefore do not establish native-runtime inference or model accuracy.

All three checked-in benchmark programs use mock/synthetic plugins. The two
targets registered to benchmark module `image_analysis` when present are
`bench_image_analysis` and `bench_image_analysis_latency`; the third,
`benchmark_image_analysis`, is registered to `benchmark_image` when present.
The benchmark suite returns early unless `THEMIS_BUILD_BENCHMARKS=ON` and
Google Benchmark is found; the target helper skips missing source files.
Registration in CTest is also conditional on the target existing. The root
project defaults the option to `OFF`. No real OCR or detection performance
result is established by these sources. See
[`benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt) and the root
[`CMakeLists.txt`](../../CMakeLists.txt).

## Canonical sources

- [`image_analysis_interface.h`](../../include/plugins/image_analysis_interface.h)
- [`tesseract_ocr_plugin.h`](../../include/plugins/tesseract_ocr_plugin.h)
- [`yolov8_onnx_plugin.h`](../../include/plugins/yolov8_onnx_plugin.h)
- [`src/image_analysis/CMakeLists.txt`](CMakeLists.txt)
- [`tests/image_analysis/CMakeLists.txt`](../../tests/image_analysis/CMakeLists.txt)
- [`tests/CMakeLists.txt`](../../tests/CMakeLists.txt)
- [`benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt)
- [`CMakeLists.txt`](../../CMakeLists.txt) and [`cmake/CMakeLists.txt`](../../cmake/CMakeLists.txt)
