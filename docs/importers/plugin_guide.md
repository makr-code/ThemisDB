# ThemisDB Importer Plugin Guide

This guide explains how to write a third-party importer plugin for ThemisDB using the
Plugin API defined in `include/importers/importer_plugin_api.h`.

> **Stability notice:** The importer plugin API will be stabilised in ThemisDB v1.5.0.
> Breaking changes are possible before that milestone.  Gate your plugins on
> `THEMIS_IMPORTER_PLUGIN_API_VERSION` if you need compile-time guards.

---

## Overview

An importer plugin is a shared library (`.so` / `.dll` / `.dylib`) that exports
two C-linkage functions:

```c
IThemisPlugin* createPlugin();
void           destroyPlugin(IThemisPlugin*);
```

ThemisDB discovers these functions via `dlopen` / `LoadLibrary` and registers the
plugin's importer factory in `ImporterPluginRegistry`.

---

## Quick-start: five-step checklist

1. Create a class that inherits from `themis::importers::ImporterPluginBase`.
2. Implement all pure-virtual methods (see [Minimal implementation](#minimal-implementation)).
3. Place `THEMIS_IMPORTER_PLUGIN_IMPL(YourClass)` in **one** translation unit.
4. Build the shared library.
5. Load it at runtime with `ImporterPluginLoader::load("/path/to/your.so")`.

---

## Minimal implementation

```cpp
// my_oracle_importer.h
#pragma once
#include "importers/importer_plugin_api.h"

class MyOracleImporter : public themis::importers::ImporterPluginBase {
public:
    // ── IThemisPlugin identifiers ──────────────────────────────────────────
    const char* getName()    const override { return "my_oracle_importer"; }
    const char* getVersion() const override { return "0.1.0"; }

    // ── IImporter ─────────────────────────────────────────────────────────
    std::vector<std::string> getSupportedTypes() const override {
        return {"oracle"};
    }

    bool initialize(const std::string& config_json) override {
        // Parse config_json (connection string, credentials, etc.)
        // Return false if the configuration is invalid.
        return true;
    }

    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override {
        if (source_path.empty()) {
            errors.push_back("source_path must not be empty");
            return false;
        }
        // Additional validation (e.g., test the connection)
        return true;
    }

    themis::importers::ImportStats importData(
        const std::string& source_path,
        const themis::importers::ImportOptions& options,
        themis::importers::ProgressCallback cb = nullptr) override
    {
        themis::importers::ImportStats stats;
        // Implement your import logic here.
        // Increment stats.imported_records / stats.failed_records as you go.
        return stats;
    }

    std::shared_ptr<themis::importers::ImportHandle> importDataAsync(
        const std::string& source_path,
        const themis::importers::ImportOptions& options) override
    {
        auto handle = std::make_shared<themis::importers::ImportHandle>();
        handle->id      = "oracle-async-job";
        handle->running.store(true);
        auto promise = std::make_shared<std::promise<themis::importers::ImportStats>>();
        handle->future  = promise->get_future().share();
        std::thread([this, source_path, options, handle, promise]() {
            auto stats = importData(source_path, options);
            handle->running.store(false);
            promise->set_value(stats);
        }).detach();
        return handle;
    }

    void cancel() override {
        cancelled_.store(true);
    }

    nlohmann::json getSourceSchema(const std::string& /*source_path*/) override {
        // Return a JSON description of the source schema, e.g.:
        //   { "tables": [ { "name": "users", "columns": [...] } ] }
        return nlohmann::json::object();
    }

private:
    std::atomic<bool> cancelled_{false};
};
```

In **one** `.cpp` file of your shared library, add:

```cpp
#include "my_oracle_importer.h"
THEMIS_IMPORTER_PLUGIN_IMPL(MyOracleImporter)
```

This macro expands to the required `createPlugin` / `destroyPlugin` C-linkage entry
points.

---

## Building the shared library

### CMake (recommended)

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_oracle_importer)

find_package(themisdb CONFIG REQUIRED)   # provides ThemisDB::importers_plugin_api

add_library(my_oracle_importer SHARED
    my_oracle_importer.cpp
)

target_link_libraries(my_oracle_importer PRIVATE
    ThemisDB::importers_plugin_api
    # your Oracle client library, e.g. Oracle::OCCI
)
```

### Compiler flags (manual)

```sh
# Linux / macOS
g++ -std=c++17 -fPIC -shared \
    -I /usr/include/themisdb \
    -o my_oracle_importer.so \
    my_oracle_importer.cpp \
    -locci
```

---

## Loading the plugin at runtime

```cpp
#include "importers/importer_plugin_api.h"

themis::importers::ImporterPluginLoader loader;

if (!loader.load("/opt/my_plugins/my_oracle_importer.so")) {
    std::cerr << "Failed to load plugin: " << loader.lastError() << "\n";
    return 1;
}

// The plugin is now registered in ImporterPluginRegistry.
auto importer = themis::importers::ImporterPluginRegistry::instance()
                    .create("my_oracle_importer");
if (!importer) {
    std::cerr << "Plugin not found in registry\n";
    return 1;
}

themis::importers::ImportOptions opts;
auto stats = importer->importData("oracle://host:1521/orcl", opts);
std::cout << "Imported " << stats.imported_records << " records\n";

// Unloading closes the library and removes the factory from the registry.
loader.unload();
```

---

## Registering built-in importers programmatically

You can also register importers directly (without a shared library) using the
factory API, e.g. for built-in connectors:

```cpp
themis::importers::ImporterPluginRegistry::instance().registerFactory(
    "my_inline_importer",
    []() -> std::unique_ptr<themis::importers::IImporter> {
        return std::make_unique<MyInlineImporter>();
    });
```

---

## Advertising capabilities

Override `getCapabilities()` to let callers query supported features:

```cpp
themis::plugins::PluginCapabilities getCapabilities() const override {
    themis::plugins::PluginCapabilities caps;
    caps.supports_streaming = true;   // importDataStreaming() is optimised
    caps.supports_batching  = true;   // honours ImportOptions::batch_size
    return caps;
}
```

---

## Versioning

The compile-time API version token is available for forward-compatibility guards:

```cpp
// THEMIS_IMPORTER_PLUGIN_API_VERSION encodes the version as:
//   MAJOR * 10000 + MINOR * 100 + PATCH
// So 10000 == 1.0.0, 10100 == 1.1.0, 20000 == 2.0.0, etc.
#if THEMIS_IMPORTER_PLUGIN_API_VERSION < 10000
#  error "Requires ThemisDB importer plugin API >= 1.0.0"
#endif
```

---

## Thread-safety

`ImporterPluginRegistry` is thread-safe.  Individual `IImporter` instances are
**not** thread-safe by default.  Do not share a single instance across threads
without external synchronisation.

---

## Security guidelines

- **Never** log credentials (passwords, API keys, connection tokens).  Log only
  sanitised identifiers (e.g. `host:port/database`).
- Validate every value read from `config_json` before use; reject unknown or
  malformed input with `initialize()` returning `false`.
- If your plugin reads files or network resources, validate paths and URIs to
  prevent path traversal or SSRF.

---

## Reference

| Symbol | Header | Description |
|--------|--------|-------------|
| `ImporterPluginBase` | `importers/importer_plugin_api.h` | Convenience base combining `IImporter` + `IThemisPlugin` |
| `ImporterPluginRegistry` | `importers/importer_plugin_api.h` | Singleton factory registry |
| `ImporterPluginLoader` | `importers/importer_plugin_api.h` | Shared-library loader |
| `ImporterPluginDescriptor` | `importers/importer_plugin_api.h` | Lightweight plugin descriptor |
| `THEMIS_IMPORTER_PLUGIN_IMPL` | `importers/importer_plugin_api.h` | Macro to export C-linkage entry points |
| `THEMIS_IMPORTER_PLUGIN_API_VERSION` | `importers/importer_plugin_api.h` | Numeric API version for compile-time guards |
| `IImporter` | `importers/importer_interface.h` | Core importer interface |
| `ImportOptions` | `importers/importer_interface.h` | Import configuration |
| `ImportStats` | `importers/importer_interface.h` | Import result statistics |
