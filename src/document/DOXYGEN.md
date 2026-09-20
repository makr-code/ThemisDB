# DOCUMENT DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\document\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\document\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 22
- Compounds: 110
- Classes/Structs: 62
- Namespaces: 17
- File Compounds: 22

## Namespaces
- @015175111354113357057072157276114167326264004271
- @204101236166254174375162334033312342061104312017
- @254127034173365265316211035366007201303376131160
- @357147206375076227265067316153262347120335274102
- benchmark
- std
- std::chrono_literals
- testing
- themis
- themis::bench
- themis::bench::document
- themis::bench::document::@050077026330212046304003327173010222210324264345
- themis::bench::document::@205004161003017302230133161212175021140214147372
- themis::bench::document::@333054231171311064233006203016210332213173023313
- themis::document
- themis::document::@322367247162237031313162321325154054105014164004
- themis::errors

## Types
### Classes
- DocumentDiffMergeTest
- DocumentLifecycleTest
- DocumentManagerTest
- DocumentSchemaEvolutionTest
- DocumentStoreTest
- FailingDocumentStore
- MergeEdgeTest
- MultiBranchMergeTest
- OperatorDiagnosticsTest
- RoundTripDiagnosticsTest
- RoundTripEdgeTest
- RoundTripPersistenceTest
- SchemaEdgeTest
- SchemaSealTest
- SchemaSealVersionTest
- StressConflictTest
- StubDocumentStore
- XDOMEAExportTest
- XDOMEAImportTest
- XDOMEAMultiVersionTest
- XDOMEAStoreTest
- themis::bench::document::DocumentDiffMergeFixture
- themis::bench::document::DocumentStoreFixture
- themis::bench::document::RoundTripFixture
- themis::document::DocumentDiagnosticSink
- themis::document::IDocumentDiffMerge
- themis::document::IDocumentLifecycleHook
- themis::document::IDocumentManager
- themis::document::IDocumentSchemaEvolution
- themis::document::IDocumentStore
- themis::document::IEncryptedDocumentEntity
- themis::document::IRoundTripEditor
- themis::document::IXDOMEAConnector
- themis::document::InMemoryDocumentDiffMerge
- themis::document::InMemoryDocumentManager
- themis::document::InMemoryDocumentSchemaEvolution
- themis::document::InMemoryDocumentStore
- themis::document::InMemoryEncryptedEntity
- themis::document::InMemoryXDOMEAConnector
- themis::document::StoreBackedRoundTripEditor

### Structs
- MergeResult
- StubDocumentStore::Entry
- TestHook
- std::hash< themis::document::DocumentErrorClass >
- themis::Customer
- themis::SecureDocument
- themis::User
- themis::document::DocumentDiff
- themis::document::DocumentLifecycleEvent
- themis::document::DocumentRecord
- themis::document::FieldChange
- themis::document::FieldViolation
- themis::document::KeyRotationDescriptor
- themis::document::MergeConflict
- themis::document::MergeResult
- themis::document::RoundTripSnapshot
- themis::document::SchemaDescriptor
- themis::document::SchemaFieldDescriptor
- themis::document::ValidationReport
- themis::document::XDOMEADocument
- themis::document::XDOMEAExportResult
- themis::document::XDOMEAImportResult

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 378

### DocumentDiffMergeTest

#### `void put(const std::string &id, const nlohmann::json &body)`
- Source: `tests/document/test_document_store.cpp`:496
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `body` (const nlohmann::json &): n/a

### DocumentLifecycleTest

#### `void SetUp() override`
- Source: `tests/document/test_document_store.cpp`:349
- Brief: n/a
- Parameters: none

### FailingDocumentStore

#### `Result< std::size_t > count(const CollectionId &) const override`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CollectionId &): n/a

#### `Result< std::optional< DocumentRecord > > get(const CollectionId &, const DocumentId &) const override`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CollectionId &): n/a
  - `<unnamed>` (const DocumentId &): n/a

#### `Result< std::vector< DocumentId > > list(const CollectionId &) const override`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CollectionId &): n/a

#### `Result< DocumentId > put(const DocumentRecord &) override`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DocumentRecord &): n/a

#### `Result< void > remove(const CollectionId &, const DocumentId &) override`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CollectionId &): n/a
  - `<unnamed>` (const DocumentId &): n/a

#### `Result< void > update(const CollectionId &, const DocumentId &, const nlohmann::json &) override`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CollectionId &): n/a
  - `<unnamed>` (const DocumentId &): n/a
  - `<unnamed>` (const nlohmann::json &): n/a

### MergeEdgeTest

#### `void put(const std::string &id, const nlohmann::json &body)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:370
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `body` (const nlohmann::json &): n/a

### MultiBranchMergeTest

#### `void SetUp() override`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:120
- Brief: n/a
- Parameters: none

### OperatorDiagnosticsTest

#### `void SetUp() override`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:528
- Brief: n/a
- Parameters: none

### SchemaSealVersionTest

#### `void SetUp() override`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:376
- Brief: n/a
- Parameters: none

#### `void registerMinimalVersion(SchemaVersion version, const std::string &field_name)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:382
- Brief: Helper — register a minimal single-field schema at version.
- Parameters:
  - `version` (SchemaVersion): n/a
  - `field_name` (const std::string &): n/a

### StressConflictTest

#### `nlohmann::json makeDoc50(int base_val)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:644
- Brief: Build a 50-field JSON object: {"f0": base_val, "f1": base_val+1, …}.
- Parameters:
  - `base_val` (int): n/a

#### `void put(const std::string &id, const nlohmann::json &body)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:652
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `body` (const nlohmann::json &): n/a

### StubDocumentStore

#### `bool read(const std::string &key, Entry &out) const`
- Source: `tests/document/test_document_highcardinality_stress.cpp`:73
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `out` (Entry &): n/a

#### `uint64_t readCount() const noexcept`
- Source: `tests/document/test_document_highcardinality_stress.cpp`:93
- Brief: n/a
- Parameters: none

#### `bool remove(const std::string &key)`
- Source: `tests/document/test_document_highcardinality_stress.cpp`:82
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::size_t size() const`
- Source: `tests/document/test_document_highcardinality_stress.cpp`:87
- Brief: n/a
- Parameters: none

#### `bool write(const std::string &key, const std::string &content, uint32_t schema_ver=1)`
- Source: `tests/document/test_document_highcardinality_stress.cpp`:65
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `content` (const std::string &): n/a
  - `schema_ver` (uint32_t): n/a

#### `uint64_t writeCount() const noexcept`
- Source: `tests/document/test_document_highcardinality_stress.cpp`:92
- Brief: n/a
- Parameters: none

### TestHook

#### `void afterCreate(const DocumentLifecycleEvent &evt) noexcept override`
- Source: `tests/document/test_document_store.cpp`:326
- Brief: After Create.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

#### `void afterDelete(const DocumentLifecycleEvent &evt) noexcept override`
- Source: `tests/document/test_document_store.cpp`:338
- Brief: After Delete.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

#### `void afterUpdate(const DocumentLifecycleEvent &evt) noexcept override`
- Source: `tests/document/test_document_store.cpp`:332
- Brief: After Update.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

#### `void beforeCreate(const DocumentLifecycleEvent &evt) noexcept override`
- Source: `tests/document/test_document_store.cpp`:323
- Brief: Before Create.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

#### `void beforeDelete(const DocumentLifecycleEvent &evt) noexcept override`
- Source: `tests/document/test_document_store.cpp`:335
- Brief: Before Delete.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

#### `void beforeUpdate(const DocumentLifecycleEvent &evt) noexcept override`
- Source: `tests/document/test_document_store.cpp`:329
- Brief: Before Update.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

### XDOMEAMultiVersionTest

#### `void SetUp() override`
- Source: `tests/document/test_document_xdomea_focused.cpp`:591
- Brief: n/a
- Parameters: none

### bench_document_serialization_gates.cpp

#### `BENCHMARK(BM_DOCBM01_SerializationP95) -> UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_DOCBM01_SerializationP95): n/a

#### `BENCHMARK(BM_DOCBM02_ListReadP95) -> UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_DOCBM02_ListReadP95): n/a

#### `BENCHMARK(BM_DOCBM03_DiffP95) -> UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_DOCBM03_DiffP95): n/a

#### `BENCHMARK(BM_DOCBM04_MergeP95) -> UseRealTime() ->MinTime(1.0)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_DOCBM04_MergeP95): n/a

#### `void BM_DOCBM01_SerializationP95(benchmark::State &state)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:123
- Brief: DOC-BM-01 — document serialization p95 latency gate.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Serializes a single document (key + ~200-byte JSON body) per iteration. Gate: p95 latency ≤ 500 µs. Counter: "gate_threshold_serialization_p95_us"

#### `void BM_DOCBM02_ListReadP95(benchmark::State &state)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:146
- Brief: DOC-BM-02 — document list-read p95 throughput gate.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Scans a pre-built list of 1000 document entries per iteration. Gate: throughput ≥ 50 000 ops/s. Counter: "gate_threshold_list_read_p95_ops_per_sec"

#### `void BM_DOCBM03_DiffP95(benchmark::State &state)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:175
- Brief: DOC-BM-03 — document diff p95 latency gate.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Computes a diff between two distinct document bodies per iteration. Gate: p95 latency ≤ 1 000 µs. Counter: "gate_threshold_diff_p95_us"

#### `void BM_DOCBM04_MergeP95(benchmark::State &state)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:198
- Brief: DOC-BM-04 — document merge p95 latency gate.
- Parameters:
  - `state` (benchmark::State &): n/a
- Details: Performs a three-way (base + two branches) merge per iteration. Gate: p95 latency ≤ 2 000 µs. Counter: "gate_threshold_merge_p95_us"

#### `std::string makeDocBody(uint64_t idx)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:72
- Brief: Build a deterministic document JSON body of ~200 bytes.
- Parameters:
  - `idx` (uint64_t): n/a

#### `std::string stubDiff(const std::string &from, const std::string &to)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:100
- Brief: Stub diff: produces a deterministic patch string for two distinct bodies.
- Parameters:
  - `from` (const std::string &): n/a
  - `to` (const std::string &): n/a

#### `std::size_t stubListRead(const std::vector< std::string > &entries)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:90
- Brief: Stub list-read: scans a vector of pre-built entries.
- Parameters:
  - `entries` (const std::vector< std::string > &): n/a

#### `std::string stubMerge(const std::string &base, const std::string &, const std::string &)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:108
- Brief: Stub merge: conflict-free three-way merge for non-overlapping field sets.
- Parameters:
  - `base` (const std::string &): n/a
  - `<unnamed>` (const std::string &): n/a
  - `<unnamed>` (const std::string &): n/a

#### `std::string stubSerialize(const std::string &key, const std::string &body)`
- Source: `benchmarks/document/bench_document_serialization_gates.cpp`:83
- Brief: Stub serializer: returns a serialized string (mirrors DocumentStore write path).
- Parameters:
  - `key` (const std::string &): n/a
  - `body` (const std::string &): n/a

### std::hash< themis::document::DocumentErrorClass >

#### `std::size_t operator()(themis::document::DocumentErrorClass cls) const noexcept`
- Source: `include/document/document_diagnostics.h`:77
- Brief: n/a
- Parameters:
  - `cls` (themis::document::DocumentErrorClass): n/a

### test_document_conflict_diagnostics_focused.cpp

#### `TEST_F(MultiBranchMergeTest, MBM01_OursWinsOnConflict)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiBranchMergeTest): n/a
  - `<unnamed>` (MBM01_OursWinsOnConflict): n/a
- Details: TestMBM-01: OURS_WINS resolves a conflicting field in favour of ours. Base has field "status"="pending". Ours changes it to "approved". Theirs changes it to "rejected". OURS_WINS must produce "approved".

#### `TEST_F(MultiBranchMergeTest, MBM02_TheirsWinsOnConflict)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:159
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiBranchMergeTest): n/a
  - `<unnamed>` (MBM02_TheirsWinsOnConflict): n/a
- Details: TestMBM-02: THEIRS_WINS resolves a conflicting field in favour of theirs.

#### `TEST_F(MultiBranchMergeTest, MBM03_FailStrategyReturnsConflictError)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiBranchMergeTest): n/a
  - `<unnamed>` (MBM03_FailStrategyReturnsConflictError): n/a
- Details: TestMBM-03: FAIL strategy returns ERR_DOC_MERGE_CONFLICT when fields diverge.

#### `TEST_F(MultiBranchMergeTest, MBM04_CleanMergeSucceedsUnderAllStrategies)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiBranchMergeTest): n/a
  - `<unnamed>` (MBM04_CleanMergeSucceedsUnderAllStrategies): n/a
- Details: TestMBM-04: Clean merge (no conflict) succeeds under all three strategies. Ours adds field "a"; theirs adds field "b"; base has neither. No shared field is modified, so all strategies must produce a clean merge.

#### `TEST_F(MultiBranchMergeTest, MBM05_PartialConflictMultiFieldOursWins)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiBranchMergeTest): n/a
  - `<unnamed>` (MBM05_PartialConflictMultiFieldOursWins): n/a
- Details: TestMBM-05: Multi-field document — partial conflict, OURS_WINS strategy. Base has three fields. Ours modifies field "c". Theirs modifies fields "b" and "c" (creating a conflict on "c"). OURS_WINS must preserve ours' "c" value and merge theirs' "b" change cleanly.

