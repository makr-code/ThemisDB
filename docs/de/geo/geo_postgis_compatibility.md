# PostGIS Kompatibilität in ThemisDB

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Geo  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Unterstützte PostGIS-Funktionen](#unterstützte-postgis-funktionen)
- [Funktions-Mapping](#funktions-mapping)
- [Migrations-Beispiele](#migrations-beispiele)
- [Performance-Vergleich](#performance-vergleich)
- [Einschränkungen](#einschränkungen)
- [Best Practices](#best-practices)

---

## Übersicht

ThemisDB bietet umfassende Kompatibilität mit PostGIS-Funktionen für räumliche Operationen. Dies ermöglicht einen nahtlosen Übergang von PostgreSQL/PostGIS zu ThemisDB mit minimalen Codeänderungen.

### Kompatibilitäts-Level

| Feature-Kategorie | Kompatibilität | Notizen |
|-------------------|----------------|---------|
| **Geometrie-Typen** | ✅ 95% | Point, LineString, Polygon, Multi* |
| **Räumliche Operationen** | ✅ 90% | Intersects, Contains, Distance, etc. |
| **Indexierung** | ✅ 100% | R-Tree, S2, H3 |
| **Koordinatensysteme** | ✅ 85% | WGS84, Web Mercator, UTM |
| **Ausgabe-Formate** | ✅ 90% | WKT, WKB, GeoJSON |

---

## Unterstützte PostGIS-Funktionen

### Geometrie-Konstruktion

```sql
-- PostGIS
SELECT ST_MakePoint(13.405, 52.520);
SELECT ST_GeomFromText('POINT(13.405 52.520)', 4326);
SELECT ST_GeomFromGeoJSON('{"type":"Point","coordinates":[13.405,52.520]}');
```

```aql
// ThemisDB AQL
LET point = GEO_POINT(13.405, 52.520)

LET point_from_wkt = GEO_FROM_TEXT("POINT(13.405 52.520)")

LET point_from_json = GEO_FROM_GEOJSON('{"type":"Point","coordinates":[13.405,52.520]}')

RETURN {point, point_from_wkt, point_from_json}
```

### Räumliche Prädikate

```sql
-- PostGIS: Intersects
SELECT * FROM buildings 
WHERE ST_Intersects(geometry, ST_MakeEnvelope(13.0, 52.0, 14.0, 53.0, 4326));

-- PostGIS: Contains
SELECT * FROM districts 
WHERE ST_Contains(boundary, ST_MakePoint(13.405, 52.520));

-- PostGIS: Distance
SELECT name, ST_Distance(location, ST_MakePoint(13.405, 52.520)) as distance
FROM poi
WHERE ST_DWithin(location, ST_MakePoint(13.405, 52.520), 1000)
ORDER BY distance;
```

```aql
// ThemisDB AQL: Intersects
FOR building IN buildings
  FILTER GEO_INTERSECTS(building.geometry, 
    GEO_POLYGON([
      [13.0, 52.0], [14.0, 52.0], [14.0, 53.0], [13.0, 53.0], [13.0, 52.0]
    ]))
  RETURN building

// ThemisDB AQL: Contains
FOR district IN districts
  FILTER GEO_CONTAINS(district.boundary, GEO_POINT(13.405, 52.520))
  RETURN district

// ThemisDB AQL: Distance with Within
FOR poi IN poi
  LET distance = GEO_DISTANCE(poi.location, GEO_POINT(13.405, 52.520))
  FILTER distance < 1000
  SORT distance ASC
  RETURN {name: poi.name, distance}
```

### Geometrie-Operationen

```sql
-- PostGIS: Area, Length, Perimeter
SELECT 
  ST_Area(geometry) as area,
  ST_Length(geometry) as length,
  ST_Perimeter(geometry) as perimeter
FROM parcels;

-- PostGIS: Centroid, Buffer
SELECT 
  ST_AsText(ST_Centroid(geometry)) as centroid,
  ST_AsText(ST_Buffer(geometry, 100)) as buffer_100m
FROM regions;
```

```aql
// ThemisDB AQL: Area, Length
FOR parcel IN parcels
  RETURN {
    id: parcel._key,
    area: GEO_AREA(parcel.geometry),
    length: GEO_LENGTH(parcel.geometry),
    perimeter: GEO_PERIMETER(parcel.geometry)
  }

// ThemisDB AQL: Centroid, Buffer
FOR region IN regions
  RETURN {
    id: region._key,
    centroid: GEO_CENTROID(region.geometry),
    buffer: GEO_BUFFER(region.geometry, 100)
  }
```

---

## Funktions-Mapping

### Räumliche Operationen

| PostGIS | ThemisDB AQL | Kompatibilität | Notizen |
|---------|--------------|----------------|---------|
| `ST_Intersects()` | `GEO_INTERSECTS()` | ✅ 100% | Identische Semantik |
| `ST_Contains()` | `GEO_CONTAINS()` | ✅ 100% | Identische Semantik |
| `ST_Within()` | `GEO_WITHIN()` | ✅ 100% | Identische Semantik |
| `ST_Distance()` | `GEO_DISTANCE()` | ✅ 100% | In Metern |
| `ST_DWithin()` | `GEO_DISTANCE() < n` | ✅ 95% | Funktional äquivalent |
| `ST_Area()` | `GEO_AREA()` | ✅ 100% | In Quadratmetern |
| `ST_Length()` | `GEO_LENGTH()` | ✅ 100% | In Metern |
| `ST_Buffer()` | `GEO_BUFFER()` | ✅ 90% | Leichte Unterschiede bei sehr kleinen Buffern |
| `ST_Centroid()` | `GEO_CENTROID()` | ✅ 100% | Identische Semantik |
| `ST_ConvexHull()` | `GEO_CONVEX_HULL()` | ✅ 100% | Identische Semantik |

### Geometrie-Ausgabe

| PostGIS | ThemisDB AQL | Kompatibilität |
|---------|--------------|----------------|
| `ST_AsText()` | `GEO_AS_TEXT()` | ✅ 100% |
| `ST_AsBinary()` | `GEO_AS_BINARY()` | ✅ 100% |
| `ST_AsGeoJSON()` | `GEO_AS_GEOJSON()` | ✅ 100% |

### Geometrie-Eingabe

| PostGIS | ThemisDB AQL | Kompatibilität |
|---------|--------------|----------------|
| `ST_GeomFromText()` | `GEO_FROM_TEXT()` | ✅ 100% |
| `ST_GeomFromWKB()` | `GEO_FROM_BINARY()` | ✅ 100% |
| `ST_GeomFromGeoJSON()` | `GEO_FROM_GEOJSON()` | ✅ 100% |
| `ST_MakePoint()` | `GEO_POINT()` | ✅ 100% |
| `ST_MakeLine()` | `GEO_LINESTRING()` | ✅ 100% |
| `ST_MakePolygon()` | `GEO_POLYGON()` | ✅ 100% |

---

## Migrations-Beispiele

### Beispiel 1: Spatial Join

**PostGIS:**
```sql
SELECT 
  p.name as poi_name,
  d.name as district_name
FROM points_of_interest p
JOIN districts d ON ST_Contains(d.boundary, p.location)
WHERE p.category = 'restaurant'
ORDER BY d.name, p.name;
```

**ThemisDB AQL:**
```aql
FOR poi IN points_of_interest
  FILTER poi.category == 'restaurant'
  FOR district IN districts
    FILTER GEO_CONTAINS(district.boundary, poi.location)
    SORT district.name, poi.name
    RETURN {
      poi_name: poi.name,
      district_name: district.name
    }
```

### Beispiel 2: Nearest Neighbor

**PostGIS:**
```sql
SELECT 
  name,
  ST_Distance(location, ST_MakePoint(13.405, 52.520)) as distance
FROM points_of_interest
ORDER BY location <-> ST_MakePoint(13.405, 52.520)
LIMIT 10;
```

**ThemisDB AQL:**
```aql
LET center = GEO_POINT(13.405, 52.520)

FOR poi IN points_of_interest
  LET distance = GEO_DISTANCE(poi.location, center)
  SORT distance ASC
  LIMIT 10
  RETURN {
    name: poi.name,
    distance
  }
```

### Beispiel 3: Spatial Index mit R-Tree

**PostGIS:**
```sql
-- Index erstellen
CREATE INDEX idx_buildings_geom ON buildings USING GIST(geometry);

-- Query mit Index
SELECT * FROM buildings
WHERE ST_Intersects(geometry, ST_MakeEnvelope(13.0, 52.0, 14.0, 53.0, 4326));
```

**ThemisDB AQL:**
```aql
// Index erstellen (via Admin API)
db.buildings.ensureIndex({
  type: "geo",
  fields: ["geometry"],
  geoJson: true
});

// Query mit Index
FOR building IN buildings
  FILTER GEO_INTERSECTS(building.geometry,
    GEO_POLYGON([[13.0, 52.0], [14.0, 52.0], [14.0, 53.0], [13.0, 53.0], [13.0, 52.0]]))
  RETURN building
```

---

## Performance-Vergleich

### Benchmark: Nearest Neighbor (10 Ergebnisse)

| Datenbank | Datensatz | Index | Query-Zeit | Durchsatz |
|-----------|-----------|-------|------------|-----------|
| **PostGIS** | 1M POIs | GIST | 45 ms | 22 queries/sec |
| **ThemisDB (CPU)** | 1M POIs | R-Tree | 38 ms | 26 queries/sec |
| **ThemisDB (GPU)** | 1M POIs | R-Tree | 12 ms | 83 queries/sec |

### Benchmark: Spatial Join (Intersects)

| Datenbank | Datensatz | Query-Zeit | Ergebnisse |
|-----------|-----------|------------|------------|
| **PostGIS** | 100K Buildings + 5K Districts | 2.3 sec | 98,456 |
| **ThemisDB (CPU)** | 100K Buildings + 5K Districts | 1.9 sec | 98,456 |
| **ThemisDB (GPU)** | 100K Buildings + 5K Districts | 0.6 sec | 98,456 |

**Test-Setup:**
- CPU: AMD EPYC 7742 (64 Cores)
- GPU: NVIDIA A100 (80GB)
- RAM: 256 GB
- PostGIS: Version 3.4
- ThemisDB: Version 1.4.0

---

## Einschränkungen

### Nicht unterstützte Funktionen

| PostGIS-Funktion | Status | Alternative |
|------------------|--------|-------------|
| `ST_Union()` | ⏳ Geplant v1.5 | Manuell kombinieren |
| `ST_Difference()` | ⏳ Geplant v1.5 | - |
| `ST_SymDifference()` | ⏳ Geplant v1.5 | - |
| `ST_Simplify()` | ✅ Verfügbar | `GEO_SIMPLIFY()` |
| `ST_Transform()` | ⚠️ Eingeschränkt | Nur WGS84 ↔ Web Mercator |
| Topology-Funktionen | ❌ Nicht geplant | - |
| Raster-Operationen | ❌ Nicht geplant | GDAL verwenden |

### Koordinatensystem-Limitierungen

- **Unterstützt:** WGS84 (EPSG:4326), Web Mercator (EPSG:3857), UTM Zonen
- **Nicht unterstützt:** Lokale Koordinatensysteme, historische Datums

---

## Best Practices

### 1. Index-Nutzung optimieren

```aql
// ✅ Gut: Index wird verwendet
FOR building IN buildings
  FILTER GEO_INTERSECTS(building.geometry, @searchArea)
  RETURN building

// ❌ Schlecht: Index kann nicht verwendet werden
FOR building IN buildings
  FILTER GEO_AREA(building.geometry) > 1000  // Keine Index-Nutzung
  RETURN building
```

### 2. Batch-Operationen nutzen

```aql
// ✅ Gut: Batch Processing
LET centers = [
  GEO_POINT(13.405, 52.520),
  GEO_POINT(13.388, 52.517),
  GEO_POINT(13.398, 52.525)
]

FOR center IN centers
  LET nearby = (
    FOR poi IN points_of_interest
      FILTER GEO_DISTANCE(poi.location, center) < 500
      RETURN poi
  )
  RETURN {center, nearby}
```

### 3. Koordinatensystem beachten

```aql
// ✅ Gut: Explizite Angabe des Koordinatensystems
LET point = GEO_POINT(13.405, 52.520, "EPSG:4326")

// ⚠️ Vorsicht: Implizit WGS84 angenommen
LET point = GEO_POINT(13.405, 52.520)
```

### 4. Geometrie-Validierung

```aql
// ✅ Gut: Geometrie validieren vor Speicherung
FOR doc IN @importData
  LET geom = GEO_FROM_GEOJSON(doc.geometry)
  FILTER GEO_IS_VALID(geom)
  INSERT {geometry: geom, data: doc.properties} INTO spatial_data
```

---

## Siehe auch

- [Geospatial Query Examples](geo_query_examples.md)
- [Geo Benchmarks](geo_benchmarks.md)
- [Geo Architecture](geo_architecture.md)
- [PostGIS Official Documentation](https://postgis.net/documentation/)
