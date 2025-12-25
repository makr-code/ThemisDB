# WriteBatch API Guide for ThemisDB

**Date:** December 25, 2024  
**Version:** 1.0  
**Target:** ThemisDB v1.4.0+  
**Source:** THEMISDB_OPTIMIZATION_IMPACT_ANALYSIS_SUMMARY.md

---

## 📋 Executive Summary

ThemisDB's WriteBatch API provides atomic multi-operation transactions with **+2-5× throughput** improvement over individual operations. This guide covers usage, best practices, and multi-agent LLM specific optimizations.

**Key Benefits:**
- ✅ Atomic commits (all-or-nothing guarantee)
- ✅ 2-5× throughput for bulk operations
- ✅ Reduced network round-trips
- ✅ Lower lock contention
- ✅ Better consistency for related entities

---

## 🎯 What is WriteBatch?

WriteBatch groups multiple write operations (PUT, DELETE) into a single atomic transaction that either fully succeeds or fully fails.

### Without WriteBatch (Sequential)
```
PUT entity1  → Round-trip 1
PUT entity2  → Round-trip 2
PUT entity3  → Round-trip 3
DELETE old   → Round-trip 4
Total: 4 network round-trips, no atomicity guarantee
```

### With WriteBatch (Atomic)
```
BATCH {
  PUT entity1
  PUT entity2
  PUT entity3
  DELETE old
}
Total: 1 network round-trip, atomic commit
```

---

## 📊 Performance Comparison

| Operation | Sequential | WriteBatch | Speedup |
|-----------|-----------|------------|---------|
| 10 PUTs | 50ms | 12ms | **4.2×** |
| 100 PUTs | 480ms | 95ms | **5.1×** |
| Mixed (50 PUT + 50 DELETE) | 510ms | 105ms | **4.9×** |

*Benchmarked on: 8-core CPU, local RocksDB, 1KB entities*

---

## 🚀 Quick Start

### C++ API

```cpp
#include "rocksdb_wrapper.h"

// Create batch
auto batch = db_->createWriteBatch();

// Add operations
batch->put("user:123", "{\"name\": \"Alice\"}");
batch->put("user:124", "{\"name\": \"Bob\"}");
batch->delete_key("user:old");

// Atomic commit
rocksdb::Status status = batch->commit();
if (!status.ok()) {
    // All operations rolled back
    THEMIS_ERROR("Batch commit failed: {}", status.ToString());
}
```

### HTTP API

```bash
curl -X POST http://localhost:8765/api/batch/write \
  -H "Content-Type: application/json" \
  -d '{
    "operations": [
      {
        "type": "put",
        "model": "documents",
        "collection": "articles",
        "uuid": "doc_001",
        "entity": {"title": "Article 1", "content": "..."}
      },
      {
        "type": "put",
        "model": "documents",
        "collection": "articles",
        "uuid": "doc_002",
        "entity": {"title": "Article 2", "content": "..."}
      },
      {
        "type": "delete",
        "model": "documents",
        "collection": "articles",
        "uuid": "doc_old"
      }
    ]
  }'
```

**Response:**
```json
{
  "success": true,
  "operations_count": 3,
  "commit_timestamp_ns": 1703520000000000000
}
```

---

## 🤖 Multi-Agent LLM Use Cases

### 1. Agent Task Results Batch

**Scenario:** Multiple agents complete subtasks; commit all results atomically.

```cpp
// Multi-agent orchestrator collecting results
auto batch = db_->createWriteBatch();

for (const auto& agent_result : agent_results) {
    std::string key = "task_result:" + agent_result.agent_id;
    batch->put(key, agent_result.toJson());
}

// Update task status atomically
batch->put("task:" + task_id + ":status", "completed");
batch->delete_key("task:" + task_id + ":in_progress");

if (!batch->commit().ok()) {
    // All agent results rolled back - retry orchestration
    retry_multi_agent_task();
}
```

**Benefits:**
- Atomic: Either all agent results are saved or none
- Performance: Single RocksDB commit instead of N+2 commits
- Consistency: Task status matches agent results

