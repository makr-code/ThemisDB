# Error Handling Migration - Phase 3: High-Value Code Paths

**Status:** 🟡 PLANNED  
**Priority:** P1 (High)  
**Effort:** 6-8 weeks  
**Dependencies:** ✅ Phase 1-2 Complete  
**Target Start:** Q2 2026  
**Owner:** TBD

---

## 🎯 Executive Summary

Phase 3 focuses on migrating 20-30 high-traffic, high-value code paths from legacy error patterns to `tl::expected`-based error handling. This represents ~10% of the codebase but covers the most critical user-facing and error-prone operations.

**Scope:** Migrate key modules with maximum impact:
- IndexManager (15 methods)
- ContentFS (8 methods)
- TSStore (10 methods)
- PluginManager (12 methods)
- GraphQL/AQL Parser (8 methods)
- API Layer (20+ endpoints)

**Benefits:**
- Consistent error handling across all user-facing APIs
- Type-safe error propagation for critical operations
- Rich error context for debugging production issues
- Foundation for Phase 4 full migration

---

## 📋 Background & Context

### Current State (Post Phase 1-2)

✅ **Infrastructure Ready:**
- `tl::expected` dependency integrated
- `Result<T>` wrapper and `Error` class available
- 62 error codes with rich metadata
- 20+ unit tests validating foundation
- 3 migration examples (900+ lines)
- Comprehensive migration guide

❌ **Legacy Patterns Still Dominant:**
- ~300+ error sites using inconsistent patterns:
  - `return nullptr` - 40% of sites
  - `Status{ok, message}` - 30% of sites
  - `std::optional` - 30% of sites
- No machine-readable error codes at call sites
- Error context lost across function boundaries
- Inconsistent error handling across modules

### Why Phase 3 First?

**High-Impact, Manageable Scope:**
- Focus on ~10% of codebase with highest user visibility
- User-facing APIs benefit most from structured errors
- Enables client applications to handle errors programmatically
- Provides team experience before Phase 4 full migration

**Risk Mitigation:**
- Validate patterns on real code before wide adoption
- Discover integration issues early
- Gather team feedback on developer experience
- Refine migration process for Phase 4

---

## 🗺️ Detailed Migration Plan

### Week 1: Planning & Analysis

**Goal:** Identify and prioritize migration targets

**Tasks:**
1. **Profile Application** (2 days)
   - Run production profiler to identify hot paths
   - Analyze request traces for common code paths
   - Review error logs for failure-prone operations
   - Map frequency data to code locations

2. **Analyze Error Patterns** (2 days)
   - Audit existing error handling in target modules
   - Document current error patterns per module
   - Identify error information being lost
   - Map legacy patterns to Result<T> equivalents

3. **Create Migration Priority List** (1 day)
   - Score each module by impact and effort
   - Create ordered work breakdown
   - Assign effort estimates
   - Identify dependencies between modules

**Deliverables:**
- `docs/error_handling/phase3_migration_targets.md` - Prioritized list
- `docs/error_handling/phase3_effort_estimates.md` - Detailed estimates
- Team kickoff presentation

---

### Week 2-3: IndexManager Migration

**Priority:** 🔴 Critical (foundation for query operations)

#### Files to Migrate
```
src/index/index_manager.cpp          (Primary)
include/index/index_manager.h        (Interface)
src/index/secondary_index_manager.cpp
src/index/vector_index_manager.cpp
src/index/graph_index_manager.cpp
tests/test_index_manager.cpp         (Tests)
```

#### Methods to Convert (~15 methods)

**Pattern:** `nullptr → Result<T*>`

| Method | Current | New | Error Codes |
|--------|---------|-----|-------------|
| `createSecondaryIndex()` | `ISecondaryIndex*` | `Result<ISecondaryIndex*>` | NOT_INITIALIZED, CREATION_FAILED, INVALID_REQUEST |
| `getSecondaryIndex()` | `ISecondaryIndex*` | `Result<ISecondaryIndex*>` | NOT_FOUND, INVALID_TYPE |
| `createVectorIndex()` | `IVectorIndex*` | `Result<IVectorIndex*>` | NOT_INITIALIZED, CREATION_FAILED |
| `getVectorIndex()` | `IVectorIndex*` | `Result<IVectorIndex*>` | NOT_FOUND |
| `createGraphIndex()` | `IGraphIndex*` | `Result<IGraphIndex*>` | NOT_INITIALIZED, CREATION_FAILED |
| `getGraphIndex()` | `IGraphIndex*` | `Result<IGraphIndex*>` | NOT_FOUND |
| `dropIndex()` | `bool` | `Result<void>` | NOT_FOUND, PERMISSION_DENIED |
| `listIndexes()` | `std::vector<...>*` | `Result<std::vector<...>>` | NOT_INITIALIZED |
| ... | ... | ... | ... |

