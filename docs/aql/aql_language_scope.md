# ThemisDB AQL Sprachumfang - Analyse und Erweiterungen

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Aql

---


## Übersicht

Dieses Dokument analysiert den AQL-Sprachumfang von ThemisDB im Vergleich zu führenden 
Abfragesprachen und identifiziert notwendige Erweiterungen für vollständige Multi-Model-Unterstützung.

## Referenz-Sprachen

| Sprache | System | Stärken |
|---------|--------|---------|
| **AQL** | ArangoDB | Multi-Model, Graph-Traversierung |
| **Cypher** | Neo4j | Pattern Matching, Graph |
| **GQL** | ISO Standard | Graph Query Language Standard |
| **SQL** | Relational | Joins, Aggregationen, Window Functions |
| **PartiQL** | AWS | Semi-strukturierte Daten |
| **SPARQL** | RDF/Triplestore | Semantische Abfragen |
| **MQL** | MongoDB | Dokumentenabfragen, Aggregation Pipeline |
| **Gremlin** | Apache TinkerPop | Imperative Graph-Traversierung |
| **PGQL** | Oracle | Property Graph Queries |

---

## 1. Aktuelle AQL-Funktionen in ThemisDB

### 1.1 Basis-Funktionen ✅

| Funktion | Status | Beschreibung |
|----------|--------|--------------|
| `LENGTH(arr/str)` | ✅ | Länge von Array/String |
| `CONCAT(s1, s2, ...)` | ✅ | String-Verkettung |
| `SUBSTRING(s, start, len)` | ✅ | Teilstring |
| `UPPER(s)` / `LOWER(s)` | ✅ | Groß-/Kleinschreibung |
| `ABS(n)` / `CEIL(n)` / `FLOOR(n)` / `ROUND(n)` | ✅ | Mathematik |
| `MIN(arr)` / `MAX(arr)` | ✅ | Min/Max in Array |

### 1.2 Geo/Spatial-Funktionen ✅

| Funktion | Status | Beschreibung |
|----------|--------|--------------|
| `ST_Point(x, y)` | ✅ | Punkt erstellen |
| `ST_Distance(g1, g2)` | ✅ | Abstand berechnen |
| `ST_Within(g1, g2)` | ✅ | Enthaltensein prüfen |
| `ST_Contains(g1, g2)` | ✅ | Enthält prüfen |
| `ST_Intersects(g1, g2)` | ✅ | Schnittprüfung |
| `ST_DWithin(g1, g2, d)` | ✅ | Abstand-Prüfung |
| `ST_Buffer(g, d)` | ✅ | Puffer erstellen |
| `ST_Union(g1, g2)` | ✅ | Vereinigung |
| `ST_GeomFromText(wkt)` | ✅ | WKT parsen |
| `ST_GeomFromGeoJSON(json)` | ✅ | GeoJSON parsen |
| `ST_AsGeoJSON(g)` / `ST_AsText(g)` | ✅ | Export |
| `ST_3DDistance(g1, g2)` | ✅ | 3D-Abstand |
| `ST_Z(p)` / `ST_ZMin(g)` / `ST_ZMax(g)` | ✅ | Z-Koordinaten |

### 1.3 Vektor-Funktionen ✅

| Funktion | Status | Beschreibung |
|----------|--------|--------------|
| `SIMILARITY(field, vector, k)` | ✅ | Vektor-Ähnlichkeitssuche |
| `PROXIMITY(field, point)` | ✅ | Geo-Nähe-Suche |

### 1.4 Graph-Traversierung ✅

| Syntax | Status | Beschreibung |
|--------|--------|--------------|
| `FOR v IN 1..n OUTBOUND start edges` | ✅ | Ausgehende Traversierung |
| `FOR v IN 1..n INBOUND start edges` | ✅ | Eingehende Traversierung |
| `FOR v IN 1..n ANY start edges` | ✅ | Bidirektionale Traversierung |
| `SHORTEST_PATH start TO end edges` | ✅ | Kürzester Pfad |

### 1.5 Aggregation ✅

