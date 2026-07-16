# Sprint 8 Phase 2: Batch A Type A Remediation Implementation

**Date:** 2026-07-05  
**Phase:** Type A Violation Remediation (Implicit Move Semantics)  
**Target Gaps:** 45-50 gaps across 6 priority modules  
**Status:** Ready for Implementation

---

## Module-by-Module Remediation Plan

### 1. LLM Module - Model Cache & Adapter (12 gaps)

#### Gap 1.1: ModelCacheEntry Move Semantics

**File:** `src/llm/model_cache.h`

**Issue:** ModelCacheEntry holds std::shared_ptr<Model> and metadata but lacks explicit move semantics.

**Remediation:**
```cpp
class ModelCacheEntry {
 private:
  std::shared_ptr<Model> model_;
  std::string model_id_;
  std::chrono::system_clock::time_point load_time_;
  size_t access_count_ = 0;

 public:
  // Added explicit move constructor
  ModelCacheEntry(ModelCacheEntry&& other) noexcept
      : model_(std::move(other.model_)),
        model_id_(std::move(other.model_id_)),
        load_time_(std::move(other.load_time_)),
        access_count_(other.access_count_) {
    other.access_count_ = 0;
  }

  // Added explicit move assignment
  ModelCacheEntry& operator=(ModelCacheEntry&& other) noexcept {
    if (this != &other) {
      model_ = std::move(other.model_);
      model_id_ = std::move(other.model_id_);
      load_time_ = std::move(other.load_time_);
      access_count_ = other.access_count_;
      other.access_count_ = 0;
    }
    return *this;
  }

  // Delete copy semantics
  ModelCacheEntry(const ModelCacheEntry&) = delete;
  ModelCacheEntry& operator=(const ModelCacheEntry&) = delete;
};
```

**Test:** `tests/llm/test_model_cache_move.cpp`
- Verify move construction
- Verify move assignment
- Verify access_count_ reset after move
- Verify model_ transferred to destination

#### Gap 1.2-1.5: LLMAdapterFactory Output Moves (4 gaps)

**File:** `src/llm/adapter_factory.cpp`

**Issue:** Methods returning std::unique_ptr<LLMAdapter> don't ensure source cleanup.

**Remediation Pattern:**
```cpp
std::unique_ptr<LLMAdapter> LLMAdapterFactory::createAdapter(const std::string& type) {
  std::unique_ptr<LLMAdapter> adapter;
  
  if (type == "openai") {
    adapter = std::make_unique<OpenAIAdapter>();
  } else if (type == "anthropic") {
    adapter = std::make_unique<AnthropicAdapter>();
  }
  
  // SafeMove for validation
  return THEMIS_SAFE_MOVE(std::unique_ptr<LLMAdapter>, std::move(adapter)).take();
}
```

#### Gap 1.3-1.12: Remaining LLM Adapter Moves (8+ gaps)

**Files:** 
- `src/llm/adapter_registry.cpp` (3 gaps)
- `src/llm/model_loader.cpp` (4 gaps)
- `src/llm/tokenizer_pool.cpp` (2 gaps)

**Pattern:** Add explicit move constructors/assignments to adapter storage containers and loader state objects.

---

### 2. Query Module - Query Plan & Iterator (10 gaps)

#### Gap 2.1: QueryPlanNode Move Semantics

**File:** `include/query/query_plan.h`

**Issue:** QueryPlanNode hierarchy lacks explicit move semantics for concrete implementations.

**Remediation:**
```cpp
class QueryPlanNode {
 protected:
  std::vector<std::unique_ptr<QueryPlanNode>> children_;
  std::string node_type_;

 public:
  QueryPlanNode(QueryPlanNode&& other) noexcept
      : children_(std::move(other.children_)),
        node_type_(std::move(other.node_type_)) {
    other.children_.clear();
  }

  QueryPlanNode& operator=(QueryPlanNode&& other) noexcept {
    if (this != &other) {
      children_ = std::move(other.children_);
      node_type_ = std::move(other.node_type_);
      other.children_.clear();
    }
    return *this;
  }

  QueryPlanNode(const QueryPlanNode&) = delete;
  QueryPlanNode& operator=(const QueryPlanNode&) = delete;
};
```

