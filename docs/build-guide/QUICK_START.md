# Quick Start Guide - Building ThemisDB

**Duration**: ~10 minutes  
**Difficulty**: Beginner-friendly

## 5-Minute Express Start

### Windows

```powershell
cd C:\VCC\themis

# Step 1: Configure (2 minutes)
cmake --preset windows-vs2022-release

# Step 2: Build (3-5 minutes)
cmake --build --preset windows-vs2022-release --parallel 8

# Result: C:\VCC\themis\build-msvc\Release\themis_server.exe
```

### Linux/WSL

```bash
cd /mnt/c/VCC/themis  # or ~/themis

# Step 1: Configure
cmake --preset linux-gcc-release

# Step 2: Build
cmake --build --preset linux-gcc-release --parallel 8

# Result: /themis/build-wsl/themis_server
```

### macOS

```bash
cd ~/themis

# Step 1: Configure
cmake --preset macos-clang-release

# Step 2: Build
cmake --build --preset macos-clang-release --parallel 8

# Result: ~/themis/build-macos/themis_server
```

---

## One-Liner Builds

### Just Compile (No Tests/Benchmarks)

**Windows**:
```powershell
cmake --preset windows-vs2022-release && cmake --build --preset windows-vs2022-release
```

**Linux**:
```bash
cmake --preset linux-gcc-release && cmake --build --preset linux-gcc-release
```

### Maximum Performance (with GPU)

**Windows**:
```powershell
cmake -S . -B build-perf -DTHEMIS_EDITION=HYPERSCALER -DTHEMIS_ENABLE_GPU=ON && cmake --build build-perf --parallel 8
```

**Linux**:
```bash
cmake -S . -B build-perf -DTHEMIS_EDITION=HYPERSCALER -DTHEMIS_ENABLE_GPU=ON && cmake --build build-perf --parallel 8
```

### Minimal for Embedded (50 MB)

```bash
cmake -S . -B build-minimal -DTHEMIS_EDITION=MINIMAL -DTHEMIS_BUILD_TESTS=OFF
cmake --build build-minimal
```

---

## Prerequisites Checklist

### Windows

- ✅ Visual Studio 2022 (with C++ tools)
  ```powershell
  cl /v  # Check: should show v19.44 or later
  ```

- ✅ CMake 3.20+
  ```powershell
  cmake --version
  ```

- ✅ vcpkg (auto-setup)
  ```powershell
  cd C:\VCC\themis\vcpkg
  .\bootstrap-vcpkg.bat
  ```

### Linux

- ✅ GCC 11+ or Clang 14+
  ```bash
  g++ --version  # Check version
  ```

- ✅ CMake 3.20+
  ```bash
  cmake --version
  ```

- ✅ Build tools
  ```bash
  sudo apt install build-essential  # Ubuntu/Debian
  ```

---

## Available Presets

### Windows Presets

```bash
# Release builds (fast runtime)
cmake --preset windows-vs2022-release          # Visual Studio 2022
cmake --preset windows-ninja-msvc-release      # Ninja + MSVC (faster)

# Debug builds (with debugging info)
cmake --preset windows-vs2022-debug
cmake --preset windows-ninja-msvc-debug

# Alternative compilers
cmake --preset windows-ninja-clangcl-release   # Clang (experimental)
```

### Linux Presets

```bash
cmake --preset linux-gcc-release               # GCC Compiler
cmake --preset linux-clang-release             # Clang Compiler
cmake --preset linux-gcc-debug                 # GCC + Debug Info
```

### macOS Presets

```bash
cmake --preset macos-clang-release
cmake --preset macos-clang-debug
```

---

## Feature Editions

Choose your edition based on your use case:

### 🟢 MINIMAL (50 MB)
Embedded, IoT, QNAP NAS

```powershell
cmake -S . -B build -DTHEMIS_EDITION=MINIMAL
```

**Includes**: Core database, SQL, vector search  
**Excludes**: LLM, GPU, gRPC, advanced protocols

### 🔵 COMMUNITY (150 MB - Default)
Development, small production

```powershell
cmake -S . -B build  # Or -DTHEMIS_EDITION=COMMUNITY
```

**Includes**: Everything except enterprise features  
**Best for**: Learning, testing, single-node deployments

### 🟠 ENTERPRISE (250 MB)
Data centers, distributed deployments

```powershell
cmake -S . -B build -DTHEMIS_EDITION=ENTERPRISE
```

**Includes**: gRPC sharding, OpenTelemetry tracing  
**Best for**: Multi-node clusters, observability

### 🔴 HYPERSCALER (500 MB)
Cloud, AI/ML, maximum features

```powershell
cmake -S . -B build -DTHEMIS_EDITION=HYPERSCALER
```

