# Kapitel 30: Deployment & Operations

> *"Deployment ist kein einmaliger Akt, sondern ein kontinuierlicher Prozess."*

---

## Überblick

Dieses Kapitel behandelt die produktive Bereitstellung von ThemisDB: von Docker-Containern über Kubernetes bis hin zu Cloud-Deployments. Außerdem lernen Sie Monitoring, Backup-Strategien und Skalierungstechniken.

**Was Sie in diesem Kapitel lernen:**
- Docker-basierte Deployments
- Kubernetes-Orchestrierung mit Helm Charts
- Cloud-Provider-Integration (AWS, Azure, GCP)
- Monitoring mit Prometheus & Grafana
- Backup & Disaster Recovery
- Horizontal Scaling & Sharding
- Security Best Practices

**Voraussetzungen:** Kapitel 4 (Installation), Grundkenntnisse in Containerisierung.

---

## 30.1 Docker Deployment

### 30.1.1 Basic Docker Setup

**Dockerfile:**

```dockerfile
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y libssl3 libzstd1
RUN useradd -r themis
COPY themis_server /usr/local/bin/
EXPOSE 8529
CMD ["themis_server"]
```

**docker-compose.yml:**

```yaml
services:
  themis:
    image: themisdb/themis:v1.3.4
    ports:
      - "8529:8529"
    volumes:
      - themis-data:/var/lib/themis/data
    healthcheck:
      test: ["curl", "-f", "http://localhost:8529/_api/version"]

volumes:
  themis-data:
```

**Starten:**

```bash
# Build Image
docker build -t themisdb/themis:v1.3.4 .

# Start with docker-compose
docker-compose up -d

# Check logs
docker-compose logs -f themis

# Health check
curl http://localhost:8529/_api/version
```

### 30.1.2 Docker Multi-Stage Build

```dockerfile
# Stage 1: Build
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    git \
    ninja-build \
    libssl-dev \
    libzstd-dev \
    libjemalloc-dev

WORKDIR /build
COPY . .
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --target themis_server -j8

# Stage 2: Runtime
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libssl3 \
    libzstd1 \
    libjemalloc2 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /build/build/themis_server /usr/local/bin/

RUN useradd -r -u 999 -g users themis && \
    mkdir -p /var/lib/themis/data && \
    chown -R themis:users /var/lib/themis

USER themis
EXPOSE 8529 8530

CMD ["/usr/local/bin/themis_server"]
```

---

## 30.2 Kubernetes Deployment

### 30.2.1 Kubernetes Manifests

**namespace.yaml:**

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: themis-prod
```

**configmap.yaml:**

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: themis-config
  namespace: themis-prod
data:
  themis.conf: |
    [database]
    path = /var/lib/themis/data
    
    [server]
    endpoint = http://0.0.0.0:8529
    threads = 16
    
    [logging]
    level = info
    file = /var/log/themis/themis.log
```

**statefulset.yaml:**

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: themis
  namespace: themis-prod
spec:
  serviceName: themis-headless
  replicas: 3
  selector:
    matchLabels:
      app: themis
  template:
    metadata:
      labels:
        app: themis
    spec:
      containers:
      - name: themis
        image: themisdb/themis:v1.3.4
        ports:
        - containerPort: 8529
          name: http
        - containerPort: 8530
          name: websocket
        volumeMounts:
        - name: data
          mountPath: /var/lib/themis/data
        - name: config
          mountPath: /etc/themis
        resources:
          requests:
            memory: "4Gi"
            cpu: "2000m"
          limits:
            memory: "8Gi"
            cpu: "4000m"
        livenessProbe:
          httpGet:
            path: /_api/version
            port: 8529
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /_api/version
            port: 8529
          initialDelaySeconds: 5
          periodSeconds: 5
      volumes:
      - name: config
        configMap:
          name: themis-config
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: [ "ReadWriteOnce" ]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
```

**service.yaml:**

```yaml
apiVersion: v1
kind: Service
metadata:
  name: themis-headless
  namespace: themis-prod
