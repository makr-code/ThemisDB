# ArcGIS Pro vs ThemisDB Geospatial Capabilities: Comparison & Emission Protection Analysis

**Date:** 2026-01-11  
**Version:** 1.0  
**Author:** Research Analysis for ThemisDB  
**Target:** Geospatial GPU/VRAM acceleration and emission protection use cases

---

## Executive Summary

This document provides a comprehensive comparison between ArcGIS Pro's geospatial toolbox and ThemisDB's current capabilities, with specific focus on:
1. **Feature Gap Analysis** - What ArcGIS Pro offers vs. ThemisDB's implementation
2. **Emission Protection Use Cases** - Functions critical for environmental compliance
3. **GPU/VRAM Acceleration Opportunities** - Which operations benefit most from GPU processing

**Key Findings:**
- ThemisDB has solid foundation (25+ spatial functions, R-Tree indexing, 3D support)
- **Critical gaps for emission protection**: Buffer analysis, viewshed analysis, dispersion modeling
- **High-value GPU opportunities**: Large-scale buffer operations, visibility analysis, spatial interpolation
- **Quick wins**: Implement ST_BUFFER, ST_ISVALID, spatial statistics functions

---

## Scope: Database vs. Application Logic

> **Important Distinction:** ThemisDB is a database system focused on **data preparation and provisioning**. This document identifies geospatial functions that belong in the database layer, not application-level analysis tools.

### Database Responsibilities (ThemisDB Scope) ✅
**Core Principle:** Prepare and provide spatial data efficiently for applications to consume.

- **Spatial data storage** - Store geometries with appropriate indexing (R-Tree, Morton encoding)
- **Spatial queries** - Enable applications to find, filter, and retrieve spatial data
  - Example: `ST_INTERSECTS`, `ST_CONTAINS`, `ST_DWITHIN` for spatial predicates
- **Geometric operations** - Basic transformations and measurements
  - Example: `ST_BUFFER`, `ST_CENTROID`, `ST_AREA`, `ST_DISTANCE`
- **Coordinate transformations** - Convert between coordinate systems (EPSG codes)
  - Example: `ST_TRANSFORM` for projection changes
- **Data validation** - Ensure geometric integrity
  - Example: `ST_ISVALID`, `ST_MAKEVALID`
- **Spatial indexing** - Enable fast spatial queries via R-Tree and spatial indexes
- **GPU acceleration** - Accelerate *database operations* (buffer, intersection, distance calculations)

### Application Logic (Outside ThemisDB Scope) ❌
**These belong in specialized GIS applications, not the database:**

- **Domain-specific modeling** - Emission dispersion calculations, noise propagation models
  - *Why*: Requires domain expertise, regulatory formulas, meteorological data
  - *Solution*: Application reads prepared spatial data from ThemisDB and applies models
  
- **Complex workflows** - Multi-step analysis pipelines (viewshed → interpolation → risk assessment)
  - *Why*: Business logic varies by use case and regulatory requirements
  - *Solution*: Application orchestrates multiple database queries
  
- **Visualization** - Map rendering, 3D visualization, interactive dashboards
  - *Why*: UI/UX concerns, not data management
  - *Solution*: Application consumes GeoJSON/WKT from ThemisDB
  
- **Decision support** - Optimal sensor placement, facility siting recommendations
  - *Why*: Requires optimization algorithms and domain constraints
  - *Solution*: Application evaluates alternatives using spatial queries
  
- **Statistical analysis** - Geostatistical modeling (kriging), spatial autocorrelation
  - *Why*: Complex statistical methods better suited for R/Python/specialized tools
  - *Solution*: Export data from ThemisDB, analyze externally, store results back

### Boundary Cases: Database-Assisted Analysis
Some operations can be *partially* supported by the database to improve performance:

| Operation | Database Role | Application Role |
|-----------|---------------|------------------|
| **Kriging Interpolation** | Store sample points, spatial index | Compute semivariogram, solve kriging system |
| **Viewshed Analysis** | Store DEM raster, spatial queries | Ray-tracing algorithm, visibility computation |
| **Network Analysis** | Store network topology, cost functions | Dijkstra's algorithm, route optimization |
| **Kernel Density** | Store point data, spatial index | Compute density surface, bandwidth selection |

**Recommendation:** ThemisDB should provide *building blocks* (spatial queries, geometric operations) that applications compose into domain-specific workflows.

---

## GIS Integration: Interfaces & Protocols

> **Question:** How can ThemisDB provide interfaces to programs like ArcGIS, binary interfaces, or OGC web services (WMS, WFS, WMTS)?

**Answer:** Yes, ThemisDB can provide multiple integration paths for GIS applications. Here's a comprehensive overview:

### Current Integration Capabilities ✅

ThemisDB already supports several integration methods:

1. **REST API (HTTP/1.1)** ✅ Currently Available
   - Endpoint: `http://localhost:8765/`
   - Methods: GET, POST, PUT, DELETE
   - Format: JSON request/response
   - Spatial data: GeoJSON format via `ST_ASGEOJSON()`
   - **Use case:** Custom applications, web GIS clients

2. **GraphQL API** ✅ Currently Available
   - Query language for flexible data retrieval
   - Can request specific geometry fields
   - **Use case:** Modern web applications, mobile apps

3. **Binary Wire Protocol** ✅ Currently Available
   - Port: 18765
   - High-performance binary communication
   - **Use case:** High-throughput applications, custom clients

4. **gRPC** ✅ Currently Available (v1.3.0+)
   - Protocol Buffers for efficient serialization
   - Bidirectional streaming support
   - **Use case:** Microservices, distributed systems

5. **PostgreSQL Wire Protocol** ✅ Currently Available (v1.3.0+)
   - Port: 5432 (configurable)
   - **Critical for GIS integration:** Enables PostGIS-compatible clients
   - **Use case:** QGIS, ArcGIS Pro (via PostgreSQL connection), pgAdmin
   - **Enable:** `cmake -DTHEMIS_ENABLE_POSTGRES_WIRE=ON`

### OGC Web Services Implementation Path 🚧

ThemisDB can implement OGC (Open Geospatial Consortium) standards for broad GIS compatibility:

#### 1. WFS (Web Feature Service) - **RECOMMENDED PRIORITY**
**Status:** Not yet implemented (can be added)

**Architecture:**
```
Client (ArcGIS Pro, QGIS) 
    ↓ HTTP GET/POST
WFS Endpoint (:8080/wfs)
    ↓ Parse WFS request (GetCapabilities, GetFeature, DescribeFeatureType)
ThemisDB Query Engine
    ↓ ST_ASGEOJSON() or ST_ASTEXT()
GML/GeoJSON Response
```

**Implementation Approach:**
- Add WFS handler to existing HTTP server (`src/server/http_server.cpp`)
- Map WFS operations to AQL queries:
  - `GetCapabilities` → Return available feature types from schema
  - `GetFeature` → `FOR f IN features FILTER bbox RETURN f` + GML serialization
  - `DescribeFeatureType` → Return schema for feature class
  - `Transaction` (WFS-T) → INSERT/UPDATE/DELETE via AQL

