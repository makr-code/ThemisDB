# Hot Spare Management System - API Documentation

## Overview

The Hot Spare Management system provides automatic failover and rebuild capabilities for ThemisDB sharded deployments, enabling zero-downtime recovery from shard failures.

## Features

- **Automatic Failover**: Detect shard failures and promote spare shards in < 5 seconds
- **Background Rebuild**: Copy data from replicas to spare shards with configurable throttling
- **Progress Tracking**: Monitor rebuild progress with ETA calculation
- **Prometheus Metrics**: Export detailed metrics for monitoring
- **Configurable Policies**: Customize rebuild priority, throttling, and health checks

## Quick Start

### Basic Configuration

```cpp
#include "sharding/hot_spare_manager.h"
#include "sharding/redundancy_strategy.h"

using namespace themisdb::sharding;

// Configure redundancy with hot spares
RedundancyConfig config;
config.mode = RedundancyMode::MIRROR;
config.replication_factor = 3;
config.hot_spare.enable = true;
config.hot_spare.spare_shards = {"spare-1", "spare-2", "spare-3"};
config.hot_spare.auto_rebuild = true;
config.hot_spare.rebuild_throttle_mbps = 100;
config.hot_spare.health_check_interval = std::chrono::seconds(30);

// Create redundancy strategy
RedundancyStrategy strategy(config);

// Create topology
ShardTopology topology;

// Create hot spare manager
HotSpareConfig spare_config;
spare_config.enable = true;
spare_config.spare_shards = {"spare-1", "spare-2", "spare-3"};
spare_config.auto_rebuild = true;
spare_config.rebuild_priority = RebuildPriority::HIGH;
spare_config.rebuild_throttle_mbps = 100;
spare_config.health_check_interval = std::chrono::seconds(30);

HotSpareManager spare_manager(spare_config, strategy, topology);

// Start the manager
spare_manager.start();
```

### Monitoring Rebuild Progress

```cpp
// Get rebuild status
auto status = spare_manager.getRebuildStatus();

if (status.is_rebuilding) {
    std::cout << "Active rebuilds: " << status.active_rebuilds << std::endl;
    std::cout << "Overall progress: " << status.overall_progress << "%" << std::endl;
    std::cout << "ETA: " << status.estimated_time_remaining.count() << " seconds" << std::endl;
    std::cout << "Throughput: " << status.average_throughput_mbps << " MB/s" << std::endl;
}
```

### Manual Failover

```cpp
// Define handlers for data access
auto write_handler = [](const std::string& shard_id, 
                       const std::string& doc_id,
                       const std::vector<uint8_t>& data) -> bool {
    // Your write logic here
    return true;
};

auto read_handler = [](const std::string& shard_id, 
                      const std::string& doc_id) -> std::optional<std::vector<uint8_t>> {
    // Your read logic here
    return std::nullopt;
};

auto doc_iterator = [](const std::string& shard_id) -> std::vector<std::string> {
    // Return list of document IDs in shard
    return {};
};

// Activate spare for failed shard
ConsistentHashRing ring(100);
bool success = spare_manager.activateSpare(
    "failed-shard-id",
    ring,
    read_handler,
    write_handler,
    doc_iterator
);

if (success) {
    std::cout << "Spare activated successfully" << std::endl;
}
```

### Rebuild Control

```cpp
// Pause rebuild for a spare
spare_manager.pauseRebuild("spare-1");

// Resume rebuild
spare_manager.resumeRebuild("spare-1");

// Cancel rebuild
spare_manager.cancelRebuild("spare-1");
```

### Statistics and Monitoring

```cpp
// Get statistics
auto stats = spare_manager.getStats();
std::cout << "Total failovers: " << stats.total_failovers << std::endl;
std::cout << "Successful failovers: " << stats.successful_failovers << std::endl;
std::cout << "Average failover time: " << stats.avg_failover_time.count() << "ms" << std::endl;
std::cout << "Available spares: " << stats.spares_available << std::endl;

// Get failover history
auto history = spare_manager.getFailoverHistory(10);
for (const auto& event : history) {
    std::cout << "Failover: " << event.failed_shard_id 
              << " -> " << event.spare_shard_id 
              << " (" << event.failover_duration.count() << "ms)" << std::endl;
}

// Export Prometheus metrics
std::string metrics = spare_manager.exportPrometheusMetrics();
std::cout << metrics << std::endl;
```

## Configuration Options

### HotSpareConfig

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `enable` | bool | false | Enable hot spare management |
| `spare_shards` | vector<string> | [] | List of spare shard IDs |
| `auto_rebuild` | bool | true | Automatically rebuild after failover |
| `rebuild_priority` | RebuildPriority | HIGH | Rebuild priority level |
| `rebuild_throttle_mbps` | uint32_t | 100 | Bandwidth limit in MB/s |
| `health_check_interval` | seconds | 30 | Health check frequency |
| `max_concurrent_rebuilds` | uint32_t | 2 | Maximum parallel rebuilds |
| `rebuild_chunk_size_mb` | uint32_t | 64 | Chunk size for rebuild |

