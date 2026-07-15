# Auto-generated module tests for prefix 'artifact'

file(GLOB ARTIFACT_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_artifact_*.cpp"
)

foreach(_src IN LISTS ARTIFACT_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_artifact_${_stem}_focused")
    set(_ctest "${_stem}_artifact_FocusedTests")
    set(_extra_include_dirs)

    if(TARGET ${_target})
        continue()
    endif()

    if(_stem STREQUAL "test_artifact_preflight")
        list(APPEND _extra_include_dirs
            ${THEMIS_ROOT_DIR}/benchmarks
        )
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${_extra_include_dirs}
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE artifact
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS artifact
    )
endforeach()