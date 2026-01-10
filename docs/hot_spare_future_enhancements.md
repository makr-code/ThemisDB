# Hot Spare Management - Future Enhancements

This document captures observations and potential improvements for the Hot Spare Management system that can be addressed in future iterations.

## Minor Observations from Code Review

### 1. Total Bytes Estimation Assumes Uniform Document Sizes

**Location**: `src/sharding/hot_spare_manager.cpp`, line 796-797

**Current Implementation**:
```cpp
it->second.total_bytes = bytes_transferred + 
    (data->size() * (task.documents.size() - i - 1));
```

**Observation**:
The total bytes estimation assumes that all remaining documents are approximately the same size as the current document. This provides a reasonable approximation for progress tracking but may not be accurate if document sizes vary significantly.

**Potential Enhancement**:
- Calculate average document size from transferred documents
- Use document size metadata if available from storage layer
- Pre-scan document sizes before starting rebuild (with caching)

**Priority**: Low - Current approximation is acceptable for most use cases

**Impact**: Progress percentage and ETA calculations may be slightly inaccurate with highly variable document sizes

---

### 2. Hardcoded Replica Count in Document Recovery

**Location**: `src/sharding/hot_spare_manager.cpp`, line 754

**Current Implementation**:
```cpp
auto replicas = task.ring->getReplicaNodes(doc_id, 2);
```

**Observation**:
The number of replicas to query is hardcoded to 2. This works for typical configurations but doesn't adapt to the actual `replication_factor` in the `RedundancyConfig`.

**Potential Enhancement**:
- Pass `replication_factor` through `RebuildTask`
- Query `replication_factor - 1` replicas (excluding failed shard)
- Add configuration option for rebuild replica count

**Example**:
```cpp
// Future implementation
uint32_t replica_count = std::min(
    static_cast<uint32_t>(2), 
    config_.replication_factor - 1
);
auto replicas = task.ring->getReplicaNodes(doc_id, replica_count);
```

**Priority**: Medium - Should be addressed when supporting variable replication factors

**Impact**: May not utilize all available replicas in high-replication scenarios, but doesn't cause functional issues

---

### 3. RebuildTask Memory Usage with Handler Lambdas

**Location**: `include/sharding/hot_spare_manager.h`, lines 277-288

**Current Implementation**:
```cpp
struct RebuildTask {
    std::string spare_shard_id;
    std::string source_shard_id;
    std::vector<std::string> documents;
    uint64_t total_bytes;
    bool paused = false;
    
    // Handlers for data transfer
    ConsistentHashRing* ring;
    ReadHandler read_handler;  // std::function with captures
    WriteHandler write_handler; // std::function with captures
};
```

**Observation**:
The `RebuildTask` structure holds handlers by value. Since these are typically lambdas with captures, they may copy significant state. However, this is acceptable because:
- Tasks are short-lived (duration of rebuild)
- Queue typically has few entries (limited by `max_concurrent_rebuilds`)
- Handler copies are minimal with proper lambda design

**Potential Enhancement**:
- Use `std::shared_ptr` to handlers if memory becomes a concern
- Profile memory usage under high load
- Consider handler pooling for very large deployments

**Priority**: Very Low - Current design is appropriate for intended use cases

**Impact**: Minimal - typical handler captures are small (this pointer, shared_ptr references)

---

## Recommended Future Improvements

Based on the code review, here are additional enhancements for future consideration:

### 1. Document Transfer Retry Logic

**Current Behavior**: If a document fails to read from all replicas, it's skipped and the rebuild continues.

**Enhancement**: 
- Add configurable retry count for failed documents
- Implement exponential backoff between retries
- Option to fail entire rebuild if critical threshold is exceeded

**Configuration Example**:
```cpp
HotSpareConfig config;
config.rebuild_retry_count = 3;
config.rebuild_retry_backoff_ms = 100;
config.rebuild_max_failure_percentage = 0.01; // Fail if >1% docs fail
```

---

### 2. Configurable Timeouts for Read/Write Operations

**Current Behavior**: No explicit timeouts on handler operations.

**Enhancement**:
- Add timeout configuration for read and write handlers
- Implement timeout wrapper around handler calls
- Log timeout events separately from failures

**Configuration Example**:
```cpp
HotSpareConfig config;
config.rebuild_read_timeout = std::chrono::seconds(30);
config.rebuild_write_timeout = std::chrono::seconds(30);
```

---

### 3. Data Integrity Verification

**Current Behavior**: Documents are transferred without checksum verification.

**Enhancement**:
- Optional checksum calculation and verification
- Configurable verification mode (none, sampling, full)
- Report integrity metrics in rebuild status

**Configuration Example**:
```cpp
HotSpareConfig config;
config.rebuild_verify_integrity = true;
config.rebuild_verify_mode = VerifyMode::SAMPLING; // 10% of docs
config.rebuild_checksum_algorithm = ChecksumAlgo::XXH3;
```

---

## Implementation Notes

When addressing these enhancements:

1. **Backward Compatibility**: Ensure all changes maintain API compatibility
2. **Configuration Defaults**: Choose sensible defaults that work for most use cases
3. **Performance Impact**: Measure impact of each enhancement
4. **Documentation**: Update API docs and examples
5. **Testing**: Add specific tests for new functionality

---

## Version History

- **v1.4.0** (2026-01-05): Initial hot spare implementation
  - Core failover and rebuild functionality
  - Document created to track future enhancements

---

## References

- Main documentation: `docs/hot_spare_management.md`
- Implementation: `src/sharding/hot_spare_manager.cpp`
- Tests: `tests/test_hot_spare.cpp`
- Issue: [v1.4.0] Implement Hot Spare Management System
