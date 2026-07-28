# tensor module tests

file(GLOB TENSOR_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS TENSOR_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_tensor_${_stem}_focused")
    set(_ctest "${_stem}_tensor_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if("${_stem}" STREQUAL "test_tensor_phase3")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/query/tensor_aware_query_optimizer.cpp
            ${THEMIS_ROOT_DIR}/src/rag/targ_retrieval.cpp
            ${THEMIS_ROOT_DIR}/src/rag/flare_retrieval.cpp
            ${THEMIS_ROOT_DIR}/src/rag/tensor_rag_pipeline.cpp
            ${THEMIS_ROOT_DIR}/src/storage/tensor_compaction_filter.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_butterfly_operator.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_fingerprint_graph.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/adapter_repository.cpp
        )
    endif()
    if("${_stem}" STREQUAL "test_tensor_mid_layer_integration")
        # Integration test for full tensor mid-layer pipeline (Phase 4)
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/tensor/adapter_repository.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_fingerprint_graph.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_error_handling.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_routing_strategy.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_redundancy_detection.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/compression_strategy.cpp
        )
    endif()
    if("${_stem}" STREQUAL "test_federated_tensor_summaries")
        # Federated and cross-shard tensor summaries tests (Issue #5427)
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/tensor/adapter_repository.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_fingerprint_graph.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_summary_types.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_error_handling.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_routing_strategy.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/compression_strategy.cpp
        )
    endif()
    if("${_stem}" STREQUAL "test_persistent_tensor_fingerprint_graph")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_fingerprint_graph.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/persistent_tensor_fingerprint_graph.cpp
        )
    endif()
    if("${_stem}" STREQUAL "test_tensor_acceleration_observability")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_compression_routing_accelerator.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_workflow_observability.cpp
        )
    endif()
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

    # Link the shared/module library when available; otherwise add
    # a guarded fallback that compiles the necessary tensor implementation
    # translation units directly into the test target. This mirrors the
    # pattern used in other tests to make focused/autofocused builds work
    # even when the full module target is not part of the current build.
    if(TARGET themis_tensor)
        target_link_libraries(${_target} PRIVATE themis_tensor)
    else()
        # Add the common tensor implementation sources used by tests.
        # Only add files that actually exist to avoid configure-time errors.
        set(_tensor_impls
            ${THEMIS_ROOT_DIR}/src/storage/tensor_compaction_filter.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/adapter_repository.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/compression_strategy.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/hiss_structural_search.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/hnsw_tt_bridge.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/persistent_tensor_fingerprint_graph.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/ht_index.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/hyper_index_builder.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_compression_routing_accelerator.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_butterfly_operator.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_core_bridge.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_error_handling.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_fingerprint_graph.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_index.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_index_manager.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_ingestion_bridge.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_mid_layer.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_mmap_bridge.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_redundancy_detection.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_routing_strategy.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_summary_types.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tensor_workflow_observability.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/tnsr_task.cpp
            ${THEMIS_ROOT_DIR}/src/tensor/utr_converter.cpp
        )
        foreach(_impl IN LISTS _tensor_impls)
            if(EXISTS "${_impl}")
                target_sources(${_target} PRIVATE ${_impl})
            endif()
        endforeach()
    endif()

    themis_register_module_focused_test(
        MODULE tensor
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS tensor
    )
endforeach()