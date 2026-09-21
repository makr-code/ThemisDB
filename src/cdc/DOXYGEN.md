# CDC DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\cdc\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\cdc\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 79
- Compounds: 255
- Classes/Structs: 146
- Namespaces: 22
- File Compounds: 79

## Namespaces
- @000232131033242367233245252010223212241005370265
- @026263367247306150225017025356055225100052260160
- @033345333111026137313001154313177300234123032314
- @163264013163235232227237260120346267336154223067
- @242325062177215224264354026125155230252011242205
- @250237206066212005040143150057271041010017014316
- @324134255235275077073135253325054114265312363335
- @364370333032010242347276307100234157235026050306
- @365140260354211245174305206321165027175275372053
- @370061244064011061351072241077252014164356111074
- benchmark
- rocksdb
- std
- std::chrono
- std::chrono_literals
- testing
- themis
- themis::@066033116003323140174027117072115053356060347357
- themis::cdc
- themis::cdc::@257330121234017047361220206133010340231363143146
- themis::cdc::error
- themisdb::analytics

## Types
### Classes
- CDCAdminTest
- CDCEventEnrichmentTest
- CDCGDPRRedactionTest
- CDCOperationFilterTest
- CDCProductionFixesTest
- CDCRetentionTest
- CapturingTransport
- CdcDeliveryGateFixture
- CdcFeatureDisabledAuthTest
- CdcPipelineBenchFixture
- CdcSchemaEncoderTest
- CdcSubscriptionAuthTest
- CdcWsGroupIntegrationTest
- CdcWsOverflowTest
- Changefeed
- Changefeed::SubscriptionHandle
- ChangefeedBenchmarkFixture
- ChangefeedBufferTest
- ChangefeedCoreTest
- ConsumerGroupTest
- CrossCollectionStreamTest
- DLQBufferIntegrationTest
- DeadLetterQueueTest
- DiagnosticsHardeningTest
- FanInTest
- OutboxTest
- ReplayAckHardeningTest
- SequenceCounterTest
- TransportDegradationTest
- themis::Changefeed
- themis::Changefeed::SubscriptionHandle
- themis::ChangefeedBuffer
- themis::cdc::CDCAdmin
- themis::cdc::CDCException
- themis::cdc::CDCMaterializedViewMaintainer
- themis::cdc::CdcSchemaEncoder
- themis::cdc::CdcWebSocketHandler
- themis::cdc::ChangeStreamCompressor
- themis::cdc::ConsumerGroupManager
- themis::cdc::CrossCollectionStream
- themis::cdc::DeadLetterQueue
- themis::cdc::DebeziumFormatter
- themis::cdc::DeliveryTracker
- themis::cdc::EventTypeFilter
- themis::cdc::ICDCBackpressureSignal
- themis::cdc::ICDCBatchCommitCoordinator
- themis::cdc::ICDCEventSchema
- themis::cdc::ICDCFanIn
- themis::cdc::ICDCFilterPipeline
- themis::cdc::ICDCPauseControl
- themis::cdc::ICDCReplayController
- themis::cdc::ICDCTransport
- themis::cdc::IDeliveryGuaranteeConfig
- themis::cdc::IEventFilter
- themis::cdc::IFanInMergePolicy
- themis::cdc::IIdempotentCDCListener
- themis::cdc::IReplaySession
- themis::cdc::ISchemaEvolutionCallback
- themis::cdc::ISchemaRegistryBackend
- themis::cdc::InMemoryBackpressureSignal
- themis::cdc::InMemoryBatchCommitCoordinator
- themis::cdc::InMemoryCDCEventSchema
- themis::cdc::InMemoryDeliveryGuaranteeConfig
- themis::cdc::InMemoryFanIn
- themis::cdc::InMemoryFilterPipeline
- themis::cdc::InMemoryIdempotentListener
- themis::cdc::InMemoryPauseControl
- themis::cdc::InMemoryReplayController
- themis::cdc::InMemoryReplaySession
- themis::cdc::InMemorySchemaRegistryBackend
- themis::cdc::KafkaCDCProducer
- themis::cdc::KeyPrefixFilter
- themis::cdc::LatencyHistogram
- themis::cdc::OutboxRelay
- themis::cdc::OutboxWriter
- themis::cdc::PredicateFilter
- themis::cdc::SchemaRegistryClient
- themis::cdc::ScopedTimer
- themis::cdc::TenantBufferManager
- themis::cdc::ThroughputTracker
- themis::cdc::TimestampMergePolicy

### Structs
- Changefeed::ChangeEvent
- Changefeed::CompactionResult
- Changefeed::ListOptions
- Changefeed::RedactionResult
- Changefeed::RetentionPolicy
- Changefeed::Stats
- Changefeed::SubscriptionEntry
- Changefeed::SubscriptionFilter
- Changefeed::Watermarks
- TestCallback
- themis::Changefeed::ChangeEvent
- themis::Changefeed::CompactionResult
- themis::Changefeed::ListOptions
- themis::Changefeed::RedactionResult
- themis::Changefeed::RetentionPolicy
- themis::Changefeed::Stats
- themis::Changefeed::SubscriptionEntry
- themis::Changefeed::SubscriptionFilter
- themis::Changefeed::Watermarks
- themis::ChangefeedBuffer::BufferedEvent
- themis::ChangefeedBuffer::EventTypeBuffer
- themis::ChangefeedBufferConfig
- themis::ChangefeedBufferStats
- themis::cdc::AggregatedEvent
- themis::cdc::BatchConfig
- themis::cdc::BatchInfo
- themis::cdc::CDCMetrics
- themis::cdc::CdcWebSocketHandler::PendingEvent
- themis::cdc::CdcWebSocketHandler::Subscription
- themis::cdc::ChangeStreamCompressor::Config
- themis::cdc::ChangeStreamCompressor::Stats
- themis::cdc::CompressedBatch
- themis::cdc::ConsumerDeliveryStats
- themis::cdc::ConsumerGroupConfig
- themis::cdc::ConsumerGroupInfo
- themis::cdc::ConsumerGroupManager::InFlightRecord
- themis::cdc::CrossCollectionStream::StreamOptions
- themis::cdc::DLQEntry
- themis::cdc::DebeziumEnvelope
- themis::cdc::DebeziumFormatter::Config
- themis::cdc::DebeziumSource
- themis::cdc::DeliveryTracker::ConsumerState
- themis::cdc::DeliveryTracker::PendingEvent
- themis::cdc::DeliveryTrackerConfig
- themis::cdc::DiagnosticsInfo
- themis::cdc::EncodedEvent
- themis::cdc::FanInEvent
- themis::cdc::GDPRRedactionResult
- themis::cdc::HealthStatus
- themis::cdc::InFlightStats
- themis::cdc::InMemoryCDCEventSchema::SchemaEntry
- themis::cdc::KafkaProducerConfig
- themis::cdc::KafkaProducerStats
- themis::cdc::OutboxRecord
- themis::cdc::OutboxRelayConfig
- themis::cdc::PurgeResult
- themis::cdc::ReplayOptions
- themis::cdc::RetentionStatus
- themis::cdc::SchemaConflict
- themis::cdc::SchemaEvolutionDescriptor
- themis::cdc::SchemaInfo
- themis::cdc::SchemaRegistryConfig
- themis::cdc::TenantBufferManager::TenantBufferState
- themis::cdc::TenantConfig
- themis::cdc::TenantStats

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 1408

### CDCAdminTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_admin.cpp`:22
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_admin.cpp`:53
- Brief: n/a
- Parameters: none

#### `void addEventsForKey(const std::string &key, int count, Changefeed::ChangeEventType last_type=Changefeed::ChangeEventType::EVENT_PUT)`
- Source: `tests/cdc/test_cdc_admin.cpp`:71
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `count` (int): n/a
  - `last_type` (Changefeed::ChangeEventType): n/a

#### `void addTestEvents(int count)`
- Source: `tests/cdc/test_cdc_admin.cpp`:60
- Brief: n/a
- Parameters:
  - `count` (int): n/a

### CDCEventEnrichmentTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:22
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:42
- Brief: n/a
- Parameters: none

#### `Changefeed::ChangeEvent recordDeleteWithSnapshot(const std::string &key, std::optional< std::string > before_snap=std::nullopt)`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:71
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `before_snap` (std::optional< std::string >): n/a

#### `Changefeed::ChangeEvent recordPutWithSnapshots(const std::string &key, const std::string &value, std::optional< std::string > before_snap=std::nullopt, std::optional< std::string > after_snap=std::nullopt)`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:52
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a
  - `before_snap` (std::optional< std::string >): n/a
  - `after_snap` (std::optional< std::string >): n/a

### CDCGDPRRedactionTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:24
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:52
- Brief: n/a
- Parameters: none

#### `Changefeed::ChangeEvent makePutEvent(const std::string &key, const std::string &value)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:64
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

### CDCOperationFilterTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_operation_filter.cpp`:16
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_operation_filter.cpp`:41
- Brief: n/a
- Parameters: none

#### `Changefeed::ChangeEvent makeEvent(Changefeed::ChangeEventType type, const std::string &key)`
- Source: `tests/cdc/test_cdc_operation_filter.cpp`:52
- Brief: n/a
- Parameters:
  - `type` (Changefeed::ChangeEventType): n/a
  - `key` (const std::string &): n/a

### CDCProductionFixesTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_production_fixes.cpp`:22
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_production_fixes.cpp`:47
- Brief: n/a
- Parameters: none

### CDCRetentionTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_retention.cpp`:17
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_retention.cpp`:48
- Brief: n/a
- Parameters: none

### CapturingTransport

#### `bool publish(const Changefeed::ChangeEvent &ev) override`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:312
- Brief: n/a
- Parameters:
  - `ev` (const Changefeed::ChangeEvent &): n/a

#### `bool start() override`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:310
- Brief: n/a
- Parameters: none

#### `void stop() override`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:311
- Brief: Stop.
- Parameters: none

### CdcDeliveryGateFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `std::vector< Changefeed::ChangeEvent > makeSyntheticEvents(int count, uint64_t base_seq=1)`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:126
- Brief: n/a
- Parameters:
  - `count` (int): n/a
  - `base_seq` (uint64_t): n/a

#### `void seedEvents(int n)`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:115
- Brief: n/a
- Parameters:
  - `n` (int): n/a

### CdcFeatureDisabledAuthTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:426
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:460
- Brief: n/a
- Parameters: none

### CdcPipelineBenchFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/cdc/bench_cdc_pipeline.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/cdc/bench_cdc_pipeline.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void seedEvents(int n)`
- Source: `benchmarks/cdc/bench_cdc_pipeline.cpp`:65
- Brief: n/a
- Parameters:
  - `n` (int): n/a

### CdcSchemaEncoderTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:279
- Brief: n/a
- Parameters: none

### CdcSubscriptionAuthTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:109
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:160
- Brief: n/a
- Parameters: none

#### `std::string bearer(const char *token)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:170
- Brief: n/a
- Parameters:
  - `token` (const char *): n/a

#### `void recordEvent(const std::string &key, const std::string &value="val")`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:175
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

### CdcWsGroupIntegrationTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:424
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:449
- Brief: n/a
- Parameters: none

#### `void addOrderEvents(int count)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:458
- Brief: n/a
- Parameters:
  - `count` (int): n/a

### CdcWsOverflowTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:261
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:278
- Brief: n/a
- Parameters: none

### Changefeed

#### `Changefeed(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf=nullptr, RetentionPolicy retention=RetentionPolicy::defaults())`
- Source: `include/cdc/changefeed.h`:127
- Brief: n/a
- Parameters:
  - `db` (rocksdb::TransactionDB *): n/a
  - `cf` (rocksdb::ColumnFamilyHandle *): n/a
  - `retention` (RetentionPolicy): n/a

#### `size_t applyRetentionPolicy()`
- Source: `include/cdc/changefeed.h`:236
- Brief: Apply Retention Policy.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: plk(), getStats(), count(), std::chrono::system_clock::now(), time_since_epoch(), deleteOldEventsByTimestamp(), THEMIS_DEBUG(), deleteOldEvents().

#### `void clear()`
- Source: `include/cdc/changefeed.h`:173
- Brief: Clear.
- Parameters: none
- Details: Calls: reset(), NewIterator(), Seek(), Valid(), Next(), key(), ToString(), compare().

#### `CompactionResult compactByKey()`
- Source: `include/cdc/changefeed.h`:223
- Brief: Compact By Key.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: reset(), NewIterator(), Seek(), Valid(), Next(), key(), ToString(), compare().

#### `size_t deleteOldEvents(uint64_t before_sequence)`
- Source: `include/cdc/changefeed.h`:180
- Brief: Delete Old Events.
- Parameters:
  - `before_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: before_sequence Input parameter. Return value. before_sequence Input parameter. Return value. Calls: reset(), NewIterator(), Seek(), Valid(), Next(), key(), ToString(), compare().

#### `size_t deleteOldEventsBySequence(uint64_t before_sequence)`
- Source: `include/cdc/changefeed.h`:188
- Brief: Delete Old Events By Sequence.
- Parameters:
  - `before_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: before_sequence Input parameter. Return value. Calls: deleteOldEvents().

#### `size_t deleteOldEventsByTimestamp(int64_t before_timestamp_ms)`
- Source: `include/cdc/changefeed.h`:197
- Brief: Delete Old Events By Timestamp.
- Parameters:
  - `before_timestamp_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: before_timestamp_ms Input parameter. Return value. before_timestamp_ms Input parameter. Return value. Calls: reset(), NewIterator(), Seek(), Valid(), Next(), key(), ToString(), compare().

#### `ChangeEvent getEvent(uint64_t sequence) const`
- Source: `include/cdc/changefeed.h`:204
- Brief: Get Event.
- Parameters:
  - `sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: sequence Input parameter. Return value.

#### `uint64_t getLatestSequence() const`
- Source: `include/cdc/changefeed.h`:156
- Brief: Get Latest Sequence.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `RetentionPolicy getRetentionPolicy() const`
- Source: `include/cdc/changefeed.h`:248
- Brief: Get Retention Policy.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Stats getStats() const`
- Source: `include/cdc/changefeed.h`:162
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Watermarks getWatermarks() const`
- Source: `include/cdc/changefeed.h`:168
- Brief: Get Watermarks.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool isRetentionCleanupRunning() const noexcept`
- Source: `include/cdc/changefeed.h`:265
- Brief: Is Retention Cleanup Running.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Exception safety: noexcept.

#### `std::vector< ChangeEvent > listEvents() const`
- Source: `include/cdc/changefeed.h`:144
- Brief: List Events.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< ChangeEvent > listEvents(const ListOptions &options) const`
- Source: `include/cdc/changefeed.h`:150
- Brief: List Events.
- Parameters:
  - `options` (const ListOptions &): Input parameter.
- Return: Return value.
- Details: options Input parameter. Return value.

#### `uint64_t loadInitialSequence() const`
- Source: `include/cdc/changefeed.h`:371
- Brief: Load the initial sequence counter value from RocksDB at construction.
- Parameters: none
- Return: Return value.
- Details: Return value. Handles both the binary little-endian uint64 format (new) and the legacy decimal-string format (old). Falls back to scanning events when the DB key cannot be read (e.g. unresolved Merge operands without a registered merge operator).

#### `std::string makeKey(uint64_t sequence) const`
- Source: `include/cdc/changefeed.h`:359
- Brief: Make Key.
- Parameters:
  - `sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: sequence Input parameter. Return value.

#### `std::shared_ptr< rocksdb::MergeOperator > makeSequenceMergeOperator()`
- Source: `include/cdc/changefeed.h`:125
- Brief: Make Sequence Merge Operator.
- Parameters: none
- Return: Return value.
- Details: ===== Changefeed Implementation ===== Return value. Return value. Implements makeSequenceMergeOperator without additional internal calls.

#### `uint64_t nextSequence()`
- Source: `include/cdc/changefeed.h`:364
- Brief: Next Sequence.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: fetch_add(), load(), delta_slice(), Merge(), ok(), compare_exchange_weak(), store(), THEMIS_ERROR().

#### `void notifySubscribers(const ChangeEvent &event)`
- Source: `include/cdc/changefeed.h`:425
- Brief: Notify Subscribers.
- Parameters:
  - `event` (const ChangeEvent &): Input parameter.
- Details: event Input parameter. event Input parameter. Calls: load(), lk(), reserve(), size(), push_back(), matches(), callback(), THEMIS_WARN().

#### `ChangeEvent recordEvent(ChangeEvent event)`
- Source: `include/cdc/changefeed.h`:138
- Brief: Record Event.
- Parameters:
  - `event` (ChangeEvent): Input parameter.
- Return: Return value.
- Throws:
  - error::eventRecordFailed: if an error occurs.
- Details: event Input parameter. Return value. event Input parameter. Return value. error::eventRecordFailed if an error occurs. Calls: nextSequence(), std::chrono::system_clock::now(), time_since_epoch(), count(), toJson(), dump(), makeKey(), Put().

#### `RedactionResult redactByKeyPrefix(const std::string &key_prefix)`
- Source: `include/cdc/changefeed.h`:230
- Brief: Redact By Key Prefix.
- Parameters:
  - `key_prefix` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - error::invalidArgument: if an error occurs.
- Details: key_prefix Input parameter. Return value. key_prefix Input parameter. Return value. error::invalidArgument if an error occurs. Calls: empty(), reset(), NewIterator(), Seek(), makeKey(), Valid(), Next(), key().

#### `void retentionCleanupThread()`
- Source: `include/cdc/changefeed.h`:409
- Brief: Retention Cleanup Thread.
- Parameters: none
- Details: Calls: load(), applyRetentionPolicy(), plk(), compactByKey(), THEMIS_DEBUG(), lock(), wait_for(), THEMIS_ERROR().

#### `uint64_t scanMaxSequence() const`
- Source: `include/cdc/changefeed.h`:378
- Brief: Scan all stored changefeed events and return the maximum sequence number.
- Parameters: none
- Return: Return value.
- Details: Return value. Used as a crash-recovery fallback when loadInitialSequence() cannot read SEQUENCE_KEY directly.

#### `void startRetentionCleanup()`
- Source: `include/cdc/changefeed.h`:253
- Brief: Start Retention Cleanup.
- Parameters: none
- Details: Calls: exchange(), THEMIS_WARN(), std::thread(), lk(), THEMIS_INFO(), count().

#### `void stopRetentionCleanup()`
- Source: `include/cdc/changefeed.h`:258
- Brief: Stop Retention Cleanup.
- Parameters: none
- Details: Calls: exchange(), notify_all(), joinable(), join(), THEMIS_INFO().

#### `SubscriptionHandle subscribe(SubscriptionFilter filter, SubscriptionCallback callback)`
- Source: `include/cdc/changefeed.h`:336
- Brief: Subscribe.
- Parameters:
  - `filter` (SubscriptionFilter): Input parameter.
  - `callback` (SubscriptionCallback): Input parameter.
- Return: Return value.
- Details: filter Input parameter. callback Input parameter. Return value. filter Input parameter. callback Input parameter. Return value. Calls: fetch_add(), lk(), emplace(), std::move(), THEMIS_DEBUG().

#### `void unsubscribe(uint64_t subscription_id) noexcept`
- Source: `include/cdc/changefeed.h`:344
- Brief: Unsubscribe.
- Parameters:
  - `subscription_id` (uint64_t): Identifier of the subscription.
- Details: subscription_id Identifier of the subscription. Exception safety: noexcept.

#### `void updateRetentionPolicy(const RetentionPolicy &policy)`
- Source: `include/cdc/changefeed.h`:242
- Brief: Update Retention Policy.
- Parameters:
  - `policy` (const RetentionPolicy &): Input parameter.
- Details: policy Input parameter. policy Input parameter. Calls: lock(), THEMIS_INFO(), count(), startRetentionCleanup(), stopRetentionCleanup(), notify_all().

#### `bool waitForEvents(uint64_t from_sequence, uint32_t timeout_ms) const`
- Source: `include/cdc/changefeed.h`:386
- Brief: Helper to wait for new events (for long-poll).
- Parameters:
  - `from_sequence` (uint64_t): Input parameter.
  - `timeout_ms` (uint32_t): Input parameter.
- Return: True when the operation succeeds.
- Details: from_sequence Input parameter. timeout_ms Input parameter. True when the operation succeeds.

#### `~Changefeed() noexcept`
- Source: `include/cdc/changefeed.h`:131
- Brief: n/a
- Parameters: none

### Changefeed::ChangeEvent

#### `ChangeEvent fromJson(const nlohmann::json &j)`
- Source: `include/cdc/changefeed.h`:76
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: value(), uint64_t(), contains(), is_null(), int64_t(), is_string().

#### `nlohmann::json toJson() const`
- Source: `include/cdc/changefeed.h`:70
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### Changefeed::RetentionPolicy

#### `RetentionPolicy defaults()`
- Source: `include/cdc/changefeed.h`:104
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

### Changefeed::SubscriptionFilter

#### `bool matches(const ChangeEvent &ev) const noexcept`
- Source: `include/cdc/changefeed.h`:281
- Brief: Matches.
- Parameters:
  - `ev` (const ChangeEvent &): Input parameter.
- Return: True when the operation succeeds.
- Details: ev Input parameter. True when the operation succeeds. Exception safety: noexcept.

### Changefeed::SubscriptionHandle

#### `SubscriptionHandle()=default`
- Source: `include/cdc/changefeed.h`:286
- Brief: n/a
- Parameters: none

#### `SubscriptionHandle(Changefeed *feed, uint64_t id) noexcept`
- Source: `include/cdc/changefeed.h`:321
- Brief: n/a
- Parameters:
  - `feed` (Changefeed *): n/a
  - `id` (uint64_t): n/a

#### `SubscriptionHandle(SubscriptionHandle &&other) noexcept`
- Source: `include/cdc/changefeed.h`:292
- Brief: n/a
- Parameters:
  - `other` (SubscriptionHandle &&): n/a

#### `SubscriptionHandle(const SubscriptionHandle &)=delete`
- Source: `include/cdc/changefeed.h`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SubscriptionHandle &): n/a

#### `bool active() const noexcept`
- Source: `include/cdc/changefeed.h`:315
- Brief: n/a
- Parameters: none

#### `void cancel() noexcept`
- Source: `include/cdc/changefeed.h`:313
- Brief: Cancel.
- Parameters: none
- Details: Exception safety: noexcept.

#### `uint64_t id() const noexcept`
- Source: `include/cdc/changefeed.h`:317
- Brief: n/a
- Parameters: none

#### `SubscriptionHandle & operator=(SubscriptionHandle &&other) noexcept`
- Source: `include/cdc/changefeed.h`:298
- Brief: n/a
- Parameters:
  - `other` (SubscriptionHandle &&): n/a

#### `SubscriptionHandle & operator=(const SubscriptionHandle &)=delete`
- Source: `include/cdc/changefeed.h`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SubscriptionHandle &): n/a

#### `~SubscriptionHandle() noexcept`
- Source: `include/cdc/changefeed.h`:287
- Brief: n/a
- Parameters: none

### ChangefeedBenchmarkFixture

#### `void SetUp(const ::benchmark::State &state) override`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:24
- Brief: n/a
- Parameters:
  - `state` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

### ChangefeedBufferTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:25
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:45
- Brief: n/a
- Parameters: none

#### `Changefeed::ChangeEvent makeDelete(const std::string &key)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:60
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `Changefeed::ChangeEvent makePut(const std::string &key, const std::string &value="{}")`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:51
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

### ChangefeedCoreTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:26
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:48
- Brief: n/a
- Parameters: none

#### `Changefeed::ChangeEvent makeDelete(const std::string &key)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:66
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `Changefeed::ChangeEvent makePut(const std::string &key, const std::string &value="{}")`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:57
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

### ConsumerGroupTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:22
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:52
- Brief: n/a
- Parameters: none

#### `void addEvents(int count, const std::string &key_prefix="key")`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:65
- Brief: n/a
- Parameters:
  - `count` (int): n/a
  - `key_prefix` (const std::string &): n/a

### CrossCollectionStreamTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:47
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:67
- Brief: n/a
- Parameters: none

#### `Changefeed::ChangeEvent makeEvent(const std::string &key, Changefeed::ChangeEventType type=Changefeed::ChangeEventType::EVENT_PUT, const std::string &value="v", int64_t timestamp_ms=0)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:94
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `type` (Changefeed::ChangeEventType): n/a
  - `value` (const std::string &): n/a
  - `timestamp_ms` (int64_t): n/a

#### `std::unique_ptr< RocksDBWrapper > openDB(const std::string &path)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:32
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

### DLQBufferIntegrationTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:227
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:247
- Brief: n/a
- Parameters: none

### DeadLetterQueueTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:21
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:46
- Brief: n/a
- Parameters: none

#### `Changefeed::ChangeEvent makeEvent(const std::string &key, const std::string &value="v")`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:58
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

### DiagnosticsHardeningTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_diagnostics_hardening.cpp`:58
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_diagnostics_hardening.cpp`:79
- Brief: n/a
- Parameters: none

#### `void seedEvents(int n, const std::string &prefix="key")`
- Source: `tests/cdc/test_cdc_diagnostics_hardening.cpp`:86
- Brief: n/a
- Parameters:
  - `n` (int): n/a
  - `prefix` (const std::string &): n/a

### FanInTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:58
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:76
- Brief: n/a
- Parameters: none

#### `Changefeed::ChangeEvent makeEv(const std::string &key, const std::string &value="v", int64_t ts_ms=0, Changefeed::ChangeEventType t=Changefeed::ChangeEventType::EVENT_PUT)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:103
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a
  - `ts_ms` (int64_t): n/a
  - `t` (Changefeed::ChangeEventType): n/a

#### `std::unique_ptr< RocksDBWrapper > openDB(const std::string &path)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:41
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

#### `void put(Changefeed &feed, const std::string &key, const std::string &value, int64_t timestamp_ms=0)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:117
- Brief: n/a
- Parameters:
  - `feed` (Changefeed &): n/a
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a
  - `timestamp_ms` (int64_t): n/a

### OutboxTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_outbox.cpp`:24
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_outbox.cpp`:55
- Brief: n/a
- Parameters: none

#### `OutboxRecord makeRecord(const std::string &key, const std::string &collection="orders", const std::string &value="{}")`
- Source: `tests/cdc/test_cdc_outbox.cpp`:68
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `collection` (const std::string &): n/a
  - `value` (const std::string &): n/a

### ReplayAckHardeningTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:60
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:80
- Brief: n/a
- Parameters: none

#### `std::vector< Changefeed::ChangeEvent > makeEvents(uint64_t from, int count)`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:103
- Brief: Build synthetic ChangeEvents with sequences [from, from+count).
- Parameters:
  - `from` (uint64_t): n/a
  - `count` (int): n/a

#### `std::vector< uint64_t > seedEvents(int n, const std::string &prefix="key")`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:87
- Brief: Record n events and return the list of assigned sequences.
- Parameters:
  - `n` (int): n/a
  - `prefix` (const std::string &): n/a

### SequenceCounterTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:52
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:63
- Brief: n/a
- Parameters: none

#### `Changefeed::ChangeEvent makePut(const std::string &key, const std::string &val="{}")`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:69
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `val` (const std::string &): n/a

### TestCallback

#### `void onCompatible(const SchemaEvolutionDescriptor &d) override`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:44
- Brief: On Compatible.
- Parameters:
  - `descriptor` (const SchemaEvolutionDescriptor &): Input parameter.
- Details: descriptor Input parameter.

#### `void onIncompatible(const SchemaEvolutionDescriptor &d, const SchemaConflict &c) override`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:50
- Brief: On Incompatible.
- Parameters:
  - `descriptor` (const SchemaEvolutionDescriptor &): Input parameter.
  - `conflict` (const SchemaConflict &): Input parameter.
- Details: descriptor Input parameter. conflict Input parameter.

### TransportDegradationTest

#### `void SetUp() override`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:64
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:85
- Brief: n/a
- Parameters: none

#### `std::vector< Changefeed::ChangeEvent > makeEvents(uint64_t from, int count, const std::string &key_prefix="trd")`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:105
- Brief: Build a vector of synthetic ChangeEvents with sequences [from, from+count).
- Parameters:
  - `from` (uint64_t): n/a
  - `count` (int): n/a
  - `key_prefix` (const std::string &): n/a

#### `void seedEvents(int n, const std::string &key_prefix="key")`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:93
- Brief: Record n events with distinct keys into the changefeed.
- Parameters:
  - `n` (int): n/a
  - `key_prefix` (const std::string &): n/a

### bench_cdc_delivery_gates.cpp

#### `BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG01_TrackDelivery10Events)(benchmark`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcDeliveryGateFixture): n/a
  - `<unnamed>` (CDG01_TrackDelivery10Events): n/a

#### `BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG02_AcknowledgeSingleEvent)(benchmark`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcDeliveryGateFixture): n/a
  - `<unnamed>` (CDG02_AcknowledgeSingleEvent): n/a

#### `BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG03_AcknowledgeUpTo100Events)(benchmark`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcDeliveryGateFixture): n/a
  - `<unnamed>` (CDG03_AcknowledgeUpTo100Events): n/a

#### `BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG04_CreateDeleteGroup)(benchmark`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcDeliveryGateFixture): n/a
  - `<unnamed>` (CDG04_CreateDeleteGroup): n/a

#### `BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG05_FetchEventsAtLeastOnce50)(benchmark`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:270
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcDeliveryGateFixture): n/a
  - `<unnamed>` (CDG05_FetchEventsAtLeastOnce50): n/a

#### `BENCHMARK_DEFINE_F(CdcDeliveryGateFixture, CDG06_ReplayFromSequenceDrain100)(benchmark`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcDeliveryGateFixture): n/a
  - `<unnamed>` (CDG06_ReplayFromSequenceDrain100): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:321
- Brief: n/a
- Parameters: none

#### `Unit(benchmark::kMicrosecond) -> Repetitions(kRepetitions) ->Iterations(500) ->UseRealTime()`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

#### `Unit(benchmark::kMillisecond) -> Repetitions(kRepetitions) ->Iterations(200) ->UseRealTime()`
- Source: `benchmarks/cdc/bench_cdc_delivery_gates.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

### bench_cdc_pipeline.cpp

#### `Arg(1) -> Arg(2) ->Arg(4) ->Unit(benchmark::kMillisecond) ->Threads(1) ->Iterations(100)`
- Source: `benchmarks/cdc/bench_cdc_pipeline.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Arg(10) -> Arg(50) ->Arg(100) ->Unit(benchmark::kMillisecond) ->Iterations(200)`
- Source: `benchmarks/cdc/bench_cdc_pipeline.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `BENCHMARK_DEFINE_F(CdcPipelineBenchFixture, AcknowledgeEvents)(benchmark`
- Source: `benchmarks/cdc/bench_cdc_pipeline.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcPipelineBenchFixture): n/a
  - `<unnamed>` (AcknowledgeEvents): n/a

#### `BENCHMARK_DEFINE_F(CdcPipelineBenchFixture, ConcurrentFetch)(benchmark`
- Source: `benchmarks/cdc/bench_cdc_pipeline.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcPipelineBenchFixture): n/a
  - `<unnamed>` (ConcurrentFetch): n/a

#### `BENCHMARK_DEFINE_F(CdcPipelineBenchFixture, CreateDeleteGroup)(benchmark`
- Source: `benchmarks/cdc/bench_cdc_pipeline.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcPipelineBenchFixture): n/a
  - `<unnamed>` (CreateDeleteGroup): n/a

#### `BENCHMARK_DEFINE_F(CdcPipelineBenchFixture, FetchEventsAtLeastOnce)(benchmark`
- Source: `benchmarks/cdc/bench_cdc_pipeline.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcPipelineBenchFixture): n/a
  - `<unnamed>` (FetchEventsAtLeastOnce): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/cdc/bench_cdc_pipeline.cpp`:215
- Brief: n/a
- Parameters: none

#### `Unit(benchmark::kMicrosecond) -> Iterations(2000)`
- Source: `benchmarks/cdc/bench_cdc_pipeline.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

### bench_changefeed_throughput.cpp

#### `Arg(1) -> Arg(10) ->Arg(50) ->Unit(benchmark::kMillisecond) ->Iterations(3)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Arg(10) -> Arg(100) ->Arg(1000) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:339
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `Arg(100) -> Arg(1000) ->Arg(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Arg(2) -> Arg(50) ->Unit(benchmark::kMillisecond) ->Iterations(5)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:504
- Brief: n/a
- Parameters:
  - `<unnamed>` (2): n/a

#### `BENCHMARK_DEFINE_F(ChangefeedBenchmarkFixture, ConcurrentSubscribers)(benchmark`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBenchmarkFixture): n/a
  - `<unnamed>` (ConcurrentSubscribers): n/a

#### `BENCHMARK_DEFINE_F(ChangefeedBenchmarkFixture, EventPolling)(benchmark`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBenchmarkFixture): n/a
  - `<unnamed>` (EventPolling): n/a

#### `BENCHMARK_DEFINE_F(ChangefeedBenchmarkFixture, EventRecordingThroughput)(benchmark`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBenchmarkFixture): n/a
  - `<unnamed>` (EventRecordingThroughput): n/a

#### `void BM_BurstWrites(benchmark::State &state)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:290
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EventTypeMix(benchmark::State &state)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:226
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ListEventsLatency(benchmark::State &state)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:577
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RecordEventLatency(benchmark::State &state)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:516
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ReplicationLag(benchmark::State &state)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:348
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ReplicationLagWAN(benchmark::State &state)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:425
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Threads(1) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Unit(benchmark::kMillisecond) -> Iterations(5)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:416
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

#### `UseManualTime() -> Threads(1) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/cdc/bench_changefeed_throughput.cpp`:568
- Brief: n/a
- Parameters: none

### test_cdc_admin.cpp

#### `TEST_F(CDCAdminTest, BasicHealthCheck)`
- Source: `tests/cdc/test_cdc_admin.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (BasicHealthCheck): n/a

#### `TEST_F(CDCAdminTest, CompactLog_EmptyLog)`
- Source: `tests/cdc/test_cdc_admin.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (CompactLog_EmptyLog): n/a

#### `TEST_F(CDCAdminTest, CompactLog_IsIdempotent)`
- Source: `tests/cdc/test_cdc_admin.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (CompactLog_IsIdempotent): n/a

#### `TEST_F(CDCAdminTest, CompactLog_MultipleKeys)`
- Source: `tests/cdc/test_cdc_admin.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (CompactLog_MultipleKeys): n/a

#### `TEST_F(CDCAdminTest, CompactLog_NoOpWhenAllKeysUnique)`
- Source: `tests/cdc/test_cdc_admin.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (CompactLog_NoOpWhenAllKeysUnique): n/a

#### `TEST_F(CDCAdminTest, CompactLog_PreservesDeleteTombstones)`
- Source: `tests/cdc/test_cdc_admin.cpp`:313
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (CompactLog_PreservesDeleteTombstones): n/a

#### `TEST_F(CDCAdminTest, CompactLog_RemovesSupersededEvents)`
- Source: `tests/cdc/test_cdc_admin.cpp`:278
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (CompactLog_RemovesSupersededEvents): n/a

#### `TEST_F(CDCAdminTest, CompactLog_ResultJsonSerialization)`
- Source: `tests/cdc/test_cdc_admin.cpp`:335
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (CompactLog_ResultJsonSerialization): n/a

#### `TEST_F(CDCAdminTest, DiagnosticsExport)`
- Source: `tests/cdc/test_cdc_admin.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (DiagnosticsExport): n/a

#### `TEST_F(CDCAdminTest, HealthStatusJSONSerialization)`
- Source: `tests/cdc/test_cdc_admin.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (HealthStatusJSONSerialization): n/a

#### `TEST_F(CDCAdminTest, InvalidPurgeRange)`
- Source: `tests/cdc/test_cdc_admin.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (InvalidPurgeRange): n/a

#### `TEST_F(CDCAdminTest, MultiplePurgeOperations)`
- Source: `tests/cdc/test_cdc_admin.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (MultiplePurgeOperations): n/a

#### `TEST_F(CDCAdminTest, PurgeAllEvents)`
- Source: `tests/cdc/test_cdc_admin.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (PurgeAllEvents): n/a

#### `TEST_F(CDCAdminTest, PurgeBySequenceRange)`
- Source: `tests/cdc/test_cdc_admin.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (PurgeBySequenceRange): n/a

#### `TEST_F(CDCAdminTest, PurgeByTimestamp)`
- Source: `tests/cdc/test_cdc_admin.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (PurgeByTimestamp): n/a

#### `TEST_F(CDCAdminTest, PurgeOlderThanNegativeTimestampThrows)`
- Source: `tests/cdc/test_cdc_admin.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (PurgeOlderThanNegativeTimestampThrows): n/a

#### `TEST_F(CDCAdminTest, PurgeResultJSONSerialization)`
- Source: `tests/cdc/test_cdc_admin.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (PurgeResultJSONSerialization): n/a

#### `TEST_F(CDCAdminTest, ReplayEmptyRange)`
- Source: `tests/cdc/test_cdc_admin.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (ReplayEmptyRange): n/a

#### `TEST_F(CDCAdminTest, ReplayFromSequence)`
- Source: `tests/cdc/test_cdc_admin.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (ReplayFromSequence): n/a

#### `TEST_F(CDCAdminTest, ReplayWithLimit)`
- Source: `tests/cdc/test_cdc_admin.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCAdminTest): n/a
  - `<unnamed>` (ReplayWithLimit): n/a

### test_cdc_backpressure_signal.cpp

#### `TEST(InMemoryBackpressureSignalTest, CallbackInvokedOnChange)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (CallbackInvokedOnChange): n/a

#### `TEST(InMemoryBackpressureSignalTest, CallbackNotInvokedWhenLevelUnchanged)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (CallbackNotInvokedWhenLevelUnchanged): n/a

#### `TEST(InMemoryBackpressureSignalTest, ClearBackpressureResetsToNone)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (ClearBackpressureResetsToNone): n/a

#### `TEST(InMemoryBackpressureSignalTest, ClearCallbackTriggeredWhenWasNonNone)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (ClearCallbackTriggeredWhenWasNonNone): n/a

#### `TEST(InMemoryBackpressureSignalTest, ConcurrentSignals)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (ConcurrentSignals): n/a

#### `TEST(InMemoryBackpressureSignalTest, InitialLevelIsNone)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:31
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (InitialLevelIsNone): n/a

#### `TEST(InMemoryBackpressureSignalTest, PolymorphicUsage)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (PolymorphicUsage): n/a

#### `TEST(InMemoryBackpressureSignalTest, ReduceLevelOnRecovery)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (ReduceLevelOnRecovery): n/a

#### `TEST(InMemoryBackpressureSignalTest, SetCallbackReplacesPrevious)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (SetCallbackReplacesPrevious): n/a

#### `TEST(InMemoryBackpressureSignalTest, SetCallbackToNull)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (SetCallbackToNull): n/a

#### `TEST(InMemoryBackpressureSignalTest, SignalCritical)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (SignalCritical): n/a

#### `TEST(InMemoryBackpressureSignalTest, SignalHigh)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (SignalHigh): n/a

#### `TEST(InMemoryBackpressureSignalTest, SignalLow)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:40
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (SignalLow): n/a

#### `TEST(InMemoryBackpressureSignalTest, SignalMedium)`
- Source: `tests/cdc/test_cdc_backpressure_signal.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackpressureSignalTest): n/a
  - `<unnamed>` (SignalMedium): n/a

### test_cdc_batch_commit_coordinator.cpp

#### `TEST(InMemoryBatchCommitCoordinatorTest, AddEventAdded)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (AddEventAdded): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, AddEventBatchFull)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (AddEventBatchFull): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, AddEventNoBatchOpen)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (AddEventNoBatchOpen): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, BeginBatchReturnsNonZeroId)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (BeginBatchReturnsNonZeroId): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, BeginBatchSucceedsAfterCommit)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (BeginBatchSucceedsAfterCommit): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, BeginBatchSucceedsAfterRollback)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (BeginBatchSucceedsAfterRollback): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, BeginBatchWhileOpenReturnsZero)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (BeginBatchWhileOpenReturnsZero): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, CommitBatchCommitsEvents)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (CommitBatchCommitsEvents): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, CommitBatchNoBatchOpen)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (CommitBatchNoBatchOpen): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, CommitHistoryFifoEviction)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (CommitHistoryFifoEviction): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, CommittedEventsReturnedInOrder)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (CommittedEventsReturnedInOrder): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, CommittedEventsUnknownIdReturnsEmpty)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (CommittedEventsUnknownIdReturnsEmpty): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, ConcurrentAddEventThreadSafe)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (ConcurrentAddEventThreadSafe): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, InfoReflectsCurrentState)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (InfoReflectsCurrentState): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, IsCommittedReflectsState)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (IsCommittedReflectsState): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, PolymorphicUsage)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (PolymorphicUsage): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, RollbackBatchClearsEvents)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (RollbackBatchClearsEvents): n/a

