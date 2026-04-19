# Geo Module Headers

This directory contains header files (.h) for the geo module.

## Purpose

Public interfaces and declarations for geo functionality. Implementation source files are in `../../src/geo/`.

## Header Files

| Header | Description |
|--------|-------------|
| `spatial_backend.h` | `ISpatialComputeBackend` interface, `IGeoRegistry` plugin registry, `GeoPrecisionMode` enum, `getBackendForPrecision()` factory |
| `geo_ops_ext.h` | `IGeoOpsExtension` — optional geometry operation extensions (ST_BUFFER, ST_UNION, ST_DIFFERENCE) |
| `geo_rtree.h` | `GeoRtree` — R-tree spatial index for 2D bounding-box queries; lazy build, read-write lock |
| `gpu_kernel_dispatcher.h` | `GpuKernelDispatcher` — dispatch table for CUDA/HIP geo kernels; manages host↔device memory |
| `spatial_join.h` | `SpatialJoin` — all-pairs spatial join within a configurable distance threshold; `haversineDistanceM()` helper |
| `geo_clustering.h` | `dbscanCluster()`, `kmeansCluster()` — density-based and centroid-based geo-point clustering |
| `raster.h` | `RasterGrid`, `sampleAt()`, `queryBBox()`, `generateHeatmap()` — raster data query abstraction |
| `temporal_spatial_query.h` | `TemporalSpatialQuery` — location-at-time-T and entities-within-distance-at-time-T queries |
| `tile_server.h` | `TileServer` — map tile server integration (tile request routing, cache) |
| `device_detector.h` | `DeviceDetector` — runtime GPU device discovery, compute-capability and VRAM reporting |

## Documentation

- `../../src/geo/README.md` — module overview and feature list
- `../../src/geo/ARCHITECTURE.md` — component architecture, data flow, threading model
- `../../src/geo/ROADMAP.md` — module roadmap and implementation phases
- `../../src/geo/FUTURE_ENHANCEMENTS.md` — planned enhancements with scientific references
- `../../docs/de/geo/` — German-language developer documentation

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "geo/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
