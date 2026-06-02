# ingestion Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: ingestion
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 468
- Actionable Findings (Critical + High): 156
- Affected Files: 34

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 36 |
| High | 120 |
| Medium | 312 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 133 |
| reliability | 90 |
| container | 74 |
| performance | 46 |
| exception_safety | 30 |
| observability | 21 |
| platform | 18 |
| determinism | 13 |
| memory | 11 |
| raii | 10 |
| audit_logging | 6 |
| llm_ai_safety | 6 |
| concurrency | 4 |
| legacy_duplication | 4 |
| input_validation | 2 |
| security | 2 |
| uninitialized | 2 |
| oop_design | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/ingestion/ingestion_manager.cpp | 57 | 5 | 10 | 42 | 0 |
| src/ingestion/legal_domain.cpp | 42 | 0 | 6 | 36 | 0 |
| src/ingestion/workflow_engine.cpp | 30 | 0 | 5 | 25 | 0 |
| src/ingestion/ingestion_coordinator.cpp | 29 | 3 | 15 | 11 | 0 |
| src/ingestion/database_connector.cpp | 26 | 3 | 2 | 21 | 0 |
| src/ingestion/cdc_connector.cpp | 23 | 2 | 6 | 15 | 0 |
| src/ingestion/ingestion_sinks.cpp | 23 | 3 | 9 | 11 | 0 |
| src/ingestion/entity_assembler.cpp | 20 | 1 | 2 | 17 | 0 |
| src/ingestion/steps/ner_step.cpp | 20 | 0 | 5 | 15 | 0 |
| src/ingestion/filesystem_ingester.cpp | 18 | 2 | 2 | 14 | 0 |
| src/ingestion/agentic_reference_validator.cpp | 15 | 0 | 4 | 11 | 0 |
| src/ingestion/web_crawler_connector.cpp | 15 | 2 | 4 | 9 | 0 |
| src/ingestion/ingestion_quality_judge.cpp | 14 | 3 | 3 | 8 | 0 |
| src/ingestion/api_connector.cpp | 13 | 0 | 4 | 9 | 0 |
| src/ingestion/kafka_connector.cpp | 12 | 2 | 5 | 5 | 0 |
| src/ingestion/huggingface_connector.cpp | 10 | 1 | 5 | 4 | 0 |
| src/ingestion/deontic_extractor.cpp | 9 | 0 | 0 | 9 | 0 |
| src/ingestion/llm_adapter.cpp | 9 | 0 | 3 | 6 | 0 |
| src/ingestion/object_storage_connector.cpp | 9 | 0 | 5 | 4 | 0 |
| src/ingestion/s3_connector.cpp | 8 | 2 | 0 | 6 | 0 |
| src/ingestion/steps/llm_extract_step.cpp | 8 | 0 | 3 | 5 | 0 |
| src/ingestion/steps/decompress_step.cpp | 7 | 0 | 2 | 5 | 0 |
| src/ingestion/steps/legal_reference_step.cpp | 7 | 0 | 2 | 5 | 0 |
| src/ingestion/steps/parse_text_step.cpp | 7 | 4 | 2 | 1 | 0 |
| src/ingestion/semantic_validator.cpp | 5 | 0 | 0 | 5 | 0 |
| src/ingestion/steps/base_entity_assembler_step.cpp | 5 | 1 | 2 | 2 | 0 |
| src/ingestion/steps/chunk_tt_decompose_step.cpp | 5 | 0 | 2 | 3 | 0 |
| src/ingestion/steps/tensor_core_bridge_step.cpp | 5 | 2 | 2 | 1 | 0 |
| src/ingestion/oauth_token_manager.cpp | 4 | 0 | 4 | 0 | 0 |
| src/ingestion/steps/chunk_embed_step.cpp | 4 | 0 | 2 | 2 | 0 |
| src/ingestion/steps/format_parse_step.cpp | 3 | 0 | 1 | 2 | 0 |
| src/ingestion/steps/legal_metadata_step.cpp | 3 | 0 | 1 | 2 | 0 |
| src/ingestion/steps/chunk_text_step.cpp | 2 | 0 | 1 | 1 | 0 |
| src/ingestion/steps/deontic_step.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/ingestion/ingestion_manager.cpp
Total findings: 57

