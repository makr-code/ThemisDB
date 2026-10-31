---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
Source_Level: "L1"
SOT_Domain: "module-behavior"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Geo research and evaluation notes

## Purpose and scope

This note helps geo developers choose and evaluate spatial methods. It records
the assumptions and evidence present in the implementation; it is not a claim
that every method has a production benchmark or equivalent accuracy.
The [API contract](./API.md), [data model](./MODELING.md), and
[performance expectations](./PERFORMANCE_EXPECTATIONS.md) remain the relevant
references for contracts and benchmark targets.

## Methods and assumptions

| Workload | Implemented method | Decision-relevant limit |
|---|---|---|
| Point distance | `haversineDistanceM` uses a spherical mean-Earth-radius model; the geo math header documents approximate error up to about 0.5% over global distances. | Use the Vincenty path when ellipsoidal distance is required; the CPU backend falls back to Haversine for the near-antipodal non-convergence case. |
| Point in polygon | CPU ray casting evaluates a planar ring. The helper is documented for small/projected polygons or geographic polygons not spanning a hemisphere. | It is not a global geodesic containment model; callers must select an appropriate CRS and geometry extent. |
| Spatial filtering | MBR/bounding-box tests cheaply select candidates; exact predicates are a separate refinement step where the operation uses them. | An MBR overlap is not proof of geometry intersection. Approximate mode is suitable as a prefilter, not as a substitute for an exact result. |
| DBSCAN | Haversine neighborhood distances; the CPU neighborhood search has quadratic worst-case work. | CUDA can accelerate distance computation only within the configured DBSCAN point limit; cluster expansion remains on CPU. |
| k-means | Haversine point-to-centroid distance with arithmetic longitude/latitude centroid updates. | The header limits this approximation to clusters spanning less than a few hundred kilometres; centroid updates are not a spherical mean. |
| Geographic k-NN | Geographic points are projected to 3D unit-sphere coordinates and searched with Euclidean L2 distance. | Returned distances are approximate spherical distances; this is a candidate-ranking bridge, not an ellipsoidal exact-distance index. |

These method descriptions are per API: coordinate order and distance units are
not uniform across all entry points. Check the declarations linked from
[`API.md`](./API.md) before comparing results.

## Evaluation evidence

- [`test_geo_contract_hardening_focused.cpp`](../../tests/geo/test_geo_contract_hardening_focused.cpp)
  documents GCH-01..GCH-16. Its validation, backend, index, and join fixtures
  are deliberately self-contained mocks; passing it verifies the stated
  contract fixture, not every production implementation.
- Concrete behavior has separate suites under
  [`tests/geo`](../../tests/geo/), including geometry, clustering, index,
  spatial-join, raster, temporal-query, and backend tests. Select the suite
  matching the method under review.
- [`bench_geo_release_gates.cpp`](../../benchmarks/geo/bench_geo_release_gates.cpp)
  defines seeded CPU-only mock cases and p99 limits for six hot paths. These
  limits are release targets, not a measurement report or evidence of GPU
  performance. Compare actual release-profile runs with the matching baseline
  before making a performance claim.
- [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) maps broader
  CPU/GPU, index, join, clustering, and parsing benchmark cases to release
  expectations. The [roadmap](./ROADMAP.md) records the current evidence gap
  for a fresh focused build/test run in a dependency-complete environment.

## Reproducible evaluation

1. Choose one method and its concrete implementation test under `tests/geo`;
   keep the contract-only mock suite separate from implementation evidence.
2. Record coordinate order, CRS, units, geometry extent, precision mode, and
   selected backend with the test input.
3. For latency claims, run the mapped benchmark in the release profile on
   representative hardware, retain the run manifest, and compare the same
   benchmark case against its baseline.
4. For CPU/GPU parity, use identical fixtures and report the tolerance and
   whether the GPU path actually executed. A fallback result is CPU evidence.

## Trade-offs and open evidence

- Spherical distance is inexpensive and predictable but does not model the
  WGS-84 ellipsoid.
- A planar point-in-polygon algorithm is not suitable for arbitrary global
  geometry without a compatible projection/extent.
- ECEF/FAISS search favors fast nearest-neighbor candidate lookup; its
  spherical/float representation is not a replacement for an exact
  ellipsoidal distance check.
- The module roadmap still tracks mixed-backend precision permutations,
  advanced-query regressions, benchmark breadth, and fresh execution evidence.
  Treat these as open work until the cited code, tests, and current roadmap
  show otherwise.

## Sourcecode verification

- Method and units: [`geo_math.h`](../../include/geo/geo_math.h),
  [`spatial_backend.h`](../../include/geo/spatial_backend.h),
  [`geo_clustering.h`](../../include/geo/geo_clustering.h),
  [`geo_faiss_knn.cpp`](./geo_faiss_knn.cpp).
- Test and benchmark scope: the GCH test and release-gate benchmark linked
  above; planning/evidence status: [`ROADMAP.md`](./ROADMAP.md).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