#### Gap 2.2-2.10: Concrete QueryPlanNode Implementations (9 gaps)

**Files:**
- `src/query/select_node.cpp` (2 gaps)
- `src/query/join_node.cpp` (3 gaps)
- `src/query/aggregate_node.cpp` (2 gaps)
- `src/query/filter_node.cpp` (2 gaps)

**Pattern:** Add move semantics to:
- SelectNode (columns_, predicates_ vectors)
- JoinNode (join_conditions_, join_type_)
- AggregateNode (group_expressions_, aggregate_functions_)
- FilterNode (filter_expressions_)

---

### 3. Tensor Module - Tensor Data & Metadata (8 gaps)

#### Gap 3.1: TensorData Move Semantics

**File:** `include/distributed_tensor/tensor_data.h`

**Issue:** TensorData holds large data buffer but compiler-generated move may cause issues.

**Remediation:**
```cpp
class TensorData {
 private:
  std::vector<float> data_;
  TensorShape shape_;
  DataType dtype_;

 public:
  TensorData(TensorData&& other) noexcept
      : data_(std::move(other.data_)),
        shape_(std::move(other.shape_)),
        dtype_(other.dtype_) {
    other.dtype_ = DataType::UNKNOWN;
  }

  TensorData& operator=(TensorData&& other) noexcept {
    if (this != &other) {
      data_ = std::move(other.data_);
      shape_ = std::move(other.shape_);
      dtype_ = other.dtype_;
      other.dtype_ = DataType::UNKNOWN;
    }
    return *this;
  }

  TensorData(const TensorData&) = delete;
  TensorData& operator=(const TensorData&) = delete;
};
```

#### Gap 3.2-3.8: Tensor Metadata & Sharding Moves (7 gaps)

**Files:**
- `src/distributed_tensor/tensor_metadata.cpp` (3 gaps)
- `src/distributed_tensor/shard_metadata.cpp` (2 gaps)
- `src/distributed_tensor/tensor_registry.cpp` (2 gaps)

**Pattern:** Add move semantics to metadata containers and registry storage.

---

### 4. Storage Module - RocksDB Wrapper & Index (7 gaps)

#### Gap 4.1: RocksDBHandle Move Semantics

**File:** `include/storage/rocksdb_wrapper.h`

**Issue:** RocksDBHandle wraps raw DB pointer without explicit move.

**Remediation:**
```cpp
class RocksDBHandle {
 private:
  rocksdb::DB* db_ = nullptr;
  std::string db_path_;
  std::unique_ptr<rocksdb::Options> options_;

 public:
  RocksDBHandle(RocksDBHandle&& other) noexcept
      : db_(other.db_),
        db_path_(std::move(other.db_path_)),
        options_(std::move(other.options_)) {
    other.db_ = nullptr;
  }

  RocksDBHandle& operator=(RocksDBHandle&& other) noexcept {
    if (this != &other) {
      if (db_ != nullptr) {
        delete db_;
      }
      db_ = other.db_;
      db_path_ = std::move(other.db_path_);
      options_ = std::move(other.options_);
      other.db_ = nullptr;
    }
    return *this;
  }

  RocksDBHandle(const RocksDBHandle&) = delete;
  RocksDBHandle& operator=(const RocksDBHandle&) = delete;

  ~RocksDBHandle() {
    if (db_ != nullptr) {
      delete db_;
    }
  }
};
```

#### Gap 4.2-4.7: Index & Cursor Moves (6 gaps)

**Files:**
- `src/storage/index_cursor.cpp` (3 gaps)
- `src/storage/index_builder.cpp` (2 gaps)
- `src/storage/column_store.cpp` (1 gap)

**Pattern:** Add explicit move semantics to cursor state and index builder output.

---

### 5. Acceleration Module - GPU Kernel Wrapper (6 gaps)

#### Gap 5.1: GPUKernelHandle Move Semantics

