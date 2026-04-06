# Geo Architecture - Cross-Cutting Capability

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Geo

---


**Erstellt:** 17. November 2025  
**Status:** Phase 0.1 implementiert (EWKB Storage + Sidecar)  
**Design:** Geo als Cross-Cutting Infrastructure für alle 5 Datenbank-Modelle

---

## 1. Architektur-Prinzip

**Geo ist KEIN separates Datenbank-Modell**, sondern eine **optionale Cross-Cutting Capability** für alle 5 Modelle:

```
┌─────────────────────────────────────────────────────────────┐
│                   ThemisDB Multi-Model                       │
├─────────────────────────────────────────────────────────────┤
│  Relational  │  Graph  │  Vector  │  Content  │ Time-Series │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│      GEO CROSS-CUTTING CAPABILITY (Optional)                 │
│      - EWKB Storage (geometry_ field in BaseEntity)          │
│      - R-Tree Spatial Index (table-agnostic)                 │
│      - ST_* Functions (universal AQL extensions)             │
│      - Query Optimizer (spatial selectivity)                 │
│                                                               │
├─────────────────────────────────────────────────────────────┤
│              RocksDB Blob Storage Layer                       │
└─────────────────────────────────────────────────────────────┘
```

**Vorteile:**
- ✅ **Jedes Modell** kann geo-enabled sein (optional `geometry_` field)
- ✅ **Gemeinsamer** R-Tree Index für alle Tabellen
- ✅ **Universelle** ST_* Functions in AQL
- ✅ **Symbiotische** Beziehungen zwischen Modellen

---

## 2. Storage Layer

### 2.1 EWKB Format (Extended Well-Known Binary)

**Standard:** PostGIS-kompatibles EWKB/EWKBZ Format

```cpp
// Geometry Types
enum class GeometryType : uint32_t {
    Point = 1,
    LineString = 2,
    Polygon = 3,
    // 3D variants (Z flag: type | 0x80000000)
    PointZ = 0x80000001,
    LineStringZ = 0x80000002,
    PolygonZ = 0x80000003,
    // MultiPoint, MultiLineString, etc.
};
```

**EWKB Binary Layout:**
```
┌─────────────┬─────────────┬──────────┬────────────────┐
│ Byte Order  │ Type (4B)   │ SRID(4B) │ Coordinates... │
│ (1 byte)    │ +Z flag     │ optional │ (8 bytes/coord)│
└─────────────┴─────────────┴──────────┴────────────────┘
```

**Beispiel: Point(13.4, 52.5) - Berlin**
```
0x01                                // Little Endian
0x01 0x00 0x00 0x00                // Type: Point
0xCD 0xCC 0xCC 0xCC 0xCC 0xCC 0x2A 0x40  // X: 13.4
0x00 0x00 0x00 0x00 0x00 0x40 0x4A 0x40  // Y: 52.5
```

### 2.2 BaseEntity Integration

**Optional Geometry Field:**
```cpp
class BaseEntity {
    // Existing fields
    std::string primary_key_;
    Blob blob_;                              // Relational/Graph/Vector/Content data
    
    // GEO: Optional geometry field (cross-cutting)
    std::optional<Blob> geometry_;           // EWKB blob
    std::optional<geo::GeoSidecar> geo_sidecar_;  // Fast filtering metadata
};
```

**Design Rationale:**
- ✅ **Optional:** Entities without geometry have zero overhead
- ✅ **Separate:** Geometry independent from main blob (different access patterns)
- ✅ **Automatic:** Sidecar computed on `setGeometry()`

### 2.3 GeoSidecar (Fast Filtering)

**Metadata for Spatial Index:**
```cpp
struct GeoSidecar {
    MBR mbr;              // 2D bounding box (minx, miny, maxx, maxy)
    Coordinate centroid;  // Geometric center
    double z_min = 0.0;   // Min elevation (for 3D)
    double z_max = 0.0;   // Max elevation (for 3D)
};
```

