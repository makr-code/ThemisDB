<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Themis Module Implementation - Future Enhancements

- Build metadata API: `getBuildConfiguration()`, `build_info.cpp` (version, compiler, flags, git SHA)
- License validation: `license_info.cpp` with Ed25519 signature verification and expiry checking
- Module loading: `module_loader.cpp` — secure `dlopen`/`LoadLibrary` with SHA-256 hash verification
- Wire protocol server: `wire_protocol_server.cpp` (namespace `themis::wire`), binary client session management
- Module dependency resolution and topological load-order management (`module_dependency_resolver.cpp`)
- Edition feature gating (Community / Enterprise / Cloud) via `edition_manager.cpp`
- Runtime checks: `isModuleCompiledIn()`, Authenticode/GPG signature validation, Zone.Identifier quarantine detection

## Design Constraints

- [ ] Public headers in `include/themis/` are frozen for v1.x; no breaking changes to existing APIs or ABI
- [ ] Module loads must verify SHA-256 hash against a signed manifest before `dlopen`/`LoadLibrary`; mismatch aborts load
- [ ] License comparison must use `CRYPTO_memcmp` or equivalent constant-time function; `memcmp`/`strcmp` are forbidden
- [ ] Edition feature gates must be enforced exclusively at the `edition_manager` layer; modules must not hard-code edition checks
- [ ] `WireProtocolServer` must operate correctly in a single-threaded `io_context`; shared state accessed only from event-loop thread
- [ ] `getBuildConfiguration()` must return deterministic output for the same binary; runtime environment variables must not affect the result
- [ ] LZ4 compress/decompress stubs must be replaced before v2.0.0; a compile-time test must fail if stubs remain in a release build

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IBuildInfo` | `getBuildConfiguration()` | Version string, compiler, flags, git SHA; read-only after init |
| `ILicenseValidator` | `edition_manager`, startup check | Ed25519 signature verify, expiry check, constant-time compare |
| `IModuleLoader` | `module_dependency_resolver` | `dlopen`/`LoadLibrary` + SHA-256 + Authenticode/GPG verification |
| `IEditionManager` | All feature-gated subsystems | `isFeatureEnabled(FeatureFlag)`, `getEdition()` |
| `IWireProtocolHandler` | `WireProtocolServer` | Per-opcode request dispatch; register/unregister handlers |
| `IModuleDependencyResolver` | Module boot sequence | Topological sort of module load order; cycle detection |
| `IFeatureFlag` | Enterprise/Cloud subsystems | Runtime bool gate keyed by `EditionFeature` enum; ≤ 100 ns per call |

## Implementation Roadmap

This document outlines planned implementation enhancements for the Themis core framework module.

## Near-Term Enhancements (v1.7.0)

### Modular Build System
**Priority:** Critical
**Target Version:** v1.7.0

Migrate from monolithic to modular build architecture.

**Tasks:**
1. Extract core implementations to `src/themis/`
2. Create `libthemis-base.so` / `themis-base.dll`
3. Update CMakeLists.txt for modular builds
4. Add export macros to all public APIs
5. Test on Windows and Linux

**Migration Steps:**
```bash
# Step 1: Extract implementations
git mv src/core/build_info.cpp src/themis/build_info.cpp
git mv src/security/license_manager.cpp src/themis/license_info.cpp

# Step 2: Update CMakeLists.txt
add_library(themis-base SHARED
    src/themis/build_info.cpp
    src/themis/license_info.cpp
    src/themis/module_loader.cpp
)

