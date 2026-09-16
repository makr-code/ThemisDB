# base module tests

# Wave D: Soak test — registered separately with extended timeout and soak label
# so it is excluded from the standard test run but accessible via `ctest -L soak`.
set(_soak_src    "${CMAKE_CURRENT_SOURCE_DIR}/test_base_soak.cpp")
set(_soak_target "module_base_test_base_soak_focused")
set(_soak_ctest  "test_base_soak_base_FocusedTests")

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
    MODULE base
    NAME ${_soak_ctest}
    TARGET ${_soak_target}
    TIER soak
    TIMEOUT 7200
    LABELS "base;soak"
)

file(GLOB BASE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS BASE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_base_${_stem}_focused")
    set(_ctest "${_stem}_base_FocusedTests")

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
        MODULE base
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS base
    )
endforeach()