spec:
  clusterIP: None
  selector:
    app: themis
  ports:
  - port: 8529
    name: http
  - port: 8530
    name: websocket

---
apiVersion: v1
kind: Service
metadata:
  name: themis-loadbalancer
  namespace: themis-prod
spec:
  type: LoadBalancer
  selector:
    app: themis
  ports:
  - port: 8529
    targetPort: 8529
    name: http
```

**Deploy:**

```bash
# Apply all manifests
kubectl apply -f namespace.yaml
kubectl apply -f configmap.yaml
kubectl apply -f statefulset.yaml
kubectl apply -f service.yaml

# Check pods
kubectl get pods -n themis-prod

# Check service
kubectl get svc -n themis-prod

# Logs
kubectl logs -f themis-0 -n themis-prod
```

### 30.2.2 Helm Chart

**Chart.yaml:**

```yaml
apiVersion: v2
name: themis
description: A Helm chart for ThemisDB
type: application
version: 1.3.4
appVersion: "1.3.4"
```

**values.yaml:**

```yaml
replicaCount: 3

image:
  repository: themisdb/themis
  tag: v1.3.4
  pullPolicy: IfNotPresent

resources:
  requests:
    memory: 4Gi
    cpu: 2000m
  limits:
    memory: 8Gi
    cpu: 4000m

persistence:
  enabled: true
  storageClass: fast-ssd
  size: 100Gi

service:
  type: LoadBalancer
  port: 8529

ingress:
  enabled: true
  className: nginx
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
  hosts:
    - host: themis.example.com
      paths:
        - path: /
          pathType: Prefix
  tls:
    - secretName: themis-tls
      hosts:
        - themis.example.com

monitoring:
  enabled: true
  serviceMonitor:
    enabled: true
    interval: 30s
```

**Install Helm Chart:**

```bash
# Add Helm repo
helm repo add themis https://charts.themisdb.io
helm repo update

# Install
helm install themis-prod themis/themis \
  --namespace themis-prod \
  --create-namespace \
  --values values.yaml

# Upgrade
helm upgrade themis-prod themis/themis \
  --namespace themis-prod \
  --values values.yaml

# Uninstall
helm uninstall themis-prod -n themis-prod
```

---

## 30.3 Cloud Provider Integration

### 30.3.1 AWS Deployment

**EKS Cluster:**

```bash
# Create EKS cluster
eksctl create cluster \
  --name themis-prod \
  --region eu-central-1 \
  --nodegroup-name standard-workers \
  --node-type m5.2xlarge \
  --nodes 3 \
  --nodes-min 3 \
  --nodes-max 10 \
  --managed

# Install ThemisDB
helm install themis-prod themis/themis \
  --namespace themis-prod \
  --create-namespace \
  --set persistence.storageClass=gp3 \
  --set service.annotations."service\.beta\.kubernetes\.io/aws-load-balancer-type"=nlb
```

**RDS Integration (Optional):**

```yaml
# Use RDS PostgreSQL for metadata
metadata:
  backend: postgresql
  connection: "postgresql://user:pass@themis-metadata.c9akljh4.eu-central-1.rds.amazonaws.com:5432/themis"
```

### 30.3.2 Azure Deployment

**AKS Cluster:**

```bash
# Create AKS cluster
az aks create \
  --resource-group themis-rg \
  --name themis-prod \
  --location westeurope \
  --node-count 3 \
  --node-vm-size Standard_D8s_v3 \
  --enable-managed-identity \
  --generate-ssh-keys

# Get credentials
az aks get-credentials \
  --resource-group themis-rg \
  --name themis-prod

# Install ThemisDB
helm install themis-prod themis/themis \
  --namespace themis-prod \
  --create-namespace \
  --set persistence.storageClass=managed-premium
```

### 30.3.3 GCP Deployment

**GKE Cluster:**

```bash
# Create GKE cluster
gcloud container clusters create themis-prod \
  --region europe-west3 \
  --num-nodes 3 \
  --machine-type n2-standard-8 \
  --disk-size 100

# Get credentials
gcloud container clusters get-credentials themis-prod \
  --region europe-west3

