# ThemisDB Operations Certification (TOC)

## Certification Overview

The **ThemisDB Operations Certification (TOC)** is an advanced certification that validates your expertise in deploying, managing, and maintaining ThemisDB in production environments. This certification demonstrates that you can ensure high availability, implement effective monitoring, perform backup and recovery, tune performance, and troubleshoot complex issues.

### Certification Details

- **Certification Code**: TOC
- **Level**: Advanced
- **Duration**: 120 minutes (exam) + capstone project
- **Question Count**: 30-35 questions + practical scenarios
- **Question Types**: Multiple choice, scenario-based, and hands-on labs
- **Passing Score**: 75% (23/30 minimum on exam + passing capstone)
- **Validity**: 2 years
- **Prerequisites**: ThemisDB Fundamentals Certification (TDF)
- **Exam Fee**: $250 USD
- **Retake Fee**: $125 USD
- **Language**: English

---

## Target Audience

This certification is ideal for:

- **Database Administrators (DBAs)** managing ThemisDB
- **DevOps Engineers** deploying and automating ThemisDB
- **Site Reliability Engineers (SREs)** ensuring uptime
- **System Administrators** maintaining database infrastructure
- **Infrastructure Engineers** designing database architecture
- **Cloud Engineers** managing cloud deployments
- **Operations Managers** overseeing database operations
- **Platform Engineers** building database platforms

---

## Prerequisites

### Required Certification
- **ThemisDB Fundamentals Certification (TDF)** - Must be current (not expired)

### Technical Prerequisites
- 6-12 months operational experience with databases
- Strong Linux/Unix system administration skills
- Experience with monitoring and alerting tools
- Understanding of networking and security
- Familiarity with scripting (Bash, Python)
- Basic understanding of cloud platforms (AWS, GCP, Azure)

### Recommended Experience
- Managed production database systems
- Performed backup and recovery operations
- Implemented high availability solutions
- Troubleshot database performance issues
- Used infrastructure-as-code tools

---

## Learning Objectives

Upon completing this certification, you will be able to:

### 1. Production Deployment (20%)
- Deploy ThemisDB on various platforms (bare metal, VM, containers, cloud)
- Implement infrastructure-as-code
- Configure production-ready settings
- Design scalable architecture
- Implement security hardening
- Automate deployment processes

### 2. Monitoring and Alerting (15%)
- Configure comprehensive monitoring
- Set up alerting for critical issues
- Use Prometheus and Grafana
- Monitor key performance metrics
- Implement log aggregation
- Create operational dashboards

### 3. Backup and Recovery (15%)
- Design backup strategies
- Implement automated backups
- Perform point-in-time recovery
- Test recovery procedures
- Plan disaster recovery
- Ensure business continuity

### 4. High Availability and Replication (20%)
- Configure replication
- Implement clustering
- Design failover strategies
- Ensure data consistency
- Handle split-brain scenarios
- Minimize downtime

### 5. Performance Tuning (15%)
- Analyze system performance
- Optimize database configuration
- Tune storage and memory
- Monitor resource utilization
- Identify bottlenecks
- Implement caching strategies

### 6. Troubleshooting (10%)
- Debug connection issues
- Diagnose performance problems
- Analyze logs and metrics
- Use diagnostic tools
- Resolve replication issues
- Handle corruption scenarios

### 7. Capacity Planning (5%)
- Forecast resource needs
- Plan for growth
- Optimize resource allocation
- Manage storage capacity
- Scale horizontally and vertically

---

## Production Deployment

### Bare Metal Deployment

#### Hardware Requirements
```
Minimum Production Specs:
- CPU: 8 cores (16 recommended)
- RAM: 32GB (64GB+ for large datasets)
- Storage: SSD/NVMe with at least 500GB
- Network: 1Gbps (10Gbps recommended)
- OS: Ubuntu 22.04 LTS, RHEL 8+, or equivalent

Recommended Configuration:
- CPU: 16+ cores with high clock speed
- RAM: 128GB+ ECC memory
- Storage: NVMe SSD RAID 10, 2TB+
- Network: 10Gbps with redundant NICs
- Separate disks for data, WAL, and backups
```

