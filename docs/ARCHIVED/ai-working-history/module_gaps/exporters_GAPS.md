# exporters Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: exporters
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 112
- Actionable Findings (Critical + High): 72
- Affected Files: 16

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 25 |
| High | 47 |
| Medium | 36 |
| Low | 4 |

## Category Summary

| Category | Count |
|---|---:|
| data_race | 10 |
| range_temporary | 10 |
| string_concat_loop | 9 |
| uncaught_exception | 9 |
| copy_overhead | 8 |
| uninitialized_access | 7 |
| manual_cleanup | 6 |
| o_n_squared | 6 |
| no_timeout | 4 |
| pointer_arithmetic_unbounded | 4 |
| shift_overflow | 4 |
| array_bounds | 3 |
| array_bounds_violation | 3 |
| legacy_or_compat_path | 3 |
| arithmetic_overflow | 2 |
| blocking_no_timeout | 2 |
| generic_catch | 2 |
| hardcoded_output | 2 |
| hardcoded_path | 2 |
| map_vs_unordered_map | 2 |
| module_doc_linkset_drift | 2 |
| nested_loop_find | 2 |
| null_dereference | 2 |
| thread_join_no_timeout | 2 |
| db_connection_leak | 1 |
| manual_cleanup_in_destructor | 1 |
| shared_state_no_sync | 1 |
| unchecked_memcpy | 1 |
| uninitialized_array | 1 |
| unnecessary_copy | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| exporters/huggingface_hub_client.cpp | 16 | 9 | 4 | 3 | 0 |
| exporters/parquet_exporter.cpp | 16 | 1 | 9 | 6 | 0 |
| exporters/export_encryption.cpp | 15 | 11 | 0 | 4 | 0 |
| exporters/huggingface_exporter.cpp | 11 | 0 | 5 | 6 | 0 |
| exporters/stream_writer.cpp | 11 | 3 | 5 | 3 | 0 |
| exporters/arrow_ipc_exporter.cpp | 10 | 0 | 5 | 5 | 0 |
| exporters/data_augmentation.cpp | 10 | 0 | 6 | 4 | 0 |
| exporters/jsonl_llm_exporter.cpp | 9 | 1 | 6 | 0 | 2 |
| exporters/exporter_metrics.cpp | 3 | 0 | 1 | 2 | 0 |
| exporters/streaming_exporter.cpp | 3 | 0 | 1 | 2 | 0 |
| exporters/aql_predicate_filter.cpp | 2 | 0 | 2 | 0 | 0 |
| exporters/export_format_registry.cpp | 2 | 0 | 2 | 0 | 0 |
| exporters/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| exporters/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| exporters/join_exporter.cpp | 1 | 0 | 1 | 0 | 0 |
| exporters/pii_detector.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### exporters/huggingface_hub_client.cpp
Total findings: 16

