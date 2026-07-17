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

file(GLOB EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

list(FILTER EPIC3_DISTRIBUTED_TENSOR_TEST_SOURCES EXCLUDE REGEX ".*/test_manifest_store_phase_a\\.cpp$")

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
