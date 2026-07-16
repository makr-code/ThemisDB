> ⚠️ **Historischer Plan** – Dieser Plan beschreibt den Entwicklungsstand zum Zeitpunkt der Erstellung.
> Für aktuellen Teststatus: `ctest --preset linux-ninja-release --test-dir build/linux-ninja-release -R <pattern>` verwenden.

# ThemisDB RAID-Sharding Comprehensive Test Plan

**Version:** 1.0  
**Status:** Ready for Execution  
**Execution Timeline:** 2 Phases (Pure RAID + Under Load)  
**Total Duration:** ~48-72 hours

---

## Executive Summary

Comprehensive test infrastructure to validate ThemisDB's RAID replication and sharding capabilities in two distinct phases:

### Phase 1: Pure RAID Capabilities (12-16 hours)
- Data synchronization accuracy
- Node replication consistency
- Failover and recovery mechanisms
- High availability guarantees
- **Focus:** Raw RAID functionality without external load

### Phase 2: RAID Under Load (36-56 hours)
- 6.7M Wikipedia articles distributed across shards
- Real-world query patterns at scale
- Cross-shard operations (joins, aggregations)
- Failover under active load
- **Focus:** RAID behavior with realistic data and traffic patterns

---

## Phase 1: Pure RAID Capabilities Test

### 1.1 RAID Replication Test (Basic)

**Objective:** Verify core RAID replication functionality

**Test Configuration:**
```yaml
raid_level: RAID1 (Mirror)
nodes: 3
disk_per_node: 50GB
data_size: 10GB
```

**Test Steps:**

1. **[1a] Write Phase**
   - Write 10GB test data to primary node
   - Measure write throughput
   - Expected: >200 MB/s
   - Target latency: <5ms per write

2. **[1b] Synchronization Phase**
   - Verify data synced to all replicas
   - Calculate sync latency (P50, P95, P99)
   - Expected: <50ms for 10GB
   - Checksum verification: 100% match across replicas

3. **[1c] Node Failure & Recovery**
   - Stop primary node (simulation)
   - Promote replica to primary (failover)
   - Restart failed node
   - Verify automatic re-synchronization
   - Expected recovery time: <5 seconds
   - Data consistency: 100%

4. **[1d] Read Performance**
   - Execute 100 sequential reads from each replica
   - Measure read throughput
   - Expected: >300 MB/s aggregate

**Metrics to Collect:**
- Write throughput (MB/s)
- Read throughput (MB/s)
- Sync latency (ms) - P50, P95, P99
- Recovery time (seconds)
- Checksum match rate (%)
- Error count (0 expected)

**Success Criteria:**
- ✅ All checksums match across replicas (100%)
- ✅ Recovery completes within 5 seconds
- ✅ Zero data loss detected
- ✅ Write throughput >200 MB/s
- ✅ Sync latency <50ms for 10GB

---

### 1.2 RAID Failover & High Availability Test

**Objective:** Validate failover mechanisms and availability guarantees

**Test Configuration:**
```yaml
raid_level: RAID5 (Striping + Parity)
nodes: 4
disk_per_node: 100GB
stripe_size: 8KB
parity_blocks: 1 per stripe
```

**Test Steps:**

1. **[2a] Multi-Node Failure Simulation**
   - Write 20GB data across 4 nodes
   - Fail node 1 (observe failover)
   - Fail node 2 while node 1 is down (max single fault for RAID5)
   - Verify data remains accessible
   - Expected: All reads succeed during degraded mode

2. **[2b] Automatic Recovery**
   - Restart failed node 1
   - System automatically rebuilds parity
   - Measure rebuild time
   - Expected: <30 minutes for 100GB node rebuild
   - Monitor: I/O impact on active queries (<5% degradation)

3. **[2c] Cascading Failures**
   - While rebuilding, inject transient network partition
   - Verify cluster stays online (no split-brain)
   - Verify split-brain prevention (e.g., quorum consensus)
   - Recovery after partition heals

**Metrics to Collect:**
- Failover time (ms)
- Data availability during degradation (%)
- Rebuild time for failed node (minutes)
- I/O impact during rebuild (%)
- Queries handled during failover
- Error rate during degradation

**Success Criteria:**
- ✅ Failover completes in <100ms
- ✅ Data availability >99.99% during degradation
- ✅ No data corruption detected
- ✅ Automatic rebuild initiates within 5 seconds
- ✅ Read availability maintained at 100%

