# Geo Troubleshooting Guide

The `geo` module provides geospatial indexing and querying for ThemisDB, including an R-tree spatial index, coordinate validation, approximate radius search, GDAL-based geometry processing, and an optional GPU-accelerated backend stub.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `GeoRtree: point outside valid range` | Coordinates in wrong order (lat/lng vs lng/lat) | Use GeoJSON order: `[lng, lat]` |
| Radius search returns no results | Unit mismatch (km vs m) | Check `geo.distance_unit` config |
| `GpuBackendStub: GPU geo not implemented` | GPU geo is a stub | Use CPU backend for geo queries |
| Spatial index not used | Index not created on geo field | Create geo index on the field |
| `GdalProcessor: library not found` | GDAL not installed | Install `libgdal-dev` |
| Bounding box query returns too many results | Bounding box too large | Use polygon query for precise filtering |
| `CpuExactBackend: OOM on large polygon` | Polygon has too many vertices | Simplify polygon; use Douglas-Peucker |
| Coordinate precision loss | Storing as float32 instead of float64 | Use `geo.precision: double` |
| Distance calculation wrong for antipodal points | Using Euclidean instead of Haversine | Set `geo.distance_formula: haversine` |
| Import of GeoJSON fails | Wrong GeoJSON format | Validate with `geojsonlint` |

## Common Issues

### Issue 1: Coordinate Order Mismatch

**Description:** Spatial queries return wrong results because coordinates are in lat/lng order instead of GeoJSON lng/lat.

**Symptoms:**
- Point queried at Berlin `[52.52, 13.40]` returns nothing
- Correct GeoJSON is `[13.40, 52.52]` (longitude first)

**Cause:** Application is building GeoJSON with lat/lng instead of the GeoJSON standard lng/lat.

**Solution:**
```yaml
geo:
  coordinate_order: lng_lat         # GeoJSON standard: [longitude, latitude]
  validate_coordinate_range: true   # reject invalid coordinates
```
```json
{
  "type": "Point",
  "coordinates": [13.4050, 52.5200]
}
```
```bash
# Validate a GeoJSON document
themisdb-admin geo validate --geojson '{"type":"Point","coordinates":[13.4050,52.5200]}'
```

---

### Issue 2: Radius Search Returns No Results

**Description:** `WITHIN RADIUS` queries return empty results despite nearby documents existing.

**Symptoms:**
- `FOR d IN stores FILTER GEO_DISTANCE(d.location, @point) < 5000 RETURN d` returns nothing
- Units are meters but query seems like km

**Cause:** Distance threshold is in the wrong unit; default is meters.

**Solution:**
```sql
-- Distance in meters (5 km = 5000 m)
FOR d IN stores
  FILTER GEO_DISTANCE(d.location, GEO_POINT(13.40, 52.52)) < 5000
  SORT GEO_DISTANCE(d.location, GEO_POINT(13.40, 52.52))
  RETURN d
```
```yaml
geo:
  distance_unit: meters             # "meters" | "kilometers" | "miles"
  distance_formula: haversine       # "haversine" | "vincenty" | "euclidean"
```

---

### Issue 3: Spatial Index Not Created

**Description:** Geo queries perform full collection scans because no spatial index exists.

**Symptoms:**
- EXPLAIN shows `CollectionScan` for geo filter
- Geo queries time out on large collections

**Cause:** No geo index on the `location` field.

**Solution:**
```bash
# Create geo index
themisdb-admin index create \
  --collection stores \
  --type geo \
  --fields location \
  --geo-json true

# Verify index
themisdb-admin index list --collection stores --type geo
```

---

### Issue 4: GDAL Library Not Found

**Description:** Complex geometry operations fail because GDAL is not installed.

**Symptoms:**
- Log: `GdalProcessor: libgdal.so not found; geometry operations unavailable`
- WKT/WKB import and coordinate reprojection fail

**Cause:** GDAL library not installed on the system.

**Solution:**
```bash
# Install GDAL
apt install libgdal-dev gdal-bin

# Verify
gdal-config --version
ldconfig -p | grep gdal
```
```yaml
geo:
  gdal:
    enabled: true
    library_path: ""               # empty = auto-detect
    default_srs: EPSG:4326        # WGS84
```

---

### Issue 5: Large Polygon Causes OOM

**Description:** A polygon with many vertices exhausts memory during containment checks.

**Symptoms:**
- Log: `CpuExactBackend: polygon has 50000 vertices – memory limit exceeded`
- Process RSS spikes during polygon queries

**Cause:** Raw polygon from GIS export has too many vertices; simplification not applied.

