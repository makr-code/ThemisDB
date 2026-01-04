# ThemisDB RAID Quick Start Guide

**Version:** 1.3.3+  
**Last Updated:** 2026-01-04

## Introduction

This guide helps you quickly get started with ThemisDB's RAID-like redundancy system for distributed sharding. Choose the right RAID mode for your use case and configure it in minutes.

## Quick Configuration Examples

### RAID 1 (Mirror) - High Availability

Best for: Critical data that requires high availability and fast reads.

```yaml
# config/raid_config.yaml
collections:
  critical_data:
    mode: MIRROR
    replication_factor: 3
    write_concern: MAJORITY
    read_preference: NEAREST
    
    # Automatic failover
    enable_auto_failover: true
    failover_timeout_ms: 5000
```

**C++ API:**
```cpp
#include "sharding/redundancy_strategy.h"

RedundancyConfig config;
config.mode = RedundancyMode::MIRROR;
config.replication_factor = 3;
config.write_concern = WriteConcern::MAJORITY;
config.read_preference = ReadPreference::NEAREST;

RedundancyStrategy strategy(config);
```

### RAID 5 (Parity) - Storage Efficiency

Best for: Large datasets where storage efficiency matters more than write performance.

```yaml
collections:
  large_datasets:
    mode: PARITY
    erasure_coding:
      data_shards: 4
      parity_shards: 2
      algorithm: REED_SOLOMON
    
    # Storage: 67% efficiency (stores 6 units for 4 units of data)
    # Fault tolerance: 2 shard failures
```

**C++ API:**
```cpp
RedundancyConfig config;
config.mode = RedundancyMode::PARITY;
config.erasure_coding.data_shards = 4;
config.erasure_coding.parity_shards = 2;
config.erasure_coding.algorithm = ErasureCodingAlgorithm::REED_SOLOMON;

RedundancyStrategy strategy(config);
```

### RAID 0 (Stripe) - Maximum Performance

Best for: Temporary data, caches, or data that can be regenerated.

```yaml
collections:
  cache_data:
    mode: STRIPE
    stripe:
      stripe_size_kb: 64
      min_stripe_shards: 4
      parallel_stripe_io: true
```

**C++ API:**
```cpp
RedundancyConfig config;
config.mode = RedundancyMode::STRIPE;
config.stripe.stripe_size_kb = 64;
config.stripe.min_stripe_shards = 4;

RedundancyStrategy strategy(config);
```

### RAID 10 (Stripe+Mirror) - Performance + Reliability

Best for: High-performance applications with critical data.

```yaml
collections:
  high_perf_critical:
    mode: STRIPE_MIRROR
    replication_factor: 2
    stripe:
      stripe_size_kb: 64
    write_concern: ALL
```

## Common Operations

### Writing Data

```cpp
// Prepare data
std::vector<uint8_t> data = loadDocument();

// Write with automatic redundancy
auto result = strategy.write(
    "doc123",           // document ID
    data,               // data to write
    "my_collection",    // collection name
    ring,               // consistent hash ring
    topology,           // shard topology
    writeHandler        // write callback
);

if (result.success) {
    std::cout << "Written to " << result.written_shards.size() << " shards\n";
    std::cout << "Latency: " << result.latency.count() << "ms\n";
}
```

### Reading Data

```cpp
auto result = strategy.read(
    "doc123",
    "my_collection",
    ring,
    topology,
    readHandler
);

if (result.success) {
    std::cout << "Read from: " << result.source_shard << "\n";
    std::cout << "Data size: " << result.data.size() << " bytes\n";
}
```

### Monitoring Health

```cpp
auto health = strategy.checkDocumentHealth(
    "doc123",
    "my_collection",
    ring,
    topology,
    readHandler
);

std::cout << "Available replicas: " << health.available_replicas 
          << "/" << health.required_replicas << "\n";
std::cout << "Can recover: " << (health.can_recover ? "yes" : "no") << "\n";
```

## Performance Tuning

### For Maximum Throughput (RAID 0)
- Increase `stripe_size_kb` to 128 or 256
- Enable `parallel_stripe_io`
- Use more shards (`min_stripe_shards`)

### For Maximum Reliability (RAID 1)
- Set `replication_factor` to 3 or higher
- Use `write_concern: ALL` for critical data
- Enable `enable_auto_failover`

### For Storage Efficiency (RAID 5)
- Adjust data/parity ratio (e.g., 6+3 for larger datasets)
- Consider `algorithm: CAUCHY` for better performance
- Monitor recovery time with higher parity counts

## Monitoring and Metrics

### Prometheus Metrics

```
# Writes
themis_redundancy_writes_total
themis_redundancy_bytes_written_total

# Reads  
themis_redundancy_reads_total
themis_redundancy_bytes_read_total

# Health
themis_redundancy_degraded_documents
themis_redundancy_recoveries_total
```

### Example Grafana Query

```promql
# Write throughput by mode
rate(themis_redundancy_writes_total[5m])

# Storage efficiency
themis_redundancy_logical_bytes / themis_redundancy_physical_bytes

# Read latency
histogram_quantile(0.95, rate(themis_redundancy_read_latency_bucket[5m]))
```

## Troubleshooting

### Degraded Documents

```cpp
auto degraded = manager.getDegradedDocuments();
for (const auto& doc_id : degraded) {
    // Trigger repair
    strategy.recoverDocument(doc_id, collection, ring, topology, 
                            readHandler, writeHandler);
}
```

### Shard Failure

```bash
# Check shard health
curl http://localhost:8080/api/sharding/health

# Manual failover
curl -X POST http://localhost:8080/api/sharding/failover \
  -d '{"failed_shard": "shard-2", "target_shard": "shard-5"}'
```

### Performance Issues

1. **Slow writes with RAID 1?** 
   - Reduce `write_concern` to MAJORITY
   - Check network latency between shards

2. **High CPU with RAID 5?**
   - Reed-Solomon encoding is CPU-intensive
   - Consider reducing parity shards
   - Enable hardware acceleration if available

3. **Storage filling up?**
   - Check replication factor settings
   - Consider switching from MIRROR to PARITY mode
   - Monitor `storage_efficiency` metric

## Migration Between Modes

```cpp
// Migrate collection from MIRROR to PARITY
auto migrator = DataMigrator::create(ring, topology);

MigrationPlan plan;
plan.source_mode = RedundancyMode::MIRROR;
plan.target_mode = RedundancyMode::PARITY;
plan.target_config.erasure_coding.data_shards = 4;
plan.target_config.erasure_coding.parity_shards = 2;

migrator.migrate("my_collection", plan);
```

## Best Practices

1. **Start with RAID 1** for simplicity and reliability
2. **Use RAID 5** for large, infrequently-modified datasets
3. **Reserve RAID 0** for truly temporary data
4. **Test failover** scenarios in staging before production
5. **Monitor storage efficiency** and adjust as needed
6. **Set up alerts** for degraded documents
7. **Regular backups** even with redundancy
8. **Document your RAID configuration** for operations team

## Next Steps

- Read the [Full RAID Implementation Report](../../RAID_LORA_IMPLEMENTATION_REPORT.md)
- Check [Production Deployment Guide](../de/SHARDING_PRODUCTION_DEPLOYMENT_RAID_v1.4.md)
- Review [Monitoring Guide](../de/SHARDING_MONITORING_OBSERVABILITY_RAID_v1.4.md)
- Explore [Configuration Reference](../de/SHARDING_RAID_MODES_CONFIGURATION_v1.4.md)

## Support

- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Documentation: https://themisdb.io/docs
- Community: https://discord.gg/themisdb
