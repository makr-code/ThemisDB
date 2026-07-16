### Context

This issue implements the roadmap item '`PKIClient`: Replace Fallback Stub Verification' for the utils domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `PKIClient`: Replace Fallback Stub Verification

### Goal

Deliver the scoped changes for `PKIClient`: Replace Fallback Stub Verification in src/utils/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `PKIClient`: Replace Fallback Stub Verification
**Priority:** High
**Target Version:** v1.8.0

`pki_client.cpp` has 2 stub fallback paths:
- Line 456: "Fallback: stub behavior (base64 of hash)" — certificate issuance falls back to a non-standard base64 encoding instead of real PKCS#10 / X.509 certificate.
- Line 575: "Fallback stub verification: compare base64(hash) equality" — TLS certificate verification falls back to comparing base64 hashes instead of validating the certificate chain.

**Implementation Notes:**
- `[ ]` Line 456: implement real PKCS#10 CSR generation and submission using OpenSSL `X509_REQ_*` API; only fall back when ACME/internal CA is not configured.
- `[ ]` Line 575: implement real X.509 chain verification using `X509_verify_cert()` with the configured trust store; never fall back to hash comparison for production traffic.
- `[ ]` Add explicit `#ifdef THEMIS_TEST_MODE` guard around the stub paths so they cannot be used in production builds.

---


**Priority:** High
**Target Version:** v0.9.0

Refactor `pii_detection_engine.cpp` and `pii_pseudonymizer.cpp` to operate on a chunked streaming interface so that large legal documents (>100 MB) can be processed without full in-memory buffering. Entity spans that straddle chunk boundaries must be detected and merged correctly.

**Implementation Notes:**
- Define a `PIIStreamScanner` class in `pii_detection_engine.cpp` with `scan_chunk(chunk, is_last)` → `PIISpanList`; internally maintains a lookahead buffer sized to the maximum entity length (configurable, default 256 bytes) to handle cross-boundary spans.
- `pii_pseudonymizer.cpp` adds a companion `PIIStreamPseudonymizer::process_chunk()` that applies replacements using the span offsets from `PIIStreamScanner`; replacements are deterministic per `(entity_text, tenant_id)` using HMAC-SHA-256 keyed with the tenant pseudonymisation key from `lek_manager.cpp`.
- The `regex_detection_engine.cpp` must also support chunk-boundary-aware matching; cross-chunk regex detection uses a sliding window overlap equal to `max_pattern_length`.
- Add a throughput benchmark in `benchmarks/` measuring end-to-end scan+pseudonymise throughput on 100 MB synthetic legal text.

**Performance Targets:**
- Streaming PII scan throughput: >100 MB/s per core for English legal text.
- Memory footprint during streaming scan of 1 GB document: <10 MB.

---

### Acceptance Criteria

- [ ] Line 456: "Fallback: stub behavior (base64 of hash)" — certificate issuance falls back to a non-standard base64 encoding instead of real PKCS#10 / X.509 certificate.
- [ ] Line 575: "Fallback stub verification: compare base64(hash) equality" — TLS certificate verification falls back to comparing base64 hashes instead of validating the certificate chain.
- [ ] Line 456: implement real PKCS#10 CSR generation and submission using OpenSSL `X509_REQ_*` API; only fall back when ACME/internal CA is not configured.
- [ ] Line 575: implement real X.509 chain verification using `X509_verify_cert()` with the configured trust store; never fall back to hash comparison for production traffic.
- [ ] Add explicit `#ifdef THEMIS_TEST_MODE` guard around the stub paths so they cannot be used in production builds.
- [ ] Define a `PIIStreamScanner` class in `pii_detection_engine.cpp` with `scan_chunk(chunk, is_last)` → `PIISpanList`; internally maintains a lookahead buffer sized to the maximum entity length (configurable, default 256 bytes) to handle cross-boundary spans.
- [ ] `pii_pseudonymizer.cpp` adds a companion `PIIStreamPseudonymizer::process_chunk()` that applies replacements using the span offsets from `PIIStreamScanner`; replacements are deterministic per `(entity_text, tenant_id)` using HMAC-SHA-256 keyed with the tenant pseudonymisation key from `lek_manager.cpp`.
- [ ] The `regex_detection_engine.cpp` must also support chunk-boundary-aware matching; cross-chunk regex detection uses a sliding window overlap equal to `max_pattern_length`.
- [ ] Add a throughput benchmark in `benchmarks/` measuring end-to-end scan+pseudonymise throughput on 100 MB synthetic legal text.
- [ ] Streaming PII scan throughput: >100 MB/s per core for English legal text.
- [ ] Memory footprint during streaming scan of 1 GB document: <10 MB.

### Relationships

- Roadmap row: #218 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/utils/FUTURE_ENHANCEMENTS.md#pkiclient-replace-fallback-stub-verification
- Source key: roadmap:218:utils:v1.8.0:pkiclient-replace-fallback-stub-verification

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:218:utils:v1.8.0:pkiclient-replace-fallback-stub-verification -->
<!-- roadmap-ref: row=218;module=utils;target=v1.8.0 -->
<!-- roadmap-detail: src/utils/FUTURE_ENHANCEMENTS.md#pkiclient-replace-fallback-stub-verification -->
