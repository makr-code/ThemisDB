# Phase 4: Complete Full Error Handling Migration - Matrix

**Date:** 2026-01-20 (Updated)  
**Status:** 🟡 IN PROGRESS  
**Dependencies:** ✅ Phase 3 Complete (IndexManager, ContentFS migrated)

---

## 🎯 Objective

Complete migration of all remaining error sites from legacy error patterns to unified `tl::expected`-based error handling system. This document provides a detailed inventory and migration strategy for **~181 `return nullptr` sites**, **~916 `std::optional` usages**, and **~19+ Status struct definitions** that need standardization.

---

## 📊 Executive Summary

### Current State (Post-Phase 3) - UPDATED INVENTORY

| Metric | Count | Notes |
|--------|-------|-------|
| **Completed Migrations** | IndexManager, ContentFS, 1 storage function | Phase 3 ✅ + Phase 4 started |
| **Remaining nullptr Sites** | **181** | Significantly more than initially estimated (was 91) |
| **std::optional Usage** | **916** | Not previously counted - many for error handling |
| **Status Struct Definitions** | **19+** | Need removal/consolidation |
| **Error Codes Defined** | 31+ | Storage, LLM, LoRA, MCP, Schema, Network, Index, Query, API, Plugin |
| **Infrastructure** | Complete | `include/utils/expected.h` ready |

### Phase 4 Scope - REVISED

- **Total Files to Update:** ~200+ files (revised from 104)
- **Effort Estimate:** 14-16 weeks (revised from 10-12 weeks)
- **Priority Modules:** Query Engine (P0), LLM/LoRA (P0), Storage (P1)
- **Complexity:** VERY HIGH - scope significantly larger than initially estimated

---

## 🗺️ Migration Matrix

### Priority 0 (Critical - Weeks 1-5)

#### 1. Query Engine Migration

| Attribute | Details |
|-----------|---------|
| **Legacy Sites** | 28 return nullptr, 237 Status/Result returns |
| **Priority** | **P0 - CRITICAL** |
| **Complexity** | **VERY HIGH** |
| **Estimated Effort** | 3 weeks |
| **Impact** | User-facing query execution; high visibility |

**Files Affected:**
- `src/query/aql_parser.cpp` - AQL parsing
- `src/query/aql_translator.cpp` - 96 Status returns
- `src/query/query_engine.cpp` - 5 nullptr + 67 Status returns
- `src/query/cte_subquery.cpp` - 8 nullptr + 32 Status returns
- `src/query/statistical_aggregator.cpp` - 10 nullptr
- `src/query/semantic_cache.cpp` - 15 Status returns
- `src/query/expression_evaluator.cpp`
- `src/query/join_*.cpp`

**Error Codes Available:**
- `ERR_QUERY_PARSE_FAILED` (6100)
- `ERR_QUERY_INVALID_SYNTAX` (6101)
- `ERR_QUERY_EXECUTION_FAILED` (6102)
- `ERR_QUERY_TIMEOUT` (6103)

**Error Codes to Add:**
- `ERR_QUERY_CTE_CYCLE_DETECTED` (6104)
- `ERR_QUERY_AGGREGATION_FAILED` (6105)
- `ERR_QUERY_CACHE_ERROR` (6106)

**Migration Pattern:**
```cpp
// Before: return nullptr
ASTNode* parseExpression() {
    if (!valid) return nullptr;
    return node;
}

// After: Result<ASTNode*>
Result<ASTNode*> parseExpression() {
    if (!valid) {
        return Err<ASTNode*>(ERR_QUERY_PARSE_FAILED, 
            fmt::format("Invalid expression at line {}", line));
    }
    return Ok(node);
}
```

**Testing Requirements:**
- [ ] Update ~12 test files
- [ ] Add parse error tests with line/column info
- [ ] Add CTE cycle detection tests
- [ ] Add aggregation failure tests
- [ ] Add timeout simulation tests
- [ ] Performance benchmarks (< 5% regression)

