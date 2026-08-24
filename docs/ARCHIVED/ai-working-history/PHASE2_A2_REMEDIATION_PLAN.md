# Phase 2: A2 Critical Findings Remediation Plan

**Timeline**: Aug 15-21, 2026  
**Effort**: 16 hours  
**Target**: Resolve all 11 critical findings in tensor module  

## Executive Summary

This plan addresses 11 findings from the Block A2 Diagnostic Audit:
- **2 CRITICAL** findings: Silent failures & missing diagnostics  
- **2 HIGH** findings: Error state propagation & error code range  
- **5 MEDIUM** findings: RocksDB, logging, context, concurrency, exceptions  
- **2 LOW** findings: Formatting & test coverage  

**Impact**: Current queries with 90% adapter failure rates appear to work; zero root-cause traceability; MTTR 4-8 hours.

**Target**: All 11 findings resolved with 100% test pass rate and production-ready code by Aug 20-21.

## Phase 2 Scope: 11 Findings

### CRITICAL-1: Silent Failure Cascade (4 hours)
**Location**: `src/tensor/tensor_fingerprint_graph.cpp` lines 267, 282–293, 311–320, 358  
**Issue**: 7 error paths silently return without diagnostics

**Affected Lines**:
1. Line 267: `if (!std::isfinite(query_self_ip))`
2. Line 282: `catch (...) { continue; }`
3. Line 283: `if (!std::isfinite(score))`
4. Line 311: `if (!std::isfinite(other_self_ip))`
5. Line 318: `if (!std::isfinite(cross_ip))`
6. Line 358: Entry path (TBD)
7. Additional paths in similarity computation

**Fix**: Add `emitDiagnostic(DiagnosticLevel::ERROR, "TENSOR-9501", ...)` before each return.

### CRITICAL-2: Missing Diagnostic Emission Infrastructure (6 hours)
**Locations**:
- `src/tensor/tensor_fingerprint_graph.cpp` (~6 paths)
- `src/tensor/tensor_index_manager.cpp` (~8 paths)  
- `src/tensor/tensor_core_bridge.cpp` (~4 paths)

**Issue**: 18 production error paths missing diagnostic context

**Fix**: Audit each file, identify error paths, add semantic error codes.

### HIGH-3: Incomplete Error State Propagation (2 hours)
**Location**: `src/tensor/tensor_index_manager.cpp` lines 323–334  
**Issue**: Three failure modes collapse to identical `nullptr`

**Distinct Error Codes**:
- `TENSOR-9510`: Graph construction failure
- `TENSOR-9511`: Index lookup failure
- `TENSOR-9512`: Adapter routing failure

### HIGH-4: Tensor Error Code Range Not Defined (1.5 hours)
**Location**: `include/utils/error_registry.h`  
**Issue**: TENSOR-specific error codes (9500–9599) not registered

**Code Ranges**:
- 9500-9509: Graph errors
- 9510-9519: Index errors
- 9520-9529: Adapter errors
- 9530-9539: Routing errors
- 9540-9549: Fingerprint errors
- 9550-9559: Verification errors
- 9560-9569: Recovery errors
- 9570-9579: (Reserved)
- 9580-9599: Future expansion

### MEDIUM-1: RocksDB Deletion Errors (30 min)
**Location**: Tensor core bridge write path  
**Fix**: Check `RocksDB::delete()` return status, emit diagnostic on failure

### MEDIUM-2: spdlog Integration Missing (30 min)
**Location**: `tensor_core_bridge::write()`  
**Fix**: Add spdlog logging at INFO/ERROR levels

### MEDIUM-3: Missing Diagnostic Context (30 min)
**Location**: `addAdapter()` method  
**Fix**: Call `emitDiagnostic()` on success/failure

### MEDIUM-4: Concurrent Diagnostics Not Atomic (1 hour)
**Location**: Shared diagnostic emission  
**Fix**: Use atomic operations or mutex guards for diagnostic state

### MEDIUM-5: Silent Exception Swallowing (30 min)
**Location**: `getRaw()` method  
**Fix**: Add try-catch with `emitDiagnostic()` for exception context

### LOW-1: Inconsistent Error Message Formatting (30 min)
**Location**: Error message templates across tensor module  
**Fix**: Standardize format strings and semantic structure

