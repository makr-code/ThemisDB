# Kapitel 14: Geo-Spatial Features

## Einleitung

Standortbasierte Dienste sind aus modernen Anwendungen nicht mehr wegzudenken. Von Lieferdiensten über Immobiliensuche bis zu Flottenmanagement – geografische Daten spielen eine zentrale Rolle. ThemisDB bietet native Unterstützung für Geo-Spatial Daten mit effizienten räumlichen Indizes und umfangreichen Abfragefunktionen.

In diesem Kapitel lernen Sie:
- Geo-Datentypen und Koordinatensysteme
- Räumliche Indizes (R-Tree, Geo-Hash)
- Location-Based Queries (Radius, Bounding Box, Polygon)
- Integration von GPS-Tracking und Kartendiensten
- Best Practices für Geo-Anwendungen

## 14.1 Grundlagen der Geo-Spatial Daten

### Koordinatensysteme

ThemisDB verwendet das WGS84-Koordinatensystem (EPSG:4326), das auch GPS verwendet:

```python
# Koordinaten als [longitude, latitude]
berlin_coordinates = [13.405, 52.520]
munich_coordinates = [11.575, 48.137]

# WICHTIG: Longitude (Längengrad) kommt zuerst!
# Longitude: -180 bis +180 (West/Ost)
# Latitude: -90 bis +90 (Süd/Nord)
```

### Geo-Datentypen in ThemisDB

```aql
-- Point: Einzelner Punkt
CREATE TABLE locations (
    id INTEGER PRIMARY KEY,
    name TEXT,
    coordinates POINT,  -- [lon, lat]
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- LineString: Verbundene Punkte (Routen, Wege)
CREATE TABLE routes (
    id INTEGER PRIMARY KEY,
    name TEXT,
    path LINESTRING,  -- Array von Points
    distance_km REAL
);

-- Polygon: Geschlossene Fläche (Gebiete, Zonen)
CREATE TABLE delivery_zones (
    id INTEGER PRIMARY KEY,
    name TEXT,
    area POLYGON,  -- Array von Points (geschlossen)
    active BOOLEAN DEFAULT TRUE
);
```

## 14.2 Geo-Spatial Indizes

### R-Tree Index

R-Trees sind die effizienteste Indexstruktur für räumliche Daten:

```aql
-- Geo-Index erstellen
CREATE INDEX idx_locations_geo ON locations USING RTREE(coordinates);

-- Automatische Nutzung bei räumlichen Queries
FOR location IN locations 
  FILTER ST_Distance(location.coordinates, POINT(13.405, 52.520)) < 5000
  RETURN location
-- Index wird automatisch genutzt!
```

**R-Tree Eigenschaften:**
- Organisiert Punkte in hierarchischen Bounding Boxes
- O(log n) Suchzeit für Bereichsabfragen
- Automatische Rebalancierung bei Updates
- Optimiert für Nearest-Neighbor Queries

### Geo-Hash Index

Für weltweite Abdeckung und Präfix-Suche:

```aql
CREATE INDEX idx_locations_geohash ON locations USING GEOHASH(coordinates);

-- Nützlich für:
-- - Clustering auf Karten
-- - Hierarchische Zoomlevel
-- - Verteilte Systeme (Sharding nach Geo-Hash)
```

## 14.3 Räumliche Abfragen

### Distanz-Queries

```python
from themisdb import Connection

conn = Connection("localhost:7687")

# Radius-Suche: Alle Restaurants im Umkreis von 2km
restaurants = conn.query("""
    SELECT id, name, coordinates,
           ST_Distance(coordinates, POINT(?, ?)) as distance_m
    FROM restaurants
    WHERE ST_Distance(coordinates, POINT(?, ?)) < 2000
    ORDER BY distance_m
""", [user_lon, user_lat, user_lon, user_lat])

for r in restaurants:
    print(f"{r['name']}: {r['distance_m']:.0f}m entfernt")
```

### Bounding Box Queries

Rechteckiger Bereich (sehr effizient mit R-Tree):

```python
# Berlin-Mitte Bounding Box
min_lon, max_lon = 13.35, 13.45
min_lat, max_lat = 52.50, 52.55

pois = conn.query("""
    FOR poi IN points_of_interest
      FILTER ST_Within(poi.coordinates, 
                      BBOX(@min_lon, @min_lat, @max_lon, @max_lat))
      RETURN poi
""", {"min_lon": min_lon, "min_lat": min_lat, "max_lon": max_lon, "max_lat": max_lat})
```

