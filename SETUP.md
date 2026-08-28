# ThemisDB Development Setup

> Canonical **Step 2** after [QUICKSTART.md](QUICKSTART.md).
> This page defines the authoritative local development setup.

---

## Scope and Canonical Flow

Use the root onboarding path in this order:

1. [README.md](README.md)
2. [QUICKSTART.md](QUICKSTART.md)
3. [SETUP.md](SETUP.md) *(this page)*
4. [SUPPORT.md](SUPPORT.md)
5. [RELEASE_STRATEGY.md](RELEASE_STRATEGY.md)
6. [INDEX.md](INDEX.md)

---

## Prerequisites

- Git 2.x
- Python 3.8+
- CMake 3.20+
- C++20 compiler (GCC/Clang/MSVC)
- PowerShell 7+ (for cross-platform dependency bootstrap script)

Optional for fastest onboarding:

- Docker / Docker Desktop
- VS Code + Dev Containers extension

---

## 1) Clone Repository

```bash
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB
git submodule update --init --recursive
```

---

## 2) Install Hooks and Bootstrap Dependencies via CMake

### Linux / macOS

```bash
./scripts/setup-pre-commit.sh
cmake --preset linux-release -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON
```

### Windows (PowerShell)

```powershell
.\scripts\setup-pre-commit.ps1
cmake --preset windows-release -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON
```

---

## 3) Configure and Build with Canonical Presets

Available canonical build/test presets are defined in [CMakePresets.json](CMakePresets.json).

### Preset Selection Guide

ThemisDB provides multiple presets optimized for different scenarios:

#### **Primary Presets** (use these for standard development)

| Preset | Platform | Requirements | Use Case |
|--------|----------|--------------|----------|
| **linux-release** | Linux | Ninja + vcpkg | Production builds with full optimizations (recommended for most users) |
| **windows-release** | Windows | MSVC 2022+ + Ninja + vcpkg | Production builds on Windows (MSVC only) |
| **community-release** | Linux/macOS | System packages only (no vcpkg) | Fallback preset when vcpkg is unavailable; uses system-installed libraries |

#### **Development Presets** (for development and debugging)

| Preset | Platform | Requirements | Use Case |
|--------|----------|--------------|----------|
| **linux-debug** | Linux | Ninja + vcpkg | Debug builds with symbols and assertions |
| **windows-debug** | Windows | MSVC 2022+ + Ninja + vcpkg | Debug builds on Windows |
| **nightly-bench-sweep** | Linux | Ninja + vcpkg | Nightly benchmarking with GPU/LLM disabled |

### Choosing the Right Preset

1. **If vcpkg is available** (recommended):
   - Linux: Use `linux-release`
   - Windows: Use `windows-release`
   - Benefits: Consistent dependencies, optimized builds, reproducible CI