---

### 1.3 Data Consistency Test

**Objective:** Verify consistency guarantees (eventual or strong)

**Test Configuration:**
```yaml
raid_level: RAID6 (Dual Parity)
nodes: 6
disk_per_node: 80GB
test_data: 15GB
```

**Test Steps:**

1. **[3a] Write Consistency**
   - Issue 1000 concurrent writes to distributed data
   - Generate checksums for each write
   - Verify all replicas receive identical data
   - Measure write-to-replica propagation time
   - Expected: <100ms for all replicas

2. **[3b] Read Consistency**
   - Issue concurrent reads from multiple replicas
   - Verify all reads return identical data
   - Test during ongoing writes (concurrent load)
   - Expected: 100% read consistency

3. **[3c] Cross-Node Verification**
   - Read full dataset from node N1 (checksum A)
   - Read same dataset from node N2 (checksum B)
   - Compare: A == B for all ranges
   - Repeat for all node pairs
   - Expected: 100% match across all pairs

4. **[3d] Partial Failure Consistency**
   - While replicating data, disconnect one replica
   - Verify replicas detect divergence
   - Measure divergence detection time
   - Reconnect replica and verify re-sync
   - Expected: Divergence detected within 5 seconds

**Metrics to Collect:**
- Write-to-replica propagation (ms)
- Read consistency violations (count)
- Cross-node checksum mismatches (count)
- Divergence detection time (seconds)
- Re-sync time (seconds)

**Success Criteria:**
- ✅ Write propagation <100ms to all replicas
- ✅ Zero read consistency violations in 10,000 reads
- ✅ Cross-node checksums match (100%)
- ✅ Divergence detected within 5 seconds
- ✅ No orphaned data detected

---

### 1.4 Network Partition Test

**Objective:** Verify behavior under network splits (brain split prevention)

**Test Configuration:**
```yaml
raid_level: RAID5
nodes: 5 (odd number for quorum)
partition_scenario: Network split 3-node vs 2-node
```

**Test Steps:**

1. **[4a] Partition Creation**
   - Create network partition: nodes {1,2,3} vs {4,5}
   - Write data to majority partition (3 nodes)
   - Verify minority partition blocks writes
   - Expected: Majority accepts writes, minority rejects

2. **[4b] Partition Healing**
   - Heal network partition
   - Verify minority partition re-joins
   - Check for conflicts/divergence
   - Trigger conflict resolution
   - Expected: Re-sync completes without data loss

3. **[4c] Minority Partition Restart**
   - Restart minority partition nodes
   - Verify they become read-only
   - After rejoining majority, verify they sync
   - Expected: <10 second sync time

**Metrics to Collect:**
- Partition detection time (seconds)
- Write rejection rate in minority (%)
- Partition healing time (seconds)
- Conflict resolution time (seconds)
- Re-sync data transferred (GB)

**Success Criteria:**
- ✅ Minority partition becomes unavailable for writes
- ✅ Zero data loss on partition heal
- ✅ No write conflicts detected
- ✅ Partition healing <20 seconds
- ✅ Automatic re-sync without manual intervention

---

## Phase 2: RAID Under Load Test (Wikipedia Data)

### 2.1 Wikipedia Data Setup

**Dataset:** English Wikipedia Complete Articles
```yaml
articles: 6,700,000
dump_size: 24GB (compressed: 20.2GB bzip2)
avg_article_size: 5-10KB
uncompressed_size: ~90GB
fields_extracted:
  - article_id (BIGINT)
  - title (VARCHAR 255)
  - namespace (SMALLINT)
  - text (LONGTEXT)
  - timestamp (DATETIME)
  - contributor (VARCHAR)
  - contributor_id (BIGINT)
```

**Schema Design:**

**SQL Schema (for Relational RAID):**
```sql
CREATE TABLE wikipedia_articles (
    article_id BIGINT PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    namespace SMALLINT DEFAULT 0,
    text LONGTEXT NOT NULL,
    size_bytes INTEGER,
    timestamp DATETIME,
    contributor VARCHAR(255),
    contributor_id BIGINT,
    INDEX idx_title (title),
    INDEX idx_namespace (namespace),
    FULLTEXT INDEX ft_text (text)
);

-- Sharding key: HASH(article_id) % num_shards
-- Or: Range-based: article_id / (6700000 / num_shards)
```

