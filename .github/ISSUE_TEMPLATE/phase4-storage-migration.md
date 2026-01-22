---
name: Phase 4 - Storage Layer Migration
about: Track storage layer error handling migration to Result<T> pattern
title: '[Phase 4] Storage Layer Migration'
labels: ['error-handling', 'phase-4', 'storage', 'refactoring']
assignees: ''
---

## 📋 Module: Storage Layer

**Priority:** P1 (High)  
**Estimated Effort:** 4-5 weeks  
**Complexity:** Medium-High  
**Dependencies:** Phase 4 Foundation PR must be merged

## 🎯 Objective

Migrate storage layer error handling from legacy patterns (`return nullptr`, custom Status structs) to unified `Result<T>` pattern using `tl::expected`.

## 📊 Scope

### Functions to Migrate

**RocksDB Wrapper** (6 remaining nullptr sites):
- [ ] `newAsyncIterator()` - 2 nullptr returns → `Result<unique_ptr<Iterator>>`
- [ ] `newIterator()` - 2 nullptr returns → `Result<unique_ptr<Iterator>>`
- [ ] `newSafeIterator()` - 1 nullptr return → `Result<SafeIterator>`
- [ ] ~~`getOrCreateColumnFamily()`~~ ✅ Complete (Foundation PR)

**Blob Redundancy Manager** (14 functions):
- [ ] `createRocksDBListener()` - 1 nullptr → `Result<shared_ptr<EventListener>>`
- [ ] `ensureRedundancy()` - BlobOperationResult → `Result<void>`
- [ ] `repairBlob()` - BlobOperationResult → `Result<void>`
- [ ] `writeBlob()` - BlobOperationResult → `Result<void>`
- [ ] `readBlob()` - optional → `Result<vector<uint8_t>>`
- [ ] `deleteBlob()` - BlobOperationResult → `Result<void>`
- [ ] Additional 8 functions with BlobOperationResult pattern

**Blob Backends** (16 functions - 4 files × 4 functions each):
- [ ] `blob_backend_filesystem.cpp` - 4 functions (exception-based → Result<T>)
- [ ] `blob_backend_s3.cpp` - 4 functions (exception-based → Result<T>)
- [ ] `blob_backend_azure.cpp` - 4 functions (exception-based → Result<T>)
- [ ] `blob_backend_webdav.cpp` - 4 functions (exception-based → Result<T>)

**Total:** 8 nullptr sites + 33 Status/Result returns = **41 migration points**

## 📚 Resources

**Foundation Documentation:**
- Phase 4 Migration Matrix: `docs/error_handling/phase4_migration_matrix.md`
- Storage Migration Log: `docs/error_handling/phase4_week2_storage_migration.md`
- Migration Example: `docs/error_handling/phase4_week2_getOrCreateColumnFamily_example.md`
- Progress Summary: `docs/error_handling/phase4_progress_summary.md`
- Handoff Document: `docs/error_handling/phase4_final_summary_handoff.md`

**Error Codes Available:**
- `ERR_STORAGE_TRANSACTION_FAILED` (1004)
- `ERR_STORAGE_CACHE_ERROR` (1005)
- `ERR_STORAGE_LOG_FULL` (1006)
- `ERR_STORAGE_REDUNDANCY_FAILED` (1007)
- `ERR_INDEX_NOT_INITIALIZED` (6000)
- `ERR_INDEX_CREATION_FAILED` (6001)
- `ERR_IO_READ_FAILED` (2001)
- `ERR_IO_WRITE_FAILED` (2002)

## 🔧 Implementation Steps

### Phase 1: RocksDB Wrapper (Week 1)
- [ ] Migrate `newAsyncIterator()` and update call sites
- [ ] Migrate `newIterator()` and update call sites
- [ ] Migrate `newSafeIterator()` and update call sites
- [ ] Add unit tests for iterator error scenarios
- [ ] Build verification and integration testing

