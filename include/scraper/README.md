> **Build (Linux):** `cmake --preset linux-release && cmake --build --preset linux-release`<br>
> **Build (Windows):** `cmake --preset windows-release && cmake --build --preset windows-release`

# ThemisDB Scraper Module Headers

<!-- Status: current | validated: 2026-05-13 | Primary: src/scraper/ | Secondary: docs/de/scraper/ -->
<!-- Links: ../../src/scraper/README.md · ../../src/scraper/ROADMAP.md · ../../src/scraper/FUTURE_ENHANCEMENTS.md -->

This folder exposes the public C++ header API for the agentic web-scraper plugin.
All headers are included as `scraper/<header>.h` because in-tree builds expose
`include/` on the include path.

---

## Public Entry Points

| Header | Purpose |
|---|---|
| `scraper/scraper_plugin.h` | Top-level entry point — `IScraperPlugin`, `ScraperPlugin`, `ScrapedDocument`, `ScraperRunStats` |
| `scraper/scraper_config.h` | Configuration types — `ScraperConfig`, `ScraperRenderMode`, `GapContext`, `UrlPolicy` |
| `scraper/scraper_api_client.h` | JSON/REST API client — `IScraperApiClient`, `HttpScraperApiClient`, `ApiEndpointConfig`, `ApiResult` |
| `scraper/scraper_search_engine.h` | HTML search-form engine — `IScraperSearchEngine`, `HtmlSearchEngine`, `SearchForm`, `SearchResultPage` |
| `scraper/scraper_llm_evaluator.h` | LLM quality evaluator — `IScraperLLMEvaluator`, `ScraperLLMEvaluator`, `EvaluationResult` |
| `scraper/scraper_metadata_writer.h` | Persistence layer — `IScraperMetadataWriter`, `ScraperRelationalRecord`, `ScraperRecordBuilder` |
| `scraper/scraper_js_renderer.h` | Headless JS renderer — `IScraperJSRenderer`, `SubprocessJSRenderer` |
| `scraper/gov_source_catalog.h` | Government source registry — `GovSourceCatalog`, `GovDataSource` |

---

## Public API Surface

### `IScraperPlugin` / `ScraperPlugin`

The primary plugin interface and its production implementation.

```cpp
// Lifecycle
bool initialize(const ScraperConfig& config);
ScraperRunStats scrape();
const std::vector<ScrapedDocument>& getResults() const;
void reset();
bool isInitialized() const;

// Dependency injection (for tests / custom backends)
void setEvaluator(std::shared_ptr<IScraperLLMEvaluator> e);
void setWriter(std::shared_ptr<IScraperMetadataWriter> w);
void setSearchEngine(std::shared_ptr<IScraperSearchEngine> se);
void setJsRenderer(std::shared_ptr<IScraperJSRenderer> r);
void setApiClient(std::shared_ptr<IScraperApiClient> c);
void setHttpFetch(HttpFn fn);   // inject custom HTTP backend (replaces libcurl)
```

### `ScrapedDocument`

Result record produced for every accepted document.

| Field | Type | Description |
|---|---|---|
| `url` | `string` | Source URL |
| `title` | `string` | Page title |
| `raw_html` | `string` | Raw HTML of the page |
| `extracted_text` | `string` | Plain text extracted from HTML |
| `source_name` | `string` | Gov source id or hostname |
| `document_type` | `string` | `"Urteil"`, `"Gesetz"`, `"API"`, … |
| `quality_score` | `double` | 0.0–1.0; produced by the LLM evaluator |
| `gap_relevance` | `double` | 0.0–1.0; relevance to the gap context |
| `discarded` | `bool` | `true` when the document was below threshold |
| `discard_reason` | `string` | Non-empty when `discarded == true` |
| `doc_id` | `string` | FNV-1a content hash (16 hex chars) |
| `metadata` | `map<string,string>` | Arbitrary extra metadata |
| `is_scraper_ingested` | `bool` | **Provenance** — always `true` |
| `ingestion_source_type` | `string` | **Provenance** — always `"SCRAPER"` |
| `ingestion_plugin_version` | `string` | **Provenance** — semver of the plugin |

### `ScraperRunStats`

