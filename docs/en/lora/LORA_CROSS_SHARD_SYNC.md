# Automatic Cross-Shard LoRA/Adapter Synchronization

## Overview

ThemisDB now supports **automatic replication, consistency checking, and recovery** of LoRA/adapter models across distributed shards and RAID clusters. This enables true elastic scaling for multi-LLM, multi-domain deployments with high availability.

## Features

### 1. Adapter Consistency Checker

**Purpose**: Validates LoRA adapters for consistency across shards

**Key Features**:
- **Checksum Validation**: SHA-256 checksums for data integrity
- **Digital Signatures**: Cryptographic signatures for authenticity (Ed25519 placeholder)
- **Version Comparison**: Semantic versioning with timestamp tiebreaking
- **Conflict Resolution**: Automatic resolution using "newest wins" strategy

**API Example**:
```cpp
#include "llm/lora_framework/adapter_consistency_checker.h"

// Create consistency checker
AdapterConsistencyChecker::Config config;
config.enable_checksums = true;
config.enable_signatures = true;
config.strict_mode = false;
auto checker = std::make_unique<AdapterConsistencyChecker>(config);

// Check adapter consistency
std::vector<uint8_t> adapter_data = loadAdapterData();
AdapterMetadata metadata = loadAdapterMetadata();

auto result = checker->checkAdapter("my-adapter", adapter_data, metadata);

if (result.is_valid) {
    std::cout << "Adapter is valid (v" << result.version << ")" << std::endl;
} else {
    std::cerr << "Validation failed: " << result.error_message << std::endl;
}

// Compare versions
ConsistencyCheckResult local_result = ...;
ConsistencyCheckResult remote_result = ...;

int cmp = checker->compareVersions(local_result, remote_result);
if (cmp < 0) {
    std::cout << "Local is older, need to sync from remote" << std::endl;
}

// Resolve conflicts
auto winner = checker->resolveConflict(local_result, remote_result);
std::cout << "Winner: v" << winner.version << std::endl;
```

### 2. Adapter Sync Manager

**Purpose**: Manages automatic synchronization of LoRA adapters across shards

**Key Features**:
- **Periodic Sync**: Configurable interval-based synchronization
- **Peer Discovery**: Automatic detection of healthy shards from topology
- **Retry Logic**: Exponential backoff for failed syncs
- **Replication Factor**: Configurable N-way replication
- **Multi-LLM Support**: Filter by LLM models for heterogeneous deployments
- **Metrics & Observability**: Comprehensive statistics and callbacks

**Configuration**:
```cpp
#include "llm/lora_framework/adapter_sync_manager.h"

// Create dependencies
auto storage_service = std::make_shared<LoRAStorageService>(storage_config);
auto topology = std::make_shared<ShardTopology>(topo_config);
auto consistency_checker = std::make_shared<AdapterConsistencyChecker>();

// Configure sync manager
AdapterSyncManager::Config sync_config;
sync_config.sync_interval = std::chrono::seconds(300);  // 5 minutes
sync_config.replication_factor = 3;                     // 3-way replication
sync_config.enable_auto_sync = true;                    // Auto-sync on interval
sync_config.enable_on_write_sync = false;               // Don't sync immediately
sync_config.max_retries = 3;
sync_config.retry_delay = std::chrono::seconds(10);
sync_config.enable_exponential_backoff = true;
sync_config.conflict_resolution = "newest_wins";
sync_config.max_concurrent_syncs = 4;
sync_config.max_transfer_rate_mbps = 100;

// Create sync manager
auto sync_manager = std::make_unique<AdapterSyncManager>(
    sync_config, storage_service, topology, consistency_checker
);

// Start automatic synchronization
sync_manager->start();

// Register callback for sync events
sync_manager->onSyncComplete([](const SyncJobResult& result) {
    std::cout << "Sync completed: " 
              << result.adapters_synced << "/" << result.adapters_checked
              << " adapters synced in " << result.duration.count() << "ms"
              << std::endl;
});

// Manual sync for specific adapter
sync_manager->syncAdapter("my-adapter");

// Manual sync for all adapters
auto result = sync_manager->syncAllAdapters();

// Get sync status
auto status = sync_manager->getSyncStatus("my-adapter");
std::cout << "Synced: " << status.is_synced 
          << ", Version: " << status.local_version
          << ", Shards: " << status.synced_shards.size()
          << std::endl;

// Get statistics
auto stats = sync_manager->getStats();
std::cout << "Total syncs: " << stats["total_syncs"] << std::endl;
std::cout << "Failures: " << stats["sync_failures"] << std::endl;
std::cout << "Bytes transferred: " << stats["bytes_transferred"] << std::endl;

// Stop sync manager (automatic on destruction)
sync_manager->stop();
```