#### Installation Script
```bash
#!/bin/bash
# production-install.sh

set -e

# Variables
THEMISDB_VERSION="1.0.0"
INSTALL_DIR="/opt/themisdb"
DATA_DIR="/data/themisdb"
WAL_DIR="/wal/themisdb"
LOG_DIR="/var/log/themisdb"
BACKUP_DIR="/backup/themisdb"
USER="themisdb"
GROUP="themisdb"

# Create user and directories
useradd -r -s /bin/false $USER
mkdir -p $INSTALL_DIR $DATA_DIR $WAL_DIR $LOG_DIR $BACKUP_DIR

# Set permissions
chown -R $USER:$GROUP $DATA_DIR $WAL_DIR $LOG_DIR $BACKUP_DIR

# Download and install
wget https://download.themisdb.com/releases/$THEMISDB_VERSION/themisdb-linux.tar.gz
tar -xzf themisdb-linux.tar.gz -C $INSTALL_DIR --strip-components=1

# Configure systemd service
cat > /etc/systemd/system/themisdb.service <<EOF
[Unit]
Description=ThemisDB Database Server
After=network.target

[Service]
Type=simple
User=$USER
Group=$GROUP
ExecStart=$INSTALL_DIR/bin/themisdb-server --config /etc/themisdb/themisdb.conf
Restart=always
RestartSec=10
LimitNOFILE=65536
LimitNPROC=32768

[Install]
WantedBy=multi-user.target
EOF

# Enable and start
systemctl daemon-reload
systemctl enable themisdb
systemctl start themisdb

echo "ThemisDB installed successfully"
```

### Docker Deployment

#### Production Docker Compose
```yaml
version: '3.8'

services:
  themisdb:
    image: themisdb/themisdb:1.0.0
    container_name: themisdb-prod
    restart: always
    
    environment:
      THEMISDB_CACHE_SIZE: 8GB
      THEMISDB_MAX_CONNECTIONS: 1000
      THEMISDB_LOG_LEVEL: info
    
    ports:
      - "8529:8529"
    
    volumes:
      - themisdb-data:/var/lib/themisdb
      - themisdb-wal:/var/lib/themisdb/wal
      - themisdb-logs:/var/log/themisdb
      - ./config/themisdb.conf:/etc/themisdb/themisdb.conf:ro
    
    networks:
      - themisdb-net
    
    ulimits:
      nofile:
        soft: 65536
        hard: 65536
      nproc:
        soft: 32768
        hard: 32768
    
    logging:
      driver: "json-file"
      options:
        max-size: "100m"
        max-file: "10"
    
    healthcheck:
      test: ["CMD", "themisdb-client", "--server", "localhost:8529", "--command", "SELECT 1"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 40s

volumes:
  themisdb-data:
    driver: local
    driver_opts:
      type: none
      o: bind
      device: /data/themisdb
  
  themisdb-wal:
    driver: local
    driver_opts:
      type: none
      o: bind
      device: /wal/themisdb
  
  themisdb-logs:
    driver: local
    driver_opts:
      type: none
      o: bind
      device: /var/log/themisdb

networks:
  themisdb-net:
    driver: bridge
```

### Kubernetes Deployment

#### StatefulSet Configuration
```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: themisdb
  namespace: database
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
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values:
                - themisdb
            topologyKey: kubernetes.io/hostname
      
      containers:
      - name: themisdb
        image: themisdb/themisdb:1.0.0
        ports:
        - containerPort: 8529
          name: db
        
        env:
        - name: THEMISDB_CLUSTER_ENABLED
          value: "true"
        - name: THEMISDB_NODE_ID
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        
        resources:
          requests:
            cpu: "4"
            memory: 16Gi
          limits:
            cpu: "8"
            memory: 32Gi
        
        volumeMounts:
        - name: data
          mountPath: /var/lib/themisdb
        - name: config
          mountPath: /etc/themisdb
        
        livenessProbe:
          exec:
            command:
            - themisdb-client
            - --server
            - localhost:8529
            - --command
            - SELECT 1
          initialDelaySeconds: 60
          periodSeconds: 30
        
        readinessProbe:
          exec:
            command:
            - themisdb-health-check
          initialDelaySeconds: 30
          periodSeconds: 10
      
      volumes:
      - name: config
        configMap:
          name: themisdb-config
  
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 500Gi
```

