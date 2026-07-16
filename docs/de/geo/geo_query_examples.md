# Geospatial Query-Beispiele für ThemisDB

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Geo  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [Grundlegende Queries](#grundlegende-queries)
- [Räumliche Joins](#räumliche-joins)
- [Proximity Search](#proximity-search)
- [Polygon-Operationen](#polygon-operationen)
- [Routing und Pfade](#routing-und-pfade)
- [Komplexe Szenarien](#komplexe-szenarien)
- [Performance-Optimierung](#performance-optimierung)

---

## Grundlegende Queries

### Punkt-in-Polygon Abfrage

Finde alle Punkte innerhalb eines bestimmten Polygons (z.B. Stadt-Grenze).

```aql
// Beispiel: Restaurants in Berlin Mitte
LET berlin_mitte = GEO_POLYGON([
  [13.37, 52.52],
  [13.41, 52.52],
  [13.41, 52.54],
  [13.37, 52.54],
  [13.37, 52.52]
])

FOR restaurant IN restaurants
  FILTER GEO_CONTAINS(berlin_mitte, restaurant.location)
  RETURN {
    name: restaurant.name,
    location: restaurant.location,
    rating: restaurant.rating
  }
```

### Distanz-Berechnung

Berechne die Entfernung zwischen zwei Punkten.

```aql
// Beispiel: Entfernung zwischen zwei Standorten
LET brandenburger_tor = GEO_POINT(13.3777, 52.5163)
LET reichstag = GEO_POINT(13.3761, 52.5186)

LET distance_meters = GEO_DISTANCE(brandenburger_tor, reichstag)
LET distance_km = distance_meters / 1000

RETURN {
  from: "Brandenburger Tor",
  to: "Reichstag",
  distance_meters,
  distance_km
}
```

### Intersects-Abfrage

Finde überlappende Geometrien.

```aql
// Beispiel: Gebäude die Straßen schneiden
LET hauptstrasse = GEO_LINESTRING([
  [13.40, 52.51],
  [13.42, 52.52],
  [13.43, 52.53]
])

FOR building IN buildings
  FILTER GEO_INTERSECTS(building.footprint, hauptstrasse)
  RETURN {
    name: building.name,
    address: building.address,
    footprint: building.footprint
  }
```

---

## Räumliche Joins

### Join: POIs und Distrikte

Verknüpfe Points of Interest mit ihren zugehörigen Stadtteilen.

```aql
// Finde alle Cafés mit ihrem Stadtteil
FOR poi IN points_of_interest
  FILTER poi.category == "cafe"
  
  FOR district IN districts
    FILTER GEO_CONTAINS(district.boundary, poi.location)
    
    RETURN {
      poi_name: poi.name,
      poi_address: poi.address,
      district_name: district.name,
      district_population: district.population
    }
```

### Spatial Join mit Aggregation

Zähle POIs pro Stadtteil.

```aql
FOR district IN districts
  LET poi_count = COUNT(
    FOR poi IN points_of_interest
      FILTER GEO_CONTAINS(district.boundary, poi.location)
      RETURN 1
  )
  
  LET cafe_count = COUNT(
    FOR poi IN points_of_interest
      FILTER poi.category == "cafe"
      FILTER GEO_CONTAINS(district.boundary, poi.location)
      RETURN 1
  )
  
  RETURN {
    district: district.name,
    total_pois: poi_count,
    cafes: cafe_count,
    poi_density: poi_count / district.area_km2
  }
```

### Multi-Level Spatial Join

Hierarchischer Join: Stadt → Distrikt → POI.

```aql
FOR city IN cities
  FILTER city.name == "Berlin"
  
  LET districts_data = (
    FOR district IN districts
      FILTER GEO_CONTAINS(city.boundary, district.centroid)
      
      LET pois = (
        FOR poi IN points_of_interest
          FILTER GEO_CONTAINS(district.boundary, poi.location)
          RETURN {
            name: poi.name,
            category: poi.category
          }
      )
      
      RETURN {
        district_name: district.name,
        poi_count: LENGTH(pois),
        pois: pois
      }
  )
  
  RETURN {
    city: city.name,
    districts: districts_data
  }
```

---

## Proximity Search

### Nächste Nachbarn (K-NN)

Finde die 10 nächsten POIs zu einem gegebenen Punkt.

```aql
LET user_location = GEO_POINT(13.405, 52.520)

FOR poi IN points_of_interest
  LET distance = GEO_DISTANCE(poi.location, user_location)
  SORT distance ASC
  LIMIT 10
  RETURN {
    name: poi.name,
    category: poi.category,
    distance_meters: distance,
    distance_km: ROUND(distance / 1000, 2)
  }
```

### Radius Search mit Filter

Finde alle Restaurants im 2km Radius mit Mindest-Rating.

```aql
LET center = GEO_POINT(13.405, 52.520)
LET radius = 2000  // 2 km in Metern

FOR restaurant IN restaurants
  LET distance = GEO_DISTANCE(restaurant.location, center)
  FILTER distance <= radius
  FILTER restaurant.rating >= 4.0
  SORT distance ASC
  RETURN {
    name: restaurant.name,
    rating: restaurant.rating,
    distance_meters: distance,
    cuisine: restaurant.cuisine
  }
```

### Isochrone Query (Travel Time)

Finde alle erreichbaren Punkte innerhalb einer bestimmten Reisezeit.

```aql
// Pseudo-Code: Erreichbare POIs in 15 Minuten zu Fuß (ca. 1.2 km)
LET start = GEO_POINT(13.405, 52.520)
LET walking_speed = 5  // km/h
LET time_minutes = 15
LET max_distance = (walking_speed * 1000 / 60) * time_minutes  // ~1250 Meter

FOR poi IN points_of_interest
  LET distance = GEO_DISTANCE(poi.location, start)
  FILTER distance <= max_distance
  LET travel_time_min = (distance / 1000) / walking_speed * 60
  SORT travel_time_min ASC
  RETURN {
    name: poi.name,
    category: poi.category,
    distance_meters: distance,
    estimated_walking_time_min: ROUND(travel_time_min, 0)
  }
```

---

## Polygon-Operationen

### Buffer-Operation

Erstelle einen Puffer um einen Punkt oder Linie.

```aql
// Beispiel: 500m Puffer um eine S-Bahn Station
LET station = GEO_POINT(13.405, 52.520)
LET buffer = GEO_BUFFER(station, 500)  // 500 Meter Radius

FOR poi IN points_of_interest
  FILTER GEO_CONTAINS(buffer, poi.location)
  RETURN {
    name: poi.name,
    category: poi.category
  }
```

### Convex Hull

Berechne die konvexe Hülle einer Punktmenge.

```aql
// Beispiel: Konvexe Hülle aller Filialen einer Kette
LET chain_locations = (
  FOR poi IN points_of_interest
    FILTER poi.brand == "Starbucks"
    RETURN poi.location
)

LET convex_hull = GEO_CONVEX_HULL(chain_locations)

RETURN {
  brand: "Starbucks",
  locations_count: LENGTH(chain_locations),
  coverage_area: convex_hull,
  area_km2: GEO_AREA(convex_hull) / 1000000
}
```

### Polygon Simplification

Vereinfache komplexe Polygone zur schnelleren Verarbeitung.

```aql
FOR district IN districts
  LET simplified = GEO_SIMPLIFY(district.boundary, 0.001)  // Tolerance
  LET original_points = LENGTH(GEO_COORDINATES(district.boundary))
  LET simplified_points = LENGTH(GEO_COORDINATES(simplified))
  
  UPDATE district WITH {
    boundary_simplified: simplified,
    optimization_ratio: simplified_points / original_points
  } IN districts
  
  RETURN {
    district: district.name,
    original_points,
    simplified_points,
    reduction: ROUND((1 - simplified_points / original_points) * 100, 1)
  }
```

---

## Routing und Pfade

### Nächster Punkt auf Straße

Finde den nächsten Punkt auf einer Straße (Snap to Road).

```aql
LET user_location = GEO_POINT(13.405, 52.520)

FOR road IN roads
  LET closest_point = GEO_CLOSEST_POINT(road.geometry, user_location)
  LET distance = GEO_DISTANCE(user_location, closest_point)
  SORT distance ASC
  LIMIT 1
  RETURN {
    road_name: road.name,
    snapped_location: closest_point,
    distance_to_road: distance
  }
```

### LineString Interpolation

Berechne Punkte entlang einer Route.

```aql
// Beispiel: Gleichmäßig verteilte Punkte entlang einer Route
LET route = GEO_LINESTRING([
  [13.40, 52.51],
  [13.42, 52.52],
  [13.44, 52.53],
  [13.46, 52.54]
])

LET total_length = GEO_LENGTH(route)
LET num_points = 10

FOR i IN 0..num_points-1
  LET fraction = i / (num_points - 1)
  LET point = GEO_INTERPOLATE(route, fraction)
  
  RETURN {
    index: i,
    fraction,
    point,
    distance_from_start: fraction * total_length
  }
```

---

## Komplexe Szenarien

### Heatmap-Daten generieren

Berechne POI-Dichte für Heatmap-Visualisierung.

```aql
// Raster-basierte Heatmap (100x100m Zellen)
LET bbox = {
  min_lon: 13.35,
  max_lon: 13.45,
  min_lat: 52.48,
  max_lat: 52.55
}

LET cell_size = 0.001  // ~100 Meter
LET lon_steps = FLOOR((bbox.max_lon - bbox.min_lon) / cell_size)
LET lat_steps = FLOOR((bbox.max_lat - bbox.min_lat) / cell_size)

FOR lon_i IN 0..lon_steps
  FOR lat_i IN 0..lat_steps
    LET cell_lon = bbox.min_lon + lon_i * cell_size
    LET cell_lat = bbox.min_lat + lat_i * cell_size
    
    LET cell_polygon = GEO_POLYGON([
      [cell_lon, cell_lat],
      [cell_lon + cell_size, cell_lat],
      [cell_lon + cell_size, cell_lat + cell_size],
      [cell_lon, cell_lat + cell_size],
      [cell_lon, cell_lat]
    ])
    
    LET poi_count = COUNT(
      FOR poi IN points_of_interest
        FILTER GEO_CONTAINS(cell_polygon, poi.location)
        RETURN 1
    )
    
    FILTER poi_count > 0
    
    RETURN {
      cell: {lon: cell_lon, lat: cell_lat},
      poi_count,
      centroid: GEO_CENTROID(cell_polygon)
    }
```

### Gebiet-Überschneidungs-Analyse

Analysiere Überschneidungen zwischen verschiedenen Zonen.

```aql
// Beispiel: Welche Schutzgebiete überschneiden sich mit Baugebieten?
FOR protected IN protected_areas
  LET overlapping_construction = (
    FOR construction IN construction_zones
      FILTER GEO_INTERSECTS(protected.boundary, construction.boundary)
      
      // Berechne Überschneidungs-Fläche (vereinfacht)
      RETURN {
        zone_name: construction.name,
        zone_type: construction.type
      }
  )
  
  FILTER LENGTH(overlapping_construction) > 0
  
  RETURN {
    protected_area: protected.name,
    conflicts: overlapping_construction,
    conflict_count: LENGTH(overlapping_construction),
    priority: protected.protection_level
  }
```

### Multi-Criteria Standort-Suche

Finde optimale Standorte basierend auf mehreren räumlichen Kriterien.

```aql
// Beispiel: Ideale Standorte für neues Café
// Kriterien: Nähe zu U-Bahn, hohe Fußgängerfrequenz, wenig Konkurrenz

LET candidate_points = (
  FOR station IN subway_stations
    LET buffer = GEO_BUFFER(station.location, 300)  // 300m Radius
    RETURN buffer
)

FOR candidate IN candidate_points
  LET nearby_cafes = COUNT(
    FOR cafe IN points_of_interest
      FILTER cafe.category == "cafe"
      FILTER GEO_CONTAINS(candidate, cafe.location)
      RETURN 1
  )
  
  LET foot_traffic = (
    FOR traffic IN foot_traffic_data
      FILTER GEO_INTERSECTS(candidate, traffic.area)
      RETURN traffic.daily_count
  )[0] OR 0
  
  // Scoring
  LET competition_score = nearby_cafes < 3 ? 10 : MAX([0, 10 - nearby_cafes * 2])
  LET traffic_score = MIN([10, foot_traffic / 1000])
  LET total_score = competition_score + traffic_score
  
  FILTER total_score > 12  // Nur gute Standorte
  
  SORT total_score DESC
  LIMIT 20
  
  RETURN {
    location: GEO_CENTROID(candidate),
    nearby_cafes,
    foot_traffic,
    competition_score,
    traffic_score,
    total_score
  }
```

---

## Performance-Optimierung

### 1. Index-Nutzung erzwingen

```aql
// ✅ Gut: Index wird verwendet
FOR poi IN points_of_interest
  FILTER GEO_DISTANCE(poi.location, @center) < 1000
  RETURN poi

// ❌ Schlecht: Full Scan
FOR poi IN points_of_interest
  LET distance = GEO_DISTANCE(poi.location, @center)
  FILTER distance < 1000
  RETURN poi
```

### 2. Bounding Box Pre-Filter

```aql
// Pre-Filter mit Bounding Box für bessere Performance
LET center = GEO_POINT(13.405, 52.520)
LET radius = 5000

// Bounding Box berechnen (vereinfacht)
LET lat_delta = radius / 111000  // ~111km pro Grad Latitude
LET lon_delta = radius / (111000 * COS(52.520 * PI() / 180))

FOR poi IN points_of_interest
  // Pre-Filter: Schneller Bounding Box Check
  FILTER poi.location.coordinates[0] >= center.coordinates[0] - lon_delta
  FILTER poi.location.coordinates[0] <= center.coordinates[0] + lon_delta
  FILTER poi.location.coordinates[1] >= center.coordinates[1] - lat_delta
  FILTER poi.location.coordinates[1] <= center.coordinates[1] + lat_delta
  
  // Exakte Distanz-Prüfung
  LET distance = GEO_DISTANCE(poi.location, center)
  FILTER distance <= radius
  
  RETURN {name: poi.name, distance}
```

### 3. Materialized Views für häufige Queries

```aql
// View erstellen (einmalig)
db._createView("nearby_restaurants_view", "arangosearch", {
  links: {
    restaurants: {
      analyzers: ["identity"],
      fields: {
        location: {
          analyzers: ["geojson"]
        }
      }
    }
  }
});

// View nutzen
FOR doc IN nearby_restaurants_view
  SEARCH ANALYZER(
    GEO_DISTANCE(doc.location, GEO_POINT(13.405, 52.520)) < 1000,
    "geojson"
  )
  RETURN doc
```

---

## Siehe auch

- [PostGIS Compatibility](geo_postgis_compatibility.md)
- [Geo Benchmarks](geo_benchmarks.md)
- [Geo Index Guide](../features/FEATURE_GEO_INDEX.md)
- [AQL Reference](../aql/AQL_REFERENCE.md)
