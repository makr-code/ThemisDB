# Utils Module - Future Header Enhancements

## Scope

- `IUtilsPipeline` extensions for composable utility stage registration and lifecycle management
- Streaming PII detector API (`IStreamingPIIDetector`) for stateless per-chunk detection and pseudonymisation
- Hash-chain audit log interface (`IHashChainAuditLog`) for tamper-evident append-only event recording
- HKDF key derivation cache API (`IHKDFKeyCache`) with TTL-enforced eviction before key reuse
- Structured log sampler interface (`IStructuredLogSampler`) with `noexcept` sampling decisions and rate-limiting
- SAGA logger compaction API (`ISAGALogCompactor`) for async non-blocking log segment compaction and replay

## Design Constraints

- `[ ]` `IStreamingPIIDetector` is **stateless per-call**; no state is retained between `detect()` invocations; safe to call concurrently from multiple threads
- `[ ]` `IHashChainAuditLog` is **append-only**; no delete or update methods exist on the public interface
- `[ ]` `IHKDFKeyCache` enforces TTL **before** any key reuse; expired keys are evicted and re-derived, never served stale
- `[ ]` `IStructuredLogSampler` sampling decision methods are `noexcept`; log sampling must never throw or abort a caller
- `[ ]` `ISAGALogCompactor` compaction is **async and non-blocking**; `compact()` returns immediately and compaction proceeds in the background
- `[ ]` Security events (audit log entries tagged `EventClass::Security`) are **never** subject to sampling; `IStructuredLogSampler` must always pass them through

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IStreamingPIIDetector` | Ingest pipeline, log sanitiser | Stateless; exposes `detect(std::span<const std::byte>) -> PIIDetectionResult`, `pseudonymise(std::span<const std::byte>) -> SanitisedChunk` |
| `IHashChainAuditLog` | Audit system, compliance reporter | Append-only; exposes `append(AuditEvent) -> EntryId`, `verifyChain() -> ChainVerifyResult`, `query(AuditQuery) -> AuditCursor` |
| `IHKDFKeyCache` | Crypto layer, key management | TTL-enforced; exposes `derive(KeyContext) -> KeyHandle`, `evict(KeyContext)`, `ttl(KeyContext) -> std::chrono::milliseconds` |
| `IStructuredLogSampler` | Logging framework, observability | `noexcept`; exposes `shouldSample(const LogEntry&) noexcept -> bool`, `recordDecision(const LogEntry&, bool sampled) noexcept` |
| `ISAGALogCompactor` | SAGA transaction log, recovery engine | Async; exposes `compact(SegmentRange) -> std::future<CompactionResult>`, `replay(SegmentId) -> ReplayIterator` |
| `IUtilsPipeline` | Application bootstrap, plugin host | Exposes `registerStage(std::unique_ptr<IUtilsStage>)`, `run() -> std::future<PipelineResult>`, `shutdown() noexcept` |

## Planned Features

### Streaming PII Detection and Pseudonymisation API

- `[ ]` Define `IStreamingPIIDetector` with `detect(std::span<const std::byte> chunk) -> PIIDetectionResult`
- `[ ]` `PIIDetectionResult` carries `containsPII` (bool), `categories` (`std::vector<PIICategory>`), `spanCount` — no original values
- `[ ]` Add `pseudonymise(std::span<const std::byte> chunk) -> SanitisedChunk` replacing detected PII spans with deterministic pseudonyms
- `[ ]` `SanitisedChunk` carries `sanitisedData` (byte span), `replacementCount`, `pseudonymMap` (opaque handle, not raw values)
- `[ ]` Interface exposes `supportedCategories() -> std::span<const PIICategory>` for capability discovery

### Hash-Chain Tamper-Evident Audit Log Interface

- `[ ]` Define `IHashChainAuditLog` with `append(const AuditEvent&) -> EntryId`; each entry's hash includes the previous entry hash
- `[ ]` Add `verifyChain(EntryId from, EntryId to) -> ChainVerifyResult` — verifies HMAC-SHA-256 chain integrity over a range
- `[ ]` `ChainVerifyResult` carries `valid` (bool), `firstTamperedEntry` (optional `EntryId`), `verifiedCount`
- `[ ]` Add `query(const AuditQuery&) -> AuditCursor` for paginated forward-only iteration; cursor is read-only and invalidated by new appends
- `[ ]` Audit log exposes `entryCount() -> size_t` and `lastEntryId() -> EntryId` for health monitoring

### HKDF Key Derivation Cache API

- `[ ]` Define `IHKDFKeyCache` with `derive(const KeyContext&) -> KeyHandle` — returns cached key if TTL has not expired, else re-derives
- `[ ]` `KeyHandle` is move-only RAII; destructor zeroes key material in memory before release
- `[ ]` Add `evict(const KeyContext&)` for explicit cache invalidation and `evictAll()` for emergency key cache flush
- `[ ]` Expose `ttl(const KeyContext&) -> std::chrono::milliseconds` returning remaining lifetime; returns `0` for expired or absent keys
- `[ ]` Cache exposes `cacheSize() -> size_t` and `maxCacheSize() -> size_t` for capacity monitoring; no method exposes raw key bytes

### Structured Log Sampling and Rate-Limiting Interface

- `[ ]` Define `IStructuredLogSampler` with `shouldSample(const LogEntry&) noexcept -> bool`
- `[ ]` Security events (`LogEntry::eventClass == EventClass::Security`) must always return `true`; this is a hard contract guaranteed by the interface
- `[ ]` Add `recordDecision(const LogEntry&, bool sampled) noexcept` for feedback to adaptive rate-limiting algorithms
- `[ ]` Expose `currentRate() -> double noexcept` (fraction 0.0–1.0) and `setTargetRate(double) noexcept`
- `[ ]` Sampler exposes `sampledCount() -> size_t noexcept` and `droppedCount() -> size_t noexcept` for observability

### SAGA Logger Compaction API

- `[ ]` Define `ISAGALogCompactor` with `compact(SegmentRange) -> std::future<CompactionResult>` — returns immediately, compacts in background
- `[ ]` `CompactionResult` carries `compactedSegments` (count), `bytesSaved`, `retainedEntries`, `durationMs`
- `[ ]` Add `replay(SegmentId) -> ReplayIterator` for forward-only, read-only iteration over a compacted segment
- `[ ]` `ReplayIterator` exposes `hasNext() -> bool`, `next() -> SAGALogEntry`, and `reset()` — no random access
- `[ ]` Compaction explicitly preserves all committed SAGA entries; the interface documents that `compact()` never drops committed state

## Test Strategy

- PII detector statelessness tests: interleave calls from 32 threads with different chunk content; assert no cross-call state contamination
- Audit log integrity tests: append 10,000 events, corrupt one byte in a middle entry, call `verifyChain()` and assert `firstTamperedEntry` is correctly identified
- HKDF cache TTL tests: derive a key, advance mocked clock past TTL, re-derive and assert a new key is returned (not the stale one)
- Log sampler security event tests: create entries with `EventClass::Security` and assert `shouldSample()` returns `true` for 100% of them regardless of target rate
- SAGA compaction tests: compact a segment, replay it, and verify all committed entries are present and byte-identical to pre-compaction content
- Utils pipeline lifecycle tests: register multiple stages, call `run()`, then `shutdown()` and verify all stages are torn down cleanly without leaks

## Performance Targets

- `IStreamingPIIDetector::detect()` per 1 KB chunk: **≤ 2 ms**
- `IHashChainAuditLog::append()` including hash chain computation: **≤ 500 µs**
- `IHKDFKeyCache::derive()` cache hit path: **≤ 100 ns**
- `IStructuredLogSampler::shouldSample()` decision: **≤ 50 ns**
- `ISAGALogCompactor::compact()` for 10,000 entries: **≤ 100 ms** (background, non-blocking)
- `IHashChainAuditLog::verifyChain()` over 100,000 entries: **≤ 500 ms**

## Security / Reliability

- `IStreamingPIIDetector` output (`PIIDetectionResult`, `SanitisedChunk`) never includes original PII values; only category labels, span counts, and opaque pseudonym handles are exposed
- `IHashChainAuditLog` hash chain is verified on read via `verifyChain()`; any gap or mutation in the chain is reported with the first tampered entry ID
- `IHKDFKeyCache` never stores raw key material after TTL expiry; `KeyHandle` destructor performs a zeroing wipe of key bytes before deallocation
- `IStructuredLogSampler` is contractually prohibited from dropping any entry with `EventClass::Security`; violating implementations are rejected at compile-time via `static_assert` in the base class
- `ISAGALogCompactor` preserves all committed SAGA entries during compaction; only uncommitted or superseded entries may be removed, and only after quorum acknowledgement
- `IHKDFKeyCache::derive()` accepts a `KeyContext` that includes purpose and identity fields to prevent cross-context key reuse; same raw input with different purpose yields a distinct key
