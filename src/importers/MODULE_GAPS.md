# importers Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: importers
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 506
- Actionable Findings (Critical + High): 225
- Affected Files: 28

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 43 |
| High | 182 |
| Medium | 280 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 156 |
| container | 84 |
| security | 65 |
| reliability | 47 |
| performance | 39 |
| llm_ai_safety | 36 |
| concurrency | 25 |
| exception_safety | 13 |
| raii | 13 |
| platform | 11 |
| memory | 10 |
| determinism | 5 |
| audit_logging | 2 |
| input_validation | 1 |
| observability | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/importers/postgres_importer.cpp | 94 | 6 | 32 | 56 | 0 |
| src/importers/mysql_importer.cpp | 45 | 5 | 13 | 27 | 0 |
| src/importers/schema_inference.cpp | 44 | 0 | 35 | 9 | 0 |
| src/importers/flatfile_importer.cpp | 43 | 6 | 10 | 27 | 0 |
| src/importers/mongo_importer.cpp | 34 | 2 | 10 | 22 | 0 |
| src/importers/mdm_engine.cpp | 32 | 1 | 13 | 18 | 0 |
| src/importers/deterministic_matcher.cpp | 25 | 3 | 3 | 19 | 0 |
| src/importers/huggingface_ingestion_plugin.cpp | 23 | 9 | 3 | 11 | 0 |
| src/importers/oracle_importer.cpp | 20 | 1 | 8 | 11 | 0 |
| src/importers/s3_importer.cpp | 20 | 2 | 10 | 8 | 0 |
| src/importers/kafka_importer.cpp | 19 | 1 | 12 | 6 | 0 |
| src/importers/sqlite_importer.cpp | 19 | 1 | 9 | 9 | 0 |
| src/importers/data_quality.cpp | 11 | 1 | 0 | 10 | 0 |
| src/importers/adaptive_import.cpp | 9 | 0 | 3 | 6 | 0 |
| src/importers/federated_learning.cpp | 9 | 0 | 0 | 8 | 1 |
| src/importers/graphql_federation.cpp | 8 | 0 | 3 | 5 | 0 |
| src/importers/gui_import_wizard.cpp | 8 | 2 | 1 | 5 | 0 |
| src/importers/blockchain_integrity.cpp | 7 | 3 | 3 | 1 | 0 |
| src/importers/canonical_resolver.cpp | 7 | 0 | 5 | 2 | 0 |
| src/importers/column_importance.cpp | 6 | 0 | 3 | 3 | 0 |
| src/importers/polyglot_mapper.cpp | 5 | 0 | 0 | 5 | 0 |
| src/importers/schema_validator.cpp | 4 | 0 | 1 | 3 | 0 |
| src/importers/audit_trail.cpp | 3 | 0 | 1 | 2 | 0 |
| src/importers/mdm_audit_trail.cpp | 3 | 0 | 0 | 3 | 0 |
| src/importers/postgres_importer_mdm.cpp | 3 | 0 | 2 | 1 | 0 |
| src/importers/conflict_resolver.cpp | 2 | 0 | 1 | 1 | 0 |
| src/importers/entity_linker.cpp | 2 | 0 | 0 | 2 | 0 |
| src/importers/postgres_cdc.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/importers/postgres_importer.cpp
Total findings: 94