**Effort Estimate:** 3-4 weeks
- 1 week: WFS protocol parser (GetCapabilities, GetFeature)
- 1 week: GML 3.2 output formatter
- 1 week: WFS-T (transactional) support
- 1 week: Testing with QGIS, ArcGIS Pro

**Standards Compliance:**
- WFS 2.0.0 (ISO 19142:2010)
- GML 3.2.1 (ISO 19136:2007)
- Filter Encoding 2.0 (OGC 09-026r2)

**Example WFS Request:**
```http
GET /wfs?
  service=WFS&
  version=2.0.0&
  request=GetFeature&
  typeName=emission_sources&
  bbox=8.0,50.0,9.0,51.0&
  outputFormat=application/json
```

**ThemisDB Implementation:**
```cpp
// src/server/wfs_handler.cpp
std::string handleWFSGetFeature(const WFSRequest& req) {
    // Build AQL query from WFS request
    std::string aql = "FOR feature IN " + req.typeName;
    
    if (req.bbox) {
        aql += " FILTER ST_INTERSECTS(feature.geometry, "
               "ST_ENVELOPE(" + req.bbox.toWKT() + "))";
    }
    
    if (req.filter) {
        aql += " FILTER " + translateCQLToAQL(req.filter);
    }
    
    aql += " RETURN {geometry: ST_ASGEOJSON(feature.geometry), "
           "properties: feature.properties}";
    
    auto results = queryEngine->execute(aql);
    return formatAsGeoJSON(results);  // or GML if requested
}
```

#### 2. WMS (Web Map Service) - **OPTIONAL**
**Status:** Not yet implemented (application layer concern)

**Architecture:**
```
Client (ArcGIS Pro, Web Browser)
    ↓ HTTP GET
WMS Endpoint (:8080/wms)
    ↓ GetMap request (bbox, width, height, layers)
ThemisDB Query + Rendering Engine
    ↓ Rasterize vector features or serve raster tiles
PNG/JPEG Image Response
```

**Recommendation:** ⚠️ **Consider external renderer**
- WMS requires **map rendering** (application logic, not database)
- **Alternative 1:** Use MapServer/GeoServer as WMS frontend, ThemisDB as backend via PostgreSQL protocol
- **Alternative 2:** Implement basic WMS for raster data only (serve stored tiles)
- **Alternative 3:** Use WFS + client-side rendering (QGIS, Leaflet, OpenLayers)

**If implementing in ThemisDB:**
- Effort: 6-8 weeks (complex rendering pipeline)
- Dependencies: Image rendering library (Cairo, AGG, Skia)
- Standards: WMS 1.3.0 (ISO 19128:2005)

#### 3. WMTS (Web Map Tile Service) - **MEDIUM PRIORITY**
**Status:** Not yet implemented (can be added)

**Architecture:**
```
Client (Web Map, ArcGIS Pro)
    ↓ HTTP GET
WMTS Endpoint (:8080/wmts/{z}/{x}/{y}.png)
    ↓ Tile coordinate → bbox
ThemisDB Raster Storage
    ↓ Pre-computed tiles or on-the-fly generation
PNG Tile Response
```

**Implementation Approach:**
- **Pre-computed tiles:** Store tiles in ThemisDB blob storage, serve directly
- **On-the-fly tiles:** Query features in tile bbox, render with library
- **Standards:** WMTS 1.0.0 (OGC 07-057r7)

**Effort Estimate:** 2-3 weeks (for pre-computed tiles)

**Use Case:** Base maps, aerial imagery, elevation tiles

#### 4. WCS (Web Coverage Service) - **LOW PRIORITY**
**Status:** Not yet implemented

**Purpose:** Serve raster data (DEM, satellite imagery) with metadata
**Standards:** WCS 2.0.1 (OGC 09-110r4)
**Recommendation:** Implement if ThemisDB stores significant raster data

### Binary Interface for Direct Integration 🔧

For high-performance GIS applications (custom ArcGIS extensions, QGIS plugins):

#### Option 1: PostgreSQL Wire Protocol (RECOMMENDED) ✅
**Status:** Already implemented in ThemisDB v1.3.0+

**Advantages:**
- ✅ ArcGIS Pro can connect as PostgreSQL database
- ✅ QGIS has native PostgreSQL/PostGIS support
- ✅ pgAdmin for database management
- ✅ Spatial queries via SQL: `SELECT ST_Buffer(geom, 500) FROM features`

**Connection String:**
```
Host: localhost
Port: 5432
Database: themis
User: admin
Password: ****

# ArcGIS Pro: Add Database Connection → PostgreSQL
# QGIS: Layer → Add Layer → Add PostGIS Layers
```

**Limitation:** SQL dialect differences (ThemisDB uses AQL, not full PostgreSQL SQL)
**Workaround:** Implement SQL-to-AQL translator or PostgreSQL-compatible SQL parser

#### Option 2: GDAL/OGR Driver (BEST COMPATIBILITY)
**Status:** Not yet implemented (high-value target)

**Why Important:**
- GDAL/OGR is the **universal GIS library**
- Used by: ArcGIS, QGIS, GRASS GIS, MapServer, GeoServer, FME, etc.
- Enables: `ogrinfo themisdb:emission_sources`, `ogr2ogr -f "ThemisDB" ...`

**Implementation Path:**
1. Create GDAL driver plugin (`ogr_themis.cpp`)
2. Register driver in GDAL driver list
3. Implement OGR vector driver interface:
   - `Open()` - Connect to ThemisDB
   - `GetLayer()` - Map ThemisDB tables to OGR layers
   - `GetFeature()` - Execute spatial queries
   - `CreateFeature()` - Insert geometries

**Effort Estimate:** 4-6 weeks
- 2 weeks: OGR driver skeleton + read operations
- 1 week: Write operations (insert, update, delete)
- 1 week: Spatial filter translation (OGR → AQL)
- 1-2 weeks: Testing with GDAL utilities and GIS clients

**Example Usage After Implementation:**
```bash
# List ThemisDB layers
ogrinfo themisdb:localhost:8765

# Query with spatial filter
ogrinfo themisdb:localhost:8765 emission_sources \
  -spat 8.0 50.0 9.0 51.0

# Convert from Shapefile to ThemisDB
ogr2ogr -f ThemisDB \
  themisdb:localhost:8765 \
  emission_sources.shp

# ArcGIS Pro can then use "Add Data from Path" → OGR driver
```

#### Option 3: Native Client Libraries (SDK)
**Status:** Partially available (REST/gRPC clients)

ThemisDB already has client SDKs:
- ✅ Python client (`clients/python/`)
- ✅ JavaScript client (`clients/javascript/`)
- ✅ Go client (`clients/go/`)
- ✅ Rust client (`clients/rust/`)
- ✅ Java client (`clients/java/`)

**For GIS Integration:**
- Create ArcGIS Pro Python toolbox using ThemisDB Python client
- Create QGIS plugin using ThemisDB Python client
- Create FME transformer using Java/C++ client

### Recommended Implementation Priority 🎯