# Install ThemisDB
helm install themis-prod themis/themis \
  --namespace themis-prod \
  --create-namespace \
  --set persistence.storageClass=pd-ssd
```

---

## 30.4 Monitoring & Observability

### 30.4.1 Prometheus Integration

**ServiceMonitor:**

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: themis
  namespace: themis-prod
spec:
  selector:
    matchLabels:
      app: themis
  endpoints:
  - port: http
    path: /metrics
    interval: 30s
```

**Key Metrics:**

```
# Requests
themis_http_requests_total
themis_http_request_duration_seconds

# Database
themis_db_operations_total
themis_db_size_bytes
themis_db_compaction_duration_seconds

# Cache
themis_cache_hit_ratio
themis_cache_size_bytes

# Threads
themis_thread_pool_active
themis_thread_pool_queue_size
```

### 30.4.2 Grafana Dashboards

**Dashboard JSON:**

```json
{
  "dashboard": {
    "title": "ThemisDB Overview",
    "panels": [
      {
        "title": "Request Rate",
        "targets": [
          {
            "expr": "rate(themis_http_requests_total[5m])"
          }
        ]
      },
      {
        "title": "Database Size",
        "targets": [
          {
            "expr": "themis_db_size_bytes"
          }
        ]
      },
      {
        "title": "Cache Hit Ratio",
        "targets": [
          {
            "expr": "themis_cache_hit_ratio"
          }
        ]
      }
    ]
  }
}
```

### 30.4.3 Alerting Rules

**prometheus-rules.yaml:**

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: themis-alerts
  namespace: themis-prod
spec:
  groups:
  - name: themis
    interval: 30s
    rules:
    - alert: ThemisHighErrorRate
      expr: rate(themis_http_requests_total{status=~"5.."}[5m]) > 0.05
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "High error rate on ThemisDB"
        description: "Error rate is {{ $value }} req/s"
    
    - alert: ThemisHighMemoryUsage
      expr: (container_memory_usage_bytes{pod=~"themis-.*"} / container_spec_memory_limit_bytes) > 0.9
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "ThemisDB high memory usage"
        description: "Memory usage is {{ $value | humanizePercentage }}"
    
    - alert: ThemisDiskSpaceLow
      expr: (node_filesystem_avail_bytes{mountpoint="/var/lib/themis/data"} / node_filesystem_size_bytes) < 0.1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "ThemisDB low disk space"
        description: "Only {{ $value | humanizePercentage }} disk space remaining"
```

---

## 30.5 Backup & Disaster Recovery

### 30.5.1 Snapshot Backups

**Backup Script:**

```bash
#!/bin/bash

# Backup configuration
BACKUP_DIR="/var/backups/themis"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
DB_PATH="/var/lib/themis/data"

# Create snapshot
curl -X POST http://localhost:8529/_admin/backup/create \
  -H "Content-Type: application/json" \
  -d '{
    "label": "backup-'$TIMESTAMP'",
    "timeout": 300
  }'

# Wait for snapshot
sleep 10

# Copy snapshot
rsync -av --progress $DB_PATH/snapshots/backup-$TIMESTAMP \
  $BACKUP_DIR/backup-$TIMESTAMP

# Upload to S3
aws s3 sync $BACKUP_DIR/backup-$TIMESTAMP \
  s3://themis-backups/backup-$TIMESTAMP \
  --storage-class GLACIER

# Cleanup old local backups (keep last 7 days)
find $BACKUP_DIR -type d -mtime +7 -exec rm -rf {} +

echo "Backup completed: backup-$TIMESTAMP"
```

**Cron Job:**

```cron
# Daily backup at 2 AM
0 2 * * * /usr/local/bin/themis-backup.sh >> /var/log/themis-backup.log 2>&1
```

### 30.5.2 Point-in-Time Recovery

```bash
# List available backups
aws s3 ls s3://themis-backups/

# Download backup
aws s3 sync s3://themis-backups/backup-20250115_020000 \
  /var/restore/themis/

# Stop ThemisDB
systemctl stop themis

# Restore data
rm -rf /var/lib/themis/data
cp -r /var/restore/themis /var/lib/themis/data

