# VaultSigningProvider - Signing-Only Limitation

**Version:** 1.4.2  
**Last Updated:** April 2026  
**Classification:** Public  
**Related:** Security Hardening, HashiCorp Vault Integration

---

## Overview

`VaultSigningProvider` is a **signing-only** provider that uses HashiCorp Vault Transit Engine for cryptographic signature operations. It does **NOT** support key management operations (getKey, rotateKey, listKeys, etc.).

### Purpose

- ✅ **Sign data** using Vault Transit Engine keys
- ✅ **Verify signatures** using Vault Transit Engine
- ❌ **NOT for key extraction** - keys never leave Vault
- ❌ **NOT for key management** - use Vault API or VaultKeyProvider

---

## Signing-Only Design

### What VaultSigningProvider Does

```cpp
VaultSigningProvider::Config config;
config.vault_addr = "https://vault.example.com:8200";
config.vault_token = "s.xxxxx";
config.transit_mount = "transit";

VaultSigningProvider provider(config);

// ✅ Signing works
std::vector<uint8_t> data = {1, 2, 3, 4};
SigningResult result = provider.sign("my-signing-key", data);
```

### What VaultSigningProvider Does NOT Do

```cpp
// ❌ Key operations throw KeyOperationException with helpful error messages
try {
    auto key = provider.getKey("my-key");  // THROWS
} catch (const KeyOperationException& e) {
    // Error: "VaultSigningProvider: getKey() not implemented - signing-only provider.
    //         Use VaultKeyProvider for key management operations.
    //         See: docs/security/VAULT_SIGNING_PROVIDER.md"
}

// ❌ These also throw with similar messages:
provider.rotateKey("my-key");           // THROWS
provider.listKeys();                    // THROWS
provider.getKeyMetadata("my-key");      // THROWS
provider.deleteKey("my-key", 1);        // THROWS
provider.hasKey("my-key");              // THROWS
provider.createKeyFromBytes("k", data); // THROWS
```

---

## Why Signing-Only?

### Security Benefits

1. **Keys Never Leave Vault**
   - Private keys remain in Vault's secure storage
   - Only signatures are returned to the application
   - Reduces attack surface for key extraction

2. **Least Privilege**
   - Application only needs signing permissions
   - No access to key material or management operations
   - Follows principle of least privilege

3. **Simplified Token Policies**
   ```hcl
   # Vault policy for signing-only access
   path "transit/sign/my-signing-key" {
     capabilities = ["update"]
   }
   
   path "transit/verify/my-signing-key" {
     capabilities = ["update"]
   }
   ```

### Design Trade-offs

- ✅ **Pro:** Enhanced security through limited scope
- ✅ **Pro:** Clear separation of concerns
- ❌ **Con:** Cannot be used as a full KeyProvider
- ❌ **Con:** Requires separate mechanism for key management

---

## Migration Paths

### If You Need Full Key Management

**Option 1: Use VaultKeyProvider**
```cpp
#include "security/vault_key_provider.h"

VaultKeyProvider::Config config;
config.vault_addr = "https://vault.example.com:8200";
config.vault_token = "s.xxxxx";
config.transit_mount = "transit";

auto provider = std::make_shared<VaultKeyProvider>(config);

// ✅ Supports full KeyProvider interface
auto key = provider->getKey("my-key");
uint32_t new_version = provider->rotateKey("my-key");
auto keys = provider->listKeys();
```

**Option 2: Use Vault CLI/API Directly**
```bash
# Key rotation via Vault CLI
vault write -f transit/keys/my-key/rotate

# List keys
vault list transit/keys

# Get key metadata
vault read transit/keys/my-key
```

**Option 3: Use Cloud KMS**
```cpp
// AWS KMS
#include "security/aws_kms_provider.h"

// Azure Key Vault
#include "security/azure_keyvault_provider.h"

// GCP Cloud KMS
#include "security/gcp_kms_provider.h"
```

### If You Only Need Signing

`VaultSigningProvider` is the right choice! It provides:
- Minimal permissions required
- Clear security boundaries
- Simple signing workflow

---

## Feature Flag

### THEMIS_VAULT_SIGNING_ONLY

Set this environment variable to explicitly acknowledge the signing-only limitation:

```bash
export THEMIS_VAULT_SIGNING_ONLY=1
```

**Purpose:**
- Documents intent to use signing-only functionality
- May suppress certain warnings in future versions
- Makes limitation explicit in deployment configuration

---

## Configuration

### Basic Configuration

```yaml
# config/security.yaml
vault:
  provider: vault_signing
  
  # Vault connection
  vault_addr: "https://vault.example.com:8200"
  vault_token: "${VAULT_TOKEN}"  # Use environment variable
  transit_mount: "transit"
  
  # Optional settings
  request_timeout_ms: 5000
  verify_ssl: true
```

### Environment Variables

