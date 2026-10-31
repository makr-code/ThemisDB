---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "themisdb-doc-orchestrator"
Human_Review: false
Human_Author: "unknown"
---

> **Validation notice (2026-10-31):** Public headers are authoritative for
> signatures and behavior. Some older examples and maturity statements in this
> overview are illustrative, not build- or runtime-validated. In particular,
> verify security policy, wire feature integration, edition selection, and
> CMake target names against the linked headers and current CMake files before
> relying on examples.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# ThemisDB Core Framework Headers

## Module Purpose

This directory contains the fundamental headers for ThemisDB's core framework and foundation layer. These headers define build configuration, edition management, licensing, module loading, cross-cutting concerns, and the network wire protocol - essential infrastructure that all other modules depend on.

## Scope

**In Scope:**
- Build and edition information (compile-time metadata)
- License information and validation
- Module loading and security verification
- Export macros for modular architecture
- Base interfaces for dependency inversion
- Wire protocol server (binary TCP protocol)

**Out of Scope:**
- Implementation details (see `../../src/`)
- Business logic and domain-specific functionality
- Protocol-specific implementations

## Key Components

### Build & Edition Management

#### build_info.h
**Runtime access to compile-time build configuration**

Provides runtime APIs to query which modules were compiled, compiler version, build flags, and edition settings.

**Key Types:**
- `ModuleInfo` - Information about a compiled module
- `BuildConfiguration` - Complete build configuration snapshot
- Functions for querying module availability

**Features:**
- Module compilation status
- Edition information (COMMUNITY, ENTERPRISE, HYPERSCALER)
- Compiler and build type information
- Compile-time flags inspection
- Version summary for diagnostics

**Usage:**
```cpp
#include "themis/build_info.h"

auto config = themis::build_info::getBuildConfiguration();
std::cout << "Edition: " << config.edition_name << std::endl;
std::cout << "Compiler: " << config.compiler << std::endl;

if (themis::build_info::isModuleCompiledIn("themis_llm")) {
    // LLM module is available
}

// Print comprehensive build info at startup
std::cout << themis::build_info::formatBuildInfo(config) << std::endl;
```

---

#### edition.h
**Compile-time edition configuration and feature gating**

`edition.h` declares five edition enum values and compile-time constants.
Edition support is currently inconsistent across the CMake sources: the root
edition defaults and checked-in presets include `MILITARY`, while
`cmake/CMakeLists.txt` contains a four-edition validation that excludes it.
Until that registration conflict is resolved, do not infer successful
`MILITARY` configuration from the header or preset alone.

**Editions:**
- **MINIMAL**: Lightweight/embedded (0 GB GPU VRAM cap — CPU fallback, 1 node)
- **COMMUNITY**: Free, open-source (16 GB GPU VRAM — 1× Tesla T4, up to 5 nodes)
- **ENTERPRISE**: Paid subscription (320 GB GPU VRAM — 4× A100 80 GB, up to 100 nodes)
- **MILITARY**: Hardened/air-gapped (80 GB GPU VRAM — 2× A100 40 GB, up to 50 nodes)
- **HYPERSCALER**: OEM/Custom (unlimited VRAM and nodes)

**Key Types:**
- `EditionType` - Edition enumeration
- `EditionInfo` - Complete edition metadata
- Constexpr functions for compile-time edition checks

**Feature Flags:**
- `FEATURE_ENTERPRISE_PLUGINS` - Plugin loading capability
- `FEATURE_MULTI_MASTER` - Active-active replication
- `FEATURE_FIELD_ENCRYPTION` - Column-level encryption
- `FEATURE_RBAC` - Role-based access control
- `FEATURE_HSM` - Hardware security module integration

**Hardware Constraints:**
- `GPU_MAX_VRAM_GB` - Maximum GPU memory (0 / 16 / 80 / 320 / 0=unlimited)
- `SHARDING_MAX_NODES` - Maximum cluster nodes (1 / 5 / 50 / 100 / 0=unlimited)