# Start ThemisDB
systemctl start themis

# Verify
curl http://localhost:8529/_api/version
```

### 30.5.3 Continuous Backup with WAL

```yaml
# Enable WAL archiving
backup:
  wal_archiving:
    enabled: true
    archive_command: "aws s3 cp %p s3://themis-wal/%f"
    archive_timeout: 60s
```

---

## 30.6 Horizontal Scaling

### 30.6.1 Sharding Strategy

```mermaid
flowchart TB
    A[Load Balancer] --> B[Shard 1<br/>ID Range: 0-333]
    A --> C[Shard 2<br/>ID Range: 334-666]
    A --> D[Shard 3<br/>ID Range: 667-999]
    
    B --> B1[(RocksDB 1)]
    C --> C1[(RocksDB 2)]
    D --> D1[(RocksDB 3)]
    
    style A fill:#e1f5ff
    style B fill:#fff4e1
    style C fill:#e1ffe1
    style D fill:#ffe1f5
```

Abb. 30.1: Deployment-Strategy-Matrix

**Sharding Config:**

```yaml
sharding:
  enabled: true
  strategy: hash
  shards:
    - id: shard1
      endpoint: http://themis-shard1:8529
      range: [0, 333]
    - id: shard2
      endpoint: http://themis-shard2:8529
      range: [334, 666]
    - id: shard3
      endpoint: http://themis-shard3:8529
      range: [667, 999]
```

### 30.6.2 Read Replicas

```yaml
replication:
  enabled: true
  mode: async
  replicas:
    - endpoint: http://themis-replica1:8529
      lag_max: 1s
    - endpoint: http://themis-replica2:8529
      lag_max: 1s
```

---

## 30.7 Security Best Practices

### 30.7.1 TLS/SSL Configuration

```yaml
server:
  tls:
    enabled: true
    cert_file: /etc/themis/tls/cert.pem
    key_file: /etc/themis/tls/key.pem
    ca_file: /etc/themis/tls/ca.pem
    min_version: "1.3"
```

### 30.7.2 Authentication

```yaml
auth:
  enabled: true
  providers:
    - type: jwt
      issuer: "https://auth.example.com"
      audience: "themis-api"
      jwks_uri: "https://auth.example.com/.well-known/jwks.json"
    - type: basic
      users_file: /etc/themis/users.htpasswd
```

### 30.7.3 Network Policies (Kubernetes)

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: themis-netpol
  namespace: themis-prod
spec:
  podSelector:
    matchLabels:
      app: themis
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8529
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: redis
    ports:
    - protocol: TCP
      port: 6379
```

---

## 30.8 Performance Tuning

### 30.8.1 Resource Limits

```yaml
resources:
  requests:
    memory: "8Gi"
    cpu: "4000m"
  limits:
    memory: "16Gi"
    cpu: "8000m"

# JVM-style tuning for RocksDB
env:
  - name: ROCKSDB_MAX_OPEN_FILES
    value: "10000"
  - name: ROCKSDB_BLOCK_CACHE_SIZE
    value: "4GB"
```

### 30.8.2 Connection Pooling

```yaml
server:
  connection_pool:
    size: 100
    timeout: 30s
    max_idle: 50
```

---

## 30.9 Zusammenfassung

### Deployment-Optionen

1. **Docker:** Einfacher Einstieg, lokale Entwicklung
2. **Kubernetes:** Produktionsreif, auto-scaling, self-healing
3. **Cloud:** Managed Services (EKS, AKS, GKE)

### Operations-Checkliste

- ✅ Monitoring mit Prometheus & Grafana
- ✅ Tägliche Backups mit S3/Glacier
- ✅ TLS/SSL für alle Verbindungen
- ✅ Network Policies für Pod-Isolation
- ✅ Resource Limits für Stabilität
- ✅ Alerting für kritische Metriken
- ✅ Disaster Recovery Plan dokumentiert

---

## 30.8 Advanced Deployment Patterns

### 30.8.1 Blue-Green Deployments

