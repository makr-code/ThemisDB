# ThemisDB Importer Plugin Guide

> Alignment note (2026-05-31): This plugin guide is a secondary interface document.
> Authoritative current workload and target behavior are defined in:
> - `src/importers/FUTURE_ENHANCEMENTS.md`
> - `src/importers/MODULE_GAPS.md`
> - `src/importers/ROADMAP.md`
> If this guide conflicts with newer planning docs, planning docs take precedence.

This guide explains how to write a third-party importer plugin for ThemisDB using the
Plugin API defined in `include/importers/importer_plugin_api.h` and the stable C ABI
in `include/importers/importer_plugin.h`.

> **API status:** The V1 C-linkage ABI (`THEMIS_IMPORTER_PLUGIN_V1`) is stable as of
> ThemisDB v1.9.0.  It is designed for ABI evolution: future revisions (V2, V3, …) add
> fields at the end of the struct and introduce a new version constant.  A plugin compiled
> against V1 can always be loaded by a newer host.

---

## Two plugin ABIs

ThemisDB supports two plugin ABIs.  New plugins should use the **V1 C ABI** (this guide);
the legacy `createPlugin` / `destroyPlugin` C++ ABI is still supported for backwards
compatibility.

| ABI | Entry points | Load method | When to use |
|-----|-------------|-------------|-------------|
| **V1 C ABI** (recommended) | `themis_importer_create` | `ImporterRegistry::loadPlugin()` | New plugins (v1.9.0+) |
| Legacy C++ ABI | `createPlugin` / `destroyPlugin` | `ImporterPluginLoader::load()` | Existing plugins |

---

## V1 C ABI — quick-start

### Step 1: Implement the function table

```cpp
// oracle_importer.cpp
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include "importers/importer_plugin.h"   // THEMIS_IMPORTER_PLUGIN_V1 ABI

// ── Internal state ────────────────────────────────────────────────────────────
struct OracleImporterState {
    char   connection_string[512];
    bool   initialized;
    bool   cancelled;
    char   last_schema[4096];
};

// ── Lifecycle ─────────────────────────────────────────────────────────────────
static void* oracle_create(const ThemisImporterAllocator* /*alloc*/) {
    return new OracleImporterState{};
}
static void oracle_destroy(void* p, const ThemisImporterAllocator* /*alloc*/) {
    delete static_cast<OracleImporterState*>(p);
}

// ── Configuration ─────────────────────────────────────────────────────────────
static int oracle_init(void* p, const char* config_json) {
    auto* state = static_cast<OracleImporterState*>(p);
    // Parse config_json to extract "connection_string", "username", etc.
    // Credentials must NOT be logged.
    state->initialized = true;
    (void)config_json;
    return 0;  // 0 = success
}

// ── Pre-flight validation ──────────────────────────────────────────────────────
static int oracle_validate(void* p, const char* source_path,
                           char* error_buf, size_t error_buf_size) {
    (void)p;
    if (!source_path || source_path[0] == '\0') {
        if (error_buf && error_buf_size > 0)
            std::snprintf(error_buf, error_buf_size,
                          "source_path (table/view name) must not be empty");
        return 1;
    }
    return 0;
}

// ── Import ────────────────────────────────────────────────────────────────────
static int oracle_import(void* p, const char* source_path,
                         const char* /*options_json*/,
                         uint64_t* imported_out, uint64_t* failed_out) {
    auto* state = static_cast<OracleImporterState*>(p);
    if (!state->initialized) return 1;

    uint64_t imported = 0, failed = 0;

    // TODO: open an OCI session, execute SELECT * FROM <source_path>,
    //       iterate rows, write to ThemisDB sink.
    // Check state->cancelled between row batches and exit early if set.

    if (imported_out) *imported_out = imported;
    if (failed_out)   *failed_out   = failed;
    return 0;
}

// ── Schema introspection ──────────────────────────────────────────────────────
static const char* oracle_schema(void* p, const char* source_path) {
    auto* state = static_cast<OracleImporterState*>(p);
    // TODO: query USER_TAB_COLUMNS / ALL_TAB_COLUMNS and build JSON.
    (void)source_path;
    std::snprintf(state->last_schema, sizeof(state->last_schema),
                  R"({"tables":[]})");
    return state->last_schema;
}

// ── Cancel ───────────────────────────────────────────────────────────────────
static void oracle_cancel(void* p) {
    static_cast<OracleImporterState*>(p)->cancelled = true;
}
```

### Step 2: Export the factory symbol

In **one** `.cpp` file of your shared library add:

```cpp
// oracle_importer_entry.cpp
#include "importers/importer_plugin.h"
#include "oracle_importer.cpp"   // or forward-declare the functions above

THEMIS_IMPORTER_PLUGIN_V1_EXPORT(
    "oracle_importer",           // plugin name (snake_case, stable)
    "1.0.0",                     // plugin version
    oracle_create,               // create_instance
    oracle_destroy,              // destroy_instance
    oracle_init,                 // initialize
    oracle_validate,             // validate_source
    oracle_import,               // import_data
    oracle_schema,               // get_schema (may be nullptr)
    oracle_cancel                // cancel
)
```

This macro expands to:

```cpp
extern "C" THEMIS_IMPORTER_V1_EXPORT_ATTR
const THEMIS_IMPORTER_PLUGIN_V1* themis_importer_create(void) {
    return &/* static descriptor */;
}
```

### Step 3: Build the shared library

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(oracle_importer VERSION 1.0.0)

find_package(themisdb CONFIG REQUIRED)   # provides ThemisDB::importers_plugin_api

add_library(oracle_importer SHARED
    oracle_importer.cpp
    oracle_importer_entry.cpp
)

target_compile_definitions(oracle_importer PRIVATE THEMIS_IMPORTER_PLUGIN_EXPORTS)

target_link_libraries(oracle_importer PRIVATE
    ThemisDB::importers_plugin_api
    # Oracle Instant Client / OCI library
    # occi                  # Oracle C++ Call Interface
    # clntsh                # Oracle Client Shared Library
)
```

**Manual build (Linux/macOS):**

```sh
g++ -std=c++17 -fPIC -shared \
    -DTHEMIS_IMPORTER_PLUGIN_EXPORTS \
    -I /usr/include/themisdb \
    -o oracle_importer.so \
    oracle_importer.cpp oracle_importer_entry.cpp \
    -locci -lclntsh
```

### Step 4: Load the plugin at runtime

```cpp
#include "importers/importer_plugin_api.h"

// Load via ImporterRegistry (alias for ImporterPluginRegistry):
bool ok = themis::importers::ImporterRegistry::instance()
              .loadPlugin("/opt/themis/plugins/oracle_importer.so");
if (!ok) {
    std::cerr << "Load error: "
              << themis::importers::ImporterRegistry::instance().lastLoadError()
              << "\n";
    return 1;
}

// Create an importer instance:
auto importer = themis::importers::ImporterRegistry::instance()
                    .create("oracle_importer");
if (!importer) {
    std::cerr << "Plugin not found in registry\n";
    return 1;
}

themis::importers::ImportOptions opts;
auto stats = importer->importData("HR.EMPLOYEES", opts);
std::cout << "Imported " << stats.imported_records << " records\n";

// Unload when done:
themis::importers::ImporterRegistry::instance().unloadPlugin("oracle_importer");
```

---

## ABI versioning in detail

The `THEMIS_IMPORTER_PLUGIN_V1` struct is versioned via two fields:

| Field | Value | Purpose |
|-------|-------|---------|
| `abi_version` | `THEMIS_IMPORTER_PLUGIN_ABI_V1` (1) | Host rejects plugins with an unrecognised version |
| `struct_size` | `sizeof(THEMIS_IMPORTER_PLUGIN_V1)` | Host detects plugins compiled against an older (smaller) struct |

Future V2 plugins will set `abi_version = 2` and add new fields at the end of the struct.
A V2 host can load a V1 plugin by checking `abi_version == 1` and treating the missing
V2 fields as `nullptr`.

```cpp
// Compile-time guard for plugins that require V1+:
#if THEMIS_IMPORTER_PLUGIN_ABI_V1 < 1
#  error "Requires ThemisDB importer plugin ABI >= 1"
#endif
```

---

## Plugin sandbox

When a plugin is loaded via `ImporterRegistry::loadPlugin()`, each `importData()` call
runs in a **sandboxed thread** with the limits from `PluginSandboxConfig`:

```cpp
themis::importers::PluginSandboxConfig sandbox;
sandbox.memory_limit_bytes = 512UL * 1024 * 1024;  // 512 MiB
sandbox.timeout_ms         = 60'000;                 // 1 minute

themis::importers::ImporterRegistry::instance()
    .loadPlugin("/path/to/oracle_importer.so", sandbox);
