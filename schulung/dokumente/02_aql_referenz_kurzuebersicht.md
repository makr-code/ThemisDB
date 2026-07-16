# AQL Referenz — Kurzübersicht

> Schnellreferenz für die wichtigsten AQL-Klauseln, Operatoren und Built-in-Funktionen.

---

## Kern-Klauseln

| Klausel | Syntax | Beschreibung |
|---|---|---|
| `FOR` | `FOR var IN collection` | Iteration über Collection oder Array |
| `LET` | `LET x = expression` | Variable definieren |
| `FILTER` | `FILTER condition` | Dokumente filtern |
| `SORT` | `SORT field ASC\|DESC` | Ergebnisse sortieren |
| `LIMIT` | `LIMIT [offset,] count` | Anzahl begrenzen |
| `RETURN` | `RETURN expression` | Ergebnis projizieren |
| `COLLECT` | `COLLECT field = expr` | Gruppieren + Aggregieren |
| `INSERT` | `INSERT doc INTO coll` | Dokument einfügen |
| `UPDATE` | `UPDATE key WITH patch IN coll` | Teilweise aktualisieren |
| `REPLACE` | `REPLACE key WITH doc IN coll` | Vollständig ersetzen |
| `REMOVE` | `REMOVE key IN coll` | Löschen |
| `UPSERT` | `UPSERT filter INSERT doc UPDATE patch IN coll` | Insert oder Update |

---

## Vergleichsoperatoren

| Operator | Bedeutung | Beispiel |
|---|---|---|
| `==` | Gleich | `user.age == 30` |
| `!=` | Ungleich | `user.status != "deleted"` |
| `<` | Kleiner | `order.total < 100` |
| `<=` | Kleiner oder gleich | `item.stock <= 0` |
| `>` | Größer | `product.price > 50` |
| `>=` | Größer oder gleich | `user.age >= 18` |
| `IN` | In Array/Range | `tag IN ["db", "ai"]` |
| `NOT IN` | Nicht in Array | `status NOT IN ["deleted", "banned"]` |
| `LIKE` | Wildcard-Muster | `name LIKE "Anna%"` |
| `=~` | Regex-Muster | `email =~ "^[a-z]+@example\\.com$"` |

---

## Logische Operatoren

```aql
FILTER user.active == true AND user.age > 18
FILTER user.role == "admin" OR user.role == "moderator"
FILTER NOT (user.banned == true)

// Kurzform
FILTER user.active  -- entspricht user.active == true
```

---

## COLLECT — Aggregation

```aql
// Einfaches Zählen
FOR item IN collection
  COLLECT group = item.category
    WITH COUNT INTO count
  RETURN { group, count }

// Mehrere Aggregationsfunktionen
FOR order IN orders
  COLLECT user = order.user_id
    AGGREGATE
      total     = SUM(order.amount),
      count     = COUNT(1),
      avg       = AVG(order.amount),
      maximum   = MAX(order.amount),
      minimum   = MIN(order.amount),
      values    = UNIQUE(order.status)
  RETURN { user, total, count, avg, maximum, minimum, values }
```

---

## Graph-Traversierungen

```aql
// Grundlegende Traversierung
FOR vertex, edge, path
  IN min_depth..max_depth
  OUTBOUND|INBOUND|ANY start_vertex
  GRAPH "graph_name"
  [OPTIONS { uniqueVertices: "global|path|none" }]
  RETURN vertex

// Kürzester Pfad
FOR path IN OUTBOUND SHORTEST_PATH from TO to GRAPH "g" RETURN path

// K kürzeste Pfade
FOR path IN 1..5 OUTBOUND K_SHORTEST_PATHS from TO to GRAPH "g" RETURN path

// Alle einfachen Pfade
FOR path IN OUTBOUND ALL_SHORTEST_PATHS from TO to GRAPH "g" RETURN path
```

---

## LLM-Erweiterungen

