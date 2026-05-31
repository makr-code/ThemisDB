# updates Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: updates
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 355
- Actionable Findings (Critical + High): 153
- Affected Files: 21

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 23 |
| High | 130 |
| Medium | 202 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 113 |
| performance_patterns | 88 |
| reliability | 50 |
| raii | 28 |
| exception_safety | 22 |
| concurrency | 19 |
| memory | 15 |
| performance | 4 |
| platform | 4 |
| audit_logging | 3 |
| determinism | 3 |
| security | 3 |
| legacy_duplication | 1 |
| type_conversion | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/updates/schema_migration.cpp | 54 | 0 | 45 | 9 | 0 |
| src/updates/delta_update_engine.cpp | 38 | 0 | 1 | 37 | 0 |
| src/updates/dependency_resolver.cpp | 28 | 2 | 3 | 23 | 0 |
| src/updates/in_place_schema_migrator.cpp | 25 | 1 | 6 | 18 | 0 |
| src/updates/updates_config.cpp | 25 | 0 | 1 | 24 | 0 |
| src/updates/hot_reload_engine.cpp | 22 | 2 | 8 | 12 | 0 |
| src/updates/canary_rollout.cpp | 20 | 0 | 13 | 7 | 0 |
| src/updates/manifest_database.cpp | 20 | 4 | 5 | 11 | 0 |
| src/updates/parallel_downloader.cpp | 18 | 4 | 9 | 5 | 0 |
| src/updates/update_state_machine.cpp | 17 | 0 | 7 | 10 | 0 |
| src/updates/coordinated_update_manager.cpp | 13 | 2 | 8 | 3 | 0 |
| src/updates/hardware_telemetry.cpp | 12 | 5 | 7 | 0 | 0 |
| src/updates/tenant_update_scheduler.cpp | 12 | 3 | 3 | 6 | 0 |
| src/updates/cluster_update_manager.cpp | 11 | 0 | 6 | 5 | 0 |
| src/updates/preflight_health_check.cpp | 10 | 0 | 2 | 8 | 0 |
| src/updates/release_manifest.cpp | 10 | 0 | 2 | 8 | 0 |
| src/updates/build_verifier.cpp | 6 | 0 | 1 | 5 | 0 |
| src/updates/notification_webhook.cpp | 5 | 0 | 1 | 4 | 0 |
| src/updates/update_history_logger.cpp | 4 | 0 | 0 | 4 | 0 |
| src/updates/blue_green_deployment.cpp | 3 | 0 | 2 | 1 | 0 |
| src/updates/schema_migration_tester.cpp | 2 | 0 | 0 | 2 | 0 |

## Full Scanner Findings

