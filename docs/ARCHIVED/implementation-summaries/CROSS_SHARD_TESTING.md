# Cross-Shard Testing Guide

## Overview

This guide covers testing multi-shard LoRA operations in ThemisDB, including serialization, network transfer, replication, and failure handling across distributed shards.

## Architecture

### Shard Topology

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Shard 0   │────▶│   Shard 1   │────▶│   Shard 2   │
│             │     │             │     │             │
│ LoRA Store  │     │ LoRA Store  │     │ LoRA Store  │
└─────────────┘     └─────────────┘     └─────────────┘
      │                   │                   │
      └───────────────────┴───────────────────┘
                Network Layer
```

### Data Flow

1. **Serialization**: LoRA adapter → Binary format
2. **Transfer**: Binary → Network → Remote shard
3. **Deserialization**: Binary → LoRA adapter
4. **Verification**: Checksum validation

## Test Infrastructure

### Mock Shard Cluster

The `MockShardCluster` simulates a distributed shard environment for testing:

```cpp
#include "fixtures/mock_shard_cluster.h"

// Create 3-shard cluster
MockShardCluster cluster(3);

// Configure network behavior
cluster.setLatency(10, 50);           // 10-50ms latency
cluster.injectPacketLoss(0.1f);       // 10% packet loss
cluster.simulateNetworkPartition({1}); // Isolate shard 1
```

### Features

- **Network Simulation**
  - Configurable latency (min/max range)
  - Packet loss injection (0-100%)
  - Network partition simulation
  - Bandwidth throttling (planned)

- **Shard Management**
  - Dynamic shard failure/recovery
  - Health status tracking
  - Operation statistics (reads/writes/bytes)

- **Data Operations**
  - Key-value storage per shard
  - Atomic operations
  - Concurrent access support

## Serialization Format

### Binary Format Structure

```
┌──────────────────────────────────────────────────────┐
│  4 bytes: Metadata Size (uint32_t)                   │
├──────────────────────────────────────────────────────┤
│  N bytes: Metadata (JSON)                            │
│    - adapter_id                                      │
│    - base_model                                      │
│    - version                                         │
│    - hyperparameters                                 │
│    - created_at / updated_at                         │
├──────────────────────────────────────────────────────┤
│  M bytes: Weights Data (binary)                      │
│    - Adapter weight tensors                          │
│    - Format: safetensors/pickle/gguf                 │
└──────────────────────────────────────────────────────┘
```

### Serialization API

```cpp
// Serialize LoRA to binary
std::vector<uint8_t> serializeLoRA(
    const AdapterWeights& weights,
    const AdapterMetadata& metadata);

// Deserialize binary to LoRA
bool deserializeLoRA(
    const std::vector<uint8_t>& data,
    AdapterWeights& weights,
    AdapterMetadata& metadata);
```

### Checksum Calculation

```cpp
uint32_t calculateChecksum(const std::vector<uint8_t>& data) {
    uint32_t checksum = 0;
    for (uint8_t byte : data) {
        checksum = (checksum << 1) ^ byte;
    }
    return checksum;
}
```

## Test Scenarios

### 1. Basic Cross-Shard Transfer

**Test:** `TransferLoRAAcrossShards`

```cpp
// Save to shard 0
auto lora_data = createTestLoRAData(2048);
cluster->saveToShard(0, "lora:adapter1", lora_data);

// Load from shard 0
auto loaded = cluster->loadFromShard(0, "lora:adapter1");

// Transfer to shard 1
cluster->saveToShard(1, "lora:adapter1", *loaded);

// Verify byte-exact copy
auto from_shard_1 = cluster->loadFromShard(1, "lora:adapter1");
ASSERT_EQ(*from_shard_1, lora_data);
```

**Expected:** ✅ Byte-exact transfer with integrity preservation

### 2. Network Latency Simulation

**Test:** `TransferWithNetworkLatency`

```cpp
cluster->setLatency(10, 50);  // 10-50ms latency

