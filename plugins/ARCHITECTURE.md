# ThemisDB Plugin Architecture

**Datum:** 2026-08-07  
**Status:** Active (Wave-1 Private Plugin Strategy)  
**Primary (Quelle der Wahrheit):** src/plugins/ROADMAP.md, ROADMAP.md, FUTURE_ENHANCEMENTS.md, include/plugins/  
**Bezug:** T5 Plugin Boundary (T0–T5 trust model); private plugin externalization

---

## Overview

The `plugins/` directory contains optional, runtime-loadable plugin modules that extend ThemisDB's core database functionality. Plugins implement a standardized C++ interface contract and are loaded dynamically at startup.

**Key Principles:**
1. **Optional**: Plugins are not required to run ThemisDB; all core functionality is always integrated in `src/`
2. **Standardized Interface**: All plugins (public and private) implement the same C++ SDK defined in `include/plugins/`
3. **Manifest-Driven**: Plugin capabilities and edition availability are declared in JSON manifests; loaders validate declaratively
4. **Fail-Closed**: Missing or unsupported plugins do not break core operation; fallback behaviors are defined per plugin type
5. **Audit Trail**: Plugin load decisions (success/failure/disabled) are logged for governance

---

## Directory Structure

```
plugins/
├── CMakeLists.txt                 # Plugin ecosystem build orchestration
├── ARCHITECTURE.md                # This file
├── README.md                      # Plugin development guide
├── PLANNED_ACCELERATION_PLUGINS.md # Future acceleration/hardware backends

# Public Plugins (in source tree)
├── blob_storage/                  # S3/Azure/local blob storage
├── cuda/                          # CUDA acceleration backend
├── ethics_ai/                     # Reference ethics evaluation impl
├── exporters/                     # Export drivers (Parquet, Arrow, CSV, JSON, JSONL)
├── huggingface/                   # HuggingFace model ingestion
├── image_analysis/                # Image analysis (ONNX CLIP reference)
├── importers/                     # Import drivers (PostgreSQL, MySQL reference)
├── llama_cpp/                     # llama.cpp inference binding
├── rpc/                           # Adapter factory for database compatibility
├── saga/                          # SAGA transaction coordinator
├── scraper/                       # Web scraper integrations
├── stable_diffusion/              # Stable Diffusion image generation
├── user_storage_encrypted/        # Reference field-level encryption
├── whisper/                       # OpenAI Whisper STT binding
└── themisdb_plugin_signer/        # Plugin manifest signing/verification

# Private Plugin Submodules (checked out only with .gitmodules)
└── private/
    └── README.md                  # Private plugin guidelines
    ├── themisdb_ethic_ai/         # (optional submodule) Private ethics_ai plugin
    ├── themisdb_storage/          # (optional submodule) User encryption, blob storage
    ├── themisdb_importer/         # (optional submodule) MySQL, Mongo, Kafka, S3 importers
    └── themisdb_llm_wiki/         # (optional submodule) LLM Wiki enterprise plugin
```

---

## Plugin Classification

### Public Plugins (Community+)

These plugins are always present in the source tree and available to all editions (Community, Enterprise, Hyperscaler, Military).

| Plugin | Edition | Status | Purpose |
|--------|---------|--------|---------|
| **blob_storage** | Community+ | Stable | Object storage backends (S3, Azure, GCS, local) |
| **cuda** | Community+ | Stable | NVIDIA CUDA GPU acceleration |
| **ethics_ai** | Community+ | Stable | Reference ethics evaluation implementation |
| **exporters** | Community+ | Stable | Export to Parquet, Arrow, CSV, JSON, JSONL |
| **huggingface** | Community+ | Stable | HuggingFace model hub ingestion |
| **image_analysis** | Community+ | Beta | Image classification and tagging (ONNX CLIP) |
| **importers** | Community+ | Stable | PostgreSQL and MySQL reference importers |
| **llama_cpp** | Community+ | Stable | llama.cpp local inference engine binding |
| **rpc** | Community+ | Stable | Database adapter factory (PostgreSQL, MySQL, etc.) |
| **saga** | Community+ | Stable | SAGA distributed transaction coordinator |
| **scraper** | Community+ | Beta | Web scraper integrations |
| **stable_diffusion** | Community+ | Beta | Stable Diffusion image generation |
| **user_storage_encrypted** | Community+ | Stable | Reference field-level encryption implementation |
| **whisper** | Community+ | Stable | OpenAI Whisper speech-to-text binding |
| **themisdb_plugin_signer** | Enterprise+ | Stable | Plugin manifest signing and verification |

