---
marp: true
theme: default
paginate: true
backgroundColor: '#ffffff'
header: 'ThemisDB Schulung'
footer: '© ThemisDB – Modul 5: Anwendungsbeispiele & Best Practices'
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
## Modul 5: Anwendungsbeispiele & Best Practices

**Schulungsversion 1.0 · Niveau: Fortgeschritten**

---

## Agenda

1. Use Case 1: Blog-Plattform (Multi-Model)
2. Use Case 2: Empfehlungs-Engine (Graph + Vektor)
3. Use Case 3: IoT-Sensorik (Zeitreihen)
4. Use Case 4: KI-gestützte Suche (LLM + RAG)
5. Performance-Best-Practices
6. Sicherheits-Best-Practices
7. Datenmodellierungs-Best-Practices
8. Fehlerbehandlung & Resilienz
9. Migrations-Strategien

---

## Use Case 1: Blog-Plattform

```
Datenmodell:
  posts      (Dokument)   — Artikel, Entwürfe, Metadaten
  users      (Vertex)     — Autoren, Leser
  tags       (Vertex)     — Themen-Tags
  comments   (Dokument)   — Kommentare (verschachtelt)
  tagged_as  (Edge)       — Post → Tag
  authored   (Edge)       — User → Post
  post_views (Zeitreihen) — Aufrufstatistiken

Abfragen:
  • Artikel nach Tag filtern       → Graph-Traversierung
  • Ähnliche Artikel vorschlagen   → Vektorsuche
  • Aufruftrends visualisieren     → Zeitreihen-Aggregation
  • Kommentar-Hierarchie laden     → Rekursive Subquery
```

---

## Blog — Beispielabfragen

```aql
// Ähnliche Artikel (Vektorähnlichkeit)
LET current_post = DOCUMENT("posts/@post_id")
FOR post IN posts
  FILTER post._key != current_post._key
  LET score = COSINE_SIMILARITY(post.embedding, current_post.embedding)
  FILTER score > 0.7
  SORT score DESC
  LIMIT 5
  RETURN { title: post.title, score: ROUND(score, 3) }

// Top-Artikel der letzten Woche nach Views
FOR v IN post_views
  FILTER v.ts >= DATE_SUBTRACT(DATE_NOW(), 7, "day")
  COLLECT post_id = v.post_id
    AGGREGATE view_count = COUNT(1)
  SORT view_count DESC
  LIMIT 10
  LET post = DOCUMENT("posts", post_id)
  RETURN { title: post.title, views: view_count }
```

---

## Use Case 2: Empfehlungs-Engine

```
Datenmodell:
  users       (Vertex)  — Benutzerprofile, Präferenzen
  products    (Vertex)  — Produktkatalog
  purchased   (Edge)    — user → product (mit Bewertung)
  viewed      (Edge)    — user → product (Klickdaten)
  similar_to  (Edge)    — product ↔ product (Vektorähnlichkeit)

Algorithmen:
  1. Collaborative Filtering  → Graph: "User kaufte auch..."
  2. Content-Based            → Vektor: Produktähnlichkeit
  3. Hybrid                   → Graph + Vektor kombiniert
```

---

## Empfehlungs-Engine — Queries

```aql
// Collaborative Filtering: Was kaufen ähnliche User?
LET user_purchases = (
  FOR v IN 1..1 OUTBOUND "users/@user_id" purchased
    RETURN v._key
)

FOR similar_user IN 1..2 OUTBOUND "users/@user_id" purchased, similar_user
  FOR recommended IN 1..1 OUTBOUND similar_user purchased
    FILTER recommended._key NOT IN user_purchases
    COLLECT product_id = recommended._key
      WITH COUNT INTO score
    SORT score DESC
    LIMIT 10
    RETURN { product_id, collaborative_score: score }
```

---

## Use Case 3: IoT-Sensorik