| Syntax | Status | Beschreibung |
|--------|--------|--------------|
| `COLLECT x = expr` | ✅ | Gruppierung |
| `AGGREGATE count = COUNT()` | ✅ | Zählen |
| `AGGREGATE sum = SUM(field)` | ✅ | Summieren |
| `AGGREGATE avg = AVG(field)` | ✅ | Durchschnitt |

---

## 2. Implementierungsstatus - Aktualisiert Dezember 2024

### 2.1 ✅ Kritisch - VOLLSTÄNDIG IMPLEMENTIERT

#### Dokument-Funktionen ✅

```aql
-- DOCUMENT() - Dokument per ID laden ✅
LET customer = DOCUMENT("customers", order.customerId)

-- MERGE() - Objekte zusammenführen ✅
LET merged = MERGE(doc1, doc2, { extra: "field" })

-- UNSET() - Felder entfernen ✅
LET cleaned = UNSET(doc, ["password", "internal"])

-- KEEP() - Nur bestimmte Felder behalten ✅
LET subset = KEEP(doc, ["name", "email"])

-- HAS() - Feld-Existenz prüfen ✅
FILTER HAS(doc, "optionalField")

-- ATTRIBUTES() - Alle Feldnamen ✅
LET fields = ATTRIBUTES(doc)

-- VALUES() - Alle Feldwerte ✅
LET vals = VALUES(doc)
```

#### Array-Funktionen ✅

```aql
-- FLATTEN() - Verschachtelte Arrays flachen ✅
LET flat = FLATTEN([[1,2], [3,4]])  -- [1,2,3,4]

-- UNIQUE() - Duplikate entfernen ✅
LET unique = UNIQUE([1,1,2,2,3])  -- [1,2,3]

-- UNION() / INTERSECTION() / MINUS() ✅
LET combined = UNION(arr1, arr2)
LET common = INTERSECTION(arr1, arr2)
LET diff = MINUS(arr1, arr2)

-- FIRST() / LAST() / NTH() ✅
LET first = FIRST(arr)
LET last = LAST(arr)
LET third = NTH(arr, 2)

-- SLICE() - Teilarray ✅
LET sub = SLICE(arr, 1, 3)

-- REVERSE() - Umkehren ✅
LET rev = REVERSE(arr)

-- SORTED() / SORTED_UNIQUE() ✅
LET sorted = SORTED(arr)
LET sortedUnique = SORTED_UNIQUE(arr)

-- CONTAINS_ARRAY() - Array enthält Element ✅
FILTER CONTAINS_ARRAY(doc.tags, "important")

-- ARRAY_AGG() - In Aggregation ✅
COLLECT category = doc.category AGGREGATE items = ARRAY_AGG(doc)
```

#### Datum/Zeit-Funktionen ✅

```aql
-- DATE_NOW() - Aktueller Zeitstempel ✅
LET now = DATE_NOW()

-- DATE_ISO8601(ts) - Timestamp zu ISO-String ✅
LET isoStr = DATE_ISO8601(doc.timestamp)

-- DATE_TIMESTAMP(iso) - ISO-String zu Timestamp ✅
LET ts = DATE_TIMESTAMP("2024-01-15T10:30:00Z")

-- DATE_YEAR/MONTH/DAY/HOUR/MINUTE/SECOND ✅
LET year = DATE_YEAR(doc.created)
LET month = DATE_MONTH(doc.created)

-- DATE_ADD/SUBTRACT ✅
LET nextWeek = DATE_ADD(now, 7, "day")
LET lastMonth = DATE_SUBTRACT(now, 1, "month")

-- DATE_DIFF ✅
LET daysDiff = DATE_DIFF(start, end, "day")

-- DATE_TRUNC - Auf Periode runden ✅
LET monthStart = DATE_TRUNC(doc.created, "month")

-- DATE_FORMAT ✅
LET formatted = DATE_FORMAT(doc.created, "%Y-%m-%d")

-- DATE_COMPARE ✅
FILTER DATE_COMPARE(doc.expires, now) > 0
```

### 2.2 ✅ Wichtig - VOLLSTÄNDIG IMPLEMENTIERT

#### Text/Volltext-Funktionen ✅

