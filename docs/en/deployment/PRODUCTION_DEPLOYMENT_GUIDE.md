# ThemisDB Production Deployment Guide

**Version:** 1.4.0  
**Last Updated:** 2026-04-06  
**Target Audience:** DevOps Engineers, Database Administrators, Site Reliability Engineers

---

## Table of Contents

1. [Prerequisites & System Requirements](#prerequisites--system-requirements)
2. [Pre-deployment Checklist](#pre-deployment-checklist)
3. [Deployment Steps](#deployment-steps)
4. [Configuration Best Practices](#configuration-best-practices)
5. [Monitoring Setup](#monitoring-setup)
6. [Disaster Recovery](#disaster-recovery)

---

## Prerequisites & System Requirements

### Hardware Recommendations

#### Minimum Requirements (Development/Testing)
```yaml
CPU: 4 cores (2.0 GHz+)
RAM: 8 GB
Storage: 50 GB SSD
Network: 1 Gbps
```

#### Recommended Requirements (Production - Single Node)
```yaml
CPU: 16-32 cores (2.5 GHz+)
RAM: 64-128 GB
Storage: 1-4 TB NVMe SSD (RAID-10 recommended)
Network: 10 Gbps
```

#### High-Performance Production (Multi-Node Cluster)
```yaml
CPU: 32-64 cores per node (3.0 GHz+)
RAM: 256-512 GB per node
Storage: Multiple NVMe SSDs (2-8 drives per node)
  - Recommended: 2x 2TB NVMe for WAL
  - Recommended: 4x 4TB NVMe for data
Network: 25-100 Gbps low-latency network
```

### Storage Considerations

**SSD Requirements:**
- **IOPS:** Minimum 50,000 IOPS (4K random reads)
- **Throughput:** Minimum 500 MB/s sequential writes
- **Latency:** Sub-millisecond latency preferred
- **Durability:** Enterprise-grade SSDs with power-loss protection

**Multi-SSD Configuration (Recommended):**
```yaml
# Optimal configuration for high throughput
WAL_SSD_1: /mnt/ssd1/themisdb/wal    # Dedicated WAL device
WAL_SSD_2: /mnt/ssd2/themisdb/wal    # Redundant WAL device
DATA_SSD_1: /mnt/ssd3/themisdb/data  # Primary data storage
DATA_SSD_2: /mnt/ssd4/themisdb/data  # Secondary data storage
```

**Filesystem Recommendations:**
- **Linux:** XFS (recommended) or ext4
- **Mount Options:** `noatime,nodiratime,discard`
- **Block Size:** 4KB (default)

### Operating System Support

**Supported Platforms:**
- **Linux:** Ubuntu 20.04+, CentOS 8+, RHEL 8+, Debian 11+
- **Kernel Version:** 5.4+ (5.15+ recommended for io_uring support)
- **Windows:** Windows Server 2019+, Windows 10/11 (limited support)

**Required System Packages (Linux):**
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  librocksdb-dev \
  libssl-dev \
  libboost-all-dev \
  libjemalloc-dev \
  libtbb-dev \
  zlib1g-dev \
  libbz2-dev \
  liblz4-dev \
  libzstd-dev \
  libsnappy-dev

# RHEL/CentOS
sudo yum install -y \
  gcc-c++ \
  cmake3 \
  rocksdb-devel \
  openssl-devel \
  boost-devel \
  jemalloc-devel \
  tbb-devel \
  zlib-devel \
  bzip2-devel \
  lz4-devel \
  libzstd-devel \
  snappy-devel
```

### Network Requirements

**Port Configuration:**
```yaml
# ThemisDB Server
8765: Main database port (TCP)
8080: HTTP API endpoint (TCP)
9090: Health/Error service port (TCP)
9091: Prometheus metrics endpoint (TCP)

# Raft Consensus (Multi-node)
7700: Raft RPC port (TCP)
7701: Raft heartbeat port (TCP)

# Sharding/Replication
8766-8775: Shard communication ports (TCP, range depends on shard count)
```

**Firewall Rules Example (iptables):**
```bash
# Allow ThemisDB ports
sudo iptables -A INPUT -p tcp --dport 8765 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 9090 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 9091 -j ACCEPT

# Allow Raft ports (multi-node only)
sudo iptables -A INPUT -p tcp --dport 7700 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 7701 -j ACCEPT

# Save rules
sudo iptables-save > /etc/iptables/rules.v4
```

### RocksDB Compatibility Matrix

| ThemisDB Version | RocksDB Version | Status | Notes |
|------------------|-----------------|--------|-------|
| 1.4.0 | 8.x | ✅ Recommended | Full feature support |
| 1.4.0 | 7.10+ | ✅ Supported | Missing some optimizations |
| 1.4.0 | 7.0-7.9 | ⚠️ Compatible | Limited BlobDB support |
| 1.4.0 | 6.x | ❌ Not Supported | Missing TransactionDB features |

**RocksDB Build Flags (if building from source):**
```bash
cmake -DROCKSDB_BUILD_SHARED=ON \
      -DUSE_RTTI=ON \
      -DWITH_SNAPPY=ON \
      -DWITH_LZ4=ON \
      -DWITH_ZSTD=ON \
      -DWITH_ZLIB=ON \
      -DWITH_BZ2=ON \
      -DWITH_JEMALLOC=ON \
      -DWITH_TBB=ON \
      -DPORTABLE=ON \
      ..
```

---

## Pre-deployment Checklist

### Database Sizing

**Estimating Storage Requirements:**

```python
# Storage calculation formula
total_storage = (
    num_entities * avg_entity_size_bytes +
    num_edges * avg_edge_size_bytes +
    num_vectors * vector_dimensions * 4  # 4 bytes per float32
) * replication_factor * 1.5  # 50% overhead for compaction/WAL

# Example: 10M entities, 50M edges, 1M vectors (768 dims), 3x replication
total_storage = (
    10_000_000 * 1024 +           # 10GB for entities (1KB avg)
    50_000_000 * 256 +            # 12.5GB for edges (256 bytes avg)
    1_000_000 * 768 * 4           # 3GB for vectors
) * 3 * 1.5
# = 38.25GB * 3 * 1.5 = ~172GB

print(f"Estimated storage required: {total_storage / (1024**3):.2f} GB")
```

**Memory Requirements Estimation:**
```python
# Memory calculation
block_cache_mb = 1024  # RocksDB block cache
memtable_mb = 256      # Active memtables
write_buffer_mb = 256  # Write buffers
server_overhead_mb = 512  # Server overhead
per_connection_mb = 0.5   # Per-connection memory
max_connections = 100

total_memory_mb = (
    block_cache_mb +
    memtable_mb * 3 +  # 3 memtables per column family
    write_buffer_mb +
    server_overhead_mb +
    (per_connection_mb * max_connections)
)

# Add 20% safety margin
total_memory_gb = (total_memory_mb * 1.2) / 1024

print(f"Recommended RAM: {total_memory_gb:.1f} GB")
# Output: Recommended RAM: 2.4 GB (minimum)
```

### Capacity Planning

**Workload Classification:**

| Workload Type | Read/Write Ratio | Latency SLA | Throughput Requirements |
|---------------|------------------|-------------|-------------------------|
| OLTP | 70/30 | < 10ms p99 | 10K-100K ops/sec |
| Analytics | 95/5 | < 100ms p99 | 1K-10K ops/sec |
| Hybrid | 50/50 | < 50ms p99 | 5K-50K ops/sec |
| Write-Heavy | 20/80 | < 20ms p99 | 20K-200K writes/sec |

**Performance Baseline Establishment:**

```bash
# Run baseline benchmark before production deployment
./themisdb-benchmark \
  --workload=ycsb_workloada \
  --threads=16 \
  --duration=300 \
  --target-ops=10000 \
  --output=baseline_results.json

# Verify results meet SLA requirements
cat baseline_results.json | jq '.latency.p99_ms'
# Expected: < 10ms for OLTP workload
```

### Security Hardening Requirements

**Pre-deployment Security Checklist:**

- [ ] **TLS/mTLS Configuration**
  - [ ] Generate CA certificate
  - [ ] Generate server certificates
  - [ ] Generate client certificates
  - [ ] Configure certificate rotation policy

- [ ] **Authentication Setup**
  - [ ] Enable authentication (`enable_auth: true`)
  - [ ] Create admin user with strong password
  - [ ] Configure LDAP/Kerberos integration (if applicable)
  - [ ] Set up JWT token expiration

- [ ] **Authorization (RBAC)**
  - [ ] Define role hierarchy
  - [ ] Create application service accounts
  - [ ] Assign minimum required permissions
  - [ ] Enable audit logging

- [ ] **Network Isolation**
  - [ ] Configure firewall rules
  - [ ] Set up VPC/network segmentation
  - [ ] Enable rate limiting
  - [ ] Configure DDoS protection

- [ ] **Encryption**
  - [ ] Enable encryption at rest
  - [ ] Enable encryption in transit
  - [ ] Configure field-level encryption for sensitive data
  - [ ] Set up key rotation schedule

**See:** [Security Deployment Guide](../security/SECURITY_DEPLOYMENT_GUIDE.md) for detailed instructions.

---

## Deployment Steps

### Single-Node Deployment

#### Step 1: Install ThemisDB

**From Binary Release:**
```bash
# Download latest release
wget https://github.com/themisdb/themisdb/releases/download/v1.4.0/themisdb-v1.4.0-linux-x64.tar.gz

# Extract archive
tar -xzf themisdb-v1.4.0-linux-x64.tar.gz
cd themisdb-v1.4.0

# Install to /opt/themisdb
sudo mkdir -p /opt/themisdb
sudo cp -r bin lib config /opt/themisdb/
sudo chown -R themisdb:themisdb /opt/themisdb

# Create data directories
sudo mkdir -p /var/lib/themisdb/data
sudo mkdir -p /var/lib/themisdb/wal
sudo mkdir -p /var/log/themisdb
sudo chown -R themisdb:themisdb /var/lib/themisdb /var/log/themisdb
```

**From Source:**
```bash
# Clone repository
git clone https://github.com/themisdb/themisdb.git
cd themisdb

# Build with optimizations
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_LLM=ON \
      -DENABLE_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=/opt/themisdb \
      ..
make -j$(nproc)
sudo make install
```

#### Step 2: Configure ThemisDB

**Basic Production Configuration (`/opt/themisdb/config/config.yaml`):**
```yaml
# Server Configuration
server:
  host: "0.0.0.0"
  port: 8765
  worker_threads: 0  # Auto-detect CPU cores
  max_connections: 100
  enable_http2: true
  health_error_service_port: 9090

# Database Configuration
database:
  path: "/var/lib/themisdb/data"
  wal_path: "/var/lib/themisdb/wal"
  enable_wal: true
  wal_sync_mode: "normal"  # Options: none, normal, full

# RocksDB Configuration
rocksdb:
  # Memory Configuration
  memtable_size_mb: 256
  block_cache_size_mb: 8192  # 8GB, adjust based on available RAM
  db_write_buffer_size_mb: 512
  max_write_buffer_number: 3
  
  # Performance Tuning
  max_background_jobs: 8
  max_subcompactions: 2
  enable_pipelined_write: false  # Must be false for TransactionDB
  allow_concurrent_memtable_write: true
  
  # Compaction
  level0_file_num_compaction_trigger: 4
  level0_slowdown_writes_trigger: 20
  level0_stop_writes_trigger: 36
  target_file_size_base_mb: 64
  max_bytes_for_level_base_mb: 256
  
  # Compression
  compression: "lz4"
  bottommost_compression: "zstd"
  
  # BlobDB (for large values)
  enable_blobdb: true
  blob_size_threshold: 4096  # 4KB

# Transaction Configuration
transaction:
  isolation_level: "snapshot"  # Options: read_committed, snapshot
  write_policy: "write_unprepared"
  default_timeout_ms: 30000  # 30 seconds
  enable_two_write_queues: true
  
# MVCC Configuration
mvcc:
  snapshot_retention_seconds: 3600  # 1 hour
  max_snapshots: 100
  enable_read_only_snapshots: true

# Monitoring
monitoring:
  enable_prometheus: true
  prometheus_port: 9091
  metrics_interval_ms: 1000
  enable_slow_query_log: true
  slow_query_threshold_ms: 1000

# Logging
logging:
  level: "info"  # Options: trace, debug, info, warn, error
  path: "/var/log/themisdb"
  max_size_mb: 100
  max_files: 10
  enable_console: false
  enable_audit_log: true

# Security
security:
  enable_auth: true
  enable_tls: true
  tls_cert_file: "/opt/themisdb/certs/server.crt"
  tls_key_file: "/opt/themisdb/certs/server.key"
  tls_ca_file: "/opt/themisdb/certs/ca.crt"
  enable_mtls: false  # Client certificate authentication
```

#### Step 3: Create SystemD Service

**Create service file (`/etc/systemd/system/themisdb.service`):**
```ini
[Unit]
Description=ThemisDB Multi-Model Database
After=network.target
Requires=network.target

[Service]
Type=simple
User=themisdb
Group=themisdb
WorkingDirectory=/opt/themisdb
ExecStart=/opt/themisdb/bin/themisdb-server \
  --config=/opt/themisdb/config/config.yaml
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=10
StandardOutput=journal
StandardError=journal
SyslogIdentifier=themisdb

# Resource Limits
LimitNOFILE=65536
LimitMEMLOCK=infinity

# Security Hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/themisdb /var/log/themisdb

[Install]
WantedBy=multi-user.target
```

**Enable and start service:**
```bash
# Reload systemd
sudo systemctl daemon-reload

# Enable service
sudo systemctl enable themisdb

# Start service
sudo systemctl start themisdb

# Check status
sudo systemctl status themisdb

# View logs
sudo journalctl -u themisdb -f
```

#### Step 4: Verify Installation

**Health Check:**
```bash
# Check server health
curl http://localhost:9090/health
# Expected output: {"status":"healthy","uptime_seconds":123,...}

# Check Prometheus metrics
curl http://localhost:9091/metrics | grep themisdb_

# Test connection
themisdb-cli --host localhost --port 8765 --command "SELECT VERSION()"
```

### Multi-Node Cluster Deployment

#### Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                    Load Balancer                         │
│                  (HAProxy/NGINX)                         │
└──────┬──────────────┬──────────────┬───────────────────┘
       │              │              │
   ┌───▼───┐      ┌───▼───┐      ┌───▼───┐
   │Node 1 │      │Node 2 │      │Node 3 │
   │Leader │      │Follower│     │Follower│
   └───┬───┘      └───┬───┘      └───┬───┘
       │              │              │
       └──────────────┴──────────────┘
              Raft Consensus
```

#### Step 1: Prepare All Nodes

**On each node, complete single-node deployment steps 1-2.**

#### Step 2: Configure Raft Consensus

**Node 1 Configuration (`/opt/themisdb/config/cluster.yaml`):**
```yaml
# Cluster Configuration
cluster:
  enabled: true
  node_id: "node1"
  node_address: "192.168.1.10:7700"
  
  # Raft Configuration
  raft:
    enabled: true
    election_timeout_ms: 1000
    heartbeat_interval_ms: 100
    snapshot_interval_seconds: 3600
    log_dir: "/var/lib/themisdb/raft/log"
    snapshot_dir: "/var/lib/themisdb/raft/snapshots"
    
    # Peer Nodes
    peers:
      - node_id: "node2"
        address: "192.168.1.11:7700"
      - node_id: "node3"
        address: "192.168.1.12:7700"
  
  # Replication Configuration
  replication:
    factor: 3  # Number of replicas
    sync_mode: "quorum"  # Options: async, quorum, sync
    max_lag_bytes: 10485760  # 10MB
    enable_read_from_followers: true

# Include base configuration
include: "config.yaml"
```

**Node 2 and Node 3:** Same configuration with updated `node_id` and `node_address`.

#### Step 3: Initialize Cluster

**On Node 1 (Leader):**
```bash
# Initialize cluster
themisdb-admin cluster init \
  --config=/opt/themisdb/config/cluster.yaml \
  --node-id=node1

# Wait for cluster to be ready
themisdb-admin cluster status
```

**On Node 2 and Node 3:**
```bash
# Join cluster
themisdb-admin cluster join \
  --config=/opt/themisdb/config/cluster.yaml \
  --node-id=node2 \
  --leader-address=192.168.1.10:7700

# Verify cluster membership
themisdb-admin cluster members
```

#### Step 4: Verify Cluster Health

```bash
# Check cluster status
themisdb-admin cluster status

# Expected output:
# Cluster: healthy
# Leader: node1
# Nodes: 3/3 online
# Replication: in-sync

# Check Raft state
curl http://localhost:9091/metrics | grep raft_
```

### Sharding Configuration

**When to Use Sharding:**
- Dataset size > 1TB
- Write throughput > 50K ops/sec
- Need to scale horizontally

**Shard Configuration (`/opt/themisdb/config/sharding.yaml`):**
```yaml
sharding:
  enabled: true
  num_shards: 8  # Must be power of 2 (2, 4, 8, 16, 32, ...)
  replication_factor: 3
  
  # Shard Strategy
  strategy: "hash"  # Options: hash, range, consistent_hash
  hash_function: "murmur3"
  
  # Shard Assignment
  shards:
    - shard_id: 0
      primary_node: "node1"
      replica_nodes: ["node2", "node3"]
      
    - shard_id: 1
      primary_node: "node2"
      replica_nodes: ["node3", "node1"]
      
    - shard_id: 2
      primary_node: "node3"
      replica_nodes: ["node1", "node2"]
    
    # ... additional shards

  # Cross-Shard Operations
  enable_distributed_transactions: true
  distributed_transaction_timeout_ms: 60000
  
# Include cluster configuration
include: "cluster.yaml"
```

**Initialize Sharding:**
```bash
# Create shards
themisdb-admin sharding init \
  --config=/opt/themisdb/config/sharding.yaml

# Verify shard distribution
themisdb-admin sharding status

# Test cross-shard query
themisdb-cli --query "SELECT COUNT(*) FROM entities"
```

### Replication Setup (Raft)

**Replication Modes:**

| Mode | Description | Durability | Performance | Use Case |
|------|-------------|------------|-------------|----------|
| **Async** | Leader doesn't wait for followers | Low | High | Read-heavy, can tolerate data loss |
| **Quorum** | Wait for majority (N/2+1) | Medium | Medium | Balanced (recommended) |
| **Sync** | Wait for all replicas | High | Low | Critical data, no data loss tolerance |

**Configuration Example:**
```yaml
replication:
  sync_mode: "quorum"
  
  # Timeout settings
  replication_timeout_ms: 1000
  follower_timeout_ms: 5000
  
  # WAL shipping
  wal_shipping:
    enabled: true
    max_batch_size: 1000
    batch_interval_ms: 10
    compression: "lz4"
  
  # Snapshot transfer
  snapshot_transfer:
    enabled: true
    chunk_size_mb: 64
    parallel_transfers: 4
```

**Monitor Replication Lag:**
```bash
# Check replication metrics
curl http://localhost:9091/metrics | grep replication_lag

# Output:
# themisdb_replication_lag_bytes{node="node2"} 1024
# themisdb_replication_lag_bytes{node="node3"} 512
```

---

## Configuration Best Practices

### RocksDB Tuning Parameters

**For High Write Throughput:**
```yaml
rocksdb:
  # Increase write buffer
  memtable_size_mb: 512
  max_write_buffer_number: 6
  db_write_buffer_size_mb: 2048
  
  # More aggressive compaction
  max_background_compactions: 8
  max_subcompactions: 4
  level0_file_num_compaction_trigger: 2
  
  # Enable concurrent writes
  allow_concurrent_memtable_write: true
  enable_two_write_queues: true
```

**For Low Read Latency:**
```yaml
rocksdb:
  # Larger block cache
  block_cache_size_mb: 16384  # 16GB
  cache_index_and_filter_blocks: true
  pin_l0_filter_and_index_blocks_in_cache: true
  
  # Optimize bloom filters
  bloom_bits_per_key: 10
  whole_key_filtering: true
  
  # Enable direct I/O for reads
  use_direct_reads: true
  
  # Fast compression
  compression: "lz4"
```

**For Balanced Workload:**
```yaml
rocksdb:
  memtable_size_mb: 256
  block_cache_size_mb: 8192
  max_write_buffer_number: 3
  max_background_jobs: 8
  max_subcompactions: 2
  compression: "lz4"
  bottommost_compression: "zstd"
```

### MVCC Snapshot Configuration

```yaml
mvcc:
  # Snapshot retention
  snapshot_retention_seconds: 3600  # 1 hour
  max_snapshots: 100
  
  # Automatic snapshot cleanup
  enable_automatic_cleanup: true
  cleanup_interval_seconds: 300  # 5 minutes
  
  # Snapshot isolation
  enable_read_only_snapshots: true
  snapshot_cache_size: 1000
```

**Recommended Settings by Use Case:**

| Use Case | Retention | Max Snapshots | Notes |
|----------|-----------|---------------|-------|
| OLTP | 1-4 hours | 50-100 | Short transactions |
| Analytics | 4-24 hours | 100-500 | Long-running queries |
| Audit/Compliance | 7-30 days | 1000+ | Regulatory requirements |

### Transaction Timeout Settings

```yaml
transaction:
  # Default timeout
  default_timeout_ms: 30000  # 30 seconds
  
  # Maximum timeout (prevents indefinite locks)
  max_timeout_ms: 300000  # 5 minutes
  
  # Lock timeout
  lock_timeout_ms: 10000  # 10 seconds
  
  # Deadlock detection
  deadlock_detect_interval_ms: 1000
  max_deadlock_detect_depth: 50
```

**Timeout Recommendations:**

| Transaction Type | Timeout | Isolation Level |
|------------------|---------|-----------------|
| Point queries | 1-5 seconds | ReadCommitted |
| Small updates | 5-30 seconds | Snapshot |
| Batch operations | 30-120 seconds | Snapshot |
| Analytics | 2-5 minutes | Snapshot |

### WAL Management

```yaml
database:
  wal_path: "/mnt/fast-ssd/themisdb/wal"
  enable_wal: true
  
  # WAL sync mode
  wal_sync_mode: "normal"  # Options: none, normal, full
  
  # WAL file management
  wal_max_file_size_mb: 256
  wal_max_total_size_mb: 4096  # 4GB total
  
  # WAL archival (for PITR)
  wal_archive_enabled: true
  wal_archive_path: "/mnt/backup/themisdb/wal-archive"
  wal_archive_retention_hours: 168  # 7 days
```

**WAL Sync Modes:**

| Mode | Durability | Performance | Use Case |
|------|------------|-------------|----------|
| **none** | Low (data loss on crash) | Highest | Development only |
| **normal** | Medium (group commit) | High | Most production workloads |
| **full** | Highest (fsync per write) | Low | Critical financial data |

### Compaction Strategy Selection

**Level-Based Compaction (Default):**
```yaml
rocksdb:
  compaction_style: "level"
  
  # Level targets
  max_bytes_for_level_base_mb: 256
  max_bytes_for_level_multiplier: 10
  
  # File sizes
  target_file_size_base_mb: 64
  target_file_size_multiplier: 1
  
  # Trigger points
  level0_file_num_compaction_trigger: 4
  level0_slowdown_writes_trigger: 20
  level0_stop_writes_trigger: 36
```

**Universal Compaction (Write-Heavy Workloads):**
```yaml
rocksdb:
  compaction_style: "universal"
  
  # Universal compaction options
  universal_compaction_options:
    size_ratio: 1
    min_merge_width: 2
    max_merge_width: 5
    max_size_amplification_percent: 200
    compression_size_percent: -1
    stop_style: "total_size"
```

**Comparison Table:**

| Aspect | Level-Based | Universal |
|--------|-------------|-----------|
| **Write Amplification** | Higher (2-10x) | Lower (1-2x) |
| **Read Performance** | Better | Slightly worse |
| **Space Amplification** | Lower | Higher |
| **Best For** | Balanced, read-heavy | Write-heavy, SSD-constrained |
| **Compaction CPU** | Moderate | Higher (periodic bursts) |

---

## Monitoring Setup

### Prometheus Metrics Export

**Configuration:**
```yaml
monitoring:
  enable_prometheus: true
  prometheus_port: 9091
  metrics_interval_ms: 1000
  enable_detailed_metrics: true
```

**Key Metrics to Monitor:**

```prometheus
# Query Performance
themisdb_query_latency_ms{quantile="0.99"}
themisdb_query_throughput_ops
themisdb_slow_queries_total

# Transaction Metrics
themisdb_transaction_active_count
themisdb_transaction_commit_latency_ms{quantile="0.99"}
themisdb_transaction_conflict_rate
themisdb_transaction_abort_rate

# RocksDB Metrics
themisdb_rocksdb_memtable_flush_pending
themisdb_rocksdb_l0_num_files
themisdb_rocksdb_compaction_pending
themisdb_rocksdb_block_cache_hit_ratio

# Replication Metrics
themisdb_replication_lag_bytes
themisdb_raft_leader_election_count
themisdb_raft_heartbeat_latency_ms

# System Metrics
themisdb_memory_usage_bytes
themisdb_cpu_usage_percent
themisdb_disk_io_ops_total
themisdb_network_bytes_total
```

**Prometheus Scrape Configuration (`prometheus.yml`):**
```yaml
scrape_configs:
  - job_name: 'themisdb'
    scrape_interval: 15s
    static_configs:
      - targets:
          - 'node1:9091'
          - 'node2:9091'
          - 'node3:9091'
    metric_relabel_configs:
      - source_labels: [__name__]
        regex: 'themisdb_.*'
        action: keep
```

### Health Check Endpoints

**Available Endpoints:**

```bash
# Overall health status
curl http://localhost:9090/health

# Component-level health
curl http://localhost:9090/health/components

# Readiness probe (for Kubernetes)
curl http://localhost:9090/ready

# Liveness probe
curl http://localhost:9090/alive
```

**Health Check Response:**
```json
{
  "status": "healthy",
  "uptime_seconds": 86400,
  "version": "1.4.0",
  "components": {
    "database": {
      "status": "healthy",
      "rocksdb_status": "ok",
      "wal_enabled": true
    },
    "replication": {
      "status": "healthy",
      "role": "leader",
      "peers_online": 2,
      "replication_lag_ms": 5
    },
    "cluster": {
      "status": "healthy",
      "nodes_online": 3,
      "quorum": true
    }
  }
}
```

### Alert Configuration

**Grafana Alert Rules (`alerts.yaml`):**
```yaml
groups:
  - name: themisdb_alerts
    interval: 30s
    rules:
      # High CPU Usage
      - alert: HighCPUUsage
        expr: themisdb_cpu_usage_percent > 80
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High CPU usage on {{ $labels.instance }}"
          description: "CPU usage is {{ $value }}%"
      
      # Memory Pressure
      - alert: MemoryPressure
        expr: themisdb_memory_usage_bytes / themisdb_memory_total_bytes > 0.9
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Memory pressure on {{ $labels.instance }}"
          description: "Memory usage is {{ $value | humanizePercentage }}"
      
      # Transaction Conflicts
      - alert: HighTransactionConflictRate
        expr: rate(themisdb_transaction_conflicts_total[5m]) > 100
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "High transaction conflict rate"
          description: "Conflict rate is {{ $value | humanize }} conflicts/sec"
      
      # Replication Lag
      - alert: ReplicationLagHigh
        expr: themisdb_replication_lag_bytes > 10485760  # 10MB
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "High replication lag on {{ $labels.node }}"
          description: "Lag is {{ $value | humanize1024 }}B"
      
      # Disk Space
      - alert: DiskSpaceLow
        expr: themisdb_disk_free_bytes / themisdb_disk_total_bytes < 0.1
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Low disk space on {{ $labels.instance }}"
          description: "Only {{ $value | humanizePercentage }} free"
```

### Log Aggregation Setup

**Fluentd Configuration (`fluent.conf`):**
```conf
<source>
  @type tail
  path /var/log/themisdb/*.log
  pos_file /var/log/td-agent/themisdb.log.pos
  tag themisdb.logs
  
  <parse>
    @type json
    time_key timestamp
    time_format %Y-%m-%dT%H:%M:%S.%NZ
  </parse>
</source>

<filter themisdb.logs>
  @type record_transformer
  <record>
    hostname ${hostname}
    service themisdb
  </record>
</filter>

<match themisdb.logs>
  @type elasticsearch
  host elasticsearch.example.com
  port 9200
  logstash_format true
  logstash_prefix themisdb
  
  <buffer>
    @type file
    path /var/log/td-agent/buffer/themisdb
    flush_interval 10s
  </buffer>
</match>
```

---

## Disaster Recovery

### Backup Strategies

#### Full Backup

**Configuration:**
```yaml
backup:
  enabled: true
  schedule: "0 2 * * *"  # 2 AM daily
  type: "full"
  destination: "s3://themisdb-backups/full/"
  compression: "zstd"
  encryption: "aes256"
  retention_days: 30
```

**Manual Full Backup:**
```bash
# Create full backup
themisdb-admin backup create \
  --type=full \
  --destination=/mnt/backup/themisdb/full-$(date +%Y%m%d) \
  --compression=zstd \
  --verify

# Expected output:
# Backup started: backup-20260118-020000
# Backing up RocksDB database...
# Backing up WAL files...
# Creating manifest...
# Verifying backup integrity...
# Backup completed successfully: 128.5 GB in 15m32s
```

#### Incremental Backup

**Configuration:**
```yaml
backup:
  incremental:
    enabled: true
    schedule: "0 */6 * * *"  # Every 6 hours
    base_backup: "full"
    destination: "s3://themisdb-backups/incremental/"
    retention_days: 7
```

**Manual Incremental Backup:**
```bash
# Create incremental backup
themisdb-admin backup create \
  --type=incremental \
  --base=/mnt/backup/themisdb/full-20260118 \
  --destination=/mnt/backup/themisdb/incr-$(date +%Y%m%d-%H%M)

# Incremental backups only copy changed data since last full backup
```

### PITR (Point-in-Time Recovery) Setup

**Enable WAL Archival:**
```yaml
database:
  wal_archive_enabled: true
  wal_archive_path: "/mnt/backup/themisdb/wal-archive"
  wal_archive_retention_hours: 168  # 7 days
  wal_archive_compression: "lz4"
```

**Configure PITR:**
```yaml
pitr:
  enabled: true
  wal_archive_path: "/mnt/backup/themisdb/wal-archive"
  checkpoint_interval_seconds: 3600  # 1 hour
  max_recovery_time_hours: 168  # 7 days
```

**Perform PITR Recovery:**
```bash
# Stop ThemisDB
sudo systemctl stop themisdb

# Restore to specific point in time
themisdb-admin pitr restore \
  --base-backup=/mnt/backup/themisdb/full-20260118 \
  --target-time="2026-01-18 14:30:00" \
  --wal-archive=/mnt/backup/themisdb/wal-archive \
  --destination=/var/lib/themisdb/data-restored

# Verify recovery
themisdb-admin pitr verify --path=/var/lib/themisdb/data-restored

# Start ThemisDB with restored data
sudo mv /var/lib/themisdb/data /var/lib/themisdb/data-old
sudo mv /var/lib/themisdb/data-restored /var/lib/themisdb/data
sudo systemctl start themisdb
```

### Recovery Time Objectives (RTO)

**Target RTOs by Scenario:**

| Scenario | Target RTO | Recovery Method |
|----------|------------|-----------------|
| **Single node failure** | < 5 minutes | Automatic failover (Raft) |
| **Multiple node failure** | < 30 minutes | Restore from last healthy node |
| **Data corruption** | < 2 hours | PITR from last checkpoint |
| **Complete disaster** | < 4 hours | Full restore from backup |
| **Regional failure** | < 8 hours | Cross-region replica promotion |

**Automated Failover Configuration:**
```yaml
cluster:
  failover:
    enabled: true
    detection_interval_ms: 100
    failover_timeout_ms: 5000
    auto_promote: true
    
    # Split-brain prevention
    enable_fencing: true
    fencing_method: "zookeeper"
```

### Recovery Point Objectives (RPO)

**Target RPOs by Replication Mode:**

| Replication Mode | Target RPO | Data Loss Risk |
|------------------|------------|----------------|
| **Sync** | 0 seconds | None (wait for all replicas) |
| **Quorum** | < 1 second | Minimal (majority committed) |
| **Async** | < 10 seconds | Low (leader-only committed) |
| **No Replication** | Last backup | High (depends on backup frequency) |

**Backup-Based RPO:**

| Backup Strategy | Target RPO | Notes |
|-----------------|------------|-------|
| **Continuous WAL Archival** | < 1 minute | Requires PITR |
| **Hourly Incremental** | < 1 hour | Good balance |
| **Daily Full** | < 24 hours | Acceptable for non-critical data |

**Recommended Configuration for Critical Data:**
```yaml
# Combine synchronous replication with WAL archival
replication:
  sync_mode: "quorum"  # RPO < 1 second
  
database:
  wal_archive_enabled: true  # RPO < 1 minute for PITR
  
backup:
  enabled: true
  schedule: "0 */6 * * *"  # Every 6 hours as last resort
```

---

## Next Steps

After completing the deployment:

1. **Configure Monitoring:** Set up [Monitoring & Observability](../operations/MONITORING_SETUP_GUIDE.md)
2. **Establish Operational Procedures:** Follow [Operational Procedures](../operations/OPERATIONAL_PROCEDURES.md)
3. **Optimize Performance:** Review [RocksDB Optimization Guide](../storage/ROCKSDB_OPTIMIZATION_GUIDE.md)
4. **Secure Your Deployment:** Implement [Security Best Practices](../security/SECURITY_DEPLOYMENT_GUIDE.md)
5. **Plan for Disasters:** Test [Backup and Recovery Procedures](../operations/OPERATIONAL_PROCEDURES.md#backup--recovery-procedures)

---

## Related Documentation

- [MVCC Tuning Guide](../features/MVCC_TUNING_GUIDE.md)
- [Transaction Best Practices](../features/TRANSACTION_BEST_PRACTICES.md)
- [RocksDB Optimization Guide](../storage/ROCKSDB_OPTIMIZATION_GUIDE.md)
- [Security Deployment Guide](../security/SECURITY_DEPLOYMENT_GUIDE.md)
- [Monitoring Setup Guide](../operations/MONITORING_SETUP_GUIDE.md)
- [Operational Procedures](../operations/OPERATIONAL_PROCEDURES.md)
- [Troubleshooting Guide](../operations/TROUBLESHOOTING_GUIDE.md)

---

**Document Version:** 1.0  
**ThemisDB Compatibility:** 1.4.0+  
**Last Reviewed:** 2026-01-18
