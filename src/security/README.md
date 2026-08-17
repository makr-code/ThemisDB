> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# Security Module

Comprehensive security infrastructure for ThemisDB providing encryption, authentication, authorization, and compliance features.

## Module Purpose

Provides encryption, key management, and PKI integration for ThemisDB, implementing AES-256-GCM field-level encryption, TLS certificate management, and hardware security module (HSM) integration.

## Subsystem Scope

**In scope:** AES-256-GCM field-level encryption, TLS certificate lifecycle management, PKI client integration, key rotation, HSM support.

**Out of scope:** Network protocol routing, storage/query execution internals, and non-security business logic.

## Relevant Interfaces

- `field_encryption.cpp` — AES-256-GCM field-level encryption/decryption
- `vault_key_provider.cpp` / `hsm_provider_pkcs11.cpp` — key lifecycle management and rotation
- `pki_key_provider.cpp` — PKI certificate-based key integration
- `rbac.cpp` / `access_control_manager.cpp` — RBAC and ABAC authorization

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — Defense-in-depth security stack (TLS/mTLS, encryption, RBAC/ABAC, HSM integration, audit/compliance) is operational.

### Developer Update (2026-08-17)

- TSA transport hardening for RFC-3161 requests is implemented in `src/security/timestamp_authority_openssl.cpp` (HTTPS-only, HTTPS-protocol restriction, TLS minimum, connect/total timeout hardening).
- `src/security/timestamp_authority.cpp` remains the non-OpenSSL fallback/stub bridge path and is not the hardened TSA transport implementation.
- A full fresh security-gap rescan is still open and explicitly tracked in `src/security/MODULE_GAPS.md`.

## Architecture Overview

ThemisDB's security module implements defense-in-depth with multiple layers:

```
┌─────────────────────────────────────────────────────────────────┐
│                        Security Layers                          │
├─────────────────────────────────────────────────────────────────┤
│  Layer 1: Transport Security (TLS 1.3, mTLS)                   │
│  Layer 2: Authentication (USB Admin, PKI, Multi-factor)        │
│  Layer 3: Authorization (RBAC with Role Hierarchy)             │
│  Layer 4: Data Protection (Field-Level AES-256-GCM)            │
│  Layer 5: Audit & Compliance (Timestamping, eIDAS)             │
│  Layer 6: Threat Detection (AQL Injection, Malware Scan)       │
└─────────────────────────────────────────────────────────────────┘
```

### Component Architecture

```
                    ┌─────────────────────────┐
                    │   Security Manager      │
                    │   (Orchestrator)        │
                    └───────────┬─────────────┘
                                │
            ┌───────────────────┼───────────────────┐
            │                   │                   │
    ┌───────▼───────┐   ┌──────▼──────┐   ┌───────▼────────┐
    │ Encryption     │   │    RBAC     │   │  Key Mgmt      │
    │ Engine         │   │  (AuthZ)    │   │  (Vault/HSM)   │
    └───────┬───────┘   └──────┬──────┘   └───────┬────────┘
            │                   │                   │
    ┌───────▼───────────────────▼───────────────────▼────────┐
    │              Field-Level Encryption                     │
    │  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
    │  │ Document │  │  Array   │  │  VRAM    │             │
    │  │  Fields  │  │  Fields  │  │  Secure  │             │
    │  └──────────┘  └──────────┘  └──────────┘             │
    └─────────────────────────────────────────────────────────┘
            │                   │                   │
    ┌───────▼───────┐   ┌──────▼──────┐   ┌───────▼────────┐
    │ Signing &     │   │  Malware    │   │  Injection     │
    │ Timestamping  │   │  Scanner    │   │  Detection     │
    │ (CMS/eIDAS)   │   │  (Manifest) │   │  (AQL)         │
    └───────────────┘   └─────────────┘   └────────────────┘
```

### Key Management Hierarchy

```
                ┌──────────────────────────┐
                │   Master Key (HSM)       │
                │   Hardware-Protected     │
                └────────────┬─────────────┘
                             │
                  ┌──────────┴──────────┐
                  │  Key Encryption Key │
                  │      (KEK)          │
                  └──────────┬──────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
┌───────▼───────┐   ┌────────▼────────┐  ┌───────▼───────┐
│  Data Encrypt │   │  Data Encrypt   │  │  Data Encrypt │
│  Key (DEK) v1 │   │  Key (DEK) v2   │  │  Key (DEK) v3 │
│  DEPRECATED   │   │    ACTIVE       │  │   ROTATING    │
└───────────────┘   └─────────────────┘  └───────────────┘
```

