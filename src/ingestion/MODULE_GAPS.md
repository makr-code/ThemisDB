# ingestion Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: ingestion
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 324
- Actionable Findings (Critical + High): 144
- Affected Files: 34

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 41 |
| High | 103 |
| Medium | 173 |
| Low | 7 |

## Category Summary

| Category | Count |
|---|---:|
| string_concat_loop | 47 |
| resource_leaked_in_exception | 28 |
| range_temporary | 21 |
| uncaught_exception | 21 |
| copy_overhead | 19 |
| generic_catch | 19 |
| no_timeout | 15 |
| unordered_container_iter | 13 |
| missing_latency_metric | 12 |
| stale_doc_section_reference | 12 |
| hardcoded_path | 9 |
| iterator_invalidation | 9 |
| missing_trace_point | 8 |
| primitive_no_volatile | 7 |
| hardcoded_output | 5 |
| no_retry_logic | 5 |
| o_n_squared | 5 |
| pointer_arithmetic_unbounded | 5 |
| uninitialized_access | 5 |
| data_race | 4 |
| manual_cleanup | 4 |
| db_connection_leak | 3 |
| expensive_inner_op | 3 |
| missing_resource_limits | 3 |
| new_without_raii | 3 |
| allocation_loop | 2 |
| command_injection | 2 |
| duplicate_qualified_signature | 2 |
| exception_in_destructor | 2 |
| legacy_or_compat_path | 2 |
| missing_dtor | 2 |
| module_doc_linkset_drift | 2 |
| null_dereference | 2 |
| repeated_search | 2 |
| thread_join_no_timeout | 2 |
| unchecked_array_index | 2 |
| delete_no_nullptr | 1 |
| delete_without_nullptr | 1 |
| explicit_delete | 1 |
| explicit_lock_unlock | 1 |
| lock_contention | 1 |
| lock_in_loop | 1 |
| missing_health_check | 1 |
| missing_override_keyword | 1 |
| new_without_delete | 1 |
| path_traversal | 1 |
| regex_in_loop | 1 |
| sensitive_data_logging | 1 |
| smart_ptr_misuse | 1 |
| uninitialized_array | 1 |
| uninitialized_member_field | 1 |
| unnecessary_copy | 1 |
| unvalidated_llm_output | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| ingestion/ingestion_manager.cpp | 44 | 5 | 12 | 27 | 0 |
| ingestion/cdc_connector.cpp | 26 | 2 | 5 | 17 | 2 |
| ingestion/ingestion_coordinator.cpp | 24 | 5 | 15 | 4 | 0 |
| ingestion/legal_domain.cpp | 18 | 0 | 6 | 11 | 1 |
| ingestion/kafka_connector.cpp | 17 | 2 | 5 | 10 | 0 |
| ingestion/api_connector.cpp | 16 | 1 | 3 | 11 | 1 |
| ingestion/database_connector.cpp | 16 | 3 | 0 | 13 | 0 |
| ingestion/web_crawler_connector.cpp | 16 | 2 | 4 | 10 | 0 |
| ingestion/filesystem_ingester.cpp | 14 | 3 | 1 | 10 | 0 |
| ingestion/ingestion_quality_judge.cpp | 14 | 3 | 3 | 8 | 0 |
| ingestion/steps/ner_step.cpp | 11 | 0 | 5 | 6 | 0 |
| ingestion/huggingface_connector.cpp | 10 | 2 | 4 | 3 | 1 |
| ingestion/entity_assembler.cpp | 9 | 1 | 2 | 6 | 0 |
| ingestion/steps/parse_text_step.cpp | 9 | 5 | 3 | 1 | 0 |
| ingestion/ingestion_sinks.cpp | 8 | 3 | 4 | 1 | 0 |
| ingestion/workflow_engine.cpp | 8 | 0 | 3 | 5 | 0 |
| ingestion/llm_adapter.cpp | 7 | 0 | 2 | 5 | 0 |
| ingestion/object_storage_connector.cpp | 7 | 0 | 5 | 2 | 0 |
| ingestion/oauth_token_manager.cpp | 6 | 0 | 4 | 2 | 0 |
| ingestion/s3_connector.cpp | 6 | 1 | 0 | 5 | 0 |
| ingestion/agentic_reference_validator.cpp | 5 | 0 | 4 | 1 | 0 |
| ingestion/steps/chunk_tt_decompose_step.cpp | 5 | 0 | 4 | 1 | 0 |
| ingestion/steps/chunk_embed_step.cpp | 4 | 0 | 3 | 1 | 0 |
| ingestion/steps/decompress_step.cpp | 4 | 0 | 2 | 2 | 0 |
| ingestion/steps/llm_extract_step.cpp | 4 | 0 | 1 | 3 | 0 |
| ingestion/steps/tensor_core_bridge_step.cpp | 4 | 2 | 1 | 1 | 0 |
| ingestion/steps/base_entity_assembler_step.cpp | 3 | 1 | 1 | 1 | 0 |
| ingestion/steps/format_parse_step.cpp | 2 | 0 | 0 | 2 | 0 |
| ingestion/steps/legal_reference_step.cpp | 2 | 0 | 1 | 1 | 0 |
| ingestion/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| ingestion/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| ingestion/deontic_extractor.cpp | 1 | 0 | 0 | 1 | 0 |
| ingestion/semantic_validator.cpp | 1 | 0 | 0 | 1 | 0 |
| ingestion/steps/legal_metadata_step.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### ingestion/ingestion_manager.cpp
Total findings: 44

