# Multi-Agent LLM Reasoning: Production Deployment Guide

**Version:** v1.4.0  
**Date:** December 20, 2025  
**Status:** Production-Ready Framework

---

## 📋 Overview

This guide covers deployment of the ThemisDB Multi-Agent LLM Reasoning framework in production environments. The framework enables collaborative problem-solving using multiple specialized LLM agents with LoRA adapters.

---

## 🏗️ Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     Load Balancer / Reverse Proxy                │
│                         (nginx / HAProxy)                         │
└────────────────────────┬────────────────────────────────────────┘
                         │
         ┌───────────────┼───────────────┐
         │               │               │
         ▼               ▼               ▼
    ┌─────────┐    ┌─────────┐    ┌─────────┐
    │ ThemisDB│    │ ThemisDB│    │ ThemisDB│
    │ Instance│    │ Instance│    │ Instance│
    │ 1       │    │ 2       │    │ 3       │
    └────┬────┘    └────┬────┘    └────┬────┘
         │              │              │
         └──────────────┼──────────────┘
                        │
              ┌─────────▼─────────┐
              │   RocksDB Cluster  │
              │   (Persistent)     │
              └────────────────────┘
```

---

## 🚀 Deployment Steps

### 1. System Requirements

**Minimum:**
- CPU: 8 cores (16 threads)
- RAM: 16 GB
- Storage: 100 GB SSD
- Network: 1 Gbps

**Recommended for Production:**
- CPU: 16+ cores (32+ threads)
- RAM: 32-64 GB
- Storage: 500 GB NVMe SSD
- Network: 10 Gbps
- GPU: Optional (for v1.5.0 LLM inference)

**Operating System:**
- Linux (Ubuntu 22.04 LTS, Debian 12, RHEL 8/9)
- macOS 12+ (development only)
- Windows Server 2022 (via WSL2)

### 2. Build Configuration

Enable multi-agent module in CMake:

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_BUILD_TESTS=ON \
  -DTHEMIS_ENABLE_TRACING=ON \
  -DTHEMIS_ENABLE_GPU=OFF \
  -DTHEMIS_STRICT_BUILD=OFF

cmake --build build --parallel $(nproc)
```

**Build Options:**
- `THEMIS_BUILD_TESTS`: Include test suite (recommended for staging)
- `THEMIS_ENABLE_TRACING`: Enable OpenTelemetry (recommended)
- `THEMIS_ENABLE_GPU`: GPU acceleration (for v1.5.0+)

### 3. Configuration

Create `config/multi_agent_prod.yaml`:

```yaml
orchestrator:
  name: "Production Multi-Agent Orchestrator"
  strategy: PARALLEL
  max_concurrent_agents: 10
  timeout_seconds: 60
  
agents:
  - id: "legal_expert_prod"
    role: "legal_expert"
    base_model: "mistral-7b-instruct-v0.2"
    lora_adapter: "legal_contracts_v2"
    max_context_length: 4096
    temperature: 0.7
    replicas: 2  # For load balancing
    
  - id: "technical_analyst_prod"
    role: "technical_analyst"
    base_model: "mistral-7b-instruct-v0.2"
    lora_adapter: "tech_analysis_v1"
    max_context_length: 4096
    temperature: 0.6
    replicas: 2
    
  - id: "business_strategist_prod"
    role: "business_strategist"
    base_model: "llama-3-8b"
    lora_adapter: "business_analysis_v1"
    max_context_length: 4096
    temperature: 0.7
    replicas: 2

consensus:
  strategy: WEIGHTED_AVERAGE
  confidence_threshold: 0.75
  role_weights:
    legal_expert: 0.4
    technical_analyst: 0.35
    business_strategist: 0.25

persistence:
  rocksdb_path: "/var/lib/themisdb/data"
  backup_path: "/var/lib/themisdb/backups"
  backup_interval_hours: 24

monitoring:
  prometheus_port: 9090
  metrics_enabled: true
  tracing_enabled: true
  log_level: "info"
```

### 4. Database Setup

Initialize RocksDB for multi-agent data:

```bash
# Create data directories
sudo mkdir -p /var/lib/themisdb/data
sudo mkdir -p /var/lib/themisdb/backups
sudo chown -R themis:themis /var/lib/themisdb

# Initialize database (run as themis user)
themis_server --init --config config/multi_agent_prod.yaml
```

### 5. Service Configuration

Create systemd service `/etc/systemd/system/themisdb.service`:

```ini
[Unit]
Description=ThemisDB Multi-Agent LLM Server
After=network.target
Wants=network.target

[Service]
Type=simple
User=themis
Group=themis
WorkingDirectory=/opt/themisdb
ExecStart=/opt/themisdb/bin/themis_server --config /etc/themisdb/config.yaml
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=10s
LimitNOFILE=65536
LimitNPROC=4096

# Resource limits
MemoryMax=32G
CPUQuota=1600%

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable themisdb
sudo systemctl start themisdb
sudo systemctl status themisdb
```

---

## 🔒 Security Configuration

### TLS/SSL Configuration

Enable HTTPS in config:

