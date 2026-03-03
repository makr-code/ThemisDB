# Timeseries Module - Future Header Enhancements

## Scope

- `ITimeSeriesStore` interface extensions for continuous aggregation and multi-tier query dispatch
- Continuous aggregation query API (`IContinuousAggregator`) with watermark-driven incremental updates
- Multi-tier downsampling pipeline interface (`IDownsamplingPipeline`) preserving source chunks
- Chunk-level encryption API (`IChunkEncryption`) with per-collection key derivation
- Backpressure signal interface (`IBackpressureSignal`) for adaptive flush coordination
- Gorilla codec extension interface (`IGorillaCodec`) for pluggable SIMD-accelerated chunk encoding

## Design Constraints

- `[ ]` Continuous aggregation queries are **append-only**; aggregators never modify or delete source chunks
- `[ ]` Downsampling is non-destructive; source chunk data is always retained and accessible via the original `ITimeSeriesStore` path
- `[ ]` Chunk encryption key is derived per-collection; no two collections share the same derived key
- `[ ]` `IBackpressureSignal` callbacks must be `noexcept`; backpressure signalling must never abort an in-flight write
- `[ ]` `IGorillaCodec` is stateless per encode/decode call; codec implementations must be safe to call from multiple threads concurrently
- `[ ]` `IDownsamplingPipeline` tier configuration is immutable after the pipeline is started; tier changes require a full pipeline restart

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `ITimeSeriesStore` | Query engine, ingest path | Extended with `queryRange()`, `appendChunk()`, `getChunkEncryption()`; all range queries return typed iterators |
| `IContinuousAggregator` | Query planner, materialized view manager | Exposes `registerQuery(AggregateSpec) -> AggregatorHandle`, `advance(Watermark)`, `materialize() -> AggregateResult` |
| `IDownsamplingPipeline` | Archive manager, dashboard query path | Exposes `addTier(TierConfig)`, `downsample(ChunkRef, TierLevel) -> DownsampledChunk`; non-destructive |
| `IChunkEncryption` | Storage layer, backup manager | Exposes `encrypt(ChunkRef) -> EncryptedChunk`, `decrypt(EncryptedChunk) -> ChunkRef`; key never in public API |
| `IBackpressureSignal` | Write coordinator, flush manager | `noexcept`; exposes `onPressureHigh() noexcept`, `onPressureNormal() noexcept`, `currentLevel() -> PressureLevel` |
| `IGorillaCodec` | Chunk store, ingest pipeline | Stateless; exposes `encode(std::span<const TimePoint>) -> EncodedChunk`, `decode(EncodedChunk) -> std::vector<TimePoint>` |

## Planned Features

### Continuous Aggregation Query Interface

- `[ ]` Define `IContinuousAggregator` with `registerQuery(const AggregateSpec&) -> AggregatorHandle`
- `[ ]` `AggregateSpec` declares `windowSize`, `slideInterval`, `aggregateFunctions` (enum: `Sum`, `Avg`, `Min`, `Max`, `Count`), and `watermarkDelay`
- `[ ]` Add `advance(Watermark wm)` to push the watermark forward and trigger incremental materialization
- `[ ]` `AggregatorHandle` is RAII; destruction unregisters the aggregation query and releases associated state

### Multi-Tier Downsampling Pipeline API

- `[ ]` Define `IDownsamplingPipeline` with `addTier(const TierConfig&)` — tiers are ordered by resolution (finest first)
- `[ ]` `TierConfig` exposes `retentionDuration`, `sampleInterval`, `aggregationFunction`, `compressionCodec`
- `[ ]` Add `downsample(const ChunkRef&, TierLevel) -> std::future<DownsampledChunk>` for async per-tier downsampling
- `[ ]` Pipeline exposes `tierCount() -> size_t` and `tierConfig(TierLevel) -> const TierConfig&` for introspection

### Chunk-Level Encryption Interface

