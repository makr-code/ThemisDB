---
name: "Error Handling Migration - Meta Tracking"
about: Master tracking issue for complete error handling migration
title: "[Error Handling] Meta: Complete Migration to tl::expected"
labels: priority:P1, type:epic, area:core, phase:meta
assignees: ''
---

# 🎯 Error Handling Migration - Master Tracking Issue

**Project Status:** 🟢 Phase 1-2 Complete | 🟡 Phase 3-4 In Planning  
**Total Duration:** 18-22 weeks across 4 phases  
**Start Date:** 2026-01-19  
**Target Completion:** Q2 2026

---

## 📋 Executive Summary

ThemisDB currently uses three inconsistent error patterns across ~300+ error sites:
1. `return nullptr` - No error context
2. `Status{ok, message}` - No machine-readable codes
3. `std::optional` - No error information

**Goal:** Migrate to unified `tl::expected<T, E>` error handling system providing:
- ✅ Type-safe error propagation (zero overhead)
- ✅ Structured error codes with metadata
- ✅ Rich error context
- ✅ Composable error handling (monadic operations)
- ✅ Forward-compatible with C++23 std::expected

---

## 🗺️ Project Phases

### ✅ Phase 1 & 2: Foundation (COMPLETED - 2026-01-19)

**Status:** 🟢 Merged  
**PR:** #XXX  
**Duration:** 2 weeks  
**Deliverables:**
- [x] `tl-expected` dependency added to vcpkg.json
- [x] `include/utils/expected.h` - Result<T> wrapper and Error class
- [x] 16 new error codes (Index, Query, API, Plugin)
- [x] 62 total error codes across 10 categories
- [x] Comprehensive unit tests (`tests/test_expected.cpp`)
- [x] 3 migration examples (900+ lines)
- [x] Migration guide (`examples/migration/README.md`)
- [x] Implementation summary (`ERROR_HANDLING_IMPLEMENTATION_SUMMARY.md`)

**Key Achievements:**
- Zero-overhead error handling foundation ✅
- Type-safe error propagation ✅
- Conversion helpers for legacy patterns ✅
- Comprehensive documentation ✅

**Metrics:**
- Files Added: 6
- Files Modified: 4
- Lines Added: ~2,280
- Test Coverage: 20+ test cases
- Error Codes: 62 total (16 new)

---

### 🟡 Phase 3: High-Value Code Paths (PLANNED)

**Status:** ⚪ Not Started  
**Issue:** #YYY  
**Priority:** P1 (High)  
**Duration:** 6-8 weeks  
**Target Start:** After Phase 1-2 merge

**Scope:** Migrate 20-30 high-traffic methods (~10% of total)

**Target Modules:**
- [ ] **IndexManager** (Week 2-3)
  - 15 methods: createSecondaryIndex, getSecondaryIndex, etc.
  - Pattern: `nullptr` → `Result<T*>`
  
- [ ] **ContentFS** (Week 3-4)
  - 8 methods: put, get, del, exists, list
  - Pattern: `Status{ok, msg}` → `Result<T>`
  
- [ ] **TSStore** (Week 4-5)
  - 10 methods: parseKey, extractTimestamp, validateKey, etc.
  - Pattern: `std::optional` → `Result<T>`
  
- [ ] **PluginManager** (Week 5-6)
  - 12 methods: loadPlugin, getPlugin, verifySignature, etc.
  - Pattern: `nullptr` → `Result<T*>`
  
- [ ] **GraphQL Parser** (Week 6-7)
  - 8 methods: parseQuery, parseOperation, etc.
  - Pattern: `nullptr/optional` → `Result<T>`
  
- [ ] **API Layer** (Week 7-8)
  - 20+ endpoints: HTTP, gRPC, GraphQL
  - Pattern: Mixed → `Result<T>`

**Success Metrics:**
- 20-30 high-value methods migrated
- All user-facing APIs use Result<T>
- All I/O operations use Result<T>
- No performance regression (< 5%)
- Error context preserved in 100% of cases

**Dependencies:**
- Phase 1 & 2 merged ✅
- Team trained on Result<T> patterns
- Migration guide reviewed

---

### ⚪ Phase 4: Full Migration (PLANNED)

**Status:** ⚪ Not Started  
**Issue:** #ZZZ  
**Priority:** P2 (Medium)  
**Duration:** 10-12 weeks  
**Dependencies:** Phase 3 completion

**Scope:** Migrate remaining ~270+ error sites (~90% of total)

