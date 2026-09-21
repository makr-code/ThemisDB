# SCRAPER DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\scraper\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\scraper\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 35
- Compounds: 113
- Classes/Structs: 58
- Namespaces: 12
- File Compounds: 35

## Namespaces
- themis
- themis::bench
- themis::bench::scr
- themis::scraper
- themis::scraper::@032042117164004234060366336375137164074270166213
- themis::scraper::@115162224355176260223235224345047133234232365307
- themis::scraper::@155043246257106357121057243217171104106172104326
- themis::scraper::@203047337007100135033017174174262016100176030203
- themis::scraper::@221132150243246277203241102145006007116116371201
- themis::scraper::@234272204360235230102062145102350223135047017107
- themis::scraper::detail
- themis::scraper::test

## Types
### Classes
- StubFetcher
- StubRateLimiter
- themis::scraper::BurstCrawlController
- themis::scraper::GovSourceCatalog
- themis::scraper::HtmlSearchEngine
- themis::scraper::HttpScraperApiClient
- themis::scraper::IScraperApiClient
- themis::scraper::IScraperDiagnosticSink
- themis::scraper::IScraperJSRenderer
- themis::scraper::IScraperLLMEvaluator
- themis::scraper::IScraperMetadataWriter
- themis::scraper::IScraperPlugin
- themis::scraper::IScraperSearchEngine
- themis::scraper::InMemoryJSRenderer
- themis::scraper::InMemoryLLMEvaluator
- themis::scraper::InMemoryScraperApiClient
- themis::scraper::InMemoryScraperMetadataWriter
- themis::scraper::InMemorySearchEngine
- themis::scraper::ListeningScraperDiagnosticSink
- themis::scraper::NullScraperDiagnosticSink
- themis::scraper::RobotsTxtCache
- themis::scraper::ScraperLLMEvaluator
- themis::scraper::ScraperPlugin
- themis::scraper::ScraperRunSummaryCollector
- themis::scraper::SitemapCrawler
- themis::scraper::SubprocessJSRenderer
- themis::scraper::UrlPolicy

### Structs
- FetchRecord
- themis::scraper::ApiEndpointConfig
- themis::scraper::ApiOptions
- themis::scraper::ApiResult
- themis::scraper::CrawlOptions
- themis::scraper::EvaluationResult
- themis::scraper::GapContext
- themis::scraper::GovDataSource
- themis::scraper::GovSourcesOptions
- themis::scraper::JsRenderContractResult
- themis::scraper::JsRenderRequest
- themis::scraper::JsRenderResult
- themis::scraper::LlmOptions
- themis::scraper::RobotsTxtRules
- themis::scraper::ScrapeRequest
- themis::scraper::ScrapeResult
- themis::scraper::ScrapedDocument
- themis::scraper::ScraperConfig
- themis::scraper::ScraperDiagnosticEvent
- themis::scraper::ScraperGraphEdge
- themis::scraper::ScraperGraphNode
- themis::scraper::ScraperRecordBuilder
- themis::scraper::ScraperRelationalRecord
- themis::scraper::ScraperRunStats
- themis::scraper::ScraperRunSummary
- themis::scraper::ScraperVectorRecord
- themis::scraper::SearchForm
- themis::scraper::SearchOptions
- themis::scraper::SearchResultItem
- themis::scraper::SearchResultPage
- themis::scraper::WriteResult

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 250

### StubFetcher

#### `FetchRecord fetch(const std::string &url) noexcept`
- Source: `tests/scraper/test_scraper_highcardinality_stress.cpp`:50
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a

#### `uint64_t totalOps() const noexcept`
- Source: `tests/scraper/test_scraper_highcardinality_stress.cpp`:54
- Brief: n/a
- Parameters: none

### StubRateLimiter

#### `StubRateLimiter(double rps)`
- Source: `tests/scraper/test_scraper_highcardinality_stress.cpp`:61
- Brief: n/a
- Parameters:
  - `rps` (double): n/a

#### `uint64_t attempts() const noexcept`
- Source: `tests/scraper/test_scraper_highcardinality_stress.cpp`:69
- Brief: n/a
- Parameters: none

#### `bool tryAcquire() noexcept`
- Source: `tests/scraper/test_scraper_highcardinality_stress.cpp`:63
- Brief: n/a
- Parameters: none

### bench_scraper_pipeline_depth.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/scraper/bench_scraper_pipeline_depth.cpp`:174
- Brief: n/a
- Parameters: none

### bench_scraper_release_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/scraper/bench_scraper_release_gates.cpp`:201
- Brief: n/a
- Parameters: none

### test_scraper_highcardinality_stress.cpp

#### `TEST(ScraperHighCardinalityStress, SSTR01_HighCardinalityUrlFetch)`
- Source: `tests/scraper/test_scraper_highcardinality_stress.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperHighCardinalityStress): n/a
  - `<unnamed>` (SSTR01_HighCardinalityUrlFetch): n/a

#### `TEST(ScraperHighCardinalityStress, SSTR02_ConcurrentIngestStress)`
- Source: `tests/scraper/test_scraper_highcardinality_stress.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperHighCardinalityStress): n/a
  - `<unnamed>` (SSTR02_ConcurrentIngestStress): n/a

#### `TEST(ScraperHighCardinalityStress, SSTR03_RateLimitStress)`
- Source: `tests/scraper/test_scraper_highcardinality_stress.cpp`:142
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperHighCardinalityStress): n/a
  - `<unnamed>` (SSTR03_RateLimitStress): n/a

### test_scraper_plugin.cpp

#### `TEST(ScraperPluginFocusedTests, ScraperConfigLoadFromYamlBasic)`
- Source: `tests/scraper/test_scraper_plugin.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPluginFocusedTests): n/a
  - `<unnamed>` (ScraperConfigLoadFromYamlBasic): n/a

#### `TEST(ScraperPluginFocusedTests, UrlPolicyAllowsHttpAndHttps)`
- Source: `tests/scraper/test_scraper_plugin.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPluginFocusedTests): n/a
  - `<unnamed>` (UrlPolicyAllowsHttpAndHttps): n/a

#### `TEST(ScraperPluginFocusedTests, UrlPolicyBlacklistOverridesWhitelist)`
- Source: `tests/scraper/test_scraper_plugin.cpp`:51
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPluginFocusedTests): n/a
  - `<unnamed>` (UrlPolicyBlacklistOverridesWhitelist): n/a

#### `TEST(ScraperPluginFocusedTests, UrlPolicyBlocksNonHttpScheme)`
- Source: `tests/scraper/test_scraper_plugin.cpp`:44
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPluginFocusedTests): n/a
  - `<unnamed>` (UrlPolicyBlocksNonHttpScheme): n/a

#### `TEST(ScraperPluginFocusedTests, UrlPolicyEmptyWhitelistAllowsAll)`
- Source: `tests/scraper/test_scraper_plugin.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPluginFocusedTests): n/a
  - `<unnamed>` (UrlPolicyEmptyWhitelistAllowsAll): n/a

#### `TEST(ScraperPluginFocusedTests, UrlPolicyGlobPrefixWildcard)`
- Source: `tests/scraper/test_scraper_plugin.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPluginFocusedTests): n/a
  - `<unnamed>` (UrlPolicyGlobPrefixWildcard): n/a

#### `TEST(ScraperPluginFocusedTests, UrlPolicyGlobSuffixBlocksPdf)`
- Source: `tests/scraper/test_scraper_plugin.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPluginFocusedTests): n/a
  - `<unnamed>` (UrlPolicyGlobSuffixBlocksPdf): n/a

### test_scraper_v12_features_focused.cpp

#### `TEST(ScraperV12Contract, PluginStartsUninitialized)`
- Source: `tests/scraper/test_scraper_v12_features_focused.cpp`:12
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperV12Contract): n/a
  - `<unnamed>` (PluginStartsUninitialized): n/a

#### `TEST(ScraperV12Contract, ResetKeepsPluginInSafeState)`
- Source: `tests/scraper/test_scraper_v12_features_focused.cpp`:17
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperV12Contract): n/a
  - `<unnamed>` (ResetKeepsPluginInSafeState): n/a

### themis::bench::scr

#### `void BM_PIPE01_BatchEmit1000(benchmark::State &state)`
- Source: `benchmarks/scraper/bench_scraper_pipeline_depth.cpp`:46
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PIPE02_SummaryAggregation10k(benchmark::State &state)`
- Source: `benchmarks/scraper/bench_scraper_pipeline_depth.cpp`:82
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PIPE03_ScrapeRequestBatchAlloc(benchmark::State &state)`
- Source: `benchmarks/scraper/bench_scraper_pipeline_depth.cpp`:117
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PIPE04_FaultClassLoop10k(benchmark::State &state)`
- Source: `benchmarks/scraper/bench_scraper_pipeline_depth.cpp`:139
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SCR01_ErrorEnumCast(benchmark::State &state)`
- Source: `benchmarks/scraper/bench_scraper_release_gates.cpp`:64
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SCR02_SwitchDispatch(benchmark::State &state)`
- Source: `benchmarks/scraper/bench_scraper_release_gates.cpp`:81
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SCR03_StructAlloc(benchmark::State &state)`
- Source: `benchmarks/scraper/bench_scraper_release_gates.cpp`:120
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SCR04_BatchCast(benchmark::State &state)`
- Source: `benchmarks/scraper/bench_scraper_release_gates.cpp`:138
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SCR05_BurstAcquire(benchmark::State &state)`
- Source: `benchmarks/scraper/bench_scraper_release_gates.cpp`:165
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_SCR06_SummaryRead(benchmark::State &state)`
- Source: `benchmarks/scraper/bench_scraper_release_gates.cpp`:183
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Repetitions(kRepetitions) -> ReportAggregatesOnly(true) ->UseRealTime()`
- Source: `benchmarks/scraper/bench_scraper_pipeline_depth.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (kRepetitions): n/a

### themis::scraper

#### `ScraperFaultSeverity defaultSeverityOf(ScraperError e) noexcept`
- Source: `include/scraper/scraper_diagnostics.h`:252
- Brief: Returns the default ScraperFaultSeverity for the given error.
- Parameters:
  - `e` (ScraperError): n/a
- Details: Fatal errors (isScraperFailClosed) map to kFatal. kSuccess maps to kInfo. All others map to kError.

#### `JsRenderContractResult enforceRenderTimeout(const JsRenderResult &raw, int timeout_ms=static_cast< int >(kDefaultRenderTimeout.count())) noexcept`
- Source: `include/scraper/scraper_render_contract.h`:128
- Brief: Maps a raw JsRenderResult onto a JsRenderContractResult with an authoritative ScraperError code.
- Parameters:
  - `raw` (const JsRenderResult &): Raw result returned by IScraperJSRenderer::render().
  - `timeout_ms` (int): Time budget in milliseconds used to classify elapsed time. Defaults to kDefaultRenderTimeout.count() (30 000).
