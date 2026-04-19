> ⚠️ **Historischer Plan** – Dieser Plan beschreibt den Entwicklungsstand zum Zeitpunkt der Erstellung.
> Für aktuellen Teststatus: `ctest --preset linux-ninja-release --test-dir build/linux-ninja-release -R <pattern>` verwenden.

# ThemisDB Multi-Shard RAID Benchmark Evaluation

**Version:** 2.0  
**Created:** 11. Dezember 2025  
**Status:** Implementation Ready  
**Execution Timeline:** 3-4 Tage (72-96 Stunden)  
**Purpose:** Umfassende Performance-Evaluation von Multi-Shard ThemisDB mit verschiedenen RAID-Konfigurationen

---

## Executive Summary

Dieses Benchmark-Szenario evaluiert ThemisDB in realistischen Multi-Shard-Deployments mit verschiedenen RAID-Konfigurationen. Ziel ist die Identifikation optimaler Storage- und Sharding-Strategien für verschiedene Workload-Typen.

### Kern-Dimensionen

1. **Shard-Topologien:** 3, 6, 12, 24 Shards
2. **RAID-Level:** RAID0, RAID1, RAID5, RAID6, RAID10
3. **Workload-Typen:** OLTP, OLAP, Mixed, Time-Series, Vector Search
4. **Datenvolumen:** 100GB, 500GB, 1TB pro Testfall
5. **Netzwerk-Topologien:** Single DC, Multi-DC, Cross-Region

### Key Performance Indicators (KPIs)

- **Throughput:** Queries/sec, MB/s (Read/Write)
- **Latency:** P50, P95, P99, P999 (ms)
- **Skalierbarkeit:** Linear vs. Sub-Linear Scaling
- **Ausfallsicherheit:** MTTR, RPO, RTO
- **Kosteneffizienz:** $/query, $/GB, TCO

---

## Test-Matrix Overview

| Szenario | Shards | RAID | Workload | Daten | Dauer | Priorität |
|----------|--------|------|----------|-------|-------|-----------|
| **S1** | 3 | RAID0 | OLTP | 100GB | 4h | HIGH |
| **S2** | 3 | RAID1 | OLTP | 100GB | 6h | HIGH |
| **S3** | 3 | RAID5 | OLTP | 100GB | 8h | HIGH |
| **S4** | 6 | RAID10 | Mixed | 500GB | 12h | HIGH |
| **S5** | 12 | RAID6 | OLAP | 1TB | 18h | MEDIUM |
| **S6** | 24 | RAID10 | Time-Series | 1TB | 24h | MEDIUM |
| **S7** | 6 | RAID5 | Vector Search | 500GB | 10h | HIGH |
| **S8** | 12 | RAID1 | Multi-DC Failover | 500GB | 16h | HIGH |

**Total:** 98 Stunden (parallelisierbar auf 72h mit 2 parallelen Test-Clustern)

---

## Phase 1: Single-DC Multi-Shard Benchmarks (Tag 1-2)

### Szenario S1: 3 Shards + RAID0 (Baseline Performance)

**Ziel:** Maximale Performance ohne Redundanz (Baseline für Vergleiche)

**Hardware-Konfiguration:**
```yaml
cluster:
  shards: 3
  nodes_per_shard: 1
  total_nodes: 3

node_specs:
  cpu: 8 cores (AMD EPYC/Intel Xeon)
  ram: 32GB
  storage: 4x 500GB NVMe SSD
  network: 10Gbps Ethernet

raid_config:
  level: RAID0 (Striping)
  stripe_size: 128KB
  disks_per_shard: 4
  capacity_per_shard: 2TB (raw)
  expected_throughput: 3000 MB/s read, 2500 MB/s write
```

**Sharding-Strategie:**
```yaml
sharding:
  method: consistent_hashing
  replication_factor: 1  # Keine Replikation (RAID0 ist nicht redundant)
  partitioning_key: document_id
  virtual_nodes: 150 per shard
```

