> **Full-repository build examples (not scraper-only):** `cmake --preset linux-release && cmake --build --preset linux-release`<br>
> **Windows:** `cmake --preset windows-release && cmake --build --preset windows-release`

# ThemisDB Scraper Module Headers

<!-- Status: current | validated: 2026-10-31 | Primary: src/scraper/ -->
<!-- Links: ../../src/scraper/README.md · ../../src/scraper/ROADMAP.md · ../../src/scraper/FUTURE_ENHANCEMENTS.md -->

**Author:** ThemisDB Contributors<br>
**Created:** 2026-10-31<br>
**Last Updated:** 2026-10-31<br>
**Status:** review

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot; themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** unknown
Exact model backend not verifiable from final document text alone.

This folder exposes the public C++ header API for the agentic web-scraper plugin.
All headers are included as `scraper/<header>.h` because in-tree builds expose
`include/` on the include path.

---

## Public Entry Points

| Header | Purpose |
|---|---|
| `scraper/scraper_plugin.h` | Top-level entry point — `IScraperPlugin`, `ScraperPlugin`, `ScrapedDocument`, `ScraperRunStats` |
| `scraper/scraper_config.h` | Configuration types — `ScraperConfig`, `ScraperRenderMode`, `GapContext`, `UrlPolicy` |
| `scraper/scraper_api_client.h` | JSON REST/GraphQL client — `IScraperApiClient`, `HttpScraperApiClient`, `ApiEndpointConfig`, `ApiResult` |
| `scraper/scraper_search_engine.h` | HTML search-form engine — `IScraperSearchEngine`, `HtmlSearchEngine`, `SearchForm`, `SearchResultPage` |
| `scraper/scraper_llm_evaluator.h` | LLM quality evaluator — `IScraperLLMEvaluator`, `ScraperLLMEvaluator`, `EvaluationResult` |
| `scraper/scraper_metadata_writer.h` | Writer contract and record builders — `IScraperMetadataWriter`, relational/graph/vector records, `ScraperRecordBuilder` |
| `scraper/scraper_js_renderer.h` | Headless JS renderer — `IScraperJSRenderer`, `SubprocessJSRenderer` |
| `scraper/gov_source_catalog.h` | Government source registry — `GovSourceCatalog`, `GovDataSource` |
| `scraper/scraper_api_contract.h` | Error/result contract types and constants |
| `scraper/scraper_burst_controller.h` | Optional per-seed token-bucket controller |
| `scraper/scraper_diagnostics.h` | Standalone fault/event/sink types; not emitted by `ScraperPlugin` |
| `scraper/scraper_render_contract.h` | Standalone renderer-result and pagination predicates; not wired into `ScraperPlugin` |
| `scraper/scraper_run_summary.h` | Standalone collector for explicitly emitted diagnostic events |
| `scraper/scraper_robots.h`, `scraper/scraper_sitemap.h` | Helper declarations; not invoked by `ScraperPlugin`, omitted from `scraper_plugin` source list, and not covered by scoped scraper tests |

---

## Public API Surface

### `IScraperPlugin` / `ScraperPlugin`

The plugin interface and its concrete orchestration implementation.

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

Result record produced for each processed document (including discarded documents).

| Field | Type | Description |
|---|---|---|
| `url` | `string` | Source URL |
| `title` | `string` | Page title |
| `raw_html` | `string` | Raw HTML of the page |
| `extracted_text` | `string` | Plain text extracted from HTML |
| `source_name` | `string` | Gov source id or hostname |
| `document_type` | `string` | `"Urteil"`, `"Gesetz"`, `"API"`, … |
| `date_issued` | `string` | Publication/decision date when present |
| `quality_score` | `double` | Evaluator output; LLM or heuristic path |
| `gap_relevance` | `double` | 0.0–1.0; relevance to the gap context |
| `discarded` | `bool` | `true` when the document was below threshold |
| `discard_reason` | `string` | Non-empty when `discarded == true` |
| `doc_id` | `string` | 16-hex FNV-1a hash of URL plus the first 512 text characters, for accepted records |
| `metadata` | `map<string,string>` | Arbitrary extra metadata |
| `is_scraper_ingested` | `bool` | **Provenance** — always `true` |
| `ingestion_source_type` | `string` | **Provenance** — always `"SCRAPER"` |
| `ingestion_plugin_version` | `string` | **Provenance** — semver of the plugin |