- Line 352: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool CheckpointStore::write(const IngestionCheckpoint& cp) {
- Line 368: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool CheckpointStore::read(const std::string& source_id,
- Line 706: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto plug = plugin_registry_.create(pit->second);
- Line 776: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (cs->read(source_id, cp)) {
- Line 1234: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return cs->read(source_id, out);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 95: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (char c : s) {
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: std::regex             key_re;     ///< compiled [{,]\\s*"name"\\s*: pattern
- Line 642: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: obj_connector->setRetryConfig(retry_config_);
- Line 643: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!obj_connector->initialize(config)) {
- Line 1195: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto& entry : fs::recursive_directory_iterator(root)) {
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 95: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (kMeta.find(c) != std::string::npos) out += '\\';
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (kMeta.find(c) != std::string::npos) out += '\\';
- Line 129: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n':  value += '\n'; break;
- Line 130: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r':  value += '\r'; break;
- Line 131: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't':  value += '\t'; break;
- Line 132: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  value += '"';  break;
- Line 133: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': value += '\\'; break;
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: compiled_fields.emplace_back(kv.first, kv.second);
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '_';
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '_';
- Line 363: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 383: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { out.processed_count = std::stoull(val); } catch (...) {}
- Line 385: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { out.byte_offset = std::stoull(val); } catch (...) {}
- Line 391: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 400: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 833: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("schema_validation");
- Line 837: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("deontic_extraction");
- Line 838: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("semantic_validation");
- Line 840: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("reference_validation");
- Line 845: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("mime_detection");
- Line 932: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_sources.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 933: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_sources.push_back(pair.second);
- Line 967: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(
  Confidence: band=high; score=0.74
- Line 1015: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1015: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1016: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(pair.second);
- Line 1369: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: quarantine_.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 1441: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 1663: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back(w);
  Confidence: band=high; score=0.74
- Line 1725: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\\') out += "\\\\";
  Confidence: band=high; score=0.74
- Line 1726: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\\') out += "\\\\";
- Line 1727: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '"')  out += "\\\"";
- Line 1728: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') out += "\\n";
- Line 1879: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, size_t> code_counts;
  Confidence: band=medium; score=0.66
- Line 1884: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: labels.push_back({"error_code", std::to_string(code_int)});
  Confidence: band=high; score=0.74
- Line 1884: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: labels.push_back({"error_code", std::to_string(code_int)});
  Confidence: band=high; score=0.74
- Line 1885: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: labels.push_back({"error_code", std::to_string(code_int)});
- Line 2154: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<SourceStatus> result;
- Line 2156: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SourceStatus s;
- Line 2166: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/ingestion/legal_domain.cpp
Total findings: 42

- Line 183: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = teil_begin; it != std::sregex_iterator(); ++it) {
- Line 243: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto e = std::sregex_iterator(); it != e; ++it) {
- Line 266: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto ae = std::sregex_iterator(); abs_it != ae; ++abs_it) {
- Line 558: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto ae = std::sregex_iterator(); ai != ae; ++ai) {
- Line 566: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto ne = std::sregex_iterator(); ni != ne; ++ni) {
- Line 703: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (safe.find(static_cast<char>(c)) != std::string::npos) {
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: teile.emplace_back(it->position(), std::move(tn));
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hier.root.children.push_back(std::move(para));
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: target_teil->children.push_back(std::move(para));
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: target_teil->children.push_back(std::move(para));
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hier.root.children.push_back(std::move(tn));
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: match_starts.push_back(static_cast<std::size_t>((*it).position()));
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: match_starts.push_back(static_cast<std::size_t>((*it).position()));
- Line 250: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto body_start = headers[i].second;
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(std::move(ae));
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"((?:Aktenzeichen|Az\.|Geschäftszeichen|AZ)[:\s]+([A-Z0-9\-/]+(?:\s[A-Z0-9\-/]+)?))",
- Line 559: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!item.empty()) be.auflagen.push_back(item);
  Confidence: band=high; score=0.74
- Line 567: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!nb.empty()) be.nebenbestimmungen.push_back(nb);
  Confidence: band=high; score=0.74
- Line 594: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) auflagen_str += "; ";
  Confidence: band=high; score=0.74
- Line 594: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) auflagen_str += "; ";
  Confidence: band=high; score=0.74
- Line 595: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) auflagen_str += "; ";
- Line 630: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const BaseEntity*> id_map;
  Confidence: band=medium; score=0.66
