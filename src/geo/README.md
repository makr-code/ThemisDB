# Geospatial Module

Geospatial query processing and indexing implementation for ThemisDB.

## Components

- CPU-based geospatial backend (`cpu_backend.cpp`, `boost_cpu_exact_backend.cpp`)
- GPU-accelerated geospatial backend with CPU fallback (`gpu_backend_stub.cpp`)
- Spatial index structures
- Geo query processors

## Features

- 2D and 3D spatial queries
- Geometry operations (contains, intersects, distance)
- S2/H3 cell indexing support
- GPU acceleration for large-scale queries (CPU fallback when no device present)

## Documentation

For geospatial documentation, see:
- [CPU Backend](../../docs/src/geo/cpu_backend.cpp.md)
- [GPU Backend](../../docs/src/geo/gpu_backend_stub.cpp.md) — device discovery, circuit-breaker fallback, metrics, audit log
- [Geo Architecture](../../docs/GEO_ARCHITECTURE.md)
- [Geo Integration](../../docs/geo_integration_readme.md)
- [Geo Feature Tiering](../../docs/geo_feature_tiering.md)
