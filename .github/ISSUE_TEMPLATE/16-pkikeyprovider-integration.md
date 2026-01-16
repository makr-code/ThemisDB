---
name: "🔐 PKIKeyProvider Integration für LoRa Adapters"
about: Integrate certificate-based key management for LoRA adapter encryption
title: "[Security] Integrate PKIKeyProvider for LoRA Adapter Encryption"
labels: priority:P2, type:security, area:security, area:llm, effort:medium, phase:production
assignees: ''

---

## 📋 Description

Integrate the existing `PKIKeyProvider` implementation with LoRA adapter storage to enable certificate-based key management. This approach uses X.509 certificates and public/private key pairs for encryption, suitable for environments where PKI infrastructure is already established.

**Related Files**:
- `include/security/pki_key_provider.h` - Existing PKIKeyProvider implementation
- `src/llm/lora_framework/lora_storage_service_themisdb.cpp:41-54` - Integration point
- `LORA_STORAGE_BACKEND_COMPLETION.md` - Implementation documentation

**Use Cases**:
- Organizations with existing PKI infrastructure
- Certificate-based key distribution
- Hybrid cloud deployments with cert-based auth
- Air-gapped environments requiring offline key management

**Current Status**: MockKeyProvider used in development (NOT suitable for production)

## 🎯 Goals

- [ ] Integrate PKIKeyProvider with LoRA storage encryption
- [ ] Support X.509 certificate-based key derivation
- [ ] Implement certificate rotation without data migration
- [ ] Test with self-signed and CA-signed certificates
- [ ] Document certificate setup and management
- [ ] Production deployment guide

## 📝 Tasks

### 1. PKIKeyProvider Configuration

**Task**: Configure PKI-based key management for LoRA adapters

**Certificate-Based Key Derivation**:
```cpp
// File: src/llm/lora_framework/lora_storage_service_themisdb.cpp

// Option 1: File-based certificates
auto key_provider = std::make_shared<PKIKeyProvider>(
    config_.cert_path,        // e.g., "/etc/themis/certs/lora-encryption.crt"
    config_.private_key_path  // e.g., "/etc/themis/keys/lora-encryption.key"
);

// Option 2: Certificate store
auto key_provider = std::make_shared<PKIKeyProvider>();
key_provider->loadFromStore(config_.cert_thumbprint);

encryption_ = std::make_shared<FieldEncryption>(key_provider);
```

**Key Derivation**: 
- Derive encryption key from certificate's public key
- Use PBKDF2 or HKDF for key derivation
- Include key version in derivation (for rotation)

**Tasks**:
- [ ] Replace MockKeyProvider with PKIKeyProvider
- [ ] Support file-based certificate loading
- [ ] Support certificate store integration (Windows)
- [ ] Implement key derivation function
- [ ] Test with different certificate types

**File**: `src/llm/lora_framework/lora_storage_service_themisdb.cpp`
**Lines**: 41-54

### 2. Certificate Generation and Management

**Task**: Generate and manage certificates for LoRA encryption

**Generate Self-Signed Certificate** (Development):
```bash
# Generate private key
openssl genrsa -out lora-encryption.key 4096

# Generate self-signed certificate
openssl req -new -x509 -key lora-encryption.key \
  -out lora-encryption.crt -days 365 \
  -subj "/CN=ThemisDB-LoRA-Encryption/O=ThemisDB/C=DE"

# Verify certificate
openssl x509 -in lora-encryption.crt -text -noout
```

**CA-Signed Certificate** (Production):
```bash
# Generate CSR
openssl req -new -key lora-encryption.key \
  -out lora-encryption.csr \
  -subj "/CN=themisdb-lora.example.com/O=ThemisDB/C=DE"

# Submit CSR to CA (manual or ACME)
# Receive signed certificate: lora-encryption.crt

# Verify chain
openssl verify -CAfile ca-bundle.crt lora-encryption.crt
```

**Tasks**:
- [ ] Document certificate generation process
- [ ] Create scripts for self-signed certs (dev/test)
- [ ] Document CA integration (production)
- [ ] Implement certificate validation
- [ ] Test certificate chain verification

### 3. LoRAStorageService Configuration

**Task**: Add PKI configuration to LoRAStorageService::Config

**Implementation**:
```cpp
// File: include/llm/lora_framework/lora_storage_service.h

struct Config {
    // ... existing fields ...
    
    // PKI configuration
    bool use_pki_for_encryption;           // Enable PKI encryption
    std::string cert_path;                 // Certificate file path
    std::string private_key_path;          // Private key file path
    std::string ca_bundle_path;            // CA bundle for verification
    std::string cert_thumbprint;           // Certificate thumbprint (store lookup)
    bool verify_certificate_chain;         // Verify against CA (default: true)
};
```

**Configuration Sources**:
- Config file (YAML)
- Environment variables
- Certificate store (Windows/macOS)

**Tasks**:
- [ ] Add PKI configuration fields
- [ ] Support multiple config sources
- [ ] Validate certificate before use
- [ ] Secure private key handling

**File**: `include/llm/lora_framework/lora_storage_service.h`