#### `TEST_F(MultiBranchMergeTest, MBM06_ChainedMergeConverges)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiBranchMergeTest): n/a
  - `<unnamed>` (MBM06_ChainedMergeConverges): n/a
- Details: TestMBM-06: Chained merge — three sequential branch merges converge cleanly. Models: A branched into B and C; C branched into D. Merge(A, B, C) → BC; Merge(A, BC, D) → final. All branches modify distinct fields, so no conflicts arise.

#### `TEST_F(MultiBranchMergeTest, MBM07_MissingBaseReturnsError)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiBranchMergeTest): n/a
  - `<unnamed>` (MBM07_MissingBaseReturnsError): n/a
- Details: TestMBM-07: Merge returns ERR_DOC_DIFF_NOT_FOUND when base document is absent.

#### `TEST_F(MultiBranchMergeTest, MBM08_SymmetricConflictNonCommutative)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:340
- Brief: n/a
- Parameters:
  - `<unnamed>` (MultiBranchMergeTest): n/a
  - `<unnamed>` (MBM08_SymmetricConflictNonCommutative): n/a
- Details: TestMBM-08: Symmetric conflict — swapping ours/theirs changes OURS_WINS outcome. Verifies that the merge result is not commutative when OURS_WINS is applied and that swapping the branch positions flips the resolved value.

#### `TEST_F(OperatorDiagnosticsTest, ODE01_ExchangeBoundaryViolatedClassifiesCorrectly)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:539
- Brief: n/a
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (ODE01_ExchangeBoundaryViolatedClassifiesCorrectly): n/a
- Details: TestODE-01: ERR_DOC_EXCHANGE_BOUNDARY_VIOLATED classifies as EXCHANGE_ERROR.

#### `TEST_F(OperatorDiagnosticsTest, ODE02_DiffNotFoundClassifiesAsExchangeError)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:550
- Brief: n/a
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (ODE02_DiffNotFoundClassifiesAsExchangeError): n/a
- Details: TestODE-02: ERR_DOC_DIFF_NOT_FOUND classifies as EXCHANGE_ERROR. diff() is the primary XDOMEA-adjacent operation that produces this error.

#### `TEST_F(OperatorDiagnosticsTest, ODE03_RoundTripPersistFailClassifiesCorrectly)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:559
- Brief: n/a
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (ODE03_RoundTripPersistFailClassifiesCorrectly): n/a
- Details: TestODE-03: ERR_DOC_ROUND_TRIP_PERSIST_FAIL classifies as ROUND_TRIP_ERROR.

#### `TEST_F(OperatorDiagnosticsTest, ODE04_SnapshotCollisionClassifiesAsRoundTripError)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:568
- Brief: n/a
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (ODE04_SnapshotCollisionClassifiesAsRoundTripError): n/a
- Details: TestODE-04: ERR_DOC_SNAPSHOT_COLLISION classifies as ROUND_TRIP_ERROR.

#### `TEST_F(OperatorDiagnosticsTest, ODE05_SinkCountsExchangeErrors)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:580
- Brief: n/a
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (ODE05_SinkCountsExchangeErrors): n/a
- Details: TestODE-05: DocumentDiagnosticSink correctly counts EXCHANGE_ERROR events. Records three exchange errors and verifies that count() returns 3 and totalCount() returns 3.

#### `TEST_F(OperatorDiagnosticsTest, ODE06_SinkCountsRoundTripErrors)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:593
- Brief: n/a
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (ODE06_SinkCountsRoundTripErrors): n/a
- Details: TestODE-06: DocumentDiagnosticSink correctly counts ROUND_TRIP_ERROR events.

#### `TEST_F(OperatorDiagnosticsTest, ODE07_MixedClassCountsAreIndependent)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:608
- Brief: n/a
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (ODE07_MixedClassCountsAreIndependent): n/a
- Details: TestODE-07: Mixed error-class recording produces correct per-class counts. Records events spanning EXCHANGE_ERROR, ROUND_TRIP_ERROR, and MERGE_CONFLICT. Verifies that each class counter is independent and the total is the sum.

#### `TEST_F(OperatorDiagnosticsTest, ODE08_DescriptionsNonEmptyForExchangeAndRoundTripCodes)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:629
- Brief: n/a
- Parameters:
  - `<unnamed>` (OperatorDiagnosticsTest): n/a
  - `<unnamed>` (ODE08_DescriptionsNonEmptyForExchangeAndRoundTripCodes): n/a
- Details: TestODE-08: documentErrorDescription returns a non-empty string for all EXCHANGE_ERROR and ROUND_TRIP_ERROR codes. Guards against silent gaps in the description map for the critical exchange and round-trip failure paths.

#### `TEST_F(SchemaSealVersionTest, SSV01_DuplicateVersionRegistrationRejected)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:397
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaSealVersionTest): n/a
  - `<unnamed>` (SSV01_DuplicateVersionRegistrationRejected): n/a
- Details: TestSSV-01: Registering the same version twice returns ERR_DOC_SCHEMA_VERSION_EXISTS.

#### `TEST_F(SchemaSealVersionTest, SSV02_ValidateUnknownVersionReturnsError)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:411
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaSealVersionTest): n/a
  - `<unnamed>` (SSV02_ValidateUnknownVersionReturnsError): n/a
- Details: TestSSV-02: Validating against an unknown version returns ERR_DOC_SCHEMA_VERSION_NOT_FOUND.

#### `TEST_F(SchemaSealVersionTest, SSV03_VersionsReturnedInAscendingOrder)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:426
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaSealVersionTest): n/a
  - `<unnamed>` (SSV03_VersionsReturnedInAscendingOrder): n/a
- Details: TestSSV-03: Version ordering — registeredVersions() returns versions in ascending order regardless of registration order.

#### `TEST_F(SchemaSealVersionTest, SSV04_MissingRequiredFieldProducesViolation)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:446
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaSealVersionTest): n/a
  - `<unnamed>` (SSV04_MissingRequiredFieldProducesViolation): n/a
- Details: TestSSV-04: Document with missing required field fails validation. Registers a schema with a required field "id". Validates a document body that omits the required field and expects a non-empty violations list.

#### `TEST_F(SchemaSealVersionTest, SSV05_ForwardVersionRegistrationValid)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:465
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaSealVersionTest): n/a
  - `<unnamed>` (SSV05_ForwardVersionRegistrationValid): n/a
- Details: TestSSV-05: Registering a new schema version after v1 is valid (forward-only). Verifies that v2 can be registered independently after v1 and that both versions coexist in the registry without interfering.

#### `TEST_F(SchemaSealVersionTest, SSV06_TypeMismatchProducesViolation)`
- Source: `tests/document/test_document_conflict_diagnostics_focused.cpp`:492
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaSealVersionTest): n/a
  - `<unnamed>` (SSV06_TypeMismatchProducesViolation): n/a
- Details: TestSSV-06: Type mismatch on a required field produces a violation. Schema expects field "count" to be of type NUMBER. Document provides a STRING — must result in a type violation.

### test_document_highcardinality_stress.cpp

#### `TEST(WaveD_DocumentHighCardinalityStress, ConcurrentSchemaVersionStress)`
- Source: `tests/document/test_document_highcardinality_stress.cpp`:201
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_DocumentHighCardinalityStress): n/a
  - `<unnamed>` (ConcurrentSchemaVersionStress): n/a

#### `TEST(WaveD_DocumentHighCardinalityStress, HighCardinalityDocumentCRUD)`
- Source: `tests/document/test_document_highcardinality_stress.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_DocumentHighCardinalityStress): n/a
  - `<unnamed>` (HighCardinalityDocumentCRUD): n/a

#### `TEST(WaveD_DocumentHighCardinalityStress, MergeConflictUnderLoad)`
- Source: `tests/document/test_document_highcardinality_stress.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_DocumentHighCardinalityStress): n/a
  - `<unnamed>` (MergeConflictUnderLoad): n/a

#### `MergeResult stubMerge(const std::string &base, const std::string &branch_a, const std::string &branch_b)`
- Source: `tests/document/test_document_highcardinality_stress.cpp`:111
- Brief: n/a
- Parameters:
  - `base` (const std::string &): n/a
  - `branch_a` (const std::string &): n/a
  - `branch_b` (const std::string &): n/a

### test_document_round_trip_persistence_focused.cpp

#### `TEST_F(RoundTripDiagnosticsTest, RTD01_BeginRelayStoreFailure_PropagatesError)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:330
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripDiagnosticsTest): n/a
  - `<unnamed>` (RTD01_BeginRelayStoreFailure_PropagatesError): n/a

#### `TEST_F(RoundTripDiagnosticsTest, RTD02_SaveInteractionStoreFailure_PropagatesError)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:338
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripDiagnosticsTest): n/a
  - `<unnamed>` (RTD02_SaveInteractionStoreFailure_PropagatesError): n/a

#### `TEST_F(RoundTripDiagnosticsTest, RTD03_LoadInteractionStoreFailure_PropagatesError)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:346
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripDiagnosticsTest): n/a
  - `<unnamed>` (RTD03_LoadInteractionStoreFailure_PropagatesError): n/a

#### `TEST_F(RoundTripDiagnosticsTest, RTD04_CountSnapshotsStoreListFailure_PropagatesError)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:354
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripDiagnosticsTest): n/a
  - `<unnamed>` (RTD04_CountSnapshotsStoreListFailure_PropagatesError): n/a

#### `TEST_F(RoundTripPersistenceTest, RTP01_BeginRelayStoresSeedAtIndexZero)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripPersistenceTest): n/a
  - `<unnamed>` (RTP01_BeginRelayStoresSeedAtIndexZero): n/a

#### `TEST_F(RoundTripPersistenceTest, RTP02_SaveInteractionAtIndex1_DistinctFromSeed)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:167
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripPersistenceTest): n/a
  - `<unnamed>` (RTP02_SaveInteractionAtIndex1_DistinctFromSeed): n/a

#### `TEST_F(RoundTripPersistenceTest, RTP03_CountSnapshotsConsistentWithSaves)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripPersistenceTest): n/a
  - `<unnamed>` (RTP03_CountSnapshotsConsistentWithSaves): n/a

#### `TEST_F(RoundTripPersistenceTest, RTP04_SnapshotIdsDeterministic)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripPersistenceTest): n/a
  - `<unnamed>` (RTP04_SnapshotIdsDeterministic): n/a

#### `TEST_F(RoundTripPersistenceTest, RTP05_StoreDirectlyConfirmsSnapshotKey)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripPersistenceTest): n/a
  - `<unnamed>` (RTP05_StoreDirectlyConfirmsSnapshotKey): n/a

#### `TEST_F(RoundTripPersistenceTest, RTP06_LoadIndex0_ReturnsSeedExactly)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripPersistenceTest): n/a
  - `<unnamed>` (RTP06_LoadIndex0_ReturnsSeedExactly): n/a

#### `TEST_F(RoundTripPersistenceTest, RTP07_LoadIndexN_ReturnsNthInteractionExactly)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripPersistenceTest): n/a
  - `<unnamed>` (RTP07_LoadIndexN_ReturnsNthInteractionExactly): n/a

#### `TEST_F(RoundTripPersistenceTest, RTP08_NonAlphanumericRelayId_ValidSnapshotIds)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripPersistenceTest): n/a
  - `<unnamed>` (RTP08_NonAlphanumericRelayId_ValidSnapshotIds): n/a

#### `TEST_F(RoundTripPersistenceTest, RTP09_LargeInteractionIndex_CorrectTenDigitId)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripPersistenceTest): n/a
  - `<unnamed>` (RTP09_LargeInteractionIndex_CorrectTenDigitId): n/a

#### `TEST_F(RoundTripPersistenceTest, RTP10_SnapshotBodyContainsAllRequiredFields)`
- Source: `tests/document/test_document_round_trip_persistence_focused.cpp`:288
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripPersistenceTest): n/a
  - `<unnamed>` (RTP10_SnapshotBodyContainsAllRequiredFields): n/a

### test_document_schema_hardening_focused.cpp

#### `TEST_F(MergeEdgeTest, ME01_BothBranchesDeleteField_CleanMergeFieldAbsent)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEdgeTest): n/a
  - `<unnamed>` (ME01_BothBranchesDeleteField_CleanMergeFieldAbsent): n/a

#### `TEST_F(MergeEdgeTest, ME02_OursDeletesTheirsModifies_ConflictReported)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEdgeTest): n/a
  - `<unnamed>` (ME02_OursDeletesTheirsModifies_ConflictReported): n/a

#### `TEST_F(MergeEdgeTest, ME03_TheirsAddsNewField_CleanMergeFieldPresent)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:410
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEdgeTest): n/a
  - `<unnamed>` (ME03_TheirsAddsNewField_CleanMergeFieldPresent): n/a

#### `TEST_F(MergeEdgeTest, ME04_BothAddSameFieldSameValue_CleanMerge)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:424
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEdgeTest): n/a
  - `<unnamed>` (ME04_BothAddSameFieldSameValue_CleanMerge): n/a

#### `TEST_F(MergeEdgeTest, ME05_BothAddSameFieldDifferentValues_Conflict)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:437
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEdgeTest): n/a
  - `<unnamed>` (ME05_BothAddSameFieldDifferentValues_Conflict): n/a

#### `TEST_F(MergeEdgeTest, ME06_TheirsWinsResolvesConflictWithTheirsValue)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:456
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEdgeTest): n/a
  - `<unnamed>` (ME06_TheirsWinsResolvesConflictWithTheirsValue): n/a

