# onnx_clip Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: onnx_clip
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 10
- Actionable Findings (Critical + High): 3
- Affected Files: 1

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 2 |
| High | 1 |
| Medium | 7 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 4 |
| concurrency | 3 |
| raii | 3 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/onnx_clip/onnx_clip_plugin.cpp | 10 | 2 | 1 | 7 | 0 |

## Full Scanner Findings

### src/onnx_clip/onnx_clip_plugin.cpp
Total findings: 10

- Line 127: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: seed ^= static_cast<uint64_t>(metadata->width + 31 * metadata->height + 17 * metadata->channels);
- Line 129: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: seed ^= static_cast<uint64_t>(metadata->bits_per_channel + 13);
- Line 297: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 80: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 88: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 91: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(token);
  Confidence: band=high; score=0.74
- Line 424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(impl_->computeEmbedding(images[i], nullptr,
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(impl_->computeEmbedding(images[i], nullptr,
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
