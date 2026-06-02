# toolbox Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: toolbox
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 15
- Actionable Findings (Critical + High): 9
- Affected Files: 6

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 1 |
| High | 8 |
| Medium | 6 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| memory | 3 |
| oop_design | 3 |
| performance_patterns | 2 |
| platform | 2 |
| security | 2 |
| concurrency | 1 |
| container | 1 |
| observability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/toolbox/content_toolbox_bridge.cpp | 5 | 0 | 5 | 0 | 0 |
| src/toolbox/toolbox_builder.cpp | 4 | 0 | 3 | 1 | 0 |
| src/toolbox/ingestion_toolbox.cpp | 3 | 1 | 0 | 2 | 0 |
| src/toolbox/content_fingerprinter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/toolbox/language_detector.cpp | 1 | 0 | 0 | 1 | 0 |
| src/toolbox/text_chunker.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/toolbox/content_toolbox_bridge.cpp
Total findings: 5

- Line 127: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: toolbox_ptr = impl_->toolbox_;
- Line 130: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: toolbox_ptr->extractEntitySet(extracted_text, mime_type, filename);
- Line 130: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: toolbox_ptr->extractEntitySet(extracted_text, mime_type, filename);
- Line 208: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: toolbox_ptr2->extractEntitySet(extracted_text, mime_type, filename_hint);
- Line 208: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: toolbox_ptr2->extractEntitySet(extracted_text, mime_type, filename_hint);

### src/toolbox/toolbox_builder.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->format_extractors.push_back(std::move(ext));
  Confidence: band=high; score=0.74

### src/toolbox/ingestion_toolbox.cpp
Total findings: 3

- Line 213: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->extract_entities_total_ += static_cast<uint64_t>(entity_count);
- Line 227: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "# HELP toolbox_extract_calls_total Total extractEntities() / extractEntitySet() calls.\n";
- Line 235: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "# HELP toolbox_extract_entities_total Cumulative number of entities / chunks extracted.\n";

### src/toolbox/content_fingerprinter.cpp
Total findings: 1

- Line 43: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ContentFingerprint ContentFingerprinter::compute(std::string_view text) const {
  Confidence: band=high; score=0.74

### src/toolbox/language_detector.cpp
Total findings: 1

- Line 29: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(utils::toLower(word));

### src/toolbox/text_chunker.cpp
Total findings: 1

- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(c.text));
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
