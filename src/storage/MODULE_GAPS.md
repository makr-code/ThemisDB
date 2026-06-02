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
| W1-S19 | (simd_filter remediation: detectSIMDLevel cache load/store upgraded to acquire/release semantics; SIMD/scalar kernels now pre-reserve output capacity and scanBatch pre-reserves aggregated row capacity to reduce missing-reserve/copy-overhead hotspots) | simd_filter.cpp | High memory_order finding addressed and medium performance reserve hotspots reduced across scalar/AVX2/NEON paths |
| W1-S20 | (HIGH false-positive annotation pass across 10 files: distributed_transaction_manager.cpp — extended uninitialized_access annotation (lines 149-252), lock_contention/no_retry_logic per-iteration shard copy, deadlock_risk/null_dereference in registerShard; tt_quantizer.cpp — pointer_arithmetic vector-element accesses and uncaught_exception for unsupported quant type; hlc.cpp — memory_order relaxed CAS-loop initialisation and db_connection_leak scanner artifact; backup_manager.cpp — range_temporary annotation extended to lines 2519/2550; storage_audit_logger.cpp — deadlock_risk sequential mutex acquisitions; pitr_manager.cpp — uncaught_exception constructor preconditions, repeated_search small-list find, delete_no_nullptr method call; hamming_coder.cpp — uncaught_exception preconditions, pointer_arithmetic shard index, o_n_squared inherent parity algorithm, uninitialized_access vector<bool>; tiered_storage.cpp — uncaught_exception sanitizeKey, delete_no_nullptr deleteFromTier method, lock_contention workerLoop wait_for; mvcc_store.cpp — uncaught_exception constructor null check, null_dereference string_view reinterpret_cast; streaming_ingest_manager.cpp — uncaught_exception factory precondition, db_connection_leak scanner artifact, lock_contention condition-variable worker) | distributed_transaction_manager.cpp, tt_quantizer.cpp, hlc.cpp, backup_manager.cpp, storage_audit_logger.cpp, pitr_manager.cpp, hamming_coder.cpp, tiered_storage.cpp, mvcc_store.cpp, streaming_ingest_manager.cpp | All remaining actionable HIGH findings annotated as reviewed false positives; no genuine bugs introduced |
| W1-S22 | (final annotation/remediation pass: nlp_metadata_extractor — add reserve() for keywords loop + annotate entity-dispatch push_backs; security_signature_manager — annotate snprintf hex-encode hardcoded_output FP + narrow all catch(...) to std::exception; storage_parquet_exporter — annotate Line-0 phantom + size_assumption FP for sizeof(int64_t); blob_backend_filesystem — annotate Line-7 header-comment scanner artifact + precondition throw + narrow catch; key_schema — annotate legacy_duplication FPs for intentional backward-compat paths; schema_dead_weight_detector + storage_layout_advisor — confirm stale reserve findings already fixed; mvcc_chain_pruner — annotate push_back inside scanVersions callback) | nlp_metadata_extractor.cpp, security_signature_manager.cpp, storage_parquet_exporter.cpp, blob_backend_filesystem.cpp, key_schema.cpp, schema_dead_weight_detector.cpp, storage_layout_advisor.cpp, mvcc_chain_pruner.cpp | All remaining unannotated HIGH/MEDIUM findings reviewed; 1 genuine reserve() fix applied; all other findings annotated as reviewed false positives |
| W1-S23 | (storage perf hotpath follow-up: backup_manager winQuoteForCreateProcess now pre-reserves and appends without loop concatenation, restoreCollections precomputes requested-collection log buffer capacity; compressed_storage stats/message builders now use append/reserve patterns; history_manager append_crc32 pre-reserves trailer bytes) | backup_manager.cpp, compressed_storage.cpp, history_manager.cpp | Reduced string_concat_loop/copy_overhead allocation churn in issue-scope storage utility paths with behavior preserved |
| W1-S24 | (storage perf follow-up: compression_strategy RLE and dictionary codecs now pre-reserve output buffers for worst-case/heuristic growth and dictionary working sets to reduce push_back reallocation churn in encode/decode loops) | compression_strategy.cpp | Reduced missing-reserve/copy_overhead pressure in issue-scope codec hotpaths with behavior preserved |
| W1-S25 | (final annotation pass across 6 remaining files: raft_mvcc_bridge — annotate uncaught_exception constructor preconditions (lines 33/36) and unspecified_consistency snapshotRead (line 100); transaction_retry_manager — annotate 2 Line-0 phantom uncategorized findings + narrow catch(...) to std::exception in invokeAlertCallback; compaction_manager — annotate lock_contention cv::wait_for pattern (line 125), hardcoded_output scanner misread of comment text (line 185), narrow catch(...) to std::exception for stats parsing; security_signature — annotate model_integrity_gap with explicit validation evidence (isValidResourceId/isHexLowerString/isSupportedAlgorithm) + annotate already-specific catch(std::exception&) at lines 51/64; blob_backend_webdav — annotate Line-7 header-comment uninitialized_access artifact + Line-0 uncategorized ReadData phantom; vector_index_backend — annotate missing_vector_reserve/copy_overhead findings as stale since reserve() already precedes push_back loop) | raft_mvcc_bridge.cpp, transaction_retry_manager.cpp, compaction_manager.cpp, security_signature.cpp, blob_backend_webdav.cpp, vector_index_backend.cpp | All remaining unannotated findings reviewed and annotated as reviewed false positives or false-alarm scanner artifacts; all genuine fixes (catch narrowing) applied |
| W1-S26 | (storage audit writer hardening: writeEntry now handles EINTR and partial writes with a full write loop before advancing sequence/segment counters) | storage_audit_logger.cpp | Eliminated partial-write data loss risk and improved robustness of local audit-log persistence path |
| W1-S27 | (nvme fallback I/O hardening: submitRead/submitWrite synchronous paths now retry on EINTR and require full transfer completion; added regression tests for short-read failure and full-buffer fallback writes) | nvme_manager.cpp, test_nvme_manager.cpp | Eliminated partial-transfer acceptance risk in non-io_uring fallback path and added focused regression coverage for strict transfer semantics |
| W1-S28 | (annotation + genuine-fix pass across 6 files; all false positives explicitly identified with reasoning): **hierarchical_tucker_decomposer.cpp** — uninitialized_access (expandNode return), fp_exact_comparison (fl==0.0f guard), legacy_duplication (toTTTrain bridge comment), model_integrity_gap (CRC32 verified path in deserializeNode/HTTrain::deserialize), null_dereference (readFloats after CRC guard), pointer_arithmetic (standard C++ iterators in modeKUnfolding/modeKProduct), and 2× Line-0 phantom FP — all **false positives**. **columnar_cache.cpp** — size_assumption (sizeof(int64_t)==8 target guarantee), lock_in_loop/deadlock_risk (clear() runs callback outside lock; pinnedCount() accesses shared range under single lock) — all **false positives**. **wom_tree.cpp** — null_dereference in splitLeaf (right->data.front() guaranteed non-empty by branch), Line-0 phantom — **false positives**. **database_connection_manager.cpp** — db_connection_leak/lock_contention in acquireConnection and getConnectionHealth (no resource opened; single lock held throughout), lock_in_loop in getStats (mutex held for full loop), pointer_arithmetic on structured-binding loops — all **false positives**. **ggml_tensor_bridge.cpp** — CRITICAL Line-0 uncategorized ×2 (phantom scanner artifacts score=0.85), null_dereference on handle.impl_ (assigned by make_unique immediately above) — all **false positives**. **tensor_compaction_filter.cpp** — Line-0 HIGH uncategorized ×8 (phantom), pointer_arithmetic (reinterpret_cast+bounded size) — **false positives**; `catch(...)` narrowed to `catch(std::exception&)` at lines 155 and 175 — **genuine fix** (prevented non-std exception types from being silently swallowed). | hierarchical_tucker_decomposer.cpp, columnar_cache.cpp, wom_tree.cpp, database_connection_manager.cpp, ggml_tensor_bridge.cpp, tensor_compaction_filter.cpp | All unannotated CRITICAL/HIGH findings across 6 files reviewed, explicitly classified as false positives with reasoning, and annotated inline; one genuine bug fixed (catch narrowing in tensor_compaction_filter) |
| W1-S29 | (exception-safety hardening pass: replace broad catch(...) handlers with typed std::exception/std::future_error handling in quantization, tensor routing pilot path, GGUF HMAC injection path, RocksDB stats parsing paths, and concurrent-write promise signaling) | tt_quantizer.cpp, tensor_router.cpp, gguf_metadata.cpp, index_maintenance.cpp, index_analyzer.cpp, concurrent_write_controller.cpp | Reduced generic exception swallowing in issue-scope storage paths while preserving fail-closed behavior and fallback semantics |
| W1-S30 | (exception-safety hardening follow-up: narrow broad catch(...) handlers in entity parsing, history/conflict deserialization, TT train deserialization, and NLP metadata serialization/parsing paths to typed std::exception handling) | base_entity.cpp, history_manager.cpp, tensor_train_decomposer.cpp, nlp_metadata_extractor.cpp | Further reduced generic exception swallowing in issue-scope storage paths while preserving existing nullopt/false fallback behavior |
| W1-S31 | (exception-safety hardening + cleanup RAII follow-up: encrypted_blob_backend now uses unique_ptr-managed EVP_CIPHER_CTX in encrypt/decrypt to remove generic cleanup catch blocks; concurrent_write_controller acquire() future-get rejection paths narrowed to std::exception; rocksdb_wrapper merge fallback and backup-count probe catches narrowed to std::exception) | encrypted_blob_backend.cpp, concurrent_write_controller.cpp, rocksdb_wrapper.cpp | Reduced additional generic catch(...) usage in issue-scope storage paths while preserving fallback/rethrow behavior |

