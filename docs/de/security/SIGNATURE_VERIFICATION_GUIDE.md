# RSA-SHA256 and ECDSA Signature Verification - Usage Guide

## Overview

The signature verification system provides production-ready cryptographic verification for LoRA adapters and model weights using OpenSSL. It implements RSA-SHA256, ECDSA-SHA256, and ECDSA-SHA384 signature verification with X.509 certificate validation.

## Architecture

The system uses the **Chain of Responsibility** and **Builder** patterns:

```
┌─────────────────────────────────────────────────────────┐
│              SignatureVerifierBuilder                   │
│  (Fluent interface for building verification chains)   │
└─────────────────────┬───────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────┐
│            ISignatureVerifier (Base Class)              │
└──┬─────────┬──────────────┬────────────┬────────────┬───┘
   │         │              │            │            │
   ▼         ▼              ▼            ▼            ▼
┌──────┐ ┌────────┐ ┌──────────────┐ ┌───────┐ ┌──────────┐
│ RSA  │ │ ECDSA  │ │Certificate   │ │ CRL   │ │CRL       │
│ SHA  │→│ SHA256 │→│Chain         │→│Check  │ │Checker   │
│ 256  │ │        │ │Verifier      │ │       │ └──────────┘
└──────┘ │ECDSA   │ └──────────────┘ └───────┘
         │ SHA384 │
         └────────┘
```

## Quick Start

### Basic RSA-SHA256 Verification

```cpp
#include "llm/security/signature_verifier.h"

using namespace themis::llm::security;

// Create verifier
RSA_SHA256_Verifier verifier;

// Load data and signature
std::vector<uint8_t> data = /* your data */;
std::vector<uint8_t> signature = /* signature bytes */;
std::string cert_pem = /* X.509 certificate in PEM format */;

// Verify
auto result = verifier.verify(data, signature, cert_pem);

if (result.is_valid) {
    std::cout << "Signature valid!" << std::endl;
    std::cout << "Signer: " << result.signer_identity << std::endl;
} else {
    std::cerr << "Signature invalid: " << result.error_message << std::endl;
}
```

### Basic ECDSA-SHA256 Verification (P-256 or P-384)

```cpp
#include "llm/security/signature_verifier.h"

using namespace themis::llm::security;

// Create ECDSA-SHA256 verifier (supports P-256 and P-384 curves)
ECDSA_SHA256_Verifier verifier;

// Load data and signature
std::vector<uint8_t> data = /* your data */;
std::vector<uint8_t> signature = /* ECDSA signature bytes */;
std::string cert_pem = /* X.509 certificate with P-256 or P-384 public key */;

// Verify
auto result = verifier.verify(data, signature, cert_pem);

if (result.is_valid) {
    std::cout << "ECDSA signature valid!" << std::endl;
    std::cout << "Algorithm: " << result.algorithm << std::endl;  // "ECDSA-SHA256"
    std::cout << "Signer: " << result.signer_identity << std::endl;
} else {
    std::cerr << "Signature invalid: " << result.error_message << std::endl;
}
```

### Basic ECDSA-SHA384 Verification

```cpp
#include "llm/security/signature_verifier.h"

using namespace themis::llm::security;

// Create ECDSA-SHA384 verifier
ECDSA_SHA384_Verifier verifier;

// Verify (same interface as SHA256)
auto result = verifier.verify(data, signature, cert_pem);

if (result.is_valid) {
    std::cout << "ECDSA-SHA384 signature valid!" << std::endl;
}
```

### Mixed Algorithm Verification with Builder

