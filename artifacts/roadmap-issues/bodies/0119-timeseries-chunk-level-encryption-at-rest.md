### Context

This issue implements the roadmap item 'Chunk-Level Encryption at Rest' for the timeseries domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: [ ] Chunk-Level Encryption at Rest

### Goal

Deliver the scoped changes for Chunk-Level Encryption at Rest in src/timeseries/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### [ ] Chunk-Level Encryption at Rest
**Priority:** Medium
**Target Version:** v1.0.0

Add AES-256-GCM encryption to individual time series chunks in `tsstore.cpp` using data encryption keys derived by `utils/hkdf_helper.cpp` and managed by `utils/lek_manager.cpp`. Encryption must be transparent to the query path; chunks are decrypted on-demand during scan.

**Implementation Notes:**
- Add `EncryptedChunkStore` wrapper in `tsstore.cpp` that intercepts chunk write/read operations and applies AES-256-GCM using keys fetched from `utils/lek_manager.cpp` by series ID.
- Key rotation must re-encrypt affected chunks in a background job without blocking reads; implement in `ts_encrypted_key_rotation.cpp`.
- Gorilla-compressed data must be encrypted after compression (compress-then-encrypt) to maximise compression ratio and avoid information leakage from cipher input patterns.
- Audit every key access via `utils/audit_logger.cpp` with series ID, chunk range, and accessor identity.

**Performance Targets:**
- Encryption overhead on write path: <5% throughput reduction vs. unencrypted baseline.
- AES-256-GCM throughput per core: >1 GB/s (AES-NI assisted via OpenSSL EVP).

---

### Acceptance Criteria

- [ ] Add `EncryptedChunkStore` wrapper in `tsstore.cpp` that intercepts chunk write/read operations and applies AES-256-GCM using keys fetched from `utils/lek_manager.cpp` by series ID.
- [ ] Key rotation must re-encrypt affected chunks in a background job without blocking reads; implement in `ts_encrypted_key_rotation.cpp`.
- [ ] Gorilla-compressed data must be encrypted after compression (compress-then-encrypt) to maximise compression ratio and avoid information leakage from cipher input patterns.
- [ ] Audit every key access via `utils/audit_logger.cpp` with series ID, chunk range, and accessor identity.
- [ ] Encryption overhead on write path: <5% throughput reduction vs. unencrypted baseline.
- [ ] AES-256-GCM throughput per core: >1 GB/s (AES-NI assisted via OpenSSL EVP).

### Relationships

- Roadmap row: #119 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/timeseries/FUTURE_ENHANCEMENTS.md#-chunk-level-encryption-at-rest
- Source key: roadmap:119:timeseries:v1.7.0:chunk-level-encryption-at-rest

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:119:timeseries:v1.7.0:chunk-level-encryption-at-rest -->
<!-- roadmap-ref: row=119;module=timeseries;target=v1.7.0 -->
<!-- roadmap-detail: src/timeseries/FUTURE_ENHANCEMENTS.md#-chunk-level-encryption-at-rest -->