## Scan Snapshot

- Module: storage
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 331
- Actionable Findings (Critical + High): 132
- Affected Files: 61

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 59 |
| High | 73 |
| Medium | 187 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| performance_patterns | 168 |
| container | 136 |
| exception_safety | 120 |
| reliability | 108 |
| concurrency | 106 |
| llm_ai_safety | 79 |
| platform | 64 |
| memory | 56 |
| raii | 54 |
| security | 44 |
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
| src/storage/gpu_compression.cpp | 46 | 10 | 31 | 5 | 0 |
| src/storage/columnar_format.cpp | 35 | 11 | 8 | 16 | 0 |
| src/storage/erasure_coder_factory.cpp | 21 | 0 | 0 | 21 | 0 |
| src/storage/simd_filter.cpp | 20 | 0 | 0 | 20 | 0 |
| src/storage/backup_manager.cpp | 17 | 0 | 1 | 16 | 0 |
| src/storage/blob_redundancy_manager.cpp | 16 | 0 | 0 | 16 | 0 |
| src/storage/rocksdb_wrapper.cpp | 16 | 0 | 6 | 10 | 0 |
| src/storage/tt_quantizer.cpp | 16 | 3 | 0 | 13 | 0 |
| src/storage/history_manager.cpp | 12 | 9 | 0 | 3 | 0 |
| src/storage/hierarchical_tucker_decomposer.cpp | 11 | 5 | 2 | 4 | 0 |
| src/storage/wom_tree.cpp | 10 | 0 | 4 | 6 | 0 |
| src/storage/storage_audit_logger.cpp | 8 | 0 | 0 | 1 | 7 |
| src/storage/tensor_router.cpp | 8 | 0 | 4 | 0 | 4 |
| src/storage/compressed_storage.cpp | 7 | 5 | 0 | 2 | 0 |
| src/storage/distributed_transaction_manager.cpp | 7 | 1 | 5 | 1 | 0 |
| src/storage/erasure_coding_backend.cpp | 7 | 0 | 0 | 7 | 0 |
| src/storage/blob_backend_s3.cpp | 6 | 3 | 3 | 0 | 0 |
| src/storage/tensor_train_decomposer.cpp | 6 | 2 | 0 | 4 | 0 |
| src/storage/base_entity.cpp | 5 | 1 | 0 | 4 | 0 |
| src/storage/gguf_metadata.cpp | 5 | 2 | 1 | 2 | 0 |
| src/storage/nlp_metadata_extractor.cpp | 5 | 0 | 0 | 5 | 0 |
| src/storage/online_schema_migration.cpp | 5 | 0 | 1 | 4 | 0 |
| src/storage/tensor_compaction_filter.cpp | 4 | 4 | 0 | 0 | 0 |
| src/storage/compression_strategy.cpp | 3 | 0 | 0 | 3 | 0 |
| src/storage/index_analyzer.cpp | 3 | 0 | 1 | 2 | 0 |
| src/storage/mvcc_store.cpp | 3 | 0 | 0 | 3 | 0 |
| src/storage/tensor_network_storage_engine.cpp | 3 | 1 | 0 | 2 | 0 |
| src/storage/wal_storage.cpp | 3 | 0 | 0 | 3 | 0 |
| src/storage/columnar_cache.cpp | 2 | 0 | 0 | 2 | 0 |
| src/storage/ggml_tensor_bridge.cpp | 2 | 0 | 2 | 0 | 0 |
| src/storage/raft_mvcc_bridge.cpp | 2 | 1 | 1 | 0 | 0 |
| src/storage/security_signature_manager.cpp | 2 | 0 | 0 | 2 | 0 |
| src/storage/adaptive_compaction.cpp | 1 | 0 | 0 | 0 | 1 |
| src/storage/compaction_manager.cpp | 1 | 0 | 1 | 0 | 0 |
| src/storage/database_connection_manager.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/hamming_coder.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/hlc.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/index_maintenance.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/key_schema.cpp | 1 | 0 | 1 | 0 | 0 |
| src/storage/pitr_manager.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/schema_dead_weight_detector.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/security_signature.cpp | 1 | 1 | 0 | 0 | 0 |
| src/storage/storage_engine.cpp | 1 | 0 | 1 | 0 | 0 |
| src/storage/storage_layout_advisor.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/storage_parquet_exporter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/tiered_storage.cpp | 1 | 0 | 0 | 1 | 0 |
| src/storage/vector_index_backend.cpp | 1 | 0 | 0 | 1 | 0 |
| include/storage/examples/schema_layout_advisor_example.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/blob_backend_azure.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/blob_backend_filesystem.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/blob_backend_gcs.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/blob_backend_webdav.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/concurrent_write_controller.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/disk_space_monitor.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/encrypted_blob_backend.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/merge_operators.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/mvcc_chain_pruner.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/nvme_manager.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/streaming_ingest_manager.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/transaction_retry_manager.cpp | 0 | 0 | 0 | 0 | 0 |
| src/storage/zero_copy_blob_transfer.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/storage/gpu_compression.cpp
Total findings: 46