- Line 352: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: bool CheckpointStore::write(const IngestionCheckpoint& cp) {
- Line 368: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: bool CheckpointStore::read(const std::string& source_id,
- Line 706: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto plug = plugin_registry_.create(pit->second);
- Line 776: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (cs->read(source_id, cp)) {
- Line 1234: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: return cs->read(source_id, out);
- Line 95: severity=HIGH; category=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (char c : s) {
- Line 161: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::regex             key_re;     ///< compiled [{,]\\s*"name"\\s*: pattern
- Line 433: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 486: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 491: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 642: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: obj_connector->setRetryConfig(retry_config_);
- Line 643: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!obj_connector->initialize(config)) {
- Line 1077: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1195: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto& entry : fs::recursive_directory_iterator(root)) {
- Line 1466: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2188: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2268: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy fallback: re-run the entire source from the last checkpoint.
- Line 95: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (kMeta.find(c) != std::string::npos) out += '\\';
- Line 96: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (kMeta.find(c) != std::string::npos) out += '\\';
- Line 129: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'n':  value += '\n'; break;
- Line 130: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'r':  value += '\r'; break;
- Line 131: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 't':  value += '\t'; break;
- Line 132: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  value += '"';  break;
- Line 133: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': value += '\\'; break;
- Line 322: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += '_';
- Line 323: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += '_';
- Line 833: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("schema_validation");
- Line 837: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("deontic_extraction");
- Line 838: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("semantic_validation");
- Line 840: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("reference_validation");
- Line 845: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("mime_detection");
- Line 848: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("rate_limiting");
- Line 851: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("incremental_checkpoint");
- Line 854: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("dry_run");
- Line 1016: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(pair.second);
- Line 1376: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 1386: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool incremental_mode_ = false;
- Line 1725: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (c == '\\') out += "\\\\";
- Line 1726: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '\\') out += "\\\\";
- Line 1727: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '"')  out += "\\\"";
- Line 1728: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\n') out += "\\n";
- Line 1879: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<int, size_t> code_counts;
- Line 2216: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool success = false;
- Line 2222: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int attempt = 1; attempt <= max_attempts; ++attempt) {

### ingestion/cdc_connector.cpp
Total findings: 26

- Line 133: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator b may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto b = token.find_first_not_of(" \t");
- Line 134: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator e may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto e = token.find_last_not_of(" \t");
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3249 [ingestion] Implement CDC s... (2026-03-12) | #3197 feat(ingestion): CD
- Line 209: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 306: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 358: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 360: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 62: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '"')       { out += "\\\""; }
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\\') { out += "\\\\"; }
- Line 64: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\n') { out += "\\n"; }
- Line 65: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\r') { out += "\\r"; }
- Line 66: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\t') { out += "\\t"; }
- Line 74: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& m) {
- Line 120: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!text.empty()) text += ' ';
- Line 136: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(token.substr(b, e - b + 1));
- Line 176: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: uint32_t hi = static_cast<uint32_t>(std::stoul(s.substr(0, slash), nullptr, 16));

        uint32_t lo = static_cast<uint32_t>(std::stoul(s.substr(slash + 1), nullptr, 16));

        return (static_cast<uint64_t>(hi) << 32) | lo;

    } catch (...) { return 0; }

}