**Purpose:**
- **Broadphase:** R-Tree uses MBR for fast spatial queries
- **3D Filtering:** Z-Range index for elevation queries
- **Centroid:** Distance calculations, visual representation

**Storage:**
```
RocksDB Column Families:
  data:<table>:<pk> → BaseEntity blob
  geo:<table>:<pk>  → EWKB + GeoSidecar (separate CF for cache locality)
```

---

## 3. Index Layer

### 3.1 R-Tree Spatial Index (Table-Agnostic)

**Design:** Single R-Tree implementation works for ALL 5 models

```cpp
class SpatialIndexManager {
public:
    // Create spatial index for ANY table
    Status createSpatialIndex(
        std::string_view table,     // "cities", "locations", "images", etc.
        std::string_view geometry_column = "geometry",
        const RTreeConfig& config = {}
    );
    
    // Query (returns PKs, model-agnostic)
    std::vector<std::string> searchIntersects(
        std::string_view table,
        const MBR& query_bbox
    );
};
```

**RocksDB Key Schema:**
```
# Table-agnostic design:
spatial:<table>:<morton_code> → list<PK>

# Examples for different models:
spatial:cities:12345678 → ["cities/berlin", "cities/munich"]        # Relational
spatial:locations:23456789 → ["locations/loc1", "locations/loc2"]   # Graph nodes
spatial:images:34567890 → ["images/img1", "images/img2"]            # Vector entities
spatial:documents:45678901 → ["content/doc1", "content/doc2"]       # Content
spatial:sensors:56789012 → ["sensors/temp1", "sensors/temp2"]       # Time-Series
```

**Morton Codes (Z-Order):**
- Interleave X/Y coordinates → single 64-bit integer
- Preserves spatial locality in 1D key space
- Efficient range queries in RocksDB

### 3.2 Z-Range Index (3D Elevation)

**For 3D Geometries:**
```
# Composite index on z_min/z_max:
zrange:<table>:<z_bucket> → list<PK>

# Example: Find entities between elevation 50-150m
zrange:locations:050 → ["loc1", "loc2", ...]
zrange:locations:100 → ["loc3", "loc4", ...]
zrange:locations:150 → ["loc5", ...]
```

---

## 4. Query Layer

### 4.1 ST_* Functions (Universal)

**17 Core Functions (alle Modelle):**

**Constructors:**
```sql
ST_Point(lon DOUBLE, lat DOUBLE, z DOUBLE = NULL) -> GEOMETRY
ST_GeomFromGeoJSON(json STRING) -> GEOMETRY
ST_GeomFromText(wkt STRING) -> GEOMETRY
```

**Converters:**
```sql
ST_AsGeoJSON(geom GEOMETRY) -> STRING
ST_AsText(geom GEOMETRY) -> STRING
ST_Envelope(geom GEOMETRY) -> GEOMETRY  -- Returns MBR as polygon
```

**Predicates (2D + 3D):**
```sql
ST_Intersects(geom1 GEOMETRY, geom2 GEOMETRY) -> BOOL
ST_Within(geom1 GEOMETRY, geom2 GEOMETRY) -> BOOL
ST_Contains(geom1 GEOMETRY, geom2 GEOMETRY) -> BOOL
```

**Distance (Haversine for geodetic):**
```sql
ST_Distance(geom1 GEOMETRY, geom2 GEOMETRY) -> DOUBLE
ST_DWithin(geom1 GEOMETRY, geom2 GEOMETRY, distance DOUBLE) -> BOOL
ST_3DDistance(geom1 GEOMETRY, geom2 GEOMETRY) -> DOUBLE
```

