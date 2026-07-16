# Datenmodellierung mit ThemisDB

> Ein praxisorientierter Leitfaden zum Entwerfen von Datenmodellen für ThemisDB — vom einfachen Dokumentenmodell bis hin zu komplexen Multi-Model-Architekturen.

---

## Inhaltsverzeichnis

1. [Modellwahl-Framework](#1-modellwahl-framework)
2. [Dokumentenmodell](#2-dokumentenmodell)
3. [Relationales Modell](#3-relationales-modell)
4. [Graphmodell](#4-graphmodell)
5. [Vektormodell](#5-vektormodell)
6. [Zeitreihen](#6-zeitreihen)
7. [Multi-Model-Design](#7-multi-model-design)
8. [Normalisierung vs. Denormalisierung](#8-normalisierung-vs-denormalisierung)
9. [Index-Strategie](#9-index-strategie)
10. [Migrations-Muster](#10-migrations-muster)

---

## 1. Modellwahl-Framework

```
Frage 1: Sind Ihre Daten stark strukturiert mit festen Beziehungen?
  JA  → Relationales Modell (Tabellen, Joins, ACID)
  NEIN → Weiter zu Frage 2

Frage 2: Haben Ihre Daten variable/verschachtelte Struktur?
  JA  → Dokumentenmodell (JSON Collections)
  NEIN → Weiter zu Frage 3

Frage 3: Stehen Beziehungen zwischen Entitäten im Mittelpunkt?
  JA  → Graphmodell (Vertex/Edge Collections)
  NEIN → Weiter zu Frage 4

Frage 4: Suchen Sie nach semantischer Ähnlichkeit?
  JA  → Vektormodell (Embeddings + HNSW)
  NEIN → Weiter zu Frage 5

Frage 5: Sind Ihre Daten zeitgestempelt und fortlaufend?
  JA  → Zeitreihen-Collection
```

**In der Praxis**: Fast jede komplexe Anwendung nutzt mehrere Modelle gleichzeitig.

---

## 2. Dokumentenmodell

### Wann einsetzen?
- Flexible oder variable Dokumentstrukturen
- Hierarchische Daten (verschachtelte Objekte/Arrays)
- Content-Management, Produktkataloge, Konfigurationen

### Beispiel: Produktkatalog

```json
{
  "_key": "prod_001",
  "sku":   "DB-GUIDE-2025",
  "name":  "ThemisDB Handbuch",
  "price": 49.99,
  "categories": ["Bücher", "Datenbanken"],
  "attributes": {
    "pages":    420,
    "language": "de",
    "format":   "PDF"
  },
  "variants": [
    { "format": "PDF",       "price": 29.99 },
    { "format": "Hardcover", "price": 49.99 }
  ],
  "tags":    ["themisdb", "database", "aql"],
  "created": "2025-01-15T10:00:00Z"
}
```

### Design-Regeln

- **Dokument-Größe** unter 64 KB halten
- **Arrays** für 1:N-Beziehungen innerhalb eines Dokuments
- **Referenzen** (Schlüssel anderer Collections) für externe Beziehungen
- **Verschachtelung** maximal 3–4 Ebenen tief

---

## 3. Relationales Modell

### Wann einsetzen?
- Klare, stabile Schema-Anforderungen
- Komplexe Joins und Aggregationen
- Finanzielle Transaktionen, Bestellsysteme, ERP

### Beispiel: Bestellsystem

```aql
// Strikte Schema-Definition
CREATE COLLECTION orders (
  order_id     STRING  NOT NULL,
  user_id      STRING  NOT NULL,
  status       STRING  NOT NULL DEFAULT "pending",
  total_amount FLOAT   NOT NULL,
  currency     STRING  NOT NULL DEFAULT "EUR",
  created_at   STRING  NOT NULL,
  updated_at   STRING
)

CREATE COLLECTION order_items (
  item_id    STRING  NOT NULL,
  order_id   STRING  NOT NULL,
  product_id STRING  NOT NULL,
  quantity   INT     NOT NULL,
  unit_price FLOAT   NOT NULL,
  subtotal   FLOAT   NOT NULL
)
```

### Normalisierungsregeln (1NF–3NF)

```
1NF: Atomare Werte — kein verschachtelter JSON in relationalen Feldern
2NF: Keine partiellen Abhängigkeiten vom zusammengesetzten Schlüssel
3NF: Keine transitiven Abhängigkeiten (z. B. Adresse in separate Collection)
```

---

## 4. Graphmodell

### Wann einsetzen?
- Netzwerke und Beziehungsgeflechte
- Traversierungen, kürzeste Pfade
- Empfehlungssysteme, Organigramme, Wissensgraphen

### Beispiel: Soziales Netzwerk

```aql
// Vertex-Collections (Entitäten)
CREATE COLLECTION users    TYPE VERTEX
CREATE COLLECTION hashtags TYPE VERTEX
CREATE COLLECTION posts    TYPE VERTEX

// Edge-Collections (Beziehungen)
CREATE COLLECTION follows   TYPE EDGE FROM users    TO users
CREATE COLLECTION likes     TYPE EDGE FROM users    TO posts
CREATE COLLECTION uses_tag  TYPE EDGE FROM posts    TO hashtags
CREATE COLLECTION authored  TYPE EDGE FROM users    TO posts

// Graph-Definition
CREATE GRAPH social_graph
  EDGE DEFINITION follows  FROM users TO users
  EDGE DEFINITION likes    FROM users TO posts
  EDGE DEFINITION uses_tag FROM posts TO hashtags
  EDGE DEFINITION authored FROM users TO posts
```

### Graph-Design-Regeln

- **Eine Edge-Collection pro Beziehungstyp** (nicht alles in eine Edge-Collection)
- **Kanten-Properties** für Beziehungsgewichte, Zeitstempel
- **Vertex-Properties** für Knotenattribute
- **Richtung** bewusst wählen: `OUTBOUND` von Quelle zu Ziel

---

## 5. Vektormodell

### Wann einsetzen?
- Semantische Ähnlichkeitssuche
- KI/ML-Integration (Embeddings)
- Dokumenten-Retrieval, Bild-/Audiosuche

### Embedding-Strategie

```aql
// Text-Embedding speichern (384-dimensional)
CREATE COLLECTION articles (
  _key      STRING,
  title     STRING,
  content   STRING,
  embedding VECTOR(384),  -- Vektordimension
  published STRING
)

// Vektorindex
CREATE INDEX idx_articles_embedding
  ON articles(embedding)
  TYPE VECTOR
  OPTIONS {
    metric:          "cosine",
    dimension:        384,
    m:                16,
    efConstruction:   200
  }
```

### Embeddings generieren und speichern

```aql
// Beim Einfügen Embedding erzeugen
INSERT {
  title:     "ThemisDB Guide",
  content:   @content,
  embedding: LLM EMBED @content
    USING MODEL "sentence-transformers/all-MiniLM-L6-v2",
  published: DATE_NOW()
} INTO articles
```

---

## 6. Zeitreihen

### Wann einsetzen?
- IoT-Sensordaten, Metriken, Logs
- Finanzmarktkurse
- Nutzungsstatistiken

```aql
// Zeitreihen-Collection
CREATE COLLECTION sensor_readings
  TYPE TIMESERIES
  TIMESTAMP_FIELD "ts"
  OPTIONS {
    retention:     "90d",          -- Automatisches Löschen nach 90 Tagen
    compression:   "gorilla",      -- Gorilla-Kompression für Floats
    granularity:   "1s"            -- Minumalauflösung
  }

// Kontinuierliche Aggregation (Materialisierte View)
CREATE CONTINUOUS AGGREGATE hourly_avg_temp
  ON sensor_readings
  EVERY "1h"
  AS (
    FOR r IN sensor_readings
      COLLECT hour    = DATE_TRUNC(r.ts, "hour"),
              sensor  = r.sensor_id
        AGGREGATE avg = AVG(r.temperature),
                  max = MAX(r.temperature),
                  min = MIN(r.temperature)
      RETURN { hour, sensor, avg, max, min }
  )
```

---

## 7. Multi-Model-Design

### Beispiel: E-Commerce-Plattform

```
Collections:
┌─────────────────────────────────────────────────────────────┐
│  DOKUMENT                                                    │
│  products      — Produktkatalog (flexibles Schema)          │
│  reviews       — Kundenbewertungen                          │
├─────────────────────────────────────────────────────────────┤
│  RELATIONAL                                                  │
│  orders        — Bestellungen (ACID-Transaktionen)          │
│  order_items   — Bestellpositionen                          │
│  invoices      — Rechnungen                                 │
├─────────────────────────────────────────────────────────────┤
│  GRAPH (Vertex)                                              │
│  customers     — Kundengraph                                │
│  categories    — Kategoriehierarchie                        │
│  Graph-Edges: purchased, belongs_to, recommended_for        │
├─────────────────────────────────────────────────────────────┤
│  VEKTOR                                                      │
│  product_embeddings  — Für semantische Suche               │
├─────────────────────────────────────────────────────────────┤
│  ZEITREIHEN                                                  │
│  page_views    — Klickdaten                                 │
│  inventory_log — Lagerbestandsverlauf                       │
└─────────────────────────────────────────────────────────────┘
```

---

## 8. Normalisierung vs. Denormalisierung

### Wann normalisieren?

```
✅ Daten werden häufig aktualisiert
✅ Konsistenz ist kritisch
✅ Speicherplatz soll minimiert werden
✅ Viele verschiedene Zugriffsprofile
```

### Wann denormalisieren?

```
✅ Read-Heavy-Workload (>>100:1 Reads:Writes)
✅ Performance ist kritisch (keine Joins)
✅ Daten ändern sich selten
✅ Snapshot-Daten (historische Werte)
```

### Hybrid-Pattern

```json
// Denormalisiert: Häufig gelesene Felder im Order-Dokument einbetten
{
  "order_id":   "ord_123",
  "user_name":  "Anna Schmidt",     // denormalisiert (read-optimized)
  "user_email": "anna@example.com", // denormalisiert
  "user_id":    "users/anna",       // Referenz für Updates
  "items": [...]
}
```

---

## 9. Index-Strategie

### Indizes systematisch planen

```
Frage: Welche Felder werden in FILTER-Klauseln verwendet?
→ Diese Felder brauchen Indizes

Frage: Sind die Werte eindeutig (wie E-Mail, Bestellnummer)?
→ UNIQUE Hash-Index

Frage: Werden Bereichsabfragen gemacht (Datum, Preis)?
→ Skiplist-Index

Frage: Wird Volltextsuche benötigt?
→ Fulltext-Index

Frage: Werden Geo-Abfragen gemacht?
→ Geo-Index (R-Tree)

Frage: Wird Vektorähnlichkeitssuche benötigt?
→ HNSW-Vektorindex
```

### Anti-Pattern: Zu viele Indizes

```
Jeder Index verlangsamt INSERT/UPDATE/DELETE.
Faustregel: Maximal 5-7 Indizes pro Collection.
Ungenutzte Indizes regelmäßig entfernen.
```

---

## 10. Migrations-Muster

### Schema-Evolution (sicher)

```aql
// Schritt 1: Neues optionales Feld hinzufügen (kein Breaking Change)
FOR doc IN users
  FILTER doc.preferences == null
  UPDATE doc WITH { preferences: { theme: "light" } } IN users

// Schritt 2: Altes Feld umbenennen (zweistufig)
// Phase 1: Neues Feld befüllen
FOR doc IN users
  UPDATE doc WITH { full_name: doc.name } IN users

// Phase 2: Altes Feld entfernen (nach Validierung)
FOR doc IN users
  UPDATE doc WITH { name: null } IN users

// Schritt 3: Feldtyp ändern (String → Number)
FOR doc IN products
  FILTER IS_STRING(doc.price)
  UPDATE doc WITH { price: TO_NUMBER(doc.price) } IN products
```

---

## Checkliste für neues Datenmodell

- [ ] Welche Datenmodelle werden benötigt?
- [ ] Dokumentengrößen geschätzt (< 64 KB?)
- [ ] Indizes für alle FILTER-Felder geplant
- [ ] Transaktionsgrenzen definiert
- [ ] Backup-Strategie festgelegt
- [ ] Migration von Bestandsdaten geplant
- [ ] Performance-Tests entworfen
- [ ] RBAC-Berechtigungen für neue Collections

## 🔗 Weiterführende Ressourcen

- [AQL Grammatik](../../aql/README.md)
- [Vollständige Beispiele](../../examples/)
- [Best Practices Guide](05_best_practices_guide.md)
