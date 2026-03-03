# Importers Module - Future Header Enhancements

## Scope

- `IImporter` interface extensions for multi-source import dispatch
- Conflict resolver hook API exposed via `IImportConflictResolver` header
- Flat-file schema auto-detection interface for CSV, Parquet, and JSON Lines
- Apache Kafka consumer source interface for streaming import
- Plugin-based importer registry for third-party importer discovery
- Incremental import cursor for resumable and checkpoint-based import

## Design Constraints

- [ ] All importers must implement `IImporter`; no direct instantiation of concrete importer types across module boundaries
- [ ] Conflict resolver is stateless; it receives both the existing and incoming record and returns a resolution decision, without retaining any state
- [ ] Flat-file schema detection is advisory and non-blocking; callers may proceed with a manual schema if detection returns `SchemaConfidence::LOW`
- [ ] Kafka consumer source is asynchronous; offset commits are decoupled from record delivery via an explicit `commitOffset()` call
- [ ] Importer plugin registry is read-only after module initialization; plugins register at static-init time via `REGISTER_IMPORTER_PLUGIN`
- [ ] Incremental import cursor exposes checkpoint tokens; cursor must be resumable from the last committed checkpoint after a process restart

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IImporter` | Import pipeline, REST import endpoint, CLI import command | Base interface; all importers derive from this |
| `IImportConflictResolver` | Import pipeline, upsert handler, merge engine | Stateless; receives existing + incoming record, returns `ConflictResolution` |
| `IFlatFileSchemaDetector` | CSV importer, Parquet importer, JSON Lines importer | Advisory; returns detected schema with confidence score |
| `IKafkaConsumerSource` | Kafka importer, streaming ingestion pipeline | Async; offset commit is explicit via `commitOffset()` |
| `IImporterPlugin` | Plugin registry, third-party importer loader | Plugin entry point; provides `IImporter` factory function |
| `IIncrementalImportCursor` | Resumable import, CDC consumer, checkpoint-based ETL | Pull-based; yields `ImportBatch` per `next()` call |

## Planned Features

### Flat-File Schema Auto-Detection Interface

- [ ] Define `IFlatFileSchemaDetector` with `detect(FileHandle, SampleRows) -> SchemaDetectionResult`
- [ ] `SchemaDetectionResult` carries detected column names, inferred types, and a `SchemaConfidence` enum (`HIGH`, `MEDIUM`, `LOW`)
- [ ] Detection samples up to a configurable number of rows (default: 1,000); caller controls sample size
- [ ] Detector must handle BOM-prefixed UTF-8, Latin-1, and UTF-16 encoded files; encoding detection is part of the result

### Conflict Resolution Hook API

- [ ] Define `IImportConflictResolver` with `resolve(existing: Record, incoming: Record) -> ConflictResolution`
- [ ] `ConflictResolution` variants: `KEEP_EXISTING`, `REPLACE_WITH_INCOMING`, `MERGE_FIELDS`, `REJECT`
- [ ] `MERGE_FIELDS` resolution carries a `FieldMergeSpec` listing which fields to take from each source
- [ ] Resolver is stateless and must be safe to call concurrently from multiple import worker threads

### Kafka Consumer Source Interface

- [ ] Define `IKafkaConsumerSource` with `poll(timeout) -> KafkaBatch` and `commitOffset(KafkaOffset)`
- [ ] `KafkaBatch` contains a list of `KafkaRecord` with topic, partition, offset, key, and value
- [ ] SASL authentication parameters injected at construction via `KafkaSourceConfig`; credentials never stored in the interface type
- [ ] Consumer group ID is mandatory; `IKafkaConsumerSource` construction fails with `KafkaError::MISSING_GROUP_ID` if absent

### Incremental Import Cursor

- [ ] Define `IIncrementalImportCursor` with `next(ImportBatch&) -> CursorStatus` and `checkpoint() -> CheckpointToken`
- [ ] `CheckpointToken` is an opaque, serializable value type; cursor can be resumed by passing the token to `IImporter::openCursor(token)`
- [ ] Cursor exposes `estimatedRemainingRows()` for progress reporting; may return `UNKNOWN` for streaming sources
- [ ] `CursorStatus::CHECKPOINT_REQUIRED` returned when the source requires an explicit commit before proceeding

### Plugin-Based Importer Registry

- [ ] Define `IImporterPlugin` with `pluginId()`, `supportedSchemes()`, and `createImporter(ImportConfig) -> std::unique_ptr<IImporter>`
- [ ] Registry keyed by URI scheme (e.g., `mysql://`, `mongodb://`, `s3://`)
- [ ] Plugins registered at static-init time via `REGISTER_IMPORTER_PLUGIN(PluginClass)` macro
- [ ] `IImporterPluginRegistry::resolve(uri)` returns `nullptr` for unknown schemes without throwing

## Test Strategy

- Unit-test `IImportConflictResolver` with all four `ConflictResolution` variants; verify `MERGE_FIELDS` produces the correct field selection
- Test `IFlatFileSchemaDetector` against CSV, Parquet, and JSON Lines samples with known schemas; assert `HIGH` confidence for well-formed files
- Integration-test `IKafkaConsumerSource` against an embedded Kafka broker; verify offset commit decoupling and SASL authentication
- Test `IIncrementalImportCursor` checkpoint round-trip: run to mid-stream, serialize token, reopen cursor, verify no records are duplicated or skipped
- Plugin registry test: register a mock plugin, resolve by URI scheme, assert factory produces a valid `IImporter`
- Concurrency test: invoke `IImportConflictResolver::resolve()` from 16 threads simultaneously; assert no data races under TSan

## Performance Targets

- Import batch dispatch (`IImporter::importBatch()`) ≤ 10 ms per 1,000 rows on a 4-core reference machine
- Conflict resolution (`IImportConflictResolver::resolve()`) ≤ 1 ms per conflict for records up to 64 fields
- Flat-file schema detection (`IFlatFileSchemaDetector::detect()`) ≤ 50 ms for a 1,000-row sample from a 100 MB file
- Kafka source offset commit (`IKafkaConsumerSource::commitOffset()`) round-trip ≤ 5 ms p95 under normal broker load
- Incremental cursor `next()` ≤ 2 ms per batch of 1,000 records for file-backed sources
- Plugin registry `resolve()` lookup ≤ 1 µs (hash-map backed, lock-free read path after init)

## Security / Reliability

- Importer credentials (passwords, tokens, connection strings) are never stored in header-visible types; they are injected via opaque `ImportConfig` and handled by the implementation
- Conflict resolver cannot modify the source record; the `incoming` parameter is passed as `const Record&` and the interface enforces this
- Import file paths and URIs are validated against a configurable allow-list before `IImporter::open()` proceeds; relative paths and symlinks are rejected
- Kafka consumer source requires SASL authentication in production; plaintext auth is compile-time disabled when `THEMIS_KAFKA_PLAINTEXT_AUTH=0`
- Checkpoint tokens are signed to prevent injection of forged resume positions; signature verified in `IImporter::openCursor(token)`
- `IImporterPlugin` factory functions are isolated; a plugin that throws during `createImporter()` surfaces `PluginError::FACTORY_EXCEPTION` without propagating to the caller