**3D Helpers:**
```sql
ST_HasZ(geom GEOMETRY) -> BOOL
ST_Z(geom GEOMETRY) -> DOUBLE
ST_ZMin(geom GEOMETRY) -> DOUBLE
ST_ZMax(geom GEOMETRY) -> DOUBLE
ST_Force2D(geom GEOMETRY) -> GEOMETRY
ST_Force3D(geom GEOMETRY, z DOUBLE = 0.0) -> GEOMETRY
ST_ZBetween(geom GEOMETRY, z_min DOUBLE, z_max DOUBLE) -> BOOL
```

### 4.2 Spatial Execution Plan

**Model-Agnostic Execution:**
```cpp
// Step 1: Parse spatial predicate
ST_Intersects(entity.geometry, @viewport)

// Step 2: Extract MBR from @viewport
MBR query_bbox = extractMBR(@viewport);

// Step 3: Broadphase - R-Tree scan
std::vector<std::string> candidates = spatial_index.searchIntersects(table, query_bbox);

// Step 4: Z-Filter (optional for 3D)
if (query.has_z_constraint) {
    candidates = z_index.filterByRange(candidates, z_min, z_max);
}

// Step 5: Load entities
std::vector<BaseEntity> entities = loadEntities(candidates);

// Step 6: Exact check (Boost.Geometry)
for (auto& entity : entities) {
    if (exact_intersects(entity.geometry, viewport)) {
        results.push_back(entity);
    }
}

// Step 7: Apply additional filters (population, type, etc.)
```

### 4.3 Query Optimizer (Spatial Selectivity)

**Cost-Based Decision:**
```cpp
struct SpatialSelectivity {
    double area_ratio;      // query_bbox_area / total_area
    double density;         // avg entities per unit area
    int estimated_hits;     // from R-Tree stats
};

// Decision logic (universal for all models):
if (spatial_selectivity < 0.01) {
    plan = SPATIAL_FIRST;  // Geo filter -> other filters
} else {
    plan = FILTER_FIRST;   // Other filters -> geo filter
}
```

**Example:**
```sql
-- Query with high spatial selectivity (small area)
FOR city IN cities
  FILTER city.population > 100000           -- Low selectivity (many cities)
    AND ST_Intersects(city.boundary, @viewport)  -- High selectivity (small area)
  RETURN city

-- Optimizer chooses: SPATIAL_FIRST
-- 1. R-Tree scan (viewport) -> 10 candidates
-- 2. Filter by population -> 7 results
```

---

## 5. Symbiose mit allen 5 Modellen

| Modell | Geo-Enabled Use Cases | Geo profitiert von | Geo enhances |
|--------|----------------------|-------------------|--------------|
| **Relational** | Spatial joins, WHERE + ST_Intersects | Secondary Indexes (country, type) | Attribute + Location queries |
| **Graph** | Spatial graph traversal (road networks) | Edge connectivity, routing | Geo-constrained path finding |
| **Vector** | Spatial-filtered ANN (location + similarity) | HNSW whitelist/mask | Pre-filtered vector search |
| **Content** | Geo-tagged documents/chunks | Fulltext search | Location-based RAG |
| **Time-Series** | Geo-temporal queries (trajectories) | Timestamp evolution | Sensor location tracking |

### 5.1 Relational + Geo

**Use Case: Spatial Join**
```sql
FOR city IN cities
  FILTER city.population > 100000 
    AND city.country == 'Germany'
    AND ST_Intersects(city.boundary, @viewport)
  RETURN city
```

**Benefits:**
- Combine attribute filters (population, country) with spatial filters
- Secondary indexes for attributes + R-Tree for geometry
- Hybrid query optimization

### 5.2 Graph + Geo

**Use Case: Spatial Graph Traversal**
```sql
FOR v IN 1..5 OUTBOUND 'locations/berlin' GRAPH 'roads'
  FILTER ST_DWithin(v.location, @center, 5000)  -- Within 5km
  RETURN v
```

**Benefits:**
- Geo-constrained BFS (prune frontier by location)
- Road network routing with spatial bounds
- Graph connectivity + spatial proximity