---

#### 2. LLM/LoRA Modules Migration

| Attribute | Details |
|-----------|---------|
| **Legacy Sites** | 41 return nullptr, 103 Status/Result returns |
| **Priority** | **P0 - CRITICAL** |
| **Complexity** | **VERY HIGH** |
| **Estimated Effort** | 4 weeks |
| **Impact** | Core ML functionality; GPU operations |

**Files Affected:**
- `src/llm/llamacpp_inference_engine.cpp`
- `src/llm/model_loader.cpp` - 7 nullptr returns
- `src/llm/lora_framework/*.cpp` - 51 files
- `src/llm/distributed_training_coordinator.cpp`
- `src/llm/batch_processor.cpp`

**Error Codes Available:**
- LLM: 2000-2099 (12 codes) ✅
- LoRA: 2100-2199 (7 codes) ✅

**Error Codes to Add:**
- `ERR_LLM_BATCH_SIZE_EXCEEDED` (2012)
- `ERR_LORA_ADAPTER_CONFLICT` (2107)
- `ERR_LORA_TRAINING_DIVERGED` (2108)

**Complex Cases:**
1. **Multi-GPU Error Propagation** - Errors across GPU boundaries
2. **Async Inference Errors** - Error handling in async contexts
3. **LoRA Hot-Swap** - Adapter switching error scenarios

**Migration Pattern:**
```cpp
// Before: return nullptr
LLMContext* createContext() {
    if (gpu_memory_full) return nullptr;
    return ctx;
}

// After: Result<LLMContext*>
Result<LLMContext*> createContext() {
    if (gpu_memory_full) {
        return Err<LLMContext*>(ERR_LLM_GPU_OOM,
            fmt::format("GPU {} out of memory (needed {}MB)", 
                gpu_id, required_mb));
    }
    return Ok(ctx);
}
```

**Testing Requirements:**
- [ ] Update ~20 test files
- [ ] Add GPU failure simulation tests
- [ ] Add multi-GPU error propagation tests
- [ ] Add LoRA adapter conflict tests
- [ ] Add model loading stress tests
- [ ] Add async error handling tests

---

### Priority 1 (High - Weeks 6-8)

#### 3. Storage Layer Migration

| Attribute | Details |
|-----------|---------|
| **Legacy Sites** | 9 return nullptr, 33 Status/Result returns |
| **Priority** | **P1 - HIGH** |
| **Complexity** | **HIGH** |
| **Estimated Effort** | 2 weeks |
| **Impact** | Data persistence; reliability critical |

**Files Affected:**
- `src/storage/rocksdb_wrapper.cpp`
- `src/storage/blob_redundancy_manager.cpp`
- `src/storage/blob_backend_*.cpp` (5 files)
- `src/storage/log_manager.cpp`

**Error Codes Available:**
- `ERR_STORAGE_FILE_NOT_FOUND` (1000)
- `ERR_STORAGE_PERMISSION_DENIED` (1001)
- `ERR_STORAGE_DISK_FULL` (1002)
- `ERR_STORAGE_CORRUPTION` (1003)

**Error Codes to Add:**
- `ERR_STORAGE_TRANSACTION_FAILED` (1004)
- `ERR_STORAGE_CACHE_ERROR` (1005)
- `ERR_STORAGE_LOG_FULL` (1006)
- `ERR_STORAGE_REDUNDANCY_FAILED` (1007)

**Migration Pattern:**
```cpp
// Before: return nullptr
BlobHandle* getBlob(const std::string& key) {
    if (!exists(key)) return nullptr;
    return blob;
}

// After: Result<BlobHandle*>
Result<BlobHandle*> getBlob(const std::string& key) {
    if (!exists(key)) {
        return Err<BlobHandle*>(ERR_STORAGE_FILE_NOT_FOUND,
            fmt::format("Blob not found: {}", key));
    }
    return Ok(blob);
}
```