- Line 154: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: /// Parse the header of a GPU container.  Returns false on malformed input.
  Confidence: band=very_high; score=0.99
- Line 276: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Upload input to device
  Confidence: band=very_high; score=0.99
- Line 280: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.error_message = std::string("cudaMalloc input: ") +
  Confidence: band=very_high; score=0.99
- Line 297: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_in
  Context: static_cast<uint8_t*>(d_in), size, cfg, result,
  Confidence: band=very_high; score=0.99
- Line 347: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Each input buffer is treated as a single nvCOMP chunk.
  Confidence: band=very_high; score=0.99
- Line 381: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // --- Step 1: Upload all input buffers ---
  Confidence: band=very_high; score=0.99
- Line 624: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: result.error_message = "cudaMalloc failed for device arrays";
  Confidence: band=very_high; score=0.99
- Line 632: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: result.error_message = "cudaMalloc failed for output chunk";
  Confidence: band=very_high; score=0.99
- Line 971: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (data_size == 0) return false;         // empty input always uses CPU
  Confidence: band=very_high; score=0.99
- Line 1351: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: res.error_message = "LZ4: input too large";
  Confidence: band=very_high; score=0.99
- Line 154: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: /// Parse the header of a GPU container.  Returns false on malformed input.
  Confidence: band=very_high; score=0.9
- Line 276: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Upload input to device
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: result.error_message = std::string("cudaMalloc input: ") +
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.error_message = std::string("cudaMalloc input: ") +
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_in);
  Confidence: band=very_high; score=0.9
- Line 288: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: result.error_message = std::string("cudaMemcpyAsync H2D: ") +
  Confidence: band=very_high; score=0.9
- Line 313: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_in);
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Each input buffer is treated as a single nvCOMP chunk.
  Confidence: band=very_high; score=0.9
- Line 369: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
  Confidence: band=very_high; score=0.9
- Line 377: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: for (void* p : to_free) cudaFree(p);
  Confidence: band=very_high; score=0.9
- Line 381: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // --- Step 1: Upload all input buffers ---
  Confidence: band=very_high; score=0.9
