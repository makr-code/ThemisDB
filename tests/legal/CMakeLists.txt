# legal module tests

file(GLOB LEGAL_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS LEGAL_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_legal_${_stem}_focused")
    set(_ctest "${_stem}_legal_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_sources(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/src/training/auto_labeler.cpp
        ${THEMIS_ROOT_DIR}/src/training/knowledge_graph_enricher.cpp
        ${THEMIS_ROOT_DIR}/src/training/modality_parser.cpp
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE legal
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS legal
    )
endforeach()