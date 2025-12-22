# AQL Vollständiger Sprachumfang - Keywords + Funktionen

**Datum:** 22. Dezember 2025  
**Zweck:** Vollständige Kalkulation des AQL-Sprachumfangs (Keywords + Funktionen)  
**Status:** Komplette Analyse für v1.3.0 und v1.3.1 Proposal

---

## Executive Summary

| Aspekt | v1.3.0 (Aktuell) | v1.3.1 (Final) | Änderung |
|--------|------------------|----------------|----------|
| **Reservierte Keywords** | 72 | **119** | **+47 (+65%)** |
| **Eingebaute Funktionen** | ~360 | ~360 (unverändert) | 0 |
| **Gesamt-Sprachumfang** | **432** | **479** | **+47 (+11%)** |

### Wichtige Erkenntnis

Der **Gesamt-Sprachumfang wächst nur um 11%** (von 432 auf 479), da:
- Die 47 neuen Keywords hauptsächlich **Sprachkonstrukte** sind (TYPE, FUNCTION, CLASS, etc.)
- Die **~360 Funktionen bleiben unverändert** (keine neuen Funktionen in v1.3.1)
- v1.3.1 fokussiert auf **strukturelle Features** (OOP), nicht neue Funktionen
- **Detection Types als Strings** (objects, text, faces, landmarks) → -8 Keywords

---

## Detaillierte Kalkulation v1.3.0

### 1. Reservierte Keywords (72)

#### Core Language (8)
```
FOR, IN, LET, FILTER, COLLECT, SORT, LIMIT, RETURN
```

#### Logical Operators (3)
```
AND, OR, NOT
```

#### Aggregation (8)
```
COUNT, SUM, AVG, MIN, MAX, VARIANCE, STDDEV, UNIQUE
```

#### Graph (4)
```
OUTBOUND, INBOUND, ANY, SHORTEST_PATH
```

#### DDL (5)
```
CREATE, DROP, COLLECTION, INDEX, VIEW
```

#### DML (6)
```
INSERT, UPDATE, REPLACE, REMOVE, UPSERT, INTO, WITH
```

#### LLM (13)
```
LLM, INFER, RAG, EMBED, MODEL, LORA, STATS, CACHE,
LOAD, UNLOAD, LIST, INGEST, BLOB
```

#### Index Types (7)
```
HASH, SKIPLIST, FULLTEXT, GEO, PERSISTENT, TTL, VECTOR
```

#### Options & Modifiers (13)
```
OPTIONS, DISTINCT, ASC, DESC, FROM, TO, TOP, USING,
PIN, REPLICATE, VERSION, RESPONSE, PREFIX
```

#### Literals (3)
```
null, true, false
```

#### Reserved (2)
```
LIKE, WHERE
```

**Summe Keywords: 72**

---

### 2. Eingebaute Funktionen (~360)

#### 2.1 Date/Time Funktionen (45)

**Aktuelle Zeit/Datum (11):**
```
DATE_NOW, NOW, CURRENT_TIMESTAMP, CURRENT_DATE, CURRENT_TIME,
TODAY, YESTERDAY, TOMORROW, GETDATE, SYSDATE, UNIX_TIMESTAMP
```

**Interval-Funktionen (8):**
```
YEARS, MONTHS, WEEKS, DAYS, HOURS, MINUTES, SECONDS, INTERVAL
```

**Komponenten (11):**
```
DATE_YEAR, DATE_MONTH, DATE_DAY, DATE_HOUR, DATE_MINUTE,
DATE_SECOND, DATE_MILLISECOND, DATE_DAYOFWEEK, DATE_DAYOFYEAR,
DATE_QUARTER, DATE_WEEK
```

**Konstruktion (5):**
```
MAKE_DATE, MAKE_DATETIME, MAKE_TIME, FROM_UNIXTIME, EPOCH_SECONDS
```