| Priority | Interface | Effort | Impact | Compatibility |
|----------|-----------|--------|--------|---------------|
| 🔴 **P0** | **GDAL/OGR Driver** | 4-6 weeks | **Universal GIS compatibility** | ArcGIS, QGIS, all GIS tools |
| 🔴 **P0** | **PostgreSQL Protocol Enhancement** | 2-3 weeks | **Immediate QGIS/pgAdmin support** | PostGIS-compatible clients |
| 🟡 **P1** | **WFS 2.0** | 3-4 weeks | **Standards-based feature access** | Web GIS, ArcGIS Server |
| 🟡 **P1** | **WMTS** | 2-3 weeks | **Tile-based visualization** | Web maps, mobile apps |
| 🟢 **P2** | **WCS** | 3-4 weeks | **Raster data access** | Remote sensing applications |
| 🔵 **P3** | **WMS** | 6-8 weeks | **Rendered maps** | Consider external renderer |

### Integration Architecture Example

```
┌─────────────────────────────────────────────────────────────┐
│                     GIS Clients                              │
│  ArcGIS Pro │ QGIS │ Web GIS │ Custom Apps │ Mobile         │
└────┬────────┴──┬───┴────┬────┴─────┬───────┴────┬───────────┘
     │           │         │          │            │
     ├───────────┴─────────┴──────────┴────────────┤
     │           Integration Layer                  │
     ├──────────────────────────────────────────────┤
     │  GDAL/OGR │ PostgreSQL │ WFS │ REST │ gRPC  │
     └────┬──────┴──────┬─────┴─────┴──┬───┴───┬───┘
          │             │                │       │
     ┌────▼─────────────▼────────────────▼───────▼────┐
     │            ThemisDB Core Engine                 │
     │  ┌─────────────────────────────────────────┐   │
     │  │ Spatial Storage (R-Tree, Morton)        │   │
     │  ├─────────────────────────────────────────┤   │
     │  │ Query Engine (AQL + Spatial Functions)  │   │
     │  ├─────────────────────────────────────────┤   │
     │  │ GPU Acceleration (CUDA/Vulkan)          │   │
     │  └─────────────────────────────────────────┘   │
     └──────────────────────────────────────────────────┘
```

### Summary: Enabling GIS Integration

**Short Answer:** Yes, ThemisDB can provide interfaces to ArcGIS and other GIS programs through:

1. ✅ **Currently Available:**
   - PostgreSQL wire protocol (connect as PostGIS database)
   - REST API with GeoJSON
   - gRPC binary protocol
   - Native client SDKs (Python, JavaScript, Go, Java, Rust)

2. 🚧 **Recommended Additions:**
   - **GDAL/OGR driver** (4-6 weeks) - Universal compatibility
   - **WFS 2.0** (3-4 weeks) - Standards-based feature access
   - **WMTS** (2-3 weeks) - Tile service for web maps

3. ⚠️ **Application Layer (External):**
   - **WMS rendering** - Use MapServer/GeoServer with ThemisDB backend
   - **Complex cartography** - Client-side rendering via WFS + Leaflet/OpenLayers

**Best Path Forward:**
1. Enhance PostgreSQL protocol compatibility (SQL dialect support)
2. Implement GDAL/OGR driver for universal GIS access
3. Add WFS endpoint for standards-based integration
4. Document integration examples for ArcGIS Pro, QGIS, Web GIS

---

## Table of Contents

