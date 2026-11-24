# ── Distributed Planner contract tests (distributed_planner_test.cc) ────

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/distributed_planner_test.cc")
    message(STATUS "Adding EPIC3: Distributed Planner contract tests")

    add_executable(module_epic3_distributed_tensor_distributed_planner_test_focused
        distributed_planner_test.cc
    )

    target_include_directories(module_epic3_distributed_tensor_distributed_planner_test_focused PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        ${THEMIS_ROOT_DIR}/src
    )

    set(_epic3_link_libs themis_distributed_tensor)
    if(TARGET themis_tensor)
        list(APPEND _epic3_link_libs themis_tensor)
    endif()
    if(TARGET themis_rag)
        list(APPEND _epic3_link_libs themis_rag)
    endif()
    if(NOT TARGET themis_tensor)
        target_sources(module_epic3_distributed_tensor_distributed_planner_test_focused
            PRIVATE ${THEMIS_ROOT_DIR}/src/tensor/tensor_summary_types.cpp
        )
    endif()
    list(APPEND _epic3_link_libs ${TEST_LIBS} GTest::gtest_main Threads::Threads)
    target_link_libraries(module_epic3_distributed_tensor_distributed_planner_test_focused
        PRIVATE ${_epic3_link_libs}
    )

    target_compile_definitions(module_epic3_distributed_tensor_distributed_planner_test_focused
        PRIVATE THEMIS_TEST_BUILD=1
    )

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE epic3_distributed_tensor
            NAME DistributedPlannerContractTests
            TARGET module_epic3_distributed_tensor_distributed_planner_test_focused
            TIER unit
            TIMEOUT 120
            LABELS epic3 distributed_tensor distributed_planner contract
        )
    else()
        themis_register_test_target(
            NAME DistributedPlannerContractTests
            TARGET module_epic3_distributed_tensor_distributed_planner_test_focused
            TIMEOUT 120
            LABELS module:epic3_distributed_tensor tier:unit kind:focused epic3 distributed_tensor
        )
    endif()

    message(STATUS "  DistributedPlanner: artifact placement, shard assignment, planner contract")
else()
    message(STATUS "EPIC3 Distributed Planner tests not found at ${CMAKE_CURRENT_LIST_DIR}/distributed_planner_test.cc")
endif()

# EPIC 3 Distributed Tensor — tests and benchmarks

# ── Phase A ManifestStore tests (durability, recovery, MS-01..20) ─────────────

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_manifest_store_phase_a.cpp")
    message(STATUS "Adding EPIC3: Phase A ManifestStore tests (MS-01..20)")

    add_executable(test_manifest_store_phase_a
        test_manifest_store_phase_a.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/manifest_store.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/artifact_manifest.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/tensor_artifact_classes.cc
    )

    target_include_directories(test_manifest_store_phase_a PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_manifest_store_phase_a PRIVATE
        GTest::gtest_main
        themis_storage
        themis_base
        Threads::Threads
    )

    target_compile_definitions(test_manifest_store_phase_a PRIVATE
        THEMIS_TEST_BUILD=1
    )

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE epic3_distributed_tensor
            NAME ManifestStorePhaseATests
            TARGET test_manifest_store_phase_a
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor manifest phase-a advisory-only freshness
        )
    else()
        themis_register_test_target(
            NAME ManifestStorePhaseATests
            TARGET test_manifest_store_phase_a
            TIMEOUT 120
            LABELS module:epic3_distributed_tensor tier:unit kind:focused epic3 distributed_tensor
        )
    endif()

    message(STATUS "  ManifestStore: persistence/restart, corruption, freshness, metrics, concurrency (MS-01..20)")
else()
    message(STATUS "EPIC3 ManifestStore tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_manifest_store_phase_a.cpp")
endif()

# ── Lifecycle & Staleness Management tests (issue #5442, LSM-01..31) ──────────

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_lifecycle_staleness_management.cpp")
    message(STATUS "Adding EPIC3: Lifecycle & Staleness Management tests (LSM-01..31)")

    add_executable(test_lifecycle_staleness_management
        test_lifecycle_staleness_management.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/artifact_manifest.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/artifact_invalidation.cc
    )

    target_include_directories(test_lifecycle_staleness_management PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_lifecycle_staleness_management PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_lifecycle_staleness_management PRIVATE
        THEMIS_TEST_BUILD=1
    )

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE epic3_distributed_tensor
            NAME LifecycleStalenessManagementTests
            TARGET test_lifecycle_staleness_management
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor lifecycle staleness advisory-only
        )
    else()
        themis_register_test_target(
            NAME LifecycleStalenessManagementTests
            TARGET test_lifecycle_staleness_management
            TIMEOUT 120
            LABELS module:epic3_distributed_tensor tier:unit kind:focused epic3 distributed_tensor
        )
    endif()

    message(STATUS "  Lifecycle: state machine, staleness, invalidation, advisory-only (LSM-01..31)")