- Line 373: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto h = weak_handle.lock()) {
- Line 2104: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (ct != custom_type_map_.end()) return ct->second;
- Line 2104: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (ct != custom_type_map_.end()) return ct->second;
- Line 2106: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (ct != custom_type_map_.end()) return ct->second;
- Line 2106: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (ct != custom_type_map_.end()) return ct->second;
- Line 2452: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::importers::PostgreSQLImporterPlugin();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4242 feat(importers): v1.8.0 Mon... (2026-03-15) | #3241 [importers] Integra
- Line 356: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->id = "import-" + std::to_string(ms) + "-" +
- Line 359: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->source_path = source_path;  // v2.0: store for schema preview
- Line 360: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 362: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(true);
- Line 363: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("pending");
- Line 367: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->future = promise->get_future().share();
- Line 400: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(false);
- Line 401: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("completed");
- Line 402: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 615: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (hdr_line.find("-- PostgreSQL database dump") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 616: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: hdr_line.find("pg_dump") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 620: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (hdr_line.find("schema only") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 621: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: hdr_line.find("schema-only") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 622: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: hdr_line.find("--schema-only") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 625: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (hdr_line.find("data only") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 626: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: hdr_line.find("data-only") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 627: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: hdr_line.find("--data-only") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 951: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (upper_def.find("FOREIGN KEY") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1278: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *     [DEFERRABLE [INITIALLY DEFERRED|INITIALLY IMMEDIATE]]
- Line 1343: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * @brief Parse a CREATE [UNIQUE] INDEX statement.
- Line 1346: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   CREATE [UNIQUE] INDEX [CONCURRENTLY] [name] ON [schema.]table
- Line 1395: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   ALTER TABLE [ONLY] [schema.]table
- Line 1436: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(target.columns.begin(), target.columns.end(), col)
  Confidence: band=very_high; score=0.9
- Line 1436: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(target.columns.begin(), target.columns.end(), col)
- Line 1436: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(target.columns.begin(), target.columns.end(), col)
- Line 1436: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(target.columns.begin(), target.columns.end(), col)
- Line 2360: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: hash ^= static_cast<uint8_t>(data[i]);
- Line 2378: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find(schema_columns.begin(), schema_columns.end(), kc);
- Line 2411: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
- Line 2456: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: plugin = nullptr;
  Context: delete plugin;
- Line 260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("File does not appear to be a PostgreSQL dump");
- Line 392: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 539: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& fk : schema.foreign_keys) fk_arr.push_back(fk.toJson());
- Line 555: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& idx : schema.indexes) idx_arr.push_back(idx.toJson());
- Line 559: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ck : schema.check_constraints) ck_arr.push_back(ck.toJson());
  Confidence: band=high; score=0.74
- Line 560: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& ck : schema.check_constraints) ck_arr.push_back(ck.toJson());
- Line 564: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& g : schema.generated_columns) gen_arr.push_back(g.toJson());
  Confidence: band=high; score=0.74
- Line 565: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& g : schema.generated_columns) gen_arr.push_back(g.toJson());
- Line 569: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& ex : schema.exclude_constraints) excl_arr.push_back(ex.toJson());
  Confidence: band=high; score=0.74
- Line 570: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& ex : schema.exclude_constraints) excl_arr.push_back(ex.toJson());
- Line 572: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(table_json);
  Confidence: band=high; score=0.74
- Line 578: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& m : forward_mappings) relationships_arr.push_back(m.toJson());
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& m : inverse_mappings) relationships_arr.push_back(m.toJson());
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& m : inverse_mappings) relationships_arr.push_back(m.toJson());
- Line 581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& c : cycles) cycles_arr.push_back(c);
  Confidence: band=high; score=0.74
- Line 712: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back("Statement too large near line " +
- Line 755: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back("Failed to parse CREATE TABLE near line " +
- Line 863: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back("Could not parse COPY header near line " +
- Line 1074: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.generated_columns.push_back(gen);
  Confidence: band=high; score=0.74
- Line 1131: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) out += ",";
  Confidence: band=high; score=0.74
- Line 1132: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) out += ",";
- Line 1150: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fk.on_delete = dm[1].str();
- Line 1214: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fk.on_delete = dm[1].str();
- Line 1303: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.empty()) result += ",";
- Line 1375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!col.empty()) index.columns.push_back(col);
  Confidence: band=high; score=0.74
- Line 1428: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(err);
  Confidence: band=high; score=0.74
- Line 1428: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(err);
  Confidence: band=high; score=0.74
- Line 1430: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back(err.message);
- Line 1445: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(err);
  Confidence: band=high; score=0.74
- Line 1447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back(err.message);
- Line 1573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: excl.elements.push_back(std::move(el));
  Confidence: band=high; score=0.74
- Line 1573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: excl.elements.push_back(std::move(el));
  Confidence: band=high; score=0.74
- Line 1805: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back("Row truncated in table " + table_name +
- Line 1828: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.errors.push_back("Binary COPY unsupported in table " + table_name);
- Line 1851: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back("Row too large in table " + table_name +
- Line 1873: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back("Invalid UTF-8 in table " + table_name +
- Line 1897: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"table", table_name}, {"status", "skipped"}}, 1.0);
- Line 1981: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"table", table_name}, {"status", "imported"}}, 1.0);
- Line 1988: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"table", table_name}, {"status", "imported"}}, 1.0);
- Line 2005: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(unescapeCopyValue(raw));
- Line 2023: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case 'N':  /* \N already handled above as entire field */ out += '\\'; out += 'N'; break;
  Confidence: band=high; score=0.74
- Line 2023: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case 'N':  /* \N already handled above as entire field */ out += '\\'; out += 'N'; break;
  Confidence: band=high; score=0.74
