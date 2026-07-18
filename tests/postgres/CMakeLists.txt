# postgres module tests

file(GLOB POSTGRES_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS POSTGRES_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_postgres_${_stem}_focused")
    set(_ctest "${_stem}_postgres_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")

    if(_stem STREQUAL "test_postgres_importer_fk" OR
       _stem STREQUAL "test_postgres_import_integration_v2")
        target_sources(${_target} PRIVATE
            "${THEMIS_ROOT_DIR}/src/importers/postgres_importer.cpp"
        )
    endif()

    if(_stem STREQUAL "test_postgres_importer_mdm")
        target_sources(${_target} PRIVATE
            "${THEMIS_ROOT_DIR}/src/importers/postgres_importer.cpp"
            "${THEMIS_ROOT_DIR}/src/importers/postgres_importer_mdm.cpp"
        )
    endif()

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
        MODULE postgres
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS postgres
    )
endforeach()