### 2. LoRA Adapter Batch Loading

**Scenario:** Load multiple LoRA adapters for different agent roles.

```cpp
// LoRARegistry bulk load
auto batch = db_->createWriteBatch();

for (const auto& adapter : adapters_to_load) {
    std::string key = "lora:" + adapter.name;
    std::string value = adapter.serialize();
    batch->put(key, value);
    
    // Update metadata
    std::string meta_key = "lora_meta:" + adapter.name;
    batch->put(meta_key, adapter.metadata.serialize());
}

// Update registry index atomically
batch->put("lora:index:version", std::to_string(new_version));

batch->commit();
```

### 3. Consensus Builder Result Storage

**Scenario:** Store multi-agent consensus with all supporting evidence.

```cpp
// ConsensusBuilder storing results
auto batch = db_->createWriteBatch();

// Store synthesized result
batch->put("consensus:" + task_id, consensus_result.toJson());

// Store individual agent responses
for (const auto& response : agent_responses) {
    std::string key = "agent_response:" + task_id + ":" + response.agent_id;
    batch->put(key, response.toJson());
}

// Store metadata (conflict scores, strategies used)
batch->put("consensus_meta:" + task_id, metadata.toJson());

// Atomic commit ensures consistency
batch->commit();
```

---

## 💡 Best Practices

### 1. Batch Size Optimization

**Small Batches (<10 operations):**
- Overhead of batching might not be worth it
- Consider individual operations

**Medium Batches (10-1000 operations):**
- **Sweet spot** - good performance gain
- Single RocksDB write batch

**Large Batches (>1000 operations):**
- Risk of memory pressure
- Consider splitting into multiple batches

```cpp
// Recommended: Split large batches
const size_t BATCH_SIZE = 500;

for (size_t i = 0; i < total_ops; i += BATCH_SIZE) {
    auto batch = db_->createWriteBatch();
    
    for (size_t j = i; j < std::min(i + BATCH_SIZE, total_ops); ++j) {
        batch->put(keys[j], values[j]);
    }
    
    batch->commit();
}
```

### 2. Error Handling

```cpp
auto batch = db_->createWriteBatch();

// Add operations (no errors thrown here)
batch->put("key1", "value1");
batch->put("key2", "value2");

// Commit can fail - check status
rocksdb::Status status = batch->commit();
if (!status.ok()) {
    if (status.IsCorruption()) {
        THEMIS_ERROR("Data corruption detected");
        // Critical error - alert ops team
    } else if (status.IsIOError()) {
        THEMIS_ERROR("I/O error - disk full?");
        // Retry with exponential backoff
    } else {
        THEMIS_ERROR("Batch commit failed: {}", status.ToString());
        // Generic error handling
    }
    
    // All operations automatically rolled back
    return false;
}

return true;
```

### 3. Idempotency

Ensure batch operations are idempotent for safe retries:

```cpp
// ❌ BAD: Non-idempotent counter increment
batch->put("counter", std::to_string(current_value + 1));

// ✅ GOOD: Set absolute value
batch->put("counter", std::to_string(new_absolute_value));

// ✅ GOOD: Use versioning for CAS
if (current_version == expected_version) {
    batch->put("entity", new_value);
    batch->put("entity:version", std::to_string(expected_version + 1));
}
```

### 4. Transaction Isolation

WriteBatch provides **atomic commit** but not **snapshot isolation** during construction:

```cpp
// ❌ RISK: Values might change between reads
auto batch = db_->createWriteBatch();
std::string val1 = db_->get("key1");  // Read at T1
std::string val2 = db_->get("key2");  // Read at T2 (might be inconsistent)
batch->put("derived", compute(val1, val2));
batch->commit();

// ✅ BETTER: Use MVCC snapshots for consistency
auto snapshot = db_->getSnapshot();
std::string val1 = db_->get("key1", snapshot);
std::string val2 = db_->get("key2", snapshot);
batch->put("derived", compute(val1, val2));
snapshot->release();
batch->commit();
```

