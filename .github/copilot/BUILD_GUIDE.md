# ThemisDB - Build System Guide

## Modern Build Architecture (v2.1 - Jan 2026)

### Directory Structure

```
themis/
├── CMakeLists.txt                    # Root CMake (delegates to cmake/)
├── cmake/                            # ✨ Centralized
│   ├── CMakeLists.txt                # Main configuration (2600+ lines)
│   ├── CMakePresets.json             # All build profiles (MSVC, WSL, Docker)
│   ├── config/                       # Feature configuration
│   ├── modules/                      # CMake modules
│   └── ModularBuild.cmake
├── docker/                           # ✨ Centralized
│   ├── Dockerfile.themis-server      # Production (LLM + GPU)
│   ├── Dockerfile.minimal            # Minimal Edition
│   ├── Dockerfile.qnap               # QNAP NAS
│   └── docker-compose-minimal.yml
└── docs/build-guide/                 # ✨ Detailed documentation
```

### vcpkg Offline-First Architecture

**vcpkg Cache Structure:**
```
vcpkg/
├── downloads/          # ~2.5 GB - Source Archives
├── buildtrees/         # ~3 GB - Temporary build artifacts
├── packages/           # ~10 GB - Installed packages
└── scripts/buildsystems/vcpkg.cmake
```

**One-time Setup:**
```powershell
# Windows
.\scripts\setup-vcpkg-offline.ps1

# Linux/WSL
./scripts/setup-vcpkg-offline.sh
```

## Build Platforms & Triplets

| Platform | Triplet | Build Directory | Generator |
|----------|---------|-----------------|-----------|
| **Windows MSVC** | x64-windows | `build-msvc` | Visual Studio 17 2022 |
| **Linux/WSL x64** | x64-linux | `build-wsl` or `build-linux` | Ninja |
| **Linux ARM64** | arm64-linux | `build-arm` | Ninja |
| **Docker Multi-Arch** | x64-linux, arm64-linux | Container | Multi-stage |
| **QNAP NAS** | x64-linux | `build-qnap` | Ninja |

## Quick Start Commands

All builds use **CMake Presets** (defined in `cmake/CMakePresets.json`).

### Recommended: VSCode CMake Tools

**Best practice for daily development:**

1. **Install CMake Tools extension** (ms-vscode.cmake-tools)
2. **Select preset:** Command Palette → "CMake: Select Configure Preset"
3. **Build:** Press `F7` or Command Palette → "CMake: Build"
4. **Test:** Command Palette → "CMake: Run Tests"
5. **Debug:** Press `F5` with debug configuration

**Benefits:**
- ✅ Automatic preset detection
- ✅ Integrated build/test/debug
- ✅ IntelliSense auto-configuration
- ✅ Progress visualization
- ✅ Error navigation

See [VSCODE_CONTEXT.md](VSCODE_CONTEXT.md) for detailed setup.

---

### Command Line (Alternative)

For CI/CD or when VSCode is not available:

### Windows (MSVC 2022)

```powershell
cd C:\VCC\themis

# With Preset (recommended)
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8

# Or manual
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON

cmake --build build-msvc --config Release --parallel 8
```

**Output:** `build-msvc/Release/themis_server.exe`  
**Docs:** [docs/build-guide/BUILD_WINDOWS.md](../../docs/build-guide/BUILD_WINDOWS.md)

### Linux/WSL (GCC/Ninja)

```bash
cd /path/to/themis

# With Preset
cmake --preset linux-gcc-release
cmake --build build-wsl --parallel 8

# Or manual
cmake -S . -B build-wsl -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_ENABLE_LLM=ON

cmake --build build-wsl --parallel $(nproc)
```

**Output:** `build-wsl/themis_server`  
**Docs:** [docs/build-guide/BUILD_LINUX.md](../../docs/build-guide/BUILD_LINUX.md)

### Docker (Multi-Arch: x86_64 + ARM64)

```bash
docker build -f docker/Dockerfile.themis-server \
  -t themis-server:hyperscaler-llm \
  --build-arg THEMIS_ENABLE_LLM=ON \
  --build-arg THEMIS_ENABLE_GPU=ON \
  .
```

**Docs:** [docs/build-guide/BUILD_DOCKER.md](../../docs/build-guide/BUILD_DOCKER.md)  
**Note:** Multi-Stage Build (Builder 2.5GB → Runtime 200MB)

### Raspberry Pi (ARM64)

```bash
# Native build on RPi 4+ with reduced parallelism
cmake -S . -B build-rpi -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DTHEMIS_BUILD_TESTS=OFF

cmake --build build-rpi --parallel 2
```

**Output:** `build-rpi/themis_server`  
**Docs:** [docs/build-guide/BUILD_RASPBERRY_PI.md](../../docs/build-guide/BUILD_RASPBERRY_PI.md)

### QNAP NAS (x86_64 Docker)

```bash
docker build -f docker/Dockerfile.qnap \
  -t themis-server:qnap \
  --build-arg THEMIS_ENABLE_LLM=OFF \
  .
```

**Docs:** [docs/build-guide/BUILD_QNAP.md](../../docs/build-guide/BUILD_QNAP.md)

## Edition-Specific Builds

| Edition | Script | CMake Flag | Features |
|---------|--------|-----------|----------|
| **Community** | `build-community-release.ps1` | `-DTHEMIS_EDITION=COMMUNITY` | Core, 24GB GPU, Single-Node |
| **Enterprise** | `build-enterprise-release.ps1` | `-DTHEMIS_EDITION=ENTERPRISE` | + Sharding, 256GB GPU, 100 Nodes |
| **Hyperscaler** | `build-hyperscaler-release.ps1` | `-DTHEMIS_EDITION=HYPERSCALER` | Unlimited |

