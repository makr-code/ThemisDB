> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Security Module Headers

Public API headers for ThemisDB security features.

## Overview

This directory contains the public interface for ThemisDB's security module, providing:
- Encryption and key management APIs
- Access control and authorization
- Cryptographic signing and verification
- Hardware security module (HSM) integration
- Compliance and audit features

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                     Security Public API                      │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌────────────────┐  ┌────────────────┐  ┌───────────────┐ │
│  │   Encryption   │  │  Key Provider  │  │     RBAC      │ │
│  │   (AES-GCM)    │  │  (Vault/HSM)   │  │   (AuthZ)     │ │
│  └────────────────┘  └────────────────┘  └───────────────┘ │
│                                                              │
│  ┌────────────────┐  ┌────────────────┐  ┌───────────────┐ │
│  │    Signing     │  │   Malware      │  │   Injection   │ │
│  │  (CMS/eIDAS)   │  │   Scanner      │  │   Detection   │ │
│  └────────────────┘  └────────────────┘  └───────────────┘ │
│                                                              │
│  ┌────────────────┐  ┌────────────────┐  ┌───────────────┐ │
│  │  Timestamp     │  │  USB Admin     │  │  VRAM Clear   │ │
│  │  Authority     │  │  Authenticator │  │   (GPU)       │ │
│  └────────────────┘  └────────────────┘  └───────────────┘ │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

## Core Headers

### Encryption & Key Management

#### `encryption.h`
**Purpose**: Field-level AES-256-GCM encryption

**Key Classes**:
- `FieldEncryption`: Main encryption engine
- `EncryptedBlob`: Encrypted data with metadata
- `EncryptionException` / `DecryptionException`: Error handling

**Example**:
```cpp
#include "security/encryption.h"

FieldEncryption encryption(key_provider);

// Encrypt sensitive field
std::string ssn = "123-45-6789";
auto encrypted = encryption.encrypt("user_pii", ssn);

// Serialize for storage
std::string stored = encrypted.toBase64();

// Decrypt
auto blob = EncryptedBlob::fromBase64(stored);
std::string decrypted = encryption.decrypt(blob);
```

**Serialization Format**:
```
{key_id}:{version}:{base64(iv)}:{base64(ciphertext)}:{base64(tag)}
```

**Thread Safety**: Thread-safe (internal locking)

---

#### `key_provider.h`
**Purpose**: Abstract interface for key management backends

**Key Classes**:
- `KeyProvider`: Base interface for key storage
- `KeyMetadata`: Key version, status, timestamps
- `KeyStatus`: ACTIVE, ROTATING, DEPRECATED, DELETED

**Implementations**:
- `VaultKeyProvider`: HashiCorp Vault
- `HSMKeyProviderAdapter`: Hardware Security Modules
- `PKIKeyProvider`: Certificate-based keys
- `MockKeyProvider`: Testing only

**Example**:
```cpp
#include "security/key_provider.h"

class CustomKeyProvider : public KeyProvider {
public:
    std::vector<uint8_t> getKey(const std::string& key_id,
                                uint32_t version) override {
        // Fetch from custom backend
    }

    void createKey(const KeyMetadata& metadata) override {
        // Generate new key
    }

    void rotateKey(const std::string& key_id) override {
        // Create new version
    }
};
```

**Key Lifecycle**:
```
CREATE → ACTIVE → ROTATING → DEPRECATED → DELETED
```

---

#### `vault_key_provider.h`
**Purpose**: HashiCorp Vault integration via Transit engine

**Key Classes**:
- `VaultKeyProvider`: Vault client implementation
- `VaultConfig`: Connection configuration

**Features**:
- Transit secrets engine support
- Automatic token renewal
- Key caching (configurable TTL)
- Namespace support
- TLS with client certificates

