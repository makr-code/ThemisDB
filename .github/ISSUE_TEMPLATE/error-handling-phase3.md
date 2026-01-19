---
name: "Error Handling Migration - Phase 3: High-Value Code Paths"
about: Migrate high-traffic code paths to tl::expected error handling
title: "[Error Handling] Phase 3: Migrate High-Value Code Paths"
labels: priority:P1, type:feature, area:core, effort:x-large, phase:3
assignees: ''
---

## 🎯 Objective

Migrate high-traffic, high-value code paths from legacy error patterns (`return nullptr`, `Status{ok, msg}`, `std::optional`) to the new `tl::expected`-based error handling system.

**Status:** ⚪ PLANNED  
**Priority:** P1 (High)  
**Effort:** 6-8 weeks  
**Dependencies:** Error handling foundation (Phase 1 & 2) must be merged

## 📋 Background

Phase 1 & 2 established the foundation with:
- ✅ `include/utils/expected.h` - Result<T> wrapper and Error class
- ✅ 62 error codes across 10 categories
- ✅ Unit tests and migration examples
- ✅ Comprehensive documentation

**Current State:**
- ~300+ error sites still using legacy patterns
- Only ~40% of codebase uses structured error codes
- Inconsistent error handling across modules

**Goal:**
Focus on high-impact areas first to maximize benefits while keeping changes manageable and reviewable.

## 🔧 Implementation Tasks

### 1. Identify High-Value Code Paths (Week 1)

**Priority Criteria:**
- [ ] Code frequently executed (hot paths)
- [ ] User-facing APIs (HTTP/gRPC/GraphQL endpoints)
- [ ] Error-prone operations (I/O, network, parsing)
- [ ] Code with existing error handling issues

**Analysis Tasks:**
- [ ] Profile application to identify hot paths
- [ ] Review API endpoint handlers for error patterns
- [ ] Analyze error logs to find common failure points
- [ ] Create prioritized migration list (20-30 sites)

**Deliverable:** `docs/error_handling/phase3_migration_targets.md`

---

### 2. IndexManager Migration (Week 2-3)

**Files to Migrate:**
- [ ] `src/index/index_manager.cpp` - Core index operations
- [ ] `include/index/index_manager.h` - Interface definitions
- [ ] `src/index/secondary_index_manager.cpp` - Secondary indexes
- [ ] `src/index/vector_index_manager.cpp` - Vector indexes

**Methods to Convert:** (~15 methods)
```cpp
// Pattern: nullptr → Result<T*>
- createSecondaryIndex()
- getSecondaryIndex()
- createVectorIndex()
- getVectorIndex()
- createGraphIndex()
- getGraphIndex()
```

**Error Codes to Use:**
- `ERR_INDEX_NOT_INITIALIZED`
- `ERR_INDEX_CREATION_FAILED`
- `ERR_INDEX_NOT_FOUND`
- `ERR_INDEX_INVALID_TYPE`

**Testing:**
- [ ] Update existing unit tests in `tests/test_index_manager.cpp`
- [ ] Add new tests for error scenarios
- [ ] Integration tests with query engine
- [ ] Performance benchmarks (compare with legacy)

**Acceptance Criteria:**
- All IndexManager methods return Result<T> or Result<T*>
- All tests pass
- No performance regression
- API documentation updated

---

### 3. ContentFS Migration (Week 3-4)

**Files to Migrate:**
- [ ] `src/content/content_fs.cpp` - Content filesystem
- [ ] `include/content/content_fs.h` - Interface
- [ ] Related ContentFS tests

**Methods to Convert:** (~8 methods)
```cpp
// Pattern: Status{ok, msg} → Result<T>
- put() → Result<void>
- get() → Result<std::vector<uint8_t>>
- del() → Result<void>
- exists() → Result<bool>
- list() → Result<std::vector<std::string>>
```

**Error Codes to Use:**
- `ERR_STORAGE_FILE_NOT_FOUND`
- `ERR_STORAGE_PERMISSION_DENIED`
- `ERR_STORAGE_DISK_FULL`
- `ERR_STORAGE_CORRUPTION`
- `ERR_API_INVALID_REQUEST`

