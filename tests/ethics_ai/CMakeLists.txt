cmake_minimum_required(VERSION 3.20)

file(GLOB ETHICS_AI_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS ETHICS_AI_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_ethics_ai_${_stem}_focused")
    set(_ctest "${_stem}_EthicsAiFocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")

    # Compile the LDM implementation sources directly into the test target so
    # focused tests have no dependency on the full ethics_ai_plugin shared library.
    target_sources(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_ai_types.cpp
        ${THEMIS_ROOT_DIR}/src/ethics_ai/philosophy_loader.cpp
        ${THEMIS_ROOT_DIR}/src/ethics_ai/argument_store.cpp
        ${THEMIS_ROOT_DIR}/src/ethics_ai/rag_context_engine.cpp
        ${THEMIS_ROOT_DIR}/src/ethics_ai/discourse_engine.cpp
        ${THEMIS_ROOT_DIR}/src/ethics_ai/chain_visualizer.cpp
        ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_profile_registry.cpp
        ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_selection_router.cpp
        ${THEMIS_ROOT_DIR}/src/ethics_ai/discourse_orchestrator.cpp
        ${THEMIS_ROOT_DIR}/src/ethics_ai/meta_verdict_builder.cpp
        ${THEMIS_ROOT_DIR}/src/ethics_ai/mirror_school_handler.cpp
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
    set_target_properties(${_target} PROPERTIES CXX_STANDARD 20)

    if(TARGET yaml-cpp::yaml-cpp)
        target_link_libraries(${_target} PRIVATE yaml-cpp::yaml-cpp)
        target_compile_definitions(${_target} PRIVATE HAVE_YAML_CPP)
    endif()

    themis_register_module_focused_test(
        MODULE ethics_ai
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ethics_ai
    )
endforeach()
