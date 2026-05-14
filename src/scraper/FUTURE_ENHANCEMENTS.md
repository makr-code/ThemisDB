> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ROADMAP.md · ../../include/scraper/README.md -->

# Future Enhancements — Scraper Module

---

## 1. Per-Source Retry with Exponential Backoff

### Scope
Add configurable retry logic to `ScraperPlugin::fetchPage()` and
`HttpScraperApiClient::fetchAll()` so transient network errors (timeout,
connection reset, 5xx response) are retried automatically before failing.

### Design Constraints
- Config keys added to `CrawlOptions` and `ApiOptions`:
  `max_retries` (default 3), `retry_base_delay_ms` (default 1000),
  `retry_max_delay_ms` (default 30000).
- Jitter: each retry delay = `min(base * 2^attempt + rand(0, base), max_delay)`.
- HTTP 429 (Rate Limited): use `Retry-After` header value when present.
- Non-retryable: 4xx responses except 429 and 408; scheme errors; SSL errors.

### Required Interfaces
```cpp
// CrawlOptions addition
int max_retries       = 3;
int retry_base_delay_ms = 1000;
int retry_max_delay_ms  = 30000;

// Exposed for tests
struct RetryOutcome { int attempts; bool succeeded; std::string last_error; };
```

### Implementation Notes
- Retry loop wraps the existing `fetch_fn_` call in `ScraperPlugin`.
- `ScraperRunStats` gains `total_retries` and `retry_failures` fields.

### Test Strategy
- Unit: inject HTTP function that fails once then succeeds; verify `attempts == 2`.
- Unit: inject HTTP function that always returns 429; verify `retry_failures == 1`.
- Unit: inject 404 response; verify no retry (non-retryable).
- Perf: retry overhead on the happy path must be < 0.1 ms per request.

### Performance Targets
- No overhead on the happy path (single attempt, no retry logic executed).
- Retry loop sleep must not block other pending fetches (thread-model TBD).

### Security / Reliability
- Max retry budget (`max_retries`) caps total attempts to prevent infinite loops.
- `retry_max_delay_ms` caps sleep duration to prevent large stalls.

---

## 2. Structured Per-Run Event Log

### Scope
Add a structured logging sink to `ScraperPlugin` that emits JSON Lines events
during a run. Events cover: seed start/end, URL fetch, form discovery,
document evaluation, write outcome, and run summary.

### Design Constraints
- Sink injected via `ScraperPlugin::setLogSink(std::ostream*)` before `initialize()`.
- Null sink = no logging (default behavior unchanged).
- All emitted objects must be valid JSON (no bare strings, no trailing commas).
- Log output must not contain PII (URL paths are included; full HTML is not).

### Required Interfaces
```cpp
// scraper_plugin.h addition
using LogSink = std::ostream;
void setLogSink(LogSink* sink);     // nullptr disables logging
```

### Event Format
```json
{"ts":"2026-05-13T07:00:00Z","event":"RUN_START","gap_id":"GAP-001","seeds":3}
{"ts":"2026-05-13T07:00:00Z","event":"FETCH","url":"https://example.com/","status":200,"elapsed_ms":42}
{"ts":"2026-05-13T07:00:00Z","event":"DOC_EVALUATED","url":"https://example.com/doc/1","quality":0.87,"gap_relevance":0.91,"accepted":true}
{"ts":"2026-05-13T07:00:00Z","event":"DOC_WRITTEN","doc_id":"abcdef1234567890","source":"gesetze_im_internet"}
{"ts":"2026-05-13T07:00:00Z","event":"RUN_END","docs_scraped":42,"docs_accepted":38,"elapsed_ms":8100}
```

### Implementation Notes
- Events emitted under the existing `mutex_` lock to prevent interleaved output.
- Use monotonic clock for `elapsed_ms` fields; `ts` from system clock (ISO-8601).

### Test Strategy
- Unit: inject `std::ostringstream` as sink; run with mock deps; verify
  `RUN_START`, at least one `DOC_EVALUATED`, and `RUN_END` events appear.
- Unit: all emitted lines must parse as valid JSON.
- Unit: null sink — no crash, no output.

### Performance Targets
- Log emission overhead ≤ 0.5 ms per event at P99.
- Log output must not be held in memory; must flush to sink incrementally.

---

## 3. Content-Hash Deduplication Across Runs

### Scope
Prevent re-processing and re-writing documents that have already been ingested
in a previous run by maintaining a persistent seen-URL (or content-hash) store.

### Design Constraints
- Interface `ISeenUrlStore` with `contains(url)` / `mark(url)` / `flush()`.
- Default production implementation: SQLite-backed store (file path from config).
- `InMemorySeenUrlStore` test double with `clear()` helper.
- Config key: `crawl_options.dedup_store_path` — path to SQLite DB; empty = dedup disabled.