**Test-Daten:**
```python
dataset:
  total_size: 100GB
  documents: 5,000,000
  avg_doc_size: 20KB
  distribution: uniform across shards
  
document_structure:
  - id: UUID v4
  - timestamp: ISO8601
  - category: enum (10 values)
  - content: text (1-50KB)
  - metadata: JSON (nested, 5-10 fields)
  - vector: float32[768] (embeddings)
```

**Workload-Definition (OLTP):**
```yaml
operations:
  - type: point_read
    ratio: 40%
    target_latency_p99: 5ms
    
  - type: point_write
    ratio: 30%
    target_latency_p99: 10ms
    
  - type: range_scan
    ratio: 20%
    range_size: 100-1000 docs
    target_latency_p99: 50ms
    
  - type: vector_search
    ratio: 10%
    k: 10
    target_latency_p99: 20ms

workload_profile:
  duration: 4 hours
  ramp_up: 10 minutes
  steady_state: 3h 40m
  cool_down: 10 minutes
  
  concurrent_clients: 128
  target_qps: 10,000
```

**Metrics-Sammlung:**
```yaml
metrics:
  system:
    - cpu_utilization (per core)
    - memory_usage (RSS, cache, swap)
    - disk_io (IOPS, throughput, latency)
    - network_io (packets/s, bandwidth)
    - context_switches
    
  application:
    - query_latency (P50, P95, P99, P999)
    - throughput (QPS)
    - cache_hit_rate
    - index_efficiency
    - gc_pause_time
    
  distributed:
    - cross_shard_queries (count, latency)
    - scatter_gather_overhead
    - network_hop_count
    - data_skew (per shard)
```

**Erwartete Ergebnisse:**
- **Throughput:** 10,000+ QPS sustained
- **Latency P99:** <10ms (point operations), <50ms (range scans)
- **CPU Usage:** 60-70% average
- **IOPS:** 20,000+ per shard
- **Network:** <30% utilization on 10Gbps link

---

### Szenario S2: 3 Shards + RAID1 (High Availability)

**Ziel:** Ausfallsicherheit mit Mirroring bei moderater Performance

**RAID-Konfiguration:**
```yaml
raid_config:
  level: RAID1 (Mirroring)
  mirrors: 2 (primary + 1 replica)
  disks_per_shard: 4 (2 mirror pairs)
  capacity_per_shard: 1TB (effective)
  sync_mode: synchronous
  expected_throughput: 
    read: 3000 MB/s (beide Mirrors lesbar)
    write: 1200 MB/s (Sync-Overhead)
```

**Sharding mit Replikation:**
```yaml
sharding:
  method: consistent_hashing
  replication_factor: 2  # RAID1 auf Storage-Layer + Shard-Replikation
  replica_placement: cross_rack
  consistency_level: QUORUM (W=2, R=1)
```

**Failover-Test:**
```yaml
failover_scenarios:
  - scenario: disk_failure
    trigger: kill_disk_2 on shard_1
    expected_behavior:
      - automatic_failover: <500ms
      - read_degradation: <10%
      - write_degradation: <5%
      - rebuild_time: <30 minutes for 333GB
      
  - scenario: node_failure
    trigger: stop_shard_1_primary
    expected_behavior:
      - replica_promotion: <2 seconds
      - query_rerouting: automatic
      - data_loss: 0 writes
      - downtime: <3 seconds
```

**Workload (Gleich wie S1, plus Failover-Simulation):**
```yaml
test_phases:
  - phase: steady_state
    duration: 2 hours
    
  - phase: disk_failure_injection
    time: T+2h
    action: simulate_disk_failure shard_1 disk_2
    observe: 30 minutes
    
  - phase: node_failure_injection
    time: T+2h30m
    action: stop_node shard_2
    observe: 30 minutes
    
  - phase: recovery_validation
    duration: 1 hour
    verify: data_consistency, performance_restoration
```