**Arbeitstage (6):**
```
WORKDAYS, WORKDAYS_ADD, IS_WEEKEND, IS_WORKDAY, HOLIDAYS,
HOLIDAYS_BETWEEN, LIST_CALENDARS
```

**Hilfsfunktionen (4):**
```
DATE_LEAPYEAR, DATE_DAYS_IN_MONTH, DATE_START_OF_WEEK,
DATE_END_OF_MONTH, AGE, DATE_COMPARE, DATE_BETWEEN
```

**Formatierung & Arithmetik (4):**
```
DATE_FORMAT, DATE_ISO8601, DATE_TRUNC, DATE_PARSE,
DATE_ADD, DATE_SUBTRACT, DATE_DIFF
```

#### 2.2 String-Funktionen (20)
```
CONCAT, CONCAT_SEPARATOR, SUBSTRING, UPPER, LOWER, TRIM,
LTRIM, RTRIM, LENGTH, REVERSE, REPLACE, SPLIT, JOIN,
STARTS_WITH, ENDS_WITH, CONTAINS, FIND, REGEX_TEST,
REGEX_MATCHES, REGEX_REPLACE
```

#### 2.3 Math-Funktionen (30)
```
ABS, CEIL, FLOOR, ROUND, SQRT, POW, EXP, LOG, LOG10, LN,
SIN, COS, TAN, ASIN, ACOS, ATAN, ATAN2, SINH, COSH, TANH,
MIN, MAX, RAND, RAND_INT, PI, E, SIGN, TRUNC, MOD, QUOTIENT
```

#### 2.4 Array-Funktionen (20)
```
PUSH, POP, SHIFT, UNSHIFT, APPEND, PREPEND, FIRST, LAST,
NTH, SLICE, FLATTEN, UNIQUE, UNION, INTERSECTION, DIFFERENCE,
MAP, FILTER, REDUCE, SORT_ARRAY, REVERSE_ARRAY
```

#### 2.5 Document-Funktionen (20)
```
DOCUMENT, HAS, ATTRIBUTES, VALUES, KEYS, MERGE, UNSET,
KEEP, TRANSLATE, ZIP, UNZIP, MATCHES, IS_SAME_COLLECTION,
PARSE_IDENTIFIER, IS_KEY, COLLECTION_COUNT, SCHEMA_GET,
SCHEMA_VALIDATE, PARSE_JSON, TO_JSON
```

#### 2.6 Collection-Funktionen (25)
```
LENGTH, COUNT_DISTINCT, PUSH_UNIQUE, REMOVE_VALUE,
REMOVE_VALUES, REMOVE_NTH, REPLACE_NTH, SHIFT_LEFT,
SHIFT_RIGHT, POSITION, FIND_FIRST, FIND_LAST, CONTAINS_ARRAY,
JACCARD, COSINE, EDIT_DISTANCE, SOUNDEX, LEVENSHTEIN,
NTH_PERCENTILE, AVERAGE, MEDIAN, VARIANCE_POPULATION,
STDDEV_POPULATION, RANGE, OUTERSECTION
```

#### 2.7 Geo-Funktionen (25)
```
GEO_POINT, GEO_MULTIPOINT, GEO_LINESTRING, GEO_MULTILINESTRING,
GEO_POLYGON, GEO_MULTIPOLYGON, GEO_DISTANCE, GEO_CONTAINS,
GEO_INTERSECTS, GEO_AREA, GEO_EQUALS, GEO_IN_RANGE,
ST_Point, ST_Distance, ST_Contains, ST_Within, ST_Intersects,
ST_Union, ST_Buffer, ST_Area, ST_Length, ST_Centroid,
ST_Envelope, ST_ConvexHull, ST_Simplify
```

