> **Navigation:** Verlinkungen auf aktuelle Docker-Benchmark-Dateipfade prüfen.

# ThemisDB Docker Benchmarks - Unified Suite

**Version:** 1.0.0  
**Status:** ✅ Production Ready  
**Last Updated:** 2026-04-06

---

## 📋 Übersicht

Vollständiges Docker-basiertes Benchmark-System für ThemisDB v1.0.1:

- ✅ **Unified Orchestrator** - Master-Skript für alle Benchmarks
- ✅ **Multi-Workload** - Relational, Vector, Graph, Geo, Document, Hybrid
- ✅ **Multi-Competitor** - PostgreSQL, MySQL, MongoDB, Elasticsearch, Neo4j, Milvus, etc.
- ✅ **Multi-Protocol** - TCP, HTTP, gRPC, Wire
- ✅ **Automated Reporting** - JSON, CSV, HTML, Markdown
- ✅ **Gap Analysis** - Automatische Konkurrenten-Vergleiche
- ✅ **Docker Integration** - 3 Varianten (optimized/lite/extended)

---

## 🚀 Quick Start

### 1. Docker Stack starten
```bash
cd benchmarks/comparative
docker compose -f docker-compose.benchmark-optimized.yml up -d
```

### 2. Benchmarks ausführen
```bash
# Relational Workloads (schnell, ~15 Min)
python3 docker_benchmarks_unified.py --workload relational --duration 60

# Alle Workloads (umfassend, ~2 Std)
python3 docker_benchmarks_unified.py --workload all --duration 120

# Mit Lite-Konfiguration (eingeschränkte Ressourcen)
python3 docker_benchmarks_unified.py --workload all --docker-file lite
```

### 3. Ergebnisse anschauen
```bash
# HTML Report öffnen
firefox docker_benchmarks_results_*/reports/benchmark_results.html

# Oder JSON inspizieren
cat docker_benchmarks_results_*/reports/benchmark_results.json | jq '.summary'

# CSV in Excel öffnen
docker_benchmarks_results_*/reports/benchmark_results.csv

# Markdown lesen
less docker_benchmarks_results_*/reports/BENCHMARK_RESULTS.md
```

### 4. Docker cleanup
```bash
cd benchmarks/comparative
docker compose down -v
```

---

## 📊 Workload-Details

### 1. Relational CRUD (Schnellste)
```bash
python3 docker_benchmarks_unified.py --workload relational --duration 60
```
- **Tests:** insert, read, update, delete, range_query
- **Konkurrenten:** PostgreSQL, MySQL, MariaDB, CockroachDB
- **Protokolle:** TCP, HTTP, gRPC
- **Dauer:** ~15 Min
- **Focus:** SLSA L1 Baseline

### 2. Vector Search
```bash
python3 docker_benchmarks_unified.py --workload vector --duration 120
```
- **Tests:** index, search, range_search, recall
- **Konkurrenten:** Milvus, Weaviate, Qdrant, Chroma
- **Protokolle:** gRPC, HTTP
- **Dauer:** ~30 Min

### 3. Graph Operations
```bash
python3 docker_benchmarks_unified.py --workload graph --duration 120
```
- **Tests:** node_insert, edge_insert, traversal, shortest_path
- **Konkurrenten:** Neo4j, ArangoDB, JanusGraph
- **Protokolle:** TCP, gRPC
- **Dauer:** ~25 Min

### 4. Geo-Spatial
```bash
python3 docker_benchmarks_unified.py --workload geo --duration 90
```
- **Tests:** point_insert, radius_search, polygon_search
- **Konkurrenten:** PostgreSQL+PostGIS, MongoDB, Elasticsearch
- **Protokolle:** HTTP, TCP
- **Dauer:** ~20 Min

### 5. Document Operations
```bash
python3 docker_benchmarks_unified.py --workload document --duration 60
```
- **Tests:** insert, read, update, bulk_insert
- **Konkurrenten:** MongoDB, CouchDB, Firebase
- **Protokolle:** HTTP
- **Dauer:** ~15 Min

