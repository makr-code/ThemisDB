# Transaction module test registration (pilot extraction from root tests/CMakeLists.txt)

# Transaction Auditor Focused Tests (v1.8.0)
# AC-1..25: enableAuditing, record, queryAuditLog (filters, limit, sort),
#           size, clear, exportToKafka/S3, concurrent record()
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_transaction_auditor.cpp")
    message(STATUS "Adding Transaction Auditor focused tests (transaction, v1.8.0)")

    add_executable(test_transaction_auditor_focused
        test_transaction_auditor.cpp
        ${THEMIS_ROOT_DIR}/src/transaction/transaction_auditor.cpp
    )

    target_include_directories(test_transaction_auditor_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_transaction_auditor_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_auditor_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE transaction
        NAME TransactionAuditorFocusedTests
        TARGET test_transaction_auditor_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS audit audit-trail v1.8.0
    )

    message(STATUS "  TransactionAuditor: enableAuditing, record, queryAuditLog, size, clear, exportToKafka/S3, concurrent")
else()
    message(STATUS "Transaction Auditor focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_transaction_auditor.cpp")
endif()

# Transaction Batcher Focused Tests (v1.8.0)
# AC-1..26: BatchConfig, window clamping, submitAsync, flush, per-table policies,
#           adaptive sizing, fair FIFO scheduling, stats, concurrent submitters
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_transaction_batcher.cpp")
    message(STATUS "Adding Transaction Batcher focused tests (transaction, v1.8.0)")

    add_executable(test_transaction_batcher_focused
        test_transaction_batcher.cpp
        ${THEMIS_ROOT_DIR}/src/transaction/transaction_batcher.cpp
    )

    target_include_directories(test_transaction_batcher_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_transaction_batcher_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_batcher_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE transaction
        NAME TransactionBatcherFocusedTests
        TARGET test_transaction_batcher_focused
        TIER unit
        KIND focused
        TIMEOUT 120
        LABELS batching coalescing write-batching adaptive v1.8.0
    )

    message(STATUS "  TransactionBatcher: BatchConfig, submitAsync, flush, per-table policies, adaptive sizing, FIFO scheduling, stats, concurrent submitters")
else()
    message(STATUS "Transaction Batcher focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_transaction_batcher.cpp")
endif()

# Transaction Manager Focused Tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_transaction_manager.cpp")
    message(STATUS "Adding Transaction Manager focused tests")

    add_executable(test_transaction_manager_focused
        test_transaction_manager.cpp
    )

    target_link_libraries(test_transaction_manager_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_manager_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE transaction
        NAME TransactionManagerFocusedTests
        TARGET test_transaction_manager_focused
        TIER unit
        KIND focused
        TIMEOUT 120
        LABELS mvcc acid occ explain
    )

    message(STATUS "  TransactionManager: ACID commit/rollback, graph/vector ops, isolation, statistics, explain")
else()
    message(STATUS "Transaction Manager focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_transaction_manager.cpp")
endif()

# Adaptive Deadlock Prevention Focused Tests (v1.9.0)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_adaptive_deadlock_prevention.cpp")
    message(STATUS "Adding Adaptive Deadlock Prevention focused tests")

    add_executable(test_adaptive_deadlock_prevention_focused
        test_adaptive_deadlock_prevention.cpp
    )

    target_include_directories(test_adaptive_deadlock_prevention_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_adaptive_deadlock_prevention_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_adaptive_deadlock_prevention_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE transaction
        NAME AdaptiveDeadlockPreventionFocusedTests
        TARGET test_adaptive_deadlock_prevention_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS deadlock adaptive predictor ml v1.9.0
    )

    message(STATUS "  AdaptiveDeadlockPrevention: DeadlockPredictor (record/predict/recommend/timeout), TransactionManager integration")
else()
    message(STATUS "Adaptive Deadlock Prevention tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_adaptive_deadlock_prevention.cpp")
endif()