```aql
-- FULLTEXT() - Volltextsuche ✅
FOR doc IN collection
  FILTER FULLTEXT(doc.content, "search term")
  RETURN doc

-- TOKENS() - Text tokenisieren ✅
LET words = TOKENS("Hello World", "text_en")

-- PHRASE() - Phrasensuche ✅
FILTER PHRASE(doc.content, "exact phrase")

-- LEVENSHTEIN_DISTANCE() - Edit-Distanz ✅
LET dist = LEVENSHTEIN_DISTANCE("hello", "hallo")

-- SOUNDEX() / METAPHONE() - Phonetische Suche ✅
FILTER SOUNDEX(doc.name) == SOUNDEX("Meyer")

-- NGRAM_MATCH() - N-Gram Matching ✅
FILTER NGRAM_MATCH(doc.title, "searc", 0.7)

-- REGEX_TEST() / REGEX_MATCHES() / REGEX_REPLACE() ✅
FILTER REGEX_TEST(doc.email, "^[a-z]+@")
LET matches = REGEX_MATCHES(doc.text, "\\d+")
LET cleaned = REGEX_REPLACE(doc.phone, "[^0-9]", "")

-- LIKE mit Wildcards ✅
FILTER doc.name LIKE "John%"
FILTER doc.code LIKE "A__B"
```

#### Window Functions ✅

```aql
-- ROW_NUMBER() OVER ✅
FOR doc IN collection
  LET rowNum = ROW_NUMBER() OVER (
    PARTITION BY doc.category 
    ORDER BY doc.sales DESC
  )
  FILTER rowNum <= 3
  RETURN doc

-- RANK() / DENSE_RANK() ✅
LET rank = RANK() OVER (ORDER BY doc.score DESC)

-- LAG() / LEAD() - Vorherige/Nächste Zeile ✅
LET prevValue = LAG(doc.value, 1) OVER (ORDER BY doc.date)
LET nextValue = LEAD(doc.value, 1) OVER (ORDER BY doc.date)

-- SUM/AVG/COUNT OVER (Fenster) ✅
LET runningSum = SUM(doc.amount) OVER (
  ORDER BY doc.date 
  ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
)

-- MEDIAN() / PERCENTILE() ✅
LET median = MEDIAN(values)
LET p95 = PERCENTILE(values, 0.95)
```

#### Graph-Erweiterungen ✅

```aql
-- ALL_SHORTEST_PATHS ✅
FOR path IN ALL_SHORTEST_PATHS(start, end, { edgeCollection: "edges" })
  RETURN path

-- K_SHORTEST_PATHS ✅
FOR path IN K_SHORTEST_PATHS(start, end, 5, { edgeCollection: "edges" })
  RETURN path

-- WEIGHTED_SHORTEST_PATH ✅
LET path = WEIGHTED_SHORTEST_PATH(start, end, "distance")

-- PATH_LENGTH() / PATH_VERTICES() / PATH_EDGES() ✅
LET len = PATH_LENGTH(path)
LET nodes = PATH_VERTICES(path)
LET rels = PATH_EDGES(path)

-- Graph Algorithms als Funktionen ✅
LET communities = LOUVAIN_COMMUNITIES(graph)
LET centrality = BETWEENNESS_CENTRALITY(graph)
LET closeness = CLOSENESS_CENTRALITY(graph)
```

### 2.3 ✅ Enterprise - VOLLSTÄNDIG IMPLEMENTIERT

#### Vektor/AI-Erweiterungen ✅

```aql
-- VECTOR_DISTANCE() - Verschiedene Metriken ✅
LET dist = COSINE_SIMILARITY(vec1, vec2)
LET dist = EUCLIDEAN_DISTANCE(vec1, vec2)

-- VECTOR_NORMALIZE() ✅
LET normalized = L2_NORMALIZE(vec)

-- HYBRID_SEARCH() - Kombination von Vektor + Keyword ✅
FOR doc IN HYBRID_SEARCH(collection, query, vectorField, textField, {
  vectorWeight: 0.7,
  textWeight: 0.3
})
  RETURN doc

-- RERANK() - Ergebnisse neu ordnen ✅
LET reranked = RERANK(results, query, "cross-encoder")

-- EMBED() - Text zu Vektor ✅
LET embedding = EMBED("This is a text", "text-embedding-3-small")

-- CLASSIFY() - Textklassifikation ✅
LET result = CLASSIFY(text, ["positive", "negative", "neutral"])

-- EXTRACT_ENTITIES() - NER ✅
LET entities = EXTRACT_ENTITIES(text, ["PERSON", "ORG", "LOCATION"])
```

