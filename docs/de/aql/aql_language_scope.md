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

## 🔍 WICHTIGER HINWEIS: Tatsächlicher Implementierungsstatus

**Analysedatum:** 8. Dezember 2024 (Aktualisiert nach Code-Review)

### 🎯 WICHTIGE ENTDECKUNG: Parallele Funktionsimplementierung!

Die ursprüngliche Analyse war unvollständig. Eine tiefere Code-Überprüfung ergab:

**Es gibt ZWEI getrennte Funktions-Systeme im Code:**

1. **Legacy-System** (`/src/query/let_evaluator.cpp`): Hardcodierte Funktionsprüfungen - nur Basis-Funktionen
2. **Modernes Registry-System** (`/include/query/functions/*.h`): Vollständige, modulare Implementierung - ALLE Funktionen!

**Das Problem:** Das moderne Registry-System ist IMPLEMENTIERT, aber noch NICHT in den Query-Ausführungspfad integriert!

### ✅ Was IST im Code vorhanden (Query-Ausführung aktiv):

#### Basis-Funktionen (in let_evaluator.cpp - funktionsfähig):
- **String-Funktionen:** LENGTH, CONCAT, SUBSTRING, UPPER, LOWER
- **Mathematik:** ABS, CEIL, FLOOR, ROUND, MIN, MAX
- **Geo/Spatial:** ST_Point, ST_Distance, ST_Within, ST_Contains, ST_Intersects, ST_DWithin, ST_Buffer, ST_Union, ST_GeomFromText, ST_GeomFromGeoJSON, ST_AsGeoJSON, ST_AsText, ST_3DDistance, ST_Z, ST_ZMin, ST_ZMax
- **Vektor:** SIMILARITY (Vektor-Ähnlichkeitssuche), PROXIMITY (Geo-Nähe)
- **Graph-Traversierung:** FOR v IN 1..n OUTBOUND/INBOUND/ANY, SHORTEST_PATH
- **Aggregation:** COLLECT x = expr, AGGREGATE COUNT/SUM/AVG
- **Window Functions:** ROW_NUMBER, RANK, DENSE_RANK, LAG, LEAD, FIRST_VALUE, LAST_VALUE

**Quelle:** `/src/query/let_evaluator.cpp`, `/src/query/window_evaluator.cpp`

### ✨ Was AUCH im Code existiert (Function Registry - noch nicht integriert):

#### Dokument-Funktionen (in `/include/query/functions/document_functions.h`):
- **Implementiert:** DOCUMENT, MERGE, MERGE_RECURSIVE, UNSET, KEEP, HAS, ATTRIBUTES, VALUES, ZIP, UNZIP
- **Typ-Funktionen:** TYPENAME, IS_NULL, IS_BOOL, IS_NUMBER, IS_STRING, IS_ARRAY, IS_OBJECT
- **Konvertierung:** TO_NUMBER, TO_STRING, TO_BOOL, TO_ARRAY

#### Array-Funktionen (in `/include/query/functions/array_functions.h`):
- **Implementiert:** FIRST, LAST, NTH, PUSH, POP, SHIFT, UNSHIFT, SLICE, FLATTEN, UNIQUE, SORTED, REVERSE_ARRAY, UNION, INTERSECTION, MINUS, POSITION, COUNT, RANGE

#### Datum/Zeit-Funktionen (in `/include/query/functions/date_functions.h`):
- **Implementiert:** 54 Funktionen! Inklusive DATE_NOW, DATE_ADD, DATE_DIFF, DATE_FORMAT, DATE_YEAR, DATE_MONTH, DATE_DAY, CURRENT_TIMESTAMP, WORKDAYS, MAKE_DATE, und viele mehr

#### String-Funktionen (in `/include/query/functions/string_functions.h`):
- **Implementiert:** CONCAT, SUBSTRING, LENGTH, UPPER, LOWER, TRIM, LTRIM, RTRIM, SPLIT, REPLACE, REVERSE, CONTAINS, STARTS_WITH, ENDS_WITH, REGEX_TEST, REGEX_REPLACE, LEVENSHTEIN_DISTANCE

**Quelle:** `/include/query/functions/*.h`, registriert in `/src/query/functions/function_registry.cpp`

### ⚠️ Was fehlt (weder in Legacy noch im Registry):