- Line 2024: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: case 'N':  /* \N already handled above as entire field */ out += '\\'; out += 'N'; break;
- Line 2024: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'N':  /* \N already handled above as entire field */ out += '\\'; out += 'N'; break;
- Line 2025: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't':  out += '\t'; break;
- Line 2026: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n':  out += '\n'; break;
- Line 2027: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r':  out += '\r'; break;
- Line 2028: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += '\\'; break;
- Line 2029: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: default:   out += '\\'; out += next; break;
- Line 2056: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: val += '\'';
- Line 2177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fk_arr.push_back(fk.toJson());
  Confidence: band=high; score=0.74
- Line 2177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fk_arr.push_back(fk.toJson());
  Confidence: band=high; score=0.74
- Line 2178: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fk_arr.push_back(fk.toJson());
- Line 2399: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2411: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
- Line 2456: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete plugin;

### src/importers/mysql_importer.cpp
Total findings: 45

- Line 257: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto h = weak_handle.lock()) {
- Line 846: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (ci != config_type_overrides_.end()) return ci->second;
- Line 1076: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: *   tinyInt1isBit=true/false  -> jdbc_config_.tinyint1_as_boolean
- Line 1077: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: *   useSSL=true/false         -> jdbc_config_.ssl
- Line 1331: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::importers::MySQLImporterPlugin();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4288 feat(importers): MySQL/Mari... (2026-03-16) | #4242 feat(importers): v1
- Line 243: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->id = "mysql-import-" + std::to_string(ms) + "-" +
- Line 246: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 248: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(true);
- Line 249: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("pending");
- Line 252: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->future = promise->get_future().share();
- Line 283: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(false);
- Line 284: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("completed");
- Line 285: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 1260: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find(schema_columns.begin(), schema_columns.end(), kc);
- Line 1292: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
- Line 1335: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: plugin = nullptr;
  Context: delete plugin;
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Cannot open file: " + source_path);
- Line 144: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("File does not appear to be a MySQL/MariaDB mysqldump");
- Line 275: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 337: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(table_json);
  Confidence: band=high; score=0.74
- Line 416: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back("Line truncated at " + std::to_string(line_number));
- Line 447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back("Statement too large near line " +
- Line 552: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (c == '\\') { ++k; continue; }  // MySQL backslash escape inside strings
- Line 590: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_defs.push_back(cur);
  Confidence: band=high; score=0.74
- Line 793: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"table", table_name}, {"status", "skipped"}}, 1.0);
- Line 812: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"table", table_name}, {"status", "failed"}}, 1.0);
- Line 826: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"table", table_name}, {"status", "imported"}}, 1.0);
- Line 985: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'n':  val += '\n'; break;
- Line 986: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 't':  val += '\t'; break;
- Line 987: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case 'r':  val += '\r'; break;
- Line 988: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '0':  val += '\0'; break;
- Line 989: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': val += '\\'; break;
- Line 990: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': val += '\''; break;
- Line 991: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  val += '"';  break;
- Line 992: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: default:   val += '\\'; val += esc; break;
- Line 997: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: val += '\'';
- Line 1015: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: val += '"'; i += 2;
- Line 1051: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token);
  Confidence: band=high; score=0.74
- Line 1120: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { out.port = std::stoi(port_str); } catch (...) {
- Line 1188: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 1281: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1292: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "%016" PRIx64, h);
- Line 1335: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete plugin;

### src/importers/schema_inference.cpp
Total findings: 44

- Line 2: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * ThemisDB | File: schema_inference.cpp | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=very_high; score=0.9
- Line 10: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "importers/schema_inference.h"
  Confidence: band=very_high; score=0.9
- Line 24: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::SchemaInferenceEngine(Config cfg)
  Confidence: band=very_high; score=0.9