## Core Components

### 1. Field-Level Encryption

**Purpose**: AES-256-GCM encryption with field-level granularity

**Features**:
- AES-256-GCM authenticated encryption
- Per-field encryption with separate keys
- Automatic key rotation with version tracking
- Support for encrypted arrays and nested objects
- Zero-copy decryption for performance
- Streaming encryption for large payloads

**Key Classes**:
- `FieldEncryption`: Main encryption engine
- `EncryptedBlob`: Serialized encrypted data with metadata
- `EncryptedField`: Document field wrapper with encryption

**Example - Basic Encryption**:
```cpp
#include "security/encryption.h"
#include "security/vault_key_provider.h"

// Initialize Vault-backed key provider
VaultConfig vault_config;
vault_config.url = "https://vault.example.com:8200";
vault_config.token = "s.your-vault-token";
vault_config.mount_path = "secret";

auto key_provider = std::make_shared<VaultKeyProvider>(vault_config);

// Create field encryption engine
FieldEncryption encryption(key_provider);

// Encrypt sensitive data
std::string ssn = "123-45-6789";
auto encrypted = encryption.encrypt("user_pii", ssn);

// Store encrypted blob (Base64 encoded)
std::string stored = encrypted.toBase64();
// Result: "user_pii:1:YWJjZGVmZ2hpams=:SGVsbG8gV29ybGQ=:MTIzNDU2Nzg5MEFCQ0RFRg=="

// Decrypt when needed
auto decrypted_blob = EncryptedBlob::fromBase64(stored);
std::string decrypted = encryption.decrypt(decrypted_blob);
// decrypted == "123-45-6789"
```

**Example - Document Field Encryption**:
```cpp
#include "security/encrypted_field.h"

// Encrypt multiple document fields
nlohmann::json document = {
    {"name", "John Doe"},
    {"email", "john@example.com"},
    {"ssn", "123-45-6789"},      // Sensitive
    {"credit_card", "4111-1111-1111-1111"}, // Sensitive
    {"address", {
        {"street", "123 Main St"},
        {"zipcode", "12345"}      // Sensitive
    }}
};

// Define which fields to encrypt
std::vector<std::string> sensitive_fields = {
    "ssn",
    "credit_card",
    "address.zipcode"
};

// Encrypt fields in-place
for (const auto& field_path : sensitive_fields) {
    auto value = getNestedField(document, field_path);
    auto encrypted = encryption.encrypt("user_pii", value);
    setNestedField(document, field_path, encrypted.toBase64());
}
```

**Example - Key Rotation**:
```cpp
// Rotate key for a specific key_id
encryption.rotateKey("user_pii");

// During rotation, old version can still decrypt
auto old_encrypted = EncryptedBlob::fromBase64(stored_old_data);
std::string decrypted = encryption.decrypt(old_encrypted); // Works!

// New encryptions use new version
auto new_encrypted = encryption.encrypt("user_pii", "new-data");
// new_encrypted.key_version == 2

// Re-encrypt old data with new key (background job)
auto re_encrypted = encryption.reEncrypt(old_encrypted);
// re_encrypted.key_version == 2
```

**Performance Considerations**:
- Encryption overhead: ~5-10μs per field (256-byte payload)
- Decryption overhead: ~3-7μs per field
- Key cache: ~100ns lookup time (in-memory)
- Vault API call: ~50-100ms (cached for 1 hour)
- HSM operation: ~5-20ms (hardware-dependent)

**Serialization Format**:
```
{key_id}:{version}:{base64(iv)}:{base64(ciphertext)}:{base64(tag)}

Components:
- key_id: Logical identifier (e.g., "user_pii", "payment_info")
- version: Key version for rotation (1, 2, 3, ...)
- iv: 12-byte initialization vector (random, unique per encryption)
- ciphertext: Encrypted payload
- tag: 16-byte GCM authentication tag
```

### 2. Key Management

**Purpose**: Centralized cryptographic key lifecycle management

**Supported Backends**:
- HashiCorp Vault (Transit engine)
- HSM via PKCS#11 (Luna, CloudHSM, SoftHSM)
- PKI with certificate-based keys
- Mock provider (testing only)

**Key Providers**:

#### Vault Key Provider
```cpp
#include "security/vault_key_provider.h"

VaultConfig config;
config.url = "https://vault.example.com:8200";
config.token = "s.your-vault-token";
config.mount_path = "transit";  // Transit secrets engine
config.namespace_path = "production";

auto provider = std::make_shared<VaultKeyProvider>(config);

// Create new encryption key
KeyMetadata meta;
meta.key_id = "user_pii";
meta.algorithm = "aes256-gcm96";
meta.exportable = false;  // Key never leaves Vault

provider->createKey(meta);

// Retrieve key for encryption
auto key_material = provider->getKey("user_pii", 1);

// Rotate key (creates new version)
provider->rotateKey("user_pii");

// List all key versions
auto versions = provider->listKeyVersions("user_pii");
```

#### HSM Key Provider
```cpp
#include "security/hsm_key_provider_adapter.h"
#include "security/hsm_provider.h"

// Initialize HSM (PKCS#11)
HSMConfig hsm_config;
hsm_config.library_path = "/usr/lib/softhsm/libsofthsm2.so";
hsm_config.slot_id = 0;
hsm_config.pin = "1234";

auto hsm = std::make_shared<HSMProvider>(hsm_config);
if (!hsm->initialize()) {
    throw std::runtime_error("HSM initialization failed");
}

// Wrap HSM as key provider
auto provider = std::make_shared<HSMKeyProviderAdapter>(hsm);

// Generate key in HSM (never exported)
KeyMetadata meta;
meta.key_id = "payment_key";
meta.algorithm = "AES-256-GCM";

provider->createKey(meta);
```

#### PKI Key Provider
```cpp
#include "security/pki_key_provider.h"

PKIConfig pki_config;
pki_config.cert_path = "/etc/themis/certs/encryption.crt";
pki_config.key_path = "/etc/themis/certs/encryption.key";
pki_config.ca_bundle_path = "/etc/themis/certs/ca-bundle.crt";

auto provider = std::make_shared<PKIKeyProvider>(pki_config);

// Derive encryption key from certificate
auto key = provider->getKey("default", 1);
```

**Key Lifecycle**:
```
┌──────────┐    ┌──────────┐    ┌─────────────┐    ┌──────────┐
│  ACTIVE  │───▶│ ROTATING │───▶│ DEPRECATED  │───▶│ DELETED  │
└──────────┘    └──────────┘    └─────────────┘    └──────────┘
     │               │                  │                │
   Encrypt        Encrypt          Decrypt only      Forbidden
   Decrypt        Decrypt          (grace period)
```

**Key Rotation Strategy**:
1. **Create new key version** (v2) while v1 is still active
2. **Dual-write period**: Encrypt with v2, can decrypt v1 & v2
3. **Deprecate old version**: v1 marked DEPRECATED (decrypt-only)
4. **Re-encrypt old data**: Background job re-encrypts v1→v2
5. **Delete old version**: After re-encryption complete

### 3. Role-Based Access Control (RBAC)

**Purpose**: Fine-grained authorization with role hierarchy

**Features**:
- Role inheritance (e.g., admin inherits operator)
- Resource wildcards (data:*, *:read)
- Permission composition
- Dynamic role assignment
- Audit logging of permission checks

**Key Classes**:
- `RBAC`: Main authorization engine
- `Role`: Collection of permissions
- `Permission`: Resource:Action pair

**Example - Define Roles**:
```cpp
#include "security/rbac.h"

RBACConfig config;
config.config_path = "/etc/themis/rbac.json";
config.enable_role_inheritance = true;

RBAC rbac(config);

// Define roles
Role admin_role;
admin_role.name = "admin";
admin_role.description = "Full system administrator";
admin_role.permissions = {
    {"*", "*"},           // All resources, all actions
    {"keys", "rotate"},   // Explicit key rotation
    {"audit", "delete"}   // Can delete audit logs
};

Role operator_role;
operator_role.name = "operator";
operator_role.description = "Database operator";
operator_role.permissions = {
    {"data", "read"},
    {"data", "write"},
    {"config", "read"},
    {"audit", "read"}
};

Role analyst_role;
analyst_role.name = "analyst";
analyst_role.description = "Read-only data analyst";
analyst_role.permissions = {
    {"data", "read"},
    {"audit", "read"}
};
analyst_role.inherits = {"readonly"};  // Inherit from base role

// Register roles
rbac.addRole(admin_role);
rbac.addRole(operator_role);
rbac.addRole(analyst_role);

// Assign roles to users
rbac.assignRole("alice@example.com", "admin");
rbac.assignRole("bob@example.com", "operator");
rbac.assignRole("charlie@example.com", "analyst");
```