**Erwartete Ergebnisse vs. S1:**
- **Throughput:** 8,000-9,000 QPS (10-20% Degradierung wegen Sync-Overhead)
- **Write Latency P99:** +30-50% (Mirroring-Overhead)
- **Read Latency:** Ähnlich zu S1 (beide Mirrors lesbar)
- **Failover Time:** <3 Sekunden (Node), <500ms (Disk)
- **Data Loss:** 0

---

### Szenario S3: 3 Shards + RAID5 (Balanced)

**Ziel:** Optimaler Kompromiss zwischen Performance, Kapazität und Ausfallsicherheit

**RAID-Konfiguration:**
```yaml
raid_config:
  level: RAID5 (Striping + Distributed Parity)
  disks_per_shard: 4
  stripe_size: 128KB
  parity_algorithm: XOR
  capacity_per_shard: 1.5TB (75% efficiency bei 4 disks)
  expected_throughput:
    read: 2400 MB/s (3 data disks parallel)
    write: 800 MB/s (Read-Modify-Write für Parity)
    random_write: 400 MB/s (worst case)
```

**Write-Penalty-Analyse:**
```yaml
raid5_overhead:
  sequential_write: 1.5x overhead (1 parity write per 3 data writes)
  random_write: 4x overhead (Read old data, Read old parity, Write new data, Write new parity)
  
mitigation:
  - use_nvme_ssds: reduces latency impact
  - write_coalescing: batch writes to same stripe
  - parity_cache: 2GB DRAM per shard for hot parity blocks
```

**Workload-Optimierung:**
```yaml
# RAID5 ist schlecht für Random Writes, gut für Sequential Reads
workload_adjustments:
  - reduce_random_writes: 30% → 15%
  - increase_range_scans: 20% → 35%
  - add_bulk_imports: 5% (sequential writes)
  
operations:
  - point_read: 40%
  - point_write: 15% (reduced)
  - range_scan: 35% (increased)
  - vector_search: 5%
  - bulk_import: 5%
```

**Degraded-Mode-Test:**
```yaml
degraded_operation:
  trigger: fail_1_disk_per_shard
  expected_behavior:
    read_performance: -40% (parity reconstruction needed)
    write_performance: -20%
    rebuild_rate: 50 MB/s per disk
    rebuild_time: ~3 hours for 500GB disk
```

**Erwartete Ergebnisse:**
- **Throughput:** 7,000-8,000 QPS (Read-heavy workload)
- **Random Write Latency:** +100-150% vs. RAID0
- **Sequential Read Throughput:** 90% von RAID0
- **Storage Efficiency:** 75% (vs. 50% bei RAID1)
- **Degraded Mode:** -40% Read Performance

---

## Phase 2: Scale-Out Multi-Shard Benchmarks (Tag 2-3)

### Szenario S4: 6 Shards + RAID10 (Production Standard)

**Ziel:** Realistic Production Deployment mit Balance aus Performance und Redundanz

**Cluster-Konfiguration:**
```yaml
cluster:
  shards: 6
  nodes_per_shard: 2 (Primary + Hot Standby)
  total_nodes: 12
  
node_specs:
  cpu: 16 cores
  ram: 64GB
  storage: 6x 1TB NVMe SSD
  network: 25Gbps Ethernet
  
raid_config:
  level: RAID10 (Mirrored Stripes)
  disks_per_shard: 6 (3 mirror pairs)
  stripe_size: 256KB
  capacity_per_shard: 3TB (50% efficiency)
  expected_throughput:
    read: 6000 MB/s (alle 6 Disks parallel lesbar)
    write: 3000 MB/s (3 Stripes parallel, Mirroring-Overhead)
```