- **Text/Volltext:** FULLTEXT, TOKENS, PHRASE, SOUNDEX, METAPHONE, NGRAM_MATCH
- **Erweiterte Graph:** ALL_SHORTEST_PATHS, K_SHORTEST_PATHS, PATH_LENGTH, PATH_VERTICES, Graph-Algorithmen
- **AI/ML:** EMBED, CLASSIFY, EXTRACT_ENTITIES, VECTOR_DISTANCE (erweiterte Metriken), HYBRID_SEARCH
- **Erweiterte Geo:** GEO_DISTANCE (Haversine), GEO_AREA, H3_*, ISOCHRONE
- **JSON:** JSON_EXTRACT, JSON_SET, JSON_TYPE (komplexe JSON-Pfad-Operationen)
- **Statistik:** MODE, STDDEV, VARIANCE, CORRELATION, LINEAR_REGRESSION
- **Syntax:** UPSERT, EXISTS, Transaktionen

### 🧪 Tests & Benchmarks Bestätigen Implementierung

**Google Tests:** 191 Test-Fälle in `/tests/test_aql_functions.cpp`
- String-Funktionen: LENGTH, CONCAT, SUBSTRING, UPPER, LOWER, TRIM, SPLIT, REGEX_TEST, LEVENSHTEIN_DISTANCE
- Math-Funktionen: ABS, CEIL, FLOOR, ROUND, SQRT, POW, LOG, SIN, COS, TAN, PI, MIN, MAX, SUM, AVG
- Array-Funktionen: FIRST, LAST, NTH, PUSH, POP, SLICE, FLATTEN, UNIQUE, SORTED, UNION, INTERSECTION, RANGE
- Date-Funktionen: DATE_NOW, DATE_ADD, DATE_DIFF, DATE_FORMAT, DATE_YEAR, DATE_MONTH, DATE_DAY
- Document-Funktionen: MERGE, UNSET, KEEP, HAS, ATTRIBUTES, VALUES, ZIP, UNZIP
- Geo-Funktionen: ST_Point, ST_Distance, ST_Contains, ST_GeomFromText, ST_AsText

**Benchmarks:** 39 Performance-Tests in `/benchmarks/bench_aql_functions.cpp`
- Alle Funktionskategorien haben Benchmark-Coverage
- Performance-Messungen für kritische Operationen

**Zusätzlich:** ~40 weitere AQL-Test-Dateien für spezielle Features (Graph, Fulltext, Parser, etc.)

**Beweis:** Die Funktionen sind vollständig implementiert, getestet UND gebenchmarkt!

**✅ ERLEDIGT - FunctionRegistry Integration abgeschlossen (8. Dezember 2024):**
1. ~~**FunctionRegistry in let_evaluator integrieren**~~ - FERTIG!
   - `src/query/let_evaluator.cpp` nutzt jetzt FunctionRegistry
   - ~140+ Funktionen sofort aktiviert
   - 96 Zeilen redundanter Code entfernt

**Hohe Priorität - Fehlende Funktionen implementieren:**
2. Text/Volltext-Funktionen (FULLTEXT, TOKENS, PHRASE)
3. Erweiterte Graph-Funktionen

**Mittlere Priorität:**
4. AI/ML-Funktionen
5. Statistische Funktionen

---

## 2. Implementierungsstatus - Aktualisiert 8. Dezember 2024

> **⚠️ WICHTIGER HINWEIS:** Die folgende Analyse basiert auf einer Code-Überprüfung vom 8. Dezember 2024.
> Viele zuvor als "✅ implementiert" markierte Funktionen sind tatsächlich **NICHT im Sourcecode vorhanden**.
> Dieser Abschnitt wurde korrigiert, um den **tatsächlichen** Stand widerzuspiegeln.

### 2.1 ⚠️ Implementiert aber nicht integriert - Document/Array/Date/String Funktionen

> **STATUS:** Diese Funktionen sind vollständig im Code implementiert (`/include/query/functions/*.h`),
> aber der Query-Executor (`let_evaluator.cpp`) nutzt das Function Registry-System noch nicht.
> **Integration erforderlich, KEINE Neu-Implementierung!**

#### Dokument-Funktionen ⚠️ (Implementiert in `document_functions.h`, nicht integriert)

