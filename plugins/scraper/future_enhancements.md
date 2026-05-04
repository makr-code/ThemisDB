# Scraper Plugin – Future Enhancements

## Scope

Planned improvements beyond `ROADMAP.md` for the `plugins/scraper/` agentic
scraper module covering search-form handling, result-list parsing, government
database integration, JS-rendered page support, and LLM evaluation.

---

## Design Constraints

- All new backends must implement the same interfaces (`IScraperJSRenderer`,
  `IScraperApiClient`, `IScraperSearchEngine`, `IScraperLLMEvaluator`,
  `IScraperMetadataWriter`) to remain testable via mock injection.
- Network access must always go through `UrlPolicy::isAllowed()` before
  any HTTP call is made.
- The gov source catalog must remain YAML-extensible so new sources can be
  added without recompilation.
- LLM calls must be guarded behind `THEMIS_ENABLE_LLM` to allow builds
  without llama.cpp.

---

## EUR-Lex SPARQL Integration

### Scope
Query the EUR-Lex CELLAR SPARQL endpoint for structured legal metadata.

### Design Constraints
- SPARQL endpoint: `https://publications.europa.eu/webapi/rdf/sparql`
- Content-Type: `application/sparql-query` (POST)
- Response format: `application/sparql-results+json`

### Required Interfaces
- New `SparqlApiClient : IScraperApiClient` subclass
- `SparqlQueryBuilder::build(gap_context)` → SPARQL SELECT query string

### Implementation Notes
- Use gap_context keywords to build a FILTER / regex condition
- Map `results.bindings` array to `ApiResult` (title from `label.value`,
  url from `work.value`, date from `date.value`)

### Test Strategy
- Mock SPARQL response JSON fixture
- Test query builder with various GapContext configurations
- Verify ApiResult mapping

### Performance Targets
- ≤ 5 s per 100 results from SPARQL endpoint

---

## Parallel Crawl with Per-Domain Rate Limiting

### Scope
Replace the sequential fetch loop with a thread-pool-based parallel crawler.

### Design Constraints
- Max `crawl_options.max_concurrent_requests` parallel requests (default 4)
- Token-bucket rate limiter per domain (default: 1 req/s per domain)
- Thread-safe `results_` and `stats_` access (already mutex-guarded)

### Required Interfaces
- `CrawlScheduler` (work-stealing queue, per-domain rate limiter)
- `ThreadPool` (reuse existing ThemisDB thread pool from ingestion module)

### Test Strategy
- Test that max_concurrent_requests is respected via semaphore mock
- Test per-domain rate limiting with a mock clock

### Performance Targets
- ≥ 4× throughput improvement vs. sequential crawl at 4 workers

---

## Incremental Checkpointing

### Scope
Allow interrupted scraper runs to resume from the last committed URL.

### Design Constraints
- Checkpoint key: `scraper:<gap_id>:<source_id>` → last processed URL
- Backend: file-based by default; pluggable (Redis, RocksDB)
- Commit after every accepted document

### Required Interfaces
- `IScraperCheckpointStore` with `commit(gap_id, source_id, url)` / `last(gap_id, source_id)`
- `FileScraperCheckpointStore` (default)

### Test Strategy
- Simulate crash after N documents; verify resume skips already-seen URLs

---

## Full DB Write Integration

### Scope
Replace `InMemoryScraperMetadataWriter` with a production writer that calls
the ThemisDB AQL, graph, and vector APIs.

### Required Interfaces
- `ThemisDBScraperMetadataWriter : IScraperMetadataWriter`
  - Relational: AQL `INSERT` into `scraper_documents` collection
  - Graph: `GraphModule::addNode()` / `addEdge()`
  - Vector: `IAnnIndex::build()` with generated embeddings

### Implementation Notes
- Embedding generation via `LLMPluginManager::embed()` (once available)
- Deduplication by doc_id before insert

---

## Prometheus Metrics Export

### Scope
Add Prometheus-compatible metrics for observability.

### Metrics
| Metric | Type | Description |
|--------|------|-------------|
| `scraper_docs_scraped_total` | Counter | Total documents scraped |
| `scraper_docs_accepted_total` | Counter | Documents above threshold |
| `scraper_docs_discarded_total` | Counter | Documents below threshold |
| `scraper_quality_score_histogram` | Histogram | Quality score distribution |
| `scraper_gap_relevance_histogram` | Histogram | Gap relevance distribution |
| `scraper_run_duration_seconds` | Gauge | Last run duration |

### Implementation Notes
- Extend `ScraperRunStats` with Prometheus text format serialiser
- Expose via `ScraperPlugin::exportMetrics()` → `std::string`
