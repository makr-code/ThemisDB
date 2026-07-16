# Performance Dashboard - Dokumentation

## Übersicht

Das **ThemisDB Performance Dashboard** ist ein zentrales System zur Visualisierung, Überwachung und Analyse der Performance-Benchmarks über alle Releases, Branches und Pull Requests hinweg.

## Features

### ✅ Implementierte Features

1. **Zentrale Visualisierung**
   - Grafana-Dashboard mit historischen Trends
   - Durchsatz-Metriken (ops/sec, tokens/sec)
   - Latenz-Perzentile (P50, P95, P99)
   - Fehlerraten-Tracking
   
2. **Branch & Release Vergleiche**
   - Vergleich zwischen main, develop und feature branches
   - Release-über-Release Performance-Tracking
   - Hardware-Vergleich verschiedener Konfigurationen

3. **Automatische Regression-Erkennung**
   - Konfigurierbare Schwellwerte (minor, major, critical)
   - CI/CD Integration mit automatischen Checks
   - PR-Kommentare bei Regressionen
   - Slack/E-Mail Benachrichtigungen

4. **Historische Datenspeicherung**
   - Time-Series Datenbank für langfristige Trends
   - Baseline-Management für Vergleiche
   - Prometheus Metrics Export

5. **CI/CD Pipeline Integration**
   - Automatische Baseline-Updates
   - Performance-Checks in Pull Requests
   - Status-Reporting

## Architektur

```
┌─────────────────────────────────────────────────────────────────┐
│                    Performance Dashboard System                  │
└─────────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌──────────────┐    ┌──────────────────┐   ┌──────────────┐
│  Benchmarks  │───▶│ Performance      │──▶│  Prometheus  │
│   (C++/Go)   │    │   Tracker        │   │   Metrics    │
└──────────────┘    └──────────────────┘   └──────────────┘
                             │                      │
                             ▼                      ▼
                    ┌──────────────────┐   ┌──────────────┐
                    │  Time-Series     │   │   Grafana    │
                    │    Storage       │   │  Dashboard   │
                    └──────────────────┘   └──────────────┘
                             │
                             ▼
                    ┌──────────────────┐
                    │  Regression      │
                    │   Detection      │
                    └──────────────────┘
                             │
                             ▼
                    ┌──────────────────┐
                    │  Alerts & PR     │
                    │   Comments       │
                    └──────────────────┘
```

## Komponenten

### 1. Grafana Dashboard

**Datei:** `grafana/performance-dashboard.json`

Das zentrale Dashboard visualisiert:

- **Regression Overview**: Critical, Major, Minor Regressions im Überblick
- **Throughput Trends**: CRUD, LLM Token Generation, Vector Search
- **Latency Percentiles**: P99/P95/P50 für alle Benchmarks
- **Error Rate Tracking**: Fehlerraten über Zeit
- **Branch Comparisons**: main vs develop vs feature branches
- **Release Comparisons**: Performance-Entwicklung über Releases
- **Hardware Comparisons**: Performance auf verschiedenen Systemen

**Zugriff:**
- Lokale Entwicklung: http://localhost:3000
- Template-Variablen: Branch, Release, Hardware (dynamisch)
- Zeitbereich: Letzte 7 Tage (konfigurierbar)

### 2. Alert Rules

**Datei:** `grafana/alerts/performance_regression_alerts.yaml`

Vorkonfigurierte Alerts für:

- **Critical Regressions**: >20% Performance-Degradation
- **Major Regressions**: 10-20% Degradation
- **Minor Regressions**: 5-10% Degradation
- **Latency Thresholds**: P99 > 100ms (Warning), > 500ms (Critical)
- **Error Rate**: >5% (Warning), >10% (Critical)
- **Throughput**: Unter Baseline-Werten

**Alert-Kanäle:**
- Grafana (Dashboard)
- Slack Webhooks
- E-Mail (konfigurierbar)
- GitHub Issues (automatisch)

### 3. Performance Tracker

**Datei:** `benchmarks/performance_tracker.py`

Python-Tool zum:

- Sammeln von Benchmark-Ergebnissen
- Speichern in Time-Series Format
- Export zu Prometheus Metrics
- Baseline-Export für Regression Detection

