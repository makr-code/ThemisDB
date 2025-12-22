# AQL Implementierungs-Analyse - Antwort auf die Frage

**Datum:** 8. Dezember 2024  
**Frage:** "Im Dokument sind noch offene Phasen für die Implementierung von AQL Sprachumfang dokumentiert. Was ist im Sourcecode bereits vorhanden und was können wir tatsächlich noch implementieren?"

## Zusammenfassung

Die ursprüngliche Dokumentation `aql_language_scope.md` war **irreführend** - viele als "✅ vollständig implementiert" markierte Funktionen sind **tatsächlich NICHT im Sourcecode vorhanden**.

Diese Analyse basiert auf einer detaillierten Code-Überprüfung der folgenden Dateien:
- `/src/query/let_evaluator.cpp` (Funktionsauswertung)
- `/src/query/window_evaluator.cpp` (Window Functions)
- `/src/query/aql_translator.cpp` (Query-Übersetzung)
- `/include/query/aql_parser.h` (Parser-Definitionen)

## ✅ Was IST im Sourcecode vorhanden

### Voll funktionsfähig:

#### 1. Basis-String- und Mathematik-Funktionen
**Quelle:** `/src/query/let_evaluator.cpp` (Zeilen 363-494)
- `LENGTH()` - Länge von Array/String
- `CONCAT()` - String-Verkettung
- `SUBSTRING()` - Teilstring
- `UPPER()`, `LOWER()` - Groß-/Kleinschreibung
- `ABS()`, `CEIL()`, `FLOOR()`, `ROUND()` - Mathematik
- `MIN()`, `MAX()` - Min/Max in Array

#### 2. Geo/Spatial-Funktionen (umfangreich!)
**Quelle:** `/src/query/let_evaluator.cpp` (Zeilen 500-1200+)
- `ST_Point()`, `ST_Distance()`, `ST_Within()`, `ST_Contains()`
- `ST_Intersects()`, `ST_DWithin()`, `ST_Buffer()`, `ST_Union()`
- `ST_GeomFromText()`, `ST_GeomFromGeoJSON()`
- `ST_AsGeoJSON()`, `ST_AsText()`
- `ST_3DDistance()`, `ST_Z()`, `ST_ZMin()`, `ST_ZMax()`
- Zusätzlich: `ST_Force2D()`, `ST_HasZ()`, `ST_ZBetween()`

#### 3. Vektor-Funktionen (Basis)
**Quelle:** `/src/query/aql_translator.cpp` (Zeilen 84-188)
- `SIMILARITY(field, vector, k)` - Vektor-Ähnlichkeitssuche mit HNSW
- `PROXIMITY(field, point)` - Geo-Nähe-Suche

#### 4. Graph-Traversierung
**Quelle:** `/include/query/aql_parser.h` (Zeilen 456-492)
- `FOR v IN 1..n OUTBOUND start edges` - Ausgehende Traversierung
- `FOR v IN 1..n INBOUND start edges` - Eingehende Traversierung
- `FOR v IN 1..n ANY start edges` - Bidirektionale Traversierung
- `SHORTEST_PATH start TO end edges` - Kürzester Pfad

#### 5. Aggregation
**Quelle:** `/include/query/aql_parser.h` (Zeilen 383-409)
- `COLLECT x = expr` - Gruppierung
- `AGGREGATE COUNT()` - Zählen
- `AGGREGATE SUM(field)` - Summieren
- `AGGREGATE AVG(field)` - Durchschnitt

#### 6. Window Functions (vollständig!)
**Quelle:** `/src/query/window_evaluator.cpp`, `/include/query/window_evaluator.h`
- `ROW_NUMBER()` - Fortlaufende Nummerierung
- `RANK()`, `DENSE_RANK()` - Ranking mit/ohne Lücken
- `LAG(expr, offset)` - Zugriff auf vorherige Zeile
- `LEAD(expr, offset)` - Zugriff auf nächste Zeile
- `FIRST_VALUE(expr)` - Erster Wert im Fenster
- `LAST_VALUE(expr)` - Letzter Wert im Fenster

