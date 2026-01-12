---
name: 🔌 RPC Phase 1 - Infrastructure & Basic CRUD Implementation
about: Implement basic CRUD operations and infrastructure for RPC service
title: "[RPC-P1] Implement Infrastructure & Basic CRUD Operations"
labels: ["type:feature", "priority:P0", "area:networking", "area:storage", "effort:large"]
assignees: []
---

## 📋 Summary

Implement the foundational infrastructure for the RPC service and basic CRUD operations (GET, PUT, DELETE, Batch GET, Batch PUT). This is Phase 1 of the RPC service implementation plan.

**Part of**: RPC Service Full Implementation  
**Phase**: 1 of 4  
**Duration**: 5-7 days  
**LOC**: ~400 lines  
**Priority**: P0 (Critical - Foundation for distributed operations)

## 🎯 Problem Statement

The RPC service currently contains only stub implementations that return mock data. This phase establishes the foundation by:
- Adding dependency injection for core components
- Implementing actual storage operations
- Adding proper error handling
- Establishing metrics and logging

## 🏗️ Implementation Tasks

### Task 1: Dependency Injection (Day 1)
- [ ] Add `QueryEngine*` pointer to `ThemisRPCService` class
- [ ] Add `RocksDBWrapper*` pointer for storage access
- [ ] Add `TransactionManager*` pointer for transaction support
- [ ] Update constructor to accept and initialize dependencies
- [ ] Update any initialization code to wire dependencies

**Files to modify:**
- `include/server/rpc_service_impl.h`
- `src/server/rpc/rpc_service_impl.cpp`

### Task 2: Implement GET Operation (Day 2)
- [ ] Remove TODO comment from `handleGet()`
- [ ] Implement actual storage read via RocksDBWrapper
- [ ] Convert storage keys from RPC parameters (model, collection, uuid)
- [ ] Deserialize entity from storage format to JSON
- [ ] Handle "not found" cases properly
- [ ] Add error handling for storage exceptions

**Success Criteria:**
- GET request returns actual data from storage
- Proper error responses for missing entities
- Latency < 1ms for single-entity GET

### Task 3: Implement PUT Operation (Day 2-3)
- [ ] Remove TODO comment from `handlePut()`
- [ ] Implement actual storage write via RocksDBWrapper
- [ ] Serialize entity from JSON to storage format
- [ ] Generate or validate entity version
- [ ] Handle storage write failures
- [ ] Add conflict detection for concurrent updates

**Success Criteria:**
- PUT request persists data to storage
- Data is retrievable via subsequent GET
- Proper error handling for conflicts

### Task 4: Implement DELETE Operation (Day 3)
- [ ] Remove TODO comment from `handleDelete()`
- [ ] Implement actual storage delete via RocksDBWrapper
- [ ] Handle "not found" cases for DELETE
- [ ] Consider soft delete vs hard delete
- [ ] Add audit logging for deletions

**Success Criteria:**
- DELETE request removes data from storage
- Subsequent GET returns "not found"
- Audit trail maintained