**Testing:**
- [ ] Update `tests/test_content_fs.cpp`
- [ ] Add corruption detection tests
- [ ] Add disk full simulation tests
- [ ] Stress tests for concurrent operations

**Acceptance Criteria:**
- Remove local Status struct definition
- All methods return Result<T>
- Error context includes file paths
- All tests pass

---

### 4. TSStore Migration (Week 4-5)

**Files to Migrate:**
- [ ] `src/timeseries/tsstore.cpp` - Time series storage
- [ ] `include/timeseries/tsstore.h` - Interface

**Methods to Convert:** (~10 methods)
```cpp
// Pattern: std::optional<T> → Result<T>
- parseKey() → Result<KeyComponents>
- extractTimestamp() → Result<int64_t>
- validateKey() → Result<void>
- get() → Result<TimeSeriesData>
- put() → Result<void>
```

**Error Codes to Use:**
- `ERR_QUERY_PARSE_FAILED`
- `ERR_QUERY_INVALID_SYNTAX`
- `ERR_SCHEMA_INVALID_TYPE`
- `ERR_API_INVALID_REQUEST`

**Testing:**
- [ ] Update `tests/test_tsstore.cpp`
- [ ] Add malformed key tests
- [ ] Add timestamp overflow tests
- [ ] Performance comparison tests

**Acceptance Criteria:**
- All parsing methods return Result<T>
- Detailed error messages for parse failures
- All tests pass
- Documentation updated

---

### 5. PluginManager Migration (Week 5-6)

**Files to Migrate:**
- [ ] `src/plugins/plugin_manager.cpp` - Plugin loading/management
- [ ] `include/plugins/plugin_manager.h` - Interface

**Methods to Convert:** (~12 methods)
```cpp
// Pattern: nullptr → Result<T*>
- loadPlugin() → Result<IPlugin*>
- getPlugin() → Result<IPlugin*>
- unloadPlugin() → Result<void>
- verifySignature() → Result<bool>
- calculateFileHash() → Result<std::string>
```

**Error Codes to Use:**
- `ERR_PLUGIN_NOT_FOUND`
- `ERR_PLUGIN_LOAD_FAILED`
- `ERR_PLUGIN_INCOMPATIBLE`
- `ERR_PLUGIN_INVALID_SIGNATURE`

**Testing:**
- [ ] Update `tests/test_plugin_manager.cpp`
- [ ] Add signature verification tests
- [ ] Add incompatible plugin tests
- [ ] Security tests for malicious plugins

**Acceptance Criteria:**
- All plugin operations return Result<T>
- Signature failures provide detailed errors
- All tests pass
- Security documentation updated

---

### 6. GraphQL Parser Migration (Week 6-7)

**Files to Migrate:**
- [ ] `src/query/aql_parser.cpp` - AQL query parser
- [ ] `src/api/graphql.cpp` - GraphQL parser
- [ ] Related parser headers

**Methods to Convert:** (~8 methods)
```cpp
// Pattern: nullptr/optional → Result<T>
- parseQuery() → Result<ParsedQuery>
- parseOperation() → Result<Operation>
- parseSelection() → Result<Selection>
- validateQuery() → Result<void>
```

**Error Codes to Use:**
- `ERR_QUERY_PARSE_FAILED`
- `ERR_QUERY_INVALID_SYNTAX`
- `ERR_API_INVALID_REQUEST`

**Testing:**
- [ ] Update parser tests
- [ ] Add syntax error tests with detailed messages
- [ ] Add edge case tests
- [ ] Performance tests for large queries

**Acceptance Criteria:**
- Remove custom ParseError struct
- All parsing returns Result<T>
- Error messages include line/column info
- All tests pass

---

### 7. API Layer Migration (Week 7-8)

**Files to Migrate:**
- [ ] HTTP API handlers
- [ ] gRPC service implementations
- [ ] GraphQL resolvers

