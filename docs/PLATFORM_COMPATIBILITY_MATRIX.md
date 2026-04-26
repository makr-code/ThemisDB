# ThemisDB Platform Compatibility Matrix

This document tracks the compilation status of ThemisDB source files across different compilers and platforms.

**Last Updated**: 2026-04-06

## Legend

- ✅ Compiles without errors or warnings
- ⚠️ Compiles with warnings
- ❌ Compilation fails
- 🔄 In progress / Being fixed
- ⏸️ Not tested yet

## Compiler Versions

| Compiler | Version | Platform |
|----------|---------|----------|
| MSVC | 19.29+ (Visual Studio 2019+) | Windows |
| GCC | 11.0+ | Linux |
| Clang | 14.0+ | Linux, macOS |
| Apple Clang | 13.0+ | macOS |
| ARM GCC | 11.0+ | ARM64, ARMv7 |

## Core Modules

### Storage

| File | MSVC | GCC11 | Clang14 | Apple Clang | ARM64 | Issues | Priority |
|------|------|-------|---------|-------------|-------|--------|----------|
| src/storage/rocksdb_wrapper.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | - | High |
| src/storage/index_manager.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | - | High |
| src/storage/blob_storage.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | - | Medium |

### Query Engine

| File | MSVC | GCC11 | Clang14 | Apple Clang | ARM64 | Issues | Priority |
|------|------|-------|---------|-------------|-------|--------|----------|
| src/query/query_executor.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | - | High |
| src/query/optimizer.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | - | Medium |

### LLM Integration

| File | MSVC | GCC11 | Clang14 | Apple Clang | ARM64 | Issues | Priority |
|------|------|-------|---------|-------------|-------|--------|----------|
| src/llm/lora_framework.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | Template instantiation | High |
| src/llm/gpu_coordinator.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | CUDA/ROCm dependencies | High |
| src/llm/inference_engine.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | - | Medium |

### Network

| File | MSVC | GCC11 | Clang14 | Apple Clang | ARM64 | Issues | Priority |
|------|------|-------|---------|-------------|-------|--------|----------|
| src/network/http_server.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | Socket API differences | Medium |
| src/network/grpc_server.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | - | Medium |
| src/network/websocket_handler.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | - | Low |

### Security

| File | MSVC | GCC11 | Clang14 | Apple Clang | ARM64 | Issues | Priority |
|------|------|-------|---------|-------------|-------|--------|----------|
| src/security/encryption.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | Intrinsics (AES-NI) | High |
| src/security/rbac.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | - | Medium |
| src/security/pki_manager.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | OpenSSL linking | Medium |

### Sharding

| File | MSVC | GCC11 | Clang14 | Apple Clang | ARM64 | Issues | Priority |
|------|------|-------|---------|-------------|-------|--------|----------|
| src/sharding/shard_manager.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | - | High |
| src/sharding/consistent_hash.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | Hash function ABI | Medium |
| src/sharding/rebalancer.cpp | ⏸️ | ⏸️ | ⏸️ | ⏸️ | ⏸️ | - | Low |

## Known Platform-Specific Issues

### Windows (MSVC)

| Issue | Affected Files | Status | Solution |
|-------|----------------|--------|----------|
| `char8_t` compatibility | Multiple UTF-8 handling code | 🔄 | Add `/Zc:char8_t-` flag |
| DLL export bloat | All modules | 🔄 | Remove `WINDOWS_EXPORT_ALL_SYMBOLS`, add explicit exports |
| PDB file conflicts | Parallel builds | 🔄 | Use `/FS` flag |
| Unreachable code warnings | Error handling paths | ⏸️ | Review and suppress as needed |

### Linux (GCC/Clang)

| Issue | Affected Files | Status | Solution |
|-------|----------------|--------|----------|
| Undefined references | Linker phase | 🔄 | Fix link order, add `-Wl,--no-as-needed` |
| Position-independent code | Shared libraries | ⏸️ | Ensure `-fPIC` is used |
| Weak symbol conflicts | Multiple translation units | ⏸️ | Use inline linkage or anonymous namespaces |

### macOS (Apple Clang)

| Issue | Affected Files | Status | Solution |
|-------|----------------|--------|----------|
| Framework linking | System frameworks | ⏸️ | Use `find_library()` for frameworks |
| Symbol mangling | C++/Objective-C mix | ⏸️ | Use `extern "C"` where needed |
| Universal binary | ARM64 + x86_64 | ⏸️ | Configure CMAKE_OSX_ARCHITECTURES |

### ARM (Cross-compilation)

| Issue | Affected Files | Status | Solution |
|-------|----------------|--------|----------|
| Endianness | Serialization code | ⏸️ | Use portable byte order functions |
| ABI mismatches | Function pointers | ⏸️ | Ensure consistent calling conventions |
| NEON intrinsics | SIMD code | ⏸️ | Add ARM NEON equivalents or fallbacks |
| Unaligned access | Memory operations | ⏸️ | Use memcpy for unaligned data |

## Testing Instructions

To update this matrix:

1. **Run the diagnostic scanner**:
   ```bash
   python tools/compiler_diagnostics/diagnostic_scanner.py <compiler_log> --output errors.db
   ```

2. **Generate platform report**:
   ```bash
   python tools/compiler_diagnostics/issue_tracker.py report --output report.md
   ```

3. **Update this matrix** based on the findings

4. **Re-test** after fixes and update status

## Continuous Integration

See `.github/workflows/` for platform-specific CI configurations:
- `ci-windows-full.yml` - Windows MSVC builds
- `ci-linux-full.yml` - Linux GCC/Clang builds
- `ci-arm-cross.yml` - ARM cross-compilation
- `ci-sanitizers.yml` - Memory/UB sanitizers

## Contributing

When adding new code:

1. Test on at least 2 different compilers
2. Use export macros for public APIs
3. Guard platform-specific code with preprocessor directives
4. Add fallback implementations for intrinsics
5. Run the source audit tool before committing

See [CONTRIBUTING_PLATFORM_GUIDELINES.md](CONTRIBUTING_PLATFORM_GUIDELINES.md) for details.
