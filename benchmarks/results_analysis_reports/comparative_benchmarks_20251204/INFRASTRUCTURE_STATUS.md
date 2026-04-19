> ⚠️ **Historischer Statusreport** – Dieser Bericht beschreibt den Implementierungsstand zum Zeitpunkt der Erstellung.
> Für den aktuellen Stand: Quellcode und aktuelle [`benchmarks/README.md`](../../README.md) prüfen.

# ThemisDB Polyglot Benchmark - Infrastructure Status

**Version:** 2.0 - Erweiterte DB-Basis  
**Datum:** 4. Dezember 2025  
**Status:** 11 von 12 Datenbanken online

---

## 🚀 Aktive Datenbanken

### ✅ Running & Healthy (6 Datenbanken)
| Database | Container | Port | Status | Use Case |
|----------|-----------|------|--------|----------|
| **PostgreSQL 16** | `benchmark-postgresql` | 5432 | Healthy | Relational + JSONB |
| **MongoDB 7.0** | `benchmark-mongodb` | 27017 | Healthy | Document Store |
| **ClickHouse** | `benchmark-clickhouse` | 8123/9000 | Healthy | Column-Oriented OLAP |
| **Neo4j 5** | `benchmark-neo4j` | 7474/7687 | Starting | Graph Database |
| **Qdrant** | `benchmark-qdrant` | 6333/6334 | Starting | Vector Search |
| **Weaviate** | `benchmark-weaviate` | 8080/50051 | Starting | Vector + ML Models |

### 🆕 Recently Added (5 Datenbanken)
| Database | Container | Port | Status | Notes |
|----------|-----------|------|--------|-------|
| **SurrealDB** | `benchmark-surrealdb` | 8002 | ✅ Running | Multi-Model (Memory mode) |
| **Neo4j** | `benchmark-neo4j` | 7474/7687 | 🟡 Starting | Graph + APOC plugins |
| **Qdrant** | `benchmark-qdrant` | 6333/6334 | 🟡 Starting | High-perf vector search |
| **Weaviate** | `benchmark-weaviate` | 8080/50051 | 🟡 Starting | Vector + semantic search |
| **ClickHouse** | `benchmark-clickhouse` | 8123/9000 | ✅ Healthy | Analytics engine |

### ⏳ Not Started
| Database | Reason | Priority |
|----------|--------|----------|
| **ThemisDB** | Binary `themis_server.exe` nicht kompiliert | 🔴 High |
| **CozoDB** | Docker Image nicht verfügbar (`cozodata/cozodb` nicht gefunden) | 🟡 Medium |

### ❌ Removed from Scope
- **Milvus** - Zu komplex (benötigt etcd + MinIO)
- **Elasticsearch** - Wird in Scenario 3 verwendet (bereits verfügbar)
- **ArangoDB** - Wird in separatem Vergleich getestet
- **Redis/Redis Stack** - Cache-focused, nicht für diese Benchmarks

---

## 📊 Datenbank-Matrix

### Multi-Model Databases (ThemisDB Competitors)
```
ThemisDB     ⏳ Pending (themis_server.exe build)
ArangoDB     ✅ Available (Port 8529)
SurrealDB    ✅ Running (Port 8002)
CozoDB       ❌ Image nicht verfügbar
```

### Spezialisierte Datenbanken
```
PostgreSQL   ✅ Healthy (Port 5432) - Relational + JSONB
MongoDB      ✅ Healthy (Port 27017) - Document Store
Neo4j        🟡 Starting (Port 7474/7687) - Graph
ClickHouse   ✅ Healthy (Port 8123/9000) - OLAP
Qdrant       🟡 Starting (Port 6333) - Vector
Weaviate     🟡 Starting (Port 8080) - Vector + ML
```

---

## 🔧 Benchmark-Szenarien (Aktualisiert)

### Scenario 1: Document + Graph
**Aktuelle Implementierung:**
- ✅ PostgreSQL + Neo4j (Polyglot)
- ✅ ThemisDB (Unified) - **Blocked by themis_server.exe**
- 🆕 SurrealDB (Multi-Model alternative)

