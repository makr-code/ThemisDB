# Encryption Strategy

**Document Version**: 1.1  
**Last Updated**: 2026-04-06  
**Status**: Active  
**Owner**: Security Team

> **NEW:** See [At-Rest Encryption Research](../en/security/at_rest_encryption_research.md) for comprehensive analysis of hyperscaler encryption mechanisms (AWS, Google Cloud, Azure) and recommendations for filesystem/hardware-level encryption.

---

## 📋 Overview

This document defines ThemisDB's encryption strategy for protecting data at rest and in transit, ensuring compliance with security standards (GDPR, eIDAS, ISO 27001).

## 🎯 Objectives

1. **Confidentiality**: Protect sensitive data from unauthorized access
2. **Integrity**: Ensure data has not been tampered with
3. **Compliance**: Meet regulatory requirements (GDPR Art. 32, eIDAS)
4. **Performance**: Minimize encryption overhead while maintaining security

---

## 🔐 Encryption Architecture

### Data at Rest Encryption

#### 1. Field-Level Encryption

**Implementation**: `src/security/field_encryption.cpp`

- **Algorithm**: AES-256-GCM (Galois/Counter Mode)
- **Key Size**: 256 bits
- **Mode**: Authenticated encryption with associated data (AEAD)
- **Use Cases**: 
  - Personally Identifiable Information (PII)
  - Financial data
  - Health records
  - Sensitive business data

**Features**:
- Schema-driven encryption (configured per field)
- Automatic encryption on write
- Automatic decryption on read
- Support for field-level access control

**Example Configuration**:
```json
{
  "collection": "users",
  "fields": {
    "ssn": {"encrypt": true, "algorithm": "AES-256-GCM"},
    "email": {"encrypt": true, "algorithm": "AES-256-GCM"},
    "creditCard": {"encrypt": true, "algorithm": "AES-256-GCM"}
  }
}
```

#### 2. Content Blob Encryption

**Implementation**: `src/storage/blob_storage.cpp`, `src/utils/zstd_codec.cpp`

- **Strategy**: Encrypt-then-compress
- **Algorithm**: AES-256-GCM
- **Compression**: ZSTD (after encryption for non-encrypted blobs)
- **Use Cases**:
  - Large binary objects (BLOBs)
  - Document attachments
  - Media files

**Storage Savings**: ~50% with ZSTD compression on non-encrypted content

#### 3. Vector Metadata Encryption

**Implementation**: `src/index/vector_index.cpp`

- **Vector Embeddings**: NEVER encrypted (breaks similarity search)
- **Metadata**: Schema-driven encryption
- **Associated Data**: Encrypted based on schema configuration

**Important**: The embedding field itself is never encrypted to preserve mathematical properties required for cosine similarity and other distance metrics.

#### 4. Storage-Level Encryption (Filesystem/Hardware)

**See**: [At-Rest Encryption Research](../en/security/at_rest_encryption_research.md) for detailed analysis

ThemisDB supports multiple layers of at-rest encryption beyond application-level field encryption:

**Cloud Deployments:**
- **AWS EBS Encryption**: Transparent volume encryption with AWS KMS (AES-256-XTS)
- **Azure Disk Encryption**: BitLocker/dm-crypt integration with Azure Key Vault
- **GCP Default Encryption**: Automatic encryption for all data (AES-256, Titan chip)

**On-Premise Deployments:**
- **dm-crypt/LUKS**: Linux kernel-level encryption (AES-256-XTS with AES-NI)
- **fscrypt**: Per-directory encryption for ext4/f2fs filesystems
- **Self-Encrypting Drives (SED)**: Hardware-based disk encryption (TCG Opal 2.0)

**Recommendation**: Use managed cloud encryption or dm-crypt/LUKS for comprehensive at-rest protection without custom implementation overhead.

#### 5. Audit Log Encryption

**Implementation**: `src/utils/saga_logger.cpp`

- **Pattern**: Encrypt-then-Sign
- **Encryption**: AES-256-GCM via FieldEncryption
- **Signature**: PKI-based digital signatures via PKIClient
- **Compliance**: GDPR Art. 32, eIDAS-compliant
- **Integrity**: Cryptographic signatures prevent tampering

---

### Data in Transit Encryption

#### 1. TLS/SSL

**Minimum Version**: TLS 1.2 (TLS 1.3 preferred)

**Cipher Suites** (in order of preference):
1. `TLS_AES_256_GCM_SHA384` (TLS 1.3)
2. `TLS_CHACHA20_POLY1305_SHA256` (TLS 1.3)
3. `TLS_AES_128_GCM_SHA256` (TLS 1.3)
4. `TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384` (TLS 1.2)
5. `TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256` (TLS 1.2)

**Certificate Requirements**:
- Minimum 2048-bit RSA or 256-bit ECC keys
- Valid certificate chain from trusted CA
- Certificate rotation every 12 months (recommended)

#### 2. gRPC Encryption

**Implementation**: `plugins/rpc/grpc/grpc_plugin.cpp`

- **Protocol**: gRPC with TLS
- **Mutual TLS**: Supported for client authentication
- **Use Cases**: Service-to-service communication, shard-to-shard RPC

---

## 🔑 Key Management

See [ENCRYPTION_KEY_MANAGEMENT_POLICY.md](ENCRYPTION_KEY_MANAGEMENT_POLICY.md) for detailed key management procedures.

### Key Hierarchy

```
Master Key (HSM or KMS)
    ├─ Data Encryption Keys (DEK)
    │   ├─ Field Encryption Keys
    │   ├─ Blob Encryption Keys
    │   └─ Audit Log Encryption Keys
    └─ Key Encryption Keys (KEK)
        └─ Encrypted DEKs at rest
```

### Key Storage

