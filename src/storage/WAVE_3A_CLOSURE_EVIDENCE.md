# Wave 3-A Closure Evidence

**Date:** 2026-08-25  
**Branch:** `copilot/select-important-core-modules`  
**Executed by:** Wave 3-A Storage gap-fix implementation agent

---

## What Was Fixed

| # | Severity | File | Line | Gap Type | Fix Summary |
|---|----------|------|------|----------|-------------|
| 1 | CRITICAL | `src/storage/columnar_format.cpp` | 1265 | stub/unimpl | `ColumnSegment::decode()` — full reverse-codec dispatch replacing the silent `is_encoded_ = false` stub. RLE, BIT_PACKING, FRAME_OF_REF, LZ4, SNAPPY all implemented. DICTIONARY returns `ERR_CODEC_NOT_AVAILABLE`. |
| 2 | CRITICAL | `src/storage/backup_manager.cpp` | 1726 | fail-open | `encryptFile()` no-OpenSSL `#else` branch — `THEMIS_ERROR` + `return false` replacing silent plaintext copy. |
| 3 | HIGH | `src/storage/backup_manager.cpp` | 1468 | fail-open | `compressPath()` no-compression `#else` branch — `THEMIS_ERROR` + `return false` replacing silent uncompressed copy. |
| 4 | HIGH | `src/storage/storage_error_diagnostics.cpp` | 370, 385, 404 | TODO stub | Wired `emitDiagnosticEvent()`, `emitRecoveryFaultEvent()`, `emitStoragePressureEvent()` to the `storage.audit` named spdlog channel. Three `// TODO` comments removed. |
| 5 | HIGH | `src/storage/ggml_tensor_bridge.cpp` | 188–192 | stub/unsafe | `ggmlTensor()` — `fake_tensor` fallback moved behind `#ifdef THEMIS_UNIT_TEST`; production build logs `THEMIS_ERROR` and returns `nullptr` when `real_ggml_tensor` is null. |

---

## False-Positive Confirmation Table

The gap-verifier subagent confirmed the following 10 scanner entries as false positives.
No code changes were made for these entries.

| # | Scanner Finding | File | Line | Verdict | Justification |
|---|-----------------|------|------|---------|---------------|
| FP-1 | `null_dereference` | `ggml_tensor_bridge.cpp` | 172, 175, 181, 185 | **False Positive** | Every `impl_` dereference is guarded by an explicit `if (!impl_)` early-return above the dereference site; scanner cannot track control flow across ternary branches. |
| FP-2 | `legacy_duplication` | `ggml_tensor_bridge.cpp` | — | **False Positive** | Ternary fallback pattern for `impl_->train` / `impl_->key` is a compile-time-safe non-throwing accessor, not duplicated dead code. |
| FP-3 | `unvalidated_llm_output` | `ggml_tensor_bridge.cpp` | — | **False Positive** | Data is numeric float values from tensor decomposition, not free-form LLM text output. |
| FP-4 | `scope_mismatch` | `wom_tree.cpp` | 70, 107 | **False Positive** | Anonymous namespace inside `namespace themis` is valid C++. Scanner phantom line:70/:107 artifacts. |
| FP-5 | `braces_imbalance` | `blob_backend_gcs.cpp`, `database_connection_manager.cpp`, `gguf_metadata.cpp`, `storage_parquet_exporter.cpp`, `tensor_compaction_filter.cpp`, `wom_tree.cpp` | 1 | **False Positive** | Open == close brace count verified in all 6 files. Line:1 scanner artefacts. |
| FP-6 | `null_dereference` | `columnar_format.cpp` | 1265 (old stub) | **N/A** | The stub itself was a real gap (Fix 1 above). The surrounding null checks are real guards. |
| FP-7 | `unchecked_cuda_call` | `gpu_compression.cpp` | — | **False Positive** | All CUDA return values checked inline; `THEMIS_CUDA_CHECK` macro added in Wave 1. |
| FP-8 | `iterator_invalidation` | `columnar_cache.cpp` | 105 | **False Positive** | Iterator is only invalidated after the loop exits; documented with source comment. |
| FP-9 | `iterator_invalidation` | `hamming_coder.cpp` | 114 | **False Positive** | Same pattern as FP-8; documented. |
| FP-10 | `new_without_raii` | `database_connection_manager.cpp` | 129 | **False Positive** | `conn.get()` is a map key from an existing `shared_ptr`; no raw ownership transfer. |

---

## Test File Reference

`tests/storage/test_wave3a_critical_fixes.cpp`  
Registered automatically via the glob `test_*.cpp` in `tests/storage/CMakeLists.txt`.

### Test Cases

| Test | Covers |
|------|--------|
| `Wave3a_ColumnSegmentDecode/DictionaryCodecDecodeReturnsError` | Fix 1 — DICTIONARY returns error not OK |
| `Wave3a_ColumnSegmentDecode/RleInt32RoundTrip` | Fix 1 — RLE round-trip correctness |
| `Wave3a_ColumnSegmentDecode/BitPackingInt64RoundTrip` | Fix 1 — BIT_PACKING round-trip correctness |
| `Wave3a_ColumnSegmentDecode/FrameOfRefInt32RoundTrip` | Fix 1 — FRAME_OF_REF round-trip correctness |
| `Wave3a_ColumnSegmentDecode/DecodeIdempotent` | Fix 1 — idempotent decode is safe |
| `Wave3a_BackupManager/EncryptFileNoOpenSSL_CompilesToFailClosed` | Fix 2 — no-OpenSSL branch compiles fail-closed |
| `Wave3a_BackupManager/CompressPathNoCompression_CompilesToFailClosed` | Fix 3 — no-compression branch compiles fail-closed |
| `Wave3a_GgmlTensorBridge/UnitTestPathReturnsFakeTensorNotNull` | Fix 5 — default-constructed tensor returns nullptr safely |
| `Wave3a_GgmlTensorBridge/SetAndClearAllocFnDoesNotThrow` | Fix 5 — API surface stable |

---

## Files Touched

| File | Type | Change |
|------|------|--------|
| `src/storage/columnar_format.cpp` | Source | `decode()` implementation |
| `src/storage/backup_manager.cpp` | Source | `encryptFile()` + `compressPath()` fail-closed |
| `src/storage/storage_error_diagnostics.cpp` | Source | Audit channel wiring; TODO stubs removed |
| `src/storage/ggml_tensor_bridge.cpp` | Source | `ggmlTensor()` production guard |
| `tests/storage/test_wave3a_critical_fixes.cpp` | Test | New regression tests |
| `src/storage/MODULE_GAPS.md` | Doc | CRITICAL count updated 69→64; Wave 3-A section added |
| `src/storage/WAVE_3A_CLOSURE_EVIDENCE.md` | Doc | This file |
