# ingestion Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: ingestion
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 626
- Actionable Findings (Critical + High): 230
- Affected Files: 34

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 54 |
| High | 176 |
| Medium | 396 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 166 |
| performance_patterns | 152 |
| reliability | 103 |
| performance | 46 |
| exception_safety | 30 |
| observability | 21 |
| platform | 18 |
| security | 18 |
| memory | 14 |
| determinism | 13 |
| concurrency | 12 |
| raii | 12 |
| audit_logging | 6 |
| llm_ai_safety | 6 |
| legacy_duplication | 4 |
| input_validation | 2 |
| uninitialized | 2 |
| oop_design | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/ingestion/ingestion_manager.cpp | 75 | 9 | 17 | 49 | 0 |
| src/ingestion/legal_domain.cpp | 57 | 1 | 7 | 49 | 0 |
| src/ingestion/ingestion_sinks.cpp | 54 | 9 | 28 | 17 | 0 |
| src/ingestion/workflow_engine.cpp | 45 | 0 | 13 | 32 | 0 |
| src/ingestion/ingestion_coordinator.cpp | 43 | 5 | 22 | 16 | 0 |
| src/ingestion/entity_assembler.cpp | 29 | 2 | 2 | 25 | 0 |
| src/ingestion/database_connector.cpp | 27 | 3 | 2 | 22 | 0 |
| src/ingestion/cdc_connector.cpp | 23 | 2 | 6 | 15 | 0 |
| src/ingestion/web_crawler_connector.cpp | 23 | 6 | 5 | 12 | 0 |
| src/ingestion/filesystem_ingester.cpp | 22 | 2 | 3 | 17 | 0 |
| src/ingestion/steps/ner_step.cpp | 22 | 0 | 5 | 17 | 0 |
| src/ingestion/agentic_reference_validator.cpp | 21 | 0 | 5 | 16 | 0 |
| src/ingestion/api_connector.cpp | 17 | 0 | 6 | 11 | 0 |
| src/ingestion/ingestion_quality_judge.cpp | 17 | 3 | 5 | 9 | 0 |
| src/ingestion/huggingface_connector.cpp | 13 | 1 | 7 | 5 | 0 |
| src/ingestion/semantic_validator.cpp | 12 | 0 | 0 | 12 | 0 |
| src/ingestion/kafka_connector.cpp | 11 | 2 | 4 | 5 | 0 |
| src/ingestion/s3_connector.cpp | 11 | 2 | 0 | 9 | 0 |
| src/ingestion/llm_adapter.cpp | 10 | 0 | 3 | 7 | 0 |
| src/ingestion/object_storage_connector.cpp | 10 | 0 | 6 | 4 | 0 |
| src/ingestion/deontic_extractor.cpp | 9 | 0 | 0 | 9 | 0 |
| src/ingestion/steps/llm_extract_step.cpp | 9 | 0 | 3 | 6 | 0 |
| src/ingestion/oauth_token_manager.cpp | 8 | 0 | 8 | 0 | 0 |
| src/ingestion/steps/chunk_tt_decompose_step.cpp | 8 | 0 | 3 | 5 | 0 |
| src/ingestion/steps/decompress_step.cpp | 8 | 0 | 2 | 6 | 0 |
| src/ingestion/steps/legal_reference_step.cpp | 8 | 0 | 2 | 6 | 0 |
| src/ingestion/steps/parse_text_step.cpp | 7 | 4 | 2 | 1 | 0 |
| src/ingestion/steps/base_entity_assembler_step.cpp | 6 | 1 | 2 | 3 | 0 |
| src/ingestion/steps/chunk_embed_step.cpp | 5 | 0 | 2 | 3 | 0 |
| src/ingestion/steps/tensor_core_bridge_step.cpp | 5 | 2 | 2 | 1 | 0 |
| src/ingestion/steps/legal_metadata_step.cpp | 4 | 0 | 1 | 3 | 0 |
| src/ingestion/steps/chunk_text_step.cpp | 3 | 0 | 1 | 2 | 0 |
| src/ingestion/steps/format_parse_step.cpp | 3 | 0 | 1 | 2 | 0 |
| src/ingestion/steps/deontic_step.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/ingestion/ingestion_manager.cpp
Total findings: 75

