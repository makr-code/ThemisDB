# importers Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: importers
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 282
- Actionable Findings (Critical + High): 195
- Affected Files: 27

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 44 |
| High | 151 |
| Medium | 82 |
| Low | 5 |

## Category Summary

| Category | Count |
|---|---:|
| null_dereference | 65 |
| string_concat_loop | 32 |
| data_race | 21 |
| nested_loop_find | 16 |
| map_vs_unordered_map | 13 |
| resource_leaked_in_exception | 13 |
| uninitialized_access | 11 |
| pointer_arithmetic_unbounded | 10 |
| manual_cleanup | 9 |
| blocking_no_timeout | 8 |
| no_timeout | 8 |
| o_n_squared | 8 |
| hardcoded_path | 7 |
| repeated_search | 7 |
| generic_catch | 5 |
| uncaught_exception | 5 |
| delete_no_nullptr | 4 |
| delete_without_nullptr | 4 |
| explicit_delete | 4 |
| smart_ptr_misuse | 4 |
| fp_exact_comparison | 3 |
| iterator_invalidation | 3 |
| copy_overhead | 2 |
| expensive_inner_op | 2 |
| hardcoded_output | 2 |
| missing_vector_reserve | 2 |
| module_doc_linkset_drift | 2 |
| range_temporary | 2 |
| size_assumption | 2 |
| unnecessary_copy | 2 |
| unordered_container_iter | 2 |
| no_retry_logic | 1 |
| stale_doc_section_reference | 1 |
| unchecked_array_index | 1 |
| unstructured_log | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| importers/postgres_importer.cpp | 55 | 5 | 31 | 18 | 1 |
| importers/mysql_importer.cpp | 38 | 6 | 15 | 16 | 1 |
| importers/mongo_importer.cpp | 24 | 3 | 12 | 9 | 0 |
| importers/flatfile_importer.cpp | 22 | 7 | 10 | 5 | 0 |
| importers/s3_importer.cpp | 16 | 3 | 12 | 1 | 0 |
| importers/kafka_importer.cpp | 15 | 2 | 12 | 1 | 0 |
| importers/mdm_engine.cpp | 14 | 1 | 10 | 3 | 0 |
| importers/oracle_importer.cpp | 14 | 2 | 8 | 4 | 0 |
| importers/sqlite_importer.cpp | 14 | 2 | 9 | 3 | 0 |
| importers/huggingface_ingestion_plugin.cpp | 13 | 7 | 4 | 2 | 0 |
| importers/schema_inference.cpp | 11 | 0 | 4 | 7 | 0 |
| importers/deterministic_matcher.cpp | 9 | 3 | 3 | 3 | 0 |
| importers/adaptive_import.cpp | 6 | 0 | 3 | 3 | 0 |
| importers/canonical_resolver.cpp | 5 | 0 | 5 | 0 | 0 |
| importers/graphql_federation.cpp | 4 | 0 | 2 | 2 | 0 |
| importers/column_importance.cpp | 3 | 0 | 2 | 1 | 0 |
| importers/gui_import_wizard.cpp | 3 | 2 | 1 | 0 | 0 |
| importers/audit_trail.cpp | 2 | 0 | 1 | 1 | 0 |
| importers/conflict_resolver.cpp | 2 | 0 | 1 | 1 | 0 |
| importers/data_quality.cpp | 2 | 1 | 0 | 1 | 0 |
| importers/entity_linker.cpp | 2 | 0 | 2 | 0 | 0 |
| importers/federated_learning.cpp | 2 | 0 | 0 | 1 | 1 |
| importers/postgres_importer_mdm.cpp | 2 | 0 | 2 | 0 | 0 |
| importers/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| importers/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| importers/postgres_cdc.cpp | 1 | 0 | 1 | 0 | 0 |
| importers/schema_validator.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### importers/postgres_importer.cpp
Total findings: 55

