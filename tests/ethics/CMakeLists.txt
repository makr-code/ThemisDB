# ethics module tests

file(GLOB ETHICS_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

find_package(yaml-cpp QUIET)

foreach(_src IN LISTS ETHICS_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_ethics_${_stem}_focused")
    set(_ctest "${_stem}_ethics_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    set(_ethics_pipeline_sources)
    if(_stem STREQUAL "test_ethics_ai_pipeline")
        list(APPEND _ethics_pipeline_sources
            ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_ai_types.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/philosophy_loader.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/argument_store.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/chain_visualizer.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/rag_context_engine.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/discourse_engine.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_evaluator.cpp
        )
    endif()
    add_executable(${_target} "${_src}" ${_ethics_pipeline_sources})
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
    if(_stem STREQUAL "test_ethics_ai_pipeline")
        target_include_directories(${_target} PRIVATE ${THEMIS_ROOT_DIR}/src/ethics_ai)
        if(TARGET themis_query)
            target_link_libraries(${_target} PRIVATE themis_query)
        endif()
        if(TARGET themis_storage)
            target_link_libraries(${_target} PRIVATE themis_storage)
        endif()
        if(TARGET yaml-cpp::yaml-cpp)
            target_link_libraries(${_target} PRIVATE yaml-cpp::yaml-cpp)
            target_compile_definitions(${_target} PRIVATE HAVE_YAML_CPP)
        elseif(TARGET yaml-cpp)
            target_link_libraries(${_target} PRIVATE yaml-cpp)
            target_compile_definitions(${_target} PRIVATE HAVE_YAML_CPP)
        endif()
    else()
        # If the Ethics AI plugin is available in this modular build, link it
        # so focused tests have the implementation symbols at link time.
        if(TARGET ethics_ai_plugin)
            target_link_libraries(${_target} PRIVATE ethics_ai_plugin)
        else()
            # If the plugin target is not present (plugin disabled in this build),
            # compile the plugin implementation sources directly into the focused
            # test binary so required symbols are available at link time.
            set(_ethics_plugin_src_dir "${THEMIS_ROOT_DIR}/src/ethics_ai")
            if(EXISTS "${_ethics_plugin_src_dir}/argument_store.cpp")
                target_sources(${_target} PRIVATE
                    ${_ethics_plugin_src_dir}/ethics_ai_types.cpp
                    ${_ethics_plugin_src_dir}/ethics_ai_plugin.cpp
                    ${_ethics_plugin_src_dir}/philosophy_loader.cpp
                    ${_ethics_plugin_src_dir}/argument_store.cpp
                    ${_ethics_plugin_src_dir}/rag_context_engine.cpp
                    ${_ethics_plugin_src_dir}/discourse_engine.cpp
                    ${_ethics_plugin_src_dir}/chain_visualizer.cpp
                    ${_ethics_plugin_src_dir}/ethics_profile_registry.cpp
                    ${_ethics_plugin_src_dir}/ethics_selection_router.cpp
                )
                target_include_directories(${_target} PRIVATE ${_ethics_plugin_src_dir})
                # Match compile defs used by the plugin build to ensure consistent symbols
                if(yaml-cpp_FOUND)
                    target_compile_definitions(${_target} PRIVATE THEMIS_PLUGIN_EXPORTS HAVE_YAML_CPP)
                    target_link_libraries(${_target} PRIVATE yaml-cpp themis_core)
                else()
                    target_compile_definitions(${_target} PRIVATE THEMIS_PLUGIN_EXPORTS)
                    target_link_libraries(${_target} PRIVATE themis_core)
                endif()
            endif()
        endif()
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE ethics
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ethics
    )

    if(_stem STREQUAL "test_ethics_ai_pipeline")
        add_test(NAME EthicsAiPipelineFocusedTests
            COMMAND $<TARGET_FILE:${_target}>
        )
        set_tests_properties(EthicsAiPipelineFocusedTests PROPERTIES
            LABELS "ethics;ai;pipeline;rag;integration;focused"
            TIMEOUT 120
        )
    endif()
endforeach()