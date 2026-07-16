# Phase 1 Implementation Plan v2 - Data-Driven Tuning

**Generated from:** SCANNER_FEEDBACK_REPORT.json  
**Date:** 2026-06-02  
**Baseline:** 24% TP, 36% FP, 40% uncertain  
**Target:** 35-40% TP (2-week Sprint)

---

## Executive Summary

Das **Scanner Feedback Loop System** hat folgende Erkenntnisse geliefert:

- **30 Kategorien gescannt** → davon 15 mit 0% TP (pure FP)
- **5 Kategorien mit 100% TP** (bereits exzellent kalibriert)
- **Gesamteffort: 53.9 Tage** für alle Kategorien

**Strategie:** Fokus auf HIGH-Priority Kategorien mit höchstem Impact/Effort Ratio

---

## HIGH-Priority Categories (Start Here)

### Tier 1: Quick Wins (2-3 Tage)

Diese 5 Kategorien haben **nur 1 Gap** mit 0% TP und sind schnell zu fixen:

1. **HARDCODED_PATH** (1 gap, 0% TP)
   - **Effort:** 2.4 Tage | **Impact:** 0% → 50%
   - **Fix:** Distinguish compile-time vs runtime paths
   - **Key Changes:**
     ```python
     # gap_scanner_v3_input_validation.py
     def _should_skip_hardcoded_path(line, context):
         # WHITELIST: Compile-time constants
         if 'constexpr' in context or '#define' in context:
             return True
         # WHITELIST: Configuration sources
         if 'config.' in context or 'Config::' in context:
             return True
         # WHITELIST: Environment variables
         if 'getenv' in context or 'std::getenv' in context:
             return True
         # WHITELIST: Test code
         if '_test.cpp' in file_path or '_unittest.cpp' in file_path:
             return True
         return False
     ```
   - **Implementation:** Update input_validation scanner

2. **NO_TIMEOUT** (1 gap, 0% TP)
   - **Effort:** 2.4 Tage | **Impact:** 0% → 50%
   - **Fix:** Identify async operations requiring timeout
   - **Key Changes:** Add sync-only operation whitelist
   - **Implementation:** Update reliability scanner

3. **DB_CONNECTION_LEAK** (1 gap, 0% TP)
   - **Effort:** 4.5 Tage | **Impact:** 0% → 50%
   - **Fix:** Recognize RAII + ConnectionPool patterns
   - **Key Changes:**
     ```python
     # gap_scanner_v3_raii.py
     def _should_skip_connection_leak(line, context, file_path):
         # WHITELIST: Smart pointers
         if 'std::unique_ptr' in context or 'std::shared_ptr' in context:
             return True
         # WHITELIST: ConnectionPool
         if 'ConnectionPool' in context:
             return True
         # WHITELIST: Explicit close()
         if '.close()' in context:
             return True
         # Expand context window to ±20 for destructor visibility
         return False
     ```
   - **Implementation:** Enhance RAII scanner with context expansion

4. **NO_HEALTH_CHECK** (1 gap, 0% TP)
   - **Effort:** 3.0 Tage | **Impact:** 0% → 60%
   - **Fix:** Limit to HTTP/gRPC handlers only
   - **Key Changes:** Add handler pattern detection
   - **Implementation:** Scope observability checks

### Tier 2: Medium-Effort (3-4 Tage)

Diese Kategorien haben mehrere Gaps und höhere Komplexität:

5. **COPY_OVERHEAD** (3 gaps, 0% TP)
   - **Effort:** 3.0 Tage | **Impact:** 0% → 60%
   - **Pattern FPs:** 
     - make_shared/make_unique flagged
     - POD types (int, float) flagged
     - Keine Loop-Kontext
   - **Fixes:**
     1. Whitelist make_shared/make_unique
     2. Add POD type detection
     3. Require loop context
     4. Check for std::move
     5. Expand context window
   - **Implementation:** Update gap_scanner_v3_phase8_performance_patterns.py (ALREADY STARTED)

6. **OBSERVABILITY** (2 gaps, 0% TP)
   - **Effort:** 2.4 Tage | **Impact:** 0% → 70%
   - **Pattern FPs:**
     - Internal functions flagged
     - Trivial functions (<5 lines) flagged
     - Private members flagged
   - **Fixes:**
     1. Skip internal namespaces
     2. Skip trivial functions
     3. Require public API marker
     4. Skip getters/setters/operators
   - **Implementation:** Update gap_scanner_v3_phase10_observability.py (ALREADY STARTED)

