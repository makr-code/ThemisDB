# Cross-Compilation Complete Guide

**Stand:** 23. April 2026  
**Version:** v1.4.0  
**Kategorie:** 🔨 Build/Cross-Compile  
**Status:** Production-Ready

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Platform Support Matrix](#-platform-support-matrix)
- [Scenario 1: x86_64 → ARM64](#-scenario-1-x86_64--arm64-raspberry-pi-45-graviton)
- [Scenario 2: x86_64 → ARMv7](#-scenario-2-x86_64--armv7-raspberry-pi-3-32-bit)
- [Scenario 3: Windows → Linux](#-scenario-3-windows--linux-cross-compile)
- [Scenario 4: Docker Multi-Arch](#-scenario-4-docker-multi-arch-build)
- [Toolchain Files Reference](#-toolchain-files-reference)
- [Performance Optimization](#-performance-optimization)
- [Troubleshooting](#-troubleshooting)

---

## 🎯 Übersicht

### Native vs Cross-Compilation

```
┌─────────────────────────────────────────────┐
│        Native Build                         │
├─────────────────────────────────────────────┤
│  Host: x86_64 Linux                         │
│  Target: x86_64 Linux (same)                │
│  ✅ Simple, fast                             │
│  ✅ Direct execution & testing               │
│  ❌ Requires target hardware                 │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│        Cross-Compilation                    │
├─────────────────────────────────────────────┤
│  Host: x86_64 Linux                         │
│  Target: ARM64 Linux (different)            │
│  ✅ Build without target hardware            │
│  ✅ Faster on powerful host                  │
│  ❌ Cannot execute on host                   │
│  ❌ Requires cross-toolchain                 │
└─────────────────────────────────────────────┘
```

### When to Cross-Compile?

**Use Cross-Compilation:**
- ✅ Target device zu langsam (Raspberry Pi)
- ✅ Kein Target-Device verfügbar
- ✅ CI/CD für Multi-Arch
- ✅ Batch-Builds für mehrere Architekturen
- ✅ Entwicklung auf x86_64, Deployment auf ARM

**Use Native Compilation:**
- ✅ Target-Device verfügbar und schnell
- ✅ Einfacheres Setup
- ✅ Direktes Testing möglich
- ✅ Debugging einfacher

---

## 📊 Platform Support Matrix

### Supported Cross-Compilation Paths

| Host OS | Host Arch | Target OS | Target Arch | Support | Method |
|---------|-----------|-----------|-------------|---------|--------|
| **Linux** | x86_64 | Linux | ARM64 | ✅ Full | gcc-aarch64-linux-gnu |
| **Linux** | x86_64 | Linux | ARMv7 | ✅ Full | gcc-arm-linux-gnueabihf |
| **Linux** | x86_64 | Linux | ARMv6 | ✅ Full | gcc-arm-linux-gnueabi |
| **Windows** | x86_64 | Linux | ARM64 | ✅ WSL/Docker | WSL + gcc-aarch64 |
| **Windows** | x86_64 | Windows | ARM64 | ✅ Full | MSVC ARM64 |
| **macOS** | x86_64 | macOS | ARM64 | ✅ Full | clang -arch arm64 |
| **macOS** | ARM64 | macOS | x86_64 | ✅ Full | clang -arch x86_64 |
| **Any** | Any | Any | Any | ✅ Full | Docker buildx |

### Target Platform Details

| Target | CPU | OS | Use Case |
|--------|-----|----|----|
| **Raspberry Pi 5** | ARM64 (Cortex-A76) | Debian 12+ | Home server, IoT |
| **Raspberry Pi 4** | ARM64 (Cortex-A72) | Debian 11+ | Edge computing |
| **Raspberry Pi 3** | ARMv7 (Cortex-A53) | Debian 10+ | Legacy devices |
| **AWS Graviton** | ARM64 (Neoverse-N1) | Ubuntu 20.04+ | Cloud deployment |
| **QNAP NAS** | x86_64 (Baseline) | Ubuntu 20.04 | NAS appliance |
| **Docker Multi-Arch** | x86_64, ARM64 | Ubuntu 24.04 | Container deployment |

---

## 🎯 Scenario 1: x86_64 → ARM64 (Raspberry Pi 4/5, Graviton)

### Prerequisites (Ubuntu/Debian Host)

```bash
# Install cross-compiler toolchain
sudo apt-get update
sudo apt-get install -y \
  gcc-aarch64-linux-gnu \
  g++-aarch64-linux-gnu \
  binutils-aarch64-linux-gnu

# Install QEMU for testing (optional)
sudo apt-get install -y qemu-user-static

# Verify toolchain
aarch64-linux-gnu-gcc --version
# gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
```

### Step 1: Create Toolchain File

**cmake/toolchains/arm64-linux-gnu.cmake:**

```cmake
# ARM64 Linux GNU Toolchain
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Specify the cross compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Where to find target environment
set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# vcpkg triplet
set(VCPKG_TARGET_TRIPLET arm64-linux)
```

### Step 2: Configure vcpkg for ARM64

**vcpkg/triplets/arm64-linux.cmake:**

```cmake
set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_CMAKE_SYSTEM_PROCESSOR aarch64)

# Use cross-compiler
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE 
    ${CMAKE_CURRENT_LIST_DIR}/../../../cmake/toolchains/arm64-linux-gnu.cmake)
```

### Step 3: Cross-Compile Build

```bash
# Set vcpkg root
export VCPKG_ROOT=/path/to/vcpkg

# Configure with toolchain
cmake -B build-arm64 -S . \
  -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=cmake/toolchains/arm64-linux-gnu.cmake \
  -DVCPKG_TARGET_TRIPLET=arm64-linux \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=COMMUNITY \
  -DTHEMIS_ENABLE_AVX2=OFF \
  -DTHEMIS_BUILD_TESTS=OFF

# Build
cmake --build build-arm64 --parallel 8

# Check binary architecture
file build-arm64/themis_server
# themis_server: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV)
```

### Step 4: Test with QEMU (Optional)

```bash
# Run ARM64 binary on x86_64 with QEMU
qemu-aarch64-static -L /usr/aarch64-linux-gnu build-arm64/themis_server --version
# ThemisDB v1.4.0 (ARM64/Linux)
```

### Step 5: Deploy to Target

```bash
# Copy to Raspberry Pi
scp build-arm64/themis_server pi@raspberrypi.local:/opt/themisdb/

# SSH to target and run
ssh pi@raspberrypi.local
cd /opt/themisdb
./themis_server --config config.json
```

### Performance Notes

**Build Time:**
- Host (x86_64 16-core): ~25 minutes
- Native (Raspberry Pi 4): ~4 hours
- **Speedup: ~10x faster** ⚡

**Optimization Flags:**

```cmake
# For Raspberry Pi 4/5 (Cortex-A72/A76)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a72 -mtune=cortex-a72")

# For AWS Graviton (Neoverse-N1)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=neoverse-n1 -mtune=neoverse-n1")
```

---

## 🎯 Scenario 2: x86_64 → ARMv7 (Raspberry Pi 3 32-bit)

### Prerequisites

```bash
# Install ARMv7 cross-compiler
sudo apt-get install -y \
  gcc-arm-linux-gnueabihf \
  g++-arm-linux-gnueabihf \
  binutils-arm-linux-gnueabihf

# Verify
arm-linux-gnueabihf-gcc --version
```

### Toolchain File

**cmake/toolchains/armv7-linux-gnueabihf.cmake:**

```cmake
# ARMv7 Linux GNU EABI HF Toolchain
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7l)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(VCPKG_TARGET_TRIPLET arm-linux)
```

### vcpkg Triplet

**vcpkg/triplets/arm-linux.cmake:**

```cmake
set(VCPKG_TARGET_ARCHITECTURE arm)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_CMAKE_SYSTEM_PROCESSOR armv7l)

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE 
    ${CMAKE_CURRENT_LIST_DIR}/../../../cmake/toolchains/armv7-linux-gnueabihf.cmake)
```

### Build

```bash
cmake -B build-armv7 -S . \
  -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=cmake/toolchains/armv7-linux-gnueabihf.cmake \
  -DVCPKG_TARGET_TRIPLET=arm-linux \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_EDITION=MINIMAL \
  -DTHEMIS_EMBEDDED=ON \
  -DTHEMIS_ENABLE_AVX2=OFF \
  -DTHEMIS_BUILD_TESTS=OFF

cmake --build build-armv7 --parallel 8

# Verify
file build-armv7/themis_server
# themis_server: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV)
```

### Optimization for ARMv7

```cmake
# Raspberry Pi 3 (Cortex-A53)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=cortex-a53 -mfpu=neon-vfpv4")
```

---

## 🎯 Scenario 3: Windows → Linux Cross-Compile

### Option 1: WSL (Recommended)

**Windows Host mit WSL:**

```powershell
# Windows: Install WSL
wsl --install -d Ubuntu-22.04

# Enter WSL
wsl

# Inside WSL: Install toolchain
sudo apt-get update
sudo apt-get install -y build-essential gcc-aarch64-linux-gnu

# Clone repo (if not accessible from /mnt/c)
cd ~
git clone https://github.com/your-org/themisdb.git
cd themisdb

# Build for ARM64
cmake -B build-arm64 -S . \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm64-linux-gnu.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-arm64 --parallel $(nproc)
```

**Access from Windows:**

```powershell
# Windows PowerShell
# Binaries accessible at
\\wsl$\Ubuntu-22.04\home\username\themisdb\build-arm64\
```

### Option 2: Docker on Windows

```powershell
# Windows: Use Docker Desktop
docker run --rm -it -v ${PWD}:/workspace ubuntu:22.04

# Inside container
apt-get update
apt-get install -y cmake gcc-aarch64-linux-gnu

cd /workspace
cmake -B build-arm64 -S . \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm64-linux-gnu.cmake

cmake --build build-arm64
```

### Option 3: MinGW (Windows → Windows)

**Cross-compile Windows x64 → Windows ARM64:**

```powershell
# Visual Studio 2022 with ARM64 tools
cmake -B build-arm64 -G "Visual Studio 17 2022" -A ARM64 -S . `
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-arm64 --config Release
```

---

## 🎯 Scenario 4: Docker Multi-Arch Build

### Using Docker Buildx

**Buildx Setup:**

```bash
# Enable buildx
docker buildx create --name multiarch --use
docker buildx inspect --bootstrap

# Verify platforms
docker buildx ls
# NAME/NODE    DRIVER/ENDPOINT  STATUS   PLATFORMS
# multiarch *  docker-container running  linux/amd64, linux/arm64, linux/arm/v7
```

### Multi-Arch Dockerfile

**Dockerfile:**

```dockerfile
# syntax=docker/dockerfile:1
FROM --platform=$BUILDPLATFORM ubuntu:24.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    cmake ninja-build gcc g++ git \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# Set target architecture
ARG TARGETPLATFORM
ARG BUILDPLATFORM

WORKDIR /build
COPY . .

# Configure based on target
RUN if [ "$TARGETPLATFORM" = "linux/amd64" ]; then \
      cmake -B build -S . -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DTHEMIS_EDITION=COMMUNITY; \
    elif [ "$TARGETPLATFORM" = "linux/arm64" ]; then \
      cmake -B build -S . -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DTHEMIS_EDITION=COMMUNITY \
        -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm64-linux-gnu.cmake; \
    elif [ "$TARGETPLATFORM" = "linux/arm/v7" ]; then \
      cmake -B build -S . -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DTHEMIS_EDITION=MINIMAL \
        -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/armv7-linux-gnueabihf.cmake; \
    fi

# Build
RUN cmake --build build --parallel $(nproc)

# Runtime stage
FROM ubuntu:24.04
COPY --from=builder /build/build/themis_server /usr/local/bin/
ENTRYPOINT ["/usr/local/bin/themis_server"]
```

### Build Multi-Arch Images

```bash
# Build for multiple platforms
docker buildx build \
  --platform linux/amd64,linux/arm64,linux/arm/v7 \
  -t themisdb:v1.4.0 \
  --push \
  .

# Verify
docker buildx imagetools inspect themisdb:v1.4.0
# Name:      docker.io/library/themisdb:v1.4.0
# MediaType: application/vnd.docker.distribution.manifest.list.v2+json
# Digest:    sha256:...
# 
# Manifests:
#   Name:      docker.io/library/themisdb:v1.4.0@sha256:...
#   MediaType: application/vnd.docker.distribution.manifest.v2+json
#   Platform:  linux/amd64
#   
#   Name:      docker.io/library/themisdb:v1.4.0@sha256:...
#   MediaType: application/vnd.docker.distribution.manifest.v2+json
#   Platform:  linux/arm64
#   
#   Name:      docker.io/library/themisdb:v1.4.0@sha256:...
#   MediaType: application/vnd.docker.distribution.manifest.v2+json
#   Platform:  linux/arm/v7
```

### CI/CD Integration

**.github/workflows/multiarch-build.yml:**

```yaml
name: Multi-Arch Build

on:
  push:
    branches: [main]
    tags: ['v*.*.*']

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Set up QEMU
        uses: docker/setup-qemu-action@v3
      
      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v3
      
      - name: Build and push
        uses: docker/build-push-action@v5
        with:
          platforms: linux/amd64,linux/arm64,linux/arm/v7
          push: true
          tags: |
            themisdb:latest
            themisdb:${{ github.ref_name }}
```

---

## 📁 Toolchain Files Reference

### Standard Toolchains

**Location:** `cmake/toolchains/`

```
cmake/toolchains/
├── arm64-linux-gnu.cmake          # ARM64 Linux (Raspberry Pi 4/5, Graviton)
├── armv7-linux-gnueabihf.cmake    # ARMv7 Linux (Raspberry Pi 3)
├── armv6-linux-gnueabi.cmake      # ARMv6 Linux (Raspberry Pi Zero)
├── windows-arm64.cmake            # Windows ARM64
├── macos-arm64.cmake              # macOS ARM64 (M1/M2)
└── qnap-x64-baseline.cmake        # QNAP x64 (Baseline, no AVX)
```

### Generic Toolchain Template

```cmake
# cmake/toolchains/generic-cross.cmake

# System name and processor
set(CMAKE_SYSTEM_NAME ${TARGET_OS})          # Linux, Windows, Darwin
set(CMAKE_SYSTEM_PROCESSOR ${TARGET_ARCH})   # aarch64, armv7l, x86_64

# Compilers
set(CMAKE_C_COMPILER ${TARGET_C_COMPILER})
set(CMAKE_CXX_COMPILER ${TARGET_CXX_COMPILER})

# Sysroot (optional)
if(DEFINED TARGET_SYSROOT)
    set(CMAKE_SYSROOT ${TARGET_SYSROOT})
    set(CMAKE_FIND_ROOT_PATH ${TARGET_SYSROOT})
endif()

# Search modes
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# vcpkg integration
if(DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_FILE})
endif()
```

**Usage:**

```bash
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/generic-cross.cmake \
  -DTARGET_OS=Linux \
  -DTARGET_ARCH=aarch64 \
  -DTARGET_C_COMPILER=aarch64-linux-gnu-gcc \
  -DTARGET_CXX_COMPILER=aarch64-linux-gnu-g++
```

---

## ⚡ Performance Optimization

### Build Time Optimization

**1. Use Ninja Generator:**

```bash
cmake -B build -S . -GNinja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm64-linux-gnu.cmake

ninja -C build -j$(nproc)
# ~30% faster than Make
```

**2. ccache (Compiler Cache):**

```bash
# Install ccache
sudo apt-get install -y ccache

# Enable in CMake
cmake -B build -S . \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm64-linux-gnu.cmake

# Subsequent builds ~5x faster
```

**3. vcpkg Binary Cache:**

```bash
# Set binary cache directory
export VCPKG_BINARY_SOURCES="clear;files,${HOME}/.vcpkg-cache,readwrite"

# First build: downloads & caches
cmake -B build1 -S . -DCMAKE_TOOLCHAIN_FILE=...
cmake --build build1

# Second build: reuses cache ~10x faster
cmake -B build2 -S . -DCMAKE_TOOLCHAIN_FILE=...
cmake --build build2
```

### Runtime Performance

**1. CPU-Specific Optimizations:**

```cmake
# Raspberry Pi 4/5 (Cortex-A72/A76)
set(CMAKE_CXX_FLAGS_RELEASE 
    "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mcpu=cortex-a72 -mtune=cortex-a72")

# AWS Graviton (Neoverse-N1)
set(CMAKE_CXX_FLAGS_RELEASE 
    "${CMAKE_CXX_FLAGS_RELEASE} -O3 -mcpu=neoverse-n1 -mtune=neoverse-n1")
```

**2. Link-Time Optimization (LTO):**

```cmake
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
# ~10-15% runtime improvement
```

**3. Profile-Guided Optimization (PGO):**

```bash
# Step 1: Build with profiling
cmake -B build -S . \
  -DCMAKE_CXX_FLAGS="-fprofile-generate" \
  -DCMAKE_EXE_LINKER_FLAGS="-fprofile-generate"

cmake --build build

# Step 2: Run workload to collect profile
./build/themis_server --benchmark

# Step 3: Rebuild with profile data
cmake -B build -S . \
  -DCMAKE_CXX_FLAGS="-fprofile-use" \
  -DCMAKE_EXE_LINKER_FLAGS="-fprofile-use"

cmake --build build
# ~20-30% runtime improvement
```

---

## 🔧 Troubleshooting

### Error: Cross-Compiler Not Found

**Problem:**
```
CMake Error: CMAKE_C_COMPILER not found: aarch64-linux-gnu-gcc
```

**Solution:**
```bash
# Verify installation
which aarch64-linux-gnu-gcc

# Install if missing
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

### Error: Cannot Find Libraries

**Problem:**
```
fatal error: boost/asio.hpp: No such file or directory
```

**Solution:**
```bash
# Ensure vcpkg triplet is correct
cmake -B build -S . \
  -DVCPKG_TARGET_TRIPLET=arm64-linux \
  -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake \
  -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=cmake/toolchains/arm64-linux-gnu.cmake

# Install dependencies for target
${VCPKG_ROOT}/vcpkg install --triplet arm64-linux
```

### Error: Undefined Reference

**Problem:**
```
undefined reference to `__sync_fetch_and_add_8'
```

**Solution:**
```cmake
# Add atomic library for 32-bit ARM
if(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7")
    target_link_libraries(themis_server PRIVATE atomic)
endif()
```

### Error: Illegal Instruction (on target)

**Problem:**
```
./themis_server
Illegal instruction (core dumped)
```

**Solution:**
```bash
# Rebuild without AVX2/AVX512 (x86 only)
cmake -B build -S . \
  -DTHEMIS_ENABLE_AVX2=OFF \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm64-linux-gnu.cmake

# Or use correct CPU target
# For Raspberry Pi 3:
-DCMAKE_CXX_FLAGS="-mcpu=cortex-a53"

# For Raspberry Pi 4:
-DCMAKE_CXX_FLAGS="-mcpu=cortex-a72"
```

### Error: QEMU Can't Execute

**Problem:**
```
qemu-aarch64-static: Could not open '/lib/ld-linux-aarch64.so.1'
```

**Solution:**
```bash
# Specify library path
qemu-aarch64-static -L /usr/aarch64-linux-gnu build-arm64/themis_server

# Or install binfmt support
sudo apt-get install -y qemu-user-static binfmt-support
sudo update-binfmts --enable
```

### Performance: Slow Cross-Compilation

**Problem:**
Cross-compilation taking >2 hours

**Solutions:**

1. **Use more cores:**
   ```bash
   cmake --build build --parallel $(nproc)
   ```

2. **Use Ninja:**
   ```bash
   cmake -B build -S . -GNinja
   ninja -C build -j$(nproc)
   ```

3. **Enable ccache:**
   ```bash
   cmake -B build -S . \
     -DCMAKE_C_COMPILER_LAUNCHER=ccache \
     -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
   ```

4. **Disable tests/benchmarks:**
   ```bash
   cmake -B build -S . \
     -DTHEMIS_BUILD_TESTS=OFF \
     -DTHEMIS_BUILD_BENCHMARKS=OFF
   ```

---

## 📊 Zusammenfassung

### Cross-Compilation Matrix

| Scenario | Host | Target | Method | Speedup |
|----------|------|--------|--------|---------|
| **Raspberry Pi** | x86_64 Linux | ARM64 Linux | gcc-aarch64 | 10x ⚡ |
| **Old Raspberry** | x86_64 Linux | ARMv7 Linux | gcc-arm-hf | 15x ⚡ |
| **Windows → Linux** | Windows | ARM64 Linux | WSL/Docker | 8x ⚡ |
| **Multi-Arch** | Any | Multiple | Docker buildx | N/A |

### Key Tools

**Cross-Compilers:**
- `gcc-aarch64-linux-gnu` - ARM64
- `gcc-arm-linux-gnueabihf` - ARMv7
- `gcc-arm-linux-gnueabi` - ARMv6

**Emulation:**
- `qemu-user-static` - Run cross-compiled binaries
- `binfmt-support` - Transparent execution

**Build Acceleration:**
- `ninja` - Fast build system
- `ccache` - Compiler cache
- `distcc` - Distributed compilation

### Best Practices

✅ **Always specify target CPU:**
```cmake
-DCMAKE_CXX_FLAGS="-mcpu=cortex-a72"
```

✅ **Use vcpkg binary cache:**
```bash
export VCPKG_BINARY_SOURCES="files,${HOME}/.vcpkg-cache,readwrite"
```

✅ **Test with QEMU before deploying:**
```bash
qemu-aarch64-static -L /usr/aarch64-linux-gnu ./themis_server --version
```

✅ **Use Docker buildx for CI/CD:**
```bash
docker buildx build --platform linux/amd64,linux/arm64,linux/arm/v7
```

---

## 🔗 Verwandte Dokumentation

### Build & Deployment
- [CMake Build System Overview](../deployment/CMAKE_BUILD_SYSTEM_OVERVIEW.md) - Architektur
- [Build Guide](guides_build.md) - Quick Start
- [Build Scripts Reference](BUILD_SCRIPTS_REFERENCE.md) - Automatisierung

### Platform-Specific
- [ARM Deployment](../deployment/deployment_arm_build.md) - Raspberry Pi Details
- [Docker Multi-Arch](../deployment/deployment_docker_multiarch.md) - Container Deployment
- [QNAP Deployment](../deployment/deployment_qnap.md) - NAS Deployment

### Editions
- [Edition Limits Matrix](../deployment/EDITION_LIMITS_MATRIX.md) - Edition Vergleich
- [License Requirements](../deployment/LICENSE_REQUIREMENTS.md) - Lizenz-Info

---

**Letzte Aktualisierung:** 23. April 2026  
**Version:** v1.4.0  
**Kategorie:** Cross-Compilation Complete Guide
