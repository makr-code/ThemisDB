# 🔒 Security Module - Main Overview

**Category:** 📋 Reports & Documentation  
**Version:** v1.5.1  
**Status:** ✅ Production Ready  
**Last Updated:** April 2026

---

## 📑 Table of Contents

- [📋 Overview](#-overview)
- [✨ Features & Highlights](#-features--highlights)
- [🚀 Quick Start](#-quick-start)
- [📖 Detailed Documentation](#-detailed-documentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 See Also](#-see-also)
- [📝 Changelog](#-changelog)

---

## 📋 Overview

The Security Module implements comprehensive security features for ThemisDB, including Field-Level Encryption, Key Management, RBAC, PKI Integration, Vector Encryption, and Malware Scanning.

### Main Components

| Component | Header | Source | Description |
|-----------|--------|--------|-------------|
| FieldEncryption | `encryption.h` | `encryption.cpp` | AES-256-GCM |
| KeyProvider | `key_provider.h` | - | Key Provider Interface |
| MockKeyProvider | `mock_key_provider.h` | `mock_key_provider.cpp` | Test Provider |
| VaultKeyProvider | `vault_key_provider.h` | `vault_key_provider.cpp` | HashiCorp Vault |
| HSMProvider | `hsm_provider.h` | `hsm_provider_pkcs11.cpp` | PKCS#11 HSM |
| PKIKeyProvider | `pki_key_provider.h` | `pki_key_provider.cpp` | PKI Integration |
| RBAC | `rbac.h` | `rbac.cpp` | Role-Based Access |
| MalwareScanner | `malware_scanner.h` | `malware_scanner.cpp` | Content Scanning |
| CMSSigning | `cms_signing.h` | `cms_signing.cpp` | CMS Signatures |
| **TimestampAuthority** | `timestamp_authority.h` | `timestamp_authority_openssl.cpp` | **RFC 3161 TSA (Production)** ✅ |
| **PIIRedactionPolicy** | `pii_redaction_policy.h` | `pii_redaction_policy.cpp` | **PII Redaction – Logs/Traces/Metrics** ✅ |

**Total:** 17 Headers, 17 Source Files, ~8,500 LOC

---

## ✨ Features & Highlights

### 🆕 PII Redaction Policy (v1.5.1) - PRODUCTION READY ✅

**Quick Start:**
- **[PII_REDACTION_POLICY.md](PII_REDACTION_POLICY.md)** - Full policy document (PII taxonomy, architecture, configuration, developer checklist)

**Features:**
- ✅ Automatic PII redaction enforced at the infrastructure level (no call-site changes needed)
- ✅ `PIIRedactingSink` wraps every spdlog sink — all `THEMIS_*` log macros auto-redact
- ✅ `Tracer::Span::setAttribute(string)` and `recordError()` auto-redact before OTel
- ✅ `MetricsCollector::makeKey()` auto-redacts label maps before Prometheus export
- ✅ Configurable per-pattern mode (`partial` / `strict`); global `THEMIS_PII_STRICT=1` override
- ✅ CI gate: `check_pii_leakage.py` fails the build on new unredacted PII writes
- ✅ AFL++ fuzz harness with 7-seed corpus and PII dictionary
- ✅ 18 unit tests covering all three telemetry channels

**PII Categories Covered:**

| Category | Example | Redaction |
|----------|---------|-----------|
| Email | `alice@example.com` | Partial |
| Phone | `+49-123-456789` | Partial |
| SSN | `123-45-6789` | Strict |
| Credit Card | `4242-4242-4242-4242` | Strict |
| IBAN | `DE89370400440532013000` | Strict |
| IP Address | `192.168.1.42` | Partial |

**Architecture:**
```
Application Code  →  Logger / Tracer::Span / MetricsCollector
                          ↓ (automatic)
              PIIRedactionPolicy singleton
                          ↓
                    PIIDetector pipeline
                    (RegexEngine + optional NER/Embedding)
                          ↓
       spdlog sinks / OTel SDK / Prometheus endpoint
```

**Configuration:**
```yaml
# config/pii_patterns.yaml
detection_engines:
  - type: regex
    patterns:
      - name: email
        pattern: "[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z]{2,}"
        mode: partial
```

**Example:**
```cpp
// Automatic – no changes needed for new code using Logger/Tracer/MetricsCollector:
THEMIS_INFO("Invoice sent to {}", recipient);  // 'recipient' auto-redacted by PIIRedactingSink

// Explicit pre-redaction for bare spdlog:: calls before Logger::init():
auto safe = PIIRedactionPolicy::get().redactForLog(raw_value);
spdlog::info("Value: {}", safe);

// Strict mode (environment variable or runtime):
// THEMIS_PII_STRICT=1  or  PIIRedactionPolicy::get().setStrictMode(true);
```

### 🆕 RFC 3161 Timestamp Authority (v1.5.0) - PRODUCTION READY ✅

**Quick Start:**
- **[TSA_SETUP.md](TSA_SETUP.md)** - Comprehensive TSA setup guide (400+ lines)

**Features:**
- ✅ Full RFC 3161 Time-Stamp Protocol implementation
- ✅ OpenSSL-based cryptographic operations (SHA-256, SHA-384, SHA-512)
- ✅ Integration with external TSA providers (FreeTSA, DigiCert, Sectigo)
- ✅ eIDAS compliance support for qualified electronic timestamps
- ✅ Long-term validation (LTV) for 30-year timestamp retention
- ✅ Certificate chain validation and verification
- ✅ 10+ comprehensive tests

**Use Cases:**
- eIDAS qualified electronic timestamps (Art. 42)
- Long-term signature validation (30 years)
- Audit trail compliance (DSGVO Art. 30)
- Non-repudiation and legal proof of timing
- Document timestamping for regulated industries

**Configuration:**
```yaml
timestamp_authority:
  enabled: true
  url: "https://freetsa.org/tsr"
  hash_algorithm: "SHA256"
  verify_tsa_cert: true
```

**Example:**
```cpp
TSAConfig config;
config.url = "https://freetsa.org/tsr";
TimestampAuthority tsa(config);
auto token = tsa.getTimestamp(data);
```

### 🆕 Vector Encryption (Phase 1 + 2) - FULLY IMPLEMENTED ✅

**Quick Start:**
- **[QUICK_START_VECTOR_ENCRYPTION.md](../../de/security/QUICK_START_VECTOR_ENCRYPTION.md)** - 5-minute quick start guide

**User Manuals:**
- **[VECTOR_ENCRYPTION_CONFIGURATION.md](../../de/security/VECTOR_ENCRYPTION_CONFIGURATION.md)** - Phase 1: Vector encryption in RocksDB
- **[HNSW_ENCRYPTION_CONFIGURATION.md](../../de/security/HNSW_ENCRYPTION_CONFIGURATION.md)** - Phase 2: HNSW index encryption

**Implementation Details:**
- **[COMPLETE_IMPLEMENTATION_SUMMARY.md](../../de/security/COMPLETE_IMPLEMENTATION_SUMMARY.md)** - Complete overview of all phases
- **[PHASE1_FINAL_REPORT.md](../../de/security/PHASE1_FINAL_REPORT.md)** - Phase 1 final report
- **[PHASE2_IMPLEMENTATION_REPORT.md](../../de/security/PHASE2_IMPLEMENTATION_REPORT.md)** - Phase 2 implementation report

**Build & Test:**
- **[BUILD_VERIFICATION_GUIDE.md](../../de/security/BUILD_VERIFICATION_GUIDE.md)** - Build and test guide

**Performance & Optimization:**
- **[PERFORMANCE_OPTIMIZATION_NOTES.md](../../de/security/PERFORMANCE_OPTIMIZATION_NOTES.md)** - Performance optimizations

**Analysis:**
- **[HNSW_PERSISTENCE_ENCRYPTION_ANALYSIS.md](../../de/security/HNSW_PERSISTENCE_ENCRYPTION_ANALYSIS.md)** - HNSW persistence security analysis
- **[EMBEDDING_REVERSIBILITY_ANALYSIS.md](../../de/security/EMBEDDING_REVERSIBILITY_ANALYSIS.md)** - Vector embedding security analysis
- **[ENCRYPTED_HNSW_SEARCHABILITY.md](../../de/security/ENCRYPTED_HNSW_SEARCHABILITY.md)** - Encrypted search analysis

**Results:**
- ✅ 100% At-Rest encryption for vectors
- ✅ AES-256-GCM for RocksDB vectors and HNSW index files
- ✅ Full compliance with BSI C5 CRY-03
- ✅ 8 integration tests + 5 examples
- ✅ Migration tool for existing data

### 🏆 BSI C5 Compliance - Cryptography

**[➡️ BSI C5 Column Encryption Compliance Report](../../de/security/BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md)**  
Comprehensive analysis of column encryption implementation against BSI C5 requirements (CRY-01 to CRY-06).  
**Compliance Score: 95% → 100% (with new documentation)** ✅

**[➡️ BSI C5 Multi-Model Encryption Analysis](../../de/security/BSI_C5_MULTI_MODEL_ENCRYPTION_ANALYSIS.md)** ⭐ **NEW**  
Detailed analysis of encryption across **all data model layers**: Relational, Vector, Graph, Geo, Timeline, Process.  
**Result: Unified Storage Architecture ensures consistent encryption across all models** ✅

**Formal Documentation (December 2025):**
- **[Cryptography Policy](../../de/security/CRYPTOGRAPHY_POLICY.md)** - Formal policy per BSI C5 CRY-01, BSI TR-02102-1 compliant
- **[Key Lifecycle Management](../../de/security/KEY_LIFECYCLE_MANAGEMENT.md)** - Complete key lifecycle per BSI C5 CRY-02
- **[Executive Summary (EN)](../../de/security/BSI_C5_EXECUTIVE_SUMMARY.md)** - Brief summary for stakeholders

### 🛡️ Implementation Status (December 2025 / February 2026)

| Component | Status | Implementation |
|-----------|--------|----------------|
| **RBAC/ABAC Policy Engine** | ✅ Production Ready | Ranger-compatible |
| **Apache Ranger Integration** | ✅ Production Ready | `src/server/ranger_adapter.cpp` |
| **RFC 3161 Timestamp Authority** | ✅ Production Ready (v1.5.0) | `src/security/timestamp_authority_openssl.cpp` |
| **PII Redaction Policy** | ✅ Production Ready (v1.5.1) | `src/security/pii_redaction_policy.cpp` |
| **VaultKeyProvider (KMS)** | ✅ Production Ready | `src/security/vault_key_provider.cpp` |
| **HSMProvider (PKCS#11)** | ✅ Production Ready | `src/security/hsm_provider_pkcs11.cpp` |
| **PKI Client (OpenSSL)** | ✅ Production Ready | `src/utils/pki_client.cpp` |
| **Audit Logging** | ✅ Production Ready | Hash-Chain, PKI signatures |
| **Field Encryption** | ✅ Production Ready | AES-256-GCM |
| **Timestamp Authority** | ✅ Production Ready | RFC 3161 via OpenSSL |

---

## 🚀 Quick Start

### Enable Vector Encryption

```cpp
// Initialize encryption
auto key_provider = std::make_shared<KeyProvider>();
auto field_encryption = std::make_shared<FieldEncryption>(key_provider);
EncryptedField<std::vector<float>>::setFieldEncryption(field_encryption);

// Enable encryption
VectorIndexManager vim(db);
vim.init("documents", 768);
vim.setVectorEncryptionEnabled(true);

// Add vectors - automatically encrypted!
BaseEntity entity("doc1");
entity.setField("embedding", std::vector<float>(768, 0.5f));
vim.addEntity(entity);
```

### Using Field Encryption

```cpp
// Encrypt sensitive field
FieldEncryption encryption(keyProvider);
auto blob = encryption.encrypt("sensitive data", "user_pii");
entity.setField("email_encrypted", blob.toBase64());

// Decrypt
auto decrypted = encryption.decrypt(EncryptedBlob::fromBase64(encrypted_value));
```

### Configure RBAC

```cpp
RBAC rbac;
rbac.createRole("analyst", {{"data", "read"}, {"reports", "read"}});
rbac.assignRole("user@example.com", "analyst");

// Check permission
if (rbac.authorize("user@example.com", "data", "read")) {
    // Access granted
}
```

---

## 📖 Detailed Documentation

### 🔒 Encryption

For detailed encryption documentation:
- **[at_rest_encryption_research.md](at_rest_encryption_research.md)** ⭐ - At-Rest Encryption Research (AWS, Google Cloud, Azure)
- [security_encryption_strategy.md](../../de/security/security_encryption_strategy.md) - Encryption strategy
- [security_column_encryption.md](../../de/security/security_column_encryption.md) - Column encryption
- [security_encryption_deployment.md](../../de/security/security_encryption_deployment.md) - Deployment guide
- [BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md](../../de/security/BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md) - BSI C5 compliance

### 🔑 Key Management

- [security_key_management.md](../../de/security/security_key_management.md) - Key management overview
- [security_key_rotation.md](../../de/security/security_key_rotation.md) - Key rotation
- [security_hsm.md](../../de/security/security_hsm.md) - HSM integration
- [KEY_LIFECYCLE_MANAGEMENT.md](../../de/security/KEY_LIFECYCLE_MANAGEMENT.md) - Key lifecycle management

### 🛡️ Security Operations

- [security_hardening.md](../../de/security/security_hardening.md) - Hardening guide
- [security_hardware_attack_vectors.md](security_hardware_attack_vectors.md) - Hardware attack vectors (USB, PCIe, CPU, RAM, IO)
- [security_audit_checklist.md](../../de/security/security_audit_checklist.md) - Audit checklist
- [security_incident_response.md](../../de/security/security_incident_response.md) - Incident response
- [security_threat_model.md](../../de/security/security_threat_model.md) - Threat model

### 📜 Compliance & Policies

- [security_compliance.md](../../de/security/security_compliance.md) - Compliance overview
- [security_policies.md](../../de/security/security_policies.md) - Security policies
- [CRYPTOGRAPHY_POLICY.md](../../de/security/CRYPTOGRAPHY_POLICY.md) - Cryptography policy

> **Note:** Most security documentation is currently available in German. English translations are in progress. 
> For the most up-to-date information, please refer to the [German documentation](../../de/security/).

---

## 💡 Best Practices

### 1. Always Use TLS/mTLS

```cpp
// Enable TLS for all connections
ServerConfig config;
config.tls_enabled = true;
config.tls_cert_path = "/path/to/cert.pem";
config.tls_key_path = "/path/to/key.pem";
```

### 2. Rotate Keys Regularly

```cpp
// Implement key rotation schedule
KeyRotationScheduler scheduler;
scheduler.setRotationInterval(std::chrono::days(90));
scheduler.scheduleRotation("data_encryption_key");
```

### 3. Use HSM for Production

```cpp
// Use HSM instead of mock provider in production
auto hsm = std::make_shared<HSMProvider>(pkcs11_library, slot_id);
auto encryption = std::make_shared<FieldEncryption>(hsm);
```

### 4. Enable Audit Logging

```cpp
// Log all security-relevant events
AuditLogger logger;
logger.log(AuditEvent::ACCESS_GRANTED, user_id, resource_id);
logger.log(AuditEvent::ENCRYPTION_PERFORMED, key_id, data_size);
```

### 5. Regular Security Audits

- Perform quarterly security audits
- Review access logs monthly
- Test incident response procedures
- Keep security documentation up to date

---

## 🔧 Troubleshooting

### Encryption Errors

**Symptom:** "Failed to encrypt data"

**Solutions:**
```cpp
// Check key provider is initialized
if (!keyProvider->isInitialized()) {
    keyProvider->initialize();
}

// Verify key exists
if (!keyProvider->hasKey("encryption_key")) {
    keyProvider->generateKey("encryption_key", 256);
}
```

### RBAC Permission Denied

**Symptom:** "Access denied" for valid users

**Solutions:**
```cpp
// Verify role assignment
auto roles = rbac.getUserRoles(user_id);
for (const auto& role : roles) {
    LOG(INFO) << "User has role: " << role;
}

// Check resource permissions
auto permissions = rbac.getRolePermissions(role_name);
```

### HSM Connection Failed

**Symptom:** "Cannot connect to HSM"

**Solutions:**
```bash
# Check PKCS#11 library path
ldd /usr/lib/libpkcs11.so

# Verify slot availability
pkcs11-tool --list-slots

# Test token access
pkcs11-tool --login --test
```

---

## 📚 See Also

- **[Build Verification Guide](../../de/security/BUILD_VERIFICATION_GUIDE.md)** - How to build and test security features
- **[BSI C5 Executive Summary](../../de/security/BSI_C5_EXECUTIVE_SUMMARY.md)** - Compliance overview
- **[Security Sprint Summary](../../de/security/security_sprint_summary.md)** - Development history
- **[Main Documentation](../README.md)** - ThemisDB documentation index

---

## 📝 Changelog

### v1.5.1 (February 2026)
- ✅ **PII Redaction Policy** – `PIIRedactionPolicy` singleton with automatic enforcement
- ✅ `PIIRedactingSink` – every spdlog sink auto-redacts via `Logger::init()`
- ✅ `Tracer::Span::setAttribute(string)` / `recordError()` auto-redact PII
- ✅ `MetricsCollector::makeKey()` auto-redacts Prometheus label values
- ✅ CI gate: `check_pii_leakage.py` fails on new unredacted PII writes
- ✅ AFL++ fuzz harness (`pii_redaction`) with corpus seeds and dictionary
- ✅ Documentation: [PII_REDACTION_POLICY.md](PII_REDACTION_POLICY.md)

### v1.3.0 (December 2025)
- ✅ Vector encryption fully implemented (Phase 1 + 2)
- ✅ BSI C5 compliance documentation complete
- ✅ Key lifecycle management documented
- ✅ Cryptography policy formalized
- ✅ Multi-model encryption analysis added

### v1.2.0 (November 2025)
- ✅ RBAC implementation complete
- ✅ Apache Ranger integration added
- ✅ Audit logging with hash-chains
- ✅ PKI client implemented

### v1.1.0 (October 2025)
- ✅ Field-level encryption
- ✅ Vault key provider
- ✅ HSM support via PKCS#11

---

**For questions or issues, please refer to:**
- [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- [Security Policy](../../../SECURITY.md)
- [Documentation](https://makr-code.github.io/ThemisDB/)
