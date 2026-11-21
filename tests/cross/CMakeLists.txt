# cross module tests

file(GLOB CROSS_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS CROSS_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_cross_${_stem}_focused")
    set(_ctest "${_stem}_cross_FocusedTests")

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
    if(_stem STREQUAL "test_cross_shard_coordinator")
        target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
        add_test(NAME PercolatorCoordinatorFocusedTests
            COMMAND $<TARGET_FILE:${_target}>
        )
        set_tests_properties(PercolatorCoordinatorFocusedTests PROPERTIES
            LABELS "sharding;percolator;distributed;transaction;truetime;orphan-detector;v1.7.0;unit"
            TIMEOUT 120
        )
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE cross
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS cross
    )
endforeach()