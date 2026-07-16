# Auto-generated module tests for prefix 'behavioral'

file(GLOB BEHAVIORAL_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_behavioral_*.cpp"
)

foreach(_src IN LISTS BEHAVIORAL_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_behavioral_${_stem}_focused")
    set(_ctest "${_stem}_behavioral_FocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    if(_stem STREQUAL "test_behavioral_anomaly_detector")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/security/behavioral_anomaly_detector.cpp
        )
    endif()

    add_executable(${_target}
        "${_src}"
        ${_extra_sources}
    )
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
        MODULE behavioral
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS behavioral
    )
endforeach()