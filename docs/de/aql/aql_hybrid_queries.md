# 🔎 AQL Hybrid Queries Guide (Phase 2 + 2.5)

**Category:** 🔎 Advanced Queries  
**Version:** v1.3.0  
**Status:** ✅ Production Ready  
**Datum:** 22. Dezember 2025

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features & Highlights](#-features--highlights)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
  - [Vector+Geo (SIMILARITY)](#vectorgeo-similarity)
  - [Content+Geo (PROXIMITY)](#contentgeo-proximity)
  - [Graph+Geo (SHORTEST_PATH)](#graphgeo-shortest_path)
  - [Performance & Kostenmodell](#performance--kostenmodell)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht
Dieses Dokument beschreibt die **Hybrid Query Syntax** für ThemisDB AQL, die mehrere Datenmodelle in einer Query kombiniert.

---

## ✨ Features & Highlights

### 🎯 Unterstützte Hybrid-Typen

- **`SIMILARITY(field, [vector], k?)`** für Vector+Geo Ranking
- **`PROXIMITY(geoField, [lon, lat])`** für Content+Geo Distanz-basiertes Re-Ranking (mit `FULLTEXT` Filter)
- **`SHORTEST_PATH TO "vertexKey"`** für kürzeste Pfad-Abfragen in Graphen mit optionalen Spatial Constraints
- **LET-Unterstützung** für SIMILARITY/PROXIMITY (Phase 2.5)

### 🚀 Kern-Features

- **Kostenbasierte Optimierung:** Automatische Wahl zwischen Spatial-first vs Vector-first
- **Index-Prefilter:** Equality/Range/Composite-Indizes für hohe Selektivität
- **HNSW Integration:** Effiziente k-NN-Suche mit räumlichen Constraints
- **BM25 Fulltext:** Volltext-Suche kombiniert mit Geo-Proximity
- **Observability:** Tracer-Attribute für Plan-Analyse

---

## 🚀 Schnellstart

### Vector+Geo (Direktes Sorting)

```aql
FOR doc IN hotels
  FILTER ST_Within(doc.location, [13.4,52.5,13.6,52.7])
  SORT SIMILARITY(doc.embedding, [0.12,0.08,0.33], 10) DESC
  LIMIT 10
  RETURN doc
```

### Content+Geo (Fulltext + Nähe)

```aql
FOR doc IN places
  FILTER FULLTEXT(doc.description, "coffee", 200)
  FILTER ST_Within(doc.location, [13.4,52.5,13.6,52.7])
  SORT PROXIMITY(doc.location, [13.5,52.55]) ASC
  LIMIT 20
  RETURN doc
```

### Graph+Geo Shortest Path

```aql
FOR v, e, p IN 1..6 OUTBOUND "city:berlin" edges
  FILTER ST_Within(v.location, @boundary)
  SHORTEST_PATH TO "city:dresden"
  RETURN p
```

---

## 📖 Detaillierte Dokumentation

### Vector+Geo (SIMILARITY)

#### Beispiele
```aql
FOR doc IN hotels
  FILTER ST_Within(doc.location, [13.4,52.5,13.6,52.7])
  SORT SIMILARITY(doc.embedding, [0.12,0.08,0.33], 10) DESC
  LIMIT 10
  RETURN doc
```

### Vector+Geo mit Equality + Range Prädikaten (Index Prefilter)
```aql
FOR doc IN hotels
  FILTER ST_Within(doc.location, [13.4,52.5,13.6,52.7])
  FILTER doc.city == "Berlin" AND doc.stars >= 4 AND doc.stars <= 5
  SORT SIMILARITY(doc.embedding, [0.12,0.08,0.33], 10) DESC
  RETURN doc
```
Intern: Gleichheits- und Range-Prädikate erzeugen einen PK-Whitelist Intersect über Sekundär- & Range-Indizes.

### Vector+Geo mit Composite Index (Mehrfach-Gleichheit)
```aql
FOR doc IN hotels
  FILTER ST_Within(doc.location, [13.4,52.5,13.6,52.7])
  FILTER doc.city == "Berlin" AND doc.category == "luxury"
  SORT SIMILARITY(doc.embedding, [0.1,0.2,0.3], 10) DESC
  RETURN doc
```
Voraussetzung: Composite Index über `(city, category)` erstellt.
Intern: `scanKeysEqualComposite()` liefert PK-Intersect, Kostenmodell bevorzugt Vector-first bei hoher Selektivität.

### Vector+Geo mit LET
```aql
FOR doc IN hotels
  LET sim = SIMILARITY(doc.embedding, [0.1,0.2,0.3], 5)
  SORT sim DESC
  RETURN { doc, similarity: sim }
```

### Content+Geo (Fulltext + Nähe)
```aql
FOR doc IN places
  FILTER FULLTEXT(doc.description, "coffee", 200)
  FILTER ST_Within(doc.location, [13.4,52.5,13.6,52.7])
  SORT PROXIMITY(doc.location, [13.5,52.55]) ASC
  LIMIT 20
  RETURN doc
```

### Content+Geo mit LET
```aql
FOR doc IN places
  FILTER FULLTEXT(doc.description, "coffee", 50)
  LET prox = PROXIMITY(doc.location, [13.5,52.55])
  SORT prox ASC
  RETURN { doc, dist: prox }
```

### Graph + Geo Shortest Path
```aql
FOR v, e, p IN 1..6 OUTBOUND "city:berlin" edges
  FILTER ST_Within(v.location, @boundary)
  SHORTEST_PATH TO "city:dresden"
  RETURN p
```

## Performance Hinweise
- Verwende räumliche Bounding-Box oder Polygon Filter früh für hohe Selektivität.
- Bei stark selektiven Equality/Range-Prädikaten wird Vector-first bevorzugt (Kostenmodell).
- `overfetch` (Konfiguration) steuert Qualität vs Kosten im Vector-first Plan.

### Kostenmodell-getriebene Planwahl
- **Vector+Geo**: Wählt zwischen Spatial-first (R-Tree Filter, dann ANN) und Vector-first (ANN mit overfetch, dann Spatial) basierend auf `bboxRatio`, Prefilter-Größe und Index-Verfügbarkeit.
- **Content+Geo**: Wählt zwischen Fulltext-first (BM25, dann Spatial) und Spatial-first (R-Tree, dann naive Token-Match) basierend auf `bboxRatio` und geschätzten Fulltext-Treffern.
- **Graph+Geo**: Dynamische Branching-Faktor-Schätzung über Sampling; Frühabbruch bei geschätzter Expansion >1M Vertices.

### Tracer-Attribute für Observability
- `optimizer.plan`: gewählter Ausführungsplan (z.B. `vector_then_spatial`)
- `optimizer.cost_spatial_first`, `optimizer.cost_vector_first`: Kostenschätzungen
- `optimizer.cg.plan`: Content+Geo Plan (`fulltext_then_spatial` | `spatial_then_fulltext`)
- `optimizer.graph.branching_estimate`: geschätzter Branching-Faktor bei Graph-Queries
- `index_prefilter_size`: Anzahl Kandidaten nach Equality/Range/Composite Prefilter
- `composite_prefilter_applied`: true wenn Composite Index genutzt wurde

## Indizes
- Gleichheit: `createIndex(table, column)`
- Range: `createRangeIndex(table, column)` für numerische / lexikographische Bereiche.
- Composite: `createCompositeIndex(table, [col1, col2, ...])` für mehrfach-Gleichheit (AND-verknüpft).
- Fulltext: `createFulltextIndex(table, column)` für PROXIMITY.
- Spatial: R-Tree via `createSpatialIndex(table, geometryColumn)` (Vorarbeit Phase 1.5).
- Vector: HNSW via `VectorIndexManager::load(table.field, dim)` oder Batch-Build.

## Rückgabe & Variablen
- Derzeit werden SIMILARITY/PROXIMITY Distanzwerte nicht automatisch als Feld injiziert; Bei LET Syntax kannst du sie im RETURN explizit nutzen.
- Standard-Dispatch JSON (`executeAql`) enthält für Vector+Geo `distance` und für Content+Geo `bm25` sowie optional `geo_distance`.

## Fehlermeldungen
- Falsche Argumentanzahl führt zu klarer Translator-Error.
- Fehlende FULLTEXT bei PROXIMITY -> Fehler.
- K soll Integer Literal sein (kein Parameter-Array in Phase 2.5 für k).

## Zukunft (Roadmap)
- ✅ Composite Index Prefiltering (mehrspaltig) – Phase 2.5 abgeschlossen
- Distanz-Metriken für PROXIMITY in Metern (aktuell einfache euklidische Projektion).
- LET Rückgabe von numerischen Similarity/Proximity Werten in generischen Ausdrücken (Aggregation).
- Erweiterter Cost Estimator mit Statistikprofilen (Histogramme, kumulative Verteilungen).
- Adaptive Overfetch-Steuerung basierend auf Trefferqualität.
- Konfigurierbare Kostenmodell-Parameter (`config:hybrid_query`).

## Troubleshooting
- Leere Ergebnisliste trotz vorhandener Dokumente: Prüfe Indexexistenz & Datentypen (String vs Zahl) in Prädikaten.
- Langsame Query: Reduziere `overfetch` oder erhöhe Selektivität durch zusätzliche Gleichheitsprädikate.
- Unterschiedliche Sortierung vs Erwartung: Prüfe Vektordimension; Mixed Dimensions werden ignoriert.

## Beispiel-End-to-End (Vector+Geo Setup)
```cpp
// Index Setup
sec.createIndex("hotels", "city");
sec.createRangeIndex("hotels", "stars");
spatial.createSpatialIndex("hotels", "location");
vectorIndex.load("hotels.embedding", /*dim=*/384);

// Query
std::string q = R"(
FOR doc IN hotels
  FILTER ST_Within(doc.location, [13.4,52.5,13.6,52.7])
  FILTER doc.city == "Berlin" AND doc.stars >= 4
  SORT SIMILARITY(doc.embedding, [ /* 384 floats */ ], 10 ) DESC
  LIMIT 10
  RETURN doc
)";
auto [st, json] = executeAql(q, engine);
```

---

## 💡 Best Practices

### ✅ DO: Räumliche Filter früh anwenden

```aql
-- ✅ GUT: Bounding-Box Filter reduziert Kandidaten
FOR doc IN hotels
  FILTER ST_Within(doc.location, [13.4,52.5,13.6,52.7])
  SORT SIMILARITY(doc.embedding, @vec, 10) DESC
  RETURN doc
```

### ✅ DO: Equality-Prädikate für hohe Selektivität

```aql
-- ✅ GUT: city-Index reduziert Kandidaten massiv
FOR doc IN hotels
  FILTER doc.city == "Berlin"
  FILTER ST_Within(doc.location, @bbox)
  SORT SIMILARITY(doc.embedding, @vec, 10) DESC
  RETURN doc
```

### ⚠️ VORSICHT: Zu große Bounding-Box

```aql
-- ⚠️ SUBOPTIMAL: Große Bbox → viele Kandidaten
FOR doc IN hotels
  FILTER ST_Within(doc.location, [0,0,180,90])  -- Halber Planet!
  SORT SIMILARITY(doc.embedding, @vec, 100) DESC
  RETURN doc
```

---

## 🔧 Troubleshooting

### Query liefert keine Ergebnisse

**Problem:** Vector+Geo Query gibt leere Menge zurück

**Lösung:**
1. Teste Spatial-Filter separat: `FOR doc IN hotels FILTER ST_Within(...) RETURN COUNT(doc)`
2. Prüfe Vector-Dimensionen: Müssen exakt zur Index-Dimension passen
3. Erhöhe k-Parameter in SIMILARITY: `SIMILARITY(field, vec, 50)` statt `10`

### Unerwartete Sortierung

**Problem:** Ergebnisse haben nicht die erwartete Reihenfolge

**Lösung:**
- Bei Vector+Geo: Sortierung ist nach Vector-Distance (L2/Cosine)
- Bei Content+Geo: Sortierung ist nach BM25-Score oder Geo-Distanz
- Nutze `explain: true` um zu sehen welcher Plan gewählt wurde

### Performance-Probleme

**Problem:** Query dauert > 1 Sekunde

**Lösung:**
1. Prüfe `optimizer.cost_spatial_first` vs `optimizer.cost_vector_first` in Metrics
2. Erstelle fehlende Indizes (Spatial, Vector, Secondary)
3. Reduziere Bounding-Box oder erhöhe Selektivität durch zusätzliche Filter
4. Bei Composite-Indizes: Stelle sicher dass alle Filter-Spalten im Index sind

---

## 📚 Siehe auch

### 📘 Kern-Dokumentation

- [AQL Syntax](aql_syntax.md) - SIMILARITY() und PROXIMITY() Syntax
- [Query Engine](aql_query_engine.md) - Hybrid Query Execution
- [Query Optimizer](aql_query_engine.md#query-optimizer) - Kostenbasierte Planwahl

### 🔎 Erweiterte Features

- [Hybrid Queries Phase 1.5](aql_hybrid_queries_phase15.md) - Implementierungsdetails
- [Vector Index](../features/vector_index.md) - HNSW-Index Details
- [Spatial Index](../features/spatial_index.md) - R-Tree Details
- [Fulltext API](../search/fulltext_api.md) - BM25-Index Konfiguration

### ⚙️ Performance

- [EXPLAIN & PROFILE](aql_explain_profile.md) - Query-Analyse
- [Benchmarks](../../benchmarks/ADVANCED_BENCHMARKS_GUIDE.md) - Performance-Messungen

---

## 📝 Changelog

### v1.3.0 - 22. Dezember 2025
- ✅ **Template-Update:** Standardisierung auf v1.3.0 Dokumentationsformat
- ✅ **Struktur:** 8-Abschnitte-Format mit Emojis und TOC
- ✅ **Navigation:** Verbesserte interne Verlinkungen

### Phase 2.5 - 5. Dezember 2025
- LET-Unterstützung für SIMILARITY() und PROXIMITY()
- Erweiterte Beispiele mit LET-Bindings

### Phase 2 - 17. November 2025
- SIMILARITY() Syntax Sugar
- PROXIMITY() Syntax Sugar
- SHORTEST_PATH TO für Graph+Geo

### Phase 1 - Initial Release
- Vector+Geo Hybrid Queries
- Content+Geo Hybrid Queries
- Kostenmodell-getriebene Planwahl