- Line 351: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool CheckpointStore::write(const IngestionCheckpoint& cp) {
- Line 367: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool CheckpointStore::read(const std::string& source_id,
- Line 432: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 705: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto plug = plugin_registry_.create(pit->second);
- Line 775: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (cs->read(source_id, cp)) {
- Line 812: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: cs->write(cp);
- Line 1046: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = schema_configs_.find(source_id);
- Line 1233: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return cs->read(source_id, out);
- Line 1431: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = factories_.find(plugin_name);
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
- Line 94: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (char c : s) {
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: std::regex             key_re;     ///< compiled [{,]\\s*"name"\\s*: pattern
- Line 641: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: obj_connector->setRetryConfig(retry_config_);
- Line 642: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!obj_connector->initialize(config)) {
- Line 985: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& config : enabled_sources) {
  Confidence: band=very_high; score=0.9
- Line 995: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& config : disabled_sources) {
  Confidence: band=very_high; score=0.9
- Line 1194: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto& entry : fs::recursive_directory_iterator(root)) {
- Line 1287: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 1357: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& err : stats.errors) {
  Confidence: band=very_high; score=0.9
- Line 1414: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 1662: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& w : ref_report.warnings) {
  Confidence: band=very_high; score=0.9
- Line 2267: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy fallback: re-run the entire source from the last checkpoint.
  Confidence: band=high; score=0.8
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 94: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (kMeta.find(c) != std::string::npos) out += '\\';
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (kMeta.find(c) != std::string::npos) out += '\\';
- Line 128: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n':  value += '\n'; break;
- Line 129: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r':  value += '\r'; break;
- Line 130: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't':  value += '\t'; break;
- Line 131: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  value += '"';  break;
- Line 132: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': value += '\\'; break;
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: compiled_fields.emplace_back(kv.first, kv.second);
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += '_';
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += '_';
- Line 362: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 382: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { out.processed_count = std::stoull(val); } catch (...) {}
- Line 384: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { out.byte_offset = std::stoull(val); } catch (...) {}
- Line 390: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 399: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 832: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("schema_validation");
- Line 836: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("deontic_extraction");
- Line 837: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("semantic_validation");
- Line 839: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("reference_validation");
- Line 844: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("mime_detection");
- Line 931: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_sources.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 932: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_sources.push_back(pair.second);
- Line 948: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: enabled_sources.push_back(cfg);
- Line 950: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: disabled_sources.push_back(cfg);
- Line 966: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(
  Confidence: band=high; score=0.74
- Line 967: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(
- Line 1014: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1014: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pair.second);
  Confidence: band=high; score=0.74
- Line 1015: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(pair.second);
- Line 1368: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: quarantine_.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 1369: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: quarantine_.push_back(std::move(entry));
- Line 1440: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 1441: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(kv.first);
- Line 1662: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back(w);
  Confidence: band=high; score=0.74
- Line 1663: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back(w);
- Line 1724: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\\') out += "\\\\";
  Confidence: band=high; score=0.74
- Line 1725: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\\') out += "\\\\";
- Line 1726: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '"')  out += "\\\"";
- Line 1727: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') out += "\\n";
- Line 1878: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, size_t> code_counts;
  Confidence: band=medium; score=0.66
- Line 1883: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: labels.push_back({"error_code", std::to_string(code_int)});
  Confidence: band=high; score=0.74
- Line 1883: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: labels.push_back({"error_code", std::to_string(code_int)});
  Confidence: band=high; score=0.74
- Line 1884: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: labels.push_back({"error_code", std::to_string(code_int)});
- Line 2153: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<SourceStatus> result;
- Line 2155: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SourceStatus s;
- Line 2165: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 2169: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(s));

### src/ingestion/legal_domain.cpp
Total findings: 57

- Line 641: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = id_map.find(normId(e1.id));
- Line 182: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = teil_begin; it != std::sregex_iterator(); ++it) {
- Line 242: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto e = std::sregex_iterator(); it != e; ++it) {
- Line 265: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto ae = std::sregex_iterator(); abs_it != ae; ++abs_it) {
- Line 557: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto ae = std::sregex_iterator(); ai != ae; ++ai) {
- Line 565: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto ne = std::sregex_iterator(); ni != ne; ++ni) {
- Line 702: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (safe.find(static_cast<char>(c)) != std::string::npos) {
- Line 707: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
  Confidence: band=very_high; score=0.9
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: teile.emplace_back(it->position(), std::move(tn));
  Confidence: band=high; score=0.74
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hier.root.children.push_back(std::move(para));
  Confidence: band=high; score=0.74
- Line 194: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hier.root.children.push_back(std::move(para));
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: target_teil->children.push_back(std::move(para));
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: target_teil->children.push_back(std::move(para));
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: target_teil->children.push_back(std::move(para));
- Line 214: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hier.root.children.push_back(std::move(para));
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hier.root.children.push_back(std::move(tn));
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hier.root.children.push_back(std::move(tn));
- Line 242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: match_starts.push_back(static_cast<std::size_t>((*it).position()));
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: match_starts.push_back(static_cast<std::size_t>((*it).position()));
- Line 249: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto body_start = headers[i].second;
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: para.children.push_back(std::move(abs));
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(node));
- Line 337: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back(std::move(ae));
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back(std::move(ae));
- Line 518: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"((?:Aktenzeichen|Az\.|Geschäftszeichen|AZ)[:\s]+([A-Z0-9\-/]+(?:\s[A-Z0-9\-/]+)?))",
- Line 558: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!item.empty()) be.auflagen.push_back(item);
  Confidence: band=high; score=0.74
- Line 559: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!item.empty()) be.auflagen.push_back(item);
- Line 566: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!nb.empty()) be.nebenbestimmungen.push_back(nb);
  Confidence: band=high; score=0.74
