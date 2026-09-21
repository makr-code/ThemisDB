# TRANSACTION DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\transaction\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\transaction\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 82
- Compounds: 309
- Classes/Structs: 193
- Namespaces: 23
- File Compounds: 82

## Namespaces
- @030174125317341027325213373254142173326163105150
- @050305051174032171224071272060246364142065066331
- @327077047374022350146004022266314357307153174005
- @370324226357302207201335272134247057166150316167
- benchmark
- plugins
- std::chrono_literals
- std::string_literals
- storage
- storage::DatabaseConnectionManager
- testing
- themis
- themis::@166106125174211275250005156300020323333367050073
- themis::@213217234310121114173122303311115057073346021260
- themis::@220375023242066236371135013354051062227340216056
- themis::sharding
- themis::test
- themis::testing
- themis::transaction
- themis::transaction::@044277112360073350317214061107317207365142140360
- themis::transaction::@275174324046316311370105355261023363012031276026
- themis::transaction::test
- themisdb::storage

## Types
### Classes
- AbortThrowingParticipant
- AdaptiveDeadlockIntegrationTest
- BenchMockParticipant
- BulkTransactionTest
- ConnectionLeakTests
- DeadlockDetectionFixture
- DeadlockPredictorTest
- DistributedPhase4Fixture
- DistributedTxnManagerTest
- MockParticipant
- OccTest
- Phase2MockParticipant
- SSITest
- StatefulMockCoordinator
- Stub2PCCoordinator
- StubTxStore
- TransactionBatcherTest
- TransactionBenchmarkFixture
- TransactionDistributedPhase2Test
- TransactionErrorPathDeterminismPhase1Test
- TransactionIsolationContentionPhase1Test
- TransactionLifecyclePhase1Test
- TransactionManagerTest
- TransactionPhase4Fixture
- TransactionTimeoutTest
- TwoPhaseCommitFixture
- themis::DeadlockPredictor
- themis::DistributedSagaCoordinator
- themis::LockManager
- themis::SAGAOrchestrator
- themis::Saga
- themis::TransactionAuditor
- themis::TransactionBatcher
- themis::TransactionManager
- themis::TransactionManager::Transaction
- themis::test::FaultInjectableCoordinator
- themis::test::MockSAGAStep
- themis::test::MockTransactionCoordinator
- themis::test::StressTransactionManager
- themis::test::TransactionChaosTest
- themis::test::TransactionFaultInjectionPhase3Test
- themis::test::TransactionSAGAPhase2Test
- themis::test::TransactionStressTest
- themis::testing::MockConnection
- themis::testing::MockDatabaseConnectionManager
- themis::transaction::BranchManager
- themis::transaction::BranchManagerBenchmark
- themis::transaction::CompensationLog
- themis::transaction::ConnectionGuard
- themis::transaction::ConnectionScopeTracker
- themis::transaction::CrashRecoveryManager
- themis::transaction::DistributedTransactionManager
- themis::transaction::GlobalTransactionManager
- themis::transaction::GlobalTwoPhaseCommitRecoveryManager
- themis::transaction::GrpcRpcPhase1Adapter
- themis::transaction::GrpcRpcPhase2Adapter
- themis::transaction::IDistributedParticipantCallback
- themis::transaction::IGlobalRegionParticipant
- themis::transaction::IInDoubtRecoveryCoordinator
- themis::transaction::IRecoverableTwoPhaseCoordinator
- themis::transaction::ITransactionCoordinator
- themis::transaction::MergeEngine
- themis::transaction::SAGAOrchestratorGuard
- themis::transaction::SagaOrchestratorPlugin
- themis::transaction::SnapshotManager
- themis::transaction::TransactionConnectionGuard
- themis::transaction::TransactionSemanticAdvisor
- themis::transaction::TwoPhaseCommitWALRecovery
- themis::transaction::test::ByzantineMockCoordinator
- themis::transaction::test::CascadingTimeoutTest
- themis::transaction::test::CircuitBreakerTest
- themis::transaction::test::CompensationIdempotencyTest
- themis::transaction::test::CompensationLog
- themis::transaction::test::CrashRecoveryChaosTest
- themis::transaction::test::CrashRecoveryDeterminismTest
- themis::transaction::test::CrashRecoveryIntegrationTest
- themis::transaction::test::CrashRecoveryUnitTest
- themis::transaction::test::CrossShardFaultInjector
- themis::transaction::test::PartialFailureTest
- themis::transaction::test::RetryStormTest
- themis::transaction::test::SAGACircuitBreaker
- themis::transaction::test::TimeoutDetectionTest
- themis::transaction::test::TimeoutDeterminismTest
- themis::transaction::test::TimeoutEdgeCaseTest
- themis::transaction::test::TransactionWaveAByzantineTest
- themis::transaction::test::TransactionWaveACrossShardTest
- themis::transaction::test::TransactionWaveARecoveryTest
- themis::transaction::test::TransactionWaveASAGATest
- themis::transaction::test::TransactionWaveATimeoutTest
- themis::transaction::test::WALReplaySimulator

### Structs
- Phase2Record
- StatefulMockCoordinator::Config
- TempDirCleanup
- themis::ConsensusVerificationResult
- themis::DeadlockPredictor::Config
- themis::DeadlockPredictor::LockPattern
- themis::DistributedSAGADefinition
- themis::DistributedSagaCoordinator::Metrics
- themis::DistributedSagaCoordinatorConfig
- themis::DistributedSagaDefinition
- themis::DistributedSagaReport
- themis::DistributedSagaStatus
- themis::DistributedSagaStep
- themis::LockManager::LockEntry
- themis::LockManager::LockRequest
- themis::LockManager::LockResult
- themis::LockManager::LockStats
- themis::LockManager::LockTableEntry
- themis::LockManager::PredicateLock
- themis::RemoteStep
- themis::SAGADefinition
- themis::SAGAExecutionStatus
- themis::SAGAOrchestrator::Metrics
- themis::SAGAOrchestratorConfig
- themis::SAGAStep
- themis::Saga::Metrics
- themis::Saga::Step
- themis::SagaOperation
- themis::SagaOrchestratorStatus
- themis::SagaVisualization
- themis::StepRecord
- themis::TransactionAuditor::AuditRecord
- themis::TransactionAuditor::IAuditExportTransport
- themis::TransactionAuditor::Operation
- themis::TransactionAuditor::Status
- themis::TransactionBatcher::BatchConfig
- themis::TransactionBatcher::BatchPolicy
- themis::TransactionBatcher::EffectivePolicy
- themis::TransactionBatcher::PendingEntry
- themis::TransactionBatcher::Stats
- themis::TransactionBatcher::Status
- themis::TransactionManager::DeadlockInfo
- themis::TransactionManager::DeadlockMetrics
- themis::TransactionManager::LockInfo
- themis::TransactionManager::SSIConfig
- themis::TransactionManager::SerializationConflict
- themis::TransactionManager::Stats
- themis::TransactionManager::Status
- themis::TransactionManager::TenantStatsEntry
- themis::TransactionManager::TenantTransactionStats
- themis::TransactionManager::TimeTravelRecord
- themis::TransactionManager::Transaction::ExplainLockEntry
- themis::TransactionManager::Transaction::ExplainResult
- themis::TransactionManager::Transaction::ExplainWriteEntry
- themis::TransactionManager::Transaction::SavepointEntry
- themis::test::MockTransactionCoordinator::Transaction
- themis::test::StressTransactionManager::Transaction
- themis::transaction::BranchManager::Branch
- themis::transaction::BranchManager::BranchGCPolicy
- themis::transaction::BranchManager::BranchHistoryEntry
- themis::transaction::BranchManager::BranchStats
- themis::transaction::BranchManager::CreateBranchOptions
- themis::transaction::BranchManager::MergeOptions
- themis::transaction::BranchManager::MergeResult
- themis::transaction::CompensationLogEntry
- themis::transaction::CoordinatorCapabilities
- themis::transaction::CrashRecoveryManager::LogEntry
- themis::transaction::CrashRecoveryManager::OperationEntry
- themis::transaction::CrashRecoveryManager::RecoveryMetrics
- themis::transaction::CrashRecoveryManager::RecoveryResult
- themis::transaction::DistributedTransaction
- themis::transaction::DistributedTransactionManager::BatchPrepareEntry
- themis::transaction::DistributedTransactionManager::Statistics
- themis::transaction::DistributedTxnManagerConfig
- themis::transaction::DistributedTxnStatus
- themis::transaction::GlobalTransactionManager::Config
- themis::transaction::GlobalTwoPhaseCommitRecoveryReport
- themis::transaction::GlobalTxnOutcome
- themis::transaction::GlobalTxnRecord
- themis::transaction::InDoubtTxnDescriptor
- themis::transaction::MergeEngine::Conflict
- themis::transaction::MergeEngine::ConflictResolution
- themis::transaction::MergeEngine::MergeOptions
- themis::transaction::MergeEngine::MergeResult
- themis::transaction::MergeEngine::MergeStats
- themis::transaction::MtlsConfig
- themis::transaction::Participant
- themis::transaction::RecoverableTwoPhaseCoordinatorReport
- themis::transaction::RecoverableTwoPhaseTransaction
- themis::transaction::RecoveredTwoPhaseCommitTransaction
- themis::transaction::RegionTxnRecord
- themis::transaction::SnapshotManager::RestoreResult
- themis::transaction::SnapshotManager::RetentionPolicy
- themis::transaction::SnapshotManager::Snapshot
- themis::transaction::SnapshotManager::SnapshotStats
- themis::transaction::TransactionContext
- themis::transaction::TransactionSemanticAdvisor::BatchAffinityHint
- themis::transaction::TransactionSemanticAdvisor::Config
- themis::transaction::TransactionStateSnapshot
- themis::transaction::TxnCoordinatorOptions
- themis::transaction::TxnCoordinatorResult
- themis::transaction::test::BackoffConfig
- themis::transaction::test::TxnRecord

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 1468

### AbortThrowingParticipant

#### `void onAbort(const std::string &) override`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:115
- Brief: On Abort.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `void onCommit(const std::string &) override`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:114
- Brief: On Commit.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `bool onPrepare(const std::string &, const std::set< std::string > &) override`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (const std::set< std::string > &): n/a

### AdaptiveDeadlockIntegrationTest

#### `void SetUp() override`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:261
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:279
- Brief: n/a
- Parameters: none

### BenchMockParticipant

#### `void onAbort(const std::string &) override`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:90
- Brief: On Abort.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `void onCommit(const std::string &) override`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:89
- Brief: On Commit.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `bool onPrepare(const std::string &, const std::set< std::string > &) override`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (const std::set< std::string > &): n/a

### BulkTransactionTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_bulk.cpp`:20
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_bulk.cpp`:34
- Brief: n/a
- Parameters: none

#### `BaseEntity makeEntity(const std::string &pk, const std::string &name="", const std::string &city="")`
- Source: `tests/transaction/test_transaction_bulk.cpp`:44
- Brief: n/a
- Parameters:
  - `pk` (const std::string &): n/a
  - `name` (const std::string &): n/a
  - `city` (const std::string &): n/a

### ConnectionLeakTests

#### `void SetUp() override`
- Source: `tests/transaction/connection_leak_tests.cpp`:95
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/connection_leak_tests.cpp`:100
- Brief: n/a
- Parameters: none

### DeadlockDetectionFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:815
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:846
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `DeadlockPredictor & predictor()`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:851
- Brief: n/a
- Parameters: none

### DistributedPhase4Fixture

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:162
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `std::vector< Participant > makeParticipants()`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:185
- Brief: n/a
- Parameters: none

### DistributedTxnManagerTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:172
- Brief: n/a
- Parameters: none

### MockParticipant

#### `MockParticipant(Policy p=Policy::COMMIT)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:68
- Brief: n/a
- Parameters:
  - `p` (Policy): n/a

#### `MockParticipant(Policy policy=Policy::ALWAYS_COMMIT)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:52
- Brief: n/a
- Parameters:
  - `policy` (Policy): n/a

#### `int abortCount() const`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:82
- Brief: n/a
- Parameters: none

#### `int commitCount() const`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:81
- Brief: n/a
- Parameters: none

#### `std::string lastAbortTxn() const`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:92
- Brief: n/a
- Parameters: none

#### `std::string lastCommitTxn() const`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:88
- Brief: n/a
- Parameters: none

#### `std::string lastPrepareTxn() const`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:84
- Brief: n/a
- Parameters: none

#### `void onAbort(const std::string &) override`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:78
- Brief: On Abort.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `void onAbort(const std::string &txn_id) override`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:73
- Brief: On Abort.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `void onCommit(const std::string &) override`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:77
- Brief: On Commit.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `void onCommit(const std::string &txn_id) override`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:67
- Brief: On Commit.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `bool onPrepare(const std::string &, const std::set< std::string > &) override`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (const std::set< std::string > &): n/a

#### `bool onPrepare(const std::string &txn_id, const std::set< std::string > &keys) override`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:55
- Brief: n/a
- Parameters:
  - `txn_id` (const std::string &): n/a
  - `keys` (const std::set< std::string > &): n/a

#### `int prepareCount() const`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:80
- Brief: n/a
- Parameters: none

### OccTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_occ.cpp`:20
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_occ.cpp`:34
- Brief: n/a
- Parameters: none

#### `BaseEntity makeEntity(const std::string &pk, const std::string &name="")`
- Source: `tests/transaction/test_transaction_occ.cpp`:44
- Brief: n/a
- Parameters:
  - `pk` (const std::string &): n/a
  - `name` (const std::string &): n/a

### Phase2MockParticipant

#### `Phase2MockParticipant(Policy policy=Policy::ALWAYS_COMMIT)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:46
- Brief: n/a
- Parameters:
  - `policy` (Policy): n/a

#### `int abortCount() const`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:72
- Brief: n/a
- Parameters: none

#### `int commitCount() const`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:71
- Brief: n/a
- Parameters: none

#### `void onAbort(const std::string &txn_id) override`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:64
- Brief: On Abort.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `void onCommit(const std::string &txn_id) override`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:58
- Brief: On Commit.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `bool onPrepare(const std::string &txn_id, const std::set< std::string > &) override`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:49
- Brief: n/a
- Parameters:
  - `txn_id` (const std::string &): n/a
  - `<unnamed>` (const std::set< std::string > &): n/a

#### `int prepareCount() const`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:70
- Brief: n/a
- Parameters: none

### SSITest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_ssi.cpp`:50
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_ssi.cpp`:64
- Brief: n/a
- Parameters: none

#### `BaseEntity makeEntity(const std::string &pk, const std::string &value="")`
- Source: `tests/transaction/test_transaction_ssi.cpp`:73
- Brief: n/a
- Parameters:
  - `pk` (const std::string &): n/a
  - `value` (const std::string &): n/a

### StatefulMockCoordinator

#### `StatefulMockCoordinator()`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:94
- Brief: n/a
- Parameters: none

#### `StatefulMockCoordinator(Config cfg)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:95
- Brief: n/a
- Parameters:
  - `cfg` (Config): n/a

#### `TxnCoordinatorResult abort(std::string_view txn_id) override`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:172
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `TxnCoordinatorResult begin(std::string_view txn_id, const TxnCoordinatorOptions &opts={}) override`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:113
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a
  - `opts` (const TxnCoordinatorOptions &): n/a

#### `CoordinatorCapabilities capabilities() const noexcept override`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:107
- Brief: n/a
- Parameters: none

#### `TxnCoordinatorResult commit(std::string_view txn_id) override`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:154
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `std::vector< InDoubtTxnDescriptor > getInDoubtTransactions() const override`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:213
- Brief: n/a
- Parameters: none

#### `TxnLifecycleState getState(std::string_view txn_id) const override`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:191
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `void injectCommittingTxn(std::string_view txn_id)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:244
- Brief: Inject a transaction in COMMITTING state (durable commit decision written).
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `void injectPreparedTxn(std::string_view txn_id)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:239
- Brief: Inject an in-doubt transaction directly (simulates coordinator restart after prepare).
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `TxnCoordinatorResult prepare(std::string_view txn_id) override`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:133
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `std::string_view protocolName() const noexcept override`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:103
- Brief: n/a
- Parameters: none

#### `CommitProtocol protocolType() const noexcept override`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:99
- Brief: n/a
- Parameters: none

#### `std::size_t recoverInDoubt() override`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:201
- Brief: n/a
- Parameters: none

#### `const TxnCoordinatorOptions & storedOptions(std::string_view txn_id) const`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:234
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

### Stub2PCCoordinator

#### `uint64_t committed() const`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:102
- Brief: n/a
- Parameters: none

#### `bool do_commit(uint64_t tx_id)`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:95
- Brief: n/a
- Parameters:
  - `tx_id` (uint64_t): n/a

#### `Vote prepare(uint64_t tx_id, uint64_t key)`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:88
- Brief: n/a
- Parameters:
  - `tx_id` (uint64_t): n/a
  - `key` (uint64_t): n/a

#### `uint64_t prepared() const`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:101
- Brief: n/a
- Parameters: none

### StubTxStore

#### `CommitResult commit(uint64_t tx_id, uint64_t key, uint64_t value)`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:47
- Brief: n/a
- Parameters:
  - `tx_id` (uint64_t): n/a
  - `key` (uint64_t): n/a
  - `value` (uint64_t): n/a

#### `uint64_t committed_count() const`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:70
- Brief: n/a
- Parameters: none

#### `uint64_t conflict_count() const`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:71
- Brief: n/a
- Parameters: none

#### `void lock_key(uint64_t tx_id, uint64_t key)`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:60
- Brief: n/a
- Parameters:
  - `tx_id` (uint64_t): n/a
  - `key` (uint64_t): n/a

#### `void unlock_key(uint64_t, uint64_t key)`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (uint64_t): n/a
  - `key` (uint64_t): n/a

### TempDirCleanup

#### `~TempDirCleanup() noexcept`
- Source: `tests/transaction/test_cross_coordinator_wal_recovery.cpp`:48
- Brief: n/a
- Parameters: none

### TransactionBatcherTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_batcher.cpp`:56
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_batcher.cpp`:66
- Brief: n/a
- Parameters: none

### TransactionBenchmarkFixture

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:42
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void populateTestData(size_t count)`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:96
- Brief: n/a
- Parameters:
  - `count` (size_t): n/a

### TransactionDistributedPhase2Test

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:111
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:120
- Brief: n/a
- Parameters: none

### TransactionErrorPathDeterminismPhase1Test

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:23
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:46
- Brief: n/a
- Parameters: none

### TransactionIsolationContentionPhase1Test

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_isolation_contention_phase1.cpp`:23
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_isolation_contention_phase1.cpp`:45
- Brief: n/a
- Parameters: none

### TransactionLifecyclePhase1Test

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:20
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:43
- Brief: n/a
- Parameters: none

### TransactionManagerTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_manager.cpp`:16
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_manager.cpp`:41
- Brief: n/a
- Parameters: none

### TransactionPhase4Fixture

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:100
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

### TransactionTimeoutTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_timeout.cpp`:36
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_timeout.cpp`:57
- Brief: n/a
- Parameters: none

### TwoPhaseCommitFixture

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:706
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:730
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `themis::sharding::TwoPhaseCommitCoordinator & coordinator()`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:742
- Brief: Access from benchmark body.
- Parameters: none

#### `const std::map< std::string, std::unique_ptr< themis::sharding::TwoPhaseCommitParticipant > > & participants() const`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:745
- Brief: n/a
- Parameters: none

### bench_branch_manager.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:275
- Brief: n/a
- Parameters: none

### bench_transaction_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:138
- Brief: n/a
- Parameters: none

#### `void BM_Transaction_2PCRoundTripP99(benchmark::State &state)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:101
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Transaction_AbortP95(benchmark::State &state)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:84
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Transaction_CommitP95(benchmark::State &state)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:67
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Transaction_ConcurrentThroughput(benchmark::State &state)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:124
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Name("BM_Transaction_2PCRoundTripP99") -> Iterations(5000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` ("BM_Transaction_2PCRoundTripP99"): n/a

#### `Name("BM_Transaction_AbortP95") -> Iterations(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` ("BM_Transaction_AbortP95"): n/a

#### `Name("BM_Transaction_CommitP95") -> Iterations(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` ("BM_Transaction_CommitP95"): n/a

#### `Name("BM_Transaction_ConcurrentThroughput") -> Threads(8) ->Iterations(1000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` ("BM_Transaction_ConcurrentThroughput"): n/a

#### `bool stub_2pc_commit(uint64_t)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (uint64_t): n/a

#### `bool stub_2pc_prepare(uint64_t)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (uint64_t): n/a

#### `bool stub_abort(uint64_t)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (uint64_t): n/a

#### `uint64_t stub_begin()`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:41
- Brief: n/a
- Parameters: none

#### `bool stub_commit(uint64_t)`
- Source: `benchmarks/transaction/bench_transaction_dedicated_gates.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (uint64_t): n/a

### bench_transaction_phase4.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:450
- Brief: n/a
- Parameters: none

#### `state SetItemsProcessed(state.iterations())`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:227
- Brief: n/a
- Parameters:
  - `iterations` (state.): n/a

#### `UseManualTime() -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:232
- Brief: n/a
- Parameters: none

#### `for(auto _ :state)`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (auto _ :state): n/a

#### `publishLatencyCounters(state, histogram)`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (state): n/a
  - `<unnamed>` (histogram): n/a

#### `publishLatencyGate(state, kLatencyTailGateMs)`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (state): n/a
  - `<unnamed>` (kLatencyTailGateMs): n/a

#### `publishThroughputGate(state, average_ms, kDistributedThroughputGateTxnPerSec)`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:412
- Brief: n/a
- Parameters:
  - `<unnamed>` (state): n/a
  - `<unnamed>` (average_ms): n/a
  - `<unnamed>` (kDistributedThroughputGateTxnPerSec): n/a

#### `publishThroughputGate(state, average_ms, kLocalThroughputGateTxnPerSec)`
- Source: `benchmarks/transaction/bench_transaction_phase4.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (state): n/a
  - `<unnamed>` (average_ms): n/a
  - `<unnamed>` (kLocalThroughputGateTxnPerSec): n/a

### bench_transaction_throughput.cpp

#### `Arg(1) -> Arg(10) ->Arg(100) ->Arg(1000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:282
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Arg(2) -> Arg(5) ->Arg(10) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:416
- Brief: n/a
- Parameters:
  - `<unnamed>` (2): n/a

#### `Arg(3) -> Arg(5) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:789
- Brief: n/a
- Parameters:
  - `<unnamed>` (3): n/a

#### `Args({0, 5}) -> Args({5, 5}) ->Args({5, 20}) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:368
- Brief: n/a
- Parameters:
  - `<unnamed>` ({0, 5}): n/a

#### `BENCHMARK_DEFINE_F(DeadlockDetectionFixture, DeadlockDetectionOverhead_LockOrder)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:902
- Brief: Benchmark: recommendLockOrder() call overhead (TX-6 direct).
- Parameters:
  - `<unnamed>` (DeadlockDetectionFixture): n/a
  - `<unnamed>` (DeadlockDetectionOverhead_LockOrder): n/a
- Details: Each iteration asks the pre-trained predictor to sort a 5-key candidate set into a safe acquisition order. This exercises the pair-conflict matrix traversal that dominates the predictor's steady-state cost. SLO TX-6: same overhead budget as DeadlockDetectionOverhead_Predict.

#### `BENCHMARK_DEFINE_F(DeadlockDetectionFixture, DeadlockDetectionOverhead_Predict)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:865
- Brief: Benchmark: predictDeadlockProbability() call overhead (TX-6 direct).
- Parameters:
  - `<unnamed>` (DeadlockDetectionFixture): n/a
  - `<unnamed>` (DeadlockDetectionOverhead_Predict): n/a
- Details: Each iteration queries the pre-trained predictor for a 5-key lock set drawn from the hot 20-key space. The result is consumed via DoNotOptimize to prevent the call from being elided. SLO TX-6: this call should consume ≤ 1 % of a typical transaction's wall-clock budget (~0.1 ms base → ≤ 1 µs target).

#### `BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, AbortTransaction)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBenchmarkFixture): n/a
  - `<unnamed>` (AbortTransaction): n/a

#### `BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, CommitLatency)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBenchmarkFixture): n/a
  - `<unnamed>` (CommitLatency): n/a

#### `BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, MixedTransaction)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBenchmarkFixture): n/a
  - `<unnamed>` (MixedTransaction): n/a

#### `BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, OccOptimisticErase)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:570
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBenchmarkFixture): n/a
  - `<unnamed>` (OccOptimisticErase): n/a

#### `BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, OccOptimisticPut)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:465
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBenchmarkFixture): n/a
  - `<unnamed>` (OccOptimisticPut): n/a

#### `BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, OccReadVersionAndUpdate)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:502
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBenchmarkFixture): n/a
  - `<unnamed>` (OccReadVersionAndUpdate): n/a

#### `BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, ReadOnlyTransaction)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBenchmarkFixture): n/a
  - `<unnamed>` (ReadOnlyTransaction): n/a

#### `BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, SavepointCreateAndRollback)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:325
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBenchmarkFixture): n/a
  - `<unnamed>` (SavepointCreateAndRollback): n/a

#### `BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, SavepointNested)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:377
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBenchmarkFixture): n/a
  - `<unnamed>` (SavepointNested): n/a

#### `BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, SavepointRelease)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:425
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBenchmarkFixture): n/a
  - `<unnamed>` (SavepointRelease): n/a

#### `BENCHMARK_DEFINE_F(TransactionBenchmarkFixture, WriteOnlyTransaction)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBenchmarkFixture): n/a
  - `<unnamed>` (WriteOnlyTransaction): n/a

#### `BENCHMARK_DEFINE_F(TwoPhaseCommitFixture, TwoPhaseCommitLatency)(benchmark`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:757
- Brief: Benchmark: end-to-end 2PC commit latency across N in-process shards.
- Parameters:
  - `<unnamed>` (TwoPhaseCommitFixture): n/a
  - `<unnamed>` (TwoPhaseCommitLatency): n/a
- Details: Each iteration commits one transaction via the full two-phase protocol: PREPARE to all N shards, then COMMIT (or ABORT on failure). The operation payload is a single JSON string per shard — enough to exercise the protocol framing without adding storage overhead. state Benchmark state; range(0) = number of shards. SLO TX-4: ≤ 5 ms per commit for 5 shards.

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:929
- Brief: n/a
- Parameters: none

#### `void BM_TransactionContention(benchmark::State &state)`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:615
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Threads(1) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/transaction/bench_transaction_throughput.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

### connection_leak_tests.cpp

#### `TEST_F(ConnectionLeakTests, ConcurrentConnectionAcquisition)`
- Source: `tests/transaction/connection_leak_tests.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (ConcurrentConnectionAcquisition): n/a

#### `TEST_F(ConnectionLeakTests, ConnectionGuardAcquireRelease)`
- Source: `tests/transaction/connection_leak_tests.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (ConnectionGuardAcquireRelease): n/a

#### `TEST_F(ConnectionLeakTests, ConnectionGuardExceptionPath)`
- Source: `tests/transaction/connection_leak_tests.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (ConnectionGuardExceptionPath): n/a

#### `TEST_F(ConnectionLeakTests, ConnectionGuardManualRelease)`
- Source: `tests/transaction/connection_leak_tests.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (ConnectionGuardManualRelease): n/a

#### `TEST_F(ConnectionLeakTests, ConnectionGuardMarkError)`
- Source: `tests/transaction/connection_leak_tests.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (ConnectionGuardMarkError): n/a

#### `TEST_F(ConnectionLeakTests, ConnectionGuardMove)`
- Source: `tests/transaction/connection_leak_tests.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (ConnectionGuardMove): n/a

#### `TEST_F(ConnectionLeakTests, ConnectionScopeTrackerFailure)`
- Source: `tests/transaction/connection_leak_tests.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (ConnectionScopeTrackerFailure): n/a

#### `TEST_F(ConnectionLeakTests, ConnectionScopeTrackerImplicitRecord)`
- Source: `tests/transaction/connection_leak_tests.cpp`:328
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (ConnectionScopeTrackerImplicitRecord): n/a

#### `TEST_F(ConnectionLeakTests, ConnectionScopeTrackerSuccess)`
- Source: `tests/transaction/connection_leak_tests.cpp`:305
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (ConnectionScopeTrackerSuccess): n/a

#### `TEST_F(ConnectionLeakTests, ExecuteWithConnectionException)`
- Source: `tests/transaction/connection_leak_tests.cpp`:427
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (ExecuteWithConnectionException): n/a

#### `TEST_F(ConnectionLeakTests, ExecuteWithConnectionSuccess)`
- Source: `tests/transaction/connection_leak_tests.cpp`:406
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (ExecuteWithConnectionSuccess): n/a

#### `TEST_F(ConnectionLeakTests, TransactionConnectionGuardBasic)`
- Source: `tests/transaction/connection_leak_tests.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (TransactionConnectionGuardBasic): n/a

#### `TEST_F(ConnectionLeakTests, TransactionConnectionGuardException)`
- Source: `tests/transaction/connection_leak_tests.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (TransactionConnectionGuardException): n/a

#### `TEST_F(ConnectionLeakTests, TransactionConnectionGuardFailure)`
- Source: `tests/transaction/connection_leak_tests.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (TransactionConnectionGuardFailure): n/a

#### `TEST_F(ConnectionLeakTests, TransactionConnectionGuardMultiple)`
- Source: `tests/transaction/connection_leak_tests.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (TransactionConnectionGuardMultiple): n/a

#### `TEST_F(ConnectionLeakTests, TransactionGuardUnderLoad)`
- Source: `tests/transaction/connection_leak_tests.cpp`:372
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConnectionLeakTests): n/a
  - `<unnamed>` (TransactionGuardUnderLoad): n/a

#### `int main(int argc, char **argv)`
- Source: `tests/transaction/connection_leak_tests.cpp`:452
- Brief: n/a
- Parameters:
  - `argc` (int): n/a
  - `argv` (char **): n/a

### saga_orchestrator_plugin.cpp

#### `THEMIS_SAGA_PLUGIN_EXPORT themis::plugins::IThemisPlugin * createPlugin() noexcept`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:220
- Brief: Plugin factory function - C interface for dynamic loading.
- Parameters: none
- Return: Pointer to newly allocated SagaOrchestratorPlugin instance; nullptr on failure
- Details: MEMORY OWNERSHIP: Caller is responsible for calling destroyPlugin() to free returned pointer Returns nullptr on failure (C-ABI safe error reporting) Exception safety: no exceptions escape this function Pointer to newly allocated SagaOrchestratorPlugin instance; nullptr on failure

#### `THEMIS_SAGA_PLUGIN_EXPORT void destroyPlugin(themis::plugins::IThemisPlugin *plugin) noexcept`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:235
- Brief: Plugin destroyer function - C interface for dynamic unloading.
- Parameters:
  - `plugin` (themis::plugins::IThemisPlugin *): Pointer to SagaOrchestratorPlugin instance to destroy
- Details: PRECONDITION: plugin must be a non-null pointer returned from createPlugin() POSTCONDITION: plugin pointer is deleted; caller should not use it afterward EXCEPTION SAFETY: noexcept; any exceptions are logged and suppressed plugin Pointer to SagaOrchestratorPlugin instance to destroy

### test_adaptive_deadlock_prevention.cpp

#### `TEST_F(AdaptiveDeadlockIntegrationTest, CommitRecordsTransactionInPredictor)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:358
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (CommitRecordsTransactionInPredictor): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, DetachPredictor)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:306
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (DetachPredictor): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, MultipleTransactionsDoNotCrashWithPredictor)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (MultipleTransactionsDoNotCrashWithPredictor): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, NoPredictorByDefault)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (NoPredictorByDefault): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, PredictProbabilityDelegatesToPredictor)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (PredictProbabilityDelegatesToPredictor): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, PredictProbabilityUsesActiveTransactionCount)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:404
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (PredictProbabilityUsesActiveTransactionCount): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, PredictProbabilityWithoutPredictorReturnsZero)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (PredictProbabilityWithoutPredictorReturnsZero): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, PredictorCanBeReplacedLive)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:394
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (PredictorCanBeReplacedLive): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, RecommendLockOrderDelegatesToPredictor)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:337
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (RecommendLockOrderDelegatesToPredictor): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, RecommendLockOrderWithoutPredictorIsLexicographic)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (RecommendLockOrderWithoutPredictorIsLexicographic): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, RecommendTimeoutDelegatesToPredictor)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:350
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (RecommendTimeoutDelegatesToPredictor): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, RecommendTimeoutWithoutPredictorUsesDeadlockTimeout)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:344
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (RecommendTimeoutWithoutPredictorUsesDeadlockTimeout): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, RollbackRecordsTransactionInPredictor)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:369
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (RollbackRecordsTransactionInPredictor): n/a

#### `TEST_F(AdaptiveDeadlockIntegrationTest, SetAndGetPredictor)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaptiveDeadlockIntegrationTest): n/a
  - `<unnamed>` (SetAndGetPredictor): n/a

#### `TEST_F(DeadlockPredictorTest, ActiveTransactionsInflateProbability)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (ActiveTransactionsInflateProbability): n/a

#### `TEST_F(DeadlockPredictorTest, EmptyDeadlockRecordIsIgnored)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (EmptyDeadlockRecordIsIgnored): n/a

#### `TEST_F(DeadlockPredictorTest, EmptyLockSetRecordIsIgnored)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (EmptyLockSetRecordIsIgnored): n/a

#### `TEST_F(DeadlockPredictorTest, InitialStateHasZeroCounts)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:40
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (InitialStateHasZeroCounts): n/a

#### `TEST_F(DeadlockPredictorTest, PatternsAccumulate)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (PatternsAccumulate): n/a

#### `TEST_F(DeadlockPredictorTest, PredictProbabilityClampedToOne)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (PredictProbabilityClampedToOne): n/a

#### `TEST_F(DeadlockPredictorTest, PredictProbabilityIsHigherForDeadlockedPairs)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (PredictProbabilityIsHigherForDeadlockedPairs): n/a

#### `TEST_F(DeadlockPredictorTest, PredictProbabilityNonZeroFromCoOccurrenceAlone)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (PredictProbabilityNonZeroFromCoOccurrenceAlone): n/a

#### `TEST_F(DeadlockPredictorTest, PredictReturnsProbabilityAfterSufficientSamples)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (PredictReturnsProbabilityAfterSufficientSamples): n/a

#### `TEST_F(DeadlockPredictorTest, PredictReturnsZeroBelowMinSamples)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (PredictReturnsZeroBelowMinSamples): n/a

#### `TEST_F(DeadlockPredictorTest, RecommendLockOrderIsIdempotent)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (RecommendLockOrderIsIdempotent): n/a

#### `TEST_F(DeadlockPredictorTest, RecommendLockOrderIsStableWhenNoData)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (RecommendLockOrderIsStableWhenNoData): n/a

#### `TEST_F(DeadlockPredictorTest, RecommendLockOrderReturnsAllKeys)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (RecommendLockOrderReturnsAllKeys): n/a

#### `TEST_F(DeadlockPredictorTest, RecommendTimeoutFallsBackToMinWhenNoData)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (RecommendTimeoutFallsBackToMinWhenNoData): n/a

#### `TEST_F(DeadlockPredictorTest, RecommendTimeoutGrowsWithLongHoldTimes)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (RecommendTimeoutGrowsWithLongHoldTimes): n/a

#### `TEST_F(DeadlockPredictorTest, RecommendTimeoutIsClamped)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (RecommendTimeoutIsClamped): n/a

#### `TEST_F(DeadlockPredictorTest, RecordDeadlockIncreasesCount)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (RecordDeadlockIncreasesCount): n/a

#### `TEST_F(DeadlockPredictorTest, RecordTransactionIncreasesCount)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (RecordTransactionIncreasesCount): n/a

#### `TEST_F(DeadlockPredictorTest, ResetClearsAllState)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (ResetClearsAllState): n/a

#### `TEST_F(DeadlockPredictorTest, SetAndGetConfig)`
- Source: `tests/transaction/test_adaptive_deadlock_prevention.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadlockPredictorTest): n/a
  - `<unnamed>` (SetAndGetConfig): n/a

### test_cross_coordinator_wal_recovery.cpp

#### `TEST(CrossCoordinatorWALRecovery, AppendEntry_VoidOverload_DoesNotThrow)`
- Source: `tests/transaction/test_cross_coordinator_wal_recovery.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCoordinatorWALRecovery): n/a
  - `<unnamed>` (AppendEntry_VoidOverload_DoesNotThrow): n/a

#### `TEST(CrossCoordinatorWALRecovery, BuildEntry_BeginTx_HasCorrectType)`
- Source: `tests/transaction/test_cross_coordinator_wal_recovery.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCoordinatorWALRecovery): n/a
  - `<unnamed>` (BuildEntry_BeginTx_HasCorrectType): n/a

#### `TEST(CrossCoordinatorWALRecovery, BuildEntry_JsonPayloadRoundTrip)`
- Source: `tests/transaction/test_cross_coordinator_wal_recovery.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCoordinatorWALRecovery): n/a
  - `<unnamed>` (BuildEntry_JsonPayloadRoundTrip): n/a

#### `TEST(CrossCoordinatorWALRecovery, DTM_RecoverFromEmptyWAL_ReturnsZero)`
- Source: `tests/transaction/test_cross_coordinator_wal_recovery.cpp`:212
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCoordinatorWALRecovery): n/a
  - `<unnamed>` (DTM_RecoverFromEmptyWAL_ReturnsZero): n/a

#### `TEST(CrossCoordinatorWALRecovery, DTM_RecoverWithoutWAL_ReturnsZero)`
- Source: `tests/transaction/test_cross_coordinator_wal_recovery.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCoordinatorWALRecovery): n/a
  - `<unnamed>` (DTM_RecoverWithoutWAL_ReturnsZero): n/a

#### `TEST(CrossCoordinatorWALRecovery, EntryTypes_CommitAndAbort_AreDistinct)`
- Source: `tests/transaction/test_cross_coordinator_wal_recovery.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCoordinatorWALRecovery): n/a
  - `<unnamed>` (EntryTypes_CommitAndAbort_AreDistinct): n/a

#### `TEST(CrossCoordinatorWALRecovery, NullWAL_AppendWithResult_ReturnsNullopt)`
- Source: `tests/transaction/test_cross_coordinator_wal_recovery.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCoordinatorWALRecovery): n/a
  - `<unnamed>` (NullWAL_AppendWithResult_ReturnsNullopt): n/a

#### `TEST(CrossCoordinatorWALRecovery, RealWAL_AppendWithResult_ReturnsValidLSN)`
- Source: `tests/transaction/test_cross_coordinator_wal_recovery.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCoordinatorWALRecovery): n/a
  - `<unnamed>` (RealWAL_AppendWithResult_ReturnsValidLSN): n/a

#### `std::filesystem::path makeTempDir(const std::string &suffix)`
- Source: `tests/transaction/test_cross_coordinator_wal_recovery.cpp`:37
- Brief: Create a temporary directory unique to this process invocation.
- Parameters:
  - `suffix` (const std::string &): n/a

### test_grpc_rpc_adapter.cpp

#### `TEST(GrpcRpcAdapterContention, SerialTransactionsDeterminism)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:457
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterContention): n/a
  - `<unnamed>` (SerialTransactionsDeterminism): n/a

#### `TEST(GrpcRpcAdapterDtmIntegration, Phase1FnAbortVote)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:407
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterDtmIntegration): n/a
  - `<unnamed>` (Phase1FnAbortVote): n/a

#### `TEST(GrpcRpcAdapterDtmIntegration, Phase1FnWiredAllCommit)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:384
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterDtmIntegration): n/a
  - `<unnamed>` (Phase1FnWiredAllCommit): n/a

#### `TEST(GrpcRpcAdapterDtmIntegration, Phase2FnDeliverCommit)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:428
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterDtmIntegration): n/a
  - `<unnamed>` (Phase2FnDeliverCommit): n/a

#### `TEST(GrpcRpcAdapterMtls, ConstructWithExplicitInsecureOverride)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:579
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterMtls): n/a
  - `<unnamed>` (ConstructWithExplicitInsecureOverride): n/a

#### `TEST(GrpcRpcAdapterMtls, ConstructWithMtlsConfig)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:528
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterMtls): n/a
  - `<unnamed>` (ConstructWithMtlsConfig): n/a

#### `TEST(GrpcRpcAdapterMtls, ConstructWithNulloptFailsClosed)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:557
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterMtls): n/a
  - `<unnamed>` (ConstructWithNulloptFailsClosed): n/a

#### `TEST(GrpcRpcAdapterPhase1, UnknownNodeVotesAbort)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterPhase1): n/a
  - `<unnamed>` (UnknownNodeVotesAbort): n/a

#### `TEST(GrpcRpcAdapterPhase1, VoteAbortOnFailure)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterPhase1): n/a
  - `<unnamed>` (VoteAbortOnFailure): n/a

#### `TEST(GrpcRpcAdapterPhase1, VoteAbortOnNetworkException)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterPhase1): n/a
  - `<unnamed>` (VoteAbortOnNetworkException): n/a

#### `TEST(GrpcRpcAdapterPhase1, VoteAbortOnTimeout)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterPhase1): n/a
  - `<unnamed>` (VoteAbortOnTimeout): n/a

#### `TEST(GrpcRpcAdapterPhase1, VoteCommitOnSuccess)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterPhase1): n/a
  - `<unnamed>` (VoteCommitOnSuccess): n/a

#### `TEST(GrpcRpcAdapterPhase2, CommitOnFirstAttempt)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:278
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterPhase2): n/a
  - `<unnamed>` (CommitOnFirstAttempt): n/a

#### `TEST(GrpcRpcAdapterPhase2, FailAfterThreeRetries)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterPhase2): n/a
  - `<unnamed>` (FailAfterThreeRetries): n/a

#### `TEST(GrpcRpcAdapterPhase2, RetrySucceedsOnSecondAttempt)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:303
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterPhase2): n/a
  - `<unnamed>` (RetrySucceedsOnSecondAttempt): n/a

#### `TEST(GrpcRpcAdapterPhase2, RollbackOnFirstAttempt)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterPhase2): n/a
  - `<unnamed>` (RollbackOnFirstAttempt): n/a

#### `TEST(GrpcRpcAdapterPhase2, UnknownNodeThrows)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:372
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterPhase2): n/a
  - `<unnamed>` (UnknownNodeThrows): n/a

#### `TEST(GrpcRpcAdapterWal, CommitDecisionDurableBeforePhase2)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:496
- Brief: n/a
- Parameters:
  - `<unnamed>` (GrpcRpcAdapterWal): n/a
  - `<unnamed>` (CommitDecisionDurableBeforePhase2): n/a

#### `DistributedTxnManagerConfig makeConfig()`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:91
- Brief: n/a
- Parameters: none

#### `DistributedTransactionManager::RpcPhase1Fn makeP1Fn(bool vote)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:103
- Brief: n/a
- Parameters:
  - `vote` (bool): n/a

#### `DistributedTransactionManager::RpcPhase2Fn makeP2RecordFn(Phase2Record &rec, bool throw_on_attempt=false, int succeed_after=0)`
- Source: `tests/transaction/test_grpc_rpc_adapter.cpp`:120
- Brief: n/a
- Parameters:
  - `rec` (Phase2Record &): n/a
  - `throw_on_attempt` (bool): n/a
  - `succeed_after` (int): n/a

### test_itransaction_coordinator.cpp

#### `TEST(ITransactionCoordinator, ITC01_CommitProtocolEnumValues)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC01_CommitProtocolEnumValues): n/a

#### `TEST(ITransactionCoordinator, ITC02_CapabilitiesDefaultsFalse)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC02_CapabilitiesDefaultsFalse): n/a

#### `TEST(ITransactionCoordinator, ITC03_ResultOk)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC03_ResultOk): n/a

#### `TEST(ITransactionCoordinator, ITC04_05_ResultFail)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:298
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC04_05_ResultFail): n/a

#### `TEST(ITransactionCoordinator, ITC06_OptionsDefaults)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC06_OptionsDefaults): n/a

#### `TEST(ITransactionCoordinator, ITC07_InDoubtDescriptorDefaults)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:323
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC07_InDoubtDescriptorDefaults): n/a

#### `TEST(ITransactionCoordinator, ITC08_TwoPCCommitLifecycle)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:335
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC08_TwoPCCommitLifecycle): n/a

#### `TEST(ITransactionCoordinator, ITC09_TwoPCAbortLifecycle)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:355
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC09_TwoPCAbortLifecycle): n/a

#### `TEST(ITransactionCoordinator, ITC10_PrepareParticipantAbort)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:370
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC10_PrepareParticipantAbort): n/a

#### `TEST(ITransactionCoordinator, ITC11_SagaPrepareIsNoOp)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC11_SagaPrepareIsNoOp): n/a

#### `TEST(ITransactionCoordinator, ITC12_PercolatorCapabilities)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:406
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC12_PercolatorCapabilities): n/a

#### `TEST(ITransactionCoordinator, ITC13_CalvinCapabilities)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:428
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC13_CalvinCapabilities): n/a

#### `TEST(ITransactionCoordinator, ITC14_GetStateUnknown)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:447
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC14_GetStateUnknown): n/a

#### `TEST(ITransactionCoordinator, ITC15_RecoverInDoubtEmpty)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:456
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC15_RecoverInDoubtEmpty): n/a

#### `TEST(ITransactionCoordinator, ITC16_GetInDoubtEmpty)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:465
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC16_GetInDoubtEmpty): n/a

#### `TEST(ITransactionCoordinator, ITC17_DuplicateTxnIdInvalidState)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:474
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC17_DuplicateTxnIdInvalidState): n/a

#### `TEST(ITransactionCoordinator, ITC18_CommitUnknownTxn)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:486
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC18_CommitUnknownTxn): n/a

#### `TEST(ITransactionCoordinator, ITC19_AbortUnknownTxn)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:497
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC19_AbortUnknownTxn): n/a

#### `TEST(ITransactionCoordinator, ITC20_PrepareUnknownTxn)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:508
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC20_PrepareUnknownTxn): n/a

#### `TEST(ITransactionCoordinator, ITC21_OptionsIsolationForwarded)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:519
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC21_OptionsIsolationForwarded): n/a

#### `TEST(ITransactionCoordinator, ITC22_OptionsMetadataForwarded)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:533
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC22_OptionsMetadataForwarded): n/a

#### `TEST(ITransactionCoordinator, ITC23_TwoPCCapabilities)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:546
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC23_TwoPCCapabilities): n/a

#### `TEST(ITransactionCoordinator, ITC24_SagaCapabilities)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:561
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC24_SagaCapabilities): n/a

#### `TEST(ITransactionCoordinator, ITC25_PercolatorOptimisticMvcc)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:578
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC25_PercolatorOptimisticMvcc): n/a

#### `TEST(ITransactionCoordinator, ITC26_CalvinDeterministic)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:592
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC26_CalvinDeterministic): n/a

#### `TEST(ITransactionCoordinator, ITC27_ThreePCPreCommit)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:606
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC27_ThreePCPreCommit): n/a

#### `TEST(ITransactionCoordinator, ITC28_SuccessResultCodeNone)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:623
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC28_SuccessResultCodeNone): n/a

#### `TEST(ITransactionCoordinator, ITC29_ErrorCodeEnumeratorsCompile)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:632
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC29_ErrorCodeEnumeratorsCompile): n/a

#### `TEST(ITransactionCoordinator, ITC30_GetInDoubtDescriptors)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:657
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC30_GetInDoubtDescriptors): n/a

#### `TEST(ITransactionCoordinator, ITC31_RecoverInDoubtCount)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:675
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC31_RecoverInDoubtCount): n/a

#### `TEST(ITransactionCoordinator, ITC32_GetStateActive)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:688
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC32_GetStateActive): n/a

#### `TEST(ITransactionCoordinator, ITC33_GetStatePrepared)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:694
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC33_GetStatePrepared): n/a

#### `TEST(ITransactionCoordinator, ITC34_GetStateCompletedAfterCommit)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:701
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC34_GetStateCompletedAfterCommit): n/a

#### `TEST(ITransactionCoordinator, ITC35_GetStateCompletedAfterAbort)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:709
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC35_GetStateCompletedAfterAbort): n/a

#### `TEST(ITransactionCoordinator, ITC36_NonCopyable)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:720
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC36_NonCopyable): n/a

#### `TEST(ITransactionCoordinator, ITC37_InterfaceIsAbstract)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:731
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC37_InterfaceIsAbstract): n/a

#### `TEST(ITransactionCoordinator, ITC38_ProtocolNameNonEmpty)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:740
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC38_ProtocolNameNonEmpty): n/a

#### `TEST(ITransactionCoordinator, ITC39_ProtocolTypeMatchesName)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:749
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC39_ProtocolTypeMatchesName): n/a

#### `TEST(ITransactionCoordinator, ITC40_TwoPCCommitBeforePrepareInvalidState)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:776
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC40_TwoPCCommitBeforePrepareInvalidState): n/a

#### `TEST(ITransactionCoordinator, ITC41_CommitDecidedTrueForCommitting)`
- Source: `tests/transaction/test_itransaction_coordinator.cpp`:791
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator): n/a
  - `<unnamed>` (ITC41_CommitDecidedTrueForCommitting): n/a

### test_transaction_auditor.cpp

#### `TEST(TransactionAuditorTest, AC10_FilterByTimeRange)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC10_FilterByTimeRange): n/a

#### `TEST(TransactionAuditorTest, AC11_LimitRespected)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC11_LimitRespected): n/a

#### `TEST(TransactionAuditorTest, AC12_LimitZeroReturnsAll)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC12_LimitZeroReturnsAll): n/a

#### `TEST(TransactionAuditorTest, AC13_MostRecentFirst)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:284
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC13_MostRecentFirst): n/a

#### `TEST(TransactionAuditorTest, AC14_Size)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC14_Size): n/a

#### `TEST(TransactionAuditorTest, AC15_Clear)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:318
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC15_Clear): n/a

#### `TEST(TransactionAuditorTest, AC16_ExportToKafkaNoTransport)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:333
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC16_ExportToKafkaNoTransport): n/a

#### `TEST(TransactionAuditorTest, AC16b_ExportToKafkaWithTransport)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:389
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC16b_ExportToKafkaWithTransport): n/a

#### `TEST(TransactionAuditorTest, AC16c_ExportToKafkaEmptyLogReturnsOK)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:428
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC16c_ExportToKafkaEmptyLogReturnsOK): n/a

#### `TEST(TransactionAuditorTest, AC17_ExportToS3NoTransport)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:346
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC17_ExportToS3NoTransport): n/a

#### `TEST(TransactionAuditorTest, AC17b_ExportToS3WithTransport)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:409
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC17b_ExportToS3WithTransport): n/a

#### `TEST(TransactionAuditorTest, AC17c_SetNullTransportDisablesExport)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:438
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC17c_SetNullTransportDisablesExport): n/a

#### `TEST(TransactionAuditorTest, AC18_ResultEnumValues)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:456
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC18_ResultEnumValues): n/a

#### `TEST(TransactionAuditorTest, AC19_OperationTypeEnumValues)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:468
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC19_OperationTypeEnumValues): n/a

#### `TEST(TransactionAuditorTest, AC1_DisabledByDefault)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC1_DisabledByDefault): n/a

#### `TEST(TransactionAuditorTest, AC20_ConcurrentRecord)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:479
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC20_ConcurrentRecord): n/a

#### `TEST(TransactionAuditorTest, AC21_AllFieldsPreserved)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:507
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC21_AllFieldsPreserved): n/a

#### `TEST(TransactionAuditorTest, AC22_CombinedUserIdAndTimeRange)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:552
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC22_CombinedUserIdAndTimeRange): n/a

#### `TEST(TransactionAuditorTest, AC23_RecordAfterClear)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:582
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC23_RecordAfterClear): n/a

#### `TEST(TransactionAuditorTest, AC24_OperationsPreserved)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:598
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC24_OperationsPreserved): n/a

#### `TEST(TransactionAuditorTest, AC25_DefaultLimitOf1000)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:620
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC25_DefaultLimitOf1000): n/a

#### `TEST(TransactionAuditorTest, AC2_EnableDisable)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC2_EnableDisable): n/a

#### `TEST(TransactionAuditorTest, AC3_IsEnabled)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC3_IsEnabled): n/a

#### `TEST(TransactionAuditorTest, AC4_RecordAppendsWhenEnabled)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC4_RecordAppendsWhenEnabled): n/a

#### `TEST(TransactionAuditorTest, AC5_RecordNoopWhenDisabled)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC5_RecordNoopWhenDisabled): n/a

#### `TEST(TransactionAuditorTest, AC6_QueryAllRecords)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC6_QueryAllRecords): n/a

#### `TEST(TransactionAuditorTest, AC7_FilterByUserId)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC7_FilterByUserId): n/a

#### `TEST(TransactionAuditorTest, AC8_FilterByStartTime)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC8_FilterByStartTime): n/a

#### `TEST(TransactionAuditorTest, AC9_FilterByEndTime)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditorTest): n/a
  - `<unnamed>` (AC9_FilterByEndTime): n/a

#### `TransactionAuditor::AuditRecord makeRecord(TransactionAuditor::TransactionId txn_id, std::string user_id, TransactionAuditor::AuditRecord::Result result=TransactionAuditor::AuditRecord::Result::COMMITTED, std::chrono::system_clock::time_point ts=std::chrono::system_clock::now(), IsolationLevel isolation=IsolationLevel::ReadCommitted, uint64_t duration_us=1000)`
- Source: `tests/transaction/test_transaction_auditor.cpp`:53
- Brief: n/a
- Parameters:
  - `txn_id` (TransactionAuditor::TransactionId): n/a
  - `user_id` (std::string): n/a
  - `result` (TransactionAuditor::AuditRecord::Result): n/a
  - `ts` (std::chrono::system_clock::time_point): n/a
  - `isolation` (IsolationLevel): n/a
  - `duration_us` (uint64_t): n/a

### test_transaction_batcher.cpp

#### `TEST(TransactionBatcherAdaptiveTest, Adaptive_AdjustmentsIncrement)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:506
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherAdaptiveTest): n/a
  - `<unnamed>` (Adaptive_AdjustmentsIncrement): n/a

#### `TEST(TransactionBatcherConcurrencyTest, ConcurrentSubmitters_AllResolve)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:418
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherConcurrencyTest): n/a
  - `<unnamed>` (ConcurrentSubmitters_AllResolve): n/a

#### `TEST(TransactionBatcherConfigTest, GetBatchConfig_ReturnsClamped)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:541
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherConfigTest): n/a
  - `<unnamed>` (GetBatchConfig_ReturnsClamped): n/a

#### `TEST(TransactionBatcherConfigTest, MaxBatchSizeClamped)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherConfigTest): n/a
  - `<unnamed>` (MaxBatchSizeClamped): n/a

#### `TEST(TransactionBatcherConfigTest, MinBatchSizeCappedByMax)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherConfigTest): n/a
  - `<unnamed>` (MinBatchSizeCappedByMax): n/a

#### `TEST(TransactionBatcherConfigTest, WindowClampedToMax)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherConfigTest): n/a
  - `<unnamed>` (WindowClampedToMax): n/a

#### `TEST(TransactionBatcherConfigTest, WindowClampedToMin)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherConfigTest): n/a
  - `<unnamed>` (WindowClampedToMin): n/a

#### `TEST(TransactionBatcherDefaultTest, DefaultConfig)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherDefaultTest): n/a
  - `<unnamed>` (DefaultConfig): n/a

#### `TEST(TransactionBatcherDestructorTest, Destructor_FlushesRemainingItems)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:561
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherDestructorTest): n/a
  - `<unnamed>` (Destructor_FlushesRemainingItems): n/a

#### `TEST(TransactionBatcherHighThroughputTest, HighThroughput_AllResolve)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:611
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherHighThroughputTest): n/a
  - `<unnamed>` (HighThroughput_AllResolve): n/a

#### `TEST(TransactionBatcherMaxSizeTest, MaxBatchSize_TriggersFlush)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherMaxSizeTest): n/a
  - `<unnamed>` (MaxBatchSize_TriggersFlush): n/a

#### `TEST(TransactionBatcherPolicyTest, GetTablePolicy_UnknownTableReturnsZero)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:406
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherPolicyTest): n/a
  - `<unnamed>` (GetTablePolicy_UnknownTableReturnsZero): n/a

#### `TEST(TransactionBatcherPolicyTest, TablePolicy_OverridesMaxBatchSize)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherPolicyTest): n/a
  - `<unnamed>` (TablePolicy_OverridesMaxBatchSize): n/a

#### `TEST(TransactionBatcherPolicyTest, TablePolicy_OverridesWindow)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherPolicyTest): n/a
  - `<unnamed>` (TablePolicy_OverridesWindow): n/a

#### `TEST(TransactionBatcherPolicyTest, ZeroFieldPolicy_InheritsGlobal)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:595
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherPolicyTest): n/a
  - `<unnamed>` (ZeroFieldPolicy_InheritsGlobal): n/a

#### `TEST_F(TransactionBatcherTest, CStringExceptionInCommitFn_ReturnsError)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (CStringExceptionInCommitFn_ReturnsError): n/a

#### `TEST_F(TransactionBatcherTest, ExceptionInCommitFn_ReturnsError)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:278
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (ExceptionInCommitFn_ReturnsError): n/a

#### `TEST_F(TransactionBatcherTest, FIFO_OrderPreservedWithinBatch)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:467
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (FIFO_OrderPreservedWithinBatch): n/a

#### `TEST_F(TransactionBatcherTest, FailedCommitFn_PropagatesError)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (FailedCommitFn_PropagatesError): n/a

#### `TEST_F(TransactionBatcherTest, Flush_DrainsQueueImmediately)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (Flush_DrainsQueueImmediately): n/a

#### `TEST_F(TransactionBatcherTest, MultipleItems_AllResolve)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (MultipleItems_AllResolve): n/a

#### `TEST_F(TransactionBatcherTest, NullCommitFn_ReturnsError)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (NullCommitFn_ReturnsError): n/a

#### `TEST_F(TransactionBatcherTest, SingleItem_ResolvesOK)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (SingleItem_ResolvesOK): n/a

#### `TEST_F(TransactionBatcherTest, Stats_AvgBatchSizeIsPositive)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (Stats_AvgBatchSizeIsPositive): n/a

#### `TEST_F(TransactionBatcherTest, Stats_AvgLatencyNonNegative)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:362
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (Stats_AvgLatencyNonNegative): n/a

#### `TEST_F(TransactionBatcherTest, Stats_BatchesFlushedIncrements)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:303
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (Stats_BatchesFlushedIncrements): n/a

#### `TEST_F(TransactionBatcherTest, Stats_CommittedAndFailedCounts)`
- Source: `tests/transaction/test_transaction_batcher.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcherTest): n/a
  - `<unnamed>` (Stats_CommittedAndFailedCounts): n/a

### test_transaction_bulk.cpp

#### `TEST_F(BulkTransactionTest, BulkEraseEmptyVectorIsNoOp)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkEraseEmptyVectorIsNoOp): n/a

#### `TEST_F(BulkTransactionTest, BulkEraseFailsOnFinishedTransaction)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkEraseFailsOnFinishedTransaction): n/a

#### `TEST_F(BulkTransactionTest, BulkEraseRejectsEmptyPrimaryKey)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:212
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkEraseRejectsEmptyPrimaryKey): n/a

#### `TEST_F(BulkTransactionTest, BulkEraseRemovesInsertedEntities)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkEraseRemovesInsertedEntities): n/a

#### `TEST_F(BulkTransactionTest, BulkPutAndEraseInSameTransaction)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkPutAndEraseInSameTransaction): n/a

#### `TEST_F(BulkTransactionTest, BulkPutEmptyVectorIsNoOp)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkPutEmptyVectorIsNoOp): n/a

#### `TEST_F(BulkTransactionTest, BulkPutFailsOnFinishedTransaction)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkPutFailsOnFinishedTransaction): n/a

#### `TEST_F(BulkTransactionTest, BulkPutLargeBatchCommit)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:282
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkPutLargeBatchCommit): n/a

#### `TEST_F(BulkTransactionTest, BulkPutMultipleEntitiesCommit)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkPutMultipleEntitiesCommit): n/a

#### `TEST_F(BulkTransactionTest, BulkPutRejectsEmptyPrimaryKey)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkPutRejectsEmptyPrimaryKey): n/a

#### `TEST_F(BulkTransactionTest, BulkPutRollbackLeavesNoData)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkPutRollbackLeavesNoData): n/a

#### `TEST_F(BulkTransactionTest, BulkPutSingleEntity)`
- Source: `tests/transaction/test_transaction_bulk.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (BulkTransactionTest): n/a
  - `<unnamed>` (BulkPutSingleEntity): n/a

### test_transaction_distributed_2pc.cpp

#### `TEST(Distributed2PCPerfTests, AbortWithThreadPool)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:919
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCPerfTests): n/a
  - `<unnamed>` (AbortWithThreadPool): n/a

#### `TEST(Distributed2PCPerfTests, BatchWindowCanBeConfigured)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:823
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCPerfTests): n/a
  - `<unnamed>` (BatchWindowCanBeConfigured): n/a

#### `TEST(Distributed2PCPerfTests, BatchedPrepareWindowGroupsTransactions)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:869
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCPerfTests): n/a
  - `<unnamed>` (BatchedPrepareWindowGroupsTransactions): n/a

#### `TEST(Distributed2PCPerfTests, ConcurrentTxnsWithThreadPool)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:966
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCPerfTests): n/a
  - `<unnamed>` (ConcurrentTxnsWithThreadPool): n/a

#### `TEST(Distributed2PCPerfTests, DefaultConfigNoBatchWindow)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:817
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCPerfTests): n/a
  - `<unnamed>` (DefaultConfigNoBatchWindow): n/a

#### `TEST(Distributed2PCPerfTests, DefaultWorkerThreadCount)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:830
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCPerfTests): n/a
  - `<unnamed>` (DefaultWorkerThreadCount): n/a

#### `TEST(Distributed2PCPerfTests, FullHappyPathWithThreadPool)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:843
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCPerfTests): n/a
  - `<unnamed>` (FullHappyPathWithThreadPool): n/a

#### `TEST(Distributed2PCPerfTests, LegacyModeWorkerCount0)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:945
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCPerfTests): n/a
  - `<unnamed>` (LegacyModeWorkerCount0): n/a

#### `TEST(Distributed2PCPerfTests, P99LatencyFiveShards)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1009
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCPerfTests): n/a
  - `<unnamed>` (P99LatencyFiveShards): n/a

#### `TEST(Distributed2PCPerfTests, ThroughputAtLeast10kOpsPerSec)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1061
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCPerfTests): n/a
  - `<unnamed>` (ThroughputAtLeast10kOpsPerSec): n/a

#### `TEST(Distributed2PCPerfTests, WorkerThreadCountCanBeConfigured)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:836
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCPerfTests): n/a
  - `<unnamed>` (WorkerThreadCountCanBeConfigured): n/a

#### `TEST(Distributed2PCWaveAGapTests, BatchedPrepareFutureTimeout)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:2056
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCWaveAGapTests): n/a
  - `<unnamed>` (BatchedPrepareFutureTimeout): n/a

#### `TEST(Distributed2PCWaveAGapTests, NonBatchedPrepareSucceedsNormally)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:2103
- Brief: n/a
- Parameters:
  - `<unnamed>` (Distributed2PCWaveAGapTests): n/a
  - `<unnamed>` (NonBatchedPrepareSucceedsNormally): n/a

#### `TEST_F(DistributedTxnManagerTest, AbortBeforePrepare)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:792
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (AbortBeforePrepare): n/a

#### `TEST_F(DistributedTxnManagerTest, AbortDistributedCallsOnAbortOnAllParticipants)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (AbortDistributedCallsOnAbortOnAllParticipants): n/a

#### `TEST_F(DistributedTxnManagerTest, ActiveTransactionCountAfterBegin)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:706
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (ActiveTransactionCountAfterBegin): n/a

#### `TEST_F(DistributedTxnManagerTest, AffectedKeysForwardedToPrepare)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:719
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (AffectedKeysForwardedToPrepare): n/a

#### `TEST_F(DistributedTxnManagerTest, ApplyAbortReturnsOkForKnownTxn)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (ApplyAbortReturnsOkForKnownTxn): n/a

#### `TEST_F(DistributedTxnManagerTest, ApplyCommitReturnsErrorForUnknownTxn)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:344
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (ApplyCommitReturnsErrorForUnknownTxn): n/a

#### `TEST_F(DistributedTxnManagerTest, ApplyCommitReturnsOkForKnownTxn)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (ApplyCommitReturnsOkForKnownTxn): n/a

#### `TEST_F(DistributedTxnManagerTest, BatchedFiveParticipantTransaction)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:557
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (BatchedFiveParticipantTransaction): n/a

#### `TEST_F(DistributedTxnManagerTest, BeginDistributedAllowsNewTxnAfterCommitReleasesCapacity)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:650
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (BeginDistributedAllowsNewTxnAfterCommitReleasesCapacity): n/a

#### `TEST_F(DistributedTxnManagerTest, BeginDistributedEnforcesMaxActiveTransactions)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:637
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (BeginDistributedEnforcesMaxActiveTransactions): n/a

#### `TEST_F(DistributedTxnManagerTest, BeginDistributedIdsAreUnique)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (BeginDistributedIdsAreUnique): n/a

#### `TEST_F(DistributedTxnManagerTest, BeginDistributedReturnsNonEmptyId)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (BeginDistributedReturnsNonEmptyId): n/a

#### `TEST_F(DistributedTxnManagerTest, BeginWithNoParticipantsThrows)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:633
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (BeginWithNoParticipantsThrows): n/a

#### `TEST_F(DistributedTxnManagerTest, CC1_SuccessfulWALWriteDoesNotSuppressPhase2)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1474
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (CC1_SuccessfulWALWriteDoesNotSuppressPhase2): n/a

#### `TEST_F(DistributedTxnManagerTest, CheckTimeoutsAbortsExpiredTransactions)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (CheckTimeoutsAbortsExpiredTransactions): n/a

#### `TEST_F(DistributedTxnManagerTest, CheckTimeoutsDoesNotCountIncompleteAbortDelivery)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (CheckTimeoutsDoesNotCountIncompleteAbortDelivery): n/a

#### `TEST_F(DistributedTxnManagerTest, CommitCallsOnCommitOnAllParticipants)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (CommitCallsOnCommitOnAllParticipants): n/a

#### `TEST_F(DistributedTxnManagerTest, CommitWithoutPrepareReturnsError)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:696
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (CommitWithoutPrepareReturnsError): n/a

#### `TEST_F(DistributedTxnManagerTest, ConcurrentTransactionsAllSucceed)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:515
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (ConcurrentTransactionsAllSucceed): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM1_MixedLocalAndRemoteVotesAbort)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1150
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM1_MixedLocalAndRemoteVotesAbort): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM1_Phase2BridgeDoesNotBypassMissingPhase1Vote)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1163
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM1_Phase2BridgeDoesNotBypassMissingPhase1Vote): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM1_RemoteParticipantWithoutCallbackVotesAbort)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1142
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM1_RemoteParticipantWithoutCallbackVotesAbort): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM2_RecoveryBroadcastsAbortToInMemoryParticipants)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1203
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM2_RecoveryBroadcastsAbortToInMemoryParticipants): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM2_RecoveryDoesNotMarkResolvedWhenAbortDeliveryFails)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1221
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM2_RecoveryDoesNotMarkResolvedWhenAbortDeliveryFails): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM3_InstanceBridgeTakesPriorityOverStaticBridge)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:2011
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM3_InstanceBridgeTakesPriorityOverStaticBridge): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM3_IsParticipantAliveDistinguishesLocalAndRemote)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1452
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM3_IsParticipantAliveDistinguishesLocalAndRemote): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM3_LivenessBridgeInstanceCStringExceptionIsNotAlive)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1934
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM3_LivenessBridgeInstanceCStringExceptionIsNotAlive): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM3_LivenessBridgeInstanceExceptionIsNotAlive)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1912
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM3_LivenessBridgeInstanceExceptionIsNotAlive): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM3_LivenessBridgeInstanceReturnsFalse)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1889
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM3_LivenessBridgeInstanceReturnsFalse): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM3_LivenessBridgeInstanceReturnsTrue)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1857
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM3_LivenessBridgeInstanceReturnsTrue): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM3_StaticLivenessBridgeIsConsulted)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1957
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM3_StaticLivenessBridgeIsConsulted): n/a

#### `TEST_F(DistributedTxnManagerTest, DTM3_StaticLivenessBridgeStringExceptionIsNotAlive)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1986
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DTM3_StaticLivenessBridgeStringExceptionIsNotAlive): n/a

#### `TEST_F(DistributedTxnManagerTest, DoubleCommitReturnsError)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:779
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (DoubleCommitReturnsError): n/a

#### `TEST_F(DistributedTxnManagerTest, GetTransactionUnknownReturnsNullopt)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:747
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (GetTransactionUnknownReturnsNullopt): n/a

#### `TEST_F(DistributedTxnManagerTest, IdempotentAbort)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:669
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (IdempotentAbort): n/a

#### `TEST_F(DistributedTxnManagerTest, IsParticipantAliveReturnsTrueByDefault)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:397
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (IsParticipantAliveReturnsTrueByDefault): n/a

#### `TEST_F(DistributedTxnManagerTest, PartialCommitAutoRollback)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:469
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (PartialCommitAutoRollback): n/a

#### `TEST_F(DistributedTxnManagerTest, Phase2DeadlineExpiryFailsClosedOnCommit)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (Phase2DeadlineExpiryFailsClosedOnCommit): n/a

#### `TEST_F(DistributedTxnManagerTest, PrepareCommitLatency)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:492
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (PrepareCommitLatency): n/a

#### `TEST_F(DistributedTxnManagerTest, PrepareExceptionTreatedAsAbortVote)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:415
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (PrepareExceptionTreatedAsAbortVote): n/a

#### `TEST_F(DistributedTxnManagerTest, PreparePhaseAbortVote)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (PreparePhaseAbortVote): n/a

#### `TEST_F(DistributedTxnManagerTest, PreparePhaseAllCommitVotes)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (PreparePhaseAllCommitVotes): n/a

#### `TEST_F(DistributedTxnManagerTest, PrepareTimeoutAbortsTransaction)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:439
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (PrepareTimeoutAbortsTransaction): n/a

#### `TEST_F(DistributedTxnManagerTest, PrepareUnknownTxnReturnsError)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:687
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (PrepareUnknownTxnReturnsError): n/a

#### `TEST_F(DistributedTxnManagerTest, RecoverInDoubtTransactionsWithoutWAL)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:405
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (RecoverInDoubtTransactionsWithoutWAL): n/a

#### `TEST_F(DistributedTxnManagerTest, RecoveryReplaysCommitForInMemoryCommittingTransaction)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1239
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (RecoveryReplaysCommitForInMemoryCommittingTransaction): n/a

#### `TEST_F(DistributedTxnManagerTest, RecoveryWithWALDoesNotRecountCommitDecisionAcrossRestartWithoutLiveTxn)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1392
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (RecoveryWithWALDoesNotRecountCommitDecisionAcrossRestartWithoutLiveTxn): n/a

#### `TEST_F(DistributedTxnManagerTest, RecoveryWithWALIsIdempotentAcrossRestartForPreparedTxn)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1353
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (RecoveryWithWALIsIdempotentAcrossRestartForPreparedTxn): n/a

#### `TEST_F(DistributedTxnManagerTest, RecoveryWithWALKeepsPreparedTxnAbortingOnAbortDeliveryFailure)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1320
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (RecoveryWithWALKeepsPreparedTxnAbortingOnAbortDeliveryFailure): n/a

#### `TEST_F(DistributedTxnManagerTest, RecoveryWithWALReplaysCommitForInMemoryCommittingTransaction)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1276
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (RecoveryWithWALReplaysCommitForInMemoryCommittingTransaction): n/a

#### `TEST_F(DistributedTxnManagerTest, SingleParticipantHappyPath)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:755
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (SingleParticipantHappyPath): n/a

#### `TEST_F(DistributedTxnManagerTest, StatisticsCountAborts)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:620
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (StatisticsCountAborts): n/a

#### `TEST_F(DistributedTxnManagerTest, StatisticsCountCommits)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:610
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (StatisticsCountCommits): n/a

#### `TEST_F(DistributedTxnManagerTest, Stub279_Phase1RpcFnExceptionIsAbortVote)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1665
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (Stub279_Phase1RpcFnExceptionIsAbortVote): n/a

#### `TEST_F(DistributedTxnManagerTest, Stub279_Phase1RpcFnNoVoteAbortsTransaction)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1623
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (Stub279_Phase1RpcFnNoVoteAbortsTransaction): n/a

#### `TEST_F(DistributedTxnManagerTest, Stub279_Phase1RpcFnYesVoteAllowsCommit)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1579
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (Stub279_Phase1RpcFnYesVoteAllowsCommit): n/a

#### `TEST_F(DistributedTxnManagerTest, Stub279_PureRemoteTransactionSucceedsWithPhase1Rpc)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1733
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (Stub279_PureRemoteTransactionSucceedsWithPhase1Rpc): n/a

#### `TEST_F(DistributedTxnManagerTest, Stub279_RemoteAbortUsesConfiguredPhase2Dispatcher)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1490
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (Stub279_RemoteAbortUsesConfiguredPhase2Dispatcher): n/a

#### `TEST_F(DistributedTxnManagerTest, Stub279_RemoteCommitUsesConfiguredPhase2Dispatcher)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1529
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (Stub279_RemoteCommitUsesConfiguredPhase2Dispatcher): n/a

#### `TEST_F(DistributedTxnManagerTest, Stub279_RemotePhase1DispatchCommit)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1693
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (Stub279_RemotePhase1DispatchCommit): n/a

#### `TEST_F(DistributedTxnManagerTest, Stub279_RemotePhase2DispatchRetriesThreeTimesOnPersistentFailure)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1816
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (Stub279_RemotePhase2DispatchRetriesThreeTimesOnPersistentFailure): n/a

#### `TEST_F(DistributedTxnManagerTest, Stub279_StaticPhase1FnCommit)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1780
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (Stub279_StaticPhase1FnCommit): n/a

#### `TEST_F(DistributedTxnManagerTest, VoteOnPrepareErrorForUnknownTxn)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:601
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (VoteOnPrepareErrorForUnknownTxn): n/a

#### `TEST_F(DistributedTxnManagerTest, VoteOnPrepareRegistersCOMMITVote)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:583
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTxnManagerTest): n/a
  - `<unnamed>` (VoteOnPrepareRegistersCOMMITVote): n/a

#### `std::string beginDistributedWithExplicitTestConsistency(DistributedTransactionManager &mgr, const std::vector< Participant > &participants)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:137
- Brief: n/a
- Parameters:
  - `mgr` (DistributedTransactionManager &): n/a
  - `participants` (const std::vector< Participant > &): n/a

#### `Participant makeParticipant(const std::string &node_id, IDistributedParticipantCallback *cb, std::set< std::string > keys={"key1"})`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:124
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a
  - `cb` (IDistributedParticipantCallback *): n/a
  - `keys` (std::set< std::string >): n/a

#### `Participant makeRemoteParticipant(const std::string &node_id, std::set< std::string > keys={"key1"})`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:1128
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a
  - `keys` (std::set< std::string >): n/a

#### `std::string makeTempWalDir(const std::string &suffix)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:145
- Brief: n/a
- Parameters:
  - `suffix` (const std::string &): n/a

#### `void removeTempWalDir(const std::string &wal_dir)`
- Source: `tests/transaction/test_transaction_distributed_2pc.cpp`:155
- Brief: n/a
- Parameters:
  - `wal_dir` (const std::string &): n/a

### test_transaction_distributed_phase2.cpp

#### `TEST(TransactionDistributedPhase2Contract, DistributedTxnStatusCarriesCanonicalRetryMetadata)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:488
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Contract): n/a
  - `<unnamed>` (DistributedTxnStatusCarriesCanonicalRetryMetadata): n/a

#### `TEST_F(TransactionDistributedPhase2Test, CoordinatorProtocol_2PC_AbortPath)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:206
- Brief: Validates explicit abort after begin without prepare. @acceptance AC-4.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (CoordinatorProtocol_2PC_AbortPath): n/a
- Details: TestCoordinatorProtocol_2PC_AbortPath

#### `TEST_F(TransactionDistributedPhase2Test, CoordinatorProtocol_2PC_HappyPath)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:138
- Brief: Validates basic 2PC: begin → prepare → commit, all participants vote COMMIT. @acceptance AC-4.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (CoordinatorProtocol_2PC_HappyPath): n/a
- Details: TestCoordinatorProtocol_2PC_HappyPath

#### `TEST_F(TransactionDistributedPhase2Test, CoordinatorProtocol_2PC_ParticipantAbort)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:162
- Brief: Validates 2PC behavior when one participant votes ABORT during prepare. @acceptance AC-4.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (CoordinatorProtocol_2PC_ParticipantAbort): n/a
- Details: TestCoordinatorProtocol_2PC_ParticipantAbort

#### `TEST_F(TransactionDistributedPhase2Test, CoordinatorProtocol_2PC_ParticipantCrash)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:185
- Brief: Validates 2PC behavior when participant throws (crash) during prepare. @acceptance AC-4.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (CoordinatorProtocol_2PC_ParticipantCrash): n/a
- Details: TestCoordinatorProtocol_2PC_ParticipantCrash

#### `TEST_F(TransactionDistributedPhase2Test, FailureRecovery_PrepareAbortFollowedByNewTransaction)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:292
- Brief: After a prepare failure the manager accepts new transactions. @acceptance AC-5.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (FailureRecovery_PrepareAbortFollowedByNewTransaction): n/a
- Details: TestFailureRecovery_PrepareAbortFollowedByNewTransaction

#### `TEST_F(TransactionDistributedPhase2Test, InDoubtReconciliation_CommitCountTracked)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:319
- Brief: Multiple successful commits increment the internal commit counter. @acceptance AC-6.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (InDoubtReconciliation_CommitCountTracked): n/a
- Details: TestInDoubtReconciliation_CommitCountTracked

#### `TEST_F(TransactionDistributedPhase2Test, InDoubtReconciliation_ParticipantRecovery)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:374
- Brief: Abort callbacks track in-doubt resolutions correctly. @acceptance AC-6.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (InDoubtReconciliation_ParticipantRecovery): n/a
- Details: TestInDoubtReconciliation_ParticipantRecovery

#### `TEST_F(TransactionDistributedPhase2Test, InDoubtReconciliation_WalReplay)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:344
- Brief: WAL-backed coordinator can be constructed without errors. @acceptance AC-6.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (InDoubtReconciliation_WalReplay): n/a
- Details: TestInDoubtReconciliation_WalReplay

#### `TEST_F(TransactionDistributedPhase2Test, RetryBehavior_IdempotentAbort)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:278
- Brief: Aborting an already-aborted transaction is safe (idempotent). @acceptance AC-5.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (RetryBehavior_IdempotentAbort): n/a
- Details: TestRetryBehavior_IdempotentAbort

#### `TEST_F(TransactionDistributedPhase2Test, StressTest_ConcurrentDistributedTransactions)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:398
- Brief: Stress test: multiple concurrent 2PC transactions. @acceptance AC-4.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (StressTest_ConcurrentDistributedTransactions): n/a
- Details: TestStressTest_ConcurrentDistributedTransactions

#### `TEST_F(TransactionDistributedPhase2Test, StressTest_HighContentionWithAbortingParticipants)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:447
- Brief: Stress test with mixed commit/abort participants under high concurrency. @acceptance AC-4, AC-5, AC-6.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (StressTest_HighContentionWithAbortingParticipants): n/a
- Details: TestStressTest_HighContentionWithAbortingParticipants

#### `TEST_F(TransactionDistributedPhase2Test, TimeoutDeterminism_RepeatedRetries_ConsistentErrors)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:252
- Brief: ABORT votes produce consistent results across multiple retried transactions. @acceptance AC-5.
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (TimeoutDeterminism_RepeatedRetries_ConsistentErrors): n/a
- Details: TestTimeoutDeterminism_RepeatedRetries_ConsistentErrors

#### `TEST_F(TransactionDistributedPhase2Test, TimeoutDeterminism_ShortTimeoutAbortsTransaction)`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:230
- Brief: checkTimeouts() aborts transactions that have exceeded their timeout. @acceptance AC-5
- Parameters:
  - `<unnamed>` (TransactionDistributedPhase2Test): n/a
  - `<unnamed>` (TimeoutDeterminism_ShortTimeoutAbortsTransaction): n/a
- Details: TestTimeoutDeterminism_ShortTimeoutAbortsTransaction

#### `DistributedTxnManagerConfig makeDefaultConfig()`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:97
- Brief: n/a
- Parameters: none

#### `Participant makeParticipant(const std::string &node_id, IDistributedParticipantCallback *cb, std::set< std::string > keys={"key1"})`
- Source: `tests/transaction/test_transaction_distributed_phase2.cpp`:85
- Brief: n/a
- Parameters:
  - `node_id` (const std::string &): n/a
  - `cb` (IDistributedParticipantCallback *): n/a
  - `keys` (std::set< std::string >): n/a

### test_transaction_error_path_determinism_phase1.cpp

#### `TEST_F(TransactionErrorPathDeterminismPhase1Test, ErrorPropagation_ConcurrentErrorScenarios)`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:189
- Brief: AC-2: Error propagation - concurrent error scenarios.
- Parameters:
  - `<unnamed>` (TransactionErrorPathDeterminismPhase1Test): n/a
  - `<unnamed>` (ErrorPropagation_ConcurrentErrorScenarios): n/a

#### `TEST_F(TransactionErrorPathDeterminismPhase1Test, RecoveryPath_ConsistentErrorRecovery)`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:242
- Brief: AC-2: Recovery path - consistent recovery from error states.
- Parameters:
  - `<unnamed>` (TransactionErrorPathDeterminismPhase1Test): n/a
  - `<unnamed>` (RecoveryPath_ConsistentErrorRecovery): n/a

#### `TEST_F(TransactionErrorPathDeterminismPhase1Test, RecoveryPath_MultipleConsecutiveRecoveries)`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:266
- Brief: AC-2: Recovery path - multiple consecutive error recoveries.
- Parameters:
  - `<unnamed>` (TransactionErrorPathDeterminismPhase1Test): n/a
  - `<unnamed>` (RecoveryPath_MultipleConsecutiveRecoveries): n/a

#### `TEST_F(TransactionErrorPathDeterminismPhase1Test, RetryBehavior_ConsistentRetryOutcomes)`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:293
- Brief: AC-2: Retry behavior - consistent retry outcomes.
- Parameters:
  - `<unnamed>` (TransactionErrorPathDeterminismPhase1Test): n/a
  - `<unnamed>` (RetryBehavior_ConsistentRetryOutcomes): n/a

#### `TEST_F(TransactionErrorPathDeterminismPhase1Test, RetryBehavior_TimeoutRetryConsistency)`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:315
- Brief: AC-7: Retry behavior - timeout retry consistency.
- Parameters:
  - `<unnamed>` (TransactionErrorPathDeterminismPhase1Test): n/a
  - `<unnamed>` (RetryBehavior_TimeoutRetryConsistency): n/a

#### `TEST_F(TransactionErrorPathDeterminismPhase1Test, RollbackDeterminism_ConsistentBehavior)`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:138
- Brief: AC-2: Rollback determinism - consistent behavior.
- Parameters:
  - `<unnamed>` (TransactionErrorPathDeterminismPhase1Test): n/a
  - `<unnamed>` (RollbackDeterminism_ConsistentBehavior): n/a

#### `TEST_F(TransactionErrorPathDeterminismPhase1Test, RollbackDeterminism_StatusMessagesAreConsistent)`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:156
- Brief: AC-2: Rollback determinism - consecutive double-rollback returns consistent false.
- Parameters:
  - `<unnamed>` (TransactionErrorPathDeterminismPhase1Test): n/a
  - `<unnamed>` (RollbackDeterminism_StatusMessagesAreConsistent): n/a

#### `TEST_F(TransactionErrorPathDeterminismPhase1Test, StressTest_HighFrequencyErrorPathExercise)`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:352
- Brief: AC-2, AC-7: Stress test - high-frequency error path exercise.
- Parameters:
  - `<unnamed>` (TransactionErrorPathDeterminismPhase1Test): n/a
  - `<unnamed>` (StressTest_HighFrequencyErrorPathExercise): n/a

#### `TEST_F(TransactionErrorPathDeterminismPhase1Test, TimeoutHandling_ConsistentErrorStatus)`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:72
- Brief: AC-7: Timeout handling - consistent error status.
- Parameters:
  - `<unnamed>` (TransactionErrorPathDeterminismPhase1Test): n/a
  - `<unnamed>` (TimeoutHandling_ConsistentErrorStatus): n/a

#### `TEST_F(TransactionErrorPathDeterminismPhase1Test, TimeoutHandling_DeterministicAcrossAttempts)`
- Source: `tests/transaction/test_transaction_error_path_determinism_phase1.cpp`:97
- Brief: AC-7: Timeout handling - deterministic behavior across multiple attempts.
- Parameters:
  - `<unnamed>` (TransactionErrorPathDeterminismPhase1Test): n/a
  - `<unnamed>` (TimeoutHandling_DeterministicAcrossAttempts): n/a

### test_transaction_highcardinality_stress.cpp

#### `TEST(TransactionStress, Concurrent2PCStress)`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionStress): n/a
  - `<unnamed>` (Concurrent2PCStress): n/a

#### `TEST(TransactionStress, ConflictResolutionStress)`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionStress): n/a
  - `<unnamed>` (ConflictResolutionStress): n/a

#### `TEST(TransactionStress, HighCardinalityCommit)`
- Source: `tests/transaction/test_transaction_highcardinality_stress.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionStress): n/a
  - `<unnamed>` (HighCardinalityCommit): n/a

### test_transaction_isolation_contention_phase1.cpp

#### `TEST_F(TransactionIsolationContentionPhase1Test, IsolationEdgeCase_DirtyReadPrevention)`
- Source: `tests/transaction/test_transaction_isolation_contention_phase1.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionIsolationContentionPhase1Test): n/a
  - `<unnamed>` (IsolationEdgeCase_DirtyReadPrevention): n/a

#### `TEST_F(TransactionIsolationContentionPhase1Test, IsolationEdgeCase_NonRepeatableReadPrevention)`
- Source: `tests/transaction/test_transaction_isolation_contention_phase1.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionIsolationContentionPhase1Test): n/a
  - `<unnamed>` (IsolationEdgeCase_NonRepeatableReadPrevention): n/a

#### `TEST_F(TransactionIsolationContentionPhase1Test, IsolationEdgeCase_PhantomReadPrevention)`
- Source: `tests/transaction/test_transaction_isolation_contention_phase1.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionIsolationContentionPhase1Test): n/a
  - `<unnamed>` (IsolationEdgeCase_PhantomReadPrevention): n/a

#### `TEST_F(TransactionIsolationContentionPhase1Test, LockContention_DeadlockDetection)`
- Source: `tests/transaction/test_transaction_isolation_contention_phase1.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionIsolationContentionPhase1Test): n/a
  - `<unnamed>` (LockContention_DeadlockDetection): n/a

#### `TEST_F(TransactionIsolationContentionPhase1Test, LockContention_HighConcurrentWriteLoad)`
- Source: `tests/transaction/test_transaction_isolation_contention_phase1.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionIsolationContentionPhase1Test): n/a
  - `<unnamed>` (LockContention_HighConcurrentWriteLoad): n/a

#### `TEST_F(TransactionIsolationContentionPhase1Test, StressTest_MixedIsolationLevelsUnderContention)`
- Source: `tests/transaction/test_transaction_isolation_contention_phase1.cpp`:335
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionIsolationContentionPhase1Test): n/a
  - `<unnamed>` (StressTest_MixedIsolationLevelsUnderContention): n/a

#### `TEST_F(TransactionIsolationContentionPhase1Test, StressTest_SimultaneousTransactionsWithContention)`
- Source: `tests/transaction/test_transaction_isolation_contention_phase1.cpp`:278
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionIsolationContentionPhase1Test): n/a
  - `<unnamed>` (StressTest_SimultaneousTransactionsWithContention): n/a

#### `TEST_F(TransactionIsolationContentionPhase1Test, TimeoutDeterminism_RetryConsistency)`
- Source: `tests/transaction/test_transaction_isolation_contention_phase1.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionIsolationContentionPhase1Test): n/a
  - `<unnamed>` (TimeoutDeterminism_RetryConsistency): n/a

#### `TEST_F(TransactionIsolationContentionPhase1Test, TimeoutDeterminism_ShortTimeoutUnderContention)`
- Source: `tests/transaction/test_transaction_isolation_contention_phase1.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionIsolationContentionPhase1Test): n/a
  - `<unnamed>` (TimeoutDeterminism_ShortTimeoutUnderContention): n/a

### test_transaction_isolation_levels.cpp

#### `TEST(IsolationLevel2PL, GrowingPhaseAllowsAcquisition)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevel2PL): n/a
  - `<unnamed>` (GrowingPhaseAllowsAcquisition): n/a

#### `TEST(IsolationLevel2PL, ShrinkingPhaseAllowsRelease)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevel2PL): n/a
  - `<unnamed>` (ShrinkingPhaseAllowsRelease): n/a

#### `TEST(IsolationLevel2PL, ShrinkingPhaseBlocksAcquisition)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevel2PL): n/a
  - `<unnamed>` (ShrinkingPhaseBlocksAcquisition): n/a

#### `TEST(IsolationLevelComplianceRC, ReadCommittedAfterReleaseReaderSucceeds)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelComplianceRC): n/a
  - `<unnamed>` (ReadCommittedAfterReleaseReaderSucceeds): n/a

#### `TEST(IsolationLevelComplianceRC, ReadCommittedAllowsNonRepeatableReads)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelComplianceRC): n/a
  - `<unnamed>` (ReadCommittedAllowsNonRepeatableReads): n/a

#### `TEST(IsolationLevelComplianceRC, ReadCommittedWriterBlocksReader)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelComplianceRC): n/a
  - `<unnamed>` (ReadCommittedWriterBlocksReader): n/a

#### `TEST(IsolationLevelComplianceRR, RepeatableReadHoldsSharedLock)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelComplianceRR): n/a
  - `<unnamed>` (RepeatableReadHoldsSharedLock): n/a

#### `TEST(IsolationLevelComplianceSZ, SerializableAfterCommitOtherCanRead)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelComplianceSZ): n/a
  - `<unnamed>` (SerializableAfterCommitOtherCanRead): n/a

#### `TEST(IsolationLevelComplianceSZ, SerializableExclusivePreventsAllAccess)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelComplianceSZ): n/a
  - `<unnamed>` (SerializableExclusivePreventsAllAccess): n/a

#### `TEST(IsolationLevelEnum, AllFourLevelsDefined)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:10
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelEnum): n/a
  - `<unnamed>` (AllFourLevelsDefined): n/a

#### `TEST(IsolationLevelEnum, ExactNumericValues)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelEnum): n/a
  - `<unnamed>` (ExactNumericValues): n/a

#### `TEST(IsolationLevelEnum, LegacyAliasesMatch)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:19
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelEnum): n/a
  - `<unnamed>` (LegacyAliasesMatch): n/a

#### `TEST(IsolationLevelEnum, ReadUncommittedIsLowest)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelEnum): n/a
  - `<unnamed>` (ReadUncommittedIsLowest): n/a

#### `TEST(IsolationLevelEnum, SerializableIsHighest)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelEnum): n/a
  - `<unnamed>` (SerializableIsHighest): n/a

#### `TEST(IsolationLevelEnum, StrictnessOrder)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:26
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelEnum): n/a
  - `<unnamed>` (StrictnessOrder): n/a

#### `TEST(IsolationLevelEnum, ValueTwoIsUnused)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelEnum): n/a
  - `<unnamed>` (ValueTwoIsUnused): n/a

#### `TEST(IsolationLevelIntent, ExclusiveBlocksIntentShared)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelIntent): n/a
  - `<unnamed>` (ExclusiveBlocksIntentShared): n/a

#### `TEST(IsolationLevelIntent, IntentExclusiveCompatibleWithIntentExclusive)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelIntent): n/a
  - `<unnamed>` (IntentExclusiveCompatibleWithIntentExclusive): n/a

#### `TEST(IsolationLevelIntent, IntentExclusiveIncompatibleWithShared)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelIntent): n/a
  - `<unnamed>` (IntentExclusiveIncompatibleWithShared): n/a

#### `TEST(IsolationLevelIntent, IntentSharedCompatibleWithShared)`
- Source: `tests/transaction/test_transaction_isolation_levels.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (IsolationLevelIntent): n/a
  - `<unnamed>` (IntentSharedCompatibleWithShared): n/a

### test_transaction_lifecycle_phase1.cpp

#### `TEST_F(TransactionLifecyclePhase1Test, ErrorPathDeterminism_LargeStateTransitionVolume)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:349
- Brief: AC-2: Error path determinism - Large number of state transitions.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (ErrorPathDeterminism_LargeStateTransitionVolume): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, ErrorPathDeterminism_TimeoutBehavior)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:318
- Brief: AC-2: Error path determinism - Timeout behavior.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (ErrorPathDeterminism_TimeoutBehavior): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, IsolationLevel_MixedConcurrentLevels)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:258
- Brief: AC-3: Multiple isolation levels in concurrent transactions.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (IsolationLevel_MixedConcurrentLevels): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, IsolationLevel_ReadCommitted_BasicBehavior)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:221
- Brief: AC-3: Isolation level behavior - READ_COMMITTED basic test.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (IsolationLevel_ReadCommitted_BasicBehavior): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, IsolationLevel_Serializable_SSIBehavior)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:246
- Brief: AC-3: Isolation level behavior - SERIALIZABLE SSI basic test.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (IsolationLevel_Serializable_SSIBehavior): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, IsolationLevel_Snapshot_MVCCBehavior)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:233
- Brief: AC-3: Isolation level behavior - SNAPSHOT (MVCC) basic test.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (IsolationLevel_Snapshot_MVCCBehavior): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, LifecycleInvariants_ConcurrentTransactions)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:159
- Brief: AC-1: ACID lifecycle - Multiple concurrent transactions.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (LifecycleInvariants_ConcurrentTransactions): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, LifecycleStateTransitions_BeginToAbort)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:90
- Brief: AC-2: Begin/Prepare/Commit/Abort state machine correctness - Rollback path.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (LifecycleStateTransitions_BeginToAbort): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, LifecycleStateTransitions_BeginToCommit)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:69
- Brief: AC-1: ACID lifecycle isolation enforcement - Basic state transitions.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (LifecycleStateTransitions_BeginToCommit): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, LifecycleStateTransitions_CommitAfterRollbackPrevention)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:143
- Brief: AC-2: Invalid state transitions - commit after rollback prevention.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (LifecycleStateTransitions_CommitAfterRollbackPrevention): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, LifecycleStateTransitions_DoubleCommitPrevention)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:111
- Brief: AC-2: Invalid state transitions must be detected and rejected.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (LifecycleStateTransitions_DoubleCommitPrevention): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, LifecycleStateTransitions_DoubleRollbackPrevention)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:127
- Brief: AC-2: Invalid state transitions - double rollback prevention.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (LifecycleStateTransitions_DoubleRollbackPrevention): n/a

#### `TEST_F(TransactionLifecyclePhase1Test, StressTest_HighTransactionCreationRate)`
- Source: `tests/transaction/test_transaction_lifecycle_phase1.cpp`:382
- Brief: AC-1: Stress test - High transaction creation rate.
- Parameters:
  - `<unnamed>` (TransactionLifecyclePhase1Test): n/a
  - `<unnamed>` (StressTest_HighTransactionCreationRate): n/a

### test_transaction_manager.cpp

#### `TEST_F(TransactionManagerTest, AtomicMultiEntityCommit)`
- Source: `tests/transaction/test_transaction_manager.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (AtomicMultiEntityCommit): n/a

#### `TEST_F(TransactionManagerTest, AtomicRollbackPreventsPersistence)`
- Source: `tests/transaction/test_transaction_manager.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (AtomicRollbackPreventsPersistence): n/a

#### `TEST_F(TransactionManagerTest, AutoRollbackOnDestruction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:461
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (AutoRollbackOnDestruction): n/a

#### `TEST_F(TransactionManagerTest, BeginMultipleTransactions)`
- Source: `tests/transaction/test_transaction_manager.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (BeginMultipleTransactions): n/a

#### `TEST_F(TransactionManagerTest, BeginTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (BeginTransaction): n/a

#### `TEST_F(TransactionManagerTest, CleanupOldTransactions)`
- Source: `tests/transaction/test_transaction_manager.cpp`:448
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (CleanupOldTransactions): n/a

#### `TEST_F(TransactionManagerTest, CommitNonExistentTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:412
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (CommitNonExistentTransaction): n/a

#### `TEST_F(TransactionManagerTest, CommitTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (CommitTransaction): n/a

#### `TEST_F(TransactionManagerTest, ConcurrentTransactionsNonConflicting)`
- Source: `tests/transaction/test_transaction_manager.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (ConcurrentTransactionsNonConflicting): n/a

#### `TEST_F(TransactionManagerTest, DoubleCommit)`
- Source: `tests/transaction/test_transaction_manager.cpp`:418
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (DoubleCommit): n/a

#### `TEST_F(TransactionManagerTest, ExplainActiveTransactionWriteSet)`
- Source: `tests/transaction/test_transaction_manager.cpp`:603
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (ExplainActiveTransactionWriteSet): n/a

#### `TEST_F(TransactionManagerTest, ExplainEmptyWriteSet)`
- Source: `tests/transaction/test_transaction_manager.cpp`:675
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (ExplainEmptyWriteSet): n/a

#### `TEST_F(TransactionManagerTest, ExplainFinishedTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:650
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (ExplainFinishedTransaction): n/a

#### `TEST_F(TransactionManagerTest, ExplainIsolationLevelReported)`
- Source: `tests/transaction/test_transaction_manager.cpp`:642
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (ExplainIsolationLevelReported): n/a

#### `TEST_F(TransactionManagerTest, ExplainNotFoundReturnsNullopt)`
- Source: `tests/transaction/test_transaction_manager.cpp`:670
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (ExplainNotFoundReturnsNullopt): n/a

#### `TEST_F(TransactionManagerTest, GraphEdgeRollback)`
- Source: `tests/transaction/test_transaction_manager.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (GraphEdgeRollback): n/a

#### `TEST_F(TransactionManagerTest, GraphEdgeTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (GraphEdgeTransaction): n/a

#### `TEST_F(TransactionManagerTest, IsolationLevelReadCommitted)`
- Source: `tests/transaction/test_transaction_manager.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (IsolationLevelReadCommitted): n/a

#### `TEST_F(TransactionManagerTest, IsolationLevelSnapshot)`
- Source: `tests/transaction/test_transaction_manager.cpp`:305
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (IsolationLevelSnapshot): n/a

#### `TEST_F(TransactionManagerTest, MaxDurationTracking)`
- Source: `tests/transaction/test_transaction_manager.cpp`:394
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (MaxDurationTracking): n/a

#### `TEST_F(TransactionManagerTest, ReadEntityJsonReturnsUncommittedWriteInSameTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (ReadEntityJsonReturnsUncommittedWriteInSameTransaction): n/a

#### `TEST_F(TransactionManagerTest, ReadEntityJsonSeesDeleteInSameTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (ReadEntityJsonSeesDeleteInSameTransaction): n/a

#### `TEST_F(TransactionManagerTest, RollbackAfterCommit)`
- Source: `tests/transaction/test_transaction_manager.cpp`:430
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (RollbackAfterCommit): n/a

#### `TEST_F(TransactionManagerTest, RollbackTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (RollbackTransaction): n/a

#### `TEST_F(TransactionManagerTest, StatisticsTracking)`
- Source: `tests/transaction/test_transaction_manager.cpp`:372
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (StatisticsTracking): n/a

#### `TEST_F(TransactionManagerTest, TransactionDurationTracking)`
- Source: `tests/transaction/test_transaction_manager.cpp`:355
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (TransactionDurationTracking): n/a

#### `TEST_F(TransactionManagerTest, VectorAddTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:483
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (VectorAddTransaction): n/a

#### `TEST_F(TransactionManagerTest, VectorRemoveTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:576
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (VectorRemoveTransaction): n/a

#### `TEST_F(TransactionManagerTest, VectorRollbackTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:513
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (VectorRollbackTransaction): n/a

#### `TEST_F(TransactionManagerTest, VectorUpdateTransaction)`
- Source: `tests/transaction/test_transaction_manager.cpp`:540
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerTest): n/a
  - `<unnamed>` (VectorUpdateTransaction): n/a

### test_transaction_manager_comprehensive.cpp

#### `TEST(TransactionManagerComprehensive, ExplainEntryFields)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (ExplainEntryFields): n/a

#### `TEST(TransactionManagerComprehensive, ExplainLocksEmptyAfterRelease)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (ExplainLocksEmptyAfterRelease): n/a

#### `TEST(TransactionManagerComprehensive, ExplainLocksHeldViaLockManager)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (ExplainLocksHeldViaLockManager): n/a

#### `TEST(TransactionManagerComprehensive, ExplainResultDefaultValues)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (ExplainResultDefaultValues): n/a

#### `TEST(TransactionManagerComprehensive, IsolationLevelEnumValues)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:12
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (IsolationLevelEnumValues): n/a

#### `TEST(TransactionManagerComprehensive, LegacyIsolationLevelAliases)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:19
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (LegacyIsolationLevelAliases): n/a

#### `TEST(TransactionManagerComprehensive, LockManagerAccessible)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:32
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (LockManagerAccessible): n/a

#### `TEST(TransactionManagerComprehensive, LockManagerStatistics)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (LockManagerStatistics): n/a

#### `TEST(TransactionManagerComprehensive, LockManagerTwoPL)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (LockManagerTwoPL): n/a

#### `TEST(TransactionManagerComprehensive, NonSerializableIsolationNoPredicateTracking)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (NonSerializableIsolationNoPredicateTracking): n/a

#### `TEST(TransactionManagerComprehensive, PredicateLockAcquireAndCount)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (PredicateLockAcquireAndCount): n/a

#### `TEST(TransactionManagerComprehensive, PredicateLockConflictDetected)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (PredicateLockConflictDetected): n/a

#### `TEST(TransactionManagerComprehensive, PredicateLockMultipleTransactions)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (PredicateLockMultipleTransactions): n/a

#### `TEST(TransactionManagerComprehensive, PredicateLockNoConflictOutsideRange)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (PredicateLockNoConflictOutsideRange): n/a

#### `TEST(TransactionManagerComprehensive, PredicateLockNoConflictSameTransaction)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (PredicateLockNoConflictSameTransaction): n/a

#### `TEST(TransactionManagerComprehensive, PredicateLockReleasedOnCommit)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (PredicateLockReleasedOnCommit): n/a

#### `TEST(TransactionManagerComprehensive, SerializableIsolationLevelValue)`
- Source: `tests/transaction/test_transaction_manager_comprehensive.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionManagerComprehensive): n/a
  - `<unnamed>` (SerializableIsolationLevelValue): n/a

### test_transaction_occ.cpp

#### `TEST_F(OccTest, GetVersionAfterOptimisticPutIsOne)`
- Source: `tests/transaction/test_transaction_occ.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (GetVersionAfterOptimisticPutIsOne): n/a

#### `TEST_F(OccTest, GetVersionNonExistentIsZero)`
- Source: `tests/transaction/test_transaction_occ.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (GetVersionNonExistentIsZero): n/a

#### `TEST_F(OccTest, GetVersionReturnsNulloptWhenFinished)`
- Source: `tests/transaction/test_transaction_occ.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (GetVersionReturnsNulloptWhenFinished): n/a

#### `TEST_F(OccTest, OccWorksWithAllIsolationLevels)`
- Source: `tests/transaction/test_transaction_occ.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (OccWorksWithAllIsolationLevels): n/a

#### `TEST_F(OccTest, OptimisticEraseFailsOnVersionConflict)`
- Source: `tests/transaction/test_transaction_occ.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (OptimisticEraseFailsOnVersionConflict): n/a

#### `TEST_F(OccTest, OptimisticEraseFailsWhenEntityNotFound)`
- Source: `tests/transaction/test_transaction_occ.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (OptimisticEraseFailsWhenEntityNotFound): n/a

#### `TEST_F(OccTest, OptimisticEraseSuccess)`
- Source: `tests/transaction/test_transaction_occ.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (OptimisticEraseSuccess): n/a

#### `TEST_F(OccTest, OptimisticPutCreateFailsIfAlreadyExists)`
- Source: `tests/transaction/test_transaction_occ.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (OptimisticPutCreateFailsIfAlreadyExists): n/a

#### `TEST_F(OccTest, OptimisticPutCreateNewEntity)`
- Source: `tests/transaction/test_transaction_occ.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (OptimisticPutCreateNewEntity): n/a

#### `TEST_F(OccTest, OptimisticPutUpdateFailsOnVersionConflict)`
- Source: `tests/transaction/test_transaction_occ.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (OptimisticPutUpdateFailsOnVersionConflict): n/a

#### `TEST_F(OccTest, OptimisticPutUpdateWithCorrectVersion)`
- Source: `tests/transaction/test_transaction_occ.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (OptimisticPutUpdateWithCorrectVersion): n/a

#### `TEST_F(OccTest, OptimisticPutVersionConflictShowsExpectedAndActual)`
- Source: `tests/transaction/test_transaction_occ.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (OptimisticPutVersionConflictShowsExpectedAndActual): n/a

#### `TEST_F(OccTest, ReadVersionThenOptimisticPutPattern)`
- Source: `tests/transaction/test_transaction_occ.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (OccTest): n/a
  - `<unnamed>` (ReadVersionThenOptimisticPutPattern): n/a

### test_transaction_retry.cpp

#### `TEST(TransactionRetryManager, AlertCallbackCanQueryCircuitState)`
- Source: `tests/transaction/test_transaction_retry.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (AlertCallbackCanQueryCircuitState): n/a

#### `TEST(TransactionRetryManager, AlertCallbackCanUnregisterItself)`
- Source: `tests/transaction/test_transaction_retry.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (AlertCallbackCanUnregisterItself): n/a

#### `TEST(TransactionRetryManager, AlertCallbackFiredOnCircuitChange)`
- Source: `tests/transaction/test_transaction_retry.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (AlertCallbackFiredOnCircuitChange): n/a

#### `TEST(TransactionRetryManager, CircuitBreakerOpensAfterThreshold)`
- Source: `tests/transaction/test_transaction_retry.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (CircuitBreakerOpensAfterThreshold): n/a

#### `TEST(TransactionRetryManager, ClassifyNonRetryableErrors)`
- Source: `tests/transaction/test_transaction_retry.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (ClassifyNonRetryableErrors): n/a

#### `TEST(TransactionRetryManager, ClassifyRetryableErrors)`
- Source: `tests/transaction/test_transaction_retry.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (ClassifyRetryableErrors): n/a

#### `TEST(TransactionRetryManager, IsRetryable)`
- Source: `tests/transaction/test_transaction_retry.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (IsRetryable): n/a

#### `TEST(TransactionRetryManager, NoRetryOnPermissionDenied)`
- Source: `tests/transaction/test_transaction_retry.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (NoRetryOnPermissionDenied): n/a

#### `TEST(TransactionRetryManager, PerOperationPolicyOverridesMaxAttempts)`
- Source: `tests/transaction/test_transaction_retry.cpp`:263
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (PerOperationPolicyOverridesMaxAttempts): n/a

#### `TEST(TransactionRetryManager, RetriesOnWriteConflict)`
- Source: `tests/transaction/test_transaction_retry.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (RetriesOnWriteConflict): n/a

#### `TEST(TransactionRetryManager, RetryAttemptsNotOvercountedWhenExhausted)`
- Source: `tests/transaction/test_transaction_retry.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (RetryAttemptsNotOvercountedWhenExhausted): n/a

#### `TEST(TransactionRetryManager, StatisticsTracking)`
- Source: `tests/transaction/test_transaction_retry.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (StatisticsTracking): n/a

#### `TEST(TransactionRetryManager, SuccessOnFirstAttempt)`
- Source: `tests/transaction/test_transaction_retry.cpp`:29
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (SuccessOnFirstAttempt): n/a

#### `TEST(TransactionRetryManager, ThrowsAfterMaxAttempts)`
- Source: `tests/transaction/test_transaction_retry.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionRetryManager): n/a
  - `<unnamed>` (ThrowsAfterMaxAttempts): n/a

#### `TransactionRetryConfig fastConfig(size_t max_attempts=3)`
- Source: `tests/transaction/test_transaction_retry.cpp`:16
- Brief: n/a
- Parameters:
  - `max_attempts` (size_t): n/a
- Details: Return a config with negligible delays so tests run fast.

### test_transaction_semantic_advisor.cpp

#### `TEST(TransactionSemanticAdvisorTest, AnalyzeBatchNonBlocking)`
- Source: `tests/transaction/test_transaction_semantic_advisor.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorTest): n/a
  - `<unnamed>` (AnalyzeBatchNonBlocking): n/a

#### `TEST(TransactionSemanticAdvisorTest, CompetingWritesHighConflict)`
- Source: `tests/transaction/test_transaction_semantic_advisor.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorTest): n/a
  - `<unnamed>` (CompetingWritesHighConflict): n/a

#### `TEST(TransactionSemanticAdvisorTest, DisjointConflictProbabilityLow)`
- Source: `tests/transaction/test_transaction_semantic_advisor.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorTest): n/a
  - `<unnamed>` (DisjointConflictProbabilityLow): n/a

#### `TEST(TransactionSemanticAdvisorTest, DisjointEntitiesNoHints)`
- Source: `tests/transaction/test_transaction_semantic_advisor.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorTest): n/a
  - `<unnamed>` (DisjointEntitiesNoHints): n/a

#### `TEST(TransactionSemanticAdvisorTest, MixedBatch)`
- Source: `tests/transaction/test_transaction_semantic_advisor.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorTest): n/a
  - `<unnamed>` (MixedBatch): n/a

#### `TEST(TransactionSemanticAdvisorTest, SameEntityGrouped)`
- Source: `tests/transaction/test_transaction_semantic_advisor.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorTest): n/a
  - `<unnamed>` (SameEntityGrouped): n/a

#### `TEST(TransactionSemanticAdvisorTest, SuggestDeferralNoConflict)`
- Source: `tests/transaction/test_transaction_semantic_advisor.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorTest): n/a
  - `<unnamed>` (SuggestDeferralNoConflict): n/a

#### `TEST(TransactionSemanticAdvisorTest, SuggestDeferralWithConflict)`
- Source: `tests/transaction/test_transaction_semantic_advisor.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorTest): n/a
  - `<unnamed>` (SuggestDeferralWithConflict): n/a

#### `TransactionContext makeTx(const std::string &id, std::map< std::string, std::string > entities, Op op=Op::WRITE)`
- Source: `tests/transaction/test_transaction_semantic_advisor.cpp`:27
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `entities` (std::map< std::string, std::string >): n/a
  - `op` (Op): n/a

### test_transaction_semantic_advisor_llm_focused.cpp

#### `TEST(TransactionSemanticAdvisorLlmFocused, TS1_DefaultConstruct_NoThrow)`
- Source: `tests/transaction/test_transaction_semantic_advisor_llm_focused.cpp`:16
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorLlmFocused): n/a
  - `<unnamed>` (TS1_DefaultConstruct_NoThrow): n/a

#### `TEST(TransactionSemanticAdvisorLlmFocused, TS2_EmptyBatch_ReturnsNoHints)`
- Source: `tests/transaction/test_transaction_semantic_advisor_llm_focused.cpp`:21
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorLlmFocused): n/a
  - `<unnamed>` (TS2_EmptyBatch_ReturnsNoHints): n/a

#### `TEST(TransactionSemanticAdvisorLlmFocused, TS3_SingleTx_NoAffinityHints)`
- Source: `tests/transaction/test_transaction_semantic_advisor_llm_focused.cpp`:28
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorLlmFocused): n/a
  - `<unnamed>` (TS3_SingleTx_NoAffinityHints): n/a

#### `TEST(TransactionSemanticAdvisorLlmFocused, TS4_DisjointTxs_NoHints)`
- Source: `tests/transaction/test_transaction_semantic_advisor_llm_focused.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorLlmFocused): n/a
  - `<unnamed>` (TS4_DisjointTxs_NoHints): n/a

#### `TEST(TransactionSemanticAdvisorLlmFocused, TS5_SharedEntityWrites_HintProduced)`
- Source: `tests/transaction/test_transaction_semantic_advisor_llm_focused.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorLlmFocused): n/a
  - `<unnamed>` (TS5_SharedEntityWrites_HintProduced): n/a

#### `TEST(TransactionSemanticAdvisorLlmFocused, TS6_CompetingWrites_HighConflictProbability)`
- Source: `tests/transaction/test_transaction_semantic_advisor_llm_focused.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorLlmFocused): n/a
  - `<unnamed>` (TS6_CompetingWrites_HighConflictProbability): n/a

#### `TEST(TransactionSemanticAdvisorLlmFocused, TS7_NoConcurrentConflict_ZeroDeferral)`
- Source: `tests/transaction/test_transaction_semantic_advisor_llm_focused.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorLlmFocused): n/a
  - `<unnamed>` (TS7_NoConcurrentConflict_ZeroDeferral): n/a

#### `TEST(TransactionSemanticAdvisorLlmFocused, TS8_SetNullProcessor_NoThrow)`
- Source: `tests/transaction/test_transaction_semantic_advisor_llm_focused.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisorLlmFocused): n/a
  - `<unnamed>` (TS8_SetNullProcessor_NoThrow): n/a

### test_transaction_ssi.cpp

#### `TEST(LockManagerPredicateTest, AcquireAndCheck)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:539
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockManagerPredicateTest): n/a
  - `<unnamed>` (AcquireAndCheck): n/a

#### `TEST(LockManagerPredicateTest, GetMaxPredicateLocks)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:592
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockManagerPredicateTest): n/a
  - `<unnamed>` (GetMaxPredicateLocks): n/a

#### `TEST(LockManagerPredicateTest, GetPredicateLockRanges_AfterRelease)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:723
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockManagerPredicateTest): n/a
  - `<unnamed>` (GetPredicateLockRanges_AfterRelease): n/a

#### `TEST(LockManagerPredicateTest, GetPredicateLockRanges_Empty)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:695
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockManagerPredicateTest): n/a
  - `<unnamed>` (GetPredicateLockRanges_Empty): n/a

#### `TEST(LockManagerPredicateTest, GetPredicateLockRanges_MultipleOwners)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:710
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockManagerPredicateTest): n/a
  - `<unnamed>` (GetPredicateLockRanges_MultipleOwners): n/a

#### `TEST(LockManagerPredicateTest, GetPredicateLockRanges_SingleLock)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:701
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockManagerPredicateTest): n/a
  - `<unnamed>` (GetPredicateLockRanges_SingleLock): n/a

#### `TEST(LockManagerPredicateTest, MaxPredicateLocksEnforced)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:555
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockManagerPredicateTest): n/a
  - `<unnamed>` (MaxPredicateLocksEnforced): n/a

#### `TEST(LockManagerPredicateTest, MaxPredicateLocksZeroMeansUnlimited)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:565
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockManagerPredicateTest): n/a
  - `<unnamed>` (MaxPredicateLocksZeroMeansUnlimited): n/a

#### `TEST(LockManagerPredicateTest, ReleaseClears)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:547
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockManagerPredicateTest): n/a
  - `<unnamed>` (ReleaseClears): n/a

#### `TEST(LockManagerPredicateTest, SetPredicateLockingDisabled)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:575
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockManagerPredicateTest): n/a
  - `<unnamed>` (SetPredicateLockingDisabled): n/a

#### `TEST(LockManagerPredicateTest, SetPredicateLockingEnabled_Default)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:583
- Brief: n/a
- Parameters:
  - `<unnamed>` (LockManagerPredicateTest): n/a
  - `<unnamed>` (SetPredicateLockingEnabled_Default): n/a

#### `TEST_F(SSITest, BeginTransactionWithSerializableSnapshotAlias)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (BeginTransactionWithSerializableSnapshotAlias): n/a

#### `TEST_F(SSITest, DetectConflicts_EmptyForNonSerializable)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:437
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (DetectConflicts_EmptyForNonSerializable): n/a

#### `TEST_F(SSITest, DetectConflicts_EmptyForUnknownTxn)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:444
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (DetectConflicts_EmptyForUnknownTxn): n/a

#### `TEST_F(SSITest, DetectConflicts_EmptyWhenRangesDoNotOverlap)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:650
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (DetectConflicts_EmptyWhenRangesDoNotOverlap): n/a

#### `TEST_F(SSITest, DetectConflicts_ReturnsConflict_WhenRangesOverlap)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:617
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (DetectConflicts_ReturnsConflict_WhenRangesOverlap): n/a

#### `TEST_F(SSITest, DetectConflicts_ReturnsEmptyWhenNoConflict)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:423
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (DetectConflicts_ReturnsEmptyWhenNoConflict): n/a

#### `TEST_F(SSITest, DetectConflicts_SymmetricRanges)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:670
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (DetectConflicts_SymmetricRanges): n/a

#### `TEST_F(SSITest, DisablePredicateLocking_NoConflictDetected)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (DisablePredicateLocking_NoConflictDetected): n/a

#### `TEST_F(SSITest, MaxPredicateLocks_Enforced)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:368
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (MaxPredicateLocks_Enforced): n/a

#### `TEST_F(SSITest, PredicateLocksReleasedOnCommit)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:451
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (PredicateLocksReleasedOnCommit): n/a

#### `TEST_F(SSITest, PredicateLocksReleasedOnRollback)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:467
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (PredicateLocksReleasedOnRollback): n/a

#### `TEST_F(SSITest, ReadWriteConflict_DetectedOnWrite)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (ReadWriteConflict_DetectedOnWrite): n/a

#### `TEST_F(SSITest, ReadWriteConflict_NotDetected_WhenKeyOutsideRange)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (ReadWriteConflict_NotDetected_WhenKeyOutsideRange): n/a

#### `TEST_F(SSITest, RetryManager_ExponentialBackoff_StatsTracked)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:337
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (RetryManager_ExponentialBackoff_StatsTracked): n/a

#### `TEST_F(SSITest, RetryManager_RetriesOnSerializationError)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (RetryManager_RetriesOnSerializationError): n/a

#### `TEST_F(SSITest, SSIConfigDefaults)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (SSIConfigDefaults): n/a

#### `TEST_F(SSITest, SSI_ErrorIsClassifiedAsRetryable)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (SSI_ErrorIsClassifiedAsRetryable): n/a

#### `TEST_F(SSITest, SerializableSnapshotAliasEqualsSerializable)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (SerializableSnapshotAliasEqualsSerializable): n/a

#### `TEST_F(SSITest, SerializationConflict_FieldsArePopulated)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:601
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (SerializationConflict_FieldsArePopulated): n/a

#### `TEST_F(SSITest, SerializationFailureIsReturnedAsErrorStatus)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (SerializationFailureIsReturnedAsErrorStatus): n/a

#### `TEST_F(SSITest, SetSSIConfigUpdatesValues)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (SetSSIConfigUpdatesValues): n/a

#### `TEST_F(SSITest, TrackPredicateRead_FinishedTransaction_ReturnsError)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (TrackPredicateRead_FinishedTransaction_ReturnsError): n/a

#### `TEST_F(SSITest, TrackPredicateRead_MultipleRanges)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (TrackPredicateRead_MultipleRanges): n/a

#### `TEST_F(SSITest, TrackPredicateRead_NonSerializable_IsNoOp)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (TrackPredicateRead_NonSerializable_IsNoOp): n/a

#### `TEST_F(SSITest, TrackPredicateRead_Serializable_Succeeds)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (TrackPredicateRead_Serializable_Succeeds): n/a

#### `TEST_F(SSITest, WriteSkew_SerializableIsolation_DetectsConflict)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:482
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (WriteSkew_SerializableIsolation_DetectsConflict): n/a

#### `TEST_F(SSITest, WriteWriteConflict_DetectedAtCommitTime)`
- Source: `tests/transaction/test_transaction_ssi.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (SSITest): n/a
  - `<unnamed>` (WriteWriteConflict_DetectedAtCommitTime): n/a

### test_transaction_timeout.cpp

#### `TEST_F(TransactionTimeoutTest, Commit_AfterTimeout_ReturnsError)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (Commit_AfterTimeout_ReturnsError): n/a

#### `TEST_F(TransactionTimeoutTest, DefaultTimeout_AppliedToNewTransactions)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (DefaultTimeout_AppliedToNewTransactions): n/a

#### `TEST_F(TransactionTimeoutTest, DefaultTimeout_AppliedToSessionTransactions)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (DefaultTimeout_AppliedToSessionTransactions): n/a

#### `TEST_F(TransactionTimeoutTest, DefaultTimeout_InitiallyZero)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (DefaultTimeout_InitiallyZero): n/a

#### `TEST_F(TransactionTimeoutTest, ExpiredTransaction_AutoRolledBack)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (ExpiredTransaction_AutoRolledBack): n/a

#### `TEST_F(TransactionTimeoutTest, GetDurationMs_FrozenAfterCommit)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (GetDurationMs_FrozenAfterCommit): n/a

#### `TEST_F(TransactionTimeoutTest, GetDurationMs_FrozenAfterRollback)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:311
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (GetDurationMs_FrozenAfterRollback): n/a

#### `TEST_F(TransactionTimeoutTest, GetStatsLockFree_InitialTimedOutIsZero)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (GetStatsLockFree_InitialTimedOutIsZero): n/a

#### `TEST_F(TransactionTimeoutTest, GetStats_AvgDuration_DoesNotGrowAfterCommit)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:326
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (GetStats_AvgDuration_DoesNotGrowAfterCommit): n/a

#### `TEST_F(TransactionTimeoutTest, GetStats_InitialTimedOutIsZero)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (GetStats_InitialTimedOutIsZero): n/a

#### `TEST_F(TransactionTimeoutTest, GetStats_TimedOutAppearsAfterAutoRollback)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (GetStats_TimedOutAppearsAfterAutoRollback): n/a

#### `TEST_F(TransactionTimeoutTest, GetTimeoutCount_IncrementsOnAutoRollback)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (GetTimeoutCount_IncrementsOnAutoRollback): n/a

#### `TEST_F(TransactionTimeoutTest, GetTimeoutCount_InitiallyZero)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (GetTimeoutCount_InitiallyZero): n/a

#### `TEST_F(TransactionTimeoutTest, LongTimeout_CommitSucceeds)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (LongTimeout_CommitSucceeds): n/a

#### `TEST_F(TransactionTimeoutTest, LongTimeout_IsNotTimedOut)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (LongTimeout_IsNotTimedOut): n/a

#### `TEST_F(TransactionTimeoutTest, NegativeDefaultTimeout_TreatedAsDisabled)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (NegativeDefaultTimeout_TreatedAsDisabled): n/a

#### `TEST_F(TransactionTimeoutTest, NegativeTimeout_TreatedAsDisabled)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (NegativeTimeout_TreatedAsDisabled): n/a

#### `TEST_F(TransactionTimeoutTest, NoTimeout_CommitSucceeds)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (NoTimeout_CommitSucceeds): n/a

#### `TEST_F(TransactionTimeoutTest, NoTimeout_IsNotTimedOut)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (NoTimeout_IsNotTimedOut): n/a

#### `TEST_F(TransactionTimeoutTest, SetDefaultTimeout_GetDefaultTimeout)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (SetDefaultTimeout_GetDefaultTimeout): n/a

#### `TEST_F(TransactionTimeoutTest, SetDefaultTimeout_ZeroDisablesTimeout)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (SetDefaultTimeout_ZeroDisablesTimeout): n/a

#### `TEST_F(TransactionTimeoutTest, SetTimeout_GetTimeout)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (SetTimeout_GetTimeout): n/a

#### `TEST_F(TransactionTimeoutTest, VeryShortTimeout_IsTimedOut)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (VeryShortTimeout_IsTimedOut): n/a

#### `TEST_F(TransactionTimeoutTest, ZeroTimeout_AfterSet_IsNotTimedOut)`
- Source: `tests/transaction/test_transaction_timeout.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionTimeoutTest): n/a
  - `<unnamed>` (ZeroTimeout_AfterSet_IsNotTimedOut): n/a

### test_wave4c_t1t4_hardening.cpp

#### `TEST(Wave4CT1StubNote, Phase2BridgeDocumentsActivation)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT1StubNote): n/a
  - `<unnamed>` (Phase2BridgeDocumentsActivation): n/a

#### `TEST(Wave4CT1StubNote, Phase2BridgeDocumentsProductionDelta)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT1StubNote): n/a
  - `<unnamed>` (Phase2BridgeDocumentsProductionDelta): n/a

#### `TEST(Wave4CT1StubNote, Phase2BridgeDocumentsRemovalPlan)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT1StubNote): n/a
  - `<unnamed>` (Phase2BridgeDocumentsRemovalPlan): n/a

#### `TEST(Wave4CT1StubNote, Phase2BridgeHasStubNoteComment)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT1StubNote): n/a
  - `<unnamed>` (Phase2BridgeHasStubNoteComment): n/a

#### `TEST(Wave4CT2UpgradeDeadlock, MutualUpgradeReturnsDeniedNotTimeout)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT2UpgradeDeadlock): n/a
  - `<unnamed>` (MutualUpgradeReturnsDeniedNotTimeout): n/a

#### `TEST(Wave4CT2UpgradeDeadlock, SingleUpgradeSucceeds)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT2UpgradeDeadlock): n/a
  - `<unnamed>` (SingleUpgradeSucceeds): n/a

#### `TEST(Wave4CT2UpgradeDeadlock, ThreeConcurrentUpgraders_NoHang)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT2UpgradeDeadlock): n/a
  - `<unnamed>` (ThreeConcurrentUpgraders_NoHang): n/a

#### `TEST(Wave4CT3GTMPhase2, Phase2CalledOutsideLock)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT3GTMPhase2): n/a
  - `<unnamed>` (Phase2CalledOutsideLock): n/a

#### `TEST(Wave4CT3GTMPhase2, SnapshotPatternPresent)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT3GTMPhase2): n/a
  - `<unnamed>` (SnapshotPatternPresent): n/a

#### `TEST(Wave4CT4PredicateLockDrop, CounterReflectsOnlyDrops)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT4PredicateLockDrop): n/a
  - `<unnamed>` (CounterReflectsOnlyDrops): n/a

#### `TEST(Wave4CT4PredicateLockDrop, DropIncrementsCounter)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT4PredicateLockDrop): n/a
  - `<unnamed>` (DropIncrementsCounter): n/a

#### `TEST(Wave4CT4PredicateLockDrop, MultipleDropsAccumulate)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT4PredicateLockDrop): n/a
  - `<unnamed>` (MultipleDropsAccumulate): n/a

#### `TEST(Wave4CT4PredicateLockDrop, SuccessUnderCapacity)`
- Source: `tests/transaction/test_wave4c_t1t4_hardening.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave4CT4PredicateLockDrop): n/a
  - `<unnamed>` (SuccessUnderCapacity): n/a

### themis

#### `uint64_t decodeVersion(const std::vector< uint8_t > &buf)`
- Source: `src/transaction/transaction_manager.cpp`:1631
- Brief: Decode Version.
- Parameters:
  - `buf` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: buf Input parameter. Return value. Calls: size().

#### `std::vector< uint8_t > encodeVersion(uint64_t v)`
- Source: `src/transaction/transaction_manager.cpp`:1617
- Brief: n/a
- Parameters:
  - `v` (uint64_t): n/a

#### `std::string isolationLevelName(IsolationLevel level)`
- Source: `src/transaction/transaction_manager.cpp`:2381
- Brief: Isolation Level Name.
- Parameters:
  - `level` (IsolationLevel): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Implements isolationLevelName without additional internal calls.

#### `std::string lockTypeName(LockType t)`
- Source: `src/transaction/transaction_manager.cpp`:2397
- Brief: Lock Type Name.
- Parameters:
  - `t` (LockType): Input parameter.
- Return: Return value.
- Details: t Input parameter. Return value. Implements lockTypeName without additional internal calls.

#### `std::string versionKey(std::string_view table, std::string_view pk)`
- Source: `src/transaction/transaction_manager.cpp`:1649
- Brief: Version Key.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: table Input parameter. pk Input parameter. Return value. Calls: reserve(), size().

### themis::DeadlockPredictor

#### `DeadlockPredictor()=default`
- Source: `include/transaction/deadlock_predictor.h`:56
- Brief: n/a
- Parameters: none

#### `DeadlockPredictor(Config config)`
- Source: `include/transaction/deadlock_predictor.h`:62
- Brief: Deadlock Predictor.
- Parameters:
  - `config` (Config): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `double computeConflictScore(const std::vector< std::string > &keys) const`
- Source: `include/transaction/deadlock_predictor.h`:158
- Brief: Compute Conflict Score.
- Parameters:
  - `keys` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: keys Input parameter. Return value.

#### `Config getConfig() const`
- Source: `include/transaction/deadlock_predictor.h`:73
- Brief: Get Config.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< LockPattern > getPatterns() const`
- Source: `include/transaction/deadlock_predictor.h`:136
- Brief: Get Patterns.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string makePairKey(const std::string &a, const std::string &b)`
- Source: `include/transaction/deadlock_predictor.h`:151
- Brief: Make Pair Key.
- Parameters:
  - `a` (const std::string &): Input parameter.
  - `b` (const std::string &): Input parameter.
- Return: Return value.
- Details: static a Input parameter. b Input parameter. Return value. a Input parameter. b Input parameter. Return value. Implements makePairKey without additional internal calls.

#### `std::chrono::microseconds percentile(std::vector< std::chrono::microseconds > values, int p)`
- Source: `include/transaction/deadlock_predictor.h`:166
- Brief: Percentile.
- Parameters:
  - `values` (std::vector< std::chrono::microseconds >): Input parameter.
  - `p` (int): Input parameter.
- Return: Return value.
- Details: static values Input parameter. p Input parameter. Return value.

#### `double predictDeadlockProbability(const std::vector< std::string > &proposed_locks, const std::set< TransactionId > &active_transactions) const`
- Source: `include/transaction/deadlock_predictor.h`:99
- Brief: Predict Deadlock Probability.
- Parameters:
  - `proposed_locks` (const std::vector< std::string > &): Input parameter.
  - `active_transactions` (const std::set< TransactionId > &): Input parameter.
- Return: Return value.
- Details: proposed_locks Input parameter. active_transactions Input parameter. Return value.

#### `std::vector< std::string > recommendLockOrder(const std::vector< std::string > &keys) const`
- Source: `include/transaction/deadlock_predictor.h`:108
- Brief: Recommend Lock Order.
- Parameters:
  - `keys` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: keys Input parameter. Return value.

#### `std::chrono::milliseconds recommendTimeout(const std::vector< std::string > &keys) const`
- Source: `include/transaction/deadlock_predictor.h`:116
- Brief: Recommend Timeout.
- Parameters:
  - `keys` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: keys Input parameter. Return value.

#### `void recordDeadlock(const std::vector< std::string > &keys)`
- Source: `include/transaction/deadlock_predictor.h`:90
- Brief: Record Deadlock.
- Parameters:
  - `keys` (const std::vector< std::string > &): Input parameter.
- Details: keys Input parameter. keys Input parameter. Calls: empty(), lk(), size(), makePairKey(), std::min_element(), begin(), end(), erase().

#### `void recordTransaction(TransactionId txn_id, const std::vector< std::string > &locks_acquired, std::chrono::microseconds duration)`
- Source: `include/transaction/deadlock_predictor.h`:82
- Brief: Record Transaction.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
  - `locks_acquired` (const std::vector< std::string > &): Input parameter.
  - `duration` (std::chrono::microseconds): Input parameter.
- Details: ── Training API ────────────────────────────────────────────────────────────── txn_id Identifier of the txn. locks_acquired Input parameter. duration Input parameter. TransactionId Input parameter. locks_acquired Input parameter. duration Input parameter.

#### `size_t recordedDeadlockCount() const`
- Source: `include/transaction/deadlock_predictor.h`:130
- Brief: Recorded Deadlock Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t recordedTransactionCount() const`
- Source: `include/transaction/deadlock_predictor.h`:124
- Brief: Recorded Transaction Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void reset()`
- Source: `include/transaction/deadlock_predictor.h`:141
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Calls: lk(), clear().

#### `void setConfig(Config config)`
- Source: `include/transaction/deadlock_predictor.h`:68
- Brief: Set Config.
- Parameters:
  - `config` (Config): Input parameter.
- Details: config Input parameter. config Input parameter. Calls: lk(), std::move().

### themis::DistributedSagaCoordinator

#### `DistributedSagaCoordinator(Config config={})`
- Source: `include/transaction/distributed_saga.h`:247
- Brief: n/a
- Parameters:
  - `config` (Config): n/a

#### `DistributedSagaCoordinator(DistributedSagaCoordinator &&) noexcept=default`
- Source: `include/transaction/distributed_saga.h`:253
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSagaCoordinator &&): n/a

#### `DistributedSagaCoordinator(const DistributedSagaCoordinator &)=delete`
- Source: `include/transaction/distributed_saga.h`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedSagaCoordinator &): n/a

#### `void compensate(const std::map< std::string, DistributedSagaStep > &step_map, const std::vector< std::string > &executed_order, RecordIndex &index)`
- Source: `include/transaction/distributed_saga.h`:395
- Brief: n/a
- Parameters:
  - `step_map` (const std::map< std::string, DistributedSagaStep > &): n/a
  - `executed_order` (const std::vector< std::string > &): n/a
  - `index` (RecordIndex &): n/a

#### `DistributedSagaStatus compensateStep(const DistributedSagaStep &step, StepRecord &record)`
- Source: `include/transaction/distributed_saga.h`:407
- Brief: Compensate Step.
- Parameters:
  - `step` (const DistributedSagaStep &): Input parameter.
  - `record` (StepRecord &): Input/output parameter.
- Return: Return value.
- Details: step Input parameter. record Input/output parameter. Return value. step Input parameter. record Input/output parameter. Return value. Calls: count(), std::min(), std::this_thread::sleep_for(), std::async(), wait_for(), DistributedSagaStatus::Error(), lk(), get().

#### `DistributedSagaReport execute(const DistributedSagaDefinition &saga)`
- Source: `include/transaction/distributed_saga.h`:262
- Brief: Execute.
- Parameters:
  - `saga` (const DistributedSagaDefinition &): Input parameter.
- Return: Return value.
- Throws:
  - std::logic_error: if an error occurs.
- Details: saga Input parameter. Return value. saga Input parameter. Return value. std::logic_error if an error occurs. Calls: lk(), find(), end(), THEMIS_ERROR(), journalWrite(), mk(), std::chrono::system_clock::now(), count().

#### `DistributedSagaReport executeDistributed(const DistributedSAGADefinition &saga)`
- Source: `include/transaction/distributed_saga.h`:277
- Brief: Execute Distributed.
- Parameters:
  - `saga` (const DistributedSAGADefinition &): Input parameter.
- Return: Return value.
- Details: saga Input parameter. Return value. remote_saga Input parameter. Return value. Calls: empty(), journalWrite(), lk(), THEMIS_ERROR(), rejectDistributed(), size(), std::to_string(), push_back().

#### `DistributedSagaStatus executeStep(const DistributedSagaStep &step, StepRecord &record, std::optional< std::chrono::steady_clock::time_point > deadline)`
- Source: `include/transaction/distributed_saga.h`:382
- Brief: Execute Step.
- Parameters:
  - `step` (const DistributedSagaStep &): Input parameter.
  - `record` (StepRecord &): Input/output parameter.
  - `deadline` (std::optional< std::chrono::steady_clock::time_point >): Input parameter.
- Return: Return value.
- Details: step Input parameter. record Input/output parameter. deadline Input parameter. Return value. step Input parameter. record Input/output parameter. deadline Input parameter. Return value. Calls: std::chrono::system_clock::now(), count(), std::min(), std::this_thread::sleep_for(), THEMIS_DEBUG(), has_value(), std::chrono::steady_clock::now(), DistributedSagaStatus::Error().

#### `DistributedSagaStatus executeWave(const std::vector< std::string > &wave, const std::map< std::string, DistributedSagaStep > &step_map, RecordIndex &index, std::string &failure_reason, std::optional< std::chrono::steady_clock::time_point > deadline)`
- Source: `include/transaction/distributed_saga.h`:367
- Brief: n/a
- Parameters:
  - `wave` (const std::vector< std::string > &): n/a
  - `step_map` (const std::map< std::string, DistributedSagaStep > &): n/a
  - `index` (RecordIndex &): n/a
  - `failure_reason` (std::string &): n/a
  - `deadline` (std::optional< std::chrono::steady_clock::time_point >): n/a

#### `bool forceCompensate(const std::string &saga_id)`
- Source: `include/transaction/distributed_saga.h`:308
- Brief: Force Compensate.
- Parameters:
  - `saga_id` (const std::string &): Identifier of the saga.
- Return: True when the operation succeeds.
- Details: saga_id Identifier of the saga. True when the operation succeeds. saga_id Identifier of the saga. True when the operation succeeds. Calls: lk(), find(), end(), journalWrite(), THEMIS_WARN().

#### `bool forceComplete(const std::string &saga_id)`
- Source: `include/transaction/distributed_saga.h`:315
- Brief: Force Complete.
- Parameters:
  - `saga_id` (const std::string &): Identifier of the saga.
- Return: True when the operation succeeds.
- Details: saga_id Identifier of the saga. True when the operation succeeds. saga_id Identifier of the saga. True when the operation succeeds. Calls: lk(), find(), end(), clear(), journalWrite(), THEMIS_WARN().

#### `std::optional< DistributedSagaReport > getDistributedStatus(const std::string &saga_id) const`
- Source: `include/transaction/distributed_saga.h`:284
- Brief: Get Distributed Status.
- Parameters:
  - `saga_id` (const std::string &): Identifier of the saga.
- Return: Return value.
- Details: saga_id Identifier of the saga. Return value.

#### `Metrics getMetrics() const`
- Source: `include/transaction/distributed_saga.h`:342
- Brief: Get Metrics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< DistributedSagaReport > getReport(const std::string &saga_id) const`
- Source: `include/transaction/distributed_saga.h`:323
- Brief: Get Report.
- Parameters:
  - `saga_id` (const std::string &): Identifier of the saga.
- Return: Return value.
- Details: saga_id Identifier of the saga. Return value.

#### `void journalWrite(const std::string &saga_id, const std::string &event, const std::string &detail={})`
- Source: `include/transaction/distributed_saga.h`:412
- Brief: Journal Write.
- Parameters:
  - `saga_id` (const std::string &): Identifier of the saga.
  - `event` (const std::string &): Input parameter.
  - `detail` (const std::string &): Input parameter.
- Details: saga_id Identifier of the saga. event Input parameter. detail Input parameter. Calls: empty(), f(), is_open(), std::chrono::system_clock::now(), time_since_epoch(), count(), reserve(), size().

#### `DistributedSagaCoordinator & operator=(DistributedSagaCoordinator &&) noexcept=default`
- Source: `include/transaction/distributed_saga.h`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSagaCoordinator &&): n/a

#### `DistributedSagaCoordinator & operator=(const DistributedSagaCoordinator &)=delete`
- Source: `include/transaction/distributed_saga.h`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedSagaCoordinator &): n/a

#### `std::vector< std::string > recoverInProgressSAGAs()`
- Source: `include/transaction/distributed_saga.h`:292
- Brief: Recover In Progress SAGAs.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: empty(), f(), is_open(), std::getline(), nlohmann::json::parse(), value(), lk(), emplace().

#### `DistributedSagaStep remoteStepToLocal(const RemoteStep &remote) const`
- Source: `include/transaction/distributed_saga.h`:420
- Brief: Remote Step To Local.
- Parameters:
  - `remote` (const RemoteStep &): Input parameter.
- Return: Return value.
- Details: remote Input parameter. Return value.

#### `std::vector< std::string > topologicalSort(const DistributedSagaDefinition &saga) const`
- Source: `include/transaction/distributed_saga.h`:360
- Brief: Topological Sort.
- Parameters:
  - `saga` (const DistributedSagaDefinition &): Input parameter.
- Return: Return value.
- Details: saga Input parameter. Return value.

#### `DistributedSagaStatus validate(const DistributedSagaDefinition &saga) const`
- Source: `include/transaction/distributed_saga.h`:269
- Brief: Validate.
- Parameters:
  - `saga` (const DistributedSagaDefinition &): Input parameter.
- Return: Return value.
- Details: saga Input parameter. Return value.

#### `bool verifyStepConsensus(const std::string &step_name, const std::string &node_id, StepRecord &record, std::string *failure_detail=nullptr)`
- Source: `include/transaction/distributed_saga.h`:388
- Brief: ───────────────────────────────────────────────────────────────────────────── verifyStepConsensus() — QW-39: Distributed consensus verification ─────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `step_name` (const std::string &): Name of the step.
  - `node_id` (const std::string &): Identifier of the node.
  - `record` (StepRecord &): Input/output parameter.
  - `failure_detail` (std::string *): Input/output parameter.
- Return: True when the operation succeeds.
- Details: step_name Name of the step. node_id Identifier of the node. record Input/output parameter. failure_detail Input/output parameter. True when the operation succeeds.

#### `SagaVisualization visualize(const DistributedSagaDefinition &saga) const`
- Source: `include/transaction/distributed_saga.h`:300
- Brief: Visualize.
- Parameters:
  - `saga` (const DistributedSagaDefinition &): Input parameter.
- Return: Return value.
- Details: saga Input parameter. Return value.

#### `~DistributedSagaCoordinator()=default`
- Source: `include/transaction/distributed_saga.h`:248
- Brief: n/a
- Parameters: none

### themis::DistributedSagaReport

#### `bool succeeded() const`
- Source: `include/transaction/distributed_saga.h`:139
- Brief: n/a
- Parameters: none

### themis::DistributedSagaStatus

#### `DistributedSagaStatus Error(std::string msg)`
- Source: `include/transaction/distributed_saga.h`:50
- Brief: Error.
- Parameters:
  - `msg` (std::string): Input parameter.
- Return: Return value.
- Details: msg Input parameter. Return value. Calls: std::move().

#### `DistributedSagaStatus OK()`
- Source: `include/transaction/distributed_saga.h`:43
- Brief: OK.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements OK without additional internal calls.

### themis::LockManager

#### `LockManager()`
- Source: `include/transaction/lock_manager.h`:86
- Brief: n/a
- Parameters: none

#### `LockManager(const LockManager &)=delete`
- Source: `include/transaction/lock_manager.h`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LockManager &): n/a

#### `LockResult acquireLock(TransactionId txn_id, const std::string &key, LockType type, std::chrono::milliseconds timeout=DEFAULT_LOCK_TIMEOUT)`
- Source: `include/transaction/lock_manager.h`:92
- Brief: Acquire Lock.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
  - `key` (const std::string &): Input parameter.
  - `type` (LockType): Input parameter.
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Return: Return value.
- Details: txn_id Identifier of the txn. key Input parameter. type Input parameter. timeout Input parameter. Return value.

#### `bool acquirePredicateLock(TransactionId txn_id, const std::string &start_key, const std::string &end_key)`
- Source: `include/transaction/lock_manager.h`:181
- Brief: Acquire Predicate Lock.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
  - `start_key` (const std::string &): Input parameter.
  - `end_key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: txn_id Identifier of the txn. start_key Input parameter. end_key Input parameter. True when the operation succeeds.

#### `void beginShrinkingPhase(TransactionId txn_id)`
- Source: `include/transaction/lock_manager.h`:132
- Brief: Begin Shrinking Phase.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
- Details: ------------------------------------------------------------------------ Two-Phase Locking phase management ------------------------------------------------------------------------ txn_id Identifier of the txn. txn_id Identifier of the txn. Calls: lk(), insert(), THEMIS_DEBUG().

#### `void checkEscalation(TransactionId txn_id, const std::string &key)`
- Source: `include/transaction/lock_manager.h`:285
- Brief: Check Escalation.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
  - `key` (const std::string &): Input parameter.
- Details: txn_id Identifier of the txn. key Input parameter. txn_id Identifier of the txn. key Input parameter. Calls: find(), end(), load(), size(), substr(), push_back(), tryGrantLock(), erase().

#### `TransactionId checkPredicateConflict(TransactionId writing_txn_id, const std::string &key) const`
- Source: `include/transaction/lock_manager.h`:221
- Brief: Check Predicate Conflict.
- Parameters:
  - `writing_txn_id` (TransactionId): Identifier of the writing txn.
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: writing_txn_id Identifier of the writing txn. key Input parameter. Return value.

#### `bool compatible(LockType held, LockType requested) noexcept`
- Source: `include/transaction/lock_manager.h`:263
- Brief: Compatible.
- Parameters:
  - `held` (LockType): Input parameter.
  - `requested` (LockType): Input parameter.
- Return: True when the operation succeeds.
- Details: held Input parameter. requested Input parameter. True when the operation succeeds. Exception safety: noexcept.

#### `std::vector< std::pair< std::string, LockType > > getLocksHeld(TransactionId txn_id) const`
- Source: `include/transaction/lock_manager.h`:126
- Brief: n/a
- Parameters:
  - `txn_id` (TransactionId): n/a

#### `size_t getMaxPredicateLocks() const`
- Source: `include/transaction/lock_manager.h`:195
- Brief: Get Max Predicate Locks.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t getPredicateLockCount(TransactionId txn_id) const`
- Source: `include/transaction/lock_manager.h`:229
- Brief: Get Predicate Lock Count.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
- Return: Return value.
- Details: txn_id Identifier of the txn. Return value.

#### `std::vector< std::pair< std::string, std::string > > getPredicateLockRanges(TransactionId txn_id) const`
- Source: `include/transaction/lock_manager.h`:231
- Brief: n/a
- Parameters:
  - `txn_id` (TransactionId): n/a

#### `LockStats getStats() const`
- Source: `include/transaction/lock_manager.h`:157
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< TransactionId > getWaiters(const std::string &key) const`
- Source: `include/transaction/lock_manager.h`:164
- Brief: Get Waiters.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::vector< std::string > getWaitingFor(TransactionId txn_id) const`
- Source: `include/transaction/lock_manager.h`:171
- Brief: Get Waiting For.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
- Return: Return value.
- Details: txn_id Identifier of the txn. Return value.

#### `bool holdsLock(TransactionId txn_id, const std::string &key, LockType type) const`
- Source: `include/transaction/lock_manager.h`:124
- Brief: Holds Lock.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
  - `key` (const std::string &): Input parameter.
  - `type` (LockType): Input parameter.
- Return: True when the operation succeeds.
- Details: txn_id Identifier of the txn. key Input parameter. type Input parameter. True when the operation succeeds.

#### `bool isInShrinkingPhase(TransactionId txn_id) const`
- Source: `include/transaction/lock_manager.h`:139
- Brief: Is In Shrinking Phase.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
- Return: True when the operation succeeds.
- Details: txn_id Identifier of the txn. True when the operation succeeds.

#### `bool isPredicateLockingEnabled() const`
- Source: `include/transaction/lock_manager.h`:207
- Brief: Is Predicate Locking Enabled.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `LockManager & operator=(const LockManager &)=delete`
- Source: `include/transaction/lock_manager.h`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LockManager &): n/a

#### `uint64_t predicateLockDropCount() const noexcept`
- Source: `include/transaction/lock_manager.h`:326
- Brief: n/a
- Parameters: none

#### `void processWaiters(const std::string &key)`
- Source: `include/transaction/lock_manager.h`:278
- Brief: Process Waiters.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Details: key Input parameter. key Input parameter. Calls: find(), end(), compatible(), push_back(), std::chrono::system_clock::now(), notify_one().

#### `void releaseAllLocks(TransactionId txn_id)`
- Source: `include/transaction/lock_manager.h`:110
- Brief: Release All Locks.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
- Details: txn_id Identifier of the txn. txn_id Identifier of the txn. Calls: lk(), find(), end(), erase(), reserve(), size(), push_back(), std::remove_if().

#### `bool releaseLock(TransactionId txn_id, const std::string &key)`
- Source: `include/transaction/lock_manager.h`:104
- Brief: Release Lock.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
  - `key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: txn_id Identifier of the txn. key Input parameter. True when the operation succeeds. txn_id Identifier of the txn. key Input parameter. True when the operation succeeds. Calls: lk(), find(), end(), erase(), empty(), std::remove_if(), begin(), processWaiters().

#### `void releasePredicateLocks(TransactionId txn_id)`
- Source: `include/transaction/lock_manager.h`:213
- Brief: Release Predicate Locks.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `void setDefaultTimeout(std::chrono::milliseconds timeout)`
- Source: `include/transaction/lock_manager.h`:151
- Brief: Set Default Timeout.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Details: timeout Input parameter. timeout Input parameter. Calls: store(), count().

#### `void setEscalationThreshold(size_t threshold)`
- Source: `include/transaction/lock_manager.h`:145
- Brief: Set Escalation Threshold.
- Parameters:
  - `threshold` (size_t): Input parameter.
- Details: threshold Input parameter.

#### `void setMaxPredicateLocks(size_t max_locks)`
- Source: `include/transaction/lock_manager.h`:189
- Brief: Set Max Predicate Locks.
- Parameters:
  - `max_locks` (size_t): Input parameter.
- Details: max_locks Input parameter.

#### `void setPredicateLockingEnabled(bool enabled)`
- Source: `include/transaction/lock_manager.h`:201
- Brief: Set Predicate Locking Enabled.
- Parameters:
  - `enabled` (bool): Input parameter.
- Details: enabled Input parameter.

#### `bool tryGrantLock(const std::string &key, TransactionId txn_id, LockType type)`
- Source: `include/transaction/lock_manager.h`:272
- Brief: Try Grant Lock.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `txn_id` (TransactionId): Identifier of the txn.
  - `type` (LockType): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. txn_id Identifier of the txn. type Input parameter. True when the operation succeeds.

#### `LockResult upgradeLock(TransactionId txn_id, const std::string &key, std::chrono::milliseconds timeout=DEFAULT_LOCK_TIMEOUT)`
- Source: `include/transaction/lock_manager.h`:112
- Brief: Upgrade Lock.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
  - `key` (const std::string &): Input parameter.
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Return: Return value.
- Details: txn_id Identifier of the txn. key Input parameter. timeout Input parameter. Return value.

#### `~LockManager()=default`
- Source: `include/transaction/lock_manager.h`:87
- Brief: n/a
- Parameters: none

### themis::LockManager::LockRequest

#### `LockRequest(TransactionId t, LockType lt)`
- Source: `include/transaction/lock_manager.h`:248
- Brief: n/a
- Parameters:
  - `t` (TransactionId): n/a
  - `lt` (LockType): n/a

### themis::LockManager::LockResult

#### `LockResult Denied(std::string msg)`
- Source: `include/transaction/lock_manager.h`:74
- Brief: Denied.
- Parameters:
  - `msg` (std::string): Input parameter.
- Return: Return value.
- Details: msg Input parameter. Return value. Calls: std::move().

#### `LockResult Granted()`
- Source: `include/transaction/lock_manager.h`:61
- Brief: Granted.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements Granted without additional internal calls.

#### `LockResult Timeout()`
- Source: `include/transaction/lock_manager.h`:67
- Brief: Timeout.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements Timeout without additional internal calls.

### themis::SAGAOrchestrator

#### `SAGAOrchestrator(Config config={})`
- Source: `include/transaction/saga_orchestrator.h`:146
- Brief: n/a
- Parameters:
  - `config` (Config): n/a

#### `SAGAOrchestrator(SAGAOrchestrator &&) noexcept=default`
- Source: `include/transaction/saga_orchestrator.h`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (SAGAOrchestrator &&): n/a

#### `SAGAOrchestrator(const SAGAOrchestrator &)=delete`
- Source: `include/transaction/saga_orchestrator.h`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SAGAOrchestrator &): n/a

#### `StepMap buildStepMap(const SAGADefinition &saga)`
- Source: `include/transaction/saga_orchestrator.h`:281
- Brief: Build Step Map.
- Parameters:
  - `saga` (const SAGADefinition &): Input parameter.
- Return: Return value.
- Details: saga Input parameter. Return value. saga Input parameter. Return value. Calls: reserve(), size(), emplace().

#### `void compensateAll(const SAGADefinition &saga, const StepMap &step_map, const std::vector< std::string > &executed_order, SAGAExecutionStatus &status_rec)`
- Source: `include/transaction/saga_orchestrator.h`:301
- Brief: Compensate All.
- Parameters:
  - `saga` (const SAGADefinition &): Input parameter.
  - `step_map` (const StepMap &): Input parameter.
  - `executed_order` (const std::vector< std::string > &): Input parameter.
  - `status_rec` (SAGAExecutionStatus &): Input/output parameter.
- Details: saga Input parameter. step_map Input parameter. executed_order Input parameter. status_rec Input/output parameter. param Input parameter. step_map Input parameter. executed_order Input parameter. status_rec Input/output parameter. Calls: rbegin(), rend(), find(), end(), compensateStep().

#### `void compensateStep(const SAGAStep &step, SAGAExecutionStatus &status_rec)`
- Source: `include/transaction/saga_orchestrator.h`:311
- Brief: Compensate Step.
- Parameters:
  - `step` (const SAGAStep &): Input parameter.
  - `status_rec` (SAGAExecutionStatus &): Input/output parameter.
- Details: step Input parameter. status_rec Input/output parameter. step Input parameter. status_rec Input/output parameter. Calls: lk(), compensate(), what(), std::string().

#### `SagaOrchestratorStatus execute(const SAGADefinition &saga)`
- Source: `include/transaction/saga_orchestrator.h`:160
- Brief: Execute.
- Parameters:
  - `saga` (const SAGADefinition &): Input parameter.
- Return: Return value.
- Details: saga Input parameter. Return value. saga Input parameter. Return value. Calls: empty(), std::chrono::steady_clock::now(), time_since_epoch(), count(), fetch_add(), std::to_string(), validate(), lk().

#### `StepState executeStep(const SAGAStep &step, const std::string &saga_id, const Config &cfg)`
- Source: `include/transaction/saga_orchestrator.h`:290
- Brief: Execute Step.
- Parameters:
  - `step` (const SAGAStep &): Input parameter.
  - `saga_id` (const std::string &): Identifier of the saga.
  - `cfg` (const Config &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: step Input parameter. saga_id Identifier of the saga. cfg Input parameter. Return value. step Input parameter. saga_id Identifier of the saga. cfg Input parameter. Return value. std::runtime_error if an error occurs. Calls: condition(), lk(), tryAcquireCircuitBreakerExecution(), journalWrite(), effectiveTimeout(), effectiveDelay(), count(), std::async().

#### `Metrics getMetrics() const`
- Source: `include/transaction/saga_orchestrator.h`:192
- Brief: Get Metrics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< SAGAExecutionStatus > getStatus(const std::string &saga_id) const`
- Source: `include/transaction/saga_orchestrator.h`:175
- Brief: Get Status.
- Parameters:
  - `saga_id` (const std::string &): Identifier of the saga.
- Return: Return value.
- Details: saga_id Identifier of the saga. Return value.

#### `SAGADefinition instantiateTemplate(const std::string &template_name, const std::string &instance_id, std::map< std::string, std::string > context_overrides={}) const`
- Source: `include/transaction/saga_orchestrator.h`:202
- Brief: n/a
- Parameters:
  - `template_name` (const std::string &): n/a
  - `instance_id` (const std::string &): n/a
  - `context_overrides` (std::map< std::string, std::string >): n/a

#### `bool isCircuitBreakerOpen(const std::string &step_name) const`
- Source: `include/transaction/saga_orchestrator.h`:244
- Brief: Is Circuit Breaker Open.
- Parameters:
  - `step_name` (const std::string &): Name of the step.
- Return: True when the operation succeeds.
- Details: step_name Name of the step. True when the operation succeeds.

#### `void journalWrite(const std::string &saga_id, const std::string &event, const std::string &detail={})`
- Source: `include/transaction/saga_orchestrator.h`:314
- Brief: Journal Write.
- Parameters:
  - `saga_id` (const std::string &): Identifier of the saga.
  - `event` (const std::string &): Input parameter.
  - `detail` (const std::string &): Input parameter.
- Details: saga_id Identifier of the saga. event Input parameter. detail Input parameter. Calls: empty(), lk(), out(), is_open(), std::chrono::system_clock::now(), time_since_epoch(), count(), jsonEscape().

#### `SAGAOrchestrator & operator=(SAGAOrchestrator &&) noexcept=default`
- Source: `include/transaction/saga_orchestrator.h`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (SAGAOrchestrator &&): n/a

#### `SAGAOrchestrator & operator=(const SAGAOrchestrator &)=delete`
- Source: `include/transaction/saga_orchestrator.h`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SAGAOrchestrator &): n/a

#### `void recordCircuitBreakerFailure(const std::string &step_name)`
- Source: `include/transaction/saga_orchestrator.h`:250
- Brief: Record Circuit Breaker Failure.
- Parameters:
  - `step_name` (const std::string &): Name of the step.
- Details: step_name Name of the step. step_name Name of the step. Calls: lk(), std::chrono::system_clock::now().

#### `void recordCircuitBreakerSuccess(const std::string &step_name)`
- Source: `include/transaction/saga_orchestrator.h`:256
- Brief: Record Circuit Breaker Success.
- Parameters:
  - `step_name` (const std::string &): Name of the step.
- Details: step_name Name of the step. step_name Name of the step. Calls: lk().

#### `void registerTemplate(const std::string &template_name, SAGADefinition tmpl)`
- Source: `include/transaction/saga_orchestrator.h`:200
- Brief: Register Template.
- Parameters:
  - `template_name` (const std::string &): Name of the template.
  - `tmpl` (SAGADefinition): Input parameter.
- Details: template_name Name of the template. tmpl Input parameter. template_name Name of the template. tmpl Input parameter. Calls: lk(), std::move().

#### `std::string renderWorkflow(const SAGADefinition &saga) const`
- Source: `include/transaction/saga_orchestrator.h`:214
- Brief: Render Workflow.
- Parameters:
  - `saga` (const SAGADefinition &): Input parameter.
- Return: Return value.
- Details: saga Input parameter. Return value.

#### `std::vector< std::string > topologicalSort(const SAGADefinition &saga) const`
- Source: `include/transaction/saga_orchestrator.h`:271
- Brief: Topological Sort.
- Parameters:
  - `saga` (const SAGADefinition &): Input parameter.
- Return: Return value.
- Details: saga Input parameter. Return value.

#### `bool tryAcquireCircuitBreakerExecution(const std::string &step_name)`
- Source: `include/transaction/saga_orchestrator.h`:263
- Brief: Try Acquire Circuit Breaker Execution.
- Parameters:
  - `step_name` (const std::string &): Name of the step.
- Return: True when the operation succeeds.
- Details: step_name Name of the step. True when the operation succeeds. step_name Name of the step. True when the operation succeeds. Calls: lk(), find(), end(), std::chrono::system_clock::now().

#### `SagaOrchestratorStatus validate(const SAGADefinition &saga) const`
- Source: `include/transaction/saga_orchestrator.h`:167
- Brief: Validate.
- Parameters:
  - `saga` (const SAGADefinition &): Input parameter.
- Return: Return value.
- Details: saga Input parameter. Return value.

#### `~SAGAOrchestrator()=default`
- Source: `include/transaction/saga_orchestrator.h`:147
- Brief: n/a
- Parameters: none

### themis::Saga

#### `Saga()=default`
- Source: `include/transaction/saga.h`:48
- Brief: n/a
- Parameters: none

#### `Saga(Saga &&) noexcept=default`
- Source: `include/transaction/saga.h`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (Saga &&): n/a

#### `Saga(const Saga &)=delete`
- Source: `include/transaction/saga.h`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (const Saga &): n/a

#### `void addStep(std::string operation_name, CompensatingAction compensate)`
- Source: `include/transaction/saga.h`:62
- Brief: Add Step.
- Parameters:
  - `operation_name` (std::string): Name of the operation.
  - `compensate` (CompensatingAction): Input parameter.
- Details: operation_name Name of the operation. compensate Input parameter. operation_name Name of the operation. compensate Input parameter. Calls: emplace_back(), std::move(), THEMIS_DEBUG(), back(), size().

#### `void clear()`
- Source: `include/transaction/saga.h`:75
- Brief: Clear.
- Parameters: none
- Details: Calls: THEMIS_DEBUG(), size().

#### `void compensate()`
- Source: `include/transaction/saga.h`:67
- Brief: Compensate.
- Parameters: none
- Details: Calls: THEMIS_WARN(), THEMIS_INFO(), size(), rbegin(), rend(), THEMIS_DEBUG(), THEMIS_ERROR(), what().

#### `void compensateWithRetry(int max_retries=3, std::chrono::milliseconds backoff_ms=std::chrono::milliseconds(50))`
- Source: `include/transaction/saga.h`:69
- Brief: Compensate With Retry.
- Parameters:
  - `max_retries` (int): Input parameter.
  - `backoff_ms` (std::chrono::milliseconds): Input parameter.
- Details: max_retries Input parameter. backoff_ms Input parameter. Calls: THEMIS_WARN(), std::min(), THEMIS_INFO(), size(), count(), rbegin(), rend(), THEMIS_DEBUG().

#### `size_t compensatedCount() const`
- Source: `include/transaction/saga.h`:89
- Brief: Compensated Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `int64_t getDurationMs() const`
- Source: `include/transaction/saga.h`:107
- Brief: Get Duration Ms.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Metrics getMetrics() const`
- Source: `include/transaction/saga.h`:121
- Brief: Get Metrics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< std::string > getStepHistory() const`
- Source: `include/transaction/saga.h`:101
- Brief: Get Step History.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool isFullyCompensated() const`
- Source: `include/transaction/saga.h`:95
- Brief: Is Fully Compensated.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `Saga & operator=(Saga &&) noexcept=default`
- Source: `include/transaction/saga.h`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (Saga &&): n/a

#### `Saga & operator=(const Saga &)=delete`
- Source: `include/transaction/saga.h`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (const Saga &): n/a

#### `size_t stepCount() const`
- Source: `include/transaction/saga.h`:83
- Brief: n/a
- Parameters: none

#### `void trimToSize(size_t n)`
- Source: `include/transaction/saga.h`:81
- Brief: Trim To Size.
- Parameters:
  - `n` (size_t): Input parameter.
- Details: n Input parameter. n Input parameter. Calls: size(), THEMIS_DEBUG(), erase(), begin(), end().

#### `~Saga()`
- Source: `include/transaction/saga.h`:49
- Brief: n/a
- Parameters: none

### themis::Saga::Step

#### `Step(std::string name, CompensatingAction action)`
- Source: `include/transaction/saga.h`:42
- Brief: n/a
- Parameters:
  - `name` (std::string): n/a
  - `action` (CompensatingAction): n/a

### themis::SagaOperation

#### `void deleteEntityWithCompensation(RocksDBWrapper &db, const std::string &key, Saga &saga)`
- Source: `include/transaction/saga.h`:154
- Brief: Delete Entity With Compensation.
- Parameters:
  - `db` (RocksDBWrapper &): Input/output parameter.
  - `key` (const std::string &): Input parameter.
  - `saga` (Saga &): Input/output parameter.
- Details: db Input/output parameter. key Input parameter. saga Input/output parameter. db Input/output parameter. key Input parameter. saga Input/output parameter. Calls: get(), has_value(), THEMIS_WARN(), std::move(), addStep(), put(), THEMIS_DEBUG().

#### `void graphAddWithCompensation(GraphIndexManager &graph, const BaseEntity &edge, RocksDBWrapper::WriteBatchWrapper &batch, Saga &saga)`
- Source: `include/transaction/saga.h`:183
- Brief: Graph Add With Compensation.
- Parameters:
  - `graph` (GraphIndexManager &): Input/output parameter.
  - `edge` (const BaseEntity &): Input parameter.
  - `batch` (RocksDBWrapper::WriteBatchWrapper &): Input/output parameter.
  - `saga` (Saga &): Input/output parameter.
- Details: graph Input/output parameter. edge Input parameter. batch Input/output parameter. saga Input/output parameter. graph Input/output parameter. edge Input parameter. batch Input/output parameter. saga Input/output parameter. Calls: getPrimaryKey(), addStep(), deleteEdge(), THEMIS_WARN(), THEMIS_DEBUG().

#### `void indexPutWithCompensation(SecondaryIndexManager &idx, const std::string &table, const BaseEntity &entity, RocksDBWrapper::WriteBatchWrapper &batch, Saga &saga)`
- Source: `include/transaction/saga.h`:168
- Brief: Index Put With Compensation.
- Parameters:
  - `idx` (SecondaryIndexManager &): Input/output parameter.
  - `table` (const std::string &): Input parameter.
  - `entity` (const BaseEntity &): Input parameter.
  - `batch` (RocksDBWrapper::WriteBatchWrapper &): Input/output parameter.
  - `saga` (Saga &): Input/output parameter.
- Details: idx Input/output parameter. table Input parameter. entity Input parameter. batch Input/output parameter. saga Input/output parameter. idx Input/output parameter. table Input parameter. entity Input parameter. batch Input/output parameter. saga Input/output parameter. Calls: getPrimaryKey(), addStep(), erase(), THEMIS_WARN(), THEMIS_DEBUG().

#### `void putEntityWithCompensation(RocksDBWrapper &db, const std::string &key, const std::vector< uint8_t > &value, Saga &saga)`
- Source: `include/transaction/saga.h`:141
- Brief: Put Entity With Compensation.
- Parameters:
  - `db` (RocksDBWrapper &): Input/output parameter.
  - `key` (const std::string &): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
  - `saga` (Saga &): Input/output parameter.
- Details: ========== SAGA Operations ========== db Input/output parameter. key Input parameter. value Input parameter. saga Input/output parameter. db Input/output parameter. key Input parameter. value Input parameter. saga Input/output parameter. Calls: get(), has_value(), std::move(), addStep(), put(), THEMIS_DEBUG(), del().

#### `void vectorAddWithCompensation(VectorIndexManager &vec, const BaseEntity &entity, RocksDBWrapper::WriteBatchWrapper &batch, const std::string &vectorField, Saga &saga)`
- Source: `include/transaction/saga.h`:198
- Brief: Vector Add With Compensation.
- Parameters:
  - `vec` (VectorIndexManager &): Input/output parameter.
  - `entity` (const BaseEntity &): Input parameter.
  - `batch` (RocksDBWrapper::WriteBatchWrapper &): Input/output parameter.
  - `vectorField` (const std::string &): Input parameter.
  - `saga` (Saga &): Input/output parameter.
- Details: vec Input/output parameter. entity Input parameter. batch Input/output parameter. vectorField Input parameter. saga Input/output parameter. vec Input/output parameter. entity Input parameter. batch Input/output parameter. vectorField Input parameter. saga Input/output parameter. Calls: getPrimaryKey(), addStep(), removeByPk(), THEMIS_WARN(), THEMIS_DEBUG().

### themis::SagaOrchestratorStatus

#### `SagaOrchestratorStatus Error(std::string msg)`
- Source: `include/transaction/saga_orchestrator.h`:48
- Brief: Error.
- Parameters:
  - `msg` (std::string): Input parameter.
- Return: Return value.
- Details: msg Input parameter. Return value. Calls: std::move().

#### `SagaOrchestratorStatus OK()`
- Source: `include/transaction/saga_orchestrator.h`:41
- Brief: OK.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements OK without additional internal calls.

### themis::TransactionAuditor

#### `TransactionAuditor()=default`
- Source: `include/transaction/transaction_auditor.h`:91
- Brief: n/a
- Parameters: none

#### `TransactionAuditor(TransactionAuditor &&) noexcept=default`
- Source: `include/transaction/transaction_auditor.h`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditor &&): n/a

#### `TransactionAuditor(const TransactionAuditor &)=delete`
- Source: `include/transaction/transaction_auditor.h`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransactionAuditor &): n/a

#### `void clear()`
- Source: `include/transaction/transaction_auditor.h`:132
- Brief: Clear.
- Parameters: none

#### `void enableAuditing(bool enabled)`
- Source: `include/transaction/transaction_auditor.h`:104
- Brief: Enable Auditing.
- Parameters:
  - `enabled` (bool): Input parameter.
- Details: enabled Input parameter.

#### `Status exportToKafka(const std::string &topic)`
- Source: `include/transaction/transaction_auditor.h`:176
- Brief: Export To Kafka.
- Parameters:
  - `topic` (const std::string &): Input parameter.
- Return: Return value.
- Details: topic Input parameter. Return value.

#### `Status exportToS3(const std::string &bucket, const std::string &prefix)`
- Source: `include/transaction/transaction_auditor.h`:184
- Brief: Export To S3.
- Parameters:
  - `bucket` (const std::string &): Input parameter.
  - `prefix` (const std::string &): Input parameter.
- Return: Return value.
- Details: bucket Input parameter. prefix Input parameter. Return value.

#### `bool isEnabled() const`
- Source: `include/transaction/transaction_auditor.h`:106
- Brief: n/a
- Parameters: none

#### `TransactionAuditor & operator=(TransactionAuditor &&) noexcept=default`
- Source: `include/transaction/transaction_auditor.h`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionAuditor &&): n/a

#### `TransactionAuditor & operator=(const TransactionAuditor &)=delete`
- Source: `include/transaction/transaction_auditor.h`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransactionAuditor &): n/a

#### `std::vector< AuditRecord > queryAuditLog(std::optional< std::string > user_id=std::nullopt, std::optional< std::chrono::system_clock::time_point > start_time=std::nullopt, std::optional< std::chrono::system_clock::time_point > end_time=std::nullopt, size_t limit=1000) const`
- Source: `include/transaction/transaction_auditor.h`:117
- Brief: n/a
- Parameters:
  - `user_id` (std::optional< std::string >): n/a
  - `start_time` (std::optional< std::chrono::system_clock::time_point >): n/a
  - `end_time` (std::optional< std::chrono::system_clock::time_point >): n/a
  - `limit` (size_t): n/a

#### `void record(AuditRecord record)`
- Source: `include/transaction/transaction_auditor.h`:113
- Brief: Record.
- Parameters:
  - `record` (AuditRecord): Input parameter.
- Details: record Input parameter.

#### `void setExportTransport(IAuditExportTransport *transport)`
- Source: `include/transaction/transaction_auditor.h`:168
- Brief: Set Export Transport.
- Parameters:
  - `transport` (IAuditExportTransport *): Input/output parameter.
- Details: transport Input/output parameter.

#### `size_t size() const`
- Source: `include/transaction/transaction_auditor.h`:127
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `~TransactionAuditor()=default`
- Source: `include/transaction/transaction_auditor.h`:92
- Brief: n/a
- Parameters: none

### themis::TransactionAuditor::IAuditExportTransport

#### `Status sendKafka(const std::string &topic, const std::string &ndjson_payload)=0`
- Source: `include/transaction/transaction_auditor.h`:149
- Brief: Send Kafka.
- Parameters:
  - `topic` (const std::string &): Input parameter.
  - `ndjson_payload` (const std::string &): Input parameter.
- Return: Return value.
- Details: topic Input parameter. ndjson_payload Input parameter. Return value.

#### `Status writeS3(const std::string &bucket, const std::string &key, const std::string &ndjson_payload)=0`
- Source: `include/transaction/transaction_auditor.h`:159
- Brief: Write S3.
- Parameters:
  - `bucket` (const std::string &): Input parameter.
  - `key` (const std::string &): Input parameter.
  - `ndjson_payload` (const std::string &): Input parameter.
- Return: Return value.
- Details: bucket Input parameter. key Input parameter. ndjson_payload Input parameter. Return value.

#### `~IAuditExportTransport()=default`
- Source: `include/transaction/transaction_auditor.h`:141
- Brief: IAudit Export Transport.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::TransactionAuditor::Status

#### `Status Error(std::string msg)`
- Source: `include/transaction/transaction_auditor.h`:47
- Brief: Error.
- Parameters:
  - `msg` (std::string): Input parameter.
- Return: Return value.
- Details: msg Input parameter. Return value. Calls: std::move().

#### `Status OK()`
- Source: `include/transaction/transaction_auditor.h`:40
- Brief: OK.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements OK without additional internal calls.

### themis::TransactionBatcher

#### `TransactionBatcher()`
- Source: `include/transaction/transaction_batcher.h`:91
- Brief: n/a
- Parameters: none

#### `TransactionBatcher(TransactionBatcher &&)=delete`
- Source: `include/transaction/transaction_batcher.h`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcher &&): n/a

#### `TransactionBatcher(const TransactionBatcher &)=delete`
- Source: `include/transaction/transaction_batcher.h`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransactionBatcher &): n/a

#### `void adaptWindow(size_t batch_size, std::chrono::microseconds elapsed)`
- Source: `include/transaction/transaction_batcher.h`:186
- Brief: Adapt Window.
- Parameters:
  - `batch_size` (size_t): Input parameter.
  - `elapsed` (std::chrono::microseconds): Input parameter.
- Details: batch_size Input parameter. elapsed Input parameter.

#### `EffectivePolicy effectivePolicyFor(const std::string &table) const`
- Source: `include/transaction/transaction_batcher.h`:167
- Brief: Effective Policy For.
- Parameters:
  - `table` (const std::string &): Input parameter.
- Return: Return value.
- Details: table Input parameter. Return value.

#### `void executeBatch(std::vector< PendingEntry > &batch)`
- Source: `include/transaction/transaction_batcher.h`:179
- Brief: Execute Batch.
- Parameters:
  - `batch` (std::vector< PendingEntry > &): Input/output parameter.
- Details: batch Input/output parameter.

#### `void flush()`
- Source: `include/transaction/transaction_batcher.h`:136
- Brief: Flush.
- Parameters: none

#### `void flushLoop()`
- Source: `include/transaction/transaction_batcher.h`:173
- Brief: Flush Loop.
- Parameters: none

#### `BatchConfig getBatchConfig() const`
- Source: `include/transaction/transaction_batcher.h`:112
- Brief: Get Batch Config.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Stats getStats() const`
- Source: `include/transaction/transaction_batcher.h`:142
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `BatchPolicy getTablePolicy(const std::string &table) const`
- Source: `include/transaction/transaction_batcher.h`:126
- Brief: Get Table Policy.
- Parameters:
  - `table` (const std::string &): Input parameter.
- Return: Return value.
- Details: table Input parameter. Return value.

#### `TransactionBatcher & operator=(TransactionBatcher &&)=delete`
- Source: `include/transaction/transaction_batcher.h`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionBatcher &&): n/a

#### `TransactionBatcher & operator=(const TransactionBatcher &)=delete`
- Source: `include/transaction/transaction_batcher.h`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransactionBatcher &): n/a

#### `void setBatchConfig(const BatchConfig &config)`
- Source: `include/transaction/transaction_batcher.h`:106
- Brief: Set Batch Config.
- Parameters:
  - `config` (const BatchConfig &): Input parameter.
- Details: config Input parameter.

#### `void setTablePolicy(const std::string &table, const BatchPolicy &policy)`
- Source: `include/transaction/transaction_batcher.h`:119
- Brief: Set Table Policy.
- Parameters:
  - `table` (const std::string &): Input parameter.
  - `policy` (const BatchPolicy &): Input parameter.
- Details: table Input parameter. policy Input parameter.

#### `std::future< Status > submitAsync(std::function< Status()> commit_fn, const std::string &table_hint="")`
- Source: `include/transaction/transaction_batcher.h`:130
- Brief: n/a
- Parameters:
  - `commit_fn` (std::function< Status()>): n/a
  - `table_hint` (const std::string &): n/a

#### `~TransactionBatcher()`
- Source: `include/transaction/transaction_batcher.h`:93
- Brief: n/a
- Parameters: none

### themis::TransactionBatcher::Status

#### `Status Error(std::string msg)`
- Source: `include/transaction/transaction_batcher.h`:48
- Brief: Error.
- Parameters:
  - `msg` (std::string): Input parameter.
- Return: Return value.
- Details: msg Input parameter. Return value. Calls: std::move().

#### `Status OK()`
- Source: `include/transaction/transaction_batcher.h`:41
- Brief: OK.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements OK without additional internal calls.

### themis::TransactionManager

#### `TransactionManager(RocksDBWrapper &db, SecondaryIndexManager &secIdx, GraphIndexManager &graphIdx, VectorIndexManager &vecIdx)`
- Source: `include/transaction/transaction_manager.h`:96
- Brief: Transaction Manager.
- Parameters:
  - `db` (RocksDBWrapper &): Input/output parameter.
  - `secIdx` (SecondaryIndexManager &): Input/output parameter.
  - `graphIdx` (GraphIndexManager &): Input/output parameter.
  - `vecIdx` (VectorIndexManager &): Input/output parameter.
- Return: Return value.
- Details: db Input/output parameter. secIdx Input/output parameter. graphIdx Input/output parameter. vecIdx Input/output parameter. Return value.

#### `size_t abortTenantTransactions(std::string_view tenant_id)`
- Source: `include/transaction/transaction_manager.h`:522
- Brief: Abort Tenant Transactions.
- Parameters:
  - `tenant_id` (std::string_view): Identifier of the tenant.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. Return value. tenant_id Identifier of the tenant. Return value. Calls: lock(), push_back(), rollbackTransaction(), THEMIS_INFO(), size().

#### `size_t abortTimedOutTransactions()`
- Source: `include/transaction/transaction_manager.h`:593
- Brief: Abort Timed Out Transactions.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: load(), std::chrono::system_clock::now(), std::chrono::milliseconds(), lock(), isFinished(), getStartTime(), push_back(), THEMIS_WARN().

#### `void applyDefaultTimeout(Transaction &txn) const`
- Source: `include/transaction/transaction_manager.h`:913
- Brief: Apply Default Timeout.
- Parameters:
  - `txn` (Transaction &): Input/output parameter. apply default timeout if configured
- Details: txn Input/output parameter. apply default timeout if configured

#### `Transaction begin(IsolationLevel isolation=IsolationLevel::ReadCommitted)`
- Source: `include/transaction/transaction_manager.h`:525
- Brief: Direct transaction (legacy API).
- Parameters:
  - `isolation` (IsolationLevel): Input parameter.
- Return: Return value.
- Details: isolation Input parameter. Return value. Calls: generateTransactionId(), updateStatsWithSeqLock(), fetch_add(), txn(), applyDefaultTimeout().

#### `TransactionId beginTransaction(IsolationLevel isolation=IsolationLevel::ReadCommitted)`
- Source: `include/transaction/transaction_manager.h`:449
- Brief: Begin Transaction.
- Parameters:
  - `isolation` (IsolationLevel): Input parameter.
- Return: Return value.
- Details: isolation Input parameter. Return value. Calls: generateTransactionId(), applyDefaultTimeout(), lock(), updateStatsWithSeqLock(), fetch_add(), THEMIS_INFO(), logBegin().

#### `TransactionId beginTransaction(std::string_view tenant_id, IsolationLevel isolation=IsolationLevel::ReadCommitted)`
- Source: `include/transaction/transaction_manager.h`:451
- Brief: Begin Transaction.
- Parameters:
  - `tenant_id` (std::string_view): Identifier of the tenant.
  - `isolation` (IsolationLevel): Input parameter.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. isolation Input parameter. Return value.

#### `void cleanupOldTransactions(std::chrono::seconds max_age=std::chrono::hours(1))`
- Source: `include/transaction/transaction_manager.h`:568
- Brief: Cleanup Old Transactions.
- Parameters:
  - `max_age` (std::chrono::seconds): Input parameter.
- Details: max_age Input parameter. Calls: lock(), std::chrono::system_clock::now(), begin(), end(), getStartTime(), erase().

#### `void clearWaiting(TransactionId txn_id)`
- Source: `include/transaction/transaction_manager.h`:985
- Brief: Clear Waiting.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
- Details: txn_id Identifier of the txn. txn_id Identifier of the txn. Calls: load(), lock(), erase().

#### `Status commitTransaction(TransactionId id)`
- Source: `include/transaction/transaction_manager.h`:465
- Brief: Commit Transaction.
- Parameters:
  - `id` (TransactionId): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value. id Input parameter. Return value. Calls: lock(), find(), end(), Status::Error(), commit(), updateStatsWithSeqLock(), fetch_add(), empty().

#### `size_t countActiveTenantTransactionsLocked(std::string_view tenant_id) const`
- Source: `include/transaction/transaction_manager.h`:901
- Brief: Count Active Tenant Transactions Locked.
- Parameters:
  - `tenant_id` (std::string_view): Identifier of the tenant.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. Return value.

#### `transaction::CrashRecoveryManager::RecoveryResult crashRecover()`
- Source: `include/transaction/transaction_manager.h`:712
- Brief: Crash Recover.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void deadlockDetectorLoop()`
- Source: `include/transaction/transaction_manager.h`:950
- Brief: Deadlock Detector Loop.
- Parameters: none
- Details: Calls: load(), lock(), std::chrono::milliseconds(), wait_for(), abortTimedOutTransactions(), detectDeadlockCycle(), THEMIS_WARN(), size().

#### `std::vector< SerializationConflict > detectConflicts(TransactionId txn_id) const`
- Source: `include/transaction/transaction_manager.h`:843
- Brief: Detect Conflicts.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
- Return: Return value.
- Details: txn_id Identifier of the txn. Return value.

#### `bool detectDeadlockCycle(std::vector< TransactionId > &cycle)`
- Source: `include/transaction/transaction_manager.h`:956
- Brief: Detect Deadlock Cycle.
- Parameters:
  - `cycle` (std::vector< TransactionId > &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: cycle Input/output parameter. True when the operation succeeds. cycle Input/output parameter. True when the operation succeeds. Calls: lock(), empty(), find(), end(), insert(), bool(), push_back(), count().

#### `void enableCrashRecovery(const std::string &wal_path, bool sync_on_write=true)`
- Source: `include/transaction/transaction_manager.h`:699
- Brief: ── Phase 8: Durability & Crash-Recovery ─────────────────────────────────────
- Parameters:
  - `wal_path` (const std::string &): Path to the wal.
  - `sync_on_write` (bool): Input parameter.
- Details: wal_path Path to the wal. sync_on_write Input parameter. Calls: lock(), THEMIS_INFO().

#### `std::optional< Transaction::ExplainResult > explainTransaction(TransactionId id) const`
- Source: `include/transaction/transaction_manager.h`:478
- Brief: Explain Transaction.
- Parameters:
  - `id` (TransactionId): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value.

#### `TransactionId generateTransactionId()`
- Source: `include/transaction/transaction_manager.h`:878
- Brief: Generate Transaction Id.
- Parameters: none
- Return: Return value.
- Details: Session-based transaction management. Return value. Return value. Calls: fetch_add().

#### `size_t getActiveTenantTransactionCount(std::string_view tenant_id) const`
- Source: `include/transaction/transaction_manager.h`:508
- Brief: Get Active Tenant Transaction Count.
- Parameters:
  - `tenant_id` (std::string_view): Identifier of the tenant.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. Return value.

#### `std::vector< TenantTransactionStats > getAllTenantTransactionStats() const`
- Source: `include/transaction/transaction_manager.h`:501
- Brief: Get All Tenant Transaction Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `transaction::CrashRecoveryManager * getCrashRecoveryManager()`
- Source: `include/transaction/transaction_manager.h`:719
- Brief: Get Crash Recovery Manager.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result. Calls: get().

#### `const transaction::CrashRecoveryManager * getCrashRecoveryManager() const`
- Source: `include/transaction/transaction_manager.h`:722
- Brief: n/a
- Parameters: none

#### `uint64_t getDeadlockCount() const`
- Source: `include/transaction/transaction_manager.h`:644
- Brief: n/a
- Parameters: none

#### `DeadlockMetrics getDeadlockMetrics() const`
- Source: `include/transaction/transaction_manager.h`:650
- Brief: Get Deadlock Metrics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `DeadlockPredictor * getDeadlockPredictor() const`
- Source: `include/transaction/transaction_manager.h`:663
- Brief: Get Deadlock Predictor.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result.

#### `DeadlockVictimPolicy getDeadlockVictimPolicy() const`
- Source: `include/transaction/transaction_manager.h`:640
- Brief: Get Deadlock Victim Policy.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< DeadlockInfo > getDeadlocks(std::chrono::seconds max_age=std::chrono::hours(24)) const`
- Source: `include/transaction/transaction_manager.h`:642
- Brief: n/a
- Parameters:
  - `max_age` (std::chrono::seconds): n/a

#### `std::chrono::milliseconds getDefaultTransactionTimeout() const`
- Source: `include/transaction/transaction_manager.h`:538
- Brief: Get Default Transaction Timeout.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `LockManager & getLockManager()`
- Source: `include/transaction/transaction_manager.h`:694
- Brief: Get Lock Manager.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getLockManager without additional internal calls.

#### `const LockManager & getLockManager() const`
- Source: `include/transaction/transaction_manager.h`:695
- Brief: n/a
- Parameters: none

#### `SSIConfig getSSIConfig() const`
- Source: `include/transaction/transaction_manager.h`:826
- Brief: Get SSIConfig.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Stats getStats() const`
- Source: `include/transaction/transaction_manager.h`:559
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Stats getStatsLockFree() const`
- Source: `include/transaction/transaction_manager.h`:565
- Brief: Get Stats Lock Free.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `TenantTransactionStats getTenantTransactionStats(std::string_view tenant_id) const`
- Source: `include/transaction/transaction_manager.h`:495
- Brief: Get Tenant Transaction Stats.
- Parameters:
  - `tenant_id` (std::string_view): Identifier of the tenant.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. Return value.

#### `uint64_t getTimedOutCount() const`
- Source: `include/transaction/transaction_manager.h`:587
- Brief: Get Timed Out Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `uint64_t getTimeoutCount() const`
- Source: `include/transaction/transaction_manager.h`:540
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< Transaction > getTransaction(TransactionId id)`
- Source: `include/transaction/transaction_manager.h`:459
- Brief: Get Transaction.
- Parameters:
  - `id` (TransactionId): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value. id Input parameter. Return value. Calls: lock(), find(), end().

#### `std::chrono::milliseconds getTransactionTimeout() const`
- Source: `include/transaction/transaction_manager.h`:581
- Brief: Get Transaction Timeout.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< TimeTravelRecord > listEntityVersions(std::string_view table, std::string_view pk) const`
- Source: `include/transaction/transaction_manager.h`:802
- Brief: List Entity Versions.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: table Input parameter. pk Input parameter. Return value.

#### `std::vector< TransactionId > listTenantTransactionIds(std::string_view tenant_id) const`
- Source: `include/transaction/transaction_manager.h`:515
- Brief: List Tenant Transaction Ids.
- Parameters:
  - `tenant_id` (std::string_view): Identifier of the tenant.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. Return value.

#### `void moveToCompleted(TransactionId id)`
- Source: `include/transaction/transaction_manager.h`:883
- Brief: Move To Completed.
- Parameters:
  - `id` (TransactionId): Input parameter.
- Details: id Input parameter. id Input parameter. Calls: lock(), find(), end(), THEMIS_WARN(), erase(), std::move().

#### `bool needsCrashRecovery() const`
- Source: `include/transaction/transaction_manager.h`:706
- Brief: Needs Crash Recovery.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `double predictDeadlockProbability(const std::vector< std::string > &proposed_locks) const`
- Source: `include/transaction/transaction_manager.h`:670
- Brief: Predict Deadlock Probability.
- Parameters:
  - `proposed_locks` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: proposed_locks Input parameter. Return value.

#### `std::optional< TimeTravelRecord > readEntityAtSnapshot(std::string_view table, std::string_view pk, const std::string &tag_name) const`
- Source: `include/transaction/transaction_manager.h`:791
- Brief: Read Entity At Snapshot.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
  - `tag_name` (const std::string &): Name of the tag.
- Return: Return value.
- Details: table Input parameter. pk Input parameter. tag_name Name of the tag. Return value.

#### `std::optional< TimeTravelRecord > readEntityAtTimestamp(std::string_view table, std::string_view pk, HLCTimestamp ts) const`
- Source: `include/transaction/transaction_manager.h`:767
- Brief: Read Entity At Timestamp.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
  - `ts` (HLCTimestamp): Input parameter.
- Return: Return value.
- Details: table Input parameter. pk Input parameter. ts Input parameter. Return value.

#### `std::optional< TimeTravelRecord > readEntityAtUnixMs(std::string_view table, std::string_view pk, int64_t unix_ms) const`
- Source: `include/transaction/transaction_manager.h`:779
- Brief: Read Entity At Unix Ms.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
  - `unix_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: table Input parameter. pk Input parameter. unix_ms Input parameter. Return value.

#### `std::vector< std::string > recommendLockOrder(const std::vector< std::string > &keys) const`
- Source: `include/transaction/transaction_manager.h`:678
- Brief: Recommend Lock Order.
- Parameters:
  - `keys` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: keys Input parameter. Return value.

#### `std::chrono::milliseconds recommendTimeout(const std::vector< std::string > &keys) const`
- Source: `include/transaction/transaction_manager.h`:686
- Brief: Recommend Timeout.
- Parameters:
  - `keys` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: keys Input parameter. Return value.

#### `void resolveDeadlock(const std::vector< TransactionId > &cycle)`
- Source: `include/transaction/transaction_manager.h`:961
- Brief: Resolve Deadlock.
- Parameters:
  - `cycle` (const std::vector< TransactionId > &): Input parameter.
- Details: cycle Input parameter. cycle Input parameter. Calls: empty(), load(), std::min_element(), begin(), end(), THEMIS_WARN(), max(), lk().

#### `bool rollbackTransaction(TransactionId id)`
- Source: `include/transaction/transaction_manager.h`:471
- Brief: Rollback Transaction.
- Parameters:
  - `id` (TransactionId): Input parameter.
- Return: True when the operation succeeds.
- Details: id Input parameter. True when the operation succeeds. id Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), rollback(), updateStatsWithSeqLock(), fetch_add(), empty(), THEMIS_INFO().

#### `void setConflictManager(ConflictManager *mgr)`
- Source: `include/transaction/transaction_manager.h`:739
- Brief: Set Conflict Manager.
- Parameters:
  - `mgr` (ConflictManager *): Input/output parameter.
- Details: mgr Input/output parameter. Implements setConflictManager without additional internal calls.

#### `void setDeadlockDetection(bool enabled)`
- Source: `include/transaction/transaction_manager.h`:622
- Brief: Set Deadlock Detection.
- Parameters:
  - `enabled` (bool): Input parameter.
- Details: enabled Input parameter.

#### `void setDeadlockPredictor(DeadlockPredictor *predictor)`
- Source: `include/transaction/transaction_manager.h`:657
- Brief: Set Deadlock Predictor.
- Parameters:
  - `predictor` (DeadlockPredictor *): Input/output parameter.
- Details: ── Adaptive Deadlock Prevention (v1. predictor Input/output parameter. predictor Input/output parameter. 9.0) ──────────────────────────────────── Calls: store().

#### `void setDeadlockTimeout(std::chrono::milliseconds timeout_ms)`
- Source: `include/transaction/transaction_manager.h`:628
- Brief: Set Deadlock Timeout.
- Parameters:
  - `timeout_ms` (std::chrono::milliseconds): Input parameter.
- Details: timeout_ms Input parameter. timeout_ms Input parameter. Calls: store(), count(), THEMIS_INFO(), lock(), notify_one().

#### `void setDeadlockVictimPolicy(DeadlockVictimPolicy policy)`
- Source: `include/transaction/transaction_manager.h`:634
- Brief: Set Deadlock Victim Policy.
- Parameters:
  - `policy` (DeadlockVictimPolicy): Input parameter.
- Details: policy Input parameter. policy Input parameter. Calls: store(), THEMIS_INFO().

#### `void setDefaultTransactionTimeout(std::chrono::milliseconds timeout)`
- Source: `include/transaction/transaction_manager.h`:532
- Brief: Set Default Transaction Timeout.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Details: timeout Input parameter. timeout Input parameter. Calls: count(), store(), THEMIS_INFO().

#### `void setHistoryManager(HistoryManager *mgr)`
- Source: `include/transaction/transaction_manager.h`:732
- Brief: Set History Manager.
- Parameters:
  - `mgr` (HistoryManager *): Input/output parameter.
- Details: mgr Input/output parameter. Implements setHistoryManager without additional internal calls.

#### `void setSSIConfig(const SSIConfig &config)`
- Source: `include/transaction/transaction_manager.h`:820
- Brief: Set SSIConfig.
- Parameters:
  - `config` (const SSIConfig &): Input parameter.
- Details: ── Serializable Snapshot Isolation (SSI) ───────────────────────────────────── config Input parameter. config Input parameter. Calls: lock(), setPredicateLockingEnabled(), setMaxPredicateLocks(), THEMIS_INFO(), count().

#### `void setSnapshotManager(transaction::SnapshotManager *mgr)`
- Source: `include/transaction/transaction_manager.h`:756
- Brief: Set Snapshot Manager.
- Parameters:
  - `mgr` (transaction::SnapshotManager *): Input/output parameter.
- Details: mgr Input/output parameter. Implements setSnapshotManager without additional internal calls.

#### `void setTransactionTimeout(std::chrono::milliseconds timeout_ms)`
- Source: `include/transaction/transaction_manager.h`:575
- Brief: Set Transaction Timeout.
- Parameters:
  - `timeout_ms` (std::chrono::milliseconds): Input parameter.
- Details: ── Transaction Timeout / Auto-Rollback ────────────────────────────────────── timeout_ms Input parameter. timeout_ms Input parameter. Calls: exchange(), count(), THEMIS_DEBUG().

#### `void timeoutExpiredTransactions()`
- Source: `include/transaction/transaction_manager.h`:908
- Brief: Timeout Expired Transactions.
- Parameters: none
- Details: roll back active transactions that exceeded their timeout Calls: lock(), isFinished(), isTimedOut(), push_back(), THEMIS_WARN(), rollbackTransaction(), fetch_add().

#### `void trackLockAcquired(TransactionId txn_id, const std::string &key)`
- Source: `include/transaction/transaction_manager.h`:968
- Brief: Track Lock Acquired.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
  - `key` (const std::string &): Input parameter.
- Details: txn_id Identifier of the txn. key Input parameter. txn_id Identifier of the txn. key Input parameter. Calls: load(), lock(), std::chrono::system_clock::now(), find(), end(), erase(), empty().

#### `void trackLockReleased(TransactionId txn_id, const std::string &key)`
- Source: `include/transaction/transaction_manager.h`:974
- Brief: Track Lock Released.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
  - `key` (const std::string &): Input parameter.
- Details: txn_id Identifier of the txn. key Input parameter. txn_id Identifier of the txn. key Input parameter. Calls: load(), lock(), find(), end(), erase().

#### `void trackLockWaiting(TransactionId txn_id, const std::string &key)`
- Source: `include/transaction/transaction_manager.h`:980
- Brief: Track Lock Waiting.
- Parameters:
  - `txn_id` (TransactionId): Identifier of the txn.
  - `key` (const std::string &): Input parameter.
- Details: txn_id Identifier of the txn. key Input parameter. txn_id Identifier of the txn. key Input parameter. Calls: load(), lock(), insert().

#### `void updateStatsWithSeqLock(std::function< void()> update)`
- Source: `include/transaction/transaction_manager.h`:886
- Brief: n/a
- Parameters:
  - `update` (std::function< void()>): n/a

#### `~TransactionManager()`
- Source: `include/transaction/transaction_manager.h`:100
- Brief: n/a
- Parameters: none

### themis::TransactionManager::Status

#### `Status Conflict(std::string msg, std::string cid, std::vector< std::string > keys)`
- Source: `include/transaction/transaction_manager.h`:76
- Brief: Conflict.
- Parameters:
  - `msg` (std::string): Input parameter.
  - `cid` (std::string): Input parameter.
  - `keys` (std::vector< std::string >): Input parameter.
- Return: Return value.
- Details: msg Input parameter. cid Input parameter. keys Input parameter. Return value. Calls: std::move().

#### `Status Error(std::string msg)`
- Source: `include/transaction/transaction_manager.h`:67
- Brief: Error.
- Parameters:
  - `msg` (std::string): Input parameter.
- Return: Return value.
- Details: msg Input parameter. Return value. Calls: std::move().

#### `Status OK()`
- Source: `include/transaction/transaction_manager.h`:60
- Brief: OK.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements OK without additional internal calls.

### themis::TransactionManager::Transaction

#### `Transaction(Transaction &&) noexcept`
- Source: `include/transaction/transaction_manager.h`:117
- Brief: n/a
- Parameters:
  - `other` (Transaction &&): n/a

#### `Transaction(TransactionId id, RocksDBWrapper &db, SecondaryIndexManager &secIdx, GraphIndexManager &graphIdx, VectorIndexManager &vecIdx, IsolationLevel isolation, LockManager *lock_manager=nullptr, std::string_view tenant_id={})`
- Source: `include/transaction/transaction_manager.h`:104
- Brief: n/a
- Parameters:
  - `id` (TransactionId): n/a
  - `db` (RocksDBWrapper &): n/a
  - `secIdx` (SecondaryIndexManager &): n/a
  - `graphIdx` (GraphIndexManager &): n/a
  - `vecIdx` (VectorIndexManager &): n/a
  - `isolation` (IsolationLevel): n/a
  - `lock_manager` (LockManager *): n/a
  - `tenant_id` (std::string_view): n/a

#### `Transaction(const Transaction &)=delete`
- Source: `include/transaction/transaction_manager.h`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (const Transaction &): n/a

#### `Status addEdge(const BaseEntity &edgeEntity)`
- Source: `include/transaction/transaction_manager.h`:189
- Brief: Add Edge.
- Parameters:
  - `edgeEntity` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: edgeEntity Input parameter. Return value. edgeEntity Input parameter. Return value. Calls: isActive(), Status::Error(), isTimedOut(), makeNamespacedKey(), getPrimaryKey(), checkSerializableWriteConflict(), empty(), serialize().

#### `Status addVector(const BaseEntity &entity, std::string_view vectorField="embedding")`
- Source: `include/transaction/transaction_manager.h`:198
- Brief: Add Vector.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `vectorField` (std::string_view): Input parameter.
- Return: Return value.
- Details: entity Input parameter. vectorField Input parameter. Return value. Calls: isActive(), Status::Error(), isTimedOut(), getPrimaryKey(), makeNamespacedKey(), checkSerializableWriteConflict(), empty(), serialize().

#### `Status bulkEraseEntities(std::string_view table, const std::vector< std::string > &pks)`
- Source: `include/transaction/transaction_manager.h`:337
- Brief: Bulk Erase Entities.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `pks` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: table Input parameter. pks Input parameter. Return value.

#### `Status bulkPutEntities(std::string_view table, const std::vector< BaseEntity > &entities)`
- Source: `include/transaction/transaction_manager.h`:328
- Brief: Bulk Put Entities.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `entities` (const std::vector< BaseEntity > &): Input parameter.
- Return: Return value.
- Details: table Input parameter. entities Input parameter. Return value.

#### `void captureDuration() noexcept`
- Source: `include/transaction/transaction_manager.h`:417
- Brief: Capture Duration.
- Parameters: none
- Details: Exception safety: noexcept.

#### `std::string checkSerializableWriteConflict(const std::string &key) const`
- Source: `include/transaction/transaction_manager.h`:265
- Brief: Check Serializable Write Conflict.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `Status commit()`
- Source: `include/transaction/transaction_manager.h`:212
- Brief: Commit.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: compare_exchange_strong(), Status::Error(), captureDuration(), isActive(), isTimedOut(), THEMIS_WARN(), getDurationMs(), rollback().

#### `Status createSavepoint(std::string_view name)`
- Source: `include/transaction/transaction_manager.h`:292
- Brief: Create Savepoint.
- Parameters:
  - `name` (std::string_view): Input parameter.
- Return: Return value.
- Details: name Input parameter. Return value. name Input parameter. Return value. Calls: load(), Status::Error(), isActive(), empty(), sname(), reserve(), size(), setSavePoint().

#### `Status deleteEdge(std::string_view edgeId)`
- Source: `include/transaction/transaction_manager.h`:195
- Brief: Delete Edge.
- Parameters:
  - `edgeId` (std::string_view): Input parameter.
- Return: Return value.
- Details: edgeId Input parameter. Return value. edgeId Input parameter. Return value. Calls: isActive(), Status::Error(), isTimedOut(), makeNamespacedKey(), std::string(), checkSerializableWriteConflict(), empty(), del().

#### `Status eraseEntity(std::string_view table, std::string_view pk)`
- Source: `include/transaction/transaction_manager.h`:172
- Brief: Erase Entity.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: table Input parameter. pk Input parameter. Return value. table Input parameter. pk Input parameter. Return value. Calls: isActive(), Status::Error(), isTimedOut(), makeNamespacedKey(), std::string(), checkSerializableWriteConflict(), empty(), count().

#### `ExplainResult explain() const`
- Source: `include/transaction/transaction_manager.h`:382
- Brief: Explain.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `uint64_t getDurationMs() const`
- Source: `include/transaction/transaction_manager.h`:128
- Brief: Get Duration Ms.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< uint64_t > getEntityVersion(std::string_view table, std::string_view pk)`
- Source: `include/transaction/transaction_manager.h`:225
- Brief: Get Entity Version.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: table Input parameter. pk Input parameter. Return value.

#### `TransactionId getId() const`
- Source: `include/transaction/transaction_manager.h`:121
- Brief: n/a
- Parameters: none

#### `IsolationLevel getIsolationLevel() const`
- Source: `include/transaction/transaction_manager.h`:122
- Brief: n/a
- Parameters: none

#### `Saga & getSaga()`
- Source: `include/transaction/transaction_manager.h`:354
- Brief: Get Saga.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getSaga without additional internal calls.

#### `const Saga & getSaga() const`
- Source: `include/transaction/transaction_manager.h`:355
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > getSavepoints() const`
- Source: `include/transaction/transaction_manager.h`:312
- Brief: Get Savepoints.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::system_clock::time_point getStartTime() const`
- Source: `include/transaction/transaction_manager.h`:123
- Brief: n/a
- Parameters: none

#### `const std::string & getTenantId() const`
- Source: `include/transaction/transaction_manager.h`:131
- Brief: n/a
- Parameters: none

#### `std::chrono::milliseconds getTimeout() const`
- Source: `include/transaction/transaction_manager.h`:146
- Brief: n/a
- Parameters: none

#### `bool hasSavepoint(std::string_view name) const`
- Source: `include/transaction/transaction_manager.h`:319
- Brief: Has Savepoint.
- Parameters:
  - `name` (std::string_view): Input parameter.
- Return: True when the operation succeeds.
- Details: name Input parameter. True when the operation succeeds.

#### `bool hasWrites() const`
- Source: `include/transaction/transaction_manager.h`:346
- Brief: n/a
- Parameters: none

#### `bool isFinished() const`
- Source: `include/transaction/transaction_manager.h`:129
- Brief: n/a
- Parameters: none

#### `bool isReadOnly() const`
- Source: `include/transaction/transaction_manager.h`:344
- Brief: n/a
- Parameters: none

#### `bool isTimedOut() const`
- Source: `include/transaction/transaction_manager.h`:150
- Brief: n/a
- Parameters: none

#### `std::string makeNamespacedKey(std::string_view key) const`
- Source: `include/transaction/transaction_manager.h`:438
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a

#### `std::string makeNamespacedTable(std::string_view table) const`
- Source: `include/transaction/transaction_manager.h`:431
- Brief: n/a
- Parameters:
  - `table` (std::string_view): n/a

#### `Transaction & operator=(Transaction &&) noexcept`
- Source: `include/transaction/transaction_manager.h`:118
- Brief: n/a
- Parameters:
  - `other` (Transaction &&): n/a

#### `Transaction & operator=(const Transaction &)=delete`
- Source: `include/transaction/transaction_manager.h`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (const Transaction &): n/a

#### `Status optimisticErase(std::string_view table, std::string_view pk, uint64_t expected_version)`
- Source: `include/transaction/transaction_manager.h`:246
- Brief: Optimistic Erase.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
  - `expected_version` (uint64_t): Input parameter.
- Return: Return value.
- Details: table Input parameter. pk Input parameter. expected_version Input parameter. Return value.

#### `Status optimisticPut(std::string_view table, const BaseEntity &entity, uint64_t expected_version)`
- Source: `include/transaction/transaction_manager.h`:235
- Brief: Optimistic Put.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `entity` (const BaseEntity &): Input parameter.
  - `expected_version` (uint64_t): Input parameter.
- Return: Return value.
- Details: table Input parameter. entity Input parameter. expected_version Input parameter. Return value.

#### `Status popSavePoint()`
- Source: `include/transaction/transaction_manager.h`:284
- Brief: Pop Save Point.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: load(), Status::Error(), isActive(), Status::OK().

#### `Status putEntity(std::string_view table, const BaseEntity &entity)`
- Source: `include/transaction/transaction_manager.h`:165
- Brief: Put Entity.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `entity` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: table Input parameter. entity Input parameter. Return value. table Input parameter. entity Input parameter. Return value. Calls: isActive(), Status::Error(), isTimedOut(), serialize(), makeNamespacedKey(), std::string(), getPrimaryKey(), checkSerializableWriteConflict().

#### `std::optional< std::string > readEntityJson(std::string_view table, std::string_view pk)`
- Source: `include/transaction/transaction_manager.h`:180
- Brief: Read Entity Json.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: table Input parameter. pk Input parameter. Return value. table Input parameter. pk Input parameter. Return value. Calls: isActive(), makeNamespacedKey(), std::string(), get(), BaseEntity::deserialize(), toJson().

#### `Status releaseSavepoint(std::string_view name)`
- Source: `include/transaction/transaction_manager.h`:306
- Brief: Release Savepoint.
- Parameters:
  - `name` (std::string_view): Input parameter.
- Return: Return value.
- Details: name Input parameter. Return value. name Input parameter. Return value. Calls: load(), Status::Error(), isActive(), sname(), std::find_if(), begin(), end(), popSavePoint().

#### `Status removeVector(std::string_view pk)`
- Source: `include/transaction/transaction_manager.h`:205
- Brief: Remove Vector.
- Parameters:
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: pk Input parameter. Return value. pk Input parameter. Return value. Calls: isActive(), Status::Error(), isTimedOut(), pk_str(), makeNamespacedKey(), checkSerializableWriteConflict(), empty(), get().

#### `void rollback()`
- Source: `include/transaction/transaction_manager.h`:216
- Brief: Rollback.
- Parameters: none
- Details: Calls: compare_exchange_strong(), THEMIS_WARN(), captureDuration(), THEMIS_DEBUG(), stepCount(), isActive(), compensate(), releasePredicateLocks().

#### `Status rollbackToSavePoint()`
- Source: `include/transaction/transaction_manager.h`:278
- Brief: Rollback To Save Point.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: load(), Status::Error(), isActive(), Status::OK().

#### `Status rollbackToSavepoint(std::string_view name)`
- Source: `include/transaction/transaction_manager.h`:299
- Brief: Rollback To Savepoint.
- Parameters:
  - `name` (std::string_view): Input parameter.
- Return: Return value.
- Details: name Input parameter. Return value. name Input parameter. Return value. Calls: load(), Status::Error(), isActive(), sname(), std::find_if(), begin(), end(), popSavePoint().

#### `Status setReadOnly(bool read_only=true)`
- Source: `include/transaction/transaction_manager.h`:342
- Brief: n/a
- Parameters:
  - `read_only` (bool): n/a

#### `Status setSavePoint()`
- Source: `include/transaction/transaction_manager.h`:272
- Brief: Set Save Point.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: load(), Status::Error(), isActive(), Status::OK().

#### `void setTimeout(std::chrono::milliseconds timeout)`
- Source: `include/transaction/transaction_manager.h`:139
- Brief: Set Timeout.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Details: timeout Input parameter. Calls: count(), store().

#### `Status trackPredicateRead(const std::string &start_key, const std::string &end_key)`
- Source: `include/transaction/transaction_manager.h`:257
- Brief: Track Predicate Read.
- Parameters:
  - `start_key` (const std::string &): Input parameter.
  - `end_key` (const std::string &): Input parameter.
- Return: Return value.
- Details: start_key Input parameter. end_key Input parameter. Return value.

#### `void trackWrite(std::string key, std::string operation)`
- Source: `include/transaction/transaction_manager.h`:390
- Brief: Track Write.
- Parameters:
  - `key` (std::string): Input parameter.
  - `operation` (std::string): Input parameter.
- Details: ── Transaction Explain ─────────────────────────────────────────────────────── key Input parameter. operation Input parameter. key Input parameter. operation Input parameter. Calls: push_back(), std::move().

#### `Status updateVector(const BaseEntity &entity, std::string_view vectorField="embedding")`
- Source: `include/transaction/transaction_manager.h`:199
- Brief: Update Vector.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `vectorField` (std::string_view): Input parameter.
- Return: Return value.
- Details: entity Input parameter. vectorField Input parameter. Return value. Calls: isActive(), Status::Error(), isTimedOut(), getPrimaryKey(), makeNamespacedKey(), checkSerializableWriteConflict(), empty(), get().

#### `~Transaction()`
- Source: `include/transaction/transaction_manager.h`:112
- Brief: n/a
- Parameters: none

### themis::test

#### `TEST(TransactionSAGAPhase2OrchestratorHardening, CircuitBreakerBlocksRetryStormAfterThreshold)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2OrchestratorHardening): n/a
  - `<unnamed>` (CircuitBreakerBlocksRetryStormAfterThreshold): n/a

#### `TEST(TransactionSAGAPhase2OrchestratorHardening, HalfOpenAllowsOnlySingleConcurrentProbe)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:410
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2OrchestratorHardening): n/a
  - `<unnamed>` (HalfOpenAllowsOnlySingleConcurrentProbe): n/a

#### `TEST_F(TransactionChaosTest, CascadingRollbackScenario)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:352
- Brief: Test: Cascading rollback scenario.
- Parameters:
  - `<unnamed>` (TransactionChaosTest): n/a
  - `<unnamed>` (CascadingRollbackScenario): n/a
- Details: Multiple transactions fail sequentially, causing cascade of rollbacks.

#### `TEST_F(TransactionChaosTest, ConcurrentPrepareAndCommit)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:434
- Brief: Test: Concurrent prepare and commit operations.
- Parameters:
  - `<unnamed>` (TransactionChaosTest): n/a
  - `<unnamed>` (ConcurrentPrepareAndCommit): n/a
- Details: Multiple threads executing prepare and commit concurrently. Verifies no race conditions or deadlocks.

#### `TEST_F(TransactionChaosTest, CoordinatorCrashRecovery)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:239
- Brief: Test: Coordinator crash mid-commit recovery.
- Parameters:
  - `<unnamed>` (TransactionChaosTest): n/a
  - `<unnamed>` (CoordinatorCrashRecovery): n/a
- Details: Injects crash before FSYNC during commit phase. Verifies transaction is either fully committed or fully rolled back.

#### `TEST_F(TransactionChaosTest, DeadlockDetectionAndBreak)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:293
- Brief: Test: Deadlock detection and breaking.
- Parameters:
  - `<unnamed>` (TransactionChaosTest): n/a
  - `<unnamed>` (DeadlockDetectionAndBreak): n/a
- Details: Simulates circular wait condition and verifies deadlock detector.

#### `TEST_F(TransactionChaosTest, LongRunningTransactionTimeout)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:325
- Brief: Test: Long-running transaction timeout.
- Parameters:
  - `<unnamed>` (TransactionChaosTest): n/a
  - `<unnamed>` (LongRunningTransactionTimeout): n/a
- Details: Transaction that exceeds timeout limit. Verifies system triggers timeout and cleans up resources.

#### `TEST_F(TransactionChaosTest, PreCommitCrashRecovery)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:394
- Brief: Test: Pre-commit crash recovery.
- Parameters:
  - `<unnamed>` (TransactionChaosTest): n/a
  - `<unnamed>` (PreCommitCrashRecovery): n/a
- Details: Crash between prepare and commit phases. Verifies recovery mechanism restores consistent state.

#### `TEST_F(TransactionChaosTest, TimeoutRecoveryCorrectness)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:495
- Brief: Test: Timeout recovery correctness.
- Parameters:
  - `<unnamed>` (TransactionChaosTest): n/a
  - `<unnamed>` (TimeoutRecoveryCorrectness): n/a
- Details: Verify proper cleanup when transaction times out.

#### `TEST_F(TransactionChaosTest, WALRecoveryAfterCrash)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:468
- Brief: Test: WAL recovery after crash.
- Parameters:
  - `<unnamed>` (TransactionChaosTest): n/a
  - `<unnamed>` (WALRecoveryAfterCrash): n/a
- Details: Simulates WAL-based recovery from crash.

#### `TEST_F(TransactionFaultInjectionPhase3Test, CascadingFailureRecovery_MultiNodeRecovery)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:529
- Brief: Validate recovery of cascading failures across sequential node failures. @acceptance AC-13: Recovery from Cascading Failures.
- Parameters:
  - `<unnamed>` (TransactionFaultInjectionPhase3Test): n/a
  - `<unnamed>` (CascadingFailureRecovery_MultiNodeRecovery): n/a
- Details: TestCascadingFailureRecovery_MultiNodeRecovery

#### `TEST_F(TransactionFaultInjectionPhase3Test, CascadingFailureRecovery_ThreeLevel)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:498
- Brief: Accumulate cascading failures and recover via recoverInDoubt(). @acceptance AC-13: Recovery from Cascading Failures.
- Parameters:
  - `<unnamed>` (TransactionFaultInjectionPhase3Test): n/a
  - `<unnamed>` (CascadingFailureRecovery_ThreeLevel): n/a
- Details: TestCascadingFailureRecovery_ThreeLevel

#### `TEST_F(TransactionFaultInjectionPhase3Test, ChaosEngineering_ByzantineBehavior)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:473
- Brief: Simulate Byzantine failures (conflicting/unexpected prepare results). @acceptance AC-12: Chaos Engineering Validation.
- Parameters:
  - `<unnamed>` (TransactionFaultInjectionPhase3Test): n/a
  - `<unnamed>` (ChaosEngineering_ByzantineBehavior): n/a
- Details: TestChaosEngineering_ByzantineBehavior

#### `TEST_F(TransactionFaultInjectionPhase3Test, ChaosEngineering_NetworkPartitions)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:446
- Brief: Simulate network partitions (prepare and commit timeouts). @acceptance AC-12: Chaos Engineering Validation.
- Parameters:
  - `<unnamed>` (TransactionFaultInjectionPhase3Test): n/a
  - `<unnamed>` (ChaosEngineering_NetworkPartitions): n/a
- Details: TestChaosEngineering_NetworkPartitions

#### `TEST_F(TransactionFaultInjectionPhase3Test, ChaosEngineering_SimultaneousParticipantCrashes)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:421
- Brief: Inject simultaneous participant abort (crash) on every prepare. @acceptance AC-12: Chaos Engineering Validation.
- Parameters:
  - `<unnamed>` (TransactionFaultInjectionPhase3Test): n/a
  - `<unnamed>` (ChaosEngineering_SimultaneousParticipantCrashes): n/a
- Details: TestChaosEngineering_SimultaneousParticipantCrashes

#### `TEST_F(TransactionFaultInjectionPhase3Test, FaultInjection_CommitPhaseTimeout)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:322
- Brief: Inject faults in commit phase after successful prepare. @acceptance AC-11: Extended Fault Injection Coverage.
- Parameters:
  - `<unnamed>` (TransactionFaultInjectionPhase3Test): n/a
  - `<unnamed>` (FaultInjection_CommitPhaseTimeout): n/a
- Details: TestFaultInjection_CommitPhaseTimeout

#### `TEST_F(TransactionFaultInjectionPhase3Test, FaultInjection_CrossShardCoordination)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:348
- Brief: Cascade failures across multiple logical shards (coordinator instances). @acceptance AC-11: Extended Fault Injection Coverage.
- Parameters:
  - `<unnamed>` (TransactionFaultInjectionPhase3Test): n/a
  - `<unnamed>` (FaultInjection_CrossShardCoordination): n/a
- Details: TestFaultInjection_CrossShardCoordination

#### `TEST_F(TransactionFaultInjectionPhase3Test, FaultInjection_ParticipantNodeRecovery)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:393
- Brief: Simulate crash/recovery cycles via recoverInDoubt(). @acceptance AC-11: Extended Fault Injection Coverage.
- Parameters:
  - `<unnamed>` (TransactionFaultInjectionPhase3Test): n/a
  - `<unnamed>` (FaultInjection_ParticipantNodeRecovery): n/a
- Details: TestFaultInjection_ParticipantNodeRecovery

#### `TEST_F(TransactionFaultInjectionPhase3Test, FaultInjection_PreparePhaseTimeout)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:297
- Brief: Inject TIMEOUT errors during prepare phase. @acceptance AC-11: Extended Fault Injection Coverage.
- Parameters:
  - `<unnamed>` (TransactionFaultInjectionPhase3Test): n/a
  - `<unnamed>` (FaultInjection_PreparePhaseTimeout): n/a
- Details: TestFaultInjection_PreparePhaseTimeout

#### `TEST_F(TransactionFaultInjectionPhase3Test, StressTest_HighConcurrencyWithFaultInjection)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:558
- Brief: Stress test with high concurrency and continuous fault injection. @acceptance AC-11, AC-12: Fault injection under load.
- Parameters:
  - `<unnamed>` (TransactionFaultInjectionPhase3Test): n/a
  - `<unnamed>` (StressTest_HighConcurrencyWithFaultInjection): n/a
- Details: TestStressTest_HighConcurrencyWithFaultInjection

#### `TEST_F(TransactionFaultInjectionPhase3Test, StressTest_LongRunningDegradedConditions)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:601
- Brief: Long-running test with sustained cascading fault injection. @acceptance AC-11, AC-12: Chaos validation.
- Parameters:
  - `<unnamed>` (TransactionFaultInjectionPhase3Test): n/a
  - `<unnamed>` (StressTest_LongRunningDegradedConditions): n/a
- Details: TestStressTest_LongRunningDegradedConditions

#### `TEST_F(TransactionSAGAPhase2Test, CompensationIdempotency_MultiStepChain)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:128
- Brief: Validates compensation order and idempotency in multi-step chain @acceptance AC-8: Compensation Idempotency.
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2Test): n/a
  - `<unnamed>` (CompensationIdempotency_MultiStepChain): n/a
- Details: TestCompensationIdempotency_MultiStepChain

#### `TEST_F(TransactionSAGAPhase2Test, CompensationIdempotency_RetryStorm)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:169
- Brief: Validates compensation handles retry storms (repeated compensation) @acceptance AC-8: Compensation Idempotency, AC-10: Retry Storm Handling.
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2Test): n/a
  - `<unnamed>` (CompensationIdempotency_RetryStorm): n/a
- Details: TestCompensationIdempotency_RetryStorm

#### `TEST_F(TransactionSAGAPhase2Test, CompensationIdempotency_SingleStepCompensation)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:100
- Brief: Validates compensation is idempotent for single step @acceptance AC-8: Compensation Idempotency.
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2Test): n/a
  - `<unnamed>` (CompensationIdempotency_SingleStepCompensation): n/a
- Details: TestCompensationIdempotency_SingleStepCompensation

#### `TEST_F(TransactionSAGAPhase2Test, RecoveryPath_ManualIntervention)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:476
- Brief: Validates manual intervention path for stuck compensation @acceptance AC-10: Recovery and Retry Storm Handling.
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2Test): n/a
  - `<unnamed>` (RecoveryPath_ManualIntervention): n/a
- Details: TestRecoveryPath_ManualIntervention

#### `TEST_F(TransactionSAGAPhase2Test, RetryStormHandling_BoundedRetries)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:321
- Brief: Validates retry storm is bounded (max retries enforced) @acceptance AC-10: Recovery and Retry Storm Handling.
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2Test): n/a
  - `<unnamed>` (RetryStormHandling_BoundedRetries): n/a
- Details: TestRetryStormHandling_BoundedRetries

#### `TEST_F(TransactionSAGAPhase2Test, RetryStormHandling_CircuitBreaker)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:350
- Brief: Validates circuit breaker pattern prevents retry storms @acceptance AC-10: Recovery and Retry Storm Handling.
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2Test): n/a
  - `<unnamed>` (RetryStormHandling_CircuitBreaker): n/a
- Details: TestRetryStormHandling_CircuitBreaker

#### `TEST_F(TransactionSAGAPhase2Test, SAGAOrchestration_CascadingFailure)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:282
- Brief: Validates SAGA handles cascading failures (step fails, affects next) @acceptance AC-9: SAGA Orchestration Under Failures.
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2Test): n/a
  - `<unnamed>` (SAGAOrchestration_CascadingFailure): n/a
- Details: TestSAGAOrchestration_CascadingFailure

#### `TEST_F(TransactionSAGAPhase2Test, SAGAOrchestration_NetworkDegradation)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:243
- Brief: Validates SAGA handles network degradation (slow responses) @acceptance AC-9: SAGA Orchestration Under Failures.
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2Test): n/a
  - `<unnamed>` (SAGAOrchestration_NetworkDegradation): n/a
- Details: TestSAGAOrchestration_NetworkDegradation

#### `TEST_F(TransactionSAGAPhase2Test, SAGAOrchestration_PartialRemoteFailure)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:200
- Brief: Validates SAGA behavior when remote service fails mid-execution @acceptance AC-9: SAGA Orchestration Under Failures.
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2Test): n/a
  - `<unnamed>` (SAGAOrchestration_PartialRemoteFailure): n/a
- Details: TestSAGAOrchestration_PartialRemoteFailure

#### `TEST_F(TransactionSAGAPhase2Test, StressTest_ConcurrentSAGAFlows)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:510
- Brief: Stress test with multiple concurrent SAGA flows @acceptance AC-8, AC-9: SAGA orchestration and compensation.
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2Test): n/a
  - `<unnamed>` (StressTest_ConcurrentSAGAFlows): n/a
- Details: TestStressTest_ConcurrentSAGAFlows

#### `TEST_F(TransactionSAGAPhase2Test, StressTest_SAGAWithIntermittentFailures)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:565
- Brief: Stress test SAGA with random intermittent failures @acceptance AC-9, AC-10: Failure handling and recovery.
- Parameters:
  - `<unnamed>` (TransactionSAGAPhase2Test): n/a
  - `<unnamed>` (StressTest_SAGAWithIntermittentFailures): n/a
- Details: TestStressTest_SAGAWithIntermittentFailures

#### `TEST_F(TransactionStressTest, ConcurrentShortTransactions10k)`
- Source: `tests/transaction/test_transaction_stress.cpp`:114
- Brief: Stress: 10k concurrent short transactions.
- Parameters:
  - `<unnamed>` (TransactionStressTest): n/a
  - `<unnamed>` (ConcurrentShortTransactions10k): n/a
- Details: 10 threads × 1000 transactions each = 10k total Verifies all commit successfully and cleanup occurs.

#### `TEST_F(TransactionStressTest, HighContentionLockStress)`
- Source: `tests/transaction/test_transaction_stress.cpp`:160
- Brief: Stress: High contention lock stress.
- Parameters:
  - `<unnamed>` (TransactionStressTest): n/a
  - `<unnamed>` (HighContentionLockStress): n/a
- Details: Multiple threads contending for same locks. Verifies no deadlocks and reasonable performance.

#### `TEST_F(TransactionStressTest, TransactionCleanupVerification)`
- Source: `tests/transaction/test_transaction_stress.cpp`:204
- Brief: Stress: Transaction cleanup verification.
- Parameters:
  - `<unnamed>` (TransactionStressTest): n/a
  - `<unnamed>` (TransactionCleanupVerification): n/a
- Details: Verify all transactions are cleaned up after execution.

#### `bool runHappyPath(ITransactionCoordinator &coord, const std::string &txn_id)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:256
- Brief: Begin + prepare + commit (or abort on prepare failure). Returns true on full commit.
- Parameters:
  - `coord` (ITransactionCoordinator &): n/a
  - `txn_id` (const std::string &): n/a

### themis::test::FaultInjectableCoordinator

#### `FaultInjectableCoordinator(FaultPattern pattern=FaultPattern::NONE, unsigned rng_seed=42)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:67
- Brief: n/a
- Parameters:
  - `pattern` (FaultPattern): n/a
  - `rng_seed` (unsigned): n/a

#### `TxnCoordinatorResult abort(std::string_view txn_id) override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:195
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `TxnCoordinatorResult begin(std::string_view txn_id, const TxnCoordinatorOptions &={}) override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:101
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a
  - `<unnamed>` (const TxnCoordinatorOptions &): n/a

#### `CoordinatorCapabilities capabilities() const noexcept override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:87
- Brief: n/a
- Parameters: none

#### `TxnCoordinatorResult commit(std::string_view txn_id) override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:168
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `std::vector< InDoubtTxnDescriptor > getInDoubtTransactions() const override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:219
- Brief: n/a
- Parameters: none

#### `TxnLifecycleState getState(std::string_view txn_id) const override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:213
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `TxnCoordinatorResult prepare(std::string_view txn_id) override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:120
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `std::string_view protocolName() const noexcept override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:83
- Brief: n/a
- Parameters: none

#### `CommitProtocol protocolType() const noexcept override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:79
- Brief: n/a
- Parameters: none

#### `size_t recoverInDoubt() override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:232
- Brief: n/a
- Parameters: none

#### `void setFaultPattern(FaultPattern p)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:72
- Brief: n/a
- Parameters:
  - `p` (FaultPattern): n/a

### themis::test::MockSAGAStep

#### `MockSAGAStep(const std::string &name, int node_id)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:48
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `node_id` (int): n/a

#### `int getCompensationCount() const`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:56
- Brief: n/a
- Parameters: none

#### `int getExecutionCount() const`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:55
- Brief: n/a
- Parameters: none

#### `const std::string & getName() const`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:52
- Brief: n/a
- Parameters: none

#### `int getNodeId() const`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:53
- Brief: n/a
- Parameters: none

#### `StepState getState() const`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:54
- Brief: n/a
- Parameters: none

#### `void incrementCompensationCount()`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:60
- Brief: n/a
- Parameters: none

#### `void incrementExecutionCount()`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:59
- Brief: n/a
- Parameters: none

#### `void setState(StepState s)`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:58
- Brief: n/a
- Parameters:
  - `s` (StepState): n/a

### themis::test::MockTransactionCoordinator

#### `MockTransactionCoordinator()`
- Source: `tests/transaction/test_transaction_chaos.cpp`:67
- Brief: n/a
- Parameters: none

#### `bool abortTransaction(int txn_id)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:126
- Brief: n/a
- Parameters:
  - `txn_id` (int): n/a

#### `int beginTransaction()`
- Source: `tests/transaction/test_transaction_chaos.cpp`:69
- Brief: n/a
- Parameters: none

#### `void clearDeadlocks()`
- Source: `tests/transaction/test_transaction_chaos.cpp`:200
- Brief: n/a
- Parameters: none

#### `bool commitTransaction(int txn_id)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:99
- Brief: n/a
- Parameters:
  - `txn_id` (int): n/a

#### `int getDeadlockCount() const`
- Source: `tests/transaction/test_transaction_chaos.cpp`:195
- Brief: n/a
- Parameters: none

#### `int getPendingTransactionCount() const`
- Source: `tests/transaction/test_transaction_chaos.cpp`:163
- Brief: n/a
- Parameters: none

#### `TxnState getState(int txn_id) const`
- Source: `tests/transaction/test_transaction_chaos.cpp`:154
- Brief: n/a
- Parameters:
  - `txn_id` (int): n/a

#### `bool hasDeadlock(int txn_id) const`
- Source: `tests/transaction/test_transaction_chaos.cpp`:185
- Brief: n/a
- Parameters:
  - `txn_id` (int): n/a

#### `bool prepareTransaction(int txn_id)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:77
- Brief: n/a
- Parameters:
  - `txn_id` (int): n/a

#### `void setCrashInjector(CrashInjector *injector)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:174
- Brief: n/a
- Parameters:
  - `injector` (CrashInjector *): n/a

#### `bool setKey(int txn_id, const std::string &key, const std::string &value)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:143
- Brief: n/a
- Parameters:
  - `txn_id` (int): n/a
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

#### `void simulateDeadlock(int txn_id_a, int txn_id_b)`
- Source: `tests/transaction/test_transaction_chaos.cpp`:179
- Brief: n/a
- Parameters:
  - `txn_id_a` (int): n/a
  - `txn_id_b` (int): n/a

### themis::test::StressTransactionManager

#### `StressTransactionManager()`
- Source: `tests/transaction/test_transaction_stress.cpp`:43
- Brief: n/a
- Parameters: none

#### `bool abortTransaction(int txn_id)`
- Source: `tests/transaction/test_transaction_stress.cpp`:64
- Brief: n/a
- Parameters:
  - `txn_id` (int): n/a

#### `int beginTransaction()`
- Source: `tests/transaction/test_transaction_stress.cpp`:45
- Brief: n/a
- Parameters: none

#### `bool commitTransaction(int txn_id)`
- Source: `tests/transaction/test_transaction_stress.cpp`:52
- Brief: n/a
- Parameters:
  - `txn_id` (int): n/a

#### `int getAbortedCount() const`
- Source: `tests/transaction/test_transaction_stress.cpp`:81
- Brief: n/a
- Parameters: none

#### `int getCommittedCount() const`
- Source: `tests/transaction/test_transaction_stress.cpp`:80
- Brief: n/a
- Parameters: none

#### `int getPendingTransactionCount() const`
- Source: `tests/transaction/test_transaction_stress.cpp`:75
- Brief: n/a
- Parameters: none

### themis::test::TransactionChaosTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_chaos.cpp`:220
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_chaos.cpp`:224
- Brief: n/a
- Parameters: none

### themis::test::TransactionFaultInjectionPhase3Test

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:273
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:277
- Brief: n/a
- Parameters: none

#### `std::string txn(const char *prefix, int i)`
- Source: `tests/transaction/test_transaction_fault_injection_phase3.cpp`:283
- Brief: n/a
- Parameters:
  - `prefix` (const char *): n/a
  - `i` (int): n/a

### themis::test::TransactionSAGAPhase2Test

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:73
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_saga_compensation_phase2.cpp`:84
- Brief: n/a
- Parameters: none

### themis::test::TransactionStressTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_stress.cpp`:97
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_stress.cpp`:99
- Brief: n/a
- Parameters: none

### themis::testing::MockConnection

#### `MockConnection()`
- Source: `tests/transaction/connection_leak_tests.cpp`:31
- Brief: n/a
- Parameters: none

#### `void close() override`
- Source: `tests/transaction/connection_leak_tests.cpp`:54
- Brief: n/a
- Parameters: none

#### `std::string getError() const override`
- Source: `tests/transaction/connection_leak_tests.cpp`:50
- Brief: n/a
- Parameters: none

#### `bool isValid() const override`
- Source: `tests/transaction/connection_leak_tests.cpp`:42
- Brief: n/a
- Parameters: none

#### `bool ping() override`
- Source: `tests/transaction/connection_leak_tests.cpp`:46
- Brief: n/a
- Parameters: none

#### `void simulateError(std::string error)`
- Source: `tests/transaction/connection_leak_tests.cpp`:60
- Brief: n/a
- Parameters:
  - `error` (std::string): n/a

#### `bool wasClosedProperly() const`
- Source: `tests/transaction/connection_leak_tests.cpp`:65
- Brief: n/a
- Parameters: none

#### `~MockConnection() override`
- Source: `tests/transaction/connection_leak_tests.cpp`:35
- Brief: n/a
- Parameters: none

### themis::testing::MockDatabaseConnectionManager

#### `MockDatabaseConnectionManager()`
- Source: `tests/transaction/connection_leak_tests.cpp`:79
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< Connection > createConnection() override`
- Source: `tests/transaction/connection_leak_tests.cpp`:82
- Brief: n/a
- Parameters: none

### themis::transaction

#### `Arg(10) -> Arg(50) ->Arg(100) ->Arg(500)`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `BENCHMARK_DEFINE_F(BranchManagerBenchmark, ValidateBranchName)(benchmark`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManagerBenchmark): n/a
  - `<unnamed>` (ValidateBranchName): n/a

#### `BENCHMARK_F(BranchManagerBenchmark, BranchExists)(benchmark`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManagerBenchmark): n/a
  - `<unnamed>` (BranchExists): n/a

#### `BENCHMARK_F(BranchManagerBenchmark, CreateBranch)(benchmark`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManagerBenchmark): n/a
  - `<unnamed>` (CreateBranch): n/a

#### `BENCHMARK_F(BranchManagerBenchmark, DeleteBranch)(benchmark`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManagerBenchmark): n/a
  - `<unnamed>` (DeleteBranch): n/a

#### `BENCHMARK_F(BranchManagerBenchmark, GetBranch)(benchmark`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManagerBenchmark): n/a
  - `<unnamed>` (GetBranch): n/a

#### `BENCHMARK_F(BranchManagerBenchmark, GetStats)(benchmark`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManagerBenchmark): n/a
  - `<unnamed>` (GetStats): n/a

#### `BENCHMARK_F(BranchManagerBenchmark, JsonSerialization)(benchmark`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:233
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManagerBenchmark): n/a
  - `<unnamed>` (JsonSerialization): n/a

#### `BENCHMARK_F(BranchManagerBenchmark, ListBranches)(benchmark`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManagerBenchmark): n/a
  - `<unnamed>` (ListBranches): n/a

#### `BENCHMARK_F(BranchManagerBenchmark, MergeBranchesFastForward)(benchmark`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManagerBenchmark): n/a
  - `<unnamed>` (MergeBranchesFastForward): n/a

#### `BENCHMARK_F(BranchManagerBenchmark, SwitchBranch)(benchmark`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManagerBenchmark): n/a
  - `<unnamed>` (SwitchBranch): n/a

#### `BENCHMARK_REGISTER_F(BranchManagerBenchmark, ValidateBranchName)`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManagerBenchmark): n/a
  - `<unnamed>` (ValidateBranchName): n/a

#### `Result< SAGAOrchestrator * > bindSagaOrchestratorFromPlugin(plugins::PluginManager &plugin_manager, const std::string &plugin_name="saga_orchestrator")`
- Source: `src/transaction/saga_plugin_bridge.cpp`:23
- Brief: Load (if necessary) and bind the SAGA orchestrator from a plugin.
- Parameters:
  - `plugin_manager` (plugins::PluginManager &): Input/output parameter.
  - `plugin_name` (const std::string &): Name of the plugin.
- Return: Result with typed orchestrator pointer or an error when loading/ binding fails.
- Details: Bind Saga Orchestrator From Plugin. This helper wraps PluginManager interaction so callers can retrieve a typed SAGAOrchestrator* from a dynamically loaded plugin DLL. plugin_manager Plugin manager used for discovery/load. plugin_name Plugin manifest name (default: "saga_orchestrator"). Result with typed orchestrator pointer or an error when loading/ binding fails. plugin_manager Input/output parameter. plugin_name Name of the plugin. Return value. Calls: isPluginLoaded(), loadPlugin(), tl::unexpected(), error(), getPlugin(), value(), getInstance().

#### `bool deliverPhase2WithRetry(Fn &&deliver_fn, const char *bridge_name, const std::string &node_id, const std::string &txn_id, const std::string &coordinator_id, bool do_commit)`
- Source: `src/transaction/distributed_transaction_manager.cpp`:209
- Brief: Deliver Phase2 With Retry.
- Parameters:
  - `deliver_fn` (Fn &&): Input parameter.
  - `bridge_name` (const char *): Name of the bridge.
  - `node_id` (const std::string &): Identifier of the node.
  - `txn_id` (const std::string &): Identifier of the txn.
  - `coordinator_id` (const std::string &): Identifier of the coordinator.
  - `do_commit` (bool): Input parameter.
- Return: True when the operation succeeds.
- Details: deliver_fn Input parameter. bridge_name Name of the bridge. node_id Identifier of the node. txn_id Identifier of the txn. coordinator_id Identifier of the coordinator. do_commit Input parameter. True when the operation succeeds.

#### `bool executeWithConnection(storage::DatabaseConnectionManager &manager, Func &&operation, std::string_view operation_name="operation") noexcept`
- Source: `include/transaction/connection_resource_guard.h`:213
- Brief: n/a
- Parameters:
  - `manager` (storage::DatabaseConnectionManager &): n/a
  - `operation` (Func &&): n/a
  - `operation_name` (std::string_view): n/a

#### `DistributedTransactionManager::StaticLivenessCheckFn getLivenessCheckFn()`
- Source: `src/transaction/distributed_transaction_manager.cpp`:193
- Brief: Get Liveness Check Fn.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: lock().

#### `DistributedTransactionManager::RpcPhase1Fn getRpcPhase1Fn()`
- Source: `src/transaction/distributed_transaction_manager.cpp`:155
- Brief: Get Rpc Phase1 Fn.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: lock().

#### `DistributedTransactionManager::RpcPhase2Fn getRpcPhase2Fn()`
- Source: `src/transaction/distributed_transaction_manager.cpp`:111
- Brief: Get Rpc Phase2 Fn.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: lock().

### themis::transaction::BranchManager

#### `BranchManager(BranchManager &&) noexcept=default`
- Source: `include/transaction/branch_manager.h`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManager &&): n/a

#### `BranchManager(RocksDBWrapper &db, Changefeed &changefeed, SnapshotManager &snapshot_manager, MergeEngine *merge_engine=nullptr)`
- Source: `include/transaction/branch_manager.h`:95
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper &): n/a
  - `changefeed` (Changefeed &): n/a
  - `snapshot_manager` (SnapshotManager &): n/a
  - `merge_engine` (MergeEngine *): n/a

#### `BranchManager(const BranchManager &)=delete`
- Source: `include/transaction/branch_manager.h`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BranchManager &): n/a

#### `void appendHistory(const BranchHistoryEntry &entry)`
- Source: `include/transaction/branch_manager.h`:374
- Brief: Append History.
- Parameters:
  - `entry` (const BranchHistoryEntry &): Input parameter.
- Details: entry Input parameter. entry Input parameter. Calls: fetch_add(), std::string(), std::to_string(), serializeHistory(), put().

#### `bool branchExists(const std::string &branch_name) const`
- Source: `include/transaction/branch_manager.h`:210
- Brief: Branch Exists.
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
- Return: True when the operation succeeds.
- Details: branch_name Name of the branch. True when the operation succeeds.

#### `std::optional< Branch > createBranch(const std::string &branch_name, const std::string &parent_branch, const std::string &description, const std::string &created_by, const CreateBranchOptions &options)`
- Source: `include/transaction/branch_manager.h`:132
- Brief: Create Branch.
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
  - `parent_branch` (const std::string &): Input parameter.
  - `description` (const std::string &): Input parameter.
  - `created_by` (const std::string &): Input parameter.
  - `options` (const CreateBranchOptions &): Input parameter.
- Return: Return value.
- Details: branch_name Name of the branch. parent_branch Input parameter. description Input parameter. created_by Input parameter. options Input parameter. Return value. branch_name Name of the branch. parent_branch Input parameter. description Input parameter. created_by Input parameter. options Input parameter. Return value. Calls: lock(), isValidBranchName(), branchExists(), empty(), resolveSequence(), has_value(), getLatestSequence(), std::chrono::system_clock::now().

#### `std::optional< Branch > createBranch(const std::string &branch_name, const std::string &parent_branch, const std::string &description, const std::string &created_by="system")`
- Source: `include/transaction/branch_manager.h`:116
- Brief: Create branch (4-arg overload without options).
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
  - `parent_branch` (const std::string &): Input parameter.
  - `description` (const std::string &): Input parameter.
  - `created_by` (const std::string &): Input parameter.
- Return: Return value.
- Details: branch_name Name of the branch. parent_branch Input parameter. description Input parameter. created_by Input parameter. Return value. Implements createBranch without additional internal calls.

#### `bool deleteBranch(const std::string &branch_name, bool force=false)`
- Source: `include/transaction/branch_manager.h`:166
- Brief: Delete Branch.
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
  - `force` (bool): Input parameter.
- Return: True when the operation succeeds.
- Details: branch_name Name of the branch. force Input parameter. True when the operation succeeds. Calls: lock(), branchExists(), isBranchMerged(), del(), makeKey().

#### `std::optional< Branch > deserialize(const std::vector< uint8_t > &data) const`
- Source: `include/transaction/branch_manager.h`:334
- Brief: Deserialize.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value.

#### `std::optional< BranchHistoryEntry > deserializeHistory(const std::vector< uint8_t > &data) const`
- Source: `include/transaction/branch_manager.h`:388
- Brief: Deserialize History.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value.

#### `std::string extractBranchName(const std::string &key) const`
- Source: `include/transaction/branch_manager.h`:320
- Brief: Extract Branch Name.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::string getActiveBranch() const`
- Source: `include/transaction/branch_manager.h`:164
- Brief: Get Active Branch.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< Branch > getBranch(const std::string &branch_name) const`
- Source: `include/transaction/branch_manager.h`:145
- Brief: Get Branch.
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
- Return: Return value.
- Details: branch_name Name of the branch. Return value.

#### `std::vector< BranchHistoryEntry > getBranchHistory(const std::string &branch_name, size_t limit=0) const`
- Source: `include/transaction/branch_manager.h`:268
- Brief: n/a
- Parameters:
  - `branch_name` (const std::string &): n/a
  - `limit` (size_t): n/a

#### `std::string getDefaultBranch()`
- Source: `include/transaction/branch_manager.h`:243
- Brief: Get Default Branch.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Implements getDefaultBranch without additional internal calls.

#### `std::optional< uint64_t > getSequenceForBranch(const std::string &branch_name) const`
- Source: `include/transaction/branch_manager.h`:223
- Brief: Get Sequence For Branch.
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
- Return: Return value.
- Details: branch_name Name of the branch. Return value.

#### `BranchStats getStats() const`
- Source: `include/transaction/branch_manager.h`:216
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< int64_t > getTimestampForBranch(const std::string &branch_name) const`
- Source: `include/transaction/branch_manager.h`:230
- Brief: Get Timestamp For Branch.
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
- Return: Return value.
- Details: branch_name Name of the branch. Return value.

#### `bool isBranchMerged(const std::string &branch_name, const std::string &target_branch) const`
- Source: `include/transaction/branch_manager.h`:361
- Brief: Is Branch Merged.
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
  - `target_branch` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: branch_name Name of the branch. target_branch Input parameter. True when the operation succeeds.

#### `bool isValidBranchName(const std::string &branch_name)`
- Source: `include/transaction/branch_manager.h`:237
- Brief: Is Valid Branch Name.
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
- Return: True when the operation succeeds.
- Details: branch_name Name of the branch. True when the operation succeeds. branch_name Name of the branch. True when the operation succeeds. Calls: empty(), length(), pattern(), std::regex_match().

#### `std::vector< Branch > listBranches(size_t limit=0, const std::string &sort_by="name", bool ascending=true) const`
- Source: `include/transaction/branch_manager.h`:147
- Brief: n/a
- Parameters:
  - `limit` (size_t): n/a
  - `sort_by` (const std::string &): n/a
  - `ascending` (bool): n/a

#### `void loadActiveBranch()`
- Source: `include/transaction/branch_manager.h`:339
- Brief: Load Active Branch.
- Parameters: none
- Details: Calls: get(), has_value(), branch_name(), value(), begin(), end().

#### `std::string makeKey(const std::string &branch_name) const`
- Source: `include/transaction/branch_manager.h`:313
- Brief: Make Key.
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
- Return: Return value.
- Details: branch_name Name of the branch. Return value.

#### `MergeResult mergeBranches(const std::string &source_branch, const std::string &target_branch)`
- Source: `include/transaction/branch_manager.h`:174
- Brief: Merge Branches.
- Parameters:
  - `source_branch` (const std::string &): Input parameter.
  - `target_branch` (const std::string &): Input parameter.
- Return: Return value.
- Details: Merge branches (2-arg overload without options). source_branch Input parameter. target_branch Input parameter. Return value. source_branch Input parameter. target_branch Input parameter. Return value. Implements mergeBranches without additional internal calls.

#### `MergeResult mergeBranches(const std::string &source_branch, const std::string &target_branch, const MergeOptions &options)`
- Source: `include/transaction/branch_manager.h`:186
- Brief: Merge Branches.
- Parameters:
  - `source_branch` (const std::string &): Input parameter.
  - `target_branch` (const std::string &): Input parameter.
  - `options` (const MergeOptions &): Input parameter.
- Return: Return value.
- Details: source_branch Input parameter. target_branch Input parameter. options Input parameter. Return value. source_branch Input parameter. target_branch Input parameter. options Input parameter. Return value. Calls: getBranch(), has_value(), std::min(), recordMergeStatus(), merge(), push_back(), fmt::format(), what().

#### `BranchManager & operator=(BranchManager &&) noexcept=default`
- Source: `include/transaction/branch_manager.h`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (BranchManager &&): n/a

#### `BranchManager & operator=(const BranchManager &)=delete`
- Source: `include/transaction/branch_manager.h`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BranchManager &): n/a

#### `MergeEngine::MergeResult previewBranchMerge(const std::string &source_branch, const std::string &target_branch, const std::string &base_branch="") const`
- Source: `include/transaction/branch_manager.h`:192
- Brief: n/a
- Parameters:
  - `source_branch` (const std::string &): n/a
  - `target_branch` (const std::string &): n/a
  - `base_branch` (const std::string &): n/a

#### `size_t pruneMergedBranches()`
- Source: `include/transaction/branch_manager.h`:289
- Brief: Prune Merged Branches.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: lk(), std::chrono::system_clock::now(), time_since_epoch(), count(), newSafeIterator(), value(), Seek(), Valid().

#### `void recordMergeStatus(const std::string &source_branch, const std::string &target_branch)`
- Source: `include/transaction/branch_manager.h`:368
- Brief: Record Merge Status.
- Parameters:
  - `source_branch` (const std::string &): Input parameter.
  - `target_branch` (const std::string &): Input parameter.
- Details: source_branch Input parameter. target_branch Input parameter. source_branch Input parameter. target_branch Input parameter. Calls: std::string(), put().

#### `MergeEngine::MergeResult resolveAndMergeBranches(const std::string &source_branch, const std::string &target_branch, const std::vector< MergeEngine::ConflictResolution > &resolutions, const std::string &base_branch="")`
- Source: `include/transaction/branch_manager.h`:198
- Brief: Resolve conflicts and complete a branch merge.
- Parameters:
  - `source_branch` (const std::string &): Input parameter.
  - `target_branch` (const std::string &): Input parameter.
  - `resolutions` (const std::vector< MergeEngine::ConflictResolution > &): Input parameter.
  - `base_branch` (const std::string &): Input parameter.
- Return: Return value.
- Details: source_branch Input parameter. target_branch Input parameter. resolutions Input parameter. base_branch Input parameter. Return value. Calls: getBranch(), has_value(), fmt::format(), empty(), std::min(), merge(), recordMergeStatus(), std::chrono::system_clock::now().

#### `std::optional< uint64_t > resolveSequence(const CreateBranchOptions &options) const`
- Source: `include/transaction/branch_manager.h`:353
- Brief: Resolve Sequence.
- Parameters:
  - `options` (const CreateBranchOptions &): Input parameter.
- Return: Return value.
- Details: options Input parameter. Return value.

#### `bool saveActiveBranch(const std::string &branch_name)`
- Source: `include/transaction/branch_manager.h`:346
- Brief: Save Active Branch.
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
- Return: True when the operation succeeds.
- Details: branch_name Name of the branch. True when the operation succeeds. branch_name Name of the branch. True when the operation succeeds. Calls: data(), begin(), end(), put().

#### `std::vector< uint8_t > serialize(const Branch &branch) const`
- Source: `include/transaction/branch_manager.h`:327
- Brief: Serialize.
- Parameters:
  - `branch` (const Branch &): Input parameter.
- Return: Return value.
- Details: branch Input parameter. Return value.

#### `std::vector< uint8_t > serializeHistory(const BranchHistoryEntry &entry) const`
- Source: `include/transaction/branch_manager.h`:381
- Brief: Serialize History.
- Parameters:
  - `entry` (const BranchHistoryEntry &): Input parameter.
- Return: Return value.
- Details: entry Input parameter. Return value.

#### `void setBranchGCPolicy(const BranchGCPolicy &policy)`
- Source: `include/transaction/branch_manager.h`:283
- Brief: Set Branch GCPolicy.
- Parameters:
  - `policy` (const BranchGCPolicy &): Input parameter.
- Details: - Phase 5: Branch GC - policy Input parameter. policy Input parameter. Calls: lk().

#### `void setMergeEngine(MergeEngine *merge_engine)`
- Source: `include/transaction/branch_manager.h`:114
- Brief: Set Merge Engine.
- Parameters:
  - `merge_engine` (MergeEngine *): Input/output parameter.
- Details: merge_engine Input/output parameter. merge_engine Input/output parameter. Calls: lock().

#### `bool switchBranch(const std::string &branch_name)`
- Source: `include/transaction/branch_manager.h`:158
- Brief: Switch Branch.
- Parameters:
  - `branch_name` (const std::string &): Name of the branch.
- Return: True when the operation succeeds.
- Details: branch_name Name of the branch. True when the operation succeeds. branch_name Name of the branch. True when the operation succeeds. Calls: lock(), branchExists(), saveActiveBranch(), std::chrono::system_clock::now(), time_since_epoch(), count(), getLatestSequence(), appendHistory().

#### `~BranchManager()=default`
- Source: `include/transaction/branch_manager.h`:102
- Brief: n/a
- Parameters: none

### themis::transaction::BranchManager::Branch

#### `Branch fromJson(const json &j)`
- Source: `include/transaction/branch_manager.h`:52
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: value().

#### `json toJson() const`
- Source: `include/transaction/branch_manager.h`:46
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::BranchManager::BranchHistoryEntry

#### `BranchHistoryEntry fromJson(const json &j)`
- Source: `include/transaction/branch_manager.h`:265
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value.

#### `json toJson() const`
- Source: `include/transaction/branch_manager.h`:259
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::BranchManager::BranchStats

#### `json toJson() const`
- Source: `include/transaction/branch_manager.h`:66
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::BranchManager::MergeResult

#### `json toJson() const`
- Source: `include/transaction/branch_manager.h`:92
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::BranchManagerBenchmark

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:17
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/transaction/bench_branch_manager.cpp`:62
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

### themis::transaction::CompensationLog

#### `CompensationLog(const CompensationLog &)=delete`
- Source: `include/transaction/compensation_log.h`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CompensationLog &): n/a

#### `CompensationLog(const std::string &saga_id)`
- Source: `include/transaction/compensation_log.h`:43
- Brief: Compensation Log.
- Parameters:
  - `saga_id` (const std::string &): Identifier of the saga.
- Return: Return value.
- Details: saga_id Identifier of the saga. Return value.

#### `void clear()`
- Source: `include/transaction/compensation_log.h`:96
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `std::vector< CompensationLogEntry > getEntries() const`
- Source: `include/transaction/compensation_log.h`:83
- Brief: Get Entries.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< CompensationLogEntry > getEntriesForStep(const std::string &step_name) const`
- Source: `include/transaction/compensation_log.h`:90
- Brief: Get Entries For Step.
- Parameters:
  - `step_name` (const std::string &): Name of the step.
- Return: Return value.
- Details: step_name Name of the step. Return value.

#### `bool hasSucceeded(const std::string &step_name) const`
- Source: `include/transaction/compensation_log.h`:77
- Brief: Has Succeeded.
- Parameters:
  - `step_name` (const std::string &): Name of the step.
- Return: True when the operation succeeds.
- Details: step_name Name of the step. True when the operation succeeds.

#### `CompensationLog & operator=(const CompensationLog &)=delete`
- Source: `include/transaction/compensation_log.h`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CompensationLog &): n/a

#### `uint32_t recordCompensationAttempt(const std::string &step_name)`
- Source: `include/transaction/compensation_log.h`:56
- Brief: Record Compensation Attempt.
- Parameters:
  - `step_name` (const std::string &): Name of the step.
- Return: Return value.
- Details: step_name Name of the step. Return value. step_name Name of the step. Return value. Calls: lock(), size(), std::chrono::system_clock::now(), push_back().

#### `void recordCompensationFailure(const std::string &step_name, uint32_t sequence_number, const std::string &error_detail={})`
- Source: `include/transaction/compensation_log.h`:67
- Brief: Record Compensation Failure.
- Parameters:
  - `step_name` (const std::string &): Name of the step.
  - `sequence_number` (uint32_t): Input parameter.
  - `error_detail` (const std::string &): Input parameter.
- Details: step_name Name of the step. sequence_number Input parameter. error_detail Input parameter. Calls: lock(), find(), end(), size().

#### `void recordCompensationSuccess(const std::string &step_name, uint32_t sequence_number)`
- Source: `include/transaction/compensation_log.h`:63
- Brief: Record Compensation Success.
- Parameters:
  - `step_name` (const std::string &): Name of the step.
  - `sequence_number` (uint32_t): Input parameter.
- Details: step_name Name of the step. sequence_number Input parameter. step_name Name of the step. sequence_number Input parameter. Calls: lock(), find(), end(), size().

#### `~CompensationLog()=default`
- Source: `include/transaction/compensation_log.h`:45
- Brief: n/a
- Parameters: none

### themis::transaction::ConnectionGuard

#### `ConnectionGuard(ConnectionGuard &&other) noexcept`
- Source: `include/transaction/connection_resource_guard.h`:46
- Brief: n/a
- Parameters:
  - `other` (ConnectionGuard &&): n/a

#### `ConnectionGuard(const ConnectionGuard &)=delete`
- Source: `include/transaction/connection_resource_guard.h`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ConnectionGuard &): n/a

#### `ConnectionGuard(storage::DatabaseConnectionManager &manager, bool blocking=true, std::chrono::seconds timeout=std::chrono::seconds(10))`
- Source: `include/transaction/connection_resource_guard.h`:33
- Brief: n/a
- Parameters:
  - `manager` (storage::DatabaseConnectionManager &): n/a
  - `blocking` (bool): n/a
  - `timeout` (std::chrono::seconds): n/a

#### `std::shared_ptr< const storage::DatabaseConnectionManager::Connection > getConnection() const noexcept`
- Source: `include/transaction/connection_resource_guard.h`:53
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< storage::DatabaseConnectionManager::Connection > getConnection() noexcept`
- Source: `include/transaction/connection_resource_guard.h`:50
- Brief: n/a
- Parameters: none

#### `bool isReleased() const noexcept`
- Source: `include/transaction/connection_resource_guard.h`:80
- Brief: Is Released.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Exception safety: noexcept.

#### `bool isValid() const noexcept`
- Source: `include/transaction/connection_resource_guard.h`:60
- Brief: Is Valid.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Exception safety: noexcept.

#### `void markError(std::string_view error_desc) noexcept`
- Source: `include/transaction/connection_resource_guard.h`:67
- Brief: Mark Error.
- Parameters:
  - `error_desc` (std::string_view): Input parameter.
- Details: error_desc Input parameter. Exception safety: noexcept.

#### `ConnectionGuard & operator=(ConnectionGuard &&other) noexcept`
- Source: `include/transaction/connection_resource_guard.h`:47
- Brief: n/a
- Parameters:
  - `other` (ConnectionGuard &&): n/a

#### `ConnectionGuard & operator=(const ConnectionGuard &)=delete`
- Source: `include/transaction/connection_resource_guard.h`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ConnectionGuard &): n/a

#### `void release() noexcept`
- Source: `include/transaction/connection_resource_guard.h`:73
- Brief: Release.
- Parameters: none
- Details: Exception safety: noexcept.

#### `~ConnectionGuard() noexcept`
- Source: `include/transaction/connection_resource_guard.h`:39
- Brief: n/a
- Parameters: none

### themis::transaction::ConnectionScopeTracker

#### `ConnectionScopeTracker(std::string_view operation_name, bool is_write=false)`
- Source: `include/transaction/connection_resource_guard.h`:91
- Brief: n/a
- Parameters:
  - `operation_name` (std::string_view): n/a
  - `is_write` (bool): n/a

#### `uint64_t getDurationMs() const noexcept`
- Source: `include/transaction/connection_resource_guard.h`:116
- Brief: Get Duration Ms.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `void recordFailure(std::string_view error_msg) noexcept`
- Source: `include/transaction/connection_resource_guard.h`:109
- Brief: Record Failure.
- Parameters:
  - `error_msg` (std::string_view): Input parameter.
- Details: error_msg Input parameter. Exception safety: noexcept.

#### `void recordSuccess() noexcept`
- Source: `include/transaction/connection_resource_guard.h`:102
- Brief: Record Success.
- Parameters: none
- Details: Exception safety: noexcept.

#### `~ConnectionScopeTracker() noexcept`
- Source: `include/transaction/connection_resource_guard.h`:96
- Brief: n/a
- Parameters: none

### themis::transaction::CrashRecoveryManager

#### `CrashRecoveryManager(const CrashRecoveryManager &)=delete`
- Source: `include/transaction/crash_recovery_manager.h`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CrashRecoveryManager &): n/a

#### `CrashRecoveryManager(const std::string &wal_path, bool sync_on_write=true)`
- Source: `include/transaction/crash_recovery_manager.h`:90
- Brief: n/a
- Parameters:
  - `wal_path` (const std::string &): n/a
  - `sync_on_write` (bool): n/a

#### `void appendLine(const std::string &json_line)`
- Source: `include/transaction/crash_recovery_manager.h`:203
- Brief: Append Line.
- Parameters:
  - `json_line` (const std::string &): Input parameter.
- Details: json_line Input parameter. json_line Input parameter. Calls: f(), is_open(), THEMIS_WARN(), flush().

#### `std::string base64Decode(const std::string &s)`
- Source: `include/transaction/crash_recovery_manager.h`:237
- Brief: Base64 Decode.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value.

#### `std::string base64Encode(const std::string &s)`
- Source: `include/transaction/crash_recovery_manager.h`:230
- Brief: Base64 Encode.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value.

#### `std::optional< LogEntry > deserialize(const std::string &line)`
- Source: `include/transaction/crash_recovery_manager.h`:223
- Brief: Deserialize.
- Parameters:
  - `line` (const std::string &): Input parameter.
- Return: Return value.
- Details: line Input parameter. Return value.

#### `std::vector< uint64_t > getInFlightTransactionIds() const`
- Source: `include/transaction/crash_recovery_manager.h`:149
- Brief: Get In Flight Transaction Ids.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `RecoveryMetrics getMetrics() const`
- Source: `include/transaction/crash_recovery_manager.h`:162
- Brief: Get Metrics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void logAbort(uint64_t txn_id)`
- Source: `include/transaction/crash_recovery_manager.h`:129
- Brief: Log Abort.
- Parameters:
  - `txn_id` (uint64_t): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `void logBegin(uint64_t txn_id, IsolationLevel isolation)`
- Source: `include/transaction/crash_recovery_manager.h`:103
- Brief: Log Begin.
- Parameters:
  - `txn_id` (uint64_t): Identifier of the txn.
  - `isolation` (IsolationLevel): Input parameter.
- Details: txn_id Identifier of the txn. isolation Input parameter. txn_id Identifier of the txn. isolation Input parameter. Calls: lk(), emplace(), erase(), nowMs(), appendLine(), serialize(), fetch_add().

#### `void logCommit(uint64_t txn_id)`
- Source: `include/transaction/crash_recovery_manager.h`:123
- Brief: Log Commit.
- Parameters:
  - `txn_id` (uint64_t): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `void logOperation(uint64_t txn_id, const std::string &op, const std::string &key, const std::optional< std::string > &old_value, const std::optional< std::string > &new_value)`
- Source: `include/transaction/crash_recovery_manager.h`:113
- Brief: Log Operation.
- Parameters:
  - `txn_id` (uint64_t): Identifier of the txn.
  - `op` (const std::string &): Input parameter.
  - `key` (const std::string &): Input parameter.
  - `old_value` (const std::optional< std::string > &): Input parameter.
  - `new_value` (const std::optional< std::string > &): Input parameter.
- Details: txn_id Identifier of the txn. op Input parameter. key Input parameter. old_value Input parameter. new_value Input parameter.

#### `bool needsRecovery() const`
- Source: `include/transaction/crash_recovery_manager.h`:136
- Brief: Needs Recovery.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `int64_t nowMs()`
- Source: `include/transaction/crash_recovery_manager.h`:243
- Brief: Now Ms.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `CrashRecoveryManager & operator=(const CrashRecoveryManager &)=delete`
- Source: `include/transaction/crash_recovery_manager.h`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CrashRecoveryManager &): n/a

#### `size_t pendingTransactionCount() const`
- Source: `include/transaction/crash_recovery_manager.h`:175
- Brief: Pending Transaction Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t pruneLog()`
- Source: `include/transaction/crash_recovery_manager.h`:156
- Brief: Prune Log.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: lk(), scanInFlight(), f(), is_open(), std::getline(), deserialize(), push_back(), count().

#### `std::vector< LogEntry > readAllEntries() const`
- Source: `include/transaction/crash_recovery_manager.h`:169
- Brief: Read All Entries.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `RecoveryResult recover(RocksDBWrapper &db)`
- Source: `include/transaction/crash_recovery_manager.h`:143
- Brief: Recover.
- Parameters:
  - `db` (RocksDBWrapper &): Input/output parameter.
- Return: Return value.
- Details: db Input/output parameter. Return value.

#### `std::unordered_set< uint64_t > scanInFlight() const`
- Source: `include/transaction/crash_recovery_manager.h`:209
- Brief: Scan In Flight.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string serialize(const LogEntry &e)`
- Source: `include/transaction/crash_recovery_manager.h`:216
- Brief: Serialize.
- Parameters:
  - `e` (const LogEntry &): Input parameter.
- Return: Return value.
- Details: e Input parameter. Return value.

#### `~CrashRecoveryManager()`
- Source: `include/transaction/crash_recovery_manager.h`:92
- Brief: n/a
- Parameters: none

### themis::transaction::CrashRecoveryManager::RecoveryResult

#### `bool hadWorkToDo() const`
- Source: `include/transaction/crash_recovery_manager.h`:71
- Brief: n/a
- Parameters: none

### themis::transaction::DistributedTransactionManager

#### `DistributedTransactionManager(DistributedTransactionManager &&)=delete`
- Source: `include/transaction/distributed_transaction_manager.h`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTransactionManager &&): n/a

#### `DistributedTransactionManager(const DistributedTransactionManager &)=delete`
- Source: `include/transaction/distributed_transaction_manager.h`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedTransactionManager &): n/a

#### `DistributedTransactionManager(std::string coordinator_id, DistributedTxnManagerConfig config={})`
- Source: `include/transaction/distributed_transaction_manager.h`:267
- Brief: n/a
- Parameters:
  - `coordinator_id` (std::string): n/a
  - `config` (DistributedTxnManagerConfig): n/a

#### `void abortDistributed(const TransactionId &txn_id)`
- Source: `include/transaction/distributed_transaction_manager.h`:306
- Brief: Abort Distributed.
- Parameters:
  - `txn_id` (const TransactionId &): Identifier of the txn.
- Details: txn_id Identifier of the txn. txn_id Identifier of the txn. Calls: lock(), findTransaction(), THEMIS_DEBUG(), logToWAL(), flush(), unlock(), runPhase2Unlocked(), empty().

#### `size_t activeTransactionCount() const`
- Source: `include/transaction/distributed_transaction_manager.h`:399
- Brief: Active Transaction Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `DistributedTxnStatus applyAbort(const TransactionId &txn_id)`
- Source: `include/transaction/distributed_transaction_manager.h`:334
- Brief: Apply Abort.
- Parameters:
  - `txn_id` (const TransactionId &): Identifier of the txn.
- Return: Return value.
- Details: txn_id Identifier of the txn. Return value. txn_id Identifier of the txn. Return value. Calls: lock(), findTransaction(), DistributedTxnStatus::Error(), THEMIS_DEBUG(), DistributedTxnStatus::OK().

#### `DistributedTxnStatus applyCommit(const TransactionId &txn_id)`
- Source: `include/transaction/distributed_transaction_manager.h`:327
- Brief: Apply Commit.
- Parameters:
  - `txn_id` (const TransactionId &): Identifier of the txn.
- Return: Return value.
- Details: txn_id Identifier of the txn. Return value. txn_id Identifier of the txn. Return value. Calls: lock(), findTransaction(), DistributedTxnStatus::Error(), THEMIS_DEBUG(), DistributedTxnStatus::OK().

#### `void batchFlushLoop()`
- Source: `include/transaction/distributed_transaction_manager.h`:548
- Brief: Batch Flush Loop.
- Parameters: none
- Details: ───────────────────────────────────────────────────────────────────────────── Batch-prepare flush loop (PERF-D4) ───────────────────────────────────────────────────────────────────────────── Calls: load(), lock(), wait_for(), empty(), swap(), THEMIS_DEBUG(), size(), runPhase1Unlocked().

#### `TransactionId beginDistributed(const std::vector< Participant > &participants)`
- Source: `include/transaction/distributed_transaction_manager.h`:286
- Brief: Begin Distributed.
- Parameters:
  - `participants` (const std::vector< Participant > &): Input parameter.
- Return: Return value.
- Details: participants Input parameter. Return value.

#### `size_t checkTimeouts()`
- Source: `include/transaction/distributed_transaction_manager.h`:352
- Brief: Check Timeouts.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: std::chrono::system_clock::now(), lock(), push_back(), THEMIS_WARN(), findTransaction(), abortDistributed().

#### `void clearLivenessCheckFn()`
- Source: `include/transaction/distributed_transaction_manager.h`:449
- Brief: Clear Liveness Check Fn.
- Parameters: none
- Details: Calls: lock().

#### `void clearRpcPhase1Fn()`
- Source: `include/transaction/distributed_transaction_manager.h`:433
- Brief: Clear Rpc Phase1 Fn.
- Parameters: none
- Details: Calls: lock().

#### `void clearRpcPhase2Fn()`
- Source: `include/transaction/distributed_transaction_manager.h`:416
- Brief: Clear Rpc Phase2 Fn.
- Parameters: none
- Details: Calls: lock().

#### `DistributedTxnStatus commitDistributed(const TransactionId &txn_id)`
- Source: `include/transaction/distributed_transaction_manager.h`:300
- Brief: Commit Distributed.
- Parameters:
  - `txn_id` (const TransactionId &): Identifier of the txn.
- Return: Return value.
- Details: txn_id Identifier of the txn. Return value.

#### `DistributedTransaction * findTransaction(const TransactionId &txn_id)`
- Source: `include/transaction/distributed_transaction_manager.h`:490
- Brief: Find Transaction.
- Parameters:
  - `txn_id` (const TransactionId &): Identifier of the txn.
- Return: Pointer to the result.
- Details: txn_id Identifier of the txn. Pointer to the result.

#### `const DistributedTransaction * findTransaction(const TransactionId &txn_id) const`
- Source: `include/transaction/distributed_transaction_manager.h`:496
- Brief: Find Transaction.
- Parameters:
  - `txn_id` (const TransactionId &): Identifier of the txn.
- Return: Pointer to the result.
- Details: txn_id Identifier of the txn. Pointer to the result.

#### `std::string generateTransactionId()`
- Source: `include/transaction/distributed_transaction_manager.h`:457
- Brief: Generate Transaction Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), str().

#### `std::vector< RecoverableTwoPhaseTransaction > getRecoverableTransactions() const override`
- Source: `include/transaction/distributed_transaction_manager.h`:345
- Brief: Return the currently known recoverable/non-final transactions.
- Parameters: none
- Return: Canonical recovery snapshot for global orchestration.
- Details: The returned snapshot reflects the coordinator's view at the time of the call. Non-final means any state except COMPLETED. Canonical recovery snapshot for global orchestration.

#### `Statistics getStatistics() const`
- Source: `include/transaction/distributed_transaction_manager.h`:393
- Brief: Return access control statistics.
- Parameters: none
- Return: Access control statistics.
- Details: Access control statistics.

#### `std::optional< DistributedTransaction > getTransaction(const TransactionId &txn_id) const`
- Source: `include/transaction/distributed_transaction_manager.h`:387
- Brief: Get Transaction.
- Parameters:
  - `txn_id` (const TransactionId &): Identifier of the txn.
- Return: Return value.
- Details: txn_id Identifier of the txn. Return value.

#### `bool isParticipantAlive(const std::string &node_id) const`
- Source: `include/transaction/distributed_transaction_manager.h`:379
- Brief: Is Participant Alive.
- Parameters:
  - `node_id` (const std::string &): Identifier of the node.
- Return: True when the operation succeeds.
- Details: node_id Identifier of the node. True when the operation succeeds.

#### `void logToWAL(themis::sharding::WALEntryType type, const std::string &txn_id, const std::string &data="")`
- Source: `include/transaction/distributed_transaction_manager.h`:459
- Brief: Log To WAL.
- Parameters:
  - `type` (themis::sharding::WALEntryType): Input parameter.
  - `txn_id` (const std::string &): Identifier of the txn.
  - `data` (const std::string &): Input parameter.
- Details: type Input parameter. txn_id Identifier of the txn. data Input parameter. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), empty(), append(), THEMIS_ERROR(), what().

#### `DistributedTransactionManager & operator=(DistributedTransactionManager &&)=delete`
- Source: `include/transaction/distributed_transaction_manager.h`:278
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTransactionManager &&): n/a

#### `DistributedTransactionManager & operator=(const DistributedTransactionManager &)=delete`
- Source: `include/transaction/distributed_transaction_manager.h`:276
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedTransactionManager &): n/a

#### `DistributedTxnStatus prepareDistributed(const TransactionId &txn_id)`
- Source: `include/transaction/distributed_transaction_manager.h`:293
- Brief: Prepare Distributed.
- Parameters:
  - `txn_id` (const TransactionId &): Identifier of the txn.
- Return: Return value.
- Details: txn_id Identifier of the txn. Return value.

#### `size_t recoverInDoubtTransactions() override`
- Source: `include/transaction/distributed_transaction_manager.h`:338
- Brief: Recover In Doubt Transactions.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: runPhase2Unlocked(), lock(), findTransaction(), THEMIS_DEBUG(), push_back(), finalizeRecovery(), THEMIS_INFO(), getOldestLSN().

#### `std::string recoveryBackendName() const override`
- Source: `include/transaction/distributed_transaction_manager.h`:342
- Brief: Return the durable backend used by this coordinator.
- Parameters: none
- Return: Backend name such as "WAL", "WAL/snapshot", or "disabled".
- Details: Backend name such as "WAL", "WAL/snapshot", or "disabled".

#### `std::string recoveryCoordinatorName() const override`
- Source: `include/transaction/distributed_transaction_manager.h`:340
- Brief: Return a stable coordinator identifier for recovery reports.
- Parameters: none
- Return: Human-readable coordinator type name (e.g. "TwoPhaseCommitCoordinator").
- Details: Human-readable coordinator type name (e.g. "TwoPhaseCommitCoordinator").

#### `bool runPhase1Unlocked(const TransactionId &txn_id)`
- Source: `include/transaction/distributed_transaction_manager.h`:470
- Brief: Run Phase1 Unlocked.
- Parameters:
  - `txn_id` (const TransactionId &): Identifier of the txn.
- Return: True when the operation succeeds.
- Details: ───────────────────────────────────────────────────────────────────────────── Phase 1: send PREPARE to all participants (without holding the mutex) ───────────────────────────────────────────────────────────────────────────── txn_id Identifier of the txn. True when the operation succeeds. txn_id Identifier of the txn. True when the operation succeeds. Calls: lock(), findTransaction(), std::chrono::steady_clock::now(), reserve(), size(), empty(), THEMIS_ERROR(), push_back().

#### `bool runPhase2Unlocked(const TransactionId &txn_id, const std::vector< Participant > &parts, bool do_commit)`
- Source: `include/transaction/distributed_transaction_manager.h`:479
- Brief: Run Phase2 Unlocked.
- Parameters:
  - `txn_id` (const TransactionId &): Identifier of the txn.
  - `parts` (const std::vector< Participant > &): Input parameter.
  - `do_commit` (bool): Input parameter.
- Return: True when the operation succeeds.
- Details: ───────────────────────────────────────────────────────────────────────────── Phase 2: send COMMIT or ABORT to all participants (without holding the mutex) ───────────────────────────────────────────────────────────────────────────── txn_id Identifier of the txn. parts Input parameter. do_commit Input parameter. True when the operation succeeds. txn_id Identifier of the txn. parts Input parameter. do_commit Input parameter. True when the operation succeeds. Calls: std::chrono::steady_clock::now(), reserve(), size(), empty(), THEMIS_ERROR(), push_back(), submitTask(), deliverPhase2WithRetry().

#### `void setLivenessCheckFn(StaticLivenessCheckFn fn)`
- Source: `include/transaction/distributed_transaction_manager.h`:444
- Brief: Set Liveness Check Fn.
- Parameters:
  - `fn` (StaticLivenessCheckFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lock(), std::move().

#### `void setRemotePhase2Fn(RemotePhase2Fn fn)`
- Source: `include/transaction/distributed_transaction_manager.h`:368
- Brief: Set Remote Phase2 Fn.
- Parameters:
  - `fn` (RemotePhase2Fn): Input parameter.
- Details: fn Input parameter. Calls: lock(), std::move().

#### `void setRpcPhase1Fn(RpcPhase1Fn fn)`
- Source: `include/transaction/distributed_transaction_manager.h`:428
- Brief: Set Rpc Phase1 Fn.
- Parameters:
  - `fn` (RpcPhase1Fn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lock(), std::move().

#### `void setRpcPhase2Fn(RpcPhase2Fn fn)`
- Source: `include/transaction/distributed_transaction_manager.h`:411
- Brief: Set Rpc Phase2 Fn.
- Parameters:
  - `fn` (RpcPhase2Fn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lock(), std::move().

#### `void startThreadPool()`
- Source: `include/transaction/distributed_transaction_manager.h`:531
- Brief: Start Thread Pool.
- Parameters: none
- Details: Calls: reserve(), emplace_back(), void(), lock(), wait_for(), std::chrono::seconds(), empty(), THEMIS_WARN().

#### `void stopThreadPool()`
- Source: `include/transaction/distributed_transaction_manager.h`:536
- Brief: Stop Thread Pool.
- Parameters: none
- Details: Calls: lock(), notify_all(), joinable(), join(), clear().

#### `std::future< decltype(f())> submitTask(F &&f)`
- Source: `include/transaction/distributed_transaction_manager.h`:501
- Brief: n/a
- Parameters:
  - `f` (F &&): n/a

#### `DistributedTxnStatus voteOnPrepare(const TransactionId &txn_id, const std::string &node_id, bool can_commit)`
- Source: `include/transaction/distributed_transaction_manager.h`:316
- Brief: Vote On Prepare.
- Parameters:
  - `txn_id` (const TransactionId &): Identifier of the txn.
  - `node_id` (const std::string &): Identifier of the node.
  - `can_commit` (bool): Input parameter.
- Return: Return value.
- Details: txn_id Identifier of the txn. node_id Identifier of the node. can_commit Input parameter. Return value. txn_id Identifier of the txn. node_id Identifier of the node. can_commit Input parameter. Return value. Calls: lock(), findTransaction(), DistributedTxnStatus::Error(), notify_all(), DistributedTxnStatus::OK().

#### `~DistributedTransactionManager()`
- Source: `include/transaction/distributed_transaction_manager.h`:272
- Brief: n/a
- Parameters: none

### themis::transaction::DistributedTxnStatus

#### `DistributedTxnStatus Error(std::string msg, std::uint32_t retry_count=0, themis::utils::RetryExhaustionReason exhaustion_reason=themis::utils::RetryExhaustionReason::NONE, themis::utils::RetryTimeoutSource timeout_source=themis::utils::RetryTimeoutSource::NONE, std::string correlation_id={})`
- Source: `include/transaction/distributed_transaction_manager.h`:171
- Brief: n/a
- Parameters:
  - `msg` (std::string): n/a
  - `retry_count` (std::uint32_t): n/a
  - `exhaustion_reason` (themis::utils::RetryExhaustionReason): n/a
  - `timeout_source` (themis::utils::RetryTimeoutSource): n/a
  - `correlation_id` (std::string): n/a

#### `DistributedTxnStatus OK()`
- Source: `include/transaction/distributed_transaction_manager.h`:170
- Brief: OK.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements OK without additional internal calls.

### themis::transaction::GlobalTransactionManager

#### `GlobalTransactionManager(const GlobalTransactionManager &)=delete`
- Source: `include/transaction/global_transaction_manager.h`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GlobalTransactionManager &): n/a

#### `GlobalTransactionManager(const std::string &coordinator_id, std::shared_ptr< themis::sharding::TrueTime > truetime)`
- Source: `include/transaction/global_transaction_manager.h`:158
- Brief: Global Transaction Manager.
- Parameters:
  - `coordinator_id` (const std::string &): Identifier of the coordinator.
  - `truetime` (std::shared_ptr< themis::sharding::TrueTime >): Input parameter.
- Return: Return value.
- Details: coordinator_id Identifier of the coordinator. truetime Input parameter. Return value.

#### `GlobalTransactionManager(const std::string &coordinator_id, std::shared_ptr< themis::sharding::TrueTime > truetime, const Config &config)`
- Source: `include/transaction/global_transaction_manager.h`:170
- Brief: Global Transaction Manager.
- Parameters:
  - `coordinator_id` (const std::string &): Identifier of the coordinator.
  - `truetime` (std::shared_ptr< themis::sharding::TrueTime >): Input parameter.
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: coordinator_id Identifier of the coordinator. truetime Input parameter. config Input parameter. Return value.

#### `bool abort(const std::string &txn_id)`
- Source: `include/transaction/global_transaction_manager.h`:239
- Brief: Abort.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Return: True when the operation succeeds.
- Details: txn_id Identifier of the txn. True when the operation succeeds. txn_id Identifier of the txn. True when the operation succeeds. Calls: lock(), find(), end(), THEMIS_WARN(), runPhase2(), fetch_add(), logToWAL(), THEMIS_INFO().

#### `bool addOperation(const std::string &txn_id, const std::string &region_id, const nlohmann::json &op)`
- Source: `include/transaction/global_transaction_manager.h`:221
- Brief: Add Operation.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
  - `region_id` (const std::string &): Identifier of the region.
  - `op` (const nlohmann::json &): Input parameter.
- Return: True when the operation succeeds.
- Details: txn_id Identifier of the txn. region_id Identifier of the region. op Input parameter. True when the operation succeeds. txn_id Identifier of the txn. region_id Identifier of the region. op Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), THEMIS_WARN(), push_back().

#### `std::string beginTransaction(const std::vector< std::string > &region_ids)`
- Source: `include/transaction/global_transaction_manager.h`:212
- Brief: Begin Transaction.
- Parameters:
  - `region_ids` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: region_ids Input parameter. Return value. region_ids Input parameter. Return value. std::invalid_argument if an error occurs. Calls: empty(), lock(), find(), end(), generateTransactionId(), std::chrono::steady_clock::now(), nlohmann::json::array(), std::move().

#### `GlobalTxnOutcome commit(const std::string &txn_id)`
- Source: `include/transaction/global_transaction_manager.h`:232
- Brief: Commit.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Return: Return value.
- Details: txn_id Identifier of the txn. Return value. txn_id Identifier of the txn. Return value. Calls: std::chrono::steady_clock::now(), lock(), find(), end(), fetch_add(), runPhase1(), now(), count().

#### `std::string generateTransactionId()`
- Source: `include/transaction/global_transaction_manager.h`:322
- Brief: Generate Transaction Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: fetch_add(), std::chrono::system_clock::now(), time_since_epoch(), count(), str().

#### `std::vector< RecoverableTwoPhaseTransaction > getRecoverableTransactions() const override`
- Source: `include/transaction/global_transaction_manager.h`:250
- Brief: Return the currently known recoverable/non-final transactions.
- Parameters: none
- Return: Canonical recovery snapshot for global orchestration.
- Details: The returned snapshot reflects the coordinator's view at the time of the call. Non-final means any state except COMPLETED. Canonical recovery snapshot for global orchestration.

#### `nlohmann::json getStatistics() const`
- Source: `include/transaction/global_transaction_manager.h`:266
- Brief: Return access control statistics.
- Parameters: none
- Return: Access control statistics.
- Details: Access control statistics.

#### `std::optional< GlobalTxnState > getTransactionState(const std::string &txn_id) const`
- Source: `include/transaction/global_transaction_manager.h`:258
- Brief: Get Transaction State.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Return: Return value.
- Details: txn_id Identifier of the txn. Return value.

#### `void logToWAL(themis::sharding::WALEntryType type, const std::string &txn_id, const nlohmann::json &data)`
- Source: `include/transaction/global_transaction_manager.h`:312
- Brief: Log To WAL.
- Parameters:
  - `type` (themis::sharding::WALEntryType): Input parameter.
  - `txn_id` (const std::string &): Identifier of the txn.
  - `data` (const nlohmann::json &): Input parameter.
- Details: type Input parameter. txn_id Identifier of the txn. data Input parameter. type Input parameter. txn_id Identifier of the txn. data Input parameter. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), append(), flush(), THEMIS_ERROR(), what().

#### `GlobalTransactionManager & operator=(const GlobalTransactionManager &)=delete`
- Source: `include/transaction/global_transaction_manager.h`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GlobalTransactionManager &): n/a

#### `size_t recoverInDoubtTransactions() override`
- Source: `include/transaction/global_transaction_manager.h`:243
- Brief: Recover In Doubt Transactions.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: THEMIS_INFO(), readRange(), getOldestLSN(), empty(), std::chrono::steady_clock::now(), contains(), nlohmann::json::array(), value().

#### `std::string recoveryBackendName() const override`
- Source: `include/transaction/global_transaction_manager.h`:247
- Brief: Return the durable backend used by this coordinator.
- Parameters: none
- Return: Backend name such as "WAL", "WAL/snapshot", or "disabled".
- Details: Backend name such as "WAL", "WAL/snapshot", or "disabled".

#### `std::string recoveryCoordinatorName() const override`
- Source: `include/transaction/global_transaction_manager.h`:245
- Brief: Return a stable coordinator identifier for recovery reports.
- Parameters: none
- Return: Human-readable coordinator type name (e.g. "TwoPhaseCommitCoordinator").
- Details: Human-readable coordinator type name (e.g. "TwoPhaseCommitCoordinator").

#### `size_t regionCount() const`
- Source: `include/transaction/global_transaction_manager.h`:204
- Brief: Region Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void registerRegion(const std::string &region_id, IGlobalRegionParticipant *participant)`
- Source: `include/transaction/global_transaction_manager.h`:188
- Brief: Register Region.
- Parameters:
  - `region_id` (const std::string &): Identifier of the region.
  - `participant` (IGlobalRegionParticipant *): Input/output parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: region_id Identifier of the region. participant Input/output parameter. region_id Identifier of the region. participant Input/output parameter. std::invalid_argument if an error occurs. Calls: lock(), THEMIS_DEBUG().

#### `bool runPhase1(GlobalTxnRecord &rec)`
- Source: `include/transaction/global_transaction_manager.h`:297
- Brief: Run Phase1.
- Parameters:
  - `rec` (GlobalTxnRecord &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: ───────────────────────────────────────────────────────────────────────────── Internal helpers (may be called with mutex_ held) ───────────────────────────────────────────────────────────────────────────── rec Input/output parameter. True when the operation succeeds. rec Input/output parameter. True when the operation succeeds. Calls: find(), end(), THEMIS_ERROR(), count(), at(), nlohmann::json::array(), prepare(), what().

#### `void runPhase2(GlobalTxnRecord &rec, bool do_commit)`
- Source: `include/transaction/global_transaction_manager.h`:304
- Brief: Run Phase2.
- Parameters:
  - `rec` (GlobalTxnRecord &): Input/output parameter.
  - `do_commit` (bool): Input parameter.
- Details: rec Input/output parameter. do_commit Input parameter. rec Input/output parameter. do_commit Input parameter. Calls: find(), end(), THEMIS_WARN(), commit(), abort(), THEMIS_ERROR(), what().

#### `bool unregisterRegion(const std::string &region_id)`
- Source: `include/transaction/global_transaction_manager.h`:198
- Brief: Unregister Region.
- Parameters:
  - `region_id` (const std::string &): Identifier of the region.
- Return: True when the operation succeeds.
- Details: region_id Identifier of the region. True when the operation succeeds. region_id Identifier of the region. True when the operation succeeds. Calls: lock(), erase().

#### `~GlobalTransactionManager()=default`
- Source: `include/transaction/global_transaction_manager.h`:176
- Brief: n/a
- Parameters: none

### themis::transaction::GlobalTwoPhaseCommitRecoveryManager

#### `GlobalTwoPhaseCommitRecoveryReport recoverAll(const std::vector< IRecoverableTwoPhaseCoordinator * > &coordinators)`
- Source: `include/transaction/recoverable_two_phase_coordinator.h`:179
- Brief: Recover all supplied coordinators and return a unified report.
- Parameters:
  - `coordinators` (const std::vector< IRecoverableTwoPhaseCoordinator * > &): Recovery-capable coordinators to visit exactly once.
- Return: Aggregated before/after counts and per-coordinator results.
- Details: Iterates coordinators in order, calls recoverInDoubtTransactions() on each, and accumulates before/after counts and per-coordinator details. Null pointers in the list are silently skipped. coordinators Recovery-capable coordinators to visit exactly once. Aggregated before/after counts and per-coordinator results.

### themis::transaction::GlobalTxnOutcome

#### `bool committed() const`
- Source: `include/transaction/global_transaction_manager.h`:108
- Brief: n/a
- Parameters: none

### themis::transaction::GrpcRpcPhase1Adapter

#### `DistributedTransactionManager::RpcPhase1Fn make(const std::map< std::string, std::string > &node_addresses, std::chrono::milliseconds timeout, std::optional< MtlsConfig > mtls=std::nullopt)`
- Source: `include/transaction/grpc_rpc_adapter.h`:124
- Brief: Create a RpcPhase1Fn callable.
- Parameters:
  - `node_addresses` (const std::map< std::string, std::string > &): Map from node_id → "host:port". Unknown node_ids vote ABORT.
  - `timeout` (std::chrono::milliseconds): gRPC deadline applied to every PREPARE call.
  - `mtls` (std::optional< MtlsConfig >): Optional mTLS credential bundle. When present and all three PEM fields are non-empty, the channel is created with grpc::SslCredentials. Missing PEM material is rejected unless allow_insecure is explicitly set for a local test override.
- Return: Callable compatible with DistributedTransactionManager::RpcPhase1Fn.
- Details: node_addresses Map from node_id → "host:port". Unknown node_ids vote ABORT. timeout gRPC deadline applied to every PREPARE call. mtls Optional mTLS credential bundle. When present and all three PEM fields are non-empty, the channel is created with grpc::SslCredentials. Missing PEM material is rejected unless allow_insecure is explicitly set for a local test override. Callable compatible with DistributedTransactionManager::RpcPhase1Fn.

### themis::transaction::GrpcRpcPhase2Adapter

#### `DistributedTransactionManager::RpcPhase2Fn make(const std::map< std::string, std::string > &node_addresses, std::chrono::milliseconds timeout, std::optional< MtlsConfig > mtls=std::nullopt)`
- Source: `include/transaction/grpc_rpc_adapter.h`:163
- Brief: Create a RpcPhase2Fn callable.
- Parameters:
  - `node_addresses` (const std::map< std::string, std::string > &): Map from node_id → "host:port".
  - `timeout` (std::chrono::milliseconds): Per-attempt gRPC deadline.
  - `mtls` (std::optional< MtlsConfig >): Optional mTLS credential bundle (see GrpcRpcPhase1Adapter::make for semantics).
- Return: Callable compatible with DistributedTransactionManager::RpcPhase2Fn.
- Details: node_addresses Map from node_id → "host:port". timeout Per-attempt gRPC deadline. mtls Optional mTLS credential bundle (see GrpcRpcPhase1Adapter::make for semantics). Callable compatible with DistributedTransactionManager::RpcPhase2Fn.

### themis::transaction::IDistributedParticipantCallback

#### `void onAbort(const std::string &txn_id)=0`
- Source: `include/transaction/distributed_transaction_manager.h`:96
- Brief: On Abort.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `void onCommit(const std::string &txn_id)=0`
- Source: `include/transaction/distributed_transaction_manager.h`:90
- Brief: On Commit.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `bool onPrepare(const std::string &txn_id, const std::set< std::string > &affected_keys)=0`
- Source: `include/transaction/distributed_transaction_manager.h`:81
- Brief: n/a
- Parameters:
  - `txn_id` (const std::string &): n/a
  - `affected_keys` (const std::set< std::string > &): n/a

#### `~IDistributedParticipantCallback()=default`
- Source: `include/transaction/distributed_transaction_manager.h`:79
- Brief: IDistributed Participant Callback.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::IGlobalRegionParticipant

#### `void abort(const std::string &txn_id)=0`
- Source: `include/transaction/global_transaction_manager.h`:89
- Brief: Abort.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
- Details: txn_id Identifier of the txn.

#### `void commit(const std::string &txn_id, int64_t commit_timestamp)=0`
- Source: `include/transaction/global_transaction_manager.h`:80
- Brief: Commit.
- Parameters:
  - `txn_id` (const std::string &): Identifier of the txn.
  - `commit_timestamp` (int64_t): Input parameter.
- Details: txn_id Identifier of the txn. commit_timestamp Input parameter.

#### `bool prepare(const std::string &txn_id, const nlohmann::json &ops)=0`
- Source: `include/transaction/global_transaction_manager.h`:70
- Brief: n/a
- Parameters:
  - `txn_id` (const std::string &): n/a
  - `ops` (const nlohmann::json &): n/a

#### `~IGlobalRegionParticipant()=default`
- Source: `include/transaction/global_transaction_manager.h`:68
- Brief: IGlobal Region Participant.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::IInDoubtRecoveryCoordinator

#### `size_t recoverInDoubtTransactions()=0`
- Source: `include/transaction/in_doubt_recovery_coordinator.h`:32
- Brief: n/a
- Parameters: none

#### `~IInDoubtRecoveryCoordinator()=default`
- Source: `include/transaction/in_doubt_recovery_coordinator.h`:30
- Brief: IIn Doubt Recovery Coordinator.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::IRecoverableTwoPhaseCoordinator

#### `std::vector< RecoverableTwoPhaseTransaction > getRecoverableTransactions() const =0`
- Source: `include/transaction/recoverable_two_phase_coordinator.h`:116
- Brief: Return the currently known recoverable/non-final transactions.
- Parameters: none
- Return: Canonical recovery snapshot for global orchestration.
- Details: The returned snapshot reflects the coordinator's view at the time of the call. Non-final means any state except COMPLETED. Canonical recovery snapshot for global orchestration.

#### `std::string recoveryBackendName() const =0`
- Source: `include/transaction/recoverable_two_phase_coordinator.h`:105
- Brief: Return the durable backend used by this coordinator.
- Parameters: none
- Return: Backend name such as "WAL", "WAL/snapshot", or "disabled".
- Details: Backend name such as "WAL", "WAL/snapshot", or "disabled".

#### `std::string recoveryCoordinatorName() const =0`
- Source: `include/transaction/recoverable_two_phase_coordinator.h`:99
- Brief: Return a stable coordinator identifier for recovery reports.
- Parameters: none
- Return: Human-readable coordinator type name (e.g. "TwoPhaseCommitCoordinator").
- Details: Human-readable coordinator type name (e.g. "TwoPhaseCommitCoordinator").

#### `~IRecoverableTwoPhaseCoordinator() override=default`
- Source: `include/transaction/recoverable_two_phase_coordinator.h`:93
- Brief: n/a
- Parameters: none

### themis::transaction::ITransactionCoordinator

#### `ITransactionCoordinator()=default`
- Source: `include/transaction/transaction_coordinator.h`:207
- Brief: n/a
- Parameters: none

#### `ITransactionCoordinator(ITransactionCoordinator &&) noexcept=default`
- Source: `include/transaction/transaction_coordinator.h`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator &&): n/a

#### `ITransactionCoordinator(const ITransactionCoordinator &)=delete`
- Source: `include/transaction/transaction_coordinator.h`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ITransactionCoordinator &): n/a

#### `TxnCoordinatorResult abort(std::string_view txn_id)=0`
- Source: `include/transaction/transaction_coordinator.h`:182
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `TxnCoordinatorResult begin(std::string_view txn_id, const TxnCoordinatorOptions &opts={})=0`
- Source: `include/transaction/transaction_coordinator.h`:169
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a
  - `opts` (const TxnCoordinatorOptions &): n/a

#### `CoordinatorCapabilities capabilities() const noexcept=0`
- Source: `include/transaction/transaction_coordinator.h`:165
- Brief: n/a
- Parameters: none

#### `TxnCoordinatorResult commit(std::string_view txn_id)=0`
- Source: `include/transaction/transaction_coordinator.h`:178
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `std::vector< InDoubtTxnDescriptor > getInDoubtTransactions() const =0`
- Source: `include/transaction/transaction_coordinator.h`:197
- Brief: n/a
- Parameters: none

#### `TxnLifecycleState getState(std::string_view txn_id) const =0`
- Source: `include/transaction/transaction_coordinator.h`:188
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `ITransactionCoordinator & operator=(ITransactionCoordinator &&) noexcept=default`
- Source: `include/transaction/transaction_coordinator.h`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (ITransactionCoordinator &&): n/a

#### `ITransactionCoordinator & operator=(const ITransactionCoordinator &)=delete`
- Source: `include/transaction/transaction_coordinator.h`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ITransactionCoordinator &): n/a

#### `TxnCoordinatorResult prepare(std::string_view txn_id)=0`
- Source: `include/transaction/transaction_coordinator.h`:174
- Brief: n/a
- Parameters:
  - `txn_id` (std::string_view): n/a

#### `std::string_view protocolName() const noexcept=0`
- Source: `include/transaction/transaction_coordinator.h`:163
- Brief: n/a
- Parameters: none

#### `CommitProtocol protocolType() const noexcept=0`
- Source: `include/transaction/transaction_coordinator.h`:161
- Brief: n/a
- Parameters: none

#### `std::size_t recoverInDoubt()=0`
- Source: `include/transaction/transaction_coordinator.h`:194
- Brief: n/a
- Parameters: none

#### `~ITransactionCoordinator()=default`
- Source: `include/transaction/transaction_coordinator.h`:157
- Brief: ITransaction Coordinator.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::MergeEngine

#### `MergeEngine(MergeEngine &&) noexcept=default`
- Source: `include/transaction/merge_engine.h`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEngine &&): n/a

#### `MergeEngine(analytics::DiffEngine &diff_engine, SnapshotManager &snapshot_manager, Changefeed &changefeed)`
- Source: `include/transaction/merge_engine.h`:155
- Brief: Merge Engine.
- Parameters:
  - `diff_engine` (analytics::DiffEngine &): Input/output parameter.
  - `snapshot_manager` (SnapshotManager &): Input/output parameter.
  - `changefeed` (Changefeed &): Input/output parameter.
- Return: Return value.
- Details: diff_engine Input/output parameter. snapshot_manager Input/output parameter. changefeed Input/output parameter. Return value.

#### `MergeEngine(const MergeEngine &)=delete`
- Source: `include/transaction/merge_engine.h`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MergeEngine &): n/a

#### `uint64_t applyChanges(const std::vector< analytics::DiffEngine::Change > &changes)`
- Source: `include/transaction/merge_engine.h`:289
- Brief: Apply Changes.
- Parameters:
  - `changes` (const std::vector< analytics::DiffEngine::Change > &): Input parameter.
- Return: Return value.
- Details: changes Input parameter. Return value. changes Input parameter. Return value. Calls: getLatestSequence(), std::chrono::system_clock::now(), time_since_epoch(), count(), json::object(), recordEvent().

#### `std::optional< analytics::DiffEngine::Change > autoResolve(const Conflict &conflict) const`
- Source: `include/transaction/merge_engine.h`:316
- Brief: Auto Resolve.
- Parameters:
  - `conflict` (const Conflict &): Input parameter.
- Return: Return value.
- Details: conflict Input parameter. Return value.

#### `bool canFastForward(uint64_t base_sequence, uint64_t source_sequence, uint64_t target_sequence)`
- Source: `include/transaction/merge_engine.h`:245
- Brief: Can Fast Forward.
- Parameters:
  - `base_sequence` (uint64_t): Input parameter.
  - `source_sequence` (uint64_t): Input parameter.
  - `target_sequence` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: base_sequence Input parameter. source_sequence Input parameter. target_sequence Input parameter. True when the operation succeeds.

#### `std::vector< Conflict > detectConflicts(const analytics::DiffEngine::DiffResult &source_diff, const analytics::DiffEngine::DiffResult &target_diff, uint64_t base_sequence)`
- Source: `include/transaction/merge_engine.h`:267
- Brief: Detect Conflicts.
- Parameters:
  - `source_diff` (const analytics::DiffEngine::DiffResult &): Input parameter.
  - `target_diff` (const analytics::DiffEngine::DiffResult &): Input parameter.
  - `base_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: source_diff Input parameter. target_diff Input parameter. base_sequence Input parameter. Return value. source_diff Input parameter. target_diff Input parameter. base_sequence Input parameter. Return value. Calls: addToMap(), find(), end(), getValueAtSequence(), push_back().

#### `std::optional< std::string > getValueAtSequence(const std::string &key, uint64_t sequence)`
- Source: `include/transaction/merge_engine.h`:299
- Brief: Get Value At Sequence.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: Get value at sequence. key Input parameter. sequence Input parameter. Return value. key Input parameter. sequence Input parameter. Return value. Calls: listEvents().

#### `bool isAutoResolvable(const Conflict &conflict) const`
- Source: `include/transaction/merge_engine.h`:309
- Brief: Is Auto Resolvable.
- Parameters:
  - `conflict` (const Conflict &): Input parameter.
- Return: True when the operation succeeds.
- Details: conflict Input parameter. True when the operation succeeds.

#### `MergeResult merge(uint64_t base_sequence, uint64_t source_sequence, uint64_t target_sequence)`
- Source: `include/transaction/merge_engine.h`:176
- Brief: Merge.
- Parameters:
  - `base_sequence` (uint64_t): Input parameter.
  - `source_sequence` (uint64_t): Input parameter.
  - `target_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: Main merge function (no-arg overload). base_sequence Input parameter. source_sequence Input parameter. target_sequence Input parameter. Return value. base_sequence Input parameter. source_sequence Input parameter. target_sequence Input parameter. Return value. Implements merge without additional internal calls.

#### `MergeResult merge(uint64_t base_sequence, uint64_t source_sequence, uint64_t target_sequence, const MergeOptions &options)`
- Source: `include/transaction/merge_engine.h`:190
- Brief: Merge.
- Parameters:
  - `base_sequence` (uint64_t): Input parameter.
  - `source_sequence` (uint64_t): Input parameter.
  - `target_sequence` (uint64_t): Input parameter.
  - `options` (const MergeOptions &): Input parameter.
- Return: Return value.
- Details: base_sequence Input parameter. source_sequence Input parameter. target_sequence Input parameter. options Input parameter. Return value. base_sequence Input parameter. source_sequence Input parameter. target_sequence Input parameter. options Input parameter. Return value. Calls: spdlog::info(), computeDiff(), spdlog::debug(), push_back(), size(), applyChanges(), detectConflicts(), empty().

#### `MergeResult mergeByTag(const std::string &base_tag, const std::string &source_tag, const std::string &target_tag)`
- Source: `include/transaction/merge_engine.h`:204
- Brief: Merge By Tag.
- Parameters:
  - `base_tag` (const std::string &): Input parameter.
  - `source_tag` (const std::string &): Input parameter.
  - `target_tag` (const std::string &): Input parameter.
- Return: Return value.
- Details: Merge by tag (no-arg overload). base_tag Input parameter. source_tag Input parameter. target_tag Input parameter. Return value. base_tag Input parameter. source_tag Input parameter. target_tag Input parameter. Return value. Implements mergeByTag without additional internal calls.

#### `MergeResult mergeByTag(const std::string &base_tag, const std::string &source_tag, const std::string &target_tag, const MergeOptions &options)`
- Source: `include/transaction/merge_engine.h`:218
- Brief: Merge By Tag.
- Parameters:
  - `base_tag` (const std::string &): Input parameter.
  - `source_tag` (const std::string &): Input parameter.
  - `target_tag` (const std::string &): Input parameter.
  - `options` (const MergeOptions &): Input parameter.
- Return: Return value.
- Details: base_tag Input parameter. source_tag Input parameter. target_tag Input parameter. options Input parameter. Return value. base_tag Input parameter. source_tag Input parameter. target_tag Input parameter. options Input parameter. Return value. Calls: spdlog::info(), getTag(), has_value(), fmt::format(), spdlog::error(), getLatestSequence(), merge().

#### `MergeEngine & operator=(MergeEngine &&) noexcept=default`
- Source: `include/transaction/merge_engine.h`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEngine &&): n/a

#### `MergeEngine & operator=(const MergeEngine &)=delete`
- Source: `include/transaction/merge_engine.h`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MergeEngine &): n/a

#### `MergeResult previewMerge(uint64_t base_sequence, uint64_t source_sequence, uint64_t target_sequence)`
- Source: `include/transaction/merge_engine.h`:232
- Brief: Preview Merge.
- Parameters:
  - `base_sequence` (uint64_t): Input parameter.
  - `source_sequence` (uint64_t): Input parameter.
  - `target_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: base_sequence Input parameter. source_sequence Input parameter. target_sequence Input parameter. Return value. base_sequence Input parameter. source_sequence Input parameter. target_sequence Input parameter. Return value. Calls: merge().

#### `std::vector< analytics::DiffEngine::Change > resolveConflicts(const std::vector< Conflict > &conflicts, const MergeOptions &options)`
- Source: `include/transaction/merge_engine.h`:279
- Brief: Resolve Conflicts.
- Parameters:
  - `conflicts` (const std::vector< Conflict > &): Input parameter.
  - `options` (const MergeOptions &): Input parameter.
- Return: Return value.
- Details: conflicts Input parameter. options Input parameter. Return value. conflicts Input parameter. options Input parameter. Return value. Calls: find(), end(), has_value(), push_back(), isAutoResolvable(), autoResolve().

#### `~MergeEngine()=default`
- Source: `include/transaction/merge_engine.h`:161
- Brief: n/a
- Parameters: none

### themis::transaction::MergeEngine::Conflict

#### `Conflict fromJson(const json &j)`
- Source: `include/transaction/merge_engine.h`:63
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: contains().

#### `json toJson() const`
- Source: `include/transaction/merge_engine.h`:57
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::MergeEngine::ConflictResolution

#### `ConflictResolution fromJson(const json &j)`
- Source: `include/transaction/merge_engine.h`:80
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: contains().

#### `json toJson() const`
- Source: `include/transaction/merge_engine.h`:74
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::MergeEngine::MergeOptions

#### `MergeOptions fromJson(const json &j)`
- Source: `include/transaction/merge_engine.h`:99
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: contains(), push_back().

#### `json toJson() const`
- Source: `include/transaction/merge_engine.h`:93
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::MergeEngine::MergeResult

#### `MergeResult fromJson(const json &j)`
- Source: `include/transaction/merge_engine.h`:145
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: contains(), push_back().

#### `json toJson() const`
- Source: `include/transaction/merge_engine.h`:139
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::MergeEngine::MergeStats

#### `MergeStats fromJson(const json &j)`
- Source: `include/transaction/merge_engine.h`:120
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: value().

#### `json toJson() const`
- Source: `include/transaction/merge_engine.h`:114
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::SAGAOrchestratorGuard

#### `SAGAOrchestratorGuard(SAGAOrchestratorGuard &&)=delete`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (SAGAOrchestratorGuard &&): n/a

#### `SAGAOrchestratorGuard(const SAGAOrchestrator::Config &config={})`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:42
- Brief: Create orchestrator with given configuration.
- Parameters:
  - `config` (const SAGAOrchestrator::Config &): n/a
- Throws:
  - std::bad_alloc: or any exception from orchestrator initialization
- Details: std::bad_alloc or any exception from orchestrator initialization

#### `SAGAOrchestratorGuard(const SAGAOrchestratorGuard &)=delete`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SAGAOrchestratorGuard &): n/a

#### `const SAGAOrchestrator * get() const noexcept`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:93
- Brief: Get the orchestrator instance (const).
- Parameters: none

#### `SAGAOrchestrator * get() noexcept`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:86
- Brief: Get the orchestrator instance.
- Parameters: none
- Return: Pointer to orchestrator, or nullptr if not initialized
- Details: Pointer to orchestrator, or nullptr if not initialized

#### `SAGAOrchestratorGuard & operator=(SAGAOrchestratorGuard &&)=delete`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (SAGAOrchestratorGuard &&): n/a

#### `SAGAOrchestratorGuard & operator=(const SAGAOrchestratorGuard &)=delete`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SAGAOrchestratorGuard &): n/a

#### `void reset(const SAGAOrchestrator::Config &config={})`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:108
- Brief: Replace the orchestrator with a new one.
- Parameters:
  - `config` (const SAGAOrchestrator::Config &): n/a
- Throws:
  - std::bad_alloc: or any exception from orchestrator initialization
- Details: std::bad_alloc or any exception from orchestrator initialization

#### `bool valid() const noexcept`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:100
- Brief: Check if orchestrator is valid.
- Parameters: none

#### `~SAGAOrchestratorGuard() noexcept`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:60
- Brief: Destructor waits for pending operations and cleans up.
- Parameters: none

### themis::transaction::SagaOrchestratorPlugin

#### `SagaOrchestratorPlugin()`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:128
- Brief: n/a
- Parameters: none

#### `plugins::PluginCapabilities getCapabilities() const override`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:143
- Brief: n/a
- Parameters: none

#### `void * getInstance() override`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:200
- Brief: n/a
- Parameters: none

#### `const char * getName() const override`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:131
- Brief: n/a
- Parameters: none

#### `plugins::PluginType getType() const override`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:139
- Brief: n/a
- Parameters: none

#### `const char * getVersion() const override`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:135
- Brief: n/a
- Parameters: none

#### `bool initialize(const char *config_json) override`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:153
- Brief: n/a
- Parameters:
  - `config_json` (const char *): n/a

#### `void shutdown() override`
- Source: `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`:192
- Brief: n/a
- Parameters: none

### themis::transaction::SnapshotManager

#### `SnapshotManager(RocksDBWrapper &db, Changefeed &changefeed)`
- Source: `include/transaction/snapshot_manager.h`:70
- Brief: Snapshot Manager.
- Parameters:
  - `db` (RocksDBWrapper &): Input/output parameter.
  - `changefeed` (Changefeed &): Input/output parameter.
- Return: Return value.
- Details: db Input/output parameter. changefeed Input/output parameter. Return value.

#### `SnapshotManager(SnapshotManager &&) noexcept=default`
- Source: `include/transaction/snapshot_manager.h`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManager &&): n/a

#### `SnapshotManager(const SnapshotManager &)=delete`
- Source: `include/transaction/snapshot_manager.h`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SnapshotManager &): n/a

#### `size_t checkConsistency() const`
- Source: `include/transaction/snapshot_manager.h`:164
- Brief: Check Consistency.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< Snapshot > createTag(const std::string &tag_name, const std::string &description, const std::string &created_by="system")`
- Source: `include/transaction/snapshot_manager.h`:80
- Brief: Create a new tag.
- Parameters:
  - `tag_name` (const std::string &): Name of the tag.
  - `description` (const std::string &): Input parameter.
  - `created_by` (const std::string &): Input parameter.
- Return: Return value.
- Details: tag_name Name of the tag. description Input parameter. created_by Input parameter. Return value. Calls: lock(), isValidTagName(), spdlog::error(), tagExists(), getLatestSequence(), std::chrono::system_clock::now(), time_since_epoch(), count().

#### `bool deleteTag(const std::string &tag_name)`
- Source: `include/transaction/snapshot_manager.h`:104
- Brief: Delete Tag.
- Parameters:
  - `tag_name` (const std::string &): Name of the tag.
- Return: True when the operation succeeds.
- Details: tag_name Name of the tag. True when the operation succeeds. tag_name Name of the tag. True when the operation succeeds. Calls: lock(), tagExists(), spdlog::warn(), makeKey(), del(), spdlog::error(), spdlog::info().

#### `std::optional< Snapshot > deserialize(const std::vector< uint8_t > &data) const`
- Source: `include/transaction/snapshot_manager.h`:221
- Brief: Deserialize.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value.

#### `std::string extractTagName(const std::string &key) const`
- Source: `include/transaction/snapshot_manager.h`:207
- Brief: Extract Tag Name.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value.

#### `std::optional< uint64_t > getSequenceForTag(const std::string &tag_name) const`
- Source: `include/transaction/snapshot_manager.h`:124
- Brief: Get Sequence For Tag.
- Parameters:
  - `tag_name` (const std::string &): Name of the tag.
- Return: Return value.
- Details: tag_name Name of the tag. Return value.

#### `SnapshotStats getStats() const`
- Source: `include/transaction/snapshot_manager.h`:117
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< Snapshot > getTag(const std::string &tag_name) const`
- Source: `include/transaction/snapshot_manager.h`:91
- Brief: Get Tag.
- Parameters:
  - `tag_name` (const std::string &): Name of the tag.
- Return: Return value.
- Details: tag_name Name of the tag. Return value.

#### `std::optional< int64_t > getTimestampForTag(const std::string &tag_name) const`
- Source: `include/transaction/snapshot_manager.h`:131
- Brief: Get Timestamp For Tag.
- Parameters:
  - `tag_name` (const std::string &): Name of the tag.
- Return: Return value.
- Details: tag_name Name of the tag. Return value.

#### `bool isValidTagName(const std::string &tag_name)`
- Source: `include/transaction/snapshot_manager.h`:138
- Brief: Is Valid Tag Name.
- Parameters:
  - `tag_name` (const std::string &): Name of the tag.
- Return: True when the operation succeeds.
- Details: tag_name Name of the tag. True when the operation succeeds. tag_name Name of the tag. True when the operation succeeds. Calls: empty(), length(), pattern(), std::regex_match().

#### `std::vector< Snapshot > listTags(size_t limit=0, const std::string &sort_by="timestamp", bool ascending=false) const`
- Source: `include/transaction/snapshot_manager.h`:93
- Brief: n/a
- Parameters:
  - `limit` (size_t): n/a
  - `sort_by` (const std::string &): n/a
  - `ascending` (bool): n/a

#### `std::string makeKey(const std::string &tag_name) const`
- Source: `include/transaction/snapshot_manager.h`:200
- Brief: Make Key.
- Parameters:
  - `tag_name` (const std::string &): Name of the tag.
- Return: Return value.
- Details: tag_name Name of the tag. Return value.

#### `SnapshotManager & operator=(SnapshotManager &&) noexcept=default`
- Source: `include/transaction/snapshot_manager.h`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (SnapshotManager &&): n/a

#### `SnapshotManager & operator=(const SnapshotManager &)=delete`
- Source: `include/transaction/snapshot_manager.h`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SnapshotManager &): n/a

#### `size_t pruneOldSnapshots()`
- Source: `include/transaction/snapshot_manager.h`:158
- Brief: Prune Old Snapshots.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: lock(), newIterator(), spdlog::warn(), std::move(), value(), Seek(), Valid(), Next().

#### `RestoreResult restoreToTag(const std::string &tag_name, const std::string &created_by="system")`
- Source: `include/transaction/snapshot_manager.h`:182
- Brief: - Phase 7: Snapshot Restore -
- Parameters:
  - `tag_name` (const std::string &): Name of the tag.
  - `created_by` (const std::string &): Input parameter.
- Return: Return value.
- Details: tag_name Name of the tag. created_by Input parameter. Return value.

#### `std::vector< uint8_t > serialize(const Snapshot &snapshot) const`
- Source: `include/transaction/snapshot_manager.h`:214
- Brief: Serialize.
- Parameters:
  - `snapshot` (const Snapshot &): Input parameter.
- Return: Return value.
- Details: snapshot Input parameter. Return value.

#### `void setRetentionPolicy(const RetentionPolicy &policy)`
- Source: `include/transaction/snapshot_manager.h`:152
- Brief: Set Retention Policy.
- Parameters:
  - `policy` (const RetentionPolicy &): Input parameter.
- Details: - Phase 7: GC & Retention Policy - policy Input parameter. policy Input parameter. Calls: lock(), spdlog::info().

#### `bool tagExists(const std::string &tag_name) const`
- Source: `include/transaction/snapshot_manager.h`:111
- Brief: Tag Exists.
- Parameters:
  - `tag_name` (const std::string &): Name of the tag.
- Return: True when the operation succeeds.
- Details: tag_name Name of the tag. True when the operation succeeds.

#### `~SnapshotManager()=default`
- Source: `include/transaction/snapshot_manager.h`:72
- Brief: n/a
- Parameters: none

### themis::transaction::SnapshotManager::RestoreResult

#### `json toJson() const`
- Source: `include/transaction/snapshot_manager.h`:179
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::SnapshotManager::Snapshot

#### `Snapshot fromJson(const json &j)`
- Source: `include/transaction/snapshot_manager.h`:47
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Implements fromJson without additional internal calls.

#### `json toJson() const`
- Source: `include/transaction/snapshot_manager.h`:41
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::SnapshotManager::SnapshotStats

#### `json toJson() const`
- Source: `include/transaction/snapshot_manager.h`:61
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::transaction::TransactionConnectionGuard

#### `TransactionConnectionGuard(const TransactionConnectionGuard &)=delete`
- Source: `include/transaction/connection_resource_guard.h`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransactionConnectionGuard &): n/a

#### `TransactionConnectionGuard(uint64_t txn_id, storage::DatabaseConnectionManager &manager)`
- Source: `include/transaction/connection_resource_guard.h`:133
- Brief: Transaction Connection Guard.
- Parameters:
  - `txn_id` (uint64_t): Identifier of the txn.
  - `manager` (storage::DatabaseConnectionManager &): Input/output parameter.
- Return: Return value.
- Details: txn_id Identifier of the txn. manager Input/output parameter. Return value.

#### `std::shared_ptr< storage::DatabaseConnectionManager::Connection > acquireConnection(std::string_view operation_name, bool is_write=false) noexcept`
- Source: `include/transaction/connection_resource_guard.h`:145
- Brief: n/a
- Parameters:
  - `operation_name` (std::string_view): n/a
  - `is_write` (bool): n/a

#### `size_t getConnectionCount() const noexcept`
- Source: `include/transaction/connection_resource_guard.h`:180
- Brief: Get Connection Count.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `uint64_t getConnectionTimeMs() const noexcept`
- Source: `include/transaction/connection_resource_guard.h`:173
- Brief: Get Connection Time Ms.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `size_t getFailureCount() const noexcept`
- Source: `include/transaction/connection_resource_guard.h`:194
- Brief: Get Failure Count.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `size_t getSuccessCount() const noexcept`
- Source: `include/transaction/connection_resource_guard.h`:187
- Brief: Get Success Count.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `TransactionConnectionGuard & operator=(const TransactionConnectionGuard &)=delete`
- Source: `include/transaction/connection_resource_guard.h`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransactionConnectionGuard &): n/a

#### `void recordFailure(std::string_view operation_name, std::string_view error_msg) noexcept`
- Source: `include/transaction/connection_resource_guard.h`:163
- Brief: Record Failure.
- Parameters:
  - `operation_name` (std::string_view): Name of the operation.
  - `error_msg` (std::string_view): Input parameter.
- Details: operation_name Name of the operation. error_msg Input parameter. Exception safety: noexcept.

#### `void recordSuccess(std::string_view operation_name) noexcept`
- Source: `include/transaction/connection_resource_guard.h`:155
- Brief: Record Success.
- Parameters:
  - `operation_name` (std::string_view): Name of the operation.
- Details: operation_name Name of the operation. Exception safety: noexcept.

#### `void releaseAllConnections() noexcept`
- Source: `include/transaction/connection_resource_guard.h`:200
- Brief: Release All Connections.
- Parameters: none
- Details: Exception safety: noexcept.

#### `~TransactionConnectionGuard() noexcept`
- Source: `include/transaction/connection_resource_guard.h`:138
- Brief: n/a
- Parameters: none

### themis::transaction::TransactionSemanticAdvisor

#### `TransactionSemanticAdvisor()`
- Source: `include/transaction/transaction_semantic_advisor.h`:71
- Brief: n/a
- Parameters: none

#### `TransactionSemanticAdvisor(Config config)`
- Source: `include/transaction/transaction_semantic_advisor.h`:77
- Brief: Transaction Semantic Advisor.
- Parameters:
  - `config` (Config): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `TransactionSemanticAdvisor(TransactionSemanticAdvisor &&) noexcept=default`
- Source: `include/transaction/transaction_semantic_advisor.h`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisor &&): n/a

#### `TransactionSemanticAdvisor(const TransactionSemanticAdvisor &)=delete`
- Source: `include/transaction/transaction_semantic_advisor.h`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransactionSemanticAdvisor &): n/a

#### `std::vector< BatchAffinityHint > analyzeBatch(const std::vector< TransactionContext > &pending_txs) const`
- Source: `include/transaction/transaction_semantic_advisor.h`:99
- Brief: Analyze Batch.
- Parameters:
  - `pending_txs` (const std::vector< TransactionContext > &): Input parameter.
- Return: Return value.
- Details: pending_txs Input parameter. Return value.

#### `void emitDecisionRecord(size_t hint_count, size_t tx_count) const`
- Source: `include/transaction/transaction_semantic_advisor.h`:139
- Brief: Emit Decision Record.
- Parameters:
  - `hint_count` (size_t): Input parameter.
  - `tx_count` (size_t): Input parameter.
- Details: hint_count Input parameter. tx_count Input parameter.

#### `double entityOverlap(const TransactionContext &a, const TransactionContext &b)`
- Source: `include/transaction/transaction_semantic_advisor.h`:122
- Brief: Entity Overlap.
- Parameters:
  - `a` (const TransactionContext &): Input parameter.
  - `b` (const TransactionContext &): Input parameter.
- Return: Return value.
- Details: a Input parameter. b Input parameter. Return value.

#### `bool hasWriteConflict(const TransactionContext &a, const TransactionContext &b)`
- Source: `include/transaction/transaction_semantic_advisor.h`:131
- Brief: Has Write Conflict.
- Parameters:
  - `a` (const TransactionContext &): Input parameter.
  - `b` (const TransactionContext &): Input parameter.
- Return: True when the operation succeeds.
- Details: a Input parameter. b Input parameter. True when the operation succeeds.

#### `TransactionSemanticAdvisor & operator=(TransactionSemanticAdvisor &&) noexcept=default`
- Source: `include/transaction/transaction_semantic_advisor.h`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionSemanticAdvisor &&): n/a

#### `TransactionSemanticAdvisor & operator=(const TransactionSemanticAdvisor &)=delete`
- Source: `include/transaction/transaction_semantic_advisor.h`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TransactionSemanticAdvisor &): n/a

#### `void setDecisionRecordProcessor(std::shared_ptr< themis::llm::DecisionRecordYamlProcessor > processor)`
- Source: `include/transaction/transaction_semantic_advisor.h`:90
- Brief: Set Decision Record Processor.
- Parameters:
  - `processor` (std::shared_ptr< themis::llm::DecisionRecordYamlProcessor >): Input parameter.
- Details: processor Input parameter.

#### `std::chrono::milliseconds suggestDeferral(const TransactionContext &tx, const std::vector< TransactionContext > &concurrent_txs) const`
- Source: `include/transaction/transaction_semantic_advisor.h`:108
- Brief: Suggest Deferral.
- Parameters:
  - `tx` (const TransactionContext &): Input parameter.
  - `concurrent_txs` (const std::vector< TransactionContext > &): Input parameter.
- Return: Return value.
- Details: tx Input parameter. concurrent_txs Input parameter. Return value.

#### `~TransactionSemanticAdvisor()=default`
- Source: `include/transaction/transaction_semantic_advisor.h`:78
- Brief: n/a
- Parameters: none

### themis::transaction::TransactionStateSnapshot

#### `TransactionStateSnapshot(const DistributedTransaction &txn)`
- Source: `src/transaction/distributed_transaction_manager.cpp`:57
- Brief: Transaction State Snapshot.
- Parameters:
  - `txn` (const DistributedTransaction &): Input parameter.
- Return: Return value.
- Details: txn Input parameter. Return value.

### themis::transaction::TwoPhaseCommitWALRecovery

#### `void applyBegin(RecoveredTwoPhaseCommitTransaction &rec, const nlohmann::json &data)`
- Source: `include/transaction/two_phase_commit_wal_recovery.h`:110
- Brief: Apply Begin.
- Parameters:
  - `rec` (RecoveredTwoPhaseCommitTransaction &): Input/output parameter.
  - `data` (const nlohmann::json &): Input parameter.
- Details: rec Input/output parameter. data Input parameter. Calls: contains(), is_string(), mergeParticipants().

#### `void applyDecisionOrComplete(RecoveredTwoPhaseCommitTransaction &rec, const nlohmann::json &data, bool is_commit)`
- Source: `include/transaction/two_phase_commit_wal_recovery.h`:132
- Brief: Apply Decision Or Complete.
- Parameters:
  - `rec` (RecoveredTwoPhaseCommitTransaction &): Input/output parameter.
  - `data` (const nlohmann::json &): Input parameter.
  - `is_commit` (bool): Input parameter.
- Details: rec Input/output parameter. data Input parameter. is_commit Input parameter. Calls: contains(), is_string(), is_number().

#### `void mergeParticipants(RecoveredTwoPhaseCommitTransaction &rec, const nlohmann::json &data, const char *field_name)`
- Source: `include/transaction/two_phase_commit_wal_recovery.h`:171
- Brief: Merge Participants.
- Parameters:
  - `rec` (RecoveredTwoPhaseCommitTransaction &): Input/output parameter.
  - `data` (const nlohmann::json &): Input parameter.
  - `field_name` (const char *): Name of the field.
- Details: rec Input/output parameter. data Input parameter. field_name Name of the field. Calls: contains(), is_array(), is_string(), push_back(), is_object().

#### `std::map< std::string, RecoveredTwoPhaseCommitTransaction > reconstruct(const std::vector< themis::sharding::WALEntry > &entries)`
- Source: `include/transaction/two_phase_commit_wal_recovery.h`:63
- Brief: n/a
- Parameters:
  - `entries` (const std::vector< themis::sharding::WALEntry > &): n/a

### themis::transaction::TxnCoordinatorResult

#### `TxnCoordinatorResult Fail(ErrorCode ec, std::string msg)`
- Source: `include/transaction/transaction_coordinator.h`:117
- Brief: n/a
- Parameters:
  - `ec` (ErrorCode): n/a
  - `msg` (std::string): n/a

#### `TxnCoordinatorResult OK()`
- Source: `include/transaction/transaction_coordinator.h`:115
- Brief: n/a
- Parameters: none

#### `operator bool() const noexcept`
- Source: `include/transaction/transaction_coordinator.h`:121
- Brief: n/a
- Parameters: none

### themis::transaction::test

#### `TEST_F(CascadingTimeoutTest, CascadingAbort_FullChainTermination)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:313
- Brief: AC-5.3: Cascading Abort — Full chain aborts on timeout.
- Parameters:
  - `<unnamed>` (CascadingTimeoutTest): n/a
  - `<unnamed>` (CascadingAbort_FullChainTermination): n/a

#### `TEST_F(CascadingTimeoutTest, MixedTimeoutAndSuccess_PartialCompletion)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:369
- Brief: AC-5.3: Mixed Timeout and Success — Some steps timeout, some succeed.
- Parameters:
  - `<unnamed>` (CascadingTimeoutTest): n/a
  - `<unnamed>` (MixedTimeoutAndSuccess_PartialCompletion): n/a

#### `TEST_F(CascadingTimeoutTest, PartialTimeoutCascade_SelectiveCompensation)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:344
- Brief: AC-5.3: Partial Timeout Cascade — Some steps completed before timeout.
- Parameters:
  - `<unnamed>` (CascadingTimeoutTest): n/a
  - `<unnamed>` (PartialTimeoutCascade_SelectiveCompensation): n/a

#### `TEST_F(CircuitBreakerTest, CircuitBreakerActivatesAtThreshold_5Failures)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:69
- Brief: AC-9.1: Circuit Breaker Activation — Threshold 5 consecutive failures.
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CircuitBreakerActivatesAtThreshold_5Failures): n/a

#### `TEST_F(CircuitBreakerTest, CircuitBreakerEdgeCase_NoTriggerBeforeThreshold)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:152
- Brief: AC-9.1: Edge Case — Single failure doesn't trigger circuit breaker.
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CircuitBreakerEdgeCase_NoTriggerBeforeThreshold): n/a

#### `TEST_F(CircuitBreakerTest, CircuitBreakerMetrics_CountingAndTracking)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:134
- Brief: AC-9.1: Metrics Collection — Failure count, state transitions.
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CircuitBreakerMetrics_CountingAndTracking): n/a

#### `TEST_F(CircuitBreakerTest, CircuitBreakerRecovery_HalfOpenAfterTimeout)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:90
- Brief: AC-9.1: Circuit Breaker Recovery — Half-open state after timeout.
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CircuitBreakerRecovery_HalfOpenAfterTimeout): n/a

#### `TEST_F(CircuitBreakerTest, CircuitBreakerRecovery_SuccessfulTransition)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:110
- Brief: AC-9.1: Successful Recovery — Transition HALF_OPEN → CLOSED.
- Parameters:
  - `<unnamed>` (CircuitBreakerTest): n/a
  - `<unnamed>` (CircuitBreakerRecovery_SuccessfulTransition): n/a

#### `TEST_F(CompensationIdempotencyTest, CompensationIdempotency_10ConcurrentRetries)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:188
- Brief: AC-10.1: Compensation Idempotency — Same compensation called 10x → same outcome.
- Parameters:
  - `<unnamed>` (CompensationIdempotencyTest): n/a
  - `<unnamed>` (CompensationIdempotency_10ConcurrentRetries): n/a

#### `TEST_F(CompensationIdempotencyTest, CompensationOrdering_ReverseSequence)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:233
- Brief: AC-10.1: Compensation Ordering — Reverse sequence (LIFO).
- Parameters:
  - `<unnamed>` (CompensationIdempotencyTest): n/a
  - `<unnamed>` (CompensationOrdering_ReverseSequence): n/a

#### `TEST_F(CompensationIdempotencyTest, CompensationTimeout_PartialWithTimeout)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:317
- Brief: AC-10.1: Compensation Timeout — Partial compensation with timeout.
- Parameters:
  - `<unnamed>` (CompensationIdempotencyTest): n/a
  - `<unnamed>` (CompensationTimeout_PartialWithTimeout): n/a

#### `TEST_F(CompensationIdempotencyTest, ConcurrentCompensationCalls_NoRaceConditions)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:288
- Brief: AC-10.1: Concurrent Compensation Calls — Race condition handling.
- Parameters:
  - `<unnamed>` (CompensationIdempotencyTest): n/a
  - `<unnamed>` (ConcurrentCompensationCalls_NoRaceConditions): n/a

#### `TEST_F(CompensationIdempotencyTest, ErrorPropagation_ConsistentAcrossRetries)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:337
- Brief: AC-10.1: Error Propagation — Errors consistent across retries.
- Parameters:
  - `<unnamed>` (CompensationIdempotencyTest): n/a
  - `<unnamed>` (ErrorPropagation_ConsistentAcrossRetries): n/a

#### `TEST_F(CompensationIdempotencyTest, RollbackChain_CascadingCompensation)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:264
- Brief: AC-10.1: Rollback Chain — Cascading compensation under cascading failures.
- Parameters:
  - `<unnamed>` (CompensationIdempotencyTest): n/a
  - `<unnamed>` (RollbackChain_CascadingCompensation): n/a

#### `TEST_F(CrashRecoveryChaosTest, ClockSkew_OutOfOrderTimestamps)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:365
- Brief: Chaos 3: Clock Skew (Out-of-Order Timestamps).
- Parameters:
  - `<unnamed>` (CrashRecoveryChaosTest): n/a
  - `<unnamed>` (ClockSkew_OutOfOrderTimestamps): n/a

#### `TEST_F(CrashRecoveryChaosTest, NetworkPartitionDuringRecovery)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:318
- Brief: Chaos 1: Network Partition During Recovery.
- Parameters:
  - `<unnamed>` (CrashRecoveryChaosTest): n/a
  - `<unnamed>` (NetworkPartitionDuringRecovery): n/a

#### `TEST_F(CrashRecoveryChaosTest, ResourceExhaustion_LargeWAL)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:344
- Brief: Chaos 2: Resource Exhaustion (Large WAL).
- Parameters:
  - `<unnamed>` (CrashRecoveryChaosTest): n/a
  - `<unnamed>` (ResourceExhaustion_LargeWAL): n/a

#### `TEST_F(CrashRecoveryDeterminismTest, DeterministicRollback_50Replays)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:412
- Brief: AC-6.2 + AC-6.5: Deterministic Rollback — Same Scenario Replayed 50x Yields Identical Outcomes.
- Parameters:
  - `<unnamed>` (CrashRecoveryDeterminismTest): n/a
  - `<unnamed>` (DeterministicRollback_50Replays): n/a

#### `TEST_F(CrashRecoveryDeterminismTest, IdempotentRecovery_MultipleRuns)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:455
- Brief: AC-6.5: Idempotent Recovery — Verify Multiple Recovery Runs Yield Same State.
- Parameters:
  - `<unnamed>` (CrashRecoveryDeterminismTest): n/a
  - `<unnamed>` (IdempotentRecovery_MultipleRuns): n/a

#### `TEST_F(CrashRecoveryIntegrationTest, DeterministicRollbackUnderContention)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:199
- Brief: AC-6.2 + AC-6.3: Deterministic Rollback Under Contention.
- Parameters:
  - `<unnamed>` (CrashRecoveryIntegrationTest): n/a
  - `<unnamed>` (DeterministicRollbackUnderContention): n/a

#### `TEST_F(CrashRecoveryIntegrationTest, IdempotentWALReplay_MultipleReplays)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:240
- Brief: AC-6.5: Idempotent WAL Replay — Same WAL replayed N times yields identical state.
- Parameters:
  - `<unnamed>` (CrashRecoveryIntegrationTest): n/a
  - `<unnamed>` (IdempotentWALReplay_MultipleReplays): n/a

#### `TEST_F(CrashRecoveryIntegrationTest, RecoveryTimeBudget_WithinSLA)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:265
- Brief: AC-6.1: Recovery Time Budget — Verify completion within SLA.
- Parameters:
  - `<unnamed>` (CrashRecoveryIntegrationTest): n/a
  - `<unnamed>` (RecoveryTimeBudget_WithinSLA): n/a

#### `TEST_F(CrashRecoveryIntegrationTest, RecoveryUnderSIGKILL_AbruptTermination)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:163
- Brief: AC-6.1 + AC-6.6: Recovery under SIGKILL — Simulate abrupt termination + recovery.
- Parameters:
  - `<unnamed>` (CrashRecoveryIntegrationTest): n/a
  - `<unnamed>` (RecoveryUnderSIGKILL_AbruptTermination): n/a

#### `TEST_F(CrashRecoveryUnitTest, InFlightDetection_UncommittedTransactions)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:113
- Brief: AC-6.3: In-Flight Detection — Verify detection of uncommitted transactions.
- Parameters:
  - `<unnamed>` (CrashRecoveryUnitTest): n/a
  - `<unnamed>` (InFlightDetection_UncommittedTransactions): n/a

#### `TEST_F(CrashRecoveryUnitTest, WALEntryParsing_BasicStructure)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:71
- Brief: AC-6.1: WAL Entry Parsing — Verify BEGIN/OP/COMMIT/ABORT entries are correctly parsed.
- Parameters:
  - `<unnamed>` (CrashRecoveryUnitTest): n/a
  - `<unnamed>` (WALEntryParsing_BasicStructure): n/a

#### `TEST_F(CrashRecoveryUnitTest, WALLogicalOrdering_NoTimestampDependency)`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:91
- Brief: AC-6.4: WAL Logical Ordering — Verify entries are in logical order (not timestamp-based).
- Parameters:
  - `<unnamed>` (CrashRecoveryUnitTest): n/a
  - `<unnamed>` (WALLogicalOrdering_NoTimestampDependency): n/a

#### `TEST_F(PartialFailureTest, CascadingPartialFailure_StepBlockage)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:422
- Brief: AC-9.2: Cascading Partial Failure — Failure in step 2 blocks step 3.
- Parameters:
  - `<unnamed>` (PartialFailureTest): n/a
  - `<unnamed>` (CascadingPartialFailure_StepBlockage): n/a

#### `TEST_F(PartialFailureTest, PartialFailureRecovery_CheckpointRetry)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:452
- Brief: AC-9.2: Partial Failure Recovery — Allow retry from specific checkpoint.
- Parameters:
  - `<unnamed>` (PartialFailureTest): n/a
  - `<unnamed>` (PartialFailureRecovery_CheckpointRetry): n/a

#### `TEST_F(PartialFailureTest, PartialFailureWithTimeout_MixedScenarios)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:437
- Brief: AC-9.2: Partial Failure with Timeout — Handle mixed timeouts + failures.
- Parameters:
  - `<unnamed>` (PartialFailureTest): n/a
  - `<unnamed>` (PartialFailureWithTimeout_MixedScenarios): n/a

#### `TEST_F(PartialFailureTest, PartialFailure_MixedSuccessAndFailure)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:374
- Brief: AC-9.2: Partial Failure — Some steps OK, some fail.
- Parameters:
  - `<unnamed>` (PartialFailureTest): n/a
  - `<unnamed>` (PartialFailure_MixedSuccessAndFailure): n/a

#### `TEST_F(PartialFailureTest, SelectiveCompensation_SkipFailedSteps)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:396
- Brief: AC-9.2: Selective Compensation — Don't compensate failed steps.
- Parameters:
  - `<unnamed>` (PartialFailureTest): n/a
  - `<unnamed>` (SelectiveCompensation_SkipFailedSteps): n/a

#### `TEST_F(RetryStormTest, BoundedRetries_MaximumThreeRetries)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:537
- Brief: AC-10.2: Bounded Retries — Max 3 retries (no infinite loops).
- Parameters:
  - `<unnamed>` (RetryStormTest): n/a
  - `<unnamed>` (BoundedRetries_MaximumThreeRetries): n/a

#### `TEST_F(RetryStormTest, ExponentialBackoff_BaseAndFactor)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:488
- Brief: AC-10.2: Exponential Backoff — Base 100ms, factor 2x.
- Parameters:
  - `<unnamed>` (RetryStormTest): n/a
  - `<unnamed>` (ExponentialBackoff_BaseAndFactor): n/a

#### `TEST_F(RetryStormTest, JitterBounds_PlusMinus20Percent)`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:511
- Brief: AC-10.2: Jitter Bounds — ±20% jitter on backoff.
- Parameters:
  - `<unnamed>` (RetryStormTest): n/a
  - `<unnamed>` (JitterBounds_PlusMinus20Percent): n/a

#### `TEST_F(TimeoutDetectionTest, CascadingTimeout_AbortPropagation)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:128
- Brief: AC-5.3: Cascading Timeout — Abort cascades to subsequent steps.
- Parameters:
  - `<unnamed>` (TimeoutDetectionTest): n/a
  - `<unnamed>` (CascadingTimeout_AbortPropagation): n/a

#### `TEST_F(TimeoutDetectionTest, TimeoutClockDriftInvariance_PlusMinus100ms)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:98
- Brief: AC-5.2: Timeout Handling Independent of Clock Drift — ±100ms variation.
- Parameters:
  - `<unnamed>` (TimeoutDetectionTest): n/a
  - `<unnamed>` (TimeoutClockDriftInvariance_PlusMinus100ms): n/a

#### `TEST_F(TimeoutDetectionTest, TimeoutDetection_BasicTimeout)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:71
- Brief: AC-5.1: Timeout Detection — Basic timeout at configured duration.
- Parameters:
  - `<unnamed>` (TimeoutDetectionTest): n/a
  - `<unnamed>` (TimeoutDetection_BasicTimeout): n/a

#### `TEST_F(TimeoutDeterminismTest, ClockJitterInvariance_DeterministicOutcome)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:200
- Brief: AC-5.2: Clock Jitter Invariance — ±100ms jitter doesn't break determinism.
- Parameters:
  - `<unnamed>` (TimeoutDeterminismTest): n/a
  - `<unnamed>` (ClockJitterInvariance_DeterministicOutcome): n/a

#### `TEST_F(TimeoutDeterminismTest, DeterministicTimeout_50xReplay)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:174
- Brief: AC-5.1: Deterministic Timeout Detection — 50x replay identical outcome.
- Parameters:
  - `<unnamed>` (TimeoutDeterminismTest): n/a
  - `<unnamed>` (DeterministicTimeout_50xReplay): n/a

#### `TEST_F(TimeoutDeterminismTest, TimeoutOrdering_DistributedLedger)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:231
- Brief: AC-5.5: Timeout Ordering Preserved — Distributed ledger ordering.
- Parameters:
  - `<unnamed>` (TimeoutDeterminismTest): n/a
  - `<unnamed>` (TimeoutOrdering_DistributedLedger): n/a

#### `TEST_F(TimeoutDeterminismTest, TimeoutStateConsistency_MultipleQueries)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:272
- Brief: AC-5.1: Timeout State Consistency — Same state across lookups.
- Parameters:
  - `<unnamed>` (TimeoutDeterminismTest): n/a
  - `<unnamed>` (TimeoutStateConsistency_MultipleQueries): n/a

#### `TEST_F(TimeoutEdgeCaseTest, TimeoutAccuracy_Within50msMargin)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:411
- Brief: AC-5.4: Maximum Timeout Accuracy — Within ±50ms bounds.
- Parameters:
  - `<unnamed>` (TimeoutEdgeCaseTest): n/a
  - `<unnamed>` (TimeoutAccuracy_Within50msMargin): n/a

#### `TEST_F(TimeoutEdgeCaseTest, TimeoutDuringCompensation_GracefulHandling)`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:440
- Brief: AC-5.3: Timeout During Compensation — Handle gracefully.
- Parameters:
  - `<unnamed>` (TimeoutEdgeCaseTest): n/a
  - `<unnamed>` (TimeoutDuringCompensation_GracefulHandling): n/a

#### `TEST_F(TransactionWaveAByzantineTest, TxnByzantine01_ConflictingVotesForceAbort)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:603
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveAByzantineTest): n/a
  - `<unnamed>` (TxnByzantine01_ConflictingVotesForceAbort): n/a
- Details: TestTXN-BYZANTINE-01: Any BYZANTINE_CONFLICT vote forces coordinator to ABORT. 4 participants; 2 send conflicting votes → outcome must be ABORTED.

#### `TEST_F(TransactionWaveAByzantineTest, TxnByzantine02_AllCommitVotesYieldCommit)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:619
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveAByzantineTest): n/a
  - `<unnamed>` (TxnByzantine02_AllCommitVotesYieldCommit): n/a
- Details: TestTXN-BYZANTINE-02: All COMMIT votes (no conflicts) yield COMMITTED outcome. Validates the normal path is preserved after Byzantine safeguard.

#### `TEST_F(TransactionWaveACrossShardTest, TxnXShard01_CrashAtPrepare)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:639
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveACrossShardTest): n/a
  - `<unnamed>` (TxnXShard01_CrashAtPrepare): n/a
- Details: TestTXN-XSHARD-01: Coordinator crash at prepare phase → all shards see ABORTED.

#### `TEST_F(TransactionWaveACrossShardTest, TxnXShard02_NetworkPartition2PC)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:656
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveACrossShardTest): n/a
  - `<unnamed>` (TxnXShard02_NetworkPartition2PC): n/a
- Details: TestTXN-XSHARD-02: Network partition during 2PC → TIMEOUT, no inconsistency.

#### `TEST_F(TransactionWaveARecoveryTest, TxnRecovery01_CleanRestart)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveARecoveryTest): n/a
  - `<unnamed>` (TxnRecovery01_CleanRestart): n/a
- Details: TestTXN-RECOVERY-01: Clean coordinator restart resolves all in-doubt entries. WAL contains 100 in-flight transactions. After replay all must be resolved.

#### `TEST_F(TransactionWaveARecoveryTest, TxnRecovery02_CrashDuring2PCPrepare)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:364
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveARecoveryTest): n/a
  - `<unnamed>` (TxnRecovery02_CrashDuring2PCPrepare): n/a
- Details: TestTXN-RECOVERY-02: Coordinator crash during 2PC prepare resolves via abort. All in-doubt transactions must be aborted (conservative policy).

#### `TEST_F(TransactionWaveARecoveryTest, TxnRecovery03_IdempotentReplay)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:388
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveARecoveryTest): n/a
  - `<unnamed>` (TxnRecovery03_IdempotentReplay): n/a
- Details: TestTXN-RECOVERY-03: Crash during 3PC pre-commit — WAL replay is idempotent. Calling replay twice must not change the outcome (no double-abort).

#### `TEST_F(TransactionWaveARecoveryTest, TxnRecovery04_CascadingCrash)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:405
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveARecoveryTest): n/a
  - `<unnamed>` (TxnRecovery04_CascadingCrash): n/a
- Details: TestTXN-RECOVERY-04: Cascading coordinator+participant crash resolved without data loss — all records end in a terminal state, none lost.

#### `TEST_F(TransactionWaveASAGATest, TxnSagaHardening01_CircuitBreakerTrip)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:431
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveASAGATest): n/a
  - `<unnamed>` (TxnSagaHardening01_CircuitBreakerTrip): n/a
- Details: TestTXN-SAGA-HARDENING-01: Circuit breaker opens after 5 consecutive failures. After the threshold, canAttempt() must return false.

#### `TEST_F(TransactionWaveASAGATest, TxnSagaHardening02_IdempotentCompensation)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:453
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveASAGATest): n/a
  - `<unnamed>` (TxnSagaHardening02_IdempotentCompensation): n/a
- Details: TestTXN-SAGA-HARDENING-02: Idempotent compensation under concurrent retry storm. 10 concurrent threads each call compensate() for the same step; only one must succeed.

#### `TEST_F(TransactionWaveASAGATest, TxnSagaHardening03_PartialFailureOrdering)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:483
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveASAGATest): n/a
  - `<unnamed>` (TxnSagaHardening03_PartialFailureOrdering): n/a
- Details: TestTXN-SAGA-HARDENING-03: Partial failure ordering — steps compensated in reverse order (last-executed first).

#### `TEST_F(TransactionWaveASAGATest, TxnSagaHardening04_RetryStormBoundedBackoff)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:506
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveASAGATest): n/a
  - `<unnamed>` (TxnSagaHardening04_RetryStormBoundedBackoff): n/a
- Details: TestTXN-SAGA-HARDENING-04: Retry storm with bounded backoff stays within max retries and does not attempt after circuit opens.

#### `TEST_F(TransactionWaveATimeoutTest, TxnTimeout01_BackoffSchedule)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:535
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveATimeoutTest): n/a
  - `<unnamed>` (TxnTimeout01_BackoffSchedule): n/a
- Details: TestTXN-TIMEOUT-01: Backoff schedule validation — base 100ms, factor 2×. First delay ≈ 100ms, second ≈ 200ms, third ≈ 400ms (within jitter tolerance).

#### `TEST_F(TransactionWaveATimeoutTest, TxnTimeout02_MonotonicExpectedValues)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:556
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveATimeoutTest): n/a
  - `<unnamed>` (TxnTimeout02_MonotonicExpectedValues): n/a
- Details: TestTXN-TIMEOUT-02: Delays must increase monotonically in expected-value terms (stochastic; seed 42 provides deterministic outcome).

#### `TEST_F(TransactionWaveATimeoutTest, TxnTimeout03_JitterBoundsStatistical)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:572
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransactionWaveATimeoutTest): n/a
  - `<unnamed>` (TxnTimeout03_JitterBoundsStatistical): n/a
- Details: TestTXN-TIMEOUT-03: Jitter bounds stay within ±20% of nominal for all retries. Runs 100 seeds to confirm statistical invariant.

#### `std::vector< int > computeBackoffDelays(const BackoffConfig &cfg, uint32_t seed)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:214
- Brief: Computes the backoff delays for a retry sequence. Returns one delay per retry attempt (does not include initial attempt).
- Parameters:
  - `cfg` (const BackoffConfig &): n/a
  - `seed` (uint32_t): n/a

### themis::transaction::test::ByzantineMockCoordinator

#### `ByzantineMockCoordinator(int participant_count)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:236
- Brief: n/a
- Parameters:
  - `participant_count` (int): n/a

#### `CommitResult decide() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:254
- Brief: Evaluate votes and return commit decision. Any BYZANTINE_CONFLICT vote must force abort (fail-safe).
- Parameters: none

#### `bool hasByzantineVote() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:264
- Brief: n/a
- Parameters: none

#### `void injectByzantineVotes()`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:242
- Brief: Inject Byzantine votes: participant_count/2 conflict pairs.
- Parameters: none

#### `int participantCount() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:274
- Brief: n/a
- Parameters: none

#### `const std::vector< PrepareVote > & votes() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:273
- Brief: n/a
- Parameters: none

### themis::transaction::test::CascadingTimeoutTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:301
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:305
- Brief: n/a
- Parameters: none

### themis::transaction::test::CircuitBreakerTest

#### `void SetUp() override`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:56
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:61
- Brief: n/a
- Parameters: none

### themis::transaction::test::CompensationIdempotencyTest

#### `void SetUp() override`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:173
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:178
- Brief: n/a
- Parameters: none

### themis::transaction::test::CompensationLog

#### `bool compensate(const std::string &step_id)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:179
- Brief: Record a compensation call for a step.
- Parameters:
  - `step_id` (const std::string &): n/a
- Return: false if the step was already compensated (idempotent guard).
- Details: false if the step was already compensated (idempotent guard).

#### `int compensatedCount() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:189
- Brief: n/a
- Parameters: none

#### `bool wasCompensated(const std::string &step_id) const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:184
- Brief: n/a
- Parameters:
  - `step_id` (const std::string &): n/a

### themis::transaction::test::CrashRecoveryChaosTest

#### `void SetUp() override`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:298
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:306
- Brief: n/a
- Parameters: none

### themis::transaction::test::CrashRecoveryDeterminismTest

#### `void SetUp() override`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:392
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:400
- Brief: n/a
- Parameters: none

### themis::transaction::test::CrashRecoveryIntegrationTest

#### `void SetUp() override`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:143
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:151
- Brief: n/a
- Parameters: none

### themis::transaction::test::CrashRecoveryUnitTest

#### `void SetUp() override`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:51
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_coordinator_crash_recovery.cpp`:59
- Brief: n/a
- Parameters: none

### themis::transaction::test::CrossShardFaultInjector

#### `CrossShardFaultInjector(int shard_count)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:287
- Brief: n/a
- Parameters:
  - `shard_count` (int): n/a

#### `bool allShardsConsistent(const std::vector< TxnState > &states) const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:305
- Brief: n/a
- Parameters:
  - `states` (const std::vector< TxnState > &): n/a

#### `CommitResult attemptCommit(const std::string &txn_id)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:294
- Brief: n/a
- Parameters:
  - `txn_id` (const std::string &): n/a

#### `void setCrashAtPrepare(bool v)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:291
- Brief: n/a
- Parameters:
  - `v` (bool): n/a

#### `void setNetworkPartition(bool v)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:292
- Brief: n/a
- Parameters:
  - `v` (bool): n/a

#### `int shardCount() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:318
- Brief: n/a
- Parameters: none

### themis::transaction::test::PartialFailureTest

#### `void SetUp() override`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:362
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:366
- Brief: n/a
- Parameters: none

### themis::transaction::test::RetryStormTest

#### `void SetUp() override`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:476
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_saga_orchestration_hardening.cpp`:480
- Brief: n/a
- Parameters: none

### themis::transaction::test::SAGACircuitBreaker

#### `SAGACircuitBreaker(int failure_threshold=5)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:133
- Brief: n/a
- Parameters:
  - `failure_threshold` (int): n/a

#### `bool canAttempt() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:138
- Brief: n/a
- Parameters: none

#### `int failureCount() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:139
- Brief: n/a
- Parameters: none

#### `void halfOpen()`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:157
- Brief: n/a
- Parameters: none

#### `void recordFailure()`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:141
- Brief: n/a
- Parameters: none

#### `void recordSuccess()`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:150
- Brief: n/a
- Parameters: none

#### `CircuitState state() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:137
- Brief: n/a
- Parameters: none

### themis::transaction::test::TimeoutDetectionTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:56
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:61
- Brief: n/a
- Parameters: none

### themis::transaction::test::TimeoutDeterminismTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:159
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:164
- Brief: n/a
- Parameters: none

### themis::transaction::test::TimeoutEdgeCaseTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:399
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/transaction/test_transaction_timeout_determinism.cpp`:403
- Brief: n/a
- Parameters: none

### themis::transaction::test::TransactionWaveARecoveryTest

#### `void SetUp() override`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:333
- Brief: n/a
- Parameters: none

### themis::transaction::test::WALReplaySimulator

#### `WALReplaySimulator(int in_flight_count)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:72
- Brief: n/a
- Parameters:
  - `in_flight_count` (int): n/a

#### `bool anyInDoubt() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:108
- Brief: n/a
- Parameters: none

#### `int inFlightCount() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:118
- Brief: n/a
- Parameters: none

#### `void markPreparedPrefix(int count)`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:100
- Brief: n/a
- Parameters:
  - `count` (int): n/a

#### `int replayAndResolve()`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:87
- Brief: Replay the WAL and resolve all in-doubt transactions.
- Parameters: none
- Return: Number of transactions resolved (committed or aborted).
- Details: Number of transactions resolved (committed or aborted).

#### `const std::vector< TxnRecord > & wal() const`
- Source: `tests/transaction/test_transaction_wave_a_closure.cpp`:117
- Brief: n/a
- Parameters: none

### transaction_semantic_advisor_example.cpp

#### `int main()`
- Source: `include/transaction/examples/transaction_semantic_advisor_example.cpp`:76
- Brief: Main.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: buildConflictingBatch(), size(), PLANNED(), setDeadlockPredictor(), std::chrono::high_resolution_clock::now(), analyzeBatch(), count(), assert().