#### 2.8 CRS-Transformationen (10)
```
ST_TRANSFORM, ST_SRID, ST_SetSRID, WGS84_TO_UTM, UTM_TO_WGS84,
ETRS89_TO_WGS84, WGS84_TO_ETRS89, HELMERT_TRANSFORM,
GAUSS_KRUEGER_TO_UTM, UTM_TO_GAUSS_KRUEGER
```

#### 2.9 Vector-Funktionen (20)
```
COSINE_SIMILARITY, DOT_PRODUCT, EUCLIDEAN_DISTANCE,
MANHATTAN_DISTANCE, HAMMING_DISTANCE, JACCARD_SIMILARITY,
VECTOR_NORM, VECTOR_NORMALIZE, VECTOR_ADD, VECTOR_SUBTRACT,
VECTOR_MULTIPLY, VECTOR_DIVIDE, VECTOR_DOT, VECTOR_CROSS,
L1_DISTANCE, L2_DISTANCE, CHEBYSHEV_DISTANCE, KNN_SEARCH,
ANNOY_SEARCH, HNSW_SEARCH
```

#### 2.10 Graph-Funktionen (15)
```
SHORTEST_PATH, ALL_SHORTEST_PATHS, K_SHORTEST_PATHS,
NEIGHBORS, CONNECTED_COMPONENTS, BETWEENNESS_CENTRALITY,
CLOSENESS_CENTRALITY, EIGENVECTOR_CENTRALITY, PAGERANK,
GRAPH_DISTANCE, GRAPH_DIAMETER, GRAPH_ECCENTRICITY,
GRAPH_RADIUS, CLIQUE_NUMBER, TRIANGLE_COUNT
```

#### 2.11 Logical-Funktionen (20)
```
IF, IFNULL, COALESCE, NULLIF, ISNULL, NOT_NULL, IS_NULL,
IS_BOOL, IS_NUMBER, IS_STRING, IS_ARRAY, IS_OBJECT,
IS_DATESTRING, TO_BOOL, TO_NUMBER, TO_STRING, TO_ARRAY,
TO_OBJECT, ASSERT, WARN
```

#### 2.12 File/Path-Funktionen (20)
```
PATH_JOIN, PATH_DIRNAME, PATH_BASENAME, PATH_EXTENSION,
PATH_NORMALIZE, PATH_RELATIVE, PATH_ABSOLUTE, PATH_SPLIT,
PATH_PARENT, PATH_IS_ABSOLUTE, PATH_IS_RELATIVE, FILENAME,
FILENAME_WITHOUT_EXT, FILE_EXT, SANITIZE_FILENAME, MIME_TYPE,
IS_IMAGE, IS_VIDEO, IS_AUDIO, IS_DOCUMENT, FORMAT_FILESIZE,
PARSE_FILESIZE
```

#### 2.13 Security-Funktionen (15)
```
HASH_MD5, HASH_SHA1, HASH_SHA256, HASH_SHA512, HMAC,
ENCRYPT_AES, DECRYPT_AES, ENCRYPT_RSA, DECRYPT_RSA,
RANDOM_TOKEN, UUID, BASE64_ENCODE, BASE64_DECODE,
URL_ENCODE, URL_DECODE
```

#### 2.14 Excel-kompatible Funktionen (30)

**Lookup & Reference (4):**
```
VLOOKUP, HLOOKUP, INDEX, MATCH
```

**Text-Funktionen (6):**
```
PROPER, SUBSTITUTE, REPT, EXACT, TEXT, VALUE
```

**Statistische Funktionen (6):**
```
SUMPRODUCT, AVERAGEIF, RANK, LARGE, SMALL, MODE
```

**Math-Funktionen (4):**
```
PRODUCT, FACT, MOD, QUOTIENT
```

**Informations-Funktionen (7):**
```
ISERROR, ISBLANK, ISTEXT, ISNUMBER, ISLOGICAL, TYPE, N
```

**Financial Functions (3):**
```
PMT, FV, PV
```