1. **Production**: Hardware Security Module (HSM) or Cloud KMS
2. **Development**: Key provider interface with file-based keys (NOT FOR PRODUCTION)
3. **Testing**: Mock key provider

**Implementation**: `src/security/key_provider.h`, `include/security/mock_key_provider.h`

### Key Rotation

**Implementation**: `src/security/field_encryption.cpp`

- **Method**: Lazy re-encryption via `decryptAndReEncrypt()`
- **Benefits**: Zero-downtime key rotation
- **Process**: 
  1. New key version deployed
  2. Reads decrypt with old key, re-encrypt with new key on write
  3. Background job can batch-update remaining data
  4. Old keys retained for decryption until all data migrated

**Rotation Schedule**:
- **Data Encryption Keys (DEK)**: Annually or after security incident
- **Key Encryption Keys (KEK)**: Every 2 years
- **Master Keys**: Every 3 years or as required by policy

---

## 📊 Encryption Metrics

**Prometheus Integration**: `include/security/field_encryption.h`

### Available Metrics

1. **Operation Counters**:
   - `encryption_operations_total`
   - `decryption_operations_total`
   - `reencryption_operations_total`

2. **Error Counters**:
   - `encryption_errors_total`
   - `decryption_errors_total`

3. **Performance Metrics**:
   - `encryption_duration_seconds` (histogram)
   - `decryption_duration_seconds` (histogram)

4. **Data Metrics**:
   - `encrypted_bytes_total`
   - `decrypted_bytes_total`

**Grafana Dashboard**: See `grafana/dashboards/encryption_metrics.json`

**Documentation**: `docs/encryption_metrics.md`

---

## 🔒 Compliance Mapping

### GDPR (General Data Protection Regulation)

**Article 32**: Security of processing
- ✅ Encryption of personal data at rest (field-level encryption)
- ✅ Encryption of personal data in transit (TLS 1.2+)
- ✅ Regular testing and evaluation (automated tests, metrics)
- ✅ Pseudonymization capabilities (via encryption)

**Article 33**: Notification of personal data breach
- ✅ Audit logging with encryption and signatures
- ✅ Tamper-evident logs for breach detection

### eIDAS (Electronic Identification and Trust Services)

- ✅ Qualified electronic signatures (PKI integration)
- ✅ Long-term signature validation
- ✅ Timestamp authority integration
- ✅ Certificate validation and revocation checking

**Implementation**: `src/security/pki_client.cpp`

**Documentation**: `docs/pki_integration_architecture.md`, `docs/pki_signatures.md`

### ISO 27001

**Control A.10.1.1**: Cryptographic controls
- ✅ Policy on the use of cryptographic controls (this document)
- ✅ Key management procedures (see ENCRYPTION_KEY_MANAGEMENT_POLICY.md)
- ✅ Implementation standards (AES-256-GCM, TLS 1.2+)

---

## 🛠️ Implementation Guidelines

### For Developers

#### Encrypting New Fields

1. **Define Schema**:
```cpp
FieldEncryptionSchema schema;
schema.collection = "users";
schema.fields["sensitive_data"] = {
    .algorithm = "AES-256-GCM",
    .key_id = "data-encryption-key-v1"
};
```

2. **Apply Encryption**:
```cpp
FieldEncryption encryptor(key_provider);
std::string encrypted = encryptor.encrypt(plaintext, schema);
```

3. **Automatic Decryption on Read**:
```cpp
std::string decrypted = encryptor.decrypt(encrypted, schema);
```

#### Best Practices

1. **Never encrypt vector embeddings** - breaks similarity search
2. **Use schema-driven encryption** - centralized configuration
3. **Encrypt-then-sign** for audit logs and legal documents
4. **Monitor encryption metrics** - detect performance issues
5. **Test key rotation** - ensure lazy re-encryption works
6. **Use HSM/KMS in production** - never store keys in code

### For Operations

#### Deployment Checklist

- [ ] HSM or KMS configured and accessible
- [ ] TLS certificates installed and valid
- [ ] Encryption schemas configured for all sensitive collections
- [ ] Prometheus metrics endpoint configured
- [ ] Grafana dashboards deployed
- [ ] Key rotation schedule documented
- [ ] Backup encryption keys stored securely offsite
- [ ] Disaster recovery tested with encrypted data

---

## 🔍 Testing

### Unit Tests

- `tests/test_field_encryption.cpp` - Field encryption/decryption
- `tests/test_schema_encryption.cpp` - Schema-based encryption E2E
- `tests/test_vector_metadata_encryption_edge_cases.cpp` - Vector metadata
- `tests/test_lazy_reencryption.cpp` - Key rotation

### Integration Tests

- Encryption with RocksDB storage
- TLS in gRPC communication
- PKI signature verification
- Audit log encryption end-to-end

### Performance Tests

- Encryption throughput benchmarks
- Impact on query latency
- Key rotation performance

---

## 📚 Related Documentation

- **[At-Rest Encryption Research](../en/security/at_rest_encryption_research.md)** - Comprehensive analysis of hyperscaler encryption mechanisms
- [ENCRYPTION_KEY_MANAGEMENT_POLICY.md](ENCRYPTION_KEY_MANAGEMENT_POLICY.md) - Key management procedures
- [INFORMATION_SECURITY_POLICY.md](INFORMATION_SECURITY_POLICY.md) - Overall security framework
- `docs/encryption_metrics.md` - Prometheus metrics guide
- `docs/pki_integration_architecture.md` - PKI architecture
- `docs/pki_signatures.md` - Digital signature operations

---

## 📝 Change History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-11 | Security Team | Initial encryption strategy document |

---

## ✅ Approval

**Reviewed by**: Security Team Lead  
**Approved by**: CTO  
**Date**: 2026-01-11  
**Next Review**: 2027-01-11
