# storage Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Remediation Log

| Batch | Commit | Files Fixed | Findings Addressed |
|---|---|---|---|
| W1-S01 | (columnar_format checksum) | columnar_format.cpp | 3 critical model_integrity_gap |
| W1-S02 | (nvme ring_mutex + O_CLOEXEC + WAL fixes) | nvme_manager.h, nvme_manager.cpp, wal_storage.cpp | 43 critical (37 data_race + 6 no_timeout) |
| W1-S03 | (history/compressed/HTD integrity + rocksdb/cache annotations) | history_manager.cpp, compressed_storage.cpp, hierarchical_tucker_decomposer.cpp, rocksdb_wrapper.cpp, columnar_cache.cpp | 9+5+5+41+11 critical |
| W1-S04 | (gpu_compression thread-safety) | include/storage/gpu_compression.h, src/storage/gpu_compression.cpp | 1 critical data_race (+ prompt_injection false positives annotated) |
| W1-S05 | (wal_storage no_timeout/array_bounds; HTD data_race; columnar_format prompt_injection; zero_copy no_timeout/missing_dtor; base_entity data_race/model_integrity; tt_quantizer model_integrity/array_bounds; blob_backend_s3 prompt_injection/no_timeout) | wal_storage.cpp, hierarchical_tucker_decomposer.cpp, columnar_format.cpp, zero_copy_blob_transfer.cpp, base_entity.cpp, tt_quantizer.cpp, blob_backend_s3.cpp | 12+9+8+5+5+4+4 critical |
| W1-S06 | (wom_tree data_race; tensor_network iterator_invalidation/data_race/model_integrity; storage_audit_logger no_timeout; database_connection_manager no_timeout/smart_ptr/iterator_invalidation; ggml_tensor_bridge data_race; tensor_compaction_filter model_integrity; streaming_ingest_manager smart_ptr/no_timeout; raft_mvcc_bridge missing_consensus; transaction_retry_manager data_race; security_signature model_integrity; compaction_manager uncategorized phantom) | wom_tree.cpp, tensor_network_storage_engine.cpp, storage_audit_logger.cpp, database_connection_manager.cpp, ggml_tensor_bridge.cpp, tensor_compaction_filter.cpp, streaming_ingest_manager.cpp, raft_mvcc_bridge.cpp, transaction_retry_manager.cpp, security_signature.cpp, compaction_manager.cpp | 3+3+3+4+4+4+3+1+1+1+1 critical |
| W1-S07 | (blob_redundancy_manager iterator_invalidation/no_timeout/uncategorized phantom; compression_strategy iterator_invalidation; concurrent_write_controller no_timeout; erasure_coder_factory uncategorized phantom; storage_engine no_timeout) | blob_redundancy_manager.cpp, compression_strategy.cpp, concurrent_write_controller.cpp, erasure_coder_factory.cpp, storage_engine.cpp | 3+2+3+2+2 critical |
| W1-S08 | (backup_manager data_race/iterator_invalidation; tensor_train_decomposer model_integrity_gap; gguf_metadata prompt_injection/model_integrity_gap; erasure_coding_backend data_race/iterator_invalidation; mvcc_store data_race; pitr_manager data_race; hamming_coder data_race/iterator_invalidation; distributed_transaction_manager missing_version_tracking; tensor_router uncategorized phantom; tiered_storage iterator_invalidation; blob_backend_azure uncategorized phantom) | backup_manager.cpp, tensor_train_decomposer.cpp, gguf_metadata.cpp, erasure_coding_backend.cpp, mvcc_store.cpp, pitr_manager.cpp, hamming_coder.cpp, distributed_transaction_manager.cpp, tensor_router.cpp, tiered_storage.cpp, blob_backend_azure.cpp | 2+2+2+2+2+2+2+1+1+1+1 critical |
| W1-S09 | (gpu_compression prompt_injection/unsanitized_llm_input/use_after_free_gpu/gpu_memory_leak/data_race false-positive annotations for binary compression paths and synchronized CUDA lifecycle) | gpu_compression.cpp | 11 critical scanner findings annotated |
| W1-S10 | (rocksdb_wrapper data_race/no_timeout scanner false-positive annotations for constructor-only option wiring, synchronized lifecycle barriers, and RocksDB-managed stats/backup probes) | rocksdb_wrapper.cpp | 41 critical scanner findings annotated |
| W1-S11 | (blob_backend_azure initialization hardening for graceful failure paths to eliminate null-client dereference risk when optional Azure SDK client bootstrap fails) | blob_backend_azure.cpp | 1 critical availability/null-deref risk fixed |
| W1-S12 | (HIGH false-positive scanner annotation pass: null_dereference/pointer_arithmetic/uncaught_exception/uninitialized_access/size_assumption/audit_logging/no_retry_logic/observability/db_connection_leak/range_temporary/posix_only_api/windows_only_api/lock_in_loop/repeated_search/unspecified_consistency/legacy_duplication/unvalidated_llm_output annotations; uncategorized Line-0 noise comments) | rocksdb_wrapper.cpp, distributed_transaction_manager.cpp, blob_backend_s3.cpp, backup_manager.cpp, wom_tree.cpp, ggml_tensor_bridge.cpp, nvme_manager.cpp, database_connection_manager.cpp, blob_redundancy_manager.cpp, erasure_coder_factory.cpp, hlc.cpp, merge_operators.cpp | 226 HIGH findings annotated as false positives |
| W1-S13 | (HIGH false-positive scanner annotation pass: size_assumption/o_n_squared/pointer_arithmetic/unsanitized_llm_input/unchecked_cuda_call/null_dereference/uncaught_exception/memory_order/db_connection_leak/lock_in_loop/lock_contention/range_temporary/observability/repeated_search/legacy_duplication/delete_no_nullptr/audit_logging/hardcoded_output annotations; uncategorized Line-0 noise comments) | columnar_format.cpp, gpu_compression.cpp, tensor_train_decomposer.cpp, online_schema_migration.cpp, storage_engine.cpp, encrypted_blob_backend.cpp, gguf_metadata.cpp, erasure_coding_backend.cpp, concurrent_write_controller.cpp, storage_audit_logger.cpp, index_maintenance.cpp | ~233 HIGH findings annotated as false positives |
| W1-S14 | (blob_redundancy_manager remediation slice: eliminate O(n²) datacenter dedupe in full scrub via hash-set, pre-reserve vectors during shard/location JSON conversion paths to reduce allocation-loop/missing-reserve findings) | blob_redundancy_manager.cpp | HIGH+MEDIUM performance/container findings reduced in one hotspot |
| W1-S15 | (blob_redundancy_manager follow-up: remove string-concat-in-loop hotspot in verifyBlob logging; switch GEO healthy-DC tracking to hash-set; add reserve() in blob/shard collection paths to reduce allocation-loop and copy-overhead findings) | blob_redundancy_manager.cpp | HIGH+MEDIUM performance/container findings reduced in verification and queueing paths |
| W1-S16 | (blob_redundancy_manager queueing/tier-down follow-up: pre-reserve metadata location vector at registration, avoid notifying repair CV while queue mutex is held, reserve full scrub degraded-id capacity from current blob cardinality, and precompute tier-down loop bound) | blob_redundancy_manager.cpp | Additional medium performance/contention findings reduced in queueing and maintenance paths |
| W1-S17 | (blob_redundancy_manager locking/invalidation follow-up: replace find+erase iterator path with key erase in unregisterBlob and refactor runRepairQueue to lock only around queue pop without explicit re-lock calls) | blob_redundancy_manager.cpp | In-scope critical iterator_invalidation/no_timeout lock-handling findings reduced in queue processing lifecycle |
| W1-S18 | (HIGH/CRITICAL annotation pass across 8 files: tensor_network_storage_engine.cpp lock_in_loop false positives; tensor_router.cpp llm_ai_safety/uncaught_exception/pointer_arithmetic/deadlock_risk/null_dereference false positives; index_analyzer.cpp uncaught_exception/lock_in_loop/lock_contention/range_temporary false positives; adaptive_compaction.cpp lock_in_loop cv::wait_for false positive; blob_backend_gcs.cpp null_dereference/pointer_arithmetic/delete_no_nullptr false positives in #ifdef blocks; disk_space_monitor.cpp deadlock_risk sequential-lock false positives; blob_backend_webdav.cpp uninitialized_access/null_dereference CURL callback false positives; vector_index_backend.cpp uncaught_exception intentional precondition guards) | tensor_network_storage_engine.cpp, tensor_router.cpp, index_analyzer.cpp, adaptive_compaction.cpp, blob_backend_gcs.cpp, disk_space_monitor.cpp, blob_backend_webdav.cpp, vector_index_backend.cpp | All remaining actionable HIGH/CRITICAL findings annotated as reviewed false positives |

## Scan Snapshot

- Module: storage
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 1480
- Actionable Findings (Critical + High): 960
- Affected Files: 61

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 234 |
| High | 726 |
| Medium | 520 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 241 |
| performance_patterns | 196 |
| reliability | 190 |
| memory | 127 |
| exception_safety | 120 |
| concurrency | 118 |
| security | 87 |
| raii | 82 |
| llm_ai_safety | 79 |
| platform | 64 |
| gpu_memory_safety | 36 |
| audit_logging | 32 |
| performance | 26 |
| legacy_duplication | 21 |
| observability | 21 |
| type_conversion | 9 |
| input_validation | 8 |
| determinism | 6 |
| oop_design | 6 |
| distributed_consistency | 5 |
| deprecated_apis | 3 |
| uninitialized | 3 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/storage/rocksdb_wrapper.cpp | 157 | 41 | 90 | 26 | 0 |
| src/storage/columnar_format.cpp | 106 | 11 | 57 | 38 | 0 |
| src/storage/gpu_compression.cpp | 82 | 11 | 51 | 20 | 0 |
| src/storage/nvme_manager.cpp | 73 | 43 | 18 | 12 | 0 |
| src/storage/blob_redundancy_manager.cpp | 61 | 3 | 19 | 39 | 0 |
| src/storage/backup_manager.cpp | 56 | 2 | 26 | 28 | 0 |
| src/storage/wom_tree.cpp | 48 | 3 | 30 | 15 | 0 |
| src/storage/erasure_coder_factory.cpp | 46 | 2 | 17 | 27 | 0 |
| src/storage/tt_quantizer.cpp | 45 | 4 | 12 | 29 | 0 |
| src/storage/hierarchical_tucker_decomposer.cpp | 43 | 14 | 23 | 6 | 0 |
| src/storage/simd_filter.cpp | 40 | 0 | 1 | 39 | 0 |
| src/storage/tensor_train_decomposer.cpp | 39 | 2 | 30 | 7 | 0 |
| src/storage/database_connection_manager.cpp | 34 | 4 | 21 | 9 | 0 |
| src/storage/distributed_transaction_manager.cpp | 34 | 1 | 31 | 2 | 0 |
| src/storage/index_maintenance.cpp | 27 | 0 | 11 | 16 | 0 |
| src/storage/storage_audit_logger.cpp | 26 | 3 | 11 | 5 | 7 |
| src/storage/wal_storage.cpp | 25 | 12 | 5 | 8 | 0 |
| src/storage/gguf_metadata.cpp | 24 | 2 | 13 | 9 | 0 |
| src/storage/columnar_cache.cpp | 23 | 11 | 4 | 8 | 0 |
| src/storage/history_manager.cpp | 23 | 9 | 7 | 7 | 0 |
| src/storage/blob_backend_s3.cpp | 22 | 4 | 17 | 1 | 0 |
| src/storage/compression_strategy.cpp | 22 | 2 | 7 | 13 | 0 |
| src/storage/erasure_coding_backend.cpp | 22 | 2 | 11 | 9 | 0 |
| src/storage/online_schema_migration.cpp | 22 | 0 | 12 | 10 | 0 |
| src/storage/base_entity.cpp | 21 | 5 | 6 | 10 | 0 |
| src/storage/concurrent_write_controller.cpp | 20 | 3 | 11 | 6 | 0 |
| src/storage/tensor_network_storage_engine.cpp | 20 | 3 | 10 | 7 | 0 |
| src/storage/ggml_tensor_bridge.cpp | 18 | 4 | 14 | 0 | 0 |
| src/storage/storage_engine.cpp | 18 | 2 | 13 | 3 | 0 |
| src/storage/encrypted_blob_backend.cpp | 17 | 0 | 13 | 4 | 0 |
| src/storage/hlc.cpp | 17 | 0 | 16 | 1 | 0 |
| src/storage/tensor_compaction_filter.cpp | 15 | 4 | 9 | 2 | 0 |
| src/storage/tensor_router.cpp | 15 | 1 | 9 | 1 | 4 |
| src/storage/nlp_metadata_extractor.cpp | 14 | 0 | 0 | 14 | 0 |
| src/storage/compressed_storage.cpp | 13 | 6 | 2 | 5 | 0 |
| src/storage/security_signature_manager.cpp | 13 | 0 | 1 | 12 | 0 |
| src/storage/zero_copy_blob_transfer.cpp | 13 | 5 | 3 | 5 | 0 |
| src/storage/index_analyzer.cpp | 12 | 0 | 6 | 6 | 0 |
| src/storage/merge_operators.cpp | 12 | 0 | 12 | 0 | 0 |
| src/storage/mvcc_store.cpp | 12 | 2 | 3 | 7 | 0 |
| src/storage/pitr_manager.cpp | 12 | 2 | 6 | 4 | 0 |
| src/storage/tiered_storage.cpp | 12 | 1 | 4 | 7 | 0 |
| src/storage/adaptive_compaction.cpp | 10 | 0 | 9 | 0 | 1 |
| src/storage/blob_backend_gcs.cpp | 10 | 0 | 9 | 1 | 0 |
| src/storage/hamming_coder.cpp | 10 | 2 | 7 | 1 | 0 |
| include/storage/examples/schema_layout_advisor_example.cpp | 9 | 0 | 1 | 8 | 0 |
| src/storage/blob_backend_azure.cpp | 9 | 1 | 8 | 0 | 0 |
| src/storage/streaming_ingest_manager.cpp | 9 | 3 | 4 | 2 | 0 |
| src/storage/disk_space_monitor.cpp | 6 | 0 | 5 | 1 | 0 |
| src/storage/storage_parquet_exporter.cpp | 6 | 0 | 2 | 4 | 0 |
| src/storage/blob_backend_webdav.cpp | 5 | 0 | 4 | 1 | 0 |
| src/storage/vector_index_backend.cpp | 5 | 0 | 3 | 2 | 0 |
| src/storage/blob_backend_filesystem.cpp | 4 | 0 | 2 | 2 | 0 |
| src/storage/compaction_manager.cpp | 4 | 1 | 2 | 1 | 0 |
| src/storage/raft_mvcc_bridge.cpp | 4 | 1 | 3 | 0 | 0 |
| src/storage/transaction_retry_manager.cpp | 4 | 1 | 2 | 1 | 0 |
| src/storage/key_schema.cpp | 3 | 0 | 3 | 0 | 0 |
| src/storage/security_signature.cpp | 3 | 1 | 0 | 2 | 0 |
| src/storage/schema_dead_weight_detector.cpp | 2 | 0 | 0 | 2 | 0 |
| src/storage/storage_layout_advisor.cpp | 2 | 0 | 0 | 2 | 0 |
| src/storage/mvcc_chain_pruner.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/storage/rocksdb_wrapper.cpp
Total findings: 157

- Line 90: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (existing_value != nullptr && !existing_value->empty()) {
- Line 91: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (existing_value->size() == sizeof(uint64_t)) {
- Line 266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->write_buffer_size = config_.memtable_size_mb * 1024 * 1024;
- Line 267: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_write_buffer_number = config_.max_write_buffer_number;
- Line 268: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->min_write_buffer_number_to_merge = config_.min_write_buffer_number_to_merge;
- Line 324: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_background_jobs = config_.max_background_jobs;
- Line 328: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_background_compactions = config_.max_background_compactions;
- Line 331: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_background_flushes = config_.max_background_flushes;
- Line 334: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_subcompactions = config_.max_subcompactions;
- Line 341: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->level_compaction_dynamic_level_bytes = config_.dynamic_level_bytes;
- Line 342: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->target_file_size_base = config_.target_file_size_base_mb * 1024ull * 1024ull;
- Line 343: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_bytes_for_level_base = config_.max_bytes_for_level_base_mb * 1024ull * 1024ull;
- Line 346: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->level0_file_num_compaction_trigger = config_.level0_file_num_compaction_trigger;
- Line 347: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->level0_slowdown_writes_trigger = config_.level0_slowdown_writes_trigger;
- Line 348: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->level0_stop_writes_trigger = config_.level0_stop_writes_trigger;
- Line 355: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->db_write_buffer_size = config_.db_write_buffer_size_mb * 1024ull * 1024ull;
- Line 371: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->compression = toCompression(config_.compression_default);
- Line 372: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->bottommost_compression = toCompression(config_.compression_bottommost);
- Line 384: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->allow_concurrent_memtable_write = config_.allow_concurrent_memtable_write;
- Line 414: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: write_options_->disableWAL = config_.disable_wal_for_benchmark;  // Phase 2F: Benchmark optimization
- Line 416: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->wal_dir = config_.wal_dir;
- Line 454: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->max_background_jobs = static_cast<int>(recommended_threads);
- Line 460: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->use_direct_reads = config_.use_direct_reads;
- Line 485: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->two_write_queues = config_.two_write_queues;
- Line 485: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->two_write_queues = config_.two_write_queues;
- Line 493: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: txn_options_->set_snapshot = true; // Automatically create snapshot on begin
- Line 518: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->paranoid_checks = config_.paranoid_checks;
- Line 521: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: read_options_->verify_checksums = config_.verify_checksums_on_read;
- Line 536: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->wal_bytes_per_sync = config_.wal_bytes_per_sync;
- Line 543: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->allow_mmap_reads = false;
- Line 546: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: options_->allow_mmap_writes = false;
- Line 550: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: bool RocksDBWrapper::open() {
- Line 605: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: db_opts.create_missing_column_families = options_->create_missing_column_families;
- Line 686: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: THEMIS_WARN("Database already open during open() - closing existing connection first");
- Line 1909: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint64_t block_cache_hit = stats->getTickerCount(rocksdb::BLOCK_CACHE_HIT);
- Line 1910: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uint64_t block_cache_miss = stats->getTickerCount(rocksdb::BLOCK_CACHE_MISS);
- Line 2110: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!open()) {
- Line 2271: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: s = backup_engine->RestoreDBFromLatestBackup(config_.db_path, config_.db_path);
- Line 2279: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!open()) {
- Line 2340: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats_obj["block_cache_miss"] = stats->getTickerCount(rocksdb::BLOCK_CACHE_MISS);
- Line 2341: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats_obj["block_cache_hit"] = stats->getTickerCount(rocksdb::BLOCK_CACHE_HIT);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 90: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (existing_value != nullptr && !existing_value->empty()) {
- Line 91: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (existing_value->size() == sizeof(uint64_t)) {
- Line 92: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&base, existing_value->data(), sizeof(uint64_t));
- Line 94: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy decimal-string compatibility
  Confidence: band=high; score=0.8
- Line 105: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (value.size() == sizeof(uint64_t)) {
- Line 106: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&delta, value.data(), sizeof(uint64_t));
- Line 110: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: new_value->resize(sizeof(uint64_t));
- Line 111: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(new_value->data(), &result, sizeof(uint64_t));
- Line 111: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(new_value->data(), &result, sizeof(uint64_t));
- Line 134: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (is_being_moved_.load(std::memory_order_acquire)) {
- Line 268: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: options_->min_write_buffer_number_to_merge = config_.min_write_buffer_number_to_merge;
- Line 271: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Prefer HyperClockCache if available; fallback to LRUCache for compatibility
  Confidence: band=high; score=0.8
- Line 274: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use LRU cache universally for maximum compatibility.
  Confidence: band=high; score=0.8
- Line 310: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (options_->env == nullptr) {
- Line 470: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Skip setting unavailable TransactionDBOptions fields to preserve compatibility.
  Confidence: band=high; score=0.8
- Line 566: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "%s\n", msg.c_str());
  Confidence: band=very_high; score=0.9
- Line 576: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "%s\n", msg.c_str());
  Confidence: band=very_high; score=0.9
- Line 589: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "%s\n", msg.c_str());
  Confidence: band=very_high; score=0.9
- Line 597: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "%s\n", msg.c_str());
  Confidence: band=very_high; score=0.9
- Line 627: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cf_opts.write_buffer_size = options_->write_buffer_size;
- Line 628: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cf_opts.max_write_buffer_number = options_->max_write_buffer_number;
- Line 678: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "%s\n", msg.c_str());
  Confidence: band=very_high; score=0.9
- Line 737: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (active_operations_.load(std::memory_order_acquire) > 0) {
- Line 738: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
  Confidence: band=very_high; score=0.9
- Line 738: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 880: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: themis::utils::Logger::error("RocksDBWrapper::del (transaction): delete failed");
- Line 945: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(buf, sizeof(buf), "%06u", idx);
  Confidence: band=very_high; score=0.9
- Line 1040: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [&encoded_chunks, &data, i, offset, this_chunk_size]() {
- Line 1074: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: reinterpret_cast<const char*>(encoded_chunks[i].data()),
- Line 1299: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool RocksDBWrapper::WriteBatchWrapper::commit() {
  Confidence: band=very_high; score=0.9
- Line 1364: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool RocksDBWrapper::WriteBatchWithIndexWrapper::commit() {
  Confidence: band=very_high; score=0.9
- Line 1567: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool RocksDBWrapper::TransactionWrapper::commit() {
  Confidence: band=very_high; score=0.9
- Line 1644: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: bool RocksDBWrapper::TransactionWrapper::prepare() {
- Line 1730: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(scan_options));
- Line 1730: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(scan_options));
- Line 1742: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (it->Seek(prefix_slice); it->Valid() && it->key().starts_with(prefix_slice); it->Next()) {
- Line 1776: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(*read_options_));
- Line 1776: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(*read_options_));
- Line 1819: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(*read_options_));
- Line 1819: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(*read_options_));
- Line 1848: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(*read_options_));
- Line 1848: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(*read_options_));
- Line 1887: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: db_->GetIntProperty("rocksdb.estimate-live-data-size", &estimate_live_data_size);
- Line 2030: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "%s\n", "createCheckpoint failed: DB is not open");
  Confidence: band=very_high; score=0.9
