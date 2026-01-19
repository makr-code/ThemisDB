---
name: "Error Handling Migration - Phase 4: Full Migration"
about: Complete migration of all remaining error sites to tl::expected
title: "[Error Handling] Phase 4: Complete Full Migration"
labels: priority:P2, type:feature, area:core, effort:xx-large, phase:4
assignees: ''
---

## 🎯 Objective

Complete the migration of all remaining error sites (~270+ locations) from legacy error patterns to the unified `tl::expected`-based error handling system.

**Status:** ⚪ PLANNED  
**Priority:** P2 (Medium - after Phase 3)  
**Effort:** 10-12 weeks  
**Dependencies:** Phase 3 (High-Value Migration) must be completed

## 📋 Background

**Completed Phases:**
- ✅ Phase 1 & 2: Foundation and documentation
- ✅ Phase 3: High-value code paths (~20-30 sites)

**Current State:**
- ~270+ remaining error sites using legacy patterns
- ~60% of codebase using structured error codes
- Legacy patterns still active for backward compatibility

**Goal:**
Complete the migration to achieve 100% adoption of unified error handling across the entire codebase.

## 🔧 Implementation Tasks

### 1. Remaining Code Inventory (Week 1)

**Scan Codebase for Legacy Patterns:**
- [ ] Find all `return nullptr` sites
  ```bash
  grep -r "return nullptr" --include="*.cpp" --include="*.h" src/ include/
  ```
- [ ] Find all custom Status struct usage
  ```bash
  grep -r "struct Status" --include="*.h" include/
  ```
- [ ] Find all `std::optional` used for error handling
  ```bash
  grep -r "std::optional" --include="*.cpp" --include="*.h" src/ include/
  ```

**Categorize by Module:**
- [ ] Storage layer (~50 sites)
- [ ] Network layer (~40 sites)
- [ ] LLM/LoRA modules (~60 sites)
- [ ] Query engine (~30 sites)
- [ ] Schema management (~20 sites)
- [ ] Utilities (~40 sites)
- [ ] Other modules (~30 sites)

**Create Migration Matrix:**
```
Module          | Legacy Sites | Priority | Complexity | Est. Effort
----------------|--------------|----------|------------|------------
Storage         | 50           | High     | Medium     | 2 weeks
Network         | 40           | High     | Low        | 1.5 weeks
LLM/LoRA        | 60           | Medium   | High       | 3 weeks
Query Engine    | 30           | Medium   | Medium     | 1.5 weeks
Schema Mgmt     | 20           | Low      | Low        | 1 week
Utilities       | 40           | Low      | Low        | 1.5 weeks
Other           | 30           | Low      | Medium     | 1.5 weeks
```

**Deliverable:** `docs/error_handling/phase4_migration_matrix.md`

---

### 2. Storage Layer Migration (Week 2-3)

**Modules:**
- [ ] RocksDB wrapper
- [ ] Blob storage
- [ ] Log storage
- [ ] Cache layer
- [ ] Transaction manager

**Files:** (~25 files, ~50 methods)
- `src/storage/rocksdb_wrapper.cpp`
- `src/storage/blob_storage.cpp`
- `src/storage/log_manager.cpp`
- `src/cache/*.cpp`
- `src/transaction/*.cpp`

**Error Codes to Add:**
- [ ] `ERR_STORAGE_TRANSACTION_FAILED`
- [ ] `ERR_STORAGE_CACHE_ERROR`
- [ ] `ERR_STORAGE_LOG_FULL`

**Testing:**
- [ ] Update ~15 test files
- [ ] Add transaction failure tests
- [ ] Add cache eviction tests

---

### 3. Network Layer Migration (Week 4)

**Modules:**
- [ ] HTTP client/server
- [ ] WebSocket handler
- [ ] gRPC client/server
- [ ] Connection pool

**Files:** (~20 files, ~40 methods)
- `src/network/http_client.cpp`
- `src/network/http_server.cpp`
- `src/network/websocket_handler.cpp`
- `src/network/grpc_*.cpp`

**Error Codes Already Available:**
- `ERR_NET_CONNECTION_REFUSED`
- `ERR_NET_TIMEOUT`
- `ERR_NET_DNS_FAILURE`

**Testing:**
- [ ] Update ~10 test files
- [ ] Add connection timeout tests
- [ ] Add DNS failure tests

---

### 4. LLM/LoRA Migration (Week 5-7)

**Modules:**
- [ ] LLM inference engine
- [ ] LoRA adapter management
- [ ] Model loader
- [ ] Training coordinator
- [ ] Batch processor