```aql
-- DOCUMENT() - Dokument per ID laden ⚠️ Implementiert, nicht integriert
LET customer = DOCUMENT("customers", order.customerId)

-- MERGE() - Objekte zusammenführen ⚠️ Implementiert, nicht integriert
LET merged = MERGE(doc1, doc2, { extra: "field" })

-- UNSET() - Felder entfernen ⚠️ Implementiert, nicht integriert
LET cleaned = UNSET(doc, ["password", "internal"])

-- KEEP() - Nur bestimmte Felder behalten ⚠️ Implementiert, nicht integriert
LET subset = KEEP(doc, ["name", "email"])

-- HAS() - Feld-Existenz prüfen ⚠️ Implementiert, nicht integriert
FILTER HAS(doc, "optionalField")

-- ATTRIBUTES() - Alle Feldnamen ⚠️ Implementiert, nicht integriert
LET fields = ATTRIBUTES(doc)

-- VALUES() - Alle Feldwerte ⚠️ Implementiert, nicht integriert
LET vals = VALUES(doc)
```

**Status:** Vollständig implementiert in `/include/query/functions/document_functions.h`
**Problem:** `let_evaluator.cpp` nutzt FunctionRegistry nicht
**Lösung:** FunctionRegistry-Integration (Aufwand: ~1 Woche)

#### Array-Funktionen ⚠️ (Implementiert in `array_functions.h`, nicht integriert)

```aql
-- FLATTEN() - Verschachtelte Arrays flachen ⚠️ Implementiert, nicht integriert
LET flat = FLATTEN([[1,2], [3,4]])  -- [1,2,3,4]

-- UNIQUE() - Duplikate entfernen ⚠️ Implementiert, nicht integriert
LET unique = UNIQUE([1,1,2,2,3])  -- [1,2,3]

-- UNION() / INTERSECTION() / MINUS() ⚠️ Implementiert, nicht integriert
LET combined = UNION(arr1, arr2)
LET common = INTERSECTION(arr1, arr2)
LET diff = MINUS(arr1, arr2)

-- FIRST() / LAST() / NTH() ⚠️ Implementiert, nicht integriert
LET first = FIRST(arr)
LET last = LAST(arr)
LET third = NTH(arr, 2)

-- SLICE() - Teilarray ⚠️ Implementiert, nicht integriert
LET sub = SLICE(arr, 1, 3)

-- REVERSE() - Umkehren ⚠️ Implementiert, nicht integriert
LET rev = REVERSE(arr)

-- SORTED() / SORTED_UNIQUE() ⚠️ Implementiert, nicht integriert
LET sorted = SORTED(arr)
LET sortedUnique = SORTED_UNIQUE(arr)

-- CONTAINS_ARRAY() - Array enthält Element ⚠️ Nicht implementiert
FILTER CONTAINS_ARRAY(doc.tags, "important")

-- ARRAY_AGG() - In Aggregation ⚠️ Nicht implementiert
COLLECT category = doc.category AGGREGATE items = ARRAY_AGG(doc)
```

**Status:** 18 Array-Funktionen in `/include/query/functions/array_functions.h`
**Nicht vorhanden:** CONTAINS_ARRAY, ARRAY_AGG, SORTED_UNIQUE
**Lösung:** FunctionRegistry-Integration (Aufwand: ~1 Woche)

#### Datum/Zeit-Funktionen ⚠️ (54 Funktionen in `date_functions.h`, nicht integriert)

```aql
-- DATE_NOW() - Aktueller Zeitstempel ⚠️ Implementiert, nicht integriert
LET now = DATE_NOW()

-- DATE_ISO8601(ts) - Timestamp zu ISO-String ⚠️ Implementiert, nicht integriert
LET isoStr = DATE_ISO8601(doc.timestamp)

-- DATE_TIMESTAMP(iso) - ISO-String zu Timestamp ⚠️ Implementiert, nicht integriert
LET ts = DATE_TIMESTAMP("2024-01-15T10:30:00Z")

-- DATE_YEAR/MONTH/DAY/HOUR/MINUTE/SECOND ⚠️ Implementiert, nicht integriert
LET year = DATE_YEAR(doc.created)
LET month = DATE_MONTH(doc.created)

-- DATE_ADD/SUBTRACT ⚠️ Implementiert, nicht integriert
LET nextWeek = DATE_ADD(now, 7, "day")
LET lastMonth = DATE_SUBTRACT(now, 1, "month")

-- DATE_DIFF ⚠️ Implementiert, nicht integriert
LET daysDiff = DATE_DIFF(start, end, "day")

-- DATE_TRUNC - Auf Periode runden ⚠️ Implementiert, nicht integriert
LET monthStart = DATE_TRUNC(doc.created, "month")

-- DATE_FORMAT ⚠️ Implementiert, nicht integriert
LET formatted = DATE_FORMAT(doc.created, "%Y-%m-%d")

-- DATE_COMPARE ⚠️ Implementiert, nicht integriert
FILTER DATE_COMPARE(doc.expires, now) > 0
```