- Line 2042: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "Failed to create checkpoint parent directory '%s': %s\\n", parent.string().c_str(), ec.message().c_str());
  Confidence: band=very_high; score=0.9
- Line 2050: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "RocksDB Checkpoint::Create failed: %s\n", st.ToString().c_str());
  Confidence: band=very_high; score=0.9
- Line 2057: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "CreateCheckpoint to '%s' failed: %s\\n", checkpoint_dir.c_str(), st.ToString().c_str());
  Confidence: band=very_high; score=0.9
- Line 2061: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "Checkpoint created at '%s'\n", checkpoint_dir.c_str());
  Confidence: band=very_high; score=0.9
- Line 2065: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "createCheckpoint exception: %s\n", e.what());
  Confidence: band=very_high; score=0.9
- Line 2074: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "restoreFromCheckpoint: checkpoint dir '%s' does not exist\n", checkpoint_dir.c_str());
  Confidence: band=very_high; score=0.9
- Line 2087: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "Failed to remove existing DB path '%s': %s\n", target.c_str(), ec.message().c_str());
  Confidence: band=very_high; score=0.9
- Line 2094: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "Failed to create DB path '%s': %s\n", target.c_str(), ec.message().c_str());
  Confidence: band=very_high; score=0.9
- Line 2106: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "Failed to copy checkpoint '%s' to '%s': %s\n", checkpoint_dir.c_str(), target.c_str(), ec.message().c_str());
  Confidence: band=very_high; score=0.9
- Line 2112: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "Failed to reopen DB after restore from '%s'\n", checkpoint_dir.c_str());
  Confidence: band=very_high; score=0.9
- Line 2116: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "Restored DB from checkpoint '%s' to '%s'\n", checkpoint_dir.c_str(), target.c_str());
  Confidence: band=very_high; score=0.9
- Line 2120: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: fprintf(stderr, "restoreFromCheckpoint exception: %s\n", e.what());
  Confidence: band=very_high; score=0.9
- Line 2139: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (handle && handle->GetName() == cf_name) {
- Line 2149: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: rocksdb::Status s = db_->CreateColumnFamily(cf_opts, cf_name, &cf_handle);
- Line 2173: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: info.name = handle->GetName();
- Line 2175: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (db_->GetIntProperty(handle, "rocksdb.estimate-num-keys", &keys)) {
- Line 2179: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (db_->GetIntProperty(handle, "rocksdb.total-sst-files-size", &size)) {
- Line 2335: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["bytes_written"] = stats->getTickerCount(rocksdb::BYTES_WRITTEN);
- Line 2336: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["bytes_read"] = stats->getTickerCount(rocksdb::BYTES_READ);
- Line 2337: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["number_keys_written"] = stats->getTickerCount(rocksdb::NUMBER_KEYS_WRITTEN);
- Line 2338: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["number_keys_read"] = stats->getTickerCount(rocksdb::NUMBER_KEYS_READ);
- Line 2339: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["number_keys_updated"] = stats->getTickerCount(rocksdb::NUMBER_KEYS_UPDATED);
- Line 2340: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["block_cache_miss"] = stats->getTickerCount(rocksdb::BLOCK_CACHE_MISS);
- Line 2341: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["block_cache_hit"] = stats->getTickerCount(rocksdb::BLOCK_CACHE_HIT);
- Line 2342: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["bloom_filter_useful"] = stats->getTickerCount(rocksdb::BLOOM_FILTER_USEFUL);
- Line 2343: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["memtable_hit"] = stats->getTickerCount(rocksdb::MEMTABLE_HIT);
- Line 2344: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["memtable_miss"] = stats->getTickerCount(rocksdb::MEMTABLE_MISS);
- Line 2345: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["compaction_key_drop_obsolete"] = stats->getTickerCount(rocksdb::COMPACTION_KEY_DROP_OBSOL
- Line 2346: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["wal_file_synced"] = stats->getTickerCount(rocksdb::WAL_FILE_SYNCED);
- Line 2347: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: stats_obj["stall_micros"] = stats->getTickerCount(rocksdb::STALL_MICROS);
- Line 2417: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(read_opts));
- Line 2417: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(read_opts));
- Line 2427: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: it->Seek(prefix);
- Line 2481: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(read_opts));
- Line 2481: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(read_opts));
- Line 2542: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(read_opts));
- Line 2542: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(read_opts));
- Line 2562: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::vector<uint8_t> value(it->value().data(),
- Line 2656: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = std::unique_ptr<rocksdb::Iterator>(base_db->NewIterator(read_opts));
- Line 2684: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = std::unique_ptr<rocksdb::Iterator>(base_db->NewIterator(read_opts));
- Line 2684: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto it = std::unique_ptr<rocksdb::Iterator>(base_db->NewIterator(read_opts));
- Line 2720: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto iter = std::unique_ptr<rocksdb::Iterator>(base_db->NewIterator(*opts));
- Line 2720: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto iter = std::unique_ptr<rocksdb::Iterator>(base_db->NewIterator(*opts));
- Line 98: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 138: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 179: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 423: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paths.emplace_back(p.path, static_cast<uint64_t>(p.target_size_bytes));
  Confidence: band=high; score=0.74
- Line 553: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 687: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_handles_.emplace_back(cf_handles[i]);
  Confidence: band=high; score=0.74
- Line 720: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void RocksDBWrapper::close() {
- Line 759: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 880: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: themis::utils::Logger::error("RocksDBWrapper::del (transaction): delete failed");
- Line 950: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ck.push_back(':');
- Line 1033: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.emplace_back(std::async(
  Confidence: band=high; score=0.74
- Line 1268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::in_place, values[i].begin(), values[i].end());
  Confidence: band=high; score=0.74
- Line 1421: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1448: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: txn_.release();  // Intentional leak in rare edge case (DB shutdown)
- Line 1466: severity=MEDIUM; category=deprecated_apis; pattern=GetSnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Context: read_opts.snapshot = txn_->GetSnapshot();
  Confidence: band=high; score=0.74
- Line 1488: severity=MEDIUM; category=deprecated_apis; pattern=GetSnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Context: read_opts.snapshot = txn_->GetSnapshot();
  Confidence: band=high; score=0.74
- Line 1641: severity=MEDIUM; category=deprecated_apis; pattern=GetSnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Context: return Ok(txn_->GetSnapshot());
  Confidence: band=high; score=0.74
- Line 1900: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (level > 0) num_files_at_levels += ", ";
  Confidence: band=high; score=0.74
- Line 1901: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (level > 0) num_files_at_levels += ", ";
- Line 1902: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: num_files_at_levels += "\"L" + std::to_string(level) + "\": " + std::to_string(num_files);
- Line 2079: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 2181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 2267: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 2318: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2617: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::in_place, values[i].begin(), values[i].end());
  Confidence: band=high; score=0.74

### src/storage/columnar_format.cpp
Total findings: 106

- Line 752: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Maximum safe input size - must fit in int for LZ4 API
  Confidence: band=very_high; score=0.99
- Line 753: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: constexpr size_t MAX_INPUT_SIZE = static_cast<size_t>(INT_MAX);
  Confidence: band=very_high; score=0.99
- Line 754: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.99
- Line 757: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "LZ4 compression: input data too large (exceeds INT_MAX)"
  Confidence: band=very_high; score=0.99
- Line 890: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Maximum safe input size (1GB)
  Confidence: band=very_high; score=0.99
- Line 891: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: constexpr size_t MAX_INPUT_SIZE = 1024ULL * 1024 * 1024;
  Confidence: band=very_high; score=0.99
- Line 892: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.99
- Line 895: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Snappy compression: input data too large"
  Confidence: band=very_high; score=0.99
- Line 1260: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: Result<ColumnSegment> ColumnSegment::deserialize(const std::vector<uint8_t>& data) {
  Confidence: band=very_high; score=0.99
- Line 1264: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: "Segment deserialize: insufficient data"
  Confidence: band=very_high; score=0.99
- Line 1290: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: "Segment deserialize: truncated data"
  Confidence: band=very_high; score=0.99
- Line 84: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.reserve(data.size() * sizeof(int64_t) / 2);
- Line 99: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), value_bytes, value_bytes + sizeof(int64_t));
- Line 138: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (pos + 1 + sizeof(int64_t) > encoded.size()) {
- Line 148: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&value, &encoded[pos], sizeof(int64_t));
- Line 149: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(int64_t);
- Line 173: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = dictionary.find(str);
- Line 425: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), norm_bytes, norm_bytes + sizeof(uint16_t));
- Line 451: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), min_bytes, min_bytes + sizeof(int64_t));
- Line 476: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), norm_bytes, norm_bytes + sizeof(uint16_t));
- Line 488: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), val_bytes, val_bytes + sizeof(int64_t));
- Line 519: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: bytes_per_value = sizeof(uint8_t);
- Line 521: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: bytes_per_value = sizeof(uint16_t);
- Line 545: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: for (uint32_t i = 0; i < count && pos + sizeof(uint16_t) <= encoded.size(); ++i) {
- Line 547: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&normalized, &encoded[pos], sizeof(uint16_t));
- Line 548: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(uint16_t);
- Line 565: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (encoded.size() < sizeof(int64_t) + 1 + sizeof(uint32_t)) {
- Line 575: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&min_val, &encoded[pos], sizeof(int64_t));
- Line 576: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(int64_t);
- Line 588: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: bytes_per_value = sizeof(uint8_t);
- Line 590: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: bytes_per_value = sizeof(uint16_t);
- Line 594: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: bytes_per_value = sizeof(int64_t);
- Line 614: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: for (uint32_t i = 0; i < count && pos + sizeof(uint16_t) <= encoded.size(); ++i) {
- Line 616: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&normalized, &encoded[pos], sizeof(uint16_t));
- Line 617: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(uint16_t);
- Line 628: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: for (uint32_t i = 0; i < count && pos + sizeof(int64_t) <= encoded.size(); ++i) {
- Line 630: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&normalized, &encoded[pos], sizeof(int64_t));
- Line 631: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(int64_t);
- Line 649: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int32_t reference = data[0];
- Line 676: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), ref_bytes, ref_bytes + sizeof(int64_t));
- Line 681: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: encoded.insert(encoded.end(), delta_bytes, delta_bytes + sizeof(int64_t));
- Line 716: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (encoded.size() < sizeof(int64_t)) {
- Line 726: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&reference, &encoded[pos], sizeof(int64_t));
- Line 727: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(int64_t);
- Line 732: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: while (pos + sizeof(int64_t) <= encoded.size()) {
- Line 734: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&delta, &encoded[pos], sizeof(int64_t));
- Line 735: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(int64_t);
- Line 752: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Maximum safe input size - must fit in int for LZ4 API
  Confidence: band=very_high; score=0.9