---

## Recommended Implementation Sequence

### Phase 1a: Quick Wins (Days 1-2)
- [ ] **HARDCODED_PATH:** Runtime vs compile-time detection (2.4 d)
- [ ] **NO_TIMEOUT:** Async operation detection (2.4 d)

**Expected Impact:** 24% → 30% TP

### Phase 1b: Core Patterns (Days 3-7)
- [ ] **COPY_OVERHEAD:** POD + loop context (3.0 d)
- [ ] **OBSERVABILITY:** Scope + API awareness (2.4 d)
- [ ] **DB_CONNECTION_LEAK:** RAII + ConnectionPool (4.5 d)
- [ ] **NO_HEALTH_CHECK:** Handler scoping (3.0 d)

**Expected Impact:** 30% → 35-40% TP

### Phase 1c: Context Window Expansion (Day 8+)
- [ ] Global: Expand ±5 → ±15 lines in all scanners (1.0 d)
- [ ] Validate + test (0.5 d)

**Expected Impact:** +2-3% TP across all categories

---

## Key Data-Driven Insights

### Excellent Categories (100% TP - Don't Touch!)
- `smart_ptr_misuse` (2/2 TP)
- `string_concat_loop` (2/2 TP)
- `memory_order` (2/2 TP)
- `repeated_search` (1/1 TP)
- `legacy_duplication` (1/1 TP)

→ **Action:** Study these for pattern extraction

### Problematic Categories (0% TP - All FP)
- hardcoded_path (1/0 FP)
- no_timeout (1/0 FP)
- null_dereference (1/0 FP)
- pointer_arithmetic (1/0 FP)
- no_retry_logic (1/0 FP)
- o_n_squared (1/0 FP)

→ **Action:** These need scoping or whitelisting

### Medium Categories (Partial TP)
- copy_overhead (0/3 TP) → Generic detection missing POD awareness
- observability (0/2 TP) → Flagging internals/trivials
- db_connection_leak (0/1 TP) → RAII not recognized
- no_health_check (0/1 TP) → Scope too broad

→ **Action:** Pattern-based whitelisting + scoping

---

## Technical Implementation Checklist

### Context Window Expansion
- [ ] `gap_scanner_v3_progressive_context_filters.py`: DEFAULT 5 → 15
- [ ] All wave1/2/3 functions: Use 15-line context
- [ ] RAII/memory scanners: Use 20-line context for cleanup visibility

### Whitelisting & Scoping
- [ ] Performance scanner: Whitelist make_shared/unique, POD types
- [ ] Observability scanner: Skip internals, trivials, getters
- [ ] Input validation: Distinguish compile-time vs runtime
- [ ] Reliability scanner: Limit no_health_check to handlers

### Testing Strategy
```bash
# After each category fix:
python tools/gap_audit_pipeline_v3.py --repo . --sample 50
python tools/analyze_validation_sample.py
python tools/scanner_feedback_loop.py

# Expected output: SCANNER_FEEDBACK_REPORT.md shows improvement
```

### Success Metrics
| Category | Current | Target | Effort |
|----------|---------|--------|--------|
| hardcoded_path | 0% TP | 50% TP | 2.4 d |
| no_timeout | 0% TP | 50% TP | 2.4 d |
| copy_overhead | 0% TP | 60% TP | 3.0 d |
| observability | 0% TP | 70% TP | 2.4 d |
| db_connection_leak | 0% TP | 50% TP | 4.5 d |
| **OVERALL** | **24% TP** | **35-40% TP** | **~14 d** |

---

## Adaptive Feedback Loop

Nach jedem Fix:

1. **Run:** `python tools/scanner_feedback_loop.py`
2. **Review:** SCANNER_FEEDBACK_REPORT.md
3. **Decide:** Adjust or continue to next category
4. **Measure:** Track TP improvement per category

**Loop Termination:** Wenn TP ≥ 35%, Phase 1 complete. Starte Phase 2.

---

## Lesson Learned: Reinforced Learning Works!

Das Feedback Loop System zeigt:
- **24% baseline** ist nicht schlecht für untuned generisches Scanning
- **Pattern Recognition** statt Blindheit: FP-Patterns sind vorhersehbar
- **Quick Wins:** Categories mit nur 1-3 Gaps sind am schnellsten zu fixen
- **Quality Tiers:** 100% TP Categories zeigen was möglich ist

→ **Nächste Iteration:** Noch bessere Pattern-Extraction aus 100% TP Categories