#### 2.15 Relational-Funktionen (25)
```
ROW_NUMBER, RANK_WINDOW, DENSE_RANK, NTILE, PERCENT_RANK,
CUME_DIST, FIRST_VALUE, LAST_VALUE, NTH_VALUE, LAG, LEAD,
SUM_OVER, AVG_OVER, MIN_OVER, MAX_OVER, COUNT_OVER,
VARIANCE_OVER, STDDEV_OVER, RATIO_TO_REPORT, PERCENTILE_CONT,
PERCENTILE_DISC, MEDIAN_OVER, LISTAGG, PIVOT, UNPIVOT
```

#### 2.16 Process Mining (15)
```
PROCESS_DISCOVER, PROCESS_CONFORMANCE, PROCESS_VARIANTS,
PROCESS_PERFORMANCE, PROCESS_BOTTLENECKS, CASE_DURATION,
ACTIVITY_FREQUENCY, TRANSITION_MATRIX, BPMN_EXTRACT,
PETRI_NET, ALPHA_MINER, HEURISTIC_MINER, INDUCTIVE_MINER,
DOTTED_CHART, SOCIAL_NETWORK
```

**Summe Funktionen: ~360**

---

## Detaillierte Kalkulation v1.3.1 Proposal

### 1. Neue Keywords (+47) ✅ FINAL

#### OOP Extensions (18)
```
NAMESPACE, IMPORT, TYPE, FUNCTION, CLASS, PUBLIC, PRIVATE,
CONSTRUCTOR, METHOD, NEW, THIS, SELF, EXTENDS,
String, Int, Float, Bool, Any, Object
```

#### Control Flow (10)
```
IF, THEN, ELSE, ELSEIF, ENDIF, TRY, CATCH, THROW, CASE, END
```

#### Pattern Matching (2)
```
MATCH, WHEN
```

#### Async Operations (4)
```
ASYNC, AWAIT, PARALLEL, TIMEOUT
```

#### Vision Extensions (9) ✅ OPTIMIERT
```
VISION, ANALYZE, DETECT, QUESTION, ABOUT, IMAGE, IMAGES,
TRANSFORM, COMPARE, BATCH, OPERATIONS, OUTPUT, METRIC
```

**✅ Detection Types als Strings:**
```aql
-- Statt Keywords: DETECT [objects, text, faces]
-- Jetzt Strings: DETECT ['objects', 'text', 'faces']
```
**Einsparung:** -8 Keywords (objects, text, faces, landmarks, emotions, brands, celebrities, scenes)

#### Type Keywords (3)
```
Array, Map, Result
```

#### Macros (1)
```
MACRO
```

**Summe neue Keywords: +47** (statt +55)

**Gesamt Keywords v1.3.1: 119** ✅ FINAL  
**Gesamt Keywords v1.3.1 (minimal): 81** (ohne optionale Features)

---

### 2. Funktionen bleiben unverändert (~360)

Die v1.3.1 Proposal fügt **keine neuen Funktionen** hinzu, sondern fokussiert auf:
- **Strukturelle Features** (Namespaces, Types, Classes)
- **Syntax-Verbesserungen** (Pipeline Operator, Error Handling)
- **Vision-Befehle** (neue Keywords, keine Funktionen)

**Summe Funktionen v1.3.1: ~360** (unverändert)

---

## Gesamtübersicht

### v1.3.0 - Vollständiger Sprachumfang

```
Keywords:    72
Funktionen: 360
─────────────────
Gesamt:     432
```

### v1.3.1 - Vollständiger Sprachumfang ✅ FINAL

```
Keywords:    119  (+47, +65%)
Funktionen:  360  (+0, +0%)
─────────────────────────────
Gesamt:      479  (+47, +11%)
```

### v1.3.1 - Vollständiger Sprachumfang (Minimal, falls gewünscht)

```
Keywords:     81  (+9, +13%)
Funktionen:  360  (+0, +0%)
─────────────────────────────
Gesamt:      441  (+9, +2%)
```