### Phase 2: Blob Redundancy Manager (Week 2-3)
- [ ] Migrate `createRocksDBListener()` and update call sites
- [ ] Migrate blob write operations (`writeBlob`, `ensureRedundancy`, `repairBlob`)
- [ ] Migrate blob read operations (`readBlob`)
- [ ] Migrate blob delete operations (`deleteBlob`)
- [ ] Remove custom `BlobOperationResult` struct
- [ ] Add unit tests for redundancy failure scenarios
- [ ] Build verification and integration testing

### Phase 3: Blob Backends (Week 3-4)
- [ ] Migrate filesystem backend (4 functions)
- [ ] Migrate S3 backend (4 functions)
- [ ] Migrate Azure backend (4 functions)
- [ ] Migrate WebDAV backend (4 functions)
- [ ] Convert exception-based error handling to Result<T>
- [ ] Add unit tests for backend-specific failures
- [ ] Build verification and integration testing

### Phase 4: Testing & Validation (Week 4-5)
- [ ] Update ~15 existing test files
- [ ] Add transaction failure tests
- [ ] Add cache eviction tests
- [ ] Add blob redundancy failure tests
- [ ] Performance benchmarking (ensure <5% regression)
- [ ] Code review and refinement
- [ ] Documentation updates

## ✅ Acceptance Criteria

- [ ] All 41 storage functions migrated to `Result<T>` pattern
- [ ] All call sites updated to use Result<T> checks
- [ ] All try-catch blocks converted to Result pattern where applicable
- [ ] Custom `BlobOperationResult` struct removed
- [ ] Zero build warnings or errors
- [ ] All unit tests passing
- [ ] Integration tests passing
- [ ] Performance regression <5%
- [ ] Code review approved
- [ ] Documentation updated

## 📝 Migration Pattern

```cpp
// BEFORE: nullptr pattern
Iterator* newIterator() {
    if (!db_) return nullptr;
    auto* iter = db_->NewIterator(options);
    if (!iter) return nullptr;
    return iter;
}

// AFTER: Result<T> pattern
Result<std::unique_ptr<Iterator>> newIterator() {
    if (!db_) {
        return Err<std::unique_ptr<Iterator>>(
            ERR_INDEX_NOT_INITIALIZED,
            "RocksDB not opened for iterator creation"
        );
    }
    auto* iter = db_->NewIterator(options);
    if (!iter) {
        return Err<std::unique_ptr<Iterator>>(
            ERR_INDEX_CREATION_FAILED,
            "Failed to create RocksDB iterator"
        );
    }
    return Ok(std::unique_ptr<Iterator>(iter));
}

// Call site update
auto iter_result = storage->newIterator();
if (iter_result) {
    auto iter = std::move(*iter_result);
    // use iter
} else {
    LOG_ERROR("Iterator creation failed: {}", iter_result.error().message());
    return iter_result.error();
}
```

## 🔗 Related Issues

- Depends on: Phase 4 Foundation PR
- Blocks: Phase 4 Query Engine Migration
- Blocks: Phase 4 LLM/LoRA Migration

## 📊 Progress Tracking

**Week 1:** ⬜⬜⬜⬜⬜ 0%  
**Week 2:** ⬜⬜⬜⬜⬜ 0%  
**Week 3:** ⬜⬜⬜⬜⬜ 0%  
**Week 4:** ⬜⬜⬜⬜⬜ 0%  
**Week 5:** ⬜⬜⬜⬜⬜ 0%

**Overall:** 0 of 41 functions migrated (0%)

## 💬 Notes

- Pattern established in foundation PR: `getOrCreateColumnFamily()` migration
- Use documentation extensively - all patterns and examples provided
- Coordinate with Query Engine team on shared dependencies
- Weekly sync recommended for consistency

---
**Assigned to:** TBD  
**Started:** TBD  
**Target Completion:** TBD  
**Actual Completion:** TBD
