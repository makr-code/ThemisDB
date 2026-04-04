# Performance Dashboard

Zentrale Performance-Überwachung und Visualisierung für ThemisDB Benchmarks.

## 🎯 Features

- ✅ **Zentrale Visualisierung** - Grafana Dashboard mit historischen Trends
- ✅ **Regression-Erkennung** - Automatische Erkennung von Performance-Regressions
- ✅ **CI/CD Integration** - Automatische Performance-Checks in PRs
- ✅ **Alerts & Monitoring** - Konfigurierbare Schwellwerte und Benachrichtigungen
- ✅ **Branch/Release Vergleich** - Performance über Branches und Releases hinweg
- ✅ **Hardware Comparison** - Vergleich verschiedener Hardware-Konfigurationen

## 🚀 Quick Start

```bash
# 1. Start Dashboard Stack
cd grafana
docker-compose up -d

# 2. Access Dashboard
# Open http://localhost:3000 (admin/admin)

# 3. Run Benchmarks
cd ..
cmake -B build -DBUILD_BENCHMARKS=ON
cmake --build build
./build/bench_crud --benchmark_format=json --benchmark_out=results.json

# 4. Track Results
python3 benchmarks/performance_tracker.py \
  --results results.json \
  --storage benchmarks/performance_data
```

Dashboard ist jetzt verfügbar mit Ihren Benchmark-Daten!

## 📊 Dashboard Components

### Main Dashboard
- **Throughput Trends**: CRUD, LLM, Vector Search Performance
- **Latency Percentiles**: P99/P95/P50 für alle Operationen
- **Error Rates**: Fehlerraten-Tracking
- **Regressions**: Critical, Major, Minor Regressions

### Alerts
- Performance-Regressions (>5%, >10%, >20%)
- Latenz-Schwellwerte (P99 > 100ms)
- Fehlerraten (>1%, >5%)
- Throughput unter Baseline

## 🔧 Tools

### Performance Tracker
Sammelt und speichert Benchmark-Ergebnisse:
```bash
python3 benchmarks/performance_tracker.py \
  --results <path> \
  --storage benchmarks/performance_data \
  --export-baseline benchmarks/baselines/main/latest.json
```

### Baseline Manager
Verwaltet Performance-Baselines:
```bash
# Baseline speichern
python3 benchmarks/baseline_manager.py save \
  --results build/benchmark_results \
  --branch main \
  --version 1.4.1 \
  --commit $(git rev-parse HEAD)

# Baselines auflisten
python3 benchmarks/baseline_manager.py list
```

### Regression Detector
Vergleicht Ergebnisse mit Baseline:
```bash
python3 benchmarks/performance_regression_detector.py \
  --baseline benchmarks/baselines/main/latest.json \
  --current build/results.json \
  --fail-on major
```

## 📖 Dokumentation

- [Vollständige Dokumentation (DE)](docs/de/PERFORMANCE_DASHBOARD.md)
- [Quick Start Guide (EN)](docs/en/PERFORMANCE_DASHBOARD_QUICKSTART.md)
- [Example Charts & Interpretation](docs/en/PERFORMANCE_DASHBOARD_EXAMPLES.md)
- [Benchmark Best Practices](BENCHMARK_BEST_PRACTICES.md)

## 🔄 CI/CD Integration

### Automatische PR-Checks
Bei jedem Pull Request:
1. Benchmarks werden automatisch ausgeführt
2. Ergebnisse mit Baseline verglichen
3. PR-Kommentar mit Regression-Report
4. PR blockiert bei Major/Critical Regressions

### Automatische Baseline-Updates
Bei Push zu main/develop oder Releases:
1. Full Benchmark Suite läuft
2. Neue Baseline wird erstellt
3. Baseline committed und gepusht
4. Artifacts für 90 Tage gespeichert

## 📈 Metriken

### Prometheus Metriken
- `themisdb_benchmark_throughput_ops` - Durchsatz in ops/sec
- `themisdb_benchmark_latency_ms` - Latenz-Percentile
- `themisdb_benchmark_error_rate` - Fehlerraten
- `themisdb_regression_count` - Anzahl Regressions nach Severity
- `llm_tokens_generated_total` - LLM Token Generation
- `themisdb_benchmark_vector_search_ops` - Vector Search Performance

### Alert Rules
Alle Alert-Regeln in: `grafana/alerts/performance_regression_alerts.yaml`

## 🎯 Schwellwerte

| Severity | Threshold | Action |
|----------|-----------|--------|
| **Minor** | 5-10% Regression | ℹ️ Review empfohlen |
| **Major** | 10-20% Regression | ⚠️ PR-Block, Review erforderlich |
| **Critical** | >20% Regression | ❌ PR-Block, sofortige Untersuchung |

## 🔍 Beispiel-Visualisierungen

### Throughput Trend
```
50K │        ╭────────────╮
45K │   ╭────╯            ╰───╮
40K │───╯                     ╰───
    └─────────────────────────────
    v1.3  v1.4.0  v1.4.1  HEAD
```

### Latency Distribution
```
P99 │     *
P95 │   * *
P50 │ * * *
    └──────────
    Read Write Query
```

## 🛠️ Wartung

### Dashboard-Aktualisierung
```bash
# Dashboard neu laden
cd grafana
docker-compose restart grafana
```

### Datenbereinigung
```bash
# Alte Metriken löschen (>30 Tage)
find benchmarks/performance_data/raw -mtime +30 -delete
```

### Baseline-Backup
```bash
# Baselines sichern
tar czf baselines-backup-$(date +%Y%m%d).tar.gz benchmarks/baselines/
```

## 🐛 Troubleshooting

### Dashboard zeigt keine Daten
```bash
# 1. Prüfe Prometheus
curl http://localhost:9090/api/v1/targets

# 2. Prüfe Metriken
ls -la benchmarks/performance_data/metrics/

# 3. Neustart
cd grafana && docker-compose restart
```

### Alerts werden nicht ausgelöst
```bash
# Validiere Alert Rules
promtool check rules grafana/alerts/performance_regression_alerts.yaml

# Prüfe aktive Alerts
curl http://localhost:9090/api/v1/alerts
```

## 📞 Support

- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Label: `performance` oder `dashboard`
- Performance Team: performance-team@themisdb.io

## 🗺️ Roadmap

- [ ] Machine Learning Anomalie-Erkennung
- [ ] Automatische wöchentliche Performance-Reports
- [ ] GitHub Issues Integration (Auto-Create)
- [ ] A/B Testing zwischen Branches
- [ ] Mobile Dashboard App
- [ ] Kosten-Analyse (Performance vs. Ressourcen)

---

**Status:** ✅ Production Ready  
**Version:** 1.0.0  
**Letzte Aktualisierung:** 2026-02-02