**Status:** Vollständige Datum/Zeit-Bibliothek mit 54 Funktionen in `/include/query/functions/date_functions.h`
**Zusätzlich verfügbar:** CURRENT_TIMESTAMP, WORKDAYS, MAKE_DATE, AGE, IS_WEEKEND, und 40+ mehr
**Lösung:** FunctionRegistry-Integration (Aufwand: ~1 Woche)

### 2.2 ❌ Wichtig - NICHT IMPLEMENTIERT (zuvor fälschlicherweise als ✅ markiert)

#### Text/Volltext-Funktionen ❌

```aql
-- FULLTEXT() - Volltextsuche ❌ NICHT IMPLEMENTIERT
FOR doc IN collection
  FILTER FULLTEXT(doc.content, "search term")
  RETURN doc

-- TOKENS() - Text tokenisieren ❌ NICHT IMPLEMENTIERT
LET words = TOKENS("Hello World", "text_en")

-- PHRASE() - Phrasensuche ❌ NICHT IMPLEMENTIERT
FILTER PHRASE(doc.content, "exact phrase")

-- LEVENSHTEIN_DISTANCE() - Edit-Distanz ❌ NICHT IMPLEMENTIERT
LET dist = LEVENSHTEIN_DISTANCE("hello", "hallo")

-- SOUNDEX() / METAPHONE() - Phonetische Suche ❌ NICHT IMPLEMENTIERT
FILTER SOUNDEX(doc.name) == SOUNDEX("Meyer")

-- NGRAM_MATCH() - N-Gram Matching ❌ NICHT IMPLEMENTIERT
FILTER NGRAM_MATCH(doc.title, "searc", 0.7)

-- REGEX_TEST() / REGEX_MATCHES() / REGEX_REPLACE() ❌ NICHT IMPLEMENTIERT
FILTER REGEX_TEST(doc.email, "^[a-z]+@")
LET matches = REGEX_MATCHES(doc.text, "\\d+")
LET cleaned = REGEX_REPLACE(doc.phone, "[^0-9]", "")

-- LIKE mit Wildcards ❌ NICHT IMPLEMENTIERT
FILTER doc.name LIKE "John%"
FILTER doc.code LIKE "A__B"
```

**Status:** Diese Funktionen sind im Sourcecode nicht vorhanden.
**Quelle überprüft:** `/src/query/let_evaluator.cpp`, `/src/query/aql_translator.cpp`

#### Window Functions ✅ TATSÄCHLICH IMPLEMENTIERT

```aql
-- ROW_NUMBER() OVER ✅ IMPLEMENTIERT
FOR doc IN collection
  LET rowNum = ROW_NUMBER() OVER (
    PARTITION BY doc.category 
    ORDER BY doc.sales DESC
  )
  FILTER rowNum <= 3
  RETURN doc

-- RANK() / DENSE_RANK() ✅ IMPLEMENTIERT
LET rank = RANK() OVER (ORDER BY doc.score DESC)

-- LAG() / LEAD() - Vorherige/Nächste Zeile ✅ IMPLEMENTIERT
LET prevValue = LAG(doc.value, 1) OVER (ORDER BY doc.date)
LET nextValue = LEAD(doc.value, 1) OVER (ORDER BY doc.date)

-- SUM/AVG/COUNT OVER (Fenster) ❌ NICHT IMPLEMENTIERT
LET runningSum = SUM(doc.amount) OVER (
  ORDER BY doc.date 
  ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW
)

-- MEDIAN() / PERCENTILE() ❌ NICHT IMPLEMENTIERT
LET median = MEDIAN(values)
LET p95 = PERCENTILE(values, 0.95)
```

**Status:** ROW_NUMBER, RANK, DENSE_RANK, LAG, LEAD sind implementiert.
**Quelle bestätigt:** `/src/query/window_evaluator.cpp`, `/include/query/window_evaluator.h`
**Nicht implementiert:** Aggregat-Fenster-Funktionen (SUM/AVG OVER), MEDIAN, PERCENTILE

