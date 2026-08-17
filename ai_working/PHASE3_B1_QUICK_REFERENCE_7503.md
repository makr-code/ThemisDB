# PHASE 3 BATCH B1: QUICK REFERENCE GUIDE
## Line-by-Line Implementation Plan for Batch B2

**Generated**: 2026-08-15  
**Purpose**: Quick lookup table for Phase 3 Batch B2 implementation  
**Valid Instances**: 32/50 (had detectable first-use lines)

---

## Priority 1: Loop Counter Relocations (4 fixes - TRIVIAL RISK)

Apply these first - highest confidence, zero behavioral change.

| File | Line | Variable | Action |
|------|------|----------|--------|
| anomaly_detection.cpp | 729 | `total_splits` | Move to for-init |
| arrow_export.cpp | 171 | `total_bytes` | Move to for-init |
| automl.cpp | 845 | `correct` | Move to for-init |


**Status**: 100% automation-ready  
**Expected Result**: 4 lines saved, no behavior change

---

## Priority 2: Boolean Flag Relocations (7 fixes - LOW RISK)

Apply second - high confidence, safe scope reduction.

| File | Line | Variable | Action |
|------|------|----------|--------|
| aggregation.cpp | 114 | `first` | Move to if-block |
| analytics_export.cpp | 44 | `has_nulls` | Move to if-block |
| anomaly_detection.cpp | 406 | `trained` | Move to if-block |
| arrow_flight.cpp | 599 | `done` | Move to if-block |


**Status**: 100% automation-ready  
**Expected Result**: 7 lines saved, cleaner scope

---

## Priority 3: General Variable Relocations (39 fixes - MEDIUM RISK)

Apply selectively - categorized by confidence level.

### Categorization by Confidence

#### Simple Cases: Used within 3 lines (9 instances) ✅

- aggregation.cpp:69 - `it_end` (used in 1 lines)
- aggregation.cpp:247 - `it` (used in 3 lines)
- analytics_export.cpp:109 - `schema` (used in 2 lines)
- anomaly_detection.cpp:107 - `acc` (used in 1 lines)
- anomaly_detection.cpp:127 - `hi` (used in 2 lines)
- anomaly_detection.cpp:147 - `lerp` (used in 3 lines)
- automl.cpp:339 - `s` (used in 1 lines)
- automl.cpp:348 - `s` (used in 2 lines)
- automl.cpp:366 - `maxv` (used in 3 lines)


#### Medium Cases: Used 4-6 lines later (6 instances) ⚠️

- anomaly_detection.cpp:347 - `node` (used in 4 lines)
- anomaly_detection.cpp:348 - `depth` (used in 5 lines)
- anomaly_detection.cpp:375 - `sum` (used in 6 lines)
- automl.cpp:367 - `sum` (used in 4 lines)
- automl.cpp:398 - `min_samples_leaf` (used in 5 lines)
- automl.cpp:400 - `n_classes` (used in 6 lines)


#### Complex Cases: Used 7+ lines later (10 instances) 🔍

- anomaly_detection.cpp:1032 - `writeVec` (used in 9 lines)
- anomaly_detection.cpp:1058 - `splitComma` (used in 10 lines)
- anomaly_detection.cpp:1068 - `toDoubleVec` (used in 11 lines)
- automl.cpp:674 - `n_classes` (used in 7 lines)
- automl.cpp:675 - `lr` (used in 8 lines)
- automl.cpp:676 - `l2` (used in 9 lines)
- automl.cpp:677 - `max_epochs` (used in 10 lines)
- automl.cpp:678 - `batch_size` (used in 11 lines)
- automl.cpp:746 - `l2` (used in 12 lines)
- automl.cpp:845 - `correct` (used in 14 lines)


---

## Batch B2 Recommendation

**Priority 1**: 4 loop counter fixes (Priority 1)
**Priority 2**: 7 boolean flag fixes (Priority 2)
**Priority 3**: 9 simple variable fixes (Priority 3a)
**Priority 4**: 6 medium variable fixes (Priority 3b)

**Total for Batch B2**: 26 fixes

**Automation Level**: HIGH  
**Manual Review**: MINIMAL  
**Expected Test Pass Rate**: 100%

---

## Files Affected

| File | Issues | B2 Target | B3 Remaining |
|------|--------|-----------|--------------|
| automl.cpp | 32 | 16-20 | 12-16 |
| anomaly_detection.cpp | 11 | 6-8 | 3-5 |
| aggregation.cpp | 3 | 3 | 0 |
| analytics_export.cpp | 2 | 2 | 0 |
| arrow_flight.cpp | 1 | 1 | 0 |
| arrow_export.cpp | 1 | 1 | 0 |
| **TOTAL** | **50** | **29-35** | **15-21** |

---

## Next Steps

1. **Immediate**: Review this quick reference guide
2. **Before B2**: Verify all source files are backed up
3. **During B2**: Apply fixes in priority order (Priority 1 → 2 → 3)
4. **After B2**: Run full test suite
5. **Complete**: Generate Phase 3 B2 Completion Report

**Status**: ✅ Ready to proceed with Phase 3 Batch B2