#### `TEST(InMemoryBatchCommitCoordinatorTest, RollbackBatchNoBatchOpen)`
- Source: `tests/cdc/test_cdc_batch_commit_coordinator.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBatchCommitCoordinatorTest): n/a
  - `<unnamed>` (RollbackBatchNoBatchOpen): n/a

### test_cdc_change_stream_compressor.cpp

#### `TEST(ChangeStreamCompressorTest, AllEventFieldsPreservedOnRoundTrip)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (AllEventFieldsPreservedOnRoundTrip): n/a

#### `TEST(ChangeStreamCompressorTest, BelowThresholdStoredUncompressed)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (BelowThresholdStoredUncompressed): n/a

#### `TEST(ChangeStreamCompressorTest, CompressionRatioAboveOneForLargeBatch)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (CompressionRatioAboveOneForLargeBatch): n/a

#### `TEST(ChangeStreamCompressorTest, DecompressCorruptedZstdThrows)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (DecompressCorruptedZstdThrows): n/a

#### `TEST(ChangeStreamCompressorTest, DecompressNoneBatch)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (DecompressNoneBatch): n/a

#### `TEST(ChangeStreamCompressorTest, DecompressZstdBatch)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (DecompressZstdBatch): n/a

#### `TEST(ChangeStreamCompressorTest, DefaultConfig)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (DefaultConfig): n/a

#### `TEST(ChangeStreamCompressorTest, DeleteEventRoundTrip)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:431
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (DeleteEventRoundTrip): n/a

#### `TEST(ChangeStreamCompressorTest, DeserializeSerializedEmptyBatchSucceeds)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:222
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (DeserializeSerializedEmptyBatchSucceeds): n/a

#### `TEST(ChangeStreamCompressorTest, DeserializeTruncatedReturnsNullopt)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (DeserializeTruncatedReturnsNullopt): n/a

#### `TEST(ChangeStreamCompressorTest, DeserializeWrongMagicReturnsNullopt)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (DeserializeWrongMagicReturnsNullopt): n/a

#### `TEST(ChangeStreamCompressorTest, EmptyBatchRoundTrip)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (EmptyBatchRoundTrip): n/a

#### `TEST(ChangeStreamCompressorTest, EmptyBatchSerializeDeserialize)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (EmptyBatchSerializeDeserialize): n/a

#### `TEST(ChangeStreamCompressorTest, ExplicitNoneAlgorithmNeverCompresses)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (ExplicitNoneAlgorithmNeverCompresses): n/a

#### `TEST(ChangeStreamCompressorTest, LargeBatchRoundTrip)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (LargeBatchRoundTrip): n/a

#### `TEST(ChangeStreamCompressorTest, LargeBatchUsesZSTD)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (LargeBatchUsesZSTD): n/a

#### `TEST(ChangeStreamCompressorTest, ResetStatsZeroesAllCounters)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:361
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (ResetStatsZeroesAllCounters): n/a

#### `TEST(ChangeStreamCompressorTest, SerializeDeserializePreservesHeader)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (SerializeDeserializePreservesHeader): n/a

#### `TEST(ChangeStreamCompressorTest, SetConfigUpdatesThreshold)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (SetConfigUpdatesThreshold): n/a

#### `TEST(ChangeStreamCompressorTest, SingleEventRoundTrip)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (SingleEventRoundTrip): n/a

#### `TEST(ChangeStreamCompressorTest, StatsAfterCompress)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (StatsAfterCompress): n/a

#### `TEST(ChangeStreamCompressorTest, StatsDecompressErrorsCounted)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:331
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (StatsDecompressErrorsCounted): n/a

#### `TEST(ChangeStreamCompressorTest, StatsDecompressedCountedAfterDecompress)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:303
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (StatsDecompressedCountedAfterDecompress): n/a

#### `TEST(ChangeStreamCompressorTest, StatsSkippedCountedForNoneBatches)`
- Source: `tests/cdc/test_cdc_change_stream_compressor.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangeStreamCompressorTest): n/a
  - `<unnamed>` (StatsSkippedCountedForNoneBatches): n/a

### test_cdc_changefeed_buffer.cpp

#### `TEST_F(ChangefeedBufferTest, AsyncFlushThreadStartsAndStops)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:432
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (AsyncFlushThreadStartsAndStops): n/a

#### `TEST_F(ChangefeedBufferTest, AutoFlushWhenSizeThresholdReached)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:328
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (AutoFlushWhenSizeThresholdReached): n/a

#### `TEST_F(ChangefeedBufferTest, CompressionDisabledStillFlushes)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:374
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (CompressionDisabledStillFlushes): n/a

#### `TEST_F(ChangefeedBufferTest, CompressionEnabledDoesNotDropEvents)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:351
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (CompressionEnabledDoesNotDropEvents): n/a

#### `TEST_F(ChangefeedBufferTest, DestructorStopsBuffer)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (DestructorStopsBuffer): n/a

#### `TEST_F(ChangefeedBufferTest, DoubleStartIsIdempotent)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (DoubleStartIsIdempotent): n/a

#### `TEST_F(ChangefeedBufferTest, EmptyKeyReturnsEventWithoutRecording)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (EmptyKeyReturnsEventWithoutRecording): n/a

#### `TEST_F(ChangefeedBufferTest, ExplicitFlushWritesToChangefeed)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (ExplicitFlushWritesToChangefeed): n/a

#### `TEST_F(ChangefeedBufferTest, FlushEmptyBufferReturnsZero)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (FlushEmptyBufferReturnsZero): n/a

#### `TEST_F(ChangefeedBufferTest, FlushForSpecificEventType)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (FlushForSpecificEventType): n/a

#### `TEST_F(ChangefeedBufferTest, FlushForUnknownTypeReturnsZero)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (FlushForUnknownTypeReturnsZero): n/a

#### `TEST_F(ChangefeedBufferTest, NullChangefeedThrows)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (NullChangefeedThrows): n/a

#### `TEST_F(ChangefeedBufferTest, PutAndDeleteEventsBufferedSeparately)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:458
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (PutAndDeleteEventsBufferedSeparately): n/a

#### `TEST_F(ChangefeedBufferTest, RateLimitingConfigDoesNotCrash)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (RateLimitingConfigDoesNotCrash): n/a

#### `TEST_F(ChangefeedBufferTest, RecordEventReturnsSequenceZeroWhileBuffered)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (RecordEventReturnsSequenceZeroWhileBuffered): n/a

#### `TEST_F(ChangefeedBufferTest, RecordMultipleEventsBeforeFlush)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (RecordMultipleEventsBeforeFlush): n/a

#### `TEST_F(ChangefeedBufferTest, SetConfigUpdatesFlushInterval)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:309
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (SetConfigUpdatesFlushInterval): n/a

#### `TEST_F(ChangefeedBufferTest, SetDeadLetterQueueAttaches)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:414
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (SetDeadLetterQueueAttaches): n/a

#### `TEST_F(ChangefeedBufferTest, StartAndStopLifecycle)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (StartAndStopLifecycle): n/a

#### `TEST_F(ChangefeedBufferTest, StatsInitiallyZero)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (StatsInitiallyZero): n/a

#### `TEST_F(ChangefeedBufferTest, StatsTrackBufferedAndFlushedEvents)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:284
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (StatsTrackBufferedAndFlushedEvents): n/a

#### `TEST_F(ChangefeedBufferTest, StopPerformsFinalFlush)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (StopPerformsFinalFlush): n/a

#### `TEST_F(ChangefeedBufferTest, ValidConstructionDoesNotThrow)`
- Source: `tests/cdc/test_cdc_changefeed_buffer.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferTest): n/a
  - `<unnamed>` (ValidConstructionDoesNotThrow): n/a

### test_cdc_changefeed_core.cpp

#### `TEST_F(ChangefeedCoreTest, CallbackExceptionDoesNotPropagateToRecordEvent)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (CallbackExceptionDoesNotPropagateToRecordEvent): n/a

#### `TEST_F(ChangefeedCoreTest, CallbackInvokedOnMatchingEvent)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (CallbackInvokedOnMatchingEvent): n/a

#### `TEST_F(ChangefeedCoreTest, CallbackNotInvokedWhenFilterExcludesKey)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (CallbackNotInvokedWhenFilterExcludesKey): n/a

#### `TEST_F(ChangefeedCoreTest, CallbackNotInvokedWhenTypeFiltered)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (CallbackNotInvokedWhenTypeFiltered): n/a

#### `TEST_F(ChangefeedCoreTest, CallbackReceivesCorrectEventData)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (CallbackReceivesCorrectEventData): n/a

#### `TEST_F(ChangefeedCoreTest, CancelledSubscriberNoLongerReceivesEvents)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:272
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (CancelledSubscriberNoLongerReceivesEvents): n/a

#### `TEST_F(ChangefeedCoreTest, ClearOnEmptyFeedIsNoop)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ClearOnEmptyFeedIsNoop): n/a

#### `TEST_F(ChangefeedCoreTest, ClearRemovesAllEvents)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ClearRemovesAllEvents): n/a

#### `TEST_F(ChangefeedCoreTest, CombinedPrefixAndTypeFilter)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (CombinedPrefixAndTypeFilter): n/a

#### `TEST_F(ChangefeedCoreTest, ConcurrentSubscribeAndRecordIsThreadSafe)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:680
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ConcurrentSubscribeAndRecordIsThreadSafe): n/a

#### `TEST_F(ChangefeedCoreTest, EmptyFilterMatchesAllEvents)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (EmptyFilterMatchesAllEvents): n/a

#### `TEST_F(ChangefeedCoreTest, EventTypesFilterMatchesSpecifiedTypes)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (EventTypesFilterMatchesSpecifiedTypes): n/a

#### `TEST_F(ChangefeedCoreTest, EventTypesMultiTypeFilter)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (EventTypesMultiTypeFilter): n/a

#### `TEST_F(ChangefeedCoreTest, GetEventBySequenceReturnsCorrectEvent)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:551
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (GetEventBySequenceReturnsCorrectEvent): n/a

#### `TEST_F(ChangefeedCoreTest, GetEventForNonExistentSequenceThrows)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:560
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (GetEventForNonExistentSequenceThrows): n/a

#### `TEST_F(ChangefeedCoreTest, GetLatestSequenceEmptyFeedIsZero)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:358
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (GetLatestSequenceEmptyFeedIsZero): n/a

#### `TEST_F(ChangefeedCoreTest, GetLatestSequenceMonotonicallyIncreases)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:362
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (GetLatestSequenceMonotonicallyIncreases): n/a

#### `TEST_F(ChangefeedCoreTest, GetStatsAfterRecordingEvents)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:344
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (GetStatsAfterRecordingEvents): n/a

#### `TEST_F(ChangefeedCoreTest, GetStatsEmptyFeed)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (GetStatsEmptyFeed): n/a

#### `TEST_F(ChangefeedCoreTest, GetWatermarksAfterRecordingEvents)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:663
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (GetWatermarksAfterRecordingEvents): n/a

#### `TEST_F(ChangefeedCoreTest, GetWatermarksEmptyFeed)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:657
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (GetWatermarksEmptyFeed): n/a

#### `TEST_F(ChangefeedCoreTest, HandleCancelDeactivates)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (HandleCancelDeactivates): n/a

#### `TEST_F(ChangefeedCoreTest, HandleDestructorCancelsSubscription)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (HandleDestructorCancelsSubscription): n/a

#### `TEST_F(ChangefeedCoreTest, HandleMoveTransfersOwnership)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (HandleMoveTransfersOwnership): n/a

#### `TEST_F(ChangefeedCoreTest, JsonRoundtripDeleteEvent)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:586
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (JsonRoundtripDeleteEvent): n/a

#### `TEST_F(ChangefeedCoreTest, JsonRoundtripPutEvent)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:568
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (JsonRoundtripPutEvent): n/a

#### `TEST_F(ChangefeedCoreTest, JsonRoundtripRedactedEvent)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:640
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (JsonRoundtripRedactedEvent): n/a

#### `TEST_F(ChangefeedCoreTest, JsonRoundtripTransactionCommitEvent)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:600
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (JsonRoundtripTransactionCommitEvent): n/a

#### `TEST_F(ChangefeedCoreTest, JsonRoundtripTransactionRollbackEvent)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:612
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (JsonRoundtripTransactionRollbackEvent): n/a

#### `TEST_F(ChangefeedCoreTest, JsonRoundtripWithBeforeAfterSnapshots)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:624
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (JsonRoundtripWithBeforeAfterSnapshots): n/a

#### `TEST_F(ChangefeedCoreTest, KeyPrefixFilterMatchesPrefixedKeys)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (KeyPrefixFilterMatchesPrefixedKeys): n/a

#### `TEST_F(ChangefeedCoreTest, ListEventsEmptyEventTypesReturnsAll)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:475
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ListEventsEmptyEventTypesReturnsAll): n/a

#### `TEST_F(ChangefeedCoreTest, ListEventsReturnsAllEventsInOrder)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:394
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ListEventsReturnsAllEventsInOrder): n/a

#### `TEST_F(ChangefeedCoreTest, ListEventsToSequenceZeroMeansUnbounded)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:527
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ListEventsToSequenceZeroMeansUnbounded): n/a

#### `TEST_F(ChangefeedCoreTest, ListEventsWithFromSequenceAndLimit)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:485
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ListEventsWithFromSequenceAndLimit): n/a

#### `TEST_F(ChangefeedCoreTest, ListEventsWithFromSequenceFilters)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:408
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ListEventsWithFromSequenceFilters): n/a

#### `TEST_F(ChangefeedCoreTest, ListEventsWithKeyPrefixFilter)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:435
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ListEventsWithKeyPrefixFilter): n/a

#### `TEST_F(ChangefeedCoreTest, ListEventsWithLimitCapsResults)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:424
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ListEventsWithLimitCapsResults): n/a

#### `TEST_F(ChangefeedCoreTest, ListEventsWithMultiTypeFilter)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:464
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ListEventsWithMultiTypeFilter): n/a

#### `TEST_F(ChangefeedCoreTest, ListEventsWithSingleEventTypeFilter)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:450
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ListEventsWithSingleEventTypeFilter): n/a

#### `TEST_F(ChangefeedCoreTest, ListEventsWithToSequenceBound)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:505
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (ListEventsWithToSequenceBound): n/a

#### `TEST_F(ChangefeedCoreTest, MoveAssignmentCancelsOldSubscription)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (MoveAssignmentCancelsOldSubscription): n/a

#### `TEST_F(ChangefeedCoreTest, MultipleSubscribersAllNotified)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (MultipleSubscribersAllNotified): n/a

#### `TEST_F(ChangefeedCoreTest, SubscribeReturnsActiveHandle)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (SubscribeReturnsActiveHandle): n/a

#### `TEST_F(ChangefeedCoreTest, SubscribeWithinCallbackDoesNotDeadlock)`
- Source: `tests/cdc/test_cdc_changefeed_core.cpp`:313
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedCoreTest): n/a
  - `<unnamed>` (SubscribeWithinCallbackDoesNotDeadlock): n/a

### test_cdc_changefeed_sequence_counter.cpp

#### `TEST(SequenceCounterCrashRecovery, ContinuesAfterReopen)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequenceCounterCrashRecovery): n/a
  - `<unnamed>` (ContinuesAfterReopen): n/a

#### `TEST(SequenceCounterLegacyInit, LoadsLegacyStringFormatOnConstruction)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:363
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequenceCounterLegacyInit): n/a
  - `<unnamed>` (LoadsLegacyStringFormatOnConstruction): n/a

#### `TEST(SequenceMergeOperatorFactory, ReturnsNamedOperator)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequenceMergeOperatorFactory): n/a
  - `<unnamed>` (ReturnsNamedOperator): n/a

#### `TEST(SequenceMergeOperatorSemantics, MergesFromAbsent)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequenceMergeOperatorSemantics): n/a
  - `<unnamed>` (MergesFromAbsent): n/a

#### `TEST(SequenceMergeOperatorSemantics, MergesOnTopOfBinaryBase)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequenceMergeOperatorSemantics): n/a
  - `<unnamed>` (MergesOnTopOfBinaryBase): n/a

#### `TEST(SequenceMergeOperatorSemantics, MergesOnTopOfLegacyStringBase)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequenceMergeOperatorSemantics): n/a
  - `<unnamed>` (MergesOnTopOfLegacyStringBase): n/a

#### `TEST_F(SequenceCounterTest, ClearResetsSequenceToZero)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:345
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequenceCounterTest): n/a
  - `<unnamed>` (ClearResetsSequenceToZero): n/a

#### `TEST_F(SequenceCounterTest, GetLatestSequenceReflectsCounter)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequenceCounterTest): n/a
  - `<unnamed>` (GetLatestSequenceReflectsCounter): n/a

#### `TEST_F(SequenceCounterTest, NoDuplicateSequencesUnder8Threads)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequenceCounterTest): n/a
  - `<unnamed>` (NoDuplicateSequencesUnder8Threads): n/a

#### `TEST_F(SequenceCounterTest, SingleThreadMonotonicSequences)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequenceCounterTest): n/a
  - `<unnamed>` (SingleThreadMonotonicSequences): n/a

#### `TEST_F(SequenceCounterTest, ThroughputAtLeast50KPerSecUnder8Threads)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (SequenceCounterTest): n/a
  - `<unnamed>` (ThroughputAtLeast50KPerSecUnder8Threads): n/a

#### `rocksdb::TransactionDB * openDB(const std::string &path, bool with_merge_operator=false)`
- Source: `tests/cdc/test_cdc_changefeed_sequence_counter.cpp`:31
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a
  - `with_merge_operator` (bool): n/a

### test_cdc_consumer_group.cpp

#### `TEST(ConsumerGroupConfigTest, JsonRoundTrip)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:666
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupConfigTest): n/a
  - `<unnamed>` (JsonRoundTrip): n/a

#### `TEST(ConsumerGroupInfoTest, JsonContainsExpectedFields)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:678
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupInfoTest): n/a
  - `<unnamed>` (JsonContainsExpectedFields): n/a

#### `TEST(ConsumerGroupStaticTest, FNV1a32DifferentInputs)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupStaticTest): n/a
  - `<unnamed>` (FNV1a32DifferentInputs): n/a

#### `TEST(ConsumerGroupStaticTest, FNV1a32IsStable)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupStaticTest): n/a
  - `<unnamed>` (FNV1a32IsStable): n/a

#### `TEST(ConsumerGroupStaticTest, PartitionCountOneAlwaysZero)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupStaticTest): n/a
  - `<unnamed>` (PartitionCountOneAlwaysZero): n/a

#### `TEST(ConsumerGroupStaticTest, PartitionCountZeroSafe)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupStaticTest): n/a
  - `<unnamed>` (PartitionCountZeroSafe): n/a

#### `TEST(ConsumerGroupStaticTest, PartitionDistribution)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupStaticTest): n/a
  - `<unnamed>` (PartitionDistribution): n/a

#### `TEST(ConsumerGroupStaticTest, PartitionForKeyInRange)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupStaticTest): n/a
  - `<unnamed>` (PartitionForKeyInRange): n/a

#### `TEST_F(ConsumerGroupTest, AtLeastOnce_AcknowledgeClearsInFlightAndAdvancesOffset)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:461
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (AtLeastOnce_AcknowledgeClearsInFlightAndAdvancesOffset): n/a

#### `TEST_F(ConsumerGroupTest, AtLeastOnce_AcknowledgeDoesNotGoBackward)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:641
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (AtLeastOnce_AcknowledgeDoesNotGoBackward): n/a

#### `TEST_F(ConsumerGroupTest, AtLeastOnce_AcknowledgePartialBatch)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:480
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (AtLeastOnce_AcknowledgePartialBatch): n/a

#### `TEST_F(ConsumerGroupTest, AtLeastOnce_AcknowledgedEventsNotRedelivered)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:554
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (AtLeastOnce_AcknowledgedEventsNotRedelivered): n/a

#### `TEST_F(ConsumerGroupTest, AtLeastOnce_DeleteGroupClearsInflightState)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:624
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (AtLeastOnce_DeleteGroupClearsInflightState): n/a

#### `TEST_F(ConsumerGroupTest, AtLeastOnce_EmptyConsumerIdThrows)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:610
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (AtLeastOnce_EmptyConsumerIdThrows): n/a

#### `TEST_F(ConsumerGroupTest, AtLeastOnce_GetInFlightStats)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:580
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (AtLeastOnce_GetInFlightStats): n/a

#### `TEST_F(ConsumerGroupTest, AtLeastOnce_InitialFetchTracksInFlight)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:444
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (AtLeastOnce_InitialFetchTracksInFlight): n/a

#### `TEST_F(ConsumerGroupTest, AtLeastOnce_NoRedeliveryBeforeTimeout)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:535
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (AtLeastOnce_NoRedeliveryBeforeTimeout): n/a

#### `TEST_F(ConsumerGroupTest, AtLeastOnce_NonExistentGroupThrows)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:601
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (AtLeastOnce_NonExistentGroupThrows): n/a

#### `TEST_F(ConsumerGroupTest, AtLeastOnce_RedeliveryOnTimeout)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:501
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (AtLeastOnce_RedeliveryOnTimeout): n/a

#### `TEST_F(ConsumerGroupTest, CommitOffsetAdvances)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (CommitOffsetAdvances): n/a

#### `TEST_F(ConsumerGroupTest, CommitOffsetDoesNotGoBackward)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (CommitOffsetDoesNotGoBackward): n/a

#### `TEST_F(ConsumerGroupTest, CommitOffsetNonExistentGroupThrows)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (CommitOffsetNonExistentGroupThrows): n/a

#### `TEST_F(ConsumerGroupTest, ConsumerHandlesKeyConsistency)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:314
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (ConsumerHandlesKeyConsistency): n/a

#### `TEST_F(ConsumerGroupTest, ConsumerPartitionInRange)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (ConsumerPartitionInRange): n/a

#### `TEST_F(ConsumerGroupTest, ConsumerPartitionIsStable)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:303
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (ConsumerPartitionIsStable): n/a

#### `TEST_F(ConsumerGroupTest, CreateGroupAndExists)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (CreateGroupAndExists): n/a

#### `TEST_F(ConsumerGroupTest, CreateGroupInvalidArguments)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (CreateGroupInvalidArguments): n/a

#### `TEST_F(ConsumerGroupTest, DeleteGroup)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (DeleteGroup): n/a

#### `TEST_F(ConsumerGroupTest, DeleteNonExistentGroupIsIdempotent)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (DeleteNonExistentGroupIsIdempotent): n/a

#### `TEST_F(ConsumerGroupTest, FetchEventsAllConsumersReceiveAllEvents)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (FetchEventsAllConsumersReceiveAllEvents): n/a

#### `TEST_F(ConsumerGroupTest, FetchEventsEmptyChangefeed)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:424
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (FetchEventsEmptyChangefeed): n/a

#### `TEST_F(ConsumerGroupTest, FetchEventsNonExistentGroupThrows)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:434
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (FetchEventsNonExistentGroupThrows): n/a

#### `TEST_F(ConsumerGroupTest, FetchEventsRespectsCommittedOffset)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:396
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (FetchEventsRespectsCommittedOffset): n/a

#### `TEST_F(ConsumerGroupTest, FetchEventsReturnsOnlyConsumersPartition)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:352
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (FetchEventsReturnsOnlyConsumersPartition): n/a

#### `TEST_F(ConsumerGroupTest, GetGroupConfigRoundTrip)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (GetGroupConfigRoundTrip): n/a

#### `TEST_F(ConsumerGroupTest, GetOffsetNonExistentGroupThrows)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (GetOffsetNonExistentGroupThrows): n/a

#### `TEST_F(ConsumerGroupTest, GetPartitionForKeyIsConsistent)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:335
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (GetPartitionForKeyIsConsistent): n/a

#### `TEST_F(ConsumerGroupTest, GroupInfoReturnsConfigAndOffset)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (GroupInfoReturnsConfigAndOffset): n/a

#### `TEST_F(ConsumerGroupTest, InitialOffsetIsZero)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (InitialOffsetIsZero): n/a

#### `TEST_F(ConsumerGroupTest, ListGroups)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (ListGroups): n/a

#### `TEST_F(ConsumerGroupTest, UpdateGroupPreservesOffset)`
- Source: `tests/cdc/test_cdc_consumer_group.cpp`:203
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupTest): n/a
  - `<unnamed>` (UpdateGroupPreservesOffset): n/a

### test_cdc_cross_collection_stream.cpp

#### `TEST_F(CrossCollectionStreamTest, AddAndQueryCollections)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (AddAndQueryCollections): n/a

#### `TEST_F(CrossCollectionStreamTest, AddCollectionRejectsEmptyName)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (AddCollectionRejectsEmptyName): n/a

#### `TEST_F(CrossCollectionStreamTest, AddCollectionRejectsNullFeed)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (AddCollectionRejectsNullFeed): n/a

#### `TEST_F(CrossCollectionStreamTest, AggregatedEventToJsonIncludesCollection)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (AggregatedEventToJsonIncludesCollection): n/a

#### `TEST_F(CrossCollectionStreamTest, FilterByCollectionSubset)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (FilterByCollectionSubset): n/a

#### `TEST_F(CrossCollectionStreamTest, FilterByEventType)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (FilterByEventType): n/a

#### `TEST_F(CrossCollectionStreamTest, FilterByKeyPrefix)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (FilterByKeyPrefix): n/a

#### `TEST_F(CrossCollectionStreamTest, FromSequenceCursorResumesCorrectly)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (FromSequenceCursorResumesCorrectly): n/a

#### `TEST_F(CrossCollectionStreamTest, HighWatermarkAllCollections)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (HighWatermarkAllCollections): n/a

#### `TEST_F(CrossCollectionStreamTest, HighWatermarkSubsetOfCollections)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (HighWatermarkSubsetOfCollections): n/a

#### `TEST_F(CrossCollectionStreamTest, IndependentCursorsPerCollection)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (IndependentCursorsPerCollection): n/a

#### `TEST_F(CrossCollectionStreamTest, LimitCapsReturnedEvents)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (LimitCapsReturnedEvents): n/a

#### `TEST_F(CrossCollectionStreamTest, ListEventsAggregatedEventCarriesCollectionName)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (ListEventsAggregatedEventCarriesCollectionName): n/a

#### `TEST_F(CrossCollectionStreamTest, ListEventsEmptyStream)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:174
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (ListEventsEmptyStream): n/a

#### `TEST_F(CrossCollectionStreamTest, ListEventsForEmptySetReturnsAll)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (ListEventsForEmptySetReturnsAll): n/a

#### `TEST_F(CrossCollectionStreamTest, ListEventsFromSingleCollection)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (ListEventsFromSingleCollection): n/a

#### `TEST_F(CrossCollectionStreamTest, ListEventsMergedAndSortedByTimestamp)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (ListEventsMergedAndSortedByTimestamp): n/a

#### `TEST_F(CrossCollectionStreamTest, RemoveCollection)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (RemoveCollection): n/a

#### `TEST_F(CrossCollectionStreamTest, ReplaceExistingCollection)`
- Source: `tests/cdc/test_cdc_cross_collection_stream.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStreamTest): n/a
  - `<unnamed>` (ReplaceExistingCollection): n/a

### test_cdc_dead_letter_queue.cpp

#### `TEST_F(DLQBufferIntegrationTest, GetAndSetDeadLetterQueue)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:263
- Brief: n/a
- Parameters:
  - `<unnamed>` (DLQBufferIntegrationTest): n/a
  - `<unnamed>` (GetAndSetDeadLetterQueue): n/a

#### `TEST_F(DeadLetterQueueTest, DlqEntryRoundTripJson)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (DlqEntryRoundTripJson): n/a

#### `TEST_F(DeadLetterQueueTest, DrainOnEmptyQueueReturnsZero)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (DrainOnEmptyQueueReturnsZero): n/a

#### `TEST_F(DeadLetterQueueTest, DrainRemovesAllEntries)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (DrainRemovesAllEntries): n/a

#### `TEST_F(DeadLetterQueueTest, EnqueueAssignsDlqSequence)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (EnqueueAssignsDlqSequence): n/a

#### `TEST_F(DeadLetterQueueTest, EnqueueMultipleIncreasesSequence)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (EnqueueMultipleIncreasesSequence): n/a

#### `TEST_F(DeadLetterQueueTest, GetEntryRoundTrip)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (GetEntryRoundTrip): n/a

#### `TEST_F(DeadLetterQueueTest, GetEntryThrowsOnMissingSequence)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (GetEntryThrowsOnMissingSequence): n/a

#### `TEST_F(DeadLetterQueueTest, ListEntriesRespectsLimit)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (ListEntriesRespectsLimit): n/a

#### `TEST_F(DeadLetterQueueTest, ListEntriesReturnsAll)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (ListEntriesReturnsAll): n/a

#### `TEST_F(DeadLetterQueueTest, RemoveDecreasesSize)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (RemoveDecreasesSize): n/a

#### `TEST_F(DeadLetterQueueTest, RemoveReturnsFalseForMissing)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (RemoveReturnsFalseForMissing): n/a

#### `TEST_F(DeadLetterQueueTest, ReplayRerecordsEventAndRemovesEntry)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (ReplayRerecordsEventAndRemovesEntry): n/a

#### `TEST_F(DeadLetterQueueTest, ReplayThrowsOnMissingEntry)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (ReplayThrowsOnMissingEntry): n/a

#### `TEST_F(DeadLetterQueueTest, SizeReflectsEnqueueCount)`
- Source: `tests/cdc/test_cdc_dead_letter_queue.cpp`:99
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueueTest): n/a
  - `<unnamed>` (SizeReflectsEnqueueCount): n/a

### test_cdc_debezium_format.cpp

#### `TEST(DebeziumFormatterTest, CollectionDerivedFromKeyPrefix)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (CollectionDerivedFromKeyPrefix): n/a

#### `TEST(DebeziumFormatterTest, Create_NullBefore_AfterFromValue)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (Create_NullBefore_AfterFromValue): n/a

#### `TEST(DebeziumFormatterTest, Delete_BeforeFromSnapshot_AfterIsNull)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (Delete_BeforeFromSnapshot_AfterIsNull): n/a

#### `TEST(DebeziumFormatterTest, Delete_MapsToDelete)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (Delete_MapsToDelete): n/a

#### `TEST(DebeziumFormatterTest, ExplicitCollectionOverridesKeyPrefix)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (ExplicitCollectionOverridesKeyPrefix): n/a

#### `TEST(DebeziumFormatterTest, InvalidSnapshotJson_FallsBackToRawKey)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (InvalidSnapshotJson_FallsBackToRawKey): n/a

#### `TEST(DebeziumFormatterTest, KeyWithoutColonUsesFullKeyAsCollection)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (KeyWithoutColonUsesFullKeyAsCollection): n/a

#### `TEST(DebeziumFormatterTest, PayloadContainsNullTransactionField)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (PayloadContainsNullTransactionField): n/a

#### `TEST(DebeziumFormatterTest, PayloadOnly_NoSchemaKey)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:266
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (PayloadOnly_NoSchemaKey): n/a

#### `TEST(DebeziumFormatterTest, PutWithBefore_MapsToUpdate)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (PutWithBefore_MapsToUpdate): n/a

#### `TEST(DebeziumFormatterTest, PutWithoutBefore_MapsToCreate)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (PutWithoutBefore_MapsToCreate): n/a

#### `TEST(DebeziumFormatterTest, RedactedEvent_SnapshotFieldReflectsRedaction)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:328
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (RedactedEvent_SnapshotFieldReflectsRedaction): n/a

#### `TEST(DebeziumFormatterTest, SchemaBlock_ContainsEnvelopeName)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (SchemaBlock_ContainsEnvelopeName): n/a

