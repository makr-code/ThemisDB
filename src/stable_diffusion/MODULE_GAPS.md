# stable_diffusion Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: stable_diffusion
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 15
- Actionable Findings (Critical + High): 10
- Affected Files: 4

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 2 |
| High | 8 |
| Medium | 5 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| resource_leaked_in_exception | 3 |
| copy_overhead | 1 |
| delete_no_nullptr | 1 |
| delete_without_nullptr | 1 |
| explicit_delete | 1 |
| manual_cleanup | 1 |
| missing_module_doc | 1 |
| missing_vector_reserve | 1 |
| multiplication_overflow | 1 |
| shift_overflow | 1 |
| smart_ptr_misuse | 1 |
| uninitialized_access | 1 |
| uninitialized_array | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| stable_diffusion/sd_plugin.cpp | 9 | 1 | 4 | 4 | 0 |
| stable_diffusion/tests/test_sd_plugin.cpp | 4 | 1 | 3 | 0 | 0 |
| stable_diffusion | 1 | 0 | 0 | 1 | 0 |
| stable_diffusion/sd_plugin_registrar.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### stable_diffusion/sd_plugin.cpp
Total findings: 9

- Line 381: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return new themis::imggen::SDPlugin();
- Line 187: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 386: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete p;
- Line 386: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: extern "C" THEMIS_PLUGIN_EXPORT

void themis_imggen_destroy(themis::imggen::IImageGenerationBackend* p) {

    delete p;

}

#endif
- Line 386: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete p;
- Line 154: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: idat_payload.push_back(0x78u);
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: idat_payload.push_back(is_final ? 0x01u : 0x00u);  // BFINAL | BTYPE=00
- Line 189: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    uint8_t ihdr[13];', '    ihdr[0]  = static_cast<uint8_t>(width  >> 24); ihdr[1]  = static_cast<uint8_t>(width  >> 16);', '    ihdr[2]  = static_cast<uint8_t>(width  >>  8); ihdr[3]  = static_cast<uint8_t>(width       );', '    ihdr[4]  = static_cast<uint8_t>(height >> 24); ihdr[5]  = static_cast<uint8_t>(height >> 16);', '    ihdr[6]  = static_cast<uint8_t>(height >>  8); ihdr[7]  = static_cast<uint8_t>(height      );']
- Line 386: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete p;

### stable_diffusion/tests/test_sd_plugin.cpp
Total findings: 4

- Line 658: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    EXPECT_EQ(out_w, 8);', '    EXPECT_EQ(out_h, 8);', '    EXPECT_EQ(result.size(), static_cast<size_t>(8 * 8 * 3));', '}', '']
- Line 419: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 432: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 444: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### stable_diffusion
Total findings: 1

- Line 1: severity=MEDIUM; category=missing_module_doc
  Description: Module 'stable_diffusion' missing required governance doc 'PRODUCTION_REQUIREMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_module_governance_rules
  Context: Expected file: src/stable_diffusion/PRODUCTION_REQUIREMENTS.md

### stable_diffusion/sd_plugin_registrar.cpp
Total findings: 1

- Line 81: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return [](SDPlugin& plugin, const json& config) -> bool {

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