# Step 3: Update other modules
target_link_libraries(themis-storage themis-base)
target_link_libraries(themis-query themis-base)
```

---

## Module Loader Implementation
**Priority:** Critical
**Target Version:** v1.7.0

Implement secure module loading with signature verification.

**Components:**
- `ModuleLoader` class implementation
- `ModuleSecurityVerifier` class implementation
- Platform-specific loading (Windows/Linux)
- Hash and signature verification
- Audit logging

**Implementation Files:**
- `src/themis/module_loader.cpp` - Main implementation
- `src/themis/module_loader_win32.cpp` - Windows-specific
- `src/themis/module_loader_linux.cpp` - Linux-specific
- `src/themis/module_security.cpp` - Security verification

**Security Features:**
```cpp
// Implementation priorities
1. SHA-256 hash verification ✓
2. Signature verification (X.509/GPG) ✓
3. Whitelist/blacklist support ✓
4. Zone.Identifier detection (Windows) ✓
5. Authenticode verification (Windows) ✓
6. Audit logging ✓
```

---

### Wire Protocol Performance Optimizations
**Priority:** High
**Target Version:** v1.7.0

Optimize wire protocol for lower latency and higher throughput.

**Optimizations:**
1. **Zero-Copy Serialization**
   - Avoid buffer copies where possible
   - Use memory mapping for large payloads
   - Direct Protocol Buffer serialization to socket

2. **Connection Pooling**
   - Reuse connections efficiently
   - Connection keep-alive
   - Automatic reconnection

3. **Batch Processing**
   - Batch small messages
   - Coalesce writes
   - Nagle's algorithm tuning

4. **Compression Strategies**
   - Adaptive compression (compress only if beneficial)
   - Compression level selection based on payload
   - Dictionary compression for repeated data

**Performance Targets:**
```
Current:
  - Round-trip latency: ~2ms
  - Throughput: 50K ops/sec

Target v1.7.0:
  - Round-trip latency: <1ms (p99)
  - Throughput: 100K ops/sec
  - Memory overhead: <10MB per 1000 connections
```

---

### Build Reproducibility Implementation
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Implemented in v1.7.0

Ensure all builds are reproducible for security auditing.

**Implementation** (`src/themis/build_info.cpp`, `include/themis/build_info.h`):
```cpp
namespace themis::build_info {

struct ReproducibilityInfo {
    std::string git_commit;        ///< Full SHA-1 of HEAD at build time
    std::string git_commit_date;   ///< ISO-8601 author date of HEAD
    std::string git_branch;        ///< Branch name (or "(detached)")
    bool        git_dirty = false; ///< True if working tree had uncommitted changes
    std::string build_host;        ///< Hostname of the build machine
    std::string build_user;        ///< OS username that invoked the build
    std::string toolchain;         ///< "compiler-id/version" e.g. "GCC/13.2.0"
    std::map<std::string, std::string> dependencies; ///< dep-name → version/hash
    std::string binary_hash;       ///< SHA-256 of the running executable (OpenSSL)
};

ReproducibilityInfo getReproducibilityInfo();
bool exportBuildManifest(const std::string& output_path);
bool verifyBuildManifest(const std::string& manifest_path);

} // namespace themis::build_info
```

**CMake Integration** (`cmake/CMakeLists.txt`):
```cmake
# Capture git HEAD, branch, dirty flag, and build environment
find_package(Git QUIET)
execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%H
    OUTPUT_VARIABLE _THEMIS_GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%ci
    OUTPUT_VARIABLE _THEMIS_GIT_COMMIT_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
    OUTPUT_VARIABLE _THEMIS_GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
cmake_host_system_information(RESULT _THEMIS_BUILD_HOST QUERY HOSTNAME)

# Injected as compile definitions into src/themis/build_info.cpp
add_compile_definitions(
    "THEMIS_GIT_COMMIT=\"${_THEMIS_GIT_COMMIT}\""
    "THEMIS_GIT_COMMIT_DATE=\"${_THEMIS_GIT_COMMIT_DATE}\""
    "THEMIS_GIT_BRANCH=\"${_THEMIS_GIT_BRANCH}\""
    "THEMIS_GIT_DIRTY=${_THEMIS_GIT_DIRTY}"
    "THEMIS_BUILD_HOST=\"${_THEMIS_BUILD_HOST}\""
    "THEMIS_BUILD_USER=\"${_THEMIS_BUILD_USER}\""
)
```

**Tests:** `tests/test_build_info.cpp` — CTest target `BuildInfoTests` (15 tests).
**CI:** `.github/workflows/build-reproducibility-ci.yml`.

---

## Mid-Term Enhancements (v1.8.0)

### Hot Module Reload
**Priority:** Medium
**Target Version:** v1.8.0

Support reloading modules without restarting the server.

**Implementation:**
```cpp
// module_loader.cpp additions
class HotReloadManager {
public:
    struct ReloadResult {
        bool success;
        std::string old_version;
        std::string new_version;
        std::chrono::milliseconds reload_time;
        std::string error_message;
    };

