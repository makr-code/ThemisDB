# updates Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: updates
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 175
- Actionable Findings (Critical + High): 115
- Affected Files: 20

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 22 |
| High | 93 |
| Medium | 55 |
| Low | 5 |

## Category Summary

| Category | Count |
|---|---:|
| uninitialized_access | 38 |
| resource_leaked_in_exception | 22 |
| manual_cleanup | 12 |
| unnecessary_copy | 12 |
| data_race | 10 |
| copy_overhead | 8 |
| string_concat_loop | 6 |
| db_connection_leak | 5 |
| delete_no_nullptr | 5 |
| delete_without_nullptr | 5 |
| explicit_delete | 5 |
| map_vs_unordered_map | 5 |
| no_timeout | 4 |
| pointer_arithmetic_unbounded | 4 |
| generic_catch | 3 |
| hardcoded_output | 3 |
| o_n_squared | 3 |
| thread_join_no_timeout | 3 |
| uncaught_exception | 3 |
| blocking_no_timeout | 2 |
| module_doc_linkset_drift | 2 |
| null_dereference | 2 |
| range_temporary | 2 |
| unordered_container_iter | 2 |
| hardcoded_path | 1 |
| legacy_or_compat_path | 1 |
| lock_contention | 1 |
| missing_dtor | 1 |
| multiplication_overflow | 1 |
| primitive_no_volatile | 1 |
| timestamp_sorting_unstable | 1 |
| uninitialized_member_field | 1 |
| windows_only_api | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| updates/schema_migration.cpp | 48 | 0 | 45 | 3 | 0 |
| updates/manifest_database.cpp | 23 | 4 | 16 | 3 | 0 |
| updates/parallel_downloader.cpp | 16 | 9 | 5 | 2 | 0 |
| updates/hot_reload_engine.cpp | 14 | 2 | 7 | 5 | 0 |
| updates/hardware_telemetry.cpp | 13 | 6 | 5 | 1 | 1 |
| updates/in_place_schema_migrator.cpp | 13 | 0 | 5 | 8 | 0 |
| updates/updates_config.cpp | 12 | 0 | 0 | 12 | 0 |
| updates/dependency_resolver.cpp | 9 | 0 | 2 | 6 | 1 |
| updates/canary_rollout.cpp | 5 | 0 | 0 | 4 | 1 |
| updates/delta_update_engine.cpp | 5 | 0 | 2 | 3 | 0 |
| updates/update_state_machine.cpp | 5 | 0 | 3 | 2 | 0 |
| updates/build_verifier.cpp | 3 | 0 | 1 | 2 | 0 |
| updates/notification_webhook.cpp | 2 | 0 | 0 | 2 | 0 |
| updates/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| updates/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| updates/blue_green_deployment.cpp | 1 | 0 | 1 | 0 | 0 |
| updates/cluster_update_manager.cpp | 1 | 0 | 1 | 0 | 0 |
| updates/preflight_health_check.cpp | 1 | 0 | 0 | 1 | 0 |
| updates/tenant_update_scheduler.cpp | 1 | 1 | 0 | 0 | 0 |
| updates/update_history_logger.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### updates/schema_migration.cpp
Total findings: 48