### Required Interfaces
```cpp
class ISeenUrlStore {
public:
    virtual ~ISeenUrlStore() = default;
    virtual bool contains(const std::string& url) const = 0;
    virtual void mark(const std::string& url) = 0;
    virtual bool flush() = 0;
};
```

### Implementation Notes
- `ScraperPlugin::processDocument()` checks the store before calling `evaluate()`.
- `mark()` is called after a successful write, not on every fetch (to allow retry
  if a previous run crashed before writing).
- SQLite schema: single table `seen_urls (url TEXT PRIMARY KEY, seen_at TEXT)`.

### Test Strategy
- Unit: first run marks URL; second run with same seed sees `contains(url) == true` and skips.
- Unit: write failure on first run — URL not marked; second run retries.
- Perf: `contains()` for 100k entries ≤ 1 ms.

### Performance Targets
- `contains()` read: ≤ 1 ms for store up to 1M entries.
- `mark()` + `flush()` batch: ≤ 10 ms for 1000 new entries.

### Security / Reliability
- SQLite file must be writable only by the ThemisDB process (chmod 0600).
- Corrupt store triggers a warning log and disables dedup for that run (fail-open
  for dedup; dedup is an optimization, not a correctness requirement).

---

## 4. Incremental Crawl via HTTP Conditional Requests

### Scope
Reduce unnecessary page fetches by storing `ETag` / `Last-Modified` headers
per URL and sending `If-None-Match` / `If-Modified-Since` on subsequent visits.
Pages returning HTTP 304 are skipped without re-evaluation.

### Design Constraints
- Requires `ISeenUrlStore` to also persist `etag` and `last_modified` per URL.
- Config key: `crawl_options.incremental: true` (default `false`).
- Only applicable to `STATIC` render mode.

### Required Interfaces
```cpp
// ISeenUrlStore extension
virtual void markWithHeaders(const std::string& url,
                              const std::string& etag,
                              const std::string& last_modified) = 0;
virtual bool getHeaders(const std::string& url,
                        std::string& out_etag,
                        std::string& out_last_modified) const = 0;
```

### Test Strategy
- Unit: inject HTTP function returning 304; verify URL counted as visited,
  no document evaluated, no write attempted.
- Unit: inject ETag mismatch (200 + new content); verify document processed normally.
- Perf: conditional request overhead vs. unconditional request ≤ 5 %.

---

## 5. Prometheus Metrics Export for Run Stats

### Scope
Expose `ScraperRunStats` counters as Prometheus gauge/counter metrics via
ThemisDB's existing Prometheus registry so run progress and quality metrics
can be scraped by an external monitoring system.

### Design Constraints
- Metrics registered at `initialize()` time with a `scraper_` prefix.
- `scrape()` updates metrics atomically at run end.
- No Prometheus dependency when `THEMIS_ENABLE_PROMETHEUS` is not defined.

### Planned Metrics

| Metric | Type | Labels |
|---|---|---|
| `scraper_docs_scraped_total` | Counter | `gap_id` |
| `scraper_docs_accepted_total` | Counter | `gap_id` |
| `scraper_docs_discarded_total` | Counter | `gap_id` |
| `scraper_write_errors_total` | Counter | `gap_id` |
| `scraper_run_elapsed_ms` | Gauge | `gap_id` |
| `scraper_quality_score_avg` | Gauge | `gap_id` |

### Test Strategy
- Unit: run plugin with mock deps; verify metric values via registry getter.
- Unit: `THEMIS_ENABLE_PROMETHEUS` undefined — no crash, no metric registration.

### Performance Targets
- Metric update overhead at run end: ≤ 0.1 ms.

---

## 6. Authenticated Gov Source Support

### Scope
Enable gov sources with `requires_auth = true` by supporting configurable
credential injection (API keys, OAuth2 client credentials, bearer tokens).

### Design Constraints
- Credentials injected via environment variables (referenced by `GovDataSource::api_key_env`)
  or via a new `ScraperCredentialStore` interface.
- Credentials must never be logged.
- Credential refresh (OAuth2 token expiry) must be handled transparently.

### Required Interfaces
```cpp
class IScraperCredentialStore {
public:
    virtual ~IScraperCredentialStore() = default;
    virtual std::string getToken(const std::string& source_id) const = 0;
    virtual void refreshToken(const std::string& source_id) = 0;
};
```

### Security / Reliability
- Credentials stored in memory only (no write-back to disk from this interface).
- HTTP 401 response triggers one token refresh attempt before failing.
- Token values must not appear in log output or `ScrapedDocument` metadata.

### Test Strategy
- Unit: inject credential store returning a bearer token; verify `Authorization`
  header is set on API requests.
- Unit: inject 401 response; verify refresh is called once and request is retried.
- Security: verify no credential value appears in any log event.