## ❌ Was NICHT im Sourcecode vorhanden ist (aber als ✅ dokumentiert war!)

### Phase 1 - Dokument-Funktionen (alle fehlen!)
- `DOCUMENT()` - Dokument per ID laden
- `MERGE()` - Objekte zusammenführen
- `UNSET()` - Felder entfernen
- `KEEP()` - Nur bestimmte Felder behalten
- `HAS()` - Feld-Existenz prüfen
- `ATTRIBUTES()` - Alle Feldnamen
- `VALUES()` - Alle Feldwerte

**Implementierungs-Aufwand:** NIEDRIG - JSON-Manipulation mit nlohmann/json

### Phase 1 - Array-Funktionen (alle fehlen!)
- `FLATTEN()` - Arrays flachen
- `UNIQUE()` - Duplikate entfernen
- `UNION()`, `INTERSECTION()`, `MINUS()` - Set-Operationen
- `FIRST()`, `LAST()`, `NTH()` - Array-Zugriff
- `SLICE()` - Teilarray
- `REVERSE()` - Umkehren
- `SORTED()`, `SORTED_UNIQUE()` - Sortieren
- `CONTAINS_ARRAY()` - Element-Test

**Implementierungs-Aufwand:** NIEDRIG - Standard STL-Algorithmen

### Phase 1 - Datum/Zeit-Funktionen (alle fehlen!)
- `DATE_NOW()`, `DATE_ISO8601()`, `DATE_TIMESTAMP()`
- `DATE_YEAR()`, `DATE_MONTH()`, `DATE_DAY()`
- `DATE_HOUR()`, `DATE_MINUTE()`, `DATE_SECOND()`
- `DATE_ADD()`, `DATE_SUBTRACT()`, `DATE_DIFF()`
- `DATE_TRUNC()`, `DATE_FORMAT()`, `DATE_COMPARE()`

**Implementierungs-Aufwand:** NIEDRIG - Standard C++ chrono oder date-Bibliothek

### Phase 2 - Text/Volltext-Funktionen (alle fehlen!)
- `FULLTEXT()` - Volltextsuche
- `TOKENS()` - Tokenisierung
- `PHRASE()` - Phrasensuche
- `LEVENSHTEIN_DISTANCE()` - Edit-Distanz
- `SOUNDEX()`, `METAPHONE()` - Phonetische Suche
- `NGRAM_MATCH()` - N-Gram Matching
- `REGEX_TEST()`, `REGEX_MATCHES()`, `REGEX_REPLACE()`
- `LIKE` mit Wildcards

**Implementierungs-Aufwand:** 
- REGEX: NIEDRIG - Standard C++ regex
- LEVENSHTEIN: NIEDRIG - Einfacher Algorithmus
- FULLTEXT: HOCH - Benötigt Text-Indexierung

### Phase 2 - Erweiterte Graph-Funktionen (alle fehlen!)
- `ALL_SHORTEST_PATHS()` - Alle kürzesten Pfade
- `K_SHORTEST_PATHS()` - K kürzeste Pfade
- `WEIGHTED_SHORTEST_PATH()` - Gewichteter Pfad
- `PATH_LENGTH()`, `PATH_VERTICES()`, `PATH_EDGES()`
- Graph-Algorithmen: `LOUVAIN_COMMUNITIES()`, `BETWEENNESS_CENTRALITY()`, `CLOSENESS_CENTRALITY()`

**Implementierungs-Aufwand:** HOCH - Benötigt Graph-Algorithmen-Bibliothek (z.B. Boost Graph)

### Phase 3 - Vektor/AI-Erweiterungen (alle fehlen!)
- `COSINE_SIMILARITY()`, `EUCLIDEAN_DISTANCE()` - Erweiterte Metriken
- `L2_NORMALIZE()` - Vektor-Normalisierung
- `HYBRID_SEARCH()` - Kombination Vektor + Text
- `EMBED()` - Text zu Vektor
- `CLASSIFY()` - Textklassifikation
- `EXTRACT_ENTITIES()` - Named Entity Recognition

