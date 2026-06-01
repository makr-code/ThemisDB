# toolbox Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: toolbox
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 34
- Actionable Findings (Critical + High): 25
- Affected Files: 7

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 3 |
| High | 22 |
| Medium | 9 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 10 |
| memory | 5 |
| container | 4 |
| security | 4 |
| concurrency | 3 |
| oop_design | 3 |
| performance_patterns | 2 |
| platform | 2 |
| observability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/toolbox/toolbox_builder.cpp | 13 | 0 | 10 | 3 | 0 |
| src/toolbox/content_toolbox_bridge.cpp | 12 | 2 | 10 | 0 | 0 |
| src/toolbox/ingestion_toolbox.cpp | 3 | 1 | 0 | 2 | 0 |
| src/toolbox/text_chunker.cpp | 2 | 0 | 0 | 2 | 0 |
| src/toolbox/toolbox_composite.cpp | 2 | 0 | 2 | 0 | 0 |
| src/toolbox/content_fingerprinter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/toolbox/language_detector.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/toolbox/toolbox_builder.cpp
Total findings: 13

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 53: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ToolboxBuilder::withWorkflowProfile: path must not be empty");
- Line 98: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ToolboxBuilder::withWorkflowEngine: engine must not be null");
- Line 108: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ToolboxBuilder::withFormatExtractor: extractor must not be null");
- Line 118: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ToolboxBuilder::withFormatExtractorFactory: factory must not be null");
- Line 139: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error("ToolboxBuilder::build() called more than once");
- Line 156: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto registerFormatStep = [&](std::shared_ptr<ingestion::IFormatExtractor> ext) {
- Line 259: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error(
- Line 110: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->format_extractors.push_back(std::move(extractor));
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->format_extractors.push_back(std::move(ext));
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->format_extractors.push_back(std::move(ext));

### src/toolbox/content_toolbox_bridge.cpp
Total findings: 12

- Line 139: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto res = impl_->graph_writer_->writeEntities(entity_set.nodes);
- Line 216: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto res = impl_->graph_writer_->writeEntities(entity_set.nodes);
- Line 56: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ContentToolboxBridge: toolbox must not be null");
- Line 59: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ContentToolboxBridge: content_manager must not be null");
- Line 129: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: toolbox_ptr = impl_->toolbox_;
- Line 129: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: toolbox_ptr = impl_->toolbox_;
- Line 132: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: toolbox_ptr->extractEntitySet(extracted_text, mime_type, filename);
- Line 132: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: toolbox_ptr->extractEntitySet(extracted_text, mime_type, filename);
- Line 207: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: toolbox_ptr2 = impl_->toolbox_;
- Line 207: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: toolbox_ptr2 = impl_->toolbox_;
- Line 210: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: toolbox_ptr2->extractEntitySet(extracted_text, mime_type, filename_hint);
- Line 210: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: toolbox_ptr2->extractEntitySet(extracted_text, mime_type, filename_hint);

### src/toolbox/ingestion_toolbox.cpp
Total findings: 3

- Line 215: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->extract_entities_total_ += static_cast<uint64_t>(entity_count);
- Line 229: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "# HELP toolbox_extract_calls_total Total extractEntities() / extractEntitySet() calls.\n";
- Line 237: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "# HELP toolbox_extract_entities_total Cumulative number of entities / chunks extracted.\n";

### src/toolbox/text_chunker.cpp
Total findings: 2

- Line 54: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(c.text));
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(c.text));

### src/toolbox/toolbox_composite.cpp
Total findings: 2

- Line 75: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 91: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error(

### src/toolbox/content_fingerprinter.cpp
Total findings: 1

- Line 45: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ContentFingerprint ContentFingerprinter::compute(std::string_view text) const {
  Confidence: band=high; score=0.74

### src/toolbox/language_detector.cpp
Total findings: 1

- Line 31: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(utils::toLower(word));

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