# Distributed Transaction Coordinator (2PC) Focused Tests (v1.9.0)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_transaction_distributed_2pc.cpp")
    message(STATUS "Adding Distributed Transaction Coordinator (2PC) focused tests")

    add_executable(test_transaction_distributed_2pc_focused
        test_transaction_distributed_2pc.cpp
        ${THEMIS_ROOT_DIR}/src/transaction/distributed_transaction_manager.cpp
        ${THEMIS_ROOT_DIR}/src/utils/logger.cpp
    )

    target_include_directories(test_transaction_distributed_2pc_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_transaction_distributed_2pc_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_distributed_2pc_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE transaction
        NAME TransactionDistributed2PCFocusedTests
        TARGET test_transaction_distributed_2pc_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS distributed 2pc coordinator v1.9.0
    )

    message(STATUS "  Distributed2PC: 2PC coordinator, prepare/commit/abort, timeout, recovery, failure detection")
else()
    message(STATUS "Distributed Transaction Coordinator (2PC) tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_transaction_distributed_2pc.cpp")
endif()

# Transaction Isolation Levels Focused Tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_transaction_isolation_levels.cpp")
    message(STATUS "Adding Transaction Isolation Levels focused tests")

    add_executable(test_transaction_isolation_levels_focused
        test_transaction_isolation_levels.cpp
    )

    target_link_libraries(test_transaction_isolation_levels_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_isolation_levels_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE transaction
        NAME TransactionIsolationLevelsFocusedTests
        TARGET test_transaction_isolation_levels_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS isolation lock 2pl intent-lock
    )

    message(STATUS "  TransactionIsolationLevels: enum correctness, LockManager RC/RR/SZ compliance, 2PL, intent locks")
else()
    message(STATUS "Transaction Isolation Levels focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_transaction_isolation_levels.cpp")
endif()

# ITransactionCoordinator Interface Contract Tests (Issue #5374)
# ITC-1..39: CommitProtocol enum, CoordinatorCapabilities, TxnCoordinatorResult,
#            TxnCoordinatorOptions, lifecycle (begin/prepare/commit/abort),
#            state queries, WAL recovery, mockability, non-copyability.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_itransaction_coordinator.cpp")
    message(STATUS "Adding ITransactionCoordinator interface contract tests (Issue #5374)")

    add_executable(test_itransaction_coordinator_focused
        test_itransaction_coordinator.cpp
    )

    target_include_directories(test_itransaction_coordinator_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_itransaction_coordinator_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_itransaction_coordinator_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE transaction
        NAME ITransactionCoordinatorInterfaceTests
        TARGET test_itransaction_coordinator_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS coordinator interface 2pc saga percolator calvin strategy-pattern
    )

    message(STATUS "  ITransactionCoordinator: CommitProtocol, capabilities, lifecycle, state, recovery, mockability")
else()
    message(STATUS "ITransactionCoordinator interface tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_itransaction_coordinator.cpp")
endif()

# Cross-Coordinator WAL Recovery Contract Tests (Issue #5372)
# CWR-1..CWR-8: WALLoggingHelper null-safety, LSN round-trip, JSON payload
#               preservation, entry-type correctness, DistributedTransactionManager
#               recovery with no WAL and with an empty WAL.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_cross_coordinator_wal_recovery.cpp")
    message(STATUS "Adding Cross-Coordinator WAL Recovery contract tests (Issue #5372)")

    add_executable(test_cross_coordinator_wal_recovery_focused
        test_cross_coordinator_wal_recovery.cpp
        ${THEMIS_ROOT_DIR}/src/transaction/distributed_transaction_manager.cpp
        ${THEMIS_ROOT_DIR}/src/sharding/wal_manager.cpp
        ${THEMIS_ROOT_DIR}/src/utils/logger.cpp
    )

    target_include_directories(test_cross_coordinator_wal_recovery_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_cross_coordinator_wal_recovery_focused PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_cross_coordinator_wal_recovery_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE transaction
        NAME CrossCoordinatorWALRecoveryTests
        TARGET test_cross_coordinator_wal_recovery_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS coordinator wal recovery 2pc cross-coordinator cc-5
    )

    message(STATUS "  CrossCoordinatorWALRecovery: WALLoggingHelper null-safety, LSN, JSON payload, DTM recovery")
else()
    message(STATUS "Cross-Coordinator WAL Recovery tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_cross_coordinator_wal_recovery.cpp")
endif()