- Line 392: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: spdlog::error("[gpu_compress] cudaMemcpyAsync H2D[{}] failed: {}",
  Confidence: band=very_high; score=0.9
- Line 459: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_ptrs_arr, d_in_bufs.data(),
  Confidence: band=very_high; score=0.9
- Line 462: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_out_ptrs_arr, d_out_bufs.data(),
  Confidence: band=very_high; score=0.9
- Line 523: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(results[i].data.data() + hdr,
  Confidence: band=very_high; score=0.9
- Line 574: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
  Confidence: band=very_high; score=0.9
- Line 582: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: for (void* p : to_free) cudaFree(p);
  Confidence: band=very_high; score=0.9
- Line 624: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: result.error_message = "cudaMalloc failed for device arrays";
  Confidence: band=very_high; score=0.9
- Line 632: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: result.error_message = "cudaMalloc failed for output chunk";
  Confidence: band=very_high; score=0.9
- Line 638: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_ptrs, h_in_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 644: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_out_ptrs, h_out_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 683: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: n_chunks * sizeof(size_t), cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 746: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::error("[gpu_compress] cudaMalloc({}) failed: {}",
  Confidence: band=very_high; score=0.9
- Line 754: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: for (void* p : to_free) cudaFree(p);
  Confidence: band=very_high; score=0.9
- Line 767: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaError_t e = cudaMemcpyAsync(h_in_ptrs[i], chunk_data, cs,
  Confidence: band=very_high; score=0.9
- Line 770: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: spdlog::error("[gpu_compress] cudaMemcpyAsync H2D chunk[{}] failed: {}",
  Confidence: band=very_high; score=0.9
- Line 810: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_in_ptrs,  h_in_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 816: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpyAsync(d_out_ptrs,  h_out_ptrs.data(),
  Confidence: band=very_high; score=0.9
- Line 868: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: e = cudaMemcpy(result.data() + off, h_out_ptrs[i],
  Confidence: band=very_high; score=0.9
- Line 971: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (data_size == 0) return false;         // empty input always uses CPU
  Confidence: band=very_high; score=0.9
- Line 1351: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: res.error_message = "LZ4: input too large";
  Confidence: band=very_high; score=0.9
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(compress(ptrs[i], sizes[i], algorithm, cfg));
  Confidence: band=high; score=0.74
- Line 372: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_free.push_back(*ptr);
  Confidence: band=high; score=0.74
- Line 577: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_free.push_back(*ptr);
  Confidence: band=high; score=0.74
- Line 1182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ptrs.push_back(buffers[idx].data());
  Confidence: band=high; score=0.74
- Line 1243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(decompress(compressed_buffers[i], algorithm, orig));
  Confidence: band=high; score=0.74

### src/storage/columnar_format.cpp
Total findings: 35

- Line 750: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Maximum safe input size - must fit in int for LZ4 API
  Confidence: band=very_high; score=0.99
- Line 751: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: constexpr size_t MAX_INPUT_SIZE = static_cast<size_t>(INT_MAX);
  Confidence: band=very_high; score=0.99
- Line 752: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.99
- Line 755: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "LZ4 compression: input data too large (exceeds INT_MAX)"
  Confidence: band=very_high; score=0.99
- Line 888: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Maximum safe input size (1GB)
  Confidence: band=very_high; score=0.99
- Line 889: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: constexpr size_t MAX_INPUT_SIZE = 1024ULL * 1024 * 1024;
  Confidence: band=very_high; score=0.99
- Line 890: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.99
- Line 893: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Snappy compression: input data too large"
  Confidence: band=very_high; score=0.99
- Line 1258: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: Result<ColumnSegment> ColumnSegment::deserialize(const std::vector<uint8_t>& data) {
  Confidence: band=very_high; score=0.99
- Line 1262: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: "Segment deserialize: insufficient data"
  Confidence: band=very_high; score=0.99
- Line 1288: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: "Segment deserialize: truncated data"
  Confidence: band=very_high; score=0.99
- Line 750: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Maximum safe input size - must fit in int for LZ4 API
  Confidence: band=very_high; score=0.9
- Line 751: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: constexpr size_t MAX_INPUT_SIZE = static_cast<size_t>(INT_MAX);
  Confidence: band=very_high; score=0.9
- Line 752: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.9
- Line 755: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "LZ4 compression: input data too large (exceeds INT_MAX)"
  Confidence: band=very_high; score=0.9
- Line 888: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Maximum safe input size (1GB)
  Confidence: band=very_high; score=0.9
- Line 889: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: constexpr size_t MAX_INPUT_SIZE = 1024ULL * 1024 * 1024;
  Confidence: band=very_high; score=0.9
- Line 890: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (data.size() > MAX_INPUT_SIZE) {
  Confidence: band=very_high; score=0.9
- Line 893: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Snappy compression: input data too large"
  Confidence: band=very_high; score=0.9
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(value);
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(value);
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint32_t> dictionary;
  Confidence: band=medium; score=0.66
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dict_values.push_back(str);
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(dictionary[idx]);
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: encoded.push_back(normalized);
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: encoded.push_back(normalized);
  Confidence: band=high; score=0.74
- Line 538: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int32_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 546: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int32_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 554: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(normalized + min_val);
  Confidence: band=high; score=0.74
- Line 608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 615: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 622: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(static_cast<int64_t>(normalized) + min_val);
  Confidence: band=high; score=0.74
- Line 629: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decoded.push_back(normalized + min_val);
  Confidence: band=high; score=0.74
- Line 1386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: projected.push_back(segments[idx]);
  Confidence: band=high; score=0.74
- Line 1408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matching_indices.push_back(i);
  Confidence: band=high; score=0.74

### src/storage/erasure_coder_factory.cpp
Total findings: 21

- Line 36: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Context: bool ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
  Confidence: band=medium; score=0.56
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(index);
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Context: uint8_t ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 228: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_inv(uint8_t a)
  Context: uint8_t ReedSolomonCoder::gf_inv(uint8_t a) {
  Confidence: band=medium; score=0.56
- Line 243: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_div(uint8_t a, uint8_t b)
  Context: uint8_t ReedSolomonCoder::gf_div(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 247: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp)
  Context: uint8_t ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp) {
  Confidence: band=medium; score=0.56
- Line 271: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b)
  Context: uint8_t CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
  Confidence: band=medium; score=0.56
- Line 287: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::gf_inv(uint8_t a)
  Context: uint8_t CauchyReedSolomonCoder::gf_inv(uint8_t a) {
  Confidence: band=medium; score=0.56
- Line 348: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix)
  Context: bool CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
  Confidence: band=medium; score=0.56
- Line 420: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(chunk));
  Confidence: band=high; score=0.74
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 433: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: chunks.push_back(std::move(parity));
  Confidence: band=high; score=0.74
- Line 441: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_indices.push_back(index);
  Confidence: band=high; score=0.74
- Line 532: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: ErasureCoder::create(ErasureCodingAlgorithm algorithm)
  Context: std::unique_ptr<ErasureCoder> ErasureCoder::create(ErasureCodingAlgorithm algorithm) {
  Confidence: band=medium; score=0.56

### src/storage/simd_filter.cpp
Total findings: 20

- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 250: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 284: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 384: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 416: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 448: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 453: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint32_t>(i + lane));
  Confidence: band=high; score=0.74
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (scalar_cmp(data[i], op, thr)) out.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 664: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint32_t>(i));
  Confidence: band=high; score=0.74
- Line 689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(idx + row_offset);
  Confidence: band=high; score=0.74
- Line 689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(idx + row_offset);
  Confidence: band=high; score=0.74

### src/storage/backup_manager.cpp
Total findings: 17

- Line 1897: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //    puts.  This avoids overwriting CFs outside the requested scope.
  Confidence: band=very_high; score=0.9
- Line 52: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static std::string winQuoteForCreateProcess(const std::string& s) {
  Confidence: band=high; score=0.74
- Line 54: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (c == '"') out += "\\\"";
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards_array.push_back(shard_obj);
  Confidence: band=high; score=0.74
- Line 724: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: backups.push_back(name);
  Confidence: band=high; score=0.74
- Line 730: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (filename format ensures correct sort order)
  Confidence: band=high; score=0.74
- Line 882: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: Result<void> BackupManager::isBackupComplete(const std::string& backup_dir,
  Confidence: band=high; score=0.74
- Line 989: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string cmd = "tar -czf " + winQuoteForCreateProcess(compressed_file) +
  Confidence: band=high; score=0.74
- Line 1061: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string cmd = "tar -xzf " + winQuoteForCreateProcess(compressed_file) +
  Confidence: band=high; score=0.74
- Line 1653: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: full_backups.push_back(name);
  Confidence: band=high; score=0.74
- Line 1884: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) coll_list += ", ";
  Confidence: band=high; score=0.74
- Line 1913: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> target_cfs;
  Confidence: band=medium; score=0.66
- Line 1923: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_descriptors.emplace_back(cf_name, rocksdb::ColumnFamilyOptions{});
  Confidence: band=high; score=0.74
- Line 1923: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_descriptors.emplace_back(cf_name, rocksdb::ColumnFamilyOptions{});
  Confidence: band=high; score=0.74
- Line 1923: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_descriptors.emplace_back(cf_name, rocksdb::ColumnFamilyOptions{});
  Confidence: band=high; score=0.74
- Line 2271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(kv.second.schedule_id, kv.second.cron_expression);
  Confidence: band=high; score=0.74
- Line 2551: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry.path().string());
  Confidence: band=high; score=0.74

### src/storage/blob_redundancy_manager.cpp
Total findings: 16

- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(loc.shard_id);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: locs.push_back(location_to_json(loc));
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: m.locations.push_back(location_from_json(lj));
  Confidence: band=high; score=0.74
- Line 744: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i) s += ", ";
  Confidence: band=high; score=0.74
- Line 768: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: healthy_dcs.push_back(loc.datacenter);
  Confidence: band=high; score=0.74
- Line 843: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: written_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 866: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: written_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 985: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deleted_shards.push_back(location.shard_id);
  Confidence: band=high; score=0.74
- Line 1028: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1053: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: degraded.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1067: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: critical.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: degraded_ids.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: healthy_dcs.push_back(loc.datacenter);
  Confidence: band=high; score=0.74
- Line 1299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: affected_blob_ids.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1299: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: affected_blob_ids.push_back(blob_id);
  Confidence: band=high; score=0.74
- Line 1476: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(location.shard_id);
  Confidence: band=high; score=0.74

### src/storage/rocksdb_wrapper.cpp
Total findings: 16

- Line 269: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Prefer HyperClockCache if available; fallback to LRUCache for compatibility
  Confidence: band=high; score=0.8
- Line 272: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Use LRU cache universally for maximum compatibility.
  Confidence: band=high; score=0.8
- Line 468: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Skip setting unavailable TransactionDBOptions fields to preserve compatibility.
  Confidence: band=high; score=0.8
- Line 1297: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool RocksDBWrapper::WriteBatchWrapper::commit() {
  Confidence: band=very_high; score=0.9
- Line 1362: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool RocksDBWrapper::WriteBatchWithIndexWrapper::commit() {
  Confidence: band=very_high; score=0.9
- Line 1565: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool RocksDBWrapper::TransactionWrapper::commit() {
  Confidence: band=very_high; score=0.9
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: paths.emplace_back(p.path, static_cast<uint64_t>(p.target_size_bytes));
  Confidence: band=high; score=0.74
- Line 703: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cf_handles_.emplace_back(cf_handles[i]);
  Confidence: band=high; score=0.74
- Line 1031: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.emplace_back(std::async(
  Confidence: band=high; score=0.74
- Line 1266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::in_place, values[i].begin(), values[i].end());
  Confidence: band=high; score=0.74
- Line 1464: severity=MEDIUM; category=deprecated_apis; pattern=GetSnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Context: read_opts.snapshot = txn_->GetSnapshot();
  Confidence: band=high; score=0.74
- Line 1486: severity=MEDIUM; category=deprecated_apis; pattern=GetSnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Context: read_opts.snapshot = txn_->GetSnapshot();
  Confidence: band=high; score=0.74
- Line 1639: severity=MEDIUM; category=deprecated_apis; pattern=GetSnapshot\(\)
  Description: Deprecated API: GetSnapshot\(\) → Use recent API version
  Context: return Ok(txn_->GetSnapshot());
  Confidence: band=high; score=0.74
- Line 1898: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (level > 0) num_files_at_levels += ", ";
  Confidence: band=high; score=0.74
- Line 2179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.emplace_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 2615: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(std::in_place, values[i].begin(), values[i].end());
  Confidence: band=high; score=0.74

### src/storage/tt_quantizer.cpp
Total findings: 16

- Line 44: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<QuantizedCore> QuantizedCore::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 118: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<QuantizedTrain> QuantizedTrain::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 147: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto oc = QuantizedCore::deserialize(cb);
  Confidence: band=very_high; score=0.99
- Line 32: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 4; ++i) out.push_back((u >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>(quant_type));
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 8; ++i) out.push_back((v >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 8; ++i) out.push_back((v >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (int i = 0; i < 8; ++i) out.push_back((u >> (i*8)) & 0xFF);
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>(quant_type));
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>(quant_type));
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qt.cores.push_back(std::move(qc));
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qt.cores.push_back(std::move(qc));
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: qt.cores.push_back(std::move(qc));
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train.cores.push_back(std::move(core));
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train.cores.push_back(std::move(core));
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train.cores.push_back(std::move(core));
  Confidence: band=high; score=0.74

### src/storage/history_manager.cpp
Total findings: 12

- Line 105: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<HistoryRecord> HistoryManager::deserializeHistoryRecord(std::string_view data) {
  Confidence: band=very_high; score=0.99
- Line 207: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return deserializeHistoryRecord(it.value());
  Confidence: band=very_high; score=0.99
- Line 214: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto rec = deserializeHistoryRecord(val);
  Confidence: band=very_high; score=0.99
- Line 272: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<ConflictRecord> ConflictManager::deserializeConflictRecord(std::string_view data) {
  Confidence: band=very_high; score=0.99
- Line 313: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return deserializeConflictRecord(
  Confidence: band=very_high; score=0.99
- Line 321: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto rec = deserializeConflictRecord(val);
  Confidence: band=very_high; score=0.99
- Line 342: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<ConflictSet> ConflictManager::deserializeConflictSet(std::string_view data) {
  Confidence: band=very_high; score=0.99
- Line 377: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return deserializeConflictSet(
  Confidence: band=very_high; score=0.99
- Line 385: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto set = deserializeConflictSet(val);
  Confidence: band=very_high; score=0.99
- Line 40: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hi = hex[i];
  Confidence: band=high; score=0.74
- Line 41: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto lo = hex[i + 1];
  Confidence: band=high; score=0.74
- Line 47: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((nibble(hi) << 4) | nibble(lo)));
  Confidence: band=high; score=0.74

### src/storage/hierarchical_tucker_decomposer.cpp
Total findings: 11

- Line 248: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::unique_ptr<HTNode> deserializeNode(Reader& r) {
  Confidence: band=very_high; score=0.99
- Line 274: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: node->left  = deserializeNode(r);
  Confidence: band=very_high; score=0.99
- Line 275: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: node->right = deserializeNode(r);
  Confidence: band=very_high; score=0.99
- Line 295: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<HTTrain> HTTrain::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 318: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: ht.root = deserializeNode(r);
  Confidence: band=very_high; score=0.99
- Line 120: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (fl == 0.0f) continue;
  Confidence: band=very_high; score=0.9
- Line 143: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // toTTTrain — compatibility bridge with memoization (stub #286 resolved)
  Confidence: band=high; score=0.8
- Line 705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_shape.push_back(r_inner);
  Confidence: band=high; score=0.74
- Line 705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_shape.push_back(r_inner);
  Confidence: band=high; score=0.74
- Line 705: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: left_shape.push_back(r_inner);
  Confidence: band=high; score=0.74
- Line 893: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: root_core_shape.push_back(1);  // r_out = 1 at root
  Confidence: band=high; score=0.74

### src/storage/wom_tree.cpp
Total findings: 10

- Line 219: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::atomic<uint64_t> stat_puts{0};
  Confidence: band=very_high; score=0.9
- Line 682: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: impl_->stat_puts.fetch_add(1, std::memory_order_relaxed);
  Confidence: band=very_high; score=0.9
- Line 817: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: impl_->stat_puts.store(0, std::memory_order_relaxed);
  Confidence: band=very_high; score=0.9
- Line 832: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: s.total_puts          = impl_->stat_puts.load(std::memory_order_relaxed);
  Confidence: band=very_high; score=0.9
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: child_ops[idx].push_back(std::move(op));
  Confidence: band=high; score=0.74
- Line 349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: child.buffer.push_back(std::move(op));
  Confidence: band=high; score=0.74
- Line 382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_root->children.push_back(std::move(root));
  Confidence: band=high; score=0.74
- Line 432: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_root->pivot_keys.push_back(std::move(pivot));
  Confidence: band=high; score=0.74
- Line 579: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: void collectAllEntries(std::map<std::string, std::string>& out) const {
  Confidence: band=high; score=0.74
- Line 584: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string>& out) const {
  Confidence: band=high; score=0.74

### src/storage/storage_audit_logger.cpp
Total findings: 8

- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: found.push_back(std::stoull(m[1].str()));
  Confidence: band=high; score=0.74
- Line 173: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: Result<void> StorageAuditLogger::log(Event event,
  Confidence: band=medium; score=0.6
- Line 182: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::PUT, key, extra);
  Confidence: band=medium; score=0.6
- Line 187: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::DEL, key, extra);
  Confidence: band=medium; score=0.6
- Line 191: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::CHECKPOINT, "", detail);
  Confidence: band=medium; score=0.6
- Line 195: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::RECOVERY, "", detail);
  Confidence: band=medium; score=0.6
- Line 199: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::COMPACTION, "", detail);
  Confidence: band=medium; score=0.6
- Line 203: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: return log(Event::SNAPSHOT, "", detail);
  Confidence: band=medium; score=0.6

### src/storage/tensor_router.cpp
Total findings: 8

- Line 209: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Force-LIFT for inference-bound data when policy says so
  Confidence: band=very_high; score=0.9
- Line 210: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (policy.force_lift_for_inference && hint.inference_use) {
  Confidence: band=very_high; score=0.9
- Line 403: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"force_lift_for_inference",     impl_->policy.force_lift_for_inference},
  Confidence: band=very_high; score=0.9
- Line 409: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"inference_use", hint.inference_use},
  Confidence: band=very_high; score=0.9
- Line 189: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double log_n = std::log(static_cast<double>(n_pilot));
  Confidence: band=medium; score=0.6
- Line 190: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double log_r = std::log(static_cast<double>(res.pilot_rank));
  Confidence: band=medium; score=0.6
- Line 449: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: void TensorRouter::setTemplateCatalog(
  Confidence: band=medium; score=0.6
- Line 471: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: TensorRouter::templateCatalog() const noexcept {
  Confidence: band=medium; score=0.6

### src/storage/compressed_storage.cpp
Total findings: 7

- Line 39: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<CompressedValue> CompressedValue::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 102: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize
  Confidence: band=very_high; score=0.99
- Line 103: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto cv = CompressedValue::deserialize(*serialized);
  Confidence: band=very_high; score=0.99
- Line 183: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto cv = CompressedValue::deserialize(*serialized);
  Confidence: band=very_high; score=0.99
- Line 29: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(static_cast<uint8_t>((size >> (i * 8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += "Column: " + pair.first + "\n";
  Confidence: band=high; score=0.74

### src/storage/distributed_transaction_manager.cpp
Total findings: 7

- Line 304: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // reference that outlives any concurrent unregisterShard() call.
  Confidence: band=very_high; score=0.99
- Line 125: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: std::optional<std::string> DistributedTransaction::get(std::string_view key) {
  Confidence: band=very_high; score=0.9
- Line 129: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: return participant->get(logical_key);
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function commit without trace point
  Context: bool DistributedTransaction::commit() {
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [shard_id, ops] : pending_ops_) {
  Confidence: band=very_high; score=0.9
- Line 245: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function abort without trace point
  Context: THEMIS_ERROR("DistributedTransaction [{}]: shard '{}' abort (rollback) threw: {}",
  Confidence: band=very_high; score=0.9
- Line 260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(shard_id);
  Confidence: band=high; score=0.74

### src/storage/erasure_coding_backend.cpp
Total findings: 7

- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, EncodedShard>& shards,
  Confidence: band=high; score=0.74
- Line 152: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<uint32_t, std::vector<uint8_t>> chunk_map;
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(i);
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(i);
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(i);
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<uint32_t, EncodedShard> shard_map;
  Confidence: band=high; score=0.74

### src/storage/blob_backend_s3.cpp
Total findings: 6

- Line 118: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto input_stream = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
  Confidence: band=very_high; score=0.99
- Line 119: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_stream->write(reinterpret_cast<const char*>(data.data()), data.size());
  Confidence: band=very_high; score=0.99
- Line 120: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: request.SetBody(input_stream);
  Confidence: band=very_high; score=0.99
- Line 118: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input_stream = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_stream->write(reinterpret_cast<const char*>(data.data()), data.size());
  Confidence: band=very_high; score=0.9
- Line 120: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: request.SetBody(input_stream);
  Confidence: band=very_high; score=0.9

### src/storage/tensor_train_decomposer.cpp
Total findings: 6

- Line 167: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<TTTrain> TTTrain::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 172: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: if (pos + 8 > bytes.size()) throw std::runtime_error("TTTrain::deserialize: underflow");
  Confidence: band=very_high; score=0.99
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((v >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<uint8_t>((u >> (i*8)) & 0xFF));
  Confidence: band=high; score=0.74

### src/storage/base_entity.cpp
Total findings: 5

- Line 624: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity BaseEntity::deserialize(std::string_view pk, const Blob& blob) {
  Confidence: band=very_high; score=0.99
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(el.get<std::string>());
  Confidence: band=high; score=0.74
- Line 357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(static_cast<float>(dres.value_unsafe()));
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(static_cast<float>(elem.get<double>()));
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec.push_back(static_cast<float>(elem.get<double>()));
  Confidence: band=high; score=0.74

### src/storage/gguf_metadata.cpp
Total findings: 5

- Line 76: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "[ThemisDB][SECURITY] GGUFMetadata: HMAC input exceeds INT_MAX; "
  Confidence: band=very_high; score=0.99
- Line 345: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool GGUFMetadata::deserialize(const std::vector<uint8_t>& bytes) {
  Confidence: band=very_high; score=0.99
- Line 76: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "[ThemisDB][SECURITY] GGUFMetadata: HMAC input exceeds INT_MAX; "
  Confidence: band=very_high; score=0.9
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>(v >>  0));
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(k);
  Confidence: band=high; score=0.74

### src/storage/nlp_metadata_extractor.cpp
Total findings: 5

- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.keywords.push_back(keywords[i].text);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.keywords.push_back(keywords[i].text);
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: meta.emails.push_back(entity.text);
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keywords.push_back(kw.text);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result[entity.type].push_back(entity.text);
  Confidence: band=high; score=0.74

### src/storage/online_schema_migration.cpp
Total findings: 5

- Line 148: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function migrate without trace point
  Context: MigrationResult SchemaMigrator::migrate()
  Confidence: band=very_high; score=0.9
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tables.push_back(op.table_name);
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back(msg);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.properties.push_back(std::move(prop));
  Confidence: band=high; score=0.74
- Line 577: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schema.properties.push_back(std::move(meta));
  Confidence: band=high; score=0.74

### src/storage/tensor_compaction_filter.cpp
Total findings: 4

- Line 108: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize raw TTTrain
  Confidence: band=very_high; score=0.99
- Line 113: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto opt = TTTrain::deserialize(bytes);
  Confidence: band=very_high; score=0.99
- Line 139: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize QuantizedTrain header
  Confidence: band=very_high; score=0.99
- Line 144: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto opt = QuantizedTrain::deserialize(bytes);
  Confidence: band=very_high; score=0.99

### src/storage/compression_strategy.cpp
Total findings: 3

- Line 573: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(value);
  Confidence: band=high; score=0.74
- Line 597: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dictionary.push_back(value);
  Confidence: band=high; score=0.74
- Line 641: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(dictionary[idx]);
  Confidence: band=high; score=0.74

### src/storage/index_analyzer.cpp
Total findings: 3

- Line 243: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : snapshot) {
  Confidence: band=very_high; score=0.9
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.indices.push_back(std::move(ie));
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: reports.push_back(std::move(report));
  Confidence: band=high; score=0.74

### src/storage/mvcc_store.cpp
Total findings: 3

- Line 266: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Keys are already in ascending timestamp order (big-endian sort).
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: base_keys.emplace_back(bk);
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: base_keys.emplace_back(vkey.data(), vkey.size() - 9);
  Confidence: band=high; score=0.74

### src/storage/tensor_network_storage_engine.cpp
Total findings: 3

- Line 193: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return QuantizedTrain::deserialize(*meta);
  Confidence: band=very_high; score=0.99
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: logical_keys.push_back(raw_key.substr(raw_prefix.size()));
  Confidence: band=high; score=0.74

### src/storage/wal_storage.cpp
Total findings: 3

- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments_.push_back(sid);
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: segments_.push_back(1);
  Confidence: band=high; score=0.74
- Line 477: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(sid);
  Confidence: band=high; score=0.74

### src/storage/columnar_cache.cpp
Total findings: 2

- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (on_evict_cb) evicted_keys.push_back(it->first);
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (cfg_.on_evict) to_notify.push_back(k);
  Confidence: band=high; score=0.74

### src/storage/ggml_tensor_bridge.cpp
Total findings: 2

- Line 142: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Pretend to be a ggml_tensor for pointer compatibility in tests.
  Confidence: band=high; score=0.8
- Line 176: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // not for llama.cpp inference until a real allocator is injected).
  Confidence: band=very_high; score=0.9

### src/storage/raft_mvcc_bridge.cpp
Total findings: 2

- Line 106: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: HLCTimestamp RaftMvccBridge::raftAwareWrite(
  Confidence: band=very_high; score=0.99
- Line 98: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: RaftMvccBridge::snapshotRead(std::string_view key, HLCTimestamp ts) {
  Confidence: band=very_high; score=0.9

### src/storage/security_signature_manager.cpp
Total findings: 2

- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: signatures.push_back(*sig);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.failed_resource_ids.push_back(sig->resource_id);
  Confidence: band=high; score=0.74

### src/storage/adaptive_compaction.cpp
Total findings: 1

- Line 157: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: ? std::log(current_write_amp) / std::log(config_.urgent_write_amp_threshold)
  Confidence: band=medium; score=0.6

### src/storage/compaction_manager.cpp
Total findings: 1

- Line 183: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // writes are memtable flush outputs; L1+ writes are compaction outputs.
  Confidence: band=very_high; score=0.9

### src/storage/database_connection_manager.cpp
Total findings: 1

- Line 264: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: health_list.push_back(health);
  Confidence: band=high; score=0.74

### src/storage/hamming_coder.cpp
Total findings: 1

- Line 74: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
  Confidence: band=high; score=0.74

### src/storage/hlc.cpp
Total findings: 1

- Line 78: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: HybridLogicalClock::now()
  Context: HLCTimestamp HybridLogicalClock::now() {
  Confidence: band=medium; score=0.56

### src/storage/index_maintenance.cpp
Total findings: 1

- Line 350: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jobs.push_back(status);
  Confidence: band=high; score=0.74

### src/storage/key_schema.cpp
Total findings: 1

- Line 123: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Assume DOCUMENT for backward compatibility (was more common in early versions)
  Confidence: band=high; score=0.8

### src/storage/pitr_manager.cpp
Total findings: 1

- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: preview.affected_keys.push_back(event.key);
  Confidence: band=high; score=0.74

### src/storage/schema_dead_weight_detector.cpp
Total findings: 1

- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: signal.push_back(static_cast<double>(count));
  Confidence: band=high; score=0.74

### src/storage/security_signature.cpp
Total findings: 1

- Line 58: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<SecuritySignature> SecuritySignature::deserialize(const std::string& data) {
  Confidence: band=very_high; score=0.99

### src/storage/storage_engine.cpp
Total findings: 1

- Line 24: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // They are provided for testing, development, and backward compatibility only.
  Confidence: band=high; score=0.8

### src/storage/storage_layout_advisor.cpp
Total findings: 1

- Line 59: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: diffs.push_back(ts[i] - ts[i - 1]);
  Confidence: band=high; score=0.74

### src/storage/storage_parquet_exporter.cpp
Total findings: 1

- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>((u >> s) & 0xFF));
  Confidence: band=high; score=0.74

### src/storage/tiered_storage.cpp
Total findings: 1

- Line 97: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: safe += (c == '/' || c == '\\' || c == ':' || c == '*' ||
  Confidence: band=high; score=0.74

### src/storage/vector_index_backend.cpp
Total findings: 1

- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({id, dist, toScore(dist)});
  Confidence: band=high; score=0.74

### include/storage/examples/schema_layout_advisor_example.cpp
Total findings: 0


### src/storage/blob_backend_azure.cpp
Total findings: 0


### src/storage/blob_backend_filesystem.cpp
Total findings: 0


### src/storage/blob_backend_gcs.cpp
Total findings: 0


### src/storage/blob_backend_webdav.cpp
Total findings: 0


### src/storage/concurrent_write_controller.cpp
Total findings: 0


### src/storage/disk_space_monitor.cpp
Total findings: 0


### src/storage/encrypted_blob_backend.cpp
Total findings: 0


### src/storage/merge_operators.cpp
Total findings: 0


### src/storage/mvcc_chain_pruner.cpp
Total findings: 0


### src/storage/nvme_manager.cpp
Total findings: 0


### src/storage/streaming_ingest_manager.cpp
Total findings: 0


### src/storage/transaction_retry_manager.cpp
Total findings: 0


### src/storage/zero_copy_blob_transfer.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
