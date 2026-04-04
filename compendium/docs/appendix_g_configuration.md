# Appendix G: Configuration Reference

> "Good defaults get you 90% of the way. This reference handles the other 10%."

**Version:** 1.5.0-dev  
**Last Updated:** 2026-02-15

---

## Overview
Complete reference for all ThemisDB configuration options. Default values optimized for single-node deployments.

**Note on Port Configurations:** ThemisDB uses different default ports depending on the deployment platform. See the platform-specific section below for details.

---

## G.1 Main Configuration File (themis.conf)

### Location & Format
```yaml
# Location: /etc/themis/themis.conf (root) or ~/.themis/themis.conf (user)
# Format: YAML 1.2
# Reload: systemctl reload themis (no restart needed for most settings)
```

### Server Settings

#### port (Platform-Specific Defaults)
```yaml
server:
  port: 8765  # Default for most configs (standard, RPi)
  # Platform-specific defaults:
  #   - Standard/RPi: 8765
  #   - QNAP/Docker: 18765 (binary wire protocol)
  #   - Kubernetes: 8529 (HTTP/REST API)
  # Range: 1024-65535
  # Tip: Use ports 8765-8775 to run multiple instances
  #
  # See docs/de/deployment/PORT_REFERENCE.md for complete port mapping
```

#### bind_address
```yaml
server:
  bind_address: "127.0.0.1"  # Default: localhost only
  # "0.0.0.0"  → Listen on all interfaces
  # Tip: Use localhost in dev, 0.0.0.0 in containerized deployments
```

#### max_connections
```yaml
server:
  max_connections: 1000  # Default: 1000
  # Depends on file descriptor limit (ulimit -n)
  # Scaling rule: 1 connection = ~10 KB memory overhead
  # If 10k connections: ~100 MB RAM just for connection state
```

#### request_timeout_ms
```yaml
server:
  request_timeout_ms: 30000  # Default: 30 sec
  # Queries longer than this abort with timeout
  # Long-running imports: increase to 300000 (5 min)
```

#### enable_http2
```yaml
server:
  enable_http2: true  # Default: true
  # Set false for testing/debugging with curl
```

---

## G.2 Database Settings

### data_directory
```yaml
database:
  data_directory: "/var/lib/themis"  # Default
  # Must be on fast storage (SSD preferred)
  # Recommendation: NVMe > SSD > HDD (in order of performance)
  # Min free space: 2x database size
```

### engine_type
```yaml
database:
  engine_type: "rocksdb"  # Only option (default)
  # RocksDB is LSM-tree optimized for writes
  # 45k-60k writes/sec on modern hardware
```

### wal_enabled
```yaml
database:
  wal_enabled: true  # Default: true (REQUIRED for ACID)
  # Set false ONLY for ephemeral/test deployments
```

### wal_directory
```yaml
database:
  wal_directory: "/var/lib/themis/wal"  # Default
  # Tip: Put on separate disk from data for durability
```

### wal_sync_mode
```yaml
database:
  wal_sync_mode: "fsync"  # Default: fsync (safe)
  # Options:
  #   "fsync"      → Durability: ✓✓✓ (safe) | Speed: ✓ (slowest)
  #   "os"         → Durability: ✓✓  (eventual) | Speed: ✓✓
  #   "none"       → Durability: ✓   (risky) | Speed: ✓✓✓ (fastest)
  # Recommendation: fsync for production, os for staging, none for local dev
```

### compression_algorithm
```yaml
database:
  compression_algorithm: "zstd"  # Default: zstd
  # Options: "zstd" (best) | "lz4" (fast) | "snappy" | "none"
  # Recommendation: zstd for storage-constrained, lz4 for latency-critical
```

---

## G.3 Cache Settings

### cache_size_mb
```yaml
cache:
  size_mb: 2048  # Default: 2048 (2 GB)
  # Rule of thumb: 25-30% of total system RAM
  # Example: 32 GB system → 8-10 GB cache
  # Observability: curl http://localhost:8529/_admin/cache
```

