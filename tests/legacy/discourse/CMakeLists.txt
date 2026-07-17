# Auto-generated module tests for prefix 'discourse'

file(GLOB DISCOURSE_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_discourse_*.cpp"
)

foreach(_src IN LISTS DISCOURSE_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_discourse_${_stem}_focused")
    set(_ctest "${_stem}_discourse_FocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    if(_stem STREQUAL "test_discourse_engine")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/ethics_ai/discourse_engine.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/philosophy_loader.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/argument_store.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/rag_context_engine.cpp
            ${THEMIS_ROOT_DIR}/src/ethics_ai/ethics_ai_types.cpp
        )
    endif()

    add_executable(${_target}
        "${_src}"
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
        MODULE discourse
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS discourse
    )
endforeach()