**Solution:**
```bash
# Simplify polygon before import (using ogr2ogr/GDAL)
ogr2ogr -simplify 0.0001 simplified.geojson input.geojson

# Or use ThemisDB admin simplification
themisdb-admin geo simplify-polygon \
  --geojson /tmp/complex_polygon.geojson \
  --tolerance 0.0001 \
  --output /tmp/simplified.geojson
```
```yaml
geo:
  max_polygon_vertices: 10000      # reject polygons with > 10000 vertices
  auto_simplify: true
  simplify_tolerance: 0.0001       # degrees
```

---

### Issue 6: Distance Formula Gives Wrong Results for Long Distances

**Description:** Distance between two far-apart points is significantly wrong.

**Symptoms:**
- Distance between New York and Los Angeles is calculated as ~3200 km instead of ~4000 km
- Euclidean formula used instead of Haversine

**Cause:** `geo.distance_formula` set to `euclidean`; only valid for small areas.

**Solution:**
```yaml
geo:
  distance_formula: haversine      # correct for WGS84 Earth distances
  # vincenty provides higher precision but is slower
```

---

### Issue 7: GeoJSON Import Fails with Schema Error

**Description:** Bulk import of GeoJSON documents fails validation.

**Symptoms:**
- Log: `GeoRtree: invalid GeoJSON: missing 'type' field`
- Import API returns `400 Bad Request`

**Cause:** GeoJSON missing required `type` field or nested feature collection not unwrapped.

**Solution:**
```bash
# Validate GeoJSON file
python3 -c "import json,sys; data=json.load(open('data.geojson')); print('valid')"
geojsonlint data.geojson

# Unwrap FeatureCollection to individual Features
themisdb-admin geo import \
  --file data.geojson \
  --collection stores \
  --unwrap-feature-collection
```

---

### Issue 8: Approximate Radius Search Has Too Many False Positives

**Description:** `ApproximateRadiusSearch` returns points outside the requested radius.

**Symptoms:**
- Points 6 km away appear in a 5 km radius search
- Log: `ApproximateRadiusSearch: using R-tree first-pass; FP rate=12%`

**Cause:** R-tree bounding box filter passes points that fail exact distance check; exact post-filter disabled.

**Solution:**
```yaml
geo:
  approximate_radius_search:
    rtree_first_pass: true
    exact_post_filter: true         # always apply exact Haversine after R-tree
    fp_rate_threshold: 0.05         # warn if false positive rate > 5%
```

## Diagnostic Commands

```bash
# Test geo index query
themisdb-admin geo query \
  --collection stores \
  --center "[13.4050, 52.5200]" \
  --radius 5000

# Validate a GeoJSON document
themisdb-admin geo validate --geojson-file /tmp/store.geojson

# Geo index statistics
themisdb-admin index stats --collection stores --type geo

# GDAL info
gdal-config --version
themisdb-admin geo gdal-info

# Live geo metrics
curl -s http://localhost:9100/metrics | grep themisdb_geo

# Tail geo logs
journalctl -u themisdb -f | grep -E "geo|rtree|gdal|radius|polygon|haversine"
```

## Configuration Reference

```yaml
geo:
  enabled: true
  coordinate_order: lng_lat
  distance_unit: meters
  distance_formula: haversine
  validate_coordinate_range: true
  max_polygon_vertices: 10000
  auto_simplify: true
  gdal:
    enabled: true
    default_srs: EPSG:4326
  approximate_radius_search:
    rtree_first_pass: true
    exact_post_filter: true
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `coordinate_order` | `lat_lng` | `lng_lat` (GeoJSON standard) |
| `distance_formula` | `euclidean` | `haversine` for WGS84 |
| `distance_unit` | `km` | `meters` (API default) |
| `exact_post_filter` | `false` | `true` |

## Known Limitations

- GPU geospatial backend (`gpu_backend_stub.cpp`) is a stub only; all GPU geo operations fall back to CPU.
- R-tree index does not support 3D coordinates (altitude/elevation).
- Polygon containment checks for very complex polygons (>5000 vertices) may be slow even with exact filter.
- GDAL coordinate reprojection requires EPSG database; ensure `gdal-data` package is installed.

## Related Documentation

- [Geo Module ROADMAP](../../src/geo/ROADMAP.md)
- [GDAL Implementation Complete](../ARCHIVED/implementation-summaries/GDAL_IMPLEMENTATION_COMPLETE.md)
- [GDAL Integration Summary](../ARCHIVED/implementation-summaries/GDAL_INTEGRATION_SUMMARY.md)
- [Geospatial Future Enhancements](../GEOSPATIAL_FUTURE_ENHANCEMENTS.md)
- [RPC Geospatial Query](../api/RPC_GEOSPATIAL_QUERY.md)
- [Approximate Radius Search](../performance/ApproximateRadiusSearch.md)
