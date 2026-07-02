# Sprint 6 Phase 2: Batch B Remediation Plan
## Format String + ReDoS Vulnerabilities

**Date:** 2026-07-02  
**Status:** PLANNING → EXECUTION  
**Target Release:** v1.5.0 (Q3 2026)  
**Timeline:** 2026-07-09 to 2026-07-15 (Week 29)

---

## Executive Summary

Implement remediation for 202 Format String (CWE-134) and ReDoS (CWE-1333) vulnerabilities using existing SafeFormat and SafeRegex libraries.

**Batch B Breakdown:**
- **Format String Vulnerabilities (CWE-134):** 93 gaps
- **ReDoS Vulnerabilities (CWE-1333):** 109 gaps
- **Primary Modules:** query, security, analytics
- **Secondary Modules:** api, server, utils

---

## Phase B1: Format String Vulnerabilities (93 gaps)

### Approach
1. **Identify all vulnerable patterns:**
   - `printf(user_input)` or `printf(user_input.c_str())`
   - `sprintf(buffer, user_input)`
   - `syslog(user_input)`
   - `log(user_input)` (custom logging)

2. **Apply remediation using SafeFormat:**
   - Replace `printf(user_input)` → `SafeFormat::print_string(user_input)`
   - Replace `printf(format, user_var)` → `SafeFormat::printf_safe(format, user_var)`
   - Update all sprintf calls with bounds checking

3. **Verification:**
   - Format string tests cover all replacements
   - No format specifiers interpreted from user input

### High-Risk Modules (Priority 1)
- `src/query/query_planner.cpp` - Query plan logging (estimated 15 gaps)
- `src/security/audit_logger.cpp` - Security audit logging (estimated 12 gaps)
- `src/analytics/event_processor.cpp` - Event stream logging (estimated 10 gaps)

### Medium-Risk Modules (Priority 2)
- `src/api/rest_handler.cpp` (8 gaps)
- `src/server/http_server.cpp` (7 gaps)
- `src/utils/logger.cpp` (6 gaps)

### Low-Risk Modules (Priority 3)
- Remaining modules: ~20 gaps distributed

---

## Phase B2: ReDoS Vulnerabilities (109 gaps)

### Approach
1. **Identify vulnerable regex patterns:**
   - Nested quantifiers: `(a+)+`, `(a*)*`
   - Overlapping alternation: `(a|a)*`, `(a|ab)*`
   - Complex backtracking patterns
   - User-controlled patterns

2. **Apply remediation using SafeRegex:**
   - Simplify patterns (remove nested quantifiers)
   - Add timeout (default 5 seconds, configurable per pattern)
   - Pre-validate input length before regex matching
   - Cache compiled patterns

3. **Verification:**
   - ReDoS regression tests with pathological inputs
   - Performance benchmarks show no degradation
   - Timeout mechanism works as expected

### High-Risk Modules (Priority 1)
- `src/query/query_validator.cpp` - Query pattern validation (estimated 18 gaps)
- `src/security/input_validator.cpp` - Input sanitization (estimated 15 gaps)
- `src/analytics/filter_engine.cpp` - Filter pattern matching (estimated 12 gaps)

### Medium-Risk Modules (Priority 2)
- `src/api/request_parser.cpp` (10 gaps)
- `src/search/search_engine.cpp` (9 gaps)
- `src/importers/csv_parser.cpp` (8 gaps)

### Low-Risk Modules (Priority 3)
- Remaining modules: ~37 gaps distributed

---

## Implementation Strategy

### Execution Order (User Preference: Larger Batches)
1. **Phase B1 - Format String:** Implement top 50 format string gaps (query, security, analytics)
2. **Phase B2 - ReDoS:** Implement top 50 ReDoS gaps (same modules)
3. **Testing & Verification:** All new tests pass, no regressions
4. **Single Coordinated Commit:** All changes in one commit (user preference for larger batches)

### Files to Modify (Planned)

**Format String (B1):**
- src/query/query_planner.cpp (+15)
- src/security/audit_logger.cpp (+12)
- src/analytics/event_processor.cpp (+10)
- src/api/rest_handler.cpp (+8)
- src/server/http_server.cpp (+7)
- Total: ~52 lines modified for top 50 gaps

**ReDoS (B2):**
- src/query/query_validator.cpp (+18)
- src/security/input_validator.cpp (+15)
- src/analytics/filter_engine.cpp (+12)
- src/api/request_parser.cpp (+10)
- src/search/search_engine.cpp (+9)
- Total: ~64 lines modified for top 50 gaps

