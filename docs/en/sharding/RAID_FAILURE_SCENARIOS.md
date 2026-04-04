# RAID Failure Scenarios Documentation

## Overview

This document describes the RAID failure scenarios tested in ThemisDB's LoRA storage system. Understanding these scenarios is critical for deploying reliable LoRA adapter storage in production.

## RAID Modes Overview

| Mode | Description | Failure Tolerance | Use Case |
|------|-------------|-------------------|----------|
| RAID 0 | Striping | 0 failures | Maximum performance, no redundancy |
| RAID 1 | Mirroring | N-1 failures | Maximum redundancy |
| RAID 5 | Parity | 1 failure | Balanced redundancy + space |
| RAID 10 | Stripe+Mirror | N/2 failures | High performance + redundancy |

## RAID 0 (STRIPE) Failure Scenarios

### Configuration
- Minimum shards: 2
- Data distribution: Round-robin striping
- Chunk size: Configurable (default: 4KB)

### Failure Behavior

#### Scenario 1: Single Shard Failure
**Result:** ❌ **TOTAL DATA LOSS**

```
Shards: [0] [1] [2] [3]
Data:   [A] [B] [C] [D]

Shard 1 fails: [0] [X] [2] [3]
                [A] [?] [C] [D]

Result: Cannot reconstruct - missing chunk B
```

**Test:** `RAID0_SingleShardFailureLosesData`

**Mitigation:** 
- Use RAID 1/5/10 for critical data
- Implement application-level backups
- Monitor shard health proactively

### Performance Characteristics
- Read throughput: N × single shard throughput
- Write throughput: N × single shard throughput
- Latency: Single shard latency

## RAID 1 (MIRROR) Failure Scenarios

### Configuration
- Minimum shards: 2
- Replication factor: Configurable (default: 3)
- Synchronous replication

### Failure Behavior

#### Scenario 1: Single Shard Failure
**Result:** ✅ **DATA AVAILABLE**

```
Shards: [0] [1] [2]
Data:   [A] [A] [A]  (replicated)

Shard 0 fails: [X] [1] [2]
                [?] [A] [A]

Result: Read from shard 1 or 2
```

**Test:** `RAID1_SingleShardFailureStillReadable`

#### Scenario 2: Two Shard Failures
**Result:** ✅ **DATA AVAILABLE** (with RF=3)

```
Shards: [0] [1] [2]
Data:   [A] [A] [A]

Shards 0,1 fail: [X] [X] [2]
                  [?] [?] [A]

Result: Read from shard 2
```

**Test:** `RAID1_TwoShardFailuresStillReadable`

#### Scenario 3: All Shards Fail
**Result:** ❌ **TOTAL DATA LOSS**

```
All shards fail: [X] [X] [X]
                  [?] [?] [?]

Result: No replicas available
```

**Test:** `RAID1_AllShardsFailDataLost`

### Recovery Strategy

1. **Automatic Failover**
   - Reads automatically route to healthy replicas
   - No manual intervention required
   - Sub-second failover time

2. **Shard Recovery**
   ```
   Step 1: Detect failed shard
   Step 2: Provision replacement shard
   Step 3: Copy data from healthy replica
   Step 4: Verify data integrity
   Step 5: Add to active pool
   ```

3. **Quorum Requirements**
   - Read quorum: 1 (any replica)
   - Write quorum: Majority (RF/2 + 1)

## RAID 5 (PARITY) Failure Scenarios

### Configuration
- Minimum shards: 3
- Data shards: N-1
- Parity shards: 1
- Parity algorithm: XOR

### Failure Behavior

#### Scenario 1: Single Shard Failure
**Result:** ✅ **DATA RECOVERABLE**

```
Shards: [0] [1] [2] [3]
Data:   [A] [B] [C] [P]  (P = A⊕B⊕C)

Shard 1 fails: [0] [X] [2] [3]
                [A] [?] [C] [P]

Recovery: B = A⊕C⊕P
Result: Full data reconstructed
```

**Test:** `RAID5_RecoverFromSingleFailure`

**Recovery Time:** O(data_size / bandwidth)

#### Scenario 2: Two Shard Failures
**Result:** ❌ **DATA LOSS**

```
Shards: [0] [1] [2] [3]
Data:   [A] [B] [C] [P]

Shards 1,2 fail: [0] [X] [X] [3]
                  [A] [?] [?] [P]

Equations:
  P = A⊕B⊕C
  
But we have: A, P (known), B, C (unknown)
Cannot solve: 1 equation, 2 unknowns

Result: Insufficient data for recovery
```

**Test:** `RAID5_CannotRecoverFromTwoFailures`

### Parity Calculation

```cpp
// XOR-based parity
uint8_t calculateParity(std::vector<uint8_t> chunks[]) {
    uint8_t parity = 0;
    for (auto& chunk : chunks) {
        for (size_t i = 0; i < chunk.size(); i++) {
            parity[i] ^= chunk[i];
        }
    }
    return parity;
}
```

### Recovery Algorithm

```cpp
// Reconstruct missing chunk
std::vector<uint8_t> reconstructChunk(
    int missing_index,
    std::vector<std::optional<std::vector<uint8_t>>>& chunks) {
    
    std::vector<uint8_t> reconstructed(chunk_size, 0);
    
    for (size_t i = 0; i < chunks.size(); i++) {
        if (i != missing_index && chunks[i].has_value()) {
            for (size_t j = 0; j < chunk_size; j++) {
                reconstructed[j] ^= (*chunks[i])[j];
            }
        }
    }
    
    return reconstructed;
}
```

## RAID 10 (HYBRID) Failure Scenarios

