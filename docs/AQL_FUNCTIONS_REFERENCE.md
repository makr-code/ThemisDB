# AQL Functions Reference - Complete Documentation

> **ThemisDB Query Language (AQL)** - Die einzige Abfragesprache, die Graph, Vector, Relational, Geo und File in einer einheitlichen Syntax vereint.

**Version:** 1.3  
**Stand:** Dezember 2024  
**Funktionen:** ~355  
**Kategorien:** 13

---

## Inhaltsverzeichnis

### Grundlagen
1. [Alleinstellungsmerkmale](#alleinstellungsmerkmale)
2. [Architektur-Übersicht](#architektur-übersicht)
3. [Vergleich mit anderen Datenbanken](#vergleich-mit-anderen-datenbanken)
4. [Funktionskategorien](#funktionskategorien)
5. [Syntax-Grundlagen](#syntax-grundlagen)

### Funktionsreferenz
6. [String-Funktionen](#string-funktionen) (~20 Funktionen)
7. [Math-Funktionen](#math-funktionen) (~30 Funktionen) ⭐ *Erweitert*
8. [Array-Funktionen](#array-funktionen) (~20 Funktionen)
9. [Date-Funktionen](#date-funktionen) (~45 Funktionen)
10. [Document-Funktionen](#document-funktionen) (~20 Funktionen)
11. [Collection-Funktionen](#collection-funktionen) (~25 Funktionen)
12. [Logical-Funktionen](#logical-funktionen) (~20 Funktionen) ⭐ *Neu*
13. [Geo-Funktionen](#geo-funktionen) (~25 Funktionen)
14. [CRS-Funktionen (Koordinatentransformation)](#crs-funktionen) (~10 Funktionen)
15. [Vector-Funktionen](#vector-funktionen) (~20 Funktionen)
16. [Graph-Funktionen](#graph-funktionen) (~15 Funktionen)
17. [Relational-Funktionen](#relational-funktionen) (~25 Funktionen)
18. [File-Funktionen](#file-funktionen) (~20 Funktionen)
19. [Security-Funktionen](#security-funktionen) (~15 Funktionen)
20. [Excel-kompatible Funktionen](#excel-kompatible-funktionen) (~30 Funktionen) ⭐ *Neu*

### Praxis & Referenz
21. [Praxisbeispiele nach Branche](#praxisbeispiele-nach-branche)
22. [Performance-Optimierung](#performance-optimierung)
23. [Benchmarks](#benchmarks) ⭐ *Neu*
24. [Fehlerbehandlung](#fehlerbehandlung)
25. [FAQ - Häufige Fragen](#faq)
26. [Migrations-Leitfäden](#migrations-leitfäden)
27. [Glossar](#glossar)

---

## Alleinstellungsmerkmale

### 🎯 Was macht ThemisDB einzigartig?

ThemisDB ist die erste und einzige Datenbank, die **echte Multi-Model-Queries** in einer einheitlichen Abfragesprache ermöglicht. Während andere Datenbanken einzelne Stärken haben, vereint ThemisDB alle Paradigmen nahtlos.

#### Feature-Matrix im Vergleich

| Feature | ThemisDB | Neo4j | PostgreSQL | MongoDB | Pinecone | ArangoDB |
|---------|----------|-------|------------|---------|----------|----------|
| **Unified Query Language** | ✅ Eine Syntax für alles | ❌ Cypher only | ❌ SQL only | ❌ MQL only | ❌ API only | ⚠️ AQL (limitiert) |
| **Native Graph + Vector** | ✅ Integriert | ❌ Plugin | ❌ Extension | ❌ Atlas Search | ✅ Vector only | ⚠️ Separat |
| **Geo + Graph kombiniert** | ✅ ST_* + SHORTEST_PATH | ❌ Separat | ✅ PostGIS | ✅ GeoJSON | ❌ | ⚠️ Basic |
| **BPMN Process Mining** | ✅ Native | ❌ | ❌ | ❌ | ❌ | ❌ |
| **CRS Transformation** | ✅ ETRS89/UTM/WGS84 | ❌ | ✅ PostGIS | ❌ | ❌ | ❌ |
| **Multi-Model in einer Query** | ✅ Vollständig | ❌ | ❌ | ❌ | ❌ | ⚠️ Teilweise |
| **Window Functions** | ✅ ROW_NUMBER, LAG, LEAD | ❌ | ✅ | ❌ | ❌ | ❌ |
| **File/MIME Operations** | ✅ Native | ❌ | ❌ | ❌ | ❌ | ❌ |
| **Helmert Transformation** | ✅ 7-Parameter | ❌ | ✅ PostGIS | ❌ | ❌ | ❌ |

#### Die 7 Killer-Features im Detail

##### 1. Multi-Model Queries in einer Zeile

**Das Problem:** In traditionellen Architekturen müssen Sie mehrere Systeme kombinieren:
- PostgreSQL für relationale Daten
- Neo4j für Graphen
- Pinecone/Weaviate für Vektoren
- Elasticsearch für Volltextsuche
- Redis für Caching

**Die ThemisDB-Lösung:**

```aql
-- Eine Query, die Graph, Vector, Geo und Relational kombiniert
FOR customer IN customers
  -- Geo: Kunden im Umkreis von 10 km
  FILTER GEO_DISTANCE(customer.location, @myLocation) < 10000
  
  -- Vector: Ähnliche Interessen (ML-Embedding)
  LET similarity = COSINE_SIMILARITY(customer.interests_embedding, @myInterests)
  FILTER similarity > 0.8
  
  -- Graph: Verbindungen über Empfehlungsnetzwerk
  FOR connection IN 1..3 OUTBOUND customer knows
    FILTER connection.active == true
    
    -- Relational: Aggregation und Window Functions
    LET orderStats = (
      FOR order IN orders
        FILTER order.customer_id == customer._key
        COLLECT AGGREGATE 
          total = SUM(order.amount),
          count = COUNT(1)
        RETURN { total, count }
    )
    
  RETURN { 
    customer, 
    similarity, 
    connection,
    orderStats,
    distance_km: GEO_DISTANCE(customer.location, @myLocation) / 1000
  }
```

**Vergleich: Das gleiche in anderen Systemen**

| System | Erforderliche Queries/Calls | Komplexität |
|--------|----------------------------|-------------|
| ThemisDB | 1 Query | ⭐ Einfach |
| PostgreSQL + PostGIS + pgvector | 3 Queries + Application Join | ⭐⭐⭐ Komplex |
| Neo4j + Pinecone | 2 Systeme + 2 API-Calls + Application Join | ⭐⭐⭐ Komplex |
| MongoDB Atlas | Aggregation Pipeline + Atlas Search + $graphLookup | ⭐⭐⭐⭐ Sehr komplex |

##### 2. Native Prozess-Mining (BPMN/EPK)

ThemisDB kann Prozesse aus Event-Logs **entdecken** und als BPMN-Diagramme exportieren.

```aql
-- Prozesse aus Audit-Logs entdecken
LET events = (
  FOR e IN audit_logs
    FILTER e.timestamp >= DATE_SUBTRACT(DATE_NOW(), 90, "days")
    SORT e.case_id, e.timestamp
    RETURN {
      case_id: e.case_id,
      activity: e.activity,
      timestamp: e.timestamp,
      resource: e.user_id
    }
)

LET process = DISCOVER_PROCESS(events, "case_id", "activity", "timestamp")

RETURN {
  -- Entdeckte Aktivitäten
  activities: process.activities,
  
  -- Übergänge zwischen Aktivitäten
  transitions: process.transitions,
  
  -- Prozessvarianten (unterschiedliche Pfade)
  variants: process.variants,
  
  -- Engpässe identifizieren
  bottlenecks: process.bottlenecks,
  
  -- BPMN-Export
  bpmn_xml: EXPORT_BPMN(process)
}
```

**Anwendungsfälle:**
- 🏥 Krankenhaus: Patientenpfade optimieren
- 🏭 Fertigung: Produktionsprozesse analysieren
- 🏦 Bank: Kreditanträge beschleunigen
- 📦 Logistik: Lieferketten visualisieren

##### 3. Vollständige Koordinatensystem-Transformation

ThemisDB transformiert zwischen **allen gängigen Koordinatensystemen** - inklusive der komplexen Helmert-7-Parameter-Transformation für historische Datensätze.

```aql
-- Beispiel: Katasterdaten aus verschiedenen Epochen harmonisieren

-- 1. Historische Gauß-Krüger Daten (DHDN, Bessel-Ellipsoid)
FOR parcel IN historic_parcels_gk
  LET wgs84 = ST_TRANSFORM(parcel.geometry, 31467, 4326)
  UPDATE parcel WITH { geometry_wgs84: wgs84 } IN historic_parcels_gk

-- 2. Aktuelle ETRS89/UTM Daten
FOR parcel IN modern_parcels_utm
  LET wgs84 = ST_TRANSFORM(parcel.geometry, 25832, 4326)
  UPDATE parcel WITH { geometry_wgs84: wgs84 } IN modern_parcels_utm

-- 3. Kombinierte Abfrage über alle Epochen
FOR parcel IN UNION(historic_parcels_gk, modern_parcels_utm)
  LET center = ST_CENTROID(parcel.geometry_wgs84)
  FILTER ST_CONTAINS(@searchArea, center)
  RETURN {
    id: parcel.id,
    original_crs: parcel.original_srid,
    area_sqm: ST_AREA(parcel.geometry),
    center: { lat: ST_Y(center), lon: ST_X(center) }
  }
```

**Unterstützte Transformationen:**

| Von | Nach | Methode |
|-----|------|---------|
| EPSG:31466-31469 (Gauß-Krüger) | EPSG:4326 (WGS84) | Helmert 7-Parameter |
| EPSG:25831-25833 (ETRS89/UTM) | EPSG:4326 (WGS84) | Transverse Mercator |
| EPSG:32631-32633 (WGS84/UTM) | EPSG:4326 (WGS84) | Transverse Mercator |
| EPSG:3857 (Web Mercator) | EPSG:4326 (WGS84) | Spherical Mercator |
| EPSG:4258 (ETRS89) | EPSG:4326 (WGS84) | Identity (praktisch gleich) |

##### 4. ML-Ready Vector Operations

ThemisDB speichert und durchsucht Embeddings von beliebigen ML-Modellen nativ.

```aql
-- OpenAI Embeddings (1536 Dimensionen)
INSERT { 
  text: "Künstliche Intelligenz revolutioniert die Medizin",
  embedding: [0.0123, -0.0456, 0.0789, ...],  -- 1536 Werte
  source: "research_paper",
  published: DATE_NOW()
} INTO documents

-- Cohere Embeddings (1024 Dimensionen)
INSERT {
  text: "AI transforms healthcare",
  embedding: [0.0234, 0.0567, -0.0890, ...],  -- 1024 Werte
  source: "news_article"
} INTO documents

-- Lokale Sentence-Transformers (384 Dimensionen)
INSERT {
  text: "Machine Learning im Gesundheitswesen",
  embedding: [0.1234, 0.5678, 0.9012, ...],  -- 384 Werte
  model: "all-MiniLM-L6-v2"
} INTO documents

-- Hybride Suche: Semantisch + Keyword + Geo + Zeit
FOR doc IN documents
  LET semantic_score = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
  LET keyword_match = CONTAINS(LOWER(doc.text), LOWER(@searchTerm))
  LET geo_score = doc.location ? 1 / (1 + GEO_DISTANCE(doc.location, @userLocation) / 10000) : 0
  LET recency_score = 1 / (1 + DATE_DIFF(doc.published, DATE_NOW(), "days") / 30)
  
  -- Kombinierter Relevanz-Score
  LET combined_score = (
    semantic_score * 0.5 +
    (keyword_match ? 0.2 : 0) +
    geo_score * 0.15 +
    recency_score * 0.15
  )
  
  FILTER semantic_score > 0.7 OR keyword_match
  SORT combined_score DESC
  LIMIT 20
  
  RETURN {
    doc,
    scores: { semantic: semantic_score, geo: geo_score, recency: recency_score },
    combined_score
  }
```

##### 5. Native Graph-Traversierung mit SQL-Komfort

ThemisDB kombiniert die Eleganz von Cypher mit der Vertrautheit von SQL.

```aql
-- Finde Influencer im Netzwerk mit mehreren Kriterien
FOR influencer IN users
  -- Graph: Follower-Netzwerk analysieren
  LET followers = (
    FOR f IN 1..1 INBOUND influencer follows
      RETURN f
  )
  LET follower_count = LENGTH(followers)
  
  -- Graph: Reichweite (2-Hop Netzwerk)
  LET reach = (
    FOR r IN 1..2 INBOUND influencer follows
      RETURN DISTINCT r
  )
  LET reach_count = LENGTH(reach)
  
  -- Zentralitätsmaße
  LET pagerank = PAGERANK(influencer, "follows", 0.85)
  LET clustering = CLUSTERING_COEFFICIENT(influencer, "follows")
  
  -- Geo: Durchschnittliche Entfernung der Follower
  LET avg_follower_distance = AVG(
    FOR f IN followers
      FILTER f.location != null
      RETURN GEO_DISTANCE(f.location, influencer.location)
  )
  
  -- Relational: Engagement-Statistiken
  LET engagement = (
    FOR post IN posts
      FILTER post.author_id == influencer._key
      FILTER DATE_DIFF(post.created, DATE_NOW(), "days") <= 30
      COLLECT AGGREGATE
        posts = COUNT(1),
        likes = SUM(post.likes),
        comments = SUM(post.comments),
        shares = SUM(post.shares)
      RETURN { posts, likes, comments, shares }
  )[0]
  
  FILTER follower_count >= 1000
  SORT pagerank DESC
  LIMIT 100
  
  RETURN {
    username: influencer.username,
    follower_count,
    reach_count,
    pagerank,
    clustering_coefficient: clustering,
    avg_follower_distance_km: avg_follower_distance / 1000,
    engagement,
    influence_score: pagerank * LOG(follower_count + 1) * (engagement.likes / (engagement.posts + 1))
  }
```

##### 6. SQL-kompatible Window Functions

ThemisDB unterstützt vollständige Window Functions wie PostgreSQL.

```aql
-- Umsatzanalyse mit Window Functions
FOR sale IN sales
  LET sale_date = DATE_TIMESTAMP(sale.created_at)
  
  -- Gruppierung nach Region und Monat
  COLLECT 
    region = sale.region,
    month = DATE_TRUNC(sale_date, "month")
  AGGREGATE
    revenue = SUM(sale.amount),
    orders = COUNT(1),
    avg_order = AVG(sale.amount)
  
  -- Window Functions
  LET prev_month_revenue = LAG(revenue, 1) OVER (PARTITION BY region ORDER BY month)
  LET next_month_revenue = LEAD(revenue, 1) OVER (PARTITION BY region ORDER BY month)
  LET running_total = RUNNING_SUM(revenue) OVER (PARTITION BY region ORDER BY month)
  LET rank_in_region = ROW_NUMBER() OVER (PARTITION BY region ORDER BY revenue DESC)
  LET percentile_rank = PERCENT_RANK() OVER (ORDER BY revenue)
  
  -- Berechnete Metriken
  LET mom_growth = prev_month_revenue ? ((revenue - prev_month_revenue) / prev_month_revenue * 100) : null
  LET yoy_revenue = LAG(revenue, 12) OVER (PARTITION BY region ORDER BY month)
  LET yoy_growth = yoy_revenue ? ((revenue - yoy_revenue) / yoy_revenue * 100) : null
  
  SORT region, month
  
  RETURN {
    region,
    month: DATE_FORMAT(month, "%Y-%m"),
    revenue,
    orders,
    avg_order: ROUND(avg_order, 2),
    prev_month_revenue,
    mom_growth: mom_growth ? CONCAT(ROUND(mom_growth, 1), "%") : "N/A",
    yoy_growth: yoy_growth ? CONCAT(ROUND(yoy_growth, 1), "%") : "N/A",
    running_total,
    rank_in_region,
    percentile: ROUND(percentile_rank * 100, 1)
  }
```

##### 7. Datei-Operationen im Query

ThemisDB kann mit Dateimetadaten direkt im Query arbeiten.

```aql
-- Datei-Repository analysieren
FOR file IN files
  LET path_parts = PATH_SPLIT(file.path)
  LET ext = FILE_EXT(file.name)
  LET mime = MIME_TYPE(file.name)
  LET size_human = FORMAT_FILESIZE(file.size)
  
  -- Kategorisierung
  LET category = (
    IS_IMAGE(file.name) ? "images" :
    IS_VIDEO(file.name) ? "videos" :
    IS_AUDIO(file.name) ? "audio" :
    IS_DOCUMENT(file.name) ? "documents" :
    ext IN ["zip", "tar", "gz", "7z"] ? "archives" :
    ext IN ["js", "py", "java", "cpp", "h"] ? "code" :
    "other"
  )
  
  -- Duplikat-Erkennung via Hash
  COLLECT 
    hash = file.content_hash
  INTO duplicates
  
  LET is_duplicate = LENGTH(duplicates) > 1
  
  FOR dup IN duplicates
    LET f = dup.file
    RETURN {
      path: f.path,
      name: f.name,
      extension: ext,
      mime_type: mime,
      size: f.size,
      size_human,
      category,
      is_duplicate,
      duplicate_count: LENGTH(duplicates),
      created: f.created_at,
      modified: f.modified_at
    }
```

---

## Architektur-Übersicht

### Wie AQL intern funktioniert

```
┌──────────────────────────────────────────────────────────────────────┐
│                           AQL Query                                   │
│  FOR doc IN collection FILTER ... LET x = FUNC(...) RETURN ...       │
└────────────────────────────────┬─────────────────────────────────────┘
                                 │
                                 ▼
┌──────────────────────────────────────────────────────────────────────┐
│                         Query Parser                                  │
│   - Lexikalische Analyse                                             │
│   - Syntaxbaum (AST) erstellen                                       │
│   - Semantische Validierung                                          │
└────────────────────────────────┬─────────────────────────────────────┘
                                 │
                                 ▼
┌──────────────────────────────────────────────────────────────────────┐
│                      Query Optimizer                                  │
│   - Index-Auswahl                                                    │
│   - Join-Reihenfolge                                                 │
│   - Filter-Pushdown                                                  │
│   - Subquery-Optimierung                                             │
└────────────────────────────────┬─────────────────────────────────────┘
                                 │
                                 ▼
┌──────────────────────────────────────────────────────────────────────┐
│                     Execution Engine                                  │
│  ┌──────────────────────────────────────────────────────────────┐   │
│  │                  FunctionRegistry                              │   │
│  │  ┌─────────┬─────────┬─────────┬─────────┬─────────────────┐ │   │
│  │  │ String  │  Math   │  Array  │  Date   │    Document     │ │   │
│  │  │ (~15)   │  (~25)  │  (~20)  │  (~15)  │     (~20)       │ │   │
│  │  ├─────────┼─────────┼─────────┼─────────┼─────────────────┤ │   │
│  │  │   Geo   │   CRS   │ Vector  │  Graph  │   Relational    │ │   │
│  │  │  (~25)  │  (~10)  │  (~20)  │  (~15)  │     (~25)       │ │   │
│  │  ├─────────┴─────────┴─────────┴─────────┴─────────────────┤ │   │
│  │  │                      File (~20)                          │ │   │
│  │  └──────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────┘   │
└────────────────────────────────┬─────────────────────────────────────┘
                                 │
                                 ▼
┌──────────────────────────────────────────────────────────────────────┐
│                      Storage Engines                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────────────┐ │
│  │ Document │  │   Graph  │  │  Vector  │  │    Geo (R-Tree)      │ │
│  │  Store   │  │   Index  │  │   Index  │  │                      │ │
│  └──────────┘  └──────────┘  └──────────┘  └──────────────────────┘ │
└──────────────────────────────────────────────────────────────────────┘
```

### OOP-basiertes Funktionssystem

Jede AQL-Funktion ist eine eigene Klasse mit definierter Schnittstelle:

```cpp
// Interface für alle Funktionen
class IFunction {
public:
    virtual ~IFunction() = default;
    
    // Funktionsname (z.B. "LENGTH", "ST_DISTANCE")
    virtual std::string getName() const = 0;
    
    // Beschreibung für Dokumentation
    virtual std::string getDescription() const = 0;
    
    // Erlaubte Signaturen
    virtual std::vector<FunctionSignature> getSignatures() const = 0;
    
    // Ausführung
    virtual JsonValue execute(
        const std::vector<JsonValue>& args,
        const FunctionContext& ctx
    ) const = 0;
};

// Beispiel: LENGTH-Funktion
class LengthFunction : public IFunction {
public:
    std::string getName() const override { return "LENGTH"; }
    
    std::string getDescription() const override {
        return "Returns length of string, array, or object";
    }
    
    std::vector<FunctionSignature> getSignatures() const override {
        return {
            { {ArgType::STRING}, ReturnType::NUMBER },
            { {ArgType::ARRAY}, ReturnType::NUMBER },
            { {ArgType::OBJECT}, ReturnType::NUMBER }
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, const FunctionContext& ctx) const override {
        if (args[0].isString()) return args[0].asString().length();
        if (args[0].isArray()) return args[0].asArray().size();
        if (args[0].isObject()) return args[0].asObject().size();
        throw FunctionError("LENGTH requires string, array, or object");
    }
};
```

**Vorteile dieses Designs:**
- ✅ Single Responsibility: Jede Funktion in eigener Klasse
- ✅ Open/Closed: Neue Funktionen ohne Änderung bestehenden Codes
- ✅ Testbarkeit: Jede Funktion isoliert testbar
- ✅ Dokumentation: Automatisch aus Metadaten generierbar
- ✅ Plugin-fähig: Externe Funktionen registrierbar

---

## Vergleich mit anderen Datenbanken

### Detaillierter Feature-Vergleich

#### ThemisDB vs. Neo4j (Cypher)

| Aufgabe | ThemisDB AQL | Neo4j Cypher | Anmerkung |
|---------|--------------|--------------|-----------|
| **Einfache Traversierung** | `FOR v IN 1..5 OUTBOUND start knows RETURN v` | `MATCH (start)-[:knows*1..5]->(v) RETURN v` | Ähnliche Syntax |
| **Mit Geo-Filter** | `FILTER GEO_DISTANCE(v.loc, @point) < 1000` | ❌ Nicht möglich ohne Plugin | ThemisDB: Native Integration |
| **Mit Vector-Similarity** | `LET sim = COSINE_SIMILARITY(v.emb, @vec)` | ❌ Braucht externes System | ThemisDB: Native Vektorsuche |
| **Shortest Path** | `SHORTEST_PATH(a, b, "knows")` | `shortestPath((a)-[:knows*]-(b))` | Beide nativ unterstützt |
| **Aggregation** | `COLLECT ... AGGREGATE SUM(), AVG()` | `WITH ... COLLECT` | ThemisDB: SQL-ähnlicher |
| **Window Functions** | `ROW_NUMBER() OVER (...)` | ❌ Nicht verfügbar | ThemisDB exklusiv |
| **Subqueries** | `LET x = (FOR ...)` | `CALL { ... }` | Beide unterstützt |
| **CRS Transformation** | `ST_TRANSFORM(geom, 25832, 4326)` | ❌ Nicht verfügbar | ThemisDB exklusiv |

**Migration von Cypher zu AQL:**

```cypher
// Neo4j Cypher
MATCH (p:Person)-[:KNOWS*1..3]->(friend:Person)
WHERE p.name = 'Alice'
AND friend.age > 30
RETURN friend.name, friend.age
ORDER BY friend.age DESC
LIMIT 10
```

```aql
// ThemisDB AQL
FOR p IN persons
  FILTER p.name == "Alice"
  FOR friend IN 1..3 OUTBOUND p knows
    FILTER friend.age > 30
    SORT friend.age DESC
    LIMIT 10
    RETURN { name: friend.name, age: friend.age }
```

#### ThemisDB vs. PostgreSQL

| Aufgabe | ThemisDB AQL | PostgreSQL | Anmerkung |
|---------|--------------|------------|-----------|
| **JSON-Dokumente** | Native | `jsonb` Typ | ThemisDB: Schema-frei |
| **Graph-Traversal** | `FOR v IN OUTBOUND` | `WITH RECURSIVE` (komplex!) | ThemisDB: Eleganter |
| **Vector-Search** | `COSINE_SIMILARITY()` | `pgvector` Extension | Beide gut |
| **Geo-Operationen** | `ST_*` Funktionen | PostGIS Extension | Beide OGC-kompatibel |
| **CRS Transformation** | `ST_TRANSFORM()` | PostGIS `ST_Transform()` | Beide vollständig |
| **Window Functions** | `ROW_NUMBER`, `LAG`, `LEAD` | Vollständig | Beide vollständig |
| **Alles kombiniert** | ✅ Eine Query | ❌ Mehrere Queries/CTEs | ThemisDB: Einfacher |

**Migration von SQL zu AQL:**

```sql
-- PostgreSQL
SELECT c.name, o.total, 
       ROW_NUMBER() OVER (ORDER BY o.total DESC) as rank
FROM customers c
JOIN (
  SELECT customer_id, SUM(amount) as total
  FROM orders
  GROUP BY customer_id
) o ON c.id = o.customer_id
WHERE o.total > 1000
ORDER BY o.total DESC;
```

```aql
-- ThemisDB AQL
FOR c IN customers
  LET orderTotal = SUM(
    FOR o IN orders
      FILTER o.customer_id == c._key
      RETURN o.amount
  )
  FILTER orderTotal > 1000
  LET rank = ROW_NUMBER() OVER (ORDER BY orderTotal DESC)
  SORT orderTotal DESC
  RETURN { name: c.name, total: orderTotal, rank }
```

#### ThemisDB vs. MongoDB

| Aufgabe | ThemisDB AQL | MongoDB | Anmerkung |
|---------|--------------|---------|-----------|
| **Syntax** | SQL-ähnlich, lesbar | JSON-basiert, verschachtelt | ThemisDB: Lesbarer |
| **Graph** | Native Traversal | `$graphLookup` (limitiert) | ThemisDB: Mächtiger |
| **Aggregation** | `COLLECT`, `AGGREGATE` | Pipeline Stages | Beide mächtig |
| **Joins** | `FOR ... FOR` | `$lookup` | ThemisDB: Flexibler |
| **Window Functions** | `ROW_NUMBER`, `LAG`, `LEAD` | ❌ Nicht verfügbar | ThemisDB exklusiv |
| **Vector Search** | Native | Atlas Search (Cloud) | ThemisDB: On-premise möglich |
| **Geo** | OGC ST_* Funktionen | GeoJSON-basiert | Beide gut |

**Migration von MongoDB Aggregation zu AQL:**

```javascript
// MongoDB Aggregation Pipeline
db.orders.aggregate([
  { $match: { status: "completed" } },
  { $group: { 
      _id: "$customer_id", 
      total: { $sum: "$amount" }, 
      count: { $sum: 1 } 
  }},
  { $lookup: {
      from: "customers",
      localField: "_id",
      foreignField: "_id",
      as: "customer"
  }},
  { $unwind: "$customer" },
  { $sort: { total: -1 } },
  { $limit: 10 },
  { $project: {
      customerName: "$customer.name",
      total: 1,
      count: 1
  }}
])
```

```aql
// ThemisDB AQL - Deutlich lesbarer
FOR o IN orders
  FILTER o.status == "completed"
  COLLECT customerId = o.customer_id
  AGGREGATE 
    total = SUM(o.amount),
    count = COUNT(1)
  LET customer = DOCUMENT("customers", customerId)
  SORT total DESC
  LIMIT 10
  RETURN {
    customerName: customer.name,
    total,
    count
  }
```

#### ThemisDB vs. ArangoDB

| Aufgabe | ThemisDB AQL | ArangoDB AQL | Anmerkung |
|---------|--------------|--------------|-----------|
| **Graph Traversal** | Identische Syntax | Identische Syntax | Kompatibel |
| **Vector Search** | Native | Nur über Views | ThemisDB: Einfacher |
| **CRS Transformation** | `ST_TRANSFORM()` mit Helmert | Nur WGS84 | ThemisDB: Vollständiger |
| **Process Mining** | Native BPMN | ❌ Nicht verfügbar | ThemisDB exklusiv |
| **Window Functions** | `ROW_NUMBER`, `LAG`, `LEAD` | ❌ Nicht verfügbar | ThemisDB exklusiv |
| **File Functions** | `MIME_TYPE`, `PATH_*` | ❌ Nicht verfügbar | ThemisDB exklusiv |

#### ThemisDB vs. Pinecone/Weaviate (Vector DBs)

| Aufgabe | ThemisDB | Pinecone/Weaviate | Anmerkung |
|---------|----------|-------------------|-----------|
| **Vector Search** | `COSINE_SIMILARITY()` | Native API | Beide schnell |
| **Metadata Filter** | AQL Filter-Syntax | Eigene Filter-Syntax | ThemisDB: Mächtiger |
| **Joins** | Native | ❌ Nicht möglich | ThemisDB exklusiv |
| **Geo-Filter** | `GEO_DISTANCE()` | Limitiert | ThemisDB: Vollständig |
| **Graph** | Native Traversal | ❌ Nicht möglich | ThemisDB exklusiv |
| **Aggregation** | `COLLECT AGGREGATE` | ❌ Nur Count | ThemisDB: Vollständig |

**Typische Pinecone-Abfrage in ThemisDB:**

```python
# Pinecone Python
results = index.query(
    vector=query_embedding,
    top_k=10,
    filter={"category": "electronics", "price": {"$lt": 100}}
)
```

```aql
-- ThemisDB AQL - Mit zusätzlichen Möglichkeiten
FOR doc IN products
  FILTER doc.category == "electronics"
  FILTER doc.price < 100
  LET similarity = COSINE_SIMILARITY(doc.embedding, @queryEmbedding)
  FILTER similarity > 0.7
  SORT similarity DESC
  LIMIT 10
  
  -- Zusätzlich: Ähnliche Produkte über Graph
  LET related = (
    FOR r IN 1..2 OUTBOUND doc similar_to
      RETURN r.name
  )
  
  -- Zusätzlich: Durchschnittliche Bewertung
  LET avgRating = AVG(
    FOR review IN reviews
      FILTER review.product_id == doc._key
      RETURN review.rating
  )
  
  RETURN { 
    doc, 
    similarity, 
    related: SLICE(related, 0, 5),
    avgRating
  }
```

---

## Funktionskategorien

ThemisDB bietet **~210 Funktionen** in **11 Kategorien**:

| Kategorie | Anzahl | Beschreibung | Haupt-Anwendungsfälle |
|-----------|--------|--------------|----------------------|
| **String** | ~15 | Textmanipulation, Pattern Matching, Fuzzy Search | Datenbereinigung, Suche, Validierung |
| **Math** | ~25 | Arithmetik, Trigonometrie, Statistik | Berechnungen, Analysen, ML-Features |
| **Array** | ~20 | Listen-Operationen, Set-Funktionen | Datenstruktur-Manipulation |
| **Date** | ~15 | Datum/Zeit-Verarbeitung, Formatierung | Zeitreihen, Berichte, Scheduling |
| **Document** | ~20 | Objektmanipulation, Typ-Prüfungen | Schema-Validierung, Transformation |
| **Geo** | ~25 | Räumliche Operationen (OGC-kompatibel) | GIS, Location-Services, Routing |
| **CRS** | ~10 | Koordinatentransformationen | Vermessung, Kataster, Kartografie |
| **Vector** | ~20 | ML-Embeddings, Ähnlichkeitssuche | Semantic Search, Recommendations |
| **Graph** | ~15 | Traversierung, Zentralität, Pfade | Social Networks, Fraud Detection |
| **Relational** | ~25 | SQL-Joins, Aggregation, Window | Business Analytics, Reporting |
| **File** | ~20 | Pfade, MIME-Typen, Dateigrößen | Document Management, Storage |

---

## Syntax-Grundlagen

### Grundstruktur einer AQL-Query

```aql
// Iteration über Collection
FOR variable IN collection
  
  // Filterung
  FILTER variable.field == "value"
  FILTER variable.number > 10
  
  // Berechnungen und Zwischenvariablen
  LET computed = FUNCTION(variable.field)
  LET subquery = (
    FOR sub IN other_collection
      FILTER sub.ref == variable._key
      RETURN sub
  )
  
  // Sortierung
  SORT variable.field ASC
  
  // Limitierung
  LIMIT 10
  
  // Rückgabe
  RETURN {
    id: variable._key,
    computed,
    subquery
  }
```

### Inline-Literale in RETURN (JSON-alike)

AQL unterstützt vollständig verschachtelte inline Object- und Array-Literale im RETURN:

```aql
// Einfaches Objekt-Literal
RETURN { name: "Alice", age: 30 }

// Verschachteltes Objekt mit Array
RETURN {
  user: {
    name: doc.name,
    contact: {
      email: doc.email,
      phone: doc.phone
    }
  },
  tags: ["active", "premium"],
  metadata: {
    created: NOW(),
    version: 1
  }
}

// Array von Objekten
RETURN [
  { type: "primary", value: doc.email },
  { type: "secondary", value: doc.phone }
]

// Dynamische Verschachtelung mit Funktionen
FOR user IN users
  LET orders = (FOR o IN orders FILTER o.user == user._key RETURN o)
  RETURN {
    profile: {
      id: user._key,
      name: CONCAT(user.firstName, " ", user.lastName),
      initials: CONCAT(SUBSTRING(user.firstName, 0, 1), SUBSTRING(user.lastName, 0, 1))
    },
    statistics: {
      orderCount: LENGTH(orders),
      totalSpent: SUM(orders[*].amount),
      avgOrder: AVG(orders[*].amount)
    },
    recentOrders: (
      FOR o IN orders
        SORT o.date DESC
        LIMIT 3
        RETURN { id: o._key, date: o.date, amount: o.amount }
    ),
    flags: [
      user.isActive ? "active" : "inactive",
      LENGTH(orders) > 10 ? "frequent" : "occasional"
    ]
  }
```

**Wichtig:** Die Syntax ist JSON-ähnlich, aber mit AQL-Erweiterungen:
- Schlüssel ohne Anführungszeichen: `{ name: "value" }` statt `{ "name": "value" }`
- Shorthand-Syntax: `{ name, age }` entspricht `{ name: name, age: age }`
- Ausdrücke als Werte: `{ total: SUM(values) }`
- Ternäre Operatoren: `{ status: isActive ? "on" : "off" }`

### Operatoren

| Kategorie | Operatoren | Beispiel |
|-----------|------------|----------|
| **Vergleich** | `==`, `!=`, `<`, `>`, `<=`, `>=` | `x == 5` |
| **Logisch** | `AND`, `OR`, `NOT`, `!` | `a > 5 AND b < 10` |
| **Arithmetisch** | `+`, `-`, `*`, `/`, `%` | `price * quantity` |
| **String** | `+` (Konkatenation) | `firstName + " " + lastName` |
| **Ternär** | `? :` | `age >= 18 ? "adult" : "minor"` |
| **In** | `IN`, `NOT IN` | `status IN ["active", "pending"]` |
| **Like** | `LIKE` | `name LIKE "A%"` |
| **Range** | `..` | `FOR i IN 1..10` |

### Datentypen

| Typ | Beispiel | AQL-Literal |
|-----|----------|-------------|
| **null** | Kein Wert | `null` |
| **Boolean** | Wahr/Falsch | `true`, `false` |
| **Number** | Ganz-/Fließkommazahl | `42`, `3.14`, `-17` |
| **String** | Text | `"Hello"`, `'World'` |
| **Array** | Liste | `[1, 2, 3]`, `["a", "b"]` |
| **Object** | Dokument | `{ key: "value", num: 42 }` |

### Variablen-Bindung

```aql
// LET für Zwischenvariablen
LET x = 5
LET greeting = CONCAT("Hello, ", @userName)
LET result = (FOR doc IN docs RETURN doc.value)

// @ für Parameter (Query-Bindung)
FOR user IN users
  FILTER user.age >= @minAge
  FILTER user.country == @country
  RETURN user
```

### Subqueries

```aql
// Korrelierte Subquery
FOR user IN users
  LET orders = (
    FOR order IN orders
      FILTER order.user_id == user._key
      SORT order.date DESC
      LIMIT 5
      RETURN order
  )
  RETURN { user, recentOrders: orders }

// Aggregierte Subquery
FOR user IN users
  LET totalSpent = SUM(
    FOR order IN orders
      FILTER order.user_id == user._key
      RETURN order.amount
  )
  RETURN { user, totalSpent }
```

---

## String-Funktionen

Die String-Funktionen bieten umfassende Textmanipulation von einfacher Verkettung bis zu Fuzzy-Matching mit Levenshtein-Distanz.

### Übersicht

| Funktion | Beschreibung | Beispiel-Rückgabe |
|----------|--------------|-------------------|
| `LENGTH(str)` | Länge eines Strings | `11` |
| `CONCAT(...)` | Strings verketten | `"Hello World"` |
| `SUBSTRING(str, start, len)` | Teilstring extrahieren | `"llo"` |
| `UPPER(str)` | Großschreibung | `"HELLO"` |
| `LOWER(str)` | Kleinschreibung | `"hello"` |
| `TRIM(str)` | Leerzeichen entfernen | `"text"` |
| `LTRIM(str)` | Links trimmen | `"text  "` |
| `RTRIM(str)` | Rechts trimmen | `"  text"` |
| `SPLIT(str, sep)` | String teilen | `["a", "b", "c"]` |
| `CONTAINS(str, search)` | Enthält Substring? | `true` / `false` |
| `STARTS_WITH(str, prefix)` | Beginnt mit? | `true` / `false` |
| `ENDS_WITH(str, suffix)` | Endet mit? | `true` / `false` |
| `REPLACE(str, old, new)` | Ersetzen | `"Hello World"` |
| `REGEX_TEST(str, pattern)` | Regex-Match? | `true` / `false` |
| `REGEX_REPLACE(str, pattern, replacement)` | Regex-Ersetzung | `"processed"` |
| `LEVENSHTEIN_DISTANCE(a, b)` | Edit-Distanz | `3` |

---

### LENGTH(value)

Gibt die Länge eines Strings, Arrays oder Objekts zurück.

**Signatur:**
```
LENGTH(value) → number
```

**Parameter:**

| Parameter | Typ | Beschreibung |
|-----------|-----|--------------|
| `value` | string \| array \| object | Der zu messende Wert |

**Rückgabewert:** `number` - Die Länge

**Verhalten nach Typ:**
- **String**: Anzahl der UTF-8 Zeichen (nicht Bytes!)
- **Array**: Anzahl der Elemente
- **Object**: Anzahl der Eigenschaften (Top-Level)

**Beispiele:**

```aql
-- String-Länge
RETURN LENGTH("Hello World")
// Ergebnis: 11

-- Unicode-Strings (Zeichen, nicht Bytes)
RETURN LENGTH("Hëllö Wörld")
// Ergebnis: 11

RETURN LENGTH("你好世界")
// Ergebnis: 4

-- Emoji-Strings
RETURN LENGTH("👋🌍")
// Ergebnis: 2

-- Array-Länge
RETURN LENGTH([1, 2, 3, 4, 5])
// Ergebnis: 5

RETURN LENGTH([])
// Ergebnis: 0

-- Objekt-Eigenschaften zählen
RETURN LENGTH({ name: "Max", age: 30 })
// Ergebnis: 2

-- Verschachtelte Objekte (nur Top-Level)
RETURN LENGTH({ user: { name: "Max", age: 30 }, active: true })
// Ergebnis: 2 (nicht 3!)

-- Null-Handling
RETURN LENGTH(null)
// Ergebnis: 0
```

**Praxisbeispiele:**

```aql
-- Kurze Beschreibungen finden
FOR product IN products
  FILTER LENGTH(product.description) < 50
  RETURN { id: product._key, description: product.description }

-- Validierung: Mindestlänge
FOR user IN users
  FILTER LENGTH(user.password_hash) < 60
  RETURN { user: user.email, warning: "Passwort-Hash zu kurz" }

-- Array-Größen analysieren
FOR order IN orders
  LET itemCount = LENGTH(order.items)
  COLLECT bucket = FLOOR(itemCount / 5) * 5
  AGGREGATE count = COUNT(1)
  SORT bucket
  RETURN { items: CONCAT(bucket, "-", bucket + 4), orders: count }
```

**Edge Cases:**

```aql
RETURN LENGTH("")           // 0
RETURN LENGTH(" ")          // 1
RETURN LENGTH("   ")        // 3
RETURN LENGTH([null, null]) // 2 (Elemente zählen, auch null)
RETURN LENGTH({})           // 0
```

---

### CONCAT(...values)

Verbindet mehrere Werte zu einem String. Null-Werte werden als leerer String behandelt.

**Signatur:**
```
CONCAT(value1, value2, ...) → string
CONCAT(array) → string
```

**Parameter:**

| Parameter | Typ | Beschreibung |
|-----------|-----|--------------|
| `values` | any... | Beliebig viele Werte |
| `array` | array | Array von Werten |

**Rückgabewert:** `string` - Der verkettete String

**Typ-Konvertierung:**
- **string**: Unverändert
- **number**: Als Dezimalzahl
- **boolean**: `"true"` oder `"false"`
- **null**: Leerer String `""`
- **array**: JSON-Darstellung
- **object**: JSON-Darstellung

**Beispiele:**

```aql
-- Einfache Verkettung
RETURN CONCAT("Hello", " ", "World")
// Ergebnis: "Hello World"

-- Mit Variablen
FOR user IN users
  RETURN CONCAT(user.firstName, " ", user.lastName)
// Ergebnis: "Max Mustermann"

-- Zahlen einbinden
RETURN CONCAT("Preis: ", 42.50, " EUR")
// Ergebnis: "Preis: 42.5 EUR"

-- Null-Handling (ignoriert null)
RETURN CONCAT("Hello", null, "World")
// Ergebnis: "HelloWorld"

-- Array als Eingabe
RETURN CONCAT(["Hello", " ", "World"])
// Ergebnis: "Hello World"

-- Boolean einbinden
RETURN CONCAT("Status: ", true)
// Ergebnis: "Status: true"
```

**Praxisbeispiele:**

```aql
-- Vollständige Adresse generieren
FOR customer IN customers
  LET address = CONCAT(
    customer.street, " ", customer.houseNumber, "\n",
    customer.zipCode, " ", customer.city, "\n",
    customer.country
  )
  RETURN { customer: customer.name, address }

-- URL generieren
FOR product IN products
  LET url = CONCAT(
    "https://shop.example.com/products/",
    product.category, "/",
    product.slug
  )
  RETURN { product: product.name, url }

-- Log-Nachricht erstellen
FOR event IN events
  LET logLine = CONCAT(
    "[", DATE_FORMAT(event.timestamp, "%Y-%m-%d %H:%M:%S"), "] ",
    "[", UPPER(event.level), "] ",
    event.message
  )
  RETURN logLine
// Ergebnis: "[2024-01-15 10:30:45] [ERROR] Connection timeout"
```

**Verwandte Funktionen:**
- `CONCAT_SEPARATOR()` - Mit Trennzeichen
- `+` Operator - Alternative für zwei Strings

---

### SUBSTRING(str, start, length?)

Extrahiert einen Teilstring ab einer Position.

**Signatur:**
```
SUBSTRING(str, start) → string
SUBSTRING(str, start, length) → string
```

**Parameter:**

| Parameter | Typ | Beschreibung | Default |
|-----------|-----|--------------|---------|
| `str` | string | Quell-String | - |
| `start` | number | Startposition (0-basiert) | - |
| `length` | number | Anzahl Zeichen | Bis Ende |

**Rückgabewert:** `string` - Der extrahierte Teilstring

**Besonderheiten:**
- Negative `start`-Werte: Vom Ende zählen
- `length` > verfügbare Zeichen: Bis Ende
- `start` > String-Länge: Leerer String

**Beispiele:**

```aql
-- Einfache Extraktion
RETURN SUBSTRING("ThemisDB", 0, 6)
// Ergebnis: "Themis"

RETURN SUBSTRING("ThemisDB", 6)
// Ergebnis: "DB"

-- Negative Startposition
RETURN SUBSTRING("ThemisDB", -2)
// Ergebnis: "DB"

RETURN SUBSTRING("ThemisDB", -5, 3)
// Ergebnis: "mis"

-- Über String-Ende hinaus
RETURN SUBSTRING("Hello", 3, 100)
// Ergebnis: "lo"

-- Leeres Ergebnis
RETURN SUBSTRING("Hello", 10)
// Ergebnis: ""
```

**Praxisbeispiele:**

```aql
-- Vorwahl extrahieren (deutsche Telefonnummern)
FOR contact IN contacts
  LET phone = contact.phone
  LET areaCode = (
    STARTS_WITH(phone, "+49") ? SUBSTRING(phone, 3, 4) :
    STARTS_WITH(phone, "0") ? SUBSTRING(phone, 0, 4) :
    null
  )
  RETURN { name: contact.name, phone, areaCode }

-- Dateiname kürzen für Anzeige
FOR file IN files
  LET shortName = LENGTH(file.name) > 30 
    ? CONCAT(SUBSTRING(file.name, 0, 27), "...")
    : file.name
  RETURN { id: file._key, displayName: shortName }

-- Jahr aus Datumsstring extrahieren
FOR event IN events
  LET year = SUBSTRING(event.date_string, 0, 4)
  COLLECT year = year
  AGGREGATE count = COUNT(1)
  RETURN { year, count }
```

---

### UPPER(str) / LOWER(str)

Konvertiert String zu Groß- oder Kleinschreibung.

**Signatur:**
```
UPPER(str) → string
LOWER(str) → string
```

**Parameter:**

| Parameter | Typ | Beschreibung |
|-----------|-----|--------------|
| `str` | string | Der zu konvertierende String |

**Rückgabewert:** `string` - Der konvertierte String

**Unicode-Unterstützung:**
Vollständige Unicode-Unterstützung inklusive:
- Deutsche Umlaute (ä→Ä, ö→Ö, ü→Ü, ß→SS)
- Akzentzeichen (é→É, ñ→Ñ)
- Griechische, kyrillische und andere Alphabete

**Beispiele:**

```aql
RETURN UPPER("hello world")
// Ergebnis: "HELLO WORLD"

RETURN LOWER("HELLO WORLD")
// Ergebnis: "hello world"

-- Deutsche Umlaute
RETURN UPPER("größe")
// Ergebnis: "GRÖSSE" (ß → SS)

RETURN LOWER("GRÖSSE")
// Ergebnis: "grösse"

-- Mixed Case normalisieren
RETURN LOWER("ThEmIsDb")
// Ergebnis: "themisdb"
```

**Praxisbeispiele:**

```aql
-- Case-insensitive Suche
FOR product IN products
  FILTER LOWER(product.name) == LOWER(@searchTerm)
  RETURN product

-- E-Mail-Normalisierung
FOR user IN users
  UPDATE user WITH { email: LOWER(user.email) } IN users

-- Titel-Formatierung (First Letter Uppercase)
FOR article IN articles
  LET words = SPLIT(article.title, " ")
  LET formatted = (
    FOR word IN words
      RETURN CONCAT(UPPER(SUBSTRING(word, 0, 1)), LOWER(SUBSTRING(word, 1)))
  )
  RETURN { 
    original: article.title, 
    formatted: CONCAT_SEPARATOR(" ", formatted) 
  }
```

---

### TRIM(str) / LTRIM(str) / RTRIM(str)

Entfernt Leerzeichen oder spezifische Zeichen von Strings.

**Signatur:**
```
TRIM(str) → string
TRIM(str, chars) → string
LTRIM(str) → string
LTRIM(str, chars) → string
RTRIM(str) → string
RTRIM(str, chars) → string
```

**Parameter:**

| Parameter | Typ | Beschreibung | Default |
|-----------|-----|--------------|---------|
| `str` | string | Der zu trimmende String | - |
| `chars` | string | Zu entfernende Zeichen | Whitespace |

**Rückgabewert:** `string` - Der getrimmte String

**Whitespace-Definition:**
Standardmäßig werden entfernt: Space, Tab, Newline, Carriage Return, Form Feed

**Beispiele:**

```aql
-- Basis-Trim
RETURN TRIM("  hello  ")
// Ergebnis: "hello"

RETURN LTRIM("  hello  ")
// Ergebnis: "hello  "

RETURN RTRIM("  hello  ")
// Ergebnis: "  hello"

-- Newlines entfernen
RETURN TRIM("  \n  hello  \n  ")
// Ergebnis: "hello"

-- Spezifische Zeichen entfernen
RETURN TRIM("---hello---", "-")
// Ergebnis: "hello"

RETURN LTRIM("000123", "0")
// Ergebnis: "123"

RETURN RTRIM("price$$$", "$")
// Ergebnis: "price"

-- Mehrere Zeichen
RETURN TRIM("<<hello>>", "<>")
// Ergebnis: "hello"
```

**Praxisbeispiele:**

```aql
-- Datenbereinigung bei Import
FOR record IN raw_import
  UPDATE record WITH {
    name: TRIM(record.name),
    email: TRIM(LOWER(record.email)),
    phone: TRIM(REPLACE(record.phone, " ", ""))
  } IN raw_import

-- CSV-Werte bereinigen
FOR line IN csv_lines
  LET values = SPLIT(line, ",")
  LET cleaned = (FOR v IN values RETURN TRIM(v))
  RETURN cleaned

-- Führende Nullen entfernen (z.B. Artikelnummern)
FOR product IN products
  LET cleanedSku = LTRIM(product.sku, "0")
  UPDATE product WITH { sku_clean: cleanedSku } IN products
```

---

### SPLIT(str, separator)

Teilt einen String in ein Array anhand eines Trennzeichens.

**Signatur:**
```
SPLIT(str, separator) → array
SPLIT(str, separator, limit) → array
```

**Parameter:**

| Parameter | Typ | Beschreibung | Default |
|-----------|-----|--------------|---------|
| `str` | string | Der zu teilende String | - |
| `separator` | string | Das Trennzeichen | - |
| `limit` | number | Maximale Teile | Unbegrenzt |

**Rückgabewert:** `array` - Array von Strings

**Besonderheiten:**
- Leerer Separator teilt in einzelne Zeichen
- Separator am Ende erzeugt leeres Element
- Aufeinanderfolgende Separatoren erzeugen leere Elemente

**Beispiele:**

```aql
-- Einfaches Teilen
RETURN SPLIT("a,b,c", ",")
// Ergebnis: ["a", "b", "c"]

-- Mit Limit
RETURN SPLIT("a,b,c,d,e", ",", 3)
// Ergebnis: ["a", "b", "c,d,e"]

-- Leere Elemente
RETURN SPLIT("a,,b", ",")
// Ergebnis: ["a", "", "b"]

-- Leerer String
RETURN SPLIT("", ",")
// Ergebnis: [""]

-- In Zeichen teilen
RETURN SPLIT("hello", "")
// Ergebnis: ["h", "e", "l", "l", "o"]
```

**Praxisbeispiele:**

```aql
-- E-Mail-Domain extrahieren
FOR user IN users
  LET parts = SPLIT(user.email, "@")
  RETURN { 
    user: parts[0], 
    domain: parts[1],
    topLevel: LAST(SPLIT(parts[1], "."))
  }

-- Tags parsen (kommasepariert)
FOR article IN articles
  LET tags = (
    FOR tag IN SPLIT(article.tags_string, ",")
      RETURN TRIM(tag)
  )
  RETURN { title: article.title, tags }

-- Pfad-Komponenten
FOR file IN files
  LET pathParts = SPLIT(file.path, "/")
  RETURN {
    file: LAST(pathParts),
    folder: NTH(pathParts, LENGTH(pathParts) - 2),
    depth: LENGTH(pathParts) - 1
  }

-- IP-Adresse parsen
FOR log IN access_logs
  LET octets = SPLIT(log.ip_address, ".")
  FILTER TO_NUMBER(octets[0]) == 192
  FILTER TO_NUMBER(octets[1]) == 168
  RETURN log
```

---

### CONTAINS(str, search) / STARTS_WITH(str, prefix) / ENDS_WITH(str, suffix)

Prüft ob ein String einen Teilstring enthält oder mit bestimmten Zeichen beginnt/endet.

**Signatur:**
```
CONTAINS(str, search) → bool
CONTAINS(str, search, returnIndex) → bool | number
STARTS_WITH(str, prefix) → bool
ENDS_WITH(str, suffix) → bool
```

**Parameter:**

| Parameter | Typ | Beschreibung |
|-----------|-----|--------------|
| `str` | string | Der zu durchsuchende String |
| `search`/`prefix`/`suffix` | string | Der gesuchte String |
| `returnIndex` | boolean | Wenn true, Position statt bool |

**Rückgabewert:** `bool` oder `number` (Position, -1 wenn nicht gefunden)

**Beispiele:**

```aql
-- Einfache Prüfung
RETURN CONTAINS("Hello World", "World")
// Ergebnis: true

RETURN CONTAINS("Hello World", "world")
// Ergebnis: false (case-sensitive!)

-- Position zurückgeben
RETURN CONTAINS("Hello World", "World", true)
// Ergebnis: 6

RETURN CONTAINS("Hello World", "xyz", true)
// Ergebnis: -1

-- Prefix-Prüfung
RETURN STARTS_WITH("ThemisDB", "Themis")
// Ergebnis: true

RETURN STARTS_WITH("ThemisDB", "themis")
// Ergebnis: false

-- Suffix-Prüfung
RETURN ENDS_WITH("document.pdf", ".pdf")
// Ergebnis: true

RETURN ENDS_WITH("document.pdf", ".PDF")
// Ergebnis: false
```

**Praxisbeispiele:**

```aql
-- Case-insensitive Suche
FOR product IN products
  FILTER CONTAINS(LOWER(product.description), LOWER(@searchTerm))
  RETURN product

-- Dateitypen filtern
FOR file IN files
  FILTER ENDS_WITH(LOWER(file.name), ".pdf") OR 
         ENDS_WITH(LOWER(file.name), ".doc") OR
         ENDS_WITH(LOWER(file.name), ".docx")
  RETURN file

-- URLs analysieren
FOR url IN urls
  LET isSecure = STARTS_WITH(url.href, "https://")
  LET domain = (
    LET withoutProtocol = STARTS_WITH(url.href, "https://") 
      ? SUBSTRING(url.href, 8) 
      : SUBSTRING(url.href, 7)
    LET endPos = CONTAINS(withoutProtocol, "/", true)
    RETURN endPos > 0 ? SUBSTRING(withoutProtocol, 0, endPos) : withoutProtocol
  )
  RETURN { url: url.href, isSecure, domain }

-- E-Mail-Validation (einfach)
FOR user IN users
  LET email = user.email
  FILTER CONTAINS(email, "@")
  FILTER CONTAINS(email, ".")
  FILTER NOT STARTS_WITH(email, "@")
  FILTER NOT ENDS_WITH(email, ".")
  RETURN user
```

---

### REPLACE(str, search, replacement)

Ersetzt alle Vorkommen eines Substrings.

**Signatur:**
```
REPLACE(str, search, replacement) → string
REPLACE(str, search, replacement, limit) → string
```

**Parameter:**

| Parameter | Typ | Beschreibung | Default |
|-----------|-----|--------------|---------|
| `str` | string | Der Quell-String | - |
| `search` | string | Der zu ersetzende String | - |
| `replacement` | string | Der Ersetzungs-String | - |
| `limit` | number | Max. Ersetzungen | Alle |

**Rückgabewert:** `string` - Der modifizierte String

**Beispiele:**

```aql
-- Einfache Ersetzung
RETURN REPLACE("Hello World", "World", "ThemisDB")
// Ergebnis: "Hello ThemisDB"

-- Mehrfache Ersetzung
RETURN REPLACE("aaa", "a", "b")
// Ergebnis: "bbb"

-- Mit Limit
RETURN REPLACE("aaa", "a", "b", 2)
// Ergebnis: "bba"

-- Zeichen entfernen
RETURN REPLACE("Hello World", " ", "")
// Ergebnis: "HelloWorld"

-- Leerer Ersetzungsstring
RETURN REPLACE("H-e-l-l-o", "-", "")
// Ergebnis: "Hello"
```

**Praxisbeispiele:**

```aql
-- Telefonnummern normalisieren
FOR contact IN contacts
  LET phone = contact.phone
  LET normalized = REPLACE(REPLACE(REPLACE(
    phone, " ", ""), "-", ""), "/", "")
  UPDATE contact WITH { phone_normalized: normalized } IN contacts

-- Slugs generieren
FOR article IN articles
  LET slug = LOWER(REPLACE(REPLACE(REPLACE(
    article.title, " ", "-"), "ä", "ae"), "ü", "ue"))
  UPDATE article WITH { slug } IN articles

-- Sensible Daten maskieren
FOR user IN users
  LET maskedEmail = CONCAT(
    SUBSTRING(user.email, 0, 2),
    REPLACE(SUBSTRING(user.email, 2, CONTAINS(user.email, "@", true) - 2), 
            REGEX_REPLACE(SUBSTRING(user.email, 2, CONTAINS(user.email, "@", true) - 2), ".", "*")),
    SUBSTRING(user.email, CONTAINS(user.email, "@", true))
  )
  RETURN { id: user._key, maskedEmail }
```

---

### REGEX_TEST(str, pattern) / REGEX_REPLACE(str, pattern, replacement)

Reguläre Ausdrücke für Pattern Matching und Ersetzung.

**Signatur:**
```
REGEX_TEST(str, pattern) → bool
REGEX_TEST(str, pattern, ignoreCase) → bool
REGEX_REPLACE(str, pattern, replacement) → string
REGEX_REPLACE(str, pattern, replacement, ignoreCase) → string
```

**Parameter:**

| Parameter | Typ | Beschreibung | Default |
|-----------|-----|--------------|---------|
| `str` | string | Der zu prüfende String | - |
| `pattern` | string | Regex-Pattern | - |
| `replacement` | string | Ersetzungs-String | - |
| `ignoreCase` | boolean | Case-insensitive | false |

**Rückgabewert:** `bool` oder `string`

**Regex-Syntax:**
ThemisDB unterstützt ECMAScript-kompatible reguläre Ausdrücke:

| Pattern | Bedeutung |
|---------|-----------|
| `.` | Beliebiges Zeichen |
| `*` | 0 oder mehr |
| `+` | 1 oder mehr |
| `?` | 0 oder 1 |
| `^` | Zeilenanfang |
| `$` | Zeilenende |
| `[abc]` | Zeichenklasse |
| `[^abc]` | Negierte Zeichenklasse |
| `\d` | Ziffer |
| `\w` | Wortzeichen |
| `\s` | Whitespace |
| `(...)` | Gruppe |
| `\1`, `\2` | Rückreferenz |

**Beispiele:**

```aql
-- E-Mail-Validierung
RETURN REGEX_TEST("user@example.com", "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$")
// Ergebnis: true

RETURN REGEX_TEST("invalid-email", "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$")
// Ergebnis: false

-- Telefonnummer-Format (deutsch)
RETURN REGEX_TEST("+49 123 456789", "^\\+49\\s*\\d{3,}\\s*\\d+$")
// Ergebnis: true

-- Case-insensitive
RETURN REGEX_TEST("Hello World", "hello", true)
// Ergebnis: true

-- Ersetzung mit Gruppen
RETURN REGEX_REPLACE("John Doe", "^(\\w+)\\s+(\\w+)$", "$2, $1")
// Ergebnis: "Doe, John"

-- Alle Zahlen entfernen
RETURN REGEX_REPLACE("abc123def456", "\\d+", "")
// Ergebnis: "abcdef"

-- Mehrere Leerzeichen zu einem
RETURN REGEX_REPLACE("Hello    World", "\\s+", " ")
// Ergebnis: "Hello World"
```

**Praxisbeispiele:**

```aql
-- Verschiedene Datumsformate erkennen
FOR doc IN documents
  LET dateStr = doc.date_field
  LET format = (
    REGEX_TEST(dateStr, "^\\d{4}-\\d{2}-\\d{2}$") ? "ISO" :
    REGEX_TEST(dateStr, "^\\d{2}\\.\\d{2}\\.\\d{4}$") ? "DE" :
    REGEX_TEST(dateStr, "^\\d{2}/\\d{2}/\\d{4}$") ? "US" :
    "UNKNOWN"
  )
  RETURN { dateStr, format }

-- PLZ validieren (Deutschland)
FOR customer IN customers
  LET validPLZ = REGEX_TEST(customer.zip, "^\\d{5}$")
  FILTER NOT validPLZ
  RETURN { customer: customer.name, invalidZip: customer.zip }

-- Kreditkartennummern maskieren
FOR transaction IN transactions
  LET maskedCC = REGEX_REPLACE(
    transaction.card_number,
    "^(\\d{4})\\d{8}(\\d{4})$",
    "$1********$2"
  )
  RETURN { id: transaction._key, card: maskedCC }

-- HTML-Tags entfernen
FOR article IN articles
  LET plainText = REGEX_REPLACE(article.content, "<[^>]+>", "")
  RETURN { title: article.title, plainText: SUBSTRING(plainText, 0, 200) }
```

**Performance-Hinweis:**
Regex-Operationen sind rechenintensiv. Für einfache Suchen ist `CONTAINS()` deutlich schneller.

---

### LEVENSHTEIN_DISTANCE(str1, str2)

Berechnet die Edit-Distanz (minimale Anzahl von Einfügungen, Löschungen, Ersetzungen) zwischen zwei Strings.

**Signatur:**
```
LEVENSHTEIN_DISTANCE(str1, str2) → number
```

**Parameter:**

| Parameter | Typ | Beschreibung |
|-----------|-----|--------------|
| `str1` | string | Erster String |
| `str2` | string | Zweiter String |

**Rückgabewert:** `number` - Die Levenshtein-Distanz (0 = identisch)

**Anwendungsfälle:**
- Tippfehler-Toleranz
- Fuzzy-Suche
- Ähnlichkeitsanalyse
- Plagiatserkennung

**Beispiele:**

```aql
-- Identische Strings
RETURN LEVENSHTEIN_DISTANCE("hello", "hello")
// Ergebnis: 0

-- Ein Buchstabe anders
RETURN LEVENSHTEIN_DISTANCE("hello", "hallo")
// Ergebnis: 1

-- Klassisches Beispiel
RETURN LEVENSHTEIN_DISTANCE("kitten", "sitting")
// Ergebnis: 3 (k→s, e→i, +g)

-- Unterschiedliche Längen
RETURN LEVENSHTEIN_DISTANCE("abc", "abcdef")
// Ergebnis: 3

-- Komplett unterschiedlich
RETURN LEVENSHTEIN_DISTANCE("abc", "xyz")
// Ergebnis: 3
```

**Praxisbeispiele:**

```aql
-- Tippfehler-tolerante Suche
FOR product IN products
  LET distance = LEVENSHTEIN_DISTANCE(LOWER(product.name), LOWER(@searchTerm))
  FILTER distance <= 2  -- Max. 2 Änderungen
  SORT distance ASC, product.popularity DESC
  LIMIT 10
  RETURN { product, distance, match: distance == 0 ? "exact" : "fuzzy" }

-- "Did you mean?" Vorschläge
LET searchTerm = "Thims"
FOR keyword IN search_keywords
  LET distance = LEVENSHTEIN_DISTANCE(LOWER(keyword.term), LOWER(searchTerm))
  FILTER distance > 0 AND distance <= 2
  SORT distance ASC, keyword.frequency DESC
  LIMIT 5
  RETURN keyword.term

-- Duplikaterkennung
FOR doc1 IN documents
  FOR doc2 IN documents
    FILTER doc1._key < doc2._key  -- Vermeidet Doppelvergleiche
    LET titleSim = 1 - (LEVENSHTEIN_DISTANCE(doc1.title, doc2.title) / 
                        MAX(LENGTH(doc1.title), LENGTH(doc2.title)))
    FILTER titleSim > 0.8
    RETURN {
      doc1: doc1.title,
      doc2: doc2.title,
      similarity: ROUND(titleSim * 100)
    }

-- Normalisierte Ähnlichkeit (0-1)
LET str1 = "ThemisDB"
LET str2 = "ThemisDatabase"
LET distance = LEVENSHTEIN_DISTANCE(str1, str2)
LET maxLen = MAX(LENGTH(str1), LENGTH(str2))
LET similarity = 1 - (distance / maxLen)
RETURN { str1, str2, distance, similarity: ROUND(similarity, 2) }
// Ergebnis: { str1: "ThemisDB", str2: "ThemisDatabase", distance: 6, similarity: 0.57 }
```

**Performance-Hinweis:**
Levenshtein-Berechnung ist O(n*m) mit n und m als String-Längen. Für große Datensätze:
1. Erst mit `LENGTH()` vorfiltern
2. Erst mit `STARTS_WITH()` vorfiltern
3. Index auf normalisierte Versionen erstellen

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

ThemisDB bietet **~45 Date-Funktionen** mit SQL-kompatibler Syntax und erweiterten Arbeitstag-Berechnungen.

### Aktuelles Datum/Zeit (SQL-kompatibel)

```aql
-- Grundfunktionen
RETURN DATE_NOW()           -- Unix Timestamp in ms
RETURN NOW()                -- SQL-Standard Alias
RETURN CURRENT_TIMESTAMP()  -- SQL-Standard
RETURN CURRENT_DATE()       -- Nur Datum (00:00:00 UTC)
RETURN CURRENT_TIME()       -- Zeit seit Mitternacht (ms)

-- Praktische Helper
RETURN TODAY()              -- Start von heute (00:00:00)
RETURN YESTERDAY()          -- Start von gestern
RETURN TOMORROW()           -- Start von morgen

-- DB-kompatible Aliase
RETURN GETDATE()            -- SQL Server kompatibel
RETURN SYSDATE()            -- Oracle kompatibel
RETURN UNIX_TIMESTAMP()     -- MySQL kompatibel (Sekunden)
```

### Komponenten extrahieren

```aql
LET ts = DATE_TIMESTAMP("2024-06-15T14:30:00Z")
RETURN DATE_YEAR(ts)        -- 2024
RETURN DATE_MONTH(ts)       -- 6
RETURN DATE_DAY(ts)         -- 15
RETURN DATE_HOUR(ts)        -- 14
RETURN DATE_MINUTE(ts)      -- 30
RETURN DATE_SECOND(ts)      -- 0
RETURN DATE_MILLISECOND(ts) -- 0
RETURN DATE_DAYOFWEEK(ts)   -- 6 (Samstag, 0=Sonntag)
RETURN DATE_DAYOFYEAR(ts)   -- 167
RETURN DATE_QUARTER(ts)     -- 2
RETURN DATE_WEEK(ts)        -- 24 (ISO-Kalenderwoche)
```

### Datum-Konstruktion

```aql
-- Datum aus Komponenten
RETURN MAKE_DATE(2024, 12, 25)              -- Weihnachten 2024
RETURN MAKE_DATETIME(2024, 12, 24, 18, 0)   -- Heiligabend 18:00
RETURN MAKE_TIME(14, 30, 0)                 -- 14:30:00

-- Umrechnung
RETURN FROM_UNIXTIME(1700000000)            -- Sekunden zu Timestamp
RETURN EPOCH_SECONDS(1700000000000)         -- ms zu Sekunden
```

### Interval-Funktionen für relative Zeitangaben ⭐

```aql
-- Einfache Syntax für relative Zeit
FOR event IN events
  FILTER event.date >= NOW() - DAYS(7)      -- Letzte 7 Tage
  RETURN event

-- Alle Interval-Funktionen
RETURN YEARS(1)       -- 1 Jahr in ms (~31,557,600,000)
RETURN MONTHS(3)      -- 3 Monate in ms
RETURN WEEKS(2)       -- 2 Wochen in ms
RETURN DAYS(10)       -- 10 Tage in ms
RETURN HOURS(24)      -- 24 Stunden in ms
RETURN MINUTES(30)    -- 30 Minuten in ms
RETURN SECONDS(60)    -- 60 Sekunden in ms

-- Flexibles INTERVAL
RETURN INTERVAL(6, "months")    -- Wie MONTHS(6)
RETURN INTERVAL(2.5, "weeks")   -- 2.5 Wochen

-- Praktische Kombination
FOR order IN orders
  FILTER order.created >= TODAY() - WEEKS(2)   -- Letzte 2 Wochen
  FILTER order.deadline < TOMORROW()           -- Fällig bis morgen
  RETURN order
```

### Datum-Arithmetik

```aql
LET now = DATE_NOW()

-- Mit DATE_ADD/SUBTRACT
RETURN DATE_ADD(now, 7, "days")
RETURN DATE_SUBTRACT(now, 1, "months")

-- Mit Interval-Funktionen (eleganter)
RETURN now + DAYS(7)          -- 7 Tage in die Zukunft
RETURN now - MONTHS(1)        -- 1 Monat zurück
RETURN now + YEARS(1)         -- In einem Jahr

-- Differenz berechnen
LET start = DATE_TIMESTAMP("2024-01-01")
LET end = DATE_TIMESTAMP("2024-12-31")
RETURN DATE_DIFF(start, end, "days")    -- 365
RETURN DATE_DIFF(start, end, "months")  -- 12
RETURN DATE_DIFF(start, end, "weeks")   -- 52
```

### Arbeitstage-Funktionen ⭐

```aql
-- Arbeitstage mit Feiertags-Kalender
LET holidays = HOLIDAYS("DE_2024")   -- Deutsche Feiertage 2024
LET start = MAKE_DATE(2024, 12, 1)
LET end = MAKE_DATE(2024, 12, 31)

-- Arbeitstage zählen (Mo-Fr, ohne Feiertage)
RETURN WORKDAYS(start, end, holidays)   -- ~18 Arbeitstage

-- Arbeitstage addieren (Lieferzeit-Berechnung)
LET orderDate = NOW()
LET deliveryDate = WORKDAYS_ADD(orderDate, 10, holidays)
RETURN DATE_FORMAT(deliveryDate, "%Y-%m-%d")

-- Prüffunktionen
RETURN IS_WEEKEND(MAKE_DATE(2024, 12, 25))    -- true (Mittwoch? false)
RETURN IS_WORKDAY(MAKE_DATE(2024, 12, 25), holidays)  -- false (Feiertag)
```

### Verfügbare Feiertags-Kalender ⭐

```aql
-- Liste aller verfügbaren Kalender
RETURN LIST_CALENDARS()
-- ["DE_2024", "DE_2025", "AT_2024", "CH_2024", "US_FEDERAL_2024", ...]

-- Kalender laden
LET deHolidays = HOLIDAYS("DE_2024")      -- Deutschland 2024
LET atHolidays = HOLIDAYS("AT_2024")      -- Österreich 2024
LET chHolidays = HOLIDAYS("CH_2024")      -- Schweiz 2024
LET usHolidays = HOLIDAYS("US_FEDERAL_2024")  -- USA Federal 2024
LET ukHolidays = HOLIDAYS("UK_2024")      -- Großbritannien 2024
LET frHolidays = HOLIDAYS("FR_2024")      -- Frankreich 2024

-- Kalender zusammenführen (Firma mit Standorten DE + AT)
LET companyHolidays = HOLIDAYS("DE_2024", "AT_2024")

-- Inline-Feiertage definieren
LET customHolidays = HOLIDAYS("2024-12-23", "2024-12-27", "2024-12-30")

-- Feiertage in Zeitraum filtern
RETURN HOLIDAYS_BETWEEN("DE_2024", MAKE_DATE(2024, 12, 1), MAKE_DATE(2024, 12, 31))
```

### Hilfsfunktionen

```aql
-- Schaltjahr prüfen
RETURN DATE_LEAPYEAR(2024)  -- true
RETURN DATE_LEAPYEAR(2023)  -- false

-- Tage im Monat
RETURN DATE_DAYS_IN_MONTH(2024, 2)  -- 29 (Schaltjahr)
RETURN DATE_DAYS_IN_MONTH(2023, 2)  -- 28

-- Wochenanfang/-ende
RETURN DATE_START_OF_WEEK(NOW())        -- Montag 00:00
RETURN DATE_START_OF_WEEK(NOW(), 0)     -- Sonntag 00:00 (US-Style)
RETURN DATE_END_OF_MONTH(NOW())         -- Letzter Tag des Monats

-- Alter berechnen
FOR person IN persons
  LET age = AGE(person.birthdate)
  FILTER age >= 18
  RETURN { name: person.name, age }

-- Datums-Vergleich
RETURN DATE_COMPARE(date1, date2)       -- -1, 0, oder 1
RETURN DATE_BETWEEN(checkDate, start, end)  -- true/false
```

### Formatierung

```aql
LET ts = DATE_TIMESTAMP("2024-06-15T14:30:00Z")
RETURN DATE_FORMAT(ts, "%Y-%m-%d")        -- "2024-06-15"
RETURN DATE_FORMAT(ts, "%d.%m.%Y %H:%M")  -- "15.06.2024 14:30"
RETURN DATE_ISO8601(ts)                   -- "2024-06-15T14:30:00Z"

-- Auf Zeiteinheit runden
RETURN DATE_TRUNC(ts, "year")    -- "2024-01-01T00:00:00Z"
RETURN DATE_TRUNC(ts, "month")   -- "2024-06-01T00:00:00Z"
RETURN DATE_TRUNC(ts, "week")    -- Wochenanfang
RETURN DATE_TRUNC(ts, "day")     -- "2024-06-15T00:00:00Z"
RETURN DATE_TRUNC(ts, "hour")    -- "2024-06-15T14:00:00Z"
```

**Praxisbeispiel: Aktivität der letzten 30 Tage**

```aql
FOR event IN events
  FILTER event.created_at >= NOW() - DAYS(30)   -- Elegante Syntax
  COLLECT week = DATE_TRUNC(event.created_at, "week")
  AGGREGATE count = COUNT(1)
  SORT week ASC
  RETURN { week: DATE_FORMAT(week, "%Y-W%V"), count }
```

**Praxisbeispiel: Fälligkeitsdatum mit Arbeitstagen**

```aql
LET holidays = HOLIDAYS("DE_2024")
FOR order IN orders
  FILTER order.status == "pending"
  LET dueDate = WORKDAYS_ADD(order.created_at, 10, holidays)
  LET overdue = dueDate < NOW()
  RETURN {
    orderId: order._key,
    createdAt: DATE_FORMAT(order.created_at, "%d.%m.%Y"),
    dueDate: DATE_FORMAT(dueDate, "%d.%m.%Y"),
    overdue,
    daysRemaining: overdue ? 0 : WORKDAYS(NOW(), dueDate, holidays)
  }
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

## Collection-Funktionen

ThemisDB bietet **~25 Collection-Funktionen** für das Erstellen und Manipulieren von Arrays, Objekten und JSON-Daten mit **JSON-Native Support**.

### Array-Konstruktoren

```aql
-- Einfaches Array erstellen
RETURN ARRAY(1, 2, 3)                    -- [1, 2, 3]
RETURN ARRAY("a", "b", "c")              -- ["a", "b", "c"]
RETURN ARRAY()                           -- []

-- JSON-Native Parsing ⭐
RETURN ARRAY('[1, 2, 3]')                -- [1, 2, 3] (JSON-String geparst)
RETURN ARRAY('[1, 2]', '[3, 4]')         -- [[1, 2], [3, 4]]

-- Set (unique values)
RETURN SET(1, 2, 2, 3, 3, 3)             -- [1, 2, 3]
RETURN SET("a", "b", "a")                -- ["a", "b"]

-- Tuple (feste Größe)
RETURN TUPLE(x, y, z)                    -- [x, y, z]
RETURN PAIR("key", "value")              -- ["key", "value"]

-- Range
RETURN RANGE(0, 5)                       -- [0, 1, 2, 3, 4]
RETURN RANGE(1, 10, 2)                   -- [1, 3, 5, 7, 9]
RETURN RANGE(10, 0, -1)                  -- [10, 9, 8, ..., 1]

-- Repeat
RETURN REPEAT(0, 5)                      -- [0, 0, 0, 0, 0]
RETURN REPEAT("x", 3)                    -- ["x", "x", "x"]
```

### Objekt-Konstruktoren (DICT)

```aql
-- Key-Value Paare
RETURN DICT("name", "Alice", "age", 30)
-- {"name": "Alice", "age": 30}

RETURN DICT("x", 1, "y", 2, "z", 3)
-- {"x": 1, "y": 2, "z": 3}

-- JSON-Native Parsing ⭐
RETURN DICT('{"name": "Alice", "age": 30}')
-- {"name": "Alice", "age": 30}

-- Verschachtelt
RETURN DICT("person", DICT('{"name": "Bob"}'), "active", true)
-- {"person": {"name": "Bob"}, "active": true}

-- OBJECT Alias
RETURN OBJECT("key", "value")            -- {"key": "value"}
```

### JSON-Funktionen ⭐

```aql
-- JSON parsen
RETURN JSON('[1, 2, 3]')                 -- [1, 2, 3]
RETURN JSON('{"name": "Alice"}')         -- {"name": "Alice"}
RETURN JSON('null')                      -- null
RETURN JSON('123')                       -- 123

-- JSON serialisieren
RETURN TO_JSON([1, 2, 3])                -- "[1,2,3]"
RETURN TO_JSON({name: "Alice"})          -- '{"name":"Alice"}'
RETURN TO_JSON(doc, true)                -- Pretty-printed (mit Indentation)

-- JSON validieren
RETURN JSON_VALID('[1, 2, 3]')           -- true
RETURN JSON_VALID('not json')            -- false
RETURN JSON_VALID('{"incomplete":')      -- false

-- JSON-Typ ermitteln
RETURN JSON_TYPE([1, 2])                 -- "array"
RETURN JSON_TYPE({a: 1})                 -- "object"
RETURN JSON_TYPE(123)                    -- "number"
RETURN JSON_TYPE("text")                 -- "string"
RETURN JSON_TYPE(null)                   -- "null"
RETURN JSON_TYPE(true)                   -- "boolean"
```

### Objekt-Konvertierung

```aql
-- Object zu Array
RETURN KEYS({a: 1, b: 2, c: 3})          -- ["a", "b", "c"]
RETURN ENTRIES({a: 1, b: 2})             -- [["a", 1], ["b", 2]]

-- Array zu Object
RETURN FROM_ENTRIES([["a", 1], ["b", 2]])  -- {a: 1, b: 2}

-- String zu Array (splitting)
RETURN LIST("a,b,c")                     -- ["a", "b", "c"]
RETURN LIST("a;b;c")                     -- ["a", "b", "c"]
RETURN LIST("a\nb\nc")                   -- ["a", "b", "c"]

-- Object Values zu Array
RETURN LIST({a: 1, b: 2})                -- [1, 2]
```

### Holiday-Funktionen ⭐

```aql
-- Feiertags-Kalender laden
LET holidays = HOLIDAYS("DE_2024")       -- Deutsche Feiertage 2024

-- Verfügbare Kalender anzeigen
RETURN LIST_CALENDARS()
-- ["DE_2024", "DE_2025", "AT_2024", "CH_2024", 
--  "US_FEDERAL_2024", "US_FEDERAL_2025", "UK_2024", "FR_2024", 
--  "NONE", "WEEKENDS_ONLY"]

-- Kalender zusammenführen
LET combined = HOLIDAYS("DE_2024", "AT_2024")    -- DE + AT Feiertage

-- Inline-Feiertage (Betriebsferien)
LET companyHolidays = HOLIDAYS("2024-12-23", "2024-12-27", "2024-12-30")

-- Feiertage in Zeitraum
LET december = HOLIDAYS_BETWEEN("DE_2024", MAKE_DATE(2024,12,1), MAKE_DATE(2024,12,31))
-- [1735084800000, 1735171200000]  // 25.12. und 26.12.

-- Mit WORKDAYS kombinieren
FOR project IN projects
  LET holidays = HOLIDAYS("DE_2024")
  LET workdays = WORKDAYS(project.start, project.deadline, holidays)
  RETURN { project: project.name, workdays }
```

### Built-in Kalender

| Kalender | Region | Jahr | Beschreibung |
|----------|--------|------|--------------|
| `DE_2024` | Deutschland | 2024 | Bundesweite Feiertage |
| `DE_2025` | Deutschland | 2025 | Bundesweite Feiertage |
| `AT_2024` | Österreich | 2024 | Nationale Feiertage |
| `CH_2024` | Schweiz | 2024 | Bundesfeiertage |
| `US_FEDERAL_2024` | USA | 2024 | Federal Holidays |
| `US_FEDERAL_2025` | USA | 2025 | Federal Holidays |
| `UK_2024` | Großbritannien | 2024 | Bank Holidays |
| `FR_2024` | Frankreich | 2024 | Jours fériés |
| `NONE` | - | - | Leerer Kalender |
| `WEEKENDS_ONLY` | - | - | Nur Wochenenden |

**Praxisbeispiel: Lieferzeit-Berechnung**

```aql
LET holidays = HOLIDAYS("DE_2024")

FOR order IN orders
  FILTER order.status == "shipped"
  LET estimatedDelivery = WORKDAYS_ADD(order.shipped_at, 3, holidays)
  LET isLate = estimatedDelivery < NOW() AND order.delivered_at == null
  RETURN {
    orderId: order._key,
    shippedAt: DATE_FORMAT(order.shipped_at, "%d.%m.%Y"),
    estimatedDelivery: DATE_FORMAT(estimatedDelivery, "%d.%m.%Y"),
    isLate,
    customer: order.customer_name
  }
```

**Praxisbeispiel: JSON-Daten aus API verarbeiten**

```aql
-- Externe API-Daten (als JSON-String empfangen)
LET apiResponse = '{"users": [{"name": "Alice"}, {"name": "Bob"}]}'
LET data = JSON(apiResponse)

FOR user IN data.users
  INSERT user INTO users

-- Oder direkt mit DICT
LET config = DICT('{"theme": "dark", "language": "de"}')
RETURN config.theme  -- "dark"
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

## Security-Funktionen

ThemisDB verfügt über ein umfassendes integriertes Security-Modul. Die AQL-Funktionen ermöglichen Validierung, Sanitization und Maskierung direkt in Queries.

### Daten-Validierung

```aql
-- Format-Validierung
FILTER IS_EMAIL(doc.email)           -- E-Mail-Format prüfen
FILTER IS_URL(doc.website)           -- URL-Format prüfen
FILTER IS_UUID(doc._key)             -- UUID-Format prüfen
FILTER IS_IP(doc.ip_address)         -- IP-Adresse prüfen

-- Schema-Validierung (registrierte Schemas)
LET result = VALIDATE(doc, "user_schema")
RETURN result.valid                   -- true/false
RETURN result.errors                  -- ["field: error message", ...]

-- Schnelle Boolean-Prüfung
FILTER IS_VALID(doc, "order_schema")
```

### Input-Sanitization

```aql
-- Daten bereinigen
LET clean = SANITIZE(userInput)

-- Mit Optionen
LET clean = SANITIZE(data, {
  escapeHtml: true,
  escapeSql: true,
  trimStrings: true,
  maxStringLength: 1000
})

-- Einzelnen String bereinigen
LET cleanName = SANITIZE_STRING(userName, {escapeHtml: true})

-- Injection-Prüfung
FILTER NOT HAS_INJECTION(userInput)
```

### Sensitive Daten maskieren

```aql
-- Einzelwert maskieren
RETURN MASK(password, "password")     -- "********"
RETURN MASK(email, "email")           -- "j***@example.com"
RETURN MASK(ccNumber, "credit_card")  -- "************1234"
RETURN MASK(ssn, "ssn")               -- "***-**-1234"
RETURN MASK(phone, "phone")           -- "*******1234"

-- Mehrere Felder maskieren
LET safeUser = MASK_FIELDS(user, {
  password: "password",
  ssn: "ssn",
  creditCard: "credit_card"
})

-- Auto-Erkennung von sensitiven Feldern
LET safeData = AUTO_MASK(userData)    -- Erkennt password, api_key, etc.

-- Felder entfernen
LET publicUser = REDACT(user, ["password", "ssn", "internal_id"])
```

### Daten-Integrität

```aql
-- Checksumme berechnen
LET hash = CHECKSUM(doc)              -- "a1b2c3d4..."

-- Checksumme verifizieren
FILTER VERIFY_CHECKSUM(doc, doc._checksum)

-- Signatur erstellen (HMAC)
LET signature = SIGN(doc, @secretKey)

-- Signatur verifizieren
FILTER VERIFY_SIGNATURE(doc, doc._signature, @secretKey)
```

**Praxisbeispiel: API-Response mit maskierten Daten**

```aql
FOR user IN users
  FILTER user._key == @userId
  
  -- Sensitive Felder maskieren vor Rückgabe
  LET safeUser = REDACT(user, ["password_hash", "api_keys", "internal_notes"])
  LET maskedUser = MASK_FIELDS(safeUser, {
    email: "email",
    phone: "phone"
  })
  
  RETURN maskedUser
```

**Praxisbeispiel: Input-Validierung vor Insert**

```aql
LET userInput = @input

-- Validieren
LET validation = VALIDATE(userInput, "new_user_schema")
FILTER validation.valid

-- Sanitizen
LET clean = SANITIZE(userInput, {trimStrings: true})

-- Injection-Check
FILTER NOT HAS_INJECTION(clean.name)
FILTER NOT HAS_INJECTION(clean.bio)

-- Sicher speichern
INSERT clean INTO users
RETURN NEW
```

---

## Zusammenfassung

ThemisDB AQL bietet:

✅ **~255 Funktionen** in 13 Kategorien  
✅ **Einheitliche Syntax** für alle Datenmodelle  
✅ **Native Multi-Model Queries** (Graph + Vector + Geo + Relational)  
✅ **Vollständige CRS-Unterstützung** (ETRS89, UTM, WGS84, Gauß-Krüger)  
✅ **SQL-kompatible** Aggregation und Window Functions  
✅ **~45 Date-Funktionen** mit Interval-Syntax und Arbeitstagen  
✅ **JSON-Native** ARRAY/DICT Konstruktoren  
✅ **Built-in Feiertags-Kalender** (DE, AT, CH, US, UK, FR)  
✅ **Integrierte Security-Funktionen** (Validation, Sanitization, Masking)  
✅ **Prozess-Mining** aus Event-Daten  
✅ **Keine Vendor Lock-in** bei Query-Sprache

### Neue Features in Version 1.1

| Feature | Beschreibung |
|---------|--------------|
| **Interval-Syntax** | `NOW() - DAYS(7)` statt `DATE_SUBTRACT(DATE_NOW(), 7, "days")` |
| **Arbeitstage** | `WORKDAYS()`, `WORKDAYS_ADD()`, `IS_WORKDAY()` |
| **Holiday-Kalender** | `HOLIDAYS("DE_2024")` mit Built-in Kalendern |
| **JSON-Native** | `ARRAY('[1,2,3]')`, `DICT('{"a":1}')` parsen JSON-Strings |
| **Security-Funktionen** | `VALIDATE()`, `SANITIZE()`, `MASK()`, `REDACT()` |
| **Format-Validierung** | `IS_EMAIL()`, `IS_URL()`, `IS_UUID()`, `IS_IP()` |

**Nächste Schritte:**
- [Installation Guide](./INSTALLATION.md)
- [Multi-Model Architecture](./MULTI_MODEL_ARCHITECTURE.md)
- [Enterprise Analytics](./ENTERPRISE_ANALYTICS.md)
- [API Reference](./API_REFERENCE.md)

---

## Logical-Funktionen

Die Logical-Funktionen bieten Excel-kompatible logische Operationen für Bedingungen und Verzweigungen.

### Grundlegende logische Operationen

#### AND(value1, value2, ...)

Gibt `true` zurück, wenn alle Argumente wahr sind.

```aql
-- Prüfe ob alle Bedingungen erfüllt sind
RETURN AND(age >= 18, hasLicense, !suspended)  -- true/false

-- In FILTER verwenden
FOR user IN users
  FILTER AND(user.active, user.verified, user.age >= 18)
  RETURN user
```

#### OR(value1, value2, ...)

Gibt `true` zurück, wenn mindestens ein Argument wahr ist.

```aql
-- Premium oder VIP Kunde
FILTER OR(customer.isPremium, customer.isVIP)
```

#### NOT(value)

Negiert einen Wert.

```aql
FILTER NOT(user.suspended)
```

#### XOR(value1, value2)

Exklusives Oder - genau ein Wert muss wahr sein.

```aql
-- Entweder Student ODER Rentner, aber nicht beides
FILTER XOR(person.isStudent, person.isRetired)
```

### Bedingte Funktionen

#### IF(condition, trueValue, falseValue)

Gibt einen Wert abhängig von einer Bedingung zurück.

```aql
LET status = IF(order.paid, "Bezahlt", "Ausstehend")
LET discount = IF(customer.isPremium, 0.2, 0.0)
```

#### IFS(condition1, value1, condition2, value2, ...)

Mehrere Bedingungen prüfen und entsprechenden Wert zurückgeben.

```aql
LET grade = IFS(
  score >= 90, "A",
  score >= 80, "B",
  score >= 70, "C",
  score >= 60, "D",
  true, "F"
)
```

#### SWITCH(expression, case1, value1, case2, value2, ..., default)

Switch-Case Logik.

```aql
LET monthName = SWITCH(month,
  1, "Januar",
  2, "Februar",
  3, "März",
  "Unbekannt"
)
```

#### CHOOSE(index, value1, value2, ...)

Wählt einen Wert basierend auf einem Index (1-basiert).

```aql
LET dayName = CHOOSE(dayOfWeek, "Mo", "Di", "Mi", "Do", "Fr", "Sa", "So")
```

### Array-basierte logische Funktionen

#### ALL(array)

Prüft ob alle Elemente wahr sind.

```aql
LET allPassed = ALL([test1.passed, test2.passed, test3.passed])
```

#### ANY(array)

Prüft ob mindestens ein Element wahr ist.

```aql
LET hasWarnings = ANY(checks[*].hasWarning)
```

#### NONE(array)

Prüft ob kein Element wahr ist.

```aql
LET noErrors = NONE(results[*].hasError)
```

### Bedingte Aggregation

#### COUNT_IF(array, operator, value)

Zählt Elemente die eine Bedingung erfüllen.

```aql
LET highScores = COUNT_IF(scores, ">", 80)
LET activeUsers = COUNT_IF(users[*].status, "==", "active")
```

#### SUM_IF(array, operator, value)

Summiert Elemente die eine Bedingung erfüllen.

```aql
LET totalPremium = SUM_IF(orders[*].amount, ">", 100)
```

### Fehlerbehandlung

#### IFERROR(value, fallback)

Gibt fallback zurück wenn value null oder error ist.

```aql
LET safeValue = IFERROR(riskyCalculation, 0)
LET safeName = IFERROR(user.name, "Unbekannt")
```

#### IFNA(value, fallback)

Gibt fallback zurück wenn value null ist (NA = Not Available).

```aql
LET displayName = IFNA(user.nickname, user.fullName)
```

---

## Excel-kompatible Funktionen

Diese Funktionen bieten vertraute Excel-Funktionalität für Benutzer, die von Tabellenkalkulationen kommen.

### Lookup & Reference

#### VLOOKUP(searchValue, table, columnIndex, [rangeLookup])

Vertikale Suche in einer Tabelle.

| Parameter | Typ | Beschreibung |
|-----------|-----|--------------|
| searchValue | any | Der zu suchende Wert |
| table | array | 2D-Array als Tabelle |
| columnIndex | number | Spaltenindex (1-basiert) |
| rangeLookup | boolean | false = exakte Übereinstimmung (Standard) |

```aql
LET employees = [
  ["E001", "Alice", 50000],
  ["E002", "Bob", 60000],
  ["E003", "Carol", 55000]
]

LET salary = VLOOKUP("E002", employees, 3)  -- Ergebnis: 60000
```

#### HLOOKUP(searchValue, table, rowIndex, [rangeLookup])

Horizontale Suche in einer Tabelle.

```aql
LET headers = [
  ["Q1", "Q2", "Q3", "Q4"],
  [1000, 1200, 1100, 1500]
]

LET q3Revenue = HLOOKUP("Q3", headers, 2)  -- Ergebnis: 1100
```

#### INDEX(array, rowNum, [colNum])

Gibt einen Wert aus einem Array zurück.

```aql
LET colors = ["Rot", "Grün", "Blau"]
RETURN INDEX(colors, 2)  -- Ergebnis: "Grün"

-- 2D Array
LET matrix = [[1, 2], [3, 4], [5, 6]]
RETURN INDEX(matrix, 2, 1)  -- Ergebnis: 3
```

#### MATCH(lookupValue, lookupArray, [matchType])

Findet die Position eines Wertes.

```aql
LET products = ["Apple", "Banana", "Cherry"]
RETURN MATCH("Banana", products)  -- Ergebnis: 2
```

### Text-Funktionen (Excel-Stil)

#### PROPER(text)

Wandelt Text in Titel-Case um.

```aql
RETURN PROPER("hello world")     -- "Hello World"
RETURN PROPER("JOHN DOE")        -- "John Doe"
RETURN PROPER("mÜNCHEN")         -- "München"
```

#### SUBSTITUTE(text, oldText, newText, [instanceNum])

Ersetzt Text (ohne Regex).

```aql
RETURN SUBSTITUTE("Mr Blue", "Blue", "Green")    -- "Mr Green"
RETURN SUBSTITUTE("a-a-a", "a", "b", 2)          -- "a-b-a" (nur 2. Vorkommen)
```

#### REPT(text, times)

Wiederholt Text n-mal.

```aql
RETURN REPT("*", 5)     -- "*****"
RETURN REPT("Ha", 3)    -- "HaHaHa"
```

#### EXACT(text1, text2)

Vergleicht Texte (case-sensitiv).

```aql
RETURN EXACT("Hello", "Hello")   -- true
RETURN EXACT("Hello", "hello")   -- false
```

#### TEXT(value, format)

Formatiert einen Wert als Text.

```aql
RETURN TEXT(1234.567, "0.00")           -- "1234.57"
RETURN TEXT(0.75, "0%")                 -- "75%"
RETURN TEXT(DATE_NOW(), "YYYY-MM-DD")   -- "2024-12-01"
```

#### VALUE(text)

Konvertiert Text in eine Zahl.

```aql
RETURN VALUE("123.45")   -- 123.45
RETURN VALUE("1,234")    -- 1234
```

### Statistische Funktionen

#### SUMPRODUCT(array1, array2, ...)

Multipliziert Arrays elementweise und summiert.

```aql
LET prices = [10, 20, 30]
LET quantities = [5, 3, 2]

RETURN SUMPRODUCT(prices, quantities)  -- 10*5 + 20*3 + 30*2 = 170
```

#### AVERAGEIF(range, criteria, [avgRange])

Durchschnitt mit Bedingung.

```aql
LET sales = [100, 200, 150, 300, 50]
RETURN AVERAGEIF(sales, ">", 100)  -- (200 + 150 + 300) / 3 = 216.67
```

#### RANK(number, array, [order])

Rang eines Wertes in einer Liste.

```aql
LET scores = [80, 90, 70, 100, 85]
RETURN RANK(90, scores)        -- 2 (2. höchster, absteigend)
RETURN RANK(90, scores, 1)     -- 4 (4. niedrigster, aufsteigend)
```

#### LARGE(array, k)

Gibt den k-größten Wert zurück.

```aql
LET values = [5, 2, 8, 1, 9, 3]
RETURN LARGE(values, 1)   -- 9 (größter)
RETURN LARGE(values, 2)   -- 8 (zweitgrößter)
RETURN LARGE(values, 3)   -- 5 (drittgrößter)
```

#### SMALL(array, k)

Gibt den k-kleinsten Wert zurück.

```aql
LET values = [5, 2, 8, 1, 9, 3]
RETURN SMALL(values, 1)   -- 1 (kleinster)
RETURN SMALL(values, 2)   -- 2 (zweitkleinster)
```

#### MODE(array)

Häufigster Wert (Modus).

```aql
LET ratings = [4, 5, 4, 3, 4, 5, 4]
RETURN MODE(ratings)  -- 4 (kommt am häufigsten vor)
```

### Math-Funktionen (Excel-Stil)

#### PRODUCT(value1, value2, ...)

Produkt aller Werte.

```aql
RETURN PRODUCT(2, 3, 4)     -- 24
RETURN PRODUCT([2, 3, 4])   -- 24
```

#### FACT(number)

Fakultät.

```aql
RETURN FACT(5)   -- 120 (5! = 5*4*3*2*1)
RETURN FACT(0)   -- 1
```

#### MOD(number, divisor)

Rest der Division (Modulo).

```aql
RETURN MOD(17, 5)   -- 2
RETURN MOD(10, 3)   -- 1
```

#### QUOTIENT(numerator, denominator)

Ganzzahliger Anteil der Division.

```aql
RETURN QUOTIENT(17, 5)   -- 3
RETURN QUOTIENT(10, 3)   -- 3
```

### Informations-Funktionen

#### ISERROR(value)

Prüft ob Wert ein Fehler ist.

```aql
RETURN ISERROR(1/0)      -- true
RETURN ISERROR(null)     -- true
RETURN ISERROR(42)       -- false
```

#### ISBLANK(value)

Prüft ob Wert leer ist.

```aql
RETURN ISBLANK("")       -- true
RETURN ISBLANK(null)     -- true
RETURN ISBLANK("text")   -- false
```

#### ISTEXT(value)

Prüft ob Wert ein Text ist.

```aql
RETURN ISTEXT("hello")   -- true
RETURN ISTEXT(123)       -- false
```

#### ISNUMBER(value)

Prüft ob Wert eine Zahl ist.

```aql
RETURN ISNUMBER(123)     -- true
RETURN ISNUMBER(3.14)    -- true
RETURN ISNUMBER("123")   -- false
```

#### ISLOGICAL(value)

Prüft ob Wert ein Boolean ist.

```aql
RETURN ISLOGICAL(true)   -- true
RETURN ISLOGICAL(false)  -- true
RETURN ISLOGICAL(1)      -- false
```

#### TYPE(value)

Gibt den Typ als Nummer zurück (Excel-kompatibel).

| Rückgabewert | Typ |
|--------------|-----|
| 1 | Number |
| 2 | Text |
| 4 | Boolean |
| 16 | Error/Null |
| 64 | Array |
| 128 | Object |

```aql
RETURN TYPE(123)         -- 1
RETURN TYPE("hello")     -- 2
RETURN TYPE(true)        -- 4
RETURN TYPE([1, 2, 3])   -- 64
```

#### N(value)

Konvertiert Wert zu Zahl.

```aql
RETURN N(true)    -- 1
RETURN N(false)   -- 0
RETURN N(123)     -- 123
RETURN N("text")  -- 0
```

#### T(value)

Gibt Text zurück, wenn Wert Text ist, sonst leerer String.

```aql
RETURN T("hello")   -- "hello"
RETURN T(123)       -- ""
RETURN T(true)      -- ""
```

### Finanz-Funktionen

#### PMT(rate, nper, pv, [fv], [type])

Berechnet die periodische Zahlung für ein Darlehen.

| Parameter | Typ | Beschreibung |
|-----------|-----|--------------|
| rate | number | Zinssatz pro Periode |
| nper | number | Anzahl der Perioden |
| pv | number | Barwert (Darlehensbetrag) |
| fv | number | Endwert (Standard: 0) |
| type | number | 0 = Ende der Periode, 1 = Anfang |

```aql
-- Monatliche Rate für 200.000€ Hypothek, 6% p.a., 30 Jahre
LET monthlyPayment = PMT(0.06/12, 360, 200000)
-- Ergebnis: -1199.10 (negative Zahl = Auszahlung)
```

#### FV(rate, nper, pmt, [pv], [type])

Berechnet den Endwert einer Investition.

```aql
-- Endwert bei 100€/Monat, 5% p.a., 10 Jahre
LET futureValue = FV(0.05/12, 120, -100)
-- Ergebnis: ~15,528€
```

#### PV(rate, nper, pmt, [fv], [type])

Berechnet den Barwert.

```aql
-- Barwert einer Rente: 1000€/Monat, 5% p.a., 20 Jahre
LET presentValue = PV(0.05/12, 240, -1000)
-- Ergebnis: ~151,525€
```

#### NPV(rate, cashflow1, cashflow2, ...)

Berechnet den Nettobarwert (Net Present Value).

```aql
LET cashflows = [-100000, 30000, 40000, 50000, 60000]
LET npv = NPV(0.10, cashflows)
-- Positiver NPV = rentable Investition
```

---

## Benchmarks

Die Performance der AQL-Funktionen wurde mit Google Benchmark gemessen.

### Benchmark-Ergebnisse (Referenzsystem)

| Funktion | Operationen/s | Latenz (ns) | Komplexität |
|----------|--------------|-------------|-------------|
| LENGTH | 50M | 20 | O(1) |
| CONCAT | 10M | 100 | O(n) |
| REGEX_TEST | 2M | 500 | O(n) |
| LEVENSHTEIN_DISTANCE | 500K | 2000 | O(n*m) |
| SUM (1000 Elemente) | 1M | 1000 | O(n) |
| UNIQUE (1000 Elemente) | 100K | 10000 | O(n log n) |
| SORTED (1000 Elemente) | 50K | 20000 | O(n log n) |
| GEO_DISTANCE | 5M | 200 | O(1) |
| ST_TRANSFORM | 500K | 2000 | O(1) |
| COSINE_SIMILARITY (1000 Dim) | 1M | 1000 | O(n) |
| SHORTEST_PATH (100 Nodes) | 10K | 100000 | O(V + E) |
| PAGERANK | 1K | 1000000 | O(k*E) |
| VLOOKUP (1000 Rows) | 100K | 10000 | O(n) |
| PMT | 10M | 100 | O(1) |

### Benchmark ausführen

```bash
# Build benchmarks
cmake -DBUILD_BENCHMARKS=ON ..
make bench_aql_functions

# Ausführen
./bench_aql_functions --benchmark_format=json > results.json
```

### Performance-Tipps

1. **Indizierte Lookups** statt VLOOKUP bei großen Datenmengen
2. **Vektorisierte Operationen** für Array-Funktionen nutzen
3. **CRS-Transformationen** cachen wenn möglich
4. **Graph-Algorithmen** profitieren von Indexierung


---

## Security-Funktionen (Erweiterte Dokumentation)

Die Security-Funktionen bieten umfassende Validierung, Sanitization und Maskierung für sichere Datenverarbeitung. Sie integrieren sich mit dem bestehenden ThemisDB Security-Modul.

### Validierung

#### IS_EMAIL(str)

Validiert E-Mail-Adressen nach RFC 5322 (vereinfacht).

```aql
-- E-Mail-Validierung
FOR user IN users
  LET valid = IS_EMAIL(user.email)
  RETURN { email: user.email, valid }

-- Beispiele
IS_EMAIL("user@example.com")        → true
IS_EMAIL("user.name@sub.domain.com") → true
IS_EMAIL("invalid")                  → false
IS_EMAIL("@missing.local")           → false
```

#### IS_URL(str)

Validiert URLs (http, https, ftp).

```aql
-- URL-Validierung
IS_URL("https://example.com/path?query=1")  → true
IS_URL("ftp://files.example.com")           → true
IS_URL("not-a-url")                         → false
IS_URL("javascript:alert(1)")               → false
```

#### IS_UUID(str)

Validiert UUIDs (Version 1-5).

```aql
IS_UUID("550e8400-e29b-41d4-a716-446655440000") → true
IS_UUID("550e8400-e29b-61d4-a716-446655440000") → false (Version 6 nicht gültig)
IS_UUID("invalid")                               → false
```

#### IS_IP(str, version?)

Validiert IP-Adressen (IPv4/IPv6).

```aql
-- IPv4
IS_IP("192.168.1.1")           → true
IS_IP("10.0.0.1")              → true
IS_IP("192.168.1.1", 4)        → true

-- IPv6
IS_IP("::1")                   → true
IS_IP("::1", 6)                → true
IS_IP("192.168.1.1", 6)        → false

-- Ungültig
IS_IP("999.999.999.999")       → false
IS_IP("not-an-ip")             → false
```

#### IS_PHONE(str, countryCode?)

Validiert Telefonnummern.

```aql
IS_PHONE("+49 123 456789")     → true
IS_PHONE("0123456789")         → true
IS_PHONE("+1 555 123 4567")    → true
IS_PHONE("abc")                → false
```

#### IS_IBAN(str)

Validiert IBAN mit Prüfsumme (Mod 97).

```aql
IS_IBAN("DE89370400440532013000") → true
IS_IBAN("DE89 3704 0044 0532 0130 00") → true (Leerzeichen erlaubt)
IS_IBAN("DE00000000000000000000") → false (ungültige Prüfsumme)
```

#### IS_CREDIT_CARD(str)

Validiert Kreditkartennummern mit Luhn-Algorithmus.

```aql
IS_CREDIT_CARD("4532015112830366")     → true (Visa)
IS_CREDIT_CARD("5425233430109903")     → true (Mastercard)
IS_CREDIT_CARD("1234 5678 9012 3456")  → false
```

### Sanitization

#### SANITIZE(str, type?)

Bereinigt Eingaben für verschiedene Kontexte.

| Type | Beschreibung |
|------|--------------|
| "html" | HTML-Entitäten escapen (Standard) |
| "sql" | SQL-Injection verhindern |
| "json" | JSON-Sonderzeichen escapen |
| "filename" | Sichere Dateinamen |

```aql
-- HTML
SANITIZE("<script>alert('xss')</script>", "html")
→ "&lt;script&gt;alert(&#39;xss&#39;)&lt;/script&gt;"

-- SQL
SANITIZE("O'Brien", "sql")
→ "O''Brien"

-- JSON
SANITIZE("Line1\nLine2", "json")
→ "Line1\\nLine2"

-- Filename
SANITIZE("../../../etc/passwd", "filename")
→ "etcpasswd"
```

#### HAS_INJECTION(str, type?)

Erkennt potenzielle Injection-Angriffe.

| Type | Erkannte Muster |
|------|-----------------|
| "sql" | DROP, DELETE, UNION SELECT, --, etc. |
| "xss" | \<script\>, javascript:, onerror=, etc. |
| "path" | ../, ..\, %2e%2e |
| "cmd" | ;, |, \`, $(), &&, || |
| "all" | Alle Typen (Standard) |

```aql
-- SQL Injection
HAS_INJECTION("1'; DROP TABLE users--", "sql")  → true
HAS_INJECTION("' OR '1'='1", "sql")             → true
HAS_INJECTION("normal input", "sql")             → false

-- XSS
HAS_INJECTION("<script>alert(1)</script>", "xss") → true
HAS_INJECTION("javascript:void(0)", "xss")        → true

-- Path Traversal
HAS_INJECTION("../../../etc/passwd", "path")      → true

-- Praxis: Input-Validierung
FOR input IN user_inputs
  LET unsafe = HAS_INJECTION(input.value)
  FILTER unsafe == false
  RETURN input
```

### Maskierung

#### MASK(str, start?, end?, char?)

Maskiert Zeichen in einem String.

```aql
-- Standard: Alles maskieren
MASK("secret")                → "******"

-- Zeige erste und letzte Zeichen
MASK("1234567890", 0, 4)     → "******7890"
MASK("secret", 1, 1)          → "s****t"

-- Anderes Maskierungszeichen
MASK("password", 2, 2, '#')   → "pa####rd"
```

#### MASK_EMAIL(email)

Maskiert E-Mail-Adressen intelligent.

```aql
MASK_EMAIL("john.doe@example.com")
→ "j******e@e*****e.com"

MASK_EMAIL("a@b.de")
→ "a@b.de" (zu kurz zum Maskieren)
```

#### MASK_CREDIT_CARD(card)

Zeigt nur die letzten 4 Ziffern.

```aql
MASK_CREDIT_CARD("4532015112830366")
→ "************0366"

MASK_CREDIT_CARD("4532 0151 1283 0366")
→ "************0366"
```

#### MASK_IBAN(iban)

Zeigt Ländercode und letzte 4 Zeichen.

```aql
MASK_IBAN("DE89370400440532013000")
→ "DE**************3000"
```

### Hashing & Prüfsummen

#### HASH(str, algorithm?)

Berechnet einen Hash-Wert (nicht-kryptografisch).

```aql
-- FNV-1a (Standard, schnell)
HASH("password")              → "af63bd4c8601b7df"

-- DJB2
HASH("password", "djb2")      → "7c9e6679350367a3"

-- Praxis: Deduplizierung
FOR doc IN documents
  LET hash = HASH(doc.content)
  COLLECT h = hash INTO grouped
  RETURN { hash: h, count: LENGTH(grouped) }
```

#### CHECKSUM(data, algorithm?)

Berechnet eine Prüfsumme.

```aql
-- CRC32 (Standard)
CHECKSUM("hello world")       → 222957957

-- Adler32
CHECKSUM("hello world", "adler32") → 436929629

-- Praxis: Datenintegrität
FOR file IN files
  LET stored = file.checksum
  LET current = CHECKSUM(file.content)
  RETURN { name: file.name, valid: stored == current }
```

### Praxisbeispiele

#### Sichere Benutzerregistrierung

```aql
LET input = {
  email: @email,
  phone: @phone,
  password: @password
}

-- Validierung
LET validEmail = IS_EMAIL(input.email)
LET validPhone = IS_PHONE(input.phone)
LET noInjection = NOT HAS_INJECTION(input.email)

FILTER validEmail AND validPhone AND noInjection

-- Registrierung mit maskierten Daten im Log
INSERT {
  email: input.email,
  phone: input.phone,
  _log: {
    maskedEmail: MASK_EMAIL(input.email),
    maskedPhone: MASK(input.phone, 0, 4)
  }
} INTO users
```

#### GDPR-konforme API-Response

```aql
FOR customer IN customers
  RETURN {
    id: customer._key,
    email: MASK_EMAIL(customer.email),
    phone: MASK(customer.phone, 0, 4),
    iban: MASK_IBAN(customer.iban),
    creditCard: MASK_CREDIT_CARD(customer.creditCard)
  }
```

#### Input-Sanitization für Suchfunktion

```aql
LET searchTerm = @query

-- Prüfe auf Injection
LET isSafe = NOT HAS_INJECTION(searchTerm)

FILTER isSafe

-- Sanitize für sichere Verwendung
LET cleanTerm = SANITIZE(searchTerm, "html")

FOR doc IN FULLTEXT(products, "description", cleanTerm)
  RETURN doc
```

---

## Changelog

### Version 1.3 (aktuell)
- **Security-Funktionen erweitert**: IS_IBAN, IS_CREDIT_CARD, IS_PHONE, IS_IP hinzugefügt
- **Maskierung**: MASK_EMAIL, MASK_CREDIT_CARD, MASK_IBAN
- **Injection-Erkennung**: HAS_INJECTION mit sql, xss, path, cmd Typen
- **Hashing**: HASH (FNV-1a, DJB2), CHECKSUM (CRC32, Adler32)
- **Insgesamt**: ~355 Funktionen in 13 Kategorien

### Version 1.2
- Collection-Funktionen (ARRAY, DICT, JSON, HOLIDAYS)
- Logical-Funktionen (AND, OR, IF, SWITCH, ALL, ANY)
- Excel-kompatible Funktionen (VLOOKUP, SUMPRODUCT, PMT)

### Version 1.1
- Date-Funktionen erweitert (NOW, TODAY, INTERVAL, WORKDAYS)
- CRS-Transformationen (ST_TRANSFORM, ETRS89/UTM)

### Version 1.0
- Initiale Version mit ~210 Funktionen

---

## CRUD-Operationen in Prozessen

ThemisDB ermöglicht die Modellierung und Ausführung von Geschäftsprozessen mit vollständiger Datenmanipulation. Dieser Abschnitt zeigt, wie CRUD-Operationen (Create, Read, Update, Delete) in typischen Verwaltungsprozessen implementiert werden.

### Grundprinzipien

1. **Prozess als Graph**: Jeder Prozess wird als Graph mit Knoten (Aktivitäten) und Kanten (Übergänge) modelliert
2. **Dokumente als Prozessinstanzen**: Jede Prozessinstanz ist ein Dokument mit Status und Variablen
3. **Transaktionale Updates**: Statusänderungen sind atomar und nachvollziehbar
4. **Audit-Trail**: Alle Änderungen werden protokolliert

### Beispiel: Antragsverfahren (Verwaltungsprozess)

#### Schritt 1: Antrag einreichen (CREATE)

```aql
-- Neuen Antrag erstellen
LET antrag = {
  _key: UUID(),
  typ: "bauantrag",
  status: "eingegangen",
  antragsteller: @personId,
  eingangsdatum: NOW(),
  daten: {
    grundstueck: @grundstueckId,
    bauvorhaben: @bauvorhaben,
    geschosszahl: @geschosse
  },
  dokumente: [],
  sachbearbeiter: null,
  _created: NOW(),
  _history: [{
    zeitpunkt: NOW(),
    aktion: "eingereicht",
    benutzer: @benutzer,
    status: "eingegangen"
  }]
}

INSERT antrag INTO antraege
RETURN NEW
```

#### Schritt 2: Sachbearbeiter zuweisen (UPDATE)

```aql
-- Automatische Zuweisung nach Arbeitslast
LET freieSachbearbeiter = (
  FOR sb IN sachbearbeiter
    FILTER sb.abteilung == "bauamt"
    LET offeneAntraege = LENGTH(
      FOR a IN antraege
        FILTER a.sachbearbeiter == sb._key
        FILTER a.status NOT IN ["abgeschlossen", "abgelehnt"]
        RETURN 1
    )
    SORT offeneAntraege ASC
    LIMIT 1
    RETURN sb._key
)

UPDATE @antragId WITH {
  status: "in_bearbeitung",
  sachbearbeiter: FIRST(freieSachbearbeiter),
  _history: PUSH(OLD._history, {
    zeitpunkt: NOW(),
    aktion: "zugewiesen",
    benutzer: "system",
    status: "in_bearbeitung",
    sachbearbeiter: FIRST(freieSachbearbeiter)
  })
} IN antraege
RETURN NEW
```

#### Schritt 3: Dokumente hinzufügen (UPDATE)

```aql
-- Dokument zum Antrag hinzufügen
LET dokument = {
  id: UUID(),
  typ: @dokumentTyp,
  name: SANITIZE(@filename, "filename"),
  hochgeladen: NOW(),
  von: @benutzer,
  pfad: @speicherpfad,
  mime: MIME_TYPE(@filename),
  checksum: CHECKSUM(@inhalt)
}

-- Validierung
FILTER IS_VALID(@dokumentTyp, ["lageplan", "bauplan", "statik", "brandschutz"])
FILTER NOT HAS_INJECTION(@filename, "path")

UPDATE @antragId WITH {
  dokumente: PUSH(OLD.dokumente, dokument),
  _history: PUSH(OLD._history, {
    zeitpunkt: NOW(),
    aktion: "dokument_hinzugefuegt",
    benutzer: @benutzer,
    dokument: dokument.name
  })
} IN antraege
RETURN NEW
```

#### Schritt 4: Prüfung durchführen (READ + UPDATE)

```aql
-- Antrag lesen und Prüfungsergebnis eintragen
FOR antrag IN antraege
  FILTER antrag._key == @antragId
  
  -- Vollständigkeitsprüfung
  LET erforderlicheDokumente = ["lageplan", "bauplan", "statik"]
  LET vorhandeneDokumente = antrag.dokumente[*].typ
  LET fehlend = MINUS(erforderlicheDokumente, vorhandeneDokumente)
  LET vollstaendig = LENGTH(fehlend) == 0
  
  -- Geo-Prüfung: Liegt Grundstück im Bebauungsplan-Gebiet?
  LET grundstueck = DOCUMENT("grundstuecke", antrag.daten.grundstueck)
  LET imBebauungsplan = (
    FOR bp IN bebauungsplaene
      FILTER ST_CONTAINS(bp.gebiet, grundstueck.geometrie)
      RETURN bp
  )
  
  -- Ergebnis speichern
  UPDATE antrag WITH {
    pruefung: {
      vollstaendig: vollstaendig,
      fehlendeUnterlagen: fehlend,
      bebauungsplan: FIRST(imBebauungsplan),
      geprueft: NOW(),
      pruefer: @benutzer
    },
    status: vollstaendig ? "geprueft" : "unvollstaendig",
    _history: PUSH(antrag._history, {
      zeitpunkt: NOW(),
      aktion: "geprueft",
      benutzer: @benutzer,
      ergebnis: vollstaendig ? "vollstaendig" : "unvollstaendig"
    })
  } IN antraege
  RETURN NEW
```

#### Schritt 5: Genehmigung/Ablehnung (UPDATE)

```aql
-- Entscheidung treffen
UPDATE @antragId WITH {
  status: @entscheidung,  -- "genehmigt" oder "abgelehnt"
  entscheidung: {
    datum: NOW(),
    entscheider: @benutzer,
    begruendung: @begruendung,
    auflagen: @auflagen
  },
  _history: PUSH(OLD._history, {
    zeitpunkt: NOW(),
    aktion: "entschieden",
    benutzer: @benutzer,
    status: @entscheidung,
    begruendung: @begruendung
  })
} IN antraege
RETURN NEW
```

#### Schritt 6: Archivierung (UPDATE + CREATE)

```aql
-- Antrag archivieren
LET antrag = DOCUMENT("antraege", @antragId)

-- Archiv-Eintrag erstellen
INSERT {
  _key: antrag._key,
  originalDaten: antrag,
  archiviertAm: NOW(),
  aufbewahrungsbis: DATE_ADD(NOW(), YEARS(10)),
  kategorie: "bauantraege"
} INTO archiv

-- Original-Status aktualisieren
UPDATE @antragId WITH {
  status: "archiviert",
  archivReferenz: antrag._key,
  _history: PUSH(OLD._history, {
    zeitpunkt: NOW(),
    aktion: "archiviert",
    benutzer: @benutzer
  })
} IN antraege

RETURN { archiviert: true, referenz: antrag._key }
```

### Prozess-übergreifende Abfragen

#### Dashboard: Offene Anträge nach Status

```aql
FOR antrag IN antraege
  FILTER antrag.status NOT IN ["abgeschlossen", "archiviert"]
  COLLECT status = antrag.status WITH COUNT INTO anzahl
  RETURN { status, anzahl }
```

#### Bearbeitungszeiten analysieren

```aql
FOR antrag IN antraege
  FILTER antrag.status == "genehmigt"
  LET eingangsdatum = antrag.eingangsdatum
  LET entscheidungsdatum = antrag.entscheidung.datum
  LET bearbeitungstage = WORKDAYS(eingangsdatum, entscheidungsdatum, HOLIDAYS("DE_2024"))
  COLLECT typ = antrag.typ
  AGGREGATE avgTage = AVG(bearbeitungstage), maxTage = MAX(bearbeitungstage)
  RETURN { typ, avgTage, maxTage }
```

#### Sachbearbeiter-Leistungsübersicht

```aql
FOR sb IN sachbearbeiter
  LET antraege = (
    FOR a IN antraege
      FILTER a.sachbearbeiter == sb._key
      FILTER a.entscheidung != null
      RETURN a
  )
  LET bearbeitungszeiten = (
    FOR a IN antraege
      LET tage = WORKDAYS(a.eingangsdatum, a.entscheidung.datum, HOLIDAYS("DE_2024"))
      RETURN tage
  )
  RETURN {
    sachbearbeiter: sb.name,
    abgeschlossen: LENGTH(antraege),
    avgBearbeitungszeit: AVG(bearbeitungszeiten),
    genehmigungsquote: LENGTH(antraege[* FILTER CURRENT.status == "genehmigt"]) / LENGTH(antraege)
  }
```

---

## FAQ - Erweitert

### Grundlagen

#### F: Was ist der Unterschied zwischen AQL und SQL?

**A:** AQL ist eine **Multi-Model-Abfragesprache**, die SQL-ähnliche Syntax mit Graph-, Vector- und Geo-Operationen kombiniert:

| Aspekt | SQL | AQL |
|--------|-----|-----|
| Iteration | `SELECT ... FROM table` | `FOR doc IN collection` |
| Filter | `WHERE column = value` | `FILTER doc.field == value` |
| Joins | `JOIN table2 ON ...` | Graph-Traversierung oder `FOR ... IN` |
| Aggregation | `GROUP BY` | `COLLECT ... AGGREGATE` |
| Updates | `UPDATE table SET ...` | `UPDATE doc WITH {...} IN collection` |
| Multi-Model | Nicht möglich | Graph + Vector + Geo in einer Query |

#### F: Wie unterscheidet sich `[...]` von `ARRAY(...)`?

**A:** Beide erzeugen Arrays, aber mit unterschiedlichen Fähigkeiten:

```aql
-- Native Syntax (für statische Werte)
LET arr = [1, 2, 3]

-- ARRAY() Funktion (für JSON-Parsing)
LET arr = ARRAY('[1, 2, 3]')  -- parst JSON-String!
LET arr = ARRAY(singleValue)   -- Typ-Coercion
```

Empfehlung: Native Syntax für Literale, Funktionen für dynamische Konstruktion.

#### F: Welche Funktionen haben Aliase?

**A:** Excel- und SQL-kompatible Aliase:

| Alias | Ziel-Funktion | Herkunft |
|-------|---------------|----------|
| CEILING | CEIL | Excel |
| CONCATENATE | CONCAT | Excel |
| LEN | LENGTH | Excel/SQL |
| MID | SUBSTRING | Excel |
| LCASE | LOWER | SQL |
| UCASE | UPPER | SQL |
| POWER | POW | SQL |
| OBJECT | DICT | Alternative |

### Prozesse und CRUD

#### F: Wie implementiere ich Transaktionen?

**A:** ThemisDB unterstützt ACID-Transaktionen:

```aql
-- Transaktion mit mehreren Operationen
LET transfer = (
  -- Abbuchung
  UPDATE "konto_a" WITH { saldo: OLD.saldo - @betrag } IN konten
  
  -- Gutschrift
  UPDATE "konto_b" WITH { saldo: OLD.saldo + @betrag } IN konten
)
RETURN { success: true }
```

Bei Fehlern wird die gesamte Transaktion zurückgerollt.

#### F: Wie erstelle ich einen Audit-Trail?

**A:** Nutzen Sie das `_history`-Feld-Pattern:

```aql
UPDATE docId WITH {
  feldname: neuerWert,
  _history: PUSH(OLD._history, {
    zeitpunkt: NOW(),
    feld: "feldname",
    alterWert: OLD.feldname,
    neuerWert: neuerWert,
    benutzer: @benutzer,
    grund: @aenderungsgrund
  })
} IN collection
```

#### F: Wie validiere ich Eingabedaten vor dem INSERT?

**A:** Kombinieren Sie Validierungsfunktionen:

```aql
LET validierung = {
  emailGueltig: IS_EMAIL(@email),
  telefonGueltig: IS_PHONE(@telefon),
  keineInjection: NOT HAS_INJECTION(CONCAT(@email, @name), "sql"),
  pflichtfelderVorhanden: @name != null AND @email != null
}

LET alleGueltig = ALL(VALUES(validierung))

FILTER alleGueltig

INSERT { ... } INTO collection
RETURN { success: true }
```

#### F: Wie setze ich Soft-Delete um?

**A:** Markieren statt löschen:

```aql
-- Soft-Delete
UPDATE docId WITH {
  _deleted: true,
  _deletedAt: NOW(),
  _deletedBy: @benutzer
} IN collection

-- Normale Abfragen ignorieren gelöschte
FOR doc IN collection
  FILTER doc._deleted != true
  RETURN doc

-- Gelöschte wiederherstellen
UPDATE docId WITH {
  _deleted: null,
  _deletedAt: null,
  _deletedBy: null,
  _restoredAt: NOW(),
  _restoredBy: @benutzer
} IN collection
```

### Performance

#### F: Wie optimiere ich langsame Abfragen?

**A:** Befolgen Sie diese Regeln:

1. **Indizes nutzen**: FILTER-Bedingungen auf indizierte Felder
2. **Früh filtern**: FILTER vor aufwändigen Operationen
3. **LIMIT verwenden**: Ergebnismenge begrenzen
4. **Projektionen**: Nur benötigte Felder zurückgeben
5. **Subqueries vermeiden**: Wenn möglich durch JOINs ersetzen

```aql
-- Schlecht
FOR doc IN large_collection
  LET related = (FOR r IN other FILTER r.parent == doc._key RETURN r)
  RETURN { doc, related }

-- Besser
FOR doc IN large_collection
  FOR related IN other
    FILTER related.parent == doc._key
    COLLECT parentDoc = doc INTO relatedDocs
    RETURN { doc: parentDoc, related: relatedDocs[*].related }
```

#### F: Welche Funktionen können Indizes nutzen?

**A:** Index-fähige Funktionen:

| Funktion | Index-Typ | Beschreibung |
|----------|-----------|--------------|
| GEO_DISTANCE | Geo-Index | Räumliche Suche |
| GEO_CONTAINS | Geo-Index | Enthaltensein |
| FULLTEXT | Volltext-Index | Textsuche |
| SIMILARITY | Vector-Index | Ähnlichkeitssuche |
| DOCUMENT | Primary-Key | Direktzugriff |

### Sicherheit

#### F: Wie schütze ich vor SQL-Injection in AQL?

**A:** ThemisDB verwendet Bind-Parameter:

```aql
-- NIEMALS so (unsicher!)
FOR doc IN collection
  FILTER doc.name == "${userInput}"  -- GEFÄHRLICH!

-- IMMER so (sicher)
FOR doc IN collection
  FILTER doc.name == @userInput  -- Bind-Parameter
```

Zusätzlich: `HAS_INJECTION()` zur Validierung.

#### F: Wie maskiere ich sensible Daten für Logs?

**A:** Nutzen Sie die MASK-Funktionen:

```aql
LET logEntry = {
  email: MASK_EMAIL(user.email),        -- u***@example.com
  kreditkarte: MASK_CREDIT_CARD(cc),     -- ****-****-****-1234
  iban: MASK_IBAN(iban),                 -- DE**************4321
  telefon: MASK(telefon, 0, 4)           -- +49 *** ****4567
}
INSERT logEntry INTO audit_log
```

### Migration

#### F: Wie migriere ich von ArangoDB?

**A:** AQL-Syntax ist weitgehend kompatibel. Hauptunterschiede:

```aql
-- ArangoDB                    -- ThemisDB
DOCUMENT(collection, key)      -- DOCUMENT(collection, key)  ✓
FOR v IN 1..3 OUTBOUND         -- FOR v IN 1..3 OUTBOUND     ✓
COLLECT ... INTO groups        -- COLLECT ... INTO groups    ✓

-- Neue ThemisDB-Features
ST_TRANSFORM(geom, 25832)      -- CRS-Transformation
SIMILARITY(vec1, vec2, k)      -- Vector-Suche
WORKDAYS(start, end, holidays) -- Arbeitstage
HOLIDAYS("DE_2024")            -- Feiertags-Kalender
```

#### F: Wie migriere ich von Neo4j/Cypher?

**A:** Syntax-Übersetzung:

```cypher
-- Cypher
MATCH (p:Person)-[:KNOWS]->(f:Person)
WHERE p.name = 'Alice'
RETURN f.name
```

```aql
-- AQL
FOR p IN persons
  FILTER p.name == 'Alice'
  FOR f IN 1..1 OUTBOUND p knows
    RETURN f.name
```

#### F: Wie migriere ich von MongoDB?

**A:** Aggregation-Pipeline zu AQL:

```javascript
// MongoDB
db.orders.aggregate([
  { $match: { status: "completed" } },
  { $group: { _id: "$customerId", total: { $sum: "$amount" } } },
  { $sort: { total: -1 } }
])
```

```aql
-- AQL
FOR order IN orders
  FILTER order.status == "completed"
  COLLECT customerId = order.customerId
  AGGREGATE total = SUM(order.amount)
  SORT total DESC
  RETURN { customerId, total }
```
