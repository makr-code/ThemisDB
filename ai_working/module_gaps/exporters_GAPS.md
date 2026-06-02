# exporters Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: exporters
- Generated: 2026-06-02 11:55:48
- Status: Critical Findings Present
- Total Findings: 243
- Actionable Findings (Critical + High): 153
- Affected Files: 14

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 26 |
| High | 127 |
| Medium | 90 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 76 |
| container | 49 |
| performance_patterns | 43 |
| memory | 23 |
| concurrency | 12 |
| llm_ai_safety | 10 |
| raii | 7 |
| performance | 6 |
| type_conversion | 6 |
| legacy_duplication | 3 |
| audit_logging | 2 |
| platform | 2 |
| security | 2 |
| input_validation | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/exporters/export_encryption.cpp | 49 | 8 | 27 | 14 | 0 |
| src/exporters/parquet_exporter.cpp | 39 | 2 | 16 | 21 | 0 |
| src/exporters/arrow_ipc_exporter.cpp | 35 | 0 | 17 | 18 | 0 |
| src/exporters/jsonl_llm_exporter.cpp | 25 | 7 | 13 | 5 | 0 |
| src/exporters/huggingface_exporter.cpp | 18 | 0 | 8 | 10 | 0 |
| src/exporters/export_format_registry.cpp | 17 | 0 | 15 | 2 | 0 |
| src/exporters/huggingface_hub_client.cpp | 12 | 5 | 4 | 3 | 0 |
| src/exporters/stream_writer.cpp | 12 | 4 | 6 | 2 | 0 |
| src/exporters/data_augmentation.cpp | 11 | 0 | 6 | 5 | 0 |
| src/exporters/join_exporter.cpp | 8 | 0 | 8 | 0 | 0 |
| src/exporters/pii_detector.cpp | 6 | 0 | 0 | 6 | 0 |
| src/exporters/aql_predicate_filter.cpp | 4 | 0 | 4 | 0 | 0 |
| src/exporters/streaming_exporter.cpp | 4 | 0 | 2 | 2 | 0 |
| src/exporters/exporter_metrics.cpp | 3 | 0 | 1 | 2 | 0 |

## Full Scanner Findings

### src/exporters/export_encryption.cpp
Total findings: 49

- Line 31: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 4
  Remediation: Fix loop condition or increase array size
  Context: static constexpr uint8_t MAGIC[4]    = {'T', 'E', 'N', 'C'};
- Line 64: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Fix loop condition or increase array size
  Context: return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2
- Line 65: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 3
  Remediation: Fix loop condition or increase array size
  Context: | (static_cast<uint32_t>(p[3]) << 24);
- Line 131: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto kek = config_.key_provider->getKey(config_.kek_id, key_version);
- Line 157: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto meta                  = config_.key_provider->getKeyMetadata(config_.kek_id);
- Line 691: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: kek         = config_.key_provider->getKey(config_.kek_id);
- Line 692: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: kek_version = config_.key_provider->getKeyMetadata(config_.kek_id).version;
- Line 875: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: kek = config_.key_provider->getKey(kek_id, kek_version);
- Line 572: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to generate random job ID");
- Line 684: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Encryption not configured: kek_id or key_provider is missing");
- Line 725: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Cannot open input file for encryption", input_path);
- Line 731: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Cannot create output file for encryption", output_path);
- Line 741: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to allocate EVP cipher context");
- Line 751: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("EVP_EncryptInit_ex (AES-256-GCM) failed");
- Line 755: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("EVP_CTRL_GCM_SET_IVLEN failed");
- Line 759: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("EVP_EncryptInit_ex (key/IV) failed");
- Line 768: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to set GCM AAD (job_id)");
- Line 786: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("EVP_EncryptUpdate failed during streaming");
- Line 798: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("EVP_EncryptFinal_ex failed");
- Line 808: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to retrieve GCM authentication tag");
- Line 817: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Write error while finalising encrypted file", output_path);
- Line 832: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("No key_provider configured for export decryption");
- Line 838: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Cannot open encrypted file for decryption", input_path);
- Line 849: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to parse encrypted file header in '" + input_path
- Line 857: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Encrypted file '" + input_path + "' is too small to contain GCM tag");
- Line 866: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to read GCM tag from '" + input_path + "'");
- Line 904: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to allocate EVP cipher context");
- Line 914: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("EVP_DecryptInit_ex (AES-256-GCM) failed");
- Line 918: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("EVP_CTRL_GCM_SET_IVLEN failed");
- Line 922: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("EVP_DecryptInit_ex (key/IV) failed");
- Line 931: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to set GCM AAD (job_id)");
- Line 953: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("EVP_DecryptUpdate failed during streaming");
- Line 964: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to set expected GCM tag");
- Line 976: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("GCM authentication tag verification failed for '" + input_path
- Line 988: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Write error while finalising decrypted file", output_path);
- Line 42: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v & 0xFFu));
- Line 43: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
- Line 44: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFFu));
- Line 45: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFFu));
- Line 49: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFFu));
  Confidence: band=high; score=0.74