#### `TEST_F(MergeEdgeTest, ME07_FailStrategyAccumulatesAllConflicts)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:470
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEdgeTest): n/a
  - `<unnamed>` (ME07_FailStrategyAccumulatesAllConflicts): n/a

#### `TEST_F(MergeEdgeTest, ME08_EmptyBaseId_InvalidArgument)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:485
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEdgeTest): n/a
  - `<unnamed>` (ME08_EmptyBaseId_InvalidArgument): n/a

#### `TEST_F(MergeEdgeTest, ME09_AllThreeIdentical_CleanMergeEqualsBase)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:495
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEdgeTest): n/a
  - `<unnamed>` (ME09_AllThreeIdentical_CleanMergeEqualsBase): n/a

#### `TEST_F(MergeEdgeTest, ME10_NonObjectPayloads_ReturnsOursBody)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:509
- Brief: n/a
- Parameters:
  - `<unnamed>` (MergeEdgeTest): n/a
  - `<unnamed>` (ME10_NonObjectPayloads_ReturnsOursBody): n/a

#### `TEST_F(RoundTripEdgeTest, RTE01_BeginRelayTwiceSameId_AlreadyExists)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:539
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripEdgeTest): n/a
  - `<unnamed>` (RTE01_BeginRelayTwiceSameId_AlreadyExists): n/a

#### `TEST_F(RoundTripEdgeTest, RTE02_LoadNonExistentRelay_ReturnsNullopt)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:548
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripEdgeTest): n/a
  - `<unnamed>` (RTE02_LoadNonExistentRelay_ReturnsNullopt): n/a

#### `TEST_F(RoundTripEdgeTest, RTE03_SaveBeforeBeginRelay_Succeeds)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:556
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripEdgeTest): n/a
  - `<unnamed>` (RTE03_SaveBeforeBeginRelay_Succeeds): n/a

#### `TEST_F(RoundTripEdgeTest, RTE04_CountSnapshotsUnknownRelay_ReturnsZero)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:563
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripEdgeTest): n/a
  - `<unnamed>` (RTE04_CountSnapshotsUnknownRelay_ReturnsZero): n/a

#### `TEST_F(RoundTripEdgeTest, RTE05_CountSnapshotsAfterNInteractions_CorrectCount)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:570
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripEdgeTest): n/a
  - `<unnamed>` (RTE05_CountSnapshotsAfterNInteractions_CorrectCount): n/a

#### `TEST_F(RoundTripEdgeTest, RTE06_SaveSameIndexTwice_ReturnsError)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:586
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripEdgeTest): n/a
  - `<unnamed>` (RTE06_SaveSameIndexTwice_ReturnsError): n/a

#### `TEST_F(RoundTripEdgeTest, RTE07_SameIndexDifferentRelays_NoCollision)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:596
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripEdgeTest): n/a
  - `<unnamed>` (RTE07_SameIndexDifferentRelays_NoCollision): n/a

#### `TEST_F(RoundTripEdgeTest, RTE08_LoadInteraction_CorrectFields)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:613
- Brief: n/a
- Parameters:
  - `<unnamed>` (RoundTripEdgeTest): n/a
  - `<unnamed>` (RTE08_LoadInteraction_CorrectFields): n/a

#### `TEST_F(SchemaEdgeTest, SE01_EmptyBodyAllOptionalSchema_IsValid)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE01_EmptyBodyAllOptionalSchema_IsValid): n/a

#### `TEST_F(SchemaEdgeTest, SE02_NullValueForStringField_TypeMismatch)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE02_NullValueForStringField_TypeMismatch): n/a

#### `TEST_F(SchemaEdgeTest, SE03_ArrayForBooleanField_TypeMismatch)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE03_ArrayForBooleanField_TypeMismatch): n/a

#### `TEST_F(SchemaEdgeTest, SE04_ObjectForNumberField_TypeMismatch)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE04_ObjectForNumberField_TypeMismatch): n/a

#### `TEST_F(SchemaEdgeTest, SE05_AllRequiredFieldsMissing_ReportsAllViolations)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:158
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE05_AllRequiredFieldsMissing_ReportsAllViolations): n/a

#### `TEST_F(SchemaEdgeTest, SE06_ValidateOnSealedRegistry_StillWorks)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE06_ValidateOnSealedRegistry_StillWorks): n/a

#### `TEST_F(SchemaEdgeTest, SE07_RegisterVersionZero_Succeeds)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE07_RegisterVersionZero_Succeeds): n/a

#### `TEST_F(SchemaEdgeTest, SE08_RegisterVersionMaxUint32_Succeeds)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE08_RegisterVersionMaxUint32_Succeeds): n/a

#### `TEST_F(SchemaEdgeTest, SE09_TwoConsecutiveVersions_ListedAscending)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE09_TwoConsecutiveVersions_ListedAscending): n/a

#### `TEST_F(SchemaEdgeTest, SE10_IndependentInstancesAreIsolated)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE10_IndependentInstancesAreIsolated): n/a

#### `TEST_F(SchemaEdgeTest, SE11_ExtraFieldsInDocument_Valid)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:236
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE11_ExtraFieldsInDocument_Valid): n/a

#### `TEST_F(SchemaEdgeTest, SE12_AnyFieldType_NeverTypeMismatch)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaEdgeTest): n/a
  - `<unnamed>` (SE12_AnyFieldType_NeverTypeMismatch): n/a

#### `TEST_F(SchemaSealTest, SS01_SealIsIdempotent)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaSealTest): n/a
  - `<unnamed>` (SS01_SealIsIdempotent): n/a

#### `TEST_F(SchemaSealTest, SS02_SealPreservesRegisteredVersions)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaSealTest): n/a
  - `<unnamed>` (SS02_SealPreservesRegisteredVersions): n/a

#### `TEST_F(SchemaSealTest, SS03_RegisterAfterSeal_ReturnsSealedError)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:311
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaSealTest): n/a
  - `<unnamed>` (SS03_RegisterAfterSeal_ReturnsSealedError): n/a

#### `TEST_F(SchemaSealTest, SS04_VersionTransitionValidation_V1PassesV2Fails)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:322
- Brief: n/a
- Parameters:
  - `<unnamed>` (SchemaSealTest): n/a
  - `<unnamed>` (SS04_VersionTransitionValidation_V1PassesV2Fails): n/a

#### `TEST_F(StressConflictTest, SC01_FiftyConflicts_OursWins_AllOursValues)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:658
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressConflictTest): n/a
  - `<unnamed>` (SC01_FiftyConflicts_OursWins_AllOursValues): n/a

#### `TEST_F(StressConflictTest, SC02_FiftyConflicts_TheirsWins_AllTheirsValues)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:677
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressConflictTest): n/a
  - `<unnamed>` (SC02_FiftyConflicts_TheirsWins_AllTheirsValues): n/a

#### `TEST_F(StressConflictTest, SC03_FiftyConflicts_FailStrategy_ExactlyFiftyConflicts)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:696
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressConflictTest): n/a
  - `<unnamed>` (SC03_FiftyConflicts_FailStrategy_ExactlyFiftyConflicts): n/a

#### `TEST_F(StressConflictTest, SC04_SequentialMergeChain_ResultDeterministic)`
- Source: `tests/document/test_document_schema_hardening_focused.cpp`:711
- Brief: n/a
- Parameters:
  - `<unnamed>` (StressConflictTest): n/a
  - `<unnamed>` (SC04_SequentialMergeChain_ResultDeterministic): n/a

### test_document_store.cpp

#### `TEST_F(DocumentDiffMergeTest, DiffErrNotFoundForUnknownBase)`
- Source: `tests/document/test_document_store.cpp`:543
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (DiffErrNotFoundForUnknownBase): n/a

#### `TEST_F(DocumentDiffMergeTest, DiffIdenticalDocumentsEmpty)`
- Source: `tests/document/test_document_store.cpp`:502
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (DiffIdenticalDocumentsEmpty): n/a

#### `TEST_F(DocumentDiffMergeTest, DiffReportsAddedFields)`
- Source: `tests/document/test_document_store.cpp`:511
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (DiffReportsAddedFields): n/a

#### `TEST_F(DocumentDiffMergeTest, DiffReportsModifiedFields)`
- Source: `tests/document/test_document_store.cpp`:531
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (DiffReportsModifiedFields): n/a

#### `TEST_F(DocumentDiffMergeTest, DiffReportsRemovedFields)`
- Source: `tests/document/test_document_store.cpp`:521
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (DiffReportsRemovedFields): n/a

#### `TEST_F(DocumentDiffMergeTest, MergeCleanNonOverlapping)`
- Source: `tests/document/test_document_store.cpp`:551
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (MergeCleanNonOverlapping): n/a

#### `TEST_F(DocumentDiffMergeTest, MergeFailStrategyReturnsError)`
- Source: `tests/document/test_document_store.cpp`:589
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (MergeFailStrategyReturnsError): n/a

#### `TEST_F(DocumentDiffMergeTest, MergeListsConflicts)`
- Source: `tests/document/test_document_store.cpp`:564
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (MergeListsConflicts): n/a

#### `TEST_F(DocumentDiffMergeTest, MergeOursWinsResolvesConflicts)`
- Source: `tests/document/test_document_store.cpp`:577
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (MergeOursWinsResolvesConflicts): n/a

#### `TEST_F(DocumentDiffMergeTest, ReencryptErrInvalidArgumentForEmptyNewKey)`
- Source: `tests/document/test_document_store.cpp`:610
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (ReencryptErrInvalidArgumentForEmptyNewKey): n/a

#### `TEST_F(DocumentDiffMergeTest, ReencryptSucceedsWithValidKeyIds)`
- Source: `tests/document/test_document_store.cpp`:601
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (ReencryptSucceedsWithValidKeyIds): n/a

#### `TEST_F(DocumentDiffMergeTest, ReencryptUpdatesCurrentKeyId)`
- Source: `tests/document/test_document_store.cpp`:621
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiffMergeTest): n/a
  - `<unnamed>` (ReencryptUpdatesCurrentKeyId): n/a

#### `TEST_F(DocumentLifecycleTest, BeforeAndAfterCreateBothFire)`
- Source: `tests/document/test_document_store.cpp`:355
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentLifecycleTest): n/a
  - `<unnamed>` (BeforeAndAfterCreateBothFire): n/a

#### `TEST_F(DocumentLifecycleTest, BeforeAndAfterDeleteBothFire)`
- Source: `tests/document/test_document_store.cpp`:363
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentLifecycleTest): n/a
  - `<unnamed>` (BeforeAndAfterDeleteBothFire): n/a

#### `TEST_F(DocumentLifecycleTest, BeforeAndAfterUpdateBothFire)`
- Source: `tests/document/test_document_store.cpp`:374
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentLifecycleTest): n/a
  - `<unnamed>` (BeforeAndAfterUpdateBothFire): n/a

#### `TEST_F(DocumentLifecycleTest, UnregisteredHookNoLongerReceivesEvents)`
- Source: `tests/document/test_document_store.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentLifecycleTest): n/a
  - `<unnamed>` (UnregisteredHookNoLongerReceivesEvents): n/a

#### `TEST_F(DocumentManagerTest, CreateAndGet)`
- Source: `tests/document/test_document_store.cpp`:250
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentManagerTest): n/a
  - `<unnamed>` (CreateAndGet): n/a

#### `TEST_F(DocumentManagerTest, CreateEncryptedReturnsErrInvalidId)`
- Source: `tests/document/test_document_store.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentManagerTest): n/a
  - `<unnamed>` (CreateEncryptedReturnsErrInvalidId): n/a

#### `TEST_F(DocumentManagerTest, CreateEncryptedReturnsOpaqueHandle)`
- Source: `tests/document/test_document_store.cpp`:302
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentManagerTest): n/a
  - `<unnamed>` (CreateEncryptedReturnsOpaqueHandle): n/a

#### `TEST_F(DocumentManagerTest, CreateReturnsErrAlreadyExistsOnDuplicate)`
- Source: `tests/document/test_document_store.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentManagerTest): n/a
  - `<unnamed>` (CreateReturnsErrAlreadyExistsOnDuplicate): n/a

#### `TEST_F(DocumentManagerTest, CreateReturnsErrInvalidIdForEmpty)`
- Source: `tests/document/test_document_store.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentManagerTest): n/a
  - `<unnamed>` (CreateReturnsErrInvalidIdForEmpty): n/a

