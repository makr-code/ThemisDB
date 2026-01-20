---
name: 🔌 RPC Service: Database Integration
about: Implement database operations for RPC service methods
title: "[RPC] Complete Database Integration (16 methods)"
labels: priority:P0, type:feature, area:api, area:storage, effort:x-large, production-blocker
assignees: ''
---

## 🔴 Production Blocker

**Current Status:** All methods return placeholder data  
**Priority:** P0 (Critical)  
**Effort:** 2-3 weeks  
**Target Version:** v1.3.1 (Already Planned)  
**Related Audit:** `NAMESPACE_IMPLEMENTATION_AUDIT_REPORT.md` Section 3.2.5

---

## 📋 Problem Description

The RPC service implementation has **16 TODO markers** indicating unimplemented database operations:

```cpp
// src/server/rpc/rpc_service_impl.cpp
// TODO(v1.3.1): Implement actual database GET operation
// TODO: Implement actual database PUT operation
// TODO: Implement actual database DELETE operation
// TODO: Implement actual search
// TODO: Implement stats
// TODO: Implement entity update with merge logic
// TODO: Implement index management operations
// TODO: Implement batch insert
// TODO: Implement batch update
// TODO: Implement paginated query
// TODO: Implement aggregation pipelines
```

**Risk:** **MEDIUM**  
- RPC clients receive placeholder data instead of actual database results
- Cannot be used in production
- API contract is met but functionality is missing

---

## 🎯 Requirements

### Must Have (P0) - v1.3.1

All 16 methods must be implemented with actual database operations:

#### CRUD Operations
- [ ] **`handleGet`** - Get entity by UUID
  - Query storage layer
  - Return actual entity data
  - Handle not found cases
  
- [ ] **`handlePut`** - Create/update entity
  - Write to storage layer
  - Generate version number
  - Return actual version
  
- [ ] **`handleDelete`** - Delete entity
  - Remove from storage layer
  - Handle cascading deletes
  - Return success/failure
  
- [ ] **`handleUpdate`** - Partial entity update
  - Merge logic for partial updates
  - Optimistic locking support
  - Return updated version

#### Query Operations
- [ ] **`handleSearch`** - Search entities
  - AQL query execution
  - Filter support
  - Return actual results
  
- [ ] **`handleQuery`** - Complex queries
  - Pagination support
  - Sorting support
  - Projection support
  
- [ ] **`handleAggregation`** - Aggregation pipelines
  - Group by operations
  - Sum, count, avg, min, max
  - Pipeline stages

#### Batch Operations
- [ ] **`handleBatchInsert`** - Insert multiple entities
  - Transaction support
  - All-or-nothing semantics
  - Return inserted IDs
  
- [ ] **`handleBatchUpdate`** - Update multiple entities
  - Bulk update support
  - Transaction support
  - Return update count
  
- [ ] **`handleBatchDelete`** - Delete multiple entities
  - Bulk delete support
  - Transaction support
  - Return delete count

#### Index Operations
- [ ] **`handleCreateIndex`** - Create index
  - Index type selection
  - Background creation
  - Return status
  
- [ ] **`handleDropIndex`** - Drop index
  - Verify index exists
  - Safe deletion
  - Return status
  
- [ ] **`handleListIndexes`** - List all indexes
  - Return index metadata
  - Include statistics

#### Statistics
- [ ] **`handleStats`** - Get statistics
  - Collection stats
  - Document counts
  - Index usage stats
  
- [ ] **`handleCollectionInfo`** - Collection metadata
  - Schema information
  - Actual data
  
- [ ] **`handleServerInfo`** - Server information
  - Real version info
  - Real metrics

---

## 🔧 Implementation Details

### Files to Modify

- `src/server/rpc/rpc_service_impl.cpp` - Implement all 16 methods
- `include/server/rpc_service_impl.h` - Add helper methods if needed

### Database Integration Points

```cpp
class ThemisRPCService {
private:
    // Add database dependencies
    std::shared_ptr<StorageEngine> storage_;
    std::shared_ptr<QueryEngine> query_engine_;
    std::shared_ptr<IndexManager> index_manager_;
    std::shared_ptr<TransactionManager> tx_manager_;
    
public:
    // Inject dependencies in constructor
    ThemisRPCService(
        std::shared_ptr<StorageEngine> storage,
        std::shared_ptr<QueryEngine> query,
        std::shared_ptr<IndexManager> index,
        std::shared_ptr<TransactionManager> tx
    );
};
```