### src/updates/schema_migration.cpp
Total findings: 54

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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 210: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: starting online DDL ({} operations)",
- Line 218: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase SHADOW_CREATE", version_);
- Line 222: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase DUAL_WRITE", version_);
- Line 226: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase BACKFILL", version_);
- Line 236: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: {}", version_, result.error_message);
- Line 241: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_WARN("SchemaMigration [{}]: rollback also failed: {}",
- Line 251: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase CONSISTENCY_CHECK", version_);
- Line 256: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase ATOMIC_SWAP (shadow → main)", version_);
- Line 261: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase CLEANUP", version_);
- Line 267: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: upgrading schema version marker: {} → {}",
- Line 271: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_WARN("SchemaMigration [{}]: could not persist schema version marker",
- Line 285: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: migration completed successfully "
- Line 308: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: rolling back from phase {}",
- Line 316: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: rollback failed to restore key '{}'",
- Line 322: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: rollback failed to remove key '{}'",
- Line 346: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: addColumn '{}' has empty column name",
- Line 351: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: addColumn has empty table name", version_);
- Line 369: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write column metadata for '{}.{}'",
- Line 385: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: renameColumn '{}' has empty column name",
- Line 390: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: renameColumn has empty table name", version_);
- Line 423: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write renamed column metadata '{}.{}'",
- Line 429: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to remove old column metadata '{}.{}'",
- Line 436: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write rename marker for '{}.{}'",
- Line 441: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: renameColumn '{}.{}' → '{}'",
- Line 449: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: addIndex '{}' has empty index name",
- Line 454: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: addIndex '{}' on table '{}' has no columns",
- Line 459: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: addIndex has empty table name", version_);
- Line 483: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write index metadata '{}.{}'",
- Line 502: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: dropColumn '{}' has empty column name",
- Line 507: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: dropColumn has empty table name", version_);
- Line 534: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write drop marker for '{}.{}'",
- Line 541: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to remove column metadata for '{}.{}'",
- Line 559: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_WARN("SchemaMigration [{}]: listKeys() unsupported or failed; "
- Line 566: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: custom migration callback returned false",
- Line 569: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: custom migration callback succeeded",
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 374: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backfilled_columns_.push_back(op.column.name);
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: undo_log_.push_back({op.table, new_meta_key, existing_new_meta, had_new_meta});
- Line 418: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: undo_log_.push_back({op.table, old_meta_key, meta_value, had_old});
- Line 419: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: undo_log_.push_back({op.table, rename_key, existing_rename, had_rename});
- Line 468: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) cols += ",";
  Confidence: band=high; score=0.74
- Line 469: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) cols += ",";
- Line 479: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: undo_log_.push_back({op.table, idx_key, existing, had_value});
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: undo_log_.push_back({op.table, idx_key, existing, had_value});

### src/updates/delta_update_engine.cpp
Total findings: 38

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 63: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (rel_path[0] == '/' || rel_path[0] == '\\' || rel_path.find('\0') != std::string::npos) return fa
- Line 63: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (rel_path[0] == '/' || rel_path[0] == '\\' || rel_path.find('\0') != std::string::npos) return fa
- Line 63: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (rel_path[0] == '/' || rel_path[0] == '\\' || rel_path.find('\0') != std::string::npos) return fa
- Line 84: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 125: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 129: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 136: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 139: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 230: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["deltas"].push_back(d.toJson());
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["deltas"].push_back(d.toJson());
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["deltas"].push_back(d.toJson());
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["deltas"].push_back(d.toJson());
- Line 270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (fd) dm.deltas.push_back(*fd);
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (fd) dm.deltas.push_back(*fd);
- Line 275: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: registered_deltas_.push_back(manifest);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: registered_deltas_.push_back(manifest);
- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.files_fallback.push_back(fd.path);
  Confidence: band=high; score=0.74
- Line 345: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 359: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 369: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 379: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 391: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 401: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 412: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_patched.push_back(fd.path);
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 450: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: pf.close();
- Line 657: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(val        & 0xFF));
- Line 658: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((val >>  8) & 0xFF));
- Line 659: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
- Line 660: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ht[h].push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ht[h].push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 684: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ht[h].push_back(static_cast<uint32_t>(i));
- Line 722: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: instructions.push_back(INSTR_COPY);
  Confidence: band=high; score=0.74
- Line 723: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: instructions.push_back(INSTR_COPY);

### src/updates/dependency_resolver.cpp
Total findings: 28

- Line 257: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator cur_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto cur_it = current_versions.find(dep.package);
- Line 431: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = current_versions.find(pkg);
- Line 180: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "%d.%d.%d", maj, min, pat + 1);
  Confidence: band=very_high; score=0.9