- Line 567: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!nb.empty()) be.nebenbestimmungen.push_back(nb);
- Line 593: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) auflagen_str += "; ";
  Confidence: band=high; score=0.74
- Line 593: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) auflagen_str += "; ";
  Confidence: band=high; score=0.74
- Line 594: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) auflagen_str += "; ";
- Line 629: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, const BaseEntity*> id_map;
  Confidence: band=medium; score=0.66
- Line 649: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rel));
  Confidence: band=high; score=0.74
- Line 649: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rel));
  Confidence: band=high; score=0.74
- Line 650: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(std::move(rel));
- Line 668: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rel));
  Confidence: band=high; score=0.74
- Line 669: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(std::move(rel));
- Line 707: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
- Line 718: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 718: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 719: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 720: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 721: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 722: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 766: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 766: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 767: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: graph.push_back(std::move(node));
- Line 786: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: graph.push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 787: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: graph.push_back(std::move(edge));
- Line 799: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
- Line 800: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "@prefix dc:   <http://purl.org/dc/elements/1.1/> .\n";
- Line 801: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "@prefix themis: <https://themisdb.io/legal/> .\n";
- Line 802: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: out << "@prefix xsd:  <http://www.w3.org/2001/XMLSchema#> .\n\n";
- Line 854: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " <https://themisdb.io/legal/" << entityTypeName(e.entity_type) << "> .\n";
- Line 854: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " <https://themisdb.io/legal/" << entityTypeName(e.entity_type) << "> .\n";

### src/ingestion/ingestion_sinks.cpp
Total findings: 54

- Line 80: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: Result<void> IGraphWriter::write(const BaseEntitySet& entity_set) {
- Line 171: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = records_.find(chunk_id);
- Line 239: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = docs_.find(doc_id);
- Line 256: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto r = graph->write(entity_set);
- Line 256: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto r = graph->write(entity_set);
- Line 260: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto r = vector->writeVectors(entity_set.chunks);
- Line 264: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto r = doc->writeDocument(entity_set, collection);
- Line 522: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: Result<void> InMemoryTensorCoreBridge::write(const TensorCoreRecord& record,
- Line 566: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = records_.find(makeKey(tenant_id, chunk_id));
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 68: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [key, value] : record.metadata) {
- Line 69: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: fields["metadata." + key] = value;
- Line 92: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = nodes_.find(n.id);
- Line 97: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Merge properties: new values overwrite existing
- Line 99: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [k, v] : n.properties) {
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& e : edges) {
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find_if(edges_.begin(), edges_.end(),
- Line 129: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [k, v] : e.properties)
  Confidence: band=very_high; score=0.9
- Line 137: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mtx_);
- Line 158: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& rec : records) {
  Confidence: band=very_high; score=0.9
- Line 172: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return it != records_.end() ? &it->second : nullptr;
- Line 172: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return it != records_.end() ? &it->second : nullptr;
- Line 206: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& e : es.edges) {
  Confidence: band=very_high; score=0.9
- Line 279: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DocumentStoreSinkAdapter: store must not be null");
- Line 334: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cj["metadata"]       = c.metadata;
- Line 389: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("GraphStoreSinkAdapter: db must not be null");
- Line 392: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("GraphStoreSinkAdapter: graph_index must not be null");
- Line 398: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& node : nodes) {
  Confidence: band=very_high; score=0.9
- Line 400: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!db_->put(node_key_prefix_ + node.id, storage_node.serialize())) {
- Line 412: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& edge : edges) {
  Confidence: band=very_high; score=0.9
- Line 443: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("VectorIndexSinkAdapter: vector_index must not be null");
- Line 446: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("VectorIndexSinkAdapter: object_name must not be empty");
- Line 449: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("VectorIndexSinkAdapter: dimension must be > 0");
- Line 510: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return it != last_written_records_.end() ? &it->second : nullptr;
- Line 510: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return it != last_written_records_.end() ? &it->second : nullptr;
- Line 567: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return (it == records_.end()) ? nullptr : &it->second;
- Line 567: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it == records_.end()) ? nullptr : &it->second;
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_.push_back(e);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges_.push_back(e);
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_arr.push_back(std::move(nj));
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes_arr.push_back(std::move(nj));
- Line 211: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges_arr.push_back(std::move(ej));
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges_arr.push_back(std::move(ej));
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(ej));
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(ej));
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(std::move(ej));
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rj));
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(rj));
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edges.push_back(std::move(rj));
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(cj));
  Confidence: band=high; score=0.74
- Line 339: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(cj));
- Line 418: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "': " + status.message));
- Line 534: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "illegal characters ('/' or '\\0')");
- Line 534: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "illegal characters ('/' or '\\0')");

### src/ingestion/workflow_engine.cpp
Total findings: 45

