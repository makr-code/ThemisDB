> **⚠️ STATUS: STALE – Archivierungskandidat**
> Dieser Inhalt enthält veraltete TODO/FIXME/PLACEHOLDER-Marker und wird im nächsten Archiv-Run nach `docs/ARCHIVED/` verschoben.
> Bitte nicht als aktuelle Referenz nutzen. Inventar: [DOCS_INVENTORY_2026-Q3.md](Audit/DOCS_INVENTORY_2026-Q3.md)

---

# Future Enhancements for Geospatial Implementation

**Status:** Planning  
**Version:** 1.0.0  
**Category:** Geospatial / Roadmap  
**Date:** February 2026

---

## Overview

This document outlines planned enhancements for ThemisDB's geospatial implementation. These improvements will expand functionality, improve performance, and enhance compatibility with industry-standard spatial systems like PostGIS and Oracle Spatial.

**Current Capabilities:**
- R-Tree spatial indexing with Morton encoding
- Basic spatial operations (ST_INTERSECTS, ST_CONTAINS, ST_WITHIN, ST_DISTANCE)
- GeoJSON geometry support in entity storage
- Bounding box (MBR) queries via RPC and HTTP APIs
- GDAL integration for Shapefile and GeoTIFF processing
- CPU and GPU backend support (pluggable architecture)

**Target:** Expand ThemisDB into a comprehensive geospatial database while maintaining its multi-model architecture and performance characteristics.

---

## 1. Full GeoJSON/WKT Parsing

### Current State
- **Limitation:** ThemisDB currently only supports simple bounding box literals in AQL queries using the format `[[minx,miny],[maxx,maxy]]`.
- **Storage:** GeoJSON geometries can be stored in entity blobs but are not parsed or validated at query translation time.
- **Example:** 
  ```aql
  FOR doc IN locations
    FILTER ST_INTERSECTS(doc.geometry, [[13.0, 52.0], [14.0, 53.0]])
    RETURN doc
  ```

### Planned Enhancement
- **Parse complete GeoJSON at translation time:**
  ```aql
  FOR doc IN locations
    FILTER ST_INTERSECTS(doc.geometry, {
      "type": "Polygon",
      "coordinates": [[[13.0, 52.0], [13.5, 52.0], [13.5, 52.5], [13.0, 52.5], [13.0, 52.0]]]
    })
    RETURN doc
  ```

- **Support Well-Known Text (WKT) format:**
  ```aql
  FOR doc IN locations
    FILTER ST_WITHIN(doc.point, 'POLYGON((13.0 52.0, 13.5 52.0, 13.5 52.5, 13.0 52.5, 13.0 52.0))')
    RETURN doc
  ```

- **Complex geometry types:**
  - **Point:** Single coordinate pair
  - **LineString:** Connected line segments
  - **Polygon:** Closed rings with optional holes
  - **MultiPoint:** Collection of points
  - **MultiLineString:** Collection of line strings
  - **MultiPolygon:** Collection of polygons
  - **GeometryCollection:** Mixed geometry types

### Benefits
- **Rich spatial queries:** Support complex geometries directly in AQL without preprocessing
- **Standard compliance:** Full OGC Simple Features compatibility
- **Developer experience:** Natural syntax for geospatial developers familiar with PostGIS/MongoDB
- **Query optimization:** Enable geometry-specific optimizations at translation time

### Requirements
- **Runtime geometry evaluation framework:** Parser must convert GeoJSON/WKT strings into internal geometry representations
- **Integration with AQL translator:** Extend expression parser to recognize and validate geometry literals
- **Geometry validation:** Ensure geometries are valid (e.g., closed rings, no self-intersections)
- **Backward compatibility:** Maintain support for existing bbox literal syntax

### Implementation Considerations
- Use GDAL/OGR for WKT/GeoJSON parsing (already integrated)
- Cache parsed geometries for repeated queries (prepared geometry pattern)
- Consider memory footprint of large polygon literals in queries
- Estimated effort: 3-4 weeks

---

## 2. Advanced Spatial Operations

### Current State
- **Implemented operations:** ST_INTERSECTS, ST_CONTAINS, ST_WITHIN, ST_DISTANCE (Haversine formula for points)
- **Limitation:** No geometry manipulation or advanced topological operations
- **Use case coverage:** Basic point-in-polygon and proximity queries only

### Planned Enhancements