- Return: A JsRenderContractResult with html cleared on any failure.
- Details: Decision table: raw.success == true → error = kSuccess, timed_out = false, html preserved. raw.success == false AND (raw.elapsed_ms >= timeout_ms OR raw.error contains "timeout" case-insensitively): → error = kRenderTimeout, timed_out = true, html cleared. raw.success == false (other failure): → error = kFetchFailed, timed_out = false, html cleared. The default timeout_ms value matches kDefaultRenderTimeout (30 000 ms). raw Raw result returned by IScraperJSRenderer::render(). timeout_ms Time budget in milliseconds used to classify elapsed time. Defaults to kDefaultRenderTimeout.count() (30 000). A JsRenderContractResult with html cleared on any failure. This function is noexcept and safe to call from any pipeline stage.

#### `std::string extractScheme(const std::string &url)`
- Source: `src/scraper/scraper_robots.cpp`:89
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a

#### `ScraperFaultClass faultClassOf(ScraperError e) noexcept`
- Source: `include/scraper/scraper_diagnostics.h`:229
- Brief: Returns the ScraperFaultClass for the given ScraperError.
- Parameters:
  - `e` (ScraperError): n/a
- Details: Used to route diagnostic events to the correct pipeline-stage counter without requiring callers to maintain their own mapping table.

#### `bool isPaginationLimitReached(uint32_t current_page, uint32_t max_depth) noexcept`
- Source: `include/scraper/scraper_render_contract.h`:180
- Brief: Returns true when the crawl has reached or exceeded the configured maximum pagination depth.
- Parameters:
  - `current_page` (uint32_t): Zero- or one-based page index (consistent with caller).
  - `max_depth` (uint32_t): Maximum number of pages allowed (inclusive boundary).
- Return: true if current_page >= max_depth.
- Details: This enforces the contract from scraper_api_contract.h: Crawl hits configured pagination depth limit → kPaginationLimit. The caller is responsible for emitting ScraperError::kPaginationLimit once this returns true. The default depth limit is kDefaultMaxPaginationDepth (50). current_page Zero- or one-based page index (consistent with caller). max_depth Maximum number of pages allowed (inclusive boundary). true if current_page >= max_depth. Pure predicate: no side-effects, always noexcept.

#### `bool isScraperFailClosed(ScraperError e) noexcept`
- Source: `include/scraper/scraper_api_contract.h`:138
- Brief: Returns true when the given error mandates fail-closed denial of write.
- Parameters:
  - `e` (ScraperError): n/a

#### `ScraperDiagnosticEvent makeDiagnosticEvent(ScraperError e, const std::string &url, const std::string &message)`
- Source: `include/scraper/scraper_diagnostics.h`:271
- Brief: Build a minimal well-formed ScraperDiagnosticEvent from an error.
- Parameters:
  - `e` (ScraperError): Error code that triggered the diagnostic.
  - `url` (const std::string &): Source URL (may be empty for catalog-level faults).
  - `message` (const std::string &): Human-readable description. Must be non-empty.
- Return: A fully-populated ScraperDiagnosticEvent ready for emit.
- Details: e Error code that triggered the diagnostic. url Source URL (may be empty for catalog-level faults). message Human-readable description. Must be non-empty. A fully-populated ScraperDiagnosticEvent ready for emit.

### themis::scraper::BurstCrawlController

#### `BurstCrawlController(const BurstCrawlController &)=delete`
- Source: `include/scraper/scraper_burst_controller.h`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BurstCrawlController &): n/a

#### `BurstCrawlController(uint32_t max_tokens, double refill_rate_per_sec) noexcept`
- Source: `include/scraper/scraper_burst_controller.h`:53
- Brief: Construct a fully-charged token bucket.
- Parameters:
  - `max_tokens` (uint32_t): Maximum bucket capacity (must be > 0).
  - `refill_rate_per_sec` (double): Tokens restored per second (must be >= 0).
- Details: max_tokens Maximum bucket capacity (must be > 0). refill_rate_per_sec Tokens restored per second (must be >= 0).

#### `double burstUtilization() const noexcept`
- Source: `include/scraper/scraper_burst_controller.h`:99
- Brief: Current fill level as a fraction of capacity.
- Parameters: none
- Return: Value in [0.0, 1.0]: 0.0 = empty, 1.0 = full.
- Details: Value in [0.0, 1.0]: 0.0 = empty, 1.0 = full.

#### `BurstCrawlController & operator=(const BurstCrawlController &)=delete`
- Source: `include/scraper/scraper_burst_controller.h`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (const BurstCrawlController &): n/a

#### `void refill() noexcept`
- Source: `include/scraper/scraper_burst_controller.h`:133
- Brief: Top-up the bucket for time elapsed since the last refill.
- Parameters: none
- Details: Protected by refill_mu_ to serialise the elapsed-time calculation and last_refill_ update. The actual token store is still atomic so that tryAcquire() CAS loops remain correct even if multiple threads race through this path.

#### `void reset() noexcept`
- Source: `include/scraper/scraper_burst_controller.h`:114
- Brief: Restore the bucket to full capacity.
- Parameters: none
- Details: Typically called between crawl runs to reset rate-limit state.

#### `bool tryAcquire() noexcept`
- Source: `include/scraper/scraper_burst_controller.h`:77
- Brief: Attempt to consume one token from the bucket.
- Parameters: none
- Return: true A token was successfully consumed; the caller may proceed.
- Details: Performs a refill pass for elapsed time, then tries an atomic compare-exchange decrement. Never blocks. true A token was successfully consumed; the caller may proceed. false The bucket is exhausted; the caller should skip/defer.

### themis::scraper::EvaluationResult

#### `bool shouldDiscard() const`
- Source: `include/scraper/scraper_llm_evaluator.h`:38
- Brief: n/a
- Parameters: none

### themis::scraper::GovSourceCatalog

#### `GovSourceCatalog()`
- Source: `include/scraper/gov_source_catalog.h`:119
- Brief: n/a
- Parameters: none

#### `GovSourceCatalog(GovSourceCatalog &&) noexcept=default`
- Source: `include/scraper/gov_source_catalog.h`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (GovSourceCatalog &&): n/a

#### `GovSourceCatalog(const GovSourceCatalog &)=delete`
- Source: `include/scraper/gov_source_catalog.h`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GovSourceCatalog &): n/a

#### `const std::vector< GovDataSource > & all() const`
- Source: `include/scraper/gov_source_catalog.h`:131
- Brief: All sources in the catalog.
- Parameters: none

#### `std::vector< const GovDataSource * > byBundesland(const std::string &iso) const`
- Source: `include/scraper/gov_source_catalog.h`:140
- Brief: All Bundesland sources for a specific state (ISO code).
- Parameters:
  - `iso` (const std::string &): n/a

#### `std::vector< const GovDataSource * > byIds(const std::vector< std::string > &ids) const`
- Source: `include/scraper/gov_source_catalog.h`:146
- Brief: Filter by a list of explicit IDs (preserves order).
- Parameters:
  - `ids` (const std::vector< std::string > &): n/a

#### `std::vector< const GovDataSource * > byType(GovSourceType type) const`
- Source: `include/scraper/gov_source_catalog.h`:137
- Brief: All sources of a given type.
- Parameters:
  - `type` (GovSourceType): n/a

#### `std::vector< const GovDataSource * > enabled() const`
- Source: `include/scraper/gov_source_catalog.h`:143
- Brief: All sources that are currently enabled.
- Parameters: none

#### `const GovDataSource * findById(const std::string &id) const`
- Source: `include/scraper/gov_source_catalog.h`:134
- Brief: Look up by unique id; returns nullptr when not found.
- Parameters:
  - `id` (const std::string &): n/a

#### `void loadFromFile(const std::string &path)`
- Source: `include/scraper/gov_source_catalog.h`:173
- Brief: Load overlay from a file path.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Throws:
  - std::runtime_error: when file cannot be read.
  - std::runtime_error: if an error occurs.
- Details: Load From File. std::runtime_error when file cannot be read. path Input parameter. std::runtime_error if an error occurs. Calls: f(), is_open(), rdbuf(), loadFromYaml(), str().

#### `void loadFromYaml(const std::string &yaml_content)`
- Source: `include/scraper/gov_source_catalog.h`:167
- Brief: Overlay entries from a YAML file on top of the built-in catalog.
- Parameters:
  - `yaml_content` (const std::string &): Input parameter.
- Throws:
  - std::runtime_error: on parse error.
  - std::runtime_error: if an error occurs.
- Details: Load From Yaml. Unknown sources are added; known sources are merged (only non-empty fields from the YAML override the built-in defaults). std::runtime_error on parse error. yaml_content Input parameter. std::runtime_error if an error occurs. Calls: YAML::Load(), IsSequence(), IsMap(), empty(), upsert(), std::move(), std::string(), what().

#### `GovSourceCatalog & operator=(GovSourceCatalog &&) noexcept=default`
- Source: `include/scraper/gov_source_catalog.h`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (GovSourceCatalog &&): n/a

#### `GovSourceCatalog & operator=(const GovSourceCatalog &)=delete`
- Source: `include/scraper/gov_source_catalog.h`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (const GovSourceCatalog &): n/a

#### `void populateBuiltinBund()`
- Source: `include/scraper/gov_source_catalog.h`:176
- Brief: ============================================================================ Built-in catalog – Federal Germany (Bund) ============================================================================
- Parameters: none
- Details: Calls: push_back(), std::move(), add().

#### `void populateBuiltinBundeslaender()`
- Source: `include/scraper/gov_source_catalog.h`:177
- Brief: ============================================================================ Built-in catalog – 16 Bundesländer ============================================================================
- Parameters: none
- Details: Calls: push_back(), std::move(), add().

#### `void populateBuiltinEU()`
- Source: `include/scraper/gov_source_catalog.h`:178
- Brief: ============================================================================ Built-in catalog – European Union ============================================================================
- Parameters: none
- Details: Calls: push_back(), std::move(), add().

#### `bool setEnabled(const std::string &id, bool enabled)`
- Source: `include/scraper/gov_source_catalog.h`:155
- Brief: Enable/disable a source by id.
- Parameters:
  - `id` (const std::string &): Input parameter.
  - `enabled` (bool): n/a
- Return: True when the operation succeeds.
- Details: Set Enabled. id Input parameter. en Input parameter. True when the operation succeeds. Implements setEnabled without additional internal calls.

#### `void upsert(GovDataSource source)`
- Source: `include/scraper/gov_source_catalog.h`:152
- Brief: Add or replace a source.
- Parameters:
  - `source` (GovDataSource): Input parameter.
- Details: Upsert. source Input parameter. Calls: std::move(), push_back().

#### `~GovSourceCatalog()=default`
- Source: `include/scraper/gov_source_catalog.h`:120
- Brief: n/a
- Parameters: none

