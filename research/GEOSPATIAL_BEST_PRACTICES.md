# Geospatial Best Practices for ThemisDB (Research Review)

## Abstract

This reviewed article aligns geospatial best practices with the current ThemisDB state using repository-verifiable evidence only. It summarizes what is implemented, what is measurable in existing benchmark artifacts, and which next steps are realistic without over-claiming. All core claims are tied to concrete code paths, documentation, or benchmark sources.

## Introduction

Geospatial support in ThemisDB spans multiple layers:

- **AQL query layer** (geospatial functions and query translation)
- **Storage/index layer** (R-tree + Morton, sidecar-index hooks)
- **Content ingestion layer** (GDAL/OGR-based parsers for vector/raster formats)

To keep terminology consistent with the current codebase and root documentation:

- **AQL** refers to ThemisDB's query language interfaces and related optimizer/translator components.
- **Multi-Model** refers to ThemisDB's combined relational, graph, vector, document, geospatial, and time-series architecture.
- **Consistency Model** refers to the ACID-oriented model documented in the project README (MVCC, SSI, 2PC, SAGA orchestration, HLC ordering in distributed settings).

Primary objective: provide a review-ready, evidence-backed baseline for geospatial engineering decisions and future roadmap work.

## Methodology

The review used three evidence classes:

Repository state for this review:

- Review date: 2026-05-14
- Working tree basis: `makr-code/ThemisDB` (local checkout state at review time)

1. **Code verification** (authoritative for implementation state)
   - `../include/index/spatial_index.h`
   - `../include/query/functions/geo_functions.h`
   - `../src/content/geo_processor.cpp`
2. **Product documentation** (authoritative for public architecture/feature language)
   - `../README.md`
   - `../docs/features/geo_gdal_integration.md`
3. **Benchmark artifacts** (authoritative for published performance claims)
   - `../docs/de/geo/geo_benchmarks.md`

Validation criteria:

- Remove or qualify statements that cannot be tied to code or benchmark artifacts.
- Avoid speculative "will definitely" claims for unimplemented features.
- Keep comparisons to external systems at the level of referenced literature and ThemisDB's own measured artifacts.
- Verification method: symbol/path inspection in source headers and implementations, cross-reference checks against product docs, and markdown/link validation of this review document.
- Path convention: all repository references are relative to this file (`research/`) and were validated via internal link checking.

## Current ThemisDB Geospatial Baseline (Code-Verified)

### 1) Query and function surface (AQL)

The geospatial function surface includes OGC-style `ST_*` and Arango-style `GEO_*` patterns, including distance and predicate operations (`ST_DISTANCE`, `ST_INTERSECTS`, `ST_WITHIN`, `GEO_DISTANCE`, `GEO_CONTAINS`) in:

- `../include/query/functions/geo_functions.h`

### 2) Spatial index primitives and operations

The index layer provides:

- **MortonEncoder** (`encode2D`, `encode3D`, range generation)
- **RTreeConfig** and model-agnostic **SpatialIndexManager**
- **Bulk load** API (`SpatialIndexManager::bulkLoad`) and metrics instrumentation

Source:

- `../include/index/spatial_index.h`

### 3) GDAL/OGR ingestion path

The geospatial content processor (`GeoProcessor`) supports GeoJSON/KML/GPX/Shapefile/GeoPackage/GeoTIFF ingestion, with:

- `/vsimem/` loading via `VSIFileFromMemBuffer`
- optional bounding-box prefilter via `SetSpatialFilterRect`
- CRS/projection extraction

Source:

- `../src/content/geo_processor.cpp`
- `../include/content/geo_processor.h`

### 4) Multi-Model and consistency context

Geospatial functionality exists within ThemisDB's documented multi-model and ACID-oriented system positioning.

Source:

- `../README.md`

## Evaluation

### 1) Claim-to-evidence matrix

| Claim | Status | Evidence |
|---|---|---|
| ThemisDB supports geospatial query functions in AQL. | **Verified** | [../include/query/functions/geo_functions.h](../include/query/functions/geo_functions.h) |
| ThemisDB provides Morton + R-tree based index support. | **Verified** | [../include/index/spatial_index.h](../include/index/spatial_index.h) |
| Bulk loading for spatial index exists in the public API. | **Verified** | [../include/index/spatial_index.h](../include/index/spatial_index.h) (`SpatialIndexManager::bulkLoad`) |
| GDAL parsing uses in-memory `/vsimem/` paths. | **Verified** | [../src/content/geo_processor.cpp](../src/content/geo_processor.cpp) (`VSIFileFromMemBuffer`, `VSIUnlink`) |
| Optional spatial prefiltering exists in ingestion. | **Verified** | [../src/content/geo_processor.cpp](../src/content/geo_processor.cpp) (`SetSpatialFilterRect`) |
| ThemisDB benchmark document reports CPU/GPU geo scenarios in its published tables. | **Verified (artifact-level)** | [../docs/de/geo/geo_benchmarks.md](../docs/de/geo/geo_benchmarks.md) |