- Line 650: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rel));
  Confidence: band=high; score=0.74
- Line 650: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rel));
  Confidence: band=high; score=0.74
- Line 669: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rel));
  Confidence: band=high; score=0.74
- Line 708: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
- Line 719: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 719: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 720: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 721: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 722: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 723: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 767: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 767: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 787: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 800: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
- Line 801: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "@prefix dc:   <http://purl.org/dc/elements/1.1/> .\n";
- Line 802: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "@prefix themis: <https://themisdb.io/legal/> .\n";
- Line 803: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "@prefix xsd:  <http://www.w3.org/2001/XMLSchema#> .\n\n";
- Line 855: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " <https://themisdb.io/legal/" << entityTypeName(e.entity_type) << "> .\n";
- Line 855: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " <https://themisdb.io/legal/" << entityTypeName(e.entity_type) << "> .\n";

### src/ingestion/workflow_engine.cpp
Total findings: 30

- Line 290: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return it != impl_->steps_.end() ? it->second.step : nullptr;
- Line 534: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = step->execute(ctx, step_cfg);
- Line 669: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(directory_path, ec)) {
- Line 708: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<BaseEntitySet> WorkflowEngine::execute(ExtractionContext& ctx) {
- Line 708: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<BaseEntitySet> WorkflowEngine::execute(ExtractionContext& ctx) {
  Confidence: band=very_high; score=0.9
- Line 81: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::dlclose(handle);
- Line 148: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Entry> steps_;
  Confidence: band=medium; score=0.66
- Line 222: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 350: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (m.is_string()) p.file_patterns.mime_types.push_back(m);
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (fn.is_string()) p.file_patterns.filename_patterns.push_back(fn);
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: p.steps.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obj[key] = val.as<bool>(); continue; } catch (...) {}
- Line 410: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obj[key] = val.as<int64_t>(); continue; } catch (...) {}
- Line 411: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obj[key] = val.as<double>(); continue; } catch (...) {}
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (item.IsScalar()) arr.push_back(item.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (item.IsScalar()) arr.push_back(item.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 416: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (item.IsScalar()) arr.push_back(item.as<std::string>(""));
- Line 447: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (m.IsScalar()) p.file_patterns.mime_types.push_back(m.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 448: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (m.IsScalar()) p.file_patterns.mime_types.push_back(m.as<std::string>(""));
- Line 452: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (f.IsScalar()) p.file_patterns.filename_patterns.push_back(f.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 453: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (f.IsScalar()) p.file_patterns.filename_patterns.push_back(f.as<std::string>(""));
- Line 468: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: p.steps.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back("Step '" + step_cfg.name
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back("Step '" + step_cfg.name
  Confidence: band=high; score=0.74
- Line 515: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back("Step '" + step_cfg.name
- Line 528: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back("Step '" + step_cfg.name
- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back("Quality gate: only "
  Confidence: band=high; score=0.74
- Line 574: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back("Quality gate: only "
- Line 659: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->profiles_.push_back(std::move(profile));
  Confidence: band=high; score=0.74
- Line 708: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<BaseEntitySet> WorkflowEngine::execute(ExtractionContext& ctx) {
  Confidence: band=high; score=0.74

### src/ingestion/ingestion_coordinator.cpp
Total findings: 29

- Line 77: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool InMemorySharedCheckpointStore::write(const IngestionCheckpoint& cp) {
- Line 83: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool InMemorySharedCheckpointStore::read(const std::string& source_id,
- Line 593: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!checkpoint_store_->write(cp)) {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        try {', '            IngestionReport report =', '                nodes_[my_idx]->ingest({src}, target_collection_, cb);', '            std::lock_guard<std::mutex> lock(results_mtx_);', '            results_.push_back(std::move(report));']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 120: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool can_acquire = !current_lease_.isValid()
- Line 122: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!can_acquire) {
- Line 276: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 1; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(deques_[victim].mtx, std::try_to_lock);
- Line 518: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "IngestionCoordinator: failed to acquire leader lease — "
- Line 169: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::removeNode(const std::string& node_id)
  Context: void ConsistentHashRing::removeNode(const std::string& node_id) {
  Confidence: band=medium; score=0.56
- Line 181: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::getNode(const std::string& key)
  Context: std::string ConsistentHashRing::getNode(const std::string& key) const {
  Confidence: band=medium; score=0.56
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back(&WorkStealingPool::workerFn, this, i, cb);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_.push_back(node);
  Confidence: band=high; score=0.74
- Line 462: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 477: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<SourceConfig>>
  Confidence: band=medium; score=0.66
- Line 483: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<SourceConfig>> partitions;
  Confidence: band=medium; score=0.66
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partitions[node_id].push_back(src);
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.quarantine.push_back(q);
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.quarantine.push_back(q);
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.quarantine.push_back(q);
  Confidence: band=high; score=0.74

### src/ingestion/database_connector.cpp
Total findings: 26

- Line 173: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Context: if (!password.empty()) cs << "PWD=" << password << ";";
  Confidence: band=very_high; score=0.92
- Line 257: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator b may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto b = token.find_first_not_of(" \t");
- Line 258: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator e may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto e = token.find_last_not_of(" \t");
- Line 352: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ok) SQLDisconnect(hdbc);
- Line 543: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: SQLDisconnect(hdbc);
- Line 127: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { result.port = 0; }
- Line 215: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  { out += "\\\""; }
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  { out += "\\\""; }
  Confidence: band=high; score=0.74
- Line 216: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')  { out += "\\\""; }
- Line 216: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')  { out += "\\\""; }
- Line 217: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') { out += "\\\\"; }
- Line 217: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') { out += "\\\\"; }
- Line 218: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') { out += "\\n"; }
- Line 218: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') { out += "\\n"; }
- Line 219: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\r') { out += "\\r"; }
- Line 219: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\r') { out += "\\r"; }
- Line 220: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\t') { out += "\\t"; }
- Line 220: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\t') { out += "\\t"; }
- Line 243: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!text.empty()) text += ' ';
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token.substr(b, e - b + 1));
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(token.substr(b, e - b + 1));
- Line 298: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { batch_size_ = 500; }
- Line 302: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_rows_ = 0; }
- Line 305: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { timeout_s_ = 30; }
- Line 569: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: "SQLAllocHandle(STMT) failed",
  Confidence: band=high; score=0.74
- Line 606: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_names.push_back(std::string(reinterpret_cast<char*>(name),
  Confidence: band=high; score=0.74

### src/ingestion/cdc_connector.cpp
Total findings: 23

- Line 133: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator b may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto b = token.find_first_not_of(" \t");
- Line 134: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator e may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto e = token.find_last_not_of(" \t");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3249 [ingestion] Implement CDC s... (2026-03-12) | #3197 feat(ingestion): CD
- Line 180: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: /// PostgreSQL itself outputs LSN values in uppercase hexadecimal (e.g. `0/16E0478`);
  Confidence: band=very_high; score=0.9
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')       { out += "\\\""; }
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') { out += "\\\\"; }
- Line 64: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') { out += "\\n"; }
- Line 65: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\r') { out += "\\r"; }
- Line 66: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\t') { out += "\\t"; }
- Line 74: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& m) {
  Confidence: band=medium; score=0.66
- Line 120: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!text.empty()) text += ' ';
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token.substr(b, e - b + 1));
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(token.substr(b, e - b + 1));
- Line 176: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return 0; }
- Line 263: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: val += '\'';
- Line 425: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { batch_size_ = 500; }
- Line 429: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_events_ = 0; }
- Line 432: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { poll_timeout_ms_ = 1000; }
- Line 435: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_empty_polls_ = 3; }

### src/ingestion/ingestion_sinks.cpp
Total findings: 23

- Line 81: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: Result<void> IGraphWriter::write(const BaseEntitySet& entity_set) {
- Line 257: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto r = graph->write(entity_set);
- Line 523: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: Result<void> InMemoryTensorCoreBridge::write(const TensorCoreRecord& record,
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 69: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [key, value] : record.metadata) {
- Line 70: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: fields["metadata." + key] = value;
- Line 93: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = nodes_.find(n.id);
- Line 98: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Merge properties: new values overwrite existing
- Line 120: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find_if(edges_.begin(), edges_.end(),
- Line 335: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cj["metadata"]       = c.metadata;
- Line 511: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return it != last_written_records_.end() ? &it->second : nullptr;
- Line 568: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it == records_.end()) ? nullptr : &it->second;
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_.push_back(e);
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_arr.push_back(std::move(nj));
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_arr.push_back(std::move(ej));
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(ej));
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(ej));
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rj));
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rj));
  Confidence: band=high; score=0.74
- Line 339: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(cj));
  Confidence: band=high; score=0.74
- Line 419: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "': " + status.message));
- Line 535: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "illegal characters ('/' or '\\0')");
- Line 535: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "illegal characters ('/' or '\\0')");

