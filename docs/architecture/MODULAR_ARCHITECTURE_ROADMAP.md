# CMake Modular Architecture - Implementation Roadmap

**Version**: 1.0  
**Date**: 2026-01-12  
**Status**: 📋 Planning Phase  
**Related**: [CMAKE_MODULAR_ARCHITECTURE.md](CMAKE_MODULAR_ARCHITECTURE.md)

---

## Executive Summary

This roadmap outlines the plan to refactor ThemisDB's CMake build system from a **monolithic 3115-line cmake/CMakeLists.txt** to a **modular, maintainable architecture** with feature-based modules and separated target definitions.

### Current State (v1.4.0)

| Component | Status | Lines |
|-----------|--------|-------|
| Root CMakeLists.txt | ✅ Clean orchestration | 241 lines |
| cmake/CMakeLists.txt | ❌ Monolithic | **3115 lines** |
| cmake/Features/ | ❌ Does not exist | - |
| cmake/Targets/ | ❌ Does not exist | - |
| cmake/Versions.cmake | ✅ Modular | ~50 lines |
| cmake/CompilerOptions.cmake | ✅ Modular | ~70 lines |
| cmake/Dependencies.cmake | ✅ Modular | ~250 lines |

### Target State (v1.5.0+)

| Component | Status | Lines (Target) |
|-----------|--------|----------------|
| Root CMakeLists.txt | ✅ Already done | 241 lines |
| cmake/CMakeLists.txt | 📋 To be refactored | **<500 lines** |
| cmake/Features/*.cmake | 📋 To be created | 5 modules (~100-200 lines each) |
| cmake/Targets/*.cmake | 📋 To be created | 4 modules (~100-300 lines each) |
| Supporting modules | ✅ Already done | Unchanged |

**Net Result:**
- Reduction from 3115 lines (monolithic) to ~500 lines (orchestration) + ~1500 lines (distributed across modules)
- **Improved Maintainability:** Each feature/target is self-contained
- **Easier Onboarding:** New developers can understand structure quickly
- **Reduced Merge Conflicts:** Changes isolated to specific modules

---

## Implementation Phases

### Phase 0: Documentation and Planning ✅ COMPLETE

**Duration:** 1-2 days  
**Status:** ✅ Complete (this document)

- [x] Analyze current build system structure
- [x] Update CMAKE_MODULAR_ARCHITECTURE.md to reflect reality
- [x] Create this roadmap document
- [x] Define acceptance criteria for each phase
- [x] Identify dependencies between phases

**Deliverables:**
- ✅ Updated CMAKE_MODULAR_ARCHITECTURE.md
- ✅ MODULAR_ARCHITECTURE_ROADMAP.md (this file)
- ✅ Updated DOCUMENTATION_INDEX.md

---

### Phase 1: Feature Module Extraction

**Duration:** 2-3 weeks (40-60 hours)  
**Status:** 📋 Not Started  
**Target:** v1.5.0 or v1.6.0  
**Depends On:** Phase 0

#### Objectives

1. Create `cmake/Features/` directory
2. Extract feature-specific source lists and dependencies
3. Reduce cmake/CMakeLists.txt from 3115 lines to ~1500 lines
4. Maintain 100% build compatibility (no functional changes)

#### Sub-Tasks

##### 1.1 Create Feature: LLM.cmake (3-5 days)

**Complexity:** HIGH - ~20 source files, multiple dependencies

Extract LLM-related sources:
- `src/llm/*.cpp` (llama_plugin.cpp, llama_engine.cpp, etc.)
- `src/llm/lora/*.cpp` (LoRA adapter code)
- `src/llm/kernel_fusion/*.cpp` (CUDA kernels)
- Dependencies: llama.cpp, CUDA (optional)

**Module Structure:**
```cmake
# cmake/Features/LLM.cmake
if(NOT THEMIS_ENABLE_LLM)
    return()
endif()

message(STATUS "[Feature] LLM enabled (llama.cpp integration)")

# Source files
list(APPEND THEMIS_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/llm/llama_plugin.cpp
    ${CMAKE_SOURCE_DIR}/src/llm/llama_engine.cpp
    # ... ~20 files
)

# Conditional CUDA kernel fusion
if(THEMIS_ENABLE_CUDA)
    list(APPEND THEMIS_CORE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/llm/kernel_fusion/attention_cuda.cu
        # ...
    )
endif()

# Compile definitions
add_compile_definitions(THEMIS_LLM_ENABLED=1)

# Link libraries (to be called after target creation)
# target_link_libraries(themis_core PUBLIC llama)
```

**Testing:**
- [ ] Build with THEMIS_ENABLE_LLM=ON
- [ ] Build with THEMIS_ENABLE_LLM=OFF
- [ ] Verify all ~20 LLM source files are included
- [ ] Run LLM-specific tests

##### 1.2 Create Feature: GPU.cmake (2-3 days)

**Complexity:** MEDIUM - GPU acceleration abstractions

Extract GPU-related sources:
- `src/acceleration/*.cpp` (gpu_backend.cpp, cuda_backend.cpp, hip_backend.cpp)
- `src/index/faiss_*.cpp` (FAISS integration)
- Dependencies: CUDA, HIP (optional), FAISS

**Module Structure:**
```cmake
# cmake/Features/GPU.cmake
if(NOT THEMIS_ENABLE_GPU)
    return()
endif()

message(STATUS "[Feature] GPU acceleration enabled")

list(APPEND THEMIS_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/acceleration/gpu_backend.cpp
    ${CMAKE_SOURCE_DIR}/src/index/faiss_wrapper.cpp
    # ...
)

if(THEMIS_ENABLE_CUDA)
    add_compile_definitions(THEMIS_CUDA_ENABLED=1)
    # target_link_libraries(themis_core PUBLIC CUDA::cudart)
endif()

if(THEMIS_ENABLE_HIP)
    add_compile_definitions(THEMIS_HIP_ENABLED=1)
    # target_link_libraries(themis_core PUBLIC hip::host)
endif()
```

**Testing:**
- [ ] Build with THEMIS_ENABLE_GPU=ON + CUDA
- [ ] Build with THEMIS_ENABLE_GPU=ON + HIP
- [ ] Build with THEMIS_ENABLE_GPU=OFF
- [ ] Verify FAISS integration works

##### 1.3 Create Feature: gRPC.cmake (1-2 days)

**Complexity:** LOW - Single service, proto generation

Extract gRPC-related sources:
- `src/server/wal_grpc_service.cpp`
- `proto/shard_rpc.proto` (protocol buffer)
- Dependencies: gRPC++, Protobuf

**Module Structure:**
```cmake
# cmake/Features/gRPC.cmake
if(NOT THEMIS_ENABLE_GRPC)
    return()
endif()

message(STATUS "[Feature] gRPC enabled (inter-shard communication)")

# Proto generation
set(PROTO_FILES ${CMAKE_SOURCE_DIR}/proto/shard_rpc.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_FILES})

list(APPEND THEMIS_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/server/wal_grpc_service.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

add_compile_definitions(THEMIS_GRPC_ENABLED=1)

# target_link_libraries(themis_core PUBLIC gRPC::grpc++)
```

**Testing:**
- [ ] Build with THEMIS_ENABLE_GRPC=ON
- [ ] Build with THEMIS_ENABLE_GRPC=OFF
- [ ] Verify proto generation works
- [ ] Run gRPC communication tests

##### 1.4 Create Feature: Protocols.cmake (3-4 days)

**Complexity:** MEDIUM-HIGH - Multiple protocols

Extract protocol handler sources:
- `src/server/http2_handler.cpp`
- `src/server/http3_handler.cpp`
- `src/server/websocket_handler.cpp`
- `src/server/mqtt_handler.cpp`
- `src/server/postgres_wire_handler.cpp`
- `src/server/mcp_handler.cpp`
- Dependencies: nghttp2, nghttp3, ngtcp2, libwebsockets, mosquitto, etc.

**Module Structure:**
```cmake
# cmake/Features/Protocols.cmake

# HTTP/2
if(THEMIS_ENABLE_HTTP2)
    message(STATUS "[Protocol] HTTP/2 enabled")
    list(APPEND THEMIS_CORE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/server/http2_handler.cpp
    )
    add_compile_definitions(THEMIS_HTTP2_ENABLED=1)
    # target_link_libraries(themis_core PUBLIC nghttp2::nghttp2)
endif()

# HTTP/3 (Experimental)
if(THEMIS_ENABLE_HTTP3)
    message(STATUS "[Protocol] HTTP/3 enabled (EXPERIMENTAL)")
    list(APPEND THEMIS_CORE_SOURCES
        ${CMAKE_SOURCE_DIR}/src/server/http3_handler.cpp
    )
    add_compile_definitions(THEMIS_HTTP3_ENABLED=1)
    # target_link_libraries(themis_core PUBLIC nghttp3::nghttp3 ngtcp2::ngtcp2)
endif()

# WebSocket
if(THEMIS_ENABLE_WEBSOCKET)
    # ...
endif()

# MQTT
if(THEMIS_ENABLE_MQTT)
    # ...
endif()

# PostgreSQL Wire Protocol
if(THEMIS_ENABLE_POSTGRES_WIRE)
    # ...
endif()

# Model Context Protocol (MCP)
if(THEMIS_ENABLE_MCP)
    # ...
endif()
```

**Testing:**
- [ ] Build with each protocol independently enabled/disabled
- [ ] Verify all protocol handlers are included
- [ ] Run protocol-specific tests

##### 1.5 Create Feature: Tracing.cmake (1 day)

**Complexity:** LOW - Observability integration

Extract tracing sources:
- `src/observability/*.cpp` (OpenTelemetry integration)
- Dependencies: opentelemetry-cpp

**Module Structure:**
```cmake
# cmake/Features/Tracing.cmake
if(NOT THEMIS_ENABLE_TRACING)
    return()
endif()

message(STATUS "[Feature] OpenTelemetry tracing enabled")

list(APPEND THEMIS_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/observability/telemetry_init.cpp
    ${CMAKE_SOURCE_DIR}/src/observability/metrics_exporter.cpp
    ${CMAKE_SOURCE_DIR}/src/observability/trace_context.cpp
)

add_compile_definitions(THEMIS_TRACING_ENABLED=1)

# target_link_libraries(themis_core PUBLIC opentelemetry-cpp::api opentelemetry-cpp::sdk)
```

**Testing:**
- [ ] Build with THEMIS_ENABLE_TRACING=ON
- [ ] Build with THEMIS_ENABLE_TRACING=OFF
- [ ] Verify OpenTelemetry integration

##### 1.6 Integrate Feature Modules into cmake/CMakeLists.txt (2 days)

**Complexity:** LOW - Integration work

1. Add includes for all feature modules:
```cmake
# cmake/CMakeLists.txt (after source definitions)
include(${CMAKE_SOURCE_DIR}/cmake/Features/LLM.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Features/GPU.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Features/gRPC.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Features/Protocols.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Features/Tracing.cmake)
```

2. Remove extracted source lists from monolithic file
3. Remove extracted conditional blocks
4. Verify line count reduction (3115 → ~1500)

**Testing:**
- [ ] Build all edition configurations
- [ ] Build with all feature combinations
- [ ] Verify no regressions in Windows/Linux/macOS builds
- [ ] Run full test suite

#### Phase 1 Acceptance Criteria

- [ ] cmake/Features/ directory exists with 5 modules
- [ ] cmake/Features/LLM.cmake contains all LLM sources
- [ ] cmake/Features/GPU.cmake contains all GPU sources
- [ ] cmake/Features/gRPC.cmake contains all gRPC sources
- [ ] cmake/Features/Protocols.cmake contains all protocol handlers
- [ ] cmake/Features/Tracing.cmake contains all tracing sources
- [ ] cmake/CMakeLists.txt reduced from 3115 to ~1500 lines
- [ ] All builds pass (Windows MSVC, Linux GCC, macOS Clang)
- [ ] All tests pass (180+ test executables)
- [ ] All benchmarks build successfully (72 benchmarks)
- [ ] Documentation updated to reflect new structure
- [ ] No functional changes to build output

#### Phase 1 Deliverables

- [ ] 5 new cmake/Features/*.cmake modules
- [ ] Updated cmake/CMakeLists.txt (~1500 lines, down from 3115)
- [ ] Updated CMAKE_MODULAR_ARCHITECTURE.md (mark Phase 1 as complete)
- [ ] Migration guide for developers
- [ ] CI/CD pipeline validation

---

### Phase 2: Target Module Extraction

**Duration:** 1-2 weeks (20-30 hours)  
**Status:** 📋 Not Started  
**Target:** v1.5.0 or v1.6.0  
**Depends On:** Phase 1

#### Objectives

1. Create `cmake/Targets/` directory
2. Extract target definitions (libraries, executables, tests, benchmarks)
3. Reduce cmake/CMakeLists.txt from ~1500 lines to <500 lines
4. Maintain 100% build compatibility

#### Sub-Tasks

##### 2.1 Create Target: CoreLibrary.cmake (3 days)

**Complexity:** MEDIUM - Core library definition

Extract:
- `add_library(themis_core STATIC ...)` definition
- `target_include_directories(themis_core ...)`
- `target_link_libraries(themis_core ...)`
- `target_compile_definitions(themis_core ...)`

**Module Structure:**
```cmake
# cmake/Targets/CoreLibrary.cmake

# Core library (all THEMIS_CORE_SOURCES already populated by Feature modules)
add_library(themis_core STATIC ${THEMIS_CORE_SOURCES})

target_include_directories(themis_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)

# Link dependencies
target_link_libraries(themis_core PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
    RocksDB::rocksdb
    gRPC::grpc++
    protobuf::libprotobuf
    # ... all required dependencies
)

# Link optional dependencies (from Feature modules)
if(THEMIS_ENABLE_LLM)
    target_link_libraries(themis_core PUBLIC llama)
endif()

if(THEMIS_ENABLE_GPU AND THEMIS_ENABLE_CUDA)
    target_link_libraries(themis_core PUBLIC CUDA::cudart)
endif()

# ... similar for other features
```

**Testing:**
- [ ] Verify themis_core builds correctly
- [ ] Verify all dependencies link properly

##### 2.2 Create Target: Executables.cmake (2 days)

**Complexity:** LOW - Server and demo executables

Extract:
- `add_executable(themis_server ...)` definition
- `add_executable(themis_demo ...)` definition
- Executable-specific link libraries and properties

**Module Structure:**
```cmake
# cmake/Targets/Executables.cmake

# Server executable
add_executable(themis_server
    ${CMAKE_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(themis_server PRIVATE
    themis_core
)

set_target_properties(themis_server PROPERTIES
    OUTPUT_NAME "themis-server"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Demo executable
add_executable(themis_demo
    ${CMAKE_SOURCE_DIR}/examples/demo_main.cpp
)

target_link_libraries(themis_demo PRIVATE
    themis_core
)

set_target_properties(themis_demo PROPERTIES
    OUTPUT_NAME "themis-demo"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
```

**Testing:**
- [ ] Verify themis_server builds and runs
- [ ] Verify themis_demo builds and runs

##### 2.3 Create Target: Tests.cmake (3-4 days)

**Complexity:** HIGH - 180+ test executables

Extract:
- All `add_executable(test_* ...)` definitions (~180 tests)
- `target_link_libraries(test_* ...)` for each test
- `add_test(...)` CTest registrations

**Module Structure:**
```cmake
# cmake/Targets/Tests.cmake

if(NOT THEMIS_BUILD_TESTS)
    return()
endif()

message(STATUS "[Targets] Building tests (180+ executables)")

# Utility function for test creation
function(add_themis_test TEST_NAME SOURCE_FILE)
    add_executable(${TEST_NAME} ${CMAKE_SOURCE_DIR}/${SOURCE_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE
        themis_core
        GTest::gtest
        GTest::gtest_main
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Core tests
add_themis_test(test_storage tests/storage/test_rocksdb_wrapper.cpp)
add_themis_test(test_index tests/index/test_vector_index.cpp)
# ... ~180 more tests
```

**Testing:**
- [ ] Verify all 180+ tests build
- [ ] Run full test suite: `ctest --output-on-failure`
- [ ] Verify no test regressions

##### 2.4 Create Target: Benchmarks.cmake (2 days)

**Complexity:** MEDIUM - 72 benchmark executables

Extract:
- All `add_executable(bench_* ...)` definitions (~72 benchmarks)
- Benchmark-specific dependencies

**Module Structure:**
```cmake
# cmake/Targets/Benchmarks.cmake

if(NOT THEMIS_BUILD_BENCHMARKS)
    return()
endif()

message(STATUS "[Targets] Building benchmarks (72 executables)")

function(add_themis_benchmark BENCH_NAME SOURCE_FILE)
    add_executable(${BENCH_NAME} ${CMAKE_SOURCE_DIR}/${SOURCE_FILE})
    target_link_libraries(${BENCH_NAME} PRIVATE
        themis_core
        benchmark::benchmark
        benchmark::benchmark_main
    )
endfunction()

# Performance benchmarks
add_themis_benchmark(bench_vector_search benchmarks/bench_vector_search.cpp)
add_themis_benchmark(bench_aql_parser benchmarks/bench_aql_parser.cpp)
# ... ~70 more benchmarks
```

**Testing:**
- [ ] Verify all 72 benchmarks build
- [ ] Smoke test: run key benchmarks

##### 2.5 Integrate Target Modules (1 day)

**Complexity:** LOW - Integration work

Update cmake/CMakeLists.txt:
```cmake
# cmake/CMakeLists.txt

# Include feature modules (Phase 1)
include(${CMAKE_SOURCE_DIR}/cmake/Features/LLM.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Features/GPU.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Features/gRPC.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Features/Protocols.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Features/Tracing.cmake)

# Include target modules (Phase 2)
include(${CMAKE_SOURCE_DIR}/cmake/Targets/CoreLibrary.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Targets/Executables.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Targets/Tests.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Targets/Benchmarks.cmake)

# CPack configuration (if any remains)
# ...
```

Remove all extracted target definitions.

**Testing:**
- [ ] Build all targets
- [ ] Verify line count: cmake/CMakeLists.txt < 500 lines
- [ ] Full CI/CD pipeline validation

#### Phase 2 Acceptance Criteria

- [ ] cmake/Targets/ directory exists with 4 modules
- [ ] cmake/Targets/CoreLibrary.cmake contains themis_core definition
- [ ] cmake/Targets/Executables.cmake contains server/demo executables
- [ ] cmake/Targets/Tests.cmake contains all 180+ test definitions
- [ ] cmake/Targets/Benchmarks.cmake contains all 72 benchmark definitions
- [ ] cmake/CMakeLists.txt reduced to <500 lines
- [ ] All builds pass (Windows, Linux, macOS)
- [ ] All tests pass
- [ ] All benchmarks build
- [ ] Documentation updated
- [ ] No functional changes

#### Phase 2 Deliverables

- [ ] 4 new cmake/Targets/*.cmake modules
- [ ] Updated cmake/CMakeLists.txt (<500 lines, down from ~1500)
- [ ] Updated CMAKE_MODULAR_ARCHITECTURE.md (mark Phase 2 as complete)
- [ ] Updated MERGE_STRATEGY documentation (if needed)
- [ ] CI/CD pipeline validation

---

### Phase 3: Validation and Documentation (Optional)

**Duration:** 3-5 days  
**Status:** 📋 Not Started  
**Target:** v1.5.0  
**Depends On:** Phase 1 & 2

#### Objectives

1. External validation of new architecture
2. Complete documentation overhaul
3. Create migration examples for contributors

#### Sub-Tasks

- [ ] External contributor tests new architecture
- [ ] Create "Adding a New Feature" tutorial
- [ ] Create "Adding a New Test" tutorial
- [ ] Update all build guides to reference new structure
- [ ] Record video walkthrough (optional)
- [ ] Blog post announcement (optional)

#### Acceptance Criteria

- [ ] At least one external contributor successfully adds a feature using new architecture
- [ ] Tutorial documentation created
- [ ] All references to old monolithic structure updated
- [ ] Architecture validated by 2+ reviewers

---

## Risk Assessment

### High Risk

| Risk | Impact | Mitigation |
|------|--------|------------|
| **Build breakage during refactoring** | Critical - Blocks development | Incremental changes, test after each sub-phase, use feature branches |
| **Merge conflicts with ongoing work** | High - Slows down development | Coordinate with team, freeze cmake/ during refactor, communicate timeline |
| **Incomplete extraction** | High - Missing source files | Automated verification scripts, compare before/after builds |

### Medium Risk

| Risk | Impact | Mitigation |
|------|--------|------------|
| **Performance regression** | Medium - Slower builds | Benchmark build times before/after, optimize includes |
| **Cross-platform issues** | Medium - Platform-specific failures | Test on Windows, Linux, macOS after each phase |
| **Documentation drift** | Medium - Confusion for developers | Update docs in same PR as code changes |

### Low Risk

| Risk | Impact | Mitigation |
|------|--------|------------|
| **Learning curve for contributors** | Low - Temporary slowdown | Provide tutorials, examples, and migration guide |
| **Tool compatibility** | Low - IDE/editor issues | Test with Visual Studio, CLion, VS Code |

---

## Success Metrics

### Quantitative

- [ ] cmake/CMakeLists.txt reduced from **3115 lines → <500 lines** (>80% reduction)
- [ ] Build time unchanged or improved (target: ≤150s on 8-core Windows)
- [ ] All 180+ tests pass (100% pass rate)
- [ ] Zero functional regressions
- [ ] Documentation gap score: 0 (all docs accurate)

### Qualitative

- [ ] Easier for new contributors to understand build system
- [ ] Faster feature development (add new feature in isolated module)
- [ ] Reduced merge conflicts in cmake/ directory
- [ ] Improved code review process (reviewers understand changes quickly)
- [ ] Positive feedback from development team

---

## Timeline

### Optimistic (Best Case)

| Phase | Duration | Calendar Time |
|-------|----------|---------------|
| Phase 0: Documentation | 2 days | ✅ Complete |
| Phase 1: Feature Modules | 2 weeks | Weeks 1-2 |
| Phase 2: Target Modules | 1 week | Week 3 |
| Phase 3: Validation | 3 days | Week 4 |
| **Total** | **~3.5 weeks** | **1 month** |

### Realistic (Expected)

| Phase | Duration | Calendar Time |
|-------|----------|---------------|
| Phase 0: Documentation | 2 days | ✅ Complete |
| Phase 1: Feature Modules | 3 weeks | Weeks 1-3 |
| Phase 2: Target Modules | 2 weeks | Weeks 4-5 |
| Phase 3: Validation | 5 days | Week 6 |
| **Total** | **~6 weeks** | **1.5 months** |

### Pessimistic (Worst Case)

| Phase | Duration | Calendar Time |
|-------|----------|---------------|
| Phase 0: Documentation | 2 days | ✅ Complete |
| Phase 1: Feature Modules | 4 weeks | Weeks 1-4 |
| Phase 2: Target Modules | 3 weeks | Weeks 5-7 |
| Phase 3: Validation | 1 week | Week 8 |
| **Total** | **~8 weeks** | **2 months** |

**Recommendation:** Plan for realistic timeline (6 weeks), allocate buffer for unexpected issues.

---

## Dependencies and Prerequisites

### Prerequisites (Must Have Before Starting)

- [x] Phase 0 complete (this document)
- [ ] Team agreement on architecture direction
- [ ] Dedicated time allocation (no interruptions during refactor)
- [ ] Feature freeze on cmake/ directory during refactoring
- [ ] Backup branch with current working state

### External Dependencies

- None - Refactoring uses existing tools and dependencies

### Team Dependencies

- Build system maintainer (lead)
- 1-2 reviewers for each phase
- CI/CD engineer for pipeline validation

---

## Communication Plan

### Before Starting

- [ ] Share roadmap with development team
- [ ] Create GitHub issue for tracking: "Refactor CMake to Modular Architecture"
- [ ] Announce feature freeze on cmake/ directory
- [ ] Set up dedicated Slack/Discord channel (optional)

### During Implementation

- [ ] Daily standup updates (if in sprint)
- [ ] Weekly progress updates in team meeting
- [ ] Immediate notification if blockers encountered
- [ ] PR reviews within 24 hours (critical path)

### After Completion

- [ ] Announcement: "New Modular CMake Architecture Live"
- [ ] Tutorial session for development team
- [ ] Update onboarding documentation
- [ ] Retrospective: What went well, what to improve

---

## Rollback Plan

If critical issues arise during refactoring:

1. **Immediate Rollback:**
   - Revert PR to last known good state
   - Resume work on backup branch

2. **Partial Rollback:**
   - Complete Phase 1, pause before Phase 2
   - Use Phase 1 architecture temporarily
   - Re-evaluate approach

3. **Full Rollback:**
   - Revert all changes to monolithic architecture
   - Document lessons learned
   - Revisit roadmap with team

**Decision Criteria for Rollback:**
- Build time increases by >20%
- >5% test failure rate
- Unresolvable cross-platform issues
- Team consensus that approach is not working

---

## Related Documentation

- [CMAKE_MODULAR_ARCHITECTURE.md](CMAKE_MODULAR_ARCHITECTURE.md) - Target architecture description
- [FEATURE_FLAGS_REFERENCE.md](FEATURE_FLAGS_REFERENCE.md) - Feature flag documentation
- [MIGRATION_GUIDE_v13_v14.md](MIGRATION_GUIDE_v13_v14.md) - Previous migration guide
- [DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md](../Audit/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md) - Gap analysis that identified this issue

---

## Tracking

**GitHub Issue:** TBD (to be created)  
**Project Board:** TBD  
**Milestone:** v1.5.0 or v1.6.0

**Status:** 📋 Planning Phase  
**Next Action:** Create GitHub issue and get team approval to proceed

---

**Document Maintainer:** ThemisDB Build System Team  
**Last Updated:** 2026-04-06  
**Next Review:** After Phase 1 completion