### LOW-2: Test Coverage Gap (1 hour)
**Location**: New test files  
**Fix**: Add 11 focused tests (P2-A2-01..11) for all findings

---

## Implementation Strategy

### Phase A: Foundation (2 hours)
1. **Define Error Registry**: Add TENSOR-9500..9599 range to `error_registry.h`
2. **Create Diagnostic Helpers**: Add tensor-specific emitDiagnostic wrapper
3. **Implement CRITICAL-1 fixes**: 7 paths in tensor_fingerprint_graph.cpp

### Phase B: Core Hardening (8 hours)
4. **Implement CRITICAL-2 fixes**: 18 paths across 3 files
5. **Implement HIGH-3 fixes**: Error propagation in tensor_index_manager.cpp
6. **Implement MEDIUM fixes** (1–5): RocksDB, logging, context, concurrency, exceptions

### Phase C: Testing & Documentation (6 hours)
7. **Create Test Suite**: 11 focused tests (P2-A2-01..11)
8. **Verify All Fixes**: Build, compile, run tests
9. **Document Remediation**: Complete A2_REMEDIATION_COMPLETION.md

---

## Deliverables Checklist

- [ ] Production Code: 4 files hardened
  - [ ] `include/utils/error_registry.h` (TENSOR codes)
  - [ ] `src/tensor/tensor_fingerprint_graph.cpp` (CRITICAL-1, CRITICAL-2)
  - [ ] `src/tensor/tensor_index_manager.cpp` (HIGH-3, CRITICAL-2)
  - [ ] `src/tensor/tensor_core_bridge.cpp` (CRITICAL-2, MEDIUM-1..5)

- [ ] Test Suite: 11 tests
  - [ ] P2-A2-01: CRITICAL-1 diagnostics
  - [ ] P2-A2-02: CRITICAL-2 diagnostics (fingerprint graph)
  - [ ] P2-A2-03: CRITICAL-2 diagnostics (index manager)
  - [ ] P2-A2-04: CRITICAL-2 diagnostics (core bridge)
  - [ ] P2-A2-05: HIGH-3 error propagation
  - [ ] P2-A2-06: HIGH-4 error code registration
  - [ ] P2-A2-07: MEDIUM-1 (RocksDB)
  - [ ] P2-A2-08: MEDIUM-2 (spdlog)
  - [ ] P2-A2-09: MEDIUM-3 (context)
  - [ ] P2-A2-10: MEDIUM-4 (concurrency)
  - [ ] P2-A2-11: MEDIUM-5 (exceptions)

- [ ] Documentation
  - [ ] ai_working/A2_REMEDIATION_COMPLETION.md
  - [ ] Before/after code snippets for each finding
  - [ ] Evidence links to test code
  - [ ] MTTR reduction estimate (target: 60 min from 4-8 hours)

---

## Quality Standards

✅ No stubs/mocks in production code  
✅ All production code fully functional  
✅ 100% pass rate on all tests  
✅ No breaking API changes  
✅ ThreadSanitizer clean  
✅ C++17 compliant  

---

## Timeline

| Phase | Task | Hours | Target Date |
|-------|------|-------|-------------|
| A | Foundation (error registry, CRITICAL-1) | 2 | Aug 15 |
| B | Core hardening (18 paths, HIGH-3, MEDIUM) | 8 | Aug 18 |
| C | Testing & documentation | 6 | Aug 20 |
| **Total** | **All 11 findings** | **16** | **Aug 20-21** |

---

## Entry Criteria

- ✅ All A1 concurrent hardening tests PASS
- ✅ Tensor module compiles on all presets
- ✅ Stream A completion summary available

## Exit Criteria

- ✅ All 11 findings resolved with passing tests (P2-A2-01..11)
- ✅ Production code production-ready (zero stubs)
- ✅ Documentation complete with evidence links
- ✅ Ready for Phase 3 integration testing
- ✅ No new regressions vs baseline

## Success Metrics

- ✅ 0 silent failures (all error paths emit diagnostics)
- ✅ 11/11 findings resolved
- ✅ 100% test pass rate
- ✅ Zero CRITICAL/HIGH findings in remediation code
- ✅ MTTR reduction documented (target: 60 min from current 4-8 hours)

---

**Status**: PLANNING  
**Updated**: 2026-08-07T15:42:27Z