```aql
// Text in Embedding umwandeln
LET emb = LLM EMBED "text" USING MODEL "model-name"

// Inferenz
LET result = LLM INFER "prompt" USING MODEL "model-name"
  OPTIONS { max_tokens: 100, temperature: 0.7 }

// RAG
LET answer = LLM RAG "question"
  WITH CONTEXT "context text"
  USING MODEL "model-name"

// Modell-Verwaltung
LLM MODEL LOAD "llama-3.2-1b"
LLM MODEL LIST
LLM MODEL UNLOAD "llama-3.2-1b"
```

---

## String-Funktionen

| Funktion | Beispiel | Ergebnis |
|---|---|---|
| `CONCAT(a, b, ...)` | `CONCAT("Hello", " ", "World")` | `"Hello World"` |
| `CONCAT_SEPARATOR(sep, arr)` | `CONCAT_SEPARATOR(", ", ["a","b"])` | `"a, b"` |
| `UPPER(s)` | `UPPER("hello")` | `"HELLO"` |
| `LOWER(s)` | `LOWER("HELLO")` | `"hello"` |
| `TRIM(s)` | `TRIM("  hi  ")` | `"hi"` |
| `LENGTH(s)` | `LENGTH("hello")` | `5` |
| `SUBSTRING(s, start, len)` | `SUBSTRING("hello", 1, 3)` | `"ell"` |
| `SPLIT(s, sep)` | `SPLIT("a,b,c", ",")` | `["a","b","c"]` |
| `CONTAINS(s, sub)` | `CONTAINS("hello", "ell")` | `true` |
| `STARTS_WITH(s, prefix)` | `STARTS_WITH("hello", "hel")` | `true` |
| `REGEX_MATCHES(s, regex)` | `REGEX_MATCHES("abc123", "^[a-z]+")` | `true` |

---

## Array-Funktionen

| Funktion | Beschreibung |
|---|---|
| `LENGTH(arr)` | Länge des Arrays |
| `APPEND(arr, val)` | Element anhängen |
| `PREPEND(arr, val)` | Element voranstellen |
| `PUSH(arr, val)` | Element hinzufügen |
| `POP(arr)` | Letztes Element entfernen |
| `SHIFT(arr)` | Erstes Element entfernen |
| `REVERSE(arr)` | Array umkehren |
| `FLATTEN(arr [, depth])` | Verschachtelte Arrays glätten |
| `UNIQUE(arr)` | Duplikate entfernen |
| `UNION(a, b)` | Vereinigung |
| `INTERSECTION(a, b)` | Schnittmenge |
| `MINUS(a, b)` | Differenz (a ohne b) |
| `SLICE(arr, start, len)` | Teilarray |
| `SORTED(arr)` | Sortiertes Array |
| `FIRST(arr)` | Erstes Element |
| `LAST(arr)` | Letztes Element |

---

## Datum-Funktionen

| Funktion | Beschreibung |
|---|---|
| `DATE_NOW()` | Aktueller Unix-Timestamp (ms) |
| `DATE_ISO8601(ts)` | Timestamp als ISO-8601-String |
| `DATE_TIMESTAMP(iso)` | ISO-8601-String als Timestamp |
| `DATE_YEAR(ts)` | Jahr extrahieren |
| `DATE_MONTH(ts)` | Monat extrahieren |
| `DATE_DAY(ts)` | Tag extrahieren |
| `DATE_HOUR(ts)` | Stunde extrahieren |
| `DATE_ADD(ts, n, unit)` | Datum addieren |
| `DATE_SUBTRACT(ts, n, unit)` | Datum subtrahieren |
| `DATE_DIFF(ts1, ts2, unit)` | Differenz berechnen |
| `DATE_TRUNC(ts, unit)` | Auf Einheit runden |
| `DATE_FORMAT(ts, format)` | Formatiert ausgeben |

**Einheiten**: `"year"`, `"month"`, `"day"`, `"hour"`, `"minute"`, `"second"`, `"millisecond"`

---

