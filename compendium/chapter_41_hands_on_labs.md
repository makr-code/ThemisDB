# Kapitel 41: Hands-on Labs – Deployment, Index-Tuning, Vector-Suche

> "Lernen durch Tun: Diese Labs führen Sie Schritt für Schritt durch echte Produktionsaufgaben." 

---

## Überblick

Drei geführte Labs mit klaren Zielen, Schritt-für-Schritt-Anleitungen, Verifikation und Aufräumen.

**Labs:**
- Lab A: Schnellstart-Deployment (Docker) + Smoke Tests
- Lab B: Index-Tuning & Query-Optimierung mit EXPLAIN/PROFILE
- Lab C: Vector-Suche End-to-End (Embeddings erzeugen, Indexieren, Query)

**Voraussetzungen:** Docker/WSL, themis_client CLI oder HTTP, Grundkenntnisse AQL.

---

## Lab A: Deployment & Smoke Tests (30-45 min)

### Ziel
- ThemisDB per Docker starten
- Healthcheck & Basiskonfiguration prüfen
- Erste Write/Read-Query ausführen

### Schritte
1) **Container starten**
```bash
docker run -d --name themisdb -p 8529:8529 themisdb/server:latest
```
2) **Healthcheck**
```bash
curl -s http://localhost:8529/_admin/status | jq '.server.status'
```
Erwartet: `"ok"`

3) **Admin-Login setzen (falls nötig)**
```bash
curl -X POST http://localhost:8529/_admin/setup --data '{"password":"StrongPass!"}'
```

4) **Smoke-Test Write/Read**
```aql
-- write
INSERT { _key: "alice", name: "Alice" } INTO users
-- read
FOR u IN users FILTER u._key == "alice" RETURN u
```

5) **Persistenz prüfen**
```bash
docker restart themisdb
curl -s http://localhost:8529/_api/document/users/alice
```

### Aufräumen
```bash
docker rm -f themisdb
```

---

## Lab B: Index-Tuning & Query-Optimierung (40-60 min)

### Ziel
- Langsame Query identifizieren
- EXPLAIN/PROFILE nutzen
- Index anlegen und Improvement messen

### Dataset vorbereiten
```aql
FOR i IN 1..200000
  INSERT {
    _key: CONCAT('user_', i),
    email: CONCAT('user', i, '@example.com'),
    status: i % 5 == 0 ? 'inactive' : 'active',
    created_at: DATE_NOW()
  } INTO users
```

### Baseline messen
```aql
PROFILE FOR u IN users
  FILTER u.email == 'user199999@example.com'
  RETURN u
```
Notieren: p99 Latenz.

### EXPLAIN prüfen
```aql
EXPLAIN FOR u IN users
  FILTER u.email == 'user199999@example.com'
  RETURN u
```
Erwartet: CollectionScan (schlecht).

### Index anlegen
```aql
CREATE INDEX idx_users_email ON users(email)
```

### Nachher messen
```aql
PROFILE FOR u IN users
  FILTER u.email == 'user199999@example.com'
  RETURN u
```
Erwartet: IndexNode, stark reduzierte Latenz. Dokumentieren Sie Vor/Nach.

### Optionale Optimierungen
- Projection pushdown: nur benötigte Felder returnen
- LIMIT 1 setzen, wenn eindeutig

### Aufräumen
```aql
DROP INDEX users.idx_users_email
TRUNCATE users
```

---

## Lab C: Vector-Suche End-to-End (45-75 min)

### Ziel
- Texte einbetten
- Vektor-Index aufbauen
- Ähnlichkeitssuche ausführen

### Setup
1) **Kollektion anlegen**
```aql
CREATE COLLECTION articles
```

2) **Beispiel-Dokumente einfügen**
```aql
INSERT {
  _key: "art1",
  title: "Graph Algorithms",
  content: "Graphs, shortest paths, centrality",
  vector: null
} INTO articles
INSERT {
  _key: "art2",
  title: "Neural Networks",
  content: "Deep learning, backpropagation",
  vector: null
} INTO articles
INSERT {
  _key: "art3",
  title: "Databases",
  content: "Indexing, transactions, sharding",
  vector: null
} INTO articles
```

3) **Embeddings erzeugen (Client-seitig, Beispiel Python)**
```python
# embed_articles.py
from sentence_transformers import SentenceTransformer
import themis_client

client = themis_client.connect()
model = SentenceTransformer('all-MiniLM-L6-v2')

articles = client.execute("FOR a IN articles RETURN {key: a._key, text: a.content}")
for a in articles:
    vec = model.encode(a['text']).tolist()
    client.execute(
        "UPDATE { _key: @key } WITH { vector: @vec } IN articles",
        bind_vars={"key": a['key'], "vec": vec}
    )
```

4) **Vektor-Index anlegen**
```aql
CREATE VECTOR INDEX idx_articles_vec ON articles(vector) WITH {dimensions: 384, type: "hnsw", efConstruction: 200, M: 16}
```

5) **Ähnlichkeitssuche**
```aql
LET query_vec = @query_vec
FOR doc IN articles
  SEARCH doc.vector ANN { query: query_vec, top_k: 3 }
  RETURN { _key: doc._key, title: doc.title, score: doc._score }
```
Bind Vars: `query_vec` = Embedding eines Suchtexts, z.B. "machine learning".

6) **Relevanz prüfen**
- Erwartet: "Neural Networks" weit oben bei ML-Query
- Score dokumentieren

### Optionen
- efSearch erhöhen für bessere Recall
- PQ/IVF nutzen für größere Datenmengen
- Caching von Embeddings

### Aufräumen
```aql
TRUNCATE articles
```

---

## Checkliste (Abgehakt, wenn Lab bestanden)
- [ ] Lab A: Container-Start, Healthcheck, Smoke Query
- [ ] Lab B: EXPLAIN/PROFILE vor/nach, Index Speedup dokumentiert
- [ ] Lab C: Embeddings erzeugt, Vektor-Index erstellt, ANN Query ausgeführt

**Nächste Schritte:** Kombinieren Sie Labs: Deploy → Index-Tuning → Vector-Suche auf realen Datensätzen.