### RebuildPriority

- **LOW**: Minimal impact, background rebuild
- **MEDIUM**: Balanced approach
- **HIGH**: Aggressive rebuild for faster recovery (default)
- **CRITICAL**: Maximum speed, may impact production

### SpareState

- **AVAILABLE**: Ready for activation
- **ACTIVATING**: Being promoted to active
- **ACTIVE**: Currently serving traffic
- **REBUILDING**: Receiving data from replicas
- **DEGRADED**: Rebuild failed or incomplete
- **OFFLINE**: Not available

## Prometheus Metrics

The Hot Spare Manager exports the following Prometheus metrics:

```
# Failover metrics
themis_hot_spare_total_failovers
themis_hot_spare_successful_failovers
themis_hot_spare_failed_failovers
themis_hot_spare_avg_failover_time_ms

# Rebuild metrics
themis_hot_spare_total_rebuilds
themis_hot_spare_successful_rebuilds
themis_hot_spare_failed_rebuilds
themis_hot_spare_avg_rebuild_time_ms

# Spare status
themis_hot_spare_spares_available
themis_hot_spare_spares_active
themis_hot_spare_spares_rebuilding

# Per-spare metrics
themis_hot_spare_state{shard="spare-1",state="active"}
themis_hot_spare_rebuild_progress{shard="spare-1"}
themis_hot_spare_rebuild_throughput_mbps{shard="spare-1"}
themis_hot_spare_rebuild_eta_seconds{shard="spare-1"}
```

## Error Handling

### No Available Spares

```cpp
bool success = spare_manager.activateSpare(...);
if (!success) {
    // Check if spares are available
    auto available = spare_manager.getAvailableSpares();
    if (available.empty()) {
        // Alert: No spares available for failover!
        // Consider adding more spares or scaling up
    }
}
```

### Rebuild Failures

```cpp
// Monitor rebuild status
auto status = spare_manager.getRebuildStatus();
if (status.total_rebuilds_failed > 0) {
    // Check failed rebuilds and take action
    // Consider retry or manual intervention
}
```

## Best Practices

1. **Spare Capacity**: Maintain at least 20% spare capacity for your cluster
2. **Health Checks**: Set appropriate health check intervals (30-60 seconds)
3. **Rebuild Throttling**: Balance between recovery speed and production impact
4. **Monitoring**: Set up alerts for failover events and rebuild failures
5. **Testing**: Regularly test failover scenarios in staging environments

## Integration with RedundancyStrategy

```cpp
// The hot spare configuration is embedded in RedundancyConfig
RedundancyConfig config;
config.mode = RedundancyMode::MIRROR;
config.hot_spare.enable = true;
config.hot_spare.spare_shards = {"spare-1", "spare-2"};

RedundancyStrategy strategy(config);
```

## Lifecycle Management

```cpp
// Start hot spare manager
spare_manager.start();

// ... run operations ...

// Stop hot spare manager (graceful shutdown)
spare_manager.stop();
```

## Advanced Usage

### Custom Alert Callbacks

```cpp
spare_config.enable_alerts = true;
spare_config.alert_callback = [](const std::string& message) {
    // Send to your alerting system
    send_to_pagerduty(message);
    log_to_monitoring(message);
};
```

### Dynamic Spare Management

```cpp
// Add spare at runtime
spare_manager.addSpare("spare-4");

// Remove spare (only if not active)
spare_manager.removeSpare("spare-4");
```

### Configuration Updates

```cpp
// Update configuration dynamically
HotSpareConfig new_config = spare_manager.getConfig();
new_config.rebuild_throttle_mbps = 200;  // Increase bandwidth
spare_manager.updateConfig(new_config);
```

## Performance Characteristics

- **Failover Time**: Typically < 5 seconds
- **Rebuild Rate**: Configurable, 10-100 MB/s default
- **Memory Overhead**: ~100 KB per spare shard
- **CPU Overhead**: Minimal (< 1% during health checks)

## Limitations

1. Maximum concurrent rebuilds: Limited by `max_concurrent_rebuilds`
2. Requires at least one available spare for failover
3. Rebuild performance depends on network bandwidth
4. Does not automatically provision new spares after activation

## See Also

- [RedundancyStrategy Documentation](redundancy_strategy.md)
- [Auto Recovery Manager](auto_recovery_manager.md)
- [RAID Quick Start Guide](raid_quickstart.md)
- [Prometheus Metrics Guide](prometheus_metrics.md)