#### ST_Distance - Enhanced Distance Calculations
- **Current:** Haversine distance for point-to-point only
- **Enhancement:**
  - Point-to-LineString distance
  - Point-to-Polygon distance (to boundary)
  - Polygon-to-Polygon distance (minimum distance between boundaries)
  - Line-to-Line distance
- **Use cases:**
  - "Find the nearest road to this building"
  - "What is the shortest distance between these two parcels?"
  - "Calculate buffer zones around infrastructure"

#### ST_Buffer - Buffer Zone Creation
- **Description:** Create a polygon representing all points within a specified distance of a geometry
- **Syntax:** `ST_BUFFER(geometry, distance, [segments])`
- **Use cases:**
  - Environmental impact zones around facilities
  - Service areas around stores/restaurants
  - Noise/pollution zones around airports/highways
- **Example:**
  ```aql
  FOR facility IN industrial_sites
    LET impact_zone = ST_BUFFER(facility.location, 1000)  // 1km buffer
    FOR resident IN residential_areas
      FILTER ST_INTERSECTS(resident.area, impact_zone)
      RETURN {facility: facility.name, impacted: resident.name}
  ```

#### ST_Union / ST_Difference - Set Operations
- **ST_UNION:** Combine multiple geometries into a single geometry
  - Use cases: Merge adjacent parcels, combine service areas, aggregate coverage zones
- **ST_DIFFERENCE:** Subtract one geometry from another
  - Use cases: Exclude protected areas, remove overlapping regions, calculate uncovered areas
- **ST_INTERSECTION:** Extract the overlapping region between geometries
  - Use cases: Find shared boundaries, calculate overlap areas, identify conflicts

#### ST_Crosses / ST_Overlaps - Additional Topological Predicates
- **ST_CROSSES:** Check if geometries cross each other (e.g., road crosses pipeline)
- **ST_OVERLAPS:** Check if geometries overlap but neither contains the other
- **ST_TOUCHES:** Check if geometries touch at boundaries without overlapping
- **ST_DISJOINT:** Check if geometries have no points in common
- **Use cases:**
  - Infrastructure conflict detection
  - Utility network analysis
  - Zoning compliance checks

### Benefits
- **Comprehensive spatial analysis:** Support for complex GIS workflows
- **Standards compliance:** Match PostGIS functionality
- **Reduced client-side processing:** Perform complex operations on the server
- **Better query expressiveness:** Enable sophisticated spatial queries in AQL

### Implementation Considerations
- Leverage Boost.Geometry for CPU backend implementations
- Consider GPU acceleration for computationally intensive operations (ST_BUFFER, ST_UNION)
- Add query optimizer rules for operation chaining (e.g., buffer + intersect)
- Estimated effort: 6-8 weeks

---

## 3. Edge-Edge Intersection Detection

### Current State
- **CPU Backend Implementation:** The current Boost.Geometry-based CPU backend uses the `boost::geometry::intersects()` function for exact geometry checks.
- **Known Issue:** The implementation can produce **false negatives** in specific edge cases where polygon edges intersect but vertices do not lie within the opposite polygon.
- **Impact:** Rare but possible incorrect query results when checking polygon-polygon intersections.

### Problem Description
The current implementation correctly handles:
- **Vertex-in-polygon intersections:** When a vertex of one polygon lies inside another
- **Simple edge overlaps:** When edges clearly cross through polygon interiors

The implementation **may miss:**
- **Edge-only intersections:** When polygon boundaries touch or overlap without vertices entering the interior
- **Tangent cases:** When edges run parallel along boundaries
- **Complex multi-part intersections:** When polygons intersect at multiple disconnected edge segments

### Example Scenario
```
Polygon A: Square [0,0] - [2,2]
Polygon B: Rotated rectangle where edges just touch A's edges without vertices inside A

Current behavior: May return false (no intersection detected)
Expected behavior: Should return true (edges intersect/touch)
```

### Planned Enhancement
- **Implement complete polygon-polygon intersection algorithm:**
  - Edge-edge intersection testing using line segment intersection algorithms
  - Proper handling of degenerate cases (collinear edges, touching vertices)
  - Support for polygon holes and multi-polygons
  - Consistent handling of boundary conditions (inclusive/exclusive)

