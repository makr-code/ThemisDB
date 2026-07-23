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
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE ethics
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ethics
    )
endforeach()