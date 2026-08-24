# EPIC-003: Phase 6 Extended Scanner (5 New Patterns, 1.500-2.500 Gaps)

**Status:** 🔵 Planned  
**Target Release:** v2.0.0 Stable (2026-10-31)  
**Timeline:** 8 Weeks (W40-W47, 2026-10-06 to 2026-11-23)  
**Owner:** Code Quality Scanner Team  
**Scope:** +5 New Scanner Categories, ~2.000 New Gaps Discovered

## Epic Summary

Extend the Phase 1-5 scanner portfolio (27 categories) with 5 new detection patterns targeting critical code quality gaps. Phase 6 adds 48-55 new patterns across Type Conversion, Input Validation, Exception Safety, Uninitialized Variables, and OOP Design domains.

**Expected Impact:**
- **New Scanners:** 5 categories
- **New Patterns:** 48-55 rules
- **Estimated Gaps:** 1.500-2.500 (target: 2.000)
- **FP Rate Target:** < 15% (validation via manual spot-checks)
- **Portfolio Growth:** 27 → 32 scanner categories

## Success Criteria

- ✅ **5 New Scanners Fully Implemented:** Type Conversion, Input Validation, Exception Safety, Uninitialized Vars, OOP Design
- ✅ **1.500-2.500 New Gaps Discovered:** Target 2.000 on themis_core scope
- ✅ **FP Rate < 15%:** Validation through manual review of 100+ random samples
- ✅ **CI/CD Integration:** Automated nightly scan with reports
- ✅ **Zero Performance Regression:** < 15% overhead on standard build
- ✅ **Documentation Complete:** Scanner guide, pattern specs, usage examples

## Key Deliverables

| Scanner | Patterns | Est. Gaps | FP Rate | Effort |
|---------|----------|-----------|---------|--------|
| **Type Conversion** | 8-10 | 300-500 | < 15% | 30h |
| **Input Validation** | 10-12 | 400-600 | < 12% | 35h |
| **Exception Safety** | 8-10 | 300-450 | < 18% | 30h |
| **Uninitialized Vars** | 8-10 | 350-500 | < 20% | 30h |
| **OOP Design** | 14-18 | 450-700 | < 25% | 35h |
| **Integration & CI/CD** | — | — | — | 20h |

## Dependencies

- **Prerequisite:** Phase 1-5 scanner pipeline stable (22.085 deduplicated findings)
- **Parallel:** EPIC-002 Module Hardening (starts W35, overlaps weeks 40-42)
- **Supports:** v2.0.0 release (extends gap inventory, improves coverage)

## Sub-Issues

### [003-A] Type Conversion Safety Scanner (8-10 Patterns)
- **Title:** `Scanner: Type Conversion & Implicit Cast Detection`
- **Type:** Feature / Tool Enhancement
- **Labels:** `phase-6-scanner`, `type-safety`, `implicit-conversion`, `tooling`
- **Estimated Effort:** 30 hours
- **Patterns to Detect:**
  - Implicit narrowing conversions (int→char, double→int, etc.)
  - Unsafe pointer/int conversions
  - Enum boundary violations
  - Sign-extension issues (signed→unsigned conversions)
  - Floating-point precision loss in casts
- **Acceptance Criteria:**
  - 8-10 pattern rules fully implemented
  - 300-500 gaps discovered on themis_core
  - < 15% FP rate on validation set (100+ samples)
  - Zero false negatives on known patterns
  - Unit tests for each pattern
  - Integration with gap_scanner_v3 pipeline

### [003-B] Input Validation Scanner (10-12 Patterns)
- **Title:** `Scanner: Input Validation & Sanitization Detection`
- **Type:** Feature / Tool Enhancement
- **Labels:** `phase-6-scanner`, `input-validation`, `security`, `tooling`
- **Estimated Effort:** 35 hours
- **Patterns to Detect:**
  - Missing input validation before use
  - Missing size checks on arrays/buffers
  - Missing format string validation
  - Unvalidated command-line argument usage
  - Missing boundary checks on user input
  - Unvalidated file path operations
- **Acceptance Criteria:**
  - 10-12 pattern rules fully implemented
  - 400-600 gaps discovered on themis_core
  - < 12% FP rate on validation set
  - Security audit of patterns
  - Integration with security scanner category

### [003-C] Exception Safety Scanner (8-10 Patterns)
- **Title:** `Scanner: Exception Safety & RAII Compliance Detection`
- **Type:** Feature / Tool Enhancement
- **Labels:** `phase-6-scanner`, `exception-safety`, `raii`, `tooling`
- **Estimated Effort:** 30 hours
- **Patterns to Detect:**
  - Weak exception safety (new without delete in exception paths)
  - Missing move semantics in exception handlers
  - Incomplete RAII patterns
  - Unhandled exceptions in destructors
  - Resource leaks in exception paths
  - Missing noexcept specifications
