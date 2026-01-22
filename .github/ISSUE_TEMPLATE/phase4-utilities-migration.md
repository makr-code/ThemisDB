---
name: Phase 4 - Utilities Migration
about: Track utilities and support modules error handling migration to Result<T> pattern
title: '[Phase 4] Utilities Migration'
labels: ['error-handling', 'phase-4', 'utilities', 'refactoring']
assignees: ''
---

## 📋 Module: Utilities & Support

**Priority:** P2 (Medium)  
**Estimated Effort:** 2 weeks  
**Complexity:** Low-Medium  
**Dependencies:** Phase 4 Foundation PR must be merged

## 🎯 Objective

Migrate utility and support module error handling from legacy patterns (`return nullptr`, exception-based) to unified `Result<T>` pattern using `tl::expected`.

## 📊 Scope

### Files to Migrate

**PKI Client** (`src/pki/pki_client.cpp`):
- [ ] 5 nullptr returns → `Result<T*>`
- [ ] Certificate validation failures
- [ ] Key loading errors

**PII Detection Engine** (`src/privacy/pii_detection_engine.cpp`):
- [ ] 6 nullptr returns → `Result<T*>`
- [ ] Pattern matching failures
- [ ] Classification errors

**Retention Manager** (`src/storage/retention_manager.cpp`):
- [ ] 1 nullptr return → `Result<T*>`
- [ ] Policy validation errors

**Crypto Utilities** (`src/crypto/*.cpp`):
- [ ] Encryption/decryption failures
- [ ] Key generation errors
- [ ] Hash computation failures

**Compression Utilities** (`src/compression/*.cpp`):
- [ ] Compression failures
- [ ] Decompression errors
- [ ] Buffer size errors

**File Utilities** (`src/utils/*.cpp`):
- [ ] File operation failures
- [ ] Permission errors
- [ ] Path validation errors

**Total:** 12 nullptr sites + 15 Status returns = **27 migration points**

## 📚 Resources

**Foundation Documentation:**
- Phase 4 Migration Matrix: `docs/error_handling/phase4_migration_matrix.md`
- Migration Example: `docs/error_handling/phase4_week2_getOrCreateColumnFamily_example.md`

**Error Codes to Add:**
- [ ] `ERR_UTIL_INVALID_ARGUMENT` (9000)
- [ ] `ERR_UTIL_FILE_OPERATION_FAILED` (9001)
- [ ] `ERR_UTIL_PERMISSION_DENIED` (9002)
- [ ] `ERR_CRYPTO_ENCRYPTION_FAILED` (8000)
- [ ] `ERR_CRYPTO_DECRYPTION_FAILED` (8001)
- [ ] `ERR_CRYPTO_KEY_GENERATION_FAILED` (8002)
- [ ] `ERR_CRYPTO_INVALID_KEY` (8003)
- [ ] `ERR_COMPRESSION_FAILED` (7000)
- [ ] `ERR_COMPRESSION_BUFFER_TOO_SMALL` (7001)
- [ ] `ERR_COMPRESSION_INVALID_FORMAT` (7002)

## 🔧 Implementation Steps

### Phase 1: Error Code Addition (Week 1 Day 1-2)
- [ ] Add 10 new error codes to error registry
- [ ] Register with detailed metadata
- [ ] Update error documentation

### Phase 2: PKI Client (Week 1 Day 3-4)
- [ ] Migrate certificate validation (2 nullptr)
- [ ] Migrate key loading (3 nullptr)
- [ ] Update call sites
- [ ] Add unit tests for certificate errors
- [ ] Build verification

### Phase 3: PII Detection Engine (Week 1 Day 5)
- [ ] Migrate pattern matching (3 nullptr)
- [ ] Migrate classification (3 nullptr)
- [ ] Update call sites
- [ ] Add unit tests for detection failures
- [ ] Build verification

### Phase 4: Crypto Utilities (Week 1-2)
- [ ] Migrate encryption functions (Status → Result<T>)
- [ ] Migrate decryption functions (Status → Result<T>)
- [ ] Migrate key generation (Status → Result<T>)
- [ ] Convert exception-based to Result<T>
- [ ] Update call sites across codebase
- [ ] Add unit tests for crypto failures
- [ ] Build verification

