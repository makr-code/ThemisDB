> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ThemisDB Polyglot Benchmark - Quick Start Guide

**Version:** 2.0  
**Platform:** Windows + Docker Desktop  
**Ziel:** Vergleich von ThemisDB (Unified Multi-Model) vs Polyglot Persistence (multiple DBs)

---

## 🚀 Setup (5 Minuten)

### 1. Docker Container starten
```powershell
cd c:\VCC\themis\benchmarks\comparative

# Starte alle Datenbanken
docker-compose -f docker-compose.benchmark.yml up -d postgresql mongodb neo4j qdrant weaviate clickhouse surrealdb

# Prüfe Status
docker ps --filter "name=benchmark-"
```

**Erwartete Ausgabe:**
```
benchmark-postgresql   Up 30 seconds (healthy)
benchmark-mongodb      Up 30 seconds (healthy)
benchmark-neo4j        Up 30 seconds (health: starting)
benchmark-qdrant       Up 30 seconds (health: starting)
benchmark-weaviate     Up 30 seconds (health: starting)
benchmark-clickhouse   Up 30 seconds (healthy)
benchmark-surrealdb    Up 30 seconds (healthy)
```

---

### 2. ThemisDB Server kompilieren
```powershell
cd c:\VCC\themis

# Build Server Binary
cmake --build build-msvc --config Release --target themis_server

# Prüfe Binary
Get-ChildItem build-msvc\Release\themis_server.exe
```

---

### 3. ThemisDB Server starten
```powershell
cd c:\VCC\themis

# Starte Server (Port 8765)
.\build-msvc\Release\themis_server.exe --config config\default.yaml
```

**In separatem Terminal-Fenster:**
```powershell
# Health-Check
curl http://localhost:8765/health
```

---

### 4. Benchmark ausführen
```powershell
cd c:\VCC\themis\benchmarks\comparative\scripts

# Starte Extended Benchmark (3 Szenarien)
python extended_polyglot_benchmark.py
```

**Erwartete Laufzeit:** 10-15 Minuten (100 Iterationen pro Szenario)

---

## 📊 Benchmark-Szenarien

### Scenario 1: Document + Graph
**Vergleich:**
- **Polyglot:** PostgreSQL (Dokumente) + Neo4j (Graph) = 2 DBs
- **ThemisDB:** Unified Multi-Model = 1 DB

**Query:** Dokument-Fetch + 2-hop Graph-Traversierung (Author → Paper → Citations)

**Erwarteter ThemisDB Vorteil:** 40-50% schneller (kein Cross-DB Network Overhead)

---

### Scenario 2: Document + Vector
**Vergleich:**
- **Polyglot:** MongoDB (Dokumente) + Qdrant (Vector Search) = 2 DBs
- **ThemisDB:** Unified Multi-Model = 1 DB

**Query:** Hybrid Vector Search (Semantic Similarity + Metadata Filter)

**Erwarteter ThemisDB Vorteil:** 50-60% schneller (native HNSW Index)

---

### Scenario 3: OLAP + Document
**Vergleich:**
- **Polyglot:** ClickHouse (Analytics) + MongoDB (Documents) = 2 DBs
- **ThemisDB:** Unified Multi-Model = 1 DB

**Query:** Aggregationen (AVG, GROUP BY) + Document-Lookup

**Erwarteter ThemisDB Vorteil:** 30-40% schneller (einheitliches Storage Layer)

---

## 📈 Ergebnis-Interpretation

### Metriken
```
Mean (ms)   - Durchschnittliche Latenz
Median (ms) - Median-Latenz (robuster gegen Outliers)
P95 (ms)    - 95. Perzentil (Tail Latency)
P99 (ms)    - 99. Perzentil (Worst Case)
Improvement - ThemisDB Vorteil in % (positiv = schneller)
```

### Beispiel-Ausgabe
```
┏━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━┳━━━━━━━━━━━┳━━━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━━━━┓
┃ Scenario      ┃ Database          ┃ Operation   ┃ Mean (ms) ┃ Median (ms)┃ P95 (ms) ┃ P99 (ms) ┃ Improvement ┃
┡━━━━━━━━━━━━━━━╇━━━━━━━━━━━━━━━━━━━╇━━━━━━━━━━━━━╇━━━━━━━━━━━╇━━━━━━━━━━━━╇━━━━━━━━━━╇━━━━━━━━━━╇━━━━━━━━━━━━━┩
│ Document+Graph│ PostgreSQL+Neo4j  │ Hybrid Query│ 45.23     │ 43.12      │ 52.45    │ 58.34    │             │
│ Document+Graph│ ThemisDB          │ Hybrid Query│ 25.67     │ 24.89      │ 31.23    │ 35.67    │ +43.3%      │
└───────────────┴───────────────────┴─────────────┴───────────┴────────────┴──────────┴──────────┴─────────────┘
```