### Polygon Queries

Prüfen ob Punkt innerhalb eines Polygons liegt:

```python
# Liefergebiet definieren
delivery_area = [
    [13.40, 52.52],  # Punkt 1
    [13.42, 52.52],  # Punkt 2
    [13.42, 52.50],  # Punkt 3
    [13.40, 52.50],  # Punkt 4
    [13.40, 52.52]   # Schließt Polygon
]

# Prüfen ob Adresse im Liefergebiet
customer_location = [13.41, 52.51]

result = conn.query("""
    SELECT ST_Contains(
        POLYGON(?),
        POINT(?, ?)
    ) as in_delivery_area
""", [delivery_area, customer_location[0], customer_location[1]])

if result[0]['in_delivery_area']:
    print("Wir liefern zu Ihnen!")
```

### K-Nearest-Neighbor (KNN)

Die K nächsten Punkte finden:

```python
# 5 nächste Cafés finden
cafes = conn.query("""
    SELECT id, name, coordinates,
           ST_Distance(coordinates, POINT(?, ?)) as distance_m
    FROM cafes
    ORDER BY distance_m
    LIMIT 5
""", [user_lon, user_lat])
```

## 14.4 Geo-Funktionen

### Distanzberechnung

```aql
-- Haversine-Formel (Kugeloberfläche der Erde)
ST_Distance(point1, point2) -> meters

-- Beispiel: Berlin → München
SELECT ST_Distance(
    POINT(13.405, 52.520),  -- Berlin
    POINT(11.575, 48.137)   -- München
) as distance_m;
-- Ergebnis: ~504.000m (504km)
```

### Bearing (Richtung)

```aql
-- Peilung von A nach B in Grad (0° = Norden)
ST_Bearing(point1, point2) -> degrees

SELECT ST_Bearing(
    POINT(13.405, 52.520),  -- Berlin
    POINT(11.575, 48.137)   -- München
) as bearing;
-- Ergebnis: ~195° (Süd-Südwest)
```

### Bounding Box

```aql
-- Kleinste Box um Geometrie
ST_Envelope(geometry) -> bbox

-- Buffer: Bereich um Punkt/Linie
ST_Buffer(point, radius_m) -> polygon
```

## 14.5 Beispiel: Location-Based Services (LBS)

Wir bauen einen Lieferdienst mit Echtzeit-Tracking:

### Datenmodell

```python
# Erstelle Tabellen
conn.execute("""
CREATE TABLE IF NOT EXISTS restaurants (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    cuisine TEXT,
    coordinates POINT NOT NULL,
    rating REAL,
    delivery_radius_m INTEGER DEFAULT 3000
)
""")

conn.execute("""
CREATE INDEX idx_restaurants_geo 
ON restaurants USING RTREE(coordinates)
""")

conn.execute("""
CREATE TABLE IF NOT EXISTS orders (
    id INTEGER PRIMARY KEY,
    restaurant_id INTEGER REFERENCES restaurants(id),
    customer_name TEXT,
    delivery_address POINT NOT NULL,
    status TEXT DEFAULT 'pending',
    driver_id INTEGER,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
)
""")

conn.execute("""
CREATE TABLE IF NOT EXISTS drivers (
    id INTEGER PRIMARY KEY,
    name TEXT,
    current_location POINT,
    status TEXT DEFAULT 'available',
    last_update TIMESTAMP
)
""")

conn.execute("""
CREATE INDEX idx_drivers_geo 
ON drivers USING RTREE(current_location)
""")
```

### Restaurant-Suche

```python
def find_nearby_restaurants(user_lat, user_lon, cuisine=None, max_distance_m=5000):
    """Findet Restaurants in der Nähe"""
    
    query = """
        SELECT id, name, cuisine, coordinates,
               rating,
               ST_Distance(coordinates, POINT(?, ?)) as distance_m
        FROM restaurants
        WHERE ST_Distance(coordinates, POINT(?, ?)) < ?
    """
    
    params = [user_lon, user_lat, user_lon, user_lat, max_distance_m]
    
    if cuisine:
        query += " AND cuisine = ?"
        params.append(cuisine)
    
    query += " ORDER BY distance_m"
    
    return conn.query(query, params)

# Verwendung
restaurants = find_nearby_restaurants(
    user_lat=52.520,
    user_lon=13.405,
    cuisine="Italian",
    max_distance_m=3000
)

for r in restaurants:
    distance_km = r['distance_m'] / 1000
    print(f"{r['name']}: {distance_km:.1f}km, Rating: {r['rating']}/5")
```