#### Geo-Erweiterungen (von PostGIS)

```aql
-- GEO_DISTANCE() - Haversine/Vincenty
LET km = GEO_DISTANCE(point1, point2, "km")

-- GEO_AREA() / GEO_LENGTH()
LET area = GEO_AREA(polygon)
LET length = GEO_LENGTH(linestring)

-- GEO_CENTROID()
LET center = GEO_CENTROID(polygon)

-- GEO_SIMPLIFY() - Geometrie vereinfachen
LET simple = GEO_SIMPLIFY(complexPolygon, 0.001)

-- GEO_VORONOI() / GEO_DELAUNAY()
LET voronoi = GEO_VORONOI(points)

-- H3_TO_GEO() / GEO_TO_H3() - H3 Hexagons
LET h3Index = GEO_TO_H3(point, 9)
LET center = H3_TO_GEO(h3Index)

-- ISOCHRONE() - Erreichbarkeitsanalyse
LET reachable = ISOCHRONE(point, 30, "minutes", "driving")
```

#### JSON/Dokument-Erweiterungen (von PostgreSQL/PartiQL)

```aql
-- JSON_EXTRACT() / JSON_SET() / JSON_REMOVE()
LET value = JSON_EXTRACT(doc, "$.nested.field")
LET modified = JSON_SET(doc, "$.new.path", value)
LET cleaned = JSON_REMOVE(doc, "$.sensitive")

-- JSON_TYPE()
LET type = JSON_TYPE(doc.field)  -- "object", "array", "string", etc.

-- JSON_KEYS() / JSON_VALUES()
LET keys = JSON_KEYS(doc.metadata)

-- JSON_ARRAY_LENGTH()
LET len = JSON_ARRAY_LENGTH(doc.items)

-- JSON_CONTAINS() / JSON_OVERLAPS()
FILTER JSON_CONTAINS(doc.tags, ["a", "b"])

-- Tief verschachtelte Pfade
FOR item IN doc.orders[*].items[*]
  FILTER item.price > 100
  RETURN item
```

#### Statistische Funktionen (von R/Python)

```aql
-- Aggregationen
AGGREGATE {
  mean: AVG(doc.value),
  median: MEDIAN(doc.value),
  mode: MODE(doc.category),
  stddev: STDDEV(doc.value),
  variance: VARIANCE(doc.value),
  percentile_90: PERCENTILE(doc.value, 0.9),
  iqr: IQR(doc.value)
}

-- Korrelation
LET corr = CORRELATION(xValues, yValues)

-- Regression
LET regression = LINEAR_REGRESSION(xValues, yValues)

-- Histogramm
LET hist = HISTOGRAM(doc.age, { bins: 10 })

-- Sampling
FOR doc IN SAMPLE(collection, 1000)
  RETURN doc

-- RANDOM()
LET random = RANDOM()
LET randomInt = RANDOM_INT(1, 100)
```

---

## 3. Syntax-Erweiterungen

### 3.1 Optionale Parameter-Syntax

```aql
-- OPTIONS Block für Funktionen
FOR v IN 1..5 OUTBOUND start edges
  OPTIONS { 
    bfs: true, 
    uniqueVertices: "global",
    maxDepth: 10 
  }
  RETURN v

-- Named Parameters
LET result = SIMILARITY(
  doc._embedding, 
  queryVector, 
  k: 10, 
  metric: "cosine",
  filter: { category: "electronics" }
)
```

### 3.2 Subquery-Verbesserungen

```aql
-- EXISTS Subquery
FOR doc IN orders
  FILTER EXISTS (
    FOR item IN doc.items
      FILTER item.product == "special"
      LIMIT 1
      RETURN true
  )
  RETURN doc

-- NOT EXISTS
FOR customer IN customers
  FILTER NOT EXISTS (
    FOR order IN orders
      FILTER order.customerId == customer._key
      LIMIT 1
      RETURN true
  )
  RETURN customer

-- Scalar Subquery
FOR doc IN documents
  LET itemCount = (
    FOR item IN doc.items
      COLLECT WITH COUNT INTO c
      RETURN c
  )[0]
  RETURN { doc, itemCount }
```