**Usage:**
```cpp
#include "themis/edition.h"

using namespace themis::edition;

// Compile-time checks
static_assert(IsEdition<EditionType::ENTERPRISE>(),
              "Enterprise edition required");

// Runtime checks
if (FEATURE_FIELD_ENCRYPTION) {
    // Enable field encryption
}

// Get edition info
auto info = EditionInfo::Get();
std::cout << "Edition: " << info.name << std::endl;
std::cout << "Max VRAM: " << info.gpu_max_vram_gb << "GB" << std::endl;
std::cout << "Max Nodes: " << info.sharding_max_nodes << std::endl;

// Check specific features
if (IsFeatureEnabled("rbac")) {
    // Enable RBAC
}
```

---

#### license_info.h
**Embedded license data and validation**

Provides runtime access to license information embedded during build. Used for license validation and compliance tracking.

**Key Types:**
- `LicenseData` - Complete license information
- Functions for license validation and expiry checking

**License Fields:**
- Organization name and ID
- Contact email
- License key and edition
- Validity period (issued/expiry dates)
- Resource limits (nodes, cores, storage)
- Build ID and timestamp
- Digital signature for verification

**Features:**
- License data embedding at build time
- Expiry date validation
- Days-until-expiry calculation
- Digital signature verification
- Human-readable license summaries

**Usage:**
```cpp
#include "themis/license_info.h"

using namespace themis::license;

// Check for embedded license
if (hasEmbeddedLicense()) {
    auto license = getEmbeddedLicense().value();

    // Validate license
    if (isLicenseValid(license)) {
        std::cout << "Licensed to: " << license.organization_name << std::endl;
        std::cout << "Edition: " << license.edition << std::endl;

        int days = getDaysUntilExpiry(license);
        if (days > 0 && days < 30) {
            std::cerr << "Warning: License expires in " << days << " days" << std::endl;
        }

        // Verify signature
        if (!verifyLicenseSignature(license)) {
            std::cerr << "Error: Invalid license signature" << std::endl;
        }
    } else {
        std::cerr << "Error: License expired" << std::endl;
    }

    // Print license info at startup
    std::cout << formatLicenseInfo(license) << std::endl;
}
```

---

### Module Architecture

#### export.h
**DLL export/import macros for modular builds**

Cross-platform macros for exporting symbols from shared libraries (DLLs on Windows, .so files on Linux).

---

#### edition_manager.h
**Runtime edition management and feature-gate evaluation**

Provides APIs to check active edition at runtime and evaluate per-feature access gates.

---

#### module_hash_verifier.h
**SHA-256 module hash verification**

Verifies module binary hashes against a trusted manifest to detect tampering before load.

---

#### module_signature_verifier.h
**X.509 digital signature verification for modules**

Validates module code signatures using certificate chains; supports Authenticode (Windows) and GPG (Linux).

---

#### runtime_license_gate.h
**Runtime license enforcement gate**

Enforces license-derived feature restrictions at runtime; raises policy exceptions for unlicensed feature use.

---

**Platform Support:**
- Windows: `__declspec(dllexport/dllimport)`
- Linux/Unix: `__attribute__((visibility("default")))`

**Module-Specific Macros:**
- `THEMIS_BASE_API` - Core types and interfaces
- `THEMIS_STORAGE_API` - Storage engine
- `THEMIS_QUERY_API` - Query engine
- `THEMIS_SECURITY_API` - Encryption and security
- `THEMIS_NETWORK_API` - Network protocols
- `THEMIS_TRANSACTION_API` - Transaction management
- `THEMIS_SHARDING_API` - Distributed systems (optional)
- `THEMIS_LLM_API` - LLM integration (optional)
- `THEMIS_CONTENT_API` - Content processors (optional)
- `THEMIS_TIMESERIES_API` - Time-series (optional)
- `THEMIS_GRAPH_API` - Graph analytics (optional)
- `THEMIS_GEO_API` - Geospatial (optional)

