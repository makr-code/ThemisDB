> **Hinweis:** Docker-Kommandos gegen aktuellen Stand der Compose-Dateien und Images prüfen.

#!/usr/bin/env markdown
# Docker Comparative Benchmarks - Quick Start

**Version:** 1.0.1 Gap Validation  
**Status:** ✅ Ready  
**Date:** 2025-12-09

---

## 🚀 Start in 5 Minuten

### 1. Docker prüfen
```bash
docker --version    # v24+
docker compose version  # v2+
```

### 2. Stack starten
```bash
cd benchmarks/comparative
docker compose -f docker-compose.benchmark-optimized.yml up -d
```

### 3. Warten (2-3 Min)
```bash
docker compose ps   # Alle "running"?
```

### 4. Benchmarks starten
```bash
# Relational (15 Min)
python3 ../../scripts/run_docker_comparative_benchmarks.py --workload relational --duration 60

# Oder alle (2 Std)
python3 ../../scripts/run_docker_comparative_benchmarks.py --workload all --duration 120
```

### 5. Reports anschauen
```bash
firefox docker_benchmark_results_*/benchmark_report.html
```

---

## 📊 Gap-Closure prüfen

```bash
# Historische Gaps (v1.0.0)
cat ../gap_analysis/historical_gaps.md

# Targets (v1.0.1)
cat ../gap_analysis/v1.0.1_closure_targets.json | jq '.summary'
```

---

## 🛑 Container cleanup
```bash
cd benchmarks/comparative
docker compose down -v
```

---

**Weitere Details:** `DOCKER_COMPARATIVE_BENCHMARKS_README.md`
