# Auto-generated module tests for prefix 'diff'

file(GLOB DIFF_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_diff_*.cpp"
)

foreach(_src IN LISTS DIFF_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_diff_${_stem}_focused")
    set(_ctest "${_stem}_diff_FocusedTests")

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

    themis_register_module_focused_test(
        MODULE diff
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS diff
    )
endforeach()