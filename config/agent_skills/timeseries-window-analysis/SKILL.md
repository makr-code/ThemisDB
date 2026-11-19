---
name: timeseries-window-analysis
description: Compare ThemisDB time series across two equal windows, optionally scoped by a geographic radius.
metadata:
  AI_Used: true
  AI_Models: "unknown"
  AI_Contributor: "GitHub Copilot"
  Human_Review: false
  Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

# Compare time series windows

1. Establish the metric, unit, data freshness, and baseline. This skill works
   with ingested finance, engineering, or other time series; it does not fetch
   data from external systems. Only use geography when the task needs it.
2. Use `compare_timeseries_windows` only if the host offers it for the intended
   collection and metric. Pass `baseline_start_ms`, `current_start_ms`, and
   `duration_ms` as integer milliseconds. The equal windows are half-open,
   with the baseline preceding the current window without overlap. Optionally
   pass `geo` with `lon`, `lat`, and `radius_m`.
3. Report the two sample counts, observed sample means, and signed difference
   (current minus baseline).
   Missing measurements are unknown, not zero. With geography, report entities
   excluded for missing locations. Means are not time-weighted; counts alone
   do not establish regular sampling or complete coverage.
4. Report unavailable tools, invalid inputs, and read or limit errors
   explicitly. The native comparison tool rejects malformed matching stored
   records/chunks, but cannot prove that the source contains every expected
   measurement. Do not retry with broader scope or claim an atomic snapshot.