**File:** `include/acceleration/gpu_kernel_handle.h`

**Issue:** GPUKernelHandle holds GPU resource pointers without explicit move.

**Remediation:**
```cpp
class GPUKernelHandle {
 private:
  CUfunction kernel_ = nullptr;
  std::string kernel_name_;
  CUdevice device_ = 0;
  std::unique_ptr<char[]> ptx_code_;

 public:
  GPUKernelHandle(GPUKernelHandle&& other) noexcept
      : kernel_(other.kernel_),
        kernel_name_(std::move(other.kernel_name_)),
        device_(other.device_),
        ptx_code_(std::move(other.ptx_code_)) {
    other.kernel_ = nullptr;
    other.device_ = 0;
  }

  GPUKernelHandle& operator=(GPUKernelHandle&& other) noexcept {
    if (this != &other) {
      kernel_ = other.kernel_;
      kernel_name_ = std::move(other.kernel_name_);
      device_ = other.device_;
      ptx_code_ = std::move(other.ptx_code_);
      other.kernel_ = nullptr;
      other.device_ = 0;
    }
    return *this;
  }

  GPUKernelHandle(const GPUKernelHandle&) = delete;
  GPUKernelHandle& operator=(const GPUKernelHandle&) = delete;
};
```

#### Gap 5.2-5.6: GPU Batch & Memory Moves (5 gaps)

**Files:**
- `src/acceleration/gpu_batch_processor.cpp` (2 gaps)
- `src/acceleration/gpu_memory_pool.cpp` (2 gaps)
- `src/acceleration/gpu_stream.cpp` (1 gap)

---

### 6. Cache Module - Cache Entries & Eviction (5 gaps)

#### Gap 6.1: CacheEntry Move Semantics

**File:** `include/cache/cache_entry.h`

**Issue:** CacheEntry lacks explicit move constructors for value storage.

**Remediation:**
```cpp
template<typename K, typename V>
class CacheEntry {
 private:
  K key_;
  V value_;
  std::chrono::system_clock::time_point expiry_;
  size_t access_count_ = 0;

 public:
  CacheEntry(CacheEntry&& other) noexcept
      : key_(std::move(other.key_)),
        value_(std::move(other.value_)),
        expiry_(std::move(other.expiry_)),
        access_count_(other.access_count_) {
    other.access_count_ = 0;
  }

  CacheEntry& operator=(CacheEntry&& other) noexcept {
    if (this != &other) {
      key_ = std::move(other.key_);
      value_ = std::move(other.value_);
      expiry_ = std::move(other.expiry_);
      access_count_ = other.access_count_;
      other.access_count_ = 0;
    }
    return *this;
  }

  CacheEntry(const CacheEntry&) = delete;
  CacheEntry& operator=(const CacheEntry&) = delete;
};
```

#### Gap 6.2-6.5: Cache Manager & Eviction Moves (4 gaps)

**Files:**
- `src/cache/cache_manager.cpp` (2 gaps)
- `src/cache/lru_eviction_policy.cpp` (2 gaps)

---

## Implementation Checklist

### Batch A - Type A Remediation (45-50 gaps):

#### LLM Module (12 gaps)
- [ ] ModelCacheEntry::move constructor
- [ ] ModelCacheEntry::move assignment
- [ ] ModelCacheEntry delete copy
- [ ] LLMAdapterFactory moves (4 gaps)
- [ ] AdapterRegistry moves (3 gaps)
- [ ] ModelLoader moves (4 gaps)
- [ ] TokenizerPool moves (2 gaps)

#### Query Module (10 gaps)
- [ ] QueryPlanNode::move constructor
- [ ] QueryPlanNode::move assignment
- [ ] SelectNode moves (2 gaps)
- [ ] JoinNode moves (3 gaps)
- [ ] AggregateNode moves (2 gaps)
- [ ] FilterNode moves (2 gaps)

#### Tensor Module (8 gaps)
- [ ] TensorData::move constructor
- [ ] TensorData::move assignment
- [ ] TensorMetadata moves (3 gaps)
- [ ] ShardMetadata moves (2 gaps)
- [ ] TensorRegistry moves (2 gaps)