### Fahrer-Zuordnung

```python
def assign_nearest_driver(order_id):
    """Findet den nächsten verfügbaren Fahrer"""
    
    # Hole Bestelladresse
    order = conn.query("""
        FOR order IN orders 
          FILTER order.id == @order_id 
          LIMIT 1 
          RETURN order.delivery_address
    """, {"order_id": order_id})[0]
    
    delivery_loc = order
    
    # Finde nächsten verfügbaren Fahrer
    driver = conn.query("""
        FOR driver IN drivers
          FILTER driver.status == 'available'
          LET distance_m = ST_Distance(driver.current_location, POINT(@lon, @lat))
          SORT distance_m ASC
          LIMIT 1
          RETURN {
            id: driver.id,
            name: driver.name,
            current_location: driver.current_location,
            distance_m
          }
    """, {"lon": delivery_loc[0], "lat": delivery_loc[1]})
    
    if not driver:
        raise ValueError("Kein Fahrer verfügbar")
    
    driver = driver[0]
    
    # Zuordnen
    conn.execute("""
        UPDATE orders SET driver_id = ?, status = 'assigned'
        WHERE id = ?
    """, [driver['id'], order_id])
    
    conn.execute("""
        UPDATE drivers SET status = 'busy' WHERE id = ?
    """, [driver['id']])
    
    return driver

# Verwendung
driver = assign_nearest_driver(order_id=123)
print(f"Fahrer {driver['name']} zugewiesen")
```

### Live-Tracking

```python
import time
from datetime import datetime

def update_driver_location(driver_id, lat, lon):
    """Aktualisiert Fahrerposition (z.B. alle 5 Sekunden)"""
    
    conn.execute("""
        UPDATE drivers
        SET current_location = POINT(?, ?),
            last_update = ?
        WHERE id = ?
    """, [lon, lat, datetime.now(), driver_id])

def get_delivery_eta(order_id):
    """Schätzt Ankunftszeit"""
    
    result = conn.query("""
        SELECT o.id, o.delivery_address,
               d.current_location,
               ST_Distance(d.current_location, o.delivery_address) as distance_m
        FROM orders o
        JOIN drivers d ON o.driver_id = d.id
        WHERE o.id = ?
    """, [order_id])
    
    if not result:
        return None
    
    data = result[0]
    distance_km = data['distance_m'] / 1000
    
    # Annahme: 30 km/h in der Stadt
    eta_minutes = (distance_km / 30) * 60
    
    return {
        'distance_km': round(distance_km, 1),
        'eta_minutes': round(eta_minutes),
        'driver_location': data['current_location']
    }

# Verwendung
eta = get_delivery_eta(order_id=123)
print(f"Fahrer ist noch {eta['distance_km']}km entfernt")
print(f"ETA: {eta['eta_minutes']} Minuten")
```

## 14.6 Beispiel: Immobiliensuche

Geo-basierte Immobiliensuche mit Filterung:

### Datenmodell

```python
conn.execute("""
CREATE TABLE IF NOT EXISTS properties (
    id INTEGER PRIMARY KEY,
    title TEXT NOT NULL,
    address TEXT,
    coordinates POINT NOT NULL,
    price_eur INTEGER,
    size_sqm REAL,
    rooms INTEGER,
    type TEXT,  -- 'apartment', 'house', 'commercial'
    features JSON  -- ['balcony', 'parking', 'elevator', ...]
)
""")

conn.execute("""
CREATE INDEX idx_properties_geo 
ON properties USING RTREE(coordinates)
""")
```

### Radius-Suche mit Filtern

