# ArcGIS Pro and ThemisDB Geospatial Capabilities: Emission-Protection Workflow Assessment

**Status:** Review-ready research note (codebase-aligned)
**Last Updated:** 2026-05-14
**Evidence Base:** See Methodology for the source-backed artifact set used in this assessment.

---

## Abstract

This review provides a repository-grounded assessment of how ThemisDB currently relates to ArcGIS Pro in emission-protection workflows. The central result is that ThemisDB already implements substantial geospatial database functionality: the AQL function registry exposes core constructors, measurements, predicates, `ST_BUFFER`, `ST_UNION`, `ST_INTERSECTION`, GeoJSON/WKT export, and CRS transformation via `ST_TRANSFORM`.

At the same time, the codebase does **not** justify describing ThemisDB as a replacement for ArcGIS Pro's analyst toolboxes. The repository supports the database side of environmental workflows well—storage, indexing, spatial filtering, proximity checks, buffering, CRS normalization, raster query building blocks, and spatial join primitives—but it does not provide **open-repository, code-verified** viewshed analysis, kriging, dispersion modeling, network/service-area analysis, or OGC WMS/WFS/WMTS publishing. Here, "open-repository, code-verified" means evidenced by concrete files or benchmark artifacts inside the reviewed tree, not merely by forward-looking narrative text. Where enterprise-only or roadmap-adjacent documentation exists, that status is treated explicitly as a limitation rather than as present-tense feature availability.

The resulting conclusion is narrower and more defensible: ThemisDB is a strong geospatial data and query backend for emission-protection applications, while ArcGIS Pro remains the more complete interactive GIS and advanced analysis environment.

---

## Introduction

### Problem Statement

The previous version mixed together three different claim classes:

1. **Current repository behavior**
2. **Roadmap or deferred GPU targets**
3. **Hypothetical GIS integration paths**

That made the document unsuitable for review because implemented features, planned features, and external-tool responsibilities were not clearly separated.

### Review Goal

This version answers three focused questions:

1. Which emission-protection-relevant geospatial capabilities are currently implemented in ThemisDB and evidenced in the repository?
2. Where does ArcGIS Pro still provide materially broader functionality?
3. Which integration claims are documented as implemented, and which should remain explicitly unverified or out of scope?

### Terminology Used Consistently in This Review

| Term | Meaning in this document |
|---|---|
| **AQL** | ThemisDB query language used for `ST_*` functions and query composition |
| **Geo module** | The geospatial subsystem under `src/geo/` and related public headers |
| **Hybrid database system** | The repository term used in core docs/README for ThemisDB as a database engine with multiple workload types |
| **ArcGIS Pro** | Desktop GIS and analysis environment used here as the comparison baseline |
| **Emission-protection workflow** | Environmental screening, receptor analysis, zoning, or monitoring-support workflow; not a claim of built-in regulatory compliance automation |

---

## Methodology

### M1 — Codebase Fact Check

Claims were checked against repository artifacts instead of prior narrative text. The most important sources were:

- `include/query/functions/geo_functions.h` for exposed AQL geospatial functions
- `include/query/functions/crs_functions.h` for CRS transformation support
- `src/geo/ARCHITECTURE.md` and `src/geo/ROADMAP.md` for backend behavior and known limitations
- `benchmarks/bench_spatial_index.cpp` and `benchmarks/bench_spatial_join.cpp` for reproducible benchmark definitions
- `PERFORMANCE_EXPECTATIONS.md` and `artifacts/perf_local/bench_geo_v182_reference.json` for recorded benchmark results
- `README.md` and `ARCHITECTURE.md` for documented protocol frontends and ports

### M2 — Claim Classification

Every central statement was assigned to one of three classes:

- **Implemented / code-verified**
- **Documented target / roadmap-limited**
- **Not evidenced in the open repository**

### M3 — Comparison Scope

The comparison is intentionally asymmetric:

- **ThemisDB** is evaluated as a database/query backend.
- **ArcGIS Pro** is evaluated as a full GIS application and analysis suite.

This matters because some tasks that appear in environmental GIS practice—dispersion modeling, kriging, network analysis, interactive cartography—are application-layer or domain-tool responsibilities rather than database-core responsibilities.

### M4 — Evidence Standard for Performance Claims

A performance statement is treated as measured only when the repository contains both:

1. a benchmark path or executable source, and
2. a concrete recorded result or SLO reference.

Projected GPU speedups without repository measurements were removed.

---

## Evaluation

### E1 — Repository-Grounded ThemisDB Capability Snapshot

