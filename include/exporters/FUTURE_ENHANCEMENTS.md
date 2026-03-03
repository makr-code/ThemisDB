# Exporters Module - Future Header Enhancements

## Scope

- `IExporter` interface extensions for format-agnostic export dispatch
- Parquet schema builder API exposed via `IParquetSchemaBuilder` header
- Streaming export cursor interface for pull-based large-collection export
- Delta/incremental export interface via `IDeltaExporter`
- Export encryption hook API for pluggable per-record encryption
- Export format registry for runtime format discovery and selection

## Design Constraints

- [ ] All exporters must implement `IExporter`; no direct instantiation of concrete types across module boundaries
- [ ] Parquet schema is generated once at export initialization time and is immutable during export
- [ ] Streaming cursor is pull-based; consumers call `next()` to advance — no push callbacks
- [ ] Encryption hook is optional and stateless; it must not retain any state between record calls
- [ ] Export format registry entries are registered at static-init time and must be thread-safe to read
- [ ] Delta exporter interface exposes only a cursor and a change-type enum; diff logic is internal

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IExporter` | All export pipelines, CLI, REST export endpoint | Base interface; all exporters must derive from this |
| `IParquetSchemaBuilder` | Parquet export adapter, schema inference pipeline | Generates Arrow/Parquet schema from collection metadata |
| `IStreamingExportCursor` | Large-collection export, HTTP chunked response writer | Pull-based; yields one record batch per `next()` call |
| `IDeltaExporter` | Incremental sync clients, CDC pipelines | Exposes change type (`INSERT`, `UPDATE`, `DELETE`) and record snapshot |
| `IExportEncryptionHook` | Encrypted export adapter, compliance export pipeline | Stateless hook; called per record; returns encrypted bytes |
| `IExportFormatRegistry` | Export dispatcher, plugin loader | Allows runtime registration and lookup of export format factories |

## Planned Features

### Parquet Schema Builder Interface

- [ ] Define `IParquetSchemaBuilder` with `addField()`, `addNestedGroup()`, and `build()` methods
- [ ] Support nullable, repeated, and required field annotations
- [ ] Expose `schemaFingerprint()` for schema versioning and cache keying
- [ ] Schema builder must be invocable before any data rows are produced

### Streaming Export Cursor API

- [ ] Define `IStreamingExportCursor` with `next(RecordBatch&)` returning `ExportStatus`
- [ ] Cursor must expose `estimatedRemainingRows()` for progress reporting
- [ ] Support `reset()` to restart export from the beginning when the underlying source allows
- [ ] Cursor lifetime tied to the originating `IExporter` instance

### Incremental/Delta Export Interface

- [ ] Define `IDeltaExporter` extending `IExporter` with `openDeltaCursor(since: Timestamp)`
- [ ] Delta cursor yields `DeltaRecord` structs containing change type and full record snapshot
- [ ] Checkpoint tokens exposed via `IDeltaExporter::lastCheckpoint()` for resumable export
- [ ] Delta export must be composable with encryption and streaming cursor interfaces

### Export Encryption Hook

- [ ] Define `IExportEncryptionHook` with `encrypt(std::span<const std::byte>) -> std::vector<std::byte>`
- [ ] Hook registered per-export-job via `IExporter::setEncryptionHook()`
- [ ] Hook must be stateless; key material injected via constructor, not stored in hook interface
- [ ] HMAC tag generated and appended by hook implementation; verified by consumer

### Format Registry API

- [ ] Define `IExportFormatRegistry` with `registerFormat()`, `unregisterFormat()`, and `resolve()`
- [ ] Registry keyed by MIME type and file extension
- [ ] Factory functions registered at static-init time via `REGISTER_EXPORT_FORMAT` macro
- [ ] Registry is read-only after module initialization; write access restricted to init phase

## Test Strategy

- Unit-test each interface contract using mock implementations; verify correct sequencing of `next()` calls on `IStreamingExportCursor`
- Integration-test `IParquetSchemaBuilder` against the Arrow C++ library schema validator
- Property-based test `IDeltaExporter` to verify that replaying a delta sequence reconstructs the original collection
- Test `IExportEncryptionHook` round-trip: encrypt via hook, decrypt externally, assert byte-for-byte equality
- Verify `IExportFormatRegistry` thread-safety by concurrent read access from 16 threads after init
- Regression-test export initiation latency to enforce the ≤ 100 ms budget on a reference dataset

## Performance Targets

- Export initiation (`IExporter::open()`) completes in ≤ 100 ms for collections up to 10 M records
- Streaming cursor `next()` returns one record batch in ≤ 1 ms per chunk at batch size 1,000
- `IParquetSchemaBuilder::build()` completes in ≤ 50 ms for schemas with up to 500 fields
- `IExportEncryptionHook::encrypt()` overhead ≤ 100 µs per record (AES-256-GCM reference impl)
- Delta cursor `openDeltaCursor()` setup ≤ 200 ms regardless of total collection size
- Format registry `resolve()` lookup ≤ 1 µs (hash-map backed, no lock on read path)

## Security / Reliability

- Export authorization is checked at `IExporter::open()` before any data is read; unauthorized callers receive `ExportStatus::FORBIDDEN`
- Encrypted exports include an HMAC tag computed over the full export stream; consumers must verify before use
- Export destination paths are validated against a configurable allow-list; relative paths and symlinks are rejected
- `IExportEncryptionHook` implementations must not log plaintext record data; this is enforced by interface contract documentation
- Delta export checkpoint tokens are signed to prevent tampering with the `since` timestamp
- All `IExporter` implementations must handle partial write failures gracefully and surface `ExportStatus::IO_ERROR` without data corruption