### themis::scraper::HtmlSearchEngine

#### `HtmlSearchEngine()=default`
- Source: `include/scraper/scraper_search_engine.h`:151
- Brief: n/a
- Parameters: none

#### `std::string buildSearchBody(const SearchForm &form, const std::string &query, int page=1) const override`
- Source: `include/scraper/scraper_search_engine.h`:168
- Brief: Build a POST body for a search form.
- Parameters:
  - `form` (const SearchForm &): n/a
  - `query` (const std::string &): n/a
  - `page` (int): n/a
- Return: URL-encoded form body.
- Details: URL-encoded form body.

#### `std::string buildSearchUrl(const SearchForm &form, const std::string &query, int page=1) const override`
- Source: `include/scraper/scraper_search_engine.h`:163
- Brief: Build the URL (or POST body) to submit a search form.
- Parameters:
  - `form` (const SearchForm &): SearchForm to submit.
  - `query` (const std::string &): The search query string.
  - `page` (int): 1-based page number.
- Return: Fully-qualified URL with query string encoded for GET forms, or base action URL for POST forms (caller must supply body).
- Details: form SearchForm to submit. query The search query string. page 1-based page number. Fully-qualified URL with query string encoded for GET forms, or base action URL for POST forms (caller must supply body).

#### `std::vector< SearchForm > discoverForms(const std::string &html, const std::string &base_url) const override`
- Source: `include/scraper/scraper_search_engine.h`:154
- Brief: Discover all HTML search forms in a page.
- Parameters:
  - `html` (const std::string &): Raw HTML of the page.
  - `base_url` (const std::string &): Absolute URL of the page (used to resolve relative action URLs).
- Return: List of discovered SearchForm objects (may be empty).
- Details: html Raw HTML of the page. base_url Absolute URL of the page (used to resolve relative action URLs). List of discovered SearchForm objects (may be empty).

#### `std::string extractText(const std::string &html_fragment)`
- Source: `include/scraper/scraper_search_engine.h`:188
- Brief: Collect all plain text from an XML subtree.
- Parameters:
  - `html_fragment` (const std::string &): n/a

#### `bool isSearchInput(const std::string &input_type, const std::string &input_name, const std::string &input_id, const std::string &placeholder)`
- Source: `include/scraper/scraper_search_engine.h`:182
- Brief: Detect whether a node looks like the primary search input.
- Parameters:
  - `input_type` (const std::string &): n/a
  - `input_name` (const std::string &): n/a
  - `input_id` (const std::string &): n/a
  - `placeholder` (const std::string &): n/a

#### `SearchResultPage parseResults(const std::string &html, const std::string &base_url, const std::string &selector="") const override`
- Source: `include/scraper/scraper_search_engine.h`:158
- Brief: Parse a search-result page into a structured SearchResultPage.
- Parameters:
  - `html` (const std::string &): Raw HTML of the result page.
  - `base_url` (const std::string &): Absolute URL (for resolving relative links).
  - `selector` (const std::string &): Optional CSS-like selector hint (class or id prefix such as ".result-list" or "#results"); empty = auto.
- Return: Parsed SearchResultPage.
- Details: Heuristic detection order: JSON-LD / microdata structured data Common CSS patterns: ol.results li, ul.results li, div.result, article.result, .search-result, [data-result] Generic list fallback: largest item count For openjur.de the relevant container is .result-list > li. html Raw HTML of the result page. base_url Absolute URL (for resolving relative links). selector Optional CSS-like selector hint (class or id prefix such as ".result-list" or "#results"); empty = auto. Parsed SearchResultPage.

#### `std::string resolveUrl(const std::string &href, const std::string &base_url)`
- Source: `include/scraper/scraper_search_engine.h`:175
- Brief: Resolve a relative URL against base_url.
- Parameters:
  - `href` (const std::string &): n/a
  - `base_url` (const std::string &): n/a

#### `std::string urlEncode(const std::string &s)`
- Source: `include/scraper/scraper_search_engine.h`:179
- Brief: URL-encode a single string component.
- Parameters:
  - `s` (const std::string &): n/a

#### `~HtmlSearchEngine() override=default`
- Source: `include/scraper/scraper_search_engine.h`:152
- Brief: n/a
- Parameters: none

### themis::scraper::HttpScraperApiClient

#### `HttpScraperApiClient(HttpFetchFn fetch_fn={})`
- Source: `include/scraper/scraper_api_client.h`:132
- Brief: n/a
- Parameters:
  - `fetch_fn` (HttpFetchFn): n/a

#### `HttpScraperApiClient(const HttpScraperApiClient &)=delete`
- Source: `include/scraper/scraper_api_client.h`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (const HttpScraperApiClient &): n/a

#### `std::string buildBody(const ApiEndpointConfig &cfg, const std::string &query, int page, const std::string &cursor) const`
- Source: `include/scraper/scraper_api_client.h`:150
- Brief: n/a
- Parameters:
  - `cfg` (const ApiEndpointConfig &): n/a
  - `query` (const std::string &): n/a
  - `page` (int): n/a
  - `cursor` (const std::string &): n/a

#### `std::string buildGetUrl(const ApiEndpointConfig &cfg, const std::string &query, int page, int offset, const std::string &cursor) const`
- Source: `include/scraper/scraper_api_client.h`:145
- Brief: n/a
- Parameters:
  - `cfg` (const ApiEndpointConfig &): n/a
  - `query` (const std::string &): n/a
  - `page` (int): n/a
  - `offset` (int): n/a
  - `cursor` (const std::string &): n/a

#### `std::string curlFetch(const std::string &url, const std::string &method, const std::map< std::string, std::string > &headers, const std::string &body)`
- Source: `include/scraper/scraper_api_client.h`:165
- Brief: Default libcurl-backed HTTP fetch implementation.
- Parameters:
  - `url` (const std::string &): n/a
  - `method` (const std::string &): n/a
  - `headers` (const std::map< std::string, std::string > &): n/a
  - `body` (const std::string &): n/a

#### `std::vector< ApiResult > fetchAll(const ApiEndpointConfig &cfg, const std::string &query) override`
- Source: `include/scraper/scraper_api_client.h`:138
- Brief: Fetch All.
- Parameters:
  - `cfg` (const ApiEndpointConfig &): Input parameter.
  - `query` (const std::string &): Input parameter.
- Return: Return value.
- Details: cfg Input parameter. query Input parameter. Return value. Calls: buildGetUrl(), buildBody(), fetch_fn_(), empty(), parseResultsArray(), insert(), end(), begin().

#### `std::string flattenJson(const std::string &json_text)`
- Source: `include/scraper/scraper_api_client.h`:162
- Brief: Flatten a JSON value to plain text recursively.
- Parameters:
  - `json_text` (const std::string &): n/a

#### `HttpScraperApiClient & operator=(const HttpScraperApiClient &)=delete`
- Source: `include/scraper/scraper_api_client.h`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (const HttpScraperApiClient &): n/a

#### `std::vector< ApiResult > parseResultsArray(const std::string &json_text, const std::string &results_field, const std::string &source_url)`
- Source: `include/scraper/scraper_api_client.h`:156
- Brief: Parse a JSON array field into ApiResult objects.
- Parameters:
  - `json_text` (const std::string &): n/a
  - `results_field` (const std::string &): n/a
  - `source_url` (const std::string &): n/a

#### `~HttpScraperApiClient() override=default`
- Source: `include/scraper/scraper_api_client.h`:133
- Brief: n/a
- Parameters: none

### themis::scraper::IScraperApiClient

#### `std::vector< ApiResult > fetchAll(const ApiEndpointConfig &cfg, const std::string &query)=0`
- Source: `include/scraper/scraper_api_client.h`:115
- Brief: Fetch all results from an API endpoint.
- Parameters:
  - `cfg` (const ApiEndpointConfig &): Endpoint configuration.
  - `query` (const std::string &): Search query (replaces {{QUERY}} in body_template or appended as search_param to GET URLs).
- Return: All collected results across all pages.
- Details: cfg Endpoint configuration. query Search query (replaces {{QUERY}} in body_template or appended as search_param to GET URLs). All collected results across all pages.

#### `~IScraperApiClient()=default`
- Source: `include/scraper/scraper_api_client.h`:106
- Brief: n/a
- Parameters: none

### themis::scraper::IScraperDiagnosticSink

#### `void emit(const ScraperDiagnosticEvent &event) noexcept=0`
- Source: `include/scraper/scraper_diagnostics.h`:140
- Brief: Emit a diagnostic event to the sink.
- Parameters:
  - `event` (const ScraperDiagnosticEvent &): The structured event. event.message must be non-empty.
- Details: event The structured event. event.message must be non-empty. Implementations MUST NOT throw. Any internal error during emit should be swallowed and counted (not propagated to the scraper pipeline).

#### `~IScraperDiagnosticSink()=default`
- Source: `include/scraper/scraper_diagnostics.h`:130
- Brief: n/a
- Parameters: none

### themis::scraper::IScraperJSRenderer

#### `bool isAvailable() const =0`
- Source: `include/scraper/scraper_js_renderer.h`:85
- Brief: Returns true when the renderer backend is available and usable.
- Parameters: none
- Details: For SubprocessJSRenderer this means the renderer command is non-empty and its first token resolves to an executable on PATH. Call before ScraperPlugin::initialize() to validate JS rendering mode.

#### `JsRenderResult render(const JsRenderRequest &req)=0`
- Source: `include/scraper/scraper_js_renderer.h`:76
- Brief: Render a URL using a headless browser and return the resulting HTML.
- Parameters:
  - `req` (const JsRenderRequest &): Render request with URL, timeout, and optional CSS wait selector.
- Return: JsRenderResult — success==false with an error message on timeout or subprocess failure. Never throws.
- Details: req Render request with URL, timeout, and optional CSS wait selector. JsRenderResult — success==false with an error message on timeout or subprocess failure. Never throws.

#### `~IScraperJSRenderer()=default`
- Source: `include/scraper/scraper_js_renderer.h`:68
- Brief: n/a
- Parameters: none

### themis::scraper::IScraperLLMEvaluator

#### `EvaluationResult evaluate(const std::string &text, const std::string &url, const GapContext &gap, double threshold) const =0`
- Source: `include/scraper/scraper_llm_evaluator.h`:67
- Brief: Evaluate a document.
- Parameters:
  - `text` (const std::string &): Plain text extracted from the scraped page.
  - `url` (const std::string &): Source URL (for logging / metadata).
  - `gap` (const GapContext &): Gap context to evaluate against.
  - `threshold` (double): Minimum quality_score to pass; set below_threshold when score falls short.
- Return: EvaluationResult.
- Details: text Plain text extracted from the scraped page. url Source URL (for logging / metadata). gap Gap context to evaluate against. threshold Minimum quality_score to pass; set below_threshold when score falls short. EvaluationResult.