**Usage:**
```cpp
#include "themis/export.h"

// Exporting a class from themis_storage module
class THEMIS_STORAGE_API StorageEngine {
public:
    bool put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
};

// Exporting a function from themis_query module
THEMIS_QUERY_API QueryResult executeQuery(const std::string& query);

// Exporting a template class (header-only, no export needed)
template<typename T>
class DataContainer {
    // Implementation in header
};
```

**Best Practices:**
- Use module-specific macros (not generic `THEMIS_EXPORT_MACRO`)
- Export classes, functions, and global variables that cross DLL boundaries
- Don't export template classes (they're header-only)
- Don't export internal helper classes

---

#### base/module_loader.h
**Secure module loading with signature verification**

Provides module loading with configurable security verification. The current
`ModuleSecurityVerifier` requires signatures and rejects unsigned modules in
`NDEBUG` builds; its non-`NDEBUG` defaults allow unsigned modules. Callers and
deployments must verify the effective policy rather than assume signature
verification is unconditional.

**Key Classes:**
- `ModuleLoader` - Main module loading class
- `ModuleSecurityVerifier` - Security verification engine
- `ModuleRegistry` - Singleton module registry
- `LoadedModule` - Loaded module metadata
- `ModuleVerificationResult` - Verification result

**Security Features:**
- SHA-256 hash verification
- Digital signature verification (X.509)
- Blacklist/whitelist support
- Certificate chain validation
- Audit logging of all load attempts
- Authenticode verification (Windows)
- GPG signature verification (Linux)
- Zone.Identifier detection (Windows)

**Usage:**
```cpp
#include "themis/base/module_loader.h"

using namespace themis::modules;

// Create module loader
ModuleLoader loader;

// Configure security policy
loader.setRequireSignature(true);   // Production mode
loader.setAllowUnsigned(false);     // Reject unsigned modules

// Add trusted hashes
loader.addWhitelistedHash("abc123...");

// Load a specific module
auto result = loader.loadModule("/path/to/themis_storage.dll", "themis_storage");
if (result.success) {
    std::cout << "Module loaded: " << result.modulePath << std::endl;
    std::cout << "Hash: " << result.moduleHash << std::endl;
} else {
    std::cerr << "Failed to load module: " << result.errorMessage << std::endl;
}

// Load all modules from directory
size_t loaded = loader.loadAllModules("/path/to/modules/");
std::cout << "Loaded " << loaded << " modules" << std::endl;

// Query loaded modules
if (loader.isModuleLoaded("themis_storage")) {
    auto info = loader.getModuleInfo("themis_storage").value();
    std::cout << "Storage module version: " << info.version << std::endl;
}

// Export audit log
loader.exportAuditLog("module_audit.json");

#ifdef _WIN32
// Windows-specific: Check for internet downloads
int zone = loader.getZoneIdentifier("/path/to/module.dll");
if (zone == 3) {
    std::cerr << "Warning: Module downloaded from internet" << std::endl;
}

// Verify Authenticode signature
std::string signer;
if (loader.verifyAuthenticodeSignature("/path/to/module.dll", signer)) {
    std::cout << "Signed by: " << signer << std::endl;
}
#endif

#ifdef __linux__
// Linux-specific: Verify GPG signature
if (loader.verifyGPGSignature("/path/to/module.so")) {
    std::cout << "Valid GPG signature" << std::endl;
}

// Check extended attributes
auto attrs = loader.getExtendedAttributes("/path/to/module.so");
for (const auto& [key, value] : attrs) {
    std::cout << key << " = " << value << std::endl;
}
#endif
```

**Security Best Practices:**
1. **Production**: Always require signatures (`setRequireSignature(true)`)
2. **Development**: Use `setAllowUnsigned(true)` only in dev environments
3. **CI/CD**: Sign all modules as part of the build pipeline
4. **Distribution**: Include signatures with module files (.asc for Linux, embedded for Windows)
5. **Monitoring**: Export and review audit logs regularly

---

#### base/export.h
**Export macros for base module**

Simplified export macros specifically for the base module. Duplicates functionality from top-level `export.h` for backward compatibility.

**Key Macros:**
- `THEMIS_BASE_API` - Export/import for base module

