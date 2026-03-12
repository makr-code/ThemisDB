### Context

This issue implements the roadmap item 'Zero-Copy Blob Transfers' for the storage domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.7.0.

Primary detail section: Zero-Copy Blob Transfers

### Goal

Deliver the scoped changes for Zero-Copy Blob Transfers in src/storage/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Zero-Copy Blob Transfers
**Priority:** Medium  
**Target Version:** v1.7.0

Eliminate memory copies when transferring blobs between backends.

**Techniques:**
- `sendfile()` for local filesystem
- S3 multipart upload with streaming
- Memory-mapped files for large blobs

**Expected Improvement:** 40-60% faster blob transfers

---

### Acceptance Criteria

- [ ] `sendfile()` for local filesystem
- [ ] S3 multipart upload with streaming
- [ ] Memory-mapped files for large blobs

### Relationships

- Roadmap row: #231 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/storage/FUTURE_ENHANCEMENTS.md#zero-copy-blob-transfers
- Source key: roadmap:231:storage:v1.7.0:zero-copy-blob-transfers

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:231:storage:v1.7.0:zero-copy-blob-transfers -->
<!-- roadmap-ref: row=231;module=storage;target=v1.7.0 -->
<!-- roadmap-detail: src/storage/FUTURE_ENHANCEMENTS.md#zero-copy-blob-transfers -->
