# 🔷 THEMIS SHARDING - ADVANCED SCENARIOS & SCALING BEYOND 8 SHARDS v1.4

**Version:** 1.0  
**Status:** Planning Document  
**Last Updated:** April 2026  
**Target Audience:** Architecture, Engineering Leadership

---

## 📋 EXECUTIVE SUMMARY

This document outlines architectural considerations, performance projections, and implementation strategies for scaling Themis beyond the standard 8-shard configuration.

**Key Scenarios:**
1. **16-Shard Cluster** (2TB total data)
2. **32-Shard Cluster** (4TB total data, multi-region)
3. **64+ Shard Cluster** (8TB+, enterprise-scale)
4. **Multi-Region Deployment** (Active-Active replication)

---

## 🏗️ SCALING ARCHITECTURE

### Current Standard: 8-Shard Cluster

```
Configuration:
  └─ 8 Shards × RF=2 (2 replicas each)
     └─ 16 nodes total
     └─ 800GB-1.2TB data
     └─ Throughput: 6.4M ops/sec
     └─ Latency p99: 1.8ms
     └─ Cost: ~$60K/month (8 × c6i.4xlarge)
```

### Scenario 1: 16-Shard Cluster (2TB Scale)

**Use Cases:**
- Large enterprise with 2TB+ datasets
- High-throughput requirements (12M+ ops/sec)
- Regional clustering (2 x 8-shard clusters per region)

**Architecture:**

```
┌───────────────────────────────────────────────────────┐
│ 16-Shard Cluster (US-East Region)                    │
└───────────────────────────────────────────────────────┘

Router Service
    ↓
[Shard-1] [Shard-2] [Shard-3] [Shard-4]
[Shard-5] [Shard-6] [Shard-7] [Shard-8]
[Shard-9] [Shard-10][Shard-11][Shard-12]
[Shard-13][Shard-14][Shard-15][Shard-16]

Each Shard: RF=2 (32 nodes total)
Data per Shard: 125GB (2TB / 16)
Replicas per Shard: 2 (1 primary + 1 secondary)
```

**Performance Projections:**

```
Metric                    8-Shard    16-Shard    Improvement
─────────────────────────────────────────────────────────────
Total Throughput          6.4M ops   12.8M ops   +100%
Per-Shard Throughput      800k ops   800k ops    (linear)
Latency p99               1.8ms      1.8ms       (similar)
Data per Shard            100GB      125GB       +25%
Total Nodes               16         32          +100%
Network Hops              1-2        2-3         +1 max
Rebalance Time (2→4→16)   90s        180s        +100% (expected)
Cost                      $60K/mo    $120K/mo    +100%

Efficiency Ratio: 12.8M ops / $120K = 106k ops/$ (vs 107k for 8-shard)
```

**Implementation Strategy:**

```
Phase 1: Build 8-shard cluster first (validation)
Phase 2: Add 2 more shards (8→10)
         └─ Rebalance: 10% data moved
         └─ Expected impact: -8% throughput, 60s recovery
Phase 3: Scale to 16 shards (10→16)
         └─ Rebalance: 40% data moved
         └─ Expected impact: -12% throughput, 120s recovery
Phase 4: Enable multi-region replication
```

**Architectural Considerations:**

```
Pros:
  ✅ Exactly linear scaling (12.8M ops vs 6.4M)
  ✅ Same per-shard latency (no overhead)
  ✅ Supports 2TB datasets
  ✅ Can split into 2 × 8-shard clusters per region

Cons:
  ❌ 2x infrastructure cost
  ❌ More complex routing (256 hash buckets)
  ❌ Rebalance takes 2× longer
  ❌ More monitoring overhead
  ❌ Higher network interconnect requirements
```

---

### Scenario 2: 32-Shard Cluster (4TB Enterprise Scale)

**Use Cases:**
- Enterprise with 4TB+ datasets
- Multiple product lines/regions
- Very high concurrency (20M+ ops/sec)

**Architecture:**

```
┌─────────────────────────────────────────────┐
│ 32-Shard Cluster (Enterprise Distribution)  │
└─────────────────────────────────────────────┘

            Router Service
            (Consistent Hash)
                   ↓
    ┌──────────────┬──────────────┐
    │              │              │
Region-1 (8 Shards) Region-2 (8 Shards) Region-3 (8 Shards) Region-4 (8 Shards)
[1-8]               [9-16]              [17-24]               [25-32]

Each Regional Cluster: Independent 8-shard cluster
Replication: RF=2 within region (cross-AZ)
Replication: RF=1 to other regions (async)
RTO between regions: 1-2 hours
RPO between regions: <1 hour
```

