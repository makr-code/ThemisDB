# Grafana Integration für ThemisDB

Umfassendes Monitoring und Visualisierung für ThemisDB - LLM-Subsystem und SIEM-Sicherheitsüberwachung.

## Übersicht

Diese Grafana-Integration bietet Echtzeit-Monitoring für:

### LLM/llama.cpp Monitoring
- **Inference Performance** (Latenz, Throughput, Tokens/sec)
- **GPU Metriken** (Memory, Utilization, Temperature)
- **Model Management** (Loaded Models, Memory Usage)
- **Cache Performance** (Hit Rates, Efficiency)
- **Scheduler Status** (Queue Length, Batch Size, Preemptions)
- **Error Tracking** (Fehlerrate, Error Types)

### SIEM Security Monitoring (NEU)
- **Authentication & Authorization** (Failed Logins, Privilege Escalation, Rate Limiting)
- **Audit & Security Events** (CRUD, Admin Actions, Policy Checks, Security Incidents)
- **Query & Performance** (Error Rate, Slow Queries, Request Rate, Cache Hit Rate)
- **Infrastructure** (CPU, Memory, Storage, Network, Replication)
- **Compliance** (SOC2, GDPR, HIPAA)

## Verzeichnisstruktur

```
grafana/
├── dashboards/
│   ├── themisdb-llm-dashboard.json           # LLM Monitoring Dashboard
│   └── sla-monitoring.json
├── siem-security-monitoring.json             # SIEM Security Dashboard (NEU)
├── alerts/
│   ├── graph_security.yaml                   # Graph Security Alerts
│   └── siem_security_alerts.yaml             # SIEM Security Alerts (NEU)
├── provisioning/
│   ├── datasources/
│   │   └── prometheus.yml                    # Prometheus Datasource
│   ├── dashboards/
│   │   └── dashboards.yml                    # Dashboard Provisioning
│   └── alerts.yml                            # Alert Rules
├── compliance_exporter.py                    # Compliance Report Generator (NEU)
├── COMPLIANCE_EXPORTER_README.md             # Compliance Exporter Documentation (NEU)
├── docker-compose.yml                        # Docker Setup
├── prometheus.yml                            # Prometheus Configuration
└── README.md                                 # Diese Datei
```

## Quick Start

### Option 1: Docker Compose (Empfohlen)

```bash
cd grafana
docker-compose up -d
```

Öffne Browser:
- **Grafana**: http://localhost:3000 (admin/admin)
- **Prometheus**: http://localhost:9090
- **ThemisDB Metrics**: http://localhost:9091/metrics

### Option 2: Manuelle Installation

**1. Prometheus Setup**

```bash
# Download Prometheus
wget https://github.com/prometheus/prometheus/releases/download/v2.45.0/prometheus-2.45.0.linux-amd64.tar.gz
tar xvfz prometheus-*.tar.gz
cd prometheus-*

# Create config
cat > prometheus.yml <<EOF
global:
  scrape_interval: 5s
  evaluation_interval: 5s

scrape_configs:
  - job_name: 'themisdb-llm'
    static_configs:
      - targets: ['localhost:9091']
    metrics_path: '/metrics'
    
rule_files:
  - 'alerts.yml'
EOF

# Copy alert rules
cp ../provisioning/alerts.yml .

# Start Prometheus
./prometheus --config.file=prometheus.yml
```

**2. Grafana Setup**

```bash
# Install Grafana
sudo apt-get install -y software-properties-common
sudo add-apt-repository "deb https://packages.grafana.com/oss/deb stable main"
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
sudo apt-get update
sudo apt-get install grafana

# Copy provisioning files
sudo cp -r provisioning/* /etc/grafana/provisioning/
sudo cp dashboards/*.json /etc/grafana/provisioning/dashboards/

# Start Grafana
sudo systemctl start grafana-server
sudo systemctl enable grafana-server
```

**3. ThemisDB Metrics aktivieren**