2. **If vcpkg is unavailable** (fallback):
   - Use `community-release` on Linux/macOS
   - Requires system development packages to be installed
   - See [System Package Setup](#system-package-setup) below

### Quick Start: Linux x64

#### With vcpkg (recommended):

```bash
# Prerequisites: submodules initialized or CMake auto-bootstrap enabled

cmake --preset linux-release
cmake --build --preset linux-release --parallel 16
ctest --preset linux-release --output-on-failure
```

#### Without vcpkg (fallback):

```bash
# Install system development packages (see System Package Setup)
cmake --preset community-release
cmake --build --preset community-release --parallel 16
ctest --preset community-release --output-on-failure
```

### Quick Start: Windows x64

From a Visual Studio Developer Command Prompt:

```powershell
# Prerequisites: submodules initialized or CMake auto-bootstrap enabled

cmake --preset windows-release
cmake --build --preset windows-release --parallel 16
ctest --preset windows-release --output-on-failure
```

### System Package Setup

If using `community-release` preset, install required system development packages:

#### Debian/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  libssl-dev \
  zlib1g-dev \
  librocksdb-dev \
  libzstd-dev \
  libfmt-dev \
  libspdlog-dev \
  libcpp-httplib-dev
```

#### Fedora/RHEL:

```bash
sudo dnf install -y \
  gcc-c++ \
  cmake \
  ninja-build \
  openssl-devel \
  zlib-devel \
  rocksdb-devel \
  libzstd-devel \
  fmt-devel \
  spdlog-devel
```

#### macOS:

```bash
brew install cmake ninja rocksdb zstd openssl fmt spdlog
export OPENSSL_DIR=$(brew --prefix openssl@3)
```

---

## 3b) Build with Presets

## 3b) Build and Test Details

Full build and test workflow with parallel jobs:

### Configure

```bash
cmake --preset <PRESET_NAME> -S . -B build/<PRESET_NAME>
```

### Build with parallelization

```bash
cmake --build --preset <PRESET_NAME> --parallel 16
```

Or use the build command directly:

```bash
cmake --build build/<PRESET_NAME> --parallel 16
```

### Run tests

```bash
ctest --preset <PRESET_NAME> --output-on-failure
```

---

## 3c) Reproducible Builds Policy

ThemisDB build metadata is reproducible by policy:

- `cmake/BuildInfo.cmake` uses `SOURCE_DATE_EPOCH` when it is set.
- If `SOURCE_DATE_EPOCH` is not set, BuildInfo falls back to the Git `HEAD`
  commit timestamp so repeated builds from the same source tree do not drift by
  configure time alone.
- If neither source is available, the build falls back to configure time only
  for non-strict local builds and emits a warning because that output is not
  reproducible.

### Strict mode for CI, release packaging, and audit builds

Use `-DTHEMIS_REQUIRE_REPRODUCIBLE_BUILD=ON` whenever a build must fail instead
of silently falling back to non-reproducible metadata.

```bash
export SOURCE_DATE_EPOCH="$(git log -1 --format=%ct)"
cmake --preset community-release -DTHEMIS_REQUIRE_REPRODUCIBLE_BUILD=ON
cmake --build --preset community-release --parallel 16
```

This follows the `SOURCE_DATE_EPOCH` convention used by Debian, F-Droid, and
the wider reproducible-builds ecosystem.

CI enforcement lives in
`.github/workflows/09-pr-gates_reproducible-builds.yml`, which uploads the
generated `build_info.h` files plus their SHA-256 digests as audit evidence.

### Local verification

Run the focused reproducibility probe to verify that identical source plus
identical `SOURCE_DATE_EPOCH` produces identical generated build metadata:

```bash
export SOURCE_DATE_EPOCH="$(git log -1 --format=%ct)"
cmake -DOUTPUT_DIR=/tmp/themis-repro -P cmake/VerifyReproducibleBuildInfo.cmake
```

The probe configures isolated temporary build directories, compares the SHA-256
hashes of the generated `build_info.h` artifacts, and fails if the metadata is
not deterministic.

---

## 4) Troubleshooting Presets

### Error: "Could not find toolchain file: vcpkg/scripts/buildsystems/vcpkg.cmake"

**Cause**: Using a vcpkg-based preset (linux-release, windows-release) without initialized submodules/bootstrap.

**Solution**:
1. Use CMake-native auto-bootstrap:
   ```bash
   cmake --preset linux-release -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON
   ```

   Optional manual fallback:
   ```bash
   git submodule update --init --recursive
   ```

2. Or use fallback preset:
   ```bash
   cmake --preset community-release  # Linux/macOS only
   ```

### Error: "Could not find build program corresponding to Ninja"

**Cause**: Using a Ninja-based preset without Ninja installed.

**Solution**:
- Install Ninja:
  ```bash
  sudo apt-get install ninja-build   # Debian/Ubuntu
  brew install ninja                  # macOS
  choco install ninja                 # Windows (with Chocolatey)
  ```

### Error: "RocksDB not found. Install via vcpkg or system package librocksdb-dev"

**Cause**: Using `community-release` without required system packages.

**Solution**: Install system development packages (see [System Package Setup](#system-package-setup) section).

### Error: "fmt not found" or "spdlog not found"

**Cause**: Using `community-release` on a system where fmt and spdlog are not installed as system packages.

**Solution**: Install development packages for fmt and spdlog:
- Debian/Ubuntu: `sudo apt-get install libfmt-dev libspdlog-dev`
- Fedora/RHEL: `sudo dnf install fmt-devel spdlog-devel`
- macOS: `brew install fmt spdlog`

Alternatively, use the `linux-release` preset with vcpkg, which includes all dependencies.

### Error: "httplib.h: No such file or directory"

**Cause**: Using `community-release` on a system where cpp-httplib is installed only via system packages and the required development package is missing.

**Solution**: Install the cpp-httplib development package:
- Debian/Ubuntu: `sudo apt-get install libcpp-httplib-dev`
- Fedora/RHEL: install the distro package that provides `httplib.h` / `cpp-httplib`
- macOS: use the `linux-release` preset with vcpkg or provide a compatible `cpp-httplib` install

### Build Reproducibility Issues on `linux-release` or `community-release`

**Batch A Gate Status**: [~] In Progress (active Phase-0 gate blockers)

**Known Issues**:

1. **`linux-release` requires vcpkg toolchain**: If you receive "Could not find toolchain file", ensure:
   - submodules are initialized (`git submodule update --init --recursive`)
   - configure is retried with `-DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON`
   - CMAKE_TOOLCHAIN_FILE in CMakePresets.json points to correct path

2. **`community-release` may fail on missing packages**: This preset depends on system development packages being installed.
   - Use the [System Package Setup](#system-package-setup) commands above for your OS
   - Or use `linux-release` preset with vcpkg if available

3. **Reproducible builds require SOURCE_DATE_EPOCH**: For CI and release builds:
   ```bash
   export SOURCE_DATE_EPOCH="$(git log -1 --format=%ct)"
   cmake --preset linux-release -DTHEMIS_REQUIRE_REPRODUCIBLE_BUILD=ON
   ```
   See [Reproducible Builds Policy](#3c-reproducible-builds-policy) above.

**Mitigation**: Both presets are functional and gate-integrated for release-critical tests. Known limitations are tracked in `ROADMAP.md` §Known Issues & Limitations.

**Cause**: CMakePresets.json has invalid syntax or preset references.

**Solution**:
1. Verify CMakePresets.json syntax:
   ```bash
   python3 -m json.tool CMakePresets.json
   ```

2. Verify preset is available:
   ```bash
   cmake --list-presets
   ```

3. Check the Git history for recent changes:
   ```bash
   git log -p CMakePresets.json | head -100
   ```

---

## 5) Run Server from Local Build

```bash
./build/linux-release/themis_server --data-dir ./data
```

Health check:

```bash
curl http://localhost:8765/health
```

---

## 6) Optional: Dev Container

1. Open repository in VS Code
2. Run `Dev Containers: Reopen in Container`
3. Build and test via the same canonical presets (`linux-release` / `linux-debug`)

---

## Preset Matrix Summary

For quick reference, here's the complete preset matrix:

| Preset | Platform | Generator | vcpkg | System Packages | Use Case |
|--------|----------|-----------|-------|-----------------|----------|
| linux-release | Linux | Ninja | ✅ Required | Optional | **Primary** - Production with vcpkg |
| linux-debug | Linux | Ninja | ✅ Required | Optional | Development debug build |
| windows-release | Windows | Ninja | ✅ Required | N/A | **Primary** - Windows production |
| windows-debug | Windows | Ninja | ✅ Required | N/A | Windows debug build |
| community-release | Linux/macOS | Ninja | ❌ Not used | ✅ Required | **Fallback** - System packages only |
| nightly-bench-sweep | Linux | Ninja | ✅ Required | Optional | Benchmarking (no GPU/LLM) |
| hyperscaler-debug-windows | Windows | Ninja | ✅ Required | N/A | Enterprise debug builds |
| hyperscaler-debug-linux | Linux | Ninja | ✅ Required | Optional | Enterprise debug builds |

---

## Deprecated / Non-Canonical Instructions

## Need Help?

- [SUPPORT.md](SUPPORT.md)
- [docs/FAQ.md](docs/FAQ.md)

---
Zuletzt geprueft (Root-Sync): 2026-05-26
