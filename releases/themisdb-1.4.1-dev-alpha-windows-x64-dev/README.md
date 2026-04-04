# ThemisDB 1.4.1-dev-alpha - Windows Development Release

## 📦 Package Contents

- **bin/**: Main executables and runtime DLLs
  - themis_server.exe - ThemisDB server
  - All required runtime dependencies (DLLs)

- **tests/**: Test suite
  - themis_tests.exe - Comprehensive test suite (488+ tests)
  - All test runtime dependencies

- **lib/**: Static libraries
  - themis_core.lib - Core library for development

- **config/**: Configuration files
  - Default configuration templates
  - Example configurations for various scenarios

- **docs/**: Documentation
  - README, CHANGELOG, License
  - Release notes and guides

## 🚀 Quick Start

### Running the Server

```powershell
cd bin
.\themis_server.exe --help
```

### Running Tests

```powershell
cd tests
.\themis_tests.exe
```

## 📋 System Requirements

- **OS**: Windows 10/11 or Windows Server 2019+
- **RAM**: 4 GB minimum, 8 GB recommended
- **Disk**: 2 GB free space
- **CPU**: x64 processor with AVX2 support

## 🔧 Build Information

- **Version**: 1.4.1-dev-alpha
- **Platform**: Windows x64
- **Build Type**: Release (with optimizations)
- **Compiler**: MSVC 19.44
- **Build System**: CMake + Ninja
- **Features**:
  - ✓ LLM Support (llama.cpp integration)
  - ✓ gRPC Support
  - ✓ HTTP Server
  - ✓ RocksDB Backend
  - ✓ Vector Search (HNSW)
  - ✓ AVX2 Optimizations
  - ✓ Performance Optimizations (RCU Index, LIRS Cache)
  - ✓ mimalloc Allocator

## 🐛 Known Issues

This is a **development release** intended for testing and development purposes.

For production use, please wait for a stable release.

## 📄 License

MIT License - See LICENSE file for details

## 🔗 Links

- GitHub: https://github.com/kyr0/themis
- Documentation: https://themisdb.org/docs
- Issue Tracker: https://github.com/kyr0/themis/issues

## 📝 What's New in v1.4.1-dev

See docs/RELEASE_NOTES.md for detailed release notes.

---
Generated: 20260129-110447