**Performance Projections:**

```
Metric                    16-Shard   32-Shard    Notes
─────────────────────────────────────────────────────────────
Total Throughput          12.8M      25.6M       Linear scaling
Per-Shard Throughput      800k       800k        No overhead
Latency p99 (intra-region) 1.8ms     1.8ms       Independent clusters
Latency p99 (cross-region) 50-200ms  50-200ms    Network dependent
Total Data                 2TB        4TB         Scales linearly
Total Nodes                64         128         (32 primary + 32 replica)
Network Links Required     40Gbps     80Gbps      Between shards
Regional RTT Required      <2ms       <2ms        Within region

Regional Split:
  Region-1: 800k ops/sec × 8 shards = 6.4M ops
  Region-2: 800k ops/sec × 8 shards = 6.4M ops
  Region-3: 800k ops/sec × 8 shards = 6.4M ops
  Region-4: 800k ops/sec × 8 shards = 6.4M ops
  ────────────────────────────────────────────
  TOTAL: 25.6M ops/sec global throughput

Cost Analysis:
  Regional Cost: $60K/month × 4 regions = $240K/month
  + Cross-region replication infra: $20K/month
  + Monitoring/Observability: $10K/month
  ─────────────────────────────────────
  Total: ~$270K/month
  
  Efficiency: 25.6M ops / $270K = 94.8k ops/$
  (vs 107k ops/$ for single 8-shard cluster)
  
  Cost Premium for Multi-Region HA: ~12%
```

**Deployment Strategy:**

```
Timeline: 3-4 Months to Full Deployment

Month 1: Region-1 (8 shards)
  - Deploy primary cluster
  - Validate monitoring
  - Load test

Month 2: Region-2 (8 shards) + Cross-Region Replication
  - Deploy 2nd region cluster
  - Setup async replication (RPO <1 hour)
  - Test failover procedures

Month 3: Region-3 & Region-4 (8 shards each)
  - Deploy remaining regions
  - Complete global replication mesh
  - Load test cross-region operations

Month 4: Optimization & Tuning
  - Optimize routing policies
  - Tune rebalance thresholds
  - Document runbooks
  - Train operations team
```

---

### Scenario 3: 64+ Shard Cluster (8TB+ Hyperscale)

**Use Cases:**
- Petabyte-scale data warehouses
- Multi-tenant SaaS with thousands of customers
- Real-time analytics at scale
- AI/ML feature stores (100M+ features)

**Architecture:**

```
┌──────────────────────────────────────────────────────┐
│ 64-Shard Hyperscale Architecture                     │
└──────────────────────────────────────────────────────┘

                  Global Router
                  (Geo-aware)
                       ↓
        ┌──────────┬──────────┬──────────┬──────────┐
        ↓          ↓          ↓          ↓          ↓
    North-1   North-2   South-1   South-2   Europe
    [1-8]     [9-16]    [17-24]   [25-32]   [33-48]
    [49-56]   [57-64]   ...                 

Each Zone: 8-16 shards
Hierarchy: Global → Region → Zone → Shard → Replica

Data Distribution by Tenant:
  Tenant A: Shards 1-8 (all regions for HA)
  Tenant B: Shards 9-16
  Tenant C: Shards 17-24
  ...

Replication Strategy:
  Within Zone: RF=2 (synchronous)
  Cross-Zone: RF=1 (asynchronous)
  Across Regions: RF=1 (eventual consistency)
```

**Critical Design Changes (vs 8-16 Shard):**

```
1. HIERARCHICAL ROUTING
   ├─ Global Layer: Route to appropriate region
   ├─ Regional Layer: Route to zone
   ├─ Zone Layer: Hash to specific shard
   └─ Shard Layer: Execute query
   
   Latency Impact: +0-5ms for global lookups

2. EVENTUAL CONSISTENCY MODEL
   ├─ Strong Consistency: Within zone (RF=2)
   ├─ Weak Consistency: Cross-zone (RF=1, eventual)
   └─ Requires application-level conflict resolution
   
3. QUERY PLANNING COMPLEXITY
   ├─ Single region queries: Simple (group by shard)
   ├─ Multi-region queries: Complex (merge results)
   ├─ Cross-tenant queries: Need access control checks
   └─ Aggregate queries: May require sampling/approximation

4. OPERATIONAL OVERHEAD
   ├─ 5x more servers to manage
   ├─ 10x more monitoring signals
   ├─ 20+ potential failure modes
   ├─ More complex disaster recovery
```