#### `~IScraperLLMEvaluator()=default`
- Source: `include/scraper/scraper_llm_evaluator.h`:56
- Brief: n/a
- Parameters: none

### themis::scraper::IScraperMetadataWriter

#### `bool flush()=0`
- Source: `include/scraper/scraper_metadata_writer.h`:157
- Brief: Flush any buffered writes to durable storage.
- Parameters: none
- Return: true when all buffered writes were persisted successfully.
- Details: true when all buffered writes were persisted successfully.

#### `WriteResult write(const ScraperRelationalRecord &rel, const ScraperGraphNode &node, const std::vector< ScraperGraphEdge > &edges, const ScraperVectorRecord &vec)=0`
- Source: `include/scraper/scraper_metadata_writer.h`:147
- Brief: Persist one scraped document to all storage layers.
- Parameters:
  - `rel` (const ScraperRelationalRecord &): Relational record with full document metadata.
  - `node` (const ScraperGraphNode &): Property-graph node derived from the document.
  - `edges` (const std::vector< ScraperGraphEdge > &): Property-graph edges (e.g. FILLS_GAP, PUBLISHED_BY).
  - `vec` (const ScraperVectorRecord &): Vector record for ANN indexing.
- Return: WriteResult indicating per-layer success and the assigned doc_id.
- Details: Implementations must set provenance fields before writing and must not modify the rel.is_scraper_ingested, rel.ingestion_source_type, or rel.ingestion_plugin_version fields. rel Relational record with full document metadata. node Property-graph node derived from the document. edges Property-graph edges (e.g. FILLS_GAP, PUBLISHED_BY). vec Vector record for ANN indexing. WriteResult indicating per-layer success and the assigned doc_id.

#### `~IScraperMetadataWriter()=default`
- Source: `include/scraper/scraper_metadata_writer.h`:132
- Brief: n/a
- Parameters: none

### themis::scraper::IScraperPlugin

#### `const std::vector< ScrapedDocument > & getResults() const =0`
- Source: `include/scraper/scraper_plugin.h`:136
- Brief: Returns all documents collected in the most recent scrape() call.
- Parameters: none
- Details: Includes both accepted (discarded=false) and discarded (discarded=true) documents. The vector is cleared by reset().

#### `bool initialize(const ScraperConfig &config)=0`
- Source: `include/scraper/scraper_plugin.h`:115
- Brief: Initialise the plugin from a configuration.
- Parameters:
  - `config` (const ScraperConfig &): Complete scraper configuration (seed URLs, crawl options, …).
- Return: true on success; false when the config is structurally invalid.
- Throws:
  - std::runtime_error: When config.loadFromFile/Yaml parsing fails.
- Details: config Complete scraper configuration (seed URLs, crawl options, …). true on success; false when the config is structurally invalid. std::runtime_error When config.loadFromFile/Yaml parsing fails.

#### `bool isInitialized() const =0`
- Source: `include/scraper/scraper_plugin.h`:149
- Brief: Returns true after a successful call to initialize().
- Parameters: none

#### `void reset()=0`
- Source: `include/scraper/scraper_plugin.h`:144
- Brief: Reset state so the plugin can be re-initialised for a new run.
- Parameters: none
- Details: Clears results, resets statistics, and sets initialized=false. Injected dependencies (evaluator, writer, …) are preserved.

#### `ScraperRunStats scrape()=0`
- Source: `include/scraper/scraper_plugin.h`:128
- Brief: Execute the agentic scraper loop.
- Parameters: none
- Return: Run statistics (URLs visited, documents accepted/discarded, …).
- Throws:
  - std::logic_error: When called before a successful initialize().
- Details: Blocks until all seeds and search forms have been processed or the max_pages limit is reached. Error isolation guarantees that per-URL failures do not abort the run. initialize() must have been called and returned true. Run statistics (URLs visited, documents accepted/discarded, …). std::logic_error When called before a successful initialize().

#### `~IScraperPlugin()=default`
- Source: `include/scraper/scraper_plugin.h`:107
- Brief: n/a
- Parameters: none

### themis::scraper::IScraperSearchEngine

#### `std::string buildSearchBody(const SearchForm &form, const std::string &query, int page=1) const =0`
- Source: `include/scraper/scraper_search_engine.h`:132
- Brief: Build a POST body for a search form.
- Parameters:
  - `form` (const SearchForm &): n/a
  - `query` (const std::string &): n/a
  - `page` (int): n/a
- Return: URL-encoded form body.
- Details: URL-encoded form body.

#### `std::string buildSearchUrl(const SearchForm &form, const std::string &query, int page=1) const =0`
- Source: `include/scraper/scraper_search_engine.h`:123
- Brief: Build the URL (or POST body) to submit a search form.
- Parameters:
  - `form` (const SearchForm &): SearchForm to submit.
  - `query` (const std::string &): The search query string.
  - `page` (int): 1-based page number.
- Return: Fully-qualified URL with query string encoded for GET forms, or base action URL for POST forms (caller must supply body).
- Details: form SearchForm to submit. query The search query string. page 1-based page number. Fully-qualified URL with query string encoded for GET forms, or base action URL for POST forms (caller must supply body).

#### `std::vector< SearchForm > discoverForms(const std::string &html, const std::string &base_url) const =0`
- Source: `include/scraper/scraper_search_engine.h`:89
- Brief: Discover all HTML search forms in a page.
- Parameters:
  - `html` (const std::string &): Raw HTML of the page.
  - `base_url` (const std::string &): Absolute URL of the page (used to resolve relative action URLs).
- Return: List of discovered SearchForm objects (may be empty).
- Details: html Raw HTML of the page. base_url Absolute URL of the page (used to resolve relative action URLs). List of discovered SearchForm objects (may be empty).

#### `SearchResultPage parseResults(const std::string &html, const std::string &base_url, const std::string &selector="") const =0`
- Source: `include/scraper/scraper_search_engine.h`:110
- Brief: Parse a search-result page into a structured SearchResultPage.
- Parameters:
  - `html` (const std::string &): Raw HTML of the result page.
  - `base_url` (const std::string &): Absolute URL (for resolving relative links).
  - `selector` (const std::string &): Optional CSS-like selector hint (class or id prefix such as ".result-list" or "#results"); empty = auto.
- Return: Parsed SearchResultPage.
- Details: Heuristic detection order: JSON-LD / microdata structured data Common CSS patterns: ol.results li, ul.results li, div.result, article.result, .search-result, [data-result] Generic list fallback: largest item count For openjur.de the relevant container is .result-list > li. html Raw HTML of the result page. base_url Absolute URL (for resolving relative links). selector Optional CSS-like selector hint (class or id prefix such as ".result-list" or "#results"); empty = auto. Parsed SearchResultPage.

#### `~IScraperSearchEngine()=default`
- Source: `include/scraper/scraper_search_engine.h`:81
- Brief: n/a
- Parameters: none

### themis::scraper::InMemoryJSRenderer

#### `InMemoryJSRenderer()=default`
- Source: `include/scraper/scraper_js_renderer.h`:139
- Brief: n/a
- Parameters: none

#### `int callCount() const`
- Source: `include/scraper/scraper_js_renderer.h`:148
- Brief: n/a
- Parameters: none

#### `void clearInjections()`
- Source: `include/scraper/scraper_js_renderer.h`:144
- Brief: n/a
- Parameters: none

#### `void injectResult(JsRenderResult result)`
- Source: `include/scraper/scraper_js_renderer.h`:141
- Brief: n/a
- Parameters:
  - `result` (JsRenderResult): n/a

#### `bool isAvailable() const override`
- Source: `include/scraper/scraper_js_renderer.h`:149
- Brief: Returns true when the renderer backend is available and usable.
- Parameters: none
- Details: For SubprocessJSRenderer this means the renderer command is non-empty and its first token resolves to an executable on PATH. Call before ScraperPlugin::initialize() to validate JS rendering mode.

#### `JsRenderResult render(const JsRenderRequest &) override`
- Source: `include/scraper/scraper_js_renderer.h`:151
- Brief: Render a URL using a headless browser and return the resulting HTML.
- Parameters:
  - `req` (const JsRenderRequest &): Render request with URL, timeout, and optional CSS wait selector.
- Return: JsRenderResult — success==false with an error message on timeout or subprocess failure. Never throws.
- Details: req Render request with URL, timeout, and optional CSS wait selector. JsRenderResult — success==false with an error message on timeout or subprocess failure. Never throws.

### themis::scraper::InMemoryLLMEvaluator

#### `int callCount() const`
- Source: `include/scraper/scraper_llm_evaluator.h`:131
- Brief: n/a
- Parameters: none

#### `EvaluationResult evaluate(const std::string &, const std::string &url, const GapContext &, double threshold) const override`
- Source: `include/scraper/scraper_llm_evaluator.h`:133
- Brief: Evaluate a document.
- Parameters:
  - `text` (const std::string &): Plain text extracted from the scraped page.
  - `url` (const std::string &): Source URL (for logging / metadata).
  - `gap` (const GapContext &): Gap context to evaluate against.
  - `threshold` (double): Minimum quality_score to pass; set below_threshold when score falls short.
- Return: EvaluationResult.
- Details: text Plain text extracted from the scraped page. url Source URL (for logging / metadata). gap Gap context to evaluate against. threshold Minimum quality_score to pass; set below_threshold when score falls short. EvaluationResult.

#### `void injectResult(const std::string &url_substring, EvaluationResult r)`
- Source: `include/scraper/scraper_llm_evaluator.h`:128
- Brief: n/a
- Parameters:
  - `url_substring` (const std::string &): n/a
  - `r` (EvaluationResult): n/a

#### `void setDefaultResult(EvaluationResult r)`
- Source: `include/scraper/scraper_llm_evaluator.h`:127
- Brief: n/a
- Parameters:
  - `r` (EvaluationResult): n/a

### themis::scraper::InMemoryScraperApiClient

#### `InMemoryScraperApiClient()=default`
- Source: `include/scraper/scraper_api_client.h`:184
- Brief: n/a
- Parameters: none

#### `int callCount() const`
- Source: `include/scraper/scraper_api_client.h`:189
- Brief: n/a
- Parameters: none

#### `std::vector< ApiResult > fetchAll(const ApiEndpointConfig &, const std::string &query) override`
- Source: `include/scraper/scraper_api_client.h`:192
- Brief: Fetch all results from an API endpoint.
- Parameters:
  - `cfg` (const ApiEndpointConfig &): Endpoint configuration.
  - `query` (const std::string &): Search query (replaces {{QUERY}} in body_template or appended as search_param to GET URLs).
- Return: All collected results across all pages.
- Details: cfg Endpoint configuration. query Search query (replaces {{QUERY}} in body_template or appended as search_param to GET URLs). All collected results across all pages.

#### `void injectResults(std::vector< ApiResult > results)`
- Source: `include/scraper/scraper_api_client.h`:186
- Brief: n/a
- Parameters:
  - `results` (std::vector< ApiResult >): n/a

