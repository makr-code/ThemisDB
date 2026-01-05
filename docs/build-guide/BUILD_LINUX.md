# Linux / WSL Build Guide

## Voraussetzungen

### Ubuntu 20.04+ / Debian 11+

```bash
# System aktualisieren
sudo apt-get update && sudo apt-get upgrade -y

# Build Tools installieren
sudo apt-get install -y \
    build-essential \
    gcc-11 g++-11 \
    cmake ninja-build \
    git curl wget \
    pkg-config \
    libssl-dev libcurl4-openssl-dev

# Python (für Doku)
sudo apt-get install -y python3 python3-dev python3-pip
```

### Compiler Settings (Optional)
```bash
# Falls gcc-11 nicht default ist:
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100
```

### CMake >= 3.20
```bash
cmake --version  # Check current version

# Falls zu alt, von cmake.org herunterladen oder:
sudo apt-get install -y cmake
```

### vcpkg Setup
```bash
cd /path/to/themis/vcpkg
./bootstrap-vcpkg.sh

# Oder mit Metrics deaktiviert:
./bootstrap-vcpkg.sh -disableMetrics
```

## Quick Start

### Methode 1: CMake Presets

```bash
cd /path/to/themis

# Release Build konfigurieren
cmake --preset linux-gcc-release

# Build
cmake --build build-wsl --parallel 8

# Binary findet sich in:
# /path/to/themis/build-wsl/themis_server
```

### Methode 2: Manuelle CMake Commands

```bash
cd /path/to/themis

# Configure
cmake -S . -B build-wsl \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"

# Build
cmake --build build-wsl --parallel 8
```

### Methode 3: Shell Script

```bash
cd /path/to/themis
./scripts/build-linux.sh
```

## WSL2 Spezifika

### WSL2 Umgebung Setup
```powershell
# Aus Windows PowerShell:
wsl --install Ubuntu-24.04

# Oder Updates:
wsl --update
```

### WSL2 vs WSL1
- **WSL2**: Vollständiger Linux Kernel, bessere Performance ✅
- **WSL1**: Kompatibilitätsmodus, langsamer

Zu WSL2 wechseln:
```powershell
# PowerShell (Admin)
wsl --set-version Ubuntu-24.04 2
```

### Windows-Linux Interop
```bash
# Von WSL zum Windows-Home:
cd /mnt/c/Users/YourUser/Documents

# Umgekehrt (von cmd.exe):
wsl -- /path/to/file
```

## Build Varianten

### Standard Community Edition
```bash
cmake --preset linux-gcc-release
cmake --build build-wsl --parallel 8
```

### Hyperscaler mit LLM + GPU
```bash
cmake -S . -B build-wsl \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_TRACING=ON

cmake --build build-wsl --parallel 8
```

### Debug Build
```bash
cmake --preset linux-gcc-debug
cmake --build build-wsl --parallel 8 --config Debug
```

## Tests ausführen

```bash
cd /path/to/themis/build-wsl

# Alle Tests
ctest --parallel 8 --output-on-failure

# Spezifische Test-Suite
ctest -R "ThreadSafety" -V

# Mit CTest Output
ctest --output-on-failure -j8
```

## Compile Warnings Überprüfen

```bash
cd /path/to/themis/build-wsl

# Nach dem Build
grep -i "warning" compile_commands.json

# Oder mit ccache für schnellere Rebuilds:
export CCACHE_BASEDIR=/path/to/themis
cmake -S . -B build-wsl -DCMAKE_C_COMPILER=ccache\ gcc -DCMAKE_CXX_COMPILER=ccache\ g++
```

## Performance-Tips

### Parallel Build
```bash
# Auto-detect CPU cores
cmake --build build-wsl --parallel $(nproc)

# Oder manuell
cmake --build build-wsl --parallel 8
```

### Incremental Builds
```bash
# Nach Code-Änderungen: Nur geänderte Dateien
cmake --build build-wsl --parallel 8
```

### Link-Time Optimization (LTO)
```bash
cmake -S . -B build-wsl \
  -G Ninja \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-wsl --parallel 8
```
⚠️ Warnung: LTO kann Build um 50% verlangsamen.

### ccache für schnellere Rebuilds
```bash
sudo apt-get install ccache

cmake -S . -B build-wsl \
  -DCMAKE_C_COMPILER=ccache\ gcc \
  -DCMAKE_CXX_COMPILER=ccache\ g++ \
  ...

# Größe und Statistiken:
ccache -s
```

## Alternative Compiler

### Clang
```bash
sudo apt-get install clang-14

cmake -S . -B build-clang \
  -DCMAKE_C_COMPILER=/usr/bin/clang-14 \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++-14 \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-clang --parallel 8
```

### GCC-12
```bash
sudo apt-get install gcc-12 g++-12

cmake -S . -B build-gcc12 \
  -DCMAKE_C_COMPILER=/usr/bin/gcc-12 \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++-12 \
  ...
```

## Output Locations

| Artifact | Pfad |
|----------|------|
| Server Executable | `build-wsl/themis_server` |
| Tests Executable | `build-wsl/themis_tests` |
| Libraries | `build-wsl/lib*.a` |
| Compile Commands | `build-wsl/compile_commands.json` |

## Troubleshooting

### Problem: vcpkg Module nicht gefunden
```bash
export VCPKG_ROOT=/path/to/themis/vcpkg
cmake --preset linux-gcc-release
```

### Problem: "gcc: command not found"
```bash
sudo apt-get install build-essential
gcc --version
```

### Problem: Zu wenig Disk Space
```bash
# Cleanup
rm -rf build-wsl
cmake --preset linux-gcc-release  # Start fresh
```

### Problem: "undefined reference to `xyz'"
**Lösung**: vcpkg neukonfigurieren:
```bash
rm -rf build-wsl
export VCPKG_ROOT=/path/to/themis/vcpkg
cmake --preset linux-gcc-release -DVCPKG_ROOT=$VCPKG_ROOT
```

### Problem: Locale errors
```bash
export LC_ALL=C.UTF-8
export LANG=C.UTF-8
cmake --preset linux-gcc-release
```

## Cross-Compilation (für ARM)

Falls Sie für ARM64 (z.B. Apple Silicon) bauen wollen:

```bash
# Auf macOS (ARM64):
cmake -S . -B build-macos-arm64 \
  -DCMAKE_SYSTEM_PROCESSOR=arm64 \
  -DCMAKE_SYSTEM_NAME=Darwin \
  ...

cmake --build build-macos-arm64
```

## Nächste Schritte

Nach erfolgreichem Build lesen Sie:
- **Deployment**: [docs/de/deployment/deployment_strategy.md](../../de/deployment/deployment_strategy.md)
- **Releases**: [docs/de/releases/updates_distribution_strategy.md](../../de/releases/updates_distribution_strategy.md)

## Weitere Infos

- [cmake/CMakePresets.json](../../cmake/CMakePresets.json) - Alle Presets
- [cmake/CMakeLists.txt](../../cmake/CMakeLists.txt) - CMake Konfiguration
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Weitere Fehlersuche
- [WSL Docs](https://docs.microsoft.com/en-us/windows/wsl/) - Microsoft WSL Dokumentation