- Line 31: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool SchemaInferenceEngine::columnNameSimilar(const std::string& a,
  Confidence: band=very_high; score=0.9
- Line 46: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: double SchemaInferenceEngine::jaccardSimilarity(const std::vector<std::string>& a,
  Confidence: band=very_high; score=0.9
- Line 62: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<SchemaInferenceEngine::InferredSchema>
  Confidence: band=very_high; score=0.9
- Line 63: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::inferImplicitRelationships(
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::vector<InferenceTableSchema>& schemas,
  Confidence: band=very_high; score=0.9
- Line 67: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<InferredSchema> results;
  Confidence: band=very_high; score=0.9
- Line 72: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferredSchema inferred;
  Confidence: band=very_high; score=0.9
- Line 73: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.table_name = schema.name;
  Confidence: band=very_high; score=0.9
- Line 74: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.recommendations = json::object();
  Confidence: band=very_high; score=0.9
- Line 101: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.likely_relationships.push_back(
  Confidence: band=very_high; score=0.9
- Line 112: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.cardinality_distribution[col] =
  Confidence: band=very_high; score=0.9
- Line 120: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: for (const auto& [col, ratio] : inferred.cardinality_distribution) {
  Confidence: band=very_high; score=0.9
- Line 122: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.denormalization_candidates.push_back(col);
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.recommendations["relationship_count"] =
  Confidence: band=very_high; score=0.9
- Line 127: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.likely_relationships.size();
  Confidence: band=very_high; score=0.9
- Line 128: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.recommendations["denorm_candidates"] =
  Confidence: band=very_high; score=0.9
- Line 129: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred.denormalization_candidates.size();
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: results.push_back(std::move(inferred));
  Confidence: band=very_high; score=0.9
- Line 141: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::SemanticType
  Confidence: band=very_high; score=0.9
- Line 142: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::detectSingleColumn(
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::map<std::string, SchemaInferenceEngine::SemanticType>
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::detectSemanticTypes(
  Confidence: band=very_high; score=0.9
- Line 185: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::vector<InferenceTableSchema>& schemas,
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = sample_index.find(key);
- Line 200: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = sample_index.find(key);
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string SchemaInferenceEngine::semanticTypeToString(SemanticType t) {
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<SchemaInferenceEngine::CardinalityEstimate>
  Confidence: band=very_high; score=0.9
- Line 229: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SchemaInferenceEngine::estimateCardinalities(
  Confidence: band=very_high; score=0.9
- Line 230: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::vector<InferenceTableSchema>& schemas,
  Confidence: band=very_high; score=0.9
- Line 244: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = stats.find(local_key);
- Line 244: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = stats.find(local_key);
- Line 245: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = stats.find(local_key);
  Confidence: band=very_high; score=0.9
- Line 51: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> setA(a.begin(), a.end());
  Confidence: band=medium; score=0.66
- Line 52: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> setB(b.begin(), b.end());
  Confidence: band=medium; score=0.66
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred.likely_relationships.push_back(
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred.denormalization_candidates.push_back(col);
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<SemanticType, size_t> votes;
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, SchemaInferenceEngine::SemanticType>
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, SemanticType> result;
  Confidence: band=high; score=0.74
- Line 192: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> sample_index;
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, ColumnStatistics>& stats)
  Confidence: band=high; score=0.74

### src/importers/flatfile_importer.cpp
Total findings: 43

- Line 266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != options.table_mappings.end()) table = it->second;
- Line 350: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto h = weak_handle.lock()) {
- Line 721: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != options.column_mappings.end()) col = it->second;
- Line 936: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != options.column_mappings.end()) col = it->second;
- Line 1025: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: remapped[it != options.column_mappings.end() ? it->second : key]
- Line 1149: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != options.column_mappings.end()) col = it->second;
- Line 334: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->id = "flatfile-import-" + std::to_string(ms) + "-" +
- Line 338: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->started_at_ms =
- Line 341: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(true);
- Line 342: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("pending");
- Line 345: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->future = promise->get_future().share();
- Line 378: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(false);
- Line 379: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("completed");
- Line 380: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->finished_at_ms =
- Line 934: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = options.column_mappings.find(col);
- Line 1147: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = options.column_mappings.find(col);
- Line 67: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: while (file.get(c) && c != '\n') { /* discard */ }
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("First record is not a JSON object");
- Line 170: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("First record is not valid JSON: " + line);
- Line 370: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 419: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols_vec.push_back("col_" + std::to_string(i));
  Confidence: band=high; score=0.74
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cols_vec.push_back("col_" + std::to_string(i));
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(SchemaAutoDetector::schemaToJson(schema));
  Confidence: band=high; score=0.74
- Line 488: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 517: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected.columns.push_back(field->name());
  Confidence: band=high; score=0.74
- Line 615: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(field);
  Confidence: band=high; score=0.74
- Line 766: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back(
- Line 785: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.push_back("col_" + std::to_string(i));
  Confidence: band=high; score=0.74
- Line 786: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: columns.push_back("col_" + std::to_string(i));
- Line 818: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.warnings.push_back(ve.message);
  Confidence: band=high; score=0.74
- Line 922: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(key);
  Confidence: band=high; score=0.74
- Line 942: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 967: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back(
- Line 1003: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.errors.push_back("JSON parse error at line " +
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(key);
  Confidence: band=high; score=0.74
- Line 1034: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(key);
  Confidence: band=high; score=0.74
- Line 1048: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.warnings.push_back(ve.message);
  Confidence: band=high; score=0.74
- Line 1149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.push_back(std::move(col));
  Confidence: band=high; score=0.74
- Line 1212: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row_values.emplace_back();
  Confidence: band=high; score=0.74
- Line 1285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.warnings.push_back(ve.message);
  Confidence: band=high; score=0.74
- Line 1286: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back(ve.message);
- Line 1373: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(err);
  Confidence: band=high; score=0.74

### src/importers/mongo_importer.cpp
Total findings: 34

- Line 241: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto h = weak_handle.lock()) {
- Line 843: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::importers::MongoDBImporterPlugin();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4242 feat(importers): v1.8.0 Mon... (2026-03-15) | #3241 [importers] Integra
- Line 227: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->id = "mongo-import-" + std::to_string(ms) + "-" +
- Line 230: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 232: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(true);
- Line 233: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("pending");
- Line 236: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->future = promise->get_future().share();
- Line 268: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(false);
- Line 269: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("completed");
- Line 270: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 847: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: plugin = nullptr;
  Context: delete plugin;
- Line 89: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("File does not appear to be a MongoDB JSON export "
- Line 155: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: while (peek_file.get(c) && c != '\n') { /* discard */ }
- Line 162: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: peek_file.close();
- Line 260: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 288: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> field_types;
  Confidence: band=high; score=0.74
- Line 300: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_order.push_back(key);
  Confidence: band=high; score=0.74
- Line 341: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 344: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_arr.push_back({{"name", fname}, {"type", field_types.at(fname)}});
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: col_arr.push_back({{"name", fname}, {"type", field_types.at(fname)}});
- Line 354: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schema.push_back({
- Line 402: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back("Document skipped: line too large at index " +
- Line 522: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 531: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"collection", collection}, {"status", "imported"}}, 1.0);
- Line 547: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"collection", collection}, {"status", "failed"}}, 1.0);
- Line 561: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"collection", collection}, {"status", "imported"}}, 1.0);
- Line 638: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return std::stoi(n.get<std::string>()); } catch (...) {}
- Line 647: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return std::stod(n.get<std::string>()); } catch (...) {}
- Line 719: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(unwrapDocument(elem));
  Confidence: band=high; score=0.74
- Line 720: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(unwrapDocument(elem));
- Line 743: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: size_t sep = path.find_last_of("/\\");
- Line 847: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete plugin;

### src/importers/mdm_engine.cpp
Total findings: 32

- Line 134: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator matches may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto matches = hybrid_matcher_.findMatchingEntities(
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 125: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (std::find(key_fields.begin(), key_fields.end(), uf) == key_fields.end()) {
  Confidence: band=very_high; score=0.9
- Line 125: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(key_fields.begin(), key_fields.end(), uf) == key_fields.end()) {
- Line 125: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(key_fields.begin(), key_fields.end(), uf) == key_fields.end()) {
- Line 182: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: link.metadata["collection"] = collection_name;
- Line 183: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: link.metadata["match_method"] = match.match_method;
- Line 240: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = existing_map.find(tid);
- Line 240: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = existing_map.find(tid);
- Line 240: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = existing_map.find(tid);
- Line 240: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = existing_map.find(tid);
- Line 241: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = existing_map.find(tid);
  Confidence: band=very_high; score=0.9
- Line 246: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto inc_it = incoming_map.find(src_id);
- Line 246: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto inc_it = incoming_map.find(src_id);
- Line 39: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& l : created_links) link_arr.push_back(l.toJson());
- Line 41: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& g : golden_records) gr_arr.push_back(g.toJson());
  Confidence: band=high; score=0.74
- Line 42: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& g : golden_records) gr_arr.push_back(g.toJson());
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_fields.push_back(uf);
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_fields.push_back(uf);
  Confidence: band=high; score=0.74
