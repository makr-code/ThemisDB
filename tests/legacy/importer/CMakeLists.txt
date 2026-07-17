# importer module tests

file(GLOB IMPORTER_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS IMPORTER_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_importer_${_stem}_focused")
    set(_ctest "${_stem}_importer_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    set(_target_sources "${_src}")

    if(_stem STREQUAL "test_wikipedia_ingestion_plugin")
        list(APPEND _target_sources
            ${THEMIS_ROOT_DIR}/src/importers/wikipedia_plugin.cpp
            ${THEMIS_ROOT_DIR}/src/importers/wikipedia_pipeline.cpp
            ${THEMIS_ROOT_DIR}/src/importers/wikipedia_dump_reader.cpp
            ${THEMIS_ROOT_DIR}/src/importers/wikipedia_xml_parser.cpp
            ${THEMIS_ROOT_DIR}/src/importers/wikipedia_transform.cpp
            ${THEMIS_ROOT_DIR}/src/importers/wikipedia_project_graph.cpp
            ${THEMIS_ROOT_DIR}/src/importers/wikipedia_project_vector.cpp
            ${THEMIS_ROOT_DIR}/src/importers/wikipedia_project_timeseries.cpp
            ${THEMIS_ROOT_DIR}/src/importers/wikipedia_project_process.cpp
            ${THEMIS_ROOT_DIR}/src/importers/wikipedia_checkpoint.cpp
            ${THEMIS_ROOT_DIR}/src/importers/wikipedia_validator.cpp
            ${THEMIS_ROOT_DIR}/src/plugins/plugin_registry.cpp
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
        spdlog::spdlog
        Threads::Threads
    )

    if(_stem STREQUAL "test_wikipedia_ingestion_plugin")
        target_link_libraries(${_target} PRIVATE
            nlohmann_json::nlohmann_json
            ${CMAKE_DL_LIBS}
        )
    endif()

    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE importer
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS importer
    )
endforeach()