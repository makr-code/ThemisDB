# core Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: core
- Generated: 2026-06-02 11:09:12
- Status: High-Priority Findings Present
- Total Findings: 22
- Actionable Findings (Critical + High): 5
- Affected Files: 6

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 0 |
| High | 5 |
| Medium | 11 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance | 28 |
| performance_patterns | 14 |
| reliability | 11 |
| exception_safety | 8 |
| raii | 8 |
| observability | 6 |
| container | 5 |
| audit_logging | 3 |
| platform | 3 |
| determinism | 1 |
| legacy_duplication | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/core/concerns/zero_copy_logger.cpp | 12 | 0 | 0 | 6 | 6 |
| src/core/concerns/redis_cache.cpp | 8 | 0 | 5 | 3 | 0 |
| src/core/concerns/lockfree_metrics.cpp | 2 | 0 | 0 | 2 | 0 |
| src/core/adapters/otel_tracer.cpp | 0 | 0 | 0 | 0 | 0 |
| src/core/concerns/concerns_context.cpp | 0 | 0 | 0 | 0 | 0 |
| src/core/concerns/prometheus_metrics.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/core/concerns/zero_copy_logger.cpp
Total findings: 12

- Line 163: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ",\"";
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ' ';
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "\\\"";
  Confidence: band=high; score=0.74
- Line 353: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += "{\"ts\":\"";
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ",\"";
  Confidence: band=high; score=0.74
- Line 382: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: buf += ' ';
  Confidence: band=high; score=0.74
- Line 46: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return logger_ && logger_->should_log(toSpdlogLevel(level));
  Confidence: band=medium; score=0.6
- Line 57: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: logger_->log(toSpdlogLevel(level), message);
  Confidence: band=medium; score=0.6
- Line 105: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: if (!logger_->should_log(toSpdlogLevel(level))) {
  Confidence: band=medium; score=0.6
- Line 118: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: logger_->log(toSpdlogLevel(level), buf);
  Confidence: band=medium; score=0.6
- Line 129: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: if (!logger_->should_log(toSpdlogLevel(level))) {
  Confidence: band=medium; score=0.6
- Line 189: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: logger_->log(toSpdlogLevel(level), buf);
  Confidence: band=medium; score=0.6

### src/core/concerns/redis_cache.cpp
Total findings: 8

- Line 16: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * POSIX sockets (Linux/macOS) with a thin Win32 compatibility shim.
  Confidence: band=high; score=0.8
- Line 707: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 959: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: return (total == 0.0L) ? 0.0 : static_cast<double>(static_cast<long double>(h) / total);
  Confidence: band=very_high; score=0.9
- Line 985: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 1006: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &nc : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_.push_back(std::move(nc));
  Confidence: band=high; score=0.74
- Line 440: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: cmd += '$';
  Confidence: band=high; score=0.74
- Line 905: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(std::move(elem));
  Confidence: band=high; score=0.74

### src/core/concerns/lockfree_metrics.cpp
Total findings: 2

- Line 265: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += '{';
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ',';
  Confidence: band=high; score=0.74

### src/core/adapters/otel_tracer.cpp
Total findings: 0


### src/core/concerns/concerns_context.cpp
Total findings: 0


### src/core/concerns/prometheus_metrics.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
