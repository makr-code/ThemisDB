# Image Analysis Module — Architecture

<!-- Status: IN_PROGRESS | plugin architecture | validated: 2026-10-07 -->

## Overview

The image analysis module is a plugin-based vision capability for ThemisDB. The code path that is actually present in this tree is not a top-level `include/image_analysis/` API surface; instead, the canonical runtime contract lives under `include/plugins/`, and the module-specific implementation is assembled from `src/image_analysis/*.cpp` plus `src/image_analysis/CMakeLists.txt`.

## Design Principles

1. **Plugin abstraction:** image analysis backends are selected through `IImageAnalysisBackend` and `ImageAnalysisManager`.
2. **Optional dependency gating:** Tesseract and ONNX Runtime are compiled in only when present, with explicit error results otherwise.
3. **Fail-graceful behavior:** backend absence or runtime failures return structured error states instead of crashing the caller.
4. **Dependency separation:** the core plugin contract stays in `include/plugins/`; module-specific implementation remains in `src/image_analysis/`.
5. **Source-first validation:** roadmap and performance expectations are treated as target gates, not as claims of already-fixed production status.

## Architecture Diagram

```
┌──────────────────────────────────────────────────────────────┐
│ ImageAnalysisManager / PluginConfig                          │
│ • discovers + registers backend implementations               │
│ • chooses backends by capability and runtime availability     │
└──────────────────────────────┬───────────────────────────────┘
                              │
                              ▼
                   ┌──────────────────────────────┐
                   │ IImageAnalysisBackend        │
                   │ common contract              │
                   └──────────────┬───────────────┘
                                  │
                 ┌────────────────┼────────────────┐
                 │                │                │
                 ▼                ▼                ▼
      ┌────────────────────┐ ┌────────────────────┐ ┌────────────────────┐
      │ TesseractOCRPlugin  │ │ YOLOv8OnnxPlugin    │ │ Future plugin(s)    │
      │ OCR + layout boxes  │ │ detection + NMS     │ │ adapter expansion   │
      └────────────────────┘ └────────────────────┘ └────────────────────┘
                                  │
                                  ▼
                   Structured result objects
                   (DetectionResult, EmbeddingResult,
                   CaptionResult, PluginInfo)
```

## Core Components

### ImageAnalysisManager

**Purpose:** Maintain the plugin registry and select the best backend for a requested capability.

**Responsibilities:**
- discover and load plugins from a configured directory
- validate plugin metadata and compatibility
- keep the default plugin selected for common operations
- provide a manager API for detection and capability queries

### IImageAnalysisBackend Contract

**Purpose:** Define the stable runtime contract for all image-analysis backends.

**Key responsibilities:**
- initialize / shutdown lifecycle
- readiness and backend reporting
- detection and embedding entry points
- statistics and health checking

### OCR Backend (Tesseract)

**Purpose:** Extract OCR text and word-level bounding boxes from input images.

**Actual implementation:**
- `src/image_analysis/tesseract_ocr_plugin.cpp`
- `include/plugins/tesseract_ocr_plugin.h`

**Behavior:**
- when `HAVE_TESSERACT` is defined, real OCR runs with Tesseract API
- without the dependency, return a well-formed failure result with an explanatory error message

### Object Detection Backend (YOLOv8 ONNX)

**Purpose:** Run YOLOv8 detection and return normalised bounding boxes with confidence scores.

**Actual implementation:**
- `src/image_analysis/yolov8_onnx_plugin.cpp`
- `include/plugins/yolov8_onnx_plugin.h`

**Behavior:**
- when `HAVE_ONNXRUNTIME` is defined, model inference runs with ONNX Runtime
- otherwise, the plugin reports a controlled failure rather than failing the process

## Data Flow

### Plugin-driven analysis flow

```
Caller / indexer
   │
   ├─► ImageAnalysisManager::getBestPluginForCapability()
   │
   ├─► Backend initialize() / isReady()
   │
   ├─► detectObjects() / generateEmbedding()
   │
   └─► Structured Result (success + error_message + detections / embedding)
```

## Performance Characteristics

Performance is treated as target-based evidence rather than solved production status. The module keeps quantitative expectations in `PERFORMANCE_EXPECTATIONS.md`, but the current implementation is still best described as plugin-backed and partially hardened rather than fully release-validated.

## Error Handling

### Graceful Degradation

1. **Tesseract unavailable** → plugin reports a safe error result
2. **ONNX Runtime unavailable** → detection returns an explicit failure message
3. **Invalid inputs** → backend returns `success = false` with context
4. **Backend startup failure** → manager keeps the module operational while the failure is surfaced to the caller

### Error Codes / failures

The operational contract is the plugin interface's structured result types, not an undocumented `include/image_analysis/` API that is absent in the live tree.

## Integration Points

### ThemisDB integration

- `include/plugins/image_analysis_interface.h` is the canonical plugin contract
- `include/plugins/image_analysis_manager.h` owns backend discovery and capability routing
- `src/image_analysis/CMakeLists.txt` wires optional runtime dependencies
- `src/image_analysis/*.cpp` implement the actual plugin behaviour and dependency fallbacks

## See Also

- [`README.md`](README.md) — module overview and current status
- [`ROADMAP.md`](ROADMAP.md) — implementation phases and open items
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) — planned module enhancements
- [`../../include/plugins/image_analysis_interface.h`](../../include/plugins/image_analysis_interface.h) — canonical contract

---

### Direct Downstream Consumers

| Module | Via | Notes |
|--------|-----|-------|
| `plugins` | `include/plugins/image_analysis_interface.h` | Shared plugin contract for image analysis backends |
| `onnx_clip` | `include/plugins/image_analysis_interface.h` | Embedding-oriented vision path shares the same interface |
| `content` | manager + plugin selection | image processing is integrated via plugin resolution rather than a dedicated `include/image_analysis/` header tree |
