# Architecture — Geo Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · API.md · IMPLEMENTATION.md · ROADMAP.md -->

## Scope and ownership

The public declarations live under `include/geo/`
(with shared geometry representations under
`include/utils/geo/`). Implementations and
build selection live under `src/geo/`.
The module contains geometry parsing and operations, CPU/GPU compute paths,
in-memory spatial indexing, joins, clustering, raster and temporal-spatial
queries, tile helpers, and a point-only k-NN bridge.

This is a map of component ownership, not a claim that every operation passes
through one common query pipeline or has identical limits and fallback behavior.
Use the individual header and implementation for operation-specific guarantees.

## Runtime components

| Component | Representative public surface | Role |
|---|---|---|
| Geometry and contract | `include/geo/geo_api_contract.h`, `include/geo/geo_json_geometry.h`, `include/utils/geo/ewkb.h` | Error vocabulary, typed GeoJSON objects, and parsed geometry representation |
| Compute backends | `spatial_backend.h`, `geo_backend_dispatch.h`, `gpu_kernel_dispatcher.h` | CPU backend registry and separate batched kernel dispatch surfaces |
| Index and joins | `geo_rtree.h`, `rtree_cursor.h`, `spatial_join.h`, `spatial_join_filter.h` | In-memory MBR index, cursor API, joins, and predicates |
| Additional workflows | `geo_clustering.h`, `raster_query_interface.h`, `temporal_spatial_query.h`, `tile_server.h`, `geo_faiss_knn.h` | Clustering, raster queries, versioned-geometry queries, tile conversion/encoding, and point k-NN |

These are API groupings; the headers do not imply shared storage ownership.
`GeoRTree` owns its in-memory index, while persistent document access and
request authorization belong to the owning integration.

## Build and dispatch flow

The default integrated build uses the geo implementation under
`src/geo/`. The parent CMake also supports
`THEMIS_EXTERNALIZE_GEO_PLUGIN=ON`: when `THEMIS_PLUGIN_GEO` is enabled and the
configured plugin submodule has a `CMakeLists.txt`, the external plugin replaces
the integrated geo sources. If those conditions are not met, CMake retains the
integrated implementation. The API/implementation descriptions in this module
refer to the integrated source unless explicitly stated; external-plugin
behavior was not audited here.

In the integrated build, `src/geo/CMakeLists.txt`
adds the core geo sources to the parent build. It selects the CUDA source when
`THEMIS_GEO_CUDA` is enabled, otherwise the HIP source when `THEMIS_GEO_HIP`
is enabled, and otherwise the CPU dispatcher source. The stub and production
backend wrappers are also listed. The repository CMake configuration makes
CUDA take precedence over HIP. A compiled GPU path does not establish that a
device is available, nor that every geo API uses that path.

The public `GeoBackendDispatcher::computeVincentyBatch()` currently computes
on the CPU; its implementation documents the missing GPU dispatch integration.
Do not describe the separate CUDA kernel or `GpuKernelDispatcher` formula
parameter as evidence that this method executes Vincenty on a GPU.

## Integration and failure boundaries

- `src/server/spatial_api_handler.cpp` is a
  server integration point. Authentication and authorization remain at that
  service boundary; `SpatialQueryPolicy::allowed_collections` is advisory.
- `GeoErrorCode` is declared in
  `include/geo/geo_api_contract.h`; concrete
  methods may also use result/status types or empty results. Consult the method
  declaration rather than assuming all failures use one return shape.
- `NoOpRasterQueryImpl` reports `RasterStatus::NOT_SUPPORTED`. The raster
  interface names `THEMIS_ENABLE_RASTER`, but a corresponding repository CMake
  option was not found in the geo feature configuration; see
  `include/geo/README.md` (TODO).
- Contract constants and helper validation are not proof that every request
  path enforces every limit. Trace the concrete call path before treating a
  declared limit as a deployment-wide resource boundary.

## Source references

- Build selection: `src/geo/CMakeLists.txt`,
  `cmake/features/GeoFeatures.cmake`, and `cmake/CMakeLists.txt`.
- Public interfaces: `include/geo/`.
- Focused test registration: `tests/geo/CMakeLists.txt`.

---

**AI_Used:** true · **AI_Models:** unknown · **AI_Contributor:** GitHub Copilot coding agent · **Human_Review:** false · **Human_Author:** ThemisDB Contributors
Exact model backend not verifiable from final document text alone.