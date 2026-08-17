# Process focused module tests (flat hierarchy under tests/process)

file(GLOB PROCESS_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS PROCESS_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_process_${_stem}_focused")
    set(_ctest "${_stem}_ProcessFocusedTests")
    set(_extra_sources)

    if(_stem STREQUAL "test_process_graph")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/index/edge_types.cpp
        )
    elseif(_stem STREQUAL "test_process_module")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/process/dmn_evaluator.cpp
            ${THEMIS_ROOT_DIR}/src/process/ocel_exporter.cpp
            ${THEMIS_ROOT_DIR}/src/process/process_model_generator.cpp
        )
    elseif(_stem STREQUAL "test_critical_batch1_fixes")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/process/dmn_evaluator.cpp
            ${THEMIS_ROOT_DIR}/src/process/process_graph_rag.cpp
            ${THEMIS_ROOT_DIR}/src/process/vcc_vpb_importer.cpp
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
        MODULE process
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS "release_critical;process"
    )
endforeach()

# AUTOGEN PREFIX BLOCK: process
file(GLOB PROCESS_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_process_*.cpp"
)

foreach(_src IN LISTS PROCESS_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_process_${_stem}_autofocused")
    set(_ctest "${_stem}_process_AutoFocusedTests")
    set(_extra_sources)

    if(_stem STREQUAL "test_process_graph")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/index/edge_types.cpp
        )
    elseif(_stem STREQUAL "test_process_module")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/process/dmn_evaluator.cpp
            ${THEMIS_ROOT_DIR}/src/process/ocel_exporter.cpp
            ${THEMIS_ROOT_DIR}/src/process/process_model_generator.cpp
        )
    elseif(_stem STREQUAL "test_critical_batch1_fixes")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/process/dmn_evaluator.cpp
            ${THEMIS_ROOT_DIR}/src/process/process_graph_rag.cpp
            ${THEMIS_ROOT_DIR}/src/process/vcc_vpb_importer.cpp
        )
    endif()

    if(TARGET ${_target})
        continue()
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
        MODULE process
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS "release_critical;process;autogen"
    )
endforeach()