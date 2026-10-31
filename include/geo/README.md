# Geo public headers

This directory declares public geo interfaces. Implementations are primarily
under `src/geo/`; shared EWKB and parsed
geometry types are also declared in
`include/utils/geo/`.

## Interface map

| Header | Public surface |
|---|---|
| `geo_api_contract.h` | `GeoErrorCode` and contract constants |
| `geo_policy.h` | Per-query policy fields, validation, and advisory collection filtering |
| `geo_json_geometry.h` | `IGeoJSONGeometry`, concrete GeoJSON geometry types, CRS identifiers, validation |
| `geo_math.h` | Geodesic and geometry math helpers |
| `geo_ops_ext.h` | `IGeoOpsExtension` extension-discovery interface |
| `spatial_backend.h` | `ISpatialComputeBackend`, precision mode, backend registry/getters and diagnostic reports |
| `geo_backend_dispatch.h` | `GeoBackendDispatcher` batched Haversine, Vincenty and point-in-polygon methods |
| `gpu_kernel_dispatcher.h` | `GpuKernelDispatcher` CUDA/HIP-facing distance and containment dispatch |
| `device_detector.h` | Geo device capability detection |
| `geo_rtree.h`, `rtree_cursor.h` | In-memory R-tree and cursor/index interfaces |
| `spatial_join.h`, `spatial_join_filter.h` | Spatial joins, iterator and filter predicates |
| `geo_clustering.h` | `dbscanCluster()` and `kmeansCluster()` |
| `geo_faiss_knn.h` | Point-only geographic nearest-neighbor and radius search |
| `raster.h`, `raster_query_interface.h` | Raster grid and typed raster query/status interfaces |
| `temporal_spatial_query.h`, `temporal_spatial_query_builder.h` | Versioned-geometry query and builder |
| `tile_server.h` | XYZ/Web Mercator helpers and vector-tile encoding |
| `geo_edge_case_handler.h`, `geo_operator_diagnostics.h`, `gpu_buffer_guard.h`, `phase2_phase3_hardening.h` | Hardening, diagnostics and resource-guard interfaces |

The interface names above are a navigation aid, not a promise that every
operation uses the same backend or is enabled in every build. Consult the
specific declaration and implementation for input order, units, status/error
behavior, ownership, and fallback details.

## Build-selection notes

- The parent build uses the integrated `src/geo/` implementation by default.
  `THEMIS_EXTERNALIZE_GEO_PLUGIN=ON` selects the optional plugin only when
  `THEMIS_PLUGIN_GEO` is enabled and its configured submodule has a
  `CMakeLists.txt`; otherwise the integrated implementation remains active.
  The external plugin's behavior is outside this header inventory.
- The integrated geo CMake sources select CUDA when `THEMIS_GEO_CUDA` is enabled; otherwise
  HIP when `THEMIS_GEO_HIP` is enabled; otherwise the CPU dispatcher is selected.
  The repository configuration makes CUDA take precedence over HIP.
- `THEMIS_GEO_BOOST_BACKEND` is a compile definition used for the optional
  Boost-backed implementation.
- `raster_query_interface.h` declares `THEMIS_ENABLE_RASTER` as a compile-time
  guard and provides `NoOpRasterQueryImpl`, which returns
  `RasterStatus::NOT_SUPPORTED`. The inspected geo CMake feature file does not
  define a matching user-facing option. **TODO:** verify the owning build's
  raster enablement mechanism before documenting a configuration switch.
- Compile-time support does not prove runtime device availability or execution
  on a GPU. In particular, `GeoBackendDispatcher::computeVincentyBatch()`
  currently runs its CPU implementation.

No `geo.backend`, `geo.precision_mode`, or `geo.gpu.batch_size` runtime setting
was found in the inspected geo declarations or CMake configuration.

## Include usage

The parent project exposes the repository `include/` directory to its
consumers. For example:

```cpp
#include "geo/geo_api_contract.h"
#include "geo/geo_json_geometry.h"
#include "geo/spatial_backend.h"
```

## Canonical documentation

- Contract/interface guide:
  `src/geo/API.md`.
- Component and build boundaries:
  `src/geo/ARCHITECTURE.md`.
- Implementation and test registration:
  `src/geo/IMPLEMENTATION.md` and `tests/geo/CMakeLists.txt`.

---

**AI_Used:** true · **AI_Models:** unknown · **AI_Contributor:** GitHub Copilot coding agent · **Human_Review:** false · **Human_Author:** ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
