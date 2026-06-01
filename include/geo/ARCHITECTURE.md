> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/geo/ARCHITECTURE.md -->

# GEO Module — Public Header Architecture

**Module Path:** `include/geo/`
**Implementation:** `../../src/geo/`
**Canonical architecture doc:** [`../../src/geo/ARCHITECTURE.md`](../../src/geo/ARCHITECTURE.md)

---

## 1. Overview

The `include/geo/` directory contains the **public C++ header contract** for ThemisDB's CPU/GPU spatial backends, indexing structures, geometry processing, and advanced geo workflows for geospatial workloads. Headers define types, interfaces, and configuration structures consumed by internal implementation files and embedders.

All headers are `#pragma once` guarded and contain no implementation code.

For full architectural details — data flow diagrams, threading model, integration point map — see the canonical document:

→ [`../../src/geo/ARCHITECTURE.md`](../../src/geo/ARCHITECTURE.md)

---

## 2. Namespace

All public types live under `themis::geo`.

---

## 3. Header Surface Map

| Execution Plane | Key Headers |
|---|---|
| `Backend execution` | `spatial_backend.h`, `gpu_kernel_dispatcher.h`, `device_detector.h` |
| `Spatial index and geometry` | `geo_rtree.h`, `rtree_cursor.h`, `geo_json_geometry.h`... |
| `Query and analytics` | `spatial_join.h`, `spatial_join_filter.h`, `geo_clustering.h`... |

Full header list: see [`README.md`](README.md).

---

## 4. Build Conditionals

| CMake Symbol | Headers Affected | Required Dependency |
|---|---|---|
| `THEMIS_ENABLE_CUDA or THEMIS_ENABLE_HIP` | gpu_kernel_dispatcher.h | GPU spatial execution (CUDA or HIP) |
| `THEMIS_ENABLE_FAISS` | geo_faiss_knn.h | FAISS k-NN search library |

---

## 5. Compatibility and Stability

- **ABI stability:** Public types follow semantic versioning; breaking changes trigger a major version bump.
- **No implementation code:** Headers contain only declarations and `constexpr`/template helpers.
- **`[[nodiscard]]`:** Factory functions and error-returning methods use `[[nodiscard]]`.

---

## 6. References

- Full architecture: [`../../src/geo/ARCHITECTURE.md`](../../src/geo/ARCHITECTURE.md)
- Module overview: [`../../src/geo/README.md`](../../src/geo/README.md)
- Roadmap: [`../../src/geo/ROADMAP.md`](../../src/geo/ROADMAP.md)
- Future enhancements: [`../../src/geo/FUTURE_ENHANCEMENTS.md`](../../src/geo/FUTURE_ENHANCEMENTS.md)
- Public header overview: [`README.md`](README.md)