```bash
# Required
export THEMIS_VAULT_ADDR="https://vault.example.com:8200"
export THEMIS_VAULT_TOKEN="s.xxxxx"

# Optional
export THEMIS_VAULT_TRANSIT_MOUNT="transit"  # default: "transit"
export THEMIS_VAULT_SIGNING_ONLY="1"         # acknowledge limitation
```

---

## Production Deployment

### Vault Setup

1. **Enable Transit Engine**
   ```bash
   vault secrets enable transit
   ```

2. **Create Signing Key**
   ```bash
   vault write -f transit/keys/themis-signing-key \
     type=rsa-2048 \
     exportable=false
   ```

3. **Create Token Policy**
   ```hcl
   # signing-policy.hcl
   path "transit/sign/themis-signing-key" {
     capabilities = ["update"]
   }
   
   path "transit/verify/themis-signing-key" {
     capabilities = ["update"]
   }
   ```

4. **Generate Application Token**
   ```bash
   vault token create -policy=signing-policy
   ```

### Security Checklist

- [ ] Keys created with `exportable=false`
- [ ] Token policy limited to signing operations only
- [ ] Token TTL configured appropriately
- [ ] TLS enabled for Vault communication (`verify_ssl: true`)
- [ ] Token stored securely (not in version control)
- [ ] Token rotation policy in place

---

## Error Messages

All unsupported operations throw `KeyOperationException` with descriptive messages:

```
VaultSigningProvider: getKey() not implemented - signing-only provider.
Use VaultKeyProvider for key management operations.
See: docs/security/VAULT_SIGNING_PROVIDER.md
```

**Error Message Components:**
1. **Provider identification:** `VaultSigningProvider:`
2. **Specific operation:** `getKey() not implemented`
3. **Explanation:** `signing-only provider`
4. **Guidance:** `Use VaultKeyProvider for key management operations`
5. **Documentation:** `See: docs/security/VAULT_SIGNING_PROVIDER.md`

---

## Testing

### Unit Tests

```cpp
TEST(VaultSigningProviderTest, SigningWorks) {
    // Set up Vault connection
    VaultSigningProvider::Config config;
    config.vault_addr = "http://localhost:8200";
    config.vault_token = "test-token";
    
    VaultSigningProvider provider(config);
    
    std::vector<uint8_t> data = {1, 2, 3, 4};
    SigningResult result = provider.sign("test-key", data);
    
    EXPECT_TRUE(result.signature.size() > 0);
    EXPECT_EQ(result.algorithm, "VAULT+TRANSIT");
}

TEST(VaultSigningProviderTest, KeyOperationsThrow) {
    VaultSigningProvider::Config config;
    VaultSigningProvider provider(config);
    
    EXPECT_THROW(provider.getKey("key"), KeyOperationException);
    EXPECT_THROW(provider.rotateKey("key"), KeyOperationException);
    EXPECT_THROW(provider.listKeys(), KeyOperationException);
}
```

### Integration Tests

See: `tests/test_vault_signing_provider.cpp`

---

## Troubleshooting

### Common Issues

**Issue:** `KeyOperationException: getKey not implemented`
- **Cause:** Attempting to use VaultSigningProvider as a full KeyProvider
- **Solution:** Use `VaultKeyProvider` instead, or implement key management separately

**Issue:** `Vault not configured: THEMIS_VAULT_ADDR not set`
- **Cause:** Environment variables not set
- **Solution:** Set `THEMIS_VAULT_ADDR` and `THEMIS_VAULT_TOKEN`

**Issue:** `Vault request failed: Couldn't connect to server`
- **Cause:** Vault server not reachable
- **Solution:** Check network connectivity, Vault address, and firewall rules

---

## Compliance Notes

### Standards Alignment

- **NIST SP 800-57:** Key management via separate mechanism
- **PCI DSS 3.6:** Keys protected in Vault (never extracted)
- **GDPR Article 32:** Cryptographic protection maintained
- **ISO 27001 A.10.1.2:** Least privilege principle

### Audit Trail

All signing operations are logged by Vault Transit Engine:
```bash
vault audit enable file file_path=/var/log/vault_audit.log

# Review signing operations
vault audit log | grep "transit/sign"
```

---

## Related Documentation

- [HSM Production Setup](HSM_PRODUCTION_SETUP.md)
- [VaultKeyProvider Documentation](../../src/security/vault_key_provider.cpp.md)
- [Security Configuration](../../config/security.yaml)
- [PKCS#11 Integration](PKCS11_INTEGRATION.md)

---

## Summary

`VaultSigningProvider` is a **specialized, security-focused** provider for signing operations only:

✅ **Use When:**
- You need signing operations via Vault Transit
- Keys should never leave Vault
- Minimal permissions are required

❌ **Don't Use When:**
- You need to extract key material
- You need key management operations
- You need full KeyProvider interface

For full key management, use `VaultKeyProvider` or cloud KMS alternatives.