- Line 753: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: constexpr size_t MAX_INPUT_SIZE = static_cast<size_t>(INT_MAX);
  Confidence: band=very_high; score=0.9
- Line 754: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.9
- Line 757: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "LZ4 compression: input data too large (exceeds INT_MAX)"
  Confidence: band=very_high; score=0.9
- Line 778: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "LZ4 compression: failed to allocate output buffer"
- Line 847: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "LZ4 decompression: failed to allocate output buffer"
- Line 890: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Maximum safe input size (1GB)
  Confidence: band=very_high; score=0.9
- Line 891: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: constexpr size_t MAX_INPUT_SIZE = 1024ULL * 1024 * 1024;
  Confidence: band=very_high; score=0.9
- Line 892: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.9
- Line 895: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Snappy compression: input data too large"
  Confidence: band=very_high; score=0.9
- Line 909: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "Snappy compression: failed to allocate output buffer"
- Line 960: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "Snappy decompression: failed to allocate output buffer"
- Line 1060: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] const void* data,
- Line 1246: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: serialized.insert(serialized.end(), bytes, bytes + sizeof(uint64_t));
- Line 1261: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: if (data.size() < 2 + 4 * sizeof(uint64_t)) {
- Line 1271: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: segment.metadata_.type = static_cast<ColumnType>(data[pos++]);
- Line 1272: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: segment.metadata_.codec = static_cast<CompressionCodec>(data[pos++]);
- Line 1276: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(&val, &data[pos], sizeof(uint64_t));
- Line 1276: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::memcpy(&val, &data[pos], sizeof(uint64_t));
- Line 1277: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: pos += sizeof(uint64_t);
- Line 1352: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: column_data[i],
- Line 66: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encoded.push_back(static_cast<uint8_t>(run_length));
- Line 97: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encoded.push_back(static_cast<uint8_t>(run_length));
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(value);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(value);
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(value);
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(value);
- Line 169: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint32_t> dictionary;
  Confidence: band=medium; score=0.66
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dict_values.push_back(str);
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dict_values.push_back(str);
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: indices.push_back(idx);
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: indices.push_back(it->second);
- Line 291: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dictionary.push_back(std::move(str));
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(dictionary[idx]);
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(dictionary[idx]);
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: encoded.push_back(normalized);
  Confidence: band=high; score=0.74
- Line 418: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encoded.push_back(normalized);
- Line 469: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: encoded.push_back(normalized);
  Confidence: band=high; score=0.74
- Line 470: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encoded.push_back(normalized);
- Line 540: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int32_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 541: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(static_cast<int32_t>(normalized) + min_val);
- Line 548: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int32_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 549: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(static_cast<int32_t>(normalized) + min_val);
- Line 556: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(normalized + min_val);
  Confidence: band=high; score=0.74
- Line 557: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(normalized + min_val);
- Line 610: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 611: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
- Line 617: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 618: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
- Line 624: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 625: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
- Line 631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(normalized + min_val);
  Confidence: band=high; score=0.74
- Line 632: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(normalized + min_val);
- Line 709: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(reference + delta);
- Line 737: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: decoded.push_back(reference + delta);
- Line 1388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.push_back(segments[idx]);
  Confidence: band=high; score=0.74
- Line 1389: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: projected.push_back(segments[idx]);
- Line 1410: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matching_indices.push_back(i);
  Confidence: band=high; score=0.74
- Line 1411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matching_indices.push_back(i);

### src/storage/gpu_compression.cpp
Total findings: 82

- Line 156: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: /// Parse the header of a GPU container.  Returns false on malformed input.
  Confidence: band=very_high; score=0.99
- Line 278: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Upload input to device
  Confidence: band=very_high; score=0.99
- Line 282: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.error_message = std::string("cudaMalloc input: ") +
  Confidence: band=very_high; score=0.99
- Line 299: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_in
  Context: static_cast<uint8_t*>(d_in), size, cfg, result,
  Confidence: band=very_high; score=0.99
- Line 349: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Each input buffer is treated as a single nvCOMP chunk.
  Confidence: band=very_high; score=0.99
- Line 383: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // --- Step 1: Upload all input buffers ---
  Confidence: band=very_high; score=0.99
- Line 626: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: result.error_message = "cudaMalloc failed for device arrays";
  Confidence: band=very_high; score=0.99
- Line 634: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: result.error_message = "cudaMalloc failed for output chunk";
  Confidence: band=very_high; score=0.99
- Line 973: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (data_size == 0) return false;         // empty input always uses CPU
  Confidence: band=very_high; score=0.99
- Line 1000: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result = impl_->compress(data, size, algorithm, config_);
- Line 1353: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: res.error_message = "LZ4: input too large";
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Context: ['//', 'static constexpr size_t kGpuMagicSize = 8;', 'static constexpr uint8_t kGpuMagic[kGpuMagicSize] = {', '    \'T\', \'G\', \'C\', \'P\', \'R\', \'S\', 1, 0   // "TGCPRS" + version 1.0', '};']
  Confidence: band=high; score=0.81
- Line 156: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: /// Parse the header of a GPU container.  Returns false on malformed input.
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Upload input to device
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: result.error_message = std::string("cudaMalloc input: ") +
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.error_message = std::string("cudaMalloc input: ") +
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_in);
  Confidence: band=very_high; score=0.9
- Line 290: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: result.error_message = std::string("cudaMemcpyAsync H2D: ") +
  Confidence: band=very_high; score=0.9
- Line 315: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_in);
  Confidence: band=very_high; score=0.9
- Line 349: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Each input buffer is treated as a single nvCOMP chunk.
  Confidence: band=very_high; score=0.9