**Example - Check Permissions**:
```cpp
// Check if user can perform action
bool can_write = rbac.hasPermission("bob@example.com", "data", "write");
// true (operator has data:write)

bool can_delete = rbac.hasPermission("charlie@example.com", "data", "delete");
// false (analyst only has data:read)

bool can_rotate = rbac.hasPermission("alice@example.com", "keys", "rotate");
// true (admin has *:* and explicit keys:rotate)

// Get all permissions for user
auto permissions = rbac.getUserPermissions("bob@example.com");
for (const auto& perm : permissions) {
    std::cout << perm.toString() << std::endl;
}
// Output:
// data:read
// data:write
// config:read
// audit:read
```

**Example - Role Hierarchy**:
```cpp
// Define role inheritance
Role readonly_role;
readonly_role.name = "readonly";
readonly_role.permissions = {
    {"data", "read"},
    {"config", "read"}
};

Role poweruser_role;
poweruser_role.name = "poweruser";
poweruser_role.permissions = {
    {"data", "write"},
    {"data", "delete"}
};
poweruser_role.inherits = {"readonly"};  // Inherits read permissions

rbac.addRole(readonly_role);
rbac.addRole(poweruser_role);

rbac.assignRole("dave@example.com", "poweruser");

// Dave has both poweruser and inherited readonly permissions
bool can_read = rbac.hasPermission("dave@example.com", "data", "read");
// true (inherited from readonly)

bool can_write = rbac.hasPermission("dave@example.com", "data", "write");
// true (poweruser permission)
```

**RBAC Configuration File** (JSON):
```json
{
  "roles": [
    {
      "name": "admin",
      "description": "System administrator",
      "permissions": [
        {"resource": "*", "action": "*"}
      ]
    },
    {
      "name": "operator",
      "description": "Database operator",
      "permissions": [
        {"resource": "data", "action": "read"},
        {"resource": "data", "action": "write"},
        {"resource": "config", "action": "read"}
      ]
    },
    {
      "name": "analyst",
      "description": "Data analyst",
      "inherits": ["readonly"],
      "permissions": [
        {"resource": "analytics", "action": "*"}
      ]
    }
  ],
  "users": [
    {"username": "alice@example.com", "roles": ["admin"]},
    {"username": "bob@example.com", "roles": ["operator"]},
    {"username": "charlie@example.com", "roles": ["analyst"]}
  ]
}
```

### 4. AQL Injection Detection

**Purpose**: Detect and prevent AQL (Aqua Query Language) injection attacks

**Features**:
- Pattern-based detection
- AST analysis for malicious patterns
- Parameterized query enforcement
- Query complexity limits
- Rate limiting for suspicious patterns

**Key Classes**:
- `AQLInjectionDetector`: Main detection engine

**Example**:
```cpp
#include "security/aql_injection_detector.h"

AQLInjectionDetector detector;

// Safe query (parameterized)
std::string safe_query = R"(
    FOR user IN users
    FILTER user.email == @email
    RETURN user
)";
nlohmann::json safe_params = {{"email", "alice@example.com"}};

bool is_safe = detector.isSafe(safe_query, safe_params);
// true

// Injection attempt
std::string malicious_query = R"(
    FOR user IN users
    FILTER user.email == 'alice@example.com' OR 1==1
    RETURN user
)";

bool is_malicious = detector.isSafe(malicious_query, {});
// false - detects SQL-style injection pattern
```

**Detection Patterns**:
- SQL injection patterns: `' OR '1'='1`, `'; DROP TABLE`
- Command injection: `; rm -rf`, `| cat /etc/passwd`
- Path traversal: `../../../etc/passwd`
- Script injection: `<script>`, `javascript:`
- Excessive complexity: Nested subqueries beyond limit

### 5. Malware Scanning

**Purpose**: Scan uploaded binaries and LoRA adapters for malware

**Features**:
- ClamAV integration
- Binary manifest validation
- Signature verification
- Checksum validation
- Quarantine for suspicious files

**Key Classes**:
- `MalwareScanner`: Virus scanning interface
- `BinaryManifest`: Binary metadata and signatures
- `ManifestSigner`: Digital signature generation