### Private Plugins (Wave 1+)

These plugins are provided as optional, commit-pinned submodules under `plugins/private/` or external repositories. They are only available in Enterprise, Hyperscaler, and Military editions.

**Current Wave-1 Private Plugins (2026-08-07):**

| Plugin | Repository | Path | Edition | Status | Wave |
|--------|------------|------|---------|--------|------|
| **themisdb_ethic_ai** | makr-code/themisdb_ethic_ai | plugins/themisdb_ethic_ai/ | Enterprise+ | Submodule (commit-pinned) | Wave 1 |
| **themisdb_storage** | makr-code/themisdb_storage | plugins/themisdb_storage/ | Enterprise+ | Submodule (commit-pinned) | Wave 1 |
| **themisdb_importer** | makr-code/themisdb_importer | plugins/themisdb_importer/ | Enterprise+ | Submodule (commit-pinned) | Wave 1 |
| **themisdb_llm_wiki** | makr-code/themisdb_llm_wiki | plugins/themisdb_llm_wiki/ | Enterprise+ | Submodule (commit-pinned) | Wave 1 |

**Wave-2 Candidates (Planned):**
- `gpu-impact-analysis` — GPU performance profiling and cost analysis (deferred until SDK/ABI stable)

---

## Public Plugin SDK

### Interface Definition

**Location:** `include/plugins/plugin_interface.h`

All plugins (public and private) implement this standardized C++ interface:

```cpp
namespace themis::plugins {

// Plugin status codes
enum class Status : int {
  OK = 0,
  Error = 1,
  PermissionDenied = 2,
  InvalidArgument = 3,
  NotFound = 4,
  AlreadyExists = 5,
  ResourceExhausted = 6,
  Unavailable = 7,
  // ... (full list in header)
};

// Plugin capabilities
struct PluginCapabilities {
  std::string name;
  std::string version;
  std::vector<std::string> features;  // e.g., ["export_parquet", "export_arrow"]
  std::vector<std::string> backends;  // e.g., ["cuda", "hip"]
};

// Plugin initialization config (JSON)
struct PluginConfig {
  std::string json_config;  // Plugin-specific JSON config
  std::string plugin_dir;   // Directory where plugin resides
  std::string core_version; // ThemisDB core version
};

// Base plugin interface
class IPlugin {
public:
  virtual ~IPlugin() = default;
  
  // Lifecycle
  virtual Status initialize(const PluginConfig& cfg) = 0;
  virtual Status shutdown() = 0;
  
  // Discovery
  virtual PluginCapabilities capabilities() const = 0;
  virtual Status validate_license(const std::string& feature) const = 0;
  
  // Health
  virtual Status health_check() = 0;
  
  // Metrics (optional)
  virtual std::string get_metrics_json() const { return "{}"; }
};

// Factory function (C linkage)
extern "C" {
  IPlugin* create_plugin();
  void destroy_plugin(IPlugin* plugin);
}

}  // namespace themis::plugins
```

### Manifest Schema (v2)