```yaml
# Strategy: Run two complete ThemisDB environments
# Route traffic via load balancer
apiVersion: v1
kind: Service
metadata:
  name: themis-service
spec:
  selector:
    app: themis
    version: blue  # Initially point to blue
  ports:
    - port: 8529
      targetPort: 8529
  type: LoadBalancer
---
# Blue: Current production
apiVersion: v1
kind: ConfigMap
metadata:
  name: themis-blue-config
data:
  version: "blue"
  database:
    replication_role: "primary"
---
# Green: New version (idle)
apiVersion: v1
kind:ConfigMap
metadata:
  name: themis-green-config
data:
  version: "green"
  database:
    replication_role: "primary"
```

**Deployment Steps:**
1. Deploy Green with new version (idle)
2. Run smoke tests against Green
3. Sync data: Primary → Green (catchup)
4. Switch load balancer: Service selector → green
5. Blue becomes backup/rollback

### 30.8.2 Canary Deployments

```yaml
# Route % of traffic to new version
# Gradually increase as stability confirmed
---
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: themis-canary
spec:
  hosts:
  - themis.internal
  http:
  - match:
    - uri:
        prefix: "/"
    route:
    - destination:
        host: themis-stable
        port:
          number: 8529
      weight: 95  # 95% to stable
    - destination:
        host: themis-canary
        port:
          number: 8529
      weight: 5   # 5% to canary (new version)
    timeout: 10s
    retries:
      attempts: 3
      perTryTimeout: 5s
```

**Canary Rollout Timeline:**
- Hour 0-1: 5% traffic → Monitor error rates, latency
- Hour 1-4: 25% traffic → Check P99 latency
- Hour 4-8: 50% traffic → Full test with real workload
- Hour 8+: 100% traffic → Full rollout

### 30.8.3 GitOps Continuous Deployment

```yaml
# Use ArgoCD to sync Kubernetes manifests from Git
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: themis-gitops
spec:
  project: default
  source:
    repoURL: https://github.com/org/themis-deployment.git
    targetRevision: main
    path: k8s/production
  destination:
    server: https://kubernetes.default.svc
    namespace: themis
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
    - CreateNamespace=true
  # Only deploy if tests pass in CI
  notification:
    when:
      onSuccess: notify-slack
      onFailure: notify-pagerduty
```

### 30.8.4 Staged Rollout Checklist

```
Pre-Deployment
  ✓ Run all integration tests
  ✓ Performance benchmarks (P99 latency < 10% increase)
  ✓ Memory/CPU profiling
  ✓ Backup created & tested

Deployment
  ✓ Dry-run: 'kubectl apply --dry-run=client -f manifest.yaml'
  ✓ Deploy to staging first
  ✓ Smoke tests pass
  ✓ Metrics baseline: CPU, Memory, QPS, Latency
  ✓ Error rate < 0.1%

Monitoring (First Hour)
  ✓ P99 latency stable
  ✓ No error spikes
  ✓ No memory leaks
  ✓ Replication lag < 500ms

Post-Deployment
  ✓ Keep blue environment for 24h (rollback ready)
  ✓ Archive logs for audit
  ✓ Document any issues/hotfixes
  ✓ Send deployment notification
```

---

## 30.9 Disaster Recovery & Business Continuity

### 30.9.1 RTO/RPO Strategy

```
RTO (Recovery Time Objective) = Max downtime acceptable
RPO (Recovery Point Objective) = Max data loss acceptable

ThemisDB Recommendations:
  ┌──────────────────────────────────────────────────┐
  │ Tier      │ RTO      │ RPO      │ Cost  │ Effort│
  ├──────────────────────────────────────────────────┤
  │ Bronze    │ 24h      │ 24h      │ $     │ Low   │
  │ Silver    │ 4h       │ 1h       │ $$    │ Med   │
  │ Gold      │ 1h       │ 5min     │ $$$   │ High  │
  │ Platinum  │ 15min    │ 0min(RPO)│ $$$$  │ Critical
  └──────────────────────────────────────────────────┘

Platinum = Multi-region sync (4 data centers, quorum write)
```

### 30.9.2 Disaster Recovery Plan