**Example**:
```cpp
#include "security/vault_key_provider.h"

VaultConfig config;
config.url = "https://vault.example.com:8200";
config.token = "s.your-vault-token";
config.mount_path = "transit";
config.namespace_path = "production";
config.tls_ca_cert = "/etc/themis/ca.crt";

auto provider = std::make_shared<VaultKeyProvider>(config);

// Create encryption key in Vault
KeyMetadata meta;
meta.key_id = "user_pii";
meta.algorithm = "aes256-gcm96";
provider->createKey(meta);

// Rotate key (creates v2)
provider->rotateKey("user_pii");
```

**Vault API Endpoints**:
- `POST /v1/transit/keys/{name}` - Create key
- `GET /v1/transit/keys/{name}` - Read key metadata
- `POST /v1/transit/rotate/{name}` - Rotate key
- `POST /v1/transit/encrypt/{name}` - Encrypt (optional)

---

#### `hsm_provider.h`
**Purpose**: Hardware Security Module interface (PKCS#11)

**Key Classes**:
- `HSMProvider`: PKCS#11 wrapper
- `HSMConfig`: Library path, slot, PIN

**Supported HSMs**:
- Thales/SafeNet Luna HSM
- Utimaco CryptoServer
- AWS CloudHSM
- SoftHSM2 (testing)

**Example**:
```cpp
#include "security/hsm_provider.h"

HSMConfig config;
config.library_path = "/usr/lib/softhsm/libsofthsm2.so";
config.slot_id = 0;
config.pin = "1234";

auto hsm = std::make_shared<HSMProvider>(config);
if (!hsm->initialize()) {
    throw std::runtime_error("HSM init failed");
}

// Generate key in HSM (never exported)
auto key_handle = hsm->generateKey("payment_key", 256);

// Sign with HSM key
auto signature = hsm->sign(data, key_handle);
```

**PKCS#11 Functions Used**:
- `C_Initialize` / `C_Finalize`
- `C_OpenSession` / `C_CloseSession`
- `C_Login` / `C_Logout`
- `C_GenerateKey` / `C_GenerateKeyPair`
- `C_Sign` / `C_Verify`
- `C_Encrypt` / `C_Decrypt`

---

#### `hsm_key_provider_adapter.h`
**Purpose**: Adapt HSMProvider to KeyProvider interface

**Example**:
```cpp
#include "security/hsm_key_provider_adapter.h"

auto hsm = std::make_shared<HSMProvider>(config);
auto provider = std::make_shared<HSMKeyProviderAdapter>(hsm);

// Use as regular KeyProvider
auto key = provider->getKey("my_key", 1);
```

---

#### `pki_key_provider.h`
**Purpose**: Derive encryption keys from X.509 certificates

**Key Classes**:
- `PKIKeyProvider`: Certificate-based key derivation
- `PKIConfig`: Certificate paths, CA bundle

**Example**:
```cpp
#include "security/pki_key_provider.h"

PKIConfig config;
config.cert_path = "/etc/themis/certs/encryption.crt";
config.key_path = "/etc/themis/certs/encryption.key";
config.ca_bundle_path = "/etc/themis/certs/ca-bundle.crt";

auto provider = std::make_shared<PKIKeyProvider>(config);

// Derive key from certificate
auto key = provider->getKey("default", 1);
```

---

### Access Control

#### `rbac.h`
**Purpose**: Role-Based Access Control with hierarchy

**Key Classes**:
- `RBAC`: Authorization engine
- `Role`: Collection of permissions
- `Permission`: Resource:Action pair

**Example**:
```cpp
#include "security/rbac.h"

RBACConfig config;
config.config_path = "/etc/themis/rbac.json";

RBAC rbac(config);

// Define role
Role admin;
admin.name = "admin";
admin.permissions = {{"*", "*"}};  // All resources

rbac.addRole(admin);
rbac.assignRole("alice@example.com", "admin");

// Check permission
bool allowed = rbac.hasPermission("alice@example.com", "data", "write");
```

**Permission Syntax**:
- `resource:action` - Specific permission
- `*:action` - All resources, specific action
- `resource:*` - Specific resource, all actions
- `*:*` - All permissions (superuser)

**Role Inheritance**:
```json
{
  "name": "poweruser",
  "inherits": ["readonly", "operator"],
  "permissions": [...]
}
```

---

#### `access_control.h`
**Purpose**: Low-level access control primitives

**Key Classes**:
- `AccessControlManager`: Policy enforcement
- `AccessPolicy`: Policy rules
- `AccessControlList`: ACL management

**Example**:
```cpp
#include "security/access_control.h"

AccessControlManager acm;

// Set ACL
ACL acl;
acl.owner = "alice@example.com";
acl.entries = {
    {"bob@example.com", {Permission::READ}},
    {"charlie@example.com", {Permission::READ, Permission::WRITE}}
};

acm.setACL("/data/sensitive", acl);

// Check access
bool can_read = acm.checkAccess("bob@example.com", "/data/sensitive",
                                 Permission::READ);
```

---

#### `access_control_manager.h`
**Purpose**: High-level access control orchestration

**Combines**:
- RBAC (role-based)
- ACLs (resource-based)
- Attributes (ABAC)
- Time-based constraints

---

### Threat Detection

#### `aql_injection_detector.h`
**Purpose**: Detect AQL injection attacks

**Key Classes**:
- `AQLInjectionDetector`: Pattern-based detection

**Detection Methods**:
- SQL injection patterns
- Command injection
- Path traversal
- Script injection
- Complexity limits

**Example**:
```cpp
#include "security/aql_injection_detector.h"

AQLInjectionDetector detector;

// Check query
std::string query = "FOR u IN users FILTER u.id == @id RETURN u";
nlohmann::json params = {{"id", user_input}};

if (!detector.isSafe(query, params)) {
    throw SecurityException("Injection detected");
}
```

**Patterns Detected**:
- `' OR '1'='1`
- `'; DROP TABLE`
- `../../../etc/passwd`
- `<script>alert(1)</script>`
- Excessive nesting depth

---

#### `malware_scanner.h`
**Purpose**: Malware scanning for uploaded files

**Key Classes**:
- `MalwareScanner`: ClamAV integration
- `ScanResult`: Scan outcome and threat info

**Example**:
```cpp
#include "security/malware_scanner.h"

MalwareScannerConfig config;
config.clamd_host = "localhost";
config.clamd_port = 3310;

MalwareScanner scanner(config);

auto result = scanner.scanFile("/uploads/file.bin");
if (!result.is_clean) {
    std::cout << "Threat: " << result.threat_name << std::endl;
    scanner.quarantine("/uploads/file.bin");
}
```

**Supported Backends**:
- ClamAV (via clamd)
- VirusTotal API
- Windows Defender (on Windows)

---

### Signing & Timestamping

#### `cms_signing.h`
**Purpose**: CMS/PKCS#7 digital signatures

**Key Classes**:
- `CMSSigningService`: Signature generation/verification

**Standards**:
- RFC 5652: Cryptographic Message Syntax
- PKCS#7: Cryptographic Message Syntax
- eIDAS: Qualified electronic signatures

**Example**:
```cpp
#include "security/cms_signing.h"

auto cert = loadX509Cert("/etc/themis/signing.crt");
auto pkey = loadPrivateKey("/etc/themis/signing.key");

CMSSigningService cms(cert, pkey);

auto result = cms.sign(document, "signing-key");
bool valid = cms.verify(document, result.signature, "signing-key");
```

---

#### `signing.h`
**Purpose**: Abstract signing interface

**Key Classes**:
- `SigningService`: Base interface
- `SigningResult`: Signature and metadata

**Implementations**:
- `CMSSigningService`: CMS/PKCS#7
- `VaultSigningProvider`: Vault Transit
- `KeyproviderSigning`: Generic provider

---

#### `signing_provider.h`
**Purpose**: Pluggable signing backend interface

---

#### `timestamp_authority.h`
**Purpose**: RFC 3161 Time-Stamp Protocol client

**Key Classes**:
- `TimestampAuthority`: TSP client
- `TSAConfig`: TSA server configuration
- `TimestampToken`: Timestamp response

**Example**:
```cpp
#include "security/timestamp_authority.h"

TSAConfig config;
config.url = "https://freetsa.org/tsr";
config.hash_algorithm = "SHA256";

TimestampAuthority tsa(config);

auto token = tsa.getTimestamp(data);
std::cout << "Timestamp: " << token.timestamp_utc << std::endl;

bool valid = tsa.verifyTimestamp(data, token);
```

**Use Cases**:
- eIDAS qualified signatures (long-term validation)
- Audit log timestamping
- Document notarization
- SAGA transaction ordering

---

#### `manifest_signer.h`
**Purpose**: Binary manifest signing and verification

**Key Classes**:
- `ManifestSigner`: Sign/verify manifests
- `BinaryManifest`: Binary metadata

**Example**:
```cpp
#include "security/manifest_signer.h"
#include "security/binary_manifest.h"

BinaryManifest manifest;
manifest.name = "lora-adapter";
manifest.hash = computeSHA256(binary);
manifest.version = "1.0.0";

ManifestSigner signer(cert, pkey);
auto signature = signer.sign(manifest);

bool valid = signer.verify(manifest);
```

---

### Hardware & Platform Security

#### `usb_admin_authenticator.h`
**Purpose**: USB token authentication (YubiKey, FIDO2)

**Key Classes**:
- `USBAdminAuthenticator`: Token interface
- `USBAuthConfig`: Configuration

**Supported Tokens**:
- YubiKey 5 Series
- Nitrokey
- SoloKeys
- Titan Security Key

**Example**:
```cpp
#include "security/usb_admin_authenticator.h"

USBAuthConfig config;
config.require_presence = true;  // Touch required
config.require_pin = true;

USBAdminAuthenticator auth(config);

auto tokens = auth.listTokens();
std::string challenge = generateChallenge();

auto response = auth.authenticate(tokens[0].serial, challenge);
if (response.success) {
    grantAdminAccess(response.user_id);
}
```

**Protocols**:
- FIDO2/WebAuthn
- FIDO U2F
- OATH HOTP/TOTP
- PIV (smart card)

---

#### `vram_secure_clear.h`
**Purpose**: Secure GPU VRAM clearing

**Key Classes**:
- `VRAMSecureClear`: VRAM sanitization

**Methods**:
- Zero-fill (single pass)
- Multi-pass overwrite (DOD 5220.22-M)
- Verification

**Example**:
```cpp
#include "security/vram_secure_clear.h"

VRAMSecureClear vram_clear;

void* vram = cudaMalloc(size);
// ... use VRAM ...

vram_clear.secureErase(vram, size);
cudaFree(vram);
```

**Standards**:
- NIST SP 800-88: Media sanitization
- DOD 5220.22-M: Clearing and sanitization

---

### Supporting Headers

#### `binary_manifest.h`
**Purpose**: Binary file metadata and validation

**Key Classes**:
- `BinaryManifest`: Metadata structure

**Fields**:
- `name`, `version`, `author`
- `hash`, `hash_algorithm`
- `size_bytes`, `timestamp`
- `signature`, `certificate_chain`

---

#### `crypto_capabilities.h`
**Purpose**: Query available cryptographic capabilities

**Example**:
```cpp
#include "security/crypto_capabilities.h"

CryptoCapabilities caps;
if (caps.hasAESNI()) {
    // Use hardware AES
}
if (caps.hasHSM()) {
    // Use HSM for keys
}
```

---

#### `pkcs11_minimal.h`
**Purpose**: Minimal PKCS#11 definitions (no external dependency)

---

#### `pkcs11_wrapper.h`
**Purpose**: RAII C++ wrapper interface over the raw PKCS#11 C API

**Key classes and helpers** (in namespace `themis::security::pkcs11`):
- `Pkcs11Library`: RAII loader for a PKCS#11 shared library (dlopen / LoadLibrary).
  Calls `C_GetFunctionList` and `C_Initialize` on `load()`; calls `C_Finalize` and
  unloads the library in its destructor.
- `Pkcs11Session`: RAII guard for a PKCS#11 session.  Opens a session via
  `C_OpenSession` and closes / logs out in its destructor.
- `Pkcs11Category` / `makePkcs11Error()`: Maps `CK_RV` codes to `std::error_code`
  for integration with standard C++ error handling.
- `ckrvToString()`: Human-readable string for any `CK_RV` return value.
- `AttributeFilter`: Convenience struct for building `CK_ATTRIBUTE` search templates.
- Free helper functions: `listSlots`, `findObjects`, `findObjectsByLabel`,
  `signData`, `verifyData`, `encryptData`, `decryptData`,
  `generateRsaKeyPair`, `getAttribute`, `getAttributeBytes`.

**Design principles**:
- Header-only (no additional `.cpp` required).
- No exceptions thrown internally; all operations return `bool`,
  `std::optional`, or empty containers.
- Non-copyable; movable (both `Pkcs11Library` and `Pkcs11Session`).
- No hidden state: callers interact directly with `CK_FUNCTION_LIST_PTR`
  and `CK_OBJECT_HANDLE` values.

**Example**:
```cpp
#include "security/pkcs11_wrapper.h"
using namespace themis::security::pkcs11;

Pkcs11Library lib;
if (!lib.load("/usr/lib/softhsm/libsofthsm2.so")) {
    throw std::runtime_error(lib.lastError());
}

auto slots = listSlots(lib.functions());

Pkcs11Session session(lib.functions());
session.open(slots[0]);
session.login(CKU_USER, "1234");

auto privKeys = findObjectsByLabel(session, "my-key", CKO_PRIVATE_KEY);
auto sig = signData(session, privKeys[0], CKM_SHA256_RSA_PKCS, data);
```

**Thread safety**: `Pkcs11Library` is not thread-safe after construction.
`Pkcs11Session` objects must not be shared across threads.

---

#### `vcc_pki_client.h`
**Purpose**: VCC PKI client for certificate operations

---

#### `user_registration_plugin.h`
**Purpose**: User registration plugin interface

---

#### `vault_signing_provider.h`
**Purpose**: Vault-backed signing provider

---

#### `hsm_security_checker.h`
**Purpose**: HSM security policy enforcement

---

#### `hsm_security_metrics.h`
**Purpose**: HSM performance and security metrics

---

#### `transport_security_checker.h`
**Purpose**: TLS/mTLS validation and enforcement

---

## Usage Patterns

### Pattern 1: Field-Level Encryption
```cpp
#include "security/encryption.h"
#include "security/vault_key_provider.h"

// Setup
auto provider = std::make_shared<VaultKeyProvider>(vault_config);
FieldEncryption encryption(provider);

// Encrypt document fields
nlohmann::json doc = {
    {"name", "John Doe"},
    {"ssn", "123-45-6789"}  // Sensitive
};

auto encrypted_ssn = encryption.encrypt("user_pii", doc["ssn"]);
doc["ssn"] = encrypted_ssn.toBase64();

// Store encrypted document
db.insert(doc);

// Decrypt when needed
auto stored_doc = db.findOne({{"name", "John Doe"}});
auto encrypted_blob = EncryptedBlob::fromBase64(stored_doc["ssn"]);
std::string ssn = encryption.decrypt(encrypted_blob);
```

### Pattern 2: RBAC Authorization
```cpp
#include "security/rbac.h"

// Setup
RBAC rbac(config);
rbac.loadConfig("/etc/themis/rbac.json");

// API handler
void handleRequest(const Request& req, Response& res) {
    // Check permission
    if (!rbac.hasPermission(req.user, "data", "read")) {
        res.status(403).json({{"error", "Forbidden"}});
        return;
    }

    // Process request
    auto data = db.query(req.body["query"]);
    res.json(data);
}
```

### Pattern 3: Key Rotation
```cpp
#include "security/encryption.h"
#include "security/key_provider.h"

// Rotate key
encryption.rotateKey("user_pii");

// Re-encrypt old data (background job)
auto old_docs = db.find({{"ssn", {"$exists", true}}});

for (auto& doc : old_docs) {
    auto blob = EncryptedBlob::fromBase64(doc["ssn"]);

    // Check if old version
    if (blob.key_version < current_version) {
        // Re-encrypt with new key
        auto decrypted = encryption.decrypt(blob);
        auto re_encrypted = encryption.encrypt("user_pii", decrypted);

        doc["ssn"] = re_encrypted.toBase64();
        db.update(doc);
    }
}
```

### Pattern 4: HSM-Backed Signing
```cpp
#include "security/hsm_provider.h"
#include "security/cms_signing.h"

// Initialize HSM
auto hsm = std::make_shared<HSMProvider>(hsm_config);
hsm->initialize();

// Sign document
auto signature = hsm->sign(document, "signing-key-label");

// Verify signature
bool valid = hsm->verify(document, signature, "signing-key-label");
```

### Pattern 5: Malware Scanning
```cpp
#include "security/malware_scanner.h"
#include "security/binary_manifest.h"

// Scan uploaded file
auto scan_result = scanner.scanFile(upload_path);
if (!scan_result.is_clean) {
    scanner.quarantine(upload_path);
    throw SecurityException("Malware detected: " + scan_result.threat_name);
}

// Verify manifest
auto manifest = BinaryManifest::load(upload_path + ".manifest");
if (!manifest_signer.verify(manifest)) {
    throw SecurityException("Invalid manifest");
}

// Load file
loadBinary(upload_path);
```

## Best Practices

### DO's ✅
1. **Always use parameterized queries** with AQLInjectionDetector
2. **Rotate keys regularly** (e.g., every 90 days)
3. **Use HSM for production** key storage
4. **Enable key versioning** for rotation
5. **Implement RBAC** for all data access
6. **Scan all uploads** before processing
7. **Timestamp critical operations** for audit trails
8. **Use TLS 1.3** for all network communication
9. **Clear VRAM** after processing sensitive data
10. **Log all security events** with timestamps

### DON'Ts ❌
1. **Don't store keys in database** or application code
2. **Don't reuse IVs** with the same key
3. **Don't disable authentication tags** (GCM)
4. **Don't bypass RBAC** in admin interfaces
5. **Don't hard-code credentials** or tokens
6. **Don't skip key rotation** after incidents
7. **Don't trust user input** (always validate)
8. **Don't log sensitive data** (encrypt logs)
9. **Don't export HSM keys** unnecessarily
10. **Don't use weak algorithms** (MD5, SHA1, 3DES)

## Security Considerations

### Threat Model
- **Data at Rest**: Encrypted with AES-256-GCM
- **Data in Transit**: TLS 1.3 with mTLS
- **Key Storage**: HSM or Vault (hardware-backed)
- **Access Control**: RBAC with audit logging
- **Code Injection**: AQL pattern detection
- **Malware**: ClamAV scanning
- **VRAM Leakage**: Secure clearing
- **Repudiation**: CMS signing + timestamps

### Known Limitations
1. **Side-channel attacks**: Timing analysis possible (mitigate with constant-time ops)
2. **Cold boot attacks**: Keys in RAM (mitigate with HSM)
3. **Quantum threats**: RSA vulnerable (plan post-quantum migration)
4. **Social engineering**: Multi-factor authentication helps but not foolproof

## Compliance Mapping

| Standard | Features | Headers |
|----------|----------|---------|
| GDPR Art. 32 | Encryption at rest | `encryption.h` |
| GDPR Art. 17 | Crypto-erasure (key deletion) | `key_provider.h` |
| SOC 2 CC6.1 | Encryption controls | `encryption.h` |
| SOC 2 CC6.6 | Access control | `rbac.h` |
| HIPAA §164.312(a) | ePHI encryption | `encryption.h` |
| HIPAA §164.312(b) | Audit controls | `timestamp_authority.h` |
| eIDAS Art. 24 | Qualified signatures | `cms_signing.h` |
| eIDAS Art. 32 | Long-term validation | `timestamp_authority.h` |
| PCI DSS 3.4 | Key management | `key_provider.h`, `hsm_provider.h` |
| FIPS 140-3 | Cryptographic modules | `hsm_provider.h` |

## Performance Characteristics

| Operation | Latency | Notes |
|-----------|---------|-------|
| AES-256-GCM encrypt | 5-10μs | 256-byte payload |
| AES-256-GCM decrypt | 3-7μs | 256-byte payload |
| Key cache lookup | <100ns | In-memory |
| Vault API call | 50-100ms | Network latency |
| HSM sign | 5-20ms | Hardware-dependent |
| RBAC check | 1-5μs | In-memory |
| AQL injection check | 10-50μs | Pattern matching |
| Malware scan | 100-500ms | File size dependent |

## Thread Safety

| Header | Thread Safety | Notes |
|--------|---------------|-------|
| `encryption.h` | ✅ Thread-safe | Internal locking |
| `key_provider.h` | ✅ Thread-safe | Depends on implementation |
| `vault_key_provider.h` | ✅ Thread-safe | Connection pooling |
| `hsm_provider.h` | ⚠️ Depends | PKCS#11 session-dependent |
| `rbac.h` | ✅ Thread-safe | Read-write locks |
| `aql_injection_detector.h` | ✅ Thread-safe | Stateless |
| `malware_scanner.h` | ✅ Thread-safe | Socket per thread |

## Testing

### Unit Tests
```bash
cd build
ctest -R security_headers
```

### Integration Tests
```bash
# With Vault
export VAULT_ADDR=http://localhost:8200
./tests/security/vault_integration_test

# With HSM
export SOFTHSM2_CONF=/etc/softhsm2.conf
./tests/security/hsm_integration_test
```

## Code Coverage
```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Coverage ..
make coverage
```

## See Also

- [Implementation README](../../src/security/README.md)
- [Future Enhancements](FUTURE_ENHANCEMENTS.md)
- [Security Best Practices Guide](../../docs/security-best-practices.md)
- [Key Management Guide](../../docs/key-management.md)
- [Compliance Documentation](../../docs/compliance/)

## References

### Standards
- [RFC 5652](https://tools.ietf.org/html/rfc5652) - Cryptographic Message Syntax
- [RFC 3161](https://tools.ietf.org/html/rfc3161) - Time-Stamp Protocol
- [NIST SP 800-38D](https://csrc.nist.gov/publications/detail/sp/800-38d/final) - GCM Mode
- [FIPS 197](https://csrc.nist.gov/publications/detail/fips/197/final) - AES Standard

### Libraries
- OpenSSL 3.0+
- HashiCorp Vault
- ClamAV
- SoftHSM2

### Documentation
- [PKCS#11 Specification](http://docs.oasis-open.org/pkcs11/pkcs11-base/v2.40/)
- [eIDAS Regulation](https://eur-lex.europa.eu/legal-content/EN/TXT/?uri=CELEX:32014R0910)
- [GDPR](https://gdpr-info.eu/)
- [PCI DSS](https://www.pcisecuritystandards.org/)

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
