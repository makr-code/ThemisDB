set(EPIC2_EVALUATION_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/hardware_profile_test.cc"
)

foreach(_src IN LISTS EPIC2_EVALUATION_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_epic2_evaluation_${_stem}_focused")
    set(_ctest "${_stem}_epic2_evaluation_FocusedTests")

    add_executable(${_target}
        "${_src}"
        "${THEMIS_ROOT_DIR}/src/evaluation/src/hardware_profile.cc"
    )
    target_include_directories(${_target} PRIVATE
        "${THEMIS_ROOT_DIR}/src/evaluation/include"
        "${THEMIS_ROOT_DIR}/src"
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE epic2_evaluation
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS evaluation epic2
    )
endforeach()