#### `TEST(DebeziumFormatterTest, SchemaBlock_OpFieldPresent)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (SchemaBlock_OpFieldPresent): n/a

#### `TEST(DebeziumFormatterTest, SourceBlock_CustomConfig)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:222
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (SourceBlock_CustomConfig): n/a

#### `TEST(DebeziumFormatterTest, SourceBlock_DefaultConfig)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (SourceBlock_DefaultConfig): n/a

#### `TEST(DebeziumFormatterTest, SourceBlock_SerializesToJson)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (SourceBlock_SerializesToJson): n/a

#### `TEST(DebeziumFormatterTest, TimestampPropagatedToPayloadAndSource)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (TimestampPropagatedToPayloadAndSource): n/a

#### `TEST(DebeziumFormatterTest, TransactionCommit_MapsToRead)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (TransactionCommit_MapsToRead): n/a

#### `TEST(DebeziumFormatterTest, TransactionRollback_MapsToRead)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (TransactionRollback_MapsToRead): n/a

#### `TEST(DebeziumFormatterTest, Update_BeforeAndAfterFromSnapshots)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (Update_BeforeAndAfterFromSnapshots): n/a

#### `TEST(DebeziumFormatterTest, WithSchema_ContainsSchemaAndPayload)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumFormatterTest): n/a
  - `<unnamed>` (WithSchema_ContainsSchemaAndPayload): n/a

#### `TEST(DebeziumOpStringTest, AllOpCodes)`
- Source: `tests/cdc/test_cdc_debezium_format.cpp`:354
- Brief: n/a
- Parameters:
  - `<unnamed>` (DebeziumOpStringTest): n/a
  - `<unnamed>` (AllOpCodes): n/a

### test_cdc_delivery_guarantee_config.cpp

#### `TEST(DeliveryGuaranteeConfigTest, ConcurrentModeUpdates)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryGuaranteeConfigTest): n/a
  - `<unnamed>` (ConcurrentModeUpdates): n/a

#### `TEST(DeliveryGuaranteeConfigTest, ConstructorOverridesDefaults)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryGuaranteeConfigTest): n/a
  - `<unnamed>` (ConstructorOverridesDefaults): n/a

#### `TEST(DeliveryGuaranteeConfigTest, DefaultAckTimeout)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryGuaranteeConfigTest): n/a
  - `<unnamed>` (DefaultAckTimeout): n/a

#### `TEST(DeliveryGuaranteeConfigTest, DefaultDeduplicationWindow)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryGuaranteeConfigTest): n/a
  - `<unnamed>` (DefaultDeduplicationWindow): n/a

#### `TEST(DeliveryGuaranteeConfigTest, DefaultModeIsAtLeastOnce)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryGuaranteeConfigTest): n/a
  - `<unnamed>` (DefaultModeIsAtLeastOnce): n/a

#### `TEST(DeliveryGuaranteeConfigTest, PolymorphicUsage)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryGuaranteeConfigTest): n/a
  - `<unnamed>` (PolymorphicUsage): n/a

#### `TEST(DeliveryGuaranteeConfigTest, SetAckTimeout)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryGuaranteeConfigTest): n/a
  - `<unnamed>` (SetAckTimeout): n/a

#### `TEST(DeliveryGuaranteeConfigTest, SetAckTimeoutZero)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryGuaranteeConfigTest): n/a
  - `<unnamed>` (SetAckTimeoutZero): n/a

#### `TEST(DeliveryGuaranteeConfigTest, SetDeduplicationWindow)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryGuaranteeConfigTest): n/a
  - `<unnamed>` (SetDeduplicationWindow): n/a

#### `TEST(DeliveryGuaranteeConfigTest, SetModeExactlyOnce)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryGuaranteeConfigTest): n/a
  - `<unnamed>` (SetModeExactlyOnce): n/a

#### `TEST(DeliveryGuaranteeConfigTest, SwitchModeBackToAtLeastOnce)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryGuaranteeConfigTest): n/a
  - `<unnamed>` (SwitchModeBackToAtLeastOnce): n/a

#### `TEST(InMemoryIdempotentListenerTest, ConcurrentMarkAndCheck)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryIdempotentListenerTest): n/a
  - `<unnamed>` (ConcurrentMarkAndCheck): n/a

#### `TEST(InMemoryIdempotentListenerTest, FifoEvictionOnWindowFull)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryIdempotentListenerTest): n/a
  - `<unnamed>` (FifoEvictionOnWindowFull): n/a

#### `TEST(InMemoryIdempotentListenerTest, MarkProcessedIdempotent)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryIdempotentListenerTest): n/a
  - `<unnamed>` (MarkProcessedIdempotent): n/a

#### `TEST(InMemoryIdempotentListenerTest, MarkProcessedMakesDuplicate)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryIdempotentListenerTest): n/a
  - `<unnamed>` (MarkProcessedMakesDuplicate): n/a

#### `TEST(InMemoryIdempotentListenerTest, MultipleCollectionsAreIndependent)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryIdempotentListenerTest): n/a
  - `<unnamed>` (MultipleCollectionsAreIndependent): n/a

#### `TEST(InMemoryIdempotentListenerTest, NewSequenceIsNotDuplicate)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryIdempotentListenerTest): n/a
  - `<unnamed>` (NewSequenceIsNotDuplicate): n/a

#### `TEST(InMemoryIdempotentListenerTest, PolymorphicUsage)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryIdempotentListenerTest): n/a
  - `<unnamed>` (PolymorphicUsage): n/a

#### `TEST(InMemoryIdempotentListenerTest, ProcessedCountIncrements)`
- Source: `tests/cdc/test_cdc_delivery_guarantee_config.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryIdempotentListenerTest): n/a
  - `<unnamed>` (ProcessedCountIncrements): n/a

### test_cdc_delivery_tracker.cpp

#### `TEST(DeliveryTrackerTest, AckedEventsNotRedelivered)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (AckedEventsNotRedelivered): n/a

#### `TEST(DeliveryTrackerTest, AcknowledgeUnknownConsumer)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (AcknowledgeUnknownConsumer): n/a

#### `TEST(DeliveryTrackerTest, AcknowledgeUnknownSequence)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (AcknowledgeUnknownSequence): n/a

#### `TEST(DeliveryTrackerTest, AcknowledgeUpTo)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (AcknowledgeUpTo): n/a

#### `TEST(DeliveryTrackerTest, AcknowledgeUpToCoversAll)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (AcknowledgeUpToCoversAll): n/a

#### `TEST(DeliveryTrackerTest, AcknowledgeUpToUnknownConsumer)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (AcknowledgeUpToUnknownConsumer): n/a

#### `TEST(DeliveryTrackerTest, ConcurrentDeliveryAndAck)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:316
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (ConcurrentDeliveryAndAck): n/a

#### `TEST(DeliveryTrackerTest, EmptyDeliveryAlwaysSucceeds)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (EmptyDeliveryAlwaysSucceeds): n/a

#### `TEST(DeliveryTrackerTest, ExpiredEventsRemovedAfterMaxAttempts)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (ExpiredEventsRemovedAfterMaxAttempts): n/a

#### `TEST(DeliveryTrackerTest, MultipleConsumers)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (MultipleConsumers): n/a

#### `TEST(DeliveryTrackerTest, NoPendingRedeliveryBeforeTimeout)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (NoPendingRedeliveryBeforeTimeout): n/a

#### `TEST(DeliveryTrackerTest, PendingLimitRejection)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (PendingLimitRejection): n/a

#### `TEST(DeliveryTrackerTest, PendingRedeliveryAfterTimeout)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (PendingRedeliveryAfterTimeout): n/a

#### `TEST(DeliveryTrackerTest, PendingRedeliveryUnknownConsumer)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:233
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (PendingRedeliveryUnknownConsumer): n/a

#### `TEST(DeliveryTrackerTest, RedeliveryCallbackInvokedByBackgroundThread)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (RedeliveryCallbackInvokedByBackgroundThread): n/a

#### `TEST(DeliveryTrackerTest, RemoveConsumer)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (RemoveConsumer): n/a

#### `TEST(DeliveryTrackerTest, RemoveUnknownConsumerIsNoop)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (RemoveUnknownConsumerIsNoop): n/a

#### `TEST(DeliveryTrackerTest, StartStopIdempotent)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (StartStopIdempotent): n/a

#### `TEST(DeliveryTrackerTest, StatsAllConsumers)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (StatsAllConsumers): n/a

#### `TEST(DeliveryTrackerTest, StatsUnknownConsumer)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:307
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (StatsUnknownConsumer): n/a

#### `TEST(DeliveryTrackerTest, TrackAndAcknowledge)`
- Source: `tests/cdc/test_cdc_delivery_tracker.cpp`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeliveryTrackerTest): n/a
  - `<unnamed>` (TrackAndAcknowledge): n/a

### test_cdc_diagnostics_hardening.cpp

#### `TEST_F(DiagnosticsHardeningTest, DGH01_HealthCheckReturnsHealthyWithEvents)`
- Source: `tests/cdc/test_cdc_diagnostics_hardening.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsHardeningTest): n/a
  - `<unnamed>` (DGH01_HealthCheckReturnsHealthyWithEvents): n/a

#### `TEST_F(DiagnosticsHardeningTest, DGH02_DiagnosticsIncludesEventCountAndWatermarks)`
- Source: `tests/cdc/test_cdc_diagnostics_hardening.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsHardeningTest): n/a
  - `<unnamed>` (DGH02_DiagnosticsIncludesEventCountAndWatermarks): n/a

#### `TEST_F(DiagnosticsHardeningTest, DGH03_HealthCheckHandlesEmptyChangefeed)`
- Source: `tests/cdc/test_cdc_diagnostics_hardening.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsHardeningTest): n/a
  - `<unnamed>` (DGH03_HealthCheckHandlesEmptyChangefeed): n/a

#### `TEST_F(DiagnosticsHardeningTest, DGH04_LatencyHistogramPercentilesAreFinite)`
- Source: `tests/cdc/test_cdc_diagnostics_hardening.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsHardeningTest): n/a
  - `<unnamed>` (DGH04_LatencyHistogramPercentilesAreFinite): n/a

#### `TEST_F(DiagnosticsHardeningTest, DGH05_ThroughputTrackerIncrementsCounter)`
- Source: `tests/cdc/test_cdc_diagnostics_hardening.cpp`:170
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsHardeningTest): n/a
  - `<unnamed>` (DGH05_ThroughputTrackerIncrementsCounter): n/a

#### `TEST_F(DiagnosticsHardeningTest, DGH06_GetAllStatsReturnsAllConsumers)`
- Source: `tests/cdc/test_cdc_diagnostics_hardening.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsHardeningTest): n/a
  - `<unnamed>` (DGH06_GetAllStatsReturnsAllConsumers): n/a

#### `TEST_F(DiagnosticsHardeningTest, DGH07_DiagnosticsSerializesToValidJson)`
- Source: `tests/cdc/test_cdc_diagnostics_hardening.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsHardeningTest): n/a
  - `<unnamed>` (DGH07_DiagnosticsSerializesToValidJson): n/a

#### `TEST_F(DiagnosticsHardeningTest, DGH08_HealthStatusSerializesToJson)`
- Source: `tests/cdc/test_cdc_diagnostics_hardening.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiagnosticsHardeningTest): n/a
  - `<unnamed>` (DGH08_HealthStatusSerializesToJson): n/a

### test_cdc_error_codes.cpp

#### `TEST(CDCErrorHelpersTest, BufferOverflow)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorHelpersTest): n/a
  - `<unnamed>` (BufferOverflow): n/a

#### `TEST(CDCErrorHelpersTest, CompressionFailed)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorHelpersTest): n/a
  - `<unnamed>` (CompressionFailed): n/a

#### `TEST(CDCErrorHelpersTest, DBOperationFailed)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorHelpersTest): n/a
  - `<unnamed>` (DBOperationFailed): n/a

#### `TEST(CDCErrorHelpersTest, DecompressionFailed)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorHelpersTest): n/a
  - `<unnamed>` (DecompressionFailed): n/a

#### `TEST(CDCErrorHelpersTest, EventRecordFailed)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorHelpersTest): n/a
  - `<unnamed>` (EventRecordFailed): n/a

#### `TEST(CDCErrorHelpersTest, InvalidArgument)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorHelpersTest): n/a
  - `<unnamed>` (InvalidArgument): n/a

#### `TEST(CDCErrorHelpersTest, RateLimitExceeded)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorHelpersTest): n/a
  - `<unnamed>` (RateLimitExceeded): n/a

#### `TEST(CDCErrorHelpersTest, RetryExhausted)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorHelpersTest): n/a
  - `<unnamed>` (RetryExhausted): n/a

#### `TEST(CDCErrorHelpersTest, SequenceGenerationFailed)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorHelpersTest): n/a
  - `<unnamed>` (SequenceGenerationFailed): n/a

#### `TEST(CDCErrorTest, ErrorCodeRanges)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorTest): n/a
  - `<unnamed>` (ErrorCodeRanges): n/a

#### `TEST(CDCErrorTest, ErrorCodeValues)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:12
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorTest): n/a
  - `<unnamed>` (ErrorCodeValues): n/a

#### `TEST(CDCErrorTest, ErrorSeverityLevels)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:23
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorTest): n/a
  - `<unnamed>` (ErrorSeverityLevels): n/a

#### `TEST(CDCErrorTest, MultipleErrors)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCErrorTest): n/a
  - `<unnamed>` (MultipleErrors): n/a

#### `TEST(CDCExceptionTest, BasicConstruction)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCExceptionTest): n/a
  - `<unnamed>` (BasicConstruction): n/a

#### `TEST(CDCExceptionTest, CatchAsStdException)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCExceptionTest): n/a
  - `<unnamed>` (CatchAsStdException): n/a

#### `TEST(CDCExceptionTest, CodeValue)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCExceptionTest): n/a
  - `<unnamed>` (CodeValue): n/a

#### `TEST(CDCExceptionTest, IsDataLossRisk)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCExceptionTest): n/a
  - `<unnamed>` (IsDataLossRisk): n/a

#### `TEST(CDCExceptionTest, IsRetryable)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCExceptionTest): n/a
  - `<unnamed>` (IsRetryable): n/a

#### `TEST(CDCExceptionTest, JsonSerialization)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCExceptionTest): n/a
  - `<unnamed>` (JsonSerialization): n/a

#### `TEST(CDCExceptionTest, ThrowAndCatch)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCExceptionTest): n/a
  - `<unnamed>` (ThrowAndCatch): n/a

#### `TEST(CDCExceptionTest, WhatMessage)`
- Source: `tests/cdc/test_cdc_error_codes.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCExceptionTest): n/a
  - `<unnamed>` (WhatMessage): n/a

### test_cdc_event_enrichment.cpp

#### `TEST_F(CDCEventEnrichmentTest, DeleteEvent_HasBeforeSnapshot_NoAfterSnapshot)`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCEventEnrichmentTest): n/a
  - `<unnamed>` (DeleteEvent_HasBeforeSnapshot_NoAfterSnapshot): n/a

#### `TEST_F(CDCEventEnrichmentTest, FromJson_RoundTrip_WithSnapshots)`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCEventEnrichmentTest): n/a
  - `<unnamed>` (FromJson_RoundTrip_WithSnapshots): n/a

#### `TEST_F(CDCEventEnrichmentTest, FromJson_RoundTrip_WithoutSnapshots)`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCEventEnrichmentTest): n/a
  - `<unnamed>` (FromJson_RoundTrip_WithoutSnapshots): n/a

#### `TEST_F(CDCEventEnrichmentTest, InsertEvent_NoBeforeSnapshot_HasAfterSnapshot)`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCEventEnrichmentTest): n/a
  - `<unnamed>` (InsertEvent_NoBeforeSnapshot_HasAfterSnapshot): n/a

#### `TEST_F(CDCEventEnrichmentTest, PersistAndRetrieve_DeleteSnapshotPreserved)`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCEventEnrichmentTest): n/a
  - `<unnamed>` (PersistAndRetrieve_DeleteSnapshotPreserved): n/a

#### `TEST_F(CDCEventEnrichmentTest, PersistAndRetrieve_SnapshotsPreserved)`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCEventEnrichmentTest): n/a
  - `<unnamed>` (PersistAndRetrieve_SnapshotsPreserved): n/a

#### `TEST_F(CDCEventEnrichmentTest, ToJson_ContainsSnapshotFields)`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCEventEnrichmentTest): n/a
  - `<unnamed>` (ToJson_ContainsSnapshotFields): n/a

#### `TEST_F(CDCEventEnrichmentTest, ToJson_OmitsAbsentSnapshotFields)`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCEventEnrichmentTest): n/a
  - `<unnamed>` (ToJson_OmitsAbsentSnapshotFields): n/a

#### `TEST_F(CDCEventEnrichmentTest, UpdateEvent_HasBothSnapshots)`
- Source: `tests/cdc/test_cdc_event_enrichment.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCEventEnrichmentTest): n/a
  - `<unnamed>` (UpdateEvent_HasBothSnapshots): n/a

### test_cdc_event_schema.cpp

#### `TEST(InMemoryCDCEventSchemaTest, ConcurrentRegisterAndGet)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:234
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (ConcurrentRegisterAndGet): n/a

#### `TEST(InMemoryCDCEventSchemaTest, CurrentVersionBeforeRegistrationIsMinusOne)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (CurrentVersionBeforeRegistrationIsMinusOne): n/a

#### `TEST(InMemoryCDCEventSchemaTest, FirstConflictPassedToCallback)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (FirstConflictPassedToCallback): n/a

#### `TEST(InMemoryCDCEventSchemaTest, GetLatestSchema)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (GetLatestSchema): n/a

#### `TEST(InMemoryCDCEventSchemaTest, GetSchemaUnknownCollectionReturnsEmpty)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (GetSchemaUnknownCollectionReturnsEmpty): n/a

#### `TEST(InMemoryCDCEventSchemaTest, GetSchemaUnknownVersionReturnsEmpty)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (GetSchemaUnknownVersionReturnsEmpty): n/a

#### `TEST(InMemoryCDCEventSchemaTest, IndependentCollections)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (IndependentCollections): n/a

#### `TEST(InMemoryCDCEventSchemaTest, MultipleFormats)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (MultipleFormats): n/a

#### `TEST(InMemoryCDCEventSchemaTest, PolymorphicUsage)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (PolymorphicUsage): n/a

#### `TEST(InMemoryCDCEventSchemaTest, RegisterAndRetrieve)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (RegisterAndRetrieve): n/a

#### `TEST(InMemoryCDCEventSchemaTest, RegisterDuplicateVersionReturnsFalse)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (RegisterDuplicateVersionReturnsFalse): n/a

#### `TEST(InMemoryCDCEventSchemaTest, ReplaceCallback)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (ReplaceCallback): n/a

#### `TEST(InMemoryCDCEventSchemaTest, TriggerCompatibleEvolution)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (TriggerCompatibleEvolution): n/a

#### `TEST(InMemoryCDCEventSchemaTest, TriggerIncompatibleEvolution)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (TriggerIncompatibleEvolution): n/a

#### `TEST(InMemoryCDCEventSchemaTest, TriggerWithoutCallbackReturnsFalse)`
- Source: `tests/cdc/test_cdc_event_schema.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryCDCEventSchemaTest): n/a
  - `<unnamed>` (TriggerWithoutCallbackReturnsFalse): n/a

### test_cdc_fan_in.cpp

#### `TEST_F(FanInTest, AddDuplicateSourceReturnsFalse)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (AddDuplicateSourceReturnsFalse): n/a

#### `TEST_F(FanInTest, AddSourceSucceeds)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (AddSourceSucceeds): n/a

#### `TEST_F(FanInTest, CollectionSubsetFilter)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (CollectionSubsetFilter): n/a

#### `TEST_F(FanInTest, CustomMergePolicyReverseOrder)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:300
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (CustomMergePolicyReverseOrder): n/a

#### `TEST_F(FanInTest, FanInEventToJsonIncludesCollection)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (FanInEventToJsonIncludesCollection): n/a

#### `TEST_F(FanInTest, FromSequenceCursor)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (FromSequenceCursor): n/a

#### `TEST_F(FanInTest, LimitEnforced)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (LimitEnforced): n/a

#### `TEST_F(FanInTest, ListEventsFromSingleSource)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (ListEventsFromSingleSource): n/a

#### `TEST_F(FanInTest, ListEventsMergesTwoSources)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (ListEventsMergesTwoSources): n/a

#### `TEST_F(FanInTest, ListEventsNoSourcesReturnsEmpty)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (ListEventsNoSourcesReturnsEmpty): n/a

#### `TEST_F(FanInTest, ListEventsOrderedByTimestamp)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (ListEventsOrderedByTimestamp): n/a

#### `TEST_F(FanInTest, PolymorphicUsage)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (PolymorphicUsage): n/a

#### `TEST_F(FanInTest, RemoveSourceSucceeds)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (RemoveSourceSucceeds): n/a

#### `TEST_F(FanInTest, RemoveUnknownSourceReturnsFalse)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (RemoveUnknownSourceReturnsFalse): n/a

#### `TEST_F(FanInTest, SourceIds_MultipleCollections)`
- Source: `tests/cdc/test_cdc_fan_in.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (FanInTest): n/a
  - `<unnamed>` (SourceIds_MultipleCollections): n/a

### test_cdc_filter_pipeline.cpp

#### `TEST(InMemoryFilterPipelineTest, AddFilterReturnsFalseForDuplicate)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (AddFilterReturnsFalseForDuplicate): n/a

#### `TEST(InMemoryFilterPipelineTest, ApplyBatchReturnOnlyPassingEventsInOrder)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (ApplyBatchReturnOnlyPassingEventsInOrder): n/a

#### `TEST(InMemoryFilterPipelineTest, ConcurrentAddFilterAndApplyAreThreadSafe)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (ConcurrentAddFilterAndApplyAreThreadSafe): n/a

#### `TEST(InMemoryFilterPipelineTest, EmptyEventTypeListPassesAll)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (EmptyEventTypeListPassesAll): n/a

#### `TEST(InMemoryFilterPipelineTest, EmptyKeyPrefixPassesAll)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (EmptyKeyPrefixPassesAll): n/a

#### `TEST(InMemoryFilterPipelineTest, EmptyPipelinePassesAll)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (EmptyPipelinePassesAll): n/a

#### `TEST(InMemoryFilterPipelineTest, EventTypeFilterPassesMatchingTypes)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (EventTypeFilterPassesMatchingTypes): n/a

#### `TEST(InMemoryFilterPipelineTest, FilterNamesInInsertionOrder)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (FilterNamesInInsertionOrder): n/a

#### `TEST(InMemoryFilterPipelineTest, FirstDropShortCircuitsRemainingStages)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (FirstDropShortCircuitsRemainingStages): n/a

#### `TEST(InMemoryFilterPipelineTest, HasFilterReportsPresence)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (HasFilterReportsPresence): n/a

#### `TEST(InMemoryFilterPipelineTest, KeyPrefixFilterPassesMatchingPrefix)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (KeyPrefixFilterPassesMatchingPrefix): n/a

#### `TEST(InMemoryFilterPipelineTest, PassDropCountersAndReset)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (PassDropCountersAndReset): n/a

#### `TEST(InMemoryFilterPipelineTest, PolymorphicUsage)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (PolymorphicUsage): n/a

#### `TEST(InMemoryFilterPipelineTest, PredicateFilterPassDrop)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (PredicateFilterPassDrop): n/a

#### `TEST(InMemoryFilterPipelineTest, RemoveFilterReturnsFalseForUnknown)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (RemoveFilterReturnsFalseForUnknown): n/a

#### `TEST(InMemoryFilterPipelineTest, RemoveStageAffectsApply)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (RemoveStageAffectsApply): n/a

#### `TEST(InMemoryFilterPipelineTest, SizeAndEmpty)`
- Source: `tests/cdc/test_cdc_filter_pipeline.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryFilterPipelineTest): n/a
  - `<unnamed>` (SizeAndEmpty): n/a

### test_cdc_gdpr_redaction.cpp

#### `TEST_F(CDCGDPRRedactionTest, AdminRedactByKeyPrefixJsonSerialization)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (AdminRedactByKeyPrefixJsonSerialization): n/a

#### `TEST_F(CDCGDPRRedactionTest, AdminRedactByKeyPrefixReturnsCorrectCounts)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (AdminRedactByKeyPrefixReturnsCorrectCounts): n/a

#### `TEST_F(CDCGDPRRedactionTest, AdminRedactEmptyKeyPrefixThrows)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (AdminRedactEmptyKeyPrefixThrows): n/a

#### `TEST_F(CDCGDPRRedactionTest, AdminRedactWithDefaultOperatorId)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (AdminRedactWithDefaultOperatorId): n/a

#### `TEST_F(CDCGDPRRedactionTest, AuditLogNotWrittenWhenStorageNotSet)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:295
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (AuditLogNotWrittenWhenStorageNotSet): n/a

#### `TEST_F(CDCGDPRRedactionTest, AuditLogWrittenToCdcRedactionsCF)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (AuditLogWrittenToCdcRedactionsCF): n/a

#### `TEST_F(CDCGDPRRedactionTest, DeleteEventsAreAlsoRedacted)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (DeleteEventsAreAlsoRedacted): n/a

#### `TEST_F(CDCGDPRRedactionTest, EmptyChangelogReturnsZeroCounts)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (EmptyChangelogReturnsZeroCounts): n/a

#### `TEST_F(CDCGDPRRedactionTest, EmptyKeyPrefixThrows)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (EmptyKeyPrefixThrows): n/a

#### `TEST_F(CDCGDPRRedactionTest, NoTombstonesPublishedWhenNoEventsMatched)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:361
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (NoTombstonesPublishedWhenNoEventsMatched): n/a

#### `TEST_F(CDCGDPRRedactionTest, NoTombstonesPublishedWhenTransportNotSet)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:355
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (NoTombstonesPublishedWhenTransportNotSet): n/a

#### `TEST_F(CDCGDPRRedactionTest, PreservesAuditFields)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (PreservesAuditFields): n/a

#### `TEST_F(CDCGDPRRedactionTest, RedactedEventsNotReturnedWithOriginalValue)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (RedactedEventsNotReturnedWithOriginalValue): n/a

#### `TEST_F(CDCGDPRRedactionTest, RedactsMatchingEventValues)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (RedactsMatchingEventValues): n/a

#### `TEST_F(CDCGDPRRedactionTest, SkipsAlreadyRedactedEvents)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (SkipsAlreadyRedactedEvents): n/a

#### `TEST_F(CDCGDPRRedactionTest, SkipsNonMatchingKeys)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (SkipsNonMatchingKeys): n/a

#### `TEST_F(CDCGDPRRedactionTest, TombstonesDeduplicatedForMultipleEventsWithSameKey)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:340
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (TombstonesDeduplicatedForMultipleEventsWithSameKey): n/a

#### `TEST_F(CDCGDPRRedactionTest, TombstonesPublishedForEachAffectedKey)`
- Source: `tests/cdc/test_cdc_gdpr_redaction.cpp`:319
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCGDPRRedactionTest): n/a
  - `<unnamed>` (TombstonesPublishedForEachAffectedKey): n/a

### test_cdc_highcardinality_stress.cpp

#### `TEST(WaveD_CDCStress, AcknowledgementTimeoutStress)`
- Source: `tests/cdc/test_cdc_highcardinality_stress.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_CDCStress): n/a
  - `<unnamed>` (AcknowledgementTimeoutStress): n/a

#### `TEST(WaveD_CDCStress, ConcurrentReplayStress)`
- Source: `tests/cdc/test_cdc_highcardinality_stress.cpp`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_CDCStress): n/a
  - `<unnamed>` (ConcurrentReplayStress): n/a

#### `TEST(WaveD_CDCStress, HighCardinalityEventStream)`
- Source: `tests/cdc/test_cdc_highcardinality_stress.cpp`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_CDCStress): n/a
  - `<unnamed>` (HighCardinalityEventStream): n/a

### test_cdc_kafka_producer.cpp

#### `TEST(KafkaCDCProducerTest, ConfigPerCollectionTopicPrefix)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (ConfigPerCollectionTopicPrefix): n/a

#### `TEST(KafkaCDCProducerTest, ConfigSingleTopicOverride)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (ConfigSingleTopicOverride): n/a

#### `TEST(KafkaCDCProducerTest, DebeziumConfigCanBeCustomized)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:193
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (DebeziumConfigCanBeCustomized): n/a

#### `TEST(KafkaCDCProducerTest, DebeziumConfigDefaultServerName)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (DebeziumConfigDefaultServerName): n/a

#### `TEST(KafkaCDCProducerTest, DebeziumFormatFlagCanBeEnabled)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (DebeziumFormatFlagCanBeEnabled): n/a

#### `TEST(KafkaCDCProducerTest, DebeziumFormatterProducesValidEnvelope)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (DebeziumFormatterProducesValidEnvelope): n/a

#### `TEST(KafkaCDCProducerTest, DefaultConfigValues)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:26
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (DefaultConfigValues): n/a

#### `TEST(KafkaCDCProducerTest, DefaultStatsValues)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (DefaultStatsValues): n/a

#### `TEST(KafkaCDCProducerTest, GetStatsBeforeStart)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (GetStatsBeforeStart): n/a

#### `TEST(KafkaCDCProducerTest, ImplementsICDCTransport)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (ImplementsICDCTransport): n/a

#### `TEST(KafkaCDCProducerTest, MetricsHasKafkaCounters)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (MetricsHasKafkaCounters): n/a

#### `TEST(KafkaCDCProducerTest, MetricsToJsonContainsKafkaCounters)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (MetricsToJsonContainsKafkaCounters): n/a

#### `TEST(KafkaCDCProducerTest, PolymorphicOwnershipViaUniquePtr)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (PolymorphicOwnershipViaUniquePtr): n/a

#### `TEST(KafkaCDCProducerTest, StubPublishReturnsFalse)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (StubPublishReturnsFalse): n/a

#### `TEST(KafkaCDCProducerTest, StubStartReturnsFalse)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (StubStartReturnsFalse): n/a

#### `TEST(KafkaCDCProducerTest, StubStopIsNoOp)`
- Source: `tests/cdc/test_cdc_kafka_producer.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (KafkaCDCProducerTest): n/a
  - `<unnamed>` (StubStopIsNoOp): n/a

### test_cdc_materialized_view.cpp

#### `TEST(CDCMaterializedViewMaintainer, ApplyEventsBatch)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:323
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (ApplyEventsBatch): n/a

#### `TEST(CDCMaterializedViewMaintainer, CollectionExtractedFromKeyPrefix)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (CollectionExtractedFromKeyPrefix): n/a

#### `TEST(CDCMaterializedViewMaintainer, CreateAndHasView)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (CreateAndHasView): n/a

#### `TEST(CDCMaterializedViewMaintainer, DeleteEventRollsBackAggregation)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (DeleteEventRollsBackAggregation): n/a

#### `TEST(CDCMaterializedViewMaintainer, DropView)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (DropView): n/a

#### `TEST(CDCMaterializedViewMaintainer, DuplicateCreateReturnsFalse)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (DuplicateCreateReturnsFalse): n/a

#### `TEST(CDCMaterializedViewMaintainer, GetView)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:156
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (GetView): n/a

#### `TEST(CDCMaterializedViewMaintainer, InsertEventCountsProcessed)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (InsertEventCountsProcessed): n/a

#### `TEST(CDCMaterializedViewMaintainer, InsertEventUpdatesView)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (InsertEventUpdatesView): n/a

#### `TEST(CDCMaterializedViewMaintainer, InvalidJsonSnapshotHandledGracefully)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:411
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (InvalidJsonSnapshotHandledGracefully): n/a

#### `TEST(CDCMaterializedViewMaintainer, KeyWithoutColonUsedAsCollection)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (KeyWithoutColonUsedAsCollection): n/a

#### `TEST(CDCMaterializedViewMaintainer, ListViews)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (ListViews): n/a

#### `TEST(CDCMaterializedViewMaintainer, MultipleViewsDispatched)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (MultipleViewsDispatched): n/a

#### `TEST(CDCMaterializedViewMaintainer, QueryLimitOffset)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:378
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (QueryLimitOffset): n/a

#### `TEST(CDCMaterializedViewMaintainer, QueryUnknownViewReturnsEmpty)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:400
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (QueryUnknownViewReturnsEmpty): n/a

#### `TEST(CDCMaterializedViewMaintainer, TransactionEventsAreSkipped)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (TransactionEventsAreSkipped): n/a

#### `TEST(CDCMaterializedViewMaintainer, UpdateEventAppliesDelta)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (UpdateEventAppliesDelta): n/a

#### `TEST(CDCMaterializedViewMaintainer, ValueFallbackForInsert)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMaterializedViewMaintainer): n/a
  - `<unnamed>` (ValueFallbackForInsert): n/a

#### `FieldValue findValue(const ViewQueryResult &result, const std::string &dim_val, const std::string &agg_name)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:105
- Brief: n/a
- Parameters:
  - `result` (const ViewQueryResult &): n/a
  - `dim_val` (const std::string &): n/a
  - `agg_name` (const std::string &): n/a
- Details: Find the aggregated value for a given dimension key in the result.

#### `Changefeed::ChangeEvent makeDelete(const std::string &key, const std::string &before_snapshot)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:58
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `before_snapshot` (const std::string &): n/a
- Details: Build a DELETE event.

#### `Changefeed::ChangeEvent makePut(const std::string &key, const std::string &after_snapshot, std::optional< std::string > before_snapshot=std::nullopt)`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:41
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `after_snapshot` (const std::string &): n/a
  - `before_snapshot` (std::optional< std::string >): n/a
- Details: Build a PUT event (INSERT when no before_snapshot).

#### `Changefeed::ChangeEvent makeTxCommit()`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:72
- Brief: n/a
- Parameters: none
- Details: Build a TRANSACTION_COMMIT event (should be skipped).

#### `Changefeed::ChangeEvent makeTxRollback()`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:82
- Brief: n/a
- Parameters: none
- Details: Build a TRANSACTION_ROLLBACK event (should be skipped).

#### `int64_t nowMs()`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:34
- Brief: n/a
- Parameters: none

#### `ViewDefinition salesViewDef(const std::string &name="sales_by_region")`
- Source: `tests/cdc/test_cdc_materialized_view.cpp`:92
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
- Details: Build a simple COUNT/SUM-by-region view on the "sales" collection.

### test_cdc_metrics.cpp

#### `TEST(CDCMetricsTest, AllMetricsPresent)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMetricsTest): n/a
  - `<unnamed>` (AllMetricsPresent): n/a

#### `TEST(CDCMetricsTest, Counters)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMetricsTest): n/a
  - `<unnamed>` (Counters): n/a

#### `TEST(CDCMetricsTest, HighThroughputScenario)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:334
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMetricsTest): n/a
  - `<unnamed>` (HighThroughputScenario): n/a

#### `TEST(CDCMetricsTest, RealWorldScenario)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMetricsTest): n/a
  - `<unnamed>` (RealWorldScenario): n/a

#### `TEST(CDCMetricsTest, Reset)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:225
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCMetricsTest): n/a
  - `<unnamed>` (Reset): n/a

#### `TEST(LatencyHistogramTest, Average)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:23
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyHistogramTest): n/a
  - `<unnamed>` (Average): n/a

#### `TEST(LatencyHistogramTest, BasicRecording)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:13
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyHistogramTest): n/a
  - `<unnamed>` (BasicRecording): n/a

#### `TEST(LatencyHistogramTest, EmptyHistogram)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyHistogramTest): n/a
  - `<unnamed>` (EmptyHistogram): n/a

#### `TEST(LatencyHistogramTest, HighLatencies)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyHistogramTest): n/a
  - `<unnamed>` (HighLatencies): n/a

#### `TEST(LatencyHistogramTest, JsonSerialization)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyHistogramTest): n/a
  - `<unnamed>` (JsonSerialization): n/a

#### `TEST(LatencyHistogramTest, Percentiles)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:33
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyHistogramTest): n/a
  - `<unnamed>` (Percentiles): n/a

#### `TEST(LatencyHistogramTest, Reset)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (LatencyHistogramTest): n/a
  - `<unnamed>` (Reset): n/a