**Shard Distribution:**
```yaml
num_shards: 5
articles_per_shard: 1,340,000
target_per_shard_size: 6-7GB

distribution_check:
  - min_shard_articles: 1,339,500
  - max_shard_articles: 1,340,500
  - balance_ratio_target: >0.95 (1.0 = perfect)
```

---

### 2.2 Bulk Insert Under RAID

**Objective:** Measure insert performance with RAID replication

**Configuration:**
```yaml
raid_level: RAID5
num_nodes: 5
replica_count: 2 (via RAID5 parity)
articles_total: 6,700,000
batch_size: 50,000 articles per batch
expected_duration: 30-45 minutes
```

**Test Procedure:**

1. **[5a] Pre-Load Phase**
   - Validate RAID cluster health
   - Pre-allocate disk space
   - Warm up caches
   - Duration: 5 minutes

2. **[5b] Streaming Insert Phase**
   - Stream Wikipedia articles from XML
   - Distribute via sharding algorithm
   - Batch insert 50,000 at a time
   - Measure throughput every 100,000 articles
   - Duration: 30-45 minutes

   **Progress Milestones:**
   - 1M articles: 15 min (66K articles/sec)
   - 3M articles: 45 min
   - 6.7M articles: ~100 min total

3. **[5c] Post-Load Verification**
   - Count articles in each shard
   - Verify sharding distribution
   - Check replication consistency
   - Duration: 5-10 minutes

**Metrics to Collect:**
- Insert throughput (articles/sec): target >50K
- Throughput by shard (to detect skew)
- Write latency (P50, P95, P99): target <20ms
- Replication sync lag (ms): target <100ms
- Memory growth over time
- CPU utilization: target <70%
- Network I/O (MB/s)
- Shard balance ratio (1.0 = perfect)

**Monitoring During Insert:**
```bash
# Monitor in real-time
watch -n 1 'docker stats themis-raid-*'

# Track shard distribution
psql -h shard1 -c "SELECT COUNT(*) FROM wikipedia_articles"
psql -h shard2 -c "SELECT COUNT(*) FROM wikipedia_articles"
# ... (repeat for all shards)
```

**Success Criteria:**
- ✅ Insert throughput >50K articles/sec
- ✅ Write latency P99 <50ms
- ✅ Replication lag <100ms
- ✅ All articles replicated to ≥2 nodes (RAID5)
- ✅ Shard balance ratio >0.95
- ✅ Zero insert errors

---

### 2.3 Full-Text Search Under RAID

**Objective:** Measure query performance across distributed RAID shards

**Configuration:**
```yaml
query_type: Full-text search on Wikipedia articles
num_queries: 5,000 (executed over 10 minutes)
search_terms: 100 common Wikipedia search patterns
  - "machine learning"
  - "artificial intelligence"
  - "database"
  - "distributed systems"
  - etc.
concurrent_clients: 10
```

**Query Patterns:**

```sql
-- Pattern 1: Simple full-text search
SELECT article_id, title, snippet(text, 50) 
FROM wikipedia_articles 
WHERE text CONTAINS "machine learning"
LIMIT 100;

-- Pattern 2: Search with range filter
SELECT article_id, title, contributor
FROM wikipedia_articles
WHERE article_id > 1000000 AND article_id < 2000000
  AND text CONTAINS "artificial intelligence"
LIMIT 50;

-- Pattern 3: Aggregation across shards
SELECT COUNT(*) as article_count, 
       COUNT(DISTINCT contributor_id) as contributors
FROM wikipedia_articles
WHERE text CONTAINS "python programming";

-- Pattern 4: JOIN across shards
SELECT a.title, b.title
FROM wikipedia_articles a
JOIN wikipedia_articles b ON a.contributor_id = b.contributor_id
WHERE a.text CONTAINS "machine learning"
LIMIT 100;
```

**Test Execution:**

1. **[6a] Query Execution**
   - Execute 5,000 queries over 10 minutes
   - Distribute across 10 concurrent clients
   - Mix different query patterns
   - Measure latency for each query
   - Expected: P50 <10ms, P95 <50ms, P99 <200ms

2. **[6b] Cache Behavior**
   - Measure cache hit rate
   - Monitor memory pressure
   - Expected: >80% cache hit rate after warmup