auto start = std::chrono::high_resolution_clock::now();
cluster->saveToShard(0, key, data);
auto end = std::chrono::high_resolution_clock::now();

auto latency = duration_cast<milliseconds>(end - start);
EXPECT_GE(latency.count(), 10);  // At least min latency
```

**Expected:** ✅ Operations delayed by configured latency

### 3. Packet Loss Handling

**Test:** `TransferWithPacketLoss`

```cpp
cluster->injectPacketLoss(0.3f);  // 30% packet loss

int successful = 0;
for (int i = 0; i < 10; i++) {
    if (cluster->saveToShard(0, key, data)) {
        successful++;
    }
}

// ~70% success rate expected
EXPECT_GT(successful, 5);
EXPECT_LT(successful, 10);
```

**Expected:** ✅ ~70% success rate (100% - 30% loss)

### 4. Multi-Shard Replication

**Test:** `ReplicateAcrossShards`

```cpp
// Replicate to all shards
for (int shard : {0, 1, 2}) {
    cluster->saveToShard(shard, key, lora_data);
}

// Verify on all shards
for (int shard : {0, 1, 2}) {
    auto loaded = cluster->loadFromShard(shard, key);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, lora_data);
}
```

**Expected:** ✅ Identical copies on all shards

### 5. Shard Failure During Transfer

**Test:** `TransferFailsWhenShardDown`

```cpp
cluster->failShard(1);

bool saved = cluster->saveToShard(1, key, data);
EXPECT_FALSE(saved);  // Transfer fails

cluster->recoverShard(1);

saved = cluster->saveToShard(1, key, data);
EXPECT_TRUE(saved);  // Transfer succeeds after recovery
```

**Expected:** ✅ Graceful failure detection and recovery

### 6. Network Partition

**Test:** `NetworkPartitionIsolatesShards`

```cpp
// Partition shard 1
cluster->simulateNetworkPartition({1});

// Operations on shard 1 fail
EXPECT_FALSE(cluster->saveToShard(1, key, data));

// Operations on other shards succeed
EXPECT_TRUE(cluster->saveToShard(0, key, data));
EXPECT_TRUE(cluster->saveToShard(2, key, data));

// Heal partition
cluster->healNetworkPartition();

