# ThemisDB Core Framework Implementation

## Module Purpose

This directory contains the implementation code for ThemisDB's core framework and foundation layer. These implementations provide the runtime behavior for build configuration, edition management, licensing, module loading, and the wire protocol server.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `themis_db.cpp` | Main ThemisDB orchestration class |
| `query_router.cpp` | Query dispatch to appropriate engine |
| `module_coordinator.cpp` | Inter-module coordination and dependency management |
| `lifecycle_manager.cpp` | Startup and shutdown sequencing |

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

**Note:** This directory is currently empty as ThemisDB uses a monolithic build architecture. Implementation files will be added as part of the modularization effort (v1.7.0+).

In the current monolithic build, the functionality described here is distributed across:
- `src/core/` - Build and edition management
- `src/security/` - License validation
- `src/server/` - Wire protocol implementation

## Planned Implementation Files (v1.7.0+)

### Build & Edition Management

#### build_info.cpp
**Implementation of build information API**

Collects compile-time configuration and formats it for runtime access.

**Planned Functions:**
- `getBuildConfiguration()` - Aggregate build metadata
- `formatBuildInfo()` - Human-readable build summary
- `getVersionSummary()` - Compact version string
- `isModuleCompiledIn()` - Check module availability
- `getCompiledModules()` - List of compiled modules

---

#### license_info.cpp
**Implementation of license validation**

Validates embedded license data and verifies digital signatures.

**Planned Functions:**
- `getEmbeddedLicense()` - Extract embedded license
- `isLicenseValid()` - Validate expiry date
- `verifyLicenseSignature()` - Cryptographic verification

**Security Features:**
- RSA-2048 or Ed25519 signatures
- OpenSSL for signature verification
- Constant-time comparison for security

---

### Module Architecture

#### module_loader.cpp
**Secure module loading implementation**

Loads shared libraries with mandatory security verification.

**Planned Security Checks:**
1. SHA-256 hash verification
2. Digital signature verification (X.509 or GPG)
3. Whitelist/blacklist checking
4. Zone.Identifier detection (Windows)
5. Authenticode verification (Windows)
6. GPG signature verification (Linux)

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

## Current Architecture (Monolithic)

For now, these implementations are found in:

### Build Information
```
src/core/build_info.cpp (planned)
```

Currently handled by CMake-generated headers and runtime queries.

### License Management
```
src/security/license_manager.cpp (if exists)
```

Currently integrated with security module.

### Wire Protocol
```
src/server/wire_protocol_server.cpp (if exists)
```

Currently integrated with server module.

---

## Migration Plan

### Phase 1: Extract to Separate Files (v1.7.0)
- Move build info implementation to `src/themis/build_info.cpp`
- Move license implementation to `src/themis/license_info.cpp`
- Move wire protocol to `src/themis/wire_protocol_server.cpp`

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

**Planning Phase** (as of v1.5.0)

⏳ **Planned Implementations:**
- Build information (v1.7.0)
- License validation (v1.7.0)
- Module loading (v1.7.0)
- Wire protocol server (v1.7.0)

📝 **Current Status:**
- Functionality exists in monolithic build
- Refactoring planned for modular architecture
- Interfaces defined in `include/themis/`

---

## Related Documentation

- [Header Documentation](../../include/themis/README.md) - Public API
- [Future Enhancements](FUTURE_ENHANCEMENTS.md) - Planned features
- [Architecture](../../ARCHITECTURE.md) - System architecture
- [Modularization Plan](../../docs/architecture/MODULARIZATION_PLAN.md) - Module strategy

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