### 6. Hybrid Workloads
```bash
python3 docker_benchmarks_unified.py --workload hybrid --duration 120
```
- **Tests:** hybrid_search, polyglot_query, multi_modal
- **Konkurrenten:** ThemisDB (unique capability)
- **Protokolle:** gRPC
- **Dauer:** ~20 Min

### ALL (Vollständig)
```bash
python3 docker_benchmarks_unified.py --workload all --duration 120
```
- **Dauer:** ~2 Stunden (alle 6 Workloads)
- **Tests:** 40+
- **Metriken:** 150-200

---

## 📁 File-Struktur

```
benchmarks/
├── docker_benchmarks_unified.py      ← NEW: Unified Orchestrator (800 Zeilen)
├── docker_benchmarks_suite_index.md  ← NEW: Diese Datei
├── DOCKER_COMPARATIVE_BENCHMARKS_README.md (Alt)
├── DOCKER_BENCHMARKS_STATUS_REPORT.md
├── DOCKER_QUICKSTART.md
│
├── comparative/
│   ├── docker-compose.benchmark.yml
│   ├── docker-compose.benchmark-optimized.yml
│   ├── docker-compose.benchmark-lite.yml
│   └── docker-compose.benchmark-extended.yml
│
├── gap_analysis/
│   ├── historical_gaps.json
│   ├── historical_gaps.md
│   └── v1.0.1_closure_targets.json
│
├── enterprise_benchmarks_20251204_*/  (Baseline)
│   └── benchmark_results.json
│
├── docker_benchmark_results_YYYYMMDD_HHMMSS/  (Neue Ergebnisse)
│   └── reports/
│       ├── benchmark_results.json
│       ├── benchmark_results.csv
│       ├── benchmark_results.html
│       └── BENCHMARK_RESULTS.md
```

---

## 🔧 Fortgeschrittene Verwendung

### Nur Analyse ohne Docker
```bash
# Nur Gap-Analyse durchführen ohne Docker Benchmarks
python3 docker_benchmarks_unified.py --analyze-only
```

### Spezifische Workload + spezifisches Docker-Setup
```bash
# Relational mit Extended Konfiguration
python3 docker_benchmarks_unified.py \
  --workload relational \
  --docker-file extended \
  --duration 180 \
  --output my_results/
```

### Benutzerdefiniertes Output-Verzeichnis
```bash
python3 docker_benchmarks_unified.py \
  --workload all \
  --output /data/benchmarks/v1.0.1_validation/
```

---

## 📊 Output-Format

### JSON Structure
```json
{
  "timestamp": "2025-12-09T14:30:00",
  "version": "1.0.1",
  "metrics": [
    {
      "workload": "relational",
      "test_name": "insert",
      "competitor": "PostgreSQL",
      "protocol": "tcp",
      "latency_ms": 1.453,
      "latency_p50": 1.3,
      "latency_p95": 1.9,
      "throughput": 688,
      "memory_mb": 256.5,
      "cpu_percent": 32.1,
      "success_rate": 99.5
    }
  ],
  "gaps": {
    "relational": [
      {
        "test": "insert",
        "competitor": "PostgreSQL",
        "improvement_pct": 45.1,
        "is_closed": true,
        "severity": "excellent"
      }
    ]
  },
  "summary": {
    "total_metrics": 156,
    "total_gaps": 52,
    "gaps_closed": 48,
    "gap_closure_rate": "92.3%"
  }
}
```

### CSV Format
```
workload,test_name,competitor,protocol,latency_ms,latency_p95,throughput,memory_mb,cpu_percent
relational,insert,PostgreSQL,tcp,1.453,1.9,688,256.5,32.1
relational,insert,MySQL,tcp,1.0,1.3,1000,240.2,28.5
```

### HTML Report
- Visual Charts (Latency, Throughput)
- Gap Analysis Table
- Competitor Rankings
- Performance Trends