- Line 177: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: : ResolutionStatus::MANUAL_REVIEW);
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: created.push_back(link);
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const json*> incoming_map;
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const json*> existing_map;
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> groups;
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups[link.source_id].push_back(link.target_id);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups[link.source_id].push_back(link.target_id);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: groups[link.source_id].push_back(link.target_id);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: groups[link.source_id].push_back(link.target_id);
- Line 242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: contributors.emplace_back(tid, *(it->second));
  Confidence: band=high; score=0.74
- Line 242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: contributors.emplace_back(tid, *(it->second));
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.review_queue.push_back(e);
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.review_queue.push_back(e);
  Confidence: band=high; score=0.74

### src/importers/deterministic_matcher.cpp
Total findings: 25

- Line 122: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = identifier_mapping.begin(); it != identifier_mapping.end(); ++it) {
- Line 464: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: algo = alg_it->second;
- Line 655: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: hmr.semantic_score      = (it != sem_map.end()) ? it->second : 0.0;
- Line 99: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (r.existing_entity_id == m.existing_entity_id) {
  Confidence: band=very_high; score=0.9
- Line 197: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min({s1.size(), s2.size(), size_t{4}}); ++i) {
- Line 418: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (norm1 == 0.0 || norm2 == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 40: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_keys.push_back(field);
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(m));
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: source_fields.push_back(it.key());
  Confidence: band=high; score=0.74
- Line 302: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: code += '0';
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: code += '0';
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(score));
  Confidence: band=high; score=0.74
- Line 581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: det_results.push_back(d);
  Confidence: band=high; score=0.74
- Line 581: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: det_results.push_back(d);
  Confidence: band=high; score=0.74
- Line 606: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 616: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 644: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> sem_map;
  Confidence: band=high; score=0.74
- Line 658: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 658: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 676: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74
- Line 676: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(hmr));
  Confidence: band=high; score=0.74