**Target Modules:**
- [ ] **Storage Layer** (Week 2-3) - 50 sites
  - RocksDB wrapper, Blob storage, Cache layer
  
- [ ] **Network Layer** (Week 4) - 40 sites
  - HTTP client/server, WebSocket, gRPC
  
- [ ] **LLM/LoRA** (Week 5-7) - 60 sites
  - Inference engine, Adapter management, Training
  
- [ ] **Query Engine** (Week 8) - 30 sites
  - AQL executor, Query planner, Aggregations
  
- [ ] **Schema Management** (Week 9) - 20 sites
  - Validator, Type checker, Migration manager
  
- [ ] **Utilities** (Week 10) - 40 sites
  - File utils, Crypto, Compression
  
- [ ] **Cleanup** (Week 11)
  - Remove legacy patterns
  - Deprecate conversion helpers
  - Update documentation
  
- [ ] **Validation** (Week 12)
  - Full test suite
  - Performance benchmarks
  - Security audit

**Success Metrics:**
- 100% of codebase using Result<T>
- 0 remaining legacy error patterns
- All tests pass
- No performance regression
- Code coverage ≥ 85%

---

## 📊 Overall Progress Tracking

### Coverage Metrics

```
Phase           | Sites Migrated | % Complete | Status
----------------|----------------|------------|--------
Foundation      | 0 (infra only) | N/A        | ✅ Done
High-Value      | 0 / ~30        | 0%         | ⚪ Planned
Full Migration  | 0 / ~270       | 0%         | ⚪ Planned
----------------|----------------|------------|--------
TOTAL           | 0 / ~300       | 0%         | 🟡 In Progress
```

### Module Migration Status

```
Module          | Total Sites | Migrated | Status
----------------|-------------|----------|--------
IndexManager    | 15          | 0        | ⚪ Phase 3
ContentFS       | 8           | 0        | ⚪ Phase 3
TSStore         | 10          | 0        | ⚪ Phase 3
PluginManager   | 12          | 0        | ⚪ Phase 3
GraphQL/AQL     | 8           | 0        | ⚪ Phase 3
API Layer       | 20          | 0        | ⚪ Phase 3
Storage         | 50          | 0        | ⚪ Phase 4
Network         | 40          | 0        | ⚪ Phase 4
LLM/LoRA        | 60          | 0        | ⚪ Phase 4
Query Engine    | 30          | 0        | ⚪ Phase 4
Schema Mgmt     | 20          | 0        | ⚪ Phase 4
Utilities       | 40          | 0        | ⚪ Phase 4
Other           | ~17         | 0        | ⚪ Phase 4
----------------|-------------|----------|--------
TOTAL           | ~300        | 0        | 0%
```

### Error Code Coverage

```
Category        | Error Codes | Status
----------------|-------------|--------
Storage         | 4           | ✅ Complete
LLM             | 11          | ✅ Complete
LoRA            | 7           | ✅ Complete
MCP             | 5           | ✅ Complete
Schema          | 3           | ✅ Complete
Network         | 3           | ✅ Complete
Index           | 4           | ✅ Complete (NEW)
Query           | 4           | ✅ Complete (NEW)
API             | 4           | ✅ Complete (NEW)
Plugin          | 4           | ✅ Complete (NEW)
----------------|-------------|--------
TOTAL           | 62          | ✅ Foundation Complete
```

---

## 🎯 Key Milestones

### M1: Foundation Complete ✅
- **Date:** 2026-01-19
- **Status:** ✅ Achieved
- **Deliverables:**
  - [x] tl::expected infrastructure
  - [x] Error codes and registry
  - [x] Unit tests
  - [x] Migration examples
  - [x] Documentation

### M2: Phase 3 - 25% Complete
- **Target:** Week 4 of Phase 3
- **Criteria:**
  - IndexManager + ContentFS migrated
  - All tests passing
  - No performance regression

### M3: Phase 3 - 50% Complete
- **Target:** Week 6 of Phase 3
- **Criteria:**
  - TSStore + PluginManager migrated
  - API error handling patterns established

### M4: Phase 3 Complete
- **Target:** Week 8 of Phase 3
- **Criteria:**
  - All high-value paths migrated
  - Documentation updated
  - Team trained

### M5: Phase 4 - 50% Complete
- **Target:** Week 6 of Phase 4
- **Criteria:**
  - Storage, Network, LLM/LoRA migrated
  - ~150 sites converted

### M6: Phase 4 - 90% Complete
- **Target:** Week 10 of Phase 4
- **Criteria:**
  - All modules migrated
  - Cleanup in progress