- **Algorithm components:**
  1. **Bounding box pre-filter:** Quick rejection of non-intersecting polygons (already implemented)
  2. **Edge intersection sweep:** Test all edge pairs for intersections using line segment intersection
  3. **Point-in-polygon tests:** Check if any vertices lie inside the opposite polygon
  4. **Boundary handling:** Configurable behavior for touching boundaries (ST_INTERSECTS vs ST_OVERLAPS)

### Benefits
- **Correctness:** Eliminate false negatives in spatial queries
- **Reliability:** Consistent behavior across all geometry configurations
- **Standards compliance:** Match OGC specifications for topological predicates
- **User confidence:** Accurate results for critical applications (land registry, legal boundaries, infrastructure planning)

### Implementation Considerations
- Extend current `exactIntersects()` function in CPU backend
- Add comprehensive test suite covering edge cases
- Consider performance impact (edge-edge tests are O(n×m) for n and m edges)
- Implement spatial indexing of polygon edges for large polygons
- Document behavior for boundary cases (touching vs intersecting)
- Estimated effort: 2-3 weeks

---

## 4. Spatial Index Optimizations

### Current State
- **R-Tree indexing:** Standard R-Tree with configurable max/min entries per node
- **Morton encoding:** Z-order space-filling curves for 1D mapping of 2D coordinates
- **Two-stage filtering:** Fast MBR checks followed by optional exact geometry verification

### Planned Enhancements

#### Hilbert Curve Indexing
- **Current:** Morton (Z-order) encoding with bit interleaving
- **Enhancement:** Hilbert curve as alternative space-filling curve
- **Advantages:**
  - **Better spatial locality:** 15-20% less distance between consecutive cells
  - **Improved cache performance:** More sequential access patterns
  - **Better range query performance:** Fewer curve discontinuities
- **Trade-offs:**
  - More complex encode/decode (O(n) vs O(log n))
  - Higher implementation complexity
  - Similar worst-case behavior for point queries
- **Recommendation:** Make configurable; use Hilbert for analytical workloads, Morton for OLTP
- **Estimated improvement:** 15-20% better cache hit rates on full-table scans

#### R-Tree Bulk Loading (STR Packing)
- **Current:** Incremental insertion of geometries (one at a time)
- **Problem:** Creates unbalanced tree structure, suboptimal query performance
- **Enhancement:** Sort-Tile-Recursive (STR) bulk loading algorithm
- **Algorithm:**
  1. Sort geometries by X coordinate of centroid
  2. Partition into vertical slices (S slices for S = sqrt(N/M))
  3. Within each slice, sort by Y coordinate
  4. Pack into leaf nodes with M entries each
  5. Build parent levels bottom-up
- **Benefits:**
  - **5-10x faster bulk inserts:** Avoid repeated tree rebalancing
  - **30-50% better query performance:** More balanced tree structure
  - **Predictable tree height:** log_M(N) depth for N geometries
- **Use cases:**
  - Initial data loading from Shapefiles
  - Batch imports of spatial data
  - Index rebuilding/optimization
- **Estimated effort:** 3-4 weeks

#### R*-Tree Implementation
- **Current:** Standard R-Tree with quadratic split algorithm
- **Enhancement:** R*-Tree with improved split heuristics
- **Improvements:**
  1. **Minimize overlap:** Primary split criterion (reduces false positives)
  2. **Minimize area:** Secondary criterion (tighter bounds)
  3. **Minimize margin:** Tertiary criterion (more regular shapes)
  4. **Forced reinsertion:** Reinsert 30% of entries on overflow before split
- **Benefits:**
  - **30-50% better query performance:** Fewer false positives in candidate filtering
  - **Better tree quality:** More balanced and compact structure
  - **Comparable insert cost:** Only 10% slower than standard R-Tree
- **Estimated effort:** 2-3 weeks

#### Adaptive Morton Precision
- **Current:** Fixed precision Morton encoding (typically 32-bit or 64-bit)
- **Problem:** Precision-locality trade-off is data-dependent
- **Enhancement:** Dynamically adjust Morton code precision based on data distribution
- **Strategy:**
  - **Sparse data:** Lower precision (fewer bits) for broader grouping
  - **Dense data:** Higher precision (more bits) for fine-grained locality
  - **Adaptive quantization:** Analyze data distribution at index build time
  - **Per-level precision:** Different precision at different R-Tree levels
- **Benefits:**
  - Better index size/performance balance
  - Automatic tuning for diverse datasets
  - Improved handling of multi-scale data
- **Estimated effort:** 2 weeks