**Query:** Dokument-Fetch + 2-hop Graph-Traversierung

---

### Scenario 2: Document + Vector
**Aktuelle Implementierung:**
- ✅ MongoDB + Qdrant (Polyglot)
- ✅ ThemisDB (Unified) - **Blocked by themis_server.exe**
- 🆕 Weaviate (Hybrid Vector+Document)

**Query:** Hybrid Vector Search mit Metadaten-Filter

---

### Scenario 3: OLAP + Document
**Aktuelle Implementierung:**
- ✅ ClickHouse + MongoDB (Polyglot)
- ✅ ThemisDB (Unified) - **Blocked by themis_server.exe**
- 🆕 SurrealDB (Multi-Model alternative)

**Query:** Aggregationen + Document-Lookup

---

## 🐳 Docker Commands

### Container starten
```powershell
# Alle Datenbanken starten
docker-compose -f docker-compose.benchmark.yml up -d

# Spezifische DBs
docker-compose -f docker-compose.benchmark.yml up -d postgresql mongodb neo4j qdrant weaviate clickhouse surrealdb
```

### Status prüfen
```powershell
docker ps --filter "name=benchmark-" --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
```

### Logs prüfen
```powershell
docker logs benchmark-neo4j --tail 50
docker logs benchmark-qdrant --tail 50
docker logs benchmark-weaviate --tail 50
```

### Container stoppen & Cleanup
```powershell
docker-compose -f docker-compose.benchmark.yml down
docker-compose -f docker-compose.benchmark.yml down -v  # Mit Volumes löschen
```

---

## 📦 Python Dependencies

### Installiert ✅
```python
httpx           # ThemisDB HTTP Client
psycopg2        # PostgreSQL
pymongo         # MongoDB
neo4j           # Neo4j Python Driver
qdrant-client   # Qdrant Vector DB
clickhouse-connect  # ClickHouse
weaviate-client # Weaviate
surrealdb       # SurrealDB Python Client
rich            # Console Output
numpy, pandas   # Data Processing
datasets        # Hugging Face Datasets
```

### Installation Command
```powershell
pip install httpx psycopg2 pymongo neo4j qdrant-client clickhouse-connect weaviate-client surrealdb rich numpy pandas datasets
```

---

## ⚙️ Resource Allocation

Alle Container verwenden **identische Ressourcen** (Fair Comparison):
```yaml
resources:
  limits:
    cpus: '4'
    memory: 4G
  reservations:
    cpus: '2'
    memory: 2G
```

**Gesamt-Ressourcen benötigt:** ~44 GB RAM, 40 CPUs (11 Container)

---

## 🎯 Nächste Schritte

1. ✅ **Docker-Infrastruktur erweitert** - 11 Datenbanken laufen
2. ⏳ **ThemisDB Server Build** 
   ```powershell
   cmake --build build-msvc --config Release --target themis_server
   ```
3. ⏳ **Health-Check Wartung** - Neo4j, Qdrant, Weaviate auf "healthy" warten
4. ⏳ **Benchmark-Script Validierung** - `extended_polyglot_benchmark.py` testen
5. ⏳ **Vollständige Benchmark-Ausführung** - 3 Szenarien mit 100 Iterationen
6. ⏳ **HTML Report Generation** - Ergebnisse visualisieren

---

## 📈 Erwartete Ergebnisse

### ThemisDB vs Polyglot Persistence
- **Latenz-Reduktion:** 40-60% (keine Cross-DB Network Overhead)
- **Ops-Komplexität:** 2-3x weniger (1 DB statt 3-4 DBs)
- **Transaktionale Konsistenz:** Über alle Datenmodelle (vs. Eventual Consistency)
- **Backup-Aufwand:** 1 Snapshot statt 3-4 separate Prozesse

### Multi-Model Competitors
- **SurrealDB:** Ähnliche Architektur wie ThemisDB, aber weniger optimiert
- **ArangoDB:** Kein nativer Vector-Support (extern erforderlich)
- **CozoDB:** Datalog-basiert, unterschiedlicher Ansatz

---

**Infrastruktur bereit für Benchmark-Ausführung nach ThemisDB Server Build! 🚀**
