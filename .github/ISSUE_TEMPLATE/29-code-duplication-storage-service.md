---
name: "🔧 Code Duplication in LoRa Storage Service"
about: Refactor duplicated key provider initialization code (Kritisch - P0)
title: "[LoRa] Refactor lora_storage_service_themisdb.cpp - Remove Code Duplication"
labels: priority:P0, type:refactoring, area:llm, area:storage, effort:small, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Lines 134-240 in `lora_storage_service_themisdb.cpp` enthalten duplizierte Key Provider Initialisierungs-Code in verschachtelten catch-Blöcken. Dies führt zu Wartbarkeits-Problemen und Risiko von inkonsistentem Verhalten.

**EN**: Lines 134-240 in `lora_storage_service_themisdb.cpp` contain duplicated key provider initialization code in nested catch blocks. This leads to maintainability issues and risk of inconsistent behavior.

**Related Analysis**: `REMAINING_GAPS_SUMMARY.md` §1 (Priority 0)  
**Current Status**: `src/llm/lora_framework/lora_storage_service_themisdb.cpp:134-240`  
**Blocker**: ⚠️ **CODE QUALITY ISSUE** - Confusing code structure, maintenance risk

## 🎯 Ziele / Goals

- [ ] Extract key provider initialization into separate method
- [ ] Single source of truth for provider selection logic
- [ ] Clean exception handling flow
- [ ] Improved code readability
- [ ] Unit tests for key provider creation

## 📝 Aufgaben / Tasks

### 1. Extract createKeyProvider() Method
**Priorität**: P0 - Kritisch

**Current Problem**:
```cpp
// Line 45: First try block with HSM/Vault/Mock provider logic
try {
    // HSM configuration...
    std::shared_ptr<KeyProvider> key_provider;
    
    if (config_.use_hsm_for_encryption && !config_.hsm_library_path.empty()) {
        // HSM setup...
    } else if (config_.use_vault_for_encryption && !config_.vault_addr.empty()) {
        // Vault setup...
    } else {
        // MockKeyProvider fallback...
    }
    
} catch (const std::exception& e) {
    // Line 134: DUPLICATE code with PKI and Mock provider
    if (config_.use_pki_for_encryption) {
        // PKI setup...
    } else {
        // DUPLICATE MockKeyProvider fallback...
    }
} catch (const std::exception& e) {
    // Line 183: SECOND catch with MORE duplicate Vault configuration
    if (config_.use_vault_for_encryption) {
        // DUPLICATE Vault setup...
    }
}
```

**Implementation Steps**:
- [ ] Create new method: `std::shared_ptr<KeyProvider> createKeyProvider(const Config& config)`
- [ ] Move all key provider selection logic into this method
- [ ] Remove duplicated code from catch blocks
- [ ] Use single try-catch in main initialization

**Target Signature**:
```cpp
class LoRAStorageService::Impl {
private:
    /**
     * @brief Create appropriate key provider based on configuration
     * @param config Storage service configuration
     * @return Shared pointer to key provider
     * @throws std::runtime_error if production mode without secure provider
     */
    std::shared_ptr<KeyProvider> createKeyProvider(const Config& config);
};
```

---

### 2. Implement Provider Selection Logic
**Priorität**: P0 - Kritisch

**Implementation**:
```cpp
std::shared_ptr<KeyProvider> LoRAStorageService::Impl::createKeyProvider(const Config& config) {
    // 1. Try HSM first (highest priority)
    if (config.use_hsm_for_encryption) {
        if (config.hsm_library_path.empty()) {
            throw std::runtime_error("HSM encryption enabled but library path not configured");
        }
        return createHSMKeyProvider(config);
    }
    
    // 2. Try Vault second
    if (config.use_vault_for_encryption) {
        if (config.vault_addr.empty()) {
            throw std::runtime_error("Vault encryption enabled but address not configured");
        }
        return createVaultKeyProvider(config);
    }
    
    // 3. Try PKI third
    if (config.use_pki_for_encryption) {
        if (config.pki_cert_path.empty() || config.pki_private_key_path.empty()) {
            throw std::runtime_error("PKI encryption enabled but certificates not configured");
        }
        return createPKIKeyProvider(config);
    }
    
    // 4. Fallback to MockKeyProvider (development only)
    const char* env_mode = std::getenv("THEMIS_ENVIRONMENT");
    bool is_production = (env_mode != nullptr && 
                         (std::string(env_mode) == "production" || 
                          std::string(env_mode) == "prod"));
    
    if (is_production) {
        throw std::runtime_error(
            "Production environment requires HSM, Vault, or PKI key provider. "
            "Set THEMIS_ENVIRONMENT=development to use MockKeyProvider."
        );
    }
    
    spdlog::warn("⚠️  Using MockKeyProvider - DEVELOPMENT MODE ONLY");
    return std::make_shared<MockKeyProvider>();
}
```

