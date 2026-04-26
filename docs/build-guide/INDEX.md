# Building ThemisDB v1.4.0+ Documentation Index

## 📚 Documentation Structure

### Quick Start (New!)
- 🚀 **[QUICK_START.md](QUICK_START.md)** - 5-minute build guide
  - One-liner commands
  - Preset list
  - Verification steps

### Platform-Specific Guides
- 🪟 **[BUILD_WINDOWS.md](BUILD_WINDOWS.md)** - MSVC/Visual Studio 2022
  - Presets & commands
  - Feature flags (40+ options)
  - Troubleshooting
  - Performance tips

- 🐧 **[BUILD_LINUX.md](BUILD_LINUX.md)** - GCC/Clang on Ubuntu/Debian
  - GCC vs Clang
  - WSL2 integration
  - Docker build

- 🍎 **[BUILD_MACOS.md](BUILD_MACOS.md)** - macOS/Apple Silicon
  - Clang setup
  - Homebrew dependencies
  - M1/M2/M3 optimization

### CMake Architecture (Updated!)
- 🏗️ **[../architecture/CMAKE_MODULAR_ARCHITECTURE.md](../architecture/CMAKE_MODULAR_ARCHITECTURE.md)**
  - **TARGET** modular architecture (PLANNED)
  - Current state vs. future roadmap
  - Feature modules (PLANNED): cmake/Features/*.cmake
  - Target modules (PLANNED): cmake/Targets/*.cmake
  - Edition system (MINIMAL/COMMUNITY/ENTERPRISE/HYPERSCALER)

- 🗺️ **[../architecture/MODULAR_ARCHITECTURE_ROADMAP.md](../architecture/MODULAR_ARCHITECTURE_ROADMAP.md)** ⭐ NEW!
  - Implementation plan for modular architecture
  - Phase 1: Feature module extraction (2-3 weeks)
  - Phase 2: Target module extraction (1-2 weeks)
  - Timeline, risk assessment, and success metrics

- 🔧 **[../architecture/FEATURE_FLAGS_REFERENCE.md](../architecture/FEATURE_FLAGS_REFERENCE.md)**
  - ALL 40+ feature flags documented
  - Build impact (size, time, runtime)
  - Dependencies per feature
  - Configuration examples

- 📖 **[../architecture/MIGRATION_GUIDE_v13_v14.md](../architecture/MIGRATION_GUIDE_v13_v14.md)**
  - v1.3.x → v1.4.0+ migration
  - Backward compatibility (commands still work!)
  - Preset equivalents
  - CI/CD updates

### Advanced Topics
- 🐳 **[BUILD_DOCKER.md](BUILD_DOCKER.md)** - Docker multi-stage builds
- 🧪 **[TESTING.md](TESTING.md)** - Run test suites
- 📦 **[PACKAGING.md](PACKAGING.md)** - CPack distribution

---

## 🎯 Quick Navigation

### I want to...

**Build for the first time** → [QUICK_START.md](QUICK_START.md)

**Build on Windows** → [BUILD_WINDOWS.md](BUILD_WINDOWS.md)

**Build on Linux** → [BUILD_LINUX.md](BUILD_LINUX.md)

**Understand the CMake system** → [CMAKE_MODULAR_ARCHITECTURE.md](../architecture/CMAKE_MODULAR_ARCHITECTURE.md) (current state + roadmap)

**See CMake refactoring plan** → [MODULAR_ARCHITECTURE_ROADMAP.md](../architecture/MODULAR_ARCHITECTURE_ROADMAP.md) ⭐ NEW!

**Use advanced features (LLM, GPU)** → [FEATURE_FLAGS_REFERENCE.md](../architecture/FEATURE_FLAGS_REFERENCE.md)

**Migrate from v1.3.x** → [MIGRATION_GUIDE_v13_v14.md](../architecture/MIGRATION_GUIDE_v13_v14.md)

**Create Docker container** → [BUILD_DOCKER.md](BUILD_DOCKER.md)

**Run tests** → [TESTING.md](TESTING.md)

**Create distribution package** → [PACKAGING.md](PACKAGING.md)

---

## 🆕 What's New in v1.4.0

### CMake Status ✅ / ⚠️
- ✅ **Clean Orchestration** - Root CMakeLists.txt (241 lines)
- ✅ **Modular Dependencies** - cmake/Versions.cmake, CompilerOptions.cmake, Dependencies.cmake
- ⚠️ **Monolithic Build Logic** - cmake/CMakeLists.txt (3115 lines - needs refactoring)
- 📋 **Planned**: Feature modules (cmake/Features/) - See [MODULAR_ARCHITECTURE_ROADMAP.md](../architecture/MODULAR_ARCHITECTURE_ROADMAP.md)
- 📋 **Planned**: Target modules (cmake/Targets/)
- ✅ **Edition System** - MINIMAL/COMMUNITY/ENTERPRISE/HYPERSCALER
- ✅ **CMake Presets** - Simplified `cmake --preset` commands

### Key Improvements 🚀
- **Windows**: CMake configuration works correctly
- **Path Resolution**: CMAKE_SOURCE_DIR issue fixed
- **Speed**: Build time unchanged (~150s on 8-core Windows)
- **Features**: 40+ flags documented
- **Documentation**: Accurate state + roadmap for future

### Status 📊
- ✅ CMake configuration: **PASSING** (Windows, Linux, macOS)
- ⚠️ C++ compilation: Pre-existing source code issues (not CMake)
- 📝 Documentation: **COMPREHENSIVE** (this index)
- 🧪 Tests: **180+ test suites ready**
- 📦 Benchmarks: **72 benchmarks ready**

---

## 🚀 Quick Build Commands

### Windows (Fastest)
```powershell
cmake --preset windows-vs2022-release
cmake --build --preset windows-vs2022-release --parallel 8
```

### Linux (GCC)
```bash
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release --parallel 8
```

### Hyperscaler Edition (LLM + GPU)
```bash
cmake -S . -B build -DTHEMIS_EDITION=HYPERSCALER -DTHEMIS_ENABLE_CUDA=ON
cmake --build build --parallel 8
```

---

## 📋 Feature Summary

| Feature | Status | Size | Build Time |
|---------|--------|------|------------|
| Core Database | ✅ | 50 MB | 30s |
| Vector Search | ✅ | +30 MB | +10s |
| LLM Integration | ✅ | +150 MB | +60s |
| GPU Acceleration | ✅ | +200 MB | +40s |
| gRPC Sharding | ✅ | +50 MB | +15s |
| OpenTelemetry | ✅ | +30 MB | +10s |
| HTTP/2 | ✅ | +20 MB | +5s |
| WebSocket | ✅ | +10 MB | +3s |

---

## 🔍 Documentation Quality

- ✅ All guides bilingual (EN/DE where applicable)
- ✅ Code examples for every feature flag
- ✅ Troubleshooting sections
- ✅ Performance optimization tips
- ✅ Edge case handling
- ✅ Links to architecture docs

---

## 📞 Support

- **Quick Help**: [QUICK_START.md](QUICK_START.md)
- **Platform Issues**: Platform-specific guide (BUILD_WINDOWS.md, etc.)
- **CMake Issues**: [CMAKE_MODULAR_ARCHITECTURE.md](../architecture/CMAKE_MODULAR_ARCHITECTURE.md)
- **Feature Questions**: [FEATURE_FLAGS_REFERENCE.md](../architecture/FEATURE_FLAGS_REFERENCE.md)
- **Bug Report**: [GitHub Issues](https://github.com/makr-code/themis/issues)

---

**Last Updated**: 2026-04-06  
**Status**: ✅ Accurate Documentation (reflects current state + roadmap)
