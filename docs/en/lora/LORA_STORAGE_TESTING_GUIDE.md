# LoRA Storage Testing Guide

## Overview

This guide provides comprehensive instructions for testing ThemisDB's LoRA (Low-Rank Adaptation) storage system. The test suite covers storage operations, cross-shard distribution, RAID configurations, encryption, failure scenarios, and versioning.

## Test Suites

### 1. Basic Storage Operations (`test_lora_storage_integration`)

**Purpose:** Validates core storage functionality including save/load, metadata handling, and concurrent access.

**Test Coverage:**
- ✅ Save and load adapters
- ✅ Metadata serialization/deserialization
- ✅ Checksum validation
- ✅ Adapter deletion
- ✅ Listing adapters
- ✅ Metadata updates
- ✅ Versioning basics
- ✅ Storage statistics
- ✅ Concurrent saves/reads
- ✅ Edge cases (empty, large, special characters)

**Running Tests:**
```bash
cd build
ctest -R LoRAStorageIntegrationTests -V
```

### 2. Cross-Shard Distribution (`test_cross_shard_distribution`)

**Purpose:** Tests LoRA distribution across multiple shards with network simulation.

**Test Coverage:**
- ✅ LoRA serialization/deserialization
- ✅ Cross-shard transfer
- ✅ Network latency simulation
- ✅ Packet loss handling
- ✅ Shard failure detection
- ✅ Multi-shard replication
- ✅ Network partitions
- ✅ Performance benchmarks (<100ms for 1KB transfers)

**Running Tests:**
```bash
ctest -R CrossShardDistributionTests -V
```

**Key Features:**
- Mock shard cluster with 3 shards
- Configurable network latency (default: 1-5ms)
- Packet loss injection (0-100%)
- Network partition simulation

### 3. RAID Mode Integration (`test_raid_lora_integration`)

**Purpose:** Validates RAID configurations for LoRA storage redundancy.

**Test Coverage:**

#### RAID 0 (STRIPE)
- ✅ Data striping across shards
- ✅ Uniform distribution verification
- ✅ Single shard failure = data loss
- ✅ Zero failure tolerance

#### RAID 1 (MIRROR)
- ✅ 3-way replication
- ✅ Single/dual shard failure resilience
- ✅ All shards fail = data loss
- ✅ N-1 failure tolerance

#### RAID 5 (PARITY)
- ✅ Parity-based encoding
- ✅ Single shard recovery
- ✅ Dual failure = data loss
- ✅ XOR parity calculation
- ✅ One failure tolerance

#### RAID 10 (HYBRID)
- ✅ Striped mirrors
- ✅ Single failure per stripe group
- ✅ Multiple stripe group failures
- ✅ Mirror pair failure = data loss

**Running Tests:**
```bash
ctest -R RAIDLoRAIntegrationTests -V
```

**Configuration:**
- 6-shard cluster for RAID tests
- Configurable chunk sizes
- Failure injection support

### 4. Encryption Integration (`test_lora_encryption_integration`)

**Purpose:** Tests encrypted storage with multiple key providers.

**Test Coverage:**

#### HSM (Hardware Security Module)
- ✅ Configuration validation
- ✅ Library path handling
- ✅ Key rotation scenarios
- ✅ Session pool management

#### Vault Integration
- ✅ Vault server configuration
- ✅ Token-based authentication
- ✅ Key version tracking
- ✅ Mount path configuration

#### PKI (Public Key Infrastructure)
- ✅ Certificate-based encryption
- ✅ Public/private key pairs
- ✅ Certificate validation
- ✅ Expiration handling

**Running Tests:**
```bash
ctest -R LoRAEncryptionIntegrationTests -V
```

**Notes:**
- Tests work with mock configurations
- Actual HSM/Vault integration requires setup
- Performance tests included (10MB encryption < 5s)

### 5. Failure Scenarios (`test_lora_failure_scenarios`)

**Purpose:** Validates system behavior under various failure conditions.

**Test Coverage:**
- ✅ Corrupted metadata detection
- ✅ Replica recovery
- ✅ Incomplete write detection
- ✅ Mid-transfer interruption
- ✅ Transfer retry logic
- ✅ Disk space issues
- ✅ Old version cleanup
- ✅ Permission errors
- ✅ Read-only directories
- ✅ Concurrent write conflicts
- ✅ Read-during-write safety
- ✅ Network partition handling
- ✅ Transient failures
- ✅ Cascading failures
- ✅ Automatic failover

**Running Tests:**
```bash
ctest -R LoRAFailureScenariosTests -V
```

### 6. Versioning (`test_lora_versioning`)