### cache_mode
```yaml
cache:
  mode: "lru"  # Default: lru (Least Recently Used)
  # Options: "lru" | "lfu" (Least Frequently Used)
  # LRU: Better for time-dependent patterns
  # LFU: Better for popularity-based patterns
```

### block_cache_size_mb
```yaml
cache:
  block_cache_size_mb: 1024  # Default: 1024
  # RocksDB block cache (L1 cache for block decompression)
  # Typically 50% of cache_size_mb
```

---

## G.4 Index Settings

### default_index_type
```yaml
indexing:
  default_index_type: "btree"  # Default: btree
  # Options: "btree" | "hash" | "skiplist"
  # btree: Ordered access (good for range queries)
  # hash: Point lookups only (fastest for equality)
  # skiplist: Ordered with faster random access than btree
```

### vector_index_default
```yaml
indexing:
  vector_index_default: "hnsw"  # Default: hnsw
  # Options: "hnsw" (Hierarchical Navigable Small Worlds)
  # Settings:
  #   m: 16        # Connections per layer (16-64, default 16)
  #   ef_construct: 200  # Quality during construction (default)
  #   ef_search: 100     # Quality during search (default)
```

---

## G.5 Query Settings

### query_timeout_ms
```yaml
query:
  timeout_ms: 30000  # Default: 30 sec
  # Queries exceeding this timeout abort
  # Set to -1 for no timeout (not recommended)
```

### query_cache_enabled
```yaml
query:
  cache_enabled: true  # Default: true
  # Caches query plans (not results)
  # Reduces compilation overhead for repeated queries
```

### max_query_result_size_mb
```yaml
query:
  max_query_result_size_mb: 512  # Default: 512
  # Result sets larger than this fail
  # Reason: Prevents memory bombs from buggy queries
  # Fix: Use LIMIT or OFFSET to paginate results
```

---

## G.6 Replication Settings

### replication_enabled
```yaml
replication:
  enabled: true  # Default: true
  # false for standalone instances
```

### replication_role
```yaml
replication:
  role: "primary"  # or "follower"
  # primary: Accepts writes, logs all changes
  # follower: Read-only, applies logs from primary
```

### follower_primary_address
```yaml
replication:
  follower_primary_address: "primary.example.com:8529"  # Only for followers
  # Follower connects to primary at this address
```

### replication_buffer_size_mb
```yaml
replication:
  buffer_size_mb: 256  # Default: 256
  # Follower buffer for incoming changes
  # Larger = handle larger write spikes
```

### replication_sync_interval_ms
```yaml
replication:
  sync_interval_ms: 100  # Default: 100
  # How often to sync WAL changes to follower
  # Lower = lower latency, higher CPU
  # 100ms is good for most workloads
```

---

## G.7 Security Settings

### authentication_enabled
```yaml
security:
  authentication:
    enabled: true  # Default: true
  # false: No authentication (dev-only!)
```

### jwt_secret
```yaml
security:
  authentication:
    jwt_secret: "${JWT_SECRET}"  # Must set via env var
  # Min 32 characters, high entropy
  # Generate: openssl rand -hex 32
```

### jwt_expiry_minutes
```yaml
security:
  authentication:
    jwt_expiry_minutes: 1440  # Default: 24 hours
  # Tokens expire after this time
  # Shorter = better security, more refresh overhead
```

### tls_enabled
```yaml
security:
  tls:
    enabled: true  # Default: true
  # false only for insecure local dev
```

### tls_cert_path
```yaml
security:
  tls:
    cert_path: "/etc/themis/certs/server.crt"
  # X.509 certificate (RSA 4096 recommended)
```

### tls_key_path
```yaml
security:
  tls:
    key_path: "/etc/themis/certs/server.key"
  # Private key (must have 0600 permissions)
```