- Line 154: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [name, entry] : impl_->steps_) {
  Confidence: band=very_high; score=0.9
- Line 179: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->steps_[plugin_name] = Impl::Entry{std::move(step), nullptr};
- Line 179: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->steps_[plugin_name] = Impl::Entry{std::move(step), nullptr};
- Line 281: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->steps_[plugin_name] = Impl::Entry{std::move(step), handle};
- Line 289: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return it != impl_->steps_.end() ? it->second.step : nullptr;
- Line 289: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return it != impl_->steps_.end() ? it->second.step : nullptr;
- Line 301: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [name, _] : impl_->steps_) names.push_back(name);
  Confidence: band=very_high; score=0.9
- Line 313: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it->second.dl_handle) {
- Line 314: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: closeDynamicLibrary(it->second.dl_handle);
- Line 533: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = step->execute(ctx, step_cfg);
- Line 668: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(directory_path, ec)) {
- Line 707: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<BaseEntitySet> WorkflowEngine::execute(ExtractionContext& ctx) {
- Line 707: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<BaseEntitySet> WorkflowEngine::execute(ExtractionContext& ctx) {
  Confidence: band=very_high; score=0.9
- Line 80: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::dlclose(handle);
- Line 147: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, Entry> steps_;
  Confidence: band=medium; score=0.66
- Line 221: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& [name, _] : impl_->steps_) names.push_back(name);
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (m.is_string()) p.file_patterns.mime_types.push_back(m);
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (m.is_string()) p.file_patterns.mime_types.push_back(m);
- Line 354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (fn.is_string()) p.file_patterns.filename_patterns.push_back(fn);
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (fn.is_string()) p.file_patterns.filename_patterns.push_back(fn);
- Line 370: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: p.steps.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: p.steps.push_back(std::move(sc));
- Line 408: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obj[key] = val.as<bool>(); continue; } catch (...) {}
- Line 409: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obj[key] = val.as<int64_t>(); continue; } catch (...) {}
- Line 410: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obj[key] = val.as<double>(); continue; } catch (...) {}
- Line 414: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (item.IsScalar()) arr.push_back(item.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 414: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (item.IsScalar()) arr.push_back(item.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (item.IsScalar()) arr.push_back(item.as<std::string>(""));
- Line 446: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (m.IsScalar()) p.file_patterns.mime_types.push_back(m.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (m.IsScalar()) p.file_patterns.mime_types.push_back(m.as<std::string>(""));
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (f.IsScalar()) p.file_patterns.filename_patterns.push_back(f.as<std::string>(""));
  Confidence: band=high; score=0.74
- Line 452: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (f.IsScalar()) p.file_patterns.filename_patterns.push_back(f.as<std::string>(""));
- Line 467: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: p.steps.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 468: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: p.steps.push_back(std::move(sc));
- Line 513: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back("Step '" + step_cfg.name
  Confidence: band=high; score=0.74
- Line 513: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back("Step '" + step_cfg.name
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back("Step '" + step_cfg.name
- Line 527: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back("Step '" + step_cfg.name
- Line 572: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back("Quality gate: only "
  Confidence: band=high; score=0.74
- Line 573: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back("Quality gate: only "
- Line 658: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->profiles_.push_back(std::move(profile));
  Confidence: band=high; score=0.74
- Line 659: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->profiles_.push_back(std::move(profile));
- Line 695: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& p : impl_->profiles_) names.push_back(p.name);
- Line 707: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<BaseEntitySet> WorkflowEngine::execute(ExtractionContext& ctx) {
  Confidence: band=high; score=0.74

### src/ingestion/ingestion_coordinator.cpp
Total findings: 43

- Line 76: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool InMemorySharedCheckpointStore::write(const IngestionCheckpoint& cp) {
- Line 82: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool InMemorySharedCheckpointStore::read(const std::string& source_id,
- Line 85: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = store_.find(source_id);
- Line 537: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node_idx_map[active_nodes[i]->nodeId()] = i;
- Line 592: severity=CRITICAL; category=no_timeout
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
- Line 58: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (unsigned char c : s) {
  Confidence: band=very_high; score=0.9
- Line 94: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 119: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: bool can_acquire = !current_lease_.isValid()
- Line 121: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!can_acquire) {
- Line 275: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 1; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(deques_[victim].mtx, std::try_to_lock);
- Line 335: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < nodes_.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 338: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& t : threads) {
  Confidence: band=very_high; score=0.9
- Line 442: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& existing : nodes_) {
  Confidence: band=very_high; score=0.9
- Line 517: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "IngestionCoordinator: failed to acquire leader lease — "
- Line 536: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < active_nodes.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 640: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error(
- Line 168: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::removeNode(const std::string& node_id)
  Context: void ConsistentHashRing::removeNode(const std::string& node_id) {
  Confidence: band=medium; score=0.56
- Line 180: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::getNode(const std::string& key)
  Context: std::string ConsistentHashRing::getNode(const std::string& key) const {
  Confidence: band=medium; score=0.56
- Line 335: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back(&WorkStealingPool::workerFn, this, i, cb);
  Confidence: band=high; score=0.74
- Line 402: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes_.push_back(node);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes_.push_back(node);
- Line 447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes_.push_back(node);
- Line 461: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 462: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(info));
- Line 476: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<SourceConfig>>
  Confidence: band=medium; score=0.66
- Line 482: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<SourceConfig>> partitions;
  Confidence: band=medium; score=0.66
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partitions[node_id].push_back(src);
  Confidence: band=high; score=0.74
- Line 493: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: partitions[node_id].push_back(src);
- Line 721: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.quarantine.push_back(q);
  Confidence: band=high; score=0.74
- Line 721: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.quarantine.push_back(q);
  Confidence: band=high; score=0.74
- Line 721: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.quarantine.push_back(q);
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.quarantine.push_back(q);

### src/ingestion/entity_assembler.cpp
Total findings: 29

- Line 202: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = id_to_idx.find(ent.id);
- Line 358: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = authority_by_text.find(auth);
- Line 201: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = id_to_idx.find(ent.id);
- Line 209: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Merge properties from existing into new (keep missing keys)
- Line 32: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<char>(std::tolower(c)));
  Confidence: band=high; score=0.74
- Line 33: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<char>(std::tolower(c)));
- Line 36: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('_');
- Line 58: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string>
  Confidence: band=medium; score=0.66
- Line 67: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> parts;
  Confidence: band=medium; score=0.66
- Line 197: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> id_to_idx;
  Confidence: band=medium; score=0.66
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduped.push_back(std::move(ent));
  Confidence: band=high; score=0.74
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: deduped.push_back(std::move(ent));
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 266: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.relations.push_back(std::move(r));
- Line 296: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> id_map;
  Confidence: band=medium; score=0.66
- Line 310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.relations.push_back(std::move(r));
- Line 319: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> by_section;
  Confidence: band=medium; score=0.66
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: by_section[ref].push_back(ent.id);
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: by_section[ref].push_back(ent.id);
- Line 335: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 335: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 335: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.relations.push_back(std::move(r));
- Line 346: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> authority_by_text;
  Confidence: band=medium; score=0.66
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.relations.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.relations.push_back(std::move(r));

### src/ingestion/database_connector.cpp
Total findings: 27

- Line 172: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Context: if (!password.empty()) cs << "PWD=" << password << ";";
  Confidence: band=very_high; score=0.92
- Line 256: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator b may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto b = token.find_first_not_of(" \t");
- Line 257: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator e may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto e = token.find_last_not_of(" \t");
- Line 351: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: if (ok) SQLDisconnect(hdbc);
- Line 542: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: SQLDisconnect(hdbc);
- Line 126: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { result.port = 0; }
- Line 214: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  { out += "\\\""; }
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"')  { out += "\\\""; }
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')  { out += "\\\""; }
- Line 215: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')  { out += "\\\""; }
- Line 216: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') { out += "\\\\"; }
- Line 216: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') { out += "\\\\"; }
- Line 217: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') { out += "\\n"; }
- Line 217: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') { out += "\\n"; }
- Line 218: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\r') { out += "\\r"; }
- Line 218: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\r') { out += "\\r"; }
- Line 219: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\t') { out += "\\t"; }
- Line 219: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\t') { out += "\\t"; }
- Line 242: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!text.empty()) text += ' ';
- Line 258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token.substr(b, e - b + 1));
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(token.substr(b, e - b + 1));
- Line 297: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { batch_size_ = 500; }
- Line 301: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_rows_ = 0; }
- Line 304: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { timeout_s_ = 30; }
- Line 568: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: "SQLAllocHandle(STMT) failed",
  Confidence: band=high; score=0.74
- Line 605: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_names.push_back(std::string(reinterpret_cast<char*>(name),
  Confidence: band=high; score=0.74
- Line 606: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col_names.push_back(std::string(reinterpret_cast<char*>(name),

### src/ingestion/cdc_connector.cpp
Total findings: 23

- Line 132: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator b may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto b = token.find_first_not_of(" \t");
- Line 133: severity=CRITICAL; category=iterator_invalidation
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
- Line 179: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: /// PostgreSQL itself outputs LSN values in uppercase hexadecimal (e.g. `0/16E0478`);
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "%X/%X",
  Confidence: band=very_high; score=0.9
- Line 61: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"')       { out += "\\\""; }
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') { out += "\\\\"; }
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') { out += "\\n"; }
- Line 64: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\r') { out += "\\r"; }
- Line 65: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\t') { out += "\\t"; }
- Line 73: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& m) {
  Confidence: band=medium; score=0.66
- Line 119: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!text.empty()) text += ' ';
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token.substr(b, e - b + 1));
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(token.substr(b, e - b + 1));
- Line 175: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { return 0; }
- Line 262: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: val += '\'';
- Line 424: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { batch_size_ = 500; }
- Line 428: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_events_ = 0; }
- Line 431: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { poll_timeout_ms_ = 1000; }
- Line 434: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_empty_polls_ = 3; }

### src/ingestion/web_crawler_connector.cpp
Total findings: 23

- Line 213: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator href_pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto href_pos = tag_lc.find("href=");
- Line 220: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator val_end may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto val_end = tag.find(quote, val_start);
- Line 247: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator start may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto start = xml.find(open, pos);
- Line 250: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator end may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto end = xml.find(close, val_start);
- Line 286: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator hash may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto hash = line.find('#');
- Line 297: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator colon may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto colon = line.find(':');
- Line 185: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto a_pos = html.find('<', pos);
- Line 353: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: buf->append(ptr, size * nmemb);
- Line 459: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& loc : extractSitemapLocs(sbody)) {
- Line 465: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& loc : extractSitemapLocs(sbody)) {
- Line 536: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& href : extractHrefs(body)) {
- Line 144: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!text.empty() && text.back() != ' ') text += ' ';
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!text.empty() && text.back() != ' ') text += ' ';
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!text.empty() && text.back() != ' ') text += ' ';
- Line 168: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!prev_space) { result += ' '; prev_space = true; }
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!prev_space) { result += ' '; prev_space = true; }
- Line 222: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hrefs.push_back(tag.substr(val_start, val_end - val_start));
- Line 230: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hrefs.push_back(tag.substr(val_start, val_end - val_start));
- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!loc.empty()) locs.push_back(loc);
- Line 316: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (in_matching_agent && !val.empty()) disallowed_specific.push_back(val);
- Line 317: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (in_wildcard_agent && !val.empty()) disallowed_wildcard.push_back(val);
- Line 382: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_depth_ = 3; }
- Line 384: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_pages_ = 0; }

