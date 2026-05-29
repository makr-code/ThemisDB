# ThemisDB Demo Data Setup

Automatische Generierung und Import von umfassenden Demo-Daten für die Kickstarter-Video-Demo.

## 🚀 Quick Start (2 Minuten)

### Schritt 1: Setup-Script ausführen

```powershell
cd C:\Projects\ThemisDB

# Stelle sicher, dass ThemisDB Server läuft
# (oder starte ihn in einem separaten Terminal)

# Führe das Setup-Script aus
.\demo\setup\setup_demo_data.ps1
```

Das Script wird automatisch:
1. ✅ Demo-Daten generieren (Python-Script)
2. ✅ 3 Collections erstellen (articles, embeddings, knowledge_graph)
3. ✅ ~30 Demo-Records importieren
4. ✅ Alles verifizieren

### Schritt 2: Demo starten

```powershell
.\demo\kickstarter_demo_script.ps1
```

---

## 📊 Generierte Demo-Daten

### 1. Document Collection (`demo_articles`)

**13 Research Papers** zu verschiedenen Themen:

| Thema | Anzahl | Beispiele |
|-------|--------|----------|
| AI & Deep Learning | 4 | ResNet, Vision Transformer, YOLO, Transformers |
| Quantum Computing | 2 | NISQ Algorithms, Quantum ML |
| Data Science | 3 | Time Series, Graph Neural Networks, Vector Databases |
| Databases | 2 | Multi-Model Architecture, Transactions |
| AI Security & Ethics | 2 | Adversarial Robustness, Ethical AI |

**Felder pro Artikel:**
```json
{
  "id": "doc_001",
  "title": "Deep Learning Architectures for Computer Vision",
  "author": "Dr. Alice Johnson",
  "category": "research",
  "published": "2024-03-15",
  "content": "This paper explores state-of-the-art...",
  "tags": ["AI", "deep-learning", "computer-vision"],
  "citation_count": 342
}
```

**Demo-Query (Text Search):**
```aql
FOR doc IN demo_articles
  FILTER doc.title LIKE '%AI%' OR doc.content LIKE '%machine learning%'
  SORT doc.published DESC
  LIMIT 5
  RETURN doc
```

---

### 2. Vector Collection (`demo_embeddings`)

**13 Embeddings** (128-dimensional vectors):

- Ein Embedding pro Artikel
- Mock-Vektoren (realistisch für Demo-Zwecke)
- Score zwischen 0.7-1.0 (Relevanz)
- Tags für jedes Embedding

**Felder pro Embedding:**
```json
{
  "id": "vec_001",
  "doc_id": "doc_001",
  "title": "Deep Learning Architectures for Computer Vision",
  "author": "Dr. Alice Johnson",
  "embedding": [0.123, -0.456, 0.789, ...],
  "score": 0.95,
  "relevance_tags": ["AI", "deep-learning", "computer-vision"]
}
```

**Demo-Query (Vector Search):**
```aql
FOR vec IN demo_embeddings
  LET similarity = COSINE_SIMILARITY(vec.embedding, @query_embedding)
  FILTER similarity > 0.7
  SORT similarity DESC
  LIMIT 5
  RETURN { title: vec.title, similarity: similarity }
```

---

### 3. Graph Collection (`demo_knowledge_graph`)

**Knowledge Graph mit 3 Datentypen:**

#### Nodes (22 insgesamt)
- **10 Researchers** (Dr. Alice Johnson, Prof. Bob Chen, etc.)
  - Affiliation: MIT, Stanford, Berkeley, Oxford, Cambridge, etc.
  - Fields: Deep Learning, NLP, Quantum Computing, etc.

- **7 Papers** (Research papers zu verschiedenen Topics)
  - Years: 2024
  - Titles: Matching zu den Articles

- **4 Conferences**
  - NeurIPS 2024, ICML 2024, ICCV 2024, VLDB 2024
  - Locations worldwide

#### Edges (16 insgesamt)
- `wrote` — Researcher hat Paper geschrieben
- `cites` — Paper zitiert anderes Paper
- `collaborates_with` — Forscher arbeiten zusammen
- `presented_at` — Paper wurde auf Konferenz präsentiert

**Demo-Query (Graph Traversal):**
```aql
FOR researcher IN demo_knowledge_graph
  FILTER researcher.type == 'researcher'
  FOR paper IN 1..2 OUTBOUND researcher._id graph_edges
    FILTER paper.type == 'paper'
  RETURN {
    researcher: researcher.name,
    paper: paper.title
  }
```

---

## 🔧 Setup Details

### Requirements
- Python 3.7+
- `themisctl` gebaut (z. B. `build-msvc-windows-release\bin\themisctl.exe`)
- ThemisDB Server läuft auf `localhost:8765`

### Was das Setup-Script tut

**1. `generate_demo_data.py`**
```powershell
python .\demo\setup\generate_demo_data.py
```