**Implementierungs-Aufwand:** SEHR HOCH - Benötigt ML-Framework (TensorFlow, ONNX)

### Phase 3 - Erweiterte Geo-Funktionen (alle fehlen!)
- `GEO_DISTANCE()`, `GEO_AREA()`, `GEO_LENGTH()`
- `GEO_CENTROID()`, `GEO_SIMPLIFY()`
- `H3_TO_GEO()`, `GEO_TO_H3()` - H3 Hexagons
- `ISOCHRONE()` - Erreichbarkeitsanalyse

**Implementierungs-Aufwand:** MITTEL-HOCH - PostGIS oder H3-Bibliothek

### Phase 3 - JSON-Funktionen (alle fehlen!)
- `JSON_EXTRACT()`, `JSON_SET()`, `JSON_REMOVE()`
- `JSON_TYPE()`, `JSON_KEYS()`, `JSON_VALUES()`
- `JSON_ARRAY_LENGTH()`, `JSON_CONTAINS()`, `JSON_OVERLAPS()`

**Implementierungs-Aufwand:** NIEDRIG - nlohmann/json hat bereits viele Features

### Phase 3 - Statistische Funktionen (alle fehlen!)
- `MODE()`, `STDDEV()`, `VARIANCE()`, `IQR()`
- `CORRELATION()`, `LINEAR_REGRESSION()`
- `HISTOGRAM()`, `SAMPLE()`
- `RANDOM()`, `RANDOM_INT()`
- `MEDIAN()`, `PERCENTILE()`

**Implementierungs-Aufwand:** MITTEL - Numerische Bibliothek (Eigen, Boost Math)

### Syntax-Erweiterungen (keine implementiert!)
- `UPSERT` - Insert or Update
- `MERGE INTO` - SQL-Style Merge
- `EXISTS` / `NOT EXISTS` Subqueries
- `BEGIN TRANSACTION` / `COMMIT` / `ROLLBACK`

**Implementierungs-Aufwand:** VARIABEL (UPSERT: MITTEL, Transaktionen: SEHR HOCH)

## 💡 Was kann tatsächlich noch implementiert werden?

### Hohe Priorität (einfach & wichtig):

#### 1. Array-Funktionen
**Warum:** Basis-Funktionalität für jede Dokument-Datenbank  
**Aufwand:** ~2-3 Wochen  
**Implementierung:** In `let_evaluator.cpp` mit STL-Algorithmen

```cpp
// Beispiel: FLATTEN
if (funcName == "FLATTEN") {
    nlohmann::json result = nlohmann::json::array();
    for (const auto& item : args[0]) {
        if (item.is_array()) {
            for (const auto& sub : item) result.push_back(sub);
        } else {
            result.push_back(item);
        }
    }
    return result;
}
```

#### 2. Dokument-Funktionen
**Warum:** Essentiell für Dokument-Manipulation  
**Aufwand:** ~1-2 Wochen  
**Implementierung:** In `let_evaluator.cpp` mit nlohmann/json

```cpp
// Beispiel: MERGE
if (funcName == "MERGE") {
    nlohmann::json result = args[0];
    for (size_t i = 1; i < args.size(); i++) {
        result.merge_patch(args[i]);
    }
    return result;
}
```

#### 3. Datum/Zeit-Funktionen
**Warum:** Standard-Funktionalität  
**Aufwand:** ~2-3 Wochen  
**Implementierung:** In `let_evaluator.cpp` mit C++ chrono oder Howard Hinnant's date library

```cpp
// Beispiel: DATE_NOW
if (funcName == "DATE_NOW") {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    return ms;
}
```

### Mittlere Priorität:

#### 4. Regex-Funktionen
**Aufwand:** ~1 Woche  
**Implementierung:** C++ std::regex

#### 5. LEVENSHTEIN_DISTANCE
**Aufwand:** ~2-3 Tage  
**Implementierung:** Standard-Algorithmus