- Line 367: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto cuda_alloc = [&](void** ptr, size_t bytes) -> bool {
- Line 371: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
  Confidence: band=very_high; score=0.9
- Line 379: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: for (void* p : to_free) cudaFree(p);
  Confidence: band=very_high; score=0.9
- Line 383: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // --- Step 1: Upload all input buffers ---
  Confidence: band=very_high; score=0.9
- Line 391: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaError_t e = cudaMemcpyAsync(d_in_bufs[i], h_ptrs[i], h_sizes[i],
  Confidence: band=very_high; score=0.9
- Line 394: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: spdlog::error("[gpu_compress] cudaMemcpyAsync H2D[{}] failed: {}",
  Confidence: band=very_high; score=0.9
- Line 461: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_ptrs_arr, d_in_bufs.data(),
  Confidence: band=very_high; score=0.9
- Line 464: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_out_ptrs_arr, d_out_bufs.data(),
  Confidence: band=very_high; score=0.9
- Line 467: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_sz_arr, h_in_sizes.data(),
  Confidence: band=very_high; score=0.9
- Line 512: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(h_out_sizes.data(), d_out_sz_arr,
  Confidence: band=very_high; score=0.9
- Line 519: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: write_gpu_container_header(results[i].data,
- Line 525: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(results[i].data.data() + hdr,
  Confidence: band=very_high; score=0.9
- Line 567: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: h_in_ptrs[i]  = const_cast<uint8_t*>(d_in) + i * chunk;
- Line 573: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto cuda_alloc = [&](void** ptr, size_t bytes) -> bool {
- Line 573: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto cuda_alloc = [&](void** ptr, size_t bytes) -> bool {
- Line 576: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
  Confidence: band=very_high; score=0.9
- Line 584: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: for (void* p : to_free) cudaFree(p);
  Confidence: band=very_high; score=0.9
- Line 626: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: result.error_message = "cudaMalloc failed for device arrays";
  Confidence: band=very_high; score=0.9
- Line 634: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: result.error_message = "cudaMalloc failed for output chunk";
  Confidence: band=very_high; score=0.9
- Line 640: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_ptrs, h_in_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 643: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_sizes, h_in_sizes.data(),
  Confidence: band=very_high; score=0.9
- Line 646: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_out_ptrs, h_out_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 684: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(h_out_sizes.data(), d_out_sizes,
  Confidence: band=very_high; score=0.9
- Line 685: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: n_chunks * sizeof(size_t), cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 706: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(p, h_out_ptrs[i], h_out_sizes[i],
  Confidence: band=very_high; score=0.9
- Line 706: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: e = cudaMemcpy(p, h_out_ptrs[i], h_out_sizes[i],
- Line 745: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto cuda_alloc = [&](void** ptr, size_t bytes) -> bool {
- Line 745: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto cuda_alloc = [&](void** ptr, size_t bytes) -> bool {
- Line 748: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
  Confidence: band=very_high; score=0.9
- Line 756: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: for (void* p : to_free) cudaFree(p);
  Confidence: band=very_high; score=0.9
- Line 769: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaError_t e = cudaMemcpyAsync(h_in_ptrs[i], chunk_data, cs,
  Confidence: band=very_high; score=0.9
- Line 772: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: spdlog::error("[gpu_compress] cudaMemcpyAsync H2D chunk[{}] failed: {}",
  Confidence: band=very_high; score=0.9
- Line 812: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_ptrs,  h_in_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 815: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_sizes, h_chunk_sizes.data(),
  Confidence: band=very_high; score=0.9
- Line 818: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_out_ptrs,  h_out_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 821: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_out_sizes, h_out_sizes.data(),
  Confidence: band=very_high; score=0.9
- Line 860: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(h_out_sizes.data(), d_out_sizes,
  Confidence: band=very_high; score=0.9
- Line 870: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(result.data() + off, h_out_ptrs[i],
  Confidence: band=very_high; score=0.9
- Line 973: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (data_size == 0) return false;         // empty input always uses CPU
  Confidence: band=very_high; score=0.9
- Line 1190: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto gpu_results = impl_->compress_batch(ptrs, sizes,
- Line 1342: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: static constexpr size_t kLz4HeaderSize = sizeof(uint64_t);
- Line 1353: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: res.error_message = "LZ4: input too large";
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(compress(ptrs[i], sizes[i], algorithm, cfg));
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(compress(ptrs[i], sizes[i], algorithm, cfg));
- Line 374: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_free.push_back(*ptr);
  Confidence: band=high; score=0.74
- Line 375: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_free.push_back(*ptr);
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_free.push_back(*ptr);
  Confidence: band=high; score=0.74
- Line 580: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_free.push_back(*ptr);
- Line 752: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_free.push_back(*ptr);
- Line 1176: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: gpu_indices.push_back(i);
- Line 1184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ptrs.push_back(buffers[idx].data());
  Confidence: band=high; score=0.74
- Line 1185: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ptrs.push_back(buffers[idx].data());
- Line 1186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sizes.push_back(buffers[idx].size());
- Line 1232: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(compress(buf, algorithm));
- Line 1245: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(decompress(compressed_buffers[i], algorithm, orig));
  Confidence: band=high; score=0.74
- Line 1246: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(decompress(compressed_buffers[i], algorithm, orig));

### src/storage/nvme_manager.cpp
Total findings: 73

- Line 223: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int dfd = ::open(probe_path, O_WRONLY | O_DIRECT, 0600);
- Line 290: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring_ && ring_->ring_fd >= 0;
- Line 318: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: struct io_uring_sqe* sqe = &ring->sqes[index];
- Line 322: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->off       = static_cast<uint64_t>(req.offset);
- Line 324: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->len       = static_cast<uint32_t>(req.len);
- Line 325: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->user_data = static_cast<uint64_t>(req.user_data);
- Line 328: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int ret = themis_io_uring_enter(ring->ring_fd, 1, 0,
- Line 358: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: struct io_uring_sqe* sqe = &ring->sqes[index];
- Line 362: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->off       = static_cast<uint64_t>(req.offset);
- Line 364: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->len       = static_cast<uint32_t>(req.len);
- Line 365: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sqe->user_data = static_cast<uint64_t>(req.user_data);
- Line 368: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int ret = themis_io_uring_enter(ring->ring_fd, 1, 0,
- Line 395: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int ret = themis_io_uring_enter(ring->ring_fd, 0, min_complete,
- Line 409: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const struct io_uring_cqe* cqe = &ring->cqes[head & *ring->cq_mask];
- Line 411: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: r.user_data = static_cast<int64_t>(cqe->user_data);
- Line 435: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(config_.device_path.c_str(), O_RDWR);
- Line 437: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: THEMIS_ERROR("NVMeManager::resetZone: open('{}') failed: {}",
- Line 464: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(config_.device_path.c_str(), O_RDWR);
- Line 466: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: THEMIS_ERROR("NVMeManager::finishZone: open('{}') failed: {}",
- Line 493: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int fd = ::open(config_.device_path.c_str(), O_RDONLY);
- Line 593: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = themis_io_uring_setup(config_.io_uring_queue_depth, &params);
- Line 593: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = themis_io_uring_setup(config_.io_uring_queue_depth, &params);
- Line 593: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = themis_io_uring_setup(config_.io_uring_queue_depth, &params);
- Line 593: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = themis_io_uring_setup(config_.io_uring_queue_depth, &params);
- Line 602: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sq_mmap_size = params.sq_off.array +
- Line 604: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sq_mmap = ::mmap(nullptr, ring->sq_mmap_size,
- Line 612: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = -1;
- Line 616: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* sq_base = static_cast<uint8_t*>(ring->sq_mmap);
- Line 619: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sq_mask  = reinterpret_cast<uint32_t*>(sq_base + params.sq_off.ring_mask);
- Line 623: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sqe_mmap_size = params.sq_entries * sizeof(struct io_uring_sqe);
- Line 623: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sqe_mmap_size = params.sq_entries * sizeof(struct io_uring_sqe);
- Line 624: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sqe_mmap = ::mmap(nullptr, ring->sqe_mmap_size,
- Line 634: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = -1;
- Line 637: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sqes = static_cast<struct io_uring_sqe*>(ring->sqe_mmap);
- Line 637: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->sqes = static_cast<struct io_uring_sqe*>(ring->sqe_mmap);
- Line 641: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->cq_mmap_size = params.cq_off.cqes +
- Line 643: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->cq_mmap = ::mmap(nullptr, ring->cq_mmap_size,
- Line 655: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = -1;
- Line 658: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto* cq_base = static_cast<uint8_t*>(ring->cq_mmap);
- Line 661: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->cq_mask = reinterpret_cast<uint32_t*>(cq_base + params.cq_off.ring_mask);
- Line 662: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->cqes    = reinterpret_cast<struct io_uring_cqe*>(cq_base + params.cq_off.cqes);
- Line 688: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (ring->ring_fd >= 0) {
- Line 690: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ring->ring_fd = -1;
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Allocate a report buffer for 1 zone', '    constexpr size_t BUF_SIZE = sizeof(struct blk_zone_report) + sizeof(struct blk_zone);', '    alignas(alignof(struct blk_zone_report)) char buf[BUF_SIZE];', '    std::memset(buf, 0, BUF_SIZE);', '']
  Confidence: band=high; score=0.78
- Line 129: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (initialized_.load(std::memory_order_acquire)) {
- Line 139: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (initialized_.load(std::memory_order_acquire)) {
- Line 231: severity=HIGH; category=posix_only_api
  Description: POSIX-only API unlink( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: ::unlink(probe_path);  // Always clean up, after the O_DIRECT test
- Line 288: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return initialized_.load(std::memory_order_acquire) &&
- Line 325: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: sqe->user_data = static_cast<uint64_t>(req.user_data);
- Line 326: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring->sq_array[index] = index;
- Line 365: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: sqe->user_data = static_cast<uint64_t>(req.user_data);
- Line 366: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring->sq_array[index] = index;
- Line 411: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: r.user_data = static_cast<int64_t>(cqe->user_data);
- Line 602: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring->sq_mmap_size = params.sq_off.array +
- Line 604: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ring->sq_mmap = ::mmap(nullptr, ring->sq_mmap_size,
- Line 604: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring->sq_mmap = ::mmap(nullptr, ring->sq_mmap_size,
- Line 620: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring->sq_array = reinterpret_cast<uint32_t*>(sq_base + params.sq_off.array);
- Line 624: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ring->sqe_mmap = ::mmap(nullptr, ring->sqe_mmap_size,
- Line 624: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring->sqe_mmap = ::mmap(nullptr, ring->sqe_mmap_size,
- Line 643: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ring->cq_mmap = ::mmap(nullptr, ring->cq_mmap_size,
- Line 643: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ring->cq_mmap = ::mmap(nullptr, ring->cq_mmap_size,
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['        if (f.is_open()) {', '            uint32_t count = 1;', '            f >> count;', '            if (count > 0) return count;', '        }']
  Confidence: band=medium; score=0.65
- Line 220: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(tmp_fd);
- Line 226: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(dfd);
- Line 413: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(r);
- Line 447: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 475: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 508: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 552: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd);
- Line 611: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring->ring_fd);
- Line 633: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring->ring_fd);
- Line 654: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring->ring_fd);
- Line 689: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(ring->ring_fd);

### src/storage/blob_redundancy_manager.cpp
Total findings: 61

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['    if (size < 1024 * 1024) {', '        return BlobType::BLOB_SMALL;', '    } else if (size < 100 * 1024 * 1024) {', '        return BlobType::BLOB_MEDIUM;', '    } else {']
  Confidence: band=very_high; score=0.93
- Line 665: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = blobs_.find(blob_id);
- Line 1230: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 645: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: blobs_[blob_id] = metadata;
- Line 684: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& metadata = it->second;
- Line 769: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(healthy_dcs.cbegin(), healthy_dcs.cend(), loc.datacenter)
- Line 822: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& metadata = it->second;
- Line 897: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& metadata = it->second;
- Line 983: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto& metadata = it->second;
- Line 1023: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [blob_id, metadata] : blobs_) {
- Line 1054: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [blob_id, metadata] : blobs_) {
- Line 1068: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [blob_id, metadata] : blobs_) {
- Line 1139: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(tier_candidates.size(), max_tier_ops); ++i) {
- Line 1183: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(healthy_dcs.cbegin(), healthy_dcs.cend(),
- Line 1203: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& id : degraded_ids) {
  Confidence: band=very_high; score=0.9
- Line 1285: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [blob_id, metadata] : blobs_) {
- Line 1296: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // files that have been superseded by new ones, and in those cases the
- Line 1312: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& blob_id : unrecoverable_blob_ids) {
  Confidence: band=very_high; score=0.9
- Line 1350: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(
  Confidence: band=very_high; score=0.9
- Line 1362: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(repair_mutex_);
- Line 1365: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: repair_cv_.wait_for(lock, std::chrono::seconds(5), [this]() {
- Line 1502: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void BlobRedundancyManager::updateMetadataStore([[maybe_unused]] const BlobMetadata& blob) {
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(loc.shard_id);
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: missing.push_back(loc.shard_id);
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locs.push_back(location_to_json(loc));
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: locs.push_back(location_to_json(loc));
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: m.locations.push_back(location_from_json(lj));
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: m.locations.push_back(location_from_json(lj));
- Line 746: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) s += ", ";
  Confidence: band=high; score=0.74
- Line 747: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) s += ", ";
- Line 766: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<std::string> healthy_dcs;
- Line 770: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: healthy_dcs.push_back(loc.datacenter);
  Confidence: band=high; score=0.74
- Line 771: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: healthy_dcs.push_back(loc.datacenter);
- Line 845: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: written_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 846: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: written_shards.push_back(shard_id);
- Line 868: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: written_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 869: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: written_shards.push_back(shard_id);
- Line 987: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deleted_shards.push_back(location.shard_id);
  Confidence: band=high; score=0.74
- Line 988: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: deleted_shards.push_back(location.shard_id);
- Line 1030: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1031: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(blob_id);
- Line 1055: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: degraded.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1056: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: degraded.push_back(blob_id);
- Line 1069: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: critical.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1070: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: critical.push_back(blob_id);
- Line 1086: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: stats.healthy_blobs++;
- Line 1166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: degraded_ids.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: degraded_ids.push_back(blob_id);
- Line 1180: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<std::string> healthy_dcs;
- Line 1184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: healthy_dcs.push_back(loc.datacenter);
  Confidence: band=high; score=0.74
- Line 1185: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: healthy_dcs.push_back(loc.datacenter);
- Line 1193: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: degraded_ids.push_back(blob_id);
- Line 1243: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ss << "# HELP themis_blob_redundancy_healthy_blobs Number of healthy blobs\n";
- Line 1244: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ss << "# TYPE themis_blob_redundancy_healthy_blobs gauge\n";
- Line 1245: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: ss << "themis_blob_redundancy_healthy_blobs " << stats.healthy_blobs << "\n";
- Line 1301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: affected_blob_ids.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: affected_blob_ids.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1302: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: affected_blob_ids.push_back(blob_id);
- Line 1304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: unrecoverable_blob_ids.push_back(blob_id);
- Line 1478: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(location.shard_id);
  Confidence: band=high; score=0.74
- Line 1479: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shards.push_back(location.shard_id);

### src/storage/backup_manager.cpp
Total findings: 56

- Line 1724: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats->rto_seconds = static_cast<uint32_t>(
- Line 2255: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = scheduled_backups_.find(schedule_id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 68: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("db_wrapper cannot be null");
- Line 320: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(src_dir)) {
- Line 723: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(backup_dir)) {
- Line 774: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(checkpoint_dir)) {
- Line 863: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(raid_topology_dir)) {
- Line 962: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: pid_t pid = fork();
- Line 965: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: "fork() failed when invoking tar");
- Line 1002: severity=HIGH; category=windows_only_api
  Description: Windows-only API WaitForSingleObject without platform guard
  Remediation: Wrap in #ifdef _WIN32 ... #endif or provide cross-platform abstraction
  Context: WaitForSingleObject(pi.hProcess, INFINITE);
- Line 1041: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: pid_t pid = fork();
- Line 1044: severity=HIGH; category=posix_only_api
  Description: POSIX-only API fork( without platform guard
  Remediation: Wrap in #ifndef _WIN32 ... #endif or provide Windows alternative
  Context: "fork() failed when invoking tar");
- Line 1074: severity=HIGH; category=windows_only_api
  Description: Windows-only API WaitForSingleObject without platform guard
  Remediation: Wrap in #ifdef _WIN32 ... #endif or provide cross-platform abstraction
  Context: WaitForSingleObject(pi.hProcess, INFINITE);
- Line 1120: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(src_root, ec)) {
- Line 1246: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(src_root, ec)) {
- Line 1652: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(backup_dir)) {
- Line 1731: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(checkpoint_dir)) {
- Line 1899: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //    puts.  This avoids overwriting CFs outside the requested scope.
  Confidence: band=very_high; score=0.9
- Line 1949: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (auto* h : ro_handles) ro_db->DestroyColumnFamilyHandle(h);
- Line 1964: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto dst_handle_result = db_wrapper_->getOrCreateColumnFamily(cf_name);
- Line 1977: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ro_db->NewIterator(ro_opts, src_handle));
- Line 1984: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: batch.Put(dst_handle, it->key(), it->value());
- Line 2099: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(backup_path)) {
- Line 2137: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::recursive_directory_iterator(backup_path)) {
- Line 2519: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(snapshot_dir)) {
- Line 2550: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(snap_base, ec)) {
- Line 54: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static std::string winQuoteForCreateProcess(const std::string& s) {
  Confidence: band=high; score=0.74
- Line 56: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') out += "\\\"";
  Confidence: band=high; score=0.74
- Line 57: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (c == '"') out += "\\\"";
- Line 143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: config.shards.push_back(info);
- Line 235: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards_array.push_back(shard_obj);
  Confidence: band=high; score=0.74
- Line 236: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shards_array.push_back(shard_obj);
- Line 254: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: out.close();
- Line 726: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: backups.push_back(name);
  Confidence: band=high; score=0.74
- Line 727: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backups.push_back(name);
- Line 732: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (filename format ensures correct sort order)
  Confidence: band=high; score=0.74
- Line 884: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: Result<void> BackupManager::isBackupComplete(const std::string& backup_dir,
  Confidence: band=high; score=0.74
- Line 991: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string cmd = "tar -czf " + winQuoteForCreateProcess(compressed_file) +
  Confidence: band=high; score=0.74
- Line 996: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: mutable_cmd.push_back('\0');
- Line 1063: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string cmd = "tar -xzf " + winQuoteForCreateProcess(compressed_file) +
  Confidence: band=high; score=0.74
- Line 1447: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 1557: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 1655: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: full_backups.push_back(name);
  Confidence: band=high; score=0.74
- Line 1656: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: full_backups.push_back(name);
- Line 1886: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) coll_list += ", ";
  Confidence: band=high; score=0.74
- Line 1887: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i) coll_list += ", ";
- Line 1915: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> target_cfs;
  Confidence: band=medium; score=0.66
- Line 1925: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_descriptors.emplace_back(cf_name, rocksdb::ColumnFamilyOptions{});
  Confidence: band=high; score=0.74
- Line 1925: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_descriptors.emplace_back(cf_name, rocksdb::ColumnFamilyOptions{});
  Confidence: band=high; score=0.74
- Line 1925: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_descriptors.emplace_back(cf_name, rocksdb::ColumnFamilyOptions{});
  Confidence: band=high; score=0.74
- Line 2273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(kv.second.schedule_id, kv.second.cron_expression);
  Confidence: band=high; score=0.74
- Line 2553: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.path().string());
  Confidence: band=high; score=0.74
- Line 2554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry.path().string());
- Line 2558: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: std::sort(result.begin(), result.end()); // alphabetical = chronological (timestamps in name)
  Confidence: band=high; score=0.74

### src/storage/wom_tree.cpp
Total findings: 48

- Line 425: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (node_ref->children.size() <= static_cast<size_t>(config.fanout)) {
- Line 843: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: s.leaf_count          = static_cast<uint64_t>(impl_->countLeaves(*impl_->root));
- Line 844: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: s.internal_node_count = static_cast<uint64_t>(impl_->countInternals(*impl_->root));
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        stat_flush_passes.fetch_add(1, std::memory_order_relaxed);', '', '        uint32_t next_depth = depth + 1;', '', '        // Apply ops to each original child.']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            if (child.is_leaf) {', '                // Apply ops directly to leaf.', '                for (auto& op : child_ops[orig_idx]) {', '                    stat_internal_bytes.fetch_add(op.byteSize(),', '                                                  std::memory_order_relaxed);']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['            } else {', "                // Push ops into child's buffer.", '                for (auto& op : child_ops[orig_idx]) {', '                    stat_internal_bytes.fetch_add(op.byteSize(),', '                                                  std::memory_order_relaxed);']
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
- Line 170: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: right->data.assign(leaf.data.begin() + static_cast<ptrdiff_t>(mid),
- Line 221: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::atomic<uint64_t> stat_puts{0};
  Confidence: band=very_high; score=0.9
- Line 269: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: root->buffer_bytes += op.byteSize();
- Line 270: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: root->buffer.push_back(std::move(op));
- Line 415: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node_ref->is_leaf) return false;
- Line 418: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (size_t i = 0; i < node_ref->children.size(); ++i) {
- Line 419: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (doOneInternalSplit(node_ref->children[i], node_ref.get(), i)) {
- Line 425: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (node_ref->children.size() <= static_cast<size_t>(config.fanout)) {
- Line 443: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: parent->pivot_keys.begin() + static_cast<ptrdiff_t>(idx_in_parent),
- Line 446: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: parent->children.begin() + static_cast<ptrdiff_t>(idx_in_parent + 1),
- Line 542: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto it = node->buffer.rbegin(); it != node->buffer.rend(); ++it) {
- Line 649: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("WomTree: fanout must be >= 2");
- Line 652: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("WomTree: leaf_capacity must be >= 2");
- Line 655: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("WomTree: buffer_size_bytes must be > 0");
- Line 684: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: impl_->stat_puts.fetch_add(1, std::memory_order_relaxed);
  Confidence: band=very_high; score=0.9
- Line 784: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = it_begin; it != it_end; ++it) {
  Confidence: band=very_high; score=0.9
- Line 819: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: impl_->stat_puts.store(0, std::memory_order_relaxed);
  Confidence: band=very_high; score=0.9
- Line 834: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: s.total_puts          = impl_->stat_puts.load(std::memory_order_relaxed);
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: root->buffer.push_back(std::move(op));
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: child_ops[idx].push_back(std::move(op));
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: child_ops[idx].push_back(std::move(op));
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: child.buffer.push_back(std::move(op));
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: child.buffer.push_back(std::move(op));
- Line 384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_root->children.push_back(std::move(root));
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_root->children.push_back(std::move(root));
- Line 434: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_root->pivot_keys.push_back(std::move(pivot));
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_root->pivot_keys.push_back(std::move(pivot));
- Line 436: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_root->children.push_back(std::move(root));
- Line 437: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: new_root->children.push_back(std::move(right));
- Line 581: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void collectAllEntries(std::map<std::string, std::string>& out) const {
  Confidence: band=high; score=0.74
- Line 586: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string>& out) const {
  Confidence: band=high; score=0.74

### src/storage/erasure_coder_factory.cpp
Total findings: 46

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        if (offset < data.size()) {', '            const size_t size = std::min(chunk_size, data.size() - offset);', '            std::memcpy(chunk.data(), data.data() + offset, size);', '        }', '        chunks.push_back(std::move(chunk));']
  Confidence: band=very_high; score=0.9
- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['        if (offset < data.size()) {', '            const size_t size = std::min(chunk_size, data.size() - offset);', '            std::memcpy(chunk.data(), data.data() + offset, size);', '        }', '        chunks.push_back(std::move(chunk));']
  Confidence: band=very_high; score=0.9
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    uint32_t parity_shards', ') {', '    const size_t chunk_size = (data.size() + data_shards - 1) / data_shards;', '    std::vector<std::vector<uint8_t>> chunks;', '    chunks.reserve(data_shards + parity_shards);']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    uint32_t parity_shards', ') {', '    const size_t chunk_size = (data.size() + data_shards - 1) / data_shards;', '    std::vector<std::vector<uint8_t>> chunks;', '    chunks.reserve(data_shards + parity_shards);']
  Confidence: band=high; score=0.81
- Line 25: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Too many shards: rows + cols must be <= 255");
- Line 113: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const uint8_t coeff = vandermonde[parity_row][data_row];
- Line 134: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 140: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Not enough chunks for recovery");
- Line 169: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: full_matrix[data_shards + row] = vandermonde[row];
- Line 187: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to invert decode matrix for Reed-Solomon recovery");
- Line 202: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: recovered_data[row][byte] = recovered_bytes[row];
- Line 308: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Too many shards: rows + cols must be <= 256");
- Line 325: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid Cauchy matrix: x[i] == y[j]");
- Line 432: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: parity_byte ^= gf_mul(cauchy_matrix[parity_row][data_row], chunks[data_row][byte]);
- Line 449: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 455: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Not enough chunks for recovery");
- Line 487: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: full_matrix[data_shards + row][col] = cauchy_matrix[row][col];
- Line 508: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to invert decode matrix");
- Line 522: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: recovered_data[row][byte] = recovered_bytes[row];
- Line 38: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Context: bool ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
  Confidence: band=medium; score=0.56
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(chunk));
- Line 120: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(parity));
- Line 128: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 175: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(index);
  Confidence: band=high; score=0.74
- Line 176: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: available_indices.push_back(index);
- Line 214: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Context: uint8_t ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 230: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_inv(uint8_t a)
  Context: uint8_t ReedSolomonCoder::gf_inv(uint8_t a) {
  Confidence: band=medium; score=0.56
- Line 245: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_div(uint8_t a, uint8_t b)
  Context: uint8_t ReedSolomonCoder::gf_div(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 249: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp)
  Context: uint8_t ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp) {
  Confidence: band=medium; score=0.56
- Line 273: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Context: uint8_t CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 289: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_inv(uint8_t a)
  Context: uint8_t CauchyReedSolomonCoder::gf_inv(uint8_t a) {
  Confidence: band=medium; score=0.56
- Line 350: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Context: bool CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
  Confidence: band=medium; score=0.56
- Line 422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 423: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(chunk));
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 435: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 436: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(parity));
- Line 443: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 494: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(index);
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: available_indices.push_back(index);
- Line 534: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ErasureCoder::create(ErasureCodingAlgorithm algorithm)
  Context: std::unique_ptr<ErasureCoder> ErasureCoder::create(ErasureCodingAlgorithm algorithm) {
  Confidence: band=medium; score=0.56

### src/storage/tt_quantizer.cpp
Total findings: 45

- Line 46: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<QuantizedCore> QuantizedCore::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 120: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<QuantizedTrain> QuantizedTrain::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 149: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto oc = QuantizedCore::deserialize(cb);
  Confidence: band=very_high; score=0.99
- Line 167: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 16 > array 0
  Remediation: Fix loop condition or increase array size
  Context: float best_dist = std::abs(v - kNF4Table[0]);
- Line 193: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float q = core.data[i] / qc.scale;
- Line 196: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: qc.data[i] = static_cast<uint8_t>(static_cast<int8_t>(qi));
- Line 232: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float normalised = (core.data[i] - qc.mean) / qc.scale;
- Line 252: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int8_t qi = static_cast<int8_t>(qc.data[i]);
- Line 253: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: core.data[i] = static_cast<float>(qi) * qc.scale;
- Line 266: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint8_t byte_val = qc.data[i / 2];
- Line 269: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: core.data[i] = nf4_val * qc.scale + qc.mean;
- Line 299: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TTQuantizer::quantize: empty TTTrain");
- Line 320: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint32_t u; std::memcpy(&u, &core.data[i], 4);
- Line 322: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: qc.data[i*4+j] = static_cast<uint8_t>((u >> (j*8)) & 0xFF);
- Line 357: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: u |= static_cast<uint32_t>(qc.data[i*4+j]) << (j*8);
- Line 358: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::memcpy(&core.data[i], &u, 4);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['            qc.data[i / 2] = idx & 0x0F;', '        else', '            qc.data[i / 2] |= (idx << 4) & 0xF0;', '    }', '    return qc;']
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    for (std::size_t i = 0; i < nelems; ++i) {', '        uint8_t byte_val = qc.data[i / 2];', '        uint8_t idx = (i % 2 == 0) ? (byte_val & 0x0F) : ((byte_val >> 4) & 0x0F);', '        float nf4_val = kNF4Table[idx];', '        core.data[i] = nf4_val * qc.scale + qc.mean;']
  Confidence: band=medium; score=0.62
- Line 31: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (int i = 0; i < 8; ++i) out.push_back((v >> (i*8)) & 0xFF);
- Line 34: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 4; ++i) out.push_back((u >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 35: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (int i = 0; i < 4; ++i) out.push_back((u >> (i*8)) & 0xFF);
- Line 38: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>(quant_type));
  Confidence: band=high; score=0.74
- Line 39: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>(quant_type));
- Line 73: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 8; ++i) out.push_back((v >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 8; ++i) out.push_back((v >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (int i = 0; i < 8; ++i) out.push_back((v >> (i*8)) & 0xFF);
- Line 102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 8; ++i) out.push_back((u >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (int i = 0; i < 8; ++i) out.push_back((u >> (i*8)) & 0xFF);
- Line 107: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>(quant_type));
  Confidence: band=high; score=0.74
- Line 107: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>(quant_type));
  Confidence: band=high; score=0.74
- Line 108: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>(quant_type));
- Line 155: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qt.cores.push_back(std::move(qc));
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qt.cores.push_back(std::move(qc));
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qt.cores.push_back(std::move(qc));
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: qt.cores.push_back(std::move(qc));
- Line 328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: qt.cores.push_back(quantizeINT8(core));
- Line 331: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: qt.cores.push_back(quantizeNF4(core));
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train.cores.push_back(std::move(core));
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train.cores.push_back(std::move(core));
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train.cores.push_back(std::move(core));
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: train.cores.push_back(std::move(core));
- Line 364: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: train.cores.push_back(dequantizeINT8(qc));
- Line 367: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: train.cores.push_back(dequantizeNF4(qc));

### src/storage/hierarchical_tucker_decomposer.cpp
Total findings: 43

- Line 250: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::unique_ptr<HTNode> deserializeNode(Reader& r) {
  Confidence: band=very_high; score=0.99
- Line 258: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->rank = static_cast<std::size_t>(rank_u);
- Line 265: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->mode_index = static_cast<std::size_t>(mi);
- Line 266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->n_k        = static_cast<std::size_t>(nk);
- Line 273: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->r_left  = static_cast<std::size_t>(rl);
- Line 274: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->r_right = static_cast<std::size_t>(rr);
- Line 276: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: node->left  = deserializeNode(r);
  Confidence: band=very_high; score=0.99
- Line 277: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: node->right = deserializeNode(r);
  Confidence: band=very_high; score=0.99
- Line 297: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<HTTrain> HTTrain::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 320: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: ht.root = deserializeNode(r);
  Confidence: band=very_high; score=0.99
- Line 616: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: leaf_left->U          = U_cache[L];
- Line 624: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: leaf_right->U          = U_cache[L + 1];
- Line 709: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->left = buildHTNode(G_left, left_shape, L, M, U_cache, T_shape);
- Line 715: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: node->right = buildHTNode(G_right, right_shape, M, R, U_cache, T_shape);
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
- Line 96: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return node.U;  // already stored as [n_k × rank] row-major
- Line 122: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fl == 0.0f) continue;
  Confidence: band=very_high; score=0.9
- Line 145: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // toTTTrain — compatibility bridge with memoization (stub #286 resolved)
  Confidence: band=high; score=0.8
- Line 267: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!r.readFloats(node->U)) return nullptr;
- Line 275: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!r.readFloats(node->B)) return nullptr;
- Line 472: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: mat[j * N_other + col] = data[flat];
- Line 505: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: val += U[ik * r + alpha] * data[o * n_k * stride_k + ik * stride_k + s];
- Line 553: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   SVD-1: unfold core along [L..M-1] vs [M..R-1, out]
- Line 554: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *          → G_left  [phys_L,...,phys_{M-1}, r_inner]
- Line 555: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *          → G_right_raw  [n_right * r_out, r_inner]
- Line 556: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   SVD-2: unfold G_right_raw as [n_right, r_out * r_inner]
- Line 557: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *          → G_right  [phys_M,...,phys_{R-1}, r_23]
- Line 558: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *          → B_node   [r_inner, r_23, r_out]  (transfer tensor at this node)
- Line 728: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (d < 2) throw std::invalid_argument("HTDecomposer: need at least 2 modes");
- Line 732: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (s == 0) throw std::invalid_argument("HTDecomposer: mode size 0 is invalid");
- Line 736: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HTDecomposer: data.size() != product of shape");
- Line 752: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto Tk = modeKUnfolding(data, shape, k);  // [nk × n_other]
- Line 707: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_shape.push_back(r_inner);
  Confidence: band=high; score=0.74
- Line 707: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_shape.push_back(r_inner);
  Confidence: band=high; score=0.74
- Line 707: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_shape.push_back(r_inner);
  Confidence: band=high; score=0.74
- Line 708: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: left_shape.push_back(r_inner);
- Line 895: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: root_core_shape.push_back(1);  // r_out = 1 at root
  Confidence: band=high; score=0.74
- Line 896: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: root_core_shape.push_back(1);  // r_out = 1 at root

### src/storage/simd_filter.cpp
Total findings: 40

- Line 57: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: int v = cached.load(std::memory_order_relaxed);
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i));
- Line 195: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 196: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i));
- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 252: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i));
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 281: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i));
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 320: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i));
- Line 386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 387: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 391: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
- Line 418: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 419: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 423: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 424: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
- Line 450: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 455: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 456: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
- Line 482: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 483: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint32_t>(i + lane));
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 488: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
- Line 666: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint32_t>(i));
- Line 691: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(idx + row_offset);
  Confidence: band=high; score=0.74
- Line 691: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(idx + row_offset);
  Confidence: band=high; score=0.74
- Line 692: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(idx + row_offset);

### src/storage/tensor_train_decomposer.cpp
Total findings: 39

- Line 169: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<TTTrain> TTTrain::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 174: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: if (pos + 8 > bytes.size()) throw std::runtime_error("TTTrain::deserialize: underflow");
  Confidence: band=very_high; score=0.99
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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 174: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (pos + 8 > bytes.size()) throw std::runtime_error("TTTrain::deserialize: underflow");
- Line 590: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TensorTrainDecomposer: need at least 2 modes");
- Line 595: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TensorTrainDecomposer: data.size() != product(mode_sizes)");
- Line 621: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("TensorTrainDecomposer: invalid unfolding shape in decompose");
- Line 797: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (float v : res.cores[0].data) norm_sq += static_cast<double>(v) * v;
- Line 865: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TTTrain::innerProduct: incompatible mode_sizes");
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((u >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>((u >> (i*8)) & 0xFF));
- Line 209: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/database_connection_manager.cpp
Total findings: 34

- Line 92: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 105: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: spdlog::info("Created new connection (total: {})", total + 1);
- Line 105: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: spdlog::info("Created new connection (total: {})", total + 1);
- Line 145: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = active_connections_.find(conn.get());
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Check active connections (just update health check time)', '    for (auto& [ptr, conn] : active_connections_) {', '        auto& health = connection_health_[ptr];', '        if (conn->isValid()) {', '            health.last_health_check = std::chrono::system_clock::now();']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    // Active connections will be reconnected when released', '    for (auto& [ptr, conn] : active_connections_) {', '        auto& health = connection_health_[ptr];', '        health.state = ConnectionState::RECONNECTING;', '    }']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 43: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: DatabaseConnectionManager::acquireConnection(
- Line 52: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: spdlog::warn("Circuit breaker open - cannot acquire connection");
- Line 56: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 124: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 212: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [ptr, conn] : active_connections_) {
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [ptr, conn] : active_connections_) {
- Line 213: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& health = connection_health_[ptr];
- Line 242: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [ptr, health] : connection_health_) {
  Confidence: band=very_high; score=0.9
- Line 242: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& [ptr, health] : connection_health_) {
- Line 260: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: DatabaseConnectionManager::getConnectionHealth() const {
- Line 266: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [ptr, health] : connection_health_) {
  Confidence: band=very_high; score=0.9
- Line 292: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [ptr, conn] : active_connections_) {
  Confidence: band=very_high; score=0.9
- Line 292: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [ptr, conn] : active_connections_) {
- Line 293: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& health = connection_health_[ptr];
- Line 311: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [ptr, conn] : active_connections_) {
- Line 80: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();
- Line 171: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();
- Line 189: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::queue<std::shared_ptr<Connection>> healthy_connections;
- Line 199: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();
- Line 266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: health_list.push_back(health);
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: health_list.push_back(health);
- Line 288: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();
- Line 307: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();
- Line 312: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: conn->close();

### src/storage/distributed_transaction_manager.cpp
Total findings: 34

- Line 306: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // reference that outlives any concurrent unregisterShard() call.
  Confidence: band=very_high; score=0.99
- Line 59: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 76: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 77: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "DistributedTransaction [" + txn_id_ + "]: unknown shard '" + shard_id + "'"
- Line 88: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 89: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "DistributedTransaction [" + txn_id_ + "]: put() called on non-ACTIVE transaction"
- Line 108: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 109: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "DistributedTransaction [" + txn_id_ + "]: del() called on non-ACTIVE transaction"
- Line 127: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<std::string> DistributedTransaction::get(std::string_view key) {
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return participant->get(logical_key);
  Confidence: band=very_high; score=0.9
- Line 136: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool DistributedTransaction::commit() {
  Confidence: band=very_high; score=0.9
- Line 144: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("DistributedTransaction [{}]: commit() called in unexpected state", txn_id_);
- Line 149: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("DistributedTransaction [{}]: Phase 1 — PREPARE to {} shard(s)",
- Line 155: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [shard_id, ops] : pending_ops_) {
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lk(mgr_state_->shards_mutex);
- Line 162: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' no longer registered during prepare",
- Line 172: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: voted_commit = participant->prepare(txn_id_, ops);
- Line 174: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' prepare threw: {}",
- Line 191: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_DEBUG("DistributedTransaction [{}]: Phase 2 — COMMIT to {} shard(s)",
- Line 202: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' commit threw: {}",
- Line 208: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransaction [{}]: COMMITTED across {} shard(s)",
- Line 220: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort threw: {}",
- Line 226: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransaction [{}]: ABORTED (prepare phase failed)", txn_id_);
- Line 247: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort (rollback) threw: {}",
  Confidence: band=very_high; score=0.9
- Line 247: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort (rollback) threw: {}",
- Line 252: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_INFO("DistributedTransaction [{}]: rolled back", txn_id_);
- Line 287: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& sc : shards) {
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DistributedTransactionManager: shard_id must not be empty");
- Line 302: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DistributedTransactionManager: participant must not be null");
- Line 308: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: state_->shards[shard_id] = std::shared_ptr<IDistributedShardParticipant>(
- Line 308: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: state_->shards[shard_id] = std::shared_ptr<IDistributedShardParticipant>(
- Line 315: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(state_->shards_mutex);
- Line 262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(shard_id);

### src/storage/index_maintenance.cpp
Total findings: 27

- Line 35: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("db_wrapper cannot be null");
- Line 351: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [job_id, status] : active_jobs_) {
  Confidence: band=very_high; score=0.9
- Line 423: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 424: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::milliseconds(policy_.time_based_interval_ms),
- Line 582: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto s = db->CompactRange(options, nullptr, nullptr);
- Line 582: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto s = db->CompactRange(options, nullptr, nullptr);
- Line 631: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto s = db->CompactRange(options, nullptr, nullptr);
- Line 631: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto s = db->CompactRange(options, nullptr, nullptr);
- Line 673: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 706: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto s = db->CompactRange(options, nullptr, nullptr);
- Line 706: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto s = db->CompactRange(options, nullptr, nullptr);
- Line 106: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 129: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus job_status;
- Line 179: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 202: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus job_status;
- Line 252: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 279: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 306: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 350: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: std::vector<MaintenanceJobStatus> jobs;
- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jobs.push_back(status);
  Confidence: band=high; score=0.74
- Line 353: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: jobs.push_back(status);
- Line 500: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { metrics.file_count = std::stoull(file_count_str); } catch (...) {}
- Line 602: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status.after_metrics.fragmentation_percentage);
- Line 651: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status.after_metrics.fragmentation_percentage);
- Line 844: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: MaintenanceJobStatus status;
- Line 860: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: status.job_id, status.index_name);
- Line 878: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: errors::ErrorCode::ERR_INDEX_REBUILD_FAILED, reindex_status.message);

### src/storage/storage_audit_logger.cpp
Total findings: 26

- Line 54: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: static int themis_open_fd(const char* path, int flags, int mode) { return ::open(path, flags, mode);
- Line 58: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return ::write(fd, data, len);
- Line 113: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: StorageAuditLogger::open(const Config& config) {
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
- Line 145: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(config_.dir)) {
- Line 272: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 47: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static int themis_close_fd(int fd) { return _close(fd); }
- Line 55: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static int themis_close_fd(int fd) { return ::close(fd); }
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: found.push_back(std::stoull(m[1].str()));
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: found.push_back(std::stoull(m[1].str()));
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: segments_.push_back(new_id);
- Line 175: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: Result<void> StorageAuditLogger::log(Event event,
  Confidence: band=medium; score=0.6
- Line 184: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::PUT, key, extra);
  Confidence: band=medium; score=0.6
- Line 189: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::DEL, key, extra);
  Confidence: band=medium; score=0.6
- Line 193: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::CHECKPOINT, "", detail);
  Confidence: band=medium; score=0.6
- Line 197: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::RECOVERY, "", detail);
  Confidence: band=medium; score=0.6
- Line 201: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::COMPACTION, "", detail);
  Confidence: band=medium; score=0.6
- Line 205: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::SNAPSHOT, "", detail);
  Confidence: band=medium; score=0.6

### src/storage/wal_storage.cpp
Total findings: 25

- Line 57: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: static int themis_open_fd(const char* path, int flags, int mode) { return ::open(path, flags, mode);
- Line 61: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return ::write(fd, data, len);
- Line 119: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Fix loop condition or increase array size
  Context: buf[0] = static_cast<uint8_t>(v);
- Line 120: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 1
  Remediation: Fix loop condition or increase array size
  Context: buf[1] = static_cast<uint8_t>(v >> 8);
- Line 121: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 2
  Remediation: Fix loop condition or increase array size
  Context: buf[2] = static_cast<uint8_t>(v >> 16);
- Line 122: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 3
  Remediation: Fix loop condition or increase array size
  Context: buf[3] = static_cast<uint8_t>(v >> 24);
- Line 132: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Fix loop condition or increase array size
  Context: return static_cast<uint32_t>(buf[0])
- Line 133: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 1
  Remediation: Fix loop condition or increase array size
  Context: | (static_cast<uint32_t>(buf[1]) << 8)
- Line 134: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 2
  Remediation: Fix loop condition or increase array size
  Context: | (static_cast<uint32_t>(buf[2]) << 16)
- Line 135: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 3
  Remediation: Fix loop condition or increase array size
  Context: | (static_cast<uint32_t>(buf[3]) << 24);
- Line 179: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: Result<std::unique_ptr<WALStorage>> WALStorage::open(
- Line 326: severity=CRITICAL; category=missing_dtor
  Description: Class stat allocates resources but has no destructor
  Remediation: Add explicit destructor: ~stat() { /* cleanup */ }
  Context: class/struct stat
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 206: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(config_.dir)) {
- Line 478: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint64_t sid : segments_) {
  Confidence: band=very_high; score=0.9
- Line 483: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint64_t sid : to_remove) {
  Confidence: band=very_high; score=0.9
- Line 498: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 50: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static int themis_close_fd(int fd) { return _close(fd); }
- Line 58: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: static int themis_close_fd(int fd) { return ::close(fd); }
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments_.push_back(sid);
  Confidence: band=high; score=0.74
- Line 210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: segments_.push_back(sid);
- Line 237: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments_.push_back(1);
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: segments_.push_back(1);
- Line 479: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(sid);
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_remove.push_back(sid);

### src/storage/gguf_metadata.cpp
Total findings: 24

- Line 78: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "[ThemisDB][SECURITY] GGUFMetadata: HMAC input exceeds INT_MAX; "
  Confidence: band=very_high; score=0.99
- Line 347: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool GGUFMetadata::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 64: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[nodiscard]] std::string toHex(const unsigned char* data, size_t len) {
- Line 68: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: oss << std::setw(2) << static_cast<unsigned int>(data[i]);
- Line 73: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[nodiscard]] std::string computeHmacSha256(const std::string& data,
- Line 77: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 78: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "[ThemisDB][SECURITY] GGUFMetadata: HMAC input exceeds INT_MAX; "
  Confidence: band=very_high; score=0.9
- Line 264: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: "[ThemisDB][SECURITY] GGUFMetadata::sign: injected HmacFn returned "
- Line 270: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: "[ThemisDB][SECURITY] GGUFMetadata::sign: injected HmacFn threw; "
- Line 281: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 300: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 360: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint32_t i = 0; i < count; ++i) {
  Confidence: band=very_high; score=0.9
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>(v >>  0));
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v >>  0));
- Line 136: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v >>  8));
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v >> 16));
- Line 138: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v >> 24));
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(k);
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(k);
- Line 269: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 306: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/columnar_cache.cpp
Total findings: 23

- Line 101: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = store_.find(key);
- Line 107: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lit = lru_map_.find(key);
- Line 132: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = store_.find(key);
- Line 140: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lit = lru_map_.find(key);
- Line 170: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = store_.find(key);
- Line 177: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lit = lru_map_.find(key);
- Line 201: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = store_.begin(); it != store_.end(); ) {
- Line 205: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = lru_map_.find(it->first);
- Line 205: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lit = lru_map_.find(it->first);
- Line 205: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lit = lru_map_.find(it->first);
- Line 280: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator sit may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto sit = store_.find(k);
- Line 33: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: case SegmentDType::Int64:  return n * sizeof(int64_t) + n;   // data + null bitmap
- Line 218: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& k : evicted_keys) on_evict_cb(k);
  Confidence: band=very_high; score=0.9
- Line 234: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [k, e] : store_) {
  Confidence: band=very_high; score=0.9
- Line 241: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mu_);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 55: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: release();
- Line 66: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: release();
- Line 76: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void PinGuard::release() noexcept {
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (on_evict_cb) evicted_keys.push_back(it->first);
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (on_evict_cb) evicted_keys.push_back(it->first);
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (cfg_.on_evict) to_notify.push_back(k);
  Confidence: band=high; score=0.74
- Line 283: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (cfg_.on_evict) to_notify.push_back(k);

### src/storage/history_manager.cpp
Total findings: 23

- Line 107: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<HistoryRecord> HistoryManager::deserializeHistoryRecord(std::string_view data) {
  Confidence: band=very_high; score=0.99
- Line 209: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return deserializeHistoryRecord(it.value());
  Confidence: band=very_high; score=0.99
- Line 216: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto rec = deserializeHistoryRecord(val);
  Confidence: band=very_high; score=0.99
- Line 274: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<ConflictRecord> ConflictManager::deserializeConflictRecord(std::string_view data) {
  Confidence: band=very_high; score=0.99
- Line 315: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return deserializeConflictRecord(
  Confidence: band=very_high; score=0.99
- Line 323: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto rec = deserializeConflictRecord(val);
  Confidence: band=very_high; score=0.99
- Line 344: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<ConflictSet> ConflictManager::deserializeConflictSet(std::string_view data) {
  Confidence: band=very_high; score=0.99
- Line 379: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return deserializeConflictSet(
  Confidence: band=very_high; score=0.99
- Line 387: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto set = deserializeConflictSet(val);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2766 [storage/transaction] Atomic History/Conflict Layer for MVCCStore a... (2026-03-11T17:25
- Line 67: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HistoryManager: db cannot be null");
- Line 215: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&](std::string_view /*key*/, std::string_view val) -> bool {
- Line 235: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ConflictManager: db cannot be null");
- Line 322: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix("conflict:", [&](std::string_view /*key*/, std::string_view val) -> bool {
- Line 386: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix("conflictset:", [&](std::string_view /*key*/, std::string_view val) -> bool {
- Line 42: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hi = hex[i];
  Confidence: band=high; score=0.74
- Line 43: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto lo = hex[i + 1];
  Confidence: band=high; score=0.74
- Line 49: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((nibble(hi) << 4) | nibble(lo)));
  Confidence: band=high; score=0.74
- Line 50: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<uint8_t>((nibble(hi) << 4) | nibble(lo)));
- Line 118: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 288: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 355: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/blob_backend_s3.cpp
Total findings: 22

- Line 120: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto input_stream = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
  Confidence: band=very_high; score=0.99
- Line 121: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_stream->write(reinterpret_cast<const char*>(data.data()), data.size());
  Confidence: band=very_high; score=0.99
- Line 121: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: input_stream->write(reinterpret_cast<const char*>(data.data()), data.size());
- Line 122: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: request.SetBody(input_stream);
  Confidence: band=very_high; score=0.99
- Line 59: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
  Confidence: band=very_high; score=0.9
- Line 115: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetBucket(bucket_);
- Line 116: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetKey(s3_key);
- Line 117: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetServerSideEncryption(Aws::S3::Model::ServerSideEncryption::AES256);
- Line 120: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input_stream = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
  Confidence: band=very_high; score=0.9
- Line 121: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_stream->write(reinterpret_cast<const char*>(data.data()), data.size());
  Confidence: band=very_high; score=0.9
- Line 122: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: request.SetBody(input_stream);
  Confidence: band=very_high; score=0.9
- Line 122: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetBody(input_stream);
- Line 123: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetContentLength(data.size());
- Line 158: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetBucket(bucket_);
- Line 159: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetKey(s3_key);
- Line 186: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: char buffer[4096];
- Line 216: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetBucket(bucket_);
- Line 217: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetKey(s3_key);
- Line 228: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: "S3 delete failed: " + error.GetMessage()
- Line 243: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetBucket(bucket_);
- Line 244: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.SetKey(s3_key);
- Line 228: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "S3 delete failed: " + error.GetMessage()

### src/storage/compression_strategy.cpp
Total findings: 22

- Line 464: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = mapping.find(str);
- Line 596: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = value_to_index.find(value);
- Line 185: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(size, size_t(100)); ++i) {
- Line 502: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint8_t value = data[i];
- Line 506: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: while (i + run_length < size && data[i + run_length] == value) {
- Line 552: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: result.push_back(data[0]);
- Line 556: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: int16_t delta = static_cast<int16_t>(data[i]) - static_cast<int16_t>(data[i-1]);
- Line 595: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = value_to_index.find(value);
- Line 595: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: uint8_t value = data[i];
- Line 474: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(static_cast<uint8_t>(value | 0x80));
- Line 477: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: output.push_back(static_cast<uint8_t>(value));
- Line 512: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(value);
- Line 557: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint8_t>(delta & 0xFF));
- Line 570: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(data[0]);
- Line 575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(value);
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(value);
- Line 599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dictionary.push_back(value);
  Confidence: band=high; score=0.74
- Line 600: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dictionary.push_back(value);
- Line 602: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: indices.push_back(idx);
- Line 604: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: indices.push_back(it->second);
- Line 643: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(dictionary[idx]);
  Confidence: band=high; score=0.74
- Line 644: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(dictionary[idx]);

### src/storage/erasure_coding_backend.cpp
Total findings: 22

- Line 167: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto recovered = coder_->decode(chunk_map, missing, k, m);
- Line 257: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator chunk_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto  chunk_it = chunks.find(shard_index);
- Line 36: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 79: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 91: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: raw_chunks = coder_->encode(data, k, m);
- Line 113: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: s.data          = std::move(raw_chunks[i]);
- Line 137: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 156: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: chunk_map[idx] = shard.data;
- Line 161: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (chunk_map.find(i) == chunk_map.end()) {
- Line 161: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (chunk_map.find(i) == chunk_map.end()) {
- Line 194: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& s : shards) {
  Confidence: band=very_high; score=0.9
- Line 222: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [idx, chunk] : entry.chunks) {
  Confidence: band=very_high; score=0.9
- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shards.push_back(std::move(s));
- Line 130: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, EncodedShard>& shards,
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<uint32_t, std::vector<uint8_t>> chunk_map;
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(i);
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(i);
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(i);
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: missing.push_back(i);
- Line 221: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<uint32_t, EncodedShard> shard_map;
  Confidence: band=high; score=0.74

### src/storage/online_schema_migration.cpp
Total findings: 22

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
- Line 39: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("SchemaMigrator: max_ops must be > 0");
- Line 150: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function migrate without trace point
  Context: MigrationResult SchemaMigrator::migrate()
  Confidence: band=very_high; score=0.9
- Line 172: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(tables.begin(), tables.end(), op.table_name) == tables.end()) {
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tables.push_back(op.table_name);
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tables.push_back(op.table_name);
- Line 183: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back(msg);
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back(msg);
- Line 243: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back(msg);
- Line 333: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.properties.push_back(std::move(prop));
  Confidence: band=high; score=0.74
- Line 334: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schema.properties.push_back(std::move(prop));
- Line 495: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schema.indexes.push_back(std::move(idx));
- Line 579: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.properties.push_back(std::move(meta));
  Confidence: band=high; score=0.74
- Line 580: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: schema.properties.push_back(std::move(meta));

### src/storage/base_entity.cpp
Total findings: 21

- Line 106: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return field_cache_->find(std::string(field_name)) != field_cache_->end();
- Line 111: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = field_cache_->find(std::string(field_name));
- Line 112: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != field_cache_->end()) {
- Line 255: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator last may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto last = token.find_last_not_of(" \t");
- Line 626: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity BaseEntity::deserialize(std::string_view pk, const Blob& blob) {
  Confidence: band=very_high; score=0.99
- Line 249: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy comma-separated fallback.
  Confidence: band=high; score=0.8
- Line 381: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("JSON parse failed");
- Line 386: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("JSON parse failed: root is not an object");
- Line 421: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("JSON parse failed");
- Line 507: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Unknown type tag encountered while parsing BaseEntity binary blob");
- Line 515: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Binary parse failed");
- Line 167: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 236: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(el.get<std::string>());
  Confidence: band=high; score=0.74
- Line 237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(el.get<std::string>());
- Line 239: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(el.dump());
- Line 262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(token));
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(static_cast<float>(dres.value_unsafe()));
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(static_cast<float>(dres.value_unsafe()));
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(static_cast<float>(elem.get<double>()));
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(static_cast<float>(elem.get<double>()));
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec.push_back(static_cast<float>(elem.get<double>()));

### src/storage/concurrent_write_controller.cpp
Total findings: 20

- Line 115: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: WriteGuard ConcurrentWriteController::acquire() {
- Line 123: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: "ConcurrentWriteController: acquire() called after shutdown");
- Line 152: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: f.wait();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 80: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: , acquire_timeout_(config.acquire_timeout) {
- Line 115: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: WriteGuard ConcurrentWriteController::acquire() {
- Line 123: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "ConcurrentWriteController: acquire() called after shutdown");
- Line 131: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: total_acquired_.fetch_add(1, std::memory_order_relaxed);
- Line 138: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 149: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (acquire_timeout_.count() > 0) {
- Line 150: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: got_slot = (f.wait_for(acquire_timeout_) == std::future_status::ready);
- Line 150: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: got_slot = (f.wait_for(acquire_timeout_) == std::future_status::ready);
  Confidence: band=very_high; score=0.9
- Line 178: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "ConcurrentWriteController: acquire() timed out");
- Line 274: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: s.total_acquired = total_acquired_.load(std::memory_order_relaxed);
- Line 40: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: release();
- Line 50: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: release();
- Line 57: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void WriteGuard::release() noexcept {
- Line 106: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {}
- Line 160: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 225: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { next.set_value(); } catch (...) {}

### src/storage/tensor_network_storage_engine.cpp
Total findings: 20

- Line 59: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = store_.find(key);
- Line 162: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return (it != version_cache_.end()) ? it->second : 0;
- Line 195: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return QuantizedTrain::deserialize(*meta);
  Confidence: band=very_high; score=0.99
- Line 32: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (unsigned char c : s) {
  Confidence: band=very_high; score=0.9
- Line 89: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RocksDBTensorBackend: db must not be null");
- Line 114: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix,
- Line 134: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TensorNetworkStorageEngine: backend must not be null");
- Line 208: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TensorNetworkStorageEngine::put: size mismatch");
- Line 224: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [raw_train, _] = decomposer_.decompose(data, mode_sizes, raw_cfg);
- Line 239: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::size_t k = 0; k < qtrain.cores.size(); ++k)
  Confidence: band=very_high; score=0.9
- Line 320: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::size_t k = 0; k < oqt->cores.size(); ++k)
  Confidence: band=very_high; score=0.9
- Line 344: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto keys = backend_->listKeys(makePrefix(key));
- Line 411: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto raw_keys = backend_->listKeys(raw_prefix);
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(kv.first);
- Line 251: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* observer must not throw; swallow exceptions */ }
- Line 332: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* observer must not throw; swallow exceptions */ }
- Line 353: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { /* ignore parse errors */ }
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: logical_keys.push_back(raw_key.substr(raw_prefix.size()));
  Confidence: band=high; score=0.74
- Line 416: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: logical_keys.push_back(raw_key.substr(raw_prefix.size()));

### src/storage/ggml_tensor_bridge.cpp
Total findings: 18

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 223: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: raw = storage->getVersion(key, static_cast<std::size_t>(version));
- Line 258: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.total_bytes_mapped += raw->size() * sizeof(float);
- Line 144: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Pretend to be a ggml_tensor for pointer compatibility in tests.
  Confidence: band=high; score=0.8
- Line 174: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!impl_ || !impl_->valid) return nullptr;
- Line 178: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // not for llama.cpp inference until a real allocator is injected).
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return impl_ ? &impl_->train : nullptr;
- Line 183: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_ ? &impl_->train : nullptr;
- Line 187: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return impl_ ? &impl_->key : nullptr;
- Line 187: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_ ? &impl_->key : nullptr;
- Line 216: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle.impl_->key = key;
- Line 234: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle.impl_->fake_tensor.data = *raw;
- Line 234: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: handle.impl_->fake_tensor.data = *raw;
- Line 235: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle.impl_->fake_tensor.n_elements = raw->size();
- Line 246: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle.impl_->real_ggml_tensor =
- Line 247: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: alloc_fn_copy(handle.impl_->fake_tensor.n_elements);
- Line 251: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle.impl_->valid = true;

### src/storage/storage_engine.cpp
Total findings: 18

- Line 264: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: Result<void> StorageEngine::open(const std::string& db_path) {
- Line 277: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!rocksdb_->open()) {
- Line 26: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // They are provided for testing, development, and backward compatibility only.
  Confidence: band=high; score=0.8
- Line 71: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error(
- Line 86: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 117: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 242: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("StorageEngine: evaluator cannot be null");
- Line 245: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("StorageEngine: encryption cannot be null");
- Line 248: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("StorageEngine: key_provider cannot be null");
- Line 303: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t cur = m.load(std::memory_order_relaxed);
- Line 304: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: while (v < cur && !m.compare_exchange_weak(cur, v, std::memory_order_relaxed))
- Line 308: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t cur = m.load(std::memory_order_relaxed);
- Line 309: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: while (v > cur && !m.compare_exchange_weak(cur, v, std::memory_order_relaxed))
- Line 401: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: key = nullptr;
  Context: "Failed to delete key: " + key);
- Line 489: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: rocksdb_->scanPrefix(prefix,
- Line 287: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void StorageEngine::close() {
- Line 293: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: rocksdb_->close();
- Line 401: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "Failed to delete key: " + key);

### src/storage/encrypted_blob_backend.cpp
Total findings: 17

- Line 59: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("EncryptedBlobBackend: inner backend must not be null");
- Line 62: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("EncryptedBlobBackend: key provider must not be null");
- Line 93: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto raw = inner_->get(ref);
- Line 113: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return inner_->remove(ref);
- Line 122: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return inner_->exists(ref);
- Line 165: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EncryptedBlobBackend: RAND_bytes failed");
- Line 175: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EncryptedBlobBackend: EVP_CIPHER_CTX_new failed");
- Line 201: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EncryptedBlobBackend: EVP_EncryptFinal_ex failed");
- Line 207: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EncryptedBlobBackend: EVP_CTRL_GCM_GET_TAG failed");
- Line 229: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 244: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EncryptedBlobBackend: EVP_CIPHER_CTX_new failed");
- Line 271: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("EncryptedBlobBackend: EVP_CTRL_GCM_SET_TAG failed");
- Line 282: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 210: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 211: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 276: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 286: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/hlc.cpp
Total findings: 17

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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 51: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t cur = state_.load(std::memory_order_relaxed);
- Line 128: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return HLCTimestamp{state_.load(std::memory_order_acquire)};
- Line 80: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: HybridLogicalClock::now()
  Context: HLCTimestamp HybridLogicalClock::now() {
  Confidence: band=medium; score=0.56

### src/storage/tensor_compaction_filter.cpp
Total findings: 15

- Line 110: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize raw TTTrain
  Confidence: band=very_high; score=0.99
- Line 115: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto opt = TTTrain::deserialize(bytes);
  Confidence: band=very_high; score=0.99
- Line 141: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize QuantizedTrain header
  Confidence: band=very_high; score=0.99
- Line 146: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto opt = QuantizedTrain::deserialize(bytes);
  Confidence: band=very_high; score=0.99
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
- Line 180: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: new_bytes->assign(reinterpret_cast<const char*>(new_serial.data()),
- Line 155: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 175: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/tensor_router.cpp
Total findings: 15

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 211: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Force-LIFT for inference-bound data when policy says so
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (policy.force_lift_for_inference && hint.inference_use) {
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("TensorRouter: storage engine must not be null");
- Line 393: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto pilot    = impl_->runPilot(data, mode_sizes);
- Line 405: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"force_lift_for_inference",     impl_->policy.force_lift_for_inference},
  Confidence: band=very_high; score=0.9
- Line 411: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"inference_use", hint.inference_use},
  Confidence: band=very_high; score=0.9
- Line 458: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(impl_->template_apply_mu);
- Line 464: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->template_topology_apply_fn = nullptr;
- Line 464: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->template_topology_apply_fn = nullptr;
- Line 196: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 191: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double log_n = std::log(static_cast<double>(n_pilot));
  Confidence: band=medium; score=0.6
- Line 192: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double log_r = std::log(static_cast<double>(res.pilot_rank));
  Confidence: band=medium; score=0.6
- Line 451: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: void TensorRouter::setTemplateCatalog(
  Confidence: band=medium; score=0.6
- Line 473: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: TensorRouter::templateCatalog() const noexcept {
  Confidence: band=medium; score=0.6

### src/storage/nlp_metadata_extractor.cpp
Total findings: 14

- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.keywords.push_back(keywords[i].text);
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.keywords.push_back(keywords[i].text);
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: meta.keywords.push_back(keywords[i].text);
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.emails.push_back(entity.text);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: meta.emails.push_back(entity.text);
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: meta.urls.push_back(entity.text);
- Line 92: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: meta.dates.push_back(entity.text);
- Line 94: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: meta.measurements.push_back(entity.text);
- Line 188: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keywords.push_back(kw.text);
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keywords.push_back(kw.text);
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result[entity.type].push_back(entity.text);
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result[entity.type].push_back(entity.text);
- Line 365: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/compressed_storage.cpp
Total findings: 13

- Line 41: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<CompressedValue> CompressedValue::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 49: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Fix loop condition or increase array size
  Context: result.method = static_cast<compression::CompressionMethod>(bytes[0]);
- Line 104: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize
  Confidence: band=very_high; score=0.99
- Line 105: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto cv = CompressedValue::deserialize(*serialized);
  Confidence: band=very_high; score=0.99
- Line 185: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize
  Confidence: band=very_high; score=0.99
- Line 186: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto cv = CompressedValue::deserialize(*serialized);
  Confidence: band=very_high; score=0.99
- Line 206: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto decompressed = compressor->decompress(cv->data, cv->method);
- Line 223: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& pair : column_compressors_) {
  Confidence: band=very_high; score=0.9
- Line 31: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((size >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 32: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(static_cast<uint8_t>((size >> (i * 8)) & 0xFF));
- Line 223: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "Column: " + pair.first + "\n";
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "Column: " + pair.first + "\n";
- Line 226: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += "\n";

### src/storage/security_signature_manager.cpp
Total findings: 13

- Line 141: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(&hex_output[i * 2], 3, "%02x", static_cast<unsigned int>(digest[i]));
  Confidence: band=very_high; score=0.9
- Line 53: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 76: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 88: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: signatures.push_back(*sig);
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: signatures.push_back(*sig);
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: signatures.push_back(*sig);
- Line 141: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: snprintf(&hex_output[i * 2], 3, "%02x", static_cast<unsigned int>(digest[i]));
- Line 146: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 169: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 196: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.failed_resource_ids.push_back(sig->resource_id);
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.failed_resource_ids.push_back(sig->resource_id);

### src/storage/zero_copy_blob_transfer.cpp
Total findings: 13

- Line 74: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return ::write(fd, data, len);
- Line 120: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: fd_ = ::open(file_path.c_str(), O_RDONLY);
- Line 126: severity=CRITICAL; category=missing_dtor
  Description: Class stat allocates resources but has no destructor
  Remediation: Add explicit destructor: ~stat() { /* cleanup */ }
  Context: class/struct stat
- Line 292: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: int src_fd = ::open(source_path.c_str(), O_RDONLY);
- Line 484: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: part_stream->write(part_buf.data(), this_part);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    int64_t file_size = static_cast<int64_t>(fs::file_size(source_path));', '    if (length == 0) {', '        length = file_size - offset;', '    }', '    if (length <= 0 || offset < 0 || offset >= file_size) {']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 484: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: part_stream->write(part_buf.data(), this_part);
- Line 128: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 138: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 187: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(fd_);
- Line 309: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(src_fd);
- Line 322: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ::close(src_fd);

### src/storage/index_analyzer.cpp
Total findings: 12

- Line 170: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("IndexAnalyzer: db_wrapper must not be null");
- Line 245: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : snapshot) {
  Confidence: band=very_high; score=0.9
- Line 340: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 348: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: cv_.wait_for(lock, std::chrono::minutes(1),
  Confidence: band=very_high; score=0.9
- Line 348: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::minutes(1),
- Line 358: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: cv_.wait_for(lock, std::chrono::seconds(60),
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.indices.push_back(std::move(ie));
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cfg.indices.push_back(std::move(ie));
- Line 256: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reports.push_back(std::move(report));
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: reports.push_back(std::move(report));
- Line 431: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { l0_files = std::stoull(l0_str); } catch (...) {}
- Line 487: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/merge_operators.cpp
Total findings: 12

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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73

### src/storage/mvcc_store.cpp
Total findings: 12

- Line 98: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it == latest_ts_map_.end() || ts.value >= it->second.value) {
- Line 98: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it == latest_ts_map_.end() || ts.value >= it->second.value) {
- Line 33: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("MVCCStore: db cannot be null");
- Line 235: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&](std::string_view vkey, std::string_view raw_val) -> bool {
- Line 259: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_->scanPrefix(prefix, [&](std::string_view vkey, std::string_view) -> bool {
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 176: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: seek_key.push_back('\x01');
- Line 268: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Keys are already in ascending timestamp order (big-endian sort).
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: uint64_t num_to_delete = 0;
- Line 282: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: num_to_delete = std::min(num_to_delete, max_deletable);
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: base_keys.emplace_back(bk);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: base_keys.emplace_back(vkey.data(), vkey.size() - 9);
  Confidence: band=high; score=0.74

### src/storage/pitr_manager.cpp
Total findings: 12

- Line 105: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto snapshot = snapshot_mgr_->getTag(tag_name);
- Line 207: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto snapshot = snapshot_mgr_->getTag(tag_name);
- Line 35: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("PITRManager: db cannot be null");
- Line 38: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("PITRManager: changefeed cannot be null");
- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("PITRManager: snapshot_mgr cannot be null");
- Line 165: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: std::find(options.tables.begin(), options.tables.end(), table) != options.tables.end()) {
- Line 285: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(options.tables.begin(), options.tables.end(), table) == options.tables.end()) {
- Line 335: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: key = nullptr;
  Context: return Status::Error("Failed to delete key: " + event.key);
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: preview.affected_keys.push_back(event.key);
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: preview.affected_keys.push_back(event.key);
- Line 311: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: event.sequence, status.message);
- Line 335: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: return Status::Error("Failed to delete key: " + event.key);

### src/storage/tiered_storage.cpp
Total findings: 12

- Line 51: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = entries_.find(key);
- Line 84: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Key contains path traversal sequence: " + key);
- Line 93: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Key is a current-directory reference: " + key);
- Line 297: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: source = nullptr;
  Context: THEMIS_WARN("TieredStorage: migrateKey({}) copied but could not delete source", key);
- Line 381: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock lock(worker_mutex_);
- Line 89: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: while (!trimmed.empty() && (trimmed.front() == '/' || trimmed.front() == '\\')) {
- Line 89: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: while (!trimmed.empty() && (trimmed.front() == '/' || trimmed.front() == '\\')) {
- Line 99: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: safe += (c == '/' || c == '\\' || c == ':' || c == '*' ||
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: safe += (c == '/' || c == '\\' || c == ':' || c == '*' ||
- Line 100: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: safe += (c == '/' || c == '\\' || c == ':' || c == '*' ||
- Line 191: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_WARN("TieredStorage: failed to delete '{}': {}", path, ec.message());
- Line 297: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_WARN("TieredStorage: migrateKey({}) copied but could not delete source", key);

### src/storage/adaptive_compaction.cpp
Total findings: 10

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
- Line 89: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(sample_mutex_);
- Line 90: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: sample_cv_.wait_for(lock, config_.sample_interval,
  Confidence: band=very_high; score=0.9
- Line 104: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t reads  = window_reads_.exchange(0, std::memory_order_relaxed);
- Line 105: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: uint64_t writes = window_writes_.exchange(0, std::memory_order_relaxed);
- Line 159: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: ? std::log(current_write_amp) / std::log(config_.urgent_write_amp_threshold)
  Confidence: band=medium; score=0.6

### src/storage/blob_backend_gcs.cpp
Total findings: 10

- Line 91: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (impl_->prefix.empty()) {
- Line 95: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::string pfx = impl_->prefix;
- Line 104: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[maybe_unused]] const std::vector<uint8_t>& data) {
- Line 114: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto writer = impl_->client->WriteObject(impl_->bucket, obj);
- Line 129: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ref.uri        = "gs://" + impl_->bucket + "/" + obj;
- Line 154: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto reader = impl_->client->ReadObject(impl_->bucket, obj);
- Line 214: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto status = impl_->client->DeleteObject(impl_->bucket, obj);
- Line 219: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: "GCS delete failed: " + status.message());
- Line 239: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto metadata = impl_->client->GetObjectMetadata(impl_->bucket, obj);
- Line 219: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: "GCS delete failed: " + status.message());

### src/storage/hamming_coder.cpp
Total findings: 10

- Line 85: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const uint32_t shard_size = static_cast<uint32_t>(available_chunks.begin()->second.size());
- Line 91: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto it = available_chunks.find(s);
- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HammingCoder::encode: shard counts must be > 0");
- Line 44: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HammingCoder::encode: data must not be empty");
- Line 61: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& parity = shards[data_shards + p];
- Line 81: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HammingCoder::decode: no chunks available");
- Line 90: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto it = available_chunks.find(s);
- Line 93: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 182: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 76: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74

### include/storage/examples/schema_layout_advisor_example.cpp
Total findings: 9

- Line 176: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: std::cout << "\n  [PLANNED — StorageLayoutAdvisor::adviseLayout() — IMPL-B10]\n"
- Line 89: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "  " << f.field_name
- Line 104: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "  " << candidate.field_name
- Line 131: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "    " << f.field_name << " → " << rec << "\n";
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  Write rate: " << ts_profile.rows_per_day_write_rate << " rows/day\n"
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  Read/write: " << ts_profile.read_write_ratio * 100.0 << " % reads\n"
- Line 161: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  Read/write: " << ts_profile.read_write_ratio * 100.0 << " % reads\n"
- Line 201: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  Expected RETAIN:         3  (order_status, customer_email/GDPR, seasonal)\n"
- Line 206: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\nSee docs/issues/optimization_layers/ for implementation specs.\n";

### src/storage/blob_backend_azure.cpp
Total findings: 9

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #746 [Phase 4] Storage Layer: Migrate error handling to Result<T> pattern (2026-03-11T18:06:57
- Line 81: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: service_client.GetBlobContainerClient(container_name_)
- Line 116: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto response = blob_client.Upload(stream, options);
- Line 149: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto response = blob_client.Download();
- Line 203: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: blob_client.Delete();
- Line 209: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: THEMIS_ERROR("Azure delete failed: {}", e.what());
- Line 212: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: failed = nullptr;
  Context: "Azure delete failed: " + std::string(e.what())
- Line 227: severity=HIGH; category=no_retry_logic
  Description: grpc_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto properties = blob_client.GetProperties();

### src/storage/streaming_ingest_manager.cpp
Total findings: 9

- Line 47: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: new StreamingIngestManager(std::move(db), std::move(cfg)));
- Line 261: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 271: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 38: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("StreamingIngestManager: db cannot be null");
- Line 203: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: while (running_.load(std::memory_order_acquire)) {
- Line 204: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(mu_);
- Line 207: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: not_empty_.wait_for(lock, cfg_.flush_interval, [this] {
  Confidence: band=very_high; score=0.9
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buffer_.push_back({std::string(key), std::string(value)});
- Line 179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buffer_.push_back(std::move(ev));

### src/storage/disk_space_monitor.cpp
Total findings: 6

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 228: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 233: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 619: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: size_t pos = path.find_last_of("/\\");

### src/storage/storage_parquet_exporter.cpp
Total findings: 6

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    size_t n = seg.metadata().row_count;', '    // rawData() invariant: raw.size() == n * element_size (1 for BOOL).', '    size_t packed_bytes = (n + 7) / 8;', '    std::vector<uint8_t> values(packed_bytes, 0);', '    for (size_t i = 0; i < n; ++i) {']
  Confidence: band=high; score=0.78
- Line 269: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: raw.data() + n * sizeof(int64_t));
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>((u >> s) & 0xFF));
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((u >> s) & 0xFF));
- Line 88: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(type);
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(T_STOP);

### src/storage/blob_backend_webdav.cpp
Total findings: 5

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #746 [Phase 4] Storage Layer: Migrate error handling to Result<T> pattern (2026-03-11T18:06:57
- Line 44: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: vec->insert(vec->end(), static_cast<uint8_t*>(ptr), static_cast<uint8_t*>(ptr) + total);
- Line 44: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vec->insert(vec->end(), static_cast<uint8_t*>(ptr), static_cast<uint8_t*>(ptr) + total);
- Line 62: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::memcpy(ptr, rd->data + rd->offset, to_copy);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65

### src/storage/vector_index_backend.cpp
Total findings: 5

- Line 33: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("VectorIndexConfig::dim must be > 0");
- Line 46: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 70: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({id, dist, toScore(dist)});
  Confidence: band=high; score=0.74
- Line 91: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({id, dist, toScore(dist)});

### src/storage/blob_backend_filesystem.cpp
Total findings: 4

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #746 [Phase 4] Storage Layer: Migrate error handling to Result<T> pattern (2026-03-11T18:06:57
- Line 42: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Invalid blob_id: too short");
- Line 75: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ofs.close();
- Line 154: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/compaction_manager.cpp
Total findings: 4

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 125: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(bg_mutex_);
- Line 185: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // writes are memtable flush outputs; L1+ writes are compaction outputs.
  Confidence: band=very_high; score=0.9
- Line 220: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}

### src/storage/raft_mvcc_bridge.cpp
Total findings: 4

- Line 108: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: HLCTimestamp RaftMvccBridge::raftAwareWrite(
  Confidence: band=very_high; score=0.99
- Line 34: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RaftMvccBridge: mvcc_store cannot be null");
- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("RaftMvccBridge: coordinator cannot be null");
- Line 100: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: RaftMvccBridge::snapshotRead(std::string_view key, HLCTimestamp ts) {
  Confidence: band=very_high; score=0.9

### src/storage/transaction_retry_manager.cpp
Total findings: 4

- Line 180: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: BackoffStrategy strategy = policy ? policy->backoff_strategy : config_.backoff_strategy;
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 312: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/key_schema.cpp
Total findings: 3

- Line 124: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Fallback for legacy keys without prefixes
  Confidence: band=high; score=0.8
- Line 125: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Assume DOCUMENT for backward compatibility (was more common in early versions)
  Confidence: band=high; score=0.8
- Line 136: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // If no separator, return the entire key (edge case/legacy)
  Confidence: band=high; score=0.8

### src/storage/security_signature.cpp
Total findings: 3

- Line 60: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<SecuritySignature> SecuritySignature::deserialize(const std::string& data) {
  Confidence: band=very_high; score=0.99
- Line 51: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 64: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/storage/schema_dead_weight_detector.cpp
Total findings: 2

- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: signal.push_back(static_cast<double>(count));
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: signal.push_back(static_cast<double>(count));

### src/storage/storage_layout_advisor.cpp
Total findings: 2

- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diffs.push_back(ts[i] - ts[i - 1]);
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: diffs.push_back(ts[i] - ts[i - 1]);

### src/storage/mvcc_chain_pruner.cpp
Total findings: 1

- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_versions.push_back({e.timestamp, e.value});

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