/// Format a uint64 LSN to the PostgreSQL "X/YYYYYYYY" representation.
- Line 176: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { return 0; }
- Line 263: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: val += '\'';
- Line 425: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: : splitCommaCdc(text_cols_str);



        try { batch_size_ = static_cast<size_t>(std::stoull(opt("batch_size", "500"))); }

        catch (...) { batch_size_ = 500; }

        if (batch_size_ == 0) batch_size_ = 500;



        try { max_events_ = static_cast<size_t>(std::stoull(opt("max_events", "0"))); }
- Line 425: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { batch_size_ = 500; }
- Line 429: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (batch_size_ == 0) batch_size_ = 500;



        try { max_events_ = static_cast<size_t>(std::stoull(opt("max_events", "0"))); }

        catch (...) { max_events_ = 0; }



        try { poll_timeout_ms_ = std::stoi(opt("poll_timeout_ms", "1000")); }

        catch (...) { poll_timeout_ms_ = 1000; }
- Line 429: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { max_events_ = 0; }
- Line 562: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 3: Distributed Sources & Connectors' that was not found in 'src/ingestion/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/ROADMAP.md § "Phase 3: Distributed Sources & Connectors"
- Line 565: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub/Simulation Lifecycle' that was not found in 'src/ingestion/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
- Line 180: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: /// PostgreSQL itself outputs LSN values in uppercase hexadecimal (e.g. `0/16E0478`);
- Line 184: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "%X/%X",

### ingestion/ingestion_coordinator.cpp
Total findings: 24

