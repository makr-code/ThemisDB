# Phase 4 Week 2-3: Storage Layer Migration - Implementation Log

**Date Started:** 2026-01-19  
**Status:** 🟡 IN PROGRESS  
**Assigned:** @copilot

---

## 📊 Migration Targets

### Files to Migrate

| File | nullptr Returns | Status/Result Returns | Complexity |
|------|-----------------|----------------------|------------|
| rocksdb_wrapper.cpp | 7 | ~20 (rocksdb::Status) | HIGH |
| blob_redundancy_manager.cpp | 1 | 13 (BlobOperationResult) | MEDIUM |
| blob_backend_filesystem.cpp | 0 | exception-based | LOW |
| blob_backend_s3.cpp | 0 | exception-based | LOW |
| blob_backend_azure.cpp | 0 | exception-based | LOW |
| blob_backend_webdav.cpp | 0 | exception-based | LOW |

**Total:** 8 nullptr → 373 legacy patterns to migrate

---

## ✅ Completed Tasks

### 1. Error Code Addition (2026-01-19)

Added 4 new storage error codes to `error_registry.h` and `error_registry.cpp`:

- **ERR_STORAGE_TRANSACTION_FAILED (1004)**
  - Category: Storage
  - Severity: Error
  - Use: Transaction commit/rollback failures

- **ERR_STORAGE_CACHE_ERROR (1005)**
  - Category: Storage
  - Severity: Warning
  - Use: Cache read/write/eviction errors

- **ERR_STORAGE_LOG_FULL (1006)**
  - Category: Storage
  - Severity: Critical
  - Use: WAL capacity exceeded

- **ERR_STORAGE_REDUNDANCY_FAILED (1007)**
  - Category: Storage
  - Severity: Error
  - Use: Redundancy/replication failures

**Commit:** f9ea6e0

---

## 🚧 In Progress

### 2. RocksDB Wrapper Migration

**File:** `src/storage/rocksdb_wrapper.cpp` (1,737 lines)  
**Header:** `include/storage/rocksdb_wrapper.h`

#### Functions to Migrate

##### A. getOrCreateColumnFamily() - 2 nullptr returns

**Current Signature:**
```cpp
rocksdb::ColumnFamilyHandle* getOrCreateColumnFamily(const std::string& name);
```

**Target Signature:**
```cpp
Result<rocksdb::ColumnFamilyHandle*> getOrCreateColumnFamily(const std::string& name);
```

**Error Cases:**
- Line 1209: DB not open → `ERR_INDEX_NOT_INITIALIZED`
- Line 1228: RocksDB Status error → `ERR_INDEX_CREATION_FAILED`

**Migration Pattern:**
```cpp
// Before
if (!db_) {
    THEMIS_ERROR("RocksDB not opened");
    return nullptr;
}

// After
if (!db_) {
    return Err<rocksdb::ColumnFamilyHandle*>(
        ERR_INDEX_NOT_INITIALIZED,
        "RocksDB not opened"
    );
}
```

---

##### B. newAsyncIterator() - 2 nullptr returns

**Current Signature:**
```cpp
std::unique_ptr<rocksdb::Iterator> newAsyncIterator(
    rocksdb::ColumnFamilyHandle* cf = nullptr,
    const rocksdb::ReadOptions* opts = nullptr);
```

**Target Signature:**
```cpp
Result<std::unique_ptr<rocksdb::Iterator>> newAsyncIterator(
    rocksdb::ColumnFamilyHandle* cf = nullptr,
    const rocksdb::ReadOptions* opts = nullptr);
```

**Error Cases:**
- Line 1640: DB not open → `ERR_INDEX_NOT_INITIALIZED`
- Line 1646: Base DB null → `ERR_INDEX_NOT_INITIALIZED`

---

##### C. newIterator() - 2 nullptr returns

