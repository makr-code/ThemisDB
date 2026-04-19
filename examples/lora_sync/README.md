> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# LoRA Cross-Shard Synchronization Example

This example demonstrates how to use ThemisDB's automatic cross-shard LoRA/adapter synchronization feature.

## Overview

The example shows:
- Setting up the sync infrastructure (storage, topology, consistency checker)
- Configuring automatic synchronization with replication
- Creating and syncing LoRA adapters
- Monitoring sync status and metrics
- Handling sync callbacks

## Building

```bash
cd /home/runner/work/ThemisDB/ThemisDB
mkdir -p build && cd build
cmake .. -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ENABLE_LORA_TESTS=ON
cmake --build . --target example_lora_sync
```

## Running

```bash
./example_lora_sync
```

## Expected Output

```
=== ThemisDB LoRA Cross-Shard Synchronization Example ===

1. Creating LoRA Storage Service...
✓ Storage service created

2. Setting up Shard Topology...
  Added shard_1 (dc1)
  Added shard_2 (dc1)
  Added shard_3 (dc1)
  Added shard_4 (dc2)
  Added shard_5 (dc2)
✓ Topology configured with 5 shards

3. Creating Consistency Checker...
✓ Consistency checker created

4. Creating Sync Manager...
✓ Sync manager created
  Sync interval: 60s
  Replication factor: 3

5. Registering sync callback...
✓ Callback registered

6. Creating example adapters...
  ✓ Created adapter_1
  ✓ Created adapter_2
  ✓ Created adapter_3

7. Performing manual sync...
✓ Manual sync completed
  Checked: 3
  Synced: 3
  Failed: 0

8. Checking sync status...
  adapter_1:
    Synced: yes
    Version: 1
    Failures: 0
  adapter_2:
    Synced: yes
    Version: 1
    Failures: 0
  adapter_3:
    Synced: yes
    Version: 1
    Failures: 0

9. Viewing statistics...
  Total syncs: 1
  Successful: 3
  Failures: 0
  Bytes transferred: 3072
  Adapters tracked: 3

10. Starting automatic sync...
✓ Auto-sync is running
  Will sync every 60 seconds

Waiting 5 seconds...

11. Cleaning up...
✓ Sync manager stopped

=== Example completed successfully ===
```

## Key Components

### 1. Storage Service
Manages persistent storage of LoRA adapters with versioning and encryption support.

### 2. Shard Topology
Maintains cluster topology information including shard locations, health status, and datacenter awareness.

### 3. Consistency Checker
Validates adapter integrity using checksums and digital signatures, and resolves version conflicts.

### 4. Sync Manager
Orchestrates automatic synchronization across shards with configurable replication factor and retry logic.

## Configuration Options

### Replication Factor
Controls how many replica shards each adapter is synced to:
```cpp
sync_config.replication_factor = 3;  // 3-way replication
```

### Sync Interval
Controls how often auto-sync runs:
```cpp
sync_config.sync_interval = std::chrono::seconds(300);  // 5 minutes
```

### Conflict Resolution
Choose how version conflicts are resolved:
```cpp
sync_config.conflict_resolution = "newest_wins";  // or "manual"
```

### Performance
Control concurrent operations and rate limiting:
```cpp
sync_config.max_concurrent_syncs = 4;
sync_config.max_transfer_rate_mbps = 100;
```

## Integration with RAID

The sync manager works seamlessly with ThemisDB's RAID configurations:

- **RAID 1 (MIRROR)**: Full N-way replication for high availability
- **RAID 5 (PARITY)**: Erasure coding for storage efficiency
- **RAID 10**: Striped mirrors for performance + reliability

Set the replication factor to match your RAID configuration.

## Monitoring

### Status API
Check sync status for individual adapters:
```cpp
auto status = sync_manager->getSyncStatus("adapter_1");
std::cout << "Synced: " << status.is_synced << std::endl;
std::cout << "Version: " << status.local_version << std::endl;
```

### Statistics API
Get aggregate statistics:
```cpp
auto stats = sync_manager->getStats();
std::cout << "Total syncs: " << stats["total_syncs"] << std::endl;
std::cout << "Failures: " << stats["sync_failures"] << std::endl;
```

### Prometheus Metrics
If built with Prometheus support, metrics are automatically exported:
- `themis_lora_sync_syncs_total` - Total sync operations
- `themis_lora_sync_syncs_success_total` - Successful syncs
- `themis_lora_sync_syncs_failures_total` - Failed syncs
- `themis_lora_sync_bytes_transferred_total` - Bytes transferred
- `themis_lora_sync_sync_duration_seconds` - Sync duration histogram
- `themis_lora_sync_adapters_tracked` - Number of adapters tracked
- `themis_lora_sync_replication_lag_seconds` - Replication lag gauge

## See Also

- [Cross-Shard Sync Documentation](/docs/LORA_CROSS_SHARD_SYNC.md)
- [RAID Implementation Report](/docs/RAID_LORA_IMPLEMENTATION_REPORT.md)
- [LoRA Storage Testing Guide](/docs/LORA_STORAGE_TESTING_GUIDE.md)