- Line 77: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: bool InMemorySharedCheckpointStore::write(const IngestionCheckpoint& cp) {
- Line 83: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: bool InMemorySharedCheckpointStore::read(const std::string& source_id,
- Line 340: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (t.joinable()) t.join();
- Line 428: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lease_renewal_thread_.join();
- Line 593: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (!checkpoint_store_->write(cp)) {
- Line 120: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool can_acquire = !current_lease_.isValid()
- Line 122: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!can_acquire) {
- Line 276: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t i = 1; i < n; ++i) {
- Line 278: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(deques_[victim].mtx, std::try_to_lock);
- Line 311: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        try {', '            IngestionReport report =', '                nodes_[my_idx]->ingest({src}, target_collection_, cb);', '            std::lock_guard<std::mutex> lock(results_mtx_);', '            results_.push_back(std::move(report));']
- Line 410: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 411: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 423: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 427: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 518: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "IngestionCoordinator: failed to acquire leader lease — "
- Line 676: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 685: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 686: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 689: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 693: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 169: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::removeNode(const std::string& node_id)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void ConsistentHashRing::removeNode(const std::string& node_id) {
- Line 181: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ConsistentHashRing::getNode(const std::string& key)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: std::string ConsistentHashRing::getNode(const std::string& key) const {
- Line 477: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<SourceConfig>>
- Line 483: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<SourceConfig>> partitions;

### ingestion/legal_domain.cpp
Total findings: 18

- Line 183: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = teil_begin; it != std::sregex_iterator(); ++it) {
- Line 243: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto e = std::sregex_iterator(); it != e; ++it) {
- Line 266: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto ae = std::sregex_iterator(); abs_it != ae; ++abs_it) {
- Line 558: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto ae = std::sregex_iterator(); ai != ae; ++ai) {
- Line 566: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto ne = std::sregex_iterator(); ni != ne; ++ni) {
- Line 703: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (safe.find(static_cast<char>(c)) != std::string::npos) {
- Line 250: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto body_start = headers[i].second;
- Line 519: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: R"((?:Aktenzeichen|Az\.|Geschäftszeichen|AZ)[:\s]+([A-Z0-9\-/]+(?:\s[A-Z0-9\-/]+)?))",
- Line 594: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) auflagen_str += "; ";
- Line 595: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) auflagen_str += "; ";
- Line 630: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, const BaseEntity*> id_map;
- Line 708: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
- Line 719: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '"':  out += "\\\""; break;
- Line 720: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  out += "\\\""; break;
- Line 721: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': out += "\\\\"; break;
- Line 722: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': out += "\\n";  break;
- Line 723: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': out += "\\r";  break;
- Line 708: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);

### ingestion/kafka_connector.cpp
Total findings: 17

- Line 66: severity=CRITICAL; category=missing_dtor
  Description: Class KafkaConnector allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct KafkaConnector
- Line 221: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: checkpoint_store_->write(cp);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4188 feat(ingestion): Kafka Cons... (2026-03-13) | #3287 security(ingestion)
- Line 131: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 218: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        // 32 bytes provides extra headroom for locale-specific variations.', '        constexpr std::size_t kTimestampBufSize = 32;', '        char buf[kTimestampBufSize] = {};', '        std::strftime(buf, kTimestampBufSize, "%Y-%m-%dT%H:%M:%SZ", &tm_buf);', '        cp.timestamp = buf;']
- Line 367: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 373: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 98: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: schema_reg_url_   = opt("schema_registry_url", "");



        try { poll_timeout_ms_   = std::stoi(opt("poll_timeout_ms",   "1000")); }

        catch (...) { poll_timeout_ms_ = 1000; }



        try { max_messages_ = static_cast<size_t>(std::stoull(opt("max_messages","0"))); }

        catch (...) { max_messages_ = 0; }
- Line 98: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { poll_timeout_ms_ = 1000; }
- Line 101: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: catch (...) { poll_timeout_ms_ = 1000; }



        try { max_messages_ = static_cast<size_t>(std::stoull(opt("max_messages","0"))); }

        catch (...) { max_messages_ = 0; }



        try { session_timeout_ms_ = std::stoi(opt("session_timeout_ms","10000")); }

        catch (...) { session_timeout_ms_ = 10000; }
- Line 101: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { max_messages_ = 0; }
- Line 104: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: catch (...) { max_messages_ = 0; }



        try { session_timeout_ms_ = std::stoi(opt("session_timeout_ms","10000")); }

        catch (...) { session_timeout_ms_ = 10000; }



        security_protocol_ = opt("security_protocol", "plaintext");

        sasl_mechanism_    = opt("sasl_mechanism",    "");
- Line 104: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { session_timeout_ms_ = 10000; }
- Line 232: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 3: Distributed Sources & Connectors' that was not found in 'src/ingestion/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/ROADMAP.md § "Phase 3: Distributed Sources & Connectors"
- Line 234: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub/Simulation Lifecycle' that was not found in 'src/ingestion/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
- Line 396: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: rd_kafka_consumer_close(rk);
- Line 472: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: rd_kafka_consumer_close(rk);

### ingestion/api_connector.cpp
Total findings: 16

- Line 563: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: oauth_config_.access_token = std::move(new_token);



        // Update the refresh token if the server issued a new one (RFC 6749 §6).

        std::string new_refresh = jsonExtractStringValue(resp.body, "refresh_token");

        if (!new_refresh.empty())

            oauth_config_.refresh_token = std::move(new_refresh);
- Line 558: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 563: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 565: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 276: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string ps = opt("page_size", "100");

        try { page_size_ = static_cast<size_t>(std::stoul(ps)); }

        catch (...) { page_size_ = 100; }



        std::string mp = opt("max_pages", "0");

        try { max_pages_ = static_cast<size_t>(std::stoul(mp)); }
- Line 276: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { page_size_ = 100; }
- Line 280: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string mp = opt("max_pages", "0");

        try { max_pages_ = static_cast<size_t>(std::stoul(mp)); }

        catch (...) { max_pages_ = 0; }



        // Pagination mode: "offset" (default) or "cursor"

        std::string pm = opt("pagination_mode", "offset");
- Line 280: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { max_pages_ = 0; }
- Line 333: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

            auto r = httpGet(endpoint_, buildAuthHeader(), retry_config_.timeout_ms);

            return r.status_code == 200;

        } catch (...) {

            return false;

        }

    }
- Line 333: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 349: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (total == 0) total = jsonExtractSizeT(r.body, "totalResults");

                return total;

            }

        } catch (...) {}

        return 0;

    }
- Line 349: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 538: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
- Line 550: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: body += "&client_id=" + urlEncode(oauth_config_.client_id);
- Line 596: severity=MEDIUM; category=missing_health_check
  Description: Service initialization without nearby health/status handling
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool GenericApiConnector::initialize(const SourceConfig& config) {
- Line 538: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);

### ingestion/database_connector.cpp
Total findings: 16