---

## 📈 Performance Tuning

### 1. Write Buffer Size

```cpp
// Increase RocksDB write buffer for large batches
rocksdb::Options options;
options.write_buffer_size = 256 * 1024 * 1024;  // 256MB (default: 64MB)
options.max_write_buffer_number = 4;            // More buffers for parallelism
```

### 2. Disable WAL for Non-Critical Data

```cpp
// Faster commits but no durability guarantee on crash
rocksdb::WriteOptions write_opts;
write_opts.disableWAL = true;  // 2-3× faster, but risky

auto batch = db_->createWriteBatch();
batch->put("cache_key", "value");
batch->commit(write_opts);
```

**Use Cases for WAL-disabled:**
- Caches (can rebuild)
- Temporary data
- Derived data (can recompute)

**Never disable WAL for:**
- User data
- Financial transactions
- Critical system state

### 3. Parallel Batches

```cpp
// Process independent batches in parallel
std::vector<std::future<bool>> futures;

for (const auto& batch_data : independent_batches) {
    futures.push_back(std::async(std::launch::async, [&]() {
        auto batch = db_->createWriteBatch();
        // ... populate batch ...
        return batch->commit().ok();
    }));
}

// Wait for all batches
for (auto& future : futures) {
    if (!future.get()) {
        THEMIS_ERROR("One or more batches failed");
    }
}
```

---

## 🔍 Monitoring & Debugging

### Batch Metrics

```bash
# Check batch performance
curl http://localhost:8765/metrics | grep writebatch

# Example metrics:
themis_writebatch_operations_total{status="success"} 1250
themis_writebatch_operations_total{status="failed"} 3
themis_writebatch_size_bytes{quantile="0.5"} 4096
themis_writebatch_size_bytes{quantile="0.99"} 524288
themis_writebatch_commit_duration_seconds{quantile="0.99"} 0.012
```

### Debug Logging

```cpp
// Enable batch logging for debugging
#define THEMIS_LOG_WRITEBATCH

auto batch = db_->createWriteBatch();
batch->put("key1", "value1");  // Logged: "WriteBatch: PUT key1 (7 bytes)"
batch->delete_key("key2");      // Logged: "WriteBatch: DELETE key2"
auto status = batch->commit();  // Logged: "WriteBatch: COMMIT (2 ops, 12ms)"
```

---

## 🎓 Advanced Patterns

### Pattern 1: Batch with Conditional Logic

```cpp
auto batch = db_->createWriteBatch();

// Conditional operations based on current state
if (db_->exists("feature_flag:new_algorithm")) {
    batch->put("algorithm", "v2");
} else {
    batch->put("algorithm", "v1");
}

// Add related operations
batch->put("algorithm:updated_at", current_timestamp());
batch->commit();
```

### Pattern 2: Batch Rollback with Compensation

```cpp
// Save pre-batch state for manual rollback
std::unordered_map<std::string, std::string> backup;
for (const auto& key : keys_to_modify) {
    backup[key] = db_->get(key);
}

auto batch = db_->createWriteBatch();
// ... populate batch ...

if (!batch->commit().ok()) {
    // Manual compensation (WriteBatch already rolled back)
    THEMIS_WARN("Batch failed - restoring related state");
    
    // Restore dependent external state if needed
    restore_external_state(backup);
}
```

### Pattern 3: Two-Phase Commit Simulation

```cpp
// Phase 1: Prepare (write to staging)
auto prepare_batch = db_->createWriteBatch();
for (const auto& [key, value] : operations) {
    prepare_batch->put("staging:" + key, value);
}
prepare_batch->put("tx:" + tx_id + ":status", "prepared");
prepare_batch->commit();

// ... check all participants ready ...

// Phase 2: Commit (move from staging to final)
auto commit_batch = db_->createWriteBatch();
for (const auto& [key, value] : operations) {
    commit_batch->put(key, value);
    commit_batch->delete_key("staging:" + key);
}
commit_batch->put("tx:" + tx_id + ":status", "committed");
commit_batch->commit();
```