1. [ThemisDB Current Geospatial Capabilities](#1-themisdb-current-geospatial-capabilities)
2. [ArcGIS Pro Geospatial Toolbox Overview](#2-arcgis-pro-geospatial-toolbox-overview)
3. [Feature Comparison Matrix](#3-feature-comparison-matrix)
4. [Emission Protection Use Cases](#4-emission-protection-use-cases-immissionsschutz)
5. [GPU/VRAM Acceleration Opportunities](#5-gpuvram-acceleration-opportunities)
6. [Implementation Roadmap](#6-implementation-roadmap)
7. [GIS Integration Interfaces](#gis-integration-interfaces--protocols)
8. [References](#7-references)

---

## 1. ThemisDB Current Geospatial Capabilities

### 1.1 Implemented Spatial Functions

Based on `/include/query/functions/geo_functions.h` and `/src/geo/`:

#### Construction Functions ✅
- `ST_POINT(x, y, [z])` - Create point geometry
- `ST_LINESTRING(points)` - Create line geometry
- `ST_POLYGON(rings)` - Create polygon geometry
- `ST_GEOMFROMTEXT(wkt)` - Parse WKT (Well-Known Text)
- `ST_GEOMFROMGEOJSON(json)` - Parse GeoJSON

#### Measurement Functions ✅
- `ST_DISTANCE(geom1, geom2)` - Haversine distance (geographic) or Euclidean (projected)
- `ST_LENGTH(linestring)` - Line length calculation
- `ST_AREA(polygon)` - Polygon area calculation
- `ST_PERIMETER(polygon)` - Polygon perimeter

#### Spatial Predicates ✅
- `ST_INTERSECTS(geom1, geom2)` - Check if geometries intersect
- `ST_CONTAINS(geom1, geom2)` - Check if geom1 contains geom2
- `ST_WITHIN(geom1, geom2)` - Check if geom1 is within geom2
- `ST_TOUCHES(geom1, geom2)` - Check if boundaries touch
- `ST_OVERLAPS(geom1, geom2)` - Check if geometries overlap
- `ST_DWITHIN(geom1, geom2, distance)` - Check if within distance

#### Accessors ✅
- `ST_X(point)` - Get X coordinate
- `ST_Y(point)` - Get Y coordinate
- `ST_Z(point)` - Get Z coordinate (3D support)
- `ST_SRID(geom)` - Get Spatial Reference ID
- `ST_ASGEOJSON(geom)` - Export to GeoJSON
- `ST_ASTEXT(geom)` - Export to WKT
- `ST_HASZ(geom)` - Check if geometry has Z coordinate

#### Processing Functions ✅ (Partial)
- `ST_CENTROID(geom)` - Calculate centroid
- `ST_ENVELOPE(geom)` - Get minimum bounding rectangle
- `ST_SIMPLIFY(geom, tolerance)` - Simplify geometry
- `ST_UNION(geom1, geom2)` - Union geometries
- `ST_INTERSECTION(geom1, geom2)` - Intersect geometries

### 1.2 Indexing & Performance

#### Spatial Indexing ✅
- **R-Tree** - Multi-dimensional indexing (`include/index/spatial_index.h`)
- **Morton Z-order encoding** - Space-filling curve for efficient spatial clustering
- **Two-stage filtering** - Fast MBR check → Exact geometry check
- **Bounding box operations** - Optimized MBR contains/intersects

#### Backend Architecture ✅
- **CPU Backend** - Boost.Geometry integration (`src/geo/cpu_backend.cpp`)
- **GPU Backend Stub** - Placeholder for CUDA/Vulkan (`src/geo/gpu_backend_stub.cpp`)
- **Pluggable backend system** - ISpatialComputeBackend interface

#### Coordinate Systems ✅
- **WGS84 (EPSG:4326)** - Default geographic coordinate system
- **3D support** - X, Y, Z coordinates
- **SRS parsing** - Via GDAL integration

### 1.3 Integration Status

#### GDAL Integration ✅
- Shapefile parsing with OGR
- GeoTIFF raster processing
- Coordinate extraction with bounding boxes
- Performance: 1K features < 100ms

#### Missing Components ❌
- No on-the-fly coordinate transformation
- No spatial statistics functions
- No raster analysis beyond metadata
- No network analysis
- No 3D visibility analysis

---

## 2. ArcGIS Pro Geospatial Toolbox Overview

ArcGIS Pro provides comprehensive GIS capabilities organized into toolboxes. Key categories relevant to emission protection:

### 2.1 Analysis Toolbox

#### Proximity Tools (Critical for Emission Protection)
- **Buffer** - Create buffer zones around features
  - Fixed distance, variable distance, multiple rings
  - Planar vs. geodesic buffering
  - Dissolve options for overlapping buffers
  
- **Near** - Calculate nearest feature and distance
  - Find nearest facility
  - Distance accumulation
  - Allocation zones

- **Generate Near Table** - Tabular proximity analysis
  - K-nearest neighbors
  - Distance matrices

#### Overlay Tools
- **Intersect** - Geometric intersection of features
- **Union** - Combine all features preserving boundaries
- **Erase** - Remove overlapping areas
- **Identity** - Preserve input features, compute overlaps
- **Symmetrical Difference** - XOR operation on geometries

#### Extract Tools
- **Clip** - Extract features within boundary
- **Select** - Spatial and attribute queries
- **Split** - Divide features by boundaries

### 2.2 Spatial Analyst Extension (Raster Analysis)

#### Surface Analysis (Critical for Air Dispersion)
- **Slope** - Calculate terrain slope
- **Aspect** - Calculate terrain orientation
- **Hillshade** - 3D visualization
- **Viewshed** - Visibility analysis from observation points
  - **CRITICAL for emission sources**: Determine affected areas
  - GPU-accelerated in ArcGIS Pro

#### Interpolation (for Concentration Mapping)
- **IDW (Inverse Distance Weighting)** - Simple interpolation
- **Kriging** - Geostatistical interpolation
  - Ordinary kriging, universal kriging, simple kriging
  - **CRITICAL for air quality monitoring**: Predict concentrations between sensors
- **Spline** - Smooth surface interpolation
- **Natural Neighbor** - Voronoi-based interpolation

#### Density Analysis
- **Kernel Density** - Concentration heatmaps
- **Point Density** - Incident density mapping
- **Line Density** - Linear feature density

### 2.3 3D Analyst Extension

#### Visibility Analysis (Emission Plume Modeling)
- **Viewshed** - 3D visibility from point sources
- **Observer Points** - Multiple observation locations
- **Skyline** - Horizon analysis
- **Line of Sight** - Obstruction analysis

#### 3D Surface Analysis
- **Surface Volume** - Calculate volumes between surfaces
- **Cut/Fill** - Excavation/deposition volumes
- **3D Buffer** - Volumetric buffers

### 2.4 Network Analyst Extension

#### Routing (for Emergency Response)
- **Route** - Optimal path finding
- **Service Area** - Reachability analysis from emission sources
- **Closest Facility** - Nearest emergency services
- **Location-Allocation** - Optimal sensor placement

### 2.5 Geostatistical Analyst

#### Statistical Analysis (Air Quality Monitoring)
- **Exploratory Spatial Data Analysis (ESDA)**
  - Trend analysis
  - Semivariogram/covariance modeling
  - Cross-validation
  
- **Geostatistical Simulations**
  - Monte Carlo simulations for uncertainty
  - **CRITICAL**: Risk assessment for emission scenarios

### 2.6 Image Analyst Extension

#### Raster Processing
- **Classification** - Land use/land cover mapping
- **Change Detection** - Temporal analysis
- **Segmentation** - Object-based analysis

---

## 3. Feature Comparison Matrix

> **Note:** "DB Layer" column indicates whether function belongs in database (✅) or application logic (❌).

### 3.1 Core Spatial Operations (Database Layer)

| Function | ThemisDB | ArcGIS Pro | DB Layer | Priority | GPU Suitable |
|----------|----------|------------|----------|----------|--------------|
| **ST_BUFFER** | ❌ | ✅ Full | ✅ Core | **HIGH** | ✅ Yes |
| ST_INTERSECTS | ✅ | ✅ | ✅ Core | Medium | ✅ Yes |
| ST_CONTAINS | ✅ | ✅ | ✅ Core | Medium | ✅ Yes |
| ST_DISTANCE | ✅ | ✅ | ✅ Core | Low | ⚠️ Partial |
| ST_UNION | ✅ Partial | ✅ Full | ✅ Core | Medium | ✅ Yes |
| ST_INTERSECTION | ✅ Partial | ✅ Full | ✅ Core | Medium | ✅ Yes |
| ST_DIFFERENCE | ❌ | ✅ | ✅ Core | Medium | ✅ Yes |
| **ST_CONVEXHULL** | ❌ | ✅ | ✅ Core | **HIGH** | ✅ Yes |
| ST_CENTROID | ✅ | ✅ | ✅ Core | Low | ❌ No |
| ST_ENVELOPE | ✅ | ✅ | ✅ Core | Low | ❌ No |

### 3.2 Advanced Analysis (Mixed Responsibility)

| Function | ThemisDB | ArcGIS Pro | DB Layer | Priority | GPU Suitable | Notes |
|----------|----------|------------|----------|----------|--------------|-------|
| **Viewshed Analysis** | ❌ | ✅ Full | ⚠️ Partial | Medium | ✅ Yes | DB: Store DEM, spatial queries; App: Ray-tracing algorithm |
| **Kriging Interpolation** | ❌ | ✅ Full | ⚠️ Partial | Medium | ✅ Yes | DB: Store samples, index; App: Compute semivariogram, solve system |
| IDW Interpolation | ❌ | ✅ Full | ⚠️ Partial | Low | ✅ Yes | DB: Store samples; App: Compute weights, interpolate |
| Kernel Density | ❌ | ✅ Full | ⚠️ Partial | Low | ✅ Yes | DB: Store points, index; App: Compute density surface |
| Network Analysis | ❌ | ✅ Full | ⚠️ Partial | Low | ❌ No | DB: Store topology; App: Dijkstra's algorithm |
| 3D Volume Calculation | ❌ | ✅ Full | ✅ Core | Medium | ✅ Yes | DB can provide geometric volume calculations |
| Surface Analysis | ❌ | ✅ Full | ⚠️ Partial | Low | ✅ Yes | DB: Store raster; App: Slope/aspect algorithms |
| Cost Distance | ❌ | ✅ Full | ❌ App | Low | ✅ Yes | Complex algorithm, application logic |

### 3.3 Validation & Quality

| Function | ThemisDB | ArcGIS Pro | Priority | GPU Suitable |
|----------|----------|------------|----------|--------------|
| **ST_ISVALID** | ❌ | ✅ | **HIGH** | ❌ No |
| **ST_MAKEVALID** | ❌ | ✅ | **HIGH** | ❌ No |
| ST_ISSIMPLE | ❌ | ✅ | Medium | ❌ No |
| Topology Rules | ❌ | ✅ Full | Medium | ❌ No |

### 3.4 Coordinate Systems

| Function | ThemisDB | ArcGIS Pro | Priority | GPU Suitable |
|----------|----------|------------|----------|--------------|
| **ST_TRANSFORM** | ❌ | ✅ Full | **HIGH** | ⚠️ Partial |
| Define Projection | ✅ Partial | ✅ Full | Medium | ❌ No |
| Project Raster | ❌ | ✅ Full | Medium | ✅ Yes |

---

## 4. Emission Protection Use Cases (Immissionsschutz)

> **Architecture Note:** These use cases illustrate how ThemisDB provides spatial data preparation, while specialized applications handle domain-specific analysis and regulatory compliance workflows.

### 4.1 Air Quality Monitoring Network Design

**Regulatory Context:** EU Air Quality Directive (2008/50/EC), German BImSchG, TA Luft

#### Use Case 1: Optimal Sensor Placement
**Goal:** Position air quality sensors to maximize coverage while minimizing costs

**Database Responsibilities (ThemisDB):**
1. ✅ `ST_BUFFER` - **MISSING IN THEMISDB** - Create coverage zones around potential sensor locations
   - Fixed radius (e.g., 500m urban, 2km rural per TA Luft guidelines)
   - Variable radius based on population density or emission sources
   
2. ✅ `ST_UNION` (with dissolve) - Combine overlapping coverage zones
   - Currently partial in ThemisDB - needs dissolve functionality
   
3. ✅ `ST_DISTANCE` - Calculate distances to emission sources
   - ThemisDB supports this

4. ✅ `ST_INTERSECTS` - Check which areas are covered by sensor zones

**Application Responsibilities (External GIS Software):**
- ❌ **Location-Allocation Optimization** - Determine optimal sensor positions
  - Requires optimization algorithms (linear programming, genetic algorithms)
  - Business logic: Cost constraints, regulatory requirements, accessibility
  - *Application workflow*: Query candidate locations from ThemisDB, run optimization, store results back

**ThemisDB Role:** Provide spatial building blocks (buffers, distance calculations, intersection tests) that applications use to evaluate sensor placement scenarios.

**ArcGIS Pro Workflow (Application Level):**
```python
# Application orchestrates multiple database queries
# 1. Query candidate locations from ThemisDB
candidates = themisdb.query("FOR loc IN candidate_locations RETURN loc")

# 2. ArcGIS Pro optimization (application logic)
optimal_locations = arcpy.sa.LocationAllocation(
    facilities=candidates,
    demand_points=emission_sources,
    problem_type="MAXIMIZE_COVERAGE",
    impedance_cutoff=2000
)

# 3. Store results back to ThemisDB
themisdb.insert("sensors", optimal_locations)
```

**ThemisDB Query Example (Database Level):**
```sql
-- ThemisDB provides spatial queries for coverage analysis
FOR candidate IN candidate_locations
  LET buffer = ST_BUFFER(candidate.location, 500)  -- DB operation
  LET covered_sources = (
    FOR source IN emission_sources
      FILTER ST_INTERSECTS(source.location, buffer)  -- DB operation
      RETURN source
  )
  RETURN {
    candidate: candidate,
    coverage_zone: buffer,
    covered_count: LENGTH(covered_sources)
  }
```

#### Use Case 2: Emission Source Impact Analysis
**Goal:** Determine which areas are affected by industrial emissions

**Database Responsibilities (ThemisDB):**
1. ✅ `ST_BUFFER` - **MISSING** - Create protection zones
   - TA Luft: 200m, 500m, 1500m zones depending on facility type
   - Geodesic buffering for large areas (>10km)
   
2. ✅ `ST_INTERSECTS` - Check for residential/sensitive areas
   - ThemisDB supports this
   
3. ✅ Spatial queries to retrieve relevant features (buildings, land use, demographics)

**Application Responsibilities (External Software):**
- ❌ **Viewshed Analysis** - Determine visible areas from emission stack
  - Requires ray-tracing algorithm with terrain data
  - *Application*: Use DEM from ThemisDB, compute visibility, store results back
  
- ❌ **Kriging/IDW Interpolation** - Map pollutant concentrations
  - Requires geostatistical modeling (semivariogram, kriging system)
  - *Application*: Query monitoring data from ThemisDB, compute interpolation surface in R/Python, store back
  
- ❌ **Dispersion Modeling** - Calculate pollutant transport
  - Requires meteorological data, emission formulas (TA Luft Appendix 2)
  - *Application*: Specialized tools (AUSTAL2000, LASAT) query stack parameters from ThemisDB

**ThemisDB Role:** Store emission sources, DEM raster, monitoring locations. Provide spatial queries for application to consume.

**Application Workflow (Multi-Tool):**
```python
# 1. Query emission source from ThemisDB
source = themisdb.query("FOR s IN emission_sources FILTER s.id == @id RETURN s")[0]

# 2. Run dispersion model (application logic - AUSTAL2000)
concentration_grid = austal2000.compute_dispersion(
    stack_height=source.stack_height,
    emission_rate=source.emission_rate,
    meteorology=weather_data,  # External data source
    terrain=themisdb.get_dem(source.location, radius=5000)  # DB query
)

# 3. Store results back to ThemisDB
themisdb.insert_raster("emission_concentration", concentration_grid)

# 4. Query exceedances using ThemisDB spatial functions
exceedances = themisdb.query("""
  FOR pixel IN emission_concentration_raster
    FILTER pixel.value > 40  -- PM10 limit
    LET buildings = (
      FOR b IN buildings
        FILTER ST_INTERSECTS(b.geometry, pixel.geometry)
        RETURN b
    )
    RETURN {pixel, affected_buildings: buildings}
""")
```

**ThemisDB Query Example (Database Level):**
```sql
-- ThemisDB provides spatial building blocks
FOR source IN emission_sources
  LET buffer_zone = ST_BUFFER(source.location, 500)  -- DB operation
  LET affected_buildings = (
    FOR building IN buildings
      FILTER ST_INTERSECTS(building.geometry, buffer_zone)  -- DB operation
      RETURN building
  )
  RETURN {
    source: source.name,
    protection_zone: buffer_zone,
    building_count: LENGTH(affected_buildings),
    affected: affected_buildings
  }
```

### 4.2 Noise Emission Mapping (Lärmschutz)

**Regulatory Context:** EU Environmental Noise Directive, German BImSchG §§ 41-43

#### Use Case 3: Road Traffic Noise Mapping
**Goal:** Create noise maps showing affected residential areas

**Required Functions:**
1. ✅ `ST_BUFFER` - **MISSING** - Distance-based attenuation zones
   - Multiple rings: 50m (75 dB), 100m (70 dB), 200m (65 dB), 400m (60 dB)
   - Account for traffic volume and speed
   
2. ❌ **Surface Analysis** - **MISSING** - Terrain effects on noise propagation
   - Slope calculations
   - Barrier analysis (buildings, noise walls)
   
3. ❌ **3D Line of Sight** - **MISSING** - Check for sound barriers
   - Buildings blocking direct path
   - Noise walls and berms
   
4. ✅ `ST_INTERSECTS` - Identify affected buildings
   - ThemisDB supports this
   
5. ❌ **Cost Distance** - **MISSING** - Weighted distance accounting for terrain

**ArcGIS Pro Workflow:**
```python
# Calculate noise levels with terrain consideration
noise_surface = arcpy.sa.PathDistance(
    road_centerlines,
    in_surface_raster=dem,
    vertical_factor="BINARY 1 -30 30"  # Slope attenuation
)

# Apply noise formula: L = L0 - 10*log10(d/d0) - barrier_reduction
noise_db = 85 - 10 * arcpy.sa.Log10(noise_surface / 10) - barrier_effect

# Find affected residential areas
affected_buildings = arcpy.analysis.Clip(
    buildings,
    arcpy.sa.Con(noise_db > 65, 1)  # 65 dB threshold
)
```

**ThemisDB Current Capability:** ⚠️ **Limited**
- Can do basic distance calculations
- Missing terrain analysis and 3D considerations
- **GPU opportunity**: Parallel noise propagation calculations

### 4.3 Water Protection Zones (Wasserschutz)

**Regulatory Context:** German WHG (Wasserhaushaltsgesetz), Drinking Water Directive

#### Use Case 4: Wellhead Protection Area Delineation
**Goal:** Define protection zones around drinking water wells

**Required Functions:**
1. ✅ `ST_BUFFER` - **MISSING** - Create protection zones
   - Zone I: 10m radius (immediate protection)
   - Zone II: 50-day travel time to well (requires flow modeling)
   - Zone III: Entire catchment area
   
2. ❌ **Flow Direction/Accumulation** - **MISSING** - Determine groundwater flow
   - DEM-based surface water flow analysis
   - Hydraulic modeling for subsurface
   
3. ✅ `ST_INTERSECTS` - Identify potential contamination sources
   - Gas stations, industrial facilities, agriculture
   - ThemisDB supports this
   
4. ❌ **Watershed Delineation** - **MISSING** - Define catchment boundaries

**ArcGIS Pro Workflow:**
```python
# Delineate watershed for Zone III
filled_dem = arcpy.sa.Fill(dem)
flow_direction = arcpy.sa.FlowDirection(filled_dem)
watershed = arcpy.sa.Watershed(flow_direction, pour_points=wells)

# Simple Zone I (10m buffer)
zone_1 = arcpy.analysis.Buffer(wells, "10 Meters")

# Zone II approximation using travel time
zone_2 = arcpy.sa.CostDistance(
    wells,
    cost_raster=permeability_raster,
    maximum_distance=50_day_travel_distance
)
```

**ThemisDB Current Capability:** ⚠️ **Very Limited**
- Can create simple circular buffers (when ST_BUFFER implemented)
- No flow analysis or watershed delineation
- **Medium priority** - specialized use case

### 4.4 Industrial Facility Siting

**Regulatory Context:** TA Luft, BImSchG, Seveso III Directive

#### Use Case 5: Safety Distance Analysis for Seveso Plants
**Goal:** Ensure adequate separation from residential areas

**Required Functions:**
1. ✅ `ST_BUFFER` - **MISSING** - Create consultation/safety zones
   - Lower-tier: 200m consultation zone
   - Upper-tier: 500m consultation zone, larger for domino effects
   
2. ✅ `ST_INTERSECTS` - Check for residential/sensitive areas
   - Schools, hospitals, residential zones
   - ThemisDB supports this
   
3. ❌ **Viewshed** - **MISSING** - Visual impact assessment
   
4. ✅ `ST_AREA` - Calculate affected land area
   - ThemisDB supports this
   
5. ❌ **Network Analysis** - Emergency response routing
   - Fire brigade access
   - Evacuation routes

**ThemisDB Current Capability:** ⚠️ **Partial**
- Can do basic proximity checks (once ST_BUFFER implemented)
- Missing advanced safety analysis
- **Medium priority**

---

## 5. GPU/VRAM Acceleration Opportunities

### 5.1 High-Value GPU Operations

#### 5.1.1 Buffer Operations ⭐⭐⭐ (Highest Priority)
**Why GPU-suitable:**
- Embarrassingly parallel - each point processed independently
- Fixed-size output per input feature
- Minimal inter-thread communication

**Performance Gain:** 10-100x on large datasets (>100K features)

**Implementation Strategy:**
```cpp
// CUDA kernel for parallel buffering
__global__ void bufferKernel(
    const Point* input_points,      // N points
    const double* buffer_distances,  // N distances (or scalar)
    Polygon* output_buffers,        // N polygons
    int n_points,
    int n_segments_per_circle       // e.g., 32
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n_points) return;
    
    // Generate circle vertices in parallel
    double cx = input_points[idx].x;
    double cy = input_points[idx].y;
    double radius = buffer_distances[idx];
    
    for (int i = 0; i < n_segments_per_circle; i++) {
        double angle = 2.0 * M_PI * i / n_segments_per_circle;
        output_buffers[idx].vertices[i] = Point{
            cx + radius * cos(angle),
            cy + radius * sin(angle)
        };
    }
}
```

**Use Cases:**
- ✅ Protection zone creation (emission, noise, water)
- ✅ Service area approximation
- ✅ Visibility buffers

**ArcGIS Pro Implementation:** GPU-accelerated in ArcGIS Pro 3.0+

#### 5.1.2 Viewshed Analysis ⭐⭐⭐ (Critical for Emissions)
**Why GPU-suitable:**
- Ray-tracing algorithm - highly parallelizable
- Each pixel checked independently
- Texture memory access patterns (DEM as texture)

**Performance Gain:** 20-200x on high-resolution DEMs

**Implementation Strategy:**
```cpp
// CUDA kernel for parallel viewshed
__global__ void viewshedKernel(
    const float* dem,              // Elevation raster
    int width, int height,
    float observer_x, float observer_y, float observer_z,
    float max_distance,
    uint8_t* visibility_mask       // Output: 1=visible, 0=not
) {
    int px = blockIdx.x * blockDim.x + threadIdx.x;
    int py = blockIdx.y * blockDim.y + threadIdx.y;
    if (px >= width || py >= height) return;
    
    // Bresenham line from observer to target pixel
    float target_z = dem[py * width + px];
    bool visible = checkLineOfSight(
        observer_x, observer_y, observer_z,
        px, py, target_z,
        dem, width, height
    );
    
    visibility_mask[py * width + px] = visible ? 1 : 0;
}
```

**Use Cases:**
- ✅ Emission stack visibility for dispersion modeling
- ✅ Sensor placement optimization
- ✅ Visual impact assessment

**ArcGIS Pro Implementation:** GPU-accelerated since ArcGIS Pro 2.8

#### 5.1.3 Kriging Interpolation ⭐⭐ (Important for Monitoring)
**Why GPU-suitable:**
- Matrix operations (semivariogram, kriging matrix)
- Prediction at each raster cell independent
- Batch processing of multiple interpolations

**Performance Gain:** 5-50x depending on grid resolution

**Implementation Strategy:**
```cpp
// CUDA kernel for ordinary kriging prediction
__global__ void krigingKernel(
    const Point* sample_locations,  // N observation points
    const float* sample_values,     // N measurements
    const float* kriging_weights,   // Pre-computed weights (NxM matrix)
    int n_samples,
    float* output_grid,             // MxN output raster
    int grid_width, int grid_height
) {
    int gx = blockIdx.x * blockDim.x + threadIdx.x;
    int gy = blockIdx.y * blockDim.y + threadIdx.y;
    if (gx >= grid_width || gy >= grid_height) return;
    
    // Weighted sum of nearby samples
    float prediction = 0.0f;
    for (int i = 0; i < n_samples; i++) {
        float weight = kriging_weights[i * (grid_width * grid_height) + gy * grid_width + gx];
        prediction += weight * sample_values[i];
    }
    
    output_grid[gy * grid_width + gx] = prediction;
}
```

**Use Cases:**
- ✅ Air quality concentration mapping
- ✅ Noise level interpolation
- ✅ Groundwater contamination mapping

**ArcGIS Pro Implementation:** GPU-accelerated in Geostatistical Analyst extension

#### 5.1.4 Spatial Overlay Operations ⭐⭐
**Why GPU-suitable:**
- Polygon-polygon intersections can be parallelized
- Clip/mask operations work on rasters efficiently
- Batch processing of overlay candidates

**Performance Gain:** 3-20x for complex polygon overlays

**Use Cases:**
- ✅ Clip features to study area
- ✅ Intersect emission zones with land use
- ✅ Union of multiple protection zones

### 5.2 Limited GPU Value Operations

#### Not Suitable for GPU ❌
- **Topology validation** - Sequential graph traversal
- **Network analysis** - Dijkstra's algorithm (sequential dependencies)
- **Coordinate transformation** - Math-heavy but not data-heavy
- **Centroid calculation** - Simple reduce operation, overhead > benefit

---

## 6. Implementation Roadmap

> **Scope Clarification:** This roadmap focuses on database-layer functionality. Complex analysis workflows (dispersion modeling, geostatistical optimization, network analysis) remain application responsibilities.

### Phase 1: Core Database Functions (Q1 2026)

**Priority 1: ST_BUFFER Implementation** 🔴 **[Database Core]**
- **Scope:** Geometric buffer operation for spatial queries
- **Impact:** Unlocks 80% of emission protection spatial queries
- **Complexity:** Medium (planar), High (geodesic)
- **GPU:** Yes - implement GPU kernel for large datasets
- **Dependencies:** None
- **Estimated Effort:** 3-4 weeks
  - 1 week: Planar buffer (Euclidean)
  - 1 week: Geodesic buffer (great circle)
  - 1 week: GPU kernel
  - 1 week: Testing and optimization
- **Application Use:** Applications query buffered zones for coverage analysis, protection zones, proximity queries

**Priority 2: ST_ISVALID / ST_MAKEVALID** 🔴 **[Database Core]**
- **Scope:** Geometry validation and repair
- **Impact:** Data quality assurance for emission calculations
- **Complexity:** Medium
- **GPU:** No
- **Dependencies:** None
- **Estimated Effort:** 1-2 weeks
- **Application Use:** Ensure data integrity before applications perform analysis

**Priority 3: ST_TRANSFORM (Coordinate Reprojection)** 🔴 **[Database Core]**
- **Scope:** Convert geometries between coordinate reference systems
- **Impact:** Essential for working with official datasets (ETRS89, UTM)
- **Complexity:** High (leverage PROJ library)
- **GPU:** Partial (batch transformation)
- **Dependencies:** PROJ library integration
- **Estimated Effort:** 2-3 weeks
- **Application Use:** Applications work with data in consistent CRS

### Phase 2: Spatial Query Enhancements (Q2 2026)

**Priority 4: DEM Raster Storage & Queries** 🟡 **[Database Core]**
- **Scope:** Store and query digital elevation models efficiently
- **Impact:** Enables terrain-aware applications
- **Complexity:** Medium
- **GPU:** Yes - raster operations
- **Dependencies:** Raster storage backend
- **Estimated Effort:** 2-3 weeks
- **Application Use:** Applications query DEM for viewshed, slope, aspect calculations

**Priority 5: ST_DIFFERENCE, ST_SYMDIFFERENCE** 🟡 **[Database Core]**
- **Scope:** Complete set of geometric overlay operations
- **Impact:** Enable complex spatial queries
- **Complexity:** Medium
- **GPU:** Yes
- **Dependencies:** None
- **Estimated Effort:** 2-3 weeks

**Priority 6: Spatial Statistics Functions** 🟡 **[Database Support]**
- **Scope:** Basic statistical queries (mean center, standard distance)
- **Impact:** Support simple spatial analysis
- **Complexity:** Low
- **GPU:** Partial
- **Estimated Effort:** 1-2 weeks
- **Application Use:** Applications get aggregated spatial statistics for decision making

### Phase 3: Advanced Storage (Q3 2026)

**Priority 7: Topology Support** 🟢 **[Database Core]**
- **Scope:** Store and maintain topological relationships
- **Impact:** Efficient network storage, polygon adjacency
- **Complexity:** High
- **GPU:** No
- **Estimated Effort:** 4-5 weeks

**Priority 8: 3D Geometry Support Enhancement** 🟢 **[Database Core]**
- **Scope:** Full 3D operations (3D buffer, 3D intersection)
- **Complexity:** High
- **GPU:** Yes
- **Estimated Effort:** 3-4 weeks

### Phase 4: Performance Optimization (Q4 2026)

**Priority 9: Spatial Index Optimization** 🔵 **[Database Core]**
- **Scope:** R*-Tree implementation, adaptive index tuning
- **Complexity:** Medium
- **Estimated Effort:** 2-3 weeks

**Priority 10: GPU Batch Operations** 🔵 **[Database Core]**
- **Scope:** Batch multiple spatial operations for GPU efficiency
- **Complexity:** Medium
- **GPU:** Yes - core focus
- **Estimated Effort:** 3-4 weeks

### Explicitly Out of Scope (Application Layer)

These remain application responsibilities, not database features:

- ❌ **Viewshed/Visibility Analysis** - Ray-tracing algorithm (application computes using DEM from DB)
- ❌ **Kriging/Geostatistical Interpolation** - Statistical modeling (application: R/Python with data from DB)
- ❌ **Network Analysis** - Routing algorithms (application uses topology stored in DB)
- ❌ **Dispersion Modeling** - Domain-specific calculations (application: AUSTAL2000, LASAT)
- ❌ **Optimization Problems** - Location-allocation, facility siting (application: OR tools)
- ❌ **Complex Workflows** - Multi-step analysis pipelines (application orchestrates DB queries)
  - 1 week: GPU implementation
  - 1 week: Cross-validation
  - 1 week: Testing with air quality data

**Priority 6: IDW Interpolation** 🟡
- **Impact:** Simpler alternative to kriging
- **Complexity:** Low
- **GPU:** Yes - embarrassingly parallel
- **Dependencies:** None
- **Estimated Effort:** 1-2 weeks

### Phase 3: Advanced Analysis (Q3 2026)

**Priority 7: Surface Analysis Tools** 🟢
- Slope, Aspect, Hillshade
- **Complexity:** Medium
- **GPU:** Yes - convolution operations
- **Estimated Effort:** 3-4 weeks

**Priority 8: Kernel Density Analysis** 🟢
- Heatmap generation for emission sources
- **Complexity:** Medium
- **GPU:** Yes
- **Estimated Effort:** 2-3 weeks

**Priority 9: Cost Distance Analysis** 🟢
- Weighted distance for noise propagation
- **Complexity:** High
- **GPU:** Limited (dynamic programming)
- **Estimated Effort:** 3-4 weeks

### Phase 4: Advanced Features (Q4 2026)

**Priority 10: Network Analysis** 🔵
- Routing for emergency response
- Service area analysis
- **Complexity:** Very High
- **GPU:** No (graph algorithms)
- **Estimated Effort:** 6-8 weeks

---

## 7. References

### Academic Literature
1. Haverkort, H., & van Walderveen, F. (2008). "Locality and bounding-box quality of two-dimensional space-filling curves." *Computational Geometry*, 43(2), 131-147.
2. De Floriani, L., & Magillo, P. (2003). "Algorithms for visibility computation on terrains: a survey." *Environment and Planning B*, 30(5), 709-728.
3. Cressie, N. A. (1993). *Statistics for Spatial Data* (Revised ed.). Wiley.

### Standards & Regulations
- **OGC Simple Features Specification** - OpenGIS Implementation Standard for Geographic Information
- **ISO 19107:2019** - Geographic information — Spatial schema
- **EU Air Quality Directive 2008/50/EC**
- **German BImSchG** (Bundes-Immissionsschutzgesetz)
- **TA Luft 2021** - Technical Instructions on Air Quality Control
- **EU Environmental Noise Directive 2002/49/EC**
- **Seveso III Directive 2012/18/EU**

### Software Documentation
- **ArcGIS Pro 3.x Documentation**: https://pro.arcgis.com/
- **PostGIS 3.x Manual**: https://postgis.net/docs/
- **GDAL/OGR Documentation**: https://gdal.org/
- **PROJ Coordinate Transformation**: https://proj.org/

### GPU Computing
- NVIDIA CUDA Toolkit Documentation
- Vulkan Compute Specification
- OpenCL for Geospatial Processing (FOSS4G papers)

---

## Appendix A: Sample Code Snippets

### A.1 ThemisDB Buffer Query (Once Implemented)

```sql
-- AQL: Create 500m protection zone around emission source
FOR source IN emission_sources
  FILTER source.facility_type == "Industrial"
  LET buffer = ST_BUFFER(source.location, 500)  -- 500m radius
  LET affected = (
    FOR building IN buildings
      FILTER ST_INTERSECTS(building.geometry, buffer)
      RETURN building
  )
  RETURN {
    source: source.name,
    protection_zone: buffer,
    affected_buildings: affected
  }
```

### A.2 ThemisDB Kriging Query (Once Implemented)

```sql
-- AQL: Interpolate PM10 concentrations across study area
LET monitoring_data = (
  FOR sensor IN air_quality_sensors
    FILTER sensor.pollutant == "PM10"
    RETURN {
      location: sensor.location,
      value: sensor.last_measurement
    }
)

LET concentration_surface = ST_KRIGING(
  monitoring_data,
  {
    method: "ordinary",
    model: "spherical",
    grid_resolution: 100,  -- 100m grid
    bounds: study_area_bbox
  }
)

-- Find areas exceeding 40 µg/m³ (annual limit)
LET exceedance_areas = ST_RASTER_WHERE(
  concentration_surface,
  concentration_surface > 40
)

RETURN {
  interpolated_surface: concentration_surface,
  exceedance_zones: exceedance_areas,
  max_concentration: ST_RASTER_MAX(concentration_surface)
}
```

### A.3 ThemisDB Viewshed Query (Once Implemented)

```sql
-- AQL: Determine visibility from emission stack
LET stack_location = {
  x: 8.5, y: 50.0, z: 50  -- 50m stack height
}

LET visibility = ST_VIEWSHED(
  dem_raster,
  stack_location,
  {
    max_distance: 5000,  -- 5km radius
    z_factor: 1.0
  }
)

-- Find residential areas in visible zone
LET affected_residential = (
  FOR zone IN land_use
    FILTER zone.type == "residential"
    FILTER ST_INTERSECTS_RASTER(zone.geometry, visibility)
    RETURN zone
)

RETURN {
  visibility_map: visibility,
  affected_areas: affected_residential
}
```

---

## Appendix B: GPU Performance Benchmarks (Projected)

Based on comparable implementations in ArcGIS Pro and academic literature:

| Operation | Dataset Size | CPU Time | GPU Time | Speedup |
|-----------|--------------|----------|----------|---------|
| ST_BUFFER | 100K points | 45 sec | 1.2 sec | 37x |
| ST_BUFFER | 1M points | 480 sec | 8 sec | 60x |
| Viewshed | 2048x2048 DEM | 120 sec | 2.5 sec | 48x |
| Viewshed | 8192x8192 DEM | 1800 sec | 18 sec | 100x |
| Kriging | 1000 samples, 1024x1024 | 180 sec | 12 sec | 15x |
| IDW | 1000 samples, 1024x1024 | 30 sec | 1.5 sec | 20x |
| Kernel Density | 500K points | 90 sec | 3 sec | 30x |

**Hardware Assumptions:**
- CPU: 8-core x86_64 @ 3.0 GHz
- GPU: NVIDIA RTX 4070 (5888 CUDA cores, 12GB VRAM)
- RAM: 32GB DDR4

---

## Appendix C: Emission Protection Calculation Examples

### C.1 TA Luft 2021 - Minimum Stack Height

According to TA Luft 2021, Section 5.5:

```
H = 10 * (V_dot)^0.3 + ΔH

Where:
- H = effective emission height (m)
- V_dot = exhaust gas volume flow (m³/s)
- ΔH = plume rise (m)
```

**Geospatial Analysis:**
1. Use ST_BUFFER to create consultation zones
2. Use ST_INTERSECTS to check for sensitive receptors
3. Use viewshed to validate visibility/impact

### C.2 EU Air Quality - PM10 Assessment

**Annual limit value:** 40 µg/m³  
**Daily limit value:** 50 µg/m³ (not to be exceeded >35 days/year)

**Geospatial Workflow:**
1. Collect monitoring data from sensors (point features)
2. Use kriging to interpolate concentration surface
3. Use ST_RASTER_WHERE to identify exceedance areas
4. Calculate affected population using intersection with census data

---

**Document Status:** Draft v1.0  
**Next Review:** Q2 2026 (after Phase 1 implementation)  
**Feedback:** Please submit to ThemisDB GitHub Issues
