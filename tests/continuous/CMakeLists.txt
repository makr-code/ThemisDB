# continuous module tests

file(GLOB CONTINUOUS_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS CONTINUOUS_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_continuous_${_stem}_focused")
    set(_ctest "${_stem}_continuous_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    # Resolve modular static-link references pulled in by themis_query objects
    # (DocsAssistant/ThemisHelpLoRA/EmbeddedLLM factory and adapter symbols).
    if(TARGET themis_query)
        target_link_libraries(${_target} PRIVATE themis_query)
    endif()
    if(TARGET themis_llm_api)
        target_link_libraries(${_target} PRIVATE themis_llm_api)
    endif()
    if(TARGET themis_llm)
        target_link_libraries(${_target} PRIVATE themis_llm)
    endif()
    if(TARGET themis_llm_ext)
        target_link_libraries(${_target} PRIVATE themis_llm_ext)
    endif()
    # Ensure XML parser symbols resolve in focused modular test links when
    # security paths reference pugixml through shared-module boundaries.
    if(TARGET pugixml::shared)
        target_link_libraries(${_target} PRIVATE pugixml::shared)
    elseif(TARGET pugixml::pugixml)
        target_link_libraries(${_target} PRIVATE pugixml::pugixml)
    elseif(TARGET unofficial::pugixml::pugixml)
        target_link_libraries(${_target} PRIVATE unofficial::pugixml::pugixml)
    elseif(TARGET pugixml::static)
        target_link_libraries(${_target} PRIVATE pugixml::static)
    elseif(TARGET pugixml)
        target_link_libraries(${_target} PRIVATE pugixml)
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE continuous
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS continuous
    )
endforeach()