#### `TEST_F(DocumentManagerTest, RemoveClearsDocument)`
- Source: `tests/document/test_document_store.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentManagerTest): n/a
  - `<unnamed>` (RemoveClearsDocument): n/a

#### `TEST_F(DocumentManagerTest, UpdateReplacesBody)`
- Source: `tests/document/test_document_store.cpp`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentManagerTest): n/a
  - `<unnamed>` (UpdateReplacesBody): n/a

#### `TEST_F(DocumentManagerTest, UpdateReturnsErrNotFound)`
- Source: `tests/document/test_document_store.cpp`:285
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentManagerTest): n/a
  - `<unnamed>` (UpdateReturnsErrNotFound): n/a

#### `TEST_F(DocumentSchemaEvolutionTest, IsSealedTransition)`
- Source: `tests/document/test_document_store.cpp`:434
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentSchemaEvolutionTest): n/a
  - `<unnamed>` (IsSealedTransition): n/a

#### `TEST_F(DocumentSchemaEvolutionTest, RegisterVersionErrOnDuplicate)`
- Source: `tests/document/test_document_store.cpp`:417
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentSchemaEvolutionTest): n/a
  - `<unnamed>` (RegisterVersionErrOnDuplicate): n/a

#### `TEST_F(DocumentSchemaEvolutionTest, RegisterVersionSucceeds)`
- Source: `tests/document/test_document_store.cpp`:409
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentSchemaEvolutionTest): n/a
  - `<unnamed>` (RegisterVersionSucceeds): n/a

#### `TEST_F(DocumentSchemaEvolutionTest, SealedRegistryRejectsNewVersion)`
- Source: `tests/document/test_document_store.cpp`:425
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentSchemaEvolutionTest): n/a
  - `<unnamed>` (SealedRegistryRejectsNewVersion): n/a

#### `TEST_F(DocumentSchemaEvolutionTest, ValidateCompliantDocument)`
- Source: `tests/document/test_document_store.cpp`:441
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentSchemaEvolutionTest): n/a
  - `<unnamed>` (ValidateCompliantDocument): n/a

#### `TEST_F(DocumentSchemaEvolutionTest, ValidateErrUnknownVersion)`
- Source: `tests/document/test_document_store.cpp`:479
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentSchemaEvolutionTest): n/a
  - `<unnamed>` (ValidateErrUnknownVersion): n/a

#### `TEST_F(DocumentSchemaEvolutionTest, ValidateReportsMissingRequired)`
- Source: `tests/document/test_document_store.cpp`:450
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentSchemaEvolutionTest): n/a
  - `<unnamed>` (ValidateReportsMissingRequired): n/a

#### `TEST_F(DocumentSchemaEvolutionTest, ValidateReportsTypeMismatch)`
- Source: `tests/document/test_document_store.cpp`:462
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentSchemaEvolutionTest): n/a
  - `<unnamed>` (ValidateReportsTypeMismatch): n/a

#### `TEST_F(DocumentStoreTest, ConcurrentPutIsThreadSafe)`
- Source: `tests/document/test_document_store.cpp`:222
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (ConcurrentPutIsThreadSafe): n/a

#### `TEST_F(DocumentStoreTest, CountIsCollectionScoped)`
- Source: `tests/document/test_document_store.cpp`:213
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (CountIsCollectionScoped): n/a

#### `TEST_F(DocumentStoreTest, EmptyStoreCountZero)`
- Source: `tests/document/test_document_store.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (EmptyStoreCountZero): n/a

#### `TEST_F(DocumentStoreTest, GetReturnsCorrectDocumentAfterPut)`
- Source: `tests/document/test_document_store.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (GetReturnsCorrectDocumentAfterPut): n/a

#### `TEST_F(DocumentStoreTest, GetReturnsNulloptForUnknown)`
- Source: `tests/document/test_document_store.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (GetReturnsNulloptForUnknown): n/a

#### `TEST_F(DocumentStoreTest, ListReturnsAllIds)`
- Source: `tests/document/test_document_store.cpp`:196
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (ListReturnsAllIds): n/a

#### `TEST_F(DocumentStoreTest, ListReturnsEmptyForUnknownCollection)`
- Source: `tests/document/test_document_store.cpp`:206
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (ListReturnsEmptyForUnknownCollection): n/a

#### `TEST_F(DocumentStoreTest, PutIncrementsCount)`
- Source: `tests/document/test_document_store.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (PutIncrementsCount): n/a

#### `TEST_F(DocumentStoreTest, PutReturnsDocumentId)`
- Source: `tests/document/test_document_store.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (PutReturnsDocumentId): n/a

#### `TEST_F(DocumentStoreTest, PutReturnsErrAlreadyExistsOnDuplicate)`
- Source: `tests/document/test_document_store.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (PutReturnsErrAlreadyExistsOnDuplicate): n/a

#### `TEST_F(DocumentStoreTest, PutReturnsErrInvalidIdForEmptyId)`
- Source: `tests/document/test_document_store.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (PutReturnsErrInvalidIdForEmptyId): n/a

#### `TEST_F(DocumentStoreTest, RemoveDecreasesCount)`
- Source: `tests/document/test_document_store.cpp`:182
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (RemoveDecreasesCount): n/a

#### `TEST_F(DocumentStoreTest, RemoveIsNoOpForUnknown)`
- Source: `tests/document/test_document_store.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (RemoveIsNoOpForUnknown): n/a

#### `TEST_F(DocumentStoreTest, UpdateModifiesBody)`
- Source: `tests/document/test_document_store.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (UpdateModifiesBody): n/a

#### `TEST_F(DocumentStoreTest, UpdateReturnsErrNotFoundForMissing)`
- Source: `tests/document/test_document_store.cpp`:175
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentStoreTest): n/a
  - `<unnamed>` (UpdateReturnsErrNotFoundForMissing): n/a

#### `nlohmann::json makeBody(const std::string &tag="test")`
- Source: `tests/document/test_document_store.cpp`:89
- Brief: n/a
- Parameters:
  - `tag` (const std::string &): n/a

#### `SchemaDescriptor makeSchema()`
- Source: `tests/document/test_document_store.cpp`:400
- Brief: n/a
- Parameters: none

### test_document_xdomea_focused.cpp

#### `TEST_F(XDOMEAExportTest, XE01_V30ExportContainsVersionAttribute)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:473
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAExportTest): n/a
  - `<unnamed>` (XE01_V30ExportContainsVersionAttribute): n/a
- Details: TestXE-01: exportToXML V3.0 produces valid envelope with correct version attribute. The exported XML must contain the xdomea:version="3.0.0" attribute and the correct nachrichtentyp code for ERFASSUNG (0601).

#### `TEST_F(XDOMEAExportTest, XE02_V21ExportContainsVersionAttribute)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:491
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAExportTest): n/a
  - `<unnamed>` (XE02_V21ExportContainsVersionAttribute): n/a
- Details: TestXE-02: exportToXML V2.1 produces correct version attribute. The exported XML must contain the xdomea:version="2.1.0" attribute.

#### `TEST_F(XDOMEAExportTest, XE03_ExportContainsDocumentId)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:508
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAExportTest): n/a
  - `<unnamed>` (XE03_ExportContainsDocumentId): n/a
- Details: TestXE-03: exportToXML includes document ID in the XML output. The document's ID field must appear in the serialized output so a downstream importer can re-identify it.

#### `TEST_F(XDOMEAExportTest, XE04_XmlEscapingApplied)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:524
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAExportTest): n/a
  - `<unnamed>` (XE04_XmlEscapingApplied): n/a
- Details: TestXE-04: exportToXML escapes reserved XML characters in betreff. A betreff containing <, >, &, ' and " must be safely escaped in the XML output so the result is well-formed.

#### `TEST_F(XDOMEAExportTest, XE05_AussonderungMessageTypeCode)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:550
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAExportTest): n/a
  - `<unnamed>` (XE05_AussonderungMessageTypeCode): n/a
- Details: TestXE-05: exportToXML produces the correct nachrichtentyp code for AUSSONDERUNG. Validates that the message-type code mapping is applied correctly for a non-default message type.

#### `TEST_F(XDOMEAExportTest, XE06_EmptyDocumentListProducesEnvelope)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:567
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAExportTest): n/a
  - `<unnamed>` (XE06_EmptyDocumentListProducesEnvelope): n/a
- Details: TestXE-06: exportToXML empty document list produces valid empty envelope. An empty document vector must produce a valid XML envelope without error and with zero documents exported.

#### `TEST_F(XDOMEAImportTest, XI01_V30SingleDocumentSuccess)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:324
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAImportTest): n/a
  - `<unnamed>` (XI01_V30SingleDocumentSuccess): n/a
- Details: TestXI-01: importFromXML V3.0 single-document success path. Verifies that a well-formed single-document V3.0 XML is parsed into exactly one document with the correct ID, aktenzeichen, and version string.

#### `TEST_F(XDOMEAImportTest, XI02_V21SingleDocumentSuccess)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:343
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAImportTest): n/a
  - `<unnamed>` (XI02_V21SingleDocumentSuccess): n/a
- Details: TestXI-02: importFromXML V2.1 single-document success path. Verifies that a well-formed V2.1 XML produces xdomea_version = "2.1.0".

#### `TEST_F(XDOMEAImportTest, XI03_MultiObjectTypeParsing)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:360
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAImportTest): n/a
  - `<unnamed>` (XI03_MultiObjectTypeParsing): n/a
- Details: TestXI-03: importFromXML parses both <akte> and <dokument> elements. A fixture with one <akte> and one <dokument> must produce two documents with the correct object_type assignments.

#### `TEST_F(XDOMEAImportTest, XI04_ImportedDocumentsPersisted)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:386
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAImportTest): n/a
  - `<unnamed>` (XI04_ImportedDocumentsPersisted): n/a
- Details: TestXI-04: importFromXML persists parsed documents into the connector's store. After a successful import, documents must be retrievable by ID via getDocument().

#### `TEST_F(XDOMEAImportTest, XI05_EmptyXmlReturnsEmptyResult)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:401
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAImportTest): n/a
  - `<unnamed>` (XI05_EmptyXmlReturnsEmptyResult): n/a
- Details: TestXI-05: importFromXML returns empty result for empty XML. An empty string must not crash; result must report success=false and zero documents.

#### `TEST_F(XDOMEAImportTest, XI06_UnclosedDokumentRecordsError)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:416
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAImportTest): n/a
  - `<unnamed>` (XI06_UnclosedDokumentRecordsError): n/a
- Details: TestXI-06: importFromXML records an error for an unclosed <dokument> element. Unclosed element must not crash; result must contain at least one error and no successfully imported documents.

#### `TEST_F(XDOMEAImportTest, XI07_XmlWithNoDocumentElements)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:431
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAImportTest): n/a
  - `<unnamed>` (XI07_XmlWithNoDocumentElements): n/a
- Details: TestXI-07: importFromXML handles XML with no document elements gracefully. Valid XML with no <dokument> or <akte> tags must produce an empty document list without error.

#### `TEST_F(XDOMEAImportTest, XI08_FallbackIdAssignedWhenMissing)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:449
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAImportTest): n/a
  - `<unnamed>` (XI08_FallbackIdAssignedWhenMissing): n/a
- Details: TestXI-08: importFromXML assigns fallback ID when <id> element is absent. Documents without an explicit <id> element must receive a generated ID (not empty) so they can be stored and retrieved.

#### `TEST_F(XDOMEAMultiVersionTest, XMV01_BothSchemaVersionsRegistered)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:615
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAMultiVersionTest): n/a
  - `<unnamed>` (XMV01_BothSchemaVersionsRegistered): n/a
- Details: TestXMV-01: V1 and V2 schema versions are registered without conflict. Verifies that registeredVersions() returns both versions in ascending order.

#### `TEST_F(XDOMEAMultiVersionTest, XMV02_V1BodyValidatesAgainstV1Schema)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:629
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAMultiVersionTest): n/a
  - `<unnamed>` (XMV02_V1BodyValidatesAgainstV1Schema): n/a
- Details: TestXMV-02: Document body valid against schema v1 passes validation. A JSON body containing only the required v1 fields must validate cleanly against schema version 1.

#### `TEST_F(XDOMEAMultiVersionTest, XMV03_OptionalFieldAbsenceDoesNotViolateV2)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:646
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAMultiVersionTest): n/a
  - `<unnamed>` (XMV03_OptionalFieldAbsenceDoesNotViolateV2): n/a
- Details: TestXMV-03: Document body missing optional field validates against v2. A body with the required fields but without the optional aktenzeichen field must still pass schema v2 validation (optional = no violation).

#### `TEST_F(XDOMEAMultiVersionTest, XMV04_XDOMEAAndSchemaLayerCoexistOnSameDocument)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:669
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAMultiVersionTest): n/a
  - `<unnamed>` (XMV04_XDOMEAAndSchemaLayerCoexistOnSameDocument): n/a
- Details: TestXMV-04: Stored XDOMEA document coexists with schema-validated document body. Verifies the complete multi-version lifecycle: import an XDOMEA document, persist it in the connector, construct a parallel JSON body for schema validation, and confirm both layers agree on the canonical ID. This test models the expected integration pattern where an XDOMEA exchange layer and the schema evolution registry operate independently on the same logical document.

#### `TEST_F(XDOMEAStoreTest, XS01_StoreAndGetRoundTrip)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:165
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAStoreTest): n/a
  - `<unnamed>` (XS01_StoreAndGetRoundTrip): n/a
- Details: TestXS-01: storeDocument and getDocument round-trip. Stores a deterministic document and retrieves it by ID, verifying identity and field preservation.

#### `TEST_F(XDOMEAStoreTest, XS02_GetUnknownIdReturnsNullopt)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAStoreTest): n/a
  - `<unnamed>` (XS02_GetUnknownIdReturnsNullopt): n/a
- Details: TestXS-02: getDocument returns nullopt for unknown ID. Verifies that querying an ID that has never been stored returns std::nullopt (not an error throw).

#### `TEST_F(XDOMEAStoreTest, XS03_DuplicateStoreThrows)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAStoreTest): n/a
  - `<unnamed>` (XS03_DuplicateStoreThrows): n/a
- Details: TestXS-03: Duplicate storeDocument throws std::runtime_error. Verifies the duplicate-ID guard is enforced and throws the expected exception type.

#### `TEST_F(XDOMEAStoreTest, XS04_EmptyIdThrowsInvalidArgument)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAStoreTest): n/a
  - `<unnamed>` (XS04_EmptyIdThrowsInvalidArgument): n/a