### tls_min_version
```yaml
security:
  tls:
    min_version: "1.3"  # Default: "1.3"
  # Options: "1.2" (legacy) | "1.3" (modern)
  # Recommendation: "1.3" for new deployments
```

### mfa_required
```yaml
security:
  mfa_required: false  # Default: false
  # true: All users must enable MFA
```

---

## G.8 Audit Logging

### audit_enabled
```yaml
audit:
  enabled: true  # Default: true
  # Logs all changes to collections
```

### audit_directory
```yaml
audit:
  directory: "/var/log/themis/audit"
  # Must be on write-optimized storage
```

### audit_retention_days
```yaml
audit:
  retention_days: 365  # Default: 365 days
  # Older logs auto-purged (if compliance allows)
```

### audit_include_data
```yaml
audit:
  include_data: false  # Default: false
  # true: Store full document in audit log (verbose!)
  # Recommendation: false (log metadata only)
```

---

## G.9 Performance Tuning

### memory_cache_warmup
```yaml
performance:
  cache_warmup_enabled: true  # Default: true
  # Loads frequently accessed data into cache on startup
  # Reduces first-query latency
```

### memory_buffer_size_mb
```yaml
performance:
  memory_buffer_size_mb: 512  # Default: 512
  # In-memory write buffer before flushing to disk
  # Larger = better batching, more RAM
  # Rule: 10-20% of cache_size_mb
```

### io_parallelism
```yaml
performance:
  io_parallelism: 4  # Default: number of CPU cores
  # Concurrent IO operations
  # Higher = better throughput on SSD, diminishing returns on HDD
```

---

## G.10 Logging

### log_level
```yaml
logging:
  level: "info"  # Default: info
  # Options: "debug" | "info" | "warn" | "error" | "critical"
  # Recommendations:
  #   production: "warn"
  #   staging: "info"
  #   development: "debug"
```

### log_format
```yaml
logging:
  format: "json"  # Default: json
  # Options: "json" | "text"
  # json: Machine parseable (for aggregation)
  # text: Human readable
```

### log_retention_days
```yaml
logging:
  retention_days: 30  # Default: 30
  # Older logs auto-deleted
  # For compliance: increase to 365+
```

### log_max_file_size_mb
```yaml
logging:
  max_file_size_mb: 100  # Default: 100
  # Rotate log file when it exceeds this size
```

---

## G.11 Vector Search Tuning

### vector_similarity_metric
```yaml
vector:
  similarity_metric: "cosine"  # Default: cosine
  # Options: "cosine" | "euclidean" | "manhattan" | "dot"
  # cosine: Best for normalized embeddings (e.g., from BERT)
  # euclidean: Good for raw embeddings
```

### hnsw_ef_search
```yaml
vector:
  hnsw_ef_search: 100  # Default: 100
  # Exploration factor during search
  # Higher = more accurate, slower
  # Range: 10-1000
  # Rule: Start with 100, tune based on latency/accuracy tradeoff
```

### hnsw_m
```yaml
vector:
  hnsw_m: 16  # Default: 16
  # Connections per layer in HNSW graph
  # Higher = more accurate, more memory
  # Range: 8-64
```

---

## G.12 Development Settings

### dev_mode_enabled
```yaml
development:
  dev_mode_enabled: false  # Default: false
  # true: Disables some security checks (LOCAL DEV ONLY)
  # Enables: Hot reload, relaxed CORS, schema auto-creation
```

### cors_enabled
```yaml
development:
  cors_enabled: false  # Default: false
  # true: Allow cross-origin requests
  # Set specific origins in production (not "*")
```

### cors_origins
```yaml
development:
  cors_origins:
    - "http://localhost:3000"  # Frontend dev server
    - "http://localhost:8080"
```

---

## G.13 Example Configurations

### Single Node (Development)
```yaml
server:
  port: 8529
  bind_address: "127.0.0.1"
  max_connections: 100

database:
  wal_sync_mode: "os"  # Fast, eventual durability
  compression_algorithm: "lz4"

cache:
  size_mb: 512  # Small for dev machine

query:
  timeout_ms: -1  # No timeout for debugging

logging:
  level: "debug"
  format: "text"  # Human readable
```