### Configuration
- Minimum shards: 4
- Stripe groups: N/2
- Mirrors per stripe: 2
- Combined striping + mirroring

### Architecture

```
Stripe Group 0:  [Shard 0] [Shard 1] (mirrored)
                    ↓          ↓
                 [Data A] [Data A]

Stripe Group 1:  [Shard 2] [Shard 3] (mirrored)
                    ↓          ↓
                 [Data B] [Data B]
```

### Failure Behavior

#### Scenario 1: Single Failure Per Stripe Group
**Result:** ✅ **DATA AVAILABLE**

```
Before: [0:A] [1:A] [2:B] [3:B]

Shard 0 fails: [X:?] [1:A] [2:B] [3:B]

Read A from shard 1 ✓
Read B from shard 2 or 3 ✓
```

**Test:** `RAID10_SingleFailureInStripeGroup`

#### Scenario 2: Multiple Failures Across Groups
**Result:** ✅ **DATA AVAILABLE**

```
Before: [0:A] [1:A] [2:B] [3:B]

Shards 0,2 fail: [X:?] [1:A] [X:?] [3:B]

Read A from shard 1 ✓
Read B from shard 3 ✓
```

**Test:** `RAID10_MultipleFailuresAcrossStripeGroups`

#### Scenario 3: Both Mirrors in Stripe Fail
**Result:** ❌ **PARTIAL DATA LOSS**

```
Before: [0:A] [1:A] [2:B] [3:B]

Shards 0,1 fail: [X:?] [X:?] [2:B] [3:B]

Cannot read A ✗
Can read B ✓

Result: Lost data from stripe group 0
```

**Test:** `RAID10_BothMirrorsFailDataLost`

## Failure Detection and Monitoring

### Health Checks

```cpp
class ShardHealthMonitor {
public:
    // Check shard responsiveness
    bool checkShard(int shard_id) {
        auto start = now();
        bool responsive = sendHeartbeat(shard_id);
        auto latency = now() - start;
        
        return responsive && latency < threshold;
    }
    
    // Monitor failure rates
    void recordFailure(int shard_id) {
        failure_count_[shard_id]++;
        if (failure_count_[shard_id] > threshold) {
            markAsFailed(shard_id);
            triggerFailover(shard_id);
        }
    }
};
```

### Automatic Recovery

```cpp
class AutoRecoveryManager {
public:
    void handleShardFailure(int failed_shard) {
        // 1. Identify RAID mode and affected data
        auto raid_mode = getRaidMode();
        auto affected_adapters = getAffectedAdapters(failed_shard);
        
        // 2. Check if recovery is possible
        if (!canRecover(raid_mode, failed_shard)) {
            alertDataLoss(affected_adapters);
            return;
        }
        
        // 3. Initiate recovery
        for (auto& adapter : affected_adapters) {
            recoverAdapter(adapter, failed_shard);
        }
        
        // 4. Verify recovered data
        verifyIntegrity(affected_adapters);
    }
};
```

## Cascading Failures

### Definition
A cascading failure occurs when one shard failure triggers additional failures in the system.

### Common Causes
1. **Load redistribution**
   - Failed shard's load distributed to others
   - Remaining shards become overloaded
   - Additional shards fail from overload

2. **Network congestion**
   - Recovery traffic saturates network
   - Timeouts on healthy shards
   - False-positive failure detection

3. **Resource exhaustion**
   - Reconstruction uses CPU/memory
   - Normal operations starved
   - Performance degradation → failures

### Prevention Strategies

```cpp
class CascadePreventionManager {
public:
    // Rate limit recovery operations
    void throttleRecovery() {
        auto bandwidth_limit = getAvailableBandwidth() * 0.5;
        setRecoveryBandwidth(bandwidth_limit);
    }
    
    // Staged recovery
    void stageRecovery(std::vector<int> failed_shards) {
        for (auto shard : failed_shards) {
            recoverShard(shard);
            waitForStabilization();
        }
    }
    
    // Circuit breaker
    void checkSystemHealth() {
        auto healthy_ratio = getHealthyShards() / getTotalShards();
        if (healthy_ratio < 0.5) {
            pauseRecovery();
            alertOperators();
        }
    }
};
```

### Test Coverage

**Test:** `CascadingFailureDetection`
```cpp
// Inject cascading failures
injector.injectCascadingFailures(
    initial_shard: 0,
    cascade_count: 2,
    cascade_delay: 100ms
);
```

## Production Recommendations

### RAID Mode Selection

| Workload | Recommended RAID | Reason |
|----------|------------------|---------|
| Temporary adapters | RAID 0 | Performance, no durability needed |
| Production adapters | RAID 1 (RF=3) | High availability |
| Cost-sensitive | RAID 5 | Balance of redundancy & space |
| High-performance + HA | RAID 10 | Best of both worlds |

### Monitoring Thresholds

```yaml
alerts:
  shard_failure:
    threshold: 1
    window: 5m
    action: page_oncall
    
  multiple_failures:
    threshold: 2
    window: 10m
    action: page_oncall_urgent
    
  slow_shard:
    latency_p99: 100ms
    window: 5m
    action: investigate
```

### Recovery SLAs

| Scenario | Target MTTR | Priority |
|----------|-------------|----------|
| RAID 1 single failure | < 1 hour | P2 |
| RAID 5 single failure | < 2 hours | P1 |
| Multiple failures | < 30 minutes | P0 |

## References

- Test Implementation: `tests/test_raid_lora_integration.cpp`
- RAID Simulator: `tests/utils/raid_simulator.cpp`
- Failure Injector: `tests/utils/shard_failure_injector.cpp`
