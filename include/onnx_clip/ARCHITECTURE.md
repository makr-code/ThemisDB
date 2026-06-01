> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/onnx_clip/ARCHITECTURE.md -->

# ONNX CLIP Module — Public Header Architecture

**Module Path:** `include/onnx_clip/`  
**Implementation:** `../../src/onnx_clip/`  
**Canonical architecture doc:** [`../../src/onnx_clip/ARCHITECTURE.md`](../../src/onnx_clip/ARCHITECTURE.md)

---

## 1. Overview

`include/onnx_clip/` defines the **public ONNX-runtime CLIP model integration for vision-language embeddings API contract** for ThemisDB.

> **Note:** No public headers exported from this module; ONNX CLIP functionality is accessed via the `acceleration` and `rag` module APIs.