Erzeugt 4 JSONL-Dateien in `demo/data/`:
- `demo_articles.jsonl` (13 Articles)
- `demo_embeddings.jsonl` (13 Embeddings)
- `demo_knowledge_graph_nodes.jsonl` (22 Nodes)
- `demo_knowledge_graph_edges.jsonl` (16 Edges)
- `DATA_SUMMARY.md` (Documentation)

**2. `setup_demo_data.ps1`**
```powershell
.\demo\setup\setup_demo_data.ps1
```

Führt aus:
1. Python-Script aufrufen → Dateien generieren
2. Jede JSONL-Datei in themisctl importieren
3. Alle Collections verifizieren

### Ablage in der ThemisDB Demo-DB (wichtig)

Die Demo-Daten werden nicht nur als Dateien erzeugt, sondern per `themisctl put` in die laufende Demo-DB geschrieben.

- Physischer DB-Pfad im Demo-Flow:
  - `./demo/data/themis_db` (wenn der Server wie im Runbook mit `--db .\\demo\\data\\themis_db` gestartet wird)
- Primärschlüssel-Schema pro Collection:
  - `demo_articles:art_0001 ... art_0013`
  - `demo_embeddings:vec_0001 ...`
  - `demo_knowledge_graph:node_0001 ...` und `demo_knowledge_graph:edge_0001 ...`
- Payload-Schema beim Import:
  - Das Setup schreibt `{"blob":"<jsonl-zeile-als-string>"}`
  - Quelle: `Import-JsonlViaPut` in `demo/setup/setup_demo_data.ps1`

Beispiel (manuell inspizieren):

```powershell
& .\build-msvc-windows-release\bin\themisctl.exe --host 127.0.0.1 --port 8765 get demo_articles:art_0001
& .\build-msvc-windows-release\bin\themisctl.exe --host 127.0.0.1 --port 8765 get demo_knowledge_graph:node_0001
```

---

## 📋 Troubleshooting

### "themisctl not found"
```powershell
# Build themisctl zuerst
cmake --build --preset windows-release --target themisctl

# Dann Setup-Script nochmal laufen
.\demo\setup\setup_demo_data.ps1
```

### "Server connection refused"
```powershell
# In separatem Terminal: Server starten
.\build-msvc-windows-release\bin\themis_server.exe --db .\demo\data\themis_db --port 8765 --allow-degraded-build --allow-stub-hsm

# Dann in anderem Terminal: Setup-Script laufen
.\demo\setup\setup_demo_data.ps1
```

### "Collection already exists"
```powershell
# Alte Collections löschen (falls gewünscht)
themisctl admin drop-collection demo_articles
themisctl admin drop-collection demo_embeddings
themisctl admin drop-collection demo_knowledge_graph

# Dann Setup nochmal laufen
.\demo\setup\setup_demo_data.ps1
```

### "Import failed"
```powershell
# Prüfe ob JSONL-Dateien valide sind
Test-Path .\demo\data\demo_articles.jsonl

# Versuche manuellen Import
Get-Content .\demo\data\demo_articles.jsonl | themisctl batch-insert --collection demo_articles
```

---

## 📚 Weitere Ressourcen

- **[QUICKSTART.md](../QUICKSTART.md)** — 5-Minuten Demo-Start
- **[DEMO_QUERIES.md](../DEMO_QUERIES.md)** — Aktueller PowerShell Live-Runbook
- **[README.md](../README.md)** — Vollständiger Überblick

---

## 🎯 Use Cases für Video-Demo

### Document Search (Minute 2-3)
```aql
FOR doc IN demo_articles
  FILTER doc.title LIKE '%AI%'
  RETURN { title: doc.title, author: doc.author, citations: doc.citation_count }
```

**Voiceover:** "Das ist Full-Text-Suche in Millionen von Artikeln..."

### Vector Search (Minute 4-5)
```aql
FOR vec IN demo_embeddings
  LET sim = COSINE_SIMILARITY(vec.embedding, @query)
  FILTER sim > 0.7
  SORT sim DESC
  LIMIT 5
  RETURN { title: vec.title, score: ROUND(sim * 100, 1) }
```

**Voiceover:** "Semantische Suche - Bedeutung statt Keywords..."

### Graph Traversal (Minute 6-7)
```aql
FOR researcher IN demo_knowledge_graph
  FILTER researcher.type == 'researcher'
  FOR paper IN 1 OUTBOUND researcher._id graph_edges
  RETURN { researcher: researcher.name, paper: paper.title }
LIMIT 8
```

**Voiceover:** "Graph-Navigation - Beziehungen zwischen Daten..."

---

## ✅ Checkliste vor Video

- [ ] Setup-Script erfolgreich ausgeführt
- [ ] Alle 3 Collections importiert
- [ ] Queries aus DEMO_QUERIES.md funktionieren
- [ ] Server läuft stabil (keine Crashes)
- [ ] API-Spotcheck aus DEMO_QUERIES.md (Schema + Graph Explain) funktioniert
- [ ] Terminal font ist groß genug für Video

---

**Fertig! Jetzt kannst du die Demo aufnehmen! 🎬**
