# Content Module - Future Header Enhancements

## Scope

- `IContentProcessor` interface extensions for multi-format document ingestion (PDF, Office, HTML)
- Streaming ingestion interface (`IStreamingIngester`) using a pull-based cursor for large file processing
- Perceptual hash API (`IPerceptualHasher`) for image and document deduplication at the interface level
- OCR backend interface (`IOCRBackend`) as a compile-time-optional extension to `IContentProcessor`
- Embedding pipeline interface (`IEmbeddingPipeline`) for async embedding generation from ingested content
- Deduplication hook (`IDeduplicationHook`) for plugging custom dedup strategies into the ingestion path

## Design Constraints

- `[ ]` Content processors implementing `IContentProcessor` are stateless per call; no per-call state persists across invocations
- `[ ]` Streaming API uses a pull-based cursor model; consumers call `IStreamingIngester::nextChunk()` — no push callbacks
- `[ ]` OCR backend is compile-time optional via `THEMIS_ENABLE_OCR`; `IOCRBackend` symbols are absent when the guard is undefined
- `[ ]` Embedding pipeline dispatch is asynchronous; `IEmbeddingPipeline::submit()` returns a `EmbeddingFuture<T>` without blocking
- `[ ]` Deduplication hash computation is deterministic; identical inputs must produce identical `PerceptualHash` values across runs
- `[ ]` All content processor methods sanitize file path arguments; relative paths and path traversal sequences are rejected at the interface boundary

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IContentProcessor` | `IngestionPipeline`, `SearchIndexer`, `EmbeddingLayer` | Base processor interface; `process(ContentDescriptor) -> Result<ProcessedContent>` is stateless |
| `IStreamingIngester` | `IngestionPipeline`, `LargeFileHandler` | Pull-based cursor; `nextChunk() -> Result<ContentChunk>` returns `EndOfStream` when exhausted |
| `IPerceptualHasher` | `DeduplicationService`, `ContentIndexer` | Computes `PerceptualHash` for images and document pages; deterministic and pure |
| `IOCRBackend` | `IContentProcessor` implementations, `OCROrchestrator` | Compile-time optional; `recognizeText(ImageDescriptor) -> Result<OCRResult>` |
| `IEmbeddingPipeline` | `SearchIndexer`, `RAGLayer`, `VectorStore` | Async dispatch; `submit(ContentChunk, CancellationToken) -> EmbeddingFuture<EmbeddingVector>` |
| `IDeduplicationHook` | `IngestionPipeline`, `ContentIndexer` | Called before indexing; `isDuplicate(PerceptualHash) -> DuplicateCheckResult` |

## Planned Features

### Streaming Content Ingestion API

- `[ ]` Define `IStreamingIngester` with `open(ContentSource&) -> Result<StreamHandle>` and `nextChunk(StreamHandle) -> Result<ContentChunk>`
- `[ ]` Expose `ContentChunk` as a plain-data struct: byte span, chunk index, total size hint, MIME type
- `[ ]` Add `IStreamingIngester::close(StreamHandle)` as a `noexcept` cleanup method
- `[ ]` Document that `nextChunk()` returns `Result::error(EndOfStream)` as the terminal condition; error is not an exception

### Perceptual Deduplication Interface

- `[ ]` Define `IPerceptualHasher::hash(ImageDescriptor) -> Result<PerceptualHash>` and `hash(DocumentPage) -> Result<PerceptualHash>`
- `[ ]` Expose `PerceptualHash` as a fixed-width (64-bit) value type with `distance(PerceptualHash) -> uint8_t` method
- `[ ]` Add `IPerceptualHasher::threshold() -> uint8_t` to expose the configured similarity threshold for duplicate detection
- `[ ]` Document that `hash()` is a pure function; identical inputs always produce identical outputs regardless of call order

### OCR Backend Interface

- `[ ]` Define `IOCRBackend` (guarded by `THEMIS_ENABLE_OCR`) with `recognizeText(ImageDescriptor) -> Result<OCRResult>`
- `[ ]` Expose `OCRResult` with recognized text, confidence score, bounding boxes, and language hint
- `[ ]` Add `IOCRBackend::supportedLanguages() -> std::span<const LanguageCode>` for runtime capability discovery
- `[ ]` Document that `recognizeText()` sanitizes output text for null bytes, control characters, and script-injection sequences

### Embedding Pipeline API

- `[ ]` Define `IEmbeddingPipeline::submit(ContentChunk, CancellationToken) -> EmbeddingFuture<EmbeddingVector>`
- `[ ]` Expose `EmbeddingVector` as a fixed-dimension float span with dimension count queryable via `IEmbeddingPipeline::dimensions()`
- `[ ]` Add `IEmbeddingPipeline::submitBatch(std::span<ContentChunk>, CancellationToken) -> std::vector<EmbeddingFuture<EmbeddingVector>>`
- `[ ]` Document that `CancellationToken` cancellation before dispatch results in `EmbeddingFuture::error(Cancelled)`; no partial embeddings are returned

### Archive / Compressed Content Processor Interface

- `[ ]` Define `IArchiveContentProcessor : IContentProcessor` for ZIP, TAR, and 7z archive traversal
- `[ ]` Expose `ArchiveEntryDescriptor` with entry path, size, compression ratio, and MIME type of the inner file
- `[ ]` Add `IArchiveContentProcessor::entries(ContentDescriptor) -> Result<std::vector<ArchiveEntryDescriptor>>` for directory listing
- `[ ]` Document that archive traversal depth is bounded by a configured `maxDepth` parameter to prevent zip-bomb attacks

## Test Strategy

- Statelessness tests invoke `IContentProcessor::process()` twice with identical inputs from two different threads and assert identical outputs with no shared state
- Streaming cursor tests open a 1 GB synthetic file via `IStreamingIngester` and assert that every chunk is delivered exactly once with no gaps
- Perceptual hash determinism tests compute `IPerceptualHasher::hash()` for the same image across 100 calls and assert all results are bit-identical
- OCR sanitization tests inject images containing HTML and SQL injection payloads and verify `OCRResult` text is sanitized before return
- Embedding pipeline cancellation tests cancel `CancellationToken` before `submit()` returns and assert `EmbeddingFuture::error(Cancelled)` with no memory leak
- Archive depth limit tests construct a 20-level nested ZIP and verify `IArchiveContentProcessor::entries()` returns `Result::error(MaxDepthExceeded)` at the configured limit

## Performance Targets

- `IStreamingIngester::nextChunk()` dispatch latency ≤ 1 ms per chunk for chunks up to 1 MB at the public interface boundary
- `IPerceptualHasher::hash()` computation ≤ 10 ms per 4K image (512×512 thumbnail input path)
- `IOCRBackend::recognizeText()` end-to-end latency ≤ 500 ms per A4 page at 300 DPI with Tesseract backend
- `IEmbeddingPipeline::submit()` dispatch overhead (call to future creation, before actual embedding) ≤ 5 ms
- `IDeduplicationHook::isDuplicate()` hash lookup ≤ 1 ms per call against an index of 10M stored hashes
- `IArchiveContentProcessor::entries()` listing overhead ≤ 50 ms for an archive with 10k entries

## Security / Reliability

- All `IContentProcessor` implementations sanitize file path arguments at the interface boundary; paths containing `..`, null bytes, or absolute segments outside the configured root are rejected with `Result::error(InvalidPath)`
- `IOCRBackend::recognizeText()` sanitizes recognized text output for HTML/script injection sequences before returning `OCRResult`
- `IEmbeddingPipeline::submit()` validates `ContentChunk` token count against the configured `maxTokens` limit; oversized inputs return `Result::error(InputTooLarge)` rather than truncating silently
- `IArchiveContentProcessor` enforces a `maxDepth` limit to prevent zip-bomb and tar-bomb attacks; limit is a required constructor parameter with no default
- `IStreamingIngester::open()` validates that the `ContentSource` is within the allowed content root before returning a `StreamHandle`
- `IDeduplicationHook` implementations cannot access stored document content; they receive only `PerceptualHash` values to prevent data leakage through dedup side-channels