- Line 373: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::weak_ptr<ImportHandle> weak_handle = handle;

    ProgressCallback progress_cb = [weak_handle](const std::string& stage,

                                                  size_t current, size_t total) {

        if (auto h = weak_handle.lock()) {

            h->current_records.store(current);

            h->total_records.store(total);

            h->setStage(stage);
- Line 373: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto h = weak_handle.lock()) {
- Line 2104: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (ct != custom_type_map_.end()) return ct->second;
- Line 2106: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (ct != custom_type_map_.end()) return ct->second;
- Line 2452: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return new themis::importers::PostgreSQLImporterPlugin();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4242 feat(importers): v1.8.0 Mon... (2026-03-15) | #3241 [importers] Integra
- Line 356: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->id = "import-" + std::to_string(ms) + "-" +
- Line 359: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->source_path = source_path;  // v2.0: store for schema preview
- Line 360: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 362: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(true);
- Line 363: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("pending");
- Line 367: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->future = promise->get_future().share();
- Line 400: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(false);
- Line 401: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("completed");
- Line 402: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 615: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (hdr_line.find("-- PostgreSQL database dump") != std::string::npos ||
- Line 616: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: hdr_line.find("pg_dump") != std::string::npos) {
- Line 620: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (hdr_line.find("schema only") != std::string::npos ||
- Line 621: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: hdr_line.find("schema-only") != std::string::npos ||
- Line 622: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: hdr_line.find("--schema-only") != std::string::npos) {
- Line 625: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (hdr_line.find("data only") != std::string::npos ||
- Line 626: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: hdr_line.find("data-only") != std::string::npos ||
- Line 627: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: hdr_line.find("--data-only") != std::string::npos) {
- Line 951: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (upper_def.find("FOREIGN KEY") != std::string::npos) {
- Line 1278: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *     [DEFERRABLE [INITIALLY DEFERRED|INITIALLY IMMEDIATE]]
- Line 1343: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @brief Parse a CREATE [UNIQUE] INDEX statement.
- Line 1346: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *   CREATE [UNIQUE] INDEX [CONCURRENTLY] [name] ON [schema.]table
- Line 1395: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *   ALTER TABLE [ONLY] [schema.]table
- Line 1436: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (std::find(target.columns.begin(), target.columns.end(), col)
- Line 1436: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(target.columns.begin(), target.columns.end(), col)
- Line 1997: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 2378: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto it = std::find(schema_columns.begin(), schema_columns.end(), kc);
- Line 2411: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
- Line 2456: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete plugin;
- Line 2456: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    void destroyPlugin(themis::plugins::IThemisPlugin* plugin) {

        delete plugin;

    }

}
- Line 2456: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete plugin;
- Line 1131: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) out += ",";
- Line 1132: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) out += ",";
- Line 1150: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fk.on_delete = dm[1].str();
- Line 1214: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fk.on_delete = dm[1].str();
- Line 1303: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!result.empty()) result += ",";
- Line 1504: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Postgres EXCLUDE Constraint Parsing' that was not found in 'src/importers/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/importers/FUTURE_ENHANCEMENTS.md §"Postgres EXCLUDE Constraint Parsing"
- Line 2005: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(unescapeCopyValue(raw));
- Line 2023: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case 'N':  /* \N already handled above as entire field */ out += '\\'; out += 'N'; break;
- Line 2024: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: case 'N':  /* \N already handled above as entire field */ out += '\\'; out += 'N'; break;
- Line 2024: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'N':  /* \N already handled above as entire field */ out += '\\'; out += 'N'; break;
- Line 2025: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 't':  out += '\t'; break;
- Line 2026: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'n':  out += '\n'; break;
- Line 2027: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'r':  out += '\r'; break;
- Line 2028: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': out += '\\'; break;
- Line 2029: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: default:   out += '\\'; out += next; break;
- Line 2056: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: val += '\'';
- Line 2411: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
- Line 2456: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete plugin;
- Line 2411: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);

### importers/mysql_importer.cpp
Total findings: 38

- Line 257: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::weak_ptr<ImportHandle> weak_handle = handle;

    ProgressCallback progress_cb = [weak_handle](const std::string& stage,

                                                  size_t current, size_t total) {

        if (auto h = weak_handle.lock()) {

            h->current_records.store(current);

            h->total_records.store(total);

            h->setStage(stage);
- Line 257: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto h = weak_handle.lock()) {
- Line 846: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (ci != config_type_overrides_.end()) return ci->second;
- Line 1076: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: *   tinyInt1isBit=true/false  -> jdbc_config_.tinyint1_as_boolean
- Line 1077: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: *   useSSL=true/false         -> jdbc_config_.ssl
- Line 1331: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return new themis::importers::MySQLImporterPlugin();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4288 feat(importers): MySQL/Mari... (2026-03-16) | #4242 feat(importers): v1
- Line 243: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->id = "mysql-import-" + std::to_string(ms) + "-" +
- Line 246: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 248: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(true);
- Line 249: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("pending");
- Line 252: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->future = promise->get_future().share();
- Line 283: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(false);
- Line 284: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("completed");
- Line 285: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 1256: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1260: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto it = std::find(schema_columns.begin(), schema_columns.end(), kc);
- Line 1292: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
- Line 1335: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete plugin;
- Line 1335: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    void destroyMySQLPlugin(themis::plugins::IThemisPlugin* plugin) {

        delete plugin;

    }

}
- Line 1335: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete plugin;
- Line 552: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (c == '\\') { ++k; continue; }  // MySQL backslash escape inside strings
- Line 985: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'n':  val += '\n'; break;
- Line 986: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 't':  val += '\t'; break;
- Line 987: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case 'r':  val += '\r'; break;
- Line 988: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '0':  val += '\0'; break;
- Line 989: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': val += '\\'; break;
- Line 990: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\'': val += '\''; break;
- Line 991: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  val += '"';  break;
- Line 992: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: default:   val += '\\'; val += esc; break;
- Line 997: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: val += '\'';
- Line 1015: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: val += '"'; i += 2;
- Line 1120: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: out.host = host_port.substr(0, colon_pos);

        std::string port_str = host_port.substr(colon_pos + 1);

        if (!port_str.empty()) {

            try { out.port = std::stoi(port_str); } catch (...) {

                THEMIS_WARN("MySQL/MariaDB Importer: invalid port '{}' in JDBC URL; using default {}",

                            port_str, out.port);

            }
- Line 1120: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { out.port = std::stoi(port_str); } catch (...) {
- Line 1188: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ' ';
- Line 1292: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
- Line 1335: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete plugin;
- Line 1292: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);

### importers/mongo_importer.cpp
Total findings: 24

- Line 241: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::weak_ptr<ImportHandle> weak_handle = handle;

    ProgressCallback progress_cb = [weak_handle](const std::string& stage,

                                                  size_t current, size_t total) {

        if (auto h = weak_handle.lock()) {

            h->current_records.store(current);

            h->total_records.store(total);

            h->setStage(stage);
- Line 241: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto h = weak_handle.lock()) {
- Line 843: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return new themis::importers::MongoDBImporterPlugin();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4242 feat(importers): v1.8.0 Mon... (2026-03-15) | #3241 [importers] Integra
- Line 227: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->id = "mongo-import-" + std::to_string(ms) + "-" +
- Line 230: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 232: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(true);
- Line 233: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("pending");
- Line 236: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->future = promise->get_future().share();
- Line 268: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(false);
- Line 269: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("completed");
- Line 270: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 847: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete plugin;
- Line 847: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    void destroyMongoDBPlugin(themis::plugins::IThemisPlugin* plugin) {

        delete plugin;

    }

}
- Line 847: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete plugin;
- Line 155: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: while (peek_file.get(c) && c != '\n') { /* discard */ }
- Line 162: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: peek_file.close();
- Line 288: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> field_types;
- Line 341: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                    json doc = json::parse(line);

                    processDoc(doc);

                } catch (...) {}

            }

        }

    } catch (...) {}
- Line 341: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 344: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } catch (...) {}

            }

        }

    } catch (...) {}



    json schema = json::array();

    std::string collection = configured_collection_.empty()