3. **[6c] Cross-Shard Performance**
   - Identify queries that span multiple shards
   - Measure cross-shard latency overhead
   - Expected: <5ms additional latency vs single-shard

**Metrics to Collect:**
- Query latency (P50, P95, P99): target P95 <50ms
- Queries per second (QPS): target >200 QPS aggregate
- Cache hit rate: target >80%
- Cross-shard queries (count and latency)
- Error rate: target <0.1%
- Network I/O per query (KB)

**Success Criteria:**
- ✅ Query latency P95 <50ms
- ✅ Aggregate QPS >200
- ✅ Cache hit rate >80%
- ✅ Query error rate <0.1%
- ✅ Cross-shard latency <5ms overhead

---

### 2.4 Complex Cross-Shard Operations

**Objective:** Test joins, aggregations, and operations spanning multiple shards

**Test Configuration:**
```yaml
num_operations: 1,000 cross-shard operations
operation_types:
  - JOIN: 40% (articles linked by contributor)
  - AGGREGATION: 30% (GROUP BY across shards)
  - UNION: 20% (combine results from multiple shards)
  - SUBQUERY: 10% (subqueries on different shards)
```

**Sample Operations:**

```sql
-- JOIN across shards: Find articles by same contributor
SELECT a.article_id, a.title, b.article_id, b.title
FROM wikipedia_articles a
JOIN wikipedia_articles b 
  ON a.contributor_id = b.contributor_id
WHERE a.article_id < 1000000
  AND b.article_id > 5000000
LIMIT 100;

-- AGGREGATION across shards
SELECT contributor_id, COUNT(*) as article_count, 
       AVG(size_bytes) as avg_size
FROM wikipedia_articles
WHERE namespace = 0
GROUP BY contributor_id
HAVING article_count > 100
LIMIT 1000;

-- UNION combining results
SELECT * FROM (
  SELECT article_id, title FROM wikipedia_articles WHERE article_id < 1000000
  UNION ALL
  SELECT article_id, title FROM wikipedia_articles WHERE article_id > 5000000
) combined
LIMIT 500;
```

**Test Execution:**

1. **[7a] Operation Execution**
   - Execute 1,000 complex operations
   - Measure execution time per operation
   - Track data shuffled between shards
   - Expected: <500ms per complex operation

2. **[7b] Network Impact**
   - Measure inter-shard network traffic
   - Calculate data transferred per operation
   - Expected: <50MB aggregate for 1,000 ops

3. **[7c] Correctness Verification**
   - Validate results against single-shard baseline
   - Check row counts and aggregations
   - Expected: 100% correctness

**Metrics to Collect:**
- Operation latency: target <500ms
- Inter-shard network traffic (MB)
- Result row counts (verify correctness)
- Error count: target 0
- Execution time breakdown (planning, execution, fetch)

**Success Criteria:**
- ✅ Complex operation latency <500ms
- ✅ 100% result correctness
- ✅ Inter-shard traffic <50MB for 1,000 ops
- ✅ Zero operation errors
- ✅ Network utilization <30% peak

---

### 2.5 Failover Under Active Load

**Objective:** Verify RAID failover while queries are active

**Configuration:**
```yaml
background_load:
  - 10 concurrent search clients (5,000 queries/10 min)
  - 2 concurrent insert clients (1,000 articles/min)
  - 3 concurrent update clients (500 updates/min)
failover_scenario: Node 1 failure at T=5 minutes
```

**Test Procedure:**

1. **[8a] Load Establishment** (0-5 minutes)
   - Start all client types
   - Reach steady state
   - Baseline metrics: QPS, insert rate, update rate
   - Expected: 200+ QPS, 1K articles/min inserts

2. **[8b] Failover Trigger** (T=5 min)
   - Stop node 1 (simulating hardware failure)
   - Record: Failover time, query interruptions
   - Expected: Failover <100ms, queries resume within 1 second

3. **[8c] Degraded Operation** (5-15 minutes)
   - Continue all clients during degraded state (4 nodes instead of 5)
   - Measure: Latency increase, throughput change
   - Expected: <10% latency degradation, >95% throughput

4. **[8d] Recovery** (15-20 minutes)
   - Restart failed node 1
   - System rebuilds data via RAID parity
   - Monitor: Rebuild progress, performance impact
   - Expected: <20% query latency during rebuild

**Metrics to Collect During Failover:**