**Current Signature:**
```cpp
std::unique_ptr<rocksdb::Iterator> newIterator(
    rocksdb::ColumnFamilyHandle* cf = nullptr,
    const rocksdb::ReadOptions* opts = nullptr);
```

**Target Signature:**
```cpp
Result<std::unique_ptr<rocksdb::Iterator>> newIterator(
    rocksdb::ColumnFamilyHandle* cf = nullptr,
    const rocksdb::ReadOptions* opts = nullptr);
```

**Error Cases:**
- Line 1662: DB not open → `ERR_INDEX_NOT_INITIALIZED`
- Line 1668: Base DB null → `ERR_INDEX_NOT_INITIALIZED`

---

##### D. newSafeIterator() - 1 nullptr return

**Current Signature:**
```cpp
SafeIterator newSafeIterator(
    rocksdb::ColumnFamilyHandle* cf = nullptr,
    const rocksdb::ReadOptions* opts = nullptr);
```

**Target Signature:**
```cpp
Result<SafeIterator> newSafeIterator(
    rocksdb::ColumnFamilyHandle* cf = nullptr,
    const rocksdb::ReadOptions* opts = nullptr);
```

**Error Cases:**
- Line 1682: Guard validity check → Custom return structure needs analysis

---

### 3. Blob Redundancy Manager Migration

**File:** `src/storage/blob_redundancy_manager.cpp` (1,009 lines)

#### Custom Status Struct (to be removed)

```cpp
struct BlobOperationResult {
    bool success;
    std::string error_message;
};
```

**Replace with:** `Result<void>` or `Result<T>`

#### Functions to Migrate

##### A. createRocksDBListener() - 1 nullptr

**Current:** Returns `shared_ptr<EventListener>`  
**Target:** `Result<shared_ptr<EventListener>>`  
**Error Code:** `ERR_STORAGE_CACHE_ERROR`

##### B. ensureRedundancy()

**Current:** Returns `BlobOperationResult`  
**Target:** `Result<void>`  
**Error Code:** `ERR_STORAGE_REDUNDANCY_FAILED`

##### C. repairBlob()

**Current:** Returns `BlobOperationResult`  
**Target:** `Result<void>`  
**Error Code:** `ERR_STORAGE_CORRUPTION`

##### D. writeBlob()

**Current:** Returns `BlobOperationResult`  
**Target:** `Result<void>`  
**Error Codes:** `ERR_STORAGE_DISK_FULL`, `ERR_STORAGE_PERMISSION_DENIED`

##### E. readBlob()

**Current:** Returns `std::optional<vector<uint8_t>>`  
**Target:** `Result<vector<uint8_t>>`  
**Error Code:** `ERR_STORAGE_FILE_NOT_FOUND`

##### F. deleteBlob()

**Current:** Returns `BlobOperationResult`  
**Target:** `Result<void>`  
**Error Code:** `ERR_STORAGE_FILE_NOT_FOUND`

---

### 4. Blob Backend Migrations

#### A. blob_backend_filesystem.cpp

**Error Pattern:** Exception-based + `std::optional`

**Functions:**
- `put()` → Wrap exceptions in `Result<void>`
- `get()` → Convert `std::nullopt` to `Result<vector<uint8_t>>`
- `remove()` → Convert `bool` to `Result<void>`
- `exists()` → Keep as `bool` (not error-related)

**Error Codes:** `ERR_STORAGE_PERMISSION_DENIED`, `ERR_STORAGE_DISK_FULL`

---

#### B. blob_backend_s3.cpp

**Error Pattern:** AWS SDK exceptions

**Functions:**
- `put()` → Wrap `runtime_error` in `Result<void>`
- `get()` → Convert `nullopt` to `Result<vector<uint8_t>>`
- `remove()` → Convert `bool` to `Result<void>`

**Error Codes:** `ERR_NET_CONNECTION_REFUSED`, `ERR_NET_TIMEOUT`

---

#### C. blob_backend_azure.cpp