```cpp
#include "llm/security/signature_verifier.h"

using namespace themis::llm::security;

// Build chain supporting both RSA and ECDSA signatures
SignatureVerifierBuilder builder;
auto verifier = builder
    .withRSA_SHA256()                  // Also accept RSA signatures
    .withECDSA_SHA256()                // Accept ECDSA-SHA256 (P-256, P-384)
    .withECDSA_SHA384()                // Accept ECDSA-SHA384 (P-256, P-384)
    .withCertificateChainValidation("/etc/ssl/certs/ca-certificates.crt")
    .build();

// Verify with full chain (accepts any of RSA, ECDSA-SHA256, or ECDSA-SHA384)
auto result = verifier->verify(data, signature, cert_pem);

if (result.is_valid) {
    std::cout << "Verification passed with: " << result.algorithm << std::endl;
}
```

```cpp
#include "llm/security/signature_verifier.h"

using namespace themis::llm::security;

// Build verification chain with RSA, certificate chain, and CRL checks
SignatureVerifierBuilder builder;
auto verifier = builder
    .withRSA_SHA256()
    .withCertificateChainValidation("/etc/ssl/certs/ca-certificates.crt")
    .withCRLCheck("http://crl.example.com/adapter.crl")
    .build();

// Verify with full chain
auto result = verifier->verify(data, signature, cert_pem);

if (result.is_valid && result.chain_valid) {
    std::cout << "Full verification passed!" << std::endl;
} else {
    std::cerr << "Verification failed: " << result.error_message << std::endl;
}
```

## Components

### 1. RSA_SHA256_Verifier

Performs cryptographic RSA-SHA256 signature verification.

**Features**:
- Loads X.509 certificates from PEM format
- Extracts and validates RSA public keys
- Enforces minimum 2048-bit key size
- Computes SHA-256 hash of data
- Verifies signature using OpenSSL EVP API

**Usage**:
```cpp
RSA_SHA256_Verifier verifier;
auto result = verifier.verify(data, signature, cert_pem);
```

### 2. ECDSA_SHA256_Verifier

Performs cryptographic ECDSA-SHA256 signature verification with elliptic curve cryptography.

**Features**:
- Supports P-256 (prime256v1) and P-384 (secp384r1) curves
- Rejects unsupported curves (P-521, etc.) to prevent downgrade attacks
- Loads X.509 certificates from PEM format
- Extracts and validates EC public keys
- Computes SHA-256 hash of data
- Verifies signature using OpenSSL EVP API
- More efficient than RSA for equivalent security

**Supported Curves**:
- ✅ P-256 (secp256r1/prime256v1): NIST standard, 256-bit security
- ✅ P-384 (secp384r1): NIST standard, 384-bit security
- ❌ P-521 and other curves: Rejected for security consistency

**Usage**:
```cpp
ECDSA_SHA256_Verifier verifier;
auto result = verifier.verify(data, signature, cert_pem);

if (result.is_valid && result.algorithm == "ECDSA-SHA256") {
    std::cout << "ECDSA-SHA256 signature verified" << std::endl;
}
```

### 3. ECDSA_SHA384_Verifier

Performs cryptographic ECDSA-SHA384 signature verification with elliptic curve cryptography.

**Features**:
- Supports P-256 and P-384 curves
- Computes SHA-384 hash (longer than SHA-256)
- Suitable for high-security applications
- Same security as ECDSA-SHA256 but with larger hash digest
- Uses OpenSSL EVP API for verification

**Usage**:
```cpp
ECDSA_SHA384_Verifier verifier;
auto result = verifier.verify(data, signature, cert_pem);

if (result.is_valid && result.algorithm == "ECDSA-SHA384") {
    std::cout << "ECDSA-SHA384 signature verified" << std::endl;
}
```

### 4. CertificateChainVerifier

Validates X.509 certificate chains against trusted CAs.

**Features**:
- Loads CA bundles from system or custom paths
- Validates certificate chain to trusted root
- Checks certificate expiration
- Detects self-signed certificates
- Reports specific validation errors

**Usage**:
```cpp
CertificateChainVerifier verifier("/etc/ssl/certs/ca-certificates.crt");
auto result = verifier.verify(data, signature, cert_pem);

if (!result.chain_valid) {
    std::cerr << "Chain validation failed" << std::endl;
}
```

