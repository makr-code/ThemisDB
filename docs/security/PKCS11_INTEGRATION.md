# PKCS#11 Integration Guide

**Version:** 1.4.2  
**Last Updated:** April 2026  
**Classification:** Public  
**Related:** HSM Production Setup, Security Hardening

---

## Overview

ThemisDB uses PKCS#11 (Cryptoki) to interface with Hardware Security Modules (HSMs). This document explains the PKCS#11 integration strategy, limitations, and production requirements.

---

## Table of Contents

1. [PKCS#11 Header Strategy](#pkcs11-header-strategy)
2. [Minimal Header (Development)](#minimal-header-development)
3. [Vendor Headers (Production)](#vendor-headers-production)
4. [Compile-Time Validation](#compile-time-validation)
5. [HSM Vendor Configuration](#hsm-vendor-configuration)
6. [Migration Path](#migration-path)
7. [Troubleshooting](#troubleshooting)

---

## PKCS#11 Header Strategy

### Two-Tier Approach

ThemisDB uses a two-tier PKCS#11 header strategy:

1. **Development:** Minimal header (`pkcs11_minimal.h`) for testing
2. **Production:** Vendor-provided PKCS#11 headers

| Feature | Minimal Header | Vendor Header |
|---------|---------------|---------------|
| **Purpose** | Development, SoftHSM2 testing | Production HSM deployment |
| **Scope** | Basic sign/verify operations | Full PKCS#11 functionality |
| **Validation** | Limited | Vendor-specific extensions |
| **Support** | Community | Vendor support |

---

## Minimal Header (Development)

### Location

```
include/security/pkcs11_minimal.h
```

### Purpose

`pkcs11_minimal.h` is a **subset** of PKCS#11 types and constants for:
- Development without HSM vendor SDK
- Testing with SoftHSM2
- CI/CD pipeline builds
- Quick prototyping

### Limitations

⚠️ **WARNING:** This header is NOT suitable for production use!

**Missing Features:**
- Limited mechanism types (only RSA-PKCS, SHA256-RSA-PKCS, ECDSA)
- No vendor-specific extensions
- Limited attribute types
- No advanced key management functions
- No hardware-specific optimizations

**Supported Operations:**
- ✅ Basic signing (RSA, ECDSA)
- ✅ Basic verification
- ✅ Key discovery
- ✅ Session management
- ❌ Advanced key generation
- ❌ Key wrapping/unwrapping
- ❌ Hardware acceleration features
- ❌ Vendor-specific mechanisms

### Example Usage (Development Only)

```cpp
#ifndef THEMIS_USE_VENDOR_PKCS11
    #include "security/pkcs11_minimal.h"
#else
    #include <cryptoki.h>  // Vendor header
#endif

// Code works with both headers for basic operations
```

### Compile-Time Warnings

When building with `THEMIS_ENABLE_HSM_REAL` but without vendor headers:

```
warning: Using pkcs11_minimal.h with real HSM support. 
         Consider using vendor PKCS#11 headers for production.
```

---

## Vendor Headers (Production)

### Why Vendor Headers?

Production HSM deployments **MUST** use vendor-provided PKCS#11 headers because:

1. **Complete API Coverage**
   - Full PKCS#11 v2.40 (or later) compliance
   - All mechanisms and attributes
   - Vendor-specific extensions

2. **Hardware Compatibility**
   - Tested against actual hardware
   - Hardware-specific optimizations
   - Vendor support and updates

3. **Security Assurance**
   - Cryptographic validation
   - FIPS compliance verification
   - Vendor security certifications

4. **Support**
   - Vendor technical support
   - Bug fixes and patches
   - Compatibility guarantees

### Supported HSM Vendors

| Vendor | SDK Package | Header File | PKCS#11 Library |
|--------|-------------|-------------|-----------------|
| **Thales Luna** | Luna HSM Client | `cryptoki.h` | `libCryptoki2.so` |
| **AWS CloudHSM** | CloudHSM Client | `pkcs11.h` | `libcloudhsm_pkcs11.so` |
| **Utimaco** | CryptoServer SDK | `cryptoki.h` | `libcs_pkcs11_R2.so` |
| **nCipher** | nShield SDK | `pkcs11.h` | `libcknfast.so` |
| **SoftHSM2** | SoftHSM2 | `pkcs11.h` | `libsofthsm2.so` |

### Installation Examples

#### Thales Luna HSM

```bash
# Install Luna Client SDK
wget https://thalesdocs.com/gphsm/luna/7/docs/network/Content/sdk/luna_client_7.2.0.tar
tar -xf luna_client_7.2.0.tar
cd luna_client_7.2.0
./install.sh

# Headers installed to:
# /usr/include/cryptoki.h
# /usr/include/cryptoki_v2.h

# Library installed to:
# /usr/lib/libCryptoki2.so
```

#### AWS CloudHSM

```bash
# Install CloudHSM Client
wget https://s3.amazonaws.com/cloudhsmv2-software/CloudHsmClient/EL7/cloudhsm-client-latest.el7.x86_64.rpm
sudo rpm -ivh cloudhsm-client-latest.el7.x86_64.rpm

# Headers: /opt/cloudhsm/include/pkcs11.h
# Library: /opt/cloudhsm/lib/libcloudhsm_pkcs11.so
```

#### Utimaco CryptoServer

```bash
# Install CryptoServer SDK (requires license)
tar -xf cs_pkcs11_R2-<version>.tar.gz
cd cs_pkcs11_R2
./install.sh

# Headers: /usr/include/cryptoki.h
# Library: /usr/lib/libcs_pkcs11_R2.so
```

---

## Compile-Time Validation

### Static Assertions

`pkcs11_minimal.h` includes compile-time checks to detect mismatches:

```cpp
#ifdef THEMIS_ENABLE_HSM_REAL
    // Verify critical constants
    static_assert(CKR_OK == 0x00000000U, 
        "PKCS#11 CKR_OK mismatch - vendor header may be incompatible");
    
    static_assert(CKM_RSA_PKCS == 0x00000001U,
        "PKCS#11 CKM_RSA_PKCS mismatch - vendor header may be incompatible");
    
    static_assert(CKO_PRIVATE_KEY == 0x00000003U,
        "PKCS#11 CKO_PRIVATE_KEY mismatch - vendor header may be incompatible");
#endif
```

**Purpose:**
- Early detection of header incompatibilities
- Compile-time error if vendor header defines different values
- Prevents runtime failures from constant mismatches

### Build System Integration

```cmake
# CMakeLists.txt
if(THEMIS_ENABLE_HSM_REAL)
    if(THEMIS_USE_VENDOR_PKCS11)
        # Use vendor PKCS#11 headers
        find_path(PKCS11_INCLUDE_DIR 
            NAMES cryptoki.h pkcs11.h
            PATHS 
                /usr/include
                /opt/cloudhsm/include
                /usr/local/include
        )
        
        if(PKCS11_INCLUDE_DIR)
            target_include_directories(themis_core PRIVATE ${PKCS11_INCLUDE_DIR})
            target_compile_definitions(themis_core PRIVATE THEMIS_USE_VENDOR_PKCS11)
            message(STATUS "Using vendor PKCS#11 headers from ${PKCS11_INCLUDE_DIR}")
        else()
            message(WARNING "Vendor PKCS#11 headers not found, using minimal header")
        endif()
    else()
        message(WARNING "Using pkcs11_minimal.h - not recommended for production")
    endif()
endif()
```

---

## HSM Vendor Configuration

### Thales Luna HSM

```bash
# Build with Luna headers
cmake -B build \
  -DTHEMIS_ENABLE_HSM_REAL=ON \
  -DTHEMIS_USE_VENDOR_PKCS11=ON \
  -DPKCS11_INCLUDE_DIR=/usr/include \
  -DPKCS11_LIBRARY=/usr/lib/libCryptoki2.so
```

**Configuration:**
```yaml
# config/security.yaml
hsm:
  provider: pkcs11
  pkcs11:
    library_path: "/usr/lib/libCryptoki2.so"
    slot_id: 0
    pin: "${HSM_PIN}"
    key_label: "themis-master-key"
```

### AWS CloudHSM

```bash
# Build with CloudHSM headers
cmake -B build \
  -DTHEMIS_ENABLE_HSM_REAL=ON \
  -DTHEMIS_USE_VENDOR_PKCS11=ON \
  -DPKCS11_INCLUDE_DIR=/opt/cloudhsm/include \
  -DPKCS11_LIBRARY=/opt/cloudhsm/lib/libcloudhsm_pkcs11.so
```

**Configuration:**
```yaml
hsm:
  provider: pkcs11
  pkcs11:
    library_path: "/opt/cloudhsm/lib/libcloudhsm_pkcs11.so"
    slot_id: 0
    pin: "${HSM_PIN}"
    key_label: "themis-master-key"
```

### Utimaco CryptoServer

```bash
# Build with Utimaco headers
cmake -B build \
  -DTHEMIS_ENABLE_HSM_REAL=ON \
  -DTHEMIS_USE_VENDOR_PKCS11=ON \
  -DPKCS11_INCLUDE_DIR=/usr/include \
  -DPKCS11_LIBRARY=/usr/lib/libcs_pkcs11_R2.so
```

### SoftHSM2 (Testing Only)

```bash
# Build with SoftHSM2 for testing
cmake -B build \
  -DTHEMIS_ENABLE_HSM_REAL=ON \
  -DTHEMIS_USE_VENDOR_PKCS11=ON \
  -DPKCS11_LIBRARY=/usr/lib/softhsm/libsofthsm2.so
```

---

## Migration Path

### From Minimal Header to Vendor Header

**Step 1: Install Vendor SDK**
```bash
# Example: Thales Luna
wget https://thalesdocs.com/gphsm/luna/7/docs/network/Content/sdk/luna_client_7.2.0.tar
tar -xf luna_client_7.2.0.tar
cd luna_client_7.2.0
sudo ./install.sh
```

**Step 2: Locate Vendor Headers**
```bash
# Find installed headers
find /usr/include /opt -name "cryptoki.h" -o -name "pkcs11.h" 2>/dev/null
```

**Step 3: Update Build Configuration**
```bash
# Rebuild with vendor headers
cmake -B build \
  -DTHEMIS_ENABLE_HSM_REAL=ON \
  -DTHEMIS_USE_VENDOR_PKCS11=ON \
  -DPKCS11_INCLUDE_DIR=/usr/include \
  -DPKCS11_LIBRARY=/usr/lib/libCryptoki2.so

cmake --build build
```

**Step 4: Verify Build**
```bash
# Check for vendor header usage
grep "THEMIS_USE_VENDOR_PKCS11" build/compile_commands.json

# Should see: -DTHEMIS_USE_VENDOR_PKCS11
```

**Step 5: Test with Real HSM**
```bash
# Initialize HSM
# Create test key
# Run HSM tests
./build/tests/test_hsm_provider
```

### Validation Checklist

- [ ] Vendor SDK installed
- [ ] Vendor headers located
- [ ] Build succeeds with vendor headers
- [ ] No "using minimal header" warnings
- [ ] Static assertions pass (no constant mismatches)
- [ ] HSM tests pass with real hardware
- [ ] Production configuration tested

---

## Troubleshooting

### Header Not Found

**Error:**
```
fatal error: cryptoki.h: No such file or directory
```

**Solution:**
```bash
# Install vendor SDK
# Verify header location
find /usr /opt -name "cryptoki.h" 2>/dev/null

# Specify include directory
cmake -DPKCS11_INCLUDE_DIR=/path/to/headers
```

### Static Assertion Failed

**Error:**
```
static assertion failed: PKCS#11 CKR_OK mismatch - vendor header may be incompatible
```

**Cause:** Vendor header defines different constant values

**Solution:**
1. Verify vendor SDK version
2. Check PKCS#11 specification version
3. Contact vendor support
4. Consider updating minimal header definitions

### Symbol Not Found at Runtime

**Error:**
```
undefined symbol: C_GetFunctionList
```

**Solution:**
```bash
# Check library path
ldd ./build/themisdb | grep pkcs11

# Set LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH

# Or specify library path in config
hsm:
  pkcs11:
    library_path: "/usr/lib/libCryptoki2.so"
```

### Incompatible Vendor Extensions

**Error:**
```
error: 'CKM_VENDOR_SPECIFIC' was not declared in this scope
```

**Solution:**
- Vendor-specific mechanisms require vendor headers
- Disable vendor-specific features when using minimal header
- Use `#ifdef THEMIS_USE_VENDOR_PKCS11` guards

---

## Best Practices

### Development

✅ **Do:**
- Use `pkcs11_minimal.h` for quick development
- Test with SoftHSM2 before HSM hardware
- Guard vendor-specific code with `#ifdef`

❌ **Don't:**
- Deploy minimal header to production
- Rely on minimal header for complete functionality
- Ignore vendor SDK documentation

### Production

✅ **Do:**
- Always use vendor-provided headers
- Install complete vendor SDK
- Test thoroughly with target HSM hardware
- Document vendor SDK version
- Keep vendor SDK updated

❌ **Don't:**
- Use minimal header in production
- Mix headers from different vendors
- Skip vendor SDK installation

---

## Related Documentation

- [HSM Production Setup](HSM_PRODUCTION_SETUP.md)
- [Security Configuration](../../config/security.yaml)
- [HSM Provider Implementation](../../src/security/hsm_provider_pkcs11.cpp)
- [OASIS PKCS#11 Specification](https://docs.oasis-open.org/pkcs11/pkcs11-base/v2.40/os/pkcs11-base-v2.40-os.html)

---

## Summary

### Development Strategy

| Stage | Header | Purpose | Build Flag |
|-------|--------|---------|------------|
| **Local Dev** | `pkcs11_minimal.h` | Quick iteration | Default |
| **CI/CD** | `pkcs11_minimal.h` | Automated testing | Default |
| **Staging** | Vendor header | Pre-production validation | `-DTHEMIS_USE_VENDOR_PKCS11=ON` |
| **Production** | Vendor header | Live deployment | `-DTHEMIS_USE_VENDOR_PKCS11=ON` |

### Key Takeaways

1. **Minimal header = Development only**
2. **Vendor header = Production requirement**
3. **Compile-time validation = Early error detection**
4. **HSM vendor SDK = Must be installed**
5. **Testing = Critical with real hardware**

For production HSM deployments, vendor headers are **mandatory**.