### src/ingestion/entity_assembler.cpp
Total findings: 20

- Line 203: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = id_to_idx.find(ent.id);
- Line 202: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = id_to_idx.find(ent.id);
- Line 210: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Merge properties from existing into new (keep missing keys)
- Line 33: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::tolower(c)));
  Confidence: band=high; score=0.74
- Line 59: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string>
  Confidence: band=medium; score=0.66
- Line 68: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> parts;
  Confidence: band=medium; score=0.66
- Line 198: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> id_to_idx;
  Confidence: band=medium; score=0.66
- Line 205: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduped.push_back(std::move(ent));
  Confidence: band=high; score=0.74
- Line 266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> id_map;
  Confidence: band=medium; score=0.66
- Line 311: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 320: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> by_section;
  Confidence: band=medium; score=0.66
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: by_section[ref].push_back(ent.id);
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> authority_by_text;
  Confidence: band=medium; score=0.66
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74

### src/ingestion/steps/ner_step.cpp
Total findings: 20

- Line 65: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = std::sregex_iterator(text.begin(), text.end(), re_law);
- Line 77: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = std::sregex_iterator(text.begin(), text.end(), re_date);
- Line 89: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = std::sregex_iterator(text.begin(), text.end(), re_az);
- Line 149: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *  - `entity_types` array   subset of [ORG, PER, LAW, DATE, LOCATION]
- Line 177: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({it->str(), "LAW",
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({it->str(), "LAW",
- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({it->str(), "DATE",
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({it->str(), "DATE",
- Line 87: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(\bAz\.\s*[A-Z]\s*\d+\s*/\s*\d{2,4}\b)",
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({it->str(), "LAW",
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({it->str(), "LAW",
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!m.text.empty()) out.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 121: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (const auto& t : types) type_list += t + ", ";
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: requested_types.push_back(t.get<std::string>());
  Confidence: band=high; score=0.74
- Line 195: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: texts_with_refs.emplace_back(c.text, c.section_ref);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back(
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back(

### src/ingestion/filesystem_ingester.cpp
Total findings: 18

- Line 340: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ocr_config_.enabled = (it->second == "true");
- Line 345: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ocr_config_.language = it->second;
- Line 88: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& component : fs::path(path)) {
- Line 735: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->setMetadataExtraction(enabled);
- Line 116: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 193: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ' ';
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 241: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') escaped += "\\\"";
  Confidence: band=high; score=0.74
- Line 242: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"') escaped += "\\\"";
- Line 252: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\'') escaped += "'\"'\"'";
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\'') escaped += "'\"'\"'";
- Line 403: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 435: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_to_process.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 452: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files_to_process.push_back(entry.path());
- Line 459: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_to_process.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 460: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files_to_process.push_back(entry.path());
- Line 673: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/ingestion/agentic_reference_validator.cpp
Total findings: 15

- Line 115: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 136: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 155: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 173: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 42: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "Richtlinie\\s+(\\d+)/(\\d+)/EU",
- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.validated.push_back(vr);
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: report.warnings.push_back(
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 165: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 165: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74

### src/ingestion/web_crawler_connector.cpp
Total findings: 15

- Line 221: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator val_end may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto val_end = tag.find(quote, val_start);
- Line 251: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator end may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto end = xml.find(close, val_start);
- Line 186: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto a_pos = html.find('<', pos);
- Line 460: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& loc : extractSitemapLocs(sbody)) {
- Line 466: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& loc : extractSitemapLocs(sbody)) {
- Line 537: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& href : extractHrefs(body)) {
- Line 145: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!text.empty() && text.back() != ' ') text += ' ';
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!text.empty() && text.back() != ' ') text += ' ';
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!text.empty() && text.back() != ' ') text += ' ';
- Line 169: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!prev_space) { result += ' '; prev_space = true; }
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!prev_space) { result += ' '; prev_space = true; }
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hrefs.push_back(tag.substr(val_start, val_end - val_start));
- Line 231: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hrefs.push_back(tag.substr(val_start, val_end - val_start));
- Line 383: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_depth_ = 3; }
- Line 385: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_pages_ = 0; }

### src/ingestion/ingestion_quality_judge.cpp
Total findings: 14

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 441: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator content_start may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto content_start = line.find_first_not_of("-* \t", 1);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 294: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(ctx.chunks.size(), size_t{5}); ++i)
- Line 526: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(unique_steps.begin(), unique_steps.end(), s)
- Line 390: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 440: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (line[0] == '-' || line[0] == '*' || line[0] == '\xe2' /* UTF-8 bullet */) {
- Line 443: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(line.substr(content_start));
- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unique_steps.push_back(s);
  Confidence: band=high; score=0.74
- Line 549: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obs->onQualityEvaluated(doc_id, report); } catch (...) {}
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.history.push_back(report);
  Confidence: band=high; score=0.74
- Line 706: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obs->onReIngestionTriggered(doc_id, attempt, reasons); } catch (...) {}
- Line 717: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obs->onReIngestionComplete(doc_id, attempt, improved); } catch (...) {}

### src/ingestion/api_connector.cpp
Total findings: 13

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 46: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, total);
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.errors.push_back(err);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!value.empty()) results.push_back(std::move(value));
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { page_size_ = 100; }
- Line 280: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_pages_ = 0; }
- Line 333: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 349: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 538: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
- Line 550: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += "&client_id=" + urlEncode(oauth_config_.client_id);
  Confidence: band=high; score=0.74