**Entscheidung:** Die finale Version v1.3.1 verwendet **119 Keywords** durch Optimierung der Detection Types als Strings.

---

## Aufwandsabschätzung (Aktualisiert)

### Wire Protocol

**Aufwand:** 1-2 Wochen (unverändert)

**Grund:** 
- Wire Protocol überträgt Queries als **Text**
- Keywords werden **nicht** einzeln serialisiert
- Funktionen sind im **Function Registry**, nicht im Parser
- Keine Breaking Changes

### Server Parser

**Aufwand:** 8-12 Wochen (unverändert)

**Grund:**
- Parser muss **47 neue Keywords** erkennen (statt 55 durch Optimierung)
- **Neue Grammatik-Regeln** für OOP-Konstrukte
- **Type-Checker** für User-Defined Types
- **Code-Generator** für neue Konstrukte

**Phasenweise Umsetzung:**
- Phase 1 (Q1 2026): Namespace, UDFs, Basic Types, Pipeline → 4-6 Wochen
- Phase 2 (Q2 2026): Vision, Error Handling → 2-3 Wochen
- Phase 3 (Q3 2026): Full Types, Pattern Match, Classes → 4-6 Wochen
- Phase 4 (Q4 2026): Async/Await → 3-4 Wochen

### Client Libraries (9 Clients)

**Aufwand:** 4-6 Wochen gesamt (parallel)

**Pro Client:** ~0.5-1 Woche

**Änderungen:**
- **Query Builder** aktualisieren (neue Keywords)
- **Syntax Highlighting** erweitern
- **Autocomplete** aktualisieren
- **Dokumentation** anpassen

**Wichtig:** 
- **Funktionen bleiben gleich** → Keine Änderungen an Function Calls
- **Backward Compatible** → v1.3.0 Queries funktionieren weiterhin

---

## Vergleich mit anderen Datenbanken

### Sprachumfang-Vergleich

| Datenbank | Keywords | Funktionen | Gesamt | Multimodal |
|-----------|----------|------------|--------|------------|
| **ThemisDB v1.3.0** | 72 | ~360 | 432 | ✅ Vollständig |
| **ThemisDB v1.3.1** ✅ | **119** | ~360 | **479** | ✅ Erweitert |
| PostgreSQL | ~450 | ~300 | 750 | ❌ SQL only |
| MongoDB | ~80 | ~200 | 280 | ❌ Document only |
| Neo4j (Cypher) | ~120 | ~180 | 300 | ❌ Graph only |
| ArangoDB (AQL) | ~90 | ~250 | 340 | ⚠️ Teilweise |
| Elasticsearch (DSL) | ~200 | ~150 | 350 | ❌ Search only |

### Erkenntnisse

1. **ThemisDB v1.3.0** hat bereits einen **kompakten Sprachumfang** (432) mit vollständiger Multi-Model-Unterstützung

2. **ThemisDB v1.3.1** wächst moderat auf **479 (+11%)** ✅, bleibt aber **deutlich unter PostgreSQL** (750)

3. **Strategie erfolgreich:** Wenige Keywords (119) ✅, viele Funktionen (360) ermöglicht:
   - Kompakte Sprachdefinition
   - Einfache Erweiterbarkeit
   - Geringer Parser-Aufwand
   - Maximale Ausdruckskraft

---

## Optimierungspotenzial

### ✅ UMGESETZT: Detection Types als Strings

**Einsparung:** -8 Keywords

```aql
-- Statt Keywords
DETECT [objects, text, faces]

-- Als Strings (FINALE LÖSUNG)
DETECT ['objects', 'text', 'faces']
```

**✅ Finale Zahlen v1.3.1:**
- Keywords: **119** (statt 127)
- Gesamt: **479** (statt 487)
- Änderung: **+47 (+11%)**