- Line 362: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it_pkg = deps_.find(pkg);
- Line 365: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it_ver = it_pkg->second.find(tgt_ver);
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(themis::utils::trim(cur));
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(themis::utils::trim(cur));
- Line 80: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(themis::utils::trim(cur));
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back({
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back({
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.conflicts.push_back({
- Line 370: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> added_edges;
  Confidence: band=medium; score=0.66
- Line 378: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successors[dep.package].push_back(pkg);
  Confidence: band=high; score=0.74
- Line 378: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successors[dep.package].push_back(pkg);
  Confidence: band=high; score=0.74
- Line 379: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: successors[dep.package].push_back(pkg);
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (kv.second == 0) ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (kv.second == 0) ready.push_back(kv.first);
- Line 396: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sorted_pkgs.push_back(cur);
- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (kv.second > 0) cycle_nodes.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 411: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (kv.second > 0) cycle_nodes.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 412: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (kv.second > 0) cycle_nodes.push_back(kv.first);
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(std::move(step));
  Confidence: band=high; score=0.74
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(std::move(step));
  Confidence: band=high; score=0.74
- Line 434: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.steps.push_back(std::move(step));
- Line 451: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> installed_map;
  Confidence: band=medium; score=0.66
- Line 464: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back({p1, p2, reason});
  Confidence: band=high; score=0.74
- Line 464: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back({p1, p2, reason});
  Confidence: band=high; score=0.74
- Line 465: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conflicts.push_back({p1, p2, reason});

### src/updates/in_place_schema_migrator.cpp
Total findings: 25

- Line 133: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = to_map.find(name);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 89: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (from_names.find(p.name) == from_names.end()) {
- Line 89: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (from_names.find(p.name) == from_names.end()) {
- Line 47: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> from_props;
  Confidence: band=high; score=0.74
- Line 53: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> to_props;
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, bool> from_names;
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added.push_back(p.name);
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added.push_back(p.name);
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: added.push_back(p.name);
- Line 108: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> from_map;
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> to_map;
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.added_columns.push_back(p);
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.removed_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.removed_columns.push_back(p);
- Line 142: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.modified_columns.push_back(std::move(mod));
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.modified_columns.push_back(std::move(mod));
- Line 229: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) cols_str += ", ";
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) cols_str += ", ";

### src/updates/updates_config.cpp
Total findings: 25

- Line 345: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["canary"]["stages"] = json::array();
- Line 41: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: runtime.stages.push_back(stage);
  Confidence: band=high; score=0.74
- Line 42: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: runtime.stages.push_back(stage);
- Line 54: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto checker = config["updates"]["checker"];
  Confidence: band=high; score=0.74
- Line 73: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auto_update = config["updates"]["auto_update"];
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.auto_update.schedule_days.push_back(day.as<std::string>());
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.auto_update.schedule_days.push_back(day.as<std::string>());
- Line 93: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hot_reload = config["updates"]["hot_reload"];
  Confidence: band=high; score=0.74
- Line 108: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto notifications = config["updates"]["notifications"];
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.notifications.on_events.push_back(event.as<std::string>());
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.notifications.on_events.push_back(event.as<std::string>());
- Line 128: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto canary_yaml = config["updates"]["canary"];
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.canary.stages.push_back(stage);
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.canary.stages.push_back(stage);
- Line 146: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto tel = config["updates"]["telemetry"];
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto checker = j["checker"];
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auto_update = j["auto_update"];
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hot_reload = j["hot_reload"];
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto notifications = j["notifications"];
  Confidence: band=high; score=0.74
- Line 243: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto canary_json = j["canary"];
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.canary.stages.push_back(stage);
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.canary.stages.push_back(stage);
- Line 261: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto tel = j["telemetry"];
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["canary"]["stages"].push_back(stage_json);
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["canary"]["stages"].push_back(stage_json);

### src/updates/hot_reload_engine.cpp
Total findings: 22

- Line 99: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cached_path = manifest_db_->getCachedDownload(version, file.path);
- Line 473: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* fp = fopen(dest.c_str(), "wb");
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3432 [WIP] Update HSM-based bundle signing with hardware-backed keys (2026-03-12T07:14:25Z)
- Line 186: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check compatibility
  Confidence: band=high; score=0.8
- Line 304: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& file_json : metadata_json["files"]) {
- Line 399: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(config_.backup_directory)) {
- Line 547: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["rollback_id"] = rollback_id;
- Line 548: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
- Line 549: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["files"] = json::array();
- Line 551: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["files"].push_back({{"path", file.path}});
- Line 348: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("File not downloaded: " + file.path);
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back("File not downloaded: " + file.path);
- Line 409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rollback_points.emplace_back(rollback_id, timestamp);
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (newest first)
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(fp);
- Line 550: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata["files"].push_back({{"path", file.path}});
  Confidence: band=high; score=0.74
- Line 550: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata["files"].push_back({{"path", file.path}});
  Confidence: band=high; score=0.74
- Line 551: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: metadata["files"].push_back({{"path", file.path}});
- Line 595: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 604: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 612: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 615: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);

### src/updates/canary_rollout.cpp
Total findings: 20

- Line 56: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CanaryRollout: engine must not be null");
- Line 59: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CanaryRollout: stages must not be empty");
- Line 62: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CanaryRollout: node_id must not be empty");
- Line 65: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CanaryRollout: version must not be empty");
- Line 79: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // same inputs – no inter-node communication required.
  Confidence: band=very_high; score=0.9
- Line 413: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < stages.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 452: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CanaryDeployment: version must be set before deploy()");
- Line 455: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CanaryDeployment: node_id must be set before deploy()");
- Line 458: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CanaryDeployment: engine must be set before deploy()");
- Line 461: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("CanaryDeployment: stages must be set before deploy()");
- Line 470: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& s : stages_) {
  Confidence: band=very_high; score=0.9
- Line 581: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 586: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 316: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: CanaryStatus s;
- Line 473: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.stages.push_back(cs);
  Confidence: band=high; score=0.74
- Line 474: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cfg.stages.push_back(cs);
- Line 496: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(stage_info); } catch (...) {}
- Line 509: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(reason); } catch (...) {}
- Line 574: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: latency_samples_us_.push_back(latency.count());
- Line 699: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: CanaryStatus s;

### src/updates/manifest_database.cpp
Total findings: 20

- Line 48: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cf_files = storage_->getOrCreateColumnFamily("file_registry");
- Line 49: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cf_signatures = storage_->getOrCreateColumnFamily("signature_cache");
- Line 50: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cf_cache = storage_->getOrCreateColumnFamily("download_cache");
- Line 304: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sig.signingCertificate = manifest->signing_certificate;
- Line 153: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: it = nullptr;
  Context: delete it;
- Line 460: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: manifest = nullptr;
  Context: LOG_ERROR("Cannot delete manifest {}: not found", version);
- Line 490: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: manifest = nullptr;
  Context: LOG_ERROR("Failed to delete manifest {}: {}", version, status.ToString());
- Line 506: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: file = nullptr;
  Context: LOG_WARN("Failed to delete file registry entry for {}: {}",
- Line 517: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cached = nullptr;
  Context: LOG_WARN("Failed to delete cached file {}: {}",
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(it->key().ToString());
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: versions.push_back(it->key().ToString());
- Line 153: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete it;
- Line 223: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: temp.close();
- Line 251: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 405: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 449: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 460: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: LOG_ERROR("Cannot delete manifest {}: not found", version);
- Line 490: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: LOG_ERROR("Failed to delete manifest {}: {}", version, status.ToString());
- Line 506: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: LOG_WARN("Failed to delete file registry entry for {}: {}",
- Line 517: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: LOG_WARN("Failed to delete cached file {}: {}",

### src/updates/parallel_downloader.cpp
Total findings: 18

- Line 257: severity=CRITICAL; category=missing_dtor
  Description: Class WriteCtx allocates resources but has no destructor
  Remediation: Add explicit destructor: ~WriteCtx() { /* cleanup */ }
  Context: class/struct WriteCtx
- Line 270: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* fp = fopen(dest.c_str(), open_mode);
- Line 384: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!backoff.wait()) break;
- Line 507: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: cv.wait(lock, [&]() { return !pq.empty() || all_queued; });
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 70: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ParallelDownloader::setConcurrency: n must be >= 1");
- Line 262: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto write_cb = [](char* ptr, size_t sz, size_t nmemb, void* ud) -> size_t {
- Line 262: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto write_cb = [](char* ptr, size_t sz, size_t nmemb, void* ud) -> size_t {
- Line 264: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: size_t n = fwrite(ptr, sz, nmemb, ctx->fp);
- Line 264: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: size_t n = fwrite(ptr, sz, nmemb, ctx->fp);
- Line 490: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 506: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(queue_mutex);
- Line 202: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 218: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 278: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(fp);
- Line 303: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(fp);
- Line 527: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back(worker);
  Confidence: band=high; score=0.74

### src/updates/update_state_machine.cpp
Total findings: 17

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 116: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return state_.load(std::memory_order_acquire);
- Line 206: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& cb : callbacks_copy) {
  Confidence: band=very_high; score=0.9
- Line 245: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& cb : callbacks_copy) {
  Confidence: band=very_high; score=0.9
- Line 255: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 260: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 99: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 187: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: transaction_log_.push_back(entry);
- Line 209: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: transaction_log_.push_back(entry);
- Line 248: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: callbacks_.push_back(std::move(cb));
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: callbacks_.push_back(std::move(cb));
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: transaction_log_.push_back(*entry);
- Line 306: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 450: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}

### src/updates/coordinated_update_manager.cpp
Total findings: 13

- Line 224: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: predecessor_ready = wait_fn(pred->node_id, config_.wait_timeout);
- Line 245: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ReloadResult reload = engine_->applyHotReload(config_.version);
- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 45: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 95: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 138: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& n : sorted_nodes_) {
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!local || local->sequence_number == 0) return nullptr;
- Line 148: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& n : sorted_nodes_) {
  Confidence: band=very_high; score=0.9
- Line 375: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 103: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: NodeUpdateStatus s;
- Line 107: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_statuses_.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 108: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node_statuses_.push_back(std::move(s));

### src/updates/hardware_telemetry.cpp
Total findings: 12

- Line 446: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (config_.include_cpu_model) { snap.cpu_model    = hw_provider_->cpuModel(); }
- Line 447: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (config_.include_cpu_cores) { snap.cpu_cores    = hw_provider_->cpuCores(); }
- Line 448: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (config_.include_ram_mb)    { snap.total_ram_mb = hw_provider_->totalRamMb(); }
- Line 449: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (config_.include_os)        { snap.os_family    = hw_provider_->osFamily(); }
- Line 450: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (config_.include_arch)      { snap.cpu_arch     = hw_provider_->cpuArch(); }
- Line 200: severity=HIGH; category=windows_only_api
  Description: Windows-only API RegOpenKeyEx without platform guard
  Remediation: Wrap in #ifdef _WIN32 ... #endif or provide cross-platform abstraction
  Context: if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
- Line 399: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf),
  Confidence: band=very_high; score=0.9
- Line 498: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(2));
- Line 514: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return send(collect());
- Line 545: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return running_.load(std::memory_order_acquire);
- Line 558: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!stop_requested_.load(std::memory_order_acquire)) {
- Line 565: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (!stop_requested_.load(std::memory_order_acquire)) {

### src/updates/tenant_update_scheduler.cpp
Total findings: 12

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        const std::time_t candidate_t =', '            midnight_t', '            + static_cast<std::time_t>(offset * 24 * 3600)  // advance days', '            + static_cast<std::time_t>(start_min * 60);      // add start time', '']
  Confidence: band=very_high; score=0.9
- Line 99: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = tenants_.find(tenant_id);
- Line 475: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = tenants_.find(tenant_id);
- Line 45: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 284: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& c : s)
  Confidence: band=very_high; score=0.9
- Line 524: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [tid, state] : tenants_) {
  Confidence: band=very_high; score=0.9
- Line 505: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: TenantUpdateStatus s;
- Line 522: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<TenantUpdateStatus> result;
- Line 525: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: TenantUpdateStatus s;
- Line 533: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 534: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(s));
- Line 561: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/updates/cluster_update_manager.cpp
Total findings: 11

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 34: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 77: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < node_statuses_.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const ClusterNode& node : sorted_nodes_) {
  Confidence: band=very_high; score=0.9
- Line 406: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 411: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 43: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sorted_nodes_.push_back(n);
- Line 48: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sorted_nodes_.push_back(n);
- Line 55: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ClusterNodeStatus s;
- Line 58: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_statuses_.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 59: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node_statuses_.push_back(std::move(s));

### src/updates/preflight_health_check.cpp
Total findings: 10

- Line 195: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 255: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("PreflightHealthChecker::addCheck: check must not be null");
- Line 59: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MEMORYSTATUSEX status{};
- Line 87: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(std::stoi(token));
- Line 88: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 89: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(0);
- Line 256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checks_.push_back(std::move(check));
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: checks_.push_back(std::move(check));
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.results.push_back(std::move(cr));
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.results.push_back(std::move(cr));

### src/updates/release_manifest.cpp
Total findings: 10

- Line 106: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["files"] = files_array;
- Line 210: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["files"] = files_array;
- Line 71: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_array.push_back(file.toJson());
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files_array.push_back(file.toJson());
- Line 163: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.files.push_back(*file);
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: manifest.files.push_back(*file);
- Line 192: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_array.push_back(file.toJson());
  Confidence: band=high; score=0.74
- Line 208: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files_array.push_back(file.toJson());

### src/updates/build_verifier.cpp
Total findings: 6

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((val >> bits) & 0xFF));
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>((val >> bits) & 0xFF));
- Line 125: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);
- Line 143: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 144: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);

### src/updates/notification_webhook.cpp
Total findings: 5

- Line 138: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 196: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!files_str.empty()) files_str += "\n";
  Confidence: band=high; score=0.74
- Line 197: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!files_str.empty()) files_str += "\n";
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back({{"title", "Files Updated"},
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({{"title", "Files Updated"},

### src/updates/update_history_logger.cpp
Total findings: 4

- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(UpdateHistoryEntry::fromJson(item));
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(UpdateHistoryEntry::fromJson(item));
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJson());
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(e.toJson());

### src/updates/blue_green_deployment.cpp
Total findings: 3

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 285: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: BlueGreenStatus s;

### src/updates/schema_migration_tester.cpp
Total findings: 2

- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: user_test_cases_.push_back(std::move(tc));
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.test_results.push_back(tr);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
