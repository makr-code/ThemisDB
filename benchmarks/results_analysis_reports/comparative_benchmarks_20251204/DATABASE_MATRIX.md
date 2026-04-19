> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeipunkt und sind nicht mehr reproduzierbar ohne die ursprüngliche Testumgebung.
> Für reproduzierbare Ergebnisse: Benchmark-Kommandos und aktuelle CMake-Presets unter [`benchmarks/README.md`](../../README.md) verwenden.

# Polyglot Benchmark - Datenbank-Matrix

**Version:** 2.0 (Erweiterte DB-Basis)  
**Datum:** 4. Dezember 2025

## Übersicht

Diese Matrix dokumentiert alle **16 Datenbanksysteme** in der erweiterten Benchmark-Infrastruktur.

---

## Datenbank-Kategorien

### 🎯 Multi-Model Databases (Direct Competitors)
| Database | Port | Models | Query Language | Notes |
|----------|------|--------|----------------|-------|
| **ThemisDB** | 8765 | Document, Graph, Vector, K/V | AQL (SQL-like) | **Target** - Unified Multi-Model |
| **ArangoDB** | 8529 | Document, Graph, K/V | AQL | Direct competitor, no native vector |
| **CozoDB** | 9070 | Graph, Relational, Document | Datalog (CozoScript) | Immutable data focus |
| **SurrealDB** | 8002 | Document, Graph, Relational | SurrealQL | SQL-like, embedded mode |

### 📄 Document Databases
| Database | Port | Storage Engine | Vector Support | Notes |
|----------|------|----------------|----------------|-------|
| **MongoDB** | 27017 | WiredTiger | Via $vectorSearch | Production-grade, 2GB cache |
| **PostgreSQL** | 5432 | PostgreSQL | Via JSONB | Relational + JSONB hybrid |

### 📊 Graph Databases
| Database | Port | Query Language | ACID | Notes |
|----------|------|----------------|------|-------|
| **Neo4j** | 7474/7687 | Cypher | Full | Community Edition, APOC plugins |

### 🔍 Vector Databases
| Database | Port | Index Type | Distance Metrics | Notes |
|----------|------|------------|------------------|-------|
| **Milvus** | 19530 | HNSW, IVF, DiskANN | L2, IP, Cosine, Hamming | etcd + MinIO dependencies |
| **Qdrant** | 6333/6334 | HNSW | Cosine, Euclid, Dot | High-performance, gRPC |
| **Weaviate** | 8080/50051 | HNSW | Cosine, L2, Dot, Hamming | ML model integration |
| **ChromaDB** | 8000 | HNSW | L2, IP, Cosine | AI/ML focus, embeddings |
| **PostgreSQL + pgvector** | 5433 | IVF, HNSW | L2, IP, Cosine | Extension-based |
| **Redis Stack** | 6380 | HNSW | L2, IP, Cosine | RediSearch + RedisJSON |

### 🔑 Key-Value / Cache
| Database | Port | Persistence | Data Structures | Notes |
|----------|------|-------------|-----------------|-------|
| **Redis** | 6379 | AOF/RDB | Strings, Lists, Sets, Hashes | In-memory, 3GB max |

### 🔎 Search Engines
| Database | Port | Index Type | Full-Text | Vector | Notes |
|----------|------|------------|-----------|--------|-------|
| **Elasticsearch** | 9200 | Inverted Index | ✅ | ✅ (kNN) | 2GB heap, single-node |

### 📈 OLAP / Analytics
| Database | Port | Storage Format | Compression | Notes |
|----------|------|----------------|-------------|-------|
| **ClickHouse** | 8123/9000 | Column-Oriented | LZ4, ZSTD | High-speed aggregations |

---

## Benchmark-Szenarien (Erweitert)

### Szenario 1: Document + Graph (3-Way)
**Vergleich:**
- **Polyglot:** PostgreSQL (JSONB) + Neo4j (Graph)
- **Multi-Model Competitors:** ArangoDB, CozoDB, SurrealDB
- **ThemisDB:** Unified Multi-Model

**Query:** Dokumente mit Graph-Traversierung (2-hop relationships)

---