```aql
// Anomalie-Erkennung: Werte außerhalb 3-Sigma
FOR sensor IN sensor_data
  FILTER sensor.location_id == "factory_floor_a"
  FILTER sensor.ts >= DATE_SUBTRACT(DATE_NOW(), 1, "hour")
  COLLECT location = sensor.location_id
    AGGREGATE avg_temp = AVG(sensor.temperature),
              std_dev  = STDDEV(sensor.temperature)
  LET threshold = 3.0 * std_dev

  FOR reading IN sensor_data
    FILTER reading.location_id == location
    FILTER ABS(reading.temperature - avg_temp) > threshold
    SORT reading.ts DESC
    RETURN {
      ts:          reading.ts,
      temperature: reading.temperature,
      deviation:   ABS(reading.temperature - avg_temp) / std_dev
    }
```

---

## Use Case 4: KI-gestützte Suche (RAG)

```aql
// Frage an die Dokumentationsdatenbank
LET question = @user_question
LET query_embedding = LLM EMBED question
  USING MODEL "sentence-transformers/all-MiniLM-L6-v2"

// Relevante Dokumente abrufen
LET context_docs = (
  FOR doc IN documentation
    LET score = COSINE_SIMILARITY(doc.embedding, query_embedding)
    FILTER score > 0.75
    SORT score DESC
    LIMIT 3
    RETURN doc.content
)

// Antwort generieren
LET answer = LLM RAG question
  WITH CONTEXT CONCAT_SEPARATOR("\n\n", context_docs)
  USING MODEL "llama-3.2-3b"
  OPTIONS { max_tokens: 500, temperature: 0.1 }

RETURN { question, answer, sources: LENGTH(context_docs) }
```

---

## Performance Best Practices

### 1. Indizes strategisch einsetzen

```aql
-- ✅ GUT: Feld mit Index
FILTER user.email == "test@example.com"  -- Hash-Index auf email

-- ❌ SCHLECHT: Kein Index, Full-Collection-Scan
FILTER LOWER(user.email) == "test@example.com"

-- ✅ Für Bereichsabfragen: Skiplist-Index
CREATE INDEX idx_orders_date ON orders(created_at) TYPE SKIPLIST
FILTER order.created_at >= "2025-01-01"
```

### 2. Bind-Variablen verwenden

```aql
-- ✅ Sicher & performant (Query-Plan-Caching)
FILTER user.age > @min_age   -- @min_age = 18

-- ❌ String-Konkatenation (kein Caching, SQL-Injection-Risiko)
FILTER user.age > 18
```

---

## Performance — LIMIT früh anwenden

```aql
-- ✅ GUT: LIMIT vor teurer JOIN-Operation
FOR user IN users
  FILTER user.active == true
  SORT user.score DESC
  LIMIT 0, 100                     -- Erst limitieren
  FOR order IN orders
    FILTER order.user_id == user._key
    RETURN { user: user.name, orders: COUNT(order) }

-- ❌ SCHLECHT: Erst alle Joins, dann limitieren
FOR user IN users
  FOR order IN orders
    FILTER order.user_id == user._key
    SORT user.score DESC
    LIMIT 0, 100
    RETURN { user, order }
```

**Regel**: `LIMIT` so früh wie möglich in der Pipeline!

---

## Sicherheits-Best-Practices

### Authentifizierung aktivieren

```json
{
  "auth": {
    "enabled": true,
    "jwt_secret": "STARKES_GEHEIMNIS_MINIMUM_32_ZEICHEN",
    "token_expiry": "8h"
  }
}
```

### RBAC — Rollen und Berechtigungen

```aql
// Benutzer mit minimalen Rechten anlegen
CREATE USER app_readonly PASSWORD "..."
GRANT READ ON DATABASE production TO app_readonly

CREATE USER app_writer PASSWORD "..."
GRANT READ, WRITE ON COLLECTION orders TO app_writer
GRANT READ ON COLLECTION products TO app_writer
```

---

## Sicherheit — TLS und Verschlüsselung