#### `TEST(ScopedTimerTest, AutomaticRecording)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScopedTimerTest): n/a
  - `<unnamed>` (AutomaticRecording): n/a

#### `TEST(ScopedTimerTest, MultipleTimers)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScopedTimerTest): n/a
  - `<unnamed>` (MultipleTimers): n/a

#### `TEST(ThroughputTrackerTest, BasicRecording)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThroughputTrackerTest): n/a
  - `<unnamed>` (BasicRecording): n/a

#### `TEST(ThroughputTrackerTest, BytesPerSecond)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThroughputTrackerTest): n/a
  - `<unnamed>` (BytesPerSecond): n/a

#### `TEST(ThroughputTrackerTest, EventsPerSecond)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThroughputTrackerTest): n/a
  - `<unnamed>` (EventsPerSecond): n/a

#### `TEST(ThroughputTrackerTest, JsonSerialization)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThroughputTrackerTest): n/a
  - `<unnamed>` (JsonSerialization): n/a

#### `TEST(ThroughputTrackerTest, Reset)`
- Source: `tests/cdc/test_cdc_metrics.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThroughputTrackerTest): n/a
  - `<unnamed>` (Reset): n/a

### test_cdc_operation_filter.cpp

#### `TEST_F(CDCOperationFilterTest, CombineEventTypesWithKeyPrefix)`
- Source: `tests/cdc/test_cdc_operation_filter.cpp`:166
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCOperationFilterTest): n/a
  - `<unnamed>` (CombineEventTypesWithKeyPrefix): n/a

#### `TEST_F(CDCOperationFilterTest, EmptyEventTypesReturnsAll)`
- Source: `tests/cdc/test_cdc_operation_filter.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCOperationFilterTest): n/a
  - `<unnamed>` (EmptyEventTypesReturnsAll): n/a

#### `TEST_F(CDCOperationFilterTest, EventTypesPrecedenceOverLegacySingular)`
- Source: `tests/cdc/test_cdc_operation_filter.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCOperationFilterTest): n/a
  - `<unnamed>` (EventTypesPrecedenceOverLegacySingular): n/a

#### `TEST_F(CDCOperationFilterTest, FilterByDeleteOperationType)`
- Source: `tests/cdc/test_cdc_operation_filter.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCOperationFilterTest): n/a
  - `<unnamed>` (FilterByDeleteOperationType): n/a

#### `TEST_F(CDCOperationFilterTest, FilterByMultipleOperationTypes)`
- Source: `tests/cdc/test_cdc_operation_filter.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCOperationFilterTest): n/a
  - `<unnamed>` (FilterByMultipleOperationTypes): n/a

#### `TEST_F(CDCOperationFilterTest, FilterByPutOperationType)`
- Source: `tests/cdc/test_cdc_operation_filter.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCOperationFilterTest): n/a
  - `<unnamed>` (FilterByPutOperationType): n/a

#### `TEST_F(CDCOperationFilterTest, FilterReturnsEmptyWhenNoMatch)`
- Source: `tests/cdc/test_cdc_operation_filter.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCOperationFilterTest): n/a
  - `<unnamed>` (FilterReturnsEmptyWhenNoMatch): n/a

#### `TEST_F(CDCOperationFilterTest, LegacySingleEventTypeStillWorks)`
- Source: `tests/cdc/test_cdc_operation_filter.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCOperationFilterTest): n/a
  - `<unnamed>` (LegacySingleEventTypeStillWorks): n/a

### test_cdc_outbox.cpp

#### `TEST_F(OutboxTest, BackgroundRelayPublishesRecord)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:344
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (BackgroundRelayPublishesRecord): n/a

#### `TEST_F(OutboxTest, ListAllRecordsReturnsAllStates)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (ListAllRecordsReturnsAllStates): n/a

#### `TEST_F(OutboxTest, OutboxRecordJsonDeleteEventNullValue)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:392
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (OutboxRecordJsonDeleteEventNullValue): n/a

#### `TEST_F(OutboxTest, OutboxRecordJsonRoundTrip)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:363
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (OutboxRecordJsonRoundTrip): n/a

#### `TEST_F(OutboxTest, PurgePublishedRemovesAllPublished)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (PurgePublishedRemovesAllPublished): n/a

#### `TEST_F(OutboxTest, RelayOnceDeleteEventType)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (RelayOnceDeleteEventType): n/a

#### `TEST_F(OutboxTest, RelayOnceEmptyOutboxReturnsZero)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (RelayOnceEmptyOutboxReturnsZero): n/a

#### `TEST_F(OutboxTest, RelayOnceForwardsEventToChangefeed)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (RelayOnceForwardsEventToChangefeed): n/a

#### `TEST_F(OutboxTest, RelayOnceMultipleRecords)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (RelayOnceMultipleRecords): n/a

#### `TEST_F(OutboxTest, RelayOncePendingRecordBecomesPublished)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (RelayOncePendingRecordBecomesPublished): n/a

#### `TEST_F(OutboxTest, RemoveRecordDeletesEntry)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:276
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (RemoveRecordDeletesEntry): n/a

#### `TEST_F(OutboxTest, TotalRelayedAndFailedCounters)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (TotalRelayedAndFailedCounters): n/a

#### `TEST_F(OutboxTest, WriteToOutboxAssignsSequence)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (WriteToOutboxAssignsSequence): n/a

#### `TEST_F(OutboxTest, WriteToOutboxEmptyKeyThrows)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (WriteToOutboxEmptyKeyThrows): n/a

#### `TEST_F(OutboxTest, WriteToOutboxNullTxnThrows)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (WriteToOutboxNullTxnThrows): n/a

#### `TEST_F(OutboxTest, WriteToOutboxPersistsRecord)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (WriteToOutboxPersistsRecord): n/a

#### `TEST_F(OutboxTest, WriteToOutboxRollbackLeavesNoRecord)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (WriteToOutboxRollbackLeavesNoRecord): n/a

#### `TEST_F(OutboxTest, WriteToOutboxSequencesAreMonotonic)`
- Source: `tests/cdc/test_cdc_outbox.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (OutboxTest): n/a
  - `<unnamed>` (WriteToOutboxSequencesAreMonotonic): n/a

### test_cdc_pause_control.cpp

#### `TEST(InMemoryPauseControlTest, BufferEventDuringPause)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (BufferEventDuringPause): n/a

#### `TEST(InMemoryPauseControlTest, BufferEventFailsWhenNotPaused)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (BufferEventFailsWhenNotPaused): n/a

#### `TEST(InMemoryPauseControlTest, BufferOverflowReturnsFalse)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (BufferOverflowReturnsFalse): n/a

#### `TEST(InMemoryPauseControlTest, ConcurrentPauseResume)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:189
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (ConcurrentPauseResume): n/a

#### `TEST(InMemoryPauseControlTest, DrainBufferClearsBuffer)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (DrainBufferClearsBuffer): n/a

#### `TEST(InMemoryPauseControlTest, DrainBufferReturnsAllEventsInOrder)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (DrainBufferReturnsAllEventsInOrder): n/a

#### `TEST(InMemoryPauseControlTest, InitiallyNotPaused)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (InitiallyNotPaused): n/a

#### `TEST(InMemoryPauseControlTest, PauseReasonAdminRequest)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (PauseReasonAdminRequest): n/a

#### `TEST(InMemoryPauseControlTest, PauseReasonBackpressure)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (PauseReasonBackpressure): n/a

#### `TEST(InMemoryPauseControlTest, PauseReasonSchemaEvolution)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (PauseReasonSchemaEvolution): n/a

#### `TEST(InMemoryPauseControlTest, PauseThenResume)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (PauseThenResume): n/a

#### `TEST(InMemoryPauseControlTest, PauseTwiceIsNoOp)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (PauseTwiceIsNoOp): n/a

#### `TEST(InMemoryPauseControlTest, PolymorphicUsage)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:222
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (PolymorphicUsage): n/a

#### `TEST(InMemoryPauseControlTest, ResumeTwiceIsNoOp)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (ResumeTwiceIsNoOp): n/a

#### `TEST(InMemoryPauseControlTest, WaitForResumeSucceeds)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (WaitForResumeSucceeds): n/a

#### `TEST(InMemoryPauseControlTest, WaitForResumeTimesOut)`
- Source: `tests/cdc/test_cdc_pause_control.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryPauseControlTest): n/a
  - `<unnamed>` (WaitForResumeTimesOut): n/a

### test_cdc_production_fixes.cpp

#### `TEST_F(CDCProductionFixesTest, AtomicSequenceGenerationUnderConcurrency)`
- Source: `tests/cdc/test_cdc_production_fixes.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCProductionFixesTest): n/a
  - `<unnamed>` (AtomicSequenceGenerationUnderConcurrency): n/a

#### `TEST_F(CDCProductionFixesTest, BufferConcurrentAccess)`
- Source: `tests/cdc/test_cdc_production_fixes.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCProductionFixesTest): n/a
  - `<unnamed>` (BufferConcurrentAccess): n/a

#### `TEST_F(CDCProductionFixesTest, BufferLockSafetyUnderOverflow)`
- Source: `tests/cdc/test_cdc_production_fixes.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCProductionFixesTest): n/a
  - `<unnamed>` (BufferLockSafetyUnderOverflow): n/a

#### `TEST_F(CDCProductionFixesTest, CompressionErrorHandling)`
- Source: `tests/cdc/test_cdc_production_fixes.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCProductionFixesTest): n/a
  - `<unnamed>` (CompressionErrorHandling): n/a

#### `TEST_F(CDCProductionFixesTest, DecompressionErrorHandling)`
- Source: `tests/cdc/test_cdc_production_fixes.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCProductionFixesTest): n/a
  - `<unnamed>` (DecompressionErrorHandling): n/a

#### `TEST_F(CDCProductionFixesTest, RecoverFromEmptyKey)`
- Source: `tests/cdc/test_cdc_production_fixes.cpp`:311
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCProductionFixesTest): n/a
  - `<unnamed>` (RecoverFromEmptyKey): n/a

#### `TEST_F(CDCProductionFixesTest, SequenceGenerationConsistency)`
- Source: `tests/cdc/test_cdc_production_fixes.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCProductionFixesTest): n/a
  - `<unnamed>` (SequenceGenerationConsistency): n/a

#### `TEST_F(CDCProductionFixesTest, StressTestConcurrentSequenceGeneration)`
- Source: `tests/cdc/test_cdc_production_fixes.cpp`:344
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCProductionFixesTest): n/a
  - `<unnamed>` (StressTestConcurrentSequenceGeneration): n/a

### test_cdc_replay_ack_hardening.cpp

#### `TEST_F(ReplayAckHardeningTest, RAH01_ReplayFromSequenceReturnsOrderedEvents)`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplayAckHardeningTest): n/a
  - `<unnamed>` (RAH01_ReplayFromSequenceReturnsOrderedEvents): n/a

#### `TEST_F(ReplayAckHardeningTest, RAH02_ReplaySessionDrainsCompletely)`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplayAckHardeningTest): n/a
  - `<unnamed>` (RAH02_ReplaySessionDrainsCompletely): n/a

#### `TEST_F(ReplayAckHardeningTest, RAH03_CancelTerminatesSession)`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplayAckHardeningTest): n/a
  - `<unnamed>` (RAH03_CancelTerminatesSession): n/a

#### `TEST_F(ReplayAckHardeningTest, RAH04_ListEventsRespectsFromSequenceBound)`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplayAckHardeningTest): n/a
  - `<unnamed>` (RAH04_ListEventsRespectsFromSequenceBound): n/a

#### `TEST_F(ReplayAckHardeningTest, RAH05_AcknowledgeUpToCumulativeAck)`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:230
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplayAckHardeningTest): n/a
  - `<unnamed>` (RAH05_AcknowledgeUpToCumulativeAck): n/a

#### `TEST_F(ReplayAckHardeningTest, RAH06_ListEventsRespectsToSequenceBound)`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:250
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplayAckHardeningTest): n/a
  - `<unnamed>` (RAH06_ListEventsRespectsToSequenceBound): n/a

#### `TEST_F(ReplayAckHardeningTest, RAH07_LargeTimeoutOverrideHoldsTrackedEvents)`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplayAckHardeningTest): n/a
  - `<unnamed>` (RAH07_LargeTimeoutOverrideHoldsTrackedEvents): n/a

#### `TEST_F(ReplayAckHardeningTest, RAH08_TotalSessionsCreatedIncrements)`
- Source: `tests/cdc/test_cdc_replay_ack_hardening.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (ReplayAckHardeningTest): n/a
  - `<unnamed>` (RAH08_TotalSessionsCreatedIncrements): n/a

### test_cdc_replay_controller.cpp

#### `TEST(InMemoryReplayControllerTest, BatchSizeIsRespected)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (BatchSizeIsRespected): n/a

#### `TEST(InMemoryReplayControllerTest, BeginReplayReturnsNonNull)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (BeginReplayReturnsNonNull): n/a

#### `TEST(InMemoryReplayControllerTest, CancelMakesDone)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:305
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (CancelMakesDone): n/a

#### `TEST(InMemoryReplayControllerTest, DeliveredCountIncrementsAcrossBatches)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:317
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (DeliveredCountIncrementsAcrossBatches): n/a

#### `TEST(InMemoryReplayControllerTest, DrainAllEventsTransitionsToDone)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:364
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (DrainAllEventsTransitionsToDone): n/a

#### `TEST(InMemoryReplayControllerTest, EmptyFeedSessionIsDone)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (EmptyFeedSessionIsDone): n/a

#### `TEST(InMemoryReplayControllerTest, EventTypeFilterWorks)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (EventTypeFilterWorks): n/a

#### `TEST(InMemoryReplayControllerTest, FromSequenceFiltersOlderEvents)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (FromSequenceFiltersOlderEvents): n/a

#### `TEST(InMemoryReplayControllerTest, KeyPrefixFilterWorks)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:244
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (KeyPrefixFilterWorks): n/a

#### `TEST(InMemoryReplayControllerTest, MaxEventsPerSessionIsRespected)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (MaxEventsPerSessionIsRespected): n/a

#### `TEST(InMemoryReplayControllerTest, ReplayFromSequenceConvenienceOverload)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (ReplayFromSequenceConvenienceOverload): n/a

#### `TEST(InMemoryReplayControllerTest, ReplayFromTimestampConvenienceOverload)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:344
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (ReplayFromTimestampConvenienceOverload): n/a

#### `TEST(InMemoryReplayControllerTest, TimestampRangeFiltersCorrectly)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (TimestampRangeFiltersCorrectly): n/a

#### `TEST(InMemoryReplayControllerTest, ToSequenceCapsUpperBound)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (ToSequenceCapsUpperBound): n/a

#### `TEST(InMemoryReplayControllerTest, TotalSessionsCreatedIncrements)`
- Source: `tests/cdc/test_cdc_replay_controller.cpp`:334
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryReplayControllerTest): n/a
  - `<unnamed>` (TotalSessionsCreatedIncrements): n/a

### test_cdc_retention.cpp

#### `TEST_F(CDCRetentionTest, BackgroundCleanupThread)`
- Source: `tests/cdc/test_cdc_retention.cpp`:277
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (BackgroundCleanupThread): n/a

#### `TEST_F(CDCRetentionTest, CompactionEmptyLog)`
- Source: `tests/cdc/test_cdc_retention.cpp`:373
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (CompactionEmptyLog): n/a

#### `TEST_F(CDCRetentionTest, CompactionKeepsLatestEventPerKey)`
- Source: `tests/cdc/test_cdc_retention.cpp`:384
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (CompactionKeepsLatestEventPerKey): n/a

#### `TEST_F(CDCRetentionTest, CompactionMultipleKeys)`
- Source: `tests/cdc/test_cdc_retention.cpp`:411
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (CompactionMultipleKeys): n/a

#### `TEST_F(CDCRetentionTest, CompactionNoOpWhenAllUnique)`
- Source: `tests/cdc/test_cdc_retention.cpp`:490
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (CompactionNoOpWhenAllUnique): n/a

#### `TEST_F(CDCRetentionTest, CompactionPreservesDeleteTombstones)`
- Source: `tests/cdc/test_cdc_retention.cpp`:453
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (CompactionPreservesDeleteTombstones): n/a

#### `TEST_F(CDCRetentionTest, DeleteOldEventsBySequence)`
- Source: `tests/cdc/test_cdc_retention.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (DeleteOldEventsBySequence): n/a

#### `TEST_F(CDCRetentionTest, DeleteOldEventsByTimestamp)`
- Source: `tests/cdc/test_cdc_retention.cpp`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (DeleteOldEventsByTimestamp): n/a

#### `TEST_F(CDCRetentionTest, GetEventBySequence)`
- Source: `tests/cdc/test_cdc_retention.cpp`:511
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (GetEventBySequence): n/a

#### `TEST_F(CDCRetentionTest, RetentionByEventCount)`
- Source: `tests/cdc/test_cdc_retention.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (RetentionByEventCount): n/a

#### `TEST_F(CDCRetentionTest, RetentionByTimestamp)`
- Source: `tests/cdc/test_cdc_retention.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (RetentionByTimestamp): n/a

#### `TEST_F(CDCRetentionTest, RetentionPolicyDisabledByDefault)`
- Source: `tests/cdc/test_cdc_retention.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (RetentionPolicyDisabledByDefault): n/a

#### `TEST_F(CDCRetentionTest, RetentionStatusIncludesPolicy)`
- Source: `tests/cdc/test_cdc_retention.cpp`:529
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (RetentionStatusIncludesPolicy): n/a

#### `TEST_F(CDCRetentionTest, RetentionStatusJsonIncludesCleanupThreadRunning)`
- Source: `tests/cdc/test_cdc_retention.cpp`:717
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (RetentionStatusJsonIncludesCleanupThreadRunning): n/a

#### `TEST_F(CDCRetentionTest, RetentionStatusJsonIncludesPolicyObject)`
- Source: `tests/cdc/test_cdc_retention.cpp`:554
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (RetentionStatusJsonIncludesPolicyObject): n/a

#### `TEST_F(CDCRetentionTest, RetentionStatusReflectsUpdatedPolicy)`
- Source: `tests/cdc/test_cdc_retention.cpp`:577
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (RetentionStatusReflectsUpdatedPolicy): n/a

#### `TEST_F(CDCRetentionTest, RetentionStatusReportsCleanupThreadState)`
- Source: `tests/cdc/test_cdc_retention.cpp`:685
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (RetentionStatusReportsCleanupThreadState): n/a

#### `TEST_F(CDCRetentionTest, StopBackgroundCleanup)`
- Source: `tests/cdc/test_cdc_retention.cpp`:308
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (StopBackgroundCleanup): n/a

#### `TEST_F(CDCRetentionTest, UpdateRetentionPolicyIdempotentWhenAlreadyEnabled)`
- Source: `tests/cdc/test_cdc_retention.cpp`:654
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (UpdateRetentionPolicyIdempotentWhenAlreadyEnabled): n/a

#### `TEST_F(CDCRetentionTest, UpdateRetentionPolicyStartsThreadWhenEnabled)`
- Source: `tests/cdc/test_cdc_retention.cpp`:607
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (UpdateRetentionPolicyStartsThreadWhenEnabled): n/a

#### `TEST_F(CDCRetentionTest, UpdateRetentionPolicyStopsThreadWhenDisabled)`
- Source: `tests/cdc/test_cdc_retention.cpp`:632
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (UpdateRetentionPolicyStopsThreadWhenDisabled): n/a

#### `TEST_F(CDCRetentionTest, WatermarksAfterRetention)`
- Source: `tests/cdc/test_cdc_retention.cpp`:337
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (WatermarksAfterRetention): n/a

#### `TEST_F(CDCRetentionTest, WatermarksEmptyChangefeed)`
- Source: `tests/cdc/test_cdc_retention.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (WatermarksEmptyChangefeed): n/a

#### `TEST_F(CDCRetentionTest, WatermarksWithEvents)`
- Source: `tests/cdc/test_cdc_retention.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (CDCRetentionTest): n/a
  - `<unnamed>` (WatermarksWithEvents): n/a

### test_cdc_schema_registry.cpp

#### `TEST(CdcBinaryEncoderInjectionTest, CDCSEBIN01_AvroEncoderInjected)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:553
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcBinaryEncoderInjectionTest): n/a
  - `<unnamed>` (CDCSEBIN01_AvroEncoderInjected): n/a

#### `TEST(CdcBinaryEncoderInjectionTest, CDCSEBIN02_AvroEncoderEmptyReturnFallsBackToJson)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:578
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcBinaryEncoderInjectionTest): n/a
  - `<unnamed>` (CDCSEBIN02_AvroEncoderEmptyReturnFallsBackToJson): n/a

#### `TEST(CdcBinaryEncoderInjectionTest, CDCSEBIN03_ProtobufEncoderInjected)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:598
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcBinaryEncoderInjectionTest): n/a
  - `<unnamed>` (CDCSEBIN03_ProtobufEncoderInjected): n/a

#### `TEST(CdcBinaryEncoderInjectionTest, CDCSEBIN04_ProtobufEncoderNullptrClearsInjection)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:621
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcBinaryEncoderInjectionTest): n/a
  - `<unnamed>` (CDCSEBIN04_ProtobufEncoderNullptrClearsInjection): n/a

#### `TEST(CdcSchemaEncoderAvroTest, AvroFormatAutoRegistersAvroSchema)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:484
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderAvroTest): n/a
  - `<unnamed>` (AvroFormatAutoRegistersAvroSchema): n/a

#### `TEST(CdcSchemaEncoderDefaultsTest, DefaultAvroSchemaIsValidJson)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderDefaultsTest): n/a
  - `<unnamed>` (DefaultAvroSchemaIsValidJson): n/a

#### `TEST(CdcSchemaEncoderDefaultsTest, DefaultJsonSchemaIsValidJson)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderDefaultsTest): n/a
  - `<unnamed>` (DefaultJsonSchemaIsValidJson): n/a

#### `TEST(CdcSchemaEncoderDefaultsTest, DefaultProtobufSchemaContainsSyntax)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderDefaultsTest): n/a
  - `<unnamed>` (DefaultProtobufSchemaContainsSyntax): n/a

#### `TEST(CdcSchemaEncoderNoAutoRegTest, ThrowsWhenSchemaNotFound)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:420
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderNoAutoRegTest): n/a
  - `<unnamed>` (ThrowsWhenSchemaNotFound): n/a

#### `TEST(CdcSchemaEncoderProtoTest, ProtobufFormatAutoRegistersProtoSchema)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:503
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderProtoTest): n/a
  - `<unnamed>` (ProtobufFormatAutoRegistersProtoSchema): n/a

#### `TEST(ExtractSchemaIdTest, CorrectHeaderDecodesId)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:453
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExtractSchemaIdTest): n/a
  - `<unnamed>` (CorrectHeaderDecodesId): n/a

#### `TEST(ExtractSchemaIdTest, TooShortReturnsNegativeOne)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:443
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExtractSchemaIdTest): n/a
  - `<unnamed>` (TooShortReturnsNegativeOne): n/a

#### `TEST(ExtractSchemaIdTest, WrongMagicByteReturnsNegativeOne)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:448
- Brief: n/a
- Parameters:
  - `<unnamed>` (ExtractSchemaIdTest): n/a
  - `<unnamed>` (WrongMagicByteReturnsNegativeOne): n/a

#### `TEST(InMemoryBackendTest, ClearRemovesAllSchemas)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackendTest): n/a
  - `<unnamed>` (ClearRemovesAllSchemas): n/a

#### `TEST(InMemoryBackendTest, DifferentSubjectsDifferentIds)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackendTest): n/a
  - `<unnamed>` (DifferentSubjectsDifferentIds): n/a

#### `TEST(InMemoryBackendTest, GetByIdReturnsRegisteredSchema)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackendTest): n/a
  - `<unnamed>` (GetByIdReturnsRegisteredSchema): n/a

#### `TEST(InMemoryBackendTest, GetByIdUnknownReturnsNullopt)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackendTest): n/a
  - `<unnamed>` (GetByIdUnknownReturnsNullopt): n/a

#### `TEST(InMemoryBackendTest, GetLatestReturnsNewestSchema)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackendTest): n/a
  - `<unnamed>` (GetLatestReturnsNewestSchema): n/a

#### `TEST(InMemoryBackendTest, GetLatestUnknownSubjectReturnsNullopt)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackendTest): n/a
  - `<unnamed>` (GetLatestUnknownSubjectReturnsNullopt): n/a

#### `TEST(InMemoryBackendTest, IdempotentRegistrationReturnsSameId)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackendTest): n/a
  - `<unnamed>` (IdempotentRegistrationReturnsSameId): n/a

#### `TEST(InMemoryBackendTest, RegisterReturnsNonNegativeId)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackendTest): n/a
  - `<unnamed>` (RegisterReturnsNonNegativeId): n/a

#### `TEST(InMemoryBackendTest, SchemaInfoIsValidAfterRegistration)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackendTest): n/a
  - `<unnamed>` (SchemaInfoIsValidAfterRegistration): n/a

#### `TEST(InMemoryBackendTest, SizeReflectsRegisteredCount)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (InMemoryBackendTest): n/a
  - `<unnamed>` (SizeReflectsRegisteredCount): n/a

#### `TEST(SchemaFormatTest, FormatStringAVRO)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaFormatTest): n/a
  - `<unnamed>` (FormatStringAVRO): n/a

#### `TEST(SchemaFormatTest, FormatStringJSON)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaFormatTest): n/a
  - `<unnamed>` (FormatStringJSON): n/a

#### `TEST(SchemaFormatTest, FormatStringPROTOBUF)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaFormatTest): n/a
  - `<unnamed>` (FormatStringPROTOBUF): n/a

#### `TEST(SchemaRegistryClientTest, ClearCacheDoesNotCorruptBackend)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaRegistryClientTest): n/a
  - `<unnamed>` (ClearCacheDoesNotCorruptBackend): n/a

#### `TEST(SchemaRegistryClientTest, ConfigAccessor)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaRegistryClientTest): n/a
  - `<unnamed>` (ConfigAccessor): n/a

#### `TEST(SchemaRegistryClientTest, ConstructWithDefaultInMemoryBackend)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaRegistryClientTest): n/a
  - `<unnamed>` (ConstructWithDefaultInMemoryBackend): n/a

#### `TEST(SchemaRegistryClientTest, EnsureSchemaIdempotent)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaRegistryClientTest): n/a
  - `<unnamed>` (EnsureSchemaIdempotent): n/a

#### `TEST(SchemaRegistryClientTest, EnsureSchemaReturnsId)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaRegistryClientTest): n/a
  - `<unnamed>` (EnsureSchemaReturnsId): n/a

#### `TEST(SchemaRegistryClientTest, GetLatestSchemaReturnsInfo)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaRegistryClientTest): n/a
  - `<unnamed>` (GetLatestSchemaReturnsInfo): n/a

#### `TEST(SchemaRegistryClientTest, GetSchemaByIdReturnsInfo)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaRegistryClientTest): n/a
  - `<unnamed>` (GetSchemaByIdReturnsInfo): n/a

#### `TEST(SchemaRegistryClientTest, GetSchemaByIdUnknownReturnsNullopt)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaRegistryClientTest): n/a
  - `<unnamed>` (GetSchemaByIdUnknownReturnsNullopt): n/a

#### `TEST(SchemaRegistryConfigTest, Defaults)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaRegistryConfigTest): n/a
  - `<unnamed>` (Defaults): n/a

#### `TEST(WireFormatTest, HeaderSizeIsFive)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireFormatTest): n/a
  - `<unnamed>` (HeaderSizeIsFive): n/a

#### `TEST(WireFormatTest, MagicByteIsZero)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (WireFormatTest): n/a
  - `<unnamed>` (MagicByteIsZero): n/a

#### `TEST_F(CdcSchemaEncoderTest, ClearLocalCacheDoesNotCorruptEncoding)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:433
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (ClearLocalCacheDoesNotCorruptEncoding): n/a

#### `TEST_F(CdcSchemaEncoderTest, CollectionDerivedFromKeyWhenNotExplicit)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:403
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (CollectionDerivedFromKeyWhenNotExplicit): n/a

#### `TEST_F(CdcSchemaEncoderTest, DecodeToJsonContainsSourceBlock)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:342
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (DecodeToJsonContainsSourceBlock): n/a

#### `TEST_F(CdcSchemaEncoderTest, DecodeToJsonContainsValueField)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:334
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (DecodeToJsonContainsValueField): n/a

#### `TEST_F(CdcSchemaEncoderTest, DecodeToJsonEmptyBytesReturnsNullopt)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:465
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (DecodeToJsonEmptyBytesReturnsNullopt): n/a

#### `TEST_F(CdcSchemaEncoderTest, DecodeToJsonInvalidPayloadReturnsNullopt)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:473
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (DecodeToJsonInvalidPayloadReturnsNullopt): n/a

#### `TEST_F(CdcSchemaEncoderTest, DecodeToJsonRoundTrip)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:320
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (DecodeToJsonRoundTrip): n/a

#### `TEST_F(CdcSchemaEncoderTest, DecodeToJsonTruncatedHeaderReturnsNullopt)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:469
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (DecodeToJsonTruncatedHeaderReturnsNullopt): n/a

#### `TEST_F(CdcSchemaEncoderTest, DeleteEventOperationIsDelete)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (DeleteEventOperationIsDelete): n/a

#### `TEST_F(CdcSchemaEncoderTest, DeleteEventValueIsNull)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (DeleteEventValueIsNull): n/a

#### `TEST_F(CdcSchemaEncoderTest, DifferentCollectionsUseDifferentSchemaIds)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:394
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (DifferentCollectionsUseDifferentSchemaIds): n/a

#### `TEST_F(CdcSchemaEncoderTest, EncodeProducesNonEmptyBytes)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (EncodeProducesNonEmptyBytes): n/a

#### `TEST_F(CdcSchemaEncoderTest, EncodedDataHasCorrectHeaderSize)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (EncodedDataHasCorrectHeaderSize): n/a

#### `TEST_F(CdcSchemaEncoderTest, EncodedDataStartsWithMagicByte)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:293
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (EncodedDataStartsWithMagicByte): n/a

#### `TEST_F(CdcSchemaEncoderTest, ExplicitCollectionOverridesKeyDerived)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:411
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (ExplicitCollectionOverridesKeyDerived): n/a

#### `TEST_F(CdcSchemaEncoderTest, ExtractSchemaIdMatchesResultSchemaId)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (ExtractSchemaIdMatchesResultSchemaId): n/a

#### `TEST_F(CdcSchemaEncoderTest, RedactedEventHasRedactedField)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (RedactedEventHasRedactedField): n/a

#### `TEST_F(CdcSchemaEncoderTest, SchemaIdIsNonNegative)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (SchemaIdIsNonNegative): n/a

#### `TEST_F(CdcSchemaEncoderTest, SubjectContainsCollectionName)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (SubjectContainsCollectionName): n/a

#### `TEST_F(CdcSchemaEncoderTest, TransactionCommitOperationString)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:520
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (TransactionCommitOperationString): n/a

#### `TEST_F(CdcSchemaEncoderTest, TransactionRollbackOperationString)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:533
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (TransactionRollbackOperationString): n/a

#### `TEST_F(CdcSchemaEncoderTest, UpdateEventHasBeforeAndAfterFields)`
- Source: `tests/cdc/test_cdc_schema_registry.cpp`:369
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSchemaEncoderTest): n/a
  - `<unnamed>` (UpdateEventHasBeforeAndAfterFields): n/a

### test_cdc_subscription_auth.cpp

#### `TEST_F(CdcFeatureDisabledAuthTest, Get_FeatureDisabled_AuthorizedUser_Returns404)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:499
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcFeatureDisabledAuthTest): n/a
  - `<unnamed>` (Get_FeatureDisabled_AuthorizedUser_Returns404): n/a

#### `TEST_F(CdcFeatureDisabledAuthTest, Get_FeatureDisabled_NoAuth_Returns401NotFound)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:476
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcFeatureDisabledAuthTest): n/a
  - `<unnamed>` (Get_FeatureDisabled_NoAuth_Returns401NotFound): n/a

#### `TEST_F(CdcFeatureDisabledAuthTest, Stats_FeatureDisabled_NoAuth_Returns401NotFound)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:493
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcFeatureDisabledAuthTest): n/a
  - `<unnamed>` (Stats_FeatureDisabled_NoAuth_Returns401NotFound): n/a

#### `TEST_F(CdcFeatureDisabledAuthTest, Stream_FeatureDisabled_NoAuth_Returns401NotFound)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:487
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcFeatureDisabledAuthTest): n/a
  - `<unnamed>` (Stream_FeatureDisabled_NoAuth_Returns401NotFound): n/a

#### `TEST_F(CdcSubscriptionAuthTest, AuthDenied_ResponseBody_ContainsErrorField)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:592
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (AuthDenied_ResponseBody_ContainsErrorField): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Compact_CdcAdminToken_Returns200)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Compact_CdcAdminToken_Returns200): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Compact_CdcReadOnlyToken_Returns403)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Compact_CdcReadOnlyToken_Returns403): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Compact_NoAuthHeader_Returns401)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Compact_NoAuthHeader_Returns401): n/a

#### `TEST_F(CdcSubscriptionAuthTest, EmptyBearerToken_Returns401)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:408
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (EmptyBearerToken_Returns401): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Get_CdcAdminToken_Returns200)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:224
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Get_CdcAdminToken_Returns200): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Get_CdcReadToken_Returns200)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:218
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Get_CdcReadToken_Returns200): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Get_InvalidKeyPrefix_Returns400)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Get_InvalidKeyPrefix_Returns400): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Get_InvalidToken_Returns403)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Get_InvalidToken_Returns403): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Get_NoAuthHeader_Returns401)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Get_NoAuthHeader_Returns401): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Get_TokenMissingCdcReadScope_Returns403)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Get_TokenMissingCdcReadScope_Returns403): n/a

#### `TEST_F(CdcSubscriptionAuthTest, KeyPrefixFilter_EmptyPrefix_ReturnsAllEvents)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:560
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (KeyPrefixFilter_EmptyPrefix_ReturnsAllEvents): n/a

#### `TEST_F(CdcSubscriptionAuthTest, KeyPrefixFilter_NoEventsOutsidePrefix)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:538
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (KeyPrefixFilter_NoEventsOutsidePrefix): n/a

#### `TEST_F(CdcSubscriptionAuthTest, KeyPrefixFilter_NonMatchingPrefix_ReturnsEmpty)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:573
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (KeyPrefixFilter_NonMatchingPrefix_ReturnsEmpty): n/a

#### `TEST_F(CdcSubscriptionAuthTest, KeyPrefixFilter_OnlyReturnsMatchingEvents)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:511
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (KeyPrefixFilter_OnlyReturnsMatchingEvents): n/a

#### `TEST_F(CdcSubscriptionAuthTest, MalformedAuthHeader_NotBearer_Returns401)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:401
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (MalformedAuthHeader_NotBearer_Returns401): n/a

#### `TEST_F(CdcSubscriptionAuthTest, RetentionGet_CdcReadToken_Returns200)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (RetentionGet_CdcReadToken_Returns200): n/a

#### `TEST_F(CdcSubscriptionAuthTest, RetentionGet_NoAuthHeader_Returns401)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (RetentionGet_NoAuthHeader_Returns401): n/a

#### `TEST_F(CdcSubscriptionAuthTest, RetentionPost_CdcAdminToken_Returns200)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:319
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (RetentionPost_CdcAdminToken_Returns200): n/a

#### `TEST_F(CdcSubscriptionAuthTest, RetentionPost_CdcReadOnlyToken_Returns403)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (RetentionPost_CdcReadOnlyToken_Returns403): n/a

#### `TEST_F(CdcSubscriptionAuthTest, RetentionPost_NoAuthHeader_Returns401)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:306
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (RetentionPost_NoAuthHeader_Returns401): n/a

#### `TEST_F(CdcSubscriptionAuthTest, RetentionPut_CdcAdminToken_Returns200)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (RetentionPut_CdcAdminToken_Returns200): n/a

#### `TEST_F(CdcSubscriptionAuthTest, RetentionPut_CdcReadOnlyToken_Returns403)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (RetentionPut_CdcReadOnlyToken_Returns403): n/a