**Note:** This file will be deprecated in favor of top-level `themis/export.h` in future versions.

---

### Base Interfaces

#### base/interfaces/
**Abstract interfaces for dependency inversion**

Contains pure abstract interfaces that break circular dependencies between core components. See [base/README.md](base/README.md) for comprehensive documentation.

**Interface Files:**
- `storage_interface.h` - Storage engine abstraction
- `query_interface.h` - Query engine abstraction
- `index_interface.h` - Index manager abstraction
- `security_interface.h` - Encryption abstraction

**Purpose:**
These interfaces apply the Dependency Inversion Principle (DIP) to break circular dependencies:
- Query → Storage → Index had circular dependencies
- Interfaces allow high-level modules to depend on abstractions
- Low-level modules implement abstractions
- Enables testing with mocks
- Allows swapping implementations

---

### Network Protocol

#### network/wire_protocol_server.hpp
**Binary TCP wire protocol for high-performance client connections**

Implements ThemisDB's native binary protocol for low-latency, high-throughput client connections. Alternative to HTTP/REST for performance-critical applications.

**Key Classes:**
- `WireProtocolServer` - Main TCP server
- `WireProtocolSession` - Per-connection session handler
- `MessageDispatcher` - OpCode routing
- `WireFrameHeader` - Binary frame header (12 bytes)

**Header-declared protocol elements (not a guarantee that every operation is integrated):**
- Binary framing with magic number (`0x544D4442` = "TMDB")
- Protocol versioning (currently v1)
- Message flags (compressed, encrypted, skip checksum)
- CRC32 checksums for integrity
- LZ4 compression support
- Maximum payload size: 64MB

**OpCodes:**
- **Connection**: HELLO, AUTH_REQUEST, AUTH_RESPONSE, PING/PONG, CLOSE
- **Storage**: GET, PUT, DELETE, BATCH_GET, BATCH_PUT
- **Query**: QUERY_AQL, QUERY_RESULT, CURSOR operations
- **Transactions**: BEGIN, COMMIT, ABORT
- **Specialized**: VECTOR_SEARCH, GRAPH_TRAVERSE, GEO_QUERY, TIMESERIES_QUERY
- **BPMN**: START_PROCESS, TASK_COMPLETE, QUERY_INSTANCE
- **Response**: OK, ERROR

**Message Flags:**
- `SKIP_CHECKSUM` - Omit checksum (when TLS enabled)
- `COMPRESSED` - LZ4-compressed payload
- `ENCRYPTED` - payload marked as encrypted (implementation support is not established by this overview)

**Frame Format:**
```
+------------------+
| Magic (4 bytes)  |  0x544D4442 ("TMDB")
+------------------+
| Version (1 byte) |  0x01
+------------------+
| OpCode (1 byte)  |  Operation code
+------------------+
| Flags (2 bytes)  |  Message flags
+------------------+
| Length (4 bytes) |  Payload length
+------------------+
| Payload (N bytes)|  Protocol buffer message
+------------------+
| CRC32 (4 bytes)  |  Optional checksum
+------------------+
```

**Usage:**
```cpp
#include "themis/network/wire_protocol_server.hpp"
#include <boost/asio.hpp>

using namespace themis::wire;

// Create I/O context and server
boost::asio::io_context io_context;
WireProtocolServer server(io_context, 9090);  // Port 9090

// Start server
server.start();
std::cout << "Wire protocol server listening on port 9090" << std::endl;

// Run I/O context
io_context.run();

// Query statistics
std::cout << "Active sessions: " << server.active_sessions() << std::endl;
std::cout << "Total connections: " << server.total_connections() << std::endl;
```

**Client Connection Flow:**
```
1. Client connects to TCP port
2. Client sends HELLO with version and capabilities
3. Server responds with HELLO_ACK
4. Client sends AUTH_REQUEST with credentials
5. Server validates and sends AUTH_SUCCESS or AUTH_FAILURE
6. Client sends operations (GET, PUT, QUERY_AQL, etc.)
7. Server responds with results
8. Client sends CLOSE or connection times out
```