### 5.3 Vector + Geo

**Use Case: Spatial-Filtered ANN**
```sql
FOR img IN images
  FILTER ST_Within(img.location, @region)
  SORT SIMILARITY(img.embedding, @query) DESC
  LIMIT 10
  RETURN img
```

**Benefits:**
- Pre-filter HNSW with spatial whitelist (Roaring bitmap)
- Location + Visual Similarity hybrid search
- Efficient for location-based image search

### 5.4 Content + Geo

**Use Case: Location-Based RAG**
```sql
FOR doc IN documents
  FILTER FULLTEXT(doc.text, "hotel")
    AND ST_DWithin(doc.location, @myLocation, 2000)  -- 2km radius
  RETURN doc
```

**Benefits:**
- Geo-tagged documents/chunks
- Fulltext + Location hybrid ranking
- "Find hotels near me" queries

### 5.5 Time-Series + Geo

**Use Case: Geo-Temporal Queries**
```sql
FOR reading IN sensor_data
  FILTER reading.timestamp > @start
    AND ST_Contains(@area, reading.sensor_location)
  RETURN reading
```

**Benefits:**
- Sensor trajectories, IoT device tracking
- Timestamp + Location evolution
- Geo-fencing for time-series data

**Temporal-Spatial Query API (C++):**

The `TemporalSpatialQuery` class (see `include/geo/temporal_spatial_query.h`) bridges the
`SystemVersionedTable` temporal versioning layer with geospatial queries.  Every entity row
stores a GeoJSON geometry string in its `data["location"]` field (or a custom field name).

```cpp
#include "geo/temporal_spatial_query.h"
#include "temporal/system_versioned_table.h"
using namespace themis::geo;
using namespace themisdb::temporal;

SystemVersionedTable vehicles{"vehicles", "node_a"};
// ... insert/update rows with { "location": "{\"type\":\"Point\",\"coordinates\":[lon,lat]}" }

// Q1: Where was bus1 at time T?
auto geom = TemporalSpatialQuery::locationAtTime(vehicles, "bus1", t);

// Q2: All vehicles alive at time T (returns vector of (key, geometry) pairs)
auto all = TemporalSpatialQuery::allLocationsAtTime(vehicles, t);

// Q3: Vehicles inside Germany bounding box at time T
MBR germany{5.8, 47.2, 15.1, 55.1};
auto in_germany = TemporalSpatialQuery::entitiesInBBoxAtTime(vehicles, germany, t);

// Q4: Vehicles within 5 km of Berlin Mitte at time T, sorted by distance
auto nearby = TemporalSpatialQuery::entitiesWithinDistanceAtTimeSorted(
    vehicles, 13.4050, 52.5200, 5000.0, t);
```

---

## 6. Implementation Status

### ✅ Phase 0.1: EWKB Storage + Sidecar (COMPLETED)

**Files:**
- `include/utils/geo/ewkb.h` (200 lines)
- `src/utils/geo/ewkb.cpp` (400 lines)
- `include/storage/base_entity.h` (extended with geometry fields)
- `src/storage/base_entity.cpp` (geo methods)
- `tests/geo/test_geo_ewkb.cpp` (18 unit tests)

**Features:**
- ✅ EWKB/EWKBZ Parser/Serializer
- ✅ GeometryType: Point, LineString, Polygon (2D/3D)
- ✅ MBR computation (2D + Z-Range)
- ✅ Centroid computation
- ✅ GeoSidecar auto-computation
- ✅ GeoJSON/WKT parsing (partial)
- ✅ BaseEntity integration (optional `geometry_`, `geo_sidecar_`)

**Tests:**
- ✅ Point 2D/3D parsing
- ✅ LineString parsing
- ✅ Polygon parsing
- ✅ MBR computation (2D + 3D)
- ✅ Centroid computation
- ✅ Sidecar computation
- ✅ MBR intersection/contains
- ✅ GeoJSON parsing
- ✅ Round-trip (serialize → parse)
- ✅ EWKB validation

