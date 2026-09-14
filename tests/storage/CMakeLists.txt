# Storage focused module tests (flat hierarchy under tests/storage)

file(GLOB STORAGE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS STORAGE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_storage_${_stem}_focused")
    set(_ctest "${_stem}_StorageFocusedTests")
    set(_labels)

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
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    if(_stem STREQUAL "test_tensor_storage_observer")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/graph/tensor_fingerprint_graph.cpp
        )
    elseif(_stem STREQUAL "test_storage_contract_hardening_focused")
        list(APPEND _labels storage contract hardening wave_a release_critical)
    elseif(_stem STREQUAL "test_storage_phase3_error_handling_focused")
        list(APPEND _labels storage error_handling phase3 wave_a release_critical)
    elseif(_stem STREQUAL "test_mvcc_chain_pruner")
        list(APPEND _labels storage mvcc pruning wave_a release_critical)
    endif()

    themis_register_module_focused_test(
        MODULE storage
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${_labels}
    )
endforeach()

# AUTOGEN PREFIX BLOCK: storage
file(GLOB STORAGE_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_storage_*.cpp"
)

foreach(_src IN LISTS STORAGE_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_storage_${_stem}_autofocused")
    set(_ctest "${_stem}_storage_AutoFocusedTests")
    set(_labels storage autogen)

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
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    if(_stem STREQUAL "test_storage_phase3_error_handling_focused")
        list(APPEND _labels error_handling phase3 wave_a release_critical)
    endif()

    themis_register_module_focused_test(
        MODULE storage
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${_labels}
    )
endforeach()