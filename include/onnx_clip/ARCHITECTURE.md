---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "src/onnx_clip/onnx_clip_plugin.h"
  - "src/onnx_clip/onnx_clip_plugin.cpp"
  - "include/plugins/image_analysis_interface.h"
Related_Issue: "#7365"
---

Exact model backend not verifiable from final document text alone.

<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/onnx_clip/ARCHITECTURE.md · ../../src/onnx_clip/PRODUCTION_REQUIREMENTS.md -->

# ONNX CLIP Documentation Directory Architecture

**Module Path:** `include/onnx_clip/`  
**Implementation:** `../../src/onnx_clip/`  
**Canonical architecture doc:** [`../../src/onnx_clip/ARCHITECTURE.md`](../../src/onnx_clip/ARCHITECTURE.md)

---

## 1. Overview

`include/onnx_clip/` currently contains module documentation, not C++ headers.
The plugin declaration is source-local at
[`src/onnx_clip/onnx_clip_plugin.h`](../../src/onnx_clip/onnx_clip_plugin.h).
It declares `themis::plugins::image::ONNXClipPlugin`, which implements
[`IImageAnalysisBackend`](../plugins/image_analysis_interface.h). In-tree
consumers include the header through the repository `src/` include path; this
directory does not provide an installed public header. The implementation
currently returns deterministic reference vectors and does not execute ONNX
Runtime.

For the API and runtime behavior, see the canonical
[`src/onnx_clip/ARCHITECTURE.md`](../../src/onnx_clip/ARCHITECTURE.md).