### Terraform (AWS Example)

```hcl
# main.tf
resource "aws_instance" "themisdb" {
  count         = 3
  ami           = "ami-0c55b159cbfafe1f0"  # Ubuntu 22.04
  instance_type = "r5.2xlarge"  # 8 vCPU, 64GB RAM
  
  vpc_security_group_ids = [aws_security_group.themisdb.id]
  subnet_id              = element(aws_subnet.private.*.id, count.index)
  
  iam_instance_profile = aws_iam_instance_profile.themisdb.name
  
  root_block_device {
    volume_type = "gp3"
    volume_size = 100
    iops        = 3000
  }
  
  ebs_block_device {
    device_name = "/dev/sdf"
    volume_type = "io2"
    volume_size = 1000
    iops        = 10000
  }
  
  user_data = templatefile("${path.module}/install-themisdb.sh", {
    node_id = count.index
    cluster_nodes = aws_instance.themisdb.*.private_ip
  })
  
  tags = {
    Name = "themisdb-node-${count.index}"
    Environment = "production"
    ManagedBy = "terraform"
  }
}

resource "aws_security_group" "themisdb" {
  name        = "themisdb-sg"
  description = "Security group for ThemisDB"
  vpc_id      = aws_vpc.main.id
  
  ingress {
    from_port   = 8529
    to_port     = 8529
    protocol    = "tcp"
    cidr_blocks = [var.application_cidr]
  }
  
  ingress {
    from_port = 8530
    to_port   = 8530
    protocol  = "tcp"
    self      = true  # Inter-node communication
  }
  
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}
```

---

## Monitoring and Alerting

### Prometheus Configuration

#### Metrics Exporter
```yaml
# prometheus.yml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'themisdb'
    static_configs:
      - targets: 
        - 'themisdb-1:9100'
        - 'themisdb-2:9100'
        - 'themisdb-3:9100'
    
    metrics_path: '/metrics'
    
    relabel_configs:
      - source_labels: [__address__]
        target_label: instance
```

#### Key Metrics to Monitor
```
# Performance Metrics
themisdb_query_duration_seconds - Query execution time
themisdb_queries_per_second - Query throughput
themisdb_connections_active - Active connections
themisdb_connections_total - Total connections

# Resource Metrics
themisdb_memory_used_bytes - Memory utilization
themisdb_cache_hit_ratio - Cache effectiveness
themisdb_disk_usage_bytes - Storage utilization
themisdb_disk_io_operations - I/O operations per second

# Replication Metrics
themisdb_replication_lag_seconds - Replication delay
themisdb_replication_errors_total - Replication failures
themisdb_cluster_nodes_active - Available cluster nodes

# Transaction Metrics
themisdb_transactions_active - In-flight transactions
themisdb_transactions_committed - Successful commits
themisdb_transactions_aborted - Rollbacks
themisdb_deadlocks_total - Deadlock occurrences
```

### Grafana Dashboard

```json
{
  "dashboard": {
    "title": "ThemisDB Operations",
    "panels": [
      {
        "title": "Query Performance",
        "targets": [
          {
            "expr": "rate(themisdb_query_duration_seconds_sum[5m]) / rate(themisdb_query_duration_seconds_count[5m])",
            "legendFormat": "Avg Query Time"
          }
        ]
      },
      {
        "title": "Throughput",
        "targets": [
          {
            "expr": "rate(themisdb_queries_total[1m])",
            "legendFormat": "Queries/sec"
          }
        ]
      },
      {
        "title": "Memory Usage",
        "targets": [
          {
            "expr": "themisdb_memory_used_bytes / themisdb_memory_total_bytes * 100",
            "legendFormat": "Memory %"
          }
        ]
      },
      {
        "title": "Replication Lag",
        "targets": [
          {
            "expr": "themisdb_replication_lag_seconds",
            "legendFormat": "Lag (seconds)"
          }
        ]
      }
    ]
  }
}
```