    ReloadResult reloadModule(
        const std::string& module_name,
        const std::string& new_module_path);

    bool supportsHotReload(const std::string& module_name);

private:
    // State migration
    void migrateState(void* old_module, void* new_module);

    // Connection handling
    void pauseConnections();
    void resumeConnections();

    // Rollback on failure
    void rollback(const std::string& module_name);
};
```

**Challenges:**
- State migration between versions
- ABI compatibility checking
- Connection handling during reload
- Rollback on failure

---

### Module Sandboxing
**Priority:** Medium
**Target Version:** v1.8.0

Isolate modules in sandboxes to prevent malicious code execution.

**Implementation:**
```cpp
// module_sandbox.cpp (new file)
class ModuleSandbox {
public:
    struct SandboxConfig {
        size_t max_memory_mb;
        int max_cpu_percent;
        bool network_access;
        FilesystemAccess filesystem_access;
        std::vector<std::string> allowed_syscalls;
    };

    void launchSandboxed(
        const std::string& module_path,
        const SandboxConfig& config);

    SandboxStats getStats() const;

private:
    // Linux: namespaces, cgroups, seccomp
    void setupLinuxSandbox(const SandboxConfig& config);

    // Windows: job objects, integrity levels
    void setupWindowsSandbox(const SandboxConfig& config);
};
```

**Platform Support:**
- **Linux**: Namespaces (PID, network, mount), cgroups, seccomp-bpf
- **Windows**: Job objects, integrity levels, AppContainer

---

### Wire Protocol V2 Implementation
**Priority:** High
**Target Version:** v1.8.0
**Status:** ✅ Implemented (v1.8.0)

Implement enhanced wire protocol with multiplexing.

**Implementation:**
- Header: `include/themis/network/wire_protocol_v2.hpp`
- Source: `src/network/wire_protocol_v2.cpp`
- Tests:  `tests/test_wire_protocol_v2.cpp` (WireProtocolV2FocusedTests — 30+ tests)
- CI:     `.github/workflows/wire-protocol-v2-ci.yml`

```cpp
// V2Server – multiplexed wire protocol server
// See include/themis/network/wire_protocol_v2.hpp for full API.
class V2Server {
public:
    explicit V2Server(const V2ConnectionConfig& config);

    void start();
    void stop();
    bool is_running() const;

    void set_data_handler(V2DataHandler handler);
    void set_headers_handler(V2HeadersHandler handler);
    void set_rst_stream_handler(V2RstStreamHandler handler);

    bool push_to_client(const std::string& connection_id,
                        uint32_t associated_sid,
                        const std::unordered_map<std::string, std::string>& headers,
                        const std::vector<uint8_t>& data);

    size_t   active_connections()   const;
    uint64_t total_streams_opened() const;
    uint64_t total_frames_sent()    const;
    uint64_t total_frames_received() const;
};
```

**Protocol Features:**
- [x] HTTP/2-style multiplexing (multiple logical streams per TCP connection via `stream_id`)
- [x] Server push (`PUSH_PROMISE` frame + `push_to_client()` API)
- [x] Flow control (`WINDOW_UPDATE` frames, per-stream and connection-level windows)
- [x] Priority and dependency management (`V2Stream.priority` 0–255, `V2Stream.stream_dependency`, `V2Stream.exclusive_dependency`, `PRIORITY` frame handled in `handleFrame()`, `set_stream_priority()` on `V2Session`); RFC 7540 compliance: §6.3 stream_id=0 → `go_away(PROTOCOL_ERROR)`, §5.3.1 self-dependency → `reset_stream(PROTOCOL_ERROR)`, same guards in `set_stream_priority()`
- [x] LZ4 and Zstd connection-level compression (`COMPRESSED` / `ZSTD_COMPRESSED` flags)
- [x] Graceful connection shutdown (`GOAWAY` frame)
- [x] Stream reset (`RST_STREAM` frame)
- [x] Settings negotiation (`SETTINGS` frame with ACK)

---

### License Server Client
**Priority:** Medium
**Target Version:** v1.8.0

Implement client for online license validation.

**Implementation:**
```cpp
// license_client.cpp (new file)
class LicenseClient {
public:
    struct ActivationResult {
        bool success;
        std::string license_key;
        std::chrono::system_clock::time_point expiry;
        std::string error_message;
    };