### M7: Full Migration Complete 🎉
- **Target:** End of Phase 4
- **Criteria:**
  - 100% of codebase using Result<T>
  - All legacy patterns removed
  - Full validation passed

---

## 🧪 Quality Assurance

### Testing Strategy

**Phase 1-2:**
- [x] Unit tests for Result<T> wrapper
- [x] Unit tests for Error class
- [x] Unit tests for conversion helpers
- [x] Monadic operations tests

**Phase 3:**
- [ ] Update module-specific unit tests
- [ ] Integration tests for error propagation
- [ ] Performance benchmarks
- [ ] Security tests

**Phase 4:**
- [ ] Comprehensive test suite
- [ ] Regression tests
- [ ] Load tests
- [ ] Security audit

### Performance Validation

**Benchmarks:**
- Error-free path: Should match legacy (< 1% difference)
- Error path: Should be faster than exceptions
- Memory usage: No increase expected
- Compilation time: Minimal impact (header-only)

**Targets:**
- No regression > 5% in any hot path
- Error path faster than exceptions
- Zero heap allocations on error path

### Security Review

**Checks:**
- [ ] No sensitive data in error messages
- [ ] Error codes cannot be manipulated
- [ ] Error handling resilient to attacks
- [ ] Input validation in error paths

---

## 📚 Documentation

### Completed
- [x] Migration guide (`examples/migration/README.md`)
- [x] Implementation summary (`ERROR_HANDLING_IMPLEMENTATION_SUMMARY.md`)
- [x] Error registry documentation
- [x] Unit test examples
- [x] 3 migration pattern examples

### Planned
- [ ] API documentation updates (Phase 3)
- [ ] Best practices guide (Phase 3)
- [ ] Troubleshooting guide (Phase 3)
- [ ] Error catalog (Phase 4)
- [ ] Final migration report (Phase 4)

---

## 🚨 Risks & Mitigation

### Risk: Breaking Changes
**Severity:** High  
**Mitigation:**
- Maintain backward compatibility during migration
- Conversion helpers for gradual transition
- Feature flags for new error handling
- Comprehensive testing

### Risk: Performance Regression
**Severity:** High  
**Mitigation:**
- Continuous benchmarking
- Profile hot paths
- Rollback if regression > 5%
- Zero-overhead design

### Risk: Team Adoption
**Severity:** Medium  
**Mitigation:**
- Comprehensive training
- Clear migration guide
- Code review support
- Regular sync meetings

### Risk: Incomplete Migration
**Severity:** Medium  
**Mitigation:**
- Automated pattern detection
- CI/CD checks
- Progress tracking
- Clear success metrics

---

## 📈 Success Criteria

### Technical
- [ ] 100% of codebase using Result<T>
- [ ] 0 remaining legacy error patterns
- [ ] All tests passing
- [ ] No performance regression
- [ ] Code coverage ≥ 85%

### Developer Experience
- [ ] Single, consistent error pattern
- [ ] Clear error messages
- [ ] Easy error recovery
- [ ] Comprehensive documentation

### Business Value
- [ ] Faster debugging (structured errors)
- [ ] Better reliability (type-safe)
- [ ] Easier maintenance (unified pattern)
- [ ] Forward compatibility (C++23)

---

## 🔗 Related Issues

**Phase Issues:**
- #XXX - Phase 1 & 2: Foundation (✅ Complete)
- #YYY - Phase 3: High-Value Paths (⚪ Planned)
- #ZZZ - Phase 4: Full Migration (⚪ Planned)

**Dependencies:**
- None - Self-contained project

**References:**
- Error Registry: `include/utils/error_registry.h`
- Expected Wrapper: `include/utils/expected.h`
- Migration Guide: `examples/migration/README.md`
- Test Suite: `tests/test_expected.cpp`

---

## 📞 Project Team

**Owner:** TBD  
**Contributors:** Development Team  
**Reviewers:** Senior Engineers  
**Stakeholders:** All ThemisDB developers

---

## 💬 Communication

**Updates:**
- Weekly progress in team meeting
- Bi-weekly status report
- Monthly demo of completed phases

**Channels:**
- GitHub Issues for tracking
- Team chat for discussions
- Wiki for documentation

---

## 📅 Timeline

```
Q1 2026:  Phase 1-2 Complete ✅
Q2 2026:  Phase 3 (6-8 weeks)
Q2-Q3 2026: Phase 4 (10-12 weeks)
Q3 2026:  Project Complete 🎉
```

---

*Last Updated: 2026-01-19*  
*Next Review: Start of Phase 3*
