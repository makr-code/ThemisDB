# hnsw module tests

file(GLOB HNSW_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS HNSW_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_hnsw_${_stem}_focused")
    set(_ctest "${_stem}_hnsw_FocusedTests")
    set(_extra_sources)

    if(_stem STREQUAL "test_hnsw_tt_bridge")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/tensor/hnsw_tt_bridge.cpp
        )
    endif()

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}" ${_extra_sources})
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
        MODULE hnsw
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS hnsw
    )
endforeach()