### src/ingestion/filesystem_ingester.cpp
Total findings: 22

- Line 339: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ocr_config_.enabled = (it->second == "true");
- Line 344: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ocr_config_.language = it->second;
- Line 87: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& component : fs::path(path)) {
- Line 562: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 734: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->setMetadataExtraction(enabled);
- Line 115: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 192: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ' ';
  Confidence: band=high; score=0.74
- Line 193: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 226: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: int rc = _pclose(pipe);
- Line 228: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: int rc = pclose(pipe);
- Line 240: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') escaped += "\\\"";
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"') escaped += "\\\"";
- Line 251: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '\'') escaped += "'\"'\"'";
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '\'') escaped += "'\"'\"'";
- Line 402: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 434: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 443: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files_to_process.push_back(path_);
- Line 450: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_to_process.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files_to_process.push_back(entry.path());
- Line 458: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_to_process.push_back(entry.path());
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files_to_process.push_back(entry.path());
- Line 672: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/ingestion/steps/ner_step.cpp
Total findings: 22

- Line 67: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = std::sregex_iterator(text.begin(), text.end(), re_law);
- Line 79: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = std::sregex_iterator(text.begin(), text.end(), re_date);
- Line 91: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = std::sregex_iterator(text.begin(), text.end(), re_az);
- Line 151: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *  - `entity_types` array   subset of [ORG, PER, LAW, DATE, LOCATION]
- Line 179: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({it->str(), "LAW",
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({it->str(), "LAW",
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({it->str(), "DATE",
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({it->str(), "DATE",
- Line 89: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(\bAz\.\s*[A-Z]\s*\d+\s*/\s*\d{2,4}\b)",
- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({it->str(), "LAW",
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({it->str(), "LAW",
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!m.text.empty()) out.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!m.text.empty()) out.push_back(std::move(m));
- Line 115: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 123: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: for (const auto& t : types) type_list += t + ", ";
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: requested_types.push_back(t.get<std::string>());
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: requested_types.push_back(t.get<std::string>());
- Line 197: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: texts_with_refs.emplace_back(c.text, c.section_ref);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back(
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back(

### src/ingestion/agentic_reference_validator.cpp
Total findings: 21

- Line 114: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 135: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 154: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 172: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 209: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: bool sec_known = (sit->second.count(ref.section) > 0);
- Line 41: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "Richtlinie\\s+(\\d+)/(\\d+)/EU",
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.validated.push_back(vr);
  Confidence: band=high; score=0.74
- Line 95: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: report.validated.push_back(vr);
- Line 98: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: report.warnings.push_back(
- Line 127: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!dup) refs.push_back(std::move(ref));
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 146: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: refs.push_back(std::move(ref));
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!dup) refs.push_back(std::move(ref));
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!dup) refs.push_back(std::move(ref));
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!dup) refs.push_back(std::move(ref));

### src/ingestion/api_connector.cpp
Total findings: 17

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 45: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, total);
- Line 45: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, total);
- Line 537: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
  Confidence: band=very_high; score=0.9
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.errors.push_back(err);
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.errors.push_back(err);
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!value.empty()) results.push_back(std::move(value));
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!value.empty()) results.push_back(std::move(value));
- Line 275: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { page_size_ = 100; }
- Line 279: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_pages_ = 0; }
- Line 332: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 348: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 537: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
- Line 549: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += "&client_id=" + urlEncode(oauth_config_.client_id);
  Confidence: band=high; score=0.74
- Line 595: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool GenericApiConnector::initialize(const SourceConfig& config) {
  Confidence: band=medium; score=0.66

### src/ingestion/ingestion_quality_judge.cpp
Total findings: 17

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 440: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator content_start may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto content_start = line.find_first_not_of("-* \t", 1);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 126: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("IngestionQualityJudge: backend must not be null");
- Line 293: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(ctx.chunks.size(), size_t{5}); ++i)
- Line 524: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& s : steps) {
  Confidence: band=very_high; score=0.9
- Line 525: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(unique_steps.begin(), unique_steps.end(), s)
- Line 389: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 439: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (line[0] == '-' || line[0] == '*' || line[0] == '\xe2' /* UTF-8 bullet */) {
- Line 442: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(line.substr(content_start));
- Line 527: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unique_steps.push_back(s);
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: unique_steps.push_back(s);
- Line 548: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obs->onQualityEvaluated(doc_id, report); } catch (...) {}
- Line 634: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.history.push_back(report);
  Confidence: band=high; score=0.74
- Line 705: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obs->onReIngestionTriggered(doc_id, attempt, reasons); } catch (...) {}
- Line 716: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { obs->onReIngestionComplete(doc_id, attempt, improved); } catch (...) {}

### src/ingestion/huggingface_connector.cpp
Total findings: 13

- Line 274: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: streaming_enabled_ = (it->second == "true");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 40: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, total);
- Line 40: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: static_cast<std::string*>(userdata)->append(ptr, total);
- Line 426: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
  Confidence: band=very_high; score=0.9
- Line 674: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // (degraded mode) to preserve backward compatibility with existing connectors.
  Confidence: band=high; score=0.8
- Line 156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.errors.push_back(err);
- Line 310: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 339: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 426: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
- Line 438: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: body += "&client_id=" + urlEncode(oauth_config_.client_id);
  Confidence: band=high; score=0.74

### src/ingestion/semantic_validator.cpp
Total findings: 12

- Line 56: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (c == ' ' || c == '/' || c == '\\') c = '_';
- Line 56: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (c == ' ' || c == '/' || c == '\\') c = '_';
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back(
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back(w);
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back(w);
- Line 249: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fragments.push_back(current_fragment);
- Line 255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fragments.push_back(current_fragment);
- Line 261: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fragments.push_back(full_text);
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_result.provisions.push_back(std::move(prov));
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: doc_result.provisions.push_back(std::move(prov));
- Line 283: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_result.warnings.push_back(w);
  Confidence: band=high; score=0.74
- Line 284: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: doc_result.warnings.push_back(w);

### src/ingestion/kafka_connector.cpp
Total findings: 11

- Line 65: severity=CRITICAL; category=missing_dtor
  Description: Class KafkaConnector allocates resources but has no destructor
  Remediation: Add explicit destructor: ~KafkaConnector() { /* cleanup */ }
  Context: class/struct KafkaConnector
- Line 220: severity=CRITICAL; category=no_timeout
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
- Line 97: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { poll_timeout_ms_ = 1000; }
- Line 100: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { max_messages_ = 0; }
- Line 103: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { session_timeout_ms_ = 10000; }
- Line 395: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rd_kafka_consumer_close(rk);
- Line 471: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rd_kafka_consumer_close(rk);

### src/ingestion/s3_connector.cpp
Total findings: 11

- Line 296: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (checkpoint_store_->read(config_.source_id, cp) &&
- Line 364: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: checkpoint_store_->write(cp);
- Line 250: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 258: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 424: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futs.push_back(std::async(std::launch::async, [&fetcher, key]() {
  Confidence: band=high; score=0.74
- Line 425: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futs.push_back(std::async(std::launch::async, [&fetcher, key]() {
- Line 507: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: safe_keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 508: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: safe_keys.push_back(k);
- Line 575: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 618: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(obj.GetKey());
  Confidence: band=high; score=0.74
- Line 619: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(obj.GetKey());

### src/ingestion/llm_adapter.cpp
Total findings: 10

- Line 16: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // See include/ingestion/inference_backend.h and llm/llm_ingestion_bridge.h.
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return [captured_backend, captured_config](const std::string& text) -> DeonticExtraction {
- Line 60: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::string response = captured_backend->generate(
  Confidence: band=very_high; score=0.9
- Line 60: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: const std::string response = captured_backend->generate(
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string("ITextGenerationBackend::generate() threw: ") + e.what());
  Confidence: band=high; score=0.74
- Line 109: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "Gesetzestext:\n" + text + "\n[/INST]";
- Line 109: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "Gesetzestext:\n" + text + "\n[/INST]";
- Line 148: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.entities.emplace_back(type_str, value_str, value_str, 0.85);
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back("LLM response did not yield a valid deontic category");

### src/ingestion/object_storage_connector.cpp
Total findings: 10

- Line 477: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto it = client.ListObjects(bucket_, gcs::MaxResults(1),
- Line 494: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: for (auto& obj_meta : client.ListObjects(bucket_,
- Line 506: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string key = obj_meta->name();
- Line 563: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto props = container_client.GetProperties();
- Line 601: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto blob_client = container_client.GetBlockBlobClient(key);
- Line 603: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto dl = blob_client.Download(dl_opts);
- Line 142: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 357: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 482: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 565: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/ingestion/deontic_extractor.cpp
Total findings: 9

- Line 113: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "|Richtlinie\\s+\\d+/\\d+/EU") },
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.deontic_categories.push_back(dp.category);
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.deontic_categories.push_back(dp.category);
- Line 187: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back(
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back("No deontic patterns matched in text");
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.obligations.emplace_back(actor, action, "", 0.75);
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.emplace_back(ep.type, raw, raw, 0.85);
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.emplace_back(ep.type, raw, raw, 0.85);
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.emplace_back(ep.type, raw, raw, 0.85);
  Confidence: band=high; score=0.74

### src/ingestion/steps/llm_extract_step.cpp
Total findings: 9

- Line 14: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "ingestion/inference_backend.h"
  Confidence: band=very_high; score=0.9
- Line 92: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 92: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 83: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool canHandle(const ExtractionContext& ctx) const override {
  Confidence: band=high; score=0.74
- Line 92: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: backend_->generate(prompt, max_tokens, temperature, lora);
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.entities.push_back(std::move(ent));
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.entities.push_back(std::move(ent));
- Line 205: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/ingestion/oauth_token_manager.cpp
Total findings: 8

- Line 51: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: buf->append(static_cast<char*>(ptr), sz * nmemb);
- Line 51: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: buf->append(static_cast<char*>(ptr), sz * nmemb);
- Line 171: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
- Line 179: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw OAuthRefreshExpiredError(
- Line 183: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 212: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
- Line 220: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw OAuthRefreshExpiredError(
- Line 224: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(

### src/ingestion/steps/chunk_tt_decompose_step.cpp
Total findings: 8

- Line 71: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 71: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: rec.metadata["section_ref"] = it_sec->second;
- Line 71: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back(
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back(
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.tensor_cores.push_back(std::move(rec));
- Line 177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back(

### src/ingestion/steps/decompress_step.cpp
Total findings: 8

- Line 132: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 132: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 63: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static bool runProcess(const std::vector<const char*>& argv_vec) {
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: argv.push_back(nullptr);
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(entry.path().string());
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(entry.path().string());
- Line 132: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "decompress: extraction tool exited with non-zero status for '" + source + "'");

### src/ingestion/steps/legal_reference_step.cpp
Total findings: 8

- Line 54: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 54: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 54: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warnings_arr.push_back(w);
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: warnings_arr.push_back(w);
  Confidence: band=high; score=0.74
- Line 87: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: warnings_arr.push_back(w);
- Line 93: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.warnings.push_back(
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.warnings.push_back(

### src/ingestion/steps/parse_text_step.cpp
Total findings: 7

- Line 29: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: *  - TXT / MD / HTML: direct file read (HTML tags stripped for MD/TXT)
- Line 40: severity=CRITICAL; category=missing_dtor
  Description: Class ParseTextStep allocates resources but has no destructor
  Remediation: Add explicit destructor: ~ParseTextStep() { /* cleanup */ }
  Context: class/struct ParseTextStep
- Line 89: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: return new ParseTextStep();
- Line 89: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new ParseTextStep();
- Line 53: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx,
- Line 92: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: p = nullptr;
  Context: delete p;
- Line 92: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete p;

### src/ingestion/steps/base_entity_assembler_step.cpp
Total findings: 6

- Line 72: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = id_to_idx.find(ent.id);
- Line 71: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = id_to_idx.find(ent.id);
- Line 71: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = id_to_idx.find(ent.id);
- Line 67: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::size_t> id_to_idx;
  Confidence: band=medium; score=0.66
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deduped.push_back(std::move(ent));
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: deduped.push_back(std::move(ent));

### src/ingestion/steps/chunk_embed_step.cpp
Total findings: 5

- Line 55: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 55: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 55: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.embeddings.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.embeddings.push_back(std::move(rec));

### src/ingestion/steps/tensor_core_bridge_step.cpp
Total findings: 5

- Line 18: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: * `sink->write(record, tenant_id)` to persist the pre-computed TT-cores.
- Line 109: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto res = sink_->write(record, effective_tenant);
- Line 72: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 72: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=very_high; score=0.9
- Line 72: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
  Confidence: band=high; score=0.74

### src/ingestion/steps/legal_metadata_step.cpp
Total findings: 4

- Line 46: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx,
- Line 58: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::string(R"([A-Z]\s?\d+/\d{2,4})"));
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.entities.push_back(std::move(ent));
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.entities.push_back(std::move(ent));

### src/ingestion/steps/chunk_text_step.cpp
Total findings: 3

- Line 45: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx,
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ctx.chunks.push_back(std::move(c));
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ctx.chunks.push_back(std::move(c));

### src/ingestion/steps/format_parse_step.cpp
Total findings: 3

- Line 89: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx,
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 74: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool canHandle(const ExtractionContext& ctx) const override {
  Confidence: band=high; score=0.74

### src/ingestion/steps/deontic_step.cpp
Total findings: 1

- Line 60: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: Result<void> execute(ExtractionContext& ctx,

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