**Files:** (~30 files, ~60 methods)
- `src/llm/llamacpp_inference_engine.cpp`
- `src/llm/lora_framework/*.cpp`
- `src/llm/model_loader.cpp`
- `src/llm/distributed_training_coordinator.cpp`

**Error Codes Already Available:**
- `ERR_LLM_*` (11 codes)
- `ERR_LORA_*` (7 codes)

**Complex Cases:**
- [ ] Multi-GPU error propagation
- [ ] Async inference error handling
- [ ] LoRA hot-swap error scenarios

**Testing:**
- [ ] Update ~20 test files
- [ ] Add GPU failure simulation
- [ ] Add model loading stress tests
- [ ] Add multi-adapter error tests

---

### 5. Query Engine Migration (Week 8)

**Modules:**
- [ ] AQL executor
- [ ] Query planner
- [ ] Expression evaluator
- [ ] Join operations
- [ ] Aggregation functions

**Files:** (~15 files, ~30 methods)
- `src/query/aql_executor.cpp`
- `src/query/query_planner.cpp`
- `src/query/expression_evaluator.cpp`
- `src/query/join_*.cpp`

**Error Codes Already Available:**
- `ERR_QUERY_*` (4 codes)

**Testing:**
- [ ] Update ~12 test files
- [ ] Add complex query error tests
- [ ] Add timeout scenario tests

---

### 6. Schema Management Migration (Week 9)

**Modules:**
- [ ] Schema validator
- [ ] Type checker
- [ ] Migration manager
- [ ] Metadata store

**Files:** (~12 files, ~20 methods)
- `src/schema/schema_validator.cpp`
- `src/schema/type_checker.cpp`
- `src/schema/migration_manager.cpp`

**Error Codes Already Available:**
- `ERR_SCHEMA_*` (3 codes)

**Testing:**
- [ ] Update ~8 test files
- [ ] Add schema validation tests
- [ ] Add migration failure tests

---

### 7. Utilities & Support Modules (Week 10)

**Modules:**
- [ ] File utilities
- [ ] String utilities
- [ ] Crypto utilities
- [ ] Compression
- [ ] Logging helpers

**Files:** (~20 files, ~40 methods)
- `src/utils/*.cpp`
- `src/crypto/*.cpp`
- `src/compression/*.cpp`

**New Error Codes:**
- [ ] `ERR_UTIL_INVALID_ARGUMENT`
- [ ] `ERR_CRYPTO_OPERATION_FAILED`
- [ ] `ERR_COMPRESSION_FAILED`

**Testing:**
- [ ] Update ~15 test files
- [ ] Add utility edge case tests

---

### 8. Cleanup & Deprecation (Week 11)

**Remove Legacy Patterns:**
- [ ] Remove local Status struct definitions
  - `content/content_fs.h`
  - `timeseries/tsstore.h`
  - Other modules
- [ ] Mark conversion helpers as deprecated
  ```cpp
  [[deprecated("Use Result<T> directly")]]
  Result<T*> fromNullable(...);
  ```
- [ ] Remove backward compatibility shims
- [ ] Update all examples to use Result<T>

**Code Cleanup:**
- [ ] Remove unused error handling code
- [ ] Standardize error message formatting
- [ ] Remove TODO comments about migration
- [ ] Update code comments

**Documentation Updates:**
- [ ] Mark legacy patterns as deprecated in docs
- [ ] Update all code examples
- [ ] Update API documentation
- [ ] Update contributor guide

---

### 9. Final Validation (Week 12)

**Comprehensive Testing:**
- [ ] Run full test suite
- [ ] Run integration tests
- [ ] Run performance benchmarks
- [ ] Run security audit
- [ ] Run static analysis

**Verification:**
- [ ] Grep for remaining legacy patterns
  ```bash
  # Should return 0 results
  grep -r "return nullptr" src/ include/ | grep -v "test" | grep -v "example"
  ```
- [ ] Verify all modules use Result<T>
- [ ] Check error code coverage
- [ ] Validate error message consistency

**Performance Validation:**
- [ ] Compare before/after benchmarks
- [ ] Ensure no regression (< 5% overhead)
- [ ] Profile error-heavy paths
- [ ] Memory usage analysis

**Documentation Audit:**
- [ ] All public APIs documented
- [ ] All error codes documented
- [ ] Migration guide complete
- [ ] Examples up to date

---

## 📊 Success Metrics

**Coverage Goals:**
- [ ] 100% of codebase using Result<T>
- [ ] 0 remaining legacy error patterns
- [ ] All error codes registered in ErrorRegistry
- [ ] All modules have error handling tests

**Quality Goals:**
- [ ] All tests pass
- [ ] No performance regression (< 5%)
- [ ] Code coverage ≥ 85%
- [ ] Static analysis: 0 warnings