Counters returned by `scrape()`.

| Field | Description |
|---|---|
| `urls_visited` | Total URLs fetched |
| `forms_submitted` | HTML search forms submitted |
| `result_pages_parsed` | Search result pages paginated through |
| `api_pages_fetched` | JSON REST API pages fetched |
| `docs_scraped` | Raw documents collected |
| `docs_accepted` | Documents that passed the quality threshold |
| `docs_discarded` | Documents that were filtered out |
| `docs_written` | Documents successfully persisted |
| `write_errors` | Persistence failures |
| `elapsed_ms` | Total run duration |

### `ScraperConfig`

```cpp
static ScraperConfig loadFromFile(const std::string& path);
static ScraperConfig loadFromYaml(const std::string& yaml_content);
std::vector<std::string> effectiveSearchQueries() const;
```

Key sub-structs and their defaults:

| Sub-struct | Key fields |
|---|---|
| `GapContext` | `gap_id`, `description`, `keywords` |
| `CrawlOptions` | `max_depth=3`, `max_pages=500`, `user_agent`, `respect_robots=true`, `same_domain_only=true`, `request_delay_ms=250`, `render_mode=STATIC` |
| `SearchOptions` | `enabled=true`, `queries`, `max_result_pages=10`, `results_per_page=20`, `result_list_selector` |
| `ApiOptions` | `pagination_mode="page"`, `page_param="p"`, `results_field="results"`, `max_pages=20`, `headers`, `body_template` |
| `LlmOptions` | `quality_threshold=0.65`, `model_path`, `temperature=0.1` |
| `GovSourcesOptions` | `bund_enabled`, `bundeslaender_enabled`, `eu_enabled`, `source_ids`, `custom_catalog_path` |

`ScraperConfig` also carries `seed_urls`, `whitelist`, and `blacklist` lists that
feed into `UrlPolicy`.

### `ScraperRenderMode`

| Value | Description |
|---|---|
| `STATIC` | Plain HTTP GET + HTML parsing (libcurl, default) |
| `JS_RENDERED` | Headless browser renders the page before extraction |
| `API_JSON` | Seed URL is a JSON REST endpoint |
| `API_GRAPHQL` | Seed URL is a GraphQL endpoint; queries sent as POST |

### `UrlPolicy`

Controls which URLs may be scraped:
- Whitelist: empty = allow all; non-empty = URL must match at least one prefix/glob.
- Blacklist: URL matching any entry is blocked regardless of the whitelist.
- Schemes `http` and `https` only — SSRF guard rejects other schemes unconditionally.

### `IScraperApiClient` / `HttpScraperApiClient`

```cpp
std::vector<ApiResult> fetchAll(const ApiEndpointConfig& cfg, const std::string& query);
```

Handles `page`, `cursor`, and `offset` pagination automatically.
When `THEMIS_ENABLE_CURL` is not defined, `fetchAll()` returns an empty vector
(build environments without libcurl compile and link cleanly).

`InMemoryScraperApiClient` is a test double that returns pre-injected results.

### `IScraperSearchEngine` / `HtmlSearchEngine`

```cpp
std::vector<SearchForm> discoverForms(const std::string& html, const std::string& base_url) const;
SearchResultPage parseResults(const std::string& html, const std::string& base_url,
                              const std::string& selector = "") const;
std::string buildSearchUrl(const SearchForm& form, const std::string& query, int page = 1) const;
std::string buildSearchBody(const SearchForm& form, const std::string& query, int page = 1) const;
```

Backed by pugixml. When `THEMIS_ENABLE_PUGIXML` is not defined, both discovery
and parsing return empty results.

`InMemorySearchEngine` is a test double for unit tests.

### `IScraperLLMEvaluator` / `ScraperLLMEvaluator`

```cpp
EvaluationResult evaluate(const std::string& text, const std::string& url,
                          const GapContext& gap, double threshold) const;
bool isLlmAvailable() const;
```

When `THEMIS_ENABLE_LLM` is defined and a model is loaded via
`LLMPluginManager`, the evaluator sends a structured prompt and parses the JSON
response.  Otherwise a keyword-frequency heuristic is used as fallback.

`InMemoryLLMEvaluator` is a test double with per-URL override support.

