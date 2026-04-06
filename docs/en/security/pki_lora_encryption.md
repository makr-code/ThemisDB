# PKI-Based LoRA Adapter Encryption

This guide describes how to configure certificate-based encryption for LoRA adapters using the PKIKeyProvider.

## Overview

The PKIKeyProvider enables certificate-based key management for LoRA adapter encryption without requiring external services like HashiCorp Vault or Hardware Security Modules (HSMs). This is suitable for:

- Organizations with existing PKI infrastructure
- Development and testing environments
- Air-gapped deployments
- Certificate-based key distribution

## Key Derivation Architecture

The PKIKeyProvider uses a 3-tier key hierarchy:

1. **KEK (Key Encryption Key)**: Derived from the certificate's public key using HKDF-SHA256
2. **DEK (Data Encryption Key)**: Random 256-bit AES key, encrypted with KEK and stored in RocksDB
3. **Field Keys**: Derived from DEK using HKDF with field-specific context

This architecture enables:
- **Certificate rotation**: Update certificate and re-encrypt DEK (no data re-encryption needed)
- **Key versioning**: Multiple DEK versions for gradual migration
- **Persistent encryption**: Keys remain stable across service restarts

## Certificate Generation

### Development: Self-Signed Certificate

For development and testing, generate a self-signed certificate:

```bash
# Generate 4096-bit RSA private key
openssl genrsa -out lora-encryption.key 4096

# Generate self-signed certificate (valid for 365 days)
openssl req -new -x509 -key lora-encryption.key \
  -out lora-encryption.crt -days 365 \
  -subj "/CN=ThemisDB-LoRA-Encryption/O=ThemisDB/C=DE"

# Verify certificate
openssl x509 -in lora-encryption.crt -text -noout
```

**Security Note**: Self-signed certificates are suitable for development only. Use CA-signed certificates in production.

### Production: CA-Signed Certificate

For production environments, obtain a certificate from your Certificate Authority:

```bash
# Generate private key (if not already generated)
openssl genrsa -out lora-encryption.key 4096

# Generate Certificate Signing Request (CSR)
openssl req -new -key lora-encryption.key \
  -out lora-encryption.csr \
  -subj "/CN=themisdb-lora.example.com/O=YourOrg/C=DE"

# Submit CSR to your CA (process varies by CA)
# Receive signed certificate: lora-encryption.crt

# Verify certificate chain
openssl verify -CAfile ca-bundle.crt lora-encryption.crt
```

## Configuration

### C++ API Configuration

Configure LoRAStorageService to use PKI encryption:

```cpp
#include "llm/lora_framework/lora_storage_service.h"

// Configure storage service
themis::llm::lora::LoRAStorageService::Config config;
config.backend = themis::llm::lora::LoRAStorageService::Backend::ThemisDB;
config.db = rocksdb_instance;  // Your RocksDB instance

// Enable PKI-based encryption
config.enable_encryption = true;
config.use_pki_for_encryption = true;
config.pki_cert_path = "/etc/themis/certs/lora-encryption.crt";
config.pki_private_key_path = "/etc/themis/keys/lora-encryption.key";
config.pki_verify_certificate = true;  // Validate certificate expiration

// Initialize storage service
themis::llm::lora::LoRAStorageService storage(config);
```

### Configuration File (YAML)

Example YAML configuration:

```yaml
lora_storage:
  backend: themisdb
  collection_name: lora_adapters
  enable_encryption: true
  
  # PKI Configuration
  use_pki_for_encryption: true
  pki_cert_path: /etc/themis/certs/lora-encryption.crt
  pki_private_key_path: /etc/themis/keys/lora-encryption.key
  pki_verify_certificate: true
```

### Environment Variables

Configuration can also be provided via environment variables:

```bash
export THEMIS_LORA_ENABLE_ENCRYPTION=true
export THEMIS_LORA_USE_PKI=true
export THEMIS_LORA_PKI_CERT_PATH=/etc/themis/certs/lora-encryption.crt
export THEMIS_LORA_PKI_KEY_PATH=/etc/themis/keys/lora-encryption.key
```

## Certificate Management

### File Permissions

Protect your private key with appropriate file permissions:

```bash
# Set secure permissions
chmod 600 /etc/themis/keys/lora-encryption.key
chmod 644 /etc/themis/certs/lora-encryption.crt

# Set ownership (run as appropriate user)
chown themis:themis /etc/themis/keys/lora-encryption.key
chown themis:themis /etc/themis/certs/lora-encryption.crt
```

