# stable_diffusion Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: stable_diffusion
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 35
- Actionable Findings (Critical + High): 15
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 3 |
| High | 12 |
| Medium | 20 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 13 |
| performance_patterns | 6 |
| memory | 5 |
| exception_safety | 3 |
| reliability | 3 |
| raii | 2 |
| type_conversion | 2 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/stable_diffusion/sd_plugin.cpp | 24 | 1 | 7 | 16 | 0 |
| src/stable_diffusion/sd_prompt_sanitizer.cpp | 5 | 1 | 1 | 3 | 0 |
| src/stable_diffusion/tests/test_sd_plugin.cpp | 4 | 1 | 3 | 0 | 0 |
| src/stable_diffusion/sd_plugin_registrar.cpp | 2 | 0 | 1 | 1 | 0 |

## Full Scanner Findings

### src/stable_diffusion/sd_plugin.cpp
Total findings: 24

- Line 383: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::imggen::SDPlugin();
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 94: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto crc32_of = [&](const uint8_t* data, size_t len) -> uint32_t {
- Line 97: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: crc = kCrcTable[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
- Line 102: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto adler32_of = [](const uint8_t* data, size_t len) -> uint32_t {
- Line 105: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: s1 = (s1 + data[i]) % 65521u;
- Line 290: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& p : prompts) {
  Confidence: band=very_high; score=0.9
- Line 388: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: p = nullptr;
  Context: delete p;
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    uint8_t ihdr[13];', '    ihdr[0]  = static_cast<uint8_t>(width  >> 24); ihdr[1]  = static_cast<uint8_t>(width  >> 16);', '    ihdr[2]  = static_cast<uint8_t>(width  >>  8); ihdr[3]  = static_cast<uint8_t>(width       );', '    ihdr[4]  = static_cast<uint8_t>(height >> 24); ihdr[5]  = static_cast<uint8_t>(height >> 16);', '    ihdr[6]  = static_cast<uint8_t>(height >>  8); ihdr[7]  = static_cast<uint8_t>(height      );']
  Confidence: band=medium; score=0.62
- Line 49: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: v.push_back(static_cast<uint8_t>(x >> 24));
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: v.push_back(static_cast<uint8_t>(x >> 24));
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x >> 24));
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x >> 16));
- Line 115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x >>  8));
- Line 116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x      ));
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x     ));
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: v.push_back(static_cast<uint8_t>(x >> 8));
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idat_payload.push_back(0x78u);
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: idat_payload.push_back(0x78u);
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: idat_payload.push_back(0x01u);
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(generateLocked(p, cfg));
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(generateLocked(p, cfg));
- Line 388: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete p;

### src/stable_diffusion/sd_prompt_sanitizer.cpp
Total findings: 5

- Line 48: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator start may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto start = line.find_first_not_of(" \t\r\n");
- Line 42: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("SDPromptSanitizer: cannot open '" + path + "'");
- Line 34: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!lower.empty()) blocked_keywords_.push_back(lower);
  Confidence: band=high; score=0.74
- Line 35: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!lower.empty()) blocked_keywords_.push_back(lower);
- Line 52: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: kws.push_back(line);

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

- Line 83: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [](SDPlugin& plugin, const json& config) -> bool {
- Line 49: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
