> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# ThemisDB Core Framework Implementation

## Module Purpose

This directory contains the implementation code for ThemisDB's core framework and foundation layer. These implementations provide the runtime behavior for build configuration, edition management, licensing, module loading, and the wire protocol server.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `build_info.cpp` | Build metadata collection and formatting |
| `edition_manager.cpp` | License tier management and feature gates |
| `license_info.cpp` | License validation and Ed25519 signature verification |
| `module_loader.cpp` | Platform-independent secure module loading core |
| `module_loader_linux.cpp` | Linux-specific GPG/ELF/xattr verification |
| `module_loader_win32.cpp` | Windows Zone.Identifier and Authenticode verification |
| `module_hash_verifier.cpp` | SHA-256 manifest integrity verification |
| `module_signature_verifier.cpp` | Authenticode/GPG signature verification |
| `module_security.cpp` | `ModuleSecurityVerifier` — trust-level enforcement |
| `module_dependency_resolver.cpp` | Module dependency graph and topological load-order |
| `wire_protocol_server.cpp` | Binary wire protocol server (`themis::wire` namespace) |

## Scope

**In Scope:**
- Build information collection and formatting
- License validation and signature verification
- Module loading with security verification
- Wire protocol server implementation
- Platform-specific module loading (Windows/Linux)

**Out of Scope:**
- Public interfaces (see `../../include/themis/`)
- Business logic implementations (other modules)
- Domain-specific functionality

## Implementation Status

This directory is active and contains Themis core implementation files.

Current implementation files in `src/themis/`:
- `build_info.cpp`
- `edition_manager.cpp`
- `license_info.cpp`
- `module_dependency_resolver.cpp`
- `module_hash_verifier.cpp`
- `module_loader.cpp`
- `module_loader_linux.cpp`
- `module_loader_win32.cpp`
- `module_security.cpp`
- `module_signature_verifier.cpp`
- `wire_protocol_server.cpp`

## Implemented Core Files (v1.7.0+)

### Build & Edition Management

#### build_info.cpp
**Implementation of build information API**

Collects compile-time configuration and formats it for runtime access.

**Implemented Functions:**
- `getBuildConfiguration()` — aggregate build metadata at runtime
- `getReproducibilityInfo()` — reproducibility info (git HEAD, branch, dirty flag)
- `exportBuildManifest()` / `verifyBuildManifest()` — build manifest round-trip
- `formatBuildInfo()` — human-readable build summary
- `isModuleCompiledIn()` — check module availability at runtime
- `getCompiledModules()` — list compiled modules

---

#### license_info.cpp
**Implementation of license validation**

Validates embedded license data and verifies digital signatures.

**Implemented Functions:**
- `getEmbeddedLicense()` — extract embedded license
- `isLicenseValid()` — validate expiry and tier
- `verifyLicenseSignature()` — Ed25519 cryptographic verification
- `LicenseClient` — remote license registration/activation
- `LicenseInfo::remaining_grace_days()` — remaining grace period

**Security Features:**
- RSA-2048 or Ed25519 signatures
- OpenSSL for signature verification
- Constant-time comparison for security

---

### Module Architecture

#### module_loader.cpp
**Secure module loading implementation**

Loads shared libraries with mandatory security verification.

**Implemented Security Checks:**
1. SHA-256 hash verification (`ModuleHashVerifier`)
2. Digital signature verification — Authenticode (Windows) / GPG (Linux)
3. Whitelist/blacklist checking (`ModuleSecurityVerifier`)
4. Zone.Identifier detection (Windows, `module_loader_win32.cpp`)
5. Authenticode verification (Windows)
6. GPG signature verification via `posix_spawn` (Linux)

**Platform Support:**
- Windows: `LoadLibraryEx`, `GetProcAddress`, `FreeLibrary`
- Linux: `dlopen`, `dlsym`, `dlclose`

---

### Network Protocol

#### wire_protocol_server.cpp
**Binary wire protocol server implementation**

Implements the binary TCP protocol for high-performance client connections.

**Key Components:**
- TCP server with Boost.Asio
- Connection handler with session management
- Message dispatcher with OpCode routing
- LZ4 compression support
- ChaCha20-Poly1305 encryption support

---

## Runtime Behavior, Error Cases, and Limits