| Capability area | Status in repository | Evidence | Review note |
|---|---|---|---|
| Core geometry construction and parsing (`ST_POINT`, `ST_LINESTRING`, `ST_POLYGON`, `ST_GEOMFROMTEXT`, `ST_GEOMFROMGEOJSON`) | Implemented | `include/query/functions/geo_functions.h` | Suitable as database-side ingestion and normalization layer |
| Measurement and predicate functions (`ST_DISTANCE`, `ST_LENGTH`, `ST_AREA`, `ST_PERIMETER`, `ST_INTERSECTS`, `ST_CONTAINS`, `ST_WITHIN`, `ST_TOUCHES`, `ST_OVERLAPS`, `ST_DWITHIN`) | Implemented | `include/query/functions/geo_functions.h` | Covers the core screening/filtering primitives needed in many environmental workflows |
| Output/accessor functions (`ST_ASGEOJSON`, `ST_ASTEXT`, `ST_SRID`, `ST_HASZ`) | Implemented | `include/query/functions/geo_functions.h`, `include/query/functions/crs_functions.h` | Important for interoperability and result export |
| Processing functions (`ST_BUFFER`, `ST_CENTROID`, `ST_ENVELOPE`, `ST_SIMPLIFY`, `ST_UNION`, `ST_INTERSECTION`) | Implemented in AQL surface | `include/query/functions/geo_functions.h` | Provides essential buffering and overlay capability for backend-side proximity analysis |
| CRS transformation (`ST_TRANSFORM`) | Implemented | `include/query/functions/crs_functions.h` | Supports projection normalization for official datasets and mixed-source ingestion |
| CPU/GPU backend model | Implemented with CPU fallback behavior | `src/geo/ARCHITECTURE.md`, `src/geo/ROADMAP.md` | GPU exists, but several set operations still fall back to CPU |
| Raster query building blocks | Implemented | `src/geo/ROADMAP.md`, `src/geo/ARCHITECTURE.md` | Repository evidence supports raster query support, not full ArcGIS-style raster analyst parity |
| Spatial join primitive | Implemented and benchmarked | `src/geo/ROADMAP.md`, `benchmarks/bench_spatial_join.cpp` | Useful for proximity/receptor matching workloads |
| Protocol frontends relevant for integration | Documented | `README.md`, `ARCHITECTURE.md` | REST, GraphQL, gRPC, PostgreSQL wire compatibility, and custom Wire V2 are documented |

### E2 — What the Repository Actually Proves About Geo Performance

| Metric | Evidence path | Current evidence |
|---|---|---|
| R-tree contains / point-in-box proxy throughput | `PERFORMANCE_EXPECTATIONS.md`, `benchmarks/bench_spatial_index.cpp`, `artifacts/perf_local/bench_geo_v182_reference.json` | `BM_RTree_Contains` is used as evidence path; `PERFORMANCE_EXPECTATIONS.md` reports 435 M pts/s for the GEO-2 proxy target |
| R-tree intersects latency | `benchmarks/bench_spatial_index.cpp`, `PERFORMANCE_EXPECTATIONS.md` | `BM_RTree_Intersects/1000000` is recorded at 6.03 ms; documented as a direct measurement that narrowly misses the 5 ms target |
| Spatial join time-to-first-results | `benchmarks/bench_spatial_join.cpp`, `PERFORMANCE_EXPECTATIONS.md` | `BM_SpatialJoin_First1000/100000` is recorded at 312 ms, meeting the documented <= 500 ms target |

These results support claims about **indexing and spatial filtering primitives**. They do **not** support claims about end-to-end emission dispersion, kriging, viewshed throughput, or ArcGIS-like analyst workloads.

Benchmark context matters: `bench_spatial_index.cpp` uses synthetic lon/lat point distributions and an MBR query, while `bench_spatial_join.cpp` uses two generated Berlin-centered point collections with a 1 km threshold and a capped first-result workload. `PERFORMANCE_EXPECTATIONS.md` remains the authoritative place for SLO framing and result interpretation.

### E3 — ArcGIS Pro vs ThemisDB: Corrected Comparison Matrix