- Line 50: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFFu));
- Line 221: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 389: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 433: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: src.close();
- Line 481: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: src.close();
- Line 704: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 885: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 970: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 973: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: out_f.close();

### src/exporters/parquet_exporter.cpp
Total findings: 39

- Line 651: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ws = writer->WriteTable(**tbl, static_cast<int64_t>(config_.row_group_size));
- Line 651: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ws = writer->WriteTable(**tbl, static_cast<int64_t>(config_.row_group_size));
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3224 [exporters] Add duration an... (2026-03-12) | #3222 [exporters] Impleme
- Line 449: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (exclude_set.find(f) == exclude_set.end()) {
- Line 458: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (exclude_set.find(f) == exclude_set.end()) {
- Line 458: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (exclude_set.find(f) == exclude_set.end()) {
- Line 470: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (exclude_set.find(kv.first) == exclude_set.end()) {
- Line 471: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (exclude_set.find(kv.first) == exclude_set.end()) {
  Confidence: band=very_high; score=0.9
- Line 481: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (exclude_set.find(hint.name) == exclude_set.end()) {
- Line 500: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(errors::ErrorCode::ERR_EXPORT_TENANT_UNAUTHORIZED,
- Line 510: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigException("output_path must not be empty", "output_path");
- Line 593: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to open output file: " + open_status.status().ToString(), options.ou
- Line 615: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to create Parquet writer: " + writer_result.status().ToString(),
- Line 698: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(errors::ErrorCode::ERR_EXPORT_IO_ERROR,
- Line 813: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(errors::ErrorCode::ERR_EXPORT_IO_ERROR,
- Line 848: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Cannot open output file for writing", options.output_path, errno);
- Line 859: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: fmi.kv_metadata[kv.first] = kv.second;
- Line 929: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: fmi.kv_metadata[kv.first] = kv.second;
- Line 65: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v >> 8));
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>((u >> s) & 0xFF));
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((u >> s) & 0xFF));
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: value_buf.push_back(static_cast<uint8_t>(len & 0xFF));
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: value_buf.push_back(static_cast<uint8_t>(len & 0xFF));
- Line 368: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: value_buf.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
- Line 369: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: value_buf.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
- Line 370: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: value_buf.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
- Line 450: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 482: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(hint.name);
  Confidence: band=high; score=0.74
- Line 482: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(hint.name);
  Confidence: band=high; score=0.74
- Line 482: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(hint.name);
  Confidence: band=high; score=0.74
- Line 483: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cols.push_back(hint.name);
- Line 584: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(arrow::field(col, dt, true /* nullable */));
  Confidence: band=high; score=0.74
- Line 584: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(arrow::field(col, dt, true /* nullable */));
  Confidence: band=high; score=0.74
- Line 585: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(arrow::field(col, dt, true /* nullable */));
- Line 642: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 642: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 879: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.errors.push_back(err_code + ": Cannot open output file: " + options.output_path);
- Line 946: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ofs.close();

### src/exporters/arrow_ipc_exporter.cpp
Total findings: 35

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    writeMessageFrame(out, schema_msg, {});', '    // frame size: 4 (continuation) + 4 (meta_size) + schema_msg.size()', '    int64_t schema_frame_size = 4 + 4 + static_cast<int64_t>(schema_msg.size());', '    file_pos += schema_frame_size;', '']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        int64_t rb_body_size   = static_cast<int64_t>(batch_body.bytes.size());', '        writeMessageFrame(out, rb_msg_bytes, batch_body.bytes);', '        int64_t rb_frame_size = 4 + 4 + static_cast<int64_t>(rb_msg_bytes.size()) + rb_body_size;', '        file_pos += rb_frame_size;', '']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 308: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: fb.pre16(0);  // field[6] custom_metadata  absent
- Line 451: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: fb.pre16(20); // field[2] buffers
- Line 643: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw SizeLimitException("Column '" + col
- Line 850: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(errors::ErrorCode::ERR_EXPORT_TENANT_UNAUTHORIZED,
- Line 857: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigException("output_path must not be empty", "output_path");
- Line 896: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to open output file", options.output_path, errno);
- Line 973: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Error closing output file", options.output_path, errno);
- Line 1021: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(maybe_file.status().ToString(), options.output_path, 0);
- Line 1031: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(maybe_writer.status().ToString(), options.output_path, 0);
- Line 1037: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(maybe_writer.status().ToString(), options.output_path, 0);
- Line 1054: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(status.ToString(), options.output_path, 0);
- Line 1064: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(maybe_arr.status().ToString(), options.output_path, 0);
- Line 1072: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(status.ToString(), options.output_path, 0);
- Line 1077: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(status.ToString(), options.output_path, 0);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['        uint32_t u    = static_cast<uint32_t>(v);', '        buf_[idx]     = static_cast<uint8_t>(u & 0xFF);', '        buf_[idx + 1] = static_cast<uint8_t>((u >> 8) & 0xFF);', '        buf_[idx + 2] = static_cast<uint8_t>((u >> 16) & 0xFF);', '        buf_[idx + 3] = static_cast<uint8_t>((u >> 24) & 0xFF);']
  Confidence: band=medium; score=0.62
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['        buf_[idx]     = static_cast<uint8_t>(u & 0xFF);', '        buf_[idx + 1] = static_cast<uint8_t>((u >> 8) & 0xFF);', '        buf_[idx + 2] = static_cast<uint8_t>((u >> 16) & 0xFF);', '        buf_[idx + 3] = static_cast<uint8_t>((u >> 24) & 0xFF);', '    }']
  Confidence: band=medium; score=0.62
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['        buf_[idx + 1] = static_cast<uint8_t>((u >> 8) & 0xFF);', '        buf_[idx + 2] = static_cast<uint8_t>((u >> 16) & 0xFF);', '        buf_[idx + 3] = static_cast<uint8_t>((u >> 24) & 0xFF);', '    }', '']
  Confidence: band=medium; score=0.65
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_refs.push_back(C_field);
  Confidence: band=high; score=0.74
- Line 626: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vals.push_back(fieldToString(e, col));
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.buf_offsets.push_back(body_pos);
  Confidence: band=high; score=0.74
- Line 811: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 820: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 1003: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrow_fields.push_back(arrow::field(col, arrow::utf8()));
  Confidence: band=high; score=0.74
- Line 1004: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arrow_fields.push_back(arrow::field(col, arrow::utf8()));
- Line 1010: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 1011: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(kv.first);
- Line 1012: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: values.push_back(kv.second);
- Line 1050: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = builders[i]->Append(fieldToString(entity, columns[i]));
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(*maybe_arr);
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(*maybe_arr);
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(*maybe_arr);
  Confidence: band=high; score=0.74
- Line 1066: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arrays.push_back(*maybe_arr);

### src/exporters/jsonl_llm_exporter.cpp
Total findings: 25

- Line 417: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Optional input field
  Confidence: band=very_high; score=0.99
- Line 418: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (isFieldAllowed(mapping.input_field, options.include_fields, options.exclude_fields)) {
  Confidence: band=very_high; score=0.99
- Line 419: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto input = entity.getFieldAsString(mapping.input_field);
  Confidence: band=very_high; score=0.99
- Line 420: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input && !input->empty()) {
  Confidence: band=very_high; score=0.99
- Line 421: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: j["input"] = *input;
  Confidence: band=very_high; score=0.99
- Line 535: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string line = format_template_->render(entity, config_.template_field_mapping);
- Line 535: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string line = format_template_->render(entity, config_.template_field_mapping);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4252 feat(exporters): Replace zl... (2026-03-15) | #3760 feat(exporters): Po
- Line 78: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(themis::errors::ErrorCode::ERR_EXPORT_POLICY_DENIED,
- Line 105: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(themis::errors::ErrorCode::ERR_EXPORT_TENANT_UNAUTHORIZED,
- Line 227: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(themis::errors::ErrorCode::ERR_EXPORT_PII_VIOLATION,
- Line 417: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Optional input field
  Confidence: band=very_high; score=0.9
- Line 418: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (isFieldAllowed(mapping.input_field, options.include_fields, options.exclude_fields)) {
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input = entity.getFieldAsString(mapping.input_field);
  Confidence: band=very_high; score=0.9
- Line 420: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input && !input->empty()) {
  Confidence: band=very_high; score=0.9
- Line 421: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: j["input"] = *input;
  Confidence: band=very_high; score=0.9
- Line 678: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // The style-based path is unchanged for backward compatibility.
  Confidence: band=high; score=0.8
- Line 704: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Skip empty outputs
  Confidence: band=very_high; score=0.9
- Line 705: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (quality.skip_empty_outputs && (!output || output->empty())) {
  Confidence: band=very_high; score=0.9
- Line 843: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // LoRA Adapter Metadata (LoRAExchange.ai compatibility)
  Confidence: band=high; score=0.8
- Line 248: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.errors.push_back("Schema validation failed for " + entity.getPrimaryKey() + ": "
- Line 452: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: messages.push_back({{"role", "system"}, {"content", *system}});
- Line 811: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: runtime_metrics_.validation_errors.push_back(err_msg);
  Confidence: band=high; score=0.74
- Line 1049: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["schema_validation"]["recent_errors"].push_back(runtime_metrics_.validation_errors[i]);
  Confidence: band=high; score=0.74
- Line 1050: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["schema_validation"]["recent_errors"].push_back(runtime_metrics_.validation_errors[i]);

### src/exporters/huggingface_exporter.cpp
Total findings: 18

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3558 docs(exporters): reality-ch... (2026-03-12) | #3132 [WIP] Add Hugging F
- Line 94: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to open dataset_info.json for writing", info_path.string());
- Line 165: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &feat : resolvedFeatures()) {
- Line 175: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: split_obj["dataset_name"] = resolved_name;
- Line 179: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: info["dataset_size"]  = byte_count;
- Line 250: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &feat : resolvedFeatures()) {
- Line 266: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &feat : resolvedFeatures()) {
- Line 322: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (field_dtypes.find(field_name) == field_dtypes.end()) {
  Confidence: band=very_high; score=0.9
- Line 50: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.errors.push_back("output_path must be set to the dataset root directory");
- Line 201: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\"";
- Line 203: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\\";
- Line 205: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\n";
- Line 207: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\r";
- Line 209: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\t";
- Line 318: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> field_dtypes;
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred_features_.push_back(std::move(feat));
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred_features_.push_back(std::move(feat));
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred_features_.push_back(std::move(feat));
  Confidence: band=high; score=0.74

### src/exporters/export_format_registry.cpp
Total findings: 17

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4378 [WIP] Update documentation ... (2026-03-22) | #3781 feat(exporters): re
- Line 70: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("parquet", []() -> std::unique_ptr<IExporter> { return std::make_unique<ParquetExport
- Line 73: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("arrow", []() -> std::unique_ptr<IExporter> {
- Line 78: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("arrow_stream", []() -> std::unique_ptr<IExporter> {
- Line 86: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: []() -> std::unique_ptr<IExporter> { return std::make_unique<HuggingFaceExporter>(); });
- Line 88: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: []() -> std::unique_ptr<IExporter> { return std::make_unique<HuggingFaceExporter>(); });
- Line 91: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("streaming", []() -> std::unique_ptr<IExporter> { return std::make_unique<StreamingEx
- Line 95: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: []() -> std::unique_ptr<IExporter> { return std::make_unique<IncrementalExporter>(); });
- Line 98: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("join", []() -> std::unique_ptr<IExporter> { return std::make_unique<JoinExporter>();
- Line 99: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("join_jsonl", []() -> std::unique_ptr<IExporter> { return std::make_unique<JoinExport
- Line 102: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("jsonl_alpaca", []() -> std::unique_ptr<IExporter> {
- Line 107: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("jsonl_sharegpt", []() -> std::unique_ptr<IExporter> {
- Line 112: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("jsonl_chatml", []() -> std::unique_ptr<IExporter> {
- Line 117: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("jsonl_openai_ft", []() -> std::unique_ptr<IExporter> {
- Line 157: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = kTypeMap.find(type_str);
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validated.push_back({format_key, it->second, mapping});

### src/exporters/huggingface_hub_client.cpp
Total findings: 12

- Line 207: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto token_bytes = config_.key_provider->getKey(config_.hf_token_kek_id);
- Line 408: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto decision = config_.policy_engine->checkExportPermission(req);
- Line 494: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!file_backoff.wait()) {
- Line 591: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto decision = config_.policy_engine->checkExportPermission(req);
- Line 659: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!shard_backoff.wait()) {
- Line 62: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: s->append(data, sz * nmemb);
- Line 455: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &entry : fs::recursive_directory_iterator(dataset_dir)) {
- Line 544: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(sleep_secs));
- Line 714: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(sleep_secs));
- Line 164: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 456: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path().string());
  Confidence: band=high; score=0.74
- Line 457: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files.push_back(entry.path().string());

### src/exporters/stream_writer.cpp
Total findings: 12

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['            compressed_bytes_written_ += size;', '        } else {', '            std::memcpy(buffer_.data() + buffer_pos_, data, size);', '            buffer_pos_ += size;', '        }']
  Confidence: band=very_high; score=0.93
- Line 45: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void StreamWriter::write(const std::string& data) {
- Line 46: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: write(data.data(), data.size());
- Line 49: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void StreamWriter::write(const char* data, size_t size) {
- Line 25: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(
- Line 119: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // GZIP is accepted for backward compatibility but produces ZSTD output.
  Confidence: band=high; score=0.8
- Line 124: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to create zstd compression stream", config_.output_path, 0);
- Line 133: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to initialize zstd compression stream",
- Line 138: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("ZSTD compression not available (not compiled with THEMIS_HAS_ZSTD)",
- Line 163: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("ZSTD compression stream error", config_.output_path,
- Line 40: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 114: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file_.close();

### src/exporters/data_augmentation.cpp
Total findings: 11

- Line 37: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: {"get", {"obtain", "retrieve", "acquire"}},
- Line 183: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = combined.find(lw);
- Line 245: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &field : selectFields(entity)) {
- Line 265: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &field : selectFields(entity)) {
- Line 276: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &field : selectFields(entity)) {
- Line 287: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &field : selectFields(entity)) {
- Line 114: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ' ';
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 160: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> combined;
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(kv.first);

### src/exporters/join_exporter.cpp
Total findings: 8

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4378 [WIP] Update documentation ... (2026-03-22) | #4297 Add JoinExporter: c
- Line 34: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 55: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 82: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 89: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 172: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 279: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 324: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(

### src/exporters/pii_detector.cpp
Total findings: 6

- Line 62: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += std::string(value.length() - config_.partial_keep_prefix - config_.partial_keep_suffix, '*');
  Confidence: band=high; score=0.74

### src/exporters/aql_predicate_filter.cpp
Total findings: 4

- Line 31: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AqlPredicateFilterException("Failed to parse AQL predicate '" + predicate
- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AqlPredicateFilterException("AQL predicate '" + predicate + "' produced no filter conditions")
- Line 63: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!filter_node || !filter_node->condition) {
- Line 67: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto result = evaluator.evaluateExpression(filter_node->condition, doc);

### src/exporters/streaming_exporter.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4252 feat(exporters): Replace zl... (2026-03-15) | #3224 [exporters] Add dur
- Line 220: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("ExportEncryption: rename failed: " + ec.message(), options.output_path);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    }', '    size_t offset = 0;', '    f >> offset;', '    return f ? offset : 0;', '}']
  Confidence: band=medium; score=0.65
- Line 145: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: line += '\n';
  Confidence: band=high; score=0.74

### src/exporters/exporter_metrics.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3215 [exporters] Implement incre... (2026-03-12)
- Line 348: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "  Export Rate: " << getExportRate() << " entities/sec\n";
- Line 349: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "  Throughput: " << getThroughput() << " bytes/sec\n";

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
