# ThemisDB Troubleshooting Guide

Complete troubleshooting reference for ThemisDB database administrators and developers.

## Table of Contents

- [Common Issues and Solutions](#common-issues-and-solutions)
- [Connection Problems](#connection-problems)
- [Performance Issues](#performance-issues)
- [Memory Problems](#memory-problems)
- [Crash Scenarios](#crash-scenarios)
- [Data Corruption Recovery](#data-corruption-recovery)
- [Log Analysis Guide](#log-analysis-guide)
- [Diagnostic Commands](#diagnostic-commands)
- [When to File a Bug](#when-to-file-a-bug)

---

## Common Issues and Solutions

### Database Won't Start

**Symptoms:**
- Server fails to initialize
- Port binding errors
- Configuration validation failures

**Solutions:**

```bash
# Check if port is already in use
netstat -tulpn | grep 8529

# Verify configuration syntax
themisdb-server --validate-config /etc/themisdb/themisdb.conf

# Check file permissions
ls -la /var/lib/themisdb/
chown -R themisdb:themisdb /var/lib/themisdb/

# Review startup logs
journalctl -u themisdb -n 100 --no-pager
```

**⚠️ Warning:** Always backup data directory before changing permissions.

---

### Query Timeout Errors

**Symptoms:**
```
Error: Query execution timeout after 30s
Code: THEMIS_E_TIMEOUT
```

**Solutions:**

1. **Increase timeout temporarily:**
```aql
-- For specific query
OPTIONS { timeout: 120000 }
FOR doc IN collection
  FILTER doc.large_field != null
  RETURN doc
```

2. **Add appropriate indexes:**
```aql
-- Create index on filter field
db._collection("collection").ensureIndex({
  type: "persistent",
  fields: ["large_field"]
});
```

3. **Optimize query structure:**
```aql
-- BAD: Full collection scan
FOR doc IN collection
  FILTER doc.status == "active" AND doc.type == "premium"
  RETURN doc

-- GOOD: Use indexed fields first
FOR doc IN collection
  FILTER doc.type == "premium"  // Indexed
  FILTER doc.status == "active"  // Indexed
  RETURN doc
```

**💡 Pro Tip:** Use `EXPLAIN` to analyze query execution plans before optimization.

---

### Slow Query Performance

**Diagnosis Steps:**

```aql
-- Enable query profiling
db._query({
  query: "FOR doc IN collection RETURN doc",
  options: { profile: 2 }
});

-- Check execution statistics
db._queryCache.properties();
```

**Common Causes & Fixes:**

| Issue | Detection | Solution |
|-------|-----------|----------|
| Missing Index | `rules: ["all"]` in explain | Add persistent index |
| Large Result Set | High `httpRequests` | Add LIMIT clause |
| Complex Joins | Multiple `EnumerateCollectionNode` | Denormalize data or use graph |
| Full Collection Scan | `estimatedCost > 1000000` | Add filter conditions |

---

## Connection Problems

### Cannot Connect to Database

**Check Connection String:**
```bash
# Test TCP connectivity
telnet localhost 8529

# Test HTTP endpoint
curl -X GET http://localhost:8529/_api/version

# Test with authentication
curl -u username:password http://localhost:8529/_db/_system/_api/version
```

**Common Issues:**

1. **Wrong Credentials:**
```javascript
// Correct connection format
const db = new Database({
  url: 'http://localhost:8529',
  databaseName: 'mydb',
  auth: { username: 'root', password: 'password' }
});
```

2. **SSL/TLS Issues:**
```bash
# Verify certificate
openssl s_client -connect localhost:8530 -showcerts

# Test with certificate validation disabled (testing only!)
curl -k https://localhost:8530/_api/version
```

3. **Firewall Blocking:**
```bash
# Check firewall rules
sudo iptables -L -n | grep 8529

# Allow ThemisDB port
sudo ufw allow 8529/tcp
```

---

### Connection Pool Exhaustion

**Symptoms:**
```
Error: No available connections in pool
Active connections: 100/100
```

**Solutions:**

```yaml
# themisdb.conf - Increase pool size
network:
  maxConnections: 500
  connectionTimeout: 30000

# Application side - Configure pool
const pool = {
  min: 10,
  max: 100,
  acquireTimeoutMillis: 30000,
  idleTimeoutMillis: 30000
};
```

**Monitor Pool Health:**
```bash
# Check active connections
themisdb-admin show connections --database mydb

# View connection statistics
curl http://localhost:8529/_admin/statistics
```

---

### Intermittent Connection Drops

**Diagnosis:**

```bash
# Check for network issues
ping -c 100 database-server

# Monitor connection stability
watch -n 1 'netstat -an | grep 8529 | wc -l'

# Review system logs
dmesg | grep -i "network\|tcp"
```

**Solutions:**

1. **Configure keepalive:**
```yaml
# themisdb.conf
network:
  keepaliveTimeout: 300
  keepaliveInterval: 75
```

2. **Tune TCP parameters:**
```bash
# /etc/sysctl.conf
net.ipv4.tcp_keepalive_time = 300
net.ipv4.tcp_keepalive_probes = 3
net.ipv4.tcp_keepalive_intvl = 75

# Apply settings
sudo sysctl -p
```

---

## Performance Issues

### High CPU Usage

**Identify CPU-Intensive Queries:**

```bash
# Monitor CPU per query
themisdb-admin query list --sort-by cpu

# Show slow queries
curl http://localhost:8529/_api/query/slow
```

**Analysis Script:**
```bash
#!/bin/bash
# cpu_monitor.sh

while true; do
  CPU=$(ps -p $(pgrep themisdb-server) -o %cpu= | awk '{print $1}')
  if (( $(echo "$CPU > 80" | bc -l) )); then
    echo "[$(date)] High CPU: $CPU%"
    curl -s http://localhost:8529/_api/query/current >> high_cpu_queries.log
  fi
  sleep 5
done
```

**Common Fixes:**

```aql
-- 1. Avoid expensive operations in loops
-- BAD
FOR doc IN collection
  LET processed = DOCUMENT("other", doc.relatedId)  // N+1 query
  RETURN processed

-- GOOD
LET relatedIds = (FOR doc IN collection RETURN doc.relatedId)
LET related = DOCUMENT("other", relatedIds)
RETURN related

-- 2. Use early pruning
FOR doc IN collection
  LIMIT 100  // Limit early
  FILTER doc.status == "active"
  RETURN doc
```

---

### Disk I/O Bottlenecks

**Diagnosis:**

```bash
# Monitor I/O wait
iostat -x 5

# Check disk latency
sudo iotop -o

# ThemisDB I/O statistics
curl http://localhost:8529/_admin/statistics-description | jq '.groups[] | select(.name == "rocksdb")'
```

**Optimization Strategies:**

1. **Move to faster storage:**
```bash
# Benchmark storage performance
fio --name=random-write --ioengine=libaio --iodepth=32 --rw=randwrite \
    --bs=4k --direct=1 --size=4G --numjobs=4 --runtime=60
```

2. **Optimize RocksDB:**
```yaml
# themisdb.conf
storage:
  rocksdb:
    writeBufferSize: 512MB
    maxWriteBufferNumber: 4
    minWriteBufferNumberToMerge: 2
    level0SlowdownTrigger: 20
    level0StopTrigger: 36
```

3. **Enable compression:**
```aql
-- Collection-level compression
db._create("collection", {
  compression: "lz4",
  cacheEnabled: true
});
```

---

### High Network Latency

**Measure Latency:**

```bash
# Application to database
time curl http://database-server:8529/_api/version

# Between cluster nodes
themisdb-cluster-tool latency-test --nodes node1,node2,node3
```

**Solutions:**

```yaml
# Batch operations
const operations = [];
for (let i = 0; i < 1000; i++) {
  operations.push({
    type: 2300,  // INSERT
    collection: 'users',
    document: { name: `user${i}` }
  });
}

// Single network round-trip
db._connection.request({
  method: 'POST',
  path: '/_api/batch',
  body: operations
});
```

---

## Memory Problems

### Out of Memory Errors

**Symptoms:**
```
ERROR Cannot allocate memory
ERROR jemalloc: Out of memory
FATAL Memory allocation failed: requested 1073741824 bytes
```

**Immediate Response:**

```bash
# Check memory usage
free -h
ps aux --sort=-%mem | head -n 10

# ThemisDB memory statistics
curl http://localhost:8529/_admin/statistics | jq '.server.physicalMemory'

# Emergency: Clear query cache
curl -X DELETE http://localhost:8529/_api/query/cache
```

**Long-term Solutions:**

1. **Configure Memory Limits:**
```yaml
# themisdb.conf
server:
  maxMemorySize: 16GB  # 80% of available RAM
  
cache:
  size: 4GB
  
query:
  cacheMaxMemory: 2GB
  maxMemoryPerQuery: 1GB
```

2. **Enable Memory Monitoring:**
```bash
# Create monitoring script
cat > /opt/themisdb/monitor_memory.sh << 'EOF'
#!/bin/bash
THRESHOLD=80
MEM_USAGE=$(free | grep Mem | awk '{print int($3/$2 * 100)}')

if [ $MEM_USAGE -gt $THRESHOLD ]; then
  echo "High memory usage: $MEM_USAGE%" | mail -s "ThemisDB Memory Alert" admin@company.com
  curl -X POST http://localhost:8529/_admin/cache/clear
fi
EOF

# Schedule check every 5 minutes
*/5 * * * * /opt/themisdb/monitor_memory.sh
```

---

### Memory Leaks

**Detection:**

```bash
# Monitor memory growth over time
while true; do
  date >> mem_growth.log
  ps -p $(pgrep themisdb-server) -o pid,vsz,rss,comm >> mem_growth.log
  sleep 300
done

# Analyze growth
gnuplot << EOF
set terminal png
set output 'memory_trend.png'
plot 'mem_growth.log' using 2 with lines title 'VSZ'
EOF
```

**Investigation:**

```bash
# Enable memory profiling
export THEMISDB_MEMORY_PROFILE=1

# Generate heap profile
kill -USR1 $(pgrep themisdb-server)

# Analyze with valgrind (development only)
valgrind --leak-check=full --track-origins=yes \
  themisdb-server --config /etc/themisdb/themisdb.conf
```

---

### Cache Pressure

**Symptoms:**
- High cache eviction rate
- Increased query latency
- Frequent disk reads

**Optimize Cache Usage:**

```aql
-- Check cache statistics
db._query("RETURN CURRENT_CACHE_INFO()");

-- Prioritize hot data
db._collection("hot_data").properties({
  cacheEnabled: true,
  
});

db._collection("cold_data").properties({
  cacheEnabled: false
});
```

**Cache Configuration:**
```yaml
# themisdb.conf
cache:
  size: 8GB
  
  # Collection-specific limits
  collections:
    - name: "users"
      maxSize: 2GB
    - name: "sessions"
      maxSize: 1GB
      ttl: 3600
```

---

## Crash Scenarios

### Segmentation Fault

**Gather Crash Information:**

```bash
# Enable core dumps
ulimit -c unlimited
echo "/var/crash/core.%e.%p.%t" | sudo tee /proc/sys/kernel/core_pattern

# After crash, analyze core dump
gdb /usr/bin/themisdb-server /var/crash/core.themisdb-server.12345.timestamp

# Get backtrace
(gdb) bt full
(gdb) thread apply all bt
(gdb) info threads
```

**Check System Logs:**
```bash
# Kernel messages
dmesg -T | grep -i "themisdb\|segfault"

# System journal
journalctl -u themisdb --since "1 hour ago" -p err
```

---

### Unexpected Shutdowns

**Diagnosis Checklist:**

```bash
# 1. Check for OOM killer
grep -i "killed process" /var/log/syslog

# 2. Verify disk space
df -h
# Check specific data directory
du -sh /var/lib/themisdb/*

# 3. Review last successful operations
themisdb-admin logs --level error --since "2 hours ago"

# 4. Check for hardware issues
mcelog --ascii
```

**Prevention:**

```yaml
# themisdb.conf - Graceful shutdown settings
server:
  shutdownTimeout: 60
  flushDataOnShutdown: true
  
# Systemd service with restart policy
# /etc/systemd/system/themisdb.service
[Service]
Restart=on-failure
RestartSec=10s
StartLimitInterval=300s
StartLimitBurst=5
```

---

### Database Corruption After Crash

**Assessment:**

```bash
# Check database integrity
themisdb-server --database.check-integrity /var/lib/themisdb/

# Verify individual collections
themisdb-admin verify-collection --database mydb --collection users
```

**Recovery Steps:**

See [Data Corruption Recovery](#data-corruption-recovery) section.

---

## Data Corruption Recovery

### Detection

**Signs of Corruption:**
- Checksum errors in logs
- Inconsistent query results
- Crash on specific collection access
- Failed integrity checks

**Verification:**

```bash
# Full database scan
themisdb-server --database.integrity-check \
  --database.path /var/lib/themisdb/databases/mydb

# Per-collection verification
for coll in $(themisdb-admin list-collections); do
  echo "Checking $coll..."
  themisdb-admin verify-collection --name $coll --repair false
done
```

---

### Recovery Procedures

#### Level 1: Automatic Repair

```bash
# Stop database
systemctl stop themisdb

# Run automatic repair
themisdb-server --database.auto-repair \
  --database.path /var/lib/themisdb/databases/mydb \
  2>&1 | tee repair.log

# Verify repair
themisdb-server --database.integrity-check \
  --database.path /var/lib/themisdb/databases/mydb
```

#### Level 2: Manual Recovery

```bash
# 1. Export uncorrupted data
themisdb-admin export \
  --database mydb \
  --collection users \
  --output-directory /tmp/recovery/ \
  --skip-corrupted

# 2. Drop corrupted collection
themisdb-admin drop-collection --database mydb --collection users

# 3. Recreate collection
themisdb-admin create-collection \
  --database mydb \
  --collection users \
  --type document

# 4. Import recovered data
themisdb-admin import \
  --database mydb \
  --collection users \
  --file /tmp/recovery/users.json
```

#### Level 3: Backup Restoration

```bash
# Restore from last good backup
themisdb-restore \
  --backup-directory /backups/themisdb/2024-01-24/ \
  --database mydb \
  --overwrite

# Apply WAL logs since backup
themisdb-admin wal-replay \
  --wal-directory /var/lib/themisdb/wal/ \
  --start-time "2024-01-24 00:00:00"
```

---

### Data Consistency Checks

**Post-Recovery Validation:**

```bash
#!/bin/bash
# validate_recovery.sh

echo "1. Checking collection counts..."
themisdb-admin collection-stats --database mydb

echo "2. Validating indexes..."
for coll in $(themisdb-admin list-collections --database mydb); do
  themisdb-admin validate-indexes --collection $coll
done

echo "3. Running test queries..."
themisdb-shell --database mydb << 'EOF'
// Test basic operations
db._query("FOR doc IN users LIMIT 10 RETURN doc");

// Test joins
db._query("FOR u IN users FOR o IN orders FILTER u._key == o.userId LIMIT 10 RETURN {u, o}");

// Test aggregations
db._query("FOR doc IN users COLLECT status = doc.status WITH COUNT INTO num RETURN {status, num}");
EOF

echo "Recovery validation complete!"
```

---

## Log Analysis Guide

### Log Levels

```yaml
# themisdb.conf - Configure logging
log:
  level: INFO  # FATAL, ERROR, WARNING, INFO, DEBUG, TRACE
  output: file
  file: /var/log/themisdb/themisdb.log
  
  # Component-specific levels
  topic:
    queries: DEBUG
    replication: INFO
    cluster: WARNING
```

---

### Common Log Patterns

#### Slow Query Detection

```bash
# Pattern
grep "slow query" /var/log/themisdb/themisdb.log

# Example log entry
2024-01-24 10:15:30 WARNING [queries] Slow query (15.3s): FOR doc IN large_collection FILTER doc.field == "value" RETURN doc
```

**Automated Analysis:**
```bash
awk '/slow query/ {
  match($0, /\(([0-9.]+)s\)/, time);
  if (time[1] > 10) {
    print $0
  }
}' /var/log/themisdb/themisdb.log | sort -t'(' -k2 -n
```

---

#### Connection Issues

```bash
# Pattern: Failed authentication
grep "authentication failed" /var/log/themisdb/themisdb.log

# Pattern: Connection refused
grep "connection refused\|timeout" /var/log/themisdb/themisdb.log

# Pattern: Max connections reached
grep "max connections\|connection pool" /var/log/themisdb/themisdb.log
```

---

#### Replication Problems

```bash
# Replication lag detection
grep "replication.*behind\|replication.*delay" /var/log/themisdb/themisdb.log

# Split-brain scenario
grep "split.*brain\|conflicting.*leader" /var/log/themisdb/themisdb.log

# Network partition
grep "quorum.*lost\|cluster.*unreachable" /var/log/themisdb/themisdb.log
```

---

### Log Analysis Tools

**Structured Log Parsing:**

```python
#!/usr/bin/env python3
# parse_themis_logs.py

import re
import json
from collections import Counter
from datetime import datetime

log_pattern = re.compile(
    r'(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) (\w+) \[(\w+)\] (.+)'
)

errors = Counter()
warnings = Counter()

with open('/var/log/themisdb/themisdb.log', 'r') as f:
    for line in f:
        match = log_pattern.match(line)
        if match:
            timestamp, level, component, message = match.groups()
            
            if level == 'ERROR':
                errors[component] += 1
            elif level == 'WARNING':
                warnings[component] += 1

print("Error Summary by Component:")
for component, count in errors.most_common(10):
    print(f"  {component}: {count}")

print("\nWarning Summary by Component:")
for component, count in warnings.most_common(10):
    print(f"  {component}: {count}")
```

---

## Diagnostic Commands

### System Health Check

```bash
#!/bin/bash
# healthcheck.sh - Comprehensive system diagnostic

echo "=== ThemisDB Health Check ==="
echo "Date: $(date)"
echo

# 1. Service Status
echo "1. Service Status"
systemctl status themisdb | grep "Active:"
echo

# 2. Database Version
echo "2. Database Version"
curl -s http://localhost:8529/_api/version | jq -r '.version'
echo

# 3. Cluster Status
echo "3. Cluster Status"
curl -s http://localhost:8529/_admin/cluster/health | jq '.'
echo

# 4. Storage Usage
echo "4. Storage Usage"
du -sh /var/lib/themisdb/*
df -h | grep themisdb
echo

# 5. Memory Usage
echo "5. Memory Usage"
ps -p $(pgrep themisdb-server) -o pid,%mem,rss,vsz,comm
echo

# 6. Active Connections
echo "6. Active Connections"
netstat -an | grep :8529 | wc -l
echo

# 7. Recent Errors
echo "7. Recent Errors (last hour)"
journalctl -u themisdb --since "1 hour ago" -p err --no-pager | tail -n 10
echo

# 8. Query Cache Statistics
echo "8. Query Cache Statistics"
curl -s http://localhost:8529/_api/query/cache/properties | jq '.'
echo

echo "=== Health Check Complete ==="
```

---

### Performance Profiling

```bash
# CPU profiling
perf record -p $(pgrep themisdb-server) -g -- sleep 30
perf report

# Memory profiling
heaptrack themisdb-server --config /etc/themisdb/themisdb.conf

# I/O profiling
iotop -P -p $(pgrep themisdb-server) -d 5

# Network profiling
iftop -i eth0 -f "port 8529"
```

---

### Query Analysis

```javascript
// Execute in themisdb-shell

// Show running queries
db._query("FOR q IN _current.queries RETURN q");

// Kill long-running query
db._connection.DELETE('/_api/query/' + queryId);

// Analyze query plan
db._explain(`
  FOR doc IN collection
    FILTER doc.field == "value"
    RETURN doc
`);

// Profile query execution
db._query({
  query: "FOR doc IN collection RETURN doc",
  options: {
    profile: 2,
    fullCount: true
  }
});
```

---

### Collection Statistics

```bash
# Detailed collection information
themisdb-admin collection-info --database mydb --collection users

# Index usage statistics
curl -s http://localhost:8529/_api/index?collection=users | jq '.indexes[] | {name: .name, type: .type, fields: .fields, selectivityEstimate: .selectivityEstimate}'

# Document count and size
db._collection("users").count();
db._collection("users").properties().objectSize;
```

---

## When to File a Bug

### Criteria for Bug Reports

File a bug if you encounter:

1. **Crashes or Segmentation Faults**
   - Server crashes unexpectedly
   - Core dumps generated
   - Unrecoverable errors

2. **Data Loss or Corruption**
   - Documents disappear
   - Data inconsistencies
   - Index corruption

3. **Incorrect Query Results**
   - Wrong data returned
   - Missing or duplicate results
   - Inconsistent behavior

4. **Performance Regressions**
   - Significant slowdown compared to previous versions
   - Memory leaks
   - CPU spikes without load increase

5. **Security Vulnerabilities**
   - Authentication bypasses
   - Unauthorized data access
   - Injection vulnerabilities

---

### Bug Report Template

```markdown
**ThemisDB Version:** 1.4.0

**Operating System:** Ubuntu 22.04 LTS

**Hardware:**
- CPU: Intel Xeon E5-2680 v4
- RAM: 64 GB
- Storage: SSD RAID 10

**Description:**
Clear description of the issue...

**Steps to Reproduce:**
1. Create collection with...
2. Insert 10000 documents...
3. Execute query: FOR doc IN...
4. Observe error...

**Expected Behavior:**
Query should return 100 documents in <1s

**Actual Behavior:**
Query times out after 30s

**Logs:**
```
2024-01-24 10:15:30 ERROR [queries] Timeout in query execution
2024-01-24 10:15:30 DEBUG [queries] Query: FOR doc IN...
```

**Configuration:**
[Attach themisdb.conf with sensitive data redacted]

**Additional Context:**
- Issue started after upgrade from 1.3.5 to 1.4.0
- Only occurs with collections > 1M documents
- Reproducible in test environment
```

---

### Before Filing

**Checklist:**

- [ ] Search existing issues for duplicates
- [ ] Test on latest version
- [ ] Include minimal reproducible example
- [ ] Gather all relevant logs
- [ ] Document configuration
- [ ] Note system specifications
- [ ] Describe expected vs actual behavior
- [ ] Include version information

**Where to Report:**

- GitHub Issues: https://github.com/ThemisDB/ThemisDB/issues
- Security Issues: security@themisdb.org (do not file publicly)
- Forum: https://community.themisdb.org

---

## Quick Reference

### Emergency Commands

```bash
# Stop database immediately
systemctl stop themisdb
kill -9 $(pgrep themisdb-server)

# Clear all caches
curl -X DELETE http://localhost:8529/_api/query/cache
curl -X POST http://localhost:8529/_admin/cache/clear

# Export all data
themisdb-admin export-all --output /tmp/emergency-backup/

# Check data integrity
themisdb-server --database.integrity-check --database.path /var/lib/themisdb/
```

### Support Resources

- Documentation: https://docs.themisdb.org
- Community Forum: https://community.themisdb.org  
- Professional Support: support@themisdb.com
- Slack Channel: https://themisdb.slack.com

---

**Last Updated:** 2026-04-06  
**Version:** 1.4.0  
**Maintainer:** ThemisDB Team