#### Error Codes to Use
- `ERR_INDEX_NOT_INITIALIZED` - Manager not initialized
- `ERR_INDEX_CREATION_FAILED` - Index creation failed
- `ERR_INDEX_NOT_FOUND` - Index doesn't exist
- `ERR_INDEX_INVALID_TYPE` - Wrong index type requested
- `ERR_API_INVALID_REQUEST` - Invalid parameters
- `ERR_STORAGE_DISK_FULL` - No space for index

#### Implementation Steps

1. **Update Headers** (Day 1)
   - Change return types to `Result<T*>`
   - Add error code documentation to methods
   - Update method signatures

2. **Migrate Implementation** (Days 2-4)
   - Replace `return nullptr` with `Err<T*>(code, context)`
   - Replace successful returns with `Ok(value)`
   - Add detailed error context (index names, types)
   - Use `fromNullable()` helper where appropriate

3. **Update Call Sites** (Days 5-6)
   - Update all callers to check `Result<T>`
   - Add proper error handling at call sites
   - Propagate errors up the stack with `and_then()`
   - Add logging for errors

4. **Update Tests** (Days 7-8)
   - Modify existing tests to check `Result<T>`
   - Add error scenario tests
   - Test error messages and context
   - Add integration tests

5. **Documentation** (Day 9)
   - Update API documentation
   - Add migration notes
   - Document error codes

#### Testing Strategy
- [ ] All existing tests pass
- [ ] New error scenario tests added (10+ cases)
- [ ] Integration tests with query engine
- [ ] Performance benchmarks (< 5% regression)
- [ ] Error message quality validation

#### Acceptance Criteria
✅ All IndexManager methods return `Result<T>` or `Result<T*>`  
✅ All tests pass (unit + integration)  
✅ No performance regression  
✅ Error context includes index names/types  
✅ API documentation updated  
✅ Code review approved

---

### Week 3-4: ContentFS Migration

**Priority:** 🔴 Critical (all content operations)

#### Files to Migrate
```
src/content/content_fs.cpp
include/content/content_fs.h
tests/test_content_fs.cpp
```

#### Methods to Convert (~8 methods)

**Pattern:** `Status{ok, message} → Result<T>`

| Method | Current | New | Error Codes |
|--------|---------|-----|-------------|
| `put()` | `Status` | `Result<void>` | DISK_FULL, PERMISSION_DENIED, INVALID_REQUEST |
| `get()` | `Status + out param` | `Result<std::vector<uint8_t>>` | FILE_NOT_FOUND, PERMISSION_DENIED, CORRUPTION |
| `del()` | `Status` | `Result<void>` | FILE_NOT_FOUND, PERMISSION_DENIED |
| `exists()` | `Status + bool` | `Result<bool>` | PERMISSION_DENIED |
| `list()` | `Status + vector` | `Result<std::vector<...>>` | PERMISSION_DENIED |

#### Key Changes
- **Remove local `Status` struct** - No longer needed
- **Eliminate out parameters** - Return values in Result<T>
- **Add file path context** - Every error includes path
- **Use storage error codes** - Consistent with error registry

#### Implementation Steps

1. **Remove Status Struct** (Day 1)
   - Delete `Status` definition
   - Update all return types to `Result<T>`

2. **Migrate Methods** (Days 2-4)
   - Replace `Status{false, msg}` with `Err<T>(code, context)`
   - Replace `Status{true}` with `Ok(value)` or `OkVoid()`
   - Eliminate out parameters
   - Add file paths to error context

3. **Update Call Sites** (Days 5-6)
   - Change from `if (!status.ok)` to `if (!result)`
   - Access values with `*result` instead of out params
   - Propagate errors properly

4. **Testing** (Days 7-8)
   - Update existing tests
   - Add disk full simulation tests
   - Add corruption detection tests
   - Add concurrent operation tests