**Performance Characteristics:**
- Lower latency than HTTP (no header parsing)
- Binary serialization (Protocol Buffers)
- Efficient for high-throughput scenarios
- Persistent connections with keepalive
- Suitable for: embedded clients, SDKs, microservices

**Security and implementation notes:**
- Server handlers check authentication for protected operations.
- CRC32 checksums and a maximum payload size are implemented in the current
  server source.
- The header declares an `ENCRYPTED` flag, but this review did not establish
  payload encryption or TLS support in this server implementation. Do not
  interpret the flag as proof those features are available.
- The header lists opcodes whose handlers may return explicit "not integrated"
  or missing-backend responses. Check current server wiring before treating an
  opcode as an implemented database operation.

---

## Architecture Patterns

### Design Principles

1. **Foundation Layer**
   - Minimal dependencies
   - Stable interfaces
   - Platform abstraction
   - Version management

2. **Dependency Inversion**
   - Abstract interfaces in `base/interfaces/`
   - Break circular dependencies
   - Enable testing and mocking
   - Support pluggable implementations

3. **Security by Default**
   - Module signature verification
   - License validation
   - Secure defaults
   - Audit logging

4. **Compile-Time Configuration**
   - Edition selection at build time
   - Feature flags embedded in binary
   - Runtime introspection available
   - No runtime edition switching

5. **Cross-Platform Support**
   - Windows and Linux
   - Platform-specific optimizations
   - Consistent API across platforms

---

## Integration Points

### With All Modules

All ThemisDB modules depend on the Themis foundation:

```cpp
// Every module uses edition information
#include "themis/edition.h"

// Every module uses export macros
#include "themis/export.h"

// Modules query build configuration
#include "themis/build_info.h"
```

### Module Loading

Modular builds use the module loader:

```cpp
#include "themis/base/module_loader.h"

// Load required modules at startup
ModuleLoader loader;
loader.loadModule("themis_storage.dll", "themis_storage");
loader.loadModule("themis_query.dll", "themis_query");
```

### Wire Protocol Clients

High-performance clients use the wire protocol:

```cpp
#include "themis/network/wire_protocol_server.hpp"

// SDKs and embedded clients connect via wire protocol
// Server exposes port 9090 for binary protocol
```

---

## Runtime Behavior, Error Cases, and Limits

- **Module loading (`themis/base/module_loader.h`)** returns
  `ModuleVerificationResult`; verification failure prevents loading. Whether
  unsigned modules are rejected depends on the configured policy/build mode.
- **License checks (`themis/license_info.h`, `themis/runtime_license_gate.h`)** deny gated features when no valid license is embedded or when license constraints are violated.
- **Edition gates (`themis/edition.h`, `themis/edition_manager.h`)** are compile-time + runtime constraints: unavailable features remain disabled in Community builds even when referenced by callers.
- **Wire protocol v1 (`themis/network/wire_protocol_server.hpp`)** enforces frame integrity checks and a maximum payload size of **64 MB** per message (`themis::wire::MAX_PAYLOAD_SIZE`).
- **Operational limit:** `WireProtocolServer` is intended for a single-threaded `io_context` event loop unless external synchronization is provided (see [Roadmap Known Issues](../../src/themis/ROADMAP.md)).

## Public Entry Points (Quick Reference)

| Entry point | Purpose |
|-------------|---------|
| `themis/themis_api_contract.h` | Declares only `themis::engine::ThemisError` and `themis::engine::Edition` enums; its Doxygen prose describes methods not declared here |
| `themis/build_info.h` | Build/runtime metadata and reproducibility information |
| `themis/edition.h` | Compile-time edition configuration and feature flags |
| `themis/edition_manager.h` | Runtime feature-gate evaluation by edition/license state |
| `themis/license_info.h` | Embedded license access and signature/expiry validation |
| `themis/runtime_license_gate.h` | Structured runtime allow/deny decisions (`GateResult`) |
| `themis/base/module_loader.h` | Secure dynamic module loading + verification |
| `themis/module_hash_verifier.h` | SHA-256 module integrity verification |
| `themis/module_signature_verifier.h` | Platform-specific signature verification |
| `themis/network/wire_protocol_server.hpp` | Binary wire protocol server (v1) |
| `themis/network/wire_protocol_v2.hpp` | Multiplexed wire protocol API (v2) |