#### `TEST_F(CdcSubscriptionAuthTest, RetentionPut_NoAuthHeader_Returns401)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (RetentionPut_NoAuthHeader_Returns401): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Stats_CdcAdminToken_Returns200)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:296
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Stats_CdcAdminToken_Returns200): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Stats_CdcReadOnlyToken_Returns403)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Stats_CdcReadOnlyToken_Returns403): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Stats_NoAuthHeader_Returns401)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:281
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Stats_NoAuthHeader_Returns401): n/a

#### `TEST_F(CdcSubscriptionAuthTest, StreamAck_InvalidConsumerId_Returns400)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:367
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (StreamAck_InvalidConsumerId_Returns400): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Stream_CdcReadToken_Returns200)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Stream_CdcReadToken_Returns200): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Stream_InvalidConsumerId_Returns400)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Stream_InvalidConsumerId_Returns400): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Stream_InvalidToken_Returns403)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Stream_InvalidToken_Returns403): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Stream_NoAuthHeader_Returns401)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:241
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Stream_NoAuthHeader_Returns401): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Stream_TokenMissingCdcReadScope_Returns403)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Stream_TokenMissingCdcReadScope_Returns403): n/a

#### `TEST_F(CdcSubscriptionAuthTest, Unauthorized_ResponseBody_ContainsErrorField)`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:605
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcSubscriptionAuthTest): n/a
  - `<unnamed>` (Unauthorized_ResponseBody_ContainsErrorField): n/a

#### `http::request< http::string_body > makeGet(const std::string &target, const std::string &auth_header="")`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:54
- Brief: n/a
- Parameters:
  - `target` (const std::string &): n/a
  - `auth_header` (const std::string &): n/a

#### `http::request< http::string_body > makePost(const std::string &target, const json &body, const std::string &auth_header="")`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:69
- Brief: n/a
- Parameters:
  - `target` (const std::string &): n/a
  - `body` (const json &): n/a
  - `auth_header` (const std::string &): n/a

#### `http::request< http::string_body > makePut(const std::string &target, const json &body, const std::string &auth_header="")`
- Source: `tests/cdc/test_cdc_subscription_auth.cpp`:88
- Brief: n/a
- Parameters:
  - `target` (const std::string &): n/a
  - `body` (const json &): n/a
  - `auth_header` (const std::string &): n/a

### test_cdc_transport_degradation.cpp

#### `TEST_F(TransportDegradationTest, TRD01_TrackDeliveryRecordsPending)`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportDegradationTest): n/a
  - `<unnamed>` (TRD01_TrackDeliveryRecordsPending): n/a

#### `TEST_F(TransportDegradationTest, TRD02_ZeroTimeoutOverrideReturnsAllPending)`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportDegradationTest): n/a
  - `<unnamed>` (TRD02_ZeroTimeoutOverrideReturnsAllPending): n/a

#### `TEST_F(TransportDegradationTest, TRD03_RedeliveryAttemptCountIncrementsOnTimeout)`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportDegradationTest): n/a
  - `<unnamed>` (TRD03_RedeliveryAttemptCountIncrementsOnTimeout): n/a

#### `TEST_F(TransportDegradationTest, TRD04_RemoveConsumerClearsPendingState)`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportDegradationTest): n/a
  - `<unnamed>` (TRD04_RemoveConsumerClearsPendingState): n/a

#### `TEST_F(TransportDegradationTest, TRD05_PartitionAssignmentIsDeterministic)`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportDegradationTest): n/a
  - `<unnamed>` (TRD05_PartitionAssignmentIsDeterministic): n/a

#### `TEST_F(TransportDegradationTest, TRD06_PartitionFanOutIsDisjoint)`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportDegradationTest): n/a
  - `<unnamed>` (TRD06_PartitionFanOutIsDisjoint): n/a

#### `TEST_F(TransportDegradationTest, TRD07_MaxPendingLimitEnforcesBackpressure)`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportDegradationTest): n/a
  - `<unnamed>` (TRD07_MaxPendingLimitEnforcesBackpressure): n/a

#### `TEST_F(TransportDegradationTest, TRD08_AcknowledgeReturnsFalseForUnknownContext)`
- Source: `tests/cdc/test_cdc_transport_degradation.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (TransportDegradationTest): n/a
  - `<unnamed>` (TRD08_AcknowledgeReturnsFalseForUnknownContext): n/a

### test_cdc_ws_handler.cpp

#### `TEST(CdcWsHandlerGroupTest, AckByGroupIdNoConsumerGroupManagerIsNoop)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:374
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerGroupTest): n/a
  - `<unnamed>` (AckByGroupIdNoConsumerGroupManagerIsNoop): n/a

#### `TEST(CdcWsHandlerGroupTest, AckByIdWithNoGroupIdOrIdReturnsError)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:410
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerGroupTest): n/a
  - `<unnamed>` (AckByIdWithNoGroupIdOrIdReturnsError): n/a

#### `TEST(CdcWsHandlerGroupTest, SubscribeWithGroupIdAndExplicitIdUsesExplicitId)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerGroupTest): n/a
  - `<unnamed>` (SubscribeWithGroupIdAndExplicitIdUsesExplicitId): n/a

#### `TEST(CdcWsHandlerGroupTest, SubscribeWithGroupIdCreatesSubscription)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:314
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerGroupTest): n/a
  - `<unnamed>` (SubscribeWithGroupIdCreatesSubscription): n/a

#### `TEST(CdcWsHandlerGroupTest, SubscribeWithGroupIdNoManagerDoesNotCrash)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:392
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerGroupTest): n/a
  - `<unnamed>` (SubscribeWithGroupIdNoManagerDoesNotCrash): n/a

#### `TEST(CdcWsHandlerGroupTest, SubscribeWithNoIdAndNoGroupIdReturnsError)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerGroupTest): n/a
  - `<unnamed>` (SubscribeWithNoIdAndNoGroupIdReturnsError): n/a

#### `TEST(CdcWsHandlerGroupTest, UnsubscribeByGroupId)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:359
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerGroupTest): n/a
  - `<unnamed>` (UnsubscribeByGroupId): n/a

#### `TEST(CdcWsHandlerTest, AckForUnknownIdProducesNoResponse)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (AckForUnknownIdProducesNoResponse): n/a

#### `TEST(CdcWsHandlerTest, AckProducesNoResponseOnSuccess)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (AckProducesNoResponseOnSuccess): n/a

#### `TEST(CdcWsHandlerTest, AckWithoutIdReturnsError)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (AckWithoutIdReturnsError): n/a

#### `TEST(CdcWsHandlerTest, CheckRedeliveryReturnsEmptyBeforeTimeout)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:208
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (CheckRedeliveryReturnsEmptyBeforeTimeout): n/a

#### `TEST(CdcWsHandlerTest, CollectionMapsToKeyPrefix)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (CollectionMapsToKeyPrefix): n/a

#### `TEST(CdcWsHandlerTest, DuplicateSubscribeIdOverwrites)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (DuplicateSubscribeIdOverwrites): n/a

#### `TEST(CdcWsHandlerTest, EventTypesArrayParsed)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (EventTypesArrayParsed): n/a

#### `TEST(CdcWsHandlerTest, ExplicitKeyPrefixOverridesCollection)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (ExplicitKeyPrefixOverridesCollection): n/a

#### `TEST(CdcWsHandlerTest, FromSequenceInSubscribeFrame)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (FromSequenceInSubscribeFrame): n/a

#### `TEST(CdcWsHandlerTest, HasSubscriptionsFalseAfterAllUnsubscribed)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (HasSubscriptionsFalseAfterAllUnsubscribed): n/a

#### `TEST(CdcWsHandlerTest, HasSubscriptionsFalseOnFreshHandler)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (HasSubscriptionsFalseOnFreshHandler): n/a

#### `TEST(CdcWsHandlerTest, HasSubscriptionsTrueAfterSubscribe)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (HasSubscriptionsTrueAfterSubscribe): n/a

#### `TEST(CdcWsHandlerTest, MultipleNamedSubscriptionsCoexist)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (MultipleNamedSubscriptionsCoexist): n/a

#### `TEST(CdcWsHandlerTest, OverflowCounterStartsAtZero)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:243
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (OverflowCounterStartsAtZero): n/a

#### `TEST(CdcWsHandlerTest, OverflowCounterUnchangedWithoutBackpressure)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:248
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (OverflowCounterUnchangedWithoutBackpressure): n/a

#### `TEST(CdcWsHandlerTest, SubscribeCreatesActiveSubscription)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (SubscribeCreatesActiveSubscription): n/a

#### `TEST(CdcWsHandlerTest, SubscribeReturnsSubscribedAck)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:17
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (SubscribeReturnsSubscribedAck): n/a

#### `TEST(CdcWsHandlerTest, SubscribeWithoutIdReturnsError)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:28
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (SubscribeWithoutIdReturnsError): n/a

#### `TEST(CdcWsHandlerTest, UnknownActionReturnsError)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (UnknownActionReturnsError): n/a

#### `TEST(CdcWsHandlerTest, UnsubscribeRemovesSubscription)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (UnsubscribeRemovesSubscription): n/a

#### `TEST(CdcWsHandlerTest, UnsubscribeWithoutIdReturnsError)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsHandlerTest): n/a
  - `<unnamed>` (UnsubscribeWithoutIdReturnsError): n/a

#### `TEST_F(CdcWsGroupIntegrationTest, AckByGroupIdAdvancesCommittedOffset)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:510
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsGroupIntegrationTest): n/a
  - `<unnamed>` (AckByGroupIdAdvancesCommittedOffset): n/a

#### `TEST_F(CdcWsGroupIntegrationTest, PartitionFilterDeliverOnlyConsumerEvents)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:539
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsGroupIntegrationTest): n/a
  - `<unnamed>` (PartitionFilterDeliverOnlyConsumerEvents): n/a

#### `TEST_F(CdcWsGroupIntegrationTest, SubscribeResumesFromCommittedOffset)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:474
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsGroupIntegrationTest): n/a
  - `<unnamed>` (SubscribeResumesFromCommittedOffset): n/a

#### `TEST_F(CdcWsOverflowTest, OverflowCounterIncrementsWhenPendingAckQueueFull)`
- Source: `tests/cdc/test_cdc_ws_handler.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (CdcWsOverflowTest): n/a
  - `<unnamed>` (OverflowCounterIncrementsWhenPendingAckQueueFull): n/a

### themis::Changefeed

#### `Changefeed(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf=nullptr, RetentionPolicy retention=RetentionPolicy::defaults())`
- Source: `include/cdc/changefeed.h`:127
- Brief: n/a
- Parameters:
  - `db` (rocksdb::TransactionDB *): n/a
  - `cf` (rocksdb::ColumnFamilyHandle *): n/a
  - `retention` (RetentionPolicy): n/a

#### `size_t applyRetentionPolicy()`
- Source: `include/cdc/changefeed.h`:236
- Brief: Apply Retention Policy.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: plk(), getStats(), count(), std::chrono::system_clock::now(), time_since_epoch(), deleteOldEventsByTimestamp(), THEMIS_DEBUG(), deleteOldEvents().

#### `void clear()`
- Source: `include/cdc/changefeed.h`:173
- Brief: Clear.
- Parameters: none
- Details: Calls: reset(), NewIterator(), Seek(), Valid(), Next(), key(), ToString(), compare().

#### `CompactionResult compactByKey()`
- Source: `include/cdc/changefeed.h`:223
- Brief: Compact By Key.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: reset(), NewIterator(), Seek(), Valid(), Next(), key(), ToString(), compare().

#### `size_t deleteOldEvents(uint64_t before_sequence)`
- Source: `include/cdc/changefeed.h`:180
- Brief: Delete Old Events.
- Parameters:
  - `before_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: before_sequence Input parameter. Return value. before_sequence Input parameter. Return value. Calls: reset(), NewIterator(), Seek(), Valid(), Next(), key(), ToString(), compare().

#### `size_t deleteOldEventsBySequence(uint64_t before_sequence)`
- Source: `include/cdc/changefeed.h`:188
- Brief: Delete Old Events By Sequence.
- Parameters:
  - `before_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: before_sequence Input parameter. Return value. Calls: deleteOldEvents().

#### `size_t deleteOldEventsByTimestamp(int64_t before_timestamp_ms)`
- Source: `include/cdc/changefeed.h`:197
- Brief: Delete Old Events By Timestamp.
- Parameters:
  - `before_timestamp_ms` (int64_t): Input parameter.
- Return: Return value.
- Details: before_timestamp_ms Input parameter. Return value. before_timestamp_ms Input parameter. Return value. Calls: reset(), NewIterator(), Seek(), Valid(), Next(), key(), ToString(), compare().

#### `ChangeEvent getEvent(uint64_t sequence) const`
- Source: `include/cdc/changefeed.h`:204
- Brief: Get Event.
- Parameters:
  - `sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: sequence Input parameter. Return value.

#### `uint64_t getLatestSequence() const`
- Source: `include/cdc/changefeed.h`:156
- Brief: Get Latest Sequence.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `RetentionPolicy getRetentionPolicy() const`
- Source: `include/cdc/changefeed.h`:248
- Brief: Get Retention Policy.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Stats getStats() const`
- Source: `include/cdc/changefeed.h`:162
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Watermarks getWatermarks() const`
- Source: `include/cdc/changefeed.h`:168
- Brief: Get Watermarks.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool isRetentionCleanupRunning() const noexcept`
- Source: `include/cdc/changefeed.h`:265
- Brief: Is Retention Cleanup Running.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Exception safety: noexcept.

#### `std::vector< ChangeEvent > listEvents() const`
- Source: `include/cdc/changefeed.h`:144
- Brief: List Events.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< ChangeEvent > listEvents(const ListOptions &options) const`
- Source: `include/cdc/changefeed.h`:150
- Brief: List Events.
- Parameters:
  - `options` (const ListOptions &): Input parameter.
- Return: Return value.
- Details: options Input parameter. Return value.

#### `uint64_t loadInitialSequence() const`
- Source: `include/cdc/changefeed.h`:371
- Brief: Load the initial sequence counter value from RocksDB at construction.
- Parameters: none
- Return: Return value.
- Details: Return value. Handles both the binary little-endian uint64 format (new) and the legacy decimal-string format (old). Falls back to scanning events when the DB key cannot be read (e.g. unresolved Merge operands without a registered merge operator).

#### `std::string makeKey(uint64_t sequence) const`
- Source: `include/cdc/changefeed.h`:359
- Brief: Make Key.
- Parameters:
  - `sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: sequence Input parameter. Return value.

#### `std::shared_ptr< rocksdb::MergeOperator > makeSequenceMergeOperator()`
- Source: `include/cdc/changefeed.h`:125
- Brief: Make Sequence Merge Operator.
- Parameters: none
- Return: Return value.
- Details: ===== Changefeed Implementation ===== Return value. Return value. Implements makeSequenceMergeOperator without additional internal calls.

#### `uint64_t nextSequence()`
- Source: `include/cdc/changefeed.h`:364
- Brief: Next Sequence.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: fetch_add(), load(), delta_slice(), Merge(), ok(), compare_exchange_weak(), store(), THEMIS_ERROR().

#### `void notifySubscribers(const ChangeEvent &event)`
- Source: `include/cdc/changefeed.h`:425
- Brief: Notify Subscribers.
- Parameters:
  - `event` (const ChangeEvent &): Input parameter.
- Details: event Input parameter. event Input parameter. Calls: load(), lk(), reserve(), size(), push_back(), matches(), callback(), THEMIS_WARN().

#### `ChangeEvent recordEvent(ChangeEvent event)`
- Source: `include/cdc/changefeed.h`:138
- Brief: Record Event.
- Parameters:
  - `event` (ChangeEvent): Input parameter.
- Return: Return value.
- Throws:
  - error::eventRecordFailed: if an error occurs.
- Details: event Input parameter. Return value. event Input parameter. Return value. error::eventRecordFailed if an error occurs. Calls: nextSequence(), std::chrono::system_clock::now(), time_since_epoch(), count(), toJson(), dump(), makeKey(), Put().

#### `RedactionResult redactByKeyPrefix(const std::string &key_prefix)`
- Source: `include/cdc/changefeed.h`:230
- Brief: Redact By Key Prefix.
- Parameters:
  - `key_prefix` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - error::invalidArgument: if an error occurs.
- Details: key_prefix Input parameter. Return value. key_prefix Input parameter. Return value. error::invalidArgument if an error occurs. Calls: empty(), reset(), NewIterator(), Seek(), makeKey(), Valid(), Next(), key().

#### `void retentionCleanupThread()`
- Source: `include/cdc/changefeed.h`:409
- Brief: Retention Cleanup Thread.
- Parameters: none
- Details: Calls: load(), applyRetentionPolicy(), plk(), compactByKey(), THEMIS_DEBUG(), lock(), wait_for(), THEMIS_ERROR().

#### `uint64_t scanMaxSequence() const`
- Source: `include/cdc/changefeed.h`:378
- Brief: Scan all stored changefeed events and return the maximum sequence number.
- Parameters: none
- Return: Return value.
- Details: Return value. Used as a crash-recovery fallback when loadInitialSequence() cannot read SEQUENCE_KEY directly.

#### `void startRetentionCleanup()`
- Source: `include/cdc/changefeed.h`:253
- Brief: Start Retention Cleanup.
- Parameters: none
- Details: Calls: exchange(), THEMIS_WARN(), std::thread(), lk(), THEMIS_INFO(), count().

#### `void stopRetentionCleanup()`
- Source: `include/cdc/changefeed.h`:258
- Brief: Stop Retention Cleanup.
- Parameters: none
- Details: Calls: exchange(), notify_all(), joinable(), join(), THEMIS_INFO().

#### `SubscriptionHandle subscribe(SubscriptionFilter filter, SubscriptionCallback callback)`
- Source: `include/cdc/changefeed.h`:336
- Brief: Subscribe.
- Parameters:
  - `filter` (SubscriptionFilter): Input parameter.
  - `callback` (SubscriptionCallback): Input parameter.
- Return: Return value.
- Details: filter Input parameter. callback Input parameter. Return value. filter Input parameter. callback Input parameter. Return value. Calls: fetch_add(), lk(), emplace(), std::move(), THEMIS_DEBUG().

#### `void unsubscribe(uint64_t subscription_id) noexcept`
- Source: `include/cdc/changefeed.h`:344
- Brief: Unsubscribe.
- Parameters:
  - `subscription_id` (uint64_t): Identifier of the subscription.
- Details: subscription_id Identifier of the subscription. Exception safety: noexcept.

#### `void updateRetentionPolicy(const RetentionPolicy &policy)`
- Source: `include/cdc/changefeed.h`:242
- Brief: Update Retention Policy.
- Parameters:
  - `policy` (const RetentionPolicy &): Input parameter.
- Details: policy Input parameter. policy Input parameter. Calls: lock(), THEMIS_INFO(), count(), startRetentionCleanup(), stopRetentionCleanup(), notify_all().

#### `bool waitForEvents(uint64_t from_sequence, uint32_t timeout_ms) const`
- Source: `include/cdc/changefeed.h`:386
- Brief: Helper to wait for new events (for long-poll).
- Parameters:
  - `from_sequence` (uint64_t): Input parameter.
  - `timeout_ms` (uint32_t): Input parameter.
- Return: True when the operation succeeds.
- Details: from_sequence Input parameter. timeout_ms Input parameter. True when the operation succeeds.

#### `~Changefeed() noexcept`
- Source: `include/cdc/changefeed.h`:131
- Brief: n/a
- Parameters: none

### themis::Changefeed::ChangeEvent

#### `ChangeEvent fromJson(const nlohmann::json &j)`
- Source: `include/cdc/changefeed.h`:76
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: value(), uint64_t(), contains(), is_null(), int64_t(), is_string().

#### `nlohmann::json toJson() const`
- Source: `include/cdc/changefeed.h`:70
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::Changefeed::RetentionPolicy

#### `RetentionPolicy defaults()`
- Source: `include/cdc/changefeed.h`:104
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

### themis::Changefeed::SubscriptionFilter

#### `bool matches(const ChangeEvent &ev) const noexcept`
- Source: `include/cdc/changefeed.h`:281
- Brief: Matches.
- Parameters:
  - `ev` (const ChangeEvent &): Input parameter.
- Return: True when the operation succeeds.
- Details: ev Input parameter. True when the operation succeeds. Exception safety: noexcept.

### themis::Changefeed::SubscriptionHandle

#### `SubscriptionHandle()=default`
- Source: `include/cdc/changefeed.h`:286
- Brief: n/a
- Parameters: none

#### `SubscriptionHandle(Changefeed *feed, uint64_t id) noexcept`
- Source: `include/cdc/changefeed.h`:321
- Brief: n/a
- Parameters:
  - `feed` (Changefeed *): n/a
  - `id` (uint64_t): n/a

#### `SubscriptionHandle(SubscriptionHandle &&other) noexcept`
- Source: `include/cdc/changefeed.h`:292
- Brief: n/a
- Parameters:
  - `other` (SubscriptionHandle &&): n/a

#### `SubscriptionHandle(const SubscriptionHandle &)=delete`
- Source: `include/cdc/changefeed.h`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SubscriptionHandle &): n/a

#### `bool active() const noexcept`
- Source: `include/cdc/changefeed.h`:315
- Brief: n/a
- Parameters: none

#### `void cancel() noexcept`
- Source: `include/cdc/changefeed.h`:313
- Brief: Cancel.
- Parameters: none
- Details: Exception safety: noexcept.

#### `uint64_t id() const noexcept`
- Source: `include/cdc/changefeed.h`:317
- Brief: n/a
- Parameters: none

#### `SubscriptionHandle & operator=(SubscriptionHandle &&other) noexcept`
- Source: `include/cdc/changefeed.h`:298
- Brief: n/a
- Parameters:
  - `other` (SubscriptionHandle &&): n/a

#### `SubscriptionHandle & operator=(const SubscriptionHandle &)=delete`
- Source: `include/cdc/changefeed.h`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SubscriptionHandle &): n/a

#### `~SubscriptionHandle() noexcept`
- Source: `include/cdc/changefeed.h`:287
- Brief: n/a
- Parameters: none

### themis::ChangefeedBuffer

#### `ChangefeedBuffer(Changefeed *changefeed, ChangefeedBufferConfig config=ChangefeedBufferConfig{})`
- Source: `include/cdc/changefeed_buffer.h`:109
- Brief: n/a
- Parameters:
  - `changefeed` (Changefeed *): n/a
  - `config` (ChangefeedBufferConfig): n/a

#### `ChangefeedBuffer(ChangefeedBuffer &&)=delete`
- Source: `include/cdc/changefeed_buffer.h`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBuffer &&): n/a

#### `ChangefeedBuffer(const ChangefeedBuffer &)=delete`
- Source: `include/cdc/changefeed_buffer.h`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ChangefeedBuffer &): n/a

#### `bool checkRateLimit()`
- Source: `include/cdc/changefeed_buffer.h`:286
- Brief: Check whether a user exceeds the current rate limit.
- Parameters: none
- Return: True when the user remains within the configured limit.
- Details: True when the user remains within the configured limit. True when the user remains within the configured limit. Calls: lock(), std::chrono::steady_clock::now(), store(), load(), count(), THEMIS_DEBUG(), std::this_thread::sleep_for().

#### `std::string compressPayload(const std::string &payload)`
- Source: `include/cdc/changefeed_buffer.h`:294
- Brief: Compress Payload.
- Parameters:
  - `payload` (const std::string &): Input parameter.
- Return: Return value.
- Details: payload Input parameter. Return value. payload Input parameter. Return value. Calls: utils::zstd_compress(), empty(), std::string(), begin(), end().

#### `std::string decompressPayload(const std::string &compressed)`
- Source: `include/cdc/changefeed_buffer.h`:300
- Brief: Decompress Payload.
- Parameters:
  - `compressed` (const std::string &): Input parameter.
- Return: Return value.
- Details: compressed Input parameter. Return value. compressed Input parameter. Return value. Calls: compressed_data(), begin(), end(), utils::zstd_decompress(), empty(), std::string().

#### `size_t flush()`
- Source: `include/cdc/changefeed_buffer.h`:141
- Brief: Flush.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: flushInternal().

#### `size_t flushBuffer(Changefeed::ChangeEventType event_type, EventTypeBuffer &buffer)`
- Source: `include/cdc/changefeed_buffer.h`:268
- Brief: Flush Buffer.
- Parameters:
  - `event_type` (Changefeed::ChangeEventType): Input parameter.
  - `buffer` (EventTypeBuffer &): Input/output parameter.
- Return: Return value.
- Details: event_type Input parameter. buffer Input/output parameter. Return value. event_type Input parameter. buffer Input/output parameter. Return value. Calls: CDC_MEASURE_LATENCY(), empty(), Tracer::startSpan(), setAttribute(), size(), contains(), has_value(), compressed_data().

#### `size_t flushFor(Changefeed::ChangeEventType event_type)`
- Source: `include/cdc/changefeed_buffer.h`:148
- Brief: Flush For.
- Parameters:
  - `event_type` (Changefeed::ChangeEventType): Input parameter.
- Return: Return value.
- Details: event_type Input parameter. Return value. event_type Input parameter. Return value. Calls: lock(), find(), end(), empty(), flushBuffer().

#### `size_t flushInternal(bool lock_held=false)`
- Source: `include/cdc/changefeed_buffer.h`:261
- Brief: Flush Internal.
- Parameters:
  - `lock_held` (bool): Input parameter.
- Return: Return value.
- Details: lock_held Input parameter. Return value. Calls: Tracer::startSpan(), lock(), empty(), flushBuffer(), std::chrono::steady_clock::now(), THEMIS_DEBUG(), size().

#### `void flushThread()`
- Source: `include/cdc/changefeed_buffer.h`:260
- Brief: Flush Thread.
- Parameters: none
- Details: Calls: THEMIS_INFO(), load(), lock(), wait_for(), shouldFlushGlobal(), unlock(), flushInternal(), THEMIS_DEBUG().

#### `const ChangefeedBufferConfig & getConfig() const`
- Source: `include/cdc/changefeed_buffer.h`:164
- Brief: n/a
- Parameters: none

#### `cdc::DeadLetterQueue * getDeadLetterQueue() const`
- Source: `include/cdc/changefeed_buffer.h`:181
- Brief: n/a
- Parameters: none

#### `const CDCMetrics & getMetrics() const`
- Source: `include/cdc/changefeed_buffer.h`:156
- Brief: n/a
- Parameters: none

#### `const ChangefeedBufferStats & getStats() const`
- Source: `include/cdc/changefeed_buffer.h`:154
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool isRunning() const`
- Source: `include/cdc/changefeed_buffer.h`:172
- Brief: n/a
- Parameters: none

#### `std::string makeBufferKey(const Changefeed::ChangeEvent &event) const`
- Source: `include/cdc/changefeed_buffer.h`:256
- Brief: Make Buffer Key.
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): Input parameter.
- Return: Return value.
- Details: event Input parameter. Return value.

#### `ChangefeedBuffer & operator=(ChangefeedBuffer &&)=delete`
- Source: `include/cdc/changefeed_buffer.h`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBuffer &&): n/a

#### `ChangefeedBuffer & operator=(const ChangefeedBuffer &)=delete`
- Source: `include/cdc/changefeed_buffer.h`:116
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ChangefeedBuffer &): n/a

#### `Changefeed::ChangeEvent recordEvent(Changefeed::ChangeEvent event)`
- Source: `include/cdc/changefeed_buffer.h`:135
- Brief: Record Event.
- Parameters:
  - `event` (Changefeed::ChangeEvent): Input parameter.
- Return: Return value.
- Details: event Input parameter. Return value. event Input parameter. Return value. Calls: CDC_MEASURE_LATENCY(), Tracer::startSpan(), setAttribute(), makeBufferKey(), empty(), THEMIS_WARN(), checkRateLimit(), has_value().

#### `void resetMetrics()`
- Source: `include/cdc/changefeed_buffer.h`:162
- Brief: Reset Metrics.
- Parameters: none
- Details: Calls: reset().

#### `void setConfig(const ChangefeedBufferConfig &config)`
- Source: `include/cdc/changefeed_buffer.h`:170
- Brief: Set Config.
- Parameters:
  - `config` (const ChangefeedBufferConfig &): Input parameter.
- Details: config Input parameter. config Input parameter. Calls: lock(), THEMIS_INFO(), count().

#### `void setDeadLetterQueue(cdc::DeadLetterQueue *dlq)`
- Source: `include/cdc/changefeed_buffer.h`:179
- Brief: Set Dead Letter Queue.
- Parameters:
  - `dlq` (cdc::DeadLetterQueue *): Input/output parameter.
- Details: dlq Input/output parameter. Implements setDeadLetterQueue without additional internal calls.

#### `bool shouldFlushBuffer(const EventTypeBuffer &buffer) const`
- Source: `include/cdc/changefeed_buffer.h`:274
- Brief: Should Flush Buffer.
- Parameters:
  - `buffer` (const EventTypeBuffer &): Input parameter.
- Return: True when the operation succeeds.
- Details: buffer Input parameter. True when the operation succeeds.

#### `bool shouldFlushGlobal() const`
- Source: `include/cdc/changefeed_buffer.h`:279
- Brief: Should Flush Global.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `void start()`
- Source: `include/cdc/changefeed_buffer.h`:123
- Brief: Start.
- Parameters: none
- Details: Calls: exchange(), THEMIS_WARN(), THEMIS_INFO(), count(), std::thread().

#### `void stop()`
- Source: `include/cdc/changefeed_buffer.h`:128
- Brief: Stop.
- Parameters: none
- Details: Calls: exchange(), THEMIS_INFO(), notify_all(), joinable(), join(), flush().

#### `~ChangefeedBuffer() noexcept`
- Source: `include/cdc/changefeed_buffer.h`:112
- Brief: n/a
- Parameters: none

### themis::ChangefeedBuffer::BufferedEvent

#### `BufferedEvent(const Changefeed::ChangeEvent &e)`
- Source: `include/cdc/changefeed_buffer.h`:193
- Brief: n/a
- Parameters:
  - `e` (const Changefeed::ChangeEvent &): n/a

### themis::ChangefeedBuffer::EventTypeBuffer

#### `void add(BufferedEvent &&event)`
- Source: `include/cdc/changefeed_buffer.h`:213
- Brief: Add.
- Parameters:
  - `event` (BufferedEvent &&): Input parameter.
- Details: event Input parameter. Calls: empty(), std::chrono::steady_clock::now(), push_back(), std::move().

#### `void clear()`
- Source: `include/cdc/changefeed_buffer.h`:225
- Brief: Clear.
- Parameters: none
- Details: Implements clear without additional internal calls.

### themis::ChangefeedBufferStats

#### `ChangefeedBufferStats()=default`
- Source: `include/cdc/changefeed_buffer.h`:104
- Brief: n/a
- Parameters: none

#### `ChangefeedBufferStats(ChangefeedBufferStats &&) noexcept=delete`
- Source: `include/cdc/changefeed_buffer.h`:102
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferStats &&): n/a

#### `ChangefeedBufferStats(const ChangefeedBufferStats &)=delete`
- Source: `include/cdc/changefeed_buffer.h`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ChangefeedBufferStats &): n/a

#### `ChangefeedBufferStats & operator=(ChangefeedBufferStats &&) noexcept=delete`
- Source: `include/cdc/changefeed_buffer.h`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChangefeedBufferStats &&): n/a

#### `ChangefeedBufferStats & operator=(const ChangefeedBufferStats &)=delete`
- Source: `include/cdc/changefeed_buffer.h`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ChangefeedBufferStats &): n/a

### themis::cdc

#### `Changefeed::ChangeEventType changeEventTypeFromString(const std::string &s)`
- Source: `src/cdc/outbox.cpp`:93
- Brief: Change Event Type From String.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Implements changeEventTypeFromString without additional internal calls.

#### `std::string changeEventTypeToString(Changefeed::ChangeEventType t)`
- Source: `src/cdc/outbox.cpp`:73
- Brief: Change Event Type To String.
- Parameters:
  - `t` (Changefeed::ChangeEventType): Input parameter.
- Return: Return value.
- Details: t Input parameter. Return value. Implements changeEventTypeToString without additional internal calls.

#### `std::string debeziumOpString(DebeziumOp op)`
- Source: `include/cdc/debezium_format.h`:79
- Brief: Debezium Op String.
- Parameters:
  - `op` (DebeziumOp): Input parameter.
- Return: Return value.
- Details: op Input parameter. Return value. Calls: std::string().

#### `bool isCdcFailClosedClass(CdcTransportFailureClass fc) noexcept`
- Source: `include/cdc/cdc_delivery_contract.h`:167
- Brief: Returns true when the transport failure class mandates fail-closed behaviour (dead-letter or halt, no automatic drop or advance).
- Parameters:
  - `fc` (CdcTransportFailureClass): Transport failure class to evaluate.
- Return: true if the event must be dead-lettered or delivery must halt.
- Details: fc Transport failure class to evaluate. true if the event must be dead-lettered or delivery must halt.

#### `OutboxState outboxStateFromString(const std::string &s)`
- Source: `src/cdc/outbox.cpp`:57
- Brief: Outbox State From String.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Implements outboxStateFromString without additional internal calls.

#### `std::string outboxStateToString(OutboxState s)`
- Source: `src/cdc/outbox.cpp`:39
- Brief: Outbox State To String.
- Parameters:
  - `s` (OutboxState): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Implements outboxStateToString without additional internal calls.

#### `std::string schemaFormatString(SchemaFormat fmt)`
- Source: `include/cdc/schema_registry.h`:79
- Brief: Schema Format String.
- Parameters:
  - `fmt` (SchemaFormat): Input parameter.
- Return: Return value.
- Details: fmt Input parameter. Return value. Implements schemaFormatString without additional internal calls.

### themis::cdc::AggregatedEvent

#### `nlohmann::json toJson() const`
- Source: `include/cdc/cross_collection_stream.h`:36
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::CDCAdmin

#### `CDCAdmin(Changefeed *changefeed)`
- Source: `include/cdc/cdc_admin.h`:178
- Brief: CDCAdmin.
- Parameters:
  - `changefeed` (Changefeed *): Input/output parameter.
- Return: Return value.
- Details: changefeed Input/output parameter. Return value.

#### `CDCAdmin(TenantBufferManager *tenant_manager)`
- Source: `include/cdc/cdc_admin.h`:185
- Brief: CDCAdmin.
- Parameters:
  - `tenant_manager` (TenantBufferManager *): Input/output parameter.
- Return: Return value.
- Details: tenant_manager Input/output parameter. Return value.

#### `CompactionResult compactLog()`
- Source: `include/cdc/cdc_admin.h`:243
- Brief: Compact Log.
- Parameters: none
- Return: Return value.
- Throws:
  - error::internalError: if an error occurs.
- Details: Return value. Return value. error::internalError if an error occurs. Calls: THEMIS_INFO(), steady_clock::now(), compactByKey(), count().

#### `uint64_t countEventsInRange(uint64_t start, uint64_t end)`
- Source: `include/cdc/cdc_admin.h`:288
- Brief: Count Events In Range.
- Parameters:
  - `start` (uint64_t): Input parameter.
  - `end` (uint64_t): Input parameter.
- Return: Return value.
- Details: start Input parameter. end Input parameter. Return value. start Input parameter. end Input parameter. Return value. Implements countEventsInRange without additional internal calls.

#### `DiagnosticsInfo getDiagnostics()`
- Source: `include/cdc/cdc_admin.h`:271
- Brief: Get Diagnostics.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: THEMIS_INFO(), getWatermarks(), healthCheck(), std::move().

#### `RetentionStatus getRetentionStatus()`
- Source: `include/cdc/cdc_admin.h`:257
- Brief: Get Retention Status.
- Parameters: none
- Return: Return value.
- Throws:
  - error::internalError: if an error occurs.
- Details: Return value. Return value. error::internalError if an error occurs. Calls: THEMIS_INFO(), getStats(), system_clock::now(), time_since_epoch(), count(), getRetentionPolicy(), isRetentionCleanupRunning().

#### `HealthStatus healthCheck()`
- Source: `include/cdc/cdc_admin.h`:265
- Brief: Health Check.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: getWatermarks(), std::string(), what().

#### `PurgeResult purgeAll()`
- Source: `include/cdc/cdc_admin.h`:199
- Brief: Purge All.
- Parameters: none
- Return: Return value.
- Throws:
  - error::internalError: if an error occurs.