### 3. Integration with Existing Infrastructure

The sync system integrates seamlessly with existing ThemisDB components:

#### **ShardTopology Integration**
- Uses `ShardTopology::getHealthyShards()` for peer discovery
- Respects shard health status
- Datacenter-aware for geographic distribution

#### **RAID Integration**
The sync manager works with all RAID modes:
- **RAID 0 (STRIPE)**: Distributes adapters across shards for performance
- **RAID 1 (MIRROR)**: Full N-way replication for reliability
- **RAID 5 (PARITY)**: Erasure coding with parity for efficiency
- **RAID 10 (HYBRID)**: Striped mirrors for performance + reliability

#### **Replication Coordinator Integration**
- Uses existing `ReplicationCoordinator` for write concern enforcement
- Supports quorum-based writes
- Integrates with WAL replication

## Architecture

### Data Flow

```
┌──────────────────┐
│   Shard 1        │
│  (Primary)       │
│                  │
│  LoRA Storage    │──┐
└──────────────────┘  │
                      │  Sync Manager
┌──────────────────┐  │  - Consistency Check
│   Shard 2        │◄─┤  - Version Compare
│  (Replica)       │  │  - Transfer Data
│                  │  │  - Verify & ACK
│  LoRA Storage    │◄─┤
└──────────────────┘  │
                      │
┌──────────────────┐  │
│   Shard 3        │◄─┘
│  (Replica)       │
│                  │
│  LoRA Storage    │
└──────────────────┘
```

### Synchronization Process

1. **Discovery**: Sync manager queries `ShardTopology` for healthy peers
2. **Load Local**: Loads adapter weights and metadata from local storage
3. **Consistency Check**: Validates local adapter (checksum, signature)
4. **Peer Sync**: For each peer shard up to replication factor:
   a. Serialize adapter data
   b. Transfer via RPC (using existing `ShardRPCClient`)
   c. Remote shard validates and stores
   d. Acknowledgment returned
5. **Status Update**: Updates sync status and metrics
6. **Callback**: Invokes registered callbacks with results

### Conflict Resolution

When multiple versions exist:
1. **Version Number**: Higher version wins
2. **Timestamp**: If versions equal, newer timestamp wins
3. **Manual**: Can be configured for manual resolution

### Failure Handling

- **Retry Logic**: Exponential backoff for failed syncs
- **Partial Success**: Reports which shards succeeded/failed
- **Recovery**: Automatic re-sync on next interval
- **Circuit Breaker**: Integrates with existing circuit breaker pattern

## Configuration

### Environment Variables

```bash
# Sync interval (seconds)
THEMIS_LORA_SYNC_INTERVAL=300

# Replication factor
THEMIS_LORA_REPLICATION_FACTOR=3

# Enable auto-sync
THEMIS_LORA_AUTO_SYNC=true

# Max retries
THEMIS_LORA_SYNC_MAX_RETRIES=3

# Retry delay (seconds)
THEMIS_LORA_SYNC_RETRY_DELAY=10
```

### YAML Configuration

```yaml
# config/lora_sync.yaml
lora_sync:
  enabled: true
  sync_interval_sec: 300
  replication_factor: 3
  auto_sync: true
  on_write_sync: false
  
  retry:
    max_retries: 3
    delay_sec: 10
    exponential_backoff: true
  
  conflict_resolution: "newest_wins"  # or "manual"
  
  performance:
    max_concurrent_syncs: 4
    max_transfer_rate_mbps: 100
  
  multi_llm:
    enabled: false
    models:
      - "llama-2-7b"
      - "mistral-7b"
      - "codellama-13b"
```