### src/importers/huggingface_ingestion_plugin.cpp
Total findings: 23

- Line 506: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string dataset_name = job.config.value("dataset_name", plugin->config_.dataset_name);
- Line 506: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string dataset_name = job.config.value("dataset_name", plugin->config_.dataset_name);
- Line 507: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string split = job.config.value("split", plugin->config_.split);
- Line 515: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool from_cache = plugin->loadFromCache(dataset_name, split, documents);
- Line 515: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool from_cache = plugin->loadFromCache(dataset_name, split, documents);
- Line 520: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: size_t batch_size = plugin->config_.chunk_size;
- Line 523: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto result = plugin->fetchBatch(dataset_name, split, offset, batch_size);
- Line 538: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: size_t max_docs_limit = plugin->config_.chunk_size * 10;  // ~10 batches
- Line 559: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: json content_spec = plugin->documentToContentSpec(documents[i], dataset_name, i);
- Line 53: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["dataset_name"] = dataset_name;
- Line 191: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: job.config["dataset_name"] = dataset_name;
- Line 506: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::string dataset_name = job.config.value("dataset_name", plugin->config_.dataset_name);
- Line 219: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto info = response["dataset_info"];
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.splits.push_back(split.key());
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: metadata.splits.push_back(split.key());
- Line 229: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: metadata.total_rows += split.value()["num_examples"].get<size_t>();
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.documents.push_back(row["row"]);
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.documents.push_back(row["row"]);
- Line 428: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs.push_back(doc);
  Confidence: band=high; score=0.74
- Line 456: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cache_data.push_back(doc);
  Confidence: band=high; score=0.74
- Line 568: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: job.content_ids.push_back(content_spec["content"]["id"]);
  Confidence: band=high; score=0.74
- Line 569: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: job.content_ids.push_back(content_spec["content"]["id"]);

### src/importers/oracle_importer.cpp
Total findings: 20

- Line 235: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto h = weak_handle.lock()) {
- Line 221: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->id = "oracle-import-" + std::to_string(ms) + "-" +
- Line 224: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 226: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(true);
- Line 227: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("pending");
- Line 230: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->future = promise->get_future().share();
- Line 261: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(false);
- Line 262: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("completed");
- Line 263: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->finished_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 63: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: while (file.get(c) && c != '\n') { /* discard */ }
- Line 130: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("File does not appear to be an Oracle expdp/exp SQL dump");
- Line 253: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(table_json);
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back("Line truncated at " + std::to_string(line_number));
- Line 545: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_defs.push_back(cur);
  Confidence: band=high; score=0.74
- Line 745: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"table", table_name}, {"status", "failed"}}, 1.0);
- Line 756: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"table", table_name}, {"status", "imported"}}, 1.0);
- Line 896: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: val += '\'';
- Line 948: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token);
  Confidence: band=high; score=0.74
- Line 989: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';

### src/importers/s3_importer.cpp
Total findings: 20

