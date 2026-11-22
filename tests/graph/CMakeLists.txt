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

    if(_stem STREQUAL "test_graph_phase_gate_orchestration")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/graph_phase_gate_orchestrator.cpp
        )
    endif()

    if(_stem STREQUAL "test_graph_query_optimizer" OR
       _stem STREQUAL "test_graph_advanced_features")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/graph_query_optimizer.cpp
            ${THEMIS_ROOT_DIR}/src/graph/path_constraints.cpp
            ${THEMIS_ROOT_DIR}/src/graph/parallel_traversal.cpp
        )
    elseif(_stem STREQUAL "test_graph_distributed" OR
           _stem STREQUAL "test_graph_exact_traversal" OR
           _stem STREQUAL "test_query_explain")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/distributed_graph.cpp
            ${THEMIS_ROOT_DIR}/src/graph/graph_query_optimizer.cpp
            ${THEMIS_ROOT_DIR}/src/graph/path_constraints.cpp
            ${THEMIS_ROOT_DIR}/src/graph/parallel_traversal.cpp
        )
    elseif(_stem STREQUAL "test_graph_parallel_traversal")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/parallel_traversal.cpp
        )
    elseif(_stem STREQUAL "test_graph_query_rewriter")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/graph_query_rewriter.cpp
        )
    elseif(_stem STREQUAL "test_scheduled_edge_refresh")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/scheduled_edge_refresh.cpp
            ${THEMIS_ROOT_DIR}/src/graph/distributed_graph.cpp
            ${THEMIS_ROOT_DIR}/src/graph/graph_query_optimizer.cpp
            ${THEMIS_ROOT_DIR}/src/graph/path_constraints.cpp
            ${THEMIS_ROOT_DIR}/src/graph/parallel_traversal.cpp
        )
    elseif(_stem STREQUAL "test_ontology_manager")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/ontology_manager.cpp
        )
    elseif(_stem STREQUAL "test_path_constraints_semantic")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/ontology_manager.cpp
            ${THEMIS_ROOT_DIR}/src/graph/path_constraints.cpp
            ${THEMIS_ROOT_DIR}/src/graph/graph_query_optimizer.cpp
            ${THEMIS_ROOT_DIR}/src/graph/parallel_traversal.cpp
            ${THEMIS_ROOT_DIR}/src/graph/distributed_graph.cpp
        )
    elseif(_stem STREQUAL "test_knowledge_graph_reasoner")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/knowledge_graph_reasoner.cpp
        )
    elseif(_stem STREQUAL "test_rotate_completion" OR
           _stem STREQUAL "test_rotate_completion_acceptance")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/rotate_completion.cpp
            ${THEMIS_ROOT_DIR}/src/graph/knowledge_graph_reasoner.cpp
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
    if(_stem STREQUAL "test_graph_analytics" OR
       _stem STREQUAL "test_graph_type_filtering" OR
       _stem STREQUAL "test_graph_edge_encryption" OR
       _stem STREQUAL "test_graph_query_optimizer" OR
       _stem STREQUAL "test_graph_advanced_features" OR
       _stem STREQUAL "test_graph_distributed" OR
       _stem STREQUAL "test_graph_parallel_traversal" OR
       _stem STREQUAL "test_graph_exact_traversal" OR
       _stem STREQUAL "test_query_explain" OR
       _stem STREQUAL "test_scheduled_edge_refresh" OR
       _stem STREQUAL "test_path_constraints_semantic")
        target_link_libraries(${_target} PRIVATE fmt::fmt RocksDB::rocksdb)
        target_compile_definitions(${_target} PRIVATE THEMIS_GRAPH_EXPORTS)
        if(_stem STREQUAL "test_graph_query_optimizer" OR
           _stem STREQUAL "test_graph_advanced_features" OR
           _stem STREQUAL "test_graph_distributed" OR
           _stem STREQUAL "test_graph_exact_traversal")
            target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
        endif()
        if(WIN32 AND
           (_stem STREQUAL "test_graph_query_optimizer" OR
            _stem STREQUAL "test_graph_query_rewriter" OR
            _stem STREQUAL "test_graph_advanced_features" OR
            _stem STREQUAL "test_graph_parallel_traversal" OR
            _stem STREQUAL "test_graph_exact_traversal"))
            themis_sync_runtime_dlls_for_target(${_target})
            add_dependencies(${_target} sync_test_runtime_dlls)
        endif()
    endif()
    if(_stem STREQUAL "test_graph_query_rewriter")
        target_link_libraries(${_target} PRIVATE fmt::fmt nlohmann_json::nlohmann_json)
        if(WIN32)
            themis_sync_runtime_dlls_for_target(${_target})
            add_dependencies(${_target} sync_test_runtime_dlls)
        endif()
    endif()
    if(_stem STREQUAL "test_query_explain" OR
       _stem STREQUAL "test_scheduled_edge_refresh" OR
       _stem STREQUAL "test_path_constraints_semantic")
        target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
    endif()
    if(_stem STREQUAL "test_graph_edge_encryption")
        target_link_libraries(${_target} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
    if(_stem STREQUAL "test_graph_index_comprehensive")
        target_link_libraries(${_target} PRIVATE RocksDB::rocksdb)
    endif()
    if(_stem STREQUAL "test_knowledge_graph_reasoner")
        target_compile_definitions(${_target} PRIVATE THEMIS_GRAPH_EXPORTS)
    endif()
    if(_stem STREQUAL "test_rotate_completion" OR
       _stem STREQUAL "test_rotate_completion_acceptance")
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
        )
    endif()

    themis_register_module_focused_test(
        MODULE graph
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )

    if(_stem STREQUAL "test_gpu_traversal")
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
        )
    elseif(_stem STREQUAL "test_tensor_fingerprint_graph")
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/cmake/tests"
            EXTRA_ARGS --gtest_color=yes
        )
    endif()
endforeach()

# AUTOGEN PREFIX BLOCK: graph
file(GLOB GRAPH_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_graph_*.cpp"
)

foreach(_src IN LISTS GRAPH_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_manual_target "module_graph_${_stem}_focused")
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

    if(_stem STREQUAL "test_graph_phase_gate_orchestration")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/graph/graph_phase_gate_orchestrator.cpp
        )
    endif()

    if(TARGET ${_manual_target} OR TARGET ${_target})
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