- Line 344: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 743: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: size_t sep = path.find_last_of("/\\");
- Line 847: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete plugin;

### importers/flatfile_importer.cpp
Total findings: 22

- Line 266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it != options.table_mappings.end()) table = it->second;
- Line 350: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::weak_ptr<ImportHandle> weak_handle = handle;

    ProgressCallback progress_cb =

        [weak_handle](const std::string& stage, size_t current, size_t total) {

            if (auto h = weak_handle.lock()) {

                h->current_records.store(current);

                h->total_records.store(total);

                h->setStage(stage);
- Line 350: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto h = weak_handle.lock()) {
- Line 721: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it != options.column_mappings.end()) col = it->second;
- Line 936: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it != options.column_mappings.end()) col = it->second;
- Line 1025: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: remapped[it != options.column_mappings.end() ? it->second : key]
- Line 1149: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it != options.column_mappings.end()) col = it->second;
- Line 334: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->id = "flatfile-import-" + std::to_string(ms) + "-" +
- Line 338: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->started_at_ms =
- Line 341: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(true);
- Line 342: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("pending");
- Line 345: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->future = promise->get_future().share();
- Line 378: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(false);
- Line 379: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("completed");
- Line 380: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->finished_at_ms =
- Line 934: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = options.column_mappings.find(col);
- Line 1147: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = options.column_mappings.find(col);
- Line 67: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: while (file.get(c) && c != '\n') { /* discard */ }
- Line 488: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (sampled == 0) first_cols = cols;

                detector.feedRow(cols, vals);

                ++sampled;

            } catch (...) {}

        }



        if (sampled > 0) {
- Line 488: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 942: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (sampled == 0) first_cols = cols;

                detector.feedRow(cols, vals);

                ++sampled;

            } catch (...) {}

        }

        file.clear();

        file.seekg(data_start);
