# ThemisDB CMake Architecture

## Current Build System Structure (v1.3.x+)

```
┌─────────────────────────────────────────────────────────────┐
│                   CMakeLists.txt (ROOT)                      │
│                      (226 lines)                             │
│                                                              │
│  • Read VERSION file                                         │
│  • Define project(Themis VERSION ...)                       │
│  • Set build options (THEMIS_ENABLE_*)                      │
│  • Include modular cmake files                               │
│  • Documentation database generation                         │
│  • CPack configuration                                       │
└─────────────┬───────────────────────────────────────────────┘
              │
              │ include()
              ├──────────────────────────────────────┐
              │                                       │
              ▼                                       ▼
┌─────────────────────────┐         ┌──────────────────────────────┐
│ cmake/CompilerOptions.  │         │  cmake/Dependencies.cmake    │
│       cmake             │         │      (449 lines)             │
│    (85 lines)           │         │                              │
│                         │         │  • find_package() calls      │
│  • C++20 standard       │         │  • RocksDB, OpenSSL, gRPC    │
│  • MSVC/GCC flags       │         │  • simdjson, TBB, Arrow      │
│  • AVX2 optimization    │         │  • OpenTelemetry, etc.       │
│  • AddressSanitizer     │         │  • Target aliases            │
└─────────────────────────┘         └──────────────────────────────┘
              │                                       │
              │                                       │
              ▼                                       ▼
┌─────────────────────────┐         ┌──────────────────────────────┐
│  cmake/Versions.cmake   │         │  cmake/ModularBuild.cmake    │
│     (59 lines)          │         │      (261 lines)             │
│                         │         │                              │
│  • Parse VERSION        │         │  • Post-v1.3.0 feature      │
│  • Edition selection    │         │  • Modular architecture      │
│  • Feature defaults     │         │  • themis_add_module()       │
└─────────────────────────┘         └──────────────────────────────┘
              │
              │
              │ add_subdirectory(cmake)
              │
              ▼
┌─────────────────────────────────────────────────────────────┐
│             cmake/CMakeLists.txt (MAIN BUILD)               │
│                     (3115 lines)                            │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Section 1: Project Setup (100 lines)               │    │
│  │  • VERSION reading                                 │    │
│  │  • project() definition                            │    │
│  │  • Build options                                   │    │
│  │  • find_package() calls (duplicated)               │    │
│  └────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Section 2: License & Edition (200 lines)           │    │
│  │  • License data embedding                          │    │
│  │  • Edition feature configuration                   │    │
│  │  • MINIMAL/COMMUNITY/ENTERPRISE/HYPERSCALER        │    │
│  └────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Section 3: Compiler Configuration (200 lines)      │    │
│  │  • Architecture detection                          │    │
│  │  • MSVC/GCC compiler flags                         │    │
│  │  • Optimization flags                              │    │
│  └────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Section 4: llama.cpp Integration (100 lines)       │    │
│  │  • Find llama.cpp source                           │    │
│  │  • Configure build options                         │    │
│  │  • GPU acceleration setup                          │    │
│  └────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Section 5: Source File Lists (600 lines)           │    │
│  │  • THEMIS_CORE_SOURCES (~500 files)                │    │
│  │  • Conditional LLM sources                         │    │
│  │  • Performance optimization sources                │    │
│  └────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Section 6: Core Library Target (300 lines)         │    │
│  │  • add_library(themis_core)                        │    │
│  │  • target_link_libraries()                         │    │
│  │  • target_compile_definitions()                    │    │
│  └────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Section 7: Executable Targets (50 lines)           │    │
│  │  • themis_server                                   │    │
│  │  • themis_demo                                     │    │
│  │  • themis_demo_encryption                          │    │
│  └────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Section 8: Tests (1200 lines)                      │    │
│  │  • add_executable(themis_tests)                    │    │
│  │  • ~100 test source files                          │    │
│  │  • gtest_discover_tests()                          │    │
│  │  • LLM test targets                                │    │
│  │  • Phase 1/2/3 optimization tests                  │    │
│  └────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Section 9: Benchmarks (800 lines)                  │    │
│  │  • ~50 benchmark executables                       │    │
│  │  • Core performance benchmarks                     │    │
│  │  • Scientific benchmarks (TPC-C, YCSB, TPC-H)      │    │
│  │  • LLM benchmarks                                  │    │
│  │  • RAID benchmarks                                 │    │
│  └────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │ Section 10: Installation (50 lines)                │    │
│  │  • install(TARGETS ...)                            │    │
│  │  • install(DIRECTORY ...)                          │    │
│  └────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

## Factorization Status

### ✅ Successfully Factorized (Root Level)
- **CompilerOptions.cmake**: Compiler flags and standards
- **Dependencies.cmake**: All external package finding
- **Versions.cmake**: Version parsing and edition configuration
- **ModularBuild.cmake**: Future modular architecture framework

### ⚠️ Could Be Further Factorized (cmake/ Level)
The `cmake/CMakeLists.txt` could be split into:
- **cmake/Sources.cmake**: All source file lists
- **cmake/Targets.cmake**: Library and executable target definitions
- **cmake/Tests.cmake**: Test configuration
- **cmake/Benchmarks.cmake**: Benchmark configuration

## Design Philosophy

The current architecture follows a **two-tier approach**:

1. **Tier 1 (Root)**: Project-level orchestration
   - High-level configuration
   - Feature toggles
   - Build variant selection
   - Modular includes

2. **Tier 2 (cmake/)**: Implementation details
   - Source lists
   - Target definitions
   - Test/benchmark setup

This is a **valid and common CMake pattern**. Many large projects follow this structure.

## Comparison with Other Projects

### Similar Architectures
- **LLVM**: Root CMakeLists.txt orchestrates, subdirectories contain implementations
- **Boost**: Root handles configuration, libraries have their own CMakeLists.txt
- **RocksDB**: Minimal root, most logic in subdirectories

### Alternative Approaches
Some projects split everything into many small files:
- **Qt**: Extreme modularization with dedicated cmake/ modules
- **KDE**: Function-based approach with many helper files

## Recommendations

### Short-term (Current State) ✅
**Keep as-is** - The current architecture is functional and maintainable.

**Rationale**:
- Root CMakeLists.txt is already minimal (226 lines)
- Key concerns are properly separated
- Dependencies, compiler options, and versions are modularized
- The cmake/CMakeLists.txt being large is acceptable as it's the implementation

### Long-term (Optional Improvement)
If maintenance becomes difficult, consider:
1. Extract source lists to `cmake/Sources.cmake`
2. Extract test configuration to `cmake/Tests.cmake`
3. Extract benchmarks to `cmake/Benchmarks.cmake`

**Benefits**:
- Easier to find specific configurations
- Parallel editing by multiple developers
- Clearer separation of concerns

**Costs**:
- More files to maintain
- Need to pass variables between files
- Increased cognitive load for newcomers

## Conclusion

**The build system factorization is COMPLETE and FUNCTIONAL.**

The root CMakeLists.txt is properly minimal, and modular files handle key concerns. The cmake/CMakeLists.txt being large is by design and is a valid architecture choice.

**No immediate action required** - the build system is well-structured.
