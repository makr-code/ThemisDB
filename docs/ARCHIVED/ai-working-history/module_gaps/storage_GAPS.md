# storage Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: storage
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 837
- Actionable Findings (Critical + High): 677
- Affected Files: 61

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 188 |
| High | 489 |
| Medium | 133 |
| Low | 27 |

## Category Summary

| Category | Count |
|---|---:|
| resource_leaked_in_exception | 118 |
| data_race | 75 |
| size_assumption | 51 |
| null_dereference | 44 |
| model_integrity_gap | 38 |
| unchecked_cuda_call | 35 |
| uninitialized_access | 32 |
| legacy_or_compat_path | 26 |
| db_connection_leak | 23 |
| manual_cleanup | 23 |
| range_temporary | 22 |
| explicit_delete | 17 |
| copy_overhead | 16 |
| delete_without_nullptr | 15 |
| hardcoded_output | 14 |
| no_retry_logic | 13 |
| no_timeout | 13 |
| uncaught_exception | 13 |
| lock_contention | 12 |
| thread_join_no_timeout | 12 |
| unstructured_log | 12 |
| duplicate_qualified_signature | 11 |
| generic_catch | 11 |
| array_bounds | 10 |
| array_bounds_violation | 10 |
| delete_no_nullptr | 9 |
| map_vs_unordered_map | 8 |
| memory_order | 8 |
| arithmetic_overflow | 7 |
| primitive_no_volatile | 7 |
| path_traversal | 6 |
| unchecked_array_index | 6 |
| iterator_invalidation | 5 |
| posix_only_api | 5 |
| string_concat_loop | 5 |
| unordered_container_iter | 5 |
| lock_in_loop | 4 |
| missing_override_keyword | 4 |
| pointer_arithmetic_unbounded | 4 |
| repeated_search | 4 |
| exception_in_destructor | 3 |
| getsnapshot\(\) | 3 |
| hardcoded_path | 3 |
| manual_cleanup_in_destructor | 3 |
| missing_latency_metric | 3 |
| missing_trace_point | 3 |
| o_n_squared | 3 |
| shift_overflow | 3 |
| timestamp_sorting_unstable | 3 |
| unnecessary_copy | 3 |
| unspecified_consistency | 3 |
| blocking_no_timeout | 2 |
| broken_raii_in_assignment | 2 |
| command_injection | 2 |
| coupling_risk_sharding_storage | 2 |
| deadlock_risk | 2 |
| double_lock | 2 |
| explicit_lock_unlock | 2 |
| gpu_memory_leak | 2 |
| missing_consensus | 2 |
| missing_dtor | 2 |
| missing_move_constructor_defaulted | 2 |
| module_doc_linkset_drift | 2 |
| new_without_raii | 2 |
| pure_virtual_unimplemented | 2 |
| smart_ptr_misuse | 2 |
| stale_doc_section_reference | 2 |
| unchecked_memcpy | 2 |
| uninitialized_member_field | 2 |
| windows_only_api | 2 |
| allocation_loop | 1 |
| expensive_inner_op | 1 |
| fp_exact_comparison | 1 |
| missing_adr_reference | 1 |
| missing_resource_limits | 1 |
| missing_vector_reserve | 1 |
| multiplication_overflow | 1 |
| new_without_delete | 1 |
| repeated_lookup | 1 |
| shared_state_no_sync | 1 |
| uninitialized_array | 1 |
| unwrapped_resource | 1 |
| use_after_free_gpu | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| storage/rocksdb_wrapper.cpp | 83 | 40 | 26 | 15 | 2 |
| storage/hierarchical_tucker_decomposer.cpp | 69 | 17 | 52 | 0 | 0 |
| storage/columnar_format.cpp | 59 | 7 | 47 | 5 | 0 |
| storage/gpu_compression.cpp | 52 | 3 | 40 | 9 | 0 |
| storage/backup_manager.cpp | 38 | 1 | 23 | 13 | 1 |
| storage/nvme_manager.cpp | 35 | 22 | 7 | 6 | 0 |
| storage/wom_tree.cpp | 35 | 1 | 26 | 4 | 4 |
| storage/wal_storage.cpp | 26 | 17 | 6 | 3 | 0 |
| storage/database_connection_manager.cpp | 25 | 5 | 13 | 7 | 0 |
| storage/tensor_train_decomposer.cpp | 25 | 1 | 24 | 0 | 0 |
| storage/distributed_transaction_manager.cpp | 23 | 2 | 20 | 1 | 0 |
| storage/storage_audit_logger.cpp | 21 | 0 | 12 | 2 | 7 |
| storage/erasure_coder_factory.cpp | 20 | 2 | 6 | 12 | 0 |
| storage/hlc.cpp | 17 | 0 | 16 | 1 | 0 |
| storage/blob_backend_azure.cpp | 16 | 1 | 15 | 0 | 0 |
| storage/tensor_compaction_filter.cpp | 15 | 6 | 9 | 0 | 0 |
| storage/concurrent_write_controller.cpp | 13 | 4 | 9 | 0 | 0 |
| storage/history_manager.cpp | 13 | 6 | 5 | 2 | 0 |
| storage/merge_operators.cpp | 13 | 1 | 12 | 0 | 0 |
| storage/tiered_storage.cpp | 13 | 1 | 9 | 3 | 0 |
| storage/blob_backend_s3.cpp | 12 | 0 | 12 | 0 | 0 |
| storage/adaptive_compaction.cpp | 11 | 1 | 9 | 0 | 1 |
| storage/blob_redundancy_manager.cpp | 11 | 4 | 5 | 2 | 0 |
| storage/security_signature_manager.cpp | 11 | 0 | 2 | 9 | 0 |
| storage/tensor_network_storage_engine.cpp | 11 | 2 | 4 | 5 | 0 |
| storage/zero_copy_blob_transfer.cpp | 11 | 5 | 4 | 2 | 0 |
| storage/online_schema_migration.cpp | 10 | 0 | 10 | 0 | 0 |
| storage/pitr_manager.cpp | 10 | 2 | 8 | 0 | 0 |
| storage/storage_engine.cpp | 10 | 1 | 9 | 0 | 0 |
| storage/compaction_manager.cpp | 8 | 2 | 1 | 2 | 3 |
| storage/compressed_storage.cpp | 8 | 6 | 2 | 0 | 0 |
| storage/transaction_retry_manager.cpp | 8 | 1 | 4 | 3 | 0 |
| storage/tt_quantizer.cpp | 8 | 5 | 0 | 3 | 0 |
| storage/base_entity.cpp | 7 | 4 | 1 | 2 | 0 |
| storage/disk_space_monitor.cpp | 7 | 1 | 4 | 2 | 0 |
| storage/index_analyzer.cpp | 7 | 1 | 4 | 2 | 0 |
| storage/streaming_ingest_manager.cpp | 7 | 4 | 2 | 1 | 0 |
| storage/columnar_cache.cpp | 6 | 2 | 2 | 2 | 0 |
| storage/erasure_coding_backend.cpp | 6 | 1 | 1 | 3 | 1 |
| storage/index_maintenance.cpp | 6 | 1 | 3 | 2 | 0 |
| storage/key_schema.cpp | 6 | 0 | 6 | 0 | 0 |
| storage/hamming_coder.cpp | 5 | 2 | 1 | 2 | 0 |
| storage/tensor_router.cpp | 5 | 1 | 0 | 0 | 4 |
| storage/blob_backend_gcs.cpp | 4 | 0 | 4 | 0 | 0 |
| storage/ggml_tensor_bridge.cpp | 4 | 3 | 1 | 0 | 0 |
| storage/mvcc_chain_pruner.cpp | 4 | 0 | 4 | 0 | 0 |
| storage/mvcc_store.cpp | 4 | 0 | 0 | 4 | 0 |
| storage/security_signature.cpp | 4 | 1 | 0 | 3 | 0 |
| storage/compression_strategy.cpp | 2 | 1 | 1 | 0 | 0 |
| storage/storage_parquet_exporter.cpp | 2 | 0 | 2 | 0 | 0 |
| storage/ARCHITECTURE.md | 1 | 0 | 0 | 0 | 1 |
| storage/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| storage/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| storage/blob_backend_filesystem.cpp | 1 | 0 | 1 | 0 | 0 |
| storage/blob_backend_webdav.cpp | 1 | 0 | 1 | 0 | 0 |
| storage/gguf_metadata.cpp | 1 | 0 | 0 | 0 | 1 |
| storage/nlp_metadata_extractor.cpp | 1 | 0 | 0 | 1 | 0 |
| storage/raft_mvcc_bridge.cpp | 1 | 0 | 1 | 0 | 0 |
| storage/schema_dead_weight_detector.cpp | 1 | 0 | 1 | 0 | 0 |
| storage/simd_filter.cpp | 1 | 0 | 1 | 0 | 0 |
| storage/storage_layout_advisor.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### storage/rocksdb_wrapper.cpp
Total findings: 83

