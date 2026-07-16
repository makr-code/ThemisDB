---
marp: true
theme: default
paginate: true
backgroundColor: '#ffffff'
header: 'ThemisDB Schulung'
footer: '© ThemisDB – Modul 2: Datenmodelle & Architektur'
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
## Modul 2: Datenmodelle & Architektur

**Schulungsversion 1.0 · Niveau: Einsteiger bis Fortgeschritten**

---

## Agenda

1. Das relationale Modell in ThemisDB
2. Das Dokumentenmodell
3. Das Graphmodell
4. Das Vektormodell
5. Zeitreihen & Temporal
6. Speicher-Engine: RocksDB + MVCC
7. ACID-Transaktionen
8. Index-Architektur
9. Datenmodellierung — Wann welches Modell?

---

## Relationales Modell

ThemisDB unterstützt **tabellenartige Collections** mit schema-optionalem Design.

```aql
// Collection erstellen (Tabelle)
CREATE COLLECTION users (
  id    STRING  NOT NULL,
  name  STRING,
  email STRING  UNIQUE,
  age   INT
)

// Daten einfügen
INSERT { id: "u1", name: "Anna", email: "anna@example.com", age: 30 }
INTO users

// Join-Abfrage
FOR u IN users
  FOR o IN orders
    FILTER o.user_id == u.id
    RETURN { user: u.name, order: o.total }
```

---

## Dokumentenmodell

Collections speichern flexible JSON-Dokumente — kein festes Schema erforderlich.

```aql
// Dokument mit verschachtelten Strukturen
INSERT {
  name: "ThemisDB Handbuch",
  author: { name: "Max Müller", org: "TechCorp" },
  tags: ["database", "multi-model", "performance"],
  metadata: {
    version: "1.0",
    published: "2025-01-01",
    pages: 420
  }
} INTO documents

// Zugriff auf verschachtelte Felder
FOR doc IN documents
  FILTER "database" IN doc.tags
  RETURN doc.author.name
```

---

## Graphmodell — Knoten & Kanten

```
     [Anna]──follows──▶[Bob]
       │                  │
    likes               likes
       │                  │
       ▼                  ▼
    [Post A]          [Post B]
       │
    tagged_as
       │
       ▼
    [#ThemisDB]
```

```aql
// Graph-Traversierung (2 Schritte)
FOR v, e, p IN 1..2 OUTBOUND "users/anna"
  GRAPH "social_graph"
  RETURN v.name
```

---

## Graphmodell — Collections definieren

```aql
// Knoten-Collection (Benutzer)
CREATE COLLECTION users TYPE VERTEX

// Kanten-Collection (Verbindungen)
CREATE COLLECTION follows
  TYPE EDGE
  FROM users TO users

// Graphdefinition
CREATE GRAPH social_graph
  EDGE DEFINITION follows
    FROM users TO users
```

**Kanten-Dokument** hat immer `_from` und `_to` Felder:
```json
{ "_from": "users/anna", "_to": "users/bob", "since": "2025-01-15" }
```

---

## Vektormodell — Semantische Suche

Vektoren repräsentieren **semantische Bedeutung** als numerische Arrays (Embeddings).

```aql
// Embedding erzeugen und speichern
INSERT {
  text: "ThemisDB is a multi-model database",
  embedding: LLM EMBED "ThemisDB is a multi-model database"
    USING MODEL "sentence-transformers/all-MiniLM-L6-v2"
} INTO articles

// Ähnlichkeitssuche (k-Nearest-Neighbor)
FOR doc IN articles
  LET score = COSINE_SIMILARITY(doc.embedding, @query_vector)
  FILTER score > 0.8
  SORT score DESC
  LIMIT 10
  RETURN { text: doc.text, score: score }
```

---

## Vektorindizes — HNSW

ThemisDB verwendet **HNSW** (Hierarchical Navigable Small World) für effiziente Vektorsuche:

```aql
CREATE INDEX articles_embedding_hnsw
  ON articles(embedding)
  TYPE VECTOR
  OPTIONS {
    metric: "cosine",
    dimension: 384,
    m: 16,
    efConstruction: 200
  }
```

| Parameter | Bedeutung |
|---|---|
| `metric` | `cosine`, `euclidean`, `dot_product` |
| `dimension` | Vektordimension (z. B. 384, 768, 1536) |
| `m` | Verbindungen pro Knoten |
| `efConstruction` | Qualität beim Indexaufbau |

---

## Zeitreihen

Optimierter Speicher für zeitgestempelte Daten mit **Gorilla-Kompression**.

```aql
// Zeitreihen-Collection
CREATE COLLECTION sensor_data
  TYPE TIMESERIES
  TIMESTAMP_FIELD "ts"

// Daten einfügen
INSERT { ts: DATE_NOW(), sensor: "temp_01", value: 23.5 }
INTO sensor_data

// Zeitfenster-Aggregation
FOR d IN sensor_data
  FILTER d.ts >= DATE_SUBTRACT(DATE_NOW(), 1, "hour")
  COLLECT bucket = DATE_TRUNC(d.ts, "minute")
    AGGREGATE avg_val = AVG(d.value)
  RETURN { time: bucket, avg: avg_val }
```