- Line 330: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto h = weak_handle.lock()) {
- Line 670: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: return new themis::importers::S3ImporterPlugin();
- Line 313: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->id = "s3-import-" + std::to_string(ms) + "-" +
- Line 317: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->started_at_ms =
- Line 320: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(true);
- Line 321: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("pending");
- Line 324: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->future = promise->get_future().share();
- Line 358: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(false);
- Line 359: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("completed");
- Line 360: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->finished_at_ms =
- Line 613: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("S3 import error [{}]: {} ({})", location, message,
- Line 675: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: plugin = nullptr;
  Context: delete plugin;
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Invalid S3 URL (expected s3://bucket/key): " +
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 526: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (const auto& e : obj_stats.errors)     stats.errors.push_back(e);
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(e);
  Confidence: band=high; score=0.74
- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(e);
  Confidence: band=high; score=0.74
- Line 574: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.structured_errors.push_back(e);
  Confidence: band=high; score=0.74
- Line 675: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete plugin;

### src/importers/kafka_importer.cpp
Total findings: 19

- Line 275: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto h = weak_handle.lock()) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 260: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->id = "kafka-import-" + std::to_string(ms) + "-" +
- Line 263: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->started_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
- Line 265: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(true);
- Line 266: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("pending");
- Line 269: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->future = promise->get_future().share();
- Line 303: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(false);
- Line 304: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("completed");
- Line 305: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->finished_at_ms =
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("Topic name is empty.");
- Line 295: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 529: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rd_kafka_consumer_close(rk);
- Line 641: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 648: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 665: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/importers/sqlite_importer.cpp
Total findings: 19

- Line 184: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (auto h = weak_handle.lock()) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3014 [importers] Add SQLite impo... (2026-03-12)
- Line 168: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->id = "sqlite-import-" + std::to_string(ms) + "-" +
- Line 172: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->started_at_ms =
- Line 175: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(true);
- Line 176: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("pending");
- Line 179: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->future = promise->get_future().share();
- Line 212: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->running.store(false);
- Line 213: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->setStage("completed");
- Line 214: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->finished_at_ms =
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back("File does not appear to be a SQLite dump");
- Line 204: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(table_json);
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.warnings.push_back("Line truncated at " +
- Line 509: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: col_defs.push_back(cur);
  Confidence: band=high; score=0.74
- Line 733: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"table", table_name}, {"status", "failed"}}, 1.0);
- Line 875: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: val += '\'';
- Line 894: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: val += '"'; i += 2;
- Line 943: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(token);
  Confidence: band=high; score=0.74

### src/importers/data_quality.cpp
Total findings: 11

