# Phase 4 Week 2: RocksDB getOrCreateColumnFamily Migration Example

**Date:** 2026-01-19  
**Migration Type:** `nullptr` → `Result<T*>`  
**Function:** `getOrCreateColumnFamily`

---

## 🎯 Migration Summary

Successfully migrated `RocksDBWrapper::getOrCreateColumnFamily()` from returning `nullptr` on error to returning `Result<ColumnFamilyHandle*>`.

### Changes Made

**Files Modified:**
1. `include/storage/rocksdb_wrapper.h` - Function signature update
2. `src/storage/rocksdb_wrapper.cpp` - Implementation update
3. `src/server/http_server.cpp` - 2 call sites updated
4. `src/timeseries/hypertable.cpp` - 1 call site updated
5. `src/updates/manifest_database.cpp` - 4 call sites updated

**Total Call Sites Updated:** 7

---

## 📝 Migration Pattern

### Before (Legacy Pattern)

**Function Signature:**
```cpp
rocksdb::ColumnFamilyHandle* getOrCreateColumnFamily(const std::string& cf_name);
```

**Implementation:**
```cpp
rocksdb::ColumnFamilyHandle* RocksDBWrapper::getOrCreateColumnFamily(const std::string& cf_name) {
    if (!db_) {
        THEMIS_ERROR("getOrCreateColumnFamily: DB not open");
        return nullptr;  // ❌ Lost error context
    }
    
    // ... existing CF check ...
    
    rocksdb::Status s = db_->CreateColumnFamily(cf_opts, cf_name, &cf_handle);
    if (!s.ok()) {
        THEMIS_ERROR("Failed to create column family '{}': {}", cf_name, s.ToString());
        return nullptr;  // ❌ Lost error context
    }
    
    return cf_handle;
}
```

**Call Site (try-catch pattern):**
```cpp
try {
    pii_cf_handle_ = storage_->getOrCreateColumnFamily("pii_mappings");
    pii_api_ = std::make_unique<PIIApiHandler>(storage_->getRawDB(), pii_cf_handle_);
} catch (const std::exception& ex) {
    THEMIS_ERROR("Failed to initialize PII Manager CF: {}", ex.what());
}
```

