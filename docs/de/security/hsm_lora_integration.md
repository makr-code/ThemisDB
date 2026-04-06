# HSM Integration for LoRA Adapter Encryption

**Status**: ✅ Implemented  
**Version**: 1.8.0-rc1  
**Date**: January 16, 2026

## Overview

This guide covers the integration of Hardware Security Modules (HSM) with ThemisDB's LoRA adapter storage to provide hardware-backed encryption. The integration uses PKCS#11 standard to support various HSM devices.

## Table of Contents

1. [Architecture](#architecture)
2. [Supported HSM Devices](#supported-hsm-devices)
3. [Installation](#installation)
4. [Configuration](#configuration)
5. [Testing with SoftHSM2](#testing-with-softhsm2)
6. [Production Deployment](#production-deployment)
7. [Security Best Practices](#security-best-practices)
8. [Troubleshooting](#troubleshooting)

## Architecture

### Envelope Encryption Pattern

ThemisDB uses envelope encryption to combine the security of HSM with the performance of software encryption:

```
┌─────────────────────────────────────────────────────────────┐
│                    LoRA Adapter Storage                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  1. Generate random DEK (Data Encryption Key)                │
│     ├─> 32-byte AES-256 key                                  │
│     └─> Used for actual data encryption (fast)               │
│                                                               │
│  2. Wrap DEK with HSM KEK (Key Encryption Key)               │
│     ├─> KEK stored in HSM hardware                           │
│     ├─> DEK encrypted by HSM (secure)                        │
│     └─> Encrypted DEK stored with adapter metadata           │
│                                                               │
│  3. Encrypt adapter data with DEK                            │
│     ├─> AES-256-GCM encryption                               │
│     ├─> Fast performance (~0.5ms per KB)                     │
│     └─> Authenticated encryption                             │
│                                                               │
│  4. Cache decrypted DEK                                      │
│     ├─> TTL: 5 minutes (configurable)                        │
│     ├─> Reduces HSM operations                               │
│     └─> Thread-safe cache                                    │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Components

1. **HSMProvider** (`include/security/hsm_provider.h`)
   - Low-level PKCS#11 interface
   - Session management and pooling
   - Sign/verify operations

2. **HSMKeyProviderAdapter** (`include/security/hsm_key_provider_adapter.h`)
   - Bridges HSMProvider with KeyProvider interface
   - Implements envelope encryption
   - DEK caching and lifecycle management

3. **LoRAStorageService** (`include/llm/lora_framework/lora_storage_service.h`)
   - High-level LoRA adapter storage
   - Integrates with HSMKeyProviderAdapter
   - Transparent encryption/decryption

## Supported HSM Devices

### Hardware HSMs

| Vendor | Model | PKCS#11 Library | Tested |
|--------|-------|-----------------|--------|
| Thales/SafeNet | Luna Network HSM | `/usr/safenet/lunaclient/lib/libCryptoki2_64.so` | ✓ |
| Utimaco | CryptoServer | `/opt/utimaco/lib/libcs_pkcs11_R2.so` | ✓ |
| AWS | CloudHSM | `/opt/cloudhsm/lib/libcloudhsm_pkcs11.so` | ✓ |
| Gemalto | ProtectServer | `/usr/lib/libprotect.so` | ○ |

### Software HSMs (Testing)

| Name | PKCS#11 Library | Use Case |
|------|-----------------|----------|
| SoftHSM2 | `/usr/lib/softhsm/libsofthsm2.so` | Development & Testing |
| OpenSC | `/usr/lib/opensc-pkcs11.so` | Smart card testing |

**Legend**: ✓ = Fully tested | ○ = Compatibility verified | ✗ = Not tested

## Installation

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    softhsm2 \
    opensc-pkcs11

# RHEL/CentOS
sudo yum install -y \
    gcc-c++ \
    cmake \
    openssl-devel \
    softhsm \
    opensc
```

### ThemisDB with HSM Support

ThemisDB is built with HSM support by default. No additional compilation flags needed.

```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
./scripts/build.sh
```

## Configuration

### LoRAStorageService Configuration

Add HSM configuration to your LoRA storage service:

```cpp
#include "llm/lora_framework/lora_storage_service.h"

themis::llm::lora::LoRAStorageService::Config config;

// Enable encryption
config.enable_encryption = true;

// Enable HSM-backed encryption
config.use_hsm_for_encryption = true;

// HSM Configuration
config.hsm_library_path = "/usr/lib/softhsm/libsofthsm2.so";  // PKCS#11 library
config.hsm_slot_id = 0;                                         // HSM slot
config.hsm_pin = "1234";                                        // User PIN
config.hsm_key_label = "lora-adapter-kek";                     // KEK label
config.hsm_session_pool_size = 4;                              // Parallel sessions

// Create storage service
auto storage = std::make_unique<themis::llm::lora::LoRAStorageService>(config);
```

### Environment Variables

For production, avoid hardcoding PINs. Use environment variables:

```bash
export THEMIS_HSM_LIBRARY="/usr/lib/softhsm/libsofthsm2.so"
export THEMIS_HSM_SLOT="0"
export THEMIS_HSM_PIN="1234"  # Use secure secret management!
export THEMIS_HSM_KEY_LABEL="lora-adapter-kek"
```

```cpp
config.hsm_library_path = std::getenv("THEMIS_HSM_LIBRARY");
config.hsm_slot_id = std::stoul(std::getenv("THEMIS_HSM_SLOT") ?: "0");
config.hsm_pin = std::getenv("THEMIS_HSM_PIN");
config.hsm_key_label = std::getenv("THEMIS_HSM_KEY_LABEL") ?: "lora-adapter-kek";
```

### Configuration File (YAML)

```yaml
# config/themisdb.yaml
lora_storage:
  backend: themisdb
  encryption:
    enabled: true
    use_hsm: true
    hsm:
      library_path: /usr/lib/softhsm/libsofthsm2.so
      slot_id: 0
      pin: ${THEMIS_HSM_PIN}  # Read from environment
      key_label: lora-adapter-kek
      session_pool_size: 4
```

## Testing with SoftHSM2

SoftHSM2 is a software implementation of PKCS#11 for testing and development.

### Installation

```bash
# Ubuntu/Debian
sudo apt-get install softhsm2

# macOS
brew install softhsm

# Verify installation
softhsm2-util --version
```

### Initialize Token

```bash
# Create token storage directory
sudo mkdir -p /var/lib/softhsm/tokens
sudo chown -R $(whoami) /var/lib/softhsm

# Initialize token
softhsm2-util --init-token \
  --slot 0 \
  --label "ThemisDB-Test" \
  --pin 1234 \
  --so-pin 5678

# Verify token
softhsm2-util --show-slots
```

Expected output:
```
Slot 0
    Slot info:
        Description:      SoftHSM slot ID 0x0
        Manufacturer ID:  SoftHSM project
        Hardware version: 2.6
        Firmware version: 2.6
        Token present:    yes
    Token info:
        Manufacturer ID:  SoftHSM project
        Model:            SoftHSM v2
        Hardware version: 2.6
        Firmware version: 2.6
        Serial number:    1234567890abcdef
        Initialized:      yes
        User PIN init.:   yes
        Label:            ThemisDB-Test
```

### Generate KEK in SoftHSM

```bash
# Generate RSA-2048 key pair
pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so \
  --login --pin 1234 \
  --keypairgen \
  --key-type RSA:2048 \
  --label "lora-adapter-kek" \
  --id 01

# Verify key
pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so \
  --login --pin 1234 \
  --list-objects
```

Expected output:
```
Private Key Object; RSA
  label:      lora-adapter-kek
  ID:         01
  Usage:      decrypt, sign, unwrap
Public Key Object; RSA 2048 bits
  label:      lora-adapter-kek
  ID:         01
  Usage:      encrypt, verify, wrap
```

### Run Tests

```bash
# Set environment variables
export THEMIS_TEST_HSM_LIBRARY=/usr/lib/softhsm/libsofthsm2.so
export THEMIS_TEST_HSM_PIN=1234

# Run HSM tests
cd ThemisDB/build
ctest -R hsm -V
```

### Test Output Example

```
1/3 Test #1: test_hsm_provider ......................   Passed    0.52 sec
2/3 Test #2: test_hsm_key_provider_adapter ..........   Passed    1.23 sec
3/3 Test #3: test_lora_hsm_integration ..............   Passed    2.15 sec

100% tests passed, 0 tests failed out of 3
```

## Production Deployment

### Hardware HSM Setup

#### 1. Physical Installation

- Install HSM hardware according to vendor documentation
- Connect to network (for network HSMs) or PCIe slot
- Verify hardware status LEDs

#### 2. Initialize HSM

```bash
# Example for Thales Luna HSM
lunacm
> hsm init -label ThemisDB-Production
> partition create -partition themisdb -password <secure-password>
> exit

# Create KEK
cmu generatekeypair -modulusBits=2048 -label=lora-adapter-kek -sign=1 -encrypt=1
```

#### 3. Configure ThemisDB

Update production configuration:

```yaml
# config/production/themisdb.yaml
lora_storage:
  encryption:
    enabled: true
    use_hsm: true
    hsm:
      library_path: /usr/safenet/lunaclient/lib/libCryptoki2_64.so
      slot_id: 0
      pin: ${THEMIS_HSM_PIN}  # From Vault/Secrets Manager
      key_label: lora-adapter-kek
      session_pool_size: 8  # Tune based on load
```

#### 4. Secrets Management

**Never hardcode PINs!** Use a secrets management system:

**HashiCorp Vault:**
```bash
# Store PIN in Vault
vault kv put secret/themisdb/hsm pin=<secure-pin>

# Retrieve at runtime
export THEMIS_HSM_PIN=$(vault kv get -field=pin secret/themisdb/hsm)
```

**AWS Secrets Manager:**
```bash
# Store PIN
aws secretsmanager create-secret \
  --name themisdb/hsm/pin \
  --secret-string <secure-pin>

# Retrieve at runtime (in application startup)
aws secretsmanager get-secret-value \
  --secret-id themisdb/hsm/pin \
  --query SecretString \
  --output text
```

### Monitoring

Monitor HSM health and performance:

```cpp
// Get HSM statistics
auto stats = adapter->getStats();
spdlog::info("HSM Stats:");
spdlog::info("  Cache Hit Rate: {:.2f}%", stats["cache_hit_rate"].get<double>() * 100);
spdlog::info("  HSM Encrypt Ops: {}", stats["hsm_encrypt_operations"]);
spdlog::info("  HSM Decrypt Ops: {}", stats["hsm_decrypt_operations"]);
spdlog::info("  HSM Errors: {}", stats["hsm_errors"]);
```

Integrate with Prometheus:

```yaml
# prometheus.yml
- job_name: 'themisdb'
  static_configs:
    - targets: ['localhost:4318']
  metrics_path: /metrics
```

## Security Best Practices

### 1. PIN Management

- ✅ **DO**: Store PINs in secrets manager (Vault, AWS Secrets Manager)
- ✅ **DO**: Rotate PINs regularly (every 90 days)
- ✅ **DO**: Use different PINs for dev/staging/production
- ❌ **DON'T**: Hardcode PINs in source code
- ❌ **DON'T**: Log PINs or include in error messages
- ❌ **DON'T**: Store PINs in plain text configuration files

### 2. Key Lifecycle

- ✅ Generate KEK in HSM (never import)
- ✅ Mark KEK as non-exportable
- ✅ Implement key rotation schedule (annually)
- ✅ Backup HSM configuration (but not keys!)
- ✅ Test key recovery procedures

### 3. Access Control

- ✅ Restrict HSM access to ThemisDB service account only
- ✅ Enable HSM audit logging
- ✅ Monitor failed authentication attempts
- ✅ Implement IP allowlisting for network HSMs
- ✅ Use mTLS for network HSM connections

### 4. Performance Tuning

```cpp
// Optimize session pool based on load
config.hsm_session_pool_size = 8;  // 8 parallel sessions

// Tune DEK cache
adapter_config.cache_ttl_ms = 300000;      // 5 minutes
adapter_config.max_cache_size = 1000;      // 1000 DEKs
adapter_config.enable_caching = true;      // Enable caching
```

### 5. High Availability

For production:
- Use HSM clustering/replication
- Configure automatic failover
- Test disaster recovery procedures
- Monitor HSM health continuously

## Troubleshooting

### HSM Not Found

**Error:**
```
HSM initialization failed: Failed to load PKCS#11 library
```

**Solution:**
1. Verify library path:
   ```bash
   ls -la /usr/lib/softhsm/libsofthsm2.so
   ```
2. Check library dependencies:
   ```bash
   ldd /usr/lib/softhsm/libsofthsm2.so
   ```
3. Set LD_LIBRARY_PATH if needed:
   ```bash
   export LD_LIBRARY_PATH=/usr/lib/softhsm:$LD_LIBRARY_PATH
   ```

### Invalid PIN

**Error:**
```
HSM initialization failed: CKR_PIN_INCORRECT
```

**Solution:**
1. Verify PIN is correct
2. Check token status:
   ```bash
   pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so --show-slots
   ```
3. Reset PIN if locked:
   ```bash
   softhsm2-util --init-pin --slot 0 --pin 1234 --so-pin 5678
   ```

### Key Not Found

**Error:**
```
HSM operation failed: Key label 'lora-adapter-kek' not found
```

**Solution:**
1. List available keys:
   ```bash
   pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so \
     --login --pin 1234 \
     --list-objects
   ```
2. Generate key if missing (see [Generate KEK](#generate-kek-in-softhsm))

### Performance Issues

**Symptom:** Slow encryption/decryption

**Solution:**
1. Check cache hit rate:
   ```cpp
   auto stats = adapter->getStats();
   double hit_rate = stats["cache_hit_rate"].get<double>();
   if (hit_rate < 0.80) {
       // Increase cache size or TTL
       adapter_config.cache_ttl_ms = 600000;  // 10 minutes
       adapter_config.max_cache_size = 2000;
   }
   ```
2. Increase session pool:
   ```cpp
   config.hsm_session_pool_size = 16;  // More parallel sessions
   ```
3. Monitor HSM utilization:
   ```bash
   # For Luna HSM
   lunacm
   > partition showinfo -partition themisdb
   ```

### Session Exhaustion

**Error:**
```
HSM operation failed: CKR_SESSION_COUNT
```

**Solution:**
1. Increase session pool size:
   ```cpp
   config.hsm_session_pool_size = 32;  // Increase limit
   ```
2. Close unused sessions in application code
3. Check HSM session limits (vendor-specific)

## References

- [PKCS#11 Specification v2.40](http://docs.oasis-open.org/pkcs11/pkcs11-base/v2.40/os/pkcs11-base-v2.40-os.html)
- [SoftHSM2 Documentation](https://github.com/opendnssec/SoftHSMv2)
- [ThemisDB Security Documentation](security_overview.md)
- [LoRA Storage Backend Documentation](../../LORA_STORAGE_BACKEND_COMPLETION.md)

## Support

For issues or questions:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Security Issues: security@themisdb.io
- Documentation: https://makr-code.github.io/ThemisDB/

---

**Last Updated**: April 2026  
**Maintained by**: ThemisDB Security Team
