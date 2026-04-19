# ThemisDB Cross-Compilation Guide

This guide covers cross-compiling ThemisDB for different target platforms using CMake toolchain files.

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [ARM64 Linux Cross-Compilation](#arm64-linux-cross-compilation)
4. [ARMv7 Linux Cross-Compilation](#armv7-linux-cross-compilation)
5. [Windows MinGW Cross-Compilation](#windows-mingw-cross-compilation)
6. [vcpkg Configuration](#vcpkg-configuration)
7. [Using CMake Presets](#using-cmake-presets)
8. [Troubleshooting](#troubleshooting)

## Overview

ThemisDB supports cross-compilation to multiple target platforms using CMake toolchain files. Cross-compilation allows you to build executables for a different architecture or operating system than your build host.

**Supported Cross-Compilation Targets:**

| Target | Toolchain File | vcpkg Triplet | Use Case |
|--------|---------------|---------------|----------|
| ARM64 Linux | `arm64-linux-gnu.cmake` | `arm64-linux` | ARM servers, Raspberry Pi 4+ |
| ARMv7 Linux | `armv7-linux-gnueabihf.cmake` | `arm-linux` | Raspberry Pi 2/3 |
| Windows x64 | `x86_64-w64-mingw32.cmake` | `x64-mingw-static` | Windows from Linux |

## Prerequisites

### Ubuntu/Debian Requirements

Install cross-compilation toolchains:

```bash
# ARM64 cross-compiler
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# ARMv7 cross-compiler
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# MinGW-w64 cross-compiler (for Windows)
sudo apt-get install mingw-w64
```

### vcpkg Setup

Ensure vcpkg is properly configured:

```bash
# Clone vcpkg as a submodule (if not already done)
git submodule update --init --recursive

# Bootstrap vcpkg
cd vcpkg
./bootstrap-vcpkg.sh

# Set VCPKG_ROOT environment variable
export VCPKG_ROOT=$(pwd)
```

## ARM64 Linux Cross-Compilation

### Using CMake Presets (Recommended)

The simplest way to cross-compile for ARM64:

```bash
cmake --preset cross-arm64
cmake --build --preset cross-arm64
```

### Manual Configuration

If you need custom configuration:

```bash
cmake -S . -B build-arm64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/platforms/Toolchains/arm64-linux-gnu.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=COMMUNITY \
  -DVCPKG_TARGET_TRIPLET=arm64-linux
  
cmake --build build-arm64 -j$(nproc)
```

### Toolchain File Details

**File:** `cmake/platforms/Toolchains/arm64-linux-gnu.cmake`

```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(VCPKG_TARGET_TRIPLET arm64-linux CACHE STRING "vcpkg triplet")
```

## ARMv7 Linux Cross-Compilation

### Using CMake Presets (Recommended)

```bash
cmake --preset cross-armv7
cmake --build --preset cross-armv7
```

### Manual Configuration

```bash
cmake -S . -B build-armv7 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/platforms/Toolchains/armv7-linux-gnueabihf.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=MINIMAL \
  -DVCPKG_TARGET_TRIPLET=arm-linux
  
cmake --build build-armv7 -j$(nproc)
```

### Raspberry Pi Optimization

For Raspberry Pi 2/3 (ARMv7):

```bash
cmake -S . -B build-rpi \
  -DCMAKE_TOOLCHAIN_FILE=cmake/platforms/Toolchains/armv7-linux-gnueabihf.cmake \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DTHEMIS_EDITION=MINIMAL \
  -DTHEMIS_ENABLE_ARM_NEON=ON \
  -DVCPKG_TARGET_TRIPLET=arm-linux
```

## Windows MinGW Cross-Compilation

### Using Toolchain File

```bash
cmake -S . -B build-mingw \
  -DCMAKE_TOOLCHAIN_FILE=cmake/platforms/Toolchains/x86_64-w64-mingw32.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=COMMUNITY \
  -DVCPKG_TARGET_TRIPLET=x64-mingw-static
  
cmake --build build-mingw -j$(nproc)
```

### Notes for MinGW

- Uses static linking by default (`-static-libgcc -static-libstdc++`)
- Requires `x64-mingw-static` vcpkg triplet
- Windows-specific definitions are automatically set

## vcpkg Configuration

### Understanding vcpkg Triplets

vcpkg triplets define the target platform and library linkage:

| Triplet | Platform | Arch | Linkage |
|---------|----------|------|---------|
| `x64-linux` | Linux | x86_64 | Dynamic |
| `arm64-linux` | Linux | ARM64 | Dynamic |
| `arm-linux` | Linux | ARMv7 | Dynamic |
| `x64-mingw-static` | Windows | x86_64 | Static |

### Installing Dependencies for Cross-Compilation

vcpkg will automatically build dependencies for the target platform:

```bash
# For ARM64 Linux
export VCPKG_DEFAULT_TRIPLET=arm64-linux
./vcpkg/vcpkg install

# For ARMv7 Linux
export VCPKG_DEFAULT_TRIPLET=arm-linux
./vcpkg/vcpkg install
```

### Binary Cache

Enable vcpkg binary cache to speed up cross-compilation:

```bash
# Set cache directory
export VCPKG_BINARY_SOURCES="clear;files,$HOME/.cache/vcpkg/archives,readwrite"
```

## Using CMake Presets

ThemisDB includes pre-configured presets for common cross-compilation scenarios.

### List Available Presets

```bash
cmake --list-presets
```

### Available Cross-Compilation Presets

- **`cross-arm64`** - ARM64 Linux (COMMUNITY edition)
- **`cross-armv7`** - ARMv7 Linux (MINIMAL edition)

### Using a Preset

```bash
# Configure
cmake --preset cross-arm64

# Build
cmake --build --preset cross-arm64

# Results will be in build-cross-arm64/
```

### Customizing Presets

Create `CMakeUserPresets.json` to override preset values:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "my-arm64",
      "inherits": "cross-arm64",
      "cacheVariables": {
        "THEMIS_ENABLE_LLM": "ON"
      }
    }
  ]
}
```

## Troubleshooting

### Cannot Find Cross-Compiler

**Error:**
```
CMake Error: CMAKE_C_COMPILER not found: aarch64-linux-gnu-gcc
```

**Solution:**
```bash
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

### vcpkg Dependency Build Failures

**Error:**
```
error: building <package>:arm64-linux failed
```

**Solution:**
1. Ensure cross-compiler is in PATH
2. Set `VCPKG_FORCE_SYSTEM_BINARIES=1` on ARM build hosts
3. Check vcpkg logs: `vcpkg/buildtrees/<package>/config-*.log`

### Cross-Compiled Binary Won't Run on Target

**Problem:** Binary runs on build host but crashes on target

**Causes:**
1. **Wrong architecture flags** - Check `-march=` in toolchain file
2. **Missing libraries** - Use static linking or deploy dependencies
3. **GLIBC version mismatch** - Build on system with older GLIBC

**Solution:**
```bash
# Check binary architecture
file build-arm64/bin/themisdb

# Check library dependencies
aarch64-linux-gnu-readelf -d build-arm64/bin/themisdb
```

### vcpkg Triplet Not Found

**Error:**
```
error: could not find triplet file: arm64-linux.cmake
```

**Solution:**
```bash
# Verify vcpkg triplet exists
ls vcpkg/triplets/arm64-linux.cmake

# Update vcpkg
cd vcpkg
git pull
./bootstrap-vcpkg.sh
```

### Permission Denied When Running Cross-Compiled Binary

If testing the binary on the build host:

```bash
# Use QEMU to run ARM binaries on x86_64
sudo apt-get install qemu-user-static

# Run ARM64 binary (adjust path to actual binary location)
qemu-aarch64-static -L /usr/aarch64-linux-gnu ./build-arm64/bin/themis_server

# Run ARMv7 binary (adjust path to actual binary location)
qemu-arm-static -L /usr/arm-linux-gnueabihf ./build-armv7/bin/themis_server
```

**Note:** The exact binary name may vary (e.g., `themisdb`, `themis_server`). Check `build-*/bin/` for available executables.

## Advanced Topics

### Custom Sysroot

If you need a custom sysroot (e.g., from a target device):

```bash
cmake -S . -B build-arm64 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/platforms/Toolchains/arm64-linux-gnu.cmake \
  -DCMAKE_SYSROOT=/path/to/arm64-sysroot \
  -DCMAKE_FIND_ROOT_PATH=/path/to/arm64-sysroot
```

### Cross-Compiling vcpkg Dependencies

Create a custom vcpkg triplet:

```cmake
# vcpkg/triplets/community/my-arm64.cmake
set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE 
    ${CMAKE_CURRENT_LIST_DIR}/../../cmake/platforms/Toolchains/arm64-linux-gnu.cmake)
```

Use it:

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=cmake/platforms/Toolchains/arm64-linux-gnu.cmake \
  -DVCPKG_TARGET_TRIPLET=my-arm64
```

## References

- [CMake Cross Compiling](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling)
- [vcpkg Triplets](https://learn.microsoft.com/en-us/vcpkg/users/triplets)
- [ThemisDB Build Presets](BUILD_PRESETS.md)
- [Platform Detection](../cmake/platforms/PlatformDetection.cmake)
