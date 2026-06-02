# Automated Gap Validation Analysis Summary

**Date:** 2026-06-02
**Sample Size:** 50 gaps
**Full Dataset:** 18,795 gaps

## Classification Results

| Classification | Count | % | Est. in Full Set |
|---|---|---|---|
| **True Positives** | 12 | 24.0% | 4,510 |
| **False Positives** | 18 | 36.0% | 6,766 |
| **Uncertain** | 20 | 40.0% | 7,518 |

## Interpretation

❌ **LOW QUALITY** - Most gaps are false positives. Significant scanner tuning needed.

## Recommendations

1. **Acceptable FP Rate** (<40%) - Most gaps are actionable
2. Monitor high-severity gaps first (CRITICAL > HIGH > MEDIUM)
3. Consider sampling additional gaps from low-confidence categories

## Next Steps

1. Use TP rate (24%) as baseline quality metric
2. Prioritize ~4,510 estimated true positive gaps
3. Organize by module and severity for remediation planning
4. Track remediation progress

---

**Detailed assessment data:** See `VALIDATION_ANALYSIS_REPORT.json`