### Alternative: Minimal-Version v1.3.1 (falls später gewünscht)

**Nur kritische Features:**
- Namespace, Import (2)
- Type, Function (2)
- Basic Types (5): String, Int, Float, Bool, Any
- Vision (ohne Detection-Keywords) (9)

**Neue Zahlen:**
- Keywords v1.3.1 minimal: 81
- Gesamt v1.3.1 minimal: 441
- Änderung: +9 (+2%)

### Option 3: Phasenweise Einführung

**Phase 1 (v1.3.1):** Nur essenzielle Features
- Keywords: 81
- Gesamt: 441
- Aufwand: 7-10 Wochen

**Phase 2-4 (v1.3.2-v1.5.0):** Schrittweise Erweiterung
- Pro Phase: +15-20 Keywords
- Gesamt Endstand: 487
- Verteilter Aufwand über 4 Quartale

---

## Zusammenfassung für Entscheidungsfindung

### Zahlen auf einen Blick ✅ FINAL

| Metrik | v1.3.0 | v1.3.1 (Final) | v1.3.1 (Minimal) | PostgreSQL |
|--------|--------|----------------|------------------|------------|
| **Keywords** | 72 | **119 (+65%)** ✅ | 81 (+13%) | ~450 |
| **Funktionen** | 360 | 360 (+0%) | 360 (+0%) | ~300 |
| **Gesamt** | **432** | **479 (+11%)** ✅ | **441 (+2%)** | **750** |
| **Aufwand** | - | 13-20 Wochen | 7-10 Wochen | - |

**Entscheidung:** v1.3.1 Final mit **119 Keywords** und **479 Gesamt-Sprachumfang** (+11%)

### Empfehlung

**Phasenweise Einführung (v1.3.1 minimal → v1.5.0 full):**

1. **v1.3.1 (Q1 2026):** 81 Keywords → 441 Gesamt (+2%)
   - Namespace, UDFs, Basic Types, Pipeline
   - Aufwand: 7-10 Wochen

2. **v1.3.2 (Q2 2026):** +20 Keywords
   - Vision Commands, Error Handling
   - Aufwand: 2-3 Wochen

3. **v1.4.0 (Q3 2026):** +15 Keywords
   - Full Types, Pattern Matching
   - Aufwand: 4-6 Wochen

4. **v1.5.0 (Q4 2026):** +11 Keywords
   - Async/Await, Classes (optional)
   - Aufwand: 3-4 Wochen

**Vorteile:**
- ✅ Geringer initialer Aufwand (7-10 Wochen)
- ✅ Kontinuierliches Feedback einarbeiten
- ✅ Risiko verteilen
- ✅ Team-Kapazität schonen
- ✅ Jede Phase bringt Mehrwert

---

## Anhang: Alphabetische Funktionsliste (360 Funktionen)

### A-D (85 Funktionen)
```
ABS, ACOS, ACTIVITY_FREQUENCY, AGE, ALPHA_MINER, ANNOY_SEARCH,
APPEND, ASIN, ASSERT, ATAN, ATAN2, ATTRIBUTES, AVERAGE, AVERAGEIF,
AVG_OVER, BASE64_DECODE, BASE64_ENCODE, BETWEENNESS_CENTRALITY,
BPMN_EXTRACT, CASE_DURATION, CHEBYSHEV_DISTANCE, CLIQUE_NUMBER,
CLOSENESS_CENTRALITY, COALESCE, COLLECTION_COUNT, CONCAT,
CONCAT_SEPARATOR, CONNECTED_COMPONENTS, CONTAINS, CONTAINS_ARRAY,
COSH, COSINE, COSINE_SIMILARITY, COUNT_DISTINCT, COUNT_OVER,
CUME_DIST, CURRENT_DATE, CURRENT_TIME, CURRENT_TIMESTAMP,
DATE_ADD, DATE_BETWEEN, DATE_COMPARE, DATE_DAY, DATE_DAYOFWEEK,
DATE_DAYOFYEAR, DATE_DAYS_IN_MONTH, DATE_DIFF, DATE_END_OF_MONTH,
DATE_FORMAT, DATE_HOUR, DATE_ISO8601, DATE_LEAPYEAR, DATE_MILLISECOND,
DATE_MINUTE, DATE_MONTH, DATE_NOW, DATE_PARSE, DATE_QUARTER,
DATE_SECOND, DATE_START_OF_WEEK, DATE_SUBTRACT, DATE_TIMESTAMP,
DATE_TRUNC, DATE_WEEK, DATE_YEAR, DAYS, DECRYPT_AES, DECRYPT_RSA,
DENSE_RANK, DIFFERENCE, DOT_PRODUCT, DOTTED_CHART
```

