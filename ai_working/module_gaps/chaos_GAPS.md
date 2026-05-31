# chaos Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: chaos
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 16
- Actionable Findings (Critical + High): 12
- Affected Files: 1

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 4 |
| High | 8 |
| Medium | 4 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 7 |
| container | 6 |
| exception_safety | 1 |
| performance | 1 |
| reliability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/chaos/chaos_framework.cpp | 16 | 4 | 8 | 4 | 0 |

## Full Scanner Findings

### src/chaos/chaos_framework.cpp
Total findings: 16

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 99: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it  = active_faults_.begin();
- Line 117: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = active_faults_.find(key);
- Line 176: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = active_faults_.begin();
- Line 90: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &cb : callbacks_) {
  Confidence: band=very_high; score=0.9
- Line 102: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &cb : callbacks_) {
  Confidence: band=very_high; score=0.9
- Line 121: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &cb : callbacks_) {
  Confidence: band=very_high; score=0.9
- Line 130: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[key, af] : active_faults_) {
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[_, af] : active_faults_) {
  Confidence: band=very_high; score=0.9
- Line 191: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ChaosScheduler: injector must not be null");
- Line 246: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(sched_mutex_);
- Line 257: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &fault : to_fire) {
  Confidence: band=very_high; score=0.9
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(af);
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(af);
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: callbacks_.push_back(std::move(cb));
- Line 249: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_fire.push_back(e.fault);

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
