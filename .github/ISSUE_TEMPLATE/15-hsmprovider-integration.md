---
name: "🔐 HSMProvider Integration für LoRa Adapters"
about: Integrate Hardware Security Module for LoRA adapter encryption
title: "[Security] Integrate HSMProvider (PKCS#11) for LoRA Adapter Encryption"
labels: priority:P1, type:security, area:security, area:llm, effort:large, phase:production
assignees: ''

---

## 📋 Description

Integrate the existing `HSMProvider` implementation with LoRA adapter storage to enable hardware-backed encryption using PKCS#11 compatible HSMs. This provides the highest level of security by ensuring encryption keys never leave the hardware device.

**Related Files**:
- `include/security/hsm_provider.h` - Existing HSMProvider implementation
- `src/llm/lora_framework/lora_storage_service_themisdb.cpp:41-54` - Integration point
- `LORA_STORAGE_BACKEND_COMPLETION.md` - Implementation documentation

**Supported HSMs**:
- Thales/SafeNet Luna HSM
- Utimaco CryptoServer
- AWS CloudHSM
- SoftHSM2 (software emulation for testing)

**Current Status**: MockKeyProvider used in development (NOT suitable for production)

## 🎯 Goals

- [ ] Integrate HSMProvider with LoRA storage encryption
- [ ] Support PKCS#11 for hardware-backed keys
- [ ] Implement HSM-backed encryption/decryption
- [ ] Test with SoftHSM2 and real HSM
- [ ] Document HSM setup and configuration
- [ ] Production deployment guide

## 📝 Tasks

### 1. HSMProvider Adapter for KeyProvider Interface

**Task**: Create adapter to use HSMProvider with FieldEncryption

**Challenge**: HSMProvider doesn't directly implement KeyProvider interface

**Solution**: Create HSMKeyProviderAdapter
```cpp
// File: include/security/hsm_key_provider_adapter.h (new)

class HSMKeyProviderAdapter : public KeyProvider {
public:
    explicit HSMKeyProviderAdapter(std::shared_ptr<security::HSMProvider> hsm);
    
    // KeyProvider interface
    std::vector<uint8_t> getKey(const std::string& key_id) override;
    std::vector<uint8_t> getKey(const std::string& key_id, uint32_t version) override;
    uint32_t rotateKey(const std::string& key_id) override;
    // ... other methods ...
    
private:
    std::shared_ptr<security::HSMProvider> hsm_;
    std::string key_label_;  // HSM key label
};
```

**Tasks**:
- [ ] Create HSMKeyProviderAdapter class
- [ ] Implement KeyProvider interface methods
- [ ] Map key_id to HSM key labels
- [ ] Handle HSM-specific errors
- [ ] Test adapter with mock HSM

**Files**:
- `include/security/hsm_key_provider_adapter.h` (new)
- `src/security/hsm_key_provider_adapter.cpp` (new)

### 2. HSM Configuration

**Task**: Configure PKCS#11 connection for LoRA adapters

**HSM Configuration**:
```cpp
// File: src/llm/lora_framework/lora_storage_service_themisdb.cpp

security::HSMConfig hsm_config;
hsm_config.library_path = config_.hsm_library_path;  // e.g., "/usr/lib/softhsm/libsofthsm2.so"
hsm_config.slot_id = config_.hsm_slot_id;            // HSM slot (default: 0)
hsm_config.pin = config_.hsm_pin;                    // User PIN
hsm_config.key_label = "lora-adapter-key";           // Key label in HSM
hsm_config.signature_algorithm = "RSA-SHA256";
hsm_config.session_pool_size = 4;                    // Parallel sessions

auto hsm = std::make_unique<security::HSMProvider>(hsm_config);
if (!hsm->initialize()) {
    throw std::runtime_error("HSM initialization failed: " + hsm->getLastError());
}

auto key_provider = std::make_shared<HSMKeyProviderAdapter>(std::move(hsm));
encryption_ = std::make_shared<FieldEncryption>(key_provider);
```

**Configuration Options**:
- [ ] PKCS#11 library path
- [ ] HSM slot ID
- [ ] User PIN (from secure config or env)
- [ ] Key label for LoRA adapters
- [ ] Session pool size for performance

**File**: `src/llm/lora_framework/lora_storage_service_themisdb.cpp`

### 3. LoRAStorageService Configuration

**Task**: Add HSM configuration to LoRAStorageService::Config

**Implementation**:
```cpp
// File: include/llm/lora_framework/lora_storage_service.h

struct Config {
    // ... existing fields ...
    
    // HSM configuration
    bool use_hsm_for_encryption;           // Enable HSM encryption
    std::string hsm_library_path;          // PKCS#11 library path
    uint32_t hsm_slot_id;                  // HSM slot (default: 0)
    std::string hsm_pin;                   // User PIN
    std::string hsm_key_label;             // Key label (default: "lora-adapter-key")
    uint32_t hsm_session_pool_size;        // Parallel sessions (default: 4)
};
```

**Tasks**:
- [ ] Add HSM configuration fields
- [ ] Read from config file or environment
- [ ] Validate required HSM settings
- [ ] Secure PIN handling (not in logs)