**Tests:**
- tests/security/test_safe_format.cpp (+30 new cases)
- tests/security/test_safe_regex.cpp (+30 new cases)
- Total: ~60 lines added

**Overall Impact:** ~176 lines modified/added

---

## Acceptance Criteria

### Format String Phase (B1)
- [x] Identify all 93 format string gaps
- [ ] Remediate top 50 (prioritized by risk)
- [ ] Update all #include statements to use SafeFormat
- [ ] All format string tests pass
- [ ] Zero format specifier interpretation from user input
- [ ] Backward compatibility maintained

### ReDoS Phase (B2)
- [x] Identify all 109 ReDoS gaps
- [ ] Remediate top 50 (prioritized by risk)
- [ ] Simplify regex patterns (remove nested quantifiers)
- [ ] Add timeout mechanism to all user-pattern matching
- [ ] All ReDoS regression tests pass
- [ ] Performance benchmarks show <5% overhead
- [ ] Backward compatibility maintained

### Overall Quality Gates
- [x] Code compiles without errors
- [ ] All new tests pass
- [ ] No regressions in existing tests
- [ ] Security audit passes (no new vulnerabilities introduced)
- [ ] Code follows project C++ best practices (RAII, const-correctness)
- [ ] Documentation updated for SafeFormat/SafeRegex usage

---

## Testing Strategy

### Unit Tests
**safe_format_integration_test.cpp:**
- Test 20+ format string remediation patterns
- Verify all user input is escaped/bounded
- Test edge cases (empty strings, special chars, very long strings)

**safe_regex_integration_test.cpp:**
- Test 20+ ReDoS remediation patterns
- Verify nested quantifiers are removed
- Test timeout mechanism with pathological inputs
- Performance regression tests (acceptable overhead <5%)

### Integration Tests
- End-to-end query planner logging (no security issues)
- Audit logger with various event types
- Event processor with high-volume streams
- Query validator with malicious patterns

### Regression Tests
- All existing format string tests still pass
- All existing regex pattern tests still pass
- No functional behavior changes from user perspective

---

## Timeline

| Phase | Duration | Effort | Status |
|-------|----------|--------|--------|
| **Planning** | 2026-07-02 | 30 min | ✅ COMPLETE |
| **Implementation (B1+B2)** | 2026-07-03 to 2026-07-05 | 120 min | PENDING |
| **Testing** | 2026-07-06 to 2026-07-07 | 60 min | PENDING |
| **Verification & Merge** | 2026-07-08 | 30 min | PENDING |
| **Buffer (if needed)** | 2026-07-09 to 2026-07-15 | Flex | PENDING |

**Total Effort:** ~240 minutes (~4 hours)

---

## Success Metrics

1. **Gap Reduction:** 100 gaps remediated (50 format string + 50 ReDoS) = 8% overall reduction
2. **Security Impact:** 0 new vulnerabilities introduced
3. **Code Quality:** 0 compiler warnings, 100% test coverage for changes
4. **Performance:** <5% overhead vs baseline
5. **Developer Velocity:** Coordinated batch commit maintains merge velocity

---

## Risk Assessment

### Low Risk
- SafeFormat/SafeRegex already production-ready and tested
- Top modules are well-understood (query, security, analytics)
- Backward compatibility guaranteed (API unchanged)

### Mitigation Strategies
- Run full test suite after each module batch
- Code review against CWE-134 and CWE-1333 patterns
- Manual verification of hardcoded patterns (no user input)

---

## Dependencies

✅ **Already Available:**
- SafeFormat library (include/security/safe_format.h)
- SafeRegex library (include/security/safe_regex.h)
- Test infrastructure (GTest framework)
- Gap scanner v3 with S-3 pattern detection

**No External Dependencies Required**

---

## Deliverables

1. ✅ Remediation plan (this document)
2. Remediated source code (B1 + B2 modules)
3. Enhanced test suite (format string + ReDoS tests)
4. Sprint 6 Phase 2 completion report
5. Updated CHANGELOG entry

---

## Next Steps

1. **Immediate:** Begin Phase B1 format string remediation
2. **Parallel:** Prepare Phase B2 ReDoS remediation
3. **Week 29:** Complete all remediation, testing, and merge
4. **Post-Merge:** Update ROADMAP.md with Batch B completion
5. **Week 30:** Launch Sprint 7 - Batch C (Iterator Invalidation)

---

*Plan created: 2026-07-02*  
*Status: Ready for Implementation*  
*Coordinator: Copilot Coding Agent*
