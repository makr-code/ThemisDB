# EPIC 3 Distributed Tensor contract tests

set(EPIC3_DISTRIBUTED_TENSOR_TESTS
    distributed_planner_test.cc
)

foreach(test_src IN LISTS EPIC3_DISTRIBUTED_TENSOR_TESTS)
    if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${test_src}")
        message(STATUS "EPIC3 distributed tensor test source not found at ${CMAKE_CURRENT_LIST_DIR}/${test_src}")
        continue()
    endif()

    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    # If the full `themis_tensor` library target is not available in this
    # configured build, compile the minimal implementation required by these
    # tests directly into the test executable to avoid unresolved externals.
    if(NOT TARGET themis_tensor)
        target_sources(${test_name} PRIVATE ${THEMIS_ROOT_DIR}/src/tensor/tensor_summary_types.cpp)
    endif()
    # Link only available module targets to avoid forcing missing .lib files
    set(_epic3_link_libs themis_distributed_tensor)
    if(TARGET themis_tensor)
        list(APPEND _epic3_link_libs themis_tensor)
    endif()
    if(TARGET themis_rag)
        list(APPEND _epic3_link_libs themis_rag)
    endif()
    list(APPEND _epic3_link_libs GTest::gtest_main)
    target_link_libraries(${test_name} PRIVATE ${_epic3_link_libs})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# EPIC 3 Distributed Tensor — tests and benchmarks

# ── Phase A ManifestStore tests (Phase A advisory-only store, MS-01..12) ──────

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_manifest_store_phase_a.cpp")
    message(STATUS "Adding EPIC3: Phase A ManifestStore tests (MS-01..12)")

    add_executable(test_manifest_store_phase_a
        test_manifest_store_phase_a.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/manifest_store.cpp
    )

    target_include_directories(test_manifest_store_phase_a PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_manifest_store_phase_a PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_manifest_store_phase_a PRIVATE
        THEMIS_TEST_BUILD=1
    )

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE distributed_tensor
            NAME ManifestStorePhaseATests
            TARGET test_manifest_store_phase_a
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor manifest phase-a advisory-only freshness
        )
    else()
        add_test(NAME ManifestStorePhaseATests COMMAND test_manifest_store_phase_a)
    endif()

    message(STATUS "  ManifestStore: store/get/list/evict, freshness, metrics, concurrency (MS-01..12)")
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
            MODULE distributed_tensor
            NAME LifecycleStalenessManagementTests
            TARGET test_lifecycle_staleness_management
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor lifecycle staleness advisory-only
        )
    else()
        add_test(NAME LifecycleStalenessManagementTests COMMAND test_lifecycle_staleness_management)
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
            MODULE distributed_tensor
            NAME TensorDeltaLogPhaseATests
            TARGET test_tensor_delta_log
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor tensor_delta_log phase-a phase-b
        )
    else()
        add_test(NAME TensorDeltaLogPhaseATests COMMAND test_tensor_delta_log)
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
            MODULE distributed_tensor
            NAME TensorRebuildFallbackPhaseBTests
            TARGET test_tensor_rebuild_fallback
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor tensor_update snapshot_worker phase-b
        )
    else()
        add_test(NAME TensorRebuildFallbackPhaseBTests COMMAND test_tensor_rebuild_fallback)
    endif()

    message(STATUS "  SnapshotUpdateWorker: patch/refit/rebuild decisions, fallback, metrics (RFB-01..10)")
else()
    message(STATUS "EPIC3 TensorRebuildFallback tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_tensor_rebuild_fallback.cpp")
endif()

file(GLOB EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_manifest_store_phase_a\\.cpp$")
list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_lifecycle_staleness_management\\.cpp$")
list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_tensor_delta_log\\.cpp$")
list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_tensor_rebuild_fallback\\.cpp$")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/integrity_verification_test.cc")
    list(APPEND EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/integrity_verification_test.cc"
    )
endif()

foreach(_src IN LISTS EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_epic3_distributed_tensor_${_stem}_focused")
    set(_ctest "${_stem}_epic3_distributed_tensor_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
    )
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
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/integrity_verification.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/tensor_delta_log.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/artifact_manifest.cc
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/manifest_store.cpp
            ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/crash_recovery_checkpoint.cc
        )
    endif()
    if(TARGET themis_base)
        target_link_libraries(${_target} PRIVATE themis_base)
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

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
        add_test(NAME ${_ctest} COMMAND ${_target})
    endif()
endforeach()