- Line 596: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GenericApiConnector::initialize(const SourceConfig& config) {
  Confidence: band=medium; score=0.66

### src/ingestion/kafka_connector.cpp
Total findings: 12

- Line 66: severity=CRITICAL; category=missing_dtor
  Description: Class KafkaConnector allocates resources but has no destructor
  Remediation: Add explicit destructor: ~KafkaConnector() { /* cleanup */ }
  Context: class/struct KafkaConnector
- Line 221: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: checkpoint_store_->write(cp);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        // 32 bytes provides extra headroom for locale-specific variations.', '        constexpr std::size_t kTimestampBufSize = 32;', '        char buf[kTimestampBufSize] = {};', '        std::strftime(buf, kTimestampBufSize, "%Y-%m-%dT%H:%M:%SZ", &tm_buf);', '        cp.timestamp = buf;']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4188 feat(ingestion): Kafka Cons... (2026-03-13) | #3287 security(ingestion)
- Line 98: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { poll_timeout_ms_ = 1000; }
- Line 101: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_messages_ = 0; }
- Line 104: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { session_timeout_ms_ = 10000; }
- Line 396: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rd_kafka_consumer_close(rk);
- Line 472: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rd_kafka_consumer_close(rk);

### src/ingestion/huggingface_connector.cpp
Total findings: 10