- Line 173: severity=CRITICAL; category=sensitive_data_logging
  Description: Potential PII/credential logging: password
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (!password.empty()) cs << "PWD=" << password << ";";
- Line 257: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator b may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto b = token.find_first_not_of(" \t");
- Line 258: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator e may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto e = token.find_last_not_of(" \t");
- Line 215: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (c == '"')  { out += "\\\""; }
- Line 216: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '"')  { out += "\\\""; }
- Line 217: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\\') { out += "\\\\"; }
- Line 218: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\n') { out += "\\n"; }
- Line 219: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\r') { out += "\\r"; }
- Line 220: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\t') { out += "\\t"; }
- Line 243: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!text.empty()) text += ' ';
- Line 260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(token.substr(b, e - b + 1));
- Line 298: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: : splitComma(text_cols_str);



        try { batch_size_ = static_cast<size_t>(std::stoull(opt("batch_size", "500"))); }

        catch (...) { batch_size_ = 500; }

        if (batch_size_ == 0) batch_size_ = 500;



        try { max_rows_ = static_cast<size_t>(std::stoull(opt("max_rows", "0"))); }
- Line 298: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { batch_size_ = 500; }
- Line 452: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 3: Distributed Sources & Connectors' that was not found in 'src/ingestion/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/ROADMAP.md § "Phase 3: Distributed Sources & Connectors"
- Line 454: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub/Simulation Lifecycle' that was not found in 'src/ingestion/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
- Line 569: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: "SQLAllocHandle(STMT) failed",

### ingestion/web_crawler_connector.cpp
Total findings: 16

- Line 221: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator val_end may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto val_end = tag.find(quote, val_start);
- Line 251: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator end may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto end = xml.find(close, val_start);
- Line 186: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto a_pos = html.find('<', pos);
- Line 460: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& loc : extractSitemapLocs(sbody)) {
- Line 466: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& loc : extractSitemapLocs(sbody)) {
- Line 537: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& href : extractHrefs(body)) {
- Line 145: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!text.empty() && text.back() != ' ') text += ' ';
- Line 146: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!text.empty() && text.back() != ' ') text += ' ';
- Line 169: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!prev_space) { result += ' '; prev_space = true; }
- Line 170: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!prev_space) { result += ' '; prev_space = true; }
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: hrefs.push_back(tag.substr(val_start, val_end - val_start));
- Line 231: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: hrefs.push_back(tag.substr(val_start, val_end - val_start));
- Line 383: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: };



        try { max_depth_ = std::stoi(opt("max_depth", "3")); }

        catch (...) { max_depth_ = 3; }

        try { max_pages_ = std::stoul(opt("max_pages", "0")); }

        catch (...) { max_pages_ = 0; }
- Line 383: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: catch (...) { max_depth_ = 3; }
- Line 488: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool   succeeded = false;
- Line 489: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int attempt = 1; attempt <= retry_config_.max_attempts; ++attempt) {

### ingestion/filesystem_ingester.cpp
Total findings: 14

- Line 218: severity=CRITICAL; category=command_injection
  Description: command_injection_popen: Command injection via popen() — use safer alternatives
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: FILE* pipe = popen(cmd.c_str(), "r");  // NOLINT(cert-env33-c)
- Line 340: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ocr_config_.enabled = (it->second == "true");
- Line 345: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ocr_config_.language = it->second;
- Line 88: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& component : fs::path(path)) {
- Line 193: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += ' ';
- Line 194: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ' ';
- Line 241: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (c == '"') escaped += "\\\"";
- Line 242: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '"') escaped += "\\\"";
- Line 252: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (c == '\'') escaped += "'\"'\"'";
- Line 253: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (c == '\'') escaped += "'\"'\"'";
- Line 435: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (fs::is_regular_file(base_dir)) {

                    base_dir = base_dir.parent_path();

                }

            } catch (...) {

                // If canonical() fails fall through; isFileWithinBase() will be

                // conservative and reject files when it cannot resolve paths.

                base_dir = fs::absolute(path_);
- Line 435: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 452: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: files_to_process.push_back(entry.path());
- Line 460: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: files_to_process.push_back(entry.path());

### ingestion/ingestion_quality_judge.cpp
Total findings: 14

- Line 130: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 441: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator content_start may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto content_start = line.find_first_not_of("-* \t", 1);
- Line 569: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 294: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(ctx.chunks.size(), size_t{5}); ++i)
- Line 376: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 526: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(unique_steps.begin(), unique_steps.end(), s)
- Line 440: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (line[0] == '-' || line[0] == '*' || line[0] == '\xe2' /* UTF-8 bullet */) {
- Line 443: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: items.push_back(line.substr(content_start));
- Line 549: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: snapshot = observers_;

    }

    for (const auto& obs : snapshot) {

        try { obs->onQualityEvaluated(doc_id, report); } catch (...) {}

    }

}
- Line 549: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { obs->onQualityEvaluated(doc_id, report); } catch (...) {}
- Line 706: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const std::vector<std::string>& reasons) noexcept

