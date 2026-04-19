> **Aktueller Build-Flow:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`

# ThemisDB RAID-Sharding Test Suite - Quick Start

**Status:** Ready for Execution  
**Test Phases:** 2 (Pure RAID + Under Load)  
**Estimated Duration:** 14-72 hours total

---

## Quick Start

### Phase 1: Pure RAID Capabilities (6-8 hours)

Test RAID replication, synchronization, failover, and consistency without external load.

```bash
# Start RAID1 (mirror) cluster
docker compose -f benchmarks/docker-compose.raid-phase1.yml up -d

# Run pure RAID tests
python3 benchmarks/raid_sharding_test_suite.py --phase 1 \
  --output-dir raid_benchmarks/phase1

# Monitor in real-time
watch -n 1 'docker stats themis-raid-mirror-*'

# View results
cat raid_benchmarks/phase1/raid_test_report_*.json | jq .

# Stop cluster
docker compose -f benchmarks/docker-compose.raid-phase1.yml down
```

**What Gets Tested:**
- ✅ RAID replication (data synchronization)
- ✅ Multi-node failover (<5 second recovery)
- ✅ Data consistency guarantees
- ✅ Network partition handling
- ✅ Automatic node recovery

**Expected Results:**
```
Write Throughput:        >200 MB/s
Read Throughput:        >300 MB/s
Sync Latency:           <50ms for 10GB
Recovery Time:          <5 seconds
Checksum Match:         100%
Error Rate:             0%
```

---

### Phase 2: RAID Under Load with Wikipedia (8-10 hours)

Test RAID with 6.7 million Wikipedia articles: insert, query, cross-shard joins, and failover under realistic load.

**Prerequisites:**

```bash
# 1. Download Wikipedia dump (if not already done)
cd wikipedia-data
wget https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2
# Estimated: 4-8 hours, 20GB file

# 2. Extract Wikipedia XML
pbzip2 -d -p 8 enwiki-latest-pages-articles.xml.bz2
# Estimated: 2-4 hours, produces 90GB

# 3. Parse into database format (optional for Phase 2 - test handles it)
python3 benchmarks/wikipedia_stress_test.py --load
```

**Execution:**

```bash
# Start RAID5 (parity) cluster with 5 nodes
docker compose -f benchmarks/docker-compose.raid-phase2.yml up -d

# Run load tests with Wikipedia data
python3 benchmarks/raid_sharding_test_suite.py --phase 2 \
  --wikipedia-dir ./wikipedia-data \
  --raid-level raid5 \
  --num-nodes 5 \
  --output-dir raid_benchmarks/phase2

# Monitor load testing
watch -n 1 'docker stats themis-raid-parity-*'

# Check shard distribution
for i in {0..4}; do
  echo "Shard $i:"
  curl -s http://localhost:$((8710 + $i))/metrics | grep articles_count
done

# View Grafana dashboards
# URL: http://localhost:3001
# User: admin / Pass: admin

# Stop cluster
docker compose -f benchmarks/docker-compose.raid-phase2.yml down
```

**What Gets Tested:**
- ✅ Bulk insert of 6.7M Wikipedia articles (50K articles/sec target)
- ✅ Sharding distribution across 5 nodes (balance ratio >0.95)
- ✅ Full-text search on distributed data (5,000 queries, <50ms P95)
- ✅ Cross-shard joins and aggregations
- ✅ Failover while queries are active
- ✅ Cascading failures recovery
- ✅ Final data integrity verification

**Expected Results:**
```
Insert Throughput:      >50K articles/sec
Insert Latency P99:     <50ms
Query Latency P95:      <50ms
Queries Per Second:     >200
Shard Balance Ratio:    >0.95 (1.0 = perfect)
Query Error Rate:       <0.1%
Failover Time:          <100ms
Data Integrity:         100% verified (6.7M articles)
```

---

## Full Test Suite (Both Phases)

```bash
# Run complete test suite
python3 benchmarks/raid_sharding_test_suite.py --phase all \
  --wikipedia-dir ./wikipedia-data \
  --raid-level raid5 \
  --num-nodes 5 \
  --output-dir raid_benchmarks