- Line 275: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: streaming_enabled_ = (it->second == "true");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 41: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, total);
- Line 675: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // (degraded mode) to preserve backward compatibility with existing connectors.
  Confidence: band=high; score=0.8
- Line 311: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 340: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 427: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
- Line 439: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += "&client_id=" + urlEncode(oauth_config_.client_id);
  Confidence: band=high; score=0.74

### src/ingestion/deontic_extractor.cpp
Total findings: 9

- Line 114: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "|Richtlinie\\s+\\d+/\\d+/EU") },
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deontic_categories.push_back(dp.category);
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.deontic_categories.push_back(dp.category);
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back(
- Line 202: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back("No deontic patterns matched in text");
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.obligations.emplace_back(actor, action, "", 0.75);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.emplace_back(ep.type, raw, raw, 0.85);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.emplace_back(ep.type, raw, raw, 0.85);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.emplace_back(ep.type, raw, raw, 0.85);
  Confidence: band=high; score=0.74

### src/ingestion/llm_adapter.cpp
Total findings: 9

- Line 17: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // See include/ingestion/inference_backend.h and llm/llm_ingestion_bridge.h.
  Confidence: band=very_high; score=0.9
- Line 59: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [captured_backend, captured_config](const std::string& text) -> DeonticExtraction {
- Line 61: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::string response = captured_backend->generate(
  Confidence: band=very_high; score=0.9
- Line 61: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: const std::string response = captured_backend->generate(
  Confidence: band=high; score=0.74
- Line 70: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string("ITextGenerationBackend::generate() threw: ") + e.what());
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "Gesetzestext:\n" + text + "\n[/INST]";
- Line 110: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "Gesetzestext:\n" + text + "\n[/INST]";
- Line 149: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.entities.emplace_back(type_str, value_str, value_str, 0.85);
  Confidence: band=high; score=0.74

### src/ingestion/object_storage_connector.cpp
Total findings: 9

- Line 478: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto it = client.ListObjects(bucket_, gcs::MaxResults(1),
- Line 495: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: for (auto& obj_meta : client.ListObjects(bucket_,
- Line 564: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto props = container_client.GetProperties();
- Line 602: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto blob_client = container_client.GetBlockBlobClient(key);
- Line 604: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto dl = blob_client.Download(dl_opts);
- Line 143: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 358: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 483: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 566: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/ingestion/s3_connector.cpp
Total findings: 8

- Line 297: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (checkpoint_store_->read(config_.source_id, cp) &&
- Line 365: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: checkpoint_store_->write(cp);
- Line 251: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 259: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futs.push_back(std::async(std::launch::async, [&fetcher, key]() {
  Confidence: band=high; score=0.74
- Line 508: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: safe_keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 619: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(obj.GetKey());
  Confidence: band=high; score=0.74

### src/ingestion/steps/llm_extract_step.cpp
Total findings: 8

- Line 12: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "ingestion/inference_backend.h"
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 90: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 81: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool canHandle(const ExtractionContext& ctx) const override {
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: backend_->generate(prompt, max_tokens, temperature, lora);
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.entities.push_back(std::move(ent));
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/ingestion/steps/decompress_step.cpp
Total findings: 7

- Line 130: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 130: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 61: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static bool runProcess(const std::vector<const char*>& argv_vec) {
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(entry.path().string());
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(entry.path().string());
- Line 130: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "decompress: extraction tool exited with non-zero status for '" + source + "'");

### src/ingestion/steps/legal_reference_step.cpp
Total findings: 7

- Line 52: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 52: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 52: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warnings_arr.push_back(w);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warnings_arr.push_back(w);
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back(
  Confidence: band=high; score=0.74
- Line 92: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back(

### src/ingestion/steps/parse_text_step.cpp
Total findings: 7

- Line 27: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: *  - TXT / MD / HTML: direct file read (HTML tags stripped for MD/TXT)
- Line 38: severity=CRITICAL; category=missing_dtor
  Description: Class ParseTextStep allocates resources but has no destructor
  Remediation: Add explicit destructor: ~ParseTextStep() { /* cleanup */ }
  Context: class/struct ParseTextStep
- Line 87: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: return new ParseTextStep();
- Line 87: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new ParseTextStep();
- Line 51: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx,
- Line 90: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: p = nullptr;
  Context: delete p;
- Line 90: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete p;

### src/ingestion/semantic_validator.cpp
Total findings: 5

- Line 57: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (c == ' ' || c == '/' || c == '\\') c = '_';
- Line 57: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (c == ' ' || c == '/' || c == '\\') c = '_';
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back(w);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_result.provisions.push_back(std::move(prov));
  Confidence: band=high; score=0.74
- Line 284: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_result.warnings.push_back(w);
  Confidence: band=high; score=0.74

### src/ingestion/steps/base_entity_assembler_step.cpp
Total findings: 5

- Line 73: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = id_to_idx.find(ent.id);
- Line 72: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = id_to_idx.find(ent.id);
- Line 72: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = id_to_idx.find(ent.id);
- Line 68: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> id_to_idx;
  Confidence: band=medium; score=0.66
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduped.push_back(std::move(ent));
  Confidence: band=high; score=0.74

### src/ingestion/steps/chunk_tt_decompose_step.cpp
Total findings: 5

- Line 69: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 69: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 69: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back(
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back(

### src/ingestion/steps/tensor_core_bridge_step.cpp
Total findings: 5

- Line 16: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: * `sink->write(record, tenant_id)` to persist the pre-computed TT-cores.
- Line 107: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto res = sink_->write(record, effective_tenant);
- Line 70: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 70: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 70: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74

### src/ingestion/oauth_token_manager.cpp
Total findings: 4

- Line 172: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
- Line 180: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw OAuthRefreshExpiredError(
- Line 213: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
- Line 221: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw OAuthRefreshExpiredError(

### src/ingestion/steps/chunk_embed_step.cpp
Total findings: 4

- Line 56: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 56: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 56: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.embeddings.push_back(std::move(rec));
  Confidence: band=high; score=0.74

### src/ingestion/steps/format_parse_step.cpp
Total findings: 3

- Line 87: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx,
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 72: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool canHandle(const ExtractionContext& ctx) const override {
  Confidence: band=high; score=0.74

### src/ingestion/steps/legal_metadata_step.cpp
Total findings: 3

- Line 44: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx,
- Line 56: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::string(R"([A-Z]\s?\d+/\d{2,4})"));
- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.entities.push_back(std::move(ent));
  Confidence: band=high; score=0.74

### src/ingestion/steps/chunk_text_step.cpp
Total findings: 2

- Line 46: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx,
- Line 174: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.chunks.push_back(std::move(c));
  Confidence: band=high; score=0.74

### src/ingestion/steps/deontic_step.cpp
Total findings: 1

- Line 58: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx,

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
