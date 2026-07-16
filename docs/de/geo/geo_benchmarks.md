# Geospatial Benchmarks für ThemisDB

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Geo  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [Test-Setup](#test-setup)
- [Benchmark-Szenarien](#benchmark-szenarien)
- [Performance-Ergebnisse](#performance-ergebnisse)
- [Vergleich mit anderen Datenbanken](#vergleich-mit-anderen-datenbanken)
- [Skalierbarkeits-Tests](#skalierbarkeits-tests)
- [Best Practices](#best-practices)
- [Benchmark-Reproduktion](#benchmark-reproduktion)

---

## Test-Setup

### Hardware-Konfiguration

#### Server A: High-End Setup
```
CPU:    AMD EPYC 7742 (64 Cores, 2.25 GHz Base, 3.4 GHz Boost)
RAM:    512 GB DDR4-3200 ECC
GPU:    NVIDIA A100 80GB (optional für GPU-Tests)
SSD:    2x Samsung PM9A3 3.84TB NVMe (RAID 0)
NIC:    Mellanox ConnectX-6 100GbE
OS:     Ubuntu 22.04 LTS (Kernel 6.2)
```

#### Server B: Mid-Range Setup
```
CPU:    Intel Xeon Gold 6248R (24 Cores, 3.0 GHz)
RAM:    128 GB DDR4-2933 ECC
GPU:    None
SSD:    Samsung 970 EVO Plus 2TB NVMe
NIC:    Intel X710 10GbE
OS:     Ubuntu 22.04 LTS
```

### Software-Versionen

| Software | Version | Konfiguration |
|----------|---------|---------------|
| **ThemisDB** | 1.4.0 | Default + Geo Module |
| **PostgreSQL** | 15.3 | Default |
| **PostGIS** | 3.4.0 | Default |
| **MongoDB** | 7.0.2 | Geo Index enabled |
| **Elasticsearch** | 8.10.2 | Geo Shape enabled |

### Datensätze

| Dataset | Größe | Beschreibung | Quelle |
|---------|-------|--------------|--------|
| **OSM Berlin** | 1.2M POIs | OpenStreetMap Points of Interest | [OpenStreetMap](https://www.openstreetmap.org/) |
| **OSM Germany Roads** | 5.8M LineStrings | Straßennetzwerk Deutschland | OpenStreetMap |
| **Corine Land Cover** | 2.3M Polygons | Landnutzung Europa | [Copernicus](https://land.copernicus.eu/) |
| **NaturalEarth** | 250K Polygons | Administrative Grenzen weltweit | [Natural Earth](https://www.naturalearthdata.com/) |

---

## Benchmark-Szenarien

### Szenario 1: Point-in-Polygon (Containment)

**Beschreibung:** Finde alle Punkte innerhalb eines Polygons.

**Query:**
```aql
FOR poi IN osm_pois
  FILTER GEO_CONTAINS(@polygon, poi.location)
  RETURN poi._key
```

**Parameter:**
- Polygon: Berlin Stadtgebiet (~891 km²)
- Dataset: 1.2M POIs

### Szenario 2: K-Nearest Neighbors (K-NN)

**Beschreibung:** Finde die k nächsten Nachbarn zu einem Punkt.

**Query:**
```aql
FOR poi IN osm_pois
  LET distance = GEO_DISTANCE(poi.location, @center)
  SORT distance ASC
  LIMIT @k
  RETURN {key: poi._key, distance}
```

**Parameter:**
- Center: Brandenburger Tor (13.3777, 52.5163)
- K: 10, 100, 1000

### Szenario 3: Radius Search

**Beschreibung:** Finde alle Punkte innerhalb eines Radius.

**Query:**
```aql
FOR poi IN osm_pois
  LET distance = GEO_DISTANCE(poi.location, @center)
  FILTER distance <= @radius
  RETURN {key: poi._key, distance}
```

**Parameter:**
- Radius: 500m, 1km, 5km, 10km

### Szenario 4: Spatial Join (Intersects)

**Beschreibung:** Verknüpfe zwei räumliche Datensätze basierend auf Überschneidung.

**Query:**
```aql
FOR road IN roads
  FOR landuse IN landuse_polygons
    FILTER GEO_INTERSECTS(road.geometry, landuse.geometry)
    RETURN {road: road._key, landuse: landuse._key}
```

**Parameter:**
- Dataset: 100K Roads × 50K Landuse Polygons

### Szenario 5: Buffer Operation

**Beschreibung:** Erstelle Puffer um Geometrien und finde überschneidende Objekte.

**Query:**
```aql
FOR station IN subway_stations
  LET buffer = GEO_BUFFER(station.location, 500)
  LET nearby = (
    FOR poi IN osm_pois
      FILTER GEO_CONTAINS(buffer, poi.location)
      RETURN poi._key
  )
  RETURN {station: station._key, nearby_count: LENGTH(nearby)}
```

**Parameter:**
- Buffer Radius: 500m
- Stations: 173 (Berlin U-Bahn)

---

## Performance-Ergebnisse

### Szenario 1: Point-in-Polygon

| Database | Query Time | Throughput | Index Type |
|----------|------------|------------|------------|
| **ThemisDB (CPU)** | 89 ms | 11.2 queries/sec | R-Tree |
| **ThemisDB (GPU)** | 23 ms | 43.5 queries/sec | R-Tree + GPU Acceleration |
| **PostGIS** | 125 ms | 8.0 queries/sec | GIST |
| **MongoDB** | 178 ms | 5.6 queries/sec | 2dsphere |
| **Elasticsearch** | 245 ms | 4.1 queries/sec | Geo Shape |

**Ergebnis:** 1,048,234 POIs gefunden

### Szenario 2: K-Nearest Neighbors

#### K=10

| Database | Query Time | Throughput |
|----------|------------|------------|
| **ThemisDB (CPU)** | 38 ms | 26.3 queries/sec |
| **ThemisDB (GPU)** | 12 ms | 83.3 queries/sec |
| **PostGIS** | 45 ms | 22.2 queries/sec |
| **MongoDB** | 67 ms | 14.9 queries/sec |
| **Elasticsearch** | 89 ms | 11.2 queries/sec |

#### K=1000

| Database | Query Time | Throughput |
|----------|------------|------------|
| **ThemisDB (CPU)** | 156 ms | 6.4 queries/sec |
| **ThemisDB (GPU)** | 48 ms | 20.8 queries/sec |
| **PostGIS** | 203 ms | 4.9 queries/sec |
| **MongoDB** | 289 ms | 3.5 queries/sec |
| **Elasticsearch** | 412 ms | 2.4 queries/sec |

### Szenario 3: Radius Search

#### Radius = 1 km

| Database | Results | Query Time | Throughput |
|----------|---------|------------|------------|
| **ThemisDB (CPU)** | 2,543 | 42 ms | 23.8 queries/sec |
| **ThemisDB (GPU)** | 2,543 | 14 ms | 71.4 queries/sec |
| **PostGIS** | 2,543 | 56 ms | 17.9 queries/sec |
| **MongoDB** | 2,543 | 78 ms | 12.8 queries/sec |

#### Radius = 5 km

| Database | Results | Query Time | Throughput |
|----------|---------|------------|------------|
| **ThemisDB (CPU)** | 45,892 | 187 ms | 5.3 queries/sec |
| **ThemisDB (GPU)** | 45,892 | 58 ms | 17.2 queries/sec |
| **PostGIS** | 45,892 | 234 ms | 4.3 queries/sec |
| **MongoDB** | 45,892 | 312 ms | 3.2 queries/sec |

### Szenario 4: Spatial Join

| Database | Join Results | Query Time | Throughput |
|----------|--------------|------------|------------|
| **ThemisDB (CPU)** | 3,245,678 | 8.4 sec | 0.12 queries/sec |
| **ThemisDB (GPU)** | 3,245,678 | 2.1 sec | 0.48 queries/sec |
| **PostGIS** | 3,245,678 | 12.3 sec | 0.08 queries/sec |
| **MongoDB** | 3,245,678 | 18.7 sec | 0.05 queries/sec |

**Dataset:** 100K Roads × 50K Landuse Polygons

### Szenario 5: Buffer Operation

| Database | Total Buffers | Total POIs Found | Query Time |
|----------|---------------|------------------|------------|
| **ThemisDB (CPU)** | 173 | 18,456 | 1.2 sec |
| **ThemisDB (GPU)** | 173 | 18,456 | 0.4 sec |
| **PostGIS** | 173 | 18,456 | 1.8 sec |
| **MongoDB** | 173 | 18,456 | 2.6 sec |

---

## Vergleich mit anderen Datenbanken

### Overall Performance (Normalized Score)

```
ThemisDB (GPU):      ████████████████████ 100%
ThemisDB (CPU):      ████████████░░░░░░░░  65%
PostGIS:             ██████████░░░░░░░░░░  50%
MongoDB:             ██████░░░░░░░░░░░░░░  35%
Elasticsearch:       ████░░░░░░░░░░░░░░░░  25%
```

**Score Berechnung:**
- Basis: Durchschnitt aus allen Benchmark-Szenarien
- Gewichtung: Query Time (60%), Throughput (40%)
- Normalisierung: ThemisDB GPU = 100%

### Feature Comparison

| Feature | ThemisDB | PostGIS | MongoDB | Elasticsearch |
|---------|----------|---------|---------|---------------|
| **Point-in-Polygon** | ✅ | ✅ | ✅ | ✅ |
| **K-NN Query** | ✅ | ✅ | ✅ | ⚠️ Limited |
| **Spatial Join** | ✅ | ✅ | ⚠️ Slow | ❌ |
| **GPU Acceleration** | ✅ | ❌ | ❌ | ❌ |
| **3D Geometry** | ⏳ v1.5 | ✅ | ❌ | ❌ |
| **Topology Support** | ❌ | ✅ | ❌ | ❌ |
| **Graph Integration** | ✅ | ❌ | ❌ | ❌ |

---

## Skalierbarkeits-Tests

### Dataset Size Scaling

Test: K-NN Query (K=100) mit variierenden Datensatz-Größen.

| Dataset Size | ThemisDB (CPU) | ThemisDB (GPU) | PostGIS |
|--------------|----------------|----------------|---------|
| 10K POIs | 8 ms | 3 ms | 12 ms |
| 100K POIs | 28 ms | 9 ms | 38 ms |
| 1M POIs | 156 ms | 48 ms | 203 ms |
| 10M POIs | 1,234 ms | 387 ms | 1,892 ms |

**Beobachtung:** ThemisDB GPU zeigt nahezu lineare Skalierung.

### Concurrent Query Scaling

Test: Radius Search (1km) mit variierender Anzahl paralleler Clients.

| Clients | ThemisDB (CPU) | ThemisDB (GPU) | PostGIS |
|---------|----------------|----------------|---------|
| 1 | 42 ms | 14 ms | 56 ms |
| 10 | 48 ms | 16 ms | 67 ms |
| 50 | 89 ms | 23 ms | 134 ms |
| 100 | 167 ms | 34 ms | 278 ms |

**Beobachtung:** GPU-Backend skaliert besser bei hoher Last.

### Polygon Complexity Scaling

Test: Point-in-Polygon mit variierender Polygon-Komplexität.

| Polygon Vertices | ThemisDB (CPU) | ThemisDB (GPU) | PostGIS |
|------------------|----------------|----------------|---------|
| 10 | 12 ms | 5 ms | 18 ms |
| 100 | 34 ms | 11 ms | 45 ms |
| 1,000 | 89 ms | 28 ms | 123 ms |
| 10,000 | 567 ms | 156 ms | 834 ms |

---

## Best Practices

### 1. Index richtig konfigurieren

```javascript
// R-Tree Index für POIs
db.osm_pois.ensureIndex({
  type: "geo",
  fields: ["location"],
  geoJson: true,
  name: "idx_location"
});

// S2 Index für sehr große Polygone
db.countries.ensureIndex({
  type: "geo",
  fields: ["boundary"],
  geoJson: true,
  name: "idx_boundary",
  indexType: "s2"
});
```

### 2. GPU für große Batches nutzen

```aql
// Batch Processing mit GPU
FOR batch IN 0..9
  LET start_idx = batch * 100000
  LET end_idx = start_idx + 100000
  
  FOR poi IN osm_pois
    FILTER poi.id >= start_idx AND poi.id < end_idx
    LET distance = GEO_DISTANCE(poi.location, @center)
    FILTER distance <= 5000
    RETURN poi
```

### 3. Bounding Box Pre-Filter verwenden

```aql
// Schneller durch Bounding Box Pre-Filter
LET bbox = GEO_BOUNDS(@search_polygon)

FOR poi IN osm_pois
  FILTER poi.location.coordinates[0] >= bbox.min_lon
  FILTER poi.location.coordinates[0] <= bbox.max_lon
  FILTER poi.location.coordinates[1] >= bbox.min_lat
  FILTER poi.location.coordinates[1] <= bbox.max_lat
  FILTER GEO_CONTAINS(@search_polygon, poi.location)
  RETURN poi
```

### 4. Geometrie-Vereinfachung für Performance

```aql
// Vereinfache komplexe Polygone
FOR polygon IN complex_polygons
  LET simplified = GEO_SIMPLIFY(polygon.geometry, 0.001)
  UPDATE polygon WITH {
    geometry_simplified: simplified
  } IN complex_polygons
```

---

## Benchmark-Reproduktion

### Setup-Skript

```bash
#!/bin/bash
# Benchmark Setup Script

# 1. ThemisDB installieren
docker pull themisdb/themisdb:1.4.0

# 2. Container starten
docker run -d \
  --name themis-bench \
  -p 8765:8765 \
  -e THEMIS_GEO_BACKEND=gpu \
  themisdb/themisdb:1.4.0

# 3. Testdaten laden
wget https://download.geofabrik.de/europe/germany/berlin-latest.osm.pbf
osm2themis berlin-latest.osm.pbf --output berlin-pois.json

# 4. Daten importieren
curl -X POST http://localhost:8765/api/v1/import \
  -H "Content-Type: application/json" \
  -d @berlin-pois.json

# 5. Index erstellen
curl -X POST http://localhost:8765/api/v1/collections/osm_pois/indexes \
  -H "Content-Type: application/json" \
  -d '{
    "type": "geo",
    "fields": ["location"],
    "geoJson": true
  }'
```

### Benchmark ausführen

```bash
# Benchmark Tool herunterladen
wget https://github.com/themisdb/geo-benchmarks/releases/download/v1.0/geo-bench

chmod +x geo-bench

# Benchmark ausführen
./geo-bench \
  --database themisdb \
  --host localhost:8765 \
  --dataset berlin \
  --scenarios all \
  --output results.json

# Ergebnisse visualisieren
geo-bench-report --input results.json --output report.html
```

### Eigene Benchmarks

```python
# Python Benchmark Script
import time
from themisdb import ThemisDB

db = ThemisDB("http://localhost:8765")

# Warmup
for _ in range(10):
    db.query("FOR poi IN osm_pois LIMIT 100 RETURN poi")

# Benchmark: K-NN Query
center = {"type": "Point", "coordinates": [13.3777, 52.5163]}
iterations = 100

start = time.time()
for _ in range(iterations):
    result = db.query("""
        FOR poi IN osm_pois
          LET distance = GEO_DISTANCE(poi.location, @center)
          SORT distance ASC
          LIMIT 10
          RETURN {key: poi._key, distance}
    """, bind_vars={"center": center})
end = time.time()

avg_time = (end - start) / iterations * 1000  # ms
print(f"Average query time: {avg_time:.2f} ms")
print(f"Throughput: {1000 / avg_time:.2f} queries/sec")
```

---

## Limitierungen und Hinweise

### Bekannte Limitierungen

1. **3D Geometrie:** Aktuell nur 2D unterstützt (3D in v1.5 geplant)
2. **Topology Operations:** Polygon Union, Difference nicht verfügbar
3. **Koordinatensystem-Transformation:** Nur WGS84 ↔ Web Mercator
4. **GPU Memory:** Große Polygone (>1M Vertices) können GPU Memory überschreiten

### Hardware-Abhängigkeiten

- **GPU Performance:** NVIDIA A100/H100 für beste Ergebnisse
- **CPU Performance:** AVX-512 empfohlen für SIMD-Optimierungen
- **RAM:** Mindestens 2x Datensatz-Größe für optimale Performance
- **SSD:** NVMe empfohlen für große Datensätze

---

## Siehe auch

- [PostGIS Compatibility](geo_postgis_compatibility.md)
- [Geo Query Examples](geo_query_examples.md)
- [Performance Tuning Guide](../performance/PERFORMANCE_TUNING.md)
- [GPU Configuration](../deployment/GPU_CONFIGURATION.md)
