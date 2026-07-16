### Context

This issue implements the roadmap item '`SecuritySignatureManager`: Implement RocksDB Iteration' for the storage domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `SecuritySignatureManager`: Implement RocksDB Iteration

### Goal

Deliver the scoped changes for `SecuritySignatureManager`: Implement RocksDB Iteration in src/storage/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `SecuritySignatureManager`: Implement RocksDB Iteration
**Priority:** Medium
**Target Version:** v1.8.0

`security_signature_manager.cpp` line 110: "TODO: Implement proper RocksDB iteration when `RocksDBWrapper` supports it". Without iteration, the signature manager cannot verify integrity across all stored records.

**Implementation Notes:**
- `[ ]` Add `RocksDBWrapper::iterateRange(start_key, end_key, callback)` that uses a `rocksdb::Iterator` under the hood.
- `[ ]` Wire into `SecuritySignatureManager::verifyAll()` to scan all document keys and verify their signatures in sequence.

---

### Acceptance Criteria

- [ ] Add `RocksDBWrapper::iterateRange(start_key, end_key, callback)` that uses a `rocksdb::Iterator` under the hood.
- [ ] Wire into `SecuritySignatureManager::verifyAll()` to scan all document keys and verify their signatures in sequence.

### Relationships

- Roadmap row: #206 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/storage/FUTURE_ENHANCEMENTS.md#securitysignaturemanager-implement-rocksdb-iteration
- Source key: roadmap:206:storage:v1.8.0:securitysignaturemanager-implement-rocksdb-iteration

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:206:storage:v1.8.0:securitysignaturemanager-implement-rocksdb-iteration -->
<!-- roadmap-ref: row=206;module=storage;target=v1.8.0 -->
<!-- roadmap-detail: src/storage/FUTURE_ENHANCEMENTS.md#securitysignaturemanager-implement-rocksdb-iteration -->
