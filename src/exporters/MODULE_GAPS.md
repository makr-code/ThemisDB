# exporters Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: exporters
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 58
- Actionable Findings (Critical + High): 17
- Affected Files: 14

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 5 |
| High | 12 |
| Medium | 41 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 76 |
| container | 49 |
| performance_patterns | 43 |
| memory | 23 |
| concurrency | 12 |
| llm_ai_safety | 10 |
| raii | 7 |
| performance | 6 |
| type_conversion | 6 |
| legacy_duplication | 3 |
| audit_logging | 2 |
| platform | 2 |
| security | 2 |
| input_validation | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/exporters/jsonl_llm_exporter.cpp | 16 | 5 | 9 | 2 | 0 |
| src/exporters/parquet_exporter.cpp | 12 | 0 | 1 | 11 | 0 |
| src/exporters/arrow_ipc_exporter.cpp | 11 | 0 | 0 | 11 | 0 |
| src/exporters/pii_detector.cpp | 6 | 0 | 0 | 6 | 0 |
| src/exporters/huggingface_exporter.cpp | 5 | 0 | 1 | 4 | 0 |
| src/exporters/data_augmentation.cpp | 3 | 0 | 0 | 3 | 0 |
| src/exporters/export_encryption.cpp | 1 | 0 | 0 | 1 | 0 |
| src/exporters/export_format_registry.cpp | 1 | 0 | 0 | 1 | 0 |
| src/exporters/huggingface_hub_client.cpp | 1 | 0 | 0 | 1 | 0 |
| src/exporters/stream_writer.cpp | 1 | 0 | 1 | 0 | 0 |
| src/exporters/streaming_exporter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/exporters/aql_predicate_filter.cpp | 0 | 0 | 0 | 0 | 0 |
| src/exporters/exporter_metrics.cpp | 0 | 0 | 0 | 0 | 0 |
| src/exporters/join_exporter.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/exporters/jsonl_llm_exporter.cpp
Total findings: 16

- Line 417: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Optional input field
  Confidence: band=very_high; score=0.99
- Line 418: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (isFieldAllowed(mapping.input_field, options.include_fields, options.exclude_fields)) {
  Confidence: band=very_high; score=0.99
- Line 419: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto input = entity.getFieldAsString(mapping.input_field);
  Confidence: band=very_high; score=0.99
- Line 420: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input && !input->empty()) {
  Confidence: band=very_high; score=0.99
- Line 421: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: j["input"] = *input;
  Confidence: band=very_high; score=0.99
- Line 417: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Optional input field
  Confidence: band=very_high; score=0.9
- Line 418: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (isFieldAllowed(mapping.input_field, options.include_fields, options.exclude_fields)) {
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input = entity.getFieldAsString(mapping.input_field);
  Confidence: band=very_high; score=0.9
- Line 420: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input && !input->empty()) {
  Confidence: band=very_high; score=0.9
- Line 421: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: j["input"] = *input;
  Confidence: band=very_high; score=0.9
- Line 678: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // The style-based path is unchanged for backward compatibility.
  Confidence: band=high; score=0.8
- Line 704: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Skip empty outputs
  Confidence: band=very_high; score=0.9
- Line 705: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (quality.skip_empty_outputs && (!output || output->empty())) {
  Confidence: band=very_high; score=0.9
- Line 843: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // LoRA Adapter Metadata (LoRAExchange.ai compatibility)
  Confidence: band=high; score=0.8
- Line 811: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: runtime_metrics_.validation_errors.push_back(err_msg);
  Confidence: band=high; score=0.74
- Line 1049: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["schema_validation"]["recent_errors"].push_back(runtime_metrics_.validation_errors[i]);
  Confidence: band=high; score=0.74

### src/exporters/parquet_exporter.cpp
Total findings: 12

- Line 471: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (exclude_set.find(kv.first) == exclude_set.end()) {
  Confidence: band=very_high; score=0.9
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>((u >> s) & 0xFF));
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: value_buf.push_back(static_cast<uint8_t>(len & 0xFF));
  Confidence: band=high; score=0.74
- Line 450: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 482: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(hint.name);
  Confidence: band=high; score=0.74
- Line 482: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(hint.name);
  Confidence: band=high; score=0.74
- Line 482: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(hint.name);
  Confidence: band=high; score=0.74
- Line 584: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(arrow::field(col, dt, true /* nullable */));
  Confidence: band=high; score=0.74
- Line 584: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(arrow::field(col, dt, true /* nullable */));
  Confidence: band=high; score=0.74
- Line 642: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 642: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74

### src/exporters/arrow_ipc_exporter.cpp
Total findings: 11

- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_refs.push_back(C_field);
  Confidence: band=high; score=0.74
- Line 626: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vals.push_back(fieldToString(e, col));
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.buf_offsets.push_back(body_pos);
  Confidence: band=high; score=0.74
- Line 811: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 820: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 1003: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrow_fields.push_back(arrow::field(col, arrow::utf8()));
  Confidence: band=high; score=0.74
- Line 1010: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 1050: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = builders[i]->Append(fieldToString(entity, columns[i]));
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(*maybe_arr);
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(*maybe_arr);
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(*maybe_arr);
  Confidence: band=high; score=0.74

### src/exporters/pii_detector.cpp
Total findings: 6

- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += std::string(value.length() - config_.partial_keep_prefix - config_.partial_keep_suffix, '*');
  Confidence: band=high; score=0.74

### src/exporters/huggingface_exporter.cpp
Total findings: 5

- Line 322: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (field_dtypes.find(field_name) == field_dtypes.end()) {
  Confidence: band=very_high; score=0.9
- Line 318: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> field_dtypes;
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred_features_.push_back(std::move(feat));
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred_features_.push_back(std::move(feat));
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred_features_.push_back(std::move(feat));
  Confidence: band=high; score=0.74

### src/exporters/data_augmentation.cpp
Total findings: 3

- Line 114: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ' ';
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> combined;
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(kv.first);
  Confidence: band=high; score=0.74

### src/exporters/export_encryption.cpp
Total findings: 1

- Line 49: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFFu));
  Confidence: band=high; score=0.74

### src/exporters/export_format_registry.cpp
Total findings: 1

- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74

### src/exporters/huggingface_hub_client.cpp
Total findings: 1

- Line 456: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path().string());
  Confidence: band=high; score=0.74

### src/exporters/stream_writer.cpp
Total findings: 1

- Line 119: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // GZIP is accepted for backward compatibility but produces ZSTD output.
  Confidence: band=high; score=0.8

### src/exporters/streaming_exporter.cpp
Total findings: 1

- Line 145: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: line += '\n';
  Confidence: band=high; score=0.74

### src/exporters/aql_predicate_filter.cpp
Total findings: 0


### src/exporters/exporter_metrics.cpp
Total findings: 0


### src/exporters/join_exporter.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