- Details: Return value. Return value. error::internalError if an error occurs. Calls: THEMIS_INFO(), steady_clock::now(), getWatermarks(), deleteOldEvents(), count().

#### `PurgeResult purgeBySequenceRange(uint64_t start_sequence, uint64_t end_sequence)`
- Source: `include/cdc/cdc_admin.h`:207
- Brief: Purge By Sequence Range.
- Parameters:
  - `start_sequence` (uint64_t): Input parameter.
  - `end_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Throws:
  - error::internalError: if an error occurs.
- Details: start_sequence Input parameter. end_sequence Input parameter. Return value. start_sequence Input parameter. end_sequence Input parameter. Return value. error::internalError if an error occurs. Calls: THEMIS_INFO(), validateSequenceRange(), steady_clock::now(), deleteOldEvents(), count().

#### `PurgeResult purgeByTimestamp(uint64_t before_timestamp_ms)`
- Source: `include/cdc/cdc_admin.h`:214
- Brief: Purge By Timestamp.
- Parameters:
  - `before_timestamp_ms` (uint64_t): Input parameter.
- Return: Return value.
- Throws:
  - error::internalError: if an error occurs.
- Details: before_timestamp_ms Input parameter. Return value. before_timestamp_ms Input parameter. Return value. error::internalError if an error occurs. Calls: THEMIS_INFO(), steady_clock::now(), deleteOldEventsByTimestamp(), count().

#### `PurgeResult purgeOlderThan(int64_t before_timestamp_ms)`
- Source: `include/cdc/cdc_admin.h`:221
- Brief: Purge Older Than.
- Parameters:
  - `before_timestamp_ms` (int64_t): Input parameter.
- Return: Return value.
- Throws:
  - error::invalidArgument: if an error occurs.
- Details: before_timestamp_ms Input parameter. Return value. before_timestamp_ms Input parameter. Return value. error::invalidArgument if an error occurs. Calls: std::to_string(), purgeByTimestamp().

#### `PurgeResult purgeTenant(const std::string &tenant_id)`
- Source: `include/cdc/cdc_admin.h`:228
- Brief: Purge Tenant.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Return: Return value.
- Throws:
  - error::internalError: if an error occurs.
  - error::invalidArgument: if an error occurs.
- Details: tenant_id Identifier of the tenant. Return value. tenant_id Identifier of the tenant. Return value. error::internalError if an error occurs. error::invalidArgument if an error occurs. Calls: THEMIS_INFO(), empty(), steady_clock::now(), getTenantStats(), flushTenant(), removeTenant(), count().

#### `GDPRRedactionResult redactByKeyPrefix(const std::string &tenant_id, const std::string &key_prefix, const std::string &operator_id="")`
- Source: `include/cdc/cdc_admin.h`:247
- Brief: Redact By Key Prefix.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
  - `key_prefix` (const std::string &): Input parameter.
  - `operator_id` (const std::string &): Identifier of the operator.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. key_prefix Input parameter. operator_id Identifier of the operator. Return value.

#### `std::vector< Changefeed::ChangeEvent > replayFromSequence(uint64_t from_sequence, uint64_t limit=0, const std::set< Changefeed::ChangeEventType > &event_types={})`
- Source: `include/cdc/cdc_admin.h`:232
- Brief: Replay From Sequence.
- Parameters:
  - `from_sequence` (uint64_t): Input parameter.
  - `limit` (uint64_t): Input parameter.
  - `event_types` (const std::set< Changefeed::ChangeEventType > &): Input parameter.
- Return: Return value.
- Details: from_sequence Input parameter. limit Input parameter. event_types Input parameter. Return value.

#### `void setAuditStorage(RocksDBWrapper *storage) noexcept`
- Source: `include/cdc/cdc_admin.h`:189
- Brief: n/a
- Parameters:
  - `storage` (RocksDBWrapper *): n/a

#### `void setTransport(ICDCTransport *transport) noexcept`
- Source: `include/cdc/cdc_admin.h`:191
- Brief: n/a
- Parameters:
  - `transport` (ICDCTransport *): n/a

#### `void validateSequenceRange(uint64_t start, uint64_t end)`
- Source: `include/cdc/cdc_admin.h`:294
- Brief: Validate Sequence Range.
- Parameters:
  - `start` (uint64_t): Input parameter.
  - `end` (uint64_t): Input parameter.
- Throws:
  - error::invalidArgument: if an error occurs.
- Details: start Input parameter. end Input parameter. start Input parameter. end Input parameter. error::invalidArgument if an error occurs. Calls: std::to_string().

#### `~CDCAdmin()=default`
- Source: `include/cdc/cdc_admin.h`:187
- Brief: n/a
- Parameters: none

### themis::cdc::CDCException

#### `CDCException(ErrorCode code, ErrorSeverity severity, const std::string &message, const std::string &context="")`
- Source: `include/cdc/cdc_error.h`:107
- Brief: n/a
- Parameters:
  - `code` (ErrorCode): n/a
  - `severity` (ErrorSeverity): n/a
  - `message` (const std::string &): n/a
  - `context` (const std::string &): n/a

#### `ErrorCode code() const`
- Source: `include/cdc/cdc_error.h`:117
- Brief: n/a
- Parameters: none

#### `int codeValue() const`
- Source: `include/cdc/cdc_error.h`:122
- Brief: n/a
- Parameters: none

#### `const std::string & context() const`
- Source: `include/cdc/cdc_error.h`:120
- Brief: n/a
- Parameters: none

#### `std::string errorCodeToString(ErrorCode code)`
- Source: `include/cdc/cdc_error.h`:187
- Brief: Error Code To String.
- Parameters:
  - `code` (ErrorCode): Input parameter.
- Return: Return value.
- Details: code Input parameter. Return value. Implements errorCodeToString without additional internal calls.

#### `std::string formatMessage(ErrorCode code, ErrorSeverity severity, const std::string &message, const std::string &context)`
- Source: `include/cdc/cdc_error.h`:168
- Brief: Format Message.
- Parameters:
  - `code` (ErrorCode): Input parameter.
  - `severity` (ErrorSeverity): Input parameter.
  - `message` (const std::string &): Input parameter.
  - `context` (const std::string &): Input parameter.
- Return: Return value.
- Details: code Input parameter. severity Input parameter. message Input parameter. context Input parameter. Return value. Calls: errorCodeToString(), severityToString(), empty().

#### `bool isDataLossRisk() const`
- Source: `include/cdc/cdc_error.h`:147
- Brief: n/a
- Parameters: none

#### `bool isRetryable() const`
- Source: `include/cdc/cdc_error.h`:134
- Brief: n/a
- Parameters: none

#### `const std::string & message() const`
- Source: `include/cdc/cdc_error.h`:119
- Brief: n/a
- Parameters: none

#### `ErrorSeverity severity() const`
- Source: `include/cdc/cdc_error.h`:118
- Brief: n/a
- Parameters: none

#### `std::string severityToString(ErrorSeverity severity)`
- Source: `include/cdc/cdc_error.h`:234
- Brief: Severity To String.
- Parameters:
  - `severity` (ErrorSeverity): Input parameter.
- Return: Return value.
- Details: severity Input parameter. Return value. Implements severityToString without additional internal calls.

#### `nlohmann::json toJson() const`
- Source: `include/cdc/cdc_error.h`:124
- Brief: n/a
- Parameters: none

### themis::cdc::CDCMaterializedViewMaintainer

#### `CDCMaterializedViewMaintainer()`
- Source: `include/cdc/cdc_materialized_view.h`:39
- Brief: n/a
- Parameters: none

#### `CDCMaterializedViewMaintainer(const CDCMaterializedViewMaintainer &)=delete`
- Source: `include/cdc/cdc_materialized_view.h`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CDCMaterializedViewMaintainer &): n/a

#### `void applyEvent(const Changefeed::ChangeEvent &event)`
- Source: `include/cdc/cdc_materialized_view.h`:81
- Brief: - change ingestion -
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): Input parameter.
- Details: Apply Event. event Input parameter. event Input parameter. Calls: toChangeRecord(), empty(), applyChange().

#### `void applyEvents(const std::vector< Changefeed::ChangeEvent > &events)`
- Source: `include/cdc/cdc_materialized_view.h`:87
- Brief: Apply Events.
- Parameters:
  - `events` (const std::vector< Changefeed::ChangeEvent > &): Input parameter.
- Details: events Input parameter. events Input parameter. Calls: reserve(), size(), toChangeRecord(), empty(), push_back(), std::move(), applyChanges().

#### `bool createView(const themisdb::analytics::ViewDefinition &def)`
- Source: `include/cdc/cdc_materialized_view.h`:51
- Brief: - view lifecycle -
- Parameters:
  - `def` (const themisdb::analytics::ViewDefinition &): Input parameter.
- Return: True when the operation succeeds.
- Details: Create View. def Input parameter. True when the operation succeeds. def Input parameter. True when the operation succeeds. Implements createView without additional internal calls.

#### `bool dropView(const std::string &name)`
- Source: `include/cdc/cdc_materialized_view.h`:58
- Brief: Drop View.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: name Input parameter. True when the operation succeeds. name Input parameter. True when the operation succeeds. Implements dropView without additional internal calls.

#### `std::string extractCollection(const std::string &key)`
- Source: `include/cdc/cdc_materialized_view.h`:112
- Brief: Extract Collection.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. key Input parameter. Return value. Calls: find(), substr().

#### `std::shared_ptr< themisdb::analytics::IncrementalView > getView(const std::string &name) const`
- Source: `include/cdc/cdc_materialized_view.h`:74
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `bool hasView(const std::string &name) const`
- Source: `include/cdc/cdc_materialized_view.h`:65
- Brief: Has View.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: name Input parameter. True when the operation succeeds.

#### `std::vector< std::string > listViews() const`
- Source: `include/cdc/cdc_materialized_view.h`:71
- Brief: List Views.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `CDCMaterializedViewMaintainer & operator=(const CDCMaterializedViewMaintainer &)=delete`
- Source: `include/cdc/cdc_materialized_view.h`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CDCMaterializedViewMaintainer &): n/a

#### `themisdb::analytics::ChangeRecord::Row parseJsonRow(const std::string &json_str)`
- Source: `include/cdc/cdc_materialized_view.h`:115
- Brief: Parse Json Row.
- Parameters:
  - `json_str` (const std::string &): Input parameter.
- Return: Return value.
- Details: json_str Input parameter. Return value. Calls: empty(), nlohmann::json::parse(), is_object(), begin(), end(), value(), is_null(), key().

#### `themisdb::analytics::ViewQueryResult query(const std::string &view_name, const std::vector< themisdb::analytics::ViewFilter > &filters={}, int64_t limit=0, int64_t offset=0) const`
- Source: `include/cdc/cdc_materialized_view.h`:91
- Brief: n/a
- Parameters:
  - `view_name` (const std::string &): n/a
  - `filters` (const std::vector< themisdb::analytics::ViewFilter > &): n/a
  - `limit` (int64_t): n/a
  - `offset` (int64_t): n/a

#### `themisdb::analytics::ChangeRecord toChangeRecord(const Changefeed::ChangeEvent &event)`
- Source: `include/cdc/cdc_materialized_view.h`:105
- Brief: To Change Record.
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): Input parameter.
- Return: Return value.
- Details: event Input parameter. Return value. Calls: extractCollection(), has_value(), parseJsonRow().

#### `uint64_t totalEventsProcessed() const`
- Source: `include/cdc/cdc_materialized_view.h`:98
- Brief: n/a
- Parameters: none

#### `~CDCMaterializedViewMaintainer()`
- Source: `include/cdc/cdc_materialized_view.h`:40
- Brief: n/a
- Parameters: none

### themis::cdc::CDCMetrics

#### `void reset()`
- Source: `include/cdc/cdc_metrics.h`:299
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Implements reset without additional internal calls.

#### `nlohmann::json toJson() const`
- Source: `include/cdc/cdc_metrics.h`:270
- Brief: n/a
- Parameters: none

### themis::cdc::CdcSchemaEncoder

#### `CdcSchemaEncoder(SchemaRegistryClient *client)`
- Source: `include/cdc/schema_registry.h`:493
- Brief: n/a
- Parameters:
  - `client` (SchemaRegistryClient *): n/a

#### `CdcSchemaEncoder(const CdcSchemaEncoder &)=delete`
- Source: `include/cdc/schema_registry.h`:497
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CdcSchemaEncoder &): n/a

#### `std::vector< uint8_t > buildWireFormat(int32_t schema_id, const std::vector< uint8_t > &payload)`
- Source: `include/cdc/schema_registry.h`:701
- Brief: n/a
- Parameters:
  - `schema_id` (int32_t): n/a
  - `payload` (const std::vector< uint8_t > &): n/a

#### `void clearLocalCache()`
- Source: `include/cdc/schema_registry.h`:620
- Brief: n/a
- Parameters: none

#### `std::string collectionFromKey(const std::string &key)`
- Source: `include/cdc/schema_registry.h`:636
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::optional< nlohmann::json > decodeToJson(const std::vector< uint8_t > &wire_bytes) const`
- Source: `include/cdc/schema_registry.h`:547
- Brief: n/a
- Parameters:
  - `wire_bytes` (const std::vector< uint8_t > &): n/a

#### `std::string defaultAvroSchema(const std::string &collection)`
- Source: `include/cdc/schema_registry.h`:425
- Brief: ── Default schema templates ───────────────────────────────────────────
- Parameters:
  - `collection` (const std::string &): Input parameter.
- Return: Return value.
- Details: collection Input parameter. Return value. Calls: nlohmann::json::array(), dump().

#### `std::string defaultJsonSchema(const std::string &collection)`
- Source: `include/cdc/schema_registry.h`:465
- Brief: Default Json Schema.
- Parameters:
  - `collection` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: collection Input parameter. Return value. std::runtime_error if an error occurs. Calls: nlohmann::json::array(), dump(), defaultProtobufSchema(), std::string(), CdcSchemaEncoder(), client_(), setAvroEncoderFn(), std::move().

#### `std::string defaultProtobufSchema(const std::string &collection)`
- Source: `include/cdc/schema_registry.h`:487
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a

#### `EncodedEvent encode(const Changefeed::ChangeEvent &event, const std::string &collection="") const`
- Source: `include/cdc/schema_registry.h`:515
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a
  - `collection` (const std::string &): n/a

#### `int32_t ensureCollectionSchema(const std::string &collection) const`
- Source: `include/cdc/schema_registry.h`:580
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a

#### `nlohmann::json eventToPayload(const Changefeed::ChangeEvent &event, const std::string &collection)`
- Source: `include/cdc/schema_registry.h`:660
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a
  - `collection` (const std::string &): n/a

#### `int32_t extractSchemaId(const std::vector< uint8_t > &wire_bytes)`
- Source: `include/cdc/schema_registry.h`:565
- Brief: n/a
- Parameters:
  - `wire_bytes` (const std::vector< uint8_t > &): n/a

#### `std::string operationString(Changefeed::ChangeEventType type)`
- Source: `include/cdc/schema_registry.h`:645
- Brief: n/a
- Parameters:
  - `type` (Changefeed::ChangeEventType): n/a

#### `CdcSchemaEncoder & operator=(const CdcSchemaEncoder &)=delete`
- Source: `include/cdc/schema_registry.h`:498
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CdcSchemaEncoder &): n/a

#### `void setAvroEncoderFn(BinaryEncoderFn fn)`
- Source: `include/cdc/schema_registry.h`:505
- Brief: n/a
- Parameters:
  - `fn` (BinaryEncoderFn): n/a

#### `void setProtobufEncoderFn(BinaryEncoderFn fn)`
- Source: `include/cdc/schema_registry.h`:509
- Brief: n/a
- Parameters:
  - `fn` (BinaryEncoderFn): n/a

#### `std::string subjectForCollection(const std::string &collection) const`
- Source: `include/cdc/schema_registry.h`:641
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a

### themis::cdc::CdcWebSocketHandler

#### `CdcWebSocketHandler(size_t max_pending_ack=kMaxPendingAck, ConsumerGroupManager *group_manager=nullptr)`
- Source: `include/cdc/cdc_ws_handler.h`:34
- Brief: n/a
- Parameters:
  - `max_pending_ack` (size_t): n/a
  - `group_manager` (ConsumerGroupManager *): n/a

#### `nlohmann::json buildEventFrame(const Changefeed::ChangeEvent &ev, const std::string &sub_id)`
- Source: `include/cdc/cdc_ws_handler.h`:92
- Brief: Build Event Frame.
- Parameters:
  - `ev` (const Changefeed::ChangeEvent &): Input parameter.
  - `sub_id` (const std::string &): Identifier of the sub.
- Return: Return value.
- Details: ev Input parameter. sub_id Identifier of the sub. Return value. ev Input parameter. sub_id Identifier of the sub. Return value. Calls: toJson().

#### `std::vector< nlohmann::json > checkRedelivery()`
- Source: `include/cdc/cdc_ws_handler.h`:56
- Brief: Check Redelivery.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: std::chrono::steady_clock::now(), std::chrono::milliseconds(), lock(), empty(), front(), THEMIS_INFO(), size(), count().

#### `uint64_t getWsOverflowTotal() const`
- Source: `include/cdc/cdc_ws_handler.h`:58
- Brief: n/a
- Parameters: none

#### `std::vector< nlohmann::json > handleFrame(const nlohmann::json &frame)`
- Source: `include/cdc/cdc_ws_handler.h`:43
- Brief: Handle Frame.
- Parameters:
  - `frame` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: frame Input parameter. Return value. frame Input parameter. Return value. Calls: value(), empty(), push_back(), contains(), is_string(), is_array(), insert(), getCommittedOffset().

#### `bool hasSubscriptions() const`
- Source: `include/cdc/cdc_ws_handler.h`:66
- Brief: Has Subscriptions.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `std::vector< nlohmann::json > pollEvents(Changefeed &feed)`
- Source: `include/cdc/cdc_ws_handler.h`:50
- Brief: Poll Events.
- Parameters:
  - `feed` (Changefeed &): Input/output parameter.
- Return: Return value.
- Details: feed Input/output parameter. Return value. feed Input/output parameter. Return value. Calls: lock(), size(), fetch_add(), THEMIS_WARN(), load(), empty(), listEvents(), std::chrono::steady_clock::now().

### themis::cdc::ChangeStreamCompressor

#### `ChangeStreamCompressor(Config config=Config{})`
- Source: `include/cdc/change_stream_compressor.h`:173
- Brief: Change Stream Compressor.
- Parameters:
  - `config` (Config): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `ChangeStreamCompressor(const ChangeStreamCompressor &)=delete`
- Source: `include/cdc/change_stream_compressor.h`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ChangeStreamCompressor &): n/a

#### `CompressedBatch compress(const std::vector< Changefeed::ChangeEvent > &events)`
- Source: `include/cdc/change_stream_compressor.h`:186
- Brief: ── Compression ───────────────────────────────────────────────────────────
- Parameters:
  - `events` (const std::vector< Changefeed::ChangeEvent > &): Input parameter.
- Return: Return value.
- Details: events Input parameter. Return value. Calls: lk(), nlohmann::json::array(), push_back(), toJson(), dump(), size(), fetch_add(), utils::zstd_compress().

#### `std::vector< Changefeed::ChangeEvent > decompress(const CompressedBatch &batch)`
- Source: `include/cdc/change_stream_compressor.h`:238
- Brief: ── Decompression ─────────────────────────────────────────────────────────
- Parameters:
  - `batch` (const CompressedBatch &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: batch Input parameter. Return value. std::runtime_error if an error occurs. Calls: fetch_add(), utils::zstd_decompress(), empty(), assign(), begin(), end(), reserve(), nlohmann::json::parse().

#### `Config getConfig() const`
- Source: `include/cdc/change_stream_compressor.h`:302
- Brief: n/a
- Parameters: none

#### `Stats getStats() const noexcept`
- Source: `include/cdc/change_stream_compressor.h`:278
- Brief: n/a
- Parameters: none

#### `ChangeStreamCompressor & operator=(const ChangeStreamCompressor &)=delete`
- Source: `include/cdc/change_stream_compressor.h`:177
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ChangeStreamCompressor &): n/a

#### `void resetStats() noexcept`
- Source: `include/cdc/change_stream_compressor.h`:290
- Brief: n/a
- Parameters: none

#### `void setConfig(const Config &config)`
- Source: `include/cdc/change_stream_compressor.h`:317
- Brief: Set Config.
- Parameters:
  - `config` (const Config &): Input parameter.
- Details: config Input parameter. Calls: lock().

### themis::cdc::ChangeStreamCompressor::Stats

#### `double compression_ratio() const noexcept`
- Source: `include/cdc/change_stream_compressor.h`:165
- Brief: n/a
- Parameters: none

### themis::cdc::CompressedBatch

#### `std::optional< CompressedBatch > deserialize(const std::vector< uint8_t > &bytes)`
- Source: `include/cdc/change_stream_compressor.h`:114
- Brief: Deserialize.
- Parameters:
  - `bytes` (const std::vector< uint8_t > &): Input parameter.
- Return: Return value.
- Details: bytes Input parameter. Return value. Calls: size(), assign(), begin(), end().

#### `std::vector< uint8_t > serialize() const`
- Source: `include/cdc/change_stream_compressor.h`:81
- Brief: n/a
- Parameters: none

### themis::cdc::ConsumerGroupConfig

#### `ConsumerGroupConfig fromJson(const nlohmann::json &j)`
- Source: `include/cdc/consumer_group.h`:70
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: value().

#### `nlohmann::json toJson() const`
- Source: `include/cdc/consumer_group.h`:60
- Brief: n/a
- Parameters: none

### themis::cdc::ConsumerGroupInfo

#### `nlohmann::json toJson() const`
- Source: `include/cdc/consumer_group.h`:82
- Brief: n/a
- Parameters: none

### themis::cdc::ConsumerGroupManager

#### `ConsumerGroupManager(ConsumerGroupManager &&)=delete`
- Source: `include/cdc/consumer_group.h`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupManager &&): n/a

#### `ConsumerGroupManager(const ConsumerGroupManager &)=delete`
- Source: `include/cdc/consumer_group.h`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ConsumerGroupManager &): n/a

#### `ConsumerGroupManager(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf=nullptr)`
- Source: `include/cdc/consumer_group.h`:104
- Brief: n/a
- Parameters:
  - `db` (rocksdb::TransactionDB *): n/a
  - `cf` (rocksdb::ColumnFamilyHandle *): n/a

#### `void acknowledgeEvents(const std::string &group_id, const std::string &consumer_id, uint64_t up_to_sequence)`
- Source: `include/cdc/consumer_group.h`:236
- Brief: Acknowledge Events.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
  - `consumer_id` (const std::string &): Identifier of the consumer.
  - `up_to_sequence` (uint64_t): Input parameter.
- Throws:
  - error::invalidArgument: if an error occurs.
- Details: group_id Identifier of the group. consumer_id Identifier of the consumer. up_to_sequence Input parameter. group_id Identifier of the group. consumer_id Identifier of the consumer. up_to_sequence Input parameter. error::invalidArgument if an error occurs. Calls: empty(), lock(), readConfigLocked(), find(), end(), erase(), std::remove_if(), begin().

#### `void commitOffset(const std::string &group_id, uint64_t sequence)`
- Source: `include/cdc/consumer_group.h`:174
- Brief: Commit Offset.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
  - `sequence` (uint64_t): Input parameter.
- Throws:
  - error::invalidArgument: if an error occurs.
- Details: group_id Identifier of the group. sequence Input parameter. group_id Identifier of the group. sequence Input parameter. error::invalidArgument if an error occurs. Calls: empty(), lock(), readConfigLocked(), readOffsetLocked(), writeOffsetLocked(), THEMIS_DEBUG().

#### `bool consumerHandlesKey(const std::string &group_id, const std::string &consumer_id, const std::string &event_key) const`
- Source: `include/cdc/consumer_group.h`:196
- Brief: Consumer Handles Key.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
  - `consumer_id` (const std::string &): Identifier of the consumer.
  - `event_key` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: group_id Identifier of the group. consumer_id Identifier of the consumer. event_key Input parameter. True when the operation succeeds.

#### `void createGroup(const ConsumerGroupConfig &config)`
- Source: `include/cdc/consumer_group.h`:123
- Brief: Create Group.
- Parameters:
  - `config` (const ConsumerGroupConfig &): Input parameter.
- Throws:
  - error::invalidArgument: if an error occurs.
- Details: config Input parameter. config Input parameter. error::invalidArgument if an error occurs. Calls: empty(), lock(), writeConfigLocked(), THEMIS_INFO().

#### `void deleteGroup(const std::string &group_id)`
- Source: `include/cdc/consumer_group.h`:129
- Brief: Delete Group.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
- Throws:
  - error::invalidArgument: if an error occurs.
  - error::dbOperationFailed: if an error occurs.
- Details: group_id Identifier of the group. group_id Identifier of the group. error::invalidArgument if an error occurs. error::dbOperationFailed if an error occurs. Calls: empty(), lock(), Delete(), makeConfigKey(), makeOffsetKey(), ok(), IsNotFound(), ToString().

#### `std::vector< Changefeed::ChangeEvent > fetchEvents(const std::string &group_id, const std::string &consumer_id, const Changefeed &changefeed, size_t limit=100) const`
- Source: `include/cdc/consumer_group.h`:213
- Brief: n/a
- Parameters:
  - `group_id` (const std::string &): n/a
  - `consumer_id` (const std::string &): n/a
  - `changefeed` (const Changefeed &): n/a
  - `limit` (size_t): n/a

#### `std::vector< Changefeed::ChangeEvent > fetchEventsAtLeastOnce(const std::string &group_id, const std::string &consumer_id, const Changefeed &changefeed, size_t limit=100, uint32_t ack_timeout_ms=30000)`
- Source: `include/cdc/consumer_group.h`:223
- Brief: n/a
- Parameters:
  - `group_id` (const std::string &): n/a
  - `consumer_id` (const std::string &): n/a
  - `changefeed` (const Changefeed &): n/a
  - `limit` (size_t): n/a
  - `ack_timeout_ms` (uint32_t): n/a

#### `uint32_t fnv1a32(const std::string &s)`
- Source: `include/cdc/consumer_group.h`:259
- Brief: ----------------------------------------------------- Static helpers (also useful for unit testing) -----------------------------------------------------
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: Fnv1a32. s Input parameter. Return value. s Input parameter. Return value. Implements fnv1a32 without additional internal calls.

#### `uint64_t getCommittedOffset(const std::string &group_id) const`
- Source: `include/cdc/consumer_group.h`:167
- Brief: Get Committed Offset.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
- Return: Return value.
- Details: group_id Identifier of the group. Return value.

#### `uint32_t getConsumerPartition(const std::string &group_id, const std::string &consumer_id) const`
- Source: `include/cdc/consumer_group.h`:186
- Brief: Get Consumer Partition.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
  - `consumer_id` (const std::string &): Identifier of the consumer.
- Return: Return value.
- Details: group_id Identifier of the group. consumer_id Identifier of the consumer. Return value.

#### `ConsumerGroupConfig getGroupConfig(const std::string &group_id) const`
- Source: `include/cdc/consumer_group.h`:143
- Brief: Get Group Config.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
- Return: Return value.
- Details: group_id Identifier of the group. Return value.

#### `ConsumerGroupInfo getGroupInfo(const std::string &group_id) const`
- Source: `include/cdc/consumer_group.h`:150
- Brief: Get Group Info.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
- Return: Return value.
- Details: group_id Identifier of the group. Return value.

#### `size_t getInFlightCount(const std::string &group_id, const std::string &consumer_id) const`
- Source: `include/cdc/consumer_group.h`:246
- Brief: Get In Flight Count.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
  - `consumer_id` (const std::string &): Identifier of the consumer.
- Return: Return value.
- Details: group_id Identifier of the group. consumer_id Identifier of the consumer. Return value.

#### `InFlightStats getInFlightStats(const std::string &group_id, const std::string &consumer_id, uint32_t ack_timeout_ms=30000) const`
- Source: `include/cdc/consumer_group.h`:249
- Brief: n/a
- Parameters:
  - `group_id` (const std::string &): n/a
  - `consumer_id` (const std::string &): n/a
  - `ack_timeout_ms` (uint32_t): n/a

#### `uint32_t getPartitionForKey(const std::string &group_id, const std::string &key) const`
- Source: `include/cdc/consumer_group.h`:206
- Brief: Get Partition For Key.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: group_id Identifier of the group. key Input parameter. Return value.

#### `bool groupExists(const std::string &group_id) const`
- Source: `include/cdc/consumer_group.h`:136
- Brief: Group Exists.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
- Return: True when the operation succeeds.
- Details: group_id Identifier of the group. True when the operation succeeds.

#### `std::vector< std::string > listGroups() const`
- Source: `include/cdc/consumer_group.h`:156
- Brief: List Groups.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string makeConfigKey(const std::string &group_id) const`
- Source: `include/cdc/consumer_group.h`:293
- Brief: Make Config Key.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
- Return: Return value.
- Details: group_id Identifier of the group. Return value.

#### `std::string makeOffsetKey(const std::string &group_id) const`
- Source: `include/cdc/consumer_group.h`:299
- Brief: Make Offset Key.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
- Return: Return value.
- Details: group_id Identifier of the group. Return value.

#### `ConsumerGroupManager & operator=(ConsumerGroupManager &&)=delete`
- Source: `include/cdc/consumer_group.h`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConsumerGroupManager &&): n/a

#### `ConsumerGroupManager & operator=(const ConsumerGroupManager &)=delete`
- Source: `include/cdc/consumer_group.h`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ConsumerGroupManager &): n/a

#### `uint32_t partitionForConsumer(const std::string &consumer_id, uint32_t partition_count)`
- Source: `include/cdc/consumer_group.h`:276
- Brief: Partition For Consumer.
- Parameters:
  - `consumer_id` (const std::string &): Identifier of the consumer.
  - `partition_count` (uint32_t): Input parameter.
- Return: Return value.
- Details: consumer_id Identifier of the consumer. partition_count Input parameter. Return value. consumer_id Identifier of the consumer. partition_count Input parameter. Return value. Calls: fnv1a32().

#### `uint32_t partitionForKey(const std::string &key, uint32_t partition_count)`
- Source: `include/cdc/consumer_group.h`:267
- Brief: Partition For Key.
- Parameters:
  - `key` (const std::string &): Input parameter.
  - `partition_count` (uint32_t): Input parameter.
- Return: Return value.
- Details: key Input parameter. partition_count Input parameter. Return value. key Input parameter. partition_count Input parameter. Return value. Calls: fnv1a32().

#### `ConsumerGroupConfig readConfigLocked(const std::string &group_id) const`
- Source: `include/cdc/consumer_group.h`:306
- Brief: Internal helpers (caller must hold mutex_).
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
- Return: Return value.
- Details: group_id Identifier of the group. Return value.

#### `uint64_t readOffsetLocked(const std::string &group_id) const`
- Source: `include/cdc/consumer_group.h`:312
- Brief: Read Offset Locked.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
- Return: Return value.
- Details: group_id Identifier of the group. Return value.

#### `void writeConfigLocked(const ConsumerGroupConfig &config)`
- Source: `include/cdc/consumer_group.h`:317
- Brief: Write Config Locked.
- Parameters:
  - `config` (const ConsumerGroupConfig &): Input parameter.
- Throws:
  - error::dbOperationFailed: if an error occurs.
- Details: config Input parameter. config Input parameter. error::dbOperationFailed if an error occurs. Calls: toJson(), dump(), Put(), makeConfigKey(), ok(), ToString().

#### `void writeOffsetLocked(const std::string &group_id, uint64_t sequence)`
- Source: `include/cdc/consumer_group.h`:323
- Brief: Write Offset Locked.
- Parameters:
  - `group_id` (const std::string &): Identifier of the group.
  - `sequence` (uint64_t): Input parameter.
- Throws:
  - CDCException: if an error occurs.
- Details: group_id Identifier of the group. sequence Input parameter. group_id Identifier of the group. sequence Input parameter. CDCException if an error occurs. Calls: std::to_string(), Put(), makeOffsetKey(), ok(), ToString().

#### `~ConsumerGroupManager()=default`
- Source: `include/cdc/consumer_group.h`:107
- Brief: n/a
- Parameters: none

### themis::cdc::CrossCollectionStream

#### `CrossCollectionStream()=default`
- Source: `include/cdc/cross_collection_stream.h`:60
- Brief: n/a
- Parameters: none

#### `CrossCollectionStream(CrossCollectionStream &&)=delete`
- Source: `include/cdc/cross_collection_stream.h`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStream &&): n/a

#### `CrossCollectionStream(const CrossCollectionStream &)=delete`
- Source: `include/cdc/cross_collection_stream.h`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CrossCollectionStream &): n/a

#### `void addCollection(const std::string &name, Changefeed *feed)`
- Source: `include/cdc/cross_collection_stream.h`:79
- Brief: Add Collection.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `feed` (Changefeed *): Input/output parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: name Input parameter. feed Input/output parameter. name Input parameter. feed Input/output parameter. std::invalid_argument if an error occurs. Calls: empty(), lock(), THEMIS_DEBUG().

#### `size_t collectionCount() const`
- Source: `include/cdc/cross_collection_stream.h`:98
- Brief: Collection Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `uint64_t getHighWatermark(const std::set< std::string > &collection_names={}) const`
- Source: `include/cdc/cross_collection_stream.h`:117
- Brief: n/a
- Parameters:
  - `collection_names` (const std::set< std::string > &): n/a

#### `bool hasCollection(const std::string &name) const`
- Source: `include/cdc/cross_collection_stream.h`:92
- Brief: Has Collection.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: name Input parameter. True when the operation succeeds.

#### `std::vector< std::string > listCollections() const`
- Source: `include/cdc/cross_collection_stream.h`:104
- Brief: List Collections.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< AggregatedEvent > listEvents(const StreamOptions &options=StreamOptions::defaults()) const`
- Source: `include/cdc/cross_collection_stream.h`:110
- Brief: n/a
- Parameters:
  - `options` (const StreamOptions &): n/a

#### `std::vector< AggregatedEvent > listEventsFor(const std::set< std::string > &collection_names, size_t limit=100) const`
- Source: `include/cdc/cross_collection_stream.h`:113
- Brief: n/a
- Parameters:
  - `collection_names` (const std::set< std::string > &): n/a
  - `limit` (size_t): n/a

#### `CrossCollectionStream & operator=(CrossCollectionStream &&)=delete`
- Source: `include/cdc/cross_collection_stream.h`:68
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossCollectionStream &&): n/a

#### `CrossCollectionStream & operator=(const CrossCollectionStream &)=delete`
- Source: `include/cdc/cross_collection_stream.h`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CrossCollectionStream &): n/a

#### `void removeCollection(const std::string &name)`
- Source: `include/cdc/cross_collection_stream.h`:85
- Brief: Remove Collection.
- Parameters:
  - `name` (const std::string &): Input parameter.
- Details: name Input parameter. name Input parameter. Calls: lock(), erase().

#### `~CrossCollectionStream()=default`
- Source: `include/cdc/cross_collection_stream.h`:61
- Brief: n/a
- Parameters: none

### themis::cdc::CrossCollectionStream::StreamOptions

#### `StreamOptions defaults()`
- Source: `include/cdc/cross_collection_stream.h`:57
- Brief: Defaults.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaults without additional internal calls.

### themis::cdc::DLQEntry

#### `DLQEntry fromJson(const nlohmann::json &j)`
- Source: `include/cdc/dead_letter_queue.h`:62
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: value(), uint64_t(), int64_t(), contains().

#### `nlohmann::json toJson() const`
- Source: `include/cdc/dead_letter_queue.h`:56
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::DeadLetterQueue

#### `DeadLetterQueue(DeadLetterQueue &&)=delete`
- Source: `include/cdc/dead_letter_queue.h`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueue &&): n/a

#### `DeadLetterQueue(const DeadLetterQueue &)=delete`
- Source: `include/cdc/dead_letter_queue.h`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DeadLetterQueue &): n/a

#### `DeadLetterQueue(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf=nullptr)`
- Source: `include/cdc/dead_letter_queue.h`:67
- Brief: n/a
- Parameters:
  - `db` (rocksdb::TransactionDB *): n/a
  - `cf` (rocksdb::ColumnFamilyHandle *): n/a

