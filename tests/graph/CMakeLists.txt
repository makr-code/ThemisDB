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
add_executable(tgdsmf_launcher "${CMAKE_CURRENT_SOURCE_DIR}/tgdsmf_launcher.cpp")
target_include_directories(tgdsmf_launcher PRIVATE
    ${THEMIS_ROOT_DIR}/include
    ${THEMIS_ROOT_DIR}/src
)
target_link_libraries(tgdsmf_launcher PRIVATE
    ${TEST_LIBS}
    themis_core
    spdlog::spdlog
    Threads::Threads
)
target_compile_definitions(tgdsmf_launcher PRIVATE THEMIS_TEST_BUILD=1)

# After building the launcher, copy it to the canonical test name so CTest finds `tgdsmf.exe`.
add_custom_command(TARGET tgdsmf_launcher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin_out"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:tgdsmf_launcher> "${CMAKE_BINARY_DIR}/bin_out/tgdsmf.exe"
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