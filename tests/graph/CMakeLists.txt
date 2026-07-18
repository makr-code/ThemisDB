# Graph focused module tests (flat hierarchy under tests/graph)

file(GLOB GRAPH_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS GRAPH_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_graph_${_stem}_focused")
    set(_ctest "${_stem}_GraphFocusedTests")
    set(_extra_sources)

    if(_stem STREQUAL "test_graph_watermarking")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/graph_watermark.cpp
        )
    endif()

    if(_stem STREQUAL "test_tensor_fingerprint_graph")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/tensor_fingerprint_graph.cpp
            ${THEMIS_ROOT_DIR}/src/graph/tensor_deduplication_manager.cpp
        )
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
        MODULE graph
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
endforeach()

# Lightweight launcher for focused shared-mutex distributed graph tests
add_executable(tgdsmf "${CMAKE_CURRENT_SOURCE_DIR}/tgdsmf_launcher.cpp")
target_include_directories(tgdsmf PRIVATE
    ${THEMIS_ROOT_DIR}/include
    ${THEMIS_ROOT_DIR}/src
)
target_link_libraries(tgdsmf PRIVATE
    ${TEST_LIBS}
    themis_core
    spdlog::spdlog
    Threads::Threads
)
target_compile_definitions(tgdsmf PRIVATE THEMIS_TEST_BUILD=1)

themis_register_module_focused_test(
    MODULE graph
    NAME DistributedGraphSharedMutexFocusedTests
    TARGET tgdsmf
    TIER unit
    TIMEOUT 60
)

# AUTOGEN PREFIX BLOCK: graph
file(GLOB GRAPH_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_graph_*.cpp"
)

foreach(_src IN LISTS GRAPH_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_graph_${_stem}_autofocused")
    set(_ctest "${_stem}_graph_AutoFocusedTests")
    set(_extra_sources)

    if(_stem STREQUAL "test_graph_watermarking")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/graph_watermark.cpp
        )
    endif()

    if(_stem STREQUAL "test_tensor_fingerprint_graph")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/tensor_fingerprint_graph.cpp
            ${THEMIS_ROOT_DIR}/src/graph/tensor_deduplication_manager.cpp
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
        MODULE graph
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS graph autogen
    )
endforeach()