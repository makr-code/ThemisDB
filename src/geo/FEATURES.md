---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Geo module capabilities

## Role

The geo module provides geospatial geometry, compute, indexing, and query
components. It is a library within the parent ThemisDB build, not a standalone
server. Public entry points are declared under
`include/geo/`.
These capability notes describe the integrated source implementation; the
optional external Geo plugin path was not audited for feature parity.

## Why this matters

These components support geospatial workloads within ThemisDB while keeping
indexing, geometry operations, and server authorization at their respective
module boundaries.

## Implemented surfaces

- Typed GeoJSON geometries and EWKB/WKT/GeoJSON helpers.
- CPU spatial backends, optional CUDA/HIP build paths, and an in-memory
  bounding-box R-tree with cursor access.
- Spatial joins and composable join filters, DBSCAN/k-means, raster query
  interfaces, temporal-spatial query helpers, and tile coordinate/vector-tile
  helpers.
- Geographic point k-NN via `GeoFaissKnn`, using a 3D unit-sphere projection.

Availability and semantics differ by API and build. For example, raster
implementations can report `NOT_SUPPORTED`; the geo build selects CUDA, HIP, or
CPU dispatch; and `GeoBackendDispatcher::computeVincentyBatch()` currently uses
its CPU implementation. A CUDA/HIP build is not proof that a device-backed
operation ran.

## Caller-facing limits

Coordinate order and units are method-specific. GeoJSON uses
`[longitude, latitude]`, while some compute APIs accept latitude first; some
distance methods return metres and the batch dispatcher returns kilometres.
Approximate MBR filtering may include false positives. Check the API and model
guides before using results as exact predicates or deployment-wide guarantees.

## References

- [Public contract and interface map](API.md).
- [Component boundaries](ARCHITECTURE.md).
- Build and focused-test guidance:
  `IMPLEMENTATION.md`.
- Known implementation limits and evidence:
  `RESEARCH.md` and `PERFORMANCE_EXPECTATIONS.md`.

Exact model backend not verifiable from final document text alone.