---

## Temporale Queries — Zeitreise

ThemisDB unterstützt **bitemporale Queries**: AS OF, FROM/TO, BETWEEN AND.

```aql
// Zustand zu einem bestimmten Zeitpunkt
FOR u IN users AS OF "2025-06-01T00:00:00Z"
  FILTER u.active == true
  RETURN u

// Alle Versionen in einem Zeitraum
FOR u IN users FROM "2025-01-01" TO "2025-12-31"
  RETURN { user: u._key, valid_from: u._valid_from }

// Bitemporal: System- und Anwendungszeit
FOR record IN orders
  BETWEEN AND "2025-01-01", "2025-06-30"
  CONTAINED IN "2024-12-01", "2025-07-01"
  RETURN record
```

---

## Speicher-Engine: RocksDB + MVCC

```
Write Path                    Read Path
───────────                   ──────────
INSERT/UPDATE/DELETE          FOR ... FILTER ...
      ↓                               ↓
  WAL (Write-Ahead Log)        Snapshot (MVCC)
      ↓                               ↓
  MemTable (in-memory)          MemTable + SST
      ↓
  SST Files (RocksDB)
      ↓
  Compaction (background)
```

**MVCC** (Multi-Version Concurrency Control):
- Jeder Read liest eine konsistente **Snapshot-Version**
- Writes blockieren keine Reads
- Reads blockieren keine Writes

---

## ACID-Transaktionen

```aql
// Explizite Transaktion
BEGIN TRANSACTION
  LET from_account = DOCUMENT("accounts/A")
  LET to_account   = DOCUMENT("accounts/B")

  UPDATE { balance: from_account.balance - 100 }
    IN accounts WHERE _key == "A"
  UPDATE { balance: to_account.balance + 100 }
    IN accounts WHERE _key == "B"
COMMIT

// Bei Fehler: automatisches ROLLBACK
```

| Eigenschaft | Implementierung |
|---|---|
| **Atomicity** | WAL + Rollback |
| **Consistency** | Constraint-Prüfung |
| **Isolation** | MVCC Snapshot |
| **Durability** | RocksDB + fsync |

---

## Index-Architektur

| Index-Typ | Anwendungsfall |
|---|---|
| **B-Tree** | Bereichsabfragen, Sortierung |
| **Hash** | Exakte Suche (O(1)) |
| **HNSW** | Vektorähnlichkeitssuche |
| **Fulltext (BM25)** | Volltextsuche |
| **Spatial (R-Tree)** | Geo-Abfragen |
| **Skiplist** | Sortierte Bereiche |
| **Composite** | Multi-Feld-Queries |

```aql
CREATE INDEX idx_users_email ON users(email) TYPE HASH
CREATE INDEX idx_posts_content ON posts(content) TYPE FULLTEXT
CREATE INDEX idx_locations_geo ON locations(coord) TYPE GEO
```

---

## Wann welches Modell?

```
Frage: Welches Datenmodell passt zu meinem Use Case?

Strukturierte Daten mit Beziehungen?
  → Relational (Tabellen + Joins)

Flexible, verschachtelte Daten?
  → Dokument (JSON Collections)

Netzwerke, Beziehungsnetzwerke, Traversierungen?
  → Graph (Vertex + Edge Collections)

Semantische/KI-gestützte Suche?
  → Vektor (Embeddings + HNSW)

Sensorik, Metriken, Logs?
  → Zeitreihen (TimeSeries Collections)

Mehrere Use Cases gleichzeitig?
  → ThemisDB Multi-Model! (ein System, eine Sprache)
```

---

## Datenmodellierung — Praxisbeispiel E-Commerce

```
Collections:
  products     (Dokument) — Produktkatalog, flexible Attribute
  categories   (Vertex)   — Kategoriehierarchie
  belongs_to   (Edge)     — Produkt → Kategorie
  orders       (Relational) — Bestellungen, ACID
  order_items  (Relational) — Positionen
  embeddings   (Vektor)   — Produktbeschreibungen für Suche
  page_views   (Zeitreihen) — Klickdaten, Metriken

Abfragen:
  "Zeige ähnliche Produkte" → Vektorsuche auf embeddings
  "Kategoriebaum traversieren" → Graph-Traversierung
  "Bestellung aufgeben" → Transaktion auf orders/order_items
  "Klicktrend der letzten Stunde" → Zeitreihen-Aggregation
```

---

## Zusammenfassung Modul 2

✅ **4 Datenmodelle** in einer Datenbank (Relational, Dokument, Graph, Vektor)

✅ **RocksDB** als hochperformante Speicher-Engine

✅ **MVCC** für nicht-blockierende Transaktionsisolation

✅ **ACID** für zuverlässige Transaktionen

✅ **Vielfältige Indizes** für jeden Abfrage-Typ

✅ **Zeitreihen & Temporal** für historische und zeitgestempelte Daten

---

## 📚 Weiterführend

- **Modul 3**: AQL Abfragesprache (alle Modelle in einer Sprache abfragen)
- **Modul 4**: Installation & Setup
- Dokument: `dokumente/03_datenmodellierung_guide.md`
- Beispiel: `examples/03_graph_daten/`
