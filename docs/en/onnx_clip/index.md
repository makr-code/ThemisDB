[docs](../../README.md) > [en](../INDEX.md) > [onnx_clip](./index.md)
**Date:** 2026-05-13
**Status:** review
**Primary Source:**
- `include/onnx_clip/README.md`
- `src/onnx_clip/README.md`
- `src/onnx_clip/ARCHITECTURE.md`
- `src/onnx_clip/ROADMAP.md`
- `src/onnx_clip/FUTURE_ENHANCEMENTS.md`

**Reference:**
- Issue: `[Docs][Module] onnx_clip`
- Context: Module-focused documentation refresh for public surface, runtime behavior, and cross-links.

---

# onnx_clip — Module Overview

This secondary page summarizes the refreshed `onnx_clip` module documentation
against the current implementation state.

## Documents

- [PRIMARY_SOURCES](./PRIMARY_SOURCES.md)
- [German secondary README](../../de/onnx_clip/README.md)

## Highlights

- The public usage story is now documented even though the current header still lives in `src/onnx_clip/onnx_clip_plugin.h`.
- The documented API covers image embeddings, text embeddings, bounded batch processing, statistics, health checks, and optional model hash verification.
- `BackendType::AUTO` currently resolves to `CPU` in the generic implementation.
- Follow-up work remains tracked in [`../../../src/onnx_clip/ROADMAP.md`](../../../src/onnx_clip/ROADMAP.md) and [`../../../src/onnx_clip/FUTURE_ENHANCEMENTS.md`](../../../src/onnx_clip/FUTURE_ENHANCEMENTS.md).