# AUTOGEN PREFIX BLOCK: transaction
file(GLOB TRANSACTION_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_*.cpp"
)

foreach(_src IN LISTS TRANSACTION_AUTOGEN_PREFIX_TEST_SOURCES)
    # Skip sources already wired by explicit focused targets in this module file.
    set(_src_already_registered FALSE)
    get_property(_tx_dir_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_tx_tgt IN LISTS _tx_dir_targets)
        get_target_property(_tx_tgt_sources ${_tx_tgt} SOURCES)
        if(_tx_tgt_sources)
            list(FIND _tx_tgt_sources "${_src}" _tx_src_idx)
            if(NOT _tx_src_idx EQUAL -1)
                set(_src_already_registered TRUE)
                break()
            endif()
        endif()
    endforeach()
    if(_src_already_registered)
        continue()
    endif()

    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_transaction_${_stem}_autofocused")
    set(_ctest "${_stem}_transaction_AutoFocusedTests")

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
        MODULE transaction
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS transaction autogen
    )
endforeach()

# ===== PHASE 1 HARDENING TESTS (Q3-Q4 2026) =====

# Phase 1: Lifecycle and Isolation Safety Hardening
# AC-1..3: Lifecycle invariants, state machine, isolation levels
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_transaction_lifecycle_phase1.cpp")
    message(STATUS "Adding Phase 1: Lifecycle and Isolation Safety Hardening tests")

    add_executable(test_transaction_lifecycle_phase1_focused
        test_transaction_lifecycle_phase1.cpp
    )

    target_link_libraries(test_transaction_lifecycle_phase1_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_lifecycle_phase1_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE transaction
        NAME TransactionLifecyclePhase1Tests
        TARGET test_transaction_lifecycle_phase1_focused
        TIER unit
        KIND focused
        TIMEOUT 120
        LABELS transaction phase-1 lifecycle isolation-safety
    )

    message(STATUS "  Phase 1: Lifecycle Invariants: begin/prepare/commit/abort, state machine, isolation levels")
else()
    message(STATUS "Phase 1 Lifecycle tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_transaction_lifecycle_phase1.cpp")
endif()

# Phase 1: Isolation Contention Hardening
# AC-3, AC-7: Isolation edge cases, lock contention, timeout determinism
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_transaction_isolation_contention_phase1.cpp")
    message(STATUS "Adding Phase 1: Isolation Contention Hardening tests")

    add_executable(test_transaction_isolation_contention_phase1_focused
        test_transaction_isolation_contention_phase1.cpp
    )

    target_link_libraries(test_transaction_isolation_contention_phase1_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_isolation_contention_phase1_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE transaction
        NAME TransactionIsolationContentionPhase1Tests
        TARGET test_transaction_isolation_contention_phase1_focused
        TIER unit
        KIND focused
        TIMEOUT 120
        LABELS transaction phase-1 isolation-contention lock-management timeout
    )

    message(STATUS "  Phase 1: Isolation Contention: dirty-read prevention, phantom-read prevention, lock contention")
else()
    message(STATUS "Phase 1 Isolation Contention tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_transaction_isolation_contention_phase1.cpp")
endif()

# Phase 1: Error Path Determinism
# AC-2, AC-7: Timeout handling, rollback determinism, error propagation
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_transaction_error_path_determinism_phase1.cpp")
    message(STATUS "Adding Phase 1: Error Path Determinism tests")

    add_executable(test_transaction_error_path_determinism_phase1_focused
        test_transaction_error_path_determinism_phase1.cpp
    )

    target_link_libraries(test_transaction_error_path_determinism_phase1_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        RocksDB::rocksdb
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(test_transaction_error_path_determinism_phase1_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE transaction
        NAME TransactionErrorPathDeterminismPhase1Tests
        TARGET test_transaction_error_path_determinism_phase1_focused
        TIER unit
        KIND focused
        TIMEOUT 120
        LABELS transaction phase-1 error-handling rollback-determinism timeout-semantics
    )

    message(STATUS "  Phase 1: Error Path Determinism: timeout handling, rollback determinism, error recovery")
else()
    message(STATUS "Phase 1 Error Path Determinism tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_transaction_error_path_determinism_phase1.cpp")
endif()