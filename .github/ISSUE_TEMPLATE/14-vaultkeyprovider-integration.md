---
name: "🔐 VaultKeyProvider Integration für LoRa Adapters"
about: Integrate HashiCorp Vault key management for LoRA adapter encryption
title: "[Security] Integrate VaultKeyProvider for LoRA Adapter Encryption"
labels: priority:P1, type:security, area:security, area:llm, effort:medium, phase:production
assignees: ''

---

## 📋 Description

Integrate the existing `VaultKeyProvider` implementation with LoRA adapter storage to enable production-ready encryption using HashiCorp Vault for key management. This replaces the `MockKeyProvider` currently used in development.

**Related Files**:
- `include/security/vault_key_provider.h` - Existing VaultKeyProvider implementation
- `src/llm/lora_framework/lora_storage_service_themisdb.cpp:41-54` - Integration point
- `LORA_STORAGE_BACKEND_COMPLETION.md` - Implementation documentation

**Current Status**: MockKeyProvider used in development (NOT suitable for production)

## 🎯 Goals

- [ ] Replace MockKeyProvider with VaultKeyProvider in LoRA storage
- [ ] Configure Vault KV v2 secrets engine for LoRA adapter keys
- [ ] Test encryption/decryption roundtrip with Vault
- [ ] Implement key rotation support
- [ ] Document Vault setup and configuration
- [ ] Production deployment guide

## 📝 Tasks

### 1. VaultKeyProvider Configuration

**Task**: Configure Vault connection for LoRA adapters

**Implementation**:
```cpp
// File: src/llm/lora_framework/lora_storage_service_themisdb.cpp
// Lines: 41-54

// Replace MockKeyProvider with VaultKeyProvider
VaultKeyProvider::Config vault_config;
vault_config.vault_addr = config_.vault_addr;      // e.g., "http://localhost:8200"
vault_config.vault_token = config_.vault_token;    // From config or env
vault_config.kv_mount_path = "themis";             // KV secrets engine mount
vault_config.cache_ttl_seconds = 3600;             // 1 hour cache
vault_config.cache_capacity = 1000;                // Max cached keys

auto key_provider = std::make_shared<VaultKeyProvider>(vault_config);
encryption_ = std::make_shared<FieldEncryption>(key_provider);
```

**Configuration Options**:
- [ ] Vault address (environment: `VAULT_ADDR`)
- [ ] Vault token (environment: `VAULT_TOKEN`)
- [ ] KV mount path (default: "themis")
- [ ] Key cache TTL (default: 3600s)
- [ ] SSL verification settings

**File**: `src/llm/lora_framework/lora_storage_service_themisdb.cpp`
**Lines**: 41-54

### 2. Vault Setup

**Task**: Set up Vault KV v2 secrets engine for LoRA keys

**Vault Commands**:
```bash
# 1. Enable KV v2 secrets engine
vault secrets enable -version=2 -path=themis kv

# 2. Store encryption key for LoRA adapters
vault kv put themis/keys/lora_adapters \
  key=$(openssl rand -base64 32) \
  algorithm="AES-256-GCM" \
  version=1

# 3. Create policy for ThemisDB
cat > themis-lora-policy.hcl <<EOF
path "themis/data/keys/lora_adapters" {
  capabilities = ["read"]
}
path "themis/metadata/keys/lora_adapters" {
  capabilities = ["read"]
}
EOF

vault policy write themis-lora themis-lora-policy.hcl

# 4. Generate token with policy
vault token create -policy=themis-lora
```

**Tasks**:
- [ ] Enable KV v2 secrets engine
- [ ] Create encryption key for LoRA adapters
- [ ] Define Vault policy with least privilege
- [ ] Generate service token for ThemisDB
- [ ] Test key retrieval with policy

### 3. LoRAStorageService Configuration

**Task**: Add Vault configuration to LoRAStorageService::Config

**Implementation**:
```cpp
// File: include/llm/lora_framework/lora_storage_service.h
// Add to Config struct:

struct Config {
    // ... existing fields ...
    
    // Vault Key Provider configuration
    std::string vault_addr;          // Vault server address
    std::string vault_token;         // Vault authentication token
    std::string vault_kv_mount;      // KV mount path (default: "themis")
    std::string vault_key_id;        // Key ID (default: "lora_adapters")
    bool use_vault_for_encryption;   // Enable Vault encryption (default: false)
};
```

**Tasks**:
- [ ] Add Vault configuration fields to Config struct
- [ ] Update constructor to handle Vault config
- [ ] Read from environment variables if not provided
- [ ] Add validation for required Vault settings

**File**: `include/llm/lora_framework/lora_storage_service.h`

### 4. Key Rotation Support

**Task**: Implement key rotation without data migration

**Strategy**: Envelope encryption (Key Encryption Key + Data Encryption Key)
```cpp
// Store DEK version in metadata
struct AdapterMetadata {
    // ... existing fields ...
    uint32_t encryption_key_version;  // KEK version used
};

// Load with correct version
auto key = key_provider->getKey("lora_adapters", metadata.encryption_key_version);
```

**Tasks**:
- [ ] Store key version in adapter metadata
- [ ] Retrieve versioned keys during decryption
- [ ] Test key rotation scenario
- [ ] Document rotation procedure

**File**: `src/llm/lora_framework/lora_storage_service_themisdb.cpp`

### 5. Error Handling

**Task**: Handle Vault connection failures gracefully

