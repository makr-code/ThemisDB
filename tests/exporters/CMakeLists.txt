# Exporters focused module tests (flat hierarchy under tests/exporters)

file(GLOB EXPORTERS_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS EXPORTERS_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_exporters_${_stem}_focused")
    set(_ctest "${_stem}_ExportersFocusedTests")
    set(_labels exporters)

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

    if(_stem STREQUAL "test_exporters_contract_hardening_focused" OR
       _stem STREQUAL "test_exporters_phase23_hardening_focused" OR
       _stem STREQUAL "test_export_encryption")
        list(APPEND _labels wave_b wave_b_boundary_content_io)
    endif()

    themis_register_module_focused_test(
        MODULE exporters
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${_labels}
    )
endforeach()
