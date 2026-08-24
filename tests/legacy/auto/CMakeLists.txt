# auto module tests

file(GLOB AUTO_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS AUTO_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_auto_${_stem}_focused")
    set(_ctest "${_stem}_auto_FocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    if(_stem STREQUAL "test_auto_failover_manager")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/failover/auto_failover_manager.cpp
            ${THEMIS_ROOT_DIR}/src/failover/quorum_log.cpp
        )
    endif()

    if(_stem STREQUAL "test_auto_labeler_db_fetch" OR _stem STREQUAL "test_auto_labeler_production")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/training/auto_labeler.cpp
            ${THEMIS_ROOT_DIR}/src/training/modality_parser.cpp
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
        MODULE auto
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS auto
    )
endforeach()