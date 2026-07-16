---
category: "⚙️ Operations/Admin"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
audience: "DevOps engineers, SREs, database administrators"
---

# ⚙️ ThemisDB Administrator Guide

Operational guide for running ThemisDB in production.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features & Highlights](#-features--highlights)
- [🚀 Installation & Deployment](#-installation--deployment)
- [📖 Konfiguration & Betrieb](#-konfiguration--betrieb)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Weitere Ressourcen](#-weitere-ressourcen)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

Operational guide for DevOps engineers and database administrators covering deployment, monitoring, security, and maintenance.

**Target Audience:** DevOps engineers, SREs, database administrators

**Version:** 1.3.0  
**Last Updated:** April 2026

---

## ✨ Features & Highlights

- 🏗️ **4 Build-Varianten** - Standard, OLAP, Embedded, vLLM
- 💾 **Inkrementelle Backups** - 80-90% Speicherersparnis
- 📊 **OpenTelemetry** - Vollständiges Monitoring
- 🔐 **PKI/mTLS** - Enterprise Security
- 🤖 **vLLM Co-Location** - AI/ML Workloads
- ☸️ **Kubernetes Ready** - StatefulSet deployment

---

## 🚀 Installation & Deployment

### Build Variants

ThemisDB supports 4 build configurations:

```bash
# 1. Standard (OLTP) - Default, optimized for transactional workloads
cmake -DBUILD_VARIANT=standard ..
make -j$(nproc)

# 2. OLAP - Includes DuckDB for analytical queries
cmake -DBUILD_VARIANT=olap ..
make -j$(nproc)

# 3. Embedded - Minimal dependencies for IoT/edge
cmake -DBUILD_VARIANT=embedded ..
make -j$(nproc)

# 4. vLLM - CUDA support for AI/ML workloads
cmake -DBUILD_VARIANT=vllm -DUSE_CUDA=ON ..
make -j$(nproc)
```

**Variant Comparison:**

| Feature | Standard | OLAP | Embedded | vLLM |
|---------|----------|------|----------|------|
| Size | 50 MB | 150 MB | 20 MB | 80 MB |
| TBB | ✅ | ✅ | ❌ | ✅ |
| Arrow | ✅ | ✅ | ❌ | ✅ |
| DuckDB | ❌ | ✅ | ❌ | ❌ |
| CUDA | ❌ | ❌ | ❌ | ✅ |
| Use Case | General | Analytics | IoT/Edge | AI/ML |

### Docker Deployment

**Standard Deployment:**

```yaml
# docker-compose.yml
version: '3.8'

services:
  themisdb:
    image: themisdb/themisdb:1.2.0
    ports:
      - "8529:8529"  # HTTP API
      - "8530:8530"  # Binary protocol
    volumes:
      - ./data:/var/lib/themisdb
      - ./config:/etc/themisdb
      - ./backups:/backups
    environment:
      - THEMIS_DB_PATH=/var/lib/themisdb
      - THEMIS_LOG_LEVEL=info
      - THEMIS_MAX_CONNECTIONS=1000
    deploy:
      resources:
        limits:
          cpus: '16'
          memory: 32G
        reservations:
          cpus: '8'
          memory: 16G
```

**vLLM Co-Location:**

```yaml
version: '3.8'

services:
  themisdb:
    image: themisdb/themisdb:1.2.0-vllm
    ports:
      - "8529:8529"
    volumes:
      - ./data:/var/lib/themisdb
    environment:
      - THEMIS_VLLM_COLOCATION=true
      - THEMIS_GPU_MEMORY_FRACTION=0.3
    deploy:
      resources:
        limits:
          cpus: '50'
          memory: 200G
        reservations:
          cpus: '32'
          memory: 128G
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu]

  vllm:
    image: vllm/vllm-openai:latest
    ports:
      - "8000:8000"
    environment:
      - VLLM_GPU_MEMORY_UTILIZATION=0.7
    deploy:
      resources:
        limits:
          cpus: '14'
          memory: 56G
        devices:
          - driver: nvidia
            count: 1
            capabilities: [gpu]
```

### Kubernetes Deployment

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: themisdb
spec:
  serviceName: themisdb
  replicas: 3
  selector:
    matchLabels:
      app: themisdb
  template:
    metadata:
      labels:
        app: themisdb
    spec:
      containers:
      - name: themisdb
        image: themisdb/themisdb:1.2.0
        ports:
        - containerPort: 8529
          name: http
        - containerPort: 8530
          name: binary
        resources:
          requests:
            memory: "16Gi"
            cpu: "8"
          limits:
            memory: "32Gi"
            cpu: "16"
        volumeMounts:
        - name: data
          mountPath: /var/lib/themisdb
        - name: config
          mountPath: /etc/themisdb
        env:
        - name: THEMIS_CLUSTER_ENABLED
          value: "true"
        - name: THEMIS_RAFT_PEERS
          value: "themisdb-0.themisdb:8530,themisdb-1.themisdb:8530,themisdb-2.themisdb:8530"
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: [ "ReadWriteOnce" ]
      resources:
        requests:
          storage: 1Ti
```

## Configuration Management

### Primary Configuration File

```yaml
# /etc/themisdb/themisdb.yaml
server:
  bind_address: "0.0.0.0"
  http_port: 8529
  binary_port: 8530
  max_connections: 1000

database:
  path: "/var/lib/themisdb"
  cache_size_mb: 8192  # 8 GB
  write_buffer_size_mb: 256
  max_background_jobs: 16

rocksdb:
  block_cache_size: 8589934592  # 8 GB
  write_buffer_size: 268435456  # 256 MB
  max_write_buffer_number: 4
  level0_file_num_compaction_trigger: 8

security:
  tls_enabled: true
  tls_cert_file: "/etc/themisdb/certs/server.crt"
  tls_key_file: "/etc/themisdb/certs/server.key"
  tls_ca_file: "/etc/themisdb/certs/ca.crt"
  authentication_required: true

monitoring:
  opentelemetry_enabled: true
  opentelemetry_endpoint: "http://otel-collector:4317"
  metrics_export_interval_seconds: 60

backup:
  enabled: true
  incremental: true
  schedule: "0 2 * * *"  # 2 AM daily
  retention_days: 30
  destination: "/backups"
```

### Environment Variables

```bash
# Core settings
export THEMIS_DB_PATH="/var/lib/themisdb"
export THEMIS_LOG_LEVEL="info"  # debug, info, warn, error
export THEMIS_CONFIG="/etc/themisdb/themisdb.yaml"

# Performance tuning
export THEMIS_CACHE_SIZE_MB="8192"
export THEMIS_MAX_CONNECTIONS="1000"
export THEMIS_WORKER_THREADS="16"

# Security
export THEMIS_TLS_ENABLED="true"
export THEMIS_AUTH_REQUIRED="true"

# vLLM co-location
export THEMIS_VLLM_COLOCATION="true"
export THEMIS_GPU_MEMORY_FRACTION="0.3"
```

## Backup & Recovery

### Incremental Backups

```bash
# Create incremental backup (80-90% storage reduction)
themisdb-admin backup create \
  --type incremental \
  --destination /backups/themis \
  --share-table-files

# List backups
themisdb-admin backup list /backups/themis

# Backup retention
themisdb-admin backup prune \
  --destination /backups/themis \
  --keep-last 30
```

**Storage Savings:**
- Full backup: 1 TB
- Incremental backup #1: 100 GB (90% reduction)
- Incremental backup #2: 50 GB (95% reduction)

### Point-in-Time Recovery

```bash
# Restore to specific backup
themisdb-admin restore \
  --source /backups/themis/backup-20231220-120000 \
  --destination /var/lib/themisdb-restored

# Restore to specific timestamp
themisdb-admin restore \
  --source /backups/themis \
  --timestamp "2023-12-20 12:00:00" \
  --destination /var/lib/themisdb-restored
```

### Automated Backup Scripts

```bash
#!/bin/bash
# /usr/local/bin/themis-backup.sh

BACKUP_DIR="/backups/themis"
RETENTION_DAYS=30
DATE=$(date +%Y%m%d-%H%M%S)

# Create incremental backup
themisdb-admin backup create \
  --type incremental \
  --destination "$BACKUP_DIR" \
  --share-table-files \
  --tag "$DATE" || exit 1

# Prune old backups
themisdb-admin backup prune \
  --destination "$BACKUP_DIR" \
  --keep-last "$RETENTION_DAYS"

# Upload to S3 (optional)
aws s3 sync "$BACKUP_DIR" s3://themisdb-backups/

echo "Backup completed: $DATE"
```

**Cron Schedule:**
```cron
# Daily backup at 2 AM
0 2 * * * /usr/local/bin/themis-backup.sh >> /var/log/themis-backup.log 2>&1
```

## Monitoring & Alerting

### OpenTelemetry Integration

```yaml
# otel-collector-config.yaml
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: "0.0.0.0:4317"

processors:
  batch:
    timeout: 10s

exporters:
  prometheus:
    endpoint: "0.0.0.0:8889"
  jaeger:
    endpoint: "jaeger:14250"

service:
  pipelines:
    metrics:
      receivers: [otlp]
      processors: [batch]
      exporters: [prometheus]
    traces:
      receivers: [otlp]
      processors: [batch]
      exporters: [jaeger]
```

### Prometheus Metrics

Key metrics to monitor:

```yaml
# prometheus.yml
scrape_configs:
  - job_name: 'themisdb'
    scrape_interval: 15s
    static_configs:
      - targets: ['themisdb:8529']
    metrics_path: '/metrics'
```

**Critical Metrics:**
- `themis_rocksdb_bytes_written` - Write throughput
- `themis_rocksdb_bytes_read` - Read throughput
- `themis_block_cache_hit_rate` - Cache efficiency
- `themis_query_latency_p99` - Query performance
- `themis_connection_count` - Active connections
- `themis_disk_usage_bytes` - Storage utilization

### Grafana Dashboards

```json
{
  "dashboard": {
    "title": "ThemisDB Overview",
    "panels": [
      {
        "title": "Query Latency (P99)",
        "targets": [{
          "expr": "histogram_quantile(0.99, themis_query_latency_seconds_bucket)"
        }]
      },
      {
        "title": "Cache Hit Rate",
        "targets": [{
          "expr": "rate(themis_block_cache_hits[5m]) / (rate(themis_block_cache_hits[5m]) + rate(themis_block_cache_misses[5m]))"
        }]
      },
      {
        "title": "Throughput (QPS)",
        "targets": [{
          "expr": "rate(themis_queries_total[1m])"
        }]
      }
    ]
  }
}
```

### Alerting Rules

```yaml
# prometheus-alerts.yml
groups:
  - name: themisdb
    rules:
      - alert: HighQueryLatency
        expr: histogram_quantile(0.99, themis_query_latency_seconds_bucket) > 1
        for: 5m
        annotations:
          summary: "High query latency detected"
          description: "P99 latency is {{ $value }}s"

      - alert: LowCacheHitRate
        expr: |
          rate(themis_block_cache_hits[5m]) / 
          (rate(themis_block_cache_hits[5m]) + rate(themis_block_cache_misses[5m])) < 0.8
        for: 10m
        annotations:
          summary: "Low cache hit rate"
          description: "Cache hit rate is {{ $value | humanizePercentage }}"

      - alert: DiskSpaceRunningOut
        expr: themis_disk_free_bytes / themis_disk_total_bytes < 0.1
        for: 5m
        annotations:
          summary: "Disk space running low"
          description: "Only {{ $value | humanizePercentage }} disk space remaining"
```

## Security Hardening

### TLS/mTLS Configuration

```bash
# Generate CA certificate
openssl genrsa -out ca.key 4096
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt

# Generate server certificate
openssl genrsa -out server.key 4096
openssl req -new -key server.key -out server.csr
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 365

# Install certificates
sudo cp ca.crt /etc/themisdb/certs/
sudo cp server.crt /etc/themisdb/certs/
sudo cp server.key /etc/themisdb/certs/
sudo chmod 600 /etc/themisdb/certs/server.key
```

### User Authentication & Authorization

```bash
# Create admin user
themisdb-admin user create \
  --username admin \
  --password <strong-password> \
  --role admin

# Create read-only user
themisdb-admin user create \
  --username readonly \
  --password <password> \
  --role reader

# Create application user with specific permissions
themisdb-admin user create \
  --username app_user \
  --password <password> \
  --role custom \
  --permissions "read:documents,write:documents,read:users"
```

### Network Security

```yaml
# firewall rules (ufw)
sudo ufw allow 22/tcp          # SSH
sudo ufw allow 8529/tcp        # ThemisDB HTTP
sudo ufw allow 8530/tcp        # ThemisDB Binary
sudo ufw enable

# Restrict to specific IPs
sudo ufw allow from 10.0.1.0/24 to any port 8529
```

### Audit Logging

```yaml
# themisdb.yaml
audit:
  enabled: true
  log_file: "/var/log/themisdb/audit.log"
  events:
    - authentication
    - authorization
    - data_modification
    - schema_changes
    - admin_operations
  rotation:
    max_size_mb: 100
    max_files: 30
```

## Resource Management

### vLLM Co-Location

```yaml
# Resource allocation for ThemisDB + vLLM
vllm_colocation:
  enabled: true
  
  themisdb:
    cpu_cores: 50
    memory_gb: 200
    gpu_memory_fraction: 0.3
    
  vllm:
    cpu_cores: 14
    memory_gb: 56
    gpu_memory_fraction: 0.7
    
  gpu:
    device: 0
    priority: low  # ThemisDB uses low priority CUDA streams
```

### CPU Affinity

```bash
# Pin ThemisDB to specific CPU cores
taskset -c 0-49 themisdb-server

# Pin vLLM to different cores
taskset -c 50-63 vllm-server
```

### Memory Limits

```bash
# systemd service with memory limits
[Service]
MemoryLimit=200G
MemoryAccounting=true
```

## High Availability

*Note: Full HA with automatic failover is planned for Q2 2026 (v1.3.0)*

**Current Capabilities:**
- Manual failover with Raft consensus
- Read replicas for load distribution
- Incremental backups for fast recovery

**Planned (Q2 2026):**
- Automatic leader election
- Multi-region replication
- Geographic distribution

## Disaster Recovery

### RTO/RPO Targets

| Scenario | RTO | RPO | Strategy |
|----------|-----|-----|----------|
| Single node failure | 5 min | 0 | Hot standby + Raft |
| Data corruption | 1 hour | 1 hour | Incremental backup restore |
| Region failure | 4 hours | 1 hour | Cross-region backup |
| Complete data loss | 8 hours | 24 hours | S3 backup restore |

### DR Runbook

```bash
# 1. Detect failure
systemctl status themisdb

# 2. Check logs
tail -f /var/log/themisdb/themisdb.log

# 3. Attempt restart
systemctl restart themisdb

# 4. If restart fails, restore from backup
themisdb-admin restore \
  --source s3://themisdb-backups/latest \
  --destination /var/lib/themisdb

# 5. Start service
systemctl start themisdb

# 6. Verify
themisdb-admin health check
```

## Performance Tuning

See [Power User Guide](POWER_USER_GUIDE.md) for detailed performance tuning.

**Quick Checklist:**
- ✅ Tune RocksDB cache size (50% of RAM)
- ✅ Adjust write buffer size for workload
- ✅ Enable compression for storage savings
- ✅ Monitor and tune compaction settings
- ✅ Use appropriate build variant for use case

## Troubleshooting

### Common Issues

**High Memory Usage:**
```bash
# Check memory usage
ps aux | grep themisdb

# Reduce cache size
themisdb-admin config set database.cache_size_mb 4096

# Restart
systemctl restart themisdb
```

**Slow Queries:**
```bash
# Enable slow query log
themisdb-admin config set profiling.slow_query_threshold_ms 100

# Check slow queries
tail -f /var/log/themisdb/slow-queries.log

# Analyze query plan
themisdb-admin explain "SELECT * FROM documents WHERE category = 'ai'"
```

**Connection Errors:**
```bash
# Check connection limits
themisdb-admin config get server.max_connections

# Increase if needed
themisdb-admin config set server.max_connections 2000

# Check active connections
themisdb-admin stats connections
```

## Maintenance Tasks

### Database Compaction

```bash
# Manual compaction (reduce disk usage)
themisdb-admin compact --wait

# Schedule automatic compaction
echo "0 3 * * 0 themisdb-admin compact" | crontab -
```

### Index Rebuilding

```bash
# Rebuild specific index
themisdb-admin index rebuild documents category_idx

# Rebuild all indexes
themisdb-admin index rebuild --all
```

### Version Upgrades

```bash
# 1. Backup before upgrade
themisdb-admin backup create --destination /backups/pre-upgrade

# 2. Stop service
systemctl stop themisdb

# 3. Upgrade binary
apt-get update && apt-get install themisdb=1.2.0

# 4. Run migrations
themisdb-admin migrate

# 5. Start service
systemctl start themisdb

# 6. Verify
themisdb-admin version
themisdb-admin health check
```

## Summary

**Operational Checklist:**
- ✅ Deploy appropriate build variant
- ✅ Configure incremental backups (daily)
- ✅ Set up monitoring with OpenTelemetry
- ✅ Enable TLS/mTLS for security
- ✅ Configure resource limits
- ✅ Set up alerting for critical metrics
- ✅ Document DR procedures
- ✅ Schedule regular maintenance tasks

**Next Steps:**
- Read [System Architect Guide](SYSTEM_ARCHITECT_GUIDE.md) for sharding and distributed systems
- Review [Security Best Practices](../guides/guides_tls_setup.md)
- Set up [Monitoring Dashboards](../guides/guides_operations_runbook.md)
