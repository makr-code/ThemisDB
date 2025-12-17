# ThemisDB Enterprise Build Guide

**Version:** 1.0.0  
**Last Updated:** December 2025

---

## Overview

ThemisDB enterprise modules are built as separate DLLs/shared libraries that can be dynamically loaded at runtime. This guide explains how to build enterprise modules.

## Prerequisites

- CMake 3.20+
- C++20 compiler (MSVC 2019+, GCC 11+, Clang 14+)
- vcpkg package manager
- Valid enterprise license file

## Build Options

### Community Edition (Default)
```bash
# Build only core features (no enterprise modules)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Enterprise Edition (All Modules)
```bash
# Enable enterprise build with all modules
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_BUILD_ENTERPRISE=ON \
  -DTHEMIS_ENTERPRISE_SHARDING=ON \
  -DTHEMIS_ENTERPRISE_GPU=ON \
  -DTHEMIS_ENTERPRISE_ANALYTICS=ON \
  -DTHEMIS_ENTERPRISE_REPLICATION=ON \
  -DTHEMIS_ENTERPRISE_SECURITY=ON \
  -DTHEMIS_ENTERPRISE_MANAGEMENT=ON \
  -DTHEMIS_ENTERPRISE_CONTENT=ON

# Build all targets
cmake --build build --target themis_enterprise_all
```

### Selective Module Build
```bash
# Build only specific modules (e.g., sharding + GPU)
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_BUILD_ENTERPRISE=ON \
  -DTHEMIS_ENTERPRISE_SHARDING=ON \
  -DTHEMIS_ENTERPRISE_GPU=ON