**File**: `include/llm/lora_framework/lora_storage_service.h`

### 4. SoftHSM2 Setup for Testing

**Task**: Set up SoftHSM2 for development and testing

**Installation** (Ubuntu/Debian):
```bash
# Install SoftHSM2
sudo apt-get install softhsm2

# Initialize token
softhsm2-util --init-token --slot 0 --label "ThemisDB-Test" --pin 1234 --so-pin 5678

# Generate key in SoftHSM
pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so \
  --login --pin 1234 \
  --keypairgen --key-type RSA:2048 \
  --label "lora-adapter-key"
```

**Configuration File** (`/etc/softhsm/softhsm2.conf`):
```
directories.tokendir = /var/lib/softhsm/tokens/
objectstore.backend = file
log.level = INFO
```

**Tasks**:
- [ ] Install SoftHSM2 on development machines
- [ ] Initialize test token
- [ ] Generate test keys
- [ ] Configure ThemisDB to use SoftHSM
- [ ] Document setup process

### 5. HSM-Backed Encryption/Decryption

**Task**: Implement encryption/decryption using HSM

**Flow**:
1. Generate Data Encryption Key (DEK) in software
2. Encrypt DEK with HSM key (Key Encryption Key - KEK)
3. Store encrypted DEK with adapter metadata
4. Use DEK for actual data encryption
5. On load: Decrypt DEK with HSM, use for data decryption

**Why not encrypt data directly with HSM?**
- Performance: HSM operations are slow (~10-50 ms)
- Data size: HSMs have limits on encrypted data size
- Flexibility: Allows caching and offline access

**Implementation**:
```cpp
// Encryption
auto dek = generateRandomKey(32);  // AES-256 key
auto encrypted_dek = hsm->encrypt(dek, "lora-adapter-key");
metadata.encrypted_dek = base64_encode(encrypted_dek);

// Store DEK in metadata, use it for data encryption
auto encrypted_data = aes_gcm_encrypt(adapter_data, dek);

// Decryption
auto encrypted_dek = base64_decode(metadata.encrypted_dek);
auto dek = hsm->decrypt(encrypted_dek, "lora-adapter-key");
auto adapter_data = aes_gcm_decrypt(encrypted_data, dek);
```

**Tasks**:
- [ ] Implement envelope encryption pattern
- [ ] Store encrypted DEK in metadata
- [ ] Use HSM for KEK operations only
- [ ] Cache decrypted DEKs (with TTL)
- [ ] Measure performance impact

**File**: `src/llm/lora_framework/lora_storage_service_themisdb.cpp`

### 6. HSM Key Management

**Task**: Manage encryption keys in HSM

**Key Lifecycle**:
```bash
# Generate master key in HSM
pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so \
  --login --pin 1234 \
  --keypairgen --key-type RSA:2048 \
  --label "lora-adapter-kek" \
  --id 01

# List keys
pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so \
  --login --pin 1234 \
  --list-objects

# Backup (export public key only)
pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so \
  --login --pin 1234 \
  --read-object --type pubkey --label "lora-adapter-kek" \
  --output-file kek-public.pem
```

**Tasks**:
- [ ] Generate master KEK in HSM
- [ ] Document key backup procedures
- [ ] Implement key rotation strategy
- [ ] Test key deletion (for compliance)

### 7. Error Handling and Resilience

**Task**: Handle HSM failures gracefully

**Error Scenarios**:
- HSM not available (device unplugged, network issue)
- Invalid PIN (CKR_PIN_INCORRECT)
- Session limit reached (CKR_SESSION_COUNT)
- Key not found (CKR_OBJECT_HANDLE_INVALID)

**Implementation**:
```cpp
try {
    auto hsm = std::make_unique<security::HSMProvider>(hsm_config);
    if (!hsm->initialize()) {
        throw std::runtime_error("HSM initialization failed");
    }
    // ... use HSM ...
} catch (const std::runtime_error& e) {
    spdlog::error("HSM error: {}", e.what());
    
    // Fallback strategy (if configured)
    if (config_.fallback_to_software_keys) {
        spdlog::warn("Falling back to VaultKeyProvider");
        // Use Vault instead
    } else {
        throw;  // Fail closed for security
    }
}
```

**Tasks**:
- [ ] Implement retry logic for transient errors
- [ ] Graceful degradation options (if configured)
- [ ] Comprehensive error logging
- [ ] Alert on HSM failures

### 8. Performance Optimization

**Task**: Optimize HSM performance for production