**Tasks**:
- [ ] Implement `createKeyProvider()` method
- [ ] Extract helper methods: `createHSMKeyProvider()`, `createVaultKeyProvider()`, `createPKIKeyProvider()`
- [ ] Add proper error handling with descriptive messages
- [ ] Add logging at each decision point

---

### 3. Simplify Main Initialization
**Priorität**: P0 - Kritisch

**Refactored Code**:
```cpp
// In constructor or init method
if (config_.enable_encryption && !encryption_) {
    try {
        auto key_provider = createKeyProvider(config_);
        encryption_ = std::make_shared<FieldEncryption>(key_provider);
        spdlog::info("✓ Encryption initialized successfully");
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to initialize encryption: {}", e.what());
        throw;  // Re-throw to prevent insecure operation
    }
}
```

**Tasks**:
- [ ] Replace nested try-catch blocks with single try-catch
- [ ] Remove all duplicated initialization code
- [ ] Verify error handling is consistent
- [ ] Update logging to be more informative

---

### 4. Add Unit Tests
**Priorität**: P0 - Kritisch

**Test Cases**:
```cpp
// Test file: tests/test_lora_storage_key_provider.cpp

TEST(LoRAStorageKeyProviderTest, HSMPriorityOverVault) {
    // When both HSM and Vault configured, HSM should be used
}

TEST(LoRAStorageKeyProviderTest, VaultPriorityOverPKI) {
    // When both Vault and PKI configured, Vault should be used
}

TEST(LoRAStorageKeyProviderTest, PKIPriorityOverMock) {
    // When PKI configured, it should be used over MockKeyProvider
}

TEST(LoRAStorageKeyProviderTest, ProductionModeEnforcement) {
    // Production mode should throw without secure provider
}

TEST(LoRAStorageKeyProviderTest, DevelopmentModeAllowsMock) {
    // Development mode should allow MockKeyProvider with warning
}

TEST(LoRAStorageKeyProviderTest, MissingHSMConfig) {
    // Should throw if use_hsm_for_encryption but no library path
}

TEST(LoRAStorageKeyProviderTest, MissingVaultConfig) {
    // Should throw if use_vault_for_encryption but no address
}
```

**Tasks**:
- [ ] Create test file with comprehensive test coverage
- [ ] Test all provider selection paths
- [ ] Test error cases (missing config)
- [ ] Test production vs development mode
- [ ] Verify logging output

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

- [ ] All duplicated code removed from catch blocks
- [ ] Single `createKeyProvider()` method handles all provider selection
- [ ] Clear priority order: HSM > Vault > PKI > Mock
- [ ] Production mode enforces secure providers
- [ ] Development mode allows MockKeyProvider with warnings
- [ ] All error cases have descriptive messages
- [ ] Unit tests cover all scenarios (>90% coverage)
- [ ] Code review passed
- [ ] No regressions in existing tests

## 📊 Effort Estimation

- **Aufwand / Effort**: 2-3 hours (Small)
- **Komplexität / Complexity**: Low
- **Risiko / Risk**: Low (refactoring only, no logic changes)

## 🔗 Related Issues

- Original analysis: `REMAINING_GAPS_SUMMARY.md`
- Security improvements: Issue #14 (VaultKeyProvider Integration)
- Security improvements: Issue #15 (HSMProvider Integration)

## 📚 References

- Code location: `src/llm/lora_framework/lora_storage_service_themisdb.cpp`
- Key provider interface: `include/security/key_provider.h`
- HSM provider: `include/security/hsm_provider.h`
- Vault provider: `include/security/vault_key_provider.h`
- PKI provider: `include/security/pki_key_provider.h`

---

**Priority**: P0 - Must fix before production  
**Impact**: Code quality, maintainability, consistency  
**Status**: Ready to implement
