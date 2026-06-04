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

# AUTOGEN PREFIX BLOCK: transaction
file(GLOB TRANSACTION_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_transaction_*.cpp"
)

foreach(_src IN LISTS TRANSACTION_AUTOGEN_PREFIX_TEST_SOURCES)
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