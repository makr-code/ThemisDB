# Auto-generated module tests for prefix 'online'

file(GLOB ONLINE_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_online_*.cpp"
)

foreach(_src IN LISTS ONLINE_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_online_${_stem}_focused")
    set(_ctest "${_stem}_online_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    set(_target_sources "${_src}")

    if(_stem STREQUAL "test_online_schema_migration")
        list(APPEND _target_sources
            ${THEMIS_ROOT_DIR}/src/storage/online_schema_migration.cpp
        )
    endif()

    add_executable(${_target} ${_target_sources})
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        themis_storage
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE online
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS online
    )
endforeach()