# ThemisDB Operational Procedures

**Version:** 1.4.0  
**Last Updated:** 2026-04-06  
**Target Audience:** Site Reliability Engineers, Database Administrators, Operations Teams

---

## Table of Contents

1. [Daily Operations](#daily-operations)
2. [Backup & Recovery Procedures](#backup--recovery-procedures)
3. [Scaling Operations](#scaling-operations)
4. [Maintenance Windows](#maintenance-windows)
5. [Troubleshooting Procedures](#troubleshooting-procedures)
6. [Emergency Procedures](#emergency-procedures)

---

## Daily Operations

### Health Monitoring Checklist

**Daily Health Check Script:**

```bash
#!/bin/bash
# daily_health_check.sh

echo "=== ThemisDB Daily Health Check ==="
echo "Date: $(date)"

# 1. Service status
systemctl status themisdb | grep "Active:"

# 2. Database connectivity
curl -s http://localhost:9090/health | jq '.status'

# 3. Disk space
df -h /var/lib/themisdb | tail -1

# 4. Memory usage
free -h | grep Mem

# 5. Transaction metrics
curl -s http://localhost:9091/metrics | grep transaction_active

# 6. Replication lag
curl -s http://localhost:9091/metrics | grep replication_lag
```

### Log Review Procedures

**Daily Log Analysis:**

```bash
# Check error counts
grep "ERROR" /var/log/themisdb/themisdb.log | wc -l

# Review slow queries
tail -100 /var/log/themisdb/slow_queries.log

# Check authentication failures
grep "Authentication failed" /var/log/themisdb/audit.log
```

---

## Backup & Recovery Procedures

### Full Backup Procedure

```bash
# Create full backup
BACKUP_DIR="/mnt/backup/themisdb/full-$(date +%Y%m%d)"

themisdb-admin backup create \
  --type=full \
  --destination=$BACKUP_DIR \
  --compression=zstd \
  --verify

# Verify backup
themisdb-admin backup verify --path=$BACKUP_DIR
```

### Recovery from Backup

```bash
# Stop service
sudo systemctl stop themisdb

# Restore backup
themisdb-admin backup restore \
  --source=/mnt/backup/themisdb/full-20260118 \
  --destination=/var/lib/themisdb/data \
  --verify

# Start service
sudo systemctl start themisdb
```

---

## Scaling Operations

### Adding New Nodes

```bash
# On new node, configure cluster
cat > /opt/themisdb/config/node.yaml <<EOF
cluster:
  node_id: "node4"
  node_address: "192.168.1.13:7700"
EOF

# Add to cluster
themisdb-admin cluster add-node \
  --node-id=node4 \
  --address=192.168.1.13:7700
```

---

## Maintenance Windows

### Zero-Downtime Update

```bash
# Rolling update script
for node in node1 node2 node3; do
    ssh $node "sudo systemctl stop themisdb"
    ssh $node "sudo dpkg -i themisdb_1.4.1.deb"
    ssh $node "sudo systemctl start themisdb"
    sleep 60
done
```

---

## Troubleshooting Procedures

### High CPU Diagnosis

```bash
# Check CPU usage breakdown
themisdb-admin stats cpu

# Identify slow queries
themisdb-admin query analyze --top=10
```

### Memory Pressure

```bash
# Memory breakdown
themisdb-admin stats memory

# Kill long-running transactions
themisdb-admin transaction kill <txn_id>
```

---

## Emergency Procedures

### Emergency Shutdown

```bash
# Enable read-only mode
themisdb-admin maintenance --read-only

# Stop service
sudo systemctl stop themisdb
```

### Data Corruption Recovery

```bash
# Check for corruption
themisdb-admin db check

# Repair if possible
themisdb-admin db repair

# Otherwise restore from backup
themisdb-admin backup restore --source=/mnt/backup/latest
```

---

## Related Documentation

- [Production Deployment Guide](../deployment/PRODUCTION_DEPLOYMENT_GUIDE.md)
- [Monitoring Setup Guide](MONITORING_SETUP_GUIDE.md)
- [Troubleshooting Guide](TROUBLESHOOTING_GUIDE.md)

---

**Document Version:** 1.0  
**ThemisDB Compatibility:** 1.4.0+  
**Last Reviewed:** 2026-01-18