#### Acceptance Criteria
✅ Local `Status` struct removed  
✅ All methods return `Result<T>`  
✅ Error context includes file paths  
✅ All tests pass  
✅ Performance maintained

---

### Week 4-5: TSStore Migration

**Priority:** 🟡 High (time series operations)

#### Files to Migrate
```
src/timeseries/tsstore.cpp
include/timeseries/tsstore.h
tests/test_tsstore.cpp
```

#### Methods to Convert (~10 methods)

**Pattern:** `std::optional<T> → Result<T>`

| Method | Current | New | Error Codes |
|--------|---------|-----|-------------|
| `parseKey()` | `std::optional<KeyComponents>` | `Result<KeyComponents>` | PARSE_FAILED, INVALID_SYNTAX |
| `extractTimestamp()` | `std::optional<int64_t>` | `Result<int64_t>` | PARSE_FAILED, INVALID_TYPE |
| `validateKey()` | `bool` | `Result<void>` | INVALID_SYNTAX, INVALID_REQUEST |
| `get()` | `std::optional<TSData>` | `Result<TSData>` | NOT_FOUND, PARSE_FAILED |
| `put()` | `bool` | `Result<void>` | INVALID_REQUEST, DISK_FULL |

#### Key Changes
- **Replace std::optional** - Provide error details instead of nullopt
- **Detailed parse errors** - Include position, expected vs actual
- **Timestamp validation** - Explicit overflow/underflow errors

#### Implementation Steps

1. **Update Signatures** (Day 1)
   - Change return types to `Result<T>`
   - Add error code documentation

2. **Migrate Parsing Logic** (Days 2-4)
   - Replace `return std::nullopt` with detailed errors
   - Add position information to parse errors
   - Validate timestamp ranges explicitly
   - Use `fromOptional()` helper where appropriate

3. **Update Call Sites** (Days 5-6)
   - Change from `if (!opt)` to `if (!result)`
   - Access values with `*result` instead of `*opt`
   - Handle specific error codes

4. **Testing** (Days 7-8)
   - Malformed key tests
   - Timestamp overflow tests
   - Edge case tests
   - Performance comparison

#### Acceptance Criteria
✅ All parsing returns `Result<T>`  
✅ Detailed error messages for parse failures  
✅ Timestamp validation explicit  
✅ All tests pass  
✅ Documentation updated

---

### Week 5-6: PluginManager Migration

**Priority:** 🟡 High (security-sensitive)

#### Files to Migrate
```
src/plugins/plugin_manager.cpp
include/plugins/plugin_manager.h
tests/test_plugin_manager.cpp
```

#### Methods to Convert (~12 methods)

**Pattern:** `nullptr → Result<T*>` (security-focused)

| Method | Current | New | Error Codes |
|--------|---------|-----|-------------|
| `loadPlugin()` | `IPlugin*` | `Result<IPlugin*>` | NOT_FOUND, LOAD_FAILED, INCOMPATIBLE, INVALID_SIGNATURE |
| `getPlugin()` | `IPlugin*` | `Result<IPlugin*>` | NOT_FOUND |
| `unloadPlugin()` | `bool` | `Result<void>` | NOT_FOUND |
| `verifySignature()` | `bool` | `Result<bool>` | INVALID_SIGNATURE, FILE_NOT_FOUND |
| `checkCompatibility()` | `bool` | `Result<bool>` | INCOMPATIBLE |

#### Security Focus
- **Signature failures** - Detailed error (expired, revoked, invalid)
- **Compatibility issues** - Version mismatches explicit
- **Load failures** - Distinguish malicious vs broken plugins

#### Implementation Steps

1. **Update Signatures** (Day 1)
   - Add security-focused error codes
   - Document threat model

2. **Migrate Load Logic** (Days 2-4)
   - Signature verification first
   - Compatibility checks second
   - Load with detailed error context
   - Add plugin name/version to all errors

3. **Update Call Sites** (Days 5-6)
   - Handle security errors specially
   - Log detailed information
   - Implement retry logic where appropriate

4. **Security Testing** (Days 7-8)
   - Test signature verification
   - Test incompatible plugins
   - Test malicious plugin detection
   - Security audit

