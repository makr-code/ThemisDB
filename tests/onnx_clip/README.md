# tests/onnx_clip — ONNX CLIP Focused Test Suite

## Overview

This directory contains focused test suites for the ONNX CLIP v0.3.0 hardening phases:

- **Phase 1 (Integration):** OCP-IT-01..12 golden embedding verification
- **Phase 3 (Hot-Swap):** OCP-HS-01..12 dynamic model reloading
- **Phase 4 (Mmap):** OCP-MM-01..12 memory-mapped loading

## Test Registry

### Phase 1: Integration Tests (OCP-IT-*)
**File:** `test_onnx_clip_golden_embeddings_focused.cpp`
**Purpose:** Verify deterministic behavior against real CLIP models

| Test | Scope |
|------|-------|
| OCP-IT-01 | ViT-B/32 initialization |
| OCP-IT-02 | ViT-L/14 initialization |
| OCP-IT-03 | Single image embedding generation (deterministic) |
| OCP-IT-04 | Batch embedding generation (deterministic) |
| OCP-IT-05 | L2 normalization verification |
| OCP-IT-06 | Embedding dimension correctness |
| OCP-IT-07 | Reproducibility: identical inputs → identical embeddings |
| OCP-IT-08 | Health check + statistics |

### Phase 3: Hot-Swap Tests (OCP-HS-*)
**File:** `test_onnx_clip_hot_swap_focused.cpp`
**Purpose:** Verify model reloading without server restart

| Test | Scope |
|------|-------|
| OCP-HS-01..04 | Successful reload scenarios |
| OCP-HS-05..08 | Concurrent inference + reload races |
| OCP-HS-09..12 | Rollback and failure scenarios |

### Phase 4: Memory-Mapped Tests (OCP-MM-*)
**File:** `test_onnx_clip_mmap_focused.cpp`
**Purpose:** Verify memory-mapped model loading

| Test | Scope |
|------|-------|
| OCP-MM-01..04 | Mmap initialization success/failure |
| OCP-MM-05..08 | Memory footprint verification |
| OCP-MM-09..12 | Concurrent inference correctness |

## Build & Test

### Configure
```bash
cmake --preset linux-release -DTHEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX=ON
```

### Build Focused Tests
```bash
cmake --build --preset linux-release \
    --target module_onnx_clip_test_*_focused
```

### Run Tests
```bash
# All ONNX CLIP focused tests
ctest --preset linux-release -V -k "onnx_clip"

# Specific phase
ctest --preset linux-release -V -k "onnx_clip_golden"     # Phase 1
ctest --preset linux-release -V -k "onnx_clip_hot_swap"   # Phase 3
ctest --preset linux-release -V -k "onnx_clip_mmap"       # Phase 4
```

## Test Harness Configuration

**Timeout:** 120 seconds per focused test (per Wave 1 standard)  
**Framework:** GoogleTest (gtest)  
**CMake Integration:** `themis_register_module_focused_test()` macro

## Golden Embedding Strategy

Phase 1 tests use deterministic mock embeddings:

- **Seed:** `kClipGoldenSeed = 42`
- **Models:** ViT-B/32 (512-dim), ViT-L/14 (768-dim)
- **Vectors:** Generated via seeded LCG + FNV-1a for reproducibility
- **Comparison:** L2 distance < 1e-6 between runs

## Mmap Platform Support

| Platform | Status | Implementation |
|----------|--------|-----------------|
| Linux | ✅ | `mmap()` + `munmap()` |
| Windows | ✅ | `CreateFileMapping()` + `MapViewOfFile()` |
| macOS | ✅ | `mmap()` (BSD variant) |
| Fallback | ✅ | Traditional heap loading |

## Reference Documentation

- `src/onnx_clip/ROADMAP.md` — Delivery phases
- `src/onnx_clip/ARCHITECTURE.md` — Component design
- `benchmarks/onnx_clip/README.md` — Performance gates (Phase 2)
- `benchmarks/MEASUREMENT_HYGIENE.md` — Benchmark standards