| Phase | Metric | Normal | Degraded | During Rebuild |
|-------|--------|--------|----------|-----------------|
| Query Latency P95 | 40ms | 50ms (+25%) | 48ms |
| Insert Rate | 1000/min | 950/min (-5%) | 900/min |
| Network I/O | 150MB/s | 145MB/s | 200MB/s |
| Error Count | 0/min | <5/min | 0/min |
| CPU Usage | 60% | 75% | 80% |
| Rebuild Progress | N/A | N/A | Measure % complete |

**Success Criteria:**
- ✅ Failover time <100ms
- ✅ Query recovery <1 second
- ✅ Queries resumed with <10% latency increase
- ✅ Query error rate <0.5% during failover
- ✅ Automatic rebuild initiated
- ✅ Rebuild completes within 30 minutes
- ✅ Zero data loss or corruption

---

### 2.6 Cascading Failures

**Objective:** Test behavior under multiple simultaneous node failures

**Configuration:**
```yaml
raid_level: RAID5 (can tolerate 1 simultaneous failure)
scenario: Fail 2 nodes in sequence (stress beyond design limit)
```

**Test Procedure:**

1. **[9a] First Node Failure** (T=0)
   - Stop node 1
   - Cluster operates in degraded mode (RAID5 limit: 1 fault)
   - Expected: All reads/writes succeed

2. **[9b] Second Node Failure** (T=5 minutes)
   - While rebuilding from node 1, fail node 2
   - **Expected behavior:** Data loss OR read-only mode
   - Verify graceful degradation (no crashes)

3. **[9c] Recovery Sequence**
   - Restart node 2 (the newer failure)
   - System recognizes it can't fully recover (lost 2 nodes)
   - Options:
     - Restore from backup
     - Reconstruct from remaining parity blocks
   - Expected: Manual intervention required

4. **[9d] Full Recovery**
   - Restart node 1
   - Trigger full rebuild
   - Verify data integrity

**Metrics to Collect:**
- Error rate when exceeding RAID5 fault tolerance
- Data availability drop
- Recovery options presented
- Rebuild time after multiple failures
- Data integrity verification

**Success Criteria:**
- ✅ Graceful handling of over-fault scenarios
- ✅ Clear error messages (not silent data loss)
- ✅ Option to restore from backup
- ✅ Rebuild completes after recovery
- ✅ Zero silent data corruption

---

### 2.7 Data Integrity Verification

**Objective:** Final comprehensive verification that all 6.7M Wikipedia articles are intact and consistent

**Test Procedure:**

1. **[10a] Article Count Verification**
   ```sql
   -- Should return 6,700,000 across all shards
   SELECT COUNT(*) FROM wikipedia_articles;
   
   -- Per shard:
   SELECT shard_key, COUNT(*) FROM wikipedia_articles GROUP BY shard_key;
   ```

2. **[10b] Checksum Verification**
   - Compute SHA256 for each article text
   - Compare checksums between replicas
   - Expected: 100% match

3. **[10c] Foreign Key Integrity**
   - Verify contributor_id references are valid
   - Check namespace values are in valid range (0-828)
   - Expected: 100% valid

4. **[10d] Replication Consistency**
   - Read same article from different replicas
   - Verify byte-for-byte identical
   - Expected: 100% consistency

**Metrics to Collect:**
- Article count: target 6,700,000
- Checksum mismatches: target 0
- Data integrity issues: target 0
- Replication divergences: target 0

**Success Criteria:**
- ✅ All 6,700,000 articles present
- ✅ Zero checksum mismatches
- ✅ 100% data integrity
- ✅ 100% replication consistency

---

## Implementation Details

### Test Execution Command

```bash
# Phase 1 only (Pure RAID)
python3 raid_sharding_test_suite.py --phase 1 --output-dir raid_benchmarks

# Phase 2 only (Under Load)
python3 raid_sharding_test_suite.py --phase 2 \
  --wikipedia-dir /path/to/wikipedia-data \
  --raid-level raid5 \
  --num-nodes 5 \
  --output-dir raid_benchmarks

# Both phases (Full Suite)
python3 raid_sharding_test_suite.py --phase all \
  --wikipedia-dir /path/to/wikipedia-data \
  --output-dir raid_benchmarks
```

### Monitoring During Tests