### Task 5: Implement Batch GET (Day 4)
- [ ] Remove TODO comment from `handleBatchGet()`
- [ ] Implement batch read operation
- [ ] Use RocksDBWrapper's multiGet if available
- [ ] Handle partial failures (some keys exist, some don't)
- [ ] Optimize for performance (parallel reads)
- [ ] Add pagination support for large result sets

**Success Criteria:**
- Batch GET returns multiple entities
- Handles up to 1000 keys per request
- Performance: < 10ms for 100 entities

### Task 6: Implement Batch PUT (Day 5)
- [ ] Remove TODO comment from `handleBatchPut()`
- [ ] Implement batch write operation
- [ ] Use WriteBatch for atomic multi-entity writes
- [ ] Handle partial failures with proper rollback
- [ ] Add validation for all entities before writing
- [ ] Return detailed results (which entities succeeded/failed)

**Success Criteria:**
- Batch PUT persists multiple entities atomically
- Handles up to 1000 entities per request
- All-or-nothing semantics on failure

### Task 7: Error Handling Infrastructure (Day 6)
- [ ] Create error mapping function: storage errors → RPC error codes
- [ ] Add exception handling wrappers for all operations
- [ ] Implement retry logic for transient failures
- [ ] Add timeout handling for long operations
- [ ] Create error response builder with details

**Error Categories to Handle:**
- Storage not available
- Key not found
- Serialization errors
- Permission denied
- Timeout exceeded
- Resource exhausted

### Task 8: Metrics and Logging (Day 7)
- [ ] Add latency metrics for each operation type
- [ ] Track success/failure rates
- [ ] Count operations by type (GET, PUT, DELETE, etc.)
- [ ] Log request/response at DEBUG level
- [ ] Log errors at ERROR level with context
- [ ] Add structured logging with request IDs

**Metrics to Track:**
- `rpc_requests_total` (counter by method)
- `rpc_request_duration_seconds` (histogram)
- `rpc_errors_total` (counter by error type)
- `rpc_active_connections` (gauge)

## 📝 Implementation Notes

### Storage Key Format
The RPC service needs to construct storage keys from RPC parameters:

```cpp
// Example key format: <model>:<collection>:<uuid>
std::string storage_key = model + ":" + collection + ":" + uuid;
```

### Entity Serialization
Entities from RPC are in JSON format, storage uses a specific format:

```cpp
// Serialize to storage format
std::string serializeEntity(const json& entity) {
    // Convert JSON to BaseEntity or storage format
    // May need to add metadata (_collection, _model, etc.)
}

// Deserialize from storage
json deserializeEntity(const std::string& data) {
    // Convert storage format back to JSON
    // Include metadata in response
}
```

### Error Conversion
Map storage errors to RPC error codes:

```cpp
RPCErrorCode mapStorageError(const std::exception& e) {
    // Map different exception types to appropriate RPC errors
    if (/* not found */) return RPCErrorCode::NOT_FOUND;
    if (/* permission */) return RPCErrorCode::PERMISSION_DENIED;
    return RPCErrorCode::INTERNAL_ERROR;
}
```

## 🧪 Testing

### Unit Tests
Create or update: `tests/unit/test_rpc_service_crud.cpp`

```cpp
TEST(RPCServiceTest, GetExistingEntity) {
    // Setup: Insert entity via storage
    // Execute: Call handleGet()
    // Verify: Returns correct entity
}

TEST(RPCServiceTest, GetNonExistentEntity) {
    // Execute: Call handleGet() for non-existent key
    // Verify: Returns NOT_FOUND error
}

TEST(RPCServiceTest, PutNewEntity) {
    // Execute: Call handlePut()
    // Verify: Entity persisted to storage
}

TEST(RPCServiceTest, BatchGetMultipleEntities) {
    // Setup: Insert 10 entities
    // Execute: Call handleBatchGet() for all 10
    // Verify: All 10 returned correctly
}

// Add 10+ more test cases covering error scenarios
```

### Integration Tests
Update: `tests/integration/rpc/rpc_service_integration_test.cpp`

- Test CRUD operations end-to-end
- Test with real RocksDB instance
- Test concurrent operations
- Test error scenarios (storage unavailable, etc.)

### Performance Tests
Create: `tests/performance/bench_rpc_crud.cpp`

- Benchmark single GET latency (target: < 1ms)
- Benchmark single PUT latency (target: < 1ms)
- Benchmark batch operations (target: < 10ms for 100 entities)

## ✅ Acceptance Criteria

- [ ] All TODO comments removed from CRUD methods
- [ ] All CRUD operations execute actual storage operations
- [ ] Batch operations handle up to 1000 entities
- [ ] Error handling comprehensive and tested
- [ ] Metrics exported for all operations
- [ ] Unit tests passing (> 85% coverage)
- [ ] Integration tests passing
- [ ] Performance benchmarks meet targets:
  - Single GET < 1ms (P50)
  - Single PUT < 1ms (P50)
  - Batch 100 entities < 10ms
- [ ] Code review approved
- [ ] Documentation updated

## 📚 Resources

### Key Files
- `src/server/rpc/rpc_service_impl.cpp` - Implementation
- `include/server/rpc_service_impl.h` - Header
- `include/storage/rocksdb_wrapper.h` - Storage interface
- `include/base_entity.h` - Entity structure

### Examples
- `src/query/query_engine.cpp` - How to use RocksDBWrapper
- `tests/test_transaction_manager.cpp` - Storage usage examples
- `tests/test_base_entity.cpp` - Entity serialization examples

### Documentation
- [Implementation Plan](../../docs/planning/rpc-implementation-plan.md)
- [RocksDB Wrapper Documentation](../../include/storage/rocksdb_wrapper.h)
- [Base Entity Principle](../../BASEENTITY_PRINCIPLE.md)

## 🔗 Related Issues

- **Depends On**: None (all dependencies exist)
- **Blocks**: Phase 2 (Query Execution), Phase 3 (Transactions)
- **Related**: Issue #5 (Distributed Transaction Completion)

## 🎯 Success Metrics

### Functional
- ✅ All 5 CRUD methods implemented
- ✅ 0 TODO comments in CRUD methods
- ✅ All operations use actual storage

### Quality
- ✅ Test coverage > 85%
- ✅ All tests passing
- ✅ No memory leaks
- ✅ Static analysis clean

### Performance
- ✅ GET latency < 1ms (P50), < 10ms (P99)
- ✅ PUT latency < 1ms (P50), < 10ms (P99)
- ✅ Batch 100 entities < 10ms
- ✅ Throughput > 10k ops/sec

## 📅 Timeline

| Day | Tasks | Deliverable |
|-----|-------|-------------|
| 1 | Dependency injection | Dependencies wired |
| 2 | GET + PUT implementation | Basic read/write working |
| 3 | DELETE + Batch GET start | Single ops complete |
| 4 | Batch GET completion | Batch read working |
| 5 | Batch PUT implementation | Batch write working |
| 6 | Error handling | Robust error handling |
| 7 | Metrics + Testing | Phase 1 complete |

**Total**: 5-7 days

## 🚀 Next Steps

After Phase 1 completion:
1. Submit PR for code review
2. Address review feedback
3. Merge to `develop`
4. Start Phase 2: Query Execution

---

**Created**: 2026-01-12  
**Phase**: 1/4  
**Blocks**: RPC Service Distributed Operations
