# exporters Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: exporters
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 353
- Actionable Findings (Critical + High): 215
- Affected Files: 15

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 27 |
| High | 188 |
| Medium | 138 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| reliability | 119 |
| container | 74 |
| performance_patterns | 44 |
| memory | 33 |
| raii | 24 |
| security | 16 |
| concurrency | 12 |
| llm_ai_safety | 10 |
| performance | 6 |
| type_conversion | 6 |
| legacy_duplication | 3 |
| audit_logging | 2 |
| platform | 2 |
| input_validation | 1 |
| uninitialized | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/exporters/export_encryption.cpp | 102 | 8 | 70 | 24 | 0 |
| src/exporters/arrow_ipc_exporter.cpp | 53 | 0 | 17 | 36 | 0 |
| src/exporters/parquet_exporter.cpp | 44 | 2 | 16 | 26 | 0 |
| src/exporters/export_format_registry.cpp | 37 | 0 | 34 | 3 | 0 |
| src/exporters/jsonl_llm_exporter.cpp | 28 | 8 | 13 | 7 | 0 |
| src/exporters/huggingface_exporter.cpp | 19 | 0 | 8 | 11 | 0 |
| src/exporters/huggingface_hub_client.cpp | 16 | 5 | 7 | 4 | 0 |
| src/exporters/stream_writer.cpp | 15 | 4 | 7 | 4 | 0 |
| src/exporters/data_augmentation.cpp | 12 | 0 | 6 | 6 | 0 |
| src/exporters/pii_detector.cpp | 11 | 0 | 0 | 11 | 0 |
| src/exporters/join_exporter.cpp | 7 | 0 | 7 | 0 | 0 |
| src/exporters/streaming_exporter.cpp | 4 | 0 | 1 | 3 | 0 |
| src/exporters/aql_predicate_filter.cpp | 2 | 0 | 2 | 0 | 0 |
| src/exporters/exporter_metrics.cpp | 2 | 0 | 0 | 2 | 0 |
| src/exporters/incremental_exporter.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/exporters/export_encryption.cpp
Total findings: 102

- Line 30: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 4
  Remediation: Fix loop condition or increase array size
  Context: static constexpr uint8_t MAGIC[4]    = {'T', 'E', 'N', 'C'};
- Line 63: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 0
  Remediation: Fix loop condition or increase array size
  Context: return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2
- Line 64: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 8 > array 3
  Remediation: Fix loop condition or increase array size
  Context: | (static_cast<uint32_t>(p[3]) << 24);