- **Startup path**: build/edition/license metadata is evaluated before module activation; unresolved dependency graphs or failed verifier checks block module startup.
- **Module loading errors** (`module_loader*.cpp`): hash mismatch, signature failures, missing exports, or platform loader failures result in explicit load failures (no partial activation).
- **License and feature gate errors** (`license_info.cpp`, `edition_manager.cpp`, `runtime_license_gate` integration): invalid/expired licenses deny restricted features while allowing permitted baseline functionality.
- **Wire protocol limits** (`wire_protocol_server.cpp`): malformed frames, unsupported opcodes, checksum mismatches, or payloads above limits are rejected.
- **Concurrency limit**: current wire protocol server implementation expects a single-threaded `io_context` unless guarded by external synchronization (tracked in `ROADMAP.md` Known Issues).

## Usage Snippets

### Module verification before startup

```cpp
#include "themis/base/module_loader.h"

themis::modules::ModuleLoader loader;
loader.setRequireSignature(true);

auto result = loader.loadModule("/opt/themis/modules/themis_query.so", "themis_query");
if (!result.success) {
    std::cerr << "Module load failed: " << result.errorMessage << std::endl;
    return;
}
```

### Runtime gate check

```cpp
#include "themis/runtime_license_gate.h"

auto decision = themis::license::RuntimeLicenseGate::instance().checkFeature("rbac");
if (!decision.allowed) {
    std::cerr << "Feature denied: " << decision.message() << std::endl;
}
```

## Troubleshooting

- **Module fails to load at runtime**: verify manifest hash/signature material and platform trust settings (`module_loader_linux.cpp` / `module_loader_win32.cpp` paths differ).
- **Feature unexpectedly denied**: inspect embedded license fields and edition gate results (`edition_manager.cpp`, runtime gate checks).
- **Wire client gets immediate error frame**: confirm opcode, frame header version/magic, payload length, and checksum behavior match `wire_protocol_server` expectations.
- **Build metadata mismatch across environments**: compare configure-time definitions consumed by `build_info.cpp` and ensure consistent CMake preset usage.

---

## Current Architecture

The Themis implementation lives in `src/themis/`. Legacy or compatibility paths
may still exist in other subsystems.

### Build Information
`src/themis/build_info.cpp`

### License Management
`src/themis/license_info.cpp`

### Wire Protocol
`src/themis/wire_protocol_server.cpp`

---

## Migration Plan

### Phase 1: Extract to Separate Files (v1.7.0)
- Move build info implementation to `src/themis/build_info.cpp` ✅
- Move license implementation to `src/themis/license_info.cpp` ✅
- Move wire protocol to `src/themis/wire_protocol_server.cpp` ✅

### Phase 2: Create themis-base Library (v1.7.0)
- Build `libthemis-base.so` / `themis-base.dll`
- Export symbols with `THEMIS_BASE_API`
- Link other modules against themis-base

### Phase 3: Module Loader Implementation (v1.7.0)
- Implement `src/themis/module_loader.cpp`
- Add platform-specific loading
- Add signature verification
- Add security auditing

### Phase 4: Module Refactoring (v1.8.0)
- Refactor other modules to use module loader
- Convert to shared libraries
- Update build system for modular builds

---

## Architecture Patterns

### Implementation Guidelines

1. **Platform Abstraction**
   - Use `#ifdef` for platform-specific code
   - Provide consistent API across platforms
   - Test on both Windows and Linux

2. **Error Handling**
   - Use exceptions for exceptional conditions
   - Return `Result<T, Error>` for expected failures
   - Log all errors with context

3. **Resource Management**
   - RAII for all resources
   - Smart pointers for dynamic allocation
   - Explicit cleanup in destructors

4. **Security**
   - Constant-time comparisons for secrets
   - Secure memory wiping for sensitive data
   - Input validation on all external data
   - Defense in depth

5. **Performance**
   - Zero-copy where possible
   - Async I/O for network operations
   - Batch operations when available
   - Profile hot paths

---

## Testing

### Unit Tests

Planned location: `tests/themis/`:
- `test_build_info.cpp` - Build info API
- `test_license_info.cpp` - License validation
- `test_module_loader.cpp` - Module loading
- `test_wire_protocol.cpp` - Wire protocol

### Integration Tests

Planned location: `tests/integration/themis/`:
- `test_module_loading_integration.cpp` - End-to-end module loading
- `test_wire_protocol_integration.cpp` - Wire protocol with real clients

### Security Tests

Planned location: `tests/security/themis/`:
- `test_module_tampering.cpp` - Detect tampered modules
- `test_signature_verification.cpp` - Signature validation
- `test_zone_identifier.cpp` - Zone.Identifier handling

---

## Dependencies

### Internal Dependencies
- None (foundation module)

