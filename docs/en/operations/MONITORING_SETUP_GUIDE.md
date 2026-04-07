# ThemisDB Monitoring Setup Guide

**Version:** 1.4.0  
**Last Updated:** 2026-04-06  
**Target Audience:** Site Reliability Engineers, DevOps Engineers, Database Administrators

---

## Table of Contents

1. [Metrics Collection Setup](#metrics-collection-setup)
2. [Health Check Endpoints](#health-check-endpoints)
3. [Alert Configuration](#alert-configuration)
4. [Log Aggregation](#log-aggregation)
5. [Dashboarding](#dashboarding)
6. [Performance Monitoring](#performance-monitoring)
7. [Capacity Planning](#capacity-planning)

---

## Metrics Collection Setup

### Prometheus Configuration

**Prometheus Installation:**

```bash
#!/bin/bash
# install_prometheus.sh - Install Prometheus for ThemisDB monitoring

PROMETHEUS_VERSION="2.45.0"
PROMETHEUS_USER="prometheus"
PROMETHEUS_DIR="/opt/prometheus"

# Create prometheus user
useradd --no-create-home --shell /bin/false $PROMETHEUS_USER

# Download Prometheus
cd /tmp
wget https://github.com/prometheus/prometheus/releases/download/v${PROMETHEUS_VERSION}/prometheus-${PROMETHEUS_VERSION}.linux-amd64.tar.gz
tar xvfz prometheus-${PROMETHEUS_VERSION}.linux-amd64.tar.gz

# Install
mkdir -p $PROMETHEUS_DIR
cp prometheus-${PROMETHEUS_VERSION}.linux-amd64/prometheus $PROMETHEUS_DIR/
cp prometheus-${PROMETHEUS_VERSION}.linux-amd64/promtool $PROMETHEUS_DIR/
cp -r prometheus-${PROMETHEUS_VERSION}.linux-amd64/consoles $PROMETHEUS_DIR/
cp -r prometheus-${PROMETHEUS_VERSION}.linux-amd64/console_libraries $PROMETHEUS_DIR/

# Create directories
mkdir -p /etc/prometheus
mkdir -p /var/lib/prometheus

# Set permissions
chown -R $PROMETHEUS_USER:$PROMETHEUS_USER $PROMETHEUS_DIR
chown -R $PROMETHEUS_USER:$PROMETHEUS_USER /etc/prometheus
chown -R $PROMETHEUS_USER:$PROMETHEUS_USER /var/lib/prometheus

echo "✓ Prometheus installed"
```

**Prometheus Configuration for ThemisDB:**

```yaml
# /etc/prometheus/prometheus.yml
global:
  scrape_interval: 15s
  evaluation_interval: 15s
  external_labels:
    cluster: 'themisdb-production'
    environment: 'prod'

# Alertmanager configuration
alerting:
  alertmanagers:
    - static_configs:
        - targets:
            - 'localhost:9093'

# Rule files
rule_files:
  - '/etc/prometheus/rules/themisdb_alerts.yml'
  - '/etc/prometheus/rules/themisdb_recording.yml'

# Scrape configurations
scrape_configs:
  # ThemisDB server metrics
  - job_name: 'themisdb-server'
    static_configs:
      - targets:
          - 'themisdb-server-01:9091'
          - 'themisdb-server-02:9091'
          - 'themisdb-server-03:9091'
        labels:
          cluster: 'main'
          role: 'server'
    
    # Scrape timeout
    scrape_timeout: 10s
    
    # TLS configuration
    scheme: https
    tls_config:
      ca_file: /etc/prometheus/certs/ca.pem
      cert_file: /etc/prometheus/certs/prometheus.pem
      key_file: /etc/prometheus/certs/prometheus-key.pem
      insecure_skip_verify: false
    
    # Basic auth (if not using TLS client certs)
    # basic_auth:
    #   username: prometheus
    #   password_file: /etc/prometheus/secrets/themisdb_password

  # ThemisDB replication metrics
  - job_name: 'themisdb-replication'
    static_configs:
      - targets:
          - 'themisdb-server-01:9092'
          - 'themisdb-server-02:9092'
          - 'themisdb-server-03:9092'
        labels:
          cluster: 'main'
          role: 'replication'
    scrape_interval: 10s

  # Node exporter (system metrics)
  - job_name: 'node-exporter'
    static_configs:
      - targets:
          - 'themisdb-server-01:9100'
          - 'themisdb-server-02:9100'
          - 'themisdb-server-03:9100'
        labels:
          cluster: 'main'

  # Process exporter (detailed process metrics)
  - job_name: 'process-exporter'
    static_configs:
      - targets:
          - 'themisdb-server-01:9256'
          - 'themisdb-server-02:9256'
          - 'themisdb-server-03:9256'

  # Prometheus self-monitoring
  - job_name: 'prometheus'
    static_configs:
      - targets:
          - 'localhost:9090'

# Storage configuration
storage:
  tsdb:
    path: /var/lib/prometheus
    retention:
      time: 30d
      size: 100GB
    
    # Write-ahead log compression
    wal_compression: true

# Remote write (optional - for long-term storage)
remote_write:
  - url: "https://thanos.example.com/api/v1/receive"
    queue_config:
      capacity: 10000
      max_shards: 10
      max_samples_per_send: 1000
```

**Systemd Service for Prometheus:**

```ini
# /etc/systemd/system/prometheus.service
[Unit]
Description=Prometheus
Wants=network-online.target
After=network-online.target

[Service]
User=prometheus
Group=prometheus
Type=simple
ExecStart=/opt/prometheus/prometheus \
  --config.file=/etc/prometheus/prometheus.yml \
  --storage.tsdb.path=/var/lib/prometheus \
  --web.console.templates=/opt/prometheus/consoles \
  --web.console.libraries=/opt/prometheus/console_libraries \
  --web.listen-address=:9090 \
  --web.enable-lifecycle \
  --log.level=info

ExecReload=/bin/kill -HUP $MAINPID
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### ThemisDB Metrics Exposition

**ThemisDB Metrics Configuration:**

```yaml
# /opt/themisdb/config/metrics.yaml
metrics:
  enabled: true
  
  # Exposition endpoint
  http:
    enabled: true
    bind_address: "0.0.0.0"
    port: 9091
    path: "/metrics"
    
    # TLS for metrics endpoint
    tls:
      enabled: true
      cert_file: "/opt/themisdb/certs/metrics-cert.pem"
      key_file: "/opt/themisdb/keys/metrics-key.pem"
    
    # Authentication
    auth:
      enabled: true
      type: "basic"  # or "bearer"
      users:
        - username: "prometheus"
          password_hash: "$2b$12$..."
  
  # Metric categories
  categories:
    system:
      enabled: true
      metrics:
        - "cpu_usage"
        - "memory_usage"
        - "disk_usage"
        - "network_io"
        - "file_descriptors"
    
    database:
      enabled: true
      metrics:
        - "total_keys"
        - "total_size_bytes"
        - "databases_count"
        - "tables_count"
    
    transactions:
      enabled: true
      metrics:
        - "transaction_active"
        - "transaction_committed_total"
        - "transaction_aborted_total"
        - "transaction_duration_seconds"
        - "transaction_conflicts_total"
    
    queries:
      enabled: true
      metrics:
        - "query_executed_total"
        - "query_duration_seconds"
        - "query_errors_total"
        - "slow_queries_total"
    
    storage:
      enabled: true
      metrics:
        - "rocksdb_memtable_size_bytes"
        - "rocksdb_block_cache_usage_bytes"
        - "rocksdb_compaction_pending"
        - "rocksdb_num_immutable_mem_table"
        - "rocksdb_num_running_compactions"
        - "rocksdb_num_running_flushes"
    
    replication:
      enabled: true
      metrics:
        - "replication_lag_seconds"
        - "replication_bytes_sent"
        - "replication_bytes_received"
        - "replication_status"
    
    connections:
      enabled: true
      metrics:
        - "connections_active"
        - "connections_total"
        - "connections_rejected_total"
  
  # Metric labels
  labels:
    instance: "${HOSTNAME}"
    cluster: "main"
    version: "1.4.0"
  
  # Cardinality limits
  cardinality:
    max_timeseries: 10000
    label_value_length_limit: 128
  
  # Performance
  collection_interval_seconds: 15
  async_collection: true
```

**Custom Metrics Example:**

```cpp
// custom_metrics.cpp - Example of custom application metrics
#include "themisdb/metrics.hpp"
#include <prometheus/counter.h>
#include <prometheus/histogram.h>
#include <prometheus/gauge.h>

namespace themisdb {

class ApplicationMetrics {
private:
    // Prometheus metric families
    prometheus::Family<prometheus::Counter>& orders_processed_;
    prometheus::Family<prometheus::Histogram>& order_processing_time_;
    prometheus::Family<prometheus::Gauge>& active_orders_;
    
public:
    ApplicationMetrics(prometheus::Registry& registry) 
        : orders_processed_(prometheus::BuildCounter()
            .Name("app_orders_processed_total")
            .Help("Total number of orders processed")
            .Register(registry)),
          order_processing_time_(prometheus::BuildHistogram()
            .Name("app_order_processing_duration_seconds")
            .Help("Order processing time in seconds")
            .Register(registry)),
          active_orders_(prometheus::BuildGauge()
            .Name("app_active_orders")
            .Help("Number of currently active orders")
            .Register(registry))
    {}
    
    void recordOrderProcessed(const std::string& order_type) {
        auto& counter = orders_processed_.Add({{"type", order_type}});
        counter.Increment();
    }
    
    void recordProcessingTime(double duration_seconds, const std::string& order_type) {
        auto& histogram = order_processing_time_.Add(
            {{"type", order_type}},
            prometheus::Histogram::BucketBoundaries{0.001, 0.01, 0.1, 1.0, 10.0}
        );
        histogram.Observe(duration_seconds);
    }
    
    void setActiveOrders(int count) {
        auto& gauge = active_orders_.Add({});
        gauge.Set(count);
    }
};

} // namespace themisdb
```

### Node Exporter Setup

**Install Node Exporter:**

```bash
#!/bin/bash
# install_node_exporter.sh - System metrics collection

NODE_EXPORTER_VERSION="1.6.1"

cd /tmp
wget https://github.com/prometheus/node_exporter/releases/download/v${NODE_EXPORTER_VERSION}/node_exporter-${NODE_EXPORTER_VERSION}.linux-amd64.tar.gz
tar xvfz node_exporter-${NODE_EXPORTER_VERSION}.linux-amd64.tar.gz

cp node_exporter-${NODE_EXPORTER_VERSION}.linux-amd64/node_exporter /usr/local/bin/
chown prometheus:prometheus /usr/local/bin/node_exporter

# Create systemd service
cat > /etc/systemd/system/node_exporter.service <<'EOF'
[Unit]
Description=Node Exporter
Wants=network-online.target
After=network-online.target

[Service]
User=prometheus
Group=prometheus
Type=simple
ExecStart=/usr/local/bin/node_exporter \
  --collector.filesystem.mount-points-exclude=^/(dev|proc|sys|var/lib/docker/.+)($|/) \
  --collector.filesystem.fs-types-exclude=^(autofs|binfmt_misc|cgroup|configfs|debugfs|devpts|devtmpfs|fusectl|hugetlbfs|mqueue|overlay|proc|procfs|pstore|rpc_pipefs|securityfs|sysfs|tracefs)$ \
  --web.listen-address=:9100

Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable node_exporter
systemctl start node_exporter
```

### Process Exporter Setup

**Install Process Exporter:**

```bash
#!/bin/bash
# install_process_exporter.sh - Per-process metrics

PROCESS_EXPORTER_VERSION="0.7.10"

cd /tmp
wget https://github.com/ncabatoff/process-exporter/releases/download/v${PROCESS_EXPORTER_VERSION}/process-exporter-${PROCESS_EXPORTER_VERSION}.linux-amd64.tar.gz
tar xvfz process-exporter-${PROCESS_EXPORTER_VERSION}.linux-amd64.tar.gz

cp process-exporter-${PROCESS_EXPORTER_VERSION}.linux-amd64/process-exporter /usr/local/bin/
chown prometheus:prometheus /usr/local/bin/process-exporter

# Configuration
cat > /etc/process-exporter/config.yml <<EOF
process_names:
  - name: "{{.Comm}}"
    cmdline:
      - 'themisdb'
EOF

# Systemd service
cat > /etc/systemd/system/process_exporter.service <<'EOF'
[Unit]
Description=Process Exporter
Wants=network-online.target
After=network-online.target

[Service]
User=prometheus
Group=prometheus
Type=simple
ExecStart=/usr/local/bin/process-exporter \
  --config.path=/etc/process-exporter/config.yml \
  --web.listen-address=:9256

Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable process_exporter
systemctl start process_exporter
```


---

## Health Check Endpoints

### Health Check Configuration

**ThemisDB Health Check Endpoints:**

```yaml
# /opt/themisdb/config/health_checks.yaml
health_checks:
  enabled: true
  
  # HTTP endpoint configuration
  http:
    bind_address: "0.0.0.0"
    port: 8080
    
    endpoints:
      # Basic health check
      - path: "/health"
        method: "GET"
        response_format: "json"
        checks:
          - "service_running"
          - "database_accessible"
        
      # Detailed component health
      - path: "/health/components"
        method: "GET"
        response_format: "json"
        checks:
          - "service_running"
          - "database_accessible"
          - "storage_healthy"
          - "replication_status"
          - "memory_available"
          - "disk_space_available"
          - "connection_pool"
      
      # Readiness probe (K8s)
      - path: "/ready"
        method: "GET"
        response_format: "json"
        checks:
          - "database_accessible"
          - "accepting_connections"
      
      # Liveness probe (K8s)
      - path: "/live"
        method: "GET"
        response_format: "json"
        checks:
          - "service_running"
          - "not_deadlocked"
  
  # Check configuration
  checks:
    service_running:
      timeout_seconds: 1
      
    database_accessible:
      timeout_seconds: 5
      test_query: "SELECT 1"
      
    storage_healthy:
      timeout_seconds: 10
      check_rocksdb: true
      check_wal: true
      
    replication_status:
      timeout_seconds: 5
      max_lag_seconds: 10
      
    memory_available:
      min_available_mb: 1024
      
    disk_space_available:
      min_available_gb: 10
      check_paths:
        - "/var/lib/themisdb"
        - "/var/log/themisdb"
    
    connection_pool:
      timeout_seconds: 2
      min_available_connections: 10
    
    accepting_connections:
      timeout_seconds: 2
      test_connection: true
    
    not_deadlocked:
      timeout_seconds: 5
      check_transaction_locks: true
```

**Health Check Responses:**

```json
// GET /health - Basic health check
{
  "status": "healthy",
  "timestamp": "2026-01-18T10:30:45.123Z",
  "version": "1.4.0",
  "uptime_seconds": 86400,
  "checks": {
    "service_running": "pass",
    "database_accessible": "pass"
  }
}

// GET /health/components - Detailed component health
{
  "status": "healthy",
  "timestamp": "2026-01-18T10:30:45.123Z",
  "version": "1.4.0",
  "uptime_seconds": 86400,
  "components": {
    "service": {
      "status": "healthy",
      "message": "Service running normally"
    },
    "database": {
      "status": "healthy",
      "message": "Database accessible",
      "response_time_ms": 2
    },
    "storage": {
      "status": "healthy",
      "message": "RocksDB healthy",
      "details": {
        "memtable_usage_mb": 128,
        "block_cache_usage_mb": 2048,
        "compaction_pending": false
      }
    },
    "replication": {
      "status": "healthy",
      "message": "Replication current",
      "lag_seconds": 0.5
    },
    "memory": {
      "status": "healthy",
      "available_mb": 8192,
      "usage_percent": 65
    },
    "disk": {
      "status": "healthy",
      "paths": {
        "/var/lib/themisdb": {
          "available_gb": 150,
          "usage_percent": 45
        },
        "/var/log/themisdb": {
          "available_gb": 80,
          "usage_percent": 20
        }
      }
    },
    "connections": {
      "status": "healthy",
      "active": 42,
      "available": 958,
      "total": 1000
    }
  }
}

// Unhealthy response example
{
  "status": "unhealthy",
  "timestamp": "2026-01-18T10:30:45.123Z",
  "version": "1.4.0",
  "uptime_seconds": 86400,
  "components": {
    "replication": {
      "status": "unhealthy",
      "message": "Replication lag too high",
      "lag_seconds": 120,
      "threshold_seconds": 10
    },
    "disk": {
      "status": "warning",
      "message": "Low disk space",
      "paths": {
        "/var/lib/themisdb": {
          "available_gb": 5,
          "usage_percent": 95,
          "threshold_gb": 10
        }
      }
    }
  }
}
```

**Health Check Monitoring Script:**

```bash
#!/bin/bash
# health_check_monitor.sh - Monitor ThemisDB health

THEMISDB_HOST="localhost"
THEMISDB_PORT="8080"
HEALTH_ENDPOINT="/health/components"

# Function to check health
check_health() {
    local response=$(curl -s http://$THEMISDB_HOST:$THEMISDB_PORT$HEALTH_ENDPOINT)
    local status=$(echo "$response" | jq -r '.status')
    
    if [ "$status" = "healthy" ]; then
        echo "✓ ThemisDB is healthy"
        return 0
    else
        echo "✗ ThemisDB is unhealthy"
        echo "$response" | jq '.components | to_entries[] | select(.value.status != "healthy")'
        return 1
    fi
}

# Function to check specific component
check_component() {
    local component="$1"
    local response=$(curl -s http://$THEMISDB_HOST:$THEMISDB_PORT$HEALTH_ENDPOINT)
    local comp_status=$(echo "$response" | jq -r ".components.$component.status")
    
    echo "$component: $comp_status"
    
    if [ "$comp_status" != "healthy" ]; then
        echo "$response" | jq ".components.$component"
    fi
}

# Main health check
echo "=== ThemisDB Health Check ==="
echo "Time: $(date)"
echo ""

if check_health; then
    echo ""
    echo "All components healthy"
    exit 0
else
    echo ""
    echo "Health check failed"
    
    # Check individual components
    for component in service database storage replication memory disk connections; do
        check_component "$component"
    done
    
    exit 1
fi
```

### Health Check Frequency Recommendations

**Health Check Schedule:**

| Check Type | Frequency | Timeout | Purpose |
|------------|-----------|---------|---------|
| **Basic Health** (`/health`) | 10 seconds | 1s | Load balancer health checks |
| **Component Health** (`/health/components`) | 30 seconds | 10s | Detailed monitoring |
| **Readiness** (`/ready`) | 5 seconds | 2s | Kubernetes readiness probe |
| **Liveness** (`/live`) | 10 seconds | 5s | Kubernetes liveness probe |
| **Deep Health Check** | 5 minutes | 30s | Comprehensive validation |

**Load Balancer Integration:**

```nginx
# nginx.conf - Health check configuration
upstream themisdb_backend {
    server themisdb-server-01:7700 max_fails=3 fail_timeout=30s;
    server themisdb-server-02:7700 max_fails=3 fail_timeout=30s;
    server themisdb-server-03:7700 max_fails=3 fail_timeout=30s;
    
    # Health check (nginx Plus)
    # health_check interval=10s fails=3 passes=2 uri=/health;
}

server {
    listen 7700;
    
    location / {
        proxy_pass http://themisdb_backend;
        
        # Health check for open source nginx
        proxy_next_upstream error timeout http_503;
        proxy_connect_timeout 2s;
        
        # Headers
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }
}
```

**Kubernetes Probes:**

```yaml
# kubernetes/deployment.yaml - Health probes for ThemisDB
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb
spec:
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
        image: themisdb:1.4.0
        ports:
        - containerPort: 7700
          name: themisdb
        - containerPort: 8080
          name: health
        
        # Liveness probe - restart if unhealthy
        livenessProbe:
          httpGet:
            path: /live
            port: 8080
          initialDelaySeconds: 60
          periodSeconds: 10
          timeoutSeconds: 5
          successThreshold: 1
          failureThreshold: 3
        
        # Readiness probe - remove from service if not ready
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 5
          timeoutSeconds: 2
          successThreshold: 1
          failureThreshold: 2
        
        # Startup probe - allow slow start
        startupProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
          timeoutSeconds: 2
          successThreshold: 1
          failureThreshold: 30  # 150 seconds total
        
        resources:
          requests:
            cpu: 2
            memory: 4Gi
          limits:
            cpu: 4
            memory: 8Gi
```

---

## Alert Configuration

### Prometheus Alert Rules

**ThemisDB Alert Rules:**

```yaml
# /etc/prometheus/rules/themisdb_alerts.yml
groups:
  # Critical alerts - immediate action required
  - name: themisdb_critical
    interval: 30s
    rules:
      # Service down
      - alert: ThemisDBDown
        expr: up{job="themisdb-server"} == 0
        for: 1m
        labels:
          severity: critical
          component: service
        annotations:
          summary: "ThemisDB instance {{ $labels.instance }} is down"
          description: "ThemisDB instance {{ $labels.instance }} has been down for more than 1 minute"
          runbook_url: "https://docs.themisdb.com/troubleshooting#service-down"
      
      # High error rate
      - alert: ThemisDBHighErrorRate
        expr: rate(themisdb_errors_total[5m]) > 10
        for: 5m
        labels:
          severity: critical
          component: errors
        annotations:
          summary: "High error rate on {{ $labels.instance }}"
          description: "Error rate is {{ $value | humanize }} errors/sec on {{ $labels.instance }}"
      
      # Out of memory
      - alert: ThemisDBOutOfMemory
        expr: |
          (themisdb_memory_usage_bytes / themisdb_memory_limit_bytes) > 0.95
        for: 2m
        labels:
          severity: critical
          component: memory
        annotations:
          summary: "ThemisDB {{ $labels.instance }} is running out of memory"
          description: "Memory usage is {{ $value | humanizePercentage }} on {{ $labels.instance }}"
      
      # Disk full
      - alert: ThemisDBDiskFull
        expr: |
          (themisdb_disk_usage_bytes / themisdb_disk_capacity_bytes) > 0.90
        for: 5m
        labels:
          severity: critical
          component: storage
        annotations:
          summary: "ThemisDB {{ $labels.instance }} disk is almost full"
          description: "Disk usage is {{ $value | humanizePercentage }} on {{ $labels.instance }}"
      
      # Replication broken
      - alert: ThemisDBReplicationBroken
        expr: themisdb_replication_status == 0
        for: 2m
        labels:
          severity: critical
          component: replication
        annotations:
          summary: "Replication broken on {{ $labels.instance }}"
          description: "Replication from {{ $labels.source }} to {{ $labels.target }} is broken"
      
      # Too many transaction conflicts
      - alert: ThemisDBHighTransactionConflicts
        expr: rate(themisdb_transaction_conflicts_total[5m]) > 100
        for: 10m
        labels:
          severity: critical
          component: transactions
        annotations:
          summary: "High transaction conflict rate on {{ $labels.instance }}"
          description: "Conflict rate is {{ $value | humanize }} conflicts/sec"
  
  # Warning alerts - attention needed
  - name: themisdb_warnings
    interval: 1m
    rules:
      # High CPU usage
      - alert: ThemisDBHighCPU
        expr: rate(process_cpu_seconds_total{job="themisdb-server"}[5m]) * 100 > 80
        for: 10m
        labels:
          severity: warning
          component: cpu
        annotations:
          summary: "High CPU usage on {{ $labels.instance }}"
          description: "CPU usage is {{ $value | humanize }}% on {{ $labels.instance }}"
      
      # High memory usage
      - alert: ThemisDBHighMemory
        expr: |
          (themisdb_memory_usage_bytes / themisdb_memory_limit_bytes) > 0.80
        for: 10m
        labels:
          severity: warning
          component: memory
        annotations:
          summary: "High memory usage on {{ $labels.instance }}"
          description: "Memory usage is {{ $value | humanizePercentage }}"
      
      # Slow queries
      - alert: ThemisDBSlowQueries
        expr: rate(themisdb_slow_queries_total[5m]) > 1
        for: 10m
        labels:
          severity: warning
          component: performance
        annotations:
          summary: "Slow queries detected on {{ $labels.instance }}"
          description: "{{ $value | humanize }} slow queries per second"
      
      # Replication lag
      - alert: ThemisDBReplicationLag
        expr: themisdb_replication_lag_seconds > 10
        for: 5m
        labels:
          severity: warning
          component: replication
        annotations:
          summary: "Replication lag on {{ $labels.instance }}"
          description: "Replication lag is {{ $value | humanizeDuration }} behind"
      
      # Connection pool saturation
      - alert: ThemisDBConnectionPoolSaturated
        expr: |
          (themisdb_connections_active / themisdb_connections_max) > 0.80
        for: 5m
        labels:
          severity: warning
          component: connections
        annotations:
          summary: "Connection pool saturated on {{ $labels.instance }}"
          description: "{{ $value | humanizePercentage }} of connections in use"
      
      # Compaction pending
      - alert: ThemisDBCompactionPending
        expr: themisdb_rocksdb_compaction_pending > 5
        for: 15m
        labels:
          severity: warning
          component: storage
        annotations:
          summary: "RocksDB compaction pending on {{ $labels.instance }}"
          description: "{{ $value }} compactions pending"
      
      # High transaction duration
      - alert: ThemisDBHighTransactionDuration
        expr: |
          histogram_quantile(0.95, 
            rate(themisdb_transaction_duration_seconds_bucket[5m])
          ) > 1
        for: 10m
        labels:
          severity: warning
          component: transactions
        annotations:
          summary: "High transaction duration on {{ $labels.instance }}"
          description: "P95 transaction duration is {{ $value | humanizeDuration }}"
  
  # Info alerts - informational
  - name: themisdb_info
    interval: 5m
    rules:
      # Certificate expiring
      - alert: ThemisDBCertificateExpiring
        expr: themisdb_certificate_expiry_seconds < (30 * 24 * 3600)
        for: 1h
        labels:
          severity: info
          component: security
        annotations:
          summary: "Certificate expiring on {{ $labels.instance }}"
          description: "Certificate expires in {{ $value | humanizeDuration }}"
      
      # Backup overdue
      - alert: ThemisDBBackupOverdue
        expr: time() - themisdb_last_backup_timestamp_seconds > (24 * 3600)
        for: 1h
        labels:
          severity: info
          component: backup
        annotations:
          summary: "Backup overdue on {{ $labels.instance }}"
          description: "Last backup was {{ $value | humanizeDuration }} ago"
      
      # Version mismatch
      - alert: ThemisDBVersionMismatch
        expr: count(count by (version) (themisdb_build_info)) > 1
        for: 1h
        labels:
          severity: info
          component: cluster
        annotations:
          summary: "ThemisDB version mismatch in cluster"
          description: "Multiple versions detected in cluster"
```

**Recording Rules (Pre-computed metrics):**

```yaml
# /etc/prometheus/rules/themisdb_recording.yml
groups:
  - name: themisdb_recording
    interval: 30s
    rules:
      # Transaction rate
      - record: themisdb:transaction_rate:5m
        expr: rate(themisdb_transaction_committed_total[5m])
      
      # Query rate
      - record: themisdb:query_rate:5m
        expr: rate(themisdb_query_executed_total[5m])
      
      # Error rate
      - record: themisdb:error_rate:5m
        expr: rate(themisdb_errors_total[5m])
      
      # P95 query duration
      - record: themisdb:query_duration:p95:5m
        expr: |
          histogram_quantile(0.95,
            rate(themisdb_query_duration_seconds_bucket[5m])
          )
      
      # P99 query duration
      - record: themisdb:query_duration:p99:5m
        expr: |
          histogram_quantile(0.99,
            rate(themisdb_query_duration_seconds_bucket[5m])
          )
      
      # Memory usage percentage
      - record: themisdb:memory_usage_percent
        expr: |
          100 * (themisdb_memory_usage_bytes / themisdb_memory_limit_bytes)
      
      # Disk usage percentage
      - record: themisdb:disk_usage_percent
        expr: |
          100 * (themisdb_disk_usage_bytes / themisdb_disk_capacity_bytes)
      
      # Connection pool usage percentage
      - record: themisdb:connection_pool_usage_percent
        expr: |
          100 * (themisdb_connections_active / themisdb_connections_max)
      
      # CPU usage per instance
      - record: themisdb:cpu_usage_percent:5m
        expr: |
          100 * rate(process_cpu_seconds_total{job="themisdb-server"}[5m])
      
      # Network I/O rate
      - record: themisdb:network_io_bytes:5m
        expr: |
          rate(themisdb_network_bytes_sent[5m]) + 
          rate(themisdb_network_bytes_received[5m])
```


### Alertmanager Configuration

**Alertmanager Setup:**

```yaml
# /etc/alertmanager/alertmanager.yml
global:
  resolve_timeout: 5m
  smtp_smarthost: 'smtp.example.com:587'
  smtp_from: 'alertmanager@example.com'
  smtp_auth_username: 'alertmanager@example.com'
  smtp_auth_password: 'password'
  smtp_require_tls: true
  
  slack_api_url: 'https://hooks.slack.com/services/XXX/YYY/ZZZ'
  pagerduty_url: 'https://events.pagerduty.com/v2/enqueue'

# Route configuration
route:
  group_by: ['alertname', 'cluster', 'severity']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 12h
  receiver: 'default'
  
  routes:
    # Critical alerts - page on-call
    - match:
        severity: critical
      receiver: 'pagerduty-critical'
      group_wait: 10s
      repeat_interval: 5m
      continue: true
    
    # Critical alerts - also send to Slack
    - match:
        severity: critical
      receiver: 'slack-critical'
      group_wait: 10s
    
    # Warnings - Slack only
    - match:
        severity: warning
      receiver: 'slack-warnings'
      group_wait: 5m
      repeat_interval: 4h
    
    # Info - email only
    - match:
        severity: info
      receiver: 'email-info'
      group_wait: 10m
      repeat_interval: 24h

# Inhibition rules (suppress noisy alerts)
inhibit_rules:
  # If service is down, don't alert on other issues
  - source_match:
      alertname: 'ThemisDBDown'
    target_match_re:
      alertname: 'ThemisDB.*'
    equal: ['instance']
  
  # If disk is full, don't alert on compaction pending
  - source_match:
      alertname: 'ThemisDBDiskFull'
    target_match:
      alertname: 'ThemisDBCompactionPending'
    equal: ['instance']

# Receivers
receivers:
  # Default receiver
  - name: 'default'
    email_configs:
      - to: 'ops-team@example.com'
        headers:
          Subject: 'ThemisDB Alert: {{ .GroupLabels.alertname }}'
        html: '{{ template "email.default.html" . }}'
  
  # PagerDuty for critical alerts
  - name: 'pagerduty-critical'
    pagerduty_configs:
      - service_key: 'YOUR_PAGERDUTY_SERVICE_KEY'
        severity: 'critical'
        description: '{{ .GroupLabels.alertname }}: {{ .CommonAnnotations.summary }}'
        details:
          firing: '{{ .Alerts.Firing | len }}'
          resolved: '{{ .Alerts.Resolved | len }}'
          instance: '{{ .GroupLabels.instance }}'
          description: '{{ .CommonAnnotations.description }}'
  
  # Slack for critical alerts
  - name: 'slack-critical'
    slack_configs:
      - channel: '#themisdb-critical'
        username: 'AlertManager'
        icon_emoji: ':fire:'
        color: 'danger'
        title: '🔥 Critical: {{ .GroupLabels.alertname }}'
        text: |
          {{ range .Alerts }}
          *Instance:* {{ .Labels.instance }}
          *Summary:* {{ .Annotations.summary }}
          *Description:* {{ .Annotations.description }}
          *Runbook:* {{ .Annotations.runbook_url }}
          {{ end }}
  
  # Slack for warnings
  - name: 'slack-warnings'
    slack_configs:
      - channel: '#themisdb-warnings'
        username: 'AlertManager'
        icon_emoji: ':warning:'
        color: 'warning'
        title: '⚠️ Warning: {{ .GroupLabels.alertname }}'
        text: '{{ .CommonAnnotations.description }}'
  
  # Email for info alerts
  - name: 'email-info'
    email_configs:
      - to: 'themisdb-info@example.com'
        headers:
          Subject: 'ThemisDB Info: {{ .GroupLabels.alertname }}'

# Templates
templates:
  - '/etc/alertmanager/templates/*.tmpl'
```

**Custom Alert Templates:**

```go
{{/* /etc/alertmanager/templates/email.tmpl */}}
{{ define "email.default.html" }}
<!DOCTYPE html>
<html>
<head>
    <style>
        body { font-family: Arial, sans-serif; }
        .alert { 
            border: 1px solid #ddd; 
            padding: 15px; 
            margin: 10px 0;
            border-radius: 5px;
        }
        .critical { background-color: #ffdddd; border-color: #ff0000; }
        .warning { background-color: #fff4dd; border-color: #ffa500; }
        .info { background-color: #ddddff; border-color: #0000ff; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
    </style>
</head>
<body>
    <h2>ThemisDB Alert: {{ .GroupLabels.alertname }}</h2>
    <p><strong>Cluster:</strong> {{ .GroupLabels.cluster }}</p>
    <p><strong>Severity:</strong> {{ .GroupLabels.severity }}</p>
    <p><strong>Time:</strong> {{ .CommonAnnotations.timestamp }}</p>
    
    <h3>Firing Alerts ({{ .Alerts.Firing | len }})</h3>
    {{ range .Alerts.Firing }}
    <div class="alert {{ .Labels.severity }}">
        <h4>{{ .Labels.alertname }}</h4>
        <table>
            <tr><th>Instance</th><td>{{ .Labels.instance }}</td></tr>
            <tr><th>Summary</th><td>{{ .Annotations.summary }}</td></tr>
            <tr><th>Description</th><td>{{ .Annotations.description }}</td></tr>
            <tr><th>Started</th><td>{{ .StartsAt }}</td></tr>
            {{ if .Annotations.runbook_url }}
            <tr><th>Runbook</th><td><a href="{{ .Annotations.runbook_url }}">{{ .Annotations.runbook_url }}</a></td></tr>
            {{ end }}
        </table>
    </div>
    {{ end }}
    
    {{ if .Alerts.Resolved }}
    <h3>Resolved Alerts ({{ .Alerts.Resolved | len }})</h3>
    {{ range .Alerts.Resolved }}
    <div class="alert info">
        <h4>{{ .Labels.alertname }} - RESOLVED</h4>
        <table>
            <tr><th>Instance</th><td>{{ .Labels.instance }}</td></tr>
            <tr><th>Resolved</th><td>{{ .EndsAt }}</td></tr>
        </table>
    </div>
    {{ end }}
    {{ end }}
</body>
</html>
{{ end }}
```

---

## Log Aggregation

### Structured Logging Configuration

**ThemisDB Logging Configuration:**

```yaml
# /opt/themisdb/config/logging.yaml
logging:
  # Log levels: trace, debug, info, warn, error, critical
  level: "info"
  
  # Output destinations
  outputs:
    - type: "file"
      path: "/var/log/themisdb/themisdb.log"
      rotation:
        max_size_mb: 100
        max_age_days: 30
        max_backups: 10
        compress: true
      
    - type: "file"
      path: "/var/log/themisdb/error.log"
      level: "error"
      rotation:
        max_size_mb: 50
        max_age_days: 30
        max_backups: 20
        compress: true
    
    - type: "syslog"
      network: "tcp"
      address: "syslog.example.com:514"
      facility: "local0"
      tag: "themisdb"
    
    - type: "stdout"
      level: "info"
      format: "json"  # For container environments
  
  # Format
  format: "json"  # Options: json, text, logfmt
  
  # Structured logging fields
  fields:
    service: "themisdb"
    version: "1.4.0"
    environment: "production"
    hostname: "${HOSTNAME}"
  
  # Component-specific logging
  components:
    storage:
      level: "info"
    transactions:
      level: "info"
    replication:
      level: "debug"  # More verbose for replication
    queries:
      level: "info"
    network:
      level: "warn"
  
  # Performance logging
  performance:
    enabled: true
    slow_query_threshold_ms: 1000
    log_query_plans: true
    log_execution_stats: true
  
  # Sampling (for high-volume logs)
  sampling:
    enabled: true
    initial: 100  # Always log first 100 per second
    thereafter: 10  # Then log 1 in 10
```

**Log Format Example:**

```json
{
  "timestamp": "2026-01-18T10:30:45.123Z",
  "level": "info",
  "service": "themisdb",
  "version": "1.4.0",
  "environment": "production",
  "hostname": "themisdb-server-01",
  "component": "transactions",
  "message": "Transaction committed successfully",
  "transaction_id": "txn_abc123",
  "duration_ms": 15,
  "keys_written": 10,
  "keys_read": 5,
  "user": "app_service",
  "database": "production_db",
  "trace_id": "trace_xyz789",
  "span_id": "span_123"
}
```

### ELK Stack Integration

**Filebeat Configuration:**

```yaml
# /etc/filebeat/filebeat.yml
filebeat.inputs:
  # Application logs
  - type: log
    enabled: true
    paths:
      - /var/log/themisdb/themisdb.log
    fields:
      log_type: application
      service: themisdb
      environment: production
    json.keys_under_root: true
    json.add_error_key: true
    
  # Error logs
  - type: log
    enabled: true
    paths:
      - /var/log/themisdb/error.log
    fields:
      log_type: error
      service: themisdb
      environment: production
    json.keys_under_root: true
    
  # Slow query logs
  - type: log
    enabled: true
    paths:
      - /var/log/themisdb/slow_queries.log
    fields:
      log_type: slow_query
      service: themisdb
      environment: production
    json.keys_under_root: true
    
  # Audit logs (separate pipeline)
  - type: log
    enabled: true
    paths:
      - /var/log/themisdb/audit/*.log
    fields:
      log_type: audit
      service: themisdb
      environment: production
    json.keys_under_root: true

# Processors
processors:
  - add_host_metadata:
      when.not.contains.tags: forwarded
  
  - add_cloud_metadata: ~
  
  - add_docker_metadata: ~
  
  - drop_fields:
      fields: ["agent.ephemeral_id", "agent.id", "ecs.version"]
      ignore_missing: true
  
  # Extract trace context
  - script:
      lang: javascript
      source: >
        function process(event) {
          if (event.Get("trace_id")) {
            event.Put("trace.id", event.Get("trace_id"));
          }
          if (event.Get("span_id")) {
            event.Put("span.id", event.Get("span_id"));
          }
        }

# Output to Logstash
output.logstash:
  hosts: ["logstash-01.example.com:5044", "logstash-02.example.com:5044"]
  loadbalance: true
  ssl.certificate_authorities: ["/etc/pki/tls/certs/ca.crt"]
  ssl.certificate: "/etc/pki/tls/certs/filebeat.crt"
  ssl.key: "/etc/pki/tls/private/filebeat.key"

# Monitoring
monitoring.enabled: true
monitoring.elasticsearch:
  hosts: ["https://monitoring-es.example.com:9200"]
```

**Logstash Pipeline:**

```ruby
# /etc/logstash/conf.d/themisdb.conf
input {
  beats {
    port => 5044
    ssl => true
    ssl_certificate => "/etc/pki/tls/certs/logstash.crt"
    ssl_key => "/etc/pki/tls/private/logstash.key"
    ssl_verify_mode => "force_peer"
    ssl_certificate_authorities => ["/etc/pki/tls/certs/ca.crt"]
  }
}

filter {
  # Parse timestamp
  date {
    match => [ "timestamp", "ISO8601" ]
    target => "@timestamp"
  }
  
  # Add normalized severity
  if [level] == "critical" or [level] == "error" {
    mutate {
      add_field => { "severity_code" => 3 }
    }
  } else if [level] == "warn" {
    mutate {
      add_field => { "severity_code" => 2 }
    }
  } else {
    mutate {
      add_field => { "severity_code" => 1 }
    }
  }
  
  # Extract error details
  if [level] == "error" {
    grok {
      match => { "message" => "%{GREEDYDATA:error_message}" }
    }
  }
  
  # Parse slow queries
  if [fields][log_type] == "slow_query" {
    mutate {
      add_tag => ["slow_query"]
    }
    
    # Extract query metrics
    if [duration_ms] {
      ruby {
        code => "event.set('duration_seconds', event.get('duration_ms') / 1000.0)"
      }
    }
  }
  
  # Enrich with GeoIP (for client IPs)
  if [client_ip] {
    geoip {
      source => "client_ip"
      target => "client_geo"
    }
  }
  
  # Remove unnecessary fields
  mutate {
    remove_field => ["agent", "ecs", "input", "log"]
  }
}

output {
  # Application logs
  if [fields][log_type] == "application" {
    elasticsearch {
      hosts => ["https://es-01.example.com:9200", "https://es-02.example.com:9200"]
      index => "themisdb-application-%{+YYYY.MM.dd}"
      user => "logstash_writer"
      password => "${ELASTICSEARCH_PASSWORD}"
      ssl => true
      cacert => "/etc/pki/tls/certs/ca.crt"
    }
  }
  
  # Error logs (separate index)
  if [fields][log_type] == "error" {
    elasticsearch {
      hosts => ["https://es-01.example.com:9200", "https://es-02.example.com:9200"]
      index => "themisdb-errors-%{+YYYY.MM.dd}"
      user => "logstash_writer"
      password => "${ELASTICSEARCH_PASSWORD}"
    }
  }
  
  # Slow queries
  if [fields][log_type] == "slow_query" {
    elasticsearch {
      hosts => ["https://es-01.example.com:9200", "https://es-02.example.com:9200"]
      index => "themisdb-slow-queries-%{+YYYY.MM.dd}"
      user => "logstash_writer"
      password => "${ELASTICSEARCH_PASSWORD}"
    }
  }
  
  # Audit logs (longer retention)
  if [fields][log_type] == "audit" {
    elasticsearch {
      hosts => ["https://es-01.example.com:9200", "https://es-02.example.com:9200"]
      index => "themisdb-audit-%{+YYYY.MM.dd}"
      user => "logstash_writer"
      password => "${ELASTICSEARCH_PASSWORD}"
      pipeline => "audit-pipeline"
    }
  }
}
```

**Elasticsearch Index Templates:**

```json
// PUT _index_template/themisdb-logs
{
  "index_patterns": ["themisdb-*"],
  "priority": 100,
  "template": {
    "settings": {
      "number_of_shards": 3,
      "number_of_replicas": 1,
      "refresh_interval": "5s",
      "index.lifecycle.name": "themisdb-logs-policy",
      "index.lifecycle.rollover_alias": "themisdb-logs"
    },
    "mappings": {
      "properties": {
        "@timestamp": {
          "type": "date"
        },
        "timestamp": {
          "type": "date"
        },
        "level": {
          "type": "keyword"
        },
        "service": {
          "type": "keyword"
        },
        "hostname": {
          "type": "keyword"
        },
        "component": {
          "type": "keyword"
        },
        "message": {
          "type": "text",
          "fields": {
            "keyword": {
              "type": "keyword",
              "ignore_above": 256
            }
          }
        },
        "transaction_id": {
          "type": "keyword"
        },
        "user": {
          "type": "keyword"
        },
        "database": {
          "type": "keyword"
        },
        "duration_ms": {
          "type": "long"
        },
        "trace_id": {
          "type": "keyword"
        },
        "span_id": {
          "type": "keyword"
        }
      }
    }
  }
}

// Index Lifecycle Management Policy
PUT _ilm/policy/themisdb-logs-policy
{
  "policy": {
    "phases": {
      "hot": {
        "min_age": "0ms",
        "actions": {
          "rollover": {
            "max_size": "50gb",
            "max_age": "1d"
          },
          "set_priority": {
            "priority": 100
          }
        }
      },
      "warm": {
        "min_age": "7d",
        "actions": {
          "shrink": {
            "number_of_shards": 1
          },
          "forcemerge": {
            "max_num_segments": 1
          },
          "set_priority": {
            "priority": 50
          }
        }
      },
      "cold": {
        "min_age": "30d",
        "actions": {
          "freeze": {},
          "set_priority": {
            "priority": 0
          }
        }
      },
      "delete": {
        "min_age": "90d",
        "actions": {
          "delete": {}
        }
      }
    }
  }
}
```

### Log Analysis Queries

**Useful Elasticsearch Queries:**

```json
// Find errors in last hour
GET themisdb-errors-*/_search
{
  "query": {
    "bool": {
      "must": [
        {
          "term": {
            "level": "error"
          }
        },
        {
          "range": {
            "@timestamp": {
              "gte": "now-1h"
            }
          }
        }
      ]
    }
  },
  "sort": [
    {
      "@timestamp": "desc"
    }
  ],
  "size": 100
}

// Top 10 slow queries
GET themisdb-slow-queries-*/_search
{
  "query": {
    "range": {
      "@timestamp": {
        "gte": "now-24h"
      }
    }
  },
  "aggs": {
    "slow_queries": {
      "terms": {
        "field": "query.text.keyword",
        "size": 10,
        "order": {
          "avg_duration": "desc"
        }
      },
      "aggs": {
        "avg_duration": {
          "avg": {
            "field": "duration_ms"
          }
        },
        "max_duration": {
          "max": {
            "field": "duration_ms"
          }
        },
        "count": {
          "value_count": {
            "field": "duration_ms"
          }
        }
      }
    }
  },
  "size": 0
}

// Error rate over time
GET themisdb-errors-*/_search
{
  "query": {
    "range": {
      "@timestamp": {
        "gte": "now-24h"
      }
    }
  },
  "aggs": {
    "errors_over_time": {
      "date_histogram": {
        "field": "@timestamp",
        "fixed_interval": "1h"
      },
      "aggs": {
        "by_component": {
          "terms": {
            "field": "component"
          }
        }
      }
    }
  },
  "size": 0
}

// Transaction patterns
GET themisdb-application-*/_search
{
  "query": {
    "bool": {
      "must": [
        {
          "term": {
            "component": "transactions"
          }
        },
        {
          "range": {
            "@timestamp": {
              "gte": "now-1h"
            }
          }
        }
      ]
    }
  },
  "aggs": {
    "by_user": {
      "terms": {
        "field": "user",
        "size": 20
      },
      "aggs": {
        "avg_duration": {
          "avg": {
            "field": "duration_ms"
          }
        },
        "total_operations": {
          "sum": {
            "field": "keys_written"
          }
        }
      }
    }
  },
  "size": 0
}
```


---

## Dashboarding

### Grafana Installation and Configuration

**Grafana Installation:**

```bash
#!/bin/bash
# install_grafana.sh

# Add Grafana repository
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
echo "deb https://packages.grafana.com/oss/deb stable main" | sudo tee /etc/apt/sources.list.d/grafana.list

# Install
sudo apt-get update
sudo apt-get install -y grafana

# Start and enable
sudo systemctl enable grafana-server
sudo systemctl start grafana-server

echo "✓ Grafana installed at http://localhost:3000"
echo "Default credentials: admin/admin"
```

**Grafana Configuration:**

```ini
# /etc/grafana/grafana.ini
[server]
protocol = https
http_port = 3000
domain = grafana.example.com
root_url = https://grafana.example.com
cert_file = /etc/grafana/ssl/grafana.crt
cert_key = /etc/grafana/ssl/grafana.key

[database]
type = postgres
host = postgres.example.com:5432
name = grafana
user = grafana
password = ${GRAFANA_DB_PASSWORD}
ssl_mode = require

[security]
admin_user = admin
admin_password = ${GRAFANA_ADMIN_PASSWORD}
secret_key = ${GRAFANA_SECRET_KEY}
disable_gravatar = true
cookie_secure = true
cookie_samesite = strict

[auth]
disable_login_form = false
disable_signout_menu = false

[auth.anonymous]
enabled = false

[auth.ldap]
enabled = true
config_file = /etc/grafana/ldap.toml
allow_sign_up = true

[smtp]
enabled = true
host = smtp.example.com:587
user = grafana@example.com
password = ${SMTP_PASSWORD}
from_address = grafana@example.com
from_name = Grafana

[alerting]
enabled = true
execute_alerts = true

[metrics]
enabled = true
interval_seconds = 10
```

### ThemisDB Dashboard JSON

**Comprehensive ThemisDB Dashboard:**

```json
{
  "dashboard": {
    "title": "ThemisDB - Overview",
    "tags": ["themisdb", "database", "production"],
    "timezone": "browser",
    "refresh": "30s",
    "time": {
      "from": "now-1h",
      "to": "now"
    },
    "panels": [
      {
        "id": 1,
        "title": "Service Status",
        "type": "stat",
        "targets": [
          {
            "expr": "up{job=\"themisdb-server\"}",
            "legendFormat": "{{instance}}"
          }
        ],
        "gridPos": {"x": 0, "y": 0, "w": 6, "h": 4},
        "options": {
          "graphMode": "none",
          "colorMode": "background",
          "textMode": "auto"
        },
        "fieldConfig": {
          "defaults": {
            "mappings": [
              {"type": "value", "value": "0", "text": "DOWN", "color": "red"},
              {"type": "value", "value": "1", "text": "UP", "color": "green"}
            ],
            "thresholds": {
              "steps": [
                {"value": 0, "color": "red"},
                {"value": 1, "color": "green"}
              ]
            }
          }
        }
      },
      {
        "id": 2,
        "title": "Transaction Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(themisdb_transaction_committed_total[5m])",
            "legendFormat": "{{instance}}"
          }
        ],
        "gridPos": {"x": 6, "y": 0, "w": 9, "h": 8},
        "yaxes": [
          {"label": "txn/sec", "format": "short"},
          {"show": false}
        ]
      },
      {
        "id": 3,
        "title": "Query Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(themisdb_query_executed_total[5m])",
            "legendFormat": "{{instance}}"
          }
        ],
        "gridPos": {"x": 15, "y": 0, "w": 9, "h": 8},
        "yaxes": [
          {"label": "queries/sec", "format": "short"},
          {"show": false}
        ]
      },
      {
        "id": 4,
        "title": "CPU Usage",
        "type": "graph",
        "targets": [
          {
            "expr": "100 * rate(process_cpu_seconds_total{job=\"themisdb-server\"}[5m])",
            "legendFormat": "{{instance}}"
          }
        ],
        "gridPos": {"x": 0, "y": 8, "w": 12, "h": 8},
        "yaxes": [
          {"label": "CPU %", "format": "percent", "max": 100},
          {"show": false}
        ],
        "alert": {
          "name": "High CPU Usage",
          "conditions": [
            {
              "evaluator": {"params": [80], "type": "gt"},
              "query": {"params": ["A", "5m", "now"]}
            }
          ]
        }
      },
      {
        "id": 5,
        "title": "Memory Usage",
        "type": "graph",
        "targets": [
          {
            "expr": "themisdb_memory_usage_bytes / 1024 / 1024 / 1024",
            "legendFormat": "{{instance}} - Used"
          },
          {
            "expr": "themisdb_memory_limit_bytes / 1024 / 1024 / 1024",
            "legendFormat": "{{instance}} - Limit"
          }
        ],
        "gridPos": {"x": 12, "y": 8, "w": 12, "h": 8},
        "yaxes": [
          {"label": "Memory (GB)", "format": "bytes"},
          {"show": false}
        ]
      },
      {
        "id": 6,
        "title": "Replication Lag",
        "type": "graph",
        "targets": [
          {
            "expr": "themisdb_replication_lag_seconds",
            "legendFormat": "{{source}} -> {{target}}"
          }
        ],
        "gridPos": {"x": 0, "y": 16, "w": 12, "h": 8},
        "yaxes": [
          {"label": "Lag (seconds)", "format": "s"},
          {"show": false}
        ],
        "alert": {
          "name": "High Replication Lag",
          "conditions": [
            {
              "evaluator": {"params": [10], "type": "gt"},
              "query": {"params": ["A", "5m", "now"]}
            }
          ]
        }
      },
      {
        "id": 7,
        "title": "Active Connections",
        "type": "graph",
        "targets": [
          {
            "expr": "themisdb_connections_active",
            "legendFormat": "{{instance}} - Active"
          },
          {
            "expr": "themisdb_connections_max",
            "legendFormat": "{{instance}} - Max"
          }
        ],
        "gridPos": {"x": 12, "y": 16, "w": 12, "h": 8},
        "yaxes": [
          {"label": "Connections", "format": "short"},
          {"show": false}
        ]
      },
      {
        "id": 8,
        "title": "Query Duration (P95)",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(themisdb_query_duration_seconds_bucket[5m]))",
            "legendFormat": "{{instance}}"
          }
        ],
        "gridPos": {"x": 0, "y": 24, "w": 12, "h": 8},
        "yaxes": [
          {"label": "Duration (s)", "format": "s"},
          {"show": false}
        ]
      },
      {
        "id": 9,
        "title": "Error Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(themisdb_errors_total[5m])",
            "legendFormat": "{{instance}} - {{error_type}}"
          }
        ],
        "gridPos": {"x": 12, "y": 24, "w": 12, "h": 8},
        "yaxes": [
          {"label": "errors/sec", "format": "short"},
          {"show": false}
        ]
      },
      {
        "id": 10,
        "title": "RocksDB Compaction",
        "type": "graph",
        "targets": [
          {
            "expr": "themisdb_rocksdb_compaction_pending",
            "legendFormat": "{{instance}} - Pending"
          },
          {
            "expr": "themisdb_rocksdb_num_running_compactions",
            "legendFormat": "{{instance}} - Running"
          }
        ],
        "gridPos": {"x": 0, "y": 32, "w": 12, "h": 8}
      },
      {
        "id": 11,
        "title": "Disk Usage",
        "type": "graph",
        "targets": [
          {
            "expr": "100 * (themisdb_disk_usage_bytes / themisdb_disk_capacity_bytes)",
            "legendFormat": "{{instance}} - {{path}}"
          }
        ],
        "gridPos": {"x": 12, "y": 32, "w": 12, "h": 8},
        "yaxes": [
          {"label": "Usage %", "format": "percent", "max": 100},
          {"show": false}
        ]
      }
    ]
  }
}
```

**Performance Dashboard:**

```json
{
  "dashboard": {
    "title": "ThemisDB - Performance Deep Dive",
    "tags": ["themisdb", "performance"],
    "panels": [
      {
        "title": "Transaction Latency Heatmap",
        "type": "heatmap",
        "targets": [
          {
            "expr": "sum(rate(themisdb_transaction_duration_seconds_bucket[5m])) by (le)",
            "format": "heatmap",
            "legendFormat": "{{le}}"
          }
        ],
        "gridPos": {"x": 0, "y": 0, "w": 24, "h": 8},
        "heatmap": {
          "yAxis": {"format": "s", "decimals": 2}
        }
      },
      {
        "title": "Query Performance by Type",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(themisdb_query_duration_seconds_bucket{query_type=~\"SELECT|INSERT|UPDATE|DELETE\"}[5m])) by (query_type)",
            "legendFormat": "{{query_type}} - P95"
          }
        ],
        "gridPos": {"x": 0, "y": 8, "w": 12, "h": 8}
      },
      {
        "title": "Cache Hit Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "100 * rate(themisdb_cache_hits_total[5m]) / (rate(themisdb_cache_hits_total[5m]) + rate(themisdb_cache_misses_total[5m]))",
            "legendFormat": "{{instance}} - {{cache_type}}"
          }
        ],
        "gridPos": {"x": 12, "y": 8, "w": 12, "h": 8},
        "yaxes": [
          {"label": "Hit Rate %", "format": "percent", "max": 100}
        ]
      }
    ]
  }
}
```

### Key Performance Indicators (KPIs)

**ThemisDB KPI Dashboard:**

| KPI | Target | Warning | Critical | Query |
|-----|--------|---------|----------|-------|
| **Service Availability** | 99.99% | < 99.9% | < 99% | `avg_over_time(up{job="themisdb-server"}[30d]) * 100` |
| **Transaction Success Rate** | > 99.9% | < 99.5% | < 99% | `100 * rate(themisdb_transaction_committed_total[5m]) / (rate(themisdb_transaction_committed_total[5m]) + rate(themisdb_transaction_aborted_total[5m]))` |
| **Query Latency (P95)** | < 100ms | > 200ms | > 500ms | `histogram_quantile(0.95, rate(themisdb_query_duration_seconds_bucket[5m])) * 1000` |
| **Query Latency (P99)** | < 500ms | > 1s | > 5s | `histogram_quantile(0.99, rate(themisdb_query_duration_seconds_bucket[5m])) * 1000` |
| **Replication Lag** | < 1s | > 5s | > 10s | `themisdb_replication_lag_seconds` |
| **Error Rate** | < 0.1% | > 0.5% | > 1% | `100 * rate(themisdb_errors_total[5m]) / rate(themisdb_requests_total[5m])` |
| **CPU Usage** | < 70% | > 80% | > 90% | `100 * rate(process_cpu_seconds_total{job="themisdb-server"}[5m])` |
| **Memory Usage** | < 80% | > 85% | > 90% | `100 * themisdb_memory_usage_bytes / themisdb_memory_limit_bytes` |
| **Disk Usage** | < 80% | > 85% | > 90% | `100 * themisdb_disk_usage_bytes / themisdb_disk_capacity_bytes` |
| **Connection Pool Usage** | < 70% | > 80% | > 90% | `100 * themisdb_connections_active / themisdb_connections_max` |

**SLA Tracking Dashboard:**

```json
{
  "dashboard": {
    "title": "ThemisDB - SLA Tracking",
    "tags": ["themisdb", "sla"],
    "panels": [
      {
        "title": "Monthly Availability",
        "type": "stat",
        "targets": [
          {
            "expr": "avg_over_time(up{job=\"themisdb-server\"}[30d]) * 100",
            "legendFormat": "Availability %"
          }
        ],
        "gridPos": {"x": 0, "y": 0, "w": 6, "h": 4},
        "fieldConfig": {
          "defaults": {
            "unit": "percent",
            "decimals": 3,
            "thresholds": {
              "steps": [
                {"value": 0, "color": "red"},
                {"value": 99, "color": "yellow"},
                {"value": 99.9, "color": "green"}
              ]
            }
          }
        }
      },
      {
        "title": "Monthly Downtime",
        "type": "stat",
        "targets": [
          {
            "expr": "(1 - avg_over_time(up{job=\"themisdb-server\"}[30d])) * 30 * 24 * 60",
            "legendFormat": "Downtime (minutes)"
          }
        ],
        "gridPos": {"x": 6, "y": 0, "w": 6, "h": 4},
        "fieldConfig": {
          "defaults": {
            "unit": "m",
            "decimals": 1,
            "thresholds": {
              "steps": [
                {"value": 0, "color": "green"},
                {"value": 43, "color": "yellow"},
                {"value": 432, "color": "red"}
              ]
            }
          }
        }
      },
      {
        "title": "Error Budget Remaining",
        "type": "gauge",
        "targets": [
          {
            "expr": "100 - (100 * sum(rate(themisdb_errors_total[30d])) / sum(rate(themisdb_requests_total[30d])))",
            "legendFormat": "Error Budget %"
          }
        ],
        "gridPos": {"x": 12, "y": 0, "w": 6, "h": 4},
        "options": {
          "showThresholdLabels": true,
          "showThresholdMarkers": true
        },
        "fieldConfig": {
          "defaults": {
            "unit": "percent",
            "min": 0,
            "max": 100,
            "thresholds": {
              "steps": [
                {"value": 0, "color": "red"},
                {"value": 50, "color": "yellow"},
                {"value": 80, "color": "green"}
              ]
            }
          }
        }
      }
    ]
  }
}
```

---

## Performance Monitoring

### Transaction Performance Analysis

**Transaction Metrics Collection:**

```yaml
# Transaction performance metrics
transaction_metrics:
  - name: transaction_duration_seconds
    type: histogram
    help: "Transaction execution duration"
    buckets: [0.001, 0.01, 0.1, 0.5, 1, 5, 10]
    labels:
      - isolation_level
      - database
      - result  # committed, aborted, conflict
  
  - name: transaction_operations
    type: histogram
    help: "Number of operations per transaction"
    buckets: [1, 10, 50, 100, 500, 1000, 5000]
    labels:
      - operation_type  # read, write, delete
  
  - name: transaction_conflicts_total
    type: counter
    help: "Total number of transaction conflicts"
    labels:
      - conflict_type  # write-write, read-write
      - database
```

**Query Performance Profiling:**

```bash
#!/bin/bash
# query_performance_profile.sh - Analyze query performance

# Top 10 slowest queries
curl -s http://localhost:9091/metrics | \
  grep themisdb_query_duration | \
  sort -t '=' -k2 -nr | \
  head -10

# Query rate by type
promtool query instant http://localhost:9090 \
  'sum(rate(themisdb_query_executed_total[5m])) by (query_type)'

# P95 latency by database
promtool query instant http://localhost:9090 \
  'histogram_quantile(0.95, sum(rate(themisdb_query_duration_seconds_bucket[5m])) by (database, le))'

# Active transactions
promtool query instant http://localhost:9090 \
  'themisdb_transaction_active'

# Transaction conflicts rate
promtool query instant http://localhost:9090 \
  'rate(themisdb_transaction_conflicts_total[5m])'
```

### Storage Performance Monitoring

**RocksDB Metrics:**

```yaml
# RocksDB performance metrics
rocksdb_metrics:
  # Memtable
  - themisdb_rocksdb_memtable_size_bytes
  - themisdb_rocksdb_num_immutable_mem_table
  - themisdb_rocksdb_mem_table_flush_pending
  
  # Block cache
  - themisdb_rocksdb_block_cache_usage_bytes
  - themisdb_rocksdb_block_cache_hit_ratio
  
  # Compaction
  - themisdb_rocksdb_compaction_pending
  - themisdb_rocksdb_num_running_compactions
  - themisdb_rocksdb_num_running_flushes
  - themisdb_rocksdb_compaction_time_seconds
  
  # WAL
  - themisdb_rocksdb_wal_file_size_bytes
  - themisdb_rocksdb_wal_file_synced_total
  
  # SST files
  - themisdb_rocksdb_num_files_at_level
  - themisdb_rocksdb_total_sst_file_size_bytes
  
  # Read/Write
  - themisdb_rocksdb_get_latency_seconds
  - themisdb_rocksdb_write_latency_seconds
```

---

## Capacity Planning

### Resource Utilization Trends

**Capacity Planning Queries:**

```promql
# Storage growth rate (GB/day)
deriv(themisdb_disk_usage_bytes[1d]) * 86400 / 1024 / 1024 / 1024

# Projected days until disk full
(themisdb_disk_capacity_bytes - themisdb_disk_usage_bytes) / 
(deriv(themisdb_disk_usage_bytes[7d]) * 86400)

# Memory growth rate
deriv(themisdb_memory_usage_bytes[7d]) * 86400

# Connection pool saturation trend
predict_linear(themisdb_connections_active[1d], 7*24*3600)

# Transaction rate growth
deriv(rate(themisdb_transaction_committed_total[1d])[7d]) * 86400
```

**Capacity Planning Report Script:**

```bash
#!/bin/bash
# capacity_planning_report.sh

PROMETHEUS_URL="http://localhost:9090"
REPORT_FILE="/tmp/capacity_report_$(date +%Y%m%d).txt"

cat > $REPORT_FILE <<EOF
=== ThemisDB Capacity Planning Report ===
Generated: $(date)

EOF

# Storage capacity
echo "=== Storage Capacity ===" >> $REPORT_FILE
DISK_USAGE=$(curl -s "${PROMETHEUS_URL}/api/v1/query?query=themisdb_disk_usage_bytes/1024/1024/1024" | jq -r '.data.result[0].value[1]')
DISK_CAPACITY=$(curl -s "${PROMETHEUS_URL}/api/v1/query?query=themisdb_disk_capacity_bytes/1024/1024/1024" | jq -r '.data.result[0].value[1]')
DISK_PERCENT=$(echo "scale=2; $DISK_USAGE * 100 / $DISK_CAPACITY" | bc)

echo "Current Usage: ${DISK_USAGE} GB / ${DISK_CAPACITY} GB (${DISK_PERCENT}%)" >> $REPORT_FILE

# Growth rate
GROWTH_RATE=$(curl -s "${PROMETHEUS_URL}/api/v1/query?query=deriv(themisdb_disk_usage_bytes[7d])*86400/1024/1024/1024" | jq -r '.data.result[0].value[1]')
echo "Growth Rate: ${GROWTH_RATE} GB/day" >> $REPORT_FILE

# Days until full
DAYS_UNTIL_FULL=$(echo "scale=0; ($DISK_CAPACITY - $DISK_USAGE) / $GROWTH_RATE" | bc)
echo "Days Until Full: ${DAYS_UNTIL_FULL} days" >> $REPORT_FILE
echo "" >> $REPORT_FILE

# Memory capacity
echo "=== Memory Capacity ===" >> $REPORT_FILE
MEM_USAGE=$(curl -s "${PROMETHEUS_URL}/api/v1/query?query=themisdb_memory_usage_bytes/1024/1024/1024" | jq -r '.data.result[0].value[1]')
MEM_LIMIT=$(curl -s "${PROMETHEUS_URL}/api/v1/query?query=themisdb_memory_limit_bytes/1024/1024/1024" | jq -r '.data.result[0].value[1]')
MEM_PERCENT=$(echo "scale=2; $MEM_USAGE * 100 / $MEM_LIMIT" | bc)

echo "Current Usage: ${MEM_USAGE} GB / ${MEM_LIMIT} GB (${MEM_PERCENT}%)" >> $REPORT_FILE
echo "" >> $REPORT_FILE

# Transaction rate
echo "=== Transaction Capacity ===" >> $REPORT_FILE
TXN_RATE=$(curl -s "${PROMETHEUS_URL}/api/v1/query?query=rate(themisdb_transaction_committed_total[1h])" | jq -r '.data.result[0].value[1]')
echo "Current Rate: ${TXN_RATE} txn/sec" >> $REPORT_FILE

# Peak transaction rate
TXN_PEAK=$(curl -s "${PROMETHEUS_URL}/api/v1/query?query=max_over_time(rate(themisdb_transaction_committed_total[5m])[7d])" | jq -r '.data.result[0].value[1]')
echo "Peak Rate (7d): ${TXN_PEAK} txn/sec" >> $REPORT_FILE
echo "" >> $REPORT_FILE

# Recommendations
echo "=== Recommendations ===" >> $REPORT_FILE

if (( $(echo "$DISK_PERCENT > 80" | bc -l) )); then
    echo "⚠️  Disk usage > 80% - Consider adding storage capacity" >> $REPORT_FILE
fi

if (( $(echo "$MEM_PERCENT > 80" | bc -l) )); then
    echo "⚠️  Memory usage > 80% - Consider increasing memory" >> $REPORT_FILE
fi

if (( $(echo "$DAYS_UNTIL_FULL < 30" | bc -l) )); then
    echo "🔴 Storage will be full in < 30 days - Urgent action required" >> $REPORT_FILE
fi

cat $REPORT_FILE
```

---

## Related Documentation

- **[Production Deployment Guide](../deployment/PRODUCTION_DEPLOYMENT_GUIDE.md)** - Initial deployment procedures
- **[Operational Procedures](OPERATIONAL_PROCEDURES.md)** - Day-to-day operations
- **[Troubleshooting Guide](TROUBLESHOOTING_GUIDE.md)** - Diagnosing and resolving issues
- **[Security Deployment Guide](../security/SECURITY_DEPLOYMENT_GUIDE.md)** - Security monitoring
- **[MVCC Tuning Guide](../features/MVCC_TUNING_GUIDE.md)** - Transaction performance
- **[RocksDB Optimization Guide](../storage/ROCKSDB_OPTIMIZATION_GUIDE.md)** - Storage tuning

---

**Document Version:** 1.0  
**ThemisDB Compatibility:** 1.4.0+  
**Last Reviewed:** 2026-01-18  
**Next Review:** 2026-04-18