- Line 118: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = row.begin(); it != row.end(); ++it) {
- Line 134: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: validity_sum += computeValidity(sample_data, col, "string"); // default type
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: validity_sum += computeValidity(sample_data, col, "string"); // default type
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: validity_sum += computeValidity(sample_data, col, "string"); // default type
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table_samples[s.table_name].push_back(json{{s.column_name, v}});
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table_samples[s.table_name].push_back(json{{s.column_name, v}});
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: table_samples[s.table_name].push_back(json{{s.column_name, v}});
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.issues.push_back("Table '" + schema.name + "' has low quality score: "
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.issues.push_back("Table '" + schema.name + "' has low quality score: "
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: report.issues.push_back("Table '" + schema.name + "' has low quality score: "
- Line 221: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: report.recommendations.push_back("Investigate null values and type mismatches in '" + schema.name +

### src/importers/adaptive_import.cpp
Total findings: 9

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #5139 [Research Review] CARDINALI... (2026-05-14)
- Line 33: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: std::string parent = fk.second.substr(0, fk.second.find('.'));
  Confidence: band=very_high; score=0.9
- Line 150: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto sit = std::find_if(schemas.begin(), schemas.end(),
- Line 26: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::set<std::string>> deps; // table → tables it depends on
  Confidence: band=high; score=0.74
- Line 27: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, size_t> in_degree;
  Confidence: band=high; score=0.74
- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: order.push_back(t);
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: order.push_back(s.name);
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: order.push_back(s.name);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order.push_back(s.name);

### src/importers/federated_learning.cpp
Total findings: 9

- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(upd.gradient[d]);
  Confidence: band=high; score=0.74
- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(upd.gradient[d]);
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(upd.gradient[d]);
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(upd.gradient[d]);
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = updates[0].statistics.begin(); it != updates[0].statistics.end(); ++it) {
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(v);
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(v);
  Confidence: band=high; score=0.74
- Line 237: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(gradient[i] + mask[i]);
  Confidence: band=high; score=0.74
- Line 196: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double sigma       = sensitivity * std::sqrt(2.0 * std::log(1.25 / delta)) / epsilon;
  Confidence: band=medium; score=0.6

### src/importers/graphql_federation.cpp
Total findings: 8

- Line 142: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: = std::find(schema.primary_keys.begin(), schema.primary_keys.end(), col) != schema.primary_keys.end(
- Line 180: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: = std::find(schema.primary_keys.begin(), schema.primary_keys.end(), col) != schema.primary_keys.end(
- Line 180: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: = std::find(schema.primary_keys.begin(), schema.primary_keys.end(), col) != schema.primary_keys.end(
- Line 94: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  @link(url: \"https://specs.apollo.dev/federation/v2.3\",\n"
- Line 94: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  @link(url: \"https://specs.apollo.dev/federation/v2.3\",\n"
- Line 110: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key_fields += " ";
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key_fields += " ";
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key_fields += " ";

### src/importers/gui_import_wizard.cpp
Total findings: 8

- Line 195: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != config_.importer_factories.end() && it->second) {
- Line 280: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it == config_.importer_factories.end() || !it->second) {
- Line 187: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: ImportWizard::connect(const std::string& session_id,
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: mappings_j.push_back({
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: mappings_j.push_back({
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: s.column_mappings.push_back(std::move(cm));
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: s.preview_schema.push_back({
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: s.preview_schema.push_back({

### src/importers/blockchain_integrity.cpp
Total findings: 7

- Line 25: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string BlockchainIntegrityVerifier::MerkleTreeBuilder::sha256Hex(const std::string &input) {
  Confidence: band=very_high; score=0.99
- Line 28: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::size_t h1 = std::hash<std::string>{}(input);
  Confidence: band=very_high; score=0.99
- Line 29: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::size_t h2 = std::hash<std::string>{}(input + "_salt");
  Confidence: band=very_high; score=0.99
- Line 25: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string BlockchainIntegrityVerifier::MerkleTreeBuilder::sha256Hex(const std::string &input) {
  Confidence: band=very_high; score=0.9
- Line 28: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::size_t h1 = std::hash<std::string>{}(input);
  Confidence: band=very_high; score=0.9
- Line 29: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::size_t h2 = std::hash<std::string>{}(input + "_salt");
  Confidence: band=very_high; score=0.9
- Line 69: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next.push_back(combineHashes(layer[i], layer[i + 1]));
  Confidence: band=high; score=0.74

### src/importers/canonical_resolver.cpp
Total findings: 7

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    gr.field_provenance = json::object();', '    for (auto it = merged.begin(); it != merged.end(); ++it) {', '        gr.field_provenance[it.key()] = linked_entities[base_idx].first;', '    }', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 351: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (best == v2) {
  Confidence: band=very_high; score=0.9
- Line 117: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gr.contributing_ids.push_back(eid);
  Confidence: band=high; score=0.74

### src/importers/column_importance.cpp
Total findings: 6

- Line 101: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it         = idx.find(key);
- Line 101: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it         = idx.find(key);
- Line 102: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it         = idx.find(key);
  Confidence: band=very_high; score=0.9
- Line 84: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> idx;
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: redundant.emplace_back(a.table_name + "." + a.column_name, b.table_name + "." + b.column_name);
  Confidence: band=high; score=0.74
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: redundant.emplace_back(a.table_name + "." + a.column_name, b.table_name + "." + b.column_name);
  Confidence: band=high; score=0.74

### src/importers/polyglot_mapper.cpp
Total findings: 5

- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: mapping.rationale.push_back(
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edges.push_back(std::move(edge));
  Confidence: band=high; score=0.74

### src/importers/schema_validator.cpp
Total findings: 4

- Line 133: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = schema.column_types.find(col);
- Line 53: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 107: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(col);
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back(std::move(err));
  Confidence: band=high; score=0.74

### src/importers/audit_trail.cpp
Total findings: 3

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 68: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: events_.push_back(event);
  Confidence: band=high; score=0.74

### src/importers/mdm_audit_trail.cpp
Total findings: 3

- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e);
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: events_arr.push_back(e.toJson());
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: events_arr.push_back(e.toJson());

### src/importers/postgres_importer_mdm.cpp
Total findings: 3

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 94: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: imported_entities.push_back(e);
  Confidence: band=high; score=0.74

### src/importers/conflict_resolver.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4225 feat(importers): Import Con... (2026-03-15) | #2813 [importers] Impleme
- Line 32: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static constexpr char kSep = '\x1F'; // ASCII unit separator

### src/importers/entity_linker.cpp
Total findings: 2

- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(json{{"id", link.source_id}});
  Confidence: band=high; score=0.74

### src/importers/postgres_cdc.cpp
Total findings: 1

- Line 69: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