**Error Cases**:
- Vault unreachable (network error)
- Invalid token (403 Forbidden)
- Key not found (404 Not Found)
- Token expired (403)

**Implementation**:
```cpp
try {
    auto key_provider = std::make_shared<VaultKeyProvider>(vault_config);
    encryption_ = std::make_shared<FieldEncryption>(key_provider);
} catch (const KeyOperationException& e) {
    if (e.transient()) {
        // Retry with backoff
        spdlog::warn("Vault connection failed, retrying: {}", e.what());
    } else {
        // Fatal error - disable encryption
        spdlog::error("Vault initialization failed: {}", e.what());
        throw;
    }
}
```

**Tasks**:
- [ ] Catch `KeyOperationException` and `KeyNotFoundException`
- [ ] Implement retry logic for transient errors
- [ ] Log errors with context (adapter_id, key_id)
- [ ] Graceful degradation options

### 6. Testing

**Task**: Comprehensive testing of Vault integration

**Unit Tests**:
```cpp
TEST(VaultKeyProvider, LoRAEncryptionRoundtrip) {
    // Mock Vault responses
    auto mock_vault = std::make_shared<MockVaultKeyProvider>();
    mock_vault->setKey("lora_adapters", generateRandomKey(32));
    
    // Save encrypted adapter
    LoRAStorageService storage(config_with_vault);
    storage.saveAdapter("test", weights, metadata);
    
    // Load and decrypt
    auto loaded = storage.loadAdapter("test");
    EXPECT_EQ(loaded->data, weights.data);
}

TEST(VaultKeyProvider, KeyRotation) {
    // Encrypt with v1
    auto v1_key = vault->getKey("lora_adapters", 1);
    // ... save adapter ...
    
    // Rotate to v2
    vault->rotateKey("lora_adapters");
    
    // Decrypt with v1 still works
    auto loaded = storage.loadAdapter("test");
    EXPECT_TRUE(loaded.has_value());
}
```

**Integration Tests**:
- [ ] Test with real Vault instance (or Vault dev server)
- [ ] Test key caching behavior
- [ ] Test token expiration handling
- [ ] Test network failure scenarios

**Files**:
- `tests/test_lora_vault_integration.cpp` (new)
- `tests/test_lora_framework.cpp` (extend)

### 7. Documentation

**Task**: Document Vault setup and usage

**Documentation Files**:
- [ ] `docs/de/security/vault_lora_setup.md` - Vault setup guide
- [ ] `docs/de/llm/lora_encryption.md` - Encryption architecture
- [ ] `LORA_STORAGE_BACKEND_COMPLETION.md` - Update with Vault details

**Content**:
- Vault installation and configuration
- Policy creation and token management
- ThemisDB configuration examples
- Key rotation procedures
- Troubleshooting guide

### 8. Production Deployment

**Task**: Deploy Vault integration to production

**Deployment Checklist**:
- [ ] Vault server deployed and secured
- [ ] TLS certificates configured
- [ ] Vault policies defined and tested
- [ ] Service tokens generated and distributed
- [ ] Monitoring and alerting configured
- [ ] Backup and disaster recovery plan

**Monitoring**:
- Vault connection health
- Key retrieval latency
- Token expiration alerts
- Failed authentication attempts

## 🔗 Dependencies

**Existing Components**:
- ✅ VaultKeyProvider (`include/security/vault_key_provider.h`)
- ✅ FieldEncryption (`include/security/encryption.h`)
- ✅ KeyProvider interface (`include/security/key_provider.h`)

**External Dependencies**:
- HashiCorp Vault server (1.15+)
- libcurl (for HTTP requests)
- OpenSSL (for cryptography)

## ✅ Acceptance Criteria

### Functional Requirements
- [ ] VaultKeyProvider integrated with LoRA storage
- [ ] Adapters encrypted with Vault-managed keys
- [ ] Key rotation works without data migration
- [ ] All error cases handled gracefully

### Non-Functional Requirements
- [ ] Key retrieval latency <100ms (cached)
- [ ] Key retrieval latency <500ms (cold)
- [ ] Cache hit rate >95%
- [ ] No plaintext keys in logs or memory dumps

### Security Requirements
- [ ] No MockKeyProvider in production code
- [ ] All key operations audited
- [ ] Token rotation supported
- [ ] Least privilege policies enforced

### Production Readiness
- [ ] Vault setup documented
- [ ] Configuration validated
- [ ] Monitoring implemented
- [ ] All tests passing

## 📊 Effort Estimation

**Estimated Time**: 3-4 days

**Breakdown**:
- VaultKeyProvider integration: 1 day
- Vault setup and configuration: 0.5 day
- Key rotation implementation: 1 day
- Testing: 1 day
- Documentation: 0.5 day

**Complexity**: Medium - Uses existing Vault implementation, requires configuration and testing

## 📚 References

**Themis Documentation**:
- `include/security/vault_key_provider.h` - VaultKeyProvider API
- `include/security/encryption.h` - FieldEncryption API
- `LORA_STORAGE_BACKEND_COMPLETION.md` - Storage backend docs

**External References**:
- [HashiCorp Vault KV v2](https://www.vaultproject.io/docs/secrets/kv/kv-v2)
- [Vault Authentication](https://www.vaultproject.io/docs/auth)
- [Vault Policies](https://www.vaultproject.io/docs/concepts/policies)

---

**Created**: 16. Januar 2026
**Status**: 📋 Ready for Implementation
**Priority**: P1 - High (Production Security)