**Example - Scan Binary**:
```cpp
#include "security/malware_scanner.h"

MalwareScannerConfig config;
config.clamd_host = "localhost";
config.clamd_port = 3310;
config.enabled = true;

MalwareScanner scanner(config);

// Scan uploaded LoRA adapter
std::string file_path = "/uploads/lora-adapter.bin";
auto result = scanner.scanFile(file_path);

if (result.is_clean) {
    std::cout << "File is clean" << std::endl;
} else {
    std::cout << "Threat detected: " << result.threat_name << std::endl;
    // Quarantine file
    scanner.quarantine(file_path);
}
```

**Example - Binary Manifest**:
```cpp
#include "security/binary_manifest.h"
#include "security/manifest_signer.h"

// Create manifest for binary
BinaryManifest manifest;
manifest.name = "lora-adapter-v1";
manifest.version = "1.0.0";
manifest.hash_algorithm = "SHA256";
manifest.hash = computeSHA256(binary_data);
manifest.size_bytes = binary_data.size();
manifest.timestamp = std::chrono::system_clock::now();

// Sign manifest
ManifestSigner signer(cert, private_key);
auto signature = signer.sign(manifest);

manifest.signature = signature;

// Verify manifest before loading
bool valid = signer.verify(manifest);
if (valid) {
    // Load binary
    loadLoRAAdapter(binary_data);
}
```

### 6. VRAM Secure Clearing

**Purpose**: Securely clear GPU VRAM to prevent data leakage

**Features**:
- Zero-fill VRAM after deallocation
- Multi-pass overwrite (DOD 5220.22-M)
- Verification of clearing
- Async clearing for performance

**Key Classes**:
- `VRAMSecureClear`: GPU memory sanitization

**Example**:
```cpp
#include "security/vram_secure_clear.h"

VRAMSecureClear vram_clear;

// Allocate VRAM for sensitive data
void* vram_ptr = cudaMalloc(size);
// ... use VRAM ...

// Secure clear before deallocation
vram_clear.secureErase(vram_ptr, size);
cudaFree(vram_ptr);

// Multi-pass clear (DOD standard)
vram_clear.secureEraseMultiPass(vram_ptr, size, 3);
```

### 7. CMS Signing & eIDAS Compliance

**Purpose**: Cryptographic Message Syntax (CMS) signing for qualified signatures

**Features**:
- CMS/PKCS#7 signing
- eIDAS qualified electronic signatures
- RFC 5652 compliance
- Long-term validation with timestamps
- Certificate chain validation

**Key Classes**:
- `CMSSigningService`: CMS signature generation
- `TimestampAuthority`: RFC 3161 timestamping

**Example - CMS Signing**:
```cpp
#include "security/cms_signing.h"
#include "security/timestamp_authority.h"

// Load certificate and private key
auto cert = loadX509Certificate("/etc/themis/signing.crt");
auto pkey = loadPrivateKey("/etc/themis/signing.key");

CMSSigningService cms(cert, pkey);

// Sign document
std::vector<uint8_t> document = loadDocument("contract.pdf");
auto result = cms.sign(document, "signing-key");

if (result.success) {
    // result.signature contains CMS/PKCS#7 signature
    saveSignature(result.signature, "contract.pdf.p7s");
}

// Verify signature
bool valid = cms.verify(document, result.signature, "signing-key");
```

**Example - eIDAS Qualified Signature**:
```cpp
// Qualified signature requires timestamp
TSAConfig tsa_config;
tsa_config.url = "https://freetsa.org/tsr";
tsa_config.hash_algorithm = "SHA256";

TimestampAuthority tsa(tsa_config);

// Get timestamp for signature
auto timestamp = tsa.getTimestamp(result.signature);

if (timestamp.success) {
    // Attach timestamp to signature (long-term validation)
    result.timestamp_token = timestamp.tst_info;

    std::cout << "Timestamp: " << timestamp.timestamp_utc << std::endl;
    std::cout << "TSA: " << timestamp.tsa_name << std::endl;
}

// Verify timestamp
bool ts_valid = tsa.verifyTimestamp(result.signature, timestamp);
```

### 8. USB Admin Authenticator

**Purpose**: Hardware-based admin authentication via USB token

**Features**:
- YubiKey support
- FIDO2/WebAuthn
- Challenge-response authentication
- PIN protection
- Admin privilege elevation

**Key Classes**:
- `USBAdminAuthenticator`: USB token interface