### Markdown Report
- Executive Summary
- Gap Analysis by Workload
- Detailed Metrics Table
- Recommendations

---

## 🎯 Success Criteria

### Relational Workload (Baseline)
- ✅ Gap-Closure Rate: >85%
- ✅ PostgreSQL TCP: <0.6ms (von 0.8ms)
- ✅ Average Latency: <1.0ms

### All Workloads Combined
- ✅ Total Gap-Closure: >87% (>30/36 gaps)
- ✅ Critical Gaps: 5-6 closed (83%+)
- ✅ High-Priority: >20 closed (87%+)

---

## 🐛 Troubleshooting

### Docker-Fehler
```bash
# Prüfe Container-Status
docker compose ps

# Logs anschauen
docker logs benchmark-themisdb
docker logs benchmark-postgresql

# Neustart
docker compose down -v
docker compose up -d
```

### Nicht genug Ressourcen
```bash
# Verwende Lite-Variante
python3 docker_benchmarks_unified.py --docker-file lite

# Oder reduziere Duration
python3 docker_benchmarks_unified.py --duration 30
```

### Python-Fehler
```bash
# Installiere Requirements
pip install psutil

# Oder check Python Version
python3 --version  # 3.8+
```

---

## 📈 Integration mit CI/CD

### GitHub Actions
```yaml
- name: Run Docker Benchmarks
  run: |
    cd benchmarks
    python3 docker_benchmarks_unified.py --workload all
    
- name: Upload Results
  uses: actions/upload-artifact@v3
  with:
    name: benchmark-results
    path: docker_benchmarks_results_*/reports/
```

### GitLab CI
```yaml
docker_benchmarks:
  script:
    - cd benchmarks
    - python3 docker_benchmarks_unified.py --workload relational
  artifacts:
    paths:
      - docker_benchmarks_results_*/reports/
```

---

## 📚 Dokumentation

| File | Purpose |
|------|---------|
| `docker_benchmarks_unified.py` | Main orchestrator (800 lines) |
| `docker_benchmarks_suite_index.md` | This guide |
| `DOCKER_COMPARATIVE_BENCHMARKS_README.md` | Detailed reference |
| `DOCKER_BENCHMARKS_STATUS_REPORT.md` | Gap analysis results |
| `gap_analysis/historical_gaps.md` | Baseline comparison |

---

## 🚀 Performance Tuning

### For Faster Benchmarks
```bash
python3 docker_benchmarks_unified.py \
  --workload relational \
  --duration 30 \
  --docker-file lite
```

### For More Accurate Results
```bash
python3 docker_benchmarks_unified.py \
  --workload all \
  --duration 300 \
  --docker-file extended
```

---

## 📞 Support

**Questions about:**
- **Docker Setup:** See `DOCKER_QUICKSTART.md`
- **Gap Analysis:** See `gap_analysis/historical_gaps.md`
- **Release Process:** See `RELEASE_AND_BENCHMARKING_SESSION_SUMMARY.md`
- **v1.0.1 Plan:** See `V1.0.1_EXECUTION_PLAYBOOK.md`

---

## ✅ Checklist

- [ ] Docker installiert (v24+)
- [ ] Docker Compose installiert (v2+)
- [ ] Python 3.8+ verfügbar
- [ ] 16+ GB RAM frei
- [ ] Internet connection stabil
- [ ] Benchmarks erstmals ausführen
- [ ] Reports überprüfen (HTML + JSON)
- [ ] Gap-Closure Rate dokumentieren
- [ ] Ergebnisse committen zu Git

---

## 🎊 Ready to Benchmark!

```bash
cd benchmarks
python3 docker_benchmarks_unified.py --workload relational --duration 60
firefox docker_benchmarks_results_*/reports/benchmark_results.html
```

**Erwartete Dauer:** 30-120 Minuten (abhängig von Workload)

---

**Version:** 1.0.0  
**Status:** ✅ Production Ready  
**Last Updated:** 2026-04-06
