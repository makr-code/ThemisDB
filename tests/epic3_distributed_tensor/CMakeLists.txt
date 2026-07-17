# EPIC 3 Distributed Tensor contract tests

set(EPIC3_DISTRIBUTED_TENSOR_TESTS
    distributed_planner_test.cc
)

foreach(test_src IN LISTS EPIC3_DISTRIBUTED_TENSOR_TESTS)
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} 
        PRIVATE themis_distributed_tensor
        PRIVATE themis_tensor
        PRIVATE themis_rag
        PRIVATE GTest::gtest_main
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
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

<<<<<<< HEAD
# ── TensorDeltaLog tests (sub-issue #5472, TDL-01..18) ──────────────────────

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_tensor_delta_log.cpp")
    message(STATUS "Adding EPIC5518: TensorDeltaLog tests (TDL-01..18)")

=======
# ── Phase B delta-log and rebuild-fallback tests ──────────────────────────────

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_tensor_delta_log.cpp")
>>>>>>> origin/develop
    add_executable(test_tensor_delta_log
        test_tensor_delta_log.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/tensor_delta_log.cc
    )

    target_include_directories(test_tensor_delta_log PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
<<<<<<< HEAD
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        ${THEMIS_ROOT_DIR}/src
=======
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
>>>>>>> origin/develop
    )

    target_link_libraries(test_tensor_delta_log PRIVATE
        ${TEST_LIBS}
<<<<<<< HEAD
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_tensor_delta_log PRIVATE
        THEMIS_TEST_BUILD=1
    )

    target_compile_features(test_tensor_delta_log PRIVATE cxx_std_17)
=======
        Threads::Threads
    )

    target_compile_definitions(test_tensor_delta_log PRIVATE THEMIS_TEST_BUILD=1)
>>>>>>> origin/develop

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE distributed_tensor
<<<<<<< HEAD
            NAME TensorDeltaLogTests
            TARGET test_tensor_delta_log
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor tensor_delta_log dynamic_update hybrid_retrieval
        )
    else()
        add_test(NAME TensorDeltaLogTests COMMAND test_tensor_delta_log)
    endif()

    message(STATUS "  TDL: append, extractWindow, serialization, GC, stats (TDL-01..18)")
else()
    message(STATUS "EPIC5518 TensorDeltaLog tests not found")
endif()

# ── SnapshotBasedUpdateWorker tests (sub-issue #5472, TUW-01..15) ────────────

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_tensor_update_worker.cpp")
    message(STATUS "Adding EPIC5518: SnapshotBasedUpdateWorker tests (TUW-01..15)")

    add_executable(test_tensor_update_worker
        test_tensor_update_worker.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/snapshot_update_worker.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/tensor_delta_log.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/stale_artifact_detector.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/error_recovery_handler.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/crash_recovery_checkpoint.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/distributed_lock_manager.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/artifact_invalidation.cc
    )

    target_include_directories(test_tensor_update_worker PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_tensor_update_worker PRIVATE
        ${TEST_LIBS}
        nlohmann_json::nlohmann_json
        Threads::Threads
    )

    target_compile_definitions(test_tensor_update_worker PRIVATE
        THEMIS_TEST_BUILD=1
    )

    target_compile_features(test_tensor_update_worker PRIVATE cxx_std_17)
=======
            NAME test_tensor_delta_log
            TARGET test_tensor_delta_log
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor phase-b delta-log
        )
    else()
        add_test(NAME test_tensor_delta_log COMMAND test_tensor_delta_log)
    endif()
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_tensor_rebuild_fallback.cpp")
    add_executable(test_tensor_rebuild_fallback
        test_tensor_rebuild_fallback.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/manifest_store.cpp
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/tensor_delta_log.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/snapshot_update_worker.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/crash_recovery_checkpoint.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/distributed_lock_manager.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/stale_artifact_detector.cc
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/src/error_recovery_handler.cc
    )

    target_include_directories(test_tensor_rebuild_fallback PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/src/distributed_tensor/include
    )

    target_link_libraries(test_tensor_rebuild_fallback PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )

    target_compile_definitions(test_tensor_rebuild_fallback PRIVATE THEMIS_TEST_BUILD=1)
>>>>>>> origin/develop

    if(COMMAND themis_register_module_focused_test)
        themis_register_module_focused_test(
            MODULE distributed_tensor
<<<<<<< HEAD
            NAME TensorUpdateWorkerTests
            TARGET test_tensor_update_worker
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor tensor_update_worker dynamic_update hybrid_retrieval
        )
    else()
        add_test(NAME TensorUpdateWorkerTests COMMAND test_tensor_update_worker)
    endif()

    message(STATUS "  TUW: lifecycle, decideStrategy, processTask, thresholds (TUW-01..15)")
else()
    message(STATUS "EPIC5518 TensorUpdateWorker tests not found")
=======
            NAME test_tensor_rebuild_fallback
            TARGET test_tensor_rebuild_fallback
            TIER unit
            TIMEOUT 60
            LABELS distributed_tensor phase-b rebuild-fallback
        )
    else()
        add_test(NAME test_tensor_rebuild_fallback COMMAND test_tensor_rebuild_fallback)
    endif()
>>>>>>> origin/develop
endif()

file(GLOB EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_manifest_store_phase_a\\.cpp$")
list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_tensor_delta_log\\.cpp$")
<<<<<<< HEAD
list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_tensor_update_worker\\.cpp$")
=======
list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_tensor_rebuild_fallback\\.cpp$")
>>>>>>> origin/develop

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
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_distributed_tensor
        Threads::Threads
    )
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