#### Acceptance Criteria
✅ All plugin operations return `Result<T>`  
✅ Signature failures detailed  
✅ Security tests pass  
✅ Security documentation updated  
✅ Code review with security focus

---

### Week 6-7: GraphQL/AQL Parser Migration

**Priority:** 🟡 High (user-facing)

#### Files to Migrate
```
src/query/aql_parser.cpp
src/api/graphql.cpp
include/query/aql_parser.h
include/api/graphql.h
tests/test_aql_parser.cpp
tests/test_graphql.cpp
```

#### Methods to Convert (~8 methods)

**Pattern:** `nullptr/optional → Result<T>` (detailed parse errors)

| Method | Current | New | Error Codes |
|--------|---------|-----|-------------|
| `parseQuery()` | `ParsedQuery*` | `Result<ParsedQuery>` | PARSE_FAILED, INVALID_SYNTAX |
| `parseOperation()` | `Operation*` | `Result<Operation>` | PARSE_FAILED |
| `validateQuery()` | `bool` | `Result<void>` | INVALID_SYNTAX |

#### Key Changes
- **Remove custom ParseError struct** - Use standard Error
- **Line/column information** - Include in error context
- **Syntax highlighting** - Show error position in query

#### Implementation Steps

1. **Remove Custom Error Types** (Day 1)
   - Delete ParseError struct
   - Update to use standard Error

2. **Migrate Parsing** (Days 2-4)
   - Add line/column tracking
   - Include query excerpt in errors
   - Add suggestions for common mistakes

3. **Update Call Sites** (Days 5-6)
   - Format errors for user display
   - Include query context in logs

4. **Testing** (Days 7-8)
   - Syntax error tests
   - Large query tests
   - Performance tests

#### Acceptance Criteria
✅ Custom ParseError removed  
✅ All parsing returns `Result<T>`  
✅ Error messages include line/column  
✅ All tests pass  
✅ User-friendly error formatting

---

### Week 7-8: API Layer & Documentation

**Priority:** 🔴 Critical (user-facing)

#### API Error Translation

**HTTP Status Code Mapping:**
```cpp
ErrorCode → HTTP Status
ERR_API_INVALID_REQUEST → 400 Bad Request
ERR_API_UNAUTHORIZED → 401 Unauthorized
ERR_API_RATE_LIMIT → 429 Too Many Requests
ERR_STORAGE_FILE_NOT_FOUND → 404 Not Found
ERR_STORAGE_DISK_FULL → 507 Insufficient Storage
ERR_API_INTERNAL_ERROR → 500 Internal Server Error
```

**gRPC Status Code Mapping:**
```cpp
ErrorCode → gRPC Status
ERR_API_INVALID_REQUEST → INVALID_ARGUMENT
ERR_API_UNAUTHORIZED → UNAUTHENTICATED
ERR_API_RATE_LIMIT → RESOURCE_EXHAUSTED
ERR_STORAGE_FILE_NOT_FOUND → NOT_FOUND
ERR_API_INTERNAL_ERROR → INTERNAL
```

**Error Response Format (JSON):**
```json
{
  "error": {
    "code": 1000,
    "category": "Storage",
    "message": "File not found: /data/config.yaml",
    "severity": "Error",
    "solution": "Verify the file path in configuration...",
    "docs": ["/docs/configuration.md"]
  }
}
```

#### Implementation Tasks

1. **HTTP Handlers** (Days 1-2)
   - Update all HTTP handlers to return `Result<Response>`
   - Map errors to HTTP status codes
   - Format error responses

2. **gRPC Services** (Days 3-4)
   - Update RPC implementations to return `Result<T>`
   - Map errors to gRPC status codes
   - Include error metadata in status

3. **GraphQL Resolvers** (Days 5-6)
   - Update resolvers to return `Result<Value>`
   - Format GraphQL errors with extensions
   - Include error metadata

4. **Documentation** (Days 7-8)
   - Update API documentation with error codes
   - Create error catalog for API users
   - Add best practices guide
   - Update contributor guidelines

#### Testing Strategy
- [ ] API integration tests updated
- [ ] Error response format tests
- [ ] Status code mapping tests
- [ ] End-to-end tests
- [ ] API documentation review

#### Documentation Deliverables
- [ ] API Error Catalog (`docs/api/error_codes.md`)
- [ ] Error Handling Best Practices (`docs/dev/error_handling.md`)
- [ ] Migration Guide Updates
- [ ] Contributor Guidelines Updates
- [ ] Team Training Materials