// Shard 1 accessible again
EXPECT_TRUE(cluster->saveToShard(1, key, data));
```

**Expected:** ✅ Isolated shards unreachable until healed

## Performance Testing

### Transfer Performance Targets

| Data Size | Target Latency | Test |
|-----------|---------------|------|
| 1KB | < 100ms | TransferPerformanceUnder100ms |
| 1MB | < 1s | (Scaled test) |
| 10MB | < 10s | (Scaled test) |

### Benchmark Test

```cpp
TEST_F(CrossShardDistributionTest, TransferPerformanceUnder100ms) {
    cluster->setLatency(0, 0);  // Disable simulation
    
    auto lora_data = createTestLoRAData(1024);  // 1KB
    
    auto start = std::chrono::high_resolution_clock::now();
    
    cluster->saveToShard(0, key, lora_data);
    auto loaded = cluster->loadFromShard(0, key);
    cluster->saveToShard(1, key, *loaded);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 100);
}
```

### Concurrent Transfers

```cpp
TEST_F(CrossShardDistributionTest, ConcurrentTransfers) {
    std::vector<std::thread> threads;
    
    for (int t = 0; t < 5; t++) {
        threads.emplace_back([this, t]() {
            auto data = createTestLoRAData(1024);
            std::string key = "concurrent-" + std::to_string(t);
            
            cluster->saveToShard(0, key, data);
            auto loaded = cluster->loadFromShard(0, key);
            cluster->saveToShard(1, key, *loaded);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}
```

## Statistics and Monitoring

### Available Metrics

```cpp
// Total operations
uint64_t total_writes = cluster->getTotalWrites();
uint64_t total_reads = cluster->getTotalReads();

// Bandwidth
uint64_t bytes_written = cluster->getTotalBytesWritten();
uint64_t bytes_read = cluster->getTotalBytesRead();

// Per-shard stats
auto shard_stats = cluster->getShardStats(shard_id);
std::cout << "Shard " << shard_id 
          << " writes: " << shard_stats->write_count
          << " reads: " << shard_stats->read_count;
```

### Health Monitoring

```cpp
// Check shard health
std::vector<int> healthy = cluster->getHealthyShards();
std::vector<int> failed = cluster->getFailedShards();

// Availability calculation
float availability = (float)healthy.size() / cluster->getShardCount();
```

## Integration with Production Systems

### Configuration Example

```cpp
// Production configuration
struct ShardConfig {
    std::string host;
    uint16_t port;
    int shard_id;
    bool use_tls;
};

std::vector<ShardConfig> shards = {
    {"shard0.example.com", 9000, 0, true},
    {"shard1.example.com", 9000, 1, true},
    {"shard2.example.com", 9000, 2, true}
};

// Initialize real shard cluster
RealShardCluster cluster(shards);
```

### Deployment Strategies

#### Strategy 1: Geographic Replication

```
Region A: Shard 0, 1
Region B: Shard 2, 3  (replicas)
Region C: Shard 4, 5  (replicas)

Latency: Cross-region ~100-200ms
Benefit: Disaster recovery
```

#### Strategy 2: Performance-Based

```
Hot Shards: SSD, high IOPS
Warm Shards: SSD, medium IOPS
Cold Shards: HDD, low IOPS

Adaptive placement based on access patterns
```

#### Strategy 3: Hierarchical

```
L1 Cache: In-memory (< 1ms)
L2 Cache: Local SSD (1-10ms)
L3 Storage: Remote shards (10-100ms)
```

## Troubleshooting

### Common Issues

#### 1. Timeouts

**Symptom:** Transfer operations timeout

**Debug:**
```cpp
cluster->setLatency(0, 0);  // Disable latency
// If still times out, check network/shard health
```

#### 2. Data Corruption

**Symptom:** Checksum mismatch after transfer

**Debug:**
```cpp
auto original_checksum = calculateChecksum(original_data);
auto transferred_checksum = calculateChecksum(transferred_data);

if (original_checksum != transferred_checksum) {
    // Check for packet loss or serialization bugs
}
```

#### 3. Inconsistent Replicas

**Symptom:** Different data on different shards

**Debug:**
```cpp
for (int shard = 0; shard < num_shards; shard++) {
    auto data = cluster->loadFromShard(shard, key);
    auto checksum = calculateChecksum(*data);
    std::cout << "Shard " << shard << ": " << checksum << std::endl;
}
```

## Best Practices

1. **Always Verify Transfers**
   - Use checksums for integrity
   - Compare byte-exact after transfer
   - Log transfer metrics

2. **Handle Failures Gracefully**
   - Implement retry logic
   - Use exponential backoff
   - Set reasonable timeouts

3. **Monitor Performance**
   - Track transfer latency
   - Monitor bandwidth usage
   - Alert on degradation

4. **Test Failure Scenarios**
   - Shard failures
   - Network partitions
   - Packet loss
   - Cascading failures

5. **Optimize for Scale**
   - Batch transfers when possible
   - Use compression for large adapters
   - Implement caching for hot adapters

## Running Cross-Shard Tests

```bash
# Run all cross-shard tests
ctest -R CrossShardDistributionTests -V

# Run specific test
./test_cross_shard_distribution \
    --gtest_filter="CrossShardDistributionTest.TransferLoRAAcrossShards"

# Run with custom config
THEMIS_TEST_NETWORK_LATENCY_MS=20 \
THEMIS_TEST_PACKET_LOSS_RATE=0.05 \
./test_cross_shard_distribution
```

## References

- Implementation: `tests/test_cross_shard_distribution.cpp`
- Mock Cluster: `tests/fixtures/mock_shard_cluster.cpp`
- Serialization: See `serializeLoRA()` and `deserializeLoRA()`
- RAID Integration: `docs/RAID_FAILURE_SCENARIOS.md`
