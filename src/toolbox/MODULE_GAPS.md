# toolbox Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: toolbox
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 12
- Actionable Findings (Critical + High): 6
- Affected Files: 7

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 5 |
| Medium | 4 |
| Low | 2 |

## Category Summary

| Category | Count |
|---|---:|
| virtual_call_in_ctor_dtor | 3 |
| hardcoded_path | 2 |
| module_doc_linkset_drift | 2 |
| null_dereference | 2 |
| copy_overhead | 1 |
| data_race | 1 |
| missing_latency_metric | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| toolbox/ingestion_toolbox.cpp | 3 | 1 | 0 | 2 | 0 |
| toolbox/toolbox_builder.cpp | 3 | 0 | 3 | 0 | 0 |
| toolbox/content_toolbox_bridge.cpp | 2 | 0 | 2 | 0 | 0 |
| toolbox/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| toolbox/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| toolbox/content_fingerprinter.cpp | 1 | 0 | 0 | 1 | 0 |
| toolbox/language_detector.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### toolbox/ingestion_toolbox.cpp
Total findings: 3

- Line 213: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->extract_entities_total_ += static_cast<uint64_t>(entity_count);
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "# HELP toolbox_extract_calls_total Total extractEntities() / extractEntitySet() calls.\n";
- Line 235: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "# HELP toolbox_extract_entities_total Cumulative number of entities / chunks extracted.\n";

### toolbox/toolbox_builder.cpp
Total findings: 3

- Line 245: severity=HIGH; category=virtual_call_in_ctor_dtor
  Description: If this member function is virtual, dispatch in ctor/dtor can be unsafe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 246: severity=HIGH; category=virtual_call_in_ctor_dtor
  Description: If this member function is virtual, dispatch in ctor/dtor can be unsafe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 247: severity=HIGH; category=virtual_call_in_ctor_dtor
  Description: If this member function is virtual, dispatch in ctor/dtor can be unsafe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop

### toolbox/content_toolbox_bridge.cpp
Total findings: 2

- Line 130: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: toolbox_ptr->extractEntitySet(extracted_text, mime_type, filename);
- Line 208: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: toolbox_ptr2->extractEntitySet(extracted_text, mime_type, filename_hint);

### toolbox/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### toolbox/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### toolbox/content_fingerprinter.cpp
Total findings: 1

- Line 43: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ContentFingerprint ContentFingerprinter::compute(std::string_view text) const {

### toolbox/language_detector.cpp
Total findings: 1

- Line 29: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back(utils::toLower(word));

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