### Alerting Rules

```yaml
# alerts.yml
groups:
  - name: themisdb
    interval: 30s
    rules:
      # High Query Latency
      - alert: HighQueryLatency
        expr: themisdb_query_duration_seconds > 5
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High query latency detected"
          description: "Query latency is {{ $value }}s on {{ $labels.instance }}"
      
      # Low Cache Hit Ratio
      - alert: LowCacheHitRatio
        expr: themisdb_cache_hit_ratio < 0.8
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Low cache hit ratio"
          description: "Cache hit ratio is {{ $value }} on {{ $labels.instance }}"
      
      # Replication Lag
      - alert: HighReplicationLag
        expr: themisdb_replication_lag_seconds > 60
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "High replication lag"
          description: "Replication lag is {{ $value }}s on {{ $labels.instance }}"
      
      # Disk Space
      - alert: LowDiskSpace
        expr: (themisdb_disk_free_bytes / themisdb_disk_total_bytes) < 0.1
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Low disk space"
          description: "Only {{ $value | humanizePercentage }} disk space free on {{ $labels.instance }}"
      
      # Connection Saturation
      - alert: ConnectionSaturation
        expr: themisdb_connections_active / themisdb_connections_max > 0.9
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "Connection pool near capacity"
          description: "{{ $value | humanizePercentage }} of connections in use on {{ $labels.instance }}"
      
      # Node Down
      - alert: NodeDown
        expr: up{job="themisdb"} == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "ThemisDB node is down"
          description: "Node {{ $labels.instance }} is unreachable"
```

### Log Aggregation

```yaml
# filebeat.yml
filebeat.inputs:
  - type: log
    enabled: true
    paths:
      - /var/log/themisdb/*.log
    
    fields:
      service: themisdb
      environment: production
    
    multiline.pattern: '^[0-9]{4}-[0-9]{2}-[0-9]{2}'
    multiline.negate: true
    multiline.match: after

output.elasticsearch:
  hosts: ["elasticsearch:9200"]
  index: "themisdb-logs-%{+yyyy.MM.dd}"

processors:
  - add_host_metadata: ~
  - add_docker_metadata: ~
```

---

## Backup and Recovery

### Backup Strategy

#### Full Backup
```bash
#!/bin/bash
# full-backup.sh

BACKUP_DIR="/backup/themisdb"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_NAME="themisdb_full_${TIMESTAMP}"

# Create backup
themisdb-backup create \
  --type full \
  --output "${BACKUP_DIR}/${BACKUP_NAME}" \
  --compress gzip \
  --checksum sha256

# Upload to S3
aws s3 cp "${BACKUP_DIR}/${BACKUP_NAME}.tar.gz" \
  "s3://company-backups/themisdb/full/${BACKUP_NAME}.tar.gz" \
  --storage-class GLACIER

# Cleanup old local backups (keep 7 days)
find "${BACKUP_DIR}" -name "themisdb_full_*" -mtime +7 -delete

# Log completion
echo "$(date): Full backup completed: ${BACKUP_NAME}" >> /var/log/themisdb/backup.log
```

#### Incremental Backup
```bash
#!/bin/bash
# incremental-backup.sh

BACKUP_DIR="/backup/themisdb"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_NAME="themisdb_incr_${TIMESTAMP}"
LAST_FULL=$(ls -t ${BACKUP_DIR}/themisdb_full_* | head -1)

# Create incremental backup
themisdb-backup create \
  --type incremental \
  --base-backup "${LAST_FULL}" \
  --output "${BACKUP_DIR}/${BACKUP_NAME}" \
  --compress gzip

# Upload to S3
aws s3 cp "${BACKUP_DIR}/${BACKUP_NAME}.tar.gz" \
  "s3://company-backups/themisdb/incremental/${BACKUP_NAME}.tar.gz"

# Log completion
echo "$(date): Incremental backup completed: ${BACKUP_NAME}" >> /var/log/themisdb/backup.log
```