```yaml
# Automated DR Workflow
Kind: DisasterRecoveryPlan
metadata:
  name: themis-dr-plan
spec:
  backup:
    primary_daily: /backups/daily/themis-*.tar.gz
    archive_weekly: s3://backups/weekly/
    retention_years: 3
    
  failover_sequence:
    - detect_failure: "Primary health check fails 3x"
    - promote_secondary: "Promote Secondary to Primary"
    - validate_data: "Count docs, verify checksums"
    - test_write: "Insert test document"
    - notify_team: "PagerDuty alert + Slack message"
    - client_failover: "Update DNS CNAME"
    - monitor_2h: "Watch metrics for 2 hours"
    
  recovery_from_backup:
    - stop_database: "systemctl stop themis"
    - restore_volume: "Restore from snapshot"
    - verify_integrity: "themisdb recover --verify"
    - start_database: "systemctl start themis"
    - catchup_replicas: "Wait for lag < 1s"
```

### 30.9.3 Backup Verification Script

```bash
#!/bin/bash
# Daily backup integrity check

BACKUP_DIR="/backups/daily"
VERIFY_TIMEOUT=3600

for backup in $BACKUP_DIR/themis-*.tar.gz; do
  echo "Verifying $backup..."
  
  # Extract to temp, verify database
  TEMP_DIR=$(mktemp -d)
  tar -xzf "$backup" -C "$TEMP_DIR"
  
  # Run recovery to check integrity
  timeout $VERIFY_TIMEOUT themisdb recover --verify "$TEMP_DIR" && \
    echo "✓ $backup: OK" || \
    echo "✗ $backup: CORRUPT - Investigate!"
  
  # Try restoring to test instance
  docker run --rm -v "$TEMP_DIR:/data" themis:test \
    themisdb check-data /data && \
    echo "✓ Data schema valid" || \
    echo "✗ Schema check failed"
  
  rm -rf "$TEMP_DIR"
done

echo "Backup verification complete"
```

---

## 30.10 Cost Optimization

### 30.10.1 Resource Right-Sizing

```yaml
# Development (small)
requests:
  memory: "1Gi"
  cpu: "500m"
limits:
  memory: "2Gi"
  cpu: "1000m"
storage: "50Gi"

# Staging (medium)
requests:
  memory: "8Gi"
  cpu: "4"
limits:
  memory: "16Gi"
  cpu: "8"
storage: "500Gi"

# Production (large)
requests:
  memory: "32Gi"
  cpu: "16"
limits:
  memory: "64Gi"
  cpu: "32"
storage: "2Ti"

# Cost Savings
# - Use spot instances for stateless pods: 70% savings
# - Right-size replicas: scale down off-peak hours
# - Archive old data to cold storage (S3 Glacier)
# - Estimated monthly savings: 40-60% via optimization
```

### 30.10.2 Storage Cost Reduction

```
Strategy: Tiered Storage (Hot → Warm → Cold)

Hot (SSD, expensive)
  ├─ Active data < 30 days
  ├─ Real-time queries
  └─ RTO requirement: <1h

Warm (HDD, medium)
  ├─ Data 30-90 days old
  ├─ Monthly analytics reports
  └─ RTO requirement: <24h

Cold (Cloud Archive, cheap)
  ├─ Compliance archives (2+ years)
  ├─ Quarterly audits only
  └─ RTO requirement: 1-7 days

Cost/GB/Month: SSD ($0.10) → HDD ($0.05) → Archive ($0.004)

Example: 10TB dataset
  SSD: $1,024/month
  Mixed (70% warm, 30% cold): $216/month
  Savings: 79%
```

---

## 30.11 Compliance & Audit

### 30.11.1 Audit Logging for Compliance

```yaml
# Immutable audit log (Write-Once-Read-Many)
kind: ThemisDB
metadata:
  name: themis-production
spec:
  audit:
    enabled: true
    destination: s3://audit-logs/  # External WORM storage
    events:
      - user_login
      - data_access
      - schema_change
      - admin_action
      - privilege_grant
      - backup_restore
    
  compliance:
    frameworks:
      - SOC2
      - ISO27001
      - GDPR
      - HIPAA (if healthcare)
    
  data_retention:
    audit_logs: "7 years"
    application_logs: "90 days"
    metrics: "1 year"
```

