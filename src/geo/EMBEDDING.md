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

# Geographic point projection for k-NN

## Purpose and scope

The geo module has no learned text or image embedding model. This page
documents the coordinate projection used by `GeoFaissKnn` to make geographic
point-neighbor lookup compatible with a 3D Euclidean index. It is an
implementation note, not a claim about semantic vector search.

## Projection and distance

[`geo_faiss_knn.cpp`](./geo_faiss_knn.cpp) converts each point's longitude and
latitude in decimal degrees to a three-component unit-sphere vector:

```text
x = cos(latitude) cos(longitude)
y = cos(latitude) sin(longitude)
z = sin(latitude)
```

Coordinates are stored as `float` values and indexed in dimension 3. With
FAISS available the module uses `IndexFlatL2` on CPU or `GpuIndexFlatL2` on
CUDA; without FAISS it keeps the projected points and performs a brute-force
CPU search. GPU index construction failure also falls back to CPU FAISS when
that path was compiled. The `force_cpu` configuration disables the attempted
CUDA index path.

The Euclidean chord distance on the unit sphere is converted to a great-circle
arc distance using a mean spherical radius of 6,371,000 metres. Results are
therefore approximate spherical distances, not WGS-84 ellipsoidal geodesics.
No elevation/Z component participates in the projection.

## Input and result semantics

- `build()` indexes only non-empty `Point` geometries. Other geometry types
  and empty points are skipped; `false` is returned if no points remain.
- The internal mapping preserves each result's index into the original input
  vector, even when non-points were skipped.
- `knnSearch()` returns no results when the index is unbuilt or the query is
  not a non-empty point. It caps `k` at the indexed point count.
- `radiusSearch()` returns no results for an unbuilt index or a non-positive
  radius. `max_results == 0` means no caller-specified result cap.
- `getBackendName()` distinguishes `not_built`, `faiss_gpu`, and `faiss_cpu`;
  the last name also covers the no-FAISS brute-force implementation, so it
  does not prove that FAISS itself is present.

The API's `GeoKnnResult::dist_m` is a spherical estimate suitable for ranking
nearby points. Re-check candidates with the appropriate exact/geodesic method
when the application requires ellipsoidal-distance guarantees.

## Evaluation and limitations

No `GeoFaissKnn`-specific test or benchmark is currently present under
[`tests/geo`](../../tests/geo/) or
[`benchmarks/geo`](../../benchmarks/geo/). Do not report its behavior as
covered by the general vector/geo or contract suites. A focused test can be
added as `tests/geo/test_geo_faiss_knn.cpp`; the existing module CMake glob
will register it without a separate per-test CMake entry.

Use this deterministic fixture in that test:

1. Build a dataset containing a LineString at input index 0 followed by Point
   geometries at `(1, 0)`, `(0, 0)`, and `(2, 0)` in `[longitude, latitude]`
   order.
2. Query with a Point at `(0, 0)` and `k = 3`. The expected original input
   indices are `[2, 1, 3]`, ordered by increasing distance; the first distance
   is zero. Compare the nonzero distances with `haversineDistanceM`; require
   an absolute delta of at most 1 metre for these fixed 1-degree and 2-degree
   equatorial pairs. This is a fixture regression bound, not a global accuracy
   guarantee.
3. Check `build()`/`size()`, `k = 0`, `k` greater than the point count,
   non-point and empty queries, non-positive radius, and radius-result caps.
4. Repeat with `force_cpu`; when the build provides FAISS/CUDA, run the GPU
   case separately and assert the selected backend before treating it as GPU
   evidence. Compare both result ordering and distance deltas against the same
   CPU reference.

The relevant public declaration is
[`geo_faiss_knn.h`](../../include/geo/geo_faiss_knn.h). Until the focused
suite or benchmark exists and is run, the above is a concrete validation
specification, not a claim of current automated coverage.

The projection preserves spherical angular ordering in exact arithmetic, but
float quantization, Earth-sphere approximation, and backend differences can
affect reported distances. Evaluate ordering agreement and distance deviation
against the Haversine reference across the deployment's latitude range and
point density; measure build/query latency at representative point counts.
The repository's geo performance expectations map hybrid vector/geo and
CPU/GPU cases, but do not currently define a `GeoFaissKnn` accuracy or latency
target. Do not use the approximate result as a final ellipsoidal-geodesic
predicate.

## Sourcecode verification

- Projection, backend selection, and result mapping:
  [`geo_faiss_knn.cpp`](./geo_faiss_knn.cpp).
- Public inputs/results: [`geo_faiss_knn.h`](../../include/geo/geo_faiss_knn.h).
- Related distance contract: [`geo_math.h`](../../include/geo/geo_math.h)
  and [`spatial_backend.h`](../../include/geo/spatial_backend.h).

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