#### Point-in-Time Recovery (PITR)
```bash
#!/bin/bash
# pitr-backup.sh

WAL_DIR="/var/lib/themisdb/wal"
ARCHIVE_DIR="/backup/themisdb/wal-archive"

# Archive WAL files continuously
themisdb-wal-archiver \
  --source "${WAL_DIR}" \
  --destination "${ARCHIVE_DIR}" \
  --s3-bucket "company-backups/themisdb/wal" \
  --retention-days 30 \
  --daemon
```

### Recovery Procedures

#### Full Restore
```bash
#!/bin/bash
# restore.sh

BACKUP_FILE="$1"
RESTORE_DIR="/var/lib/themisdb"
TIMESTAMP="$2"  # Optional: for PITR

# Stop database
systemctl stop themisdb

# Clear existing data
rm -rf "${RESTORE_DIR}"/*

# Restore backup
themisdb-backup restore \
  --input "${BACKUP_FILE}" \
  --output "${RESTORE_DIR}" \
  ${TIMESTAMP:+--point-in-time "$TIMESTAMP"}

# Fix permissions
chown -R themisdb:themisdb "${RESTORE_DIR}"

# Start database
systemctl start themisdb

# Verify
themisdb-client --command "SELECT COUNT(*) FROM system.databases"

echo "Restore completed successfully"
```

#### Disaster Recovery Plan
```
1. Detection (5 minutes)
   - Automated monitoring alerts
   - Verify scope of incident
   - Activate DR team

2. Assessment (10 minutes)
   - Determine data loss extent
   - Identify recovery point
   - Select recovery strategy

3. Restoration (30-120 minutes)
   - Deploy standby infrastructure
   - Restore from backup
   - Apply WAL logs for PITR
   - Verify data integrity

4. Failover (15 minutes)
   - Update DNS/load balancer
   - Redirect application traffic
   - Monitor new primary

5. Verification (30 minutes)
   - Run data validation queries
   - Test application functionality
   - Verify replication

6. Post-Incident (24 hours)
   - Root cause analysis
   - Update runbooks
   - Improve monitoring
```

---

## High Availability and Replication

### Replication Configuration

#### Primary Node
```ini
# themisdb.conf (primary)
[replication]
enabled = true
role = primary
replication-factor = 3

[cluster]
cluster-id = prod-cluster
node-id = node-1
bind-address = 10.0.1.10:8530

peers = [
  "10.0.1.11:8530",
  "10.0.1.12:8530"
]
```

#### Replica Node
```ini
# themisdb.conf (replica)
[replication]
enabled = true
role = replica
primary-endpoint = tcp://10.0.1.10:8530
replication-lag-threshold = 10s

[cluster]
cluster-id = prod-cluster
node-id = node-2
bind-address = 10.0.1.11:8530
```

### Failover Strategies

#### Automatic Failover with Consul
```hcl
# consul-config.hcl
service {
  name = "themisdb-primary"
  port = 8529
  
  check {
    id = "themisdb-health"
    name = "ThemisDB Health Check"
    tcp = "localhost:8529"
    interval = "10s"
    timeout = "2s"
  }
  
  check {
    id = "themisdb-replication"
    name = "Replication Lag Check"
    args = ["/usr/local/bin/check-replication-lag.sh"]
    interval = "30s"
    timeout = "5s"
  }
}

# Failover script triggered by Consul
#!/bin/bash
# failover.sh

# Promote replica to primary
themisdb-admin promote-to-primary \
  --node node-2 \
  --force

# Update load balancer
aws elbv2 modify-target-group \
  --target-group-arn $TARGET_GROUP_ARN \
  --health-check-path /health \
  --targets Id=node-2

# Notify team
curl -X POST https://alerts.company.com/webhook \
  -d '{"message": "ThemisDB failover: node-2 promoted to primary"}'
```

### Split-Brain Prevention