```python
def search_properties(
    center_lat, center_lon, radius_m=2000,
    min_price=None, max_price=None,
    min_rooms=None, property_type=None,
    required_features=None
):
    """Immobiliensuche mit Geo + Filter"""
    
    query = """
        SELECT id, title, address, coordinates, 
               price_eur, size_sqm, rooms, type, features,
               ST_Distance(coordinates, POINT(?, ?)) as distance_m
        FROM properties
        WHERE ST_Distance(coordinates, POINT(?, ?)) < ?
    """
    
    params = [center_lon, center_lat, center_lon, center_lat, radius_m]
    
    if min_price:
        query += " AND price_eur >= ?"
        params.append(min_price)
    
    if max_price:
        query += " AND price_eur <= ?"
        params.append(max_price)
    
    if min_rooms:
        query += " AND rooms >= ?"
        params.append(min_rooms)
    
    if property_type:
        query += " AND type = ?"
        params.append(property_type)
    
    query += " ORDER BY distance_m"
    
    results = conn.query(query, params)
    
    # Nachfilterung für Features (JSON)
    if required_features:
        filtered = []
        for prop in results:
            prop_features = set(prop['features'] or [])
            if all(f in prop_features for f in required_features):
                filtered.append(prop)
        results = filtered
    
    return results

# Verwendung
properties = search_properties(
    center_lat=52.520,
    center_lon=13.405,
    radius_m=3000,
    min_price=500_000,
    max_price=800_000,
    min_rooms=3,
    property_type='apartment',
    required_features=['balcony', 'parking']
)

for p in properties:
    print(f"{p['title']}: {p['price_eur']:,}€, {p['rooms']} Zimmer")
    print(f"  {p['distance_m']/1000:.1f}km entfernt")
```

### POI-basierte Suche

Immobilien in der Nähe von Points of Interest:

```python
def search_near_poi(poi_name, radius_m=1000):
    """Immobilien in der Nähe eines POI"""
    
    # Finde POI
    poi = conn.query("""
        FOR poi IN points_of_interest
          FILTER poi.name == @poi_name
          LIMIT 1
          RETURN poi.coordinates
    """, {"poi_name": poi_name})
    
    if not poi:
        raise ValueError(f"POI '{poi_name}' nicht gefunden")
    
    poi_coords = poi[0]['coordinates']
    
    # Suche Immobilien
    return conn.query("""
        SELECT id, title, price_eur,
               ST_Distance(coordinates, POINT(?, ?)) as distance_m
        FROM properties
        WHERE ST_Distance(coordinates, POINT(?, ?)) < ?
        ORDER BY distance_m
    """, [poi_coords[0], poi_coords[1], 
          poi_coords[0], poi_coords[1], radius_m])

# Verwendung: Wohnungen nahe "Alexanderplatz"
properties = search_near_poi("Alexanderplatz", radius_m=1500)
```

## 14.7 Best Practices

### 1. Index-Design

```python
# ✅ GUT: R-Tree Index für räumliche Queries
CREATE INDEX idx_geo ON locations USING RTREE(coordinates);

# ✅ GUT: Composite Index für Geo + Filter
CREATE INDEX idx_geo_type ON locations(type, coordinates) USING RTREE;

# ❌ SCHLECHT: Kein Geo-Index
CREATE INDEX idx_coords ON locations(coordinates);  # B-Tree, ineffizient!
```

### 2. Query-Optimierung

```python
# ✅ GUT: Nutze Index mit Distanz-Filter
WHERE ST_Distance(coordinates, ?) < radius

# ❌ SCHLECHT: Distanz in SELECT ohne Filter
FOR location IN locations
  LET dist = ST_Distance(location.coordinates, @point)
  RETURN {location, dist}
-- Keine Index-Nutzung! Alle Rows werden berechnet
```

### 3. Koordinaten-Validierung

```python
def validate_coordinates(lon, lat):
    """Prüft ob Koordinaten gültig sind"""
    if not (-180 <= lon <= 180):
        raise ValueError(f"Longitude {lon} außerhalb [-180, 180]")
    if not (-90 <= lat <= 90):
        raise ValueError(f"Latitude {lat} außerhalb [-90, 90]")
    return True

# Verwende vor dem Speichern
validate_coordinates(lon, lat)
```

### 4. Präzision und Rundung

```python
# ✅ GUT: 6 Dezimalstellen (~10cm Genauigkeit)
coordinates = [round(lon, 6), round(lat, 6)]

# Zu viele Dezimalstellen bringen keine Vorteile:
# 5 Dezimalstellen: ~1m Genauigkeit
# 6 Dezimalstellen: ~10cm Genauigkeit
# 7 Dezimalstellen: ~1cm Genauigkeit (meist unnötig)
```

### 5. Caching für häufige Queries