    LicenseClient(const std::string& license_server_url);

    ActivationResult activate(const std::string& license_key);

    bool validate();

    void setValidationInterval(std::chrono::hours interval);

    using ValidationCallback = std::function<void(const LicenseError&)>;
    void onValidationFailure(ValidationCallback callback);

private:
    // HTTPS communication
    void httpPost(const std::string& endpoint, const nlohmann::json& data);

    // Periodic validation
    void scheduleValidation();
};
```

**Features:**
- Online activation
- Periodic validation
- Grace period handling
- Offline mode support

---

## Long-Term Enhancements (v1.9.0+)

### Trusted Execution Environment (TEE)
**Priority:** Low
**Target Version:** v1.9.0

Support Intel SGX and AMD SEV for module isolation.

**Implementation:**
```cpp
// tee_module_loader.cpp (new file)
class TEEModuleLoader : public ModuleLoader {
public:
    LoadResult loadModuleInEnclave(
        const std::string& module_path,
        const TEEConfig& config);

    bool verifyEnclave(const std::string& module_name);

    AttestationReport getAttestationReport(const std::string& module_name);

private:
    void initializeSGX();
    void initializeSEV();
};
```

**Security Benefits:**
- Memory encryption
- Attestation support
- Secure enclaves for modules
- Protection from OS-level attacks

---

### Module Capability Negotiation
**Priority:** Medium
**Target Version:** v1.9.0

Dynamic capability negotiation between modules.

**Implementation:**
```cpp
// module_capabilities.cpp (new file)
class ModuleCapabilities {
public:
    struct Capability {
        std::string name;
        Version min_version;
        Version max_version;
    };

    std::vector<Capability> query(const std::string& module_name);

    std::vector<std::string> negotiate(
        const std::vector<std::string>& modules);

    bool supports(
        const std::string& module_name,
        const std::string& capability);
};
```

---

### Compression Algorithm Extensions
**Priority:** Medium
**Target Version:** v1.9.0

Additional compression algorithms for wire protocol.

**Planned Algorithms:**
- Zstd (better compression ratio)
- Brotli (best for JSON/text)
- Snappy (Google's fast compression)
- Dictionary compression (for repeated data)

**Implementation:**
```cpp
// compression.cpp additions
class CompressionEngine {
public:
    enum class Algorithm {
        LZ4,
        ZSTD,
        BROTLI,
        SNAPPY,
        DICTIONARY
    };

    std::vector<uint8_t> compress(
        const std::vector<uint8_t>& data,
        Algorithm algo,
        int level = -1);

    std::vector<uint8_t> decompress(
        const std::vector<uint8_t>& data,
        Algorithm algo);

    // Select best algorithm based on data characteristics
    Algorithm selectBest(const std::vector<uint8_t>& data);

private:
    // Dictionary training for repeated data
    void trainDictionary(const std::vector<std::vector<uint8_t>>& samples);
};
```

---

## Testing Enhancements

### Comprehensive Test Suite
**Priority:** High
**Target Version:** v1.7.0

**Test Categories:**
1. **Unit Tests**
   - Module loader functions
   - License validation logic
   - Wire protocol message handling
   - Compression/decompression

2. **Integration Tests**
   - End-to-end module loading
   - Multi-client wire protocol
   - License server integration

3. **Security Tests**
   - Tampered module detection
   - Invalid signature rejection
   - DoS attack resistance
   - Memory safety (AddressSanitizer)

4. **Performance Tests**
   - Module load time benchmarks
   - Wire protocol latency benchmarks
   - Throughput benchmarks
   - Memory usage profiling

5. **Fuzz Testing**
   - Wire protocol fuzzing (AFL++)
   - Module file fuzzing
   - License data fuzzing

---

### Continuous Integration
**Priority:** High
**Target Version:** v1.7.0

**CI Pipeline:**
```yaml
# .github/workflows/themis-ci.yml
name: Themis Module CI

on: [push, pull_request]

jobs:
  test:
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest]
        compiler: [gcc, clang, msvc]

    steps:
      - uses: actions/checkout@v3

      - name: Build
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
          cmake --build build

      - name: Unit Tests
        run: ctest --test-dir build --output-on-failure

      - name: Security Tests
        run: ./build/tests/security/themis_security_tests

      - name: Performance Tests
        run: ./build/tests/performance/themis_perf_tests

      - name: Coverage
        run: |
          lcov --capture --directory build --output-file coverage.info
          lcov --remove coverage.info '/usr/*' --output-file coverage.info
          codecov -f coverage.info