#### `size_t drain()`
- Source: `include/cdc/dead_letter_queue.h`:113
- Brief: Drain.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: reset(), NewIterator(), makeKey(), Seek(), Valid(), key(), ToString(), compare().

#### `DLQEntry enqueue(const Changefeed::ChangeEvent &event, const std::string &failure_reason, int attempt_count)`
- Source: `include/cdc/dead_letter_queue.h`:89
- Brief: Enqueue.
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): Input parameter.
  - `failure_reason` (const std::string &): Input parameter.
  - `attempt_count` (int): Input parameter.
- Return: Return value.
- Throws:
  - error::dbOperationFailed: if an error occurs.
- Details: event Input parameter. failure_reason Input parameter. attempt_count Input parameter. Return value. event Input parameter. failure_reason Input parameter. attempt_count Input parameter. Return value. error::dbOperationFailed if an error occurs. Calls: nextSequence(), std::chrono::system_clock::now(), time_since_epoch(), count(), makeKey(), toJson(), dump(), Put().

#### `DLQEntry getEntry(uint64_t dlq_sequence) const`
- Source: `include/cdc/dead_letter_queue.h`:126
- Brief: Get Entry.
- Parameters:
  - `dlq_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: dlq_sequence Input parameter. Return value.

#### `std::vector< DLQEntry > listEntries(size_t limit=0) const`
- Source: `include/cdc/dead_letter_queue.h`:119
- Brief: n/a
- Parameters:
  - `limit` (size_t): n/a

#### `std::string makeKey(uint64_t dlq_sequence) const`
- Source: `include/cdc/dead_letter_queue.h`:148
- Brief: Make Key.
- Parameters:
  - `dlq_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: dlq_sequence Input parameter. Return value.

#### `uint64_t nextSequence()`
- Source: `include/cdc/dead_letter_queue.h`:153
- Brief: Next Sequence.
- Parameters: none
- Return: Return value.
- Throws:
  - error::sequenceGenerationFailed: if an error occurs.
- Details: Return value. Return value. error::sequenceGenerationFailed if an error occurs. Calls: lock(), Get(), ok(), empty(), std::stoull(), std::to_string(), Put(), ToString().

#### `DeadLetterQueue & operator=(DeadLetterQueue &&)=delete`
- Source: `include/cdc/dead_letter_queue.h`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (DeadLetterQueue &&): n/a

#### `DeadLetterQueue & operator=(const DeadLetterQueue &)=delete`
- Source: `include/cdc/dead_letter_queue.h`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DeadLetterQueue &): n/a

#### `bool remove(uint64_t dlq_sequence)`
- Source: `include/cdc/dead_letter_queue.h`:107
- Brief: Remove.
- Parameters:
  - `dlq_sequence` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: dlq_sequence Input parameter. True when the operation succeeds. dlq_sequence Input parameter. True when the operation succeeds. Calls: makeKey(), Get(), IsNotFound(), ok(), THEMIS_ERROR(), ToString(), Delete(), THEMIS_DEBUG().

#### `Changefeed::ChangeEvent replay(uint64_t dlq_sequence, Changefeed &changefeed)`
- Source: `include/cdc/dead_letter_queue.h`:99
- Brief: Replay.
- Parameters:
  - `dlq_sequence` (uint64_t): Input parameter.
  - `changefeed` (Changefeed &): Input/output parameter.
- Return: Return value.
- Details: dlq_sequence Input parameter. changefeed Input/output parameter. Return value. dlq_sequence Input parameter. changefeed Input/output parameter. Return value. Calls: getEntry(), recordEvent(), remove(), THEMIS_WARN(), THEMIS_INFO().

#### `size_t size() const`
- Source: `include/cdc/dead_letter_queue.h`:132
- Brief: Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `~DeadLetterQueue()=default`
- Source: `include/cdc/dead_letter_queue.h`:70
- Brief: n/a
- Parameters: none

### themis::cdc::DebeziumEnvelope

#### `nlohmann::json buildSchema(const std::string &server_name, const std::string &table)`
- Source: `include/cdc/debezium_format.h`:142
- Brief: Build Schema.
- Parameters:
  - `server_name` (const std::string &): Name of the server.
  - `table` (const std::string &): Input parameter.
- Return: Return value.
- Details: server_name Name of the server. table Input parameter. Return value. Calls: nlohmann::json::array(), makeValueField().

#### `nlohmann::json toJson(bool include_schema=false) const`
- Source: `include/cdc/debezium_format.h`:114
- Brief: n/a
- Parameters:
  - `include_schema` (bool): n/a

### themis::cdc::DebeziumFormatter

#### `DebeziumFormatter()=default`
- Source: `include/cdc/debezium_format.h`:205
- Brief: Debezium Formatter.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `DebeziumFormatter(Config cfg)`
- Source: `include/cdc/debezium_format.h`:206
- Brief: n/a
- Parameters:
  - `cfg` (Config): n/a

#### `std::string collectionFromKey(const std::string &key)`
- Source: `include/cdc/debezium_format.h`:289
- Brief: Collection From Key.
- Parameters:
  - `key` (const std::string &): Input parameter.
- Return: Return value.
- Details: key Input parameter. Return value. Calls: find(), substr().

#### `DebeziumOp opFromEvent(const Changefeed::ChangeEvent &event)`
- Source: `include/cdc/debezium_format.h`:270
- Brief: Op From Event.
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): Input parameter.
- Return: Return value.
- Details: event Input parameter. Return value. Calls: has_value().

#### `nlohmann::json parseDocument(const std::string &s)`
- Source: `include/cdc/debezium_format.h`:300
- Brief: Parse Document.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: nlohmann::json::parse().

#### `DebeziumEnvelope toEnvelope(const Changefeed::ChangeEvent &event, const std::string &collection="") const`
- Source: `include/cdc/debezium_format.h`:208
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a
  - `collection` (const std::string &): n/a

#### `nlohmann::json toJson(const Changefeed::ChangeEvent &event, const std::string &collection="") const`
- Source: `include/cdc/debezium_format.h`:251
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a
  - `collection` (const std::string &): n/a

#### `nlohmann::json toJsonWithSchema(const Changefeed::ChangeEvent &event, const std::string &collection="") const`
- Source: `include/cdc/debezium_format.h`:256
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a
  - `collection` (const std::string &): n/a

### themis::cdc::DebeziumSource

#### `nlohmann::json toJson() const`
- Source: `include/cdc/debezium_format.h`:93
- Brief: n/a
- Parameters: none

### themis::cdc::DeliveryTracker

#### `DeliveryTracker(DeliveryTrackerConfig config={}, RedeliveryCallback callback=nullptr)`
- Source: `include/cdc/delivery_tracker.h`:77
- Brief: n/a
- Parameters:
  - `config` (DeliveryTrackerConfig): n/a
  - `callback` (RedeliveryCallback): n/a

#### `DeliveryTracker(const DeliveryTracker &)=delete`
- Source: `include/cdc/delivery_tracker.h`:83
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DeliveryTracker &): n/a

#### `bool acknowledge(const std::string &consumer_id, uint64_t sequence)`
- Source: `include/cdc/delivery_tracker.h`:111
- Brief: Acknowledge.
- Parameters:
  - `consumer_id` (const std::string &): Identifier of the consumer.
  - `sequence` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: consumer_id Identifier of the consumer. sequence Input parameter. True when the operation succeeds. consumer_id Identifier of the consumer. sequence Input parameter. True when the operation succeeds. Calls: lock(), find(), end(), erase(), THEMIS_DEBUG(), size().

#### `size_t acknowledgeUpTo(const std::string &consumer_id, uint64_t up_to_sequence)`
- Source: `include/cdc/delivery_tracker.h`:119
- Brief: Acknowledge Up To.
- Parameters:
  - `consumer_id` (const std::string &): Identifier of the consumer.
  - `up_to_sequence` (uint64_t): Input parameter.
- Return: Return value.
- Details: consumer_id Identifier of the consumer. up_to_sequence Input parameter. Return value. consumer_id Identifier of the consumer. up_to_sequence Input parameter. Return value. Calls: lock(), find(), end(), begin(), erase(), THEMIS_DEBUG(), size().

#### `void checkAndRedeliver()`
- Source: `include/cdc/delivery_tracker.h`:183
- Brief: Check And Redeliver.
- Parameters: none
- Details: Calls: lock(), reserve(), size(), push_back(), getPendingRedelivery(), empty(), redelivery_callback_().

#### `size_t consumerCount() const`
- Source: `include/cdc/delivery_tracker.h`:148
- Brief: Consumer Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< ConsumerDeliveryStats > getAllStats() const`
- Source: `include/cdc/delivery_tracker.h`:142
- Brief: Get All Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< Changefeed::ChangeEvent > getPendingRedelivery(const std::string &consumer_id, std::optional< std::chrono::milliseconds > timeout_override=std::nullopt)`
- Source: `include/cdc/delivery_tracker.h`:121
- Brief: n/a
- Parameters:
  - `consumer_id` (const std::string &): n/a
  - `timeout_override` (std::optional< std::chrono::milliseconds >): n/a

#### `std::optional< ConsumerDeliveryStats > getStats(const std::string &consumer_id) const`
- Source: `include/cdc/delivery_tracker.h`:136
- Brief: Get Stats.
- Parameters:
  - `consumer_id` (const std::string &): Identifier of the consumer.
- Return: Return value.
- Details: consumer_id Identifier of the consumer. Return value.

#### `DeliveryTracker & operator=(const DeliveryTracker &)=delete`
- Source: `include/cdc/delivery_tracker.h`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DeliveryTracker &): n/a

#### `void redeliveryThreadFunc()`
- Source: `include/cdc/delivery_tracker.h`:179
- Brief: Redelivery Thread Func.
- Parameters: none
- Details: ===== Background Redelivery Thread ===== Calls: THEMIS_DEBUG(), load(), lock(), wait_for(), checkAndRedeliver(), THEMIS_ERROR(), what().

#### `void removeConsumer(const std::string &consumer_id)`
- Source: `include/cdc/delivery_tracker.h`:129
- Brief: Remove Consumer.
- Parameters:
  - `consumer_id` (const std::string &): Identifier of the consumer.
- Details: consumer_id Identifier of the consumer. consumer_id Identifier of the consumer. Calls: lock(), erase(), THEMIS_INFO().

#### `void start()`
- Source: `include/cdc/delivery_tracker.h`:89
- Brief: Start.
- Parameters: none
- Details: Calls: exchange(), std::thread(), THEMIS_INFO(), count().

#### `void stop()`
- Source: `include/cdc/delivery_tracker.h`:94
- Brief: Stop.
- Parameters: none
- Details: Calls: exchange(), notify_all(), joinable(), join(), THEMIS_INFO().

#### `bool trackDelivery(const std::string &consumer_id, const std::vector< Changefeed::ChangeEvent > &events)`
- Source: `include/cdc/delivery_tracker.h`:102
- Brief: Track Delivery.
- Parameters:
  - `consumer_id` (const std::string &): Identifier of the consumer.
  - `events` (const std::vector< Changefeed::ChangeEvent > &): Input parameter.
- Return: True when the operation succeeds.
- Details: consumer_id Identifier of the consumer. events Input parameter. True when the operation succeeds. consumer_id Identifier of the consumer. events Input parameter. True when the operation succeeds. Calls: empty(), lock(), size(), THEMIS_WARN(), std::chrono::steady_clock::now(), emplace(), std::move(), THEMIS_DEBUG().

#### `~DeliveryTracker()`
- Source: `include/cdc/delivery_tracker.h`:80
- Brief: n/a
- Parameters: none

### themis::cdc::DiagnosticsInfo

#### `nlohmann::json toJson() const`
- Source: `include/cdc/cdc_admin.h`:85
- Brief: n/a
- Parameters: none

### themis::cdc::EventTypeFilter

#### `EventTypeFilter(std::string name, std::vector< Changefeed::ChangeEventType > types)`
- Source: `include/cdc/icdc_filter_pipeline.h`:124
- Brief: n/a
- Parameters:
  - `name` (std::string): n/a
  - `types` (std::vector< Changefeed::ChangeEventType >): n/a

#### `FilterResult evaluate(const Changefeed::ChangeEvent &event) const noexcept override`
- Source: `include/cdc/icdc_filter_pipeline.h`:128
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `std::string name() const override`
- Source: `include/cdc/icdc_filter_pipeline.h`:142
- Brief: n/a
- Parameters: none

### themis::cdc::FanInEvent

#### `nlohmann::json toJson() const`
- Source: `include/cdc/icdc_fan_in.h`:54
- Brief: n/a
- Parameters: none

### themis::cdc::GDPRRedactionResult

#### `nlohmann::json toJson() const`
- Source: `include/cdc/cdc_admin.h`:158
- Brief: n/a
- Parameters: none

### themis::cdc::HealthStatus

#### `nlohmann::json toJson() const`
- Source: `include/cdc/cdc_admin.h`:57
- Brief: n/a
- Parameters: none

### themis::cdc::ICDCBackpressureSignal

#### `void clearBackpressure()=0`
- Source: `include/cdc/icdc_backpressure_signal.h`:63
- Brief: Clear Backpressure.
- Parameters: none

#### `BackpressureLevel currentLevel() const =0`
- Source: `include/cdc/icdc_backpressure_signal.h`:69
- Brief: Current Level.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void signalBackpressure(BackpressureLevel level)=0`
- Source: `include/cdc/icdc_backpressure_signal.h`:58
- Brief: Signal Backpressure.
- Parameters:
  - `level` (BackpressureLevel): Input parameter.
- Details: level Input parameter.

#### `~ICDCBackpressureSignal()=default`
- Source: `include/cdc/icdc_backpressure_signal.h`:52
- Brief: ICDCBackpressure Signal.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::ICDCBatchCommitCoordinator

#### `AddEventResult addEvent(const Changefeed::ChangeEvent &event)=0`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:118
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `BatchId beginBatch()=0`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:116
- Brief: n/a
- Parameters: none

#### `CommitResult commitBatch()=0`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:120
- Brief: n/a
- Parameters: none

#### `std::vector< Changefeed::ChangeEvent > committedEvents(BatchId batch_id) const =0`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:128
- Brief: n/a
- Parameters:
  - `batch_id` (BatchId): n/a

#### `BatchInfo info() const =0`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:126
- Brief: n/a
- Parameters: none

#### `bool isCommitted(BatchId batch_id) const =0`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:131
- Brief: n/a
- Parameters:
  - `batch_id` (BatchId): n/a

#### `RollbackResult rollbackBatch()=0`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:122
- Brief: n/a
- Parameters: none

#### `BatchStatus status() const =0`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:124
- Brief: n/a
- Parameters: none

#### `~ICDCBatchCommitCoordinator()=default`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:114
- Brief: ICDCBatch Commit Coordinator.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::ICDCEventSchema

#### `int currentVersion(const std::string &collection) const =0`
- Source: `include/cdc/icdc_event_schema.h`:136
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a

#### `std::string getSchema(const std::string &collection, int version=-1) const =0`
- Source: `include/cdc/icdc_event_schema.h`:133
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `version` (int): n/a

#### `void onSchemaEvolution(const std::string &collection, std::shared_ptr< ISchemaEvolutionCallback > callback)=0`
- Source: `include/cdc/icdc_event_schema.h`:143
- Brief: On Schema Evolution.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `callback` (std::shared_ptr< ISchemaEvolutionCallback >): Input parameter.
- Details: collection Input parameter. callback Input parameter.

#### `bool registerSchema(const std::string &collection, const std::string &schema_def, SchemaFormat format, int version)=0`
- Source: `include/cdc/icdc_event_schema.h`:128
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `schema_def` (const std::string &): n/a
  - `format` (SchemaFormat): n/a
  - `version` (int): n/a

#### `bool triggerEvolution(const SchemaEvolutionDescriptor &descriptor)=0`
- Source: `include/cdc/icdc_event_schema.h`:147
- Brief: n/a
- Parameters:
  - `descriptor` (const SchemaEvolutionDescriptor &): n/a

#### `~ICDCEventSchema()=default`
- Source: `include/cdc/icdc_event_schema.h`:126
- Brief: ICDCEvent Schema.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::ICDCFanIn

#### `bool addSource(const CollectionId &id, Changefeed *feed)=0`
- Source: `include/cdc/icdc_fan_in.h`:106
- Brief: n/a
- Parameters:
  - `id` (const CollectionId &): n/a
  - `feed` (Changefeed *): n/a

#### `std::vector< FanInEvent > listEvents(uint64_t from_sequence=0, std::size_t limit=0, const std::vector< CollectionId > &collections={}) const =0`
- Source: `include/cdc/icdc_fan_in.h`:110
- Brief: n/a
- Parameters:
  - `from_sequence` (uint64_t): n/a
  - `limit` (std::size_t): n/a
  - `collections` (const std::vector< CollectionId > &): n/a

#### `bool removeSource(const CollectionId &id)=0`
- Source: `include/cdc/icdc_fan_in.h`:108
- Brief: n/a
- Parameters:
  - `id` (const CollectionId &): n/a

#### `void setMergePolicy(std::unique_ptr< IFanInMergePolicy > policy)=0`
- Source: `include/cdc/icdc_fan_in.h`:119
- Brief: Set Merge Policy.
- Parameters:
  - `policy` (std::unique_ptr< IFanInMergePolicy >): Input parameter.
- Details: policy Input parameter.

#### `std::vector< CollectionId > sourceIds() const =0`
- Source: `include/cdc/icdc_fan_in.h`:121
- Brief: n/a
- Parameters: none

#### `~ICDCFanIn()=default`
- Source: `include/cdc/icdc_fan_in.h`:104
- Brief: ICDCFan In.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::ICDCFilterPipeline

#### `bool addFilter(std::unique_ptr< IEventFilter > filter)=0`
- Source: `include/cdc/icdc_filter_pipeline.h`:159
- Brief: n/a
- Parameters:
  - `filter` (std::unique_ptr< IEventFilter >): n/a

#### `FilterResult apply(const Changefeed::ChangeEvent &event) const =0`
- Source: `include/cdc/icdc_filter_pipeline.h`:169
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `std::vector< Changefeed::ChangeEvent > applyBatch(const std::vector< Changefeed::ChangeEvent > &events) const =0`
- Source: `include/cdc/icdc_filter_pipeline.h`:171
- Brief: n/a
- Parameters:
  - `events` (const std::vector< Changefeed::ChangeEvent > &): n/a

#### `bool empty() const =0`
- Source: `include/cdc/icdc_filter_pipeline.h`:167
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > filterNames() const =0`
- Source: `include/cdc/icdc_filter_pipeline.h`:174
- Brief: n/a
- Parameters: none

#### `bool hasFilter(const std::string &name) const =0`
- Source: `include/cdc/icdc_filter_pipeline.h`:163
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `bool removeFilter(const std::string &name)=0`
- Source: `include/cdc/icdc_filter_pipeline.h`:161
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `void resetCounters()=0`
- Source: `include/cdc/icdc_filter_pipeline.h`:183
- Brief: Reset Counters.
- Parameters: none

#### `std::size_t size() const =0`
- Source: `include/cdc/icdc_filter_pipeline.h`:165
- Brief: n/a
- Parameters: none

#### `std::size_t totalDropped() const =0`
- Source: `include/cdc/icdc_filter_pipeline.h`:178
- Brief: n/a
- Parameters: none

#### `std::size_t totalPassed() const =0`
- Source: `include/cdc/icdc_filter_pipeline.h`:176
- Brief: n/a
- Parameters: none

#### `~ICDCFilterPipeline()=default`
- Source: `include/cdc/icdc_filter_pipeline.h`:157
- Brief: ICDCFilter Pipeline.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::ICDCPauseControl

#### `std::size_t bufferedEventCount() const =0`
- Source: `include/cdc/icdc_pause_control.h`:66
- Brief: n/a
- Parameters: none

#### `bool isPaused() const =0`
- Source: `include/cdc/icdc_pause_control.h`:62
- Brief: n/a
- Parameters: none

#### `bool pause(PauseReason reason=PauseReason::AdminRequest)=0`
- Source: `include/cdc/icdc_pause_control.h`:58
- Brief: n/a
- Parameters:
  - `reason` (PauseReason): n/a

#### `PauseReason pauseReason() const =0`
- Source: `include/cdc/icdc_pause_control.h`:64
- Brief: n/a
- Parameters: none

#### `bool resume()=0`
- Source: `include/cdc/icdc_pause_control.h`:60
- Brief: n/a
- Parameters: none

#### `~ICDCPauseControl()=default`
- Source: `include/cdc/icdc_pause_control.h`:56
- Brief: ICDCPause Control.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::ICDCReplayController

#### `std::unique_ptr< IReplaySession > beginReplay(const ReplayOptions &options)=0`
- Source: `include/cdc/icdc_replay_controller.h`:110
- Brief: n/a
- Parameters:
  - `options` (const ReplayOptions &): n/a

#### `std::unique_ptr< IReplaySession > replayFromSequence(uint64_t from_sequence, uint64_t to_sequence=0)=0`
- Source: `include/cdc/icdc_replay_controller.h`:117
- Brief: n/a
- Parameters:
  - `from_sequence` (uint64_t): n/a
  - `to_sequence` (uint64_t): n/a

#### `std::unique_ptr< IReplaySession > replayFromTimestamp(int64_t from_timestamp_ms, int64_t to_timestamp_ms=0)=0`
- Source: `include/cdc/icdc_replay_controller.h`:113
- Brief: n/a
- Parameters:
  - `from_timestamp_ms` (int64_t): n/a
  - `to_timestamp_ms` (int64_t): n/a

#### `std::size_t totalSessionsCreated() const =0`
- Source: `include/cdc/icdc_replay_controller.h`:121
- Brief: n/a
- Parameters: none

#### `~ICDCReplayController()=default`
- Source: `include/cdc/icdc_replay_controller.h`:108
- Brief: ICDCReplay Controller.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::ICDCTransport

#### `bool publish(const Changefeed::ChangeEvent &event)=0`
- Source: `include/cdc/icdc_transport.h`:50
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `bool start()=0`
- Source: `include/cdc/icdc_transport.h`:43
- Brief: n/a
- Parameters: none

#### `void stop()=0`
- Source: `include/cdc/icdc_transport.h`:48
- Brief: Stop.
- Parameters: none

#### `~ICDCTransport()=default`
- Source: `include/cdc/icdc_transport.h`:41
- Brief: ICDCTransport.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::IDeliveryGuaranteeConfig

#### `std::chrono::milliseconds ackTimeout() const =0`
- Source: `include/cdc/idelivery_guarantee_config.h`:110
- Brief: Ack Timeout.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::milliseconds deduplicationWindow() const =0`
- Source: `include/cdc/idelivery_guarantee_config.h`:122
- Brief: Deduplication Window.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `DeliveryMode mode() const =0`
- Source: `include/cdc/idelivery_guarantee_config.h`:98
- Brief: Mode.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void setAckTimeout(std::chrono::milliseconds timeout)=0`
- Source: `include/cdc/idelivery_guarantee_config.h`:104
- Brief: Set Ack Timeout.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Details: timeout Input parameter.

#### `void setDeduplicationWindow(std::chrono::milliseconds window)=0`
- Source: `include/cdc/idelivery_guarantee_config.h`:116
- Brief: Set Deduplication Window.
- Parameters:
  - `window` (std::chrono::milliseconds): Input parameter.
- Details: window Input parameter.

#### `void setMode(DeliveryMode mode)=0`
- Source: `include/cdc/idelivery_guarantee_config.h`:92
- Brief: Set Mode.
- Parameters:
  - `mode` (DeliveryMode): Input parameter.
- Details: mode Input parameter.

#### `~IDeliveryGuaranteeConfig()=default`
- Source: `include/cdc/idelivery_guarantee_config.h`:86
- Brief: IDelivery Guarantee Config.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::IEventFilter

#### `FilterResult evaluate(const Changefeed::ChangeEvent &event) const noexcept=0`
- Source: `include/cdc/icdc_filter_pipeline.h`:64
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `std::string name() const =0`
- Source: `include/cdc/icdc_filter_pipeline.h`:66
- Brief: n/a
- Parameters: none

#### `~IEventFilter()=default`
- Source: `include/cdc/icdc_filter_pipeline.h`:62
- Brief: IEvent Filter.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::IFanInMergePolicy

#### `void merge(std::vector< FanInEvent > &events) const =0`
- Source: `include/cdc/icdc_fan_in.h`:75
- Brief: Merge.
- Parameters:
  - `events` (std::vector< FanInEvent > &): Input/output parameter.
- Details: events Input/output parameter.

#### `~IFanInMergePolicy()=default`
- Source: `include/cdc/icdc_fan_in.h`:69
- Brief: IFan In Merge Policy.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::IIdempotentCDCListener

#### `bool isDuplicate(const std::string &collection, uint64_t sequence) const =0`
- Source: `include/cdc/idelivery_guarantee_config.h`:66
- Brief: Is Duplicate.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `sequence` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: collection Input parameter. sequence Input parameter. True when the operation succeeds.

#### `void markProcessed(const std::string &collection, uint64_t sequence)=0`
- Source: `include/cdc/idelivery_guarantee_config.h`:74
- Brief: Mark Processed.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `sequence` (uint64_t): Input parameter.
- Details: collection Input parameter. sequence Input parameter.

#### `~IIdempotentCDCListener()=default`
- Source: `include/cdc/idelivery_guarantee_config.h`:58
- Brief: IIdempotent CDCListener.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::IReplaySession

#### `void cancel()=0`
- Source: `include/cdc/icdc_replay_controller.h`:93
- Brief: Cancel.
- Parameters: none

#### `std::size_t deliveredCount() const =0`
- Source: `include/cdc/icdc_replay_controller.h`:97
- Brief: n/a
- Parameters: none

#### `bool done() const =0`
- Source: `include/cdc/icdc_replay_controller.h`:88
- Brief: n/a
- Parameters: none

#### `std::vector< Changefeed::ChangeEvent > nextBatch()=0`
- Source: `include/cdc/icdc_replay_controller.h`:86
- Brief: n/a
- Parameters: none

#### `ReplaySessionState state() const =0`
- Source: `include/cdc/icdc_replay_controller.h`:95
- Brief: n/a
- Parameters: none

#### `~IReplaySession()=default`
- Source: `include/cdc/icdc_replay_controller.h`:84
- Brief: IReplay Session.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::ISchemaEvolutionCallback

#### `void onCompatible(const SchemaEvolutionDescriptor &descriptor)=0`
- Source: `include/cdc/icdc_event_schema.h`:107
- Brief: On Compatible.
- Parameters:
  - `descriptor` (const SchemaEvolutionDescriptor &): Input parameter.
- Details: descriptor Input parameter.

#### `void onIncompatible(const SchemaEvolutionDescriptor &descriptor, const SchemaConflict &conflict)=0`
- Source: `include/cdc/icdc_event_schema.h`:114
- Brief: On Incompatible.
- Parameters:
  - `descriptor` (const SchemaEvolutionDescriptor &): Input parameter.
  - `conflict` (const SchemaConflict &): Input parameter.
- Details: descriptor Input parameter. conflict Input parameter.

#### `~ISchemaEvolutionCallback()=default`
- Source: `include/cdc/icdc_event_schema.h`:101
- Brief: ISchema Evolution Callback.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::ISchemaRegistryBackend

#### `std::optional< SchemaInfo > getById(int32_t id) const =0`
- Source: `include/cdc/schema_registry.h`:139
- Brief: n/a
- Parameters:
  - `id` (int32_t): n/a

#### `std::optional< SchemaInfo > getLatest(const std::string &subject) const =0`
- Source: `include/cdc/schema_registry.h`:141
- Brief: n/a
- Parameters:
  - `subject` (const std::string &): n/a

#### `int32_t registerSchema(const std::string &subject, const std::string &schema_json, SchemaFormat format)=0`
- Source: `include/cdc/schema_registry.h`:135
- Brief: n/a
- Parameters:
  - `subject` (const std::string &): n/a
  - `schema_json` (const std::string &): n/a
  - `format` (SchemaFormat): n/a

#### `~ISchemaRegistryBackend()=default`
- Source: `include/cdc/schema_registry.h`:133
- Brief: ISchema Registry Backend.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::InMemoryBackpressureSignal

#### `InMemoryBackpressureSignal(LevelCallback cb={})`
- Source: `include/cdc/icdc_backpressure_signal.h`:78
- Brief: n/a
- Parameters:
  - `cb` (LevelCallback): n/a

#### `void clearBackpressure() override`
- Source: `include/cdc/icdc_backpressure_signal.h`:98
- Brief: Clear Backpressure.
- Parameters: none

#### `BackpressureLevel currentLevel() const override`
- Source: `include/cdc/icdc_backpressure_signal.h`:102
- Brief: Current Level.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void setCallback(LevelCallback cb)`
- Source: `include/cdc/icdc_backpressure_signal.h`:112
- Brief: ── Extra helpers ─────────────────────────────────────────────────────────
- Parameters:
  - `cb` (LevelCallback): Input parameter.
- Details: cb Input parameter. Calls: lk(), std::move().

#### `void signalBackpressure(BackpressureLevel level) override`
- Source: `include/cdc/icdc_backpressure_signal.h`:83
- Brief: Signal Backpressure.
- Parameters:
  - `level` (BackpressureLevel): Input parameter.
- Details: level Input parameter.

### themis::cdc::InMemoryBatchCommitCoordinator

#### `InMemoryBatchCommitCoordinator(BatchConfig cfg=BatchConfig{})`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:140
- Brief: n/a
- Parameters:
  - `cfg` (BatchConfig): n/a

#### `AddEventResult addEvent(const Changefeed::ChangeEvent &event) override`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:167
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `BatchId beginBatch() override`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:153
- Brief: n/a
- Parameters: none

#### `CommitResult commitBatch() override`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:184
- Brief: n/a
- Parameters: none

#### `std::vector< Changefeed::ChangeEvent > committedEvents(BatchId batch_id) const override`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:257
- Brief: n/a
- Parameters:
  - `batch_id` (BatchId): n/a

#### `void evictOldestIfNeeded()`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:286
- Brief: Evict Oldest If Needed.
- Parameters: none
- Details: Calls: size(), erase(), front(), pop_front().

#### `BatchInfo info() const override`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:239
- Brief: n/a
- Parameters: none

#### `bool isCommitted(BatchId batch_id) const override`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:271
- Brief: n/a
- Parameters:
  - `batch_id` (BatchId): n/a

#### `RollbackResult rollbackBatch() override`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:213
- Brief: n/a
- Parameters: none

#### `BatchStatus status() const override`
- Source: `include/cdc/icdc_batch_commit_coordinator.h`:229
- Brief: n/a
- Parameters: none

### themis::cdc::InMemoryCDCEventSchema

#### `int currentVersion(const std::string &collection) const override`
- Source: `include/cdc/icdc_event_schema.h`:199
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a

#### `std::string getSchema(const std::string &collection, int version=-1) const override`
- Source: `include/cdc/icdc_event_schema.h`:176
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `version` (int): n/a

#### `void onSchemaEvolution(const std::string &collection, std::shared_ptr< ISchemaEvolutionCallback > callback) override`
- Source: `include/cdc/icdc_event_schema.h`:210
- Brief: On Schema Evolution.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `callback` (std::shared_ptr< ISchemaEvolutionCallback >): Input parameter.
- Details: collection Input parameter. callback Input parameter.

#### `bool registerSchema(const std::string &collection, const std::string &schema_def, SchemaFormat format, int version) override`
- Source: `include/cdc/icdc_event_schema.h`:156
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `schema_def` (const std::string &): n/a
  - `format` (SchemaFormat): n/a
  - `version` (int): n/a

#### `bool triggerEvolution(const SchemaEvolutionDescriptor &descriptor) override`
- Source: `include/cdc/icdc_event_schema.h`:223
- Brief: n/a
- Parameters:
  - `descriptor` (const SchemaEvolutionDescriptor &): n/a

### themis::cdc::InMemoryDeliveryGuaranteeConfig

#### `InMemoryDeliveryGuaranteeConfig(DeliveryMode mode=DeliveryMode::AtLeastOnce, std::chrono::milliseconds ack_to=kDefaultAckTimeout, std::chrono::milliseconds dedup_w=kDefaultDedupWindow)`
- Source: `include/cdc/idelivery_guarantee_config.h`:132
- Brief: n/a
- Parameters:
  - `mode` (DeliveryMode): n/a
  - `ack_to` (std::chrono::milliseconds): n/a
  - `dedup_w` (std::chrono::milliseconds): n/a

#### `std::chrono::milliseconds ackTimeout() const override`
- Source: `include/cdc/idelivery_guarantee_config.h`:170
- Brief: Ack Timeout.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::chrono::milliseconds deduplicationWindow() const override`
- Source: `include/cdc/idelivery_guarantee_config.h`:190
- Brief: Deduplication Window.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `DeliveryMode mode() const override`
- Source: `include/cdc/idelivery_guarantee_config.h`:150
- Brief: Mode.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void setAckTimeout(std::chrono::milliseconds timeout) override`
- Source: `include/cdc/idelivery_guarantee_config.h`:160
- Brief: Set Ack Timeout.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Details: timeout Input parameter.

#### `void setDeduplicationWindow(std::chrono::milliseconds window) override`
- Source: `include/cdc/idelivery_guarantee_config.h`:180
- Brief: Set Deduplication Window.
- Parameters:
  - `window` (std::chrono::milliseconds): Input parameter.
- Details: window Input parameter.

#### `void setMode(DeliveryMode mode) override`
- Source: `include/cdc/idelivery_guarantee_config.h`:140
- Brief: Set Mode.
- Parameters:
  - `mode` (DeliveryMode): Input parameter.
- Details: mode Input parameter.

### themis::cdc::InMemoryFanIn

#### `InMemoryFanIn()`
- Source: `include/cdc/icdc_fan_in.h`:128
- Brief: n/a
- Parameters: none

#### `bool addSource(const CollectionId &id, Changefeed *feed) override`
- Source: `include/cdc/icdc_fan_in.h`:133
- Brief: n/a
- Parameters:
  - `id` (const CollectionId &): n/a
  - `feed` (Changefeed *): n/a

#### `std::vector< FanInEvent > listEvents(uint64_t from_sequence=0, std::size_t limit=0, const std::vector< CollectionId > &collections={}) const override`
- Source: `include/cdc/icdc_fan_in.h`:157
- Brief: n/a
- Parameters:
  - `from_sequence` (uint64_t): n/a
  - `limit` (std::size_t): n/a
  - `collections` (const std::vector< CollectionId > &): n/a

#### `bool removeSource(const CollectionId &id) override`
- Source: `include/cdc/icdc_fan_in.h`:147
- Brief: n/a
- Parameters:
  - `id` (const CollectionId &): n/a

#### `void setMergePolicy(std::unique_ptr< IFanInMergePolicy > policy) override`
- Source: `include/cdc/icdc_fan_in.h`:221
- Brief: Set Merge Policy.
- Parameters:
  - `policy` (std::unique_ptr< IFanInMergePolicy >): Input parameter.
- Details: policy Input parameter.

#### `std::vector< CollectionId > sourceIds() const override`
- Source: `include/cdc/icdc_fan_in.h`:231
- Brief: n/a
- Parameters: none

### themis::cdc::InMemoryFilterPipeline

#### `InMemoryFilterPipeline()=default`
- Source: `include/cdc/icdc_filter_pipeline.h`:190
- Brief: n/a
- Parameters: none

#### `bool addFilter(std::unique_ptr< IEventFilter > filter) override`
- Source: `include/cdc/icdc_filter_pipeline.h`:192
- Brief: n/a
- Parameters:
  - `filter` (std::unique_ptr< IEventFilter >): n/a

#### `FilterResult apply(const Changefeed::ChangeEvent &event) const override`
- Source: `include/cdc/icdc_filter_pipeline.h`:263
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `std::vector< Changefeed::ChangeEvent > applyBatch(const std::vector< Changefeed::ChangeEvent > &events) const override`
- Source: `include/cdc/icdc_filter_pipeline.h`:287
- Brief: n/a
- Parameters:
  - `events` (const std::vector< Changefeed::ChangeEvent > &): n/a

#### `bool empty() const override`
- Source: `include/cdc/icdc_filter_pipeline.h`:253
- Brief: n/a
- Parameters: none

#### `std::vector< std::string > filterNames() const override`
- Source: `include/cdc/icdc_filter_pipeline.h`:301
- Brief: n/a
- Parameters: none

#### `bool hasFilter(const std::string &name) const override`
- Source: `include/cdc/icdc_filter_pipeline.h`:228
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `bool removeFilter(const std::string &name) override`
- Source: `include/cdc/icdc_filter_pipeline.h`:212
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a

#### `void resetCounters() override`
- Source: `include/cdc/icdc_filter_pipeline.h`:325
- Brief: Reset Counters.
- Parameters: none

#### `std::size_t size() const override`
- Source: `include/cdc/icdc_filter_pipeline.h`:243
- Brief: n/a
- Parameters: none

#### `std::size_t totalDropped() const override`
- Source: `include/cdc/icdc_filter_pipeline.h`:321
- Brief: n/a
- Parameters: none

#### `std::size_t totalPassed() const override`
- Source: `include/cdc/icdc_filter_pipeline.h`:317
- Brief: n/a
- Parameters: none

### themis::cdc::InMemoryIdempotentListener

#### `InMemoryIdempotentListener(std::size_t max_window_size=100 '000)`
- Source: `include/cdc/idelivery_guarantee_config.h`:211
- Brief: n/a
- Parameters:
  - `max_window_size` (std::size_t): n/a

#### `bool isDuplicate(const std::string &collection, uint64_t sequence) const override`
- Source: `include/cdc/idelivery_guarantee_config.h`:214
- Brief: Is Duplicate.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `sequence` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: collection Input parameter. sequence Input parameter. True when the operation succeeds.

#### `std::string makeKey(const std::string &collection, uint64_t seq)`
- Source: `include/cdc/idelivery_guarantee_config.h`:264
- Brief: Make Key.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `seq` (uint64_t): Input parameter.
- Return: Return value.
- Details: collection Input parameter. seq Input parameter. Return value. Calls: std::to_string().

#### `void markProcessed(const std::string &collection, uint64_t sequence) override`
- Source: `include/cdc/idelivery_guarantee_config.h`:226
- Brief: Mark Processed.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `sequence` (uint64_t): Input parameter.
- Details: collection Input parameter. sequence Input parameter.

#### `std::size_t processedCount() const`
- Source: `include/cdc/idelivery_guarantee_config.h`:246
- Brief: n/a
- Parameters: none

### themis::cdc::InMemoryPauseControl

#### `InMemoryPauseControl(std::size_t max_buffer_bytes=kDefaultMaxBufferBytes)`
- Source: `include/cdc/icdc_pause_control.h`:75
- Brief: n/a
- Parameters:
  - `max_buffer_bytes` (std::size_t): n/a

#### `bool bufferEvent(const Changefeed::ChangeEvent &event)`
- Source: `include/cdc/icdc_pause_control.h`:143
- Brief: ── InMemoryPauseControl-specific API ────────────────────────────────────
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): Input parameter.
- Return: True when the operation succeeds.
- Details: event Input parameter. True when the operation succeeds. Calls: lk(), load(), toJson(), dump(), size(), push_back().