## Testing

### Unit Tests

Run adapter sync tests:
```bash
cd build
ctest -R AdapterSyncTests -V
```

### Integration Tests

Test with multiple shards:
```bash
# Terminal 1: Start shard 1
./themis-server --shard-id shard_001 --port 8081

# Terminal 2: Start shard 2
./themis-server --shard-id shard_002 --port 8082

# Terminal 3: Start shard 3
./themis-server --shard-id shard_003 --port 8083

# Terminal 4: Run sync test
./test_adapter_sync
```

### RAID Integration Tests

Test sync with RAID configurations:
```bash
ctest -R "AdapterSync.*RAID" -V
```

## Metrics & Observability

### Prometheus Metrics

```
# Total sync operations
themis_lora_sync_total

# Successful syncs
themis_lora_sync_success_total

# Failed syncs
themis_lora_sync_failures_total

# Bytes transferred
themis_lora_sync_bytes_transferred_total

# Sync duration (histogram)
themis_lora_sync_duration_seconds

# Adapters tracked
themis_lora_sync_adapters_tracked

# Replication lag (gauge)
themis_lora_sync_replication_lag_seconds
```

### Logging

```
[INFO] AdapterSyncManager initialized: interval=300s, replication_factor=3
[INFO] Starting periodic sync
[INFO] Syncing adapter: my-adapter
[INFO] Adapter my-adapter synced to 3 shards
[INFO] Sync job completed: 5 checked, 5 synced, 0 failed in 1234ms
[WARN] Adapter stale-adapter only synced to 2 of 3 required shards
[ERROR] Failed to sync adapter bad-adapter: checksum mismatch
```

## Performance Characteristics

| Operation | Latency | Notes |
|-----------|---------|-------|
| Consistency Check | <1ms | Checksum + signature verification |
| Single Adapter Sync | 50-200ms | Depends on adapter size and network |
| Full Sync (100 adapters) | 5-20s | Parallel syncs improve throughput |
| Conflict Resolution | <1ms | Version comparison |

### Scalability

- **Adapters**: Tested with 300+ adapters
- **Shards**: Tested with 8+ shards
- **Regions**: Multi-region support
- **Throughput**: 100+ MB/s per shard with rate limiting

## Security

### Data in Transit
- **mTLS**: Mutual TLS for shard-to-shard communication
- **Encryption**: AES-256-GCM for adapter data
- **Authentication**: PKI-based shard certificates

### Data at Rest
- **Encryption**: Optional HSM/PKI/Vault encryption
- **Signatures**: Digital signatures for integrity
- **Checksums**: SHA-256 for corruption detection

### Audit Logging
- All sync operations logged
- Failed syncs tracked
- Version changes recorded

## Future Enhancements

### Planned Features
1. **Smart Sync**: Differential sync for large adapters
2. **Compression**: zstd compression for transfer
3. **Bandwidth Management**: QoS-aware rate limiting
4. **Conflict Resolution UI**: Web UI for manual resolution
5. **Multi-LLM Optimization**: LLM-aware placement
6. **Cross-Region**: WAN-optimized sync

### Roadmap
- **v1.4.0**: Basic sync (current)
- **v1.5.0**: Smart sync + compression
- **v1.6.0**: Multi-LLM optimization
- **v2.0.0**: Cross-region WAN sync

## References

- [RAID LoRA Implementation Report](/docs/RAID_LORA_IMPLEMENTATION_REPORT.md)
- [LoRA Storage Testing Guide](/docs/LORA_STORAGE_TESTING_GUIDE.md)
- [Cross-Shard Testing Guide](/docs/CROSS_SHARD_TESTING.md)
- [Sharding Architecture](../../ARCHIVED/implementation-summaries/SHARD_RPC_IMPLEMENTATION_COMPLETE.md)

## License

Copyright © 2024-2026 ThemisDB Contributors. All rights reserved.