{

    for (const auto& obs : observers_) {

        try { obs->onReIngestionTriggered(doc_id, attempt, reasons); } catch (...) {}

    }

    // Forward to judge observers as well.

}
- Line 706: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { obs->onReIngestionTriggered(doc_id, attempt, reasons); } catch (...) {}
- Line 717: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: bool               improved) noexcept

{

    for (const auto& obs : observers_) {

        try { obs->onReIngestionComplete(doc_id, attempt, improved); } catch (...) {}

    }

}
- Line 717: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { obs->onReIngestionComplete(doc_id, attempt, improved); } catch (...) {}

### ingestion/steps/ner_step.cpp
Total findings: 11

- Line 65: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = std::sregex_iterator(text.begin(), text.end(), re_law);
- Line 77: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = std::sregex_iterator(text.begin(), text.end(), re_date);
- Line 89: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = std::sregex_iterator(text.begin(), text.end(), re_az);
- Line 149: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *  - `entity_types` array   subset of [ORG, PER, LAW, DATE, LOCATION]
- Line 177: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back({it->str(), "LAW",
- Line 79: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back({it->str(), "DATE",
- Line 87: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: R"(\bAz\.\s*[A-Z]\s*\d+\s*/\s*\d{2,4}\b)",
- Line 91: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back({it->str(), "LAW",
- Line 121: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& t : types) type_list += t + ", ";
- Line 177: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {

### ingestion/huggingface_connector.cpp
Total findings: 10

- Line 275: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: streaming_enabled_ = (it->second == "true");
- Line 452: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: oauth_config_.access_token = std::move(new_token);



        // Update the refresh token if the server issued a new one (RFC 6749 §6).

        std::string new_refresh = hfJsonExtractStringValue(resp.body, "refresh_token");

        if (!new_refresh.empty())

            oauth_config_.refresh_token = std::move(new_refresh);
- Line 447: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 452: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 454: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 675: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // (degraded mode) to preserve backward compatibility with existing connectors.
- Line 427: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);
- Line 439: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: body += "&client_id=" + urlEncode(oauth_config_.client_id);
- Line 676: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Data Classification Gate for' that was not found in 'src/ingestion/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/FUTURE_ENHANCEMENTS.md §"Data Classification Gate for
- Line 427: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "%%%02X", c);

### ingestion/entity_assembler.cpp
Total findings: 9

- Line 203: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = id_to_idx.find(ent.id);
- Line 202: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = id_to_idx.find(ent.id);
- Line 210: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Merge properties from existing into new (keep missing keys)
- Line 59: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string>
- Line 68: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> parts;
- Line 198: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::size_t> id_to_idx;
- Line 297: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::size_t> id_map;
- Line 320: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<std::string>> by_section;
- Line 347: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> authority_by_text;

### ingestion/steps/parse_text_step.cpp
Total findings: 9

- Line 27: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: *  - TXT / MD / HTML: direct file read (HTML tags stripped for MD/TXT)
- Line 38: severity=CRITICAL; category=missing_dtor
  Description: Class ParseTextStep allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct ParseTextStep
- Line 87: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: return new ParseTextStep();
- Line 87: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // ─────────────────────────────────────────────────────────────────────────────

extern "C" {

    IIngestionStep* themis_create_step_parse_text() {

        return new ParseTextStep();

    }

    void themis_destroy_step_parse_text(IIngestionStep* p) {

        delete p;
- Line 87: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return new ParseTextStep();
- Line 90: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete p;
- Line 90: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return new ParseTextStep();

    }

    void themis_destroy_step_parse_text(IIngestionStep* p) {

        delete p;

    }

}
- Line 90: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete p;
- Line 90: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete p;

### ingestion/ingestion_sinks.cpp
Total findings: 8

- Line 81: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: Result<void> IGraphWriter::write(const BaseEntitySet& entity_set) {
- Line 257: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: auto r = graph->write(entity_set);
- Line 523: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: Result<void> InMemoryTensorCoreBridge::write(const TensorCoreRecord& record,
- Line 93: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = nodes_.find(n.id);
- Line 98: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Merge properties: new values overwrite existing
- Line 98: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 120: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto it = std::find_if(edges_.begin(), edges_.end(),
- Line 535: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "illegal characters ('/' or '\\0')");

### ingestion/workflow_engine.cpp
Total findings: 8

- Line 72: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return ::dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
- Line 669: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(directory_path, ec)) {
- Line 708: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<BaseEntitySet> WorkflowEngine::execute(ExtractionContext& ctx) {
- Line 81: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::dlclose(handle);
- Line 148: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, Entry> steps_;
- Line 411: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (val.IsScalar()) {

            try { obj[key] = val.as<bool>(); continue; } catch (...) {}

            try { obj[key] = val.as<int64_t>(); continue; } catch (...) {}

            try { obj[key] = val.as<double>(); continue; } catch (...) {}

            obj[key] = val.as<std::string>("");

        } else if (val.IsSequence()) {

            auto arr = nlohmann::json::array();
- Line 411: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { obj[key] = val.as<double>(); continue; } catch (...) {}
- Line 708: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<BaseEntitySet> WorkflowEngine::execute(ExtractionContext& ctx) {

### ingestion/llm_adapter.cpp
Total findings: 7

- Line 59: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return [captured_backend, captured_config](const std::string& text) -> DeonticExtraction {
- Line 61: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const std::string response = captured_backend->generate(
- Line 61: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const std::string response = captured_backend->generate(
- Line 70: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string("ITextGenerationBackend::generate() threw: ") + e.what());
- Line 110: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "Gesetzestext:\n" + text + "\n[/INST]";
- Line 149: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (j.contains("confidence") && j["confidence"].is_number()) {

        try {

            result.overall_confidence = j["confidence"].get<double>();

        } catch (...) {

            result.overall_confidence = 0.0;

        }

    }
- Line 149: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### ingestion/object_storage_connector.cpp
Total findings: 7

- Line 478: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

            namespace gcs = google::cloud::storage;

            auto client = gcs::Client();

            auto it = client.ListObjects(bucket_, gcs::MaxResults(1),

                                         gcs::Prefix(prefix_));

            // If the iterator doesn't throw, the bucket is reachable.

            (void)it.begin();
- Line 495: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto client = gcs::Client();



            size_t processed = 0;

            for (auto& obj_meta : client.ListObjects(bucket_,

                                                     gcs::Prefix(prefix_))) {

                if (max_keys_ > 0 && processed >= max_keys_) break;

                if (!obj_meta) {
- Line 564: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: using namespace Azure::Storage::Blobs;

            auto container_client = BlobContainerClient::CreateFromConnectionString(

                connection_str_, container_);

            auto props = container_client.GetProperties();

            return props.Value.ETag.HasValue();

        } catch (...) {

            return false;
- Line 602: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: continue;

                    }



                    auto blob_client = container_client.GetBlockBlobClient(key);

                    Azure::Storage::Blobs::DownloadBlobOptions dl_opts;

                    auto dl = blob_client.Download(dl_opts);
- Line 604: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob_client = container_client.GetBlockBlobClient(key);

                    Azure::Storage::Blobs::DownloadBlobOptions dl_opts;

                    auto dl = blob_client.Download(dl_opts);



                    std::string body;

                    auto& stream = *dl.Value.BodyStream;
- Line 268: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 3: Distributed Sources & Connectors' that was not found in 'src/ingestion/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/ROADMAP.md § "Phase 3: Distributed Sources & Connectors"
- Line 270: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub/Simulation Lifecycle' that was not found in 'src/ingestion/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"

### ingestion/oauth_token_manager.cpp
Total findings: 6

- Line 172: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
- Line 180: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        if (code == 401) {

            throw OAuthRefreshExpiredError(

                "OAuth refresh token expired (401 from token endpoint)");

        }

        if (code < 200 || code >= 300) {
- Line 213: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
- Line 221: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



    if (code == 401) {

        throw OAuthRefreshExpiredError(

            "OAuth refresh token expired (401 from token endpoint)");

    }

    if (code < 200 || code >= 300) {
- Line 165: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int attempt = 0;
- Line 206: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int attempt = 0;

### ingestion/s3_connector.cpp
Total findings: 6

- Line 365: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: checkpoint_store_->write(cp);
- Line 251: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else {

                max_keys_per_list_ = static_cast<int>(raw);

            }

        } catch (...) {

            max_keys_per_list_ = 1000;

        }
- Line 251: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 318: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 3: Distributed Sources & Connectors' that was not found in 'src/ingestion/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/ROADMAP.md § "Phase 3: Distributed Sources & Connectors"
- Line 320: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub/Simulation Lifecycle' that was not found in 'src/ingestion/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"
- Line 485: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 3: Distributed Sources & Connectors' that was not found in 'src/ingestion/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/ingestion/ROADMAP.md § "Phase 3: Distributed Sources & Connectors"

### ingestion/agentic_reference_validator.cpp
Total findings: 5

- Line 115: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 136: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 155: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 173: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 42: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "Richtlinie\\s+(\\d+)/(\\d+)/EU",

### ingestion/steps/chunk_tt_decompose_step.cpp
Total findings: 5

- Line 69: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 153: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: max_rank);



            // Propagate provenance metadata from the original VectorRecord

            rec.metadata["source_file"] = ctx.manifest.original_path;

            // Copy any section/page hints from the VectorRecord

            auto it_sec = vec.metadata.find("section_ref");

            if (it_sec != vec.metadata.end()) {
- Line 157: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Copy any section/page hints from the VectorRecord

            auto it_sec = vec.metadata.find("section_ref");

            if (it_sec != vec.metadata.end()) {

                rec.metadata["section_ref"] = it_sec->second;

            }

            auto it_pg = vec.metadata.find("page");

            if (it_pg != vec.metadata.end()) {
- Line 161: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

            auto it_pg = vec.metadata.find("page");

            if (it_pg != vec.metadata.end()) {

                rec.metadata["page"] = it_pg->second;

            }



            if (rec.serialized_train.empty()) {
- Line 69: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {

### ingestion/steps/chunk_embed_step.cpp
Total findings: 4

- Line 56: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 110: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rec.embedding      = std::move(vec);



            if (!chunk.section_ref.empty()) {

                rec.metadata["section_ref"] = chunk.section_ref;

            }

            if (!chunk.page_ref.empty()) {

                rec.metadata["page"] = chunk.page_ref;
- Line 113: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rec.metadata["section_ref"] = chunk.section_ref;

            }

            if (!chunk.page_ref.empty()) {

                rec.metadata["page"] = chunk.page_ref;

            }



            ctx.embeddings.push_back(std::move(rec));
- Line 56: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {

### ingestion/steps/decompress_step.cpp
Total findings: 4

- Line 75: severity=HIGH; category=command_injection
  Description: command_injection_exec: Command execution — validate arguments and use safer alternatives
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ::execvp(argv[0], const_cast<char* const*>(argv.data()));
- Line 130: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 61: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: static bool runProcess(const std::vector<const char*>& argv_vec) {
- Line 130: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {

### ingestion/steps/llm_extract_step.cpp
Total findings: 4

- Line 90: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 81: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool canHandle(const ExtractionContext& ctx) const override {
- Line 90: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 127: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: backend_->generate(prompt, max_tokens, temperature, lora);

### ingestion/steps/tensor_core_bridge_step.cpp
Total findings: 4

- Line 16: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: * `sink->write(record, tenant_id)` to persist the pre-computed TT-cores.
- Line 107: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: auto res = sink_->write(record, effective_tenant);
- Line 70: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 70: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {

### ingestion/steps/base_entity_assembler_step.cpp
Total findings: 3

- Line 73: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = id_to_idx.find(ent.id);
- Line 72: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = id_to_idx.find(ent.id);
- Line 68: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::size_t> id_to_idx;

### ingestion/steps/format_parse_step.cpp
Total findings: 2

- Line 72: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool canHandle(const ExtractionContext& ctx) const override {
- Line 163: severity=MEDIUM; category=missing_override_keyword
  Description: C++11: override keyword improves code clarity and catches errors
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop

### ingestion/steps/legal_reference_step.cpp
Total findings: 2

- Line 52: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
- Line 52: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {

### ingestion/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### ingestion/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### ingestion/deontic_extractor.cpp
Total findings: 1

- Line 114: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "|Richtlinie\\s+\\d+/\\d+/EU") },

### ingestion/semantic_validator.cpp
Total findings: 1

- Line 57: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (c == ' ' || c == '/' || c == '\\') c = '_';

### ingestion/steps/legal_metadata_step.cpp
Total findings: 1

- Line 56: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::string(R"([A-Z]\s?\d+/\d{2,4})"));

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