`themis_api_contract.h` currently declares no `BuildInfo` type, version or
commit accessor, or capability-discovery function. Use `build_info.h`,
`edition.h`, and `edition_manager.h` for those separately declared contracts;
do not infer declarations from the broader comments in
`themis_api_contract.h`.

## Dependencies

### Internal Dependencies

Themis module is the foundation - it has minimal internal dependencies:
- `base/interfaces/` may include standard library headers only

### External Dependencies

- **Standard Library**: C++20 standard library
- **Boost.Asio**: Network I/O (wire protocol)
- **Protocol Buffers**: Wire protocol messages
- **OpenSSL**: Signature verification, hashing

---

## Build Configuration

### CMake Integration

The build configures `THEMIS_EDITION` in `cmake/CMakeLists.txt` and adds
`src/themis` sources through the repository's CMake source lists. Do not create
a standalone `themis-base` target from the illustrative source list in this
older overview; it is not the current registration contract. For example:

```bash
cmake -S . -B build -DTHEMIS_EDITION=COMMUNITY
cmake --build build
```

## Edition Selection

```bash
# Configure for Community edition (default)
cmake -B build -DTHEMIS_EDITION=COMMUNITY

# Configure for Enterprise edition
cmake -B build -DTHEMIS_EDITION=ENTERPRISE

# Configure for Hyperscaler edition
cmake -B build -DTHEMIS_EDITION=HYPERSCALER

# Build
cmake --build build
```

## License Embedding

```bash
# Embed license data from a protected JSON file at build time.
# Do not put license keys or license-file contents in source control or logs.
cmake -S . -B build -DTHEMIS_LICENSE_FILE=/secure/path/license.json

# The CMake configuration reads the file and embeds its fields.
cmake --build build
```

---

## Usage Examples

### Startup Initialization

```cpp
#include "themis/build_info.h"
#include "themis/edition.h"
#include "themis/license_info.h"
#include "themis/base/module_loader.h"

int main() {
    using namespace themis;

    // 1. Print build information
    auto config = build_info::getBuildConfiguration();
    std::cout << build_info::formatBuildInfo(config) << std::endl;

    // 2. Validate edition
    auto edition = edition::EditionInfo::Get();
    std::cout << "Edition: " << edition.name << std::endl;

    // 3. Check license
    if (license::hasEmbeddedLicense()) {
        auto lic = license::getEmbeddedLicense().value();
        if (!license::isLicenseValid(lic)) {
            std::cerr << "Error: License expired" << std::endl;
            return 1;
        }
        std::cout << license::formatLicenseInfo(lic) << std::endl;
    }

    // 4. Load modules
    modules::ModuleLoader loader;
    loader.setRequireSignature(true);

    // Load required modules
    auto modules = {"themis_storage", "themis_query", "themis_security"};
    for (const auto& mod : modules) {
        auto result = loader.loadModule(std::string("/modules/") + mod + ".dll", mod);
        if (!result.success) {
            std::cerr << "Failed to load " << mod << ": "
                      << result.errorMessage << std::endl;
            return 1;
        }
    }

    // 5. Start wire protocol server
    boost::asio::io_context io_context;
    wire::WireProtocolServer wire_server(io_context, 9090);
    wire_server.start();

    // 6. Run application
    std::cout << "ThemisDB started successfully" << std::endl;
    io_context.run();

    return 0;
}
```

### Edition-Specific Code

```cpp
#include "themis/edition.h"

void initialize_features() {
    using namespace themis::edition;

    if (FEATURE_FIELD_ENCRYPTION) {
        // Enterprise/Hyperscaler only
        initialize_field_encryption();
    }

    if (FEATURE_MULTI_MASTER) {
        // Enterprise/Hyperscaler only
        initialize_replication();
    }

    if (FEATURE_RBAC) {
        // Enterprise/Hyperscaler only
        initialize_rbac();
    }

    // Community edition features (always available)
    initialize_basic_auth();
    initialize_tls();
}
```

