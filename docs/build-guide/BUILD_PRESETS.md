# ThemisDB CMake Presets Guide

This guide covers using CMake Presets for building ThemisDB with pre-configured settings for different editions and scenarios. **All presets are optimized with shared vcpkg package installation and multi-tier binary caching for maximum build speed.**

## Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Build Performance & Caching](#build-performance--caching)
4. [Available Presets](#available-presets)
4. [Using Presets](#using-presets)
5. [Edition Presets](#edition-presets)
6. [Cross-Compilation Presets](#cross-compilation-presets)
7. [Test Presets](#test-presets)
8. [Custom Presets](#custom-presets)
9. [IDE Integration](#ide-integration)

## Overview

CMake Presets provide a standardized way to configure, build, and test ThemisDB without remembering complex command-line options. All presets are defined in `CMakePresets.json` at the project root.

**Benefits:**
- ✅ Reproducible builds across different machines
- ✅ Simplified CI/CD configuration
- ✅ IDE integration (VS Code, CLion, Visual Studio)
- ✅ No need to remember long cmake commands

## Quick Start

### List All Available Presets

```bash
# List configure presets
cmake --list-presets

# List build presets
cmake --list-presets=build

# List test presets
cmake --list-presets=test
```

### Configure and Build

```bash
# Configure using a preset
cmake --preset community-release

# Build using the corresponding build preset
cmake --build --preset community-release
```

### One-Step Build

```bash
# Configure and build in one command
cmake --preset community-release && cmake --build --preset community-release
```

## Build Performance & Caching

### Shared vcpkg Installation

**All CMake Presets now use a shared `vcpkg_installed` directory** to maximize build performance and minimize disk usage:

```
ThemisDB/
├── vcpkg_installed/          # ← Shared by ALL presets
│   ├── x64-linux/           # Linux packages
│   ├── x64-windows/         # Windows packages
│   └── arm64-linux/         # ARM64 packages
├── build-community-debug/   # Uses vcpkg_installed
├── build-community-release/ # Uses vcpkg_installed
└── build-minimal-debug/     # Uses vcpkg_installed
```

**Benefits:**
- ✅ **Packages installed once, used by all build configurations**
- ✅ **Instant switching between presets** (no package reinstallation)
- ✅ **10-20 GB disk space savings** (single installation vs. per-build copies)
- ✅ **Faster incremental builds** across different configurations

### Multi-Tier Binary Cache

ThemisDB now uses a **3-tier binary cache strategy** for optimal performance:

#### Tier 1: Project-Local Cache (Fastest)
```
.vcpkg-cache/         # Project-specific prebuilt binaries
```
- **Location:** `${sourceDir}/.vcpkg-cache`
- **Purpose:** Share prebuilt packages within the project
- **Speed:** Instant (local SSD)
- **Best for:** Team development, local rebuilds

#### Tier 2: User-Level Cache
```
~/.cache/vcpkg/archives/     # Linux/macOS
%LOCALAPPDATA%/vcpkg/archives/  # Windows
```
- **Location:** User home directory
- **Purpose:** Share packages across all projects
- **Speed:** Very fast (local disk)
- **Best for:** Individual developers with multiple projects

#### Tier 3: Build Directory Fallback
```
build-*/vcpkg_cache/  # Per-build fallback
```
- **Location:** Within build directory
- **Purpose:** Fallback when tiers 1-2 unavailable
- **Speed:** Fast (local build directory)

### Performance Impact

**Before Optimization:**
```
First build:           ~30 minutes (full compilation)
Switching presets:     ~30 minutes (reinstall packages)
Disk usage (5 builds): ~25 GB (separate vcpkg_installed each)
```

**After Optimization:**
```
First build:           ~2-5 minutes (with binary cache)
Switching presets:     ~10 seconds (packages already installed)
Disk usage (5 builds): ~5 GB (shared vcpkg_installed)
Savings:              ~20 GB + 29 minutes per preset switch
```

### Managing Cache

```bash
# View cache usage
du -sh .vcpkg-cache/           # Project cache
du -sh ~/.cache/vcpkg/archives/ # User cache

# Clear project cache (to force rebuild)
rm -rf .vcpkg-cache/

# Clear shared installation (to reinstall packages)
rm -rf vcpkg_installed/
```

**Note:** Both `.vcpkg-cache/` and `vcpkg_installed/` are excluded from version control via `.gitignore`.

## Available Presets

### Edition-Based Configure Presets

| Preset Name | Edition | Build Type | Features | Use Case |
|-------------|---------|------------|----------|----------|
| `minimal-debug` | MINIMAL | Debug | Core only | Embedded development |
| `minimal-release` | MINIMAL | MinSizeRel | Core only | Embedded deployment |
| `community-debug` | COMMUNITY | Debug | LLM, gRPC | Standard development |
| `community-release` | COMMUNITY | Release | LLM, gRPC | Standard deployment |
| `community-llm` | COMMUNITY | Release | LLM, GPU | AI workloads |
| `community-gpu` | COMMUNITY | Release | LLM, GPU, CUDA | GPU acceleration |
| `enterprise-debug` | ENTERPRISE | Debug | All advanced | Enterprise development |
| `enterprise-release` | ENTERPRISE | Release | All advanced | Enterprise deployment |
| `hyperscaler-release` | HYPERSCALER | Release | All features | Cloud hyperscalers |

### Cross-Compilation Presets

| Preset Name | Target Platform | Edition | Use Case |
|-------------|-----------------|---------|----------|
| `cross-arm64` | ARM64 Linux | COMMUNITY | ARM servers, RPi 4+ |
| `cross-armv7` | ARMv7 Linux | MINIMAL | Raspberry Pi 2/3 |

### Windows Presets

| Preset Name | Compiler | Build Type | Use Case |
|-------------|----------|------------|----------|
| `windows-ninja-msvc-debug` | MSVC | Debug | Windows development |
| `windows-ninja-msvc-release` | MSVC | Release | Windows deployment |
| `windows-ninja-clangcl-debug` | Clang-CL | Debug | Clang on Windows |
| `windows-ninja-clangcl-release` | Clang-CL | Release | Clang on Windows |

## Using Presets

### Standard Workflow

1. **Configure** - Set up build directory and cache variables
2. **Build** - Compile the project
3. **Test** - Run tests (optional)

```bash
# Step 1: Configure
cmake --preset community-release

# Step 2: Build
cmake --build --preset community-release

# Step 3: Test (optional)
ctest --preset test-fast
```

### Parallel Builds

All build presets use parallel compilation automatically (`jobs: 0` = all cores):

```bash
# Builds using all available CPU cores
cmake --build --preset community-release
```

To limit cores manually:

```bash
# Use only 4 cores
cmake --build --preset community-release -j 4
```

### Clean Build

```bash
# Remove build directory
rm -rf build-community-release

# Reconfigure
cmake --preset community-release
```

## Edition Presets

### MINIMAL Edition

For embedded systems and IoT devices with limited resources.

**Debug Build:**
```bash
cmake --preset minimal-debug
cmake --build --preset minimal-debug
```

**Release Build (Size-Optimized):**
```bash
cmake --preset minimal-release
cmake --build --preset minimal-release
```

**Features:**
- Core database only
- No LLM, GPU, or advanced protocols
- Optimized for minimal binary size
- Hardware limits: No GPU, 1 node, 128 MB cache

### COMMUNITY Edition (Default)

Standard edition for most users.

**Debug Build:**
```bash
cmake --preset community-debug
cmake --build --preset community-debug
```

**Release Build:**
```bash
cmake --preset community-release
cmake --build --preset community-release
```

**Features:**
- LLM support enabled
- gRPC networking
- Optional GPU support (off by default)
- Hardware limits: 16 GB GPU VRAM (1x Tesla T4), 5 nodes, 1 GB cache

### COMMUNITY with LLM/GPU

Enable full AI capabilities:

```bash
cmake --preset community-llm
cmake --build --preset community-llm
```

**With CUDA:**
```bash
cmake --preset community-gpu
cmake --build --preset community-gpu
```

### ENTERPRISE Edition

Advanced features for enterprise deployments.

**Debug Build:**
```bash
cmake --preset enterprise-debug
cmake --build --preset enterprise-debug
```

**Release Build:**
```bash
# Requires license file for Release builds
cmake --preset enterprise-release -DTHEMIS_LICENSE_FILE=/path/to/license.json
cmake --build --preset enterprise-release
```

**Features:**
- All COMMUNITY features
- HSM integration
- Multi-shard support (up to 100 nodes)
- Hardware limits: 320 GB GPU VRAM (4x A100 80 GB), 100 nodes, 4 GB cache

### HYPERSCALER Edition

Maximum performance for cloud environments.

```bash
cmake --preset hyperscaler-release
cmake --build --preset hyperscaler-release
```

**Features:**
- All features enabled
- Unlimited hardware resources
- Distributed training
- OpenTelemetry tracing
- All GPU backends (CUDA, Vulkan, HIP)

## Cross-Compilation Presets

### ARM64 Linux

Build for ARM64 Linux systems (e.g., AWS Graviton, Raspberry Pi 4+):

```bash
# Configure for ARM64
cmake --preset cross-arm64

# Build
cmake --build --preset cross-arm64

# Output: build-cross-arm64/bin/themisdb
```

**Prerequisites:**
```bash
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

### ARMv7 Linux

Build for ARMv7 Linux (e.g., Raspberry Pi 2/3):

```bash
# Configure for ARMv7
cmake --preset cross-armv7

# Build
cmake --build --preset cross-armv7

# Output: build-cross-armv7/bin/themisdb
```

**Prerequisites:**
```bash
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
```

See [BUILD_CROSS_COMPILE.md](BUILD_CROSS_COMPILE.md) for detailed cross-compilation guide.

## Test Presets

### Fast Unit Tests

Run quick unit tests only (excludes integration tests):

```bash
ctest --preset test-fast
```

**Configuration:**
- Uses `community-debug` configuration
- Excludes tests with `integration` label
- Shows output only on failure

### Full Test Suite

Run complete test suite including integration tests:

```bash
ctest --preset test-full
```

**Configuration:**
- Uses `community-debug` configuration
- Runs all tests
- Shows output only on failure

### Windows Test Presets

```bash
# MSVC Debug
ctest --preset windows-ninja-msvc-debug

# MSVC Release
ctest --preset windows-ninja-msvc-release
```

## Custom Presets

### Creating User Presets

Create `CMakeUserPresets.json` in the project root to add custom presets without modifying `CMakePresets.json`:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "my-custom-build",
      "inherits": "community-release",
      "displayName": "My Custom Build",
      "description": "Custom configuration for my workflow",
      "cacheVariables": {
        "THEMIS_ENABLE_LLM": "ON",
        "THEMIS_ENABLE_GPU": "ON",
        "THEMIS_BUILD_TESTS": "ON"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "my-custom-build",
      "configurePreset": "my-custom-build",
      "jobs": 4
    }
  ]
}
```

**Use your custom preset:**
```bash
cmake --preset my-custom-build
cmake --build --preset my-custom-build
```

### Preset Inheritance

Presets can inherit from other presets to share common configuration:

```json
{
  "configurePresets": [
    {
      "name": "my-debug",
      "inherits": "community-debug",
      "cacheVariables": {
        "THEMIS_ENABLE_ASAN": "ON"
      }
    }
  ]
}
```

### Override Environment Variables

```json
{
  "configurePresets": [
    {
      "name": "my-preset",
      "inherits": "base",
      "environment": {
        "VCPKG_ROOT": "/custom/vcpkg/path"
      }
    }
  ]
}
```

## IDE Integration

### Visual Studio Code

1. Install the CMake Tools extension
2. Open Command Palette (Ctrl+Shift+P)
3. Select **CMake: Select Configure Preset**
4. Choose a preset (e.g., `community-release`)
5. Build with **CMake: Build** (F7)

**VS Code settings.json:**
```json
{
  "cmake.useCMakePresets": "always",
  "cmake.configureOnOpen": false
}
```

### CLion

CLion automatically detects and uses CMake Presets:

1. Open **Settings** → **Build, Execution, Deployment** → **CMake**
2. CLion will auto-detect presets from `CMakePresets.json`
3. Select a preset from the dropdown
4. Build normally (Ctrl+F9)

**Tips:**
- Enable **Reload CMake project on build files changes**
- Use **Build** → **Reparse CMakeLists.txt** if presets don't appear

### Visual Studio 2022

Visual Studio has native CMake Presets support:

1. Open project folder in Visual Studio
2. CMake Presets will appear in the configuration dropdown
3. Select a preset (e.g., `windows-ninja-msvc-release`)
4. Build with **Build** → **Build All** (Ctrl+Shift+B)

**CMakeSettings.json is no longer needed** - Visual Studio reads `CMakePresets.json` directly.

### Qt Creator

Qt Creator supports CMake Presets (version 7+):

1. Open project
2. Go to **Projects** → **Build Settings**
3. Select **CMake preset** as configuration
4. Choose preset from dropdown
5. Build normally

## Advanced Usage

### Viewing Preset Configuration

```bash
# Show all variables set by a preset
cmake --preset community-release -N -LA
```

### Combining with Command-Line Options

You can override preset values:

```bash
cmake --preset community-release -DTHEMIS_ENABLE_GPU=OFF
```

### Multi-Configuration Generators

Some presets support multi-configuration generators:

```bash
cmake --preset windows-ninja-msvc-release
cmake --build --preset windows-ninja-msvc-release --config Release
```

### Preset Conditions

Presets can be conditional based on platform:

```json
{
  "configurePresets": [
    {
      "name": "linux-only",
      "condition": {
        "type": "equals",
        "lhs": "${hostSystemName}",
        "rhs": "Linux"
      }
    }
  ]
}
```

## Common Workflows

### Development Workflow

```bash
# Configure for development
cmake --preset community-debug

# Incremental build
cmake --build --preset community-debug

# Run fast tests
ctest --preset test-fast

# Debug with GDB
gdb build-community-debug/bin/themisdb
```

### CI/CD Workflow

```bash
# Clean build for CI
cmake --preset community-release
cmake --build --preset community-release
ctest --preset test-full
```

### Release Build Workflow

```bash
# Configure for release
cmake --preset community-release

# Build with all cores
cmake --build --preset community-release

# Run benchmarks
build-community-release/bin/themis_bench
```

## Troubleshooting

### Preset Not Found

**Error:**
```
CMake Error: Could not read presets from CMakePresets.json
```

**Solution:**
- Ensure CMake version ≥ 3.20
- Check JSON syntax: `python3 -m json.tool CMakePresets.json`

### VCPKG_ROOT Not Set

**Error:**
```
CMake Error: $env{VCPKG_ROOT} is not set
```

**Solution:**
```bash
export VCPKG_ROOT=/path/to/vcpkg
```

Or create `CMakeUserPresets.json`:
```json
{
  "configurePresets": [
    {
      "name": "my-preset",
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": "/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
      }
    }
  ]
}
```

### Preset Uses Wrong Generator

**Error:**
```
CMake Error: Could not find Ninja
```

**Solution:**
```bash
# Install Ninja
sudo apt-get install ninja-build  # Ubuntu/Debian
brew install ninja                 # macOS
choco install ninja                # Windows
```

Or override generator:
```bash
cmake --preset community-release -G "Unix Makefiles"
```

## References

- [CMake Presets Documentation](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- [ThemisDB Cross-Compilation Guide](BUILD_CROSS_COMPILE.md)
- [Edition Comparison](EDITION_COMPARISON.md)
- [CMake Build Instructions](../cmake/.copilot-cmake-build-instructions.md)
