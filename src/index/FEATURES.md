---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Index module features

This inventory identifies feature families implemented by the index module; it
does not imply that every backend is enabled in every build or validated on
every hardware target.

| Feature family | Current source entry points |
|---|---|
| ANN routing and search result metadata | `ann_frontdoor.cpp`, `include/index/ann_frontdoor.h` |
| Vector index lifecycle and search | `vector_index.cpp`, `include/index/vector_index.h` |
| Secondary and full-text indexing | `secondary_index.cpp`, `inverted_index.cpp` |
| Spatial and graph indexing | `spatial_index.cpp`, `graph_index.cpp` |
| GPU-aware vector indexing | `gpu_vector_index.cpp`, `gpu_vector_index_vulkan.cpp` |
| Distributed and tiered vector indexes | `distributed_vector_index.cpp`, `tiered_index_manager.cpp` |
| Compression and quantization | `index_compression.cpp`, `product_quantizer.cpp`, `binary_quantizer.cpp`, `residual_quantizer.cpp` |
| Index manifest serialization | `index_manifest_v1.cpp` |

## Validation and status

Public API details and implementation caveats are documented in
[`API.md`](API.md) and the corresponding headers. Test registration is defined
in [`../../tests/index/CMakeLists.txt`](../../tests/index/CMakeLists.txt).
Backend readiness and remaining validation gates are tracked in
[`ROADMAP.md`](ROADMAP.md); feature presence alone is not evidence of
production readiness or GPU parity.