## Feature Flags

```bash
# Minimal Build (Core only, ~150 MB)
cmake -B build -DTHEMIS_ENABLE_LLM=OFF -DTHEMIS_BUILD_RPC_FRAMEWORK=OFF

# LLM Build (Core + llama.cpp, ~250 MB)
cmake -B build -DTHEMIS_ENABLE_LLM=ON

# Full Build (Core + LLM + RPC + GPU, ~350 MB)
cmake -B build \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_BUILD_RPC_FRAMEWORK=ON \
  -DTHEMIS_ENABLE_GPU=ON
```

## Important CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `THEMIS_CORE_SHARED` | OFF | Build themis_core as DLL/SO instead of static |
| `THEMIS_STATIC_BUILD` | OFF | Fully static binary (Docker/QNAP) |
| `THEMIS_ENABLE_LLM` | OFF | llama.cpp integration |
| `THEMIS_ENABLE_GPU` | OFF | CUDA/Vulkan/ROCm support |
| `THEMIS_BUILD_TESTS` | Debug: ON / Release: OFF | Google Test suite, enabled by default for debug/dev builds |
| `THEMIS_BUILD_BENCHMARKS` | OFF | Google Benchmark suite, opt-in for benchmark-only profiles |
| `THEMIS_MODULES_ENABLE_UNITY` | OFF | Unity Build for modular MSVC targets (explicit opt-in) |
| `THEMIS_ENABLE_IPO` | OFF | IPO/LTO for Release builds only (explicit opt-in) |
| `THEMIS_QNAP_BUILD` | OFF | QNAP NAS optimizations (SSE4.2 baseline) |

### Build-System Guardrails (CMake Best-Practice)

- Include paths are applied target-scoped via `target_include_directories(...)`.
- Feature/build macros are applied target-scoped via `target_compile_definitions(...)`.
- Unity and IPO/LTO are disabled by default and must be enabled explicitly.
- Cache overrides from users/presets are respected; defaults are no longer broadly forced for debug/release policy flags.

## Troubleshooting

### CMake find_package Issues (FAISS/gRPC)

```powershell
# Quick Fix (CMake-native bootstrap)
cmake --preset windows-release -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON

# Or manually set CMAKE_PREFIX_PATH
$VCPKG = "C:\VCC\themis\vcpkg_installed\x64-windows"
cmake -DCMAKE_PREFIX_PATH="$VCPKG;$VCPKG\share" ...
```

### vcpkg Lock Contention (fmt/RocksDB in Auto-Bootstrap)

- Symptom: configure stops with missing `fmt` or `RocksDB` after an auto-bootstrap attempt.
- Root cause: another concurrent `vcpkg` process holds `vcpkg-running.lock`.
- CMake now reports this explicitly as lock contention.

Recovery (CMake-native flow):

```powershell
# 1) ensure no parallel configure/vcpkg process is running
# 2) rerun configure with auto-bootstrap enabled
cmake --preset windows-release -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON
```

Tune auto-bootstrap behavior when needed:

```powershell
# default retry/backoff: 2 retries, 3s delay
# default install timeout per attempt: 900s
# fail-fast mode: stop immediately when another vcpkg process already holds the lock
cmake --preset windows-release \
  -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON \
  -DTHEMIS_VCPKG_INSTALL_RETRY_COUNT=3 \
  -DTHEMIS_VCPKG_INSTALL_RETRY_DELAY_SEC=5 \
  -DTHEMIS_VCPKG_LOCK_WAIT_TIMEOUT_SEC=45 \
  -DTHEMIS_VCPKG_LOCK_WAIT_POLL_SEC=2 \
  -DTHEMIS_VCPKG_INSTALL_TIMEOUT_SEC=1800 \
  -DTHEMIS_VCPKG_FAIL_FAST_ON_LOCK=ON
```

Use fail-fast mode when you want an immediate diagnosis instead of waiting for lock expiry:

```powershell
cmake -S . -B build-fail-fast -G Ninja -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON -DTHEMIS_VCPKG_FAIL_FAST_ON_LOCK=ON
```

If lock contention persists, run once manually in the repository root:

```powershell
vcpkg install --triplet x64-windows
```

### WSL Build Errors (MSVC Flags on Linux)

- **Problem:** `/O1`, `/bigobj`, `/Bt+` on Linux
- **Fix:** Gate in CMakeLists.txt with `if(MSVC)`

### Docker Build Context Issues

- **Problem:** vcpkg/buildtrees too large
- **Fix:** Update .dockerignore, only include downloads/

## Docker-Specific: vcpkg Cache Strategy

For Docker builds, see: [docker/DOCKER_BUILD_STRATEGY_QUICKREF.md](../../docker/DOCKER_BUILD_STRATEGY_QUICKREF.md#vcpkg-triple-cache-strategie-)

- Triple-Cache uses host packages from `./vcpkg/packages/` + BuildKit Cache
- No unnecessary downloads/rebuilds for existing dependencies
- Check bind-mounts in `Dockerfile.unified` if cache issues occur

## Additional Documentation

- `docs/de/deployment/deployment_strategy.md` - Overall strategy
- `docs/de/deployment/deployment_arm_build.md` - ARM/Raspberry Pi builds
- `docs/de/deployment/BUILD_OPTIONEN_REFERENZ.md` - All CMake flags
- `docs/de/deployment/QUICK_REFERENCE.md` - CMake find_package fixes