**Sharding-Topologie:**
```yaml
shard_distribution:
  datacenter: single_dc
  racks: 3 (2 shards per rack)
  placement_strategy: rack_aware
  
  shard_0: rack_1 (primary), rack_2 (standby)
  shard_1: rack_1 (primary), rack_3 (standby)
  shard_2: rack_2 (primary), rack_3 (standby)
  shard_3: rack_2 (primary), rack_1 (standby)
  shard_4: rack_3 (primary), rack_1 (standby)
  shard_5: rack_3 (primary), rack_2 (standby)
```

**Test-Daten (Skaliert):**
```yaml
dataset:
  total_size: 500GB
  documents: 25,000,000
  avg_doc_size: 20KB
  distribution: 
    method: consistent_hashing
    skew: 10% (realistic imbalance)
    
  shard_sizes:
    shard_0: 90GB (110%)
    shard_1: 85GB (106%)
    shard_2: 80GB (100%)
    shard_3: 78GB (97%)
    shard_4: 82GB (102%)
    shard_5: 85GB (106%)
```

**Mixed Workload:**
```yaml
workload:
  oltp_phase:
    duration: 4 hours
    operations:
      point_read: 50%
      point_write: 25%
      range_scan: 15%
      vector_search: 10%
    target_qps: 50,000
    
  olap_phase:
    duration: 4 hours
    operations:
      cross_shard_join: 30%
      aggregation: 40%
      full_table_scan: 20%
      vector_batch_search: 10%
    target_qps: 500 (Komplexe Queries)
    
  mixed_phase:
    duration: 4 hours
    operations: 70% OLTP + 30% OLAP
    target_qps: 30,000 OLTP + 200 OLAP
```

**Cross-Shard-Operationen:**
```yaml
distributed_queries:
  - type: scatter_gather_join
    pattern: join across all 6 shards
    expected_latency_p99: 200ms
    overhead: 5-10x vs single-shard
    
  - type: distributed_aggregation
    pattern: map-reduce across 3-6 shards
    expected_latency_p99: 150ms
    optimization: push-down predicates
    
  - type: global_secondary_index
    pattern: lookup in 2-3 shards on average
    expected_latency_p99: 30ms
```

**Erwartete Ergebnisse:**
- **OLTP Throughput:** 50,000+ QPS
- **OLAP Throughput:** 500+ complex queries/sec
- **Cross-Shard Join Latency:** P99 < 200ms
- **Storage Efficiency:** 50% (RAID10)
- **Failover Time:** <5 seconds (Hot Standby)
- **Linear Scaling:** 90-95% efficiency vs. 3-Shard baseline

---

### Szenario S5: 12 Shards + RAID6 (Data Warehouse)

**Ziel:** Large-Scale OLAP mit hoher Ausfallsicherheit (2 Disk Failures tolerierbar)

**RAID-Konfiguration:**
```yaml
raid_config:
  level: RAID6 (Double Distributed Parity)
  disks_per_shard: 8
  stripe_size: 256KB
  parity_disks: 2 (P + Q)
  capacity_per_shard: 6TB (75% efficiency bei 8 disks)
  expected_throughput:
    read: 5000 MB/s (6 data disks parallel)
    write: 600 MB/s (Read-Modify-Write mit 2 Parity)
```

**OLAP-Optimiertes Schema:**
```yaml
dataset:
  total_size: 1TB
  fact_tables: 800GB (80%)
  dimension_tables: 200GB (20%)
  
  fact_table_structure:
    - sales_fact: 500GB, 2B rows
    - inventory_fact: 200GB, 800M rows
    - web_clicks_fact: 100GB, 5B rows
    
  dimension_tables:
    - product_dim: 50GB, 100M SKUs
    - customer_dim: 80GB, 500M customers
    - time_dim: 10GB, 10 years daily grain
    - geography_dim: 60GB, 1M locations
```