#### `const std::string & lastQuery() const`
- Source: `include/scraper/scraper_api_client.h`:190
- Brief: n/a
- Parameters: none

### themis::scraper::InMemoryScraperMetadataWriter

#### `InMemoryScraperMetadataWriter()=default`
- Source: `include/scraper/scraper_metadata_writer.h`:169
- Brief: n/a
- Parameters: none

#### `void clear()`
- Source: `include/scraper/scraper_metadata_writer.h`:187
- Brief: n/a
- Parameters: none

#### `bool flush() override`
- Source: `include/scraper/scraper_metadata_writer.h`:177
- Brief: Flush any buffered writes to durable storage.
- Parameters: none
- Return: true when all buffered writes were persisted successfully.
- Details: true when all buffered writes were persisted successfully.

#### `int flushCount() const`
- Source: `include/scraper/scraper_metadata_writer.h`:186
- Brief: n/a
- Parameters: none

#### `const std::vector< ScraperGraphEdge > & graphEdges() const`
- Source: `include/scraper/scraper_metadata_writer.h`:184
- Brief: n/a
- Parameters: none

#### `const std::vector< ScraperGraphNode > & graphNodes() const`
- Source: `include/scraper/scraper_metadata_writer.h`:183
- Brief: n/a
- Parameters: none

#### `const std::vector< ScraperRelationalRecord > & relationalRecords() const`
- Source: `include/scraper/scraper_metadata_writer.h`:180
- Brief: n/a
- Parameters: none

#### `const std::vector< ScraperVectorRecord > & vectorRecords() const`
- Source: `include/scraper/scraper_metadata_writer.h`:185
- Brief: n/a
- Parameters: none

#### `WriteResult write(const ScraperRelationalRecord &rel, const ScraperGraphNode &node, const std::vector< ScraperGraphEdge > &edges, const ScraperVectorRecord &vec) override`
- Source: `include/scraper/scraper_metadata_writer.h`:171
- Brief: Write.
- Parameters:
  - `rel` (const ScraperRelationalRecord &): Input parameter.
  - `node` (const ScraperGraphNode &): Input parameter.
  - `edges` (const std::vector< ScraperGraphEdge > &): Input parameter.
  - `vec` (const ScraperVectorRecord &): Input parameter.
- Return: Return value.
- Details: rel Input parameter. node Input parameter. edges Input parameter. vec Input parameter. Return value. Calls: push_back(), insert(), end(), begin().

### themis::scraper::InMemorySearchEngine

#### `InMemorySearchEngine()=default`
- Source: `include/scraper/scraper_search_engine.h`:200
- Brief: n/a
- Parameters: none

#### `std::string buildSearchBody(const SearchForm &form, const std::string &query, int page=1) const override`
- Source: `include/scraper/scraper_search_engine.h`:241
- Brief: Build a POST body for a search form.
- Parameters:
  - `form` (const SearchForm &): n/a
  - `query` (const std::string &): n/a
  - `page` (int): n/a
- Return: URL-encoded form body.
- Details: URL-encoded form body.

#### `std::string buildSearchUrl(const SearchForm &form, const std::string &query, int page=1) const override`
- Source: `include/scraper/scraper_search_engine.h`:233
- Brief: Build the URL (or POST body) to submit a search form.
- Parameters:
  - `form` (const SearchForm &): SearchForm to submit.
  - `query` (const std::string &): The search query string.
  - `page` (int): 1-based page number.
- Return: Fully-qualified URL with query string encoded for GET forms, or base action URL for POST forms (caller must supply body).
- Details: form SearchForm to submit. query The search query string. page 1-based page number. Fully-qualified URL with query string encoded for GET forms, or base action URL for POST forms (caller must supply body).

#### `int callCount() const`
- Source: `include/scraper/scraper_search_engine.h`:213
- Brief: n/a
- Parameters: none

#### `void clearInjections()`
- Source: `include/scraper/scraper_search_engine.h`:208
- Brief: n/a
- Parameters: none

#### `std::vector< SearchForm > discoverForms(const std::string &, const std::string &) const override`
- Source: `include/scraper/scraper_search_engine.h`:215
- Brief: Discover all HTML search forms in a page.
- Parameters:
  - `html` (const std::string &): Raw HTML of the page.
  - `base_url` (const std::string &): Absolute URL of the page (used to resolve relative action URLs).
- Return: List of discovered SearchForm objects (may be empty).
- Details: html Raw HTML of the page. base_url Absolute URL of the page (used to resolve relative action URLs). List of discovered SearchForm objects (may be empty).

#### `void injectForms(std::vector< SearchForm > forms)`
- Source: `include/scraper/scraper_search_engine.h`:202
- Brief: n/a
- Parameters:
  - `forms` (std::vector< SearchForm >): n/a

#### `void injectResultPage(SearchResultPage page)`
- Source: `include/scraper/scraper_search_engine.h`:205
- Brief: n/a
- Parameters:
  - `page` (SearchResultPage): n/a

#### `SearchResultPage parseResults(const std::string &, const std::string &, const std::string &="") const override`
- Source: `include/scraper/scraper_search_engine.h`:222
- Brief: Parse a search-result page into a structured SearchResultPage.
- Parameters:
  - `html` (const std::string &): Raw HTML of the result page.
  - `base_url` (const std::string &): Absolute URL (for resolving relative links).
  - `selector` (const std::string &): Optional CSS-like selector hint (class or id prefix such as ".result-list" or "#results"); empty = auto.
- Return: Parsed SearchResultPage.
- Details: Heuristic detection order: JSON-LD / microdata structured data Common CSS patterns: ol.results li, ul.results li, div.result, article.result, .search-result, [data-result] Generic list fallback: largest item count For openjur.de the relevant container is .result-list > li. html Raw HTML of the result page. base_url Absolute URL (for resolving relative links). selector Optional CSS-like selector hint (class or id prefix such as ".result-list" or "#results"); empty = auto. Parsed SearchResultPage.

### themis::scraper::ListeningScraperDiagnosticSink

#### `void addListener(Listener fn)`
- Source: `include/scraper/scraper_diagnostics.h`:175
- Brief: Register a listener called for each emitted event.
- Parameters:
  - `fn` (Listener): n/a
- Details: Listeners are called outside the internal mutex, so they may safely call back into the sink (e.g., snapshot(), size()). Keep them short to minimise emit latency.

#### `void clear()`
- Source: `include/scraper/scraper_diagnostics.h`:208
- Brief: Clear all recorded events (registered listeners are preserved).
- Parameters: none

#### `void emit(const ScraperDiagnosticEvent &event) noexcept override`
- Source: `include/scraper/scraper_diagnostics.h`:180
- Brief: Emit a diagnostic event to the sink.
- Parameters:
  - `event` (const ScraperDiagnosticEvent &): The structured event. event.message must be non-empty.
- Details: event The structured event. event.message must be non-empty. Implementations MUST NOT throw. Any internal error during emit should be swallowed and counted (not propagated to the scraper pipeline).

#### `std::size_t size() const`
- Source: `include/scraper/scraper_diagnostics.h`:202
- Brief: Number of events recorded.
- Parameters: none

#### `std::vector< ScraperDiagnosticEvent > snapshot() const`
- Source: `include/scraper/scraper_diagnostics.h`:196
- Brief: Return a snapshot of all recorded events (thread-safe copy).
- Parameters: none

### themis::scraper::NullScraperDiagnosticSink

#### `void emit(const ScraperDiagnosticEvent &) noexcept override`
- Source: `include/scraper/scraper_diagnostics.h`:152
- Brief: Emit a diagnostic event to the sink.
- Parameters:
  - `event` (const ScraperDiagnosticEvent &): The structured event. event.message must be non-empty.
- Details: event The structured event. event.message must be non-empty. Implementations MUST NOT throw. Any internal error during emit should be swallowed and counted (not propagated to the scraper pipeline).

### themis::scraper::RobotsTxtCache

#### `RobotsTxtCache()=default`
- Source: `include/scraper/scraper_robots.h`:91
- Brief: n/a
- Parameters: none

#### `RobotsTxtCache(FetchFn fetch_fn)`
- Source: `include/scraper/scraper_robots.h`:97
- Brief: Construct with a custom fetch function.
- Parameters:
  - `fetch_fn` (FetchFn): HTTP GET implementation (default: libcurl via ScraperPlugin).
- Details: fetch_fn HTTP GET implementation (default: libcurl via ScraperPlugin).

#### `void clear()`
- Source: `include/scraper/scraper_robots.h`:126
- Brief: Clear all cached entries (e.g. between test cases).
- Parameters: none
- Details: Clear. Calls: lk().

#### `std::string extractDomain(const std::string &url)`
- Source: `include/scraper/scraper_robots.h`:140
- Brief: Extract the bare hostname from a full URL.
- Parameters:
  - `url` (const std::string &): n/a

#### `std::string extractPath(const std::string &url)`
- Source: `include/scraper/scraper_robots.h`:143
- Brief: Extract the path component from a full URL.
- Parameters:
  - `url` (const std::string &): n/a

#### `void fetchAndCache(const std::string &domain, const std::string &scheme, const std::string &user_agent)`
- Source: `include/scraper/scraper_robots.h`:146
- Brief: Fetch and parse the robots.txt for domain; store in cache_.
- Parameters:
  - `domain` (const std::string &): Input parameter.
  - `scheme` (const std::string &): Input parameter.
  - `user_agent` (const std::string &): Input parameter.
- Details: Fetch And Cache. domain Input parameter. scheme Input parameter. user_agent Input parameter. Calls: fetch_fn_(), empty(), parse().

#### `void injectRobots(const std::string &domain, const std::string &content)`
- Source: `include/scraper/scraper_robots.h`:108
- Brief: Test helper: pre-populate the cache for a domain.
- Parameters:
  - `domain` (const std::string &): Input parameter.
  - `content` (const std::string &): Input parameter.
- Details: Inject Robots. domain Bare hostname (e.g. "example.com"). content Raw robots.txt content to parse and cache. After this call, isAllowed() for any URL on domain will use the injected rules without making any HTTP calls. domain Input parameter. content Input parameter. Calls: lk(), parse().

#### `bool isAllowed(const std::string &url, const std::string &user_agent)`
- Source: `include/scraper/scraper_robots.h`:120
- Brief: Check whether a URL is allowed to be fetched.
- Parameters:
  - `url` (const std::string &): Input parameter.
  - `user_agent` (const std::string &): Input parameter.
- Return: true The path is allowed by the wildcard rules (or no rules apply).
- Details: Is Allowed. Fetches and caches the domain's robots.txt on first call for that domain. url Full URL to check (e.g. "https://example.com/path"). user_agent User-agent string sent in the robots.txt fetch. true The path is allowed by the wildcard rules (or no rules apply). false A Disallow rule matches and no Allow rule overrides it. url Input parameter. user_agent Input parameter. True when the operation succeeds. Calls: extractDomain(), extractPath(), extractScheme(), lk(), find(), end(), fetchAndCache(), at().

