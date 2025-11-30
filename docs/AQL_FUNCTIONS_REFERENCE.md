# AQL Functions Reference

> **ThemisDB Query Language** - Die einzige Abfragesprache, die Graph, Vector, Relational, Geo und File in einer einheitlichen Syntax vereint.

## Inhaltsverzeichnis

1. [Alleinstellungsmerkmale](#alleinstellungsmerkmale)
2. [Vergleich mit anderen Datenbanken](#vergleich-mit-anderen-datenbanken)
3. [Funktionskategorien](#funktionskategorien)
4. [String-Funktionen](#string-funktionen)
5. [Math-Funktionen](#math-funktionen)
6. [Array-Funktionen](#array-funktionen)
7. [Date-Funktionen](#date-funktionen)
8. [Document-Funktionen](#document-funktionen)
9. [Geo-Funktionen](#geo-funktionen)
10. [CRS-Funktionen (Koordinatentransformation)](#crs-funktionen)
11. [Vector-Funktionen](#vector-funktionen)
12. [Graph-Funktionen](#graph-funktionen)
13. [Relational-Funktionen](#relational-funktionen)
14. [File-Funktionen](#file-funktionen)
15. [FAQ](#faq)

---

## Alleinstellungsmerkmale

### 🎯 Was macht ThemisDB einzigartig?

| Feature | ThemisDB | Neo4j | PostgreSQL | MongoDB | Pinecone |
|---------|----------|-------|------------|---------|----------|
| **Unified Query Language** | ✅ Eine Syntax für alles | ❌ Cypher only | ❌ SQL only | ❌ MQL only | ❌ API only |
| **Native Graph + Vector** | ✅ Integriert | ❌ Plugin | ❌ Extension | ❌ Atlas Search | ✅ Vector only |
| **Geo + Graph kombiniert** | ✅ ST_* + SHORTEST_PATH | ❌ Separat | ✅ PostGIS | ✅ GeoJSON | ❌ |
| **BPMN Process Mining** | ✅ Native | ❌ | ❌ | ❌ | ❌ |
| **CRS Transformation** | ✅ ETRS89/UTM/WGS84 | ❌ | ✅ PostGIS | ❌ | ❌ |
| **Multi-Model in einer Query** | ✅ | ❌ | ❌ | ❌ | ❌ |

### 🚀 Die "Killer-Features"

#### 1. Multi-Model Queries in einer Zeile

```aql
-- Finde Kunden in der Nähe, mit ähnlichen Interessen, über Empfehlungsnetzwerk
FOR customer IN customers
  FILTER GEO_DISTANCE(customer.location, @myLocation) < 10000
  LET similar = SIMILARITY(customer.interests_vector, @myInterests, 0.8)
  FOR friend IN 1..3 OUTBOUND customer knows
    FILTER friend.active == true
  RETURN { customer, similarity: similar, connection: friend }
```

**Das gleiche in anderen Systemen würde erfordern:**
- PostgreSQL: 3 separate Queries + Application Join
- Neo4j + Pinecone: 2 Systeme + API-Calls
- MongoDB: Aggregation Pipeline + Atlas Search + \$graphLookup (komplex!)

#### 2. Prozess-Mining aus Dokumenten

```aql
-- Entdecke Prozesse aus Event-Logs
LET events = (
  FOR e IN audit_logs
    FILTER e.timestamp >= DATE_SUBTRACT(DATE_NOW(), 30, "days")
    SORT e.case_id, e.timestamp
    RETURN e
)
LET process = DISCOVER_PROCESS(events, "case_id", "activity", "timestamp")
RETURN {
  activities: process.activities,
  transitions: process.transitions,
  variants: process.variants,
  bottlenecks: process.bottlenecks
}
```

#### 3. Koordinatentransformation on-the-fly

```aql
-- Transformiere UTM-Koordinaten (Vermessungsdaten) zu WGS84 (GPS)
FOR parcel IN land_parcels
  LET wgs84 = ST_TRANSFORM(parcel.geometry, 25832, 4326)  -- ETRS89/UTM32 → WGS84
  LET center = ST_CENTROID(wgs84)
  RETURN {
    id: parcel.id,
    area_sqm: ST_AREA(parcel.geometry),
    center_lat: ST_Y(center),
    center_lon: ST_X(center)
  }
```

---

## Vergleich mit anderen Datenbanken

### ThemisDB vs. Neo4j (Cypher)

| Aufgabe | ThemisDB AQL | Neo4j Cypher |
|---------|--------------|--------------|
| **Pfadsuche** | `FOR v IN 1..5 OUTBOUND start knows RETURN v` | `MATCH (start)-[:knows*1..5]->(v) RETURN v` |
| **Mit Geo-Filter** | `FILTER GEO_DISTANCE(v.loc, @point) < 1000` | ❌ Nicht möglich ohne Plugin |
| **Mit Vector-Similarity** | `LET sim = COSINE_SIMILARITY(v.emb, @vec)` | ❌ Braucht externes System |
| **Shortest Path** | `SHORTEST_PATH(a, b, "knows")` | `shortestPath((a)-[:knows*]-(b))` |

### ThemisDB vs. PostgreSQL

| Aufgabe | ThemisDB AQL | PostgreSQL |
|---------|--------------|------------|
| **JSON-Dokumente** | Native | `jsonb` Typ |
| **Graph-Traversal** | `FOR v IN OUTBOUND` | `WITH RECURSIVE` (komplex!) |
| **Vector-Search** | `SIMILARITY()` | `pgvector` Extension |
| **Geo-Operationen** | `ST_*` Funktionen | PostGIS Extension |
| **Alles kombiniert** | ✅ Eine Query | ❌ Mehrere Queries/CTEs |

### ThemisDB vs. MongoDB

| Aufgabe | ThemisDB AQL | MongoDB |
|---------|--------------|---------|
| **Syntax** | SQL-ähnlich, lesbar | JSON-basiert, verschachtelt |
| **Graph** | Native Traversal | `\$graphLookup` (limitiert) |
| **Aggregation** | `COLLECT`, `AGGREGATE` | Pipeline Stages |
| **Joins** | `FOR ... FOR` | `\$lookup` |
| **Window Functions** | `ROW_NUMBER`, `LAG`, `LEAD` | ❌ Nicht verfügbar |

**Beispiel - Gruppierung mit Ranking:**

```aql
-- ThemisDB: Klar und lesbar
FOR order IN orders
  COLLECT customer = order.customer_id
  AGGREGATE total = SUM(order.amount), count = COUNT(1)
  LET rank = ROW_NUMBER() OVER (ORDER BY total DESC)
  FILTER rank <= 10
  RETURN { customer, total, count, rank }
```

```javascript
// MongoDB: Verschachtelt und komplex
db.orders.aggregate([
  { \$group: { _id: "\$customer_id", total: { \$sum: "\$amount" }, count: { \$sum: 1 } } },
  { \$sort: { total: -1 } },
  { \$limit: 10 },
  { \$project: { customer: "\$_id", total: 1, count: 1 } }
])
// Hinweis: ROW_NUMBER() nicht nativ verfügbar!
```

---

## Funktionskategorien

ThemisDB bietet **~210 Funktionen** in **11 Kategorien**:

| Kategorie | Anzahl | Beschreibung |
|-----------|--------|--------------|
| String | ~15 | Textmanipulation, Pattern Matching |
| Math | ~25 | Arithmetik, Trigonometrie, Statistik |
| Array | ~20 | Listen-Operationen |
| Date | ~15 | Datum/Zeit-Verarbeitung |
| Document | ~20 | Objektmanipulation, Typ-Prüfungen |
| Geo | ~25 | Räumliche Operationen (OGC-kompatibel) |
| CRS | ~10 | Koordinatentransformationen |
| Vector | ~20 | ML-Embeddings, Ähnlichkeitssuche |
| Graph | ~15 | Traversierung, Zentralität, Pfade |
| Relational | ~25 | SQL-Joins, Aggregation, Window |
| File | ~20 | Pfade, MIME-Typen, Dateigrößen |

---

## String-Funktionen

### LENGTH(value)
Gibt die Länge eines Strings, Arrays oder Objekts zurück.

```aql
-- String-Länge
RETURN LENGTH("Hello World")  -- 11

-- Array-Länge
RETURN LENGTH([1, 2, 3, 4, 5])  -- 5

-- Objekt-Eigenschaften zählen
RETURN LENGTH({ name: "Max", age: 30 })  -- 2
```

### CONCAT(...values)
Verbindet mehrere Werte zu einem String.

```aql
-- Einfache Verkettung
RETURN CONCAT("Hello", " ", "World")  -- "Hello World"

-- Mit Variablen
FOR user IN users
  RETURN CONCAT(user.firstName, " ", user.lastName)
```

### SUBSTRING(str, start, length?)
Extrahiert einen Teilstring.

```aql
RETURN SUBSTRING("ThemisDB", 0, 6)  -- "Themis"
RETURN SUBSTRING("ThemisDB", 6)     -- "DB"
```

### UPPER(str) / LOWER(str)
Konvertiert zu Groß-/Kleinschreibung.

```aql
RETURN UPPER("hello")  -- "HELLO"
RETURN LOWER("WORLD")  -- "world"
```

### TRIM(str) / LTRIM(str) / RTRIM(str)
Entfernt Leerzeichen.

```aql
RETURN TRIM("  hello  ")   -- "hello"
RETURN LTRIM("  hello  ")  -- "hello  "
RETURN RTRIM("  hello  ")  -- "  hello"
```

### SPLIT(str, separator)
Teilt String in Array.

```aql
RETURN SPLIT("a,b,c", ",")  -- ["a", "b", "c"]

-- E-Mail-Domain extrahieren
FOR user IN users
  LET parts = SPLIT(user.email, "@")
  RETURN { user: parts[0], domain: parts[1] }
```

### CONTAINS(str, search)
Prüft ob Substring enthalten ist.

```aql
RETURN CONTAINS("Hello World", "World")  -- true
RETURN CONTAINS("Hello World", "Themis")  -- false

-- Filter mit CONTAINS
FOR product IN products
  FILTER CONTAINS(product.description, "premium")
  RETURN product
```

### REPLACE(str, search, replacement)
Ersetzt alle Vorkommen.

```aql
RETURN REPLACE("Hello World", "World", "ThemisDB")  -- "Hello ThemisDB"
```

### REGEX_TEST(str, pattern)
Prüft ob Regex-Muster matcht.

```aql
-- E-Mail-Validierung
FOR user IN users
  FILTER REGEX_TEST(user.email, "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}\$")
  RETURN user

-- Telefonnummer-Format
RETURN REGEX_TEST("+49 123 456789", "^\\+[0-9]{2}")  -- true
```

### LEVENSHTEIN_DISTANCE(str1, str2)
Berechnet Edit-Distanz (für Fuzzy-Matching).

```aql
RETURN LEVENSHTEIN_DISTANCE("kitten", "sitting")  -- 3

-- Ähnliche Produktnamen finden
FOR product IN products
  LET distance = LEVENSHTEIN_DISTANCE(product.name, @searchTerm)
  FILTER distance <= 3
  SORT distance ASC
  RETURN { product, distance }
```

---

## Math-Funktionen

### Grundrechenarten

```aql
RETURN ABS(-42)      -- 42
RETURN CEIL(4.2)     -- 5
RETURN FLOOR(4.8)    -- 4
RETURN ROUND(4.5)    -- 5
RETURN SQRT(16)      -- 4
RETURN POW(2, 10)    -- 1024
```

### Logarithmen und Exponenten

```aql
RETURN LOG(2.718281828)  -- ~1
RETURN LOG10(1000)       -- 3
RETURN EXP(1)            -- ~2.718
```

### Trigonometrie

```aql
RETURN SIN(PI() / 2)   -- 1
RETURN COS(0)          -- 1
RETURN TAN(PI() / 4)   -- ~1
RETURN ATAN2(1, 1)     -- ~0.785 (45°)

-- Grad zu Radian und zurück
RETURN RADIANS(180)    -- ~3.14159
RETURN DEGREES(PI())   -- 180
```

### Zufallszahlen

```aql
RETURN RANDOM()           -- 0.0 bis 1.0
RETURN RAND_INT(1, 100)   -- Zufällige Ganzzahl 1-100

-- Zufällige Stichprobe
FOR doc IN large_collection
  FILTER RANDOM() < 0.01  -- 1% Stichprobe
  RETURN doc
```

### Aggregation

```aql
RETURN MIN(5, 3, 8, 1)   -- 1
RETURN MAX(5, 3, 8, 1)   -- 8
RETURN SUM([1, 2, 3, 4]) -- 10
RETURN AVG([1, 2, 3, 4]) -- 2.5
```

---

## Array-Funktionen

### Zugriff

```aql
LET arr = [1, 2, 3, 4, 5]
RETURN FIRST(arr)    -- 1
RETURN LAST(arr)     -- 5
RETURN NTH(arr, 2)   -- 3 (0-basiert)
```

### Manipulation

```aql
LET arr = [1, 2, 3]
RETURN PUSH(arr, 4)       -- [1, 2, 3, 4]
RETURN POP(arr)           -- [1, 2]
RETURN SHIFT(arr)         -- [2, 3]
RETURN UNSHIFT(arr, 0)    -- [0, 1, 2, 3]
```

### Transformation

```aql
RETURN SLICE([1, 2, 3, 4, 5], 1, 3)  -- [2, 3, 4]
RETURN FLATTEN([[1, 2], [3, 4]])     -- [1, 2, 3, 4]
RETURN UNIQUE([1, 2, 2, 3, 3, 3])    -- [1, 2, 3]
RETURN SORTED([3, 1, 4, 1, 5])       -- [1, 1, 3, 4, 5]
RETURN REVERSE_ARRAY([1, 2, 3])      -- [3, 2, 1]
```

### Mengenoperationen

```aql
LET a = [1, 2, 3]
LET b = [2, 3, 4]
RETURN UNION(a, b)         -- [1, 2, 3, 4]
RETURN INTERSECTION(a, b)  -- [2, 3]
RETURN MINUS(a, b)         -- [1]
```

### Utility

```aql
RETURN POSITION([10, 20, 30], 20)  -- 1
RETURN COUNT([1, 2, 3, 4, 5])      -- 5
RETURN RANGE(1, 5)                  -- [1, 2, 3, 4, 5]
RETURN RANGE(0, 10, 2)              -- [0, 2, 4, 6, 8, 10]
```

---

## Date-Funktionen

### Aktuelles Datum/Zeit

```aql
RETURN DATE_NOW()        -- Unix Timestamp in ms
RETURN DATE_ISO8601()    -- "2024-01-15T10:30:00.000Z"
```

### Komponenten extrahieren

```aql
LET ts = DATE_TIMESTAMP("2024-06-15T14:30:00Z")
RETURN DATE_YEAR(ts)       -- 2024
RETURN DATE_MONTH(ts)      -- 6
RETURN DATE_DAY(ts)        -- 15
RETURN DATE_HOUR(ts)       -- 14
RETURN DATE_MINUTE(ts)     -- 30
RETURN DATE_DAYOFWEEK(ts)  -- 6 (Samstag, 0=Sonntag)
RETURN DATE_DAYOFYEAR(ts)  -- 167
```

### Datum-Arithmetik

```aql
LET now = DATE_NOW()

-- 7 Tage in die Zukunft
RETURN DATE_ADD(now, 7, "days")

-- 1 Monat zurück
RETURN DATE_SUBTRACT(now, 1, "months")

-- Differenz berechnen
LET start = DATE_TIMESTAMP("2024-01-01")
LET end = DATE_TIMESTAMP("2024-12-31")
RETURN DATE_DIFF(start, end, "days")  -- 365
```

### Formatierung

```aql
LET ts = DATE_TIMESTAMP("2024-06-15T14:30:00Z")
RETURN DATE_FORMAT(ts, "%Y-%m-%d")        -- "2024-06-15"
RETURN DATE_FORMAT(ts, "%d.%m.%Y %H:%M")  -- "15.06.2024 14:30"

-- Auf Monatsbeginn runden
RETURN DATE_TRUNC(ts, "month")  -- "2024-06-01T00:00:00.000Z"
```

**Praxisbeispiel: Aktivität der letzten 30 Tage**

```aql
FOR event IN events
  LET eventDate = DATE_TIMESTAMP(event.created_at)
  LET daysAgo = DATE_DIFF(eventDate, DATE_NOW(), "days")
  FILTER daysAgo <= 30
  COLLECT week = DATE_TRUNC(eventDate, "week")
  AGGREGATE count = COUNT(1)
  SORT week ASC
  RETURN { week: DATE_FORMAT(week, "%Y-W%V"), count }
```

---

## Document-Funktionen

### Dokument laden

```aql
-- Einzelnes Dokument
LET customer = DOCUMENT("customers", "customer123")
RETURN customer

-- Referenz auflösen
FOR order IN orders
  LET customer = DOCUMENT("customers", order.customer_id)
  RETURN { order, customerName: customer.name }
```

### Objekt-Manipulation

```aql
-- Objekte zusammenführen
LET defaults = { status: "active", role: "user" }
LET user = { name: "Max", role: "admin" }
RETURN MERGE(defaults, user)  -- { status: "active", role: "admin", name: "Max" }

-- Rekursives Merge
LET a = { settings: { theme: "dark", lang: "de" } }
LET b = { settings: { lang: "en" } }
RETURN MERGE_RECURSIVE(a, b)  -- { settings: { theme: "dark", lang: "en" } }

-- Eigenschaften entfernen
RETURN UNSET({ a: 1, b: 2, c: 3 }, ["b", "c"])  -- { a: 1 }

-- Nur bestimmte Eigenschaften behalten
RETURN KEEP({ a: 1, b: 2, c: 3 }, ["a", "b"])  -- { a: 1, b: 2 }
```

### Eigenschaften prüfen

```aql
LET doc = { name: "Max", age: 30 }
RETURN HAS(doc, "name")   -- true
RETURN HAS(doc, "email")  -- false

RETURN ATTRIBUTES(doc)    -- ["name", "age"]
RETURN VALUES(doc)        -- ["Max", 30]
```

### Typ-Prüfungen

```aql
RETURN TYPENAME(null)      -- "null"
RETURN TYPENAME(42)        -- "number"
RETURN TYPENAME("hello")   -- "string"
RETURN TYPENAME([1, 2])    -- "array"
RETURN TYPENAME({a: 1})    -- "object"

RETURN IS_NULL(null)       -- true
RETURN IS_NUMBER(42)       -- true
RETURN IS_STRING("hello")  -- true
RETURN IS_ARRAY([1, 2])    -- true
RETURN IS_OBJECT({a: 1})   -- true
```

### Typ-Konvertierung

```aql
RETURN TO_NUMBER("42.5")   -- 42.5
RETURN TO_STRING(42)       -- "42"
RETURN TO_BOOL(1)          -- true
RETURN TO_BOOL("")         -- false
RETURN TO_ARRAY("hello")   -- ["hello"]
```

---

## Geo-Funktionen

### Geometrie erstellen

```aql
-- Punkt erstellen
LET point = ST_POINT(8.6821, 50.1109)  -- Frankfurt Hauptbahnhof

-- LineString erstellen
LET route = ST_LINESTRING([
  [8.6821, 50.1109],   -- Frankfurt
  [11.5820, 48.1351],  -- München
  [13.4050, 52.5200]   -- Berlin
])

-- Polygon erstellen
LET area = ST_POLYGON([[
  [8.0, 50.0],
  [9.0, 50.0],
  [9.0, 51.0],
  [8.0, 51.0],
  [8.0, 50.0]
]])
```

### Distanz berechnen

```aql
-- Distanz in Metern (Haversine für WGS84)
LET frankfurt = ST_POINT(8.6821, 50.1109)
LET berlin = ST_POINT(13.4050, 52.5200)
RETURN GEO_DISTANCE(frankfurt, berlin)  -- ~423 km

-- Alle Filialen im Umkreis von 10 km
FOR store IN stores
  LET dist = GEO_DISTANCE(store.location, @myLocation)
  FILTER dist <= 10000
  SORT dist ASC
  RETURN { store, distance_km: dist / 1000 }
```

### Räumliche Prädikate

```aql
-- Punkt in Polygon?
LET point = ST_POINT(8.5, 50.5)
LET region = ST_POLYGON([[[8, 50], [9, 50], [9, 51], [8, 51], [8, 50]]])
RETURN ST_CONTAINS(region, point)  -- true

-- Geometrien schneiden sich?
RETURN ST_INTERSECTS(polygon1, polygon2)

-- Punkt innerhalb Distanz?
RETURN ST_DWITHIN(point1, point2, 1000)  -- innerhalb 1 km
```

### Format-Konvertierung

```aql
-- GeoJSON zu WKT
LET point = ST_POINT(8.6821, 50.1109)
RETURN ST_ASTEXT(point)  -- "POINT(8.6821 50.1109)"

-- WKT zu GeoJSON
LET geom = ST_GEOMFROMTEXT("POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))")
RETURN ST_ASGEOJSON(geom)

-- GeoJSON String parsen
LET geom = ST_GEOMFROMGEOJSON('{"type":"Point","coordinates":[8.6821,50.1109]}')
```

### 3D-Geometrie (Z-Koordinate)

```aql
-- 3D-Punkt
LET point3d = ST_POINT(8.6821, 50.1109, 150)  -- mit Höhe 150m

RETURN ST_HASZ(point3d)  -- true
RETURN ST_Z(point3d)     -- 150

-- 3D-Distanz
RETURN ST_3DDISTANCE(point1, point2)

-- Z-Werte in Bereich?
RETURN ST_ZBETWEEN(geometry, 100, 200)

-- Auf 2D reduzieren
RETURN ST_FORCE2D(point3d)
```

### Buffer und Aggregation

```aql
-- Puffer um Punkt (Quadrat)
LET point = ST_POINT(8.6821, 50.1109)
LET buffer = ST_BUFFER(point, 0.01)  -- ~1 km Puffer

-- Geometrien vereinigen
RETURN ST_UNION(polygon1, polygon2)

-- Zentroid berechnen
RETURN ST_CENTROID(polygon)

-- Bounding Box
RETURN ST_ENVELOPE(polygon)
```

---

## CRS-Funktionen

### 🌍 Koordinatensystem-Transformation

ThemisDB unterstützt die Transformation zwischen verschiedenen Koordinatenreferenzsystemen (CRS), was für GIS-Anwendungen und Vermessungsdaten essentiell ist.

### Unterstützte Koordinatensysteme

| EPSG | Name | Verwendung |
|------|------|------------|
| 4326 | WGS84 | GPS, Google Maps, weltweit |
| 4258 | ETRS89 | Europäisches Referenzsystem |
| 25831-25833 | ETRS89/UTM 31-33N | Deutschland, metrische Koordinaten |
| 32631-32633 | WGS84/UTM 31-33N | Globale UTM-Zonen |
| 31466-31469 | DHDN/Gauß-Krüger | Historische deutsche Daten |
| 3857 | Web Mercator | OpenStreetMap, Google Maps Tiles |

### ST_TRANSFORM(geometry, from_srid, to_srid)

```aql
-- UTM-Koordinaten (Vermessungsdaten) zu GPS konvertieren
LET utmPoint = ST_POINT(500000, 5600000)  -- UTM Zone 32N
LET wgs84Point = ST_TRANSFORM(utmPoint, 25832, 4326)
RETURN {
  lat: ST_Y(wgs84Point),  -- ~50.5°
  lon: ST_X(wgs84Point)   -- ~8.9°
}

-- Gauß-Krüger (alte Katasterdaten) zu WGS84
FOR parcel IN historic_parcels
  LET modernGeom = ST_TRANSFORM(parcel.geometry, 31467, 4326)
  UPDATE parcel WITH { geometry_wgs84: modernGeom } IN historic_parcels
```

### UTM-Zone bestimmen

```aql
-- Welche UTM-Zone für einen Längengrad?
RETURN UTM_ZONE(8.6821)     -- 32

-- EPSG-Code für UTM-Zone
RETURN UTM_EPSG(32, true)   -- 25832 (ETRS89/UTM 32N)
RETURN UTM_EPSG(32, false)  -- 32632 (WGS84/UTM 32N)
```

### CRS-Metadaten

```aql
RETURN CRS_NAME(4326)           -- "WGS 84"
RETURN CRS_NAME(25832)          -- "ETRS89 / UTM zone 32N"
RETURN CRS_IS_GEOGRAPHIC(4326)  -- true (Grad-Koordinaten)
RETURN CRS_IS_PROJECTED(25832)  -- true (Meter-Koordinaten)
```

**Praxisbeispiel: Grundstücksdaten harmonisieren**

```aql
-- Verschiedene Quellen mit unterschiedlichen Koordinatensystemen vereinheitlichen
FOR parcel IN all_parcels
  LET normalizedGeom = (
    parcel.srid == 4326 ? parcel.geometry :
    parcel.srid == 25832 ? ST_TRANSFORM(parcel.geometry, 25832, 4326) :
    parcel.srid == 31467 ? ST_TRANSFORM(parcel.geometry, 31467, 4326) :
    null
  )
  LET center = ST_CENTROID(normalizedGeom)
  RETURN {
    id: parcel.id,
    original_srid: parcel.srid,
    area_sqm: ST_AREA(ST_TRANSFORM(normalizedGeom, 4326, UTM_EPSG(UTM_ZONE(ST_X(center)), true))),
    center: { lat: ST_Y(center), lon: ST_X(center) }
  }
```

---

## Vector-Funktionen

### 🧠 Ähnlichkeitssuche (für ML/AI)

ThemisDB integriert Vektor-Operationen nativ für Embeddings aus ML-Modellen.

### Ähnlichkeitsmetriken

```aql
LET vec1 = [0.1, 0.2, 0.3, 0.4]
LET vec2 = [0.15, 0.25, 0.28, 0.45]

-- Kosinus-Ähnlichkeit (0-1, höher = ähnlicher)
RETURN COSINE_SIMILARITY(vec1, vec2)  -- ~0.996

-- Euklidische Distanz (niedriger = ähnlicher)
RETURN EUCLIDEAN_DISTANCE(vec1, vec2)  -- ~0.095

-- Dot Product
RETURN DOT_PRODUCT(vec1, vec2)  -- 0.309

-- Manhattan-Distanz
RETURN MANHATTAN_DISTANCE(vec1, vec2)  -- 0.17

-- Chebyshev-Distanz (Maximum)
RETURN CHEBYSHEV_DISTANCE(vec1, vec2)  -- 0.05
```

### SIMILARITY(vector, target, k?)

Findet die k ähnlichsten Dokumente.

```aql
-- Top 10 ähnliche Produkte finden
FOR product IN products
  LET sim = SIMILARITY(product.embedding, @queryEmbedding, 10)
  FILTER sim > 0.8
  SORT sim DESC
  RETURN { product, similarity: sim }
```

### Vektor-Normalisierung

```aql
-- L2-Normalisierung (Länge = 1)
RETURN L2_NORMALIZE([3, 4])  -- [0.6, 0.8]

-- Min-Max-Normalisierung (0-1 Bereich)
RETURN MIN_MAX_NORMALIZE([10, 20, 30])  -- [0, 0.5, 1]
```

### Vektor-Arithmetik

```aql
LET v1 = [1, 2, 3]
LET v2 = [4, 5, 6]

RETURN VECTOR_ADD(v1, v2)    -- [5, 7, 9]
RETURN VECTOR_SUB(v1, v2)    -- [-3, -3, -3]
RETURN VECTOR_MUL(v1, v2)    -- [4, 10, 18] (elementweise)
RETURN VECTOR_SCALE(v1, 2)   -- [2, 4, 6]
```

### Vektor-Aggregation

```aql
LET v = [1, 2, 3, 4, 5]

RETURN VECTOR_SUM(v)   -- 15
RETURN VECTOR_AVG(v)   -- 3
RETURN VECTOR_NORM(v)  -- ~7.416 (L2-Norm)
RETURN VECTOR_DIM(v)   -- 5
RETURN VECTOR_MIN(v)   -- 1
RETURN VECTOR_MAX(v)   -- 5
```

### Vektor-Utility

```aql
RETURN VECTOR_ZEROS(5)           -- [0, 0, 0, 0, 0]
RETURN VECTOR_ONES(3)            -- [1, 1, 1]
RETURN VECTOR_RANDOM(4)          -- [0.23, 0.87, 0.12, 0.56]
RETURN VECTOR_SLICE([1,2,3,4], 1, 3)  -- [2, 3]
RETURN VECTOR_CONCAT([1,2], [3,4])    -- [1, 2, 3, 4]
```

**Praxisbeispiel: Semantische Suche mit Kontext**

```aql
-- Finde ähnliche Artikel mit Geo- und Zeit-Kontext
FOR article IN articles
  LET semanticSim = COSINE_SIMILARITY(article.embedding, @queryEmbedding)
  LET geoDist = GEO_DISTANCE(article.location, @userLocation)
  LET recency = 1 / (1 + DATE_DIFF(article.published, DATE_NOW(), "days"))
  
  -- Kombinierter Score
  LET score = (semanticSim * 0.6) + ((1 - geoDist/100000) * 0.2) + (recency * 0.2)
  
  FILTER semanticSim > 0.7
  SORT score DESC
  LIMIT 20
  RETURN { article, score, semanticSim, geoDist, recency }
```

---

## Graph-Funktionen

### 🔗 Graph-Traversierung

ThemisDB bietet native Graph-Unterstützung ohne separate Query-Sprache.

### Nachbarn finden

```aql
-- Direkte Freunde
FOR friend IN 1..1 OUTBOUND @startUser knows
  RETURN friend

-- Freunde von Freunden (2 Hops)
FOR connection IN 1..2 OUTBOUND @startUser knows
  RETURN DISTINCT connection

-- Mit Tiefenbegrenzung
FOR v, e, p IN 1..5 OUTBOUND @startNode follows
  RETURN { vertex: v, edge: e, path: p }
```

### SHORTEST_PATH(from, to, edgeCollection)

```aql
-- Kürzester Weg zwischen zwei Personen
LET path = SHORTEST_PATH(@personA, @personB, "knows")
RETURN {
  length: LENGTH(path.vertices),
  vertices: path.vertices,
  edges: path.edges
}
```

### GRAPH_DISTANCE(from, to, edgeCollection)

```aql
-- Wie viele Hops entfernt?
RETURN GRAPH_DISTANCE(@user1, @user2, "follows")  -- z.B. 3
```

### GRAPH_CONNECTED(from, to, edgeCollection)

```aql
-- Sind zwei Knoten überhaupt verbunden?
IF GRAPH_CONNECTED(@nodeA, @nodeB, "links")
  RETURN "Verbunden"
ELSE
  RETURN "Nicht verbunden"
```

### Zentralitätsmaße

```aql
-- Degree Centrality (wie viele Verbindungen?)
FOR user IN users
  LET centrality = DEGREE_CENTRALITY(user, "follows")
  SORT centrality DESC
  LIMIT 10
  RETURN { user: user.name, centrality }

-- PageRank (Wichtigkeit im Netzwerk)
FOR page IN pages
  LET rank = PAGERANK(page, "links", 0.85)
  SORT rank DESC
  LIMIT 100
  RETURN { url: page.url, pagerank: rank }
```

### Clustering-Koeffizient

```aql
-- Wie stark sind die Freunde eines Users untereinander verbunden?
FOR user IN users
  LET clustering = CLUSTERING_COEFFICIENT(user, "knows")
  FILTER clustering > 0.5  -- Stark vernetzte Communities
  RETURN { user: user.name, clustering }
```

### Verbundene Komponenten

```aql
-- Finde alle zusammenhängenden Teilgraphen
LET components = CONNECTED_COMPONENTS("users", "knows")
FOR comp IN components
  RETURN {
    size: LENGTH(comp.members),
    members: comp.members
  }
```

**Praxisbeispiel: Influencer-Analyse**

```aql
-- Finde Top-Influencer mit Geo-Reichweite
FOR user IN users
  LET followers = (FOR f IN 1..1 INBOUND user follows RETURN f)
  LET followerCount = LENGTH(followers)
  LET avgDistance = AVG(
    FOR f IN followers
      RETURN GEO_DISTANCE(f.location, user.location)
  )
  LET pagerank = PAGERANK(user, "follows")
  
  FILTER followerCount >= 1000
  SORT pagerank DESC
  LIMIT 50
  
  RETURN {
    user: user.name,
    followers: followerCount,
    avgReachKm: avgDistance / 1000,
    pagerank,
    influenceScore: pagerank * LOG(followerCount) * LOG(avgDistance/1000 + 1)
  }
```

---

## Relational-Funktionen

### 📊 SQL-kompatible Aggregation und Joins

ThemisDB bietet SQL-ähnliche Funktionen für relationale Operationen.

### Aggregation

```aql
FOR order IN orders
  COLLECT customer = order.customer_id
  AGGREGATE
    total = SUM(order.amount),
    count = COUNT(1),
    avg = AVG(order.amount),
    distinct_products = COUNT_DISTINCT(order.product_id)
  RETURN { customer, total, count, avg, distinct_products }
```

### Statistische Funktionen

```aql
FOR sale IN sales
  COLLECT region = sale.region
  AGGREGATE
    median = MEDIAN(sale.amount),
    stddev = STDDEV(sale.amount),
    variance = VARIANCE(sale.amount),
    p95 = PERCENTILE(sale.amount, 95)
  RETURN { region, median, stddev, variance, p95 }
```

### GROUP_CONCAT / COLLECT

```aql
FOR order IN orders
  COLLECT customer = order.customer_id
  AGGREGATE products = GROUP_CONCAT(order.product_name, ", ")
  RETURN { customer, products }
  -- Ergebnis: { customer: "C1", products: "Laptop, Mouse, Keyboard" }
```

### Conditional

```aql
-- COALESCE: Erster nicht-null Wert
RETURN COALESCE(null, null, "default", "other")  -- "default"

-- NULLIF: Null wenn gleich
RETURN NULLIF(10, 10)   -- null
RETURN NULLIF(10, 20)   -- 10

-- GREATEST / LEAST
RETURN GREATEST(5, 3, 8, 1)  -- 8
RETURN LEAST(5, 3, 8, 1)     -- 1

-- IF
RETURN IF(age >= 18, "adult", "minor")
```

### Joins (über FOR-Loops)

```aql
-- INNER JOIN Equivalent
FOR order IN orders
  FOR customer IN customers
    FILTER order.customer_id == customer._key
    RETURN { order, customer }

-- LEFT JOIN mit LOOKUP
FOR order IN orders
  LET customer = LOOKUP("customers", order.customer_id)
  RETURN { order, customer }  -- customer kann null sein
```

### Window Functions

```aql
-- ROW_NUMBER, RANK, DENSE_RANK
FOR sale IN sales
  SORT sale.region, sale.amount DESC
  LET rowNum = ROW_NUMBER() OVER (PARTITION BY sale.region ORDER BY sale.amount DESC)
  FILTER rowNum <= 3  -- Top 3 pro Region
  RETURN { sale, rank: rowNum }

-- LAG / LEAD (vorheriger/nächster Wert)
FOR sale IN sales
  SORT sale.date
  LET prevAmount = LAG(sale.amount, 1) OVER (ORDER BY sale.date)
  LET nextAmount = LEAD(sale.amount, 1) OVER (ORDER BY sale.date)
  LET growth = (sale.amount - prevAmount) / prevAmount * 100
  RETURN { date: sale.date, amount: sale.amount, growth }

-- RUNNING_SUM (kumulativ)
FOR sale IN sales
  SORT sale.date
  LET runningTotal = RUNNING_SUM(sale.amount) OVER (ORDER BY sale.date)
  RETURN { date: sale.date, amount: sale.amount, runningTotal }
```

**Praxisbeispiel: Umsatz-Dashboard**

```aql
FOR sale IN sales
  LET saleDate = DATE_TIMESTAMP(sale.created_at)
  FILTER saleDate >= DATE_SUBTRACT(DATE_NOW(), 365, "days")
  
  COLLECT 
    month = DATE_TRUNC(saleDate, "month"),
    region = sale.region
  AGGREGATE
    revenue = SUM(sale.amount),
    orders = COUNT(1),
    avgOrder = AVG(sale.amount),
    topProducts = GROUP_CONCAT(sale.product, ", ")
  
  LET prevMonth = LAG(revenue, 1) OVER (PARTITION BY region ORDER BY month)
  LET growth = prevMonth ? ((revenue - prevMonth) / prevMonth * 100) : null
  
  SORT region, month
  RETURN {
    month: DATE_FORMAT(month, "%Y-%m"),
    region,
    revenue,
    orders,
    avgOrder,
    growth: growth ? CONCAT(ROUND(growth, 1), "%") : "N/A",
    topProducts: SUBSTRING(topProducts, 0, 100)
  }
```

---

## File-Funktionen

### 📁 Pfad-Manipulation

```aql
-- Pfade zusammenfügen
RETURN PATH_JOIN("/home", "user", "documents")  -- "/home/user/documents"

-- Verzeichnis extrahieren
RETURN PATH_DIRNAME("/home/user/file.txt")  -- "/home/user"

-- Dateiname extrahieren
RETURN PATH_BASENAME("/home/user/file.txt")  -- "file.txt"

-- Erweiterung extrahieren
RETURN PATH_EXTENSION("/home/user/file.txt")  -- "txt"

-- Pfad normalisieren
RETURN PATH_NORMALIZE("/home/./user/../admin/./file.txt")  -- "/home/admin/file.txt"
```

### Dateinamen-Operationen

```aql
-- Dateiname ohne Erweiterung
RETURN FILENAME_WITHOUT_EXT("document.pdf")  -- "document"

-- Erweiterung extrahieren
RETURN FILE_EXT("photo.jpg")  -- "jpg"

-- Dateinamen bereinigen (für Upload-Sicherheit)
RETURN SANITIZE_FILENAME("my file (1).txt")  -- "my_file_1_.txt"
RETURN SANITIZE_FILENAME("../../../etc/passwd")  -- "etc_passwd"
```

### MIME-Typen

```aql
-- MIME-Typ ermitteln
RETURN MIME_TYPE("document.pdf")   -- "application/pdf"
RETURN MIME_TYPE("photo.jpg")      -- "image/jpeg"
RETURN MIME_TYPE("video.mp4")      -- "video/mp4"
RETURN MIME_TYPE("data.json")      -- "application/json"

-- Typ-Prüfungen
RETURN IS_IMAGE("photo.jpg")       -- true
RETURN IS_VIDEO("movie.mp4")       -- true
RETURN IS_AUDIO("song.mp3")        -- true
RETURN IS_DOCUMENT("report.pdf")   -- true
```

### Dateigrößen

```aql
-- Größe formatieren
RETURN FORMAT_FILESIZE(1024)           -- "1 KB"
RETURN FORMAT_FILESIZE(1048576)        -- "1 MB"
RETURN FORMAT_FILESIZE(1073741824)     -- "1 GB"
RETURN FORMAT_FILESIZE(1536000)        -- "1.46 MB"

-- Größe parsen
RETURN PARSE_FILESIZE("1.5 GB")   -- 1610612736
RETURN PARSE_FILESIZE("500 KB")   -- 512000
```

**Praxisbeispiel: Datei-Inventar**

```aql
FOR file IN files
  LET ext = FILE_EXT(file.name)
  LET mimeType = MIME_TYPE(file.name)
  LET sizeFormatted = FORMAT_FILESIZE(file.size)
  LET category = (
    IS_IMAGE(file.name) ? "Bilder" :
    IS_VIDEO(file.name) ? "Videos" :
    IS_AUDIO(file.name) ? "Audio" :
    IS_DOCUMENT(file.name) ? "Dokumente" :
    "Sonstige"
  )
  
  COLLECT cat = category
  AGGREGATE
    count = COUNT(1),
    totalSize = SUM(file.size),
    extensions = GROUP_CONCAT(DISTINCT ext, ", ")
  
  RETURN {
    category: cat,
    count,
    totalSize: FORMAT_FILESIZE(totalSize),
    extensions
  }
```

---

## FAQ

### Allgemeine Fragen

#### ❓ Warum sollte ich ThemisDB statt PostgreSQL verwenden?

**Wenn mindestens einer dieser Punkte zutrifft:**
- Sie brauchen Graph-Traversierung UND Geo-Queries
- Sie haben ML-Embeddings und brauchen Ähnlichkeitssuche
- Sie möchten Prozesse aus Event-Logs entdecken
- Sie haben Daten in verschiedenen Koordinatensystemen
- Sie möchten keine 5 verschiedenen Systeme integrieren

**Bei PostgreSQL würden Sie brauchen:**
- PostGIS für Geo
- pgvector für Vectors
- `WITH RECURSIVE` für Graphen (komplex!)
- Mehrere Extensions koordinieren

#### ❓ Ist ThemisDB schneller als MongoDB?

**Für Multi-Model Queries: Ja.**
Eine Query die Graph + Geo + Vector kombiniert ist in ThemisDB nativ optimiert. In MongoDB müssten Sie:
1. `\$graphLookup` für Graph
2. Atlas Search für Vectors
3. `\$geoNear` für Geo
4. Alles in einer komplexen Aggregation-Pipeline kombinieren

**Für einfache CRUD: Vergleichbar.**

#### ❓ Kann ich ThemisDB mit meinen bestehenden ML-Modellen nutzen?

**Ja!** Speichern Sie Embeddings als Arrays:

```aql
-- Embedding speichern
INSERT { 
  text: "Hello World",
  embedding: [0.1, 0.2, 0.3, ...]  -- 1536 Dimensionen für OpenAI
} INTO documents

-- Ähnlichkeitssuche
FOR doc IN documents
  LET sim = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
  FILTER sim > 0.8
  SORT sim DESC
  RETURN doc
```

Unterstützte Embedding-Dimensionen: Beliebig (OpenAI, Cohere, lokale Modelle).

### Geo-Fragen

#### ❓ Meine Daten sind in UTM-Koordinaten. Wie konvertiere ich zu GPS?

```aql
-- ETRS89/UTM Zone 32N (Deutschland) zu WGS84
LET utmPoint = ST_POINT(500000, 5600000)
LET gpsPoint = ST_TRANSFORM(utmPoint, 25832, 4326)
RETURN {
  lat: ST_Y(gpsPoint),
  lon: ST_X(gpsPoint)
}
```

#### ❓ Welches Koordinatensystem haben meine Daten?

Typische Hinweise:
- Werte wie `8.6821, 50.1109` → WGS84 (EPSG:4326)
- Werte wie `500000, 5600000` → UTM (z.B. EPSG:25832)
- Werte wie `3500000, 5600000` → Gauß-Krüger (z.B. EPSG:31467)

### Graph-Fragen

#### ❓ Wie modelliere ich Beziehungen?

```aql
-- Edge-Collection erstellen
INSERT { _from: "users/alice", _to: "users/bob", since: "2020-01-01" } INTO knows

-- Traversieren
FOR friend IN 1..1 OUTBOUND "users/alice" knows
  RETURN friend
```

#### ❓ Kann ich gewichtete kürzeste Pfade berechnen?

```aql
LET path = SHORTEST_PATH(@from, @to, "roads", { weightAttribute: "distance" })
RETURN {
  totalDistance: SUM(path.edges[*].distance),
  route: path.vertices[*].name
}
```

### Performance-Fragen

#### ❓ Wie optimiere ich Vector-Suchen?

1. **Index erstellen:**
```aql
CREATE INDEX idx_embedding ON documents(embedding) TYPE vector
```

2. **LIMIT früh verwenden:**
```aql
FOR doc IN documents
  LET sim = COSINE_SIMILARITY(doc.embedding, @query)
  SORT sim DESC
  LIMIT 100  -- So früh wie möglich
  RETURN doc
```

3. **Vorfiltern:**
```aql
FOR doc IN documents
  FILTER doc.category == "tech"  -- Erst filtern, dann Similarity
  LET sim = COSINE_SIMILARITY(doc.embedding, @query)
  ...
```

#### ❓ Warum ist meine Geo-Query langsam?

1. **Geo-Index erstellen:**
```aql
CREATE INDEX idx_location ON stores(location) TYPE geo
```

2. **ST_DWithin statt Post-Filter:**
```aql
-- Langsam (alle laden, dann filtern)
FOR store IN stores
  FILTER GEO_DISTANCE(store.location, @point) < 10000
  
-- Schnell (Index nutzen)
FOR store IN stores
  FILTER ST_DWITHIN(store.location, @point, 10000)
```

### Migration-Fragen

#### ❓ Wie migriere ich von MongoDB?

1. **Dokumente exportieren:** `mongoexport`
2. **In ThemisDB importieren:** 
```bash
themisdb import --collection users --file users.json
```
3. **Queries anpassen:**

| MongoDB | ThemisDB AQL |
|---------|--------------|
| `db.users.find({age: {\$gt: 18}})` | `FOR u IN users FILTER u.age > 18 RETURN u` |
| `\$lookup` | `FOR ... FOR ... FILTER` |
| `\$graphLookup` | `FOR v IN OUTBOUND` |

#### ❓ Wie migriere ich von Neo4j?

1. **Knoten exportieren:** Als JSON
2. **Kanten exportieren:** Als Edge-Collection
3. **Cypher zu AQL:**

| Cypher | ThemisDB AQL |
|--------|--------------|
| `MATCH (p:Person)` | `FOR p IN persons` |
| `(a)-[:KNOWS]->(b)` | `FOR b IN OUTBOUND a knows` |
| `shortestPath()` | `SHORTEST_PATH()` |

---

## Zusammenfassung

ThemisDB AQL bietet:

✅ **~210 Funktionen** in 11 Kategorien
✅ **Einheitliche Syntax** für alle Datenmodelle
✅ **Native Multi-Model Queries** (Graph + Vector + Geo + Relational)
✅ **Vollständige CRS-Unterstützung** (ETRS89, UTM, WGS84, Gauß-Krüger)
✅ **SQL-kompatible** Aggregation und Window Functions
✅ **Prozess-Mining** aus Event-Daten
✅ **Keine Vendor Lock-in** bei Query-Sprache

**Nächste Schritte:**
- [Installation Guide](./INSTALLATION.md)
- [Multi-Model Architecture](./MULTI_MODEL_ARCHITECTURE.md)
- [Enterprise Analytics](./ENTERPRISE_ANALYTICS.md)
- [API Reference](./API_REFERENCE.md)
