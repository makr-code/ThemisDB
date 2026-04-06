# RAID-Themis Monitoring & Observability v1.4

**Version:** 1.4 (RAID-Angepasst)  
**Stand:** 6. April 2026  
**Status:** ✅ Für internes RAID-Themis System optimiert  
**Kategorie:** 📊 Monitoring | 🔀 Sharding | 🛡️ RAID-Redundanz

---

## Executive Summary

Diese Anleitung beschreibt die **vollständige Monitoring- und Observability-Infrastruktur** für RAID-Themis Sharding-Cluster. Sie umfasst:

- Prometheus Metrics (Sharding + RAID-spezifisch)
- Grafana Dashboards (4 Production-ready)
- AlertManager Rules (5 Alert Groups)
- ELK Stack für Log Aggregation
- Alert Response Playbooks (3 Runbooks)

---

## 📑 Inhaltsverzeichnis

1. [Prometheus Metrics](#1-prometheus-metrics)
2. [Grafana Dashboards](#2-grafana-dashboards)
3. [AlertManager Rules](#3-alertmanager-rules)
4. [ELK Stack Configuration](#4-elk-stack-configuration)
5. [Alert Response Playbooks](#5-alert-response-playbooks)
6. [SLA & KPI Targets](#6-sla--kpi-targets)
7. [Observability Checklist](#7-observability-checklist)

---

## 1. Prometheus Metrics

### 1.1 Metrics Collection Config

```yaml
# /etc/prometheus/prometheus-raid-themis.yml

global:
  scrape_interval: 15s
  evaluation_interval: 15s
  external_labels:
    cluster: 'raid-themis-prod'
    environment: 'production'

# Prometheus Alertmanager
alerting:
  alertmanagers:
    - static_configs:
        - targets: ['alertmanager.prod.internal:9093']

rule_files:
  - '/etc/prometheus/rules/raid-themis-alerts.yml'

scrape_configs:
  # ========================
  # Shard Metrics (8 Shards)
  # ========================
  - job_name: 'themis-shards'
    scrape_interval: 15s
    scrape_timeout: 10s
    
    static_configs:
      - targets:
        - 'shard-001.prod.internal:9090'
        - 'shard-002.prod.internal:9091'
        - 'shard-003.prod.internal:9092'
        - 'shard-004.prod.internal:9093'
        - 'shard-005.prod.internal:9094'
        - 'shard-006.prod.internal:9095'
        - 'shard-007.prod.internal:9096'
        - 'shard-008.prod.internal:9097'
        
        labels:
          job: 'themis-shard'
          cluster: 'raid-themis'
    
    metric_relabel_configs:
      # Gruppierung nach Shard-ID
      - source_labels: [__address__]
        regex: 'shard-([0-9]+).*'
        target_label: shard_id
        replacement: '${1}'
  
  # ========================
  # RocksDB Storage Metrics
  # ========================
  - job_name: 'rocksdb-storage'
    scrape_interval: 30s
    
    static_configs:
      - targets: ['shard-001.prod.internal:9090']
        labels:
          storage: 'rocksdb'
          shard: 'shard-001'
  
  # ========================
  # Replication Metrics
  # ========================
  - job_name: 'replication'
    scrape_interval: 15s
    
    static_configs:
      - targets: ['shard-001.prod.internal:9090']
        labels:
          component: 'replication'
  
  # ========================
  # Raft Consensus Metrics
  # ========================
  - job_name: 'raft-consensus'
    scrape_interval: 15s
    
    static_configs:
      - targets: ['shard-001.prod.internal:9090']
        labels:
          component: 'raft'
  
  # ========================
  # Node-level Metrics
  # ========================
  - job_name: 'node-metrics'
    scrape_interval: 30s
    
    static_configs:
      - targets:
        - 'shard-001.prod.internal:9100'
        - 'shard-002.prod.internal:9100'
        - 'shard-003.prod.internal:9100'
        - 'shard-004.prod.internal:9100'
        - 'shard-005.prod.internal:9100'
        - 'shard-006.prod.internal:9100'
        - 'shard-007.prod.internal:9100'
        - 'shard-008.prod.internal:9100'
```

### 1.2 Shard Metrics (Sharding-spezifisch)

```
# URN Routing & Sharding
themis_shard_requests_total{shard_id, method}
  - Counter für Request-Zählung pro Shard
  
themis_shard_request_latency_ms{shard_id, percentile}
  - Histogram mit p50, p95, p99 Latenz pro Shard
  
themis_shard_hash_ring_balance_factor{cluster}
  - Gauge: Balancierungs-Faktor (sollte < 5% sein)
  
themis_shard_hot_key_count{shard_id}
  - Gauge: Anzahl Hot Keys pro Shard

# Replication Metrics
themis_replication_lag_ms{shard_id, replica_id}
  - Gauge: Replication Latency (sollte < 100ms sein)
  
themis_replication_failures_total{shard_id, reason}
  - Counter: Replication-Fehler pro Shard/Grund
  
themis_replication_throughput_bytes_sec{shard_id}
  - Gauge: Replication Throughput pro Shard

# Redundanzmode-spezifisch
themis_stripe_chunk_loss{shard_id}
  - Counter für verlorene Chunks (STRIPE Mode)
  
themis_parity_reconstruction{shard_id}
  - Counter für Parity-Rekonstruktionen (PARITY Mode)
  
themis_mirror_sync_lag{primary, replica}
  - Gauge: Sync-Lag zwischen Primary und Replicas

# Raft Consensus Metrics
themis_raft_heartbeat_latency_ms{shard_id, peer}
  - Histogram: Raft Heartbeat Latency
  
themis_raft_leader_changes_total{cluster}
  - Counter: Leader-Wechsel (sollte < 1/Stunde sein)
  
themis_raft_election_duration_ms{shard_id}
  - Histogram: Election Dauer (sollte < 300ms sein)
```

### 1.3 RocksDB Storage Metrics

```
themis_rocksdb_read_latency_us{shard_id, percentile}
  - Histogram: RocksDB Read Latency (microseconds)
  
themis_rocksdb_write_latency_us{shard_id}
  - Histogram: RocksDB Write Latency
  
themis_rocksdb_compaction_time_s{shard_id}
  - Counter: Compaction Duration
  
themis_rocksdb_block_cache_hit_ratio{shard_id}
  - Gauge: L1 Cache Hit Ratio (Ziel: > 90%)
  
themis_rocksdb_wal_sync_latency_ms{shard_id}
  - Histogram: WAL Sync Latency (für Durability)
  
themis_rocksdb_sst_files{shard_id, level}
  - Gauge: Anzahl SST Files pro Level
  
themis_rocksdb_memtable_size_mb{shard_id}
  - Gauge: Memtable Größe
```

### 1.4 Node-level Metrics (via node_exporter)

```
# CPU & Memory
node_cpu_seconds_total{cpu}
node_memory_MemAvailable_bytes
node_memory_MemFree_bytes

# Disk
node_filesystem_avail_bytes{mountpoint}
node_filesystem_size_bytes{mountpoint}
node_disk_reads_completed_total
node_disk_writes_completed_total
node_disk_io_time_ms_total

# Network
node_network_receive_bytes_total{device}
node_network_transmit_bytes_total{device}
node_network_receive_errs_total{device}
```

---

## 2. Grafana Dashboards

### 2.1 Dashboard 1: Shard Overview

```json
{
  "dashboard": {
    "title": "RAID-Themis Shard Overview",
    "tags": ["sharding", "raid-themis"],
    "timezone": "UTC",
    "panels": [
      {
        "title": "Cluster Health",
        "type": "stat",
        "targets": [
          {
            "expr": "count(up{job='themis-shards'})"
          }
        ],
        "thresholds": {
          "mode": "absolute",
          "steps": [
            { "color": "red", "value": null, "comparison": "lt", "color_value": 8 },
            { "color": "yellow", "value": 7, "comparison": "lte" },
            { "color": "green", "value": 8, "comparison": "gte" }
          ]
        }
      },
      {
        "title": "Throughput (ops/sec)",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(themis_shard_requests_total[1m])) by (job)",
            "legend": "{{ job }}"
          }
        ],
        "yaxes": [
          { "label": "ops/sec", "min": 0 }
        ]
      },
      {
        "title": "Latency p99 (ms)",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.99, rate(themis_shard_request_latency_ms_bucket[5m]))",
            "legend": "{{ shard_id }}"
          }
        ]
      },
      {
        "title": "Replication Lag (ms)",
        "type": "graph",
        "targets": [
          {
            "expr": "themis_replication_lag_ms",
            "legend": "{{ shard_id }} → {{ replica_id }}"
          }
        ],
        "alert": {
          "conditions": [
            {
              "evaluator": { "params": [500], "type": "gt" },
              "operator": { "type": "and" }
            }
          ]
        }
      },
      {
        "title": "Hash Ring Balance",
        "type": "gauge",
        "targets": [
          {
            "expr": "themis_shard_hash_ring_balance_factor"
          }
        ],
        "thresholds": "0,5,10"
      }
    ]
  }
}
```

### 2.2 Dashboard 2: Replication & Redundancy

```json
{
  "dashboard": {
    "title": "Replication & Redundancy Status",
    "tags": ["replication", "raid-themis"],
    "panels": [
      {
        "title": "Replication Mode",
        "type": "stat",
        "targets": [
          {
            "expr": "label_values(themis_replication_lag_ms, replication_mode)[0]"
          }
        ]
      },
      {
        "title": "Primary-Replica Sync",
        "type": "graph",
        "targets": [
          {
            "expr": "themis_mirror_sync_lag{replication_mode='MIRROR'}",
            "legend": "{{ primary }} → {{ replica }}"
          }
        ]
      },
      {
        "title": "Stripe Chunk Health (STRIPE Mode)",
        "type": "table",
        "targets": [
          {
            "expr": "topk(20, themis_stripe_chunk_loss)",
            "format": "table"
          }
        ]
      },
      {
        "title": "Parity Reconstruction Ops (PARITY Mode)",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(themis_parity_reconstruction_total[5m])",
            "legend": "{{ shard_id }}"
          }
        ]
      },
      {
        "title": "Replication Errors",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(themis_replication_failures_total[5m])",
            "legend": "{{ shard_id }} - {{ reason }}"
          }
        ]
      }
    ]
  }
}
```

### 2.3 Dashboard 3: Raft Consensus & Leadership

```json
{
  "dashboard": {
    "title": "Raft Consensus Status",
    "tags": ["consensus", "raft"],
    "panels": [
      {
        "title": "Leader Status",
        "type": "stat",
        "targets": [
          {
            "expr": "count(themis_raft_is_leader{is_leader='1'})"
          }
        ]
      },
      {
        "title": "Heartbeat Latency",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(themis_raft_heartbeat_latency_ms_bucket[5m]))",
            "legend": "{{ shard_id }} → {{ peer }}"
          }
        ]
      },
      {
        "title": "Leader Elections",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(themis_raft_leader_changes_total[1h])",
            "legend": "Leader changes per hour"
          }
        ],
        "alert": {
          "conditions": [
            {
              "evaluator": { "params": [1], "type": "gt" },
              "message": "More than 1 leader election per hour"
            }
          ]
        }
      },
      {
        "title": "Election Duration",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.99, rate(themis_raft_election_duration_ms_bucket[5m]))",
            "legend": "{{ shard_id }}"
          }
        ]
      }
    ]
  }
}
```

### 2.4 Dashboard 4: Storage & Performance

```json
{
  "dashboard": {
    "title": "RocksDB Storage & Performance",
    "tags": ["storage", "rocksdb"],
    "panels": [
      {
        "title": "RocksDB Write Latency (p99)",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.99, rate(themis_rocksdb_write_latency_us_bucket[5m])) / 1000",
            "legend": "{{ shard_id }}"
          }
        ]
      },
      {
        "title": "Block Cache Hit Ratio",
        "type": "graph",
        "targets": [
          {
            "expr": "themis_rocksdb_block_cache_hit_ratio",
            "legend": "{{ shard_id }}"
          }
        ],
        "threshold": "0.9"
      },
      {
        "title": "Compaction Time",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(themis_rocksdb_compaction_time_s[5m])",
            "legend": "{{ shard_id }}"
          }
        ]
      },
      {
        "title": "Disk Space Usage",
        "type": "graph",
        "targets": [
          {
            "expr": "node_filesystem_avail_bytes{mountpoint=~'/data.*'} / 1024 / 1024 / 1024",
            "legend": "{{ device }} free GB"
          }
        ]
      },
      {
        "title": "WAL Sync Latency",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(themis_rocksdb_wal_sync_latency_ms_bucket[5m]))",
            "legend": "{{ shard_id }}"
          }
        ]
      }
    ]
  }
}
```

---

## 3. AlertManager Rules

### 3.1 Alert Rules YAML

```yaml
# /etc/prometheus/rules/raid-themis-alerts.yml

groups:
  # ========================
  # Group 1: Throughput Alerts
  # ========================
  - name: "themis_throughput_alerts"
    interval: 30s
    rules:
      - alert: "ClusterThroughputDegraded"
        expr: |
          (rate(themis_shard_requests_total[5m]) / 6400000) < 0.8
        for: 5m
        labels:
          severity: "warning"
          component: "sharding"
        annotations:
          summary: "Cluster throughput degraded below 80%"
          description: "Current throughput: {{ $value }}"
          runbook_url: "https://themis.io/runbooks/throughput-degraded"
      
      - alert: "ClusterThroughputCritical"
        expr: |
          (rate(themis_shard_requests_total[5m]) / 6400000) < 0.5
        for: 2m
        labels:
          severity: "critical"
        annotations:
          summary: "Cluster throughput critical (< 50%)"
          description: "Immediate action required"

  # ========================
  # Group 2: Latency Alerts
  # ========================
  - name: "themis_latency_alerts"
    interval: 30s
    rules:
      - alert: "HighLatencyP99"
        expr: |
          histogram_quantile(0.99, rate(themis_shard_request_latency_ms_bucket[5m])) > 10
        for: 5m
        labels:
          severity: "warning"
        annotations:
          summary: "p99 latency above 10ms"
          description: "Shard {{ $labels.shard_id }}: {{ $value }}ms"

      - alert: "CriticalLatency"
        expr: |
          histogram_quantile(0.99, rate(themis_shard_request_latency_ms_bucket[5m])) > 50
        for: 2m
        labels:
          severity: "critical"
        annotations:
          summary: "p99 latency above 50ms (CRITICAL)"

  # ========================
  # Group 3: Replication Alerts
  # ========================
  - name: "themis_replication_alerts"
    interval: 30s
    rules:
      - alert: "ReplicationLagHigh"
        expr: |
          themis_replication_lag_ms > 500
        for: 2m
        labels:
          severity: "warning"
        annotations:
          summary: "Replication lag high (> 500ms)"
          description: "{{ $labels.shard_id }} → {{ $labels.replica_id }}: {{ $value }}ms"

      - alert: "ReplicationLagCritical"
        expr: |
          themis_replication_lag_ms > 2000
        for: 1m
        labels:
          severity: "critical"
        annotations:
          summary: "Replication lag critical (> 2s)"
          description: "Risk of data loss, investigate immediately"

      - alert: "ReplicationErrors"
        expr: |
          rate(themis_replication_failures_total[5m]) > 0.1
        for: 5m
        labels:
          severity: "warning"
        annotations:
          summary: "Replication errors detected"
          description: "{{ $labels.shard_id }}: {{ $labels.reason }}"

  # ========================
  # Group 4: Replica Health
  # ========================
  - name: "themis_replica_health_alerts"
    interval: 30s
    rules:
      - alert: "ReplicaUnhealthy"
        expr: |
          up{job="themis-shards"} == 0
        for: 2m
        labels:
          severity: "warning"
        annotations:
          summary: "Shard replica unhealthy"
          description: "{{ $labels.instance }} is not responding"

      - alert: "MissingReplicas"
        expr: |
          (count(up{job="themis-shards"}) < 8)
        for: 2m
        labels:
          severity: "critical"
        annotations:
          summary: "Cluster missing replicas (< 8 shards)"
          description: "Current shards: {{ $value }}"

  # ========================
  # Group 5: Resource Alerts
  # ========================
  - name: "themis_resource_alerts"
    interval: 60s
    rules:
      - alert: "DiskSpaceRunningOut"
        expr: |
          (node_filesystem_avail_bytes{mountpoint=~'/data.*'} / node_filesystem_size_bytes{mountpoint=~'/data.*'}) < 0.2
        for: 5m
        labels:
          severity: "warning"
        annotations:
          summary: "Disk space < 20%"
          description: "{{ $labels.device }}: {{ $value | humanizePercentage }}"

      - alert: "HighMemoryUsage"
        expr: |
          (1 - (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)) > 0.85
        for: 5m
        labels:
          severity: "warning"
        annotations:
          summary: "High memory usage (> 85%)"
          description: "Instance: {{ $labels.instance }}"

      - alert: "HighCPUUsage"
        expr: |
          (100 - (avg by (instance) (irate(node_cpu_seconds_total{mode="idle"}[5m])) * 100)) > 80
        for: 5m
        labels:
          severity: "warning"
        annotations:
          summary: "High CPU usage (> 80%)"
```

### 3.2 AlertManager Configuration

```yaml
# /etc/alertmanager/config.yml

global:
  resolve_timeout: 5m
  slack_api_url: 'https://hooks.slack.com/services/YOUR/WEBHOOK'
  pagerduty_url: 'https://events.pagerduty.com/v2/enqueue'

route:
  receiver: 'default'
  group_by: ['alertname', 'severity', 'shard_id']
  group_wait: 10s
  group_interval: 10s
  repeat_interval: 4h

  routes:
    # Critical alerts → PagerDuty + Slack
    - match:
        severity: critical
      receiver: 'pagerduty-critical'
      continue: true

    # Warning alerts → Slack
    - match:
        severity: warning
      receiver: 'slack-warnings'

receivers:
  - name: 'default'
    slack_configs:
      - channel: '#themis-alerts'
        title: 'RAID-Themis Alert'
        text: '{{ range .Alerts }}{{ .Annotations.summary }}\n{{ end }}'

  - name: 'pagerduty-critical'
    pagerduty_configs:
      - service_key: 'YOUR_PAGERDUTY_KEY'
        description: '{{ .GroupLabels.alertname }}'
    slack_configs:
      - channel: '#themis-critical'
        title: '🚨 CRITICAL: {{ .GroupLabels.alertname }}'
        text: 'Severity: {{ .GroupLabels.severity }}\n{{ range .Alerts }}{{ .Annotations.description }}\n{{ end }}'

  - name: 'slack-warnings'
    slack_configs:
      - channel: '#themis-warnings'
        title: '⚠️ WARNING: {{ .GroupLabels.alertname }}'
```

---

## 4. ELK Stack Configuration

### 4.1 Logstash Pipeline für Themis Logs

```
# /etc/logstash/conf.d/themis-raid.conf

input {
  file {
    path => "/var/log/themis/shard-*.log"
    start_position => "beginning"
    sincedb_path => "/var/lib/logstash/.sincedb_themis"
    tags => ["themis", "shard"]
  }
}

filter {
  if "themis" in [tags] {
    multiline {
      pattern => "^%{TIMESTAMP_ISO8601}"
      negate => true
      what => "previous"
    }
    
    grok {
      match => { "message" => "%{TIMESTAMP_ISO8601:timestamp} \[%{DATA:shard_id}\] %{LOGLEVEL:level} %{GREEDYDATA:message}" }
    }
    
    # Error Pattern Matching
    if [message] =~ /replication.*failed/ {
      mutate { add_tag => ["replication_error"] }
    }
    
    if [message] =~ /latency.*high/ {
      mutate { add_tag => ["latency_warning"] }
    }
    
    if [message] =~ /disk.*full/ {
      mutate { add_tag => ["disk_critical"] }
    }
    
    date {
      match => [ "timestamp", "ISO8601" ]
      target => "@timestamp"
    }
  }
}

output {
  elasticsearch {
    hosts => ["elasticsearch.prod.internal:9200"]
    index => "themis-logs-%{+YYYY.MM.dd}"
  }
  
  # Auch Errors zu Slack pipen
  if [level] == "ERROR" or "critical" in [tags] {
    slack {
      url => "https://hooks.slack.com/services/YOUR/WEBHOOK"
      message => "[%{shard_id}] %{level}: %{message}"
      channel => "#themis-errors"
    }
  }
}
```

### 4.2 Kibana Dashboard für Themis Logs

```json
{
  "dashboard": {
    "title": "RAID-Themis Log Analysis",
    "panels": [
      {
        "id": "errors_over_time",
        "type": "histogram",
        "query": "level:ERROR",
        "timeField": "@timestamp"
      },
      {
        "id": "replication_errors",
        "type": "table",
        "query": "tags:replication_error",
        "columns": ["shard_id", "message", "@timestamp"]
      },
      {
        "id": "latency_issues",
        "type": "table",
        "query": "tags:latency_warning",
        "columns": ["shard_id", "message", "@timestamp"]
      }
    ]
  }
}
```

---

## 5. Alert Response Playbooks

### 5.1 Playbook: Throughput Degradation

```yaml
# Auslöser: ClusterThroughputDegraded (> 5 min)

runbook_throughput_degraded:
  name: "Cluster Throughput Degradation"
  severity: "warning"
  
  check:
    1: |
      # Check Cluster Health
      themis-cli cluster health
      
      - Sind alle 8 Shards gesund?
      - Gibt es offline Replicas?
      
  diagnosis:
    2a: |
      # Falls Shard unhealthy
      themis-cli shard health --shard-id shard_XXX
      
      Check:
      - Netzwerk-Konnektivität (ping, telnet Port 8080)
      - Disk Space (df -h /data/themis/rocksdb/shard_XXX)
      - CPU/Memory (top, free -h)
      - Log Errors (journalctl -u themis-shard@shard_XXX -f)
    
    2b: |
      # Falls alle Shards gesund
      # Check Replication Lag
      themis-cli metrics get replication_lag
      
      - Replication Lag > 500ms?
      - Falls ja: → Playbook "Replication Lag"
    
    2c: |
      # Falls Replication OK, check Client-side
      # Hot Keys / Unbalanced Load?
      themis-cli metrics get hash_ring_balance
      
      - Balance Factor > 10%?
      - Falls ja: Rebalance triggern
  
  mitigation:
    3a: |
      # Wenn Shard offline
      systemctl restart themis-shard@shard_XXX
      themis-cli shard health --shard-id shard_XXX --wait 60
    
    3b: |
      # Wenn Rebalance nötig
      themis-cli cluster rebalance \
        --method consistent-hash \
        --data-migration-rate 100MB/s
    
    3c: |
      # Wenn nur Replica-Lag
      # Replica-Kapazität erhöhen
      themis-cli shard scale-replica shard_XXX \
        --memory-increase 10GB
  
  monitoring:
    4: |
      # Throughput 10 Minuten beobachten
      watch -n 5 'themis-cli metrics get throughput'
      
      - Sollte wieder auf 6.4M ops/sec gehen
      - Falls nicht: Escalate zu P1 Engineer
```

### 5.2 Playbook: High Latency (p99 > 10ms)

```yaml
runbook_high_latency:
  name: "High Query Latency (p99 > 10ms)"
  severity: "warning"
  
  check:
    1: |
      # Welcher Shard?
      promtool query instant \
        'histogram_quantile(0.99, rate(themis_shard_request_latency_ms_bucket[5m]))'
      
      - Latency spikes bei bestimmtem Shard?
  
  diagnosis:
    2: |
      # RocksDB Metrics
      themis-cli metrics get rocksdb_write_latency
      themis-cli metrics get rocksdb_block_cache_hit_ratio
      
      - Cache Hit Ratio < 80%?
      - Write Latency > 2ms?
  
  mitigation:
    3a: |
      # Wenn Cache Hit Ratio niedrig
      # Memtable vergrößern
      themis-cli shard config --shard-id shard_XXX \
        --write-buffer-size 512MB
    
    3b: |
      # Wenn Compaction läuft
      # Priorität reduzieren
      themis-cli shard config --shard-id shard_XXX \
        --compression-level low
    
    3c: |
      # Wenn Replication Lag
      # → Playbook "Replication Lag"
  
  monitoring:
    4: |
      # Latenz 5 Minuten beobachten
      watch -n 2 'themis-cli metrics get p99_latency'
      
      - Sollte wieder auf 1-2ms fallen
```

### 5.3 Playbook: Replica Unhealthy

```yaml
runbook_replica_unhealthy:
  name: "Replica Shard Down"
  severity: "critical"
  
  check:
    1: |
      # Welcher Shard?
      themis-cli cluster topology
      
      - Welcher Shard ist down?
      - Wie lange schon?
  
  diagnosis:
    2: |
      # Node Health Check
      ssh shard-XXX.prod.internal
      systemctl status themis-shard@shard_XXX
      journalctl -n 50 -u themis-shard@shard_XXX
      
      - Is Process running? (ps aux | grep themis)
      - Disk voll? (df -h)
      - Network down? (ping 8.8.8.8)
  
  mitigation:
    3: |
      # Sofort: Restart
      systemctl restart themis-shard@shard_XXX
      
      # Auf Recovery warten
      themis-cli shard health --shard-id shard_XXX --wait 120
      
      # Falls immer noch down: Failover
      themis-cli shard promote-replica \
        --primary shard_XXX \
        --new-replica shard_XXX_backup
  
  escalation:
    4: |
      # Falls nicht gelöst
      - Escalate zu Infrastruktur Team
      - Disk Replacement?
      - Server Replacement?
```

---

## 6. SLA & KPI Targets

### 6.1 Production SLA

| Metrik | Target | Current | Status |
|--------|--------|---------|--------|
| **Availability** | 99.95% | 99.97% | ✅ |
| **p99 Latency** | < 10ms | 2.1ms | ✅ |
| **Throughput** | 6.4M ops/sec | 6.5M ops/sec | ✅ |
| **Replication Lag** | < 100ms | 45ms | ✅ |
| **RTO** | < 1 min | < 30sec | ✅ |
| **RPO** | 0 Data Loss | 0 | ✅ |

### 6.2 Alert Thresholds

```yaml
thresholds:
  # Throughput
  throughput_degraded: 80%         # Warning: < 5.12M ops/sec
  throughput_critical: 50%         # Critical: < 3.2M ops/sec
  
  # Latency
  latency_p99_warn: 10ms
  latency_p99_crit: 50ms
  
  # Replication
  replication_lag_warn: 500ms
  replication_lag_crit: 2000ms
  
  # Resources
  disk_warn: 20% free
  disk_crit: 10% free
  memory_warn: 85% used
  cpu_warn: 80% used
  
  # Raft
  leader_elections_per_hour: 1    # Max acceptable
  election_duration: 300ms        # Max
```

---

## 7. Observability Checklist

### Pre-Production

- [ ] Prometheus Server deployed & running
- [ ] All 8 Shard targets scraped successfully
- [ ] Grafana connected to Prometheus
- [ ] All 4 Dashboards imported
- [ ] AlertManager connected & tested
- [ ] Slack webhooks configured
- [ ] ELK Stack deployed (optional)
- [ ] Logstash pipelines tested
- [ ] Alert routing tested

### Post-Deployment

- [ ] Baseline metrics captured (Throughput, Latency, etc.)
- [ ] All alerts firing correctly on test
- [ ] Alert escalation paths tested
- [ ] Runbooks accessible and up-to-date
- [ ] Team trained on dashboards
- [ ] 24/7 on-call rotation active

---

## Summary

Diese Observability-Suite bietet **vollständige Sichtbarkeit** in RAID-Themis:

✅ **Prometheus** - Real-time Metrics  
✅ **Grafana** - Beautiful Dashboards  
✅ **AlertManager** - Intelligent Alerting  
✅ **ELK** - Log Aggregation & Analysis  
✅ **Runbooks** - Response Automation  

**Next Steps:**
1. Deploy Prometheus + Grafana
2. Import 4 Dashboards
3. Configure AlertManager + Slack
4. Train team on alert response
5. Start 24/7 monitoring

---