```python
from functools import lru_cache
import time

@lru_cache(maxsize=1000)
def get_cached_nearby_restaurants(lat, lon, radius):
    """Cached Restaurant-Suche"""
    # Cache für 5 Minuten
    cache_key = (round(lat, 3), round(lon, 3), radius)
    return find_nearby_restaurants(lat, lon, max_distance_m=radius)

# Bei wiederholten Anfragen aus gleicher Region: Cache-Hit!
```

## 14.8 Performance-Tipps

### 1. Bounding Box vor Distanz-Berechnung

```python
# ✅ EFFIZIENT: Erst grobe Filterung, dann genaue Distanz
FOR location IN locations
  FILTER ST_Within(location.coordinates, BBOX(@min_lon, @min_lat, @max_lon, @max_lat))  -- Schneller R-Tree Lookup
    AND ST_Distance(location.coordinates, POINT(@lon, @lat)) < @radius  -- Nur für Kandidaten
  RETURN location
```

### 2. Limit verwenden

```python
# Wenn nur Top-N benötigt:
FOR location IN locations
  FILTER ST_Distance(location.coordinates, POINT(@lon, @lat)) < 5000
  LET dist = ST_Distance(location.coordinates, POINT(@lon, @lat))
  SORT dist ASC
  LIMIT 10  -- Stoppt nach 10 Ergebnissen
  RETURN location
```

### 3. Materializedviews für häufige Gebiete

```python
# Erstelle View für "beliebtes Gebiet"
conn.execute("""
CREATE MATERIALIZED VIEW berlin_mitte_restaurants AS
FOR restaurant IN restaurants
  FILTER ST_Within(restaurant.coordinates, BBOX(13.35, 52.50, 13.45, 52.55))
  RETURN restaurant
""")

# Refresh periodisch (z.B. stündlich)
conn.execute("REFRESH MATERIALIZED VIEW berlin_mitte_restaurants")
```

## 14.9 Vergleich mit spezialisierten Geo-Systemen

| Feature | ThemisDB | PostGIS | MongoDB Geo |
|---------|----------|---------|-------------|
| **R-Tree Index** | ✅ Native | ✅ GiST | ✅ 2dsphere |
| **Distanz-Queries** | ✅ | ✅ | ✅ |
| **Polygon-Support** | ✅ | ✅✅ (komplex) | ✅ |
| **3D-Geometrie** | ❌ | ✅ | ❌ |
| **Koordinatensysteme** | WGS84 | Viele | WGS84 |
| **Performance** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Multi-Model** | ✅✅✅ | ❌ | ✅ |
| **Einfachheit** | ✅✅✅ | ⭐⭐ | ✅✅ |

**Empfehlung:**
- **ThemisDB:** Gut für die meisten Anwendungen, besonders mit Multi-Model Daten
- **PostGIS:** Beste Wahl für komplexe GIS-Anwendungen
- **MongoDB:** Gut für Dokument + Geo Kombination

## 14.10 Übungen

### Übung 1: Store Locator
Implementieren Sie einen Store Locator mit:
- Nächste 5 Filialen
- Öffnungszeiten-Filterung
- Routenvorschlag

### Übung 2: Geofencing
Erstellen Sie ein Geofencing-System:
- Definieren Sie virtuelle Zonen (Polygone)
- Trigger beim Ein/Austritt
- Benachrichtigungen

### Übung 3: Heat Map
Generieren Sie eine Heat Map:
- Aggregiere Punkte nach Geo-Hash
- Cluster für verschiedene Zoom-Level
- Export für Mapping-Tools

## Zusammenfassung

In diesem Kapitel haben Sie gelernt:

✅ **Geo-Datentypen:** Point, LineString, Polygon
✅ **Räumliche Indizes:** R-Tree für effiziente Geo-Queries
✅ **Abfragen:** Radius, Bounding Box, KNN, Polygon
✅ **Praxis-Beispiele:** Lieferdienst, Immobiliensuche
✅ **Best Practices:** Index-Design, Performance, Validation

Geo-Spatial Features in ThemisDB bieten eine solide Grundlage für Location-Based Services ohne externe Geo-Datenbank. Die native Integration mit anderen Datenmodellen (Relational, Graph, Dokument, Vector) macht ThemisDB zur idealen Plattform für moderne Geo-Anwendungen.

Im nächsten Kapitel sehen wir uns **Analytics & Reporting** an – Aggregationen, Dashboards und Business Intelligence mit ThemisDB.