### Szenario 2: Document + Vector (5-Way)
**Vergleich:**
- **Polyglot:** MongoDB (Documents) + ChromaDB/Qdrant (Vector)
- **Multi-Model Competitors:** Weaviate (hybrid), PostgreSQL + pgvector
- **ThemisDB:** Unified Multi-Model

**Query:** Hybrid Search (Vektor-Ähnlichkeit + Metadaten-Filter)

---

### Szenario 3: Full-Text + Vector
**Vergleich:**
- **Specialized:** Elasticsearch (Full-Text) + Milvus (Vector)
- **Hybrid:** Weaviate (both), Redis Stack (both)
- **ThemisDB:** Unified Multi-Model

**Query:** Semantische Suche mit Text-Ranking

---

### Szenario 4: OLAP + Document
**Vergleich:**
- **Polyglot:** ClickHouse (Analytics) + MongoDB (Documents)
- **Multi-Model:** SurrealDB, ThemisDB
- **ThemisDB:** Unified Multi-Model

**Query:** Aggregationen über Dokumenten-Kollektion

---

### Szenario 5: Graph + Vector
**Vergleich:**
- **Polyglot:** Neo4j (Graph) + Qdrant (Vector)
- **Multi-Model:** ArangoDB + External Vector DB
- **ThemisDB:** Native Graph + Vector Index

**Query:** Knowledge Graph Navigation mit Vektor-Ähnlichkeit

---

## Resource Allocation (Fair Comparison)

Alle Container erhalten **identische Ressourcen:**
```yaml
resources:
  limits:
    cpus: '4'
    memory: 4G
  reservations:
    cpus: '2'
    memory: 2G
```

**Ausnahmen:**
- **Milvus:** Benötigt etcd + MinIO (zusätzliche Services)
- **Redis:** 3GB Maxmemory (In-Memory Constraint)
- **Elasticsearch:** 2GB JVM Heap (Java-basiert)

---

## Erwartete Performance-Trends

### ThemisDB Vorteile (Unified Multi-Model)
- ✅ **40-60% niedrigere Latenz** - Keine Cross-DB Joins
- ✅ **2-3x weniger Ops-Komplexität** - 1 DB statt 3-4 DBs
- ✅ **Transaktionale Konsistenz** - Über alle Datenmodelle
- ✅ **Einheitliches Monitoring** - 1 Dashboard

### Polyglot Persistence Nachteile
- ❌ **Network Overhead** - Cross-DB Kommunikation
- ❌ **Data Synchronization** - Embedding/Graph Sync-Probleme
- ❌ **Operational Burden** - 3-4 separate Datenbanken
- ❌ **Complex Backups** - 3-4 separate Prozesse

### Spezialisierte DBs Vorteile
- ✅ **Höchste Single-Model Performance** - z.B. Milvus für Vektor
- ✅ **Reife Tools** - z.B. Neo4j Graph Analytics
- ✅ **Spezialisierte Features** - z.B. Elasticsearch Analyzers

---

## Datensets (Hugging Face)

### Document + Text
- **wikipedia-simple-english** (10K articles)
- **squad** (Q&A pairs)

### Embeddings
- **sentence-transformers/all-MiniLM-L6-v2** (384-dim vectors)
- **openai/text-embedding-3-small** (1536-dim vectors)

### Graph
- **ogbn-arxiv** (Citation network)
- **cora** (Research papers + citations)

---

## Nächste Schritte

1. ✅ **Docker Compose erweitert** - 16 Datenbanken konfiguriert
2. ⏳ **ThemisDB Server Build** - `themis_server.exe` kompilieren
3. ⏳ **Container Health-Check** - Alle 16 DBs starten und validieren
4. ⏳ **Benchmark-Scripts erweitern** - 5 Szenarien implementieren
5. ⏳ **Ausführung** - Vollständige Benchmark-Suite
6. ⏳ **Report-Generierung** - HTML-Dashboard mit Vergleichen

---

**Geschätzte Benchmark-Laufzeit:** 2-3 Stunden (1000 Iterationen pro Szenario)  
**Erwartete Report-Größe:** 50-100 MB (HTML + JSON + CSV)
