# stable_diffusion Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: stable_diffusion
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 27
- Actionable Findings (Critical + High): 12
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 2 |
| High | 10 |
| Medium | 15 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 7 |
| memory | 5 |
| performance_patterns | 5 |
| exception_safety | 3 |
| raii | 2 |
| reliability | 2 |
| type_conversion | 2 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/stable_diffusion/sd_plugin.cpp | 20 | 1 | 6 | 13 | 0 |
| src/stable_diffusion/tests/test_sd_plugin.cpp | 4 | 1 | 3 | 0 | 0 |
| src/stable_diffusion/sd_plugin_registrar.cpp | 2 | 0 | 1 | 1 | 0 |
| src/stable_diffusion/sd_prompt_sanitizer.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/stable_diffusion/sd_plugin.cpp
Total findings: 20

- Line 381: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::imggen::SDPlugin();
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 92: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto crc32_of = [&](const uint8_t* data, size_t len) -> uint32_t {
- Line 95: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: crc = kCrcTable[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
- Line 100: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto adler32_of = [](const uint8_t* data, size_t len) -> uint32_t {
- Line 103: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: s1 = (s1 + data[i]) % 65521u;
- Line 386: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: p = nullptr;
  Context: delete p;
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    uint8_t ihdr[13];', '    ihdr[0]  = static_cast<uint8_t>(width  >> 24); ihdr[1]  = static_cast<uint8_t>(width  >> 16);', '    ihdr[2]  = static_cast<uint8_t>(width  >>  8); ihdr[3]  = static_cast<uint8_t>(width       );', '    ihdr[4]  = static_cast<uint8_t>(height >> 24); ihdr[5]  = static_cast<uint8_t>(height >> 16);', '    ihdr[6]  = static_cast<uint8_t>(height >>  8); ihdr[7]  = static_cast<uint8_t>(height      );']
  Confidence: band=medium; score=0.62
- Line 47: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: v.push_back(static_cast<uint8_t>(x >> 24));
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: v.push_back(static_cast<uint8_t>(x >> 24));
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x >> 24));
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x >> 16));
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x >>  8));
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x      ));
- Line 117: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x     ));
- Line 118: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x >> 8));
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idat_payload.push_back(0x78u);
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(generateLocked(p, cfg));
  Confidence: band=high; score=0.74
- Line 386: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete p;

### src/stable_diffusion/tests/test_sd_plugin.cpp
Total findings: 4

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    EXPECT_EQ(out_w, 8);', '    EXPECT_EQ(out_h, 8);', '    EXPECT_EQ(result.size(), static_cast<size_t>(8 * 8 * 3));', '}', '']
  Confidence: band=very_high; score=0.93
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73

### src/stable_diffusion/sd_plugin_registrar.cpp
Total findings: 2

- Line 81: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [](SDPlugin& plugin, const json& config) -> bool {
- Line 47: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/stable_diffusion/sd_prompt_sanitizer.cpp
Total findings: 1

- Line 32: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!lower.empty()) blocked_keywords_.push_back(lower);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