| Workflow / feature class | ArcGIS Pro | ThemisDB (repo-evidenced) | Review judgement |
|---|---|---|---|
| Buffer and proximity analysis | Native analysis tools with interactive workflow support | `ST_BUFFER`, `ST_DISTANCE`, `ST_DWITHIN`, `ST_INTERSECTS` available in AQL | ThemisDB is credible for backend-side proximity screening; ArcGIS Pro remains stronger for full analyst UX |
| CRS transformation / projection changes | Mature GIS tooling | `ST_TRANSFORM` and `ST_SRID` are implemented | Prior draft understated ThemisDB here |
| Spatial indexing and candidate filtering | Present in GIS stack | R-tree and benchmarked query paths are repository-evidenced | ThemisDB has strong backend evidence in this category |
| Spatial joins / receptor matching | Available via GIS workflows | Spatial JOIN implementation plus benchmark evidence | ThemisDB is credible for batch/backend matching workloads |
| Raster analyst features (slope, aspect, hillshade, full surface analysis) | Broad toolbox support | Raster query building blocks only | Do not claim feature parity |
| Viewshed / line-of-sight analysis | Available in ArcGIS analyst toolchains | No code-verified viewshed implementation found in open geo module | Remains an ArcGIS-side advantage |
| Kriging / geostatistical interpolation | Available in ArcGIS Geostatistical Analyst | No code-verified `ST_KRIGING` or equivalent implementation found | Prior draft's pseudo-code was speculative and has been removed |
| Dispersion modeling / regulatory plume calculation | Typically handled through specialist workflows and external models | No repository evidence for built-in dispersion engine | Must remain application/external-tool scope |
| Network / service-area analysis | Available in ArcGIS Network Analyst | No geo-module evidence for network/service-area implementation | Not a defensible ThemisDB parity claim |
| OGC WMS/WFS/WMTS publishing | Standard GIS ecosystem capability | No source-backed implementation found in open repository | The prior document's proposed OGC architecture was hypothetical, not current state |
| Direct ArcGIS provider integration | ArcGIS-native | Only documentation references to an enterprise plugin were found (`docs/de/integrations/arcgis_data_provider.md`); matching source/header artifacts are not present in the open-source clone reviewed here | Treat as documented but not source-verifiable in this repository state; readers should rely on the enterprise documentation path for further verification |

### E4 — Emission-Protection Use Cases: What ThemisDB Can Realistically Support

#### 1. Air-quality monitoring and receptor screening

**Well supported as backend building blocks:**

- buffering candidate or source locations,
- distance and within-threshold checks,
- intersection of impact zones with receptors,
- spatial join style matching,
- export through GeoJSON/WKT-oriented functions.

**Not evidenced as built-in database features:**

- kriging,
- plume/dispersion simulation,
- optimization for sensor placement.

**Review judgement:** ThemisDB is a good backend for data preparation and spatial pre-filtering, but not a substitute for domain modeling tools.

#### 2. Noise screening and protection-zone analysis

**Supported building blocks:**

- proximity calculations,
- buffers,
- area/intersection-based screening,
- indexed retrieval of affected assets.

**Missing for parity with advanced GIS workflows:**

- terrain-aware propagation,
- cost-distance style analysis,
- line-of-sight / obstruction modeling.

**Review judgement:** feasible for screening and candidate selection; not evidenced for full noise-model computation.

#### 3. Water protection and source-zone workflows

**Supported building blocks:**

- simple zone buffers,
- intersection checks with facilities or land-use geometries,
- CRS normalization and geometry export.

**Not evidenced:**

- watershed delineation,
- groundwater travel-time modeling,
- hydrological simulation.

**Review judgement:** suitable for zone management inputs, not for hydrological modeling itself.

#### 4. Industrial facility siting and consultation distances

**Supported building blocks:**

- safety-distance buffers,
- receptor overlap checks,
- land-area calculations,
- batch candidate/receptor screening.

**Not evidenced:**

- emergency routing,
- domino-effect simulation,
- interactive planning workflows.

**Review judgement:** strong database-side screening support; limited evidence for end-to-end siting analysis beyond geo primitives.

### E5 — Integration Claims That Survive Review

The following statements are justified by repository artifacts:

- ThemisDB documents a REST/HTTP endpoint on port `8765` and a wire protocol on port `8766` in `README.md`.
- The root architecture documentation lists REST, GraphQL, gRPC, PostgreSQL wire compatibility, WebSocket, MQTT, and a binary wire protocol as server-side frontends.
- PostgreSQL wire compatibility should be described exactly that way; it is **not** the same as verified PostGIS feature parity.

The following statements are **not** justified as present-tense open-repository facts:

- implemented WFS/WMS/WMTS services,
- validated ArcGIS Pro compatibility via PostgreSQL/PostGIS semantics,
- source-available ArcGIS data-provider plugin code in this clone,
- measured GPU speedups for viewshed, kriging, or dispersion workloads.

---

## Limitations

