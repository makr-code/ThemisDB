# whisper Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: whisper
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 20
- Actionable Findings (Critical + High): 11
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 10 |
| Medium | 9 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 7 |
| raii | 7 |
| reliability | 2 |
| container | 1 |
| exception_safety | 1 |
| memory | 1 |
| performance | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/whisper/whisper_plugin.cpp | 9 | 1 | 7 | 1 | 0 |
| src/whisper/audio_chunk_reader.cpp | 6 | 0 | 1 | 5 | 0 |
| src/whisper/tests/test_whisper_plugin.cpp | 4 | 0 | 1 | 3 | 0 |
| src/whisper/whisper_plugin_registrar.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/whisper/whisper_plugin.cpp
Total findings: 9

- Line 273: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::whisper::WhisperPlugin();
- Line 25: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: s_stub_transcriber_factory_fn = std::move(fn);
- Line 90: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!initialized_.load(std::memory_order_acquire)) {
- Line 129: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!initialized_.load(std::memory_order_acquire)) {
- Line 157: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!initialized_.load(std::memory_order_acquire)) return {};
- Line 177: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!initialized_.load(std::memory_order_acquire)) {
- Line 259: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: {"initialized",        initialized_.load(std::memory_order_acquire)},
- Line 278: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: p = nullptr;
  Context: delete p;  // delete nullptr is well-defined; ownership transferred to this function
- Line 278: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete p;  // delete nullptr is well-defined; ownership transferred to this function

### src/whisper/audio_chunk_reader.cpp
Total findings: 6

- Line 160: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(sum / static_cast<float>(num_channels));
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(sum / static_cast<float>(num_channels));
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += '\'';
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "'\\''";
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "'\\''";

### src/whisper/tests/test_whisper_plugin.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([&p]() {
  Confidence: band=high; score=0.74
- Line 462: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([&p]() {
  Confidence: band=high; score=0.74
- Line 486: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([&p]() {
  Confidence: band=high; score=0.74

### src/whisper/whisper_plugin_registrar.cpp
Total findings: 1

- Line 90: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [](WhisperPlugin& plugin, const json& config) -> bool {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