### `IScraperMetadataWriter` / `InMemoryScraperMetadataWriter`

```cpp
WriteResult write(const ScraperRelationalRecord& rel,
                  const ScraperGraphNode& node,
                  const std::vector<ScraperGraphEdge>& edges,
                  const ScraperVectorRecord& vec);
bool flush();
```

`ScraperRecordBuilder` is a factory helper that constructs all three record
types from a scraped document and unconditionally stamps provenance fields.

### `IScraperJSRenderer` / `SubprocessJSRenderer`

```cpp
JsRenderResult render(const JsRenderRequest& req);
bool isAvailable() const;
```

`SubprocessJSRenderer` invokes an external headless browser via subprocess.
The command receives `<url> [--timeout <ms>] [--wait-for <selector>] [extra_args…]`
and must write rendered HTML to stdout and exit 0 on success.

`InMemoryJSRenderer` is a test double.

### `GovSourceCatalog`

Registry of official government data sources (Bund, 16 Bundesländer, EU).

```cpp
const std::vector<GovDataSource>& all() const;
const GovDataSource* findById(const std::string& id) const;
std::vector<const GovDataSource*> byType(GovSourceType type) const;
std::vector<const GovDataSource*> byBundesland(const std::string& iso) const;
std::vector<const GovDataSource*> enabled() const;
std::vector<const GovDataSource*> byIds(const std::vector<std::string>& ids) const;
void upsert(GovDataSource source);
bool setEnabled(const std::string& id, bool enabled);
void loadFromYaml(const std::string& yaml_content);
void loadFromFile(const std::string& path);
```

---

## Configuration — YAML Format

```yaml
gap_context:
  gap_id: "GAP-001"
  description: "Aktuelle Rechtsprechung zu §... BGB"
  keywords: ["BGB", "Vertragsrecht", "Schadensersatz"]

crawl_options:
  max_depth: 3
  max_pages: 500
  user_agent: "ThemisDB-Scraper/1.0"
  respect_robots: true
  same_domain_only: true
  request_delay_ms: 250
  render_mode: STATIC        # STATIC | JS_RENDERED | API_JSON | API_GRAPHQL
  js_renderer_cmd: ""        # Required for JS_RENDERED mode
  js_timeout_ms: 10000

search_options:
  enabled: true
  queries: []                # Empty = use gap_context.keywords
  max_result_pages: 10
  results_per_page: 20
  result_list_selector: ""   # Optional CSS hint, e.g. ".result-list"

api_options:
  pagination_mode: "page"    # page | cursor | offset | none
  page_param: "p"
  cursor_field: "next_cursor"
  results_field: "results"
  max_pages: 20
  headers: {}
  body_template: ""          # POST body; {{QUERY}}, {{PAGE}}, {{CURSOR}} substituted

llm_options:
  quality_threshold: 0.65
  model_path: ""             # GGUF path; empty = use plugin manager
  temperature: 0.1

gov_sources:
  bund_enabled: false
  bundeslaender_enabled: false
  eu_enabled: false
  source_ids: []             # e.g. ["gesetze_im_internet", "eurlex"]
  custom_catalog_path: ""    # Path to YAML overlay file

seed_urls:
  - "https://example.com/search"
whitelist:
  - "https://example.com/"
blacklist:
  - "https://example.com/private/"
```

---

## Runtime Behavior, Error Cases, and Limits

- `initialize()` must be called and must return `true` before `scrape()` may be called;
  `scrape()` on an uninitialized plugin throws `std::logic_error`.
- HTTP fetch errors (network timeout, non-200 responses) are caught per-URL; the
  scraper logs the error and continues with remaining seeds rather than aborting.
- LLM evaluation failures fall back to the heuristic scorer; the run never aborts
  due to an LLM error.
- JS rendering subprocess timeout (`js_timeout_ms`) results in an error
  `JsRenderResult{success=false}` for that page; the scraper logs and skips.
- `UrlPolicy` rejects non-http/https schemes silently (SSRF guard); the URL is
  counted as visited but no fetch is performed.
- libcurl is required at runtime when `render_mode != JS_RENDERED` and
  `THEMIS_ENABLE_CURL` is defined; without libcurl, HTTP fetches return empty strings.