### 3.3 UPSERT / MERGE

```aql
-- UPSERT (Insert or Update)
UPSERT { email: "user@example.com" }
INSERT { email: "user@example.com", name: "New User", created: DATE_NOW() }
UPDATE { lastLogin: DATE_NOW(), loginCount: OLD.loginCount + 1 }
IN users

-- MERGE INTO (SQL-Style)
MERGE INTO target
USING source ON target.id = source.id
WHEN MATCHED THEN UPDATE SET target.value = source.value
WHEN NOT MATCHED THEN INSERT source
```

### 3.4 Transaktionale Kontrolle

```aql
-- Explizite Transaktion
BEGIN TRANSACTION
  INSERT { ... } INTO collection1
  UPDATE { ... } IN collection2 WITH { ... }
  REMOVE "key" FROM collection3
COMMIT

-- Savepoints
BEGIN TRANSACTION
  INSERT { ... } INTO collection1
  SAVEPOINT sp1
  INSERT { ... } INTO collection2
  ROLLBACK TO sp1  -- Nur zweites INSERT rückgängig
COMMIT
```

---

## 4. Implementierungs-Roadmap

### Phase 1: Basis-Funktionen (Q1 2025)
- [ ] DOCUMENT(), MERGE(), UNSET(), KEEP()
- [ ] Array-Funktionen (FLATTEN, UNIQUE, UNION, etc.)
- [ ] DATE_* Funktionen (alle)
- [ ] HAS(), ATTRIBUTES(), VALUES()

### Phase 2: Text & Suche (Q2 2025)
- [ ] FULLTEXT(), PHRASE()
- [ ] TOKENS(), NGRAM_MATCH()
- [ ] REGEX_*, LIKE
- [ ] LEVENSHTEIN_DISTANCE()

### Phase 3: Window Functions (Q2 2025)
- [ ] ROW_NUMBER(), RANK(), DENSE_RANK()
- [ ] LAG(), LEAD()
- [ ] Running Aggregates (SUM/AVG OVER)
- [ ] NTILE(), PERCENT_RANK()

### Phase 4: Graph-Erweiterungen (Q3 2025)
- [ ] Pattern Matching (MATCH Syntax)
- [ ] ALL_SHORTEST_PATHS, K_SHORTEST_PATHS
- [ ] WEIGHTED_SHORTEST_PATH
- [ ] Graph-Algorithmen als Funktionen

### Phase 5: Vektor & AI (Q3 2025)
- [ ] VECTOR_DISTANCE() mit Metriken
- [ ] HYBRID_SEARCH()
- [ ] EMBED()
- [ ] RERANK()

### Phase 6: Advanced (Q4 2025)
- [ ] Statistische Funktionen
- [ ] Erweiterte Geo-Funktionen
- [ ] JSON_* Funktionen
- [ ] UPSERT/MERGE Syntax

---

## 5. Kompatibilitäts-Matrix

| Feature | ArangoDB | Neo4j | PostgreSQL | MongoDB | ThemisDB |
|---------|----------|-------|------------|---------|----------|
| FOR/FILTER/RETURN | ✅ | ❌ | ❌ | ❌ | ✅ |
| Graph Traversal | ✅ | ✅ | ❌ | ✅ | ✅ |
| Pattern Matching | ❌ | ✅ | ❌ | ❌ | 🔜 |
| Window Functions | ❌ | ❌ | ✅ | ✅ | 🔜 |
| FULLTEXT | ✅ | ✅ | ✅ | ✅ | 🔜 |
| Vector Search | ❌ | ✅ | ✅ | ✅ | ✅ |
| Geo/Spatial | ✅ | ✅ | ✅ | ✅ | ✅ |
| Aggregation | ✅ | ✅ | ✅ | ✅ | ✅ |
| Subqueries | ✅ | ✅ | ✅ | ✅ | ✅ |
| UPSERT | ✅ | ✅ | ✅ | ✅ | 🔜 |

**Legende:** ✅ = Vorhanden, 🔜 = Geplant, ❌ = Nicht vorhanden
