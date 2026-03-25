---
marp: true
theme: default
paginate: true
backgroundColor: '#ffffff'
header: 'ThemisDB Schulung'
footer: '© ThemisDB – Modul 1: Einführung & Überblick'
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
## Modul 1: Einführung & Überblick

**Schulungsversion 1.0 · Niveau: Einsteiger**

---

## Agenda

1. Was ist ThemisDB?
2. Das Multi-Model-Konzept
3. Kernfunktionen im Überblick
4. Vergleich mit anderen Datenbanken
5. Architekturprinzipien
6. Typische Anwendungsfälle
7. Editionen & Lizenzierung
8. Erste Schritte: Hello World

---

## Was ist ThemisDB?

ThemisDB ist eine **hochperformante Multi-Model-Datenbank** mit nativer KI-Integration.

- 🗄️ **Multi-Model**: Relational, Graph, Vektor, Dokument — in einem System
- 🔒 **ACID-konform**: Vollständige Transaktionsunterstützung mit MVCC
- 🚀 **Hochperformant**: 45.000 Schreiboperationen/s, 120.000 Leseoperationen/s
- 🧠 **KI-ready**: Native LLM-Integration mit llama.cpp
- 🌐 **Modern**: HTTP/2, WebSocket, gRPC, MQTT, GraphQL, PostgreSQL Wire

> *"ThemisDB keeps its own llamas."* – Optionale native LLM-Integration direkt in der Datenbank.

---

## Das Multi-Model-Konzept

```
┌─────────────────────────────────────────────────┐
│                   ThemisDB                       │
├─────────────┬───────────┬──────────┬────────────┤
│  Relational │   Graph   │  Vektor  │  Dokument  │
│  (Tabellen, │ (Knoten,  │  (HNSW,  │  (JSON,    │
│   Joins)    │  Kanten)  │  KNN)    │  Flexibel) │
└─────────────┴───────────┴──────────┴────────────┘
              ↓           ↓          ↓
         Gemeinsamer Speicher & Transaktionsengine
              ↓           ↓          ↓
         AQL — Einheitliche Abfragesprache
```

**Vorteil**: Kein Polyglot-Persistence-Problem — eine Datenbank für alle Modelle.

---

## Kernfunktionen

| Funktion | Details |
|---|---|
| 🔒 ACID Transaktionen | Snapshot Isolation, MVCC |
| 🔍 AQL Abfragesprache | Multi-Paradigm, SQL-ähnlich |
| 📊 Vektorsuche | HNSW, GPU-beschleunigt (v2.x) |
| 🌐 Protokolle | HTTP/2, gRPC, WebSocket, MQTT |
| 🧠 LLM Integration | llama.cpp, LoRA, RAG, FLARE |
| 🛡️ Sicherheit | TLS 1.3, RBAC, AES-256-GCM |
| 📈 Zeitreihen | Gorilla-Kompression, cont. Aggregation |
| ⏱️ Temporal | AS OF, bitemporale Queries |

---

## Vergleich mit anderen Datenbanken

| Feature | ThemisDB | PostgreSQL | MongoDB | Neo4j | Pinecone |
|---|---|---|---|---|---|
| Relational | ✅ | ✅ | ❌ | ❌ | ❌ |
| Dokument | ✅ | ✅ | ✅ | ❌ | ❌ |
| Graph | ✅ | ❌ | ❌ | ✅ | ❌ |
| Vektor | ✅ | pgvector | Atlas | ❌ | ✅ |
| LLM-nativ | ✅ | ❌ | ❌ | ❌ | ❌ |
| Eine Sprache | ✅ (AQL) | SQL | MQL | Cypher | API |

**ThemisDB eliminiert den Bedarf nach mehreren spezialisierten Datenbanken.**

---

## Architekturprinzipien

```
Clients (HTTP/2, gRPC, WebSocket, PostgreSQL Wire)
          ↓
    API Gateway & Auth Layer
          ↓
    AQL Parser & Query Optimizer
          ↓
  ┌───────────────────────────────┐
  │   Execution Engine            │
  │  ┌─────────┬────────────────┐ │
  │  │Transaction (MVCC/SAGA)   │ │
  │  ├─────────┬────────────────┤ │
  │  │ Storage (RocksDB)        │ │
  │  └─────────┴────────────────┘ │
  └───────────────────────────────┘
          ↓
    Index Layer (B-Tree, HNSW, Spatial, ...)
```

---

## Typische Anwendungsfälle

### 1. E-Commerce-Plattform
- Produktkatalog (Dokumente)
- Empfehlungs-Engine (Graph + Vektor)
- Bestelltransaktionen (Relational + ACID)

### 2. KI-gestützte Anwendungen
- Semantische Suche über Embeddings
- RAG (Retrieval-Augmented Generation)
- LLM-Inferenz direkt in Datenbankabfragen

### 3. Soziale Netzwerke
- Freundschaftsgraphen (Graph-Modell)
- Aktivitäts-Feeds (Zeitreihen)
- Profilsuche (Volltextsuche)

---

## Editionen

| Edition | Zielgruppe | Features |
|---|---|---|
| **MINIMAL** | Entwickler | Core + AQL |
| **COMMUNITY** | Open Source | + Graph, Volltextsuche |
| **ENTERPRISE** | Unternehmen | + RBAC, Verschlüsselung, HA |
| **HYPERSCALER** | Cloud-nativ | + Sharding, CDC, K8s |
| **MILITARY** | Sicherheitskritisch | + HSM, Air-Gap, FIPS |

---

## Hello World — Erste Verbindung

```bash
# ThemisDB mit Docker starten
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  themisdb/themisdb:latest

# Health-Check
curl http://localhost:8080/health
```

```python
from themis_client import ThemisClient

client = ThemisClient("http://localhost:8080")
result = client.query("""
  INSERT { name: "Hello, ThemisDB!", active: true }
  INTO greetings
""")
print(result)
```

---

## Zusammenfassung Modul 1

✅ ThemisDB ist eine **Multi-Model-Datenbank** (Relational + Graph + Vektor + Dokument)

✅ **Eine Abfragesprache** (AQL) für alle Modelle

✅ **ACID-konform** mit MVCC für Transaktionssicherheit

✅ **Hochperformant**: 45K Schreib-/120K Leseops/s

✅ **KI-ready**: Native LLM-Integration

✅ **Produktionsreif**: Sicherheit, Monitoring, HA

---

## 📚 Weiterführend

- **Modul 2**: Datenmodelle & Architektur (Details zu jedem Modell)
- **Modul 3**: AQL Abfragesprache (vollständige Sprachreferenz)
- Dokument: `dokumente/01_quickstart_guide.md`
- Beispiel: `examples/01_grundlegende_operationen/`

**Fragen?** → [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