```

### Memory limit

The host passes a counting allocator (`ThemisImporterAllocator`) to
`create_instance`.  Plugins **should** use this allocator for heap allocations to
participate in limit enforcement.  If cumulative bytes exceed `memory_limit_bytes`, the
allocator returns `nullptr`; the import job fails gracefully with an error in
`ImportStats::errors`.

Plugins that use system `malloc` directly are not subject to the memory limit.

### Timeout

If the import thread runs longer than `timeout_ms` milliseconds, the host calls
`cancel()` on the plugin instance and waits for the thread to join.  The import returns
an error.  Set `timeout_ms = 0` to disable the timeout.

---

## Legacy C++ ABI (retained for compatibility)

Existing plugins built against the `createPlugin` / `destroyPlugin` ABI continue to
work via `ImporterPluginLoader`:

```cpp
themis::importers::ImporterPluginLoader loader;
if (!loader.load("/opt/my_plugins/my_csv_importer.so")) {
    std::cerr << "Error: " << loader.lastError() << "\n";
}
// Plugin is now in ImporterPluginRegistry under its getName() key.
loader.unload();
```

---

## Registering built-in importers programmatically

For built-in connectors that don't need to be in a separate shared library:

```cpp
themis::importers::ImporterPluginRegistry::instance().registerFactory(
    "my_inline_importer",
    []() -> std::shared_ptr<themis::importers::IImporter> {
        return std::make_shared<MyInlineImporter>();
    });
```

---

## Advertising capabilities

Override `getCapabilities()` to let callers query supported features (C++ ABI only):

```cpp
themis::plugins::PluginCapabilities getCapabilities() const override {
    themis::plugins::PluginCapabilities caps;
    caps.supports_streaming = true;
    caps.supports_batching  = true;
    return caps;
}
```

---

## Thread-safety

`ImporterRegistry` / `ImporterPluginRegistry` is thread-safe.  Individual `IImporter`
instances are **not** thread-safe by default.  Do not share a single instance across
threads without external synchronisation.

---

## Security guidelines

- **Never** log credentials (passwords, API keys, connection tokens).  Log only
  sanitised identifiers (e.g. `host:port/database`).
- Validate every value read from `config_json` before use; reject unknown or
  malformed input.
- If your plugin reads files or network resources, validate paths and URIs to
  prevent path traversal or SSRF.
- Use the sandbox allocator where possible to let the host enforce memory limits.

---

## Reference

| Symbol | Header | Description |
|--------|--------|-------------|
| `THEMIS_IMPORTER_PLUGIN_V1` | `importers/importer_plugin.h` | Stable C ABI descriptor struct |
| `THEMIS_IMPORTER_PLUGIN_ABI_V1` | `importers/importer_plugin.h` | ABI version constant (1) |
| `THEMIS_IMPORTER_CREATE_SYMBOL` | `importers/importer_plugin.h` | Factory symbol name (`"themis_importer_create"`) |
| `ThemisImporterAllocator` | `importers/importer_plugin.h` | Allocator callbacks for sandbox tracking |
| `themis_importer_create_fn_t` | `importers/importer_plugin.h` | Factory function pointer type |
| `THEMIS_IMPORTER_PLUGIN_V1_EXPORT` | `importers/importer_plugin.h` | Macro to generate the factory entry point |
| `THEMIS_IMPORTER_V1_EXPORT_ATTR` | `importers/importer_plugin.h` | Visibility attribute for the factory symbol |
| `ImporterRegistry` | `importers/importer_plugin_api.h` | Alias for `ImporterPluginRegistry`; entry point for `loadPlugin()` |
| `ImporterPluginRegistry` | `importers/importer_plugin_api.h` | Singleton factory registry |
| `PluginSandboxConfig` | `importers/importer_plugin_api.h` | Resource limits (memory, timeout) for plugin import jobs |
| `V1ImporterAdapter` | `importers/importer_plugin_api.h` | Internal `IImporter` wrapper for V1 plugins |
| `ImporterPluginBase` | `importers/importer_plugin_api.h` | Convenience base for legacy C++ ABI plugins |
| `ImporterPluginLoader` | `importers/importer_plugin_api.h` | Legacy shared-library loader |
| `THEMIS_IMPORTER_PLUGIN_IMPL` | `importers/importer_plugin_api.h` | Legacy macro to export `createPlugin`/`destroyPlugin` |
| `THEMIS_IMPORTER_PLUGIN_API_VERSION` | `importers/importer_plugin_api.h` | Numeric API version for compile-time guards |
| `IImporter` | `importers/importer_interface.h` | Core importer interface |
| `ImportOptions` | `importers/importer_interface.h` | Import configuration |
| `ImportStats` | `importers/importer_interface.h` | Import result statistics |

---

## Performance targets

The following targets apply to the V1 plugin loading mechanism built into
`ImporterRegistry::loadPlugin()`:

| Metric | Target | Notes |
|--------|--------|-------|
| Cold `dlopen` / `LoadLibrary` | ≤ 50 ms | One-time cost per `loadPlugin()` call; subsequent `create()` calls are negligible |
| ABI version check | ≤ 1 ms | `themis_importer_create()` call + `abi_version` / `struct_size` field reads |
| Per-call import overhead | Negligible | Sandbox thread setup adds < 1 µs; plugin code dominates |

These are design targets, not hard resource limits.  Actual load times depend on
filesystem cache state, library size, and dynamic linker overhead.

