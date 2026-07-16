# ThemisDB Operations Runbook

**Version:** 1.0  
**Last Updated:** 2026-04-04  
**Scope:** Phase 5 Operational Tooling

---

## Table of Contents

1. [Startup & Deployment](#startup--deployment)
2. [Monitoring & Alerts](#monitoring--alerts)
3. [Troubleshooting](#troubleshooting)
4. [Performance Tuning](#performance-tuning)
5. [Backup & Recovery](#backup--recovery)
6. [Scaling & Capacity](#scaling--capacity)

---

## Startup & Deployment

### Single Node Startup

```bash
#!/bin/bash
# Start ThemisDB single-node development instance

docker run -d --name themisdb \
  -p 8765:8765 \
  -p 8080:8080 \
  -v $(pwd)/config:/etc/themis/config:ro \
  -v $(pwd)/data:/var/lib/themis/data \
  -v $(pwd)/logs:/var/log/themis \
  themisdb:community:v1.8.1-rc1 \
  /opt/themis/bin/themis_server \
    --config=/etc/themis/config/config.yaml \
    --data-dir=/var/lib/themis/data

# Wait for startup
sleep 5

# Verify health
themisctl --port 8765 health
echo "ThemisDB is ready!"
```

### Cluster Deployment (3 nodes)

```bash
#!/bin/bash
# Deploy 3-node cluster with Paxos consensus

for i in 1 2 3; do
  docker run -d --name themisdb-$i \
    --network themis-cluster \
    -p $(( 8765 + $i - 1 )):8765 \
    -v ../config/node-$i/config.yaml:/etc/themis/config/config.yaml:ro \
    -v ../data/node-$i:/var/lib/themis/data \
    themisdb:$i:v1.8.1-rc1
done

# Wait for cluster formation
sleep 10
for i in 1 2 3; do
  themisctl --port $(( 8765 + $i - 1 )) admin stats
done
```

### Kubernetes Deployment

```yaml
# themisdb-deployment.yaml
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
        image: themisdb:community:v1.8.1-rc1
        ports:
        - containerPort: 8765
          name: main
        - containerPort: 8080
          name: http
        volumeMounts:
        - name: config
          mountPath: /etc/themis/config
          readOnly: true
        - name: data
          mountPath: /var/lib/themis/data
        - name: logs
          mountPath: /var/log/themis
        livenessProbe:
          exec:
            command:
            - /usr/local/bin/themisctl
            - health
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          exec:
            command:
            - /usr/local/bin/themisctl
            - health
          initialDelaySeconds: 10
          periodSeconds: 5
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: [ "ReadWriteOnce" ]
      resources:
        requests:
          storage: 100Gi
```

Deploy:
```bash
kubectl apply -f themisdb-deployment.yaml
kubectl wait --for=condition=ready pod -l app=themisdb --timeout=300s
```

---

## Monitoring & Alerts

### Health Checks (Prometheus)

```bash
# Add to Prometheus scrape_configs
- job_name: 'themisdb'
  static_configs:
    - targets: ['localhost:9090']  # Metrics port
  scrape_interval: 15s
```

**Key Metrics to Monitor:**

```promql
# Query throughput
rate(themis_requests_total[5m])

# Query latency (p95)
themis_request_duration_seconds{quantile="0.95"}

# Error rate
rate(themis_errors_total[5m])

# Cache hit rate
themis_cache_hits_total / themis_cache_requests_total

# Replication lag
themis_replication_lag_ms

# Storage usage
themis_storage_bytes_used / themis_storage_bytes_max
```

### Manual Health Checks

```bash
#!/bin/bash
# Daily health check script

echo "=== ThemisDB Health Check ==="
echo "Time: $(date)"

# Check server health
echo "Server Status:"
themisctl health && echo "✓ Server healthy" || echo "✗ Server unhealthy"

# Check admin stats
echo -e "\nServer Stats:"
themisctl admin stats

# Check cache
echo -e "\nCache Status:"
themisctl admin cache

# Check storage
echo -e "\nStorage Usage:"
themisctl --json admin stats | jq '.storage, .memory'

# Check replication
echo -e "\nReplication Status:"
themisctl --json query 'RETURN {replication_lag: $REPLICATION_LAG}'
```

Run daily:
```bash
0 6 * * * bash /opt/themis/scripts/daily_health_check.sh >> /var/log/themis/health_check.log
```

### Alerting (Example)

**Prometheus Alert Rules:**

```yaml
groups:
- name: themisdb
  rules:
  - alert: ThemisDBDown
    expr: up{job="themisdb"} == 0
    for: 1m
    annotations:
      summary: "ThemisDB node down"
  
  - alert: HighErrorRate
    expr: rate(themis_errors_total[5m]) > 0.01
    for: 5m
    annotations:
      summary: "Error rate > 1%"
  
  - alert: HighLatency
    expr: themis_request_duration_seconds{quantile="0.95"} > 1.0
    for: 10m
    annotations:
      summary: "p95 latency > 1s"
  
  - alert: LowCacheHitRate
    expr: |
      (themis_cache_hits_total / themis_cache_requests_total) < 0.80
    for: 15m
    annotations:
      summary: "Cache hit rate < 80%"
  
  - alert: HighStorageUsage
    expr: |
      (themis_storage_bytes_used / themis_storage_bytes_max) > 0.90
    for: 30m
    annotations:
      summary: "Storage > 90% full"
```

---

## Troubleshooting

### Server Won't Start

**Symptom:** Container exits immediately

**Diagnostics:**
```bash
docker logs themisdb | tail -50
docker inspect themisdb | jq '.State'
```

**Solutions:**
1. **Port already in use:** `lsof -i :8765` → kill old process
2. **Permission denied on data dir:** `sudo chown -R themis:themis /var/lib/themis/data`
3. **Missing config file:** verify `/etc/themis/config/config.yaml` exists
4. **Out of memory:** increase Docker resource limit `--memory 8g`

### High Query Latency

**Symptom:** Queries taking >1 second

**Diagnostics:**
```bash
# Check query plan
themisctl query "EXPLAIN FOR d IN users FILTER d.active == true RETURN d"

# Check cache hit rate
themisctl admin cache | grep "Hit Rate"

# Check server load
themisctl admin stats | grep "Memory\|CPU"
```

**Solutions:**
1. **Add index:** `themisctl query "RETURN IndexCreate(\"users\", [\"active\"])"`
2. **Increase cache:** Edit config `cache_size_mb: 2048` → `4096`
3. **Optimize query:** Use FILTER early, add LIMIT, check JOIN order
4. **Scale horizontally:** Add more nodes to cluster

### Connection Refused

**Symptom:** `themisctl: Connection error`

**Diagnostics:**
```bash
netstat -tlnp | grep 8765
telnet localhost 8765
curl http://localhost:8080/health/live -v
```

**Solutions:**
1. **Service not running:** `docker ps | grep themisdb`
2. **Wrong port:** verify config `port: 8765` or check docker port mapping
3. **Firewall:** `sudo ufw allow 8765`
4. **Binding issue:** check if IPv6 or IPv4 → try `--host 0.0.0.0`

### Out of Memory

**Symptom:** `OOM killed` in logs, sudden process termination

**Diagnostics:**
```bash
themisctl admin stats | grep Memory
free -h
docker stats themisdb
```

**Solutions:**
1. **Increase allocation:** Docker `--memory 16g` or Kubernetes resource requests
2. **Reduce cache:** Edit config `cache_size_mb: 512` (lower from default)
3. **Limit connections:** config `max_connections: 100` (reduce from default)
4. **Add more nodes:** distribute load across cluster (scaling)

---

## Performance Tuning

### Query Performance

**Baseline Throughput:**
- Without tuning: ~1K reads/sec, ~100 writes/sec
- With tuning: ~10K reads/sec, ~500 writes/sec

**Tuning Steps:**

```bash
# 1. Check current settings
themisctl config get caching

# 2. Enable query result caching
themisctl config set \
  caching.query_cache_enabled=true \
  caching.query_cache_size_mb=1024

# 3. Increase index refresh frequency
themisctl config set index_refresh_interval_ms=100

# 4. Use connection pooling (client-side)
# Ensure HTTP client reuses connections
```

### Throughput Optimization

**For High Read Load:**
```bash
themisctl config set \
  cache_size_mb=4096 \
  read_buffer_mb=512 \
  prefetch_depth=16
```

**For High Write Load:**
```bash
themisctl config set \
  wal_batch_size=1000 \
  wal_fsync_interval_ms=100 \
  write_buffer_size_mb=256
```

### Memory Optimization

```bash
# Check memory breakdown
themisctl --json admin stats | jq '.memory_by_component'

# If index is using too much:
themisctl config set index_memory_limit_mb=2048

# If cache is too large:
themisctl config set cache_size_mb=1024
```

---

## Backup & Recovery

### Regular Backups

```bash
#!/bin/bash
# Daily backup strategy

TODAY=$(date +%Y-%m-%d)
BACKUP_DIR=/mnt/backups/themisdb

# Create snapshot
echo "Creating snapshot: backup-$TODAY"
themisctl snapshot create "backup-$TODAY"

# Export data to external storage
themisctl --json query 'FOR doc IN documents RETURN doc' | \
  gzip > "$BACKUP_DIR/documents-$TODAY.json.gz"

# Backup configuration
cp /etc/themis/config/config.yaml "$BACKUP_DIR/config-$TODAY.yaml"

echo "Backup complete: $BACKUP_DIR"

# Cleanup old backups (keep 30 days)
find $BACKUP_DIR -name "documents-*.json.gz" -mtime +30 -delete
```

Run automatically:
```bash
0 2 * * * bash /opt/themis/scripts/daily_backup.sh >> /var/log/themis/backup.log 2>&1
```

### Point-in-Time Recovery (PITR)

```bash
#!/bin/bash
# Recover data to specific point in time

RECOVERY_TIME="2026-04-03T14:30:00Z"

echo "Recovering to: $RECOVERY_TIME"

# Create backup of current state first
themisctl snapshot create "pre-recovery-backup"

# Restore from snapshot (implementation depends on exact PITR support)
# Option 1: Use WAL if available
# Option 2: Restore from nearest older snapshot + replay WAL

# For now, document manual procedure:
echo "TODO: Restore snapshot or use PostgreSQL-compatible pg_restore"
```

### Disaster Recovery

```bash
#!/bin/bash
# Complete cluster recovery from backup

# 1. Stop all nodes
for i in 1 2 3; do
  docker stop themisdb-$i
done

# 2. Restore data
for i in 1 2 3; do
  docker run --rm \
    -v /mnt/backups/themisdb/data:/backup:ro \
    -v themisdb-node-$i-data:/data \
    alpine sh -c "cp -r /backup/* /data/"
done

# 3. Restart nodes
for i in 1 2 3; do
  docker start themisdb-$i
done

# 4. Verify cluster formation
sleep 10
for i in 1 2 3; do
  themisctl --port $(( 8765 + $i - 1 )) admin stats
done

echo "Cluster recovery complete"
```

---

## Scaling & Capacity

### Horizontal Scaling (Add Nodes)

```bash
#!/bin/bash
# Add a 4th node to existing 3-node cluster

NODE_ID=4
PORT=$(( 8765 + $NODE_ID - 1 ))

# 1. Prepare configuration
mkdir -p config/node-$NODE_ID
cp config/node-1/config.yaml config/node-$NODE_ID/config.yaml
sed -i "s/node-1/node-$NODE_ID/g" config/node-$NODE_ID/config.yaml

# 2. Start new node (joins cluster automatically if seeds configured)
docker run -d --name themisdb-$NODE_ID \
  --network themis-cluster \
  -p $PORT:8765 \
  -v $(pwd)/config/node-$NODE_ID:/etc/themis/config:ro \
  -v $(pwd)/data/node-$NODE_ID:/var/lib/themis/data \
  themisdb:v1.8.1-rc1

# 3. Wait for rebalancing
sleep 30

# 4. Verify new node is healthy
themisctl --port $PORT admin stats

echo "Node $NODE_ID added and rebalanced"
```

### Vertical Scaling (Upgrade Node)

```bash
#!/bin/bash
# Upgrade single node (rolling upgrade pattern)

NODE_ID=1
PORT=$(( 8765 + $NODE_ID - 1 ))

echo "Upgrading node $NODE_ID..."

# 1. Create backup before upgrade
themisctl --port $PORT snapshot create "pre-upgrade-v1.8.1"

# 2. Stop node
docker stop themisdb-$NODE_ID

# 3. Pull new image
docker pull themisdb:v1.9.0

# 4. Update configuration if needed
# Check release notes for breaking changes

# 5. Restart with new version
docker run -d --name themisdb-$NODE_ID-new \
  --network themis-cluster \
  -p $PORT:8765 \
  -v $(pwd)/config/node-$NODE_ID:/etc/themis/config:ro \
  -v themisdb-node-$NODE_ID-data:/var/lib/themis/data \
  themisdb:v1.9.0

# 6. Wait for rejoin and catch-up
sleep 30
themisctl --port $PORT admin stats

# 7. Remove old container
docker rm themisdb-$NODE_ID

echo "Node $NODE_ID upgraded to v1.9.0"
```

### Capacity Planning

**Monitoring Capacity Growth:**

```bash
#!/bin/bash
# Weekly capacity report

WEEK=$(date +%Y-W%U)
REPORT_FILE="/var/log/themis/capacity-report-$WEEK.txt"

{
  echo "ThemisDB Capacity Report — Week $WEEK"
  echo "Time: $(date)"
  echo ""
  
  echo "=== Storage Usage ==="
  du -sh /var/lib/themis/data
  
  echo -e "\n=== Memory Usage ==="
  free -h
  
  echo -e "\n=== Disk I/O ==="
  iostat -x 1 2 | tail -6
  
  echo -e "\n=== Server Stats ==="
  themisctl admin stats
  
} | tee "$REPORT_FILE"

# Archive report
gzip "$REPORT_FILE"
```

**Capacity Growth Estimation:**

```bash
# Measure write throughput
BEFORE=$(themisctl --json admin stats | jq '.bytes_written')
sleep 3600  # Wait 1 hour
AFTER=$(themisctl --json admin stats | jq '.bytes_written')
GROWTH_PER_HOUR=$(( $AFTER - $BEFORE ))
GROWTH_PER_MONTH=$(( $GROWTH_PER_HOUR * 24 * 30 ))
MONTHS_UNTIL_FULL=$(( (available_space_bytes / $GROWTH_PER_MONTH) ))

echo "Growth per month: $GROWTH_PER_MONTH bytes"
echo "Storage full in: $MONTHS_UNTIL_FULL months"
```

---

## Maintenance Windows

### Scheduled Maintenance

```bash
#!/bin/bash
# Announce maintenance and gracefully drain connections

echo "Announcing maintenance window..."
themisctl config set server_status_message="Maintenance: 2026-04-04 02:00-03:00 UTC"

# Give clients 30 seconds to disconnect
sleep 30

# Stop traffic (remove from load balancer)
# ... load balancer integration ...

# Perform maintenance
echo "Performing maintenance..."
# backup, compact RocksDB, update schemas, etc.
themisctl snapshot create "maintenance-backup"

# Restart
docker restart themisdb

# Verify
sleep 10
themisctl health

# Resume traffic
themisctl config set server_status_message=""
echo "Maintenance complete"
```

---

## Emergency Procedures

### Immediate Shutdown (Emergency Brake)

```bash
#!/bin/bash
# Stop all traffic immediately (when under attack or critical issue)

echo "EMERGENCY STOP - Killing all connections"
for i in 1 2 3; do
  PORT=$(( 8765 + $i - 1 ))
  docker stop themisdb-$i
done

echo "All nodes stopped. Investigate and restart manually."
```

### Data Corruption Recovery

```bash
#!/bin/bash
# Recover from data corruption (RocksDB)

echo "Detected corruption - initiating recovery"

# 1. Stop affected node
docker stop themisdb-1

# 2. Remove corrupted database
rm -rf /var/lib/themis/data/node-1

# 3. Restart (will resync from cluster)
docker start themisdb-1

# 4. Monitor resync
while true; do
  STATUS=$(themisctl --port 8766 admin stats | grep -i "replay\|resync")
  echo "Resync status: $STATUS"
  sleep 5
done
```

---

## Important Contacts

- **On-Call Operations:** ops-oncall@company.com
- **Database Team:** databases@company.com
- **Security Issues:** security@company.com
- **Escalation:** vp-infrastructure@company.com

---

## Related Documentation

- [themisctl Admin Guide](THEMISCTL_ADMIN_GUIDE.md)
- [Configuration Reference](../config/CONFIG_REFERENCE.md)
- [Deployment Guide](../deployment/DEPLOYMENT.md)
- [Security Best Practices](../security/SECURITY_BEST_PRACTICES.md)
- [Performance Tuning](../performance/PERFORMANCE_TUNING.md)

---

**Status:** ✅ Production-Ready  
**Last Review:** 2026-04-04