### E-I (70 Funktionen)
```
E, EDIT_DISTANCE, EIGENVECTOR_CENTRALITY, ENCRYPT_AES, ENCRYPT_RSA,
ENDS_WITH, EPOCH_SECONDS, EUCLIDEAN_DISTANCE, EXACT, EXP, FACT,
FILE_EXT, FILENAME, FILENAME_WITHOUT_EXT, FILTER, FIND, FIND_FIRST,
FIND_LAST, FIRST, FIRST_VALUE, FLATTEN, FLOOR, FORMAT_FILESIZE,
FROM_UNIXTIME, FV, GEO_AREA, GEO_CONTAINS, GEO_DISTANCE, GEO_EQUALS,
GEO_IN_RANGE, GEO_INTERSECTS, GEO_LINESTRING, GEO_MULTILINESTRING,
GEO_MULTIPOINT, GEO_MULTIPOLYGON, GEO_POINT, GEO_POLYGON,
GETDATE, GRAPH_DIAMETER, GRAPH_DISTANCE, GRAPH_ECCENTRICITY,
GRAPH_RADIUS, HAMMING_DISTANCE, HAS, HASH_MD5, HASH_SHA1,
HASH_SHA256, HASH_SHA512, HELMERT_TRANSFORM, HEURISTIC_MINER,
HLOOKUP, HMAC, HOLIDAYS, HOLIDAYS_BETWEEN, HNSW_SEARCH, HOURS,
IF, IFNULL, INDUCTIVE_MINER, INDEX, INTERSECTION, INTERVAL,
IS_ARRAY, IS_AUDIO, IS_BOOL, IS_DATESTRING, IS_DOCUMENT, IS_IMAGE,
IS_KEY, IS_NULL, IS_NUMBER, IS_OBJECT, IS_SAME_COLLECTION,
IS_STRING, IS_VIDEO, IS_WEEKEND, IS_WORKDAY, ISBLANK, ISERROR,
ISLOGICAL, ISNUMBER, ISTEXT
```

### J-P (80 Funktionen)
```
JACCARD, JACCARD_SIMILARITY, JOIN, K_SHORTEST_PATHS, KEEP, KEYS,
KNN_SEARCH, L1_DISTANCE, L2_DISTANCE, LAG, LARGE, LAST, LAST_VALUE,
LEAD, LENGTH, LEVENSHTEIN, LISTAGG, LIST_CALENDARS, LN, LOG, LOG10,
LOWER, LTRIM, MAKE_DATE, MAKE_DATETIME, MAKE_TIME, MANHATTAN_DISTANCE,
MAP, MATCH, MAX, MAX_OVER, MEDIAN, MEDIAN_OVER, MERGE, MIME_TYPE,
MIN, MIN_OVER, MINUTES, MOD, MODE, MONTHS, N, NEIGHBORS, NOT_NULL,
NTH, NTH_PERCENTILE, NTH_VALUE, NTILE, NULLIF, NOW,
OUTERSECTION, PAGERANK, PARSE_FILESIZE, PARSE_IDENTIFIER, PARSE_JSON,
PATH_ABSOLUTE, PATH_BASENAME, PATH_DIRNAME, PATH_EXTENSION,
PATH_IS_ABSOLUTE, PATH_IS_RELATIVE, PATH_JOIN, PATH_NORMALIZE,
PATH_PARENT, PATH_RELATIVE, PATH_SPLIT, PERCENT_RANK, PERCENTILE_CONT,
PERCENTILE_DISC, PETRI_NET, PI, PIVOT, PMT, POP, POSITION, POW,
PREPEND, PROCESS_BOTTLENECKS, PROCESS_CONFORMANCE, PROCESS_DISCOVER,
PROCESS_PERFORMANCE, PROCESS_VARIANTS, PRODUCT, PROPER, PUSH,
PUSH_UNIQUE, PV
```