**Example**:
```cpp
#include "security/usb_admin_authenticator.h"

USBAuthConfig config;
config.require_presence = true;  // Physical touch required
config.require_pin = true;

USBAdminAuthenticator auth(config);

// Enumerate connected tokens
auto tokens = auth.listTokens();
for (const auto& token : tokens) {
    std::cout << "Found: " << token.serial << std::endl;
}

// Authenticate admin
std::string challenge = generateChallenge();
auto response = auth.authenticate(tokens[0].serial, challenge);

if (response.success) {
    std::cout << "Admin authenticated" << std::endl;
    // Grant admin privileges
    grantAdminRole(response.user_id);
}
```

## Security Best Practices

### 1. Encryption

**Do's**:
- ✅ Use field-level encryption for PII and sensitive data
- ✅ Rotate keys regularly (e.g., every 90 days)
- ✅ Use HSM for production key storage
- ✅ Enable key versioning for rotation
- ✅ Implement key backup and recovery procedures

**Don'ts**:
- ❌ Don't store encryption keys in database
- ❌ Don't use same key for multiple data classes
- ❌ Don't disable authentication tags (GCM)
- ❌ Don't reuse IVs with same key

### 2. Access Control

**Do's**:
- ✅ Implement least privilege principle
- ✅ Use role hierarchy to simplify management
- ✅ Audit all permission checks
- ✅ Review role assignments regularly
- ✅ Implement time-based role grants (temporary elevation)

**Don'ts**:
- ❌ Don't grant wildcard permissions unnecessarily
- ❌ Don't share service accounts
- ❌ Don't bypass RBAC in admin interfaces

### 3. Key Management

**Do's**:
- ✅ Use separate keys for different data classifications
- ✅ Implement automated key rotation
- ✅ Monitor key usage and access patterns
- ✅ Use HSM for high-value keys
- ✅ Implement key escrow for recovery

**Don'ts**:
- ❌ Don't hard-code keys in application code
- ❌ Don't export keys unnecessarily
- ❌ Don't skip key rotation after security incidents

## Threat Model

### Threats Addressed

| Threat | Mitigation | Component |
|--------|------------|-----------|
| Data breach (at rest) | AES-256-GCM encryption | FieldEncryption |
| Key theft | HSM hardware protection | HSMProvider |
| Unauthorized access | RBAC with role hierarchy | RBAC |
| AQL injection | Pattern detection & AST analysis | AQLInjectionDetector |
| Malware in uploads | ClamAV scanning | MalwareScanner |
| VRAM data leakage | Secure clearing after use | VRAMSecureClear |
| Man-in-the-middle | TLS 1.3 with mTLS | TransportSecurity |
| Repudiation | CMS signing + timestamps | CMSSigningService |
| Privilege escalation | USB admin authenticator | USBAdminAuthenticator |

### Known Limitations

1. **Side-channel attacks**: AES-GCM implementation may be vulnerable to timing attacks
2. **Cold boot attacks**: Keys temporarily in RAM (mitigated by HSM)
3. **Quantum threats**: AES-256 is quantum-resistant, but RSA keys are not (plan migration to post-quantum)

## Compliance Considerations

### GDPR (General Data Protection Regulation)

**Relevant Features**:
- ✅ **Art. 32**: Field-level encryption for personal data
- ✅ **Art. 17**: Crypto-erasure for right to deletion (key deletion = data deletion)
- ✅ **Art. 25**: Privacy by design (encryption by default)
- ✅ **Art. 30**: Audit trails with timestamps

**Implementation**:
```cpp
// Crypto-erasure for GDPR right to deletion
void deleteUserData(const std::string& user_id) {
    std::string key_id = "user_" + user_id;

    // Delete user-specific encryption key
    key_provider->deleteKey(key_id);

    // Encrypted data is now cryptographically erased
    // (cannot be decrypted without key)
}
```

### SOC 2 Type II

**Relevant Controls**:
- ✅ **CC6.1**: Encryption of data at rest
- ✅ **CC6.6**: Role-based access control
- ✅ **CC6.7**: Key management procedures
- ✅ **CC7.2**: Audit logging with timestamps

### HIPAA (Health Insurance Portability and Accountability Act)

**Relevant Safeguards**:
- ✅ **§164.312(a)(2)(iv)**: Encryption of ePHI
- ✅ **§164.312(b)**: Audit controls (timestamp authority)
- ✅ **§164.312(c)(1)**: Integrity controls (CMS signing)
- ✅ **§164.308(a)(3)**: Role-based access

### eIDAS (Electronic Identification, Authentication and Trust Services)