**Verwendung:**

```bash
# Benchmark-Ergebnisse tracken
python3 benchmarks/performance_tracker.py \
  --results build/benchmark_results \
  --storage benchmarks/performance_data \
  --branch main \
  --commit $(git rev-parse HEAD) \
  --release v1.4.1 \
  --export-baseline benchmarks/baselines/main/latest.json

# Gespeicherte Daten:
# - benchmarks/performance_data/raw/           # Raw JSON results
# - benchmarks/performance_data/aggregated/    # Time-series data
# - benchmarks/performance_data/metrics/       # Prometheus metrics
```

### 4. Baseline Manager

**Datei:** `benchmarks/baseline_manager.py`

Verwaltet Benchmark-Baselines für Regression Detection:

```bash
# Neue Baseline speichern
python3 benchmarks/baseline_manager.py save \
  --results build/benchmark_results \
  --branch main \
  --version 1.4.1 \
  --commit abc123 \
  --release

# Baseline laden
python3 benchmarks/baseline_manager.py load --branch main

# Alle Baselines auflisten
python3 benchmarks/baseline_manager.py list
```

**Baseline-Struktur:**

```
benchmarks/baselines/
├── main/
│   └── latest.json              # Aktuellste main branch baseline
├── develop/
│   └── latest.json              # Aktuellste develop branch baseline
└── releases/
    ├── v1.4.0.json              # Release baselines
    └── v1.4.1.json
```

### 5. Regression Detector

**Datei:** `benchmarks/performance_regression_detector.py`

Vergleicht aktuelle Ergebnisse mit Baseline:

```bash
# Regressionen erkennen
python3 benchmarks/performance_regression_detector.py \
  --baseline benchmarks/baselines/main/latest.json \
  --current build/benchmark_results/current.json \
  --output regression_report.txt \
  --fail-on major \
  --threshold-minor 5.0 \
  --threshold-major 10.0 \
  --threshold-critical 20.0

# Exit codes:
# 0 = Keine blockierenden Regressionen
# 1 = Regressionen über Schwellwert gefunden
```

## CI/CD Integration

### Pull Request Performance Checks

**Workflow:** `.github/workflows/performance-regression-check.yml`

Automatisch ausgeführt bei:
- Pull Requests zu main/develop
- Änderungen in `src/`, `include/`, `benchmarks/`

**Prozess:**

1. Build ThemisDB (Release)
2. Ausführen Core Benchmarks
3. Vergleich mit Baseline
4. Regression Detection
5. PR-Kommentar mit Ergebnis
6. Status Check (Block bei Major/Critical)

### Baseline Updates

**Workflow:** `.github/workflows/update-performance-baselines.yml`

Automatisch ausgeführt bei:
- Push zu main branch
- Push zu develop branch
- Release tags (v*)

**Prozess:**

1. Build ThemisDB (Release)
2. Ausführen Full Benchmark Suite
3. Baseline erstellen/aktualisieren
4. Commit und Push Baseline
5. Artifact Upload (90 Tage Retention)

## Setup & Installation

### Lokale Entwicklung

#### 1. Grafana & Prometheus mit Docker

```bash
cd grafana
docker-compose up -d

# Services:
# - Grafana:    http://localhost:3000 (admin/admin)
# - Prometheus: http://localhost:9090
# - ThemisDB:   http://localhost:8080
```

#### 2. Dashboard Import

Das Dashboard wird automatisch über Provisioning geladen. Alternativ manueller Import:

1. Öffne Grafana: http://localhost:3000
2. Navigiere zu **Dashboards** → **Import**
3. Lade `grafana/performance-dashboard.json` hoch
4. Wähle **Prometheus** als Datasource

#### 3. Benchmarks ausführen und tracken

```bash
# 1. Build mit Benchmarks
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build --config Release

# 2. Benchmarks ausführen
cd build
./bench_crud --benchmark_format=json --benchmark_out=results.json

# 3. Performance tracken
cd ..
python3 benchmarks/performance_tracker.py \
  --results build/results.json \
  --storage benchmarks/performance_data

# 4. Metriken exportieren
# Prometheus scraped automatisch von benchmarks/performance_data/metrics/benchmarks.prom
```