---

## 📊 Comparison with Alternatives

| Approach | Atomicity | Throughput | Complexity | Use Case |
|----------|-----------|------------|------------|----------|
| Individual Operations | ❌ No | 1× (baseline) | Simple | Single entity updates |
| WriteBatch | ✅ Yes | 2-5× | Medium | Bulk operations, related entities |
| RocksDB Transaction API | ✅ Yes + Isolation | 1.5-3× | High | Complex ACID requirements |
| External Transaction Coordinator | ✅ Yes (distributed) | 0.5-1× | Very High | Multi-database transactions |

**Recommendation:** Use WriteBatch for 90% of multi-operation use cases.

---

## 🐛 Common Pitfalls

### Pitfall 1: Forgetting to Commit

```cpp
// ❌ BAD: Batch goes out of scope without commit
{
    auto batch = db_->createWriteBatch();
    batch->put("key", "value");
}  // Operations lost!

// ✅ GOOD: Always commit
{
    auto batch = db_->createWriteBatch();
    batch->put("key", "value");
    batch->commit();  // Persisted
}
```

### Pitfall 2: Ignoring Commit Status

```cpp
// ❌ BAD: Ignoring errors
batch->commit();  // Might fail silently

// ✅ GOOD: Check status
if (!batch->commit().ok()) {
    handle_error();
}
```

### Pitfall 3: Mixing Batch with Individual Operations

```cpp
// ❌ BAD: Breaks atomicity
auto batch = db_->createWriteBatch();
batch->put("key1", "value1");
db_->put("key2", "value2");  // Not in batch!
batch->commit();  // Only key1 is atomic

// ✅ GOOD: All operations in batch
auto batch = db_->createWriteBatch();
batch->put("key1", "value1");
batch->put("key2", "value2");
batch->commit();  // Both atomic
```

---

## 📚 API Reference

### C++ API

```cpp
class WriteBatchWrapper {
public:
    // Add PUT operation
    void put(const std::string& key, const std::string& value);
    
    // Add DELETE operation
    void delete_key(const std::string& key);
    
    // Commit all operations atomically
    rocksdb::Status commit(const rocksdb::WriteOptions& options = {});
    
    // Get number of operations in batch
    size_t count() const;
    
    // Clear all operations (before commit)
    void clear();
};
```

### HTTP API

**Endpoint:** `POST /api/batch/write`

**Request:**
```json
{
  "operations": [
    {
      "type": "put",
      "model": "documents",
      "collection": "articles",
      "uuid": "doc_001",
      "entity": { "title": "...", "content": "..." }
    },
    {
      "type": "delete",
      "model": "documents",
      "collection": "articles",
      "uuid": "doc_old"
    }
  ],
  "write_options": {
    "sync": true,
    "disable_wal": false
  }
}
```

**Response:**
```json
{
  "success": true,
  "operations_count": 2,
  "commit_timestamp_ns": 1703520000000000000,
  "duration_ms": 12.4
}
```

---

## 🔗 Related Documentation

1. **RocksDB WriteBatch:** https://github.com/facebook/rocksdb/wiki/Basic-Operations#atomic-updates
2. **THEMISDB_OPTIMIZATION_IMPACT_ANALYSIS_SUMMARY.md**
3. **Multi-Agent API Guide:** `docs/llm/MULTI_AGENT_DEPLOYMENT_GUIDE.md`
4. **Transaction Semantics:** `docs/transactions/MVCC_GUIDE.md`

---

## ✅ Checklist for Production Use

- [ ] Batch size < 1000 operations
- [ ] Error handling implemented
- [ ] Operations are idempotent
- [ ] Monitoring/metrics enabled
- [ ] Performance benchmarks run
- [ ] Backup/recovery tested
- [ ] Load testing completed

---

**Remember:** WriteBatch provides atomicity but not snapshot isolation during construction. For complex multi-step transactions with reads, use RocksDB Transaction API or MVCC snapshots.