**Location:** `include/plugins/manifest_schema_v2.json`

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["name", "version", "type"],
  "properties": {
    "name": {
      "type": "string",
      "description": "Plugin name (e.g., 'themisdb_ethic_ai', 'blob_storage')"
    },
    "version": {
      "type": "string",
      "description": "Semantic version (e.g., '1.0.0')"
    },
    "type": {
      "type": "string",
      "enum": ["backend", "adapter", "export", "transform", "tool"],
      "description": "Plugin functional category"
    },
    "visibility": {
      "type": "string",
      "enum": ["public", "private"],
      "default": "public",
      "description": "Visibility scope"
    },
    "allowed_editions": {
      "type": "array",
      "items": {
        "enum": ["minimal", "community", "enterprise", "hyperscaler", "military"]
      },
      "default": ["community", "enterprise", "hyperscaler", "military"],
      "description": "Allowed ThemisDB editions"
    },
    "min_themisdb_version": {
      "type": "string",
      "description": "Minimum ThemisDB core version (e.g., '2.4.0')"
    },
    "max_themisdb_version": {
      "type": "string",
      "description": "Maximum ThemisDB core version (e.g., '2.5.0-beta')"
    },
    "core_abi_compatible": {
      "type": "string",
      "description": "Compatible core ABI version (e.g., '2.4.x')"
    },
    "dependencies": {
      "type": "array",
      "items": {
        "type": "object",
        "properties": {
          "name": { "type": "string" },
          "version": { "type": "string" }
        }
      },
      "description": "Plugin dependencies (other plugins or libraries)"
    },
    "features": {
      "type": "array",
      "items": { "type": "string" },
      "description": "List of features this plugin provides"
    },
    "license_features": {
      "type": "array",
      "items": { "type": "string" },
      "description": "License features required (e.g., ['AI_ETHICS_EVAL', 'PRIVATE_STORAGE'])"
    },
    "capabilities": {
      "type": "object",
      "description": "Plugin capability descriptors (backend-specific)"
    },
    "author": { "type": "string" },
    "repository": { "type": "string" },
    "license": { "type": "string" },
    "description": { "type": "string" }
  }
}
```

### Example Manifest

```json
{
  "name": "themisdb_ethic_ai",
  "version": "1.0.0",
  "type": "tool",
  "visibility": "private",
  "allowed_editions": ["enterprise", "hyperscaler", "military"],
  "min_themisdb_version": "2.4.0",
  "max_themisdb_version": "2.5.x",
  "core_abi_compatible": "2.4.x",
  "features": ["ethical_discourse", "bias_detection", "fairness_metrics"],
  "license_features": ["AI_ETHICS_EVAL"],
  "dependencies": [
    { "name": "themisdb_llm_wiki", "version": ">=1.0.0" }
  ],
  "author": "makr-code",
  "repository": "github.com/makr-code/themisdb_ethic_ai",
  "license": "Commercial",
  "description": "Private ethics evaluation and discourse engine"
}
```

---

## Plugin Loading & Validation

### Load Time (Startup)

1. **Discovery**: Plugin manager scans `plugins/` and `plugins/private/` for `plugin.json` manifests
2. **Validation**: Each manifest is validated against `manifest_schema_v2.json`
3. **Edition Check**: If `allowed_editions` excludes current edition, plugin is marked disabled (not an error)
4. **Dependency Resolution**: Check if plugin dependencies are available and compatible
5. **ABI Compatibility**: Verify `core_abi_compatible` matches running core version
6. **Signature Verification** (Enterprise+): Check plugin SHA-256 hash if signed
7. **Load or Defer**: Load plugin if all checks pass; defer if dependencies missing or not yet loaded
8. **Health Check**: Call `health_check()` to verify plugin startup succeeded

### Runtime (Operation)

1. **Feature Gate**: Before using plugin feature, call `validate_license(feature_name)` to check edition/license
2. **Status Handling**: All plugin API calls return `Status`; interpret errors as either recoverable or fatal
3. **Metrics**: Plugin manager periodically calls `get_metrics_json()` for observability

### Fail-Closed Behavior

- **Missing plugin**: Core continues with fallback behavior (usually a capability reduction)
- **Load error**: Plugin is disabled; log error and continue (not fatal)
- **License denied**: Feature access denied; return `Status::PermissionDenied`
- **Dependency unmet**: Plugin disabled; retry on next startup

---

## Plugin Types & Patterns

### Backend Plugins

Provide implementations of a data storage, compute, or networking backend.

**Examples:** cuda, blob_storage, llama_cpp, whisper

**Pattern:**
```cpp
class CudaBackend : public IPlugin {
  Status initialize(const PluginConfig& cfg) override;
  Status execute_kernel(const KernelRequest& req, KernelResult& res);
};
```

### Adapter Plugins

Provide compatibility layers with external systems or formats.

**Examples:** rpc (database adapters), exporters (format adapters)

**Pattern:**
```cpp
class PostgresAdapter : public IDatabaseAdapter {
  Status connect(const ConnectionString& connstr) override;
  Status execute_query(const std::string& sql) override;
};
```

### Transform Plugins

Provide data transformation or analysis capabilities.

**Examples:** exporters, importers, scraper

**Pattern:**
```cpp
class ParquetExporter : public IExporter {
  Status export_batch(const DataBatch& batch) override;
};
```

### Tool Plugins

Provide auxiliary tools or services (not data processing).

**Examples:** ethics_ai, stable_diffusion, themisdb_plugin_signer

**Pattern:**
```cpp
class EthicsEvaluator : public IPlugin {
  Status evaluate(const DecisionContext& ctx, EthicsScore& score) override;
};
```

---

## Plugin Loading & CMake Integration

### CMake Flags

**Build Control:**
```cmake
option(BUILD_PLUGINS "Build public plugin ecosystem" ON)
option(BUILD_PLUGIN_BLOB_STORAGE "Build blob storage plugin" ON)
option(BUILD_PLUGIN_CUDA "Build CUDA acceleration plugin" ON)
# ... per-plugin controls

