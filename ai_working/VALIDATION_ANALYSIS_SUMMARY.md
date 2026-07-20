# Automated Gap Validation Analysis Summary

**Date:** 2026-07-20T10:13:34.344350+00:00
**Sample Size:** 50 gaps
**Full Dataset:** 18,795 gaps

## Frozen Baseline

- Baseline TP: 24.0%
- CRITICAL TP baseline: 50.0%
- Priority categories: legacy_duplication, smart_ptr_misuse, memory_order, uncaught_exception
- Deferred high-FP categories: observability, copy_overhead, db_connection_leak, no_health_check, hardcoded_path

## Classification Results

| Classification | Count | % | Est. in Full Set |
|---|---|---|---|
| **True Positives** | 8 | 16.0% | 3,007 |
| **False Positives** | 20 | 40.0% | 7,518 |
| **Uncertain** | 22 | 44.0% | 8,269 |

## Interpretation

❌ **LOW QUALITY** - Most gaps are false positives. Significant scanner tuning needed.

## Recommendations

1. **Acceptable FP Rate** (<40%) - Most gaps are actionable
2. Monitor high-severity gaps first (CRITICAL > HIGH > MEDIUM)
3. Consider sampling additional gaps from low-confidence categories

## Next Steps

1. Use TP rate (16%) as baseline quality metric
2. Prioritize ~3,007 estimated true positive gaps
3. Organize by module and severity for remediation planning
4. Track remediation progress

---

**Detailed assessment data:** See `VALIDATION_ANALYSIS_REPORT.json`
