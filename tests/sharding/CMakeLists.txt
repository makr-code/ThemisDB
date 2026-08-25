# Sharding module test registration (large extraction from root tests/CMakeLists.txt)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_wal_logging_helper.cpp")
    message(STATUS "Adding Sharding: WALLoggingHelper focused tests (Issue #5375)")
    add_executable(test_wal_logging_helper_focused ../test_wal_logging_helper.cpp)
    target_include_directories(test_wal_logging_helper_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_wal_logging_helper_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_wal_logging_helper_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME WALLoggingHelperFocusedTests TARGET test_wal_logging_helper_focused TIER unit TIMEOUT 60 LABELS wal 2pc helper utility)
    message(STATUS "  WALLoggingHelper: buildEntry fields/timestamp, appendEntry null-noop, live WAL write, exception swallowing, sequential LSN ordering")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_two_phase_commit.cpp")
    message(STATUS "Adding Sharding: TwoPhaseCommit focused tests")
    add_executable(test_two_phase_commit_focused ../test_two_phase_commit.cpp)
    target_include_directories(test_two_phase_commit_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_two_phase_commit_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_two_phase_commit_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME TwoPhaseCommitFocusedTests TARGET test_two_phase_commit_focused TIER unit TIMEOUT 90 LABELS 2pc distributed transaction)
    message(STATUS "  TwoPhaseCommit: prepare/commit/abort, coordinator crash recovery")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_sharding_core.cpp")
    message(STATUS "Adding Sharding: ShardingCore focused tests")
    add_executable(test_sharding_core_focused ../test_sharding_core.cpp)
    target_include_directories(test_sharding_core_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_sharding_core_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_sharding_core_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ShardingCoreFocusedTests TARGET test_sharding_core_focused TIER unit TIMEOUT 90 LABELS core raft consensus)
    message(STATUS "  ShardingCore: Raft consensus, shard routing, rebalancing")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_replication_coordinator_focused.cpp")
    message(STATUS "Adding Sharding: ReplicationCoordinator focused tests (QW-28)")
    add_executable(test_replication_coordinator_focused ../test_replication_coordinator_focused.cpp)
    target_include_directories(test_replication_coordinator_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_replication_coordinator_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_replication_coordinator_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ReplicationCoordinatorFocusedTests TARGET test_replication_coordinator_focused TIER unit TIMEOUT 30 LABELS replication fail-closed)
    message(STATUS "  ReplicationCoordinator: Fail-closed guard tests")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_urn_resolver_focused.cpp")
    message(STATUS "Adding Sharding: URNResolver focused tests (QW-29)")
    add_executable(test_urn_resolver_focused ../test_urn_resolver_focused.cpp)
    target_include_directories(test_urn_resolver_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_urn_resolver_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_urn_resolver_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME URNResolverFocusedTests TARGET test_urn_resolver_focused TIER unit TIMEOUT 30 LABELS urn_resolver fail-closed)
    message(STATUS "  URNResolver: Fail-closed guard tests")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_shard_router_focused.cpp")
    message(STATUS "Adding Sharding: ShardRouter focused tests (QW-30)")
    add_executable(test_shard_router_focused ../test_shard_router_focused.cpp)
    target_include_directories(test_shard_router_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_shard_router_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads nlohmann_json::nlohmann_json)
    target_compile_definitions(test_shard_router_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ShardRouterFocusedTests TARGET test_shard_router_focused TIER unit TIMEOUT 30 LABELS shard_router fail-closed)
    message(STATUS "  ShardRouter: Fail-closed guard tests")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_redundancy_strategy_focused.cpp")
    message(STATUS "Adding Sharding: RedundancyStrategy focused tests (W2-S02)")
    add_executable(test_redundancy_strategy_focused ../test_redundancy_strategy_focused.cpp)
    target_include_directories(test_redundancy_strategy_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_redundancy_strategy_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_redundancy_strategy_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME RedundancyStrategyFocusedTests TARGET test_redundancy_strategy_focused TIER unit TIMEOUT 30 LABELS redundancy input-validation distributed-consistency W2-S02)
    message(STATUS "  RedundancyStrategy: Input validation & consensus hardening (W2-S02)")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_stream_protocol_focused.cpp")
    message(STATUS "Adding Sharding: StreamProtocol focused tests (W2-S03)")
    add_executable(test_stream_protocol_focused ../test_stream_protocol_focused.cpp)
    target_include_directories(test_stream_protocol_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_stream_protocol_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_stream_protocol_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME StreamProtocolFocusedTests TARGET test_stream_protocol_focused TIER unit TIMEOUT 30 LABELS stream-protocol wal-applier chunk-metadata LSN-validation W2-S03)
    message(STATUS "  StreamProtocol: Chunk metadata & LSN validation hardening (W2-S03)")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_cross_shard_transaction_focused.cpp")
    message(STATUS "Adding Sharding: CrossShardTransaction focused tests (W2-S04)")
    add_executable(test_cross_shard_transaction_focused ../test_cross_shard_transaction_focused.cpp)
    target_include_directories(test_cross_shard_transaction_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_cross_shard_transaction_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads ${TEST_GMOCK_LIB})
    target_compile_definitions(test_cross_shard_transaction_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME CrossShardTransactionFocusedTests TARGET test_cross_shard_transaction_focused TIER unit TIMEOUT 30 LABELS cross-shard-transaction precondition-validation 2pc W2-S04)
    message(STATUS "  CrossShardTransaction: Input validation & precondition hardening (W2-S04)")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_cross_shard_fk_validator.cpp")
    message(STATUS "Adding Sharding: CrossShardFKValidator tests (Issue #5390)")
    add_executable(test_cross_shard_fk_validator ../test_cross_shard_fk_validator.cpp)
    target_include_directories(test_cross_shard_fk_validator PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_cross_shard_fk_validator PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads nlohmann_json::nlohmann_json)
    target_compile_definitions(test_cross_shard_fk_validator PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME CrossShardFKValidatorTests TARGET test_cross_shard_fk_validator TIER unit TIMEOUT 60 LABELS cross-shard-fk referential-integrity 2pc fk-validator issue-5390)
    message(STATUS "  CrossShardFKValidator: FK registration, validation, 2PC integration, fail-closed, partition (Issue #5390)")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_gossip_config_manager_focused.cpp")
    message(STATUS "Adding Sharding: GossipConfigManager focused tests (W2-S05)")
    add_executable(test_gossip_config_manager_focused ../test_gossip_config_manager_focused.cpp)
    target_include_directories(test_gossip_config_manager_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_gossip_config_manager_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads ${TEST_GMOCK_LIB})
    target_compile_definitions(test_gossip_config_manager_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME GossipConfigManagerFocusedTests TARGET test_gossip_config_manager_focused TIER unit TIMEOUT 30 LABELS gossip-config config-validation resource-metrics W2-S05)
    message(STATUS "  GossipConfigManager: Config message & resource metrics validation (W2-S05)")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_shard_repair_engine_focused.cpp")
    message(STATUS "Adding Sharding: ShardRepairEngine focused tests (QW-31)")
    add_executable(test_shard_repair_engine_focused ../test_shard_repair_engine_focused.cpp)
    target_include_directories(test_shard_repair_engine_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_shard_repair_engine_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_shard_repair_engine_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ShardRepairEngineTests TARGET test_shard_repair_engine_focused TIER unit TIMEOUT 30 LABELS repair_engine fail-closed)
    message(STATUS "  ShardRepairEngine: Fail-closed guard tests")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_consistent_hash_distribution.cpp")
    message(STATUS "Adding Sharding: ConsistentHashDistribution focused tests")
    add_executable(test_consistent_hash_distribution_focused ../test_consistent_hash_distribution.cpp)
    target_include_directories(test_consistent_hash_distribution_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_consistent_hash_distribution_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_consistent_hash_distribution_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ConsistentHashDistributionFocusedTests TARGET test_consistent_hash_distribution_focused TIER unit TIMEOUT 90 LABELS distribution hash)
    message(STATUS "  ConsistentHashDistribution: strict distribution and migration checks")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_sharding_repair.cpp")
    message(STATUS "Adding Sharding: RS Repair Engine Parallelisation focused tests")
    add_executable(test_rs_repair_parallelisation_focused ../test_sharding_repair.cpp)
    target_include_directories(test_rs_repair_parallelisation_focused PRIVATE ${THEMIS_ROOT_DIR}/include ${THEMIS_ROOT_DIR}/src ${CMAKE_BINARY_DIR}/proto_generated)
    target_link_libraries(test_rs_repair_parallelisation_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog nlohmann_json::nlohmann_json Threads::Threads)
    target_compile_definitions(test_rs_repair_parallelisation_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME RSRepairParallelisationFocusedTests TARGET test_rs_repair_parallelisation_focused TIER unit TIMEOUT 120 LABELS repair reed-solomon parallelisation slo iops gpu)
    message(STATUS "  RSRepairParallelisation: parallel scan bands, SLO progress, IOPS throttle, GPU flag")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_sharding_chaos.cpp")
    message(STATUS "Adding Sharding: Chaos Engineering focused tests")
    add_executable(test_sharding_chaos_focused ../test_sharding_chaos.cpp)
    target_include_directories(test_sharding_chaos_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_sharding_chaos_focused PRIVATE ${TEST_LIBS} spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_sharding_chaos_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ShardingChaosFocusedTests TARGET test_sharding_chaos_focused TIER unit TIMEOUT 120 LABELS chaos resilience partition split-brain)
    message(STATUS "  ShardingChaos: network partitions, cascading failures, split-brain, random failure injection")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_sharding_chaos_batch_a9.cpp")
    message(STATUS "Adding Sharding: Wave A Batch A-9 chaos tests")
    add_executable(module_sharding_test_sharding_chaos_batch_a9_focused
        test_sharding_chaos_batch_a9.cpp
        ${THEMIS_ROOT_DIR}/tests/utils/fault_injector.cpp)
    target_include_directories(module_sharding_test_sharding_chaos_batch_a9_focused PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests)
    target_link_libraries(module_sharding_test_sharding_chaos_batch_a9_focused PRIVATE
        GTest::gtest GTest::gtest_main Threads::Threads)
    target_compile_definitions(module_sharding_test_sharding_chaos_batch_a9_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(
        MODULE sharding
        NAME ShardingChaosBatchA9FocusedTests
        TARGET module_sharding_test_sharding_chaos_batch_a9_focused
        TIER unit
        TIMEOUT 120
        LABELS sharding chaos batch-a9)
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_sharding_e2e.cpp")
    message(STATUS "Adding Sharding: E2E focused tests")
    add_executable(test_sharding_e2e_focused ../test_sharding_e2e.cpp)
    target_include_directories(test_sharding_e2e_focused PRIVATE ${THEMIS_ROOT_DIR}/include ${THEMIS_ROOT_DIR}/src)
    target_link_libraries(test_sharding_e2e_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog nlohmann_json::nlohmann_json Threads::Threads)
    target_compile_definitions(test_sharding_e2e_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_test(MODULE sharding NAME ShardingE2EFocusedTests TARGET test_sharding_e2e_focused TIER integration KIND focused TIMEOUT 120 LABELS e2e crud scatter-gather cross-shard migration focused)
    message(STATUS "  ShardingE2E: CRUD workflow, scatter-gather, cross-shard joins, migration simulation")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_sharding_gossip.cpp")
    message(STATUS "Adding Sharding: Gossip Protocol focused tests")
    add_executable(test_sharding_gossip_focused ../test_sharding_gossip.cpp)
    target_include_directories(test_sharding_gossip_focused PRIVATE ${THEMIS_ROOT_DIR}/include ${THEMIS_ROOT_DIR}/src)
    target_link_libraries(test_sharding_gossip_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_sharding_gossip_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ShardingGossipFocusedTests TARGET test_sharding_gossip_focused TIER unit TIMEOUT 60 LABELS gossip consensus protocol)
    message(STATUS "  ShardingGossip: GossipConfig defaults, enabled flag")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_sharding_integration.cpp")
    message(STATUS "Adding Sharding: Integration focused tests")
    add_executable(test_sharding_integration_focused ../test_sharding_integration.cpp)
    target_include_directories(test_sharding_integration_focused PRIVATE ${THEMIS_ROOT_DIR}/include ${THEMIS_ROOT_DIR}/src)
    target_link_libraries(test_sharding_integration_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog nlohmann_json::nlohmann_json Threads::Threads)
    target_compile_definitions(test_sharding_integration_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ShardingIntegrationFocusedTests TARGET test_sharding_integration_focused TIER unit TIMEOUT 120 LABELS integration urn consistent-hash router remote-executor)
    message(STATUS "  ShardingIntegration: URN resolver, consistent hash, shard router, remote executor")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_sharding_interfaces.cpp")
    message(STATUS "Adding Sharding: Public Interfaces focused tests")
    add_executable(test_sharding_interfaces_focused ../test_sharding_interfaces.cpp)
    target_include_directories(test_sharding_interfaces_focused PRIVATE ${THEMIS_ROOT_DIR}/include ${THEMIS_ROOT_DIR}/src)
    target_link_libraries(test_sharding_interfaces_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog nlohmann_json::nlohmann_json Threads::Threads)
    target_compile_definitions(test_sharding_interfaces_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ShardingInterfacesFocusedTests TARGET test_sharding_interfaces_focused TIER unit TIMEOUT 90 LABELS interfaces router rebalancer coordinator snapshot hash-ring)
    message(STATUS "  ShardingInterfaces: IShardRouter, IAdaptiveRebalancer, IDistributedTxCoordinator, IRaftSnapshotManager, IConsistentHashRing, ICrossShardQueryRouter")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_sharding_operational_metrics.cpp")
    message(STATUS "Adding Sharding: Operational Metrics focused tests")
    add_executable(test_sharding_operational_metrics_focused ../test_sharding_operational_metrics.cpp)
    target_include_directories(test_sharding_operational_metrics_focused PRIVATE ${THEMIS_ROOT_DIR}/include ${THEMIS_ROOT_DIR}/src)
    target_link_libraries(test_sharding_operational_metrics_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_sharding_operational_metrics_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ShardingOperationalMetricsFocusedTests TARGET test_sharding_operational_metrics_focused TIER unit TIMEOUT 60 LABELS observability metrics health)
    message(STATUS "  ShardingOperationalMetrics: per-shard metrics, cluster health, network traffic, resource usage")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_sharding_uncovered.cpp")
    message(STATUS "Adding Sharding: Uncovered Components focused tests")
    add_executable(test_sharding_uncovered_focused ../test_sharding_uncovered.cpp)
    target_include_directories(test_sharding_uncovered_focused PRIVATE ${THEMIS_ROOT_DIR}/include ${THEMIS_ROOT_DIR}/src)
    target_link_libraries(test_sharding_uncovered_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_sharding_uncovered_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ShardingUncoveredFocusedTests TARGET test_sharding_uncovered_focused TIER unit TIMEOUT 60 LABELS orphan-detector load-detector)
    message(STATUS "  ShardingUncovered: OrphanDetector, ShardLoadDetector")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_epoch_fencing.cpp")
    message(STATUS "Adding Sharding: Epoch Fencing focused tests (Phase 4.1)")
    add_executable(test_epoch_fencing_focused ../test_epoch_fencing.cpp ${THEMIS_ROOT_DIR}/src/sharding/epoch_fencing.cpp)
    target_include_directories(test_epoch_fencing_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_epoch_fencing_focused PRIVATE ${TEST_LIBS} spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_epoch_fencing_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME EpochFencingFocusedTests TARGET test_epoch_fencing_focused TIER unit TIMEOUT 60 LABELS phase4.1 fencing)
    message(STATUS "  EpochFencing: EpochFencingManager, LeaseManager, NullStonithProvider")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_sharding_phase5.cpp")
    message(STATUS "Adding Sharding: Phase 5 focused tests")
    add_executable(test_sharding_phase5_focused ../test_sharding_phase5.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/admin_api.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/hardware_migration_manager.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/raft_consensus.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/consistent_hash.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/shard_topology.cpp)
    target_include_directories(test_sharding_phase5_focused PRIVATE ${THEMIS_ROOT_DIR}/include ${THEMIS_ROOT_DIR}/src)
    target_link_libraries(test_sharding_phase5_focused PRIVATE ${TEST_LIBS} themis_core Threads::Threads)
    target_compile_definitions(test_sharding_phase5_focused PRIVATE THEMIS_TEST_BUILD=1)
    if(nlohmann_json_FOUND)
        target_link_libraries(test_sharding_phase5_focused PRIVATE nlohmann_json::nlohmann_json)
    endif()
    set_target_properties(test_sharding_phase5_focused PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
    themis_register_module_focused_test(MODULE sharding NAME ShardingPhase5Tests TARGET test_sharding_phase5_focused TIER unit TIMEOUT 60 LABELS phase5 hardware-migration raft drain admin-api)
    message(STATUS "  SHP5-01..04: Admin API POST /api/v1/shards/{id}/migrate-hardware")
    message(STATUS "  SHP5-05..08: RaftConsensus::updatePeerAddress()")
    message(STATUS "  SHP5-09..12: HardwareMigrationManager drain-period enforcement")
elseif(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_sharding_phase5.cpp")
    message(STATUS "Sharding Phase 5 tests not found")
endif()

# ============================================================================
# Phase 5 LoRA Artifact Distribution Tests (Issue #5418)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_sharding_phase5_lora_distribution.cpp")
    message(STATUS "Adding Sharding: Phase 5 LoRA Artifact Distribution tests (#5418)")
    add_executable(test_sharding_phase5_lora_distribution
        test_sharding_phase5_lora_distribution.cpp)
    target_include_directories(test_sharding_phase5_lora_distribution PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src)
    target_link_libraries(test_sharding_phase5_lora_distribution PRIVATE
        ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_sharding_phase5_lora_distribution PRIVATE
        THEMIS_TEST_BUILD=1)
    if(nlohmann_json_FOUND)
        target_link_libraries(test_sharding_phase5_lora_distribution PRIVATE
            nlohmann_json::nlohmann_json)
    endif()
    set_target_properties(test_sharding_phase5_lora_distribution PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
    themis_register_module_focused_test(
        MODULE  sharding
        NAME    ShardingPhase5LoRADistributionTests
        TARGET  test_sharding_phase5_lora_distribution
        TIER    unit
        TIMEOUT 60
        LABELS  phase5 lora-distribution sharding merkle receipt chain recovery artifact)
    message(STATUS "  P5-L01..P5-L03: LoRAPackageRef validation")
    message(STATUS "  P5-L04..P5-L10: InMemoryAdapterDistributionStore CRUD")
    message(STATUS "  P5-L11..P5-L16: DefaultMerkleProofEngine tree & proof verification")
    message(STATUS "  P5-L17..P5-L22: Distribution manager distribute/confirm/fail/recover")
    message(STATUS "  P5-L23..P5-L24: ShardDistributionSnapshot & recovery ordering")
    message(STATUS "  P5-L25:         End-to-end artifact receipt chain (Send/Confirm/Recover)")
endif()

# ============================================================================
# RAID-Paxos and Dual-Consensus Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_raid_paxos.cpp")
    message(STATUS "Adding Sharding: RAID-Paxos consensus tests")
    add_executable(test_raid_paxos test_raid_paxos.cpp)
    target_include_directories(test_raid_paxos PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_raid_paxos PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads ${TEST_GMOCK_LIB})
    target_compile_definitions(test_raid_paxos PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME RAIDPaxosConsensusTests TARGET test_raid_paxos TIER unit TIMEOUT 60 LABELS raid paxos consensus quorum sharding)
    message(STATUS "  RAID-Paxos: Configuration, quorum calculation, failure tolerance tests")
endif()

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_dual_consensus.cpp")
    message(STATUS "Adding Sharding: Dual-Consensus orchestrator tests")
    add_executable(test_dual_consensus test_dual_consensus.cpp)
    target_include_directories(test_dual_consensus PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_dual_consensus PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads ${TEST_GMOCK_LIB})
    target_compile_definitions(test_dual_consensus PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME DualConsensusOrchestratorTests TARGET test_dual_consensus TIER unit TIMEOUT 60 LABELS dual-consensus converged storage-inference grounding audit)
    message(STATUS "  Dual-Consensus: Cross-layer coordination, version tracking, conflict resolution")
endif()

# ============================================================================
# Redundancy Consistency Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_redundancy_consistency.cpp")
    message(STATUS "Adding Sharding: Redundancy consistency tests")
    add_executable(test_redundancy_consistency test_redundancy_consistency.cpp)
    target_include_directories(test_redundancy_consistency PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_redundancy_consistency PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads ${TEST_GMOCK_LIB})
    target_compile_definitions(test_redundancy_consistency PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME RedundancyConsistencyTests TARGET test_redundancy_consistency TIER unit TIMEOUT 60 LABELS redundancy consistency version-tracking conflict-resolution)
    message(STATUS "  RedundancyConsistency: Version-aware merging, conflict resolution, consistency checks")
endif()

# ============================================================================
# Phase 6 Sharding Hardening Tests (P6-01 2PC/3PC + P6-02 Failover/Recovery)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_sharding_phase6_hardening.cpp")
    message(STATUS "Adding Sharding: Phase 6 Hardening tests (P6-01/P6-02)")
    add_executable(test_sharding_phase6_hardening test_sharding_phase6_hardening.cpp)
    target_include_directories(test_sharding_phase6_hardening PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(test_sharding_phase6_hardening PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(test_sharding_phase6_hardening PRIVATE THEMIS_TEST_BUILD=1)
    set_target_properties(test_sharding_phase6_hardening PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
    themis_register_module_focused_test(
        MODULE  sharding
        NAME    ShardingPhase6HardeningTests
        TARGET  test_sharding_phase6_hardening
        TIER    unit
        TIMEOUT 120
        LABELS  sharding hardening phase6 release_critical sharding_p6
    )
    message(STATUS "  P6-01 TXC-01..TXC-32: 2PC/3PC consistency verification (32 tests)")
    message(STATUS "  P6-02 FLR-01..FLR-20: failover logic and recovery-path hardening (20 tests)")
endif()

# ============================================================================
# Phase 6 / P6-03 — Wave-8 Sharding Fault Injection Tests
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_sharding_p6_fault_injection.cpp")
    message(STATUS "Adding Sharding: P6-03 Wave-8 Fault Injection tests")
    add_executable(test_sharding_p6_fault_injection test_sharding_p6_fault_injection.cpp)
    target_include_directories(test_sharding_p6_fault_injection PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(test_sharding_p6_fault_injection PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(test_sharding_p6_fault_injection PRIVATE THEMIS_TEST_BUILD=1)
    set_target_properties(test_sharding_p6_fault_injection PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
    themis_register_module_focused_test(
        MODULE  sharding
        NAME    ShardingP6FaultInjectionTests
        TARGET  test_sharding_p6_fault_injection
        TIER    unit
        TIMEOUT 120
        LABELS  sharding fault_injection phase6 release_critical sharding_p6
    )
    message(STATUS "  P6-03 FI-01..FI-15: network partition scenarios (15 tests)")
    message(STATUS "  P6-03 FI-16..FI-25: coordinator failure scenarios (10 tests)")
    message(STATUS "  P6-03 FI-26..FI-40: cascade / multi-failure scenarios (15 tests)")
endif()

# ============================================================================
# Phase 1 / Phase 4 — Sharding Contract Hardening Tests (SCR-01..SCR-16)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_sharding_contract_hardening_focused.cpp")
    message(STATUS "Adding Sharding: Contract Hardening tests (SCR-01..SCR-16)")
    add_executable(test_sharding_contract_hardening_focused test_sharding_contract_hardening_focused.cpp)
    target_include_directories(test_sharding_contract_hardening_focused PRIVATE ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_sharding_contract_hardening_focused PRIVATE ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_sharding_contract_hardening_focused PRIVATE THEMIS_TEST_BUILD=1)
    themis_register_module_focused_test(MODULE sharding NAME ShardingContractHardeningFocusedTests TARGET test_sharding_contract_hardening_focused TIER unit TIMEOUT 120 LABELS sharding contract hardening release_critical)
    message(STATUS "  SCR-01..04: Routing contract (consistent hash, fallback)")
    message(STATUS "  SCR-05..08: 2PC contract (prepare/commit/abort, IN_DOUBT)")
    message(STATUS "  SCR-09..12: WAL contract (ordering, idempotent replay, corruption)")
    message(STATUS "  SCR-13..16: Error contract (QUORUM_LOST, MIGRATION_CONFLICT, fail-closed)")
endif()

# ============================================================================
# Phase C rollout gate — multi-shard exact under shard failure injection
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_sharding_multishard_exact.cpp")
    message(STATUS "Adding Sharding: Multi-shard exact Phase C gate")
    add_executable(test_sharding_multishard_exact
        test_sharding_multishard_exact.cpp)
    target_include_directories(test_sharding_multishard_exact PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src)
    target_link_libraries(test_sharding_multishard_exact PRIVATE
        ${TEST_LIBS} themis_core spdlog::spdlog nlohmann_json::nlohmann_json Threads::Threads)
    target_compile_definitions(test_sharding_multishard_exact PRIVATE
        THEMIS_TEST_BUILD=1)
    set_target_properties(test_sharding_multishard_exact PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON)
    themis_register_module_focused_test(
        MODULE  sharding
        NAME    ShardingMultiShardExactPhaseCGate
        TARGET  test_sharding_multishard_exact
        TIER    unit
        TIMEOUT 120
        LABELS  sharding retrieval multi-shard exact release_critical phase_c_gate)
    message(STATUS "  Phase C gate: exact shortest-path + neighbor fallback under shard failure")
endif()

# ============================================================================
# Wave 1 CRITICAL gap closure regression tests (WV1-BRC / EXD / NTO / ITR /
#   DBL / CLO / UNI / TODO / SUM)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_sharding_wave1_critical_closure.cpp")
    message(STATUS "Adding Sharding: Wave 1 CRITICAL gap closure regression tests")
    add_executable(test_sharding_wave1_critical_closure
        test_sharding_wave1_critical_closure.cpp)
    target_include_directories(test_sharding_wave1_critical_closure PRIVATE
        ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_sharding_wave1_critical_closure PRIVATE
        ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_sharding_wave1_critical_closure PRIVATE
        THEMIS_TEST_BUILD=1)
    set_target_properties(test_sharding_wave1_critical_closure PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON)
    themis_register_module_focused_test(
        MODULE  sharding
        NAME    ShardingWave1CriticalClosureTests
        TARGET  test_sharding_wave1_critical_closure
        TIER    unit
        TIMEOUT 60
        LABELS  sharding wave1 critical-closure release_critical)
    message(STATUS "  WV1-BRC-01..04: braces_imbalance — all 12 flagged files balanced")
    message(STATUS "  WV1-EXD-01..02: exception_in_destructor — noexcept contracts")
    message(STATUS "  WV1-NTO-01..02: no_timeout — timed-lock enforcement")
    message(STATUS "  WV1-ITR-01..02: iterator_invalidation — bounded iteration")
    message(STATUS "  WV1-DBL-01..02: db_connection_leak — RAII ownership lifecycle")
    message(STATUS "  WV1-CLO-01..03: circular_lock_ordering — std::lock canonical order")
    message(STATUS "  WV1-UNI-01..02: uninitialized_access — value-init at declaration")
    message(STATUS "  WV1-TODO-01..02: todo_as_productionlogic — no bare TODO in critical paths")
    message(STATUS "  WV1-SUM-01: 0 open CRITICAL gaps at Wave 1 closure")
endif()

# ============================================================================
# Phase C prerequisite gates — Thread-Safety / Lock-Order / Consensus (TSO/LKO/CCR)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_sharding_thread_safety_lock_order_focused.cpp")
    message(STATUS "Adding Sharding: Thread-Safety / Lock-Order / Consensus tests (TSO/LKO/CCR)")
    add_executable(test_sharding_thread_safety_lock_order_focused
        test_sharding_thread_safety_lock_order_focused.cpp)
    target_include_directories(test_sharding_thread_safety_lock_order_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include)
    target_link_libraries(test_sharding_thread_safety_lock_order_focused PRIVATE
        ${TEST_LIBS} themis_core spdlog::spdlog Threads::Threads)
    target_compile_definitions(test_sharding_thread_safety_lock_order_focused PRIVATE
        THEMIS_TEST_BUILD=1)
    set_target_properties(test_sharding_thread_safety_lock_order_focused PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON)
    themis_register_module_focused_test(
        MODULE  sharding
        NAME    ShardingThreadSafetyLockOrderFocusedTests
        TARGET  test_sharding_thread_safety_lock_order_focused
        TIER    unit
        TIMEOUT 120
        LABELS  sharding thread-safety lock-order consensus release_critical phase_c_gate)
    message(STATUS "  TSO-01..08: Thread-safety correctness (concurrent read/write, atomics)")
    message(STATUS "  LKO-01..06: Lock-ordering correctness (no deadlock, ABBA prevention)")
    message(STATUS "  CCR-01..06: Consensus coordination robustness (timeout, quorum-loss, retry)")
endif()



# AUTOGEN PREFIX BLOCK: sharding
file(GLOB SHARDING_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_sharding_*.cpp"
)

foreach(_src IN LISTS SHARDING_AUTOGEN_PREFIX_TEST_SOURCES)
    # Skip sources already wired by explicit focused targets in this module file.
    set(_src_already_registered FALSE)
    get_property(_sharding_dir_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_sharding_tgt IN LISTS _sharding_dir_targets)
        get_target_property(_sharding_tgt_sources ${_sharding_tgt} SOURCES)
        if(_sharding_tgt_sources)
            list(FIND _sharding_tgt_sources "${_src}" _sharding_src_idx)
            if(NOT _sharding_src_idx EQUAL -1)
                set(_src_already_registered TRUE)
                break()
            endif()
        endif()
    endforeach()
    if(_src_already_registered)
        continue()
    endif()

    if(_src MATCHES ".*/test_sharding_chaos_batch_a9\\.cpp$")
        continue()
    endif()

    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_sharding_${_stem}_autofocused")
    set(_ctest "${_stem}_sharding_AutoFocusedTests")

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

    themis_register_module_focused_test(
        MODULE sharding
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS sharding autogen
    )
endforeach()