### Implementation Roadmap
1. **Phase 1:** R*-Tree implementation (highest impact, well-researched)
2. **Phase 2:** Bulk loading with STR algorithm (important for data ingestion)
3. **Phase 3:** Hilbert curve support (optional, for analytical workloads)
4. **Phase 4:** Adaptive precision (advanced optimization)

---

## 5. 3D Geometry Support

### Current State
- **Coordinate storage:** X and Y coordinates only (2D geometries)
- **Spatial operations:** All operations assume 2D plane or spherical surface (for geographic coordinates)
- **Use cases:** Primarily ground-level geographic data (maps, addresses, points of interest)

### Planned Enhancement
- **Full 3D geometry support with X, Y, Z coordinates**
- **3D-aware spatial operations**
- **Volumetric queries and analysis**

### 3D Geometry Types
- **Point3D:** (X, Y, Z) coordinates
- **LineString3D:** 3D polylines (flight paths, cables)
- **Polygon3D:** Planar or non-planar 3D surfaces
- **PolyhedralSurface:** Collection of 3D polygons (building facades)
- **Solid:** Volumetric geometries (building interiors)
- **TIN (Triangulated Irregular Network):** Terrain surfaces

### 3D Spatial Operations
- **ST_Distance3D:** Euclidean distance in 3D space
- **ST_3DIntersects:** Check if 3D geometries intersect
- **ST_3DWithin:** Check if geometry is within 3D buffer
- **ST_Volume:** Calculate volume of 3D solids
- **ST_3DArea:** Calculate surface area of 3D geometries
- **ST_3DExtent:** 3D bounding box (minx, miny, minz, maxx, maxy, maxz)

### Use Cases

#### Building Information Models (BIM)
- **Store building floor plans with elevation data**
- **Query buildings by height or volume**
- **Analyze floor-to-floor clearances**
- **Detect spatial conflicts in construction planning**
- Example:
  ```aql
  FOR building IN bim_models
    FILTER building.geometry.z > 50.0  // Buildings taller than 50m
    FILTER ST_3DIntersects(building.geometry, @restricted_airspace)
    RETURN building
  ```

#### Terrain Data Analysis
- **Digital Elevation Models (DEMs)**
- **Topographic analysis (slope, aspect, visibility)**
- **Watershed modeling**
- **Line-of-sight calculations**
- Example:
  ```aql
  FOR point IN observation_points
    LET terrain_height = ST_ZValueAt(terrain_surface, point.x, point.y)
    FILTER point.geometry.z > terrain_height + 10.0  // 10m above ground
    RETURN point
  ```

#### Atmospheric and Oceanic Data
- **3D weather models (temperature, pressure at altitude)**
- **Air quality monitoring with vertical distribution**
- **Ocean current modeling at depth**
- **Pollution dispersion analysis**
- Example:
  ```aql
  FOR sensor IN air_quality_sensors
    FILTER sensor.geometry.z BETWEEN 100 AND 500  // Altitude 100-500m
    LET nearby = (
      FOR other IN air_quality_sensors
        FILTER ST_3DDistance(sensor.geometry, other.geometry) < 1000
        RETURN other
    )
    RETURN {sensor, nearby_count: LENGTH(nearby)}
  ```

#### Underground Infrastructure
- **Utility networks (water, gas, sewage) with depth information**
- **Subway and tunnel systems**
- **Mining operations**
- **Geological formations**

### Implementation Considerations

#### Storage
- Extend geometry storage to include Z coordinate (optional, default to 0)
- Update GeoJSON parser to recognize 3D coordinates: `[x, y, z]`
- Support for EWKB (Extended Well-Known Binary) with Z flag

#### Indexing
- **3D R-Tree:** Extend bounding boxes to 3D (6 coordinates: minx, miny, minz, maxx, maxy, maxz)
- **3D Morton encoding:** Interleave X, Y, Z bits (64-bit codes for reasonable precision)
- **Octree indexing:** Alternative to R-Tree for volumetric data (subdivide into 8 octants)

#### Distance Calculations
- **Euclidean 3D distance:** `sqrt((x2-x1)² + (y2-y1)² + (z2-z1)²)`
- **Note:** Geographic 3D distance requires handling Earth curvature (more complex)