**OLAP-Workload:**
```yaml
queries:
  - type: star_schema_join
    pattern: fact + 3-4 dimensions
    target_latency_p95: 2 seconds
    
  - type: aggregation_rollup
    pattern: SUM/AVG/COUNT with GROUP BY
    target_latency_p95: 5 seconds
    
  - type: window_function
    pattern: RANK(), ROW_NUMBER(), LAG()
    target_latency_p95: 10 seconds
    
  - type: full_table_scan
    pattern: scan 100-500GB
    target_duration: 30-120 seconds
    
concurrent_users: 50 analysts
query_cache_size: 10GB per shard
columnar_compression: LZ4 (2-4x compression)
```

**Erwartete Ergebnisse:**
- **Query Throughput:** 200-500 concurrent complex queries
- **Scan Throughput:** 3-5 GB/s aggregate
- **Join Performance:** 2-5 seconds for star schema
- **Storage Efficiency:** 75% mit 2-Disk-Failure-Tolerance
- **Rebuild Time:** ~6 hours per failed disk (1TB at 50 MB/s)

---

## Phase 3: Multi-DC und Geo-Distributed Benchmarks (Tag 3-4)

### Szenario S8: 12 Shards + RAID1 + Multi-DC Failover

**Ziel:** Geographic Redundanz mit Cross-Region Replication

**Geo-Topologie:**
```yaml
datacenters:
  - dc_west:
      location: us-west-2
      shards: [0, 1, 2, 3]
      latency_to_dc_east: 80ms
      
  - dc_east:
      location: us-east-1
      shards: [4, 5, 6, 7]
      latency_to_dc_central: 40ms
      
  - dc_central:
      location: eu-central-1
      shards: [8, 9, 10, 11]
      latency_to_dc_west: 150ms

replication_topology:
  primary_dc: dc_west (shards 0-3)
  secondary_dc: dc_east (shards 0-3 replicas)
  tertiary_dc: dc_central (shards 0-3 async replicas)
  
  replication_lag_target:
    dc_west → dc_east: <100ms (synchronous)
    dc_west → dc_central: <1000ms (asynchronous)
```

**Multi-DC-Failover-Szenarien:**
```yaml
disaster_recovery_tests:
  - scenario: dc_west_total_failure
    trigger: shutdown_datacenter dc_west
    expected_behavior:
      failover_time: <30 seconds
      promote_dc: dc_east
      rpo: 0 writes (sync replication)
      rto: <30 seconds
      
  - scenario: network_partition
    trigger: block_traffic dc_west ↔ dc_east
    expected_behavior:
      split_brain_prevention: enabled
      quorum_loss_detection: <10 seconds
      write_availability: dc_west only (majority partition)
      
  - scenario: rolling_regional_failover
    trigger: simulate_maintenance_mode
    expected_behavior:
      zero_downtime: true
      client_redirect: automatic
      performance_degradation: <15%
```

**Geo-Distributed-Workload:**
```yaml
traffic_distribution:
  dc_west_clients: 40% (west coast users)
  dc_east_clients: 35% (east coast users)
  dc_central_clients: 25% (european users)

locality_optimization:
  read_preference: nearest_dc
  write_routing: primary_dc + sync_replica
  cache_strategy: regional_cache_per_dc
  
consistency_model: causal_consistency
  read_your_writes: true
  monotonic_reads: true
  cross_dc_eventual_consistency: <500ms
```

**Erwartete Ergebnisse:**
- **Local Read Latency:** P99 < 10ms (within same DC)
- **Cross-DC Read Latency:** P99 < 100ms (West ↔ East)
- **Write Latency:** P99 < 150ms (sync replication to 2 DCs)
- **DC Failover Time:** <30 seconds (RTO)
- **Data Loss:** 0 (RPO = 0 with sync replication)
- **Network Overhead:** ~200-300 Mbps per DC pair (replication traffic)

---

## Test-Infrastruktur und Tooling

### Docker-Compose Multi-Shard Setup

