# RPC Service Implementation Plan

**Version:** 1.0  
**Date:** 2026-01-12  
**Status:** Planning Phase  
**Estimated Duration:** 3-4 weeks  
**Estimated LOC:** ~1500 lines

## 📋 Overview

This document outlines the comprehensive plan to remove stub implementations from the RPC service and integrate it with actual database operations. The RPC service is critical for distributed ThemisDB deployments, enabling sharding, replication, and remote client connections.

## 🎯 Goals

1. **Remove all stub implementations** from `src/server/rpc/rpc_service_impl.cpp`
2. **Integrate with core components**: QueryEngine, StorageEngine, TransactionManager
3. **Enable distributed operations**: Multi-shard queries, distributed transactions
4. **Provide production-ready RPC service** with proper error handling, authentication, and metrics

## 🏗️ Architecture Overview

### Current State
- RPC service exists with stub implementations
- All methods return mock/placeholder data
- No actual database operations are performed
- 16 TODO comments identified in the codebase

### Target State
```
┌─────────────────────┐
│   RPC Service       │
│  (ThemisRPCService) │
└──────────┬──────────┘
           │
           ├──────────> QueryEngine (AQL execution, vector search, graph queries)
           │
           ├──────────> StorageEngine (RocksDBWrapper for CRUD operations)
           │
           ├──────────> TransactionManager (begin, commit, abort, 2PC)
           │
           ├──────────> SecondaryIndexManager (indexed queries)
           │
           ├──────────> VectorIndexManager (HNSW vector search)
           │
           └──────────> GraphIndexManager (graph traversal)
```

### Key Dependencies
- `include/query/query_engine.h` - Query execution interface
- `include/storage/rocksdb_wrapper.h` - Storage layer
- `include/transaction/transaction_manager.h` - Transaction management
- `include/index/secondary_index.h` - Secondary indexes
- `include/index/vector_index.h` - Vector indexes
- `include/index/graph_index.h` - Graph indexes

## 📊 Implementation Phases

### Phase 1: Infrastructure & Basic CRUD (Week 1)
**Duration:** 5-7 days  
**LOC:** ~400 lines  
**Priority:** P0

#### Tasks
1. **Add dependency injection to RpcServiceImpl**
   - Add QueryEngine pointer
   - Add StorageEngine (RocksDBWrapper) pointer
   - Add TransactionManager pointer
   - Update constructor and initialization

2. **Implement basic CRUD operations**
   - `handleGet()` - Read single entity from storage
   - `handlePut()` - Write single entity to storage
   - `handleDelete()` - Delete single entity from storage

3. **Implement batch operations**
   - `handleBatchGet()` - Read multiple entities
   - `handleBatchPut()` - Write multiple entities

4. **Add error handling infrastructure**
   - Convert storage errors to RPC error codes
   - Add proper exception handling
   - Implement retry logic for transient failures

5. **Add basic metrics and logging**
   - Request/response latency
   - Success/failure rates
   - Operation counts by type

#### Success Criteria
- ✅ All CRUD operations execute actual database operations
- ✅ Batch operations handle up to 1000 entities per request
- ✅ Error handling covers all edge cases
- ✅ Unit tests pass for all CRUD operations
- ✅ Performance: < 1ms latency for single-entity operations

#### Deliverable
- Issue template: `phase1-rpc-crud-implementation.md`

---

### Phase 2: Query Execution (Week 2)
**Duration:** 5-7 days  
**LOC:** ~500 lines  
**Priority:** P1

#### Tasks
1. **Implement AQL query execution**
   - `handleQuery()` - Execute AQL queries via QueryEngine
   - Parse query parameters (bindVars, options)
   - Stream large result sets
   - Handle query timeouts

2. **Implement vector search**
   - `handleVectorSearch()` - HNSW-based similarity search
   - Support for filtered vector search
   - Top-K and epsilon-based search
   - Vector index integration

3. **Implement graph traversal**
   - `handleGraphTraverse()` - Execute graph queries
   - Shortest path queries
   - Recursive path queries
   - BFS/DFS traversal