### 4. Key Derivation from Certificate

**Task**: Derive encryption keys from certificate public key

**Key Derivation Function** (KDF):
```cpp
// Use certificate public key as input to KDF
std::vector<uint8_t> deriveKeyFromCertificate(
    const std::string& cert_pem,
    const std::string& key_id,
    uint32_t version
) {
    // 1. Extract public key from certificate
    auto public_key = extractPublicKeyFromCert(cert_pem);
    
    // 2. Serialize public key to bytes
    auto pubkey_bytes = serializePublicKey(public_key);
    
    // 3. Create derivation context
    std::string context = key_id + ":" + std::to_string(version);
    
    // 4. Derive key using HKDF-SHA256
    std::vector<uint8_t> derived_key(32);  // AES-256
    HKDF_SHA256(
        derived_key.data(),
        pubkey_bytes.data(), pubkey_bytes.size(),
        context.c_str(), context.length(),
        nullptr, 0  // No salt
    );
    
    return derived_key;
}
```

**Why KDF?**
- Public key not directly usable as symmetric key
- KDF provides key versioning support
- Cryptographically secure derivation

**Tasks**:
- [ ] Implement HKDF-SHA256 key derivation
- [ ] Include key_id and version in context
- [ ] Test key derivation consistency
- [ ] Validate derived key properties

**File**: `src/security/pki_key_provider.cpp`

### 5. Certificate Rotation

**Task**: Support certificate rotation without data re-encryption

**Rotation Strategy**: Store certificate version in metadata
```cpp
struct AdapterMetadata {
    // ... existing fields ...
    std::string cert_thumbprint;   // Certificate used for encryption
    uint32_t key_version;          // Key derivation version
};

// Save: Store current certificate thumbprint
metadata.cert_thumbprint = getCertificateThumbprint(cert);
metadata.key_version = 1;

// Load: Use stored certificate to derive key
auto historical_cert = loadCertificateByThumbprint(metadata.cert_thumbprint);
auto key = deriveKeyFromCertificate(historical_cert, key_id, metadata.key_version);
```

**Certificate Archive**:
- Keep historical certificates for decryption
- Archive location: `/etc/themis/certs/archive/`
- Automatic cleanup after retention period

**Tasks**:
- [ ] Store certificate thumbprint in metadata
- [ ] Implement certificate archive system
- [ ] Test rotation scenario
- [ ] Document rotation procedure

**File**: `src/llm/lora_framework/lora_storage_service_themisdb.cpp`

### 6. Certificate Validation

**Task**: Validate certificates before use

**Validation Checks**:
```cpp
bool validateCertificate(const std::string& cert_pem) {
    // 1. Parse certificate
    auto cert = X509_from_PEM(cert_pem);
    if (!cert) return false;
    
    // 2. Check expiration
    if (X509_is_expired(cert)) {
        spdlog::error("Certificate expired");
        return false;
    }
    
    // 3. Verify signature (if CA provided)
    if (config_.ca_bundle_path) {
        if (!X509_verify_chain(cert, ca_bundle)) {
            spdlog::error("Certificate chain verification failed");
            return false;
        }
    }
    
    // 4. Check key usage
    if (!X509_has_key_usage(cert, X509_KU_KEY_ENCIPHERMENT)) {
        spdlog::error("Certificate lacks key encipherment usage");
        return false;
    }
    
    return true;
}
```

**Tasks**:
- [ ] Implement certificate parsing
- [ ] Check expiration dates
- [ ] Verify certificate chain (if CA provided)
- [ ] Validate key usage extensions
- [ ] Test with expired/invalid certificates

### 7. Certificate Store Integration (Windows/macOS)

**Task**: Support system certificate stores

**Windows Certificate Store**:
```cpp
#ifdef _WIN32
// Open Windows certificate store
HCERTSTORE store = CertOpenSystemStore(NULL, "MY");

// Find certificate by thumbprint
PCCERT_CONTEXT cert = CertFindCertificateInStore(
    store,
    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
    0,
    CERT_FIND_HASH,
    &thumbprint_blob,
    NULL
);

// Use certificate
auto key_provider = std::make_shared<PKIKeyProvider>(cert);
#endif
```

**Tasks**:
- [ ] Implement Windows certificate store access
- [ ] Implement macOS Keychain access
- [ ] Support certificate thumbprint lookup
- [ ] Test on Windows and macOS

**File**: `src/security/pki_key_provider.cpp` (platform-specific)

### 8. Error Handling

**Task**: Handle certificate-related errors

**Error Scenarios**:
- Certificate file not found
- Certificate expired
- Invalid certificate format
- Missing private key
- Certificate chain verification failed

**Implementation**:
```cpp
try {
    auto key_provider = std::make_shared<PKIKeyProvider>(
        config_.cert_path,
        config_.private_key_path
    );
    
    if (!key_provider->validateCertificate()) {
        throw std::runtime_error("Certificate validation failed");
    }
    
    encryption_ = std::make_shared<FieldEncryption>(key_provider);
} catch (const std::exception& e) {
    spdlog::error("PKI initialization failed: {}", e.what());
    
    // Certificate expiration notification
    if (isCertificateExpiringSoon(config_.cert_path, /*days=*/30)) {
        spdlog::warn("Certificate expires in <30 days - renewal required");
    }
    
    throw;
}
```