#### Challenges
- **Visualization:** 3D rendering is complex (consider integration with visualization libraries)
- **Performance:** 3D intersection tests are more expensive than 2D
- **Index size:** 3D indexes can be larger (more dimensions to encode)
- **Coordinate reference systems:** Mixing projected (meters) with geographic (degrees) is tricky in 3D

### Estimated Effort
- **Phase 1 - Storage & basic operations:** 3-4 weeks
- **Phase 2 - 3D indexing (R-Tree/Octree):** 4-6 weeks
- **Phase 3 - Advanced operations (volume, surface area):** 2-3 weeks
- **Total:** 2-3 months

---

## 6. Spatial Aggregations

### Current State
- **Limitation:** No built-in spatial aggregation functions in AQL
- **Current approach:** Manual aggregation in application code or post-processing
- **Missing functionality:** Geometric operations on result sets

### Planned Enhancements

#### ST_ConvexHull - Convex Hull Computation
- **Description:** Compute the smallest convex polygon that contains all geometries in a set
- **Algorithm:** Gift wrapping or Graham scan (O(n log n))
- **Use cases:**
  - **Coverage area:** "What is the total area covered by all our stores?"
  - **Service boundary:** "What is the convex boundary of all delivery addresses?"
  - **Cluster analysis:** "What is the spatial extent of this customer cluster?"
- **Syntax:**
  ```aql
  FOR customer IN customers
    FILTER customer.region == 'North'
    COLLECT region = customer.region
    AGGREGATE hull = ST_CONVEXHULL(customer.location)
    RETURN {region, coverage_area: hull}
  ```

#### ST_Centroid - Centroid Calculation
- **Description:** Calculate the geometric center (centroid) of aggregated geometries
- **Types:**
  - **Point centroid:** Average of X,Y coordinates
  - **Polygon centroid:** Center of mass (area-weighted)
  - **LineString centroid:** Midpoint along length
- **Use cases:**
  - **Cluster centers:** "Where is the center of this customer cluster?"
  - **Representative point:** "What is the central location for this delivery route?"
  - **Load balancing:** "Place a new warehouse at the centroid of demand"
- **Syntax:**
  ```aql
  FOR order IN orders
    FILTER order.date >= @start_date
    COLLECT city = order.delivery_city
    AGGREGATE center = ST_CENTROID(order.delivery_location)
    RETURN {city, center}
  ```

#### ST_Extent - Bounding Box Aggregation
- **Description:** Compute the minimum bounding rectangle (MBR) that contains all geometries in a set
- **Faster than convex hull:** Simple min/max operations on coordinates
- **Use cases:**
  - **Map viewport:** "What bounding box should I use to display all search results?"
  - **Spatial statistics:** "What is the geographic spread of this dataset?"
  - **Index optimization:** "What are the total bounds for spatial index configuration?"
- **Syntax:**
  ```aql
  FOR poi IN points_of_interest
    FILTER poi.category == 'restaurant'
    COLLECT category = poi.category
    AGGREGATE extent = ST_EXTENT(poi.location)
    RETURN {
      category,
      bounds: {
        minx: extent.minx,
        miny: extent.miny,
        maxx: extent.maxx,
        maxy: extent.maxy
      }
    }
  ```

#### ST_Union_Agg - Union Aggregation
- **Description:** Combine all geometries in a set into a single merged geometry
- **Use cases:**
  - **Territory consolidation:** "Merge all sales territories into regional boundaries"
  - **Coverage analysis:** "What is the total coverage area of our service zones?"
  - **Gap analysis:** "Combine all served areas to identify service gaps"
- **Syntax:**
  ```aql
  FOR zone IN service_zones
    FILTER zone.provider == @provider_id
    COLLECT provider = zone.provider
    AGGREGATE merged = ST_UNION_AGG(zone.geometry)
    RETURN {provider, coverage: merged}
  ```

#### ST_Collect - Geometry Collection Aggregation
- **Description:** Collect all geometries into a GeometryCollection without merging
- **Preserves individual geometries:** Unlike ST_UNION, keeps geometries separate
- **Use cases:**
  - **Multi-geometry export:** "Export all selected parcels as a single geometry"
  - **Batch operations:** "Apply a buffer to all geometries at once"
- **Syntax:**
  ```aql
  FOR facility IN facilities
    FILTER facility.type == 'warehouse'
    COLLECT type = facility.type
    AGGREGATE collection = ST_COLLECT(facility.location)
    RETURN {type, all_locations: collection}
  ```

