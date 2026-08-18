# Replication module test registration (pilot extraction from root tests/CMakeLists.txt)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_replication_ha.cpp")
    message(STATUS "Adding Replication HA focused tests")

    add_executable(test_replication_ha_focused
        ../test_replication_ha.cpp
    )

    target_include_directories(test_replication_ha_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_replication_ha_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_replication_ha_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE replication
        NAME ReplicationHAFocusedTests
        TARGET test_replication_ha_focused
        TIER unit
        TIMEOUT 120
        LABELS ha witness-node conflict-resolution
    )

    set_tests_properties(ReplicationHAFocusedTests PROPERTIES
        RUN_SERIAL TRUE
    )

    message(STATUS "  ReplicationHA: config validation, WAL checksum, LWW/CRDT, quorum leader election, witness nodes")
else()
    message(STATUS "Replication HA focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/../test_replication_ha.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_replication_chaos.cpp")
    message(STATUS "Adding Replication: Wave A Batch A-9 chaos tests")
    add_executable(module_replication_test_replication_chaos_focused
        test_replication_chaos.cpp
        ${THEMIS_ROOT_DIR}/tests/utils/fault_injector.cpp)
    target_include_directories(module_replication_test_replication_chaos_focused PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests)
    target_link_libraries(module_replication_test_replication_chaos_focused PRIVATE
        GTest::gtest GTest::gtest_main Threads::Threads)
    target_compile_definitions(module_replication_test_replication_chaos_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(
        MODULE replication
        NAME ReplicationChaosBatchA9FocusedTests
        TARGET module_replication_test_replication_chaos_focused
        TIER unit
        TIMEOUT 120
        LABELS replication chaos batch-a9)
endif()

# AUTOGEN PREFIX BLOCK: replication
file(GLOB REPLICATION_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_replication_*.cpp"
)

foreach(_src IN LISTS REPLICATION_AUTOGEN_PREFIX_TEST_SOURCES)
    if(_src MATCHES ".*/test_replication_chaos\\.cpp$")
        continue()
    endif()
    if(_src MATCHES ".*/test_replication_fail_closed_behavior\\.cpp$")
        continue()
    endif()

    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_replication_${_stem}_autofocused")
    set(_ctest "${_stem}_replication_AutoFocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    # Promote geo-placement, WAL shipping, chaos/fault, and fail-closed tests to release_critical.
    set(_extra_labels "replication;autogen")
    if(_stem MATCHES "geo_placement_wal_shipping|chaos_failover|fail_closed_behavior")
        set(_extra_labels "replication;autogen;release_critical")
    endif()

    themis_register_module_focused_test(
        MODULE replication
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS "${_extra_labels}"
    )
endforeach()

# ============================================================================
# Replication p95/p99 baseline benchmarks (Wave A gate evidence)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench_replication_geo_wal_baselines.cpp")
    message(STATUS "Adding Replication: geo-placement + WAL shipping p95/p99 benchmarks")
    add_executable(bench_replication_geo_wal_baselines
        bench_replication_geo_wal_baselines.cpp
    )
    target_include_directories(bench_replication_geo_wal_baselines PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(bench_replication_geo_wal_baselines PRIVATE
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(bench_replication_geo_wal_baselines PRIVATE
        THEMIS_TEST_BUILD=1
    )
    themis_register_module_focused_test(
        MODULE replication
        NAME ReplicationGeoWalBaselineBenchmarks
        TARGET bench_replication_geo_wal_baselines
        TIER benchmark
        TIMEOUT 300
        LABELS "replication;benchmark;geo;wal;p95_p99_baseline"
    )
    message(STATUS "  BENCH-GEO-01..04: selectLeaderCandidate / selectFailoverCandidate p95/p99 latency")
    message(STATUS "  BENCH-WAL-01..04: enqueueSegment throughput and lag p95/p99 under load")
endif()