**Developer Experience:**
- [ ] Single, consistent error handling pattern
- [ ] Clear error messages with context
- [ ] Easy error recovery patterns
- [ ] Comprehensive documentation

---

## 🧪 Testing Strategy

### Regression Testing
- Run full test suite after each module migration
- Compare test results with baseline
- Fix any regressions immediately

### Migration Testing
- Test migrated code with legacy callers
- Test legacy code with migrated callees
- Test error propagation across boundaries

### Performance Testing
- Benchmark each module before/after
- Profile hot paths
- Memory leak detection
- Load testing

### Security Testing
- Verify no data leaks in errors
- Test error handling under attack
- Validate input sanitization

---

## 📚 Dependencies

**Prerequisites:**
- [x] Phase 1 & 2 completed
- [ ] Phase 3 completed
- [ ] All high-value paths migrated
- [ ] Team fully trained on Result<T>

**Blockers:**
- Phase 3 must be completed first
- No new features during migration period
- Code freeze on error handling patterns

**Related Issues:**
- #XXX - Phase 3 migration (must complete first)
- #YYY - Deprecate legacy patterns (part of this phase)

---

## 🎯 Definition of Done

**Code:**
- [ ] 100% of codebase migrated to Result<T>
- [ ] All legacy patterns removed
- [ ] All conversion helpers deprecated
- [ ] All tests updated and passing

**Documentation:**
- [ ] All documentation updated
- [ ] Migration guide complete
- [ ] API docs reflect Result<T>
- [ ] Examples updated

**Quality:**
- [ ] Code review passed
- [ ] Security review passed
- [ ] Performance validation passed
- [ ] Static analysis clean

**Process:**
- [ ] Team trained and confident
- [ ] Migration metrics documented
- [ ] Lessons learned documented
- [ ] Future guidelines established

---

## 📝 Migration Workflow

### Per-Module Process:

1. **Plan** (1 day)
   - Review module code
   - Identify all error sites
   - Plan migration order
   - Estimate effort

2. **Implement** (3-5 days per module)
   - Migrate methods to Result<T>
   - Add error codes if needed
   - Update error messages
   - Add error context

3. **Test** (2-3 days per module)
   - Update unit tests
   - Add error scenario tests
   - Run integration tests
   - Performance validation

4. **Review** (1-2 days per module)
   - Self-review changes
   - Peer code review
   - Address feedback
   - Security review

5. **Document** (1 day per module)
   - Update API docs
   - Update error catalog
   - Add usage examples
   - Update migration log

6. **Commit** (merge to main)
   - Small, focused commits
   - Clear commit messages
   - Link to tracking issue

---

## 🚨 Risks & Mitigation

### Risk: Breaking Changes
**Mitigation:**
- Gradual migration with deprecation period
- Maintain backward compatibility during transition
- Feature flags for new error handling
- Comprehensive testing before removal

### Risk: Performance Regression
**Mitigation:**
- Continuous performance monitoring
- Benchmark after each module
- Profile hot paths regularly
- Rollback if regression > 5%

### Risk: Team Confusion
**Mitigation:**
- Clear migration guide
- Regular team sync meetings
- Code review for all changes
- Dedicated migration channel

### Risk: Incomplete Migration
**Mitigation:**
- Automated detection of legacy patterns
- CI/CD checks for Result<T> usage
- Regular progress tracking
- Clear success metrics

---

## 📈 Progress Tracking

**Weekly Goals:**
- Week 1: Inventory complete
- Week 2-3: Storage layer done
- Week 4: Network layer done
- Week 5-7: LLM/LoRA done
- Week 8: Query engine done
- Week 9: Schema management done
- Week 10: Utilities done
- Week 11: Cleanup done
- Week 12: Validation done

**Milestone Markers:**
- [ ] 25% complete (Storage + Network)
- [ ] 50% complete (+ LLM/LoRA)
- [ ] 75% complete (+ Query + Schema)
- [ ] 100% complete (All modules)

---

## 🔗 References

- Phase 3 Issue: #XXX
- Error Handling Foundation: `include/utils/expected.h`
- Migration Guide: `examples/migration/README.md`
- Implementation Summary: `ERROR_HANDLING_IMPLEMENTATION_SUMMARY.md`
- Error Registry: `include/utils/error_registry.h`
- Test Examples: `tests/test_expected.cpp`

---

## 💬 Communication Plan

**Status Updates:**
- Weekly progress report in team meeting
- Bi-weekly blog post on progress
- Monthly demo of migrated modules

**Documentation:**
- Update CHANGELOG.md with each merge
- Document challenges and solutions
- Share best practices learned

**Training:**
- Office hours for migration questions
- Pair programming sessions
- Code review guidelines