### Integration with AQL
- **AGGREGATE keyword:** Extend AQL aggregation framework
- **Window functions:** Support spatial aggregations over windows (PARTITION BY)
- **Nested aggregations:** Allow spatial aggregations in subqueries

### Performance Considerations
- **Streaming aggregation:** Process geometries incrementally (avoid materializing all in memory)
- **Parallel aggregation:** Support parallel computation of spatial aggregates
- **Approximate aggregations:** For very large datasets, offer approximate results (sampling-based)

### Estimated Effort
- **Phase 1 - ST_EXTENT, ST_CENTROID:** 2-3 weeks (simpler operations)
- **Phase 2 - ST_CONVEXHULL:** 2 weeks (well-studied algorithm)
- **Phase 3 - ST_UNION_AGG, ST_COLLECT:** 3-4 weeks (complex merge operations)
- **Total:** 7-9 weeks

---

## 7. Performance Monitoring

### Current State
- **General metrics:** ThemisDB has comprehensive monitoring for query execution, cache hits, storage operations
- **Spatial-specific metrics:** Limited visibility into spatial query performance characteristics
- **Optimization:** Difficult to identify spatial query bottlenecks without detailed metrics

### Planned Enhancements

#### Query Profiling - Spatial Query Performance Metrics
- **Execution stage breakdown:**
  - Time spent in spatial index lookup
  - Time spent in MBR filtering (stage 1)
  - Time spent in exact geometry checks (stage 2)
  - Time spent in result materialization
  
- **Metrics to track:**
  ```json
  {
    "query_id": "q_12345",
    "query_type": "spatial_intersects",
    "total_duration_ms": 45.2,
    "stages": {
      "index_lookup_ms": 2.1,
      "mbr_filter_ms": 5.3,
      "exact_check_ms": 35.8,
      "result_fetch_ms": 2.0
    },
    "candidates_found": 1250,
    "exact_matches": 87,
    "false_positive_rate": 0.93
  }
  ```

- **Per-operation metrics:**
  - ST_INTERSECTS, ST_CONTAINS, ST_WITHIN, ST_DISTANCE execution times
  - Distribution of query execution times (p50, p95, p99)
  - Slow query logging for spatial operations

- **Query plan visualization:**
  - Show which spatial indexes were used
  - Show estimated vs actual result counts
  - Highlight optimization opportunities

#### Index Statistics - Spatial Index Performance
- **Index health metrics:**
  - Index size (bytes, node count)
  - Tree depth and balance statistics
  - Fill factor (average entries per node)
  - MBR overlap percentage (lower is better)

- **Usage statistics:**
  - Index hit rate (queries using index vs full scan)
  - Access patterns (hot spots in spatial index)
  - Cache effectiveness for index nodes
  
- **Per-collection metrics:**
  ```json
  {
    "collection": "locations",
    "spatial_index": {
      "type": "rtree",
      "tree_depth": 4,
      "node_count": 1523,
      "geometry_count": 45678,
      "avg_entries_per_node": 30.0,
      "fill_factor": 0.75,
      "mbr_overlap_pct": 8.5,
      "total_size_mb": 12.3
    }
  }
  ```

- **Index maintenance tracking:**
  - Insertions, deletions, updates since last rebuild
  - Index fragmentation percentage
  - Recommendations for rebuild/optimization

#### Spatial Selectivity Estimation
- **Purpose:** Help query optimizer choose between spatial index lookup vs full scan
- **Selectivity:** Fraction of geometries expected to match a spatial predicate

- **Histogram-based estimation:**
  - Divide spatial domain into grid cells
  - Track geometry count per cell
  - Estimate selectivity based on query bbox overlap with cells

- **Example:**
  ```
  Query: ST_INTERSECTS(geometry, BBOX(13.0, 52.0, 13.1, 52.1))
  
  Analysis:
  - Query bbox covers 3 grid cells
  - Cells contain: [120, 85, 200] geometries
  - Total geometries: 1,000,000
  - Estimated selectivity: (120+85+200)/1,000,000 = 0.000405 (0.04%)
  - Recommendation: Use spatial index (highly selective)
  ```

- **Adaptive statistics:**
  - Update histograms periodically based on query patterns
  - Learn from actual query results (selectivity estimation errors)
  - Adjust for data skew (some areas much denser than others)

#### Monitoring Dashboard
- **Real-time metrics:**
  - Spatial queries per second (QPS)
  - Average query latency (by operation type)
  - Spatial index hit rate
  - Cache hit rate for geometry data

