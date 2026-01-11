---
title: "Implement RPC Service Full Implementation (Remove Stubs)"
labels: enhancement, rpc, distribution, sharding, priority-high, v1.3.1
milestone: v1.3.1
---

## 📋 Summary

The RPC service implementation in `src/server/rpc/rpc_service_impl.cpp` contains **extensive stub implementations** for core database operations. All CRUD operations, queries, transactions, and data operations return mock/empty responses instead of executing actual database operations.

**Type**: Feature Completion / Stub Removal  
**Priority**: HIGH (blocks distributed operations)  
**Effort**: 3-4 weeks (~1500 LOC)  
**Status**: ❌ Partially Implemented (11+ TODOs verified)

## 🔍 Verification

**Checked**: `src/server/rpc/rpc_service_impl.cpp`

**Evidence of Stubs** (sample from 11+ TODOs):
```cpp
// Line 27-28
Status Get(ServerContext* context, const GetRequest* request, GetResponse* response) override {
    // TODO(v1.3.1): Implement actual database GET operation
    // Issue: https://github.com/makr-code/ThemisDB/issues/XXX
    
// Line 72
Status Put(ServerContext* context, const PutRequest* request, PutResponse* response) override {
    // TODO: Implement actual database PUT operation
    
// Line 101  
Status Delete(ServerContext* context, const DeleteRequest* request, DeleteResponse* response) override {
    // TODO: Implement actual database DELETE operation

// Line 125
Status BatchGet(ServerContext* context, const BatchGetRequest* request, BatchGetResponse* response) override {
    // TODO: Implement actual batch GET operation

// Line 150
Status BatchPut(ServerContext* context, const BatchPutRequest* request, BatchPutResponse* response) override {
    // TODO: Implement actual batch PUT operation

// Line 177
Status ExecuteQuery(ServerContext* context, const QueryRequest* request, QueryResponse* response) override {
    // TODO: Implement actual AQL query execution

// Line 212
Status VectorSearch(ServerContext* context, const VectorSearchRequest* request, VectorSearchResponse* response) override {
    // TODO: Implement actual vector search

// Line 229
Status GraphTraversal(ServerContext* context, const GraphRequest* request, GraphResponse* response) override {
    // TODO: Implement graph traversal

// Line 256
Status GeoQuery(ServerContext* context, const GeoQueryRequest* request, GeoQueryResponse* response) override {
    // TODO: Implement geo query

// Line 282
Status TimeSeriesQuery(ServerContext* context, const TimeSeriesRequest* request, TimeSeriesResponse* response) override {
    // TODO: Implement time series query

// Line 299
Status BeginTransaction(ServerContext* context, const BeginTxRequest* request, BeginTxResponse* response) override {
    // TODO: Implement transaction begin

// Line 326
Status CommitTransaction(ServerContext* context, const CommitTxRequest* request, CommitTxResponse* response) override {
    // TODO: Implement transaction commit

// Line 352
Status AbortTransaction(ServerContext* context, const AbortTxRequest* request, AbortTxResponse* response) override {
    // TODO: Implement transaction abort
```

## 🎯 Problem Statement

The RPC service is **critical for distributed ThemisDB** (sharding, replication). Currently:

❌ **All operations are stubs** - no actual data is read or written  
❌ **Cannot support multi-shard operations** - distributed transactions blocked  
❌ **Cannot support replication** - no actual data transfer  
❌ **Cannot support remote clients** - all RPC calls return mock data

This **blocks**:
- Distributed/sharded deployments
- Multi-node replication
- Remote client connections via gRPC
- Cross-shard transactions

## 📚 Related Documentation

- **Sharding Documentation**: `docs/sharding/`
- **RPC Protocol**: `proto/themis_rpc.proto`
- **Gap Analysis**: `docs/de/development/GAPS_STUBS_SUMMARY.md` (mentions distributed transactions as HIGH priority)

From GAPS_STUBS_SUMMARY.md:
> **Distributed Transactions** - 2-3 Wochen - HOCH Priorität  
> RPC-Implementierung zu Shards, Snapshot-basierte Reads, 2PC vervollständigen

## 🏗️ Proposed Implementation

### Phase 1: Basic CRUD Operations (Week 1)

Connect to actual QueryEngine/Storage:

```cpp
class RpcServiceImpl {
private:
    QueryEngine* query_engine_;          // ← Add dependency
    StorageEngine* storage_engine_;      // ← Add dependency
    TransactionManager* tx_manager_;     // ← Add dependency
    
public:
    Status Get(ServerContext* context, const GetRequest* request, GetResponse* response) override {
        // Real implementation:
        auto result = storage_engine_->get(request->key());
        if (result.has_value()) {
            response->set_value(result.value());
            response->set_found(true);
        } else {
            response->set_found(false);
        }
        return Status::OK;
    }
};
```

### Phase 2: Query Execution (Week 2)

