# Geo public-header architecture

## Scope

`include/geo/` declares geo-facing types and
interfaces. The implementation and module build selection are in
`src/geo/`. The public headers do not imply
that all geo operations share one execution pipeline or are available in every
build.

## Header boundaries

| Responsibility | Representative headers |
|---|---|
| Contracts and policy declarations | `geo_api_contract.h`, `geo_policy.h` |
| Geometry, coordinates and math | `geo_json_geometry.h`, `geo_math.h`, plus shared `include/utils/geo/ewkb.h` |
| Compute dispatch | `spatial_backend.h`, `geo_backend_dispatch.h`, `gpu_kernel_dispatcher.h`, `device_detector.h` |
| Indexing and joins | `geo_rtree.h`, `rtree_cursor.h`, `spatial_join.h`, `spatial_join_filter.h` |
| Additional geo operations | `geo_clustering.h`, `geo_faiss_knn.h`, `raster.h`, `raster_query_interface.h`, `temporal_spatial_query.h`, `temporal_spatial_query_builder.h`, `tile_server.h` |
| Supporting controls | `geo_edge_case_handler.h`, `geo_operator_diagnostics.h`, `gpu_buffer_guard.h`, `phase2_phase3_hardening.h`, `geo_ops_ext.h` |

`IGeoOpsExtension` declares an extension name/support interface; it does not
declare the `ST_*` operation implementations. `ISpatialComputeBackend` is a
compute abstraction, not a persistent spatial-index abstraction.

## Build and runtime boundaries

The default integrated module CMake selection is in
`src/geo/CMakeLists.txt`: CUDA, HIP, or a
CPU dispatcher source is selected at build time, with CUDA taking precedence
in the repository configuration. Backend availability and actual execution are
runtime facts and must be checked for the specific operation.

The parent build can select the optional external Geo plugin with
`THEMIS_EXTERNALIZE_GEO_PLUGIN=ON` when its feature and configured submodule
are available. In that mode the integrated `src/geo/` implementation is not
the active implementation; plugin API/test parity was not audited here.

The geo headers declare both status-returning interfaces and APIs with other
result/error shapes. For example, raster queries use `RasterStatus`, while the
shared `GeoErrorCode` is declared in `geo_api_contract.h`. Do not infer a
uniform error model or a common limit-enforcement path from the header set.

## Integration boundary

`SpatialQueryPolicy::allowed_collections` is advisory; authorization belongs
to the server boundary. The policy type is a declaration consumed by callers
and does not itself enforce resource limits. Likewise, the raster header names
`THEMIS_ENABLE_RASTER`, but the inspected geo feature CMake file does not
declare a matching user-facing option (**TODO:** verify the owning build path).

## Canonical sources

- Binding API contract: `include/geo/geo_api_contract.h`.
- Geo implementation architecture:
  `src/geo/ARCHITECTURE.md`.
- Build selection:
  `src/geo/CMakeLists.txt` and `cmake/features/GeoFeatures.cmake`.

---

**AI_Used:** true · **AI_Models:** unknown · **AI_Contributor:** GitHub Copilot coding agent · **Human_Review:** false · **Human_Author:** ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
