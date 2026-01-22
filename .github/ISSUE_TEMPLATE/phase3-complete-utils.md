---
name: Phase 3 - Complete Utils Module Migration to Result<T>
about: Complete migration of remaining utils methods to Result<T>
title: '[Phase 3] Complete Utils Module Migration to Result<T>'
labels: ['enhancement', 'error-handling', 'phase-3', 'utils']
assignees: ''
---

## 📋 Overview

Complete the migration of remaining utility methods from legacy error patterns to `Result<T>`.

**Current Status:** ~25% complete  
**Target:** Complete all utils methods  
**Priority:** 🟡 Medium

## 🎯 Goals

- Migrate remaining utils methods for consistency
- Provide better error context in utility functions
- Complete lower-level infrastructure migration

## 🔨 Remaining Work

### Already Complete
- ✅ `RetentionManager::getPolicy()` - Returns `Result<const RetentionPolicy*>`
- ✅ Some error registry methods

### Remaining Methods

**File Operations (cursor.cpp, ~4 methods):**
- [ ] Cursor operations returning `std::nullopt`
- [ ] File position tracking
- [ ] Read/write operations

**Validation (input_validator.cpp, ~5 methods):**
- [ ] Schema validation methods
- [ ] Input sanitization
- [ ] Type checking

**Crypto/Security (pii_pseudonymizer.cpp, pki_client.cpp, ~6 methods):**
- [ ] PII pseudonymization methods
- [ ] PKI client operations
- [ ] Key management

**Version/Update (update_checker.cpp, license_info.cpp, ~4 methods):**
- [ ] Version checking
- [ ] License validation
- [ ] Update availability

**Other Utils (~5-10 methods):**
- [ ] Audit remaining files
- [ ] Identify nullable returns
- [ ] Categorize by priority

## 📝 Implementation Strategy

### 1. Audit Phase
```bash
# Find remaining std::nullopt returns
grep -r "return std::nullopt\|return nullopt" src/utils/*.cpp

# Find remaining nullptr returns  
grep -r "return nullptr" src/utils/*.cpp

# Find remaining false returns with error context
grep -r "return false" src/utils/*.cpp | grep -i error
```

### 2. Prioritization
**High Priority:**
- Security-related methods (crypto, validation)
- File operations (risk of data loss)

**Medium Priority:**
- Version/license checks
- Monitoring/metrics

**Low Priority:**
- Convenience wrappers
- Non-critical helpers

### 3. Migration Pattern

**Example: Cursor Operations**
```cpp
// Before
std::optional<Position> getNextPosition() {
    if (!hasMore()) {
        return std::nullopt;
    }
    return current_pos_;
}

// After
Result<Position> getNextPosition() {
    if (!hasMore()) {
        return Err<Position>(
            ERR_API_INVALID_REQUEST,
            "Cursor exhausted: no more positions available"
        );
    }
    return Ok(current_pos_);
}
```

## 📋 Implementation Checklist

### Security/Crypto Methods
- [ ] Audit `pii_pseudonymizer.cpp` methods
- [ ] Audit `pki_client.cpp` methods
- [ ] Add error codes for crypto failures
- [ ] Migrate methods
- [ ] Update tests

### File/Cursor Operations
- [ ] Audit `cursor.cpp` methods
- [ ] Add error codes for file operations
- [ ] Migrate methods
- [ ] Update tests

### Validation Methods
- [ ] Audit `input_validator.cpp` methods
- [ ] Migrate schema validation
- [ ] Migrate type checking
- [ ] Update tests

### Version/License Methods
- [ ] Audit `update_checker.cpp` methods
- [ ] Audit `license_info.cpp` methods
- [ ] Migrate methods
- [ ] Update tests

### Cleanup
- [ ] Remove any remaining legacy patterns
- [ ] Verify all utils use `Result<T>` consistently
- [ ] Update documentation

## 🧪 Testing Requirements

### Security Tests
```cpp
TEST(PIIPseudonymizerTest, InvalidKey) {
    auto result = pseudonymizer.anonymize(data, "invalid-key");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ERR_CRYPTO_INVALID_KEY);
}
```

### File Operations Tests
```cpp
TEST(CursorTest, ExhaustedCursor) {
    auto result = cursor.getNextPosition();
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ERR_API_INVALID_REQUEST);
}
```

### Validation Tests
```cpp
TEST(InputValidatorTest, SchemaViolation) {
    auto result = validator.validate(data, schema);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ERR_SCHEMA_VALIDATION_FAILED);
}
```

## 📚 Documentation Updates

- [ ] Update utils module documentation
- [ ] Document error codes used
- [ ] Add migration examples
- [ ] Update API documentation

## 🎯 Success Criteria

- [ ] All utils methods use `Result<T>`
- [ ] No remaining legacy patterns
- [ ] All tests pass
- [ ] Documentation complete
- [ ] Code review approved

## 📊 Progress Tracking

**Expected Effort:** 2-3 weeks  
**Priority:** Medium (infrastructure completion)

### Weekly Breakdown
- [ ] Week 1: Security/crypto methods
- [ ] Week 2: File operations + validation
- [ ] Week 3: Version/license + cleanup

## 🔗 Related

- **Parent Issue:** #XXX (Error Handling Migration - Master Tracking)
- **Documentation:** ERROR_HANDLING_MIGRATION_STATUS.md

## 💡 Notes

- **Low Priority:** Utils are lower-level, less user-facing
- **Thoroughness:** Important to complete for consistency
- **Dependencies:** Other modules may depend on utils
- **Batch Work:** Can batch similar methods together
