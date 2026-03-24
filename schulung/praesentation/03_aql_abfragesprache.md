---
marp: true
theme: default
paginate: true
backgroundColor: '#ffffff'
header: 'ThemisDB Schulung'
footer: '© ThemisDB – Modul 3: AQL Abfragesprache'
style: |
  section {
    font-family: 'Segoe UI', Arial, sans-serif;
  }
  h1 { color: #1a73e8; }
  h2 { color: #333; border-bottom: 2px solid #1a73e8; padding-bottom: 8px; }
  code { background: #f5f5f5; padding: 2px 6px; border-radius: 4px; }
  pre { background: #1e1e1e; color: #d4d4d4; }
---

# ThemisDB
## Modul 3: AQL — Advanced Query Language

**Schulungsversion 1.0 · Niveau: Einsteiger bis Fortgeschritten**

---

## Agenda

1. Was ist AQL?
2. Grundlegende Syntax & Klauseln
3. CRUD-Operationen
4. Filter, Sort, Limit
5. Joins & Subqueries
6. Aggregationen & COLLECT
7. Graph-Traversierungen
8. Vektor- & Volltextsuche
9. LLM-Erweiterungen
10. DDL — Collections & Indizes

---

## Was ist AQL?

**AQL** (Advanced Query Language) ist ThemisDBs deklarative Abfragesprache für alle Datenmodelle.

**Inspiriert von**: ArangoDB AQL, SQL, SPARQL

```aql
// Eine Sprache für alle Modelle:
FOR user IN users                     -- Relational/Dokument
  FOR v IN OUTBOUND user follows      -- Graph
  LET sim = COSINE_SIMILARITY(...)    -- Vektor
  FILTER user.name LIKE "%Anna%"      -- Volltextsuche
  SORT user.created DESC
  LIMIT 0, 20
  RETURN { user, connections: v }
```

**Wichtige Eigenschaften**:
- Deklarativ (WAS, nicht WIE)
- Typsicher mit optionaler Schemavalidierung
- Vollständige ACID-Unterstützung

---

## Grundlegende Syntax — Kern-Klauseln

| Klausel | Bedeutung |
|---|---|
| `FOR x IN collection` | Iteration über Collection |
| `LET x = expr` | Variable definieren |
| `FILTER condition` | Filtern |
| `SORT field ASC/DESC` | Sortieren |
| `LIMIT offset, count` | Pagination |
| `RETURN expression` | Ergebnis definieren |
| `COLLECT field = expr` | Gruppieren |

**Reihenfolge**: `FOR` → `LET` → `FILTER` → `COLLECT` → `SORT` → `LIMIT` → `RETURN`

---

## Einfache Abfrage

```aql
// Alle aktiven Benutzer über 18 — sortiert nach Name
FOR user IN users
  FILTER user.active == true
  FILTER user.age > 18
  SORT user.name ASC
  LIMIT 0, 10
  RETURN {
    id:    user._key,
    name:  user.name,
    email: user.email
  }
```

**Operatoren**:
- Vergleich: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logisch: `AND`, `OR`, `NOT`
- Bereich: `IN [1,2,3]`, `NOT IN [...]`
- Muster: `LIKE "%text%"`, `=~` (Regex)

---

## INSERT — Daten einfügen

```aql
// Einzelnes Dokument einfügen
INSERT {
  name:    "Maria Schmidt",
  email:   "maria@example.com",
  age:     28,
  active:  true,
  tags:    ["python", "databases"],
  created: DATE_NOW()
} INTO users

// Mehrere Dokumente (Batch)
FOR item IN @batch_data
  INSERT item INTO users

// Mit Rückgabe des eingefügten Dokuments
INSERT { name: "New User" }
  INTO users
  RETURN NEW
```

---

## UPDATE & REPLACE

```aql
// UPDATE — nur angegebene Felder ändern
UPDATE "user_key_123" WITH {
  age:    29,
  active: false
} IN users

// UPDATE mit Bedingung (FOR + FILTER)
FOR user IN users
  FILTER user.email == "maria@example.com"
  UPDATE user WITH {
    last_login: DATE_NOW()
  } IN users

// REPLACE — gesamtes Dokument ersetzen
REPLACE "user_key_123" WITH {
  name:  "Maria Müller",
  email: "maria.mueller@example.com"
} IN users
```

---

## REMOVE & UPSERT

```aql
// REMOVE — Dokument löschen
REMOVE "user_key_123" IN users

// REMOVE mit Bedingung
FOR user IN users
  FILTER user.active == false
  FILTER user.last_login < DATE_SUBTRACT(DATE_NOW(), 1, "year")
  REMOVE user IN users

// UPSERT — Insert oder Update
UPSERT { email: "anna@example.com" }
  INSERT { email: "anna@example.com", name: "Anna", logins: 1 }
  UPDATE { logins: OLD.logins + 1 }
  IN users
```

---

## Joins — Verknüpfungen

```aql
// Inner Join (verschachteltes FOR)
FOR order IN orders
  FOR user IN users
    FILTER user._key == order.user_id
    RETURN {
      order_id: order._key,
      user:     user.name,
      total:    order.total
    }

// Left Join (Dokument-Lookup)
FOR order IN orders
  LET user = DOCUMENT("users", order.user_id)
  RETURN {
    order_id: order._key,
    user:     user != null ? user.name : "Gelöscht",
    total:    order.total
  }
```

---

## Subqueries & LET

```aql
// Subquery: Bestellanzahl pro Benutzer
FOR user IN users
  LET order_count = (
    FOR o IN orders
      FILTER o.user_id == user._key
      RETURN 1
  )
  SORT LENGTH(order_count) DESC
  RETURN {
    user:         user.name,
    order_count:  LENGTH(order_count)
  }

// LET für Zwischenergebnisse
FOR product IN products
  LET discounted_price = product.price * (1 - product.discount)
  FILTER discounted_price < 100
  RETURN { name: product.name, price: discounted_price }
```

---

## COLLECT — Gruppierung & Aggregation

```aql
// Benutzer nach Land gruppieren
FOR user IN users
  COLLECT country = user.country
    WITH COUNT INTO count
  SORT count DESC
  RETURN { country, count }

// Mehrere Aggregationen
FOR order IN orders
  COLLECT user_id = order.user_id
    AGGREGATE
      total_revenue = SUM(order.total),
      order_count   = COUNT(1),
      avg_order     = AVG(order.total),
      max_order     = MAX(order.total)
  RETURN {
    user_id,
    total_revenue,
    order_count,
    avg_order: ROUND(avg_order, 2),
    max_order
  }
```

---

## Graph-Traversierung

```aql
// Direktverbindungen (Tiefe 1)
FOR v IN 1..1 OUTBOUND "users/anna"
  GRAPH "social_graph"
  RETURN v.name

// Transitive Verbindungen bis Tiefe 3
FOR v, e, p IN 1..3 OUTBOUND "users/anna"
  GRAPH "social_graph"
  OPTIONS { uniqueVertices: "global" }
  RETURN DISTINCT v._key

// Kürzester Pfad
FOR p IN OUTBOUND SHORTEST_PATH
  "users/anna" TO "users/charlie"
  GRAPH "social_graph"
  RETURN p
```

---

## Graph — Richtungen & Filter

```aql
// Verschiedene Traversierungsrichtungen
OUTBOUND  -- nur ausgehende Kanten
INBOUND   -- nur eingehende Kanten
ANY       -- beide Richtungen

// Filter auf Kanten
FOR v, e IN 1..2 OUTBOUND "users/anna"
  GRAPH "social_graph"
  FILTER e.weight > 0.5        -- Nur starke Verbindungen
  FILTER v.active == true      -- Nur aktive Knoten
  RETURN { vertex: v.name, edge_weight: e.weight }

// K-Shortest-Paths
FOR p IN 1..5 OUTBOUND K_SHORTEST_PATHS
  "users/anna" TO "users/charlie"
  GRAPH "social_graph"
  RETURN p.vertices[*].name
```

---

## Vektor- & Volltextsuche

```aql
// Vektorähnlichkeitssuche
FOR doc IN articles
  LET score = COSINE_SIMILARITY(doc.embedding, @query_vector)
  FILTER score > 0.75
  SORT score DESC
  LIMIT 5
  RETURN { title: doc.title, score: ROUND(score, 4) }

// Volltextsuche (BM25)
FOR doc IN FULLTEXT(articles, "content", "database,+performance")
  RETURN doc.title

// Hybrid: Vektor + Metadaten-Filter
FOR doc IN articles
  LET score = COSINE_SIMILARITY(doc.embedding, @query_vector)
  FILTER score > 0.7
  FILTER doc.published >= "2025-01-01"
  FILTER "technology" IN doc.tags
  SORT score DESC
  RETURN doc
```

---

## LLM-Erweiterungen (v1.3.0+)

```aql
// Embedding aus Text erzeugen
LET emb = LLM EMBED "ThemisDB Performance Guide"
  USING MODEL "sentence-transformers/all-MiniLM-L6-v2"

// Inferenz direkt in Query
FOR product IN products
  LET review = LLM INFER CONCAT("Summarize: ", product.description)
    USING MODEL "llama-3.2-1b"
    OPTIONS { max_tokens: 100 }
  RETURN { product: product.name, summary: review }

// RAG (Retrieval-Augmented Generation)
FOR context IN articles
  LET score = COSINE_SIMILARITY(context.embedding, @query_vector)
  FILTER score > 0.8
  SORT score DESC
  LIMIT 3
  LET answer = LLM RAG @question
    WITH CONTEXT context.content
    USING MODEL "llama-3.2-3b"
  RETURN answer
```

---

## DDL — Collections erstellen

```aql
// Einfache Collection
CREATE COLLECTION products

// Typisierte Collection mit Schema
CREATE COLLECTION products (
  sku     STRING NOT NULL UNIQUE,
  name    STRING NOT NULL,
  price   FLOAT  NOT NULL,
  stock   INT    DEFAULT 0,
  active  BOOL   DEFAULT true
)

// Vertex-Collection (Graph)
CREATE COLLECTION users TYPE VERTEX

// Edge-Collection
CREATE COLLECTION follows
  TYPE EDGE
  FROM users TO users
```

---

## DDL — Indizes & Views

```aql
// Hash-Index (exakte Suche)
CREATE INDEX idx_products_sku ON products(sku) TYPE HASH UNIQUE

// Volltextindex
CREATE INDEX idx_articles_content ON articles(content) TYPE FULLTEXT

// Geo-Index
CREATE INDEX idx_locations_coord ON locations(latitude, longitude) TYPE GEO

// Vektorindex
CREATE INDEX idx_articles_embedding ON articles(embedding)
  TYPE VECTOR
  OPTIONS { metric: "cosine", dimension: 384 }

// View (materialisierte Abfrage)
CREATE VIEW active_products AS
  FOR p IN products
    FILTER p.active == true
    RETURN p
```

---

## Nützliche Built-in Funktionen

| Kategorie | Beispiele |
|---|---|
| **String** | `CONCAT`, `UPPER`, `LOWER`, `TRIM`, `SUBSTRING`, `SPLIT` |
| **Datum** | `DATE_NOW`, `DATE_FORMAT`, `DATE_ADD`, `DATE_DIFF`, `DATE_TRUNC` |
| **Array** | `LENGTH`, `APPEND`, `FLATTEN`, `UNION`, `INTERSECTION`, `MINUS` |
| **Math** | `FLOOR`, `CEIL`, `ROUND`, `ABS`, `SQRT`, `POW`, `RAND` |
| **Typ** | `IS_STRING`, `IS_NUMBER`, `IS_NULL`, `TO_STRING`, `TO_NUMBER` |
| **Geo** | `GEO_DISTANCE`, `GEO_CONTAINS`, `GEO_INTERSECTS` |
| **Vektor** | `COSINE_SIMILARITY`, `EUCLIDEAN_DISTANCE`, `DOT_PRODUCT` |

---

## Zusammenfassung Modul 3

✅ **AQL** — eine Sprache für alle 4 Datenmodelle

✅ **CRUD**: `INSERT`, `UPDATE`, `REPLACE`, `REMOVE`, `UPSERT`

✅ **Querying**: `FOR`, `FILTER`, `SORT`, `LIMIT`, `RETURN`, `COLLECT`

✅ **Graph**: `OUTBOUND/INBOUND/ANY`, `SHORTEST_PATH`, `K_SHORTEST_PATHS`

✅ **Vektor**: `COSINE_SIMILARITY`, HNSW-Suche

✅ **LLM**: `LLM INFER`, `LLM EMBED`, `LLM RAG`

✅ **DDL**: Collections, Indizes, Views

---

## 📚 Weiterführend

- Dokument: `dokumente/02_aql_referenz_kurzuebersicht.md`
- Dokument: `dokumente/04_uebungsaufgaben.md`
- Beispiele: `examples/02_aql_queries/`
- Vollständige Grammatik: `aql/AQL_GRAMMAR.ebnf`