```

---

## Performance Optimization Plan

### Profiling Strategy
**Priority:** High
**Target Version:** v1.7.0

**Tools:**
- **perf** (Linux) - CPU profiling
- **VTune** (Intel) - Microarchitecture analysis
- **heaptrack** (Linux) - Memory profiling
- **Windows Performance Analyzer** (Windows)

**Hot Paths:**
1. Module signature verification
2. Wire protocol serialization/deserialization
3. Compression/decompression
4. Hash calculation

**Optimization Targets:**
```
Module Loading:
  Current: ~30ms with signature verification
  Target:  <20ms (v1.7.0), <10ms (v1.8.0)

Wire Protocol:
  Current: ~2ms round-trip
  Target:  <1ms (v1.7.0), <0.5ms (v1.8.0)

Compression:
  Current: ~1ms per MB (LZ4)
  Target:  <0.8ms per MB (v1.7.0)
```

---

## Documentation Enhancements

### API Documentation
**Priority:** Medium
**Target Version:** v1.7.0

- Complete Doxygen comments for all public APIs
- Usage examples for each class
- Architecture diagrams
- Sequence diagrams for complex flows

### Developer Guide
**Priority:** Medium
**Target Version:** v1.7.0

- Step-by-step implementation guide
- Platform-specific considerations
- Security best practices
- Performance tuning guide

---

## Contributing

For contribution guidelines, see the main [CONTRIBUTING.md](../../CONTRIBUTING.md).

Priority areas for implementation contributions:
1. Modular build system
2. Module loader implementation
3. Wire protocol optimizations
4. Test suite development

---

## Test Strategy

- Unit test coverage ≥ 80% for `wire_protocol_server`, `module_dependency_resolver`, `edition_manager`, and `build_info`
- Integration tests: module load sequence with SHA-256 hash verification, license expiry edge cases (expired, clock-skew ±5 min), edition gate enforcement
- Wire protocol conformance tests: all defined `OpCode` values including PING/PONG, AUTH, QUERY, and ERROR; malformed-packet fuzzing via libFuzzer
- Regression test: verify `WireProtocolServer::sessions_` count is bounded (does not grow) after session disconnect fix is applied
- Constant-time license comparison timing test: 10,000 iterations, measured variance must be ≤ 2× mean to confirm no data-dependent branching

## Performance Targets

- Module load time (SHA-256 verification + `dlopen`/`LoadLibrary`) ≤ 50 ms per module on NVMe storage
- `getBuildConfiguration()` first-call latency ≤ 1 ms; subsequent calls served from in-process cache in ≤ 10 µs
- License validation (Ed25519 signature verification) ≤ 5 ms on hardware without HSM acceleration
- Wire protocol server: handle ≥ 10,000 concurrent sessions at p99 opcode dispatch latency ≤ 1 ms per session
- `edition_manager.isFeatureEnabled()` ≤ 100 ns per call in hot-path (no locking required after initialization)

## Security / Reliability

- All module loads must verify SHA-256 hash against a signed manifest before `dlopen`/`LoadLibrary`; a mismatch must abort the load and write a security audit event
- License string comparison must use `CRYPTO_memcmp` or equivalent constant-time function; `memcmp` and `strcmp` are forbidden in all license validation code paths
- `edition_manager` must fail-closed: if edition cannot be determined (missing or corrupt license), the system defaults to Community edition and logs an audit event
- `WireProtocolServer` session map (`sessions_`) must be protected by a mutex before any multi-threaded `io_context` support is added
- Zone.Identifier / quarantine attribute check must be performed on Windows before loading any module from a path outside the installation directory
- Module hash manifests must be signed with the same Ed25519 key used for license validation; unsigned or separately signed manifests are rejected

---

## See Also

- [README.md](README.md) - Current implementation status
- [Header Documentation](../../include/themis/README.md) - Public API
- [Architecture](../../ARCHITECTURE.md) - System architecture
- [Modularization Plan](../../docs/architecture/MODULARIZATION_PLAN.md)