#### Graph-Erweiterungen ❌ (teilweise implementiert)

```aql
-- ALL_SHORTEST_PATHS ❌ NICHT IMPLEMENTIERT
FOR path IN ALL_SHORTEST_PATHS(start, end, { edgeCollection: "edges" })
  RETURN path

-- K_SHORTEST_PATHS ❌ NICHT IMPLEMENTIERT
FOR path IN K_SHORTEST_PATHS(start, end, 5, { edgeCollection: "edges" })
  RETURN path

-- WEIGHTED_SHORTEST_PATH ❌ NICHT IMPLEMENTIERT
LET path = WEIGHTED_SHORTEST_PATH(start, end, "distance")

-- PATH_LENGTH() / PATH_VERTICES() / PATH_EDGES() ❌ NICHT IMPLEMENTIERT
LET len = PATH_LENGTH(path)
LET nodes = PATH_VERTICES(path)
LET rels = PATH_EDGES(path)

-- Graph Algorithms als Funktionen ❌ NICHT IMPLEMENTIERT
LET communities = LOUVAIN_COMMUNITIES(graph)
LET centrality = BETWEENNESS_CENTRALITY(graph)
LET closeness = CLOSENESS_CENTRALITY(graph)
```

**Status:** Diese erweiterten Graph-Funktionen sind im Sourcecode nicht vorhanden.
**Hinweis:** Basis-Graph-Traversierung (FOR v IN 1..n OUTBOUND/INBOUND/ANY) und SHORTEST_PATH sind implementiert (siehe Sektion 1.4)
**Quelle überprüft:** `/src/query/`, `/include/query/`

### 2.3 ❌ Enterprise - NICHT IMPLEMENTIERT (zuvor fälschlicherweise als ✅ markiert)

#### Vektor/AI-Erweiterungen ❌

```aql
-- VECTOR_DISTANCE() - Verschiedene Metriken ❌ NICHT IMPLEMENTIERT
LET dist = COSINE_SIMILARITY(vec1, vec2)
LET dist = EUCLIDEAN_DISTANCE(vec1, vec2)

-- VECTOR_NORMALIZE() ❌ NICHT IMPLEMENTIERT
LET normalized = L2_NORMALIZE(vec)

-- HYBRID_SEARCH() - Kombination von Vektor + Keyword ❌ NICHT IMPLEMENTIERT
FOR doc IN HYBRID_SEARCH(collection, query, vectorField, textField, {
  vectorWeight: 0.7,
  textWeight: 0.3
})
  RETURN doc

-- RERANK() - Ergebnisse neu ordnen ❌ NICHT IMPLEMENTIERT
LET reranked = RERANK(results, query, "cross-encoder")

-- EMBED() - Text zu Vektor ❌ NICHT IMPLEMENTIERT
LET embedding = EMBED("This is a text", "text-embedding-3-small")

-- CLASSIFY() - Textklassifikation ❌ NICHT IMPLEMENTIERT
LET result = CLASSIFY(text, ["positive", "negative", "neutral"])

-- EXTRACT_ENTITIES() - NER ❌ NICHT IMPLEMENTIERT
LET entities = EXTRACT_ENTITIES(text, ["PERSON", "ORG", "LOCATION"])
```

**Status:** Diese Funktionen sind im Sourcecode nicht vorhanden.
**Hinweis:** Basis-Vektor-Funktionen SIMILARITY() und PROXIMITY() sind implementiert (siehe Sektion 1.3)
**Quelle überprüft:** `/src/query/let_evaluator.cpp`, `/src/query/aql_translator.cpp`

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

## 4. Implementierungs-Roadmap (Korrigiert basierend auf tatsächlichem Stand)

> **Hinweis:** Diese Roadmap wurde aktualisiert, um den **tatsächlichen** Implementierungsstand zu reflektieren.
> Viele als "implementiert" markierte Funktionen sind noch nicht vorhanden.

### ✅ Bereits Implementiert (abgeschlossen):
- Basis-String-Funktionen: LENGTH, CONCAT, SUBSTRING, UPPER, LOWER
- Mathematische Funktionen: ABS, CEIL, FLOOR, ROUND, MIN, MAX
- Geo/Spatial-Funktionen: ST_Point, ST_Distance, ST_Within, ST_Contains, ST_Intersects, etc.
- Vektor-Funktionen: SIMILARITY, PROXIMITY
- Graph-Traversierung: FOR v IN 1..n OUTBOUND/INBOUND/ANY, SHORTEST_PATH
- Aggregation: COLLECT, COUNT, SUM, AVG
- Window Functions: ROW_NUMBER, RANK, DENSE_RANK, LAG, LEAD