**Relevant Requirements**:
- ✅ **Art. 24**: Qualified electronic signatures (CMS signing)
- ✅ **Art. 32**: Long-term validation (timestamp authority)
- ✅ **Annex I**: Advanced electronic signatures (CMS/PKCS#7)

**Implementation**:
```cpp
// eIDAS qualified signature
auto signature = cms.sign(document, "qualified-cert");
auto timestamp = tsa.getTimestamp(signature.signature);

// Store for long-term validation (>10 years)
QualifiedSignature qs;
qs.signature = signature.signature;
qs.timestamp = timestamp.tst_info;
qs.certificate_chain = getCertificateChain();
qs.validation_data = getOCSPResponses();

storeQualifiedSignature(qs);
```

## Performance Impact

### Encryption Overhead

| Operation | Baseline | With Encryption | Overhead |
|-----------|----------|-----------------|----------|
| Document insert | 1.2ms | 1.4ms | +16% |
| Document query | 0.8ms | 1.1ms | +37% |
| Bulk insert (1K docs) | 850ms | 1050ms | +23% |
| Index query | 0.3ms | 0.6ms | +100% |

### Key Provider Latency

| Provider | Operation | Latency | Notes |
|----------|-----------|---------|-------|
| Mock | Key retrieval | <1μs | In-memory only |
| Vault | Key retrieval (cached) | 100ns | 1-hour cache |
| Vault | Key retrieval (API) | 50-100ms | Network latency |
| HSM (PKCS#11) | Sign operation | 5-20ms | Hardware-dependent |
| PKI | Certificate validation | 10-50ms | OCSP/CRL checks |

### Optimization Strategies

1. **Key Caching**: Cache keys in memory (with expiration)
2. **Bulk Encryption**: Encrypt multiple fields in parallel
3. **Lazy Decryption**: Decrypt only requested fields
4. **Hardware Acceleration**: Use AES-NI CPU instructions
5. **Connection Pooling**: Reuse Vault connections

## Testing and Validation

### Unit Tests

```bash
# Run security module tests
cd build
ctest -R security

# Run specific test suites
./tests/security/encryption_test
./tests/security/rbac_test
./tests/security/key_rotation_test
```

## Integration Tests

```bash
# Test with real Vault instance
export VAULT_ADDR=http://localhost:8200
export VAULT_TOKEN=root
./tests/security/vault_integration_test

# Test with SoftHSM
export SOFTHSM2_CONF=/etc/softhsm2.conf
./tests/security/hsm_integration_test
```

## Security Auditing

```bash
# Run static analysis
clang-tidy src/security/*.cpp

# Check for hardcoded secrets
git secrets --scan

# Dependency vulnerability scan
trivy fs .
```

## Integration Examples

### With AQL Query Engine

```cpp
// Encrypt fields in AQL query results
auto results = aql_engine.execute(query);

for (auto& doc : results) {
    if (doc.contains("ssn")) {
        auto encrypted = encryption.encrypt("user_pii", doc["ssn"]);
        doc["ssn"] = encrypted.toBase64();
    }
}
```

### With REST API

```cpp
// API handler with RBAC
void handleDataQuery(const Request& req, Response& res) {
    // Check permissions
    if (!rbac.hasPermission(req.user, "data", "read")) {
        res.status(403).json({{"error", "Forbidden"}});
        return;
    }

    // Execute query
    auto results = db.query(req.body["query"]);

    // Decrypt sensitive fields
    decryptFields(results, req.user);

    res.json(results);
}
```

### With LoRA Adapter Loading

```cpp
// Scan LoRA adapter before loading
bool loadLoRAAdapter(const std::string& path) {
    // Malware scan
    auto scan_result = malware_scanner.scanFile(path);
    if (!scan_result.is_clean) {
        throw SecurityException("Malware detected: " + scan_result.threat_name);
    }

    // Manifest verification
    auto manifest = BinaryManifest::load(path + ".manifest");
    if (!manifest_signer.verify(manifest)) {
        throw SecurityException("Invalid manifest signature");
    }

    // Load adapter
    return lora_manager.loadAdapter(path);
}
```

## Troubleshooting

### Common Issues

**Issue**: `KeyNotFoundException: Key not found: user_pii v1`
- **Cause**: Key not created or Vault connection failed
- **Solution**: Check Vault connectivity, ensure key exists with `vault read transit/keys/user_pii`

**Issue**: `HSM initialization failed`
- **Cause**: PKCS#11 library not found or slot ID incorrect
- **Solution**: Verify `library_path`, check `pkcs11-tool --list-slots`

**Issue**: `Encryption too slow`
- **Cause**: Key cache disabled or Vault API rate limit
- **Solution**: Enable key caching, increase Vault token TTL

**Issue**: `Permission denied (RBAC)`
- **Cause**: User lacks required role or permission
- **Solution**: Check role assignments with `rbac.getUserRoles(user)`

## Files

### Headers (`include/security/`)
- `encryption.h` - Field-level encryption engine
- `key_provider.h` - Key management interface
- `vault_key_provider.h` - HashiCorp Vault integration
- `hsm_provider.h` - HSM/PKCS#11 interface
- `pki_key_provider.h` - PKI certificate-based keys
- `rbac.h` - Role-based access control
- `aql_injection_detector.h` - AQL injection detection
- `malware_scanner.h` - Malware scanning interface
- `binary_manifest.h` - Binary metadata and signatures
- `cms_signing.h` - CMS/PKCS#7 signing
- `timestamp_authority.h` - RFC 3161 timestamping
- `usb_admin_authenticator.h` - USB token authentication
- `vram_secure_clear.h` - GPU memory sanitization

### Implementation (`src/security/`)
- `field_encryption.cpp` - Encryption implementation
- `encrypted_field.cpp` - Document field wrapper
- `vault_key_provider.cpp` - Vault client
- `hsm_provider_pkcs11.cpp` - PKCS#11 implementation
- `pki_key_provider.cpp` - PKI integration
- `rbac.cpp` - RBAC engine
- `access_control.cpp` - Permission checking
- `aql_injection_detector.cpp` - Injection detection
- `malware_scanner.cpp` - ClamAV integration
- `binary_manifest.cpp` - Manifest handling
- `cms_signing.cpp` - CMS signing implementation
- `timestamp_authority_openssl.cpp` - OpenSSL/libcurl TSA client with hardened HTTPS transport
- `timestamp_authority.cpp` - TSA fallback bridge/stub path for non-OpenSSL builds
- `usb_admin_authenticator.cpp` - USB authentication
- `vram_secure_clear.cpp` - VRAM clearing

## See Also

- [ROADMAP.md](ROADMAP.md) - Security module roadmap and milestones
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned security features
- [Public Headers README](../../include/security/README.md) - Public API documentation
- [Security Documentation Overview](../../docs/de/security/README.md) - Detailed guides
- [Security Hardening Guide](../../docs/de/security/security_hardening.md)
- [Key Management Guide](../../docs/de/security/security_key_management.md)
- [Compliance Documentation](../../docs/de/security/security_compliance.md)

## Scientific References

1. Diffie, W., & Hellman, M. E. (1976). **New Directions in Cryptography**. *IEEE Transactions on Information Theory*, 22(6), 644–654. https://doi.org/10.1109/TIT.1976.1055638

2. National Institute of Standards and Technology. (2001). **Advanced Encryption Standard (AES)**. FIPS Publication 197. https://doi.org/10.6028/NIST.FIPS.197

3. Rivest, R. L., Shamir, A., & Adleman, L. (1978). **A Method for Obtaining Digital Signatures and Public-Key Cryptosystems**. *Communications of the ACM*, 21(2), 120–126. https://doi.org/10.1145/359340.359342

4. Saltzer, J. H., & Schroeder, M. D. (1975). **The Protection of Information in Computer Systems**. *Proceedings of the IEEE*, 63(9), 1278–1308. https://doi.org/10.1109/PROC.1975.9939

5. Barker, E., & Roginsky, A. (2019). **Transitioning the Use of Cryptographic Algorithms and Key Lengths**. NIST Special Publication 800-131A Rev. 2. https://doi.org/10.6028/NIST.SP.800-131Ar2

## Sourcecode Verification (Module: security/readme)

- Verified files:
    - `src/security/field_encryption.cpp`
    - `src/security/vault_key_provider.cpp`
    - `src/security/hsm_provider_pkcs11.cpp`
    - `src/security/pki_key_provider.cpp`
    - `src/security/access_control_manager.cpp`
    - `src/security/rbac.cpp`
    - `src/security/row_level_security.cpp`
    - `src/security/query_masking_policy.cpp`
    - `src/security/aql_injection_detector.cpp`
    - `src/security/security_evidence_collector.cpp`
- Verified surfaces:
    - encryption/key-provider behavior
    - access/policy enforcement paths
    - masking/detection/evidence paths
- Note:
    - Forward planning is tracked in `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`.
    - Historical implementation record remains in `CHANGELOG.md`.

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
