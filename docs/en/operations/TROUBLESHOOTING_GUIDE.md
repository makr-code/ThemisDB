# ThemisDB Troubleshooting Guide

**Version:** 1.4.0  
**Last Updated:** 2026-04-06  
**Target Audience:** Site Reliability Engineers, Database Administrators, Support Engineers

---

## Table of Contents

1. [Common Issues and Solutions](#common-issues-and-solutions)
2. [Diagnostic Tools](#diagnostic-tools)
3. [Log Analysis](#log-analysis)
4. [Performance Profiling](#performance-profiling)
5. [Emergency Procedures](#emergency-procedures)
6. [Recovery Procedures](#recovery-procedures)

---

## Common Issues and Solutions

### High Memory Usage

**Symptoms:**
- Memory usage > 85% of allocated limit
- OOM (Out of Memory) errors in logs
- Slow query performance
- Connection timeouts

**Diagnostic Steps:**

```bash
#!/bin/bash
# diagnose_memory.sh - Memory usage diagnostics

echo "=== Memory Diagnostics ==="
echo ""

# 1. Check system memory
echo "1. System Memory:"
free -h
echo ""

# 2. ThemisDB process memory
echo "2. ThemisDB Process Memory:"
PID=$(pgrep themisdb)
if [ ! -z "$PID" ]; then
    ps aux | grep $PID | grep -v grep
    echo ""
    cat /proc/$PID/status | grep -E "VmSize|VmRSS|VmSwap"
fi
echo ""

# 3. Memory breakdown from metrics
echo "3. Memory Breakdown (from metrics):"
curl -s http://localhost:9091/metrics | grep themisdb_memory | grep -v "#"
echo ""

# 4. Top memory-consuming components
echo "4. Component Memory Usage:"
curl -s http://localhost:9091/metrics | grep component_memory_bytes
echo ""

# 5. Connection pool memory
echo "5. Connection Pool Memory:"
curl -s http://localhost:9091/metrics | grep connection_pool_memory
echo ""

# 6. RocksDB memory usage
echo "6. RocksDB Memory Usage:"
curl -s http://localhost:9091/metrics | grep rocksdb_memtable
curl -s http://localhost:9091/metrics | grep rocksdb_block_cache
echo ""

# 7. Transaction memory
echo "7. Transaction Memory:"
curl -s http://localhost:9091/metrics | grep transaction_memory
echo ""

# 8. Check for memory leaks
echo "8. Memory Leak Detection:"
themisdb-admin debug memory-stats --detailed
```

**Common Causes and Solutions:**

1. **Memtable Size Too Large**
   ```yaml
   # Problem: MemTables consuming excessive memory
   # Solution: Reduce memtable size
   
   # /opt/themisdb/config/themisdb.yaml
   rocksdb:
     memtable_size_mb: 128  # Reduce from 256
     max_write_buffer_number: 2  # Reduce from 3
   ```

2. **Block Cache Oversized**
   ```yaml
   # Problem: Block cache using too much memory
   # Solution: Adjust block cache size
   
   rocksdb:
     block_cache_size_mb: 2048  # Reduce from 4096
     # Or use dynamic sizing
     block_cache_use_adaptive_mutex: true
   ```

3. **Too Many Active Transactions**
   ```bash
   # Problem: Large number of long-running transactions
   # Solution: Kill old transactions
   
   # List active transactions
   themisdb-admin transaction list --older-than=300s
   
   # Kill specific transaction
   themisdb-admin transaction kill <txn_id>
   
   # Kill all transactions older than 5 minutes
   themisdb-admin transaction kill-old --older-than=300s
   ```

4. **Connection Pool Memory Leak**
   ```bash
   # Problem: Connection pool not releasing memory
   # Solution: Restart connection pool
   
   themisdb-admin connection-pool restart
   
   # Or reduce pool size temporarily
   themisdb-admin config set connections.max_pool_size 100
   ```

**Prevention:**

```yaml
# /opt/themisdb/config/themisdb.yaml
memory:
  # Set hard limits
  max_memory_bytes: 17179869184  # 16 GB
  
  # Memory pressure handling
  pressure_thresholds:
    warning: 0.80  # 80%
    critical: 0.90  # 90%
  
  # Automatic cleanup
  auto_cleanup:
    enabled: true
    trigger_at_percent: 85
    actions:
      - "flush_memtables"
      - "evict_block_cache"
      - "kill_old_transactions"
  
  # Memory limits per component
  component_limits:
    rocksdb_memtable: 4294967296  # 4 GB
    rocksdb_block_cache: 8589934592  # 8 GB
    connection_pool: 1073741824  # 1 GB
    query_cache: 2147483648  # 2 GB
```

### High CPU Usage

**Symptoms:**
- CPU usage consistently > 80%
- Slow query performance
- Increased response times
- High system load average

**Diagnostic Steps:**

```bash
#!/bin/bash
# diagnose_cpu.sh - CPU usage diagnostics

echo "=== CPU Diagnostics ==="
echo ""

# 1. System CPU usage
echo "1. System CPU Usage:"
top -bn1 | head -20
echo ""

# 2. ThemisDB process CPU
echo "2. ThemisDB Process CPU:"
PID=$(pgrep themisdb)
if [ ! -z "$PID" ]; then
    top -bn1 -p $PID
fi
echo ""

# 3. Thread-level CPU usage
echo "3. Thread-Level CPU Usage:"
if [ ! -z "$PID" ]; then
    ps -L -o pid,tid,%cpu,comm -p $PID | head -20
fi
echo ""

# 4. CPU usage from metrics
echo "4. CPU Metrics:"
curl -s http://localhost:9091/metrics | grep process_cpu
echo ""

# 5. Query execution stats
echo "5. Query Execution Stats:"
themisdb-admin query stats --top=10
echo ""

# 6. Active queries
echo "6. Currently Running Queries:"
themisdb-admin query list --running
echo ""

# 7. Compaction activity
echo "7. RocksDB Compaction Activity:"
curl -s http://localhost:9091/metrics | grep rocksdb_compaction
curl -s http://localhost:9091/metrics | grep rocksdb_num_running
echo ""

# 8. CPU profiling
echo "8. CPU Profile (10 seconds):"
themisdb-admin debug cpu-profile --duration=10s --output=/tmp/cpu_profile.txt
```

**Common Causes and Solutions:**

1. **Inefficient Queries**
   ```bash
   # Problem: Queries without proper indexes
   # Solution: Identify and optimize slow queries
   
   # Find slow queries
   themisdb-admin query analyze --slow --threshold=1000ms
   
   # Show query plan
   themisdb-admin query explain "SELECT * FROM users WHERE email = 'test@example.com'"
   
   # Create missing indexes
   themisdb-admin index create users email_idx ON users(email)
   ```

2. **Excessive Compaction**
   ```yaml
   # Problem: Too many compactions running
   # Solution: Tune compaction settings
   
   # /opt/themisdb/config/themisdb.yaml
   rocksdb:
     max_background_compactions: 2  # Reduce from 4
     max_background_flushes: 1  # Reduce from 2
     
     # Use smaller files to reduce compaction CPU
     target_file_size_base_mb: 32  # Reduce from 64
   ```

3. **High Transaction Conflict Rate**
   ```bash
   # Problem: Many transactions conflicting and retrying
   # Solution: Review transaction patterns
   
   # Check conflict rate
   themisdb-admin transaction conflicts --last=1h
   
   # Identify conflicting keys
   themisdb-admin transaction hotspots
   
   # Consider using optimistic locking or reducing transaction scope
   ```

4. **CPU-Intensive Operations**
   ```bash
   # Problem: Long-running analytical queries
   # Solution: Throttle or schedule during off-hours
   
   # Set query timeout
   themisdb-admin config set query.timeout_seconds 30
   
   # Limit concurrent queries
   themisdb-admin config set query.max_concurrent 10
   
   # Schedule maintenance during low-traffic periods
   themisdb-admin maintenance schedule --time="02:00" --tasks="compaction,vacuum"
   ```

**Prevention:**

```yaml
# /opt/themisdb/config/themisdb.yaml
performance:
  # CPU limits
  max_cpu_percent: 80
  
  # Query optimization
  query:
    timeout_seconds: 30
    max_concurrent: 20
    enable_query_cache: true
    cache_size_mb: 512
    
  # Compaction throttling
  rocksdb:
    compaction_style: "leveled"
    level_compaction_dynamic_level_bytes: true
    max_background_jobs: 3
    
  # Connection limits
  connections:
    max_active: 1000
    max_per_user: 50
```

### Slow Queries

**Symptoms:**
- Query execution time > expected
- P95/P99 latency increasing
- Timeouts on client side
- Queue buildup

**Diagnostic Steps:**

```bash
#!/bin/bash
# diagnose_slow_queries.sh

echo "=== Slow Query Diagnostics ==="
echo ""

# 1. Current slow queries
echo "1. Currently Running Slow Queries:"
themisdb-admin query list --slow --threshold=1000ms
echo ""

# 2. Slow query log analysis
echo "2. Slow Query Log (last 100):"
tail -100 /var/log/themisdb/slow_queries.log
echo ""

# 3. Query performance statistics
echo "3. Query Performance Statistics:"
themisdb-admin query stats --groupby=query_type
echo ""

# 4. Top 10 slowest queries (last hour)
echo "4. Top 10 Slowest Queries:"
themisdb-admin query top-slow --last=1h --limit=10
echo ""

# 5. Query latency percentiles
echo "5. Query Latency Percentiles:"
curl -s http://localhost:9090/api/v1/query?query='histogram_quantile(0.95, rate(themisdb_query_duration_seconds_bucket[5m]))' | jq '.data.result'
curl -s http://localhost:9090/api/v1/query?query='histogram_quantile(0.99, rate(themisdb_query_duration_seconds_bucket[5m]))' | jq '.data.result'
echo ""

# 6. Lock contention
echo "6. Lock Contention:"
themisdb-admin debug locks --blocked
echo ""

# 7. Storage I/O performance
echo "7. Storage I/O Performance:"
iostat -x 1 5 | grep -A 5 "Device"
```

**Common Causes and Solutions:**

1. **Missing Indexes**
   ```sql
   -- Problem: Full table scan on large table
   -- Solution: Add appropriate indexes
   
   -- Analyze query
   EXPLAIN SELECT * FROM orders WHERE customer_id = 12345 AND status = 'pending';
   
   -- Create composite index
   CREATE INDEX idx_orders_customer_status ON orders(customer_id, status);
   ```

2. **Large Result Sets**
   ```yaml
   # Problem: Queries returning millions of rows
   # Solution: Implement pagination and limits
   
   # /opt/themisdb/config/themisdb.yaml
   query:
     max_result_rows: 10000
     default_limit: 1000
     warn_on_large_result: 5000
   ```

3. **Lock Contention**
   ```bash
   # Problem: Queries waiting for locks
   # Solution: Identify and resolve lock contention
   
   # Show blocked queries
   themisdb-admin debug locks --blocked
   
   # Show lock holders
   themisdb-admin debug locks --holders
   
   # Kill blocking transaction
   themisdb-admin transaction kill <txn_id>
   ```

4. **Storage Bottleneck**
   ```bash
   # Problem: Slow disk I/O
   # Solution: Check and optimize storage
   
   # Check I/O statistics
   iostat -x 5 3
   
   # RocksDB statistics
   themisdb-admin rocksdb stats
   
   # Force compaction if needed
   themisdb-admin rocksdb compact --database=production_db
   
   # Consider SSD upgrade or RAID configuration change
   ```

**Query Optimization Example:**

```cpp
// Poor query (full scan)
auto results = txn->query("SELECT * FROM users WHERE email LIKE '%@example.com'");

// Optimized query (indexed scan with limit)
auto results = txn->query(
    "SELECT id, name, email FROM users "
    "WHERE email_domain = 'example.com' "  // Use indexed column
    "ORDER BY created_at DESC "
    "LIMIT 100"
);

// With proper index:
// CREATE INDEX idx_users_email_domain ON users(email_domain, created_at DESC);
```

### Replication Lag

**Symptoms:**
- Replica data behind primary
- Replication lag metrics increasing
- Stale reads from replicas
- Replication errors in logs

**Diagnostic Steps:**

```bash
#!/bin/bash
# diagnose_replication.sh

echo "=== Replication Diagnostics ==="
echo ""

# 1. Replication status
echo "1. Replication Status:"
themisdb-admin replication status
echo ""

# 2. Replication lag
echo "2. Replication Lag:"
curl -s http://localhost:9091/metrics | grep replication_lag
echo ""

# 3. Replication throughput
echo "3. Replication Throughput:"
curl -s http://localhost:9091/metrics | grep replication_bytes
echo ""

# 4. Replication errors
echo "4. Replication Errors:"
grep "replication error" /var/log/themisdb/themisdb.log | tail -20
echo ""

# 5. Network connectivity
echo "5. Network Connectivity (to replicas):"
for replica in themisdb-replica-01 themisdb-replica-02 themisdb-replica-03; do
    echo "Testing $replica:"
    ping -c 3 $replica
    echo ""
done

# 6. Replication queue size
echo "6. Replication Queue Size:"
themisdb-admin replication queue-size
echo ""

# 7. Disk I/O on replicas
echo "7. Disk I/O on Replicas:"
for replica in themisdb-replica-01 themisdb-replica-02 themisdb-replica-03; do
    echo "Replica: $replica"
    ssh $replica "iostat -x 1 3"
    echo ""
done
```

**Common Causes and Solutions:**

1. **Network Bandwidth Limitation**
   ```yaml
   # Problem: Network too slow for replication volume
   # Solution: Tune replication settings
   
   # /opt/themisdb/config/themisdb.yaml
   replication:
     # Compression
     compression:
       enabled: true
       algorithm: "zstd"
       level: 3
     
     # Batching
     batch_size: 10000
     batch_timeout_ms: 100
     
     # Network optimization
     tcp_nodelay: true
     tcp_keepalive: true
     send_buffer_size_kb: 512
     recv_buffer_size_kb: 512
   ```

2. **Replica Overloaded**
   ```bash
   # Problem: Replica can't keep up with primary
   # Solution: Scale replica resources or reduce load
   
   # Check replica resource usage
   ssh themisdb-replica-01 "top -bn1 | head -20"
   
   # Temporarily disable replication to specific replica
   themisdb-admin replication pause --replica=themisdb-replica-01
   
   # Investigate and resolve resource issues
   
   # Resume replication
   themisdb-admin replication resume --replica=themisdb-replica-01
   ```

3. **Large Transactions**
   ```yaml
   # Problem: Large transactions causing lag spikes
   # Solution: Break up large transactions
   
   # Application-level solution
   # Instead of:
   # txn.write(1000000_keys)
   
   # Use batches:
   for batch in chunks(keys, 10000):
       txn = client.begin()
       txn.write(batch)
       txn.commit()
   ```

4. **Replication Slot Full**
   ```bash
   # Problem: WAL replication slot is full
   # Solution: Clean up old WAL files
   
   # Check replication slots
   themisdb-admin replication slots
   
   # Remove inactive slots
   themisdb-admin replication remove-slot --slot=old_replica
   
   # Cleanup old WAL
   themisdb-admin wal cleanup --older-than=24h
   ```


### Data Inconsistency

**Symptoms:**
- Checksum errors in logs
- Different data on replicas vs primary
- Corruption warnings
- Unexpected query results

**Diagnostic Steps:**

```bash
#!/bin/bash
# diagnose_data_inconsistency.sh

echo "=== Data Inconsistency Diagnostics ==="
echo ""

# 1. Run database consistency check
echo "1. Database Consistency Check:"
themisdb-admin db check --database=production_db
echo ""

# 2. Compare checksums between nodes
echo "2. Checksum Comparison:"
themisdb-admin db checksum --database=production_db --table=users
echo ""

# 3. Check for corruption
echo "3. Corruption Detection:"
themisdb-admin rocksdb check --verify-checksums
echo ""

# 4. Verify replication consistency
echo "4. Replication Consistency:"
themisdb-admin replication verify --database=production_db
echo ""

# 5. Check filesystem errors
echo "5. Filesystem Errors:"
dmesg | grep -i error | tail -20
echo ""

# 6. SMART disk health
echo "6. Disk Health:"
for disk in /dev/sda /dev/sdb /dev/sdc; do
    if [ -b "$disk" ]; then
        smartctl -H $disk
    fi
done
```

**Common Causes and Solutions:**

1. **Corrupted RocksDB Files**
   ```bash
   # Problem: SST file corruption
   # Solution: Repair database
   
   # Stop ThemisDB
   systemctl stop themisdb
   
   # Backup data
   cp -r /var/lib/themisdb /var/lib/themisdb.backup.$(date +%Y%m%d_%H%M%S)
   
   # Repair database
   themisdb-admin rocksdb repair --data-dir=/var/lib/themisdb
   
   # Verify repair
   themisdb-admin db check
   
   # Start ThemisDB
   systemctl start themisdb
   ```

2. **Split Brain (Replication)**
   ```bash
   # Problem: Primary and replicas diverged
   # Solution: Re-synchronize replicas
   
   # Identify primary
   themisdb-admin cluster status
   
   # Stop replica
   ssh themisdb-replica-01 "systemctl stop themisdb"
   
   # Remove replica data
   ssh themisdb-replica-01 "rm -rf /var/lib/themisdb/*"
   
   # Re-initialize from primary
   themisdb-admin replication rebuild --replica=themisdb-replica-01 --from-primary
   
   # Start replica
   ssh themisdb-replica-01 "systemctl start themisdb"
   
   # Verify replication
   themisdb-admin replication verify
   ```

3. **Disk Corruption**
   ```bash
   # Problem: Underlying disk corruption
   # Solution: Restore from backup
   
   # Check disk errors
   dmesg | grep -i "I/O error"
   smartctl -a /dev/sda
   
   # If disk is failing:
   # 1. Stop ThemisDB
   systemctl stop themisdb
   
   # 2. Replace disk (if hardware)
   # 3. Restore from backup
   themisdb-admin backup restore \
       --source=/mnt/backup/themisdb/full-20260118 \
       --destination=/var/lib/themisdb \
       --verify
   
   # 4. Start ThemisDB
   systemctl start themisdb
   ```

### Connection Pool Exhaustion

**Symptoms:**
- "Connection pool exhausted" errors
- Client timeouts
- New connections rejected
- High connection wait times

**Diagnostic Steps:**

```bash
#!/bin/bash
# diagnose_connections.sh

echo "=== Connection Pool Diagnostics ==="
echo ""

# 1. Active connections
echo "1. Active Connections:"
themisdb-admin connection list --active
echo ""

# 2. Connection pool statistics
echo "2. Connection Pool Stats:"
curl -s http://localhost:9091/metrics | grep themisdb_connections
echo ""

# 3. Connections by user
echo "3. Connections by User:"
themisdb-admin connection list --groupby=user
echo ""

# 4. Idle connections
echo "4. Idle Connections:"
themisdb-admin connection list --idle --older-than=300s
echo ""

# 5. Network connections
echo "5. Network Connections (netstat):"
netstat -an | grep :7700 | awk '{print $6}' | sort | uniq -c
echo ""

# 6. Connection errors
echo "6. Connection Errors:"
grep "connection" /var/log/themisdb/themisdb.log | grep -i error | tail -20
```

**Common Causes and Solutions:**

1. **Connection Leaks in Application**
   ```python
   # Problem: Application not closing connections
   # Bad code:
   client = ThemisDBClient()
   client.connect()
   result = client.query("SELECT * FROM users")
   # Connection never closed!
   
   # Good code:
   with ThemisDBClient() as client:
       result = client.query("SELECT * FROM users")
   # Connection automatically closed
   
   # Or explicitly:
   client = ThemisDBClient()
   try:
       client.connect()
       result = client.query("SELECT * FROM users")
   finally:
       client.close()
   ```

2. **Pool Size Too Small**
   ```yaml
   # Problem: Not enough connections for load
   # Solution: Increase pool size
   
   # /opt/themisdb/config/themisdb.yaml
   connections:
     max_pool_size: 2000  # Increase from 1000
     max_per_user: 100  # Increase from 50
     
     # Connection timeouts
     idle_timeout_seconds: 300
     max_lifetime_seconds: 3600
     
     # Wait queue
     max_wait_queue: 1000
     wait_timeout_seconds: 30
   ```

3. **Zombie Connections**
   ```bash
   # Problem: Connections stuck in bad state
   # Solution: Kill zombie connections
   
   # List zombie connections
   themisdb-admin connection list --zombie
   
   # Kill specific connection
   themisdb-admin connection kill <connection_id>
   
   # Kill all idle connections > 5 minutes
   themisdb-admin connection kill-idle --older-than=300s
   
   # Restart connection pool
   themisdb-admin connection-pool restart
   ```

### Transaction Timeouts

**Symptoms:**
- "Transaction timeout" errors
- Aborted transactions
- Deadlocks
- Lock wait timeouts

**Diagnostic Steps:**

```bash
#!/bin/bash
# diagnose_transaction_timeouts.sh

echo "=== Transaction Timeout Diagnostics ==="
echo ""

# 1. Active transactions
echo "1. Active Transactions:"
themisdb-admin transaction list --active
echo ""

# 2. Long-running transactions
echo "2. Long-Running Transactions:"
themisdb-admin transaction list --older-than=30s
echo ""

# 3. Transaction conflicts
echo "3. Transaction Conflicts (last hour):"
themisdb-admin transaction conflicts --last=1h
echo ""

# 4. Deadlocks
echo "4. Recent Deadlocks:"
themisdb-admin transaction deadlocks --last=24h
echo ""

# 5. Lock wait statistics
echo "5. Lock Wait Statistics:"
curl -s http://localhost:9091/metrics | grep lock_wait
echo ""

# 6. Transaction queue
echo "6. Transaction Queue:"
themisdb-admin transaction queue-depth
```

**Common Causes and Solutions:**

1. **Long-Running Transactions**
   ```bash
   # Problem: Transactions held open too long
   # Solution: Set appropriate timeouts
   
   # /opt/themisdb/config/themisdb.yaml
   transactions:
     default_timeout_seconds: 30
     max_timeout_seconds: 300
     idle_timeout_seconds: 60
     
     # Warning thresholds
     warn_threshold_seconds: 10
     
     # Auto-abort old transactions
     auto_abort:
       enabled: true
       threshold_seconds: 300
   ```

2. **Deadlock Detection**
   ```bash
   # Problem: Circular lock dependencies
   # Solution: Deadlock detection and resolution
   
   # Enable deadlock detection
   themisdb-admin config set transactions.deadlock_detection true
   themisdb-admin config set transactions.deadlock_timeout_ms 1000
   
   # View deadlock graph
   themisdb-admin transaction deadlock-graph
   
   # Application fix: Always acquire locks in same order
   # Bad:
   txn1: lock(A), lock(B)
   txn2: lock(B), lock(A)  # Deadlock!
   
   # Good:
   txn1: lock(A), lock(B)
   txn2: lock(A), lock(B)  # Consistent order
   ```

3. **Hot Spot Contention**
   ```bash
   # Problem: Many transactions accessing same keys
   # Solution: Identify and address hot spots
   
   # Find hot spot keys
   themisdb-admin transaction hotspots --top=20
   
   # Consider:
   # - Sharding the hot key
   # - Using optimistic locking
   # - Caching frequently read data
   # - Batching updates
   ```

---

## Diagnostic Tools

### Built-in Diagnostic Commands

**ThemisDB Admin Tool:**

```bash
#!/bin/bash
# themisdb_diagnostic_suite.sh - Comprehensive diagnostics

echo "=== ThemisDB Diagnostic Suite ==="
echo ""

# System information
echo "1. System Information:"
themisdb-admin system info
echo ""

# Cluster status
echo "2. Cluster Status:"
themisdb-admin cluster status --detailed
echo ""

# Database statistics
echo "3. Database Statistics:"
themisdb-admin db stats --all
echo ""

# Performance metrics
echo "4. Performance Metrics:"
themisdb-admin metrics snapshot
echo ""

# Health check
echo "5. Health Check:"
themisdb-admin health check --all-components
echo ""

# Resource usage
echo "6. Resource Usage:"
themisdb-admin resources usage
echo ""

# Recent errors
echo "7. Recent Errors:"
themisdb-admin logs errors --last=1h --limit=50
echo ""

# Configuration validation
echo "8. Configuration Validation:"
themisdb-admin config validate
```

**RocksDB Statistics:**

```bash
#!/bin/bash
# rocksdb_diagnostics.sh

echo "=== RocksDB Diagnostics ==="
echo ""

# 1. General statistics
echo "1. General Statistics:"
themisdb-admin rocksdb stats
echo ""

# 2. Level statistics
echo "2. LSM Level Statistics:"
themisdb-admin rocksdb levels
echo ""

# 3. Compaction statistics
echo "3. Compaction Statistics:"
themisdb-admin rocksdb compaction-stats
echo ""

# 4. Memory usage
echo "4. Memory Usage:"
themisdb-admin rocksdb memory-usage
echo ""

# 5. SST file information
echo "5. SST File Information:"
themisdb-admin rocksdb sst-files --database=production_db
echo ""

# 6. Property values
echo "6. RocksDB Properties:"
for prop in rocksdb.estimate-num-keys \
            rocksdb.total-sst-files-size \
            rocksdb.live-sst-files-size \
            rocksdb.estimate-pending-compaction-bytes \
            rocksdb.num-running-compactions \
            rocksdb.num-running-flushes; do
    echo "$prop:"
    themisdb-admin rocksdb property --name=$prop --database=production_db
done
```

### Thread Analysis

**Thread Dump and Analysis:**

```bash
#!/bin/bash
# thread_analysis.sh - Analyze thread activity

echo "=== Thread Analysis ==="
echo ""

# 1. Thread dump
echo "1. Thread Dump:"
PID=$(pgrep themisdb)
kill -SIGQUIT $PID  # Generates thread dump in log
sleep 2
tail -500 /var/log/themisdb/themisdb.log | grep -A 20 "Thread Dump"
echo ""

# 2. Thread count
echo "2. Thread Count:"
ps -L -p $PID | wc -l
echo ""

# 3. Top CPU-consuming threads
echo "3. Top CPU-Consuming Threads:"
ps -L -o pid,tid,%cpu,comm -p $PID | sort -k3 -nr | head -10
echo ""

# 4. Thread states
echo "4. Thread States:"
cat /proc/$PID/status | grep Threads
for tid in $(ls /proc/$PID/task); do
    cat /proc/$PID/task/$tid/status | grep State
done | sort | uniq -c
echo ""

# 5. Stack traces (requires perf)
echo "5. Stack Traces (sampling):"
if command -v perf &> /dev/null; then
    perf record -p $PID -g -- sleep 10
    perf report --stdio | head -100
fi
```

### Memory Profiling

**Memory Leak Detection:**

```bash
#!/bin/bash
# memory_profiling.sh - Detect memory leaks

echo "=== Memory Profiling ==="
echo ""

# 1. Initial snapshot
echo "1. Memory Snapshot (initial):"
themisdb-admin debug memory-snapshot --output=/tmp/mem_snapshot_1.txt
INITIAL_MEM=$(grep "Total Memory" /tmp/mem_snapshot_1.txt | awk '{print $3}')
echo "Initial memory: $INITIAL_MEM MB"
echo ""

# 2. Wait and take another snapshot
echo "2. Waiting 5 minutes..."
sleep 300

echo "3. Memory Snapshot (after 5min):"
themisdb-admin debug memory-snapshot --output=/tmp/mem_snapshot_2.txt
FINAL_MEM=$(grep "Total Memory" /tmp/mem_snapshot_2.txt | awk '{print $3}')
echo "Final memory: $FINAL_MEM MB"
echo ""

# 3. Calculate growth
GROWTH=$(echo "$FINAL_MEM - $INITIAL_MEM" | bc)
echo "4. Memory Growth: $GROWTH MB"
echo ""

# 4. Component breakdown
echo "5. Memory Growth by Component:"
diff /tmp/mem_snapshot_1.txt /tmp/mem_snapshot_2.txt | grep "^>" | grep -v "Total"
echo ""

# 5. Heap profile (if enabled)
if [ -f "/tmp/themisdb_heap.prof" ]; then
    echo "6. Heap Profile Analysis:"
    pprof --text /opt/themisdb/bin/themisdb /tmp/themisdb_heap.prof | head -50
fi
```

**Valgrind Memory Check:**

```bash
#!/bin/bash
# valgrind_check.sh - Memory leak detection with Valgrind

# WARNING: This will significantly slow down ThemisDB
# Only use in non-production environments

echo "Running Valgrind memory check..."
echo "This will take a while..."

valgrind \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --verbose \
    --log-file=/tmp/valgrind_themisdb.log \
    /opt/themisdb/bin/themisdb --config=/opt/themisdb/config/themisdb.yaml &

VALGRIND_PID=$!

# Let it run for 10 minutes
sleep 600

# Stop Valgrind
kill -SIGTERM $VALGRIND_PID

# Analyze results
echo ""
echo "Valgrind Results:"
grep "definitely lost" /tmp/valgrind_themisdb.log
grep "indirectly lost" /tmp/valgrind_themisdb.log
grep "possibly lost" /tmp/valgrind_themisdb.log

echo ""
echo "Full report: /tmp/valgrind_themisdb.log"
```

### Network Diagnostics

**Network Performance Analysis:**

```bash
#!/bin/bash
# network_diagnostics.sh

echo "=== Network Diagnostics ==="
echo ""

# 1. Network connections
echo "1. Network Connections:"
netstat -an | grep :7700 | head -20
echo ""

# 2. Connection statistics
echo "2. Connection Statistics:"
ss -s
echo ""

# 3. Bandwidth usage
echo "3. Bandwidth Usage (5 seconds):"
ifstat -i eth0 5 1
echo ""

# 4. Network errors
echo "4. Network Errors:"
netstat -i
echo ""

# 5. TCP statistics
echo "5. TCP Statistics:"
cat /proc/net/snmp | grep "Tcp:"
echo ""

# 6. Connection latency
echo "6. Connection Latency Test:"
for node in themisdb-server-01 themisdb-server-02 themisdb-server-03; do
    echo "Testing $node:"
    ping -c 5 $node | tail -2
    echo ""
done

# 7. Packet loss
echo "7. Packet Loss Test:"
for node in themisdb-server-01 themisdb-server-02 themisdb-server-03; do
    echo "Testing $node:"
    ping -c 100 -i 0.2 $node | grep "packet loss"
done
```

---

## Log Analysis

### Log Level Configuration

**Dynamic Log Level Adjustment:**

```bash
#!/bin/bash
# adjust_log_level.sh - Change log levels dynamically

# Enable debug logging for specific component
themisdb-admin logging set-level --component=transactions --level=debug

# Enable trace logging (very verbose)
themisdb-admin logging set-level --component=storage --level=trace

# Reset to default
themisdb-admin logging set-level --component=transactions --level=info

# Global log level change
themisdb-admin logging set-level --global --level=warn
```

### Error Pattern Analysis

**Common Error Patterns:**

```bash
#!/bin/bash
# analyze_error_patterns.sh

echo "=== Error Pattern Analysis ==="
echo ""

LOG_FILE="/var/log/themisdb/themisdb.log"

# 1. Error frequency by type
echo "1. Error Frequency by Type:"
grep "ERROR" $LOG_FILE | \
    awk '{print $5}' | \
    sort | uniq -c | sort -rn | head -20
echo ""

# 2. Timeline of errors (last 24h)
echo "2. Error Timeline (last 24h):"
grep "ERROR" $LOG_FILE | \
    grep "$(date -d '24 hours ago' +%Y-%m-%d)" | \
    awk '{print $1" "$2}' | \
    cut -d: -f1-2 | \
    uniq -c
echo ""

# 3. Errors by component
echo "3. Errors by Component:"
grep "ERROR" $LOG_FILE | \
    grep -oP '"component":"[^"]*"' | \
    sort | uniq -c | sort -rn
echo ""

# 4. Stack trace analysis
echo "4. Most Common Stack Traces:"
grep -A 10 "ERROR" $LOG_FILE | \
    grep "at " | \
    sort | uniq -c | sort -rn | head -20
echo ""

# 5. Connection errors
echo "5. Connection Error Summary:"
grep "connection" $LOG_FILE | grep -i error | wc -l
echo ""

# 6. Transaction errors
echo "6. Transaction Error Summary:"
grep "transaction" $LOG_FILE | grep -i error | \
    grep -oP '"error_type":"[^"]*"' | \
    sort | uniq -c
```


### Slow Query Log Analysis

**Analyzing Slow Queries:**

```bash
#!/bin/bash
# analyze_slow_queries.sh

SLOW_LOG="/var/log/themisdb/slow_queries.log"

echo "=== Slow Query Analysis ==="
echo ""

# 1. Slowest queries
echo "1. Top 10 Slowest Queries:"
jq -r '[.query.text, .duration_ms] | @tsv' $SLOW_LOG | \
    sort -t$'\t' -k2 -nr | \
    head -10
echo ""

# 2. Most frequent slow queries
echo "2. Most Frequent Slow Queries:"
jq -r '.query.text' $SLOW_LOG | \
    sort | uniq -c | sort -rn | head -10
echo ""

# 3. Slow queries by database
echo "3. Slow Queries by Database:"
jq -r '.database' $SLOW_LOG | \
    sort | uniq -c | sort -rn
echo ""

# 4. Average duration by query type
echo "4. Average Duration by Query Type:"
jq -r '[.query.type, .duration_ms] | @tsv' $SLOW_LOG | \
    awk '{sum[$1]+=$2; count[$1]++} END {for (type in sum) print type, sum[type]/count[type]}' | \
    sort -k2 -nr
echo ""

# 5. Queries without indexes
echo "5. Queries Without Indexes (full scans):"
jq 'select(.query.full_scan == true) | .query.text' $SLOW_LOG | \
    sort | uniq -c | sort -rn
```

---

## Performance Profiling

### CPU Profiling

**Continuous CPU Profiling:**

```bash
#!/bin/bash
# cpu_profiling.sh - CPU performance profiling

echo "=== CPU Profiling ==="
echo ""

PID=$(pgrep themisdb)

# 1. Enable built-in CPU profiler
echo "1. Enabling CPU Profiler (60 seconds):"
themisdb-admin debug cpu-profile start --output=/tmp/cpu_profile.txt
sleep 60
themisdb-admin debug cpu-profile stop
echo ""

# 2. Analyze profile
echo "2. CPU Profile Analysis:"
cat /tmp/cpu_profile.txt | head -50
echo ""

# 3. Function call breakdown
echo "3. Top CPU-Consuming Functions:"
grep -E "^\s+[0-9]+" /tmp/cpu_profile.txt | \
    sort -k1 -rn | head -20
echo ""

# 4. Flamegraph generation (if available)
if command -v flamegraph &> /dev/null; then
    echo "4. Generating Flamegraph:"
    perf record -p $PID -g -F 99 -- sleep 30
    perf script | flamegraph.pl > /tmp/themisdb_flamegraph.svg
    echo "Flamegraph saved to: /tmp/themisdb_flamegraph.svg"
fi
```

**perf Tools Analysis:**

```bash
#!/bin/bash
# perf_analysis.sh - Linux perf tools analysis

PID=$(pgrep themisdb)

# CPU usage analysis
echo "=== perf Analysis ==="
echo ""

# 1. Record CPU events
echo "1. Recording CPU events (30 seconds):"
perf record -p $PID -g -F 99 -- sleep 30
echo ""

# 2. Report
echo "2. perf Report:"
perf report --stdio | head -100
echo ""

# 3. Top functions
echo "3. Top Functions by CPU:"
perf report --stdio --sort=dso,symbol | head -50
echo ""

# 4. Call graph
echo "4. Call Graph (top callers):"
perf report --stdio -g graph,0.5,caller | head -100
```

### Query Plan Analysis

**Query Execution Plan:**

```sql
-- Enable query plan logging
SET debug_query_plans = true;

-- Analyze specific query
EXPLAIN ANALYZE 
SELECT u.name, o.total 
FROM users u 
JOIN orders o ON u.id = o.user_id 
WHERE u.status = 'active' AND o.created_at > '2026-01-01';

-- Sample output:
-- Query Plan:
--   -> Nested Loop Join (cost=1000.50 rows=5000)
--      -> Index Scan on users using idx_users_status (cost=0.42 rows=10000)
--           Index Cond: (status = 'active')
--           Rows: 10000
--           Time: 15ms
--      -> Index Scan on orders using idx_orders_user_created (cost=0.43 rows=50)
--           Index Cond: (user_id = users.id AND created_at > '2026-01-01')
--           Rows per join: 0.5
--           Time: 2ms per join
--   Total Time: 25ms
--   Estimated Rows: 5000
--   Actual Rows: 4832
```

### Transaction Analysis

**Transaction Performance Profiling:**

```bash
#!/bin/bash
# transaction_profiling.sh

echo "=== Transaction Performance Profiling ==="
echo ""

# 1. Transaction duration distribution
echo "1. Transaction Duration Distribution:"
curl -s http://localhost:9090/api/v1/query?query='histogram_quantile(0.50, rate(themisdb_transaction_duration_seconds_bucket[5m]))' | jq -r '.data.result[0].value[1]'
curl -s http://localhost:9090/api/v1/query?query='histogram_quantile(0.95, rate(themisdb_transaction_duration_seconds_bucket[5m]))' | jq -r '.data.result[0].value[1]'
curl -s http://localhost:9090/api/v1/query?query='histogram_quantile(0.99, rate(themisdb_transaction_duration_seconds_bucket[5m]))' | jq -r '.data.result[0].value[1]'
echo ""

# 2. Transaction conflict analysis
echo "2. Transaction Conflicts:"
themisdb-admin transaction conflicts --detailed --last=1h
echo ""

# 3. Lock contention hotspots
echo "3. Lock Contention Hotspots:"
themisdb-admin transaction hotspots --by-key --top=20
echo ""

# 4. Transaction size distribution
echo "4. Transaction Size Distribution:"
themisdb-admin transaction size-distribution
```

### Storage Performance Analysis

**I/O Performance Testing:**

```bash
#!/bin/bash
# storage_performance_test.sh

echo "=== Storage Performance Testing ==="
echo ""

DATA_DIR="/var/lib/themisdb"

# 1. Sequential write test
echo "1. Sequential Write Performance:"
dd if=/dev/zero of=$DATA_DIR/test_seq_write bs=1M count=1024 conv=fdatasync
rm -f $DATA_DIR/test_seq_write
echo ""

# 2. Sequential read test
echo "2. Sequential Read Performance:"
dd if=$DATA_DIR/test_seq_write of=/dev/null bs=1M count=1024
echo ""

# 3. Random write test (fio)
if command -v fio &> /dev/null; then
    echo "3. Random Write Performance (4K blocks):"
    fio --name=random_write \
        --ioengine=libaio \
        --iodepth=32 \
        --rw=randwrite \
        --bs=4k \
        --direct=1 \
        --size=1G \
        --numjobs=4 \
        --runtime=60 \
        --group_reporting \
        --filename=$DATA_DIR/fio_test
    rm -f $DATA_DIR/fio_test
    echo ""
    
    # 4. Random read test
    echo "4. Random Read Performance (4K blocks):"
    fio --name=random_read \
        --ioengine=libaio \
        --iodepth=32 \
        --rw=randread \
        --bs=4k \
        --direct=1 \
        --size=1G \
        --numjobs=4 \
        --runtime=60 \
        --group_reporting \
        --filename=$DATA_DIR/fio_test
    rm -f $DATA_DIR/fio_test
fi
```

**RocksDB Compaction Analysis:**

```bash
#!/bin/bash
# compaction_analysis.sh

echo "=== RocksDB Compaction Analysis ==="
echo ""

# 1. Compaction statistics
echo "1. Compaction Statistics:"
themisdb-admin rocksdb compaction-stats --detailed
echo ""

# 2. Level statistics
echo "2. LSM Level Statistics:"
themisdb-admin rocksdb levels --with-sizes
echo ""

# 3. Write amplification
echo "3. Write Amplification:"
themisdb-admin rocksdb write-amp
echo ""

# 4. Space amplification
echo "4. Space Amplification:"
themisdb-admin rocksdb space-amp
echo ""

# 5. Pending compactions
echo "5. Pending Compactions:"
curl -s http://localhost:9091/metrics | grep rocksdb_compaction_pending
echo ""

# 6. Compaction history
echo "6. Recent Compaction History:"
grep "compaction" /var/log/themisdb/themisdb.log | tail -50
```

---

## Emergency Procedures

### Emergency Shutdown

**Safe Emergency Shutdown:**

```bash
#!/bin/bash
# emergency_shutdown.sh - Safe shutdown procedure

echo "=== Emergency Shutdown Procedure ==="
echo ""

# 1. Enable read-only mode
echo "Step 1: Enabling read-only mode..."
themisdb-admin maintenance read-only-mode enable
sleep 5
echo ""

# 2. Wait for active transactions to complete
echo "Step 2: Waiting for active transactions..."
TIMEOUT=60
ELAPSED=0
while [ $(themisdb-admin transaction count --active) -gt 0 ] && [ $ELAPSED -lt $TIMEOUT ]; do
    echo "Active transactions: $(themisdb-admin transaction count --active)"
    sleep 5
    ELAPSED=$((ELAPSED + 5))
done

if [ $(themisdb-admin transaction count --active) -gt 0 ]; then
    echo "Warning: Timeout waiting for transactions. Force aborting..."
    themisdb-admin transaction abort-all
fi
echo ""

# 3. Flush memtables
echo "Step 3: Flushing memtables..."
themisdb-admin rocksdb flush --all-databases
echo ""

# 4. Stop accepting connections
echo "Step 4: Stopping new connections..."
themisdb-admin connection reject-new
echo ""

# 5. Close existing connections gracefully
echo "Step 5: Closing existing connections..."
themisdb-admin connection close-all --graceful
sleep 10
echo ""

# 6. Stop service
echo "Step 6: Stopping ThemisDB service..."
systemctl stop themisdb
echo ""

# 7. Verify shutdown
echo "Step 7: Verifying shutdown..."
if pgrep themisdb > /dev/null; then
    echo "Warning: ThemisDB still running. Force killing..."
    pkill -9 themisdb
fi

echo ""
echo "✓ Emergency shutdown complete"
```

### Force Restart

**Force Restart with Recovery:**

```bash
#!/bin/bash
# force_restart.sh - Force restart with recovery checks

echo "=== Force Restart Procedure ==="
echo ""

# 1. Kill ThemisDB process
echo "Step 1: Killing ThemisDB process..."
pkill -9 themisdb
sleep 2
echo ""

# 2. Check for corrupted data
echo "Step 2: Checking for data corruption..."
themisdb-admin db check --quick
if [ $? -ne 0 ]; then
    echo "Warning: Corruption detected. Running repair..."
    themisdb-admin db repair
fi
echo ""

# 3. Clear lock files
echo "Step 3: Clearing lock files..."
rm -f /var/lib/themisdb/*.lock
echo ""

# 4. Verify disk space
echo "Step 4: Checking disk space..."
df -h /var/lib/themisdb
echo ""

# 5. Start service
echo "Step 5: Starting ThemisDB service..."
systemctl start themisdb
sleep 10
echo ""

# 6. Wait for ready state
echo "Step 6: Waiting for ready state..."
TIMEOUT=300
ELAPSED=0
while ! curl -sf http://localhost:8080/ready > /dev/null && [ $ELAPSED -lt $TIMEOUT ]; do
    echo "Waiting for ThemisDB to be ready..."
    sleep 5
    ELAPSED=$((ELAPSED + 5))
done

if curl -sf http://localhost:8080/ready > /dev/null; then
    echo "✓ ThemisDB is ready"
else
    echo "✗ ThemisDB failed to start within timeout"
    echo "Check logs: tail -100 /var/log/themisdb/themisdb.log"
    exit 1
fi
echo ""

# 7. Verify replication
echo "Step 7: Verifying replication..."
themisdb-admin replication status
echo ""

echo "✓ Force restart complete"
```

### Data Recovery

**Emergency Data Recovery:**

```bash
#!/bin/bash
# emergency_recovery.sh - Emergency data recovery

echo "=== Emergency Data Recovery ==="
echo ""

# 1. Stop service
echo "Step 1: Stopping ThemisDB..."
systemctl stop themisdb
echo ""

# 2. Backup current state (even if corrupted)
echo "Step 2: Backing up current state..."
BACKUP_DIR="/var/lib/themisdb.emergency.$(date +%Y%m%d_%H%M%S)"
cp -r /var/lib/themisdb $BACKUP_DIR
echo "Backup saved to: $BACKUP_DIR"
echo ""

# 3. Attempt RocksDB repair
echo "Step 3: Attempting RocksDB repair..."
themisdb-admin rocksdb repair --data-dir=/var/lib/themisdb
echo ""

# 4. If repair fails, restore from backup
if [ $? -ne 0 ]; then
    echo "Repair failed. Attempting restore from backup..."
    
    # Find latest backup
    LATEST_BACKUP=$(ls -t /mnt/backup/themisdb/full-* | head -1)
    
    if [ ! -z "$LATEST_BACKUP" ]; then
        echo "Restoring from: $LATEST_BACKUP"
        
        # Remove corrupted data
        rm -rf /var/lib/themisdb/*
        
        # Restore
        themisdb-admin backup restore \
            --source=$LATEST_BACKUP \
            --destination=/var/lib/themisdb \
            --verify
    else
        echo "No backup found!"
        exit 1
    fi
fi
echo ""

# 5. Verify database consistency
echo "Step 5: Verifying database consistency..."
themisdb-admin db check --thorough
echo ""

# 6. Start service
echo "Step 6: Starting ThemisDB..."
systemctl start themisdb
echo ""

# 7. Verify functionality
echo "Step 7: Verifying functionality..."
sleep 10
themisdb-admin health check --all-components
echo ""

echo "✓ Emergency recovery complete"
```

### Split-Brain Resolution

**Resolving Split-Brain Scenario:**

```bash
#!/bin/bash
# resolve_split_brain.sh - Resolve split-brain in cluster

echo "=== Split-Brain Resolution ==="
echo ""

# 1. Identify cluster state
echo "Step 1: Identifying cluster state..."
echo "Cluster status:"
themisdb-admin cluster status --all-nodes
echo ""

# 2. Determine canonical primary
echo "Step 2: Determining canonical primary..."
echo "Enter the hostname of the node that should be primary:"
read PRIMARY_NODE
echo ""

# 3. Stop all secondary nodes
echo "Step 3: Stopping all secondary nodes..."
for node in themisdb-server-01 themisdb-server-02 themisdb-server-03; do
    if [ "$node" != "$PRIMARY_NODE" ]; then
        echo "Stopping $node..."
        ssh $node "systemctl stop themisdb"
    fi
done
echo ""

# 4. Verify primary is healthy
echo "Step 4: Verifying primary health..."
ssh $PRIMARY_NODE "themisdb-admin health check"
echo ""

# 5. Rebuild secondary nodes
echo "Step 5: Rebuilding secondary nodes from primary..."
for node in themisdb-server-01 themisdb-server-02 themisdb-server-03; do
    if [ "$node" != "$PRIMARY_NODE" ]; then
        echo "Rebuilding $node..."
        
        # Remove old data
        ssh $node "rm -rf /var/lib/themisdb/*"
        
        # Re-sync from primary
        themisdb-admin replication rebuild \
            --replica=$node \
            --from-primary=$PRIMARY_NODE
        
        # Start node
        ssh $node "systemctl start themisdb"
        
        # Wait for replication to catch up
        sleep 30
    fi
done
echo ""

# 6. Verify cluster health
echo "Step 6: Verifying cluster health..."
themisdb-admin cluster status --all-nodes
themisdb-admin replication verify --all
echo ""

echo "✓ Split-brain resolution complete"
```

---

## Recovery Procedures

### Point-in-Time Recovery

**Restore to Specific Point in Time:**

```bash
#!/bin/bash
# point_in_time_recovery.sh - PITR procedure

TARGET_TIMESTAMP="$1"

if [ -z "$TARGET_TIMESTAMP" ]; then
    echo "Usage: $0 <timestamp>"
    echo "Example: $0 '2026-01-18 10:30:00'"
    exit 1
fi

echo "=== Point-in-Time Recovery ==="
echo "Target timestamp: $TARGET_TIMESTAMP"
echo ""

# 1. Stop service
echo "Step 1: Stopping ThemisDB..."
systemctl stop themisdb
echo ""

# 2. Backup current state
echo "Step 2: Backing up current state..."
BACKUP_DIR="/var/lib/themisdb.pitr.$(date +%Y%m%d_%H%M%S)"
cp -r /var/lib/themisdb $BACKUP_DIR
echo "Backup saved to: $BACKUP_DIR"
echo ""

# 3. Find appropriate backup
echo "Step 3: Finding backup before target timestamp..."
BACKUP_TO_RESTORE=$(themisdb-admin backup list --before="$TARGET_TIMESTAMP" --latest)
echo "Using backup: $BACKUP_TO_RESTORE"
echo ""

# 4. Restore base backup
echo "Step 4: Restoring base backup..."
rm -rf /var/lib/themisdb/*
themisdb-admin backup restore \
    --source=$BACKUP_TO_RESTORE \
    --destination=/var/lib/themisdb
echo ""

# 5. Apply WAL logs up to target time
echo "Step 5: Applying WAL logs to target timestamp..."
themisdb-admin wal replay \
    --until="$TARGET_TIMESTAMP" \
    --data-dir=/var/lib/themisdb
echo ""

# 6. Verify consistency
echo "Step 6: Verifying database consistency..."
themisdb-admin db check
echo ""

# 7. Start service
echo "Step 7: Starting ThemisDB..."
systemctl start themisdb
sleep 10
echo ""

# 8. Verify recovery
echo "Step 8: Verifying recovery..."
themisdb-admin health check
echo ""

echo "✓ Point-in-time recovery complete"
echo "Database restored to: $TARGET_TIMESTAMP"
```

### Disaster Recovery

**Full Disaster Recovery Procedure:**

```bash
#!/bin/bash
# disaster_recovery.sh - Complete disaster recovery

echo "=== Disaster Recovery Procedure ==="
echo ""

# 1. Provision new hardware/VMs
echo "Step 1: Ensure new hardware is provisioned"
echo "Press Enter when ready..."
read
echo ""

# 2. Install ThemisDB
echo "Step 2: Installing ThemisDB..."
./install_themisdb.sh
echo ""

# 3. Restore configuration
echo "Step 3: Restoring configuration from backup..."
aws s3 cp s3://disaster-recovery/themisdb/config/ /opt/themisdb/config/ --recursive
echo ""

# 4. Restore encryption keys
echo "Step 4: Restoring encryption keys..."
aws s3 cp s3://disaster-recovery/themisdb/keys/ /opt/themisdb/keys/ --recursive
chmod 400 /opt/themisdb/keys/*
echo ""

# 5. Find latest backup
echo "Step 5: Finding latest backup..."
LATEST_BACKUP=$(aws s3 ls s3://disaster-recovery/themisdb/backups/ | sort | tail -1 | awk '{print $4}')
echo "Latest backup: $LATEST_BACKUP"
echo ""

# 6. Restore data
echo "Step 6: Restoring data from backup..."
aws s3 cp s3://disaster-recovery/themisdb/backups/$LATEST_BACKUP /tmp/backup.tar.gz
tar xzf /tmp/backup.tar.gz -C /var/lib/themisdb/
echo ""

# 7. Verify data integrity
echo "Step 7: Verifying data integrity..."
themisdb-admin db check --thorough
echo ""

# 8. Start service
echo "Step 8: Starting ThemisDB..."
systemctl start themisdb
sleep 15
echo ""

# 9. Verify cluster formation
echo "Step 9: Verifying cluster..."
themisdb-admin cluster status
echo ""

# 10. Setup replication
echo "Step 10: Setting up replication..."
themisdb-admin replication setup --topology=primary-replica
echo ""

# 11. Verify application connectivity
echo "Step 11: Testing application connectivity..."
themisdb-admin connection test --from-app-server
echo ""

# 12. Update DNS/Load Balancer
echo "Step 12: Update DNS/Load Balancer entries"
echo "New cluster IPs:"
themisdb-admin cluster list-nodes --ip-only
echo "Update and press Enter when done..."
read
echo ""

echo "✓ Disaster recovery complete"
echo "Cluster is operational and ready for traffic"
```

---

## Related Documentation

- **[Production Deployment Guide](../deployment/PRODUCTION_DEPLOYMENT_GUIDE.md)** - Initial deployment
- **[Operational Procedures](OPERATIONAL_PROCEDURES.md)** - Day-to-day operations
- **[Monitoring Setup Guide](MONITORING_SETUP_GUIDE.md)** - Monitoring configuration
- **[Security Deployment Guide](../security/SECURITY_DEPLOYMENT_GUIDE.md)** - Security procedures
- **[MVCC Tuning Guide](../features/MVCC_TUNING_GUIDE.md)** - Transaction tuning
- **[RocksDB Optimization Guide](../storage/ROCKSDB_OPTIMIZATION_GUIDE.md)** - Storage optimization
- **[Transaction Best Practices](../features/TRANSACTION_BEST_PRACTICES.md)** - Transaction patterns

---

**Document Version:** 1.0  
**ThemisDB Compatibility:** 1.4.0+  
**Last Reviewed:** 2026-01-18  
**Next Review:** 2026-04-18

---

## Quick Reference

### Critical Commands

```bash
# Emergency stop
systemctl stop themisdb

# Health check
curl http://localhost:8080/health

# Active transactions
themisdb-admin transaction list --active

# Kill long transaction
themisdb-admin transaction kill <txn_id>

# Database check
themisdb-admin db check

# View logs
tail -f /var/log/themisdb/themisdb.log

# Metrics snapshot
curl http://localhost:9091/metrics

# Restart service
systemctl restart themisdb
```

### Emergency Contacts

| Issue Type | Contact | Response Time |
|------------|---------|---------------|
| **P1 - Critical Outage** | ops-oncall@example.com | 15 minutes |
| **P2 - Degraded Performance** | dba-team@example.com | 1 hour |
| **P3 - Non-urgent** | support@example.com | Next business day |