- **Acceptance Criteria:**
  - 8-10 pattern rules fully implemented
  - 300-450 gaps discovered
  - < 18% FP rate (exception safety harder to detect)
  - Integration with RAII analyzer

### [003-D] Uninitialized Variables Scanner (8-10 Patterns)
- **Title:** `Scanner: Uninitialized Variable Detection`
- **Type:** Feature / Tool Enhancement
- **Labels:** `phase-6-scanner`, `uninitialized-variables`, `memory-safety`, `tooling`
- **Estimated Effort:** 30 hours
- **Patterns to Detect:**
  - Uninitialized local variables used before assignment
  - Uninitialized member variables (especially in constructors)
  - Uninitialized array elements
  - Uninitialized union fields
  - Uninitialized POD members in copy operations
- **Acceptance Criteria:**
  - 8-10 pattern rules fully implemented
  - 350-500 gaps discovered
  - < 20% FP rate
  - Integration with memory safety analyzer

### [003-E] OOP Design Violations Scanner (14-18 Patterns)
- **Title:** `Scanner: OOP Design & Cohesion Violations`
- **Type:** Feature / Tool Enhancement
- **Labels:** `phase-6-scanner`, `oop-design`, `code-quality`, `tooling`
- **Estimated Effort:** 35 hours
- **Patterns to Detect:**
  - Class cohesion violations (low LCOM score)
  - Violated Liskov substitution principle
  - Circular dependencies in inheritance
  - Anemic data classes without behavior
  - Excessive method coupling
  - Missing virtual destructors
  - God objects (large class clusters)
- **Acceptance Criteria:**
  - 14-18 pattern rules fully implemented
  - 450-700 gaps discovered
  - < 25% FP rate (design rules are subjective)
  - Design metric calculation verified
  - Integration with design analyzer

### [003-F] Phase 6 Scanner Integration & CI/CD Pipeline
- **Title:** `Phase 6: Scanner Integration & CI/CD Pipeline Setup`
- **Type:** Tool Enhancement / Infrastructure
- **Labels:** `phase-6-scanner`, `ci-cd`, `tooling`, `automation`
- **Estimated Effort:** 20 hours
- **Acceptance Criteria:**
  - All 5 new scanners integrated into gap_scanner_v3 pipeline
  - Nightly fast-scan job running and producing reports
  - HTML report generation with trend analysis
  - No CI/CD performance regression (< 15% overhead)
  - Scanner documentation + usage guide completed
  - Metrics dashboard updated with Phase 6 data

## Metrics & KPIs

| Metric | Baseline | Target | Success |
|--------|----------|--------|---------|
| Scanner Categories | 27 | 32 | +5 |
| Pattern Rules | 155-175 | 203-230 | +48-55 |
| Gaps Discovered | 22.085 | 24.585-24.585 | +2.500 (tgt) |
| FP Rate (avg) | TBD | < 15% | Acceptable |
| CI/CD Overhead | TBD | < 15% | Efficient |
| Time to Scan | TBD | < 30min | Acceptable |

## References & Evidence

- **Source:** ROADMAP.md §Phase 6 Extended Scanner Initiative
- **Design Doc:** ai_working/PHASE_6_EXTENDED_SCANNER_DESIGN.md
- **Scanner Pipeline:** tools/gap_scanner_v3_security.py, tools/gap_scanner_v3_memory.py
- **Prior Work:** Phase 1-5 complete with 12 detection patterns (S-1/S-2/S-3/M-1/M-2/C-1)

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| High FP rate | Medium | High | Pattern refinement, manual validation |
| Performance overhead | Medium | Medium | Optimize scanners, parallel execution |
| Pattern false negatives | Low | High | Property-based tests, known gap validation |
| Integration complexity | Low | Medium | Modular design, unit test each scanner |

## Timeline & Milestones

```
W40-W41: [003-A] Type Conversion scanner
W41-W42: [003-B] Input Validation scanner
W42-W43: [003-C] Exception Safety scanner
W43-W44: [003-D] Uninitialized Variables scanner
W44-W45: [003-E] OOP Design scanner
W45-W47: [003-F] Integration & CI/CD setup
```

---

**Last Updated:** 2026-07-05  
**Created By:** AI Deep-Dive Implementation Team  
**Related Epics:** EPIC-002 (parallel), EPIC-001 (prerequisite)