### ✅ Phase 0.2: R-Tree Spatial Index (COMPLETED)

**Files:**
- `include/index/spatial_index.h` (200+ lines)
- `src/index/spatial_index.cpp` (600+ lines)
- `tests/geo/test_spatial_index.cpp` (350+ lines, 13 tests)

**Features:**
- ✅ **Table-Agnostic Design:** Funktioniert für alle 5 Modelle (Relational, Graph, Vector, Content, Time-Series)
- ✅ **Morton Code Z-Order:** Spatial locality preservation in 1D RocksDB key space
- ✅ **SpatialIndexManager API:**
  - `createSpatialIndex(table, column, config)` - Für beliebige Tabelle
  - `searchIntersects(table, bbox)` - MBR intersection queries
  - `searchWithin(table, bbox, z_min, z_max)` - Containment + 3D filtering
  - `searchContains(table, x, y, z)` - Point-in-polygon broadphase
  - `searchNearby(table, x, y, max_distance)` - Distance-based (Haversine)
  - `searchKNN(table, x, y, k)` - K-nearest neighbors (spatial only)
- ✅ **RocksDB Key Schema:** `spatial:<table>:<morton_code> → list<PK + Sidecar>`
- ✅ **Multi-Table Support:** Separate indexes for cities, locations, images, documents
- ✅ **CRUD Operations:** Insert, Update, Remove with Morton bucket management
- ✅ **Index Statistics:** Entry count, Morton buckets, average MBR area

**Tests:**
- ✅ Create/Drop spatial index
- ✅ Insert and search single point
- ✅ Multiple points in same region
- ✅ Search within (strict containment)
- ✅ Search contains point
- ✅ Search nearby (distance-based, sorted by distance)
- ✅ Multi-table support (cities, locations, images, documents isolated)
- ✅ Update location (remove from old bucket, insert to new)
- ✅ Remove entity
- ✅ Index statistics
- ✅ Morton encoder (encode/decode, spatial locality verification)

**Performance:**
- Insert: O(log N) - RocksDB write + JSON list append
- Query: O(log N + K) - RocksDB range scan + MBR filtering
- Memory: ~150 bytes per entry (PK + Sidecar JSON)
- Spatial locality: Morton codes cluster nearby points in same buckets

### 🚧 Phase 0.3: AQL ST_* Functions Parser (IN PROGRESS)

**Next Steps:**
- [ ] Extend AQL parser with GEOMETRY type
- [ ] 17 ST_* functions parsing
- [ ] Function registry for spatial functions
- [ ] Type checking (GEOMETRY vs. other types)
- [ ] Tests: Parser + AST validation

### 🔮 Phase 0.3: AQL ST_* Functions (PLANNED)

**Next Steps:**
- [ ] Extend AQL parser with ST_* functions
- [ ] 17 core functions implementation
- [ ] Function registry
- [ ] Type checking (GEOMETRY type)
- [ ] Tests: Parser + execution

### 🔮 Phase 0.4: Query Engine Integration (PLANNED)

**Next Steps:**
- [ ] Spatial execution plan
- [ ] Query optimizer (spatial selectivity)
- [ ] Cost estimation
- [ ] Metrics (spatial.index_hits, etc.)
- [ ] Tests: End-to-end spatial queries

### ✅ Phase 0.5: Temporal-Spatial Queries (COMPLETED)

**Files:**
- `include/geo/temporal_spatial_query.h`
- `src/geo/temporal_spatial_query.cpp`
- `tests/geo/test_temporal_spatial_query.cpp` (unit tests)