#### Storage Module (7 gaps)
- [ ] RocksDBHandle::move constructor
- [ ] RocksDBHandle::move assignment
- [ ] IndexCursor moves (3 gaps)
- [ ] IndexBuilder moves (2 gaps)
- [ ] ColumnStore moves (1 gap)

#### Acceleration Module (6 gaps)
- [ ] GPUKernelHandle::move constructor
- [ ] GPUKernelHandle::move assignment
- [ ] GPUBatchProcessor moves (2 gaps)
- [ ] GPUMemoryPool moves (2 gaps)
- [ ] GPUStream moves (1 gap)

#### Cache Module (5 gaps)
- [ ] CacheEntry::move constructor (template)
- [ ] CacheEntry::move assignment (template)
- [ ] CacheManager moves (2 gaps)
- [ ] LRUEvictionPolicy moves (2 gaps)

**Total:** 48 core remediations + 2-4 edge cases = 50-52 gaps

---

## Testing Strategy

### Per-Module Test Files:

1. `tests/llm/test_model_cache_move.cpp` - ModelCacheEntry, adapter moves
2. `tests/query/test_query_plan_move.cpp` - QueryPlanNode concrete implementations
3. `tests/distributed_tensor/test_tensor_data_move.cpp` - TensorData, metadata
4. `tests/storage/test_rocksdb_handle_move.cpp` - RocksDB wrapper, indices
5. `tests/acceleration/test_gpu_kernel_handle_move.cpp` - GPU resources
6. `tests/cache/test_cache_entry_move.cpp` - Cache entries, manager

### Test Template:

```cpp
TEST_F(ModuleNameTest, MoveConstruction_<ComponentName>) {
  auto original = createTestComponent();
  auto moved(std::move(original));
  
  EXPECT_TRUE(isMoveValid(moved));
  EXPECT_TRUE(isMovedFromValid(original));
}

TEST_F(ModuleNameTest, MoveAssignment_<ComponentName>) {
  auto original = createTestComponent();
  auto dest = createTestComponent();
  dest = std::move(original);
  
  EXPECT_TRUE(isMoveValid(dest));
  EXPECT_TRUE(isMovedFromValid(original));
}

TEST_F(ModuleNameTest, MoveChain_<ComponentName>) {
  auto a = createTestComponent();
  auto b = std::move(a);
  auto c = std::move(b);
  
  EXPECT_TRUE(isMoveValid(c));
  EXPECT_TRUE(isMovedFromValid(a));
  EXPECT_TRUE(isMovedFromValid(b));
}
```

---

## Validation Metrics

For each remediation:
1. **Code Completeness:** All members moved in constructor/assignment
2. **Source Safety:** Source object in valid state after move
3. **Copy Deletion:** Copy semantics properly deleted where needed
4. **Exception Safety:** All moves marked noexcept
5. **Backward Compatibility:** All existing tests pass
6. **New Tests:** 100% pass rate for move tests
7. **No Warnings:** Zero compiler warnings
8. **Performance:** Move faster than copy baseline

---

## Timeline & Effort Estimate

| Phase | Duration | Gaps | Est. Hours | Target Date |
|-------|----------|------|-----------|-------------|
| Phase 1 | Week 1 | 0-5 | 2-4 | 2026-07-05 ✅ |
| Phase 2A | Week 1-2 | 45-50 | 25-30 | 2026-07-18 |
| Phase 2B | Week 2-3 | 30-35 | 20-25 | 2026-07-25 |
| Phase 3 | Week 3 | 12-17 | 8-12 | 2026-07-26 |
| **Total** | 3 weeks | **97** | **65-80** | **2026-07-26** |

---

## Next Steps

1. ✅ Phase 1: SafeMove Library Complete
2. ⏳ Phase 2A: Type A Remediation (This document)
3. ⏳ Phase 2B: Type B/C Remediation
4. ⏳ Phase 3: Testing & Documentation
5. ⏳ Phase 4: Merge to develop

**Status:** Ready to proceed with Phase 2A implementation
