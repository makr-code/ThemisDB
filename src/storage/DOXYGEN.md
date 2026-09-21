# STORAGE DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\storage\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\storage\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 164
- Compounds: 547
- Classes/Structs: 307
- Namespaces: 64
- File Compounds: 164

## Namespaces
- @041270124372221233365030045160276077305042116252
- @116015205123227314263177340074102366034211150137
- @131343215174203240037223377125177066034103337150
- @134003120000114207317027144250364366023241330265
- @137204263256331022336327204035365120375273204243
- @206054336337360303040277106300252203132056037150
- @213342300022330114247073303300007211233202017127
- @227027270336322322134265364032151045077100324215
- @272036036356016013262213167036321062310375305272
- @307041050114267364245276327245076071042316031314
- @325204060107134003325351111317046021257216372303
- @337261040162171103305317363005355050054220024024
- @363333141022121005012073156055334106336262047334
- analytics
- benchmark
- rocksdb
- std
- std::chrono_literals
- testing
- themis
- themis::@155370034123005054270240263075320123044354122303
- themis::@213205123372334330246106346147015127211124107174
- themis::@251254363166073323143062122134203043036342177007
- themis::@266013254361111353362005001217313175213150106237
- themis::@300074244007302062350370360044362171241047234175
- themis::@330314355053236041145254374302071123107104213063
- themis::bench
- themis::bench::sgrg
- themis::bench::sgrg::@347122172307335145253136034357161241034054144321
- themis::bench::st_dg
- themis::compression
- themis::graph
- themis::llm
- themis::memory
- themis::query
- themis::rcu
- themis::sharding
- themis::sharding::@330026040257005125046023270376226154002370045122
- themis::storage
- themis::storage::@030072065075070010340112142372035213056023370205
- themis::storage::@033204303115343356007142270017377040256242001254
- themis::storage::@105362251035176257121120270250125066121041352311
- themis::storage::@106316111355305307175233066067365231364313047374
- themis::storage::@137365311161104162152151223304122230077324256167
- themis::storage::@161114371265054033317215010065315372027373214102
- themis::storage::@204324247163052324104233060264014361247216010262
- themis::storage::@234240222167361000063164343061310206375236005364
- themis::storage::@250305006212222253314343202272223105013101344324
- themis::storage::@256106206202137347101163314040043304226142164206
- themis::storage::@263140105047072365367125046155241322072167232175
- themis::storage::@264320353003100246272060334323052362010213107315
- themis::storage::@333133303127240272242130076356223006306172265302
- themis::storage::@355005163004321244277221341104206106220013301116
- themis::storage::disk_utils
- themis::storage::test
- themis::tensor
- themis::tensor::@375025264341243074267131200042174167253260340145
- themis::test
- themis::test::wave_d
- themis::transaction
- themisdb
- themisdb::storage
- themisdb::storage::@150055135130030122331034004036165123166210062130
- themisdb::temporal

## Types
### Classes
- BatchInsertBenchmark
- CRUDFixture
- LatencyBenchmark
- MVCCChainPrunerTest
- MVCCFixture
- ProfiledInsertFixture
- RecoveryFaultHandlerTest
- StorageEngineErrorHandlingTest
- StorageEngineMoveSemanticTest
- StorageEngineProdTest
- StorageEngineProductionGuardTest
- StorageErrorDiagnosticsTest
- StorageFuzzTest
- StorageParquetExporterFocusedTests
- StoragePhase3IntegrationTest
- StoragePressureManagerTest
- StorageQueryIndexExplicitDITest
- TTQuantizerTest
- TensorNetworkStorageEngineTest
- TensorTrainDecomposerTest
- rocksdb::ColumnFamilyHandle
- rocksdb::DB
- rocksdb::EventListener
- rocksdb::Iterator
- rocksdb::Snapshot
- rocksdb::Transaction
- rocksdb::TransactionDB
- rocksdb::WriteBatch
- rocksdb::WriteBatchWithIndex
- themis::AdaptiveCompactionScheduler
- themis::AppendMergeOperator
- themis::BackupManager
- themis::BaseEntity
- themis::BatchWriteOptimizer
- themis::CompactionManager
- themis::ConflictManager
- themis::CounterMergeOperator
- themis::HistoryManager
- themis::HybridLogicalClock
- themis::IIndexAnalysisAdvisor
- themis::IndexAnalyzer
- themis::IndexMaintenanceManager
- themis::KeySchema
- themis::MVCCChainPruner
- themis::MVCCStore
- themis::MaxMergeOperator
- themis::PITRManager
- themis::RaftMvccBridge
- themis::RocksDBWrapper
- themis::RocksDBWrapper::OperationGuard
- themis::RocksDBWrapper::SafeIterator
- themis::RocksDBWrapper::TransactionWrapper
- themis::RocksDBWrapper::WriteBatchWithIndexWrapper
- themis::RocksDBWrapper::WriteBatchWrapper
- themis::ScopedFileDescriptor
- themis::SetMergeOperator
- themis::StorageAuditLogger
- themis::StorageEngine
- themis::StreamingIngestManager
- themis::WALStorage
- themis::WomTree
- themis::bench::st_dg::BenchKVIndex
- themis::bench::st_dg::BenchWAL
- themis::compression::CompressionStrategyManager
- themis::compression::DeltaCodec
- themis::compression::RLECodec
- themis::compression::SimpleDictionaryCodec
- themis::storage::AccessTracker
- themis::storage::AzureBlobBackend
- themis::storage::BitPackingCodec
- themis::storage::BlobStorageManager
- themis::storage::ColumnCompressedStorage
- themis::storage::ColumnSegment
- themis::storage::ColumnarCache
- themis::storage::ColumnarFormatManager
- themis::storage::CompressedStorageWrapper
- themis::storage::CompressedStorageWrapper::IStorageBackend
- themis::storage::ConcurrentWriteController
- themis::storage::ConnectionKeepalive
- themis::storage::ConnectionTimeoutGuard
- themis::storage::DatabaseConnectionManager
- themis::storage::DatabaseConnectionManager::Connection
- themis::storage::DictionaryCodec
- themis::storage::DiskSpaceGuard
- themis::storage::DiskSpaceMonitor
- themis::storage::DistributedTransaction
- themis::storage::DistributedTransactionManager
- themis::storage::EncryptedBlobBackend
- themis::storage::ExponentialBackoff
- themis::storage::FederatedBlobRouter
- themis::storage::FilesystemBlobBackend
- themis::storage::FrameOfReferenceCodec
- themis::storage::GCSBlobBackend
- themis::storage::GGUFMetadata
- themis::storage::GenericCompressionCodec
- themis::storage::GpuCompressionImpl
- themis::storage::GpuCompressionManager
- themis::storage::HierarchicalTuckerDecomposer
- themis::storage::IBlobStorageBackend
- themis::storage::IDistributedShardParticipant
- themis::storage::IEncryptionKeyProvider
- themis::storage::ITensorStorageBackend
- themis::storage::IVectorIndexBackend
- themis::storage::InMemoryTensorBackend
- themis::storage::InMemoryVectorIndex
- themis::storage::MmapBlobView
- themis::storage::NVMeManager
- themis::storage::NlpMetadataExtractor
- themis::storage::PinGuard
- themis::storage::RLECodec
- themis::storage::RecoveryFaultHandler
- themis::storage::RocksDBTensorBackend
- themis::storage::S3BlobBackend
- themis::storage::SIMDColumnFilter
- themis::storage::SchemaDeadWeightDetector
- themis::storage::SchemaMigrator
- themis::storage::SecuritySignatureManager
- themis::storage::StaticKeyProvider
- themis::storage::StorageLayoutAdvisor
- themis::storage::StorageParquetExporter
- themis::storage::StoragePressureManager
- themis::storage::TTQuantizer
- themis::storage::TensorCompactionFilter
- themis::storage::TensorNetworkStorageEngine
- themis::storage::TensorRouter
- themis::storage::TensorTrainDecomposer
- themis::storage::TieredStorageManager
- themis::storage::WebDAVBlobBackend
- themis::storage::WriteGuard
- themis::storage::ZeroCopyBlobTransfer
- themisdb::storage::BlobRedundancyManager
- themisdb::storage::ErasureCodingBackend
- themisdb::storage::RocksDBBlobListener
- themisdb::storage::TransactionRetryManager

### Structs
- rocksdb::Options
- rocksdb::ReadOptions
- rocksdb::TransactionDBOptions
- rocksdb::TransactionOptions
- rocksdb::WriteOptions
- std::hash< themis::storage::SegmentKey >
- themis::AdaptiveCompactionScheduler::AdaptedConfig
- themis::AdaptiveCompactionScheduler::CompactionImpactPrediction
- themis::AdaptiveCompactionScheduler::Config
- themis::AdaptiveCompactionScheduler::IOSample
- themis::AdaptiveCompactionScheduler::Stats
- themis::BackupManager::Config
- themis::BackupManager::ScheduledBackupEntry
- themis::BackupOptions
- themis::BatchWriteOptimizer::Config
- themis::BatchWriteOptimizer::Stats
- themis::CompactionManager::Config
- themis::CompactionManager::Stats
- themis::ConflictRecord
- themis::ConflictSet
- themis::FileIntegrityInfo
- themis::FragmentationMetrics
- themis::HLCTimestamp
- themis::HistoryRecord
- themis::IndexAnalysisReport
- themis::IndexAnalyzeConfig
- themis::IndexEntry
- themis::MVCCChainPruner::Config
- themis::MVCCChainPruner::PruneStats
- themis::MVCCStore::GCOptions
- themis::MVCCStore::VersionEntry
- themis::MaintenanceJobStatus
- themis::MaintenancePolicy
- themis::PITRManager::RestoreOptions
- themis::PITRManager::RestorePreview
- themis::PITRManager::RestoreProgress
- themis::PITRManager::Status
- themis::PITROptions
- themis::RAIDConfig
- themis::RaftMvccBridge::LinearizableResult
- themis::RecoveryStats
- themis::RocksDBWrapper::CFInfo
- themis::RocksDBWrapper::Config
- themis::RocksDBWrapper::Config::DbPath
- themis::RocksDBWrapper::KeyValuePair
- themis::ShardInfo
- themis::StorageAuditLogger::Config
- themis::StorageEngine::IOMetrics
- themis::StorageEngine::ScanCounters
- themis::StreamingIngestManager::Config
- themis::StreamingIngestManager::Event
- themis::StreamingIngestManager::Stats
- themis::TierThresholds
- themis::WALStorage::BatchEntry
- themis::WALStorage::Config
- themis::WALStorage::Entry
- themis::WomTree::Config
- themis::WomTree::Impl
- themis::WomTree::Stats
- themis::bench::st_dg::BenchCompactionChecker
- themis::bench::st_dg::BenchWAL::Entry
- themis::bench::st_dg::IndexSeeder
- themis::compression::CompressionConfig
- themis::compression::CompressionResult
- themis::storage::AccessTracker::Entry
- themis::storage::BlobRef
- themis::storage::BlobStorageConfig
- themis::storage::CollectionAccessStats
- themis::storage::ColumnMetadata
- themis::storage::ColumnPredicate
- themis::storage::ColumnarCache::Config
- themis::storage::ColumnarCache::Entry
- themis::storage::ColumnarFormatManager::CompressionStats
- themis::storage::CompressedValue
- themis::storage::ConcurrentWriteController::Waiter
- themis::storage::ConcurrentWriteControllerConfig
- themis::storage::ConcurrentWriteStats
- themis::storage::DatabaseConnectionManager::ConnectionConfig
- themis::storage::DatabaseConnectionManager::ConnectionHealth
- themis::storage::DatabaseConnectionManager::ConnectionStats
- themis::storage::DecompositionStats
- themis::storage::DiskSpaceMonitor::Config
- themis::storage::DiskSpaceMonitor::MonitorStats
- themis::storage::DiskSpaceMonitor::SpaceInfo
- themis::storage::DiskSpaceMonitor::UsageSnapshot
- themis::storage::DistributedOperation
- themis::storage::DistributedTransaction::PrivateTag
- themis::storage::DistributedTransactionManager::ShardConfig
- themis::storage::DistributedTransactionManager::Statistics
- themis::storage::DistributedTxnConfig
- themis::storage::EncryptionStats
- themis::storage::ExponentialBackoff::Config
- themis::storage::FederatedBlobReplicaTarget
- themis::storage::FederatedBlobRoute
- themis::storage::FederatedBlobWritePlan
- themis::storage::GCSBlobBackend::Impl
- themis::storage::GdprFieldRegistry
- themis::storage::GpuCompressionConfig
- themis::storage::GpuCompressionManager::Stats
- themis::storage::GpuCompressionResult
- themis::storage::HTConfig
- themis::storage::HierarchicalTuckerDecomposer::Stats
- themis::storage::KnnResult
- themis::storage::ManagerSharedState
- themis::storage::MigrationOp
- themis::storage::MigrationResult
- themis::storage::NVMeCapabilities
- themis::storage::NVMeConfig
- themis::storage::NVMeIORequest
- themis::storage::NVMeIOResult
- themis::storage::NVMeManager::IoUringState
- themis::storage::NlpMetadataExtractor::Config
- themis::storage::NlpMetadataExtractor::ExtractedMetadata
- themis::storage::ParquetColumnDesc
- themis::storage::ParquetExportConfig
- themis::storage::PartitionInfo
- themis::storage::ProvenanceRecord
- themis::storage::QuantizedCore
- themis::storage::QuantizedTrain
- themis::storage::RecoveryFaultReport
- themis::storage::SIMDFilterStats
- themis::storage::SchemaDeadWeightDetector::Config
- themis::storage::SchemaDeadWeightDetector::DeadWeightCandidate
- themis::storage::SchemaDeadWeightDetector::DeadWeightReport
- themis::storage::SchemaInfo
- themis::storage::SchemaMigrator::Config
- themis::storage::SecuritySignature
- themis::storage::SecuritySignatureManager::Options
- themis::storage::SecuritySignatureManager::VerifyAllResult
- themis::storage::SegmentKey
- themis::storage::StorageCapacityMetrics
- themis::storage::StorageErrorContext
- themis::storage::StorageLayoutAdvisor::LayoutRecommendation
- themis::storage::StorageParquetExporter::ExportStats
- themis::storage::TTCore
- themis::storage::TTTrain
- themis::storage::TensorFieldKey
- themis::storage::TensorFieldKeyHash
- themis::storage::TensorRouteHint
- themis::storage::TensorRouter::DataProfile
- themis::storage::TensorRouter::Impl
- themis::storage::TensorRouter::Impl::PilotResult
- themis::storage::TensorRouter::RouterStats
- themis::storage::TensorRouter::TemplateValidationResult
- themis::storage::TensorRoutingPolicy
- themis::storage::TensorStorageConfig
- themis::storage::TensorStorageStats
- themis::storage::TensorTrainConfig
- themis::storage::TieredStorageConfig
- themis::storage::TieredStorageManager::Stats
- themis::storage::VectorIndexConfig
- themis::storage::WebDAVBlobBackend::ReadData
- themis::storage::ZeroCopyTransferConfig
- themis::storage::ZeroCopyTransferStats
- themis::storage::ZoneMap
- themis::test::wave_d::CompactableShard
- themis::test::wave_d::KVStore
- themis::test::wave_d::TieredBlobStore
- themis::test::wave_d::TieredBlobStore::BlobEntry
- themisdb::storage::BlobLocation
- themisdb::storage::BlobMetadata
- themisdb::storage::BlobRedundancyConfig
- themisdb::storage::BlobRedundancyManager::Config
- themisdb::storage::BlobRedundancyStats
- themisdb::storage::CollectionRedundancyConfig
- themisdb::storage::EncodedShard
- themisdb::storage::ErasureCodingBackend::BlobEntry
- themisdb::storage::ErasureCodingConfig
- themisdb::storage::GeoTarget
- themisdb::storage::RetryPolicy
- themisdb::storage::RetryStatistics
- themisdb::storage::TierConfig
- themisdb::storage::TransactionRetryConfig

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 1851

### BatchInsertBenchmark

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/storage/bench_batch_insert.cpp`:17
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/storage/bench_batch_insert.cpp`:54
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `BaseEntity createTestEntity(int id)`
- Source: `benchmarks/storage/bench_batch_insert.cpp`:65
- Brief: n/a
- Parameters:
  - `id` (int): n/a

### CRUDFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/storage/bench_crud.cpp`:32
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/storage/bench_crud.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

### LatencyBenchmark

#### `void SetUp() override`
- Source: `tests/storage/test_storage_latency_bench.cpp`:34
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/storage/test_storage_latency_bench.cpp`:54
- Brief: n/a
- Parameters: none

#### `std::vector< int64_t > measureLatencies(int n, Op op)`
- Source: `tests/storage/test_storage_latency_bench.cpp`:67
- Brief: n/a
- Parameters:
  - `n` (int): n/a
  - `op` (Op): n/a

#### `int64_t percentile(const std::vector< int64_t > &sorted, double pct)`
- Source: `tests/storage/test_storage_latency_bench.cpp`:81
- Brief: n/a
- Parameters:
  - `sorted` (const std::vector< int64_t > &): n/a
  - `pct` (double): n/a

### MVCCChainPrunerTest

#### `void SetUp() override`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:39
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:63
- Brief: n/a
- Parameters: none

#### `size_t countMvccVersions(const std::string &key)`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:87
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `size_t countTierVersions(const std::string &table, const std::string &key)`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:97
- Brief: n/a
- Parameters:
  - `table` (const std::string &): n/a
  - `key` (const std::string &): n/a

#### `std::vector< HLCTimestamp > writeVersions(const std::string &key, const std::vector< std::string > &values)`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:74
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `values` (const std::vector< std::string > &): n/a

### MVCCFixture

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/storage/bench_mvcc.cpp`:15
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &state) override`
- Source: `benchmarks/storage/bench_mvcc.cpp`:32
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `BaseEntity createTestEntity(const std::string &pk, int age)`
- Source: `benchmarks/storage/bench_mvcc.cpp`:39
- Brief: n/a
- Parameters:
  - `pk` (const std::string &): n/a
  - `age` (int): n/a

### ProfiledInsertFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/storage/bench_insert_profiling.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/storage/bench_insert_profiling.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

### StorageEngineErrorHandlingTest

#### `void SetUp() override`
- Source: `tests/storage/test_storage_engine_di.cpp`:48
- Brief: n/a
- Parameters: none

### StorageEngineMoveSemanticTest

#### `void SetUp() override`
- Source: `tests/storage/test_storage_engine_move_semantics.cpp`:20
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/storage/test_storage_engine_move_semantics.cpp`:35
- Brief: n/a
- Parameters: none

### StorageEngineProdTest

#### `void SetUp() override`
- Source: `tests/storage/test_storage_engine_prod.cpp`:29
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/storage/test_storage_engine_prod.cpp`:42
- Brief: n/a
- Parameters: none

#### `void insertRange(const std::string &prefix, int from, int to)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:50
- Brief: n/a
- Parameters:
  - `prefix` (const std::string &): n/a
  - `from` (int): n/a
  - `to` (int): n/a

### StorageEngineProductionGuardTest

#### `void SetUp() override`
- Source: `tests/storage/test_storage_engine_di.cpp`:167
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/storage/test_storage_engine_di.cpp`:185
- Brief: n/a
- Parameters: none

### StorageFuzzTest

#### `void SetUp() override`
- Source: `tests/storage/test_storage_fuzz.cpp`:33
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/storage/test_storage_fuzz.cpp`:40
- Brief: n/a
- Parameters: none

#### `WALStorage::Config makeCfg(uint64_t rotate_bytes=4096)`
- Source: `tests/storage/test_storage_fuzz.cpp`:44
- Brief: n/a
- Parameters:
  - `rotate_bytes` (uint64_t): n/a

### StorageParquetExporterFocusedTests

#### `void SetUp() override`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:77
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:84
- Brief: n/a
- Parameters: none

### StorageQueryIndexExplicitDITest

#### `void SetUp() override`
- Source: `tests/storage/test_storage_query_index_explicit_di.cpp`:34
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/storage/test_storage_query_index_explicit_di.cpp`:66
- Brief: n/a
- Parameters: none

### TensorNetworkStorageEngineTest

#### `void SetUp() override`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:313
- Brief: n/a
- Parameters: none

### bench_batch_insert.cpp

#### `BENCHMARK_F(BatchInsertBenchmark, BatchInsert_100)(benchmark`
- Source: `benchmarks/storage/bench_batch_insert.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchInsertBenchmark): n/a
  - `<unnamed>` (BatchInsert_100): n/a

#### `BENCHMARK_F(BatchInsertBenchmark, BatchInsert_1000)(benchmark`
- Source: `benchmarks/storage/bench_batch_insert.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchInsertBenchmark): n/a
  - `<unnamed>` (BatchInsert_1000): n/a

#### `BENCHMARK_F(BatchInsertBenchmark, SingleInserts_100)(benchmark`
- Source: `benchmarks/storage/bench_batch_insert.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchInsertBenchmark): n/a
  - `<unnamed>` (SingleInserts_100): n/a

#### `BENCHMARK_F(BatchInsertBenchmark, SingleInserts_1000)(benchmark`
- Source: `benchmarks/storage/bench_batch_insert.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (BatchInsertBenchmark): n/a
  - `<unnamed>` (SingleInserts_1000): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/storage/bench_batch_insert.cpp`:182
- Brief: n/a
- Parameters: none

### bench_crud.cpp

#### `BENCHMARK_DEFINE_F(CRUDFixture, FulltextSearch)(benchmark`
- Source: `benchmarks/storage/bench_crud.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (CRUDFixture): n/a
  - `<unnamed>` (FulltextSearch): n/a

#### `BENCHMARK_DEFINE_F(CRUDFixture, InsertWithAllIndexes)(benchmark`
- Source: `benchmarks/storage/bench_crud.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (CRUDFixture): n/a
  - `<unnamed>` (InsertWithAllIndexes): n/a

#### `BENCHMARK_DEFINE_F(CRUDFixture, LookupBySecondaryIndex)(benchmark`
- Source: `benchmarks/storage/bench_crud.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (CRUDFixture): n/a
  - `<unnamed>` (LookupBySecondaryIndex): n/a

#### `BENCHMARK_DEFINE_F(CRUDFixture, RangeScanAge)(benchmark`
- Source: `benchmarks/storage/bench_crud.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (CRUDFixture): n/a
  - `<unnamed>` (RangeScanAge): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/storage/bench_crud.cpp`:140
- Brief: n/a
- Parameters: none

#### `Unit(benchmark::kMicrosecond) -> UseRealTime()`
- Source: `benchmarks/storage/bench_crud.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

#### `Unit(benchmark::kMillisecond) -> UseRealTime()`
- Source: `benchmarks/storage/bench_crud.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

### bench_insert_profiling.cpp

#### `BENCHMARK_DEFINE_F(ProfiledInsertFixture, IndexInsert_AllIndexes)(benchmark`
- Source: `benchmarks/storage/bench_insert_profiling.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfiledInsertFixture): n/a
  - `<unnamed>` (IndexInsert_AllIndexes): n/a

#### `BENCHMARK_DEFINE_F(ProfiledInsertFixture, IndexInsert_RegularOnly)(benchmark`
- Source: `benchmarks/storage/bench_insert_profiling.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfiledInsertFixture): n/a
  - `<unnamed>` (IndexInsert_RegularOnly): n/a

#### `BENCHMARK_DEFINE_F(ProfiledInsertFixture, RawRocksDBPut)(benchmark`
- Source: `benchmarks/storage/bench_insert_profiling.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfiledInsertFixture): n/a
  - `<unnamed>` (RawRocksDBPut): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/storage/bench_insert_profiling.cpp`:181
- Brief: n/a
- Parameters: none

#### `BENCHMARK_REGISTER_F(ProfiledInsertFixture, IndexInsert_AllIndexes) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/storage/bench_insert_profiling.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfiledInsertFixture): n/a
  - `<unnamed>` (IndexInsert_AllIndexes): n/a

#### `BENCHMARK_REGISTER_F(ProfiledInsertFixture, IndexInsert_RegularOnly) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/storage/bench_insert_profiling.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfiledInsertFixture): n/a
  - `<unnamed>` (IndexInsert_RegularOnly): n/a

#### `BENCHMARK_REGISTER_F(ProfiledInsertFixture, RawRocksDBPut) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/storage/bench_insert_profiling.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfiledInsertFixture): n/a
  - `<unnamed>` (RawRocksDBPut): n/a

### bench_mvcc.cpp

#### `BENCHMARK_F(MVCCFixture, BatchInsert100_MVCC)(benchmark`
- Source: `benchmarks/storage/bench_mvcc.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCFixture): n/a
  - `<unnamed>` (BatchInsert100_MVCC): n/a

#### `BENCHMARK_F(MVCCFixture, BatchInsert100_WriteBatch)(benchmark`
- Source: `benchmarks/storage/bench_mvcc.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCFixture): n/a
  - `<unnamed>` (BatchInsert100_WriteBatch): n/a

#### `BENCHMARK_F(MVCCFixture, InsertWithMultipleIndexes_MVCC)(benchmark`
- Source: `benchmarks/storage/bench_mvcc.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCFixture): n/a
  - `<unnamed>` (InsertWithMultipleIndexes_MVCC): n/a

#### `BENCHMARK_F(MVCCFixture, Rollback_MVCC)(benchmark`
- Source: `benchmarks/storage/bench_mvcc.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCFixture): n/a
  - `<unnamed>` (Rollback_MVCC): n/a

#### `BENCHMARK_F(MVCCFixture, SingleEntityCommit_MVCC)(benchmark`
- Source: `benchmarks/storage/bench_mvcc.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCFixture): n/a
  - `<unnamed>` (SingleEntityCommit_MVCC): n/a

#### `BENCHMARK_F(MVCCFixture, SingleEntityCommit_WriteBatch)(benchmark`
- Source: `benchmarks/storage/bench_mvcc.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCFixture): n/a
  - `<unnamed>` (SingleEntityCommit_WriteBatch): n/a

#### `BENCHMARK_F(MVCCFixture, SnapshotIsolationOverhead_MVCC)(benchmark`
- Source: `benchmarks/storage/bench_mvcc.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCFixture): n/a
  - `<unnamed>` (SnapshotIsolationOverhead_MVCC): n/a

#### `BENCHMARK_F(MVCCFixture, UpdateWithIndexes_MVCC)(benchmark`
- Source: `benchmarks/storage/bench_mvcc.cpp`:178
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCFixture): n/a
  - `<unnamed>` (UpdateWithIndexes_MVCC): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/storage/bench_mvcc.cpp`:202
- Brief: n/a
- Parameters: none

### bench_simple_insert_test.cpp

#### `BENCHMARK(BM_SimpleInsert)`
- Source: `benchmarks/storage/bench_simple_insert_test.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_SimpleInsert): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/storage/bench_simple_insert_test.cpp`:78
- Brief: n/a
- Parameters: none

#### `void BM_SimpleInsert(benchmark::State &state)`
- Source: `benchmarks/storage/bench_simple_insert_test.cpp`:11
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_ssm_phase0_baseline.cpp

#### `Arg(100) -> Arg(1000) ->Arg(10000) ->Unit(benchmark::kMillisecond) ->Iterations(10)`
- Source: `benchmarks/storage/bench_ssm_phase0_baseline.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Arg(512) -> Arg(2048) ->Arg(8192) ->Unit(benchmark::kMillisecond) ->Iterations(10)`
- Source: `benchmarks/storage/bench_ssm_phase0_baseline.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (512): n/a

#### `Args({1, 512}) -> Args({1, 2048}) ->Args({10, 512}) ->Args({10, 2048}) ->Unit(benchmark::kMillisecond) ->Iterations(5)`
- Source: `benchmarks/storage/bench_ssm_phase0_baseline.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` ({1, 512}): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/storage/bench_ssm_phase0_baseline.cpp`:242
- Brief: n/a
- Parameters: none

#### `void bench_context_quality_metrics_computation(benchmark::State &state)`
- Source: `benchmarks/storage/bench_ssm_phase0_baseline.cpp`:165
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Measurement Point: Phase 1 observability baseline for Agentic Memory layer transitions. Purpose: Establish baseline cost of context quality scoring before Phase 3 optimization.

#### `void bench_drift_metrics_prometheus_export(benchmark::State &state)`
- Source: `benchmarks/storage/bench_ssm_phase0_baseline.cpp`:211
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Measurement Point: Phase 1 observability baseline for factual drift scoring. Purpose: Establish baseline cost of Prometheus metric export before Phase 3 distributed observability.

#### `void bench_infini_attention_cpu_baseline(benchmark::State &state)`
- Source: `benchmarks/storage/bench_ssm_phase0_baseline.cpp`:69
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: SIMULATION NOTE (Stub Path): Purpose: Measure CPU Infini-attention synthetic latency and VRAM footprint during Phase 1 Activation: Benchmark execution; InfiniAttentionCpuFallback invoked Production Delta: Actual CUDA kernel measurements will replace these synthetic metrics in Phase 2 Removal Plan: Superseded by P2-D02 CUDA kernel benchmarks (Q4/2026)

#### `void bench_ssm_stub_checkpoint_resume(benchmark::State &state)`
- Source: `benchmarks/storage/bench_ssm_phase0_baseline.cpp`:121
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: SIMULATION NOTE (Stub Path): Purpose: Measure synthetic SSM state checkpoint/resume cycle during Phase 1 Activation: Benchmark execution; SyntheticSSMStub invoked Production Delta: Real Mamba SSM backend will have different latency/VRAM profile in Phase 2+ Removal Plan: Superseded by real Mamba plugin (P1-D03 replaced in P2-D03 with real model)

### bench_storage_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:235
- Brief: n/a
- Parameters: none

### bench_storage_performance.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/storage/bench_storage_performance.cpp`:775
- Brief: n/a
- Parameters: none

### bench_storage_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/storage/bench_storage_release_gates.cpp`:377
- Brief: n/a
- Parameters: none

### bench_wal_stress.cpp

#### `BENCHMARK(BM_WAL_NoSync) -> Args({1, 64}) ->Args({4, 64}) ->Args({8, 64}) ->Args({16, 64}) ->UseRealTime()`
- Source: `benchmarks/storage/bench_wal_stress.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_WAL_NoSync): n/a

#### `BENCHMARK(BM_WAL_Sync) -> Args({1, 64}) ->Args({4, 64}) ->Args({8, 64}) ->Args({16, 64}) ->UseRealTime()`
- Source: `benchmarks/storage/bench_wal_stress.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_WAL_Sync): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/storage/bench_wal_stress.cpp`:71
- Brief: n/a
- Parameters: none

#### `void BM_WAL_NoSync(benchmark::State &state)`
- Source: `benchmarks/storage/bench_wal_stress.cpp`:66
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_WAL_Sync(benchmark::State &state)`
- Source: `benchmarks/storage/bench_wal_stress.cpp`:65
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### schema_layout_advisor_example.cpp

#### `int main()`
- Source: `include/storage/examples/schema_layout_advisor_example.cpp`:79
- Brief: n/a
- Parameters: none

### std::hash< themis::storage::SegmentKey >

#### `size_t operator()(const themis::storage::SegmentKey &k) const noexcept`
- Source: `include/storage/columnar_cache.h`:54
- Brief: n/a
- Parameters:
  - `k` (const themis::storage::SegmentKey &): n/a

### test_blob_backend_cloud_integration_focused.cpp

#### `TEST(GCSBlobBackend, FakeGcsIntegrationRoundTripWhenConfigured)`
- Source: `tests/storage/test_blob_backend_cloud_integration_focused.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (GCSBlobBackend): n/a
  - `<unnamed>` (FakeGcsIntegrationRoundTripWhenConfigured): n/a

### test_blob_backend_gcs_focused.cpp

#### `TEST(GCSBlobBackend, GCS01_PutReturnsErrorWhenUnavailable)`
- Source: `tests/storage/test_blob_backend_gcs_focused.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (GCSBlobBackend): n/a
  - `<unnamed>` (GCS01_PutReturnsErrorWhenUnavailable): n/a

#### `TEST(GCSBlobBackend, GCS02_GetReturnsErrorWhenUnavailable)`
- Source: `tests/storage/test_blob_backend_gcs_focused.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (GCSBlobBackend): n/a
  - `<unnamed>` (GCS02_GetReturnsErrorWhenUnavailable): n/a

#### `TEST(GCSBlobBackend, GCS03_RemoveReturnsErrorWhenUnavailable)`
- Source: `tests/storage/test_blob_backend_gcs_focused.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (GCSBlobBackend): n/a
  - `<unnamed>` (GCS03_RemoveReturnsErrorWhenUnavailable): n/a

#### `TEST(GCSBlobBackend, GCS04_ExistsReturnsFalseWhenUnavailable)`
- Source: `tests/storage/test_blob_backend_gcs_focused.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (GCSBlobBackend): n/a
  - `<unnamed>` (GCS04_ExistsReturnsFalseWhenUnavailable): n/a

#### `TEST(GCSBlobBackend, GCS05_NameIsGcs)`
- Source: `tests/storage/test_blob_backend_gcs_focused.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (GCSBlobBackend): n/a
  - `<unnamed>` (GCS05_NameIsGcs): n/a

#### `TEST(GCSBlobBackend, GCS06_IsAvailableFalseWithoutCredentials)`
- Source: `tests/storage/test_blob_backend_gcs_focused.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (GCSBlobBackend): n/a
  - `<unnamed>` (GCS06_IsAvailableFalseWithoutCredentials): n/a

### test_mvcc_chain_pruner.cpp

#### `TEST_F(MVCCChainPrunerTest, PruneAllMigratesMultipleKeys)`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCChainPrunerTest): n/a
  - `<unnamed>` (PruneAllMigratesMultipleKeys): n/a

#### `TEST_F(MVCCChainPrunerTest, PruneAllSafeIsNoOpWithZeroHorizon)`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCChainPrunerTest): n/a
  - `<unnamed>` (PruneAllSafeIsNoOpWithZeroHorizon): n/a

#### `TEST_F(MVCCChainPrunerTest, PruneKeyHandlesBinaryValues)`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCChainPrunerTest): n/a
  - `<unnamed>` (PruneKeyHandlesBinaryValues): n/a

#### `TEST_F(MVCCChainPrunerTest, PruneKeyMigratesVersionsBelowHorizon)`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCChainPrunerTest): n/a
  - `<unnamed>` (PruneKeyMigratesVersionsBelowHorizon): n/a

#### `TEST_F(MVCCChainPrunerTest, PruneKeyNoOpWhenHorizonBeforeAllVersions)`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCChainPrunerTest): n/a
  - `<unnamed>` (PruneKeyNoOpWhenHorizonBeforeAllVersions): n/a

#### `TEST_F(MVCCChainPrunerTest, PruneKeyRespectsMinVersionsToKeep)`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCChainPrunerTest): n/a
  - `<unnamed>` (PruneKeyRespectsMinVersionsToKeep): n/a

#### `TEST_F(MVCCChainPrunerTest, PruneKeySetsSysTimeCorrectly)`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCChainPrunerTest): n/a
  - `<unnamed>` (PruneKeySetsSysTimeCorrectly): n/a

#### `TEST_F(MVCCChainPrunerTest, SetSafeHorizonIsMonotoneAndPruneAllSafeUsesIt)`
- Source: `tests/storage/test_mvcc_chain_pruner.cpp`:266
- Brief: n/a
- Parameters:
  - `<unnamed>` (MVCCChainPrunerTest): n/a
  - `<unnamed>` (SetSafeHorizonIsMonotoneAndPruneAllSafeUsesIt): n/a

### test_storage_audit_logger.cpp

#### `TEST(StorageAuditTest, AllEventTypes_LogSuccessfully)`
- Source: `tests/storage/test_storage_audit_logger.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageAuditTest): n/a
  - `<unnamed>` (AllEventTypes_LogSuccessfully): n/a

#### `TEST(StorageAuditTest, ConcurrentLogging_NoRaceConditions)`
- Source: `tests/storage/test_storage_audit_logger.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageAuditTest): n/a
  - `<unnamed>` (ConcurrentLogging_NoRaceConditions): n/a

#### `TEST(StorageAuditTest, EventName_AllTypes)`
- Source: `tests/storage/test_storage_audit_logger.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageAuditTest): n/a
  - `<unnamed>` (EventName_AllTypes): n/a

#### `TEST(StorageAuditTest, LogDel_WritesDelLine)`
- Source: `tests/storage/test_storage_audit_logger.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageAuditTest): n/a
  - `<unnamed>` (LogDel_WritesDelLine): n/a

#### `TEST(StorageAuditTest, LogPut_WritesEntryAndIncrementsSequence)`
- Source: `tests/storage/test_storage_audit_logger.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageAuditTest): n/a
  - `<unnamed>` (LogPut_WritesEntryAndIncrementsSequence): n/a

#### `TEST(StorageAuditTest, Open_CreatesSegmentFile)`
- Source: `tests/storage/test_storage_audit_logger.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageAuditTest): n/a
  - `<unnamed>` (Open_CreatesSegmentFile): n/a

#### `TEST(StorageAuditTest, Rotation_NewSegmentCreatedWhenLimitReached)`
- Source: `tests/storage/test_storage_audit_logger.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageAuditTest): n/a
  - `<unnamed>` (Rotation_NewSegmentCreatedWhenLimitReached): n/a

#### `TEST(StorageAuditTest, SegmentCount_StartsAtOne)`
- Source: `tests/storage/test_storage_audit_logger.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageAuditTest): n/a
  - `<unnamed>` (SegmentCount_StartsAtOne): n/a

#### `TEST(StorageAuditTest, SegmentName_Format)`
- Source: `tests/storage/test_storage_audit_logger.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageAuditTest): n/a
  - `<unnamed>` (SegmentName_Format): n/a

#### `TEST(StorageAuditTest, SequenceMonotonicallyIncreases)`
- Source: `tests/storage/test_storage_audit_logger.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageAuditTest): n/a
  - `<unnamed>` (SequenceMonotonicallyIncreases): n/a

#### `std::string auditDir()`
- Source: `tests/storage/test_storage_audit_logger.cpp`:38
- Brief: n/a
- Parameters: none

#### `std::string readFile(const std::string &path)`
- Source: `tests/storage/test_storage_audit_logger.cpp`:33
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### test_storage_contract_hardening_focused.cpp

#### `TEST(StorageContractMvcc, STR05_SnapshotIsolation)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:317
- Brief: STR-05: Snapshot isolation — concurrent writer's uncommitted data not visible.
- Parameters:
  - `<unnamed>` (StorageContractMvcc): n/a
  - `<unnamed>` (STR05_SnapshotIsolation): n/a

#### `TEST(StorageContractMvcc, STR06_ReadYourWrites)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:336
- Brief: STR-06: Read-your-writes within a transaction.
- Parameters:
  - `<unnamed>` (StorageContractMvcc): n/a
  - `<unnamed>` (STR06_ReadYourWrites): n/a

#### `TEST(StorageContractMvcc, STR07_DirtyReadPrevention)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:350
- Brief: STR-07: Dirty-read prevention — uncommitted data from other tx not visible.
- Parameters:
  - `<unnamed>` (StorageContractMvcc): n/a
  - `<unnamed>` (STR07_DirtyReadPrevention): n/a

#### `TEST(StorageContractMvcc, STR08_WriteWriteConflict)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:366
- Brief: STR-08: Write-write conflict → TRANSACTION_CONFLICT (retryable).
- Parameters:
  - `<unnamed>` (StorageContractMvcc): n/a
  - `<unnamed>` (STR08_WriteWriteConflict): n/a

#### `TEST(StorageContractPitr, STR13_ValidTimestampRestoreSucceeds)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:455
- Brief: STR-13: Valid past timestamp → restore succeeds with consistent data.
- Parameters:
  - `<unnamed>` (StorageContractPitr): n/a
  - `<unnamed>` (STR13_ValidTimestampRestoreSucceeds): n/a

#### `TEST(StorageContractPitr, STR14_FutureTimestampInvalid)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:474
- Brief: STR-14: Future timestamp → PITR_INVALID_TIMESTAMP.
- Parameters:
  - `<unnamed>` (StorageContractPitr): n/a
  - `<unnamed>` (STR14_FutureTimestampInvalid): n/a

#### `TEST(StorageContractPitr, STR15_ConcurrentWriteNotInSnapshot)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:488
- Brief: STR-15: Concurrent write during backup does not appear in snapshot. Backup is taken at T0; writes at T1 > T0 must not appear in snapshot.
- Parameters:
  - `<unnamed>` (StorageContractPitr): n/a
  - `<unnamed>` (STR15_ConcurrentWriteNotInSnapshot): n/a

#### `TEST(StorageContractPitr, STR16_RetryableConflictClassification)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:508
- Brief: STR-16: isRetryableConflict() is true only for TRANSACTION_CONFLICT.
- Parameters:
  - `<unnamed>` (StorageContractPitr): n/a
  - `<unnamed>` (STR16_RetryableConflictClassification): n/a

#### `TEST(StorageContractRecovery, STR09_CleanRestart)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:384
- Brief: STR-09: Clean restart replays WAL from last checkpoint.
- Parameters:
  - `<unnamed>` (StorageContractRecovery): n/a
  - `<unnamed>` (STR09_CleanRestart): n/a

#### `TEST(StorageContractRecovery, STR10_PartialWalRecoveryIncomplete)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:399
- Brief: STR-10: Partial WAL tail (torn write) → RECOVERY_INCOMPLETE diagnostic.
- Parameters:
  - `<unnamed>` (StorageContractRecovery): n/a
  - `<unnamed>` (STR10_PartialWalRecoveryIncomplete): n/a

#### `TEST(StorageContractRecovery, STR11_CheckpointOnlyAdvancesFlushed)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:417
- Brief: STR-11: Checkpoint advance only moves past fully-flushed sequences. Simulated: checkpoint seq must not exceed last written seq.
- Parameters:
  - `<unnamed>` (StorageContractRecovery): n/a
  - `<unnamed>` (STR11_CheckpointOnlyAdvancesFlushed): n/a

#### `TEST(StorageContractRecovery, STR12_DurabilityThreatClassification)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:437
- Brief: STR-12: isDurabilityThreat() is true for WAL_WRITE_FAILED and WAL_CORRUPTED.
- Parameters:
  - `<unnamed>` (StorageContractRecovery): n/a
  - `<unnamed>` (STR12_DurabilityThreatClassification): n/a

#### `TEST(StorageContractWal, STR01_WriteBeforeAck)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:248
- Brief: STR-01: Write → WAL committed → ACK ordering verified. ACK is only returned after WAL append succeeds.
- Parameters:
  - `<unnamed>` (StorageContractWal): n/a
  - `<unnamed>` (STR01_WriteBeforeAck): n/a

#### `TEST(StorageContractWal, STR02_ReplayAfterCrash)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:262
- Brief: STR-02: Replay WAL after simulated crash: all entries re-applied.
- Parameters:
  - `<unnamed>` (StorageContractWal): n/a
  - `<unnamed>` (STR02_ReplayAfterCrash): n/a

#### `TEST(StorageContractWal, STR03_IdempotentReplay)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:280
- Brief: STR-03: WAL replay is idempotent: replaying twice gives same state.
- Parameters:
  - `<unnamed>` (StorageContractWal): n/a
  - `<unnamed>` (STR03_IdempotentReplay): n/a

#### `TEST(StorageContractWal, STR04_MonotonicSequenceNumbers)`
- Source: `tests/storage/test_storage_contract_hardening_focused.cpp`:297
- Brief: STR-04: WAL sequence numbers are strictly monotonically increasing.
- Parameters:
  - `<unnamed>` (StorageContractWal): n/a
  - `<unnamed>` (STR04_MonotonicSequenceNumbers): n/a

### test_storage_engine_di.cpp

#### `TEST_F(StorageEngineErrorHandlingTest, DelReturnsResultVoid)`
- Source: `tests/storage/test_storage_engine_di.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineErrorHandlingTest): n/a
  - `<unnamed>` (DelReturnsResultVoid): n/a

#### `TEST_F(StorageEngineErrorHandlingTest, ErrorPropagation)`
- Source: `tests/storage/test_storage_engine_di.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineErrorHandlingTest): n/a
  - `<unnamed>` (ErrorPropagation): n/a

#### `TEST_F(StorageEngineErrorHandlingTest, GetReturnsResultString)`
- Source: `tests/storage/test_storage_engine_di.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineErrorHandlingTest): n/a
  - `<unnamed>` (GetReturnsResultString): n/a

#### `TEST_F(StorageEngineErrorHandlingTest, OpenReturnsResultVoid)`
- Source: `tests/storage/test_storage_engine_di.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineErrorHandlingTest): n/a
  - `<unnamed>` (OpenReturnsResultVoid): n/a

#### `TEST_F(StorageEngineErrorHandlingTest, PutReturnsResultVoid)`
- Source: `tests/storage/test_storage_engine_di.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineErrorHandlingTest): n/a
  - `<unnamed>` (PutReturnsResultVoid): n/a

#### `TEST_F(StorageEngineErrorHandlingTest, ResultBooleanConversion)`
- Source: `tests/storage/test_storage_engine_di.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineErrorHandlingTest): n/a
  - `<unnamed>` (ResultBooleanConversion): n/a

#### `TEST_F(StorageEngineErrorHandlingTest, SuccessPath)`
- Source: `tests/storage/test_storage_engine_di.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineErrorHandlingTest): n/a
  - `<unnamed>` (SuccessPath): n/a

#### `TEST_F(StorageEngineProductionGuardTest, DefaultEvaluatorAllowedInProduction)`
- Source: `tests/storage/test_storage_engine_di.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProductionGuardTest): n/a
  - `<unnamed>` (DefaultEvaluatorAllowedInProduction): n/a

#### `TEST_F(StorageEngineProductionGuardTest, DefaultImplementationsWorkInDevelopment)`
- Source: `tests/storage/test_storage_engine_di.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProductionGuardTest): n/a
  - `<unnamed>` (DefaultImplementationsWorkInDevelopment): n/a

#### `TEST_F(StorageEngineProductionGuardTest, EncryptionFailsInProductionMode)`
- Source: `tests/storage/test_storage_engine_di.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProductionGuardTest): n/a
  - `<unnamed>` (EncryptionFailsInProductionMode): n/a

#### `TEST_F(StorageEngineProductionGuardTest, IndexManagerFailsInProduction)`
- Source: `tests/storage/test_storage_engine_di.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProductionGuardTest): n/a
  - `<unnamed>` (IndexManagerFailsInProduction): n/a

#### `TEST_F(StorageEngineProductionGuardTest, KeyProviderFailsInProductionMode)`
- Source: `tests/storage/test_storage_engine_di.cpp`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProductionGuardTest): n/a
  - `<unnamed>` (KeyProviderFailsInProductionMode): n/a

#### `TEST_F(StorageEngineProductionGuardTest, NullIndexManagerRejectedInProduction)`
- Source: `tests/storage/test_storage_engine_di.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProductionGuardTest): n/a
  - `<unnamed>` (NullIndexManagerRejectedInProduction): n/a

### test_storage_engine_move_semantics.cpp

#### `TEST_F(StorageEngineMoveSemanticTest, CopyConstructor_IsDeleted)`
- Source: `tests/storage/test_storage_engine_move_semantics.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineMoveSemanticTest): n/a
  - `<unnamed>` (CopyConstructor_IsDeleted): n/a

#### `TEST_F(StorageEngineMoveSemanticTest, DecryptField_ReturnsVectorByMove)`
- Source: `tests/storage/test_storage_engine_move_semantics.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineMoveSemanticTest): n/a
  - `<unnamed>` (DecryptField_ReturnsVectorByMove): n/a

#### `TEST_F(StorageEngineMoveSemanticTest, EncryptField_ReturnsVectorByMove)`
- Source: `tests/storage/test_storage_engine_move_semantics.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineMoveSemanticTest): n/a
  - `<unnamed>` (EncryptField_ReturnsVectorByMove): n/a

#### `TEST_F(StorageEngineMoveSemanticTest, IOMetrics_ReturnsStructByMove)`
- Source: `tests/storage/test_storage_engine_move_semantics.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineMoveSemanticTest): n/a
  - `<unnamed>` (IOMetrics_ReturnsStructByMove): n/a

#### `TEST_F(StorageEngineMoveSemanticTest, MoveAssignmentOperator_TransfersState)`
- Source: `tests/storage/test_storage_engine_move_semantics.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineMoveSemanticTest): n/a
  - `<unnamed>` (MoveAssignmentOperator_TransfersState): n/a

#### `TEST_F(StorageEngineMoveSemanticTest, MoveConstructor_TransfersState)`
- Source: `tests/storage/test_storage_engine_move_semantics.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineMoveSemanticTest): n/a
  - `<unnamed>` (MoveConstructor_TransfersState): n/a

#### `TEST_F(StorageEngineMoveSemanticTest, MovedFrom_ObjectInValidState)`
- Source: `tests/storage/test_storage_engine_move_semantics.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineMoveSemanticTest): n/a
  - `<unnamed>` (MovedFrom_ObjectInValidState): n/a

#### `TEST_F(StorageEngineMoveSemanticTest, MultipleMetricsReturns_NoDoubleFreeSS)`
- Source: `tests/storage/test_storage_engine_move_semantics.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineMoveSemanticTest): n/a
  - `<unnamed>` (MultipleMetricsReturns_NoDoubleFreeSS): n/a

#### `TEST_F(StorageEngineMoveSemanticTest, ScanCounters_ReturnsStructByMove)`
- Source: `tests/storage/test_storage_engine_move_semantics.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineMoveSemanticTest): n/a
  - `<unnamed>` (ScanCounters_ReturnsStructByMove): n/a

### test_storage_engine_prod.cpp

#### `TEST_F(StorageEngineProdTest, CloseAndReopenSucceeds)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (CloseAndReopenSucceeds): n/a

#### `TEST_F(StorageEngineProdTest, ClosedEngine_DelFails)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ClosedEngine_DelFails): n/a

#### `TEST_F(StorageEngineProdTest, ClosedEngine_GetFails)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ClosedEngine_GetFails): n/a

#### `TEST_F(StorageEngineProdTest, ClosedEngine_PutFails)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ClosedEngine_PutFails): n/a

#### `TEST_F(StorageEngineProdTest, ClosedEngine_ScanRangeFails)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ClosedEngine_ScanRangeFails): n/a

#### `TEST_F(StorageEngineProdTest, Del_RemovesKey)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (Del_RemovesKey): n/a

#### `TEST_F(StorageEngineProdTest, Get_MissingKey_ReturnsError)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (Get_MissingKey_ReturnsError): n/a

#### `TEST_F(StorageEngineProdTest, OpenAlreadyOpen_ReturnsError)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (OpenAlreadyOpen_ReturnsError): n/a

#### `TEST_F(StorageEngineProdTest, PutGetRoundTrip)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (PutGetRoundTrip): n/a

#### `TEST_F(StorageEngineProdTest, Put_Overwrite)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (Put_Overwrite): n/a

#### `TEST_F(StorageEngineProdTest, ScanPrefix_EarlyStop)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ScanPrefix_EarlyStop): n/a

#### `TEST_F(StorageEngineProdTest, ScanPrefix_FindsMatchingKeys)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ScanPrefix_FindsMatchingKeys): n/a

#### `TEST_F(StorageEngineProdTest, ScanPrefix_NoMatch)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ScanPrefix_NoMatch): n/a

#### `TEST_F(StorageEngineProdTest, ScanRange_EarlyStop)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ScanRange_EarlyStop): n/a

#### `TEST_F(StorageEngineProdTest, ScanRange_EmptyResult)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ScanRange_EmptyResult): n/a

#### `TEST_F(StorageEngineProdTest, ScanRange_FullKeyspace)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ScanRange_FullKeyspace): n/a

#### `TEST_F(StorageEngineProdTest, ScanRange_HalfOpen_EndExclusive)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ScanRange_HalfOpen_EndExclusive): n/a

#### `TEST_F(StorageEngineProdTest, ScanRange_InOrder)`
- Source: `tests/storage/test_storage_engine_prod.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageEngineProdTest): n/a
  - `<unnamed>` (ScanRange_InOrder): n/a

### test_storage_fuzz.cpp

#### `TEST_F(StorageFuzzTest, AlternatingPutDel_LastOpWins)`
- Source: `tests/storage/test_storage_fuzz.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageFuzzTest): n/a
  - `<unnamed>` (AlternatingPutDel_LastOpWins): n/a

#### `TEST_F(StorageFuzzTest, ConcurrentWriters_AllEntriesRecoverable)`
- Source: `tests/storage/test_storage_fuzz.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageFuzzTest): n/a
  - `<unnamed>` (ConcurrentWriters_AllEntriesRecoverable): n/a

#### `TEST_F(StorageFuzzTest, CrashSim_TruncateFile_PartialEntriesSurvive)`
- Source: `tests/storage/test_storage_fuzz.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageFuzzTest): n/a
  - `<unnamed>` (CrashSim_TruncateFile_PartialEntriesSurvive): n/a

#### `TEST_F(StorageFuzzTest, LargeValues_RoundTripThroughRecovery)`
- Source: `tests/storage/test_storage_fuzz.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageFuzzTest): n/a
  - `<unnamed>` (LargeValues_RoundTripThroughRecovery): n/a

#### `TEST_F(StorageFuzzTest, MultipleSeedFuzz_RecoveryConsistent)`
- Source: `tests/storage/test_storage_fuzz.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageFuzzTest): n/a
  - `<unnamed>` (MultipleSeedFuzz_RecoveryConsistent): n/a

#### `TEST_F(StorageFuzzTest, RandomisedPutDel_FullRecovery)`
- Source: `tests/storage/test_storage_fuzz.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageFuzzTest): n/a
  - `<unnamed>` (RandomisedPutDel_FullRecovery): n/a

#### `TEST_F(StorageFuzzTest, RepeatedOpenClose_StateAccumulates)`
- Source: `tests/storage/test_storage_fuzz.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageFuzzTest): n/a
  - `<unnamed>` (RepeatedOpenClose_StateAccumulates): n/a

#### `TEST_F(StorageFuzzTest, SpecialCharKeys_RoundTripThroughRecovery)`
- Source: `tests/storage/test_storage_fuzz.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageFuzzTest): n/a
  - `<unnamed>` (SpecialCharKeys_RoundTripThroughRecovery): n/a

#### `std::map< std::string, std::string > replayToMap(const std::string &dir, WALStorage::Config cfg)`
- Source: `tests/storage/test_storage_fuzz.cpp`:58
- Brief: n/a
- Parameters:
  - `dir` (const std::string &): n/a
  - `cfg` (WALStorage::Config): n/a

### test_storage_latency_bench.cpp

#### `TEST_F(LatencyBenchmark, Del_P99Under50ms)`
- Source: `tests/storage/test_storage_latency_bench.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchmark): n/a
  - `<unnamed>` (Del_P99Under50ms): n/a

#### `TEST_F(LatencyBenchmark, Get_P99Under50ms)`
- Source: `tests/storage/test_storage_latency_bench.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchmark): n/a
  - `<unnamed>` (Get_P99Under50ms): n/a

#### `TEST_F(LatencyBenchmark, IOMetrics_LatencyConsistentWithDirectMeasurement)`
- Source: `tests/storage/test_storage_latency_bench.cpp`:245
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchmark): n/a
  - `<unnamed>` (IOMetrics_LatencyConsistentWithDirectMeasurement): n/a

#### `TEST_F(LatencyBenchmark, MixedWorkload_P99Under50ms)`
- Source: `tests/storage/test_storage_latency_bench.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchmark): n/a
  - `<unnamed>` (MixedWorkload_P99Under50ms): n/a

#### `TEST_F(LatencyBenchmark, Put_P99Under50ms)`
- Source: `tests/storage/test_storage_latency_bench.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchmark): n/a
  - `<unnamed>` (Put_P99Under50ms): n/a

#### `TEST_F(LatencyBenchmark, WriteAmplification_LessThan2x)`
- Source: `tests/storage/test_storage_latency_bench.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyBenchmark): n/a
  - `<unnamed>` (WriteAmplification_LessThan2x): n/a

### test_storage_layout_advisor.cpp

#### `TEST(StorageLayoutAdvisorTest, BlobCollectionHybrid)`
- Source: `tests/storage/test_storage_layout_advisor.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageLayoutAdvisorTest): n/a
  - `<unnamed>` (BlobCollectionHybrid): n/a

#### `TEST(StorageLayoutAdvisorTest, ConfidenceInRange)`
- Source: `tests/storage/test_storage_layout_advisor.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageLayoutAdvisorTest): n/a
  - `<unnamed>` (ConfidenceInRange): n/a

#### `TEST(StorageLayoutAdvisorTest, DefaultFallbackRowOriented)`
- Source: `tests/storage/test_storage_layout_advisor.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageLayoutAdvisorTest): n/a
  - `<unnamed>` (DefaultFallbackRowOriented): n/a

#### `TEST(StorageLayoutAdvisorTest, FloatTimeSeriesCompressionRatio)`
- Source: `tests/storage/test_storage_layout_advisor.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageLayoutAdvisorTest): n/a
  - `<unnamed>` (FloatTimeSeriesCompressionRatio): n/a

#### `TEST(StorageLayoutAdvisorTest, GdprFieldRequiresApproval)`
- Source: `tests/storage/test_storage_layout_advisor.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageLayoutAdvisorTest): n/a
  - `<unnamed>` (GdprFieldRequiresApproval): n/a

#### `TEST(StorageLayoutAdvisorTest, IsTimeSeriesTrue)`
- Source: `tests/storage/test_storage_layout_advisor.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageLayoutAdvisorTest): n/a
  - `<unnamed>` (IsTimeSeriesTrue): n/a

#### `TEST(StorageLayoutAdvisorTest, NoGdprFieldNoApproval)`
- Source: `tests/storage/test_storage_layout_advisor.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageLayoutAdvisorTest): n/a
  - `<unnamed>` (NoGdprFieldNoApproval): n/a

#### `TEST(StorageLayoutAdvisorTest, NonEmptyRationale)`
- Source: `tests/storage/test_storage_layout_advisor.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageLayoutAdvisorTest): n/a
  - `<unnamed>` (NonEmptyRationale): n/a

#### `TEST(StorageLayoutAdvisorTest, TimeSeriesColumnlarCompressed)`
- Source: `tests/storage/test_storage_layout_advisor.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageLayoutAdvisorTest): n/a
  - `<unnamed>` (TimeSeriesColumnlarCompressed): n/a

#### `TEST(StorageLayoutAdvisorTest, UuidPointLookupRowOriented)`
- Source: `tests/storage/test_storage_layout_advisor.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageLayoutAdvisorTest): n/a
  - `<unnamed>` (UuidPointLookupRowOriented): n/a

### test_storage_parquet_exporter.cpp

#### `TEST_F(StorageParquetExporterFocusedTests, PE10_SingleRowSegment)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE10_SingleRowSegment): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE11_StatsPopulatedAfterExport)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE11_StatsPopulatedAfterExport): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE12_ExportToFile_NonEmpty)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE12_ExportToFile_NonEmpty): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE13_ExportToFile_BadPath)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:295
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE13_ExportToFile_BadPath): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE14_MetadataLengthFieldPlausible)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:313
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE14_MetadataLengthFieldPlausible): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE15_MultipleSegmentsPerColumn)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:339
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE15_MultipleSegmentsPerColumn): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE1_OutputHasPAR1Magic)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE1_OutputHasPAR1Magic): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE2_INT32_OutputNonEmpty)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE2_INT32_OutputNonEmpty): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE3_INT64_OutputNonEmpty)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE3_INT64_OutputNonEmpty): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE4_FLOAT32_OutputNonEmpty)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE4_FLOAT32_OutputNonEmpty): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE5_FLOAT64_OutputNonEmpty)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE5_FLOAT64_OutputNonEmpty): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE6_BOOL_OutputNonEmpty)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE6_BOOL_OutputNonEmpty): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE7_MultiColumn_INT32_FLOAT64)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE7_MultiColumn_INT32_FLOAT64): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE8_EmptyColumnsConfig)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE8_EmptyColumnsConfig): n/a

#### `TEST_F(StorageParquetExporterFocusedTests, PE9_ColumnCountMismatch)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageParquetExporterFocusedTests): n/a
  - `<unnamed>` (PE9_ColumnCountMismatch): n/a

#### `bool endWithPAR1(const std::vector< uint8_t > &buf)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:63
- Brief: n/a
- Parameters:
  - `buf` (const std::vector< uint8_t > &): n/a

#### `ColumnSegment makeBoolSeg(const std::vector< uint8_t > &v)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:48
- Brief: n/a
- Parameters:
  - `v` (const std::vector< uint8_t > &): n/a

#### `ColumnSegment makeFloat32Seg(const std::vector< float > &v)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:34
- Brief: n/a
- Parameters:
  - `v` (const std::vector< float > &): n/a

#### `ColumnSegment makeFloat64Seg(const std::vector< double > &v)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:41
- Brief: n/a
- Parameters:
  - `v` (const std::vector< double > &): n/a

#### `ColumnSegment makeInt32Seg(const std::vector< int32_t > &v)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:20
- Brief: n/a
- Parameters:
  - `v` (const std::vector< int32_t > &): n/a

#### `ColumnSegment makeInt64Seg(const std::vector< int64_t > &v)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:27
- Brief: n/a
- Parameters:
  - `v` (const std::vector< int64_t > &): n/a

#### `bool startWithPAR1(const std::vector< uint8_t > &buf)`
- Source: `tests/storage/test_storage_parquet_exporter.cpp`:58
- Brief: n/a
- Parameters:
  - `buf` (const std::vector< uint8_t > &): n/a

### test_storage_phase3_error_handling_focused.cpp

#### `TEST_F(RecoveryFaultHandlerTest, SRF01_HandleTornWalEntry)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecoveryFaultHandlerTest): n/a
  - `<unnamed>` (SRF01_HandleTornWalEntry): n/a

#### `TEST_F(RecoveryFaultHandlerTest, SRF02_HandleInvalidCheckpoint)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecoveryFaultHandlerTest): n/a
  - `<unnamed>` (SRF02_HandleInvalidCheckpoint): n/a

#### `TEST_F(RecoveryFaultHandlerTest, SRF03_HandleRecoveryTimeout)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecoveryFaultHandlerTest): n/a
  - `<unnamed>` (SRF03_HandleRecoveryTimeout): n/a

#### `TEST_F(RecoveryFaultHandlerTest, SRF04_HandleWalFileCorruption)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecoveryFaultHandlerTest): n/a
  - `<unnamed>` (SRF04_HandleWalFileCorruption): n/a

#### `TEST_F(RecoveryFaultHandlerTest, SRF05_HandleWalReadErrorWithRetries)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecoveryFaultHandlerTest): n/a
  - `<unnamed>` (SRF05_HandleWalReadErrorWithRetries): n/a

#### `TEST_F(RecoveryFaultHandlerTest, SRF06_HandleReplayEntryFailureNonCritical)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecoveryFaultHandlerTest): n/a
  - `<unnamed>` (SRF06_HandleReplayEntryFailureNonCritical): n/a

#### `TEST_F(RecoveryFaultHandlerTest, SRF07_HandleReplayEntryFailureCritical)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecoveryFaultHandlerTest): n/a
  - `<unnamed>` (SRF07_HandleReplayEntryFailureCritical): n/a

#### `TEST_F(RecoveryFaultHandlerTest, SRF08_HandleRecoveryRetryExhausted)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecoveryFaultHandlerTest): n/a
  - `<unnamed>` (SRF08_HandleRecoveryRetryExhausted): n/a

#### `TEST_F(StorageErrorDiagnosticsTest, SED01_ClassifyWalWriteFailed)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageErrorDiagnosticsTest): n/a
  - `<unnamed>` (SED01_ClassifyWalWriteFailed): n/a

#### `TEST_F(StorageErrorDiagnosticsTest, SED02_ClassifyTransactionConflict)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageErrorDiagnosticsTest): n/a
  - `<unnamed>` (SED02_ClassifyTransactionConflict): n/a

#### `TEST_F(StorageErrorDiagnosticsTest, SED03_MapErrorMessageStorageExhausted)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageErrorDiagnosticsTest): n/a
  - `<unnamed>` (SED03_MapErrorMessageStorageExhausted): n/a

#### `TEST_F(StorageErrorDiagnosticsTest, SED04_MapErrorMessageCorruption)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageErrorDiagnosticsTest): n/a
  - `<unnamed>` (SED04_MapErrorMessageCorruption): n/a

#### `TEST_F(StorageErrorDiagnosticsTest, SED05_MapErrorMessageTimeout)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageErrorDiagnosticsTest): n/a
  - `<unnamed>` (SED05_MapErrorMessageTimeout): n/a

#### `TEST_F(StorageErrorDiagnosticsTest, SED06_BuildErrorContextWithSuggestions)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageErrorDiagnosticsTest): n/a
  - `<unnamed>` (SED06_BuildErrorContextWithSuggestions): n/a

#### `TEST_F(StorageErrorDiagnosticsTest, SED07_BuildErrorContextRecoveryIncomplete)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageErrorDiagnosticsTest): n/a
  - `<unnamed>` (SED07_BuildErrorContextRecoveryIncomplete): n/a

#### `TEST_F(StorageErrorDiagnosticsTest, SED08_ErrorNameAndDescription)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageErrorDiagnosticsTest): n/a
  - `<unnamed>` (SED08_ErrorNameAndDescription): n/a

#### `TEST_F(StoragePhase3IntegrationTest, E2E_CapacityManagementFlow)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (StoragePhase3IntegrationTest): n/a
  - `<unnamed>` (E2E_CapacityManagementFlow): n/a

#### `TEST_F(StoragePhase3IntegrationTest, E2E_RecoveryFaultDiagnosticsFlow)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:325
- Brief: n/a
- Parameters:
  - `<unnamed>` (StoragePhase3IntegrationTest): n/a
  - `<unnamed>` (E2E_RecoveryFaultDiagnosticsFlow): n/a

#### `TEST_F(StoragePressureManagerTest, SPM01_GetCapacityMetricsNormal)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (StoragePressureManagerTest): n/a
  - `<unnamed>` (SPM01_GetCapacityMetricsNormal): n/a

#### `TEST_F(StoragePressureManagerTest, SPM02_CanAcceptWriteNormal)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (StoragePressureManagerTest): n/a
  - `<unnamed>` (SPM02_CanAcceptWriteNormal): n/a

#### `TEST_F(StoragePressureManagerTest, SPM03_CanAcceptWriteWhenExhausted)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (StoragePressureManagerTest): n/a
  - `<unnamed>` (SPM03_CanAcceptWriteWhenExhausted): n/a

#### `TEST_F(StoragePressureManagerTest, SPM04_PressureEscalation)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (StoragePressureManagerTest): n/a
  - `<unnamed>` (SPM04_PressureEscalation): n/a

#### `TEST_F(StoragePressureManagerTest, SPM05_ReportStorageExhausted)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (StoragePressureManagerTest): n/a
  - `<unnamed>` (SPM05_ReportStorageExhausted): n/a

#### `TEST_F(StoragePressureManagerTest, SPM06_CanStartBackupNormal)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (StoragePressureManagerTest): n/a
  - `<unnamed>` (SPM06_CanStartBackupNormal): n/a

#### `TEST_F(StoragePressureManagerTest, SPM07_CanStartBackupLimitExceeded)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (StoragePressureManagerTest): n/a
  - `<unnamed>` (SPM07_CanStartBackupLimitExceeded): n/a

#### `TEST_F(StoragePressureManagerTest, SPM08_ReportBackupLimitExceeded)`
- Source: `tests/storage/test_storage_phase3_error_handling_focused.cpp`:309
- Brief: n/a
- Parameters:
  - `<unnamed>` (StoragePressureManagerTest): n/a
  - `<unnamed>` (SPM08_ReportBackupLimitExceeded): n/a

### test_storage_query_index_explicit_di.cpp

#### `TEST_F(StorageQueryIndexExplicitDITest, ExplicitDIStorageEncryptsAndDecryptsWithConfiguredProvider)`
- Source: `tests/storage/test_storage_query_index_explicit_di.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageQueryIndexExplicitDITest): n/a
  - `<unnamed>` (ExplicitDIStorageEncryptsAndDecryptsWithConfiguredProvider): n/a

#### `TEST_F(StorageQueryIndexExplicitDITest, ExplicitDIStorageIndexAndQueryComposeWithoutDefaultShims)`
- Source: `tests/storage/test_storage_query_index_explicit_di.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageQueryIndexExplicitDITest): n/a
  - `<unnamed>` (ExplicitDIStorageIndexAndQueryComposeWithoutDefaultShims): n/a

#### `TEST_F(StorageQueryIndexExplicitDITest, ExplicitDIStorageRoundTripUsesRealRocksDB)`
- Source: `tests/storage/test_storage_query_index_explicit_di.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageQueryIndexExplicitDITest): n/a
  - `<unnamed>` (ExplicitDIStorageRoundTripUsesRealRocksDB): n/a

#### `TEST_F(StorageQueryIndexExplicitDITest, ExplicitDIStorageUsesQueryEvaluatorForFilters)`
- Source: `tests/storage/test_storage_query_index_explicit_di.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (StorageQueryIndexExplicitDITest): n/a
  - `<unnamed>` (ExplicitDIStorageUsesQueryEvaluatorForFilters): n/a

### test_tensor_storage_observer.cpp

#### `TEST(TensorStorageObserverTest, TNSE_OBS_01_WriteObserverCalledAfterPut)`
- Source: `tests/storage/test_tensor_storage_observer.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorStorageObserverTest): n/a
  - `<unnamed>` (TNSE_OBS_01_WriteObserverCalledAfterPut): n/a

#### `TEST(TensorStorageObserverTest, TNSE_OBS_02_DeleteObserverCalledAfterRemove)`
- Source: `tests/storage/test_tensor_storage_observer.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorStorageObserverTest): n/a
  - `<unnamed>` (TNSE_OBS_02_DeleteObserverCalledAfterRemove): n/a

#### `TEST(TensorStorageObserverTest, TNSE_OBS_03_GraphStaysInSyncViaObservers)`
- Source: `tests/storage/test_tensor_storage_observer.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorStorageObserverTest): n/a
  - `<unnamed>` (TNSE_OBS_03_GraphStaysInSyncViaObservers): n/a

#### `TEST(TensorStorageObserverTest, TNSE_OBS_NoObserverSetDoesNotCrash)`
- Source: `tests/storage/test_tensor_storage_observer.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorStorageObserverTest): n/a
  - `<unnamed>` (TNSE_OBS_NoObserverSetDoesNotCrash): n/a

#### `TEST(TensorStorageObserverTest, TNSE_OBS_ThrowingDeleteObserverDoesNotPropagateException)`
- Source: `tests/storage/test_tensor_storage_observer.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorStorageObserverTest): n/a
  - `<unnamed>` (TNSE_OBS_ThrowingDeleteObserverDoesNotPropagateException): n/a

#### `TEST(TensorStorageObserverTest, TNSE_OBS_ThrowingObserverDoesNotPropagateException)`
- Source: `tests/storage/test_tensor_storage_observer.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorStorageObserverTest): n/a
  - `<unnamed>` (TNSE_OBS_ThrowingObserverDoesNotPropagateException): n/a

#### `std::shared_ptr< TensorNetworkStorageEngine > makeEngine()`
- Source: `tests/storage/test_tensor_storage_observer.cpp`:41
- Brief: n/a
- Parameters: none

#### `std::vector< float > randVec(std::size_t n, unsigned seed)`
- Source: `tests/storage/test_tensor_storage_observer.cpp`:31
- Brief: n/a
- Parameters:
  - `n` (std::size_t): n/a
  - `seed` (unsigned): n/a

### test_tensor_train_decomposer.cpp

#### `TEST(InMemoryTensorBackendTest, TNS01_PutGetDel)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryTensorBackendTest): n/a
  - `<unnamed>` (TNS01_PutGetDel): n/a

#### `TEST(TensorNetworkStorageEngineTest2, TNS08_NullBackendThrows)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:397
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorNetworkStorageEngineTest2): n/a
  - `<unnamed>` (TNS08_NullBackendThrows): n/a

#### `TEST_F(TTQuantizerTest, TTD13_INT8RoundTripError)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (TTQuantizerTest): n/a
  - `<unnamed>` (TTD13_INT8RoundTripError): n/a

#### `TEST_F(TTQuantizerTest, TTD14_NF4RoundTripError)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (TTQuantizerTest): n/a
  - `<unnamed>` (TTD14_NF4RoundTripError): n/a

#### `TEST_F(TTQuantizerTest, TTD15_QuantizedTrainSerializeDeserialize)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (TTQuantizerTest): n/a
  - `<unnamed>` (TTD15_QuantizedTrainSerializeDeserialize): n/a

#### `TEST_F(TTQuantizerTest, TTD16_BytesPerElement)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (TTQuantizerTest): n/a
  - `<unnamed>` (TTD16_BytesPerElement): n/a

#### `TEST_F(TensorNetworkStorageEngineTest, TNS02_PutStoresWithoutError)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorNetworkStorageEngineTest): n/a
  - `<unnamed>` (TNS02_PutStoresWithoutError): n/a

#### `TEST_F(TensorNetworkStorageEngineTest, TNS03_GetReconstructsCloseToOriginal)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorNetworkStorageEngineTest): n/a
  - `<unnamed>` (TNS03_GetReconstructsCloseToOriginal): n/a

#### `TEST_F(TensorNetworkStorageEngineTest, TNS04_GetCompressedReturnsQTrain)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorNetworkStorageEngineTest): n/a
  - `<unnamed>` (TNS04_GetCompressedReturnsQTrain): n/a

#### `TEST_F(TensorNetworkStorageEngineTest, TNS05_StatsValid)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:369
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorNetworkStorageEngineTest): n/a
  - `<unnamed>` (TNS05_StatsValid): n/a

#### `TEST_F(TensorNetworkStorageEngineTest, TNS06_RemoveMissingKeyReturnsFalse)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorNetworkStorageEngineTest): n/a
  - `<unnamed>` (TNS06_RemoveMissingKeyReturnsFalse): n/a

#### `TEST_F(TensorNetworkStorageEngineTest, TNS07_MultipleVersionsTracked)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorNetworkStorageEngineTest): n/a
  - `<unnamed>` (TNS07_MultipleVersionsTracked): n/a

#### `TEST_F(TensorNetworkStorageEngineTest, TNS09_CompactIgnoresMalformedVersionSuffix)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:403
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorNetworkStorageEngineTest): n/a
  - `<unnamed>` (TNS09_CompactIgnoresMalformedVersionSuffix): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD01_FlatRank1TensorHighCompression)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD01_FlatRank1TensorHighCompression): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD02_2DMatrixReconstructionEps)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD02_2DMatrixReconstructionEps): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD03_4DTensorCoreShapes)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD03_4DTensorCoreShapes): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD04_6DTensorReconstructionError)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD04_6DTensorReconstructionError): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD05_CompressionRatioLowRank)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD05_CompressionRatioLowRank): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD06_InnerProductSelfEqualsNormSq)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD06_InnerProductSelfEqualsNormSq): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD07_CosineSimilarityIdentical)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD07_CosineSimilarityIdentical): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD08_CosineSimilarityZeroTensor)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD08_CosineSimilarityZeroTensor): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD09_SerializeDeserializeRoundTrip)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD09_SerializeDeserializeRoundTrip): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD10_DecomposeF64)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD10_DecomposeF64): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD11_SizeMismatchThrows)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD11_SizeMismatchThrows): n/a

#### `TEST_F(TensorTrainDecomposerTest, TTD12_MaxRankCap)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (TensorTrainDecomposerTest): n/a
  - `<unnamed>` (TTD12_MaxRankCap): n/a

#### `std::vector< float > makeLowRank2D(std::size_t m, std::size_t n, std::size_t rank=2)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:72
- Brief: n/a
- Parameters:
  - `m` (std::size_t): n/a
  - `n` (std::size_t): n/a
  - `rank` (std::size_t): n/a

#### `std::vector< float > makeRandom(std::size_t n, float scale=1.0f, unsigned seed=42)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:61
- Brief: n/a
- Parameters:
  - `n` (std::size_t): n/a
  - `scale` (float): n/a
  - `seed` (unsigned): n/a

#### `std::size_t product(const std::vector< std::size_t > &v)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:53
- Brief: n/a
- Parameters:
  - `v` (const std::vector< std::size_t > &): n/a

#### `double relError(const std::vector< float > &a, const std::vector< float > &b)`
- Source: `tests/storage/test_tensor_train_decomposer.cpp`:93
- Brief: n/a
- Parameters:
  - `a` (const std::vector< float > &): n/a
  - `b` (const std::vector< float > &): n/a

### themis

#### `std::string bytesToHex(const std::vector< uint8_t > &v)`
- Source: `src/storage/history_manager.cpp`:108
- Brief: ───────────────────────────────────────────────────────────────────────────── Hex encode/decode helpers (private to this TU) ─────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `v` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. Calls: std::setfill(), std::setw(), str().

#### `uint32_t crc32_update(uint32_t crc, const void *data, size_t len)`
- Source: `src/storage/wal_storage.cpp`:258
- Brief: ────────────────────────────────────────────────────────────────────────────── CRC32 (simple table-based implementation; no external dependency) ──────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `crc` (uint32_t): Input parameter.
  - `data` (const void *): Input parameter.
  - `len` (size_t): Input parameter.
- Return: Return value.
- Details: crc Input parameter. data Input parameter. len Input parameter. Return value. Calls: std::call_once().

#### `uint32_t decode_u32(const uint8_t *buf)`
- Source: `src/storage/wal_storage.cpp`:306
- Brief: n/a
- Parameters:
  - `buf` (const uint8_t *): n/a

#### `uint32_t decode_u32(const uint8_t(&buf)[4])`
- Source: `src/storage/wal_storage.cpp`:323
- Brief: n/a
- Parameters:
  - `buf` (const uint8_t(&)): n/a

#### `uint64_t decode_u64(const uint8_t *buf)`
- Source: `src/storage/wal_storage.cpp`:313
- Brief: n/a
- Parameters:
  - `buf` (const uint8_t *): n/a

#### `uint64_t decode_u64(const uint8_t(&buf)[8])`
- Source: `src/storage/wal_storage.cpp`:325
- Brief: n/a
- Parameters:
  - `buf` (const uint8_t(&)): n/a

#### `void encode_u32(uint8_t *buf, uint32_t v)`
- Source: `src/storage/wal_storage.cpp`:293
- Brief: n/a
- Parameters:
  - `buf` (uint8_t *): n/a
  - `v` (uint32_t): n/a

#### `void encode_u32(uint8_t(&buf)[4], uint32_t v)`
- Source: `src/storage/wal_storage.cpp`:322
- Brief: n/a
- Parameters:
  - `buf` (uint8_t(&)): n/a
  - `v` (uint32_t): n/a

#### `void encode_u64(uint8_t *buf, uint64_t v)`
- Source: `src/storage/wal_storage.cpp`:300
- Brief: n/a
- Parameters:
  - `buf` (uint8_t *): n/a
  - `v` (uint64_t): n/a

#### `void encode_u64(uint8_t(&buf)[8], uint64_t v)`
- Source: `src/storage/wal_storage.cpp`:324
- Brief: n/a
- Parameters:
  - `buf` (uint8_t(&)): n/a
  - `v` (uint64_t): n/a

#### `std::vector< uint8_t > hexToBytes(const std::string &hex)`
- Source: `src/storage/history_manager.cpp`:123
- Brief: Hex To Bytes.
- Parameters:
  - `hex` (const std::string &): Input parameter.
- Return: Return value.
- Details: hex Input parameter. Return value. Calls: size(), reserve(), push_back(), nibble().

#### `int themis_close_fd(int fd)`
- Source: `src/storage/storage_audit_logger.cpp`:101
- Brief: Themis close fd.
- Parameters:
  - `fd` (int): Input parameter.
- Return: Return value.
- Details: fd Input parameter. Return value. Calls: close().

#### `int themis_close_fd(int fd)`
- Source: `src/storage/wal_storage.cpp`:129
- Brief: Themis close fd.
- Parameters:
  - `fd` (int): Input parameter.
- Return: Return value.
- Details: fd Input parameter. Return value. Calls: close().

#### `int themis_fsync_fd(int fd)`
- Source: `src/storage/storage_audit_logger.cpp`:108
- Brief: Themis fsync fd.
- Parameters:
  - `fd` (int): Input parameter.
- Return: Return value.
- Details: fd Input parameter. Return value. Calls: fsync().

#### `int themis_fsync_fd(int fd)`
- Source: `src/storage/wal_storage.cpp`:136
- Brief: Themis fsync fd.
- Parameters:
  - `fd` (int): Input parameter.
- Return: Return value.
- Details: fd Input parameter. Return value. Calls: themis_test_flag_once(), fsync().

#### `int themis_open_fd(const char *path, int flags, int mode)`
- Source: `src/storage/storage_audit_logger.cpp`:94
- Brief: no_timeout scanner alert: these are thin POSIX syscall shims for local audit-log files; block-device I/O does not require network-style timeouts.
- Parameters:
  - `path` (const char *): Input parameter.
  - `flags` (int): Input parameter.
  - `mode` (int): Input parameter.
- Return: Return value.
- Details: path Input parameter. flags Input parameter. mode Input parameter. Return value. Calls: open().

#### `int themis_open_fd(const char *path, int flags, int mode)`
- Source: `src/storage/wal_storage.cpp`:111
- Brief: O_CLOEXEC ensures the WAL FD is not inherited by child processes and is automatically closed on exec — prevents FD leaks without explicit action.
- Parameters:
  - `path` (const char *): Input parameter.
  - `flags` (int): Input parameter.
  - `mode` (int): Input parameter.
- Return: Return value.
- Details: path Input parameter. flags Input parameter. mode Input parameter. Return value. no_timeout scanner alert: these are thin POSIX syscall shims used for local WAL files; network-style timeouts do not apply to local block-device I/O. Calls: themis_test_flag_once(), open().

#### `bool themis_test_flag_once(const char *name)`
- Source: `src/storage/wal_storage.cpp`:94
- Brief: Themis test flag once.
- Parameters:
  - `name` (const char *): Input parameter.
- Return: True when the operation succeeds.
- Details: name Input parameter. True when the operation succeeds. Calls: std::getenv(), unsetenv().

#### `themis_ssize_t themis_write_fd(int fd, const void *data, size_t len)`
- Source: `src/storage/storage_audit_logger.cpp`:117
- Brief: Themis write fd.
- Parameters:
  - `fd` (int): Input parameter.
  - `data` (const void *): Input parameter.
  - `len` (size_t): Input parameter.
- Return: Return value.
- Details: fd Input parameter. data Input parameter. len Input parameter. Return value. Calls: write().

#### `themis_ssize_t themis_write_fd(int fd, const void *data, size_t len)`
- Source: `src/storage/wal_storage.cpp`:159
- Brief: Themis write fd.
- Parameters:
  - `fd` (int): Input parameter.
  - `data` (const void *): Input parameter.
  - `len` (size_t): Input parameter.
- Return: Return value.
- Details: fd Input parameter. data Input parameter. len Input parameter. Return value. Calls: themis_test_flag_once(), write().

#### `void validateConfig(const WomTree::Config &cfg)`
- Source: `src/storage/wom_tree.cpp`:750
- Brief: Validate Config.
- Parameters:
  - `cfg` (const WomTree::Config &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: cfg Input parameter. std::invalid_argument if an error occurs. Implements validateConfig without additional internal calls.

#### `bool write_all_fd(int fd, const void *data, size_t len)`
- Source: `src/storage/wal_storage.cpp`:224
- Brief: Write all fd.
- Parameters:
  - `fd` (int): Input parameter.
  - `data` (const void *): Input parameter.
  - `len` (size_t): Input parameter.
- Return: True when the operation succeeds.
- Details: fd Input parameter. data Input parameter. len Input parameter. True when the operation succeeds. Calls: themis_write_fd().

### themis::AdaptiveCompactionScheduler

#### `AdaptiveCompactionScheduler()`
- Source: `include/storage/adaptive_compaction.h`:204
- Brief: n/a
- Parameters: none
- Details: Construct with default configuration.

#### `AdaptiveCompactionScheduler(const AdaptiveCompactionScheduler &)=delete`
- Source: `include/storage/adaptive_compaction.h`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AdaptiveCompactionScheduler &): n/a

#### `AdaptiveCompactionScheduler(const Config &config)`
- Source: `include/storage/adaptive_compaction.h`:212
- Brief: Construct with custom configuration.
- Parameters:
  - `config` (const Config &): Scheduler tuning parameters used to derive compaction thresholds and sampling cadence.
- Details: config Scheduler tuning parameters used to derive compaction thresholds and sampling cadence.

#### `void applyAdaptedConfig(CompactionManager &mgr)`
- Source: `include/storage/adaptive_compaction.h`:283
- Brief: Apply the dynamically computed configuration to mgr.
- Parameters:
  - `mgr` (CompactionManager &): Input/output parameter.
- Details: Apply Adapted Config. This adjusts the tombstone GC threshold and background GC interval of the live CompactionManager based on the current workload. This restarts the background GC thread inside mgr if it was already running, so the new interval takes effect immediately. mgr Input/output parameter. mgr Input/output parameter. Calls: computeAdaptedConfig(), getConfig(), setConfig(), fetch_add().

#### `void collectSample()`
- Source: `include/storage/adaptive_compaction.h`:313
- Brief: Collect Sample.
- Parameters: none
- Details: Capture one sampling interval and update the rolling counters. Calls: std::chrono::steady_clock::now(), exchange(), lock(), count(), updateEMA(), push_back(), size(), pop_front().

#### `AdaptedConfig computeAdaptedConfig() const`
- Source: `include/storage/adaptive_compaction.h`:317
- Brief: n/a
- Parameters: none
- Details: Compute the configuration implied by the current workload state.

#### `AdaptedConfig getAdaptedConfig() const`
- Source: `include/storage/adaptive_compaction.h`:271
- Brief: Compute a CompactionManager::Config adjusted for the current workload without applying it.
- Parameters: none
- Return: The adapted configuration that would be applied for the current workload state.
- Details: The adapted configuration that would be applied for the current workload state.

#### `bool isLowLoadPeriod() const`
- Source: `include/storage/adaptive_compaction.h`:247
- Brief: Return true when the current EMA I/O rates are below the configured low-load thresholds.
- Parameters: none
- Return: True on success.
- Details: True on success.

#### `bool isSamplingRunning() const`
- Source: `include/storage/adaptive_compaction.h`:302
- Brief: n/a
- Parameters: none
- Details: Return true if the background sampling thread is running.

#### `AdaptiveCompactionScheduler & operator=(const AdaptiveCompactionScheduler &)=delete`
- Source: `include/storage/adaptive_compaction.h`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (const AdaptiveCompactionScheduler &): n/a

#### `CompactionImpactPrediction predictCompactionImpact(double current_write_amp=0.0) const`
- Source: `include/storage/adaptive_compaction.h`:237
- Brief: Predict the impact of running a compaction pass now.
- Parameters:
  - `current_write_amp` (double): Current write-amplification factor; pass 0.0 if unknown (the prediction is still useful but less precise).
- Details: current_write_amp Current write-amplification factor; pass 0.0 if unknown (the prediction is still useful but less precise).

#### `void recordRead(uint64_t count=1)`
- Source: `include/storage/adaptive_compaction.h`:223
- Brief: Record Read.
- Parameters:
  - `count` (uint64_t): Input parameter.
- Details: Record count read operations. count Input parameter. Calls: fetch_add().

#### `void recordWrite(uint64_t count=1)`
- Source: `include/storage/adaptive_compaction.h`:226
- Brief: Record Write.
- Parameters:
  - `count` (uint64_t): Input parameter.
- Details: Record count write operations. count Input parameter. Calls: fetch_add().

#### `void samplingLoop()`
- Source: `include/storage/adaptive_compaction.h`:311
- Brief: Sampling Loop.
- Parameters: none
- Details: Background sampling loop that updates the EMA and sliding window. Calls: load(), lock(), wait_for(), unlock(), collectSample().

#### `bool shouldTriggerCompaction(double current_write_amp=0.0)`
- Source: `include/storage/adaptive_compaction.h`:261
- Brief: Return true when a compaction should be triggered.
- Parameters:
  - `current_write_amp` (double): Input parameter.
- Return: True when the operation succeeds.
- Details: Should Trigger Compaction. Returns true if: write amplification is above the urgent threshold, OR write amplification is above the desired threshold AND the system is in a low-load period. A "yes" increments the internal compaction_schedules counter. current_write_amp Write-amplification factor from CompactionManager::Stats. current_write_amp Input parameter. True when the operation succeeds. Calls: predictCompactionImpact(), fetch_add().

#### `void startSampling()`
- Source: `include/storage/adaptive_compaction.h`:296
- Brief: Start the background sampling thread.
- Parameters: none
- Details: Start Sampling. The thread wakes every config.sample_interval, converts the accumulated read/write counts into per-second rates, updates the EMA, and appends an IOSample to the sliding window. Does nothing if the thread is already running. Calls: lock(), joinable(), store(), std::thread(), samplingLoop().

#### `Stats stats() const`
- Source: `include/storage/adaptive_compaction.h`:307
- Brief: n/a
- Parameters: none
- Details: Return a snapshot of current scheduler statistics.

#### `void stopSampling()`
- Source: `include/storage/adaptive_compaction.h`:299
- Brief: Stop Sampling.
- Parameters: none
- Details: Stop and join the background sampling thread. Calls: lock(), store(), notify_all(), joinable(), utils::joinThreadWithin(), THEMIS_WARN().

#### `void updateEMA(double new_value, double &ema) noexcept`
- Source: `include/storage/adaptive_compaction.h`:315
- Brief: n/a
- Parameters:
  - `new_value` (double): n/a
  - `ema` (double &): n/a
- Details: Update an exponential moving average in place.

#### `~AdaptiveCompactionScheduler()`
- Source: `include/storage/adaptive_compaction.h`:214
- Brief: n/a
- Parameters: none

### themis::AppendMergeOperator

#### `AppendMergeOperator(std::string delimiter="\|")`
- Source: `include/storage/merge_operators.h`:40
- Brief: n/a
- Parameters:
  - `delimiter` (std::string): n/a

#### `bool Merge(const rocksdb::Slice &key, const rocksdb::Slice *existing_value, const rocksdb::Slice &value, std::string *new_value, rocksdb::Logger *logger) const override`
- Source: `include/storage/merge_operators.h`:42
- Brief: n/a
- Parameters:
  - `key` (const rocksdb::Slice &): n/a
  - `existing_value` (const rocksdb::Slice *): n/a
  - `value` (const rocksdb::Slice &): n/a
  - `new_value` (std::string *): n/a
  - `logger` (rocksdb::Logger *): n/a

#### `const char * Name() const override`
- Source: `include/storage/merge_operators.h`:48
- Brief: n/a
- Parameters: none

### themis::BackupManager

#### `BackupManager(std::shared_ptr< RocksDBWrapper > db_wrapper, Config config={})`
- Source: `include/storage/backup_manager.h`:211
- Brief: Construct a backup manager around an open RocksDB wrapper.
- Parameters:
  - `db_wrapper` (std::shared_ptr< RocksDBWrapper >): Shared pointer to the storage wrapper used for checkpoint and restore operations.
  - `config` (Config): Backup manager runtime configuration. Set config.backup_base_dir to constrain restore/checksum guards to a dedicated backup root.
- Details: The wrapper is used for checkpoint creation, sequence-number discovery, restore operations, and snapshot orchestration. db_wrapper Shared pointer to the storage wrapper used for checkpoint and restore operations. config Backup manager runtime configuration. Set config.backup_base_dir to constrain restore/checksum guards to a dedicated backup root.

#### `uint32_t applyRetentionPolicy(const std::string &backup_dir, uint32_t retention_days, std::error_code &ec)`
- Source: `include/storage/backup_manager.h`:473
- Brief: Delete backups older than the retention window.
- Parameters:
  - `backup_dir` (const std::string &): Base backup directory.
  - `retention_days` (uint32_t): Number of days to retain.
  - `ec` (std::error_code &): Receives the failure reason when directory traversal or deletion fails. Partial deletion may already have occurred.
- Return: Number of backups deleted.
- Details: backup_dir Base backup directory. retention_days Number of days to retain. ec Receives the failure reason when directory traversal or deletion fails. Partial deletion may already have occurred. Number of backups deleted.

#### `Result< void > archiveWAL(const std::string &dest_dir)`
- Source: `include/storage/backup_manager.h`:310
- Brief: Archive WAL files into a destination directory.
- Parameters:
  - `dest_dir` (const std::string &): Destination directory for archived WAL files.
- Return: Result<void> on success, or an error when WAL discovery or copy fails.
- Details: Archive WAL. dest_dir Destination directory for archived WAL files. Result<void> on success, or an error when WAL discovery or copy fails. param Input parameter. Return value. Implements archiveWAL without additional internal calls.

#### `bool archiveWAL(const std::string &dest_dir, std::error_code &ec)`
- Source: `include/storage/backup_manager.h`:301
- Brief: Archive WAL files into a destination directory.
- Parameters:
  - `dest_dir` (const std::string &): Destination directory for archived WAL files.
  - `ec` (std::error_code &): Input/output parameter.
- Return: true on success, or false if WAL discovery or copy fails.
- Details: Archive WAL. dest_dir Destination directory for archived WAL files. ec Receives the failure reason when the operation returns false. true on success, or false if WAL discovery or copy fails. param Input parameter. ec Input/output parameter. True when the operation succeeds. Calls: std::make_error_code().

#### `Result< void > buildIntegrityManifest(const std::string &backup_dir, std::vector< FileIntegrityInfo > &integrity_map)`
- Source: `include/storage/backup_manager.h`:980
- Brief: Build integrity metadata for a backup payload before compression completes.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory whose files should be fingerprinted.
  - `integrity_map` (std::vector< FileIntegrityInfo > &): Receives one entry per tracked file on success.
- Return: Result<void> on success, or an error when checksum generation fails.
- Details: backup_dir Backup directory whose files should be fingerprinted. integrity_map Receives one entry per tracked file on success. Result<void> on success, or an error when checksum generation fails.

#### `Result< std::string > calculateChecksum(const std::string &file_path)`
- Source: `include/storage/backup_manager.h`:817
- Brief: Calculate the SHA-256 checksum for a file.
- Parameters:
  - `file_path` (const std::string &): File to hash.
- Return: Result<std::string> containing the lowercase hexadecimal digest on success, or an error when the file cannot be read.
- Details: file_path File to hash. Result<std::string> containing the lowercase hexadecimal digest on success, or an error when the file cannot be read.

#### `Result< void > cancelScheduledBackup(const std::string &schedule_id)`
- Source: `include/storage/backup_manager.h`:545
- Brief: Cancel a previously registered in-memory backup schedule.
- Parameters:
  - `schedule_id` (const std::string &): Schedule identifier returned by scheduleBackup().
- Return: Result<void> on success, or an Error if the identifier is empty or no matching schedule exists.
- Details: schedule_id Schedule identifier returned by scheduleBackup(). Result<void> on success, or an Error if the identifier is empty or no matching schedule exists.

#### `Result< std::string > compressBackup(const std::string &backup_dir)`
- Source: `include/storage/backup_manager.h`:389
- Brief: Compress a backup directory into an archive file.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory to compress.
- Return: Result<std::string> containing the compressed archive path on success, or an error when the archive cannot be created.
- Details: Compress Backup. backup_dir Backup directory to compress. Result<std::string> containing the compressed archive path on success, or an error when the archive cannot be created. param Input parameter. Return value. Implements compressBackup without additional internal calls.

#### `bool compressPath(const std::string &src_path, const std::string &dest_path, CompressionType type, std::error_code &ec)`
- Source: `include/storage/backup_manager.h`:888
- Brief: Compress or copy a file tree into a destination path.
- Parameters:
  - `src_path` (const std::string &): Source file or directory.
  - `dest_path` (const std::string &): Destination path for compressed output.
  - `type` (CompressionType): Compression mode to apply.
  - `ec` (std::error_code &): Receives the failure reason when the operation returns false.
- Return: true on success, or false when traversal, compression, or copy fails.
- Details: The exact behavior depends on the selected compression type and which compression libraries were linked into the build. src_path Source file or directory. dest_path Destination path for compressed output. type Compression mode to apply. ec Receives the failure reason when the operation returns false. true on success, or false when traversal, compression, or copy fails.

#### `Result< void > copyWALFiles(const std::string &src_dir, const std::string &dest_dir, uint64_t min_sequence)`
- Source: `include/storage/backup_manager.h`:807
- Brief: Copy WAL files whose sequence number is at least min_sequence.
- Parameters:
  - `src_dir` (const std::string &): Source WAL directory.
  - `dest_dir` (const std::string &): Destination WAL directory.
  - `min_sequence` (uint64_t): Minimum sequence number to retain.
- Return: Result<void> on success, or an error when traversal or copy fails.
- Details: src_dir Source WAL directory. dest_dir Destination WAL directory. min_sequence Minimum sequence number to retain. Result<void> on success, or an error when traversal or copy fails.

#### `Result< std::string > createDifferentialBackup(const std::string &dest_dir)`
- Source: `include/storage/backup_manager.h`:292
- Brief: Create a differential backup and return the resulting directory path.
- Parameters:
  - `dest_dir` (const std::string &): Base backup directory.
- Return: Result<std::string> containing the created backup directory on success, or an error when the differential backup cannot be created.
- Details: Create Differential Backup. dest_dir Base backup directory. Result<std::string> containing the created backup directory on success, or an error when the differential backup cannot be created. param Input parameter. Return value. Implements createDifferentialBackup without additional internal calls.

#### `bool createDifferentialBackup(const std::string &dest_dir, std::error_code &ec, const BackupOptions &options=BackupOptions())`
- Source: `include/storage/backup_manager.h`:282
- Brief: Create a differential backup containing changes since the last full backup.
- Parameters:
  - `dest_dir` (const std::string &): Base backup directory.
  - `ec` (std::error_code &): Input/output parameter.
  - `options` (const BackupOptions &): Backup options that shape backup generation.
- Return: true on success, or false if no valid base full backup exists or the differential payload cannot be assembled.
- Details: Create Differential Backup. dest_dir Base backup directory. ec Receives the failure reason when the operation returns false. options Backup options that shape backup generation. true on success, or false if no valid base full backup exists or the differential payload cannot be assembled. param Input parameter. ec Input/output parameter. param Input parameter. True when the operation succeeds. Calls: std::make_error_code().

#### `Result< std::string > createFullBackup(const std::string &dest_dir)`
- Source: `include/storage/backup_manager.h`:246
- Brief: Create a full backup and return the created backup directory path.
- Parameters:
  - `dest_dir` (const std::string &): Base backup directory.
- Return: Result<std::string> containing the created backup directory on success, or an error when the backup cannot be created.
- Details: Create Full Backup. dest_dir Base backup directory. Result<std::string> containing the created backup directory on success, or an error when the backup cannot be created. param Input parameter. Return value. Implements createFullBackup without additional internal calls.

#### `bool createFullBackup(const std::string &dest_dir, std::error_code &ec, const BackupOptions &options=BackupOptions())`
- Source: `include/storage/backup_manager.h`:236
- Brief: Create a full backup using a RocksDB checkpoint plus WAL capture.
- Parameters:
  - `dest_dir` (const std::string &): Base backup directory. The method creates a timestamped subdirectory beneath this path.
  - `ec` (std::error_code &): Input/output parameter.
  - `options` (const BackupOptions &): Backup options for compression, encryption, verification, retention, and transport handling.
- Return: true on success, or false when checkpoint creation, file copy, manifest generation, or verification fails.
- Details: Create Full Backup. For RAID5/6 deployments, a successful full backup includes every data and parity shard required for later recovery. dest_dir Base backup directory. The method creates a timestamped subdirectory beneath this path. ec Receives the failure reason when the operation returns false. options Backup options for compression, encryption, verification, retention, and transport handling. true on success, or false when checkpoint creation, file copy, manifest generation, or verification fails. param Input parameter. ec Input/output parameter. param Input parameter. True when the operation succeeds. Calls: std::make_error_code().

#### `Result< std::string > createIncrementalBackup(const std::string &dest_dir)`
- Source: `include/storage/backup_manager.h`:271
- Brief: Create an incremental backup and return the created backup path.
- Parameters:
  - `dest_dir` (const std::string &): Base backup directory.
- Return: Result<std::string> containing the created backup directory on success, or an error when the backup cannot be created.
- Details: Create Incremental Backup. dest_dir Base backup directory. Result<std::string> containing the created backup directory on success, or an error when the backup cannot be created. param Input parameter. Return value. Implements createIncrementalBackup without additional internal calls.

#### `bool createIncrementalBackup(const std::string &dest_dir, std::error_code &ec, const BackupOptions &options=BackupOptions())`
- Source: `include/storage/backup_manager.h`:261
- Brief: Create an incremental backup from WAL changes since the last backup.
- Parameters:
  - `dest_dir` (const std::string &): Base backup directory.
  - `ec` (std::error_code &): Input/output parameter.
  - `options` (const BackupOptions &): Backup options for compression, encryption, verification, retention, and transport handling.
- Return: true on success, or false if the incremental window cannot be computed or the backup payload cannot be written.
- Details: Create Incremental Backup. For RAID5/6 deployments, incremental state is collected across all shards participating in the backup set. dest_dir Base backup directory. ec Receives the failure reason when the operation returns false. options Backup options for compression, encryption, verification, retention, and transport handling. true on success, or false if the incremental window cannot be computed or the backup payload cannot be written. param Input parameter. ec Input/output parameter. param Input parameter. True when the operation succeeds. Calls: std::make_error_code().

#### `Result< void > createManifest(const std::string &backup_dir, const std::string &type, uint64_t sequence_number)`
- Source: `include/storage/backup_manager.h`:784
- Brief: Create the MANIFEST.json file for a backup payload.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory that receives the manifest.
  - `type` (const std::string &): Logical backup type (full, incremental, differential).
  - `sequence_number` (uint64_t): RocksDB sequence number captured for the backup.
- Return: Result<void> on success, or an error when the manifest cannot be written.
- Details: backup_dir Backup directory that receives the manifest. type Logical backup type (full, incremental, differential). sequence_number RocksDB sequence number captured for the backup. Result<void> on success, or an error when the manifest cannot be written.

#### `Result< std::string > createSnapshot(const std::string &snapshot_name, const std::string &storage_class="default")`
- Source: `include/storage/backup_manager.h`:622
- Brief: Create a consistent online snapshot of the database.
- Parameters:
  - `snapshot_name` (const std::string &): Human-readable name; used as the directory name inside the default snapshot base path ("<db_path>/../snapshots/<snapshot_name>_<ts>").
  - `storage_class` (const std::string &): Reserved for future cloud / K8s integration; ignored for local snapshots.
- Return: On success: the absolute path to the snapshot directory. On failure: an Error describing what went wrong.
- Details: Uses RocksDB's Checkpoint API to produce a crash-consistent, hard-linked copy of the current SST files. No writes are blocked during the operation (quiesce-safe). A JSON manifest is written next to the snapshot directory so the snapshot can be identified, verified, and restored later. snapshot_name Human-readable name; used as the directory name inside the default snapshot base path ("<db_path>/../snapshots/<snapshot_name>_<ts>"). storage_class Reserved for future cloud / K8s integration; ignored for local snapshots. On success: the absolute path to the snapshot directory. On failure: an Error describing what went wrong.

#### `Result< std::string > decompressBackup(const std::string &compressed_file, const std::string &dest_dir)`
- Source: `include/storage/backup_manager.h`:404
- Brief: Decompress a backup archive into a destination directory.
- Parameters:
  - `compressed_file` (const std::string &): Compressed backup archive.
  - `dest_dir` (const std::string &): Destination directory for extracted files.
- Return: Result<std::string> containing the decompressed directory path on success, or an error when extraction or post-extraction verification fails.
- Details: Decompress Backup. compressed_file Compressed backup archive. dest_dir Destination directory for extracted files. Result<std::string> containing the decompressed directory path on success, or an error when extraction or post-extraction verification fails. Phase 1 Enhancement: This method now performs integrity verification after decompression to prevent silent data corruption. All decompressed files are verified against stored checksums to ensure data integrity. param Input parameter. param Input parameter. Return value. Implements decompressBackup without additional internal calls.

#### `bool decompressPath(const std::string &src_path, const std::string &dest_path, CompressionType type, std::error_code &ec)`
- Source: `include/storage/backup_manager.h`:902
- Brief: Decompress or copy a backup payload into a destination path.
- Parameters:
  - `src_path` (const std::string &): Source file or directory.
  - `dest_path` (const std::string &): Destination directory for decompressed output.
  - `type` (CompressionType): Compression mode expected in the source payload.
  - `ec` (std::error_code &): Receives the failure reason when the operation returns false.
- Return: true on success, or false when traversal, decompression, or copy fails.
- Details: src_path Source file or directory. dest_path Destination directory for decompressed output. type Compression mode expected in the source payload. ec Receives the failure reason when the operation returns false. true on success, or false when traversal, decompression, or copy fails.

#### `bool decompressPathWithIntegrity(const std::string &src_path, const std::string &dest_path, CompressionType type, std::error_code &ec)`
- Source: `include/storage/backup_manager.h`:1034
- Brief: Decompress a payload and run the built-in integrity verification path.
- Parameters:
  - `src_path` (const std::string &): Source file or directory.
  - `dest_path` (const std::string &): Destination directory for decompressed output.
  - `type` (CompressionType): Compression mode expected in the source payload.
  - `ec` (std::error_code &): Receives the failure reason when the operation returns false.
- Return: true on success, or false when extraction or integrity verification fails.
- Details: src_path Source file or directory. dest_path Destination directory for decompressed output. type Compression mode expected in the source payload. ec Receives the failure reason when the operation returns false. true on success, or false when extraction or integrity verification fails.

#### `bool decryptFile(const std::string &src_path, const std::string &dest_path, const std::string &key, std::error_code &ec)`
- Source: `include/storage/backup_manager.h`:928
- Brief: Decrypt a file or directory payload into a destination path.
- Parameters:
  - `src_path` (const std::string &): Source file or directory.
  - `dest_path` (const std::string &): Destination path for decrypted output.
  - `key` (const std::string &): Caller-supplied encryption key material.
  - `ec` (std::error_code &): Receives the failure reason when the operation returns false.
- Return: true on success, or false when decryption or copy fails.
- Details: src_path Source file or directory. dest_path Destination path for decrypted output. key Caller-supplied encryption key material. ec Receives the failure reason when the operation returns false. true on success, or false when decryption or copy fails.

#### `RAIDConfig detectRAIDConfiguration()`
- Source: `include/storage/backup_manager.h`:449
- Brief: Detect RAID topology from environment variables.
- Parameters: none
- Return: Parsed RAID configuration. Returns RAIDMode::NONE when the environment does not describe a coordinated backup topology.
- Details: Reads THEMIS_RAID_GROUP, THEMIS_SHARD_ID, and THEMIS_SHARDS. Parsed RAID configuration. Returns RAIDMode::NONE when the environment does not describe a coordinated backup topology.

#### `Result< void > downloadFromCloud(const std::string &cloud_path, const std::string &local_path, StorageBackend backend, const std::map< std::string, std::string > &config)`
- Source: `include/storage/backup_manager.h`:956
- Brief: Download or mirror a backup payload from the selected backend.
- Parameters:
  - `cloud_path` (const std::string &): Provider URI or local mirror path.
  - `local_path` (const std::string &): Local destination directory or archive path.
  - `backend` (StorageBackend): Transport backend to use.
  - `config` (const std::map< std::string, std::string > &): Provider-specific configuration values.
- Return: Result<void> on success, or an error when validation, manifest retrieval, payload download, or local reconstruction fails.
- Details: cloud_path Provider URI or local mirror path. local_path Local destination directory or archive path. backend Transport backend to use. config Provider-specific configuration values. Result<void> on success, or an error when validation, manifest retrieval, payload download, or local reconstruction fails.

#### `bool encryptFile(const std::string &src_path, const std::string &dest_path, const std::string &key, std::error_code &ec)`
- Source: `include/storage/backup_manager.h`:915
- Brief: Encrypt a file or directory payload into a destination path.
- Parameters:
  - `src_path` (const std::string &): Source file or directory.
  - `dest_path` (const std::string &): Destination path for encrypted output.
  - `key` (const std::string &): Caller-supplied encryption key material.
  - `ec` (std::error_code &): Receives the failure reason when the operation returns false.
- Return: true on success, or false when encryption or copy fails.
- Details: src_path Source file or directory. dest_path Destination path for encrypted output. key Caller-supplied encryption key material. ec Receives the failure reason when the operation returns false. true on success, or false when encryption or copy fails.

#### `uint32_t estimateRTO(const std::string &backup_dir)`
- Source: `include/storage/backup_manager.h`:492
- Brief: Estimate the recovery time objective for a backup payload.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory to analyze.
- Return: Estimated RTO in seconds.
- Details: backup_dir Backup directory to analyze. Estimated RTO in seconds.

#### `std::string findLastFullBackup(const std::string &backup_dir)`
- Source: `include/storage/backup_manager.h`:967
- Brief: Find the most recent full backup beneath a base directory.
- Parameters:
  - `backup_dir` (const std::string &): Base backup directory.
- Return: Path to the latest full backup, or an empty string when no full backup can be found.
- Details: backup_dir Base backup directory. Path to the latest full backup, or an empty string when no full backup can be found.

#### `std::map< std::string, uint64_t > getBackupMetrics(const std::string &backup_dir)`
- Source: `include/storage/backup_manager.h`:484
- Brief: Collect size and count metrics for a backup directory.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory to analyze.
- Return: Map of metric name to metric value. Returns an empty map when the directory cannot be analyzed.
- Details: backup_dir Backup directory to analyze. Map of metric name to metric value. Returns an empty map when the directory cannot be analyzed.

#### `uint64_t getCurrentSequenceNumber() const`
- Source: `include/storage/backup_manager.h`:834
- Brief: Query the latest RocksDB sequence number.
- Parameters: none
- Return: Latest sequence number, or zero when the wrapper is unavailable.
- Details: Latest sequence number, or zero when the wrapper is unavailable.

#### `std::chrono::system_clock::time_point getRPO(const std::string &backup_dir)`
- Source: `include/storage/backup_manager.h`:501
- Brief: Determine the most recent recoverable point for a backup set.
- Parameters:
  - `backup_dir` (const std::string &): Base backup directory.
- Return: Timestamp representing the latest recoverable point. Returns the epoch when no recoverable backup can be determined.
- Details: backup_dir Base backup directory. Timestamp representing the latest recoverable point. Returns the epoch when no recoverable backup can be determined.

#### `std::string getTimestamp() const`
- Source: `include/storage/backup_manager.h`:774
- Brief: Return the current wall-clock timestamp formatted as YYYYMMDD_HHMMSS.
- Parameters: none
- Return: Timestamp string suitable for directory and manifest names.
- Details: Timestamp string suitable for directory and manifest names.

#### `Result< void > isBackupComplete(const std::string &backup_dir, const RAIDConfig &raid_config)`
- Source: `include/storage/backup_manager.h`:511
- Brief: Check whether a backup contains the shards required by a RAID topology.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory to inspect.
  - `raid_config` (const RAIDConfig &): RAID topology expected for the backup set.
- Return: Result<void> on success, or an error when the backup is incomplete or cannot be inspected.
- Details: backup_dir Backup directory to inspect. raid_config RAID topology expected for the backup set. Result<void> on success, or an error when the backup is incomplete or cannot be inspected.

#### `bool isBackupComplete(const std::string &backup_dir, const RAIDConfig &raid_config, std::error_code &ec)`
- Source: `include/storage/backup_manager.h`:460
- Brief: Check whether a backup contains the shards required by a RAID topology.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory to inspect.
  - `raid_config` (const RAIDConfig &): RAID topology expected for the backup set.
  - `ec` (std::error_code &): Receives the failure reason when the operation returns false.
- Return: true when every required shard is present, or false when the backup is incomplete or the directory cannot be inspected.
- Details: backup_dir Backup directory to inspect. raid_config RAID topology expected for the backup set. ec Receives the failure reason when the operation returns false. true when every required shard is present, or false when the backup is incomplete or the directory cannot be inspected.

#### `std::vector< std::string > listBackups(const std::string &backup_dir)`
- Source: `include/storage/backup_manager.h`:368
- Brief: List available backups beneath a base backup directory.
- Parameters:
  - `backup_dir` (const std::string &): Base backup directory.
- Return: Backup directory names sorted by timestamp. Returns an empty vector when the directory has no recognized backups.
- Details: List Backups. backup_dir Base backup directory. Backup directory names sorted by timestamp. Returns an empty vector when the directory has no recognized backups. param Input parameter. Return value. Implements listBackups without additional internal calls.

#### `std::vector< std::pair< std::string, std::string > > listScheduledBackups()`
- Source: `include/storage/backup_manager.h`:553
- Brief: List all schedules currently stored in the in-memory registry.
- Parameters: none
- Return: Vector of (schedule_id, cron_expression) pairs. Returns an empty vector when no schedules are registered.
- Details: Vector of (schedule_id, cron_expression) pairs. Returns an empty vector when no schedules are registered.

#### `Result< std::vector< std::string > > listSnapshots()`
- Source: `include/storage/backup_manager.h`:655
- Brief: List all snapshots under the default snapshot base path.
- Parameters: none
- Return: Sorted list of snapshot directory paths (oldest first).
- Details: Sorted list of snapshot directory paths (oldest first).

#### `RAIDMode parseRAIDMode(const std::string &mode_str)`
- Source: `include/storage/backup_manager.h`:842
- Brief: Parse a textual RAID mode identifier.
- Parameters:
  - `mode_str` (const std::string &): Mode string such as raid5.
- Return: Parsed RAID mode, or RAIDMode::NONE for unrecognized inputs.
- Details: mode_str Mode string such as raid5. Parsed RAID mode, or RAIDMode::NONE for unrecognized inputs.

#### `bool performPITR(const std::string &dest_dir, const PITROptions &pitr_options, std::error_code &ec, RecoveryStats *stats=nullptr)`
- Source: `include/storage/backup_manager.h`:337
- Brief: Perform point-in-time recovery from a backup chain and WAL replay target.
- Parameters:
  - `dest_dir` (const std::string &): Base backup directory containing the recovery chain.
  - `pitr_options` (const PITROptions &): PITR target timestamp and optional LSN metadata.
  - `ec` (std::error_code &): Input/output parameter.
  - `stats` (RecoveryStats *): Optional pointer that receives recovery metrics on success.
- Return: true on success, or false when the base restore fails, the target cannot be satisfied, or WAL replay reports failure.
- Details: Perform PITR. dest_dir Base backup directory containing the recovery chain. pitr_options PITR target timestamp and optional LSN metadata. ec Receives the failure reason when the operation returns false. stats Optional pointer that receives recovery metrics on success. true on success, or false when the base restore fails, the target cannot be satisfied, or WAL replay reports failure. param Input parameter. param Input parameter. ec Input/output parameter. param Input/output parameter. True when the operation succeeds. Calls: std::make_error_code().

#### `void processScheduledBackups()`
- Source: `include/storage/backup_manager.h`:755
- Brief: Evaluate all registered schedules and launch due backups.
- Parameters: none
- Details: The method is called from the scheduler loop after each wake-up and from direct maintenance paths that need to force a scan.

#### `std::string raidModeToString(RAIDMode mode)`
- Source: `include/storage/backup_manager.h`:850
- Brief: Convert a RAID mode enum to its manifest/environment string form.
- Parameters:
  - `mode` (RAIDMode): RAID mode to format.
- Return: Lowercase RAID mode name.
- Details: mode RAID mode to format. Lowercase RAID mode name.

#### `Result< std::vector< FileIntegrityInfo > > readIntegrityManifest(const std::string &backup_dir)`
- Source: `include/storage/backup_manager.h`:1000
- Brief: Load INTEGRITY_MANIFEST.json from a backup directory.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory containing the manifest.
- Return: Result<std::vector<FileIntegrityInfo>> containing parsed entries on success, or an error when the manifest is missing or malformed.
- Details: backup_dir Backup directory containing the manifest. Result<std::vector<FileIntegrityInfo>> containing parsed entries on success, or an error when the manifest is missing or malformed.

#### `Result< void > readManifest(const std::string &backup_dir, std::string &type, uint64_t &sequence_number)`
- Source: `include/storage/backup_manager.h`:796
- Brief: Read backup type and sequence metadata from MANIFEST.json.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory containing the manifest.
  - `type` (std::string &): Receives the parsed backup type on success.
  - `sequence_number` (uint64_t &): Receives the parsed sequence number on success.
- Return: Result<void> on success, or an error when the manifest is missing or malformed.
- Details: backup_dir Backup directory containing the manifest. type Receives the parsed backup type on success. sequence_number Receives the parsed sequence number on success. Result<void> on success, or an error when the manifest is missing or malformed.

#### `Result< uint32_t > repairDecompressedBackup(const std::string &backup_dir, const std::string &compressed_source="")`
- Source: `include/storage/backup_manager.h`:438
- Brief: Attempt to repair files that fail post-decompression verification.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory to repair.
  - `compressed_source` (const std::string &): Optional original compressed archive used to retry recovery of corrupted files.
- Return: Result<uint32_t> containing the number of repaired files on success, or an error when corruption cannot be isolated or the repair path fails.
- Details: backup_dir Backup directory to repair. compressed_source Optional original compressed archive used to retry recovery of corrupted files. Result<uint32_t> containing the number of repaired files on success, or an error when corruption cannot be isolated or the repair path fails. This is an advanced recovery method: Identifies corrupted files during verification Attempts to repair from original compressed source if provided If source unavailable, logs a warning and returns a repair error; it does not quarantine files

#### `bool restoreCollections(const std::string &src_dir, const std::vector< std::string > &collections, std::error_code &ec)`
- Source: `include/storage/backup_manager.h`:349
- Brief: Restore a selected subset of collections from a checkpoint backup.
- Parameters:
  - `src_dir` (const std::string &): Source backup directory.
  - `collections` (const std::vector< std::string > &): Collection or column-family names to restore.
  - `ec` (std::error_code &): Input/output parameter.
- Return: true on success, or false when selective ingest is unavailable or the restore pipeline fails.
- Details: Restore Collections. src_dir Source backup directory. collections Collection or column-family names to restore. ec Receives the failure reason when the operation returns false. true on success, or false when selective ingest is unavailable or the restore pipeline fails. param Input parameter. param Input parameter. ec Input/output parameter. True when the operation succeeds. Calls: std::make_error_code().

#### `Result< void > restoreFromBackup(const std::string &src_dir)`
- Source: `include/storage/backup_manager.h`:359
- Brief: Restore the database from a backup using the Result API.
- Parameters:
  - `src_dir` (const std::string &): Source backup directory.
- Return: Result<void> on success, or an error when the restore fails.
- Details: Restore From Backup. src_dir Source backup directory. Result<void> on success, or an error when the restore fails. param Input parameter. Return value. Implements restoreFromBackup without additional internal calls.

#### `bool restoreFromBackup(const std::string &src_dir, std::error_code &ec, RecoveryStats *stats=nullptr)`
- Source: `include/storage/backup_manager.h`:324
- Brief: Restore the database from a full or incremental backup chain.
- Parameters:
  - `src_dir` (const std::string &): Source backup directory.
  - `ec` (std::error_code &): Input/output parameter.
  - `stats` (RecoveryStats *): Optional pointer that receives recovery metrics on success.
- Return: true on success, or false when manifest validation, payload extraction, or RocksDB restore fails.
- Details: Restore From Backup. For RAID5/6 deployments, restore requires all data and parity shards that belong to the backup set. src_dir Source backup directory. ec Receives the failure reason when the operation returns false. stats Optional pointer that receives recovery metrics on success. true on success, or false when manifest validation, payload extraction, or RocksDB restore fails. param Input parameter. ec Input/output parameter. param Input/output parameter. True when the operation succeeds. Calls: std::make_error_code().

#### `Result< void > restoreFromCloud(const std::string &cloud_uri, const std::string &local_restore_path, const BackupOptions &options)`
- Source: `include/storage/backup_manager.h`:600
- Brief: Restore a backup payload from a provider-specific source into a local directory.
- Parameters:
  - `cloud_uri` (const std::string &): Provider URI or local mirror path, depending on options.storage.
  - `local_restore_path` (const std::string &): Local destination directory for the restored payload.
  - `options` (const BackupOptions &): Restore options, backend selection, and provider-specific configuration.
- Return: Result<void> on success, or an Error when validation fails, the source cannot be copied, or the provider backend is unavailable.
- Details: For StorageBackend::LOCAL, the method copies the source tree from a file:///absolute/path URI or absolute path into local_restore_path. Remote backends fetch a manifest object plus the referenced payload objects from the provider URI and reconstruct the original backup tree locally. Individual remote payload objects currently transfer in-memory and are rejected when they exceed 256 MiB. cloud_uri Provider URI or local mirror path, depending on options.storage. local_restore_path Local destination directory for the restored payload. options Restore options, backend selection, and provider-specific configuration. Result<void> on success, or an Error when validation fails, the source cannot be copied, or the provider backend is unavailable.

#### `Result< void > restoreFromSnapshot(const std::string &snapshot_id, const std::string &restore_pvc="")`
- Source: `include/storage/backup_manager.h`:637
- Brief: Restore the database from a previously created snapshot.
- Parameters:
  - `snapshot_id` (const std::string &): Absolute path to the snapshot directory (as returned by createSnapshot()).
  - `restore_pvc` (const std::string &): Reserved for future K8s PVC integration; pass an empty string for local restores.
- Return: Result<void> on success, Error on failure.
- Details: The running database is closed, its data directory is replaced with the contents of the snapshot directory, and the database is reopened. snapshot_id Absolute path to the snapshot directory (as returned by createSnapshot()). restore_pvc Reserved for future K8s PVC integration; pass an empty string for local restores. Result<void> on success, Error on failure.

#### `void runScheduledBackupLoop()`
- Source: `include/storage/backup_manager.h`:748
- Brief: Drive the scheduled-backup worker until shutdown.
- Parameters: none
- Details: Polls the in-memory schedule registry, evaluates cron triggers, and dispatches due backup jobs while the scheduler is running.

#### `Result< std::string > scheduleBackup(const std::string &schedule_cron, const std::string &backup_type, const BackupOptions &options)`
- Source: `include/storage/backup_manager.h`:534
- Brief: Register an automatic backup schedule in the in-process schedule registry.
- Parameters:
  - `schedule_cron` (const std::string &): Cron-style expression with five space-separated fields (for example, 0 2 * * *).
  - `backup_type` (const std::string &): Backup class to register (for example full, incremental, or differential).
  - `options` (const BackupOptions &): Backup options captured with the schedule entry.
- Return: Result<std::string> containing the generated schedule identifier on success, or an Error when the inputs are invalid.
- Details: The current implementation validates the cron-like expression, allocates a stable schedule identifier, and stores the schedule metadata in memory for later inspection or cancellation. It does not spawn a background executor or persist schedules across process restarts. schedule_cron Cron-style expression with five space-separated fields (for example, 0 2 * * *). backup_type Backup class to register (for example full, incremental, or differential). options Backup options captured with the schedule entry. Result<std::string> containing the generated schedule identifier on success, or an Error when the inputs are invalid.

#### `void setCfSstIngestFn(CfSstIngestFn fn)`
- Source: `include/storage/backup_manager.h`:717
- Brief: Inject a per-CF SST ingest function used by restoreCollections().
- Parameters:
  - `fn` (CfSstIngestFn): Input parameter.
- Details: When set, restoreCollections() calls this function before falling back to full checkpoint restore. fn Input parameter.

#### `void setWalReplayFn(WalReplayFn fn)`
- Source: `include/storage/backup_manager.h`:691
- Brief: Inject a WAL-replay function used by performPITR() after the base snapshot has been restored.
- Parameters:
  - `fn` (WalReplayFn): Must not throw; returning false is treated as a replay failure and performPITR() will return false too.
- Details: Without an injected function performPITR() silently omits the WAL replay step (original behaviour). When set, the function is called with the restore directory and target time so the caller can apply the remaining WAL delta. fn Must not throw; returning false is treated as a replay failure and performPITR() will return false too.

#### `bool shouldRunScheduledBackup(const ScheduledBackupEntry &entry, const std::tm &current_time) const`
- Source: `include/storage/backup_manager.h`:763
- Brief: Return true when a scheduled backup is due at current_time.
- Parameters:
  - `entry` (const ScheduledBackupEntry &): Registered schedule entry to evaluate.
  - `current_time` (const std::tm &): Wall-clock time used for the cron match.
- Return: true if the entry should trigger now.
- Details: entry Registered schedule entry to evaluate. current_time Wall-clock time used for the cron match. true if the entry should trigger now.

#### `Result< std::string > uploadBackupToCloud(const std::string &local_backup_path, const std::string &cloud_uri, const BackupOptions &options)`
- Source: `include/storage/backup_manager.h`:580
- Brief: Copy a finished backup to a provider-specific destination.
- Parameters:
  - `local_backup_path` (const std::string &): Existing local backup directory or archive.
  - `cloud_uri` (const std::string &): Provider URI or local mirror path, depending on options.storage.
  - `options` (const BackupOptions &): Transfer options, backend selection, and provider-specific configuration.
- Return: Result<std::string> containing the destination URI/path on success, or an Error when validation fails or the provider backend is unavailable.
- Details: Supported destinations: StorageBackend::LOCAL: local filesystem mirror via file:///absolute/path or an absolute path. StorageBackend::S3: s3://bucket/path (requires provider integration). StorageBackend::AZURE: azure://account/container/path or azure://container/path. When the account segment is omitted, the runtime derives the service endpoint from the configured Azure connection string / environment. StorageBackend::GCS: gs://bucket/path (requires provider integration). Remote transfers serialize the backup as a manifest plus per-file payload objects beneath the requested provider URI so that restore can reconstruct the directory tree without relying on provider-side object listing. Individual remote payload objects currently transfer in-memory and are rejected when they exceed 256 MiB. local_backup_path Existing local backup directory or archive. cloud_uri Provider URI or local mirror path, depending on options.storage. options Transfer options, backend selection, and provider-specific configuration. Result<std::string> containing the destination URI/path on success, or an Error when validation fails or the provider backend is unavailable.

#### `Result< void > uploadToCloud(const std::string &local_path, const std::string &cloud_path, StorageBackend backend, const std::map< std::string, std::string > &config)`
- Source: `include/storage/backup_manager.h`:942
- Brief: Upload or mirror a local backup payload to the selected backend.
- Parameters:
  - `local_path` (const std::string &): Existing local backup directory or archive.
  - `cloud_path` (const std::string &): Provider URI or local mirror path.
  - `backend` (StorageBackend): Transport backend to use.
  - `config` (const std::map< std::string, std::string > &): Provider-specific configuration values.
- Return: Result<void> on success, or an error when validation, manifest generation, upload, or cleanup fails.
- Details: local_path Existing local backup directory or archive. cloud_path Provider URI or local mirror path. backend Transport backend to use. config Provider-specific configuration values. Result<void> on success, or an error when validation, manifest generation, upload, or cleanup fails.

#### `Result< std::vector< std::string > > verifyAllChecksums(const std::string &backup_dir, const std::vector< FileIntegrityInfo > &integrity_map)`
- Source: `include/storage/backup_manager.h`:1021
- Brief: Verify every tracked file in a decompressed backup payload.
- Parameters:
  - `backup_dir` (const std::string &): Decompressed backup directory to verify.
  - `integrity_map` (const std::vector< FileIntegrityInfo > &): Expected integrity entries for the payload.
- Return: Result<std::vector<std::string>> containing the relative paths of corrupted or missing files, or an error when verification cannot run.
- Details: backup_dir Decompressed backup directory to verify. integrity_map Expected integrity entries for the payload. Result<std::vector<std::string>> containing the relative paths of corrupted or missing files, or an error when verification cannot run.

#### `Result< void > verifyBackup(const std::string &backup_dir)`
- Source: `include/storage/backup_manager.h`:380
- Brief: Verify a backup payload, manifest, and shard completeness.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory to verify.
- Return: Result<void> on success, or an error when checksums, manifests, or shard requirements fail validation.
- Details: Verify Backup. For RAID5/6 deployments, verification ensures that all required data and parity shards are present and internally consistent. backup_dir Backup directory to verify. Result<void> on success, or an error when checksums, manifests, or shard requirements fail validation. param Input parameter. Return value. Implements verifyBackup without additional internal calls.

#### `Result< void > verifyChecksum(const std::string &file_path, const std::string &expected_checksum)`
- Source: `include/storage/backup_manager.h`:827
- Brief: Verify that a file matches an expected SHA-256 checksum.
- Parameters:
  - `file_path` (const std::string &): File to verify.
  - `expected_checksum` (const std::string &): Expected lowercase hexadecimal SHA-256 digest.
- Return: Result<void> on success, or an error when the checksum does not match or the file cannot be read.
- Details: file_path File to verify. expected_checksum Expected lowercase hexadecimal SHA-256 digest. Result<void> on success, or an error when the checksum does not match or the file cannot be read.

#### `Result< void > verifyDecompressedBackup(const std::string &backup_dir)`
- Source: `include/storage/backup_manager.h`:421
- Brief: Verify the payload extracted from a compressed backup.
- Parameters:
  - `backup_dir` (const std::string &): Decompressed backup directory to verify.
- Return: Result<void> on success, including the backward-compatible case where the backup predates INTEGRITY_MANIFEST.json; returns an error when manifest loading fails or file checksums do not match.
- Details: backup_dir Decompressed backup directory to verify. Result<void> on success, including the backward-compatible case where the backup predates INTEGRITY_MANIFEST.json; returns an error when manifest loading fails or file checksums do not match. Performs post-decompression verification: Missing INTEGRITY_MANIFEST.json is treated as a legacy backup and skips verification Reads integrity manifest from backup Calculates SHA-256 checksum of each file Compares against stored checksums Reports corrupted files with details

#### `Result< bool > verifyFileChecksum(const std::string &file_path, const std::string &expected_checksum)`
- Source: `include/storage/backup_manager.h`:1010
- Brief: Compare a single file with an expected checksum value.
- Parameters:
  - `file_path` (const std::string &): File to verify.
  - `expected_checksum` (const std::string &): Expected lowercase hexadecimal SHA-256 digest.
- Return: Result<bool> containing true for a match and false for a mismatch; returns an error when the file cannot be read.
- Details: file_path File to verify. expected_checksum Expected lowercase hexadecimal SHA-256 digest. Result<bool> containing true for a match and false for a mismatch; returns an error when the file cannot be read.

#### `Result< void > verifyRAIDShardsInBackup(const std::string &backup_dir, const RAIDConfig &raid_config)`
- Source: `include/storage/backup_manager.h`:873
- Brief: Verify that the backup contains every shard required by the RAID topology.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory to inspect.
  - `raid_config` (const RAIDConfig &): Expected RAID topology.
- Return: Result<void> on success, or an error when required shards are missing or the directory cannot be traversed.
- Details: backup_dir Backup directory to inspect. raid_config Expected RAID topology. Result<void> on success, or an error when required shards are missing or the directory cannot be traversed.

#### `bool verifyRAIDShardsInBackup(const std::string &backup_dir, const RAIDConfig &raid_config, std::error_code &ec)`
- Source: `include/storage/backup_manager.h`:861
- Brief: Verify that the backup contains every shard required by the RAID topology.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory to inspect.
  - `raid_config` (const RAIDConfig &): Expected RAID topology.
  - `ec` (std::error_code &): Receives the failure reason when the operation returns false.
- Return: true on success, or false when required shards are missing or the directory cannot be traversed.
- Details: backup_dir Backup directory to inspect. raid_config Expected RAID topology. ec Receives the failure reason when the operation returns false. true on success, or false when required shards are missing or the directory cannot be traversed.

#### `Result< void > verifySnapshot(const std::string &snapshot_dir)`
- Source: `include/storage/backup_manager.h`:648
- Brief: Verify a snapshot by checking its manifest and the presence of the expected SST / MANIFEST files.
- Parameters:
  - `snapshot_dir` (const std::string &): Absolute path to the snapshot directory.
- Return: Result<void> on success, Error if the snapshot is corrupted or incomplete.
- Details: snapshot_dir Absolute path to the snapshot directory. Result<void> on success, Error if the snapshot is corrupted or incomplete.

#### `Result< void > writeIntegrityManifest(const std::string &backup_dir, const std::vector< FileIntegrityInfo > &integrity_map)`
- Source: `include/storage/backup_manager.h`:990
- Brief: Write INTEGRITY_MANIFEST.json for a backup payload.
- Parameters:
  - `backup_dir` (const std::string &): Backup directory that receives the manifest.
  - `integrity_map` (const std::vector< FileIntegrityInfo > &): Integrity entries to serialize.
- Return: Result<void> on success, or an error when the manifest cannot be written.
- Details: backup_dir Backup directory that receives the manifest. integrity_map Integrity entries to serialize. Result<void> on success, or an error when the manifest cannot be written.

#### `~BackupManager()`
- Source: `include/storage/backup_manager.h`:220
- Brief: Destroy the backup manager.
- Parameters: none
- Details: Destruction only releases in-memory state such as the schedule registry. It does not delete backups or mutate persisted storage state.

### themis::BaseEntity

#### `BaseEntity()=default`
- Source: `include/storage/base_entity.h`:77
- Brief: n/a
- Parameters: none

#### `BaseEntity(std::string_view pk)`
- Source: `include/storage/base_entity.h`:83
- Brief: Construct an entity with the given primary key.
- Parameters:
  - `pk` (std::string_view): Primary key used to identify the entity in storage.
- Details: pk Primary key used to identify the entity in storage.

#### `BaseEntity(std::string_view pk, Blob blob, Format format=Format::BINARY)`
- Source: `include/storage/base_entity.h`:85
- Brief: n/a
- Parameters:
  - `pk` (std::string_view): n/a
  - `blob` (Blob): n/a
  - `format` (Format): n/a

#### `BaseEntity(std::string_view pk, const FieldMap &fields)`
- Source: `include/storage/base_entity.h`:84
- Brief: n/a
- Parameters:
  - `pk` (std::string_view): n/a
  - `fields` (const FieldMap &): n/a

#### `void clear()`
- Source: `include/storage/base_entity.h`:256
- Brief: Clear all data.
- Parameters: none
- Details: Clear. Calls: invalidateCache().

#### `void clearGeometry()`
- Source: `include/storage/base_entity.h`:205
- Brief: Clear geometry (remove geo capability).
- Parameters: none
- Details: Clear Geometry. Calls: reset().

#### `BaseEntity deserialize(std::string_view pk, const Blob &blob)`
- Source: `include/storage/base_entity.h`:171
- Brief: Deserialize from binary blob.
- Parameters:
  - `pk` (std::string_view): Input parameter.
  - `blob` (const Blob &): Input parameter.
- Return: Return value.
- Details: Deserialize. pk Input parameter. blob Input parameter. Return value. Calls: empty(), BaseEntity().

#### `void ensureCache() const`
- Source: `include/storage/base_entity.h`:275
- Brief: Parse blob into field cache.
- Parameters: none

#### `Attributes extractAllFields() const`
- Source: `include/storage/base_entity.h`:215
- Brief: Return all indexable fields as name/value string pairs.
- Parameters: none
- Details: Used by secondary index maintenance paths that need a string-only view of the entity state.

#### `std::optional< std::string > extractField(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:182
- Brief: Extract a single field without fully deserializing the entity.
- Parameters:
  - `field_name` (std::string_view): Field name to extract.
- Return: String value for the field, or std::nullopt when the field is absent or not representable as a string.
- Details: field_name Field name to extract. String value for the field, or std::nullopt when the field is absent or not representable as a string.

#### `Attributes extractFieldsWithPrefix(std::string_view prefix) const`
- Source: `include/storage/base_entity.h`:218
- Brief: Extract fields matching a prefix (e.g., "metadata.*").
- Parameters:
  - `prefix` (std::string_view): n/a

#### `std::optional< std::vector< float > > extractVector(std::string_view field_name="embedding") const`
- Source: `include/storage/base_entity.h`:185
- Brief: Extract vector embedding field (for ANN index).
- Parameters:
  - `field_name` (std::string_view): n/a

#### `BaseEntity fromFields(std::string_view pk, const FieldMap &fields)`
- Source: `include/storage/base_entity.h`:168
- Brief: Create from field map.
- Parameters:
  - `pk` (std::string_view): Input parameter.
  - `fields` (const FieldMap &): Input parameter.
- Return: Return value.
- Details: From Fields. pk Input parameter. fields Input parameter. Return value. Calls: BaseEntity().

#### `BaseEntity fromJson(std::string_view pk, std::string_view json_str)`
- Source: `include/storage/base_entity.h`:165
- Brief: Create from JSON string (using simdjson for speed).
- Parameters:
  - `pk` (std::string_view): Input parameter.
  - `json_str` (std::string_view): Input parameter.
- Return: Return value.
- Details: ===== Factory Methods ===== pk Input parameter. json_str Input parameter. Return value. Calls: entity(), Blob(), begin(), end().

#### `FieldMap getAllFields() const`
- Source: `include/storage/base_entity.h`:154
- Brief: Get all fields (full parse).
- Parameters: none

#### `const Blob & getBlob() const`
- Source: `include/storage/base_entity.h`:94
- Brief: Get binary blob.
- Parameters: none

#### `size_t getBlobSize() const`
- Source: `include/storage/base_entity.h`:250
- Brief: Get blob size in bytes.
- Parameters: none

#### `std::optional< Value > getField(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:108
- Brief: Get field value (returns nullopt if not found).
- Parameters:
  - `field_name` (std::string_view): n/a

#### `std::optional< bool > getFieldAsBool(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:131
- Brief: Get field as bool.
- Parameters:
  - `field_name` (std::string_view): n/a

#### `std::optional< double > getFieldAsDouble(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:128
- Brief: Get field as double.
- Parameters:
  - `field_name` (std::string_view): n/a

#### `std::optional< int64_t > getFieldAsInt(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:125
- Brief: Get field as int64.
- Parameters:
  - `field_name` (std::string_view): n/a

#### `std::optional< std::string > getFieldAsString(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:122
- Brief: Get field as string (with type conversion).
- Parameters:
  - `field_name` (std::string_view): n/a

#### `std::optional< std::vector< std::string > > getFieldAsStringArray(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:143
- Brief: Get field as a string array.
- Parameters:
  - `field_name` (std::string_view): Input parameter.
- Return: Decoded string array, or std::nullopt when the field is absent or cannot be interpreted as a string list.
- Details: field_name Input parameter. Decoded string array, or std::nullopt when the field is absent or cannot be interpreted as a string list. Attempts to decode the named field as an ordered list of strings. The following encodings are recognised, in priority order: 1. A JSON array stored as a plain string value, e.g. ["a","b","c"]. 2. A comma-separated plain string (legacy format, backward-compatible read). Returns std::nullopt when the field is absent or cannot be decoded as any string-like type. Returns an empty vector for an empty array or an empty string.

#### `std::optional< std::vector< float > > getFieldAsVector(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:134
- Brief: Get field as float vector (for embeddings).
- Parameters:
  - `field_name` (std::string_view): n/a

#### `int64_t getFieldInt(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:119
- Brief: Return the named field as a signed 64-bit integer.
- Parameters:
  - `field_name` (std::string_view): Field name to convert.
- Return: Field value converted to int64_t, or 0 when the field is absent or cannot be converted.
- Details: field_name Field name to convert. Field value converted to int64_t, or 0 when the field is absent or cannot be converted.

#### `std::string getFieldString(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:111
- Brief: Convenience helpers returning defaults.
- Parameters:
  - `field_name` (std::string_view): n/a

#### `Format getFormat() const`
- Source: `include/storage/base_entity.h`:100
- Brief: Get storage format.
- Parameters: none

#### `const std::optional< geo::GeoSidecar > & getGeoSidecar() const`
- Source: `include/storage/base_entity.h`:199
- Brief: Get geo sidecar (MBR, centroid, z-range).
- Parameters: none

#### `const std::optional< Blob > & getGeometry() const`
- Source: `include/storage/base_entity.h`:193
- Brief: Get geometry blob (EWKB format).
- Parameters: none

#### `const std::string & getPrimaryKey() const`
- Source: `include/storage/base_entity.h`:88
- Brief: Get primary key.
- Parameters: none

#### `std::optional< size_t > getRotationPosition(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:236
- Brief: Return the rotation position for a rotated embedding.
- Parameters:
  - `field_name` (std::string_view): Field name to inspect.
- Return: Rotation position, or std::nullopt when the field is not rotated.
- Details: field_name Field name to inspect. Rotation position, or std::nullopt when the field is not rotated.

#### `std::optional< std::string > getRotationType(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:245
- Brief: Return the rotation type for a relationally rotated embedding.
- Parameters:
  - `field_name` (std::string_view): Field name to inspect.
- Return: Rotation type string, or std::nullopt when the field is not relationally rotated.
- Details: field_name Field name to inspect. Rotation type string, or std::nullopt when the field is not relationally rotated.

#### `bool hasField(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:105
- Brief: Check if field exists.
- Parameters:
  - `field_name` (std::string_view): n/a

#### `bool hasGeometry() const`
- Source: `include/storage/base_entity.h`:190
- Brief: Check if entity has geometry.
- Parameters: none

#### `bool hasRotatedEmbedding(std::string_view field_name) const`
- Source: `include/storage/base_entity.h`:228
- Brief: Check whether the named field has rotation metadata.
- Parameters:
  - `field_name` (std::string_view): Field name to inspect.
- Return: true when a rotated embedding is present.
- Details: field_name Field name to inspect. true when a rotated embedding is present.

#### `void invalidateCache()`
- Source: `include/storage/base_entity.h`:280
- Brief: Invalidate cache (after blob modification).
- Parameters: none
- Details: Invalidate Cache. Calls: reset().

#### `bool isEmpty() const`
- Source: `include/storage/base_entity.h`:253
- Brief: Check if entity is empty.
- Parameters: none

#### `FieldMap parseBinary() const`
- Source: `include/storage/base_entity.h`:290
- Brief: Parse the blob as the native binary entity format.
- Parameters: none

#### `FieldMap parseJson() const`
- Source: `include/storage/base_entity.h`:285
- Brief: Parse the blob as JSON and populate the field cache.
- Parameters: none

#### `void rebuildBlob()`
- Source: `include/storage/base_entity.h`:295
- Brief: Rebuild the serialized blob from the cached field map.
- Parameters: none
- Details: ===== Serialization ===== Calls: empty(), clear(), beginObject(), size(), encodeString(), std::visit(), constexpr(), encodeNull().

#### `Blob serialize() const`
- Source: `include/storage/base_entity.h`:159
- Brief: Serialize current fields to binary blob.
- Parameters: none

#### `void setBlob(Blob blob, Format format=Format::BINARY)`
- Source: `include/storage/base_entity.h`:97
- Brief: Set binary blob (invalidates cache).
- Parameters:
  - `blob` (Blob): Input parameter.
  - `format` (Format): Input parameter.
- Details: ===== Blob Management ===== blob Input parameter. format Input parameter. Calls: std::move(), invalidateCache().

#### `void setField(std::string_view field_name, const Value &value)`
- Source: `include/storage/base_entity.h`:151
- Brief: Set field value (modifies blob) Sets a field in the entity's field map, triggering a blob rebuild for serialization.
- Parameters:
  - `field_name` (std::string_view): Name of the field.
  - `value` (const Value &): Input parameter.
- Details: Set Field. field_name Input parameter. value Input parameter. Implements fail-closed validation: rejects empty field_name to prevent silent field map corruption. field_name Field identifier (non-empty std::string_view required) value Value to set for this field Fail-Closed Behavior: If field_name is empty, this method logs an error and returns without modifying the field cache. This prevents creating corrupt field map entries with empty keys that would propagate through getAllFields() and toJson() calls. getAllFields(), toJson() — downstream methods that depend on valid field keys field_name Name of the field. value Input parameter. Calls: empty(), spdlog::error(), ensureCache(), use_count(), std::string(), rebuildBlob().

#### `void setGeoSidecar(const geo::GeoSidecar &sidecar)`
- Source: `include/storage/base_entity.h`:202
- Brief: Set geo sidecar (computed from geometry).
- Parameters:
  - `sidecar` (const geo::GeoSidecar &): Input parameter.
- Details: Set Geo Sidecar. sidecar Input parameter. Implements setGeoSidecar without additional internal calls.

#### `void setGeometry(const Blob &ewkb)`
- Source: `include/storage/base_entity.h`:196
- Brief: Set geometry blob (EWKB format).
- Parameters:
  - `ewkb` (const Blob &): Input parameter.
- Details: ===== Geo Support (Cross-Cutting Capability) ===== ewkb Input parameter. Calls: geo::EWKBParser::parse(), geo::EWKBParser::computeSidecar(), reset().

#### `void setPrimaryKey(std::string_view pk)`
- Source: `include/storage/base_entity.h`:91
- Brief: Set primary key.
- Parameters:
  - `pk` (std::string_view): n/a

#### `std::string toJson() const`
- Source: `include/storage/base_entity.h`:162
- Brief: Serialize to JSON string.
- Parameters: none

### themis::BatchWriteOptimizer

#### `BatchWriteOptimizer()`
- Source: `include/storage/batch_write_optimizer.h`:74
- Brief: n/a
- Parameters: none

#### `BatchWriteOptimizer(const Config &config)`
- Source: `include/storage/batch_write_optimizer.h`:81
- Brief: Construct an optimizer with an explicit configuration.
- Parameters:
  - `config` (const Config &): Durability and WAL policy to apply to generated write options.
- Details: config Durability and WAL policy to apply to generated write options.

#### `rocksdb::WriteOptions getOptimizedWriteOptions() const`
- Source: `include/storage/batch_write_optimizer.h`:89
- Brief: Get optimized WriteOptions for batch operations.
- Parameters: none
- Return: Configured WriteOptions optimized for batching
- Details: Configured WriteOptions optimized for batching

#### `Stats getStats() const`
- Source: `include/storage/batch_write_optimizer.h`:106
- Brief: Return a snapshot of optimizer statistics.
- Parameters: none

#### `Config recommendedConfigForUseCase(const std::string &use_case)`
- Source: `include/storage/batch_write_optimizer.h`:122
- Brief: Create recommended configuration for use case.
- Parameters:
  - `use_case` (const std::string &): Input parameter.
- Return: Configuration tuned for the requested workload.
- Details: Recommended Config For Use Case. use_case Human-readable workload label such as bulk-load or transactional-write. Configuration tuned for the requested workload. use_case Input parameter. Return value. Implements recommendedConfigForUseCase without additional internal calls.

#### `void recordBatchWrite(size_t items, double latency_ms)`
- Source: `include/storage/batch_write_optimizer.h`:113
- Brief: Record batch write for statistics.
- Parameters:
  - `items` (size_t): Input parameter.
  - `latency_ms` (double): Input parameter.
- Details: Record Batch Write. items Input parameter. latency_ms Input parameter. items Input parameter. latency_ms Input parameter. Calls: fetch_add(), load(), compare_exchange_weak().

#### `void validateConfig(const Config &config)`
- Source: `include/storage/batch_write_optimizer.h`:128
- Brief: Validate configuration and warn about dangerous settings.
- Parameters:
  - `config` (const Config &): Input parameter.
- Details: Validate Config. config Input parameter. config Input parameter. Calls: THEMIS_INFO().

#### `~BatchWriteOptimizer()`
- Source: `include/storage/batch_write_optimizer.h`:82
- Brief: n/a
- Parameters: none

### themis::CompactionManager

#### `CompactionManager(const CompactionManager &)=delete`
- Source: `include/storage/compaction_manager.h`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CompactionManager &): n/a

#### `CompactionManager(std::shared_ptr< RocksDBWrapper > db)`
- Source: `include/storage/compaction_manager.h`:104
- Brief: Construct a CompactionManager with default configuration.
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): Shared ownership of the underlying RocksDB instance. Must already be open.
- Details: db Shared ownership of the underlying RocksDB instance. Must already be open.

#### `CompactionManager(std::shared_ptr< RocksDBWrapper > db, const Config &config)`
- Source: `include/storage/compaction_manager.h`:113
- Brief: Construct a CompactionManager.
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): Shared ownership of the underlying RocksDB instance. Must already be open.
  - `config` (const Config &): Configuration options.
- Details: db Shared ownership of the underlying RocksDB instance. Must already be open. config Configuration options.

#### `void backgroundLoop()`
- Source: `include/storage/compaction_manager.h`:199
- Brief: Background Loop.
- Parameters: none
- Details: Calls: load(), lock(), wait_for(), unlock(), runGC().

#### `Result< void > compactAll()`
- Source: `include/storage/compaction_manager.h`:136
- Brief: Compact the entire keyspace.
- Parameters: none
- Return: Return value.
- Details: Compact All. This is an expensive operation – prefer compactRange() in production. Return value. Calls: isOpen(), ErrVoid(), compactRange(), fetch_add(), OkVoid().

#### `Result< void > compactRange(std::string_view start_key, std::string_view end_key)`
- Source: `include/storage/compaction_manager.h`:129
- Brief: Compact a specific key range [start_key, end_key).
- Parameters:
  - `start_key` (std::string_view): Input parameter.
  - `end_key` (std::string_view): Input parameter.
- Return: Return value.
- Details: Compact Range. Blocks until RocksDB finishes the compaction. start_key Input parameter. end_key Input parameter. Return value. Calls: isOpen(), ErrVoid(), fetch_add(), OkVoid().

#### `Config getConfig() const`
- Source: `include/storage/compaction_manager.h`:191
- Brief: n/a
- Parameters: none
- Details: Return the active configuration.

#### `bool isBackgroundGCRunning() const`
- Source: `include/storage/compaction_manager.h`:175
- Brief: n/a
- Parameters: none
- Details: Return true if the background GC thread is currently running.

#### `CompactionManager & operator=(const CompactionManager &)=delete`
- Source: `include/storage/compaction_manager.h`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CompactionManager &): n/a

#### `void recordDeletions(uint64_t count=1)`
- Source: `include/storage/compaction_manager.h`:146
- Brief: Notify the manager that count DEL entries were written.
- Parameters:
  - `count` (uint64_t): Input parameter.
- Details: Record Deletions. When the tombstone count reaches config.tombstone_gc_threshold, a compaction is triggered on the next background GC pass. count Input parameter. Calls: fetch_add().

#### `Result< void > runGC(bool force=false)`
- Source: `include/storage/compaction_manager.h`:156
- Brief: Run a GC pass immediately (synchronous).
- Parameters:
  - `force` (bool): Input parameter.
- Return: Result<void> – ok on success, error on compaction failure.
- Details: Run GC. Triggers a full compaction if the tombstone threshold has been reached, or if force is true. Result<void> – ok on success, error on compaction failure. force Input parameter. Return value. Calls: load(), OkVoid(), compactAll(), store(), fetch_add().

#### `void setConfig(const Config &config)`
- Source: `include/storage/compaction_manager.h`:188
- Brief: Replace the current configuration.
- Parameters:
  - `config` (const Config &): Input parameter.
- Details: Set Config. If the background GC thread is running, it is stopped, the new config is applied, and the thread is restarted so the new bg_gc_interval takes effect immediately. config New configuration to apply. config Input parameter. Calls: isBackgroundGCRunning(), stopBackgroundGC(), lock(), startBackgroundGC().

#### `void startBackgroundGC()`
- Source: `include/storage/compaction_manager.h`:165
- Brief: Start the background GC thread.
- Parameters: none
- Details: Start Background GC. Does nothing if the thread is already running. Calls: lock(), joinable(), store(), std::thread(), backgroundLoop().

#### `Stats stats() const`
- Source: `include/storage/compaction_manager.h`:196
- Brief: n/a
- Parameters: none
- Details: Return a snapshot of current compaction statistics.

#### `void stopBackgroundGC()`
- Source: `include/storage/compaction_manager.h`:172
- Brief: Stop and join the background GC thread.
- Parameters: none
- Details: Stop Background GC. Blocks until the background thread has exited. Calls: lock(), store(), notify_all(), joinable(), utils::joinThreadWithin(), THEMIS_WARN().

#### `~CompactionManager() noexcept`
- Source: `include/storage/compaction_manager.h`:116
- Brief: Destructor — noexcept; stops background GC thread and swallows exceptions.
- Parameters: none

### themis::CompactionManager::Stats

#### `double writeAmplification() const noexcept`
- Source: `include/storage/compaction_manager.h`:89
- Brief: Estimated write-amplification factor.
- Parameters: none
- Details: Defined as (compact_bytes_written + flush_bytes_written) / user_bytes_written. Returns 0.0 if no user writes have occurred. A value close to 1.0 is ideal; values above 2.0 indicate excessive compaction overhead relative to the user workload.

### themis::ConflictManager

#### `ConflictManager(std::shared_ptr< RocksDBWrapper > db, std::shared_ptr< HybridLogicalClock > clock=nullptr)`
- Source: `include/storage/history_manager.h`:239
- Brief: Construct a ConflictManager.
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): Underlying RocksDB instance.
  - `clock` (std::shared_ptr< HybridLogicalClock >): HLC clock for generating unique conflict IDs.
- Details: db Underlying RocksDB instance. clock HLC clock for generating unique conflict IDs.

#### `std::string conflictKey(std::string_view conflict_id)`
- Source: `include/storage/history_manager.h`:251
- Brief: Build the storage key for a conflict record.
- Parameters:
  - `conflict_id` (std::string_view): Identifier of the conflict.
- Return: Return value.
- Details: ── Key encoding ────────────────────────────────────────────────────────────── Format: conflict:<conflict_id> conflict_id Identifier of the conflict. Return value. Calls: reserve(), size(), append(), data().

#### `std::string conflictSetKey(std::string_view conflict_set_id)`
- Source: `include/storage/history_manager.h`:258
- Brief: Build the storage key for a conflict set.
- Parameters:
  - `conflict_set_id` (std::string_view): Identifier of the conflict set.
- Return: Return value.
- Details: Conflict Set Key. Format: conflictset:<conflict_set_id> conflict_set_id Identifier of the conflict set. Return value. Calls: reserve(), size(), append(), data().

#### `std::optional< ConflictRecord > deserializeConflictRecord(std::string_view data)`
- Source: `include/storage/history_manager.h`:315
- Brief: Deserialize Conflict Record.
- Parameters:
  - `data` (std::string_view): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: verify_crc32(), nlohmann::json::parse(), begin(), end(), value(), HLCTimestamp(), hexToBytes(), THEMIS_WARN().

#### `std::optional< ConflictSet > deserializeConflictSet(std::string_view data)`
- Source: `include/storage/history_manager.h`:318
- Brief: Deserialize Conflict Set.
- Parameters:
  - `data` (std::string_view): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: verify_crc32(), nlohmann::json::parse(), begin(), end(), value(), HLCTimestamp(), THEMIS_WARN().

#### `std::optional< ConflictRecord > getConflict(std::string_view conflict_id) const`
- Source: `include/storage/history_manager.h`:289
- Brief: Retrieve a specific ConflictRecord by ID.
- Parameters:
  - `conflict_id` (std::string_view): n/a
- Return: The record, or std::nullopt if not found.
- Details: The record, or std::nullopt if not found.

#### `std::optional< ConflictSet > getConflictSet(std::string_view conflict_set_id) const`
- Source: `include/storage/history_manager.h`:296
- Brief: Retrieve a specific ConflictSet by ID.
- Parameters:
  - `conflict_set_id` (std::string_view): n/a
- Return: The set, or std::nullopt if not found.
- Details: The set, or std::nullopt if not found.

#### `std::vector< ConflictSet > listConflictSets() const`
- Source: `include/storage/history_manager.h`:310
- Brief: List all stored conflict sets, most-recent first.
- Parameters: none
- Details: Note: this is an O(N) scan over all conflictset entries.

#### `std::vector< ConflictRecord > listConflicts() const`
- Source: `include/storage/history_manager.h`:303
- Brief: List all stored conflict records, most-recent first.
- Parameters: none
- Details: Note: this is an O(N) scan over all conflict entries.

#### `std::vector< uint8_t > serializeConflictRecord(const ConflictRecord &rec)`
- Source: `include/storage/history_manager.h`:314
- Brief: ── Serialization ─────────────────────────────────────────────────────────────
- Parameters:
  - `rec` (const ConflictRecord &): Input parameter.
- Return: Return value.
- Details: rec Input parameter. Return value. Calls: bytesToHex(), dump(), buf(), begin(), end(), append_crc32().

#### `std::vector< uint8_t > serializeConflictSet(const ConflictSet &set)`
- Source: `include/storage/history_manager.h`:317
- Brief: ── ConflictSet serialization ─────────────────────────────────────────────────
- Parameters:
  - `set` (const ConflictSet &): Input parameter.
- Return: Return value.
- Details: set Input parameter. Return value. Calls: dump(), buf(), begin(), end(), append_crc32().

#### `std::string storeConflict(ConflictRecord &record)`
- Source: `include/storage/history_manager.h`:270
- Brief: Persist a ConflictRecord using a fresh non-transactional write.
- Parameters:
  - `record` (ConflictRecord &): Input/output parameter.
- Return: The assigned conflict_id.
- Details: ── Write ───────────────────────────────────────────────────────────────────── Assigns a unique conflict_id to record if it is empty, then writes it to the conflict keyspace. The assigned conflict_id. record Input/output parameter. Return value. Calls: empty(), now(), std::to_string(), physical(), logical(), conflictKey(), serializeConflictRecord(), put().

#### `std::string storeConflictSet(ConflictSet &set)`
- Source: `include/storage/history_manager.h`:280
- Brief: Persist a ConflictSet using a fresh non-transactional write.
- Parameters:
  - `set` (ConflictSet &): Input/output parameter.
- Return: The assigned conflict_set_id.
- Details: ── ConflictSet write/read ──────────────────────────────────────────────────── Assigns a unique conflict_set_id to set if it is empty, then writes it to the conflictset keyspace. The assigned conflict_set_id. set Input/output parameter. Return value. Calls: empty(), now(), std::to_string(), physical(), logical(), conflictSetKey(), serializeConflictSet(), put().

### themis::CounterMergeOperator

#### `bool Merge(const rocksdb::Slice &key, const rocksdb::Slice *existing_value, const rocksdb::Slice &value, std::string *new_value, rocksdb::Logger *logger) const override`
- Source: `include/storage/merge_operators.h`:26
- Brief: n/a
- Parameters:
  - `key` (const rocksdb::Slice &): n/a
  - `existing_value` (const rocksdb::Slice *): n/a
  - `value` (const rocksdb::Slice &): n/a
  - `new_value` (std::string *): n/a
  - `logger` (rocksdb::Logger *): n/a

#### `const char * Name() const override`
- Source: `include/storage/merge_operators.h`:32
- Brief: n/a
- Parameters: none

### themis::HLCTimestamp

#### `HLCTimestamp()=default`
- Source: `include/storage/hlc.h`:51
- Brief: n/a
- Parameters: none

#### `HLCTimestamp(uint64_t physical_ms, uint32_t logical_cnt)`
- Source: `include/storage/hlc.h`:60
- Brief: n/a
- Parameters:
  - `physical_ms` (uint64_t): n/a
  - `logical_cnt` (uint32_t): n/a
- Details: Backwards-compatible constructor used by older tests: HLCTimestamp(physical, logical)

#### `HLCTimestamp(uint64_t v)`
- Source: `include/storage/hlc.h`:52
- Brief: n/a
- Parameters:
  - `v` (uint64_t): n/a

#### `HLCTimestamp decodeFromBytes(const uint8_t in[8])`
- Source: `include/storage/hlc.h`:109
- Brief: n/a
- Parameters:
  - `in` (const uint8_t): n/a
- Details: Decode 8 big-endian bytes back into an HLCTimestamp.

#### `HLCTimestamp decodeFromString(const std::string &s)`
- Source: `include/storage/hlc.h`:134
- Brief: n/a
- Parameters:
  - `s` (const std::string &): n/a
- Details: Decode from an 8-byte string produced by encodeToString().

#### `void encodeToBytes(uint8_t out[8]) const`
- Source: `include/storage/hlc.h`:100
- Brief: Encode timestamp as 8 big-endian bytes for use in RocksDB keys.
- Parameters:
  - `out` (uint8_t): n/a
- Details: Big-endian encoding ensures lexicographic key order matches chronological order, which is required for correct MVCC range scans.

#### `std::string encodeToString() const`
- Source: `include/storage/hlc.h`:123
- Brief: Encode to an 8-byte string (for key construction).
- Parameters: none
- Details: Produces a fixed-width, big-endian byte string that compares correctly with memcmp / RocksDB's default bytewise comparator.

#### `HLCTimestamp from(uint64_t physical_ms, uint32_t logical_counter)`
- Source: `include/storage/hlc.h`:79
- Brief: n/a
- Parameters:
  - `physical_ms` (uint64_t): n/a
  - `logical_counter` (uint32_t): n/a
- Details: Build an HLCTimestamp from physical and logical components

#### `uint32_t getLogicalCounter() const`
- Source: `include/storage/hlc.h`:76
- Brief: n/a
- Parameters: none

#### `uint64_t getPhysicalTime() const`
- Source: `include/storage/hlc.h`:75
- Brief: n/a
- Parameters: none
- Details: Backwards-compat wrapper names expected by tests

#### `uint32_t logical() const`
- Source: `include/storage/hlc.h`:72
- Brief: n/a
- Parameters: none
- Details: Logical component (tie-breaker within the same millisecond)

#### `bool operator!=(const HLCTimestamp &o) const`
- Source: `include/storage/hlc.h`:92
- Brief: n/a
- Parameters:
  - `o` (const HLCTimestamp &): n/a

#### `bool operator<(const HLCTimestamp &o) const`
- Source: `include/storage/hlc.h`:87
- Brief: n/a
- Parameters:
  - `o` (const HLCTimestamp &): n/a

#### `bool operator<=(const HLCTimestamp &o) const`
- Source: `include/storage/hlc.h`:88
- Brief: n/a
- Parameters:
  - `o` (const HLCTimestamp &): n/a

#### `bool operator==(const HLCTimestamp &o) const`
- Source: `include/storage/hlc.h`:91
- Brief: n/a
- Parameters:
  - `o` (const HLCTimestamp &): n/a

#### `bool operator>(const HLCTimestamp &o) const`
- Source: `include/storage/hlc.h`:89
- Brief: n/a
- Parameters:
  - `o` (const HLCTimestamp &): n/a

#### `bool operator>=(const HLCTimestamp &o) const`
- Source: `include/storage/hlc.h`:90
- Brief: n/a
- Parameters:
  - `o` (const HLCTimestamp &): n/a

#### `uint64_t physical() const`
- Source: `include/storage/hlc.h`:69
- Brief: n/a
- Parameters: none
- Details: Physical component (wall-clock milliseconds since Unix epoch)

#### `std::string toString() const`
- Source: `include/storage/hlc.h`:144
- Brief: n/a
- Parameters: none
- Details: Human-readable representation: "<physical_ms>.<logical>"

### themis::HistoryManager

#### `HistoryManager(std::shared_ptr< RocksDBWrapper > db, std::shared_ptr< HybridLogicalClock > clock=nullptr)`
- Source: `include/storage/history_manager.h`:131
- Brief: Construct a HistoryManager.
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): Underlying RocksDB instance (shared ownership).
  - `clock` (std::shared_ptr< HybridLogicalClock >): HLC clock used to generate timestamps (shared ownership). If null, a new HybridLogicalClock is created internally.
- Details: db Underlying RocksDB instance (shared ownership). clock HLC clock used to generate timestamps (shared ownership). If null, a new HybridLogicalClock is created internally.

#### `std::optional< HistoryRecord > deserializeHistoryRecord(std::string_view data)`
- Source: `include/storage/history_manager.h`:209
- Brief: Deserialize History Record.
- Parameters:
  - `data` (std::string_view): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: verify_crc32(), nlohmann::json::parse(), begin(), end(), value(), HLCTimestamp(), hexToBytes(), THEMIS_WARN().

#### `std::optional< HistoryRecord > getAtTimestamp(std::string_view base_key, HLCTimestamp ts) const`
- Source: `include/storage/history_manager.h`:196
- Brief: Return the most-recent history record for base_key at or before ts.
- Parameters:
  - `base_key` (std::string_view): n/a
  - `ts` (HLCTimestamp): n/a
- Return: HistoryRecord, or std::nullopt if no version exists at or before ts.
- Details: HistoryRecord, or std::nullopt if no version exists at or before ts.

#### `std::string historyKey(std::string_view base_key, HLCTimestamp ts)`
- Source: `include/storage/history_manager.h`:143
- Brief: Build the history key for base_key at ts.
- Parameters:
  - `base_key` (std::string_view): Input parameter.
  - `ts` (HLCTimestamp): Input parameter.
- Return: Return value.
- Details: ── Key encoding ───────────────────────────────────────────────────────────── Format: hist:<base_key>\x00<8-byte-big-endian-ts> base_key Input parameter. ts Input parameter. Return value. Calls: reserve(), size(), append(), data(), push_back(), encodeToString().

#### `std::string historyPrefix(std::string_view base_key)`
- Source: `include/storage/history_manager.h`:150
- Brief: Build the scan prefix for all history versions of base_key.
- Parameters:
  - `base_key` (std::string_view): Input parameter.
- Return: Return value.
- Details: History Prefix. Format: hist:<base_key>\x00 base_key Input parameter. Return value. Calls: reserve(), size(), append(), data(), push_back().

#### `std::vector< HistoryRecord > listVersions(std::string_view base_key) const`
- Source: `include/storage/history_manager.h`:204
- Brief: Return all history versions of base_key, oldest first.
- Parameters:
  - `base_key` (std::string_view): n/a

#### `std::optional< HLCTimestamp > recordDel(RocksDBWrapper::TransactionWrapper &txn, std::string_view base_key, uint64_t txn_id=0)`
- Source: `include/storage/history_manager.h`:183
- Brief: Write a tombstone "del" history entry within an existing transaction.
- Parameters:
  - `txn` (RocksDBWrapper::TransactionWrapper &): Input/output parameter.
  - `base_key` (std::string_view): Input parameter.
  - `txn_id` (uint64_t): Identifier of the txn.
- Return: The HLC timestamp assigned to this history entry, or std::nullopt if the history write failed (caller should treat the operation as failed).
- Details: Record Del. txn Active RocksDB transaction. base_key The live key that was deleted. txn_id Originating transaction ID (0 if unknown). The HLC timestamp assigned to this history entry, or std::nullopt if the history write failed (caller should treat the operation as failed). txn Input/output parameter. base_key Input parameter. txn_id Identifier of the txn. Return value. Calls: now(), std::string(), historyKey(), serializeHistoryRecord(), put().

#### `std::optional< HLCTimestamp > recordPut(RocksDBWrapper::TransactionWrapper &txn, std::string_view base_key, const std::vector< uint8_t > &value, uint64_t txn_id=0)`
- Source: `include/storage/history_manager.h`:167
- Brief: Write a "put" history entry within an existing transaction.
- Parameters:
  - `txn` (RocksDBWrapper::TransactionWrapper &): Input/output parameter.
  - `base_key` (std::string_view): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
  - `txn_id` (uint64_t): Identifier of the txn.
- Return: The HLC timestamp assigned to this history entry, or std::nullopt if the history write failed (caller should treat the operation as failed).
- Details: ── Transactional write helpers ─────────────────────────────────────────────── Both the live-key write and this history write share the same txn so they are committed atomically. txn Active RocksDB transaction. base_key The live key that was written. value The value that was written. txn_id Originating transaction ID (0 if unknown). The HLC timestamp assigned to this history entry, or std::nullopt if the history write failed (caller should treat the operation as failed). txn Input/output parameter. base_key Input parameter. value Input parameter. txn_id Identifier of the txn. Return value. Calls: now(), std::string(), historyKey(), serializeHistoryRecord(), put().

#### `std::vector< uint8_t > serializeHistoryRecord(const HistoryRecord &rec)`
- Source: `include/storage/history_manager.h`:208
- Brief: ── Serialization ─────────────────────────────────────────────────────────────
- Parameters:
  - `rec` (const HistoryRecord &): Input parameter.
- Return: Return value.
- Details: rec Input parameter. Return value. Calls: bytesToHex(), dump(), buf(), begin(), end(), append_crc32().

### themis::HybridLogicalClock

#### `HybridLogicalClock()`
- Source: `include/storage/hlc.h`:161
- Brief: n/a
- Parameters: none

#### `HLCTimestamp advanceTo(uint64_t phys_ms)`
- Source: `include/storage/hlc.h`:194
- Brief: advanceTo is a helper for the CAS loop in now() – not called externally.
- Parameters:
  - `phys_ms` (uint64_t): Input parameter.
- Return: Return value.
- Details: phys_ms Input parameter. Return value. Computes the next HLCTimestamp value given current packed state and wall clock. Calls: load(), compare_exchange_weak(), HLCTimestamp().

#### `HLCTimestamp now()`
- Source: `include/storage/hlc.h`:168
- Brief: Generate a new timestamp for a local event.
- Parameters: none
- Return: Return value.
- Details: Now. Guarantees: returned value > every previous value returned by this instance. Return value. Calls: advanceTo(), wallClockMs().

#### `HLCTimestamp peek() const`
- Source: `include/storage/hlc.h`:183
- Brief: Read the current timestamp without advancing it.
- Parameters: none
- Details: The returned value reflects the last call to now() or update().

#### `HLCTimestamp update(HLCTimestamp received)`
- Source: `include/storage/hlc.h`:176
- Brief: Advance the clock after receiving a message with timestamp received.
- Parameters:
  - `received` (HLCTimestamp): Input parameter.
- Return: Return value.
- Details: Update. Updates the clock to be strictly greater than both the local clock and the received timestamp, then returns the new local timestamp. received Input parameter. Return value. Calls: load(), wallClockMs(), std::max(), physical(), logical(), compare_exchange_weak(), HLCTimestamp().

#### `uint64_t wallClockMs()`
- Source: `include/storage/hlc.h`:193
- Brief: Wall Clock Ms.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count().

### themis::IIndexAnalysisAdvisor

#### `std::optional< std::pair< IndexRecommendation, std::string > > advise(const IndexAnalysisReport &report)=0`
- Source: `include/storage/index_analyzer.h`:213
- Brief: Inspect the preliminary analysis report and optionally override.
- Parameters:
  - `report` (const IndexAnalysisReport &): Read-only preliminary report produced by rule-based logic.
- Return: Override recommendation + reason, or nullopt to keep rule-based result.
- Details: report Read-only preliminary report produced by rule-based logic. Override recommendation + reason, or nullopt to keep rule-based result.

#### `~IIndexAnalysisAdvisor()=default`
- Source: `include/storage/index_analyzer.h`:204
- Brief: n/a
- Parameters: none

### themis::IndexAnalyzeConfig

#### `Result< IndexAnalyzeConfig > fromYamlFile(const std::string &yaml_path)`
- Source: `include/storage/index_analyzer.h`:152
- Brief: Load configuration from a YAML file.
- Parameters:
  - `yaml_path` (const std::string &): Path to the yaml.
- Return: Loaded config or an error string.
- Details: From Yaml File. The file is expected to contain an "index_analyze" root key. Missing keys are filled with defaults; unknown keys are silently ignored. yaml_path Absolute or relative path to the YAML file. Loaded config or an error string. yaml_path Path to the yaml. Return value. Calls: YAML::LoadFile(), IsMap(), THEMIS_WARN(), loadTierThresholds(), TierThresholds::hot(), TierThresholds::warm(), TierThresholds::cold(), IsSequence().

#### `const TierThresholds & thresholdsFor(storage::StorageTierLevel tier) const noexcept`
- Source: `include/storage/index_analyzer.h`:157
- Brief: Return the threshold set that applies to the given tier.
- Parameters:
  - `tier` (storage::StorageTierLevel): n/a

### themis::IndexAnalyzer

#### `IndexAnalyzer(IndexAnalyzer &&)=delete`
- Source: `include/storage/index_analyzer.h`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexAnalyzer &&): n/a

#### `IndexAnalyzer(const IndexAnalyzer &)=delete`
- Source: `include/storage/index_analyzer.h`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IndexAnalyzer &): n/a

#### `IndexAnalyzer(std::shared_ptr< RocksDBWrapper > db_wrapper, IndexAnalyzeConfig config=IndexAnalyzeConfig{})`
- Source: `include/storage/index_analyzer.h`:253
- Brief: Construct with an existing RocksDB wrapper and loaded config.
- Parameters:
  - `db_wrapper` (std::shared_ptr< RocksDBWrapper >): Non-null shared pointer to the active RocksDB instance.
  - `config` (IndexAnalyzeConfig): Fully populated IndexAnalyzeConfig (from YAML or default).
- Details: db_wrapper Non-null shared pointer to the active RocksDB instance. config Fully populated IndexAnalyzeConfig (from YAML or default).

#### `Result< IndexAnalysisReport > analyze(const std::string &index_name, storage::StorageTierLevel tier, std::optional< TierThresholds > overrides=std::nullopt)`
- Source: `include/storage/index_analyzer.h`:295
- Brief: Analyse a single index and return the report.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
  - `tier` (storage::StorageTierLevel): Input parameter.
  - `overrides` (std::optional< TierThresholds >): Input parameter.
- Return: Return value.
- Details: Analyze. index_name Name of the index to analyse. tier Storage tier the index currently resides on. overrides Optional per-call threshold overrides (nullopt = use config). index_name Name of the index. tier Input parameter. overrides Input parameter. Return value. Calls: empty(), lock(), value_or(), thresholdsFor(), computeReport(), applyAdvisor(), std::move(), std::string().

#### `std::vector< IndexAnalysisReport > analyzeAll()`
- Source: `include/storage/index_analyzer.h`:306
- Brief: Analyse all indices registered in the config.
- Parameters: none
- Return: Vector of reports (one per enabled index).
- Details: Analyze All. Skips indices with enabled = false. Vector of reports (one per enabled index). Return value. Calls: lock(), reserve(), size(), value_or(), thresholdsFor(), computeReport(), applyAdvisor(), push_back().

#### `void applyAdvisor(IndexAnalysisReport &report)`
- Source: `include/storage/index_analyzer.h`:358
- Brief: ───────────────────────────────────────────────────────────────────────────── Private – AI/ML advisor dispatch ─────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `report` (IndexAnalysisReport &): Input/output parameter.
- Details: report Input/output parameter. Calls: lock(), advise(), THEMIS_INFO(), value(), THEMIS_WARN(), what().

#### `IndexRecommendation classify(double frag_pct, bool stats_stale, const TierThresholds &thresholds)`
- Source: `include/storage/index_analyzer.h`:361
- Brief: Classify the semantic intent of a query.
- Parameters:
  - `frag_pct` (double): Input parameter.
  - `stats_stale` (bool): Input parameter.
  - `thresholds` (const TierThresholds &): n/a
- Return: Return value.
- Details: frag_pct Input parameter. stats_stale Input parameter. t Input parameter. Return value. Implements classify without additional internal calls.

#### `IndexAnalysisReport computeReport(const std::string &index_name, storage::StorageTierLevel tier, const TierThresholds &thresholds)`
- Source: `include/storage/index_analyzer.h`:352
- Brief: Compute Report.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
  - `tier` (storage::StorageTierLevel): Input parameter.
  - `thresholds` (const TierThresholds &): Input parameter.
- Return: Return value.
- Details: index_name Name of the index. tier Input parameter. thresholds Input parameter. Return value. Calls: std::chrono::system_clock::now(), getRawDB(), THEMIS_WARN(), GetProperty(), std::stoull(), GetIntProperty(), std::min(), get().

#### `const IndexAnalyzeConfig & config() const`
- Source: `include/storage/index_analyzer.h`:275
- Brief: n/a
- Parameters: none

#### `bool isScheduled() const noexcept`
- Source: `include/storage/index_analyzer.h`:329
- Brief: n/a
- Parameters: none
- Return: true if the background scheduler is currently running.
- Details: true if the background scheduler is currently running.

#### `std::vector< IndexAnalysisReport > lastReports() const`
- Source: `include/storage/index_analyzer.h`:338
- Brief: Return the reports from the most recent analyzeAll() run.
- Parameters: none
- Details: Returns an empty vector if no analysis has been performed yet.

#### `std::optional< std::chrono::system_clock::time_point > lastRunTime() const`
- Source: `include/storage/index_analyzer.h`:345
- Brief: Return the timestamp of the last scheduled analysis run.
- Parameters: none
- Details: Returns nullopt if no scheduled run has completed yet.

#### `IndexAnalyzer & operator=(IndexAnalyzer &&)=delete`
- Source: `include/storage/index_analyzer.h`:263
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexAnalyzer &&): n/a

#### `IndexAnalyzer & operator=(const IndexAnalyzer &)=delete`
- Source: `include/storage/index_analyzer.h`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IndexAnalyzer &): n/a

#### `void schedulerLoop()`
- Source: `include/storage/index_analyzer.h`:349
- Brief: Scheduler Loop.
- Parameters: none
- Details: Calls: THEMIS_INFO(), load(), lock(), empty(), wait_for(), std::chrono::minutes(), CronExpression::parse(), THEMIS_ERROR().

#### `void setAdvisor(std::shared_ptr< IIndexAnalysisAdvisor > advisor)`
- Source: `include/storage/index_analyzer.h`:284
- Brief: Register an AI/ML advisor for post-analysis recommendation override.
- Parameters:
  - `advisor` (std::shared_ptr< IIndexAnalysisAdvisor >): Input parameter.
- Details: Set Advisor. Pass nullptr to remove the current advisor (reverts to rule-based only). advisor Input parameter. Calls: lock(), std::move(), THEMIS_INFO().

#### `void setConfig(IndexAnalyzeConfig config)`
- Source: `include/storage/index_analyzer.h`:273
- Brief: Replace the current configuration at runtime.
- Parameters:
  - `config` (IndexAnalyzeConfig): Input parameter.
- Details: Set Config. If the background thread is running, the new cron expression and index list take effect at the next scheduler wake-up. config Input parameter. Calls: lock(), std::move(), notify_all(), THEMIS_INFO(), size().

#### `Result< void > startScheduled()`
- Source: `include/storage/index_analyzer.h`:319
- Brief: Start the background cron scheduler thread.
- Parameters: none
- Return: Error if cron_expression is invalid or already started.
- Details: Start Scheduled. The scheduler waits until the next time matching the configured cron_expression, then calls analyzeAll() and stores the results. Idempotent – calling start() twice has no effect. Error if cron_expression is invalid or already started. Return value. Calls: exchange(), ErrVoid(), lock(), empty(), CronExpression::parse(), std::thread(), THEMIS_INFO(), OkVoid().

#### `void stopScheduled()`
- Source: `include/storage/index_analyzer.h`:326
- Brief: Stop the background cron scheduler thread.
- Parameters: none
- Details: Stop Scheduled. Blocks until the thread has exited. Idempotent. Calls: exchange(), notify_all(), joinable(), utils::joinThreadWithin(), THEMIS_WARN(), THEMIS_INFO().

#### `~IndexAnalyzer()`
- Source: `include/storage/index_analyzer.h`:257
- Brief: n/a
- Parameters: none

### themis::IndexMaintenanceManager

#### `IndexMaintenanceManager(IndexMaintenanceManager &&)=delete`
- Source: `include/storage/index_maintenance.h`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexMaintenanceManager &&): n/a

#### `IndexMaintenanceManager(const IndexMaintenanceManager &)=delete`
- Source: `include/storage/index_maintenance.h`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IndexMaintenanceManager &): n/a

#### `IndexMaintenanceManager(std::shared_ptr< RocksDBWrapper > db_wrapper, std::shared_ptr< IndexManager > index_manager=nullptr)`
- Source: `include/storage/index_maintenance.h`:156
- Brief: Constructor.
- Parameters:
  - `db_wrapper` (std::shared_ptr< RocksDBWrapper >): RocksDB wrapper for storage operations
  - `index_manager` (std::shared_ptr< IndexManager >): Optional index manager for index-specific operations
- Details: db_wrapper RocksDB wrapper for storage operations index_manager Optional index manager for index-specific operations

#### `Result< FragmentationMetrics > calculateFragmentation(const std::string &index_name)`
- Source: `include/storage/index_maintenance.h`:310
- Brief: Calculate Fragmentation.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
- Return: Return value.
- Details: index_name Name of the index. Return value. Calls: getRawDB(), GetProperty(), std::stoull(), GetIntProperty(), std::min(), classifyFragmentation(), std::chrono::system_clock::now(), time_since_epoch().

#### `Result< void > cancelJob(const std::string &job_id)`
- Source: `include/storage/index_maintenance.h`:267
- Brief: Cancel a running maintenance job.
- Parameters:
  - `job_id` (const std::string &): Identifier of the job.
- Return: Result indicating success or error
- Details: Cancel Job. job_id Job identifier Result indicating success or error job_id Identifier of the job. Return value. Calls: lock(), find(), end(), ErrVoid(), THEMIS_INFO(), OkVoid().

#### `Result< MaintenanceJobStatus > checkConsistency(const std::string &index_name, bool repair=false)`
- Source: `include/storage/index_maintenance.h`:247
- Brief: Perform consistency check.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
  - `repair` (bool): Input parameter.
- Return: Result with job status or error
- Details: Check Consistency. index_name Name of the index to check repair Attempt to repair issues if found Result with job status or error index_name Name of the index. repair Input parameter. Return value. Calls: generateJobId(), std::chrono::system_clock::now(), time_since_epoch(), count(), performConsistencyCheck(), error(), message(), tl::unexpected().

#### `FragmentationLevel classifyFragmentation(double percentage) const`
- Source: `include/storage/index_maintenance.h`:321
- Brief: n/a
- Parameters:
  - `percentage` (double): n/a

#### `Result< MaintenanceJobStatus > cleanupOrphanEntries(const std::string &index_name)`
- Source: `include/storage/index_maintenance.h`:239
- Brief: Cleanup orphan entries.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
- Return: Result with job status or error
- Details: Cleanup Orphan Entries. index_name Name of the index to cleanup Result with job status or error index_name Name of the index. Return value. Calls: generateJobId(), std::chrono::system_clock::now(), time_since_epoch(), count(), performOrphanCleanup(), error(), message(), tl::unexpected().

#### `std::string generateJobId()`
- Source: `include/storage/index_maintenance.h`:319
- Brief: Generate Job Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), str().

#### `std::map< std::string, FragmentationMetrics > getAllFragmentationMetrics()`
- Source: `include/storage/index_maintenance.h`:273
- Brief: Get fragmentation metrics for all indices.
- Parameters: none
- Return: Map of index names to fragmentation metrics
- Details: Map of index names to fragmentation metrics

#### `Result< MaintenanceJobStatus > getJobStatus(const std::string &job_id)`
- Source: `include/storage/index_maintenance.h`:254
- Brief: Get status of a maintenance job.
- Parameters:
  - `job_id` (const std::string &): Identifier of the job.
- Return: Result with job status or error
- Details: Get Job Status. job_id Job identifier Result with job status or error job_id Identifier of the job. Return value. Calls: lock(), find(), end().

#### `MaintenancePolicy getPolicy() const`
- Source: `include/storage/index_maintenance.h`:202
- Brief: Get current maintenance policy.
- Parameters: none
- Return: Current policy configuration
- Details: Current policy configuration

#### `bool isInMaintenanceWindow() const`
- Source: `include/storage/index_maintenance.h`:318
- Brief: n/a
- Parameters: none

#### `bool isRunning() const`
- Source: `include/storage/index_maintenance.h`:189
- Brief: Check if maintenance is running.
- Parameters: none
- Return: true if background thread is active
- Details: true if background thread is active

#### `std::vector< MaintenanceJobStatus > listActiveJobs() const`
- Source: `include/storage/index_maintenance.h`:260
- Brief: List all active maintenance jobs.
- Parameters: none
- Return: Vector of active job statuses
- Details: Vector of active job statuses

#### `void maintenanceThreadFunc()`
- Source: `include/storage/index_maintenance.h`:307
- Brief: Maintenance Thread Func.
- Parameters: none
- Details: Calls: THEMIS_INFO(), lock(), wait_for(), std::chrono::milliseconds(), shouldRunMaintenance(), unlock(), getAllFragmentationMetrics(), rebuildIndex().

#### `Result< FragmentationMetrics > monitorFragmentation(const std::string &index_name)`
- Source: `include/storage/index_maintenance.h`:209
- Brief: Monitor fragmentation for a specific index.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
- Return: Result with fragmentation metrics or error
- Details: Monitor Fragmentation. index_name Name of the index to monitor Result with fragmentation metrics or error index_name Name of the index. Return value. Calls: calculateFragmentation().

#### `IndexMaintenanceManager & operator=(IndexMaintenanceManager &&)=delete`
- Source: `include/storage/index_maintenance.h`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (IndexMaintenanceManager &&): n/a

#### `IndexMaintenanceManager & operator=(const IndexMaintenanceManager &)=delete`
- Source: `include/storage/index_maintenance.h`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (const IndexMaintenanceManager &): n/a

#### `Result< void > performConsistencyCheck(const std::string &index_name, bool repair, MaintenanceJobStatus &status)`
- Source: `include/storage/index_maintenance.h`:315
- Brief: Perform Consistency Check.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
  - `repair` (bool): Input parameter.
  - `status` (MaintenanceJobStatus &): Input/output parameter.
- Return: Return value.
- Details: index_name Name of the index. repair Input parameter. status Input/output parameter. Return value. Calls: THEMIS_INFO(), getRawDB(), ErrVoid(), VerifyChecksum(), ok(), THEMIS_WARN(), ToString(), OkVoid().

#### `Result< void > performOrphanCleanup(const std::string &index_name, MaintenanceJobStatus &status)`
- Source: `include/storage/index_maintenance.h`:314
- Brief: Perform Orphan Cleanup.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
  - `status` (MaintenanceJobStatus &): Input/output parameter.
- Return: Return value.
- Details: index_name Name of the index. status Input/output parameter. Return value. Calls: THEMIS_INFO(), getRawDB(), ErrVoid(), CompactRange(), ok(), THEMIS_WARN(), ToString(), OkVoid().

#### `Result< void > performRebuild(const std::string &index_name, MaintenanceJobStatus &status)`
- Source: `include/storage/index_maintenance.h`:311
- Brief: Perform Rebuild.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
  - `status` (MaintenanceJobStatus &): Input/output parameter.
- Return: Return value.
- Details: index_name Name of the index. status Input/output parameter. Return value. Calls: THEMIS_INFO(), getRawDB(), ErrVoid(), CompactRange(), ok(), ToString(), calculateFragmentation(), OkVoid().

#### `Result< void > performReorganize(const std::string &index_name, MaintenanceJobStatus &status)`
- Source: `include/storage/index_maintenance.h`:312
- Brief: Perform Reorganize.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
  - `status` (MaintenanceJobStatus &): Input/output parameter.
- Return: Return value.
- Details: index_name Name of the index. status Input/output parameter. Return value. Calls: THEMIS_INFO(), getRawDB(), ErrVoid(), CompactRange(), ok(), ToString(), calculateFragmentation(), OkVoid().

#### `Result< void > performStatisticsUpdate(const std::string &index_name, MaintenanceJobStatus &status)`
- Source: `include/storage/index_maintenance.h`:313
- Brief: Perform Statistics Update.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
  - `status` (MaintenanceJobStatus &): Input/output parameter.
- Return: Return value.
- Details: index_name Name of the index. status Input/output parameter. Return value. Calls: THEMIS_INFO(), std::this_thread::sleep_for(), std::chrono::milliseconds(), OkVoid(), ErrVoid(), std::string(), what().

#### `Result< MaintenanceJobStatus > rebuildIndex(const std::string &index_name, bool async=true)`
- Source: `include/storage/index_maintenance.h`:217
- Brief: Rebuild an index (full reconstruction).
- Parameters:
  - `index_name` (const std::string &): Name of the index.
  - `async` (bool): Input parameter.
- Return: Result with job status or error
- Details: Rebuild Index. index_name Name of the index to rebuild async Run asynchronously in background Result with job status or error index_name Name of the index. async Input parameter. Return value. Calls: generateJobId(), std::chrono::system_clock::now(), time_since_epoch(), count(), calculateFragmentation(), lock(), std::thread(), performRebuild().

#### `Result< MaintenanceJobStatus > reorganizeIndex(const std::string &index_name, bool async=true)`
- Source: `include/storage/index_maintenance.h`:225
- Brief: Reorganize an index (in-place defragmentation).
- Parameters:
  - `index_name` (const std::string &): Name of the index.
  - `async` (bool): Input parameter.
- Return: Result with job status or error
- Details: Reorganize Index. index_name Name of the index to reorganize async Run asynchronously in background Result with job status or error index_name Name of the index. async Input parameter. Return value. Calls: generateJobId(), std::chrono::system_clock::now(), time_since_epoch(), count(), calculateFragmentation(), lock(), std::thread(), performReorganize().

#### `Result< void > setPolicy(const MaintenancePolicy &policy)`
- Source: `include/storage/index_maintenance.h`:196
- Brief: Set maintenance policy.
- Parameters:
  - `policy` (const MaintenancePolicy &): Input parameter.
- Return: Result indicating success or error
- Details: Set Policy. policy New maintenance policy configuration Result indicating success or error policy Input parameter. Return value. Calls: lock(), THEMIS_INFO(), OkVoid().

#### `void setVectorIndexManager(std::shared_ptr< VectorIndexManager > vector_index)`
- Source: `include/storage/index_maintenance.h`:288
- Brief: Set the VectorIndexManager for HNSW maintenance operations.
- Parameters:
  - `vector_index` (std::shared_ptr< VectorIndexManager >): Input parameter.
- Details: Set Vector Index Manager. vector_index Shared pointer to the vector index manager vector_index Input parameter. Calls: lock(), std::move(), THEMIS_INFO().

#### `bool shouldRunMaintenance() const`
- Source: `include/storage/index_maintenance.h`:317
- Brief: n/a
- Parameters: none

#### `Result< void > start()`
- Source: `include/storage/index_maintenance.h`:177
- Brief: Start background maintenance thread.
- Parameters: none
- Return: Result indicating success or error
- Details: Start. Result indicating success or error Return value. Calls: lock(), ErrVoid(), std::thread(), THEMIS_INFO(), OkVoid().

#### `Result< void > stop()`
- Source: `include/storage/index_maintenance.h`:183
- Brief: Stop background maintenance thread.
- Parameters: none
- Return: Result indicating success or error
- Details: Stop. Result indicating success or error Return value. Calls: lock(), OkVoid(), notify_all(), joinable(), utils::joinThreadWithin(), THEMIS_WARN(), THEMIS_INFO().

#### `Result< void > triggerMaintenanceCheck()`
- Source: `include/storage/index_maintenance.h`:280
- Brief: Trigger immediate maintenance check Checks all indices and performs maintenance if needed.
- Parameters: none
- Return: Result indicating success or error
- Details: Trigger Maintenance Check. Result indicating success or error Return value. Calls: ErrVoid(), notify_one(), OkVoid().

#### `Result< MaintenanceJobStatus > updateStatistics(const std::string &index_name)`
- Source: `include/storage/index_maintenance.h`:232
- Brief: Update index statistics.
- Parameters:
  - `index_name` (const std::string &): Name of the index.
- Return: Result with job status or error
- Details: Update Statistics. index_name Name of the index to update statistics Result with job status or error index_name Name of the index. Return value. Calls: generateJobId(), std::chrono::system_clock::now(), time_since_epoch(), count(), performStatisticsUpdate(), error(), message(), tl::unexpected().

#### `Result< MaintenanceJobStatus > vectorIncrementalReindex(float rebuild_threshold=0.20f, std::string_view vector_field="embedding")`
- Source: `include/storage/index_maintenance.h`:301
- Brief: Run incremental HNSW re-index (sync in-memory index with storage).
- Parameters:
  - `rebuild_threshold` (float): Input parameter.
  - `vector_field` (std::string_view): Input parameter.
- Return: Result with job status or error
- Details: Vector Incremental Reindex. Delegates to VectorIndexManager::incrementalReindex(). If no VectorIndexManager has been set via setVectorIndexManager(), returns an error without performing any operation. rebuild_threshold Deleted-label ratio that triggers a full rebuild (0–1). vector_field Name of the vector field in stored entities. Result with job status or error rebuild_threshold Input parameter. vector_field Input parameter. Return value. Calls: lock(), generateJobId(), getObjectName(), std::chrono::system_clock::now(), time_since_epoch(), count(), THEMIS_INFO(), incrementalReindex().

#### `~IndexMaintenanceManager() noexcept`
- Source: `include/storage/index_maintenance.h`:165
- Brief: Destructor - stops background maintenance thread.
- Parameters: none
- Details: Destructor — noexcept; stops maintenance thread and swallows exceptions.

### themis::KeySchema

#### `std::string extractPrimaryKey(std::string_view key)`
- Source: `include/storage/key_schema.h`:97
- Brief: Extract primary key from any key type.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: Extract Primary Key. key Input parameter. Return value. Calls: rfind(), std::string(), substr().

#### `std::string makeDocumentKey(std::string_view collection, std::string_view pk)`
- Source: `include/storage/key_schema.h`:62
- Brief: Construct key for document.
- Parameters:
  - `collection` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: Make Document Key. collection Input parameter. pk Input parameter. Return value. Calls: reserve(), size().

#### `std::string makeGraphEdgeKey(std::string_view pk)`
- Source: `include/storage/key_schema.h`:68
- Brief: Construct key for graph edge.
- Parameters:
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: Make Graph Edge Key. pk Input parameter. Return value. Calls: reserve(), size().

#### `std::string makeGraphIndexKey(std::string_view pk_target, std::string_view pk_edge)`
- Source: `include/storage/key_schema.h`:88
- Brief: Construct key for graph index (incoming edges).
- Parameters:
  - `pk_target` (std::string_view): Input parameter.
  - `pk_edge` (std::string_view): Input parameter.
- Return: Return value.
- Details: Make Graph Index Key. pk_target Input parameter. pk_edge Input parameter. Return value. Calls: reserve(), size().

#### `std::string makeGraphNodeKey(std::string_view pk)`
- Source: `include/storage/key_schema.h`:65
- Brief: Construct key for graph node.
- Parameters:
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: Make Graph Node Key. pk Input parameter. Return value. Calls: reserve(), size().

#### `std::string makeGraphOutdexKey(std::string_view pk_start, std::string_view pk_edge)`
- Source: `include/storage/key_schema.h`:82
- Brief: Construct key for graph outdex (outgoing edges).
- Parameters:
  - `pk_start` (std::string_view): Input parameter.
  - `pk_edge` (std::string_view): Input parameter.
- Return: Return value.
- Details: Make Graph Outdex Key. pk_start Input parameter. pk_edge Input parameter. Return value. Calls: reserve(), size().

#### `std::string makeRelationalKey(std::string_view table, std::string_view pk)`
- Source: `include/storage/key_schema.h`:59
- Brief: Construct key for relational table row.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: Make Relational Key. table Input parameter. pk Input parameter. Return value. Calls: reserve(), size().

#### `std::string makeSecondaryIndexKey(std::string_view table, std::string_view column, std::string_view value, std::string_view pk)`
- Source: `include/storage/key_schema.h`:74
- Brief: Construct key for secondary index entry.
- Parameters:
  - `table` (std::string_view): Input parameter.
  - `column` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: Make Secondary Index Key. table Input parameter. column Input parameter. value Input parameter. pk Input parameter. Return value. Calls: reserve(), size().

#### `std::string makeVectorKey(std::string_view object_name, std::string_view pk)`
- Source: `include/storage/key_schema.h`:71
- Brief: Construct key for vector object.
- Parameters:
  - `object_name` (std::string_view): Name of the object.
  - `pk` (std::string_view): Input parameter.
- Return: Return value.
- Details: Make Vector Key. object_name Name of the object. pk Input parameter. Return value. Calls: reserve(), size().

#### `KeyType parseKeyType(std::string_view key)`
- Source: `include/storage/key_schema.h`:94
- Brief: Parse key type from key string.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Return value.
- Details: Parse Key Type. key Input parameter. Return value. Calls: starts_with().

### themis::MVCCChainPruner

#### `MVCCChainPruner(std::shared_ptr< MVCCStore > mvcc, std::shared_ptr< themisdb::temporal::TemporalTierManager > tier_manager)`
- Source: `include/storage/mvcc_chain_pruner.h`:97
- Brief: Construct a pruner backed by mvcc and tier_manager.
- Parameters:
  - `mvcc` (std::shared_ptr< MVCCStore >): The MVCC store to prune versions from.
  - `tier_manager` (std::shared_ptr< themisdb::temporal::TemporalTierManager >): The temporal tier manager to migrate versions into.
- Details: Both references are retained as shared_ptr for the lifetime of the pruner; the caller must ensure they remain valid. mvcc The MVCC store to prune versions from. tier_manager The temporal tier manager to migrate versions into.

#### `PruneStats pruneAll(HLCTimestamp gc_horizon, Config config=Config{})`
- Source: `include/storage/mvcc_chain_pruner.h`:137
- Brief: Prune every key in the MVCC store whose versions fall below gc_horizon.
- Parameters:
  - `gc_horizon` (HLCTimestamp): Input parameter.
  - `config` (Config): Input parameter.
- Return: Aggregate statistics across all pruned keys.
- Details: Prune All. Internally enumerates all distinct base keys via MVCCStore::scanBaseKeys(), then calls pruneKey() for each. This is an O(N) full-store scan; prefer per-key invocations in hot paths. gc_horizon Versions older than this timestamp are migrated. config Optional tuning parameters. Aggregate statistics across all pruned keys. gc_horizon Input parameter. config Input parameter. Return value. Calls: scanBaseKeys(), pruneKey().

#### `PruneStats pruneAllSafe(Config config=Config{})`
- Source: `include/storage/mvcc_chain_pruner.h`:172
- Brief: Prune all MVCC versions that are below the current safe horizon.
- Parameters:
  - `config` (Config): Input parameter.
- Return: Return value.
- Details: Prune All Safe. Convenience wrapper: equivalent to pruneAll(safeHorizon(), config). Returns an empty PruneStats if safeHorizon() is zero. config Input parameter. Return value. Calls: safeHorizon(), pruneAll().

#### `PruneStats pruneKey(std::string_view key, HLCTimestamp gc_horizon, Config config=Config{})`
- Source: `include/storage/mvcc_chain_pruner.h`:117
- Brief: Prune a single key.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `gc_horizon` (HLCTimestamp): Input parameter.
  - `config` (Config): Input parameter.
- Return: Statistics for this key.
- Details: Prune Key. All MVCC versions of key with timestamp strictly less than gc_horizon are migrated to the TemporalTierManager and then deleted from the MVCC store. The config.min_versions_to_keep most-recent versions are protected from migration/deletion regardless of timestamp. key The logical record key (base key, without timestamp). gc_horizon All versions with ts < gc_horizon are candidates. config Optional tuning parameters. Statistics for this key. key Input parameter. gc_horizon Input parameter. config Input parameter. Return value. Calls: scanVersions(), push_back(), size(), std::min(), key_str(), valueToDocument(), toTemporalTs(), insert().

#### `HLCTimestamp safeHorizon() const noexcept`
- Source: `include/storage/mvcc_chain_pruner.h`:151
- Brief: Return the current safe GC horizon.
- Parameters: none
- Details: Only versions strictly older than this timestamp can be pruned without risk of breaking an active snapshot read. Returns a zero-valued HLCTimestamp by default, meaning "nothing is safe to prune" until setSafeHorizon() has been called (e.g. by the transaction manager after all active transactions are accounted for).

#### `void setSafeHorizon(HLCTimestamp horizon) noexcept`
- Source: `include/storage/mvcc_chain_pruner.h`:164
- Brief: Set the safe GC horizon.
- Parameters:
  - `horizon` (HLCTimestamp): New safe horizon. All MVCC versions with timestamp strictly less than this value are eligible for pruning.
- Details: Typically called by the transaction manager after computing the oldest active read timestamp across all open transactions. The new horizon must be monotonically non-decreasing; a smaller value is silently ignored. horizon New safe horizon. All MVCC versions with timestamp strictly less than this value are eligible for pruning.

#### `themisdb::temporal::Timestamp toTemporalTs(HLCTimestamp ts) noexcept`
- Source: `include/storage/mvcc_chain_pruner.h`:196
- Brief: Convert an HLCTimestamp to the temporal::Timestamp (int64_t) used by TemporalTierManager.
- Parameters:
  - `ts` (HLCTimestamp): n/a
- Details: The raw uint64_t value is reinterpreted as int64_t. HLC timestamps are monotonically increasing and the physical component (physical_ms << 20) stays well below INT64_MAX for any plausible wall-clock date.

#### `themisdb::temporal::Document valueToDocument(const std::vector< uint8_t > &raw)`
- Source: `include/storage/mvcc_chain_pruner.h`:184
- Brief: Convert a raw MVCC VersionEntry value to a nlohmann::json Document for storage in TemporalTierManager.
- Parameters:
  - `raw` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: Value To Document. If raw contains valid UTF-8 JSON, it is parsed directly. Otherwise the bytes are hex-encoded and wrapped as {"_raw": "<hex>"}. raw Input parameter. Return value. Calls: empty(), data(), nlohmann::json::parse(), size(), std::setfill(), std::setw(), str().

### themis::MVCCChainPruner::Config

#### `Config()`
- Source: `include/storage/mvcc_chain_pruner.h`:59
- Brief: n/a
- Parameters: none

### themis::MVCCChainPruner::PruneStats

#### `PruneStats & operator+=(const PruneStats &o) noexcept`
- Source: `include/storage/mvcc_chain_pruner.h`:77
- Brief: n/a
- Parameters:
  - `o` (const PruneStats &): n/a

### themis::MVCCStore

#### `MVCCStore(std::shared_ptr< RocksDBWrapper > db, std::shared_ptr< HybridLogicalClock > clock=nullptr)`
- Source: `include/storage/mvcc_store.h`:92
- Brief: Construct an MVCCStore backed by an existing RocksDBWrapper.
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): Shared ownership of the underlying RocksDB instance. Must already be open.
  - `clock` (std::shared_ptr< HybridLogicalClock >): Shared ownership of the HLC clock. If null, a new HybridLogicalClock is created internally.
- Details: db Shared ownership of the underlying RocksDB instance. Must already be open. clock Shared ownership of the HLC clock. If null, a new HybridLogicalClock is created internally.

#### `HLCTimestamp currentTimestamp() const`
- Source: `include/storage/mvcc_store.h`:245
- Brief: n/a
- Parameters: none
- Details: Return the current HLC timestamp without advancing it.

#### `HLCTimestamp decodeTimestamp(std::string_view versioned_key)`
- Source: `include/storage/mvcc_store.h`:277
- Brief: Extract the timestamp from a versioned key produced by encodeVersionedKey().
- Parameters:
  - `versioned_key` (std::string_view): Input parameter.
- Return: The decoded HLCTimestamp, or a zero-valued timestamp if key is shorter than 9 bytes (and therefore cannot be a valid versioned key).
- Details: Decode Timestamp. The timestamp occupies the last 8 bytes of a versioned key. This function uses a fixed-width offset from the end of the key rather than searching for the '\0' separator, because the 8-byte big-endian timestamp can itself contain '\0' bytes. The decoded HLCTimestamp, or a zero-valued timestamp if key is shorter than 9 bytes (and therefore cannot be a valid versioned key). versioned_key Input parameter. Return value. Calls: size(), data(), HLCTimestamp::decodeFromBytes().

#### `HLCTimestamp delInTxn(RocksDBWrapper::TransactionWrapper &txn, std::string_view key)`
- Source: `include/storage/mvcc_store.h`:132
- Brief: Write a tombstone history entry for key within an existing transaction.
- Parameters:
  - `txn` (RocksDBWrapper::TransactionWrapper &): Input/output parameter.
  - `key` (std::string_view): Input parameter.
- Return: The HLC timestamp assigned to this tombstone entry.
- Details: Del In Txn. Records a deletion event in the history keyspace (op = "del"). txn Active RocksDB transaction. key The base (live) key that was deleted. The HLC timestamp assigned to this tombstone entry. txn Input/output parameter. key Input parameter. Return value. Calls: now(), encodeVersionedKey(), put().

#### `std::string encodeVersionPrefix(std::string_view base_key)`
- Source: `include/storage/mvcc_store.h`:264
- Brief: Build the prefix used to scan all versions of base_key.
- Parameters:
  - `base_key` (std::string_view): Input parameter.
- Return: Return value.
- Details: Encode Version Prefix. Format: <base_key>'\\0' base_key Input parameter. Return value. Calls: reserve(), size(), append(), data(), push_back().

#### `std::string encodeVersionedKey(std::string_view base_key, HLCTimestamp ts)`
- Source: `include/storage/mvcc_store.h`:257
- Brief: Build the versioned storage key for base_key at ts.
- Parameters:
  - `base_key` (std::string_view): Input parameter.
  - `ts` (HLCTimestamp): Input parameter.
- Return: Return value.
- Details: Encode Versioned Key. Format: <base_key>'\\0'<8-byte-big-endian-ts> base_key Input parameter. ts Input parameter. Return value. Calls: reserve(), size(), append(), data(), push_back(), encodeToString().

#### `uint64_t gcAllBefore(HLCTimestamp min_ts)`
- Source: `include/storage/mvcc_store.h`:225
- Brief: n/a
- Parameters:
  - `min_ts` (HLCTimestamp): n/a

#### `uint64_t gcAllBefore(HLCTimestamp min_ts, GCOptions opts)`
- Source: `include/storage/mvcc_store.h`:222
- Brief: Run GC across all keys in the store.
- Parameters:
  - `min_ts` (HLCTimestamp): Input parameter.
  - `opts` (GCOptions): Input parameter.
- Return: Total number of version entries deleted.
- Details: Gc All Before. Iterates the entire keyspace and applies min_ts + opts to every versioned key. This is an O(N) scan – prefer per-key GC in hot paths. Total number of version entries deleted. min_ts Input parameter. opts Input parameter. Return value. Calls: scanBaseKeys(), emplace_back(), gcVersionsBefore().

#### `uint64_t gcVersionsBefore(std::string_view key, HLCTimestamp min_ts)`
- Source: `include/storage/mvcc_store.h`:210
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a
  - `min_ts` (HLCTimestamp): n/a

#### `uint64_t gcVersionsBefore(std::string_view key, HLCTimestamp min_ts, GCOptions opts)`
- Source: `include/storage/mvcc_store.h`:203
- Brief: Remove all versions of key that are strictly older than min_ts.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `min_ts` (HLCTimestamp): Input parameter.
  - `opts` (GCOptions): Input parameter.
- Return: Number of version entries deleted.
- Details: Gc Versions Before. Versions at or after min_ts are kept. The most-recent version is always retained regardless of opts.min_versions_to_keep. key The logical record key. min_ts All versions with timestamp < min_ts may be deleted. opts GC tuning options (e.g., always keep N recent versions). Number of version entries deleted. key Input parameter. min_ts Input parameter. opts Input parameter. Return value. Calls: encodeVersionPrefix(), scanPrefix(), emplace_back(), empty(), size(), decodeTimestamp(), std::min(), del().

#### `std::optional< std::vector< uint8_t > > getAtTimestamp(std::string_view key, HLCTimestamp ts)`
- Source: `include/storage/mvcc_store.h`:171
- Brief: Read the most-recent version of key committed at or before ts.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `ts` (HLCTimestamp): Input parameter.
- Return: The value bytes, or std::nullopt if no version exists at or before ts.
- Details: Get At Timestamp. This provides a consistent snapshot read without acquiring any lock. key The logical record key. ts The read timestamp (snapshot point). The value bytes, or std::nullopt if no version exists at or before ts. key Input parameter. ts Input parameter. Return value. Calls: encodeVersionPrefix(), std::string(), data(), size(), push_back(), encodeVersionedKey(), HLCTimestamp(), newSafeIterator().

#### `std::optional< std::vector< uint8_t > > getLatest(std::string_view key)`
- Source: `include/storage/mvcc_store.h`:159
- Brief: Read the latest committed version of key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: The value bytes, or std::nullopt if the key does not exist.
- Details: Get Latest. The value bytes, or std::nullopt if the key does not exist. key Input parameter. Return value. Calls: lk(), find(), std::string(), end(), encodeVersionedKey(), unlock(), get(), empty().

#### `HLCTimestamp put(std::string_view key, const std::vector< uint8_t > &value)`
- Source: `include/storage/mvcc_store.h`:104
- Brief: Write a new version of key with an auto-generated HLC timestamp.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
- Return: The HLC timestamp assigned to this version.
- Details: Put. The HLC timestamp assigned to this version. key Input parameter. value Input parameter. Return value. Calls: now(), putWithTimestamp().

#### `HLCTimestamp putInTxn(RocksDBWrapper::TransactionWrapper &txn, std::string_view key, const std::vector< uint8_t > &value)`
- Source: `include/storage/mvcc_store.h`:117
- Brief: Write a versioned history entry for key within an existing transaction.
- Parameters:
  - `txn` (RocksDBWrapper::TransactionWrapper &): Input/output parameter.
  - `key` (std::string_view): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
- Return: The HLC timestamp assigned to this history entry.
- Details: Put In Txn. Both the live-key write and this call share txn so they commit atomically. The HLC clock is advanced and the timestamp is returned. txn Active RocksDB transaction. key The base (live) key whose value is being recorded. value The value bytes to store in the history entry. The HLC timestamp assigned to this history entry. txn Input/output parameter. key Input parameter. value Input parameter. Return value. Calls: now(), encodeVersionedKey(), put().

#### `void putWithTimestamp(std::string_view key, const std::vector< uint8_t > &value, HLCTimestamp ts)`
- Source: `include/storage/mvcc_store.h`:146
- Brief: Write a new version of key with an explicit timestamp.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
  - `ts` (HLCTimestamp): Input parameter.
- Details: Put With Timestamp. Useful when the caller already holds a commit timestamp, e.g. from a DistributedTimeCoordinator. The clock is updated to stay ≥ ts. ts Commit timestamp. Must be strictly greater than any previously written version of the same key. key Input parameter. value Input parameter. ts Input parameter. Calls: update(), encodeVersionedKey(), put(), lk(), find(), std::string(), end().

#### `void scanBaseKeys(std::function< bool(std::string_view base_key)> callback)`
- Source: `include/storage/mvcc_store.h`:240
- Brief: Enumerate every distinct base key that has at least one versioned entry in the store.
- Parameters:
  - `callback` (std::function< bool(std::string_view base_key)>): n/a
- Details: Performs a single O(N) full scan. The callback receives each unique base key exactly once. Return false from the callback to stop iteration early. Intended for use by MVCCChainPruner::pruneAll() so that the pruner can migrate-then-delete per-key without duplicating the key-discovery logic.

#### `void scanVersions(std::string_view key, std::function< bool(const VersionEntry &)> callback)`
- Source: `include/storage/mvcc_store.h`:185
- Brief: Enumerate all stored versions of key, oldest first.
- Parameters:
  - `key` (std::string_view): The logical record key.
  - `callback` (std::function< bool(const VersionEntry &)>): Called once per version in ascending timestamp order. Return false to stop iteration early.
- Details: key The logical record key. callback Called once per version in ascending timestamp order. Return false to stop iteration early.

#### `HLCTimestamp updateClock(HLCTimestamp received)`
- Source: `include/storage/mvcc_store.h`:248
- Brief: Update Clock.
- Parameters:
  - `received` (HLCTimestamp): Input parameter.
- Return: Return value.
- Details: Advance the HLC after receiving a remote timestamp. received Input parameter. Return value. Calls: update().

### themis::MaxMergeOperator

#### `bool Merge(const rocksdb::Slice &key, const rocksdb::Slice *existing_value, const rocksdb::Slice &value, std::string *new_value, rocksdb::Logger *logger) const override`
- Source: `include/storage/merge_operators.h`:73
- Brief: n/a
- Parameters:
  - `key` (const rocksdb::Slice &): n/a
  - `existing_value` (const rocksdb::Slice *): n/a
  - `value` (const rocksdb::Slice &): n/a
  - `new_value` (std::string *): n/a
  - `logger` (rocksdb::Logger *): n/a

#### `const char * Name() const override`
- Source: `include/storage/merge_operators.h`:79
- Brief: n/a
- Parameters: none

### themis::PITRManager

#### `PITRManager(PITRManager &&) noexcept=default`
- Source: `include/storage/pitr_manager.h`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (PITRManager &&): n/a

#### `PITRManager(RocksDBWrapper *db, Changefeed *changefeed, transaction::SnapshotManager *snapshot_mgr)`
- Source: `include/storage/pitr_manager.h`:151
- Brief: Construct PITRManager.
- Parameters:
  - `db` (RocksDBWrapper *): RocksDB wrapper instance (not owned)
  - `changefeed` (Changefeed *): Changefeed instance (not owned)
  - `snapshot_mgr` (transaction::SnapshotManager *): SnapshotManager instance (not owned)
- Details: db RocksDB wrapper instance (not owned) changefeed Changefeed instance (not owned) snapshot_mgr SnapshotManager instance (not owned)

#### `PITRManager(const PITRManager &)=delete`
- Source: `include/storage/pitr_manager.h`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PITRManager &): n/a

#### `Status applyEventReverse(const Changefeed::ChangeEvent &event)`
- Source: `include/storage/pitr_manager.h`:306
- Brief: Apply a single event in reverse.
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): Input parameter.
- Return: Return value.
- Details: Apply Event Reverse. PUT event → Delete the key DELETE event → Restore previous value (requires value or before_snapshot) Fails closed when the previous value is unavailable. event Input parameter. Return value. Calls: del(), Status::Error(), has_value(), put(), Status::OK().

#### `Status createAutoBackup(const RestoreOptions &options)`
- Source: `include/storage/pitr_manager.h`:311
- Brief: Create automatic backup before restore.
- Parameters:
  - `options` (const RestoreOptions &): Input parameter.
- Return: Return value.
- Details: Create Auto Backup. options Input parameter. Return value. Calls: createTag(), has_value(), Status::Error(), THEMIS_INFO(), Status::OK().

#### `std::optional< uint64_t > findSequenceForTimestamp(int64_t timestamp_ms) const`
- Source: `include/storage/pitr_manager.h`:285
- Brief: Find sequence number for a given timestamp.
- Parameters:
  - `timestamp_ms` (int64_t): n/a
- Details: Returns the latest sequence <= timestamp

#### `std::optional< RestoreProgress > getProgress() const`
- Source: `include/storage/pitr_manager.h`:243
- Brief: Get current restore progress.
- Parameters: none
- Return: Current progress, or nullopt if no restore in progress
- Details: Current progress, or nullopt if no restore in progress

#### `std::optional< uint64_t > getSequenceForTag(const std::string &tag_name) const`
- Source: `include/storage/pitr_manager.h`:260
- Brief: Get sequence number for a named tag.
- Parameters:
  - `tag_name` (const std::string &): Tag identifier
- Return: Sequence number if tag exists, nullopt otherwise
- Details: tag_name Tag identifier Sequence number if tag exists, nullopt otherwise Useful for API clients that want to convert tags to sequences.

#### `std::optional< uint64_t > getSequenceForTimestamp(int64_t timestamp_ms) const`
- Source: `include/storage/pitr_manager.h`:270
- Brief: Get sequence number for a timestamp.
- Parameters:
  - `timestamp_ms` (int64_t): Unix timestamp in milliseconds
- Return: Latest sequence number <= timestamp, nullopt if no events found
- Details: timestamp_ms Unix timestamp in milliseconds Latest sequence number <= timestamp, nullopt if no events found Useful for API clients that want to convert timestamps to sequences.

#### `bool isRestoreInProgress() const`
- Source: `include/storage/pitr_manager.h`:250
- Brief: Check if a restore operation is currently in progress.
- Parameters: none
- Return: true if restore is active
- Details: true if restore is active

#### `PITRManager & operator=(PITRManager &&) noexcept=default`
- Source: `include/storage/pitr_manager.h`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (PITRManager &&): n/a

#### `PITRManager & operator=(const PITRManager &)=delete`
- Source: `include/storage/pitr_manager.h`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PITRManager &): n/a

#### `RestorePreview previewRestore(uint64_t target_sequence) const`
- Source: `include/storage/pitr_manager.h`:236
- Brief: Preview restore with default options.
- Parameters:
  - `target_sequence` (uint64_t): n/a

#### `RestorePreview previewRestore(uint64_t target_sequence, const RestoreOptions &options) const`
- Source: `include/storage/pitr_manager.h`:233
- Brief: Preview restore operation (dry-run).
- Parameters:
  - `target_sequence` (uint64_t): Sequence to restore to
  - `options` (const RestoreOptions &): Restore options (only tables filter is used)
- Return: Preview with estimated impact
- Details: target_sequence Sequence to restore to options Restore options (only tables filter is used) Preview with estimated impact Useful for: Estimating restore time Checking affected tables/keys Validating restore feasibility

#### `Status replayBackward(uint64_t from_sequence, uint64_t to_sequence, const RestoreOptions &options)`
- Source: `include/storage/pitr_manager.h`:295
- Brief: Replay events backward from current to target sequence.
- Parameters:
  - `from_sequence` (uint64_t): Input parameter.
  - `to_sequence` (uint64_t): Input parameter.
  - `options` (const RestoreOptions &): Input parameter.
- Return: Return value.
- Details: Replay Backward. For each event in reverse order: PUT → DELETE (remove the value) DELETE → PUT (restore the value) TRANSACTION_COMMIT/ROLLBACK → Metadata only, skip from_sequence Input parameter. to_sequence Input parameter. options Input parameter. Return value. Calls: Status::Error(), std::min(), listEvents(), size(), std::to_string(), THEMIS_INFO(), std::reverse(), begin().

#### `Status restoreToSequence(uint64_t target_sequence)`
- Source: `include/storage/pitr_manager.h`:185
- Brief: Restore to sequence with default options.
- Parameters:
  - `target_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: Restore To Sequence. target_sequence Input parameter. Return value. Implements restoreToSequence without additional internal calls.

#### `Status restoreToSequence(uint64_t target_sequence, const RestoreOptions &options)`
- Source: `include/storage/pitr_manager.h`:182
- Brief: Restore database to a specific sequence number.
- Parameters:
  - `target_sequence` (uint64_t): Input parameter.
  - `options` (const RestoreOptions &): Input parameter.
- Return: Status with progress information
- Details: Restore To Sequence. target_sequence Sequence number to restore to options Restore options (dry_run, backup, etc.) Status with progress information Process: Create auto-backup snapshot (if enabled) Validate target sequence Replay events backward from current to target Commit or rollback on error Errors: INVALID_SEQUENCE: Target sequence is invalid or in the future BACKUP_FAILED: Auto-backup creation failed WAL_REPLAY_INCOMPLETE: Required replay events are missing (truncated log) REPLAY_FAILED: Event replay failed target_sequence Input parameter. options Input parameter. Return value. Calls: isRestoreInProgress(), Status::Error(), RestoreProgress::getCurrentTimeMs(), getLatestSequence(), validate(), THEMIS_INFO(), createAutoBackup(), replayBackward().

#### `Status restoreToTag(const std::string &tag_name)`
- Source: `include/storage/pitr_manager.h`:201
- Brief: Restore to tag with default options.
- Parameters:
  - `tag_name` (const std::string &): Name of the tag.
- Return: Return value.
- Details: Restore To Tag. tag_name Name of the tag. Return value. Implements restoreToTag without additional internal calls.

#### `Status restoreToTag(const std::string &tag_name, const RestoreOptions &options)`
- Source: `include/storage/pitr_manager.h`:198
- Brief: Restore database to a named snapshot tag.
- Parameters:
  - `tag_name` (const std::string &): Name of the tag.
  - `options` (const RestoreOptions &): Input parameter.
- Return: Status with progress information
- Details: Restore To Tag. tag_name Tag identifier options Restore options Status with progress information Errors: TAG_NOT_FOUND: Tag does not exist (plus all errors from restoreToSequence) tag_name Name of the tag. options Input parameter. Return value. Calls: getTag(), has_value(), Status::Error(), THEMIS_INFO(), restoreToSequence().

#### `Status restoreToTimestamp(int64_t timestamp_ms)`
- Source: `include/storage/pitr_manager.h`:219
- Brief: Restore to timestamp with default options.
- Parameters:
  - `timestamp_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: Restore To Timestamp. timestamp_ms Input parameter. Return value. Implements restoreToTimestamp without additional internal calls.

#### `Status restoreToTimestamp(int64_t timestamp_ms, const RestoreOptions &options)`
- Source: `include/storage/pitr_manager.h`:216
- Brief: Restore database to a specific timestamp.
- Parameters:
  - `timestamp_ms` (int64_t): Input parameter.
  - `options` (const RestoreOptions &): Input parameter.
- Return: Status with progress information
- Details: Restore To Timestamp. timestamp_ms Unix timestamp in milliseconds options Restore options Status with progress information Finds the latest sequence number <= timestamp and restores to it. Errors: NO_EVENTS_AT_TIME: No events found at or before timestamp (plus all errors from restoreToSequence) timestamp_ms Input parameter. options Input parameter. Return value. Calls: findSequenceForTimestamp(), has_value(), Status::Error(), THEMIS_INFO(), value(), restoreToSequence().

#### `void updateProgress(RestoreProgress::Phase phase, const std::string &message="")`
- Source: `include/storage/pitr_manager.h`:321
- Brief: Update progress tracking.
- Parameters:
  - `phase` (RestoreProgress::Phase): Input parameter.
  - `message` (const std::string &): Input parameter.
- Details: Update Progress. phase Input parameter. message Input parameter. Calls: empty().

#### `Status validate(uint64_t target_sequence, uint64_t current_sequence) const`
- Source: `include/storage/pitr_manager.h`:316
- Brief: Validate restore parameters.
- Parameters:
  - `target_sequence` (uint64_t): n/a
  - `current_sequence` (uint64_t): n/a

#### `~PITRManager()=default`
- Source: `include/storage/pitr_manager.h`:155
- Brief: n/a
- Parameters: none

### themis::PITRManager::RestoreProgress

#### `int64_t getCurrentTimeMs()`
- Source: `include/storage/pitr_manager.h`:127
- Brief: Get Current Time Ms.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::system_clock::now(), time_since_epoch(), count().

#### `int64_t getElapsedMs() const`
- Source: `include/storage/pitr_manager.h`:119
- Brief: n/a
- Parameters: none

#### `double getProgressPercent() const`
- Source: `include/storage/pitr_manager.h`:112
- Brief: n/a
- Parameters: none

### themis::PITRManager::Status

#### `Status Error(std::string msg)`
- Source: `include/storage/pitr_manager.h`:139
- Brief: n/a
- Parameters:
  - `msg` (std::string): n/a

#### `Status OK()`
- Source: `include/storage/pitr_manager.h`:138
- Brief: n/a
- Parameters: none

#### `Status WithProgress(RestoreProgress prog)`
- Source: `include/storage/pitr_manager.h`:140
- Brief: n/a
- Parameters:
  - `prog` (RestoreProgress): n/a

### themis::RaftMvccBridge

#### `RaftMvccBridge(std::shared_ptr< MVCCStore > mvcc_store, std::shared_ptr< themisdb::sharding::DistributedTimeCoordinator > coordinator)`
- Source: `include/storage/raft_mvcc_bridge.h`:106
- Brief: Construct a RaftMvccBridge.
- Parameters:
  - `mvcc_store` (std::shared_ptr< MVCCStore >): Shared MVCC store for this shard.
  - `coordinator` (std::shared_ptr< themisdb::sharding::DistributedTimeCoordinator >): Shared DistributedTimeCoordinator backed by the Raft consensus module for this shard.
- Details: mvcc_store Shared MVCC store for this shard. coordinator Shared DistributedTimeCoordinator backed by the Raft consensus module for this shard.

#### `std::shared_ptr< themisdb::sharding::DistributedTimeCoordinator > coordinator() const`
- Source: `include/storage/raft_mvcc_bridge.h`:200
- Brief: n/a
- Parameters: none
- Return: The underlying DistributedTimeCoordinator.
- Details: The underlying DistributedTimeCoordinator.

#### `bool isLeader() const`
- Source: `include/storage/raft_mvcc_bridge.h`:193
- Brief: n/a
- Parameters: none
- Return: Whether this node is the current Raft leader.
- Details: Whether this node is the current Raft leader.

#### `LinearizableResult linearizableRead(std::string_view key)`
- Source: `include/storage/raft_mvcc_bridge.h`:152
- Brief: Linearizable read: only succeeds on the Raft leader.
- Parameters:
  - `key` (std::string_view): The logical record key.
- Return: LinearizableResult with is_leader and optionally value.
- Details: Returns the latest committed version of key. If this node is not the Raft leader, is_leader will be false and value will be empty — the caller must retry on the leader returned by DistributedTimeCoordinator / its underlying ConsensusModule. key The logical record key. LinearizableResult with is_leader and optionally value.

#### `std::shared_ptr< MVCCStore > mvccStore() const`
- Source: `include/storage/raft_mvcc_bridge.h`:196
- Brief: n/a
- Parameters: none
- Return: The underlying MVCC store.
- Details: The underlying MVCC store.

#### `HLCTimestamp raftAwareWrite(std::string_view key, const std::vector< uint8_t > &value)`
- Source: `include/storage/raft_mvcc_bridge.h`:185
- Brief: Raft-aware write: stamps the value with a Raft-consistent HLC.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
- Return: The HLC timestamp assigned to this write.
- Throws:
  - std::runtime_error: if this node is not the current Raft leader.
  - std::runtime_error: if an error occurs.
- Details: Raft Aware Write. Obtains the current HLC advanced to the Raft coordinator's view of time and calls MVCCStore::putWithTimestamp so the write is visible to snapshot reads that use snapshotTimestamp() after this call returns. This call is leader-only and throws if invoked on a follower. key The logical record key. value Value bytes to store. The HLC timestamp assigned to this write. std::runtime_error if this node is not the current Raft leader. key Input parameter. value Input parameter. Return value. std::runtime_error if an error occurs. Calls: isLeader(), snapshotTimestamp(), putWithTimestamp(), spdlog::debug(), std::string().

#### `std::optional< std::vector< uint8_t > > snapshotRead(std::string_view key, HLCTimestamp ts)`
- Source: `include/storage/raft_mvcc_bridge.h`:165
- Brief: Snapshot read at a specific HLC timestamp.
- Parameters:
  - `key` (std::string_view): The logical record key.
  - `ts` (HLCTimestamp): The snapshot point (from snapshotTimestamp()).
- Return: The value bytes, or std::nullopt if no version exists ≤ ts.
- Details: Reads the most-recent version of key committed at or before ts. Safe to call on any replica (leader or follower) — data visibility is determined entirely by ts. key The logical record key. ts The snapshot point (from snapshotTimestamp()). The value bytes, or std::nullopt if no version exists ≤ ts.

#### `HLCTimestamp snapshotTimestamp()`
- Source: `include/storage/raft_mvcc_bridge.h`:124
- Brief: Produce an HLC snapshot timestamp consistent with the Raft commit index.
- Parameters: none
- Return: HLC timestamp consistent with the current Raft commit point.
- Details: Snapshot Timestamp. Advances the MVCCStore's HLC to be ≥ the synthetic HLC derived from the coordinator's current view (system wall-clock + Raft log index). The returned timestamp can be used directly in snapshotRead() or MVCCStore::getAtTimestamp(). HLC timestamp consistent with the current Raft commit point. Return value. Calls: now(), toHlcTimestamp(), updateClock(), spdlog::debug().

#### `HLCTimestamp toHlcTimestamp(const themisdb::sharding::DistributedTimeCoordinator::TimeInterval &interval)`
- Source: `include/storage/raft_mvcc_bridge.h`:135
- Brief: Convert a Raft TimeInterval to an HLC timestamp.
- Parameters:
  - `interval` (const themisdb::sharding::DistributedTimeCoordinator::TimeInterval &): Input parameter.
- Return: Equivalent HLCTimestamp with physical=wall_ms, logical=log_idx&0xFFFFF.
- Details: static This is a pure static conversion — it does not advance any clock. Use snapshotTimestamp() if you also need to advance the local clock. interval A TimeInterval returned by DistributedTimeCoordinator::now(). Equivalent HLCTimestamp with physical=wall_ms, logical=log_idx&0xFFFFF. interval Input parameter. Return value. Calls: HLCTimestamp::from().

### themis::RocksDBWrapper

#### `RocksDBWrapper(RocksDBWrapper &&) noexcept`
- Source: `include/storage/rocksdb_wrapper.h`:291
- Brief: n/a
- Parameters:
  - `other` (RocksDBWrapper &&): n/a

#### `RocksDBWrapper(const Config &config)`
- Source: `include/storage/rocksdb_wrapper.h`:285
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `RocksDBWrapper(const RocksDBWrapper &)=delete`
- Source: `include/storage/rocksdb_wrapper.h`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RocksDBWrapper &): n/a

#### `void addEventListener(std::shared_ptr< rocksdb::EventListener > listener)`
- Source: `include/storage/rocksdb_wrapper.h`:306
- Brief: Register a RocksDB event listener.
- Parameters:
  - `listener` (std::shared_ptr< rocksdb::EventListener >): Input parameter.
- Details: Add Event Listener. listener Listener that will receive compaction/flush/deletion events. listener Input parameter. Calls: lock(), isOpen(), THEMIS_WARN(), emplace_back(), std::move().

#### `std::unique_ptr< TransactionWrapper > beginTransaction(TransactionIsolationLevel isolation=TransactionIsolationLevel::ReadCommitted)`
- Source: `include/storage/rocksdb_wrapper.h`:568
- Brief: Begin Transaction.
- Parameters:
  - `isolation` (TransactionIsolationLevel): Input parameter.
- Return: Return value.
- Details: isolation Input parameter. Return value. Implements beginTransaction without additional internal calls.

#### `void close()`
- Source: `include/storage/rocksdb_wrapper.h`:299
- Brief: Close the database.
- Parameters: none
- Details: Close. Calls: THEMIS_INFO(), lock(), store(), load(), std::this_thread::sleep_for(), std::chrono::milliseconds(), THEMIS_WARN(), size().

#### `bool commitBatch(rocksdb::WriteBatch *batch)`
- Source: `include/storage/rocksdb_wrapper.h`:865
- Brief: Commit Batch.
- Parameters:
  - `batch` (rocksdb::WriteBatch *): Input/output parameter.
- Return: True when the operation succeeds.
- Details: batch Input/output parameter. True when the operation succeeds. Calls: Write(), ok().

#### `void compactRange(std::string_view start_key, std::string_view end_key)`
- Source: `include/storage/rocksdb_wrapper.h`:713
- Brief: Trigger manual compaction.
- Parameters:
  - `start_key` (std::string_view): Input parameter.
  - `end_key` (std::string_view): Input parameter.
- Details: Compact Range. start_key Range start. end_key Range end. start_key Input parameter. end_key Input parameter. Calls: start(), data(), size(), end(), CompactRange().

#### `void configureOptions()`
- Source: `include/storage/rocksdb_wrapper.h`:864
- Brief: configureOptions() runs during construction before publication to other threads, but it also takes options_mutex_ so later option snapshots (open(), statistics export) see a consistently configured set of RocksDB options.
- Parameters: none
- Details: Calls: options_lock(), rocksdb::CreateDBStatistics(), set_stats_level(), rocksdb::NewLRUCache(), reset(), rocksdb::NewBloomFilterPolicy(), rocksdb::NewBlockBasedTableFactory(), rocksdb::Env::Default().

#### `bool createCheckpoint(const std::string &checkpoint_dir)`
- Source: `include/storage/rocksdb_wrapper.h`:733
- Brief: Create a RocksDB checkpoint.
- Parameters:
  - `checkpoint_dir` (const std::string &): Input parameter.
- Return: true on success.
- Details: Create Checkpoint. checkpoint_dir Destination directory. true on success. checkpoint_dir Input parameter. True when the operation succeeds. Calls: THEMIS_ERROR(), cpp(), parent_path(), empty(), std::filesystem::create_directories(), string(), message(), rocksdb::Checkpoint::Create().

#### `bool createIncrementalBackup(const std::string &backup_dir, bool flush_before_backup=true)`
- Source: `include/storage/rocksdb_wrapper.h`:746
- Brief: Create an incremental backup (only delta since last backup).
- Parameters:
  - `backup_dir` (const std::string &): Input parameter.
  - `flush_before_backup` (bool): Input parameter.
- Return: true on success.
- Details: ===== v1. backup_dir Directory to store backups. flush_before_backup Flush memtables before backup. true on success. backup_dir Input parameter. flush_before_backup Input parameter. True when the operation succeeds. 1.0: Advanced RocksDB Features ===== Calls: THEMIS_HAS_ROCKSDB_BACKUP(), THEMIS_WARN(), THEMIS_ERROR(), GetBaseDB(), backup_opts(), rocksdb::BackupEngine::Open(), rocksdb::Env::Default(), ok().

#### `std::unique_ptr< WriteBatchWrapper > createWriteBatch()`
- Source: `include/storage/rocksdb_wrapper.h`:407
- Brief: Create a new write batch wrapper.
- Parameters: none
- Return: Batch wrapper instance.
- Details: Create Write Batch. Batch wrapper instance. Return value. Implements createWriteBatch without additional internal calls.

#### `std::unique_ptr< WriteBatchWithIndexWrapper > createWriteBatchWithIndex(bool overwrite_key=true)`
- Source: `include/storage/rocksdb_wrapper.h`:447
- Brief: Create a write batch with index.
- Parameters:
  - `overwrite_key` (bool): Input parameter.
- Return: Batch-with-index wrapper instance.
- Details: Create Write Batch With Index. overwrite_key Allow overwriting existing keys. Batch-with-index wrapper instance. overwrite_key Input parameter. Return value. Implements createWriteBatchWithIndex without additional internal calls.

#### `bool del(std::string_view key)`
- Source: `include/storage/rocksdb_wrapper.h`:336
- Brief: Delete a key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: true if at least one entry was removed.
- Details: Del. key Lookup key. true if at least one entry was removed. key Input parameter. True when the operation succeeds. Calls: themis::utils::Logger::error(), beginTransaction(), rollback(), commit().

#### `bool delBlob(std::string_view key)`
- Source: `include/storage/rocksdb_wrapper.h`:365
- Brief: Delete a blob stored by putBlob() or put().
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: true if at least one key was deleted.
- Details: Del Blob. key Logical blob key. true if at least one key was deleted. key Input parameter. True when the operation succeeds. Calls: THEMIS_ERROR(), blobManifestKey(), Get(), rocksdb::Slice(), ok(), size(), std::memcpy(), data().

#### `std::string exportStatisticsJSON() const`
- Source: `include/storage/rocksdb_wrapper.h`:760
- Brief: Export RocksDB statistics as JSON.
- Parameters: none
- Return: JSON object with statistics.
- Details: JSON object with statistics.

#### `void flush()`
- Source: `include/storage/rocksdb_wrapper.h`:716
- Brief: Flush memtable to disk.
- Parameters: none
- Details: Flush. Calls: Flush().

#### `std::optional< std::vector< uint8_t > > get(std::string_view key)`
- Source: `include/storage/rocksdb_wrapper.h`:313
- Brief: Get a value by key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Value bytes if the key exists; std::nullopt otherwise.
- Details: Get. key Lookup key. Value bytes if the key exists; std::nullopt otherwise. key Input parameter. Return value. Calls: Get(), rocksdb::Slice(), data(), size(), ok(), begin(), end().

#### `bool get(std::string_view key, std::string &out)`
- Source: `include/storage/rocksdb_wrapper.h`:319
- Brief: Get a value as a string.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `out` (std::string &): Input/output parameter.
- Return: true if the key exists.
- Details: Get. key Lookup key. out Output string. true if the key exists. key Input parameter. out Input/output parameter. True when the operation succeeds. Calls: Get(), rocksdb::Slice(), data(), size(), ok(), std::move().

#### `uint64_t getApproximateSize() const`
- Source: `include/storage/rocksdb_wrapper.h`:720
- Brief: Get the approximate database size in bytes.
- Parameters: none
- Return: Approximate size in bytes.
- Details: Approximate size in bytes.

#### `uint32_t getBackupCount(const std::string &backup_dir) const`
- Source: `include/storage/rocksdb_wrapper.h`:756
- Brief: Get number of backups available.
- Parameters:
  - `backup_dir` (const std::string &): Directory containing backups.
- Return: Number of backups.
- Details: backup_dir Directory containing backups. Number of backups.

#### `std::optional< std::vector< uint8_t > > getBlob(std::string_view key)`
- Source: `include/storage/rocksdb_wrapper.h`:360
- Brief: Read a blob previously stored by putBlob() or put().
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Blob bytes, or std::nullopt if not found.
- Details: Get Blob. key Logical blob key. Blob bytes, or std::nullopt if not found. key Input parameter. Return value. Calls: blobManifestKey(), Get(), rocksdb::Slice(), ok(), size(), data(), readLE32(), readLE64().

#### `std::string getCompressionType() const`
- Source: `include/storage/rocksdb_wrapper.h`:708
- Brief: Get the active compression type.
- Parameters: none
- Return: Compression type string.
- Details: Compression type string.

#### `const Config & getConfig() const`
- Source: `include/storage/rocksdb_wrapper.h`:723
- Brief: Get the current configuration.
- Parameters: none

#### `rocksdb::TransactionDB * getDB()`
- Source: `include/storage/rocksdb_wrapper.h`:790
- Brief: Backward-compatible alias for getRawDB().
- Parameters: none

#### `const rocksdb::TransactionDB * getDB() const`
- Source: `include/storage/rocksdb_wrapper.h`:792
- Brief: Backward-compatible alias for getRawDB().
- Parameters: none

#### `uint64_t getLatestSequenceNumber() const`
- Source: `include/storage/rocksdb_wrapper.h`:727
- Brief: Get the latest RocksDB sequence number.
- Parameters: none
- Return: Sequence number, or 0 if the database is not open.
- Details: Sequence number, or 0 if the database is not open.

#### `Result< rocksdb::ColumnFamilyHandle * > getOrCreateColumnFamily(const std::string &cf_name)`
- Source: `include/storage/rocksdb_wrapper.h`:772
- Brief: Create or open a column family.
- Parameters:
  - `cf_name` (const std::string &): Name of the cf.
- Return: Handle or an error.
- Details: Get Or Create Column Family. cf_name Column family name. Handle or an error. cf_name Name of the cf. Return value. Calls: lock(), THEMIS_ERROR(), GetName(), THEMIS_DEBUG(), Ok(), CreateColumnFamily(), ok(), ToString().

#### `rocksdb::TransactionDB * getRawDB()`
- Source: `include/storage/rocksdb_wrapper.h`:786
- Brief: Get the raw RocksDB pointer.
- Parameters: none

#### `const rocksdb::TransactionDB * getRawDB() const`
- Source: `include/storage/rocksdb_wrapper.h`:788
- Brief: Get the raw RocksDB pointer.
- Parameters: none

#### `uint64_t getStatistic(const std::string &ticker_name) const`
- Source: `include/storage/rocksdb_wrapper.h`:765
- Brief: Get a specific statistic value by ticker name.
- Parameters:
  - `ticker_name` (const std::string &): Name of the ticker.
- Return: Ticker value.
- Details: ticker_name Name of the ticker. Ticker value.

#### `std::string getStats() const`
- Source: `include/storage/rocksdb_wrapper.h`:704
- Brief: Get database statistics.
- Parameters: none
- Return: Human-readable statistics string.
- Details: Human-readable statistics string.

#### `bool isAsyncIOEnabled() const`
- Source: `include/storage/rocksdb_wrapper.h`:698
- Brief: Check if async I/O is enabled.
- Parameters: none

#### `bool isOpen() const`
- Source: `include/storage/rocksdb_wrapper.h`:302
- Brief: Check if database is open.
- Parameters: none

#### `void iterateRange(std::string_view start_key, std::string_view end_key, ScanCallback callback)`
- Source: `include/storage/rocksdb_wrapper.h`:656
- Brief: Iterate over a key range using a RocksDB iterator.
- Parameters:
  - `start_key` (std::string_view): Input parameter.
  - `end_key` (std::string_view): Input parameter.
  - `callback` (ScanCallback): Input parameter.
- Details: Iterate Range. start_key Range start. end_key Range end. callback Callback invoked for each entry. start_key Input parameter. end_key Input parameter. callback Input parameter. Calls: guard(), get(), GetBaseDB(), THEMIS_ERROR(), it(), NewIterator(), start_slice(), data().

#### `std::vector< CFInfo > listColumnFamilies() const`
- Source: `include/storage/rocksdb_wrapper.h`:783
- Brief: Enumerate all open column families with lightweight statistics.
- Parameters: none
- Return: Vector of CFInfo entries, or empty if the DB is not open.
- Details: Vector of CFInfo entries, or empty if the DB is not open.

#### `std::vector< std::optional< std::vector< uint8_t > > > multiGet(const std::vector< std::string > &keys)`
- Source: `include/storage/rocksdb_wrapper.h`:370
- Brief: Multi-get batch read.
- Parameters:
  - `keys` (const std::vector< std::string > &): Input parameter.
- Return: Values aligned with the input keys.
- Details: Multi Get. keys Lookup keys. Values aligned with the input keys. keys Input parameter. Return value. Calls: THEMIS_ERROR(), GetBaseDB(), reserve(), size(), emplace_back(), MultiGet(), ok(), empty().

#### `std::vector< std::optional< std::vector< uint8_t > > > multiGetWithAsyncIO(const std::vector< std::string > &keys)`
- Source: `include/storage/rocksdb_wrapper.h`:686
- Brief: MultiGet with async I/O optimization.
- Parameters:
  - `keys` (const std::vector< std::string > &): Input parameter.
- Return: Values aligned with the input keys.
- Details: Multi Get With Async IO. keys Lookup keys. Values aligned with the input keys. keys Input parameter. Return value. Calls: reserve(), size(), THEMIS_ERROR(), GetBaseDB(), emplace_back(), MultiGet(), ok(), begin().

#### `Result< std::unique_ptr< rocksdb::Iterator > > newAsyncIterator()`
- Source: `include/storage/rocksdb_wrapper.h`:691
- Brief: Create an async iterator with prefetching.
- Parameters: none
- Return: Iterator or an error.
- Details: New Async Iterator. Iterator or an error. Return value. Calls: GetBaseDB(), NewIterator(), Ok(), std::move().

#### `Result< std::unique_ptr< rocksdb::Iterator > > newIterator()`
- Source: `include/storage/rocksdb_wrapper.h`:695
- Brief: Create a standard iterator.
- Parameters: none
- Return: Iterator or an error.
- Details: New Iterator. Iterator or an error. Return value. Calls: GetBaseDB(), NewIterator(), Ok(), std::move().

#### `Result< SafeIterator > newSafeIterator(const rocksdb::ReadOptions *read_options=nullptr)`
- Source: `include/storage/rocksdb_wrapper.h`:633
- Brief: Create a safe iterator with automatic lifecycle management.
- Parameters:
  - `read_options` (const rocksdb::ReadOptions *): Input parameter.
- Return: Safe iterator wrapper or an error.
- Details: SafeIterator implementation - SOLUTION 1B for iterator lifecycle safety. read_options Optional read options. Safe iterator wrapper or an error. read_options Input parameter. Return value. Calls: get(), GetBaseDB(), NewIterator(), Ok(), SafeIterator(), std::move().

#### `bool open()`
- Source: `include/storage/rocksdb_wrapper.h`:296
- Brief: Open the database.
- Parameters: none
- Return: true on success.
- Details: Open. true on success. True when the operation succeeds. Calls: close(), dbp(), parent_path(), empty(), std::filesystem::create_directories(), std::string(), string(), message().

#### `RocksDBWrapper & operator=(RocksDBWrapper &&) noexcept`
- Source: `include/storage/rocksdb_wrapper.h`:292
- Brief: n/a
- Parameters:
  - `other` (RocksDBWrapper &&): n/a

#### `RocksDBWrapper & operator=(const RocksDBWrapper &)=delete`
- Source: `include/storage/rocksdb_wrapper.h`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RocksDBWrapper &): n/a

#### `Result< SafeIterator > prefixIterator(std::string_view prefix)`
- Source: `include/storage/rocksdb_wrapper.h`:644
- Brief: Create a prefix iterator for enumeration.
- Parameters:
  - `prefix` (std::string_view): Input parameter.
- Return: Iterator positioned at the first matching key, or an error.
- Details: Prefix Iterator. prefix Prefix to search for. Iterator positioned at the first matching key, or an error. prefix Input parameter. Return value. Calls: newSafeIterator(), get(), std::move(), value(), Seek(), std::string(), Ok().

#### `bool put(std::string_view key, const std::vector< uint8_t > &value)`
- Source: `include/storage/rocksdb_wrapper.h`:325
- Brief: Store a key-value pair.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
- Return: true on success.
- Details: Put. key Lookup key. value Value bytes. true on success. key Input parameter. value Input parameter. True when the operation succeeds. Calls: themis::utils::Logger::error(), beginTransaction(), rollback(), commit().

#### `bool put(std::string_view key, std::string_view value)`
- Source: `include/storage/rocksdb_wrapper.h`:331
- Brief: Store a string value.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
- Return: true on success.
- Details: Put. key Lookup key. value String value. true on success. key Input parameter. value Input parameter. True when the operation succeeds. Calls: themis::utils::Logger::error(), beginTransaction(), val_vec(), begin(), end(), rollback(), commit().

#### `bool putBatch(const std::vector< KeyValuePair > &pairs)`
- Source: `include/storage/rocksdb_wrapper.h`:347
- Brief: Write multiple key-value pairs atomically.
- Parameters:
  - `pairs` (const std::vector< KeyValuePair > &): Input parameter.
- Return: true if all writes were committed successfully.
- Details: Put Batch. pairs Key-value pairs to write. true if all writes were committed successfully. pairs Input parameter. True when the operation succeeds. Calls: themis::utils::Logger::error(), empty(), createWriteBatch(), put(), commit().

#### `bool putBlob(std::string_view key, const std::vector< uint8_t > &data)`
- Source: `include/storage/rocksdb_wrapper.h`:355
- Brief: Store a blob using the streaming write path.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: true on success.
- Details: Put Blob. key Logical blob key. data Blob bytes. true on success. key Input parameter. data Input parameter. True when the operation succeeds. Calls: THEMIS_ERROR(), size(), put(), std::max(), std::min(), encoded_chunks(), reserve(), emplace_back().

#### `std::vector< std::pair< std::string, std::vector< uint8_t > > > rangeQueryWithAsyncIO(std::string_view start_key, std::string_view end_key)`
- Source: `include/storage/rocksdb_wrapper.h`:674
- Brief: Range query with async I/O.
- Parameters:
  - `start_key` (std::string_view): Range start.
  - `end_key` (std::string_view): Range end.
- Return: Key/value pairs in range order.
- Details: start_key Range start. end_key Range end. Key/value pairs in range order.

#### `bool restoreFromBackup(const std::string &backup_dir)`
- Source: `include/storage/rocksdb_wrapper.h`:751
- Brief: Restore from the latest backup.
- Parameters:
  - `backup_dir` (const std::string &): Input parameter.
- Return: true on success.
- Details: Restore From Backup. backup_dir Directory containing backups. true on success. backup_dir Input parameter. True when the operation succeeds. Calls: THEMIS_HAS_ROCKSDB_BACKUP(), THEMIS_WARN(), backup_opts(), rocksdb::BackupEngine::Open(), rocksdb::Env::Default(), ok(), THEMIS_ERROR(), ToString().

#### `bool restoreFromCheckpoint(const std::string &checkpoint_dir)`
- Source: `include/storage/rocksdb_wrapper.h`:738
- Brief: Restore the database from a previously created checkpoint directory.
- Parameters:
  - `checkpoint_dir` (const std::string &): Input parameter.
- Return: true on success.
- Details: Restore From Checkpoint. checkpoint_dir Checkpoint directory. true on success. checkpoint_dir Input parameter. True when the operation succeeds. Calls: std::filesystem::exists(), THEMIS_ERROR(), close(), std::filesystem::remove_all(), message(), std::filesystem::create_directories(), std::filesystem::copy(), open().

#### `std::vector< std::pair< std::string, std::vector< uint8_t > > > reverseScanWithAsyncIO(std::string_view start_key, int limit=1000)`
- Source: `include/storage/rocksdb_wrapper.h`:680
- Brief: Reverse scan with async I/O.
- Parameters:
  - `start_key` (std::string_view): Range start.
  - `limit` (int): Maximum number of rows.
- Details: start_key Range start. limit Maximum number of rows.

#### `void scanAll(ScanCallback callback)`
- Source: `include/storage/rocksdb_wrapper.h`:660
- Brief: Scan the whole database.
- Parameters:
  - `callback` (ScanCallback): Input parameter.
- Details: Scan All. callback Callback invoked for each entry. callback Input parameter. Calls: guard(), get(), GetBaseDB(), THEMIS_ERROR(), it(), NewIterator(), SeekToFirst(), Valid().

#### `void scanPrefix(std::string_view prefix, ScanCallback callback)`
- Source: `include/storage/rocksdb_wrapper.h`:639
- Brief: Scan entries that share a prefix.
- Parameters:
  - `prefix` (std::string_view): Input parameter.
  - `callback` (ScanCallback): Input parameter.
- Details: Scan Prefix. prefix Prefix to match. callback Callback invoked for each entry. prefix Input parameter. callback Input parameter. Calls: guard(), get(), GetBaseDB(), THEMIS_ERROR(), it(), NewIterator(), prefix_slice(), data().

#### `void scanRange(std::string_view start_key, std::string_view end_key, ScanCallback callback)`
- Source: `include/storage/rocksdb_wrapper.h`:650
- Brief: Scan range [start_key, end_key).
- Parameters:
  - `start_key` (std::string_view): Input parameter.
  - `end_key` (std::string_view): Input parameter.
  - `callback` (ScanCallback): Input parameter.
- Details: Scan Range. start_key Range start. end_key Range end. callback Callback invoked for each entry. start_key Input parameter. end_key Input parameter. callback Input parameter. Calls: guard(), get(), GetBaseDB(), THEMIS_ERROR(), it(), NewIterator(), start_slice(), data().

#### `std::vector< std::pair< std::string, std::vector< uint8_t > > > scanWithAsyncIO(std::string_view prefix, int limit=1000)`
- Source: `include/storage/rocksdb_wrapper.h`:667
- Brief: Scan with async I/O and prefetching.
- Parameters:
  - `prefix` (std::string_view): Prefix to match.
  - `limit` (int): Maximum number of rows.
- Details: prefix Prefix to match. limit Maximum number of rows.

#### `~RocksDBWrapper()`
- Source: `include/storage/rocksdb_wrapper.h`:286
- Brief: n/a
- Parameters: none

### themis::RocksDBWrapper::OperationGuard

#### `OperationGuard(const OperationGuard &)=delete`
- Source: `include/storage/rocksdb_wrapper.h`:821
- Brief: n/a
- Parameters:
  - `<unnamed>` (const OperationGuard &): n/a

#### `OperationGuard(const RocksDBWrapper *wrapper)`
- Source: `include/storage/rocksdb_wrapper.h`:800
- Brief: n/a
- Parameters:
  - `wrapper` (const RocksDBWrapper *): n/a

#### `rocksdb::TransactionDB * get() const`
- Source: `include/storage/rocksdb_wrapper.h`:827
- Brief: Get the guarded database pointer.
- Parameters: none
- Return: Raw pointer to the guarded transaction database, or nullptr when no database is currently guarded.
- Details: Raw pointer to the guarded transaction database, or nullptr when no database is currently guarded.

#### `operator bool() const`
- Source: `include/storage/rocksdb_wrapper.h`:828
- Brief: n/a
- Parameters: none

#### `OperationGuard & operator=(const OperationGuard &)=delete`
- Source: `include/storage/rocksdb_wrapper.h`:822
- Brief: n/a
- Parameters:
  - `<unnamed>` (const OperationGuard &): n/a

#### `~OperationGuard()`
- Source: `include/storage/rocksdb_wrapper.h`:815
- Brief: n/a
- Parameters: none

### themis::RocksDBWrapper::SafeIterator

#### `void Next()`
- Source: `include/storage/rocksdb_wrapper.h`:602
- Brief: Advance to the next key.
- Parameters: none
- Details: Next. Implements Next without additional internal calls.

#### `void Prev()`
- Source: `include/storage/rocksdb_wrapper.h`:604
- Brief: Move to the previous key.
- Parameters: none
- Details: Prev. Implements Prev without additional internal calls.

#### `SafeIterator(SafeIterator &&other) noexcept=default`
- Source: `include/storage/rocksdb_wrapper.h`:585
- Brief: n/a
- Parameters:
  - `other` (SafeIterator &&): n/a

#### `SafeIterator(const SafeIterator &)=delete`
- Source: `include/storage/rocksdb_wrapper.h`:589
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SafeIterator &): n/a

#### `SafeIterator(std::unique_ptr< rocksdb::Iterator > iter, std::unique_ptr< OperationGuard > guard)`
- Source: `include/storage/rocksdb_wrapper.h`:621
- Brief: n/a
- Parameters:
  - `iter` (std::unique_ptr< rocksdb::Iterator >): n/a
  - `guard` (std::unique_ptr< OperationGuard >): n/a

#### `void Seek(const std::string &target)`
- Source: `include/storage/rocksdb_wrapper.h`:596
- Brief: Seek to a target key.
- Parameters:
  - `target` (const std::string &): Input parameter.
- Details: Seek. target Key to seek to. target Input parameter. Implements Seek without additional internal calls.

#### `void SeekToFirst()`
- Source: `include/storage/rocksdb_wrapper.h`:598
- Brief: Seek to the first key.
- Parameters: none
- Details: Seek To First. Implements SeekToFirst without additional internal calls.

#### `void SeekToLast()`
- Source: `include/storage/rocksdb_wrapper.h`:600
- Brief: Seek to the last key.
- Parameters: none
- Details: Seek To Last. Implements SeekToLast without additional internal calls.

#### `bool Valid() const`
- Source: `include/storage/rocksdb_wrapper.h`:607
- Brief: Check whether the iterator is positioned at a valid entry.
- Parameters: none
- Return: true when the iterator references a valid key-value pair.
- Details: true when the iterator references a valid key-value pair.

#### `std::string_view key() const`
- Source: `include/storage/rocksdb_wrapper.h`:610
- Brief: Get the current key.
- Parameters: none
- Return: View of the current key.
- Details: View of the current key.

#### `operator bool() const`
- Source: `include/storage/rocksdb_wrapper.h`:616
- Brief: n/a
- Parameters: none

#### `SafeIterator & operator=(SafeIterator &&other) noexcept=default`
- Source: `include/storage/rocksdb_wrapper.h`:586
- Brief: n/a
- Parameters:
  - `other` (SafeIterator &&): n/a

#### `SafeIterator & operator=(const SafeIterator &)=delete`
- Source: `include/storage/rocksdb_wrapper.h`:590
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SafeIterator &): n/a

#### `std::string_view value() const`
- Source: `include/storage/rocksdb_wrapper.h`:613
- Brief: Get the current value.
- Parameters: none
- Return: View of the current value.
- Details: View of the current value.

#### `~SafeIterator()=default`
- Source: `include/storage/rocksdb_wrapper.h`:592
- Brief: n/a
- Parameters: none

### themis::RocksDBWrapper::TransactionWrapper

#### `TransactionWrapper(RocksDBWrapper *db, TransactionIsolationLevel isolation=TransactionIsolationLevel::ReadCommitted)`
- Source: `include/storage/rocksdb_wrapper.h`:471
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper *): n/a
  - `isolation` (TransactionIsolationLevel): n/a

#### `bool commit()`
- Source: `include/storage/rocksdb_wrapper.h`:497
- Brief: Commit the transaction.
- Parameters: none
- Return: true on success.
- Details: Commit. true on success. True when the operation succeeds. Calls: Prepare(), ok(), THEMIS_ERROR(), ToString(), Commit(), IsBusy(), THEMIS_WARN(), IsTimedOut().

#### `bool del(std::string_view key)`
- Source: `include/storage/rocksdb_wrapper.h`:493
- Brief: Delete a key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: true on success.
- Details: Del. key Lookup key. true on success. key Input parameter. True when the operation succeeds. Calls: THEMIS_ERROR(), Delete(), rocksdb::Slice(), data(), size(), ok(), ToString(), what().

#### `std::optional< std::vector< uint8_t > > get(std::string_view key)`
- Source: `include/storage/rocksdb_wrapper.h`:477
- Brief: Get a value with isolation-dependent behavior.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Value bytes if found.
- Details: Get. key Lookup key. Value bytes if found. key Input parameter. Return value. Calls: THEMIS_ERROR(), GetSnapshot(), Get(), rocksdb::Slice(), data(), size(), ok(), begin().

#### `bool getForUpdate(std::string_view key)`
- Source: `include/storage/rocksdb_wrapper.h`:482
- Brief: Acquire an exclusive write lock on a key.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: true if the lock was acquired.
- Details: Get For Update. key Lookup key. true if the lock was acquired. key Input parameter. True when the operation succeeds. Calls: THEMIS_ERROR(), GetSnapshot(), GetForUpdate(), rocksdb::Slice(), data(), size(), ok(), IsNotFound().

#### `CommitFailureType getLastCommitFailureType() const`
- Source: `include/storage/rocksdb_wrapper.h`:556
- Brief: Return the failure classification of the last commit() call.
- Parameters: none

#### `Result< const rocksdb::Snapshot * > getSnapshot() const`
- Source: `include/storage/rocksdb_wrapper.h`:544
- Brief: Get the snapshot.
- Parameters: none

#### `bool isActive() const`
- Source: `include/storage/rocksdb_wrapper.h`:541
- Brief: Check if the transaction is still active.
- Parameters: none

#### `bool popSavePoint()`
- Source: `include/storage/rocksdb_wrapper.h`:536
- Brief: Discard (commit) the most recent savepoint without rolling back.
- Parameters: none
- Return: true if the savepoint was discarded; false if no savepoint is currently active.
- Details: Pop Save Point. The writes since the savepoint become permanent within the transaction. Returns true on success; false if there is no outstanding savepoint. true if the savepoint was discarded; false if no savepoint is currently active. True when the operation succeeds. Calls: PopSavePoint(), IsNotFound(), THEMIS_WARN(), ok(), THEMIS_ERROR(), ToString().

#### `bool prepare()`
- Source: `include/storage/rocksdb_wrapper.h`:504
- Brief: Prepare the transaction.
- Parameters: none
- Return: true on success.
- Details: Prepare. true on success. True when the operation succeeds. Calls: Prepare(), ok(), THEMIS_ERROR(), ToString(), what().

#### `bool put(std::string_view key, const std::vector< uint8_t > &value)`
- Source: `include/storage/rocksdb_wrapper.h`:488
- Brief: Put a key-value pair.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
- Return: true on success.
- Details: Put. key Lookup key. value Value bytes. true on success. key Input parameter. value Input parameter. True when the operation succeeds. Calls: THEMIS_ERROR(), Put(), rocksdb::Slice(), data(), size(), ok(), ToString(), what().

#### `void rollback()`
- Source: `include/storage/rocksdb_wrapper.h`:500
- Brief: Roll back the transaction.
- Parameters: none
- Details: Rollback. Calls: Rollback(), THEMIS_DEBUG(), THEMIS_ERROR(), what().

#### `bool rollbackToSavePoint()`
- Source: `include/storage/rocksdb_wrapper.h`:525
- Brief: Rollback all writes made after the most recent setSavePoint().
- Parameters: none
- Return: true if the savepoint rollback succeeded; false if no savepoint is currently active.
- Details: Rollback To Save Point. Pops the most recent savepoint from the stack. Returns true on success; returns false if there is no outstanding savepoint. true if the savepoint rollback succeeded; false if no savepoint is currently active. True when the operation succeeds. Calls: RollbackToSavePoint(), IsNotFound(), THEMIS_WARN(), ok(), THEMIS_ERROR(), ToString().

#### `void setSavePoint()`
- Source: `include/storage/rocksdb_wrapper.h`:514
- Brief: Record a savepoint at the current write position.
- Parameters: none
- Details: Set Save Point. Multiple savepoints may be set; they form a stack (LIFO). Corresponds to RocksDB Transaction::SetSavePoint(). Calls: SetSavePoint().

#### `~TransactionWrapper()`
- Source: `include/storage/rocksdb_wrapper.h`:472
- Brief: n/a
- Parameters: none

### themis::RocksDBWrapper::WriteBatchWithIndexWrapper

#### `WriteBatchWithIndexWrapper(RocksDBWrapper *db, bool overwrite_key=true)`
- Source: `include/storage/rocksdb_wrapper.h`:414
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper *): n/a
  - `overwrite_key` (bool): n/a

#### `bool commit()`
- Source: `include/storage/rocksdb_wrapper.h`:433
- Brief: Commit the batch atomically.
- Parameters: none
- Return: true on success.
- Details: Commit. true on success. True when the operation succeeds. Calls: get(), Write(), ok().

#### `void del(std::string_view key)`
- Source: `include/storage/rocksdb_wrapper.h`:423
- Brief: Delete a key from the batch.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: Del. key Lookup key. key Input parameter. Calls: Delete(), rocksdb::Slice(), data(), size().

#### `std::optional< std::vector< uint8_t > > getFromBatch(std::string_view key) const`
- Source: `include/storage/rocksdb_wrapper.h`:426
- Brief: Get from batch only.
- Parameters:
  - `key` (std::string_view): n/a

#### `std::optional< std::vector< uint8_t > > getFromBatchAndDB(std::string_view key) const`
- Source: `include/storage/rocksdb_wrapper.h`:429
- Brief: Get from batch first, then DB if not found.
- Parameters:
  - `key` (std::string_view): n/a

#### `void put(std::string_view key, const std::vector< uint8_t > &value)`
- Source: `include/storage/rocksdb_wrapper.h`:420
- Brief: Add a key-value pair to the batch.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
- Details: Put. key Lookup key. value Value bytes. key Input parameter. value Input parameter. Calls: Put(), rocksdb::Slice(), data(), size().

#### `void rollback()`
- Source: `include/storage/rocksdb_wrapper.h`:436
- Brief: Roll back the batch.
- Parameters: none
- Details: Rollback. Calls: Clear().

#### `~WriteBatchWithIndexWrapper()`
- Source: `include/storage/rocksdb_wrapper.h`:415
- Brief: n/a
- Parameters: none

### themis::RocksDBWrapper::WriteBatchWrapper

#### `WriteBatchWrapper(RocksDBWrapper *db)`
- Source: `include/storage/rocksdb_wrapper.h`:381
- Brief: n/a
- Parameters:
  - `db` (RocksDBWrapper *): n/a

#### `bool commit()`
- Source: `include/storage/rocksdb_wrapper.h`:394
- Brief: Commit the batch atomically.
- Parameters: none
- Return: true on success.
- Details: Commit. true on success. True when the operation succeeds. Calls: commitBatch(), get().

#### `void del(std::string_view key)`
- Source: `include/storage/rocksdb_wrapper.h`:390
- Brief: Delete a key from the batch.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Details: Del. key Lookup key. key Input parameter. Calls: Delete(), rocksdb::Slice(), data(), size().

#### `void put(std::string_view key, const std::vector< uint8_t > &value)`
- Source: `include/storage/rocksdb_wrapper.h`:387
- Brief: Add a key-value pair to the batch.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
- Details: Put. key Lookup key. value Value bytes. key Input parameter. value Input parameter. Calls: Put(), rocksdb::Slice(), data(), size().

#### `void rollback()`
- Source: `include/storage/rocksdb_wrapper.h`:397
- Brief: Roll back the batch.
- Parameters: none
- Details: Rollback. Calls: Clear().

#### `~WriteBatchWrapper()`
- Source: `include/storage/rocksdb_wrapper.h`:382
- Brief: n/a
- Parameters: none

### themis::ScopedFileDescriptor

#### `ScopedFileDescriptor(ScopedFileDescriptor &&other) noexcept`
- Source: `src/storage/wal_storage.cpp`:188
- Brief: n/a
- Parameters:
  - `other` (ScopedFileDescriptor &&): n/a

#### `ScopedFileDescriptor(const ScopedFileDescriptor &)=delete`
- Source: `src/storage/wal_storage.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScopedFileDescriptor &): n/a

#### `ScopedFileDescriptor(int fd=-1) noexcept`
- Source: `src/storage/wal_storage.cpp`:177
- Brief: n/a
- Parameters:
  - `fd` (int): n/a

#### `int get() const noexcept`
- Source: `src/storage/wal_storage.cpp`:199
- Brief: n/a
- Parameters: none

#### `ScopedFileDescriptor & operator=(ScopedFileDescriptor &&other) noexcept`
- Source: `src/storage/wal_storage.cpp`:191
- Brief: n/a
- Parameters:
  - `other` (ScopedFileDescriptor &&): n/a

#### `ScopedFileDescriptor & operator=(const ScopedFileDescriptor &)=delete`
- Source: `src/storage/wal_storage.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScopedFileDescriptor &): n/a

#### `int release() noexcept`
- Source: `src/storage/wal_storage.cpp`:208
- Brief: n/a
- Parameters: none

#### `void reset(int fd=-1) noexcept`
- Source: `src/storage/wal_storage.cpp`:201
- Brief: n/a
- Parameters:
  - `fd` (int): n/a

#### `~ScopedFileDescriptor()`
- Source: `src/storage/wal_storage.cpp`:179
- Brief: n/a
- Parameters: none

### themis::SetMergeOperator

#### `bool Merge(const rocksdb::Slice &key, const rocksdb::Slice *existing_value, const rocksdb::Slice &value, std::string *new_value, rocksdb::Logger *logger) const override`
- Source: `include/storage/merge_operators.h`:59
- Brief: n/a
- Parameters:
  - `key` (const rocksdb::Slice &): n/a
  - `existing_value` (const rocksdb::Slice *): n/a
  - `value` (const rocksdb::Slice &): n/a
  - `new_value` (std::string *): n/a
  - `logger` (rocksdb::Logger *): n/a

#### `const char * Name() const override`
- Source: `include/storage/merge_operators.h`:65
- Brief: n/a
- Parameters: none

### themis::StorageAuditLogger

#### `StorageAuditLogger(const Config &cfg)`
- Source: `include/storage/storage_audit_logger.h`:132
- Brief: n/a
- Parameters:
  - `cfg` (const Config &): n/a

#### `StorageAuditLogger(const StorageAuditLogger &)=delete`
- Source: `include/storage/storage_audit_logger.h`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StorageAuditLogger &): n/a

#### `std::string currentTimestamp()`
- Source: `include/storage/storage_audit_logger.h`:140
- Brief: n/a
- Parameters: none

#### `std::string_view eventName(Event e)`
- Source: `include/storage/storage_audit_logger.h`:129
- Brief: Event Name.
- Parameters:
  - `e` (Event): Input parameter.
- Return: Return value.
- Details: Convert an Event enum to its string token (e.g. Event::PUT → "PUT"). e Input parameter. Return value. Implements eventName without additional internal calls.

#### `Result< void > flush()`
- Source: `include/storage/storage_audit_logger.h`:121
- Brief: Flush.
- Parameters: none
- Return: Return value.
- Details: Flush buffered writes to the OS. Return value. Calls: lock(), themis_fsync_fd(), ErrVoid(), std::string(), std::strerror(), OkVoid().

#### `uint64_t lastSequence() const`
- Source: `include/storage/storage_audit_logger.h`:115
- Brief: n/a
- Parameters: none
- Details: Return the sequence number of the last logged entry (0 if none).

#### `Result< void > log(Event event, std::string_view key, std::string_view extra="")`
- Source: `include/storage/storage_audit_logger.h`:110
- Brief: Log.
- Parameters:
  - `event` (Event): Input parameter.
  - `key` (std::string_view): Input parameter.
  - `extra` (std::string_view): Input parameter.
- Return: Return value.
- Details: Generic log: log any event with a key and extra detail. event Input parameter. key Input parameter. extra Input parameter. Return value. Calls: lock(), writeEntry().

#### `Result< void > logCheckpoint(std::string_view detail="")`
- Source: `include/storage/storage_audit_logger.h`:98
- Brief: Log Checkpoint.
- Parameters:
  - `detail` (std::string_view): Input parameter.
- Return: Return value.
- Details: Log a CHECKPOINT event. detail Input parameter. Return value. Calls: log().

#### `Result< void > logCompaction(std::string_view detail="")`
- Source: `include/storage/storage_audit_logger.h`:104
- Brief: Log Compaction.
- Parameters:
  - `detail` (std::string_view): Input parameter.
- Return: Return value.
- Details: Log a COMPACTION event. detail Input parameter. Return value. Calls: log().

#### `Result< void > logDel(std::string_view key, std::string_view extra="")`
- Source: `include/storage/storage_audit_logger.h`:95
- Brief: Log Del.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `extra` (std::string_view): Input parameter.
- Return: Return value.
- Details: Log a DEL event. key Input parameter. extra Input parameter. Return value. Calls: log().

#### `Result< void > logPut(std::string_view key, std::string_view extra="")`
- Source: `include/storage/storage_audit_logger.h`:92
- Brief: Log Put.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `extra` (std::string_view): Input parameter.
- Return: Return value.
- Details: Log a PUT event. extra is an optional detail string (e.g. "bytes=128"). key Input parameter. extra Input parameter. Return value. Calls: log().

#### `Result< void > logRecovery(std::string_view detail="")`
- Source: `include/storage/storage_audit_logger.h`:101
- Brief: Log Recovery.
- Parameters:
  - `detail` (std::string_view): Input parameter.
- Return: Return value.
- Details: Log a RECOVERY event. detail Input parameter. Return value. Calls: log().

#### `Result< void > logSnapshot(std::string_view detail="")`
- Source: `include/storage/storage_audit_logger.h`:107
- Brief: Log Snapshot.
- Parameters:
  - `detail` (std::string_view): Input parameter.
- Return: Return value.
- Details: Log a SNAPSHOT event. detail Input parameter. Return value. Calls: log().

#### `Result< std::unique_ptr< StorageAuditLogger > > open(const Config &config)`
- Source: `include/storage/storage_audit_logger.h`:81
- Brief: Open (or create) an audit log in config.dir.
- Parameters:
  - `config` (const Config &): n/a
- Return: Result<unique_ptr<StorageAuditLogger>> on success, Error on failure.
- Details: Existing segment files are preserved; new entries are appended to a new segment so the previous segments remain immutable. Result<unique_ptr<StorageAuditLogger>> on success, Error on failure.

#### `Result< void > openNewSegment()`
- Source: `include/storage/storage_audit_logger.h`:136
- Brief: n/a
- Parameters: none

#### `Result< void > openOrCreate()`
- Source: `include/storage/storage_audit_logger.h`:134
- Brief: Open Or Create.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: seg_re(), fs::directory_iterator(), path(), filename(), string(), std::regex_match(), push_back(), std::stoull().

#### `StorageAuditLogger & operator=(const StorageAuditLogger &)=delete`
- Source: `include/storage/storage_audit_logger.h`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StorageAuditLogger &): n/a

#### `Result< void > rotateIfNeeded()`
- Source: `include/storage/storage_audit_logger.h`:135
- Brief: Rotate If Needed.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: OkVoid(), themis_fsync_fd(), themis_close_fd(), back(), push_back(), fs::path(), segmentName(), string().

#### `size_t segmentCount() const`
- Source: `include/storage/storage_audit_logger.h`:118
- Brief: n/a
- Parameters: none
- Details: Return the number of log segment files currently on disk.

#### `std::string segmentName(uint64_t segment_id)`
- Source: `include/storage/storage_audit_logger.h`:126
- Brief: n/a
- Parameters:
  - `segment_id` (uint64_t): n/a
- Details: Build the segment file name for a given segment ID.

#### `void syncIfRequired()`
- Source: `include/storage/storage_audit_logger.h`:138
- Brief: Sync If Required.
- Parameters: none
- Details: Calls: themis_fsync_fd().

#### `Result< void > writeEntry(Event event, std::string_view key, std::string_view extra)`
- Source: `include/storage/storage_audit_logger.h`:137
- Brief: Write Entry.
- Parameters:
  - `event` (Event): Input parameter.
  - `key` (std::string_view): Input parameter.
  - `extra` (std::string_view): Input parameter.
- Return: Return value.
- Details: event Input parameter. key Input parameter. extra Input parameter. Return value. Calls: rotateIfNeeded(), has_value(), currentTimestamp(), std::setw(), std::setfill(), eventName(), empty(), str().

#### `~StorageAuditLogger()`
- Source: `include/storage/storage_audit_logger.h`:83
- Brief: n/a
- Parameters: none

### themis::StorageEngine

#### `StorageEngine(IExpressionEvaluatorPtr evaluator, IFieldEncryptionPtr encryption, IKeyProviderPtr key_provider, IIndexManagerPtr index_manager=nullptr)`
- Source: `include/storage/storage_engine.h`:138
- Brief: Constructor with Dependency Injection.
- Parameters:
  - `evaluator` (IExpressionEvaluatorPtr): Expression evaluator for WHERE clauses
  - `encryption` (IFieldEncryptionPtr): Field-level encryption provider
  - `key_provider` (IKeyProviderPtr): Key management provider
  - `index_manager` (IIndexManagerPtr): Index management provider (optional)
- Throws:
  - std::invalid_argument: if required dependencies are null
- Details: evaluator Expression evaluator for WHERE clauses encryption Field-level encryption provider key_provider Key management provider index_manager Index management provider (optional) std::invalid_argument if required dependencies are null

#### `StorageEngine(StorageEngine &&other) noexcept=delete`
- Source: `include/storage/storage_engine.h`:149
- Brief: n/a
- Parameters:
  - `other` (StorageEngine &&): n/a

#### `StorageEngine(const StorageEngine &)=delete`
- Source: `include/storage/storage_engine.h`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StorageEngine &): n/a

#### `bool apply_filter(const std::string &filter_expr, const void *context)`
- Source: `include/storage/storage_engine.h`:266
- Brief: Apply a filter expression to stored data.
- Parameters:
  - `filter_expr` (const std::string &): Input parameter.
  - `context` (const void *): Input parameter.
- Return: true if filter matches, false otherwise
- Details: Apply filter. Uses the injected expression evaluator to filter documents. filter_expr The filter expression string context Context for evaluation (e.g., current document) true if filter matches, false otherwise filter_expr Input parameter. context Input parameter. True when the operation succeeds. Calls: evaluate().

#### `void close() override`
- Source: `include/storage/storage_engine.h`:164
- Brief: Close.
- Parameters: none
- Details: Calls: reset().

#### `std::shared_ptr< StorageEngine > createDefault()`
- Source: `include/storage/storage_engine.h`:160
- Brief: Static factory method for backward compatibility.
- Parameters: none
- Return: Shared pointer to StorageEngine with default dependencies
- Details: Create Default. Creates a StorageEngine with default implementations of all dependencies. Use this for simple scenarios or when migrating existing code. Shared pointer to StorageEngine with default dependencies Return value. Calls: createDefaultEvaluator(), createDefaultEncryption(), createDefaultKeyProvider(), createDefaultIndexManager().

#### `IFieldEncryptionPtr createDefaultEncryption()`
- Source: `include/storage/storage_engine.h`:315
- Brief: Create the default field-encryption implementation.
- Parameters: none
- Return: Shared pointer to the default encryption provider.
- Details: Create Default Encryption. Shared pointer to the default encryption provider. Return value. Implements createDefaultEncryption without additional internal calls.

#### `IExpressionEvaluatorPtr createDefaultEvaluator()`
- Source: `include/storage/storage_engine.h`:310
- Brief: Create default implementations (for testing and builder).
- Parameters: none
- Return: Shared pointer to the default evaluator.
- Details: Factory methods for default implementations. These factory methods create default implementations of interfaces. Used by createDefault() factory and StorageEngineBuilder::standard() Create the default expression evaluator implementation. Shared pointer to the default evaluator. Return value. Implements createDefaultEvaluator without additional internal calls.

#### `IIndexManagerPtr createDefaultIndexManager()`
- Source: `include/storage/storage_engine.h`:325
- Brief: Create the default index-manager implementation.
- Parameters: none
- Return: Shared pointer to the default index manager.
- Details: Create Default Index Manager. Shared pointer to the default index manager. Return value. Implements createDefaultIndexManager without additional internal calls.

#### `IKeyProviderPtr createDefaultKeyProvider()`
- Source: `include/storage/storage_engine.h`:320
- Brief: Create the default key-provider implementation.
- Parameters: none
- Return: Shared pointer to the default key provider.
- Details: Create Default Key Provider. Shared pointer to the default key provider. Return value. Implements createDefaultKeyProvider without additional internal calls.

#### `std::vector< uint8_t > decrypt_field(const std::string &field_name, const std::vector< uint8_t > &ciphertext)`
- Source: `include/storage/storage_engine.h`:296
- Brief: Decrypt a field after retrieving.
- Parameters:
  - `field_name` (const std::string &): Name of the field.
  - `ciphertext` (const std::vector< uint8_t > &): Input parameter.
- Return: Decrypted plaintext (moved to caller, no copy)
- Details: Decrypt field. Uses the injected field encryption provider. Move Semantics: Returned vector uses move semantics to avoid unnecessary copying of decrypted data (CWE-457 remediation). field_name Name of the field to decrypt ciphertext Encrypted data Decrypted plaintext (moved to caller, no copy) field_name Name of the field. ciphertext Input parameter. Return value. Implements decrypt_field without additional internal calls.

#### `Result< void > del(const std::string &key) override`
- Source: `include/storage/storage_engine.h`:167
- Brief: Del.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. Calls: span(), setAttribute(), size(), setStatus(), fetch_add(), ErrVoid(), std::chrono::steady_clock::now(), count().

#### `std::vector< uint8_t > encrypt_field(const std::string &field_name, const std::vector< uint8_t > &plaintext)`
- Source: `include/storage/storage_engine.h`:280
- Brief: Encrypt a field before storing.
- Parameters:
  - `field_name` (const std::string &): Name of the field.
  - `plaintext` (const std::vector< uint8_t > &): Input parameter.
- Return: Encrypted data (moved to caller, no copy)
- Details: Encrypt field. Uses the injected field encryption provider. Move Semantics: Returned vector uses move semantics to avoid unnecessary copying of encrypted data (CWE-457 remediation). field_name Name of the field to encrypt plaintext Plaintext data Encrypted data (moved to caller, no copy) field_name Name of the field. plaintext Input parameter. Return value. Implements encrypt_field without additional internal calls.

#### `Result< std::string > get(const std::string &key) override`
- Source: `include/storage/storage_engine.h`:166
- Brief: Get.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. Calls: span(), setAttribute(), size(), setStatus(), fetch_add(), std::chrono::steady_clock::now(), count(), atomicUpdateMin().

#### `IOMetrics ioMetrics() const`
- Source: `include/storage/storage_engine.h`:250
- Brief: Return a snapshot of cumulative I/O metrics.
- Parameters: none
- Return: Snapshot of the cumulative storage I/O metrics.
- Details: Thread-safe: fields are read with relaxed atomics (consistent per-field, not a cross-field snapshot). Move Semantics: Returned IOMetrics struct uses move semantics to enable Return Value Optimization (RVO) and avoid unnecessary copies (CWE-457 remediation). Snapshot of the cumulative storage I/O metrics.

#### `Result< void > open(const std::string &db_path) override`
- Source: `include/storage/storage_engine.h`:163
- Brief: Open.
- Parameters:
  - `db_path` (const std::string &): Path to the db.
- Return: Return value.
- Details: db_path Path to the db. Return value. Calls: ErrVoid(), reset(), OkVoid().

#### `StorageEngine & operator=(StorageEngine &&other) noexcept=delete`
- Source: `include/storage/storage_engine.h`:150
- Brief: n/a
- Parameters:
  - `other` (StorageEngine &&): n/a

#### `StorageEngine & operator=(const StorageEngine &)=delete`
- Source: `include/storage/storage_engine.h`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StorageEngine &): n/a

#### `Result< void > put(const std::string &key, const std::string &value) override`
- Source: `include/storage/storage_engine.h`:165
- Brief: Put.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. value Input parameter. Return value. Calls: span(), setAttribute(), size(), setStatus(), fetch_add(), ErrVoid(), std::chrono::steady_clock::now(), count().

#### `RocksDBWrapper * rawDB()`
- Source: `include/storage/storage_engine.h`:331
- Brief: Expose the mutable underlying RocksDB wrapper for advanced operations.
- Parameters: none
- Return: Non-owning pointer to the live RocksDB wrapper, or nullptr if unopened.
- Details: Non-owning pointer to the live RocksDB wrapper, or nullptr if unopened.

#### `const RocksDBWrapper * rawDB() const`
- Source: `include/storage/storage_engine.h`:336
- Brief: Expose the underlying RocksDB wrapper for read-only advanced operations.
- Parameters: none
- Return: Non-owning pointer to the live RocksDB wrapper, or nullptr if unopened.
- Details: Non-owning pointer to the live RocksDB wrapper, or nullptr if unopened.

#### `void resetIOMetrics()`
- Source: `include/storage/storage_engine.h`:255
- Brief: Reset all I/O metrics to zero / initial state.
- Parameters: none
- Details: Reset IOMetrics. Calls: store().

#### `void resetScanCounters()`
- Source: `include/storage/storage_engine.h`:237
- Brief: Reset all scan performance counters to zero.
- Parameters: none
- Details: Reset Scan Counters. Calls: store().

#### `ScanCounters scanCounters() const`
- Source: `include/storage/storage_engine.h`:232
- Brief: Return a copy of the current scan performance counters.
- Parameters: none
- Return: Snapshot of the cumulative scan counters.
- Details: Thread-safe: reads are sequentially consistent. Move Semantics: Returned ScanCounters struct uses move semantics to enable Return Value Optimization (RVO) and avoid unnecessary copies (CWE-457 remediation). Snapshot of the cumulative scan counters.

#### `Result< void > scanPredicate(std::string_view start_key, std::string_view end_key, std::function< bool(std::string_view key, std::string_view value)> predicate, std::function< bool(std::string_view key, std::string_view value)> callback)`
- Source: `include/storage/storage_engine.h`:215
- Brief: Scan a key range with an inline predicate filter.
- Parameters:
  - `start_key` (std::string_view): Inclusive lower bound (empty = beginning).
  - `end_key` (std::string_view): Exclusive upper bound (empty = end).
  - `predicate` (std::function< bool(std::string_view key, std::string_view value)>): Returns true if the entry should be delivered.
  - `callback` (std::function< bool(std::string_view key, std::string_view value)>): Called only for entries that pass the predicate. Return false to stop iteration.
- Return: Result<void> – ok on success, error on failure.
- Details: Like scanRange() but only delivers key-value pairs for which predicate returns true. The predicate is evaluated for every key visited; if it returns false the entry is counted as "examined but not returned" (keys_examined++ only). Updates ScanCounters atomically. start_key Inclusive lower bound (empty = beginning). end_key Exclusive upper bound (empty = end). predicate Returns true if the entry should be delivered. callback Called only for entries that pass the predicate. Return false to stop iteration. Result<void> – ok on success, error on failure.

#### `Result< void > scanPrefix(std::string_view prefix, std::function< bool(std::string_view key, std::string_view value)> callback) override`
- Source: `include/storage/storage_engine.h`:193
- Brief: Scan all keys with a given prefix.
- Parameters:
  - `prefix` (std::string_view): n/a
  - `callback` (std::function< bool(std::string_view key, std::string_view value)>): n/a
- Return: Result<void> – ok on success, error on failure.
- Details: Updates ScanCounters atomically. Result<void> – ok on success, error on failure.

#### `Result< void > scanRange(std::string_view start_key, std::string_view end_key, std::function< bool(std::string_view key, std::string_view value)> callback) override`
- Source: `include/storage/storage_engine.h`:180
- Brief: Scan a key range [start_key, end_key) in sorted order.
- Parameters:
  - `start_key` (std::string_view): n/a
  - `end_key` (std::string_view): n/a
  - `callback` (std::function< bool(std::string_view key, std::string_view value)>): n/a
- Return: Result<void> – ok on success, error on failure.
- Details: Iterates all keys ≥ start_key and < end_key (pass empty strings for open-ended bounds) and calls callback for each key-value pair. Returning false from the callback stops iteration early. Updates ScanCounters atomically. Result<void> – ok on success, error on failure.

### themis::StorageEngine::IOMetrics

#### `double avg_del_latency_us() const`
- Source: `include/storage/storage_engine.h`:123
- Brief: Return the average successful delete latency.
- Parameters: none
- Return: Average delete latency in microseconds, or 0.0 if no deletes were recorded.
- Details: Average delete latency in microseconds, or 0.0 if no deletes were recorded.

#### `double avg_get_latency_us() const`
- Source: `include/storage/storage_engine.h`:115
- Brief: Return the average successful get latency.
- Parameters: none
- Return: Average get latency in microseconds, or 0.0 if no gets were recorded.
- Details: Average get latency in microseconds, or 0.0 if no gets were recorded.

#### `double avg_put_latency_us() const`
- Source: `include/storage/storage_engine.h`:107
- Brief: Return the average successful put latency.
- Parameters: none
- Return: Average put latency in microseconds, or 0.0 if no puts were recorded.
- Details: Average put latency in microseconds, or 0.0 if no puts were recorded.

### themis::StorageEngine::ScanCounters

#### `double selectivity() const`
- Source: `include/storage/storage_engine.h`:62
- Brief: Return the ratio of returned keys to examined keys.
- Parameters: none
- Return: Filter selectivity in the range [0.0, 1.0], or 1.0 when no keys were examined.
- Details: Filter selectivity in the range [0.0, 1.0], or 1.0 when no keys were examined.

### themis::StreamingIngestManager

#### `StreamingIngestManager(const StreamingIngestManager &)=delete`
- Source: `include/storage/streaming_ingest_manager.h`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StreamingIngestManager &): n/a

#### `StreamingIngestManager(std::shared_ptr< RocksDBWrapper > db, Config cfg)`
- Source: `include/storage/streaming_ingest_manager.h`:184
- Brief: n/a
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): n/a
  - `cfg` (Config): n/a

#### `std::unique_ptr< StreamingIngestManager > create(std::shared_ptr< RocksDBWrapper > db, Config cfg={})`
- Source: `include/storage/streaming_ingest_manager.h`:123
- Brief: Create a StreamingIngestManager backed by db.
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): Input parameter.
  - `cfg` (Config): Input parameter.
- Return: A stopped manager; call start() before ingesting events.
- Details: Create. db RocksDB wrapper (must outlive the manager). cfg Tuning parameters. A stopped manager; call start() before ingesting events. Ownership model: the private constructor is only reachable via this factory. The caller receives sole ownership via std::unique_ptr. The internal new StreamingIngestManager(...) is immediately transferred to the returned unique_ptr — this is the correct idiom when std::make_unique cannot be used due to a private constructor, resolving the smart_ptr_misuse scanner gap at line 64. db Input parameter. cfg Input parameter. Return value.

#### `Result< void > flush()`
- Source: `include/storage/streaming_ingest_manager.h`:171
- Brief: Immediately flush all buffered events to RocksDB.
- Parameters: none
- Return: Return value.
- Details: Flush. Blocks until the flush is complete. Useful for testing and graceful shutdown scenarios. Return value. Calls: lock(), empty(), THEMIS_DEBUG(), flushOnce().

#### `void flushLoop()`
- Source: `include/storage/streaming_ingest_manager.h`:186
- Brief: Flush Loop.
- Parameters: none
- Details: Calls: load(), lock(), wait_for(), empty(), flushOnce().

#### `Result< void > flushOnce(std::unique_lock< std::mutex > &lock)`
- Source: `include/storage/streaming_ingest_manager.h`:187
- Brief: ------------------------------------------------------------------------ Internal flush (called with mu_ held; may unlock/re-lock) ------------------------------------------------------------------------
- Parameters:
  - `lock` (std::unique_lock< std::mutex > &): Input/output parameter.
- Return: Return value.
- Details: lock Input/output parameter. Return value. Calls: empty(), swap(), reserve(), std::min(), notify_all(), unlock(), size(), Put().

#### `Result< void > ingest(std::string_view key, std::string_view value)`
- Source: `include/storage/streaming_ingest_manager.h`:152
- Brief: Enqueue a single event.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
- Return: Success, or ERR_STORAGE_LOG_FULL if the buffer is full and the backpressure timeout expired.
- Details: Ingest. When Config::overflow_policy == BLOCK, this method blocks until buffer space is available (subject to backpressure_timeout). When Config::overflow_policy == DROP, returns success immediately even if the event was discarded. Success, or ERR_STORAGE_LOG_FULL if the buffer is full and the backpressure timeout expired. key Input parameter. value Input parameter. Return value.

#### `Result< size_t > ingestBatch(std::vector< Event > events)`
- Source: `include/storage/streaming_ingest_manager.h`:163
- Brief: Enqueue a batch of pre-built events.
- Parameters:
  - `events` (std::vector< Event >): Input parameter.
- Return: Number of events successfully enqueued. If back-pressure triggers, the returned count may be less than events.size().
- Details: Ingest Batch. More efficient than calling ingest() in a loop because the mutex is taken only once. Events are appended in order. Number of events successfully enqueued. If back-pressure triggers, the returned count may be less than events.size(). events Input parameter. Return value. Calls: empty(), THEMIS_DEBUG(), lock(), size(), fetch_add(), THEMIS_WARN(), count(), std::chrono::steady_clock::now().

#### `StreamingIngestManager & operator=(const StreamingIngestManager &)=delete`
- Source: `include/storage/streaming_ingest_manager.h`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StreamingIngestManager &): n/a

#### `Result< void > start()`
- Source: `include/storage/streaming_ingest_manager.h`:128
- Brief: Start.
- Parameters: none
- Return: Return value.
- Details: Start the background flush thread. Return value. Calls: compare_exchange_strong(), tl::unexpected(), Error(), std::thread(), flushLoop().

#### `Stats stats() const noexcept`
- Source: `include/storage/streaming_ingest_manager.h`:176
- Brief: n/a
- Parameters: none
- Details: Return a snapshot of current statistics.

#### `Result< void > stop()`
- Source: `include/storage/streaming_ingest_manager.h`:136
- Brief: Stop the background flush thread and flush all pending events.
- Parameters: none
- Return: Return value.
- Details: Stop. Blocks until the queue is drained. After stop() the manager may not be restarted. Return value. Calls: load(), store(), notify_all(), joinable(), utils::joinThreadWithin(), THEMIS_WARN(), lock(), empty().

#### `~StreamingIngestManager()`
- Source: `include/storage/streaming_ingest_manager.h`:178
- Brief: n/a
- Parameters: none

### themis::StreamingIngestManager::Config

#### `Config()`
- Source: `include/storage/streaming_ingest_manager.h`:97
- Brief: n/a
- Parameters: none

### themis::TierThresholds

#### `TierThresholds cold() noexcept`
- Source: `include/storage/index_analyzer.h`:77
- Brief: Defaults suitable for a cold-tier (object storage / archive) index.
- Parameters: none

#### `TierThresholds hot() noexcept`
- Source: `include/storage/index_analyzer.h`:69
- Brief: Defaults suitable for a hot-tier (NVMe) index.
- Parameters: none

#### `TierThresholds warm() noexcept`
- Source: `include/storage/index_analyzer.h`:73
- Brief: Defaults suitable for a warm-tier (SATA SSD) index.
- Parameters: none

### themis::WALStorage

#### `WALStorage(const Config &cfg)`
- Source: `include/storage/wal_storage.h`:204
- Brief: n/a
- Parameters:
  - `cfg` (const Config &): n/a

#### `WALStorage(const WALStorage &)=delete`
- Source: `include/storage/wal_storage.h`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WALStorage &): n/a

#### `Result< uint64_t > appendBatch(std::vector< BatchEntry > entries)`
- Source: `include/storage/wal_storage.h`:169
- Brief: Group-commit: write multiple entries with a single fsync.
- Parameters:
  - `entries` (std::vector< BatchEntry >): Input parameter.
- Return: The sequence number of the last entry in the batch, or an error.
- Details: Append Batch. All entries in entries are serialised and written in sequence, then the WAL file is fsynced exactly once (when Config::fsync_on_write is true). This amortises fsync overhead across many writes, allowing sustained throughput in the 100k+ ops/s range while retaining ACID durability. If the batch spans a segment boundary, a rotation is performed between segments and each segment is individually fsynced before rotation. entries Non-empty span of entries to write atomically. The sequence number of the last entry in the batch, or an error. entries Input parameter. Return value. Calls: empty(), lock(), rotateIfNeeded(), THEMIS_WARN(), error(), context(), code(), appendEntryLocked().

#### `Result< uint64_t > appendDelete(std::string_view key)`
- Source: `include/storage/wal_storage.h`:153
- Brief: Append a DELETE entry.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: The sequence number assigned to this entry.
- Details: Append Delete. The sequence number assigned to this entry. key Input parameter. Return value. Calls: appendEntry().

#### `Result< uint64_t > appendEntry(EntryType type, std::string_view key, std::string_view value)`
- Source: `include/storage/wal_storage.h`:210
- Brief: Append Entry.
- Parameters:
  - `type` (EntryType): Input parameter.
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
- Return: Return value.
- Details: type Input parameter. key Input parameter. value Input parameter. Return value. Calls: lock(), rotateIfNeeded(), error(), code(), context(), appendEntryLocked(), syncIfRequired().

#### `Result< uint64_t > appendEntryLocked(EntryType type, std::string_view key, std::string_view value)`
- Source: `include/storage/wal_storage.h`:215
- Brief: Write one entry to fd_ WITHOUT locking or syncing.
- Parameters:
  - `type` (EntryType): Input parameter.
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
- Return: Return value.
- Details: type Input parameter. key Input parameter. value Input parameter. Return value. Caller holds mutex_. Calls: size(), encode_u32(), encode_u64(), crc32_update(), data(), write_all_fd(), std::to_string(), Ok().

#### `Result< uint64_t > appendPut(std::string_view key, std::string_view value)`
- Source: `include/storage/wal_storage.h`:146
- Brief: Append a PUT entry.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
- Return: The sequence number assigned to this entry.
- Details: Append Put. The sequence number assigned to this entry. key Input parameter. value Input parameter. Return value. Calls: appendEntry().

#### `Result< uint64_t > checkpoint(bool delete_old_segments=true)`
- Source: `include/storage/wal_storage.h`:182
- Brief: Write a CHECKPOINT entry and optionally delete old segments.
- Parameters:
  - `delete_old_segments` (bool): Input parameter.
- Return: The sequence number of the checkpoint entry.
- Details: Checkpoint. After a successful checkpoint the primary store is guaranteed to contain all mutations up to the returned sequence number. Old WAL segments whose highest sequence number is ≤ the checkpoint sequence may be safely removed. delete_old_segments If true, remove fully-checkpointed segments. The sequence number of the checkpoint entry. delete_old_segments Input parameter. Return value. Calls: lock(), rotateIfNeeded(), error(), code(), context(), appendEntryLocked(), syncIfRequired(), push_back().

#### `Result< void > flush()`
- Source: `include/storage/wal_storage.h`:193
- Brief: Flush.
- Parameters: none
- Return: Return value.
- Details: Flush buffered I/O to the OS (but not necessarily to disk). Return value. Calls: lock(), OkVoid(), std::chrono::steady_clock::now(), std::chrono::seconds(), themis_fsync_fd(), THEMIS_WARN(), count(), ErrVoid().

#### `uint64_t lastSequence() const`
- Source: `include/storage/wal_storage.h`:187
- Brief: n/a
- Parameters: none
- Details: Return the sequence number of the last written entry.

#### `Result< std::unique_ptr< WALStorage > > open(const Config &config, RecoveryCallback on_recover=nullptr)`
- Source: `include/storage/wal_storage.h`:128
- Brief: Open (or create) the WAL in the given directory.
- Parameters:
  - `config` (const Config &): Input parameter.
  - `on_recover` (RecoveryCallback): Input parameter.
- Return: Result<std::unique_ptr<WALStorage>> on success, Error on failure.
- Details: Factory method: Strong Exception Guarantee (W5-Storage Hardening) Exception Safety: - If openOrCreate() throws or returns an error, the partial WALStorage object is destroyed (unique_ptr cleanup) before returning Err<>. On first open, an empty WAL is created. On subsequent opens, existing segments are replayed via on_recover. config WAL configuration (directory, rotation threshold, fsync). on_recover Callback invoked for each recovered entry; may be null. Result<std::unique_ptr<WALStorage>> on success, Error on failure. config Input parameter. on_recover Input parameter. Return value. Caller receives a Result<> that indicates success/failure. - No exception escapes open(); all errors are captured in Result. Thread Safety: - Factory method (static); no concurrent access during construction. no_timeout scanner alert: WALStorage::open is a local-file factory method; it opens a WAL directory on block storage — no network I/O, no timeout needed. Calls: WALStorage(), openOrCreate(), error(), code(), context(), Ok(), std::move().

#### `Result< void > openNewSegment(uint64_t segment_id)`
- Source: `include/storage/wal_storage.h`:209
- Brief: Close-Path Locking and Exception Safety (W5-Storage Hardening) openNewSegment() transitions file descriptor and segment state atomically under mutex_: 1.
- Parameters:
  - `segment_id` (uint64_t): Identifier of the segment.
- Return: Return value.
- Details: segment_id Identifier of the segment. Return value. Acquire mutex_ (all caller paths hold mutex_ via appendEntry/appendEntryLocked). 2. If fd_ >= 0, fsync old segment (may timeout, but continue anyway). 3. Close old fd_; set fd_ = -1. 4. Open new segment file with O_CLOEXEC (prevents FD leak in fork). 5. On error, return Err<>; caller must retry or abort. 6. On success, update current_segment_, segment_bytes_, fd_. Exception Safety: Strong Guarantee - ScopedFileDescriptor (next_fd) ensures new fd is closed if exception occurs before fd_ = next_fd.release(). - Old segment is closed before opening new one (no orphaned fds). - State transition is atomic under mutex_; no partial updates visible to readers. - If any step fails, fd_ remains valid (either old or -1), ready for retry. Thread Safety: Race Prevention - Caller must hold mutex_ (enforced by callers appendEntry, rotateIfNeeded). - Concurrent writers cannot access fd_/segment state during transition. - fd_ ≥ 0 check is stable under mutex_ (no concurrent close). Calls: std::chrono::steady_clock::now(), std::chrono::seconds(), themis_fsync_fd(), THEMIS_WARN(), count(), ErrVoid(), std::string(), std::strerror().

#### `Result< void > openOrCreate(RecoveryCallback &on_recover)`
- Source: `include/storage/wal_storage.h`:206
- Brief: Open Or Create.
- Parameters:
  - `on_recover` (RecoveryCallback &): Input/output parameter.
- Return: Return value.
- Details: on_recover Input/output parameter. Return value. Calls: fs::create_directories(), ErrVoid(), message(), clear(), fs::directory_iterator(), is_regular_file(), parseSegmentId(), path().

#### `WALStorage & operator=(const WALStorage &)=delete`
- Source: `include/storage/wal_storage.h`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WALStorage &): n/a

#### `uint64_t parseSegmentId(const std::string &filename)`
- Source: `include/storage/wal_storage.h`:201
- Brief: Parse Segment Id.
- Parameters:
  - `filename` (const std::string &): Input parameter.
- Return: Return value.
- Details: Parse the segment ID from a WAL segment file name (returns 0 on error). filename Input parameter. Return value. Calls: re(), std::regex_match(), std::stoull().

#### `Result< void > replaySegment(const std::string &path, RecoveryCallback &cb)`
- Source: `include/storage/wal_storage.h`:207
- Brief: Replay Segment.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `cb` (RecoveryCallback &): Input/output parameter.
- Return: Return value.
- Details: path Input parameter. cb Input/output parameter. Return value. Calls: f(), is_open(), ErrVoid(), std::chrono::steady_clock::now(), std::chrono::minutes(), good(), THEMIS_WARN(), count().

#### `Result< void > rotateIfNeeded()`
- Source: `include/storage/wal_storage.h`:208
- Brief: Rotate If Needed.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: OkVoid(), openNewSegment().

#### `size_t segmentCount() const`
- Source: `include/storage/wal_storage.h`:190
- Brief: n/a
- Parameters: none
- Details: Return the number of WAL segment files currently on disk.

#### `std::string segmentName(uint64_t segment_id)`
- Source: `include/storage/wal_storage.h`:198
- Brief: Segment Name.
- Parameters:
  - `segment_id` (uint64_t): Identifier of the segment.
- Return: Return value.
- Details: Build the file name for a WAL segment. segment_id Identifier of the segment. Return value. Calls: std::setw(), std::setfill(), str().

#### `Result< void > syncIfRequired()`
- Source: `include/storage/wal_storage.h`:218
- Brief: Sync If Required.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: OkVoid(), std::chrono::steady_clock::now(), std::chrono::seconds(), themis_fsync_fd(), THEMIS_WARN(), count(), ErrVoid(), std::string().

#### `~WALStorage()`
- Source: `include/storage/wal_storage.h`:133
- Brief: n/a
- Parameters: none

### themis::WomTree

#### `WomTree()`
- Source: `include/storage/wom_tree.h`:201
- Brief: Construct a WOM Tree with default configuration.
- Parameters: none

#### `WomTree(WomTree &&) noexcept`
- Source: `include/storage/wom_tree.h`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (WomTree &&): n/a

#### `WomTree(const Config &config)`
- Source: `include/storage/wom_tree.h`:209
- Brief: Construct a WOM Tree with the supplied configuration.
- Parameters:
  - `config` (const Config &): n/a
- Throws:
  - std::invalid_argument: if the configuration is invalid (e.g. fanout < 2, leaf_capacity < 2, buffer_size_bytes == 0).
- Details: std::invalid_argument if the configuration is invalid (e.g. fanout < 2, leaf_capacity < 2, buffer_size_bytes == 0).

#### `WomTree(const WomTree &)=delete`
- Source: `include/storage/wom_tree.h`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WomTree &): n/a

#### `void clear()`
- Source: `include/storage/wom_tree.h`:349
- Brief: Remove all entries from the tree, resetting all statistics.
- Parameters: none
- Details: Clear. Calls: lk(), store().

#### `Result< void > compact()`
- Source: `include/storage/wom_tree.h`:318
- Brief: Force a full flush of all node buffers down to the leaves.
- Parameters: none
- Return: OkVoid() on success.
- Details: ── compact / flushOnce ────────────────────────────────────────────────────── After this call every live mutation resides in a leaf node and all tombstones have been purged. This is the analogue of a full compaction in an LSM tree, but incurs significantly less I/O because each mutation has already been partially propagated. OkVoid() on success. Return value. Calls: lk(), flushAll(), OkVoid().

#### `const Config & config() const noexcept`
- Source: `include/storage/wom_tree.h`:363
- Brief: Return the active configuration.
- Parameters: none

#### `bool contains(std::string_view key) const`
- Source: `include/storage/wom_tree.h`:273
- Brief: Check whether a key is present.
- Parameters:
  - `key` (std::string_view): Key to test.
- Return: true if the key has a live value.
- Details: key Key to test. true if the key has a live value.

#### `bool empty() const noexcept`
- Source: `include/storage/wom_tree.h`:344
- Brief: Return true if the tree contains no live entries.
- Parameters: none

#### `Result< void > flushOnce()`
- Source: `include/storage/wom_tree.h`:329
- Brief: Flush buffered mutations from the root down one level.
- Parameters: none
- Return: OkVoid() on success.
- Details: Flush Once. This is a lightweight, incremental version of compact() suitable for background maintenance. Call periodically to keep buffer occupancy low without blocking writers for long. OkVoid() on success. Return value. Calls: lk(), empty(), flushNode(), OkVoid().

#### `Result< std::string > get(std::string_view key) const`
- Source: `include/storage/wom_tree.h`:265
- Brief: Look up the value for a key.
- Parameters:
  - `key` (std::string_view): Key to look up.
- Return: Ok(value) if found; Err(..., NOT_FOUND) if absent or tombstoned.
- Details: The search traverses all in-memory node buffers from root to leaf, applying any pending mutations in arrival order before returning the effective value. This is intentionally slower than in an LSM tree (where bloom filters can short-circuit lookups) — a documented WOM-tree trade-off. key Key to look up. Ok(value) if found; Err(..., NOT_FOUND) if absent or tombstoned.

#### `WomTree & operator=(WomTree &&) noexcept`
- Source: `include/storage/wom_tree.h`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (WomTree &&): n/a

#### `WomTree & operator=(const WomTree &)=delete`
- Source: `include/storage/wom_tree.h`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WomTree &): n/a

#### `Result< void > put(std::string_view key, std::string_view value)`
- Source: `include/storage/wom_tree.h`:234
- Brief: Insert or update a key-value pair.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
- Return: OkVoid() on success.
- Details: ── put ────────────────────────────────────────────────────────────────────── The mutation is appended to the root's write buffer. If the buffer exceeds config.buffer_size_bytes the affected subtree is flushed synchronously before returning. key Non-empty key. value Value to associate with the key. OkVoid() on success. key Input parameter. value Input parameter. Return value. Calls: empty(), ErrVoid(), lk(), std::string(), fetch_add(), doInsertOp(), std::move(), OkVoid().

#### `Result< void > remove(std::string_view key)`
- Source: `include/storage/wom_tree.h`:246
- Brief: Remove a key from the tree.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: OkVoid() on success; Err(..., NOT_FOUND) if the key does not exist in the tree.
- Details: ── remove ─────────────────────────────────────────────────────────────────── Depending on config.lazy_deletes, the key is either tombstoned immediately (deferred purge) or removed synchronously from the leaf. key Key to remove. OkVoid() on success; Err(..., NOT_FOUND) if the key does not exist in the tree. key Input parameter. Return value. Calls: empty(), ErrVoid(), lk(), doGet(), has_value(), std::string(), fetch_add(), directRemove().

#### `void scan(const std::function< bool(std::string_view key, std::string_view value)> &callback) const`
- Source: `include/storage/wom_tree.h`:288
- Brief: Scan all live key-value pairs in ascending key order.
- Parameters:
  - `callback` (const std::function< bool(std::string_view key, std::string_view value)> &): Function called with (key, value) for each entry. Return true to continue, false to stop.
- Details: The callback is invoked for every live entry. Returning false from the callback stops the scan early. callback Function called with (key, value) for each entry. Return true to continue, false to stop.

#### `void scanRange(std::string_view start_key, std::string_view end_key, const std::function< bool(std::string_view key, std::string_view value)> &callback) const`
- Source: `include/storage/wom_tree.h`:299
- Brief: Scan a half-open key range [start_key, end_key) in ascending order.
- Parameters:
  - `start_key` (std::string_view): Inclusive lower bound (empty string = beginning).
  - `end_key` (std::string_view): Exclusive upper bound (empty string = end).
  - `callback` (const std::function< bool(std::string_view key, std::string_view value)> &): Called with (key, value). Return false to stop.
- Details: start_key Inclusive lower bound (empty string = beginning). end_key Exclusive upper bound (empty string = end). callback Called with (key, value). Return false to stop.

#### `size_t size() const noexcept`
- Source: `include/storage/wom_tree.h`:339
- Brief: Return the number of live entries.
- Parameters: none
- Details: The count is exact in single-threaded usage. Under concurrent writes it may be momentarily stale by at most the number of in-flight writers, but it is always consistent with what get()/contains() observe once the same exclusive lock is acquired.

#### `Stats stats() const`
- Source: `include/storage/wom_tree.h`:358
- Brief: Return a consistent snapshot of current statistics.
- Parameters: none

#### `~WomTree()`
- Source: `include/storage/wom_tree.h`:211
- Brief: n/a
- Parameters: none

### themis::WomTree::Impl

#### `Impl(const Config &cfg)`
- Source: `src/storage/wom_tree.cpp`:259
- Brief: n/a
- Parameters:
  - `cfg` (const Config &): n/a

#### `void applyOpToLeaf(Node &leaf, const Op &op)`
- Source: `src/storage/wom_tree.cpp`:546
- Brief: Apply a single Op to a leaf and record the write in stat_internal_bytes.
- Parameters:
  - `leaf` (Node &): Input/output parameter.
  - `op` (const Op &): Input parameter.
- Details: leaf Input/output parameter. op Input parameter. stat_live_entries is NOT updated here — that responsibility belongs to: • doInsertOp (single-leaf fast path: before/after check) • doInsertOp (buffered path: doGet-based check before enqueueing) • directRemove (unconditional decrement, existence pre-verified) This separation avoids double-counting when buffered ops are flushed by flushNode. Calls: fetch_add(), byteSize(), leafApply().

#### `void clearBufferedOpsForKey(const std::string &key, Node &node)`
- Source: `src/storage/wom_tree.cpp`:559
- Brief: Remove all buffered ops for 'key' along the path from node down to the child subtree that contains 'key'.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `node` (Node &): Input/output parameter.
- Details: key Input parameter. node Input/output parameter. This prevents a previously-buffered PUT from reappearing after the next flush when lazy_deletes=false. Calls: erase(), std::remove_if(), begin(), end(), byteSize(), childIndex().

#### `void collectAllEntries(std::map< std::string, std::string > &out) const`
- Source: `src/storage/wom_tree.cpp`:666
- Brief: n/a
- Parameters:
  - `out` (std::map< std::string, std::string > &): n/a

#### `void collectNode(const Node &node, std::map< std::string, std::string > &out) const`
- Source: `src/storage/wom_tree.cpp`:670
- Brief: n/a
- Parameters:
  - `node` (const Node &): n/a
  - `out` (std::map< std::string, std::string > &): n/a

#### `size_t countBufferedEntries(const Node &node) const`
- Source: `src/storage/wom_tree.cpp`:699
- Brief: n/a
- Parameters:
  - `node` (const Node &): n/a

#### `size_t countInternals(const Node &node) const`
- Source: `src/storage/wom_tree.cpp`:721
- Brief: n/a
- Parameters:
  - `node` (const Node &): n/a

#### `size_t countLeaves(const Node &node) const`
- Source: `src/storage/wom_tree.cpp`:710
- Brief: n/a
- Parameters:
  - `node` (const Node &): n/a

#### `void directRemove(const std::string &key)`
- Source: `src/storage/wom_tree.cpp`:587
- Brief: Immediately remove 'key' from the tree without buffering a tombstone.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Details: key Input parameter. All pending buffered ops for 'key' in internal nodes are purged first, then the entry is physically erased from its leaf. Precondition: caller has verified the key exists (doGet succeeded). Calls: clearBufferedOpsForKey(), get(), childIndex(), applyOpToLeaf(), fetch_sub().

#### `std::optional< std::string > doGet(std::string_view key) const`
- Source: `src/storage/wom_tree.cpp`:614
- Brief: n/a
- Parameters:
  - `key` (std::string_view): n/a

#### `void doInsertOp(Op op)`
- Source: `src/storage/wom_tree.cpp`:269
- Brief: ── Write path ───────────────────────────────────────────────────────
- Parameters:
  - `op` (Op): Input parameter.
- Details: op Input parameter. Calls: fetch_add(), byteSize(), leafFind(), end(), applyOpToLeaf(), fetch_sub(), maybeSplitRootLeaf(), doGet().

#### `bool doOneInternalSplit(NodePtr &node_ref, Node *parent, size_t idx_in_parent)`
- Source: `src/storage/wom_tree.cpp`:487
- Brief: Perform one internal-node split, depth-first.
- Parameters:
  - `node_ref` (NodePtr &): Input/output parameter.
  - `parent` (Node *): Input/output parameter.
  - `idx_in_parent` (size_t): Input parameter.
- Return: True when the operation succeeds.
- Details: node_ref Input/output parameter. parent Input/output parameter. idx_in_parent Input parameter. True when the operation succeeds. Returns true if any split was performed (the caller should then call again until stable). Calls: size(), get(), splitInternal(), push_back(), std::move(), insert(), begin().

#### `void fixAllInternalOverflows()`
- Source: `src/storage/wom_tree.cpp`:534
- Brief: Keep splitting overfull internal nodes until the tree satisfies the fanout constraint at every level.
- Parameters: none
- Details: Calls: doOneInternalSplit().

#### `void flushAll(Node &node, uint32_t depth)`
- Source: `src/storage/wom_tree.cpp`:414
- Brief: Flush all buffers in the subtree rooted at 'node' to leaves.
- Parameters:
  - `node` (Node &): Input/output parameter.
  - `depth` (uint32_t): Input parameter.
- Details: node Input/output parameter. depth Input parameter. Calls: flushNode().

#### `void flushNode(Node &node, uint32_t depth)`
- Source: `src/storage/wom_tree.cpp`:333
- Brief: Recursively flush node's buffer one level downward.
- Parameters:
  - `node` (Node &): Input/output parameter.
  - `depth` (uint32_t): Input parameter.
- Details: node Input/output parameter. depth Input parameter. depth == depth of 'node' in the tree (root = 1). Calls: empty(), size(), child_ops(), childIndex(), push_back(), std::move(), clear(), fetch_add().

#### `void maybeSplitChild(Node &parent, size_t ci)`
- Source: `src/storage/wom_tree.cpp`:456
- Brief: If child[ci] is a leaf and has more than leaf_capacity entries, split it and insert the new pivot into 'parent'.
- Parameters:
  - `parent` (Node &): Input/output parameter.
  - `ci` (size_t): Input parameter.
- Details: parent Input/output parameter. ci Input parameter. Calls: size(), splitLeaf(), insert(), begin(), std::move().

#### `void maybeSplitRootLeaf()`
- Source: `src/storage/wom_tree.cpp`:430
- Brief: If the single-leaf root is over capacity, promote it to an internal node with two leaf children.
- Parameters: none
- Details: Calls: size(), splitLeaf(), push_back(), std::move().

#### `uint32_t treeHeight(const Node &node) const`
- Source: `src/storage/wom_tree.cpp`:732
- Brief: n/a
- Parameters:
  - `node` (const Node &): n/a

### themis::WomTree::Stats

#### `double readHitRatio() const noexcept`
- Source: `include/storage/wom_tree.h`:185
- Brief: Point-read hit ratio.
- Parameters: none
- Details: Returns fraction of get() calls that found a value. Values below 1.0 include misses for absent keys; this is expected in write-heavy workloads.

#### `double writeAmplification() const noexcept`
- Source: `include/storage/wom_tree.h`:170
- Brief: Estimated write-amplification factor.
- Parameters: none
- Details: Ratio of all bytes physically written to tree nodes (direct leaf writes + buffer propagation hops) to bytes submitted by the user. A value of 1.0 means each user byte was written exactly once (ideal, single-leaf tree); typical multi-level WOM trees target 2–5× compared to 10–30× for typical LSM trees. Returns 0.0 if no user bytes have been written yet.

### themis::bench::sgrg

#### `void BM_SGRG01_WalAppendThroughput(benchmark::State &state)`
- Source: `benchmarks/storage/bench_storage_release_gates.cpp`:212
- Brief: SGRG-01: Append to in-memory ring-buffer WAL.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Gate: ≥ 100 k ops/s.

#### `void BM_SGRG02_MvccReadWarm(benchmark::State &state)`
- Source: `benchmarks/storage/bench_storage_release_gates.cpp`:241
- Brief: SGRG-02: Hash-map read on warm MVCC store (10 k entries pre-loaded).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Gate: p99 ≤ 100 µs.

#### `void BM_SGRG03_MvccWrite(benchmark::State &state)`
- Source: `benchmarks/storage/bench_storage_release_gates.cpp`:267
- Brief: SGRG-03: WAL append + in-memory index update (write-before-ack).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Gate: p99 ≤ 500 µs.

#### `void BM_SGRG04_CheckpointOverhead(benchmark::State &state)`
- Source: `benchmarks/storage/bench_storage_release_gates.cpp`:296
- Brief: SGRG-04: Checkpoint scan of 1 000 entries (no real I/O).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Gate: p99 ≤ 10 ms.

#### `void BM_SGRG05_TieringDecision(benchmark::State &state)`
- Source: `benchmarks/storage/bench_storage_release_gates.cpp`:322
- Brief: SGRG-05: Hot/warm/cold classification based on access-count heuristic.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Gate: p99 ≤ 50 µs.

#### `void BM_SGRG06_CompactionTriggerCheck(benchmark::State &state)`
- Source: `benchmarks/storage/bench_storage_release_gates.cpp`:350
- Brief: SGRG-06: shouldCompact() heuristic evaluation (pure computation).
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Gate: p99 ≤ 100 µs.

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true)`
- Source: `benchmarks/storage/bench_storage_release_gates.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

#### `UseRealTime() -> Repetitions(kRepetitions) ->ReportAggregatesOnly(true)`
- Source: `benchmarks/storage/bench_storage_release_gates.cpp`:228
- Brief: n/a
- Parameters: none

#### `BenchMvccStore & sharedStore()`
- Source: `benchmarks/storage/bench_storage_release_gates.cpp`:198
- Brief: n/a
- Parameters: none

#### `BenchWal & sharedWal()`
- Source: `benchmarks/storage/bench_storage_release_gates.cpp`:193
- Brief: n/a
- Parameters: none

### themis::bench::st_dg

#### `void BM_ST_BM_01_KVWrite(benchmark::State &state)`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:153
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ST_BM_02_KVRead(benchmark::State &state)`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:172
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ST_BM_03_WALReplayThroughput(benchmark::State &state)`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:192
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ST_BM_04_CompactionTriggerCheck(benchmark::State &state)`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:217
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true) ->Threads(1)`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

#### `std::vector< std::string > make_keys()`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:123
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > make_values()`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:132
- Brief: n/a
- Parameters: none

### themis::bench::st_dg::BenchCompactionChecker

#### `BenchCompactionChecker(int threshold)`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:105
- Brief: n/a
- Parameters:
  - `threshold` (int): n/a

#### `bool should_compact(int pending_files) const`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:107
- Brief: n/a
- Parameters:
  - `pending_files` (int): n/a

### themis::bench::st_dg::BenchKVIndex

#### `bool get(const std::string &key, std::string *out) const`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:87
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `out` (std::string *): n/a

#### `void put(const std::string &key, const std::string &value)`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:82
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

### themis::bench::st_dg::BenchWAL

#### `BenchWAL(std::size_t cap=1<< 20)`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:56
- Brief: n/a
- Parameters:
  - `cap` (std::size_t): n/a

#### `bool append(const std::string &key, const std::string &value)`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:58
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

#### `std::size_t replay() const`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:66
- Brief: n/a
- Parameters: none

### themis::bench::st_dg::IndexSeeder

#### `IndexSeeder()`
- Source: `benchmarks/storage/bench_storage_dedicated_gates.cpp`:142
- Brief: n/a
- Parameters: none

### themis::compression

#### `bool is_known_tag(uint8_t tag) noexcept`
- Source: `include/storage/codec_tags.h`:47
- Brief: n/a
- Parameters:
  - `tag` (uint8_t): n/a
- Details: Returns true if tag is a recognised codec tag in this version of ThemisDB. Unknown tags should be treated as a framing error.

#### `uint8_t method_to_tag(CompressionMethod m) noexcept`
- Source: `include/storage/compression_strategy.h`:318
- Brief: Map a CompressionMethod to its canonical wire-format tag byte.
- Parameters:
  - `m` (CompressionMethod): Method to convert.
- Return: One of the kTag* constants from storage/codec_tags.h.
- Details: GPU variants are mapped to the same tag as their CPU counterpart because the on-wire format is identical. Methods without a tag-byte representation (RLE, DELTA, DICTIONARY, SPARSE_CSR, ADAPTIVE) fall back to kTagPassthrough so that payloads are always decodable. m Method to convert. One of the kTag* constants from storage/codec_tags.h.

#### `std::optional< CompressionMethod > tag_to_method(uint8_t tag) noexcept`
- Source: `include/storage/compression_strategy.h`:345
- Brief: Map a canonical wire-format tag byte back to a CompressionMethod.
- Parameters:
  - `tag` (uint8_t): Leading tag byte from a framed payload.
- Return: The matching method, or std::nullopt for unknown tags.
- Details: tag Leading tag byte from a framed payload. The matching method, or std::nullopt for unknown tags.

### themis::compression::CompressionResult

#### `CompressionResult()`
- Source: `include/storage/compression_strategy.h`:91
- Brief: n/a
- Parameters: none

### themis::compression::CompressionStrategyManager

#### `CompressionStrategyManager(const CompressionConfig &config=CompressionConfig{})`
- Source: `include/storage/compression_strategy.h`:107
- Brief: n/a
- Parameters:
  - `config` (const CompressionConfig &): n/a

#### `CompressionResult compress(const std::string &data, std::optional< DataType > hint=std::nullopt)`
- Source: `include/storage/compression_strategy.h`:131
- Brief: Compress string data (convenience overload).
- Parameters:
  - `data` (const std::string &): n/a
  - `hint` (std::optional< DataType >): n/a

#### `CompressionResult compress(const std::vector< uint8_t > &data, std::optional< DataType > hint=std::nullopt)`
- Source: `include/storage/compression_strategy.h`:121
- Brief: Compress vector data (convenience overload).
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a
  - `hint` (std::optional< DataType >): n/a

#### `CompressionResult compress(const uint8_t *data, size_t size, std::optional< DataType > hint=std::nullopt)`
- Source: `include/storage/compression_strategy.h`:112
- Brief: Compress data with configured or adaptive strategy.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
  - `hint` (std::optional< DataType >): Input parameter.
- Return: Return value.
- Details: Compress. data Input parameter. size Input parameter. hint Input parameter. Return value. Calls: assign(), value_or(), detect_data_type(), std::min(), select_method(), timer(), method_to_string(), compress_zstd().

#### `CompressionResult compress_delta(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:201
- Brief: Compress delta.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value. Calls: DeltaCodec::compress(), empty(), size(), assign().

#### `CompressionResult compress_dictionary(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:202
- Brief: Compress dictionary.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value. Calls: SimpleDictionaryCodec::compress(), empty(), size(), assign().

#### `CompressionResult compress_gpu_lz4(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:207
- Brief: Compress gpu lz4.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value.

#### `CompressionResult compress_gpu_snappy(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:206
- Brief: Compress gpu snappy.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value.

#### `CompressionResult compress_gpu_zstd(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:205
- Brief: Compress gpu zstd.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value.

#### `CompressionResult compress_rle(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:200
- Brief: Compress rle.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value. Calls: RLECodec::compress(), empty(), size(), assign().

#### `CompressionResult compress_zstd(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:199
- Brief: Compress zstd.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value. Calls: utils::zstd_compress(), empty(), size(), assign().

#### `std::vector< uint8_t > decompress(const std::vector< uint8_t > &data, CompressionMethod method)`
- Source: `include/storage/compression_strategy.h`:149
- Brief: Decompress data.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
  - `method` (CompressionMethod): Input parameter.
- Return: Decompressed data, or empty vector on failure
- Details: Decompress. data Compressed data method Method used for compression Decompressed data, or empty vector on failure data Input parameter. method Input parameter. Return value. Calls: empty(), timer(), method_to_string(), size(), decompress_zstd(), decompress_rle(), decompress_delta(), decompress_dictionary().

#### `std::vector< uint8_t > decompress_delta(const std::vector< uint8_t > &data)`
- Source: `include/storage/compression_strategy.h`:212
- Brief: Decompress delta.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: DeltaCodec::decompress().

#### `std::vector< uint8_t > decompress_dictionary(const std::vector< uint8_t > &data)`
- Source: `include/storage/compression_strategy.h`:213
- Brief: Decompress dictionary.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: SimpleDictionaryCodec::decompress().

#### `std::vector< uint8_t > decompress_gpu_lz4(const std::vector< uint8_t > &data)`
- Source: `include/storage/compression_strategy.h`:218
- Brief: Decompress gpu lz4.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value.

#### `std::vector< uint8_t > decompress_gpu_snappy(const std::vector< uint8_t > &data)`
- Source: `include/storage/compression_strategy.h`:217
- Brief: Decompress gpu snappy.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value.

#### `std::vector< uint8_t > decompress_gpu_zstd(const std::vector< uint8_t > &data)`
- Source: `include/storage/compression_strategy.h`:216
- Brief: Decompress gpu zstd.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value.

#### `std::vector< uint8_t > decompress_rle(const std::vector< uint8_t > &data)`
- Source: `include/storage/compression_strategy.h`:211
- Brief: Decompress rle.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: RLECodec::decompress().

#### `std::vector< uint8_t > decompress_zstd(const std::vector< uint8_t > &data)`
- Source: `include/storage/compression_strategy.h`:210
- Brief: Decompress zstd.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: utils::zstd_decompress().

#### `DataType detect_data_type(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:221
- Brief: Detect data type.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value. Calls: is_mostly_text(), std::min(), size_t(), is_sparse_data().

#### `const CompressionConfig & get_config() const`
- Source: `include/storage/compression_strategy.h`:183
- Brief: Get current configuration.
- Parameters: none

#### `std::string get_metrics() const`
- Source: `include/storage/compression_strategy.h`:166
- Brief: Get compression metrics.
- Parameters: none

#### `themis::storage::GpuCompressionManager & gpu_manager()`
- Source: `include/storage/compression_strategy.h`:226
- Brief: ============================================================================ GPU-Accelerated Compression Method Implementations ============================================================================
- Parameters: none
- Return: Return value.
- Details: Return value. Implements gpu_manager without additional internal calls.

#### `bool is_mostly_text(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:222
- Brief: Is mostly text.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: True when the operation succeeds.
- Details: data Input parameter. size Input parameter. True when the operation succeeds. Calls: std::min(), size_t(), std::isprint(), std::isspace().

#### `bool is_sparse_data(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:223
- Brief: Is sparse data.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: True when the operation succeeds.
- Details: data Input parameter. size Input parameter. True when the operation succeeds. Calls: std::min(), size_t().

#### `std::string method_to_string(CompressionMethod method)`
- Source: `include/storage/compression_strategy.h`:190
- Brief: Convert method enum to string.
- Parameters:
  - `method` (CompressionMethod): Input parameter.
- Return: Return value.
- Details: Method to string. method Input parameter. Return value. Implements method_to_string without additional internal calls.

#### `void reset_metrics()`
- Source: `include/storage/compression_strategy.h`:171
- Brief: Reset metrics.
- Parameters: none
- Details: Calls: utils::CompressionMetrics::instance(), reset().

#### `CompressionMethod select_method(const uint8_t *data, size_t size, DataType type)`
- Source: `include/storage/compression_strategy.h`:157
- Brief: Select optimal compression method for given data.
- Parameters:
  - `data` (const uint8_t *): n/a
  - `size` (size_t): n/a
  - `type` (DataType): Input parameter.
- Return: Return value.
- Details: Select method. param Input parameter. size_t Input parameter. type Input parameter. Return value. Implements select_method without additional internal calls.

#### `void set_config(const CompressionConfig &config)`
- Source: `include/storage/compression_strategy.h`:176
- Brief: Update configuration.
- Parameters:
  - `config` (const CompressionConfig &): n/a

#### `std::optional< CompressionMethod > string_to_method(const std::string &str)`
- Source: `include/storage/compression_strategy.h`:195
- Brief: Convert string to method enum.
- Parameters:
  - `str` (const std::string &): Input parameter.
- Return: Return value.
- Details: String to method. str Input parameter. Return value. Calls: find(), end().

### themis::compression::DeltaCodec

#### `std::vector< uint8_t > compress(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:266
- Brief: Compress using delta encoding.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: Compress. Stores first value, then differences between consecutive values. Efficient for monotonic or slowly-changing sequences. data Input parameter. size Input parameter. Return value. Calls: THEMIS_DEBUG(), reserve(), push_back().

#### `std::vector< uint8_t > decompress(const std::vector< uint8_t > &data)`
- Source: `include/storage/compression_strategy.h`:271
- Brief: Decompress delta-encoded data.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: Decompress. data Input parameter. Return value. Calls: empty(), THEMIS_DEBUG(), reserve(), size(), push_back().

### themis::compression::RLECodec

#### `std::vector< uint8_t > compress(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:243
- Brief: Compress using RLE.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: Compress. Format: [count:varint][value:byte]... Efficient for data with long runs of repeated bytes data Input parameter. size Input parameter. Return value. Calls: THEMIS_DEBUG(), max(), reserve(), encode_varint(), push_back().

#### `uint32_t decode_varint(const uint8_t *&ptr)`
- Source: `include/storage/compression_strategy.h`:252
- Brief: Decode varint.
- Parameters:
  - `ptr` (const uint8_t *&): Input parameter.
- Return: Return value.
- Details: ptr Input parameter. Return value. Implements decode_varint without additional internal calls.

#### `std::vector< uint8_t > decompress(const std::vector< uint8_t > &data)`
- Source: `include/storage/compression_strategy.h`:248
- Brief: Decompress RLE data.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: Decompress. data Input parameter. Return value. Calls: empty(), THEMIS_DEBUG(), size(), max(), reserve(), data(), decode_varint(), insert().

#### `void encode_varint(std::vector< uint8_t > &output, uint32_t value)`
- Source: `include/storage/compression_strategy.h`:251
- Brief: Encode varint.
- Parameters:
  - `output` (std::vector< uint8_t > &): Input/output parameter.
  - `value` (uint32_t): Input parameter.
- Details: output Input/output parameter. value Input parameter. Calls: push_back().

### themis::compression::SimpleDictionaryCodec

#### `std::vector< uint8_t > compress(const uint8_t *data, size_t size)`
- Source: `include/storage/compression_strategy.h`:285
- Brief: Compress using dictionary encoding.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: Compress. Efficient for data with repeated patterns/blocks. Format: [dict_size:4][dict_entries...][indices...] data Input parameter. size Input parameter. Return value. Calls: reserve(), find(), end(), size(), push_back(), THEMIS_DEBUG(), insert(), begin().

#### `std::vector< uint8_t > decompress(const std::vector< uint8_t > &data)`
- Source: `include/storage/compression_strategy.h`:290
- Brief: Decompress dictionary-encoded data.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: Decompress. data Input parameter. Return value. Calls: empty(), size(), THEMIS_WARN(), dictionary(), begin(), reserve(), push_back().

### themis::storage

#### `TEST(Wave3a_BackupManager, CompressPathNoCompression_CompilesToFailClosed)`
- Source: `tests/storage/test_wave3a_critical_fixes.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave3a_BackupManager): n/a
  - `<unnamed>` (CompressPathNoCompression_CompilesToFailClosed): n/a
- Details: When neither zstd nor lz4 is available the no-compression else-branch compiles. This verifies the branch compiles (no-op copy was replaced with fail-closed error).

#### `TEST(Wave3a_BackupManager, EncryptFileNoOpenSSL_CompilesToFailClosed)`
- Source: `tests/storage/test_wave3a_critical_fixes.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave3a_BackupManager): n/a
  - `<unnamed>` (EncryptFileNoOpenSSL_CompilesToFailClosed): n/a
- Details: When OpenSSL is absent the no-OpenSSL else-branch is compiled. This test verifies the behaviour document in that branch: it must return false (fail-closed), not silently copy the file. We test this indirectly by checking that the symbol is visible and that the stub path compiled in returns false. A full runtime check would require a BackupManager instance and temp files; that is covered by integration tests. Here we verify the compile-time guarantee by ensuring this translation unit compiles with THEMIS_ENABLE_OPENSSL absent, which proves the else-branch is compiled (not the OpenSSL path).

#### `TEST(Wave3a_ColumnSegmentDecode, BitPackingInt64RoundTrip)`
- Source: `tests/storage/test_wave3a_critical_fixes.cpp`:98
- Brief: BIT_PACKING round-trip: encode then decode restores original INT64 data.
- Parameters:
  - `<unnamed>` (Wave3a_ColumnSegmentDecode): n/a
  - `<unnamed>` (BitPackingInt64RoundTrip): n/a

#### `TEST(Wave3a_ColumnSegmentDecode, DecodeIdempotent)`
- Source: `tests/storage/test_wave3a_critical_fixes.cpp`:136
- Brief: decode() on an already-decoded segment is idempotent and returns OK.
- Parameters:
  - `<unnamed>` (Wave3a_ColumnSegmentDecode): n/a
  - `<unnamed>` (DecodeIdempotent): n/a

#### `TEST(Wave3a_ColumnSegmentDecode, DictionaryCodecDecodeReturnsError)`
- Source: `tests/storage/test_wave3a_critical_fixes.cpp`:33
- Brief: DICTIONARY codec returns ERR_CODEC_NOT_AVAILABLE from decode(), not silent OK.
- Parameters:
  - `<unnamed>` (Wave3a_ColumnSegmentDecode): n/a
  - `<unnamed>` (DictionaryCodecDecodeReturnsError): n/a

#### `TEST(Wave3a_ColumnSegmentDecode, FrameOfRefInt32RoundTrip)`
- Source: `tests/storage/test_wave3a_critical_fixes.cpp`:117
- Brief: FRAME_OF_REF round-trip.
- Parameters:
  - `<unnamed>` (Wave3a_ColumnSegmentDecode): n/a
  - `<unnamed>` (FrameOfRefInt32RoundTrip): n/a

#### `TEST(Wave3a_ColumnSegmentDecode, RleInt32RoundTrip)`
- Source: `tests/storage/test_wave3a_critical_fixes.cpp`:75
- Brief: RLE round-trip: encode then decode restores original INT32 data.
- Parameters:
  - `<unnamed>` (Wave3a_ColumnSegmentDecode): n/a
  - `<unnamed>` (RleInt32RoundTrip): n/a

#### `std::shared_ptr< spdlog::logger > auditLogger() noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:43
- Brief: Returns the "storage.audit" named spdlog logger, creating a stderr-colour sink on first call if the application has not already registered one. The logger is the structured event channel consumed by Prometheus exporters, Grafana dashboards, and the audit pipeline.
- Parameters: none

#### `StorageErrorContext buildErrorContext(StorageErrorCode code, std::string_view operation_context, std::string_view affected_resource="") noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:316
- Brief: n/a
- Parameters:
  - `code` (StorageErrorCode): n/a
  - `operation_context` (std::string_view): n/a
  - `affected_resource` (std::string_view): n/a

#### `RecoveryFaultReport buildFaultReport(RecoveryFaultType fault_type, StorageErrorCode error_code, std::uint64_t checkpoint_seq, std::uint64_t recovery_start_seq, std::uint64_t fault_seq, std::uint64_t entries_replayed, std::string_view fault_message, std::string_view recovery_suggestion, bool is_retryable, bool should_stop_recovery) noexcept`
- Source: `src/storage/storage_recovery_fault_handler.cpp`:34
- Brief: Helper to construct a RecoveryFaultReport with common fields.
- Parameters:
  - `fault_type` (RecoveryFaultType): n/a
  - `error_code` (StorageErrorCode): n/a
  - `checkpoint_seq` (std::uint64_t): n/a
  - `recovery_start_seq` (std::uint64_t): n/a
  - `fault_seq` (std::uint64_t): n/a
  - `entries_replayed` (std::uint64_t): n/a
  - `fault_message` (std::string_view): n/a
  - `recovery_suggestion` (std::string_view): n/a
  - `is_retryable` (bool): n/a
  - `should_stop_recovery` (bool): n/a

#### `StorageErrorContext classifyErrorIncident(StorageErrorCode code) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:260
- Brief: n/a
- Parameters:
  - `code` (StorageErrorCode): n/a

#### `PressureEscalationLevel classifyPressureLevel(double utilization_percent) noexcept`
- Source: `src/storage/storage_pressure_manager.cpp`:32
- Brief: n/a
- Parameters:
  - `utilization_percent` (double): n/a

#### `std::unique_ptr< GpuCompressionManager > create_gpu_compression_manager(const GpuCompressionConfig &config=GpuCompressionConfig{})`
- Source: `src/storage/gpu_compression.cpp`:1961
- Brief: Create a GpuCompressionManager with the best available backend.
- Parameters:
  - `config` (const GpuCompressionConfig &): Input parameter.
- Return: Return value.
- Details: Create gpu compression manager. Equivalent to constructing with GpuAccelerationType::AUTO. config Input parameter. Return value.

#### `SIMDLevel detectSIMDLevel() noexcept`
- Source: `src/storage/simd_filter.cpp`:55
- Brief: n/a
- Parameters: none
- Details: Detect the best available SIMD level at runtime. Returns NEON on AArch64, SSE4/AVX2/AVX512 on x86 when supported, SCALAR otherwise. Result is memoised after the first call.

#### `void emitDiagnosticEvent(const StorageErrorContext &context) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:374
- Brief: n/a
- Parameters:
  - `context` (const StorageErrorContext &): n/a

#### `void emitRecoveryFaultEvent(std::string_view fault_type, std::uint64_t affected_checkpoint_seq, std::uint64_t recovered_entries, std::string_view suggestion="") noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:399
- Brief: Emit a recovery fault diagnostic event.
- Parameters:
  - `fault_type` (std::string_view): Description of the recovery fault (e.g., "torn_wal_entry", "checkpoint_invalid")
  - `affected_checkpoint_seq` (std::uint64_t): Sequence number of affected checkpoint
  - `recovered_entries` (std::uint64_t): Number of successfully replayed entries before fault
  - `suggestion` (std::string_view): Recovery action for operator
- Details: Specialized event emitter for recovery-phase errors (torn WAL, checkpoint invalid, replay timeout). Includes recovery-specific metadata. fault_type Description of the recovery fault (e.g., "torn_wal_entry", "checkpoint_invalid") affected_checkpoint_seq Sequence number of affected checkpoint recovered_entries Number of successfully replayed entries before fault suggestion Recovery action for operator StorageErrorContext::recovery_suggestion

#### `void emitStoragePressureEvent(std::string_view pressure_type, std::uint64_t available_bytes, std::uint64_t required_bytes, int escalation_level=1) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:415
- Brief: Emit a storage pressure event.
- Parameters:
  - `pressure_type` (std::string_view): Type of pressure (e.g., "disk_full", "backup_limit")
  - `available_bytes` (std::uint64_t): Remaining capacity (0 if unknown)
  - `required_bytes` (std::uint64_t): Capacity requested by operation (0 if unknown)
  - `escalation_level` (int): Current pressure level (1-5; 5 = critical)
- Details: Specialized event emitter for capacity-related errors (storage exhausted, backup limit exceeded, compaction backpressure). pressure_type Type of pressure (e.g., "disk_full", "backup_limit") available_bytes Remaining capacity (0 if unknown) required_bytes Capacity requested by operation (0 if unknown) escalation_level Current pressure level (1-5; 5 = critical)

#### `std::string_view errorCodeDescription(StorageErrorCode code) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:439
- Brief: Get human-readable description of an error code.
- Parameters:
  - `code` (StorageErrorCode): Error code
- Return: Longer description suitable for operator dashboards (e.g., "Write-Ahead Log write failed (I/O error, timeout, or disk full)")
- Details: code Error code Longer description suitable for operator dashboards (e.g., "Write-Ahead Log write failed (I/O error, timeout, or disk full)")

#### `std::string_view errorCodeName(StorageErrorCode code) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:435
- Brief: Get human-readable name for an error code.
- Parameters:
  - `code` (StorageErrorCode): Error code
- Return: Short descriptive name (e.g., "WAL_WRITE_FAILED")
- Details: code Error code Short descriptive name (e.g., "WAL_WRITE_FAILED")

#### `std::mutex & hmacFnMutex()`
- Source: `src/storage/gguf_metadata.cpp`:233
- Brief: Hmac Fn Mutex.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements hmacFnMutex without additional internal calls.

#### `GGUFMetadata::HmacFn & hmacFnStorage()`
- Source: `src/storage/gguf_metadata.cpp`:239
- Brief: Hmac Fn Storage.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements hmacFnStorage without additional internal calls.

#### `std::string_view incidentTypeName(ErrorIncidentType type) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:443
- Brief: Get human-readable name for an incident type.
- Parameters:
  - `type` (ErrorIncidentType): Incident type
- Return: Short descriptive name (e.g., "RECOVERY_FAULT")
- Details: type Incident type Short descriptive name (e.g., "RECOVERY_FAULT")

#### `bool isDurabilityThreat(StorageErrorCode code) noexcept`
- Source: `include/storage/storage_api_contract.h`:252
- Brief: Returns true when code represents a durability-threatening failure.
- Parameters:
  - `code` (StorageErrorCode): n/a
- Details: Durability-threatening codes mandate immediate operator alert and MUST NOT be silently retried: WAL_WRITE_FAILED, WAL_CORRUPTED, STORAGE_EXHAUSTED.

#### `bool isRetryableConflict(StorageErrorCode code) noexcept`
- Source: `include/storage/storage_api_contract.h`:263
- Brief: Returns true when code is a transaction conflict that the caller SHOULD retry with exponential backoff.
- Parameters:
  - `code` (StorageErrorCode): n/a

#### `std::string languageToCode(NlpTextAnalyzer::Language lang)`
- Source: `src/storage/nlp_metadata_extractor.cpp`:31
- Brief: Map analyzer Language enum to short code.
- Parameters:
  - `lang` (NlpTextAnalyzer::Language): Input parameter.
- Return: Return value.
- Details: lang Input parameter. Return value. Implements languageToCode without additional internal calls.

#### `std::string_view lookupErrorDescription(StorageErrorCode code) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:247
- Brief: n/a
- Parameters:
  - `code` (StorageErrorCode): n/a

#### `std::string_view lookupErrorName(StorageErrorCode code) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:238
- Brief: n/a
- Parameters:
  - `code` (StorageErrorCode): n/a

#### `ErrorIncidentType lookupIncidentType(StorageErrorCode code) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:229
- Brief: n/a
- Parameters:
  - `code` (StorageErrorCode): n/a

#### `ErrorSeverity lookupSeverity(StorageErrorCode code) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:220
- Brief: n/a
- Parameters:
  - `code` (StorageErrorCode): n/a

#### `StorageErrorCode mapErrorMessage(std::string_view error_message) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:277
- Brief: n/a
- Parameters:
  - `error_message` (std::string_view): n/a

#### `std::string rawMetaKey(const std::string &key)`
- Source: `src/storage/tensor_network_storage_engine.cpp`:652
- Brief: Raw Meta Key.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. Implements rawMetaKey without additional internal calls.

#### `std::string_view severityName(ErrorSeverity severity) noexcept`
- Source: `src/storage/storage_error_diagnostics.cpp`:452
- Brief: Get human-readable name for a severity level.
- Parameters:
  - `severity` (ErrorSeverity): Severity level
- Return: Short name (e.g., "CRITICAL", "HIGH")
- Details: severity Severity level Short name (e.g., "CRITICAL", "HIGH")

#### `size_t simd_filter_double(const double *data, size_t n, FilterOp op, double threshold, std::vector< uint32_t > &out)`
- Source: `src/storage/simd_filter.cpp`:714
- Brief: Simd filter double.
- Parameters:
  - `data` (const double *): Input parameter.
  - `n` (size_t): Input parameter.
  - `op` (FilterOp): Input parameter.
  - `threshold` (double): Input parameter.
  - `out_indices` (std::vector< uint32_t > &): n/a
- Return: Return value.
- Details: Filter a raw double (float64) data array. Same contract as simd_filter_int32. data Input parameter. n Input parameter. op Input parameter. threshold Input parameter. out Input/output parameter. Return value. Calls: defined(), detectSIMDLevel(), avx2_filter_f64(), neon_filter_f64(), scalar_filter().

#### `size_t simd_filter_float(const float *data, size_t n, FilterOp op, float threshold, std::vector< uint32_t > &out)`
- Source: `src/storage/simd_filter.cpp`:686
- Brief: Simd filter float.
- Parameters:
  - `data` (const float *): Input parameter.
  - `n` (size_t): Input parameter.
  - `op` (FilterOp): Input parameter.
  - `threshold` (float): Input parameter.
  - `out_indices` (std::vector< uint32_t > &): n/a
- Return: Return value.
- Details: Filter a raw float (float32) data array. Same contract as simd_filter_int32. data Input parameter. n Input parameter. op Input parameter. threshold Input parameter. out Input/output parameter. Return value. Calls: defined(), detectSIMDLevel(), avx2_filter_f32(), neon_filter_f32(), scalar_filter().

#### `size_t simd_filter_int32(const int32_t *data, size_t n, FilterOp op, int32_t threshold, std::vector< uint32_t > &out)`
- Source: `src/storage/simd_filter.cpp`:630
- Brief: Simd filter int32.
- Parameters:
  - `data` (const int32_t *): Input parameter.
  - `n` (size_t): Input parameter.
  - `op` (FilterOp): Input parameter.
  - `threshold` (int32_t): Input parameter.
  - `out_indices` (std::vector< uint32_t > &): Output vector receiving matching row indices.
- Return: Number of matching rows appended.
- Details: Filter a raw int32 data array. Writes the indices of matching rows into out_indices (appended). Returns the number of matching rows written. Uses AVX2 (8-way) or scalar path depending on detectSIMDLevel(). Graceful fallback: scalar path is always correct. data Pointer to contiguous int32 values. n Number of elements. op Comparison operator. threshold Value to compare against. out_indices Output vector receiving matching row indices. Number of matching rows appended. data Input parameter. n Input parameter. op Input parameter. threshold Input parameter. out Input/output parameter. Return value. Calls: defined(), detectSIMDLevel(), avx2_filter_i32(), neon_filter_i32(), scalar_filter().

#### `size_t simd_filter_int64(const int64_t *data, size_t n, FilterOp op, int64_t threshold, std::vector< uint32_t > &out)`
- Source: `src/storage/simd_filter.cpp`:658
- Brief: Simd filter int64.
- Parameters:
  - `data` (const int64_t *): Input parameter.
  - `n` (size_t): Input parameter.
  - `op` (FilterOp): Input parameter.
  - `threshold` (int64_t): Input parameter.
  - `out_indices` (std::vector< uint32_t > &): n/a
- Return: Return value.
- Details: Filter a raw int64 data array. Same contract as simd_filter_int32. data Input parameter. n Input parameter. op Input parameter. threshold Input parameter. out Input/output parameter. Return value. Calls: defined(), detectSIMDLevel(), avx2_filter_i64(), neon_filter_i64(), scalar_filter().

#### `int themis_zc_open_read_only(const char *)`
- Source: `src/storage/zero_copy_blob_transfer.cpp`:199
- Brief: Themis zc open read only.
- Parameters:
  - `<unnamed>` (const char *): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Implements themis_zc_open_read_only without additional internal calls.

#### `themis_zc_ssize_t themis_zc_write_fd(int, const void *, size_t)`
- Source: `src/storage/zero_copy_blob_transfer.cpp`:210
- Brief: Themis zc write fd.
- Parameters:
  - `<unnamed>` (int): n/a
  - `<unnamed>` (const void *): n/a
  - `<unnamed>` (size_t): n/a
- Return: Return value.
- Details: int Input parameter. param Input parameter. size_t Input parameter. Return value. Implements themis_zc_write_fd without additional internal calls.

#### `std::string to_string(TensorRouteDecision d) noexcept`
- Source: `src/storage/tensor_router.cpp`:92
- Brief: n/a
- Parameters:
  - `d` (TensorRouteDecision): n/a

#### `Result< int64_t > zc_file_size_as_int64(const std::string &source_path, const char *operation)`
- Source: `src/storage/zero_copy_blob_transfer.cpp`:249
- Brief: Zc file size as int64.
- Parameters:
  - `source_path` (const std::string &): Path to the source.
  - `operation` (const char *): Input parameter.
- Return: Return value.
- Details: source_path Path to the source. operation Input parameter. Return value. Calls: fs::file_size(), std::string(), max(), Ok().

#### `bool zc_write_all(int fd, const void *data, size_t len)`
- Source: `src/storage/zero_copy_blob_transfer.cpp`:223
- Brief: Zc write all.
- Parameters:
  - `fd` (int): Input parameter.
  - `data` (const void *): Input parameter.
  - `len` (size_t): Input parameter.
- Return: True when the operation succeeds.
- Details: fd Input parameter. data Input parameter. len Input parameter. True when the operation succeeds. Calls: themis_zc_write_fd().

### themis::storage::AccessTracker

#### `void recordRead(const std::string &key)`
- Source: `include/storage/tiered_storage.h`:116
- Brief: Record a read access.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Details: Record Read. key Input parameter. Calls: lock(), find(), end(), std::chrono::system_clock::now().

#### `void recordWrite(const std::string &key, StorageTierLevel tier=StorageTierLevel::HOT, uint64_t value_size=0)`
- Source: `include/storage/tiered_storage.h`:111
- Brief: Record a write (creates or resets the entry).
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `tier` (StorageTierLevel): Input parameter.
  - `value_size` (uint64_t): Input parameter.
- Details: Record Write. key Input parameter. tier Input parameter. value_size Input parameter. Calls: lock(), std::chrono::system_clock::now().

#### `void remove(const std::string &key)`
- Source: `include/storage/tiered_storage.h`:122
- Brief: Remove an entry (called on key deletion).
- Parameters:
  - `key` (const std::string &): Input parameter.
- Details: Remove. key Input parameter. Calls: lock(), erase().

#### `void setTier(const std::string &key, StorageTierLevel tier)`
- Source: `include/storage/tiered_storage.h`:119
- Brief: Update the tier label (called after a successful migration).
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `tier` (StorageTierLevel): Input parameter.
- Details: Set Tier. key Input parameter. tier Input parameter. Calls: lock(), find(), end().

#### `std::size_t size() const`
- Source: `include/storage/tiered_storage.h`:128
- Brief: Return the number of tracked keys.
- Parameters: none

#### `std::unordered_map< std::string, Entry > snapshot() const`
- Source: `include/storage/tiered_storage.h`:125
- Brief: Return a snapshot of all tracked entries.
- Parameters: none

### themis::storage::AzureBlobBackend

#### `AzureBlobBackend(const std::string &connection_string, const std::string &container_name, const std::string &prefix="")`
- Source: `include/storage/blob_backend_azure.h`:42
- Brief: Construct an Azure Blob Storage backend.
- Parameters:
  - `connection_string` (const std::string &): Azure Storage connection string (e.g. from portal or environment variable AZURE_STORAGE_CONNECTION_STRING).
  - `container_name` (const std::string &): Name of the target container.
  - `prefix` (const std::string &): Optional object-name prefix (e.g. "blobs/").
- Details: connection_string Azure Storage connection string (e.g. from portal or environment variable AZURE_STORAGE_CONNECTION_STRING). container_name Name of the target container. prefix Optional object-name prefix (e.g. "blobs/").

#### `std::string computeSHA256(const std::vector< uint8_t > &data)`
- Source: `include/storage/blob_backend_azure.h`:103
- Brief: Compute the SHA-256 digest for a blob payload.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Raw blob bytes.
- Return: Hex-encoded SHA-256 digest.
- Details: data Raw blob bytes. Hex-encoded SHA-256 digest.

#### `bool exists(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_azure.h`:78
- Brief: Check whether a blob exists in the container.
- Parameters:
  - `ref` (const BlobRef &): Blob reference to check.
- Return: true if the blob exists.
- Details: ref Blob reference to check. true if the blob exists.

#### `Result< std::vector< uint8_t > > get(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_azure.h`:64
- Brief: Retrieve a blob from Azure Blob Storage.
- Parameters:
  - `ref` (const BlobRef &): Blob reference previously returned by put().
- Return: Blob data on success, or an error if the blob does not exist or the download fails.
- Details: ref Blob reference previously returned by put(). Blob data on success, or an error if the blob does not exist or the download fails.

#### `std::string getBlobName(const std::string &blob_id) const`
- Source: `include/storage/blob_backend_azure.h`:110
- Brief: Derive the storage object name for a blob identifier.
- Parameters:
  - `blob_id` (const std::string &): Logical blob identifier.
- Return: Fully qualified blob name including any configured prefix.
- Details: blob_id Logical blob identifier. Fully qualified blob name including any configured prefix.

#### `bool isAvailable() const override`
- Source: `include/storage/blob_backend_azure.h`:91
- Brief: Check whether the Azure backend is operational.
- Parameters: none
- Details: Returns false when the Azure SDK was not compiled in, when the connection string is invalid, or when the container cannot be reached.

#### `std::string name() const override`
- Source: `include/storage/blob_backend_azure.h`:83
- Brief: Return the backend name ("azure").
- Parameters: none

#### `Result< BlobRef > put(const std::string &blob_id, const std::vector< uint8_t > &data) override`
- Source: `include/storage/blob_backend_azure.h`:55
- Brief: Store a blob in Azure Blob Storage.
- Parameters:
  - `blob_id` (const std::string &): Unique blob identifier (used as blob name).
  - `data` (const std::vector< uint8_t > &): Raw blob data.
- Return: BlobRef on success, or an error if the SDK is unavailable or the upload fails.
- Details: blob_id Unique blob identifier (used as blob name). data Raw blob data. BlobRef on success, or an error if the SDK is unavailable or the upload fails.

#### `Result< void > remove(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_azure.h`:71
- Brief: Delete a blob from Azure Blob Storage.
- Parameters:
  - `ref` (const BlobRef &): Blob reference previously returned by put().
- Return: void on success, or an error if deletion fails.
- Details: ref Blob reference previously returned by put(). void on success, or an error if deletion fails.

#### `~AzureBlobBackend() override`
- Source: `include/storage/blob_backend_azure.h`:46
- Brief: n/a
- Parameters: none

### themis::storage::BitPackingCodec

#### `uint8_t calculateBitsRequired(int64_t min_val, int64_t max_val)`
- Source: `include/storage/columnar_format.h`:148
- Brief: Calculate Bits Required.
- Parameters:
  - `min_val` (int64_t): Input parameter.
  - `max_val` (int64_t): Input parameter.
- Return: Return value.
- Details: min_val Input parameter. max_val Input parameter. Return value. Calls: std::swap().

#### `Result< std::vector< int32_t > > decodeInt32(const std::vector< uint8_t > &encoded)`
- Source: `include/storage/columnar_format.h`:143
- Brief: Decode Int32.
- Parameters:
  - `encoded` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: encoded Input parameter. Return value. Calls: size(), tl::unexpected(), Error(), std::memcpy(), reserve(), push_back().

#### `Result< std::vector< int64_t > > decodeInt64(const std::vector< uint8_t > &encoded)`
- Source: `include/storage/columnar_format.h`:144
- Brief: Decode Int64.
- Parameters:
  - `encoded` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: encoded Input parameter. Return value. Calls: size(), tl::unexpected(), Error(), std::memcpy(), reserve(), push_back().

#### `Result< std::vector< uint8_t > > encodeInt32(const std::vector< int32_t > &data)`
- Source: `include/storage/columnar_format.h`:139
- Brief: Encode Int32.
- Parameters:
  - `data` (const std::vector< int32_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: empty(), std::min_element(), begin(), end(), std::max_element(), calculateBitsRequired(), insert(), push_back().

#### `Result< std::vector< uint8_t > > encodeInt64(const std::vector< int64_t > &data)`
- Source: `include/storage/columnar_format.h`:140
- Brief: Encode Int64.
- Parameters:
  - `data` (const std::vector< int64_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: empty(), std::min_element(), begin(), end(), std::max_element(), calculateBitsRequired(), insert(), push_back().

### themis::storage::BlobStorageManager

#### `BlobStorageManager(const BlobStorageConfig &config)`
- Source: `include/storage/blob_storage_manager.h`:71
- Brief: n/a
- Parameters:
  - `config` (const BlobStorageConfig &): n/a

#### `bool exists(const BlobRef &ref)`
- Source: `include/storage/blob_storage_manager.h`:180
- Brief: Check if blob exists.
- Parameters:
  - `ref` (const BlobRef &): Blob reference
- Return: true if exists
- Details: ref Blob reference true if exists

#### `std::optional< std::vector< uint8_t > > get(const BlobRef &ref)`
- Source: `include/storage/blob_storage_manager.h`:126
- Brief: Retrieve a blob.
- Parameters:
  - `ref` (const BlobRef &): Blob reference
- Return: Blob data or nullopt if not found
- Details: ref Blob reference Blob data or nullopt if not found

#### `const BlobStorageConfig & getConfig() const`
- Source: `include/storage/blob_storage_manager.h`:200
- Brief: Get configuration.
- Parameters: none

#### `std::vector< BlobStorageType > getRegisteredBackends() const`
- Source: `include/storage/blob_storage_manager.h`:207
- Brief: Get registered backend types.
- Parameters: none

#### `BlobRef put(const std::string &blob_id, const std::vector< uint8_t > &data)`
- Source: `include/storage/blob_storage_manager.h`:91
- Brief: Store a blob with automatic backend selection.
- Parameters:
  - `blob_id` (const std::string &): Unique blob identifier
  - `data` (const std::vector< uint8_t > &): Blob data
- Return: BlobRef Reference to stored blob
- Throws:
  - std::runtime_error: if no suitable backend available
- Details: blob_id Unique blob identifier data Blob data BlobRef Reference to stored blob std::runtime_error if no suitable backend available

#### `void registerBackend(BlobStorageType type, std::shared_ptr< IBlobStorageBackend > backend)`
- Source: `include/storage/blob_storage_manager.h`:79
- Brief: Register a blob storage backend.
- Parameters:
  - `type` (BlobStorageType): Backend type
  - `backend` (std::shared_ptr< IBlobStorageBackend >): Backend implementation
- Details: type Backend type backend Backend implementation

#### `bool remove(const BlobRef &ref)`
- Source: `include/storage/blob_storage_manager.h`:154
- Brief: Delete a blob.
- Parameters:
  - `ref` (const BlobRef &): Blob reference
- Return: true if deleted
- Details: ref Blob reference true if deleted

#### `BlobStorageType selectBackendType(size_t blob_size) const`
- Source: `include/storage/blob_storage_manager.h`:40
- Brief: n/a
- Parameters:
  - `blob_size` (size_t): n/a

### themis::storage::ColumnCompressedStorage

#### `ColumnCompressedStorage(std::shared_ptr< CompressedStorageWrapper::IStorageBackend > backend)`
- Source: `include/storage/compressed_storage.h`:177
- Brief: n/a
- Parameters:
  - `backend` (std::shared_ptr< CompressedStorageWrapper::IStorageBackend >): n/a

#### `void configure_column(const std::string &column, const compression::CompressionConfig &config)`
- Source: `include/storage/compressed_storage.h`:185
- Brief: Configure compression for a specific column.
- Parameters:
  - `column` (const std::string &): Input parameter.
  - `config` (const compression::CompressionConfig &): Input parameter.
- Details: Configure column. column Column name/namespace config Compression configuration for this column column Input parameter. config Input parameter. Calls: lock().

#### `bool del(const std::string &column, const std::string &key)`
- Source: `include/storage/compressed_storage.h`:220
- Brief: Delete key from column.
- Parameters:
  - `column` (const std::string &): Input parameter.
  - `key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: Del. column Input parameter. key Input parameter. True when the operation succeeds. Calls: make_full_key().

#### `std::optional< std::vector< uint8_t > > get(const std::string &column, const std::string &key)`
- Source: `include/storage/compressed_storage.h`:212
- Brief: Retrieve value from a specific column.
- Parameters:
  - `column` (const std::string &): Input parameter.
  - `key` (const std::string &): Input parameter.
- Return: Decompressed value or nullopt
- Details: Get. column Column name/namespace key Key within the column Decompressed value or nullopt column Input parameter. key Input parameter. Return value. Calls: make_full_key(), CompressedValue::deserialize(), lock(), find(), end(), decompress(), empty().

#### `std::string get_all_column_stats() const`
- Source: `include/storage/compressed_storage.h`:225
- Brief: Get compression statistics for all columns.
- Parameters: none

#### `std::string get_column_stats(const std::string &column) const`
- Source: `include/storage/compressed_storage.h`:230
- Brief: Get compression statistics for specific column.
- Parameters:
  - `column` (const std::string &): n/a

#### `std::string make_full_key(const std::string &column, const std::string &key) const`
- Source: `include/storage/compressed_storage.h`:233
- Brief: n/a
- Parameters:
  - `column` (const std::string &): n/a
  - `key` (const std::string &): n/a

#### `bool put(const std::string &column, const std::string &key, const std::vector< uint8_t > &value, std::optional< compression::DataType > hint=std::nullopt)`
- Source: `include/storage/compressed_storage.h`:198
- Brief: Store value in a specific column.
- Parameters:
  - `column` (const std::string &): Input parameter.
  - `key` (const std::string &): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
  - `hint` (std::optional< compression::DataType >): Input parameter.
- Return: True when the operation succeeds.
- Details: Put. column Column name/namespace key Key within the column value Value to store hint Optional data type hint column Input parameter. key Input parameter. value Input parameter. hint Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), get(), compress(), std::move(), serialize(), make_full_key().

### themis::storage::ColumnMetadata

#### `double compressionRatio() const`
- Source: `include/storage/columnar_format.h`:89
- Brief: n/a
- Parameters: none

### themis::storage::ColumnSegment

#### `ColumnSegment()=default`
- Source: `include/storage/columnar_format.h`:187
- Brief: n/a
- Parameters: none

#### `void buildZoneMap()`
- Source: `include/storage/columnar_format.h`:224
- Brief: Build Zone Map.
- Parameters: none
- Details: Calls: empty(), data(), max(), min(), std::min(), std::max(), lowest().

#### `size_t byteSize() const noexcept`
- Source: `include/storage/columnar_cache.h`:105
- Brief: Estimated memory footprint in bytes.
- Parameters: none

#### `bool canSkipSegment(const void *filter_value) const`
- Source: `include/storage/columnar_format.h`:215
- Brief: n/a
- Parameters:
  - `filter_value` (const void *): n/a

#### `Result< ColumnSegment > create(ColumnType type, const void *data, size_t row_count, CompressionCodec codec=CompressionCodec::NONE)`
- Source: `include/storage/columnar_format.h`:190
- Brief: Create.
- Parameters:
  - `type` (ColumnType): Input parameter.
  - `data` (const void *): Input parameter.
  - `row_count` (size_t): Input parameter.
  - `codec` (CompressionCodec): Input parameter.
- Return: Return value.
- Details: type Input parameter. data Input parameter. row_count Input parameter. codec Input parameter. Return value. Calls: tl::unexpected(), Error(), assign(), buildZoneMap(), selectOptimalCodec().

#### `Result< void > decode()`
- Source: `include/storage/columnar_format.h`:201
- Brief: Decode.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: spdlog::debug(), RLECodec::decodeInt32(), tl::unexpected(), error(), resize(), size(), std::memcpy(), data().

#### `Result< ColumnSegment > deserialize(const std::vector< uint8_t > &data)`
- Source: `include/storage/columnar_format.h`:207
- Brief: Deserialize.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Throws:
  - std::out_of_range: if an error occurs.
- Details: data Input parameter. Return value. std::out_of_range if an error occurs. Calls: size(), tl::unexpected(), Error(), std::memcpy(), read_uint64(), assign(), begin(), calculateSegmentChecksum().

#### `Result< void > encode()`
- Source: `include/storage/columnar_format.h`:198
- Brief: Encode.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: spdlog::debug(), Ok(), data(), RLECodec::encodeInt32(), RLECodec::encodeInt64(), tl::unexpected(), Error(), BitPackingCodec::encodeInt32().

#### `const std::vector< uint8_t > & encodedData() const`
- Source: `include/storage/columnar_format.h`:211
- Brief: n/a
- Parameters: none

#### `const ColumnMetadata & metadata() const`
- Source: `include/storage/columnar_format.h`:210
- Brief: n/a
- Parameters: none

#### `const std::vector< uint8_t > & rawData() const`
- Source: `include/storage/columnar_format.h`:212
- Brief: n/a
- Parameters: none

#### `CompressionCodec selectOptimalCodec(ColumnType type, const void *data, size_t row_count)`
- Source: `include/storage/columnar_format.h`:227
- Brief: Select Optimal Codec.
- Parameters:
  - `type` (ColumnType): Input parameter.
  - `data` (const void *): Input parameter.
  - `row_count` (size_t): Input parameter.
- Return: Return value.
- Details: type Input parameter. data Input parameter. row_count Input parameter. Return value. Implements selectOptimalCodec without additional internal calls.

#### `std::vector< uint8_t > serialize() const`
- Source: `include/storage/columnar_format.h`:204
- Brief: n/a
- Parameters: none

### themis::storage::ColumnarCache

#### `ColumnarCache()`
- Source: `include/storage/columnar_cache.h`:192
- Brief: Construct with default configuration.
- Parameters: none

#### `ColumnarCache(Config config)`
- Source: `include/storage/columnar_cache.h`:189
- Brief: n/a
- Parameters:
  - `config` (Config): n/a

#### `ColumnarCache(const ColumnarCache &)=delete`
- Source: `include/storage/columnar_cache.h`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ColumnarCache &): n/a

#### `size_t bytesUsed() const noexcept`
- Source: `include/storage/columnar_cache.h`:256
- Brief: n/a
- Parameters: none
- Details: Total bytes currently used by cached segments.

#### `void clear()`
- Source: `include/storage/columnar_cache.h`:243
- Brief: Evict all unpinned segments.
- Parameters: none
- Details: Clear. Calls: void(), lk(), begin(), end(), byteSize(), push_back(), find(), erase().

#### `bool contains(const SegmentKey &key) const noexcept`
- Source: `include/storage/columnar_cache.h`:230
- Brief: Returns true if the key is present in the cache.
- Parameters:
  - `key` (const SegmentKey &): n/a
- Details: Does not update the LRU order or pin count.

#### `void decrementPin(const SegmentKey &key) noexcept`
- Source: `include/storage/columnar_cache.h`:270
- Brief: n/a
- Parameters:
  - `key` (const SegmentKey &): n/a

#### `bool evict(const SegmentKey &key)`
- Source: `include/storage/columnar_cache.h`:238
- Brief: Explicitly evict a segment by key.
- Parameters:
  - `key` (const SegmentKey &): Input parameter.
- Return: true if the segment was evicted, false otherwise.
- Details: Evict. Silently ignored if the segment is pinned or not present. true if the segment was evicted, false otherwise. key Input parameter. True when the operation succeeds. Calls: void(), lk(), find(), end(), byteSize(), erase(), on_evict_cb().

#### `void evictLRU()`
- Source: `include/storage/columnar_cache.h`:274
- Brief: Iterator usage below is safe: after erase() the iterator is immediately reassigned via the return value and the old iterator is never accessed.
- Parameters: none
- Details: Data-race and iterator-invalidation scanner alerts on this loop are false positives. Calls: empty(), end(), begin(), find(), byteSize(), push_back(), erase(), on_evict().

#### `PinGuard get(const SegmentKey &key)`
- Source: `include/storage/columnar_cache.h`:223
- Brief: Retrieve and pin a segment.
- Parameters:
  - `key` (const SegmentKey &): Input parameter.
- Return: A PinGuard wrapping the segment, or an empty guard if the key is not present.
- Details: Get. key The segment key. A PinGuard wrapping the segment, or an empty guard if the key is not present. On a cache hit the segment is promoted to the MRU position. key Input parameter. Return value. Calls: lk(), find(), end(), erase(), push_front(), begin(), PinGuard().

#### `uint64_t hitCount() const noexcept`
- Source: `include/storage/columnar_cache.h`:262
- Brief: n/a
- Parameters: none
- Details: Cache hit count since construction.

#### `size_t maxBytes() const noexcept`
- Source: `include/storage/columnar_cache.h`:259
- Brief: n/a
- Parameters: none
- Details: Maximum bytes allowed by configuration.

#### `uint64_t missCount() const noexcept`
- Source: `include/storage/columnar_cache.h`:265
- Brief: n/a
- Parameters: none
- Details: Cache miss count since construction.

#### `ColumnarCache & operator=(const ColumnarCache &)=delete`
- Source: `include/storage/columnar_cache.h`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ColumnarCache &): n/a

#### `size_t pinnedCount() const noexcept`
- Source: `include/storage/columnar_cache.h`:253
- Brief: n/a
- Parameters: none
- Details: Number of pinned segments.

#### `void put(ColumnSegment segment)`
- Source: `include/storage/columnar_cache.h`:212
- Brief: Insert or replace a segment.
- Parameters:
  - `segment` (ColumnSegment): Input parameter.
- Details: Put. If inserting the segment would exceed max_bytes, unpinned segments are evicted in LRU order until enough space is available. If insufficient unpinned space exists the segment is still inserted (the cache temporarily exceeds max_bytes until pins are released). segment The segment to insert. Ownership is taken by copy. segment Input parameter. Calls: byteSize(), lk(), find(), end(), std::move(), erase(), push_front(), begin().

#### `size_t size() const noexcept`
- Source: `include/storage/columnar_cache.h`:250
- Brief: n/a
- Parameters: none
- Details: Total segments currently in the cache (pinned + unpinned).

#### `~ColumnarCache()=default`
- Source: `include/storage/columnar_cache.h`:193
- Brief: n/a
- Parameters: none

### themis::storage::ColumnarCache::Config

#### `Config()=default`
- Source: `include/storage/columnar_cache.h`:182
- Brief: n/a
- Parameters: none

### themis::storage::ColumnarFormatManager

#### `ColumnarFormatManager()=default`
- Source: `include/storage/columnar_format.h`:241
- Brief: n/a
- Parameters: none

#### `Result< std::vector< ColumnSegment > > createSegments(const std::vector< ColumnType > &column_types, const std::vector< void * > &column_data, size_t row_count, bool auto_select_codec=true)`
- Source: `include/storage/columnar_format.h`:244
- Brief: Create Segments.
- Parameters:
  - `column_types` (const std::vector< ColumnType > &): Input parameter.
  - `column_data` (const std::vector< void * > &): Input parameter.
  - `row_count` (size_t): Input parameter.
  - `auto_select_codec` (bool): Input parameter.
- Return: Return value.
- Details: column_types Input parameter. column_data Input parameter. row_count Input parameter. auto_select_codec Input parameter. Return value. Calls: size(), tl::unexpected(), Error(), reserve(), ColumnSegment::create(), error(), encode(), push_back().

#### `Result< std::vector< size_t > > filterSegments(const std::vector< ColumnSegment > &segments, size_t column_index, const void *filter_value)`
- Source: `include/storage/columnar_format.h`:258
- Brief: Filter Segments.
- Parameters:
  - `segments` (const std::vector< ColumnSegment > &): Input parameter.
  - `column_index` (size_t): Input parameter.
  - `filter_value` (const void *): Input parameter.
- Return: Return value.
- Details: segments Input parameter. column_index Input parameter. filter_value Input parameter. Return value. Calls: size(), tl::unexpected(), Error(), canSkipSegment(), push_back().

#### `CompressionStats getCompressionStats(const std::vector< ColumnSegment > &segments) const`
- Source: `include/storage/columnar_format.h`:272
- Brief: n/a
- Parameters:
  - `segments` (const std::vector< ColumnSegment > &): n/a

#### `Result< std::vector< ColumnSegment > > projectColumns(const std::vector< ColumnSegment > &segments, const std::vector< size_t > &column_indices)`
- Source: `include/storage/columnar_format.h`:252
- Brief: Project Columns.
- Parameters:
  - `segments` (const std::vector< ColumnSegment > &): Input parameter.
  - `column_indices` (const std::vector< size_t > &): Input parameter.
- Return: Return value.
- Details: segments Input parameter. column_indices Input parameter. Return value. Calls: reserve(), size(), tl::unexpected(), Error(), push_back().

### themis::storage::CompressedStorageWrapper

#### `CompressedStorageWrapper(std::shared_ptr< IStorageBackend > backend, const compression::CompressionConfig &config=compression::CompressionConfig{})`
- Source: `include/storage/compressed_storage.h`:72
- Brief: Constructor.
- Parameters:
  - `backend` (std::shared_ptr< IStorageBackend >): Storage backend implementation
  - `config` (const compression::CompressionConfig &): Compression configuration
- Details: backend Storage backend implementation config Compression configuration

#### `bool del(const std::string &key)`
- Source: `include/storage/compressed_storage.h`:125
- Brief: Delete key.
- Parameters:
  - `key` (const std::string &): n/a

#### `bool exists(const std::string &key)`
- Source: `include/storage/compressed_storage.h`:132
- Brief: Check if key exists.
- Parameters:
  - `key` (const std::string &): n/a

#### `std::optional< std::vector< uint8_t > > get(const std::string &key)`
- Source: `include/storage/compressed_storage.h`:109
- Brief: Retrieve and decompress value.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Decompressed value, or nullopt if not found or decompression fails
- Details: Get. key Storage key Decompressed value, or nullopt if not found or decompression fails key Input parameter. Return value. Calls: CompressedValue::deserialize(), decompress(), empty().

#### `const compression::CompressionConfig & get_compression_config() const`
- Source: `include/storage/compressed_storage.h`:160
- Brief: Get current compression configuration.
- Parameters: none

#### `std::string get_compression_stats() const`
- Source: `include/storage/compressed_storage.h`:139
- Brief: Get compression statistics.
- Parameters: none

#### `std::optional< std::string > get_string(const std::string &key)`
- Source: `include/storage/compressed_storage.h`:114
- Brief: Retrieve and decompress as string.
- Parameters:
  - `key` (const std::string &): n/a

#### `bool put(const std::string &key, const std::string &value, std::optional< compression::DataType > hint=std::nullopt)`
- Source: `include/storage/compressed_storage.h`:94
- Brief: Store string value with automatic compression.
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a
  - `hint` (std::optional< compression::DataType >): n/a

#### `bool put(const std::string &key, const std::vector< uint8_t > &value, std::optional< compression::DataType > hint=std::nullopt)`
- Source: `include/storage/compressed_storage.h`:85
- Brief: Store value with automatic compression.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
  - `hint` (std::optional< compression::DataType >): Input parameter.
- Return: true if successful
- Details: Put. key Storage key value Raw value to compress and store hint Optional data type hint for adaptive compression true if successful key Input parameter. value Input parameter. hint Input parameter. True when the operation succeeds. Calls: compress(), std::move(), serialize().

#### `void reset_compression_stats()`
- Source: `include/storage/compressed_storage.h`:146
- Brief: Reset compression statistics.
- Parameters: none

#### `void set_compression_config(const compression::CompressionConfig &config)`
- Source: `include/storage/compressed_storage.h`:153
- Brief: Update compression configuration.
- Parameters:
  - `config` (const compression::CompressionConfig &): n/a

### themis::storage::CompressedStorageWrapper::IStorageBackend

#### `bool del(const std::string &key)=0`
- Source: `include/storage/compressed_storage.h`:62
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `bool exists(const std::string &key)=0`
- Source: `include/storage/compressed_storage.h`:63
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::optional< std::vector< uint8_t > > get(const std::string &key)=0`
- Source: `include/storage/compressed_storage.h`:61
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `bool put(const std::string &key, const std::vector< uint8_t > &value)=0`
- Source: `include/storage/compressed_storage.h`:60
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::vector< uint8_t > &): n/a

#### `~IStorageBackend()=default`
- Source: `include/storage/compressed_storage.h`:58
- Brief: n/a
- Parameters: none

### themis::storage::CompressedValue

#### `std::optional< CompressedValue > deserialize(const std::vector< uint8_t > &bytes)`
- Source: `include/storage/compressed_storage.h`:40
- Brief: Deserialize.
- Parameters:
  - `bytes` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: bytes Input parameter. Return value. Calls: size(), cv_crc32(), data(), assign(), begin().

#### `std::vector< uint8_t > serialize() const`
- Source: `include/storage/compressed_storage.h`:37
- Brief: n/a
- Parameters: none

### themis::storage::ConcurrentWriteController

#### `ConcurrentWriteController(ConcurrentWriteController &&)=delete`
- Source: `include/storage/concurrent_write_controller.h`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrentWriteController &&): n/a

#### `ConcurrentWriteController(ConcurrentWriteControllerConfig config={})`
- Source: `include/storage/concurrent_write_controller.h`:133
- Brief: n/a
- Parameters:
  - `config` (ConcurrentWriteControllerConfig): n/a

#### `ConcurrentWriteController(const ConcurrentWriteController &)=delete`
- Source: `include/storage/concurrent_write_controller.h`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ConcurrentWriteController &): n/a

#### `WriteGuard acquire()`
- Source: `include/storage/concurrent_write_controller.h`:154
- Brief: Acquire a write slot, waiting in FIFO order if necessary.
- Parameters: none
- Return: A RAII WriteGuard; the slot is released when the guard is destroyed or release() is called.
- Throws:
  - std::runtime_error: wrapping ConcurrentWriteAcquireError if the queue is full, the timeout fires, or the controller is shut down.
  - std::runtime_error: if an error occurs.
- Details: Acquire. A RAII WriteGuard; the slot is released when the guard is destroyed or release() is called. std::runtime_error wrapping ConcurrentWriteAcquireError if the queue is full, the timeout fires, or the controller is shut down. Return value. std::runtime_error if an error occurs. Calls: std::chrono::steady_clock::now(), lk(), fetch_add(), unlock(), recordWait(), WriteGuard(), size(), get_future().

#### `ConcurrentWriteStats getStats() const noexcept`
- Source: `include/storage/concurrent_write_controller.h`:176
- Brief: n/a
- Parameters: none

#### `size_t maxConcurrentWrites() const noexcept`
- Source: `include/storage/concurrent_write_controller.h`:178
- Brief: n/a
- Parameters: none

#### `ConcurrentWriteController & operator=(ConcurrentWriteController &&)=delete`
- Source: `include/storage/concurrent_write_controller.h`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrentWriteController &&): n/a

#### `ConcurrentWriteController & operator=(const ConcurrentWriteController &)=delete`
- Source: `include/storage/concurrent_write_controller.h`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ConcurrentWriteController &): n/a

#### `void recordWait(int64_t wait_us) noexcept`
- Source: `include/storage/concurrent_write_controller.h`:191
- Brief: Record a successful acquire wait time and update EWMA / sliding window.
- Parameters:
  - `wait_us` (int64_t): n/a

#### `void releaseSlot() noexcept`
- Source: `include/storage/concurrent_write_controller.h`:188
- Brief: Called by WriteGuard destructor / release() to return a slot.
- Parameters: none

#### `bool removeWaiterLocked(const std::shared_ptr< Waiter > &waiter)`
- Source: `include/storage/concurrent_write_controller.h`:194
- Brief: Remove a still-queued waiter after timeout/cancellation.
- Parameters:
  - `waiter` (const std::shared_ptr< Waiter > &): Input parameter.
- Return: True when the operation succeeds.
- Details: Remove Waiter Locked. waiter Input parameter. True when the operation succeeds. Calls: std::find(), begin(), end(), erase().

#### `void shutdown() noexcept`
- Source: `include/storage/concurrent_write_controller.h`:172
- Brief: Shut down the controller.
- Parameters: none
- Details: Unblocks all current waiters (they receive SHUTDOWN error) and prevents future acquire() calls from succeeding.

#### `std::optional< WriteGuard > tryAcquire()`
- Source: `include/storage/concurrent_write_controller.h`:162
- Brief: Try to acquire a slot without blocking.
- Parameters: none
- Return: A WriteGuard on success; std::nullopt if no slot is immediately available.
- Details: Try Acquire. A WriteGuard on success; std::nullopt if no slot is immediately available. Return value. Calls: lk(), fetch_add(), WriteGuard().

#### `~ConcurrentWriteController()`
- Source: `include/storage/concurrent_write_controller.h`:135
- Brief: n/a
- Parameters: none

### themis::storage::ConnectionKeepalive

#### `ConnectionKeepalive(std::chrono::seconds interval, std::function< bool()> keepalive_fn)`
- Source: `include/storage/database_connection_manager.h`:283
- Brief: n/a
- Parameters:
  - `interval` (std::chrono::seconds): n/a
  - `keepalive_fn` (std::function< bool()>): n/a

#### `size_t getFailureCount() const`
- Source: `include/storage/database_connection_manager.h`:313
- Brief: Get number of failed keepalives.
- Parameters: none

#### `size_t getKeepaliveCount() const`
- Source: `include/storage/database_connection_manager.h`:308
- Brief: Get number of keepalive attempts.
- Parameters: none

#### `bool isRunning() const`
- Source: `include/storage/database_connection_manager.h`:303
- Brief: Check if keepalive is running.
- Parameters: none

#### `void keepaliveLoop()`
- Source: `include/storage/database_connection_manager.h`:326
- Brief: Keepalive Loop.
- Parameters: none
- Details: Calls: load(), lock(), wait_for(), unlock(), keepalive_fn_(), spdlog::warn(), spdlog::error(), what().

#### `void start()`
- Source: `include/storage/database_connection_manager.h`:293
- Brief: Start keepalive thread.
- Parameters: none
- Details: Start. Calls: load(), keepalive_fn_(), spdlog::warn(), spdlog::error(), what(), std::thread(), spdlog::info(), count().

#### `void stop()`
- Source: `include/storage/database_connection_manager.h`:298
- Brief: Stop keepalive thread.
- Parameters: none
- Details: Stop. Calls: load(), keepalive_fn_(), spdlog::warn(), spdlog::error(), what(), lock(), notify_all(), joinable().

#### `~ConnectionKeepalive()`
- Source: `include/storage/database_connection_manager.h`:288
- Brief: n/a
- Parameters: none

### themis::storage::ConnectionTimeoutGuard

#### `ConnectionTimeoutGuard(std::chrono::seconds timeout, const std::string &operation_name)`
- Source: `include/storage/database_connection_manager.h`:336
- Brief: n/a
- Parameters:
  - `timeout` (std::chrono::seconds): n/a
  - `operation_name` (const std::string &): n/a

#### `void cancel()`
- Source: `include/storage/database_connection_manager.h`:351
- Brief: Cancel timeout (call when operation completes).
- Parameters: none
- Details: Cancel. Implements cancel without additional internal calls.

#### `std::chrono::milliseconds getElapsedTime() const`
- Source: `include/storage/database_connection_manager.h`:356
- Brief: Get elapsed time.
- Parameters: none

#### `bool hasTimedOut() const`
- Source: `include/storage/database_connection_manager.h`:346
- Brief: Check if operation has timed out.
- Parameters: none

#### `~ConnectionTimeoutGuard()`
- Source: `include/storage/database_connection_manager.h`:341
- Brief: n/a
- Parameters: none

### themis::storage::DatabaseConnectionManager

#### `DatabaseConnectionManager(const ConnectionConfig &config)`
- Source: `include/storage/database_connection_manager.h`:126
- Brief: n/a
- Parameters:
  - `config` (const ConnectionConfig &): n/a

#### `std::shared_ptr< Connection > acquireConnection(bool blocking=true, std::chrono::seconds timeout=std::chrono::seconds(10))`
- Source: `include/storage/database_connection_manager.h`:141
- Brief: Acquire a connection from the pool.
- Parameters:
  - `blocking` (bool): Whether to wait for connection
  - `timeout` (std::chrono::seconds): Maximum time to wait
- Return: Shared pointer to connection, or nullptr if failed
- Details: If all connections are in use, will either: Wait for one to become available (if blocking) Create a new connection (if pool not at max) Return nullptr (if non-blocking and none available) blocking Whether to wait for connection timeout Maximum time to wait Shared pointer to connection, or nullptr if failed

#### `std::chrono::milliseconds calculateBackoffDelay(size_t attempt) const`
- Source: `include/storage/database_connection_manager.h`:239
- Brief: n/a
- Parameters:
  - `attempt` (size_t): n/a

#### `bool canAttemptConnection() const`
- Source: `include/storage/database_connection_manager.h`:238
- Brief: n/a
- Parameters: none

#### `void closeAll()`
- Source: `include/storage/database_connection_manager.h`:202
- Brief: Close all connections.
- Parameters: none
- Details: Close All. Calls: lock(), spdlog::info(), empty(), front(), pop(), close(), clear().

#### `std::shared_ptr< Connection > createConnection()=0`
- Source: `include/storage/database_connection_manager.h`:206
- Brief: n/a
- Parameters: none

#### `bool executeWithRetry(std::function< ResultType(std::shared_ptr< Connection >)> operation, ResultType &result)`
- Source: `include/storage/database_connection_manager.h`:167
- Brief: Execute an operation with automatic retry.
- Parameters:
  - `operation` (std::function< ResultType(std::shared_ptr< Connection >)>): Function to execute with connection
  - `result` (ResultType &): n/a
- Return: true if operation succeeded (possibly after retries)
- Details: Wraps a database operation with connection acquisition, retry logic, and automatic reconnection on failure. operation Function to execute with connection true if operation succeeded (possibly after retries)

#### `std::vector< ConnectionHealth > getConnectionHealth() const`
- Source: `include/storage/database_connection_manager.h`:187
- Brief: Get health info for all connections.
- Parameters: none

#### `ConnectionStats getStats() const`
- Source: `include/storage/database_connection_manager.h`:182
- Brief: Get connection statistics.
- Parameters: none

#### `bool isConnectionStale(const Connection *conn) const`
- Source: `include/storage/database_connection_manager.h`:235
- Brief: n/a
- Parameters:
  - `conn` (const Connection *): n/a

#### `bool isHealthy() const`
- Source: `include/storage/database_connection_manager.h`:192
- Brief: Check if manager is healthy.
- Parameters: none

#### `void performHealthCheck()`
- Source: `include/storage/database_connection_manager.h`:177
- Brief: Check health of all connections.
- Parameters: none
- Details: Perform Health Check. Pings all connections and removes stale/failed ones Calls: lock(), spdlog::debug(), empty(), front(), pop(), isConnectionStale(), get(), ping().

#### `std::shared_ptr< Connection > reconnect(std::shared_ptr< Connection > old_conn)`
- Source: `include/storage/database_connection_manager.h`:209
- Brief: n/a
- Parameters:
  - `old_conn` (std::shared_ptr< Connection >): n/a

#### `void reconnectAll()`
- Source: `include/storage/database_connection_manager.h`:197
- Brief: Force reconnect all connections.
- Parameters: none
- Details: Reconnect All. Calls: lock(), spdlog::info(), empty(), front(), pop(), erase(), get(), close().

#### `void releaseConnection(std::shared_ptr< Connection > conn, bool error_occurred=false)`
- Source: `include/storage/database_connection_manager.h`:152
- Brief: Release a connection back to the pool.
- Parameters:
  - `conn` (std::shared_ptr< Connection >): Input parameter.
  - `error_occurred` (bool): Input parameter.
- Details: Release Connection. conn Connection to release error_occurred Whether an error occurred during use conn Input parameter. error_occurred Input parameter. Calls: lock(), find(), get(), end(), spdlog::warn(), erase(), getError(), updateCircuitBreaker().

#### `bool shouldRemoveConnection(const Connection *conn) const`
- Source: `include/storage/database_connection_manager.h`:236
- Brief: n/a
- Parameters:
  - `conn` (const Connection *): n/a

#### `void updateCircuitBreaker(bool success)`
- Source: `include/storage/database_connection_manager.h`:237
- Brief: Update Circuit Breaker.
- Parameters:
  - `success` (bool): Input parameter.
- Details: success Input parameter. Calls: load(), spdlog::info(), std::chrono::system_clock::now(), spdlog::error().

#### `~DatabaseConnectionManager()`
- Source: `include/storage/database_connection_manager.h`:127
- Brief: n/a
- Parameters: none

### themis::storage::DatabaseConnectionManager::Connection

#### `void close()=0`
- Source: `include/storage/database_connection_manager.h`:117
- Brief: n/a
- Parameters: none

#### `std::string getError() const =0`
- Source: `include/storage/database_connection_manager.h`:116
- Brief: n/a
- Parameters: none

#### `bool isValid() const =0`
- Source: `include/storage/database_connection_manager.h`:114
- Brief: n/a
- Parameters: none

#### `bool ping()=0`
- Source: `include/storage/database_connection_manager.h`:115
- Brief: n/a
- Parameters: none

#### `~Connection()=default`
- Source: `include/storage/database_connection_manager.h`:112
- Brief: n/a
- Parameters: none

### themis::storage::DictionaryCodec

#### `Result< std::vector< std::string > > decodeStrings(const std::vector< uint8_t > &encoded)`
- Source: `include/storage/columnar_format.h`:124
- Brief: Decode Strings.
- Parameters:
  - `encoded` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: encoded Input parameter. Return value. Calls: size(), tl::unexpected(), Error(), std::memcpy(), reserve(), str(), push_back(), std::move().

#### `Result< std::vector< uint8_t > > encodeStrings(const std::vector< std::string > &data)`
- Source: `include/storage/columnar_format.h`:121
- Brief: Encode Strings.
- Parameters:
  - `data` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: empty(), find(), end(), size(), max(), tl::unexpected(), Error(), push_back().

#### `bool shouldUseDictionary(const std::vector< std::string > &data, double min_compression_ratio=1.5)`
- Source: `include/storage/columnar_format.h`:127
- Brief: Should Use Dictionary.
- Parameters:
  - `data` (const std::vector< std::string > &): Input parameter.
  - `min_compression_ratio` (double): Input parameter.
- Return: True when the operation succeeds.
- Details: data Input parameter. min_compression_ratio Input parameter. True when the operation succeeds. Calls: empty(), unique_strings(), begin(), end(), size().

### themis::storage::DiskSpaceGuard

#### `DiskSpaceGuard(DiskSpaceMonitor &monitor, size_t required_bytes, const std::string &operation_name)`
- Source: `include/storage/disk_space_monitor.h`:257
- Brief: n/a
- Parameters:
  - `monitor` (DiskSpaceMonitor &): n/a
  - `required_bytes` (size_t): n/a
  - `operation_name` (const std::string &): n/a

#### `std::string getError() const`
- Source: `include/storage/disk_space_monitor.h`:273
- Brief: Get error message if guard is invalid.
- Parameters: none

#### `bool isValid() const`
- Source: `include/storage/disk_space_monitor.h`:268
- Brief: Check if guard acquired space successfully.
- Parameters: none

#### `~DiskSpaceGuard()`
- Source: `include/storage/disk_space_monitor.h`:263
- Brief: n/a
- Parameters: none

### themis::storage::DiskSpaceMonitor

#### `DiskSpaceMonitor(const std::string &path)`
- Source: `include/storage/disk_space_monitor.h`:103
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

#### `DiskSpaceMonitor(const std::string &path, const Config &config)`
- Source: `include/storage/disk_space_monitor.h`:104
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a
  - `config` (const Config &): n/a

#### `float calculateUsageTrend() const`
- Source: `include/storage/disk_space_monitor.h`:247
- Brief: n/a
- Parameters: none

#### `bool canWrite(size_t bytes_to_write=0)`
- Source: `include/storage/disk_space_monitor.h`:134
- Brief: Check if write operation can proceed.
- Parameters:
  - `bytes_to_write` (size_t): Input parameter.
- Return: true if write should proceed, false if blocked
- Details: Can Write. Performs pre-flight check before allowing write operations. bytes_to_write Size of planned write operation Zero-byte writes are always allowed because they do not consume disk capacity. true if write should proceed, false if blocked bytes_to_write Input parameter. True when the operation succeeds. Calls: load(), spdlog::warn(), isReadOnly(), getSpaceInfo(), spdlog::error().

#### `SpaceInfo checkSpace()`
- Source: `include/storage/disk_space_monitor.h`:122
- Brief: Check disk space immediately.
- Parameters: none
- Return: Current space information
- Details: Check Space. Current space information Return value. Calls: queryDiskSpace(), lock(), load(), updateSpaceLevel(), recordUsage(), std::chrono::system_clock::now(), spdlog::warn(), shouldSendAlert().

#### `std::chrono::seconds estimateTimeUntilFull() const`
- Source: `include/storage/disk_space_monitor.h`:207
- Brief: Calculate time until disk full.
- Parameters: none
- Return: Estimated seconds until full, or 0 if cannot estimate
- Details: Estimates time until disk full based on recent usage patterns Estimated seconds until full, or 0 if cannot estimate

#### `std::string getRecommendedAction() const`
- Source: `include/storage/disk_space_monitor.h`:198
- Brief: Get recommended action based on space level.
- Parameters: none

#### `SpaceInfo getSpaceInfo() const`
- Source: `include/storage/disk_space_monitor.h`:149
- Brief: Get current space info.
- Parameters: none

#### `SpaceLevel getSpaceLevel() const`
- Source: `include/storage/disk_space_monitor.h`:144
- Brief: Get current space level.
- Parameters: none

#### `MonitorStats getStats() const`
- Source: `include/storage/disk_space_monitor.h`:154
- Brief: Get monitoring statistics.
- Parameters: none

#### `void handleSpaceLevelChange(SpaceLevel old_level, SpaceLevel new_level)`
- Source: `include/storage/disk_space_monitor.h`:243
- Brief: Handle Space Level Change.
- Parameters:
  - `old_level` (SpaceLevel): Input parameter.
  - `new_level` (SpaceLevel): Input parameter.
- Details: old_level Input parameter. new_level Input parameter. Calls: spdlog::warn(), shouldSendAlert(), disk_utils::formatBytes(), std::setprecision(), sendAlert(), str(), triggerGC().

#### `bool isReadOnly() const`
- Source: `include/storage/disk_space_monitor.h`:139
- Brief: Check if database is in read-only mode.
- Parameters: none

#### `void monitoringLoop()`
- Source: `include/storage/disk_space_monitor.h`:240
- Brief: Monitoring Loop.
- Parameters: none
- Details: Calls: load(), checkSpace(), spdlog::error(), what(), std::chrono::seconds(), std::min(), std::this_thread::sleep_for().

#### `SpaceInfo queryDiskSpace()`
- Source: `include/storage/disk_space_monitor.h`:241
- Brief: Query Disk Space.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: disk_utils::getDiskSpace(), spdlog::error().

#### `void recordUsage(const SpaceInfo &info)`
- Source: `include/storage/disk_space_monitor.h`:246
- Brief: Record Usage.
- Parameters:
  - `info` (const SpaceInfo &): Input parameter.
- Details: info Input parameter. Calls: std::chrono::system_clock::now(), push_back(), size(), erase(), begin().

#### `void sendAlert(const SpaceInfo &info, const std::string &message)`
- Source: `include/storage/disk_space_monitor.h`:244
- Brief: Send Alert.
- Parameters:
  - `info` (const SpaceInfo &): Input parameter.
  - `message` (const std::string &): Input parameter.
- Details: info Input parameter. message Input parameter. Calls: lock(), std::chrono::system_clock::now(), spdlog::warn(), alert_cb(), spdlog::error(), what().

#### `void setAlertCallback(AlertCallback callback)`
- Source: `include/storage/disk_space_monitor.h`:161
- Brief: Register callback for space alerts.
- Parameters:
  - `callback` (AlertCallback): Input parameter.
- Details: Set Alert Callback. Callback is invoked when space level changes or threshold crossed callback Input parameter. Calls: lock(), std::move().

#### `void setGCCallback(GCCallback callback)`
- Source: `include/storage/disk_space_monitor.h`:168
- Brief: Register callback for garbage collection.
- Parameters:
  - `callback` (GCCallback): Input parameter.
- Details: Set GCCallback. Callback is invoked when automatic GC should run callback Input parameter. Calls: lock(), std::move().

#### `void setReadOnlyOverride(bool read_only)`
- Source: `include/storage/disk_space_monitor.h`:182
- Brief: Override read-only mode.
- Parameters:
  - `read_only` (bool): Input parameter.
- Details: Set Read Only Override. Use with caution - allows writes even in critical state read_only Input parameter. Calls: spdlog::warn().

#### `void setRocksDBSize(uint64_t size_bytes)`
- Source: `include/storage/disk_space_monitor.h`:193
- Brief: Update the tracked RocksDB on-disk size.
- Parameters:
  - `size_bytes` (uint64_t): Input parameter.
- Details: Set Rocks DBSize. Called by storage components after computing the SST files size via RocksDBWrapper::getApproximateSize(). The value is propagated into SpaceInfo::rocksdb_size_bytes and is returned by getSpaceInfo(). size_bytes Total RocksDB SST on-disk size in bytes size_bytes Input parameter. Calls: lock().

#### `bool shouldSendAlert() const`
- Source: `include/storage/disk_space_monitor.h`:245
- Brief: n/a
- Parameters: none

#### `void startMonitoring()`
- Source: `include/storage/disk_space_monitor.h`:110
- Brief: Start automatic monitoring.
- Parameters: none
- Details: Start Monitoring. Calls: load(), std::thread(), spdlog::info(), count().

#### `void stopMonitoring()`
- Source: `include/storage/disk_space_monitor.h`:115
- Brief: Stop automatic monitoring.
- Parameters: none
- Details: Stop Monitoring. Calls: load(), joinable(), themis::utils::joinThreadWithin(), spdlog::warn(), spdlog::info().

#### `void triggerGC()`
- Source: `include/storage/disk_space_monitor.h`:175
- Brief: Force a garbage collection.
- Parameters: none
- Details: Trigger GC. Manually trigger the registered GC callback Calls: lock(), spdlog::info(), gc_cb(), spdlog::error(), what().

#### `void updateSpaceLevel(const SpaceInfo &info)`
- Source: `include/storage/disk_space_monitor.h`:242
- Brief: Update Space Level.
- Parameters:
  - `info` (const SpaceInfo &): Input parameter.
- Details: info Input parameter. Implements updateSpaceLevel without additional internal calls.

#### `~DiskSpaceMonitor()`
- Source: `include/storage/disk_space_monitor.h`:105
- Brief: n/a
- Parameters: none

### themis::storage::DistributedTransaction

#### `DistributedTransaction(DistributedTransaction &&)=delete`
- Source: `include/storage/distributed_transaction_manager.h`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTransaction &&): n/a

#### `DistributedTransaction(PrivateTag, std::string txn_id, char separator, std::shared_ptr< ManagerSharedState > state)`
- Source: `include/storage/distributed_transaction_manager.h`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (PrivateTag): n/a
  - `txn_id` (std::string): n/a
  - `separator` (char): n/a
  - `state` (std::shared_ptr< ManagerSharedState >): n/a

#### `DistributedTransaction(const DistributedTransaction &)=delete`
- Source: `include/storage/distributed_transaction_manager.h`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedTransaction &): n/a

#### `bool commit()`
- Source: `include/storage/distributed_transaction_manager.h`:251
- Brief: Commit the transaction via two-phase commit.
- Parameters: none
- Return: true if all shards committed successfully.
- Details: ── Commit (2PC) ────────────────────────────────────────────────────────────── Runs Phase 1 (PREPARE to all participating shards) and then Phase 2 (COMMIT if all voted YES, ABORT otherwise). true if all shards committed successfully. True when the operation succeeds. Calls: THEMIS_WARN(), THEMIS_DEBUG(), size(), lk(), find(), end(), THEMIS_ERROR(), prepare().

#### `void del(std::string_view key)`
- Source: `include/storage/distributed_transaction_manager.h`:228
- Brief: Delete a key from the appropriate shard.
- Parameters:
  - `key` (std::string_view): Input parameter.
- Throws:
  - std::invalid_argument: if the transaction is not ACTIVE.
  - std::invalid_argument: if the shard_id is not registered.
  - std::runtime_error: if shard registration changes while active.
  - std::invalid_argument: if an error occurs.
  - std::runtime_error: if an error occurs.
- Details: Del. key "shard_id:logical_key". std::invalid_argument if the transaction is not ACTIVE. std::invalid_argument if the shard_id is not registered. std::runtime_error if shard registration changes while active. key Input parameter. std::invalid_argument if an error occurs. std::runtime_error if an error occurs. Calls: parseKey(), requireParticipant(), find(), end(), emplace(), push_back(), std::move().

#### `std::optional< std::string > get(std::string_view key)`
- Source: `include/storage/distributed_transaction_manager.h`:241
- Brief: Read a key from the appropriate shard (non-transactional snapshot).
- Parameters:
  - `key` (std::string_view): Input parameter.
- Return: Value bytes, or std::nullopt if the key is not found in the shard.
- Throws:
  - std::invalid_argument: if the shard_id is not registered.
  - std::runtime_error: if shard registration changes while active.
  - std::runtime_error: if an error occurs.
- Details: ── Read operation ──────────────────────────────────────────────────────────── Reads are routed to the shard via IDistributedShardParticipant::get(). The read is NOT part of the write-set and will not be validated at commit. key "shard_id:logical_key". Value bytes, or std::nullopt if the key is not found in the shard. std::invalid_argument if the shard_id is not registered. std::runtime_error if shard registration changes while active. key Input parameter. Return value. std::runtime_error if an error occurs. Calls: parseKey(), requireParticipant(), find(), end(), emplace().

#### `const std::string & id() const`
- Source: `include/storage/distributed_transaction_manager.h`:265
- Brief: n/a
- Parameters: none
- Return: Unique transaction identifier.
- Details: Unique transaction identifier.

#### `size_t operationCount() const`
- Source: `include/storage/distributed_transaction_manager.h`:271
- Brief: n/a
- Parameters: none
- Return: Number of buffered write operations.
- Details: Number of buffered write operations.

#### `DistributedTransaction & operator=(DistributedTransaction &&)=delete`
- Source: `include/storage/distributed_transaction_manager.h`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTransaction &&): n/a

#### `DistributedTransaction & operator=(const DistributedTransaction &)=delete`
- Source: `include/storage/distributed_transaction_manager.h`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedTransaction &): n/a

#### `std::pair< std::string, std::string > parseKey(std::string_view composite) const`
- Source: `include/storage/distributed_transaction_manager.h`:293
- Brief: Parse "shard_id:logical_key" → (shard_id, logical_key).
- Parameters:
  - `composite` (std::string_view): n/a

#### `std::vector< std::string > participatingShards() const`
- Source: `include/storage/distributed_transaction_manager.h`:268
- Brief: n/a
- Parameters: none
- Return: Shards that have operations buffered in this transaction.
- Details: Shards that have operations buffered in this transaction.

#### `void put(std::string_view key, std::string_view value)`
- Source: `include/storage/distributed_transaction_manager.h`:218
- Brief: Write a key-value pair to the appropriate shard.
- Parameters:
  - `key` (std::string_view): Input parameter.
  - `value` (std::string_view): Input parameter.
- Throws:
  - std::invalid_argument: if the transaction is not ACTIVE.
  - std::invalid_argument: if the shard_id is not registered.
  - std::runtime_error: if shard registration changes while active.
  - std::invalid_argument: if an error occurs.
  - std::runtime_error: if an error occurs.
- Details: ── Write operations ────────────────────────────────────────────────────────── key "shard_id:logical_key" — the prefix up to the first separator identifies the destination shard. value Value bytes to store. std::invalid_argument if the transaction is not ACTIVE. std::invalid_argument if the shard_id is not registered. std::runtime_error if shard registration changes while active. key Input parameter. value Input parameter. std::invalid_argument if an error occurs. std::runtime_error if an error occurs. Calls: parseKey(), requireParticipant(), find(), end(), emplace(), std::string(), push_back(), std::move().

#### `std::pair< std::shared_ptr< IDistributedShardParticipant >, uint64_t > requireParticipant(const std::string &shard_id) const`
- Source: `include/storage/distributed_transaction_manager.h`:299
- Brief: n/a
- Parameters:
  - `shard_id` (const std::string &): n/a
- Details: Look up participant/version under lock and return a reference-counted copy. Throws std::invalid_argument if shard is not registered. Throws std::runtime_error if shard version metadata is missing.

#### `void rollback()`
- Source: `include/storage/distributed_transaction_manager.h`:259
- Brief: Roll back the transaction without committing.
- Parameters: none
- Details: ── Rollback ────────────────────────────────────────────────────────────────── Sends ABORT to any shards that have already received a PREPARE, then marks the transaction ABORTED. No-op if already committed or aborted. Calls: abort(), THEMIS_ERROR(), what(), THEMIS_INFO(), fetch_add(), fetch_sub().

#### `DistributedTxnState state() const`
- Source: `include/storage/distributed_transaction_manager.h`:262
- Brief: n/a
- Parameters: none
- Return: Current lifecycle state.
- Details: Current lifecycle state.

#### `~DistributedTransaction()`
- Source: `include/storage/distributed_transaction_manager.h`:200
- Brief: n/a
- Parameters: none

### themis::storage::DistributedTransaction::PrivateTag

#### `PrivateTag()=default`
- Source: `include/storage/distributed_transaction_manager.h`:281
- Brief: n/a
- Parameters: none

### themis::storage::DistributedTransactionManager

#### `DistributedTransactionManager(DistributedTransactionManager &&)=delete`
- Source: `include/storage/distributed_transaction_manager.h`:368
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTransactionManager &&): n/a

#### `DistributedTransactionManager(const DistributedTransactionManager &)=delete`
- Source: `include/storage/distributed_transaction_manager.h`:366
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedTransactionManager &): n/a

#### `DistributedTransactionManager(std::vector< ShardConfig > shards={}, DistributedTxnConfig config={})`
- Source: `include/storage/distributed_transaction_manager.h`:358
- Brief: Construct a manager with an optional initial set of shards.
- Parameters:
  - `shards` (std::vector< ShardConfig >): n/a
  - `config` (DistributedTxnConfig): n/a

#### `std::shared_ptr< DistributedTransaction > beginDistributedTransaction()`
- Source: `include/storage/distributed_transaction_manager.h`:416
- Brief: Begin a new distributed transaction.
- Parameters: none
- Return: A non-null shared_ptr to the new DistributedTransaction.
- Details: A non-null shared_ptr to the new DistributedTransaction.

#### `std::string generateTransactionId()`
- Source: `include/storage/distributed_transaction_manager.h`:433
- Brief: ── ID generation ─────────────────────────────────────────────────────────────
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: fetch_add(), std::chrono::system_clock::now(), time_since_epoch(), count(), std::setw(), std::setfill(), str().

#### `bool hasShard(const std::string &shard_id) const`
- Source: `include/storage/distributed_transaction_manager.h`:407
- Brief: n/a
- Parameters:
  - `shard_id` (const std::string &): n/a
- Return: true if the given shard_id is registered.
- Details: true if the given shard_id is registered.

#### `DistributedTransactionManager & operator=(DistributedTransactionManager &&)=delete`
- Source: `include/storage/distributed_transaction_manager.h`:369
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedTransactionManager &&): n/a

#### `DistributedTransactionManager & operator=(const DistributedTransactionManager &)=delete`
- Source: `include/storage/distributed_transaction_manager.h`:367
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DistributedTransactionManager &): n/a

#### `void registerShard(const std::string &shard_id, IDistributedShardParticipant *participant)`
- Source: `include/storage/distributed_transaction_manager.h`:386
- Brief: Register a shard participant.
- Parameters:
  - `shard_id` (const std::string &): Identifier of the shard.
  - `participant` (IDistributedShardParticipant *): Input/output parameter.
- Throws:
  - std::invalid_argument: if shard_id is empty or participant is null.
  - std::invalid_argument: if an error occurs.
- Details: ── Shard management ────────────────────────────────────────────────────────── The participant is stored internally as a reference — the caller retains ownership. The caller must ensure the participant object remains valid until it is either explicitly unregistered or this manager is destroyed. Safe to call concurrently with other shard management operations. shard_id Logical shard identifier (prefix in "shard_id:key"). participant Non-null pointer to the shard's 2PC proxy. std::invalid_argument if shard_id is empty or participant is null. shard_id Identifier of the shard. participant Input/output parameter. std::invalid_argument if an error occurs. Calls: empty(), lk(), THEMIS_DEBUG().

#### `size_t shardCount() const`
- Source: `include/storage/distributed_transaction_manager.h`:404
- Brief: n/a
- Parameters: none
- Return: Number of registered shards.
- Details: Number of registered shards.

#### `Statistics statistics() const`
- Source: `include/storage/distributed_transaction_manager.h`:427
- Brief: n/a
- Parameters: none
- Return: Approximate snapshot of coordinator statistics.
- Details: Approximate snapshot of coordinator statistics. Counters are updated with relaxed ordering and may not reflect the very latest completed transactions on all cores. Suitable for monitoring; not suitable for synchronization.

#### `bool unregisterShard(const std::string &shard_id)`
- Source: `include/storage/distributed_transaction_manager.h`:401
- Brief: Unregister a shard.
- Parameters:
  - `shard_id` (const std::string &): Identifier of the shard.
- Return: true if the shard was found and removed.
- Details: Unregister Shard. Safe to call concurrently. After this call returns, no new transactions will route operations to the shard. In-flight transactions that already hold a reference to the participant will complete their current call safely before releasing the reference. true if the shard was found and removed. shard_id Identifier of the shard. True when the operation succeeds. Calls: lk(), erase().

#### `~DistributedTransactionManager()=default`
- Source: `include/storage/distributed_transaction_manager.h`:363
- Brief: n/a
- Parameters: none

### themis::storage::EncryptedBlobBackend

#### `EncryptedBlobBackend(std::shared_ptr< IBlobStorageBackend > inner, std::shared_ptr< IEncryptionKeyProvider > keys)`
- Source: `include/storage/encrypted_blob_backend.h`:119
- Brief: Construct an encrypting backend.
- Parameters:
  - `inner` (std::shared_ptr< IBlobStorageBackend >): Underlying backend where ciphertext is stored.
  - `keys` (std::shared_ptr< IEncryptionKeyProvider >): Key provider; must outlive this object.
- Throws:
  - std::invalid_argument: if inner or keys is nullptr.
- Details: inner Underlying backend where ciphertext is stored. keys Key provider; must outlive this object. std::invalid_argument if inner or keys is nullptr.

#### `std::vector< uint8_t > decrypt(const std::vector< uint8_t > &ciphertext) const`
- Source: `include/storage/encrypted_blob_backend.h`:160
- Brief: Decrypt ciphertext (with prepended IV). Returns plaintext or throws.
- Parameters:
  - `ciphertext` (const std::vector< uint8_t > &): n/a

#### `std::vector< uint8_t > encrypt(const std::vector< uint8_t > &plaintext) const`
- Source: `include/storage/encrypted_blob_backend.h`:157
- Brief: Encrypt plaintext with AES-256-GCM; prepend 12-byte IV.
- Parameters:
  - `plaintext` (const std::vector< uint8_t > &): n/a

#### `bool exists(const BlobRef &ref) override`
- Source: `include/storage/encrypted_blob_backend.h`:142
- Brief: Delegate to the underlying backend.
- Parameters:
  - `ref` (const BlobRef &): Input parameter.
- Return: True when the operation succeeds.
- Details: Exists. ref Input parameter. True when the operation succeeds.

#### `Result< std::vector< uint8_t > > get(const BlobRef &ref) override`
- Source: `include/storage/encrypted_blob_backend.h`:136
- Brief: Retrieve and decrypt a blob.
- Parameters:
  - `ref` (const BlobRef &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if GCM authentication tag verification fails.
- Details: Get. std::runtime_error if GCM authentication tag verification fails. ref Input parameter. Return value.

#### `bool isAvailable() const override`
- Source: `include/storage/encrypted_blob_backend.h`:146
- Brief: Check if backend is available.
- Parameters: none
- Return: true if backend can be used
- Details: true if backend can be used

#### `std::string name() const override`
- Source: `include/storage/encrypted_blob_backend.h`:144
- Brief: Get backend name.
- Parameters: none
- Return: Backend name (e.g., "filesystem", "s3", "webdav")
- Details: Backend name (e.g., "filesystem", "s3", "webdav")

#### `Result< BlobRef > put(const std::string &blob_id, const std::vector< uint8_t > &data) override`
- Source: `include/storage/encrypted_blob_backend.h`:128
- Brief: Encrypt data and store in the underlying backend.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: Put. A fresh 12-byte random IV is generated per call. The ciphertext layout is [ IV (12 B) \| ciphertext \| GCM tag (16 B) ]. blob_id Identifier of the blob. data Input parameter. Return value.

#### `Result< void > remove(const BlobRef &ref) override`
- Source: `include/storage/encrypted_blob_backend.h`:139
- Brief: Delegate to the underlying backend.
- Parameters:
  - `ref` (const BlobRef &): Input parameter.
- Return: Return value.
- Details: Remove. ref Input parameter. Return value.

#### `EncryptionStats stats() const`
- Source: `include/storage/encrypted_blob_backend.h`:153
- Brief: Read a snapshot of cumulative encryption statistics.
- Parameters: none
- Details: Thread-safe (takes the internal mutex).

### themis::storage::ExponentialBackoff

#### `ExponentialBackoff(const Config &config)`
- Source: `include/storage/database_connection_manager.h`:256
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `std::chrono::milliseconds calculateDelay(size_t attempt) const`
- Source: `include/storage/database_connection_manager.h`:264
- Brief: Calculate delay for given attempt number.
- Parameters:
  - `attempt` (size_t): Attempt number (0-indexed)
- Return: Delay with jitter applied
- Details: attempt Attempt number (0-indexed) Delay with jitter applied

#### `void reset()`
- Source: `include/storage/database_connection_manager.h`:269
- Brief: Reset backoff state.
- Parameters: none
- Details: Reset the modification detection flag. Implements reset without additional internal calls.

### themis::storage::FederatedBlobRouter

#### `std::shared_ptr< IBlobStorageBackend > backendFor(const std::string &region) const`
- Source: `include/storage/federated_blob_router.h`:264
- Brief: n/a
- Parameters:
  - `region` (const std::string &): n/a

#### `bool exists(const FederatedBlobRoute &route, const std::string &preferred_region="")`
- Source: `include/storage/federated_blob_router.h`:247
- Brief: Check whether any routed region still contains the blob.
- Parameters:
  - `route` (const FederatedBlobRoute &): Region placement metadata returned by put().
  - `preferred_region` (const std::string &): Optional region to check first.
- Return: true on the first positive existence check; false otherwise.
- Details: route Region placement metadata returned by put(). preferred_region Optional region to check first. true on the first positive existence check; false otherwise.

#### `Result< std::vector< uint8_t > > get(const FederatedBlobRoute &route, const std::string &preferred_region="")`
- Source: `include/storage/federated_blob_router.h`:177
- Brief: Retrieve a blob from the preferred region with cross-region fallback.
- Parameters:
  - `route` (const FederatedBlobRoute &): Region placement metadata returned by put().
  - `preferred_region` (const std::string &): Optional region to try first before the primary region.
- Return: Blob bytes on the first successful read, otherwise the last error observed.
- Details: route Region placement metadata returned by put(). preferred_region Optional region to try first before the primary region. Blob bytes on the first successful read, otherwise the last error observed.

#### `Result< FederatedBlobRoute > put(const FederatedBlobWritePlan &plan, const std::string &blob_id, const std::vector< uint8_t > &data)`
- Source: `include/storage/federated_blob_router.h`:106
- Brief: Store a blob in the primary region and synchronously replicate it.
- Parameters:
  - `plan` (const FederatedBlobWritePlan &): Write placement plan.
  - `blob_id` (const std::string &): Stable blob identifier.
  - `data` (const std::vector< uint8_t > &): Blob payload to replicate.
- Return: Region map containing the references returned by each successful backend.
- Details: plan Write placement plan. blob_id Stable blob identifier. data Blob payload to replicate. Region map containing the references returned by each successful backend. If a required replica fails, already-written regions are best-effort rolled back and the method returns an error.

#### `std::vector< std::string > readOrder(const FederatedBlobRoute &route, const std::string &preferred_region)`
- Source: `include/storage/federated_blob_router.h`:270
- Brief: n/a
- Parameters:
  - `route` (const FederatedBlobRoute &): n/a
  - `preferred_region` (const std::string &): n/a

#### `Result< void > registerBackend(const std::string &region, std::shared_ptr< IBlobStorageBackend > backend)`
- Source: `include/storage/federated_blob_router.h`:80
- Brief: Register a backend for a region.
- Parameters:
  - `region` (const std::string &): Stable region identifier (for example eu-central-1).
  - `backend` (std::shared_ptr< IBlobStorageBackend >): Backend instance serving that region.
- Return: ERR_UTIL_INVALID_ARGUMENT when the region is empty or the backend is null.
- Details: region Stable region identifier (for example eu-central-1). backend Backend instance serving that region. ERR_UTIL_INVALID_ARGUMENT when the region is empty or the backend is null.

#### `Result< void > remove(const FederatedBlobRoute &route)`
- Source: `include/storage/federated_blob_router.h`:216
- Brief: Delete all region-local copies recorded in a route.
- Parameters:
  - `route` (const FederatedBlobRoute &): Region placement metadata returned by put().
- Return: OkVoid() when all reachable copies were deleted.
- Details: route Region placement metadata returned by put(). OkVoid() when all reachable copies were deleted. Returns an error when at least one region-local delete fails.

#### `void rollback(const FederatedBlobRoute &route)`
- Source: `include/storage/federated_blob_router.h`:290
- Brief: n/a
- Parameters:
  - `route` (const FederatedBlobRoute &): n/a

### themis::storage::FilesystemBlobBackend

#### `FilesystemBlobBackend(const std::string &base_path)`
- Source: `include/storage/blob_backend_filesystem.h`:25
- Brief: n/a
- Parameters:
  - `base_path` (const std::string &): n/a

#### `std::string computeSHA256(const std::vector< uint8_t > &data)`
- Source: `include/storage/blob_backend_filesystem.h`:38
- Brief: Compute SHA256.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: SHA256(), data(), size(), std::setw(), std::setfill(), str().

#### `bool exists(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_filesystem.h`:31
- Brief: Exists.
- Parameters:
  - `ref` (const BlobRef &): Input parameter.
- Return: True when the operation succeeds.
- Details: ref Input parameter. True when the operation succeeds. Implements exists without additional internal calls.

#### `Result< std::vector< uint8_t > > get(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_filesystem.h`:29
- Brief: Get.
- Parameters:
  - `ref` (const BlobRef &): Input parameter.
- Return: Return value.
- Details: ref Input parameter. Return value. Calls: ifs(), THEMIS_WARN(), data(), THEMIS_DEBUG(), size(), Ok(), std::move(), THEMIS_ERROR().

#### `std::string getPath(const std::string &blob_id) const`
- Source: `include/storage/blob_backend_filesystem.h`:39
- Brief: n/a
- Parameters:
  - `blob_id` (const std::string &): n/a

#### `bool isAvailable() const override`
- Source: `include/storage/blob_backend_filesystem.h`:33
- Brief: Check if backend is available.
- Parameters: none
- Return: true if backend can be used
- Details: true if backend can be used

#### `std::string name() const override`
- Source: `include/storage/blob_backend_filesystem.h`:32
- Brief: Get backend name.
- Parameters: none
- Return: Backend name (e.g., "filesystem", "s3", "webdav")
- Details: Backend name (e.g., "filesystem", "s3", "webdav")

#### `Result< BlobRef > put(const std::string &blob_id, const std::vector< uint8_t > &data) override`
- Source: `include/storage/blob_backend_filesystem.h`:28
- Brief: Put.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: blob_id Identifier of the blob. data Input parameter. Return value. std::runtime_error if an error occurs. Calls: getPath(), fs::create_directories(), fs::path(), parent_path(), ofs(), write(), data(), size().

#### `Result< void > remove(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_filesystem.h`:30
- Brief: Remove.
- Parameters:
  - `ref` (const BlobRef &): Input parameter.
- Return: Return value.
- Details: ref Input parameter. Return value. Calls: THEMIS_DEBUG(), OkVoid(), THEMIS_ERROR(), what().

#### `~FilesystemBlobBackend() override=default`
- Source: `include/storage/blob_backend_filesystem.h`:26
- Brief: n/a
- Parameters: none

### themis::storage::FrameOfReferenceCodec

#### `Result< std::vector< int32_t > > decodeInt32(const std::vector< uint8_t > &encoded)`
- Source: `include/storage/columnar_format.h`:163
- Brief: Decode Int32.
- Parameters:
  - `encoded` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: encoded Input parameter. Return value. Calls: size(), tl::unexpected(), Error(), std::memcpy(), push_back().

#### `Result< std::vector< int64_t > > decodeInt64(const std::vector< uint8_t > &encoded)`
- Source: `include/storage/columnar_format.h`:164
- Brief: Decode Int64.
- Parameters:
  - `encoded` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: encoded Input parameter. Return value. Calls: size(), tl::unexpected(), Error(), std::memcpy(), push_back().

#### `Result< std::vector< uint8_t > > encodeInt32(const std::vector< int32_t > &data)`
- Source: `include/storage/columnar_format.h`:159
- Brief: Encode Int32.
- Parameters:
  - `data` (const std::vector< int32_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: empty(), insert(), end(), size().

#### `Result< std::vector< uint8_t > > encodeInt64(const std::vector< int64_t > &data)`
- Source: `include/storage/columnar_format.h`:160
- Brief: Encode Int64.
- Parameters:
  - `data` (const std::vector< int64_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: empty(), insert(), end(), size().

### themis::storage::GCSBlobBackend

#### `GCSBlobBackend(const std::string &bucket, const std::string &prefix="")`
- Source: `include/storage/blob_backend_gcs.h`:39
- Brief: n/a
- Parameters:
  - `bucket` (const std::string &): GCS bucket name
  - `prefix` (const std::string &): Optional object-name prefix (e.g. "blobs/")
- Details: bucket GCS bucket name prefix Optional object-name prefix (e.g. "blobs/")

#### `std::string computeSHA256(const std::vector< uint8_t > &data)`
- Source: `include/storage/blob_backend_gcs.h`:54
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a

#### `bool exists(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_gcs.h`:46
- Brief: Exists.
- Parameters:
  - `ref` (const BlobRef &): Input parameter.
- Return: True when the operation succeeds.
- Details: ref Input parameter. True when the operation succeeds. Calls: lock(), objectName(), GetObjectMetadata(), ok(), THEMIS_TRACE().

#### `Result< std::vector< uint8_t > > get(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_gcs.h`:44
- Brief: Get.
- Parameters:
  - `ref` (const BlobRef &): Input parameter.
- Return: Return value.
- Details: ref Input parameter. Return value. Calls: lock(), objectName(), ReadObject(), status(), code(), THEMIS_WARN(), THEMIS_ERROR(), message().

#### `bool isAvailable() const override`
- Source: `include/storage/blob_backend_gcs.h`:48
- Brief: Check if backend is available.
- Parameters: none
- Return: true if backend can be used
- Details: true if backend can be used

#### `std::string name() const override`
- Source: `include/storage/blob_backend_gcs.h`:47
- Brief: Get backend name.
- Parameters: none
- Return: Backend name (e.g., "filesystem", "s3", "webdav")
- Details: Backend name (e.g., "filesystem", "s3", "webdav")

#### `std::string objectName(const std::string &blob_id) const`
- Source: `include/storage/blob_backend_gcs.h`:55
- Brief: n/a
- Parameters:
  - `blob_id` (const std::string &): n/a

#### `Result< BlobRef > put(const std::string &blob_id, const std::vector< uint8_t > &data) override`
- Source: `include/storage/blob_backend_gcs.h`:42
- Brief: ───────────────────────────────────────────────────────────────────────────── IBlobStorageBackend interface ───────────────────────────────────────────────────────────────────────────── null_dereference/pointer_arithmetic/delete_no_nullptr scanner alerts (lines 91, 95, 104, 114, 129, 154, 214, 219, 239): all GCS API calls that dereference impl_->client are inside #ifdef THEMIS_ENABLE_GCS blocks that are only reached when impl_->available == true.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: blob_id Identifier of the blob. data Input parameter. Return value. impl_->available is set to true only after impl_->client is successfully constructed (see Impl::Impl()). impl_->client is therefore always non-null at these call sites. The "delete_no_nullptr" alert at line 219 misidentifies the GCS API method DeleteObject() as a raw pointer delete — false positives. ───────────────────────────────────────────────────────────────────────────── Calls: lock(), objectName(), WriteObject(), write(), data(), size(), Close(), metadata().

#### `Result< void > remove(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_gcs.h`:45
- Brief: Remove.
- Parameters:
  - `ref` (const BlobRef &): Input parameter.
- Return: Return value.
- Details: ref Input parameter. Return value. Calls: lock(), objectName(), DeleteObject(), ok(), THEMIS_ERROR(), message(), THEMIS_DEBUG(), OkVoid().

#### `~GCSBlobBackend() override`
- Source: `include/storage/blob_backend_gcs.h`:40
- Brief: n/a
- Parameters: none

### themis::storage::GCSBlobBackend::Impl

#### `Impl(const std::string &bucket_, const std::string &prefix_)`
- Source: `src/storage/blob_backend_gcs.cpp`:43
- Brief: n/a
- Parameters:
  - `bucket_` (const std::string &): n/a
  - `prefix_` (const std::string &): n/a

### themis::storage::GGUFMetadata

#### `GGUFMetadata()=default`
- Source: `include/storage/gguf_metadata.h`:113
- Brief: n/a
- Parameters: none

#### `void attach(const std::string &storage_key, const ProvenanceRecord &record)`
- Source: `include/storage/gguf_metadata.h`:125
- Brief: Attach provenance metadata to a storage key.
- Parameters:
  - `storage_key` (const std::string &): Input parameter.
  - `record` (const ProvenanceRecord &): Input parameter.
- Details: Attach. Overwrites any previously attached record for the same key. storage_key The key under which the tensor / adapter is stored. record Provenance record to attach. storage_key Input parameter. record Input parameter. Calls: lock().

#### `bool deserialize(const std::vector< uint8_t > &bytes)`
- Source: `include/storage/gguf_metadata.h`:224
- Brief: Deserialise records from a binary blob produced by serialize().
- Parameters:
  - `bytes` (const std::vector< uint8_t > &): Input parameter.
- Return: true on success, false if the blob is malformed.
- Details: Deserialize. bytes Binary blob. true on success, false if the blob is malformed. bytes Input parameter. True when the operation succeeds. Calls: empty(), data(), size(), readU32(), reserve(), readStr(), readI32(), std::move().

#### `bool detach(const std::string &storage_key)`
- Source: `include/storage/gguf_metadata.h`:133
- Brief: Remove the metadata entry for a storage key.
- Parameters:
  - `storage_key` (const std::string &): Input parameter.
- Return: true if the key was found and removed, false otherwise.
- Details: Detach. true if the key was found and removed, false otherwise. storage_key Input parameter. True when the operation succeeds. Calls: lock(), erase().

#### `bool has(const std::string &storage_key) const`
- Source: `include/storage/gguf_metadata.h`:148
- Brief: Test whether a provenance record exists for the given key.
- Parameters:
  - `storage_key` (const std::string &): n/a

#### `std::vector< std::string > keys() const`
- Source: `include/storage/gguf_metadata.h`:153
- Brief: Return all storage keys that have attached provenance records.
- Parameters: none

#### `std::optional< ProvenanceRecord > retrieve(const std::string &storage_key) const`
- Source: `include/storage/gguf_metadata.h`:143
- Brief: Retrieve the provenance record for a storage key.
- Parameters:
  - `storage_key` (const std::string &): n/a
- Return: The record if found, std::nullopt otherwise.
- Details: The record if found, std::nullopt otherwise.

#### `std::vector< uint8_t > serialize() const`
- Source: `include/storage/gguf_metadata.h`:216
- Brief: Serialise all stored records to a flat binary blob.
- Parameters: none
- Details: Format: uint32_t : number of records per record: uint32_t key_len + key bytes ProvenanceRecord fields (length-prefixed strings + int32_t fields)

#### `void setHmacFn(HmacFn fn)`
- Source: `include/storage/gguf_metadata.h`:179
- Brief: Inject a real HMAC implementation (thread-safe, process-global).
- Parameters:
  - `fn` (HmacFn): Input parameter.
- Details: static Once set, all subsequent calls to sign() and verify() will delegate to fn instead of using the built-in OpenSSL HMAC-SHA256 path. Pass a null fn to restore the built-in implementation. fn HMAC function, e.g. wrapping OpenSSL HMAC() with SHA-256. fn Input parameter. Calls: lk(), hmacFnMutex(), hmacFnStorage(), std::move().

#### `void sign(ProvenanceRecord &record, const std::string &hmac_key)`
- Source: `include/storage/gguf_metadata.h`:190
- Brief: Compute a signature tag and write it to record.hmac_signature.
- Parameters:
  - `record` (ProvenanceRecord &): Input/output parameter.
  - `hmac_key` (const std::string &): Input parameter.
- Details: Sign. Delegates to the injected HmacFn when one has been set via setHmacFn(); otherwise uses the built-in OpenSSL HMAC-SHA256 path. record Record to sign (modifies hmac_signature in-place). hmac_key Tenant-specific signing key (arbitrary bytes). record Input/output parameter. hmac_key Input parameter. Calls: canonicalBytes(), lk(), hmacFnMutex(), hmacFnStorage(), fn(), empty(), THEMIS_WARN(), clear().

#### `std::size_t size() const noexcept`
- Source: `include/storage/gguf_metadata.h`:229
- Brief: Total number of provenance records stored.
- Parameters: none

#### `bool verify(const ProvenanceRecord &record, const std::string &hmac_key)`
- Source: `include/storage/gguf_metadata.h`:202
- Brief: Verify that record.hmac_signature matches a freshly computed tag.
- Parameters:
  - `record` (const ProvenanceRecord &): Input parameter.
  - `hmac_key` (const std::string &): Input parameter.
- Return: true if the signature is valid, false otherwise.
- Details: Verify identity and enforce network policies for a request. Uses the same HMAC source (injected fn or built-in OpenSSL path) as sign(). record Record whose signature should be checked. hmac_key Tenant-specific signing key. true if the signature is valid, false otherwise. record Input parameter. hmac_key Input parameter. Verification result. Calls: empty(), canonicalBytes(), lk(), hmacFnMutex(), hmacFnStorage(), fn(), THEMIS_WARN(), constantTimeEquals().

### themis::storage::GdprFieldRegistry

#### `bool isProtected(const std::string &field_path) const`
- Source: `include/storage/schema_dead_weight_detector.h`:48
- Brief: n/a
- Parameters:
  - `field_path` (const std::string &): n/a

### themis::storage::GenericCompressionCodec

#### `Result< std::vector< uint8_t > > compressLZ4(const std::vector< uint8_t > &data)`
- Source: `include/storage/columnar_format.h`:174
- Brief: ============================================================================ Generic Compression Implementation (LZ4/Snappy) ============================================================================
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: empty(), size(), tl::unexpected(), Error(), LZ4_compressBound(), resize(), std::memcpy(), data().

#### `Result< std::vector< uint8_t > > compressSnappy(const std::vector< uint8_t > &data)`
- Source: `include/storage/columnar_format.h`:177
- Brief: Compress Snappy.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: empty(), size(), tl::unexpected(), Error(), snappy::MaxCompressedLength(), resize(), snappy::RawCompress(), data().

#### `Result< std::vector< uint8_t > > decompressLZ4(const std::vector< uint8_t > &compressed)`
- Source: `include/storage/columnar_format.h`:175
- Brief: Decompress LZ4.
- Parameters:
  - `compressed` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: compressed Input parameter. Return value. Calls: empty(), size(), tl::unexpected(), Error(), std::memcpy(), data(), resize(), LZ4_decompress_safe().

#### `Result< std::vector< uint8_t > > decompressSnappy(const std::vector< uint8_t > &compressed)`
- Source: `include/storage/columnar_format.h`:178
- Brief: Decompress Snappy.
- Parameters:
  - `compressed` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: compressed Input parameter. Return value. Calls: empty(), snappy::GetUncompressedLength(), data(), size(), tl::unexpected(), Error(), resize(), snappy::RawUncompress().

### themis::storage::GpuCompressionImpl

#### `GpuCompressionResult compress(const uint8_t *data, size_t size, GpuCompressionAlgorithm algorithm, const GpuCompressionConfig &cfg)=0`
- Source: `src/storage/gpu_compression.cpp`:108
- Brief: Compress.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
  - `algorithm` (GpuCompressionAlgorithm): Input parameter.
  - `cfg` (const GpuCompressionConfig &): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. algorithm Input parameter. cfg Input parameter. Return value.

#### `std::vector< GpuCompressionResult > compress_batch(const std::vector< const uint8_t * > &ptrs, const std::vector< size_t > &sizes, GpuCompressionAlgorithm algorithm, const GpuCompressionConfig &cfg)`
- Source: `src/storage/gpu_compression.cpp`:135
- Brief: Compress batch.
- Parameters:
  - `ptrs` (const std::vector< const uint8_t * > &): Input parameter.
  - `sizes` (const std::vector< size_t > &): Input parameter.
  - `algorithm` (GpuCompressionAlgorithm): Input parameter.
  - `cfg` (const GpuCompressionConfig &): Input parameter.
- Return: Return value.
- Details: ptrs Input parameter. sizes Input parameter. algorithm Input parameter. cfg Input parameter. Return value.

#### `std::vector< uint8_t > decompress(const std::vector< uint8_t > &data, GpuCompressionAlgorithm algorithm, size_t original_size, const GpuCompressionConfig &cfg)=0`
- Source: `src/storage/gpu_compression.cpp`:121
- Brief: Decompress.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
  - `algorithm` (GpuCompressionAlgorithm): Input parameter.
  - `original_size` (size_t): Input parameter.
  - `cfg` (const GpuCompressionConfig &): Input parameter.
- Return: Return value.
- Details: data Input parameter. algorithm Input parameter. original_size Input parameter. cfg Input parameter. Return value.

#### `bool initialize(const GpuCompressionConfig &cfg)=0`
- Source: `src/storage/gpu_compression.cpp`:89
- Brief: Initialize.
- Parameters:
  - `cfg` (const GpuCompressionConfig &): Input parameter.
- Return: True when the operation succeeds.
- Details: cfg Input parameter. True when the operation succeeds.

#### `bool is_available() const =0`
- Source: `src/storage/gpu_compression.cpp`:98
- Brief: Is available.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `void shutdown()=0`
- Source: `src/storage/gpu_compression.cpp`:93
- Brief: Shutdown.
- Parameters: none

#### `~GpuCompressionImpl()=default`
- Source: `src/storage/gpu_compression.cpp`:82
- Brief: Gpu Compression Impl.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::storage::GpuCompressionManager

#### `GpuCompressionManager(GpuCompressionManager &&) noexcept=delete`
- Source: `include/storage/gpu_compression.h`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (GpuCompressionManager &&): n/a

#### `GpuCompressionManager(const GpuCompressionConfig &config=GpuCompressionConfig{})`
- Source: `include/storage/gpu_compression.h`:202
- Brief: n/a
- Parameters:
  - `config` (const GpuCompressionConfig &): n/a

#### `GpuCompressionManager(const GpuCompressionManager &)=delete`
- Source: `include/storage/gpu_compression.h`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GpuCompressionManager &): n/a

#### `std::string accel_type_to_string(GpuAccelerationType type)`
- Source: `include/storage/gpu_compression.h`:339
- Brief: Accel type to string.
- Parameters:
  - `type` (GpuAccelerationType): Input parameter.
- Return: Return value.
- Details: type Input parameter. Return value.

#### `GpuAccelerationType active_accel_type() const`
- Source: `include/storage/gpu_compression.h`:295
- Brief: Returns the actually active acceleration type.
- Parameters: none

#### `std::string algorithm_to_string(GpuCompressionAlgorithm algorithm)`
- Source: `include/storage/gpu_compression.h`:338
- Brief: Algorithm to string.
- Parameters:
  - `algorithm` (GpuCompressionAlgorithm): Input parameter.
- Return: Return value.
- Details: algorithm Input parameter. Return value.

#### `GpuCompressionResult compress(const std::vector< uint8_t > &data, GpuCompressionAlgorithm algorithm)`
- Source: `include/storage/gpu_compression.h`:228
- Brief: Compress data using the specified algorithm.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
  - `algorithm` (GpuCompressionAlgorithm): Input parameter.
- Return: Return value.
- Details: Compress. Automatically routes to the GPU path when: GPU was initialised successfully, AND data.size() >= config.min_size_for_gpu, AND !force_cpu_ flag is set. Falls back to the CPU implementation otherwise. data Input parameter. algorithm Input parameter. Return value.

#### `GpuCompressionResult compress(const uint8_t *data, size_t size, GpuCompressionAlgorithm algorithm)`
- Source: `include/storage/gpu_compression.h`:234
- Brief: Convenience overload accepting a raw pointer.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
  - `algorithm` (GpuCompressionAlgorithm): Input parameter.
- Return: Return value.
- Details: Compress. data Input parameter. size Input parameter. algorithm Input parameter. Return value.

#### `std::vector< GpuCompressionResult > compress_batch(const std::vector< std::vector< uint8_t > > &buffers, GpuCompressionAlgorithm algorithm)`
- Source: `include/storage/gpu_compression.h`:273
- Brief: Compress multiple independent buffers in a single GPU dispatch.
- Parameters:
  - `buffers` (const std::vector< std::vector< uint8_t > > &): Input parameter.
  - `algorithm` (GpuCompressionAlgorithm): Input parameter.
- Return: One GpuCompressionResult per input buffer, in the same order.
- Details: Compress batch. When a GPU backend is available all buffers are transferred to the device and compressed in a single nvCOMP batched call (one kernel launch), then the results are copied back in one pass. This amortises host-device transfer overhead and is the primary mechanism through which 5-10× throughput is achieved versus sequential per-buffer GPU calls. When no GPU is available the implementation falls back to sequential CPU compression (one buffer at a time). buffers Input buffers (may have different sizes). algorithm Algorithm to use for all buffers. One GpuCompressionResult per input buffer, in the same order. buffers Input parameter. algorithm Input parameter. Return value.

#### `GpuCompressionResult cpu_compress_lz4(const uint8_t *data, size_t size)`
- Source: `include/storage/gpu_compression.h`:353
- Brief: Cpu compress lz4.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value.

#### `GpuCompressionResult cpu_compress_snappy(const uint8_t *data, size_t size)`
- Source: `include/storage/gpu_compression.h`:352
- Brief: Cpu compress snappy.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value.

#### `GpuCompressionResult cpu_compress_zstd(const uint8_t *data, size_t size)`
- Source: `include/storage/gpu_compression.h`:351
- Brief: --------------------------------------------------------------- Zstd (reuses existing zstd_codec utility) ---------------------------------------------------------------
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. size Input parameter. Return value.

#### `std::vector< uint8_t > cpu_decompress_gpu_container(const std::vector< uint8_t > &compressed, GpuCompressionAlgorithm algorithm)`
- Source: `include/storage/gpu_compression.h`:365
- Brief: ============================================================================ CPU-side GPU-container decoder When data was compressed via the CUDA/nvCOMP path and needs to be decompressed on a CPU-only node (or after GPU failure), this helper parses the GPU container format and decompresses each chunk using the corresponding native CPU library.
- Parameters:
  - `compressed` (const std::vector< uint8_t > &): Input parameter.
  - `algorithm` (GpuCompressionAlgorithm): Input parameter.
- Return: Return value.
- Details: CPU-side decoder for data produced by the GPU (nvCOMP) path. Parses the GPU container format and decompresses each chunk with the corresponding native CPU library. compressed Input parameter. algorithm Input parameter. Return value. nvCOMP uses standard-compatible output for all three algorithms (LZ4 block, Snappy stream, Zstd frame), so the native CPU libraries can decompress them without modification. ============================================================================

#### `std::vector< uint8_t > cpu_decompress_lz4(const std::vector< uint8_t > &data, size_t original_size)`
- Source: `include/storage/gpu_compression.h`:359
- Brief: Cpu decompress lz4.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
  - `original_size` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. original_size Input parameter. Return value.

#### `std::vector< uint8_t > cpu_decompress_snappy(const std::vector< uint8_t > &data, size_t original_size)`
- Source: `include/storage/gpu_compression.h`:357
- Brief: Cpu decompress snappy.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
  - `original_size` (size_t): n/a
- Return: Return value.
- Details: data Input parameter. size_t Input parameter. Return value.

#### `std::vector< uint8_t > cpu_decompress_zstd(const std::vector< uint8_t > &data, size_t original_size)`
- Source: `include/storage/gpu_compression.h`:355
- Brief: Cpu decompress zstd.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
  - `original_size` (size_t): n/a
- Return: Return value.
- Details: data Input parameter. size_t Input parameter. Return value.

#### `std::vector< uint8_t > decompress(const std::vector< uint8_t > &compressed_data, GpuCompressionAlgorithm algorithm, size_t original_size=0)`
- Source: `include/storage/gpu_compression.h`:247
- Brief: Decompress compressed_data that was produced by compress().
- Parameters:
  - `compressed_data` (const std::vector< uint8_t > &): n/a
  - `algorithm` (GpuCompressionAlgorithm): Input parameter.
  - `original_size` (size_t): Input parameter.
- Return: Decompressed bytes, or empty vector on failure.
- Details: Decompress. Uses the same GPU/CPU routing logic as compress(). Decompressed bytes, or empty vector on failure. compressed Input parameter. algorithm Input parameter. original_size Input parameter. Return value.

#### `std::vector< std::vector< uint8_t > > decompress_batch(const std::vector< std::vector< uint8_t > > &compressed_buffers, GpuCompressionAlgorithm algorithm, const std::vector< size_t > &original_sizes={})`
- Source: `include/storage/gpu_compression.h`:281
- Brief: Decompress multiple buffers in a single GPU dispatch.
- Parameters:
  - `compressed_buffers` (const std::vector< std::vector< uint8_t > > &): Input parameter.
  - `algorithm` (GpuCompressionAlgorithm): Input parameter.
  - `original_sizes` (const std::vector< size_t > &): Input parameter.
- Return: Return value.
- Details: Decompress batch. compressed_buffers Input parameter. algorithm Input parameter. original_sizes Input parameter. Return value.

#### `void force_cpu_fallback(bool enable)`
- Source: `include/storage/gpu_compression.h`:298
- Brief: Force CPU-only mode (useful for testing).
- Parameters:
  - `enable` (bool): Input parameter.
- Details: Force cpu fallback. enable Input parameter.

#### `const GpuCompressionConfig & get_config() const`
- Source: `include/storage/gpu_compression.h`:304
- Brief: n/a
- Parameters: none

#### `Stats get_stats() const`
- Source: `include/storage/gpu_compression.h`:331
- Brief: n/a
- Parameters: none

#### `bool init_gpu()`
- Source: `include/storage/gpu_compression.h`:345
- Brief: Init gpu.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `bool is_gpu_available() const`
- Source: `include/storage/gpu_compression.h`:292
- Brief: Returns true if a GPU backend is initialised and operational.
- Parameters: none

#### `GpuCompressionManager & operator=(GpuCompressionManager &&) noexcept=delete`
- Source: `include/storage/gpu_compression.h`:212
- Brief: n/a
- Parameters:
  - `<unnamed>` (GpuCompressionManager &&): n/a

#### `GpuCompressionManager & operator=(const GpuCompressionManager &)=delete`
- Source: `include/storage/gpu_compression.h`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GpuCompressionManager &): n/a

#### `void reset_stats()`
- Source: `include/storage/gpu_compression.h`:332
- Brief: Reset stats.
- Parameters: none

#### `void set_config(const GpuCompressionConfig &cfg)`
- Source: `include/storage/gpu_compression.h`:308
- Brief: Set config.
- Parameters:
  - `cfg` (const GpuCompressionConfig &): Input parameter.
- Details: cfg Input parameter.

#### `bool should_use_gpu(size_t data_size) const`
- Source: `include/storage/gpu_compression.h`:346
- Brief: n/a
- Parameters:
  - `data_size` (size_t): n/a

#### `~GpuCompressionManager()`
- Source: `include/storage/gpu_compression.h`:206
- Brief: n/a
- Parameters: none

### themis::storage::GpuCompressionResult

#### `GpuCompressionResult()`
- Source: `include/storage/gpu_compression.h`:164
- Brief: n/a
- Parameters: none

### themis::storage::HierarchicalTuckerDecomposer

#### `HierarchicalTuckerDecomposer(HTConfig cfg={}) noexcept`
- Source: `include/storage/hierarchical_tucker_decomposer.h`:50
- Brief: n/a
- Parameters:
  - `cfg` (HTConfig): n/a

#### `std::unique_ptr< tensor::HTNode > buildHTNode(const std::vector< float > &core, const std::vector< std::size_t > &core_shape, std::size_t L, std::size_t R, const std::vector< std::vector< float > > &U_cache, const std::vector< std::size_t > &T_shape) const`
- Source: `include/storage/hierarchical_tucker_decomposer.h`:130
- Brief: Recursive top-down HT node construction from a "core" tensor.
- Parameters:
  - `core` (const std::vector< float > &): Flat core tensor of shape [phys_{L},...,phys_{R-1}, r_out] (row-major).
  - `core_shape` (const std::vector< std::size_t > &): Shape of core (length = (R-L) + 1, last elem = r_out).
  - `L` (std::size_t): Mode range [L, R) covered by this subtree.
  - `R` (std::size_t): Mode range [L, R) covered by this subtree.
  - `U_cache` (const std::vector< std::vector< float > > &): HOSVD leaf bases indexed by physical mode.
  - `T_shape` (const std::vector< std::size_t > &): Original tensor shape (for leaf n_k).
- Details: core Flat core tensor of shape [phys_{L},...,phys_{R-1}, r_out] (row-major). core_shape Shape of core (length = (R-L) + 1, last elem = r_out). L R Mode range [L, R) covered by this subtree. U_cache HOSVD leaf bases indexed by physical mode. T_shape Original tensor shape (for leaf n_k).

#### `std::pair< tensor::HTTrain, Stats > decompose(const std::vector< float > &data, const std::vector< std::size_t > &shape) const`
- Source: `include/storage/hierarchical_tucker_decomposer.h`:77
- Brief: Decompose a flat dense tensor into HT format.
- Parameters:
  - `data` (const std::vector< float > &): Flat row-major tensor data (length = ∏ shape[k]).
  - `shape` (const std::vector< std::size_t > &): Mode sizes; length d ≥ 2; each shape[k] ≥ 1.
- Return: {HTTrain, Stats}
- Throws:
  - std::invalid_argument: if data.size() != ∏ shape[k], d < 2, or any shape[k] == 0.
- Details: data Flat row-major tensor data (length = ∏ shape[k]). shape Mode sizes; length d ≥ 2; each shape[k] ≥ 1. {HTTrain, Stats} std::invalid_argument if data.size() != ∏ shape[k], d < 2, or any shape[k] == 0. Stub #287 resolved: HOOI alternating optimization loop added after HOSVD initialization in decompose() (see hierarchical_tucker_decomposer.cpp, Step 2b). Iterates until ‖G‖_F converges (rel. change < 1e-6) or 20 sweeps complete. Long-term plan (Q2 2028): extend ITensorIndex to support HT directly.

#### `std::vector< float > modeKProduct(const std::vector< float > &data, const std::vector< std::size_t > &shape, std::size_t mode_k, const std::vector< float > &U, std::size_t n_k, std::size_t r)`
- Source: `include/storage/hierarchical_tucker_decomposer.h`:112
- Brief: ── Mode-k product: T ×_k U^T ────────────────────────────────────────────────
- Parameters:
  - `data` (const std::vector< float > &): Input parameter.
  - `shape` (const std::vector< std::size_t > &): Input parameter.
  - `mode_k` (std::size_t): Input parameter.
  - `U` (const std::vector< float > &): Input parameter.
  - `n_k` (std::size_t): Input parameter.
  - `r` (std::size_t): Input parameter.
- Return: Return value.
- Details: Mode-k product: applies U^T (r × n_k) to mode k of tensor data. Returns a tensor with shape[k] replaced by r. data Input parameter. shape Input parameter. mode_k Input parameter. U Input parameter. n_k Input parameter. r Input parameter. Return value.

#### `std::vector< float > modeKUnfolding(const std::vector< float > &data, const std::vector< std::size_t > &shape, std::size_t mode_k)`
- Source: `include/storage/hierarchical_tucker_decomposer.h`:86
- Brief: Mode-k unfolding of a flat tensor: returns matrix T_(k) ∈ ℝ^{n_k × (N/n_k)}.
- Parameters:
  - `data` (const std::vector< float > &): Input parameter.
  - `shape` (const std::vector< std::size_t > &): Input parameter.
  - `mode_k` (std::size_t): Input parameter.
- Return: Return value.
- Details: ── Mode-k unfolding ────────────────────────────────────────────────────────── data Input parameter. shape Input parameter. mode_k Input parameter. Return value.

#### `void truncatedSVD(const std::vector< float > &mat, std::size_t m, std::size_t n, double delta, std::size_t max_rank_cap, std::vector< float > &U_out, std::vector< float > &S_out, std::vector< float > &Vt_out, std::size_t &rank_out)`
- Source: `include/storage/hierarchical_tucker_decomposer.h`:99
- Brief: Truncated SVD of an m×n matrix A.
- Parameters:
  - `mat` (const std::vector< float > &): Input parameter.
  - `m` (std::size_t): Input parameter.
  - `n` (std::size_t): Input parameter.
  - `delta` (double): Input parameter.
  - `max_rank_cap` (std::size_t): Input parameter.
  - `U_out` (std::vector< float > &): Input/output parameter.
  - `S_out` (std::vector< float > &): Input/output parameter.
  - `Vt_out` (std::vector< float > &): Input/output parameter.
  - `rank_out` (std::size_t &): Input/output parameter.
- Details: ── Truncated SVD ───────────────────────────────────────────────────────────── Returns U (m × r), S (r), Vt (r × n) where r is chosen such that sigma[r] < delta (or r = max_rank if the threshold is never reached). Uses the shared TensorTrainDecomposer::truncatedSVD() backend. mat Input parameter. m Input parameter. n Input parameter. delta Input parameter. max_rank_cap Input parameter. U_out Input/output parameter. S_out Input/output parameter. Vt_out Input/output parameter. rank_out Input/output parameter.

### themis::storage::IBlobStorageBackend

#### `bool exists(const BlobRef &ref)=0`
- Source: `include/storage/blob_storage_backend.h`:100
- Brief: Check if blob exists.
- Parameters:
  - `ref` (const BlobRef &): Blob reference
- Return: true if exists
- Details: ref Blob reference true if exists

#### `Result< std::vector< uint8_t > > get(const BlobRef &ref)=0`
- Source: `include/storage/blob_storage_backend.h`:84
- Brief: Retrieve a blob.
- Parameters:
  - `ref` (const BlobRef &): Blob reference
- Return: Result<vector<uint8_t>> Blob data or error if not found
- Details: ref Blob reference Result<vector<uint8_t>> Blob data or error if not found

#### `bool isAvailable() const =0`
- Source: `include/storage/blob_storage_backend.h`:112
- Brief: Check if backend is available.
- Parameters: none
- Return: true if backend can be used
- Details: true if backend can be used

#### `std::string name() const =0`
- Source: `include/storage/blob_storage_backend.h`:106
- Brief: Get backend name.
- Parameters: none
- Return: Backend name (e.g., "filesystem", "s3", "webdav")
- Details: Backend name (e.g., "filesystem", "s3", "webdav")

#### `Result< BlobRef > put(const std::string &blob_id, const std::vector< uint8_t > &data)=0`
- Source: `include/storage/blob_storage_backend.h`:74
- Brief: Store a blob.
- Parameters:
  - `blob_id` (const std::string &): Unique blob identifier
  - `data` (const std::vector< uint8_t > &): Blob data
- Return: Result<BlobRef> Reference to stored blob or error
- Details: blob_id Unique blob identifier data Blob data Result<BlobRef> Reference to stored blob or error

#### `Result< void > remove(const BlobRef &ref)=0`
- Source: `include/storage/blob_storage_backend.h`:93
- Brief: Delete a blob.
- Parameters:
  - `ref` (const BlobRef &): Blob reference
- Return: Result<bool> Success or error
- Details: ref Blob reference Result<bool> Success or error

#### `~IBlobStorageBackend()=default`
- Source: `include/storage/blob_storage_backend.h`:66
- Brief: n/a
- Parameters: none

### themis::storage::IDistributedShardParticipant

#### `void abort(const std::string &txn_id)=0`
- Source: `include/storage/distributed_transaction_manager.h`:111
- Brief: Phase 2 (abort path) — discard prepared operations and release locks.
- Parameters:
  - `txn_id` (const std::string &): Transaction to abort.
- Details: txn_id Transaction to abort.

#### `void commit(const std::string &txn_id)=0`
- Source: `include/storage/distributed_transaction_manager.h`:104
- Brief: Phase 2 (commit path) — apply the prepared operations.
- Parameters:
  - `txn_id` (const std::string &): Transaction to commit.
- Details: Called only when every participant voted COMMIT. txn_id Transaction to commit.

#### `std::optional< std::string > get(const std::string &)`
- Source: `include/storage/distributed_transaction_manager.h`:119
- Brief: Optional point-in-time read of a single key.
- Parameters:
  - `<unnamed>` (const std::string &): n/a
- Details: Used by DistributedTransaction::get() for cross-shard reads. The default implementation returns std::nullopt (key not found).

#### `bool prepare(const std::string &txn_id, const std::vector< DistributedOperation > &ops)=0`
- Source: `include/storage/distributed_transaction_manager.h`:92
- Brief: Phase 1 — lock rows and record a durable PREPARE log entry.
- Parameters:
  - `txn_id` (const std::string &): Globally unique transaction identifier.
  - `ops` (const std::vector< DistributedOperation > &): Operations destined for this shard.
- Return: true → vote COMMIT; false → vote ABORT.
- Details: txn_id Globally unique transaction identifier. ops Operations destined for this shard. true → vote COMMIT; false → vote ABORT.

#### `~IDistributedShardParticipant()=default`
- Source: `include/storage/distributed_transaction_manager.h`:83
- Brief: n/a
- Parameters: none

### themis::storage::IEncryptionKeyProvider

#### `std::array< uint8_t, 32 > currentKey() const =0`
- Source: `include/storage/encrypted_blob_backend.h`:46
- Brief: Return the current 256-bit (32-byte) encryption key.
- Parameters: none
- Details: The returned array must remain valid for the duration of the encryption/decryption call.

#### `std::string name() const =0`
- Source: `include/storage/encrypted_blob_backend.h`:51
- Brief: Human-readable key provider name (e.g., "static", "kms", "hsm").
- Parameters: none

#### `~IEncryptionKeyProvider()=default`
- Source: `include/storage/encrypted_blob_backend.h`:38
- Brief: n/a
- Parameters: none

### themis::storage::ITensorStorageBackend

#### `bool del(const std::string &key)=0`
- Source: `include/storage/tensor_network_storage_engine.h`:119
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::optional< std::vector< uint8_t > > get(const std::string &key) const =0`
- Source: `include/storage/tensor_network_storage_engine.h`:117
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::vector< std::string > listKeys(const std::string &prefix) const =0`
- Source: `include/storage/tensor_network_storage_engine.h`:123
- Brief: Iterate over all keys with the given prefix.
- Parameters:
  - `prefix` (const std::string &): n/a

#### `bool put(const std::string &key, const std::vector< uint8_t > &value)=0`
- Source: `include/storage/tensor_network_storage_engine.h`:113
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::vector< uint8_t > &): n/a

#### `~ITensorStorageBackend()=default`
- Source: `include/storage/tensor_network_storage_engine.h`:111
- Brief: n/a
- Parameters: none

### themis::storage::IVectorIndexBackend

#### `void add(const std::string &id, const std::vector< float > &embedding)=0`
- Source: `include/storage/vector_index_backend.h`:109
- Brief: Add (or replace) a vector in the index.
- Parameters:
  - `id` (const std::string &): Unique document identifier.
  - `embedding` (const std::vector< float > &): Embedding vector; length must equal config().dim.
- Throws:
  - std::invalid_argument: if embedding.size() != config().dim.
- Details: If id already exists the entry is updated atomically. id Unique document identifier. embedding Embedding vector; length must equal config().dim. std::invalid_argument if embedding.size() != config().dim.

#### `const VectorIndexConfig & config() const noexcept=0`
- Source: `include/storage/vector_index_backend.h`:140
- Brief: Configuration used when constructing this backend.
- Parameters: none

#### `std::string name() const =0`
- Source: `include/storage/vector_index_backend.h`:145
- Brief: Human-readable backend name (e.g., "in_memory", "hnswlib", "faiss").
- Parameters: none

#### `void remove(const std::string &id)=0`
- Source: `include/storage/vector_index_backend.h`:130
- Brief: Remove a vector from the index.
- Parameters:
  - `id` (const std::string &): Document identifier to remove.
- Details: Silently ignores non-existent ids. id Document identifier to remove.

#### `std::vector< KnnResult > search(const std::vector< float > &query, std::size_t k) const =0`
- Source: `include/storage/vector_index_backend.h`:121
- Brief: Query the index for the k nearest neighbours.
- Parameters:
  - `query` (const std::vector< float > &): Embedding vector; length must equal config().dim.
  - `k` (std::size_t): Number of nearest neighbours to return (≥ 1).
- Return: Up to min(k, size()) results, sorted by ascending distance.
- Throws:
  - std::invalid_argument: if query.size() != config().dim.
- Details: query Embedding vector; length must equal config().dim. k Number of nearest neighbours to return (≥ 1). Up to min(k, size()) results, sorted by ascending distance. std::invalid_argument if query.size() != config().dim.

#### `std::size_t size() const noexcept=0`
- Source: `include/storage/vector_index_backend.h`:135
- Brief: Number of vectors currently stored.
- Parameters: none

#### `~IVectorIndexBackend()=default`
- Source: `include/storage/vector_index_backend.h`:98
- Brief: n/a
- Parameters: none

### themis::storage::InMemoryTensorBackend

#### `bool del(const std::string &key) override`
- Source: `include/storage/tensor_network_storage_engine.h`:139
- Brief: Del.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds. Calls: lk(), erase().

#### `std::optional< std::vector< uint8_t > > get(const std::string &key) const override`
- Source: `include/storage/tensor_network_storage_engine.h`:137
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::vector< std::string > listKeys(const std::string &prefix) const override`
- Source: `include/storage/tensor_network_storage_engine.h`:142
- Brief: Iterate over all keys with the given prefix.
- Parameters:
  - `prefix` (const std::string &): n/a

#### `bool put(const std::string &key, const std::vector< uint8_t > &value) override`
- Source: `include/storage/tensor_network_storage_engine.h`:133
- Brief: Put.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. value Input parameter. True when the operation succeeds. Calls: lk().

### themis::storage::InMemoryVectorIndex

#### `InMemoryVectorIndex(const VectorIndexConfig &cfg)`
- Source: `include/storage/vector_index_backend.h`:167
- Brief: Construct an in-memory index.
- Parameters:
  - `cfg` (const VectorIndexConfig &): Configuration; cfg.dim must be > 0.
- Throws:
  - std::invalid_argument: if cfg.dim == 0.
- Details: cfg Configuration; cfg.dim must be > 0. std::invalid_argument if cfg.dim == 0.

#### `void add(const std::string &id, const std::vector< float > &embedding) override`
- Source: `include/storage/vector_index_backend.h`:169
- Brief: Add.
- Parameters:
  - `id` (const std::string &): Input parameter.
  - `embedding` (const std::vector< float > &): Input parameter.
- Details: id Input parameter. embedding Input parameter.

#### `float computeDistance(const std::vector< float > &a, const std::vector< float > &b) const noexcept`
- Source: `include/storage/vector_index_backend.h`:185
- Brief: Compute raw distance between two embeddings according to cfg_.metric.
- Parameters:
  - `a` (const std::vector< float > &): n/a
  - `b` (const std::vector< float > &): n/a

#### `const VectorIndexConfig & config() const noexcept override`
- Source: `include/storage/vector_index_backend.h`:179
- Brief: Configuration used when constructing this backend.
- Parameters: none

#### `std::string name() const override`
- Source: `include/storage/vector_index_backend.h`:181
- Brief: Human-readable backend name (e.g., "in_memory", "hnswlib", "faiss").
- Parameters: none

#### `void normalise(std::vector< float > &v) noexcept`
- Source: `include/storage/vector_index_backend.h`:189
- Brief: Normalise a vector in-place (L2 norm); no-op if norm is zero.
- Parameters:
  - `v` (std::vector< float > &): n/a

#### `void remove(const std::string &id) override`
- Source: `include/storage/vector_index_backend.h`:175
- Brief: Remove.
- Parameters:
  - `id` (const std::string &): Input parameter.
- Details: id Input parameter.

#### `std::vector< KnnResult > search(const std::vector< float > &query, std::size_t k) const override`
- Source: `include/storage/vector_index_backend.h`:173
- Brief: Query the index for the k nearest neighbours.
- Parameters:
  - `query` (const std::vector< float > &): Embedding vector; length must equal config().dim.
  - `k` (std::size_t): Number of nearest neighbours to return (≥ 1).
- Return: Up to min(k, size()) results, sorted by ascending distance.
- Throws:
  - std::invalid_argument: if query.size() != config().dim.
- Details: query Embedding vector; length must equal config().dim. k Number of nearest neighbours to return (≥ 1). Up to min(k, size()) results, sorted by ascending distance. std::invalid_argument if query.size() != config().dim.

#### `std::size_t size() const noexcept override`
- Source: `include/storage/vector_index_backend.h`:177
- Brief: Number of vectors currently stored.
- Parameters: none

#### `float toScore(float distance) const noexcept`
- Source: `include/storage/vector_index_backend.h`:192
- Brief: Convert raw distance to a [0,1] similarity score.
- Parameters:
  - `distance` (float): n/a

### themis::storage::MmapBlobView

#### `MmapBlobView(MmapBlobView &&) noexcept`
- Source: `include/storage/zero_copy_blob_transfer.h`:76
- Brief: n/a
- Parameters:
  - `other` (MmapBlobView &&): n/a

#### `MmapBlobView(const MmapBlobView &)=delete`
- Source: `include/storage/zero_copy_blob_transfer.h`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MmapBlobView &): n/a

#### `MmapBlobView(const std::string &file_path, bool sequential_hint=true)`
- Source: `include/storage/zero_copy_blob_transfer.h`:67
- Brief: Map the file at file_path for read-only access.
- Parameters:
  - `file_path` (const std::string &): Absolute path to the blob file.
  - `sequential_hint` (bool): When true, hints the kernel via MADV_SEQUENTIAL for forward-sequential access patterns.
- Details: file_path Absolute path to the blob file. sequential_hint When true, hints the kernel via MADV_SEQUENTIAL for forward-sequential access patterns. If the mapping fails (file not found, permission denied, etc.) the object is left in an invalid state; check valid() before accessing data().

#### `const uint8_t * data() const noexcept`
- Source: `include/storage/zero_copy_blob_transfer.h`:82
- Brief: n/a
- Parameters: none
- Return: Pointer to the first mapped byte, or nullptr if !valid().
- Details: Pointer to the first mapped byte, or nullptr if !valid().

#### `void forEach(size_t chunk_size, std::function< bool(const uint8_t *, size_t)> callback) const`
- Source: `include/storage/zero_copy_blob_transfer.h`:101
- Brief: Iterate over the mapped data in fixed-size chunks.
- Parameters:
  - `chunk_size` (size_t): Maximum bytes per callback invocation.
  - `callback` (std::function< bool(const uint8_t *, size_t)>): Called with (ptr, len) for each chunk. Return false to stop early.
- Details: chunk_size Maximum bytes per callback invocation. callback Called with (ptr, len) for each chunk. Return false to stop early.

#### `MmapBlobView & operator=(MmapBlobView &&) noexcept`
- Source: `include/storage/zero_copy_blob_transfer.h`:77
- Brief: n/a
- Parameters:
  - `other` (MmapBlobView &&): n/a

#### `MmapBlobView & operator=(const MmapBlobView &)=delete`
- Source: `include/storage/zero_copy_blob_transfer.h`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MmapBlobView &): n/a

#### `void releaseResources() noexcept`
- Source: `include/storage/zero_copy_blob_transfer.h`:105
- Brief: n/a
- Parameters: none

#### `size_t size() const noexcept`
- Source: `include/storage/zero_copy_blob_transfer.h`:87
- Brief: n/a
- Parameters: none
- Return: Number of mapped bytes (0 if !valid()).
- Details: Number of mapped bytes (0 if !valid()).

#### `bool valid() const noexcept`
- Source: `include/storage/zero_copy_blob_transfer.h`:92
- Brief: n/a
- Parameters: none
- Return: true when the file is successfully mapped and data() is usable.
- Details: true when the file is successfully mapped and data() is usable.

#### `~MmapBlobView()`
- Source: `include/storage/zero_copy_blob_transfer.h`:70
- Brief: n/a
- Parameters: none

### themis::storage::NVMeManager

#### `NVMeManager(NVMeManager &&)=delete`
- Source: `include/storage/nvme_manager.h`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (NVMeManager &&): n/a

#### `NVMeManager(const NVMeConfig &config={})`
- Source: `include/storage/nvme_manager.h`:156
- Brief: n/a
- Parameters:
  - `config` (const NVMeConfig &): n/a

#### `NVMeManager(const NVMeManager &)=delete`
- Source: `include/storage/nvme_manager.h`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (const NVMeManager &): n/a

#### `const NVMeConfig & config() const noexcept`
- Source: `include/storage/nvme_manager.h`:278
- Brief: Return the active configuration.
- Parameters: none

#### `NVMeCapabilities detectCapabilities() const`
- Source: `include/storage/nvme_manager.h`:187
- Brief: Probe the host for NVMe / kernel capabilities.
- Parameters: none
- Details: Result is cached after the first call.

#### `uint32_t detectedQueueCount() const noexcept`
- Source: `include/storage/nvme_manager.h`:197
- Brief: Return the number of hardware I/O queue pairs detected.
- Parameters: none

#### `bool finishZone(uint64_t zone_offset)`
- Source: `include/storage/nvme_manager.h`:247
- Brief: Mark a zone as full (no further writes until reset).
- Parameters:
  - `zone_offset` (uint64_t): Input parameter.
- Return: true on success.
- Details: Finish Zone. zone_offset Byte offset of the zone start. true on success. zone_offset Input parameter. True when the operation succeeds. Calls: empty(), THEMIS_WARN(), lock(), open(), c_str(), THEMIS_ERROR(), std::strerror(), ioctl().

#### `uint64_t getZoneWritePointer(uint64_t zone_offset) const`
- Source: `include/storage/nvme_manager.h`:255
- Brief: Query the write pointer (current append position) of a zone.
- Parameters:
  - `zone_offset` (uint64_t): Byte offset of the zone start.
- Return: Write pointer byte offset, or UINT64_MAX on error.
- Details: zone_offset Byte offset of the zone start. Write pointer byte offset, or UINT64_MAX on error.

#### `bool initialize()`
- Source: `include/storage/nvme_manager.h`:173
- Brief: Initialise the manager and set up the io_uring ring if enabled.
- Parameters: none
- Return: True when the operation succeeds.
- Details: Initialize. Always returns true. If io_uring or ZNS are requested but unavailable, those features are silently disabled (with a WARN log) and the manager continues operating in degraded mode. True when the operation succeeds. Calls: load(), state_lock(), THEMIS_INFO(), detectCapabilities(), THEMIS_WARN(), setupIoUring(), store().

#### `bool isIoUringActive() const noexcept`
- Source: `include/storage/nvme_manager.h`:192
- Brief: Return whether io_uring was successfully initialised.
- Parameters: none

#### `NVMeManager & operator=(NVMeManager &&)=delete`
- Source: `include/storage/nvme_manager.h`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (NVMeManager &&): n/a

#### `NVMeManager & operator=(const NVMeManager &)=delete`
- Source: `include/storage/nvme_manager.h`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (const NVMeManager &): n/a

#### `int pollCompletions(std::vector< NVMeIOResult > &results, uint32_t min_complete=0)`
- Source: `include/storage/nvme_manager.h`:228
- Brief: Flush pending submissions and collect completions.
- Parameters:
  - `results` (std::vector< NVMeIOResult > &): Input/output parameter.
  - `min_complete` (uint32_t): Input parameter.
- Return: Number of completions harvested, or -1 on error.
- Details: Poll Completions. results Output vector populated with completed I/O results. min_complete Minimum completions to wait for (0 = non-blocking). Number of completions harvested, or -1 on error. results Input/output parameter. min_complete Input parameter. Return value. Calls: clear(), isIoUringActive(), ring_lk(), get(), themis_io_uring_enter(), THEMIS_ERROR(), std::strerror(), __atomic_load_n().

#### `bool probeIoUringKernel() const`
- Source: `include/storage/nvme_manager.h`:298
- Brief: n/a
- Parameters: none

#### `uint32_t readHwQueueCount() const`
- Source: `include/storage/nvme_manager.h`:299
- Brief: n/a
- Parameters: none

#### `uint32_t recommendedBackgroundThreads() const`
- Source: `include/storage/nvme_manager.h`:273
- Brief: Recommended number of RocksDB background I/O threads.
- Parameters: none
- Details: Returns min(detected_hw_queue_count * 2, 16) as a practical upper bound.

#### `std::pair< bool, bool > recommendedDirectIOFlags() const`
- Source: `include/storage/nvme_manager.h`:266
- Brief: Populate a RocksDB-compatible Direct I/O flag summary.
- Parameters: none
- Details: Returns a pair of booleans representing { use_direct_reads, use_direct_io_for_flush_and_compaction } adjusted to reflect only the flags that the detected device supports.

#### `bool resetZone(uint64_t zone_offset)`
- Source: `include/storage/nvme_manager.h`:239
- Brief: Reset a zone (erase all data, mark zone as empty).
- Parameters:
  - `zone_offset` (uint64_t): Input parameter.
- Return: true on success.
- Details: Reset Zone. zone_offset Byte offset of the zone start (must be zone-aligned). true on success. zone_offset Input parameter. True when the operation succeeds. Calls: empty(), THEMIS_WARN(), lock(), open(), c_str(), THEMIS_ERROR(), std::strerror(), ioctl().

#### `bool setupIoUring()`
- Source: `include/storage/nvme_manager.h`:296
- Brief: Setup Io Uring.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Calls: ring_lock(), get(), themis_io_uring_setup(), THEMIS_ERROR(), std::strerror(), mmap(), close(), munmap().

#### `void shutdown()`
- Source: `include/storage/nvme_manager.h`:178
- Brief: Release all resources (io_uring ring, registered buffers/files).
- Parameters: none
- Details: Shutdown. Calls: load(), THEMIS_INFO(), teardownIoUring(), store().

#### `bool submitRead(const NVMeIORequest &req)`
- Source: `include/storage/nvme_manager.h`:209
- Brief: Submit an asynchronous read request.
- Parameters:
  - `req` (const NVMeIORequest &): Input parameter.
- Return: true if the request was enqueued; false on ring overflow or error.
- Details: Submit Read. req I/O request descriptor. true if the request was enqueued; false on ring overflow or error. Falls back to synchronous pread() when io_uring is unavailable. req Input parameter. True when the operation succeeds. Calls: isIoUringActive(), ring_lk(), get(), __atomic_load_n(), THEMIS_WARN(), std::memset(), __atomic_store_n(), themis_io_uring_enter().

#### `bool submitWrite(const NVMeIORequest &req)`
- Source: `include/storage/nvme_manager.h`:219
- Brief: Submit an asynchronous write request.
- Parameters:
  - `req` (const NVMeIORequest &): Input parameter.
- Return: true if the request was enqueued; false on ring overflow or error.
- Details: Submit Write. req I/O request descriptor. true if the request was enqueued; false on ring overflow or error. Falls back to synchronous pwrite() when io_uring is unavailable. req Input parameter. True when the operation succeeds. Calls: isIoUringActive(), ring_lk(), get(), __atomic_load_n(), THEMIS_WARN(), std::memset(), __atomic_store_n(), themis_io_uring_enter().

#### `void teardownIoUring()`
- Source: `include/storage/nvme_manager.h`:297
- Brief: Teardown Io Uring.
- Parameters: none
- Details: Calls: ring_lock(), get(), munmap(), close().

#### `~NVMeManager()`
- Source: `include/storage/nvme_manager.h`:157
- Brief: n/a
- Parameters: none

### themis::storage::NlpMetadataExtractor

#### `NlpMetadataExtractor(const Config &config)`
- Source: `include/storage/nlp_metadata_extractor.h`:94
- Brief: Constructor with configuration.
- Parameters:
  - `config` (const Config &): n/a

#### `void computeTextStats(const std::string &text, ExtractedMetadata &meta) const`
- Source: `include/storage/nlp_metadata_extractor.h`:176
- Brief: Compute text statistics.
- Parameters:
  - `text` (const std::string &): n/a
  - `meta` (ExtractedMetadata &): n/a

#### `std::string concatenateFields(const BaseEntity &entity, const std::vector< std::string > &fields) const`
- Source: `include/storage/nlp_metadata_extractor.h`:181
- Brief: Concatenate text from multiple fields.
- Parameters:
  - `entity` (const BaseEntity &): n/a
  - `fields` (const std::vector< std::string > &): n/a

#### `std::string detectLanguage(const std::string &text) const`
- Source: `include/storage/nlp_metadata_extractor.h`:153
- Brief: Detect language only (fast path).
- Parameters:
  - `text` (const std::string &): Input text
- Return: Language code (en, de, fr, etc.)
- Details: text Input text Language code (en, de, fr, etc.)

#### `bool enrichEntity(BaseEntity &entity, const std::vector< std::string > &text_fields={"content", "text", "body"}) const`
- Source: `include/storage/nlp_metadata_extractor.h`:132
- Brief: Enrich entity with NLP metadata.
- Parameters:
  - `entity` (BaseEntity &): Entity to enrich (modified in-place)
  - `text_fields` (const std::vector< std::string > &): Fields to analyze
- Return: true if successful
- Details: Adds metadata fields to entity: nlp_keywords: array of keywords nlp_language: detected language nlp_sentiment: sentiment score nlp_complexity: text complexity nlp_entities: extracted named entities entity Entity to enrich (modified in-place) text_fields Fields to analyze true if successful

#### `std::map< std::string, std::vector< std::string > > extractEntities(const std::string &text) const`
- Source: `include/storage/nlp_metadata_extractor.h`:161
- Brief: Extract named entities only (fast path).
- Parameters:
  - `text` (const std::string &): Input text
- Return: Map of entity type -> list of entities
- Details: text Input text Map of entity type -> list of entities

#### `std::vector< std::string > extractKeywords(const std::string &text, size_t max_keywords=10) const`
- Source: `include/storage/nlp_metadata_extractor.h`:143
- Brief: Extract keywords only (fast path).
- Parameters:
  - `text` (const std::string &): Input text
  - `max_keywords` (size_t): Maximum keywords to return
- Return: List of keywords
- Details: text Input text max_keywords Maximum keywords to return List of keywords

#### `ExtractedMetadata extractMetadata(const std::string &text) const`
- Source: `include/storage/nlp_metadata_extractor.h`:102
- Brief: Extract metadata from text content.
- Parameters:
  - `text` (const std::string &): Input text to analyze
- Return: Extracted metadata
- Details: text Input text to analyze Extracted metadata

#### `ExtractedMetadata extractMetadataFromEntity(const BaseEntity &entity, const std::vector< std::string > &text_fields={"content", "text", "body"}) const`
- Source: `include/storage/nlp_metadata_extractor.h`:114
- Brief: Extract metadata from BaseEntity.
- Parameters:
  - `entity` (const BaseEntity &): Entity to analyze
  - `text_fields` (const std::vector< std::string > &): Fields to analyze (e.g., {"content", "description"})
- Return: Extracted metadata
- Details: Analyzes text fields in the entity and returns metadata. Useful during document ingestion. entity Entity to analyze text_fields Fields to analyze (e.g., {"content", "description"}) Extracted metadata

#### `const Config & getConfig() const`
- Source: `include/storage/nlp_metadata_extractor.h`:167
- Brief: Get configuration.
- Parameters: none

### themis::storage::NlpMetadataExtractor::ExtractedMetadata

#### `ExtractedMetadata fromJson(const std::string &json_str)`
- Source: `include/storage/nlp_metadata_extractor.h`:88
- Brief: Parse from JSON.
- Parameters:
  - `json_str` (const std::string &): n/a

#### `std::string toJson() const`
- Source: `include/storage/nlp_metadata_extractor.h`:83
- Brief: Convert to JSON for storage.
- Parameters: none

### themis::storage::PinGuard

#### `PinGuard() noexcept=default`
- Source: `include/storage/columnar_cache.h`:131
- Brief: n/a
- Parameters: none

#### `PinGuard(ColumnarCache *cache, SegmentKey key, const ColumnSegment *seg) noexcept`
- Source: `include/storage/columnar_cache.h`:152
- Brief: n/a
- Parameters:
  - `cache` (ColumnarCache *): n/a
  - `key` (SegmentKey): n/a
  - `seg` (const ColumnSegment *): n/a

#### `PinGuard(PinGuard &&) noexcept`
- Source: `include/storage/columnar_cache.h`:134
- Brief: n/a
- Parameters:
  - `o` (PinGuard &&): n/a

#### `PinGuard(const PinGuard &)=delete`
- Source: `include/storage/columnar_cache.h`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PinGuard &): n/a

#### `operator bool() const noexcept`
- Source: `include/storage/columnar_cache.h`:141
- Brief: Returns true when a valid segment is pinned.
- Parameters: none

#### `PinGuard & operator=(PinGuard &&) noexcept`
- Source: `include/storage/columnar_cache.h`:135
- Brief: n/a
- Parameters:
  - `o` (PinGuard &&): n/a

#### `PinGuard & operator=(const PinGuard &)=delete`
- Source: `include/storage/columnar_cache.h`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (const PinGuard &): n/a

#### `void release() noexcept`
- Source: `include/storage/columnar_cache.h`:147
- Brief: Manually release the pin. After release operator bool() is false.
- Parameters: none

#### `const ColumnSegment & segment() const noexcept`
- Source: `include/storage/columnar_cache.h`:144
- Brief: Access the pinned segment. UB if !*this.
- Parameters: none

#### `~PinGuard() noexcept`
- Source: `include/storage/columnar_cache.h`:132
- Brief: n/a
- Parameters: none

### themis::storage::ProvenanceRecord

#### `std::string canonicalBytes() const`
- Source: `include/storage/gguf_metadata.h`:67
- Brief: Produce the canonical byte string that is signed / verified.
- Parameters: none

#### `bool isComplete() const noexcept`
- Source: `include/storage/gguf_metadata.h`:59
- Brief: True if all mandatory fields are non-empty / valid.
- Parameters: none

#### `bool operator!=(const ProvenanceRecord &o) const noexcept`
- Source: `include/storage/gguf_metadata.h`:78
- Brief: n/a
- Parameters:
  - `o` (const ProvenanceRecord &): n/a

#### `bool operator==(const ProvenanceRecord &o) const noexcept`
- Source: `include/storage/gguf_metadata.h`:69
- Brief: n/a
- Parameters:
  - `o` (const ProvenanceRecord &): n/a

### themis::storage::QuantizedCore

#### `std::optional< QuantizedCore > deserialize(const std::vector< uint8_t > &bytes)`
- Source: `include/storage/tt_quantizer.h`:70
- Brief: Deserialise from bytes.
- Parameters:
  - `bytes` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: Deserialize. bytes Input parameter. Return value. Calls: size(), std::memcpy(), readU64(), readF32(), assign(), begin(), THEMIS_WARN().

#### `std::size_t numElements() const noexcept`
- Source: `include/storage/tt_quantizer.h`:64
- Brief: n/a
- Parameters: none

#### `std::vector< uint8_t > serialize() const`
- Source: `include/storage/tt_quantizer.h`:67
- Brief: Serialise to bytes.
- Parameters: none

### themis::storage::QuantizedTrain

#### `double compressionRatio() const noexcept`
- Source: `include/storage/tt_quantizer.h`:93
- Brief: Compression ratio: (dense float32 elements × 4) / totalBytes().
- Parameters: none

#### `std::optional< QuantizedTrain > deserialize(const std::vector< uint8_t > &bytes)`
- Source: `include/storage/tt_quantizer.h`:99
- Brief: Deserialise from bytes.
- Parameters:
  - `bytes` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: Deserialize. bytes Input parameter. Return value. Calls: size(), std::memcpy(), readU64(), resize(), readF64(), cb(), begin(), std::move().

#### `std::size_t order() const noexcept`
- Source: `include/storage/tt_quantizer.h`:87
- Brief: n/a
- Parameters: none

#### `std::vector< uint8_t > serialize() const`
- Source: `include/storage/tt_quantizer.h`:96
- Brief: Serialise to bytes for RocksDB storage.
- Parameters: none

#### `std::size_t totalBytes() const noexcept`
- Source: `include/storage/tt_quantizer.h`:90
- Brief: Total compressed bytes across all cores.
- Parameters: none

### themis::storage::RLECodec

#### `Result< std::vector< int32_t > > decodeInt32(const std::vector< uint8_t > &encoded)`
- Source: `include/storage/columnar_format.h`:109
- Brief: Decode Int32.
- Parameters:
  - `encoded` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: encoded Input parameter. Return value. Calls: size(), tl::unexpected(), Error(), std::memcpy(), push_back().

#### `Result< std::vector< int64_t > > decodeInt64(const std::vector< uint8_t > &encoded)`
- Source: `include/storage/columnar_format.h`:110
- Brief: Decode Int64.
- Parameters:
  - `encoded` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: encoded Input parameter. Return value. Calls: size(), tl::unexpected(), Error(), std::memcpy(), push_back().

#### `Result< std::vector< uint8_t > > encodeInt32(const std::vector< int32_t > &data)`
- Source: `include/storage/columnar_format.h`:105
- Brief: ============================================================================ RLE (Run-Length Encoding) Implementation ============================================================================
- Parameters:
  - `data` (const std::vector< int32_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: empty(), reserve(), size(), push_back(), insert(), end().

#### `Result< std::vector< uint8_t > > encodeInt64(const std::vector< int64_t > &data)`
- Source: `include/storage/columnar_format.h`:106
- Brief: Encode Int64.
- Parameters:
  - `data` (const std::vector< int64_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: empty(), reserve(), size(), push_back(), insert(), end().

### themis::storage::RecoveryFaultHandler

#### `RecoveryFaultHandler()=default`
- Source: `include/storage/storage_recovery_fault_handler.h`:131
- Brief: Constructor: initializes handler with default policies.
- Parameters: none

#### `RecoveryFaultHandler(RecoveryFaultHandler &&) noexcept=default`
- Source: `include/storage/storage_recovery_fault_handler.h`:138
- Brief: Move-enabled.
- Parameters:
  - `<unnamed>` (RecoveryFaultHandler &&): n/a

#### `RecoveryFaultHandler(const RecoveryFaultHandler &)=delete`
- Source: `include/storage/storage_recovery_fault_handler.h`:134
- Brief: Non-copyable.
- Parameters:
  - `<unnamed>` (const RecoveryFaultHandler &): n/a

#### `RecoveryFaultReport handleInvalidCheckpoint(std::uint64_t checkpoint_seq, std::uint64_t wal_tail_seq, std::uint64_t recovery_start_seq) noexcept`
- Source: `include/storage/storage_recovery_fault_handler.h`:185
- Brief: Handle an invalid checkpoint (sequence past WAL tail or corrupted).
- Parameters:
  - `checkpoint_seq` (std::uint64_t): Invalid checkpoint sequence number
  - `wal_tail_seq` (std::uint64_t): Actual last sequence in WAL
  - `recovery_start_seq` (std::uint64_t): Suggested safe checkpoint to start recovery from
- Return: RecoveryFaultReport with guidance for fallback checkpoint
- Details: Implements fail-safe behavior: log diagnostic, fall back to safe checkpoint. checkpoint_seq Invalid checkpoint sequence number wal_tail_seq Actual last sequence in WAL recovery_start_seq Suggested safe checkpoint to start recovery from RecoveryFaultReport with guidance for fallback checkpoint Guaranteed behavior: Returns CHECKPOINT_FAILED error code Suggests recovery_start_seq as safe fallback Should stop current recovery and restart from suggested point (should_stop_recovery=true) Provides operator suggestion for investigation

#### `RecoveryFaultReport handleRecoveryRetryExhausted(std::uint64_t checkpoint_seq, int attempts, std::string_view last_error) noexcept`
- Source: `include/storage/storage_recovery_fault_handler.h`:300
- Brief: Handle maximum retry attempts exceeded during recovery.
- Parameters:
  - `checkpoint_seq` (std::uint64_t): Checkpoint where recovery started
  - `attempts` (int): Number of failed attempts
  - `last_error` (std::string_view): Description of last error encountered
- Return: RecoveryFaultReport with CRITICAL severity
- Details: Implements fail-safe behavior: abort recovery, emit alert for manual intervention. checkpoint_seq Checkpoint where recovery started attempts Number of failed attempts last_error Description of last error encountered RecoveryFaultReport with CRITICAL severity Guaranteed behavior: Returns RECOVERY_TIMEOUT or INTERNAL_ERROR should_stop_recovery=true (must abort) is_retryable=false (retry limit reached) Emits CRITICAL severity diagnostic Suggests manual recovery intervention

#### `RecoveryFaultReport handleRecoveryTimeout(std::chrono::milliseconds elapsed_time, std::uint64_t entries_replayed, std::uint64_t checkpoint_seq) noexcept`
- Source: `include/storage/storage_recovery_fault_handler.h`:207
- Brief: Handle recovery timeout (exceeded kRecoveryHardTimeout).
- Parameters:
  - `elapsed_time` (std::chrono::milliseconds): Time elapsed since recovery started
  - `entries_replayed` (std::uint64_t): Number of entries successfully replayed before timeout
  - `checkpoint_seq` (std::uint64_t): Checkpoint where recovery started
- Return: RecoveryFaultReport with alert-level severity
- Details: Implements fail-safe behavior: log alert, stop recovery, preserve state. elapsed_time Time elapsed since recovery started entries_replayed Number of entries successfully replayed before timeout checkpoint_seq Checkpoint where recovery started RecoveryFaultReport with alert-level severity Guaranteed behavior: Returns RECOVERY_TIMEOUT error code should_stop_recovery=true (must abort) is_retryable=true (can retry with increased timeout) Emits HIGH/CRITICAL severity diagnostic Suggests infrastructure checks (disk speed, memory pressure, etc.)

#### `RecoveryFaultReport handleReplayEntryFailure(std::uint64_t fault_seq, std::string_view replay_error, bool is_critical=false) noexcept`
- Source: `include/storage/storage_recovery_fault_handler.h`:277
- Brief: Handle failure during entry replay (e.g., sequence number error, application-level replay failure).
- Parameters:
  - `fault_seq` (std::uint64_t): Sequence number of entry that failed to replay
  - `replay_error` (std::string_view): Description of failure
  - `is_critical` (bool): true if error cannot be safely skipped
- Return: RecoveryFaultReport with skip/stop guidance
- Details: Implements fail-safe behavior: log diagnostic, decide whether to skip entry or stop recovery. fault_seq Sequence number of entry that failed to replay replay_error Description of failure is_critical true if error cannot be safely skipped RecoveryFaultReport with skip/stop guidance Guaranteed behavior: Returns appropriate error code based on failure type is_critical determines should_stop_recovery flag Emits MEDIUM/HIGH severity diagnostic Suggests manual investigation if critical

#### `RecoveryFaultReport handleTornWalEntry(std::string_view wal_path, std::uint64_t checkpoint_seq, std::uint64_t fault_seq, std::uint64_t entries_replayed) noexcept`
- Source: `include/storage/storage_recovery_fault_handler.h`:162
- Brief: Handle a torn WAL entry (partial write at tail).
- Parameters:
  - `wal_path` (std::string_view): Path to WAL file
  - `checkpoint_seq` (std::uint64_t): Checkpoint position for context
  - `fault_seq` (std::uint64_t): Sequence number of torn entry
  - `entries_replayed` (std::uint64_t): Number of entries replayed before this one
- Return: RecoveryFaultReport with fault details and recovery guidance
- Details: Implements fail-safe behavior: log diagnostic, skip entry, continue replay. wal_path Path to WAL file checkpoint_seq Checkpoint position for context fault_seq Sequence number of torn entry entries_replayed Number of entries replayed before this one RecoveryFaultReport with fault details and recovery guidance Guaranteed behavior: Returns RECOVERY_INCOMPLETE error code Emits diagnostic event for operator awareness Provides suggestion: "Partial WAL tail discarded; verify backup..." Should continue recovery (is_retryable=false, should_stop_recovery=false)

#### `RecoveryFaultReport handleWalFileCorruption(std::string_view wal_path, std::string_view corruption_type, std::uint64_t bytes_verified) noexcept`
- Source: `include/storage/storage_recovery_fault_handler.h`:231
- Brief: Handle WAL file corruption (invalid magic, CRC mismatch, etc.).
- Parameters:
  - `wal_path` (std::string_view): Path to corrupted WAL file
  - `corruption_type` (std::string_view): Description of corruption (e.g., "invalid_magic", "crc_mismatch", "truncated_file")
  - `bytes_verified` (std::uint64_t): Bytes successfully verified before corruption
- Return: RecoveryFaultReport with recovery strategy recommendations
- Details: Implements fail-safe behavior: log alert, consider alternate recovery strategies (backup restore, PITR, etc.). wal_path Path to corrupted WAL file corruption_type Description of corruption (e.g., "invalid_magic", "crc_mismatch", "truncated_file") bytes_verified Bytes successfully verified before corruption RecoveryFaultReport with recovery strategy recommendations Guaranteed behavior: Returns WAL_CORRUPTED error code should_stop_recovery=true (this WAL cannot be used) is_retryable=false (corruption is permanent) Emits CRITICAL severity diagnostic Suggests backup restore or PITR recovery

#### `RecoveryFaultReport handleWalReadError(std::string_view wal_path, std::string_view io_error_msg, int retry_count) noexcept`
- Source: `include/storage/storage_recovery_fault_handler.h`:254
- Brief: Handle I/O error during WAL read.
- Parameters:
  - `wal_path` (std::string_view): Path to WAL file
  - `io_error_msg` (std::string_view): Description of I/O error (e.g., "permission denied", "disk offline")
  - `retry_count` (int): Number of previous retry attempts
- Return: RecoveryFaultReport with retry/failover guidance
- Details: Implements fail-safe behavior: log diagnostic, decide whether to retry or fail over to backup recovery. wal_path Path to WAL file io_error_msg Description of I/O error (e.g., "permission denied", "disk offline") retry_count Number of previous retry attempts RecoveryFaultReport with retry/failover guidance Guaranteed behavior: Returns WAL_WRITE_FAILED or INTERNAL_ERROR is_retryable based on error type and retry_count Suggests failover strategy if retries exhausted Emits HIGH/CRITICAL severity diagnostic

#### `RecoveryFaultHandler & operator=(RecoveryFaultHandler &&) noexcept=default`
- Source: `include/storage/storage_recovery_fault_handler.h`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (RecoveryFaultHandler &&): n/a

#### `RecoveryFaultHandler & operator=(const RecoveryFaultHandler &)=delete`
- Source: `include/storage/storage_recovery_fault_handler.h`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (const RecoveryFaultHandler &): n/a

#### `void setContinueOnTornWal(bool continue_on_torn) noexcept`
- Source: `include/storage/storage_recovery_fault_handler.h`:337
- Brief: Set whether recovery should continue on torn WAL entries.
- Parameters:
  - `continue_on_torn` (bool): true to skip torn entries and continue, false to abort recovery
- Details: Default: true (continue; emit RECOVERY_INCOMPLETE diagnostic). continue_on_torn true to skip torn entries and continue, false to abort recovery

#### `void setMaxRecoveryRetries(int max_retries) noexcept`
- Source: `include/storage/storage_recovery_fault_handler.h`:314
- Brief: Set maximum number of recovery retry attempts.
- Parameters:
  - `max_retries` (int): Maximum number of retries (0 = no retries)
- Details: Default: 3 retries for transient faults. max_retries Maximum number of retries (0 = no retries)

#### `void setRecoveryTimeout(std::chrono::milliseconds timeout) noexcept`
- Source: `include/storage/storage_recovery_fault_handler.h`:325
- Brief: Set timeout for entire recovery procedure.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Maximum duration for recovery operation
- Details: Default: kRecoveryHardTimeout from storage_api_contract.h. timeout Maximum duration for recovery operation

#### `~RecoveryFaultHandler()=default`
- Source: `include/storage/storage_recovery_fault_handler.h`:141
- Brief: n/a
- Parameters: none

### themis::storage::RocksDBTensorBackend

#### `RocksDBTensorBackend(std::shared_ptr< RocksDBWrapper > db)`
- Source: `include/storage/tensor_network_storage_engine.h`:175
- Brief: Construct with a shared RocksDBWrapper.
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): n/a
- Throws:
  - std::invalid_argument: when db is null.
- Details: std::invalid_argument when db is null.

#### `bool del(const std::string &key) override`
- Source: `include/storage/tensor_network_storage_engine.h`:183
- Brief: Del.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. True when the operation succeeds.

#### `std::optional< std::vector< uint8_t > > get(const std::string &key) const override`
- Source: `include/storage/tensor_network_storage_engine.h`:181
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::vector< std::string > listKeys(const std::string &prefix) const override`
- Source: `include/storage/tensor_network_storage_engine.h`:187
- Brief: Returns all keys that start with prefix (sorted lexicographically).
- Parameters:
  - `prefix` (const std::string &): n/a

#### `bool put(const std::string &key, const std::vector< uint8_t > &value) override`
- Source: `include/storage/tensor_network_storage_engine.h`:177
- Brief: Put.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. value Input parameter. True when the operation succeeds.

### themis::storage::S3BlobBackend

#### `S3BlobBackend(const std::string &bucket, const std::string &region, const std::string &prefix="")`
- Source: `include/storage/blob_backend_s3.h`:42
- Brief: Construct an S3 Blob Storage backend.
- Parameters:
  - `bucket` (const std::string &): Name of the S3 bucket.
  - `region` (const std::string &): AWS region (e.g. "us-east-1").
  - `prefix` (const std::string &): Optional object-key prefix (e.g. "blobs/").
- Details: bucket Name of the S3 bucket. region AWS region (e.g. "us-east-1"). prefix Optional object-key prefix (e.g. "blobs/").

#### `std::string computeSHA256(const std::vector< uint8_t > &data)`
- Source: `include/storage/blob_backend_s3.h`:97
- Brief: n/a
- Parameters:
  - `data` (const std::vector< uint8_t > &): n/a

#### `bool exists(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_s3.h`:78
- Brief: Check whether a blob exists in the bucket.
- Parameters:
  - `ref` (const BlobRef &): Blob reference to check.
- Return: true if the object exists.
- Details: ref Blob reference to check. true if the object exists.

#### `Result< std::vector< uint8_t > > get(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_s3.h`:64
- Brief: Retrieve a blob from the S3 bucket.
- Parameters:
  - `ref` (const BlobRef &): Blob reference previously returned by put().
- Return: Blob data on success, or an error if the object does not exist or the download fails.
- Details: ref Blob reference previously returned by put(). Blob data on success, or an error if the object does not exist or the download fails.

#### `std::string getS3Key(const std::string &blob_id) const`
- Source: `include/storage/blob_backend_s3.h`:98
- Brief: n/a
- Parameters:
  - `blob_id` (const std::string &): n/a

#### `bool isAvailable() const override`
- Source: `include/storage/blob_backend_s3.h`:91
- Brief: Check whether the S3 backend is operational.
- Parameters: none
- Details: Returns false when the AWS SDK was not compiled in, when credentials are missing, or when the bucket cannot be reached.

#### `std::string name() const override`
- Source: `include/storage/blob_backend_s3.h`:83
- Brief: Return the backend name ("s3").
- Parameters: none

#### `Result< BlobRef > put(const std::string &blob_id, const std::vector< uint8_t > &data) override`
- Source: `include/storage/blob_backend_s3.h`:55
- Brief: Store a blob in the S3 bucket.
- Parameters:
  - `blob_id` (const std::string &): Unique blob identifier (used as the S3 object key).
  - `data` (const std::vector< uint8_t > &): Raw blob data.
- Return: BlobRef on success, or an error if the SDK is unavailable or the upload fails.
- Details: blob_id Unique blob identifier (used as the S3 object key). data Raw blob data. BlobRef on success, or an error if the SDK is unavailable or the upload fails.

#### `Result< void > remove(const BlobRef &ref) override`
- Source: `include/storage/blob_backend_s3.h`:71
- Brief: Delete a blob from the S3 bucket.
- Parameters:
  - `ref` (const BlobRef &): Blob reference previously returned by put().
- Return: void on success, or an error if deletion fails.
- Details: ref Blob reference previously returned by put(). void on success, or an error if deletion fails.

#### `~S3BlobBackend() override`
- Source: `include/storage/blob_backend_s3.h`:46
- Brief: n/a
- Parameters: none

### themis::storage::SIMDColumnFilter

#### `SIMDColumnFilter()=default`
- Source: `include/storage/simd_filter.h`:137
- Brief: n/a
- Parameters: none

#### `bool canSkipSegment(const ColumnSegment &segment, const ColumnPredicate &pred) noexcept`
- Source: `include/storage/simd_filter.h`:171
- Brief: n/a
- Parameters:
  - `segment` (const ColumnSegment &): n/a
  - `pred` (const ColumnPredicate &): n/a

#### `const SIMDFilterStats & lastStats() const noexcept`
- Source: `include/storage/simd_filter.h`:161
- Brief: Statistics from the most recent scan() or scanBatch() call.
- Parameters: none

#### `void resetStats() noexcept`
- Source: `include/storage/simd_filter.h`:164
- Brief: Reset statistics counters.
- Parameters: none

#### `std::vector< uint32_t > scan(const ColumnSegment &segment, const ColumnPredicate &predicate)`
- Source: `include/storage/simd_filter.h`:148
- Brief: Scan.
- Parameters:
  - `segment` (const ColumnSegment &): Input parameter.
  - `predicate` (const ColumnPredicate &): Input parameter.
- Return: Sorted list of matching row indices (0-based). Returns an empty vector if the zone-map eliminates the entire segment.
- Details: Scan a decoded ColumnSegment and return the indices of rows satisfying the predicate. The segment must already be decoded (i.e., rawData() is populated with the column's native type data). segment Decoded ColumnSegment. predicate Predicate to evaluate (must match segment column type). Sorted list of matching row indices (0-based). Returns an empty vector if the zone-map eliminates the entire segment. segment Input parameter. predicate Input parameter. Return value. Calls: std::chrono::steady_clock::now(), detectSIMDLevel(), metadata(), canSkipSegmentForPred(), count(), rawData(), empty(), reserve().

#### `std::vector< uint32_t > scanBatch(const std::vector< ColumnSegment > &segments, const ColumnPredicate &predicate)`
- Source: `include/storage/simd_filter.h`:157
- Brief: Scan Batch.
- Parameters:
  - `segments` (const std::vector< ColumnSegment > &): Input parameter.
  - `predicate` (const ColumnPredicate &): Input parameter.
- Return: Global row indices (ascending) of matching rows.
- Details: Batch-scan multiple segments for the same predicate (e.g., multi-chunk column). Offsets each segment's indices by its start row. segments Ordered list of decoded ColumnSegments. predicate Predicate to evaluate. Global row indices (ascending) of matching rows. segments Input parameter. predicate Input parameter. Return value. Calls: metadata(), max_size(), reserve(), scan(), push_back().

### themis::storage::SchemaDeadWeightDetector

#### `SchemaDeadWeightDetector()`
- Source: `include/storage/schema_dead_weight_detector.h`:148
- Brief: Construct with default configuration.
- Parameters: none

#### `SchemaDeadWeightDetector(Config config)`
- Source: `include/storage/schema_dead_weight_detector.h`:157
- Brief: Construct with explicit configuration.
- Parameters:
  - `config` (Config): n/a
- Details: Two overloads instead of = {} default arg to work around GCC DR1607 (nested struct with non-trivially-constructible default member initialisers used in an enclosing-class declaration).

#### `SchemaDeadWeightDetector(SchemaDeadWeightDetector &&) noexcept=default`
- Source: `include/storage/schema_dead_weight_detector.h`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDeadWeightDetector &&): n/a

#### `SchemaDeadWeightDetector(const SchemaDeadWeightDetector &)=delete`
- Source: `include/storage/schema_dead_weight_detector.h`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaDeadWeightDetector &): n/a

#### `DeadWeightReport analyze(const SchemaAccessStats &stats, const GdprFieldRegistry &gdpr_fields) const`
- Source: `include/storage/schema_dead_weight_detector.h`:190
- Brief: Analyse schema access statistics and produce a dead-weight report.
- Parameters:
  - `stats` (const SchemaAccessStats &): Per-field access time-series.
  - `gdpr_fields` (const GdprFieldRegistry &): Registry of GDPR-protected fields.
- Return: Advisory report.
- Details: Algorithm: For each field in stats: a. Skip if GDPR-protected. b. Compute days_since_access from the most recent AccessEntry. c. Compute seasonality_score via computeSeasonalityScore(). d. Compute confidence using the formula above. e. Skip if confidence < config_.min_confidence. f. Skip if seasonality_score >= config_.seasonality_exclusion_threshold. For surviving candidates determine recommendation. Emit a DecisionRecord. stats Per-field access time-series. gdpr_fields Registry of GDPR-protected fields. Advisory report.

#### `double computeSeasonalityScore(const std::vector< AccessEntry > &access_series) const`
- Source: `include/storage/schema_dead_weight_detector.h`:204
- Brief: Compute a seasonality score for an access time-series.
- Parameters:
  - `access_series` (const std::vector< AccessEntry > &): Chronologically ordered access counts.
- Return: Fraction of total variance explained by the harmonics.
- Details: Uses a Fourier-coefficient approximation with config_.fourier_harmonics harmonics. Returns a score in [0.0, 1.0]; values above 0.5 indicate a significant periodic component. access_series Chronologically ordered access counts. Fraction of total variance explained by the harmonics.

#### `std::string determineRecommendation(const std::string &field_path, uint32_t days_since_access)`
- Source: `include/storage/schema_dead_weight_detector.h`:211
- Brief: Determine Recommendation.
- Parameters:
  - `field_path` (const std::string &): Path to the field.
  - `days_since_access` (uint32_t): Input parameter.
- Return: Return value.
- Details: field_path Path to the field. days_since_access Input parameter. Return value.

#### `void emitDecisionRecord(const DeadWeightReport &report) const`
- Source: `include/storage/schema_dead_weight_detector.h`:214
- Brief: n/a
- Parameters:
  - `report` (const DeadWeightReport &): n/a

#### `SchemaDeadWeightDetector & operator=(SchemaDeadWeightDetector &&) noexcept=default`
- Source: `include/storage/schema_dead_weight_detector.h`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaDeadWeightDetector &&): n/a

#### `SchemaDeadWeightDetector & operator=(const SchemaDeadWeightDetector &)=delete`
- Source: `include/storage/schema_dead_weight_detector.h`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaDeadWeightDetector &): n/a

#### `void setDecisionRecordProcessor(std::shared_ptr< themis::llm::DecisionRecordYamlProcessor > processor)`
- Source: `include/storage/schema_dead_weight_detector.h`:167
- Brief: Set Decision Record Processor.
- Parameters:
  - `processor` (std::shared_ptr< themis::llm::DecisionRecordYamlProcessor >): Input parameter.
- Details: processor Input parameter.

#### `~SchemaDeadWeightDetector()=default`
- Source: `include/storage/schema_dead_weight_detector.h`:158
- Brief: n/a
- Parameters: none

### themis::storage::SchemaMigrator

#### `SchemaMigrator(SchemaManager &schema_mgr)`
- Source: `include/storage/online_schema_migration.h`:171
- Brief: Construct a SchemaMigrator with default configuration.
- Parameters:
  - `schema_mgr` (SchemaManager &): SchemaManager instance owning the target tables.
- Details: schema_mgr SchemaManager instance owning the target tables.

#### `SchemaMigrator(SchemaManager &schema_mgr, const Config &config)`
- Source: `include/storage/online_schema_migration.h`:179
- Brief: Construct a SchemaMigrator with custom configuration.
- Parameters:
  - `schema_mgr` (SchemaManager &): SchemaManager instance owning the target tables.
  - `config` (const Config &): Configuration overrides.
- Details: schema_mgr SchemaManager instance owning the target tables. config Configuration overrides.

#### `SchemaMigrator(SchemaMigrator &&) noexcept=default`
- Source: `include/storage/online_schema_migration.h`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaMigrator &&): n/a

#### `SchemaMigrator(const SchemaMigrator &)=delete`
- Source: `include/storage/online_schema_migration.h`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaMigrator &): n/a

#### `SchemaMigrator & addColumn(const std::string &table, const std::string &column, const std::string &type, bool nullable=true)`
- Source: `include/storage/online_schema_migration.h`:200
- Brief: Stage an ADD COLUMN operation.
- Parameters:
  - `table` (const std::string &): Input parameter.
  - `column` (const std::string &): Input parameter.
  - `type` (const std::string &): Input parameter.
  - `nullable` (bool): Input parameter.
- Return: *this for method chaining.
- Details: Add Column. table Table name. column New column name. type Column type string (e.g. "VARCHAR(20)", "integer"). nullable Whether the new column is nullable (default: true). *this for method chaining. table Input parameter. column Input parameter. type Input parameter. nullable Input parameter. Return value.

#### `SchemaMigrator & addIndex(const std::string &table, const std::string &column, bool unique=false)`
- Source: `include/storage/online_schema_migration.h`:249
- Brief: Stage an ADD INDEX operation.
- Parameters:
  - `table` (const std::string &): Input parameter.
  - `column` (const std::string &): Input parameter.
  - `unique` (bool): Input parameter.
- Return: *this for method chaining.
- Details: Add Index. table Table name. column Column to index. unique Whether the index should enforce uniqueness (default: false). *this for method chaining. table Input parameter. column Input parameter. unique Input parameter. Return value.

#### `MigrationResult applyAddColumn(const MigrationOp &op, SchemaManager::TableSchema &schema)`
- Source: `include/storage/online_schema_migration.h`:315
- Brief: Apply Add Column.
- Parameters:
  - `op` (const MigrationOp &): Input parameter.
  - `schema` (SchemaManager::TableSchema &): Input/output parameter.
- Return: Return value.
- Details: op Input parameter. schema Input/output parameter. Return value.

#### `MigrationResult applyAddIndex(const MigrationOp &op, SchemaManager::TableSchema &schema)`
- Source: `include/storage/online_schema_migration.h`:323
- Brief: Apply Add Index.
- Parameters:
  - `op` (const MigrationOp &): Input parameter.
  - `schema` (SchemaManager::TableSchema &): Input/output parameter.
- Return: Return value.
- Details: op Input parameter. schema Input/output parameter. Return value.

#### `MigrationResult applyChangeColumnType(const MigrationOp &op, SchemaManager::TableSchema &schema)`
- Source: `include/storage/online_schema_migration.h`:321
- Brief: Apply Change Column Type.
- Parameters:
  - `op` (const MigrationOp &): Input parameter.
  - `schema` (SchemaManager::TableSchema &): Input/output parameter.
- Return: Return value.
- Details: op Input parameter. schema Input/output parameter. Return value.

#### `MigrationResult applyDropColumn(const MigrationOp &op, SchemaManager::TableSchema &schema)`
- Source: `include/storage/online_schema_migration.h`:317
- Brief: Apply Drop Column.
- Parameters:
  - `op` (const MigrationOp &): Input parameter.
  - `schema` (SchemaManager::TableSchema &): Input/output parameter.
- Return: Return value.
- Details: op Input parameter. schema Input/output parameter. Return value.

#### `MigrationResult applyDropIndex(const MigrationOp &op, SchemaManager::TableSchema &schema)`
- Source: `include/storage/online_schema_migration.h`:325
- Brief: Apply Drop Index.
- Parameters:
  - `op` (const MigrationOp &): Input parameter.
  - `schema` (SchemaManager::TableSchema &): Input/output parameter.
- Return: Return value.
- Details: op Input parameter. schema Input/output parameter. Return value.

#### `MigrationResult applyPartitionTable(const MigrationOp &op, SchemaManager::TableSchema &schema)`
- Source: `include/storage/online_schema_migration.h`:327
- Brief: Apply Partition Table.
- Parameters:
  - `op` (const MigrationOp &): Input parameter.
  - `schema` (SchemaManager::TableSchema &): Input/output parameter.
- Return: Return value.
- Details: op Input parameter. schema Input/output parameter. Return value.

#### `MigrationResult applyRenameColumn(const MigrationOp &op, SchemaManager::TableSchema &schema)`
- Source: `include/storage/online_schema_migration.h`:319
- Brief: Apply Rename Column.
- Parameters:
  - `op` (const MigrationOp &): Input parameter.
  - `schema` (SchemaManager::TableSchema &): Input/output parameter.
- Return: Return value.
- Details: op Input parameter. schema Input/output parameter. Return value.

#### `SchemaMigrator & changeColumnType(const std::string &table, const std::string &column, const std::string &new_type, bool nullable=true)`
- Source: `include/storage/online_schema_migration.h`:236
- Brief: Stage a CHANGE COLUMN TYPE operation.
- Parameters:
  - `table` (const std::string &): Input parameter.
  - `column` (const std::string &): Input parameter.
  - `new_type` (const std::string &): Input parameter.
  - `nullable` (bool): Input parameter.
- Return: *this for method chaining.
- Details: Change Column Type. table Table name. column Column whose type should change. new_type New type string. nullable New nullability (default: true). *this for method chaining. table Input parameter. column Input parameter. new_type Input parameter. nullable Input parameter. Return value.

#### `OnlineDDLPhase currentPhase() const noexcept`
- Source: `include/storage/online_schema_migration.h`:307
- Brief: n/a
- Parameters: none
- Return: Current DDL phase (thread-safe).
- Details: Current DDL phase (thread-safe).

#### `SchemaMigrator & dropColumn(const std::string &table, const std::string &column)`
- Source: `include/storage/online_schema_migration.h`:212
- Brief: Stage a DROP COLUMN operation.
- Parameters:
  - `table` (const std::string &): Input parameter.
  - `column` (const std::string &): Input parameter.
- Return: *this for method chaining.
- Details: Drop Column. table Table name. column Column to remove. *this for method chaining. table Input parameter. column Input parameter. Return value.

#### `SchemaMigrator & dropIndex(const std::string &table, const std::string &column)`
- Source: `include/storage/online_schema_migration.h`:260
- Brief: Stage a DROP INDEX operation.
- Parameters:
  - `table` (const std::string &): Input parameter.
  - `column` (const std::string &): Input parameter.
- Return: *this for method chaining.
- Details: Drop Index. table Table name. column Indexed column whose index should be removed. *this for method chaining. table Input parameter. column Input parameter. Return value.

#### `MigrationResult migrate()`
- Source: `include/storage/online_schema_migration.h`:291
- Brief: Apply all staged operations as an online (zero-downtime) migration.
- Parameters: none
- Return: MigrationResult describing the outcome of the migration.
- Details: Migrate. Each operation is applied to the SchemaManager in the order it was staged. On success, the SchemaMigrator resets to IDLE so it can be reused. On failure, staged operations are retained and phase() returns FAILED until reset() is called. MigrationResult describing the outcome of the migration. Return value.

#### `SchemaMigrator & operator=(SchemaMigrator &&) noexcept=default`
- Source: `include/storage/online_schema_migration.h`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaMigrator &&): n/a

#### `SchemaMigrator & operator=(const SchemaMigrator &)=delete`
- Source: `include/storage/online_schema_migration.h`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaMigrator &): n/a

#### `SchemaMigrator & partitionTable(const std::string &table, const std::string &partition_key, size_t num_partitions)`
- Source: `include/storage/online_schema_migration.h`:275
- Brief: Stage a PARTITION TABLE operation.
- Parameters:
  - `table` (const std::string &): Input parameter.
  - `partition_key` (const std::string &): Input parameter.
  - `num_partitions` (size_t): Input parameter.
- Return: *this for method chaining.
- Details: Partition Table. Adds partition metadata (partition_key and logical partition names) to the table schema. Partition names are generated as <table>_p<i> for i in [0, num_partitions). table Table name. partition_key Column used as the partition key. num_partitions Number of partitions to create (>= 2). *this for method chaining. table Input parameter. partition_key Input parameter. num_partitions Input parameter. Return value.

#### `size_t pendingOps() const noexcept`
- Source: `include/storage/online_schema_migration.h`:304
- Brief: n/a
- Parameters: none
- Return: Number of operations currently staged.
- Details: Number of operations currently staged.

#### `SchemaMigrator & renameColumn(const std::string &table, const std::string &old_name, const std::string &new_name)`
- Source: `include/storage/online_schema_migration.h`:223
- Brief: Stage a RENAME COLUMN operation.
- Parameters:
  - `table` (const std::string &): Input parameter.
  - `old_name` (const std::string &): Name of the old.
  - `new_name` (const std::string &): Name of the new.
- Return: *this for method chaining.
- Details: Rename Column. table Table name. old_name Current column name. new_name Target column name. *this for method chaining. table Input parameter. old_name Name of the old. new_name Name of the new. Return value.

#### `void reset()`
- Source: `include/storage/online_schema_migration.h`:299
- Brief: Reset the migrator, discarding any staged operations.
- Parameters: none
- Details: Reset the modification detection flag. After reset(), the migrator is in IDLE phase with an empty operation queue and can be reused.

#### `const std::vector< MigrationOp > & stagedOps() const noexcept`
- Source: `include/storage/online_schema_migration.h`:310
- Brief: n/a
- Parameters: none
- Return: Read-only view of the staged operation list.
- Details: Read-only view of the staged operation list.

#### `~SchemaMigrator()=default`
- Source: `include/storage/online_schema_migration.h`:181
- Brief: n/a
- Parameters: none

### themis::storage::SecuritySignature

#### `std::optional< SecuritySignature > deserialize(const std::string &data)`
- Source: `include/storage/security_signature.h`:42
- Brief: Deserialize from binary.
- Parameters:
  - `data` (const std::string &): Input parameter.
- Return: Return value.
- Details: Deserialize. data Input parameter. Return value. Calls: json::parse(), fromJson(), THEMIS_DEBUG().

#### `std::optional< SecuritySignature > fromJson(const nlohmann::json &j)`
- Source: `include/storage/security_signature.h`:36
- Brief: Deserialize from JSON.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: From Json. j Input parameter. Return value. Calls: at(), isValidResourceId(), isHexLowerString(), isSupportedAlgorithm(), contains(), THEMIS_DEBUG().

#### `std::string serialize() const`
- Source: `include/storage/security_signature.h`:39
- Brief: Serialize to binary (for RocksDB storage).
- Parameters: none

#### `nlohmann::json toJson() const`
- Source: `include/storage/security_signature.h`:33
- Brief: Serialize to JSON.
- Parameters: none

### themis::storage::SecuritySignatureManager

#### `SecuritySignatureManager(std::shared_ptr< RocksDBWrapper > db)`
- Source: `include/storage/security_signature_manager.h`:48
- Brief: Create a signature manager backed by RocksDB.
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): Persistent RocksDB wrapper. When null, the manager stays unavailable and all mutating operations fail closed.
- Details: db Persistent RocksDB wrapper. When null, the manager stays unavailable and all mutating operations fail closed.

#### `SecuritySignatureManager(std::shared_ptr< RocksDBWrapper > db, Options options)`
- Source: `include/storage/security_signature_manager.h`:57
- Brief: Create a signature manager backed by RocksDB with explicit fallback policy.
- Parameters:
  - `db` (std::shared_ptr< RocksDBWrapper >): Persistent RocksDB wrapper. When null and options.allow_in_memory_fallback is false, the manager stays unavailable and all mutating operations fail closed.
  - `options` (Options): Construction-time fallback policy.
- Details: db Persistent RocksDB wrapper. When null and options.allow_in_memory_fallback is false, the manager stays unavailable and all mutating operations fail closed. options Construction-time fallback policy.

#### `std::string computeFileHash(const std::string &file_path)`
- Source: `include/storage/security_signature_manager.h`:100
- Brief: Compute SHA256 hash of a file.
- Parameters:
  - `file_path` (const std::string &): Path to the file.
- Return: Return value.
- Details: Compute File Hash. file_path Path to the file. Return value. Calls: file(), buffer(), SHA256(), data(), size(), snprintf(), std::string(), THEMIS_WARN().

#### `bool deleteSignature(const std::string &resource_id)`
- Source: `include/storage/security_signature_manager.h`:69
- Brief: Delete a signature by resource_id.
- Parameters:
  - `resource_id` (const std::string &): Identifier of the resource.
- Return: True when the operation succeeds.
- Details: Delete Signature. resource_id Identifier of the resource. True when the operation succeeds. Calls: makeKey(), erase(), THEMIS_ERROR(), del(), THEMIS_WARN().

#### `std::optional< SecuritySignature > getSignature(const std::string &resource_id)`
- Source: `include/storage/security_signature_manager.h`:66
- Brief: Retrieve a signature by resource_id.
- Parameters:
  - `resource_id` (const std::string &): Identifier of the resource.
- Return: Return value.
- Details: Get Signature. resource_id Identifier of the resource. Return value. Calls: makeKey(), find(), end(), THEMIS_ERROR(), get(), SecuritySignature::deserialize(), THEMIS_DEBUG().

#### `bool hasPersistentBackend() const noexcept`
- Source: `include/storage/security_signature_manager.h`:111
- Brief: Return whether a persistent RocksDB backend is available.
- Parameters: none

#### `bool isUsingFallbackMemoryStore() const noexcept`
- Source: `include/storage/security_signature_manager.h`:106
- Brief: Return whether the manager is currently using the explicit in-memory fallback store.
- Parameters: none

#### `std::vector< SecuritySignature > listAllSignatures()`
- Source: `include/storage/security_signature_manager.h`:72
- Brief: List all stored signatures.
- Parameters: none
- Return: Return value.
- Details: List All Signatures. Return value. Calls: SecuritySignature::deserialize(), has_value(), push_back(), THEMIS_ERROR(), makePrefixRange(), iterateRange(), std::string().

#### `std::string makeKey(const std::string &resource_id) const`
- Source: `include/storage/security_signature_manager.h`:121
- Brief: n/a
- Parameters:
  - `resource_id` (const std::string &): n/a

#### `std::pair< std::string, std::string > makePrefixRange()`
- Source: `include/storage/security_signature_manager.h`:126
- Brief: n/a
- Parameters: none
- Details: Returns the [start_key, end_key) range that covers all keys with KEY_PREFIX. end_key is KEY_PREFIX with the last byte incremented (e.g. "security_sig;" when KEY_PREFIX == "security_sig:").

#### `std::string normalizeResourceId(const std::string &path)`
- Source: `include/storage/security_signature_manager.h`:103
- Brief: Normalize resource identifier (resolve relative paths, symlinks).
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: Normalize Resource Id. path Input parameter. Return value. Calls: p(), fs::exists(), fs::weakly_canonical(), generic_string(), substr(), THEMIS_WARN().

#### `bool storeSignature(const SecuritySignature &sig)`
- Source: `include/storage/security_signature_manager.h`:63
- Brief: Store or update a security signature.
- Parameters:
  - `sig` (const SecuritySignature &): Input parameter.
- Return: True when the operation succeeds.
- Details: Store Signature. sig Input parameter. True when the operation succeeds. Calls: makeKey(), serialize(), THEMIS_ERROR(), put(), THEMIS_WARN().

#### `VerifyAllResult verifyAll()`
- Source: `include/storage/security_signature_manager.h`:97
- Brief: Verify All.
- Parameters: none
- Return: Return value.
- Details: Verify all stored signatures by iterating over all document keys and checking each file's SHA256 hash against its stored signature. Uses RocksDBWrapper::iterateRange under the hood when RocksDB is available. Return value. Calls: SecuritySignature::deserialize(), has_value(), verifyFile(), push_back(), THEMIS_ERROR(), makePrefixRange(), iterateRange(), std::string().

#### `bool verifyFile(const std::string &file_path, const std::string &resource_id)`
- Source: `include/storage/security_signature_manager.h`:78
- Brief: Verify File.
- Parameters:
  - `file_path` (const std::string &): Path to the file.
  - `resource_id` (const std::string &): Identifier of the resource.
- Return: True when the operation succeeds.
- Details: Verify a file against stored signature Returns true if hash matches, false if mismatch or signature missing file_path Path to the file. resource_id Identifier of the resource. True when the operation succeeds. Calls: computeFileHash(), empty(), getSignature(), has_value(), THEMIS_WARN().

### themis::storage::SecuritySignatureManager::VerifyAllResult

#### `bool success() const`
- Source: `include/storage/security_signature_manager.h`:89
- Brief: n/a
- Parameters: none

### themis::storage::SegmentKey

#### `bool operator==(const SegmentKey &o) const noexcept`
- Source: `include/storage/columnar_cache.h`:41
- Brief: n/a
- Parameters:
  - `o` (const SegmentKey &): n/a

### themis::storage::StaticKeyProvider

#### `StaticKeyProvider(std::array< uint8_t, 32 > key) noexcept`
- Source: `include/storage/encrypted_blob_backend.h`:73
- Brief: Construct with a fixed 32-byte key.
- Parameters:
  - `key` (std::array< uint8_t, 32 >): 256-bit key material.
- Details: key 256-bit key material.

#### `std::array< uint8_t, 32 > currentKey() const override`
- Source: `include/storage/encrypted_blob_backend.h`:75
- Brief: Return the current 256-bit (32-byte) encryption key.
- Parameters: none
- Details: The returned array must remain valid for the duration of the encryption/decryption call.

#### `std::string name() const override`
- Source: `include/storage/encrypted_blob_backend.h`:77
- Brief: Human-readable key provider name (e.g., "static", "kms", "hsm").
- Parameters: none

### themis::storage::StorageCapacityMetrics

#### `std::int64_t time_to_exhaustion_ms(std::uint64_t write_rate_bytes_per_sec) const noexcept`
- Source: `include/storage/storage_pressure_manager.h`:97
- Brief: n/a
- Parameters:
  - `write_rate_bytes_per_sec` (std::uint64_t): n/a
- Details: Projected time to capacity at current write rate (milliseconds). Returns -1 if write rate is zero or available space (minus reserve) is insufficient for meaningful projection (i.e., already at/below reserve).

#### `double utilization_percent() const noexcept`
- Source: `include/storage/storage_pressure_manager.h`:89
- Brief: Percentage of capacity in use (0-100).
- Parameters: none

### themis::storage::StorageLayoutAdvisor

#### `StorageLayoutAdvisor()=default`
- Source: `include/storage/storage_layout_advisor.h`:174
- Brief: n/a
- Parameters: none

#### `StorageLayoutAdvisor(const StorageLayoutAdvisor &)=default`
- Source: `include/storage/storage_layout_advisor.h`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StorageLayoutAdvisor &): n/a

#### `LayoutRecommendation analyze(const std::string &collection_name, const CollectionAccessStats &stats, const SchemaInfo &schema, const GdprFieldRegistry &gdpr_fields) const`
- Source: `include/storage/storage_layout_advisor.h`:203
- Brief: Analyse a collection and produce a layout recommendation.
- Parameters:
  - `collection_name` (const std::string &): Human-readable name of the collection.
  - `stats` (const CollectionAccessStats &): Access pattern statistics.
  - `schema` (const SchemaInfo &): Field metadata.
  - `gdpr_fields` (const GdprFieldRegistry &): Registry of GDPR-protected field paths.
- Return: Layout recommendation with rationale.
- Details: Algorithm: Determine whether the collection is time-series via isTimeSeries(). Apply the decision logic described in the class docstring. Compute compression and speedup estimates. Check whether any schema field is GDPR-protected. Emit a LAYOUT_RECOMMENDATION decision record. collection_name Human-readable name of the collection. stats Access pattern statistics. schema Field metadata. gdpr_fields Registry of GDPR-protected field paths. Layout recommendation with rationale.

#### `std::string buildRationale(LayoutType layout, const CollectionAccessStats &stats, bool gdpr_affected)`
- Source: `include/storage/storage_layout_advisor.h`:236
- Brief: Build a one-sentence rationale for the recommendation.
- Parameters:
  - `layout` (LayoutType): Input parameter.
  - `stats` (const CollectionAccessStats &): Input parameter.
  - `gdpr_affected` (bool): Input parameter.
- Return: Return value.
- Details: Build Rationale. layout Input parameter. stats Input parameter. gdpr_affected Input parameter. Return value.

#### `void emitDecisionRecord(const LayoutRecommendation &rec) const`
- Source: `include/storage/storage_layout_advisor.h`:240
- Brief: n/a
- Parameters:
  - `rec` (const LayoutRecommendation &): n/a

#### `double estimateCompressionRatio(LayoutType layout, const SchemaInfo &schema)`
- Source: `include/storage/storage_layout_advisor.h`:228
- Brief: Estimate compression ratio for the given layout and schema.
- Parameters:
  - `layout` (LayoutType): Input parameter.
  - `schema` (const SchemaInfo &): Input parameter.
- Return: Return value.
- Details: Estimate Compression Ratio. layout Input parameter. schema Input parameter. Return value.

#### `double estimateQuerySpeedup(LayoutType layout, const CollectionAccessStats &stats)`
- Source: `include/storage/storage_layout_advisor.h`:232
- Brief: Estimate query speedup for the given layout and access pattern.
- Parameters:
  - `layout` (LayoutType): Input parameter.
  - `stats` (const CollectionAccessStats &): Input parameter.
- Return: Return value.
- Details: Estimate Query Speedup. layout Input parameter. stats Input parameter. Return value.

#### `bool isTimeSeries(const CollectionAccessStats &stats) const`
- Source: `include/storage/storage_layout_advisor.h`:219
- Brief: Detect whether a collection exhibits time-series access patterns.
- Parameters:
  - `stats` (const CollectionAccessStats &): Collection access statistics.
- Return: True when time-series pattern is detected.
- Details: Returns true when the stats indicate a monotonic timestamp AND the Fourier-coefficient analysis of stats.timestamp_series shows periodic / sequential structure (variance explained > 0.3). stats Collection access statistics. True when time-series pattern is detected.

#### `std::string layoutName(LayoutType t)`
- Source: `include/storage/storage_layout_advisor.h`:222
- Brief: Human-readable name for a LayoutType value.
- Parameters:
  - `t` (LayoutType): Input parameter.
- Return: Return value.
- Details: Layout Name. t Input parameter. Return value.

#### `StorageLayoutAdvisor & operator=(const StorageLayoutAdvisor &)=default`
- Source: `include/storage/storage_layout_advisor.h`:178
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StorageLayoutAdvisor &): n/a

#### `void setDecisionRecordProcessor(std::shared_ptr< themis::llm::DecisionRecordYamlProcessor > processor)`
- Source: `include/storage/storage_layout_advisor.h`:182
- Brief: Set Decision Record Processor.
- Parameters:
  - `processor` (std::shared_ptr< themis::llm::DecisionRecordYamlProcessor >): Input parameter.
- Details: processor Input parameter.

#### `~StorageLayoutAdvisor()=default`
- Source: `include/storage/storage_layout_advisor.h`:175
- Brief: n/a
- Parameters: none

### themis::storage::StorageParquetExporter

#### `StorageParquetExporter()=default`
- Source: `include/storage/storage_parquet_exporter.h`:83
- Brief: n/a
- Parameters: none

#### `Result< std::vector< uint8_t > > buildParquet(const std::vector< std::vector< ColumnSegment > > &column_segments, const ParquetExportConfig &config)`
- Source: `include/storage/storage_parquet_exporter.h`:118
- Brief: ============================================================================ StorageParquetExporter::buildParquet (portable Parquet v2 path) ============================================================================
- Parameters:
  - `column_segments` (const std::vector< std::vector< ColumnSegment > > &): Input parameter.
  - `config` (const ParquetExportConfig &): Input parameter.
- Return: Return value.
- Details: column_segments Input parameter. config Input parameter. Return value. Calls: size(), tl::unexpected(), Error(), std::to_string(), metadata(), insert(), end(), reserve().

#### `Result< std::vector< uint8_t > > exportToBuffer(const std::vector< std::vector< ColumnSegment > > &column_segments, const ParquetExportConfig &config)`
- Source: `include/storage/storage_parquet_exporter.h`:99
- Brief: Export to an in-memory buffer (useful for testing and streaming).
- Parameters:
  - `column_segments` (const std::vector< std::vector< ColumnSegment > > &): Input parameter.
  - `config` (const ParquetExportConfig &): Input parameter.
- Return: Return value.
- Details: Export To Buffer. column_segments Input parameter. config Input parameter. Return value. Calls: std::chrono::steady_clock::now(), buildParquet(), size(), count(), empty(), metadata().

#### `Result< void > exportToFile(const std::vector< std::vector< ColumnSegment > > &column_segments, const ParquetExportConfig &config, const std::string &output_path)`
- Source: `include/storage/storage_parquet_exporter.h`:93
- Brief: Export To File.
- Parameters:
  - `column_segments` (const std::vector< std::vector< ColumnSegment > > &): Input parameter.
  - `config` (const ParquetExportConfig &): Input parameter.
  - `output_path` (const std::string &): Path to the output.
- Return: Ok(void) on success, or an Error.
- Details: Export a table to a Parquet file on disk. column_segments One entry per column; each entry is the ordered list of decoded ColumnSegments for that column (matching row partitioning across all columns). config Export configuration (column names, compression). output_path Destination file path. Ok(void) on success, or an Error. column_segments Input parameter. config Input parameter. output_path Path to the output. Return value. Calls: exportToBuffer(), tl::unexpected(), error(), out(), Error(), write(), data(), size().

#### `const ExportStats & lastStats() const noexcept`
- Source: `include/storage/storage_parquet_exporter.h`:112
- Brief: n/a
- Parameters: none

### themis::storage::StoragePressureManager

#### `StoragePressureManager()=default`
- Source: `include/storage/storage_pressure_manager.h`:131
- Brief: Constructor: initializes with default capacity thresholds.
- Parameters: none

#### `StoragePressureManager(StoragePressureManager &&) noexcept=default`
- Source: `include/storage/storage_pressure_manager.h`:138
- Brief: Move-enabled.
- Parameters:
  - `<unnamed>` (StoragePressureManager &&): n/a

#### `StoragePressureManager(const StoragePressureManager &)=delete`
- Source: `include/storage/storage_pressure_manager.h`:134
- Brief: Non-copyable.
- Parameters:
  - `<unnamed>` (const StoragePressureManager &): n/a

#### `bool canAcceptWrite(std::uint64_t requested_bytes) const noexcept`
- Source: `include/storage/storage_pressure_manager.h`:165
- Brief: Check if storage has sufficient space for a write operation.
- Parameters:
  - `requested_bytes` (std::uint64_t): Bytes needed for write operation
- Return: true if write should be accepted, false if should be rejected
- Details: Returns false if: Available space < requested_bytes + reserved_bytes Pressure level is EXHAUSTED Operation would cross reject_at_utilization threshold requested_bytes Bytes needed for write operation true if write should be accepted, false if should be rejected

#### `bool canStartBackup(std::uint64_t backup_size_estimate, int active_backup_count) const noexcept`
- Source: `include/storage/storage_pressure_manager.h`:179
- Brief: Check if storage supports a backup operation.
- Parameters:
  - `backup_size_estimate` (std::uint64_t): Expected backup size in bytes
  - `active_backup_count` (int): Current number of in-flight backups
- Return: true if backup can be started, false otherwise
- Details: Returns false if: Concurrent backup limit (kMaxConcurrentBackups) reached Available space < backup_size_estimate + reserved_bytes Pressure level is CRITICAL or EXHAUSTED backup_size_estimate Expected backup size in bytes active_backup_count Current number of in-flight backups true if backup can be started, false otherwise

#### `PressureEscalationLevel classifyPressure(double utilization_percent) const noexcept`
- Source: `include/storage/storage_pressure_manager.h`:302
- Brief: n/a
- Parameters:
  - `utilization_percent` (double): n/a

#### `StorageCapacityMetrics getCapacityMetrics() const noexcept`
- Source: `include/storage/storage_pressure_manager.h`:152
- Brief: Get current storage capacity metrics.
- Parameters: none
- Return: StorageCapacityMetrics with current usage and pressure state
- Details: StorageCapacityMetrics with current usage and pressure state Thread-safe: Returns consistent snapshot of current metrics.

#### `StoragePressureManager & operator=(StoragePressureManager &&) noexcept=default`
- Source: `include/storage/storage_pressure_manager.h`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (StoragePressureManager &&): n/a

#### `StoragePressureManager & operator=(const StoragePressureManager &)=delete`
- Source: `include/storage/storage_pressure_manager.h`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (const StoragePressureManager &): n/a

#### `StorageErrorContext reportBackupLimitExceeded(int active_backup_count, int max_concurrent_backups) noexcept`
- Source: `include/storage/storage_pressure_manager.h`:241
- Brief: Report backup limit exceeded event.
- Parameters:
  - `active_backup_count` (int): Current number of active backups
  - `max_concurrent_backups` (int): Maximum allowed concurrent backups
- Return: StorageErrorContext with HIGH severity
- Details: Called when BACKUP_LIMIT_EXCEEDED error would be returned. Emits diagnostic event. Guaranteed behavior: Emits HIGH severity diagnostic Suggests waiting for backup to complete Provides information about concurrent backup limit active_backup_count Current number of active backups max_concurrent_backups Maximum allowed concurrent backups StorageErrorContext with HIGH severity

#### `StorageErrorContext reportStorageExhausted(std::string_view operation_context, std::uint64_t total_capacity, std::uint64_t used_bytes) noexcept`
- Source: `include/storage/storage_pressure_manager.h`:221
- Brief: Report a storage exhaustion event.
- Parameters:
  - `operation_context` (std::string_view): Context of the operation that failed (e.g., "write(key=foo)", "checkpoint()")
  - `total_capacity` (std::uint64_t): Total storage capacity
  - `used_bytes` (std::uint64_t): Current usage
- Return: StorageErrorContext with CRITICAL severity
- Details: Called when STORAGE_EXHAUSTED error is detected. Emits diagnostic event and updates pressure state. Guaranteed behavior: Emits CRITICAL severity diagnostic Sets escalation level to EXHAUSTED Suggests mitigation (free space, tiering, archiving) Triggers operator alert operation_context Context of the operation that failed (e.g., "write(key=foo)", "checkpoint()") total_capacity Total storage capacity used_bytes Current usage StorageErrorContext with CRITICAL severity

#### `void setRejectBackupsAtUtilization(double percent) noexcept`
- Source: `include/storage/storage_pressure_manager.h`:271
- Brief: Set the utilization threshold at which new backups should be rejected.
- Parameters:
  - `percent` (double): Utilization percentage (0-100)
- Details: Default: 95% (equivalent to PressureEscalationLevel::CRITICAL). Backup requests when utilization >= this threshold will be rejected with BACKUP_LIMIT_EXCEEDED (or similar). percent Utilization percentage (0-100)

#### `void setRejectWritesAtUtilization(double percent) noexcept`
- Source: `include/storage/storage_pressure_manager.h`:257
- Brief: Set the utilization threshold at which writes should be rejected.
- Parameters:
  - `percent` (double): Utilization percentage (0-100)
- Details: Default: 99% (equivalent to PressureEscalationLevel::EXHAUSTED). Writes requested when utilization >= this threshold will be rejected with STORAGE_EXHAUSTED. percent Utilization percentage (0-100)

#### `void setReservedCapacity(std::uint64_t reserved_bytes) noexcept`
- Source: `include/storage/storage_pressure_manager.h`:285
- Brief: Set bytes reserved for system operations (crash recovery, temp files).
- Parameters:
  - `reserved_bytes` (std::uint64_t): Bytes to reserve for system use
- Details: Default: 5% of total capacity. Available space is computed as: total - used - reserved. This reserve ensures system operations don't fail unexpectedly under pressure. reserved_bytes Bytes to reserve for system use

#### `StorageCapacityMetrics updateCapacity(std::uint64_t total_capacity, std::uint64_t used_bytes) noexcept`
- Source: `include/storage/storage_pressure_manager.h`:200
- Brief: Update storage capacity metrics and determine pressure level.
- Parameters:
  - `total_capacity` (std::uint64_t): Total storage capacity in bytes
  - `used_bytes` (std::uint64_t): Bytes currently used
- Return: StorageCapacityMetrics with updated escalation_level
- Details: Should be called periodically or when capacity changes detected. total_capacity Total storage capacity in bytes used_bytes Bytes currently used StorageCapacityMetrics with updated escalation_level Escalation levels are determined by utilization percentage: 0-75%: NORMAL 75-85%: WARNING 85-95%: ALERT 95-99%: CRITICAL >=99%: EXHAUSTED

#### `~StoragePressureManager()=default`
- Source: `include/storage/storage_pressure_manager.h`:141
- Brief: n/a
- Parameters: none

### themis::storage::TTCore

#### `float & at(std::size_t l, std::size_t i, std::size_t r)`
- Source: `include/storage/tensor_train_decomposer.h`:43
- Brief: Access element G[l][i][r].
- Parameters:
  - `l` (std::size_t): n/a
  - `i` (std::size_t): n/a
  - `r` (std::size_t): n/a

#### `const float & at(std::size_t l, std::size_t i, std::size_t r) const`
- Source: `include/storage/tensor_train_decomposer.h`:46
- Brief: n/a
- Parameters:
  - `l` (std::size_t): n/a
  - `i` (std::size_t): n/a
  - `r` (std::size_t): n/a

#### `std::size_t numElements() const noexcept`
- Source: `include/storage/tensor_train_decomposer.h`:50
- Brief: n/a
- Parameters: none

### themis::storage::TTQuantizer

#### `TTQuantizer()=default`
- Source: `include/storage/tt_quantizer.h`:118
- Brief: n/a
- Parameters: none

#### `double bytesPerElement(QuantizationType t) noexcept`
- Source: `include/storage/tt_quantizer.h`:145
- Brief: Bytes per element for a given type (fractional for NF4 → use 0.5).
- Parameters:
  - `t` (QuantizationType): n/a

#### `TTTrain dequantize(const QuantizedTrain &qtrain) const`
- Source: `include/storage/tt_quantizer.h`:137
- Brief: Dequantise back to a (lossy) TTTrain with float32 cores.
- Parameters:
  - `qtrain` (const QuantizedTrain &): n/a

#### `TTCore dequantizeINT8(const QuantizedCore &qcore) const`
- Source: `include/storage/tt_quantizer.h`:160
- Brief: n/a
- Parameters:
  - `qcore` (const QuantizedCore &): n/a

#### `TTCore dequantizeNF4(const QuantizedCore &qcore) const`
- Source: `include/storage/tt_quantizer.h`:161
- Brief: n/a
- Parameters:
  - `qcore` (const QuantizedCore &): n/a

#### `uint8_t findNF4Index(float v) noexcept`
- Source: `include/storage/tt_quantizer.h`:164
- Brief: Find nearest index in kNF4Table for a normalised value v ∈ [-1,1].
- Parameters:
  - `v` (float): n/a

#### `QuantizedTrain quantize(const TTTrain &train, QuantizationType type=QuantizationType::INT8) const`
- Source: `include/storage/tt_quantizer.h`:131
- Brief: Quantise every core of a TTTrain.
- Parameters:
  - `train` (const TTTrain &): Source TT-train (float32 cores).
  - `type` (QuantizationType): Target quantisation type.
- Return: QuantizedTrain.
- Throws:
  - std::invalid_argument: if train.cores is empty.
- Details: train Source TT-train (float32 cores). type Target quantisation type. QuantizedTrain. std::invalid_argument if train.cores is empty.

#### `QuantizedCore quantizeINT8(const TTCore &core) const`
- Source: `include/storage/tt_quantizer.h`:157
- Brief: n/a
- Parameters:
  - `core` (const TTCore &): n/a

#### `QuantizedCore quantizeNF4(const TTCore &core) const`
- Source: `include/storage/tt_quantizer.h`:158
- Brief: n/a
- Parameters:
  - `core` (const TTCore &): n/a

#### `std::string typeName(QuantizationType t) noexcept`
- Source: `include/storage/tt_quantizer.h`:142
- Brief: Human-readable name of a QuantizationType.
- Parameters:
  - `t` (QuantizationType): n/a

#### `~TTQuantizer()=default`
- Source: `include/storage/tt_quantizer.h`:119
- Brief: n/a
- Parameters: none

### themis::storage::TTTrain

#### `double compressionRatio() const noexcept`
- Source: `include/storage/tensor_train_decomposer.h`:88
- Brief: Compression ratio vs. dense storage (∏ n_k / totalParams).
- Parameters: none

#### `std::optional< TTTrain > deserialize(const std::vector< uint8_t > &bytes)`
- Source: `include/storage/tensor_train_decomposer.h`:97
- Brief: Deserialise from bytes.
- Parameters:
  - `bytes` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Deserialize. bytes Input parameter. Return value. std::runtime_error if an error occurs. Calls: size(), std::memcpy(), readU64(), resize(), readF64(), numElements(), readF32(), THEMIS_WARN().

#### `std::size_t maxRank() const noexcept`
- Source: `include/storage/tensor_train_decomposer.h`:85
- Brief: Maximum TT-rank across all bonds.
- Parameters: none

#### `std::size_t order() const noexcept`
- Source: `include/storage/tensor_train_decomposer.h`:79
- Brief: Order (number of dimensions).
- Parameters: none

#### `std::vector< float > reconstruct() const`
- Source: `include/storage/tensor_train_decomposer.h`:91
- Brief: Reconstruct full dense tensor (use only for testing; cost = O(n^d)).
- Parameters: none

#### `std::vector< uint8_t > serialize() const`
- Source: `include/storage/tensor_train_decomposer.h`:94
- Brief: Serialise to bytes for RocksDB storage.
- Parameters: none

#### `std::size_t totalParams() const noexcept`
- Source: `include/storage/tensor_train_decomposer.h`:82
- Brief: Total number of parameters stored in all cores.
- Parameters: none

### themis::storage::TensorCompactionFilter

#### `Decision FilterV2(int level, const rocksdb::Slice &key, ValueType value_type, const rocksdb::Slice &existing_value, std::string *new_value, std::string *skip_until) const override`
- Source: `include/storage/tensor_compaction_filter.h`:76
- Brief: Compaction callback: inspect one key–value pair.
- Parameters:
  - `level` (int): n/a
  - `key` (const rocksdb::Slice &): n/a
  - `value_type` (ValueType): n/a
  - `existing_value` (const rocksdb::Slice &): n/a
  - `new_value` (std::string *): n/a
  - `skip_until` (std::string *): n/a
- Details: Returns: kKeep — key is not a TT-core key, or deserialization failed, or recompression did not reduce size. kChangeValue — recompressed value is smaller; *new_value is set.

#### `const char * Name() const override`
- Source: `include/storage/tensor_compaction_filter.h`:66
- Brief: n/a
- Parameters: none

#### `TensorCompactionFilter(double epsilon=1e-4, QuantizationType quant_type=QuantizationType::INT8) noexcept`
- Source: `include/storage/tensor_compaction_filter.h`:60
- Brief: Construct filter with compression parameters.
- Parameters:
  - `epsilon` (double): Reconstruction error tolerance ε ∈ (0, 1]. Values below the original tensor ε will have no effect. Default: 1e-4 (tight but lossless for most embeddings).
  - `quant_type` (QuantizationType): Quantisation type to apply when re-serialising QuantizedTrain values (has no effect on raw TTTrain keys). Default: INT8.
- Details: epsilon Reconstruction error tolerance ε ∈ (0, 1]. Values below the original tensor ε will have no effect. Default: 1e-4 (tight but lossless for most embeddings). quant_type Quantisation type to apply when re-serialising QuantizedTrain values (has no effect on raw TTTrain keys). Default: INT8.

#### `void clearRecompressFn()`
- Source: `include/storage/tensor_compaction_filter.h`:92
- Brief: Remove a previously injected recompress backend.
- Parameters: none
- Details: static Calls: lk(), recompressFnMutex(), recompressFnStorage().

#### `bool filterTTCore(const rocksdb::Slice &value, std::string *new_bytes) const`
- Source: `include/storage/tensor_compaction_filter.h`:112
- Brief: n/a
- Parameters:
  - `value` (const rocksdb::Slice &): n/a
  - `new_bytes` (std::string *): n/a
- Details: Process a raw TTTrain value. Returns true and sets *new_bytes if recompression produced a smaller result.

#### `bool filterTTNMeta(const rocksdb::Slice &value, std::string *new_bytes) const`
- Source: `include/storage/tensor_compaction_filter.h`:117
- Brief: n/a
- Parameters:
  - `value` (const rocksdb::Slice &): n/a
  - `new_bytes` (std::string *): n/a
- Details: Process a QuantizedTrain meta value. Returns true and sets *new_bytes if recompression produced a smaller result.

#### `bool isTTCoreKey(const rocksdb::Slice &key) noexcept`
- Source: `include/storage/tensor_compaction_filter.h`:103
- Brief: Returns true for __ttcore__: prefix (raw TTTrain values).
- Parameters:
  - `key` (const rocksdb::Slice &): n/a

#### `bool isTTNMetaKey(const rocksdb::Slice &key) noexcept`
- Source: `include/storage/tensor_compaction_filter.h`:106
- Brief: Returns true for __ttn__:...:meta: suffix (QuantizedTrain headers).
- Parameters:
  - `key` (const rocksdb::Slice &): n/a

#### `void setRecompressFn(RecompressFn fn)`
- Source: `include/storage/tensor_compaction_filter.h`:89
- Brief: Inject a recompress backend (e.g. LAPACK dgesdd wrapper).
- Parameters:
  - `fn` (RecompressFn): Input parameter.
- Details: static When set, filterTTCore()/filterTTNMeta() delegate recompression to this callback instead of using TensorTrainDecomposer::recompress(). fn Input parameter. Calls: lk(), recompressFnMutex(), recompressFnStorage(), std::move().

#### `~TensorCompactionFilter() override=default`
- Source: `include/storage/tensor_compaction_filter.h`:64
- Brief: n/a
- Parameters: none

### themis::storage::TensorFieldKey

#### `bool operator==(const TensorFieldKey &o) const noexcept`
- Source: `include/storage/tensor_network_storage_engine.h`:45
- Brief: n/a
- Parameters:
  - `o` (const TensorFieldKey &): n/a

### themis::storage::TensorFieldKeyHash

#### `std::size_t operator()(const TensorFieldKey &k) const noexcept`
- Source: `include/storage/tensor_network_storage_engine.h`:51
- Brief: n/a
- Parameters:
  - `k` (const TensorFieldKey &): n/a

### themis::storage::TensorNetworkStorageEngine

#### `TensorNetworkStorageEngine(std::shared_ptr< ITensorStorageBackend > backend, const TensorStorageConfig &cfg={})`
- Source: `include/storage/tensor_network_storage_engine.h`:226
- Brief: Construct with a storage backend and configuration.
- Parameters:
  - `backend` (std::shared_ptr< ITensorStorageBackend >): Shared pointer to the KV-store backend.
  - `cfg` (const TensorStorageConfig &): Storage configuration.
- Throws:
  - std::invalid_argument: if backend is null.
- Details: backend Shared pointer to the KV-store backend. cfg Storage configuration. std::invalid_argument if backend is null.

#### `void compact(const TensorFieldKey &key)`
- Source: `include/storage/tensor_network_storage_engine.h`:282
- Brief: Delete all versions below keep_versions newest ones.
- Parameters:
  - `key` (const TensorFieldKey &): Input parameter.
- Details: Compact. key Input parameter. Calls: wlk(), currentVersion(), listKeys(), makePrefix(), tryParseVersionSuffix(), del().

#### `const TensorStorageConfig & config() const noexcept`
- Source: `include/storage/tensor_network_storage_engine.h`:297
- Brief: Configuration used by this engine.
- Parameters: none

#### `std::size_t currentVersion(const TensorFieldKey &k) const`
- Source: `include/storage/tensor_network_storage_engine.h`:396
- Brief: n/a
- Parameters:
  - `k` (const TensorFieldKey &): n/a

#### `bool deleteRawMetadata(const std::string &key)`
- Source: `include/storage/tensor_network_storage_engine.h`:357
- Brief: Delete an opaque metadata blob stored under key.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: True if the key existed and was removed.
- Details: Delete Raw Metadata. True if the key existed and was removed. key Input parameter. True when the operation succeeds. Calls: del(), rawMetaKey().

#### `void eraseVersion(const TensorFieldKey &k)`
- Source: `include/storage/tensor_network_storage_engine.h`:398
- Brief: Erase Version.
- Parameters:
  - `k` (const TensorFieldKey &): Input parameter.
- Details: k Input parameter. Calls: lk(), erase().

#### `std::optional< std::vector< float > > get(const TensorFieldKey &key) const`
- Source: `include/storage/tensor_network_storage_engine.h`:258
- Brief: Retrieve and reconstruct the latest version of a tensor field.
- Parameters:
  - `key` (const TensorFieldKey &): n/a
- Return: Reconstructed float32 tensor, or std::nullopt if not found.
- Details: Reconstructed float32 tensor, or std::nullopt if not found.

#### `std::optional< QuantizedTrain > getCompressed(const TensorFieldKey &key) const`
- Source: `include/storage/tensor_network_storage_engine.h`:270
- Brief: Retrieve in compressed (QuantizedTrain) form — avoids decompression.
- Parameters:
  - `key` (const TensorFieldKey &): n/a

#### `std::optional< std::vector< uint8_t > > getRawMetadata(const std::string &key) const`
- Source: `include/storage/tensor_network_storage_engine.h`:350
- Brief: Load an opaque byte blob stored under key.
- Parameters:
  - `key` (const std::string &): n/a
- Return: The blob, or std::nullopt if not found.
- Details: The blob, or std::nullopt if not found.

#### `std::optional< std::vector< float > > getVersion(const TensorFieldKey &key, std::size_t version) const`
- Source: `include/storage/tensor_network_storage_engine.h`:264
- Brief: Retrieve a specific version.
- Parameters:
  - `key` (const TensorFieldKey &): n/a
  - `version` (std::size_t): n/a

#### `std::vector< std::string > listRawMetadataKeys(const std::string &prefix) const`
- Source: `include/storage/tensor_network_storage_engine.h`:366
- Brief: List logical metadata keys whose names start with prefix.
- Parameters:
  - `prefix` (const std::string &): n/a
- Details: Returned keys are relative to prefix and do not include the internal __tfgmeta__: namespace prefix.

#### `std::optional< QuantizedTrain > loadQuantizedTrain(const TensorFieldKey &key, std::size_t version) const`
- Source: `include/storage/tensor_network_storage_engine.h`:407
- Brief: n/a
- Parameters:
  - `key` (const TensorFieldKey &): n/a
  - `version` (std::size_t): n/a

#### `std::string makeCoreKey(const TensorFieldKey &k, std::size_t core_idx, std::size_t ver)`
- Source: `include/storage/tensor_network_storage_engine.h`:385
- Brief: Make Core Key.
- Parameters:
  - `k` (const TensorFieldKey &): Input parameter.
  - `core_idx` (std::size_t): Input parameter.
  - `ver` (std::size_t): Input parameter.
- Return: Return value.
- Details: k Input parameter. core_idx Input parameter. ver Input parameter. Return value. Calls: makePrefix(), std::to_string().

#### `std::string makeMetaKey(const TensorFieldKey &k, std::size_t ver)`
- Source: `include/storage/tensor_network_storage_engine.h`:384
- Brief: Make Meta Key.
- Parameters:
  - `k` (const TensorFieldKey &): Input parameter.
  - `ver` (std::size_t): Input parameter.
- Return: Return value.
- Details: k Input parameter. ver Input parameter. Return value. Calls: makePrefix(), std::to_string().

#### `std::string makePrefix(const TensorFieldKey &k)`
- Source: `include/storage/tensor_network_storage_engine.h`:387
- Brief: Make Prefix.
- Parameters:
  - `k` (const TensorFieldKey &): Input parameter.
- Return: Return value.
- Details: k Input parameter. Return value. Implements makePrefix without additional internal calls.

#### `bool persistQuantizedTrain(const TensorFieldKey &key, const QuantizedTrain &qtrain, std::size_t version)`
- Source: `include/storage/tensor_network_storage_engine.h`:402
- Brief: Persist Quantized Train.
- Parameters:
  - `key` (const TensorFieldKey &): Input parameter.
  - `qtrain` (const QuantizedTrain &): Input parameter.
  - `version` (std::size_t): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. qtrain Input parameter. version Input parameter. True when the operation succeeds.

#### `bool put(const TensorFieldKey &key, const std::vector< float > &data, const std::vector< std::size_t > &mode_sizes)`
- Source: `include/storage/tensor_network_storage_engine.h`:246
- Brief: Compress and store a tensor field.
- Parameters:
  - `key` (const TensorFieldKey &): Input parameter.
  - `data` (const std::vector< float > &): Input parameter.
  - `mode_sizes` (const std::vector< std::size_t > &): Input parameter.
- Return: True on success.
- Throws:
  - std::invalid_argument: on shape mismatch.
  - std::invalid_argument: if an error occurs.
- Details: Put. Decomposes data into TT format, quantises the cores, and persists each core under the structured key schema. key Logical field address. data Flat row-major float32 tensor. mode_sizes Shape of the tensor (∏ mode_sizes == data.size()). True on success. std::invalid_argument on shape mismatch. key Input parameter. data Input parameter. mode_sizes Input parameter. True when the operation succeeds. std::invalid_argument if an error occurs. Calls: size(), decompose(), quantize(), wlk(), currentVersion(), persistQuantizedTrain(), setVersion(), del().

#### `bool putRawMetadata(const std::string &key, const std::vector< uint8_t > &value)`
- Source: `include/storage/tensor_network_storage_engine.h`:341
- Brief: Store an opaque byte blob under a named metadata key.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::vector< uint8_t > &): Input parameter.
- Return: True on success.
- Details: Put Raw Metadata. The key is namespaced to __tfgmeta__:<key> so it cannot collide with regular tensor keys. Used by TensorDeduplicationManager to persist the fingerprint graph snapshot between process restarts. key Logical metadata key (must be non-empty). value Raw byte payload. True on success. key Input parameter. value Input parameter. True when the operation succeeds. Calls: put(), rawMetaKey().

#### `bool remove(const TensorFieldKey &key)`
- Source: `include/storage/tensor_network_storage_engine.h`:277
- Brief: Remove the latest version of a field.
- Parameters:
  - `key` (const TensorFieldKey &): Input parameter.
- Return: True when the operation succeeds.
- Details: Remove. key Input parameter. True when the operation succeeds. Calls: wlk(), currentVersion(), loadQuantizedTrain(), del(), makeMetaKey(), size(), makeCoreKey(), eraseVersion().

#### `void setDeleteObserverFn(TensorDeleteObserverFn fn)`
- Source: `include/storage/tensor_network_storage_engine.h`:326
- Brief: Register (or replace) the delete observer. Pass nullptr to remove.
- Parameters:
  - `fn` (TensorDeleteObserverFn): Input parameter.
- Details: Set Delete Observer Fn. fn Input parameter. Calls: lk(), std::move().

#### `void setVersion(const TensorFieldKey &k, std::size_t version)`
- Source: `include/storage/tensor_network_storage_engine.h`:397
- Brief: Set Version.
- Parameters:
  - `k` (const TensorFieldKey &): Input parameter.
  - `version` (std::size_t): Input parameter.
- Details: k Input parameter. version Input parameter. Calls: lk(), erase().

#### `void setWriteObserverFn(TensorWriteObserverFn fn)`
- Source: `include/storage/tensor_network_storage_engine.h`:323
- Brief: Register (or replace) the write observer. Pass nullptr to remove.
- Parameters:
  - `fn` (TensorWriteObserverFn): Input parameter.
- Details: Set Write Observer Fn. fn Input parameter. Calls: lk(), std::move().

#### `std::optional< TensorStorageStats > stats(const TensorFieldKey &key) const`
- Source: `include/storage/tensor_network_storage_engine.h`:292
- Brief: Statistics for the latest version of a field.
- Parameters:
  - `key` (const TensorFieldKey &): n/a
- Return: std::nullopt if field not found.
- Details: std::nullopt if field not found.

#### `~TensorNetworkStorageEngine()=default`
- Source: `include/storage/tensor_network_storage_engine.h`:230
- Brief: n/a
- Parameters: none

### themis::storage::TensorRouter

#### `TensorRouter(std::shared_ptr< TensorNetworkStorageEngine > engine, TensorRoutingPolicy policy={})`
- Source: `include/storage/tensor_router.h`:273
- Brief: n/a
- Parameters:
  - `engine` (std::shared_ptr< TensorNetworkStorageEngine >): n/a
  - `policy` (TensorRoutingPolicy): n/a

#### `void clearTemplateTopologyApplyFn()`
- Source: `include/storage/tensor_router.h`:381
- Brief: Clear Template Topology Apply Fn.
- Parameters: none
- Details: Calls: lk().

#### `Route decide(const DataProfile &p) noexcept`
- Source: `include/storage/tensor_router.h`:215
- Brief: Heuristic routing from a DataProfile (no pilot, no engine).
- Parameters:
  - `p` (const DataProfile &): n/a
- Details: Applies dimension and κ thresholds without running a TT-SVD pilot. Used by TensorIndexManager at index-creation time when no concrete data sample is available yet. Thresholds (from research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md §3.2): κ ≥ 1.7 AND dim ≥ 256 → TENSOR_TRAIN κ ≥ 1.3 → HYBRID otherwise → HNSW

#### `std::string explain(const std::vector< float > &data, const std::vector< std::size_t > &mode_sizes, const TensorRouteHint &hint={}) const`
- Source: `include/storage/tensor_router.h`:308
- Brief: Explain the routing decision in human-readable form.
- Parameters:
  - `data` (const std::vector< float > &): n/a
  - `mode_sizes` (const std::vector< std::size_t > &): n/a
  - `hint` (const TensorRouteHint &): n/a
- Details: Returns a JSON object with the decision, reason, estimated compression ratio, pilot TT-rank, and applied policy thresholds.

#### `TemplateTopologyApplyFn getTemplateTopologyApplyFn() const`
- Source: `include/storage/tensor_router.h`:383
- Brief: n/a
- Parameters: none

#### `bool hasTemplateTopologyApplyFn() const`
- Source: `include/storage/tensor_router.h`:382
- Brief: n/a
- Parameters: none

#### `const TensorRoutingPolicy & policy() const noexcept`
- Source: `include/storage/tensor_router.h`:324
- Brief: n/a
- Parameters: none

#### `TensorRouteDecision route(const std::vector< float > &data, const std::vector< std::size_t > &mode_sizes, const TensorRouteHint &hint={}) const`
- Source: `include/storage/tensor_router.h`:297
- Brief: Decide routing for a tensor before storage.
- Parameters:
  - `data` (const std::vector< float > &): Flat tensor data (float32).
  - `mode_sizes` (const std::vector< std::size_t > &): Shape.
  - `hint` (const TensorRouteHint &): Optional caller-supplied hints.
- Return: Routing decision.
- Details: Probes compressibility via a pilot TT-SVD on a data sample, applies policy thresholds, and returns the routing decision. data Flat tensor data (float32). mode_sizes Shape. hint Optional caller-supplied hints. Routing decision. STUB/SIMULATION NOTE: Purpose: Heuristic routing using TT-SVD pilot on sample. Activation: Always active. ML routing gated by policy.use_ml_routing. Production Delta: ML model uses a gradient-boosted decision tree trained on historical compression ratios, access patterns, and data categories. Removal Plan: Replace heuristic branch with ML inference Q2 2027.

#### `void setPolicy(TensorRoutingPolicy p)`
- Source: `include/storage/tensor_router.h`:325
- Brief: Set Policy.
- Parameters:
  - `p` (TensorRoutingPolicy): Input parameter.
- Details: p Input parameter. Calls: std::move().

#### `void setTemplateCatalog(std::shared_ptr< tensor::TemplateCatalog > catalog)`
- Source: `include/storage/tensor_router.h`:368
- Brief: Wire a TemplateCatalog for domain-tag-based routing promotion.
- Parameters:
  - `catalog` (std::shared_ptr< tensor::TemplateCatalog >): Input parameter.
- Details: Set Template Catalog. When a non-null catalog is set, route() checks the catalog for a matching template whenever TensorRouteHint::domain_tag is non-empty. On a catalog hit the router first validates the template topology via validateTemplate(). Only a compatible template (validation succeeds) and a successfully applied callback (returns true) promote the decision to LIFT. If validation fails, the callback is skipped and the router falls back to the pilot-based heuristic with a warning log entry. Passing nullptr disables template-catalog lookups (default). catalog Input parameter.

#### `void setTemplateTopologyApplyFn(TemplateTopologyApplyFn fn)`
- Source: `include/storage/tensor_router.h`:380
- Brief: Install/remove/read the template-topology embedding callback.
- Parameters:
  - `fn` (TemplateTopologyApplyFn): Input parameter.
- Details: Set Template Topology Apply Fn. Thread-safe. The callback is invoked on TemplateCatalog hits. fn Input parameter. Calls: lk(), std::move().

#### `RouterStats stats() const noexcept`
- Source: `include/storage/tensor_router.h`:322
- Brief: n/a
- Parameters: none

#### `std::shared_ptr< tensor::TemplateCatalog > templateCatalog() const noexcept`
- Source: `include/storage/tensor_router.h`:373
- Brief: Return the currently wired TemplateCatalog (may be nullptr).
- Parameters: none

#### `TemplateValidationResult validateTemplate(const tensor::TensorNetworkGraph &graph, const std::vector< std::size_t > &mode_sizes) noexcept`
- Source: `include/storage/tensor_router.h`:266
- Brief: Validate a template graph against a concrete set of mode sizes.
- Parameters:
  - `graph` (const tensor::TensorNetworkGraph &): Template graph to validate.
  - `mode_sizes` (const std::vector< std::size_t > &): Shape of the tensor data the template will be applied to (same vector that is passed to route()).
- Return: TemplateValidationResult::valid == true when compatible; false with a diagnostic reason on any constraint violation.
- Details: Checks structural compatibility of graph with the data shape described by mode_sizes before the template is applied to index construction. The router calls this internally before invoking TemplateTopologyApplyFn; callers may use it independently for pre-flight checks. Validation rules (in priority order): graph must be non-empty (at least one node). mode_sizes must be non-empty (topology cannot be checked without shape information). Every node's mode_index must be strictly less than mode_sizes.size() — a node referencing a non-existent mode indicates a topology mismatch between the template and the data. When validation fails the router skips TemplateTopologyApplyFn and falls back to the pilot-based heuristic path, logging a warning with the returned reason string. graph Template graph to validate. mode_sizes Shape of the tensor data the template will be applied to (same vector that is passed to route()). TemplateValidationResult::valid == true when compatible; false with a diagnostic reason on any constraint violation. noexcept — never throws; all error information is returned in the TemplateValidationResult.

#### `~TensorRouter()`
- Source: `include/storage/tensor_router.h`:277
- Brief: n/a
- Parameters: none

### themis::storage::TensorRouter::Impl

#### `std::optional< TensorRouteDecision > categoryOverride(const TensorRouteHint &hint) const`
- Source: `src/storage/tensor_router.cpp`:201
- Brief: n/a
- Parameters:
  - `hint` (const TensorRouteHint &): n/a

#### `TensorRouteDecision decide(const PilotResult &pilot, const TensorRouteHint &hint, const std::vector< std::size_t > &mode_sizes) const`
- Source: `src/storage/tensor_router.cpp`:251
- Brief: n/a
- Parameters:
  - `pilot` (const PilotResult &): n/a
  - `hint` (const TensorRouteHint &): n/a
  - `mode_sizes` (const std::vector< std::size_t > &): n/a

#### `PilotResult runPilot(const std::vector< float > &data, const std::vector< std::size_t > &mode_sizes) const`
- Source: `src/storage/tensor_router.cpp`:127
- Brief: n/a
- Parameters:
  - `data` (const std::vector< float > &): n/a
  - `mode_sizes` (const std::vector< std::size_t > &): n/a

### themis::storage::TensorTrainDecomposer

#### `TensorTrainDecomposer()=default`
- Source: `include/storage/tensor_train_decomposer.h`:194
- Brief: n/a
- Parameters: none

#### `TensorTrainDecomposer(const TensorTrainDecomposer &)=default`
- Source: `include/storage/tensor_train_decomposer.h`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TensorTrainDecomposer &): n/a

#### `double cosineSimilarity(const TTTrain &a, const TTTrain &b)`
- Source: `include/storage/tensor_train_decomposer.h`:282
- Brief: Cosine similarity cos(A,B) = ⟨A,B⟩ / (‖A‖·‖B‖) in [−1, 1].
- Parameters:
  - `a` (const TTTrain &): Input parameter.
  - `b` (const TTTrain &): Input parameter.
- Return: Return value.
- Details: Cosine Similarity. Returns 0.0 when either norm is zero. a Input parameter. b Input parameter. Return value. Calls: frobeniusNorm(), innerProduct().

#### `std::pair< TTTrain, DecompositionStats > decompose(const std::vector< float > &data, const std::vector< std::size_t > &mode_sizes, const TensorTrainConfig &cfg={}) const`
- Source: `include/storage/tensor_train_decomposer.h`:212
- Brief: Decompose a dense float32 tensor into TT-format.
- Parameters:
  - `data` (const std::vector< float > &): Flat row-major tensor data (length = ∏ mode_sizes).
  - `mode_sizes` (const std::vector< std::size_t > &): Sizes of each mode (d ≥ 2).
  - `cfg` (const TensorTrainConfig &): Decomposition configuration.
- Return: Pair {TTTrain, DecompositionStats}.
- Throws:
  - std::invalid_argument: if data.size() != ∏ mode_sizes or d < 2.
- Details: data Flat row-major tensor data (length = ∏ mode_sizes). mode_sizes Sizes of each mode (d ≥ 2). cfg Decomposition configuration. Pair {TTTrain, DecompositionStats}. std::invalid_argument if data.size() != ∏ mode_sizes or d < 2.

#### `std::pair< TTTrain, DecompositionStats > decomposeF64(const std::vector< double > &data, const std::vector< std::size_t > &mode_sizes, const TensorTrainConfig &cfg={}) const`
- Source: `include/storage/tensor_train_decomposer.h`:220
- Brief: Decompose a dense float64 tensor (downcast to float32 for cores).
- Parameters:
  - `data` (const std::vector< double > &): n/a
  - `mode_sizes` (const std::vector< std::size_t > &): n/a
  - `cfg` (const TensorTrainConfig &): n/a

#### `double frobeniusNorm(const TTTrain &a)`
- Source: `include/storage/tensor_train_decomposer.h`:275
- Brief: Compute ‖A‖_F = sqrt(⟨A,A⟩) in compressed domain.
- Parameters:
  - `a` (const TTTrain &): Input parameter.
- Return: Return value.
- Details: Frobenius Norm. a Input parameter. Return value. Calls: innerProduct(), std::sqrt(), std::max().

#### `double innerProduct(const TTTrain &a, const TTTrain &b)`
- Source: `include/storage/tensor_train_decomposer.h`:270
- Brief: Compute ⟨A, B⟩ = ∑ A(i₁…id)·B(i₁…id) without reconstructing.
- Parameters:
  - `a` (const TTTrain &): Input parameter.
  - `b` (const TTTrain &): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if A and B have incompatible mode_sizes.
  - std::invalid_argument: if an error occurs.
- Details: ============================================================================ Inner product / cosine similarity in compressed domain ============================================================================ Complexity: O(d · n · r³) using the transfer-matrix technique. std::invalid_argument if A and B have incompatible mode_sizes. a Input parameter. b Input parameter. Return value. std::invalid_argument if an error occurs. Calls: size(), Mnew(), std::abs(), at(), std::move().

#### `std::vector< float > matMul(const std::vector< float > &A, const std::vector< float > &B, std::size_t m, std::size_t k, std::size_t n)`
- Source: `include/storage/tensor_train_decomposer.h`:328
- Brief: Matrix multiply C = A·B where A is (m×k) and B is (k×n) — row-major.
- Parameters:
  - `A` (const std::vector< float > &): Input parameter.
  - `B` (const std::vector< float > &): Input parameter.
  - `m` (std::size_t): Input parameter.
  - `k` (std::size_t): Input parameter.
  - `n` (std::size_t): Input parameter.
- Return: Return value.
- Details: Mat Mul. A Input parameter. B Input parameter. m Input parameter. k Input parameter. n Input parameter. Return value. Calls: C().

#### `TensorTrainDecomposer & operator=(const TensorTrainDecomposer &)=default`
- Source: `include/storage/tensor_train_decomposer.h`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TensorTrainDecomposer &): n/a

#### `TTTrain recompress(const TTTrain &train, const TensorTrainConfig &cfg) const`
- Source: `include/storage/tensor_train_decomposer.h`:259
- Brief: Re-compress an existing TTTrain without full reconstruction.
- Parameters:
  - `train` (const TTTrain &): Source TT-train.
  - `cfg` (const TensorTrainConfig &): Target configuration (eps / max_rank). Typically the same or tighter than the original decomposition parameters.
- Return: Recompressed TTTrain with equal or lower bond dimensions.
- Details: Implements the efficient TT-rounding algorithm (Oseledets 2011, Algorithm 2): Right-to-left LQ orthogonalisation (Modified Gram-Schmidt on rows) redistributes the Frobenius norm into the first core. Left-to-right truncated-SVD sweep discards singular values below δ = ε · ‖T‖_F / √(d−1), reducing bond dimensions. Complexity: O(d · r² · n) — avoids the O(∏ n_k) cost of full reconstruction used by round(). Never increases any bond dimension: if the train is already compact at the requested eps, it is returned unchanged (same ranks). train Source TT-train. cfg Target configuration (eps / max_rank). Typically the same or tighter than the original decomposition parameters. Recompressed TTTrain with equal or lower bond dimensions.

#### `TTTrain round(const TTTrain &train, const TensorTrainConfig &cfg) const`
- Source: `include/storage/tensor_train_decomposer.h`:236
- Brief: Re-compress an existing TTTrain to a tighter eps or lower max_rank.
- Parameters:
  - `train` (const TTTrain &): Source TT-train.
  - `cfg` (const TensorTrainConfig &): New (tighter) configuration.
- Return: Rounded TTTrain with reduced ranks.
- Details: Implements right-to-left orthogonalisation + left-to-right truncation (Oseledets 2011, Algorithm 2 / TT-rounding). train Source TT-train. cfg New (tighter) configuration. Rounded TTTrain with reduced ranks.

#### `void truncatedSVD(const std::vector< float > &mat, std::size_t m, std::size_t n, double delta, std::size_t max_rank_cap, std::vector< float > &U, std::vector< float > &S, std::vector< float > &Vt, std::size_t &rank_out)`
- Source: `include/storage/tensor_train_decomposer.h`:317
- Brief: Truncated SVD.
- Parameters:
  - `mat` (const std::vector< float > &): Input parameter.
  - `m` (std::size_t): Input parameter.
  - `n` (std::size_t): Input parameter.
  - `delta` (double): Input parameter.
  - `max_rank_cap` (std::size_t): Input parameter.
  - `U` (std::vector< float > &): Input/output parameter.
  - `S` (std::vector< float > &): Input/output parameter.
  - `Vt` (std::vector< float > &): Input/output parameter.
  - `rank_out` (std::size_t &): Input/output parameter.
- Details: Perform truncated SVD of an m×n matrix. Returns U, S, Vt truncated to rank columns/rows (rank chosen so that σ_{rank+1} ≤ delta, or by max_rank cap). Uses Householder bidiagonalisation (Golub-Reinsch). Exposed for other tensor decomposers (e.g. HT) to reuse the same numerically robust truncation routine. mat Input parameter. m Input parameter. n Input parameter. delta Input parameter. max_rank_cap Input parameter. U Input/output parameter. S Input/output parameter. Vt Input/output parameter. rank_out Input/output parameter.

#### `void truncatedSVDShared(const std::vector< float > &mat, std::size_t m, std::size_t n, double delta, std::size_t max_rank_cap, std::vector< float > &U, std::vector< float > &S, std::vector< float > &Vt, std::size_t &rank_out)`
- Source: `include/storage/tensor_train_decomposer.h`:301
- Brief: Shared truncated-SVD helper for cross-decomposer reuse.
- Parameters:
  - `mat` (const std::vector< float > &): Input parameter.
  - `m` (std::size_t): Input parameter.
  - `n` (std::size_t): Input parameter.
  - `delta` (double): Input parameter.
  - `max_rank_cap` (std::size_t): Input parameter.
  - `U` (std::vector< float > &): Input/output parameter.
  - `S` (std::vector< float > &): Input/output parameter.
  - `Vt` (std::vector< float > &): Input/output parameter.
  - `rank_out` (std::size_t &): Input/output parameter.
- Details: Truncated SVDShared. Exposes the production Golub-Reinsch truncated SVD implementation used internally by TT-SVD so other decomposers (for example HT) can avoid maintaining duplicate low-level SVD code paths. mat Input matrix in row-major layout (m×n). m Number of rows. n Number of columns. delta Truncation threshold for singular values. max_rank_cap Hard cap for retained rank (0 = no extra cap). U Output left singular vectors (m×rank_out). S Output singular values (rank_out). Vt Output right singular vectors transposed (rank_out×n). rank_out Chosen truncated rank (>= 1 for non-empty matrices). mat Input parameter. m Input parameter. n Input parameter. delta Input parameter. max_rank_cap Input parameter. U Input/output parameter. S Input/output parameter. Vt Input/output parameter. rank_out Input/output parameter.

#### `double vecNorm(const std::vector< float > &v) noexcept`
- Source: `include/storage/tensor_train_decomposer.h`:335
- Brief: Compute Frobenius norm of a flat vector.
- Parameters:
  - `v` (const std::vector< float > &): n/a

#### `~TensorTrainDecomposer()=default`
- Source: `include/storage/tensor_train_decomposer.h`:195
- Brief: n/a
- Parameters: none

### themis::storage::TieredStorageManager

#### `TieredStorageManager(TieredStorageManager &&)=delete`
- Source: `include/storage/tiered_storage.h`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (TieredStorageManager &&): n/a

#### `TieredStorageManager(const TieredStorageConfig &config=TieredStorageConfig{})`
- Source: `include/storage/tiered_storage.h`:153
- Brief: n/a
- Parameters:
  - `config` (const TieredStorageConfig &): n/a

#### `TieredStorageManager(const TieredStorageManager &)=delete`
- Source: `include/storage/tiered_storage.h`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TieredStorageManager &): n/a

#### `AccessTracker & accessTracker()`
- Source: `include/storage/tiered_storage.h`:210
- Brief: n/a
- Parameters: none

#### `const AccessTracker & accessTracker() const`
- Source: `include/storage/tiered_storage.h`:211
- Brief: n/a
- Parameters: none

#### `const TieredStorageConfig & config() const`
- Source: `include/storage/tiered_storage.h`:209
- Brief: n/a
- Parameters: none

#### `bool del(const std::string &key)`
- Source: `include/storage/tiered_storage.h`:180
- Brief: Delete a key from all tiers.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: true if the key existed in at least one tier.
- Details: Del. true if the key existed in at least one tier. key Input parameter. True when the operation succeeds. Calls: deleteFromTier(), remove().

#### `bool deleteFromTier(const std::string &key, StorageTierLevel tier)`
- Source: `include/storage/tiered_storage.h`:278
- Brief: Delete From Tier.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `tier` (StorageTierLevel): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. tier Input parameter. True when the operation succeeds. Calls: keyFilePath(), fs::exists(), fs::remove(), THEMIS_WARN(), message().

#### `void emitPromotionEvent(const std::string &key, access_model::TierLevel from_tier, uint64_t access_count, int64_t access_window_secs)`
- Source: `include/storage/tiered_storage.h`:268
- Brief: ───────────────────────────────────────────────────────────────────────────── Phase 5: BLOCK 3 Storage Integration — Promotion Event Emission ─────────────────────────────────────────────────────────────────────────────
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `from_tier` (access_model::TierLevel): Input parameter.
  - `access_count` (uint64_t): Input parameter.
  - `access_window_secs` (int64_t): Input parameter.
- Details: key Input parameter. from_tier Input parameter. access_count Input parameter. access_window_secs Input parameter. Calls: lock(), onStorageAccess(), std::chrono::seconds().

#### `bool existsInTier(const std::string &key, StorageTierLevel tier) const`
- Source: `include/storage/tiered_storage.h`:279
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `tier` (StorageTierLevel): n/a

#### `std::string get(const std::string &key)`
- Source: `include/storage/tiered_storage.h`:174
- Brief: Read a value from whichever tier holds it.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: empty string if not found.
- Details: Get. empty string if not found. key Input parameter. Return value. Calls: snapshot(), find(), end(), existsInTier(), recordRead(), std::chrono::system_clock::now(), std::chrono::system_clock::from_time_t(), std::chrono::system_clock::to_time_t().

#### `std::string keyFilePath(const std::string &key, StorageTierLevel tier) const`
- Source: `include/storage/tiered_storage.h`:273
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `tier` (StorageTierLevel): n/a

#### `bool migrateKey(const std::string &key, StorageTierLevel from, StorageTierLevel to)`
- Source: `include/storage/tiered_storage.h`:283
- Brief: Migrate Key.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `from` (StorageTierLevel): Input parameter.
  - `to` (StorageTierLevel): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. from Input parameter. to Input parameter. True when the operation succeeds. Calls: existsInTier(), THEMIS_WARN(), readFromTier(), empty(), writeToTier(), THEMIS_ERROR(), deleteFromTier(), setTier().

#### `TieredStorageManager & operator=(TieredStorageManager &&)=delete`
- Source: `include/storage/tiered_storage.h`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (TieredStorageManager &&): n/a

#### `TieredStorageManager & operator=(const TieredStorageManager &)=delete`
- Source: `include/storage/tiered_storage.h`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (const TieredStorageManager &): n/a

#### `bool put(const std::string &key, const std::string &value)`
- Source: `include/storage/tiered_storage.h`:168
- Brief: Write a value to the hot tier.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
- Return: false on I/O failure.
- Details: Put. false on I/O failure. key Input parameter. value Input parameter. True when the operation succeeds. Calls: writeToTier(), deleteFromTier(), recordWrite(), size().

#### `std::string readFromTier(const std::string &key, StorageTierLevel tier) const`
- Source: `include/storage/tiered_storage.h`:277
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `tier` (StorageTierLevel): n/a

#### `uint32_t runMigrationCycle()`
- Source: `include/storage/tiered_storage.h`:242
- Brief: Immediately evaluate migration candidates and move them.
- Parameters: none
- Return: Number of keys migrated.
- Details: Run Migration Cycle. Number of keys migrated. Return value. Calls: snapshot(), migrateKey(), daysSince(), existsInTier(), std::chrono::system_clock::now(), count(), emitPromotionEvent(), THEMIS_INFO().

#### `void setPromotionListener(access_model::PromotionListener *listener) noexcept`
- Source: `include/storage/tiered_storage.h`:234
- Brief: Register a promotion listener for AccessCoordinator integration.
- Parameters:
  - `listener` (access_model::PromotionListener *): Pointer to a PromotionListener implementation; nullptr disables promotion notifications.
- Details: Once registered, the TieredStorageManager notifies the listener when detecting hot access patterns (candidates for promotion to cache L3). Pass nullptr to unregister. Typical usage: autocoordinator=std::make_shared<access_model::AccessCoordinator>(); storage->setPromotionListener(coordinator.get()); listener Pointer to a PromotionListener implementation; nullptr disables promotion notifications. include/access_model/access_coordinator.h docs/architecture/UNIFIED_ACCESS_MODEL.md

#### `void startMigrationWorker()`
- Source: `include/storage/tiered_storage.h`:190
- Brief: Start the background migration worker (idempotent).
- Parameters: none
- Details: Start Migration Worker. Calls: exchange(), std::thread().

#### `Stats stats() const`
- Source: `include/storage/tiered_storage.h`:207
- Brief: n/a
- Parameters: none

#### `void stopMigrationWorker()`
- Source: `include/storage/tiered_storage.h`:193
- Brief: Stop the background migration worker and wait for it to finish.
- Parameters: none
- Details: Stop Migration Worker. Calls: exchange(), lock(), notify_all(), joinable(), utils::joinThreadWithin(), THEMIS_WARN().

#### `StorageTierLevel tierOf(const std::string &key) const`
- Source: `include/storage/tiered_storage.h`:185
- Brief: Return the current tier of a key (HOT if unknown).
- Parameters:
  - `key` (const std::string &): n/a

#### `std::string tierPath(StorageTierLevel tier) const`
- Source: `include/storage/tiered_storage.h`:272
- Brief: n/a
- Parameters:
  - `tier` (StorageTierLevel): n/a

#### `void workerLoop()`
- Source: `include/storage/tiered_storage.h`:265
- Brief: Worker Loop.
- Parameters: none
- Details: Calls: THEMIS_INFO(), load(), lock(), wait_for(), std::chrono::seconds(), unlock(), runMigrationCycle(), THEMIS_ERROR().

#### `bool writeToTier(const std::string &key, const std::string &value, StorageTierLevel tier)`
- Source: `include/storage/tiered_storage.h`:275
- Brief: Write To Tier.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `value` (const std::string &): Input parameter.
  - `tier` (StorageTierLevel): Input parameter.
- Return: True when the operation succeeds.
- Details: key Input parameter. value Input parameter. tier Input parameter. True when the operation succeeds. Calls: keyFilePath(), f(), THEMIS_ERROR(), write(), data(), size(), good(), what().

#### `~TieredStorageManager()`
- Source: `include/storage/tiered_storage.h`:154
- Brief: n/a
- Parameters: none

### themis::storage::WebDAVBlobBackend

#### `WebDAVBlobBackend(const std::string &base_url, const std::string &username, const std::string &password, bool verify_ssl=true)`
- Source: `src/storage/blob_backend_webdav.cpp`:118
- Brief: n/a
- Parameters:
  - `base_url` (const std::string &): n/a
  - `username` (const std::string &): n/a
  - `password` (const std::string &): n/a
  - `verify_ssl` (bool): n/a

#### `std::string computeSHA256(const std::vector< uint8_t > &data)`
- Source: `src/storage/blob_backend_webdav.cpp`:96
- Brief: Compute SHA256.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: SHA256(), data(), size(), std::setw(), std::setfill(), str().

#### `bool exists(const BlobRef &ref) override`
- Source: `src/storage/blob_backend_webdav.cpp`:363
- Brief: Check if blob exists.
- Parameters:
  - `ref` (const BlobRef &): Blob reference
- Return: true if exists
- Details: ref Blob reference true if exists

#### `Result< std::vector< uint8_t > > get(const BlobRef &ref) override`
- Source: `src/storage/blob_backend_webdav.cpp`:248
- Brief: Retrieve a blob.
- Parameters:
  - `ref` (const BlobRef &): Blob reference
- Return: Result<vector<uint8_t>> Blob data or error if not found
- Details: ref Blob reference Result<vector<uint8_t>> Blob data or error if not found

#### `std::string getBlobUrl(const std::string &blob_id) const`
- Source: `src/storage/blob_backend_webdav.cpp`:108
- Brief: n/a
- Parameters:
  - `blob_id` (const std::string &): n/a

#### `bool isAvailable() const override`
- Source: `src/storage/blob_backend_webdav.cpp`:400
- Brief: Check if backend is available.
- Parameters: none
- Return: true if backend can be used
- Details: true if backend can be used

#### `std::string name() const override`
- Source: `src/storage/blob_backend_webdav.cpp`:396
- Brief: Get backend name.
- Parameters: none
- Return: Backend name (e.g., "filesystem", "s3", "webdav")
- Details: Backend name (e.g., "filesystem", "s3", "webdav")

#### `Result< BlobRef > put(const std::string &blob_id, const std::vector< uint8_t > &data) override`
- Source: `src/storage/blob_backend_webdav.cpp`:163
- Brief: Store a blob.
- Parameters:
  - `blob_id` (const std::string &): Unique blob identifier
  - `data` (const std::vector< uint8_t > &): Blob data
- Return: Result<BlobRef> Reference to stored blob or error
- Details: blob_id Unique blob identifier data Blob data Result<BlobRef> Reference to stored blob or error

#### `size_t readCallback(char *ptr, size_t size, size_t nmemb, void *userdata)`
- Source: `src/storage/blob_backend_webdav.cpp`:72
- Brief: Read Callback.
- Parameters:
  - `ptr` (char *): Input/output parameter.
  - `size` (size_t): Input parameter.
  - `nmemb` (size_t): Input parameter.
  - `userdata` (void *): Input/output parameter.
- Return: Return value.
- Details: ptr Input/output parameter. size Input parameter. nmemb Input parameter. userdata Input/output parameter. Return value. Calls: std::min(), std::memcpy().

#### `Result< void > remove(const BlobRef &ref) override`
- Source: `src/storage/blob_backend_webdav.cpp`:317
- Brief: Delete a blob.
- Parameters:
  - `ref` (const BlobRef &): Blob reference
- Return: Result<bool> Success or error
- Details: ref Blob reference Result<bool> Success or error

#### `size_t writeCallback(void *ptr, size_t size, size_t nmemb, void *userdata)`
- Source: `src/storage/blob_backend_webdav.cpp`:46
- Brief: CURL helper for writing data uninitialized_access scanner alert (line 37): ptr and userdata are standard CURL callback parameters — they are passed by the libcurl runtime and are always valid non-null pointers when the callback is invoked — false positive.
- Parameters:
  - `ptr` (void *): Input/output parameter.
  - `size` (size_t): Input parameter.
  - `nmemb` (size_t): Input parameter.
  - `userdata` (void *): Input/output parameter.
- Return: Return value.
- Details: ptr Input/output parameter. size Input parameter. nmemb Input parameter. userdata Input/output parameter. Return value. Calls: insert(), end().

### themis::storage::WriteGuard

#### `WriteGuard()=default`
- Source: `include/storage/concurrent_write_controller.h`:86
- Brief: n/a
- Parameters: none

#### `WriteGuard(ConcurrentWriteController *ctrl) noexcept`
- Source: `include/storage/concurrent_write_controller.h`:103
- Brief: n/a
- Parameters:
  - `ctrl` (ConcurrentWriteController *): n/a

#### `WriteGuard(WriteGuard &&other) noexcept`
- Source: `include/storage/concurrent_write_controller.h`:89
- Brief: n/a
- Parameters:
  - `other` (WriteGuard &&): n/a

#### `WriteGuard(const WriteGuard &)=delete`
- Source: `include/storage/concurrent_write_controller.h`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WriteGuard &): n/a

#### `operator bool() const noexcept`
- Source: `include/storage/concurrent_write_controller.h`:96
- Brief: Returns true when the guard actually holds a slot (not default-constructed).
- Parameters: none

#### `WriteGuard & operator=(WriteGuard &&other) noexcept`
- Source: `include/storage/concurrent_write_controller.h`:90
- Brief: n/a
- Parameters:
  - `other` (WriteGuard &&): n/a

#### `WriteGuard & operator=(const WriteGuard &)=delete`
- Source: `include/storage/concurrent_write_controller.h`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WriteGuard &): n/a

#### `void release() noexcept`
- Source: `include/storage/concurrent_write_controller.h`:99
- Brief: Manually release the slot before the guard is destroyed.
- Parameters: none

#### `~WriteGuard()`
- Source: `include/storage/concurrent_write_controller.h`:87
- Brief: n/a
- Parameters: none

### themis::storage::ZeroCopyBlobTransfer

#### `ZeroCopyBlobTransfer(const ZeroCopyTransferConfig &config=ZeroCopyTransferConfig{})`
- Source: `include/storage/zero_copy_blob_transfer.h`:175
- Brief: n/a
- Parameters:
  - `config` (const ZeroCopyTransferConfig &): n/a

#### `const ZeroCopyTransferConfig & config() const noexcept`
- Source: `include/storage/zero_copy_blob_transfer.h`:251
- Brief: n/a
- Parameters: none

#### `Result< ZeroCopyTransferStats > fallbackTransfer(const std::string &source_path, int dest_fd, int64_t offset, int64_t length)`
- Source: `include/storage/zero_copy_blob_transfer.h`:257
- Brief: Portable fallback used when sendfile() is not available.
- Parameters:
  - `source_path` (const std::string &): Path to the source.
  - `dest_fd` (int): Input parameter.
  - `offset` (int64_t): Input parameter.
  - `length` (int64_t): Input parameter.
- Return: Return value.
- Details: Fallback Transfer. source_path Path to the source. dest_fd Input parameter. offset Input parameter. length Input parameter. Return value.

#### `MmapBlobView openMmap(const std::string &file_path) const`
- Source: `include/storage/zero_copy_blob_transfer.h`:214
- Brief: Open a read-only, memory-mapped view of file_path.
- Parameters:
  - `file_path` (const std::string &): Absolute path to the blob file.
- Return: MmapBlobView
- Details: The caller should check MmapBlobView::valid() before dereferencing the data pointer. file_path Absolute path to the blob file. MmapBlobView

#### `Result< ZeroCopyTransferStats > s3MultipartUpload(const std::string &bucket, const std::string &s3_key, const std::string &source_path, const std::string &blob_id)`
- Source: `include/storage/zero_copy_blob_transfer.h`:234
- Brief: Upload source_path to S3 using multipart streaming.
- Parameters:
  - `bucket` (const std::string &): Input parameter.
  - `s3_key` (const std::string &): Input parameter.
  - `source_path` (const std::string &): Path to the source.
  - `blob_id` (const std::string &): Identifier of the blob.
- Return: Result<ZeroCopyTransferStats>
- Details: S3 Multipart Upload. The file is read in parts of config.s3_multipart_part_size_bytes (minimum 5 MB as required by S3). Each part is uploaded independently; the multipart upload is finalised once all parts succeed. Available only when THEMIS_HAS_AWS_SDK is defined. Returns ERR_UTIL_FILE_OPERATION_FAILED with a descriptive message otherwise. bucket S3 bucket name. s3_key S3 object key (e.g. "prefix/blob_id.blob"). source_path Absolute path to the local blob file. blob_id Logical blob ID (stored in BlobRef.id). Result<ZeroCopyTransferStats> bucket Input parameter. s3_key Input parameter. source_path Path to the source. blob_id Identifier of the blob. Return value.

#### `Result< ZeroCopyTransferStats > sendfileTransfer(const std::string &source_path, int dest_fd, int64_t offset=0, int64_t length=0)`
- Source: `include/storage/zero_copy_blob_transfer.h`:196
- Brief: Transfer bytes from source_path to dest_fd via sendfile().
- Parameters:
  - `source_path` (const std::string &): Path to the source.
  - `dest_fd` (int): Input parameter.
  - `offset` (int64_t): Input parameter.
  - `length` (int64_t): Input parameter.
- Return: Result<ZeroCopyTransferStats> ERR_STORAGE_FILE_NOT_FOUND – source file missing ERR_UTIL_FILE_OPERATION_FAILED – sendfile/write syscall error
- Details: Sendfile Transfer. On Linux the transfer is performed entirely in kernel space. On other platforms a portable read+write fallback is used automatically. source_path Absolute path to the source blob file. dest_fd Open, writable file descriptor to receive the data. offset Byte offset in source_path to start from (0 = BOF). length Number of bytes to transfer (0 = full file). Result<ZeroCopyTransferStats> ERR_STORAGE_FILE_NOT_FOUND – source file missing ERR_UTIL_FILE_OPERATION_FAILED – sendfile/write syscall error source_path Path to the source. dest_fd Input parameter. offset Input parameter. length Input parameter. Return value.

#### `bool shouldUseZeroCopy(int64_t size_bytes) const noexcept`
- Source: `include/storage/zero_copy_blob_transfer.h`:247
- Brief: n/a
- Parameters:
  - `size_bytes` (int64_t): n/a
- Return: true when size_bytes is above the configured threshold and therefore qualifies for zero-copy treatment.
- Details: true when size_bytes is above the configured threshold and therefore qualifies for zero-copy treatment.

#### `~ZeroCopyBlobTransfer()=default`
- Source: `include/storage/zero_copy_blob_transfer.h`:178
- Brief: n/a
- Parameters: none

### themis::storage::ZoneMap

#### `bool canSkipForFloat(double value) const`
- Source: `include/storage/columnar_format.h`:72
- Brief: n/a
- Parameters:
  - `value` (double): n/a

#### `bool canSkipForInt(int64_t value) const`
- Source: `include/storage/columnar_format.h`:71
- Brief: n/a
- Parameters:
  - `value` (int64_t): n/a

#### `bool canSkipForString(const std::string &value) const`
- Source: `include/storage/columnar_format.h`:73
- Brief: n/a
- Parameters:
  - `value` (const std::string &): n/a

### themis::storage::disk_utils

#### `std::string formatBytes(size_t bytes)`
- Source: `src/storage/disk_space_monitor.cpp`:703
- Brief: Format bytes as human-readable string.
- Parameters:
  - `bytes` (size_t): Input parameter.
- Return: Formatted string (e.g., "1.5 GB")
- Details: Format Bytes. bytes Byte count Formatted string (e.g., "1.5 GB") bytes Input parameter. Return value. Calls: std::setprecision(), str().

#### `std::string getDirectory(const std::string &path)`
- Source: `src/storage/disk_space_monitor.cpp`:740
- Brief: Get directory of a file path.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: Get Directory. path Input parameter. Return value. Calls: find_last_of(), substr().

#### `bool getDiskSpace(const std::string &path, size_t &total_bytes, size_t &free_bytes, size_t &available_bytes)`
- Source: `src/storage/disk_space_monitor.cpp`:625
- Brief: Get disk space info for a path.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `total_bytes` (size_t &): Input/output parameter.
  - `free_bytes` (size_t &): Input/output parameter.
  - `available_bytes` (size_t &): Input/output parameter.
- Return: true if query successful
- Details: Get Disk Space. Platform-independent disk space query path Path to query (file or directory) total_bytes Total disk space free_bytes Free disk space available_bytes Available space (may differ from free on some systems) true if query successful path Input parameter. total_bytes Input/output parameter. free_bytes Input/output parameter. available_bytes Input/output parameter. True when the operation succeeds. Calls: fs::absolute(), fs::path(), empty(), fs::current_path(), fs::exists(), parent_path(), wstring(), GetDiskFreeSpaceExW().

#### `bool pathExists(const std::string &path)`
- Source: `src/storage/disk_space_monitor.cpp`:724
- Brief: Check if path exists.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: Path Exists. path Input parameter. True when the operation succeeds. Calls: GetFileAttributesA(), c_str(), stat().

### themis::storage::test

#### `TEST(Wave1CriticalFixes, BracesImbalanceFilesCompileClean)`
- Source: `tests/storage/test_wave1_critical_fixes.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave1CriticalFixes): n/a
  - `<unnamed>` (BracesImbalanceFilesCompileClean): n/a

#### `TEST(Wave1CriticalFixes, CompactionManagerDestructorIsNoexcept)`
- Source: `tests/storage/test_wave1_critical_fixes.cpp`:47
- Brief: CompactionManager destructor must be declared noexcept.
- Parameters:
  - `<unnamed>` (Wave1CriticalFixes): n/a
  - `<unnamed>` (CompactionManagerDestructorIsNoexcept): n/a

#### `TEST(Wave1CriticalFixes, CudaCheckMacroDefined)`
- Source: `tests/storage/test_wave1_critical_fixes.cpp`:115
- Brief: THEMIS_CUDA_CHECK must be defined (either as a no-op or as a real check).
- Parameters:
  - `<unnamed>` (Wave1CriticalFixes): n/a
  - `<unnamed>` (CudaCheckMacroDefined): n/a

#### `TEST(Wave1CriticalFixes, CudaCheckMacroIsNoopWithoutCuda)`
- Source: `tests/storage/test_wave1_critical_fixes.cpp`:126
- Brief: When CUDA is disabled the macro must expand to a no-op (void expression).
- Parameters:
  - `<unnamed>` (Wave1CriticalFixes): n/a
  - `<unnamed>` (CudaCheckMacroIsNoopWithoutCuda): n/a

#### `TEST(Wave1CriticalFixes, IndexMaintenanceManagerDestructorIsNoexcept)`
- Source: `tests/storage/test_wave1_critical_fixes.cpp`:55
- Brief: IndexMaintenanceManager destructor must be declared noexcept.
- Parameters:
  - `<unnamed>` (Wave1CriticalFixes): n/a
  - `<unnamed>` (IndexMaintenanceManagerDestructorIsNoexcept): n/a

#### `TEST(Wave1CriticalFixes, PathTraversalGuardDocumented)`
- Source: `tests/storage/test_wave1_critical_fixes.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave1CriticalFixes): n/a
  - `<unnamed>` (PathTraversalGuardDocumented): n/a

#### `TEST(Wave1CriticalFixes, StreamingIngestManagerCreateRejectsNullDb)`
- Source: `tests/storage/test_wave1_critical_fixes.cpp`:66
- Brief: create() must throw std::invalid_argument on null db (not return nullptr).
- Parameters:
  - `<unnamed>` (Wave1CriticalFixes): n/a
  - `<unnamed>` (StreamingIngestManagerCreateRejectsNullDb): n/a

#### `TEST(Wave1CriticalFixes, StreamingIngestManagerCreateReturnType)`
- Source: `tests/storage/test_wave1_critical_fixes.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave1CriticalFixes): n/a
  - `<unnamed>` (StreamingIngestManagerCreateReturnType): n/a
- Details: create() return type must be [[nodiscard]] — checked at compile time by ensuring the return type is std::unique_ptr (ownership-semantics correct).

#### `TEST(Wave1CriticalFixes, WebdavTransitEncryptionDocumented)`
- Source: `tests/storage/test_wave1_critical_fixes.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (Wave1CriticalFixes): n/a
  - `<unnamed>` (WebdavTransitEncryptionDocumented): n/a

### themis::test::wave_d

#### `TEST(BlobTieringEdgeCases, PromotionAndDemotionConsistency)`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (BlobTieringEdgeCases): n/a
  - `<unnamed>` (PromotionAndDemotionConsistency): n/a

#### `TEST(ConcurrentCompactionStress, NoDataLossOrCorruption)`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConcurrentCompactionStress): n/a
  - `<unnamed>` (NoDataLossOrCorruption): n/a

#### `TEST(HighCardinalityKeyWrite, AllKeysPresent)`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (HighCardinalityKeyWrite): n/a
  - `<unnamed>` (AllKeysPresent): n/a

### themis::test::wave_d::CompactableShard

#### `bool compact()`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:88
- Brief: n/a
- Parameters: none

#### `long compactions() const`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:95
- Brief: n/a
- Parameters: none

#### `bool write(const std::string &key, const std::string &value)`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:80
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

#### `long writes() const`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:94
- Brief: n/a
- Parameters: none

### themis::test::wave_d::KVStore

#### `bool get(const std::string &key, std::string *out) const`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:51
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `out` (std::string *): n/a

#### `bool put(const std::string &key, const std::string &value)`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:44
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

#### `long reads() const`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:66
- Brief: n/a
- Parameters: none

#### `std::size_t size() const`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:60
- Brief: n/a
- Parameters: none

#### `long writes() const`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:65
- Brief: n/a
- Parameters: none

### themis::test::wave_d::TieredBlobStore

#### `bool access(const std::string &key)`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:124
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `long promoted() const`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:138
- Brief: n/a
- Parameters: none

#### `bool put(const std::string &key, const std::string &data)`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:117
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `data` (const std::string &): n/a

#### `long writes() const`
- Source: `tests/storage/test_storage_highcardinality_stress.cpp`:146
- Brief: n/a
- Parameters: none

### themisdb::storage

#### `std::string ecChunkPath(const std::string &blob_id, uint32_t chunk_index)`
- Source: `src/storage/blob_redundancy_manager.cpp`:1250
- Brief: Ec Chunk Path.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
  - `chunk_index` (uint32_t): Input parameter.
- Return: Return value.
- Details: blob_id Identifier of the blob. chunk_index Input parameter. Return value. Calls: std::to_string().

#### `std::string ecShardId(const BlobMetadata &meta, uint32_t chunk_index)`
- Source: `src/storage/blob_redundancy_manager.cpp`:1236
- Brief: ------------------------------------------------------------------------ Erasure-coding shard helpers (file-local) ------------------------------------------------------------------------
- Parameters:
  - `meta` (const BlobMetadata &): Input parameter.
  - `chunk_index` (uint32_t): Input parameter.
- Return: Return value.
- Details: meta Input parameter. chunk_index Input parameter. Return value. Calls: size(), std::to_string().

### themisdb::storage::BlobMetadata

#### `bool canRecover() const`
- Source: `include/storage/blob_redundancy_manager.h`:291
- Brief: n/a
- Parameters: none

#### `std::optional< BlobMetadata > fromJson(const std::string &json)`
- Source: `include/storage/blob_redundancy_manager.h`:296
- Brief: From Json.
- Parameters:
  - `json` (const std::string &): Input parameter.
- Return: Return value.
- Details: json Input parameter. Return value. Calls: nlohmann::json::parse(), value(), epoch_to_tp(), contains(), is_object(), blob_config_from_json(), is_array(), reserve().

#### `std::vector< std::string > getMissingShards() const`
- Source: `include/storage/blob_redundancy_manager.h`:292
- Brief: n/a
- Parameters: none

#### `uint32_t healthyLocationCount() const`
- Source: `include/storage/blob_redundancy_manager.h`:289
- Brief: n/a
- Parameters: none

#### `bool isHealthy() const`
- Source: `include/storage/blob_redundancy_manager.h`:288
- Brief: n/a
- Parameters: none

#### `uint32_t requiredLocationCount() const`
- Source: `include/storage/blob_redundancy_manager.h`:290
- Brief: n/a
- Parameters: none

#### `std::string toJson() const`
- Source: `include/storage/blob_redundancy_manager.h`:295
- Brief: n/a
- Parameters: none

### themisdb::storage::BlobRedundancyManager

#### `BlobRedundancyManager(const Config &config)`
- Source: `include/storage/blob_redundancy_manager.h`:385
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `std::string calculateChecksum(const std::vector< uint8_t > &data)`
- Source: `include/storage/blob_redundancy_manager.h`:501
- Brief: Calculate Checksum.
- Parameters:
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: data Input parameter. Return value. Calls: str().

#### `BlobType classifyBlobType(const std::string &path, uint64_t size)`
- Source: `include/storage/blob_redundancy_manager.h`:502
- Brief: Classify Blob Type.
- Parameters:
  - `path` (const std::string &): Input parameter.
  - `size` (uint64_t): Input parameter.
- Return: Return value.
- Details: path Input parameter. size Input parameter. Return value. Calls: find().

#### `void configReloadLoop()`
- Source: `include/storage/blob_redundancy_manager.h`:498
- Brief: Config Reload Loop.
- Parameters: none
- Details: Calls: spdlog::info(), load(), lock(), wait_for(), std::chrono::seconds(), spdlog::error(), what().

#### `Result< std::shared_ptr< rocksdb::EventListener > > createRocksDBListener()`
- Source: `include/storage/blob_redundancy_manager.h`:455
- Brief: Create Rocks DBListener.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: themis::Ok().

#### `Result< void > deleteBlob(const std::string &blob_id, DeleteHandler handler)`
- Source: `include/storage/blob_redundancy_manager.h`:430
- Brief: Delete Blob.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
  - `handler` (DeleteHandler): Input parameter.
- Return: Return value.
- Details: blob_id Identifier of the blob. handler Input parameter. Return value. Calls: lock(), find(), end(), reserve(), size(), handler(), push_back(), erase().

#### `bool deleteFromShard(const std::string &shard_id, const std::string &path, DeleteHandler handler)`
- Source: `include/storage/blob_redundancy_manager.h`:506
- Brief: Delete From Shard.
- Parameters:
  - `shard_id` (const std::string &): Identifier of the shard.
  - `path` (const std::string &): Input parameter.
  - `handler` (DeleteHandler): Input parameter.
- Return: True when the operation succeeds.
- Details: shard_id Identifier of the shard. path Input parameter. handler Input parameter. True when the operation succeeds. Calls: handler().

#### `Result< void > ensureRedundancy(const std::string &blob_id)`
- Source: `include/storage/blob_redundancy_manager.h`:414
- Brief: Ensure Redundancy.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
- Return: Return value.
- Details: blob_id Identifier of the blob. Return value. Calls: lock(), find(), end(), isHealthy(), themis::OkVoid(), repair_lock(), push(), notify_one().

#### `std::string exportPrometheusMetrics() const`
- Source: `include/storage/blob_redundancy_manager.h`:452
- Brief: n/a
- Parameters: none

#### `std::string generateBlobId()`
- Source: `include/storage/blob_redundancy_manager.h`:500
- Brief: Generate Blob Id.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: gen(), rd(), dis(), std::setfill(), std::setw(), str().

#### `BlobMetadata getBlobMetadata(const std::string &blob_id) const`
- Source: `include/storage/blob_redundancy_manager.h`:441
- Brief: n/a
- Parameters:
  - `blob_id` (const std::string &): n/a

#### `std::vector< std::string > getBlobsForTierDown() const`
- Source: `include/storage/blob_redundancy_manager.h`:438
- Brief: n/a
- Parameters: none

#### `BlobRedundancyConfig getConfigForBlob(BlobType type, const std::string &collection="")`
- Source: `include/storage/blob_redundancy_manager.h`:396
- Brief: Get Config For Blob.
- Parameters:
  - `type` (BlobType): Input parameter.
  - `collection` (const std::string &): Input parameter.
- Return: Return value.
- Details: type Input parameter. collection Input parameter. Return value. Calls: lock(), empty(), find(), end().

#### `std::vector< std::string > getCriticalBlobs() const`
- Source: `include/storage/blob_redundancy_manager.h`:443
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > getDegradedBlobs() const`
- Source: `include/storage/blob_redundancy_manager.h`:442
- Brief: n/a
- Parameters: none

#### `BlobRedundancyStats getStats() const`
- Source: `include/storage/blob_redundancy_manager.h`:444
- Brief: n/a
- Parameters: none

#### `bool isRunning() const`
- Source: `include/storage/blob_redundancy_manager.h`:391
- Brief: n/a
- Parameters: none

#### `bool loadConfig(const std::string &path)`
- Source: `include/storage/blob_redundancy_manager.h`:394
- Brief: Load Config.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: path Input parameter. True when the operation succeeds. Calls: spdlog::info(), IsMap(), YAML::LoadFile(), spdlog::warn(), lock(), parseBlobConfig(), find(), end().

#### `void loadFromMetadataStore()`
- Source: `include/storage/blob_redundancy_manager.h`:514
- Brief: Load From Metadata Store.
- Parameters: none
- Details: Implements loadFromMetadataStore without additional internal calls.

#### `void maintenanceLoop()`
- Source: `include/storage/blob_redundancy_manager.h`:496
- Brief: ═══════════════════════════════════════════════════════════ Private Methods ═══════════════════════════════════════════════════════════
- Parameters: none
- Details: Calls: spdlog::info(), load(), runMaintenanceCycle(), spdlog::error(), what(), lock(), wait_for(), std::chrono::seconds().

#### `void notifySSTFileDeleted(const std::string &file_path)`
- Source: `include/storage/blob_redundancy_manager.h`:460
- Brief: Notify SSTFile Deleted.
- Parameters:
  - `file_path` (const std::string &): Path to the file.
- Details: file_path Path to the file. Calls: lock(), reserve(), size(), healthyLocationCount(), requiredLocationCount(), canRecover(), push_back(), spdlog::error().

#### `Result< std::vector< uint8_t > > readBlob(const std::string &blob_id, ReadHandler handler)`
- Source: `include/storage/blob_redundancy_manager.h`:425
- Brief: Read Blob.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
  - `handler` (ReadHandler): Input parameter.
- Return: Return value.
- Details: blob_id Identifier of the blob. handler Input parameter. Return value. Calls: lock(), find(), end(), unlock(), totalShards(), ecShardId(), ecChunkPath(), handler().

#### `std::string registerBlob(BlobType type, const std::string &local_path, uint64_t size_bytes, const std::string &collection="", const std::string &document_id="")`
- Source: `include/storage/blob_redundancy_manager.h`:402
- Brief: Register Blob.
- Parameters:
  - `type` (BlobType): Input parameter.
  - `local_path` (const std::string &): Path to the local.
  - `size_bytes` (uint64_t): Input parameter.
  - `collection` (const std::string &): Input parameter.
  - `document_id` (const std::string &): Identifier of the document.
- Return: Return value.
- Details: type Input parameter. local_path Path to the local. size_bytes Input parameter. collection Input parameter. document_id Identifier of the document. Return value. Calls: generateBlobId(), spdlog::info(), getConfigForBlob(), std::chrono::system_clock::now(), reserve(), totalShards(), push_back(), std::to_string().

#### `bool reloadConfig()`
- Source: `include/storage/blob_redundancy_manager.h`:395
- Brief: Reload Config.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Calls: loadConfig().

#### `void removeFromMetadataStore(const std::string &blob_id)`
- Source: `include/storage/blob_redundancy_manager.h`:513
- Brief: Remove From Metadata Store.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
- Details: blob_id Identifier of the blob. Calls: spdlog::debug().

#### `Result< void > repairBlob(const std::string &blob_id)`
- Source: `include/storage/blob_redundancy_manager.h`:415
- Brief: Repair Blob.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
- Return: Return value.
- Details: blob_id Identifier of the blob. Return value. Calls: lock(), find(), end(), themis::OkVoid().

#### `void repairLoop()`
- Source: `include/storage/blob_redundancy_manager.h`:497
- Brief: Repair Loop.
- Parameters: none
- Details: Calls: spdlog::info(), load(), lock(), wait_for(), std::chrono::seconds(), empty(), unlock(), runRepairQueue().

#### `bool replicateToShard(const std::string &shard_id, const BlobMetadata &blob, const std::vector< uint8_t > &data, WriteHandler handler)`
- Source: `include/storage/blob_redundancy_manager.h`:504
- Brief: Replicate To Shard.
- Parameters:
  - `shard_id` (const std::string &): Identifier of the shard.
  - `blob` (const BlobMetadata &): Input parameter.
  - `data` (const std::vector< uint8_t > &): Input parameter.
  - `handler` (WriteHandler): Input parameter.
- Return: True when the operation succeeds.
- Details: shard_id Identifier of the shard. blob Input parameter. data Input parameter. handler Input parameter. True when the operation succeeds. Calls: handler().

#### `void runMaintenanceCycle()`
- Source: `include/storage/blob_redundancy_manager.h`:447
- Brief: Run Maintenance Cycle.
- Parameters: none
- Details: Calls: spdlog::debug(), getDegradedBlobs(), spdlog::info(), size(), lock(), push(), notify_one(), getBlobsForTierDown().

#### `void runRepairQueue()`
- Source: `include/storage/blob_redundancy_manager.h`:449
- Brief: Run Repair Queue.
- Parameters: none
- Details: Calls: spdlog::debug(), lock(), empty(), front(), pop(), repairBlob(), spdlog::warn(), error().

#### `void runScrub(bool full=false)`
- Source: `include/storage/blob_redundancy_manager.h`:448
- Brief: Run Scrub.
- Parameters:
  - `full` (bool): Input parameter.
- Details: full Input parameter. Calls: spdlog::info(), lock(), reserve(), size(), isHealthy(), getMissingShards(), spdlog::warn(), healthyLocationCount().

#### `std::string selectReadShard(const BlobMetadata &blob)`
- Source: `include/storage/blob_redundancy_manager.h`:510
- Brief: Select Read Shard.
- Parameters:
  - `blob` (const BlobMetadata &): Input parameter.
- Return: Return value.
- Details: blob Input parameter. Return value. Calls: empty().

#### `std::vector< std::string > selectTargetShards(const BlobMetadata &blob)`
- Source: `include/storage/blob_redundancy_manager.h`:509
- Brief: Select Target Shards.
- Parameters:
  - `blob` (const BlobMetadata &): Input parameter.
- Return: Return value.
- Details: blob Input parameter. Return value. Calls: reserve(), size(), push_back().

#### `void setCollectionOverride(const std::string &collection, const BlobRedundancyConfig &config)`
- Source: `include/storage/blob_redundancy_manager.h`:397
- Brief: Set Collection Override.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `config` (const BlobRedundancyConfig &): Input parameter.
- Details: collection Input parameter. config Input parameter. Calls: lock().

#### `void setDocumentOverride(const std::string &collection, const std::string &doc_id, const BlobRedundancyConfig &config)`
- Source: `include/storage/blob_redundancy_manager.h`:398
- Brief: Set Document Override.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `doc_id` (const std::string &): Identifier of the doc.
  - `config` (const BlobRedundancyConfig &): Input parameter.
- Details: collection Input parameter. doc_id Identifier of the doc. config Input parameter. Calls: lock().

#### `bool start()`
- Source: `include/storage/blob_redundancy_manager.h`:389
- Brief: Start.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Calls: exchange(), spdlog::warn(), spdlog::info(), std::thread(), maintenanceLoop(), repairLoop(), configReloadLoop().

#### `void stop()`
- Source: `include/storage/blob_redundancy_manager.h`:390
- Brief: Stop.
- Parameters: none
- Details: Calls: exchange(), spdlog::info(), notify_all(), joinable(), themis::utils::joinThreadWithin(), spdlog::warn().

#### `Result< void > tierDown(const std::string &blob_id, StorageTier target)`
- Source: `include/storage/blob_redundancy_manager.h`:436
- Brief: Tier Down.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
  - `target` (StorageTier): Input parameter.
- Return: Return value.
- Details: blob_id Identifier of the blob. target Input parameter. Return value. Calls: lock(), find(), end(), std::chrono::system_clock::now(), updateMetadataStore(), themis::OkVoid().

#### `Result< void > tierUp(const std::string &blob_id, StorageTier target)`
- Source: `include/storage/blob_redundancy_manager.h`:437
- Brief: Tier Up.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
  - `target` (StorageTier): Input parameter.
- Return: Return value.
- Details: blob_id Identifier of the blob. target Input parameter. Return value. Calls: lock(), find(), end(), std::chrono::system_clock::now(), updateMetadataStore(), themis::OkVoid().

#### `void unregisterBlob(const std::string &blob_id)`
- Source: `include/storage/blob_redundancy_manager.h`:411
- Brief: Unregister Blob.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
- Details: blob_id Identifier of the blob. Calls: lock(), erase(), spdlog::debug().

#### `void updateMetadataStore(const BlobMetadata &blob)`
- Source: `include/storage/blob_redundancy_manager.h`:512
- Brief: Update Metadata Store.
- Parameters:
  - `blob` (const BlobMetadata &): Input parameter.
- Details: blob Input parameter. Calls: spdlog::debug(), size().

#### `bool verifyBlob(const std::string &blob_id)`
- Source: `include/storage/blob_redundancy_manager.h`:416
- Brief: Verify Blob.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
- Return: True when the operation succeeds.
- Details: blob_id Identifier of the blob. True when the operation succeeds. Calls: lock(), find(), end(), empty(), spdlog::warn(), healthyLocationCount(), requiredLocationCount(), getMissingShards().

#### `Result< void > writeBlob(const std::string &blob_id, const std::vector< uint8_t > &data, WriteHandler handler)`
- Source: `include/storage/blob_redundancy_manager.h`:419
- Brief: Write Blob.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
  - `data` (const std::vector< uint8_t > &): Input parameter.
  - `handler` (WriteHandler): Input parameter.
- Return: Return value.
- Details: blob_id Identifier of the blob. data Input parameter. handler Input parameter. Return value. Calls: lock(), find(), end(), unlock(), ec_backend(), encode(), std::string(), what().

#### `~BlobRedundancyManager()`
- Source: `include/storage/blob_redundancy_manager.h`:386
- Brief: n/a
- Parameters: none

### themisdb::storage::CollectionRedundancyConfig

#### `std::optional< CollectionRedundancyConfig > loadFromYaml(const std::string &path)`
- Source: `include/storage/blob_redundancy_manager.h`:569
- Brief: ═══════════════════════════════════════════════════════════ CollectionRedundancyConfig Implementation ═══════════════════════════════════════════════════════════
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: path Input parameter. Return value. Calls: IsMap(), YAML::LoadFile(), parse_blob_cfg(), empty(), find(), end(), spdlog::warn(), spdlog::error().

#### `bool saveToYaml(const std::string &path) const`
- Source: `include/storage/blob_redundancy_manager.h`:572
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### themisdb::storage::ErasureCodingBackend

#### `ErasureCodingBackend(ErasureCodingBackend &&)=delete`
- Source: `include/storage/erasure_coding_backend.h`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErasureCodingBackend &&): n/a

#### `ErasureCodingBackend(const ErasureCodingBackend &)=delete`
- Source: `include/storage/erasure_coding_backend.h`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ErasureCodingBackend &): n/a

#### `ErasureCodingBackend(const ErasureCodingConfig &config)`
- Source: `include/storage/erasure_coding_backend.h`:88
- Brief: n/a
- Parameters:
  - `config` (const ErasureCodingConfig &): n/a
- Throws:
  - std::invalid_argument: if data_shards < 2 or parity_shards < 1.
- Details: Construct with an explicit erasure coding configuration. std::invalid_argument if data_shards < 2 or parity_shards < 1.

#### `uint32_t availableShardCount(const std::string &blob_id) const`
- Source: `include/storage/erasure_coding_backend.h`:180
- Brief: n/a
- Parameters:
  - `blob_id` (const std::string &): n/a
- Details: Return the number of healthy (available) shards for blob_id in the internal store, or 0 if the blob is not known.

#### `bool canRecover(uint32_t failed_shards) const noexcept`
- Source: `include/storage/erasure_coding_backend.h`:209
- Brief: n/a
- Parameters:
  - `failed_shards` (uint32_t): n/a
- Details: Return true if the backend can still reconstruct a blob given that failed_shards shards are unavailable.

#### `const ErasureCodingConfig & config() const noexcept`
- Source: `include/storage/erasure_coding_backend.h`:186
- Brief: n/a
- Parameters: none

#### `uint32_t dataShards() const noexcept`
- Source: `include/storage/erasure_coding_backend.h`:187
- Brief: n/a
- Parameters: none

#### `std::vector< uint8_t > decode(const std::string &blob_id, const std::map< uint32_t, EncodedShard > &shards, uint64_t original_size=0) const`
- Source: `include/storage/erasure_coding_backend.h`:135
- Brief: n/a
- Parameters:
  - `blob_id` (const std::string &): Identifier used for logging / error messages.
  - `shards` (const std::map< uint32_t, EncodedShard > &): Map of shard_index → EncodedShard for every available (non-erased) shard.
  - `original_size` (uint64_t): Expected original blob size in bytes. If zero, the value stored in the first available shard is used.
- Return: Reconstructed original data.
- Throws:
  - std::runtime_error: if fewer than data_shards shards are present.
- Details: Decode the original blob from a subset of shards. Reconstruction succeeds as long as at least data_shards shards are available (i.e. up to parity_shards simultaneous failures are tolerated). blob_id Identifier used for logging / error messages. shards Map of shard_index → EncodedShard for every available (non-erased) shard. original_size Expected original blob size in bytes. If zero, the value stored in the first available shard is used. Reconstructed original data. std::runtime_error if fewer than data_shards shards are present.

#### `bool dropShard(const std::string &blob_id, uint32_t shard_index)`
- Source: `include/storage/erasure_coding_backend.h`:174
- Brief: Drop Shard.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
  - `shard_index` (uint32_t): Input parameter.
- Return: true if the shard was present and removed.
- Details: Simulate a shard failure by removing one shard from the internal store. Intended for testing fault-tolerance scenarios. blob_id Target blob. shard_index Index of the shard to drop (0 … total_shards-1). true if the shard was present and removed. blob_id Identifier of the blob. shard_index Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), erase().

#### `std::vector< EncodedShard > encode(const std::string &blob_id, const std::vector< uint8_t > &data) const`
- Source: `include/storage/erasure_coding_backend.h`:114
- Brief: n/a
- Parameters:
  - `blob_id` (const std::string &): Identifier used only for logging.
  - `data` (const std::vector< uint8_t > &): Raw blob bytes to encode.
- Return: Vector of (data_shards + parity_shards) shards.
- Details: Encode data into (data_shards + parity_shards) EncodedShard objects. The original data is zero-padded to a multiple of data_shards before splitting; each shard therefore has the same size. The original byte count is stored in EncodedShard::original_size so that trailing padding can be stripped on decode. blob_id Identifier used only for logging. data Raw blob bytes to encode. Vector of (data_shards + parity_shards) shards.

#### `uint32_t faultTolerance() const noexcept`
- Source: `include/storage/erasure_coding_backend.h`:203
- Brief: n/a
- Parameters: none
- Details: Maximum number of simultaneous shard failures the configuration can recover from (equals parity_shards).

#### `std::optional< std::vector< uint8_t > > get(const std::string &blob_id) const`
- Source: `include/storage/erasure_coding_backend.h`:158
- Brief: n/a
- Parameters:
  - `blob_id` (const std::string &): n/a
- Details: Retrieve a blob from the internal shard store. Returns std::nullopt if the blob is unknown or if too many shards are missing for reconstruction (more than parity_shards failures).

#### `ErasureCodingBackend & operator=(ErasureCodingBackend &&)=delete`
- Source: `include/storage/erasure_coding_backend.h`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (ErasureCodingBackend &&): n/a

#### `ErasureCodingBackend & operator=(const ErasureCodingBackend &)=delete`
- Source: `include/storage/erasure_coding_backend.h`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ErasureCodingBackend &): n/a

#### `uint32_t parityShards() const noexcept`
- Source: `include/storage/erasure_coding_backend.h`:188
- Brief: n/a
- Parameters: none

#### `void put(const std::string &blob_id, const std::vector< uint8_t > &data)`
- Source: `include/storage/erasure_coding_backend.h`:150
- Brief: Put.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Details: Encode data and store all shards in the internal shard store. Any previously stored shards for blob_id are replaced. blob_id Identifier of the blob. data Input parameter. Calls: encode(), lock(), size(), clear(), std::move().

#### `void remove(const std::string &blob_id)`
- Source: `include/storage/erasure_coding_backend.h`:163
- Brief: Remove.
- Parameters:
  - `blob_id` (const std::string &): Identifier of the blob.
- Details: Remove all shards for blob_id from the internal shard store. blob_id Identifier of the blob. Calls: lock(), erase().

#### `double storageOverhead() const noexcept`
- Source: `include/storage/erasure_coding_backend.h`:195
- Brief: n/a
- Parameters: none
- Details: Storage overhead factor relative to the raw data size. e.g. RS(4,2) → 1.5×, RS(10,4) → 1.4×, RS(6,3) → 1.5×

#### `uint32_t totalShards() const noexcept`
- Source: `include/storage/erasure_coding_backend.h`:189
- Brief: n/a
- Parameters: none

#### `~ErasureCodingBackend()`
- Source: `include/storage/erasure_coding_backend.h`:90
- Brief: n/a
- Parameters: none

### themisdb::storage::ErasureCodingConfig

#### `double storageEfficiency() const`
- Source: `include/storage/blob_redundancy_manager.h`:159
- Brief: n/a
- Parameters: none

#### `uint32_t totalShards() const`
- Source: `include/storage/blob_redundancy_manager.h`:158
- Brief: n/a
- Parameters: none

### themisdb::storage::RetryStatistics

#### `RetryStatistics()=default`
- Source: `include/storage/transaction_retry_manager.h`:110
- Brief: n/a
- Parameters: none

#### `RetryStatistics(const RetryStatistics &o)`
- Source: `include/storage/transaction_retry_manager.h`:111
- Brief: n/a
- Parameters:
  - `o` (const RetryStatistics &): n/a

#### `double getAverageLatencyMs() const`
- Source: `include/storage/transaction_retry_manager.h`:146
- Brief: n/a
- Parameters: none

#### `double getRetryRate() const`
- Source: `include/storage/transaction_retry_manager.h`:154
- Brief: n/a
- Parameters: none

#### `double getSuccessRate() const`
- Source: `include/storage/transaction_retry_manager.h`:138
- Brief: n/a
- Parameters: none

#### `RetryStatistics & operator=(const RetryStatistics &o)`
- Source: `include/storage/transaction_retry_manager.h`:120
- Brief: n/a
- Parameters:
  - `o` (const RetryStatistics &): n/a

### themisdb::storage::RocksDBBlobListener

#### `void OnCompactionCompleted(rocksdb::DB *db, const rocksdb::CompactionJobInfo &info) override`
- Source: `include/storage/blob_redundancy_manager.h`:534
- Brief: On Compaction Completed.
- Parameters:
  - `db` (rocksdb::DB *): Input/output parameter.
  - `info` (const rocksdb::CompactionJobInfo &): Input parameter.
- Details: db Input/output parameter. info Input parameter. Calls: spdlog::debug(), size(), levelToBlobType(), registerBlob().

#### `void OnFlushCompleted(rocksdb::DB *db, const rocksdb::FlushJobInfo &info) override`
- Source: `include/storage/blob_redundancy_manager.h`:528
- Brief: On Flush Completed.
- Parameters:
  - `db` (rocksdb::DB *): Input/output parameter.
  - `info` (const rocksdb::FlushJobInfo &): Input parameter.
- Details: db Input/output parameter. info Input parameter. Calls: spdlog::debug(), registerBlob().

#### `void OnTableFileDeleted(const rocksdb::TableFileDeletionInfo &info) override`
- Source: `include/storage/blob_redundancy_manager.h`:540
- Brief: On Table File Deleted.
- Parameters:
  - `info` (const rocksdb::TableFileDeletionInfo &): Input parameter.
- Details: info Input parameter. Calls: spdlog::debug(), notifySSTFileDeleted().

#### `RocksDBBlobListener(BlobRedundancyManager &manager, const std::string &collection="")`
- Source: `include/storage/blob_redundancy_manager.h`:524
- Brief: n/a
- Parameters:
  - `manager` (BlobRedundancyManager &): n/a
  - `collection` (const std::string &): n/a

#### `BlobType levelToBlobType(int level)`
- Source: `include/storage/blob_redundancy_manager.h`:548
- Brief: Level To Blob Type.
- Parameters:
  - `level` (int): Input parameter.
- Return: Return value.
- Details: level Input parameter. Return value. Implements levelToBlobType without additional internal calls.

### themisdb::storage::TransactionRetryManager

#### `TransactionRetryManager(const TransactionRetryConfig &config)`
- Source: `include/storage/transaction_retry_manager.h`:191
- Brief: Construct retry manager with configuration.
- Parameters:
  - `config` (const TransactionRetryConfig &): Retry configuration
- Details: config Retry configuration

#### `uint32_t calculateDelay(size_t attempt, const RetryPolicy *policy)`
- Source: `include/storage/transaction_retry_manager.h`:349
- Brief: Calculate delay for next retry attempt.
- Parameters:
  - `attempt` (size_t): Input parameter.
  - `policy` (const RetryPolicy *): Input parameter.
- Return: Return value.
- Details: Calculate Delay. attempt Input parameter. policy Input parameter. Return value. Calls: std::pow(), std::isfinite(), std::min(), lock(), jitter_dist_().

#### `ErrorType classifyError(const std::string &error_message)`
- Source: `include/storage/transaction_retry_manager.h`:338
- Brief: Classify an error by its message.
- Parameters:
  - `error_message` (const std::string &): Input parameter.
- Return: Return value.
- Details: Classify Error. error_message Input parameter. Return value. Calls: std::transform(), begin(), end(), std::tolower(), find().

#### `decltype(operation()) executeWithRetry(Func &&operation, const std::string &operation_name, const RetryPolicy *policy=nullptr)`
- Source: `include/storage/transaction_retry_manager.h`:208
- Brief: Execute operation with automatic retry.
- Parameters:
  - `operation` (Func &&): Function to execute
  - `operation_name` (const std::string &): Name for logging/metrics
  - `policy` (const RetryPolicy *): Optional custom retry policy
- Return: Result of the operation
- Throws:
  - std::runtime_error: if max retries exceeded or circuit is open
- Details: Func Function type that returns a result operation Function to execute operation_name Name for logging/metrics policy Optional custom retry policy Result of the operation std::runtime_error if max retries exceeded or circuit is open

#### `CircuitState getCircuitState() const`
- Source: `include/storage/transaction_retry_manager.h`:323
- Brief: Get current circuit breaker state.
- Parameters: none

#### `const TransactionRetryConfig & getConfig() const`
- Source: `include/storage/transaction_retry_manager.h`:313
- Brief: Get current configuration.
- Parameters: none

#### `RetryStatistics getStatistics() const`
- Source: `include/storage/transaction_retry_manager.h`:318
- Brief: Get current statistics.
- Parameters: none

#### `void invokeAlertCallback(CircuitState state, const std::string &message) const`
- Source: `include/storage/transaction_retry_manager.h`:374
- Brief: Invoke alert callback, if configured.
- Parameters:
  - `state` (CircuitState): n/a
  - `message` (const std::string &): n/a

#### `bool isCircuitOpen() const`
- Source: `include/storage/transaction_retry_manager.h`:364
- Brief: Check if circuit breaker is open.
- Parameters: none

#### `bool isRetryable(ErrorType error_type)`
- Source: `include/storage/transaction_retry_manager.h`:343
- Brief: Check if an error type is retryable.
- Parameters:
  - `error_type` (ErrorType): Input parameter.
- Return: True when the operation succeeds.
- Details: Is Retryable. error_type Input parameter. True when the operation succeeds. Implements isRetryable without additional internal calls.

#### `void recordFailure()`
- Source: `include/storage/transaction_retry_manager.h`:359
- Brief: Record failed operation (for circuit breaker).
- Parameters: none
- Details: Record Failure. Calls: lock(), std::chrono::steady_clock::now(), transitionCircuitState(), invokeAlertCallback().

#### `void recordSuccess()`
- Source: `include/storage/transaction_retry_manager.h`:354
- Brief: Record successful operation (for circuit breaker).
- Parameters: none
- Details: Record Success. Calls: lock(), transitionCircuitState(), invokeAlertCallback().

#### `void resetStatistics()`
- Source: `include/storage/transaction_retry_manager.h`:328
- Brief: Reset statistics.
- Parameters: none
- Details: Reset Statistics. Calls: lock(), RetryStatistics().

#### `void setAlertCallback(AlertCallback callback)`
- Source: `include/storage/transaction_retry_manager.h`:333
- Brief: Set alert callback for circuit breaker state changes.
- Parameters:
  - `callback` (AlertCallback): Input parameter.
- Details: Set Alert Callback. callback Input parameter. Calls: lock(), std::move().

#### `bool transitionCircuitState(CircuitState new_state, std::string *alert_message) const`
- Source: `include/storage/transaction_retry_manager.h`:369
- Brief: Transition circuit breaker state.
- Parameters:
  - `new_state` (CircuitState): n/a
  - `alert_message` (std::string *): n/a

#### `~TransactionRetryManager()`
- Source: `include/storage/transaction_retry_manager.h`:196
- Brief: Destructor.
- Parameters: none