```cpp
Status ExecuteQuery(ServerContext* context, const QueryRequest* request, QueryResponse* response) override {
    // Parse AQL query
    auto query = request->aql_query();
    
    // Execute via QueryEngine
    auto result = query_engine_->executeAql(query);
    
    // Serialize results
    for (const auto& row : result.rows()) {
        auto* result_row = response->add_rows();
        // ... serialize row data
    }
    
    return Status::OK;
}
```

### Phase 3: Transactions (Week 3)

```cpp
Status BeginTransaction(ServerContext* context, const BeginTxRequest* request, BeginTxResponse* response) override {
    auto tx_id = tx_manager_->begin();
    response->set_transaction_id(tx_id);
    response->set_snapshot_timestamp(getCurrentTimestamp());
    return Status::OK;
}

Status CommitTransaction(ServerContext* context, const CommitTxRequest* request, CommitTxResponse* response) override {
    bool success = tx_manager_->commit(request->transaction_id());
    response->set_committed(success);
    return success ? Status::OK : Status(StatusCode::ABORTED, "Transaction conflict");
}
```

### Phase 4: Advanced Operations (Week 4)

- Vector search integration
- Graph traversal integration
- Geo query integration
- Time series integration

## 📝 Implementation Tasks

### Milestone 1: Infrastructure (Week 1)

- [ ] Add QueryEngine dependency to RpcServiceImpl
- [ ] Add StorageEngine dependency
- [ ] Add TransactionManager dependency
- [ ] Add proper error handling infrastructure
- [ ] Add authentication/authorization checks
- [ ] Add metrics/logging

### Milestone 2: Basic Operations (Week 1-2)

- [ ] Implement `Get()` with actual storage read
- [ ] Implement `Put()` with actual storage write
- [ ] Implement `Delete()` with actual storage delete
- [ ] Implement `BatchGet()` with batch reads
- [ ] Implement `BatchPut()` with batch writes
- [ ] Add unit tests for each operation

### Milestone 3: Query Execution (Week 2)

- [ ] Implement `ExecuteQuery()` with AQL execution
- [ ] Implement `VectorSearch()` with HNSW integration
- [ ] Implement `GraphTraversal()` with graph engine
- [ ] Implement `GeoQuery()` with spatial index
- [ ] Implement `TimeSeriesQuery()` with temporal index
- [ ] Add integration tests

### Milestone 4: Transactions (Week 3)

- [ ] Implement `BeginTransaction()`
- [ ] Implement `CommitTransaction()` with 2PC
- [ ] Implement `AbortTransaction()` with rollback
- [ ] Add distributed transaction tests

### Milestone 5: Testing & Polish (Week 4)

- [ ] End-to-end RPC tests
- [ ] Performance benchmarks
- [ ] Error handling validation
- [ ] Security/auth validation
- [ ] Documentation updates

## 🔗 Dependencies & Related Issues

### Depends On
- QueryEngine must be accessible from RPC layer
- TransactionManager for distributed TX

### Blocks
- Multi-shard operations
- Replication features
- Remote client support

### Related
- Issue #5: Distributed Transaction Completion
- `docs/de/development/GAPS_STUBS_SUMMARY.md`

## 📊 Success Criteria

### Functional Requirements
- ✅ All 13+ stub methods implemented with real logic
- ✅ All operations return actual data from database
- ✅ Transactions work across multiple operations
- ✅ Error handling for all edge cases
- ✅ Authentication/authorization enforced

### Technical Metrics
- ✅ RPC latency < 10ms for Get operations
- ✅ Throughput > 10k ops/sec for batch operations
- ✅ Transaction commit latency < 50ms
- ✅ Test coverage > 85%

### Quality Gates
- ✅ All unit tests passing
- ✅ All integration tests passing
- ✅ Performance benchmarks meet targets
- ✅ Code review approved
- ✅ No TODO comments remaining in RPC service

## 📅 Timeline Estimate

| Milestone | Duration | Deliverable |
|-----------|----------|-------------|
| Infrastructure | 3-4 days | Dependencies injected |
| Basic CRUD | 1 week | Get/Put/Delete working |
| Query Execution | 1 week | All query types working |
| Transactions | 1 week | TX begin/commit/abort |
| Testing & Polish | 3-4 days | Production ready |
| **Total** | **3-4 weeks** | **Feature complete** |

## ✅ Definition of Done

- [ ] All 13+ TODO comments removed from `rpc_service_impl.cpp`
- [ ] All methods execute actual database operations
- [ ] Unit tests for each RPC method
- [ ] Integration tests for end-to-end flows
- [ ] Performance benchmarks passing
- [ ] Error handling comprehensive
- [ ] Authentication/authorization implemented
- [ ] Metrics/logging added
- [ ] Code review approved
- [ ] Documentation updated

---

**Created**: 2026-01-11  
**Verified**: 2026-01-11 (13+ TODOs confirmed)  
**Target Version**: v1.3.1  
**Criticality**: HIGH (blocks distributed features)