**Performance Expectations:**

```
Metric                    32-Shard   64-Shard    Scaling Impact
─────────────────────────────────────────────────────────────────
Total Throughput          25.6M      51.2M       +100% (linear)
Per-Shard Throughput      800k       800k        (no change)
Intra-Zone Latency p99    1.8ms      1.8ms       (no change)
Cross-Zone Latency p99    50-200ms   50-200ms    (network driven)
Global Lookup Latency     <5ms       <5ms        (caching helps)

Data Capacity             4TB        8TB         +100%
Rebalance Time (logical)  120s       240s        (linear)
Rebalance Network Impact  -12%       -15%        (slightly worse)

Consistency Model:
  Strong: Only within zone
  Weak: Cross-zone (typical <100ms)
```

**Cost Projection:**

```
Infrastructure:
  64 Primary Nodes (c6i.4xlarge): $240K/month
  64 Replica Nodes: $240K/month
  Regional Interconnect (8 × 40Gbps): $40K/month
  Monitoring/Logging Infrastructure: $30K/month
  ────────────────────────────────
  Total: ~$550K/month

  Cost per Shard: $8.6K/month
  Cost per TB: $68.75K/month
  Cost per M ops/sec: $10.7K/month (vs $10.5K for 8-shard baseline)
  
  Efficiency: 51.2M ops / $550K = 93k ops/$
  (vs 107k ops/$ for baseline 8-shard)
  
  Hyperscale Premium: ~13% cost per unit
```

---

## 🔄 MULTI-REGION ACTIVE-ACTIVE DEPLOYMENT

### Architecture & Topology

```
┌─────────────────────────────────────────────────────┐
│ Multi-Region Active-Active (Read/Write Both Sites)  │
└─────────────────────────────────────────────────────┘

┌─────────────────────────┐  ┌─────────────────────────┐
│ US-EAST REGION (Primary)│  │ US-WEST REGION (Active) │
│                         │  │                         │
│ [Shard 1-8] (RF=2)      │  │ [Shard 1-8] (RF=2)      │
│ 8 Nodes (Primary)       │  │ 8 Nodes (Primary)       │
│ 8 Nodes (Secondary)     │  │ 8 Nodes (Secondary)     │
│                         │  │                         │
│ Throughput: 6.4M ops    │  │ Throughput: 6.4M ops    │
└────────────┬────────────┘  └────────────┬────────────┘
             │                            │
             └──────────────┬─────────────┘
                    Bidirectional Replication
                    (Conflict-Free Replication Data Type)
                    RPO: <100ms
                    RTO: <5 seconds
```

### Conflict Resolution Strategies

```
Strategy 1: Last-Write-Wins (LWW)
  ├─ Timestamp-based conflict resolution
  ├─ Best for: Non-critical data, metrics
  ├─ Latency: Minimal overhead
  ├─ Consistency: Eventual (few ms)
  └─ Example: User profile updates

Strategy 2: CRDT (Conflict-Free RDT)
  ├─ Mathematical data structure that resolves conflicts
  ├─ Best for: Counters, sets, maps
  ├─ Latency: No conflict resolution needed
  ├─ Consistency: Strong eventual consistency
  └─ Example: Likes counter = CRDT increment operations

Strategy 3: Application-Level Resolution
  ├─ Custom logic for conflict resolution
  ├─ Best for: Business-critical data
  ├─ Latency: Varies (may require human review)
  ├─ Consistency: Strong (manual review)
  └─ Example: Account balances = require reconciliation

Strategy 4: Write Partitioning
  ├─ Different data sets write to different regions
  ├─ Best for: Partitioned data (tenant-based)
  ├─ Latency: No conflicts (no resolution)
  ├─ Consistency: Strong within partition
  └─ Example: Tenant A writes to US-East, Tenant B to US-West
```

### Implementation Checklist