- Line 41: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 208: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: starting online DDL ({} operations)",
- Line 216: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: phase SHADOW_CREATE", version_);
- Line 220: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: phase DUAL_WRITE", version_);
- Line 224: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: phase BACKFILL", version_);
- Line 234: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: {}", version_, result.error_message);
- Line 239: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_WARN("SchemaMigration [{}]: rollback also failed: {}",
- Line 249: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: phase CONSISTENCY_CHECK", version_);
- Line 254: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: phase ATOMIC_SWAP (shadow → main)", version_);
- Line 259: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: phase CLEANUP", version_);
- Line 265: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: upgrading schema version marker: {} → {}",
- Line 269: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_WARN("SchemaMigration [{}]: could not persist schema version marker",
- Line 283: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: migration completed successfully "
- Line 306: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: rolling back from phase {}",
- Line 314: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: rollback failed to restore key '{}'",
- Line 320: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: rollback failed to remove key '{}'",
- Line 344: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: addColumn '{}' has empty column name",
- Line 349: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: addColumn has empty table name", version_);
- Line 367: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write column metadata for '{}.{}'",
- Line 383: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: renameColumn '{}' has empty column name",
- Line 388: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: renameColumn has empty table name", version_);
- Line 394: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 399: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 404: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 410: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 411: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 415: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 421: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write renamed column metadata '{}.{}'",
- Line 422: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 427: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: failed to remove old column metadata '{}.{}'",
- Line 434: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write rename marker for '{}.{}'",
- Line 439: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: renameColumn '{}.{}' → '{}'",
- Line 440: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 447: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: addIndex '{}' has empty index name",
- Line 452: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: addIndex '{}' on table '{}' has no columns",
- Line 457: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: addIndex has empty table name", version_);
- Line 481: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write index metadata '{}.{}'",
- Line 500: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: dropColumn '{}' has empty column name",
- Line 505: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: dropColumn has empty table name", version_);
- Line 532: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: failed to write drop marker for '{}.{}'",
- Line 539: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: failed to remove column metadata for '{}.{}'",
- Line 557: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_WARN("SchemaMigration [{}]: listKeys() unsupported or failed; "
- Line 564: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_ERROR("SchemaMigration [{}]: custom migration callback returned false",
- Line 567: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: LOG_INFO("SchemaMigration [{}]: custom migration callback succeeded",
- Line 612: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 177: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 466: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i) cols += ",";
- Line 467: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i) cols += ",";

### updates/manifest_database.cpp
Total findings: 23

- Line 46: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cf_files = storage_->getOrCreateColumnFamily("file_registry");
- Line 47: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cf_signatures = storage_->getOrCreateColumnFamily("signature_cache");
- Line 48: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cf_cache = storage_->getOrCreateColumnFamily("download_cache");
- Line 302: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: sig.signingCertificate = manifest->signing_certificate;
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4261 feat(updates): ManifestData... (2026-03-15) | #2604 feat(updates): Sche
- Line 151: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete it;
- Line 151: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: versions.push_back(it->key().ToString());

        }

        

        delete it;

        

        // Sort versions

        std::sort(versions.begin(), versions.end());
- Line 151: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete it;
- Line 458: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: LOG_ERROR("Cannot delete manifest {}: not found", version);
- Line 458: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Also acts as an existence check: if the manifest is absent, abort early.

        auto manifest_opt = getManifest(version);

        if (!manifest_opt) {

            LOG_ERROR("Cannot delete manifest {}: not found", version);

            return false;

        }
- Line 458: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: LOG_ERROR("Cannot delete manifest {}: not found", version);
- Line 488: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: LOG_ERROR("Failed to delete manifest {}: {}", version, status.ToString());
- Line 488: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: );



        if (!status.ok()) {

            LOG_ERROR("Failed to delete manifest {}: {}", version, status.ToString());

            // Remove tombstone so the version is not stuck in a deleting state

            storage_->getRawDB()->Delete(rocksdb::WriteOptions(), manifests_cf, tombstone_key);

            return false;
- Line 488: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: LOG_ERROR("Failed to delete manifest {}: {}", version, status.ToString());
- Line 504: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: LOG_WARN("Failed to delete file registry entry for {}: {}",
- Line 504: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: file_key

            );

            if (!file_status.ok() && !file_status.IsNotFound()) {

                LOG_WARN("Failed to delete file registry entry for {}: {}",

                         file.path, file_status.ToString());

            }
- Line 504: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: LOG_WARN("Failed to delete file registry entry for {}: {}",
- Line 515: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: LOG_WARN("Failed to delete cached file {}: {}",
- Line 515: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (std::filesystem::remove(*cached_path, ec)) {

                    LOG_DEBUG("Deleted cached file: {}", *cached_path);

                } else if (ec) {

                    LOG_WARN("Failed to delete cached file {}: {}",

                             *cached_path, ec.message());

                }
- Line 515: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: LOG_WARN("Failed to delete cached file {}: {}",
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: versions.push_back(it->key().ToString());
- Line 151: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete it;
- Line 515: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: LOG_WARN("Failed to delete cached file {}: {}",

### updates/parallel_downloader.cpp
Total findings: 16

- Line 123: severity=CRITICAL; category=db_connection_leak
  Description: Database connection from last never released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: int64_t last = last_refill_ms_.load(std::memory_order_acquire);
- Line 255: severity=CRITICAL; category=missing_dtor
  Description: Class WriteCtx allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct WriteCtx
- Line 268: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: FILE* fp = fopen(dest.c_str(), open_mode);
- Line 382: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: for (int attempt = 0; attempt <= task.max_retries; ++attempt) {

        if (attempt > 0) {

            LOG_DEBUG("ParallelDownloader: retry {}/{} for {}", attempt, task.max_retries, task.url);

            if (!backoff.wait()) break;

        }



        uint64_t    bytes_this_call = 0;
- Line 382: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (!backoff.wait()) break;
- Line 382: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (!backoff.wait()) break;
- Line 505: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: size_t idx = SIZE_MAX;

            {

                std::unique_lock<std::mutex> lock(queue_mutex);

                cv.wait(lock, [&]() { return !pq.empty() || all_queued; });

                if (pq.empty()) break;

                idx = pq.top().second;

                pq.pop();
- Line 505: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: cv.wait(lock, [&]() { return !pq.empty() || all_queued; });
- Line 532: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: t.join();
- Line 135: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 196: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 262: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: size_t n = fwrite(ptr, sz, nmemb, ctx->fp);
- Line 263: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->written += n;
- Line 504: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(queue_mutex);
- Line 216: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 301: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fclose(fp);

### updates/hot_reload_engine.cpp
Total findings: 14

- Line 97: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cached_path = manifest_db_->getCachedDownload(version, file.path);
- Line 471: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: FILE* fp = fopen(dest.c_str(), "wb");
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3432 [WIP] Update HSM-based bund... (2026-03-12) | #3419 feat(updates): auto
- Line 184: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Check compatibility
- Line 397: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(config_.backup_directory)) {
- Line 545: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Save metadata

        json metadata;

        metadata["rollback_id"] = rollback_id;

        metadata["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();

        metadata["files"] = json::array();

        for (const auto& file : files) {
- Line 546: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Save metadata

        json metadata;

        metadata["rollback_id"] = rollback_id;

        metadata["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();

        metadata["files"] = json::array();

        for (const auto& file : files) {

            metadata["files"].push_back({{"path", file.path}});
- Line 547: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json metadata;

        metadata["rollback_id"] = rollback_id;

        metadata["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();

        metadata["files"] = json::array();

        for (const auto& file : files) {

            metadata["files"].push_back({{"path", file.path}});

        }
- Line 549: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();

        metadata["files"] = json::array();

        for (const auto& file : files) {

            metadata["files"].push_back({{"path", file.path}});

        }

        

        std::ofstream metadata_file(backup_dir + "/rollback.json");
- Line 413: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort by timestamp (newest first)
- Line 487: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fclose(fp);
- Line 602: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);
- Line 610: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);
- Line 613: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);

### updates/hardware_telemetry.cpp
Total findings: 13

- Line 444: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (config_.include_cpu_model) { snap.cpu_model    = hw_provider_->cpuModel(); }
- Line 445: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (config_.include_cpu_cores) { snap.cpu_cores    = hw_provider_->cpuCores(); }
- Line 446: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (config_.include_ram_mb)    { snap.total_ram_mb = hw_provider_->totalRamMb(); }
- Line 447: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (config_.include_os)        { snap.os_family    = hw_provider_->osFamily(); }
- Line 448: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (config_.include_arch)      { snap.cpu_arch     = hw_provider_->cpuArch(); }
- Line 536: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bg_thread_.join();
- Line 198: severity=HIGH; category=windows_only_api
  Description: Windows-only API RegOpenKeyEx without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
- Line 496: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(2));
- Line 543: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return running_.load(std::memory_order_acquire);
- Line 556: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!stop_requested_.load(std::memory_order_acquire)) {
- Line 563: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (!stop_requested_.load(std::memory_order_acquire)) {
- Line 493: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int attempt = 0; attempt <= config_.max_retries; ++attempt) {
- Line 397: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf),

### updates/in_place_schema_migrator.cpp
Total findings: 13

- Line 69: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 87: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (from_names.find(p.name) == from_names.end()) {
- Line 138: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 140: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 203: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 45: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> from_props;
- Line 51: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> to_props;
- Line 81: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, bool> from_names;
- Line 89: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: added.push_back(p.name);
- Line 106: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> from_map;
- Line 110: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, const SchemaManager::PropertyInfo*> to_map;
- Line 227: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i) cols_str += ", ";
- Line 228: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i) cols_str += ", ";

### updates/updates_config.cpp
Total findings: 12

- Line 52: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto checker = config["updates"]["checker"];
- Line 71: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto auto_update = config["updates"]["auto_update"];
- Line 91: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto hot_reload = config["updates"]["hot_reload"];
- Line 106: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto notifications = config["updates"]["notifications"];
- Line 126: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto canary_yaml = config["updates"]["canary"];
- Line 144: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto tel = config["updates"]["telemetry"];
- Line 182: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto checker = j["checker"];
- Line 196: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto auto_update = j["auto_update"];
- Line 213: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto hot_reload = j["hot_reload"];
- Line 228: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto notifications = j["notifications"];
- Line 241: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto canary_json = j["canary"];
- Line 259: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto tel = j["telemetry"];

### updates/dependency_resolver.cpp
Total findings: 9

- Line 360: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it_pkg = deps_.find(pkg);
- Line 363: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it_ver = it_pkg->second.find(tgt_ver);
- Line 72: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: parts.push_back(themis::utils::trim(cur));
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: parts.push_back(themis::utils::trim(cur));
- Line 368: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> added_edges;
- Line 386: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (kv.second == 0) ready.push_back(kv.first);
- Line 410: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (kv.second > 0) cycle_nodes.push_back(kv.first);
- Line 449: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> installed_map;
- Line 178: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(buf, sizeof(buf), "%d.%d.%d", maj, min, pat + 1);

### updates/canary_rollout.cpp
Total findings: 5

- Line 494: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                }

                if (cb) {

                    try { cb(stage_info); } catch (...) {}

                }

            });
- Line 494: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { cb(stage_info); } catch (...) {}
- Line 507: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: cb = rollback_cb_;

                }

                if (cb) {

                    try { cb(reason); } catch (...) {}

                }

            });
- Line 507: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { cb(reason); } catch (...) {}
- Line 77: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // same inputs – no inter-node communication required.

### updates/delta_update_engine.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3661 feat(updates): build system... (2026-03-12) | #2586 Fix stale banner me
- Line 119: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 61: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (rel_path[0] == '/' || rel_path[0] == '\\' || rel_path.find('\0') != std::string::npos) return fa
- Line 134: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 137: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);

### updates/update_state_machine.cpp
Total findings: 5

- Line 114: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return state_.load(std::memory_order_acquire);
- Line 270: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 421: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 246: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: for (auto& cb : callbacks_copy) {

        try {

            cb(from, UpdateState::IDLE, "");

        } catch (...) {

            // Never let callbacks crash

        }

    }
- Line 246: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### updates/build_verifier.cpp
Total findings: 3

- Line 116: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 141: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 142: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);

### updates/notification_webhook.cpp
Total findings: 2

- Line 194: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!files_str.empty()) files_str += "\n";
- Line 195: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!files_str.empty()) files_str += "\n";

### updates/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### updates/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### updates/blue_green_deployment.cpp
Total findings: 1

- Line 153: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### updates/cluster_update_manager.cpp
Total findings: 1

- Line 286: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### updates/preflight_health_check.cpp
Total findings: 1

- Line 85: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: parts.push_back(std::stoi(token));

### updates/tenant_update_scheduler.cpp
Total findings: 1

- Line 364: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        const std::time_t candidate_t =', '            midnight_t', '            + static_cast<std::time_t>(offset * 24 * 3600)  // advance days', '            + static_cast<std::time_t>(start_min * 60);      // add start time', '']

### updates/update_history_logger.cpp
Total findings: 1

- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: entries.push_back(UpdateHistoryEntry::fromJson(item));

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