```bash
# Real-time Docker stats
watch -n 1 'docker stats --no-stream themis-raid-*'

# Monitor network traffic
iftop -i docker0

# Check disk I/O
iostat -x 1 10

# Monitor system resources
top -b -n 1 | head -20
```

### Result Files Generated

```
raid_benchmarks/
├── phase1/
│   ├── raid_test_report_20251209_120000.json
│   └── raid_sharding_test.log
├── phase2/
│   ├── raid_load_test_report_20251209_180000.json
│   └── raid_sharding_test.log
└── summary_report.md
```

### Report Contents

Each report includes:

```json
{
  "timestamp": "2025-12-09T12:00:00",
  "phase": "Phase 1: Pure RAID Capabilities",
  "test_count": 3,
  "results": [
    {
      "test_name": "RAID_Replication_Basic",
      "raid_level": "mirror",
      "num_nodes": 3,
      "write_throughput_mbps": 245.0,
      "sync_latency_ms": 45.2,
      "recovery_time_sec": 3.5,
      "checksum_match": true,
      "verification_success": true,
      "error_count": 0
    }
  ],
  "summary": {
    "avg_write_throughput": 250.0,
    "avg_sync_latency_ms": 42.1,
    "all_checksums_match": true
  }
}
```

---

## Success Criteria Summary

### Phase 1: Pure RAID

| Criterion | Target | Status |
|-----------|--------|--------|
| Write Throughput | >200 MB/s | Must achieve |
| Sync Latency | <50ms for 10GB | Must achieve |
| Recovery Time | <5 seconds | Must achieve |
| Data Consistency | 100% match | Must achieve |
| Failover Time | <100ms | Must achieve |
| Error Rate | 0 | Must achieve |

### Phase 2: Under Load

| Criterion | Target | Status |
|-----------|--------|--------|
| Insert Rate | >50K articles/sec | Must achieve |
| Query Latency P95 | <50ms | Must achieve |
| Shard Balance | >0.95 ratio | Must achieve |
| Query Error Rate | <0.1% | Must achieve |
| Failover Time | <100ms | Must achieve |
| Data Integrity | 100% verified | Must achieve |

---

## Timeline Estimate

```
Phase 1: Pure RAID Capabilities
├─ Setup & Config: 30 minutes
├─ RAID Replication Test: 2 hours
├─ Failover Test: 1.5 hours
├─ Data Consistency Test: 1 hour
├─ Network Partition Test: 1 hour
└─ Report Generation: 30 minutes
TOTAL PHASE 1: ~6 hours

Phase 2: RAID Under Load
├─ Wikipedia Setup: 2 hours
├─ Bulk Insert Phase: 2 hours (6.7M articles)
├─ Query Load Test: 30 minutes
├─ Cross-Shard Operations: 1 hour
├─ Failover Under Load: 1 hour
├─ Cascading Failures: 30 minutes
├─ Final Verification: 1 hour
└─ Report Generation: 30 minutes
TOTAL PHASE 2: ~8-10 hours

COMPLETE TEST SUITE: 14-16 hours (can parallelize some)
TOTAL WITH ITERATIVE RUNS: 48-72 hours
```

---

## Deliverables

### Phase 1 Deliverable
- ✅ Pure RAID Test Report (JSON + Markdown)
- ✅ Metrics: Throughput, Latency, Recovery Time
- ✅ Verification: Consistency, Integrity, Failover

### Phase 2 Deliverable
- ✅ Load Test Report with Wikipedia (JSON + Markdown)
- ✅ Metrics: Insert rate, Query latency, Cross-shard performance
- ✅ Verification: Data distribution, Query correctness, Failover under load

### Combined Deliverable
- ✅ RAID-Sharding Comprehensive Report
- ✅ Competitive comparison (vs PostgreSQL, MongoDB, Neo4j clustering)
- ✅ Performance tuning recommendations
- ✅ Operational guidelines for production deployment

---

## References

- **Test Script:** `benchmarks/raid_sharding_test_suite.py`
- **Docker Setup:** `benchmarks/docker-compose.raid-*.yml` (to be created)
- **Wikipedia Data:** `wikipedia-data/enwiki-latest-pages-articles.xml.bz2`
- **Previous Results:** `benchmarks/SCIENTIFIC_VALIDATION_REPORT_v1.0.1.md`

---

**Status:** Ready for Execution  
**Author:** ThemisDB Team  
**Date:** December 9, 2025  
**Version:** 1.0.0-final
