# Temporal focused module tests (flat hierarchy under tests/temporal)

file(GLOB TEMPORAL_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS TEMPORAL_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_temporal_${_stem}_focused")
    set(_ctest "${_stem}_TemporalFocusedTests")

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
    if(TARGET themis_temporal)
        target_link_libraries(${_target} PRIVATE themis_temporal)
        message(STATUS "Linked ${_target} with themis_temporal module")
    endif()
    # Ensure TemporalStats implementation is available when modular index
    # module is not built. Prefer linking the module; otherwise add the TU
    # implementing `TemporalStats::toString` to the test target (guarded).
    if(NOT TARGET themis_temporal)
        if(EXISTS "${THEMIS_ROOT_DIR}/src/index/temporal_graph.cpp")
            target_sources(${_target} PRIVATE "${THEMIS_ROOT_DIR}/src/index/temporal_graph.cpp")
            message(STATUS "Added temporal_graph.cpp into ${_target} (themis_temporal not present)")
        endif()
    endif()
    set(_temporal_extra_sources)
    if(_stem STREQUAL "test_bi_temporal")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/bi_temporal.cpp")
    elseif(_stem STREQUAL "test_interval_tree_index")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/interval_tree_index.cpp")
    elseif(_stem STREQUAL "test_retention_manager")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/retention_manager.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/system_versioned_table.cpp")
    elseif(_stem STREQUAL "test_snapshot_manager")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/snapshot_manager.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/system_versioned_table.cpp")
    elseif(_stem STREQUAL "test_temporal_aggregator")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/system_versioned_table.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/temporal_aggregator.cpp")
    elseif(_stem STREQUAL "test_temporal_cdc")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/temporal_cdc.cpp")
    elseif(_stem STREQUAL "test_temporal_cold_store")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/temporal_cold_store.cpp")
    elseif(_stem STREQUAL "test_temporal_compressor")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/system_versioned_table.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/temporal_compressor.cpp")
    elseif(_stem STREQUAL "test_temporal_conflict_resolver")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/replication/replication_manager.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/temporal_conflict_resolver.cpp")
    elseif(_stem STREQUAL "test_temporal_migrator")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/system_versioned_table.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/temporal_migrator.cpp")
    elseif(_stem STREQUAL "test_temporal_query_engine")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/bi_temporal.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/system_versioned_table.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/temporal_index.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/temporal_query_engine.cpp")
    elseif(_stem STREQUAL "test_temporal_tier_manager")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/temporal_cold_store.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/temporal_tier_manager.cpp")
    elseif(_stem STREQUAL "test_temporal_v18_v19")
        list(APPEND _temporal_extra_sources
            "${THEMIS_ROOT_DIR}/src/temporal/bi_temporal.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/snapshot_manager.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/system_versioned_table.cpp"
            "${THEMIS_ROOT_DIR}/src/temporal/temporal_cdc.cpp")
    endif()
    if(_temporal_extra_sources)
        target_sources(${_target} PRIVATE ${_temporal_extra_sources})
    endif()
    if(_stem STREQUAL "test_temporal_compressor" AND THEMIS_LZ4_TARGET)
        target_link_libraries(${_target} PRIVATE ${THEMIS_LZ4_TARGET})
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE temporal
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
    set(_temporal_discovery_labels)
    if(_stem STREQUAL "test_snapshot_manager")
        set(_temporal_discovery_labels "temporal;snapshot;isolation;unit")
    elseif(_stem STREQUAL "test_interval_tree_index")
        set(_temporal_discovery_labels "temporal;interval-tree;unit")
    elseif(_stem STREQUAL "test_temporal_compressor")
        set(_temporal_discovery_labels "temporal;compressor;unit")
    elseif(_stem STREQUAL "test_temporal_cdc")
        set(_temporal_discovery_labels "temporal;cdc;unit")
    elseif(_stem STREQUAL "test_temporal_migrator")
        set(_temporal_discovery_labels "temporal;migrator;phase5;unit")
    elseif(_stem STREQUAL "test_retention_manager")
        set(_temporal_discovery_labels "temporal;retention;unit")
    elseif(_stem STREQUAL "test_temporal_conflict_resolver")
        set(_temporal_discovery_labels "temporal;conflict-resolver;unit")
    elseif(_stem STREQUAL "test_temporal_aggregator")
        set(_temporal_discovery_labels "temporal;aggregator;unit")
    elseif(_stem STREQUAL "test_temporal_query_engine")
        set(_temporal_discovery_labels "temporal;query-engine;unit")
    endif()
    if(_temporal_discovery_labels)
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
            CONFIGURATIONS Release Debug
            PROPERTIES
                LABELS "${_temporal_discovery_labels}"
                TIMEOUT 60
        )
    endif()
endforeach()

# AUTOGEN PREFIX BLOCK: temporal
file(GLOB TEMPORAL_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_temporal_*.cpp"
)

foreach(_src IN LISTS TEMPORAL_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_temporal_${_stem}_focused")
    set(_ctest "${_stem}_temporal_AutoFocusedTests")

    if(TARGET ${_target})
        themis_register_module_focused_test(
            MODULE temporal
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS temporal autogen
        )
    else()
        message(FATAL_ERROR
            "AutoFocused temporal test '${_src}' has no canonical target '${_target}'"
        )
    endif()
endforeach()