**Error Pattern:** Azure SDK exceptions

**Functions:**
- `put()` → Wrap exceptions in `Result<void>`
- `get()` → Convert `nullopt` to `Result<vector<uint8_t>>`
- `remove()` → Handle RequestFailedException

**Error Codes:** `ERR_NET_CONNECTION_REFUSED`, `ERR_STORAGE_PERMISSION_DENIED`

---

#### D. blob_backend_webdav.cpp

**Error Pattern:** CURL-based

**Functions:**
- `put()` → Wrap CURL errors in `Result<void>`
- `get()` → Convert `nullopt` to `Result<vector<uint8_t>>`
- `remove()` → Convert `bool` to `Result<void>`

**Error Codes:** `ERR_NET_TIMEOUT`, `ERR_NET_DNS_FAILURE`

---

## 📝 Implementation Notes

### Design Decisions

1. **Iterator Functions:** Use `ERR_INDEX_NOT_INITIALIZED` instead of storage errors since iterators are index-like operations.

2. **BlobOperationResult Removal:** Complete removal of custom status struct in favor of unified `Result<T>`.

3. **Exception Wrapping:** Convert all exception-based error handling in blob backends to `Result<T>` for consistency.

4. **Error Context:** All error returns must include context (file names, blob keys, operation details).

### Migration Order

1. ✅ Add error codes
2. 🚧 Migrate RocksDB wrapper (highest priority - affects many modules)
3. ⏳ Migrate blob_redundancy_manager
4. ⏳ Migrate blob backends (can be done in parallel)
5. ⏳ Update all call sites
6. ⏳ Update test files

---

## 🧪 Testing Strategy

### Unit Tests to Update

**RocksDB Wrapper:**
- `tests/test_rocksdb_wrapper.cpp`
- Add iterator creation failure tests
- Add column family error tests

**Blob Redundancy:**
- `tests/test_blob_redundancy.cpp`
- Add redundancy failure scenarios
- Add corruption detection tests

**Blob Backends:**
- `tests/test_blob_backend_*.cpp`
- Add network failure simulation
- Add permission denied tests

### Integration Tests

- Test error propagation across storage boundaries
- Test transaction failure scenarios
- Test multi-backend redundancy failures

---

## 📊 Progress Tracking

### Completion Metrics

| Task | Status | Files Changed | Lines Changed |
|------|--------|---------------|---------------|
| Error code addition | ✅ Complete | 2 | +60 |
| RocksDB wrapper | ⏳ Pending | 2 | ~TBD |
| Blob redundancy | ⏳ Pending | 2 | ~TBD |
| Blob backends | ⏳ Pending | 8 | ~TBD |
| Test updates | ⏳ Pending | ~15 | ~TBD |

**Overall Progress:** 10% (Error codes added)

---

## 🚨 Risks & Blockers

### Identified Risks

1. **High Call Site Impact:** RocksDB wrapper functions are called throughout the codebase. Changes will ripple to many modules.

2. **Test Coverage:** Existing tests may not cover all error scenarios. New tests required.

3. **Performance Impact:** Need to benchmark Result<T> overhead in hot paths (iterator creation).

4. **Breaking Changes:** Iterator signatures change from `unique_ptr<>` to `Result<unique_ptr<>>`.

### Mitigation

- Incremental migration (one function at a time)
- Comprehensive testing after each function
- Performance benchmarks on critical paths
- Communication with team about breaking changes

---

## 📅 Timeline

**Week 2 (Current):**
- Day 1: ✅ Error code addition
- Day 2-3: RocksDB wrapper migration
- Day 4-5: Update RocksDB call sites

**Week 3:**
- Day 1-2: Blob redundancy manager migration
- Day 3-4: Blob backend migrations
- Day 5: Test file updates and validation

---

**Last Updated:** 2026-04-06  
**Next Update:** After RocksDB wrapper migration complete