### External Dependencies
- **Boost.Asio** - Network I/O
- **Protocol Buffers** - Wire protocol serialization
- **OpenSSL** - Cryptography (hashing, signatures)
- **LZ4** - Compression
- **gpgme** (Linux) - GPG signature verification

---

## Build Configuration (Future)

### CMake Build (Planned v1.7.0)

```cmake
# themis-base library
add_library(themis-base SHARED
    src/themis/build_info.cpp
    src/themis/license_info.cpp
    src/themis/module_loader.cpp
)

target_include_directories(themis-base PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(themis-base PRIVATE
    THEMIS_BASE_EXPORTS
)

target_link_libraries(themis-base
    OpenSSL::Crypto
    Boost::system
    ${CMAKE_DL_LIBS}  # dlopen/dlclose
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(themis-base wintrust crypt32)
endif()

if(UNIX)
    target_link_libraries(themis-base gpgme pthread)
endif()

# themis-network library (wire protocol)
add_library(themis-network SHARED
    src/themis/wire_protocol_server.cpp
)

target_compile_definitions(themis-network PRIVATE
    THEMIS_NETWORK_EXPORTS
)

target_link_libraries(themis-network
    themis-base
    Boost::asio
    protobuf::libprotobuf
    lz4::lz4
)
```

---

## Performance Considerations

### Module Loading
- Module loading is I/O bound (file reads)
- Signature verification is CPU bound (cryptography)
- Cache verified modules to avoid re-verification
- Parallel module loading for faster startup

### Wire Protocol
- Zero-copy where possible
- Connection pooling for efficiency
- Batch small messages to reduce overhead
- Compression for large payloads
- Keep-alive to reduce connection overhead

---

## Security Considerations

### Module Loading
1. **Always verify signatures in production**
2. **Use whitelist when possible** (faster than signature verification)
3. **Check blacklist first** (fail fast for known-bad modules)
4. **Log all load attempts** (audit trail)
5. **Restrict module directories** (prevent loading from untrusted locations)

### Wire Protocol
1. **Always use TLS in production** (wire protocol is plaintext)
2. **Require authentication** (no anonymous access)
3. **Rate limit connections** (prevent DoS)
4. **Validate all inputs** (prevent injection attacks)
5. **Set maximum payload size** (prevent memory exhaustion)

---

## Status

**Implemented** (as of v1.8.0)

✅ **Delivered in `src/themis/`:**
- Build information and reproducibility support
- License validation and runtime license gating integration
- Secure module loading (core + Linux + Windows split)
- Module hash and signature verification
- Module dependency resolution
- Wire protocol server implementation

📝 **Current Status:**
- `src/themis/` is the active implementation location for Themis core files
- Public interfaces are defined in `include/themis/`
- Additional modularization and hardening remain tracked in `ROADMAP.md`

---

## Related Documentation

- [Header Documentation](../../include/themis/README.md) - Public API
- [Roadmap](ROADMAP.md) - Implementation phases, known issues, and breaking-change policy
- [Future Enhancements](FUTURE_ENHANCEMENTS.md) - Planned features
- [Architecture](../../ARCHITECTURE.md) - System architecture
- [Modularization Plan (DE)](../../docs/de/architecture/MODULARIZATION_PLAN.md) - Module strategy

---

## Contributing

When contributing to Themis implementations:

1. **Security Review Required**: All code must pass security review
2. **Platform Testing**: Test on Windows and Linux
3. **Performance Testing**: Benchmark critical paths
4. **Documentation**: Update this README for new implementations
5. **Tests**: Add unit and integration tests
6. **Error Handling**: Comprehensive error handling

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

---

## See Also

- [Public Headers](../../include/themis/README.md) - Public API documentation
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned enhancements
- [Architecture Decision Records](../../docs/architecture/) - Design decisions

## Scientific References

1. Gamma, E., Helm, R., Johnson, R., & Vlissides, J. (1994). **Design Patterns: Elements of Reusable Object-Oriented Software**. Addison-Wesley. ISBN: 978-0-201-63361-0

2. Fowler, M. (2004). **Inversion of Control Containers and the Dependency Injection Pattern**. martinfowler.com. https://martinfowler.com/articles/injection.html

3. Kleppmann, M. (2017). **Designing Data-Intensive Applications**. O'Reilly Media. ISBN: 978-1-449-37332-0

4. Stonebraker, M., & Hellerstein, J. M. (Eds.). (1994). **Readings in Database Systems (3rd ed.)**. Morgan Kaufmann. ISBN: 978-1-558-60252-9

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/themis/README.md`](../../include/themis/README.md) for the public API.