4. **Implement geospatial queries**
   - `handleGeoQuery()` - Spatial queries via GDAL
   - Point-in-polygon queries
   - Distance-based queries
   - Spatial index integration

5. **Implement time series queries**
   - `handleTimeSeriesQuery()` - Temporal queries
   - Window aggregations
   - Downsampling
   - Time-based filtering

#### Success Criteria
- ✅ AQL queries return correct results
- ✅ Vector search returns top-K similar vectors
- ✅ Graph traversal handles recursive queries
- ✅ Geo queries use spatial indexes
- ✅ Time series queries support aggregations
- ✅ Performance: < 10ms for indexed queries

#### Deliverable
- Issue template: `phase2-rpc-query-execution.md`

---

### Phase 3: Transaction Support (Week 3)
**Duration:** 5-7 days  
**LOC:** ~400 lines  
**Priority:** P0

#### Tasks
1. **Implement transaction lifecycle**
   - `handleTransactionBegin()` - Start new transaction
   - `handleTransactionCommit()` - Commit with 2PC
   - `handleTransactionAbort()` - Rollback changes

2. **Add transaction context management**
   - Transaction ID generation
   - Transaction state tracking
   - Timeout handling
   - Deadlock detection

3. **Implement distributed transaction support**
   - Two-phase commit (2PC) protocol
   - Snapshot isolation
   - Cross-shard transaction coordination
   - Transaction recovery on failure

4. **Add transaction metrics**
   - Transaction duration
   - Commit/abort rates
   - Conflict detection rate
   - Active transaction count

#### Success Criteria
- ✅ Transactions maintain ACID properties
- ✅ 2PC works across multiple shards
- ✅ Deadlock detection prevents infinite waits
- ✅ Transaction recovery handles crashes
- ✅ Performance: < 50ms commit latency

#### Deliverable
- Issue template: `phase3-rpc-transactions.md`

---

### Phase 4: Advanced Features & Polish (Week 4)
**Duration:** 3-5 days  
**LOC:** ~200 lines  
**Priority:** P2

#### Tasks
1. **Implement authentication/authorization**
   - Token validation
   - User permission checks
   - Role-based access control (RBAC)
   - Audit logging

2. **Add advanced error handling**
   - Retry policies for network failures
   - Circuit breaker pattern
   - Graceful degradation
   - Error categorization

3. **Optimize performance**
   - Connection pooling
   - Request batching
   - Response compression
   - Caching for read-heavy workloads

4. **Add comprehensive metrics**
   - Prometheus metrics export
   - Request tracing
   - Performance profiling
   - Resource utilization

5. **Documentation updates**
   - API documentation
   - Usage examples
   - Deployment guide
   - Troubleshooting guide

#### Success Criteria
- ✅ Authentication enforced on all endpoints
- ✅ Error handling covers 100% of edge cases
- ✅ Performance meets SLAs (< 10ms P99)
- ✅ Metrics available in Grafana
- ✅ Documentation complete and accurate

#### Deliverable
- Issue template: `phase4-rpc-advanced-features.md`

---

## 🧪 Testing Strategy

### Unit Tests
- Test each RPC method in isolation
- Mock dependencies (QueryEngine, StorageEngine, etc.)
- Cover all error paths
- Target: > 85% code coverage

### Integration Tests
- Test end-to-end RPC workflows
- Test with real database instances
- Test concurrent requests
- Test failure scenarios

### Performance Tests
- Benchmark single-entity operations (target: < 1ms)
- Benchmark batch operations (target: < 10ms for 100 entities)
- Benchmark query execution (target: < 10ms for indexed queries)
- Benchmark transactions (target: < 50ms commit)

### Stress Tests
- Test with high concurrency (1000+ concurrent connections)
- Test with large datasets (millions of entities)
- Test with network failures
- Test with resource constraints

## 📏 Success Metrics

### Functional Metrics
- ✅ 0 TODO comments remaining in RPC service
- ✅ All 13+ stub methods implemented
- ✅ All operations return actual data
- ✅ Transactions work across multiple operations

### Quality Metrics
- ✅ Code coverage > 85%
- ✅ All tests passing
- ✅ No memory leaks detected
- ✅ Static analysis (cppcheck) passing