#### 6. Erweiterte Graph-Funktionen
**Aufwand:** ~4-6 Wochen  
**Implementierung:** Boost Graph Library

### Niedrige Priorität (komplex):

#### 7. FULLTEXT
**Aufwand:** ~8-12 Wochen  
**Benötigt:** Text-Indexierung, Tokenizer, Inverted Index

#### 8. AI/ML-Funktionen
**Aufwand:** ~12-16 Wochen  
**Benötigt:** ONNX Runtime oder TensorFlow Lite

#### 9. Transaktionale Kontrolle
**Aufwand:** ~16+ Wochen  
**Benötigt:** ACID-Implementierung, WAL, Lock-Manager

## 📊 Empfohlene Implementierungs-Reihenfolge

### Sprint 1-2 (1 Monat): Basis-Funktionen
1. Array-Funktionen (FLATTEN, UNIQUE, FIRST, LAST, NTH, SLICE, REVERSE, SORTED)
2. Dokument-Funktionen (MERGE, UNSET, KEEP, HAS, ATTRIBUTES, VALUES)
3. Basis-String-Funktionen (REGEX_TEST, REGEX_REPLACE)

**Impact:** Hoch - Schließt große Lücken in der Basis-Funktionalität

### Sprint 3-4 (1 Monat): Datum/Zeit
1. Alle DATE_* Funktionen
2. RANDOM(), RANDOM_INT()

**Impact:** Hoch - Standard-Funktionalität

### Sprint 5-6 (1 Monat): Erweiterte String-Funktionen
1. LEVENSHTEIN_DISTANCE()
2. LIKE mit Wildcards
3. SOUNDEX(), METAPHONE()

**Impact:** Mittel - Nützlich für Text-Matching

### Sprint 7+ (später): Komplexe Features
1. FULLTEXT (benötigt Text-Index)
2. Erweiterte Graph-Algorithmen
3. AI/ML-Funktionen
4. Transaktionale Kontrolle

**Impact:** Variabel - Nice-to-have, aber komplex

## ✅ Änderungen am Dokument

Das Dokument `aql_language_scope.md` wurde aktualisiert mit:

1. **Neuer Abschnitt am Anfang:** Klare Übersicht über tatsächlich implementierte Features
2. **Korrigierte Statusindikatoren:** ✅ → ❌ für nicht implementierte Funktionen
3. **Quellen-Verweise:** Verweis auf tatsächliche Sourcecode-Dateien
4. **Aktualisierte Roadmap:** Realistische Phasen basierend auf tatsächlichem Stand
5. **Korrigierte Kompatibilitäts-Matrix:** Vergleich alt vs. neu

## 🎯 Fazit

**Was ist vorhanden:**
- ✅ Solide Basis: FOR/FILTER/RETURN, Aggregation, Graph-Traversierung
- ✅ Exzellente Geo/Spatial-Unterstützung (besser als viele Konkurrenten!)
- ✅ Vollständige Window Functions (besser als ArangoDB!)
- ✅ Basis-Vektor-Suche funktioniert

**Was fehlt (aber als implementiert dokumentiert war):**
- ❌ Alle Array-Funktionen
- ❌ Alle Datum/Zeit-Funktionen
- ❌ Alle Dokument-Funktionen
- ❌ Alle Text/Volltext-Funktionen
- ❌ Erweiterte Graph-Algorithmen
- ❌ AI/ML-Features

**Was kann implementiert werden:**
- 🎯 Array-, Dokument- und Datum/Zeit-Funktionen: EINFACH (2-3 Monate)
- 🎯 Text-Funktionen (ohne FULLTEXT): MITTEL (1 Monat)
- 🎯 FULLTEXT, Graph-Algorithmen: KOMPLEX (3-6 Monate)
- 🎯 AI/ML-Features: SEHR KOMPLEX (6+ Monate)

**Empfehlung:** 
Fokus auf Phase 1 (Array, Dokument, Datum/Zeit) - diese sind essentiell, einfach zu implementieren und schließen die größten Lücken im Vergleich zu ArangoDB/MongoDB.