```python
#!/usr/bin/env python3
# split-brain-detector.py

import consul
import time

def check_split_brain():
    c = consul.Consul()
    
    # Get all nodes claiming to be primary
    primaries = []
    for node in c.health.service('themisdb-primary')[1]:
        primaries.append(node['Node']['Node'])
    
    if len(primaries) > 1:
        # Split brain detected!
        alert_ops_team(f"Split brain detected: {primaries}")
        
        # Automatic resolution: keep node with most recent data
        resolve_split_brain(primaries)
    
    return len(primaries) == 1

def resolve_split_brain(nodes):
    # Query each node for last commit timestamp
    timestamps = {}
    for node in nodes:
        ts = query_last_commit(node)
        timestamps[node] = ts
    
    # Keep most recent, demote others
    winner = max(timestamps, key=timestamps.get)
    for node in nodes:
        if node != winner:
            demote_to_replica(node)

if __name__ == "__main__":
    while True:
        check_split_brain()
        time.sleep(30)
```

---

## Performance Tuning

### Configuration Optimization

```ini
# themisdb.conf - Performance Tuned

[server]
max-connections = 2000
connection-timeout = 300
keepalive-interval = 60

[cache]
size = 32GB
page-size = 16KB
eviction-policy = lru
prefetch-enabled = true

[query]
max-query-time = 300
parallel-execution = true
max-parallel-workers = 16
query-cache-enabled = true
query-cache-size = 2GB

[storage]
checkpoint-interval = 300
wal-buffer-size = 64MB
wal-sync-mode = fsync
compression = lz4
block-size = 8KB

[memory]
shared-buffers = 16GB
work-mem = 256MB
maintenance-work-mem = 2GB
effective-cache-size = 48GB

[io]
io-threads = 8
max-io-queue-depth = 128
direct-io = true
```

### OS-Level Tuning

```bash
#!/bin/bash
# system-tuning.sh

# Kernel parameters
cat >> /etc/sysctl.conf <<EOF
# Network tuning
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 67108864
net.ipv4.tcp_wmem = 4096 65536 67108864
net.core.netdev_max_backlog = 5000
net.ipv4.tcp_max_syn_backlog = 8192

# Memory management
vm.swappiness = 1
vm.dirty_ratio = 15
vm.dirty_background_ratio = 5
vm.overcommit_memory = 2

# File system
fs.file-max = 2097152
fs.aio-max-nr = 1048576
EOF

sysctl -p

# I/O scheduler (for SSD)
echo "noop" > /sys/block/nvme0n1/queue/scheduler

# Disable transparent huge pages
echo never > /sys/kernel/mm/transparent_hugepage/enabled
echo never > /sys/kernel/mm/transparent_hugepage/defrag

# Increase file descriptors
cat >> /etc/security/limits.conf <<EOF
themisdb soft nofile 65536
themisdb hard nofile 65536
themisdb soft nproc 32768
themisdb hard nproc 32768
EOF
```

### Query Performance Analysis

```sql
-- Enable query profiling
SET profiling = ON;

-- Run query
SELECT /*+ PROFILE */ ...

-- View profile
SHOW PROFILE FOR QUERY 1;

-- Analyze slow queries
SELECT 
    query_hash,
    COUNT(*) as execution_count,
    AVG(duration) as avg_duration,
    MAX(duration) as max_duration,
    SUM(rows_examined) as total_rows
FROM system.slow_query_log
WHERE duration > 1
GROUP BY query_hash
ORDER BY avg_duration DESC
LIMIT 20;
```

---

## Sample Exam Questions

### Section 1: Deployment

**Question 1**: What is the minimum recommended RAM for a production ThemisDB server?
- A) 8GB
- B) 16GB
- C) 32GB
- D) 64GB

**Answer**: C

---

**Question 2**: Which storage type is recommended for WAL files?
- A) HDD
- B) Network storage
- C) SSD/NVMe
- D) Any type is fine

**Answer**: C

---

**Question 3**: In Kubernetes, which workload type should you use for ThemisDB?
- A) Deployment
- B) StatefulSet
- C) DaemonSet
- D) Job

**Answer**: B

---

### Section 2: Monitoring