### Production (Single Node, 32GB RAM)
```yaml
server:
  port: 8529
  bind_address: "0.0.0.0"
  max_connections: 1000

database:
  data_directory: "/data/themis"
  wal_sync_mode: "fsync"
  compression_algorithm: "zstd"

cache:
  size_mb: 8192  # 25% of 32GB

security:
  tls_enabled: true
  jwt_secret: "${JWT_SECRET}"  # From env var

audit:
  enabled: true
  retention_days: 365

logging:
  level: "warn"
  format: "json"
  retention_days: 90
```

### High-Performance (Writes, 64GB RAM)
```yaml
database:
  wal_sync_mode: "os"  # Trade safety for speed
  compression_algorithm: "lz4"  # Faster than zstd

cache:
  size_mb: 16384  # 25% of 64GB
  block_cache_size_mb: 4096

performance:
  memory_buffer_size_mb: 2048  # Larger batches
  io_parallelism: 8

query:
  max_query_result_size_mb: 2048  # Allow larger results
```

### Analytics (Reads, 128GB RAM)
```yaml
cache:
  size_mb: 32768  # 25% of 128GB
  mode: "lfu"  # Popularity-based caching

indexing:
  vector_index_default: "hnsw"
  hnsw_ef_search: 200  # More accuracy for analytics

query:
  timeout_ms: 300000  # 5 minutes (long aggregations)
  max_query_result_size_mb: 4096  # Large result sets
```

---

## G.14 Linux Kernel Tuning (sysctl)

These affect OS-level performance:

```bash
# File descriptors (support 10k+ connections)
echo "fs.file-max = 2097152" >> /etc/sysctl.conf
echo "* soft nofile 100000" >> /etc/security/limits.conf
echo "* hard nofile 100000" >> /etc/security/limits.conf

# Memory
echo "vm.swappiness = 10" >> /etc/sysctl.conf  # Prefer RAM over swap

# Network (if using TCP for replication)
echo "net.core.rmem_max = 134217728" >> /etc/sysctl.conf
echo "net.core.wmem_max = 134217728" >> /etc/sysctl.conf

# Apply changes
sysctl -p
```

---

## G.15 Environment Variables

### Common Env Vars
```bash
# Required
export JWT_SECRET="$(openssl rand -hex 32)"

# Optional
export THEMIS_PORT=8529
export THEMIS_BIND_ADDRESS=0.0.0.0
export THEMIS_DATA_DIR=/data/themis
export THEMIS_LOG_LEVEL=info
export THEMIS_CACHE_SIZE_MB=2048

# Run with env vars
themisdb run
```

---

## G.16 Configuration Validation

### Validate Config File
```bash
# Check syntax
themisdb config validate /etc/themis/themis.conf

# Check all settings
themisdb config show
```

### Health Check
```bash
# After startup
curl -s http://localhost:8529/_admin/health | jq .

# Expected:
# {
#   "status": "ready",
#   "uptime_seconds": 123,
#   "memory_mb": 456,
#   "cache_hit_rate": 0.87
# }
```

---

## G.17 Configuration Change Checklist

Before changing production config:
- [ ] Backup current config: `cp themis.conf themis.conf.backup`
- [ ] Test change on staging first
- [ ] If safe: `systemctl reload themis` (no restart)
- [ ] Monitor metrics for 5 minutes
- [ ] Have rollback plan (restore .backup file)

---

## Summary

**Most Common Tuning Knobs:**
1. **cache_size_mb** - Available memory (25-30% of RAM)
2. **wal_sync_mode** - Safety vs. latency tradeoff
3. **compression_algorithm** - Storage vs. speed
4. **log_level** - Observability cost
5. **query_timeout_ms** - Prevent runaway queries

Start with defaults, measure, change one thing at a time, monitor impact.