#### `RobotsTxtRules parse(const std::string &content)`
- Source: `include/scraper/scraper_robots.h`:136
- Brief: Parse raw robots.txt content into a RobotsTxtRules struct.
- Parameters:
  - `content` (const std::string &): Raw robots.txt text.
- Return: Wildcard-agent rules extracted from content.
- Details: Static utility exposed for unit testing the parser in isolation. content Raw robots.txt text. Wildcard-agent rules extracted from content.

### themis::scraper::ScraperConfig

#### `std::vector< std::string > effectiveSearchQueries() const`
- Source: `include/scraper/scraper_config.h`:166
- Brief: Returns the effective search queries for this run.
- Parameters: none
- Details: If search_options.queries is non-empty, those are returned as-is. Otherwise the gap_context keywords are used.

#### `ScraperConfig loadFromFile(const std::string &path)`
- Source: `include/scraper/scraper_config.h`:152
- Brief: Load config from a YAML file on disk.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: when the file cannot be read or is invalid.
  - std::runtime_error: if an error occurs.
- Details: Load From File. std::runtime_error when the file cannot be read or is invalid. path Input parameter. Return value. std::runtime_error if an error occurs. Calls: YAML::LoadFile(), parseNode(), what(), else().

#### `ScraperConfig loadFromYaml(const std::string &yaml_content)`
- Source: `include/scraper/scraper_config.h`:158
- Brief: Parse config from an in-memory YAML string.
- Parameters:
  - `yaml_content` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: when the YAML is invalid.
  - std::runtime_error: if an error occurs.
- Details: Load From Yaml. std::runtime_error when the YAML is invalid. yaml_content Input parameter. Return value. std::runtime_error if an error occurs. Calls: YAML::Load(), parseNode(), std::string(), what(), else().

### themis::scraper::ScraperLLMEvaluator

#### `ScraperLLMEvaluator()=default`
- Source: `include/scraper/scraper_llm_evaluator.h`:88
- Brief: n/a
- Parameters: none

#### `ScraperLLMEvaluator(const ScraperLLMEvaluator &)=delete`
- Source: `include/scraper/scraper_llm_evaluator.h`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScraperLLMEvaluator &): n/a

#### `std::string buildPrompt(const std::string &text, const GapContext &gap)`
- Source: `include/scraper/scraper_llm_evaluator.h`:105
- Brief: Build the LLM prompt for quality/relevance evaluation.
- Parameters:
  - `text` (const std::string &): n/a
  - `gap` (const GapContext &): n/a

#### `EvaluationResult evaluate(const std::string &text, const std::string &url, const GapContext &gap, double threshold) const override`
- Source: `include/scraper/scraper_llm_evaluator.h`:94
- Brief: Evaluate a document.
- Parameters:
  - `text` (const std::string &): Plain text extracted from the scraped page.
  - `url` (const std::string &): Source URL (for logging / metadata).
  - `gap` (const GapContext &): Gap context to evaluate against.
  - `threshold` (double): Minimum quality_score to pass; set below_threshold when score falls short.
- Return: EvaluationResult.
- Details: text Plain text extracted from the scraped page. url Source URL (for logging / metadata). gap Gap context to evaluate against. threshold Minimum quality_score to pass; set below_threshold when score falls short. EvaluationResult.

#### `EvaluationResult heuristicScore(const std::string &text, const GapContext &gap, double threshold)`
- Source: `include/scraper/scraper_llm_evaluator.h`:113
- Brief: Heuristic fallback when LLM is unavailable.
- Parameters:
  - `text` (const std::string &): n/a
  - `gap` (const GapContext &): n/a
  - `threshold` (double): n/a

#### `bool isLlmAvailable() const`
- Source: `include/scraper/scraper_llm_evaluator.h`:101
- Brief: Returns true when the LLM backend is reachable.
- Parameters: none

#### `ScraperLLMEvaluator & operator=(const ScraperLLMEvaluator &)=delete`
- Source: `include/scraper/scraper_llm_evaluator.h`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScraperLLMEvaluator &): n/a

#### `EvaluationResult parseLlmResponse(const std::string &response, double threshold)`
- Source: `include/scraper/scraper_llm_evaluator.h`:109
- Brief: Parse the LLM JSON response into an EvaluationResult.
- Parameters:
  - `response` (const std::string &): n/a
  - `threshold` (double): n/a

#### `~ScraperLLMEvaluator() override=default`
- Source: `include/scraper/scraper_llm_evaluator.h`:89
- Brief: n/a
- Parameters: none

### themis::scraper::ScraperPlugin

#### `ScraperPlugin()`
- Source: `include/scraper/scraper_plugin.h`:173
- Brief: n/a
- Parameters: none

#### `ScraperPlugin(const ScraperPlugin &)=delete`
- Source: `include/scraper/scraper_plugin.h`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScraperPlugin &): n/a

#### `ScraperPlugin(std::shared_ptr< IScraperLLMEvaluator > evaluator, std::shared_ptr< IScraperMetadataWriter > writer, std::shared_ptr< IScraperSearchEngine > search_engine, std::shared_ptr< IScraperJSRenderer > js_renderer, std::shared_ptr< IScraperApiClient > api_client)`
- Source: `include/scraper/scraper_plugin.h`:174
- Brief: n/a
- Parameters:
  - `evaluator` (std::shared_ptr< IScraperLLMEvaluator >): n/a
  - `writer` (std::shared_ptr< IScraperMetadataWriter >): n/a
  - `search_engine` (std::shared_ptr< IScraperSearchEngine >): n/a
  - `js_renderer` (std::shared_ptr< IScraperJSRenderer >): n/a
  - `api_client` (std::shared_ptr< IScraperApiClient >): n/a

#### `std::vector< std::pair< std::string, std::string > > collectSeeds() const`
- Source: `include/scraper/scraper_plugin.h`:231
- Brief: Collect all effective seed URLs (config seeds + gov catalog sources).
- Parameters: none

#### `std::string extractText(const std::string &html)`
- Source: `include/scraper/scraper_plugin.h`:238
- Brief: Extract plain text from HTML.
- Parameters:
  - `html` (const std::string &): n/a

#### `std::string fetchPage(const std::string &url) const`
- Source: `include/scraper/scraper_plugin.h`:235
- Brief: Fetch page HTML using the configured render mode.
- Parameters:
  - `url` (const std::string &): n/a

#### `const std::vector< ScrapedDocument > & getResults() const override`
- Source: `include/scraper/scraper_plugin.h`:188
- Brief: Returns all documents collected in the most recent scrape() call.
- Parameters: none
- Details: Includes both accepted (discarded=false) and discarded (discarded=true) documents. The vector is cleared by reset().

#### `ApiEndpointConfig govSourceToApiConfig(const GovDataSource &src)`
- Source: `include/scraper/scraper_plugin.h`:261
- Brief: Build an ApiEndpointConfig from a GovDataSource.
- Parameters:
  - `src` (const GovDataSource &): n/a

#### `bool initialize(const ScraperConfig &config) override`
- Source: `include/scraper/scraper_plugin.h`:186
- Brief: Initialize.
- Parameters:
  - `config` (const ScraperConfig &): Input parameter.
- Return: True when the operation succeeds.
- Details: config Input parameter. True when the operation succeeds. Calls: lk(), clear(), empty(), get(), loadFromFile().

#### `bool isInitialized() const override`
- Source: `include/scraper/scraper_plugin.h`:190
- Brief: Returns true after a successful call to initialize().
- Parameters: none

#### `ScraperPlugin & operator=(const ScraperPlugin &)=delete`
- Source: `include/scraper/scraper_plugin.h`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ScraperPlugin &): n/a

#### `void processDocument(const std::string &url, const std::string &html, const std::string &source_name, const std::string &gov_source_id, const std::string &document_type, const std::string &date_issued, const std::string &title_hint="")`
- Source: `include/scraper/scraper_plugin.h`:241
- Brief: Evaluate + conditionally store one document.
- Parameters:
  - `url` (const std::string &): Input parameter.
  - `html` (const std::string &): Input parameter.
  - `source_name` (const std::string &): Name of the source.
  - `gov_source_id` (const std::string &): Identifier of the gov source.
  - `document_type` (const std::string &): Input parameter.
  - `date_issued` (const std::string &): Input parameter.
  - `title_hint` (const std::string &): Input parameter.
- Details: Process Document. url Input parameter. html Input parameter. source_name Name of the source. gov_source_id Identifier of the gov source. document_type Input parameter. date_issued Input parameter. title_hint Input parameter. Calls: extractText(), size(), evaluate(), empty(), ss(), std::getline(), find_first_not_of(), substr().

#### `void reset() override`
- Source: `include/scraper/scraper_plugin.h`:189
- Brief: Reset the modification detection flag.
- Parameters: none
- Details: Calls: lk(), clear().

#### `void runApiLoop(const std::string &endpoint_url, const std::string &source_name, const std::string &gov_source_id)`
- Source: `include/scraper/scraper_plugin.h`:256
- Brief: Run the API crawl loop for one endpoint.
- Parameters:
  - `endpoint_url` (const std::string &): Input parameter.
  - `source_name` (const std::string &): Name of the source.
  - `gov_source_id` (const std::string &): Identifier of the gov source.
- Details: Run Api Loop. endpoint_url Input parameter. source_name Name of the source. gov_source_id Identifier of the gov source. Calls: findById(), govSourceToApiConfig(), effectiveSearchQueries(), policy(), fetchAll(), size(), isAllowed(), evaluate().

#### `void runSearchLoop(const std::string &seed_url, const std::string &page_html, const std::string &source_name, const std::string &gov_source_id)`
- Source: `include/scraper/scraper_plugin.h`:250
- Brief: Run the search-form agentic loop for one seed URL.
- Parameters:
  - `seed_url` (const std::string &): Input parameter.
  - `page_html` (const std::string &): Input parameter.
  - `source_name` (const std::string &): Name of the source.
  - `gov_source_id` (const std::string &): Identifier of the gov source.
- Details: Run Search Loop. seed_url Input parameter. page_html Input parameter. source_name Name of the source. gov_source_id Identifier of the gov source. Calls: discoverForms(), empty(), front(), effectiveSearchQueries(), policy(), buildSearchUrl(), isAllowed(), std::this_thread::sleep_for().

#### `ScraperRunStats scrape() override`
- Source: `include/scraper/scraper_plugin.h`:187
- Brief: Scrape.
- Parameters: none
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: Return value. std::runtime_error if an error occurs. Calls: lk(), std::chrono::steady_clock::now(), collectSeeds(), policy(), tryAcquire(), empty(), findById(), find().