- **Historical trends:**
  - Query performance over time
  - Index growth and health
  - Slow query patterns

- **Alerts and recommendations:**
  - Index rebuild suggested (high fragmentation)
  - Slow query detected (> threshold)
  - Index not used (full scan detected)
  - High false positive rate (> 20%)

#### Integration Points
- **Prometheus metrics:** Export spatial metrics to Prometheus for external monitoring
- **AQL EXPLAIN:** Include spatial-specific information in query plans
- **Admin API endpoints:**
  - `GET /spatial/metrics` - Overall spatial subsystem metrics
  - `GET /spatial/index/stats?collection=<name>` - Per-collection index statistics
  - `GET /spatial/queries/slow` - Recent slow spatial queries
  - `POST /spatial/index/analyze?collection=<name>` - Analyze index and provide recommendations

### Benefits
- **Performance troubleshooting:** Quickly identify spatial query bottlenecks
- **Capacity planning:** Understand spatial workload characteristics
- **Optimization guidance:** Data-driven recommendations for index tuning
- **Query optimization:** Better cost estimation for spatial operations
- **Operational visibility:** Monitor spatial subsystem health in production

### Implementation Considerations
- **Low overhead:** Metrics collection should add < 1% overhead
- **Sampling:** Use sampling for detailed per-query metrics (avoid overhead on every query)
- **Aggregation:** Pre-aggregate common metrics (QPS, latency percentiles)
- **Storage:** Consider retention policies for detailed query logs
- **Privacy:** Avoid logging sensitive geometry data in metrics

### Estimated Effort
- **Phase 1 - Basic metrics (QPS, latency, hit rate):** 1-2 weeks
- **Phase 2 - Index statistics and health:** 2 weeks
- **Phase 3 - Selectivity estimation and optimizer integration:** 3-4 weeks
- **Phase 4 - Dashboard and visualization:** 2-3 weeks
- **Total:** 8-11 weeks

---

## Implementation Priority

### High Priority (Q2 2026)
1. **Full GeoJSON/WKT Parsing** - Essential for developer experience
2. **Edge-Edge Intersection Detection** - Correctness issue
3. **R*-Tree Implementation** - Significant performance gain

### Medium Priority (Q3 2026)
4. **Advanced Spatial Operations** - Expand functionality
5. **Spatial Index Optimizations** - Performance improvements
6. **Performance Monitoring** - Operational visibility

### Lower Priority (Q4 2026+)
7. **3D Geometry Support** - Advanced feature for specific use cases
8. **Spatial Aggregations** - Nice-to-have for analytics

---

## Related Documentation

- [Geospatial Module Overview](de/geo/README.md) - Current implementation
- [RPC Geospatial Query API](api/RPC_GEOSPATIAL_QUERY.md) - Query interface
- [Geospatial Best Practices](research/GEOSPATIAL_BEST_PRACTICES.md) - Research and optimization guide
- [Geospatial Architecture](de/geo/geo_architecture.md) - System design
- [Geospatial Feature Tiering](de/geo/geo_feature_tiering.md) - Feature classification

---

## References

### Standards
- **OGC Simple Features:** https://www.ogc.org/standards/sfa
- **GeoJSON RFC 7946:** https://tools.ietf.org/html/rfc7946
- **ISO 19125 (WKT/WKB):** Geographic information — Simple feature access

### Similar Systems
- **PostGIS:** https://postgis.net/ - Industry standard spatial extension for PostgreSQL
- **SpatiaLite:** https://www.gaia-gis.it/fossil/libspatialite/ - Spatial extension for SQLite
- **MongoDB Geospatial:** https://docs.mongodb.com/manual/geospatial-queries/
- **Oracle Spatial:** Oracle Database spatial and graph features

### Academic Papers
- Guttman, A. (1984). "R-trees: A Dynamic Index Structure for Spatial Searching". SIGMOD
- Beckmann, N., et al. (1990). "The R*-tree: An Efficient and Robust Access Method". SIGMOD
- Moon, B., et al. (2001). "Analysis of the Clustering Properties of the Hilbert Space-Filling Curve". IEEE TKDE

---

**Document Metadata:**
- **Created:** February 2026
- **Last Updated:** April 2026
- **Maintained By:** ThemisDB Development Team
- **Review Cycle:** Quarterly