---

## 📊 Success Metrics

### Coverage Targets
- ✅ 20-30 high-value methods migrated (~10% of codebase)
- ✅ 100% of user-facing APIs use `Result<T>`
- ✅ 100% of I/O operations use `Result<T>`
- ✅ 100% of parser functions use `Result<T>`

### Quality Targets
- ✅ All tests pass (unit, integration, e2e)
- ✅ No performance regression (< 5% on any operation)
- ✅ Error context preserved in 100% of cases
- ✅ Code coverage maintained or improved
- ✅ Zero P0/P1 bugs in error handling

### Developer Experience
- ✅ Team trained on `Result<T>` patterns
- ✅ Zero confusion about error handling in new code
- ✅ Positive team feedback (>80% satisfaction)
- ✅ Migration guide validated and refined

---

## 🧪 Testing Strategy

### Unit Testing
**For each migrated method:**
- [ ] Test success path returns Ok(value)
- [ ] Test each error scenario returns correct error code
- [ ] Test error context includes relevant information
- [ ] Test error metadata is correct
- [ ] Test monadic operations (and_then, value_or)

### Integration Testing
**For each module:**
- [ ] Test error propagation across module boundaries
- [ ] Test error translation at API boundaries
- [ ] Test backward compatibility with Phase 4 legacy code
- [ ] Test error handling under load

### Performance Testing
**Benchmarks:**
- [ ] Measure migrated vs legacy error-free path (< 1% diff)
- [ ] Measure error path overhead (should be < 5% vs legacy)
- [ ] Profile error-heavy scenarios (no memory leaks)
- [ ] Stress test concurrent error conditions

### Security Testing
- [ ] Verify no sensitive data in error messages
- [ ] Test error handling under attack scenarios
- [ ] Validate error codes cannot be manipulated
- [ ] Audit plugin signature verification
- [ ] Test error injection attacks

---

## 🚨 Risk Management

### Technical Risks

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Performance regression | High | Medium | Continuous benchmarking, rollback plan |
| Breaking API changes | Critical | Low | Maintain backward compatibility, versioning |
| Integration issues | High | Medium | Incremental migration, extensive testing |
| Team adoption challenges | Medium | Medium | Training, documentation, support |

### Mitigation Strategies

**Performance Regression:**
- Continuous benchmarking during migration
- Performance budgets per module
- Rollback plan if regression > 5%
- Profile and optimize hot paths

**Breaking Changes:**
- Maintain conversion helpers during Phase 3
- Keep legacy patterns working alongside new system
- Version APIs if necessary
- Gradual rollout with feature flags

**Integration Issues:**
- Incremental migration (one module at a time)
- Extensive integration testing
- Staged rollout to production
- Monitor error rates closely

**Team Adoption:**
- Comprehensive training before start
- Pair programming for first migrations
- Regular knowledge sharing sessions
- Document lessons learned

---

## 📅 Timeline & Milestones

### Week-by-Week Plan

| Week | Module | Key Deliverables | % Complete |
|------|--------|------------------|------------|
| **1** | Planning | Migration targets, estimates | 12.5% |
| **2-3** | IndexManager | 15 methods migrated, tests passing | 37.5% |
| **3-4** | ContentFS | 8 methods migrated, Status removed | 50% |
| **4-5** | TSStore | 10 methods migrated, parsing improved | 62.5% |
| **5-6** | PluginManager | 12 methods migrated, security hardened | 75% |
| **6-7** | Parsers | 8 methods migrated, error messages improved | 87.5% |
| **7-8** | API Layer + Docs | Error translation, documentation | 100% |

### Key Milestones

**M1: Planning Complete (End of Week 1)**
- [ ] Migration targets identified and prioritized
- [ ] Effort estimates validated
- [ ] Team trained and ready
- [ ] Development environment set up

**M2: 25% Complete (End of Week 3)**
- [ ] IndexManager fully migrated
- [ ] All IndexManager tests passing
- [ ] First production validation
- [ ] Lessons learned documented

**M3: 50% Complete (End of Week 4)**
- [ ] ContentFS fully migrated
- [ ] Status struct removed
- [ ] Integration tests passing
- [ ] Team feedback collected

