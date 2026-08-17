# Query module test registration (pilot extraction from root tests/CMakeLists.txt)

# Query gap-fix focused tests migrated from root tests/CMakeLists.txt.
foreach(_src_name test_pagerank test_query_cancellation)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_src_name}.cpp")
        set(_target "module_query_${_src_name}_focused")
        set(_ctest "${_src_name}_QueryFocusedTests")

        add_executable(${_target}
            "${CMAKE_CURRENT_LIST_DIR}/${_src_name}.cpp"
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

        target_compile_definitions(${_target} PRIVATE
            THEMIS_TEST_BUILD=1
        )

        themis_register_module_focused_test(
            MODULE query
            NAME ${_ctest}
            TARGET ${_target}
            TIER unit
            TIMEOUT 120
            LABELS pagerank cancellation gap-fix
        )
    endif()
endforeach()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_query_engine.cpp")
    message(STATUS "Adding Query: QueryEngine focused tests")

    add_executable(test_query_engine_focused
        test_query_engine.cpp
    )

    target_include_directories(test_query_engine_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_query_engine_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_query_engine_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME QueryEngineFocusedTests
        TARGET test_query_engine_focused
        TIER unit
        TIMEOUT 90
        LABELS engine aql
    )

    message(STATUS "  QueryEngine: AQL execution, optimizer, multi-model queries")
else()
    message(STATUS "QueryEngine focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_query_engine.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_query_federation.cpp")
    message(STATUS "Adding Query: QueryFederation shard-key routing focused tests")

    add_executable(test_query_federation_focused
        test_query_federation.cpp
    )

    target_include_directories(test_query_federation_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_query_federation_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_query_federation_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME QueryFederationShardRoutingTests
        TARGET test_query_federation_focused
        TIER unit
        TIMEOUT 60
        LABELS federation sharding
    )

    message(STATUS "  QueryFederation: shard-key routing, partition pruning, scatter-gather")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_query_federation_hardness.cpp")
    message(STATUS "Adding Query: QueryFederation Phase 3 Hardness tests (FED-01..08, timeout/retry/memory)")

    add_executable(test_query_federation_hardness_focused
       test_query_federation_hardness.cpp
    )

    target_include_directories(test_query_federation_hardness_focused PRIVATE
       ${THEMIS_ROOT_DIR}/include
       ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_query_federation_hardness_focused PRIVATE
       ${TEST_LIBS}
       themis_core
       spdlog::spdlog
       Threads::Threads
    )

    target_compile_definitions(test_query_federation_hardness_focused PRIVATE
       THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
       MODULE query
       NAME QueryFederationHardnessTests
       TARGET test_query_federation_hardness_focused
       TIER unit
       TIMEOUT 120
       LABELS federation hardness timeout memory fault-injection phase3
    )

    message(STATUS "  QueryFederation Hardness: timeout/retry, bounded memory, shard routing, fault-injection (FED-01..08)")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_continuous_query_engine.cpp")
    message(STATUS "Adding Query: ContinuousQueryEngine unit tests (CQ-01..CQ-20)")

    add_executable(test_continuous_query_engine_focused
        test_continuous_query_engine.cpp
    )

    target_include_directories(test_continuous_query_engine_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_continuous_query_engine_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_continuous_query_engine_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME ContinuousQueryEngineTests
        TARGET test_continuous_query_engine_focused
        TIER unit
        TIMEOUT 60
        LABELS cql continuous
    )

    message(STATUS "  ContinuousQueryEngine: window spec, synopsis, incremental agg, watermark, result modes")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_continuous_query_e2e.cpp")
    message(STATUS "Adding Query: ContinuousQuery E2E integration tests (CQI-01..05)")

    add_executable(test_continuous_query_e2e
        test_continuous_query_e2e.cpp
    )

    target_include_directories(test_continuous_query_e2e PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_continuous_query_e2e PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_continuous_query_e2e PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE query
        NAME ContinuousQueryE2ETests
        TARGET test_continuous_query_e2e
        TIER integration
        KIND e2e
        TIMEOUT 120
        LABELS cql continuous integration e2e
    )

    message(STATUS "  ContinuousQuery E2E: register, inject, stream, fan-out, late events, cancel/resume, drop/re-register")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_query_plan_visualizer.cpp")
    message(STATUS "Adding Query: QueryPlanVisualizer focused tests")

    add_executable(test_query_plan_visualizer_focused
        test_query_plan_visualizer.cpp
    )

    target_include_directories(test_query_plan_visualizer_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
    )

    target_link_libraries(test_query_plan_visualizer_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_query_plan_visualizer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME QueryPlanVisualizerFocusedTests
        TARGET test_query_plan_visualizer_focused
        TIER unit
        TIMEOUT 60
        LABELS visualizer explain
    )

    message(STATUS "  QueryPlanVisualizer: EXPLAIN, EXPLAIN ANALYZE, JSON/text output")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_query_plan_caching.cpp")
    message(STATUS "Adding Query Plan Caching focused tests (v1.7.0, Issue #196)")

    add_executable(test_query_plan_caching_focused
        test_query_plan_caching.cpp
    )

    target_include_directories(test_query_plan_caching_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_query_plan_caching_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
        OpenSSL::Crypto
    )

    target_compile_definitions(test_query_plan_caching_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME QueryPlanCachingFocusedTests
        TARGET test_query_plan_caching_focused
        TIER unit
        TIMEOUT 60
        LABELS plan-cache fingerprint invalidation statistics v1.7.0
    )

    message(STATUS "  PlanCache: fingerprinting, parameterized reuse, table invalidation, stats drift, LRU eviction, 24h TTL")
else()
    message(STATUS "Query Plan Caching tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_query_plan_caching.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_query_jit_compilation.cpp")
    message(STATUS "Adding Query JIT Compilation focused tests (v1.8.0, Issue #89)")

    add_executable(test_query_jit_compilation_focused
        test_query_jit_compilation.cpp
    )

    target_include_directories(test_query_jit_compilation_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_query_jit_compilation_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_query_jit_compilation_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME QueryJITCompilationFocusedTests
        TARGET test_query_jit_compilation_focused
        TIER unit
        TIMEOUT 60
        LABELS jit compilation hot-path v1.8.0
    )

    message(STATUS "  QueryCompiler: hot-path detection, result identity, fallback, key determinism, invalidation, stats")
else()
    message(STATUS "Query JIT Compilation tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_query_jit_compilation.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_query_federation_routing.cpp")
    message(STATUS "Adding QueryFederation shard-key routing focused tests (query, v1.9.0)")

    add_executable(test_query_federation_routing_focused
        test_query_federation_routing.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/sharding_manager_edition.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/consistent_hash.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/shard_topology.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/urn_resolver.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/remote_executor.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/shard_router.cpp
    )

    target_include_directories(test_query_federation_routing_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_query_federation_routing_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    themis_register_module_focused_test(
        MODULE query
        NAME QueryFederationRoutingFocusedTests
        TARGET test_query_federation_routing_focused
        TIER unit
        TIMEOUT 60
        LABELS federation shard-key routing point-lookup range broadcast v1.9.0
    )

    message(STATUS "  QueryFederation: point-lookup→1 shard, range→subset, full-scan→broadcast; ShardingManager::GetShardForKey+GetShardsForKeyRange")
else()
    message(STATUS "QueryFederation routing focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_query_federation_routing.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_aql_llm_integration.cpp")
    message(STATUS "Adding AQL LLM Integration validation tests (AQL Consolidation Phase 2)")

    add_executable(test_aql_llm_integration_focused
        test_aql_llm_integration.cpp
    )

    target_include_directories(test_aql_llm_integration_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_aql_llm_integration_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_aql_llm_integration_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME AQLLLMIntegrationTests
        TARGET test_aql_llm_integration_focused
        TIER integration
        TIMEOUT 60
        LABELS aql parser llm validation consolidation
    )

    message(STATUS "  AQL LLM Integration: Parser validation, diagnostic enrichment, error feedback, retry pipeline")
else()
    message(STATUS "AQL LLM Integration tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_aql_llm_integration.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_aql_validation_performance.cpp")
    message(STATUS "Adding AQL Validation Performance tests (AQL Consolidation Phase 4)")

    add_executable(test_aql_validation_performance_focused
        test_aql_validation_performance.cpp
    )

    target_include_directories(test_aql_validation_performance_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_aql_validation_performance_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_aql_validation_performance_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME AQLValidationPerformanceTests
        TARGET test_aql_validation_performance_focused
        TIER performance
        TIMEOUT 120
        LABELS aql parser validation performance sla consolidation
    )

    message(STATUS "  AQL Validation Performance: SLA verification (≤500ms), throughput (≥100q/s), error enrichment")
else()
    message(STATUS "AQL Validation Performance tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_aql_validation_performance.cpp")
endif()

# AQL DDL Phase 2 — Schema DDL parser + executor tests (DDL-01..DDL-32)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_aql_ddl_phase2.cpp")
    message(STATUS "Adding AQL DDL Phase 2 focused tests (DDL-01..DDL-32)")

    add_executable(test_aql_ddl_phase2_focused
        test_aql_ddl_phase2.cpp
        ${THEMIS_ROOT_DIR}/src/query/ddl_executor.cpp
    )

    target_include_directories(test_aql_ddl_phase2_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_aql_ddl_phase2_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_aql_ddl_phase2_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME AQLDDLPhase2Tests
        TARGET test_aql_ddl_phase2_focused
        TIER unit
        TIMEOUT 120
        LABELS aql ddl schema phase2 parser executor
    )

    message(STATUS "  AQL DDL Phase 2: CREATE/DROP/ALTER COLLECTION/INDEX/VIEW, IF EXISTS, OPTIONS, thread-safety")
endif()

# Phase 4 Block 1 — Query Hardening tests (Issue #5184)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_phase4_query_hardening.cpp")
    message(STATUS "Adding Phase 4 Block 1: Query Module Hardening tests (Issue #5184)")

    add_executable(test_phase4_query_hardening_focused
        test_phase4_query_hardening.cpp
    )

    target_include_directories(test_phase4_query_hardening_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_phase4_query_hardening_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_phase4_query_hardening_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME Phase4QueryHardeningTests
        TARGET test_phase4_query_hardening_focused
        TIER unit
        TIMEOUT 120
        LABELS phase4 timeout concurrency prealloc federation audit hardening
    )

    message(STATUS "  Phase4 Hardening: timeout enforcement, federation audit, container pre-alloc, concurrency safety")
else()
    message(STATUS "Phase 4 Block 1 hardening tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_phase4_query_hardening.cpp")
endif()

# Query Module Phase 2: Optimizer and Planning Hardening — Regression Test Suite
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_query_optimizer_regression.cpp")
    message(STATUS "Adding Query Module Phase 2: Optimizer Regression tests (33 deterministic cases)")

    add_executable(test_query_optimizer_regression_focused
        test_query_optimizer_regression.cpp
        ${THEMIS_ROOT_DIR}/src/query/optimizer_cost_model_enhancements.cpp
    )

    target_include_directories(test_query_optimizer_regression_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_query_optimizer_regression_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
        OpenSSL::Crypto
    )

    target_compile_definitions(test_query_optimizer_regression_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME QueryOptimizerRegressionTests
        TARGET test_query_optimizer_regression_focused
        TIER unit
        TIMEOUT 120
        LABELS phase2 optimizer regression histogram correlation estimate validation performance-gates
    )

    message(STATUS "  QueryOptimizer Regression: cardinality estimation (RC1-10), cost model (CM1-8), plan cache (PC1-8), estimate validation (EV1-4), performance gates (PG1-2), stress tests (STRESS1)")
else()
    message(STATUS "Query Optimizer Regression tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_query_optimizer_regression.cpp")
endif()

# Phase 2 Agent 2: Query Optimizer Scope Bounds and Validation Tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_query_optimizer_scope_bounds.cpp")
    message(STATUS "Adding Phase 2 Agent 2: Query Optimizer Scope Bounds tests (25 deterministic cases)")

    add_executable(test_query_optimizer_scope_bounds_focused
        test_query_optimizer_scope_bounds.cpp
    )

    target_include_directories(test_query_optimizer_scope_bounds_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_query_optimizer_scope_bounds_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_query_optimizer_scope_bounds_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME QueryOptimizerScopeBoundsTests
        TARGET test_query_optimizer_scope_bounds_focused
        TIER unit
        TIMEOUT 120
        LABELS phase2 optimizer scope-bounds scope-mismatch federation isolation validation
    )

    message(STATUS "  Phase 2 Agent 2: Scope bounds (SB1-6), result boundaries (RB7-13), federation isolation (FS14-17), edge cases (EC18-22), concurrency (C23), integration (I24-25)")
else()
    message(STATUS "Phase 2 Agent 2: Query Optimizer Scope Bounds tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_query_optimizer_scope_bounds.cpp")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_geospatial_optimizer.cpp")
    message(STATUS "Adding Query: Geospatial Optimizer Phase 6C tests (GEO_OPT_01-40)")

    add_executable(test_geospatial_optimizer_focused
        test_geospatial_optimizer.cpp
    )

    target_include_directories(test_geospatial_optimizer_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_geospatial_optimizer_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_geospatial_optimizer_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME GeospatialOptimizerTests
        TARGET test_geospatial_optimizer_focused
        TIER unit
        TIMEOUT 120
        LABELS geospatial optimizer phase6c
    )

    message(STATUS "  Geospatial Optimizer: cost model, hints, index selection, query rewrite (GEO_OPT_01-40)")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_geospatial_phase2_integration.cpp")
    message(STATUS "Adding Query: Geospatial Phase 2 Integration tests (INT_GEO_01-10)")

    add_executable(test_geospatial_phase2_integration_focused
        test_geospatial_phase2_integration.cpp
    )

    target_include_directories(test_geospatial_phase2_integration_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_geospatial_phase2_integration_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_geospatial_phase2_integration_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME GeospatialPhase2IntegrationTests
        TARGET test_geospatial_phase2_integration_focused
        TIER unit
        TIMEOUT 120
        LABELS geospatial integration phase6c
    )

    message(STATUS "  Geospatial Phase 2 Integration: end-to-end spatial query optimization (INT_GEO_01-10)")
endif()

# Phase 6 FTS parser-layer focused tests (FTS-P-01..FTS-P-16)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_aql_fts_parser_focused.cpp")
    add_executable(test_aql_fts_parser_focused
        test_aql_fts_parser_focused.cpp
    )

    target_include_directories(test_aql_fts_parser_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_aql_fts_parser_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_aql_fts_parser_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE query
        NAME FtsParserTests
        TARGET test_aql_fts_parser_focused
        TIER unit
        TIMEOUT 120
        LABELS fts search parser phase6
    )

    message(STATUS "  FTS Parser: SEARCH clause tokens, AST nodes, modifiers (FTS-P-01..16)")
endif()

file(GLOB QUERY_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_query_*.cpp"
)

foreach(_src IN LISTS QUERY_AUTOGEN_PREFIX_TEST_SOURCES)
    # Skip sources already wired by explicit focused targets in this module file.
    set(_src_already_registered FALSE)
    get_property(_query_dir_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_query_tgt IN LISTS _query_dir_targets)
        get_target_property(_query_tgt_sources ${_query_tgt} SOURCES)
        if(_query_tgt_sources)
            list(FIND _query_tgt_sources "${_src}" _query_src_idx)
            if(NOT _query_src_idx EQUAL -1)
                set(_src_already_registered TRUE)
                break()
            endif()
        endif()
    endforeach()
    if(_src_already_registered)
        continue()
    endif()

    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_query_${_stem}_autofocused")
    set(_ctest "${_stem}_query_AutoFocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    if("${_stem}" STREQUAL "test_query_phase_a_gap_fixes")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/query/tensor_aware_query_optimizer.cpp
        )
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE query
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS query autogen
    )
endforeach()