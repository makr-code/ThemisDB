# search module tests

file(GLOB SEARCH_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS SEARCH_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_search_${_stem}_focused")
    set(_ctest "${_stem}_search_FocusedTests")
    set(_extra_sources "")

    if(TARGET ${_target})
        continue()
    endif()

    if(_stem STREQUAL "test_layered_retrieval_integration_phase4")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/search/layered_retrieval_orchestrator.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_fingerprint_graph.cpp
        )
    endif()

    add_executable(${_target} "${_src}")
    # Add implementation sources required by the focused test executable
    target_sources(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/src/search/conversational_search.cpp
        ${THEMIS_ROOT_DIR}/src/search/federated_search.cpp
        ${THEMIS_ROOT_DIR}/src/search/search_result_stream.cpp
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
        MODULE search
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS search
    )
endforeach()