```cpp
#include "llm/grafana_metrics.h"

using namespace themis::llm::monitoring;

// Initialize
PrometheusExporter exporter;
LLMMetricsCollector metrics(&exporter);

// Start Metrics Server
MetricsServer::ServerConfig config;
config.port = 9091;  // ThemisDB metrics port
MetricsServer server(config, &exporter);
server.start();

// Record metrics
metrics.recordInferenceRequest("mistral-7b");
metrics.recordFirstTokenLatency("mistral-7b", 72.5);
metrics.recordGPUMemoryUsage(4096, 24576);
```

## Dashboard Import

### Auto-Import (mit Provisioning)
Dashboard wird automatisch geladen wenn Grafana mit Provisioning-Konfiguration startet.

### Manueller Import
1. Öffne Grafana (http://localhost:3000)
2. Login: `admin` / `admin`
3. Gehe zu **Dashboards → Import**
4. Lade `dashboards/themisdb-llm-dashboard.json`
5. Wähle `Prometheus` als Datasource
6. Klicke **Import**

## Verfügbare Metriken

### Inference Metriken
| Metrik | Typ | Beschreibung | Labels |
|--------|-----|--------------|--------|
| `llm_inference_requests_total` | Counter | Gesamtzahl Inference Requests | `model_id` |
| `llm_inference_duration_ms` | Histogram | Inference Dauer in ms | `model_id` |
| `llm_inference_failures_total` | Counter | Fehlgeschlagene Requests | `model_id`, `error` |

### Latenz Metriken
| Metrik | Typ | Beschreibung | Labels |
|--------|-----|--------------|--------|
| `llm_first_token_latency_ms` | Histogram | Zeit bis erstes Token | `model_id` |
| `llm_per_token_latency_ms` | Histogram | Latenz pro Token | `model_id` |

### Throughput Metriken
| Metrik | Typ | Beschreibung | Labels |
|--------|-----|--------------|--------|
| `llm_tokens_generated_total` | Counter | Generierte Tokens | `model_id` |
| `llm_batch_size` | Gauge | Aktuelle Batch-Größe | - |
| `llm_concurrent_requests` | Gauge | Gleichzeitige Requests | - |

### GPU Metriken
| Metrik | Typ | Beschreibung | Labels |
|--------|-----|--------------|--------|
| `llm_gpu_memory_used_mb` | Gauge | GPU Speicher verwendet (MB) | - |
| `llm_gpu_memory_total_mb` | Gauge | GPU Speicher gesamt (MB) | - |
| `llm_gpu_utilization_pct` | Gauge | GPU Auslastung (%) | - |
| `llm_gpu_temperature_celsius` | Gauge | GPU Temperatur (°C) | - |

### Model Metriken
| Metrik | Typ | Beschreibung | Labels |
|--------|-----|--------------|--------|
| `llm_models_loaded` | Gauge | Anzahl geladener Modelle | - |
| `llm_model_memory_mb` | Gauge | Speicher pro Modell (MB) | `model_id` |

### Cache Metriken
| Metrik | Typ | Beschreibung | Labels |
|--------|-----|--------------|--------|
| `llm_cache_hits_total` | Counter | Cache Hits | `cache_type` |
| `llm_cache_misses_total` | Counter | Cache Misses | `cache_type` |

### Scheduler Metriken
| Metrik | Typ | Beschreibung | Labels |
|--------|-----|--------------|--------|
| `llm_scheduler_queue_length` | Gauge | Warteschlangen-Länge | - |
| `llm_scheduler_preemptions_total` | Counter | Preemptions | - |

### Error Metriken
| Metrik | Typ | Beschreibung | Labels |
|--------|-----|--------------|--------|
| `llm_errors_total` | Counter | Fehler nach Typ | `error_type`, `component` |

## Dashboard Panels

### 1. Inference Requests per Second
- **Query**: `rate(llm_inference_requests_total[1m])`
- **Zeigt**: Request-Rate pro Model
- **Alert**: >100 req/s

### 2. Latency Distribution  
- **Query**: `histogram_quantile(0.95, llm_first_token_latency_ms_bucket)`
- **Zeigt**: P50, P95, P99 First Token Latency

### 3. GPU Memory Usage
- **Query**: `llm_gpu_memory_used_mb / llm_gpu_memory_total_mb * 100`
- **Zeigt**: GPU Memory % Auslastung

### 4. Throughput
- **Query**: `rate(llm_tokens_generated_total[1m])`
- **Zeigt**: Tokens/sec pro Model

### 5. Cache Hit Rate
- **Query**: `llm_cache_hits / (llm_cache_hits + llm_cache_misses) * 100`
- **Zeigt**: Cache Effizienz

## PromQL Beispiele

### Durchschnittliche Latenz (5min)
```promql
avg(rate(llm_first_token_latency_ms_sum[5m])) 
  / 
avg(rate(llm_first_token_latency_ms_count[5m]))
```

### Erfolgsrate
```promql
(1 - (rate(llm_inference_failures_total[5m]) 
      / 
      rate(llm_inference_requests_total[5m]))) * 100
```

### GPU Speicher Auslastung
```promql
(llm_gpu_memory_used_mb / llm_gpu_memory_total_mb) * 100
```

### Tokens pro Request
```promql
rate(llm_tokens_generated_total[1m]) 
  / 
rate(llm_inference_requests_total[1m])
```

## Alerts

Vorkonfigurierte Alerts in `provisioning/alerts.yml`:

### Latency Alerts
- **HighFirstTokenLatency**: P95 > 100ms für 5min (Warning)
- **CriticalFirstTokenLatency**: P95 > 200ms für 2min (Critical)

### Error Alerts
- **HighErrorRate**: >5% für 5min (Warning)
- **CriticalErrorRate**: >10% für 2min (Critical)

### GPU Alerts
- **GPUMemoryHigh**: >85% für 5min (Warning)
- **GPUMemoryCritical**: >95% für 2min (Critical)
- **GPUTemperatureHigh**: >80°C für 5min (Warning)
- **GPUTemperatureCritical**: >90°C für 1min (Critical)

### System Alerts
- **LowThroughput**: <100 tokens/sec für 10min (Warning)
- **HighQueueLength**: >50 requests für 5min (Warning)
- **NoInferenceRequests**: Keine Requests für 10min (Warning)

## Integration mit Code

### LazyModelLoader
```cpp
void LazyModelLoader::loadModelInternal(...) {
    metrics_->recordModelLoaded(model_id, vram_mb);
}

void LazyModelLoader::unloadModel(...) {
    metrics_->recordModelUnloaded(model_id);
}
```

### ContinuousBatchScheduler
```cpp
void ContinuousBatchScheduler::scheduleNextBatch() {
    auto start = std::chrono::steady_clock::now();
    // ... scheduling ...
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start
    ).count();
    
    metrics_->recordSchedulingLatency(duration);
    metrics_->recordQueueLength(waiting_queue_.size());
    metrics_->recordBatchSize(batch.size());
}
```

### GPU Memory Manager
```cpp
void GPUMemoryManager::allocateGPU(...) {
    // ... allocation ...
    auto stats = getStats();
    metrics_->recordGPUMemoryUsage(
        stats.used_vram_bytes / (1024*1024),
        stats.total_vram_bytes / (1024*1024)
    );
}
```

## Troubleshooting

### Metriken werden nicht angezeigt
```bash
# Check metrics endpoint
curl http://localhost:9091/metrics

# Check Prometheus targets
open http://localhost:9090/targets

# Check ThemisDB logs
docker logs themisdb -f
```

### Dashboard zeigt keine Daten
1. Prüfe Datasource: **Configuration → Data Sources → Prometheus**
2. Teste Connection: **Save & Test** sollte "Data source is working"
3. Prüfe Time Range: Letzte 1 Stunde
4. Prüfe Query in Panel Edit Mode

### Alerts feuern nicht
```bash
# Check Prometheus rules
open http://localhost:9090/rules

# Check Alertmanager
open http://localhost:9093

# Validate alert syntax
promtool check rules provisioning/alerts.yml
```

## Best Practices

### 1. Scrape Interval
- **Empfohlen**: 5-10 Sekunden
- **Config**: `scrape_interval: 5s` in `prometheus.yml`

### 2. Retention
- **Empfohlen**: 15 Tage
- **Config**: `--storage.tsdb.retention.time=15d`
- **Disk**: ~50 MB/Tag → ~750 MB für 15 Tage

### 3. Label Cardinality
- **Limit**: Halte `model_id` unter 100 unique values
- **Avoid**: Freie Strings als Labels (nutze feste Werte)

### 4. Recording Rules
Für häufige Queries:
```yaml
groups:
  - name: llm_recording_rules
    interval: 30s
    rules:
      - record: llm:inference_rate:1m
        expr: rate(llm_inference_requests_total[1m])
      
      - record: llm:success_rate:5m
        expr: (1 - (rate(llm_inference_failures_total[5m]) / rate(llm_inference_requests_total[5m]))) * 100
```

## SIEM Security Monitoring (NEU)

### Verfügbare SIEM-Dashboards

**ThemisDB SIEM Security Monitoring** (`siem-security-monitoring.json`)
- Umfassendes Security-Dashboard für SOC-Teams
- 4 Hauptbereiche: Authentication, Audit Events, Query Performance, Infrastructure
- Echtzeit-Bedrohungserkennung
- Compliance-Mapping (SOC2, GDPR, HIPAA)

### Dokumentation

- **Integration Guide**: `docs/en/observability/siem_integration.md` (English)
- **Integration Guide**: `docs/de/observability/siem_integration.md` (Deutsch)
- **User Guide**: `docs/en/observability/siem_dashboard_user_guide.md`
- **Compliance Exporter**: `COMPLIANCE_EXPORTER_README.md`

### SIEM-relevante Alarme

Alle SIEM-Alarme sind in `alerts/siem_security_alerts.yaml` definiert:

**Kritische Alarme**:
- `BruteForceAttackDetected` - Mehrfache fehlgeschlagene Logins
- `PrivilegeEscalationDetected` - Unbefugte Rechteänderungen
- `UnauthorizedDataExport` - Datenexfiltration
- `AuditLogTamperingAttempt` - Audit-Log-Manipulation

**Compliance-Alarme**:
- `GDPRDataRetentionViolation` - DSGVO-Datenhaltung
- `SOC2AuditLogGapDetected` - SOC2-Audit-Lücken
- `EncryptionKeyRotationOverdue` - Schlüsselrotation überfällig
- `BackupFailure` - Backup-Fehler

### Compliance-Reporting

Automatische Compliance-Berichte generieren:

```bash
# SOC2-Bericht für letzte 30 Tage (PDF)
python3 compliance_exporter.py --framework soc2 --period 30d

# GDPR-Bericht (JSON)
python3 compliance_exporter.py --framework gdpr --period 7d --format json

# HIPAA-Bericht (CSV)
python3 compliance_exporter.py --framework hipaa --period 90d --format csv
```

Siehe `COMPLIANCE_EXPORTER_README.md` für Details.

### Integration mit SIEM-Systemen

**Splunk**:
```bash
pip install prometheus-splunk-exporter
# Siehe docs/en/observability/siem_integration.md für Konfiguration
```

**ELK Stack**:
- Logstash-Konfiguration verfügbar
- Siehe docs/en/observability/siem_integration.md

**Syslog (RFC 5424)**:
- Native Unterstützung in ThemisDB
- Strukturierte Ereignisse mit Compliance-Kontext

## Support

- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Security Contact**: security@themisdb.io
- **Metrics Endpoint**: http://localhost:9091/metrics
- **Prometheus UI**: http://localhost:9090
- **Grafana UI**: http://localhost:3000