**Testing Requirements:**
- [ ] Update ~15 test files
- [ ] Add transaction failure tests
- [ ] Add redundancy failure tests
- [ ] Add corruption detection tests
- [ ] Add disk full simulation tests

---

#### 4. Transaction/Cache Layer Migration

| Attribute | Details |
|-----------|---------|
| **Legacy Sites** | 1 return nullptr, 33 Status/Result returns |
| **Priority** | **P1 - HIGH** |
| **Complexity** | **MEDIUM-HIGH** |
| **Estimated Effort** | 1.5 weeks |
| **Impact** | ACID guarantees; cache consistency |

**Files Affected:**
- `src/transaction/transaction_manager.cpp` - 33 Status returns
- `src/transaction/saga.cpp`
- `src/transaction/snapshot_manager.cpp`
- `src/cache/semantic_cache.cpp`
- `src/cache/embedding_cache.cpp`

**Error Codes to Add:**
- `ERR_TX_DEADLOCK_DETECTED` (1100)
- `ERR_TX_TIMEOUT` (1101)
- `ERR_TX_CONFLICT` (1102)
- `ERR_CACHE_EVICTION_FAILED` (1103)

**Testing Requirements:**
- [ ] Add deadlock detection tests
- [ ] Add timeout scenario tests
- [ ] Add conflict resolution tests
- [ ] Add cache eviction tests

---

### Priority 2 (Medium - Weeks 9-10)

#### 5. Utilities Migration

| Attribute | Details |
|-----------|---------|
| **Legacy Sites** | 12 return nullptr, 15 Status/Result returns |
| **Priority** | **P2 - MEDIUM** |
| **Complexity** | **LOW-MEDIUM** |
| **Estimated Effort** | 1.5 weeks |
| **Impact** | Support functions; lower visibility |

**Files Affected:**
- `src/utils/pki_client.cpp` - 5 nullptr
- `src/utils/pii_detection_engine.cpp` - 6 nullptr
- `src/utils/retention_manager.cpp` - 1 nullptr
- `src/crypto/*.cpp`
- `src/compression/*.cpp`

**Error Codes to Add:**
- `ERR_UTIL_INVALID_ARGUMENT` (7000)
- `ERR_CRYPTO_OPERATION_FAILED` (7100)
- `ERR_CRYPTO_KEY_NOT_FOUND` (7101)
- `ERR_COMPRESSION_FAILED` (7200)
- `ERR_COMPRESSION_CORRUPTED` (7201)

**Testing Requirements:**
- [ ] Update ~10 test files
- [ ] Add crypto failure tests
- [ ] Add compression edge case tests

---

### Priority 3 (Low - Week 11)

#### 6. Network Layer Migration

| Attribute | Details |
|-----------|---------|
| **Legacy Sites** | 0 return nullptr, 0 Status/Result returns |
| **Priority** | **P3 - LOW** |
| **Complexity** | **MINIMAL** |
| **Estimated Effort** | 0.5 weeks |
| **Impact** | Already using modern patterns |

**Status:** Network layer (wire_protocol_server.cpp) shows no legacy patterns. Only minor standardization needed if at all.

**Error Codes Available:**
- `ERR_NET_CONNECTION_REFUSED` (5000)
- `ERR_NET_TIMEOUT` (5001)
- `ERR_NET_DNS_FAILURE` (5002)

---

## 📋 Week-by-Week Plan

| Week | Module | Tasks | Deliverables |
|------|--------|-------|--------------|
| **1** | Query Engine | Parse failures, AQL translator | 96 Status → Result<T> |
| **2** | Query Engine | Query execution, CTE handling | 28 nullptr → Result<T> |
| **3** | Query Engine | Aggregators, cache, tests | All tests passing |
| **4** | LLM Core | model_loader, inference engine | 7 nullptr → Result<T> |
| **5** | LoRA Framework | 51 lora_framework files | 41 nullptr → Result<T> |
| **6** | Storage | RocksDB, blob storage | 9 nullptr → Result<T> |
| **7** | Storage + Tx | Log manager, transactions | 33 Status standardized |
| **8** | Cache + Tx | Semantic cache, snapshot mgr | All tests passing |
| **9** | Utilities | PKI, PII, retention | 12 nullptr → Result<T> |
| **10** | Utilities | Crypto, compression | All tests passing |
| **11** | Cleanup | Deprecation, docs, final tests | Phase 4 complete |
| **12** | Buffer | Bug fixes, performance tuning | Production ready |