- Line 130: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto kek = config_.key_provider->getKey(config_.kek_id, key_version);
- Line 156: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto meta                  = config_.key_provider->getKeyMetadata(config_.kek_id);
- Line 690: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: kek         = config_.key_provider->getKey(config_.kek_id);
- Line 691: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: kek_version = config_.key_provider->getKeyMetadata(config_.kek_id).version;
- Line 874: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: kek = config_.key_provider->getKey(kek_id, kek_version);
- Line 81: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: truncated string length field");
- Line 86: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: header string exceeds maximum length (" + std::to_string
- Line 90: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: truncated string data");
- Line 123: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ExportEncryption: key_provider is null");
- Line 126: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ExportEncryption: kek_id is empty");
- Line 132: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: KEK must be 32 bytes (AES-256)");
- Line 149: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ExportEncryption: encryption enabled but kek_id/key_provider missing");
- Line 152: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ExportEncryption: job_id must not be empty");
- Line 156: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto meta                  = config_.key_provider->getKeyMetadata(config_.kek_id);
- Line 166: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: failed to generate random IV");
- Line 179: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: failed to create cipher context");
- Line 203: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: plaintext exceeds maximum supported size (INT_MAX)");
- Line 208: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: EncryptUpdate failed");
- Line 213: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: EncryptFinal failed");
- Line 218: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: get tag failed");
- Line 263: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: invalid magic; not a TENC file");
- Line 269: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: truncated header (version)");
- Line 274: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: unsupported file format version " + std::to_string(ver))
- Line 285: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: truncated header (key_version)");
- Line 292: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: truncated header (iv)");
- Line 299: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: truncated header (ct_len)");
- Line 306: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: truncated ciphertext");
- Line 313: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: truncated authentication tag");
- Line 321: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("ExportEncryption: key_provider is null");
- Line 324: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: file header has empty kek_id");
- Line 345: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: failed to create cipher context");
- Line 369: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: ciphertext exceeds maximum supported size (INT_MAX)");
- Line 372: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: DecryptUpdate failed");
- Line 379: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: set tag failed");
- Line 385: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: authentication tag mismatch – "
- Line 409: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: cannot open source file: " + src_path);
- Line 413: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: cannot open destination file: " + dst_path);
- Line 423: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: cannot open source file: " + src_path);
- Line 430: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: failed to read source file: " + src_path);
- Line 443: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: cannot open destination file: " + dst_path);
- Line 448: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: failed to write encrypted file: " + dst_path);
- Line 457: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: cannot open source file: " + src_path);
- Line 461: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: cannot open destination file: " + dst_path);
- Line 471: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: cannot open source file: " + src_path);
- Line 478: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: failed to read source file: " + src_path);
- Line 489: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: cannot open destination file: " + dst_path);
- Line 495: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("ExportEncryption: failed to write decrypted file: " + dst_path);
- Line 571: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to generate random job ID");
- Line 683: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Encryption not configured: kek_id or key_provider is missing");
- Line 691: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: kek_version = config_.key_provider->getKeyMetadata(config_.kek_id).version;
- Line 724: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Cannot open input file for encryption", input_path);
- Line 730: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Cannot create output file for encryption", output_path);
- Line 740: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to allocate EVP cipher context");
- Line 750: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("EVP_EncryptInit_ex (AES-256-GCM) failed");
- Line 754: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("EVP_CTRL_GCM_SET_IVLEN failed");
- Line 758: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("EVP_EncryptInit_ex (key/IV) failed");
- Line 767: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to set GCM AAD (job_id)");
- Line 785: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("EVP_EncryptUpdate failed during streaming");
- Line 797: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("EVP_EncryptFinal_ex failed");
- Line 807: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw EncryptionException("Failed to retrieve GCM authentication tag");
- Line 816: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Write error while finalising encrypted file", output_path);
- Line 831: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("No key_provider configured for export decryption");
- Line 837: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Cannot open encrypted file for decryption", input_path);
- Line 848: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to parse encrypted file header in '" + input_path
- Line 856: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Encrypted file '" + input_path + "' is too small to contain GCM tag");
- Line 865: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to read GCM tag from '" + input_path + "'");
- Line 903: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to allocate EVP cipher context");
- Line 913: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("EVP_DecryptInit_ex (AES-256-GCM) failed");
- Line 917: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("EVP_CTRL_GCM_SET_IVLEN failed");
- Line 921: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("EVP_DecryptInit_ex (key/IV) failed");
- Line 930: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to set GCM AAD (job_id)");
- Line 952: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("EVP_DecryptUpdate failed during streaming");
- Line 963: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("Failed to set expected GCM tag");
- Line 975: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw DecryptionException("GCM authentication tag verification failed for '" + input_path
- Line 987: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Write error while finalising decrypted file", output_path);
- Line 41: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v & 0xFFu));
- Line 42: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
- Line 43: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFFu));
- Line 44: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFFu));
- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFFu));
  Confidence: band=high; score=0.74
- Line 49: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFFu));
- Line 220: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 225: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 388: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 393: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 432: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: src.close();
- Line 480: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: src.close();
- Line 703: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 744: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 812: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 818: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: out_f.close();
- Line 819: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: in_f.close();
- Line 884: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 907: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 969: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 972: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: out_f.close();
- Line 983: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_CIPHER_CTX_free(ctx);
- Line 989: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: out_f.close();
- Line 990: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: in_f.close();

### src/exporters/arrow_ipc_exporter.cpp
Total findings: 53