1. **Open-repository visibility is incomplete for ArcGIS-specific integration claims.**
   The open-source clone contains documentation references to an enterprise ArcGIS data provider—most directly `docs/de/integrations/arcgis_data_provider.md`—but no matching header or source files were found under `include/` or `src/`. This may indicate enterprise-only artifacts outside the reviewed tree; in either case, the integration cannot be treated as source-verified here. Readers who need to assess that path should start with `docs/de/integrations/arcgis_data_provider.md` and request the corresponding non-open artifacts through the appropriate product channel.

2. **Benchmark evidence is narrow.**
   The repository contains solid evidence for R-tree queries and spatial join primitives, but not for full environmental analysis pipelines.

3. **GPU support is real but not universal.**
   `src/geo/ARCHITECTURE.md` and `src/geo/ROADMAP.md` explicitly state that some GPU requests—especially `ST_BUFFER`, `ST_UNION`, and `ST_DIFFERENCE`—still use CPU fallback paths or deferred dedicated kernels.

4. **ArcGIS Pro remains broader in analyst tooling.**
   ThemisDB should not be marketed as having parity with ArcGIS Pro's viewshed, geostatistical, network, or cartographic toolchains based on the current repository state.

5. **Emission-protection compliance is workflow-specific.**
   Regulatory acceptance depends on external models, validated datasets, and domain procedures that are outside the geo module's current code evidence.

---

## Conclusion

After aligning the document with the current ThemisDB repository, the defensible conclusion is straightforward: ThemisDB is materially capable on the **database side** of geospatial work. Core AQL geo functions, buffering, CRS transformation, spatial indexing, raster query building blocks, and spatial joins are present and partly benchmarked.

However, ArcGIS Pro still holds the advantage for full analyst workflows, especially where interactive GIS tooling, geostatistics, viewshed/surface analysis, network analysis, or specialized environmental models are required. For emission-protection workloads, ThemisDB should therefore be positioned as a **geospatial data backend and query engine**, not as a drop-in replacement for ArcGIS Pro.

---

## References

### External sources

1. Esri. *Buffer (Analysis)—ArcGIS Pro documentation*.
   URL: [https://pro.arcgis.com/en/pro-app/latest/tool-reference/analysis/buffer.htm](https://pro.arcgis.com/en/pro-app/latest/tool-reference/analysis/buffer.htm)
2. Esri. *Viewshed—ArcGIS Pro documentation*.
   URL: [https://pro.arcgis.com/en/pro-app/latest/tool-reference/3d-analyst/viewshed.htm](https://pro.arcgis.com/en/pro-app/latest/tool-reference/3d-analyst/viewshed.htm)
3. Esri. *Kriging—ArcGIS Pro documentation*.
   URL: [https://pro.arcgis.com/en/pro-app/latest/tool-reference/geostatistical-analyst/kriging.htm](https://pro.arcgis.com/en/pro-app/latest/tool-reference/geostatistical-analyst/kriging.htm)
4. Open Geospatial Consortium. *Simple Feature Access Standard*.
   URL: [https://www.ogc.org/standards/sfa/](https://www.ogc.org/standards/sfa/)
5. European Union. *Directive 2008/50/EC on ambient air quality and cleaner air for Europe*.
   URL: [https://eur-lex.europa.eu/eli/dir/2008/50/oj](https://eur-lex.europa.eu/eli/dir/2008/50/oj)
6. European Union. *Directive 2002/49/EC relating to the assessment and management of environmental noise*.
   URL: [https://eur-lex.europa.eu/eli/dir/2002/49/oj](https://eur-lex.europa.eu/eli/dir/2002/49/oj)
7. Cressie, N. A. C. (1993). *Statistics for Spatial Data*.
   URL: [https://onlinelibrary.wiley.com/doi/book/10.1002/9781119115151](https://onlinelibrary.wiley.com/doi/book/10.1002/9781119115151)

### Internal repository artifacts

The entries below mix **source-backed artifacts** (headers, architecture docs, benchmarks, result files) with one explicitly marked **documentation-only** entry for an enterprise integration path that is not source-verifiable in this clone.

- `include/query/functions/geo_functions.h`
- `include/query/functions/crs_functions.h`
- `src/geo/ARCHITECTURE.md`
- `src/geo/ROADMAP.md`
- `README.md`
- `ARCHITECTURE.md`
- `benchmarks/bench_spatial_index.cpp`
- `benchmarks/bench_spatial_join.cpp`
- `PERFORMANCE_EXPECTATIONS.md`
- `artifacts/perf_local/bench_geo_v182_reference.json`
- `docs/de/integrations/arcgis_data_provider.md` (documentation-only evidence; not source-backed in this clone)