### Module Verification

```cpp
#include "themis/base/module_loader.h"

bool verify_deployment(const std::string& modules_dir) {
    using namespace themis::modules;

    ModuleLoader loader;
    loader.setRequireSignature(true);

    // Verify all modules before starting
    std::vector<std::string> required_modules = {
        "themis_storage", "themis_query", "themis_security"
    };

    for (const auto& mod : required_modules) {
        std::string path = modules_dir + "/" + mod + ".dll";
        auto result = loader.loadModule(path, mod);

        if (!result.success) {
            std::cerr << "Module verification failed: " << mod << std::endl;
            std::cerr << "Error: " << result.errorMessage << std::endl;
            return false;
        }

        std::cout << "✓ Verified: " << mod << std::endl;
        std::cout << "  Hash: " << result.moduleHash << std::endl;

#ifdef _WIN32
        if (result.hasAuthenticode) {
            std::cout << "  Signed by: " << result.authenticodeSigner << std::endl;
        }

        if (result.zoneId == 3) {
            std::cerr << "  Warning: Downloaded from internet" << std::endl;
        }
#endif
    }

    return true;
}
```

---

## Status

**Readiness:** This overview makes no current production-readiness assertion.
Use `src/themis/ROADMAP.md` for module planning and root `ROADMAP.md` for
aggregate release readiness.

✅ **Stable Components:**
- Build information API
- Edition management
- License validation
- Export macros
- Base interfaces
- Wire protocol server (v1)

✅ **Beta Components:**
- Module loader (production-ready but new in v1.3.0)
- GPG signature verification (Linux)

---

## Related Documentation

- [Base Interfaces README](base/README.md) - Dependency inversion interfaces
- [Themis Source README](../../src/themis/README.md) - Runtime behavior and implementation details
- [Roadmap](../../src/themis/ROADMAP.md) - Delivery status, phases, limits, and known issues
- [Future Enhancements](../../src/themis/FUTURE_ENHANCEMENTS.md) - Planned improvements
- [Architecture Overview](../../ARCHITECTURE.md) - System architecture
- [Modularization Plan (DE)](../../docs/de/architecture/MODULARIZATION_PLAN.md) - Module strategy
- [German Themis Index](../../docs/de/themis/index.md) - Secondary module docs and verification
- [English Themis Index](../../docs/en/themis/index.md) - Secondary module docs and verification

---

## Contributing

When contributing to the Themis foundation module:

1. **Stability First**: This is the foundation - stability is critical
2. **Minimal Dependencies**: Keep dependencies minimal
3. **Backward Compatibility**: Never break existing interfaces
4. **Platform Support**: Test on Windows and Linux
5. **Documentation**: Comprehensive docs for all public APIs
6. **Security**: Security implications must be reviewed
7. **Performance**: Foundation code must be efficient

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

---

## See Also

- [Source Roadmap](../../src/themis/ROADMAP.md) - Current status and next phases
- [Source Security Notes](../../src/themis/SECURITY.md) - Threat model and security controls
- [Storage Module](../storage/README.md) - Storage interfaces
- [Query Module](../query/README.md) - Query interfaces
- [Server Module](../server/README.md) - Server interfaces
- [Security Module](../security/README.md) - Security interfaces

## Troubleshooting

- **`loadModule(...)` fails with verification errors:** verify module hash/signature artifacts and ensure production policy settings (`setRequireSignature`, whitelist/blacklist) match deployment.
- **Feature unexpectedly denied:** inspect `edition::EditionInfo::Get()` and runtime gate checks to confirm the active edition/license permits the feature.
- **Wire protocol client disconnects early:** validate frame magic/version/CRC and payload size; malformed frames are rejected.
- **Edition-specific behavior differs between builds:** confirm `-DTHEMIS_EDITION=...` and license embedding settings used during CMake configure.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