- Line 0: severity=HIGH; category=uncategorized
  Context: ['    writeMessageFrame(out, schema_msg, {});', '    // frame size: 4 (continuation) + 4 (meta_size) + schema_msg.size()', '    int64_t schema_frame_size = 4 + 4 + static_cast<int64_t>(schema_msg.size());', '    file_pos += schema_frame_size;', '']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['        int64_t rb_body_size   = static_cast<int64_t>(batch_body.bytes.size());', '        writeMessageFrame(out, rb_msg_bytes, batch_body.bytes);', '        int64_t rb_frame_size = 4 + 4 + static_cast<int64_t>(rb_msg_bytes.size()) + rb_body_size;', '        file_pos += rb_frame_size;', '']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 307: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: fb.pre16(0);  // field[6] custom_metadata  absent
- Line 450: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: fb.pre16(20); // field[2] buffers
- Line 642: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw SizeLimitException("Column '" + col
- Line 849: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(errors::ErrorCode::ERR_EXPORT_TENANT_UNAUTHORIZED,
- Line 856: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigException("output_path must not be empty", "output_path");
- Line 895: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to open output file", options.output_path, errno);
- Line 972: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Error closing output file", options.output_path, errno);
- Line 1020: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(maybe_file.status().ToString(), options.output_path, 0);
- Line 1030: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(maybe_writer.status().ToString(), options.output_path, 0);
- Line 1036: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(maybe_writer.status().ToString(), options.output_path, 0);
- Line 1053: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(status.ToString(), options.output_path, 0);
- Line 1063: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(maybe_arr.status().ToString(), options.output_path, 0);
- Line 1071: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(status.ToString(), options.output_path, 0);
- Line 1076: severity=HIGH; category=uncaught_exception
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
- Line 346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: field_refs.push_back(C_field);
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: field_refs.push_back(C_field);
- Line 625: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vals.push_back(fieldToString(e, col));
  Confidence: band=high; score=0.74
- Line 626: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vals.push_back(fieldToString(e, col));
- Line 630: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.buf_offsets.push_back(body_pos);
- Line 631: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.buf_lengths.push_back(0);
- Line 654: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: offsets_buf.push_back(static_cast<uint8_t>(u & 0xFF));
- Line 655: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: offsets_buf.push_back(static_cast<uint8_t>((u >> 8) & 0xFF));
- Line 656: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: offsets_buf.push_back(static_cast<uint8_t>((u >> 16) & 0xFF));
- Line 657: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: offsets_buf.push_back(static_cast<uint8_t>((u >> 24) & 0xFF));
- Line 666: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.buf_offsets.push_back(body_pos);
- Line 667: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.buf_lengths.push_back(offsets_len);
- Line 672: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.bytes.push_back(0);
- Line 682: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.buf_offsets.push_back(body_pos);
  Confidence: band=high; score=0.74
- Line 683: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.buf_offsets.push_back(body_pos);
- Line 684: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.buf_lengths.push_back(data_len);
- Line 689: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.bytes.push_back(0);
- Line 810: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 811: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cols.push_back(f);
- Line 819: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 820: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cols.push_back(f);
- Line 942: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rb_blocks.push_back(blk);
- Line 970: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: out.close();
- Line 1002: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrow_fields.push_back(arrow::field(col, arrow::utf8()));
  Confidence: band=high; score=0.74
- Line 1003: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arrow_fields.push_back(arrow::field(col, arrow::utf8()));
- Line 1009: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 1010: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(kv.first);
- Line 1011: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: values.push_back(kv.second);
- Line 1049: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = builders[i]->Append(fieldToString(entity, columns[i]));
  Confidence: band=high; score=0.74
- Line 1064: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(*maybe_arr);
  Confidence: band=high; score=0.74
- Line 1064: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(*maybe_arr);
  Confidence: band=high; score=0.74
- Line 1064: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(*maybe_arr);
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arrays.push_back(*maybe_arr);

### src/exporters/parquet_exporter.cpp
Total findings: 44

- Line 650: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ws = writer->WriteTable(**tbl, static_cast<int64_t>(config_.row_group_size));
- Line 650: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto ws = writer->WriteTable(**tbl, static_cast<int64_t>(config_.row_group_size));
- Line 448: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (exclude_set.find(f) == exclude_set.end()) {
- Line 457: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (exclude_set.find(f) == exclude_set.end()) {
- Line 457: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (exclude_set.find(f) == exclude_set.end()) {
- Line 469: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (exclude_set.find(kv.first) == exclude_set.end()) {
- Line 470: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (exclude_set.find(kv.first) == exclude_set.end()) {
  Confidence: band=very_high; score=0.9
- Line 480: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (exclude_set.find(hint.name) == exclude_set.end()) {
- Line 499: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(errors::ErrorCode::ERR_EXPORT_TENANT_UNAUTHORIZED,
- Line 509: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ConfigException("output_path must not be empty", "output_path");
- Line 592: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to open output file: " + open_status.status().ToString(), options.ou
- Line 614: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to create Parquet writer: " + writer_result.status().ToString(),
- Line 697: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(errors::ErrorCode::ERR_EXPORT_IO_ERROR,
- Line 812: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(errors::ErrorCode::ERR_EXPORT_IO_ERROR,
- Line 824: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: col_data[ci].push_back(val);
- Line 847: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Cannot open output file for writing", options.output_path, errno);
- Line 858: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: fmi.kv_metadata[kv.first] = kv.second;
- Line 928: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: fmi.kv_metadata[kv.first] = kv.second;
- Line 64: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>(v >> 8));
- Line 80: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buf.push_back(static_cast<uint8_t>((u >> s) & 0xFF));
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(static_cast<uint8_t>((u >> s) & 0xFF));
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(field_type);
- Line 99: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: buf.push_back(T_STOP);
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: value_buf.push_back(static_cast<uint8_t>(len & 0xFF));
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: value_buf.push_back(static_cast<uint8_t>(len & 0xFF));
- Line 367: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: value_buf.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
- Line 368: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: value_buf.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
- Line 369: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: value_buf.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
- Line 449: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 450: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cols.push_back(f);
- Line 458: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(f);
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cols.push_back(f);
- Line 481: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(hint.name);
  Confidence: band=high; score=0.74
- Line 481: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(hint.name);
  Confidence: band=high; score=0.74
- Line 481: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cols.push_back(hint.name);
  Confidence: band=high; score=0.74
- Line 482: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cols.push_back(hint.name);
- Line 583: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(arrow::field(col, dt, true /* nullable */));
  Confidence: band=high; score=0.74
- Line 583: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(arrow::field(col, dt, true /* nullable */));
  Confidence: band=high; score=0.74
- Line 584: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(arrow::field(col, dt, true /* nullable */));
- Line 641: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 641: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arrays.push_back(arr);
  Confidence: band=high; score=0.74
- Line 642: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arrays.push_back(arr);
- Line 878: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.errors.push_back(err_code + ": Cannot open output file: " + options.output_path);
- Line 945: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ofs.close();

### src/exporters/export_format_registry.cpp
Total findings: 37

- Line 65: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("jsonl", []() -> std::unique_ptr<IExporter> { return std::make_unique<JSONLLLMExporte
- Line 65: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("jsonl", []() -> std::unique_ptr<IExporter> { return std::make_unique<JSONLLLMExporte
- Line 66: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("llm_jsonl", []() -> std::unique_ptr<IExporter> { return std::make_unique<JSONLLLMExp
- Line 66: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("llm_jsonl", []() -> std::unique_ptr<IExporter> { return std::make_unique<JSONLLLMExp
- Line 69: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("parquet", []() -> std::unique_ptr<IExporter> { return std::make_unique<ParquetExport
- Line 69: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("parquet", []() -> std::unique_ptr<IExporter> { return std::make_unique<ParquetExport
- Line 72: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("arrow", []() -> std::unique_ptr<IExporter> {
- Line 72: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("arrow", []() -> std::unique_ptr<IExporter> {
- Line 77: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("arrow_stream", []() -> std::unique_ptr<IExporter> {
- Line 77: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("arrow_stream", []() -> std::unique_ptr<IExporter> {
- Line 85: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: []() -> std::unique_ptr<IExporter> { return std::make_unique<HuggingFaceExporter>(); });
- Line 85: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: []() -> std::unique_ptr<IExporter> { return std::make_unique<HuggingFaceExporter>(); });
- Line 87: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: []() -> std::unique_ptr<IExporter> { return std::make_unique<HuggingFaceExporter>(); });
- Line 87: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: []() -> std::unique_ptr<IExporter> { return std::make_unique<HuggingFaceExporter>(); });
- Line 90: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("streaming", []() -> std::unique_ptr<IExporter> { return std::make_unique<StreamingEx
- Line 90: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("streaming", []() -> std::unique_ptr<IExporter> { return std::make_unique<StreamingEx
- Line 94: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: []() -> std::unique_ptr<IExporter> { return std::make_unique<IncrementalExporter>(); });
- Line 94: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: []() -> std::unique_ptr<IExporter> { return std::make_unique<IncrementalExporter>(); });
- Line 97: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("join", []() -> std::unique_ptr<IExporter> { return std::make_unique<JoinExporter>();
- Line 97: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("join", []() -> std::unique_ptr<IExporter> { return std::make_unique<JoinExporter>();
- Line 98: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("join_jsonl", []() -> std::unique_ptr<IExporter> { return std::make_unique<JoinExport
- Line 98: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("join_jsonl", []() -> std::unique_ptr<IExporter> { return std::make_unique<JoinExport
- Line 101: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("jsonl_alpaca", []() -> std::unique_ptr<IExporter> {
- Line 101: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("jsonl_alpaca", []() -> std::unique_ptr<IExporter> {
- Line 106: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("jsonl_sharegpt", []() -> std::unique_ptr<IExporter> {
- Line 106: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("jsonl_sharegpt", []() -> std::unique_ptr<IExporter> {
- Line 111: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("jsonl_chatml", []() -> std::unique_ptr<IExporter> {
- Line 111: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("jsonl_chatml", []() -> std::unique_ptr<IExporter> {
- Line 116: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat("jsonl_openai_ft", []() -> std::unique_ptr<IExporter> {
- Line 116: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat("jsonl_openai_ft", []() -> std::unique_ptr<IExporter> {
- Line 156: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = kTypeMap.find(type_str);
- Line 191: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto &ve : validated) {
  Confidence: band=very_high; score=0.9
- Line 192: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: registerFormat(ve.format_key, [ttype = ve.ttype, mapping = ve.mapping]() -> std::unique_ptr<IExporte
- Line 192: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: registerFormat(ve.format_key, [ttype = ve.ttype, mapping = ve.mapping]() -> std::unique_ptr<IExporte
- Line 56: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keys.push_back(k);
  Confidence: band=high; score=0.74
- Line 57: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: keys.push_back(k);
- Line 187: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validated.push_back({format_key, it->second, mapping});

### src/exporters/jsonl_llm_exporter.cpp
Total findings: 28

- Line 416: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Optional input field
  Confidence: band=very_high; score=0.99
- Line 417: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (isFieldAllowed(mapping.input_field, options.include_fields, options.exclude_fields)) {
  Confidence: band=very_high; score=0.99
- Line 418: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto input = entity.getFieldAsString(mapping.input_field);
  Confidence: band=very_high; score=0.99
- Line 419: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input && !input->empty()) {
  Confidence: band=very_high; score=0.99
- Line 420: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: j["input"] = *input;
  Confidence: band=very_high; score=0.99
- Line 534: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string line = format_template_->render(entity, config_.template_field_mapping);
- Line 534: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: std::string line = format_template_->render(entity, config_.template_field_mapping);
- Line 557: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = j.begin(); it != j.end();) {
- Line 77: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(themis::errors::ErrorCode::ERR_EXPORT_POLICY_DENIED,
- Line 104: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(themis::errors::ErrorCode::ERR_EXPORT_TENANT_UNAUTHORIZED,
- Line 226: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(themis::errors::ErrorCode::ERR_EXPORT_PII_VIOLATION,
- Line 416: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Optional input field
  Confidence: band=very_high; score=0.9
- Line 417: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (isFieldAllowed(mapping.input_field, options.include_fields, options.exclude_fields)) {
  Confidence: band=very_high; score=0.9
- Line 418: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input = entity.getFieldAsString(mapping.input_field);
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input && !input->empty()) {
  Confidence: band=very_high; score=0.9
- Line 420: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: j["input"] = *input;
  Confidence: band=very_high; score=0.9
- Line 677: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // The style-based path is unchanged for backward compatibility.
  Confidence: band=high; score=0.8
- Line 703: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Skip empty outputs
  Confidence: band=very_high; score=0.9
- Line 704: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (quality.skip_empty_outputs && (!output || output->empty())) {
  Confidence: band=very_high; score=0.9
- Line 842: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // LoRA Adapter Metadata (LoRAExchange.ai compatibility)
  Confidence: band=high; score=0.8
- Line 863: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["training"] = {{"dataset_name", train.dataset_name},
- Line 247: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.errors.push_back("Schema validation failed for " + entity.getPrimaryKey() + ": "
- Line 341: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: writer.close();
- Line 451: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: messages.push_back({{"role", "system"}, {"content", *system}});
- Line 810: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: runtime_metrics_.validation_errors.push_back(err_msg);
  Confidence: band=high; score=0.74
- Line 811: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: runtime_metrics_.validation_errors.push_back(err_msg);
- Line 1048: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["schema_validation"]["recent_errors"].push_back(runtime_metrics_.validation_errors[i]);
  Confidence: band=high; score=0.74
- Line 1049: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["schema_validation"]["recent_errors"].push_back(runtime_metrics_.validation_errors[i]);

### src/exporters/huggingface_exporter.cpp
Total findings: 19

- Line 93: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to open dataset_info.json for writing", info_path.string());
- Line 117: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: THEMIS_INFO("HuggingFace export completed: {} entities -> {} ({}ms)", stats.exported_entities, datas
- Line 164: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &feat : resolvedFeatures()) {
- Line 174: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: split_obj["dataset_name"] = resolved_name;
- Line 178: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: info["dataset_size"]  = byte_count;
- Line 249: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &feat : resolvedFeatures()) {
- Line 265: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &feat : resolvedFeatures()) {
- Line 321: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (field_dtypes.find(field_name) == field_dtypes.end()) {
  Confidence: band=very_high; score=0.9
- Line 49: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.errors.push_back("output_path must be set to the dataset root directory");
- Line 200: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\"";
- Line 202: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\\\";
- Line 204: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\n";
- Line 206: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\r";
- Line 208: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += "\\t";
- Line 317: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::string> field_dtypes;
  Confidence: band=high; score=0.74
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred_features_.push_back(std::move(feat));
  Confidence: band=high; score=0.74
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred_features_.push_back(std::move(feat));
  Confidence: band=high; score=0.74
- Line 330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inferred_features_.push_back(std::move(feat));
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: inferred_features_.push_back(std::move(feat));

### src/exporters/huggingface_hub_client.cpp
Total findings: 16

- Line 206: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto token_bytes = config_.key_provider->getKey(config_.hf_token_kek_id);
- Line 407: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto decision = config_.policy_engine->checkExportPermission(req);
- Line 493: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!file_backoff.wait()) {
- Line 590: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto decision = config_.policy_engine->checkExportPermission(req);
- Line 658: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: if (!shard_backoff.wait()) {
- Line 61: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: s->append(data, sz * nmemb);
- Line 102: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: hdrs->append(buffer, size * nitems);
- Line 202: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HubUploadConfig::hf_token_kek_id is set but key_provider is null");
- Line 208: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HubUploadConfig::hf_token_kek_id '" + config_.hf_token_kek_id
- Line 454: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &entry : fs::recursive_directory_iterator(dataset_dir)) {
- Line 543: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(sleep_secs));
- Line 713: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(sleep_secs));
- Line 163: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 333: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: f.close();
- Line 455: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: files.push_back(entry.path().string());
  Confidence: band=high; score=0.74
- Line 456: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: files.push_back(entry.path().string());

### src/exporters/stream_writer.cpp
Total findings: 15

- Line 0: severity=CRITICAL; category=uncategorized
  Context: ['            compressed_bytes_written_ += size;', '        } else {', '            std::memcpy(buffer_.data() + buffer_pos_, data, size);', '            buffer_pos_ += size;', '        }']
  Confidence: band=very_high; score=0.93
- Line 44: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void StreamWriter::write(const std::string& data) {
- Line 45: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: write(data.data(), data.size());
- Line 48: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: void StreamWriter::write(const char* data, size_t size) {
- Line 24: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException(
- Line 118: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // GZIP is accepted for backward compatibility but produces ZSTD output.
  Confidence: band=high; score=0.8
- Line 123: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to create zstd compression stream", config_.output_path, 0);
- Line 132: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("Failed to initialize zstd compression stream",
- Line 137: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("ZSTD compression not available (not compiled with THEMIS_HAS_ZSTD)",
- Line 150: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: void StreamWriter::compressAndWrite([[maybe_unused]] const char* data, [[maybe_unused]] size_t size)
- Line 160: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("ZSTD compression stream error", config_.output_path,
- Line 38: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close();
- Line 39: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 102: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: void StreamWriter::close() {
- Line 113: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file_.close();

### src/exporters/data_augmentation.cpp
Total findings: 12

- Line 36: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: {"get", {"obtain", "retrieve", "acquire"}},
- Line 182: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = combined.find(lw);
- Line 244: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &field : selectFields(entity)) {
- Line 264: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &field : selectFields(entity)) {
- Line 275: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &field : selectFields(entity)) {
- Line 286: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &field : selectFields(entity)) {
- Line 113: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += ' ';
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += ' ';
- Line 159: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> combined;
  Confidence: band=high; score=0.74
- Line 231: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(kv.first);
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(kv.first);
- Line 330: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: copies.push_back(std::move(copy));

### src/exporters/pii_detector.cpp
Total findings: 11

- Line 61: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matches.push_back(match);
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matches.push_back(match);
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 88: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matches.push_back(match);
- Line 100: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 101: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matches.push_back(match);
- Line 113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(match);
  Confidence: band=high; score=0.74
- Line 114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matches.push_back(match);
- Line 205: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += std::string(value.length() - config_.partial_keep_prefix - config_.partial_keep_suffix, '*');
  Confidence: band=high; score=0.74

### src/exporters/join_exporter.cpp
Total findings: 7

- Line 33: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 54: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 81: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 88: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 171: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 278: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(
- Line 323: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExporterException(

### src/exporters/streaming_exporter.cpp
Total findings: 4

- Line 219: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw ExportIOException("ExportEncryption: rename failed: " + ec.message(), options.output_path);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    }', '    size_t offset = 0;', '    f >> offset;', '    return f ? offset : 0;', '}']
  Confidence: band=medium; score=0.65
- Line 144: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: line += '\n';
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: writer.close();

### src/exporters/aql_predicate_filter.cpp
Total findings: 2

- Line 30: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AqlPredicateFilterException("Failed to parse AQL predicate '" + predicate
- Line 36: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw AqlPredicateFilterException("AQL predicate '" + predicate + "' produced no filter conditions")

### src/exporters/exporter_metrics.cpp
Total findings: 2

- Line 347: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "  Export Rate: " << getExportRate() << " entities/sec\n";
- Line 348: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "  Throughput: " << getThroughput() << " bytes/sec\n";

### src/exporters/incremental_exporter.cpp
Total findings: 1

- Line 159: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: writer.close();

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
