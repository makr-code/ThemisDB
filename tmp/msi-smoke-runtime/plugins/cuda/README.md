# CUDA Acceleration Plugin Template

## Status: 📋 Template / Example Only

⚠️ **Note**: The CUDA acceleration backend IS fully implemented in `src/acceleration/cuda_backend.cpp` and can be enabled with build flags. This directory contains templates for creating **external plugin DLLs**.

The files in this directory serve as examples for developers who want to create external hardware acceleration plugins that can be loaded dynamically.

## Actual CUDA Implementation

The production CUDA backend is located at:
- **Source**: `src/acceleration/cuda_backend.cpp`
- **Kernels**: `src/acceleration/cuda/vector_kernels.cu`
- **Header**: `include/acceleration/cuda_backend.h`

To use CUDA acceleration, enable it during build:
```bash
cmake -DTHEMIS_ENABLE_CUDA=ON ..
```

## Template Files

- `CMakeLists.txt.example` - Example CMake build configuration for external plugin
- `cuda_plugin.cpp.example` - Example external plugin implementation skeleton
- `cuda_plugin.json` - Example plugin manifest
- `README.md` - This file

## Purpose

These templates demonstrate:
- How an external hardware acceleration plugin DLL/SO should be structured
- Required plugin interface implementations
- Build system configuration for external plugins
- Plugin manifest format

## Features (When Using Built-in CUDA Backend)

The implemented CUDA backend provides:

- GPU-accelerated vector similarity search
- Parallel geospatial computations
- Graph algorithm acceleration
- Matrix operations for embeddings
- Faiss GPU integration
- Custom CUDA kernels

## Requirements

- NVIDIA GPU with CUDA support
- CUDA Toolkit 11.0 or higher
- Compatible NVIDIA drivers

## Building ThemisDB with CUDA Support

```bash
# Configure
cmake -DTHEMIS_ENABLE_CUDA=ON ..

# Build
cmake --build .

# Verify CUDA backend is available
./themis_server --acceleration-info
```

## For Developers

If you're interested in implementing CUDA acceleration:

1. Review the template files in this directory
2. See [../PLANNED_ACCELERATION_PLUGINS.md](../PLANNED_ACCELERATION_PLUGINS.md) for complete details
3. Review the plugin interface in `include/plugins/plugin_interface.h`
4. Implement the required backend classes
5. Submit a pull request

## Documentation

For more information about planned hardware acceleration, see:
- [Planned Acceleration Plugins](../PLANNED_ACCELERATION_PLUGINS.md)
- [Plugin Development](../README.md#plugin-development-guide)

For information about actual working plugins, see:
- [Main Plugin README](../README.md)
- [Image Analysis Plugins](../image_analysis/README.md) (uses ONNX, can use CUDA backend)