### 30.11.2 Compliance Checklist

```
Security
  ✓ TLS 1.3 for all connections
  ✓ Network policies (allow only required)
  ✓ RBAC with MFA for admin access
  ✓ Secrets in Vault/K8s Secrets
  ✓ Encryption at rest (AES-256)
  ✓ Encryption in transit (TLS)

Data Protection
  ✓ PII detection & masking
  ✓ Data classification (PUBLIC/INTERNAL/CONFIDENTIAL)
  ✓ GDPR right-to-deletion implemented
  ✓ Backup encryption with managed keys
  ✓ No cleartext passwords in logs

Operations
  ✓ Change control (code review before deploy)
  ✓ Incident response playbooks
  ✓ Disaster recovery tested (quarterly)
  ✓ Security scanning (SBOM, vulnerability scanning)
  ✓ Penetration testing (annual)

Monitoring
  ✓ Access logs (all queries logged)
  ✓ Admin action audit trail
  ✓ Security event alerting
  ✓ SLA monitoring (uptime SLA > 99.9%)
```

---

## 30.12 Operations Runbooks

### 30.12.1 Scaling Checklist (Horizontal)

```
Add Node to Cluster
  1. Provision new node (VM/Bare Metal)
  2. Install ThemisDB + dependencies
  3. Configure as follower initially
  4. Join cluster: themisdb cluster join --primary <primary-ip>
  5. Wait for replication lag < 100ms
  6. Promote to voting member: themisdb cluster promote --member-id <id>
  7. Verify: themisdb cluster status (should show HEALTHY)
  8. Monitor CPU/Memory/IO for 24h

Remove Node from Cluster
  1. Drain connections: themisdb admin drain-connections
  2. Wait for active queries to finish (30s timeout)
  3. Demote member: themisdb cluster demote --member-id <id>
  4. Remove from cluster: themisdb cluster remove --member-id <id>
  5. Verify: themisdb cluster status (node should be REMOVED)
  6. Decommission VM
```

### 30.12.2 Incident Response (Example: High Memory Usage)

```
Symptoms
  - Memory usage > 85%
  - OOM killer started (dmesg shows)
  - Queries failing with "Out of memory"

Investigation (2 min)
  curl http://localhost:8529/_admin/memory | jq .
  # Check: Cache size, Buffer pool, Active cursors

Fix (5 min)
  Option A: Restart (emergency)
    systemctl restart themis
    → Memory drops to baseline
    
  Option B: Increase memory (planned)
    Edit themis.conf: cache.size_mb = 16384
    systemctl reload themis
    
  Option C: Identify leak (analysis)
    themisdb profile --duration 60s | grep alloc
    → Find problematic query/operation

Verify
  curl http://localhost:8529/_admin/memory | jq .usage_pct
  # Should be < 80%
```

---

## 30.13 Zusammenfassung: Operations Checkliste

### Tägliche Aufgaben
- [ ] Backup erfolgreich (log check)
- [ ] Replication lag < 500ms
- [ ] Disk usage < 80%
- [ ] No error logs above WARN
- [ ] Metrics collected (Prometheus)

### Wöchentliche Aufgaben
- [ ] Backup test restoration (pick random)
- [ ] Security scan (OWASP top 10)
- [ ] Performance baseline check
- [ ] Review failed queries (slow log)
- [ ] Capacity planning review

### Monatliche Aufgaben
- [ ] Disaster recovery drill
- [ ] Penetration testing (if required)
- [ ] Security policy review
- [ ] Cost optimization (right-size resources)
- [ ] Upgrade path planning

### Quarterly
- [ ] Full disaster recovery test (restore from backup)
- [ ] Security audit
- [ ] Compliance verification
- [ ] Training for new operators

---

**Kapitel 30 von 30** | **Teil XI: Operations** | **~10.000 Wörter (+2000 neu)**
