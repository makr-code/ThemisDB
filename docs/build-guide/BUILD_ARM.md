# ARM Build Guide (Generisch)

## Überblick

ThemisDB unterstützt mehrere ARM-Plattformen:

| Plattform | Architektur | Prozessor | Guide |
|-----------|-------------|-----------|-------|
| **Raspberry Pi 4+** | ARMv8 (64-bit) | Cortex-A72 | [BUILD_RASPBERRY_PI.md](BUILD_RASPBERRY_PI.md) |
| **QNAP NAS** | ARMv7/ARMv8 | Varies | [BUILD_QNAP.md](BUILD_QNAP.md) |
| **Generic ARM64** | ARMv8 (64-bit) | Any | Siehe unten |
| **Generic ARMv7** | ARMv7 (32-bit) | Any | Siehe unten |

## Voraussetzungen für ARM Cross-Compilation

### Host System (z.B. Linux/Windows mit WSL)

```bash
# Arm64 (64-bit) Toolchain
sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# ARMv7 (32-bit) Toolchain
sudo apt-get install -y gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# CMake und Dependencies
sudo apt-get install -y cmake ninja-build pkg-config
```

### CMake Toolchain File (Cross-Compilation)

Erstelle `cmake/toolchain-arm64.cmake`:

```cmake
# CMake Toolchain für ARM64 (aarch64)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

Oder für ARMv7:

```cmake
# CMake Toolchain für ARMv7 (32-bit)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

## Cross-Compilation: ARM64

### Build auf x86_64 für ARM64

```bash
cd /path/to/themis

# Configure mit ARM64 Toolchain
cmake -S . -B build-arm64 \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm64.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_ENABLE_GPU=OFF

# Build
cmake --build build-arm64 --parallel 8

# Binary: build-arm64/themis_server
```

### Nativer Build auf ARM64 System

```bash
# Direkter Build auf ARM64 Hardware
cmake --preset linux-gcc-release
cmake --build build-wsl --parallel 4  # Weniger Cores auf RPi
```

## Cross-Compilation: ARMv7 (32-bit)

```bash
cd /path/to/themis

# Configure mit ARMv7 Toolchain
cmake -S . -B build-armv7 \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-armv7.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=OFF

# Build
cmake --build build-armv7 --parallel 4
```

## vcpkg für ARM

vcpkg unterstützt auch ARM-Triplets:

```bash
# ARM64 Linux (aarch64)
export VCPKG_TARGET_TRIPLET=arm64-linux

# ARMv7 Linux (32-bit)
export VCPKG_TARGET_TRIPLET=arm-linux

# Dann wie gewohnt:
cmake --preset linux-gcc-release
```

## Memory-optimierte Builds für Embedded

ARM-Systeme haben oft weniger RAM (1-4GB). Optimierungen:

```cmake
# Disable LTO (Link-Time Optimization) für weniger Memory
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

# Disable Benchmarks
set(THEMIS_BUILD_BENCHMARKS OFF)

# Minimal Tests
set(THEMIS_BUILD_TESTS OFF)

# Strip Symbols nach Build
add_custom_command(TARGET themis_server POST_BUILD
    COMMAND arm-linux-gnueabihf-strip themis_server
)
```

## Docker für ARM

### Multi-Platform Build (Docker Buildx)

```bash
# Enable buildx
docker buildx create --name mybuilder --use

# Build für ARM64 + x86_64
docker buildx build -f docker/Dockerfile.themis-server \
  --platform linux/arm64,linux/amd64 \
  -t themis-server:multi-arch \
  .

# Nur ARM64
docker buildx build -f docker/Dockerfile.themis-server \
  --platform linux/arm64 \
  -t themis-server:arm64 \
  .
```

### Oder: Native ARM Build auf ARM Host

```bash
# Auf Raspberry Pi oder ARM NAS direkt:
docker build -f docker/Dockerfile.themis-server \
  -t themis-server:rpi4 \
  --build-arg THEMIS_ENABLE_LLM=OFF \
  .
```

## Performance auf ARM

### Tipps für bessere Performance

1. **Weniger Parallel Jobs**
   ```bash
   cmake --build . --parallel 2  # Statt 8
   ```

2. **Kleinere Binaries**
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_STRIP=strip
   ```

3. **NEON SIMD aktivieren** (wenn verfügbar)
   ```bash
   cmake -DENABLE_NEON=ON
   ```

4. **Swap für Build aktivieren**
   ```bash
   # Falls OOM während Build
   sudo fallocate -l 4G /swapfile
   sudo chmod 600 /swapfile
   sudo mkswap /swapfile
   sudo swapon /swapfile
   ```

## Spezielle Plattformen

- **Raspberry Pi 4**: [BUILD_RASPBERRY_PI.md](BUILD_RASPBERRY_PI.md)
- **QNAP NAS**: [BUILD_QNAP.md](BUILD_QNAP.md)

## Troubleshooting

### Problem: "No ARM toolchain found"
```bash
# Installiere ARM Toolchain
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

### Problem: "Invalid ELF class"
**Cause**: Mismatch zwischen Host und Target Architecture
```bash
# Check
file build-arm64/themis_server
# Output: ELF 64-bit LSB shared object, ARM aarch64
```

### Problem: "Out of Memory" beim Build
```bash
# Reduziere Parallel Jobs
cmake --build . --parallel 1
```

### Problem: "vcpkg packages not found for arm64"
```bash
# vcpkg needs ARM configuration
export VCPKG_TARGET_TRIPLET=arm64-linux
cmake --preset linux-gcc-release
```

## Nächste Schritte

Nach erfolgreichem ARM-Build lesen Sie:
- **ARM Deployment**: [docs/de/deployment/deployment_arm_build.md](../../de/deployment/deployment_arm_build.md)
- **Releases**: [docs/de/releases/updates_distribution_strategy.md](../../de/releases/updates_distribution_strategy.md)

## Weitere Infos

- [BUILD_RASPBERRY_PI.md](BUILD_RASPBERRY_PI.md) - RPi-spezifisch
- [BUILD_QNAP.md](BUILD_QNAP.md) - QNAP-spezifisch
- [cmake/CMakeLists.txt](../../cmake/CMakeLists.txt) - CMake-Konfiguration
