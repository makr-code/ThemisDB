# module module tests

file(GLOB MODULE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS MODULE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_module_${_stem}_focused")
    set(_ctest "${_stem}_module_FocusedTests")

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
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    if(_stem STREQUAL "test_module_sandbox_wasm_injection")
        target_link_libraries(${_target} PRIVATE ${CMAKE_DL_LIBS})
        if(TARGET themis_base)
            target_link_libraries(${_target} PRIVATE themis_base)
        endif()
        set_target_properties(${_target} PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
        )
        add_test(NAME ModuleSandboxWasmInjectionFocusedTests
            COMMAND $<TARGET_FILE:${_target}>)
        set_tests_properties(ModuleSandboxWasmInjectionFocusedTests PROPERTIES
            LABELS "base;sandbox;wasm;injection;unit;focused"
            TIMEOUT 60
        )
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
            CONFIGURATIONS Release Debug
        )
    endif()

    themis_register_module_focused_test(
        MODULE module
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS module
    )
endforeach()