- Line 942: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}

### importers/s3_importer.cpp
Total findings: 16

- Line 330: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ProgressCallback progress_cb =

        [weak_handle](const std::string& stage,

                      size_t current, size_t total) {

            if (auto h = weak_handle.lock()) {

                h->current_records.store(current);

                h->total_records.store(total);

                h->setStage(stage);
- Line 330: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto h = weak_handle.lock()) {
- Line 670: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return new themis::importers::S3ImporterPlugin();
- Line 313: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->id = "s3-import-" + std::to_string(ms) + "-" +
- Line 317: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->started_at_ms =
- Line 320: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(true);
- Line 321: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("pending");
- Line 324: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->future = promise->get_future().share();
- Line 358: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(false);
- Line 359: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("completed");
- Line 360: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->finished_at_ms =
- Line 613: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("S3 import error [{}]: {} ({})", location, message,
- Line 675: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete plugin;
- Line 675: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: THEMIS_PLUGIN_EXPORT void

themis_plugin_destroy_s3_importer(themis::plugins::IThemisPlugin* plugin) {

    delete plugin;

}



} // extern "C"
- Line 675: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete plugin;
- Line 675: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete plugin;

### importers/kafka_importer.cpp
Total findings: 15

- Line 275: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ProgressCallback progress_cb = [weak_handle](const std::string& stage,

                                                   size_t current,

                                                   size_t total) {

        if (auto h = weak_handle.lock()) {

            h->current_records.store(current);

            h->total_records.store(total);

            h->setStage(stage);
- Line 275: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto h = weak_handle.lock()) {
- Line 135: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 138: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 260: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->id = "kafka-import-" + std::to_string(ms) + "-" +
- Line 263: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 265: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(true);
- Line 266: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("pending");
- Line 269: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->future = promise->get_future().share();
- Line 303: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(false);
- Line 304: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("completed");
- Line 305: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->finished_at_ms =
- Line 504: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 510: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 529: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: rd_kafka_consumer_close(rk);

### importers/mdm_engine.cpp
Total findings: 14

- Line 134: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator matches may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto matches = hybrid_matcher_.findMatchingEntities(
- Line 125: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (std::find(key_fields.begin(), key_fields.end(), uf) == key_fields.end()) {
- Line 125: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(key_fields.begin(), key_fields.end(), uf) == key_fields.end()) {
- Line 182: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: link.matching_evidence   = match.confidence_evidence;

            link.created_at          = now;

            link.created_by          = config.initiated_by;

            link.metadata["collection"] = collection_name;

            link.metadata["match_method"] = match.match_method;



            if (linker_.createLink(link, options)) {
- Line 183: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: link.created_at          = now;

            link.created_by          = config.initiated_by;

            link.metadata["collection"] = collection_name;

            link.metadata["match_method"] = match.match_method;



            if (linker_.createLink(link, options)) {

                if (!options.dry_run) {
- Line 195: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (config.create_reverse_links) {

                EntityLink reverse = link;

                std::swap(reverse.source_id, reverse.target_id);

                reverse.metadata["reverse"] = true;

                linker_.createLink(reverse, options);

            }

        }
- Line 232: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Group links by source entity.

    std::map<std::string, std::vector<std::string>> groups;

    for (const auto& link : links) {

        if (link.metadata.contains("reverse") && link.metadata["reverse"] == true) continue;

        groups[link.source_id].push_back(link.target_id);

    }
- Line 240: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = existing_map.find(tid);
- Line 241: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = existing_map.find(tid);
- Line 246: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto inc_it = incoming_map.find(src_id);
- Line 289: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 218: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, const json*> incoming_map;
- Line 223: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, const json*> existing_map;
- Line 230: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::string>> groups;

### importers/oracle_importer.cpp
Total findings: 14

- Line 235: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::weak_ptr<ImportHandle> weak_handle = handle;

    ProgressCallback progress_cb = [weak_handle](const std::string& stage,

                                                  size_t current, size_t total) {

        if (auto h = weak_handle.lock()) {

            h->current_records.store(current);

            h->total_records.store(total);

            h->setStage(stage);
- Line 235: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto h = weak_handle.lock()) {
- Line 221: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->id = "oracle-import-" + std::to_string(ms) + "-" +
- Line 224: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 226: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(true);
- Line 227: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("pending");
- Line 230: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->future = promise->get_future().share();
- Line 261: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(false);
- Line 262: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("completed");
- Line 263: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 63: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: while (file.get(c) && c != '\n') { /* discard */ }
- Line 545: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: col_defs.push_back(cur);
- Line 896: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: val += '\'';
- Line 989: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ' ';

### importers/sqlite_importer.cpp
Total findings: 14

- Line 184: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::weak_ptr<ImportHandle> weak_handle = handle;

    ProgressCallback progress_cb =

        [weak_handle](const std::string& stage, size_t current, size_t total) {

            if (auto h = weak_handle.lock()) {

                h->current_records.store(current);

                h->total_records.store(total);

                h->setStage(stage);
- Line 184: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (auto h = weak_handle.lock()) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3014 [importers] Add SQLite impo... (2026-03-12)
- Line 168: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->id = "sqlite-import-" + std::to_string(ms) + "-" +
- Line 172: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->started_at_ms =
- Line 175: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(true);
- Line 176: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("pending");
- Line 179: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->future = promise->get_future().share();
- Line 212: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->running.store(false);
- Line 213: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->setStage("completed");
- Line 214: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: handle->finished_at_ms =
- Line 509: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: col_defs.push_back(cur);
- Line 875: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: val += '\'';
- Line 894: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: val += '"'; i += 2;

### importers/huggingface_ingestion_plugin.cpp
Total findings: 13

- Line 507: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::string dataset_name = job.config.value("dataset_name", plugin->config_.dataset_name);
- Line 508: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::string split = job.config.value("split", plugin->config_.split);
- Line 516: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool from_cache = plugin->loadFromCache(dataset_name, split, documents);
- Line 521: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t batch_size = plugin->config_.chunk_size;
- Line 524: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto result = plugin->fetchBatch(dataset_name, split, offset, batch_size);
- Line 539: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t max_docs_limit = plugin->config_.chunk_size * 10;  // ~10 batches
- Line 560: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: json content_spec = plugin->documentToContentSpec(documents[i], dataset_name, i);
- Line 582: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: job.progress = static_cast<float>(i + 1) / documents.size();

    }

    

    job.result_metadata["total_documents"] = documents.size();

    job.result_metadata["dataset_name"] = dataset_name;

    job.result_metadata["split"] = split;

    job.result_metadata["from_cache"] = from_cache;
- Line 583: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    job.result_metadata["total_documents"] = documents.size();

    job.result_metadata["dataset_name"] = dataset_name;

    job.result_metadata["split"] = split;

    job.result_metadata["from_cache"] = from_cache;
- Line 584: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: job.result_metadata["total_documents"] = documents.size();

    job.result_metadata["dataset_name"] = dataset_name;

    job.result_metadata["split"] = split;

    job.result_metadata["from_cache"] = from_cache;

    

    THEMIS_INFO("HuggingFace job {} completed: {} documents ingested",
- Line 585: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: job.result_metadata["total_documents"] = documents.size();

    job.result_metadata["dataset_name"] = dataset_name;

    job.result_metadata["split"] = split;

    job.result_metadata["from_cache"] = from_cache;

    

    THEMIS_INFO("HuggingFace job {} completed: {} documents ingested", 

        job.job_id, job.content_ids.size());
- Line 220: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto info = response["dataset_info"];
- Line 230: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: metadata.total_rows += split.value()["num_examples"].get<size_t>();

### importers/schema_inference.cpp
Total findings: 11

- Line 199: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = sample_index.find(key);
- Line 200: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = sample_index.find(key);
- Line 244: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = stats.find(local_key);
- Line 245: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = stats.find(local_key);
- Line 51: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> setA(a.begin(), a.end());
- Line 52: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> setB(b.begin(), b.end());
- Line 159: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<SemanticType, size_t> votes;
- Line 183: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, SchemaInferenceEngine::SemanticType>
- Line 188: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, SemanticType> result;
- Line 192: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::string>> sample_index;
- Line 231: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, ColumnStatistics>& stats)

### importers/deterministic_matcher.cpp
Total findings: 9

- Line 122: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = identifier_mapping.begin(); it != identifier_mapping.end(); ++it) {
- Line 464: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: algo = alg_it->second;
- Line 655: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: hmr.semantic_score      = (it != sem_map.end()) ? it->second : 0.0;
- Line 99: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (r.existing_entity_id == m.existing_entity_id) {
- Line 197: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min({s1.size(), s2.size(), size_t{4}}); ++i) {
- Line 418: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (norm1 == 0.0 || norm2 == 0.0) {
- Line 302: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: code += '0';
- Line 303: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: code += '0';
- Line 644: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> sem_map;

### importers/adaptive_import.cpp
Total findings: 6

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5139 [Research Review] CARDINALI... (2026-05-14)
- Line 33: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::string parent = fk.second.substr(0, fk.second.find('.'));
- Line 150: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto sit = std::find_if(schemas.begin(), schemas.end(),
- Line 26: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::set<std::string>> deps; // table → tables it depends on
- Line 27: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, size_t> in_degree;
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: order.push_back(s.name);

### importers/canonical_resolver.cpp
Total findings: 5

- Line 259: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 268: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 269: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 282: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    gr.field_provenance = json::object();', '    for (auto it = merged.begin(); it != merged.end(); ++it) {', '        gr.field_provenance[it.key()] = linked_entities[base_idx].first;', '    }', '']
- Line 351: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (best == v2) {

### importers/graphql_federation.cpp
Total findings: 4

- Line 142: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: = std::find(schema.primary_keys.begin(), schema.primary_keys.end(), col) != schema.primary_keys.end(
- Line 180: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: = std::find(schema.primary_keys.begin(), schema.primary_keys.end(), col) != schema.primary_keys.end(
- Line 110: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: key_fields += " ";
- Line 111: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: key_fields += " ";

### importers/column_importance.cpp
Total findings: 3

- Line 101: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it         = idx.find(key);
- Line 102: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it         = idx.find(key);
- Line 84: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::string>> idx;

### importers/gui_import_wizard.cpp
Total findings: 3

- Line 195: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it != config_.importer_factories.end() && it->second) {
- Line 280: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it == config_.importer_factories.end() || !it->second) {
- Line 187: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



ImportWizardState&

ImportWizard::connect(const std::string& session_id,

                       const json&        connection_params) {

    auto& s = requireSession(session_id);

    s.connection_params = connection_params;

### importers/audit_trail.cpp
Total findings: 2

- Line 64: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 68: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);

### importers/conflict_resolver.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4225 feat(importers): Import Con... (2026-03-15) | #2813 [importers] Impleme
- Line 32: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static constexpr char kSep = '\x1F'; // ASCII unit separator

### importers/data_quality.cpp
Total findings: 2

- Line 118: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = row.begin(); it != row.end(); ++it) {
- Line 134: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: validity_sum += computeValidity(sample_data, col, "string"); // default type

### importers/entity_linker.cpp
Total findings: 2

- Line 116: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!collection_name.empty()) {

            if (link.metadata.contains("collection")) {

                const std::string c

                    = link.metadata["collection"].is_string() ? link.metadata["collection"].get<std::string>() : "";

                if (!c.empty() && c != collection_name) {

                    continue;

                }
- Line 142: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Collection filter (if metadata present).

        if (!collection_name.empty() && link.metadata.contains("collection")) {

            const std::string c

                = link.metadata["collection"].is_string() ? link.metadata["collection"].get<std::string>() : "";

            if (!c.empty() && c != collection_name) {

                continue;

            }

### importers/federated_learning.cpp
Total findings: 2

- Line 122: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = updates[0].statistics.begin(); it != updates[0].statistics.end(); ++it) {
- Line 196: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: double sigma       = sensitivity * std::sqrt(2.0 * std::log(1.25 / delta)) / epsilon;

### importers/postgres_importer_mdm.cpp
Total findings: 2

- Line 91: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 102: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### importers/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### importers/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### importers/postgres_cdc.cpp
Total findings: 1

- Line 69: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));

### importers/schema_validator.cpp
Total findings: 1

- Line 133: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = schema.column_types.find(col);

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
