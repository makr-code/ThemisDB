# ThemisDB Log Analysis Guide

Complete guide to analyzing ThemisDB logs for debugging, monitoring, and optimization.

## Table of Contents

- [Log Levels and Configuration](#log-levels-and-configuration)
- [Log Format Explanation](#log-format-explanation)
- [Common Log Patterns](#common-log-patterns)
- [Error Interpretation](#error-interpretation)
- [Performance Insights from Logs](#performance-insights-from-logs)
- [Log Aggregation](#log-aggregation)
- [Alerting Setup](#alerting-setup)
- [Log Retention Policies](#log-retention-policies)

---

## Log Levels and Configuration

### Log Levels

ThemisDB supports multiple log levels for granular control:

| Level | Purpose | When to Use | Volume |
|-------|---------|-------------|--------|
| **FATAL** | Unrecoverable errors | Always enabled | Very Low |
| **ERROR** | Error conditions | Production | Low |
| **WARNING** | Warning messages | Production | Medium |
| **INFO** | Informational messages | Production | Medium |
| **DEBUG** | Debugging information | Development/troubleshooting | High |
| **TRACE** | Detailed execution trace | Deep debugging | Very High |

---

### Configuration

**Basic Configuration:**

```yaml
# themisdb.conf

log:
  # Global log level
  level: INFO
  
  # Output destination
  output: file  # Options: file, syslog, console
  
  # Log file settings
  file: /var/log/themisdb/themisdb.log
  maxFileSize: 100MB
  maxFiles: 10
  compress: true
  
  # Log format
  format: text  # Options: text, json
  timestamps: true
  threadId: true
  sourceLocation: false
```

---

### Component-Specific Logging

**Fine-grained control:**

```yaml
log:
  level: INFO
  
  # Per-component log levels
  topics:
    queries: DEBUG        # All AQL queries
    replication: INFO     # Replication events
    cluster: WARNING      # Cluster management
    performance: DEBUG    # Performance metrics
    security: INFO        # Authentication/authorization
    storage: WARNING      # RocksDB operations
    network: ERROR        # Network I/O
```

**Runtime Configuration:**

```bash
# Change log level without restart
curl -X PUT http://localhost:8529/_admin/log/level \
  -H "Content-Type: application/json" \
  -d '{
    "queries": "DEBUG",
    "performance": "TRACE"
  }'

# Get current log levels
curl http://localhost:8529/_admin/log/level
```

---

### Structured Logging (JSON)

**Enable JSON format for parsing:**

```yaml
log:
  format: json
  fields:
    - timestamp
    - level
    - topic
    - message
    - threadId
    - pid
    - hostname
```

**Example JSON log entry:**

```json
{
  "timestamp": "2024-01-24T10:15:30.123Z",
  "level": "INFO",
  "topic": "queries",
  "message": "Query executed successfully",
  "queryId": "q12345",
  "duration": 0.125,
  "collections": ["users"],
  "threadId": 14235,
  "pid": 12345,
  "hostname": "db-server-01"
}
```

---

## Log Format Explanation

### Text Format

**Standard log format:**

```
2024-01-24 10:15:30.123 [INFO] [queries] [thread-14235] Query executed: FOR doc IN users LIMIT 10 RETURN doc (125ms)
│                        │      │        │              │
│                        │      │        │              └─ Log message
│                        │      │        └─ Thread ID
│                        │      └─ Topic/Component
│                        └─ Log level
└─ Timestamp
```

---

### Key Components

**1. Timestamp:**
```
2024-01-24 10:15:30.123
│          │  │  │  │
│          │  │  │  └─ Milliseconds
│          │  │  └─ Seconds
│          │  └─ Minutes
│          └─ Hours (24h format)
└─ Date (YYYY-MM-DD)
```

**2. Log Level:**
- `[FATAL]` - Red flag, immediate action required
- `[ERROR]` - Problem occurred, investigate
- `[WARNING]` - Potential issue, monitor
- `[INFO]` - Normal operation
- `[DEBUG]` - Debugging information
- `[TRACE]` - Detailed trace

**3. Topic:**
Common topics include:
- `[queries]` - Query execution
- `[replication]` - Replication events
- `[cluster]` - Cluster operations
- `[performance]` - Performance metrics
- `[security]` - Security events
- `[storage]` - Storage operations

---

### Special Log Markers

**Query Logs:**
```
[queries] Query executed: FOR doc IN users FILTER doc.status == "active" RETURN doc
  Duration: 0.125s
  Collections: users
  Indexes: idx_status (persistent)
  Scanned: 1000 documents
  Returned: 250 documents
```

**Slow Query Logs:**
```
[queries] [SLOW] Query exceeded threshold (30s > 10s):
  Query: FOR doc IN large_collection RETURN doc
  Duration: 30.456s
  Scanned: 10000000 documents
  Tip: Consider adding index on filter fields
```

**Error Logs:**
```
[ERROR] [queries] Query execution failed:
  Error: Division by zero
  Code: THEMIS_E_QUERY_RUNTIME
  Query: FOR doc IN orders RETURN doc.total / doc.quantity
  Line: 1, Column: 35
  Document: orders/12345
```

---

## Common Log Patterns

### Startup Sequence

**Normal startup:**
```
2024-01-24 10:00:00.000 [INFO] [server] ThemisDB 1.4.0 starting
2024-01-24 10:00:00.100 [INFO] [config] Loading configuration: /etc/themisdb/themisdb.conf
2024-01-24 10:00:00.200 [INFO] [storage] Opening RocksDB: /var/lib/themisdb/data
2024-01-24 10:00:01.000 [INFO] [storage] RocksDB opened successfully
2024-01-24 10:00:01.100 [INFO] [network] Listening on 0.0.0.0:8529
2024-01-24 10:00:01.200 [INFO] [cluster] Node joined cluster: node1
2024-01-24 10:00:01.300 [INFO] [server] ThemisDB ready for connections
```

**Startup with warnings:**
```
2024-01-24 10:00:00.000 [INFO] [server] ThemisDB 1.4.0 starting
2024-01-24 10:00:00.500 [WARNING] [config] Deprecated option 'storage.journalSize' - use 'storage.rocksdb.walSize'
2024-01-24 10:00:01.000 [WARNING] [storage] Data directory not cleanly shut down, running recovery
2024-01-24 10:00:05.000 [INFO] [storage] Recovery complete
2024-01-24 10:00:05.100 [INFO] [server] ThemisDB ready for connections
```

---

### Query Execution Patterns

**Successful query:**
```
[INFO] [queries] Query started: q12345
[DEBUG] [queries] Parsing query: FOR doc IN users FILTER doc.status == "active" RETURN doc
[DEBUG] [queries] Query optimization: applied 3 rules
[DEBUG] [queries] Using index: idx_status
[INFO] [queries] Query completed: q12345 (duration: 0.125s, scanned: 1000, returned: 250)
```

**Failed query:**
```
[INFO] [queries] Query started: q12346
[DEBUG] [queries] Parsing query: FOR doc IN users FILTER doc.invalid == "value" RETURN doc
[ERROR] [queries] Query parse error: Unknown attribute 'invalid'
[ERROR] [queries] Query failed: q12346 (error: THEMIS_E_QUERY_PARSE)
```

**Slow query:**
```
[INFO] [queries] Query started: q12347
[WARNING] [queries] Query execution time: 15.5s (threshold: 10s)
[WARNING] [performance] Full collection scan detected: users (10M documents)
[WARNING] [queries] Consider adding index on: users.timestamp
```

---

### Replication Patterns

**Healthy replication:**
```
[INFO] [replication] Replication started: users -> replica-1
[DEBUG] [replication] Syncing operations: 1000 pending
[DEBUG] [replication] Sync batch completed: 1000 operations (0.5s)
[INFO] [replication] Replication lag: 0.1s
```

**Replication lag:**
```
[WARNING] [replication] Replication lag detected: 30s
[WARNING] [replication] Replica falling behind: replica-1
[DEBUG] [replication] Pending operations: 50000
[INFO] [replication] Increasing sync batch size to catch up
```

**Replication failure:**
```
[ERROR] [replication] Connection to replica lost: replica-1
[WARNING] [replication] Retrying connection (attempt 1/5)
[ERROR] [replication] Connection retry failed: timeout
[WARNING] [replication] Buffering operations for replica-1
[INFO] [replication] Buffer size: 10000 operations (100 MB)
```

---

### Cluster Events

**Node join:**
```
[INFO] [cluster] Node requesting to join: node3
[DEBUG] [cluster] Validating node credentials: node3
[INFO] [cluster] Node joined cluster: node3
[INFO] [cluster] Rebalancing shards across nodes
[DEBUG] [cluster] Moving shard: users_s001 -> node3
[INFO] [cluster] Cluster rebalanced successfully
```

**Node failure:**
```
[ERROR] [cluster] Node heartbeat timeout: node2
[WARNING] [cluster] Marking node as unhealthy: node2
[INFO] [cluster] Initiating failover for shards on node2
[INFO] [cluster] Promoting shard replica: users_s002
[WARNING] [cluster] Cluster degraded: 2/3 nodes healthy
```

---

### Performance Warnings

**Memory pressure:**
```
[WARNING] [performance] Memory usage high: 85% (40GB / 47GB)
[WARNING] [performance] Cache eviction rate increasing: 1000/s
[INFO] [performance] Starting aggressive cache cleanup
[WARNING] [performance] Query memory limit reached, spilling to disk
```

**I/O bottleneck:**
```
[WARNING] [performance] Disk I/O wait high: 45%
[WARNING] [storage] RocksDB write stall detected
[DEBUG] [storage] Write buffer full, flushing to disk
[INFO] [storage] Compaction triggered: level 0 -> level 1
```

**Connection exhaustion:**
```
[WARNING] [network] High connection count: 950/1000
[WARNING] [network] Connection pool pressure: 95%
[ERROR] [network] Cannot accept new connection: pool exhausted
[INFO] [network] Oldest connection: 3600s (1 hour idle)
```

---

## Error Interpretation

### Common Error Patterns

**1. Query Errors:**

```
ERROR [queries] Query execution failed: THEMIS_E_QUERY_TIMEOUT
  Query: FOR doc IN large_collection RETURN doc
  Timeout: 30s
  
SOLUTION:
- Add index on filter fields
- Increase query timeout: OPTIONS {timeout: 60000}
- Optimize query with LIMIT clause
```

**2. Connection Errors:**

```
ERROR [network] Connection refused: localhost:8529
  Reason: Connection timeout
  
SOLUTION:
- Check if database is running: systemctl status themisdb
- Verify port is listening: netstat -tulpn | grep 8529
- Check firewall rules: iptables -L | grep 8529
```

**3. Authentication Errors:**

```
ERROR [security] Authentication failed: user@localhost
  Reason: Invalid credentials
  Attempts: 3
  
SOLUTION:
- Verify username/password
- Check user exists: themisdb-admin user-list
- Reset password if needed: themisdb-admin user-password-reset
```

**4. Storage Errors:**

```
ERROR [storage] Cannot write to data directory: /var/lib/themisdb
  Reason: Disk full
  Available: 0 MB
  
SOLUTION:
- Free up disk space: df -h
- Clean old logs: find /var/log/themisdb -mtime +30 -delete
- Expand storage or add volumes
```

**5. Replication Errors:**

```
ERROR [replication] Replication divergence detected
  Source: node1, Replica: node2
  Divergence point: timestamp 2024-01-24T10:15:30Z
  
SOLUTION:
- Stop replication: themisdb-admin replication-stop
- Rebuild replica from backup
- Restart replication: themisdb-admin replication-start
```

---

### Error Code Reference

**Query Errors (1xxx):**
- `1001` - Query parse error
- `1002` - Query timeout
- `1003` - Invalid query syntax
- `1004` - Collection not found
- `1005` - Index not found

**Storage Errors (2xxx):**
- `2001` - Disk I/O error
- `2002` - Data corruption
- `2003` - Insufficient disk space
- `2004` - File permission denied

**Network Errors (3xxx):**
- `3001` - Connection refused
- `3002` - Connection timeout
- `3003` - SSL/TLS error
- `3004` - Connection pool exhausted

**Cluster Errors (4xxx):**
- `4001` - Node unreachable
- `4002` - Quorum lost
- `4003` - Split-brain detected
- `4004` - Shard not available

---

## Performance Insights from Logs

### Analyzing Query Performance

**Extract slow queries:**

```bash
# Find queries slower than 10s
grep -E '\[queries\].*duration: [1-9][0-9]\.' /var/log/themisdb/themisdb.log

# Top 10 slowest queries
grep -oP 'Query:.*duration: \K[0-9.]+' /var/log/themisdb/themisdb.log | \
  sort -rn | head -10

# Queries by collection
grep '\[queries\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'collection: \K\w+' | sort | uniq -c | sort -rn
```

**Automated slow query report:**

```python
#!/usr/bin/env python3
# slow_query_report.py

import re
import sys
from collections import defaultdict

log_file = sys.argv[1] if len(sys.argv) > 1 else '/var/log/themisdb/themisdb.log'

slow_queries = defaultdict(list)

pattern = re.compile(
    r'(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}).*\[queries\].*'
    r'Query: (.+) duration: ([\d.]+)s'
)

with open(log_file, 'r') as f:
    for line in f:
        match = pattern.search(line)
        if match:
            timestamp, query, duration = match.groups()
            duration_float = float(duration)
            
            if duration_float > 10:  # Threshold: 10s
                slow_queries[query].append({
                    'timestamp': timestamp,
                    'duration': duration_float
                })

# Report
print("=== Slow Query Report ===\n")

for query, executions in sorted(slow_queries.items(), 
                                key=lambda x: sum(e['duration'] for e in x[1]), 
                                reverse=True)[:10]:
    
    total_time = sum(e['duration'] for e in executions)
    avg_time = total_time / len(executions)
    max_time = max(e['duration'] for e in executions)
    
    print(f"Query: {query[:80]}...")
    print(f"  Executions: {len(executions)}")
    print(f"  Total time: {total_time:.2f}s")
    print(f"  Avg time: {avg_time:.2f}s")
    print(f"  Max time: {max_time:.2f}s")
    print()
```

---

### Cache Performance Analysis

**Cache hit rate over time:**

```bash
#!/bin/bash
# cache_analysis.sh

echo "Cache Performance Analysis"
echo "=========================="

# Extract cache statistics from logs
grep 'cache hit rate' /var/log/themisdb/themisdb.log | \
  awk '{
    split($1, date, "-");
    hour = substr($2, 1, 2);
    rate = $NF;
    gsub("%", "", rate);
    sum[hour] += rate;
    count[hour]++;
  }
  END {
    for (h in sum) {
      avg = sum[h] / count[h];
      printf "Hour %02d: %.2f%%\n", h, avg;
    }
  }' | sort
```

**Cache eviction patterns:**

```bash
# Find times with high eviction rates
grep 'cache eviction' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | cut -d: -f1 | uniq -c | \
  awk '{if ($1 > 100) print $0}'
```

---

### I/O Performance Tracking

**Detect I/O bottlenecks:**

```bash
#!/bin/bash
# io_analysis.sh

echo "I/O Performance Analysis"
echo "======================="

# Extract I/O wait times
grep 'I/O wait' /var/log/themisdb/themisdb.log | \
  awk '{
    match($0, /([0-9]+)%/, arr);
    if (arr[1] > 30) {
      print $1, $2, "I/O wait:", arr[1] "%"
    }
  }'

# RocksDB compaction events
echo ""
echo "Compaction Events:"
grep 'compaction' /var/log/themisdb/themisdb.log | wc -l

# Write stalls
echo ""
echo "Write Stalls:"
grep 'write stall' /var/log/themisdb/themisdb.log | wc -l
```

---

### Connection Patterns

**Analyze connection usage:**

```bash
#!/bin/bash
# connection_analysis.sh

LOG_FILE="/var/log/themisdb/themisdb.log"

echo "Connection Analysis"
echo "==================="

# Peak connections
echo "Peak Connections:"
grep 'connection count' "$LOG_FILE" | \
  awk '{match($0, /([0-9]+)\/([0-9]+)/, arr); print arr[1]}' | \
  sort -n | tail -1

# Connection errors
echo ""
echo "Connection Errors:"
grep -c 'connection.*failed\|pool exhausted' "$LOG_FILE"

# Connection sources
echo ""
echo "Top Connection Sources:"
grep 'new connection from' "$LOG_FILE" | \
  awk '{print $NF}' | sort | uniq -c | sort -rn | head -10
```

---

## Log Aggregation

### Centralized Logging with rsyslog

**Configuration:**

```bash
# /etc/rsyslog.d/themisdb.conf

# Forward ThemisDB logs to central server
if $programname == 'themisdb' then {
  action(
    type="omfwd"
    target="log-server.company.com"
    port="514"
    protocol="tcp"
    queue.type="LinkedList"
    queue.size="10000"
  )
}
```

---

### ELK Stack Integration

**Filebeat configuration:**

```yaml
# filebeat.yml

filebeat.inputs:
  - type: log
    enabled: true
    paths:
      - /var/log/themisdb/themisdb.log
    
    # Parse JSON logs
    json.keys_under_root: true
    json.add_error_key: true
    
    # Add metadata
    fields:
      service: themisdb
      environment: production
      datacenter: us-east-1

output.elasticsearch:
  hosts: ["elasticsearch:9200"]
  index: "themisdb-%{+yyyy.MM.dd}"

# Optional: Logstash output
output.logstash:
  hosts: ["logstash:5044"]
```

**Logstash pipeline:**

```ruby
# logstash-themisdb.conf

input {
  beats {
    port => 5044
  }
}

filter {
  # Parse text format logs
  if [service] == "themisdb" and [message] !~ /^{/ {
    grok {
      match => {
        "message" => "%{TIMESTAMP_ISO8601:timestamp} \[%{WORD:level}\] \[%{WORD:topic}\] %{GREEDYDATA:log_message}"
      }
    }
    
    date {
      match => ["timestamp", "ISO8601"]
      target => "@timestamp"
    }
  }
  
  # Extract query duration
  if [topic] == "queries" {
    grok {
      match => {
        "log_message" => "duration: %{NUMBER:query_duration:float}s"
      }
    }
  }
  
  # Extract error codes
  if [level] == "ERROR" {
    grok {
      match => {
        "log_message" => "error: THEMIS_E_%{WORD:error_code}"
      }
    }
  }
}

output {
  elasticsearch {
    hosts => ["elasticsearch:9200"]
    index => "themisdb-%{+YYYY.MM.dd}"
  }
}
```

**Kibana dashboards:**

```json
{
  "title": "ThemisDB Query Performance",
  "visualizations": [
    {
      "type": "line",
      "title": "Query Duration Over Time",
      "query": "topic:queries AND query_duration:*",
      "yAxis": "avg(query_duration)"
    },
    {
      "type": "bar",
      "title": "Queries by Collection",
      "query": "topic:queries",
      "aggregation": "count",
      "groupBy": "collection"
    },
    {
      "type": "pie",
      "title": "Error Distribution",
      "query": "level:ERROR",
      "groupBy": "error_code"
    }
  ]
}
```

---

### Splunk Integration

**Input configuration:**

```ini
# inputs.conf

[monitor:///var/log/themisdb/themisdb.log]
sourcetype = themisdb:log
index = themisdb
disabled = false

[monitor:///var/log/themisdb/slow-queries.log]
sourcetype = themisdb:slow_queries
index = themisdb
disabled = false
```

**Field extraction:**

```ini
# props.conf

[themisdb:log]
TRANSFORMS-extract_fields = themisdb_fields

# transforms.conf
[themisdb_fields]
REGEX = ^(\S+\s+\S+)\s+\[(\w+)\]\s+\[(\w+)\]\s+(.*)$
FORMAT = timestamp::$1 level::$2 topic::$3 message::$4
```

---

## Alerting Setup

### Prometheus Alerting

**Alert rules:**

```yaml
# themisdb_alerts.yml

groups:
  - name: themisdb
    interval: 30s
    rules:
      # High error rate
      - alert: HighErrorRate
        expr: rate(themisdb_errors_total[5m]) > 10
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High error rate detected"
          description: "Error rate is {{ $value }} errors/sec"
      
      # Slow queries
      - alert: SlowQueries
        expr: themisdb_query_duration_seconds{quantile="0.95"} > 10
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Slow queries detected"
          description: "95th percentile query time: {{ $value }}s"
      
      # Replication lag
      - alert: ReplicationLag
        expr: themisdb_replication_lag_seconds > 30
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Replication lag detected"
          description: "Lag: {{ $value }}s"
      
      # Memory pressure
      - alert: HighMemoryUsage
        expr: themisdb_memory_usage_ratio > 0.9
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "High memory usage"
          description: "Memory usage: {{ $value | humanizePercentage }}"
```

---

### Email Alerts from Logs

**logwatch configuration:**

```bash
#!/bin/bash
# themisdb_logwatch.sh

# Daily log summary email

LOGFILE="/var/log/themisdb/themisdb.log"
EMAIL="admin@company.com"
DATE=$(date +%Y-%m-%d)

{
  echo "ThemisDB Log Summary for $DATE"
  echo "================================"
  echo ""
  
  echo "Errors:"
  grep ERROR "$LOGFILE" | tail -20
  echo ""
  
  echo "Warnings:"
  grep WARNING "$LOGFILE" | wc -l
  echo ""
  
  echo "Slow Queries:"
  grep "SLOW" "$LOGFILE" | wc -l
  echo ""
  
  echo "Top Collections Queried:"
  grep 'collection:' "$LOGFILE" | \
    grep -oP 'collection: \K\w+' | \
    sort | uniq -c | sort -rn | head -10
  
} | mail -s "ThemisDB Log Summary - $DATE" "$EMAIL"
```

---

### Real-time Alert Script

```bash
#!/bin/bash
# realtime_alerts.sh

# Monitor logs in real-time and send alerts

LOGFILE="/var/log/themisdb/themisdb.log"
ALERT_EMAIL="ops@company.com"

tail -F "$LOGFILE" | while read LINE; do
  
  # Critical errors
  if echo "$LINE" | grep -q "FATAL\|CRITICAL"; then
    echo "$LINE" | mail -s "CRITICAL: ThemisDB Alert" "$ALERT_EMAIL"
  fi
  
  # Replication failures
  if echo "$LINE" | grep -q "replication.*failed"; then
    echo "$LINE" | mail -s "WARNING: Replication Issue" "$ALERT_EMAIL"
  fi
  
  # Cluster issues
  if echo "$LINE" | grep -q "quorum lost\|split-brain"; then
    echo "$LINE" | mail -s "CRITICAL: Cluster Issue" "$ALERT_EMAIL"
  fi
  
  # Memory pressure
  if echo "$LINE" | grep -q "memory.*90%\|OOM"; then
    echo "$LINE" | mail -s "WARNING: Memory Pressure" "$ALERT_EMAIL"
  fi
  
done
```

---

## Log Retention Policies

### Retention Strategy

**Recommended retention:**

| Log Type | Retention | Reason |
|----------|-----------|--------|
| Error logs | 90 days | Compliance, troubleshooting |
| Audit logs | 1 year | Security, compliance |
| Query logs | 30 days | Performance analysis |
| Debug logs | 7 days | Development only |
| Slow query logs | 60 days | Optimization |

---

### Log Rotation Configuration

**logrotate configuration:**

```bash
# /etc/logrotate.d/themisdb

/var/log/themisdb/*.log {
    daily
    rotate 30
    compress
    delaycompress
    missingok
    notifempty
    create 0640 themisdb themisdb
    sharedscripts
    
    postrotate
        # Signal ThemisDB to reopen log files
        kill -HUP $(cat /var/run/themisdb/themisdb.pid) 2>/dev/null || true
    endscript
}

# Error logs - keep longer
/var/log/themisdb/error.log {
    weekly
    rotate 12
    compress
    delaycompress
    missingok
    notifempty
    create 0640 themisdb themisdb
}
```

---

### Archival to S3

```bash
#!/bin/bash
# archive_logs.sh

# Archive old logs to S3

LOG_DIR="/var/log/themisdb"
ARCHIVE_DAYS=30
S3_BUCKET="s3://company-logs/themisdb"

# Find logs older than $ARCHIVE_DAYS
find "$LOG_DIR" -name "*.log.*.gz" -mtime +$ARCHIVE_DAYS -type f | while read LOGFILE; do
  
  # Upload to S3
  aws s3 cp "$LOGFILE" "$S3_BUCKET/$(hostname)/$(date +%Y/%m)/" \
    --storage-class GLACIER
  
  # Verify upload
  if aws s3 ls "$S3_BUCKET/$(hostname)/$(date +%Y/%m)/$(basename $LOGFILE)" > /dev/null; then
    # Delete local copy
    rm "$LOGFILE"
    echo "Archived: $LOGFILE"
  else
    echo "Failed to archive: $LOGFILE"
  fi
  
done
```

---

### Automated Cleanup

```bash
#!/bin/bash
# log_cleanup.sh

# Clean up old logs based on retention policy

LOG_DIR="/var/log/themisdb"

# Delete debug logs older than 7 days
find "$LOG_DIR" -name "debug.log*" -mtime +7 -delete

# Delete query logs older than 30 days
find "$LOG_DIR" -name "query.log*" -mtime +30 -delete

# Delete error logs older than 90 days
find "$LOG_DIR" -name "error.log*" -mtime +90 -delete

# Report disk usage
echo "Log disk usage after cleanup:"
du -sh "$LOG_DIR"
```

---

## Quick Reference

### Log Analysis Commands

```bash
# View logs in real-time
tail -f /var/log/themisdb/themisdb.log

# Search for errors
grep ERROR /var/log/themisdb/themisdb.log

# Count errors by type
grep ERROR /var/log/themisdb/themisdb.log | \
  grep -oP 'THEMIS_E_\w+' | sort | uniq -c | sort -rn

# Find slow queries
grep 'duration:' /var/log/themisdb/themisdb.log | \
  awk '$NF > 10' | sort -k $NF -rn

# Connection statistics
grep 'connection' /var/log/themisdb/themisdb.log | \
  grep -c 'new connection'

# Replication status from logs
grep 'replication lag' /var/log/themisdb/themisdb.log | tail -10

# Memory warnings
grep -i 'memory\|OOM' /var/log/themisdb/themisdb.log

# Cluster events
grep 'cluster' /var/log/themisdb/themisdb.log | grep -E 'join|leave|failed'
```

---

### Log Levels Quick Guide

```bash
# Temporarily enable debug logging
curl -X PUT http://localhost:8529/_admin/log/level \
  -d '{"queries": "DEBUG"}'

# Reset to default (INFO)
curl -X PUT http://localhost:8529/_admin/log/level \
  -d '{"queries": "INFO"}'

# Enable trace for specific issue
curl -X PUT http://localhost:8529/_admin/log/level \
  -d '{"storage": "TRACE"}'

# Get current log levels
curl http://localhost:8529/_admin/log/level | jq '.'
```

---

**Last Updated:** 2026-04-06  
**Version:** 1.4.0  
**Maintainer:** ThemisDB Team
