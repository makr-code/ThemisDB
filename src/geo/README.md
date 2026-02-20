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
- [GPU Backend Runbook](../../docs/gpu_runbooks.md#6-gpu-geospatial-backend-issues) — device discovery, circuit-breaker fallback, metrics, audit log, on-call procedures
- [GPU Roadmap](../../docs/gpu_roadmap.md) — production readiness status, completed geo backend work, remaining CUDA/ROCm items
- [Future Enhancements](../../docs/geospatial_future_enhancements.md) — planned geospatial features (full GeoJSON parsing, ST_BUFFER, GPU kernel dispatch)