### Phase 1: Basis-Funktionen (Q1 2025) - NOCH ZU IMPLEMENTIEREN
- [ ] DOCUMENT(), MERGE(), UNSET(), KEEP()
- [ ] HAS(), ATTRIBUTES(), VALUES()
- [ ] Array-Funktionen (FLATTEN, UNIQUE, UNION, INTERSECTION, MINUS, FIRST, LAST, NTH, SLICE, REVERSE, SORTED, SORTED_UNIQUE, CONTAINS_ARRAY)
- [ ] DATE_* Funktionen (alle): DATE_NOW, DATE_ISO8601, DATE_TIMESTAMP, DATE_YEAR, DATE_MONTH, DATE_DAY, DATE_HOUR, DATE_MINUTE, DATE_SECOND, DATE_ADD, DATE_SUBTRACT, DATE_DIFF, DATE_TRUNC, DATE_FORMAT, DATE_COMPARE

**Priorität:** HOCH - Diese sind Basis-Funktionalität für Dokument-Datenbanken  
**Aufwand:** MITTEL - Können mit Standard-Bibliotheken implementiert werden

### Phase 2: Text & Suche (Q2 2025) - NOCH ZU IMPLEMENTIEREN
- [ ] FULLTEXT(), PHRASE()
- [ ] TOKENS(), NGRAM_MATCH()
- [ ] REGEX_TEST(), REGEX_MATCHES(), REGEX_REPLACE()
- [ ] LEVENSHTEIN_DISTANCE()
- [ ] SOUNDEX(), METAPHONE()
- [ ] LIKE mit Wildcards

**Priorität:** HOCH - Wichtig für Text-Suchfunktionalität  
**Aufwand:** HOCH - FULLTEXT benötigt Text-Indexierung

### Phase 3: Window Functions Erweiterungen (Q2 2025) - TEILWEISE ZU IMPLEMENTIEREN
- [x] ROW_NUMBER(), RANK(), DENSE_RANK() - ✅ Implementiert
- [x] LAG(), LEAD() - ✅ Implementiert
- [ ] Running Aggregates (SUM/AVG/COUNT OVER)
- [ ] NTILE(), PERCENT_RANK()
- [ ] MEDIAN(), PERCENTILE()

**Priorität:** MITTEL - Erweitert vorhandene Window Functions  
**Aufwand:** MITTEL

### Phase 4: Graph-Erweiterungen (Q3 2025) - NOCH ZU IMPLEMENTIEREN
- [x] Basis-Traversierung (OUTBOUND/INBOUND/ANY) - ✅ Implementiert
- [x] SHORTEST_PATH - ✅ Implementiert
- [ ] Pattern Matching (MATCH Syntax)
- [ ] ALL_SHORTEST_PATHS, K_SHORTEST_PATHS
- [ ] WEIGHTED_SHORTEST_PATH
- [ ] PATH_LENGTH(), PATH_VERTICES(), PATH_EDGES()
- [ ] Graph-Algorithmen als Funktionen (LOUVAIN_COMMUNITIES, BETWEENNESS_CENTRALITY, etc.)

**Priorität:** MITTEL - Erweitert vorhandene Graph-Funktionalität  
**Aufwand:** HOCH - Benötigt Graph-Algorithmen-Bibliothek

### Phase 5: Vektor & AI (Q3 2025) - NOCH ZU IMPLEMENTIEREN
- [x] Basis-SIMILARITY() - ✅ Implementiert
- [ ] VECTOR_DISTANCE() mit verschiedenen Metriken (COSINE_SIMILARITY, EUCLIDEAN_DISTANCE)
- [ ] VECTOR_NORMALIZE(), L2_NORMALIZE()
- [ ] HYBRID_SEARCH()
- [ ] EMBED()
- [ ] RERANK()
- [ ] CLASSIFY()
- [ ] EXTRACT_ENTITIES()

**Priorität:** NIEDRIG - Enterprise-Features  
**Aufwand:** SEHR HOCH - Benötigt ML-Framework-Integration