### Example Implementation

```cpp
json ThemisRPCService::handleGet(const json& params) {
    try {
        std::string collection = params.value("collection", "");
        std::string uuid = params.value("uuid", "");
        
        // Validate input parameters
        if (collection.empty() || uuid.empty()) {
            return createError(
                RPCErrorCode::INVALID_PARAMETERS,
                "Missing required parameters: collection, uuid"
            );
        }
        
        // ✅ REPLACE: Actual database GET
        auto result = storage_->get(collection, uuid);
        if (!result) {
            return createError(
                RPCErrorCode::NOT_FOUND,
                "Entity not found"
            );
        }
        
        json response = {
            {"found", true},
            {"entity", *result},
            {"version", result->version()},
            {"timestamp_ns", result->timestamp()}
        };
        
        return createSuccess(response);
        
    } catch (const std::exception& e) {
        return createError(RPCErrorCode::INTERNAL_ERROR, e.what());
    }
}
```

---

## ✅ Acceptance Criteria

- [ ] All 16 methods interact with actual database
- [ ] **Zero placeholder data** returned
- [ ] **Zero TODO comments** remaining
- [ ] All CRUD operations work end-to-end
- [ ] Query operations return actual results
- [ ] Batch operations are transactional
- [ ] Index operations work correctly
- [ ] Statistics reflect actual data
- [ ] All tests pass
- [ ] Integration tests added for each method

---

## 🧪 Testing Requirements

### Unit Tests (per method)

Each of the 16 methods needs tests:

- [ ] Test successful operation
- [ ] Test error cases (not found, invalid input, etc.)
- [ ] Test with empty database
- [ ] Test with large datasets
- [ ] Test concurrency (multiple clients)

### Integration Tests

- [ ] End-to-end CRUD workflow
- [ ] Search with complex filters
- [ ] Batch operations with 1000+ entities
- [ ] Index creation and usage
- [ ] Transaction rollback scenarios
- [ ] Multi-client concurrent access

### Performance Tests

- [ ] Get operation < 10ms (p99)
- [ ] Search with 1M documents < 100ms (p99)
- [ ] Batch insert 1000 entities < 500ms
- [ ] Index creation completes in background

---

## 📚 References

- **RPC Service Interface:** `include/plugins/rpc_plugin_interface.h`
- **Current Implementation:** `src/server/rpc/rpc_service_impl.cpp` (lines 1-800)
- **Storage Layer:** `include/storage/` (RocksDB wrapper)
- **Query Engine:** `include/query/query_engine.h`
- **Related PR:** (v1.3.1 tracking issue)

---

## 📊 Success Metrics

- ✅ 0 TODO markers remaining
- ✅ 100% test coverage for all 16 methods
- ✅ All integration tests pass
- ✅ Performance benchmarks meet targets
- ✅ Production-ready for v1.3.1 release

---

## 🚨 Important Notes

- **Transaction Support:** All write operations should use transactions
- **Error Handling:** Use consistent error codes from `RPCErrorCode`
- **Logging:** Log all operations for debugging
- **Metrics:** Track operation latency and success/failure rates
- **Backward Compatibility:** Ensure API contract remains unchanged

---

## 📅 Implementation Plan

### Week 1: CRUD Operations (P0)
- [ ] Day 1-2: handleGet, handlePut
- [ ] Day 3-4: handleDelete, handleUpdate
- [ ] Day 5: Testing and bug fixes

### Week 2: Query and Batch Operations (P0)
- [ ] Day 1-2: handleSearch, handleQuery
- [ ] Day 3-4: handleBatchInsert, handleBatchUpdate, handleBatchDelete
- [ ] Day 5: Testing and integration

### Week 3: Index, Stats, and Polish (P0)
- [ ] Day 1-2: handleCreateIndex, handleDropIndex, handleListIndexes
- [ ] Day 3: handleStats, handleCollectionInfo, handleServerInfo
- [ ] Day 4-5: Integration tests, performance tests, documentation

---

**Created:** Based on Namespace Implementation Audit (2026-01-20)  
**Audit Section:** 3.2.5 RPC Service: Unvollständige Implementierung  
**Already Planned:** v1.3.1 per TODO comments in code
