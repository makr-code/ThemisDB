# whisper Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: whisper
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 40
- Actionable Findings (Critical + High): 28
- Affected Files: 5

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 2 |
| High | 26 |
| Medium | 12 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 16 |
| raii | 8 |
| performance_patterns | 7 |
| container | 3 |
| memory | 3 |
| concurrency | 1 |
| exception_safety | 1 |
| performance | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/whisper/audio_chunk_reader.cpp | 24 | 0 | 17 | 7 | 0 |
| src/whisper/whisper_plugin.cpp | 10 | 2 | 7 | 1 | 0 |
| src/whisper/tests/test_whisper_plugin.cpp | 4 | 0 | 1 | 3 | 0 |
| src/whisper/whisper_plugin_registrar.cpp | 1 | 0 | 1 | 0 | 0 |
| src/whisper/whisper_transcriber.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/whisper/audio_chunk_reader.cpp
Total findings: 24

- Line 66: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("WavAudioChunkReader: cannot open '" + path + "'");
- Line 70: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("WavAudioChunkReader: file too small to be a WAV: '" + path + "'");
- Line 88: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("WavAudioChunkReader: not a valid RIFF/WAV file");
- Line 110: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (!found_fmt) throw std::runtime_error("WavAudioChunkReader: 'data' chunk before 'fmt '");
- Line 115: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("WavAudioChunkReader: invalid WAV — num_channels is 0");
- Line 119: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 140: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(&s, &data[data_start + (i * num_channels + ch) * 4], 4);
- Line 155: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(&sample, &data[data_start + (i * num_channels + ch) * 2], 2);
- Line 162: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 172: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("WavAudioChunkReader: 'data' chunk not found");
- Line 195: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("FfmpegAudioChunkReader: path contains NUL byte");
- Line 221: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("FfmpegAudioChunkReader: ffmpeg not available");
- Line 228: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("FfmpegAudioChunkReader: ffmpeg not available");
- Line 240: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 253: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 265: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 294: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(sum / static_cast<float>(num_channels));
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(sum / static_cast<float>(num_channels));
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(sum / static_cast<float>(num_channels));
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(sum / static_cast<float>(num_channels));
- Line 200: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += '\'';
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "'\\''";
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "'\\''";

### src/whisper/whisper_plugin.cpp
Total findings: 10

- Line 232: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto segments = vad_->detect(pcm, sample_rate, vad_cfg_);
- Line 275: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::whisper::WhisperPlugin();
- Line 27: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: s_stub_transcriber_factory_fn = std::move(fn);
- Line 92: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!initialized_.load(std::memory_order_acquire)) {
- Line 131: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!initialized_.load(std::memory_order_acquire)) {
- Line 159: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!initialized_.load(std::memory_order_acquire)) return {};
- Line 179: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!initialized_.load(std::memory_order_acquire)) {
- Line 261: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: {"initialized",        initialized_.load(std::memory_order_acquire)},
- Line 280: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: p = nullptr;
  Context: delete p;  // delete nullptr is well-defined; ownership transferred to this function
- Line 280: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete p;  // delete nullptr is well-defined; ownership transferred to this function

### src/whisper/tests/test_whisper_plugin.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 440: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([&p]() {
  Confidence: band=high; score=0.74
- Line 464: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([&p]() {
  Confidence: band=high; score=0.74
- Line 488: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([&p]() {
  Confidence: band=high; score=0.74

### src/whisper/whisper_plugin_registrar.cpp
Total findings: 1

- Line 92: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [](WhisperPlugin& plugin, const json& config) -> bool {

### src/whisper/whisper_transcriber.cpp
Total findings: 1

- Line 32: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: whisper_free(static_cast<whisper_context*>(ctx_));

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