- `ScraperRecordBuilder` provenance fields (`is_scraper_ingested`,
  `ingestion_source_type`, `ingestion_plugin_version`) must not be cleared or
  overridden by the caller — they are the authoritative ingestion trail.
- `max_pages` caps the scraper at a safe upper bound to prevent runaway crawls;
  the default is 500 pages per seed URL.
- `request_delay_ms` (default: 250 ms) enforces polite crawl rate limiting between requests.

---

## Usage Example

```cpp
#include "scraper/scraper_plugin.h"
#include "scraper/scraper_config.h"

using namespace themis::scraper;

// 1. Load config
ScraperConfig cfg = ScraperConfig::loadFromFile("/etc/themis/scraper.yaml");

// 2. Create plugin (uses default production dependencies)
ScraperPlugin plugin;

// 3. Initialize and run
if (!plugin.initialize(cfg)) {
    throw std::runtime_error("Scraper initialization failed");
}

ScraperRunStats stats = plugin.scrape();

// 4. Inspect results
for (const ScrapedDocument& doc : plugin.getResults()) {
    if (!doc.discarded) {
        std::cout << doc.title << " [" << doc.quality_score << "]\n";
    }
}

// 5. Reset for a new run
plugin.reset();
```

### Injecting Custom Dependencies (Tests)

```cpp
#include "scraper/scraper_plugin.h"

auto writer   = std::make_shared<InMemoryScraperMetadataWriter>();
auto evaluator = std::make_shared<InMemoryLLMEvaluator>();
auto se       = std::make_shared<InMemorySearchEngine>();

ScraperPlugin plugin;
plugin.setWriter(writer);
plugin.setEvaluator(evaluator);
plugin.setSearchEngine(se);
plugin.setHttpFetch([](const std::string& url, const std::string&) {
    return "<html><body>Test content</body></html>";
});

plugin.initialize(cfg);
plugin.scrape();
assert(writer->relationalRecords().size() == 1);
```

---

## Installation

For in-tree builds both include roots must be visible:

```cmake
target_include_directories(your_target PRIVATE
    ${THEMISDB_INCLUDE_DIR}          # include/
    ${THEMISDB_SOURCE_DIR}/src       # src/ (for source-local headers)
)
```

Optionally enable backend features:

```cmake
target_compile_definitions(your_target PRIVATE
    THEMIS_ENABLE_CURL     # link libcurl for HTTP fetch
    THEMIS_ENABLE_PUGIXML  # link pugixml for HTML parsing
    THEMIS_ENABLE_LLM      # enable LLM evaluator path
)
```

---

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---|---|---|
| `scrape()` returns 0 docs | Whitelist too restrictive or all seeds blocked | Widen `whitelist` or check seed URL validity |
| `docs_accepted == 0` | `quality_threshold` too high | Lower `llm_options.quality_threshold` or inspect `discard_reason` |
| JS_RENDERED pages return empty | `js_renderer_cmd` not set or binary not on PATH | Set `crawl_options.js_renderer_cmd` to the renderer script path |
| `write_errors > 0` | DB connection unavailable or metadata writer not injected | Verify DB connection; inject a custom `IScraperMetadataWriter` |
| High latency per page | `request_delay_ms` too high or JS rendering overhead | Tune `request_delay_ms`; switch to `STATIC` mode where possible |
| LLM evaluation unavailable | `THEMIS_ENABLE_LLM` not defined or no model loaded | Heuristic fallback is used automatically; load a GGUF model if LLM quality scoring is required |

---

## See Also

- [`../../src/scraper/README.md`](../../src/scraper/README.md) — implementation overview
- [`../../src/scraper/ROADMAP.md`](../../src/scraper/ROADMAP.md) — phased delivery status
- [`../../src/scraper/FUTURE_ENHANCEMENTS.md`](../../src/scraper/FUTURE_ENHANCEMENTS.md) — planned feature work
- [`../../src/scraper/PERFORMANCE_EXPECTATIONS.md`](../../src/scraper/PERFORMANCE_EXPECTATIONS.md) — performance targets
- [`../../docs/de/scraper/README.md`](../../docs/de/scraper/README.md) — German overview