### `ScraperRunStats`

Counters returned by `scrape()`.

| Field | Description |
|---|---|
| `urls_visited` | Successful seed/result fetches and API result entries counted by the path; not a complete request-attempt count |
| `forms_submitted` | Incremented for non-empty HTML result pages processed, not a count of successful POST form submissions |
| `result_pages_parsed` | Non-empty HTML result pages passed to the parser |
| `api_pages_fetched` | API result items returned by the client, not HTTP page requests |
| `docs_scraped` | Path-specific documents processed; HTML items shorter than 50 extracted characters are excluded |
| `docs_accepted` | Documents that passed the quality threshold |
| `docs_discarded` | Documents rejected by the evaluator |
| `docs_written` | Documents successfully persisted |
| `write_errors` | Persistence failures |
| `urls_skipped` | Seeds skipped when an injected burst controller has no token |
| `elapsed_ms` | Total run duration |

`docs_accepted` counts documents that passed evaluation, even if the writer subsequently failed. The plugin does not include the result of `flush()` in the returned statistics.

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
| `ApiOptions` | `pagination_mode="page"`, `page_param="p"`, `results_field="results"`, `max_pages=20`, `headers`, `body_template` (parsed only; not applied by current `ScraperPlugin`) |
| `LlmOptions` | `quality_threshold=0.65`, `model_path`, `temperature=0.1` |
| `GovSourcesOptions` | `bund_enabled`, `bundeslaender_enabled`, `eu_enabled`, `source_ids`, `custom_catalog_path` |

`ScraperConfig` also carries `seed_urls`, `whitelist`, and `blacklist` lists that
feed into `UrlPolicy`. Important implementation limits: `max_depth`, crawl
`max_pages`, `respect_robots`, `same_domain_only`, and `api_options` are parsed
but not consumed by `ScraperPlugin`. `request_delay_ms` is used in the HTML
search path, not API pagination. API endpoint limits come from the
`ApiEndpointConfig` passed to the client (or a catalog source), not directly
from `ScraperConfig::api_options`.

### `ScraperRenderMode`

| Value | Description |
|---|---|
| `STATIC` | Plain HTTP GET + HTML parsing (libcurl, default) |
| `JS_RENDERED` | Headless browser renders the page before extraction |
| `API_JSON` | Seed URL is a JSON REST endpoint |
| `API_GRAPHQL` | Selects the API path; does not by itself configure a GraphQL POST/body in `ScraperPlugin` |

### `UrlPolicy`

Controls which URLs may be scraped:
- Whitelist: empty = allow all HTTP(S) URLs; non-empty = URL must match at least one prefix/limited `*` pattern. The plugin checks explicit seeds and HTML result URLs, but does not check selected catalog seed URLs and exempts the exact API endpoint during API-result filtering.
- Blacklist: URL matching any entry is blocked regardless of the whitelist.
- Schemes `http` and `https` only. These string checks do not provide complete SSRF/egress protection; redirect destinations are not checked again.

### `IScraperApiClient` / `HttpScraperApiClient`

```cpp
std::vector<ApiResult> fetchAll(const ApiEndpointConfig& cfg, const std::string& query);
```

Handles `page`, `cursor`, and `offset` pagination automatically.
When `THEMIS_ENABLE_CURL` is not defined, the built-in `fetchAll()` path returns
an empty vector (unless a custom fetch function is injected).