else()
    message(STATUS "EPIC3 Lifecycle tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_lifecycle_staleness_management.cpp")
endif()

# ── Phase A Tensor Delta Log tests (TDL-01..18) ────────────────────────────

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_tensor_delta_log.cpp")
    message(STATUS "Adding EPIC3: Phase A Tensor Delta Log tests (TDL-01..18)")

    add_executable(test_tensor_delta_log
        test_tensor_delta_log.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/tensor_delta_log.cc
    )

    target_include_directories(test_tensor_delta_log PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_tensor_delta_log PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_tensor_delta_log PRIVATE
        THEMIS_TEST_BUILD=1
    )

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE epic3_distributed_tensor
            NAME TensorDeltaLogPhaseATests
            TARGET test_tensor_delta_log
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor tensor_delta_log phase-a phase-b
        )
    else()
        themis_register_test_target(
            NAME TensorDeltaLogPhaseATests
            TARGET test_tensor_delta_log
            TIMEOUT 120
            LABELS module:epic3_distributed_tensor tier:unit kind:focused epic3 distributed_tensor
        )
    endif()

    message(STATUS "  TensorDeltaLog: append, window extraction, serialization, GC, retention (TDL-01..18)")
else()
    message(STATUS "EPIC3 TensorDeltaLog tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_tensor_delta_log.cpp")
endif()

# ── Phase B Tensor Rebuild Fallback tests (RFB-01..10) ──────────────────────

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_tensor_rebuild_fallback.cpp")
    message(STATUS "Adding EPIC3: Phase B Tensor Rebuild Fallback tests (RFB-01..10)")

    add_executable(test_tensor_rebuild_fallback
        test_tensor_rebuild_fallback.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/snapshot_update_worker.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/tensor_delta_log.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/crash_recovery_checkpoint.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/manifest_store.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/artifact_manifest.cc
    )

    target_include_directories(test_tensor_rebuild_fallback PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_tensor_rebuild_fallback PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_tensor_rebuild_fallback PRIVATE
        THEMIS_TEST_BUILD=1
    )

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE epic3_distributed_tensor
            NAME TensorRebuildFallbackPhaseBTests
            TARGET test_tensor_rebuild_fallback
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor tensor_update snapshot_worker phase-b
        )
    else()
        themis_register_test_target(
            NAME TensorRebuildFallbackPhaseBTests
            TARGET test_tensor_rebuild_fallback
            TIMEOUT 120
            LABELS module:epic3_distributed_tensor tier:unit kind:focused epic3 distributed_tensor
        )
    endif()

    message(STATUS "  SnapshotUpdateWorker: patch/refit/rebuild decisions, fallback, metrics (RFB-01..10)")
else()
    message(STATUS "EPIC3 TensorRebuildFallback tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_tensor_rebuild_fallback.cpp")
endif()

# ── Phase C Tensor Shard Summary tests (TSS-01..41) ──────────────────────────────

message(STATUS "Adding EPIC3: Phase C Tensor Shard Summary tests (TSS-01..41)")

add_executable(module_epic3_distributed_tensor_tensor_shard_summary_focused
    test_tensor_shard_summary.cpp
)

target_include_directories(module_epic3_distributed_tensor_tensor_shard_summary_focused PRIVATE
    ${THEMIS_ROOT_DIR}
    ${THEMIS_ROOT_DIR}/include
    ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
    ${THEMIS_ROOT_DIR}/src
)

set(_shard_summary_link_libs ${TEST_LIBS} GTest::gtest_main Threads::Threads)
if(TARGET themis_distributed_tensor)
    list(APPEND _shard_summary_link_libs themis_distributed_tensor)
endif()
if(TARGET themis_tensor)
    list(APPEND _shard_summary_link_libs themis_tensor)