**Features:**
- ✅ `TemporalSpatialQuery::locationAtTime(table, key, as_of)` — entity geometry at point-in-time T
- ✅ `TemporalSpatialQuery::allLocationsAtTime(table, as_of)` — all alive entity geometries at T
- ✅ `TemporalSpatialQuery::entitiesInBBoxAtTime(table, bbox, as_of)` — bounding-box filter at T
- ✅ `TemporalSpatialQuery::entitiesWithinDistanceAtTime(table, lon, lat, dist_m, as_of)` — radius filter at T (Haversine)
- ✅ `TemporalSpatialQuery::entitiesWithinDistanceAtTimeSorted(...)` — radius filter sorted by distance
- ✅ `TemporalSpatialQuery::extractGeometry(doc, field)` — parse GeoJSON from document field

### ✅ Phase 0.6: Spatial JOIN with Configurable Distance Threshold (COMPLETED)

**Files:**
- `include/geo/spatial_join.h`
- `src/geo/spatial_join.cpp`
- `tests/geo/test_geo_spatial_join.cpp` (17 unit tests)

**Features:**
- ✅ `spatialJoin(outer, inner, threshold_m, config)` — finds all pairs (A, B) where geodesic distance ≤ threshold_m
- ✅ `haversineDistanceM(lon1, lat1, lon2, lat2)` — WGS84 great-circle distance helper
- ✅ R-tree index (`GeoRTree`) built on the inner collection for sub-linear candidate lookup
- ✅ Exact Haversine verification after MBR pre-filter (no false negatives)
- ✅ Configurable `max_pairs` limit (default 1 000 000) with structured warning log
- ✅ Non-Point geometries use centroid for distance computation

**API:**
```cpp
#include "geo/spatial_join.h"

std::vector<std::pair<std::string, GeometryInfo>> outer = ...;
std::vector<std::pair<std::string, GeometryInfo>> inner = ...;

// Find all pairs within 1 km
auto pairs = themis::geo::spatialJoin(outer, inner, 1000.0);

// With custom limit
themis::geo::SpatialJoinConfig cfg;
cfg.max_pairs = 10'000;
auto limited = themis::geo::spatialJoin(outer, inner, 500.0, cfg);

for (const auto& p : pairs) {
    // p.key_a, p.key_b: keys from outer/inner
    // p.distance_m: exact Haversine distance in metres
}
```

**Tests (17 total):**
- ✅ Empty outer / empty inner → empty result
- ✅ Zero / negative threshold → empty result (with THEMIS_WARN log)
- ✅ Identical points within threshold
- ✅ Points too far apart for threshold
- ✅ Mixed close + distant candidates (only close ones returned)
- ✅ Multiple outer points with separate inner partners
- ✅ Returned `distance_m` accuracy verified (1° lat ≈ 111 320 m)
- ✅ `max_pairs` enforcement
- ✅ All results satisfy `distance_m ≤ threshold_m` invariant
- ✅ Self-join (same collection as both outer and inner)
- ✅ Non-Point geometry (Polygon) — centroid used for distance

---

## 7. Design Decisions

### 7.1 Why EWKB?

**Alternatives considered:**
- ❌ **GeoJSON text:** Human-readable but inefficient storage (3-5x larger)
- ❌ **Custom binary:** Not interoperable with other systems
- ✅ **EWKB:** Industry standard, compact, PostGIS-compatible

### 7.2 Why Cross-Cutting Capability?

**Alternatives considered:**
- ❌ **Separate Geo Model:** Would duplicate functionality, increase complexity
- ❌ **Only Relational:** Would exclude Graph/Vector/Content use cases
- ✅ **Cross-Cutting:** Enables all 5 models, shared infrastructure, symbiotic benefits

### 7.3 Why Optional Geometry Field?

**Alternatives considered:**
- ❌ **Always present:** Wastes space for non-geo entities (majority)
- ❌ **Separate table:** Breaks entity model, complicates queries
- ✅ **Optional field:** Zero overhead when unused, simple API

### 7.4 Why Boost.Geometry?