### Production Setup

#### 1. Prometheus Configuration

```yaml
# prometheus.yml
global:
  scrape_interval: 30s
  evaluation_interval: 30s

scrape_configs:
  - job_name: 'themisdb-benchmarks'
    static_configs:
      - targets: ['localhost:9091']
    file_sd_configs:
      - files:
        - '/path/to/benchmarks/performance_data/metrics/*.prom'
        refresh_interval: 30s

rule_files:
  - '/path/to/grafana/alerts/performance_regression_alerts.yaml'

alerting:
  alertmanagers:
    - static_configs:
        - targets: ['localhost:9093']
```

#### 2. Alert Manager Configuration

```yaml
# alertmanager.yml
route:
  receiver: 'team-performance'
  group_by: ['alertname', 'severity']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h

receivers:
  - name: 'team-performance'
    slack_configs:
      - api_url: 'YOUR_SLACK_WEBHOOK_URL'
        channel: '#performance-alerts'
        title: 'Performance Alert: {{ .GroupLabels.alertname }}'
    email_configs:
      - to: 'performance-team@example.com'
        from: 'alertmanager@example.com'
```

## Metriken

### Verfügbare Prometheus Metriken

```promql
# Throughput
themisdb_benchmark_throughput_ops{benchmark, branch, commit, release, hardware}

# Latency Percentiles
themisdb_benchmark_latency_ms{benchmark, branch, quantile="0.50|0.95|0.99"}

# Error Rate
themisdb_benchmark_error_rate{benchmark, branch}

# Regression Counts
themisdb_regression_count{severity="critical|major|minor"}

# Benchmark Runs
themisdb_benchmark_runs_total{status="success|failed"}

# LLM-spezifisch
llm_tokens_generated_total{model_id, branch}
llm_first_token_latency_ms{model_id}

# Vector Search
themisdb_benchmark_vector_search_ops{index_type}

# Query Performance
themisdb_benchmark_query_total{query_type, branch}
```

### Beispiel-Queries

```promql
# Durchschnittlicher Durchsatz über letzte Stunde
avg_over_time(themisdb_benchmark_throughput_ops{benchmark="crud"}[1h])

# P99 Latenz Trend
histogram_quantile(0.99, 
  rate(themisdb_benchmark_latency_ms_bucket[5m])
)

# Fehlerrate Prozentsatz
(rate(themisdb_benchmark_errors_total[5m]) / 
 rate(themisdb_benchmark_requests_total[5m])) * 100

# Performance-Vergleich main vs develop
avg(themisdb_benchmark_throughput_ops{branch="main"}) /
avg(themisdb_benchmark_throughput_ops{branch="develop"})

# Top 10 Regressionen
topk(10, abs(
  (themisdb_benchmark_throughput_ops - 
   themisdb_benchmark_throughput_ops offset 1h) /
  themisdb_benchmark_throughput_ops offset 1h * 100
))
```

## Status Badge

### GitHub Workflow Status Badge

Füge diese Badge zu README.md hinzu:

```markdown
![Performance](https://github.com/makr-code/ThemisDB/actions/workflows/05-quality_build_cross-module-performance-regression-ci.yml/badge.svg)
```

### Shields.io Custom Badge

Erstelle dynamische Performance-Badge:

```markdown
![Throughput](https://img.shields.io/endpoint?url=https://your-server.com/api/badges/throughput)
```

**Badge-Endpoint implementieren:**

```python
# tools/badge_endpoint.py
from flask import Flask, jsonify
import json

app = Flask(__name__)

@app.route('/api/badges/throughput')
def throughput_badge():
    # Lade aktuelle Performance-Daten
    with open('benchmarks/performance_data/aggregated/timeseries.json') as f:
        data = json.load(f)
    
    # Berechne durchschnittlichen Durchsatz
    throughput = calculate_average_throughput(data)
    
    # Shields.io JSON Format
    return jsonify({
        "schemaVersion": 1,
        "label": "Throughput",
        "message": f"{throughput:,.0f} ops/sec",
        "color": "green" if throughput > 40000 else "orange"
    })
```