# Generates comprehensive report
# Location: raid_benchmarks/summary_report.md
```

---

## Architecture Overview

### Phase 1: RAID1 Cluster (3 nodes)

```
┌─────────────────────────────────────────────────────────┐
│                  RAID1 (Mirror) Cluster                 │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  Primary Node (Port 8700)  ← All writes go here         │
│       ↓                                                   │
│  Replica 1 (Port 8701)     ← Synchronous copy           │
│  Replica 2 (Port 8702)     ← Synchronous copy           │
│       ↓ (if primary fails)                              │
│  One replica becomes primary (failover)                 │
│                                                           │
│  Network: 10.0.9.0/24 (isolated)                        │
│  Monitoring: Prometheus + Grafana                       │
└─────────────────────────────────────────────────────────┘
```

### Phase 2: RAID5 Cluster (5 nodes)

```
┌──────────────────────────────────────────────────────────┐
│            RAID5 (Parity) Cluster - 5 Nodes            │
├──────────────────────────────────────────────────────────┤
│                                                            │
│  Wikipedia Data (6.7M articles, 90GB)                    │
│  ↓                                                         │
│  Sharding Layer (HASH or RANGE based)                    │
│  ↓                                                         │
│  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐        │
│  │ S1   │  │ S2   │  │ S3   │  │ S4   │  │ S5   │        │
│  │1.34M │  │1.34M │  │1.34M │  │1.34M │  │1.34M │        │
│  │arts  │  │arts  │  │arts  │  │arts  │  │arts  │        │
│  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘        │
│  │  P50│  │  P51│  │  P52│  │  P53│  │  P54│            │
│  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘        │
│     ↓                                              ↓       │
│  Parity Stripe (P50-P54 recovery blocks)                 │
│     ↓ (if any 1 node fails)                              │
│  Reconstruct from parity + remaining 4 nodes            │
│                                                            │
│  Query Routing:                                          │
│  Full-text search → All shards (parallel)               │
│  Cross-shard JOIN → Shard 1,3,5 results + reduce       │
│  Aggregation → Partial results + merge                  │
│                                                            │
│  Network: 10.0.10.0/24 (isolated)                       │
│  Monitoring: Prometheus + Grafana + Loki                │
└──────────────────────────────────────────────────────────┘
```

---

## Monitoring & Dashboards

### Phase 1 Monitoring

**Prometheus:** http://localhost:9090  
**Grafana:** http://localhost:3000

**Key Metrics:**
- Replication lag (sync latency)
- Node health status
- Failover events
- Data consistency checks
- Network I/O per node

### Phase 2 Monitoring

**Prometheus:** http://localhost:9091  
**Grafana:** http://localhost:3001  
**Loki (logs):** http://localhost:3100  

**Key Metrics:**
- Insert throughput (articles/sec)
- Query latency (P50, P95, P99)
- Shard size distribution
- Cross-shard traffic
- Failover events and recovery
- Data integrity checks

---

## Results & Reports

### Phase 1 Report Structure

```json
{
  "timestamp": "2025-12-09T12:00:00",
  "phase": "Phase 1: Pure RAID Capabilities",
  "tests": [
    {
      "test_name": "RAID_Replication_Basic",
      "raid_level": "mirror",
      "write_throughput_mbps": 245.0,
      "sync_latency_ms": 45.2,
      "recovery_time_sec": 3.5,
      "checksum_match": true,
      "error_count": 0
    },
    {
      "test_name": "RAID_Failover_HA",
      "raid_level": "parity",
      "failover_time_ms": 87.5,
      "rebuild_time_sec": 1542.0,
      "data_consistency_verified": true
    },
    {
      "test_name": "RAID_Data_Consistency",
      "raid_level": "dual_parity",
      "consistency_violations": 0,
      "replication_lag_ms": 32.0
    }
  ],
  "summary": {
    "avg_write_throughput": 250.0,
    "avg_sync_latency": 42.1,
    "all_tests_passed": true
  }
}
```

### Phase 2 Report Structure

```json
{
  "timestamp": "2025-12-09T18:00:00",
  "phase": "Phase 2: RAID Under Load",
  "tests": [
    {
      "test_name": "RAID_Bulk_Insert",
      "articles_inserted": 6700000,
      "insert_throughput_per_sec": 55000,
      "duration_sec": 122,
      "shard_balance_ratio": 0.967,
      "consistency_verified": true
    },
    {
      "test_name": "Full_Text_Search",
      "queries_executed": 5000,
      "avg_latency_ms": 12.5,
      "latency_p95_ms": 45.2,
      "latency_p99_ms": 187.3,
      "error_rate_percent": 0.02
    },
    {
      "test_name": "Cross_Shard_Joins",
      "joins_executed": 1000,
      "avg_latency_ms": 45.2,
      "network_io_mb": 28.4
    },
    {
      "test_name": "Failover_Under_Load",
      "failover_time_ms": 95.2,
      "queries_interrupted": 12,
      "recovery_time_sec": 4.2,
      "data_integrity_verified": true
    }
  ],
  "summary": {
    "total_articles": 6700000,
    "avg_insert_throughput": 55000,
    "avg_query_latency_p95": 45.2,
    "all_tests_passed": true
  }
}
```

---

## Troubleshooting

### Phase 1: Common Issues

**Issue: Container fails to start**
```bash
# Check logs
docker logs themis-raid-mirror-node-0

