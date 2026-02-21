# Themis Core Framework – Developer Guide

> **Version:** 1.7.0  
> **Phase:** Production Readiness (Phases 1–7)  
> **Status:** Stable

---

## Table of Contents

1. [Overview](#overview)
2. [Modular Build System](#modular-build-system)
3. [Build Reproducibility](#build-reproducibility)
4. [Secure Module Loader](#secure-module-loader)
5. [Module Sandbox & ABI Checker](#module-sandbox--abi-checker) *(Phase 4)*
6. [Wire Protocol v1 – Performance Layer](#wire-protocol-v1--performance-layer) *(Phase 2)*
7. [Wire Protocol v2 – Multiplexed Streams](#wire-protocol-v2--multiplexed-streams) *(Phase 5)*
8. [License Client](#license-client) *(Phase 6)*
9. [CI / CD Pipeline](#cicd-pipeline)
10. [Sequence Diagrams](#sequence-diagrams)
11. [API Reference](#api-reference)

---

## Overview

The **Themis Core Framework** is the production-ready foundation layer of ThemisDB.
It provides:

| Component                | Header(s)                                       | Phase |
|--------------------------|-------------------------------------------------|-------|
| Modular build macros     | `include/themis/base/export.h`                  | 1     |
| Build reproducibility    | `include/themis/build_info.h`                   | 1     |
| Secure module loader     | `include/themis/base/module_loader.h`           | 1     |
| Module sandbox & ABI     | `include/themis/base/module_sandbox.h`          | 4     |
| Wire Protocol v1 perf    | `include/network/wire_protocol_performance.h`   | 2     |
| Wire Protocol v2         | `include/themis/network/wire_protocol_v2.hpp`   | 5     |
| License client           | `include/themis/license_info.h`                 | 6     |

---

## Modular Build System

ThemisDB supports both a **monolithic** build (default, `THEMIS_BUILD_MODULAR=OFF`) and
a **modular** shared-library build (`THEMIS_BUILD_MODULAR=ON`, requires ≥ v1.4.0).

### Enable modular build

```bash
cmake -B build -S cmake \
    -DTHEMIS_BUILD_MODULAR=ON \
    -DTHEMIS_MODULE_TRANSACTION=ON \
    -DTHEMIS_MODULE_LLM=OFF
```

### Using export macros

Every public symbol that must be visible from a shared library is annotated with
the appropriate `THEMIS_*_API` macro defined in `include/themis/base/export.h`:

```cpp
// In a public header:
#include "themis/base/export.h"

class THEMIS_BASE_API MyClass {
public:
    THEMIS_BASE_API void doSomething();
};
```

When building the `themis_base` shared library, `THEMIS_BASE_EXPORTS` is defined
automatically by CMake.  Consuming targets see `__declspec(dllimport)` on Windows
and `__attribute__((visibility("default")))` on Linux/macOS.

---

## Build Reproducibility

Every ThemisDB binary embeds its exact source revision at compile time.
The `BuildReproducibilityInfo` API (Phase 1) lets you audit any binary.

### Capture metadata in CMake

```cmake
find_package(Git QUIET)
execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%H ...)
add_compile_definitions(
    "THEMIS_GIT_COMMIT=\"${_THEMIS_GIT_COMMIT}\""
    "THEMIS_GIT_BRANCH=\"${_THEMIS_GIT_BRANCH}\""
    "THEMIS_GIT_DIRTY=${_THEMIS_GIT_DIRTY}"
    "THEMIS_BUILD_HOST=\"${_THEMIS_BUILD_HOST}\""
)
```

### Runtime API

```cpp
#include "themis/build_info.h"

auto info = themis::build_info::getReproducibilityInfo();
std::cout << "Git commit:  " << info.git_commit      << '\n';
std::cout << "Branch:      " << info.git_branch      << '\n';
std::cout << "Dirty tree:  " << info.git_dirty       << '\n';
std::cout << "Build host:  " << info.build_host      << '\n';
std::cout << "Toolchain:   " << info.toolchain       << '\n';
std::cout << "Binary hash: " << info.binary_hash     << '\n'; // SHA-256

// Export for CI archiving
themis::build_info::exportBuildManifest("/artifacts/build.json");

// Verify in CI
bool ok = themis::build_info::verifyBuildManifest("/artifacts/build.json");
```

### CI usage (GitHub Actions)

```yaml
- name: Verify build reproducibility
  run: |
    ./themisdb --export-build-manifest /tmp/manifest.json
    # Store as artifact
    echo "GIT_COMMIT=$(jq -r .git_commit /tmp/manifest.json)" >> $GITHUB_OUTPUT
```

---

## Secure Module Loader

The module loader (`ModuleLoader`) verifies every dynamically loaded plugin before
execution using SHA-256 file hashes and (optionally) X.509 code-signing certificates.

### Loading a single module

```cpp
#include "themis/base/module_loader.h"

themis::modules::ModuleLoader loader;
loader.setStagedLoadingEnabled(true);      // ABI check before activation

auto result = loader.loadModule(
    "/opt/themisdb/modules/libthemis_analytics.so",
    "analytics"
);

if (!result.success) {
    std::cerr << "Load failed: " << result.errorMessage << '\n';
}
```

### Batch load from a directory

```cpp
size_t loaded = loader.loadAllModules("/opt/themisdb/modules");
std::cout << "Loaded " << loaded << " module(s)\n";
```

### Security policy

```cpp
// Production: require signed modules
loader.setRequireSignature(true);
loader.setAllowUnsigned(false);
loader.addWhitelistedHash("a3f8...");  // known-good hash
loader.addBlacklistedHash("0bad...");  // revoked module
```

### Audit log

```cpp
loader.exportAuditLog("/var/log/themisdb/modules.jsonl");
```

---

## Module Sandbox & ABI Checker

Phase 4 adds safety guarantees for hot-reload and untrusted plugin execution.

### AbiChecker – deep compatibility validation

Run this **before** activating a new module handle:

```cpp
#include "themis/base/module_sandbox.h"
#include "themis/base/module_loader.h"

themis::modules::AbiChecker checker;
checker.useDefaultLists(); // themis_module_init, themis_module_version, …

// new_handle comes from dlopen() / LoadLibraryA(); new_meta from extractMetadata()
auto result = checker.check(
    new_handle, new_meta,
    /*host_major=*/1, /*host_minor=*/7
);

if (!result.compatible) {
    for (const auto& issue : result.issues)
        LOG_ERROR("ABI issue: {}", issue);
    dlclose(new_handle);
    return false;
}
```

### ModuleSandbox – resource-constrained execution

```cpp
#include "themis/base/module_sandbox.h"

themis::modules::ModuleSandbox::Config cfg;
cfg.max_memory_mb   = 512;   // 512 MiB memory cap
cfg.max_cpu_percent = 25;    // 25% CPU share
cfg.allow_network   = false; // no outbound calls

themis::modules::ModuleSandbox sandbox(cfg);
if (!sandbox.launch("themis_ml_plugin")) {
    LOG_ERROR("Sandbox failed: {}", sandbox.lastError());
}

// Inspect warnings for unsupported platform features
for (const auto& w : sandbox.launchWarnings())
    LOG_WARN("Sandbox: {}", w);

// … module runs … //

auto stats = sandbox.stats();
LOG_INFO("Peak memory: {} MB", stats.peak_memory_bytes / (1024*1024));
sandbox.shutdown();
```

---

## Wire Protocol v1 – Performance Layer

Phase 2 adds three composable helpers to the existing `WireProtocolServer`:

### WireProtocolMetrics – latency/throughput/error tracking

```cpp
#include "network/wire_protocol_performance.h"

themis::network::WireProtocolMetrics metrics;

// In each request handler:
auto t0 = std::chrono::steady_clock::now();
// … handle request, get bytes …
metrics.recordLatency(t0);
metrics.recordBytes(bytes_in, bytes_out);

// If compressed:
metrics.recordCompression(original_size, compressed_size);

// On error:
metrics.recordError("timeout");   // "connection" | "timeout" | "parse" | "auth"

// Periodic reporting (e.g., every 10 s):
auto snap = metrics.snapshot();
LOG_INFO("p99={:.2f}ms  rps={}  err_rate={:.4f}",
    snap.latency.p99_ms,
    snap.throughput.requests_total,
    snap.errors.error_rate);
```

### PayloadBufferPool – slab allocator

```cpp
#include "network/wire_protocol_performance.h"

// Shared across all sessions (thread-safe):
static themis::network::PayloadBufferPool g_pool(64 * 1024, /*depth=*/256);

// Per-request:
auto buf = g_pool.acquire();
buf->resize(payload_size);
// … fill buffer …
net::async_write(socket, net::buffer(*buf), handler);
// buf is automatically returned to pool when it goes out of scope
```

### CompressionAdvisor

```cpp
#include "network/wire_protocol_performance.h"

themis::network::CompressionAdvisor advisor;

switch (advisor.advise(payload.size())) {
case themis::network::CompressionAdvisor::Decision::SKIP:
    // send raw
    break;
case themis::network::CompressionAdvisor::Decision::LZ4_FAST:
    int acc = advisor.lz4Acceleration(decision);
    // LZ4_compress_fast(src, dst, srcSize, dstCapacity, acc)
    break;
case themis::network::CompressionAdvisor::Decision::LZ4_HC:
    int level = advisor.lz4HcLevel();
    // LZ4_compress_HC(src, dst, srcSize, dstCapacity, level)
    break;
}
```

---

## Wire Protocol v2 – Multiplexed Streams

Phase 5 introduces a fully multiplexed binary protocol on a separate port (default 7890).

### Protocol frame layout (16-byte fixed header)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     magic (TMD2 / 0x544D4432)                 |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  version=0x02 |  frame_type   |           flags               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          stream_id                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        payload_length                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    payload (variable length)                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Starting the V2 server

```cpp
#include "themis/network/wire_protocol_v2.hpp"

themis::wire::V2ConnectionConfig cfg;
cfg.port                    = 7890;
cfg.max_concurrent_streams  = 200;
cfg.enable_server_push      = true;
cfg.enable_flow_control     = true;
cfg.enable_lz4_compression  = true;

themis::wire::V2Server server(cfg);
server.set_data_handler(
    [](uint32_t stream_id, const std::vector<uint8_t>& data, bool end_stream) {
        // Handle incoming data frame
    });
server.start();
```

### Server push (unsolicited data → client)

```cpp
// Push updated index stats to all interested clients
server.push_to_client(
    connection_id,
    associated_stream_id,
    {{"content-type", "application/octet-stream"},
     {"event", "index_updated"}},
    serialized_payload
);
```

### Stream state machine

```
  IDLE ──HEADERS──► OPEN ──END_STREAM──► HALF_CLOSED_REMOTE
                      │                           │
                 END_STREAM               END_STREAM (local)
                      │                           │
              HALF_CLOSED_LOCAL          CLOSED ◄─┘
                      │
                 END_STREAM (remote)
                      │
                  CLOSED
```

---

## License Client

Phase 6 adds an activation and periodic validation client for the ThemisDB
license server.

### Online activation

```cpp
#include "themis/license_info.h"

themis::license::LicenseClientConfig cfg;
cfg.server_url      = "https://license.themisdb.io/v1";
cfg.api_key         = getenv("THEMIS_LICENSE_API_KEY");
cfg.timeout         = std::chrono::seconds(10);
cfg.allow_offline   = true;   // Grace period if server unreachable
cfg.grace_period_days = 7;

themis::license::LicenseClient client(cfg);
auto result = client.activate();

if (!result.success) {
    if (result.status == "grace") {
        LOG_WARN("License server unreachable – {} grace day(s) remaining",
                 result.grace_days_remaining);
    } else {
        LOG_ERROR("License activation failed: {}", result.error_message);
        return 1;
    }
}
```

### Machine fingerprint

```cpp
std::string fp = themis::license::LicenseClient::getMachineFingerprint();
// SHA-256 of the primary network interface MAC address.
// Example: "a3f8e2b1c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1"
```

### Periodic re-validation

```cpp
// In a background thread (e.g., every hour):
auto result = client.validate();
if (result.status == "expired") {
    LOG_ERROR("License expired – shutting down");
    initiateGracefulShutdown();
}
```

---

## CI/CD Pipeline

The following GitHub Actions workflow fragment validates build reproducibility and
runs the full test suite on each push:

```yaml
name: ThemisDB CI

on: [push, pull_request]

jobs:
  build-and-test:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4

      - name: Configure
        run: |
          cmake -B build -S cmake \
            -DTHEMIS_BUILD_TESTS=ON \
            -DCMAKE_BUILD_TYPE=Release

      - name: Build
        run: cmake --build build --parallel

      - name: Run Tests
        run: ctest --test-dir build --output-on-failure

      - name: Export Build Manifest
        run: |
          ./build/bin/themisdb_tests \
            --gtest_filter=BuildManifest.ExportCreatesFile
          # Or use the CLI:
          # ./build/bin/themisdb --export-build-manifest build/manifest.json

      - name: Upload Manifest
        uses: actions/upload-artifact@v4
        with:
          name: build-manifest
          path: build/manifest.json
```

---

## Sequence Diagrams

### Module Hot-Reload with ABI Check

```
  Operator       ModuleLoader       AbiChecker       ModuleSandbox
     │                │                  │                 │
     │ loadModule()   │                  │                 │
     │───────────────►│                  │                 │
     │                │ verifyModule()   │                 │
     │                │──────────────────►                 │
     │                │◄─── result ──────│                 │
     │                │                  │                 │
     │                │ check(handle,    │                 │
     │                │       meta,      │                 │
     │                │       major,     │                 │
     │                │       minor)     │                 │
     │                │─────────────────►│                 │
     │                │◄── AbiCheckResult│                 │
     │                │                  │                 │
     │                │ [if compatible]  │                 │
     │                │ sandbox.launch() │                 │
     │                │────────────────────────────────────►
     │                │◄─── ok ──────────────────────────────
     │                │                  │                 │
     │◄── result ─────│                  │                 │
```

### License Validation Flow

```
  Server Boot    LicenseClient    License Server    Cache
       │                │               │             │
       │ activate()     │               │             │
       │───────────────►│               │             │
       │                │ POST /activate │             │
       │                │───────────────►             │
       │                │◄── 200 OK ────│             │
       │                │ store(result) │             │
       │                │──────────────────────────────►
       │◄── active ─────│               │             │
       │                │               │             │
       │ [later]        │               │             │
       │ validate()     │               │             │
       │───────────────►│               │             │
       │                │ cache fresh?  │             │
       │                │──────────────────────────────►
       │                │◄── hit ──────────────────────│
       │◄── active ─────│               │             │
```

### Wire Protocol v2 Connection

```
  Client               V2Server          Session
    │                     │                │
    │ TCP connect         │                │
    │────────────────────►│                │
    │                     │ accept → new   │
    │                     │ V2SessionImpl  │
    │                     │───────────────►│
    │ SETTINGS frame      │                │
    │────────────────────────────────────►│
    │◄── SETTINGS ACK ────────────────────│
    │                     │                │
    │ HEADERS (stream=1)  │                │
    │────────────────────────────────────►│
    │ DATA (stream=1)     │                │
    │────────────────────────────────────►│
    │                     │                │ data_handler()
    │◄── DATA (stream=1) ─────────────────│
    │◄── WINDOW_UPDATE ───────────────────│
```

---

## API Reference

### `themis::build_info`

| Function | Returns | Description |
|---|---|---|
| `getReproducibilityInfo()` | `ReproducibilityInfo` | Build metadata (git, host, toolchain) |
| `exportBuildManifest(path)` | `bool` | Write JSON manifest |
| `verifyBuildManifest(path)` | `bool` | Compare manifest to current binary |
| `getBuildConfiguration()` | `BuildConfiguration` | Compile-time feature flags |
| `getVersionSummary()` | `string` | One-line version string |

### `themis::modules`

| Class | Key Methods | Description |
|---|---|---|
| `ModuleLoader` | `loadModule()`, `loadAllModules()`, `exportAuditLog()` | Secure DLL loader |
| `AbiChecker` | `check()`, `checkVersions()`, `checkRequiredSymbols()` | ABI validation |
| `ModuleSandbox` | `launch()`, `shutdown()`, `stats()` | OS resource jail |

### `themis::network`

| Class | Key Methods | Description |
|---|---|---|
| `WireProtocolMetrics` | `recordLatency()`, `recordBytes()`, `snapshot()` | Latency/throughput tracking |
| `PayloadBufferPool` | `acquire()`, `hitRate()` | Slab allocator |
| `CompressionAdvisor` | `advise()`, `lz4Acceleration()`, `lz4HcLevel()` | Compression policy |
| `V2Server` | `start()`, `stop()`, `push_to_client()` | Multiplexed protocol server |

### `themis::license`

| Class | Key Methods | Description |
|---|---|---|
| `LicenseClient` | `activate()`, `validate()`, `refresh()`, `getMachineFingerprint()` | License activation |