- `[ ]` Define `IChunkEncryption` with `encrypt(const ChunkRef&) -> EncryptedChunk` and `decrypt(const EncryptedChunk&) -> ChunkRef`
- `[ ]` `IChunkEncryption` is obtained via `ITimeSeriesStore::getChunkEncryption(CollectionId)` — key scope is per-collection
- `[ ]` Encryption metadata (algorithm ID, IV) is stored in `EncryptedChunk` header; raw key material is never exposed
- `[ ]` Add `rotateKey(CollectionId) -> std::future<KeyRotationResult>` for online key rotation without downtime

### Backpressure Signal API

- `[ ]` Define `IBackpressureSignal` with `onPressureHigh() noexcept` and `onPressureNormal() noexcept`
- `[ ]` Add `currentLevel() -> PressureLevel noexcept` (enum: `Normal`, `Elevated`, `High`, `Critical`)
- `[ ]` `ITimeSeriesStore` gains `registerBackpressureSignal(IBackpressureSignal&)` returning `SignalHandle` (RAII deregister)
- `[ ]` Backpressure level transitions are edge-triggered; `onPressureHigh` fires once per `Normal→High` transition, not continuously

### Gorilla Codec Extension Interface

- `[ ]` Define `IGorillaCodec` with `encode(std::span<const TimePoint>) -> EncodedChunk`
- `[ ]` Add `decode(const EncodedChunk&) -> std::vector<TimePoint>` — both are stateless, thread-safe, and `[[nodiscard]]`
- `[ ]` `IGorillaCodec` exposes `codecId() -> std::string_view` and `supportsSimd() -> bool` for capability detection
- `[ ]` `ITimeSeriesStore` accepts a codec override via `setCodec(std::unique_ptr<IGorillaCodec>)` at construction time only

## Test Strategy

- Continuous aggregation tests: insert 1M time points, advance watermark, assert materialized aggregates match brute-force reference
- Downsampling non-destructive tests: downsample to all tiers and verify source chunks are byte-identical via `ITimeSeriesStore::queryRange()`
- Chunk encryption round-trip tests: encrypt then decrypt and assert bit-exact recovery; also test that raw key material is absent from `EncryptedChunk`
- Backpressure signal tests: simulate buffer fill to trigger `onPressureHigh`, verify only one callback fires per level transition
- Gorilla codec tests: encode/decode cycle with known time-series data and assert lossless recovery; verify thread-safety with 32 concurrent workers
- Performance regression tests for all interfaces executed in CI with threshold assertions matching the targets below

## Performance Targets

- `ITimeSeriesStore` time-range query over 1M points: **≤ 10 ms**
- `IContinuousAggregator::advance()` dispatch latency: **≤ 1 ms**
- `IDownsamplingPipeline::downsample()` per tier for a 64 KB chunk: **≤ 5 ms**
- `IChunkEncryption::encrypt()` overhead per chunk: **≤ 50 µs**
- `IBackpressureSignal` callback invocation overhead: **≤ 100 ns**
- `IGorillaCodec::encode()` throughput: **≥ 500 MB/s** on a single core

## Security / Reliability

- Chunk encryption keys are derived per-collection and never exposed through any public API surface; `IChunkEncryption` exposes only ciphertext operations
- Downsampling preserves privacy: aggregated tier outputs never include raw individual sample values, only statistical aggregates
- Time-series collection access is controlled by collection-level ACL; `ITimeSeriesStore` rejects operations without a valid `AccessToken`
- `IBackpressureSignal` callbacks are `noexcept` by contract; any violation (exception escaping) is treated as a fatal interface contract breach
- `IContinuousAggregator` watermark advancement is monotonic; rewinding the watermark is rejected to prevent re-materialisation of stale aggregates
- `IGorillaCodec` implementations must not buffer encoded data beyond the lifetime of the `encode()` call to prevent data leakage across contexts