```yaml
# docker-compose.multi-shard-raid.yml
version: '3.9'

services:
  # Shard 0 - RAID Controller + ThemisDB
  shard-0-raid:
    image: linuxserver/mdadm:latest
    volumes:
      - /dev/sdb:/dev/sdb  # Disk 1
      - /dev/sdc:/dev/sdc  # Disk 2
      - /dev/sdd:/dev/sdd  # Disk 3
      - /dev/sde:/dev/sde  # Disk 4
    cap_add:
      - SYS_ADMIN
    command: |
      mdadm --create /dev/md0 --level=${RAID_LEVEL} --raid-devices=4 /dev/sdb /dev/sdc /dev/sdd /dev/sde
      mkfs.ext4 /dev/md0
      
  themis-shard-0:
    image: themisdb/themisdb:latest
    depends_on:
      - shard-0-raid
    volumes:
      - /dev/md0:/data
    environment:
      SHARD_ID: "shard_0"
      SHARD_ROLE: "primary"
      RAID_LEVEL: "${RAID_LEVEL}"
      CLUSTER_SIZE: "${NUM_SHARDS}"
    ports:
      - "8080:8765"
    deploy:
      resources:
        limits:
          cpus: '8'
          memory: 32G
          
  # Replicate for shard-1, shard-2, ... shard-N
  # (Template above, multiply by NUM_SHARDS)
  
  # Monitoring Stack
  prometheus:
    image: prom/prometheus:latest
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
    ports:
      - "9090:9090"
      
  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"
    environment:
      GF_SECURITY_ADMIN_PASSWORD: admin
      
  # Benchmark Orchestrator
  benchmark-controller:
    image: themisdb/benchmark-suite:latest
    volumes:
      - ./benchmarks:/benchmarks
      - ./results:/results
    environment:
      TARGET_CLUSTER: "shard-0,shard-1,shard-2,shard-3,shard-4,shard-5"
      WORKLOAD_TYPE: "${WORKLOAD}"
      DURATION_HOURS: "${DURATION}"
    command: python3 /benchmarks/run_multi_shard_raid_benchmark.py
```

### Benchmark-Orchestrator (Python)

```python
# run_multi_shard_raid_benchmark.py

import asyncio
import aiohttp
import time
from dataclasses import dataclass
from typing import List, Dict
import json

@dataclass
class ShardConfig:
    shard_id: str
    endpoint: str
    raid_level: str
    disk_count: int
    capacity_gb: int

@dataclass
class BenchmarkResult:
    scenario: str
    shard_count: int
    raid_level: str
    throughput_qps: float
    latency_p50_ms: float
    latency_p99_ms: float
    cpu_usage_pct: float
    disk_iops: int
    network_mbps: float

class MultiShardRaidBenchmark:
    def __init__(self, shards: List[ShardConfig], workload_type: str):
        self.shards = shards
        self.workload_type = workload_type
        self.results: List[BenchmarkResult] = []
        
    async def run_scenario(self, scenario_name: str, duration_hours: int):
        """Execute a complete benchmark scenario"""
        print(f"Starting scenario: {scenario_name}")
        print(f"Shards: {len(self.shards)}, RAID: {self.shards[0].raid_level}")
        
        # Phase 1: Data Loading
        await self.load_test_data()
        
        # Phase 2: Warmup
        await self.warmup_phase(duration_minutes=10)
        
        # Phase 3: Steady State Workload
        result = await self.steady_state_workload(duration_hours)
        
        # Phase 4: Failover Test (if applicable)
        if "failover" in scenario_name.lower():
            await self.test_failover()
            
        # Phase 5: Collect Metrics
        self.results.append(result)
        return result
        
    async def load_test_data(self):
        """Distribute test data across shards"""
        tasks = []
        for shard in self.shards:
            task = self.load_data_to_shard(shard)
            tasks.append(task)
        await asyncio.gather(*tasks)
        
    async def steady_state_workload(self, duration_hours: int) -> BenchmarkResult:
        """Execute workload for specified duration"""
        start_time = time.time()
        end_time = start_time + (duration_hours * 3600)
        
        query_count = 0
        latencies = []
        
        while time.time() < end_time:
            # Execute queries across shards
            latency = await self.execute_query_batch()
            latencies.append(latency)
            query_count += 100  # Batch size
            
            # Log progress every 5 minutes
            if int(time.time() - start_time) % 300 == 0:
                self.log_progress(query_count, latencies)
                
        return self.calculate_results(query_count, latencies)
```