**Purpose:** Tests LoRA adapter versioning and rollback functionality.

**Test Coverage:**
- ✅ Version creation
- ✅ Multiple version tracking
- ✅ Data change between versions
- ✅ Version listing
- ✅ Version ordering
- ✅ Rollback to earlier versions
- ✅ Rollback with metadata preservation
- ✅ Auto-cleanup of old versions
- ✅ Version limits (max_versions)
- ✅ Version isolation
- ✅ Independent adapter versions
- ✅ Concurrent version operations
- ✅ Versioning enable/disable

**Running Tests:**
```bash
ctest -R LoRAVersioningTests -V
```

**Configuration:**
- Default max_versions: 5
- Automatic cleanup of old versions
- Concurrent version creation support

## Running All LoRA Storage Tests

```bash
cd build
ctest -L lora -V
```

Or run all at once:
```bash
ctest -R "LoRAStorage|CrossShard|RAID|Encryption|Failure|Versioning" -V
```

## Test Configuration

### Environment Variables

```bash
# Enable LoRA tests
export THEMIS_ENABLE_LORA_TESTS=ON

# Test data directory
export THEMIS_TEST_DATA_DIR=/tmp/themis_test_data

# Network latency simulation
export THEMIS_TEST_NETWORK_LATENCY_MS=5

# Enable verbose output
export THEMIS_TEST_VERBOSE=1
```

### CMake Configuration

```bash
cmake -DTHEMIS_BUILD_TESTS=ON \
      -DTHEMIS_ENABLE_LORA_TESTS=ON \
      -DCMAKE_BUILD_TYPE=Debug \
      ..
```

## Test Infrastructure

### Mock Shard Cluster

The `MockShardCluster` provides a simulated multi-shard environment:

```cpp
MockShardCluster cluster(num_shards);
cluster.setLatency(min_ms, max_ms);
cluster.injectPacketLoss(loss_rate);
cluster.simulateNetworkPartition({isolated_shards});
```

### RAID Simulator

The `RAIDSimulator` provides RAID mode simulation:

```cpp
RAIDSimulator raid(RAIDMode::PARITY, 4);
auto chunks = raid.encodeWithParity(data);
auto reconstructed = raid.reconstruct(chunks);
```

### Shard Failure Injector

The `ShardFailureInjector` enables chaos testing:

```cpp
ShardFailureInjector injector;
injector.injectFailure(shard_id, FailureType::TRANSIENT, duration);
injector.injectCascadingFailures(initial_shard, count, delay);
```

## Performance Benchmarks

### Expected Performance Targets

| Operation | Size | Target Time | Test |
|-----------|------|-------------|------|
| Single-shard transfer | 1KB | < 100ms | CrossShard |
| Serialization | 5MB | < 100ms | CrossShard |
| Encryption | 10MB | < 5s | Encryption |
| RAID 5 reconstruction | 4MB | < 500ms | RAID |
| Version creation | N/A | < 50ms | Versioning |

### Running Performance Tests

```bash
# Run only performance-sensitive tests
ctest -L "performance" -V

# Run with timing information
ctest -R LoRA --output-on-failure --verbose
```

## Debugging Failed Tests

### Enable Verbose Logging

```bash
export SPDLOG_LEVEL=debug
ctest -R <test_name> -V
```

### Run Single Test

```bash
./test_lora_storage_integration --gtest_filter="LoRAStorageIntegrationTest.SaveLoadAdapter"
```

### Common Issues

1. **Test data not cleaned up**
   ```bash
   rm -rf /tmp/themis_lora_*
   ```

2. **Port conflicts (if using network tests)**
   ```bash
   netstat -tuln | grep 8080
   ```

3. **Permission errors**
   ```bash
   chmod 755 /tmp/themis_test_data
   ```

## CI/CD Integration

### GitHub Actions

```yaml
- name: Run LoRA Storage Tests
  run: |
    cd build
    ctest -L lora --output-on-failure
```

### Test Timeout

Default timeout: 600 seconds (10 minutes)
Adjust in `tests/CMakeLists.txt` if needed.

## Coverage Requirements

Target coverage for LoRA storage module: **≥90%**

Generate coverage report:
```bash
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make
make coverage
```

## Contributing

When adding new LoRA storage tests:

1. Follow existing test structure
2. Use descriptive test names
3. Add appropriate labels in CMakeLists.txt
4. Update this documentation
5. Ensure tests are deterministic
6. Clean up test artifacts in TearDown()

## Support

For issues or questions about LoRA storage tests:
- File an issue: https://github.com/makr-code/ThemisDB/issues
- Label: `tests`, `lora`, `storage`
