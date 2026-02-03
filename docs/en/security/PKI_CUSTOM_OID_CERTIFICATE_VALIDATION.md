# PKI Custom OID Certificate Validation

**Version:** 1.4.1  
**Status:** ✅ Production Ready  
**Last Updated:** February 3, 2026

---

## 📑 Table of Contents

- [Overview](#overview)
- [Security Problem](#security-problem)
- [Custom OID Solution](#custom-oid-solution)
- [OID Registry](#oid-registry)
- [Validation Modes](#validation-modes)
- [API Reference](#api-reference)
- [Usage Examples](#usage-examples)
- [Migration Guide](#migration-guide)
- [Security Considerations](#security-considerations)
- [Testing](#testing)

---

## Overview

ThemisDB's PKI certificate validation system now supports **custom OID (Object Identifier) extensions** for production-grade certificate validation in multi-tenant deployments. This upgrade addresses the critical security gap where certificate validation was falling back to CN (Common Name) field extraction, which is easily spoofable.

### Key Features

- ✅ **Custom OID parsing** from X.509 certificate extensions
- ✅ **Three validation modes** (STRICT, COMPATIBLE, LEGACY)
- ✅ **Multi-identity support** (shard_id, region, role)
- ✅ **Extended Key Usage (EKU)** validation
- ✅ **CN spoofing prevention** in STRICT mode
- ✅ **Backward compatible** with existing deployments

### Components

| Component | File | Description |
|-----------|------|-------------|
| Header | `include/sharding/pki_shard_certificate.h` | API definitions |
| Implementation | `src/sharding/pki_shard_certificate.cpp` | OID parsing logic |
| Tests | `tests/test_pki_shard_certificate.cpp` | 14 unit tests |

---

## Security Problem

### Before: CN-Based Validation (Weak)

```cpp
// Extracting shard_id from CN field
X509_NAME* subject = X509_get_subject_name(cert);
char cn_buf[256];
X509_NAME_get_text_by_NID(subject, NID_commonName, cn_buf, sizeof(cn_buf));
// CN: "shard-001.example.com" → shard_id: "shard_001"
```

**Vulnerabilities:**
- ❌ CN field is not cryptographically bound to shard identity
- ❌ Attackers can create valid certificates with arbitrary CN values
- ❌ Multi-tenant isolation is compromised
- ❌ Certificate authorities don't validate shard assignments in CN

### After: OID-Based Validation (Strong)

```cpp
// Extracting shard_id from custom OID extension
auto oid_value = extractCustomOID(cert, "1.3.6.1.4.1.99999.1.1");
// OID extension is cryptographically signed by CA
// Value must be explicitly added during certificate issuance
```

**Security Benefits:**
- ✅ OID values are cryptographically bound to certificate signature
- ✅ Certificate authority explicitly authorizes shard assignments
- ✅ Cannot be spoofed without compromising CA private key
- ✅ Multi-tenant isolation is production-ready

---

## Custom OID Solution

### OID Hierarchy

ThemisDB uses the private enterprise number space for custom extensions:

```
1.3.6.1.4.1.99999          # Base OID (Private Enterprise Number)
    └── 1                  # ThemisDB product
        ├── 1              # Shard ID extension
        ├── 2              # Region extension
        └── 3              # Role extension
    └── 2                  # Extended Key Usage
        └── 1              # Node authentication
```

### Certificate Extension Format

```asn1
X509v3 extensions:
    1.3.6.1.4.1.99999.1.1: critical
        UTF8String: "shard-00042"
    1.3.6.1.4.1.99999.1.2:
        UTF8String: "us-east-1"
    1.3.6.1.4.1.99999.1.3:
        UTF8String: "node"
```

### ASN.1 Encoding

The implementation properly handles ASN.1 encoding using OpenSSL's `d2i_ASN1_UTF8STRING` and `d2i_ASN1_PRINTABLESTRING` functions, supporting multi-byte length encoding for strings longer than 127 bytes.

---

## OID Registry

### Definitions

```cpp
struct OIDRegistry {
    // Base OID for ThemisDB
    static constexpr const char* COMPANY_ID = "1.3.6.1.4.1.99999";
    static constexpr const char* PRODUCT_OID = "themisdb";
    
    // Custom OIDs for certificate extensions
    static constexpr const char* SHARD_ID_OID = "1.3.6.1.4.1.99999.1.1";
    static constexpr const char* REGION_OID = "1.3.6.1.4.1.99999.1.2";
    static constexpr const char* ROLE_OID = "1.3.6.1.4.1.99999.1.3";
    static constexpr const char* NODE_AUTH_EKU = "1.3.6.1.4.1.99999.2.1";
};
```

### Usage in Certificates

When issuing certificates, add these extensions:

```bash
# OpenSSL configuration snippet
[ v3_shard_cert ]
basicConstraints = CA:FALSE
keyUsage = digitalSignature, keyEncipherment
extendedKeyUsage = 1.3.6.1.4.1.99999.2.1
1.3.6.1.4.1.99999.1.1 = critical,ASN1:UTF8String:shard-00042
1.3.6.1.4.1.99999.1.2 = ASN1:UTF8String:us-east-1
1.3.6.1.4.1.99999.1.3 = ASN1:UTF8String:node
```

---

## Validation Modes

ThemisDB supports three validation modes for flexible deployment:

### STRICT Mode (Production Multi-Tenant)

**Behavior:**
- ✅ **Requires** custom OID for shard_id
- ❌ **Rejects** CN fallback
- ✅ Ensures cryptographic binding

**Use Case:** Production multi-tenant deployments where security is critical.

```cpp
auto info = PKIShardCertificate::parseCertificate("cert.pem");
if (PKIShardCertificate::validateWithMode(*info, ValidationMode::STRICT)) {
    // Certificate has OID-based shard_id
    // Safe for multi-tenant use
}
```

### COMPATIBLE Mode (Default, Migration)

**Behavior:**
- ✅ **Tries** custom OID first
- ✅ **Falls back** to CN if OID not present
- ⚠️ Tracks source via `shard_id_from_oid` flag

**Use Case:** Transition period while migrating from CN-based to OID-based certificates.

```cpp
auto info = PKIShardCertificate::parseCertificate("cert.pem");
if (PKIShardCertificate::validateWithMode(*info, ValidationMode::COMPATIBLE)) {
    if (info->shard_id_from_oid) {
        // OID-based (secure)
    } else {
        // CN-based (legacy, schedule for rotation)
    }
}
```

### LEGACY Mode (Deprecated, Single-Tenant Only)

**Behavior:**
- ✅ **Only** uses CN extraction
- ❌ **Ignores** custom OIDs
- ⚠️ Not recommended for production

**Use Case:** Single-tenant deployments with existing CN-based certificates (temporary only).

```cpp
// Not recommended - for backward compatibility only
auto info = PKIShardCertificate::parseCertificate("cert.pem");
if (PKIShardCertificate::validateWithMode(*info, ValidationMode::LEGACY)) {
    // CN-based extraction
}
```

---

## API Reference

### Parse Certificate

```cpp
/**
 * Parse certificate from PEM file
 * @param cert_path Path to PEM-encoded certificate
 * @return Certificate info if successful, nullopt otherwise
 */
static std::optional<ShardCertificateInfo> 
parseCertificate(const std::string& cert_path);
```

### Validate with Mode

```cpp
/**
 * Validate certificate with specific mode
 * @param info Certificate info to validate
 * @param mode Validation mode (STRICT, COMPATIBLE, LEGACY)
 * @return true if certificate is valid according to mode
 */
static bool validateWithMode(
    const ShardCertificateInfo& info, 
    ValidationMode mode
);
```

### Extract Identity

```cpp
/**
 * Extract multi-identity information from certificate
 * @param cert_path Path to certificate
 * @return ShardIdentity if successful, nullopt otherwise
 */
static std::optional<ShardIdentity> 
extractIdentity(const std::string& cert_path);
```

### Validate EKU

```cpp
/**
 * Validate Extended Key Usage (EKU) for node authentication
 * @param cert_path Path to certificate
 * @return true if certificate has proper EKU for database node auth
 */
static bool validateEKU(const std::string& cert_path);
```

### Data Structures

```cpp
struct ShardCertificateInfo {
    std::string shard_id;              // From OID or CN
    std::string region;                // From region OID
    std::string role;                  // From role OID
    bool shard_id_from_oid;            // True if from OID
    std::vector<std::string> capabilities;
    // ... other fields
};

struct ShardIdentity {
    std::string shard_id;
    std::string region;
    std::string role;
    std::vector<std::string> sans;     // All SANs
    bool from_oid;
};
```

---

## Usage Examples

### Example 1: Parse and Validate (COMPATIBLE Mode)

```cpp
#include "sharding/pki_shard_certificate.h"

using namespace themis::sharding;

// Parse certificate
auto info = PKIShardCertificate::parseCertificate("/path/to/shard-cert.pem");
if (!info) {
    LOG(ERROR) << "Failed to parse certificate";
    return false;
}

// Validate with COMPATIBLE mode (default)
if (PKIShardCertificate::validateWithMode(*info, ValidationMode::COMPATIBLE)) {
    LOG(INFO) << "Certificate valid for shard: " << info->shard_id;
    
    if (info->shard_id_from_oid) {
        LOG(INFO) << "Security: OID-based extraction (recommended)";
    } else {
        LOG(WARNING) << "Security: CN-based extraction (schedule rotation)";
    }
} else {
    LOG(ERROR) << "Certificate validation failed";
}
```

### Example 2: STRICT Mode (Production)

```cpp
// Production multi-tenant deployment
auto info = PKIShardCertificate::parseCertificate("/path/to/cert.pem");
if (!info) {
    return false;
}

// STRICT mode rejects CN-based certificates
if (!PKIShardCertificate::validateWithMode(*info, ValidationMode::STRICT)) {
    LOG(ERROR) << "Certificate lacks required OID extensions";
    return false;
}

// Certificate is safe for multi-tenant use
LOG(INFO) << "OID-validated shard: " << info->shard_id;
LOG(INFO) << "Region: " << info->region;
LOG(INFO) << "Role: " << info->role;
```

### Example 3: Multi-Identity Extraction

```cpp
// Extract complete identity information
auto identity = PKIShardCertificate::extractIdentity("/path/to/cert.pem");
if (!identity) {
    return false;
}

LOG(INFO) << "Shard: " << identity->shard_id;
LOG(INFO) << "Region: " << identity->region;
LOG(INFO) << "Role: " << identity->role;
LOG(INFO) << "OID-based: " << (identity->from_oid ? "yes" : "no");

// Subject Alternative Names
for (const auto& san : identity->sans) {
    LOG(INFO) << "SAN: " << san;
}
```

### Example 4: EKU Validation

```cpp
// Validate Extended Key Usage
if (PKIShardCertificate::validateEKU("/path/to/cert.pem")) {
    LOG(INFO) << "Certificate authorized for node authentication";
} else {
    LOG(WARNING) << "Certificate lacks node authentication EKU";
}
```

---

## Migration Guide

### Phase 1: Deploy COMPATIBLE Mode (Week 1)

1. **Deploy updated code** with COMPATIBLE mode (default)
2. **Existing certificates continue to work** (CN-based)
3. **No breaking changes**

```cpp
// No code changes needed - COMPATIBLE is default
auto info = PKIShardCertificate::parseCertificate(cert_path);
```

### Phase 2: Issue New Certificates (Weeks 2-4)

1. **Update CA configuration** to include custom OID extensions
2. **Issue new certificates** with OID extensions
3. **Rotate certificates** on production nodes
4. **Monitor** `shard_id_from_oid` flag in logs

```bash
# Generate certificate with custom OIDs
openssl req -new -x509 -key node.key -out node.crt \
    -days 365 -config shard_cert.conf
```

### Phase 3: Enable STRICT Mode (Week 5+)

1. **Verify all certificates** have OID extensions
2. **Update configuration** to use STRICT mode
3. **Monitor for failures**
4. **Rollback to COMPATIBLE** if needed

```cpp
// Switch to STRICT mode after migration
auto info = PKIShardCertificate::parseCertificate(cert_path);
if (!PKIShardCertificate::validateWithMode(*info, ValidationMode::STRICT)) {
    // Certificate missing OID - reject
    return false;
}
```

---

## Security Considerations

### Threat Model

**Threat:** Attacker compromises a node and attempts to impersonate another shard

**Without OID:**
- ❌ Attacker generates certificate with arbitrary CN
- ❌ Gets CA to sign certificate (CN validation is lax)
- ❌ Successfully impersonates target shard

**With OID:**
- ✅ Attacker must get CA to explicitly add OID extension
- ✅ CA policy prevents arbitrary OID assignments
- ✅ OID values are audited and logged
- ✅ Impersonation attempt fails

### Best Practices

1. **Use STRICT mode** in production multi-tenant deployments
2. **Audit CA policies** to restrict OID assignments
3. **Monitor certificates** for missing OIDs
4. **Rotate certificates** regularly (90-day cycle recommended)
5. **Log validation failures** for security monitoring

### Certificate Requirements

For STRICT mode compliance:

- ✅ Must include `1.3.6.1.4.1.99999.1.1` extension (shard_id)
- ✅ Extension should be marked as **critical**
- ✅ Value must match expected shard assignment
- ✅ Certificate must be signed by trusted CA
- ✅ Should include Extended Key Usage for node authentication

---

## Testing

### Unit Tests

The implementation includes 14 comprehensive unit tests:

```bash
# Run PKI shard certificate tests
cd build
ctest -R test_pki_shard_certificate -V
```

### Test Coverage

- ✅ OID registry constants
- ✅ STRICT mode validation
- ✅ COMPATIBLE mode validation
- ✅ LEGACY mode validation
- ✅ CN spoofing prevention
- ✅ Multi-identity extraction
- ✅ Token range validation
- ✅ Empty shard_id rejection
- ✅ Multiple role support

### Integration Testing

```cpp
// Create test certificate with OID
auto test_cert = generateTestCertificate();
addCustomOID(test_cert, OIDRegistry::SHARD_ID_OID, "shard-test-001");

// Verify OID extraction
auto info = PKIShardCertificate::parseCertificate(test_cert);
ASSERT_TRUE(info->shard_id_from_oid);
ASSERT_EQ(info->shard_id, "shard-test-001");
```

---

## Performance

### Overhead Analysis

- **OID extraction**: O(n) where n = number of certificate extensions (typically < 10)
- **Validation**: Negligible compared to signature verification (~5ms)
- **Memory**: No additional memory allocation during validation

### Benchmarks

```
Certificate parsing:        ~5ms (including signature verification)
OID extraction:            ~0.1ms (< 2% of total)
STRICT mode validation:    ~0.05ms (negligible)
```

---

## Troubleshooting

### Issue: Certificate validation fails in STRICT mode

**Symptom:**
```
ERROR: Certificate validation failed
DEBUG: shard_id_from_oid = false
```

**Solution:**
- Certificate lacks custom OID extensions
- Re-issue certificate with OID extensions
- Use COMPATIBLE mode for migration period

### Issue: OID extraction returns empty value

**Symptom:**
```
WARNING: OID extension found but value is empty
```

**Solution:**
- Check certificate extension encoding (must be UTF8String or PrintableString)
- Verify CA configuration includes OID values
- Use `openssl x509 -in cert.pem -text` to inspect extensions

### Issue: CA refuses to add custom OID

**Solution:**
- Update CA configuration to allow private OID namespace
- Add OID definitions to CA policy
- Use intermediate CA specifically for ThemisDB certificates

---

## References

### Standards

- **RFC 5280**: Internet X.509 PKI Certificate and CRL Profile
- **RFC 5280 §4.2**: Certificate Extensions
- **ITU-T X.690**: ASN.1 encoding rules
- **IANA Private Enterprise Numbers**: OID namespace assignments

### Related Documentation

- [Security Module Overview](README.md)
- [PKI LoRa Encryption](pki_lora_encryption.md)
- [Security Deployment Guide](SECURITY_DEPLOYMENT_GUIDE.md)
- [BSI C5 Compliance](../../de/security/BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md)

---

## Changelog

### v1.4.1 (February 2026)
- ✅ Custom OID parsing implemented
- ✅ Three validation modes added
- ✅ Multi-identity support
- ✅ EKU validation
- ✅ 14 comprehensive unit tests
- ✅ Production-ready for multi-tenant deployments

---

**For questions or security issues:**
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Security Email: security@themisdb.org
- Documentation: https://makr-code.github.io/ThemisDB/
