<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/geo/ROADMAP.md -->

# Roadmap — Geo Module (Public Headers)

> Implementation roadmap: `../../src/geo/ROADMAP.md`

## Current Status

v2.2.0 — Production-ready. 10 public headers covering spatial indexing, GPU dispatch, clustering, raster, and temporal-spatial queries.

## Completed ✅

- [x] R-tree spatial index (`geo_rtree.h`)
- [x] Pluggable spatial backend abstraction (`spatial_backend.h`)
- [x] Streaming spatial join (`spatial_join.h`)
- [x] GPU kernel dispatcher (`gpu_kernel_dispatcher.h`)
- [x] Device auto-detection (`device_detector.h`)
- [x] Raster grid and heatmap (`raster.h`)
- [x] Temporal-spatial queries (`temporal_spatial_query.h`)

## Planned

- [ ] Spherical geometry (WGS-84 ellipsoid) in `spatial_backend.h` — Issue #1744 (Target: v2.3.0)
- [ ] GPU-accelerated DBSCAN / k-means in `geo_clustering.h` (Target: v2.3.0)
- [ ] CUDA kernels for `ST_BUFFER`, `ST_UNION`, `ST_DIFFERENCE` in `gpu_kernel_dispatcher.h` (Target: v2.2.0)

## Implementation Phases

### Phase 1: Core Spatial (Complete ✅)
- [x] R-tree index, spatial backend, spatial join, clustering

### Phase 2: GPU Acceleration (Complete ✅)
- [x] GPU kernel dispatcher, device detector

### Phase 3: Extended Operations (In Progress 🚧)
- [ ] Spherical geometry, GPU clustering, GPU overlay operations

## Production Readiness Checklist

- [x] 10 public headers compile cleanly
- [x] 19 CI test targets pass
- [x] GPU/CPU backend parity tested
- [ ] Spherical geometry WGS-84 support
- [ ] GPU DBSCAN/k-means