### Performance Metrics
- ✅ Single-entity GET latency < 1ms (P50)
- ✅ Single-entity GET latency < 10ms (P99)
- ✅ Batch operation throughput > 10k ops/sec
- ✅ Query execution latency < 10ms (indexed)
- ✅ Transaction commit latency < 50ms

### Operational Metrics
- ✅ Prometheus metrics exported
- ✅ Error rate < 0.1%
- ✅ No server crashes under load
- ✅ Graceful degradation under resource constraints

## 🔗 Dependencies

### Blocked By
- None - all required components exist

### Blocks
- Distributed sharding implementation
- Multi-node replication
- Remote client SDK development
- Cross-shard transaction support

### Related
- Issue #5: Distributed Transaction Completion
- `docs/de/development/GAPS_STUBS_SUMMARY.md`
- `docs/sharding/` documentation

## 📅 Timeline

| Phase | Duration | Start | End | Deliverable |
|-------|----------|-------|-----|-------------|
| Phase 1: CRUD | 5-7 days | Week 1 Day 1 | Week 1 Day 7 | CRUD + Batch ops working |
| Phase 2: Queries | 5-7 days | Week 2 Day 1 | Week 2 Day 7 | All query types working |
| Phase 3: Transactions | 5-7 days | Week 3 Day 1 | Week 3 Day 7 | Distributed TX working |
| Phase 4: Polish | 3-5 days | Week 4 Day 1 | Week 4 Day 5 | Production ready |
| **Total** | **18-26 days** | | | **Feature complete** |

## ✅ Definition of Done

### Code Complete
- [ ] All 16 TODO comments removed
- [ ] All stub implementations replaced with real logic
- [ ] Code reviewed and approved
- [ ] No compiler warnings
- [ ] Static analysis passing

### Testing Complete
- [ ] All unit tests passing
- [ ] All integration tests passing
- [ ] Performance benchmarks meet targets
- [ ] Stress tests passing

### Documentation Complete
- [ ] API documentation updated
- [ ] Usage examples added
- [ ] Deployment guide updated
- [ ] Troubleshooting guide added

### Production Ready
- [ ] Metrics and logging in place
- [ ] Error handling comprehensive
- [ ] Authentication/authorization working
- [ ] Performance SLAs met
- [ ] Code deployed to staging

## 🚀 Getting Started

### Prerequisites
1. Familiarity with C++17
2. Understanding of RPC/gRPC concepts
3. Knowledge of ThemisDB architecture
4. Access to development environment

### Development Setup
```bash
# Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Checkout feature branch
git checkout -b feature/rpc-implementation

# Build project
./scripts/setup.sh
./scripts/build.sh

# Run tests
cd build
ctest --output-on-failure
```

### Code Review Process
1. Create feature branch from `develop`
2. Implement one phase at a time
3. Submit PR after each phase
4. Address code review feedback
5. Merge to `develop` after approval

## 📚 Resources

### Key Files
- `src/server/rpc/rpc_service_impl.cpp` - Implementation file
- `include/server/rpc_service_impl.h` - Header file
- `tests/integration/rpc/rpc_service_integration_test.cpp` - Integration tests

### Documentation
- [RPC Protocol](../../proto/themis_rpc.proto)
- [Query Engine](../../include/query/query_engine.h)
- [Transaction Manager](../../include/transaction/transaction_manager.h)
- [Sharding Documentation](../sharding/)

### Examples
- See `tests/test_query_engine.cpp` for QueryEngine usage
- See `tests/test_transaction_manager.cpp` for TransactionManager usage
- See `tests/test_shard_rpc_grpc.cpp` for gRPC integration

## 🤝 Contributing

This is a large feature implementation that requires human contributors per the project's CONTRIBUTING.md guidelines. AI tools should only be used in an assistive capacity for:
- Code review and suggestions
- Boilerplate generation
- Test case generation (with human verification)
- Documentation improvements

The bulk of the implementation should be written and understood by human contributors.

---

**Document Owner:** ThemisDB Core Team  
**Last Updated:** 2026-04-06  
**Next Review:** After Phase 1 completion