- Details: TestXS-04: storeDocument with empty ID throws std::invalid_argument. Verifies the empty-ID guard is enforced before attempting storage.

#### `TEST_F(XDOMEAStoreTest, XS05_RemoveDocumentAndVerifyGone)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAStoreTest): n/a
  - `<unnamed>` (XS05_RemoveDocumentAndVerifyGone): n/a
- Details: TestXS-05: removeDocument removes by ID; subsequent getDocument returns nullopt. Verifies clean removal and that removeDocument is a no-op for unknown IDs.

#### `TEST_F(XDOMEAStoreTest, XS06_ListByTypeFiltersCorrectly)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAStoreTest): n/a
  - `<unnamed>` (XS06_ListByTypeFiltersCorrectly): n/a
- Details: TestXS-06: listByType returns only documents matching the requested type. Stores three documents with different object types and validates that listByType returns exactly the matching subset.

#### `TEST_F(XDOMEAStoreTest, XS07_ListByRetentionFiltersCorrectly)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:265
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAStoreTest): n/a
  - `<unnamed>` (XS07_ListByRetentionFiltersCorrectly): n/a
- Details: TestXS-07: listByRetention returns only documents with the matching category. Stores documents with all three retention categories and verifies that listByRetention returns exactly the matching subset for each.