### Metrics-Collection (Prometheus Config)

```yaml
# prometheus.yml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'themis-shards'
    static_configs:
      - targets:
        - 'themis-shard-0:8765'
        - 'themis-shard-1:8765'
        - 'themis-shard-2:8765'
        - 'themis-shard-3:8765'
        - 'themis-shard-4:8765'
        - 'themis-shard-5:8765'
        
  - job_name: 'node-exporter'
    static_configs:
      - targets:
        - 'shard-0-node-exporter:9100'
        - 'shard-1-node-exporter:9100'
        # ... per shard
        
  - job_name: 'raid-monitor'
    static_configs:
      - targets: ['mdadm-exporter:9101']
```

---

## Ausführung und Zeitplan

### Tagesablauf (72 Stunden)

**Tag 1 (24h):**
- 00:00-04:00: S1 (3 Shards + RAID0)
- 04:00-10:00: S2 (3 Shards + RAID1)
- 10:00-18:00: S3 (3 Shards + RAID5)
- 18:00-24:00: S4 Setup + Start (6 Shards + RAID10)

**Tag 2 (24h):**
- 00:00-06:00: S4 Fortsetzung (6 Shards + RAID10)
- 06:00-24:00: S5 (12 Shards + RAID6 OLAP)

**Tag 3 (24h):**
- 00:00-16:00: S8 (12 Shards + Multi-DC)
- 16:00-20:00: Datenanalyse und Report-Generierung
- 20:00-24:00: Puffer für Re-Runs

---

## Deliverables

1. **Benchmark-Report (PDF + Markdown):**
   - Executive Summary
   - Detaillierte Ergebnisse pro Szenario
   - Performance-Vergleichstabellen
   - Grafana-Dashboards (Screenshots)

2. **Raw-Daten (JSON/CSV):**
   - Alle Metriken im Zeitverlauf
   - Query-Logs mit Latencies
   - System-Metrics (CPU, RAM, Disk, Network)

3. **Empfehlungen:**
   - Optimale RAID-Konfiguration pro Workload-Typ
   - Shard-Count-Sizing-Guide
   - Cost-Performance-Matrix

4. **Docker-Images und Scripts:**
   - Reproduzierbare Test-Umgebung
   - Automatisierte Benchmark-Suite
   - CI/CD-Integration für Regressionstests

---

## Success Criteria

✅ **Performance:**
- S1 (RAID0): >10,000 QPS sustained
- S4 (RAID10): >50,000 QPS sustained
- S5 (RAID6 OLAP): >200 concurrent complex queries

✅ **Skalierbarkeit:**
- Linear scaling: >90% efficiency when doubling shards
- Cross-shard overhead: <2x latency vs single-shard

✅ **Ausfallsicherheit:**
- RAID1/10/6: Zero data loss on disk/node failure
- Failover time: <30 seconds for Multi-DC
- Rebuild time: Predictable and non-disruptive

✅ **Reproduzierbarkeit:**
- All tests automated via Docker Compose
- Results variance: <5% across multiple runs

---

**Status:** Ready for Implementation  
**Next Steps:** 
1. Provision Hardware/Cloud Resources
2. Deploy Docker Compose Stack
3. Execute Phase 1 Benchmarks (Tag 1)