**Includes**: LLM, GPU, gRPC, tracing, HTTP/3  
**Best for**: AI workloads, large-scale cloud deployments

---

## Verification: Did It Work?

### Check 1: Binary Created

**Windows**:
```powershell
Test-Path "C:\VCC\themis\build-msvc\Release\themis_server.exe"
# Should output: True
```

**Linux**:
```bash
test -f /path/to/build-wsl/themis_server && echo "✅ Binary found" || echo "❌ Not found"
```

### Check 2: Binary Runs

**Windows**:
```powershell
& "C:\VCC\themis\build-msvc\Release\themis_server.exe" --version
```

**Linux**:
```bash
/path/to/build-wsl/themis_server --version
```

Should print version info (e.g., `Themis 1.4.0-alpha`)

### Check 3: Features Enabled

```bash
themis_server --version --features
```

Output should show which features are enabled (LLM, GPU, gRPC, etc.)

---

## Run Tests (Optional)

```bash
cd build-msvc  # or build-wsl
ctest -j 8 --output-on-failure
```

Runs ~180 unit tests. Takes 2-5 minutes.

---

## Common Issues

### ❌ "Could not find package OpenSSL"

```bash
# Clear CMake cache and retry
cmake --preset windows-vs2022-release --fresh
```

### ❌ "CMAKE_CXX_COMPILER not set"

```powershell
# Run VS2022 Developer PowerShell
"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\Launch-VsDevShell.ps1"
```

### ❌ "Access Denied during build"

```powershell
# Kill stray compiler processes
taskkill /f /im cl.exe
```

### ❌ Build takes forever (>10 minutes)

```bash
# Increase parallel jobs (adjust to your CPU cores)
cmake --build build-msvc --parallel 16
```

---

## Next: Start the Server

After successful build:

**Windows**:
```powershell
& "C:\VCC\themis\build-msvc\Release\themis_server.exe"
# Server listening on http://localhost:7000
```

**Linux**:
```bash
./build-wsl/themis_server
# Server listening on http://localhost:7000
```

---

## API Quick Test

Once server is running:

```bash
# Get version
curl http://localhost:7000/api/v1/version

# Create a table
curl -X POST http://localhost:7000/api/v1/tables \
  -H "Content-Type: application/json" \
  -d '{"name":"my_table","columns":[{"name":"id","type":"int64"},{"name":"text","type":"string"}]}'

# Insert data
curl -X POST http://localhost:7000/api/v1/tables/my_table/rows \
  -H "Content-Type: application/json" \
  -d '[{"id":1,"text":"Hello"},{"id":2,"text":"World"}]'

# Query data
curl http://localhost:7000/api/v1/tables/my_table/rows
```

---

## Advanced: Custom Build Flags

Combine edition with custom flags:

### LLM Inference

```bash
cmake -S . -B build-llm \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_ENABLE_LLM=ON
cmake --build build-llm
```

Requires: `llama.cpp` cloned to `./llama.cpp`

### GPU Acceleration

```bash
cmake -S . -B build-gpu \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_CUDA=ON
cmake --build build-gpu
```

Requires: NVIDIA CUDA Toolkit 12.4+

### Distributed Sharding

```bash
cmake -S . -B build-enterprise \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_ENABLE_GRPC=ON
cmake --build build-enterprise
```

---

## What's New in v1.4.0

✨ **Major Improvements**:

1. **Modular CMake** - Clean architecture, easy to understand
2. **Path Resolution Fixed** - No more "cannot find source file" errors
3. **Edition System** - Choose what you build (MINIMAL/COMMUNITY/ENTERPRISE/HYPERSCALER)
4. **Feature Flags** - 40+ options for granular control
5. **CMake Presets** - Simple, IDE-friendly configuration

📚 See: [docs/architecture/CMAKE_MODULAR_ARCHITECTURE.md](../architecture/CMAKE_MODULAR_ARCHITECTURE.md)

---

## Documentation Map

- **Quick Start** (you are here)
- [Windows Build Guide](../build-guide/BUILD_WINDOWS.md)
- [Linux Build Guide](../build-guide/BUILD_LINUX.md)
- [CMake Architecture](../architecture/CMAKE_MODULAR_ARCHITECTURE.md)
- [Feature Flags](../architecture/FEATURE_FLAGS_REFERENCE.md)
- [Migration v1.3→v1.4](../architecture/MIGRATION_GUIDE_v13_v14.md)

---

## Need Help?

- 🐛 **Bug?** Report on GitHub Issues
- 💬 **Question?** Ask in GitHub Discussions
- 📖 **More docs?** https://themisdb.readthedocs.io
- 🎯 **Specific topic?** Check architecture documentation

---

**Happy Building! 🚀**