**Problems:**
- ❌ Callers cannot distinguish between "DB not open" vs "creation failed"
- ❌ Error context (cf_name, RocksDB error details) lost at call site
- ❌ Requires nullptr checks everywhere
- ❌ Try-catch pattern inconsistent (function doesn't throw)

---

### After (Unified Error Handling)

**Function Signature:**
```cpp
Result<rocksdb::ColumnFamilyHandle*> getOrCreateColumnFamily(const std::string& cf_name);
```

**Implementation:**
```cpp
Result<rocksdb::ColumnFamilyHandle*> RocksDBWrapper::getOrCreateColumnFamily(const std::string& cf_name) {
    if (!db_) {
        THEMIS_ERROR("getOrCreateColumnFamily: DB not open");
        return Err<rocksdb::ColumnFamilyHandle*>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "RocksDB not opened for column family: " + cf_name
        );  // ✅ Structured error with context
    }
    
    // ... existing CF check ...
    for (auto* handle : cf_handles_) {
        if (handle && handle->GetName() == cf_name) {
            return Ok(handle);  // ✅ Explicit success
        }
    }
    
    rocksdb::Status s = db_->CreateColumnFamily(cf_opts, cf_name, &cf_handle);
    if (!s.ok()) {
        THEMIS_ERROR("Failed to create column family '{}': {}", cf_name, s.ToString());
        return Err<rocksdb::ColumnFamilyHandle*>(
            errors::ErrorCode::ERR_INDEX_CREATION_FAILED,
            fmt::format("Failed to create column family '{}': {}", cf_name, s.ToString())
        );  // ✅ Structured error with full context
    }
    
    return Ok(cf_handle);  // ✅ Explicit success
}
```

**Call Site (Result pattern):**
```cpp
auto cf_result = storage_->getOrCreateColumnFamily("pii_mappings");
if (cf_result) {
    pii_cf_handle_ = *cf_result;
    pii_api_ = std::make_unique<PIIApiHandler>(storage_->getRawDB(), pii_cf_handle_);
    THEMIS_INFO("PII Manager initialized with dedicated CF 'pii_mappings'");
} else {
    THEMIS_ERROR("Failed to initialize PII Manager CF: {}", cf_result.error().message());
}
```

**Benefits:**
- ✅ Callers can distinguish error types via error codes
- ✅ Full error context preserved (cf_name, RocksDB details)
- ✅ Type-safe error checking (compiler enforced)
- ✅ Consistent error handling pattern
- ✅ Machine-readable error codes for programmatic handling

---

## 🔍 Call Site Migration Patterns

### Pattern 1: Simple Assignment with Fallback

**Before:**
```cpp
auto* cf_handle = db_->getOrCreateColumnFamily(chunk_name);
if (cf_handle) {
    THEMIS_DEBUG("Using chunk: {}", chunk_name);
} else {
    THEMIS_ERROR("Failed to create chunk: {}", chunk_name);
}
return cf_handle;
```

**After:**
```cpp
auto cf_result = db_->getOrCreateColumnFamily(chunk_name);
if (cf_result) {
    THEMIS_DEBUG("Using chunk: {}", chunk_name);
    return *cf_result;
} else {
    THEMIS_ERROR("Failed to create chunk: {} - {}", chunk_name, cf_result.error().message());
    return nullptr;
}
```

---

### Pattern 2: Multiple CFs with Error Aggregation

**Before:**
```cpp
try {
    cf_manifests_ = storage_->getOrCreateColumnFamily("release_manifests");
    cf_files_ = storage_->getOrCreateColumnFamily("file_registry");
    cf_signatures_ = storage_->getOrCreateColumnFamily("signature_cache");
    cf_cache_ = storage_->getOrCreateColumnFamily("download_cache");
    LOG_INFO("Column families initialized");
} catch (const std::exception& e) {
    LOG_ERROR("Failed to initialize column families: {}", e.what());
    cf_manifests_ = nullptr;
    cf_files_ = nullptr;
    cf_signatures_ = nullptr;
    cf_cache_ = nullptr;
}
```

**After:**
```cpp
auto cf_manifests = storage_->getOrCreateColumnFamily("release_manifests");
auto cf_files = storage_->getOrCreateColumnFamily("file_registry");
auto cf_signatures = storage_->getOrCreateColumnFamily("signature_cache");
auto cf_cache = storage_->getOrCreateColumnFamily("download_cache");

if (cf_manifests && cf_files && cf_signatures && cf_cache) {
    cf_manifests_ = *cf_manifests;
    cf_files_ = *cf_files;
    cf_signatures_ = *cf_signatures;
    cf_cache_ = *cf_cache;
    LOG_INFO("Column families initialized");
} else {
    LOG_ERROR("Failed to initialize column families:");
    if (!cf_manifests) LOG_ERROR("  - release_manifests: {}", cf_manifests.error().message());
    if (!cf_files) LOG_ERROR("  - file_registry: {}", cf_files.error().message());
    if (!cf_signatures) LOG_ERROR("  - signature_cache: {}", cf_signatures.error().message());
    if (!cf_cache) LOG_ERROR("  - download_cache: {}", cf_cache.error().message());
    
    cf_manifests_ = nullptr;
    cf_files_ = nullptr;
    cf_signatures_ = nullptr;
    cf_cache_ = nullptr;
}
```

**Benefits:**
- ✅ Detailed error reporting per CF
- ✅ All CFs checked before proceeding
- ✅ Clear which CF failed in logs

---

### Pattern 3: With Initialization Logic

**Before:**
```cpp
try {
    pii_cf_handle_ = storage_->getOrCreateColumnFamily("pii_mappings");
    pii_api_ = std::make_unique<PIIApiHandler>(storage_->getRawDB(), pii_cf_handle_);
    THEMIS_INFO("PII Manager initialized");
} catch (const std::exception& ex) {
    THEMIS_ERROR("Failed to initialize PII Manager: {}", ex.what());
}
```

**After:**
```cpp
auto cf_result = storage_->getOrCreateColumnFamily("pii_mappings");
if (cf_result) {
    pii_cf_handle_ = *cf_result;
    pii_api_ = std::make_unique<PIIApiHandler>(storage_->getRawDB(), pii_cf_handle_);
    THEMIS_INFO("PII Manager initialized with dedicated CF 'pii_mappings'");
} else {
    THEMIS_ERROR("Failed to initialize PII Manager CF: {}", cf_result.error().message());
}
```

---

## 📊 Error Codes Used

### ERR_INDEX_NOT_INITIALIZED (6000)

**When:** Database is not open  
**Category:** Index  
**Severity:** Error  
**Message:** "RocksDB not opened for column family: {cf_name}"

**Recovery:**
1. Check if database initialization completed
2. Verify open() was called successfully
3. Check for initialization order issues

---

### ERR_INDEX_CREATION_FAILED (6001)

**When:** RocksDB CreateColumnFamily() fails  
**Category:** Index  
**Severity:** Error  
**Message:** "Failed to create column family '{cf_name}': {rocksdb_error}"

**Recovery:**
1. Check RocksDB status message for details
2. Verify disk space available
3. Check file permissions
4. Review RocksDB logs

---

## 🧪 Testing Considerations

### Test Cases to Add/Update

1. **Success Path:**
   - Create new CF successfully
   - Get existing CF successfully

2. **Error Paths:**
   - Call with DB not open → ERR_INDEX_NOT_INITIALIZED
   - RocksDB creation fails → ERR_INDEX_CREATION_FAILED
   - Verify error messages include CF name

3. **Call Site Tests:**
   - Update all tests using getOrCreateColumnFamily()
   - Verify Result<T> pattern handling
   - Check error logging includes full context

---

## 📈 Migration Impact

### Metrics

- **Functions Migrated:** 1
- **Call Sites Updated:** 7
- **Files Modified:** 5
- **Error Codes Used:** 2 (ERR_INDEX_NOT_INITIALIZED, ERR_INDEX_CREATION_FAILED)
- **Try-Catch Blocks Removed:** 3

### Performance

- **Expected Impact:** None (Result<T> is zero-overhead)
- **Error Path:** Slightly improved (no exception unwinding)
- **Success Path:** Identical performance

---

## ✅ Completion Checklist

- [x] Function signature updated in header
- [x] Implementation migrated to Result<T>
- [x] Include expected.h added
- [x] Error codes properly used
- [x] Error context includes CF name
- [x] All 7 call sites updated
- [x] Try-catch patterns converted to Result checks
- [x] Error messages improved with full context
- [ ] Unit tests updated (pending)
- [ ] Integration tests updated (pending)
- [ ] Documentation updated (this document)

---

## 🔄 Next Steps

### Remaining RocksDB Wrapper Migrations

1. **newAsyncIterator()** - 2 nullptr returns
2. **newIterator()** - 2 nullptr returns
3. **newSafeIterator()** - 1 nullptr return

### Pattern to Follow

Use this migration as a template for remaining functions:
1. Update signature to `Result<T>`
2. Replace `return nullptr` with `Err<T>(error_code, context)`
3. Replace successful returns with `Ok(value)`
4. Update all call sites to check Result<T>
5. Improve error messages with full context

---

**Migration Completed:** 2026-01-19  
**Status:** ✅ SUCCESS  
**Next Function:** newAsyncIterator()
