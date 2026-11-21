# numa module tests

file(GLOB NUMA_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS NUMA_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_numa_${_stem}_focused")
    set(_ctest "${_stem}_numa_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if(_stem STREQUAL "test_numa_memory_manager")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/performance/numa_memory_manager.cpp
        )
    endif()
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    if(_stem STREQUAL "test_numa_memory_manager")
        add_test(NAME NUMAMemoryManagerFocusedTests
            COMMAND $<TARGET_FILE:${_target}>)
        set_tests_properties(NUMAMemoryManagerFocusedTests PROPERTIES
            LABELS "performance;numa;memory;allocation;migration;stats;v1.9.0;focused;unit"
            TIMEOUT 60
        )
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
        )
    endif()

    themis_register_module_focused_test(
        MODULE numa
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS numa
    )
endforeach()