```json
{
  "server": {
    "tls": {
      "enabled":    true,
      "cert_path":  "/etc/ssl/themisdb.crt",
      "key_path":   "/etc/ssl/themisdb.key",
      "min_version": "TLS1.3"
    }
  },
  "storage": {
    "encryption": {
      "enabled":   true,
      "algorithm": "AES-256-GCM"
    }
  }
}
```

**Checkliste für Produktion**:
- [ ] TLS 1.3 aktiviert
- [ ] Starkes JWT-Secret (min. 32 Zeichen)
- [ ] Encryption at rest aktiviert
- [ ] Minimale RBAC-Berechtigungen
- [ ] Audit-Logging aktiviert

---

## Datenmodellierung — Best Practices

### Dokumentgröße begrenzen

```aql
-- ✅ Normalisiert: große Textfelder auslagern
{
  "post_id": "p1",
  "title": "ThemisDB Guide",
  "summary": "Brief overview...",
  "content_ref": "content/p1"   // Referenz auf großes Dokument
}

-- ❌ Denormalisiert: zu große Dokumente (> 64 KB vermeiden)
{
  "post_id": "p1",
  "title": "...",
  "full_content": "10.000 Zeichen langer Text..."
}
```

### Collections nicht überladen

```
-- ✅ Separate Collections pro Entitätstyp
users, products, orders, order_items

-- ❌ Alles in einer "documents" Collection (schwierig zu indizieren)
```

---

## Fehlerbehandlung & Resilienz

```python
from themis_client import ThemisClient, ThemisQueryError
import time

client = ThemisClient("http://localhost:8080")

def query_with_retry(aql, bind_vars=None, max_retries=3):
    for attempt in range(max_retries):
        try:
            return client.query(aql, bind_vars=bind_vars)
        except ThemisQueryError as e:
            if e.code == 1200:  # Conflict / Deadlock
                time.sleep(0.1 * (2 ** attempt))  # Exponential backoff
                continue
            raise  # Andere Fehler sofort weiterwerfen
    raise RuntimeError(f"Query failed after {max_retries} retries")
```

---

## Migrations-Strategien

```aql
// Schema-Migration: Neues Pflichtfeld hinzufügen
FOR doc IN users
  FILTER doc.preferences == null
  UPDATE doc WITH {
    preferences: {
      notifications: true,
      theme: "light",
      language: "de"
    }
  } IN users
RETURN NEW._key

// Nach Migration: Index erstellen
CREATE INDEX idx_users_theme
  ON users(preferences.theme)
  TYPE HASH
```

**Migrations-Checkliste**:
1. Migration in Transaktion ausführen
2. Erst testen auf Staging-Umgebung
3. Backup vor der Migration
4. Rollback-Script bereithalten

---

## Zusammenfassung Modul 5

✅ **Multi-Model** in einem System für komplexe Anwendungen

✅ **RAG** mit LLM-Integration direkt in AQL-Abfragen

✅ **Indizes** strategisch einsetzen — `LIMIT` früh in der Pipeline

✅ **Bind-Variablen** für sichere und performante Queries

✅ **TLS + RBAC** für produktionsreife Sicherheit

✅ **Retry-Logik** und **Exponential Backoff** für Resilienz

✅ **Migrations-Strategie** mit Transaktionen und Backups

---

## 🎓 Schulung abgeschlossen!

**Sie können jetzt:**
- ThemisDB installieren und konfigurieren
- Datenmodelle für komplexe Anwendungsfälle entwerfen
- AQL-Abfragen für alle Modelle schreiben
- Best Practices für Performance und Sicherheit anwenden
- ThemisDB in eigene Anwendungen integrieren

**Weiterführend:**
- `dokumente/05_best_practices_guide.md`
- `examples/04_multimodell_anwendung/`
- [GitHub Repository](https://github.com/makr-code/ThemisDB)
- [Offizielle Dokumentation](../docs/)
