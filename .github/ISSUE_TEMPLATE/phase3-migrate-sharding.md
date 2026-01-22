---
name: Phase 3 - Migrate Sharding Module to Result<T>
about: Migrate sharding/distribution methods to Result<T>
title: '[Phase 3] Migrate Sharding Module to Result<T>'
labels: ['enhancement', 'error-handling', 'phase-3', 'sharding', 'distributed']
assignees: ''
---

## 📋 Overview

Migrate sharding and distribution logic from legacy error patterns to `Result<T>` for better error handling in distributed operations.

**Current Status:** 0% complete  
**Target:** ~15-20 methods  
**Priority:** 🟡 Medium (depends on distributed setup)

## 🎯 Goals

- Better error handling for shard operations
- Clear error messages for distribution failures
- Type-safe error propagation in distributed contexts

## 🔨 Scope Identification

### Step 1: Audit Sharding Module
```bash
# Find sharding-related files
find src/sharding include/sharding -name "*.cpp" -o -name "*.h"

# Identify error patterns
grep -r "return nullptr\|return false\|return std::nullopt" src/sharding/
```

### Step 2: Categorize Methods

**Shard Management:**
- [ ] `createShard()` - Create new shard
- [ ] `deleteShard()` - Remove shard
- [ ] `getShard()` - Retrieve shard
- [ ] `listShards()` - List all shards

**Data Distribution:**
- [ ] `distributeData()` - Distribute data to shards
- [ ] `rebalanceShards()` - Rebalance data across shards
- [ ] `moveData()` - Move data between shards

**Shard Routing:**
- [ ] `routeRequest()` - Route request to correct shard
- [ ] `getShardForKey()` - Determine shard for key
- [ ] `resolveShardLocation()` - Resolve shard location

**Consistency & Replication:**
- [ ] `ensureConsistency()` - Check/enforce consistency
- [ ] `replicateShard()` - Replicate shard data
- [ ] `syncShards()` - Synchronize shards

**Health & Monitoring:**
- [ ] `checkShardHealth()` - Health check
- [ ] `getShardStats()` - Retrieve statistics
- [ ] `detectShardFailure()` - Failure detection

## 📝 Implementation Strategy

### Error Codes to Add/Use
```cpp
// Sharding-specific error codes (may need to add)
ERR_SHARD_NOT_FOUND          // Shard doesn't exist
ERR_SHARD_UNAVAILABLE        // Shard temporarily unavailable
ERR_SHARD_REBALANCE_FAILED   // Rebalancing failed
ERR_DISTRIBUTION_FAILED      // Data distribution failed
ERR_SHARD_CONSISTENCY_ERROR  // Consistency check failed
ERR_NETWORK_ERROR            // Network communication failed
```

### Migration Pattern

**Before:**
```cpp
Shard* getShard(const std::string& shardId) {
    auto it = shards_.find(shardId);
    if (it == shards_.end()) {
        return nullptr;
    }
    return it->second.get();
}
```

**After:**
```cpp
Result<Shard*> getShard(const std::string& shardId) {
    auto it = shards_.find(shardId);
    if (it == shards_.end()) {
        return Err<Shard*>(
            ERR_SHARD_NOT_FOUND,
            fmt::format("Shard '{}' not found", shardId)
        );
    }
    
    if (!it->second->isAvailable()) {
        return Err<Shard*>(
            ERR_SHARD_UNAVAILABLE,
            fmt::format("Shard '{}' is unavailable", shardId)
        );
    }
    
    return Ok(it->second.get());
}
```

## 📋 Implementation Checklist

### Phase 1: Core Shard Operations
- [ ] Audit all shard management methods
- [ ] Identify error patterns
- [ ] Add necessary error codes
- [ ] Migrate createShard, deleteShard, getShard

### Phase 2: Distribution Logic
- [ ] Migrate data distribution methods
- [ ] Migrate rebalancing logic
- [ ] Update routing methods

### Phase 3: Consistency & Health
- [ ] Migrate consistency checks
- [ ] Migrate health monitoring
- [ ] Migrate failure detection

### Phase 4: Testing & Integration
- [ ] Update unit tests
- [ ] Add integration tests for distributed scenarios
- [ ] Test error propagation across network
- [ ] Performance testing

## 🧪 Testing Requirements

### Unit Tests
```cpp
TEST(ShardingTest, GetNonExistentShard) {
    auto result = shardMgr.getShard("shard-999");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ERR_SHARD_NOT_FOUND);
}

TEST(ShardingTest, DistributionFailure) {
    auto result = shardMgr.distributeData(data, "unavailable-shard");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ERR_SHARD_UNAVAILABLE);
}
```

### Integration Tests
- [ ] Test shard failover scenarios
- [ ] Test network partition handling
- [ ] Test rebalancing with errors
- [ ] Test consistency under failures

### Distributed Tests
- [ ] Multi-node error scenarios
- [ ] Network timeout handling
- [ ] Partial failure recovery

## 📚 Documentation Updates

- [ ] Update sharding documentation
- [ ] Document error scenarios
- [ ] Add troubleshooting guide
- [ ] Update operational runbooks

### Common Error Scenarios

1. **Shard Unavailable:**
   - Cause: Shard node down
   - Action: Retry or failover to replica

2. **Rebalance Failed:**
   - Cause: Insufficient resources
   - Action: Scale up or reduce load

3. **Consistency Error:**
   - Cause: Network partition
   - Action: Manual reconciliation

## 🎯 Success Criteria

- [ ] All sharding methods use `Result<T>`
- [ ] Error codes cover all failure modes
- [ ] Error messages are actionable
- [ ] Distributed tests pass
- [ ] Performance acceptable
- [ ] Documentation complete

## 📊 Progress Tracking

**Expected Effort:** 2-3 weeks  
**Priority:** Medium (depends on distributed deployment)

### Dependencies
- Requires distributed test environment
- May need coordination with ops team
- Network simulation for testing

## 🔗 Related

- **Parent Issue:** #XXX (Error Handling Migration - Master Tracking)
- **Documentation:** ERROR_HANDLING_MIGRATION_STATUS.md
- **Ops Documentation:** Sharding operational guide

## 💡 Notes

- **Distributed Complexity:** Sharding errors can be complex (network, partial failures)
- **Testing:** Requires distributed test environment
- **Operations:** Coordinate with ops team for production impact
- **Monitoring:** Update monitoring/alerting for new error codes
- **Gradual Rollout:** Consider gradual rollout in production
