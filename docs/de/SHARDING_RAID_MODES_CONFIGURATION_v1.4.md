# RAID-Themis Betriebsmodi & Konfigurationsbeispiele v1.4

**Version:** 1.4 (RAID-Angepasst)  
**Stand:** 6. April 2026  
**Status:** ✅ Produktionsreife Konfigurations-Templates  
**Kategorie:** 🛡️ RAID-Redundanz | 🔧 Konfiguration | 📋 Best Practices

---

## Executive Summary

Dieses Dokument beschreibt die **praktischen Konfigurationsbeispiele** für alle 6 RAID-ähnlichen Redundanzmodi in RAID-Themis. Für jeden Modus:

- ✅ Detaillierte YAML-Konfiguration
- ✅ Performance-Merkmale & Trade-offs
- ✅ Deployment-Szenarios
- ✅ Sizing Guide
- ✅ Operational Playbooks

---

## 📑 Inhaltsverzeichnis

1. [RAID-Modi Überblick](#1-raid-modi-überblick)
2. [NONE Mode - Single Shard (Baseline)](#2-none-mode---single-shard-baseline)
3. [MIRROR Mode - Vollständige Spiegelung (RAID-1)](#3-mirror-mode---vollständige-spiegelung-raid-1)
4. [STRIPE Mode - Daten-Striping (RAID-0)](#4-stripe-mode---daten-striping-raid-0)
5. [STRIPE_MIRROR Mode - Kombiniert (RAID-10)](#5-stripe_mirror-mode---kombiniert-raid-10)
6. [PARITY Mode - Erasure Coding (RAID-5/6)](#6-parity-mode---erasure-coding-raid-56)
7. [GEO_MIRROR Mode - Multi-Region](#7-geo_mirror-mode---multi-region)
8. [Entscheidungsmatrix](#8-entscheidungsmatrix)
9. [Migration zwischen Modi](#9-migration-zwischen-modi)

---

## 1. RAID-Modi Überblick

### Vergleichstabelle

```
┌─────────────────┬──────────┬────────────┬─────────┬───────────┬──────────────┐
│ Modus           │ RF*      │ Speicher   │ Through │ Latency   │ Ausfallsicher │
├─────────────────┼──────────┼────────────┼─────────┼───────────┼──────────────┤
│ NONE            │ 1        │ 100%       │ 1×      │ 1×        │ 0 Shards     │
│ MIRROR (RF=2)   │ 2        │ 50%        │ 2×      │ 1.2×      │ 1 Shard      │
│ MIRROR (RF=3)   │ 3        │ 33%        │ 3×      │ 1.5×      │ 2 Shards     │
│ STRIPE (RF=1)   │ 1        │ 100%       │ 4×      │ 0.8×      │ 0 Shards     │
│ STRIPE_MIRROR   │ 2        │ 50%        │ 2-3×    │ 1×        │ 1 Shard      │
│ PARITY (4+2)    │ 4+2      │ 67%        │ 1.5×    │ 1.3×      │ 2 Shards     │
│ PARITY (8+3)    │ 8+3      │ 73%        │ 1.3×    │ 1.5×      │ 3 Shards     │
│ GEO_MIRROR      │ 3        │ 33%        │ Lokal 3× │ Lokal 2×  │ 2 DCs        │
└─────────────────┴──────────┴────────────┴─────────┴───────────┴──────────────┘

* RF = Replication Factor
```

### Redundanzmodus-Charakteristiken

```yaml
NONE:
  Äquivalent: "RAID-0 ohne Striping (einzelne Disk)"
  Use Case: "Entwicklung, unkritische Daten"
  Datenverlustszenario: "1 Shard down = Datenverlust"

MIRROR:
  Äquivalent: "RAID-1 (gespiegelt)"
  Use Case: "Production, High Availability erforderlich"
  Datenverlustszenario: "N-1 Shards können ausfallen (bei RF=N)"

STRIPE:
  Äquivalent: "RAID-0 (striped, kein Parity)"
  Use Case: "HPC, Analytics mit Backup-Strategie"
  Datenverlustszenario: "1 Chunk down = Datenverlust"

STRIPE_MIRROR:
  Äquivalent: "RAID-10 (striped + mirrored)"
  Use Case: "Production mit Throughput-Anforderung"
  Datenverlustszenario: "1 Shard pro Stripe-Gruppe kann ausfallen"

PARITY:
  Äquivalent: "RAID-5/6 (mit Parity Chunks)"
  Use Case: "Large-Scale Data Warehouse, Cost-optimiert"
  Datenverlustszenario: "k Shards können ausfallen (k=Parity-Chunks)"

GEO_MIRROR:
  Äquivalent: "RAID-1 über Data Centers"
  Use Case: "Multi-Region Deployment, Disaster Recovery"
  Datenverlustszenario: "N-1 Data Centers können ausfallen"
```

---

## 2. NONE Mode - Single Shard (Baseline)

### Use Case
- Entwicklungs- und Test-Umgebungen
- Unkritische Daten (caches, ephemeral data)
- Maximale Storage-Effizienz (100%)
- Kein Overhead

### 2.1 Konfiguration

```yaml
# /etc/themis/shard-001-config-none.yaml

cluster:
  name: "raid-themis-dev"
  mode: "NONE"

shard:
  id: "shard_001"
  model: relational
  namespace: "development"
  
  storage:
    engine: rocksdb
    data_dir: /data/themis/rocksdb/shard_001
    wal_dir: /data/themis/wal/shard_001
    block_cache_size_gb: 16
    write_buffer_size_mb: 128
    compression: lz4

  replication:
    mode: NONE
    replication_factor: 1         # Nur Primary, KEINE Replicas
    write_concern: IMMEDIATE      # Schreib-Ack nach Primary nur
    
  network:
    primary:
      host: localhost
      port: 8080
    
  failover:
    auto_failover: false          # Keine Failover möglich (nur 1 Shard)
    
  monitoring:
    prometheus_port: 9090
    metrics_enabled: true
```

### 2.2 Operational Playbook

```bash
#!/bin/bash
# NONE Mode Deployment

# 1. Shard starten (nur 1 Shard, keine Replicas)
systemctl start themis-shard@shard_001

# 2. Health Check
themis-cli shard health --shard-id shard_001

# 3. Baseline Throughput messen
echo "Baseline Throughput (NONE Mode):"
themis-cli metrics get throughput --duration 60s

# Output: ~800K ops/sec (Single Shard baseline)

# 4. Backup-Strategie (WICHTIG! Kein RAID-Redundanz!)
#!/bin/bash
*/6 * * * * /usr/local/bin/themis-backup.sh shard_001

# themis-backup.sh
BACKUP_DIR="/data/themis/backup/shard_001"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
themis-cli shard backup create \
  --shard-id shard_001 \
  --type snapshot \
  --destination $BACKUP_DIR/backup_$TIMESTAMP.snap
```

### 2.3 Performance-Charakteristiken

```
Throughput:
  Single Shard: 800K ops/sec
  Read/Write Ratio: 50/50 (OLTP)
  
Latency:
  p50: 0.5ms
  p95: 1.2ms
  p99: 2.3ms
  
Storage:
  Overhead: 0% (Baseline)
  Effective Capacity: 100% of Disk
  
Recovery:
  RTO: N/A (Datenverlust möglich)
  RPO: N/A (keine Replikation)
```

---

## 3. MIRROR Mode - Vollständige Spiegelung (RAID-1)

### Use Case
- Production Systems mit High Availability
- Finanzielle Daten, kritische Workloads
- Read-Skalierung erforderlich
- Datensicherheit > Speicher-Kosten

### 3.1 Konfiguration (RF=3)

```yaml
# /etc/themis/shard-001-config-mirror.yaml

cluster:
  name: "raid-themis-prod"
  mode: "MIRROR"

shard:
  id: "shard_001"
  model: relational
  namespace: "production"
  
  storage:
    engine: rocksdb
    data_dir: /data/themis/rocksdb/shard_001
    wal_dir: /data/themis/wal/shard_001
    block_cache_size_gb: 32        # Größere Cache für mehr Read-Performance
    write_buffer_size_mb: 256
    compression: lz4
    
  replication:
    mode: MIRROR
    replication_factor: 3          # Primary + 2 Replicas
    read_preference: NEAREST       # Read from nearest replica
    write_concern: MAJORITY        # Quorum Writes (2/3 Replicas müssen ACK)
    read_replicas:
      - shard_002
      - shard_003
    
  network:
    # Primary Shard (Writes)
    primary:
      host: themis-shard-001.prod.internal
      port: 8080
    
    # Replica Shards (Read Load Balancing)
    replicas:
      - host: themis-shard-002.prod.internal
        port: 8080
        priority: 0
        read_enabled: true
      - host: themis-shard-003.prod.internal
        port: 8080
        priority: 1
        read_enabled: true
    
    # Raft Consensus
    raft:
      host: themis-shard-001.prod.internal
      port: 8090
  
  consensus:
    engine: raft
    heartbeat_timeout_ms: 150
    election_timeout_ms: 300
    
  failover:
    auto_failover: true
    failover_timeout_ms: 5000
    max_failover_attempts: 3
    
  monitoring:
    prometheus_port: 9090
    metrics_enabled: true
    
  # Circuit Breaker für Cascade-Prevention
  circuit_breaker:
    enabled: true
    failure_threshold: 50
    timeout_ms: 30000
```

### 3.2 Deployment für 8 Shards mit MIRROR Mode (RF=3)

```bash
#!/bin/bash
# Deploy 8 Primary Shards × 3 Replicas = 24 Shard Instances

SHARD_COUNT=8
REPLICATION_FACTOR=3

for SHARD_IDX in $(seq 1 $SHARD_COUNT); do
  SHARD_ID=$(printf "shard_%03d" $SHARD_IDX)
  
  # Primary starten
  systemctl start themis-shard@$SHARD_ID
  
  # Replicas starten (z.B. auf anderen Nodes)
  for REPLICA_IDX in $(seq 1 $((REPLICATION_FACTOR - 1))); do
    REPLICA_SHARD="${SHARD_ID}_replica_$REPLICA_IDX"
    systemctl start themis-shard@$REPLICA_SHARD
  done
  
  # Auf Consensus warten
  sleep 5
  themis-cli shard health --shard-id $SHARD_ID --wait 60
done

# Cluster-Status
themis-cli cluster topology
```

### 3.3 Performance für MIRROR (RF=3)

```
Throughput:
  Single Shard Reads: 2.4M ops/sec (3× parallele Reads)
  Single Shard Writes: 800K ops/sec (Quorum)
  Cluster (8 Shards): 19.2M ops/sec Reads, 6.4M ops/sec Writes
  
Latency (Quorum Writes):
  p50: 0.8ms (Primary + 1 Replica Ack)
  p95: 2.1ms
  p99: 4.5ms
  
Storage Overhead:
  3× Speicher (3 Kopien)
  Effective Capacity: 33% of Total Disk
  
Read Scalability:
  Per Shard: 3× vs NONE
  Full Cluster: 3.7M ops/sec per Shard
  
Recovery:
  RTO: < 1 min (Automatic Failover)
  RPO: 0 (Quorum-basierte Writes)
  Fault Tolerance: 2 Shard Ausfälle (RF=3)
```

### 3.4 Operational Playbook

```bash
#!/bin/bash

# 1. Read Load Balancing Monitor
themis-cli metrics stream --filter "replica_reads_percent"

# 2. Replication Lag Monitor
themis-cli metrics stream --filter "replication_lag_ms"

# 3. Failover Test (Replica killen)
docker kill themis-shard-002  # Eine Replica
sleep 10
themis-cli cluster health     # Sollte noch 2/3 sein

# 4. Replica Recovery
docker start themis-shard-002
themis-cli cluster health --wait 120

# 5. Rebalancing nach Node-Ausfall
themis-cli cluster rebalance \
  --failed-node shard-002 \
  --method consistent-hash
```

---

## 4. STRIPE Mode - Daten-Striping (RAID-0)

### Use Case
- High-Performance Analytics
- Cache Layers
- Backups als separate Redundanz
- Maximale Throughput erforderlich

### 4.1 Konfiguration

```yaml
# /etc/themis/stripe-mode-config.yaml

cluster:
  name: "raid-themis-analytics"
  mode: "STRIPE"

shard:
  id: "stripe_group_001"          # Stripe Group (nicht einzelner Shard)
  model: document
  namespace: "analytics"
  
  storage:
    engine: rocksdb
    data_dir: /data/themis/rocksdb/stripe_001
    
  replication:
    mode: STRIPE
    replication_factor: 1          # KEINE Replikation (Striping nur)
  
  striping:
    enabled: true
    stripe_size: 65536             # 64KB Chunks
    stripe_min_shards: 4           # Min. 4 physische Shards
    stripe_across_shards:
      - shard_001
      - shard_002
      - shard_003
      - shard_004
    
    # Nur große Dokumente stripen
    stripe_large_docs: true
    large_doc_threshold_mb: 1
  
  # Backup statt RAID-Redundanz
  backup:
    enabled: true
    strategy: ASYNC_SNAPSHOT       # Asynchrone Snapshots
    frequency: DAILY               # Daily Backups
    retention_days: 7
    backup_destination: "s3://themis-backups/stripe/"
```

### 4.2 Performance für STRIPE

```
Throughput:
  Sequential Read: 3.2M ops/sec (4 Shards parallel)
  Sequential Write: 3.2M ops/sec
  Random: 2.4M ops/sec
  
Latency:
  p50: 0.4ms (Parallel Read)
  p95: 0.8ms
  p99: 1.5ms
  
Storage Overhead:
  0% (Baseline)
  Effective: 100% of Disk
  
Fault Tolerance:
  RTO: Depends on Backup (RTO: 1-4 hours)
  RPO: 24 hours (Daily Backups)
  Loss Scenario: 1 Shard down = Total Data Loss (Chunks Lost)
```

### 4.3 Use Case Beispiel: Large-Scale Analytics

```bash
#!/bin/bash

# Striping für 100GB+ Documents
themis-bench \
  --workload-mix analytics \
  --stripe-size 64KB \
  --min-stripe-shards 4 \
  --document-size 50MB \
  --duration 300s

# Expected: 3+ M ops/sec Throughput
# Storage: 4 Shards × 100GB = 400GB gesamt
```

---

## 5. STRIPE_MIRROR Mode - Kombiniert (RAID-10)

### Use Case (EMPFOHLEN für Production)
- Production mit Throughput-Anforderung
- Balanced: Performance + Redundancy
- "Best of Both Worlds" (RAID-10)
- Standard-Konfiguration für Enterprise

### 5.1 Konfiguration

```yaml
# /etc/themis/stripe-mirror-config.yaml

cluster:
  name: "raid-themis-prod"
  mode: "STRIPE_MIRROR"
  description: "RAID-10 equivalent: Striped + Mirrored"

shard:
  id: "shard_001"
  model: relational
  namespace: "production"
  
  storage:
    engine: rocksdb
    data_dir: /data/themis/rocksdb/shard_001
    wal_dir: /data/themis/wal/shard_001
    block_cache_size_gb: 32
    write_buffer_size_mb: 256
    compression: lz4
    
  replication:
    mode: STRIPE_MIRROR
    replication_factor: 2          # Primary + 1 Mirror
    
  striping:
    enabled: true
    stripe_size: 65536             # 64KB Chunks
    stripe_min_shards: 4
    stripe_across_shards:
      - shard_001  # Primary Stripe Group
      - shard_002
      - shard_003
      - shard_004
      
    # Mirror replicas
    mirror_stripe_groups:
      - shard_005  # Mirror Stripe Group
      - shard_006
      - shard_007
      - shard_008
    
    stripe_large_docs: true
    large_doc_threshold_mb: 1
  
  network:
    primary_group:
      shards: [shard_001, shard_002, shard_003, shard_004]
      ports: [8080, 8081, 8082, 8083]
    
    mirror_group:
      shards: [shard_005, shard_006, shard_007, shard_008]
      ports: [8080, 8081, 8082, 8083]
  
  consensus:
    engine: raft
    # Raft über Primary Group
  
  failover:
    auto_failover: true
    # 1 Shard pro Stripe-Gruppe kann ausfallen
```

### 5.2 Deployment: 8-Shard Cluster mit STRIPE_MIRROR

```
Primary Stripe Group (4 Shards):
┌────────────────────────────────────┐
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──┐   │
│  │Shard1│ │Shard2│ │Shard3│ │S4│   │
│  │Chunk1│ │Chunk2│ │Chunk3│ │Ch4   │
│  └──────┘ └──────┘ └──────┘ └──┐   │
└────────────────────────────────────┘
              │
              │ Replication (vollständig)
              │
┌────────────────────────────────────┐
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──┐   │
│  │Shard5│ │Shard6│ │Shard7│ │S8│   │
│  │Chunk1│ │Chunk2│ │Chunk3│ │Ch4   │
│  └──────┘ └──────┘ └──────┘ └──┐   │
└────────────────────────────────────┘
Mirror Stripe Group (4 Shards)
```

### 5.3 Performance für STRIPE_MIRROR

```
Throughput:
  Sequential Read: 2.4M ops/sec (4 Shards × 2 Groups = Load Balanced)
  Sequential Write: 1.6M ops/sec (Primary only, mit Mirror Replication)
  Random: 1.9M ops/sec
  Cluster (8 Shards): 3.2M ops/sec per Stripe Group
  
Latency:
  p50: 0.7ms (Striped Read)
  p95: 1.5ms
  p99: 2.8ms
  
Storage Overhead:
  50% (2× Stripe Groups)
  Effective: 50% of Total Disk
  
Fault Tolerance:
  RTO: < 1 min
  RPO: 0 (Quorum Writes)
  Tolerance: 1 Shard pro Stripe-Gruppe
  
Compared to Pure MIRROR (RF=3):
  Throughput: +50% (Striping)
  Storage: -33% (2 Shards instead of 3)
  Complexity: Moderate
```

### 5.4 Operational Playbook

```bash
#!/bin/bash

# 1. Baseline Throughput (STRIPE_MIRROR)
themis-bench --workload-mix OLTP --shards 8 --stripe-mode \
  --duration 60s --threads 32

# Expected: 6.4M ops/sec combined (8 Shards)

# 2. Failover Test (1 Shard pro Stripe-Gruppe)
docker kill shard_002  # Primary Stripe Group
sleep 10
themis-cli cluster health
# Expected: All data available (via Mirror Group)
# Throughput: Degraded but NOT LOST

# 3. Recovery
docker start shard_002
themis-cli cluster rebalance --stripe-group primary

# 4. Replication Status
themis-cli metrics get replication_lag_stripe
# Expected: < 100ms across Stripe Groups
```

---

## 6. PARITY Mode - Erasure Coding (RAID-5/6)

### Use Case
- Large-Scale Data Warehouses
- Cost-optimized Redundancy
- Tolerance für multiple Shard Ausfälle
- Storage > Performance

### 6.1 Konfiguration (Reed-Solomon 4+2)

```yaml
# /etc/themis/parity-mode-config.yaml

cluster:
  name: "raid-themis-datalake"
  mode: "PARITY"
  description: "RAID-6 equivalent with Reed-Solomon EC"

shard:
  id: "parity_group_001"          # EC Group (6 physische Shards)
  model: document
  namespace: "datalake"
  
  storage:
    engine: rocksdb
    data_dir: /data/themis/rocksdb/parity_001
    
  replication:
    mode: PARITY
    replication_factor: 6          # 4 Data + 2 Parity Shards
  
  erasure_coding:
    enabled: true
    algorithm: REED_SOLOMON
    
    # 4+2 Configuration (RAID-6 equivalent)
    data_shards: 4                 # k = Data Chunks
    parity_shards: 2               # m = Parity Chunks
    
    # Shards in EC Group
    ec_group_shards:
      data:
        - shard_001
        - shard_002
        - shard_003
        - shard_004
      parity:
        - shard_005                # Parity Chunk 1
        - shard_006                # Parity Chunk 2
    
    # Nur große Dokumente stripen (EC ist teuer)
    min_doc_size_mb: 10
    
    # Parity Reconstruction Settings
    reconstruction:
      max_parallel: 2              # Max. 2 parallel reconstructions
      io_priority: LOW             # Keine Impact auf User Traffic
      bandwidth_limit_mbps: 100    # Reconstruction Bandwidth
  
  # Monitoring für EC
  monitoring:
    ec_cpu_overhead: true          # Track CPU für EC/Reconstruction
    ec_bandwidth: true
```

### 6.2 Alternative: 8+3 Configuration (Higher Fault Tolerance)

```yaml
erasure_coding:
  data_shards: 8                   # k = 8 Data Chunks
  parity_shards: 3                 # m = 3 Parity Chunks
  # Kann 3 beliebige Shards verlieren
  # Speichereffizienz: 8/11 = 73%
```

### 6.3 Performance für PARITY (4+2)

```
Throughput:
  Sequential Read: 1.6M ops/sec (Parallel Read ohne EC)
  Sequential Write: 0.8M ops/sec (EC Encoding erforderlich)
  Random: 1.2M ops/sec
  
Latency:
  Read p99: 1.8ms (kein EC nötig)
  Write p99: 4.2ms (EC Encoding)
  Reconstruction p99: 15ms+ (Background Job)
  
Storage Overhead:
  33% (4+2 Shards = 6 gesamt, 4/6 = 67% effiziency)
  Effective: 67% of Total Disk (vs 50% für RF=3 MIRROR)
  
Fault Tolerance:
  RTO: < 5 min (Reconstruction erforderlich)
  RPO: 0 (alle Daten rekonstruierbar)
  Tolerance: 2 Shards beliebig (Parity)
  
Reconstruction Time:
  1TB Shard: ~10-15 minutes (mit BW limit)
  Impact: Read Performance degraded (-20%) während Reconstruction
```

### 6.4 Operational Playbook

```bash
#!/bin/bash

# 1. Parity Group Status
themis-cli shard status parity_group_001

# 2. Simulate 2-Shard Loss (testbar)
docker kill shard_002 shard_005
sleep 10
themis-cli shard data-integrity check parity_group_001
# Expected: DATA STILL ACCESSIBLE (via EC Reconstruction)

# 3. Reconstruction starten (Automatic nach Timeout)
themis-cli shard reconstruct parity_group_001 \
  --lost-shards shard_002,shard_005 \
  --bandwidth 100mbps

# 4. Monitor Reconstruction
watch -n 5 'themis-cli shard reconstruct-status parity_group_001'

# Expected Output:
# Shard 002: 45% reconstructed, ETA 6 min
# Shard 005: 45% reconstructed, ETA 6 min
# Total Bandwidth: 98 MB/sec
```

---

## 7. GEO_MIRROR Mode - Multi-Region

### Use Case
- Multi-Region Production
- Disaster Recovery erforderlich
- Lokale Reads pro Region
- Remote Failover

### 7.1 Konfiguration

```yaml
# /etc/themis/geo-mirror-config.yaml

cluster:
  name: "raid-themis-global"
  mode: "GEO_MIRROR"
  description: "Active-Active Replication über 3 Data Centers"

# Data Center Definition
datacenters:
  us-east-1:
    location: "Virginia, USA"
    shards: [shard_001, shard_002, shard_003, shard_004]
    
  eu-west-1:
    location: "Ireland, Europe"
    shards: [shard_005, shard_006, shard_007, shard_008]
    
  ap-south-1:
    location: "Singapore, APAC"
    shards: [shard_009, shard_010, shard_011, shard_012]

shard:
  id: "shard_001"
  datacenter: "us-east-1"
  
  replication:
    mode: GEO_MIRROR
    replication_factor: 3          # 1 local + 2 remote DCs
    
    # Replication Strategy
    sync_strategy: ASYNC           # Remote DC Replication (asynchron)
    local_sync: SYNC               # Local replicas (synchron)
    
    # DC Priorities (für Read Routing)
    read_dc_preference:
      - us-east-1                  # Local DC (< 5ms)
      - eu-west-1                  # Secondary (100-150ms)
      - ap-south-1                 # Tertiary (200-300ms)
  
  network:
    # Intra-DC Communication (high speed)
    intra_dc:
      latency_budget_ms: 5
      bandwidth_reserved_mbps: 1000
    
    # Inter-DC Communication (WAN)
    inter_dc:
      latency_budget_ms: 150
      bandwidth_reserved_mbps: 100
      compression: true            # Compress WAN Traffic
      
  # Conflict Resolution
  conflict_resolution:
    strategy: LWW                  # Last-Write-Wins
    # Alternative: CRDT, Application-level, Write-Partitioning
    version_vector: true           # Track Causality
```

### 7.2 Performance für GEO_MIRROR

```
Throughput:
  Local Reads (us-east-1): 2.4M ops/sec (3 local replicas)
  Local Writes (us-east-1): 800K ops/sec (local quorum)
  Remote Reads (eu-west-1): 2.4M ops/sec (async lag < 100ms)
  
Latency:
  Local Read p99: 2.1ms (us-east-1)
  Local Write p99: 4.5ms (quorum of local replicas)
  Remote Read p99: 150ms (eu-west-1)
  Async Replication Lag: 50-100ms typical
  
Replication:
  Writes: Sync to Local DC, Async to Remote DCs
  Failover: us-east-1 down → eu-west-1 becomes primary
  RTO: < 1 min (Application reconnect)
  
Fault Tolerance:
  Tolerance: 2 DCs können offline sein
  (3 DCs total, 1 must be online)
```

### 7.3 Deployment Example

```bash
#!/bin/bash

# Alle 3 DCs parallel deployen
for DC in us-east-1 eu-west-1 ap-south-1; do
  for SHARD in shard_{001..004}; do
    ssh admin@$DC-gateway.themis.io \
      "systemctl start themis-shard@$SHARD &"
  done
done

# Warten auf Cluster-Formation
sleep 10
for DC in us-east-1 eu-west-1 ap-south-1; do
  themis-cli cluster health --datacenter $DC
done

# Replication Status
themis-cli metrics get geo_replication_lag
# Expected: us-east-1 < 5ms, eu-west-1 < 100ms, ap-south-1 < 200ms
```

---

## 8. Entscheidungsmatrix

### Welcher Modus für welchen Use Case?

```
┌─────────────────────────┬──────────────┬──────────────┬──────────────┐
│ Requirement             │ MIRROR       │ STRIPE_MIRROR│ PARITY       │
├─────────────────────────┼──────────────┼──────────────┼──────────────┤
│ High Availability       │ ✅ RF=3      │ ✅ RF=2      │ ⚠️ Recovery  │
│ High Throughput         │ ⚠️ 1.2×      │ ✅ 2-3×      │ ⚠️ Write     │
│ Cost Optimized          │ ❌ 200%      │ ⚠️ 100%      │ ✅ 67%       │
│ Large Datasets (TB+)    │ ❌ Space     │ ⚠️ 50%       │ ✅ 67%       │
│ Write-Heavy Workloads   │ ✅ Quorum    │ ✅ Striped   │ ❌ EC Overhead│
│ Read-Heavy Workloads    │ ✅ 3× Reads  │ ✅ Striped   │ ⚠️ Parallel  │
│ Multi-Region            │ ⚠️ Complex   │ ⚠️ Complex   │ ⚠️ Very Slow │
│ Development/Testing     │ ❌ Overkill  │ ❌ Overkill  │ ❌ Overkill  │
└─────────────────────────┴──────────────┴──────────────┴──────────────┘

✅ = Empfohlen | ⚠️ = Möglich mit Trade-offs | ❌ = Nicht empfohlen
```

### Decision Tree

```
START
  │
  ├─► Is Production? NO ──────► Use NONE (Dev/Test)
  │   YES
  │
  ├─► RTO < 1 min required? NO ──────► Consider PARITY or GEO
  │   YES
  │
  ├─► Throughput critical? YES ──────► Use STRIPE_MIRROR (RECOMMENDED)
  │   NO
  │
  ├─► Storage cost critical? YES ──────► Use PARITY (4+2 or 8+3)
  │   NO
  │
  └─► Use MIRROR RF=3 (safe default)
```

---

## 9. Migration zwischen Modi

### 9.1 NONE → MIRROR (Add Replication)

```bash
#!/bin/bash

# 1. Neue Replica Shards deployen (ohne Daten)
for REPLICA_IDX in $(seq 1 2); do
  REPLICA_SHARD="shard_001_replica_$REPLICA_IDX"
  systemctl start themis-shard@$REPLICA_SHARD
done

# 2. Shard-Konfiguration zu MIRROR ändern
themis-cli shard config --shard-id shard_001 \
  --replication-mode MIRROR \
  --replication-factor 3

# 3. Initiales Daten-Sync
themis-cli shard sync shard_001 \
  --target-replicas shard_001_replica_1,shard_001_replica_2 \
  --bandwidth 500mbps

# 4. Warten auf vollständiges Sync
themis-cli shard sync-status shard_001 --wait

# 5. Write-Concern umstellen (zu Quorum)
themis-cli shard config --shard-id shard_001 \
  --write-concern MAJORITY

echo "Migration NONE → MIRROR complete"
```

### 9.2 MIRROR (RF=2) → STRIPE_MIRROR

```bash
# 1. Zusätzliche Shards für Striping deployen
for SHARD_IDX in $(seq 1 4); do
  systemctl start themis-shard@stripe_shard_$SHARD_IDX
done

# 2. Replication Mode umstellen
themis-cli cluster config \
  --mode STRIPE_MIRROR \
  --stripe-min-shards 4

# 3. Daten rebalancieren (im Hintergrund)
themis-cli cluster rebalance \
  --target-mode STRIPE_MIRROR \
  --bandwidth 200mbps

# 4. Status überprüfen
watch -n 10 'themis-cli cluster rebalance-status'
```

### 9.3 STRIPE_MIRROR → PARITY (Cost Optimization)

```bash
#!/bin/bash

# Komplexe Migration: Neue EC Group + Daten-Copy

# 1. EC Group vorbereiten (6 neue Shards)
for SHARD_IDX in $(seq 1 6); do
  systemctl start themis-shard@ec_shard_$SHARD_IDX
done

# 2. Daten zu EC Group kopieren (mit Encoding)
themis-cli cluster migrate \
  --from STRIPE_MIRROR \
  --to PARITY \
  --ec-algorithm REED_SOLOMON \
  --ec-config 4+2 \
  --data-migration-rate 100mbps

# 3. Konfiguration umstellen
themis-cli cluster switchover \
  --from-mode STRIPE_MIRROR \
  --to-mode PARITY \
  --validation-period 3600s

# 4. Old Shards decommission
themis-cli shard decommission shard_{001..004}
```

---

## Summary & Recommendations

### Für Production (Enterprise)
**EMPFOHLUNG: STRIPE_MIRROR (RAID-10)**

```yaml
raid_themis_recommended_production:
  redundancy_mode: STRIPE_MIRROR
  replication_factor: 2
  stripe_size: 65536              # 64KB
  shards: 8
  
  rationale:
    - Balanced Performance & Redundancy (RAID-10 proven pattern)
    - 2-3× Throughput improvement (vs MIRROR)
    - 50% Storage Overhead (acceptable)
    - 1 Shard/Stripe-Group failover tolerant
    - Simple Operations (no EC reconstruction)
    
  expected_metrics:
    throughput: "6.4M ops/sec cluster"
    p99_latency: "2.8ms"
    storage: "50% effective"
    rto: "< 1 min"
    rpo: "0 (zero data loss)"
```

### Für Cost-Sensitive Large-Scale
**PARITY mit Reed-Solomon**

```yaml
raid_themis_cost_optimized:
  redundancy_mode: PARITY
  erasure_coding:
    data_shards: 8
    parity_shards: 3              # RAID-6 like
  
  expected_metrics:
    storage: "73% effective (8+3)"
    throughput: "1.3M ops/sec"
    fault_tolerance: "3 simultaneous shard failures"
    reconstruction_time: "15-20 min per shard"
```

### Scaling Strategy
```
Phase 1: 8 STRIPE_MIRROR Shards
  ├─► 6.4M ops/sec
  ├─► 4TB Cluster
  └─► Good for < 50GB/sec workloads

Phase 2: Scale to 16 STRIPE_MIRROR Shards
  ├─► 12.8M ops/sec
  ├─► 8TB Cluster
  └─► Multi-region ready

Phase 3: Large-Scale to 32+ PARITY Shards
  ├─► 51.2M ops/sec (with striping)
  ├─► 16TB+ Cluster
  └─► Cost-optimized
```

---

**Nächste Schritte:**
1. STRIPE_MIRROR auswählen für Production Start
2. Pre-Deployment Checklist durcharbeiten (SHARDING_PRODUCTION_DEPLOYMENT_RAID_v1.4.md)
3. Monitoring setup (SHARDING_MONITORING_OBSERVABILITY_RAID_v1.4.md)
4. Test-Deployment durchführen
5. Scaling-Strategie planen

