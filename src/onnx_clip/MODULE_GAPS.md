# onnx_clip Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: onnx_clip
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 8
- Actionable Findings (Critical + High): 3
- Affected Files: 2

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 2 |
| High | 1 |
| Medium | 5 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| manual_cleanup | 3 |
| data_race | 2 |
| deadlock_risk | 1 |
| missing_module_doc | 1 |
| stale_doc_section_reference | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| onnx_clip/onnx_clip_plugin.cpp | 7 | 2 | 1 | 4 | 0 |
| onnx_clip | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### onnx_clip/onnx_clip_plugin.cpp
Total findings: 7

- Line 127: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: seed ^= static_cast<uint64_t>(metadata->width + 31 * metadata->height + 17 * metadata->channels);
- Line 129: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: seed ^= static_cast<uint64_t>(metadata->bits_per_channel + 13);
- Line 297: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 80: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 88: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 91: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 342: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub/Simulation Lifecycle' that was not found in 'src/onnx_clip/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/onnx_clip/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"

### onnx_clip
Total findings: 1

- Line 1: severity=MEDIUM; category=missing_module_doc
  Description: Module 'onnx_clip' missing required governance doc 'PRODUCTION_REQUIREMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_module_governance_rules
  Context: Expected file: src/onnx_clip/PRODUCTION_REQUIREMENTS.md

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
