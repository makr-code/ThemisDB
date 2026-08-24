# Automated Gap Validation Analysis Summary

**Date:** 2026-07-20T11:37:26.515842+00:00
**Sample Size:** 50 gaps
**Full Dataset:** 30,015 gaps

## Frozen Baseline

- Frozen baseline TP: 24.0%
- CRITICAL TP baseline: 50.0%
- Priority categories: legacy_duplication, smart_ptr_misuse, memory_order, uncaught_exception
- Deferred high-FP categories: observability, copy_overhead, db_connection_leak, no_health_check, hardcoded_path

## Classification Results

| Classification | Count | % | Est. in Full Set |
|---|---|---|---|
| **True Positives** | 8 | 16.0% | 4,802 |
| **False Positives** | 20 | 40.0% | 12,006 |
| **Uncertain** | 22 | 44.0% | 13,206 |

## Interpretation

❌ **LOW QUALITY** - Most gaps are false positives. Significant scanner tuning needed.

## Recommendations

1. **Scanner Tuning Required:** FP rate is at least 40%. Focus on high-FP categories:
   - `pointer_arithmetic`: 1/1 FP (100%)
   - `uninitialized_access`: 1/1 FP (100%)
   - `range_temporary`: 1/1 FP (100%)

2. Investigate why these categories generate false positives
3. Modify detection logic or add filters
4. Re-run pipeline and validate again

## Next Steps

1. Compare the current sample TP rate (16%) against the frozen baseline (24%)
2. Prioritize ~4,802 estimated true positive gaps
3. Organize by module and severity for remediation planning
4. Track remediation progress

---

**Detailed assessment data:** See `VALIDATION_ANALYSIS_REPORT.json`