### Phase 5: Compression Utilities (Week 2)
- [ ] Migrate compression functions (Status → Result<T>)
- [ ] Migrate decompression functions (Status → Result<T>)
- [ ] Convert exception-based to Result<T>
- [ ] Update call sites
- [ ] Add unit tests for compression errors
- [ ] Build verification

### Phase 6: File & Other Utilities (Week 2)
- [ ] Migrate retention manager (1 nullptr)
- [ ] Migrate file utilities (Status → Result<T>)
- [ ] Migrate string utilities if needed
- [ ] Update call sites
- [ ] Add unit tests for file operation failures
- [ ] Build verification

### Phase 7: Testing & Validation (Week 2)
- [ ] Update ~10 existing test files
- [ ] Add utility edge case tests
- [ ] Add crypto operation failure tests
- [ ] Add compression error tests
- [ ] Performance benchmarking (ensure <5% regression)
- [ ] Code review and refinement
- [ ] Documentation updates

## ✅ Acceptance Criteria

- [ ] All 27 utility functions migrated to `Result<T>` pattern
- [ ] All call sites updated to use Result<T> checks
- [ ] 10 new error codes added and registered
- [ ] Exception-based error handling converted to Result<T>
- [ ] Zero build warnings or errors
- [ ] All unit tests passing
- [ ] Integration tests passing
- [ ] Performance regression <5%
- [ ] Code review approved
- [ ] Documentation updated

## 📝 Migration Pattern

```cpp
// BEFORE: nullptr + exception pattern
uint8_t* encrypt(const uint8_t* data, size_t len, size_t* out_len) {
    try {
        if (!data || len == 0) return nullptr;
        
        auto* encrypted = new uint8_t[len + 16];
        if (!performEncryption(data, len, encrypted, out_len)) {
            delete[] encrypted;
            return nullptr;
        }
        
        return encrypted;
    } catch (const CryptoException& e) {
        LOG_ERROR("Encryption failed: {}", e.what());
        return nullptr;
    }
}

// AFTER: Result<T> pattern
Result<std::vector<uint8_t>> encrypt(const std::span<const uint8_t>& data) {
    if (data.empty()) {
        return Err<std::vector<uint8_t>>(
            ERR_UTIL_INVALID_ARGUMENT,
            "Cannot encrypt empty data"
        );
    }
    
    std::vector<uint8_t> encrypted;
    encrypted.resize(data.size() + 16);
    
    auto result = performEncryption(data, encrypted);
    if (!result) {
        return Err<std::vector<uint8_t>>(
            ERR_CRYPTO_ENCRYPTION_FAILED,
            fmt::format("Encryption failed: {}", result.error().message())
        );
    }
    
    return Ok(std::move(encrypted));
}

// Call site update with modern C++
auto encrypted_result = encrypt(data);
if (encrypted_result) {
    auto& encrypted_data = *encrypted_result;
    // use encrypted_data
} else {
    LOG_ERROR("Encryption failed: {}", encrypted_result.error().message());
    return encrypted_result.error();
}
```

## 🔗 Related Issues

- Depends on: Phase 4 Foundation PR
- Low priority but high call site count
- Used throughout codebase

## 📊 Progress Tracking

**Week 1:** ⬜⬜⬜⬜⬜ 0%  
**Week 2:** ⬜⬜⬜⬜⬜ 0%

**Overall:** 0 of 27 functions migrated (0%)

**Breakdown:**
- PKI Client: 0 / 5 (0%)
- PII Detection: 0 / 6 (0%)
- Retention Manager: 0 / 1 (0%)
- Crypto: 0 / 5 (0%)
- Compression: 0 / 5 (0%)
- File Utils: 0 / 5 (0%)

## ⚠️ Risk Areas

- **High Call Site Count:** Crypto and compression used throughout codebase
- **Exception Conversion:** Careful conversion needed to avoid resource leaks
- **Buffer Management:** Compression requires careful buffer handling
- **Security Critical:** Crypto functions must maintain security properties

## 💬 Notes

- Lowest complexity module in Phase 4
- Can be done in parallel with other modules
- Good candidate for junior developers to learn the pattern
- Crypto and compression are performance-sensitive
- Use modern C++ (std::span, std::vector) instead of raw pointers

---
**Assigned to:** TBD  
**Started:** TBD  
**Target Completion:** TBD  
**Actual Completion:** TBD