# Verify network
docker network ls | grep themis-raid

# Rebuild network
docker network rm themis-raid
docker compose -f benchmarks/docker-compose.raid-phase1.yml up -d
```

**Issue: Replication lag too high**
```bash
# Check network connectivity
docker exec themis-raid-mirror-node-0 ping themis-raid-mirror-node-1

# Monitor network I/O
docker stats themis-raid-mirror-* --no-stream

# Reduce write rate if necessary
```

### Phase 2: Common Issues

**Issue: Wikipedia data not loading**
```bash
# Verify file exists
ls -lh ./wikipedia-data/enwiki-latest-pages-articles.xml

# Check disk space
df -h ./wikipedia-data/

# Verify permissions
chmod 644 ./wikipedia-data/enwiki-latest-pages-articles.xml
```

**Issue: Query timeout during load test**
```bash
# Increase query timeout
export THEMIS_QUERY_TIMEOUT=30000  # 30 seconds

# Reduce concurrent client count
python3 benchmarks/raid_sharding_test_suite.py --phase 2 \
  --concurrent-clients 5  # Reduce from default 10
```

**Issue: Node out of memory**
```bash
# Check memory usage
docker stats themis-raid-parity-*

# Increase memory allocation
# Edit docker-compose.raid-phase2.yml:
# resources:
#   limits:
#     memory: 8G  # Increase from 4G
```

---

## Performance Tuning

### Phase 1 Tuning

```bash
# Increase sync parallelism
export THEMIS_SYNC_THREADS=4

# Increase replication buffer
export THEMIS_REPLICATION_BUFFER_SIZE=268435456  # 256MB

# Tune network buffer
export THEMIS_NETWORK_BUFFER_SIZE=134217728  # 128MB
```

### Phase 2 Tuning

```bash
# Increase shard count
--num-shards 10  # Split across more shards

# Increase batch size for inserts
export THEMIS_INSERT_BATCH_SIZE=100000

# Enable compression for large text fields
export THEMIS_ENABLE_COMPRESSION=true

# Tune query cache
export THEMIS_QUERY_CACHE_SIZE=1G
```

---

## Next Steps

1. **Phase 1 Execution**
   - Run pure RAID tests (6-8 hours)
   - Generate replication baseline metrics
   - Validate failover mechanisms

2. **Wikipedia Preparation**
   - Download 24GB Wikipedia dump (if not done)
   - Extract XML (2-4 hours)
   - Parse into database formats

3. **Phase 2 Execution**
   - Load 6.7M articles (2 hours)
   - Execute search queries (30 min)
   - Test cross-shard operations (1 hour)
   - Simulate failover under load (1 hour)
   - Verify data integrity (1 hour)

4. **Analysis & Reporting**
   - Compare with PostgreSQL/MongoDB clustering
   - Identify optimization opportunities
   - Generate production deployment guidelines

---

## Key Metrics to Track

### Phase 1 Baseline

| Metric | Phase 1 Target | Phase 2 with Load |
|--------|---|---|
| Write Throughput | >200 MB/s | >100 MB/s |
| Sync Latency | <50ms | <100ms |
| Recovery Time | <5sec | <10sec |
| Data Consistency | 100% | 100% |
| Node Availability | 99.99% | 99.9% |

### Phase 2 Wikipedia

| Metric | Target | Notes |
|--------|--------|-------|
| Insert Rate | >50K arts/sec | Bulk operation |
| Query Latency P95 | <50ms | Full-text search |
| Shard Balance | >0.95 | 1.0 = perfect |
| Query Error Rate | <0.1% | Network timeouts, etc |
| Failover Time | <100ms | Active queries |
| Data Integrity | 100% | All 6.7M verified |

---

## References

- **Test Script:** `benchmarks/raid_sharding_test_suite.py`
- **Test Plan:** `benchmarks/RAID_SHARDING_TEST_PLAN.md`
- **Docker Compose Phase 1:** `benchmarks/docker-compose.raid-phase1.yml`
- **Docker Compose Phase 2:** `benchmarks/docker-compose.raid-phase2.yml`
- **Wikipedia Setup:** `benchmarks/wikipedia_stress_test.py`

---

**Status:** Ready for Execution  
**Author:** ThemisDB Team  
**Date:** December 9, 2025  
**Version:** 1.0.0