### Certificate Expiration

Monitor certificate expiration to avoid service disruption:

```bash
# Check certificate expiration
openssl x509 -in /etc/themis/certs/lora-encryption.crt -noout -dates

# Check days until expiration
openssl x509 -in /etc/themis/certs/lora-encryption.crt -noout -checkend $((30*86400))
```

**Recommendation**: Set up monitoring alerts 30, 14, and 7 days before expiration.

### Certificate Rotation

When rotating certificates (e.g., before expiration):

1. **Generate new certificate** (as shown above)
2. **Update configuration** with new certificate path
3. **Restart service** to load new certificate
4. **Verify encryption** works with new certificate

**Important**: The PKIKeyProvider derives the KEK from the certificate's public key. When you rotate certificates:
- The KEK will change
- Existing DEKs (encrypted with old KEK) will need to be re-encrypted
- Data encrypted with old DEKs remains accessible (DEKs are cached in RocksDB)

To minimize disruption, plan certificate rotation during maintenance windows.

## Security Best Practices

### Certificate Storage

- Store private keys in secure directories with restricted permissions (600)
- Never commit private keys to version control
- Use hardware security modules (HSMs) for private key storage in high-security environments
- Consider using encrypted filesystems for key storage

### Certificate Validation

Always enable certificate validation in production:

```cpp
config.pki_verify_certificate = true;  // Default: true
```

This ensures:
- Certificate has not expired
- Certificate is not yet valid (future-dated)
- Certificate is properly formatted

### Key Rotation

Implement a key rotation schedule:
- **Certificate rotation**: Every 365 days (before expiration)
- **DEK rotation**: Every 90 days (optional, for defense in depth)

### Monitoring

Monitor the following metrics:
- Certificate expiration dates
- Encryption operation success/failure rates
- Key derivation latency
- Certificate validation failures

## Troubleshooting

### Error: "Failed to open certificate file"

**Cause**: Certificate file path is incorrect or file doesn't exist.

**Solution**: Verify the file path and permissions:
```bash
ls -la /etc/themis/certs/lora-encryption.crt
```

### Error: "Certificate has expired"

**Cause**: The certificate has passed its expiration date.

**Solution**: Generate a new certificate or obtain a renewed certificate from your CA.

### Error: "Failed to parse X.509 certificate"

**Cause**: Certificate file is corrupted or not in PEM format.

**Solution**: Verify the certificate file:
```bash
openssl x509 -in /etc/themis/certs/lora-encryption.crt -text -noout
```

### Error: "PKI encryption enabled but pki_cert_path is not configured"

**Cause**: `use_pki_for_encryption` is true but certificate path is not set.

**Solution**: Configure both certificate and private key paths:
```cpp
config.pki_cert_path = "/path/to/cert.crt";
config.pki_private_key_path = "/path/to/key.key";
```

## Performance Considerations

PKI-based encryption performance:

- **KEK derivation**: ~5ms (one-time at startup)
- **DEK loading**: ~2ms (cached after first load)
- **Encryption**: ~0.5ms per 1KB (AES-256-GCM)
- **Decryption**: ~0.5ms per 1KB (AES-256-GCM)

The PKIKeyProvider caches derived keys, so performance impact is minimal after initialization.

## Comparison with Other Providers

| Feature | PKIKeyProvider | VaultKeyProvider | HSMProvider | MockKeyProvider |
|---------|---------------|------------------|-------------|-----------------|
| External dependency | None | Vault server | HSM hardware | None |
| Certificate-based | Yes | No | Optional | No |
| Production-ready | Yes | Yes | Yes | **No** |
| Key rotation | Manual | Automatic | Manual | N/A |
| Offline operation | Yes | No | Yes | Yes |
| Complexity | Low | Medium | High | Low |

## References

- [OpenSSL X.509 Documentation](https://www.openssl.org/docs/man1.1.1/man3/X509_new.html)
- [HKDF (RFC 5869)](https://tools.ietf.org/html/rfc5869)
- [AES-GCM (NIST SP 800-38D)](https://csrc.nist.gov/publications/detail/sp/800-38d/final)
- ThemisDB Security Documentation

---

**Last Updated**: April 2026
**Version**: 1.0