**Optimization Strategies**:
- Session pooling (reuse PKCS#11 sessions)
- DEK caching (avoid HSM decrypt for every read)
- Parallel operations (multiple sessions)
- Async HSM operations

**Session Pool**:
```cpp
// HSMProvider already supports session pooling
hsm_config.session_pool_size = 4;  // 4 parallel sessions

// Use round-robin for load balancing
auto session = hsm->acquireSession();
auto result = session->encrypt(data, key_label);
hsm->releaseSession(session);
```

**Benchmarks**:
- Measure HSM operation latency
- Compare with software encryption
- Profile bottlenecks

**Tasks**:
- [ ] Enable session pooling (already supported)
- [ ] Implement DEK caching
- [ ] Benchmark HSM vs software performance
- [ ] Tune pool size based on benchmarks

### 9. Testing

**Task**: Comprehensive testing with HSM

**Unit Tests**:
```cpp
TEST(HSMProvider, LoRAEncryptionWithSoftHSM) {
    // Configure SoftHSM
    security::HSMConfig config;
    config.library_path = "/usr/lib/softhsm/libsofthsm2.so";
    config.slot_id = 0;
    config.pin = "1234";
    config.key_label = "lora-adapter-key";
    
    auto hsm = std::make_unique<security::HSMProvider>(config);
    ASSERT_TRUE(hsm->initialize());
    
    // Test encryption roundtrip
    auto adapter = hsm_key_provider_adapter(hsm);
    // ... test save/load with encryption ...
}

TEST(HSMProvider, SessionPooling) {
    // Test parallel HSM operations
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            storage.saveAdapter("adapter-" + std::to_string(i), weights, metadata);
        });
    }
    for (auto& t : threads) t.join();
}
```

**Integration Tests**:
- [ ] Test with SoftHSM2
- [ ] Test with real HSM (if available)
- [ ] Test session exhaustion handling
- [ ] Test HSM disconnection scenarios

**Files**:
- `tests/test_lora_hsm_integration.cpp` (new)
- `tests/test_hsm_key_provider_adapter.cpp` (new)

### 10. Documentation

**Task**: Document HSM setup and usage

**Documentation Files**:
- [ ] `docs/de/security/hsm_lora_setup.md` - HSM setup guide
- [ ] `docs/de/security/softhsm_testing.md` - SoftHSM testing guide
- [ ] `LORA_STORAGE_BACKEND_COMPLETION.md` - Update with HSM details

**Content**:
- Supported HSM devices
- PKCS#11 library installation
- Key generation procedures
- ThemisDB configuration
- Troubleshooting guide
- Performance tuning

### 11. Production Deployment

**Task**: Deploy HSM integration to production

**Deployment Checklist**:
- [ ] HSM device installed and configured
- [ ] PKCS#11 library installed
- [ ] Master KEK generated in HSM
- [ ] Backup procedures established
- [ ] Monitoring configured
- [ ] Disaster recovery plan

**Security**:
- HSM physical security
- PIN security (secure storage)
- Key backup and recovery
- Audit logging

## 🔗 Dependencies

**Existing Components**:
- ✅ HSMProvider (`include/security/hsm_provider.h`)
- ✅ FieldEncryption (`include/security/encryption.h`)
- ✅ KeyProvider interface (`include/security/key_provider.h`)

**New Components**:
- ⚠️ HSMKeyProviderAdapter (to be created)

**External Dependencies**:
- PKCS#11 library (device-specific)
- SoftHSM2 (for testing)
- OpenSSL (for AES-GCM)

## ✅ Acceptance Criteria

### Functional Requirements
- [ ] HSMProvider integrated with LoRA storage
- [ ] Envelope encryption pattern implemented
- [ ] Keys never leave HSM device
- [ ] All CRUD operations work with HSM encryption

### Non-Functional Requirements
- [ ] HSM operation latency <50ms (with session pool)
- [ ] Throughput: >100 ops/sec with pool size=4
- [ ] No performance regression vs software encryption
- [ ] Session pool efficiency >90%

### Security Requirements
- [ ] No plaintext KEK in memory or logs
- [ ] DEK cached with TTL (max 5 minutes)
- [ ] Failed PIN attempts logged
- [ ] HSM operations audited

### Production Readiness
- [ ] SoftHSM testing automated
- [ ] Real HSM tested (if available)
- [ ] Documentation complete
- [ ] All tests passing

## 📊 Effort Estimation

**Estimated Time**: 5-6 days

**Breakdown**:
- HSMKeyProviderAdapter: 1 day
- Envelope encryption implementation: 1 day
- SoftHSM setup and testing: 1 day
- Performance optimization: 1 day
- Testing and validation: 1-2 days
- Documentation: 1 day

**Complexity**: Large - Requires new adapter, envelope encryption, and HSM-specific handling

## 📚 References

**Themis Documentation**:
- `include/security/hsm_provider.h` - HSMProvider API
- `include/security/encryption.h` - FieldEncryption API
- `LORA_STORAGE_BACKEND_COMPLETION.md` - Storage backend docs

**External References**:
- [PKCS#11 Specification](http://docs.oasis-open.org/pkcs11/pkcs11-base/v2.40/os/pkcs11-base-v2.40-os.html)
- [SoftHSM2 Documentation](https://github.com/opendnssec/SoftHSMv2)
- [AWS CloudHSM](https://docs.aws.amazon.com/cloudhsm/)
- [Thales Luna HSM](https://cpl.thalesgroup.com/encryption/hardware-security-modules)

---

**Created**: 16. Januar 2026
**Status**: 📋 Ready for Implementation
**Priority**: P1 - High (Production Security - Hardware-backed)
