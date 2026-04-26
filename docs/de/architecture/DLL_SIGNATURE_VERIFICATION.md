# DLL Signature Verification for Modular ThemisDB

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🧩 Architecture

---

## 📑 Table of Contents

- [Overview](#overview)
- [The Problem](#the-problem)
- [The Solution](#the-solution-moduleloader-with-security-verification)
- [Architecture](#architecture)
- [Key Components](#key-components)
- [Security Features](#security-features)

To prevent corrupted or malicious DLL loading in the modular architecture, ThemisDB implements a comprehensive security verification system for all `themis_*` modules.

## The Problem

When splitting `themis_core` into modular DLLs, we introduce new attack vectors:
- **DLL Corruption**: File corruption during transfer or storage
- **DLL Tampering**: Malicious modification of module files
- **DLL Substitution**: Replacement with malicious lookalike DLLs
- **Man-in-the-Middle**: Compromised DLLs during distribution

## The Solution: ModuleLoader with Security Verification

### Architecture

```
                    ┌─────────────────────┐
                    │  Application Start  │
                    └──────────┬──────────┘
                               │
                    ┌──────────▼──────────┐
                    │   ModuleLoader      │
                    └──────────┬──────────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
     ┌────────▼────────┐ ┌───▼────┐  ┌────────▼────────┐
     │ Hash Verification│ │Signature│  │  Blacklist/     │
     │   (SHA-256)      │ │Verify   │  │  Whitelist      │
     └────────┬────────┘ └───┬────┘  └────────┬────────┘
              │              │                 │
              └──────────────┼─────────────────┘
                             │
                    ┌────────▼────────┐
                    │  Load Module    │
                    │  (if verified)  │
                    └─────────────────┘
```

### Key Components

#### 1. ModuleLoader (`include/themis/base/module_loader.h`)
Main interface for loading verified modules:
```cpp
themis::modules::ModuleLoader loader;

// Load a single module with verification
auto result = loader.loadModule("themis_storage.dll", "themis_storage");
if (result.success) {
    // Module verified and loaded successfully
} else {
    // Verification failed - module NOT loaded
    spdlog::critical("Security: {}", result.errorMessage);
}

// Load all modules from directory
size_t loaded = loader.loadAllModules("/path/to/modules");
```

#### 2. ModuleSecurityVerifier
Wraps existing `PluginSecurityVerifier` for module-specific verification:
- SHA-256 hash calculation
- Digital signature verification
- Certificate chain validation
- Policy enforcement

#### 3. ModuleRegistry
Global singleton tracking all loaded modules:
- Prevents duplicate loading
- Provides system-wide status
- Audit trail of loaded modules

### Security Features

#### ✅ Hash Verification (SHA-256)
Every module file is hashed before loading:
```cpp
std::string fileHash = verifier->calculateFileHash("themis_storage.dll");
// Compare with expected hash from metadata
if (fileHash != expectedHash) {
    // REJECT: File has been modified/corrupted
}
```

#### ✅ Digital Signature Verification
Modules are signed with X.509 certificates:
```json
{
  "plugin": {
    "name": "themis_storage",
    "version": "1.4.0",
    "signature": {
      "sha256": "a1b2c3d4...",
      "signature": "e5f6g7h8...",
      "certificate": "-----BEGIN CERTIFICATE-----\n...",
      "issuer": "CN=ThemisDB Official, O=ThemisDB, C=DE",
      "subject": "CN=themis_storage",
      "timestamp": 1702828800
    }
  }
}
```

**Verification Process**:
1. Extract public key from X.509 certificate
2. Verify certificate hasn't expired
3. Verify certificate issuer is trusted
4. Verify signature using public key
5. Reject if any step fails

#### ✅ Certificate Chain Validation
- Verify issuer is in trusted list
- Check certificate expiration
- Optional: CRL (Certificate Revocation List) check
- Optional: OCSP (Online Certificate Status Protocol)

#### ✅ Blacklist/Whitelist Support
**Blacklist** - Block known malicious hashes:
```cpp
loader.addBlacklistedHash("abc123..."); // Blocked
```

**Whitelist** - Always allow known good hashes:
```cpp
loader.addWhitelistedHash("def456..."); // Trusted
```

#### ✅ Audit Logging
All module load attempts are logged:
```cpp
// Export security audit log
loader.exportAuditLog("/var/log/themis/module_audit.json");
```

**Logged Events**:
- `PLUGIN_LOADED` - Module loaded successfully
- `PLUGIN_LOAD_FAILED` - Module load failed
- `PLUGIN_UNLOADED` - Module unloaded
- `SIGNATURE_VERIFIED` - Signature verification passed
- `SIGNATURE_VERIFICATION_FAILED` - Signature invalid
- `HASH_MISMATCH` - File hash doesn't match metadata
- `BLACKLISTED` - Module on blacklist
- `UNTRUSTED_ISSUER` - Certificate issuer not trusted
- `CERTIFICATE_EXPIRED` - Certificate has expired
- `POLICY_VIOLATION` - Security policy violated

### Security Modes

#### Production Mode (NDEBUG defined)
```cpp
// Strict security - unsigned modules rejected
policy.requireSignature = true;
policy.allowUnsigned = false;
policy.verifyFileHash = true;
policy.checkRevocation = true;
policy.minTrustLevel = PluginTrustLevel::TRUSTED;
```

**Behavior**:
- ✅ Signature required for all modules
- ✅ Hash verification mandatory
- ✅ Certificate revocation check enabled
- ❌ Unsigned modules rejected
- ❌ Untrusted issuers rejected

#### Development Mode (Debug builds)
```cpp
// Relaxed security - unsigned modules allowed
policy.requireSignature = false;
policy.allowUnsigned = true;
policy.verifyFileHash = true;
policy.checkRevocation = false;
policy.minTrustLevel = PluginTrustLevel::UNTRUSTED;
```

**Behavior**:
- ⚠️ Signature optional (but verified if present)
- ✅ Hash verification still performed
- ❌ Revocation check disabled (performance)
- ✅ Unsigned modules allowed (development)
- ⚠️ Untrusted issuers accepted (local testing)

### Integration with Existing Infrastructure

The `ModuleLoader` reuses ThemisDB's existing security infrastructure:

```cpp
// Existing plugin security (src/acceleration/plugin_security.cpp)
themis::acceleration::PluginSecurityVerifier pluginVerifier;
themis::acceleration::PluginSecurityAuditor auditor;

// New module loader wraps existing infrastructure
themis::modules::ModuleSecurityVerifier moduleVerifier;
// ↓ delegates to ↓
pluginVerifier.verifyPlugin(modulePath, error);
```

**Benefits**:
- ✅ Code reuse (no duplication)
- ✅ Consistent security across plugins and modules
- ✅ Existing audit infrastructure
- ✅ Proven cryptographic implementation

## Usage Examples

### Basic Module Loading
```cpp
#include "themis/base/module_loader.h"

int main() {
    themis::modules::ModuleLoader loader;
    
    // Load core modules
    auto storage = loader.loadModule("themis_storage.dll", "themis_storage");
    auto query = loader.loadModule("themis_query.dll", "themis_query");
    auto security = loader.loadModule("themis_security.dll", "themis_security");
    
    if (!storage.success || !query.success || !security.success) {
        spdlog::critical("Failed to load core modules - cannot start");
        return 1;
    }
    
    // Application continues...
}
```

### Batch Module Loading
```cpp
#include "themis/base/module_loader.h"

int main() {
    themis::modules::ModuleLoader loader;
    
    // Load all modules from directory
    size_t loaded = loader.loadAllModules("/opt/themisdb/lib");
    
    if (loaded < 5) {  // Minimum 5 core modules required
        spdlog::critical("Insufficient modules loaded: {} < 5", loaded);
        return 1;
    }
    
    // Check specific module
    if (!loader.isModuleLoaded("themis_storage")) {
        spdlog::critical("Required module 'themis_storage' not loaded");
        return 1;
    }
    
    // Application continues...
}
```

### Custom Security Policy
```cpp
#include "themis/base/module_loader.h"

int main() {
    themis::modules::ModuleLoader loader;
    
    // Production deployment - strict security
    loader.setRequireSignature(true);
    loader.setAllowUnsigned(false);
    
    // Whitelist known good modules (from secure build)
    loader.addWhitelistedHash("a1b2c3d4e5f6g7h8...");  // themis_storage v1.4.0
    loader.addWhitelistedHash("i9j0k1l2m3n4o5p6...");  // themis_query v1.4.0
    
    // Blacklist known compromised module (security incident)
    loader.addBlacklistedHash("deadbeef12345678...");  // Compromised build
    
    // Load modules
    size_t loaded = loader.loadAllModules("/opt/themisdb/lib");
    
    // Export audit log for compliance
    loader.exportAuditLog("/var/log/themis/module_audit.json");
}
```

### Module Information
```cpp
#include "themis/base/module_loader.h"

void printModuleStatus(const themis::modules::ModuleLoader& loader) {
    auto modules = loader.getAllLoadedModules();
    
    for (const auto& module : modules) {
        spdlog::info("Module: {}", module.name);
        spdlog::info("  Path: {}", module.path);
        spdlog::info("  Version: {}", module.version);
        spdlog::info("  Hash: {}", module.fileHash);
        spdlog::info("  Verified: {}", module.verified);
        spdlog::info("  Load Time: {}", module.loadTime);
    }
}
```

## File Organization

```
include/themis/base/
  module_loader.h         # Public API for module loading
  export.h                # DLL export macros for all modules

src/base/
  module_loader.cpp       # Implementation with security verification

src/acceleration/
  plugin_security.h       # Existing plugin security (reused)
  plugin_security.cpp     # X.509, SHA-256, signature verification
  plugin_loader.h         # Existing plugin loader
  plugin_loader.cpp       # Implementation
```

## Security Guarantees

### ✅ What is Prevented
- **Corrupted DLLs**: Hash mismatch detected before loading
- **Tampered DLLs**: Signature verification fails
- **Malicious DLLs**: Blacklist blocks known bad hashes
- **Expired Certificates**: Rejected automatically
- **Untrusted Issuers**: Not in trusted issuer list
- **Unsigned DLLs (Production)**: Rejected in production builds

### ⚠️ What is NOT Prevented
- **Zero-day malware**: Unknown malicious code (requires AV integration)
- **Compromised signing key**: If attacker has private key
- **Social engineering**: User manually disabling verification
- **Memory corruption**: After DLL is loaded (use ASLR/DEP)

## Performance Impact

- **Hash Calculation**: ~2-5ms per module (SHA-256)
- **Signature Verification**: ~10-20ms per module (RSA-2048)
- **Total Overhead**: ~50-100ms for 5 core modules
- **Memory**: ~2KB per loaded module (metadata)

**Conclusion**: Minimal impact (~100ms startup time) for significant security gain

## Future Enhancements

- [ ] Code signing with EV certificates (Windows)
- [ ] Timestamping service integration
- [ ] CRL/OCSP automatic updates
- [ ] Module update verification
- [ ] Rollback protection (version pinning)
- [ ] Hardware Security Module (HSM) signing
- [ ] Reproducible builds with hash verification

## References

- **ModuleLoader API**: `include/themis/base/module_loader.h`
- **Implementation**: `src/base/module_loader.cpp`
- **Plugin Security**: `src/acceleration/plugin_security.cpp`
- **Modularization Plan**: `docs/architecture/MODULARIZATION_PLAN.md`
- **X.509 Certificates**: https://www.rfc-editor.org/rfc/rfc5280
- **Code Signing**: https://docs.microsoft.com/en-us/windows/win32/seccrypto/cryptography-tools

## Conclusion

The `ModuleLoader` with security verification ensures that only trusted, unmodified modules are loaded into ThemisDB. This prevents corrupted or malicious DLL injection and provides a strong security foundation for the modular architecture.

**Status**: ✅ Implemented and ready for post-v1.3.0 modular build  
**Production Ready**: Yes (with mandatory signatures)  
**Development Friendly**: Yes (with optional signatures)  
**Zero Trust**: All modules verified before loading
