# Auto-generated module tests for prefix 'philosophy'

file(GLOB PHILOSOPHY_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_philosophy_*.cpp"
)

foreach(_src IN LISTS PHILOSOPHY_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_philosophy_${_stem}_focused")
    set(_ctest "${_stem}_philosophy_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    set(_extra_sources)
    if(_stem STREQUAL "test_philosophy_loader")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/ethics_ai/philosophy_loader.cpp
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
        ${THEMIS_ROOT_DIR}/src/ethics_ai
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    if(_stem STREQUAL "test_philosophy_loader")
        find_package(yaml-cpp CONFIG QUIET)
        if(yaml-cpp_FOUND)
            target_link_libraries(${_target} PRIVATE yaml-cpp)
            target_compile_definitions(${_target} PRIVATE HAVE_YAML_CPP)
        endif()
    endif()

    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE philosophy
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS philosophy
    )
endforeach()