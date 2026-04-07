<!-- Status: current | validated: 2026-04-06 -->

# Utils — Security

## Scope
Security considerations for the public headers in `include/utils/`. This module provides cryptographic helpers, PII handling, safe numeric operations, and observability primitives consumed by all ThemisDB modules.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| PII leakage via logs | Privacy violation / regulatory breach | `pii_redacting_sink.h` redacts before emission; `PiiDetectionEngine` covers regex + NER |
| ReDoS via PII regex patterns | DoS on detection path | Audit planned (Q2 2026); linear-time regex alternatives tracked |
| HKDF salt with low entropy | Weak derived keys | `hkdf_helper.h` requires caller-supplied salt; document minimum 16 bytes |
| Decompression bomb via ZSTD | Memory exhaustion | `zstd_codec.h` should enforce max output size (tracked in ROADMAP) |
| Integer overflow in arithmetic | Memory corruption / logic errors | `safe_arithmetic.h` provides checked operations; use exclusively in numeric paths |
| UUID collision from weak PRNG | Security token predictability | `uuid.h` must use CSPRNG; implementation audited |
| Unaligned memory access | Crash / undefined behaviour | `unaligned_access.h` provides portable wrappers; use instead of raw pointer cast |
| OpenSSL resource leaks | Denial of service | `openssl_deleter.h` RAII wrappers; mandatory for all OpenSSL object lifetimes |
| Insecure LEK storage | Key material exfiltration | `lek_manager.h` zeroizes keys on destroy; memory region reviewed |
| Thread safety violations | Data corruption | `thread_safety.h` RAII guards; `concurrent_cache.h` internally synchronized |

## Security Controls

- **PII pipeline:** Three-layer detection (regex, NER, pseudonymization) with a log-level redacting sink.
- **Safe numerics:** All arithmetic on untrusted-size data uses `safe_arithmetic.h` / `safe_cast.h`.
- **HKDF:** Key derivation uses HKDF-SHA256; raw keys never stored in logs or error messages.
- **RAII for crypto objects:** `openssl_deleter.h` prevents OpenSSL handle leaks.
- **Input validation:** `input_validator.h` applied at all public API entry points consuming external data.
- **Tracing:** `tracing.h` spans do not include PII; verified via `pii_redacting_sink.h` integration.

## Known Limitations

- **LIMITATION-UTILS-01 (Medium):** `regex_detection_engine.h` patterns have not been formally audited for ReDoS. Mitigation: use bounded-backtrack or RE2-compatible engine; audit Q2 2026.
- **LIMITATION-UTILS-02 (Low):** `zstd_codec.h` does not yet enforce a maximum decompression output size in the public API. Callers must implement their own limit until Q2 2026.
- **LIMITATION-UTILS-03 (Info):** `hkdf_cache.h` cache eviction policy may retain derived keys in memory longer than necessary. Review planned in Phase 5.