```
Phase 1: Replication Infrastructure (1 Month)
  [ ] Deploy 2nd region cluster (mirror of 1st)
  [ ] Setup network links (IPsec VPN or Direct Connect)
  [ ] Configure bidirectional replication pipeline
  [ ] Test failover/recovery procedures
  [ ] Document runbooks for regional failover

Phase 2: Conflict Resolution (2 Weeks)
  [ ] Implement conflict detection
  [ ] Choose resolution strategy per data type
  [ ] Code review for correctness
  [ ] Load test with conflicting writes
  [ ] Monitor conflict frequency (target: <0.1%)

Phase 3: Application Integration (2-3 Weeks)
  [ ] Update application to write to both regions
  [ ] Implement dual-read (try both regions)
  [ ] Add latency monitoring
  [ ] Graceful degradation (one region down)
  [ ] Customer notification & testing

Phase 4: Production Rollout (1-2 Weeks)
  [ ] Canary rollout (10% of traffic)
  [ ] Monitor conflict rate, latency, errors
  [ ] Gradual increase to 100%
  [ ] Keep manual failover procedure ready
  [ ] 30 days of monitoring before full HA claim
```

---

## ⚠️ OPERATIONAL CHALLENGES AT SCALE

### Challenge 1: Rebalancing Time

**Problem:** As cluster grows, rebalancing takes longer

```
Cluster Size   Data per Shard   Rebalance Time   Throughput Impact
────────────────────────────────────────────────────────────────
8 shards       100GB            90s              -12%
16 shards      125GB            180s             -12%
32 shards      125GB            360s             -15%
64 shards      125GB            600s+ (10 min)   -20% (risky)
```

**Solutions:**
```
1. Hierarchical Rebalancing
   └─ Don't rebalance all shards at once
   └─ Rebalance 4 shards at a time
   └─ Reduce impact: -5% instead of -20%
   └─ Duration: 10 min instead of 10 min (sequential)

2. Background Rebalancing
   └─ Run during off-peak hours
   └─ Limit bandwidth to 5% network
   └─ Duration: 1-2 hours
   └─ Impact: <1% throughput

3. Virtual Sharding
   └─ Each physical shard = 4 virtual shards
   └─ Move virtual shards instead of data
   └─ Rebalance time: ~30s
   └─ Complexity: Higher (4x routing overhead)
```

### Challenge 2: Network Bandwidth

**Problem:** Cross-shard communication becomes bottleneck

```
Cluster Size   Network per Shard   Total Network   Single Shard Share
──────────────────────────────────────────────────────────────────
8 shards       400Mbps             3.2Gbps         12.5%
16 shards      400Mbps             6.4Gbps         6.25%
32 shards      400Mbps             12.8Gbps        3.1%
64 shards      400Mbps             25.6Gbps        1.6%
```

**Mitigation:**
```
1. Network Design
   ├─ Dedicated inter-shard network (separate from client traffic)
   ├─ 40-100Gbps fabric (not shared with clients)
   ├─ Network isolation per region
   └─ Priority queuing (rebalance < queries < writes)

2. Query Optimization
   ├─ Minimize cross-shard joins
   ├─ Push filters to shards
   ├─ Cache common query results
   └─ Use approximate queries for large scans

3. Data Locality
   ├─ Co-locate related data (same shard)
   ├─ Use locality-aware sharding key
   └─ Batch related queries together
```

### Challenge 3: Monitoring Complexity

**Problem:** 64 shards = 64× more metrics to track

```
Metrics per Shard: 50+ (throughput, latency, errors, etc)
Total Metrics: 64 × 50 = 3,200 metrics
Time Series: 3,200 × 12 months × 80k samples = 3.8TB storage

Cardinality Explosion:
  Shard × QueryType × DataType × Quantile × ... = >100k unique metric names
```

**Solution Architecture:**
```
1. Hierarchical Monitoring
   ├─ Cluster Level: Summary metrics only (10 metrics)
   ├─ Region Level: 100 metrics
   ├─ Shard Level: 50 metrics (on-demand)
   └─ Instance Level: 100+ metrics (detailed troubleshooting)

2. Metric Aggregation
   ├─ Pre-aggregate per-shard to region level
   ├─ Store raw metrics for 7 days
   ├─ Store aggregated metrics for 1 year
   ├─ Use sampling for detailed metrics

3. Alert Rationalization
   ├─ 500+ possible alerts (too many!)
   ├─ Group by: Severity, Impact, Frequency
   ├─ Only alert on >3 affected shards
   ├─ Intelligent deduplication (suppress cascading alerts)

Example Alert Policy:
  • Single shard degraded: INFO (page once/day max)
  • 3+ shards degraded: WARN (page immediately)
  • Full region degraded: CRITICAL (page CEO)
```

---

## 📊 DECISION MATRIX

**Which Configuration to Choose?**