# Private plugins
option(WITH_PRIVATE_ETHICS_AI "Enable private ethics_ai plugin" OFF)
option(WITH_PRIVATE_STORAGE "Enable private storage/encryption plugin" OFF)
option(WITH_PRIVATE_IMPORTER "Enable private importer suite" OFF)
# All default to OFF for Community/Minimal
```

**Build Output:**
```cmake
# Public plugin .so files
add_library(themisdb_blob_storage SHARED plugins/blob_storage/...)
add_library(themisdb_cuda SHARED plugins/cuda/...)

# Private plugin submodules (optional)
if(WITH_PRIVATE_ETHICS_AI AND EXISTS plugins/themisdb_ethic_ai)
  add_library(themisdb_ethic_ai SHARED plugins/themisdb_ethic_ai/...)
endif()
```

### Installation & Loading

```
themisdb/bin/
  themisdb-server

themisdb/lib/plugins/
  libthemisdb_blob_storage.so           # Public plugins (always present if BUILD_PLUGINS=ON)
  libthemisdb_cuda.so
  libthemisdb_exporters.so
  # ...
  libthemisdb_ethic_ai.so               # Private plugins (if WITH_PRIVATE_*=ON and submodule checked out)
  libthemisdb_storage.so
  # ...
```

**Startup Search Path:**
1. `$THEMISDB_PLUGIN_DIR` (env var)
2. `./plugins/` (relative to executable)
3. `{install_prefix}/lib/plugins/`
4. `/usr/local/lib/themisdb/plugins/`
5. `/usr/lib/themisdb/plugins/`

---

## Edition Availability & Gating

### Edition Matrix

| Plugin | Minimal | Community | Enterprise | Hyperscaler | Military |
|--------|---------|-----------|------------|-------------|----------|
| blob_storage | ✅ | ✅ | ✅ | ✅ | ✅ |
| cuda | ⚠️ Limited | ✅ | ✅ | ✅ | ✅ |
| ethics_ai (public ref) | ⚠️ Limited | ✅ | ✅ | ✅ | ✅ |
| exporters | ✅ | ✅ | ✅ | ✅ | ✅ |
| huggingface | ⚠️ Limited | ✅ | ✅ | ✅ | ✅ |
| image_analysis | ⚠️ Limited | ✅ | ✅ | ✅ | ✅ |
| importers (public ref) | ⚠️ Limited | ✅ | ✅ | ✅ | ✅ |
| llama_cpp | ✅ | ✅ | ✅ | ✅ | ✅ |
| rpc | ✅ | ✅ | ✅ | ✅ | ✅ |
| saga | ✅ | ✅ | ✅ | ✅ | ✅ |
| scraper | ⚠️ Limited | ✅ | ✅ | ✅ | ✅ |
| stable_diffusion | ⚠️ Limited | ✅ | ✅ | ✅ | ✅ |
| user_storage_encrypted (public ref) | ⚠️ Limited | ✅ | ✅ | ✅ | ✅ |
| whisper | ✅ | ✅ | ✅ | ✅ | ✅ |
| themisdb_plugin_signer | ❌ | ❌ | ✅ | ✅ | ✅ |
| themisdb_ethic_ai (private) | ❌ | ❌ | ✅ | ✅ | ✅ |
| themisdb_storage (private) | ❌ | ❌ | ✅ | ✅ | ✅ |
| themisdb_importer (private) | ❌ | ❌ | ✅ | ✅ | ✅ |
| themisdb_llm_wiki (private) | ❌ | ❌ | ✅ | ✅ | ✅ |

**Legend:**
- ✅ = Full support
- ⚠️ = Limited (may have reduced features or limited backends)
- ❌ = Not available

### License Feature Gating

Plugins can require specific license features. Feature validation happens at runtime:

```cpp
// In plugin user code
Status status = plugin->validate_license("AI_ETHICS_EVAL");
if (status != Status::OK) {
  // Feature not available in current license
  return Status::PermissionDenied;
}
```

---

## Plugin Development Workflow

### For Public Plugins

1. Create directory under `plugins/`
2. Implement `IPlugin` interface
3. Create `plugin.json` manifest with `visibility: "public"`
4. Add to `plugins/CMakeLists.txt` with optional `BUILD_PLUGIN_*` flag
5. Write tests in `tests/plugin_<name>/`
6. Document in `plugins/<name>/README.md` and `plugins/<name>/ROADMAP.md`

### For Private Plugins

1. Create separate Git repository (`makr-code/themisdb_*`)
2. Implement `IPlugin` interface (same as public)
3. Create `plugin.json` manifest with `visibility: "private"` and `allowed_editions: [enterprise, ...]`
4. Add `.gitmodules` entry: `plugins/themisdb_*` → `makr-code/themisdb_*`
5. Create corresponding CMake files in `cmake/PrivatePlugins.cmake`
6. Document private/license features in manifest
7. Sign manifest and plugin .so with enterprise certificate

### Plugin Testing

**Unit Tests:**
```cpp
// tests/plugin_<name>/test_<name>.cpp
TEST(PluginTest, Initialize) {
  auto cfg = create_test_config();
  auto plugin = std::make_unique<YourPlugin>();
  EXPECT_EQ(Status::OK, plugin->initialize(cfg));
}
```

**Integration Tests:**
```cpp
// tests/plugin_<name>/test_<name>_integration.cpp
TEST(PluginIntegrationTest, LoadFromDisk) {
  auto mgr = PluginManager::create();
  EXPECT_TRUE(mgr->load_plugin("plugins/libthemisdb_<name>.so"));
  auto plugin = mgr->get_plugin("<name>");
  EXPECT_NE(nullptr, plugin);
}
```

**Edition Gating Tests:**
```cpp
// Verify plugin is disabled in Community
TEST(PluginEditionTest, PrivatePluginDisabledInCommunity) {
  auto mgr = PluginManager::create_with_edition(Edition::Community);
  auto plugin = mgr->get_plugin("themisdb_ethic_ai");
  EXPECT_EQ(nullptr, plugin);  // Private plugin not loaded in Community
}
```

---

## Plugin Documentation Requirements

Each plugin directory MUST include:

| File | Purpose |
|------|---------|
| **plugin.json** | Manifest (required) |
| **README.md** | Development guide, API docs, examples |
| **ROADMAP.md** | Status, planned features, phases |
| **FUTURE_ENHANCEMENTS.md** | Open enhancement backlog |
| **ARCHITECTURE.md** | Internal design, subcomponents (if complex) |

---

## Security Considerations

### Plugin Sandboxing (Planned)

- Plugins run in the same process as core (current state)
- Future: WebAssembly or separate process sandbox for untrusted plugins
- Capability-based security model: plugins only get features they declare

### Manifest Signing

- Private plugins' manifests can be signed with enterprise certificate
- Signature verification prevents manifest tampering
- Unsigned public plugins are trusted (in tree, version controlled)

### License Enforcement

- Core calls `validate_license(feature)` before enabling plugin features
- License provider validates at startup; features are enabled/disabled persistently
- No runtime license checks (fail-closed at startup is simpler and more secure)

---

## Reference Documents

- **src/plugins/ROADMAP.md** — Plugin system development roadmap
- **ROADMAP.md** — Private plugin externalization timeline (Wave 1–2)
- **FUTURE_ENHANCEMENTS.md** — Open enhancement backlog
- **ai_context/ARCHITECTURE_CLASSIFICATION.md** — T0–T5 tier model and module classification
- **ai_context/MODULES_AND_NAMESPACES.md** — 62 integrated modules with plugin versions listed
- **BRANCHING_STRATEGY.md** — Branch/edition naming
- **RELEASE_STRATEGY.md** — Release lane and packaging

---

**Zuletzt geprueft (Plugin Architecture):** 2026-08-07
