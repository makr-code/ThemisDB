# RPC Geospatial Query API

**Status:** Implemented  
**Version:** 1.0.0  
**Category:** API / Geospatial

---

## Overview

The ThemisDB RPC service now supports geospatial queries through the `geo_query` method. This enables clients to perform spatial searches using bounding boxes, proximity searches, and intersection queries.

## Prerequisites

Before using geospatial queries, you must:

1. Create a spatial index for your collection using the HTTP API:
   ```bash
   curl -X POST http://localhost:8080/spatial/index/create \
     -H "Content-Type: application/json" \
     -d '{
       "table": "locations",
       "geometry_column": "geometry",
       "config": {
         "total_bounds": {
           "minx": -180.0,
           "miny": -90.0,
           "maxx": 180.0,
           "maxy": 90.0
         }
       }
     }'
   ```

2. Insert entities with geometry fields (GeoJSON format):
   ```bash
   curl -X PUT http://localhost:8080/entities/locations:berlin \
     -H "Content-Type: application/json" \
     -d '{
       "blob": "{\"id\":\"berlin\",\"name\":\"Berlin\",\"geometry\":{\"type\":\"Point\",\"coordinates\":[13.4,52.5]}}"
     }'
   ```

## RPC Method: `geo_query`

### Parameters

All geospatial queries require these base parameters:

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `collection` | string | Yes | Name of the collection to query |
| `type` | string | Yes | Query type: `intersects`, `within`, or `near` |

### Query Types

#### 1. Intersects Query

Find all geometries that intersect with a bounding box.

**Additional Parameters:**
- `bbox` (object, required): Bounding box with `minx`, `miny`, `maxx`, `maxy`

**Example:**
```json
{
  "method": "geo_query",
  "params": {
    "collection": "locations",
    "type": "intersects",
    "bbox": {
      "minx": 13.0,
      "miny": 52.0,
      "maxx": 14.0,
      "maxy": 53.0
    }
  }
}
```

**Response:**
```json
{
  "data": {
    "results": [
      {
        "primary_key": "berlin",
        "mbr": {
          "minx": 13.4,
          "miny": 52.5,
          "maxx": 13.4,
          "maxy": 52.5
        }
      }
    ],
    "count": 1,
    "query_type": "intersects",
    "collection": "locations"
  }
}
```

#### 2. Within Query

Alias for `intersects` - finds geometries within a bounding box.

**Parameters:** Same as `intersects`

#### 3. Near Query

Find all geometries within a specified radius of a center point.

**Additional Parameters:**
- `center` (object, required): Center point with `lon` and `lat` coordinates
- `radius` (number, required): Search radius in meters

**Example:**
```json
{
  "method": "geo_query",
  "params": {
    "collection": "locations",
    "type": "near",
    "center": {
      "lon": 13.4,
      "lat": 52.5
    },
    "radius": 50000
  }
}
```

**Response:**
```json
{
  "data": {
    "results": [
      {
        "primary_key": "berlin",
        "mbr": {
          "minx": 13.4,
          "miny": 52.5,
          "maxx": 13.4,
          "maxy": 52.5
        },
        "distance": 0.0
      },
      {
        "primary_key": "potsdam",
        "mbr": {
          "minx": 13.1,
          "miny": 52.4,
          "maxx": 13.1,
          "maxy": 52.4
        },
        "distance": 28456.3
      }
    ],
    "count": 2,
    "query_type": "near",
    "collection": "locations"
  }
}
```

## Distance Calculation

The `near` query uses the Haversine formula to calculate great circle distances on a sphere (Earth). This provides accurate distance measurements for geographic coordinates (longitude/latitude).

**Formula:**
```
d = 2 * R * arcsin(sqrt(sin²(Δφ/2) + cos(φ₁) * cos(φ₂) * sin²(Δλ/2)))
```

Where:
- R = Earth's radius (6,371,000 meters)
- φ = latitude in radians
- λ = longitude in radians

## Error Handling

### Common Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `Missing required parameter: collection` | Collection parameter not provided | Add `collection` to params |
| `Missing required parameter: type` | Query type not specified | Add `type` to params (intersects/within/near) |
| `Spatial index not initialized` | RPC service not configured with spatial index | Check server configuration |
| `Collection does not have a spatial index` | No spatial index created for collection | Create spatial index first |
| `Missing or invalid 'bbox' parameter` | Bbox required for intersects/within | Provide bbox with minx, miny, maxx, maxy |
| `Missing or invalid 'center' parameter` | Center required for near query | Provide center with lon, lat |
| `Missing 'radius' parameter` | Radius required for near query | Provide radius in meters |
| `Invalid query type` | Unsupported query type | Use: intersects, within, or near |

## Performance Considerations

1. **Spatial Index**: Queries require a spatial index on the collection. Performance depends on:
   - Number of indexed geometries
   - Query area size
   - R-Tree configuration (max/min entries per node)

2. **Bounding Box Queries**: `intersects` and `within` queries use MBR (Minimum Bounding Rectangle) filtering:
   - Fast initial filtering using Morton codes (Z-order curves)
   - Optional exact geometry checks with Boost.Geometry backend

3. **Near Queries**: Proximity searches:
   - Convert radius to bounding box
   - Filter candidates using spatial index
   - Calculate exact distances using Haversine formula
   - Filter results to within specified radius

## Integration with HTTP API

While the RPC method provides programmatic access, you can also use the HTTP REST API:

### Create Spatial Index
```bash
POST /spatial/index/create
```

### Query Spatial Index Stats
```bash
GET /spatial/index/stats?table=locations
```

### Get Spatial Metrics
```bash
GET /spatial/metrics
```

## Examples

### Finding Cities Near Berlin

```json
{
  "method": "geo_query",
  "params": {
    "collection": "cities",
    "type": "near",
    "center": {"lon": 13.4, "lat": 52.5},
    "radius": 100000
  }
}
```

### Finding Points of Interest in a Region

```json
{
  "method": "geo_query",
  "params": {
    "collection": "poi",
    "type": "intersects",
    "bbox": {
      "minx": 13.0,
      "miny": 52.0,
      "maxx": 14.0,
      "maxy": 53.0
    }
  }
}
```

### Finding Buildings Within Downtown

```json
{
  "method": "geo_query",
  "params": {
    "collection": "buildings",
    "type": "within",
    "bbox": {
      "minx": 13.38,
      "miny": 52.51,
      "maxx": 13.42,
      "maxy": 52.53
    }
  }
}
```

## See Also

- [Spatial Index Architecture](geo_architecture.md)
- [Geospatial Feature Tiering](geo_feature_tiering.md)
- [EWKB Format Support](../utils/geo/ewkb.md)
- [Spatial API Handler](../server/spatial_api_handler.md)
- [Future Enhancements for Geospatial Implementation](../GEOSPATIAL_FUTURE_ENHANCEMENTS.md) - Planned improvements and roadmap