- Line 207: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto token_bytes = config_.key_provider->getKey(config_.hf_token_kek_id);
- Line 408: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto decision = config_.policy_engine->checkExportPermission(req);
- Line 494: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: for (int attempt = 0; attempt <= config_.max_retries; ++attempt) {

            if (attempt > 0 && !rate_limited) {

                THEMIS_WARN("HuggingFaceHubClient: retry {} for file {}", attempt, rel);

                if (!file_backoff.wait()) {

                    break;

                }

            }
- Line 494: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (!file_backoff.wait()) {
- Line 494: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (!file_backoff.wait()) {
- Line 591: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto decision = config_.policy_engine->checkExportPermission(req);
- Line 659: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: for (int attempt = 0; attempt <= config_.max_retries; ++attempt) {

            if (attempt > 0 && !rate_limited) {

                THEMIS_WARN("HuggingFaceHubClient: retry {} for shard {}", attempt, rel);

                if (!shard_backoff.wait()) {

                    break;

                }

            }
- Line 659: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (!shard_backoff.wait()) {
- Line 659: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: if (!shard_backoff.wait()) {
- Line 58: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: static CurlGlobal g_curl_global; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
- Line 455: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto &entry : fs::recursive_directory_iterator(dataset_dir)) {
- Line 544: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(sleep_secs));
- Line 714: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::seconds(sleep_secs));
- Line 164: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (tail == std::string::npos && secs >= 0) {

            return secs;

        }

    } catch (...) {

        // Not an integer; fall through to date parsing.

    }
- Line 164: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 457: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: files.push_back(entry.path().string());

### exporters/parquet_exporter.cpp
Total findings: 16

- Line 651: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto ws = writer->WriteTable(**tbl, static_cast<int64_t>(config_.row_group_size));
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3224 [exporters] Add duration an... (2026-03-12) | #3222 [exporters] Impleme
- Line 449: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (exclude_set.find(f) == exclude_set.end()) {
- Line 458: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (exclude_set.find(f) == exclude_set.end()) {
- Line 470: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (exclude_set.find(kv.first) == exclude_set.end()) {
- Line 471: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (exclude_set.find(kv.first) == exclude_set.end()) {
- Line 481: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (exclude_set.find(hint.name) == exclude_set.end()) {
- Line 825: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

            }



            col_data[ci].push_back(val);

        }



        ++row_count;
- Line 859: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: fmi.num_rows   = 0;

        fmi.created_by = "ThemisDB-parquet_exporter/1.0.0";

        for (const auto &kv : config_.file_metadata) {

            fmi.kv_metadata[kv.first] = kv.second;

        }



        auto footer = encodeFileMetaData(fmi);
- Line 929: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: fmi.total_byte_size = row_group_total_bytes;

    fmi.created_by      = "ThemisDB-parquet_exporter/1.0.0";

    for (const auto &kv : config_.file_metadata) {

        fmi.kv_metadata[kv.first] = kv.second;

    }



    auto footer = encodeFileMetaData(fmi);
- Line 367: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: value_buf.push_back(static_cast<uint8_t>(len & 0xFF));
- Line 368: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: value_buf.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
- Line 369: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: value_buf.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
- Line 370: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: value_buf.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
- Line 483: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cols.push_back(hint.name);
- Line 946: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ofs.close();

### exporters/export_encryption.cpp
Total findings: 15

- Line 31: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 4
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: static constexpr uint8_t MAGIC[4]    = {'T', 'E', 'N', 'C'};
- Line 31: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 4
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // ─────────────────────────────────────────────────────────────────────────────

// File-format constants

// ─────────────────────────────────────────────────────────────────────────────



static constexpr uint8_t MAGIC[4]    = {'T', 'E', 'N', 'C'};

static constexpr uint32_t FORMAT_VER = 1u;

static constexpr size_t IV_LEN       = 12u; // AES-GCM recommended nonce size

static constexpr size_t TAG_LEN      = 16u; // Full GCM authentication tag

static constexpr size_t KEY_LEN      = 32u; // AES-256
- Line 64: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2
- Line 64: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: writeU32(buf, static_cast<uint32_t>(s.size()));

    writeBytes(buf, reinterpret_cast<const uint8_t *>(s.data()), s.size());

}



static uint32_t readU32(const uint8_t *p) {

    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16)

           | (static_cast<uint32_t>(p[3]) << 24);

}



static uint64_t readU64(const uint8_t *p) {

    uint64_t v = 0;
- Line 65: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 3
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: | (static_cast<uint32_t>(p[3]) << 24);
- Line 65: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 8 > array size 3
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: writeBytes(buf, reinterpret_cast<const uint8_t *>(s.data()), s.size());

}



static uint32_t readU32(const uint8_t *p) {

    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16)

           | (static_cast<uint32_t>(p[3]) << 24);

}



static uint64_t readU64(const uint8_t *p) {

    uint64_t v = 0;

    for (int i = 0; i < 8; ++i) {
- Line 131: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto kek = config_.key_provider->getKey(config_.kek_id, key_version);
- Line 157: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto meta                  = config_.key_provider->getKeyMetadata(config_.kek_id);
- Line 691: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: kek         = config_.key_provider->getKey(config_.kek_id);
- Line 692: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: kek_version = config_.key_provider->getKeyMetadata(config_.kek_id).version;
- Line 875: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: kek = config_.key_provider->getKey(kek_id, kek_version);
- Line 433: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: src.close();
- Line 481: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: src.close();
- Line 970: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 973: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: out_f.close();

### exporters/huggingface_exporter.cpp
Total findings: 11

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3558 docs(exporters): reality-ch... (2026-03-12) | #3132 [WIP] Add Hugging F
- Line 165: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto &feat : resolvedFeatures()) {
- Line 250: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto &feat : resolvedFeatures()) {
- Line 266: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto &feat : resolvedFeatures()) {
- Line 322: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (field_dtypes.find(field_name) == field_dtypes.end()) {
- Line 201: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\\"";
- Line 203: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\\\";
- Line 205: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\n";
- Line 207: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\r";
- Line 209: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += "\\t";
- Line 318: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::string> field_dtypes;

### exporters/stream_writer.cpp
Total findings: 11

- Line 45: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: void StreamWriter::write(const std::string& data) {
- Line 49: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: void StreamWriter::write(const char* data, size_t size) {
- Line 74: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            compressed_bytes_written_ += size;', '        } else {', '            std::memcpy(buffer_.data() + buffer_pos_, data, size);', '            buffer_pos_ += size;', '        }']
- Line 37: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: StreamWriter::~StreamWriter() {
- Line 119: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // GZIP is accepted for backward compatibility but produces ZSTD output.
- Line 124: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: #ifdef THEMIS_HAS_ZSTD

    ZSTD_CStream* cstream = ZSTD_createCStream();

    if (!cstream) {

        throw ExportIOException("Failed to create zstd compression stream", config_.output_path, 0);

    }

    // Clamp level to valid zstd range [1, 22]

    int level = config_.compression_level;
- Line 133: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: size_t init_result = ZSTD_initCStream(cstream, level);

    if (ZSTD_isError(init_result)) {

        ZSTD_freeCStream(cstream);

        throw ExportIOException("Failed to initialize zstd compression stream",

                                config_.output_path, static_cast<int>(init_result));

    }

    compression_state_ = cstream;
- Line 138: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    compression_state_ = cstream;

#else

    throw ExportIOException("ZSTD compression not available (not compiled with THEMIS_HAS_ZSTD)",

                            config_.output_path, 0);

#endif

}
- Line 40: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: StreamWriter::~StreamWriter() {

    try {

        close();

    } catch (...) {

        // Suppress exceptions in destructor

    }

}
- Line 40: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 114: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: file_.close();

### exporters/arrow_ipc_exporter.cpp
Total findings: 10

- Line 527: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 643: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: col_data_total += static_cast<int64_t>(s.size());

        }

        if (col_data_total > static_cast<int64_t>(std::numeric_limits<int32_t>::max())) {

            throw SizeLimitException("Column '" + col

                                         + "' string data exceeds 2 GiB Arrow Utf8 limit; "

                                           "use LargeUtf8 for larger payloads",

                                     static_cast<size_t>(col_data_total),
- Line 926: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    writeMessageFrame(out, schema_msg, {});', '    // frame size: 4 (continuation) + 4 (meta_size) + schema_msg.size()', '    int64_t schema_frame_size = 4 + 4 + static_cast<int64_t>(schema_msg.size());', '    file_pos += schema_frame_size;', '']
- Line 935: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        int64_t rb_body_size   = static_cast<int64_t>(batch_body.bytes.size());', '        writeMessageFrame(out, rb_msg_bytes, batch_body.bytes);', '        int64_t rb_frame_size = 4 + 4 + static_cast<int64_t>(rb_msg_bytes.size()) + rb_body_size;', '        file_pos += rb_frame_size;', '']
- Line 973: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: out.close();

    if (!out) {

        throw ExportIOException("Error closing output file", options.output_path, errno);

    }



    stats.exported_entities = entities.size();
- Line 156: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        uint32_t u    = static_cast<uint32_t>(v);', '        buf_[idx]     = static_cast<uint8_t>(u & 0xFF);', '        buf_[idx + 1] = static_cast<uint8_t>((u >> 8) & 0xFF);', '        buf_[idx + 2] = static_cast<uint8_t>((u >> 16) & 0xFF);', '        buf_[idx + 3] = static_cast<uint8_t>((u >> 24) & 0xFF);']
- Line 157: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        buf_[idx]     = static_cast<uint8_t>(u & 0xFF);', '        buf_[idx + 1] = static_cast<uint8_t>((u >> 8) & 0xFF);', '        buf_[idx + 2] = static_cast<uint8_t>((u >> 16) & 0xFF);', '        buf_[idx + 3] = static_cast<uint8_t>((u >> 24) & 0xFF);', '    }']
- Line 158: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        buf_[idx + 1] = static_cast<uint8_t>((u >> 8) & 0xFF);', '        buf_[idx + 2] = static_cast<uint8_t>((u >> 16) & 0xFF);', '        buf_[idx + 3] = static_cast<uint8_t>((u >> 24) & 0xFF);', '    }', '']
- Line 1011: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: keys.push_back(kv.first);
- Line 1050: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto status = builders[i]->Append(fieldToString(entity, columns[i]));

### exporters/data_augmentation.cpp
Total findings: 10

- Line 37: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: {"get", {"obtain", "retrieve", "acquire"}},
- Line 183: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = combined.find(lw);
- Line 245: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto &field : selectFields(entity)) {
- Line 265: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto &field : selectFields(entity)) {
- Line 276: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto &field : selectFields(entity)) {
- Line 287: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto &field : selectFields(entity)) {
- Line 114: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += ' ';
- Line 115: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ' ';
- Line 160: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<std::string>> combined;
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fields.push_back(kv.first);

### exporters/jsonl_llm_exporter.cpp
Total findings: 9

- Line 535: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::string line = format_template_->render(entity, config_.template_field_mapping);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4252 feat(exporters): Replace zl... (2026-03-15) | #3760 feat(exporters): Po
- Line 78: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: themis::utils::SecurityEventType::EXPORT_DENIED, req.requesting_user, options.collection_name,

                {{"denial_reason", decision.denial_reason}, {"export_job_id", req.export_job_id}});

        }

        throw ExporterException(themis::errors::ErrorCode::ERR_EXPORT_POLICY_DENIED,

                                "Export denied by PolicyEngine: " + decision.denial_reason,

                                "collection=" + options.collection_name + ", user=" + req.requesting_user);

    }
- Line 105: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (options.tenant_context && options.tenant_context->enforce_isolation) {

        // Check required scopes

        if (!options.tenant_context->hasScope("export:read") && !options.tenant_context->hasScope("export:write")) {

            throw ExporterException(themis::errors::ErrorCode::ERR_EXPORT_TENANT_UNAUTHORIZED,

                                    "Insufficient permissions for export operation",

                                    "tenant_id=" + options.tenant_context->tenant_id);

        }
- Line 678: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // The style-based path is unchanged for backward compatibility.
- Line 738: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (entity.hasField(field_name)) {

            auto value = entity.getFieldAsString(field_name);

            if (value) {

                metadata[field_name] = *value;

            }

        }

    }
- Line 843: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // LoRA Adapter Metadata (LoRAExchange.ai compatibility)
- Line 704: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Skip empty outputs
- Line 705: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (quality.skip_empty_outputs && (!output || output->empty())) {

### exporters/exporter_metrics.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3215 [exporters] Implement incre... (2026-03-12)
- Line 348: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "  Export Rate: " << getExportRate() << " entities/sec\n";
- Line 349: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "  Throughput: " << getThroughput() << " bytes/sec\n";

### exporters/streaming_exporter.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4252 feat(exporters): Replace zl... (2026-03-15) | #3224 [exporters] Add dur
- Line 145: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: line += '\n';
- Line 363: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    }', '    size_t offset = 0;', '    f >> offset;', '    return f ? offset : 0;', '}']

### exporters/aql_predicate_filter.cpp
Total findings: 2

- Line 63: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!filter_node || !filter_node->condition) {
- Line 67: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto result = evaluator.evaluateExpression(filter_node->condition, doc);

### exporters/export_format_registry.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4378 [WIP] Update documentation ... (2026-03-22) | #3781 feat(exporters): re
- Line 157: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = kTypeMap.find(type_str);

### exporters/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### exporters/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### exporters/join_exporter.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4378 [WIP] Update documentation ... (2026-03-22) | #4297 Add JoinExporter: c

### exporters/pii_detector.cpp
Total findings: 1

- Line 206: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += std::string(value.length() - config_.partial_keep_prefix - config_.partial_keep_suffix, '*');

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