endif()
if(NOT TARGET themis_distributed_tensor)
    target_sources(module_epic3_distributed_tensor_tensor_shard_summary_focused
        PRIVATE
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/shard_summary_coordinator.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/manifest_store.cpp
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/artifact_manifest.cc
    )
endif()
if(NOT TARGET themis_tensor)
    target_sources(module_epic3_distributed_tensor_tensor_shard_summary_focused
        PRIVATE ${THEMIS_ROOT_DIR}/src/tensor/tensor_summary_types.cpp
    )
endif()

target_link_libraries(module_epic3_distributed_tensor_tensor_shard_summary_focused
    PRIVATE ${_shard_summary_link_libs}
)

target_compile_definitions(module_epic3_distributed_tensor_tensor_shard_summary_focused
    PRIVATE THEMIS_TEST_BUILD=1
)

if(COMMAND themis_register_module_focused_test)
    themis_register_module_focused_test(
        MODULE epic3_distributed_tensor
        NAME TensorShardSummaryPhaseCTests
        TARGET module_epic3_distributed_tensor_tensor_shard_summary_focused
        TIER unit
        TIMEOUT 120
        LABELS epic3 distributed_tensor phase-c shard-summary freshness-consensus exact-on-demand
    )
else()
    themis_register_test_target(
        NAME TensorShardSummaryPhaseCTests
        TARGET module_epic3_distributed_tensor_tensor_shard_summary_focused
        TIMEOUT 120
        LABELS module:epic3_distributed_tensor tier:unit kind:focused epic3 distributed_tensor
    )
endif()

message(STATUS "  Phase C Shard Summary: refresh, routing, consensus, exact fetch (TSS-01..41)")

file(GLOB EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_manifest_store_phase_a\\.cpp$")
list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_lifecycle_staleness_management\\.cpp$")
list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_tensor_delta_log\\.cpp$")
list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_tensor_rebuild_fallback\\.cpp$")
list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_phase_b_edge_cases\\.cpp$")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integrity_verification_test.cc"
   AND OpenSSL_FOUND)
    list(APPEND EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/integrity_verification_test.cc"
    )
endif()

foreach(_src IN LISTS EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_epic3_distributed_tensor_${_stem}_focused")
    set(_ctest "${_stem}_epic3_distributed_tensor_FocusedTests")

    if(_stem STREQUAL "test_tensor_shard_summary")
        add_custom_target(${_target}
            DEPENDS module_epic3_distributed_tensor_tensor_shard_summary_focused
        )
        themis_register_module_focused_test(
            MODULE epic3_distributed_tensor
            NAME ${_ctest}
            TARGET module_epic3_distributed_tensor_tensor_shard_summary_focused
            TIER unit
            TIMEOUT 120
            LABELS epic3 distributed_tensor
        )
        continue()
    endif()

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
    )
    if((CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
        CMAKE_CXX_COMPILER_ID MATCHES "Clang") AND
       NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # Keep the repository's version file from shadowing the standard <version> header.
        target_compile_options(${_target} PRIVATE "-iquote${THEMIS_ROOT_DIR}")
    else()
        target_include_directories(${_target} PRIVATE ${THEMIS_ROOT_DIR})
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_distributed_tensor
        Threads::Threads
    )
    # If the distributed tensor library is not available, add key source
    # fallbacks directly into the test target to avoid unresolved externals
    # during partial builds or when modular targets are disabled.
    if(NOT TARGET themis_distributed_tensor)
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/snapshot_update_worker.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/tensor_delta_log.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/artifact_manifest.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/manifest_store.cpp
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/crash_recovery_checkpoint.cc
        )
        if(OpenSSL_FOUND)
            target_sources(${_target} PRIVATE
                ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/integrity_verification.cc
            )
        endif()
    endif()
    if(_stem STREQUAL "integrity_verification_test")
        if(TARGET themis_distributed_tensor)
            target_sources(${_target} PRIVATE
                ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/integrity_verification.cc
            )
        endif()
        target_link_libraries(${_target} PRIVATE OpenSSL::Crypto)
    endif()
    if(TARGET themis_base)
        target_link_libraries(${_target} PRIVATE themis_base)
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)
    if(OpenSSL_FOUND)
        target_compile_definitions(${_target}
            PRIVATE THEMIS_TEST_HAS_OPENSSL_SHA256=1
        )
    endif()

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE epic3_distributed_tensor
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS epic3 distributed_tensor
        )
    else()
        themis_register_test_target(
            NAME ${_ctest}
            TARGET ${_target}
            TIMEOUT 120
            LABELS module:epic3_distributed_tensor tier:unit kind:focused epic3 distributed_tensor
        )
    endif()