---

## 🧪 Testing Strategy

### Per-Module Testing

**For Each Module:**
1. **Unit Tests** - Update all affected test files
2. **Integration Tests** - Error propagation across boundaries
3. **Error Scenario Tests** - Each error code triggered
4. **Performance Tests** - No regression > 5%

### Overall Testing

**Phase 4 Completion Criteria:**
- [ ] All 91 nullptr sites migrated to Result<T>
- [ ] All 373 Status returns standardized to Result<T>
- [ ] Zero compiler warnings
- [ ] All unit tests passing
- [ ] All integration tests passing
- [ ] Performance within 5% of baseline
- [ ] Code coverage ≥ 85%
- [ ] Security audit passed

---

## 📊 Success Metrics

### Coverage Targets

| Metric | Target | Status |
|--------|--------|--------|
| nullptr Sites Migrated | 91 / 91 (100%) | 🟡 0% |
| Status/Result Standardized | 373 / 373 (100%) | 🟡 0% |
| Error Codes Defined | 50+ total | ✅ 31 |
| Test Coverage | ≥ 85% | 🟡 TBD |
| Performance Impact | < 5% | 🟡 TBD |

### Quality Gates

- ✅ No P0/P1 bugs introduced
- ✅ All existing tests passing
- ✅ Error context preserved in 100% of cases
- ✅ Documentation updated
- ✅ Team training complete

---

## 🚨 Risk Management

### Technical Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Query Engine complexity | High | Incremental migration, extensive testing |
| LLM/LoRA GPU errors | High | GPU simulation tests, error isolation |
| Performance regression | Medium | Continuous benchmarking, rollback plan |
| Breaking API changes | Critical | Backward compatibility layer |

### Contingency Plans

1. **If Performance Regresses >5%:**
   - Profile hot paths
   - Optimize Result<T> usage
   - Consider conditional compilation

2. **If Tests Fail:**
   - Roll back module
   - Add missing error scenarios
   - Iterate on error codes

3. **If Deadlines Slip:**
   - Defer P2/P3 modules
   - Focus on P0/P1 critical paths
   - Add buffer time

---

## 📚 Related Documents

- **Phase 1-2:** `ERROR_HANDLING_PHASE_1_2_COMPLETE.md` - Foundation
- **Phase 3:** `ERROR_HANDLING_PHASE_3_PLAN.md` - High-value paths
- **Infrastructure:** `include/utils/expected.h` - Result<T> wrapper
- **Error Registry:** `include/utils/error_registry.h` - Error codes
- **Migration Guide:** `examples/migration/README.md` - Patterns

---

## ✅ Definition of Done

Phase 4 is complete when:

**Code:**
- [ ] All 91 nullptr sites → Result<T>
- [ ] All 373 Status returns standardized
- [ ] All new error codes registered
- [ ] No compiler warnings
- [ ] Code reviews approved

**Testing:**
- [ ] All unit tests passing (100%)
- [ ] All integration tests passing (100%)
- [ ] Performance benchmarks < 5% regression
- [ ] Security audit passed
- [ ] Code coverage ≥ 85%

**Documentation:**
- [ ] Migration guide updated
- [ ] API documentation updated
- [ ] Error catalog complete
- [ ] Team training materials ready

**Deployment:**
- [ ] Staged rollout complete
- [ ] Production validation passed
- [ ] No P0/P1 bugs in 2 weeks
- [ ] Celebration! 🎉

---

**Last Updated:** 2026-04-06  
**Next Review:** End of Week 2  
**Owner:** TBD
