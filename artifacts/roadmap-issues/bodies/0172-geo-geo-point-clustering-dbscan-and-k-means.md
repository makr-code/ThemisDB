### Context

This issue implements the roadmap item 'Geo Point Clustering: DBSCAN and k-means' for the geo domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: Geo Point Clustering: DBSCAN and k-means

### Goal

Deliver the scoped changes for Geo Point Clustering: DBSCAN and k-means in src/geo/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### Geo Point Clustering: DBSCAN and k-means
**Priority:** Medium
**Target Version:** v2.4.0
**Status:** ✅ Implemented in `include/geo/geo_clustering.h` + `src/geo/geo_clustering.cpp`

**What was implemented:**
- `dbscanCluster(points, DbscanConfig)` — density-based spatial clustering:
  - Haversine distance for all pairwise neighbour queries
  - Noise points receive label `kDbscanNoise` (-1)
  - Non-Point geometries are silently assigned noise label
  - O(n²) complexity; suitable for collections up to ~50 000 points
- `kmeansCluster(points, KMeansConfig)` — Lloyd's algorithm:
  - Deterministic initialisation (first k distinct points, `seed == 0`) or
    k-means++ probabilistic seeding (`seed != 0`, LCG PRNG)
  - Centroid updates via arithmetic mean of (lon, lat) — valid for clusters
    spanning < a few hundred kilometres
  - Convergence check: stops early when all centroid shifts ≤ `tolerance_m`
  - Non-Point geometries receive label -1 and are excluded from centroid
    computation
  - Throws `std::invalid_argument` when k == 0 or k > valid point count
- 20 unit tests in `tests/geo/test_geo_clustering.cpp`

**Performance Targets (design):**
- DBSCAN: 10 000 points at 500 m epsilon in ≤ 5 s single-threaded (CPU).
- k-means: k=10, 100 000 points, 100 iterations in ≤ 2 s single-threaded (CPU).

**Scientific References:**
- [1] Ester, M., Kriegel, H.-P., Sander, J., & Xu, X. (1996). A Density-Based Algorithm for Discovering Clusters in Large Spatial Databases with Noise. *Proceedings of the 2nd International Conference on Knowledge Discovery and Data Mining (KDD-96)*, 226–231. https://dl.acm.org/doi/10.5555/3001460.3001507
- [2] Lloyd, S. P. (1982). Least Squares Quantization in PCM. *IEEE Transactions on Information Theory*, 28(2), 129–137. https://doi.org/10.1109/TIT.1982.1056489
- [3] Arthur, D., & Vassilvitskii, S. (2007). k-means++: The Advantages of Careful Seeding. *Proceedings of the 18th Annual ACM-SIAM Symposium on Discrete Algorithms (SODA '07)*, 1027–1035. https://dl.acm.org/doi/10.5555/1283383.1283494

### Acceptance Criteria

- [ ] `dbscanCluster(points, DbscanConfig)` — density-based spatial clustering:
- [ ] Haversine distance for all pairwise neighbour queries
- [ ] Noise points receive label `kDbscanNoise` (-1)
- [ ] Non-Point geometries are silently assigned noise label
- [ ] O(n²) complexity; suitable for collections up to ~50 000 points
- [ ] `kmeansCluster(points, KMeansConfig)` — Lloyd's algorithm:
- [ ] Deterministic initialisation (first k distinct points, `seed == 0`) or
- [ ] Centroid updates via arithmetic mean of (lon, lat) — valid for clusters
- [ ] Convergence check: stops early when all centroid shifts ≤ `tolerance_m`
- [ ] Non-Point geometries receive label -1 and are excluded from centroid
- [ ] Throws `std::invalid_argument` when k == 0 or k > valid point count
- [ ] 20 unit tests in `tests/geo/test_geo_clustering.cpp`
- [ ] DBSCAN: 10 000 points at 500 m epsilon in ≤ 5 s single-threaded (CPU).
- [ ] k-means: k=10, 100 000 points, 100 iterations in ≤ 2 s single-threaded (CPU).
- [ ] [1] Ester, M., Kriegel, H.-P., Sander, J., & Xu, X. (1996). A Density-Based Algorithm for Discovering Clusters in Large Spatial Databases with Noise. *Proceedings of the 2nd International Conference on Knowledge Discovery and Data Mining (KDD-96)*, 226–231. https://dl.acm.org/doi/10.5555/3001460.3001507
- [ ] [2] Lloyd, S. P. (1982). Least Squares Quantization in PCM. *IEEE Transactions on Information Theory*, 28(2), 129–137. https://doi.org/10.1109/TIT.1982.1056489
- [ ] [3] Arthur, D., & Vassilvitskii, S. (2007). k-means++: The Advantages of Careful Seeding. *Proceedings of the 18th Annual ACM-SIAM Symposium on Discrete Algorithms (SODA '07)*, 1027–1035. https://dl.acm.org/doi/10.5555/1283383.1283494

### Relationships

- Roadmap row: #172 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/geo/FUTURE_ENHANCEMENTS.md#geo-point-clustering-dbscan-and-k-means
- Source key: roadmap:172:geo:v1.8.0:geo-point-clustering-dbscan-and-k-means

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:172:geo:v1.8.0:geo-point-clustering-dbscan-and-k-means -->
<!-- roadmap-ref: row=172;module=geo;target=v1.8.0 -->
<!-- roadmap-detail: src/geo/FUTURE_ENHANCEMENTS.md#geo-point-clustering-dbscan-and-k-means -->
