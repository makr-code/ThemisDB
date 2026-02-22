# Geospatial Module

Geospatial query processing and indexing implementation for ThemisDB.

## Module Purpose

Implements geospatial query processing and spatial indexing for ThemisDB, providing 2D/3D spatial queries, geometry operations, and GPU-accelerated spatial processing with CPU fallback.

## Subsystem Scope

**In scope:** CPU-based spatial query processing (contains, intersects, distance), S2/H3 cell indexing, GPU-accelerated backend with CPU fallback, 2D and 3D geometry operations.

**Out of scope:** Full GeoJSON RFC 7946 parsing (planned), R-tree index construction (planned), routing/navigation algorithms.

## Relevant Interfaces

- `cpu_backend.cpp` — primary CPU-based geospatial backend
- `boost_cpu_exact_backend.cpp` — Boost.Geometry exact computation
- `gpu_backend_stub.cpp` — GPU stub with CPU fallback

## Current Delivery Status

**Maturity:** 🟡 Beta — CPU geospatial backend operational; GPU backend is stub pending CUDA kernel implementation.

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

## Scientific References

1. Guttman, A. (1984). **R-Trees: A Dynamic Index Structure for Spatial Searching**. *Proceedings of the 1984 ACM SIGMOD International Conference on Management of Data*, 47–57. https://doi.org/10.1145/602259.602266

2. Haverkort, H. J., & van Walderveen, F. (2008). **Locality and Bounding-Box Quality of Two-Dimensional Space-Filling Curves**. *Computational Geometry*, 42(5), 341–352. https://doi.org/10.1016/j.comgeo.2008.09.007

3. Open Geospatial Consortium. (2010). **OpenGIS® Implementation Standard for Geographic Information – Simple Feature Access – Part 1: Common Architecture (Version 1.2.1)**. OGC 06-103r4. https://www.ogc.org/standards/sfa

4. Haran, M. (2011). **Gaussian Random Field Models for Spatial Data**. *Handbook of Markov Chain Monte Carlo*, 449–478. https://doi.org/10.1201/b10905-19

5. Brinkhoff, T., Kriegel, H.-P., & Seeger, B. (1993). **Efficient Processing of Spatial Joins Using R-Trees**. *Proceedings of the 1993 ACM SIGMOD International Conference on Management of Data*, 237–246. https://doi.org/10.1145/170035.170075