cmake --build build
```

## Build Output

Enterprise DLLs are output to:
- **Windows:** `build/lib/enterprise/*.dll`
- **Linux:** `build/lib/enterprise/*.so`
- **macOS:** `build/lib/enterprise/*.dylib`

Example structure:
```
build/
├── bin/
│   └── themis_server(.exe)          # Main server
└── lib/
    └── enterprise/
        ├── themis_enterprise_sharding.dll
        ├── themis_enterprise_gpu.dll
        ├── themis_enterprise_analytics.dll
        ├── themis_enterprise_replication.dll
        ├── themis_enterprise_security.dll
        ├── themis_enterprise_management.dll
        └── themis_enterprise_content.dll
```

## License Configuration

1. **Copy example license:**
   ```bash
   cp config/enterprise_license.example.json config/enterprise_license.json
   ```

2. **Edit license file:**
   - Replace `license_key` with your actual license
   - Update `organization` name
   - Set appropriate `expiry_date`
   - Select enabled `modules`
   - Configure `limits` (max_nodes, max_cores, etc.)

3. **Configure server to load license:**
   ```yaml
   # config/config.yaml
   enterprise:
     enabled: true
     license_path: "config/enterprise_license.json"
     plugin_dir: "lib/enterprise"
   ```

## Running with Enterprise Modules

### Linux/macOS
```bash
# Ensure enterprise libraries can be found
export LD_LIBRARY_PATH=$PWD/build/lib/enterprise:$LD_LIBRARY_PATH

# Run server
./build/bin/themis_server --config config/config.yaml
```

### Windows (PowerShell)
```powershell
# Set PATH to include enterprise DLLs
$env:PATH = "$PWD\build\lib\enterprise;$env:PATH"

# Run server
.\build\bin\themis_server.exe --config config\config.yaml
```

## Verification

Check server logs for enterprise module loading:
```
[info] License loaded successfully for organization: Example Corporation
[info] License expires: 1767225600
[info] Enabled modules: sharding, gpu, analytics, replication, security, management, content
[info] Loading enterprise plugin: lib/enterprise/themis_enterprise_sharding.dll
[info] Successfully loaded enterprise plugin: Sharding v1.0.0
[info] Loading enterprise plugin: lib/enterprise/themis_enterprise_gpu.dll
[info] Successfully loaded enterprise plugin: GPU v1.0.0
...
```

Query enterprise status via API:
```bash
curl http://localhost:8765/enterprise/status
```

Expected response:
```json
{
  "license": {
    "organization": "Example Corporation",
    "edition": "enterprise",
    "expires": "2026-12-31",
    "is_valid": true
  },
  "modules": [
    {"name": "Sharding", "version": "1.0.0", "loaded": true},
    {"name": "GPU", "version": "1.0.0", "loaded": true},
    {"name": "Analytics", "version": "1.0.0", "loaded": true},
    {"name": "Replication", "version": "1.0.0", "loaded": true},
    {"name": "Security", "version": "1.0.0", "loaded": true},
    {"name": "Management", "version": "1.0.0", "loaded": true},
    {"name": "Content", "version": "1.0.0", "loaded": true}
  ]
}
```

## Troubleshooting

### DLL Not Found (Windows)
**Error:** `The specified module could not be found`

**Solution:**
- Ensure `PATH` includes `build\lib\enterprise`
- Check that all dependencies are in vcpkg
- Use Dependency Walker to check missing DLLs

### Symbol Not Found (Linux)
**Error:** `undefined symbol: createPlugin`

**Solution:**
- Verify THEMIS_ENTERPRISE_EXPORTS is defined when building DLL
- Check symbol visibility: `nm -D libthemis_enterprise_*.so | grep createPlugin`
- Ensure C linkage for factory functions

### License Validation Failed
**Error:** `License validation failed for module: Sharding`

**Solution:**
- Check license file exists: `config/enterprise_license.json`
- Verify license has not expired
- Ensure module is listed in `modules` array
- Check license key format

### Plugin API Version Mismatch
**Error:** `Plugin API version mismatch: expected 1, got 0`

**Solution:**
- Rebuild all enterprise modules with same codebase
- Ensure plugin headers match core headers
- Clear CMake cache and rebuild

## Development

### Adding a New Enterprise Module

1. **Create module directory:**
   ```bash
   mkdir src/enterprise/mymodule
   ```

2. **Implement plugin:**
   ```cpp
   // src/enterprise/mymodule/mymodule_plugin.cpp
   #define THEMIS_ENTERPRISE_EXPORTS
   #include "enterprise/enterprise_plugin.h"
   
   class MyModulePlugin : public EnterprisePluginBase {
       // Implementation
   };
   
   extern "C" {
       THEMIS_ENTERPRISE_API IEnterprisePlugin* createPlugin() {
           return new MyModulePlugin();
       }
       // ...
   }
   ```

3. **Add to CMake:**
   ```cmake
   # src/enterprise/CMakeLists.txt
   option(THEMIS_ENTERPRISE_MYMODULE "Build mymodule" OFF)
   
   if(THEMIS_ENTERPRISE_MYMODULE)
       themis_add_enterprise_module(mymodule
           mymodule/mymodule_plugin.cpp
       )
   endif()
   ```

4. **Build and test:**
   ```bash
   cmake -B build -DTHEMIS_ENTERPRISE_MYMODULE=ON
   cmake --build build
   ```

## Distribution

### Community Distribution
```
themisdb-community-1.0.0/
├── bin/
│   └── themis_server
├── lib/
│   └── themis_core.so
└── config/
    └── config.yaml
```

### Enterprise Distribution
```
themisdb-enterprise-1.0.0/
├── bin/
│   └── themis_server
├── lib/
│   ├── themis_core.so
│   └── enterprise/
│       ├── themis_enterprise_sharding.so
│       ├── themis_enterprise_gpu.so
│       ├── themis_enterprise_analytics.so
│       ├── themis_enterprise_replication.so
│       ├── themis_enterprise_security.so
│       ├── themis_enterprise_management.so
│       └── themis_enterprise_content.so
└── config/
    ├── config.yaml
    └── enterprise_license.json (customer-specific)
```

## Support

For enterprise support:
- Email: enterprise@themisdb.io
- Documentation: https://docs.themisdb.io/enterprise
- License activation: https://license.themisdb.io

---

**Document Version:** 1.0  
**Last Updated:** December 13, 2025