**Supported CA Bundle Paths** (auto-detected):
- `/etc/ssl/certs/ca-certificates.crt` (Debian/Ubuntu)
- `/etc/pki/tls/certs/ca-bundle.crt` (RHEL/CentOS)
- `/etc/ssl/ca-bundle.pem` (OpenSUSE)
- `/usr/local/share/certs/ca-root-nss.crt` (FreeBSD)
- System default paths

### 5. CRLChecker

Checks if certificates are revoked via Certificate Revocation Lists.

**Features**:
- CRL checking framework
- Graceful handling of unavailable CRLs
- Certificate serial number extraction
- Falls back on error (doesn't block validation)

**Usage**:
```cpp
CRLChecker checker("http://crl.example.com/adapter.crl");
auto result = checker.verify(data, signature, cert_pem);
```

**Note**: Full CRL download requires HTTP client integration (planned).

### 6. SignatureVerifierBuilder

Fluent interface for building verification chains supporting RSA and ECDSA algorithms.

**Available Methods**:
- `.withRSA_SHA256()` - Add RSA-SHA256 verification
- `.withECDSA_SHA256()` - Add ECDSA-SHA256 verification (P-256, P-384)
- `.withECDSA_SHA384()` - Add ECDSA-SHA384 verification (P-256, P-384)
- `.withCertificateChainValidation(path)` - Add certificate chain validation
- `.withCRLCheck(url)` - Add CRL revocation checking

**Usage Examples**:
```cpp
SignatureVerifierBuilder builder;

// Add verifiers in desired order (Chain of Responsibility pattern)
auto verifier = builder
    .withRSA_SHA256()                              // Accept RSA-SHA256
    .withECDSA_SHA256()                            // Accept ECDSA-SHA256
    .withECDSA_SHA384()                            // Accept ECDSA-SHA384
    .withCertificateChainValidation("/path/to/ca") // Step: Chain validation
    .withCRLCheck("http://crl.url")                // Step: Revocation check
    .build();

// Execute verification chain
auto result = verifier->verify(data, signature, cert_pem);
```

**Builder Chaining**:
The builder uses fluent interface for natural call sequences:
```cpp
auto verifier = SignatureVerifierBuilder()
    .withECDSA_SHA256()
    .build();  // Build returns shared_ptr<ISignatureVerifier>
```

## Result Structure

```cpp
struct SignatureVerificationResult {
    bool is_valid;                      // Overall validity
    std::string algorithm;              // Algorithm used (e.g., "RSA-SHA256")
    std::string signer_identity;        // Subject from certificate
    std::string error_message;          // Error details if invalid
    std::vector<std::string> chain_fingerprints; // Chain cert fingerprints
    bool chain_valid;                   // Certificate chain validity
};
```

## Integration with LoRA Security Validator

The signature verifier is integrated with `LoRASecurityValidator`:

```cpp
#include "llm/lora_security_validator.h"

using namespace themis::llm;

LoRASecurityConfig config;
config.require_signature = true;
config.trusted_signers = {"cert_fingerprint_1", "cert_fingerprint_2"};

LoRASecurityValidator validator(config);

// Verify LoRA adapter signature
auto result = validator.verifyEmbeddedSignature("path/to/adapter.safetensors");

if (result.is_valid) {
    std::cout << "LoRA adapter signature verified" << std::endl;
} else {
    std::cerr << "Invalid signature: " << result.error_message << std::endl;
}
```

## RSA vs ECDSA Comparison

| Aspect | RSA-SHA256 | ECDSA-SHA256 | ECDSA-SHA384 |
|--------|-----------|-------------|-------------|
| **Curves** | N/A (RSA) | P-256, P-384 | P-256, P-384 |
| **Key Size for 256-bit Security** | 2048-3072 bits | 256-bit key | 256-bit key |
| **Key Size for 384-bit Security** | 4096 bits | 384-bit key | 384-bit key |
| **Hash Algorithm** | SHA-256 | SHA-256 | SHA-384 |
| **Signature Size** | 256-512 bytes | 64 bytes (P-256) / 96 bytes (P-384) | 64 bytes (P-256) / 96 bytes (P-384) |
| **Performance** | Slower | Faster | Faster |
| **Memory** | More | Less | Less |
| **Verification Speed** | ~1ms (2048-bit) | <1ms | <1ms |
| **Recommended For** | Legacy systems | New deployments | High-security apps |

**Curve Selection Rationale**:
- **P-256**: Standard NIST curve, widely supported, 128-bit security level
- **P-384**: Higher security margin, FIPS approved, 192-bit security level
- **Others (P-521)**: Rejected to maintain algorithm consistency

## Security Considerations

### Key Requirements
- ✅ Minimum 2048-bit RSA keys (enforced)
- ✅ P-256 or P-384 ECDSA curves (others rejected)
- ✅ SHA-256 or SHA-384 hash algorithms (not SHA-1)
- ✅ X.509 v3 certificates
- ✅ Certificate chain to trusted CA
- ✅ Detailed error messages

### Threat Mitigation
| Threat | Mitigation |
|--------|-----------|
| Data tampering | Signature verification fails on modified data |
| Malicious adapter | Unsigned or improperly signed adapter rejected |
| Compromised key | CRL check fails (when CRL available) |
| Man-in-the-middle | Certificate chain validation fails |
| Weak keys | 1024-bit RSA keys rejected; only P-256/P-384 ECDSA accepted |
| Algorithm downgrade | Unsupported ECDSA curves (P-521, etc.) rejected |
| Key confusion | Verifier enforces specific algorithm (RSA vs ECDSA) |

### TODO: Security Enhancements
- [ ] Constant-time comparison (timing attack resistance)
- [ ] Certificate pinning (optional)
- [ ] Rate limiting for verification (DoS prevention)
- [ ] Full CRL download/caching

## Testing

### Generate Test Certificates

```bash
cd tests/data/certificates
./generate_test_certs.sh
```

This creates:
- CA certificate and key
- RSA test certificates (2048, 3072, 4096-bit)
- ECDSA test certificates (P-256, P-384, P-521)
- Self-signed certificate
- Expired certificate
- Weak 1024-bit certificate
- Test data and signatures (RSA and ECDSA variants)

### Run Tests

**Minimal standalone test** (no dependencies):
```bash
cd tests
./test_signature_minimal.sh
```

**GTest suite** (requires full build):
```bash
cd build
ctest -R SignatureVerifierTests
```

## Example: Signing and Verifying a LoRA Adapter

### 1. Generate Key Pair
```bash
# Generate 2048-bit RSA key
openssl genrsa -out lora_key.pem 2048

# Create certificate signing request
openssl req -new -key lora_key.pem -out lora.csr \
    -subj "/C=US/ST=CA/O=YourOrg/CN=lora-signer"

# Sign with your CA (or create self-signed for testing)
openssl x509 -req -in lora.csr -CA ca_cert.pem -CAkey ca_key.pem \
    -CAcreateserial -out lora_cert.pem -days 365 -sha256
```

### 2. Sign LoRA Adapter
```bash
# Sign the adapter file
openssl dgst -sha256 -sign lora_key.pem \
    -out adapter_signature.bin adapter.safetensors
```

### 3. Verify in Code
```cpp
// Load adapter, signature, and certificate
auto adapter_data = loadFile("adapter.safetensors");
auto signature = loadFile("adapter_signature.bin");
auto cert_pem = loadFile("lora_cert.pem");

// Verify
RSA_SHA256_Verifier verifier;
auto result = verifier.verify(adapter_data, signature, cert_pem);

if (result.is_valid) {
    // Load and use the adapter
    loadLoRAAdapter("adapter.safetensors");
}
```

## Example: ECDSA Signing and Verification

### 1. Generate ECDSA Key Pair

```bash
# Generate P-256 ECDSA key
openssl ecparam -name prime256v1 -genkey -noout -out lora_ecdsa_key.pem

# Or P-384 for higher security
openssl ecparam -name secp384r1 -genkey -noout -out lora_ecdsa_key.pem

# Create certificate signing request
openssl req -new -key lora_ecdsa_key.pem -out lora_ecdsa.csr \
    -subj "/C=US/ST=CA/O=YourOrg/CN=lora-ecdsa-signer"

# Sign with your CA (or create self-signed for testing)
openssl x509 -req -in lora_ecdsa.csr -CA ca_cert.pem -CAkey ca_key.pem \
    -CAcreateserial -out lora_ecdsa_cert.pem -days 365 -sha256
```

### 2. Sign LoRA Adapter with ECDSA

```bash
# Sign with SHA-256
openssl dgst -sha256 -sign lora_ecdsa_key.pem \
    -out adapter_ecdsa_sha256.bin adapter.safetensors

# Or sign with SHA-384
openssl dgst -sha384 -sign lora_ecdsa_key.pem \
    -out adapter_ecdsa_sha384.bin adapter.safetensors
```

### 3. Verify ECDSA Signature in Code

```cpp
// Load adapter, signature, and certificate
auto adapter_data = loadFile("adapter.safetensors");
auto signature = loadFile("adapter_ecdsa_sha256.bin");
auto cert_pem = loadFile("lora_ecdsa_cert.pem");

// Verify with ECDSA-SHA256
ECDSA_SHA256_Verifier verifier;
auto result = verifier.verify(adapter_data, signature, cert_pem);

if (result.is_valid) {
    std::cout << "ECDSA signature valid on: " << result.signer_identity << std::endl;
    loadLoRAAdapter("adapter.safetensors");
}
```

## Performance

Typical verification times (on modern CPU):
- RSA-2048 verification: < 1ms
- RSA-4096 verification: < 5ms
- Certificate chain validation: < 10ms
- Full chain (RSA + chain + CRL): < 15ms

## Error Messages

Common error messages and meanings:

| Error | Meaning |
|-------|---------|
| "Data is empty" | Input data vector is empty |
| "Signature is empty" | Signature vector is empty |
| "Certificate is empty" | Certificate PEM string is empty |
| "Failed to load certificate" | Invalid PEM format |
| "Failed to extract public key" | Certificate doesn't contain valid public key |
| "RSA key size too small: X bits" | Key is smaller than 2048 bits |
| "Signature verification failed: signature does not match" | Signature is invalid (data tampered or wrong key) |
| "Certificate chain validation failed" | Certificate not trusted by CA bundle |
| "Certificate has expired" | Certificate validity period has ended |
| "Self-signed certificate" | Certificate not signed by trusted CA |

## Best Practices

1. **Always validate certificate chains** in production
2. **Use strong keys** (≥2048 bits, prefer 3072 or 4096)
3. **Keep CA bundles updated** for security
4. **Log verification failures** for security auditing
5. **Use CRL checking** when available
6. **Rotate keys regularly** (annually recommended)
7. **Store private keys securely** (HSM for production)
8. **Never commit private keys** to source control

## References

- OpenSSL EVP API: https://www.openssl.org/docs/man3.0/man3/EVP_PKEY_verify.html
- X.509 Certificates: https://www.openssl.org/docs/man3.0/man3/X509_verify_cert.html
- RSA Best Practices: https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-57pt1r5.pdf

## Support

For issues or questions:
- Check error messages for specific failure reasons
- Review test certificates in `tests/data/certificates/`
- Run standalone tests to verify OpenSSL installation
- Check system CA bundle paths are accessible

---

**Version**: 2.0  
**Status**: Production Ready  
**Last Updated**: 2026-07-18  
**Changes**: Added ECDSA-SHA256 and ECDSA-SHA384 support (Phase 2 Block B)