**M4: 75% Complete (End of Week 6)**
- [ ] TSStore and PluginManager migrated
- [ ] Security audit passed
- [ ] Performance benchmarks acceptable
- [ ] Documentation in progress

**M5: Phase 3 Complete (End of Week 8)**
- [ ] All target modules migrated
- [ ] API layer error translation complete
- [ ] All tests passing
- [ ] Documentation complete
- [ ] Ready for Phase 4 planning

---

## 🔗 Dependencies & Blockers

### Prerequisites (All Complete ✅)
- [x] Phase 1-2 foundation merged
- [x] Error codes defined and documented
- [x] Unit tests for foundation
- [x] Migration examples created
- [x] Migration guide written

### External Dependencies
- [ ] Team availability (6-8 weeks dedicated time)
- [ ] Code review bandwidth
- [ ] QA resources for testing
- [ ] Documentation review

### Potential Blockers
- Performance issues requiring optimization
- Integration bugs requiring foundation fixes
- Team capacity constraints
- Production issues requiring attention

### Mitigation
- Buffer time in estimates (20% contingency)
- Parallel work streams where possible
- Clear escalation path for blockers
- Regular check-ins with stakeholders

---

## 📞 Team & Communication

### Roles & Responsibilities

**Project Lead:** TBD
- Overall Phase 3 coordination
- Milestone tracking
- Blocker resolution

**Tech Leads (per module):**
- IndexManager: TBD
- ContentFS: TBD
- TSStore: TBD
- PluginManager: TBD
- Parsers: TBD
- API Layer: TBD

**Support Roles:**
- Code Reviewer: TBD
- QA Lead: TBD
- Documentation Lead: TBD
- DevOps: TBD

### Communication Plan

**Daily:**
- Stand-ups (async or sync)
- Blocker escalation
- Progress updates

**Weekly:**
- Team sync meeting
- Demo of completed work
- Retrospective on challenges

**Bi-weekly:**
- Stakeholder update
- Metrics review
- Timeline adjustment if needed

**Milestone Reviews:**
- Deep dive on quality metrics
- Lessons learned
- Adjustments for remaining work

---

## 📚 References

**Foundation Documents:**
- `include/utils/expected.h` - Result<T> wrapper
- `include/utils/error_registry.h` - Error codes
- `ERROR_HANDLING_IMPLEMENTATION_SUMMARY.md` - Phase 1-2 summary
- `PHASE_1_2_VERIFICATION_REPORT.md` - Verification results
- `ERROR_HANDLING_PHASE_1_2_COMPLETE.md` - Completion summary

**Migration Resources:**
- `examples/migration/README.md` - Migration guide
- `examples/migration/index_manager_migration_example.cpp` - nullptr pattern
- `examples/migration/contentfs_migration_example.cpp` - Status pattern
- `examples/migration/tsstore_migration_example.cpp` - optional pattern

**Issue Templates:**
- `.github/ISSUE_TEMPLATE/error-handling-phase3.md` - This phase
- `.github/ISSUE_TEMPLATE/error-handling-phase4.md` - Next phase
- `.github/ISSUE_TEMPLATE/error-handling-meta.md` - Master tracking

---

## ✅ Definition of Done

Phase 3 is complete when:

**Code:**
- [ ] All 20-30 target methods migrated to `Result<T>`
- [ ] All legacy patterns removed from target modules
- [ ] All code reviewed and approved
- [ ] No compiler warnings

**Testing:**
- [ ] All unit tests passing
- [ ] All integration tests passing
- [ ] Performance benchmarks within 5% of baseline
- [ ] Security audit passed
- [ ] Code coverage ≥ 85%

**Documentation:**
- [ ] API documentation updated with error codes
- [ ] Migration guide refined based on learnings
- [ ] Best practices guide created
- [ ] Team training materials complete
- [ ] Contributor guidelines updated

**Validation:**
- [ ] Team trained and comfortable with patterns
- [ ] Production deployment successful
- [ ] No P0/P1 bugs in error handling
- [ ] Positive team feedback

**Handoff:**
- [ ] Phase 4 planning initiated
- [ ] Lessons learned documented
- [ ] Metrics and achievements recorded
- [ ] Celebration! 🎉

---

*Plan Created: 2026-01-19*  
*Next Update: Week 1 completion*  
*Contact: Project Lead TBD*
