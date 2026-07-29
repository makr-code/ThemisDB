# Auto-generated module tests for prefix 'failover'

file(GLOB FAILOVER_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_failover_*.cpp"
)

foreach(_src IN LISTS FAILOVER_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_failover_${_stem}_focused")
    set(_ctest "${_stem}_failover_FocusedTests")
    set(_extra_sources)

    if(_stem STREQUAL "test_failover_chaos_scenarios")
        list(APPEND _extra_sources
            "${THEMIS_ROOT_DIR}/src/failover/auto_failover_manager.cpp"
        )
    endif()

    if(_stem STREQUAL "test_failover_dr_edge_scenarios")
        list(APPEND _extra_sources
            "${THEMIS_ROOT_DIR}/src/failover/disaster_recovery_manager.cpp"
        )
    endif()

    if(_stem STREQUAL "test_failover_phase2_phase3_focused")
        list(APPEND _extra_sources
            "${THEMIS_ROOT_DIR}/src/failover/auto_failover_manager.cpp"
            "${THEMIS_ROOT_DIR}/src/failover/disaster_recovery_manager.cpp"
        )
    endif()

    if(TARGET ${_target})
        continue()
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
        MODULE failover
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS failover
    )
endforeach()