**Interpretation:**
- ThemisDB ist **43.3% schneller** als Polyglot (PostgreSQL + Neo4j)
- Median-Latenz: 24.89 ms (ThemisDB) vs 43.12 ms (Polyglot)
- P99-Latenz: 35.67 ms (ThemisDB) vs 58.34 ms (Polyglot) - Bessere Tail Latency!

---

## 🔧 Troubleshooting

### Container-Probleme

**Problem:** Container startet nicht
```powershell
# Prüfe Logs
docker logs benchmark-neo4j --tail 50

# Neustart
docker-compose -f docker-compose.benchmark.yml restart neo4j
```

**Problem:** Port bereits belegt
```powershell
# Prüfe Port-Usage
netstat -ano | findstr "7687"

# Container stoppen
docker-compose -f docker-compose.benchmark.yml down
```

---

### ThemisDB Server-Probleme

**Problem:** Server startet nicht
```powershell
# Prüfe Binary existiert
Get-ChildItem build-msvc\Release\themis_server.exe

# Prüfe Config
Get-Content config\default.yaml

# Rebuild
cmake --build build-msvc --config Release --target themis_server --clean-first
```

**Problem:** Port 8765 bereits belegt
```powershell
# Prüfe Port
netstat -ano | findstr "8765"

# Finde Prozess
Get-Process | Where-Object {$_.Id -eq <PID>}

# Stoppe Prozess
Stop-Process -Id <PID>
```

---

### Benchmark-Script Probleme

**Problem:** Python-Dependencies fehlen
```powershell
# Installiere alle Dependencies
pip install httpx psycopg2 pymongo neo4j qdrant-client clickhouse-connect weaviate-client surrealdb rich numpy pandas datasets
```

**Problem:** Connection Timeout
```powershell
# Prüfe DB Status
docker ps --filter "name=benchmark-"

# Warte auf healthy Status
docker ps --filter "name=benchmark-neo4j" --format "{{.Status}}"
# Erwartung: "Up 2 minutes (healthy)"
```

---

## 📁 Output-Dateien

### JSON-Results
```
benchmarks/comparative/scripts/benchmark_results_extended.json
```

**Format:**
```json
{
  "scenario": "Document+Graph",
  "database": "ThemisDB",
  "operation": "Hybrid Query",
  "statistics": {
    "mean_ms": 25.67,
    "median_ms": 24.89,
    "p95_ms": 31.23,
    "p99_ms": 35.67
  },
  "raw_latencies_ms": [24.5, 25.3, 26.1, ...]
}
```

### Console-Output
- Rich-formatierte Tabelle mit farbiger Ausgabe
- Progress-Bars während Benchmark-Ausführung
- Zusammenfassung am Ende

---

## 🎯 Performance-Erwartungen

### ThemisDB Vorteile
| Aspekt | Polyglot (3 DBs) | ThemisDB (1 DB) | Verbesserung |
|--------|------------------|-----------------|--------------|
| **Latenz** | 40-60 ms | 20-30 ms | 40-60% schneller |
| **Installation** | 3 separate DBs | 1 Binary | 3x einfacher |
| **Backup** | 3 Prozesse | 1 Snapshot | 3x weniger Aufwand |
| **Monitoring** | 3 Dashboards | 1 Dashboard | 3x weniger Komplexität |
| **Konsistenz** | Eventual | ACID über alles | Transaktional |

### Polyglot Nachteile
- ❌ **Network Overhead** - Cross-DB Kommunikation (2-10 ms pro Hop)
- ❌ **Data Sync** - Embeddings/Graph müssen synchronisiert werden
- ❌ **Ops Burden** - 3 separate Datenbanken pflegen
- ❌ **Complex Backups** - 3 separate Backup-Strategien

---

## 📚 Weitere Dokumentation

- **Database Matrix:** `DATABASE_MATRIX.md` - Alle 16 Datenbanken im Detail
- **Infrastructure Status:** `INFRASTRUCTURE_STATUS.md` - Container-Status
- **Original Benchmark:** `POLYGLOT_BENCHMARK_STATUS.md` - Erste Version
- **Docker Compose:** `docker-compose.benchmark.yml` - Container-Konfiguration
- **Benchmark Script:** `scripts/extended_polyglot_benchmark.py` - Python-Code

---

**Geschätzte Gesamtzeit:** 20-30 Minuten (Setup + Benchmark + Analyse)

**Los geht's! 🚀**