#### `void setApiClient(std::shared_ptr< IScraperApiClient > c)`
- Source: `include/scraper/scraper_plugin.h`:202
- Brief: Replace the REST/GraphQL API client (default: HttpScraperApiClient).
- Parameters:
  - `c` (std::shared_ptr< IScraperApiClient >): Input parameter.
- Details: Set Api Client. c Input parameter. Calls: std::move().

#### `void setBurstController(std::shared_ptr< BurstCrawlController > bc)`
- Source: `include/scraper/scraper_plugin.h`:208
- Brief: Inject a burst-rate controller; null disables rate limiting (default).
- Parameters:
  - `bc` (std::shared_ptr< BurstCrawlController >): Input parameter.
- Details: Set Burst Controller. bc Input parameter. Calls: std::move().

#### `void setEvaluator(std::shared_ptr< IScraperLLMEvaluator > e)`
- Source: `include/scraper/scraper_plugin.h`:194
- Brief: Replace the LLM quality evaluator (default: ScraperLLMEvaluator with heuristic fallback).
- Parameters:
  - `e` (std::shared_ptr< IScraperLLMEvaluator >): Input parameter.
- Details: Set Evaluator. e Input parameter. Calls: std::move().

#### `void setHttpFetch(HttpFn fn)`
- Source: `include/scraper/scraper_plugin.h`:206
- Brief: Set Http Fetch.
- Parameters:
  - `fn` (HttpFn): Input parameter.
- Details: fn Input parameter. Calls: std::move().

#### `void setJsRenderer(std::shared_ptr< IScraperJSRenderer > r)`
- Source: `include/scraper/scraper_plugin.h`:200
- Brief: Replace the headless JS renderer (default: nullptr — JS_RENDERED mode disabled).
- Parameters:
  - `r` (std::shared_ptr< IScraperJSRenderer >): Input parameter.
- Details: Set Js Renderer. r Input parameter. Calls: std::move().

#### `void setSearchEngine(std::shared_ptr< IScraperSearchEngine > se)`
- Source: `include/scraper/scraper_plugin.h`:198
- Brief: Replace the HTML search-form engine (default: HtmlSearchEngine).
- Parameters:
  - `se` (std::shared_ptr< IScraperSearchEngine >): n/a
- Details: Set Search Engine. s Input parameter. Calls: std::move().

#### `void setWriter(std::shared_ptr< IScraperMetadataWriter > w)`
- Source: `include/scraper/scraper_plugin.h`:196
- Brief: Replace the metadata persistence layer (default: InMemoryScraperMetadataWriter).
- Parameters:
  - `w` (std::shared_ptr< IScraperMetadataWriter >): Input parameter.
- Details: Set Writer. w Input parameter. Calls: std::move().

#### `~ScraperPlugin() override=default`
- Source: `include/scraper/scraper_plugin.h`:181
- Brief: n/a
- Parameters: none

### themis::scraper::ScraperRecordBuilder

#### `std::vector< ScraperGraphEdge > buildEdges(const ScraperRelationalRecord &rel, const EvaluationResult &eval)`
- Source: `include/scraper/scraper_metadata_writer.h`:228
- Brief: n/a
- Parameters:
  - `rel` (const ScraperRelationalRecord &): n/a
  - `eval` (const EvaluationResult &): n/a

#### `ScraperGraphNode buildNode(const ScraperRelationalRecord &rel)`
- Source: `include/scraper/scraper_metadata_writer.h`:226
- Brief: n/a
- Parameters:
  - `rel` (const ScraperRelationalRecord &): n/a

#### `ScraperRelationalRecord buildRelational(const std::string &url, const std::string &title, const std::string &text, const std::string &source_name, const std::string &gov_source_id, const EvaluationResult &eval, const GapContext &gap, const std::string &plugin_version="")`
- Source: `include/scraper/scraper_metadata_writer.h`:216
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `title` (const std::string &): n/a
  - `text` (const std::string &): n/a
  - `source_name` (const std::string &): n/a
  - `gov_source_id` (const std::string &): n/a
  - `eval` (const EvaluationResult &): n/a
  - `gap` (const GapContext &): n/a
  - `plugin_version` (const std::string &): n/a

#### `ScraperVectorRecord buildVector(const ScraperRelationalRecord &rel)`
- Source: `include/scraper/scraper_metadata_writer.h`:232
- Brief: n/a
- Parameters:
  - `rel` (const ScraperRelationalRecord &): n/a

#### `std::string computeDocId(const std::string &url, const std::string &text)`
- Source: `include/scraper/scraper_metadata_writer.h`:235
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `text` (const std::string &): n/a

#### `std::string currentIso8601()`
- Source: `include/scraper/scraper_metadata_writer.h`:237
- Brief: n/a
- Parameters: none

### themis::scraper::ScraperRunSummary

#### `bool isHealthy() const noexcept`
- Source: `include/scraper/scraper_run_summary.h`:86
- Brief: Returns true when at least one URL succeeded and no evaluation failures occurred.
- Parameters: none
- Details: An evaluation failure (kEvaluatorPath) is fail-closed: content must not be written when quality gating fails, so any such failure marks the run as unhealthy for operator review.

#### `std::string toLogLine() const`
- Source: `include/scraper/scraper_run_summary.h`:100
- Brief: Returns a single-line operator log string.
- Parameters: none
- Details: Format: scrape-run:succeeded=Nfailed_fetch=Nfailed_parse=Nfailed_eval=N failed_write=Nskipped_burst=Npages_written=Nduration_ms=N (all on one line)

### themis::scraper::ScraperRunSummaryCollector

#### `void attach(ListeningScraperDiagnosticSink &sink)`
- Source: `include/scraper/scraper_run_summary.h`:143
- Brief: Attach this collector as a listener to the given sink.
- Parameters:
  - `sink` (ListeningScraperDiagnosticSink &): The sink whose events should be counted.
- Details: Registers a lambda that increments the appropriate counter for each emitted ScraperDiagnosticEvent. The lambda holds a raw pointer to *this; the collector must outlive the sink (or be reset/detached before destruction). sink The sink whose events should be counted.

#### `void recordSuccess(uint32_t pages=1) noexcept`
- Source: `include/scraper/scraper_run_summary.h`:182
- Brief: Record a successful URL completion.
- Parameters:
  - `pages` (uint32_t): Number of metadata records written for this URL (default 1).
- Details: Call once per URL that was successfully written. pages Number of metadata records written for this URL (default 1).

#### `void reset() noexcept`
- Source: `include/scraper/scraper_run_summary.h`:222
- Brief: Reset all counters to zero.
- Parameters: none

#### `void setRunStats(uint32_t total_urls, uint64_t run_duration_ms) noexcept`
- Source: `include/scraper/scraper_run_summary.h`:194
- Brief: Set aggregate run-level statistics after the run completes.
- Parameters:
  - `total_urls` (uint32_t): Total URLs that were attempted.
  - `run_duration_ms` (uint64_t): Wall-clock run duration in milliseconds.
- Details: total_urls Total URLs that were attempted. run_duration_ms Wall-clock run duration in milliseconds.

#### `ScraperRunSummary summary() const noexcept`
- Source: `include/scraper/scraper_run_summary.h`:206
- Brief: Returns a snapshot of the accumulated counters.
- Parameters: none
- Details: The returned ScraperRunSummary is a value copy taken under the internal mutex; it will not change after return.

### themis::scraper::SitemapCrawler

#### `SitemapCrawler()=default`
- Source: `include/scraper/scraper_sitemap.h`:66
- Brief: n/a
- Parameters: none

#### `SitemapCrawler(FetchFn fetch_fn, std::size_t max_urls=5000, std::string user_agent="ThemisDB-Scraper/1.2")`
- Source: `include/scraper/scraper_sitemap.h`:74
- Brief: Construct with a custom fetch function.
- Parameters:
  - `fetch_fn` (FetchFn): HTTP GET implementation.
  - `max_urls` (std::size_t): Maximum URLs returned per sitemap (default: 5 000).
  - `user_agent` (std::string): User-agent string for HTTP GET requests.
- Details: fetch_fn HTTP GET implementation. max_urls Maximum URLs returned per sitemap (default: 5 000). user_agent User-agent string for HTTP GET requests.

#### `std::vector< std::string > fetchUrls(const std::string &sitemap_url) const`
- Source: `include/scraper/scraper_sitemap.h`:95
- Brief: Fetch and extract all URLs from a sitemap.
- Parameters:
  - `sitemap_url` (const std::string &): URL of the sitemap (or sitemap index) XML file.
- Return: De-duplicated list of <loc> URLs, capped at max_urls_. Empty on fetch/parse failure.
- Details: Handles both plain sitemaps and sitemap index files. For index files each child sitemap is fetched once and its <loc> entries merged. Errors on individual child sitemaps are silently skipped. sitemap_url URL of the sitemap (or sitemap index) XML file. De-duplicated list of <loc> URLs, capped at max_urls_. Empty on fetch/parse failure.

#### `bool isSitemapIndex(const std::string &xml_content)`
- Source: `include/scraper/scraper_sitemap.h`:114
- Brief: Returns true when xml_content is a sitemap index file.
- Parameters:
  - `xml_content` (const std::string &): n/a
- Details: Detected by the presence of a <sitemapindex root element.

#### `std::vector< std::string > parseLocEntries(const std::string &xml_content)`
- Source: `include/scraper/scraper_sitemap.h`:106
- Brief: Parse <loc> entries from raw sitemap XML.
- Parameters:
  - `xml_content` (const std::string &): Raw sitemap XML.
- Return: All <loc> values found in xml_content.
- Details: Extract all <loc>text</loc> values from xml_content. Static utility exposed for unit testing the parser in isolation. xml_content Raw sitemap XML. All <loc> values found in xml_content.

#### `void setFetchFn(FetchFn fn)`
- Source: `include/scraper/scraper_sitemap.h`:82
- Brief: Set the HTTP fetch function.
- Parameters:
  - `fn` (FetchFn): Function matching FetchFn; called for each sitemap URL.
- Details: fn Function matching FetchFn; called for each sitemap URL.

### themis::scraper::SubprocessJSRenderer

#### `SubprocessJSRenderer(const SubprocessJSRenderer &)=delete`
- Source: `include/scraper/scraper_js_renderer.h`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SubprocessJSRenderer &): n/a

#### `SubprocessJSRenderer(std::string renderer_cmd)`
- Source: `include/scraper/scraper_js_renderer.h`:111
- Brief: n/a
- Parameters:
  - `renderer_cmd` (std::string): n/a

#### `std::string buildCommand(const JsRenderRequest &req) const`
- Source: `include/scraper/scraper_js_renderer.h`:127
- Brief: Build the shell command string from the request.
- Parameters:
  - `req` (const JsRenderRequest &): n/a