**Alternatives considered:**
- ❌ **GEOS:** GPL license (incompatible with commercial use)
- ❌ **Custom implementation:** High complexity, error-prone
- ✅ **Boost.Geometry:** Permissive license (BSL-1.0), header-only, already in project

---

## 8. Performance Considerations

### 8.1 Storage Overhead

**Without Geometry:**
- BaseEntity: ~200 bytes (typical)
- Geo overhead: 0 bytes ✅

**With Geometry (Point):**
- EWKB: ~21 bytes (1 byte order + 4 type + 16 coords)
- GeoSidecar: ~72 bytes (MBR + centroid + z-range)
- Total overhead: ~93 bytes per entity

**With Geometry (Polygon, 100 points):**
- EWKB: ~1621 bytes
- GeoSidecar: ~72 bytes (fixed size)
- Total overhead: ~1693 bytes

### 8.2 Index Performance

**R-Tree Spatial Index:**
- Insert: O(log N)
- Query: O(log N + K) where K = result size
- Memory: ~100 bytes per entry (R-Tree node)

**Morton Code Z-Order:**
- Encoding: O(1) (bitwise operations)
- Range query: O(log N) RocksDB scan
- Better cache locality than naive 2D indexing

### 8.3 Query Performance

**Spatial Query Pipeline:**
```
1. Parse         : ~10 μs   (AQL → AST)
2. R-Tree scan   : ~100 μs  (broadphase, 1M entities)
3. Z-Filter      : ~50 μs   (optional, 3D)
4. Load entities : ~500 μs  (RocksDB, 100 candidates)
5. Exact check   : ~200 μs  (Boost.Geometry, 100 checks)
6. Total         : ~860 μs  (for typical query)
```

**Optimization Opportunities:**
- ⚡ SIMD (Phase 5.1): 3-5x speedup on exact checks
- ⚡ Morton + Roaring (Phase 5.1): Better set algebra
- ⚡ GPU Backend (Phase 5.3): 10-100x for batch queries

---

## 9. Future Extensions

### 9.1 Optional: SIMD Acceleration

**Google Highway (Apache-2.0):**
- `pointInPolygon_simd()`: 3-5x faster
- `bboxOverlap_simd()`: 10x faster (batch processing)
- `haversineDistance_simd()`: 4x faster

### 9.2 Optional: Shapefile/GeoTIFF Import

**Shapefile → Relational Table:**
```bash
POST /api/import/shapefile
{
  "file": "cities.shp",
  "table": "cities",
  "attributes": ["name", "population", "country"]
}
```

**GeoTIFF → Tiles:**
```bash
POST /api/import/geotiff
{
  "file": "elevation.tif",
  "table": "elevation_tiles",
  "tile_size": 256
}
```

### 9.3 Optional: GPU Backend

**Compute Shaders (DX12/Vulkan):**
- Batch ST_Intersects (10k+ geometries): 100x faster
- SoA layout, prefix sum, stream compaction

---

## 10. Summary

**Geo als Cross-Cutting Capability:**
- ✅ **Universal:** Funktioniert für alle 5 Datenbank-Modelle
- ✅ **Optional:** Zero overhead wenn nicht verwendet
- ✅ **Standard:** EWKB/EWKBZ PostGIS-kompatibel
- ✅ **Efficient:** R-Tree + Morton codes für schnelle Queries
- ✅ **Symbiotic:** Jedes Modell profitiert von Geo, Geo profitiert von jedem Modell

**Status:** Phases 0.1, 0.2, 0.5, 0.6 abgeschlossen; Phases 0.3 + 0.4 ausstehend  
**Next:** Phase 0.3 (AQL ST_* Parser Integration) + Phase 0.4 (Query Engine Integration)

---

## See Also

- [Geospatial Module Overview](README.md) - Current implementation
- [Future Enhancements for Geospatial Implementation](../../geospatial_future_enhancements.md) - Planned improvements and roadmap