#### `std::size_t bufferedEventCount() const override`
- Source: `include/cdc/icdc_pause_control.h`:126
- Brief: n/a
- Parameters: none

#### `std::deque< Changefeed::ChangeEvent > drainBuffer()`
- Source: `include/cdc/icdc_pause_control.h`:162
- Brief: Drain Buffer.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: lk(), swap().

#### `bool isPaused() const override`
- Source: `include/cdc/icdc_pause_control.h`:112
- Brief: n/a
- Parameters: none

#### `bool pause(PauseReason reason=PauseReason::AdminRequest) override`
- Source: `include/cdc/icdc_pause_control.h`:81
- Brief: n/a
- Parameters:
  - `reason` (PauseReason): n/a

#### `PauseReason pauseReason() const override`
- Source: `include/cdc/icdc_pause_control.h`:116
- Brief: n/a
- Parameters: none

#### `bool resume() override`
- Source: `include/cdc/icdc_pause_control.h`:96
- Brief: n/a
- Parameters: none

#### `bool waitForResume(std::chrono::milliseconds timeout)`
- Source: `include/cdc/icdc_pause_control.h`:176
- Brief: Wait For Resume.
- Parameters:
  - `timeout` (std::chrono::milliseconds): Input parameter.
- Return: True when the operation succeeds.
- Details: timeout Input parameter. True when the operation succeeds. Calls: lk(), wait_for(), load().

### themis::cdc::InMemoryReplayController

#### `InMemoryReplayController(Changefeed *feed)`
- Source: `include/cdc/icdc_replay_controller.h`:223
- Brief: In Memory Replay Controller.
- Parameters:
  - `feed` (Changefeed *): Input/output parameter.
- Return: Return value.
- Details: feed Input/output parameter. Return value.

#### `std::unique_ptr< IReplaySession > beginReplay(const ReplayOptions &options) override`
- Source: `include/cdc/icdc_replay_controller.h`:228
- Brief: n/a
- Parameters:
  - `options` (const ReplayOptions &): n/a

#### `std::vector< Changefeed::ChangeEvent > fetchAndFilter(const ReplayOptions &opts) const`
- Source: `include/cdc/icdc_replay_controller.h`:262
- Brief: n/a
- Parameters:
  - `opts` (const ReplayOptions &): n/a

#### `std::unique_ptr< IReplaySession > replayFromSequence(uint64_t from_sequence, uint64_t to_sequence=0) override`
- Source: `include/cdc/icdc_replay_controller.h`:247
- Brief: n/a
- Parameters:
  - `from_sequence` (uint64_t): n/a
  - `to_sequence` (uint64_t): n/a

#### `std::unique_ptr< IReplaySession > replayFromTimestamp(int64_t from_timestamp_ms, int64_t to_timestamp_ms=0) override`
- Source: `include/cdc/icdc_replay_controller.h`:237
- Brief: n/a
- Parameters:
  - `from_timestamp_ms` (int64_t): n/a
  - `to_timestamp_ms` (int64_t): n/a

#### `std::size_t totalSessionsCreated() const override`
- Source: `include/cdc/icdc_replay_controller.h`:257
- Brief: n/a
- Parameters: none

### themis::cdc::InMemoryReplaySession

#### `InMemoryReplaySession(std::vector< Changefeed::ChangeEvent > events, std::size_t batch_size)`
- Source: `include/cdc/icdc_replay_controller.h`:128
- Brief: n/a
- Parameters:
  - `events` (std::vector< Changefeed::ChangeEvent >): n/a
  - `batch_size` (std::size_t): n/a

#### `void cancel() override`
- Source: `include/cdc/icdc_replay_controller.h`:175
- Brief: Cancel.
- Parameters: none

#### `std::size_t deliveredCount() const override`
- Source: `include/cdc/icdc_replay_controller.h`:195
- Brief: n/a
- Parameters: none

#### `bool done() const override`
- Source: `include/cdc/icdc_replay_controller.h`:165
- Brief: n/a
- Parameters: none

#### `std::vector< Changefeed::ChangeEvent > nextBatch() override`
- Source: `include/cdc/icdc_replay_controller.h`:137
- Brief: n/a
- Parameters: none

#### `ReplaySessionState state() const override`
- Source: `include/cdc/icdc_replay_controller.h`:185
- Brief: n/a
- Parameters: none

### themis::cdc::InMemorySchemaRegistryBackend

#### `void clear()`
- Source: `include/cdc/schema_registry.h`:231
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `std::optional< SchemaInfo > getById(int32_t id) const override`
- Source: `include/cdc/schema_registry.h`:184
- Brief: n/a
- Parameters:
  - `id` (int32_t): n/a

#### `std::optional< SchemaInfo > getLatest(const std::string &subject) const override`
- Source: `include/cdc/schema_registry.h`:198
- Brief: n/a
- Parameters:
  - `subject` (const std::string &): n/a

#### `int32_t registerSchema(const std::string &subject, const std::string &schema_json, SchemaFormat format) override`
- Source: `include/cdc/schema_registry.h`:149
- Brief: n/a
- Parameters:
  - `subject` (const std::string &): n/a
  - `schema_json` (const std::string &): n/a
  - `format` (SchemaFormat): n/a

#### `size_t size() const`
- Source: `include/cdc/schema_registry.h`:217
- Brief: n/a
- Parameters: none

### themis::cdc::KafkaCDCProducer

#### `KafkaCDCProducer(Changefeed *, KafkaProducerConfig={}, CDCMetrics *=nullptr)`
- Source: `include/cdc/kafka_cdc_producer.h`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (Changefeed *): n/a
  - `<unnamed>` (KafkaProducerConfig): n/a
  - `<unnamed>` (CDCMetrics *): n/a

#### `KafkaCDCProducer(const KafkaCDCProducer &)=delete`
- Source: `include/cdc/kafka_cdc_producer.h`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (const KafkaCDCProducer &): n/a

#### `KafkaProducerStats getStats() const`
- Source: `include/cdc/kafka_cdc_producer.h`:220
- Brief: n/a
- Parameters: none

#### `KafkaCDCProducer & operator=(const KafkaCDCProducer &)=delete`
- Source: `include/cdc/kafka_cdc_producer.h`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (const KafkaCDCProducer &): n/a

#### `bool publish(const Changefeed::ChangeEvent &event) override`
- Source: `include/cdc/kafka_cdc_producer.h`:213
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `PublishFn & s_publish_fn_()`
- Source: `include/cdc/kafka_cdc_producer.h`:272
- Brief: S publish fn.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements s_publish_fn_ without additional internal calls.

#### `std::mutex & s_publish_fn_mutex_()`
- Source: `include/cdc/kafka_cdc_producer.h`:266
- Brief: S publish fn mutex.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements s_publish_fn_mutex_ without additional internal calls.

#### `StartFn & s_start_fn_()`
- Source: `include/cdc/kafka_cdc_producer.h`:260
- Brief: S start fn.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements s_start_fn_ without additional internal calls.

#### `std::mutex & s_start_fn_mutex_()`
- Source: `include/cdc/kafka_cdc_producer.h`:254
- Brief: Static storage via function-local statics so they are lazily initialised and avoid static-initialisation-order issues.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements s_start_fn_mutex_ without additional internal calls.

#### `void setPublishFn(PublishFn fn)`
- Source: `include/cdc/kafka_cdc_producer.h`:243
- Brief: Set Publish Fn.
- Parameters:
  - `fn` (PublishFn): Input parameter.
- Details: fn Input parameter. Calls: lk(), s_publish_fn_mutex_(), s_publish_fn_(), std::move().

#### `void setStartFn(StartFn fn)`
- Source: `include/cdc/kafka_cdc_producer.h`:233
- Brief: Set Start Fn.
- Parameters:
  - `fn` (StartFn): Input parameter.
- Details: fn Input parameter. Calls: lk(), s_start_fn_mutex_(), s_start_fn_(), std::move().

#### `bool start() override`
- Source: `include/cdc/kafka_cdc_producer.h`:204
- Brief: n/a
- Parameters: none

#### `void stop() override`
- Source: `include/cdc/kafka_cdc_producer.h`:211
- Brief: Stop.
- Parameters: none

#### `~KafkaCDCProducer()=default`
- Source: `include/cdc/kafka_cdc_producer.h`:199
- Brief: n/a
- Parameters: none

### themis::cdc::KeyPrefixFilter

#### `KeyPrefixFilter(std::string name, std::string prefix)`
- Source: `include/cdc/icdc_filter_pipeline.h`:99
- Brief: n/a
- Parameters:
  - `name` (std::string): n/a
  - `prefix` (std::string): n/a

#### `FilterResult evaluate(const Changefeed::ChangeEvent &event) const noexcept override`
- Source: `include/cdc/icdc_filter_pipeline.h`:102
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `std::string name() const override`
- Source: `include/cdc/icdc_filter_pipeline.h`:113
- Brief: n/a
- Parameters: none

### themis::cdc::LatencyHistogram

#### `LatencyHistogram()`
- Source: `include/cdc/cdc_metrics.h`:36
- Brief: n/a
- Parameters: none

#### `double average() const`
- Source: `include/cdc/cdc_metrics.h`:60
- Brief: n/a
- Parameters: none

#### `size_t bucketIndex(uint64_t latency_micros) const`
- Source: `include/cdc/cdc_metrics.h`:128
- Brief: n/a
- Parameters:
  - `latency_micros` (uint64_t): n/a

#### `uint64_t bucketMidpoint(size_t bucket) const`
- Source: `include/cdc/cdc_metrics.h`:171
- Brief: n/a
- Parameters:
  - `bucket` (size_t): n/a

#### `uint64_t count() const`
- Source: `include/cdc/cdc_metrics.h`:56
- Brief: n/a
- Parameters: none

#### `uint64_t p50() const`
- Source: `include/cdc/cdc_metrics.h`:65
- Brief: n/a
- Parameters: none

#### `uint64_t p95() const`
- Source: `include/cdc/cdc_metrics.h`:69
- Brief: n/a
- Parameters: none

#### `uint64_t p99() const`
- Source: `include/cdc/cdc_metrics.h`:73
- Brief: n/a
- Parameters: none

#### `uint64_t percentile(double p) const`
- Source: `include/cdc/cdc_metrics.h`:77
- Brief: n/a
- Parameters:
  - `p` (double): n/a

#### `void record(uint64_t latency_micros)`
- Source: `include/cdc/cdc_metrics.h`:47
- Brief: Record.
- Parameters:
  - `latency_micros` (uint64_t): Input parameter.
- Details: latency_micros Input parameter. Calls: bucketIndex().

#### `void reset()`
- Source: `include/cdc/cdc_metrics.h`:111
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Calls: store().

#### `nlohmann::json toJson() const`
- Source: `include/cdc/cdc_metrics.h`:96
- Brief: n/a
- Parameters: none

### themis::cdc::OutboxRecord

#### `OutboxRecord fromJson(const nlohmann::json &j)`
- Source: `include/cdc/outbox.h`:96
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. j Input parameter. Return value. Calls: value(), contains(), is_null(), changeEventTypeFromString(), outboxStateFromString().

#### `nlohmann::json toJson() const`
- Source: `include/cdc/outbox.h`:90
- Brief: To Json.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::cdc::OutboxRelay

#### `OutboxRelay(const OutboxRelay &)=delete`
- Source: `include/cdc/outbox.h`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (const OutboxRelay &): n/a

#### `OutboxRelay(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf, Changefeed &changefeed, OutboxRelayConfig config={})`
- Source: `include/cdc/outbox.h`:160
- Brief: n/a
- Parameters:
  - `db` (rocksdb::TransactionDB *): n/a
  - `cf` (rocksdb::ColumnFamilyHandle *): n/a
  - `changefeed` (Changefeed &): n/a
  - `config` (OutboxRelayConfig): n/a

#### `std::vector< OutboxRecord > listAllRecords(size_t limit=0) const`
- Source: `include/cdc/outbox.h`:189
- Brief: n/a
- Parameters:
  - `limit` (size_t): n/a

#### `std::vector< OutboxRecord > listRecords(OutboxState state, size_t limit=0) const`
- Source: `include/cdc/outbox.h`:186
- Brief: n/a
- Parameters:
  - `state` (OutboxState): n/a
  - `limit` (size_t): n/a

#### `std::string makeKey(uint64_t seq) const`
- Source: `include/cdc/outbox.h`:238
- Brief: Make Key.
- Parameters:
  - `seq` (uint64_t): Input parameter.
- Return: Return value.
- Details: seq Input parameter. Return value.

#### `OutboxRelay & operator=(const OutboxRelay &)=delete`
- Source: `include/cdc/outbox.h`:168
- Brief: n/a
- Parameters:
  - `<unnamed>` (const OutboxRelay &): n/a

#### `size_t purgePublished()`
- Source: `include/cdc/outbox.h`:202
- Brief: Purge Published.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: scanRecords(), removeRecord().

#### `size_t relayOnce()`
- Source: `include/cdc/outbox.h`:184
- Brief: Relay Once.
- Parameters: none
- Return: Return value.
- Details: ============================================================ OutboxRelay::relayOnce ============================================================ Return value. Return value. Calls: scanRecords(), empty(), recordEvent(), std::chrono::system_clock::now(), time_since_epoch(), count(), updateRecord(), fetch_add().

#### `void relayThreadFunc()`
- Source: `include/cdc/outbox.h`:242
- Brief: Relay Thread Func.
- Parameters: none
- Details: Calls: load(), relayOnce(), lock(), wait_for().

#### `bool removeRecord(uint64_t outbox_sequence)`
- Source: `include/cdc/outbox.h`:196
- Brief: Remove Record.
- Parameters:
  - `outbox_sequence` (uint64_t): Input parameter.
- Return: True when the operation succeeds.
- Details: outbox_sequence Input parameter. True when the operation succeeds. outbox_sequence Input parameter. True when the operation succeeds. Calls: makeKey(), Delete(), ok().

#### `std::vector< OutboxRecord > scanRecords(size_t limit, OutboxState filter_state, bool all_states) const`
- Source: `include/cdc/outbox.h`:255
- Brief: Scan Records.
- Parameters:
  - `limit` (size_t): Input parameter.
  - `filter_state` (OutboxState): Input parameter.
  - `all_states` (bool): Input parameter.
- Return: Return value.
- Details: limit Input parameter. filter_state Input parameter. all_states Input parameter. Return value.

#### `void start()`
- Source: `include/cdc/outbox.h`:173
- Brief: Start.
- Parameters: none
- Details: Calls: compare_exchange_strong(), std::thread(), THEMIS_INFO(), count().

#### `void stop()`
- Source: `include/cdc/outbox.h`:178
- Brief: Stop.
- Parameters: none
- Details: Calls: exchange(), notify_all(), joinable(), join(), THEMIS_INFO().

#### `uint64_t totalFailed() const`
- Source: `include/cdc/outbox.h`:214
- Brief: Total Failed.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `uint64_t totalRelayed() const`
- Source: `include/cdc/outbox.h`:208
- Brief: Total Relayed.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void updateRecord(const OutboxRecord &rec)`
- Source: `include/cdc/outbox.h`:247
- Brief: Update Record.
- Parameters:
  - `rec` (const OutboxRecord &): Input parameter.
- Details: rec Input parameter. rec Input parameter. Calls: makeKey(), toJson(), dump(), Put(), ok(), THEMIS_WARN(), ToString().

#### `~OutboxRelay()`
- Source: `include/cdc/outbox.h`:165
- Brief: n/a
- Parameters: none

### themis::cdc::OutboxWriter

#### `OutboxWriter(const OutboxWriter &)=delete`
- Source: `include/cdc/outbox.h`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (const OutboxWriter &): n/a

#### `OutboxWriter(rocksdb::TransactionDB *db, rocksdb::ColumnFamilyHandle *cf=nullptr)`
- Source: `include/cdc/outbox.h`:105
- Brief: n/a
- Parameters:
  - `db` (rocksdb::TransactionDB *): n/a
  - `cf` (rocksdb::ColumnFamilyHandle *): n/a

#### `std::string makeKey(uint64_t seq) const`
- Source: `include/cdc/outbox.h`:134
- Brief: Make Key.
- Parameters:
  - `seq` (uint64_t): Input parameter.
- Return: Return value.
- Details: seq Input parameter. Return value.

#### `uint64_t nextSequence()`
- Source: `include/cdc/outbox.h`:139
- Brief: Next Sequence.
- Parameters: none
- Return: Return value.
- Throws:
  - CDCException: if an error occurs.
- Details: Return value. Return value. CDCException if an error occurs. Calls: lock(), Get(), ok(), std::stoull(), std::to_string(), Put(), ToString().

#### `OutboxWriter & operator=(const OutboxWriter &)=delete`
- Source: `include/cdc/outbox.h`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (const OutboxWriter &): n/a

#### `OutboxRecord & writeToOutbox(rocksdb::Transaction *txn, OutboxRecord &rec)`
- Source: `include/cdc/outbox.h`:119
- Brief: Write To Outbox.
- Parameters:
  - `txn` (rocksdb::Transaction *): Input/output parameter.
  - `rec` (OutboxRecord &): Input/output parameter.
- Return: Return value.
- Details: txn Input/output parameter. rec Input/output parameter. Return value.

#### `~OutboxWriter()=default`
- Source: `include/cdc/outbox.h`:108
- Brief: n/a
- Parameters: none

### themis::cdc::PredicateFilter

#### `PredicateFilter(std::string name, Predicate pred)`
- Source: `include/cdc/icdc_filter_pipeline.h`:75
- Brief: n/a
- Parameters:
  - `name` (std::string): n/a
  - `pred` (Predicate): n/a

#### `FilterResult evaluate(const Changefeed::ChangeEvent &event) const noexcept override`
- Source: `include/cdc/icdc_filter_pipeline.h`:78
- Brief: n/a
- Parameters:
  - `event` (const Changefeed::ChangeEvent &): n/a

#### `std::string name() const override`
- Source: `include/cdc/icdc_filter_pipeline.h`:88
- Brief: n/a
- Parameters: none

### themis::cdc::PurgeResult

#### `nlohmann::json toJson() const`
- Source: `include/cdc/cdc_admin.h`:35
- Brief: n/a
- Parameters: none

### themis::cdc::RetentionStatus

#### `nlohmann::json toJson() const`
- Source: `include/cdc/cdc_admin.h`:126
- Brief: n/a
- Parameters: none

### themis::cdc::SchemaInfo

#### `bool is_valid() const noexcept`
- Source: `include/cdc/schema_registry.h`:96
- Brief: n/a
- Parameters: none

### themis::cdc::SchemaRegistryClient

#### `SchemaRegistryClient(SchemaRegistryClient &&) noexcept=default`
- Source: `include/cdc/schema_registry.h`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaRegistryClient &&): n/a

#### `SchemaRegistryClient(SchemaRegistryConfig config={}, std::shared_ptr< ISchemaRegistryBackend > backend=nullptr)`
- Source: `include/cdc/schema_registry.h`:253
- Brief: n/a
- Parameters:
  - `config` (SchemaRegistryConfig): n/a
  - `backend` (std::shared_ptr< ISchemaRegistryBackend >): n/a

#### `SchemaRegistryClient(const SchemaRegistryClient &)=delete`
- Source: `include/cdc/schema_registry.h`:262
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaRegistryClient &): n/a

#### `ISchemaRegistryBackend * backend() const noexcept`
- Source: `include/cdc/schema_registry.h`:369
- Brief: n/a
- Parameters: none

#### `TimePoint cacheExpiry() const`
- Source: `include/cdc/schema_registry.h`:381
- Brief: n/a
- Parameters: none

#### `void clearCache()`
- Source: `include/cdc/schema_registry.h`:361
- Brief: Clear Cache.
- Parameters: none
- Details: Calls: lock(), clear().

#### `const SchemaRegistryConfig & config() const noexcept`
- Source: `include/cdc/schema_registry.h`:367
- Brief: n/a
- Parameters: none

#### `int32_t ensureSchema(const std::string &subject, const std::string &schema_json, SchemaFormat format)`
- Source: `include/cdc/schema_registry.h`:275
- Brief: Ensure Schema.
- Parameters:
  - `subject` (const std::string &): Input parameter.
  - `schema_json` (const std::string &): Input parameter.
  - `format` (SchemaFormat): Input parameter.
- Return: Return value.
- Details: subject Input parameter. schema_json Input parameter. format Input parameter. Return value. Calls: lock(), find(), end(), isCacheExpired(), registerSchema(), getById(), cacheExpiry().

#### `std::optional< SchemaInfo > getLatestSchema(const std::string &subject) const`
- Source: `include/cdc/schema_registry.h`:329
- Brief: n/a
- Parameters:
  - `subject` (const std::string &): n/a

#### `std::optional< SchemaInfo > getSchema(int32_t id) const`
- Source: `include/cdc/schema_registry.h`:302
- Brief: n/a
- Parameters:
  - `id` (int32_t): n/a

#### `bool isCacheExpired(const TimePoint &expiry)`
- Source: `include/cdc/schema_registry.h`:394
- Brief: Is Cache Expired.
- Parameters:
  - `expiry` (const TimePoint &): Input parameter.
- Return: True when the operation succeeds.
- Details: expiry Input parameter. True when the operation succeeds. Calls: std::chrono::steady_clock::now().

#### `SchemaRegistryClient & operator=(SchemaRegistryClient &&) noexcept=default`
- Source: `include/cdc/schema_registry.h`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaRegistryClient &&): n/a

#### `SchemaRegistryClient & operator=(const SchemaRegistryClient &)=delete`
- Source: `include/cdc/schema_registry.h`:263
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SchemaRegistryClient &): n/a

### themis::cdc::ScopedTimer

#### `ScopedTimer(LatencyHistogram &histogram)`
- Source: `include/cdc/cdc_metrics.h`:322
- Brief: n/a
- Parameters:
  - `histogram` (LatencyHistogram &): n/a

#### `~ScopedTimer()`
- Source: `include/cdc/cdc_metrics.h`:326
- Brief: n/a
- Parameters: none

### themis::cdc::TenantBufferManager

#### `TenantBufferManager(Changefeed *changefeed, const ChangefeedBufferConfig &default_config=ChangefeedBufferConfig())`
- Source: `include/cdc/tenant_buffer_manager.h`:101
- Brief: n/a
- Parameters:
  - `changefeed` (Changefeed *): n/a
  - `default_config` (const ChangefeedBufferConfig &): n/a

#### `bool checkTenantQuota(const std::string &tenant_id, TenantBufferState &state)`
- Source: `include/cdc/tenant_buffer_manager.h`:237
- Brief: Check Tenant Quota.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
  - `state` (TenantBufferState &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: tenant_id Identifier of the tenant. state Input/output parameter. True when the operation succeeds. tenant_id Identifier of the tenant. state Input/output parameter. True when the operation succeeds. Calls: getStats(), THEMIS_WARN().

#### `void configureTenant(const TenantConfig &config)`
- Source: `include/cdc/tenant_buffer_manager.h`:142
- Brief: Configure Tenant.
- Parameters:
  - `config` (const TenantConfig &): Input parameter.
- Throws:
  - error::invalidArgument: if an error occurs.
- Details: config Input parameter. config Input parameter. error::invalidArgument if an error occurs. Calls: empty(), lock(), find(), end(), THEMIS_INFO(), start(), try_emplace(), std::move().

#### `void disableTenant(const std::string &tenant_id)`
- Source: `include/cdc/tenant_buffer_manager.h`:190
- Brief: Disable Tenant.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Details: tenant_id Identifier of the tenant. tenant_id Identifier of the tenant. Calls: lock(), find(), end(), stop(), THEMIS_INFO().

#### `void enableTenant(const std::string &tenant_id)`
- Source: `include/cdc/tenant_buffer_manager.h`:196
- Brief: Enable Tenant.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Details: tenant_id Identifier of the tenant. tenant_id Identifier of the tenant. Calls: lock(), find(), end(), start(), THEMIS_INFO().

#### `size_t flushAll()`
- Source: `include/cdc/tenant_buffer_manager.h`:136
- Brief: Flush All.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: lock(), flush(), updateTenantStats().

#### `size_t flushTenant(const std::string &tenant_id)`
- Source: `include/cdc/tenant_buffer_manager.h`:130
- Brief: Flush Tenant.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. Return value. tenant_id Identifier of the tenant. Return value. Calls: lock(), find(), end(), flush(), updateTenantStats().

#### `std::vector< std::string > getActiveTenants() const`
- Source: `include/cdc/tenant_buffer_manager.h`:177
- Brief: Get Active Tenants.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::map< std::string, TenantStats > getAllTenantStats() const`
- Source: `include/cdc/tenant_buffer_manager.h`:171
- Brief: n/a
- Parameters: none

#### `nlohmann::json getGlobalMetrics() const`
- Source: `include/cdc/tenant_buffer_manager.h`:169
- Brief: Get Global Metrics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `TenantBufferState & getOrCreateTenantBuffer(const std::string &tenant_id)`
- Source: `include/cdc/tenant_buffer_manager.h`:230
- Brief: Get Or Create Tenant Buffer.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. Return value.

#### `std::optional< TenantConfig > getTenantConfig(const std::string &tenant_id) const`
- Source: `include/cdc/tenant_buffer_manager.h`:149
- Brief: Get Tenant Config.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. Return value.

#### `std::optional< std::reference_wrapper< const CDCMetrics > > getTenantMetrics(const std::string &tenant_id) const`
- Source: `include/cdc/tenant_buffer_manager.h`:163
- Brief: Get Tenant Metrics.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. Return value.

#### `std::optional< TenantStats > getTenantStats(const std::string &tenant_id) const`
- Source: `include/cdc/tenant_buffer_manager.h`:156
- Brief: Get Tenant Stats.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Return: Return value.
- Details: tenant_id Identifier of the tenant. Return value.

#### `bool hasTenant(const std::string &tenant_id) const`
- Source: `include/cdc/tenant_buffer_manager.h`:184
- Brief: Has Tenant.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Return: True when the operation succeeds.
- Details: tenant_id Identifier of the tenant. True when the operation succeeds.

#### `Changefeed::ChangeEvent recordEvent(const std::string &tenant_id, Changefeed::ChangeEvent event)`
- Source: `include/cdc/tenant_buffer_manager.h`:122
- Brief: Record Event.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
  - `event` (Changefeed::ChangeEvent): Input parameter.
- Return: Return value.
- Throws:
  - CDCException: if an error occurs.
  - error::invalidArgument: if an error occurs.
- Details: tenant_id Identifier of the tenant. event Input parameter. Return value. tenant_id Identifier of the tenant. event Input parameter. Return value. CDCException if an error occurs. error::invalidArgument if an error occurs. Calls: empty(), lock(), getOrCreateTenantBuffer(), checkTenantQuota(), std::move(), std::chrono::steady_clock::now(), updateTenantStats().

#### `void removeTenant(const std::string &tenant_id)`
- Source: `include/cdc/tenant_buffer_manager.h`:202
- Brief: Remove Tenant.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Details: tenant_id Identifier of the tenant. tenant_id Identifier of the tenant. Calls: lock(), find(), end(), flush(), stop(), erase(), THEMIS_INFO().

#### `void start()`
- Source: `include/cdc/tenant_buffer_manager.h`:109
- Brief: Start.
- Parameters: none
- Details: Calls: exchange(), THEMIS_WARN(), lock(), THEMIS_INFO(), size().

#### `void stop()`
- Source: `include/cdc/tenant_buffer_manager.h`:114
- Brief: Stop.
- Parameters: none
- Details: Calls: exchange(), lock(), THEMIS_INFO().

#### `void updateTenantStats(const std::string &tenant_id, TenantBufferState &state)`
- Source: `include/cdc/tenant_buffer_manager.h`:243
- Brief: Update Tenant Stats.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
  - `state` (TenantBufferState &): Input/output parameter.
- Details: tenant_id Identifier of the tenant. state Input/output parameter. param Input parameter. state Input/output parameter. Calls: getStats(), getMetrics(), eventsPerSecond().

#### `~TenantBufferManager() noexcept`
- Source: `include/cdc/tenant_buffer_manager.h`:104
- Brief: n/a
- Parameters: none

### themis::cdc::TenantStats

#### `nlohmann::json toJson() const`
- Source: `include/cdc/tenant_buffer_manager.h`:80
- Brief: n/a
- Parameters: none

### themis::cdc::ThroughputTracker

#### `ThroughputTracker()`
- Source: `include/cdc/cdc_metrics.h`:182
- Brief: n/a
- Parameters: none

#### `double bytesPerSecond() const`
- Source: `include/cdc/cdc_metrics.h`:199
- Brief: n/a
- Parameters: none

#### `double eventsPerSecond() const`
- Source: `include/cdc/cdc_metrics.h`:193
- Brief: n/a
- Parameters: none

#### `void recordEvent(size_t bytes=0)`
- Source: `include/cdc/cdc_metrics.h`:187
- Brief: n/a
- Parameters:
  - `bytes` (size_t): n/a

#### `void reset()`
- Source: `include/cdc/cdc_metrics.h`:218
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Calls: std::chrono::steady_clock::now().

#### `void resetWindowIfNeeded()`
- Source: `include/cdc/cdc_metrics.h`:235
- Brief: Reset Window If Needed.
- Parameters: none
- Details: Calls: std::chrono::steady_clock::now(), reset().

#### `nlohmann::json toJson() const`
- Source: `include/cdc/cdc_metrics.h`:205
- Brief: n/a
- Parameters: none

### themis::cdc::TimestampMergePolicy

#### `void merge(std::vector< FanInEvent > &events) const override`
- Source: `include/cdc/icdc_fan_in.h`:82
- Brief: Merge.
- Parameters:
  - `events` (std::vector< FanInEvent > &): Input/output parameter.
- Details: events Input/output parameter.

### themis::cdc::error

#### `CDCException bufferOverflow(size_t currentSize, size_t maxSize)`
- Source: `include/cdc/cdc_error.h`:279
- Brief: Buffer Overflow.
- Parameters:
  - `currentSize` (size_t): Input parameter.
  - `maxSize` (size_t): Input parameter.
- Return: Return value.
- Details: currentSize Input parameter. maxSize Input parameter. Return value. Calls: CDCException(), std::to_string().

#### `CDCException compressionFailed(const std::string &details)`
- Source: `include/cdc/cdc_error.h`:293
- Brief: Compression Failed.
- Parameters:
  - `details` (const std::string &): Input parameter.
- Return: Return value.
- Details: details Input parameter. Return value. Calls: CDCException().

#### `CDCException dbOperationFailed(const std::string &operation, const std::string &details)`
- Source: `include/cdc/cdc_error.h`:350
- Brief: Db Operation Failed.
- Parameters:
  - `operation` (const std::string &): Input parameter.
  - `details` (const std::string &): Input parameter.
- Return: Return value.
- Details: operation Input parameter. details Input parameter. Return value. Calls: CDCException().

#### `CDCException decompressionFailed(const std::string &details)`
- Source: `include/cdc/cdc_error.h`:306
- Brief: Decompression Failed.
- Parameters:
  - `details` (const std::string &): Input parameter.
- Return: Return value.
- Details: details Input parameter. Return value. Calls: CDCException().

#### `CDCException eventRecordFailed(const std::string &details)`
- Source: `include/cdc/cdc_error.h`:265
- Brief: Event Record Failed.
- Parameters:
  - `details` (const std::string &): Input parameter.
- Return: Return value.
- Details: details Input parameter. Return value. Calls: CDCException().

#### `CDCException internalError(const std::string &message)`
- Source: `include/cdc/cdc_error.h`:390
- Brief: Internal Error.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: CDCException().

#### `CDCException invalidArgument(const std::string &argName, const std::string &reason)`
- Source: `include/cdc/cdc_error.h`:364
- Brief: Invalid Argument.
- Parameters:
  - `argName` (const std::string &): Input parameter.
  - `reason` (const std::string &): Input parameter.
- Return: Return value.
- Details: argName Input parameter. reason Input parameter. Return value. Calls: CDCException().

#### `CDCException invalidArgument(const std::string &message)`
- Source: `include/cdc/cdc_error.h`:377
- Brief: Invalid Argument.
- Parameters:
  - `message` (const std::string &): Input parameter.
- Return: Return value.
- Details: message Input parameter. Return value. Calls: CDCException().

#### `CDCException rateLimitExceeded(size_t current, size_t limit)`
- Source: `include/cdc/cdc_error.h`:335
- Brief: Rate Limit Exceeded.
- Parameters:
  - `current` (size_t): Input parameter.
  - `limit` (size_t): Input parameter.
- Return: Return value.
- Details: current Input parameter. limit Input parameter. Return value. Calls: CDCException(), std::to_string().

#### `CDCException retryExhausted(int attempts, const std::string &lastError)`
- Source: `include/cdc/cdc_error.h`:320
- Brief: Retry Exhausted.
- Parameters:
  - `attempts` (int): Input parameter.
  - `lastError` (const std::string &): Input parameter.
- Return: Return value.
- Details: attempts Input parameter. lastError Input parameter. Return value. Calls: CDCException(), std::to_string().

#### `CDCException sequenceGenerationFailed(const std::string &details)`
- Source: `include/cdc/cdc_error.h`:252
- Brief: Sequence Generation Failed.
- Parameters:
  - `details` (const std::string &): Input parameter.
- Return: Return value.
- Details: details Input parameter. Return value. Calls: CDCException().