#### `bool isAvailable() const override`
- Source: `include/scraper/scraper_js_renderer.h`:121
- Brief: n/a
- Parameters: none
- Details: Returns true when renderer_cmd is non-empty and the first token of the command resolves to an executable on PATH.

#### `SubprocessJSRenderer & operator=(const SubprocessJSRenderer &)=delete`
- Source: `include/scraper/scraper_js_renderer.h`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (const SubprocessJSRenderer &): n/a

#### `JsRenderResult render(const JsRenderRequest &req) override`
- Source: `include/scraper/scraper_js_renderer.h`:117
- Brief: Render.
- Parameters:
  - `req` (const JsRenderRequest &): Input parameter.
- Return: Return value.
- Details: req Input parameter. Return value. Calls: isAvailable(), std::chrono::steady_clock::now(), defined(), ss(), push_back(), std::to_string(), empty(), reserve().

#### `~SubprocessJSRenderer() override=default`
- Source: `include/scraper/scraper_js_renderer.h`:112
- Brief: n/a
- Parameters: none

### themis::scraper::UrlPolicy

#### `UrlPolicy(const ScraperConfig &config)`
- Source: `include/scraper/scraper_config.h`:188
- Brief: n/a
- Parameters:
  - `config` (const ScraperConfig &): n/a

#### `UrlPolicy(const std::vector< std::string > &whitelist, const std::vector< std::string > &blacklist)`
- Source: `include/scraper/scraper_config.h`:189
- Brief: n/a
- Parameters:
  - `whitelist` (const std::vector< std::string > &): n/a
  - `blacklist` (const std::vector< std::string > &): n/a

#### `bool isAllowed(const std::string &url) const`
- Source: `include/scraper/scraper_config.h`:192
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a

#### `bool matchesPattern(const std::string &url, const std::string &pattern)`
- Source: `include/scraper/scraper_config.h`:195
- Brief: n/a
- Parameters:
  - `url` (const std::string &): n/a
  - `pattern` (const std::string &): n/a

### themis::scraper::detail

#### `bool containsIgnoreCase(const std::string &haystack, const std::string &needle) noexcept`
- Source: `include/scraper/scraper_render_contract.h`:81
- Brief: Case-insensitive substring search.
- Parameters:
  - `haystack` (const std::string &): n/a
  - `needle` (const std::string &): n/a

### themis::scraper::test

#### `TEST(ScraperBurstHardening, SCR17_MaxTokensConsumable)`
- Source: `tests/scraper/test_scraper_burst_hardening_focused.cpp`:25
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperBurstHardening): n/a
  - `<unnamed>` (SCR17_MaxTokensConsumable): n/a

#### `TEST(ScraperBurstHardening, SCR18_ExhaustionReturnsFalse)`
- Source: `tests/scraper/test_scraper_burst_hardening_focused.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperBurstHardening): n/a
  - `<unnamed>` (SCR18_ExhaustionReturnsFalse): n/a

#### `TEST(ScraperBurstHardening, SCR19_UtilizationAfterTwoAcquires)`
- Source: `tests/scraper/test_scraper_burst_hardening_focused.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperBurstHardening): n/a
  - `<unnamed>` (SCR19_UtilizationAfterTwoAcquires): n/a

#### `TEST(ScraperBurstHardening, SCR20_DepletionAndReset)`
- Source: `tests/scraper/test_scraper_burst_hardening_focused.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperBurstHardening): n/a
  - `<unnamed>` (SCR20_DepletionAndReset): n/a

#### `TEST(ScraperContractHardening, SCR01_ErrorCodeUniqueness)`
- Source: `tests/scraper/test_scraper_contract_hardening_focused.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperContractHardening): n/a
  - `<unnamed>` (SCR01_ErrorCodeUniqueness): n/a

#### `TEST(ScraperContractHardening, SCR02_ErrorCodeRange)`
- Source: `tests/scraper/test_scraper_contract_hardening_focused.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperContractHardening): n/a
  - `<unnamed>` (SCR02_ErrorCodeRange): n/a

#### `TEST(ScraperContractHardening, SCR03_SwitchDispatch)`
- Source: `tests/scraper/test_scraper_contract_hardening_focused.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperContractHardening): n/a
  - `<unnamed>` (SCR03_SwitchDispatch): n/a

#### `TEST(ScraperContractHardening, SCR04_ScrapeRequestDefaults)`
- Source: `tests/scraper/test_scraper_contract_hardening_focused.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperContractHardening): n/a
  - `<unnamed>` (SCR04_ScrapeRequestDefaults): n/a

#### `TEST(ScraperContractHardening, SCR05_ScrapeResultDefaults)`
- Source: `tests/scraper/test_scraper_contract_hardening_focused.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperContractHardening): n/a
  - `<unnamed>` (SCR05_ScrapeResultDefaults): n/a

#### `TEST(ScraperContractHardening, SCR06_ScrapeRequestCopy)`
- Source: `tests/scraper/test_scraper_contract_hardening_focused.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperContractHardening): n/a
  - `<unnamed>` (SCR06_ScrapeRequestCopy): n/a

#### `TEST(ScraperContractHardening, SCR07_ScrapeResultMove)`
- Source: `tests/scraper/test_scraper_contract_hardening_focused.cpp`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperContractHardening): n/a
  - `<unnamed>` (SCR07_ScrapeResultMove): n/a

#### `TEST(ScraperContractHardening, SCR08_FailClosedPredicate)`
- Source: `tests/scraper/test_scraper_contract_hardening_focused.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperContractHardening): n/a
  - `<unnamed>` (SCR08_FailClosedPredicate): n/a

#### `TEST(ScraperPhase23Hardening, SCR09_FaultClassMapping)`
- Source: `tests/scraper/test_scraper_phase2_phase3_hardening_focused.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPhase23Hardening): n/a
  - `<unnamed>` (SCR09_FaultClassMapping): n/a

#### `TEST(ScraperPhase23Hardening, SCR10_DefaultSeverityMapping)`
- Source: `tests/scraper/test_scraper_phase2_phase3_hardening_focused.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPhase23Hardening): n/a
  - `<unnamed>` (SCR10_DefaultSeverityMapping): n/a

#### `TEST(ScraperPhase23Hardening, SCR11_MakeDiagnosticEventFields)`
- Source: `tests/scraper/test_scraper_phase2_phase3_hardening_focused.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPhase23Hardening): n/a
  - `<unnamed>` (SCR11_MakeDiagnosticEventFields): n/a

#### `TEST(ScraperPhase23Hardening, SCR12_ListeningSinkRecordsAndBroadcasts)`
- Source: `tests/scraper/test_scraper_phase2_phase3_hardening_focused.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPhase23Hardening): n/a
  - `<unnamed>` (SCR12_ListeningSinkRecordsAndBroadcasts): n/a

#### `TEST(ScraperPhase23Hardening, SCR13_ListeningSinkConcurrentEmit)`
- Source: `tests/scraper/test_scraper_phase2_phase3_hardening_focused.cpp`:144
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPhase23Hardening): n/a
  - `<unnamed>` (SCR13_ListeningSinkConcurrentEmit): n/a

#### `TEST(ScraperPhase23Hardening, SCR14_NullSinkAbsorbs)`
- Source: `tests/scraper/test_scraper_phase2_phase3_hardening_focused.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPhase23Hardening): n/a
  - `<unnamed>` (SCR14_NullSinkAbsorbs): n/a

#### `TEST(ScraperPhase23Hardening, SCR15_MultipleListenersBroadcast)`
- Source: `tests/scraper/test_scraper_phase2_phase3_hardening_focused.cpp`:194
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPhase23Hardening): n/a
  - `<unnamed>` (SCR15_MultipleListenersBroadcast): n/a

#### `TEST(ScraperPhase23Hardening, SCR16_EventTimestampInitialised)`
- Source: `tests/scraper/test_scraper_phase2_phase3_hardening_focused.cpp`:226
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperPhase23Hardening): n/a
  - `<unnamed>` (SCR16_EventTimestampInitialised): n/a

#### `TEST(ScraperRenderPagination, SCR21_JsRenderTimeoutDetection)`
- Source: `tests/scraper/test_scraper_render_pagination_focused.cpp`:34
- Brief: A result whose elapsed time exceeds the budget AND whose error string contains "timeout" must produce kRenderTimeout with html cleared.
- Parameters:
  - `<unnamed>` (ScraperRenderPagination): n/a
  - `<unnamed>` (SCR21_JsRenderTimeoutDetection): n/a

#### `TEST(ScraperRenderPagination, SCR22_JsRenderNonTimeoutFailure)`
- Source: `tests/scraper/test_scraper_render_pagination_focused.cpp`:59
- Brief: A failed render that is not a timeout (elapsed well below budget, no "timeout" keyword in error) must map to kFetchFailed.
- Parameters:
  - `<unnamed>` (ScraperRenderPagination): n/a
  - `<unnamed>` (SCR22_JsRenderNonTimeoutFailure): n/a

#### `TEST(ScraperRenderPagination, SCR23_PaginationLimitBoundary)`
- Source: `tests/scraper/test_scraper_render_pagination_focused.cpp`:84
- Brief: isPaginationLimitReached must return true when current_page equals max_depth (inclusive), and false strictly below.
- Parameters:
  - `<unnamed>` (ScraperRenderPagination): n/a
  - `<unnamed>` (SCR23_PaginationLimitBoundary): n/a

#### `TEST(ScraperRenderPagination, SCR24_SuccessfulRenderPassThrough)`
- Source: `tests/scraper/test_scraper_render_pagination_focused.cpp`:106
- Brief: A successful render result must be returned with kSuccess, no timeout flag, and the original html content intact.
- Parameters:
  - `<unnamed>` (ScraperRenderPagination): n/a
  - `<unnamed>` (SCR24_SuccessfulRenderPassThrough): n/a

#### `TEST(ScraperRunSummary, SCR25_EmptySessionIsUnhealthy)`
- Source: `tests/scraper/test_scraper_run_summary_focused.cpp`:33
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperRunSummary): n/a
  - `<unnamed>` (SCR25_EmptySessionIsUnhealthy): n/a

#### `TEST(ScraperRunSummary, SCR26_CounterAccuracyMixedEvents)`
- Source: `tests/scraper/test_scraper_run_summary_focused.cpp`:44
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperRunSummary): n/a
  - `<unnamed>` (SCR26_CounterAccuracyMixedEvents): n/a

#### `TEST(ScraperRunSummary, SCR27_ToLogLineNonEmptyContainsSucceeded)`
- Source: `tests/scraper/test_scraper_run_summary_focused.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperRunSummary): n/a
  - `<unnamed>` (SCR27_ToLogLineNonEmptyContainsSucceeded): n/a

#### `TEST(ScraperRunSummary, SCR28_IsHealthyLogic)`
- Source: `tests/scraper/test_scraper_run_summary_focused.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (ScraperRunSummary): n/a
  - `<unnamed>` (SCR28_IsHealthyLogic): n/a

