# secondary module tests

file(GLOB SECONDARY_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS SECONDARY_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_secondary_${_stem}_focused")
    set(_ctest "${_stem}_secondary_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
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
    if(_stem STREQUAL "test_secondary_index")
        target_link_libraries(${_target} PRIVATE Boost::system)
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE secondary
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS secondary
    )
    if(_stem STREQUAL "test_secondary_index")
        add_test(NAME themis_secidx_tests
            COMMAND $<TARGET_FILE:${_target}> --gtest_color=yes
        )
        set_tests_properties(themis_secidx_tests PROPERTIES
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
        )
    endif()
endforeach()