### 2) Performance interpretation (artifact-backed)

The repository benchmark artifact (`../docs/de/geo/geo_benchmarks.md`) reports scenario-specific runtimes for point-in-polygon, K-NN, radius search, and spatial joins. These numbers can be used as **published internal measurements**, but should be interpreted with the hardware/setup constraints stated in that document.

Note: the current benchmark artifact is maintained in German (`docs/de/...`); this is intentional because it is the canonical benchmark source currently available in the repository.

For this reason, this review does **not** generalize those numbers into universal speedup guarantees. Instead, they should be used as baseline inputs for reproducible benchmark pipelines in CI or controlled lab runs.

### 3) External best-practice alignment

Current implementation choices are broadly aligned with established geospatial DB practices:

- Two-stage filtering concepts (index candidate filtering + exact checks)
- R-tree family indexing as a practical baseline for spatial workloads
- Optional simplification and CRS-aware processing in ingestion pipelines

However, implementation maturity differs across features. Example: core ingestion + index primitives are code-verified today, while roadmap targets such as R*-tree migration and broader operational hardening remain planning work. Therefore, each future enhancement must be tied to explicit acceptance criteria and reproducible benchmarks.

## Recommended Next Steps (Evidence-Driven)

1. **Benchmark reproducibility first**
   - Convert selected scenarios from `../docs/de/geo/geo_benchmarks.md` into repeatable benchmark automation with versioned datasets and fixed hardware notes.
2. **Index strategy evolution with measurable gates**
   - Evaluate R*-tree-style improvements only behind explicit metrics (query latency percentiles, memory overhead, build/insert costs).
3. **CRS/transform operational hardening**
   - Standardize error reporting and fallback behavior for invalid CRS input across query and ingestion paths.
4. **Documentation hygiene**
   - Keep geospatial architecture, benchmark docs, and enhancement plans synchronized.
   - Canonical enhancement planning reference: `../docs/GEOSPATIAL_FUTURE_ENHANCEMENTS.md`.

## Limitations / Known Issues

- This review validates against the **current repository snapshot**, not against external deployment telemetry.
- Some benchmark results are documented artifacts, but not yet represented as an always-on CI benchmark suite.
- Existing geospatial docs in the repository are split across English and German pages; readers should treat benchmark and architecture pages as complementary sources.
- External system comparisons (e.g., PostGIS, MongoDB) are informative but environment-sensitive; direct parity claims require controlled, reproducible methodology.

## Conclusion

ThemisDB's geospatial stack is not a greenfield prototype; it already contains meaningful production-oriented building blocks across ingestion, indexing, and query execution. The strongest immediate value now is disciplined evidence handling: keep claims tied to code and benchmark artifacts, and prioritize reproducible evaluation over headline speedup statements.

## References

### Internal ThemisDB sources

1. ThemisDB README (system scope, multi-model + ACID context):
   [../README.md](../README.md)
2. Geospatial query functions (`ST_*`, `GEO_*`):
   [../include/query/functions/geo_functions.h](../include/query/functions/geo_functions.h)
3. Spatial index manager (`MortonEncoder`, `RTreeConfig`, `bulkLoad`):
   [../include/index/spatial_index.h](../include/index/spatial_index.h)
4. GDAL geo processor (`/vsimem/`, spatial filter integration):
   [../src/content/geo_processor.cpp](../src/content/geo_processor.cpp)
5. Geospatial benchmark artifact (documented scenarios/results):
   [../docs/de/geo/geo_benchmarks.md](../docs/de/geo/geo_benchmarks.md)
6. Geospatial GDAL integration overview:
   [../docs/features/geo_gdal_integration.md](../docs/features/geo_gdal_integration.md)

### External standards and literature

7. Open Geospatial Consortium. *OpenGIS Simple Features Access*.
   https://www.ogc.org/standards/sfa
8. Butler, H. et al. (2016). *The GeoJSON Format* (RFC 7946).
   https://datatracker.ietf.org/doc/html/rfc7946
9. Guttman, A. (1984). *R-trees: A Dynamic Index Structure for Spatial Searching*. SIGMOD.
    DOI: https://doi.org/10.1145/602259.602266
10. Beckmann, N. et al. (1990). "The R*-tree: An Efficient and Robust Access Method for Points and Rectangles". SIGMOD.
    DOI: https://doi.org/10.1145/93597.98741
11. Jacox, E., Samet, H. (2007). *Spatial Join Techniques*. ACM TODS.
    DOI: https://doi.org/10.1145/1272743.1272747
12. GDAL/OGR official documentation.
    https://gdal.org/

### Planning document (non-evidence roadmap input)

13. Geospatial future enhancements roadmap:
    [../docs/GEOSPATIAL_FUTURE_ENHANCEMENTS.md](../docs/GEOSPATIAL_FUTURE_ENHANCEMENTS.md)