## Beispiel-Charts

### 1. Throughput Trend über 30 Tage

Zeigt CRUD-Performance:
- main: Stabil bei ~45K ops/sec
- develop: ~42K ops/sec
- Releases markiert mit Annotationen

### 2. Latency Distribution

P99/P95/P50 für verschiedene Operationen:
- Write: P99 < 5ms
- Read: P99 < 2ms
- Query: P99 < 50ms

### 3. Error Rate Heatmap

Fehlerraten nach Benchmark und Zeitraum:
- Grün: < 1%
- Gelb: 1-5%
- Rot: > 5%

### 4. Branch Comparison

Side-by-side Vergleich:
- main vs develop
- Vor/Nach große Features
- Hardware-Varianten

## Troubleshooting

### Dashboard zeigt keine Daten

```bash
# 1. Prüfe Prometheus Targets
curl http://localhost:9090/api/v1/targets

# 2. Prüfe Metriken
curl http://localhost:9090/api/v1/query?query=themisdb_benchmark_throughput_ops

# 3. Prüfe Grafana Datasource
# In Grafana: Configuration → Data Sources → Prometheus → Test

# 4. Prüfe Performance Tracker
python3 benchmarks/performance_tracker.py --help
```

### Alerts werden nicht ausgelöst

```bash
# 1. Validiere Alert Rules
promtool check rules grafana/alerts/performance_regression_alerts.yaml

# 2. Prüfe aktive Alerts
curl http://localhost:9090/api/v1/alerts

# 3. Prüfe Alertmanager
curl http://localhost:9093/api/v2/alerts
```

### Baseline-Update schlägt fehl

```bash
# 1. Prüfe Baseline-Verzeichnis
ls -la benchmarks/baselines/

# 2. Manuelles Baseline-Update
python3 benchmarks/baseline_manager.py save \
  --results build/benchmark_results \
  --branch main \
  --version $(cat VERSION) \
  --commit $(git rev-parse HEAD)

# 3. Prüfe Git-Berechtigungen
git config --list | grep user
```

## Best Practices

### 1. Baseline-Management

- ✅ Baselines nach jedem Release erstellen
- ✅ Wöchentliche Baseline-Updates für main branch
- ✅ Baselines für mindestens 3 Monate aufbewahren
- ❌ Nicht zu häufig (< täglich) Baselines erstellen

### 2. Performance Monitoring

- ✅ Überwache P99 Latenz für kritische Operationen
- ✅ Setze realistische Schwellwerte (5%, 10%, 20%)
- ✅ Nutze Hardware-Labels für faire Vergleiche
- ❌ Vergleiche nicht verschiedene Hardware-Konfigurationen direkt

### 3. Regression Detection

- ✅ Block PRs mit Major/Critical Regressionen
- ✅ Untersuche auch Minor Regressionen
- ✅ Dokumentiere bekannte Performance-Änderungen
- ❌ Nicht alle Regressionen sind Bugs (manchmal Trade-offs)

### 4. Dashboard-Nutzung

- ✅ Regelmäßig Review (wöchentlich)
- ✅ Lange Zeiträume (7-30 Tage) für Trends
- ✅ Kurze Zeiträume (Stunden) für Debugging
- ✅ Export Charts für Dokumentation

## Support & Weiterentwicklung

### Geplante Features (Roadmap)

- [ ] Machine Learning für Anomalie-Erkennung
- [ ] Automatische Performance-Reports (wöchentlich/monatlich)
- [ ] Integration mit GitHub Issues (automatische Erstellung)
- [ ] A/B Testing zwischen branches
- [ ] Kosten-Analyse (Performance vs. Ressourcen)
- [ ] Mobile Dashboard App

### Kontakt

- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Performance Team**: performance-team@themisdb.io
- **Dokumentation**: https://github.com/makr-code/ThemisDB/tree/main/docs

## Ressourcen

- [CHIMERA Benchmark Suite](../benchmarks/README.md)
- [Grafana Dokumentation](https://grafana.com/docs/)
- [Prometheus Best Practices](https://prometheus.io/docs/practices/)
- [Google Benchmark Library](https://github.com/google/benchmark)