```yaml
server:
  enable_tls: true
  tls_cert_path: "/etc/themisdb/certs/server.crt"
  tls_key_path: "/etc/themisdb/certs/server.key"
  tls_ca_cert_path: "/etc/themisdb/certs/ca.crt"
  tls_min_version: "TLSv1.3"
  tls_require_client_cert: true  # mTLS
```

### Authentication

```yaml
auth:
  enabled: true
  jwt_secret: "${JWT_SECRET}"  # From environment
  token_expiry_hours: 24
  
rbac:
  enabled: true
  roles:
    - name: "admin"
      permissions: ["*"]
    - name: "analyst"
      permissions: ["multi-agent:read", "multi-agent:analyze"]
    - name: "viewer"
      permissions: ["multi-agent:read"]
```

### Rate Limiting

```yaml
rate_limiting:
  enabled: true
  requests_per_minute: 100
  burst_size: 20
  by_ip: true
  by_user: true
```

---

## 📊 Monitoring & Observability

### Prometheus Metrics

Key metrics exposed at `/metrics`:

```
# Agent metrics
multi_agent_requests_total{agent_id, role}
multi_agent_latency_seconds{agent_id, task_type}
multi_agent_active_sessions{agent_id}
multi_agent_errors_total{agent_id, error_type}

# Orchestrator metrics
orchestrator_decomposition_time_seconds
orchestrator_consensus_time_seconds
orchestrator_active_agents
orchestrator_consensus_conflicts_total

# LoRA metrics
lora_adapters_loaded
lora_memory_bytes_used
lora_load_duration_seconds
lora_gc_runs_total
```

### Grafana Dashboard

Import dashboard from `grafana/multi_agent_dashboard.json`:

**Panels:**
- Agent Response Times (heatmap)
- Active Agents (gauge)
- Request Rate (graph)
- Error Rate (graph)
- Consensus Success Rate (single stat)
- LoRA Memory Usage (gauge)

### Logging

Configure structured logging:

```yaml
logging:
  level: "info"  # debug, info, warn, error
  format: "json"
  output: "/var/log/themisdb/multi-agent.log"
  rotation:
    max_size_mb: 100
    max_files: 10
    compress: true
```

### Distributed Tracing

OpenTelemetry configuration:

```yaml
tracing:
  enabled: true
  exporter: "jaeger"
  endpoint: "http://jaeger:14268/api/traces"
  service_name: "themisdb-multi-agent"
  sample_rate: 0.1  # 10% sampling
```

---

## 🔧 Performance Tuning

### Agent Pool Sizing

```yaml
agent_pool:
  min_agents_per_role: 2
  max_agents_per_role: 10
  scale_up_threshold: 0.8  # 80% utilization
  scale_down_threshold: 0.2  # 20% utilization
  cooldown_seconds: 60
```

### RocksDB Tuning

```yaml
rocksdb:
  max_background_jobs: 16
  max_write_buffer_number: 4
  write_buffer_size_mb: 128
  block_cache_size_mb: 2048
  bloom_filter_bits: 10
  compression: "zstd"
  level0_file_num_compaction_trigger: 4
```

### Connection Pooling

```yaml
connection_pool:
  max_connections: 1000
  min_idle: 10
  connection_timeout_ms: 5000
  idle_timeout_ms: 300000  # 5 minutes
```

---

## 🐳 Docker Deployment

### Docker Compose

`docker-compose.yml`:

```yaml
version: '3.8'

services:
  themisdb:
    image: themisdb/themisdb:v1.4.0
    container_name: themisdb-multi-agent
    ports:
      - "8765:8765"
      - "9090:9090"  # Prometheus metrics
    volumes:
      - themis_data:/var/lib/themisdb/data
      - themis_config:/etc/themisdb
      - themis_logs:/var/log/themisdb
    environment:
      - THEMIS_CONFIG=/etc/themisdb/config.yaml
      - JWT_SECRET=${JWT_SECRET}
    restart: unless-stopped
    deploy:
      resources:
        limits:
          cpus: '16'
          memory: 32G
        reservations:
          cpus: '8'
          memory: 16G
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8765/health"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 60s

  prometheus:
    image: prom/prometheus:latest
    ports:
      - "9091:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
      - prometheus_data:/prometheus
    command:
      - '--config.file=/etc/prometheus/prometheus.yml'

  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"
    volumes:
      - grafana_data:/var/lib/grafana
    environment:
      - GF_SECURITY_ADMIN_PASSWORD=${GRAFANA_PASSWORD}

volumes:
  themis_data:
  themis_config:
  themis_logs:
  prometheus_data:
  grafana_data:
```

Deploy:

```bash
docker-compose up -d
docker-compose logs -f themisdb
```

---

## ☸️ Kubernetes Deployment

### StatefulSet

