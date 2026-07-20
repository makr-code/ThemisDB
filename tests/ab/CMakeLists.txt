# ab module tests

file(GLOB AB_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS AB_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_ab_${_stem}_focused")
    set(_ctest "${_stem}_ab_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    # Prefer linking against the modular `themis_rag` library when available;
    # fall back to inlining the minimal implementation sources for non-modular
    # or older build flows.
    if(TARGET themis_rag)
        target_link_libraries(${_target} PRIVATE themis_rag)
    else()
        if("${_stem}" STREQUAL "test_ab_test_production_integration")
            target_sources(${_target} PRIVATE
                ${THEMIS_ROOT_DIR}/src/rag/ab_test_production_integration.cpp
                ${THEMIS_ROOT_DIR}/src/rag/ml_learning_metrics_collector.cpp
            )
        endif()
    endif()
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        themis_llm
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE ab
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ab
    )
endforeach()