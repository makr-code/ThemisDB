# updates Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: updates
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 266
- Actionable Findings (Critical + High): 100
- Affected Files: 21

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 16 |
| High | 84 |
| Medium | 166 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 85 |
| performance_patterns | 77 |
| reliability | 31 |
| exception_safety | 22 |
| raii | 17 |
| concurrency | 10 |
| memory | 7 |
| performance | 4 |
| platform | 4 |
| audit_logging | 3 |
| determinism | 3 |
| security | 2 |
| legacy_duplication | 1 |
| type_conversion | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/updates/schema_migration.cpp | 54 | 0 | 45 | 9 | 0 |
| src/updates/delta_update_engine.cpp | 34 | 0 | 2 | 32 | 0 |
| src/updates/dependency_resolver.cpp | 22 | 0 | 2 | 20 | 0 |
| src/updates/in_place_schema_migrator.cpp | 21 | 0 | 6 | 15 | 0 |
| src/updates/updates_config.cpp | 20 | 0 | 0 | 20 | 0 |
| src/updates/hot_reload_engine.cpp | 17 | 2 | 4 | 11 | 0 |
| src/updates/manifest_database.cpp | 17 | 4 | 6 | 7 | 0 |
| src/updates/parallel_downloader.cpp | 12 | 4 | 5 | 3 | 0 |
| src/updates/hardware_telemetry.cpp | 11 | 5 | 6 | 0 | 0 |
| src/updates/update_state_machine.cpp | 10 | 0 | 3 | 7 | 0 |
| src/updates/release_manifest.cpp | 9 | 0 | 1 | 8 | 0 |
| src/updates/canary_rollout.cpp | 7 | 0 | 1 | 6 | 0 |
| src/updates/tenant_update_scheduler.cpp | 6 | 1 | 0 | 5 | 0 |
| src/updates/preflight_health_check.cpp | 5 | 0 | 0 | 5 | 0 |
| src/updates/build_verifier.cpp | 4 | 0 | 1 | 3 | 0 |
| src/updates/notification_webhook.cpp | 4 | 0 | 0 | 4 | 0 |
| src/updates/update_history_logger.cpp | 4 | 0 | 0 | 4 | 0 |
| src/updates/cluster_update_manager.cpp | 3 | 0 | 1 | 2 | 0 |
| src/updates/blue_green_deployment.cpp | 2 | 0 | 1 | 1 | 0 |
| src/updates/coordinated_update_manager.cpp | 2 | 0 | 0 | 2 | 0 |
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
- Line 208: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: starting online DDL ({} operations)",
- Line 216: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase SHADOW_CREATE", version_);
- Line 220: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase DUAL_WRITE", version_);
- Line 224: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase BACKFILL", version_);
- Line 234: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: {}", version_, result.error_message);
- Line 239: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_WARN("SchemaMigration [{}]: rollback also failed: {}",
- Line 249: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase CONSISTENCY_CHECK", version_);
- Line 254: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase ATOMIC_SWAP (shadow → main)", version_);
- Line 259: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: phase CLEANUP", version_);
- Line 265: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: upgrading schema version marker: {} → {}",
- Line 269: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_WARN("SchemaMigration [{}]: could not persist schema version marker",
- Line 283: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: migration completed successfully "
- Line 306: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: rolling back from phase {}",
- Line 314: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: rollback failed to restore key '{}'",
- Line 320: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: rollback failed to remove key '{}'",
- Line 344: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: addColumn '{}' has empty column name",
- Line 349: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: addColumn has empty table name", version_);
- Line 367: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write column metadata for '{}.{}'",
- Line 383: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: renameColumn '{}' has empty column name",
- Line 388: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: renameColumn has empty table name", version_);
- Line 421: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write renamed column metadata '{}.{}'",
- Line 427: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to remove old column metadata '{}.{}'",
- Line 434: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write rename marker for '{}.{}'",
- Line 439: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: renameColumn '{}.{}' → '{}'",
- Line 447: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: addIndex '{}' has empty index name",
- Line 452: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: addIndex '{}' on table '{}' has no columns",
- Line 457: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: addIndex has empty table name", version_);
- Line 481: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write index metadata '{}.{}'",
- Line 500: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: dropColumn '{}' has empty column name",
- Line 505: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: dropColumn has empty table name", version_);
- Line 532: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write drop marker for '{}.{}'",
- Line 539: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: failed to remove column metadata for '{}.{}'",
- Line 557: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_WARN("SchemaMigration [{}]: listKeys() unsupported or failed; "
- Line 564: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_ERROR("SchemaMigration [{}]: custom migration callback returned false",
- Line 567: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: LOG_INFO("SchemaMigration [{}]: custom migration callback succeeded",
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 372: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backfilled_columns_.push_back(op.column.name);
- Line 415: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: undo_log_.push_back({op.table, new_meta_key, existing_new_meta, had_new_meta});
- Line 416: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: undo_log_.push_back({op.table, old_meta_key, meta_value, had_old});
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: undo_log_.push_back({op.table, rename_key, existing_rename, had_rename});
- Line 466: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) cols += ",";
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) cols += ",";
- Line 477: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: undo_log_.push_back({op.table, idx_key, existing, had_value});
  Confidence: band=high; score=0.74
- Line 478: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: undo_log_.push_back({op.table, idx_key, existing, had_value});

### src/updates/delta_update_engine.cpp
Total findings: 34

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3661 feat(updates): build system... (2026-03-12) | #2586 Fix stale banner me
- Line 61: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (rel_path[0] == '/' || rel_path[0] == '\\' || rel_path.find('\0') != std::string::npos) return fa
- Line 61: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (rel_path[0] == '/' || rel_path[0] == '\\' || rel_path.find('\0') != std::string::npos) return fa
- Line 61: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (rel_path[0] == '/' || rel_path[0] == '\\' || rel_path.find('\0') != std::string::npos) return fa
- Line 82: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 134: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 137: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 228: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["deltas"].push_back(d.toJson());
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["deltas"].push_back(d.toJson());
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["deltas"].push_back(d.toJson());
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["deltas"].push_back(d.toJson());
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (fd) dm.deltas.push_back(*fd);
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (fd) dm.deltas.push_back(*fd);
- Line 273: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 311: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: registered_deltas_.push_back(manifest);
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.files_fallback.push_back(fd.path);
  Confidence: band=high; score=0.74
- Line 343: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 357: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 367: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 377: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 389: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 399: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 410: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_patched.push_back(fd.path);
- Line 415: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.files_fallback.push_back(fd.path);
- Line 655: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(val        & 0xFF));
- Line 656: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((val >>  8) & 0xFF));
- Line 657: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
- Line 658: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
- Line 681: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ht[h].push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 681: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ht[h].push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 682: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ht[h].push_back(static_cast<uint32_t>(i));
- Line 720: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: instructions.push_back(INSTR_COPY);
  Confidence: band=high; score=0.74

### src/updates/dependency_resolver.cpp
Total findings: 22

- Line 360: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it_pkg = deps_.find(pkg);
- Line 363: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it_ver = it_pkg->second.find(tgt_ver);
- Line 71: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(themis::utils::trim(cur));
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(themis::utils::trim(cur));
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(themis::utils::trim(cur));
- Line 245: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back({
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.conflicts.push_back({
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.conflicts.push_back({
- Line 368: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> added_edges;
  Confidence: band=medium; score=0.66
- Line 376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successors[dep.package].push_back(pkg);
  Confidence: band=high; score=0.74
- Line 376: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successors[dep.package].push_back(pkg);
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (kv.second == 0) ready.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 386: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (kv.second == 0) ready.push_back(kv.first);
- Line 409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (kv.second > 0) cycle_nodes.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (kv.second > 0) cycle_nodes.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (kv.second > 0) cycle_nodes.push_back(kv.first);
- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(std::move(step));
  Confidence: band=high; score=0.74
- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(std::move(step));
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> installed_map;
  Confidence: band=medium; score=0.66
- Line 462: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back({p1, p2, reason});
  Confidence: band=high; score=0.74
- Line 462: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: conflicts.push_back({p1, p2, reason});
  Confidence: band=high; score=0.74
- Line 463: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: conflicts.push_back({p1, p2, reason});

### src/updates/in_place_schema_migrator.cpp
Total findings: 21

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 87: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (from_names.find(p.name) == from_names.end()) {
- Line 87: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (from_names.find(p.name) == from_names.end()) {
- Line 45: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> from_props;
  Confidence: band=high; score=0.74
- Line 51: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> to_props;
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, bool> from_names;
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added.push_back(p.name);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: added.push_back(p.name);
  Confidence: band=high; score=0.74
- Line 89: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: added.push_back(p.name);
- Line 106: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> from_map;
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> to_map;
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.added_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.removed_columns.push_back(p);
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.modified_columns.push_back(std::move(mod));
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) cols_str += ", ";
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) cols_str += ", ";

### src/updates/updates_config.cpp
Total findings: 20

- Line 39: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: runtime.stages.push_back(stage);
  Confidence: band=high; score=0.74
- Line 52: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto checker = config["updates"]["checker"];
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auto_update = config["updates"]["auto_update"];
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.auto_update.schedule_days.push_back(day.as<std::string>());
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.auto_update.schedule_days.push_back(day.as<std::string>());
- Line 91: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hot_reload = config["updates"]["hot_reload"];
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto notifications = config["updates"]["notifications"];
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.notifications.on_events.push_back(event.as<std::string>());
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.notifications.on_events.push_back(event.as<std::string>());
- Line 126: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto canary_yaml = config["updates"]["canary"];
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.canary.stages.push_back(stage);
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto tel = config["updates"]["telemetry"];
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto checker = j["checker"];
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto auto_update = j["auto_update"];
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hot_reload = j["hot_reload"];
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto notifications = j["notifications"];
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto canary_json = j["canary"];
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.canary.stages.push_back(stage);
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto tel = j["telemetry"];
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["canary"]["stages"].push_back(stage_json);
  Confidence: band=high; score=0.74

### src/updates/hot_reload_engine.cpp
Total findings: 17

- Line 97: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cached_path = manifest_db_->getCachedDownload(version, file.path);
- Line 471: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* fp = fopen(dest.c_str(), "wb");
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3432 [WIP] Update HSM-based bund... (2026-03-12) | #3419 feat(updates): auto
- Line 184: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check compatibility
  Confidence: band=high; score=0.8
- Line 397: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(config_.backup_directory)) {
- Line 549: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata["files"].push_back({{"path", file.path}});
- Line 346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("File not downloaded: " + file.path);
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back("File not downloaded: " + file.path);
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rollback_points.emplace_back(rollback_id, timestamp);
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (newest first)
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(fp);
- Line 548: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata["files"].push_back({{"path", file.path}});
  Confidence: band=high; score=0.74
- Line 548: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata["files"].push_back({{"path", file.path}});
  Confidence: band=high; score=0.74
- Line 549: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: metadata["files"].push_back({{"path", file.path}});
- Line 602: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 610: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 613: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);

### src/updates/manifest_database.cpp
Total findings: 17

- Line 46: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cf_files = storage_->getOrCreateColumnFamily("file_registry");
- Line 47: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cf_signatures = storage_->getOrCreateColumnFamily("signature_cache");
- Line 48: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cf_cache = storage_->getOrCreateColumnFamily("download_cache");
- Line 302: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sig.signingCertificate = manifest->signing_certificate;
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4261 feat(updates): ManifestData... (2026-03-15) | #2604 feat(updates): Sche
- Line 151: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: it = nullptr;
  Context: delete it;
- Line 458: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: manifest = nullptr;
  Context: LOG_ERROR("Cannot delete manifest {}: not found", version);
- Line 488: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: manifest = nullptr;
  Context: LOG_ERROR("Failed to delete manifest {}: {}", version, status.ToString());
- Line 504: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: file = nullptr;
  Context: LOG_WARN("Failed to delete file registry entry for {}: {}",
- Line 515: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cached = nullptr;
  Context: LOG_WARN("Failed to delete cached file {}: {}",
- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(it->key().ToString());
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: versions.push_back(it->key().ToString());
- Line 151: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete it;
- Line 249: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 403: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 447: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 515: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: LOG_WARN("Failed to delete cached file {}: {}",

### src/updates/parallel_downloader.cpp
Total findings: 12

- Line 255: severity=CRITICAL; category=missing_dtor
  Description: Class WriteCtx allocates resources but has no destructor
  Remediation: Add explicit destructor: ~WriteCtx() { /* cleanup */ }
  Context: class/struct WriteCtx
- Line 268: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* fp = fopen(dest.c_str(), open_mode);
- Line 382: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!backoff.wait()) break;
- Line 505: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: cv.wait(lock, [&]() { return !pq.empty() || all_queued; });
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 262: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: size_t n = fwrite(ptr, sz, nmemb, ctx->fp);
- Line 263: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ctx->written += n;
- Line 504: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(queue_mutex);
- Line 216: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 301: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(fp);
- Line 525: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back(worker);
  Confidence: band=high; score=0.74

### src/updates/hardware_telemetry.cpp
Total findings: 11

- Line 444: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (config_.include_cpu_model) { snap.cpu_model    = hw_provider_->cpuModel(); }
- Line 445: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (config_.include_cpu_cores) { snap.cpu_cores    = hw_provider_->cpuCores(); }
- Line 446: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (config_.include_ram_mb)    { snap.total_ram_mb = hw_provider_->totalRamMb(); }
- Line 447: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (config_.include_os)        { snap.os_family    = hw_provider_->osFamily(); }
- Line 448: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (config_.include_arch)      { snap.cpu_arch     = hw_provider_->cpuArch(); }
- Line 198: severity=HIGH; category=windows_only_api
  Description: Windows-only API RegOpenKeyEx without platform guard
  Remediation: Wrap in #ifdef _WIN32 ... #endif or provide cross-platform abstraction
  Context: if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
- Line 496: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(2));
- Line 512: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return send(collect());
- Line 543: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return running_.load(std::memory_order_acquire);
- Line 556: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!stop_requested_.load(std::memory_order_acquire)) {
- Line 563: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (!stop_requested_.load(std::memory_order_acquire)) {

### src/updates/update_state_machine.cpp
Total findings: 10

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 114: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return state_.load(std::memory_order_acquire);
- Line 97: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 207: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 246: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: callbacks_.push_back(std::move(cb));
  Confidence: band=high; score=0.74
- Line 302: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: transaction_log_.push_back(*entry);
- Line 304: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 448: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}

### src/updates/release_manifest.cpp
Total findings: 9

- Line 208: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["files"] = files_array;
- Line 69: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 101: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_array.push_back(file.toJson());
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files_array.push_back(file.toJson());
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: manifest.files.push_back(*file);
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: manifest.files.push_back(*file);
- Line 190: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 205: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files_array.push_back(file.toJson());
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files_array.push_back(file.toJson());

### src/updates/canary_rollout.cpp
Total findings: 7

- Line 77: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // same inputs – no inter-node communication required.
  Confidence: band=very_high; score=0.9
- Line 314: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: CanaryStatus s;
- Line 471: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.stages.push_back(cs);
  Confidence: band=high; score=0.74
- Line 494: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(stage_info); } catch (...) {}
- Line 507: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { cb(reason); } catch (...) {}
- Line 572: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: latency_samples_us_.push_back(latency.count());
- Line 697: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: CanaryStatus s;

### src/updates/tenant_update_scheduler.cpp
Total findings: 6

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        const std::time_t candidate_t =', '            midnight_t', '            + static_cast<std::time_t>(offset * 24 * 3600)  // advance days', '            + static_cast<std::time_t>(start_min * 60);      // add start time', '']
  Confidence: band=very_high; score=0.9
- Line 503: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: TenantUpdateStatus s;
- Line 520: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<TenantUpdateStatus> result;
- Line 523: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: TenantUpdateStatus s;
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 559: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/updates/preflight_health_check.cpp
Total findings: 5

- Line 57: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MEMORYSTATUSEX status{};
- Line 85: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(std::stoi(token));
- Line 86: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checks_.push_back(std::move(check));
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.results.push_back(std::move(cr));
  Confidence: band=high; score=0.74

### src/updates/build_verifier.cpp
Total findings: 4

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((val >> bits) & 0xFF));
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 142: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pkey);

### src/updates/notification_webhook.cpp
Total findings: 4

- Line 194: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!files_str.empty()) files_str += "\n";
  Confidence: band=high; score=0.74
- Line 195: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!files_str.empty()) files_str += "\n";
- Line 197: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back({{"title", "Files Updated"},
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back({{"title", "Files Updated"},

### src/updates/update_history_logger.cpp
Total findings: 4

- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(UpdateHistoryEntry::fromJson(item));
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(UpdateHistoryEntry::fromJson(item));
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJson());
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(e.toJson());

### src/updates/cluster_update_manager.cpp
Total findings: 3

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 53: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ClusterNodeStatus s;
- Line 56: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_statuses_.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/updates/blue_green_deployment.cpp
Total findings: 2

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 283: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: BlueGreenStatus s;

### src/updates/coordinated_update_manager.cpp
Total findings: 2

- Line 101: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: NodeUpdateStatus s;
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: node_statuses_.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/updates/schema_migration_tester.cpp
Total findings: 2

- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: user_test_cases_.push_back(std::move(tc));
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.test_results.push_back(tr);
  Confidence: band=high; score=0.74

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