## Mathematische Funktionen

| Funktion | Beschreibung |
|---|---|
| `ABS(n)` | Betrag |
| `CEIL(n)` | Aufrunden |
| `FLOOR(n)` | Abrunden |
| `ROUND(n [, precision])` | Runden |
| `SQRT(n)` | Quadratwurzel |
| `POW(base, exp)` | Potenz |
| `LOG(n)` | Natürlicher Logarithmus |
| `LOG2(n)` | Binärlogarithmus |
| `LOG10(n)` | Dekadischer Logarithmus |
| `EXP(n)` | Exponentialfunktion |
| `SIN/COS/TAN(rad)` | Trigonometrie |
| `MIN(a, b)` | Minimum zweier Werte |
| `MAX(a, b)` | Maximum zweier Werte |
| `RAND()` | Zufallszahl [0, 1) |

---

## Vektor-Funktionen

| Funktion | Beschreibung |
|---|---|
| `COSINE_SIMILARITY(v1, v2)` | Kosinus-Ähnlichkeit [-1, 1] |
| `EUCLIDEAN_DISTANCE(v1, v2)` | Euklidische Distanz |
| `DOT_PRODUCT(v1, v2)` | Skalarprodukt |
| `L2_NORMALIZE(v)` | L2-Normalisierung |
| `VECTOR_MAGNITUDE(v)` | Betrag des Vektors |

---

## Geo-Funktionen

| Funktion | Beschreibung |
|---|---|
| `GEO_DISTANCE(p1, p2)` | Entfernung in Metern (Haversine) |
| `GEO_CONTAINS(polygon, point)` | Punkt in Polygon? |
| `GEO_INTERSECTS(g1, g2)` | Überschneidung? |
| `GEO_AREA(polygon)` | Fläche in m² |
| `GEO_POINT(lon, lat)` | GeoJSON-Punkt erstellen |

---

## DDL — Schnellreferenz

```aql
-- Collections
CREATE COLLECTION name [TYPE VERTEX|EDGE] [(...schema...)]
DROP COLLECTION name
TRUNCATE COLLECTION name

-- Indizes
CREATE INDEX name ON collection(fields) TYPE HASH|SKIPLIST|FULLTEXT|GEO|VECTOR [UNIQUE]
DROP INDEX name

-- Graphen
CREATE GRAPH name EDGE DEFINITION edge_coll FROM v_coll TO v_coll
DROP GRAPH name

-- Views
CREATE VIEW name AS (aql_query)
DROP VIEW name

-- Transaktionen
BEGIN TRANSACTION
COMMIT
ROLLBACK
```

---

## Typ-Prüfungs-Funktionen

| Funktion | Beschreibung |
|---|---|
| `IS_NULL(x)` | Ist null? |
| `IS_BOOL(x)` | Ist Boolean? |
| `IS_NUMBER(x)` | Ist Zahl? |
| `IS_STRING(x)` | Ist String? |
| `IS_ARRAY(x)` | Ist Array? |
| `IS_OBJECT(x)` | Ist Objekt/Dokument? |
| `TYPENAME(x)` | Typname als String |
| `TO_STRING(x)` | In String konvertieren |
| `TO_NUMBER(x)` | In Zahl konvertieren |
| `TO_BOOL(x)` | In Boolean konvertieren |
| `TO_ARRAY(x)` | In Array konvertieren |

---

## Sondervariablen in DML

| Variable | Verfügbar in | Beschreibung |
|---|---|---|
| `OLD` | `UPDATE`, `REPLACE`, `REMOVE` | Dokument vor der Änderung |
| `NEW` | `INSERT`, `UPDATE`, `REPLACE` | Dokument nach der Änderung |
| `CURRENT` | `UPSERT` | Aktuelles Dokument (falls vorhanden) |

```aql
UPDATE user WITH { login_count: user.login_count + 1 } IN users
RETURN { old_count: OLD.login_count, new_count: NEW.login_count }
```