**Methods to Convert:** (~20+ endpoints)
```cpp
// Pattern: Mixed → Result<T>
- HTTP handlers → Result<Response>
- gRPC RPCs → Result<ProtobufMessage>
- GraphQL resolvers → Result<Value>
```

**Error Codes to Use:**
- `ERR_API_INVALID_REQUEST`
- `ERR_API_UNAUTHORIZED`
- `ERR_API_RATE_LIMIT`
- `ERR_API_INTERNAL_ERROR`

**Error Translation:**
- [ ] Map Result<T> errors to HTTP status codes
- [ ] Map Result<T> errors to gRPC status codes
- [ ] Map Result<T> errors to GraphQL errors
- [ ] Include error metadata in responses

**Testing:**
- [ ] Update API integration tests
- [ ] Add error response format tests
- [ ] Add rate limiting tests
- [ ] End-to-end tests

**Acceptance Criteria:**
- All API endpoints return Result<T>
- Consistent error format across protocols
- Error metadata in responses
- All tests pass

---

### 8. Documentation & Best Practices (Week 8)

**Documentation Updates:**
- [ ] Update API documentation with error codes
- [ ] Create error handling best practices guide
- [ ] Update contributor guidelines
- [ ] Add examples to public documentation

**Code Quality:**
- [ ] Run static analysis on migrated code
- [ ] Performance benchmarks comparison
- [ ] Security audit of error handling
- [ ] Code coverage report

**Training Materials:**
- [ ] Internal team training on Result<T>
- [ ] Migration troubleshooting guide
- [ ] Common patterns and anti-patterns
- [ ] FAQ document

---

## 📊 Success Metrics

**Coverage:**
- [ ] 20-30 high-value methods migrated (~10% of total)
- [ ] All user-facing APIs use Result<T>
- [ ] All I/O operations use Result<T>

**Quality:**
- [ ] All tests pass
- [ ] No performance regression (within 5%)
- [ ] Error context preserved in 100% of cases
- [ ] Code coverage maintained or improved

**Developer Experience:**
- [ ] Migration guide validated with team
- [ ] Zero confusion about error handling patterns
- [ ] Positive team feedback on new system

---

## 🧪 Testing Strategy

### Unit Tests
- Update existing tests for migrated methods
- Add error scenario tests
- Test error message formatting
- Test error metadata retrieval

### Integration Tests
- Test error propagation across modules
- Test error translation at API boundaries
- Test backward compatibility with legacy code

### Performance Tests
- Benchmark migrated vs legacy code
- Profile error path overhead
- Stress test error-heavy scenarios

### Security Tests
- Verify no sensitive data in error messages
- Test error handling under attack scenarios
- Validate error code cannot be manipulated

---

## 📚 Dependencies

**Prerequisites:**
- [x] Error handling foundation merged (Phase 1 & 2)
- [ ] Team trained on Result<T> patterns
- [ ] Migration guide reviewed and approved

**Related Issues:**
- #XXX - Error handling foundation (completed)
- #YYY - Phase 4: Full migration (blocked by this)

---

## 🎯 Definition of Done

- [ ] All identified high-value code paths migrated
- [ ] All tests pass (unit, integration, performance)
- [ ] No performance regression
- [ ] Code review completed
- [ ] Documentation updated
- [ ] Security review passed
- [ ] Team training completed
- [ ] Migration metrics documented

---

## 📝 Notes

**Backward Compatibility:**
- Maintain conversion helpers during transition
- Keep legacy error patterns working alongside new system
- No breaking changes to external APIs

**Rollback Plan:**
- Git tags before each major migration step
- Feature flags for new error handling
- Gradual rollout with monitoring

**Communication:**
- Weekly progress updates to team
- Documentation of learnings and challenges
- Knowledge sharing sessions

---

## 🔗 References

- Error Handling Foundation: `include/utils/expected.h`
- Migration Guide: `examples/migration/README.md`
- Implementation Summary: `ERROR_HANDLING_IMPLEMENTATION_SUMMARY.md`
- Error Registry: `include/utils/error_registry.h`