### Phase 6: Advanced (Q4 2025) - NOCH ZU IMPLEMENTIEREN
- [ ] Statistische Funktionen (MODE, STDDEV, VARIANCE, CORRELATION, LINEAR_REGRESSION, HISTOGRAM)
- [ ] RANDOM(), RANDOM_INT(), SAMPLE()
- [ ] Erweiterte Geo-Funktionen (GEO_DISTANCE, GEO_AREA, H3_*, etc.)
- [ ] JSON_* Funktionen (JSON_EXTRACT, JSON_SET, JSON_TYPE, etc.)
- [ ] UPSERT/MERGE Syntax
- [ ] EXISTS/NOT EXISTS Subqueries
- [ ] Transaktionale Kontrolle (BEGIN/COMMIT/ROLLBACK)

**Priorität:** NIEDRIG - Nice-to-have Features  
**Aufwand:** VARIABEL (JSON: MITTEL, Transaktionen: SEHR HOCH)

---

## 5. Kompatibilitäts-Matrix (Korrigiert - Tatsächlicher Stand)

| Feature | ArangoDB | Neo4j | PostgreSQL | MongoDB | ThemisDB (alt) | ThemisDB (neu) |
|---------|----------|-------|------------|---------|----------------|----------------|
| FOR/FILTER/RETURN | ✅ | ❌ | ❌ | ❌ | ✅ | ✅ |
| Graph Traversal (Basis) | ✅ | ✅ | ❌ | ✅ | ✅ | ✅ |
| Pattern Matching | ❌ | ✅ | ❌ | ❌ | 🔜 | ❌ |
| Window Functions | ❌ | ❌ | ✅ | ✅ | 🔜 | ✅ |
| FULLTEXT | ✅ | ✅ | ✅ | ✅ | 🔜 | ❌ |
| Vector Search | ❌ | ✅ | ✅ | ✅ | ✅ | ✅ (Basis) |
| Geo/Spatial | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Aggregation | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Subqueries | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Array Functions | ✅ | ❌ | ✅ | ✅ | ✅ | ❌ |
| Date/Time Functions | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| Document Functions | ✅ | ❌ | ❌ | ✅ | ✅ | ❌ |
| Graph Algorithms | ✅ | ✅ | ❌ | ❌ | ✅ | ❌ |
| UPSERT | ✅ | ✅ | ✅ | ✅ | 🔜 | ❌ |

**Legende:** 
- ✅ = Vorhanden und funktionsfähig
- ❌ = Nicht vorhanden
- 🔜 = War als "geplant" markiert, aber nicht implementiert
- **(Basis)** = Nur grundlegende Funktionalität

**Änderungen zwischen "ThemisDB (alt)" und "ThemisDB (neu)":**
- **Array Functions:** ✅ → ❌ (dokumentiert aber nicht implementiert)
- **Date/Time Functions:** ✅ → ❌ (dokumentiert aber nicht implementiert)
- **Document Functions:** ✅ → ❌ (dokumentiert aber nicht implementiert)
- **Graph Algorithms:** ✅ → ❌ (dokumentiert aber nicht implementiert)
- **Window Functions:** 🔜 → ✅ (tatsächlich implementiert!)
- **Pattern Matching:** 🔜 → ❌ (geplant, nicht implementiert)
- **FULLTEXT:** 🔜 → ❌ (geplant, nicht implementiert)
- **UPSERT:** 🔜 → ❌ (geplant, nicht implementiert)

**Tatsächliche Stärken von ThemisDB (Stand Dez. 2024):**
1. ✅ Robuste Basis-Abfragesprache (FOR/FILTER/RETURN)
2. ✅ Funktionierende Graph-Traversierung (OUTBOUND/INBOUND/ANY, SHORTEST_PATH)
3. ✅ Umfassende Geo/Spatial-Unterstützung (inkl. 3D)
4. ✅ Basis-Vektor-Suche (SIMILARITY, PROXIMITY)
5. ✅ Window Functions (ROW_NUMBER, RANK, LAG, LEAD)
6. ✅ Basis-Aggregation (COLLECT, COUNT, SUM, AVG)

**Hauptlücken (im Vergleich zu ArangoDB/MongoDB):**
1. ❌ Keine Array-Funktionen (FLATTEN, UNIQUE, etc.)
2. ❌ Keine Datum/Zeit-Funktionen
3. ❌ Keine Dokument-Funktionen (MERGE, UNSET, etc.)
4. ❌ Keine Volltext-Suche
5. ❌ Keine erweiterten Graph-Algorithmen