endforeach()

# ── Integrity Verification Benchmark (IV-BENCH-01..08) ────────────────────────

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integrity_verification_bench.cc"
   AND DEFINED THEMIS_HAS_BENCHMARK
   AND THEMIS_HAS_BENCHMARK
   AND OpenSSL_FOUND)
    message(STATUS "Adding EPIC3: Integrity Verification Benchmark (IV-BENCH-01..08)")

    add_executable(module_epic3_distributed_tensor_integrity_verification_bench_focused
        integrity_verification_bench.cc
    )
    target_sources(module_epic3_distributed_tensor_integrity_verification_bench_focused
        PRIVATE
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/integrity_verification.cc
    )

    target_include_directories(module_epic3_distributed_tensor_integrity_verification_bench_focused PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
    )

    target_link_libraries(module_epic3_distributed_tensor_integrity_verification_bench_focused PRIVATE
        ${TEST_LIBS}
        themis_distributed_tensor
        OpenSSL::Crypto
        Threads::Threads
    )
    
    # Link benchmark library (the target is expected when THEMIS_HAS_BENCHMARK=1)
    target_link_libraries(module_epic3_distributed_tensor_integrity_verification_bench_focused PRIVATE
        benchmark::benchmark
    )

    if(TARGET themis_base)
        target_link_libraries(module_epic3_distributed_tensor_integrity_verification_bench_focused PRIVATE themis_base)
    endif()

    target_compile_definitions(module_epic3_distributed_tensor_integrity_verification_bench_focused PRIVATE THEMIS_TEST_BUILD=1)

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE epic3_distributed_tensor
            NAME IntegrityVerificationBenchmark
            TARGET module_epic3_distributed_tensor_integrity_verification_bench_focused
            TIER benchmark
            TIMEOUT 300
            LABELS epic3 distributed_tensor integrity verification benchmark
        )
    else()
        themis_register_test_target(
            NAME IntegrityVerificationBenchmark
            TARGET module_epic3_distributed_tensor_integrity_verification_bench_focused
            TIMEOUT 300
            LABELS module:epic3_distributed_tensor tier:benchmark kind:focused epic3 distributed_tensor integrity verification benchmark
        )
    endif()

    message(STATUS "  IntegrityVerificationBenchmark: SHA-256, Merkle proofs, receipt chains (IV-BENCH-01..08)")
else()
    message(STATUS "EPIC3 integrity benchmark unavailable (source, benchmark support, or OpenSSL Crypto missing)")
endif()

# ── Phase B Edge Case Tests (test_phase_b_edge_cases.cpp) ───────────────────

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_phase_b_edge_cases.cpp")
    message(STATUS "Adding EPIC3: Phase B Edge Case Tests")

    add_executable(module_epic3_distributed_tensor_phase_b_edge_cases_focused
        test_phase_b_edge_cases.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/artifact_manifest.cc
    )

    target_include_directories(module_epic3_distributed_tensor_phase_b_edge_cases_focused PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(module_epic3_distributed_tensor_phase_b_edge_cases_focused PRIVATE
        ${TEST_LIBS}
        themis_distributed_tensor
        GTest::gtest_main
        Threads::Threads
    )

    target_compile_definitions(module_epic3_distributed_tensor_phase_b_edge_cases_focused
        PRIVATE THEMIS_TEST_BUILD=1
    )

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE epic3_distributed_tensor
            NAME PhaseBEdgeCaseTests
            TARGET module_epic3_distributed_tensor_phase_b_edge_cases_focused
            TIER unit
            TIMEOUT 180
            LABELS epic3 distributed_tensor phase-b edge-cases overflow instability fallback
        )
    else()
        themis_register_test_target(
            NAME PhaseBEdgeCaseTests
            TARGET module_epic3_distributed_tensor_phase_b_edge_cases_focused
            TIMEOUT 180
            LABELS module:epic3_distributed_tensor tier:unit kind:focused epic3 distributed_tensor phase-b edge-cases
        )
    endif()

    message(STATUS "  PhaseBEdgeCaseTests: Delta window bounds, overflow detection, instability heuristics, state transitions (PBE-01..24)")
else()
    message(STATUS "EPIC3 Phase B Edge Case tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_phase_b_edge_cases.cpp")
endif()
