# content module tests

file(GLOB CONTENT_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS CONTENT_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_content_${_stem}_focused")
    set(_ctest "${_stem}_content_FocusedTests")

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
        MODULE content
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS content
    )
endforeach()

# ---------------------------------------------------------------------------
# Wave D Soak Test — registered separately with extended timeout and
# not_release_critical label so it does not block the release gate.
# ---------------------------------------------------------------------------
set(_soak_src "${CMAKE_CURRENT_SOURCE_DIR}/test_content_soak.cpp")
set(_soak_target "module_content_test_content_soak_focused")
set(_soak_ctest "test_content_soak_content_FocusedTests")

if(EXISTS "${_soak_src}" AND NOT TARGET ${_soak_target})
    add_executable(${_soak_target} "${_soak_src}")
    target_include_directories(${_soak_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(${_soak_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_soak_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE content
        NAME ${_soak_ctest}
        TARGET ${_soak_target}
        TIER integration
        TIMEOUT 600
        LABELS "content;wave_d;soak;not_release_critical"
    )
endif()