#### `TEST_F(XDOMEAStoreTest, XS08_ListChildrenFiltersCorrectly)`
- Source: `tests/document/test_document_xdomea_focused.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (XDOMEAStoreTest): n/a
  - `<unnamed>` (XS08_ListChildrenFiltersCorrectly): n/a
- Details: TestXS-08: listChildren returns only direct children of the given parent ID. Stores a parent Akte and two children (one with parent set, one without), and verifies listChildren returns exactly the documents whose parent_id matches.

### themis::Customer

#### `Customer()`
- Source: `include/document/encrypted_entities.h`:103
- Brief: n/a
- Parameters: none

#### `Customer fromJson(const nlohmann::json &j)`
- Source: `include/document/encrypted_entities.h`:131
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: value(), contains(), fromBase64().

#### `nlohmann::json toJson() const`
- Source: `include/document/encrypted_entities.h`:105
- Brief: n/a
- Parameters: none

### themis::SecureDocument

#### `SecureDocument()`
- Source: `include/document/encrypted_entities.h`:162
- Brief: n/a
- Parameters: none

#### `SecureDocument fromJson(const nlohmann::json &j)`
- Source: `include/document/encrypted_entities.h`:189
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: value(), contains(), fromBase64().

#### `nlohmann::json toJson() const`
- Source: `include/document/encrypted_entities.h`:164
- Brief: n/a
- Parameters: none

### themis::User

#### `User()`
- Source: `include/document/encrypted_entities.h`:34
- Brief: n/a
- Parameters: none

#### `User fromJson(const nlohmann::json &j)`
- Source: `include/document/encrypted_entities.h`:66
- Brief: From Json.
- Parameters:
  - `j` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: value(), contains(), fromBase64().

#### `nlohmann::json toJson() const`
- Source: `include/document/encrypted_entities.h`:36
- Brief: n/a
- Parameters: none

### themis::bench::document

#### `BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_01_DiffSmallDocument)(benchmark`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:240
- Brief: DDM-BM-01: diff two 5-field JSON objects (1 field differs).
- Parameters:
  - `<unnamed>` (DocumentDiffMergeFixture): n/a
  - `<unnamed>` (DDM_BM_01_DiffSmallDocument): n/a
- Details: Measures the hot-path diff latency for a minimal document payload. Useful as a low-baseline reference for overhead decomposition.

#### `BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_02_DiffLargeDocument)(benchmark`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:263
- Brief: DDM-BM-02: diff two 100-field JSON objects (50 fields differ).
- Parameters:
  - `<unnamed>` (DocumentDiffMergeFixture): n/a
  - `<unnamed>` (DDM_BM_02_DiffLargeDocument): n/a
- Details: Exercises the full field-scan loop across a realistic large payload. Result feeds GATE-DOC-01 (p99 ≤ 500 µs).

#### `BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_03_MergeCleanNoConflict)(benchmark`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:286
- Brief: DDM-BM-03: three-way merge — ours and theirs modify different fields.
- Parameters:
  - `<unnamed>` (DocumentDiffMergeFixture): n/a
  - `<unnamed>` (DDM_BM_03_MergeCleanNoConflict): n/a
- Details: No conflict is generated; FAIL strategy confirms clean-merge semantics on every iteration. Result feeds GATE-DOC-02 (p99 ≤ 200 µs).

#### `BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_04_MergeAllConflicts_OursWins)(benchmark`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:310
- Brief: DDM-BM-04: three-way merge — 50 conflicting fields, OURS_WINS strategy.
- Parameters:
  - `<unnamed>` (DocumentDiffMergeFixture): n/a
  - `<unnamed>` (DDM_BM_04_MergeAllConflicts_OursWins): n/a
- Details: All 50 fields conflict; every conflict is resolved automatically with OURS_WINS. Measures combined conflict-detection and resolution cost.

#### `BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_05_MergeAllConflicts_TheirsWins)(benchmark`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:334
- Brief: DDM-BM-05: three-way merge — 50 conflicting fields, THEIRS_WINS.
- Parameters:
  - `<unnamed>` (DocumentDiffMergeFixture): n/a
  - `<unnamed>` (DDM_BM_05_MergeAllConflicts_TheirsWins): n/a
- Details: Symmetric variant of DDM-BM-04. Confirms THEIRS_WINS resolution cost matches OURS_WINS; both paths should be within 5% of each other.

#### `BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_06_MergeAllConflicts_Fail)(benchmark`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:359
- Brief: DDM-BM-06: three-way merge — 50 conflicting fields, FAIL strategy.
- Parameters:
  - `<unnamed>` (DocumentDiffMergeFixture): n/a
  - `<unnamed>` (DDM_BM_06_MergeAllConflicts_Fail): n/a
- Details: Exercises the error path: every iteration returns ERR_DOC_MERGE_CONFLICT. Measures conflict detection + error construction cost without resolution. The error result is DoNotOptimize'd to prevent elision.

#### `BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_07_DiffEmptyDocuments)(benchmark`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:384
- Brief: DDM-BM-07: diff two empty JSON objects.
- Parameters:
  - `<unnamed>` (DocumentDiffMergeFixture): n/a
  - `<unnamed>` (DDM_BM_07_DiffEmptyDocuments): n/a
- Details: Measures the fixed overhead of the diff path when both documents are empty. Used as a lower-bound baseline for overhead decomposition.

#### `BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, DDM_BM_08_MergeIdenticalDocuments)(benchmark`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:407
- Brief: DDM-BM-08: merge where base = ours = theirs (zero divergence).
- Parameters:
  - `<unnamed>` (DocumentDiffMergeFixture): n/a
  - `<unnamed>` (DDM_BM_08_MergeIdenticalDocuments): n/a
- Details: Exercises the no-change fast path in computeMerge. All three versions are identical so no conflict or change is detected.

#### `BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, GATE_DOC_01_LargeDocDiff_p99_500us)(benchmark`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:436
- Brief: Release gate verification for GATE-DOC-01.
- Parameters:
  - `<unnamed>` (DocumentDiffMergeFixture): n/a
  - `<unnamed>` (GATE_DOC_01_LargeDocDiff_p99_500us): n/a
- Details: Runs the 100-field diff workload (DDM-BM-02 payload) and asserts that the mean per-iteration latency is within the 500 µs hard gate. The gate value is published as a benchmark counter for automated release manifest comparison. Mean latency in a controlled CPU-pinned environment serves as a conservative proxy for p99. For formal p99 attestation run with benchmark_repetitions=10 and post-process with report_variance.py.

#### `BENCHMARK_DEFINE_F(DocumentDiffMergeFixture, GATE_DOC_02_CleanMerge_p99_200us)(benchmark`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:467
- Brief: Release gate verification for GATE-DOC-02.
- Parameters:
  - `<unnamed>` (DocumentDiffMergeFixture): n/a
  - `<unnamed>` (GATE_DOC_02_CleanMerge_p99_200us): n/a
- Details: Runs the clean-merge workload (DDM-BM-03 payload: 10 fields/branch, no conflict) and asserts that mean latency is within the 200 µs hard gate.

#### `BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_01_PutThroughput)(benchmark`
- Source: `benchmarks/document/bench_document_store.cpp`:237
- Brief: DST-BM-01: put() throughput — kBatchSize sequential documents per iteration.
- Parameters:
  - `<unnamed>` (DocumentStoreFixture): n/a
  - `<unnamed>` (DST_BM_01_PutThroughput): n/a
- Details: Each iteration inserts kBatchSize documents with unique IDs derived from iterCounter_ to guarantee no ERR_DOC_ALREADY_EXISTS errors. Measures raw JSON-body storage throughput. Result feeds GATE-DOC-05 (throughput ≥ 100 000 ops/s).

#### `BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_02_GetLatency)(benchmark`
- Source: `benchmarks/document/bench_document_store.cpp`:270
- Brief: DST-BM-02: get() latency — random access into 1000-document store.
- Parameters:
  - `<unnamed>` (DocumentStoreFixture): n/a
  - `<unnamed>` (DST_BM_02_GetLatency): n/a
- Details: Each iteration performs one get() using a deterministic pseudo-random key drawn from the pre-loaded collection. Measures point-lookup latency under a realistic working-set size. Result feeds GATE-DOC-06 (p99 ≤ 100 µs).

#### `BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_03_ListThroughput)(benchmark`
- Source: `benchmarks/document/bench_document_store.cpp`:294
- Brief: DST-BM-03: list() throughput — enumerate 1000 document IDs.
- Parameters:
  - `<unnamed>` (DocumentStoreFixture): n/a
  - `<unnamed>` (DST_BM_03_ListThroughput): n/a
- Details: Calls list() on a 1000-document collection on every iteration. Exercises the full collection-scan hot path in InMemoryDocumentStore.

#### `BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_04_CountLatency)(benchmark`
- Source: `benchmarks/document/bench_document_store.cpp`:316
- Brief: DST-BM-04: count() latency — count 1000-document collection.
- Parameters:
  - `<unnamed>` (DocumentStoreFixture): n/a
  - `<unnamed>` (DST_BM_04_CountLatency): n/a
- Details: Calls count() on the 1000-document pre-loaded collection. Internally this scans the store map to count keys matching the collection prefix.

#### `BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_05_UpdateThroughput)(benchmark`
- Source: `benchmarks/document/bench_document_store.cpp`:341
- Brief: DST-BM-05: update() body replacement throughput — 1000 existing docs.
- Parameters:
  - `<unnamed>` (DocumentStoreFixture): n/a
  - `<unnamed>` (DST_BM_05_UpdateThroughput): n/a
- Details: Each iteration updates all kPreloadCount documents in kReadCollection with a fresh JSON body. Measures the combined JSON replacement and unordered_map lookup cost across 1000 records. Reports items_processed as iterations × kPreloadCount.

#### `BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_06_RemoveThroughput)(benchmark`
- Source: `benchmarks/document/bench_document_store.cpp`:369
- Brief: DST-BM-06: remove() throughput — kBatchSize documents per iteration.
- Parameters:
  - `<unnamed>` (DocumentStoreFixture): n/a
  - `<unnamed>` (DST_BM_06_RemoveThroughput): n/a
- Details: Each iteration: (paused) inserts kBatchSize documents with unique IDs, then (measured) removes all of them. PauseTiming guards the pre-insertion so only the remove() path is timed. Reports items_processed as iterations × kBatchSize.

#### `BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_07_LargeBodyPut)(benchmark`
- Source: `benchmarks/document/bench_document_store.cpp`:407
- Brief: DST-BM-07: put() with a 10 KB JSON body document.
- Parameters:
  - `<unnamed>` (DocumentStoreFixture): n/a
  - `<unnamed>` (DST_BM_07_LargeBodyPut): n/a
- Details: Each iteration inserts one document with a 10 KB body (pre-built in SetUp) using a unique ID derived from iterCounter_. Measures JSON copy and unordered_map insertion overhead under a large-body payload.

#### `BENCHMARK_DEFINE_F(DocumentStoreFixture, DST_BM_08_SchemaValidationThroughput)(benchmark`
- Source: `benchmarks/document/bench_document_store.cpp`:437
- Brief: DST-BM-08: validate() throughput — 1000 documents against 10-field schema.
- Parameters:
  - `<unnamed>` (DocumentStoreFixture): n/a
  - `<unnamed>` (DST_BM_08_SchemaValidationThroughput): n/a
- Details: Each iteration validates kPreloadCount pre-built document bodies against the sealed 10-field schema registered at version 1. Measures the combined cost of schema lookup and per-field type/presence checking. Reports items_processed as iterations × kPreloadCount.

#### `BENCHMARK_DEFINE_F(DocumentStoreFixture, GATE_DOC_05_PutThroughput_100k_ops_s)(benchmark`
- Source: `benchmarks/document/bench_document_store.cpp`:464
- Brief: Release gate verification for GATE-DOC-05.
- Parameters:
  - `<unnamed>` (DocumentStoreFixture): n/a
  - `<unnamed>` (GATE_DOC_05_PutThroughput_100k_ops_s): n/a
- Details: Runs the single-document put() workload and asserts that the measured throughput meets the 100 000 ops/s minimum. The gate is evaluated from items_processed / elapsed_time.

#### `BENCHMARK_DEFINE_F(DocumentStoreFixture, GATE_DOC_06_GetLatency_p99_100us)(benchmark`
- Source: `benchmarks/document/bench_document_store.cpp`:498
- Brief: Release gate verification for GATE-DOC-06.
- Parameters:
  - `<unnamed>` (DocumentStoreFixture): n/a
  - `<unnamed>` (GATE_DOC_06_GetLatency_p99_100us): n/a
- Details: Runs the random-access get() workload against the 1000-document pre-loaded collection and asserts that mean latency is within the 100 µs hard gate.

#### `BENCHMARK_DEFINE_F(RoundTripFixture, GATE_DOC_03_BeginRelay_p99_1ms)(benchmark`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:435
- Brief: Release gate verification for GATE-DOC-03.
- Parameters:
  - `<unnamed>` (RoundTripFixture): n/a
  - `<unnamed>` (GATE_DOC_03_BeginRelay_p99_1ms): n/a
- Details: Runs the beginRelay() workload and asserts that mean latency is within the 1 ms (1000 µs) hard gate. The gate value is published as a benchmark counter for automated release manifest comparison.

#### `BENCHMARK_DEFINE_F(RoundTripFixture, GATE_DOC_04_LoadInteraction_p99_200us)(benchmark`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:467
- Brief: Release gate verification for GATE-DOC-04.
- Parameters:
  - `<unnamed>` (RoundTripFixture): n/a
  - `<unnamed>` (GATE_DOC_04_LoadInteraction_p99_200us): n/a
- Details: Runs the loadInteraction() workload on the pre-populated relay and asserts that mean latency is within the 200 µs hard gate.

#### `BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_01_BeginRelayThroughput)(benchmark`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:179
- Brief: RTP-BM-01: beginRelay() throughput for sequential relay IDs.
- Parameters:
  - `<unnamed>` (RoundTripFixture): n/a
  - `<unnamed>` (RTP_BM_01_BeginRelayThroughput): n/a
- Details: Each iteration creates a new relay with a unique ID derived from an atomic counter. Measures the combined cost of snapshot ID construction, JSON body assembly, and InMemoryDocumentStore::put(). Result feeds GATE-DOC-03 (p99 ≤ 1 ms).

#### `BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_02_SaveInteractionThroughput)(benchmark`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:206
- Brief: RTP-BM-02: saveInteraction() throughput — 10 sequential interactions.
- Parameters:
  - `<unnamed>` (RoundTripFixture): n/a
  - `<unnamed>` (RTP_BM_02_SaveInteractionThroughput): n/a
- Details: Each iteration: (paused) begin a fresh relay, then (measured) save 10 interactions sequentially. PauseTiming() guards the beginRelay() setup call so only the saveInteraction() loop is measured. Reports items_processed as iterations × 10 for ops/s computation.

#### `BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_03_LoadInteractionLatency)(benchmark`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:242
- Brief: RTP-BM-03: loadInteraction() latency for a pre-populated relay.
- Parameters:
  - `<unnamed>` (RoundTripFixture): n/a
  - `<unnamed>` (RTP_BM_03_LoadInteractionLatency): n/a
- Details: Loads interaction index 5 (mid-range) from the pre-populated relay on every iteration. The relay contains 11 snapshots; load is a pure read path through InMemoryDocumentStore::get(). Result feeds GATE-DOC-04 (p99 ≤ 200 µs).

#### `BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_04_CountSnapshotsLatency)(benchmark`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:265
- Brief: RTP-BM-04: countSnapshots() latency for a relay with 10 snapshots.
- Parameters:
  - `<unnamed>` (RoundTripFixture): n/a
  - `<unnamed>` (RTP_BM_04_CountSnapshotsLatency): n/a
- Details: Calls countSnapshots() on the pre-populated relay (11 total snapshots). Internally lists the collection and scans all IDs for the relay prefix.

#### `BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_05_FullRelayWorkflow)(benchmark`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:291
- Brief: RTP-BM-05: end-to-end relay workflow — begin + 5x save + 5x load.
- Parameters:
  - `<unnamed>` (RoundTripFixture): n/a
  - `<unnamed>` (RTP_BM_05_FullRelayWorkflow): n/a
- Details: Each iteration: (paused) allocate a unique relay ID, then (measured) call beginRelay(), save 5 interactions, and load all 5 back. Represents the complete hot path of the DELEGATE-52 round-trip use case. Reports items_processed as iterations × 11 (1 begin + 5 save + 5 load).

#### `BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_06_SnapshotIdGeneration)(benchmark`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:332
- Brief: RTP-BM-06: makeSnapshotId-equivalent string construction throughput.
- Parameters:
  - `<unnamed>` (RoundTripFixture): n/a
  - `<unnamed>` (RTP_BM_06_SnapshotIdGeneration): n/a
- Details: Benchmarks the zero-padded snapshot ID construction pattern used internally by StoreBackedRoundTripEditor::makeSnapshotId(). Since makeSnapshotId() is private, the equivalent ostringstream construction is measured directly. At 1M iterations this benchmark reveals allocator and ostringstream overhead in the snapshot ID hot path.

#### `BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_07_LargeDocumentSnapshot)(benchmark`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:360
- Brief: RTP-BM-07: saveInteraction() with a 10 KB document string.
- Parameters:
  - `<unnamed>` (RoundTripFixture): n/a
  - `<unnamed>` (RTP_BM_07_LargeDocumentSnapshot): n/a
- Details: Each iteration: (paused) begin a relay, then (measured) save one 10 KB document snapshot. Measures JSON body assembly and in-memory store insertion cost under a realistic large-document payload.

#### `BENCHMARK_DEFINE_F(RoundTripFixture, RTP_BM_08_ConcurrentRelayLoad)(benchmark`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:396
- Brief: RTP-BM-08: two concurrent relay workloads via std::thread.
- Parameters:
  - `<unnamed>` (RoundTripFixture): n/a
  - `<unnamed>` (RTP_BM_08_ConcurrentRelayLoad): n/a
- Details: Each benchmark iteration launches two threads; each thread independently begins a relay, saves 5 interactions, and loads one snapshot. The threads share the InMemoryDocumentStore (which is protected by its internal mutex) and the StoreBackedRoundTripEditor. Measures the overhead of concurrent write+read access under the store's mutex. Reports items_processed as iterations × 2 (two workloads/iter).

#### `Iterations(1 '000 '000) -> Unit(benchmark::kNanosecond) ->UseRealTime() ->Name("DocRoundTrip/RTP-BM-06_SnapshotIdGeneration")`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:344
- Brief: n/a
- Parameters:
  - `000` (1 '000 '): n/a

#### `Iterations(1 '000) -> Unit(benchmark::kMicrosecond) ->UseRealTime() ->Name("DocRoundTrip/RTP-BM-07_LargeDocumentSnapshot")`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:376
- Brief: n/a
- Parameters:
  - `000` (1 '): n/a

#### `Iterations(10 '000) -> Unit(benchmark::kMicrosecond) ->UseRealTime() ->Name("DocDiffMerge/DDM-BM-03_MergeCleanNoConflict")`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:295
- Brief: n/a
- Parameters:
  - `000` (10 '): n/a

#### `Iterations(100) -> Unit(benchmark::kMicrosecond) ->UseRealTime() ->Name("DocStore/DST-BM-01_PutThroughput")`
- Source: `benchmarks/document/bench_document_store.cpp`:252
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Iterations(2 '000) -> Unit(benchmark::kMicrosecond) ->UseRealTime() ->Name("DocRoundTrip/RTP-BM-02_SaveInteractionThroughput")`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:224
- Brief: n/a
- Parameters:
  - `000` (2 '): n/a

#### `Iterations(20 '000) -> Unit(benchmark::kMicrosecond) ->UseRealTime() ->Name("DocDiffMerge/DDM-BM-08_MergeIdenticalDocuments")`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:416
- Brief: n/a
- Parameters:
  - `000` (20 '): n/a

#### `Iterations(5 '000) -> Unit(benchmark::kMicrosecond) ->UseRealTime() ->Name("DocDiffMerge/DDM-BM-02_DiffLargeDocument")`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:271
- Brief: n/a
- Parameters:
  - `000` (5 '): n/a

#### `Iterations(50 '000) -> Unit(benchmark::kMicrosecond) ->UseRealTime() ->Name("DocDiffMerge/DDM-BM-01_DiffSmallDocument")`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:248
- Brief: n/a
- Parameters:
  - `000` (50 '): n/a

#### `Iterations(50) -> Unit(benchmark::kMicrosecond) ->UseRealTime() ->Name("DocStore/DST-BM-06_RemoveThroughput")`
- Source: `benchmarks/document/bench_document_store.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (50): n/a

#### `Iterations(500) -> Unit(benchmark::kMicrosecond) ->UseRealTime() ->Name("DocRoundTrip/RTP-BM-08_ConcurrentRelayLoad")`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:419
- Brief: n/a
- Parameters:
  - `<unnamed>` (500): n/a

### themis::bench::document::DocumentDiffMergeFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void populateDocuments()`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:145
- Brief: n/a
- Parameters: none

#### `void warmUp()`
- Source: `benchmarks/document/bench_document_diff_merge.cpp`:218
- Brief: Prime branch predictor and instruction cache before measurement.
- Parameters: none

### themis::bench::document::DocumentStoreFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/document/bench_document_store.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/document/bench_document_store.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void preloadReadCollection()`
- Source: `benchmarks/document/bench_document_store.cpp`:179
- Brief: n/a
- Parameters: none

#### `void prepareSchemaEngine()`
- Source: `benchmarks/document/bench_document_store.cpp`:191
- Brief: n/a
- Parameters: none

#### `void warmUp()`
- Source: `benchmarks/document/bench_document_store.cpp`:212
- Brief: n/a
- Parameters: none

### themis::bench::document::RoundTripFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void preloadRelay()`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:147
- Brief: n/a
- Parameters: none

#### `void warmUp()`
- Source: `benchmarks/document/bench_document_round_trip.cpp`:157
- Brief: n/a
- Parameters: none

### themis::document

#### `DocumentErrorClass classifyDocumentError(errors::ErrorCode code) noexcept`
- Source: `include/document/document_diagnostics.h`:97
- Brief: n/a
- Parameters:
  - `code` (errors::ErrorCode): n/a

#### `std::string_view documentErrorClassName(DocumentErrorClass cls) noexcept`
- Source: `include/document/document_diagnostics.h`:152
- Brief: n/a
- Parameters:
  - `cls` (DocumentErrorClass): n/a

#### `std::string_view documentErrorDescription(errors::ErrorCode code) noexcept`
- Source: `include/document/document_diagnostics.h`:171
- Brief: n/a
- Parameters:
  - `code` (errors::ErrorCode): n/a

#### `std::string formatDocumentError(const themis::Error &err)`
- Source: `include/document/document_diagnostics.h`:232
- Brief: n/a
- Parameters:
  - `err` (const themis::Error &): n/a

#### `std::ostream & operator<<(std::ostream &os, DocumentErrorClass cls)`
- Source: `include/document/document_diagnostics.h`:223
- Brief: n/a
- Parameters:
  - `os` (std::ostream &): n/a
  - `cls` (DocumentErrorClass): n/a

### themis::document::DocumentDiagnosticSink

#### `DocumentDiagnosticSink()=default`
- Source: `include/document/document_diagnostics.h`:255
- Brief: n/a
- Parameters: none

#### `DocumentDiagnosticSink(DocumentDiagnosticSink &&) noexcept=default`
- Source: `include/document/document_diagnostics.h`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiagnosticSink &&): n/a

#### `DocumentDiagnosticSink(const DocumentDiagnosticSink &)=delete`
- Source: `include/document/document_diagnostics.h`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DocumentDiagnosticSink &): n/a

#### `void clear() noexcept`
- Source: `include/document/document_diagnostics.h`:306
- Brief: n/a
- Parameters: none

#### `std::size_t count(DocumentErrorClass cls) const noexcept`
- Source: `include/document/document_diagnostics.h`:283
- Brief: n/a
- Parameters:
  - `cls` (DocumentErrorClass): n/a

#### `DocumentDiagnosticSink & operator=(DocumentDiagnosticSink &&) noexcept=default`
- Source: `include/document/document_diagnostics.h`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (DocumentDiagnosticSink &&): n/a

#### `DocumentDiagnosticSink & operator=(const DocumentDiagnosticSink &)=delete`
- Source: `include/document/document_diagnostics.h`:258
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DocumentDiagnosticSink &): n/a

#### `void record(errors::ErrorCode code, std::string_view context) noexcept`
- Source: `include/document/document_diagnostics.h`:267
- Brief: n/a
- Parameters:
  - `code` (errors::ErrorCode): n/a
  - `context` (std::string_view): n/a

#### `std::size_t totalCount() const noexcept`
- Source: `include/document/document_diagnostics.h`:295
- Brief: n/a
- Parameters: none

#### `~DocumentDiagnosticSink()=default`
- Source: `include/document/document_diagnostics.h`:263
- Brief: n/a
- Parameters: none

### themis::document::DocumentDiff

#### `bool isEmpty() const noexcept`
- Source: `include/document/document_diff_merge.h`:57
- Brief: n/a
- Parameters: none

### themis::document::IDocumentDiffMerge

#### `Result< DocumentDiff > diff(const CollectionId &collection, const DocumentId &base_id, const DocumentId &target_id) const =0`
- Source: `include/document/document_diff_merge.h`:114
- Brief: Diff.
- Parameters:
  - `collection` (const CollectionId &): Input parameter.
  - `base_id` (const DocumentId &): Identifier of the base.
  - `target_id` (const DocumentId &): Identifier of the target.
- Return: Return value.
- Details: collection Input parameter. base_id Identifier of the base. target_id Identifier of the target. Return value.

#### `Result< MergeResult > merge(const CollectionId &collection, const DocumentId &base_id, const DocumentId &ours_id, const DocumentId &theirs_id, MergeStrategy strategy=MergeStrategy::FAIL) const =0`
- Source: `include/document/document_diff_merge.h`:118
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `base_id` (const DocumentId &): n/a
  - `ours_id` (const DocumentId &): n/a
  - `theirs_id` (const DocumentId &): n/a
  - `strategy` (MergeStrategy): n/a

#### `~IDocumentDiffMerge()=default`
- Source: `include/document/document_diff_merge.h`:105
- Brief: IDocument Diff Merge.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::document::IDocumentLifecycleHook

#### `void afterCreate(const DocumentLifecycleEvent &evt) noexcept=0`
- Source: `include/document/document_lifecycle.h`:81
- Brief: After Create.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

#### `void afterDelete(const DocumentLifecycleEvent &evt) noexcept=0`
- Source: `include/document/document_lifecycle.h`:105
- Brief: After Delete.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

#### `void afterUpdate(const DocumentLifecycleEvent &evt) noexcept=0`
- Source: `include/document/document_lifecycle.h`:93
- Brief: After Update.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

#### `void beforeCreate(const DocumentLifecycleEvent &evt) noexcept=0`
- Source: `include/document/document_lifecycle.h`:75
- Brief: Before Create.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

#### `void beforeDelete(const DocumentLifecycleEvent &evt) noexcept=0`
- Source: `include/document/document_lifecycle.h`:99
- Brief: Before Delete.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

#### `void beforeUpdate(const DocumentLifecycleEvent &evt) noexcept=0`
- Source: `include/document/document_lifecycle.h`:87
- Brief: Before Update.
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): Input parameter.
- Details: evt Input parameter. Exception safety: noexcept.

#### `~IDocumentLifecycleHook()=default`
- Source: `include/document/document_lifecycle.h`:68
- Brief: IDocument Lifecycle Hook.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::document::IDocumentManager

#### `Result< DocumentId > create(const CollectionId &collection, const DocumentId &id, const nlohmann::json &body)=0`
- Source: `include/document/document_manager.h`:88
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a
  - `body` (const nlohmann::json &): n/a

#### `Result< std::unique_ptr< IEncryptedDocumentEntity > > createEncrypted(const CollectionId &collection, const DocumentId &id, const nlohmann::json &body)=0`
- Source: `include/document/document_manager.h`:108
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a
  - `body` (const nlohmann::json &): n/a

#### `Result< std::optional< nlohmann::json > > get(const CollectionId &collection, const DocumentId &id) const =0`
- Source: `include/document/document_manager.h`:92
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a

#### `Result< std::vector< DocumentId > > list(const CollectionId &collection) const =0`
- Source: `include/document/document_manager.h`:103
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a

#### `void registerLifecycleHook(IDocumentLifecycleHook &hook)=0`
- Source: `include/document/document_manager.h`:118
- Brief: ── Lifecycle hooks ───────────────────────────────────────────────────
- Parameters:
  - `hook` (IDocumentLifecycleHook &): Input/output parameter.
- Details: hook Input/output parameter.

#### `Result< void > remove(const CollectionId &collection, const DocumentId &id)=0`
- Source: `include/document/document_manager.h`:100
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a

#### `void unregisterLifecycleHook(IDocumentLifecycleHook &hook)=0`
- Source: `include/document/document_manager.h`:124
- Brief: Unregister Lifecycle Hook.
- Parameters:
  - `hook` (IDocumentLifecycleHook &): Input/output parameter.
- Details: hook Input/output parameter.

#### `Result< void > update(const CollectionId &collection, const DocumentId &id, const nlohmann::json &body)=0`
- Source: `include/document/document_manager.h`:96
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a
  - `body` (const nlohmann::json &): n/a

#### `~IDocumentManager()=default`
- Source: `include/document/document_manager.h`:84
- Brief: IDocument Manager.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::document::IDocumentSchemaEvolution

#### `bool isSealed() const noexcept=0`
- Source: `include/document/document_schema_evolution.h`:130
- Brief: n/a
- Parameters: none

#### `Result< void > registerVersion(SchemaVersion version, const SchemaDescriptor &descriptor)=0`
- Source: `include/document/document_schema_evolution.h`:121
- Brief: n/a
- Parameters:
  - `version` (SchemaVersion): n/a
  - `descriptor` (const SchemaDescriptor &): n/a

#### `std::vector< SchemaVersion > registeredVersions() const =0`
- Source: `include/document/document_schema_evolution.h`:132
- Brief: n/a
- Parameters: none

#### `void seal() noexcept=0`
- Source: `include/document/document_schema_evolution.h`:128
- Brief: Seal.
- Parameters: none
- Details: Exception safety: noexcept.

#### `Result< ValidationReport > validate(const DocumentId &document_id, const nlohmann::json &document_body, SchemaVersion version) const =0`
- Source: `include/document/document_schema_evolution.h`:134
- Brief: n/a
- Parameters:
  - `document_id` (const DocumentId &): n/a
  - `document_body` (const nlohmann::json &): n/a
  - `version` (SchemaVersion): n/a

#### `~IDocumentSchemaEvolution()=default`
- Source: `include/document/document_schema_evolution.h`:119
- Brief: IDocument Schema Evolution.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::document::IDocumentStore

#### `Result< std::size_t > count(const CollectionId &collection) const =0`
- Source: `include/document/document_store.h`:88
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a

#### `Result< std::optional< DocumentRecord > > get(const CollectionId &collection, const DocumentId &id) const =0`
- Source: `include/document/document_store.h`:75
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a

#### `Result< std::vector< DocumentId > > list(const CollectionId &collection) const =0`
- Source: `include/document/document_store.h`:85
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a

#### `Result< DocumentId > put(const DocumentRecord &record)=0`
- Source: `include/document/document_store.h`:73
- Brief: n/a
- Parameters:
  - `record` (const DocumentRecord &): n/a

#### `Result< void > remove(const CollectionId &collection, const DocumentId &id)=0`
- Source: `include/document/document_store.h`:82
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a

#### `Result< void > update(const CollectionId &collection, const DocumentId &id, const nlohmann::json &body)=0`
- Source: `include/document/document_store.h`:78
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a
  - `body` (const nlohmann::json &): n/a

#### `~IDocumentStore()=default`
- Source: `include/document/document_store.h`:71
- Brief: IDocument Store.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::document::IEncryptedDocumentEntity

#### `const CollectionId & collectionId() const noexcept=0`
- Source: `include/document/document_manager.h`:69
- Brief: n/a
- Parameters: none

#### `const DocumentId & documentId() const noexcept=0`
- Source: `include/document/document_manager.h`:67
- Brief: n/a
- Parameters: none

#### `Result< void > reencrypt(const KeyRotationDescriptor &desc)=0`
- Source: `include/document/document_manager.h`:71
- Brief: n/a
- Parameters:
  - `desc` (const KeyRotationDescriptor &): n/a

#### `~IEncryptedDocumentEntity()=default`
- Source: `include/document/document_manager.h`:65
- Brief: IEncrypted Document Entity.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::document::IRoundTripEditor

#### `Result< void > beginRelay(const std::string &relay_id, const std::string &seed_document)=0`
- Source: `include/document/round_trip_editor.h`:51
- Brief: n/a
- Parameters:
  - `relay_id` (const std::string &): n/a
  - `seed_document` (const std::string &): n/a

#### `Result< std::size_t > countSnapshots(const std::string &relay_id) const =0`
- Source: `include/document/round_trip_editor.h`:64
- Brief: n/a
- Parameters:
  - `relay_id` (const std::string &): n/a

#### `Result< std::optional< RoundTripSnapshot > > loadInteraction(const std::string &relay_id, std::size_t interaction_index) const =0`
- Source: `include/document/round_trip_editor.h`:60
- Brief: n/a
- Parameters:
  - `relay_id` (const std::string &): n/a
  - `interaction_index` (std::size_t): n/a

#### `Result< void > saveInteraction(const std::string &relay_id, std::size_t interaction_index, const std::string &instruction, const std::string &document)=0`
- Source: `include/document/round_trip_editor.h`:54
- Brief: n/a
- Parameters:
  - `relay_id` (const std::string &): n/a
  - `interaction_index` (std::size_t): n/a
  - `instruction` (const std::string &): n/a
  - `document` (const std::string &): n/a

#### `~IRoundTripEditor()=default`
- Source: `include/document/round_trip_editor.h`:49
- Brief: IRound Trip Editor.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::document::IXDOMEAConnector

#### `std::size_t count() const =0`
- Source: `include/document/xdomea_connector.h`:236
- Brief: Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `XDOMEAExportResult exportToXML(const std::vector< XDOMEADocument > &documents, XDOMEAVersion version, XDOMEAMessageType message_type)=0`
- Source: `include/document/xdomea_connector.h`:199
- Brief: Export To XML.
- Parameters:
  - `documents` (const std::vector< XDOMEADocument > &): Input parameter.
  - `version` (XDOMEAVersion): Input parameter.
  - `message_type` (XDOMEAMessageType): Input parameter.
- Return: Return value.
- Details: documents Input parameter. version Input parameter. message_type Input parameter. Return value.

#### `std::optional< XDOMEADocument > getDocument(std::string_view id) const =0`
- Source: `include/document/xdomea_connector.h`:215
- Brief: Get Document.
- Parameters:
  - `id` (std::string_view): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value.

#### `XDOMEAImportResult importFromXML(std::string_view xml_content, XDOMEAVersion version)=0`
- Source: `include/document/xdomea_connector.h`:189
- Brief: Import From XML.
- Parameters:
  - `xml_content` (std::string_view): Input parameter.
  - `version` (XDOMEAVersion): Input parameter.
- Return: Return value.
- Details: xml_content Input parameter. version Input parameter. Return value.

#### `std::vector< XDOMEADocument > listByRetention(XDOMEARetentionCategory retention) const =0`
- Source: `include/document/xdomea_connector.h`:221
- Brief: n/a
- Parameters:
  - `retention` (XDOMEARetentionCategory): n/a

#### `std::vector< XDOMEADocument > listByType(XDOMEAObjectType type) const =0`
- Source: `include/document/xdomea_connector.h`:218
- Brief: n/a
- Parameters:
  - `type` (XDOMEAObjectType): n/a

#### `std::vector< XDOMEADocument > listChildren(std::string_view parent_id) const =0`
- Source: `include/document/xdomea_connector.h`:224
- Brief: n/a
- Parameters:
  - `parent_id` (std::string_view): n/a

#### `void removeDocument(std::string_view id)=0`
- Source: `include/document/xdomea_connector.h`:230
- Brief: Remove Document.
- Parameters:
  - `id` (std::string_view): Input parameter.
- Details: id Input parameter.

#### `void storeDocument(const XDOMEADocument &doc)=0`
- Source: `include/document/xdomea_connector.h`:208
- Brief: Store Document.
- Parameters:
  - `doc` (const XDOMEADocument &): Input parameter.
- Details: doc Input parameter.

#### `~IXDOMEAConnector()=default`
- Source: `include/document/xdomea_connector.h`:181
- Brief: IXDOMEAConnector.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::document::InMemoryDocumentDiffMerge

#### `InMemoryDocumentDiffMerge(IDocumentStore &store)`
- Source: `include/document/document_diff_merge.h`:137
- Brief: In Memory Document Diff Merge.
- Parameters:
  - `store` (IDocumentStore &): Input/output parameter.
- Return: Return value.
- Details: store Input/output parameter. Return value.

#### `DocumentDiff computeDiff(const nlohmann::json &base, const nlohmann::json &target)`
- Source: `include/document/document_diff_merge.h`:220
- Brief: ── diff helpers ──────────────────────────────────────────────────────
- Parameters:
  - `base` (const nlohmann::json &): Input parameter.
  - `target` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: base Input parameter. target Input parameter. Return value.

#### `Result< MergeResult > computeMerge(const nlohmann::json &base, const nlohmann::json &ours, const nlohmann::json &theirs, MergeStrategy strategy)`
- Source: `include/document/document_diff_merge.h`:254
- Brief: ── merge helpers ─────────────────────────────────────────────────────
- Parameters:
  - `base` (const nlohmann::json &): Input parameter.
  - `ours` (const nlohmann::json &): Input parameter.
  - `theirs` (const nlohmann::json &): Input parameter.
  - `strategy` (MergeStrategy): Input parameter.
- Return: Return value.
- Details: base Input parameter. ours Input parameter. theirs Input parameter. strategy Input parameter. Return value.

#### `Result< DocumentDiff > diff(const CollectionId &collection, const DocumentId &base_id, const DocumentId &target_id) const override`
- Source: `include/document/document_diff_merge.h`:140
- Brief: Diff.
- Parameters:
  - `collection` (const CollectionId &): Input parameter.
  - `base_id` (const DocumentId &): Identifier of the base.
  - `target_id` (const DocumentId &): Identifier of the target.
- Return: Return value.
- Details: collection Input parameter. base_id Identifier of the base. target_id Identifier of the target. Return value.

#### `Result< MergeResult > merge(const CollectionId &collection, const DocumentId &base_id, const DocumentId &ours_id, const DocumentId &theirs_id, MergeStrategy strategy=MergeStrategy::FAIL) const override`
- Source: `include/document/document_diff_merge.h`:173
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `base_id` (const DocumentId &): n/a
  - `ours_id` (const DocumentId &): n/a
  - `theirs_id` (const DocumentId &): n/a
  - `strategy` (MergeStrategy): n/a

### themis::document::InMemoryDocumentManager

#### `Result< DocumentId > create(const CollectionId &collection, const DocumentId &id, const nlohmann::json &body) override`
- Source: `include/document/document_manager.h`:170
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a
  - `body` (const nlohmann::json &): n/a

#### `Result< std::unique_ptr< IEncryptedDocumentEntity > > createEncrypted(const CollectionId &collection, const DocumentId &id, const nlohmann::json &body) override`
- Source: `include/document/document_manager.h`:258
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a
  - `body` (const nlohmann::json &): n/a

#### `void dispatchHooks(const DocumentLifecycleEvent &evt) const`
- Source: `include/document/document_manager.h`:310
- Brief: n/a
- Parameters:
  - `evt` (const DocumentLifecycleEvent &): n/a

#### `Result< std::optional< nlohmann::json > > get(const CollectionId &collection, const DocumentId &id) const override`
- Source: `include/document/document_manager.h`:199
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a

#### `Result< std::vector< DocumentId > > list(const CollectionId &collection) const override`
- Source: `include/document/document_manager.h`:250
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a

#### `int64_t nowMs()`
- Source: `include/document/document_manager.h`:334
- Brief: Now Ms.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: system_clock::now(), time_since_epoch(), count().

#### `void registerLifecycleHook(IDocumentLifecycleHook &hook) override`
- Source: `include/document/document_manager.h`:284
- Brief: ── Lifecycle hooks ───────────────────────────────────────────────────
- Parameters:
  - `hook` (IDocumentLifecycleHook &): Input/output parameter.
- Details: hook Input/output parameter.

#### `Result< void > remove(const CollectionId &collection, const DocumentId &id) override`
- Source: `include/document/document_manager.h`:233
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a

#### `void unregisterLifecycleHook(IDocumentLifecycleHook &hook) override`
- Source: `include/document/document_manager.h`:297
- Brief: Unregister Lifecycle Hook.
- Parameters:
  - `hook` (IDocumentLifecycleHook &): Input/output parameter.
- Details: hook Input/output parameter.

#### `Result< void > update(const CollectionId &collection, const DocumentId &id, const nlohmann::json &body) override`
- Source: `include/document/document_manager.h`:214
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a
  - `body` (const nlohmann::json &): n/a

### themis::document::InMemoryDocumentSchemaEvolution

#### `bool checkType(const nlohmann::json &val, SchemaFieldType expected)`
- Source: `include/document/document_schema_evolution.h`:260
- Brief: Check Type.
- Parameters:
  - `val` (const nlohmann::json &): Input parameter.
  - `expected` (SchemaFieldType): Input parameter.
- Return: True when the operation succeeds.
- Details: val Input parameter. expected Input parameter. True when the operation succeeds. Calls: is_string(), is_number(), is_boolean(), is_object(), is_array().

#### `bool isSealed() const noexcept override`
- Source: `include/document/document_schema_evolution.h`:179
- Brief: n/a
- Parameters: none

#### `Result< void > registerVersion(SchemaVersion version, const SchemaDescriptor &descriptor) override`
- Source: `include/document/document_schema_evolution.h`:146
- Brief: n/a
- Parameters:
  - `version` (SchemaVersion): n/a
  - `descriptor` (const SchemaDescriptor &): n/a

#### `std::vector< SchemaVersion > registeredVersions() const override`
- Source: `include/document/document_schema_evolution.h`:189
- Brief: n/a
- Parameters: none

#### `void seal() noexcept override`
- Source: `include/document/document_schema_evolution.h`:169
- Brief: Seal.
- Parameters: none
- Details: Exception safety: noexcept.

#### `Result< ValidationReport > validate(const DocumentId &document_id, const nlohmann::json &document_body, SchemaVersion version) const override`
- Source: `include/document/document_schema_evolution.h`:205
- Brief: n/a
- Parameters:
  - `document_id` (const DocumentId &): n/a
  - `document_body` (const nlohmann::json &): n/a
  - `version` (SchemaVersion): n/a

### themis::document::InMemoryDocumentStore

#### `Result< std::size_t > count(const CollectionId &collection) const override`
- Source: `include/document/document_store.h`:194
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a

#### `Result< std::optional< DocumentRecord > > get(const CollectionId &collection, const DocumentId &id) const override`
- Source: `include/document/document_store.h`:122
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a

#### `Result< std::vector< DocumentId > > list(const CollectionId &collection) const override`
- Source: `include/document/document_store.h`:172
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a

#### `std::string makeKey(const CollectionId &col, const DocumentId &id)`
- Source: `include/document/document_store.h`:222
- Brief: Make Key.
- Parameters:
  - `col` (const CollectionId &): Input parameter.
  - `id` (const DocumentId &): Input parameter.
- Return: Return value.
- Details: col Input parameter. id Input parameter. Return value.

#### `int64_t nowMs()`
- Source: `include/document/document_store.h`:233
- Brief: Now Ms.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: system_clock::now(), time_since_epoch(), count().

#### `Result< DocumentId > put(const DocumentRecord &record) override`
- Source: `include/document/document_store.h`:98
- Brief: n/a
- Parameters:
  - `record` (const DocumentRecord &): n/a

#### `Result< void > remove(const CollectionId &collection, const DocumentId &id) override`
- Source: `include/document/document_store.h`:159
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a

#### `Result< void > update(const CollectionId &collection, const DocumentId &id, const nlohmann::json &body) override`
- Source: `include/document/document_store.h`:139
- Brief: n/a
- Parameters:
  - `collection` (const CollectionId &): n/a
  - `id` (const DocumentId &): n/a
  - `body` (const nlohmann::json &): n/a

### themis::document::InMemoryEncryptedEntity

#### `InMemoryEncryptedEntity(DocumentId doc_id, CollectionId col_id)`
- Source: `include/document/document_manager.h`:133
- Brief: n/a
- Parameters:
  - `doc_id` (DocumentId): n/a
  - `col_id` (CollectionId): n/a

#### `const CollectionId & collectionId() const noexcept override`
- Source: `include/document/document_manager.h`:137
- Brief: n/a
- Parameters: none

#### `const std::string & currentKeyId() const noexcept`
- Source: `include/document/document_manager.h`:152
- Brief: n/a
- Parameters: none

#### `const DocumentId & documentId() const noexcept override`
- Source: `include/document/document_manager.h`:136
- Brief: n/a
- Parameters: none

#### `Result< void > reencrypt(const KeyRotationDescriptor &desc) override`
- Source: `include/document/document_manager.h`:139
- Brief: n/a
- Parameters:
  - `desc` (const KeyRotationDescriptor &): n/a

### themis::document::InMemoryXDOMEAConnector

#### `std::size_t count() const override`
- Source: `include/document/xdomea_connector.h`:486
- Brief: Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string escapeXML_(const std::string &s)`
- Source: `include/document/xdomea_connector.h`:527
- Brief: Escape XML.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: reserve(), size().

#### `XDOMEAExportResult exportToXML(const std::vector< XDOMEADocument > &documents, XDOMEAVersion version, XDOMEAMessageType message_type) override`
- Source: `include/document/xdomea_connector.h`:354
- Brief: Export To XML.
- Parameters:
  - `documents` (const std::vector< XDOMEADocument > &): Input parameter.
  - `version` (XDOMEAVersion): Input parameter.
  - `message_type` (XDOMEAMessageType): Input parameter.
- Return: Return value.
- Details: documents Input parameter. version Input parameter. message_type Input parameter. Return value.

#### `void extractField_(const std::string &fragment, const std::string &tag, std::string &out)`
- Source: `include/document/xdomea_connector.h`:505
- Brief: ── Helpers ───────────────────────────────────────────────────────────────
- Parameters:
  - `fragment` (const std::string &): Input parameter.
  - `tag` (const std::string &): Input parameter.
  - `out` (std::string &): Input/output parameter.
- Details: fragment Input parameter. tag Input parameter. out Input/output parameter. Calls: find(), size(), substr().

#### `std::optional< XDOMEADocument > getDocument(std::string_view id) const override`
- Source: `include/document/xdomea_connector.h`:404
- Brief: Get Document.
- Parameters:
  - `id` (std::string_view): Input parameter.
- Return: Return value.
- Details: id Input parameter. Return value.

#### `XDOMEAImportResult importFromXML(std::string_view xml_content, XDOMEAVersion version) override`
- Source: `include/document/xdomea_connector.h`:245
- Brief: Import From XML.
- Parameters:
  - `xml_content` (std::string_view): Input parameter.
  - `version` (XDOMEAVersion): Input parameter.
- Return: Return value.
- Details: xml_content Input parameter. version Input parameter. Return value.

#### `std::vector< XDOMEADocument > listByRetention(XDOMEARetentionCategory retention) const override`
- Source: `include/document/xdomea_connector.h`:436
- Brief: n/a
- Parameters:
  - `retention` (XDOMEARetentionCategory): n/a

#### `std::vector< XDOMEADocument > listByType(XDOMEAObjectType type) const override`
- Source: `include/document/xdomea_connector.h`:418
- Brief: n/a
- Parameters:
  - `type` (XDOMEAObjectType): n/a

#### `std::vector< XDOMEADocument > listChildren(std::string_view parent_id) const override`
- Source: `include/document/xdomea_connector.h`:454
- Brief: n/a
- Parameters:
  - `parent_id` (std::string_view): n/a

#### `std::string messageTypeCode_(XDOMEAMessageType t)`
- Source: `include/document/xdomea_connector.h`:567
- Brief: Message Type Code.
- Parameters:
  - `t` (XDOMEAMessageType): Input parameter.
- Return: Return value.
- Details: t Input parameter. Return value. Implements messageTypeCode_ without additional internal calls.

#### `std::string objectTypeTag_(XDOMEAObjectType t)`
- Source: `include/document/xdomea_connector.h`:549
- Brief: Object Type Tag.
- Parameters:
  - `t` (XDOMEAObjectType): Input parameter.
- Return: Return value.
- Details: t Input parameter. Return value. Implements objectTypeTag_ without additional internal calls.

#### `void removeDocument(std::string_view id) override`
- Source: `include/document/xdomea_connector.h`:476
- Brief: Remove Document.
- Parameters:
  - `id` (std::string_view): Input parameter.
- Details: id Input parameter.

#### `void storeDocument(const XDOMEADocument &doc) override`
- Source: `include/document/xdomea_connector.h`:388
- Brief: Store Document.
- Parameters:
  - `doc` (const XDOMEADocument &): Input parameter.
- Details: doc Input parameter.

### themis::document::StoreBackedRoundTripEditor

#### `StoreBackedRoundTripEditor(IDocumentStore &store, CollectionId collection=kDefaultRoundTripCollection)`
- Source: `include/document/round_trip_editor.h`:70
- Brief: n/a
- Parameters:
  - `store` (IDocumentStore &): n/a
  - `collection` (CollectionId): n/a

#### `Result< void > beginRelay(const std::string &relay_id, const std::string &seed_document) override`
- Source: `include/document/round_trip_editor.h`:73
- Brief: Begin Relay.
- Parameters:
  - `relay_id` (const std::string &): Identifier of the relay.
  - `seed_document` (const std::string &): Input parameter.
- Return: Return value.
- Details: relay_id Identifier of the relay. seed_document Input parameter. Return value. Calls: makeSnapshotId(), nowMs(), put(), tl::unexpected(), error().

#### `Result< std::size_t > countSnapshots(const std::string &relay_id) const override`
- Source: `include/document/round_trip_editor.h`:86
- Brief: n/a
- Parameters:
  - `relay_id` (const std::string &): n/a

#### `Result< std::optional< RoundTripSnapshot > > loadInteraction(const std::string &relay_id, std::size_t interaction_index) const override`
- Source: `include/document/round_trip_editor.h`:82
- Brief: n/a
- Parameters:
  - `relay_id` (const std::string &): n/a
  - `interaction_index` (std::size_t): n/a

#### `std::string makeSnapshotId(const std::string &relay_id, std::size_t interaction_index) const`
- Source: `include/document/round_trip_editor.h`:90
- Brief: n/a
- Parameters:
  - `relay_id` (const std::string &): n/a
  - `interaction_index` (std::size_t): n/a

#### `Result< void > saveInteraction(const std::string &relay_id, std::size_t interaction_index, const std::string &instruction, const std::string &document) override`
- Source: `include/document/round_trip_editor.h`:76
- Brief: Save Interaction.
- Parameters:
  - `relay_id` (const std::string &): Identifier of the relay.
  - `interaction_index` (std::size_t): Input parameter.
  - `instruction` (const std::string &): Input parameter.
  - `document` (const std::string &): Input parameter.
- Return: Return value.
- Details: relay_id Identifier of the relay. interaction_index Input parameter. instruction Input parameter. document Input parameter. Return value. Calls: makeSnapshotId(), nowMs(), put(), tl::unexpected(), error().

### themis::document::ValidationReport

#### `bool isValid() const noexcept`
- Source: `include/document/document_schema_evolution.h`:106
- Brief: n/a
- Parameters: none