`ApiEndpointConfig` supports POST body templates with `{{QUERY}}`, `{{PAGE}}`,
and `{{CURSOR}}` substitution. The YAML `ScraperConfig::api_options` block below
is parsed but is not copied into the endpoint built by `ScraperPlugin`; plugin
runs use catalog-source settings or `ApiEndpointConfig` defaults. To use a
custom endpoint/body template directly, configure `ApiEndpointConfig` and call
`HttpScraperApiClient::fetchAll()` (or inject an appropriately configured client).

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

`SubprocessJSRenderer` invokes an external headless browser via subprocess. On
Unix-like systems it uses an argument vector; its Windows implementation uses
`_popen` with a constructed command string. Review the renderer command and
platform-specific argument handling as privileged configuration.
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

# Parsed by ScraperConfig, but currently ignored by ScraperPlugin.
# These values do not configure its API requests; see the direct
# ApiEndpointConfig / HttpScraperApiClient contract above.
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
  model_path: ""             # parsed but currently unused by ScraperPlugin
  temperature: 0.1           # parsed but evaluator uses a fixed 0.1

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

- `initialize()` must be called before `scrape()`; the current implementation returns
  `true` after storing config and attempting the optional catalog load.
  `scrape()` before initialization throws `std::runtime_error`.
- In the HTML fetch path, an injected fetch callback exception is swallowed and
  becomes an empty body; the built-in libcurl path returns its body buffer without
  checking the transfer result or HTTP status. These paths do not log the failure.
  Empty bodies stop or continue the relevant path depending on context; do not
  assume every non-200 response is rejected.
- LLM evaluation failures fall back to the heuristic scorer; the run never aborts
  due to an LLM error.
- `js_timeout_ms` is passed to the renderer command. The subprocess adapter does
  not independently kill a hung process; the renderer must honor its argument.
- `UrlPolicy` rejects non-http/https schemes, but its rejection is not an SSRF
  boundary. Explicit seed URLs rejected during seed collection and blocked HTML
  result URLs are skipped before `urls_visited` increments. Selected catalog seed
  URLs bypass that seed check, and the exact API endpoint is exempted during API
  result filtering.
- without libcurl, the built-in HTTP fetch returns an empty string. Without
  pugixml, HTML form discovery and result-list parsing return empty results;
  direct page text extraction has a fallback tag stripper.
- `ScraperRecordBuilder` provenance fields (`is_scraper_ingested`,
  `ingestion_source_type`, `ingestion_plugin_version`) must not be cleared or
  overridden by the caller — they are the authoritative ingestion trail.
- `max_depth`, crawl `max_pages`, `respect_robots`, and `same_domain_only` are
  not enforced by the current main loop. HTML result traversal uses
  `search_options.max_result_pages`; API pagination uses the configured
  `ApiEndpointConfig::max_pages`. `request_delay_ms` applies in the HTML search
  path; API pagination has no corresponding built-in delay.
- `api_options`, `search_options.results_per_page`, `llm_options.model_path`, and
  `llm_options.temperature` are parsed but unused by the current plugin path.
- HTML search-form POST bodies are constructed, but the current plugin fetches the
  generated search URL through its simple HTTP fetch function; it does not send
  that form body as a POST request.
- `reset()` clears results and counters but leaves the plugin initialized.
  `initialize()` stores config and resets per-run state.

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

For a CMake build that includes `src/scraper` as a subdirectory, link the target and use its propagated include paths,
compile features, optional dependency links, and feature definitions:

```cmake
target_link_libraries(your_target PRIVATE scraper_plugin)
```

The module CMake file discovers yaml-cpp, pugixml, and libcurl and conditionally
enables their feature definitions. It links a discovered nlohmann_json package
or adds the repository's bundled header path when present; the source includes
`nlohmann/json.hpp`, so that header must be available at compile time.
`THEMIS_ENABLE_LLM` enables the LLM path. The module target itself is C++17.
Do not define feature macros without linking the corresponding dependency.

Focused test target and CTest names are generated from `tests/scraper/CMakeLists.txt`:
`module_scraper_<test filename>_focused` and `<test filename>_scraper_FocusedTests`.
The tests are registered with the `scraper` label.

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
