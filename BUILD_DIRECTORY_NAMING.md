# Build Directory Naming Convention

**Version:** 1.0  
**Date:** February 15, 2026  
**Status:** Active - All build systems standardized

## Overview

Build directories follow a **consistent naming pattern** across all build systems (Windows, WSL, Linux, Docker, VS Code, Ninja, etc.) to ensure clarity and maintainability.

## Naming Schema

```
build-<generator>-<config>[-<variant>]
```

**Components:**
- **generator**: The CMake generator and toolchain used
- **config**: Build configuration (release, debug)
- **variant** (optional): Special build type (static, minimal, gpu-llm, etc.)

---

## Standard Build Directories

### Windows (MSVC + Ninja)

| Directory | CMake Preset | Generator | Toolchain | Config | Features |
|-----------|-------------|-----------|-----------|--------|----------|
| `build-msvc-ninja-release` | `msvc-ninja-release` | Ninja | MSVC cl.exe | Release | Tests, GPU ON, LLM ON |
| `build-msvc-ninja-debug` | `msvc-ninja-debug` | Ninja | MSVC cl.exe | Debug | Tests, GPU OFF, LLM ON |
| `build-msvc-ninja-release-static` | `msvc-ninja-release-static` | Ninja | MSVC cl.exe | Release | Static libs, x64-windows-static |

**Usage:**
```bash
# Configure
cmake --preset msvc-ninja-release

# Build
cmake --build --preset msvc-ninja-release

# Or manually
cd build-msvc-ninja-release
ctest
```

### Windows (Visual Studio Generator - Legacy)

| Directory | CMake Preset | Generator | Status |
|-----------|-------------|-----------|--------|
| `build-vs-release` | `vs-release` | Visual Studio 17 2022 | Deprecated (Ninja preferred) |

---

### WSL / Linux (GCC + Ninja)

| Directory | CMake Preset | Generator | Toolchain | Config |
|-----------|-------------|-----------|-----------|--------|
| `build-wsl-ninja-release` | `wsl-ninja-release` | Ninja | GCC g++/gcc | Release |
| `build-wsl-ninja-debug` | `wsl-ninja-debug` | Ninja | GCC g++/gcc | Debug |

**Usage from Windows (WSL integration):**
```bash
wsl bash -lc "cd /mnt/c/VCC/themis && cmake --build --preset wsl-ninja-release"
```

**Usage from WSL:**
```bash
cd /mnt/c/VCC/themis
cmake --preset wsl-ninja-release
cmake --build --preset wsl-ninja-release
```

---

### Native Linux (GCC + Ninja)

| Directory | CMake Preset | Generator | Toolchain | Config |
|-----------|-------------|-----------|-----------|--------|
| `build-linux-ninja-release` | `linux-ninja-release` | Ninja | GCC g++/gcc | Release |
| `build-linux-ninja-debug` | `linux-ninja-debug` | Ninja | GCC g++/gcc | Debug |

**Usage:**
```bash
cmake --preset linux-ninja-release
cmake --build --preset linux-ninja-release
```

---

## Edition-Specific Builds

These directories reflect specific product editions:

| Directory | CMake Preset | Features |
|-----------|-------------|----------|
| `build-minimal` | `minimal` | No LLM, No GPU |
| `build-community` | `community` | LLM: ON, GPU: OFF |
| `build-enterprise` | `enterprise` | LLM: ON, GPU: ON |
| `build-hyperscaler` | `hyperscaler` | LLM: ON, GPU: ON, Max features |

---

## Docker Builds

Docker builds produce artifacts in container-specific directories:

| Context | Output (in container) |
|---------|----------------------|
| Dockerfile build | `/app/build-docker-release` |
| Via docker-compose | `.docker-build-*` (temporary contexts) |

---

## VS Code Configuration

### Default Preset (CMakeUserPresets.json)

```json
{
  "name": "vscode-windows-release",
  "inherits": "msvc-ninja-release",
  "displayName": "VS Code: Windows Release (Default)"
}
```

### Available VS Code Tasks

| Task Label | Preset | Purpose |
|-----------|--------|---------|
| CMake: Configure preset (msvc-ninja-release) | msvc-ninja-release | Configure Release build |
| CMake: Build preset (msvc-ninja-release) | msvc-ninja-release | Build Release (default) |
| CMake: Configure preset (msvc-ninja-debug) | msvc-ninja-debug | Configure Debug build |
| CMake: Build preset (msvc-ninja-debug) | msvc-ninja-debug | Build Debug configuration |
| CMake: Build preset (wsl-ninja-release) | wsl-ninja-release | Build in WSL via Ninja |
| CMake: Build preset (linux-ninja-release) | linux-ninja-release | Build on native Linux |

---

## Test Execution

After building, run tests from the build directory:

### Windows (MSVC + Ninja Release)
```bash
cd build-msvc-ninja-release
ctest --output-on-failure
```

### WSL/Linux
```bash
cd build-wsl-ninja-release
ctest --output-on-failure
```

---

## Benefits of This Schema

✅ **Consistency**: Same naming pattern across all platforms  
✅ **Clarity**: Directory name reveals generator, toolchain, and config  
✅ **Automation**: Scripts can parse naming to determine build properties  
✅ **IDE Integration**: VS Code presets use these standard names  
✅ **Documentation**: Self-documenting build structure  
✅ **Discoverability**: Easy to find correct build directory via `ls build-*`

---

## Migration Guide (From Old Schema)

| Old Directory | New Directory | Preset |
|---------------|---------------|--------|
| `build-msvc` | `build-msvc-ninja-release` | `msvc-ninja-release` |
| `build-msvc-ninja-release` | ✓ Already correct | `msvc-ninja-release` |
| `build-ninja-llm-gpu` | `build-msvc-ninja-release` | `msvc-ninja-release` |
| `build-wsl` | `build-wsl-ninja-release` | `wsl-ninja-release` |
| `build-linux` | `build-linux-ninja-release` | `linux-ninja-release` |
| `build-community` | ✓ Unchanged (edition) | `community` |

**Cleanup old directories:**
```bash
Remove-Item build-msvc -Recurse -Force
Remove-Item build-ninja-llm-gpu -Recurse -Force
Remove-Item build-ninja-full -Recurse -Force
```

---

## Future Extensions

Possible additional variants (future use):

```
build-msvc-ninja-release-asan      # ASAN-enabled
build-msvc-ninja-release-ubsan     # UBSan-enabled
build-msvc-ninja-release-coverage  # Coverage builds
build-wsl-ninja-release-arm64      # ARM64 targeting
```

---

## References

- **CMakePresets.json**: Defines all preset configurations
- **CMakeUserPresets.json**: VS Code user overrides
- **.vscode/tasks.json**: Build automation tasks
- **CMakeLists.txt**: PRIMARY source of truth for build logic

---

## Questions / Updates

For schema changes or questions, update this file and notify the team.
