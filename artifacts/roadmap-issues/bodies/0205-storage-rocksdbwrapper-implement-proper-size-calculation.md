### Context

This issue implements the roadmap item '`RocksDBWrapper`: Implement Proper Size Calculation' for the storage domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `RocksDBWrapper`: Implement Proper Size Calculation

### Goal

Deliver the scoped changes for `RocksDBWrapper`: Implement Proper Size Calculation in src/storage/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `RocksDBWrapper`: Implement Proper Size Calculation
**Priority:** Medium
**Target Version:** v1.8.0

`rocksdb_wrapper.cpp` line 1445: "TODO: Implement proper size calculation". The `RocksDBWrapper::getApproximateSize()` or equivalent method returns 0 or a placeholder, making disk-space monitoring, compaction triggers, and admin API storage metrics unreliable.

**Implementation Notes:**
- `[ ]` Use `rocksdb::DB::GetApproximateSizes()` API to compute the on-disk SST file sizes for a key range.
- `[ ]` Alternatively, use `rocksdb::DB::GetIntProperty(rocksdb::DB::Properties::kTotalSstFilesSize)` for total CF size.
- `[ ]` Wire the result into `DiskSpaceMonitor` and the `/v1/admin/storage/stats` endpoint.

---

### Acceptance Criteria

- [ ] Use `rocksdb::DB::GetApproximateSizes()` API to compute the on-disk SST file sizes for a key range.
- [ ] Alternatively, use `rocksdb::DB::GetIntProperty(rocksdb::DB::Properties::kTotalSstFilesSize)` for total CF size.
- [ ] Wire the result into `DiskSpaceMonitor` and the `/v1/admin/storage/stats` endpoint.

### Relationships

- Roadmap row: #205 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/storage/FUTURE_ENHANCEMENTS.md#rocksdbwrapper-implement-proper-size-calculation
- Source key: roadmap:205:storage:v1.8.0:rocksdbwrapper-implement-proper-size-calculation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:205:storage:v1.8.0:rocksdbwrapper-implement-proper-size-calculation -->
<!-- roadmap-ref: row=205;module=storage;target=v1.8.0 -->
<!-- roadmap-detail: src/storage/FUTURE_ENHANCEMENTS.md#rocksdbwrapper-implement-proper-size-calculation -->
