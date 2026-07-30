# ThemisDB Plugin System

## ⚠️ Current Status

**Development Status:** The plugin system is under active development with different maturity levels for different plugin types.

### Production-Ready Plugin Types
- ✅ **Blob Storage Plugins** (Azure Blob Storage, Amazon S3)
- ✅ **Image Analysis Plugins** (ONNX CLIP)
- ✅ **RPC Plugins** (gRPC)
- ✅ **Exporters** (JSONL LLM Exporter)
- ✅ **Importers** (PostgreSQL Importer)
- ✅ **Scraper Plugin v1.1.0** (Agentic gap-detection web scraping; 56-source knowledge catalog)
- 🔧 **User Storage Encrypted Plugin v0.2.0** (4-tier gocryptfs AES-256-GCM encrypted storage)

### Implemented in Source (Requires Build Configuration)
- 🔧 **Hardware Acceleration Backends** (CUDA, Vulkan, DirectX, HIP, Metal, OpenCL)
  - Source code fully implemented in `src/acceleration/`
  - Requires enabling build flags (e.g., `THEMIS_ENABLE_CUDA`, `THEMIS_ENABLE_VULKAN`)
  - BackendRegistry and PluginLoader are implemented
  - See [Hardware Acceleration Guide](#hardware-acceleration) below

---

## Overview

ThemisDB uses a flexible plugin system that allows extending functionality through dynamically loadable DLLs/shared libraries. The plugin architecture supports multiple plugin types for different purposes.

Compiled plugin implementations are being consolidated into the canonical source tree under src and exposed through public headers under include. The plugins directory remains the compatibility, manifest, roadmap, and legacy entry-point layer.

## Plugin Architecture

ThemisDB's plugin system consists of a unified plugin manager that handles dynamic loading and lifecycle management for various plugin types:

```
ThemisDB Core
    ↓
PluginManager (src/plugins/plugin_manager.cpp)
    ↓
Plugin Types:
    ├── Blob Storage    (Azure, S3)
    ├── Image Analysis  (ONNX CLIP)
    ├── Exporters       (JSONL LLM)
    ├── Importers       (PostgreSQL)
    ├── RPC             (gRPC)
    └── [Future] Compute Backends (CUDA, Vulkan, etc.)
```

**Core Components:**
- **PluginManager**: Unified plugin loading and lifecycle management (`include/plugins/plugin_manager.h`)
- **Plugin Interface**: Base interface all plugins must implement (`include/plugins/plugin_interface.h`)
- **Type-Specific Interfaces**: Domain-specific interfaces for each plugin type
  - Image Analysis: `include/plugins/image_analysis_interface.h`
  - RPC: `include/plugins/rpc_plugin_interface.h`

## Plugin Directory Structure

```
plugins/
├── README.md                           (This file)
├── PLANNED_ACCELERATION_PLUGINS.md     (Hardware acceleration usage guide)
├── CMakeLists.txt                      (Compatibility and manifest entry point)
├── blob_storage/                       ✅ Production
│   ├── README.md
│   ├── ROADMAP.md
│   ├── FUTURE_ENHANCEMENTS.md
│   ├── azure/                          (Azure Blob Storage plugin)
│   └── s3/                             (Amazon S3 plugin)
├── cuda/                               📋 Example/Template
│   ├── README.md
│   ├── ROADMAP.md
│   ├── FUTURE_ENHANCEMENTS.md
│   ├── CMakeLists.txt.example
│   ├── cuda_plugin.cpp.example
│   └── cuda_plugin.json
├── ethics_ai/                          🔧 WIP / compatibility shim to src/ethics_ai
│   ├── README.md
│   ├── ROADMAP.md
│   └── FUTURE_ENHANCEMENTS.md
├── exporters/                          ✅ Production
│   ├── README.md
│   ├── ROADMAP.md
│   ├── FUTURE_ENHANCEMENTS.md
│   └── jsonl_llm/                      (JSONL LLM exporter)
├── huggingface/                        ✅ Ready for use
│   ├── README.md
│   ├── ROADMAP.md
│   └── FUTURE_ENHANCEMENTS.md
├── image_analysis/                     ✅ Production / compatibility shim to src/onnx_clip
│   ├── README.md
│   ├── ROADMAP.md
│   ├── FUTURE_ENHANCEMENTS.md
│   └── onnx_clip/                      (ONNX CLIP embedding plugin)
├── importers/                          ✅ Production
│   ├── README.md
│   ├── ROADMAP.md
│   ├── FUTURE_ENHANCEMENTS.md
│   └── postgres/                       (PostgreSQL importer)
├── rpc/                                ✅ Production / compatibility shim to src/rpc_grpc
│   ├── README.md
│   ├── ROADMAP.md
│   ├── FUTURE_ENHANCEMENTS.md
│   └── grpc/                           (gRPC plugin)
├── scraper/                            ✅ Production v1.1.0 — agentic gap-detection web scraping
│   ├── CHANGELOG.md
│   ├── README.md
│   ├── ROADMAP.md
│   ├── FUTURE_ENHANCEMENTS.md
│   └── config/                         (knowledge_sources.yaml, scraper_urls.yaml, gov_sources.yaml)
├── themisdb_geo/                       🔌 Optional public submodule (makr-code/themisdb_geo)
├── themisdb_timeseries/                🔌 Optional public submodule (makr-code/themisdb_timeseries)
└── user_storage_encrypted/             🔧 Implemented v0.2.0 — 4-tier gocryptfs encrypted storage
  ├── CHANGELOG.md
  ├── README.md
  ├── ROADMAP.md
  └── FUTURE_ENHANCEMENTS.md

> 📄 **Per-plugin documentation:** Every plugin subdirectory contains three standard
> Markdown files: `README.md` (status, architecture, references), `ROADMAP.md`
> (planned work), and `FUTURE_ENHANCEMENTS.md` (ideas backlog).

Note: Hardware acceleration backends (CUDA, Vulkan, etc.) are implemented
in src/acceleration/ and can be enabled via build configuration. Several
runtime plugins are likewise built from src/* while plugins/* preserves
legacy CMake entry points, manifests, examples, and roadmap material.

Public module externalization (Geo / TimeSeries):
- `plugins/themisdb_geo` (`makr-code/themisdb_geo`)
- `plugins/themisdb_timeseries` (`makr-code/themisdb_timeseries`)
- Enable with `-DTHEMIS_EXTERNALIZE_GEO_PLUGIN=ON` and/or `-DTHEMIS_EXTERNALIZE_TIMESERIES_PLUGIN=ON`
- If enabled and submodule `CMakeLists.txt` is present, integrated `src/geo/*` and/or `src/timeseries/*`
  sources are removed from `themis_core` and built from the external submodule instead.
```

## Production Plugin Types

### 1. Blob Storage Plugins ✅

**Purpose:** Store large binary objects (BLOBs) in external cloud storage systems.

**Available Backends:**
- **Azure Blob Storage** (`blob_storage/azure/`)
  - Integration with Microsoft Azure Blob Storage
  - Cost-effective external storage for large objects
  
- **Amazon S3** (`blob_storage/s3/`)
  - Integration with Amazon S3 and S3-compatible services
  - Supports standard S3 API

**Documentation:** See [blob_storage/README.md](blob_storage/README.md)

**Status:** ✅ Production-ready

---

### 2. Image Analysis Plugins ✅

**Purpose:** Process and analyze image data, generate embeddings, captions, and perform object detection.

**Available Plugins:**
- **ONNX CLIP** (`image_analysis/onnx_clip/`)
  - CLIP-based image embedding generation using ONNX Runtime
  - Supports multiple backends: CPU, CUDA, DirectML, TensorRT
  - 512-dimensional embeddings (default)

**Documentation:** See [image_analysis/README.md](image_analysis/README.md)

**Status:** ✅ Production-ready

---

### 3. RPC Plugins ✅

**Purpose:** Enable inter-shard and client-server communication through RPC protocols.

**Available Plugins:**
- **gRPC** (`rpc/grpc/`)
  - High-performance gRPC-based communication
  - Supports inter-shard data transfers
  - mTLS support for secure communication

**Documentation:** See [docs/de/plugins/RPC_PLUGIN_ARCHITECTURE.md](../docs/de/plugins/RPC_PLUGIN_ARCHITECTURE.md)

**Status:** ✅ Production-ready

---

## Beta Plugin Types

### 4. Exporters ✅

**Purpose:** Export data from ThemisDB to various formats.

**Available Exporters:**
- **JSONL LLM Exporter** (`exporters/jsonl_llm/`)
  - Export data in JSONL format for LLM training
  - LoRA adapter metadata generation
  - vLLM integration support
  - **Implementation:** `src/exporters/jsonl_llm_exporter.cpp` (657 lines)

**Documentation:** See [exporters/README.md](exporters/README.md)

**Status:** ✅ Production-ready (Fully implemented with tests)

---

### 5. Importers ✅

**Purpose:** Import data into ThemisDB from external data sources.

**Available Importers:**
- **PostgreSQL Importer** (`importers/postgres/`)
  - Import data from PostgreSQL databases
  - Schema mapping and type conversion
  - **Implementation:** `src/importers/postgres_importer.cpp` (414 lines)

**Documentation:** See [importers/README.md](importers/README.md)

**Status:** ✅ Production-ready (Fully implemented with tests)

---

## Hardware Acceleration

### Status: 🔧 Implemented in Source Code

The hardware acceleration backends are **fully implemented** in the `src/acceleration/` directory but require enabling build flags to compile and use them.

**Implementation Details:**
- **CUDA Backend**: `src/acceleration/cuda_backend.cpp` + CUDA kernels
- **Vulkan Backend**: `src/acceleration/vulkan_backend_full.cpp` (18,777 lines)
- **DirectX Backend**: `src/acceleration/directx_backend_full.cpp`
- **HIP Backend**: `src/acceleration/hip_backend.cpp` (AMD GPUs)
- **Metal Backend**: `src/acceleration/metal_backend.mm` (Apple Silicon)
- **OpenCL Backend**: `src/acceleration/opencl_backend.cpp`
- **Backend Registry**: `src/acceleration/backend_registry.cpp` ✅ Implemented
- **Plugin Loader**: `src/acceleration/plugin_loader.cpp` ✅ Implemented

**Build Flags:**
```cmake
# Enable CUDA acceleration
-DTHEMIS_ENABLE_CUDA=ON

# Enable Vulkan acceleration
-DTHEMIS_ENABLE_VULKAN=ON

# Enable DirectX acceleration (Windows only)
-DTHEMIS_ENABLE_DIRECTX=ON

# Enable Metal acceleration (macOS only)
-DTHEMIS_ENABLE_METAL=ON
```

**Usage Example:**
```cpp
#include "acceleration/compute_backend.h"

// Get backend registry (singleton)
auto& registry = BackendRegistry::instance();

// Load plugins from directory
registry.loadPlugins("./plugins");

// Get best available vector backend
auto* backend = registry.getBestVectorBackend();
if (backend->type() != BackendType::CPU) {
    std::cout << "Using GPU acceleration: " << backend->name() << std::endl;
}

// Or get specific backend
auto* cudaBackend = registry.getBackend(BackendType::CUDA);
if (cudaBackend && cudaBackend->isAvailable()) {
    // Use CUDA backend
}
```

**Documentation:** See [PLANNED_ACCELERATION_PLUGINS.md](PLANNED_ACCELERATION_PLUGINS.md) for detailed usage and configuration.

**Note:** While the source code is fully implemented, you need to:
1. Install required SDKs (CUDA Toolkit, Vulkan SDK, etc.)
2. Enable the appropriate build flags
3. Link against required libraries

---

## Planned Plugin Types

### 6. Additional Plugin Categories 📋

Future plugin types under consideration:

## Plugin Development Guide

### Plugin Interface

All ThemisDB plugins must implement the `IThemisPlugin` interface defined in `include/plugins/plugin_interface.h`:

```cpp
#include "plugins/plugin_interface.h"

class MyPlugin : public IThemisPlugin {
public:
    const char* getName() const override {
        return "My Plugin";
    }
    
    const char* getVersion() const override {
        return "1.0.0";
    }
    
    PluginType getType() const override {
        return PluginType::CUSTOM;
    }
    
    PluginCapabilities getCapabilities() const override {
        return {
            .thread_safe = true,
            .supports_batching = true
        };
    }
    
    bool initialize(const char* config_json) override {
        // Initialize plugin with configuration
        return true;
    }
    
    void shutdown() override {
        // Clean up resources
    }
    
    void* getInstance() override {
        return this;
    }
};

// Export plugin entry points
THEMIS_PLUGIN_IMPL(MyPlugin)
```

### Plugin Manifest

Each plugin should include a `plugin.json` manifest file:

```json
{
  "name": "my_plugin",
  "version": "1.0.0",
  "description": "My custom plugin",
  "type": "CUSTOM",
  "binary_windows": "my_plugin.dll",
  "binary_linux": "my_plugin.so",
  "binary_macos": "my_plugin.dylib",
  "dependencies": [],
  "capabilities": {
    "thread_safe": true,
    "supports_batching": true
  },
  "auto_load": false,
  "load_priority": 100
}
```

### Building a Plugin

Create a `CMakeLists.txt` for your plugin:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_plugin VERSION 1.0.0)

add_library(my_plugin SHARED
    my_plugin.cpp
)

target_include_directories(my_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(my_plugin PRIVATE
    # Your dependencies
)

install(TARGETS my_plugin
    LIBRARY DESTINATION lib/themis/plugins
    RUNTIME DESTINATION lib/themis/plugins
)

install(FILES plugin.json
    DESTINATION lib/themis/plugins
)
```

Build the plugin:

```bash
mkdir build && cd build
cmake ..
cmake --build .
cmake --install .
```

### Plugin Naming Convention

Plugin binaries should follow these naming patterns:

| Plugin Type | Pattern | Example |
|-------------|---------|---------|
| Blob Storage | `themis_blob_<name>` | `themis_blob_azure.dll` |
| Image Analysis | `themis_image_<name>` | `themis_image_onnx_clip.so` |
| Compute Backend | `themis_accel_<name>` | `themis_accel_cuda.dll` |
| Importer | `themis_import_<name>` | `themis_import_postgres.so` |
| Exporter | `themis_export_<name>` | `themis_export_jsonl.so` |
| RPC | `themis_rpc_<name>` | `themis_rpc_grpc.dll` |

## Using Plugins

### Loading Plugins

Plugins can be loaded using the `PluginManager`:

```cpp
#include "plugins/plugin_manager.h"

auto& manager = PluginManager::instance();

// Scan plugin directory for available plugins
manager.scanPluginDirectory("./plugins");

// Load a specific plugin by name
IThemisPlugin* plugin = manager.loadPlugin("onnx_clip");

// Or load from explicit path
IThemisPlugin* plugin2 = manager.loadPluginFromPath(
    "./plugins/my_plugin.so",
    "{\"config_key\": \"config_value\"}"
);

// Auto-load plugins marked with auto_load=true
manager.autoLoadPlugins();
```

### Querying Plugins

```cpp
// Get all plugins of a specific type
auto imagePlugins = manager.getPluginsByType(PluginType::IMAGE_ANALYSIS);

// Check if plugin is loaded
if (manager.isPluginLoaded("onnx_clip")) {
    auto* plugin = manager.getPlugin("onnx_clip");
    // Use plugin...
}

// List all discovered plugins
auto manifests = manager.listPlugins();
for (const auto& manifest : manifests) {
    std::cout << manifest.name << " v" << manifest.version << std::endl;
}
```

### Plugin Search Paths

The plugin manager searches for plugins in the following standard locations:

| Platform | Standard Paths |
|----------|----------------|
| Windows | `C:/Program Files/ThemisDB/plugins`<br>`./plugins` (relative to executable) |
| Linux | `/usr/local/lib/themis/plugins`<br>`./plugins` (relative to executable) |
| macOS | `/usr/local/lib/themis/plugins`<br>`./plugins` (relative to executable) |

## Plugin Security

ThemisDB includes security features for plugin verification:

1. **Plugin Manifest Signatures**: Each plugin's `plugin.json` can be signed to ensure authenticity
2. **File Hash Verification**: Plugin binaries are verified against expected hashes
3. **Security Verification**: Plugins undergo security checks before loading

For more information, see [docs/de/plugins/MANIFEST_SIGNATURES.md](../docs/de/plugins/MANIFEST_SIGNATURES.md).

## Configuration Example

Plugins can be configured in your ThemisDB configuration file:

```yaml
plugins:
  # Plugin directory to scan
  directory: "./plugins"
  
  # Auto-load plugins on startup
  auto_load: true
  
  # Plugin-specific configurations
  image_analysis:
    default_plugin: "onnx_clip"
    onnx_clip:
      model_path: "./models/clip-vit-base-patch32.onnx"
      backend: "AUTO"
      
  blob_storage:
    default_backend: "s3"
    s3:
      region: "us-west-2"
      bucket: "my-themisdb-bucket"
```

## Troubleshooting

### Plugin Not Loading

**Symptom:**
```
Warning: Failed to load plugin: ./plugins/my_plugin.dll
```

**Solutions:**
1. Check if plugin file exists at the specified path
2. Verify file permissions (must be readable and executable)
3. Check that dependencies are installed (e.g., ONNX Runtime, CUDA Runtime)
4. Verify plugin naming convention matches expected pattern
5. Check plugin manifest is valid JSON

### Plugin Loads But Not Working

**Symptom:**
```
Loaded plugin: My Plugin v1.0.0
Error: Plugin initialization failed
```

**Solutions:**
1. Check plugin-specific configuration in config file
2. Verify required resources are available (models, credentials, etc.)
3. Check logs for detailed error messages
4. Ensure hardware/software requirements are met

### Missing Dependencies

**Symptom:**
```
Error loading shared library: libonnxruntime.so.1.12.0: cannot open shared object file
```

**Solutions:**
1. Install required dependencies via package manager or vcpkg
2. Set `LD_LIBRARY_PATH` (Linux) or `PATH` (Windows) to include dependency locations
3. Check plugin documentation for specific dependency requirements

## Example Plugins

### 1. Working with Blob Storage Plugins

See [blob_storage/README.md](blob_storage/README.md) for complete examples.

### 2. Working with Image Analysis Plugins

See [image_analysis/README.md](image_analysis/README.md) for complete examples.

### 3. Hardware Acceleration Plugin Template

For developers interested in creating hardware acceleration plugins, see the example template in the `cuda/` directory. Note that this is currently a template/example only.

## Best Practices

1. **Always Check Plugin Availability**
   ```cpp
   if (!manager.isPluginLoaded("required_plugin")) {
       // Handle missing plugin or use fallback
   }
   ```

2. **Handle Plugin Failures Gracefully**
   ```cpp
   try {
       plugin->initialize(config_json);
   } catch (const std::exception& e) {
       logger.error("Plugin init failed: {}", e.what());
       // Use alternative implementation
   }
   ```

3. **Version Compatibility**
   - Check plugin version compatibility with ThemisDB core
   - Use semantic versioning for plugins
   - Test plugins after ThemisDB upgrades

4. **Resource Management**
   - Properly unload plugins when shutting down
   - Clean up plugin resources in the shutdown() method
   - Avoid memory leaks in plugin implementations

## Documentation Links

- **Plugin Development**: 
  - [Plugin Migration Guide](../docs/de/plugins/PLUGIN_MIGRATION.md)
  - [Manifest Signatures](../docs/de/plugins/MANIFEST_SIGNATURES.md)
  - [LLM Plugin Development](../docs/en/llm/PLUGIN_DEVELOPER_GUIDE.md)
  
- **Specific Plugin Types**:
  - [Blob Storage Plugins](blob_storage/README.md)
  - [CUDA Plugin Template](cuda/README.md)
  - [Ethics AI Plugin](ethics_ai/README.md)
  - [Exporter Plugins](exporters/README.md)
  - [HuggingFace Ingestion Plugin](huggingface/README.md)
  - [Image Analysis Plugins](image_analysis/README.md)
  - [Importer Plugins](importers/README.md)
  - [RPC Plugins](rpc/README.md)
  - [Scraper Plugin v1.1.0](scraper/README.md)
  - [User Storage Encrypted Plugin v0.2.0](user_storage_encrypted/README.md)
  - [RPC Plugin Architecture](../docs/de/plugins/RPC_PLUGIN_ARCHITECTURE.md)
  
- **Future Plans**:
  - [Planned Acceleration Plugins](PLANNED_ACCELERATION_PLUGINS.md)

## Contributing

To contribute a new plugin:

1. Review existing plugin implementations for examples
2. Implement the `IThemisPlugin` interface
3. Add appropriate type-specific interface (e.g., `IImageAnalysisBackend`)
4. Create plugin manifest (`plugin.json`)
5. Add comprehensive tests
6. Document configuration options
7. Submit pull request

See [CONTRIBUTING.md](../CONTRIBUTING.md) for general contribution guidelines.

## Monitoring and Metrics

ThemisDB provides comprehensive metrics and monitoring for all loaded plugins. See [Plugin Metrics Documentation](../docs/plugins/PLUGIN_METRICS.md) for details on:

- Available metrics (timing, counts, resource usage, performance)
- API endpoints (`/api/plugins/metrics`, `/metrics`)
- Prometheus integration and Grafana dashboards
- Best practices for monitoring plugin health

## License

- **ThemisDB Core & Plugin System**: MIT License
- **Individual Plugins**: See respective plugin directories for license information
  - Most plugins are MIT licensed
  - Some plugins may have dependencies with different licenses (e.g., CUDA, DirectML)