**Question 4**: What metric indicates poor cache performance?
- A) High CPU usage
- B) Low cache hit ratio
- C) High network traffic
- D) Low disk I/O

**Answer**: B

---

**Question 5**: At what replication lag should you trigger a critical alert?
- A) 1 second
- B) 10 seconds
- C) 60 seconds
- D) 300 seconds

**Answer**: C

---

### Section 3: Backup and Recovery

**Question 6**: How often should you test your disaster recovery procedures?
- A) Never
- B) Annually
- C) Quarterly
- D) After every backup

**Answer**: C

---

**Question 7**: What is the advantage of incremental backups over full backups?
- A) Faster recovery
- B) Smaller backup size and faster completion
- C) Better compression
- D) More reliable

**Answer**: B

---

**Question 8**: For point-in-time recovery, what must you archive?
- A) Data files only
- B) Configuration files
- C) WAL logs
- D) Indexes

**Answer**: C

---

### Section 4: High Availability

**Question 9**: What is split-brain in a cluster?
- A) A crashed node
- B) Multiple nodes thinking they're primary
- C) Network partition
- D) Corrupted data

**Answer**: B

---

**Question 10**: What is the recommended minimum number of nodes for a production cluster?
- A) 1
- B) 2
- C) 3
- D) 5

**Answer**: C (for quorum)

---

### Section 5: Performance

**Question 11**: Which kernel parameter controls memory swapping?
- A) vm.swappiness
- B) vm.memory
- C) kernel.swap
- D) mem.swap_ratio

**Answer**: A

---

**Question 12**: What is the recommended I/O scheduler for SSDs?
- A) cfq
- B) deadline
- C) noop
- D) anticipatory

**Answer**: C

---

### Scenario Questions

**Question 13**: Your monitoring shows replication lag increasing steadily. What should you check first?
- A) Network bandwidth between nodes
- B) Primary node load
- C) Replica node resources
- D) All of the above

**Answer**: D

---

**Question 14**: After a server crash, what's the first step in recovery?
- A) Restore from backup immediately
- B) Check logs to understand cause
- C) Replace hardware
- D) Notify management

**Answer**: B

---

**Question 15**: Your database is using 95% of available connections. What should you do?
- A) Restart the database
- B) Investigate connection leaks in applications
- C) Increase max_connections
- D) Both B and C

**Answer**: D

---

## Capstone Project

### Overview
Design and implement a production-ready ThemisDB deployment with full operational procedures.

### Requirements

**Part 1: Architecture Design (20%)**
- 3-node cluster design
- High availability configuration
- Network topology
- Security architecture
- Disaster recovery plan

**Part 2: Deployment (25%)**
- Infrastructure-as-code
- Automated deployment
- Configuration management
- Security hardening
- Documentation

**Part 3: Monitoring (20%)**
- Prometheus/Grafana setup
- Custom dashboards
- Alert rules
- Log aggregation
- Runbooks

**Part 4: Backup/Recovery (20%)**
- Automated backup system
- Restore procedures
- PITR implementation
- DR testing
- Documentation

**Part 5: Operations Manual (15%)**
- Standard operating procedures
- Troubleshooting guides
- Escalation procedures
- Capacity planning
- On-call runbook

### Deliverables
1. Complete infrastructure code
2. Deployment documentation
3. Monitoring dashboards
4. Operations manual
5. 15-minute presentation

### Evaluation
- All components must be functional
- Must pass simulated failure scenarios
- Documentation must be comprehensive
- Code must follow best practices

---

## Certification Benefits

- Advanced DBA recognition
- Operations specialist designation
- 25% average salary increase
- Leadership opportunities
- Expert community access
- Speaking engagements

---

## Support

**Operations Support**: ops-cert@themisdb.com  
**Technical Help**: dba-support@themisdb.com  
**Project Questions**: capstone@themisdb.com

---

[Register for TOC Certification →](https://certify.themisdb.com/register/toc)

---

*Last Updated: April 2026*  
*Version: 1.0*  
*© 2025 ThemisDB. All rights reserved.*
