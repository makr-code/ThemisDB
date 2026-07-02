# Sprint 6 Phase 2: Batch B (Format String + ReDoS) - Kickoff Brief

**Date:** 2026-07-09 (Week 29, planned)  
**Predecessor:** Sprint 5 XXE Batch A (2026-07-02) ✅ COMPLETE  
**Target:** Format String (CWE-134) + ReDoS (CWE-1333) remediation  
**Gap Count:** 202 total (93 format strings, 109 ReDoS)

---

## Deliverables Ready from Sprint 6 Phase 1

### SafeFormat Library ✅
- **Header:** `include/security/safe_format.h` (170 lines)
- **Implementation:** `src/security/safe_format.cpp` (70 lines)
- **Functions:**
  - `printf_safe()` - Type-safe printf with fmt library
  - `snprintf_safe()` - Buffer-safe sprintf
  - `fprintf_safe()` - File stream safe formatting
  - `format_safe()` - String formatting
  - `log_user_message()` - User input logging with escaping

### SafeRegex Library ✅
- **Header:** `include/security/safe_regex.h` (200 lines)
- **Implementation:** `src/security/safe_regex.cpp` (280 lines)
- **Features:**
  - `match()` - Full string matching with timeout
  - `search()` - Substring search with timeout
  - `replace()` - Pattern replacement with timeout
  - `split()` - String splitting by pattern
  - `is_pattern_safe()` - Pattern complexity validation
  - `validate_input()` - Input length/repetition checks
  - LRU pattern cache for performance
  - Cache statistics tracking

**Status:** Phase 1 complete, ready for Phase 2 integration

---

## Implementation Strategy for Phase 2

### Task 1: Format String Remediation (93 gaps)
1. Identify top-risk 50 gaps (user input → format string sink)
2. Replace with SafeFormat::format_safe() or SafeFormat::log_user_message()
3. Add regression tests with format string attack payloads
4. Verify backward compatibility

### Task 2: ReDoS Remediation (109 gaps)
1. Identify high-complexity regex patterns
2. Apply SafeRegex::match() or SafeRegex::search() wrappers
3. Implement timeout configuration (1-5 seconds per pattern)
4. Add ReDoS regression tests (exponential expansion payloads)
5. Monitor cache hit rates

### Target Modules
- **Format String:** query, security, analytics
- **ReDoS:** query, security, analytics (overlapping modules)

### Success Criteria
- ✓ Top 50 format string gaps remediated
- ✓ Top 50 ReDoS gaps remediated
- ✓ All remediations verified with attack payloads
- ✓ Zero regressions in logging, regex functionality
- ✓ Backward compatible (API unchanged)

---

## Phase 1-4 Remediation Progress Tracker

| Sprint | Batch | Target | Deadline | Status |
|--------|-------|--------|----------|--------|
| Sprint 5 | A: XXE | 783 gaps | 2026-07-08 | ✅ COMPLETE |
| Sprint 6 | B: Format/ReDoS | 202 gaps | 2026-07-15 | 🎯 NEXT |
| Sprint 7 | C: Iterator | 134 gaps | 2026-07-22 | ⏳ Planned |
| Sprint 8 | D: Move | 97 gaps | 2026-07-29 | ⏳ Planned |
| Sprint 9 | E: Concurrency | 20 gaps | 2026-08-05 | ⏳ Planned |

**Cumulative Target:** 1,236 gaps → 50% remediated by 2026-08-31 v1.5.0

---

## Notes for Sprint 6 Implementation Agent

1. **Reuse infrastructure:** Both SafeFormat and SafeRegex already exist; focus on integration
2. **Test payloads ready:** Format string attack vectors and ReDoS patterns documented
3. **Backward compatibility:** All replacements should be transparent (same API signatures)
4. **Batch preference:** Per @makr-code preference, coordinate as larger batch commit
5. **Parallel initiatives:** Graph Module Phase 2.2-2.4 continues independently

---

*Prepared: 2026-07-02 (Sprint 5 completion)*  
*For execution: Week 29, 2026-07-09*