**Tasks**:
- [ ] Comprehensive error handling
- [ ] Certificate expiration warnings
- [ ] Automatic certificate renewal (optional)
- [ ] Logging with context

### 9. Testing

**Task**: Test PKI integration

**Unit Tests**:
```cpp
TEST(PKIKeyProvider, KeyDerivation) {
    // Generate test certificate
    auto cert = generateSelfSignedCert("CN=Test");
    
    // Derive key
    auto key1 = deriveKeyFromCertificate(cert, "test_key", 1);
    auto key2 = deriveKeyFromCertificate(cert, "test_key", 1);
    
    // Keys should be identical
    EXPECT_EQ(key1, key2);
}

TEST(PKIKeyProvider, CertificateRotation) {
    // Encrypt with cert v1
    auto cert1 = generateSelfSignedCert("CN=Cert1");
    // ... save adapter with cert1 ...
    
    // Rotate to cert v2
    auto cert2 = generateSelfSignedCert("CN=Cert2");
    
    // Old adapter still readable with archived cert1
    auto loaded = storage.loadAdapter("test");
    EXPECT_TRUE(loaded.has_value());
}

TEST(PKIKeyProvider, ExpiredCertificate) {
    auto expired_cert = loadExpiredTestCert();
    
    EXPECT_THROW(
        PKIKeyProvider(expired_cert, private_key),
        std::runtime_error
    );
}
```

**Integration Tests**:
- [ ] Test with self-signed certificates
- [ ] Test with CA-signed certificates
- [ ] Test certificate rotation
- [ ] Test certificate store integration (Windows)

**Files**:
- `tests/test_lora_pki_integration.cpp` (new)
- `tests/test_pki_key_provider.cpp` (new)

### 10. Documentation

**Task**: Document PKI setup and usage

**Documentation Files**:
- [ ] `docs/de/security/pki_lora_setup.md` - PKI setup guide
- [ ] `docs/de/security/certificate_management.md` - Cert lifecycle
- [ ] `LORA_STORAGE_BACKEND_COMPLETION.md` - Update with PKI details

**Content**:
- Certificate generation (dev vs production)
- Key derivation process
- Certificate rotation procedures
- Certificate store integration
- Troubleshooting guide

### 11. Production Deployment

**Task**: Deploy PKI integration to production

**Deployment Checklist**:
- [ ] Certificates generated and distributed
- [ ] Certificate chain verified
- [ ] Certificate archive system configured
- [ ] Monitoring for certificate expiration
- [ ] Renewal process documented

**Certificate Monitoring**:
- Expiration date tracking
- Renewal alerts (30/14/7 days)
- Certificate validation health checks

## 🔗 Dependencies

**Existing Components**:
- ✅ PKIKeyProvider (`include/security/pki_key_provider.h`)
- ✅ FieldEncryption (`include/security/encryption.h`)
- ✅ KeyProvider interface (`include/security/key_provider.h`)

**External Dependencies**:
- OpenSSL (for X.509 and key derivation)
- Platform certificate stores (Windows/macOS)

## ✅ Acceptance Criteria

### Functional Requirements
- [ ] PKIKeyProvider integrated with LoRA storage
- [ ] Key derivation from certificates working
- [ ] Certificate rotation supported
- [ ] Certificate validation implemented

### Non-Functional Requirements
- [ ] Key derivation latency <10ms
- [ ] Certificate validation <50ms
- [ ] No performance impact vs other providers
- [ ] Certificate cache hit rate >95%

### Security Requirements
- [ ] Certificate expiration detected
- [ ] Certificate chain verified (when CA provided)
- [ ] Private keys securely stored
- [ ] Historical certificates archived

### Production Readiness
- [ ] Certificate generation documented
- [ ] Rotation procedures documented
- [ ] Monitoring configured
- [ ] All tests passing

## 📊 Effort Estimation

**Estimated Time**: 3-4 days

**Breakdown**:
- PKIKeyProvider integration: 1 day
- Key derivation implementation: 1 day
- Certificate validation: 0.5 day
- Certificate rotation: 0.5 day
- Testing: 1 day
- Documentation: 0.5 day

**Complexity**: Medium - Certificate handling and key derivation required

## 📚 References

**Themis Documentation**:
- `include/security/pki_key_provider.h` - PKIKeyProvider API
- `include/security/encryption.h` - FieldEncryption API
- `LORA_STORAGE_BACKEND_COMPLETION.md` - Storage backend docs

**External References**:
- [OpenSSL X.509](https://www.openssl.org/docs/man1.1.1/man3/X509_new.html)
- [HKDF (RFC 5869)](https://tools.ietf.org/html/rfc5869)
- [Certificate Key Usage](https://tools.ietf.org/html/rfc5280#section-4.2.1.3)

---

**Created**: 16. Januar 2026
**Status**: 📋 Ready for Implementation
**Priority**: P2 - Medium (Alternative to Vault/HSM)