- Line 99: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (existing_value != nullptr && !existing_value->empty()) {
- Line 100: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (existing_value->size() == sizeof(uint64_t)) {
- Line 290: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->write_buffer_size = config_.memtable_size_mb * 1024 * 1024;
- Line 291: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->max_write_buffer_number = config_.max_write_buffer_number;
- Line 292: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->min_write_buffer_number_to_merge = config_.min_write_buffer_number_to_merge;
- Line 354: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->max_background_jobs = config_.max_background_jobs;
- Line 358: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->max_background_compactions = config_.max_background_compactions;
- Line 361: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->max_background_flushes = config_.max_background_flushes;
- Line 364: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->max_subcompactions = config_.max_subcompactions;
- Line 371: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->level_compaction_dynamic_level_bytes = config_.dynamic_level_bytes;
- Line 372: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->target_file_size_base = config_.target_file_size_base_mb * 1024ull * 1024ull;
- Line 373: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->max_bytes_for_level_base = config_.max_bytes_for_level_base_mb * 1024ull * 1024ull;
- Line 376: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->level0_file_num_compaction_trigger = config_.level0_file_num_compaction_trigger;
- Line 377: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->level0_slowdown_writes_trigger = config_.level0_slowdown_writes_trigger;
- Line 378: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->level0_stop_writes_trigger = config_.level0_stop_writes_trigger;
- Line 385: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->db_write_buffer_size = config_.db_write_buffer_size_mb * 1024ull * 1024ull;
- Line 401: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->compression = toCompression(config_.compression_default);
- Line 402: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->bottommost_compression = toCompression(config_.compression_bottommost);
- Line 414: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->allow_concurrent_memtable_write = config_.allow_concurrent_memtable_write;
- Line 444: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: write_options_->disableWAL = config_.disable_wal_for_benchmark;  // Phase 2F: Benchmark optimization
- Line 446: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->wal_dir = config_.wal_dir;
- Line 484: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->max_background_jobs = static_cast<int>(recommended_threads);
- Line 490: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->use_direct_reads = config_.use_direct_reads;
- Line 515: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->two_write_queues = config_.two_write_queues;
- Line 523: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: txn_options_->set_snapshot = true; // Automatically create snapshot on begin
- Line 548: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->paranoid_checks = config_.paranoid_checks;
- Line 551: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: read_options_->verify_checksums = config_.verify_checksums_on_read;
- Line 566: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->wal_bytes_per_sync = config_.wal_bytes_per_sync;
- Line 573: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->allow_mmap_reads = false;
- Line 576: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: options_->allow_mmap_writes = false;
- Line 580: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: bool RocksDBWrapper::open() {
- Line 633: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: db_opts.create_missing_column_families = options_->create_missing_column_families;
- Line 716: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: THEMIS_WARN("Database already open during open() - closing existing connection first");
- Line 1974: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint64_t block_cache_hit = stats->getTickerCount(rocksdb::BLOCK_CACHE_HIT);
- Line 1975: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint64_t block_cache_miss = stats->getTickerCount(rocksdb::BLOCK_CACHE_MISS);
- Line 2165: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (!open()) {
- Line 2325: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: s = backup_engine->RestoreDBFromLatestBackup(config_.db_path, config_.db_path);
- Line 2333: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (!open()) {
- Line 2396: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats_obj["block_cache_miss"] = stats->getTickerCount(rocksdb::BLOCK_CACHE_MISS);
- Line 2397: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats_obj["block_cache_hit"] = stats->getTickerCount(rocksdb::BLOCK_CACHE_HIT);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4596 perf(storage): fix ~79x sus... (2026-04-13) | #4494 [PERF-D5] Streaming
- Line 95: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 100: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (existing_value->size() == sizeof(uint64_t)) {
- Line 101: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&base, existing_value->data(), sizeof(uint64_t));
- Line 104: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // intentional backward-compatibility path for keys written before the
- Line 106: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy decimal-string compatibility
- Line 117: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (value.size() == sizeof(uint64_t)) {
- Line 118: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&delta, value.data(), sizeof(uint64_t));
- Line 122: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 122: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: new_value->resize(sizeof(uint64_t));
- Line 123: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 123: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(new_value->data(), &result, sizeof(uint64_t));
- Line 143: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: RocksDBWrapper::~RocksDBWrapper() {
- Line 149: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (is_being_moved_.load(std::memory_order_acquire)) {
- Line 296: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // and TransactionDBOptions compatibility skip are intentional build-variance guards
- Line 298: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Prefer HyperClockCache if available; fallback to LRUCache for compatibility
- Line 301: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Use LRU cache universally for maximum compatibility.
- Line 500: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Skip setting unavailable TransactionDBOptions fields to preserve compatibility.
- Line 776: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (active_operations_.load(std::memory_order_acquire) > 0) {
- Line 777: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 921: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: themis::utils::Logger::error("RocksDBWrapper::del (transaction): delete failed");
- Line 921: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // delete_no_nullptr scanner alert (line 880): txn is a non-null shared_ptr

    // (checked above); calling txn->del() is safe — false positive.

    if (!txn->del(key)) {

        themis::utils::Logger::error("RocksDBWrapper::del (transaction): delete failed");

        txn->rollback();

        return false;

    }
- Line 921: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: themis::utils::Logger::error("RocksDBWrapper::del (transaction): delete failed");
- Line 1410: severity=HIGH; category=missing_trace_point
  Description: Critical function commit without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool RocksDBWrapper::WriteBatchWithIndexWrapper::commit() {
- Line 1616: severity=HIGH; category=missing_trace_point
  Description: Critical function commit without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool RocksDBWrapper::TransactionWrapper::commit() {
- Line 2199: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 110: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                    base = std::stoull(

                        std::string(existing_value->data(), existing_value->size()));

                } catch (...) {

                    base = 0;

                }

            }
- Line 110: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 172: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool expected = false;
- Line 194: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 203: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool expected = false;
- Line 583: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close();
- Line 750: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void RocksDBWrapper::close() {
- Line 775: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int wait_count = 0;
- Line 1497: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: txn_.release();  // Intentional leak in rare edge case (DB shutdown)
- Line 1515: severity=MEDIUM; category=getsnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::deprecated_apis
  Context: read_opts.snapshot = txn_->GetSnapshot();
- Line 1537: severity=MEDIUM; category=getsnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::deprecated_apis
  Context: read_opts.snapshot = txn_->GetSnapshot();
- Line 1696: severity=MEDIUM; category=getsnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::deprecated_apis
  Context: return Ok(txn_->GetSnapshot());
- Line 1963: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (level > 0) num_files_at_levels += ", ";
- Line 1964: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (level > 0) num_files_at_levels += ", ";
- Line 1965: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: num_files_at_levels += "\"L" + std::to_string(level) + "\": " + std::to_string(num_files);
- Line 984: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // audit_logging scanner alert (line 945): snprintf writes into a local fixed-size
- Line 989: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(buf, sizeof(buf), "%06u", idx);

### storage/hierarchical_tucker_decomposer.cpp
Total findings: 69

- Line 33: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // CRC32 (IEEE polynomial) — used to detect blob corruption on deserialize.
- Line 118: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // by deserializeNode (readFloats fills the vector from the byte stream)
- Line 283: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::unique_ptr<HTNode> deserializeNode(Reader& r) {
- Line 288: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // deserializeNode is a file-scope static helper invoked exclusively from
- Line 289: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // HTTrain::deserialize, which verifies a CRC32 trailer over the entire
- Line 299: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: node->rank = static_cast<std::size_t>(rank_u);
- Line 306: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: node->mode_index = static_cast<std::size_t>(mi);
- Line 307: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: node->n_k        = static_cast<std::size_t>(nk);
- Line 317: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: node->r_left  = static_cast<std::size_t>(rl);
- Line 318: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: node->r_right = static_cast<std::size_t>(rr);
- Line 320: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: node->left  = deserializeNode(r);
- Line 321: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: node->right = deserializeNode(r);
- Line 387: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: ht.root = deserializeNode(r);
- Line 700: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: leaf_left->U          = U_cache[L];
- Line 708: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: leaf_right->U          = U_cache[L + 1];
- Line 796: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: node->left = buildHTNode(G_left, left_shape, L, M, U_cache, T_shape);
- Line 802: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: node->right = buildHTNode(G_right, right_shape, M, R, U_cache, T_shape);
- Line 121: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return node.U;  // already stored as [n_k × rank] row-major
- Line 152: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (fl == 0.0f) continue;
- Line 175: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // toTTTrain — compatibility bridge with memoization (stub #286 resolved)
- Line 176: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // legacy_duplication scanner alert: the word "compatibility" in the comment
- Line 178: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // not a legacy code path — false positive.
- Line 299: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->rank = static_cast<std::size_t>(rank_u);
- Line 302: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->is_leaf = true;
- Line 306: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->mode_index = static_cast<std::size_t>(mi);
- Line 307: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->n_k        = static_cast<std::size_t>(nk);
- Line 308: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!r.readFloats(node->U)) return nullptr;
- Line 313: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->is_leaf = false;
- Line 317: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->r_left  = static_cast<std::size_t>(rl);
- Line 318: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->r_right = static_cast<std::size_t>(rr);
- Line 319: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!r.readFloats(node->B)) return nullptr;
- Line 320: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->left  = deserializeNode(r);
- Line 321: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->right = deserializeNode(r);
- Line 348: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Verify trailing CRC32 (4 bytes) if present.  Legacy blobs without a
- Line 354: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    constexpr size_t kCrcSize = 4;', '    if (size >= kCrcSize) {', '        const size_t payload_size = size - kCrcSize;', '        uint32_t stored_crc = 0;', '        for (int i = 0; i < 4; ++i)']
- Line 362: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // If mismatch treat as legacy/no-CRC — parse original bytes.
- Line 635: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *   SVD-1: unfold core along [L..M-1] vs [M..R-1, out]
- Line 636: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *          → G_left  [phys_L,...,phys_{M-1}, r_inner]
- Line 637: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *          → G_right_raw  [n_right * r_out, r_inner]
- Line 638: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *   SVD-2: unfold G_right_raw as [n_right, r_out * r_inner]
- Line 639: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *          → G_right  [phys_M,...,phys_{R-1}, r_23]
- Line 640: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *          → B_node   [r_inner, r_23, r_out]  (transfer tensor at this node)
- Line 663: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->is_leaf    = true;
- Line 664: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->mode_index = L;
- Line 665: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->n_k        = n_L;
- Line 666: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->rank       = r_out;
- Line 667: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->U.assign(n_L * r_out, 0.0f);
- Line 674: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->U[i * r_out + ao] = val;
- Line 686: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->is_leaf  = false;
- Line 687: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->r_left   = phys_L;
- Line 688: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->r_right  = phys_R;
- Line 689: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->rank     = r_out;
- Line 690: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->B        = core;  // [phys_L × phys_R × r_out]
- Line 710: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->left  = std::move(leaf_left);
- Line 711: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->right = std::move(leaf_right);
- Line 783: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->is_leaf  = false;
- Line 784: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->r_left   = r_inner;
- Line 785: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->r_right  = r_23;
- Line 786: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->rank     = r_out;
- Line 787: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->B        = std::move(B_node);
- Line 796: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->left = buildHTNode(G_left, left_shape, L, M, U_cache, T_shape);
- Line 802: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node->right = buildHTNode(G_right, right_shape, M, R, U_cache, T_shape);
- Line 879: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 882: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 883: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 888: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: // ‖T - T̃‖_F² = ‖T‖_F² - ‖G‖_F²  (exact for orthonormal U_k)
- Line 947: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 948: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 953: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### storage/columnar_format.cpp
Total findings: 59

- Line 1296: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: Result<ColumnSegment> ColumnSegment::deserialize(const std::vector<uint8_t>& data) {
- Line 1300: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "Segment deserialize: insufficient data"
- Line 1313: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "Segment deserialize: invalid column type"
- Line 1320: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "Segment deserialize: invalid compression codec"
- Line 1329: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: throw std::out_of_range("Segment deserialize: truncated metadata");
- Line 1368: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "Segment deserialize: checksum mismatch"
- Line 1374: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "Segment deserialize: invalid trailer size"
- Line 104: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: encoded.reserve(data.size() * sizeof(int64_t) / 2);
- Line 119: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: encoded.insert(encoded.end(), value_bytes, value_bytes + sizeof(int64_t));
- Line 158: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (pos + 1 + sizeof(int64_t) > encoded.size()) {
- Line 168: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&value, &encoded[pos], sizeof(int64_t));
- Line 169: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: pos += sizeof(int64_t);
- Line 195: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = dictionary.find(str);
- Line 447: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: encoded.insert(encoded.end(), norm_bytes, norm_bytes + sizeof(uint16_t));
- Line 473: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: encoded.insert(encoded.end(), min_bytes, min_bytes + sizeof(int64_t));
- Line 498: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: encoded.insert(encoded.end(), norm_bytes, norm_bytes + sizeof(uint16_t));
- Line 510: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: encoded.insert(encoded.end(), val_bytes, val_bytes + sizeof(int64_t));
- Line 541: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: bytes_per_value = sizeof(uint8_t);
- Line 543: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: bytes_per_value = sizeof(uint16_t);
- Line 567: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: for (uint32_t i = 0; i < count && pos + sizeof(uint16_t) <= encoded.size(); ++i) {
- Line 569: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&normalized, &encoded[pos], sizeof(uint16_t));
- Line 570: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: pos += sizeof(uint16_t);
- Line 587: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (encoded.size() < sizeof(int64_t) + 1 + sizeof(uint32_t)) {
- Line 597: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&min_val, &encoded[pos], sizeof(int64_t));
- Line 598: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: pos += sizeof(int64_t);
- Line 610: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: bytes_per_value = sizeof(uint8_t);
- Line 612: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: bytes_per_value = sizeof(uint16_t);
- Line 616: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: bytes_per_value = sizeof(int64_t);
- Line 636: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: for (uint32_t i = 0; i < count && pos + sizeof(uint16_t) <= encoded.size(); ++i) {
- Line 638: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&normalized, &encoded[pos], sizeof(uint16_t));
- Line 639: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: pos += sizeof(uint16_t);
- Line 650: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: for (uint32_t i = 0; i < count && pos + sizeof(int64_t) <= encoded.size(); ++i) {
- Line 652: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&normalized, &encoded[pos], sizeof(int64_t));
- Line 653: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: pos += sizeof(int64_t);
- Line 701: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: encoded.insert(encoded.end(), ref_bytes, ref_bytes + sizeof(int64_t));
- Line 706: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: encoded.insert(encoded.end(), delta_bytes, delta_bytes + sizeof(int64_t));
- Line 741: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (encoded.size() < sizeof(int64_t)) {
- Line 751: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&reference, &encoded[pos], sizeof(int64_t));
- Line 752: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: pos += sizeof(int64_t);
- Line 757: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: while (pos + sizeof(int64_t) <= encoded.size()) {
- Line 759: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&delta, &encoded[pos], sizeof(int64_t));
- Line 760: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: pos += sizeof(int64_t);
- Line 808: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "LZ4 compression: failed to allocate output buffer"
- Line 877: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "LZ4 decompression: failed to allocate output buffer"
- Line 944: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "Snappy compression: failed to allocate output buffer"
- Line 995: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "Snappy decompression: failed to allocate output buffer"
- Line 1281: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: serialized.insert(serialized.end(), bytes, bytes + sizeof(uint64_t));
- Line 1297: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (data.size() < 2 + 4 * sizeof(uint64_t)) {
- Line 1328: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (pos + sizeof(uint64_t) > data.size()) {
- Line 1332: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&val, &data[pos], sizeof(uint64_t));
- Line 1333: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: pos += sizeof(uint64_t);
- Line 1361: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (trailing_size == sizeof(uint64_t)) {
- Line 1363: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&expected_checksum, &data[pos], sizeof(uint64_t));
- Line 1364: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const uint64_t actual_checksum = calculateSegmentChecksum(data.data(), data.size() - sizeof(uint64_t
- Line 189: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, uint32_t> dictionary;
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: indices.push_back(it->second);
- Line 734: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: decoded.push_back(reference + delta);
- Line 762: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: decoded.push_back(reference + delta);
- Line 1307: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const auto raw_type = data[pos++];

### storage/gpu_compression.cpp
Total findings: 52

- Line 311: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_in
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: static_cast<uint8_t*>(d_in), size, cfg, result,
- Line 661: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: result.error_message = "cudaMalloc failed for device arrays";
- Line 671: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: result.error_message = "cudaMalloc failed for output chunk";
- Line 139: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['//', 'static constexpr size_t kGpuMagicSize = 8;', 'static constexpr uint8_t kGpuMagic[kGpuMagicSize] = {', '    \'T\', \'G\', \'C\', \'P\', \'R\', \'S\', 1, 0   // "TGCPRS" + version 1.0', '};']
- Line 294: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: result.error_message = std::string("cudaMalloc input: ") +
- Line 301: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_in);
- Line 302: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: result.error_message = std::string("cudaMemcpyAsync H2D: ") +
- Line 329: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaFree(d_in);
- Line 385: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
- Line 396: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: for (void* p : to_free) cudaFree(p);
- Line 411: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t e = cudaMemcpyAsync(d_in_bufs[i], h_ptrs[i], h_sizes[i],
- Line 414: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::error("[gpu_compress] cudaMemcpyAsync H2D[{}] failed: {}",
- Line 481: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpyAsync(d_in_ptrs_arr, d_in_bufs.data(),
- Line 484: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpyAsync(d_out_ptrs_arr, d_out_bufs.data(),
- Line 487: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpyAsync(d_in_sz_arr, h_in_sizes.data(),
- Line 532: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpy(h_out_sizes.data(), d_out_sz_arr,
- Line 548: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpy(results[i].data.data() + hdr,
- Line 603: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // cudaMalloc, and callers check !cuda_alloc(...) before using ptr.
- Line 606: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
- Line 615: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // previously recorded after successful cudaMalloc via cuda_alloc() —
- Line 617: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: for (void* p : to_free) cudaFree(p);
- Line 661: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: result.error_message = "cudaMalloc failed for device arrays";
- Line 671: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: result.error_message = "cudaMalloc failed for output chunk";
- Line 677: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpyAsync(d_in_ptrs, h_in_ptrs.data(),
- Line 680: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpyAsync(d_in_sizes, h_in_sizes.data(),
- Line 683: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpyAsync(d_out_ptrs, h_out_ptrs.data(),
- Line 721: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpy(h_out_sizes.data(), d_out_sizes,
- Line 722: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: n_chunks * sizeof(size_t), cudaMemcpyDeviceToHost);
- Line 747: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpy(p, h_out_ptrs[i], h_out_sizes[i],
- Line 789: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
- Line 800: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: for (void* p : to_free) cudaFree(p);
- Line 813: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t e = cudaMemcpyAsync(h_in_ptrs[i], chunk_data, cs,
- Line 816: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::error("[gpu_compress] cudaMemcpyAsync H2D chunk[{}] failed: {}",
- Line 856: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpyAsync(d_in_ptrs,  h_in_ptrs.data(),
- Line 859: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpyAsync(d_in_sizes, h_chunk_sizes.data(),
- Line 862: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpyAsync(d_out_ptrs,  h_out_ptrs.data(),
- Line 865: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpyAsync(d_out_sizes, h_out_sizes.data(),
- Line 904: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpy(h_out_sizes.data(), d_out_sizes,
- Line 914: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: e = cudaMemcpy(result.data() + off, h_out_ptrs[i],
- Line 1272: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t idx : gpu_indices) {
- Line 1276: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(mu_);
- Line 1286: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: { std::lock_guard<std::mutex> lk(mu_); ++stats_.cpu_fallbacks; }
- Line 1425: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static constexpr size_t kLz4HeaderSize = sizeof(uint64_t);
- Line 84: severity=MEDIUM; category=missing_override_keyword
  Description: C++11: override keyword improves code clarity and catches errors
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 84: severity=MEDIUM; category=pure_virtual_unimplemented
  Description: Ensure all derived classes provide implementation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 85: severity=MEDIUM; category=missing_override_keyword
  Description: C++11: override keyword improves code clarity and catches errors
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 85: severity=MEDIUM; category=pure_virtual_unimplemented
  Description: Ensure all derived classes provide implementation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 86: severity=MEDIUM; category=missing_override_keyword
  Description: C++11: override keyword improves code clarity and catches errors
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 88: severity=MEDIUM; category=missing_override_keyword
  Description: C++11: override keyword improves code clarity and catches errors
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::virtual_oop
- Line 389: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: to_free.push_back(*ptr);
- Line 610: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: to_free.push_back(*ptr);
- Line 793: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: to_free.push_back(*ptr);

### storage/backup_manager.cpp
Total findings: 38

- Line 1747: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats->rto_seconds = static_cast<uint32_t>(
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4746 Add Q2 2026 Waveâ€‘1 qualit... (2026-04-21) | #3810 feat(storage): Impl
- Line 335: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(src_dir)) {
- Line 738: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(backup_dir)) {
- Line 789: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(checkpoint_dir)) {
- Line 878: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(raid_topology_dir)) {
- Line 982: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: pid_t pid = fork();
- Line 985: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "fork() failed when invoking tar");
- Line 998: severity=HIGH; category=command_injection
  Description: command_injection_exec: Command execution — validate arguments and use safer alternatives
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: execvp("tar", const_cast<char* const*>(argv));
- Line 1022: severity=HIGH; category=windows_only_api
  Description: Windows-only API WaitForSingleObject without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: WaitForSingleObject(pi.hProcess, INFINITE);
- Line 1061: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: pid_t pid = fork();
- Line 1064: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "fork() failed when invoking tar");
- Line 1073: severity=HIGH; category=command_injection
  Description: command_injection_exec: Command execution — validate arguments and use safer alternatives
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: execvp("tar", const_cast<char* const*>(argv));
- Line 1094: severity=HIGH; category=windows_only_api
  Description: Windows-only API WaitForSingleObject without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: WaitForSingleObject(pi.hProcess, INFINITE);
- Line 1140: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::recursive_directory_iterator(src_root, ec)) {
- Line 1266: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::recursive_directory_iterator(src_root, ec)) {
- Line 1439: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1557: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1672: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(backup_dir)) {
- Line 1754: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::recursive_directory_iterator(checkpoint_dir)) {
- Line 2133: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::recursive_directory_iterator(backup_path)) {
- Line 2171: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::recursive_directory_iterator(backup_path)) {
- Line 2556: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(snapshot_dir)) {
- Line 2587: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(snap_base, ec)) {
- Line 52: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: static std::string winQuoteForCreateProcess(const std::string& s) {
- Line 747: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Sort by timestamp (filename format ensures correct sort order)
- Line 899: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: Result<void> BackupManager::isBackupComplete(const std::string& backup_dir,
- Line 1011: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string cmd = "tar -czf " + winQuoteForCreateProcess(compressed_file) +
- Line 1016: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: mutable_cmd.push_back('\0');
- Line 1083: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string cmd = "tar -xzf " + winQuoteForCreateProcess(compressed_file) +
- Line 1088: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: mutable_cmd.push_back('\0');
- Line 1377: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Backup Compression.' that was not found in 'src/storage/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/storage/FUTURE_ENHANCEMENTS.md §Backup Compression.
- Line 1650: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Cloud Backup Download.' that was not found in 'src/storage/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/storage/FUTURE_ENHANCEMENTS.md §Cloud Backup Download.
- Line 1912: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: coll_list_capacity += (collections.size() - 1) * 2; // ", "
- Line 1949: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> target_cfs;
- Line 2591: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(entry.path().string());
- Line 2595: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::sort(result.begin(), result.end()); // alphabetical = chronological (timestamps in name)
- Line 1933: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: //    puts.  This avoids overwriting CFs outside the requested scope.

### storage/nvme_manager.cpp
Total findings: 35

- Line 229: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: int dfd = ::open(probe_path, O_WRONLY | O_DIRECT | O_CLOEXEC, 0600);
- Line 296: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring_ && ring_->ring_fd >= 0;
- Line 486: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: THEMIS_ERROR("NVMeManager::resetZone: open('{}') failed: {}",
- Line 515: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: THEMIS_ERROR("NVMeManager::finishZone: open('{}') failed: {}",
- Line 642: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->ring_fd = themis_io_uring_setup(config_.io_uring_queue_depth, &params);
- Line 651: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->sq_mmap_size = params.sq_off.array +
- Line 653: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->sq_mmap = ::mmap(nullptr, ring->sq_mmap_size,
- Line 661: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->ring_fd = -1;
- Line 665: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto* sq_base = static_cast<uint8_t*>(ring->sq_mmap);
- Line 668: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->sq_mask  = reinterpret_cast<uint32_t*>(sq_base + params.sq_off.ring_mask);
- Line 672: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->sqe_mmap_size = params.sq_entries * sizeof(struct io_uring_sqe);
- Line 673: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->sqe_mmap = ::mmap(nullptr, ring->sqe_mmap_size,
- Line 683: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->ring_fd = -1;
- Line 686: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->sqes = static_cast<struct io_uring_sqe*>(ring->sqe_mmap);
- Line 690: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->cq_mmap_size = params.cq_off.cqes +
- Line 692: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->cq_mmap = ::mmap(nullptr, ring->cq_mmap_size,
- Line 704: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->ring_fd = -1;
- Line 707: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto* cq_base = static_cast<uint8_t*>(ring->cq_mmap);
- Line 710: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->cq_mask = reinterpret_cast<uint32_t*>(cq_base + params.cq_off.ring_mask);
- Line 711: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->cqes    = reinterpret_cast<struct io_uring_cqe*>(cq_base + params.cq_off.cqes);
- Line 741: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (ring->ring_fd >= 0) {
- Line 743: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ring->ring_fd = -1;
- Line 131: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (initialized_.load(std::memory_order_acquire)) {
- Line 141: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (initialized_.load(std::memory_order_acquire)) {
- Line 237: severity=HIGH; category=posix_only_api
  Description: POSIX-only API unlink( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: ::unlink(probe_path);  // Always clean up, after the O_DIRECT test
- Line 294: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return initialized_.load(std::memory_order_acquire) &&
- Line 335: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: sqe->len       = static_cast<uint32_t>(req.len);

            sqe->user_data = static_cast<uint64_t>(req.user_data);

            // SQE write is visible to kernel via sq_array update and tail store

            __atomic_store_n(&ring->sq_array[index], index, __ATOMIC_RELEASE);

            __atomic_store_n(ring->sq_tail, tail + 1, __ATOMIC_RELEASE);

            int ret = themis_io_uring_enter(ring->ring_fd, 1, 0,

                                            IORING_ENTER_GETEVENTS, nullptr);
- Line 396: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: sqe->len       = static_cast<uint32_t>(req.len);

            sqe->user_data = static_cast<uint64_t>(req.user_data);

            // SQE write is visible to kernel via sq_array update and tail store

            __atomic_store_n(&ring->sq_array[index], index, __ATOMIC_RELEASE);

            __atomic_store_n(ring->sq_tail, tail + 1, __ATOMIC_RELEASE);

            int ret = themis_io_uring_enter(ring->ring_fd, 1, 0,

                                            IORING_ENTER_GETEVENTS, nullptr);
- Line 549: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    // Allocate a report buffer for 1 zone', '    constexpr size_t BUF_SIZE = sizeof(struct blk_zone_report) + sizeof(struct blk_zone);', '    alignas(alignof(struct blk_zone_report)) char buf[BUF_SIZE];', '    std::memset(buf, 0, BUF_SIZE);', '']
- Line 225: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(tmp_fd);
- Line 232: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(dfd);
- Line 557: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 601: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd);
- Line 621: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        if (f.is_open()) {', '            uint32_t count = 1;', '            f >> count;', '            if (count > 0) return count;', '        }']
- Line 742: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(ring->ring_fd);

### storage/wom_tree.cpp
Total findings: 35

- Line 440: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (node_ref->children.size() <= static_cast<size_t>(config.fanout)) {
- Line 249: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stat_user_bytes.fetch_add(op.byteSize(), std::memory_order_relaxed);
- Line 324: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        stat_flush_passes.fetch_add(1, std::memory_order_relaxed);', '', '        uint32_t next_depth = depth + 1;', '', '        // Apply ops to each original child.']
- Line 339: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            if (child.is_leaf) {', '                // Apply ops directly to leaf.', '                for (auto& op : child_ops[orig_idx]) {', '                    stat_internal_bytes.fetch_add(op.byteSize(),', '                                                  std::memory_order_relaxed);']
- Line 360: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            } else {', "                // Push ops into child's buffer.", '                for (auto& op : child_ops[orig_idx]) {', '                    stat_internal_bytes.fetch_add(op.byteSize(),', '                                                  std::memory_order_relaxed);']
- Line 394: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 396: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 397: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 398: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 399: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 427: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (node_ref->is_leaf) return false;
- Line 430: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (size_t i = 0; i < node_ref->children.size(); ++i) {
- Line 431: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (doOneInternalSplit(node_ref->children[i], node_ref.get(), i)) {
- Line 440: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (node_ref->children.size() <= static_cast<size_t>(config.fanout)) {
- Line 450: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 451: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 452: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 453: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 526: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: while (!node->is_leaf) {
- Line 527: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: size_t ci = node->childIndex(key);
- Line 528: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node = node->children[ci].get();
- Line 555: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: while (!node->is_leaf) {
- Line 557: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (auto it = node->buffer.rbegin(); it != node->buffer.rend(); ++it) {
- Line 563: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: size_t ci = node->childIndex(key);
- Line 564: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: node = node->children[ci].get();
- Line 568: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto leaf_it = node->leafFind(key);
- Line 581: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (leaf_it != node->data.end()) {
- Line 90: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 100: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 596: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: void collectAllEntries(std::map<std::string, std::string>& out) const {
- Line 601: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string>& out) const {
- Line 228: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::atomic<uint64_t> stat_puts{0};
- Line 709: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: impl_->stat_puts.fetch_add(1, std::memory_order_relaxed);
- Line 844: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: impl_->stat_puts.store(0, std::memory_order_relaxed);
- Line 859: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: s.total_puts          = impl_->stat_puts.load(std::memory_order_relaxed);

### storage/wal_storage.cpp
Total findings: 26

- Line 179: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: buf[0] = static_cast<uint8_t>(v);
- Line 179: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // array_bounds scanner alert: the raw-pointer overloads are safe — every

// call site passes a correctly-sized local array.  The scanner cannot infer

// the pointed-to size; the array-reference overloads below enforce this

// statically where possible.

static void encode_u32(uint8_t* buf, uint32_t v) {

    buf[0] = static_cast<uint8_t>(v);

    buf[1] = static_cast<uint8_t>(v >> 8);

    buf[2] = static_cast<uint8_t>(v >> 16);

    buf[3] = static_cast<uint8_t>(v >> 24);

}
- Line 180: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: buf[1] = static_cast<uint8_t>(v >> 8);
- Line 180: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // call site passes a correctly-sized local array.  The scanner cannot infer

// the pointed-to size; the array-reference overloads below enforce this

// statically where possible.

static void encode_u32(uint8_t* buf, uint32_t v) {

    buf[0] = static_cast<uint8_t>(v);

    buf[1] = static_cast<uint8_t>(v >> 8);

    buf[2] = static_cast<uint8_t>(v >> 16);

    buf[3] = static_cast<uint8_t>(v >> 24);

}



static void encode_u64(uint8_t* buf, uint64_t v) {
- Line 181: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 2
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: buf[2] = static_cast<uint8_t>(v >> 16);
- Line 181: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 2
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // the pointed-to size; the array-reference overloads below enforce this

// statically where possible.

static void encode_u32(uint8_t* buf, uint32_t v) {

    buf[0] = static_cast<uint8_t>(v);

    buf[1] = static_cast<uint8_t>(v >> 8);

    buf[2] = static_cast<uint8_t>(v >> 16);

    buf[3] = static_cast<uint8_t>(v >> 24);

}



static void encode_u64(uint8_t* buf, uint64_t v) {

    for (int i = 0; i < 8; ++i) {
- Line 182: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 3
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: buf[3] = static_cast<uint8_t>(v >> 24);
- Line 182: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 3
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // statically where possible.

static void encode_u32(uint8_t* buf, uint32_t v) {

    buf[0] = static_cast<uint8_t>(v);

    buf[1] = static_cast<uint8_t>(v >> 8);

    buf[2] = static_cast<uint8_t>(v >> 16);

    buf[3] = static_cast<uint8_t>(v >> 24);

}



static void encode_u64(uint8_t* buf, uint64_t v) {

    for (int i = 0; i < 8; ++i) {

        buf[i] = static_cast<uint8_t>(v >> (8 * i));
- Line 192: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: return static_cast<uint32_t>(buf[0])
- Line 192: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: buf[i] = static_cast<uint8_t>(v >> (8 * i));

    }

}



static uint32_t decode_u32(const uint8_t* buf) {

    return static_cast<uint32_t>(buf[0])

         | (static_cast<uint32_t>(buf[1]) << 8)

         | (static_cast<uint32_t>(buf[2]) << 16)

         | (static_cast<uint32_t>(buf[3]) << 24);

}
- Line 193: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: | (static_cast<uint32_t>(buf[1]) << 8)
- Line 193: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 1
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

}



static uint32_t decode_u32(const uint8_t* buf) {

    return static_cast<uint32_t>(buf[0])

         | (static_cast<uint32_t>(buf[1]) << 8)

         | (static_cast<uint32_t>(buf[2]) << 16)

         | (static_cast<uint32_t>(buf[3]) << 24);

}



static uint64_t decode_u64(const uint8_t* buf) {
- Line 194: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 2
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: | (static_cast<uint32_t>(buf[2]) << 16)
- Line 194: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 2
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



static uint32_t decode_u32(const uint8_t* buf) {

    return static_cast<uint32_t>(buf[0])

         | (static_cast<uint32_t>(buf[1]) << 8)

         | (static_cast<uint32_t>(buf[2]) << 16)

         | (static_cast<uint32_t>(buf[3]) << 24);

}



static uint64_t decode_u64(const uint8_t* buf) {

    uint64_t v = 0;
- Line 195: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 3
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: | (static_cast<uint32_t>(buf[3]) << 24);
- Line 195: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 3
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: static uint32_t decode_u32(const uint8_t* buf) {

    return static_cast<uint32_t>(buf[0])

         | (static_cast<uint32_t>(buf[1]) << 8)

         | (static_cast<uint32_t>(buf[2]) << 16)

         | (static_cast<uint32_t>(buf[3]) << 24);

}



static uint64_t decode_u64(const uint8_t* buf) {

    uint64_t v = 0;

    for (int i = 0; i < 8; ++i) {
- Line 427: severity=CRITICAL; category=missing_dtor
  Description: Class stat allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct stat
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4596 perf(storage): fix ~79x sus... (2026-04-13) | #4236 feat(storage): Zero
- Line 49: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: static int themis_open_fd(const char* path, int flags, int mode) { return _open(path, flags, mode);
- Line 75: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: int fd = ::open(path, flags | O_CLOEXEC, mode);
- Line 274: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(config_.dir)) {
- Line 459: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 492: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 50: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: static int themis_close_fd(int fd) { return _close(fd); }
- Line 82: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: static int themis_close_fd(int fd) { return ::close(fd); }
- Line 152: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int k = 0; k < 8; ++k) {

### storage/database_connection_manager.cpp
Total findings: 25

- Line 98: severity=CRITICAL; category=double_lock
  Description: Double lock without unlock (potential deadlock)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 117: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::info("Created new connection (total: {})", total + 1);
- Line 117: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: updateCircuitBreaker(true);

                

                spdlog::info("Created new connection (total: {})", total + 1);

                return conn;

            } else {

                updateCircuitBreaker(false);
- Line 117: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::info("Created new connection (total: {})", total + 1);
- Line 579: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: keepalive_thread_.join();
- Line 58: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::warn("Circuit breaker open - cannot acquire connection");
- Line 62: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 91: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 98: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: // no_timeout scanner alert: lock.lock() is a deliberate
- Line 101: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 104: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 117: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 136: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 237: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    // performed on the raw pointer value itself — false positive.', '    for (auto& [ptr, conn] : active_connections_) {', '        auto& health = connection_health_[ptr];', '        if (conn->isValid()) {', '            health.last_health_check = std::chrono::system_clock::now();']
- Line 269: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& [ptr, health] : connection_health_) {
- Line 290: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: DatabaseConnectionManager::getConnectionHealth() const {
- Line 330: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    // Active connections will be reconnected when released', '    for (auto& [ptr, conn] : active_connections_) {', '        auto& health = connection_health_[ptr];', '        health.state = ConnectionState::RECONNECTING;', '    }']
- Line 373: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 86: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: conn->close();
- Line 186: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: conn->close();
- Line 214: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: conn->close();
- Line 231: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // over std::unordered_map — the map outlives the loop and no temporary is
- Line 325: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: conn->close();
- Line 344: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: conn->close();
- Line 349: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: conn->close();

### storage/tensor_train_decomposer.cpp
Total findings: 25

- Line 184: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: if (pos + 8 > bytes.size()) throw std::runtime_error("TTTrain::deserialize: underflow");
- Line 112: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 113: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 114: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 116: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 118: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 126: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 804: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 844: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 847: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 848: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 851: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 852: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 853: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 867: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 870: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 871: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 872: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 904: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 905: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 908: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 916: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 918: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 922: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 923: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### storage/distributed_transaction_manager.cpp
Total findings: 23

- Line 99: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: void DistributedTransaction::put(std::string_view key, std::string_view value) {
- Line 102: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: "DistributedTransaction [" + txn_id_ + "]: put() called on non-ACTIVE transaction"
- Line 84: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "DistributedTransaction [" + txn_id_ + "]: unknown shard '" + shard_id + "'"
- Line 90: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "DistributedTransaction [" + txn_id_ + "]: missing shard version for '" + shard_id + "'"
- Line 102: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "DistributedTransaction [" + txn_id_ + "]: put() called on non-ACTIVE transaction"
- Line 131: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "DistributedTransaction [" + txn_id_ + "]: del() called on non-ACTIVE transaction"
- Line 158: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: std::optional<std::string> DistributedTransaction::get(std::string_view key) {
- Line 174: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: return participant->get(logical_key);
- Line 190: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_WARN("DistributedTransaction [{}]: commit() called in unexpected state", txn_id_);
- Line 195: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_DEBUG("DistributedTransaction [{}]: Phase 1 — PREPARE to {} shard(s)",
- Line 211: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lk(mgr_state_->shards_mutex);
- Line 214: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' no longer registered during prepare",
- Line 225: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "DistributedTransaction [{}]: shard '{}' registration version changed during prepare "
- Line 241: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' prepare threw: {}",
- Line 258: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_DEBUG("DistributedTransaction [{}]: Phase 2 — COMMIT to {} shard(s)",
- Line 269: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' commit threw: {}",
- Line 275: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("DistributedTransaction [{}]: COMMITTED across {} shard(s)",
- Line 287: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort threw: {}",
- Line 293: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("DistributedTransaction [{}]: ABORTED (prepare phase failed)", txn_id_);
- Line 314: severity=HIGH; category=missing_trace_point
  Description: Critical function abort without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort (rollback) threw: {}",
- Line 314: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort (rollback) threw: {}",
- Line 319: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: THEMIS_INFO("DistributedTransaction [{}]: rolled back", txn_id_);
- Line 212: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: shard_id
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = mgr_state_->shards.find(shard_id);

### storage/storage_audit_logger.cpp
Total findings: 21

- Line 48: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: static int themis_open_fd(const char* path, int flags, int mode) { return _open(path, flags, mode);
- Line 58: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: static int themis_open_fd(const char* path, int flags, int mode) { return ::open(path, flags, mode);
- Line 156: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(config_.dir)) {
- Line 166: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 167: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 168: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 169: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 172: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 270: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 271: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 272: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 275: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 49: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: static int themis_close_fd(int fd) { return _close(fd); }
- Line 59: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: static int themis_close_fd(int fd) { return ::close(fd); }
- Line 186: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: Result<void> StorageAuditLogger::log(Event event,
- Line 195: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return log(Event::PUT, key, extra);
- Line 200: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return log(Event::DEL, key, extra);
- Line 204: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return log(Event::CHECKPOINT, "", detail);
- Line 208: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return log(Event::RECOVERY, "", detail);
- Line 212: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return log(Event::COMPACTION, "", detail);
- Line 216: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: return log(Event::SNAPSHOT, "", detail);

### storage/erasure_coder_factory.cpp
Total findings: 20

- Line 112: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        if (offset < data.size()) {', '            const size_t size = std::min(chunk_size, data.size() - offset);', '            std::memcpy(chunk.data(), data.data() + offset, size);', '        }', '        chunks.push_back(std::move(chunk));']
- Line 429: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        if (offset < data.size()) {', '            const size_t size = std::min(chunk_size, data.size() - offset);', '            std::memcpy(chunk.data(), data.data() + offset, size);', '        }', '        chunks.push_back(std::move(chunk));']
- Line 103: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    // no concrete source location is identified, and the chunk copy is guarded by', '    // offset/data.size checks with bounded std::min for memcpy length.', '    const size_t chunk_size = (data.size() + data_shards - 1) / data_shards;', '    std::vector<std::vector<uint8_t>> chunks;', '    chunks.reserve(data_shards + parity_shards);']
- Line 195: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



    if (!invertMatrix(decode_matrix)) {

        throw std::runtime_error("Failed to invert decode matrix for Reed-Solomon recovery");

    }



    const size_t chunk_size = available_chunks.begin()->second.size();
- Line 210: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<uint8_t> recovered_bytes;

        gf_matrix_mul(decode_matrix, available_bytes, recovered_bytes);

        for (size_t row = 0; row < data_shards; ++row) {

            recovered_data[row][byte] = recovered_bytes[row];

        }

    }
- Line 420: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    uint32_t parity_shards', ') {', '    const size_t chunk_size = (data.size() + data_shards - 1) / data_shards;', '    std::vector<std::vector<uint8_t>> chunks;', '    chunks.reserve(data_shards + parity_shards);']
- Line 516: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



    if (!invertMatrix(decode_matrix)) {

        throw std::runtime_error("Failed to invert decode matrix");

    }



    std::vector<std::vector<uint8_t>> recovered_data(data_shards,
- Line 530: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<uint8_t> recovered_bytes;

        gf_matrix_mul(decode_matrix, available_bytes, recovered_bytes);

        for (size_t row = 0; row < data_shards; ++row) {

            recovered_data[row][byte] = recovered_bytes[row];

        }

    }
- Line 10: severity=MEDIUM; category=coupling_risk_sharding_storage
  Description: Potential coupling risk between sharding/ and storage/ (validate no circular dependency)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_architecture_rules
  Context: #include "sharding/redundancy_strategy.h"
- Line 43: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
- Line 136: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
- Line 222: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
- Line 238: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_inv(uint8_t a)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t ReedSolomonCoder::gf_inv(uint8_t a) {
- Line 253: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_div(uint8_t a, uint8_t b)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t ReedSolomonCoder::gf_div(uint8_t a, uint8_t b) {
- Line 257: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp) {
- Line 281: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
- Line 297: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_inv(uint8_t a)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: uint8_t CauchyReedSolomonCoder::gf_inv(uint8_t a) {
- Line 358: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: bool CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
- Line 451: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
- Line 542: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ErasureCoder::create(ErasureCodingAlgorithm algorithm)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: std::unique_ptr<ErasureCoder> ErasureCoder::create(ErasureCodingAlgorithm algorithm) {

### storage/hlc.cpp
Total findings: 17

- Line 61: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint64_t cur = state_.load(std::memory_order_relaxed);
- Line 66: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 69: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 72: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 74: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 75: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 103: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 107: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 109: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 110: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 114: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 115: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 117: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 118: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 123: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 138: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return HLCTimestamp{state_.load(std::memory_order_acquire)};
- Line 90: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: HybridLogicalClock::now()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: HLCTimestamp HybridLogicalClock::now() {

### storage/blob_backend_azure.cpp
Total findings: 16

- Line 106: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #746 [Phase 4] Storage Layer: Mi... (2026-03-11)
- Line 86: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Get container client

            container_client_ = std::make_unique<Azure::Storage::Blobs::BlobContainerClient>(

                service_client.GetBlobContainerClient(container_name_)

            );

            

            // Ensure container exists
- Line 130: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: Azure::Storage::Blobs::UploadBlockBlobOptions options;

            options.HttpHeaders.ContentType = "application/octet-stream";

            

            auto response = blob_client.Upload(stream, options);

            

            // Create blob reference

            BlobRef ref;
- Line 171: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob_client = container_client_->GetBlockBlobClient(blob_name);

            

            // Download blob

            auto response = blob_client.Download();

            

            // Read data from stream

            std::vector<uint8_t> data;
- Line 219: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("Azure delete failed: {}", reason);
- Line 219: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::lock_guard<std::mutex> lock(mutex_);

        if (!container_client_) {

            const std::string reason = init_error_.empty() ? "Azure client not initialized" : init_error_;

            THEMIS_ERROR("Azure delete failed: {}", reason);

            return Err<void>(

                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,

                "Azure backend unavailable: " + reason
- Line 219: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("Azure delete failed: {}", reason);
- Line 233: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob_client = container_client_->GetBlockBlobClient(blob_name);

            

            // Delete blob

            blob_client.Delete();

            

            THEMIS_DEBUG("Blob deleted from Azure: id={}", ref.id);

            return OkVoid();
- Line 239: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("Azure delete failed: {}", e.what());
- Line 239: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return OkVoid();

            

        } catch (const Azure::Core::RequestFailedException& e) {

            THEMIS_ERROR("Azure delete failed: {}", e.what());

            return Err<void>(

                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,

                "Azure delete failed: " + std::string(e.what())
- Line 239: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("Azure delete failed: {}", e.what());
- Line 242: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Azure delete failed: " + std::string(e.what())
- Line 242: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: THEMIS_ERROR("Azure delete failed: {}", e.what());

            return Err<void>(

                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,

                "Azure delete failed: " + std::string(e.what())

            );

        }

    }
- Line 242: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Azure delete failed: " + std::string(e.what())
- Line 262: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto blob_client = container_client_->GetBlockBlobClient(blob_name);

            

            // Check existence

            auto properties = blob_client.GetProperties();

            return true;

            

        } catch (const Azure::Core::RequestFailedException& e) {

### storage/tensor_compaction_filter.cpp
Total findings: 15

- Line 115: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // enforces block checksums; TTTrain::deserialize returns nullopt on
- Line 117: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Deserialize raw TTTrain
- Line 123: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto opt = TTTrain::deserialize(bytes);
- Line 150: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // checksums guard integrity; QuantizedTrain::deserialize validates header
- Line 152: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Deserialize QuantizedTrain header
- Line 158: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto opt = QuantizedTrain::deserialize(bytes);
- Line 142: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 143: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 192: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 196: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 197: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 217: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 218: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 222: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 223: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### storage/concurrent_write_controller.cpp
Total findings: 13

- Line 119: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // acquire (blocking, FIFO)

// ─────────────────────────────────────────────────────────────────────────────



WriteGuard ConcurrentWriteController::acquire() {

    const auto start = std::chrono::steady_clock::now();



    // db_connection_leak scanner alerts on the atomic counter operations in this
- Line 119: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: WriteGuard ConcurrentWriteController::acquire() {
- Line 131: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (shutdown_) {

        total_rejected_.fetch_add(1, std::memory_order_relaxed);

        throw std::runtime_error(

            "ConcurrentWriteController: acquire() called after shutdown");

    }



    // Fast path: a slot is available immediately.
- Line 131: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: "ConcurrentWriteController: acquire() called after shutdown");
- Line 41: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: WriteGuard::~WriteGuard() {
- Line 82: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: , acquire_timeout_(config.acquire_timeout) {
- Line 119: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: WriteGuard ConcurrentWriteController::acquire() {
- Line 131: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: "ConcurrentWriteController: acquire() called after shutdown");
- Line 139: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: total_acquired_.fetch_add(1, std::memory_order_relaxed);
- Line 160: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (acquire_timeout_.count() > 0) {
- Line 164: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: got_slot = (f.wait_for(acquire_timeout_) == std::future_status::ready);
- Line 272: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 307: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: s.total_acquired = total_acquired_.load(std::memory_order_relaxed);

### storage/history_manager.cpp
Total findings: 13

- Line 267: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return deserializeHistoryRecord(it.value());
- Line 274: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto rec = deserializeHistoryRecord(val);
- Line 377: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return deserializeConflictRecord(
- Line 385: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto rec = deserializeConflictRecord(val);
- Line 445: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return deserializeConflictSet(
- Line 453: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto set = deserializeConflictSet(val);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #2766 [storage/transaction] Atomi... (2026-03-11)
- Line 56: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // not in the new framing format (legacy path: no CRC trailer).
- Line 60: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Too short for CRC trailer — treat as legacy (no checksum).
- Line 69: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // CRC mismatch — either corrupted data or a legacy record without
- Line 242: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 96: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto hi = hex[i];
- Line 97: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto lo = hex[i + 1];

### storage/merge_operators.cpp
Total findings: 13

- Line 98: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    }



    // Parse and add new values (comma-separated)

    std::string value_str(value.data(), value.size());

    std::stringstream ss(value_str);

    std::string item;
- Line 30: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 52: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 54: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 68: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 69: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 70: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 98: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 109: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 115: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 128: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 129: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 153: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### storage/tiered_storage.cpp
Total findings: 13

- Line 421: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: worker_thread_.join();
- Line 195: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("TieredStorage: failed to delete '{}': {}", path, ec.message());
- Line 291: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return false;

    }



    // Write to destination (copy-then-delete for crash safety)

    if (!writeToTier(key, value, to)) {

        THEMIS_ERROR("TieredStorage: migrateKey({}) failed to write to destination tier {}",

                     key, static_cast<int>(to));
- Line 291: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Write to destination (copy-then-delete for crash safety)
- Line 301: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Delete from source only after successful copy

    // delete_no_nullptr scanner alert (line 300): deleteFromTier() is a class method

    // that removes a file from a storage tier; it is not the delete operator and does

    // not dereference any raw pointer — false positive.

    if (!deleteFromTier(key, from)) {

        THEMIS_WARN("TieredStorage: migrateKey({}) copied but could not delete source", key);
- Line 301: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // that removes a file from a storage tier; it is not the delete operator and does
- Line 304: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_WARN("TieredStorage: migrateKey({}) copied but could not delete source", key);
- Line 304: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // that removes a file from a storage tier; it is not the delete operator and does

    // not dereference any raw pointer — false positive.

    if (!deleteFromTier(key, from)) {

        THEMIS_WARN("TieredStorage: migrateKey({}) copied but could not delete source", key);

        // Not a hard error – we have a valid copy at destination; clean up later

    }
- Line 304: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_WARN("TieredStorage: migrateKey({}) copied but could not delete source", key);
- Line 392: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock lock(worker_mutex_);
- Line 93: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: while (!trimmed.empty() && (trimmed.front() == '/' || trimmed.front() == '\\')) {
- Line 103: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: safe += (c == '/' || c == '\\' || c == ':' || c == '*' ||
- Line 104: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: safe += (c == '/' || c == '\\' || c == ':' || c == '*' ||

### storage/blob_backend_s3.cpp
Total findings: 12

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4227 feat(ingestion): S3-Compati... (2026-03-14) | #746 [Phase 4] Storage La
- Line 130: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // transparently retries transient errors — false positives.

        auto input_stream = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");

        input_stream->write(reinterpret_cast<const char*>(data.data()), data.size());

        request.SetBody(input_stream);

        request.SetContentLength(data.size());

        

        // Upload to S3
- Line 131: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto input_stream = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");

        input_stream->write(reinterpret_cast<const char*>(data.data()), data.size());

        request.SetBody(input_stream);

        request.SetContentLength(data.size());

        

        // Upload to S3

        auto outcome = client_->PutObject(request);
- Line 166: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Create GetObject request

        Aws::S3::Model::GetObjectRequest request;

        request.SetBucket(bucket_);

        request.SetKey(s3_key);

        

        // Download from S3
- Line 167: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Create GetObject request

        Aws::S3::Model::GetObjectRequest request;

        request.SetBucket(bucket_);

        request.SetKey(s3_key);

        

        // Download from S3

        auto outcome = client_->GetObject(request);
- Line 224: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Create DeleteObject request

        Aws::S3::Model::DeleteObjectRequest request;

        request.SetBucket(bucket_);

        request.SetKey(s3_key);

        

        // Delete from S3
- Line 225: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Create DeleteObject request

        Aws::S3::Model::DeleteObjectRequest request;

        request.SetBucket(bucket_);

        request.SetKey(s3_key);

        

        // Delete from S3

        auto outcome = client_->DeleteObject(request);
- Line 236: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "S3 delete failed: " + error.GetMessage()
- Line 236: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: error.GetExceptionName(), error.GetMessage());

            return Err<void>(

                errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,

                "S3 delete failed: " + error.GetMessage()

            );

        }
- Line 236: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "S3 delete failed: " + error.GetMessage()
- Line 251: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Create HeadObject request

        Aws::S3::Model::HeadObjectRequest request;

        request.SetBucket(bucket_);

        request.SetKey(s3_key);

        

        // Check existence
- Line 252: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Create HeadObject request

        Aws::S3::Model::HeadObjectRequest request;

        request.SetBucket(bucket_);

        request.SetKey(s3_key);

        

        // Check existence

        auto outcome = client_->HeadObject(request);

### storage/adaptive_compaction.cpp
Total findings: 11

- Line 75: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: sample_thread_.join();
- Line 53: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 87: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(sample_mutex_);
- Line 88: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: // lock_in_loop scanner alert (line 90): cv::wait_for() semantics require
- Line 106: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint64_t reads  = window_reads_.exchange(0, std::memory_order_relaxed);
- Line 107: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint64_t writes = window_writes_.exchange(0, std::memory_order_relaxed);
- Line 262: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 263: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 264: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 265: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 161: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ? std::log(current_write_amp) / std::log(config_.urgent_write_amp_threshold)

### storage/blob_redundancy_manager.cpp
Total findings: 11

- Line 426: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: maintenance_thread_.join();
- Line 429: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: repair_thread_.join();
- Line 432: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: config_reload_thread_.join();
- Line 1471: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    if (size < 1024 * 1024) {', '        return BlobType::BLOB_SMALL;', '    } else if (size < 100 * 1024 * 1024) {', '        return BlobType::BLOB_MEDIUM;', '    } else {']
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4336 docs(storage): correct SECU... (2026-03-19) | #4201 feat(base): async r
- Line 1235: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(repair_mutex_);
- Line 1314: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // files that have been superseded by new ones, and in those cases the
- Line 1380: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(repair_mutex_);
- Line 1383: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: repair_cv_.wait_for(lock, std::chrono::seconds(5), [this]() {
- Line 778: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> healthy_dcs;
- Line 1197: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> healthy_dcs;

### storage/security_signature_manager.cpp
Total findings: 11

- Line 137: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // hardcoded_output scanner alert: snprintf writes to a local stack buffer
- Line 141: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(&hex_output[i * 2], 3, "%02x", static_cast<unsigned int>(digest[i]));
- Line 51: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        return db_->put(key, value);

    } catch (...) {

        return false;

    }

}
- Line 51: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 98: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: signatures.push_back(*sig);
- Line 111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: signatures.push_back(*sig);
- Line 141: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: snprintf(&hex_output[i * 2], 3, "%02x", static_cast<unsigned int>(digest[i]));
- Line 146: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: hex_output[SHA256_DIGEST_LENGTH * 2] = '\0';

        

        return std::string(hex_output);

    } catch (...) {

        return "";

    }

}
- Line 146: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 169: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        

        return normalized;

    } catch (...) {

        return path; // Return original if normalization fails

    }

}
- Line 169: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### storage/tensor_network_storage_engine.cpp
Total findings: 11

- Line 244: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // QuantizedTrain::deserialize validates header size and returns nullopt on
- Line 246: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return QuantizedTrain::deserialize(*meta);
- Line 279: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
- Line 362: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::shared_mutex> wlk(rw_mutex_);
- Line 378: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: eraseVersion(key);

    wlk.unlock();



    // Notify delete observer outside the write lock.

    {

        std::lock_guard<std::mutex> olk(observer_mutex_);

        if (delete_observer_) {
- Line 378: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Notify delete observer outside the write lock.
- Line 35: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: observer(std::forward<Args>(args)...);

    } catch (const std::exception& ex) {

        THEMIS_WARN("{} observer callback failed: {}", observer_name, ex.what());

    } catch (...) {

        THEMIS_WARN("{} observer callback failed with non-std exception", observer_name);

    }

}
- Line 35: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 48: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        return std::stoull(key.substr(colon + 1));

    } catch (...) {

        return std::nullopt;

    }

}
- Line 48: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 110: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(kv.first);

### storage/zero_copy_blob_transfer.cpp
Total findings: 11

- Line 136: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: int fd = ::open(path, O_RDONLY | O_CLOEXEC);
- Line 152: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: return ::write(fd, data, len / 2);
- Line 154: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: themis_zc_ssize_t rc = ::write(fd, data, len);
- Line 239: severity=CRITICAL; category=missing_dtor
  Description: Class stat allocates resources but has no destructor
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: class/struct stat
- Line 286: severity=CRITICAL; category=unwrapped_resource
  Description: Raw pointer allocated without RAII wrapper
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: auto* heap_buf = new uint8_t[size_];
- Line 136: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: int fd = ::open(path, O_RDONLY | O_CLOEXEC);
- Line 286: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 430: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    int64_t file_size = file_size_result.value();', '    if (length == 0) {', '        length = file_size - offset;', '    }', '    if (length <= 0 || offset < 0 || offset >= file_size ||']
- Line 513: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    int64_t file_size = file_size_result.value();', '    if (length == 0) {', '        length = file_size - offset;', '    }', '    if (offset < 0 || offset >= file_size || length <= 0 ||']
- Line 255: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: fd_      = mapped_fd.release();
- Line 311: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ::close(fd_);

### storage/online_schema_migration.cpp
Total findings: 10

- Line 89: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 104: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 183: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(tables.begin(), tables.end(), op.table_name) == tables.end()) {
- Line 391: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 392: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 395: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 396: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 403: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 432: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 442: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### storage/pitr_manager.cpp
Total findings: 10

- Line 110: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto snapshot = snapshot_mgr_->getTag(tag_name);
- Line 218: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto snapshot = snapshot_mgr_->getTag(tag_name);
- Line 169: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // repeated_search scanner alerts (lines 167, 289): std::find on options.tables
- Line 174: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::find(options.tables.begin(), options.tables.end(), table) != options.tables.end()) {
- Line 296: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(options.tables.begin(), options.tables.end(), table) == options.tables.end()) {
- Line 345: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: case Changefeed::ChangeEventType::EVENT_PUT:

            // PUT → DELETE (remove the key)

            // delete_no_nullptr scanner alert (line 338): db_->del() is a method call on

            // RocksDBWrapper, not the delete operator; there is no raw pointer being

            // deleted here — false positive.

            // RocksDBWrapper::del() returns bool - true on success

            if (!db_->del(event.key)) {
- Line 345: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // RocksDBWrapper, not the delete operator; there is no raw pointer being
- Line 349: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: return Status::Error("Failed to delete key: " + event.key);
- Line 349: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // deleted here — false positive.

            // RocksDBWrapper::del() returns bool - true on success

            if (!db_->del(event.key)) {

                return Status::Error("Failed to delete key: " + event.key);

            }

            break;
- Line 349: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: return Status::Error("Failed to delete key: " + event.key);

### storage/storage_engine.cpp
Total findings: 10

- Line 269: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: Result<void> StorageEngine::open(const std::string& db_path) {
- Line 24: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // They are provided for testing, development, and backward compatibility only.
- Line 26: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // legacy_duplication scanner alert: these backward-compat default
- Line 314: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint64_t cur = m.load(std::memory_order_relaxed);
- Line 315: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: while (v < cur && !m.compare_exchange_weak(cur, v, std::memory_order_relaxed))
- Line 319: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint64_t cur = m.load(std::memory_order_relaxed);
- Line 320: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: while (v > cur && !m.compare_exchange_weak(cur, v, std::memory_order_relaxed))
- Line 415: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "Failed to delete key: " + key);
- Line 415: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: io_del_errors_.fetch_add(1, std::memory_order_relaxed);

        span.setStatus(false, "RocksDB del failed");

        return ErrVoid(errors::ErrorCode::ERR_STORAGE_DISK_FULL,

                       "Failed to delete key: " + key);

    }

    io_del_ops_.fetch_add(1, std::memory_order_relaxed);

    io_del_latency_.fetch_add(us, std::memory_order_relaxed);
- Line 415: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "Failed to delete key: " + key);

### storage/compaction_manager.cpp
Total findings: 8

- Line 41: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 115: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bg_thread_.join();
- Line 131: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(bg_mutex_);
- Line 235: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // best-effort RocksDB stats parsing; non-parseable lines are

                    // silently skipped and stats remain at 0.  Narrowing to

                    // std::exception to reduce scan noise.

                    } catch (...) {}

                }

            }
- Line 235: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 191: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // writes are memtable flush outputs; L1+ writes are compaction outputs.
- Line 193: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // misidentified the phrase "flush outputs" / "compaction outputs" inside
- Line 194: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // this comment as a std::cout/printf call — this is comment text only,

### storage/compressed_storage.cpp
Total findings: 8

- Line 94: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: result.method = static_cast<compression::CompressionMethod>(bytes[0]);
- Line 94: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    CompressedValue result;



    // Read method

    result.method = static_cast<compression::CompressionMethod>(bytes[0]);



    // Read original size (little-endian)

    uint64_t size = 0;

    for (int i = 0; i < 8; ++i) {

        size |= static_cast<uint64_t>(bytes[1 + i]) << (i * 8);
- Line 150: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Deserialize
- Line 151: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto cv = CompressedValue::deserialize(*serialized);
- Line 231: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Deserialize
- Line 232: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto cv = CompressedValue::deserialize(*serialized);
- Line 77: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // verify it.  Legacy records (< 13 bytes or mismatching CRC) fall through.
- Line 88: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // If mismatch, treat as legacy (no CRC) and parse full bytes.

### storage/transaction_retry_manager.cpp
Total findings: 8

- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: BackoffStrategy strategy = policy ? policy->backoff_strategy : config_.backoff_strategy;
- Line 283: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 288: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 333: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 355: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 30: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: static constexpr double MAX_JITTER_FACTOR = 0.999;
- Line 371: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (callback) {

        try {

            callback(state, message);

        } catch (...) {

            // Ignore callback exceptions

        }

    }
- Line 371: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### storage/tt_quantizer.cpp
Total findings: 8

- Line 126: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // reaching this point — false positive at the deserializer level.
- Line 156: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // above; QuantizedCore::deserialize validates its own minimum size
- Line 158: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto oc = QuantizedCore::deserialize(cb);
- Line 178: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: float best_dist = std::abs(v - kNF4Table[0]);
- Line 178: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 16 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: uint8_t TTQuantizer::findNF4Index(float v) noexcept {

    // array_bounds scanner alert: kNF4Table has exactly 16 entries (indices

    // 0..15); the loop bound is < 16 — no out-of-bounds access; false positive.

    // Linear scan over the 16-entry NF4 lookup table

    uint8_t best = 0;

    float best_dist = std::abs(v - kNF4Table[0]);

    for (uint8_t i = 1; i < 16; ++i) {

        float d = std::abs(v - kNF4Table[i]);

        if (d < best_dist) { best_dist = d; best = i; }

    }

    return best;
- Line 100: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (int i = 0; i < 8; ++i) out.push_back((v >> (i*8)) & 0xFF);
- Line 256: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            qc.data[i / 2] = idx & 0x0F;', '        else', '            qc.data[i / 2] |= (idx << 4) & 0xF0;', '    }', '    return qc;']
- Line 284: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    for (std::size_t i = 0; i < nelems; ++i) {', '        uint8_t byte_val = qc.data[i / 2];', '        uint8_t idx = (i % 2 == 0) ? (byte_val & 0x0F) : ((byte_val >> 4) & 0x0F);', '        float nf4_val = kNF4Table[idx];', '        core.data[i] = nf4_val * qc.scale + qc.mean;']

### storage/base_entity.cpp
Total findings: 7

- Line 108: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: return field_cache_->find(std::string(field_name)) != field_cache_->end();
- Line 115: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto it = field_cache_->find(std::string(field_name));
- Line 116: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it != field_cache_->end()) {
- Line 262: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator last may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto last = token.find_last_not_of(" \t");
- Line 253: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy comma-separated fallback.
- Line 372: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vec.push_back(static_cast<float>(dres.value_unsafe()));
- Line 420: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vec.push_back(static_cast<float>(elem.get<double>()));

### storage/disk_space_monitor.cpp
Total findings: 7

- Line 94: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: monitor_thread_.join();
- Line 311: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 318: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 437: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 616: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return (stat(path.c_str(), &buffer) == 0);
- Line 104: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool should_send_alert = false;
- Line 621: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: size_t pos = path.find_last_of("/\\");

### storage/index_analyzer.cpp
Total findings: 7

- Line 313: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: scheduler_thread_.join();
- Line 348: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 354: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: // lock_in_loop scanner alert (line 355): cv_.wait_for() requires holding
- Line 363: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cv_.wait_for(lock, std::chrono::minutes(1),
- Line 375: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cv_.wait_for(lock, std::chrono::seconds(60),
- Line 450: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (raw_db->GetProperty("rocksdb.num-files-at-level0", &l0_str)) {

        try {

            l0_files = std::stoull(l0_str);

        } catch (...) {}

    }



    // Estimate fragmentation percentage from L0 file count and
- Line 450: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}

### storage/streaming_ingest_manager.cpp
Total findings: 7

- Line 51: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: new StreamingIngestManager(std::move(db), std::move(cfg)));
- Line 73: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: flush_thread_.join();
- Line 100: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: flush_thread_.join();
- Line 271: severity=CRITICAL; category=double_lock
  Description: Double lock without unlock (potential deadlock)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 213: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 214: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(mu_);
- Line 83: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool expected = false;

### storage/columnar_cache.cpp
Total findings: 6

- Line 108: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lit may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto lit = lru_map_.find(key);
- Line 143: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lit may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto lit = lru_map_.find(key);
- Line 31: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: case SegmentDType::Int64:  return n * sizeof(int64_t) + n;   // data + null bitmap
- Line 55: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: PinGuard::~PinGuard() noexcept {
- Line 168: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (on_evict_cb) evicted_keys.push_back(it->first);

### storage/erasure_coding_backend.cpp
Total findings: 6

- Line 177: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto recovered = coder_->decode(chunk_map, missing, k, m);
- Line 171: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (chunk_map.find(i) == chunk_map.end()) {
- Line 134: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<uint32_t, EncodedShard>& shards,
- Line 158: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<uint32_t, std::vector<uint8_t>> chunk_map;
- Line 241: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<uint32_t, EncodedShard> shard_map;
- Line 35: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // can handle invalid erasure-coding inputs explicitly.

### storage/index_maintenance.cpp
Total findings: 6

- Line 75: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: maintenance_thread_.join();
- Line 431: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 432: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cv_.wait_for(lock, std::chrono::milliseconds(policy_.time_based_interval_ms),
- Line 692: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 510: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (db->GetProperty("rocksdb.num-files-at-level0", &file_count_str)) {

            try {

                metrics.file_count = std::stoull(file_count_str);

            } catch (...) {}

        }



        // ── SST size ratio (wasted space) ────────────────────────────────────
- Line 510: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}

### storage/key_schema.cpp
Total findings: 6

- Line 123: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // backward-compatibility path for pre-prefix keys written by older versions;
- Line 125: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Fallback for legacy keys without prefixes
- Line 126: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Assume DOCUMENT for backward compatibility (was more common in early versions)
- Line 137: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // legacy_duplication scanner alert: same backward-compatibility rationale
- Line 138: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // as above — the separator-less path handles pre-prefix legacy keys.
- Line 139: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // If no separator, return the entire key (edge case/legacy)

### storage/hamming_coder.cpp
Total findings: 5

- Line 94: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const uint32_t shard_size = static_cast<uint32_t>(available_chunks.begin()->second.size());
- Line 103: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto it = available_chunks.find(s);
- Line 102: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: const auto it = available_chunks.find(s);
- Line 18: severity=MEDIUM; category=coupling_risk_sharding_storage
  Description: Potential coupling risk between sharding/ and storage/ (validate no circular dependency)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_architecture_rules
  Context: #include "sharding/redundancy_strategy.h"
- Line 82: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,

### storage/tensor_router.cpp
Total findings: 5

- Line 353: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 194: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: double log_n = std::log(static_cast<double>(n_pilot));
- Line 195: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: double log_r = std::log(static_cast<double>(res.pilot_rank));
- Line 470: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void TensorRouter::setTemplateCatalog(
- Line 500: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: TensorRouter::templateCatalog() const noexcept {

### storage/blob_backend_gcs.cpp
Total findings: 4

- Line 108: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // DeleteObject() as a raw pointer delete — false positives.
- Line 226: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: "GCS delete failed: " + status.message());
- Line 226: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!status.ok()) {

        THEMIS_ERROR("GCS DeleteObject failed for {}: {}", obj, status.message());

        return Err<void>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,

                         "GCS delete failed: " + status.message());

    }



    THEMIS_DEBUG("GCS blob deleted: id={}", ref.id);
- Line 226: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: "GCS delete failed: " + status.message());

### storage/ggml_tensor_bridge.cpp
Total findings: 4

- Line 137: severity=CRITICAL; category=broken_raii_in_assignment
  Description: Broken RAII: undefined behavior and memory corruption
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 140: severity=CRITICAL; category=broken_raii_in_assignment
  Description: Broken RAII: undefined behavior and memory corruption
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 242: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: raw = storage->getVersion(key, static_cast<std::size_t>(version));
- Line 147: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Pretend to be a ggml_tensor for pointer compatibility in tests.

### storage/mvcc_chain_pruner.cpp
Total findings: 4

- Line 90: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    // 3. Migrate the pruneable versions into the TemporalTierManager, then

    //    delete them from the MVCC store.

    //

    //    sys_time.start = this version's timestamp

    //    sys_time.end   = next version's timestamp (open interval: the moment
- Line 90: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: //    delete them from the MVCC store.
- Line 118: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ++stats.versions_migrated;

    }



    // 4. Batch-delete the migrated versions from MVCC store in a single pass.

    //    gcVersionsBefore honours min_versions_to_keep so at most num_to_prune

    //    entries are removed.

    MVCCStore::GCOptions gc_opts;
- Line 118: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // 4. Batch-delete the migrated versions from MVCC store in a single pass.

### storage/mvcc_store.cpp
Total findings: 4

- Line 240: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 277: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // Keys are already in ascending timestamp order (big-endian sort).
- Line 280: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: uint64_t num_to_delete = 0;
- Line 291: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: num_to_delete = std::min(num_to_delete, max_deletable);

### storage/security_signature.cpp
Total findings: 4

- Line 99: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::optional<SecuritySignature> SecuritySignature::deserialize(const std::string& data) {
- Line 29: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: std::all_of(value.begin(), value.end(), [](unsigned char ch)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
- Line 105: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return fromJson(j);

    // uncaught_exception scanner alert (line 64): same rationale as fromJson —

    // catch(const std::exception&) is already specific — false positive.

    } catch (...) {

        return std::nullopt;

    }

}
- Line 105: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### storage/compression_strategy.cpp
Total findings: 2

- Line 610: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = value_to_index.find(value);
- Line 184: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(size, size_t(100)); ++i) {

### storage/storage_parquet_exporter.cpp
Total findings: 2

- Line 274: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: raw.data() + n * sizeof(int64_t));
- Line 316: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    size_t n = seg.metadata().row_count;', '    // rawData() invariant: raw.size() == n * element_size (1 for BOOL).', '    size_t packed_bytes = (n + 7) / 8;', '    std::vector<uint8_t> values(packed_bytes, 0);', '    for (size_t i = 0; i < n; ++i) {']

### storage/ARCHITECTURE.md
Total findings: 1

- Line 1: severity=LOW; category=missing_adr_reference
  Description: Architecture doc missing ADR references: adr_002
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_module_governance_rules
  Context: Add explicit ADR links/references for module-critical design decisions

### storage/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### storage/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### storage/blob_backend_filesystem.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #746 [Phase 4] Storage Layer: Mi... (2026-03-11)

### storage/blob_backend_webdav.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #746 [Phase 4] Storage Layer: Mi... (2026-03-11)

### storage/gguf_metadata.cpp
Total findings: 1

- Line 82: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // cryptographic helper over binary/string inputs and is not part of any LLM

### storage/nlp_metadata_extractor.cpp
Total findings: 1

- Line 207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: keywords.push_back(kw.text);

### storage/raft_mvcc_bridge.cpp
Total findings: 1

- Line 104: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: RaftMvccBridge::snapshotRead(std::string_view key, HLCTimestamp ts) {

### storage/schema_dead_weight_detector.cpp
Total findings: 1

- Line 138: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: signal.reserve(access_series.size());  // pre-allocated; missing_vector_reserve/copy_overhead scanne

### storage/simd_filter.cpp
Total findings: 1

- Line 55: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: int v = cached.load(std::memory_order_acquire);

### storage/storage_layout_advisor.cpp
Total findings: 1

- Line 58: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: diffs.reserve(ts.size() - 1);  // pre-allocated; missing_vector_reserve/copy_overhead scanner findin

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