```
Scenario                        Recommended      Cost/Month  Effort
───────────────────────────────────────────────────────────────────
<500GB data, <2M ops/sec        Single Node      $5-10K      Low
500GB-1.5TB, 2-6M ops/sec       8 Shards         $60K        Medium
1.5-4TB, 6-20M ops/sec          16-32 Shards     $120-240K   High
4TB+, 20M+ ops/sec, Global HA   32-64+ Shards    $300K+      Very High
AI/ML Feature Store (100M+)      Specialized      Custom      Custom

Decision Tree:
─────────────────────────────────────────────────────────
Q1: Total data size?
    <500GB → Single Node
    500GB-2TB → 8 Shards
    2TB-4TB → 16-32 Shards
    4TB+ → 32-64+ Shards

Q2: Geographic requirements?
    Single Region → Use cluster in that region
    Multi-Region → 2-4 active regions, cross-region replication

Q3: Consistency requirements?
    Strong consistency only → Single region (8-16 shards)
    Eventual acceptable → Multi-region (async replication)

Q4: Budget constraints?
    <$100K/month → Max 8 shards
    $100K-200K → 16 shards
    $200K-500K → 32 shards
    >$500K → 64+ shards (enterprise)
```

---

## 🔮 FUTURE ROADMAP

### v1.5 (Q3 2026): Enhanced Sharding

```
Features:
  ✓ Hierarchical Sharding (support 64+ shards easily)
  ✓ Virtual Sharding (faster rebalancing)
  ✓ Multi-tenancy (tenant-aware routing)
  ✓ Smart Data Placement (ML-based optimization)

Performance Target:
  ✓ 100k ops/sec per core (vs 50k today)
  ✓ Rebalance <60s for 64 shards
  ✓ Multi-region latency <50ms
```

### v1.6 (Q4 2026): Distributed Features

```
Features:
  ✓ Distributed Transactions (ACID across shards)
  ✓ Global Secondary Indexes (query all shards)
  ✓ Materialized Views (cross-shard aggregations)
  ✓ Continuous Replication (event streaming)

Support:
  ✓ Distributed joins (guaranteed correct results)
  ✓ Transactions spanning multiple shards
  ✓ ACID properties across regions
```

### v2.0 (2027): Cloud-Native Sharding

```
Features:
  ✓ Elastic Sharding (auto-scale up/down)
  ✓ Serverless Shards (pay per operation)
  ✓ AI-Driven Optimization (auto-tuning)
  ✓ Multi-Cloud Support (AWS, Azure, GCP)

Operations:
  ✓ Zero-downtime upgrades
  ✓ Automatic failover (no manual intervention)
  ✓ Chaos engineering (auto-recovery)
```

---

## 🎯 RECOMMENDATIONS

### For Current Users (8 Shard Cluster)

```
✅ DO:
  • Plan for 16-shard migration path (6-12 months out)
  • Monitor data growth rate (is linear scaling needed?)
  • Test rebalancing procedures regularly
  • Establish multi-region backup (even single region deployment)

❌ DON'T:
  • Jump to 32+ shards without 16-shard validation
  • Deploy multi-region without strong consistency requirements
  • Scale shards without understanding failure modes
  • Change sharding key (almost impossible post-deployment)
```

### For New Customers

```
✅ RECOMMENDED:
  • Start with 8-shard cluster (validate benchmarks)
  • Plan for 16-shard as data grows (6-12 months)
  • Keep architecture flexible (allow 32+ shard migration)
  • Budget for 2-year growth (not just current state)

ANTI-PATTERNS:
  • Starting with 64 shards (massive overhead, 90% unused)
  • Mix of 4 and 8-shard clusters (operational nightmare)
  • Homogeneous deployment (no regional redundancy)
```

---

## 📚 REFERENCE DOCUMENTS

- [SHARDING_BENCHMARK_PLAN_v1.4.md](SHARDING_BENCHMARK_PLAN_v1.4.md) - Baseline metrics
- [SHARDING_PRODUCTION_DEPLOYMENT_v1.4.md](SHARDING_PRODUCTION_DEPLOYMENT_v1.4.md) - Implementation for 8 shards
- [SHARDING_MONITORING_OBSERVABILITY_v1.4.md](SHARDING_MONITORING_OBSERVABILITY_v1.4.md) - Monitoring at scale

---

**Version:** 1.0 | **Status:** Planning Document | **Last Updated:** April 2026

For architectural discussions, contact: architecture@themis-io.com
