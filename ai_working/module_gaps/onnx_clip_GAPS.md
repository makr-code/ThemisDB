# onnx_clip Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: onnx_clip
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 34
- Actionable Findings (Critical + High): 22
- Affected Files: 1

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 15 |
| High | 7 |
| Medium | 12 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| concurrency | 16 |
| performance_patterns | 5 |
| container | 4 |
| raii | 4 |
| memory | 3 |
| security | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/onnx_clip/onnx_clip_plugin.cpp | 34 | 15 | 7 | 12 | 0 |

## Full Scanner Findings

### src/onnx_clip/onnx_clip_plugin.cpp
Total findings: 34

- Line 129: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: seed ^= static_cast<uint64_t>(metadata->width + 31 * metadata->height + 17 * metadata->channels);
- Line 131: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: seed ^= static_cast<uint64_t>(metadata->bits_per_channel + 13);
- Line 302: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->embedding_dim = config.get<int>("model.embedding_dim", 512);
- Line 303: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (impl_->embedding_dim <= 0) {
- Line 304: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->embedding_dim = 512;
- Line 317: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->max_batch_size = std::max(1, cfg_max);
- Line 405: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->total_latency_ms += static_cast<double>(dt_ms);
- Line 429: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->total_errors += static_cast<uint64_t>(images.size());
- Line 434: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const size_t batch_limit = static_cast<size_t>(impl_->max_batch_size);
- Line 455: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->total_images += static_cast<uint64_t>(images.size());
- Line 456: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->total_inferences += static_cast<uint64_t>(images.size());
- Line 457: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->total_latency_ms += static_cast<double>(dt_ms);
- Line 458: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->clip_batch_embeddings_total += static_cast<uint64_t>(images.size());
- Line 490: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->total_latency_ms += static_cast<double>(dt_ms);
- Line 502: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double avg_latency = impl_->total_inferences > 0
- Line 131: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: seed ^= static_cast<uint64_t>(metadata->bits_per_channel + 13);
- Line 367: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(impl_->mutex);
- Line 395: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: EmbeddingResult result = impl_->computeEmbedding(image_data, metadata,
- Line 438: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: results.push_back(impl_->computeEmbedding(images[i], nullptr,
- Line 447: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& r : results) {
  Confidence: band=very_high; score=0.9
- Line 532: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: EmbeddingResult r = impl_->computeEmbedding(dummy, nullptr,
- Line 532: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: EmbeddingResult r = impl_->computeEmbedding(dummy, nullptr,
- Line 75: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 82: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 90: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 93: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(token);
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(token);
- Line 161: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(token);
- Line 426: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(result));
- Line 437: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(impl_->computeEmbedding(images[i], nullptr,
  Confidence: band=high; score=0.74
- Line 437: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(impl_->computeEmbedding(images[i], nullptr,
  Confidence: band=high; score=0.74
- Line 438: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(impl_->computeEmbedding(images[i], nullptr,

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