`k8s/statefulset.yaml`:

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: themisdb-multi-agent
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
        image: themisdb/themisdb:v1.4.0
        ports:
        - containerPort: 8765
          name: http
        - containerPort: 9090
          name: metrics
        env:
        - name: THEMIS_CONFIG
          value: /etc/themisdb/config.yaml
        - name: JWT_SECRET
          valueFrom:
            secretKeyRef:
              name: themisdb-secrets
              key: jwt-secret
        volumeMounts:
        - name: data
          mountPath: /var/lib/themisdb/data
        - name: config
          mountPath: /etc/themisdb
        resources:
          requests:
            cpu: "8"
            memory: "16Gi"
          limits:
            cpu: "16"
            memory: "32Gi"
        livenessProbe:
          httpGet:
            path: /health
            port: 8765
          initialDelaySeconds: 60
          periodSeconds: 30
        readinessProbe:
          httpGet:
            path: /health
            port: 8765
          initialDelaySeconds: 30
          periodSeconds: 10
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: "fast-ssd"
      resources:
        requests:
          storage: 500Gi
```

Deploy:

```bash
kubectl apply -f k8s/namespace.yaml
kubectl apply -f k8s/configmap.yaml
kubectl apply -f k8s/secrets.yaml
kubectl apply -f k8s/statefulset.yaml
kubectl apply -f k8s/service.yaml
kubectl apply -f k8s/ingress.yaml
```

---

## 🔄 High Availability

### Load Balancing

nginx configuration:

```nginx
upstream themisdb_backend {
    least_conn;
    server themisdb1:8765 max_fails=3 fail_timeout=30s;
    server themisdb2:8765 max_fails=3 fail_timeout=30s;
    server themisdb3:8765 max_fails=3 fail_timeout=30s;
}

server {
    listen 443 ssl http2;
    server_name themisdb.example.com;
    
    ssl_certificate /etc/nginx/certs/server.crt;
    ssl_certificate_key /etc/nginx/certs/server.key;
    ssl_protocols TLSv1.3;
    
    location /api/llm/multi-agent/ {
        proxy_pass http://themisdb_backend;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 120s;
    }
}
```

### Health Checks

```bash
# Health check endpoint
curl http://localhost:8765/health

# Response:
{
  "status": "healthy",
  "version": "1.4.0",
  "multi_agent": {
    "orchestrator": "active",
    "registered_agents": 6,
    "loaded_lora_adapters": 3
  }
}
```

---

## 🐛 Troubleshooting

### Common Issues

**Issue: High latency on multi-agent requests**
```bash
# Check agent pool utilization
curl http://localhost:8765/api/llm/multi-agent/stats

# Solution: Increase agent replicas
# Edit config: agent_pool.max_agents_per_role = 20
```

**Issue: LoRA adapters not loading**
```bash
# Check LoRA registry
curl http://localhost:8765/api/llm/multi-agent/lora

# Verify paths in config
ls -la /var/lib/themisdb/lora_adapters/

# Check permissions
sudo chown -R themis:themis /var/lib/themisdb/lora_adapters/
```

**Issue: Consensus failures**
```bash
# Check logs
tail -f /var/log/themisdb/multi-agent.log | grep consensus

# Adjust consensus threshold
# config: consensus.confidence_threshold = 0.6
```

### Debug Mode

Enable debug logging:

```yaml
logging:
  level: "debug"
  components:
    - "multi_agent"
    - "orchestrator"
    - "consensus"
```

---

## 📈 Scaling Guidelines

### Vertical Scaling

| Load Level | CPU | RAM | Storage | Agents per Role |
|------------|-----|-----|---------|-----------------|
| Small      | 8   | 16GB| 100GB   | 2-3             |
| Medium     | 16  | 32GB| 250GB   | 4-6             |
| Large      | 32  | 64GB| 500GB   | 8-10            |
| Enterprise | 64+ | 128GB| 1TB+   | 15-20           |

### Horizontal Scaling

```bash
# Add more instances
docker-compose scale themisdb=5

# Or in Kubernetes
kubectl scale statefulset themisdb-multi-agent --replicas=5
```

---

## 🔐 Backup & Recovery

### Automated Backups

```bash
# Backup script
#!/bin/bash
BACKUP_DIR="/var/lib/themisdb/backups/$(date +%Y%m%d_%H%M%S)"
mkdir -p $BACKUP_DIR

# Backup RocksDB
rsync -av /var/lib/themisdb/data/ $BACKUP_DIR/data/

# Backup configurations
cp -r /etc/themisdb/ $BACKUP_DIR/config/

# Backup LoRA adapters
cp -r /var/lib/themisdb/lora_adapters/ $BACKUP_DIR/lora/

# Compress
tar -czf $BACKUP_DIR.tar.gz $BACKUP_DIR
rm -rf $BACKUP_DIR
```

Schedule with cron:
```cron
0 2 * * * /opt/themisdb/scripts/backup.sh
```

### Recovery

```bash
# Stop service
sudo systemctl stop themisdb

# Restore from backup
tar -xzf backup_20251220_020000.tar.gz
rsync -av backup_20251220_020000/data/ /var/lib/themisdb/data/

# Start service
sudo systemctl start themisdb
```

---

## 📞 Support

- **Documentation**: https://themisdb.docs/multi-agent
- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Discussions**: https://github.com/makr-code/ThemisDB/discussions
- **Email**: support@themisdb.io

---

**Version**: v1.4.0  
**Last Updated**: December 20, 2025  
**Status**: Production-Ready