### Q-Z (125 Funktionen)
```
QUOTIENT, RAND, RAND_INT, RANDOM_TOKEN, RANGE, RANK, RANK_WINDOW,
RATIO_TO_REPORT, REDUCE, REGEX_MATCHES, REGEX_REPLACE, REGEX_TEST,
REMOVE_NTH, REMOVE_VALUE, REMOVE_VALUES, REPT, REPLACE, REPLACE_NTH,
REVERSE, REVERSE_ARRAY, ROUND, ROW_NUMBER, RTRIM, SANITIZE_FILENAME,
SCHEMA_GET, SCHEMA_VALIDATE, SECONDS, SHIFT, SHIFT_LEFT, SHIFT_RIGHT,
SHORTEST_PATH, SIGN, SIN, SINH, SLICE, SMALL, SOCIAL_NETWORK, SORT_ARRAY,
SOUNDEX, SPLIT, SQRT, ST_Area, ST_Buffer, ST_Centroid, ST_Contains,
ST_ConvexHull, ST_Distance, ST_Envelope, ST_Equals, ST_Intersects,
ST_Length, ST_Point, ST_SRID, ST_SetSRID, ST_Simplify, ST_TRANSFORM,
ST_Union, ST_Within, STARTS_WITH, STDDEV_OVER, STDDEV_POPULATION,
SUBSTITUTE, SUBSTRING, SUM_OVER, SUMPRODUCT, SYSDATE, TAN, TANH,
TEXT, TO_ARRAY, TO_BOOL, TO_JSON, TO_NUMBER, TO_OBJECT, TO_STRING,
TODAY, TOMORROW, TRANSLATE, TRIANGLE_COUNT, TRIM, TRUNC, TYPE,
UNION, UNIQUE, UNIX_TIMESTAMP, UNPIVOT, UNSET, UNSHIFT, UNZIP,
UPPER, URL_DECODE, URL_ENCODE, UUID, VALUE, VALUES, VARIANCE_OVER,
VARIANCE_POPULATION, VECTOR_ADD, VECTOR_CROSS, VECTOR_DIVIDE,
VECTOR_DOT, VECTOR_MULTIPLY, VECTOR_NORM, VECTOR_NORMALIZE,
VECTOR_SUBTRACT, VLOOKUP, WARN, WEEKS, WORKDAYS, WORKDAYS_ADD,
YEARS, YESTERDAY, ZIP
```

---

## Kontakt & Feedback

- **Vollständige Analyse:** `/docs/de/aql/AQL_COMPLETE_LANGUAGE_SCOPE.md`
- **Nur Keywords:** `/docs/de/aql/AQL_RESERVED_WORDS_ANALYSIS.md`
- **Excel Functions:** `/docs/de/aql/AQL_EXCEL_FUNCTIONS_STATUS.md`
- **Time Functions:** `/docs/de/aql/AQL_TIME_FUNCTIONS_OVERVIEW.md`
- **OOP Proposal:** `/docs/de/aql/AQL_OOP_EXTENSION_PROPOSAL.md`
