# SEARCH DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\search\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\search\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 69
- Compounds: 192
- Classes/Structs: 91
- Namespaces: 24
- File Compounds: 69

## Namespaces
- @000163036045331030135104221113306377122350064260
- @024306217135200301016025052207234344211353220334
- @060241377132155115146071330213344016353201007111
- @152071335267021160302374304121140305373235162207
- @274335230211266320167307200057155122015254374315
- @300320352250064045220100377105176226113021245162
- @301130260036330354070015175041333171341027011074
- testing
- themis
- themis::@232340204116061053346164335207053257367245116137
- themis::core
- themis::core::concerns
- themis::graph
- themis::index
- themis::llm
- themis::search
- themis::search::@053140342072134231156275124310033035311175237016
- themis::search::@200044150074115130121036030055041211010163140245
- themis::search::@220130331046231040257244135106162114016120146213
- themis::search::@231343072072152161115021156021171006225154270066
- themis::search::@251176075124374124367243053125162230077006355062
- themis::search::testing
- themis::search::testing::@013213072011044375104107202312275362216266215247
- themis::tensor

## Types
### Classes
- StressStubDocumentIndexer
- StressStubFacetFilter
- StressStubRanker
- themis::AutocompleteEngine
- themis::ConversationalSearch
- themis::CrossLingualSearch
- themis::DistributedHybridSearch
- themis::FacetedSearch
- themis::FederatedSearch
- themis::FuzzyMatcher
- themis::HybridSearch
- themis::LearningToRank
- themis::LlmQueryRewriter
- themis::LlmReranker
- themis::MultiFieldBoostedSearch
- themis::MultiModalSearch
- themis::NegativeKeywordFilter
- themis::NeuralSparseRetrieval
- themis::PersonalizedRanker
- themis::QueryExpander
- themis::SearchAnalytics
- themis::SearchHighlighter
- themis::SearchResultStream
- themis::search::LayeredRetrievalOrchestrator
- themis::search::testing::AnalyticsEdgeCasesTest
- themis::search::testing::DistributedSearchEdgeCasesTest
- themis::search::testing::SearchIntegrationPhase4Test
- themis::search::testing::UtilityComponentEdgeCasesTest

### Structs
- StressStubFacetFilter::FilterResult
- StressStubRanker::RankResult
- themis::AutocompleteEngine::Config
- themis::ClickEvent
- themis::ConversationalSearch::Config
- themis::ConversationalSearch::Turn
- themis::CrossLingualSearch::Config
- themis::CrossLingualSearch::EmbeddingQuery
- themis::CrossLingualSearch::LanguageHint
- themis::CrossLingualSearch::Result
- themis::DistributedHybridSearch::Config
- themis::DistributedHybridSearch::SearchStats
- themis::DistributedHybridSearch::ShardSearchResult
- themis::ExpandedQuery
- themis::FacetResult
- themis::FacetedSearch::ActiveFacet
- themis::FacetedSearch::RangeBucket
- themis::FederatedSearch::Config
- themis::FederatedSearch::Result
- themis::FederatedSearch::TenantStats
- themis::FuzzyMatch
- themis::FuzzyMatcher::Config
- themis::HybridSearch::Config
- themis::HybridSearch::Result
- themis::HybridSearch::SearchStats
- themis::LearningToRank::Config
- themis::LearningToRank::Variant
- themis::LlmQueryRewriter::Config
- themis::LlmRerankCandidate
- themis::LlmRerankResult
- themis::LlmReranker::Config
- themis::ModalQuery
- themis::MultiFieldBoostedSearch::Config
- themis::MultiFieldBoostedSearch::FieldConfig
- themis::MultiFieldBoostedSearch::Result
- themis::MultiModalResult
- themis::MultiModalSearch::Config
- themis::NegativeKeywordFilter::Config
- themis::NegativeKeywordFilter::ParsedQuery
- themis::NeuralSparseRetrieval::Config
- themis::NeuralSparseRetrieval::Result
- themis::PersonalizedRanker::Config
- themis::QueryExpander::Config
- themis::RankedResult
- themis::RankingFeatures
- themis::RewrittenQuery
- themis::SearchAnalytics::Config
- themis::SearchEvent
- themis::SearchHighlighter::Config
- themis::SearchMetrics
- themis::SearchResultStream::Config
- themis::SpellingCorrection
- themis::Suggestion
- themis::UserInteraction
- themis::search::AnnLayerCandidate
- themis::search::LayerDecisionRecord
- themis::search::LayeredRetrievalConfig
- themis::search::LayeredRetrievalContext
- themis::search::LayeredRetrievalResult
- themis::search::PerQueryRetrievalGuardrails
- themis::search::ProvenanceEntry
- themis::search::TensorLayerCandidate
- themis::search::testing::SearchIntegrationPhase4Test::PipelineResult

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 455

### StressStubDocumentIndexer

#### `uint64_t count() const`
- Source: `tests/search/test_search_highcardinality_stress.cpp`:49
- Brief: n/a
- Parameters: none

#### `bool index(uint64_t doc_id, const std::string &)`
- Source: `tests/search/test_search_highcardinality_stress.cpp`:45
- Brief: n/a
- Parameters:
  - `doc_id` (uint64_t): n/a
  - `<unnamed>` (const std::string &): n/a

### StressStubFacetFilter

#### `FilterResult filter(const std::string &facet_key, const std::string &facet_value, uint32_t candidate_count)`
- Source: `tests/search/test_search_highcardinality_stress.cpp`:76
- Brief: n/a
- Parameters:
  - `facet_key` (const std::string &): n/a
  - `facet_value` (const std::string &): n/a
  - `candidate_count` (uint32_t): n/a

#### `uint64_t total() const`
- Source: `tests/search/test_search_highcardinality_stress.cpp`:84
- Brief: n/a
- Parameters: none

### StressStubRanker

#### `RankResult rank(uint64_t doc_id, const std::string &term)`
- Source: `tests/search/test_search_highcardinality_stress.cpp`:60
- Brief: n/a
- Parameters:
  - `doc_id` (uint64_t): n/a
  - `term` (const std::string &): n/a

#### `uint64_t total() const`
- Source: `tests/search/test_search_highcardinality_stress.cpp`:65
- Brief: n/a
- Parameters: none

### bench_approximate_radius_search.cpp

#### `Arg(0) -> Arg(1) ->Arg(2) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:437
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a

#### `Arg(1) -> Arg(10) ->Arg(50) ->Arg(100) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `Arg(10) -> Arg(30) ->Arg(50) ->Arg(70) ->Arg(100) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:221
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `Arg(30) -> Arg(50) ->Arg(70) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:348
- Brief: n/a
- Parameters:
  - `<unnamed>` (30): n/a

#### `Arg(5) -> Arg(10) ->Arg(20) ->Arg(50) ->Arg(100) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:313
- Brief: n/a
- Parameters:
  - `<unnamed>` (5): n/a

#### `Args({1000}) -> Args({10000}) ->Args({100000}) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` ({1000}): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:442
- Brief: n/a
- Parameters: none

#### `void BM_RadiusSearch_BatchSearch(benchmark::State &state)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:231
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RadiusSearch_DatasetSize(benchmark::State &state)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:140
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RadiusSearch_EstimateCount(benchmark::State &state)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:323
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RadiusSearch_Metrics(benchmark::State &state)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:389
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RadiusSearch_RadiusVariation(benchmark::State &state)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:187
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RadiusSearch_SearchById(benchmark::State &state)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:356
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RadiusSearch_TargetCount(benchmark::State &state)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:280
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Unit(benchmark::kMillisecond)`
- Source: `benchmarks/search/bench_approximate_radius_search.cpp`:384
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

### bench_search_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:133
- Brief: n/a
- Parameters: none

#### `void BM_Search_ConcurrentThroughput(benchmark::State &state)`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:117
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Search_FTSQueryP95(benchmark::State &state)`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:63
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Search_FacetFilterP95(benchmark::State &state)`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:98
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Search_RankingP95(benchmark::State &state)`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:80
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Name("BM_Search_ConcurrentThroughput") -> Threads(8) ->Iterations(1000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` ("BM_Search_ConcurrentThroughput"): n/a

#### `Name("BM_Search_FTSQueryP95") -> Iterations(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` ("BM_Search_FTSQueryP95"): n/a

#### `Name("BM_Search_FacetFilterP95") -> Iterations(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` ("BM_Search_FacetFilterP95"): n/a

#### `Name("BM_Search_RankingP95") -> Iterations(10000) ->Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` ("BM_Search_RankingP95"): n/a

#### `uint32_t stub_facet_filter(const std::string &key, const std::string &value, uint32_t candidate_count)`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:51
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a
  - `candidate_count` (uint32_t): n/a

#### `uint32_t stub_fts_search(const std::string &term, uint32_t limit)`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:41
- Brief: n/a
- Parameters:
  - `term` (const std::string &): n/a
  - `limit` (uint32_t): n/a

#### `float stub_rank(uint32_t hit_count, const std::string &term)`
- Source: `benchmarks/search/bench_search_dedicated_gates.cpp`:46
- Brief: n/a
- Parameters:
  - `hit_count` (uint32_t): n/a
  - `term` (const std::string &): n/a

### benchmark_distributed_hybrid_search.cpp

#### `Arg(0) -> Arg(10) ->Arg(25) ->Arg(50) ->Arg(75) ->Arg(100)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (0): n/a

#### `Arg(10) -> Arg(50) ->Arg(100) ->Arg(500) ->Arg(1000)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (10): n/a

#### `Arg(2) -> Arg(4) ->Arg(8) ->Arg(16) ->Arg(32)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (2): n/a

#### `Arg(5) -> Arg(10) ->Arg(20) ->Arg(50) ->Arg(100)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (5): n/a

#### `BENCHMARK(BM_ConfigConstruction)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ConfigConstruction): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:248
- Brief: n/a
- Parameters: none

#### `void BM_ConfigConstruction(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:236
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MergeShardResults_KLimit(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:164
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MergeShardResults_Overlap(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:134
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MergeShardResults_ResultsPerShard(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:103
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MergeShardResults_ShardCount(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:73
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_MergeShardResults_WithFailures(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:197
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `DistributedHybridSearch::Config makeCfg(size_t k=10)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:30
- Brief: n/a
- Parameters:
  - `k` (size_t): n/a

#### `DistributedHybridSearch::ShardSearchResult makeShardResult(const std::string &shard_id, int n, int overlap, std::mt19937 &rng)`
- Source: `benchmarks/search/benchmark_distributed_hybrid_search.cpp`:42
- Brief: n/a
- Parameters:
  - `shard_id` (const std::string &): n/a
  - `n` (int): n/a
  - `overlap` (int): n/a
  - `rng` (std::mt19937 &): n/a

### benchmark_hybrid_search.cpp

#### `BENCHMARK(BM_ConfigConstruction)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:267
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ConfigConstruction): n/a

#### `BENCHMARK(BM_Linear_Hybrid) -> Arg(10) ->Arg(50) ->Arg(100)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:199
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Linear_Hybrid): n/a

#### `BENCHMARK(BM_NormalizeScores_BM25) -> Arg(10) ->Arg(100) ->Arg(1000) ->Arg(10000)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_NormalizeScores_BM25): n/a

#### `BENCHMARK(BM_NormalizeScores_Vector) -> Arg(10) ->Arg(100) ->Arg(1000) ->Arg(10000)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:250
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_NormalizeScores_Vector): n/a

#### `BENCHMARK(BM_RRF_BM25Only) -> Arg(10) ->Arg(50) ->Arg(100) ->Arg(500) ->Arg(1000)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RRF_BM25Only): n/a

#### `BENCHMARK(BM_RRF_Hybrid_50PctOverlap) -> Arg(50) ->Arg(100) ->Arg(500)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:150
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RRF_Hybrid_50PctOverlap): n/a

#### `BENCHMARK(BM_RRF_Hybrid_FullOverlap) -> Arg(50) ->Arg(100) ->Arg(500)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RRF_Hybrid_FullOverlap): n/a

#### `BENCHMARK(BM_RRF_Hybrid_NoOverlap) -> Arg(50) ->Arg(100) ->Arg(500)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RRF_Hybrid_NoOverlap): n/a

#### `BENCHMARK(BM_RRF_VaryingRrfK) -> Arg(1) ->Arg(10) ->Arg(60) ->Arg(100) ->Arg(1000)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RRF_VaryingRrfK): n/a

#### `BENCHMARK(BM_RRF_VectorOnly) -> Arg(10) ->Arg(50) ->Arg(100) ->Arg(500) ->Arg(1000)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:106
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RRF_VectorOnly): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:296
- Brief: n/a
- Parameters: none

#### `void BM_ConfigConstruction(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:256
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Linear_Hybrid(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:178
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_NormalizeScores_BM25(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:205
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_NormalizeScores_Vector(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:229
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RRF_BM25Only(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:66
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RRF_Hybrid_50PctOverlap(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:131
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RRF_Hybrid_FullOverlap(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:152
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RRF_Hybrid_NoOverlap(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:108
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RRF_VaryingRrfK(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:273
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RRF_VectorOnly(benchmark::State &state)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:87
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `std::vector< HybridSearch::Result > makeCandidates(int n, bool bm25, std::mt19937 &rng)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:31
- Brief: n/a
- Parameters:
  - `n` (int): n/a
  - `bm25` (bool): n/a
  - `rng` (std::mt19937 &): n/a

#### `std::pair< std::vector< HybridSearch::Result >, std::vector< HybridSearch::Result > > makeOverlappingCandidates(int n, int overlap, std::mt19937 &rng)`
- Source: `benchmarks/search/benchmark_hybrid_search.cpp`:52
- Brief: n/a
- Parameters:
  - `n` (int): n/a
  - `overlap` (int): n/a
  - `rng` (std::mt19937 &): n/a

### test_search_analytics.cpp

#### `TEST(SearchAnalyticsClear, ClearResetsCount)`
- Source: `tests/search/test_search_analytics.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsClear): n/a
  - `<unnamed>` (ClearResetsCount): n/a

#### `TEST(SearchAnalyticsConfig, ConfigRoundtrip)`
- Source: `tests/search/test_search_analytics.cpp`:28
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsConfig): n/a
  - `<unnamed>` (ConfigRoundtrip): n/a

#### `TEST(SearchAnalyticsConfig, DefaultConfigIsValid)`
- Source: `tests/search/test_search_analytics.cpp`:18
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsConfig): n/a
  - `<unnamed>` (DefaultConfigIsValid): n/a

#### `TEST(SearchAnalyticsConfig, ZeroMaxEventsThrows)`
- Source: `tests/search/test_search_analytics.cpp`:22
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsConfig): n/a
  - `<unnamed>` (ZeroMaxEventsThrows): n/a

#### `TEST(SearchAnalyticsMaxEvents, OldestEvictedWhenFull)`
- Source: `tests/search/test_search_analytics.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsMaxEvents): n/a
  - `<unnamed>` (OldestEvictedWhenFull): n/a

#### `TEST(SearchAnalyticsMetrics, AvgLatencyCorrect)`
- Source: `tests/search/test_search_analytics.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsMetrics): n/a
  - `<unnamed>` (AvgLatencyCorrect): n/a

#### `TEST(SearchAnalyticsMetrics, EmptyMetrics)`
- Source: `tests/search/test_search_analytics.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsMetrics): n/a
  - `<unnamed>` (EmptyMetrics): n/a

#### `TEST(SearchAnalyticsMetrics, PercentilesInNonDecreasingOrder)`
- Source: `tests/search/test_search_analytics.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsMetrics): n/a
  - `<unnamed>` (PercentilesInNonDecreasingOrder): n/a

#### `TEST(SearchAnalyticsMetrics, TopQueriesTracked)`
- Source: `tests/search/test_search_analytics.cpp`:187
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsMetrics): n/a
  - `<unnamed>` (TopQueriesTracked): n/a

#### `TEST(SearchAnalyticsMetrics, TotalQueriesCorrect)`
- Source: `tests/search/test_search_analytics.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsMetrics): n/a
  - `<unnamed>` (TotalQueriesCorrect): n/a

#### `TEST(SearchAnalyticsMetrics, ZeroResultRateCorrect)`
- Source: `tests/search/test_search_analytics.cpp`:179
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsMetrics): n/a
  - `<unnamed>` (ZeroResultRateCorrect): n/a

#### `TEST(SearchAnalyticsRecent, EmptyReturnsEmpty)`
- Source: `tests/search/test_search_analytics.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsRecent): n/a
  - `<unnamed>` (EmptyReturnsEmpty): n/a

#### `TEST(SearchAnalyticsRecent, MostRecentFirst)`
- Source: `tests/search/test_search_analytics.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsRecent): n/a
  - `<unnamed>` (MostRecentFirst): n/a

#### `TEST(SearchAnalyticsRecent, ReturnsUpToLimit)`
- Source: `tests/search/test_search_analytics.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsRecent): n/a
  - `<unnamed>` (ReturnsUpToLimit): n/a

#### `TEST(SearchAnalyticsRecord, EventCountIncrements)`
- Source: `tests/search/test_search_analytics.cpp`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsRecord): n/a
  - `<unnamed>` (EventCountIncrements): n/a

#### `TEST(SearchAnalyticsRecord, LatencyPreserved)`
- Source: `tests/search/test_search_analytics.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsRecord): n/a
  - `<unnamed>` (LatencyPreserved): n/a

#### `TEST(SearchAnalyticsRecord, NonZeroResultNotFlagged)`
- Source: `tests/search/test_search_analytics.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsRecord): n/a
  - `<unnamed>` (NonZeroResultNotFlagged): n/a

#### `TEST(SearchAnalyticsRecord, ZeroResultFlagged)`
- Source: `tests/search/test_search_analytics.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsRecord): n/a
  - `<unnamed>` (ZeroResultFlagged): n/a

#### `TEST(SearchAnalyticsThreadSafety, ConcurrentRecordDoesNotCrash)`
- Source: `tests/search/test_search_analytics.cpp`:292
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsThreadSafety): n/a
  - `<unnamed>` (ConcurrentRecordDoesNotCrash): n/a

#### `TEST(SearchAnalyticsTopQueries, CountsAreAccurate)`
- Source: `tests/search/test_search_analytics.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsTopQueries): n/a
  - `<unnamed>` (CountsAreAccurate): n/a

#### `TEST(SearchAnalyticsTopQueries, EmptyReturnsEmpty)`
- Source: `tests/search/test_search_analytics.cpp`:212
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsTopQueries): n/a
  - `<unnamed>` (EmptyReturnsEmpty): n/a

#### `TEST(SearchAnalyticsTopQueries, LimitHonored)`
- Source: `tests/search/test_search_analytics.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsTopQueries): n/a
  - `<unnamed>` (LimitHonored): n/a

#### `TEST(SearchAnalyticsTopQueries, LimitLargerThanDistinctQueries)`
- Source: `tests/search/test_search_analytics.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsTopQueries): n/a
  - `<unnamed>` (LimitLargerThanDistinctQueries): n/a

#### `TEST(SearchAnalyticsTopQueries, LimitZeroReturnsEmpty)`
- Source: `tests/search/test_search_analytics.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsTopQueries): n/a
  - `<unnamed>` (LimitZeroReturnsEmpty): n/a

#### `TEST(SearchAnalyticsTopQueries, SortedByDescendingFrequency)`
- Source: `tests/search/test_search_analytics.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsTopQueries): n/a
  - `<unnamed>` (SortedByDescendingFrequency): n/a

#### `TEST(SearchAnalyticsZeroResult, LimitHonored)`
- Source: `tests/search/test_search_analytics.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsZeroResult): n/a
  - `<unnamed>` (LimitHonored): n/a

#### `TEST(SearchAnalyticsZeroResult, MixedQueriesOnlyZeroReturned)`
- Source: `tests/search/test_search_analytics.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchAnalyticsZeroResult): n/a
  - `<unnamed>` (MixedQueriesOnlyZeroReturned): n/a

### test_search_future_interfaces.cpp

#### `TEST(ConversationalSearchConfig, ConfigRoundtrip)`
- Source: `tests/search/test_search_future_interfaces.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchConfig): n/a
  - `<unnamed>` (ConfigRoundtrip): n/a

#### `TEST(ConversationalSearchConfig, DefaultConfigIsValid)`
- Source: `tests/search/test_search_future_interfaces.cpp`:25
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchConfig): n/a
  - `<unnamed>` (DefaultConfigIsValid): n/a

#### `TEST(ConversationalSearchConfig, SetConfigZeroMaxHistoryThrows)`
- Source: `tests/search/test_search_future_interfaces.cpp`:52
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchConfig): n/a
  - `<unnamed>` (SetConfigZeroMaxHistoryThrows): n/a

#### `TEST(ConversationalSearchConfig, ZeroContextWindowIsValid)`
- Source: `tests/search/test_search_future_interfaces.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchConfig): n/a
  - `<unnamed>` (ZeroContextWindowIsValid): n/a

#### `TEST(ConversationalSearchConfig, ZeroMaxHistoryThrows)`
- Source: `tests/search/test_search_future_interfaces.cpp`:29
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchConfig): n/a
  - `<unnamed>` (ZeroMaxHistoryThrows): n/a

#### `TEST(ConversationalSearchHistory, ClearHistoryResetsToZero)`
- Source: `tests/search/test_search_future_interfaces.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchHistory): n/a
  - `<unnamed>` (ClearHistoryResetsToZero): n/a

#### `TEST(ConversationalSearchHistory, HistoryGrowsWithSearchCalls)`
- Source: `tests/search/test_search_future_interfaces.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchHistory): n/a
  - `<unnamed>` (HistoryGrowsWithSearchCalls): n/a

#### `TEST(ConversationalSearchHistory, MaxHistoryEvictsOldest)`
- Source: `tests/search/test_search_future_interfaces.cpp`:157
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchHistory): n/a
  - `<unnamed>` (MaxHistoryEvictsOldest): n/a

#### `TEST(ConversationalSearchHistory, TurnStoresOriginalAndReformulated)`
- Source: `tests/search/test_search_future_interfaces.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchHistory): n/a
  - `<unnamed>` (TurnStoresOriginalAndReformulated): n/a

#### `TEST(ConversationalSearchNullEngine, EmptyQueryReturnsEmpty)`
- Source: `tests/search/test_search_future_interfaces.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchNullEngine): n/a
  - `<unnamed>` (EmptyQueryReturnsEmpty): n/a

#### `TEST(ConversationalSearchNullEngine, SearchReturnsEmpty)`
- Source: `tests/search/test_search_future_interfaces.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchNullEngine): n/a
  - `<unnamed>` (SearchReturnsEmpty): n/a

#### `TEST(ConversationalSearchReformulate, EmptyHistoryReturnsQueryUnchanged)`
- Source: `tests/search/test_search_future_interfaces.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchReformulate): n/a
  - `<unnamed>` (EmptyHistoryReturnsQueryUnchanged): n/a

#### `TEST(ConversationalSearchReformulate, MultipleHistoryTurns)`
- Source: `tests/search/test_search_future_interfaces.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchReformulate): n/a
  - `<unnamed>` (MultipleHistoryTurns): n/a

#### `TEST(ConversationalSearchReformulate, SinglePriorTurnPrepended)`
- Source: `tests/search/test_search_future_interfaces.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchReformulate): n/a
  - `<unnamed>` (SinglePriorTurnPrepended): n/a

#### `TEST(ConversationalSearchReformulate, WindowLimitedToContextWindow)`
- Source: `tests/search/test_search_future_interfaces.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchReformulate): n/a
  - `<unnamed>` (WindowLimitedToContextWindow): n/a

#### `TEST(ConversationalSearchReformulate, ZeroWindowReturnsQueryUnchanged)`
- Source: `tests/search/test_search_future_interfaces.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchReformulate): n/a
  - `<unnamed>` (ZeroWindowReturnsQueryUnchanged): n/a

#### `TEST(ConversationalSearchTurn, DefaultTurnIsEmpty)`
- Source: `tests/search/test_search_future_interfaces.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (ConversationalSearchTurn): n/a
  - `<unnamed>` (DefaultTurnIsEmpty): n/a

#### `TEST(FederatedSearchConfig, ConfigRoundtrip)`
- Source: `tests/search/test_search_future_interfaces.cpp`:216
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchConfig): n/a
  - `<unnamed>` (ConfigRoundtrip): n/a

#### `TEST(FederatedSearchConfig, DefaultConfigIsValid)`
- Source: `tests/search/test_search_future_interfaces.cpp`:184
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchConfig): n/a
  - `<unnamed>` (DefaultConfigIsValid): n/a

#### `TEST(FederatedSearchConfig, NegativeRrfKThrows)`
- Source: `tests/search/test_search_future_interfaces.cpp`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchConfig): n/a
  - `<unnamed>` (NegativeRrfKThrows): n/a

#### `TEST(FederatedSearchConfig, SetConfigZeroKThrows)`
- Source: `tests/search/test_search_future_interfaces.cpp`:209
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchConfig): n/a
  - `<unnamed>` (SetConfigZeroKThrows): n/a

#### `TEST(FederatedSearchConfig, ZeroKThrows)`
- Source: `tests/search/test_search_future_interfaces.cpp`:188
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchConfig): n/a
  - `<unnamed>` (ZeroKThrows): n/a

#### `TEST(FederatedSearchConfig, ZeroRrfKThrows)`
- Source: `tests/search/test_search_future_interfaces.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchConfig): n/a
  - `<unnamed>` (ZeroRrfKThrows): n/a

#### `TEST(FederatedSearchMerge, EmptyInputReturnsEmpty)`
- Source: `tests/search/test_search_future_interfaces.cpp`:323
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchMerge): n/a
  - `<unnamed>` (EmptyInputReturnsEmpty): n/a

#### `TEST(FederatedSearchMerge, KLimitsResultCount)`
- Source: `tests/search/test_search_future_interfaces.cpp`:371
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchMerge): n/a
  - `<unnamed>` (KLimitsResultCount): n/a

#### `TEST(FederatedSearchMerge, MultiTenantResultsLabeledCorrectly)`
- Source: `tests/search/test_search_future_interfaces.cpp`:350
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchMerge): n/a
  - `<unnamed>` (MultiTenantResultsLabeledCorrectly): n/a

#### `TEST(FederatedSearchMerge, SingleTenantPreservesOrder)`
- Source: `tests/search/test_search_future_interfaces.cpp`:329
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchMerge): n/a
  - `<unnamed>` (SingleTenantPreservesOrder): n/a

#### `TEST(FederatedSearchNullSafety, SearchWithNoTenantsReturnsEmpty)`
- Source: `tests/search/test_search_future_interfaces.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchNullSafety): n/a
  - `<unnamed>` (SearchWithNoTenantsReturnsEmpty): n/a

#### `TEST(FederatedSearchNullSafety, SearchWithNullTenantReturnsEmpty)`
- Source: `tests/search/test_search_future_interfaces.cpp`:300
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchNullSafety): n/a
  - `<unnamed>` (SearchWithNullTenantReturnsEmpty): n/a

#### `TEST(FederatedSearchResult, DefaultResultIsZero)`
- Source: `tests/search/test_search_future_interfaces.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchResult): n/a
  - `<unnamed>` (DefaultResultIsZero): n/a

#### `TEST(FederatedSearchStats, DefaultStatsIsZero)`
- Source: `tests/search/test_search_future_interfaces.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchStats): n/a
  - `<unnamed>` (DefaultStatsIsZero): n/a

#### `TEST(FederatedSearchTenants, DefaultWeightIsOne)`
- Source: `tests/search/test_search_future_interfaces.cpp`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchTenants): n/a
  - `<unnamed>` (DefaultWeightIsOne): n/a

#### `TEST(FederatedSearchTenants, InitiallyEmpty)`
- Source: `tests/search/test_search_future_interfaces.cpp`:249
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchTenants): n/a
  - `<unnamed>` (InitiallyEmpty): n/a

#### `TEST(FederatedSearchTenants, RegisterIncreasesCount)`
- Source: `tests/search/test_search_future_interfaces.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchTenants): n/a
  - `<unnamed>` (RegisterIncreasesCount): n/a

#### `TEST(FederatedSearchTenants, RemoveDecreasesCount)`
- Source: `tests/search/test_search_future_interfaces.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchTenants): n/a
  - `<unnamed>` (RemoveDecreasesCount): n/a

#### `TEST(FederatedSearchTenants, RemoveUnknownTenantIsNoOp)`
- Source: `tests/search/test_search_future_interfaces.cpp`:269
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchTenants): n/a
  - `<unnamed>` (RemoveUnknownTenantIsNoOp): n/a

#### `TEST(FederatedSearchTenants, SetWeightClampedToUnitInterval)`
- Source: `tests/search/test_search_future_interfaces.cpp`:281
- Brief: n/a
- Parameters:
  - `<unnamed>` (FederatedSearchTenants): n/a
  - `<unnamed>` (SetWeightClampedToUnitInterval): n/a

#### `TEST(SearchResultStreamConfig, ConfigRoundtrip)`
- Source: `tests/search/test_search_future_interfaces.cpp`:412
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamConfig): n/a
  - `<unnamed>` (ConfigRoundtrip): n/a

#### `TEST(SearchResultStreamConfig, DefaultConfigIsValid)`
- Source: `tests/search/test_search_future_interfaces.cpp`:389
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamConfig): n/a
  - `<unnamed>` (DefaultConfigIsValid): n/a

#### `TEST(SearchResultStreamConfig, SetConfigZeroTotalKThrows)`
- Source: `tests/search/test_search_future_interfaces.cpp`:405
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamConfig): n/a
  - `<unnamed>` (SetConfigZeroTotalKThrows): n/a

#### `TEST(SearchResultStreamConfig, ZeroPageSizeThrows)`
- Source: `tests/search/test_search_future_interfaces.cpp`:399
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamConfig): n/a
  - `<unnamed>` (ZeroPageSizeThrows): n/a

#### `TEST(SearchResultStreamConfig, ZeroTotalKThrows)`
- Source: `tests/search/test_search_future_interfaces.cpp`:393
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamConfig): n/a
  - `<unnamed>` (ZeroTotalKThrows): n/a

#### `TEST(SearchResultStreamForEach, EmptyStreamCallbackNeverInvoked)`
- Source: `tests/search/test_search_future_interfaces.cpp`:473
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamForEach): n/a
  - `<unnamed>` (EmptyStreamCallbackNeverInvoked): n/a

#### `TEST(SearchResultStreamForEach, NullCallbackIsNoOp)`
- Source: `tests/search/test_search_future_interfaces.cpp`:483
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamForEach): n/a
  - `<unnamed>` (NullCallbackIsNoOp): n/a

#### `TEST(SearchResultStreamNullEngine, InitiallyHasNoMore)`
- Source: `tests/search/test_search_future_interfaces.cpp`:425
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamNullEngine): n/a
  - `<unnamed>` (InitiallyHasNoMore): n/a

#### `TEST(SearchResultStreamNullEngine, NextPageOnEmptyReturnsEmpty)`
- Source: `tests/search/test_search_future_interfaces.cpp`:437
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamNullEngine): n/a
  - `<unnamed>` (NextPageOnEmptyReturnsEmpty): n/a

#### `TEST(SearchResultStreamNullEngine, OpenReturnsEmptyStream)`
- Source: `tests/search/test_search_future_interfaces.cpp`:430
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamNullEngine): n/a
  - `<unnamed>` (OpenReturnsEmptyStream): n/a

#### `TEST(SearchResultStreamOpen, EmptyQueryProducesEmptyStream)`
- Source: `tests/search/test_search_future_interfaces.cpp`:492
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamOpen): n/a
  - `<unnamed>` (EmptyQueryProducesEmptyStream): n/a

#### `TEST(SearchResultStreamPagination, CloseResetsStateCompletely)`
- Source: `tests/search/test_search_future_interfaces.cpp`:453
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamPagination): n/a
  - `<unnamed>` (CloseResetsStateCompletely): n/a

#### `TEST(SearchResultStreamPagination, InitialCursorIsZero)`
- Source: `tests/search/test_search_future_interfaces.cpp`:448
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamPagination): n/a
  - `<unnamed>` (InitialCursorIsZero): n/a

#### `TEST(SearchResultStreamPagination, ResetRewoundsCursorToZero)`
- Source: `tests/search/test_search_future_interfaces.cpp`:461
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchResultStreamPagination): n/a
  - `<unnamed>` (ResetRewoundsCursorToZero): n/a

#### `HybridSearch::Result makeHSResult(const std::string &id, double hybrid=0.5, double bm25=0.3, double vec=0.2)`
- Source: `tests/search/test_search_future_interfaces.cpp`:311
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `hybrid` (double): n/a
  - `bm25` (double): n/a
  - `vec` (double): n/a

### test_search_highcardinality_stress.cpp

#### `TEST(SearchStress, ConcurrentRankingStress)`
- Source: `tests/search/test_search_highcardinality_stress.cpp`:131
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchStress): n/a
  - `<unnamed>` (ConcurrentRankingStress): n/a

#### `TEST(SearchStress, FacetFilterStress)`
- Source: `tests/search/test_search_highcardinality_stress.cpp`:171
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchStress): n/a
  - `<unnamed>` (FacetFilterStress): n/a

#### `TEST(SearchStress, HighCardinalityDocumentIndex)`
- Source: `tests/search/test_search_highcardinality_stress.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchStress): n/a
  - `<unnamed>` (HighCardinalityDocumentIndex): n/a

### themis

#### `std::string searchErrorCodeToString(SearchErrorCode code)`
- Source: `include/search/search_error_codes.h`:160
- Brief: Convert SearchErrorCode to human-readable string.
- Parameters:
  - `code` (SearchErrorCode): Error code to convert.
- Return: Descriptive string for the error code.
- Details: code Error code to convert. Descriptive string for the error code.

### themis::AutocompleteEngine

#### `AutocompleteEngine(SecondaryIndexManager *index, SearchAnalytics *analytics, const Config &config)`
- Source: `include/search/autocomplete.h`:93
- Brief: n/a
- Parameters:
  - `index` (SecondaryIndexManager *): Non-owning pointer to a SecondaryIndexManager (may be null if only popular-query suggestions are needed).
  - `analytics` (SearchAnalytics *): Optional non-owning pointer to SearchAnalytics for popular query sourcing (may be null).
  - `config` (const Config &): Engine configuration.
- Throws:
  - std::invalid_argument: if max_suggestions == 0 or min_prefix_length == 0.
- Details: index Non-owning pointer to a SecondaryIndexManager (may be null if only popular-query suggestions are needed). analytics Optional non-owning pointer to SearchAnalytics for popular query sourcing (may be null). config Engine configuration. std::invalid_argument if max_suggestions == 0 or min_prefix_length == 0.

#### `AutocompleteEngine(SecondaryIndexManager *index, SearchAnalytics *analytics=nullptr)`
- Source: `include/search/autocomplete.h`:83
- Brief: n/a
- Parameters:
  - `index` (SecondaryIndexManager *): Non-owning pointer to a SecondaryIndexManager (may be null if only popular-query suggestions are needed).
  - `analytics` (SearchAnalytics *): Optional non-owning pointer to SearchAnalytics for popular query sourcing (may be null).
- Throws:
  - std::invalid_argument: if max_suggestions == 0 or min_prefix_length == 0.
- Details: index Non-owning pointer to a SecondaryIndexManager (may be null if only popular-query suggestions are needed). analytics Optional non-owning pointer to SearchAnalytics for popular query sourcing (may be null). std::invalid_argument if max_suggestions == 0 or min_prefix_length == 0.

#### `const Config & getConfig() const`
- Source: `include/search/autocomplete.h`:146
- Brief: n/a
- Parameters: none

#### `std::vector< Suggestion > suggest(const std::string &prefix, const std::string &table="", const std::string &column="") const`
- Source: `include/search/autocomplete.h`:112
- Brief: Return combined prefix + popular suggestions for the given prefix.
- Parameters:
  - `prefix` (const std::string &): Incomplete query text entered by the user.
  - `table` (const std::string &): Table name for prefix-index scan (ignored when index is null).
  - `column` (const std::string &): Column to scan for prefix matches.
- Return: Ranked suggestion list.
- Details: Results are sorted by score descending, deduplicated (if Config::deduplicate), and capped at Config::max_suggestions. prefix Incomplete query text entered by the user. table Table name for prefix-index scan (ignored when index is null). column Column to scan for prefix matches. Ranked suggestion list.

#### `std::vector< Suggestion > suggestByPrefix(const std::string &prefix, const std::string &table, const std::string &column, size_t limit=20) const`
- Source: `include/search/autocomplete.h`:128
- Brief: Return prefix-based suggestions by scanning a secondary index column.
- Parameters:
  - `prefix` (const std::string &): Prefix to match.
  - `table` (const std::string &): Table name.
  - `column` (const std::string &): Column to scan.
  - `limit` (size_t): Maximum results.
- Return: List of matching field values as Suggestion objects.
- Details: Uses SecondaryIndexManager::scanKeysRange with the prefix as lower bound and prefix + '\xff' as upper bound to collect field values. prefix Prefix to match. table Table name. column Column to scan. limit Maximum results. List of matching field values as Suggestion objects.

#### `std::vector< Suggestion > suggestPopular(const std::string &prefix, size_t limit=20) const`
- Source: `include/search/autocomplete.h`:143
- Brief: Return popular past queries that start with the given prefix.
- Parameters:
  - `prefix` (const std::string &): Prefix to match.
  - `limit` (size_t): Maximum results.
- Return: List of Suggestion objects with is_popular == true.
- Details: Uses the SearchAnalytics instance (if non-null) to retrieve the most frequent queries whose text begins with prefix. prefix Prefix to match. limit Maximum results. List of Suggestion objects with is_popular == true.

### themis::ConversationalSearch

#### `ConversationalSearch(HybridSearch *hybrid_search)`
- Source: `include/search/conversational_search.h`:116
- Brief: Construct a ConversationalSearch engine.
- Parameters:
  - `hybrid_search` (HybridSearch *): Non-owning pointer to an underlying HybridSearch instance. May be null; all searches return empty.
- Throws:
  - std::invalid_argument: on invalid config values.
- Details: hybrid_search Non-owning pointer to an underlying HybridSearch instance. May be null; all searches return empty. std::invalid_argument on invalid config values.

#### `ConversationalSearch(HybridSearch *hybrid_search, const Config &config)`
- Source: `include/search/conversational_search.h`:125
- Brief: Construct a ConversationalSearch engine.
- Parameters:
  - `hybrid_search` (HybridSearch *): Non-owning pointer to an underlying HybridSearch instance. May be null; all searches return empty.
  - `config` (const Config &): Engine configuration.
- Throws:
  - std::invalid_argument: on invalid config values.
- Details: hybrid_search Non-owning pointer to an underlying HybridSearch instance. May be null; all searches return empty. config Engine configuration. std::invalid_argument on invalid config values.

#### `void clearHistory()`
- Source: `include/search/conversational_search.h`:174
- Brief: Remove all history entries.
- Parameters: none
- Details: Clear History. Calls: clear().

#### `const Config & getConfig() const`
- Source: `include/search/conversational_search.h`:180
- Brief: n/a
- Parameters: none

#### `const std::deque< Turn > & getHistory() const`
- Source: `include/search/conversational_search.h`:168
- Brief: Return the full conversation history (oldest first).
- Parameters: none

#### `size_t historySize() const`
- Source: `include/search/conversational_search.h`:171
- Brief: Number of turns in the history.
- Parameters: none

#### `std::string reformulate(const std::string &query) const`
- Source: `include/search/conversational_search.h`:161
- Brief: Build a context-enriched query string.
- Parameters:
  - `query` (const std::string &): New raw user query.
- Return: Context-enriched query string.
- Details: Concatenates the most recent Config::context_window queries from the history with query using Config::context_separator. When context_window == 0 or the history is empty, query is returned unchanged. query New raw user query. Context-enriched query string.

#### `std::vector< HybridSearch::Result > search(const std::string &query)`
- Source: `include/search/conversational_search.h`:144
- Brief: Execute a context-aware search turn.
- Parameters:
  - `query` (const std::string &): Input parameter.
- Return: Retrieved results, sorted by hybrid score descending.
- Details: Search. The user query is reformulated using the most recent Config::context_window turns from the conversation history (via reformulate()), the enriched query is dispatched to the underlying HybridSearch, and the turn is appended to the history. Never throws; all index exceptions are caught internally. query Raw user query for this turn. Retrieved results, sorted by hybrid score descending. query Input parameter. Return value. Calls: empty(), reformulate(), THEMIS_ERROR(), what(), size(), pop_front(), push_back(), std::move().

#### `void setConfig(const Config &config)`
- Source: `include/search/conversational_search.h`:181
- Brief: Set Config.
- Parameters:
  - `config` (const Config &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: config Input parameter. std::invalid_argument if an error occurs. Implements setConfig without additional internal calls.

### themis::CrossLingualSearch

#### `CrossLingualSearch(VectorIndexManager *vec_index)`
- Source: `include/search/cross_lingual_search.h`:136
- Brief: Construct a CrossLingualSearch engine.
- Parameters:
  - `vec_index` (VectorIndexManager *): Non-owning pointer to a VectorIndexManager. May be null; all searches will return empty results.
- Throws:
  - std::invalid_argument: when config contains invalid values.
- Details: vec_index Non-owning pointer to a VectorIndexManager. May be null; all searches will return empty results. std::invalid_argument when config contains invalid values.

#### `CrossLingualSearch(VectorIndexManager *vec_index, const Config &config)`
- Source: `include/search/cross_lingual_search.h`:145
- Brief: Construct a CrossLingualSearch engine.
- Parameters:
  - `vec_index` (VectorIndexManager *): Non-owning pointer to a VectorIndexManager. May be null; all searches will return empty results.
  - `config` (const Config &): Engine configuration.
- Throws:
  - std::invalid_argument: when config contains invalid values.
- Details: vec_index Non-owning pointer to a VectorIndexManager. May be null; all searches will return empty results. config Engine configuration. std::invalid_argument when config contains invalid values.

#### `std::vector< Result > applyHintsAndFinalize(std::vector< std::pair< std::string, double > > scored, const std::vector< LanguageHint > &hints) const`
- Source: `include/search/cross_lingual_search.h`:223
- Brief: n/a
- Parameters:
  - `scored` (std::vector< std::pair< std::string, double > >): n/a
  - `hints` (const std::vector< LanguageHint > &): n/a

#### `std::vector< std::pair< std::string, double > > executeKnn(const std::vector< float > &embedding) const`
- Source: `include/search/cross_lingual_search.h`:218
- Brief: n/a
- Parameters:
  - `embedding` (const std::vector< float > &): n/a

#### `const Config & getConfig() const`
- Source: `include/search/cross_lingual_search.h`:209
- Brief: n/a
- Parameters: none

#### `std::vector< Result > search(const std::vector< float > &query_embedding, const std::vector< LanguageHint > &language_hints={}) const`
- Source: `include/search/cross_lingual_search.h`:179
- Brief: Search with a single multilingual embedding.
- Parameters:
  - `query_embedding` (const std::vector< float > &): Pre-computed multilingual embedding vector. Empty vector returns empty results immediately.
  - `language_hints` (const std::vector< LanguageHint > &): Optional per-language boost factors.
- Return: Results sorted by score descending, capped at Config::k.
- Details: Issues a kNN query on the vector index, converts distances to similarity scores via 1 / (1 + distance), applies language boost factors from language_hints, filters candidates below Config::score_threshold, and returns the top-k results. query_embedding Pre-computed multilingual embedding vector. Empty vector returns empty results immediately. language_hints Optional per-language boost factors. Results sorted by score descending, capped at Config::k.

#### `std::vector< Result > searchMultiEmbedding(const std::vector< EmbeddingQuery > &queries, const std::vector< LanguageHint > &language_hints={}) const`
- Source: `include/search/cross_lingual_search.h`:200
- Brief: Fuse results from multiple embeddings via Reciprocal Rank Fusion.
- Parameters:
  - `queries` (const std::vector< EmbeddingQuery > &): One or more (embedding, weight) pairs.
  - `language_hints` (const std::vector< LanguageHint > &): Optional per-language boost factors.
- Return: Fused results sorted by score descending, capped at Config::k.
- Details: Executes an independent kNN query for each EmbeddingQuery, merges the ranked lists using weighted RRF, then applies language boosts and score threshold filtering. Queries with empty embeddings are skipped silently. RRF formula (per query list i): score(doc)+=weight_i/(rrf_k+rank_i(doc)) queries One or more (embedding, weight) pairs. language_hints Optional per-language boost factors. Fused results sorted by score descending, capped at Config::k.

#### `void setConfig(const Config &config)`
- Source: `include/search/cross_lingual_search.h`:210
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `void setLanguageMap(std::unordered_map< std::string, std::string > lang_map)`
- Source: `include/search/cross_lingual_search.h`:160
- Brief: Supply a document-language map for result annotation and boosts.
- Parameters:
  - `lang_map` (std::unordered_map< std::string, std::string >): Map of doc_id → language_code.
- Details: Maps document primary keys to ISO 639-1 language codes (e.g. "en", "de", "fr"). Replaces any previously set map. lang_map Map of doc_id → language_code.

### themis::DistributedHybridSearch

#### `DistributedHybridSearch(HybridSearch *local_search, themis::sharding::URNResolver *resolver, themis::sharding::RemoteExecutor *executor)`
- Source: `include/search/distributed_hybrid_search.h`:157
- Brief: Construct a DistributedHybridSearch engine.
- Parameters:
  - `local_search` (HybridSearch *): Local HybridSearch instance for this node. May be null; the local shard will then return no results.
  - `resolver` (themis::sharding::URNResolver *): URN/shard resolver used to enumerate healthy shards. May be null; only the local shard will be queried.
  - `executor` (themis::sharding::RemoteExecutor *): RemoteExecutor configured with mTLS credentials for inter-node communication. May be null; in that case only local search is executed.
- Throws:
  - std::invalid_argument: on invalid configuration values.
- Details: local_search Local HybridSearch instance for this node. May be null; the local shard will then return no results. resolver URN/shard resolver used to enumerate healthy shards. May be null; only the local shard will be queried. executor RemoteExecutor configured with mTLS credentials for inter-node communication. May be null; in that case only local search is executed. std::invalid_argument on invalid configuration values.

#### `DistributedHybridSearch(HybridSearch *local_search, themis::sharding::URNResolver *resolver, themis::sharding::RemoteExecutor *executor, const Config &config)`
- Source: `include/search/distributed_hybrid_search.h`:177
- Brief: Construct a DistributedHybridSearch engine.
- Parameters:
  - `local_search` (HybridSearch *): Local HybridSearch instance for this node. May be null; the local shard will then return no results.
  - `resolver` (themis::sharding::URNResolver *): URN/shard resolver used to enumerate healthy shards. May be null; only the local shard will be queried.
  - `executor` (themis::sharding::RemoteExecutor *): RemoteExecutor configured with mTLS credentials for inter-node communication. May be null; in that case only local search is executed.
  - `config` (const Config &): Engine configuration.
- Throws:
  - std::invalid_argument: on invalid configuration values.
- Details: local_search Local HybridSearch instance for this node. May be null; the local shard will then return no results. resolver URN/shard resolver used to enumerate healthy shards. May be null; only the local shard will be queried. executor RemoteExecutor configured with mTLS credentials for inter-node communication. May be null; in that case only local search is executed. config Engine configuration. std::invalid_argument on invalid configuration values.

#### `const Config & getConfig() const`
- Source: `include/search/distributed_hybrid_search.h`:236
- Brief: n/a
- Parameters: none

#### `std::vector< HybridSearch::Result > mergeShardResults(const std::vector< ShardSearchResult > &shard_results, SearchStats *stats=nullptr) const`
- Source: `include/search/distributed_hybrid_search.h`:227
- Brief: Merge per-shard result lists via Reciprocal Rank Fusion.
- Parameters:
  - `shard_results` (const std::vector< ShardSearchResult > &): Per-shard result collections (failed shards are skipped when skip_failed_shards is true).
  - `stats` (SearchStats *): Optional output: merge-specific degradation flags (merge_underflow, high_overlap_variance).
- Return: Merged results sorted by hybrid score descending, capped at k.
- Details: Phase 2: Enhanced merge with degradation flag tracking (merge_underflow, high_overlap_variance). Applies a two-level RRF: within each shard results are already ranked; across shards each result contributes 1 / (rrf_k + rank_in_shard) to the global score. Results that appear in multiple shards accumulate contributions from each shard. shard_results Per-shard result collections (failed shards are skipped when skip_failed_shards is true). stats Optional output: merge-specific degradation flags (merge_underflow, high_overlap_variance). Merged results sorted by hybrid score descending, capped at k.

#### `std::vector< HybridSearch::Result > parseShardResponse(const nlohmann::json &data)`
- Source: `include/search/distributed_hybrid_search.h`:255
- Brief: Parse a JSON result array from a shard HTTP response.
- Parameters:
  - `data` (const nlohmann::json &): Input parameter.
- Return: Return value.
- Details: Parse Shard Response. Exposed as public static so it can be unit-tested directly. Tolerates missing fields by using zero/empty defaults. Accepted formats: Direct JSON array: [{"document_id": "...", ...}, ...] Wrapped: {"results": [{...}, ...]} Entries with empty document_id are silently dropped. This method is public primarily to enable direct unit testing of the JSON deserialization logic without requiring network infrastructure. It is a stateless utility with no side effects. data Input parameter. Return value. Calls: is_array(), is_object(), contains(), THEMIS_WARN(), reserve(), size(), is_string(), empty().

#### `std::vector< HybridSearch::Result > search(const std::string &text_query, const std::vector< float > &vector_query={}, SearchStats *stats=nullptr)`
- Source: `include/search/distributed_hybrid_search.h`:202
- Brief: Execute distributed hybrid search across all healthy shards.
- Parameters:
  - `text_query` (const std::string &): Input parameter.
  - `vector_query` (const std::vector< float > &): Input parameter.
  - `stats` (SearchStats *): Input/output parameter.
- Return: Globally merged top-k results, sorted by hybrid score.
- Details: Search. Runs the local HybridSearch and dispatches the same query to every healthy remote shard in parallel. Results are merged via cross-shard RRF and the top-k globally ranked results are returned. Never throws; all errors from individual shards are caught internally. text_query Full-text query string (empty to skip BM25). vector_query Optional semantic query vector (empty to skip ANN). stats Optional output: per-shard diagnostics. Globally merged top-k results, sorted by hybrid score. text_query Input parameter. vector_query Input parameter. stats Input/output parameter. Return value. Calls: empty(), std::chrono::steady_clock::now(), data(), size(), count(), THEMIS_DEBUG(), std::string(), what().

#### `ShardSearchResult searchRemoteShard(const themis::sharding::ShardInfo &shard, const std::string &text_query, const std::vector< float > &vector_query, size_t k)`
- Source: `include/search/distributed_hybrid_search.h`:273
- Brief: Query a single remote shard via HTTP POST (mTLS).
- Parameters:
  - `shard` (const themis::sharding::ShardInfo &): n/a
  - `text_query` (const std::string &): n/a
  - `vector_query` (const std::vector< float > &): n/a
  - `k` (size_t): n/a

#### `void setConfig(const Config &config)`
- Source: `include/search/distributed_hybrid_search.h`:237
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

### themis::FacetedSearch

#### `FacetedSearch(SecondaryIndexManager *index)`
- Source: `include/search/faceted_search.h`:79
- Brief: n/a
- Parameters:
  - `index` (SecondaryIndexManager *): Non-owning pointer to a SecondaryIndexManager. Must outlive this.
- Details: index Non-owning pointer to a SecondaryIndexManager. Must outlive this.

#### `std::pair< SecondaryIndexManager::Status, std::vector< std::string > > applyFacetFilters(const std::string &table, const std::vector< std::string > &candidate_pks, const std::vector< ActiveFacet > &active_facets) const`
- Source: `include/search/faceted_search.h`:151
- Brief: Apply active facet filters to a candidate PK set.
- Parameters:
  - `table` (const std::string &): Table name.
  - `candidate_pks` (const std::vector< std::string > &): PKs to filter.
  - `active_facets` (const std::vector< ActiveFacet > &): Required field=value filters.
- Return: Filtered PK list (may be empty).
- Details: Given a set of candidate PKs and a list of required field=value constraints, return only the PKs that satisfy all constraints. table Table name. candidate_pks PKs to filter. active_facets Required field=value filters. Filtered PK list (may be empty).

#### `std::pair< SecondaryIndexManager::Status, std::vector< FacetResult > > computeDynamicFacets(const std::string &table, const std::vector< std::string > &candidate_pks={}, size_t max_values=100) const`
- Source: `include/search/faceted_search.h`:183
- Brief: Compute dynamic facets for all discoverable columns in a table.
- Parameters:
  - `table` (const std::string &): Table name.
  - `candidate_pks` (const std::vector< std::string > &): Optional set of PKs to restrict counting (search results).
  - `max_values` (size_t): Maximum distinct values per facet (0 = no limit).
- Return: Pair of Status and list of FacetResult (one per discoverable column).
- Details: Automatically calls discoverFacetableColumns() and then computeFacet() for each discovered column. This provides "dynamic" facet counting — callers do not need to know which columns are indexed in advance. table Table name. candidate_pks Optional set of PKs to restrict counting (search results). max_values Maximum distinct values per facet (0 = no limit). Pair of Status and list of FacetResult (one per discoverable column).

#### `std::pair< SecondaryIndexManager::Status, FacetResult > computeFacet(const std::string &table, const std::string &column, const std::vector< std::string > &candidate_pks={}, size_t max_values=100) const`
- Source: `include/search/faceted_search.h`:98
- Brief: Compute value counts for a single field (categorical facet).
- Parameters:
  - `table` (const std::string &): Table name.
  - `column` (const std::string &): Column to facet on.
  - `candidate_pks` (const std::vector< std::string > &): Optional set of PKs to restrict counting (search results).
  - `max_values` (size_t): Maximum number of distinct values to return (0 = no limit).
- Return: Pair of Status and FacetResult.
- Details: Scans the secondary index for table.column and counts the occurrences of each distinct value, limited to documents in candidate_pks (if non-empty). If candidate_pks is empty all documents are counted. table Table name. column Column to facet on. candidate_pks Optional set of PKs to restrict counting (search results). max_values Maximum number of distinct values to return (0 = no limit). Pair of Status and FacetResult.

#### `std::pair< SecondaryIndexManager::Status, std::vector< FacetResult > > computeFacets(const std::string &table, const std::vector< std::string > &columns, const std::vector< std::string > &candidate_pks={}) const`
- Source: `include/search/faceted_search.h`:116
- Brief: Compute multiple facets at once.
- Parameters:
  - `table` (const std::string &): Table name.
  - `columns` (const std::vector< std::string > &): List of columns to compute facets for.
  - `candidate_pks` (const std::vector< std::string > &): Optional set of PKs to restrict counting.
- Return: Pair of Status and list of FacetResult (one per column).
- Details: Convenience wrapper for calling computeFacet() on several columns in a single call. table Table name. columns List of columns to compute facets for. candidate_pks Optional set of PKs to restrict counting. Pair of Status and list of FacetResult (one per column).

#### `std::pair< SecondaryIndexManager::Status, FacetResult > computeRangeFacet(const std::string &table, const std::string &column, const std::vector< RangeBucket > &buckets, const std::vector< std::string > &candidate_pks={}) const`
- Source: `include/search/faceted_search.h`:133
- Brief: Compute a numeric range facet.
- Parameters:
  - `table` (const std::string &): Table name.
  - `column` (const std::string &): Numeric column.
  - `buckets` (const std::vector< RangeBucket > &): Ordered list of range definitions.
  - `candidate_pks` (const std::vector< std::string > &): Optional set of PKs to restrict counting.
- Return: Pair of Status and FacetResult (bucket labels as value_counts keys).
- Details: Counts documents whose field value falls into each provided bucket. table Table name. column Numeric column. buckets Ordered list of range definitions. candidate_pks Optional set of PKs to restrict counting. Pair of Status and FacetResult (bucket labels as value_counts keys).

#### `std::pair< SecondaryIndexManager::Status, std::vector< std::string > > discoverFacetableColumns(const std::string &table) const`
- Source: `include/search/faceted_search.h`:167
- Brief: Discover all columns in a table that are suitable for faceting.
- Parameters:
  - `table` (const std::string &): Table name.
- Return: Pair of Status and list of column names suitable for faceting.
- Details: Queries the index metadata to find columns with regular, range, or sparse indexes. Geo, TTL, fulltext, and composite indexes are excluded because they do not produce meaningful categorical or numeric range facets. table Table name. Pair of Status and list of column names suitable for faceting.

### themis::FederatedSearch

#### `FederatedSearch()`
- Source: `include/search/federated_search.h`:123
- Brief: Construct a FederatedSearch engine.
- Parameters: none
- Throws:
  - std::invalid_argument: on invalid config values.
- Details: std::invalid_argument on invalid config values.

#### `FederatedSearch(const Config &config)`
- Source: `include/search/federated_search.h`:130
- Brief: Construct a FederatedSearch engine.
- Parameters:
  - `config` (const Config &): Engine configuration.
- Throws:
  - std::invalid_argument: on invalid config values.
- Details: config Engine configuration. std::invalid_argument on invalid config values.

#### `const Config & getConfig() const`
- Source: `include/search/federated_search.h`:221
- Brief: n/a
- Parameters: none

#### `double getTenantWeight(const std::string &tenant_id) const`
- Source: `include/search/federated_search.h`:173
- Brief: Return the weight associated with a tenant (default 1.0).
- Parameters:
  - `tenant_id` (const std::string &): n/a

#### `std::vector< Result > mergeTenantResults(const std::unordered_map< std::string, std::vector< HybridSearch::Result > > &tenant_results) const`
- Source: `include/search/federated_search.h`:212
- Brief: Merge per-tenant result lists via weighted RRF (public for tests).
- Parameters:
  - `tenant_results` (const std::unordered_map< std::string, std::vector< HybridSearch::Result > > &): Map of tenant_id to result list.
- Return: Merged results sorted by score descending, capped at Config::k.
- Details: tenant_results Map of tenant_id to result list. Merged results sorted by score descending, capped at Config::k.

#### `void registerTenant(const std::string &tenant_id, HybridSearch *hybrid_search)`
- Source: `include/search/federated_search.h`:146
- Brief: Register a tenant HybridSearch instance.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
  - `hybrid_search` (HybridSearch *): Input/output parameter.
- Details: Register Tenant. Replaces any previously registered instance for the same tenant ID. The pointer is non-owning; the caller retains ownership. tenant_id Unique tenant identifier. hybrid_search Pointer to the tenant's HybridSearch engine. May be null; the tenant will be skipped during search. tenant_id Identifier of the tenant. hybrid_search Input/output parameter. Implements registerTenant without additional internal calls.

#### `void removeTenant(const std::string &tenant_id)`
- Source: `include/search/federated_search.h`:156
- Brief: Remove a previously registered tenant.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
- Details: Remove Tenant. No-op when the tenant ID is not registered. tenant_id Tenant to remove. tenant_id Identifier of the tenant. Calls: erase().

#### `std::vector< Result > search(const std::string &query, const std::vector< float > &vector_query={}, std::vector< TenantStats > *tenant_stats=nullptr)`
- Source: `include/search/federated_search.h`:200
- Brief: Execute a federated search across all registered tenants.
- Parameters:
  - `query` (const std::string &): Input parameter.
  - `vector_query` (const std::vector< float > &): Input parameter.
  - `tenant_stats` (std::vector< TenantStats > *): Input/output parameter.
- Return: Top-k merged results sorted by score descending.
- Details: Search. Queries each non-skipped tenant HybridSearch instance in sequence, applies per-tenant weights, merges results via Reciprocal Rank Fusion, and returns the top-k globally ranked results. Per-tenant diagnostics are written to tenant_stats when non-null. Never throws; all per-tenant exceptions are caught internally. query Full-text query string. vector_query Optional semantic embedding vector. tenant_stats Optional: per-tenant diagnostics output. Top-k merged results sorted by score descending. query Input parameter. vector_query Input parameter. tenant_stats Input/output parameter. Return value. Calls: getTenantWeight(), push_back(), THEMIS_ERROR(), size(), std::move(), what(), mergeTenantResults().

#### `void setConfig(const Config &config)`
- Source: `include/search/federated_search.h`:222
- Brief: Set Config.
- Parameters:
  - `config` (const Config &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: config Input parameter. std::invalid_argument if an error occurs. Implements setConfig without additional internal calls.

#### `void setTenantWeight(const std::string &tenant_id, double weight)`
- Source: `include/search/federated_search.h`:168
- Brief: Set the contribution weight for a tenant.
- Parameters:
  - `tenant_id` (const std::string &): Identifier of the tenant.
  - `weight` (double): Input parameter.
- Details: Set Tenant Weight. Weights are multiplied into the tenant's per-document RRF contribution before cross-tenant fusion. A weight of 0.0 effectively excludes the tenant. Weights are clamped to [0.0, 1.0]. tenant_id Target tenant. weight Contribution weight in [0.0, 1.0]. tenant_id Identifier of the tenant. weight Input parameter. Calls: std::max(), std::min().

#### `size_t tenantCount() const`
- Source: `include/search/federated_search.h`:178
- Brief: Return the number of registered tenants.
- Parameters: none

### themis::FuzzyMatcher

#### `FuzzyMatcher(SecondaryIndexManager *index)`
- Source: `include/search/fuzzy_matcher.h`:73
- Brief: n/a
- Parameters:
  - `index` (SecondaryIndexManager *): Non-owning pointer to a SecondaryIndexManager. Must outlive this.
- Throws:
  - std::invalid_argument: on invalid Config.
- Details: index Non-owning pointer to a SecondaryIndexManager. Must outlive this. std::invalid_argument on invalid Config.

#### `FuzzyMatcher(SecondaryIndexManager *index, const Config &config)`
- Source: `include/search/fuzzy_matcher.h`:79
- Brief: n/a
- Parameters:
  - `index` (SecondaryIndexManager *): Non-owning pointer to a SecondaryIndexManager. Must outlive this.
  - `config` (const Config &): Fuzzy search configuration.
- Throws:
  - std::invalid_argument: on invalid Config.
- Details: index Non-owning pointer to a SecondaryIndexManager. Must outlive this. config Fuzzy search configuration. std::invalid_argument on invalid Config.

#### `double distanceToScore(int distance, size_t query_len)`
- Source: `include/search/fuzzy_matcher.h`:142
- Brief: Distance To Score.
- Parameters:
  - `distance` (int): Input parameter.
  - `query_len` (size_t): Input parameter.
- Return: Return value.
- Details: distance Input parameter. query_len Input parameter. Return value. Calls: std::max().

#### `const Config & getConfig() const`
- Source: `include/search/fuzzy_matcher.h`:135
- Brief: n/a
- Parameters: none

#### `int levenshtein(const std::string &a, const std::string &b)`
- Source: `include/search/fuzzy_matcher.h`:108
- Brief: Compute Levenshtein edit distance between two strings.
- Parameters:
  - `a` (const std::string &): Input parameter.
  - `b` (const std::string &): Input parameter.
- Return: Return value.
- Details: Levenshtein. a Input parameter. b Input parameter. Return value. Calls: size(), prev(), curr(), std::min(), std::swap().

#### `std::string metaphone(const std::string &word)`
- Source: `include/search/fuzzy_matcher.h`:121
- Brief: Compute a simple Metaphone code for a word.
- Parameters:
  - `word` (const std::string &): Input parameter.
- Return: Return value.
- Details: Metaphone. Implements a simplified (single) Metaphone that handles common English consonant transformations. word Input parameter. Return value. Calls: empty(), std::toupper(), size(), isVowel().

#### `double ngramSimilarity(const std::string &a, const std::string &b, size_t n=2)`
- Source: `include/search/fuzzy_matcher.h`:133
- Brief: Compute bigram (or n-gram) overlap similarity in [0,1].
- Parameters:
  - `a` (const std::string &): Input parameter.
  - `b` (const std::string &): Input parameter.
  - `n` (size_t): Input parameter.
- Return: Dice coefficient in [0,1].
- Details: Ngram Similarity. Uses the Dice coefficient: 2 * \|intersection\| / (\|ngrams_a\| + \|ngrams_b\|). a First string. b Second string. n N-gram size (default 2). Dice coefficient in [0,1]. a Input parameter. b Input parameter. n Input parameter. Return value. Calls: empty(), size(), insert(), substr(), ngrams(), begin(), end().

#### `std::pair< SecondaryIndexManager::Status, std::vector< FuzzyMatch > > search(const std::string &query, const std::string &table, const std::string &column, size_t limit=100) const`
- Source: `include/search/fuzzy_matcher.h`:94
- Brief: Run fuzzy search on a fulltext-indexed column.
- Parameters:
  - `query` (const std::string &): Query string (one or more tokens).
  - `table` (const std::string &): Table name (must have a fulltext index on column).
  - `column` (const std::string &): Column name.
  - `limit` (size_t): Maximum number of results.
- Return: Pair of Status and list of FuzzyMatch, sorted by score descending.
- Details: query Query string (one or more tokens). table Table name (must have a fulltext index on column). column Column name. limit Maximum number of results. Pair of Status and list of FuzzyMatch, sorted by score descending.

#### `std::string soundex(const std::string &word)`
- Source: `include/search/fuzzy_matcher.h`:113
- Brief: Compute the Soundex code for a word (American Soundex).
- Parameters:
  - `word` (const std::string &): Input parameter.
- Return: Return value.
- Details: Soundex. word Input parameter. Return value. Calls: empty(), std::toupper(), size().

### themis::HybridSearch

#### `HybridSearch(HybridSearch &&) noexcept=default`
- Source: `include/search/hybrid_search.h`:147
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridSearch &&): n/a

#### `HybridSearch(SecondaryIndexManager *fulltext_index, VectorIndexManager *vector_index, const Config &config)`
- Source: `include/search/hybrid_search.h`:136
- Brief: n/a
- Parameters:
  - `fulltext_index` (SecondaryIndexManager *): n/a
  - `vector_index` (VectorIndexManager *): n/a
  - `config` (const Config &): n/a

#### `HybridSearch(const HybridSearch &)=delete`
- Source: `include/search/hybrid_search.h`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (const HybridSearch &): n/a

#### `const Config & getConfig() const`
- Source: `include/search/hybrid_search.h`:187
- Brief: n/a
- Parameters: none

#### `void normalizeScores(std::vector< Result > &results, bool is_bm25)`
- Source: `include/search/hybrid_search.h`:227
- Brief: Normalize scores in [min, max] to [0, 1].
- Parameters:
  - `results` (std::vector< Result > &): Input/output parameter.
  - `is_bm25` (bool): Input parameter.
- Details: Normalize Scores. When all scores are equal (range == 0): score > 0 → all normalized to 1.0 score == 0 → all normalized to 0.0 results Result list whose BM25 or vector scores are modified in place. is_bm25 True to normalize bm25_score; false to normalize vector_score. results Input/output parameter. is_bm25 Input parameter. Calls: empty(), max(), lowest(), std::min(), std::max().

#### `HybridSearch & operator=(HybridSearch &&) noexcept=default`
- Source: `include/search/hybrid_search.h`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridSearch &&): n/a

#### `HybridSearch & operator=(const HybridSearch &)=delete`
- Source: `include/search/hybrid_search.h`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (const HybridSearch &): n/a

#### `std::vector< Result > reciprocalRankFusion(const std::vector< Result > &bm25_results, const std::vector< Result > &vector_results)`
- Source: `include/search/hybrid_search.h`:182
- Brief: Fuse BM25 and vector results using Reciprocal Rank Fusion.
- Parameters:
  - `bm25_results` (const std::vector< Result > &): Input parameter.
  - `vector_results` (const std::vector< Result > &): Input parameter.
- Return: Fused results sorted by hybrid score, limited to Config::k
- Details: Reciprocal Rank Fusion. RRF formula: score(d) = sum(1 / (k + rank_i(d))) where k is a constant (default 60) and rank_i is the rank in result set i. May throw std::bad_alloc; callers (search()) catch this. bm25_results BM25 search results (ordered by score descending) vector_results Vector search results (ordered by distance ascending) Fused results sorted by hybrid score, limited to Config::k bm25_results Input parameter. vector_results Input parameter. Return value. Calls: size(), empty(), push_back(), std::sort(), begin(), end(), resize(), THEMIS_INFO().

#### `std::vector< Result > search(const std::string &text_query, const float *vector_query=nullptr, size_t vector_dim=0, SearchStats *stats=nullptr)`
- Source: `include/search/hybrid_search.h`:164
- Brief: Perform hybrid search combining BM25 and vector search.
- Parameters:
  - `text_query` (const std::string &): Input parameter.
  - `vector_query` (const float *): Input parameter.
  - `vector_dim` (size_t): Input parameter.
  - `stats` (SearchStats *): Input/output parameter.
- Return: Fused results ranked by hybrid score; may be partial if one source failed (indicated by stats.partial_result == true)
- Details: Search. Internally catches all exceptions from the index backends and returns whatever partial results are available, logging errors via THEMIS_ERROR. Never throws. text_query Text query for BM25 search (pass "" to skip BM25) vector_query Optional vector query for ANN search (nullptr to skip) vector_dim Dimension of vector_query; ignored when nullptr stats Optional output: filled with per-source diagnostics Fused results ranked by hybrid score; may be partial if one source failed (indicated by stats.partial_result == true) text_query Input parameter. vector_query Input parameter. vector_dim Input parameter. stats Input/output parameter. Return value. Calls: empty(), scanFulltextWithScores(), reserve(), size(), Result(), push_back(), THEMIS_DEBUG(), THEMIS_WARN().

#### `void setAnnFrontdoor(std::shared_ptr< index::AnnFrontdoor > frontdoor)`
- Source: `include/search/hybrid_search.h`:200
- Brief: Inject an ANN frontdoor used by the vector search path.
- Parameters:
  - `frontdoor` (std::shared_ptr< index::AnnFrontdoor >): Input parameter.
- Details: Set Ann Frontdoor. When configured, HybridSearch routes dense candidate generation through AnnFrontdoor instead of calling VectorIndexManager directly. Passing nullptr disables the frontdoor path and restores the legacy vector-index fallback. frontdoor Shared ANN frontdoor instance (may be null). frontdoor Input parameter. Calls: std::move().

#### `void setConfig(const Config &config)`
- Source: `include/search/hybrid_search.h`:188
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `void setReranker(ILlmReranker::LlmBackend backend, const ILlmReranker::Config &config=ILlmReranker::Config{})`
- Source: `include/search/hybrid_search.h`:214
- Brief: Attach an LLM re-ranker to the search pipeline.
- Parameters:
  - `backend` (ILlmReranker::LlmBackend): Input parameter.
  - `config` (const ILlmReranker::Config &): Input parameter.
- Details: Set Reranker. When set, search() applies the re-ranker as a final step after RRF fusion: the top-N fused results are scored by the LLM and returned in the re-ranked order. Calling with a null backend removes any previously attached re-ranker (falls back to RRF order). backend LLM callable (same signature as LlmReranker::LlmBackend). Pass nullptr to disable. config Optional re-ranker configuration. backend Input parameter. config Input parameter. Calls: reset(), THEMIS_DEBUG(), search::createLlmReranker(), setBackend(), std::move(), release(), THEMIS_WARN().

#### `~HybridSearch() noexcept`
- Source: `include/search/hybrid_search.h`:143
- Brief: Destructor ensures no-throw guarantee.
- Parameters: none

### themis::LearningToRank

#### `LearningToRank(const Config &config=Config::defaults())`
- Source: `include/search/learning_to_rank.h`:105
- Brief: n/a
- Parameters:
  - `config` (const Config &): LTR configuration.
- Throws:
  - std::invalid_argument: on invalid config.
- Details: config LTR configuration. std::invalid_argument on invalid config.

#### `RankingFeatures addScaled(const RankingFeatures &w, const RankingFeatures &g, double lr)`
- Source: `include/search/learning_to_rank.h`:197
- Brief: Add Scaled.
- Parameters:
  - `w` (const RankingFeatures &): Input parameter.
  - `g` (const RankingFeatures &): Input parameter.
  - `lr` (double): Input parameter.
- Return: Return value.
- Details: w Input parameter. g Input parameter. lr Input parameter. Return value. Implements addScaled without additional internal calls.

#### `double dot(const RankingFeatures &w, const RankingFeatures &f)`
- Source: `include/search/learning_to_rank.h`:194
- Brief: Dot.
- Parameters:
  - `w` (const RankingFeatures &): Input parameter.
  - `f` (const RankingFeatures &): Input parameter.
- Return: Return value.
- Details: w Input parameter. f Input parameter. Return value. Implements dot without additional internal calls.

#### `const Config & getConfig() const`
- Source: `include/search/learning_to_rank.h`:185
- Brief: n/a
- Parameters: none

#### `RankingFeatures getWeights() const`
- Source: `include/search/learning_to_rank.h`:158
- Brief: Return the current feature weight vector.
- Parameters: none

#### `RankingFeatures gradient(const RankingFeatures &f_pos, const RankingFeatures &f_neg)`
- Source: `include/search/learning_to_rank.h`:195
- Brief: Gradient.
- Parameters:
  - `f_pos` (const RankingFeatures &): n/a
  - `f_neg` (const RankingFeatures &): n/a
- Return: Return value.
- Details: pos Input parameter. neg Input parameter. Return value. Implements gradient without additional internal calls.

#### `void recordClick(const ClickEvent &event)`
- Source: `include/search/learning_to_rank.h`:142
- Brief: Record a click event for later training.
- Parameters:
  - `event` (const ClickEvent &): Input parameter.
- Details: Record Click. event Input parameter. Calls: size(), erase(), begin(), push_back().

#### `void registerVariant(const Variant &variant)`
- Source: `include/search/learning_to_rank.h`:172
- Brief: Register a named scoring variant for A/B experiments.
- Parameters:
  - `variant` (const Variant &): Input parameter.
- Details: Register Variant. variant Input parameter. Calls: THEMIS_DEBUG().

#### `RankingFeatures regularize(const RankingFeatures &w, double reg)`
- Source: `include/search/learning_to_rank.h`:199
- Brief: Regularize.
- Parameters:
  - `w` (const RankingFeatures &): Input parameter.
  - `reg` (double): n/a
- Return: Return value.
- Details: w Input parameter. decay Input parameter. Return value. Implements regularize without additional internal calls.

#### `std::vector< RankedResult > rerank(std::vector< RankedResult > candidates) const`
- Source: `include/search/learning_to_rank.h`:120
- Brief: Re-rank a list of candidates using the current weight vector.
- Parameters:
  - `candidates` (std::vector< RankedResult >): Candidates with feature vectors populated.
- Return: Sorted copy of candidates with final_score filled in.
- Details: Computes final_score = w · features for each candidate and returns the list sorted by final_score descending. candidates Candidates with feature vectors populated. Sorted copy of candidates with final_score filled in.

#### `std::vector< RankedResult > rerankWithVariant(std::vector< RankedResult > candidates, const std::string &variant_name) const`
- Source: `include/search/learning_to_rank.h`:132
- Brief: Apply a specific named variant's scoring function to candidates.
- Parameters:
  - `candidates` (std::vector< RankedResult >): Candidates to score.
  - `variant_name` (const std::string &): Name of the registered variant.
- Return: Re-ranked candidates.
- Details: Returns the same sorted list as rerank() but uses the variant scorer. Falls back to the default linear model if the variant is not found. candidates Candidates to score. variant_name Name of the registered variant. Re-ranked candidates.

#### `double score(const RankingFeatures &f) const`
- Source: `include/search/learning_to_rank.h`:193
- Brief: n/a
- Parameters:
  - `f` (const RankingFeatures &): n/a

#### `std::string selectVariant(const std::string &request_key) const`
- Source: `include/search/learning_to_rank.h`:183
- Brief: Select a variant name for a given request key (deterministic hash).
- Parameters:
  - `request_key` (const std::string &): Any string uniquely identifying the request (e.g. session ID).
- Return: Name of the selected variant, or empty string for the default model.
- Details: Routes a fraction of requests to the variant based on Variant::traffic_fraction. request_key Any string uniquely identifying the request (e.g. session ID). Name of the selected variant, or empty string for the default model.

#### `void setWeights(const RankingFeatures &weights)`
- Source: `include/search/learning_to_rank.h`:163
- Brief: Set feature weights directly (e.g. to load a pre-trained model).
- Parameters:
  - `weights` (const RankingFeatures &): n/a

#### `size_t train()`
- Source: `include/search/learning_to_rank.h`:153
- Brief: Update model weights using all buffered click events.
- Parameters: none
- Return: Number of click events used for training.
- Details: Train. Implements pairwise gradient descent: for each click event, the clicked document is treated as more relevant than documents ranked above it. Clears the click buffer after training. Number of click events used for training. Return value. Calls: empty(), size(), dot(), gradient(), addScaled(), regularize(), THEMIS_DEBUG(), clear().

### themis::LearningToRank::Config

#### `Config defaults()`
- Source: `include/search/learning_to_rank.h`:89
- Brief: n/a
- Parameters: none

### themis::LlmQueryRewriter

#### `LlmQueryRewriter(LlmQueryRewriter &&) noexcept=default`
- Source: `include/search/llm_query_rewriter.h`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmQueryRewriter &&): n/a

#### `LlmQueryRewriter(const Config &config=Config::defaults(), LlmBackend backend=nullptr)`
- Source: `include/search/llm_query_rewriter.h`:124
- Brief: Construct a rewriter with optional config and LLM backend.
- Parameters:
  - `config` (const Config &): Rewriting parameters.
  - `backend` (LlmBackend): LLM backend callable; may be nullptr / empty.
- Throws:
  - std::invalid_argument: if num_rewrites == 0.
- Details: config Rewriting parameters. backend LLM backend callable; may be nullptr / empty. std::invalid_argument if num_rewrites == 0.

#### `LlmQueryRewriter(const LlmQueryRewriter &)=delete`
- Source: `include/search/llm_query_rewriter.h`:127
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LlmQueryRewriter &): n/a

#### `bool applyOverlapFilter(std::vector< std::string > &rewrites, const std::string &original) const`
- Source: `include/search/llm_query_rewriter.h`:180
- Brief: n/a
- Parameters:
  - `rewrites` (std::vector< std::string > &): n/a
  - `original` (const std::string &): n/a
- Details: Filter rewrites in-place: discard entries whose Jaccard overlap with original falls below Config::min_token_overlap_ratio. Returns true if at least one rewrite survived the filter.

#### `std::string buildPrompt(const std::string &query) const`
- Source: `include/search/llm_query_rewriter.h`:167
- Brief: Build the prompt that instructs the LLM to produce numbered rewrites.
- Parameters:
  - `query` (const std::string &): n/a

#### `const Config & getConfig() const`
- Source: `include/search/llm_query_rewriter.h`:160
- Brief: n/a
- Parameters: none

#### `float jaccardTokenOverlap(const std::string &a, const std::string &b)`
- Source: `include/search/llm_query_rewriter.h`:175
- Brief: ============================================================================ Semantic output validator helpers (Gap 2) ============================================================================
- Parameters:
  - `a` (const std::string &): Input parameter.
  - `b` (const std::string &): Input parameter.
- Return: Return value.
- Details: Compute the Jaccard overlap between the whitespace-token sets of a and b. Returns a value in [0, 1]. a Input parameter. b Input parameter. Return value.

#### `LlmQueryRewriter & operator=(LlmQueryRewriter &&) noexcept=default`
- Source: `include/search/llm_query_rewriter.h`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmQueryRewriter &&): n/a

#### `LlmQueryRewriter & operator=(const LlmQueryRewriter &)=delete`
- Source: `include/search/llm_query_rewriter.h`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LlmQueryRewriter &): n/a

#### `std::vector< std::string > parseRewrites(const std::string &llm_output, const std::string &original) const`
- Source: `include/search/llm_query_rewriter.h`:170
- Brief: Parse numbered lines (e.g. "1. some rewrite") from the LLM response.
- Parameters:
  - `llm_output` (const std::string &): n/a
  - `original` (const std::string &): n/a

#### `RewrittenQuery rewrite(const std::string &query) const`
- Source: `include/search/llm_query_rewriter.h`:158
- Brief: Rewrite a query using the LLM backend.
- Parameters:
  - `query` (const std::string &): Raw user query.
- Return: Populated RewrittenQuery.
- Details: Builds a structured prompt, calls the LLM, parses numbered lines from the response, deduplicates, and limits to Config::num_rewrites entries. Never throws; backend exceptions trigger the fallback path. query Raw user query. Populated RewrittenQuery.

#### `void setBackend(LlmBackend backend)`
- Source: `include/search/llm_query_rewriter.h`:140
- Brief: Replace the LLM backend at runtime (e.g. after model load).
- Parameters:
  - `backend` (LlmBackend): Input parameter.
- Details: Set Backend. backend New backend; pass nullptr / empty to disable LLM. backend Input parameter. Calls: std::move(), THEMIS_DEBUG().

### themis::LlmQueryRewriter::Config

#### `Config defaults()`
- Source: `include/search/llm_query_rewriter.h`:114
- Brief: n/a
- Parameters: none

### themis::LlmReranker

#### `LlmReranker(LlmReranker &&) noexcept=default`
- Source: `include/search/llm_reranker.h`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmReranker &&): n/a

#### `LlmReranker(const Config &config=Config::defaults(), LlmBackend backend=nullptr)`
- Source: `include/search/llm_reranker.h`:132
- Brief: n/a
- Parameters:
  - `config` (const Config &): Reranker configuration.
  - `backend` (LlmBackend): LLM backend callable; may be nullptr / empty.
- Throws:
  - std::invalid_argument: on invalid config values.
- Details: config Reranker configuration. backend LLM backend callable; may be nullptr / empty. std::invalid_argument on invalid config values.

#### `LlmReranker(const LlmReranker &)=delete`
- Source: `include/search/llm_reranker.h`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LlmReranker &): n/a

#### `std::string buildPrompt(const std::string &query, const std::vector< LlmRerankCandidate > &batch) const`
- Source: `include/search/llm_reranker.h`:194
- Brief: Build a prompt that asks the LLM to rate each candidate 0–10.
- Parameters:
  - `query` (const std::string &): n/a
  - `batch` (const std::vector< LlmRerankCandidate > &): n/a

#### `const Config & getConfig() const`
- Source: `include/search/llm_reranker.h`:187
- Brief: n/a
- Parameters: none

#### `LlmReranker & operator=(LlmReranker &&) noexcept=default`
- Source: `include/search/llm_reranker.h`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmReranker &&): n/a

#### `LlmReranker & operator=(const LlmReranker &)=delete`
- Source: `include/search/llm_reranker.h`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (const LlmReranker &): n/a

#### `std::vector< double > parseScores(const std::string &llm_output, size_t count) const`
- Source: `include/search/llm_reranker.h`:201
- Brief: n/a
- Parameters:
  - `llm_output` (const std::string &): n/a
  - `count` (size_t): n/a
- Details: Parse one integer score per line from the LLM response. Returns a vector of count scores in [0, 1]; missing scores default to 0.

#### `std::vector< LlmRerankResult > rerank(const std::string &query, const std::vector< LlmRerankCandidate > &candidates) const`
- Source: `include/search/llm_reranker.h`:160
- Brief: Re-rank candidates using LLM relevance feedback.
- Parameters:
  - `query` (const std::string &): The original user query shown to the LLM.
  - `candidates` (const std::vector< LlmRerankCandidate > &): Candidate results with content snippets populated.
- Return: Re-ranked candidates; may be filtered by min_score_threshold.
- Details: Splits candidates into batches of Config::batch_size, submits each batch to the LLM, parses per-document scores (0–10 integers), normalises to [0, 1], blends with initial_score, and sorts descending by final_score. Never throws; backend errors trigger the fallback path. query The original user query shown to the LLM. candidates Candidate results with content snippets populated. Re-ranked candidates; may be filtered by min_score_threshold.

#### `void setBackend(LlmBackend backend)`
- Source: `include/search/llm_reranker.h`:144
- Brief: Replace the LLM backend at runtime (e.g. after model load).
- Parameters:
  - `backend` (LlmBackend): Input parameter.
- Details: Set Backend. backend New backend; pass nullptr / empty to disable LLM. backend Input parameter. Calls: std::move(), THEMIS_DEBUG().

#### `std::vector< ClickEvent > toClickEvents(const std::string &query, const std::vector< LlmRerankResult > &results, double relevance_threshold=0.5)`
- Source: `include/search/llm_reranker.h`:181
- Brief: Convert LLM re-rank results into ClickEvents for LTR training.
- Parameters:
  - `query` (const std::string &): Input parameter.
  - `results` (const std::vector< LlmRerankResult > &): Input parameter.
  - `relevance_threshold` (double): Input parameter.
- Return: ClickEvents suitable for LearningToRank::recordClick().
- Details: To Click Events. Results whose llm_score >= relevance_threshold are treated as relevant ("clicked") at their rank position. This closes the LLM feedback loop: autoclicks=LlmReranker::toClickEvents(query,reranked,0.5); for(constauto&ev:clicks)ltr.recordClick(ev); ltr.train(); query Query string associated with the events. results Re-ranked results from rerank(). relevance_threshold Minimum llm_score to treat as relevant [0, 1]. ClickEvents suitable for LearningToRank::recordClick(). query Input parameter. results Input parameter. relevance_threshold Input parameter. Return value. Calls: size(), push_back().

### themis::LlmReranker::Config

#### `Config defaults()`
- Source: `include/search/llm_reranker.h`:124
- Brief: n/a
- Parameters: none

### themis::MultiFieldBoostedSearch

#### `MultiFieldBoostedSearch(SecondaryIndexManager *index)`
- Source: `include/search/multi_field_search.h`:100
- Brief: n/a
- Parameters:
  - `index` (SecondaryIndexManager *): Non-owning pointer to a SecondaryIndexManager. May be null (all searches will return empty results).
- Throws:
  - std::invalid_argument: on invalid config.
- Details: index Non-owning pointer to a SecondaryIndexManager. May be null (all searches will return empty results). std::invalid_argument on invalid config.

#### `MultiFieldBoostedSearch(SecondaryIndexManager *index, const Config &config)`
- Source: `include/search/multi_field_search.h`:107
- Brief: n/a
- Parameters:
  - `index` (SecondaryIndexManager *): Non-owning pointer to a SecondaryIndexManager. May be null (all searches will return empty results).
  - `config` (const Config &): Engine configuration.
- Throws:
  - std::invalid_argument: on invalid config.
- Details: index Non-owning pointer to a SecondaryIndexManager. May be null (all searches will return empty results). config Engine configuration. std::invalid_argument on invalid config.

#### `std::vector< FieldConfig > defaultFields(const std::string &table)`
- Source: `include/search/multi_field_search.h`:145
- Brief: Build the default title/body/tags field list for a table.
- Parameters:
  - `table` (const std::string &): Table name.
- Return: Default field list ordered by descending boost.
- Details: Returns three FieldConfig entries: <table>.title boost = 3.0 <table>.body boost = 1.0 <table>.tags boost = 0.5 table Table name. Default field list ordered by descending boost.

#### `const Config & getConfig() const`
- Source: `include/search/multi_field_search.h`:147
- Brief: n/a
- Parameters: none

#### `void normalizeScores(std::vector< std::pair< std::string, double > > &scored)`
- Source: `include/search/multi_field_search.h`:160
- Brief: Normalize a list of (doc_id, raw_score) pairs to [0, 1] in place.
- Parameters:
  - `scored` (std::vector< std::pair< std::string, double > > &): n/a
- Details: When all scores are equal (range == 0): score > 0 → all normalized to 1.0 score == 0 → all normalized to 0.0 Promoted to public static for direct unit testing (same pattern as HybridSearch::normalizeScores).

#### `std::vector< Result > search(const std::string &query, const std::vector< FieldConfig > &fields) const`
- Source: `include/search/multi_field_search.h`:127
- Brief: Execute a boosted multi-field search.
- Parameters:
  - `query` (const std::string &): Text query string. Empty query returns empty results.
  - `fields` (const std::vector< FieldConfig > &): Ordered list of fields to search with their boost weights. Empty list returns empty results.
- Return: Results sorted by combined score descending, limited to Config::k.
- Details: For each FieldConfig in fields, runs a BM25 fulltext query, normalizes the scores to [0, 1], multiplies by the field's boost weight, and accumulates into a per-document combined score. Documents that do not appear in a particular field's results contribute 0.0 for that field. query Text query string. Empty query returns empty results. fields Ordered list of fields to search with their boost weights. Empty list returns empty results. Results sorted by combined score descending, limited to Config::k.

#### `void setConfig(const Config &config)`
- Source: `include/search/multi_field_search.h`:148
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

### themis::MultiModalSearch

#### `MultiModalSearch(SecondaryIndexManager *sec_index, VectorIndexManager *vec_index)`
- Source: `include/search/multi_modal_search.h`:97
- Brief: n/a
- Parameters:
  - `sec_index` (SecondaryIndexManager *): Non-owning pointer to a SecondaryIndexManager. May be null if no TEXT modality queries are used.
  - `vec_index` (VectorIndexManager *): Non-owning pointer to a VectorIndexManager. May be null if no embedding modalities are used.
- Throws:
  - std::invalid_argument: on invalid config.
- Details: sec_index Non-owning pointer to a SecondaryIndexManager. May be null if no TEXT modality queries are used. vec_index Non-owning pointer to a VectorIndexManager. May be null if no embedding modalities are used. std::invalid_argument on invalid config.

#### `MultiModalSearch(SecondaryIndexManager *sec_index, VectorIndexManager *vec_index, const Config &config)`
- Source: `include/search/multi_modal_search.h`:107
- Brief: n/a
- Parameters:
  - `sec_index` (SecondaryIndexManager *): Non-owning pointer to a SecondaryIndexManager. May be null if no TEXT modality queries are used.
  - `vec_index` (VectorIndexManager *): Non-owning pointer to a VectorIndexManager. May be null if no embedding modalities are used.
  - `config` (const Config &): Search configuration.
- Throws:
  - std::invalid_argument: on invalid config.
- Details: sec_index Non-owning pointer to a SecondaryIndexManager. May be null if no TEXT modality queries are used. vec_index Non-owning pointer to a VectorIndexManager. May be null if no embedding modalities are used. config Search configuration. std::invalid_argument on invalid config.

#### `std::vector< std::pair< std::string, double > > executeModal(const ModalQuery &query, const std::string &table, const std::string &column) const`
- Source: `include/search/multi_modal_search.h`:163
- Brief: n/a
- Parameters:
  - `query` (const ModalQuery &): n/a
  - `table` (const std::string &): n/a
  - `column` (const std::string &): n/a

#### `std::vector< MultiModalResult > fuseRRF(const std::vector< std::vector< std::pair< std::string, double > > > &ranked_lists, const std::vector< double > &weights, const std::vector< std::string > &modality_names) const`
- Source: `include/search/multi_modal_search.h`:170
- Brief: n/a
- Parameters:
  - `ranked_lists` (const std::vector< std::vector< std::pair< std::string, double > > > &): n/a
  - `weights` (const std::vector< double > &): n/a
  - `modality_names` (const std::vector< std::string > &): n/a

#### `const Config & getConfig() const`
- Source: `include/search/multi_modal_search.h`:155
- Brief: n/a
- Parameters: none

#### `std::vector< MultiModalResult > search(const std::vector< ModalQuery > &queries, const std::string &table="", const std::string &column="") const`
- Source: `include/search/multi_modal_search.h`:130
- Brief: Execute a multi-modal search and return fused results.
- Parameters:
  - `queries` (const std::vector< ModalQuery > &): One or more modal query components.
  - `table` (const std::string &): Table name for TEXT modality fulltext lookup.
  - `column` (const std::string &): Column name for TEXT modality fulltext lookup.
- Return: Fused result list sorted by score descending.
- Details: Each ModalQuery in queries is executed independently: TEXT queries use SecondaryIndexManager::scanFulltextWithScores. IMAGE / AUDIO / CUSTOM queries use VectorIndexManager::searchKnn on the specified embedding_namespace. Results from all modalities are fused via RRF and the top-k returned. queries One or more modal query components. table Table name for TEXT modality fulltext lookup. column Column name for TEXT modality fulltext lookup. Fused result list sorted by score descending.

#### `std::vector< MultiModalResult > searchTextAndImage(const std::string &text_query, const std::vector< float > &image_embedding, const std::string &image_namespace, const std::string &table, const std::string &column, double text_weight=0.5, double image_weight=1.0) const`
- Source: `include/search/multi_modal_search.h`:145
- Brief: Convenience: search with a single text + single image embedding.
- Parameters:
  - `text_query` (const std::string &): BM25 query string.
  - `image_embedding` (const std::vector< float > &): Pre-computed image embedding.
  - `image_namespace` (const std::string &): VectorIndexManager object name for images.
  - `table` (const std::string &): Table name for fulltext.
  - `column` (const std::string &): Column name for fulltext.
  - `text_weight` (double): Contribution weight for the text result list.
  - `image_weight` (double): Contribution weight for the image result list.
- Details: text_query BM25 query string. image_embedding Pre-computed image embedding. image_namespace VectorIndexManager object name for images. table Table name for fulltext. column Column name for fulltext. text_weight Contribution weight for the text result list. image_weight Contribution weight for the image result list.

### themis::NegativeKeywordFilter

#### `NegativeKeywordFilter(SecondaryIndexManager *index, const Config &config)`
- Source: `include/search/negative_keyword_filter.h`:123
- Brief: Construct with a (possibly null) secondary index and optional config.
- Parameters:
  - `index` (SecondaryIndexManager *): Non-owning pointer to the SecondaryIndexManager used to look up documents containing excluded terms. May be null; all filter() calls return an error in that case.
  - `config` (const Config &): Engine configuration.
- Details: index Non-owning pointer to the SecondaryIndexManager used to look up documents containing excluded terms. May be null; all filter() calls return an error in that case. config Engine configuration.

#### `NegativeKeywordFilter(SecondaryIndexManager *index=nullptr)`
- Source: `include/search/negative_keyword_filter.h`:114
- Brief: Construct with a (possibly null) secondary index.
- Parameters:
  - `index` (SecondaryIndexManager *): Non-owning pointer to the SecondaryIndexManager used to look up documents containing excluded terms. May be null; all filter() calls return an error in that case.
- Details: index Non-owning pointer to the SecondaryIndexManager used to look up documents containing excluded terms. May be null; all filter() calls return an error in that case.

#### `std::pair< SecondaryIndexManager::Status, std::vector< std::string > > filter(const std::string &table, const std::string &column, const std::vector< std::string > &candidate_pks, const std::vector< std::string > &negative_terms) const`
- Source: `include/search/negative_keyword_filter.h`:181
- Brief: Remove from candidate_pks any document that contains an excluded term.
- Parameters:
  - `table` (const std::string &): Table name.
  - `column` (const std::string &): Fulltext-indexed column to search for excluded terms.
  - `candidate_pks` (const std::vector< std::string > &): PKs to filter (the search results before filtering).
  - `negative_terms` (const std::vector< std::string > &): Lower-case terms that must NOT appear in matching docs.
- Return: Pair of Status and the filtered PK list. On index errors the Status is Error and the returned vector may be a partial result (terms that were successfully looked up are still filtered).
- Details: For each term in negative_terms, looks up the set of documents in table.column that contain that term via the secondary index, then removes those documents from candidate_pks. An empty negative_terms list returns all candidate_pks unchanged with an OK Status. table Table name. column Fulltext-indexed column to search for excluded terms. candidate_pks PKs to filter (the search results before filtering). negative_terms Lower-case terms that must NOT appear in matching docs. Pair of Status and the filtered PK list. On index errors the Status is Error and the returned vector may be a partial result (terms that were successfully looked up are still filtered).

#### `const Config & getConfig() const`
- Source: `include/search/negative_keyword_filter.h`:193
- Brief: n/a
- Parameters: none

#### `SecondaryIndexManager * getIndex() const`
- Source: `include/search/negative_keyword_filter.h`:192
- Brief: n/a
- Parameters: none

#### `ParsedQuery parseQuery(const std::string &raw_query)`
- Source: `include/search/negative_keyword_filter.h`:157
- Brief: Parse a raw query string for positive and negative terms.
- Parameters:
  - `raw_query` (const std::string &): User-supplied query string (UTF-8).
- Return: ParsedQuery with positive_query and negative_terms.
- Details: Splits the query on whitespace. A token is treated as a negative term when it starts with a minus (-) or when the previous token was the keyword NOT (case-insensitive). All other tokens form the positive query. Minus tokens must be at least two characters long (e.g. -word); a lone - is treated as a regular positive token. NOT is consumed as an operator and does not appear in the positive query or in the negative terms list itself. Examples: "machine learning -neural" → { positive: "machine learning", negatives: ["neural"] } "database NOT crash NOT slow" → { positive: "database", negatives: ["crash", "slow"] } "search -engine NOT index" → { positive: "search", negatives: ["engine", "index"] } "hello" → { positive: "hello", negatives: [] } raw_query User-supplied query string (UTF-8). ParsedQuery with positive_query and negative_terms.

### themis::NeuralSparseRetrieval

#### `NeuralSparseRetrieval(const Config &config=Config::defaults())`
- Source: `include/search/neural_sparse_retrieval.h`:134
- Brief: n/a
- Parameters:
  - `config` (const Config &): Engine configuration.
- Throws:
  - std::invalid_argument: on invalid config (k == 0 or max_terms_per_doc == 0 or score_threshold < 0).
- Details: config Engine configuration. std::invalid_argument on invalid config (k == 0 or max_terms_per_doc == 0 or score_threshold < 0).

#### `void addDocument(const std::string &doc_id, const SparseVector &sparse_vec)`
- Source: `include/search/neural_sparse_retrieval.h`:167
- Brief: Index a document with a pre-computed sparse vector.
- Parameters:
  - `doc_id` (const std::string &): Identifier of the doc.
  - `sparse_vec` (const SparseVector &): Input parameter.
- Details: Add Document. If a document with the same doc_id already exists it is removed first, then re-indexed with the new vector. Negative term weights in sparse_vec are clamped to zero. If the number of non-zero terms exceeds Config::max_terms_per_doc, only the top-weighted terms are kept. doc_id Unique document identifier (primary key). sparse_vec Pre-computed sparse representation of the document. doc_id Identifier of the doc. sparse_vec Input parameter. Calls: empty(), THEMIS_WARN(), find(), end(), eraseFromIndex(), erase(), sanitize(), THEMIS_DEBUG().

#### `void addDocumentText(const std::string &doc_id, const std::string &text)`
- Source: `include/search/neural_sparse_retrieval.h`:179
- Brief: Encode text and index the resulting sparse vector.
- Parameters:
  - `doc_id` (const std::string &): Identifier of the doc.
  - `text` (const std::string &): Input parameter.
- Throws:
  - Any: exception propagated from the encoder backend.
- Details: Add Document Text. Calls the attached encoder backend. If no encoder is set, the call is a no-op (the document is not indexed). doc_id Unique document identifier (primary key). text Raw document text passed to the encoder. Any exception propagated from the encoder backend. doc_id Identifier of the doc. text Input parameter. Calls: THEMIS_WARN(), encoder_(), addDocument().

#### `void clear()`
- Source: `include/search/neural_sparse_retrieval.h`:193
- Brief: Remove all documents from the index.
- Parameters: none
- Details: Clear. Calls: THEMIS_DEBUG().

#### `void eraseFromIndex(const std::string &doc_id, const SparseVector &vec)`
- Source: `include/search/neural_sparse_retrieval.h`:264
- Brief: Erase From Index.
- Parameters:
  - `doc_id` (const std::string &): Identifier of the doc.
  - `vec` (const SparseVector &): Input parameter.
- Details: doc_id Identifier of the doc. vec Input parameter. Calls: find(), end(), erase(), std::remove_if(), begin(), empty().

#### `const Config & getConfig() const`
- Source: `include/search/neural_sparse_retrieval.h`:235
- Brief: n/a
- Parameters: none

#### `void insertVector(const std::string &doc_id, const SparseVector &vec)`
- Source: `include/search/neural_sparse_retrieval.h`:261
- Brief: Insert Vector.
- Parameters:
  - `doc_id` (const std::string &): Identifier of the doc.
  - `vec` (const SparseVector &): Input parameter.
- Details: doc_id Identifier of the doc. vec Input parameter. Calls: emplace_back().

#### `void normalizeScores(std::vector< Result > &results)`
- Source: `include/search/neural_sparse_retrieval.h`:248
- Brief: Normalize result scores to [0, 1] in place.
- Parameters:
  - `results` (std::vector< Result > &): Input/output parameter.
- Details: Normalize Scores. When all scores are equal (range == 0): score > 0 → all normalized to 1.0 score == 0 → all normalized to 0.0 Promoted to public static for direct unit testing (same pattern as HybridSearch::normalizeScores). results Input/output parameter. Calls: empty(), max(), lowest(), std::min(), std::max().

#### `void removeDocument(const std::string &doc_id)`
- Source: `include/search/neural_sparse_retrieval.h`:188
- Brief: Remove a document from the index.
- Parameters:
  - `doc_id` (const std::string &): Identifier of the doc.
- Details: Remove Document. If the document is not found this is a no-op. doc_id Document identifier to remove. doc_id Identifier of the doc. Calls: find(), end(), eraseFromIndex(), erase(), THEMIS_DEBUG().

#### `SparseVector sanitize(const SparseVector &raw, size_t max_terms)`
- Source: `include/search/neural_sparse_retrieval.h`:267
- Brief: Sanitize.
- Parameters:
  - `raw` (const SparseVector &): Input parameter.
  - `max_terms` (size_t): Input parameter.
- Return: Return value.
- Details: raw Input parameter. max_terms Input parameter. Return value. Calls: reserve(), size(), emplace_back(), std::partial_sort(), begin(), end(), resize(), emplace().

#### `std::vector< Result > search(const SparseVector &query_vec, size_t k=0) const`
- Source: `include/search/neural_sparse_retrieval.h`:211
- Brief: Search using a pre-computed query sparse vector.
- Parameters:
  - `query_vec` (const SparseVector &): Pre-computed sparse query representation.
  - `k` (size_t): Override for Config::k (0 means use Config::k).
- Return: Results sorted by score descending, limited to k.
- Details: Computes score(q, d) = Σ_t( q[t] * d[t] ) via inverted-index accumulation and returns the top-k results above score_threshold. Optionally normalizes scores to [0, 1] when Config::normalize_scores is true. Never throws. query_vec Pre-computed sparse query representation. k Override for Config::k (0 means use Config::k). Results sorted by score descending, limited to k.

#### `std::vector< Result > searchText(const std::string &query_text, size_t k=0) const`
- Source: `include/search/neural_sparse_retrieval.h`:224
- Brief: Encode query_text and search.
- Parameters:
  - `query_text` (const std::string &): Raw query string.
  - `k` (size_t): Override for Config::k (0 means use Config::k).
- Return: Results sorted by score descending, limited to k.
- Details: Calls the attached encoder backend to produce a sparse query vector, then delegates to search(). Returns empty when no encoder is set or when the encoder throws (exception is caught and logged). Never throws. query_text Raw query string. k Override for Config::k (0 means use Config::k). Results sorted by score descending, limited to k.

#### `void setConfig(const Config &config)`
- Source: `include/search/neural_sparse_retrieval.h`:236
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `void setEncoder(SparseEncoderBackend encoder)`
- Source: `include/search/neural_sparse_retrieval.h`:150
- Brief: Attach a sparse encoder backend.
- Parameters:
  - `encoder` (SparseEncoderBackend): Input parameter.
- Details: Set Encoder. The backend is called by addDocumentText() and searchText() to produce SparseVector representations from raw text. Passing nullptr removes any previously attached backend; subsequent calls to addDocumentText() / searchText() will return empty without throwing. encoder Callable conforming to SparseEncoderBackend. encoder Input parameter. Calls: std::move(), THEMIS_DEBUG().

#### `size_t size() const`
- Source: `include/search/neural_sparse_retrieval.h`:233
- Brief: Number of documents currently indexed.
- Parameters: none

### themis::NeuralSparseRetrieval::Config

#### `Config defaults()`
- Source: `include/search/neural_sparse_retrieval.h`:113
- Brief: n/a
- Parameters: none

### themis::PersonalizedRanker

#### `PersonalizedRanker(const Config &config=Config::defaults())`
- Source: `include/search/personalized_ranker.h`:112
- Brief: n/a
- Parameters:
  - `config` (const Config &): Ranker configuration.
- Throws:
  - std::invalid_argument: on invalid config (decay_rate < 0, max_interactions_per_user == 0, boost_weight < 0).
- Details: config Ranker configuration. std::invalid_argument on invalid config (decay_rate < 0, max_interactions_per_user == 0, boost_weight < 0).

#### `void applyPersonalization(const std::string &user_id, std::vector< RankedResult > &candidates, std::chrono::system_clock::time_point now=std::chrono::system_clock::now()) const`
- Source: `include/search/personalized_ranker.h`:162
- Brief: Apply personalization boosts to a list of ranked candidates.
- Parameters:
  - `user_id` (const std::string &): The user to personalize for.
  - `candidates` (std::vector< RankedResult > &): In-out list of ranked results; final_score is updated.
  - `now` (std::chrono::system_clock::time_point): Reference time for decay (defaults to now()).
- Details: For each candidate, final_score is incremented by Config::boost_weight * personalizationScore(user_id, doc_id). The list is then re-sorted by final_score descending. user_id The user to personalize for. candidates In-out list of ranked results; final_score is updated. now Reference time for decay (defaults to now()).

#### `void clear()`
- Source: `include/search/personalized_ranker.h`:191
- Brief: Remove all interaction history.
- Parameters: none
- Details: Clear. Calls: lock().

#### `void clearUser(const std::string &user_id)`
- Source: `include/search/personalized_ranker.h`:186
- Brief: Remove all interaction history for a specific user.
- Parameters:
  - `user_id` (const std::string &): Identifier of the user.
- Details: Clear User. user_id Identifier of the user. Calls: lock(), erase().

#### `double computeScore(const std::string &user_id, const std::string &document_id, std::chrono::system_clock::time_point now=std::chrono::system_clock::now()) const`
- Source: `include/search/personalized_ranker.h`:145
- Brief: Compute the personalization score for a (user, document) pair.
- Parameters:
  - `user_id` (const std::string &): The user to personalize for.
  - `document_id` (const std::string &): The document to score.
  - `now` (std::chrono::system_clock::time_point): Reference time for decay calculation (defaults to system clock now).
- Return: Personalization score in [-1, 1].
- Details: Returns a value in [-1, 1]: Positive: the user has shown positive interest in the document. Negative: the user has shown negative interest (disliked). Zero: no recorded interaction. user_id The user to personalize for. document_id The document to score. now Reference time for decay calculation (defaults to system clock now). Personalization score in [-1, 1].

#### `double computeScoreUnlocked(const std::string &user_id, const std::string &document_id, std::chrono::system_clock::time_point now) const`
- Source: `include/search/personalized_ranker.h`:203
- Brief: Compute personalization score without locking (caller must hold mu_).
- Parameters:
  - `user_id` (const std::string &): n/a
  - `document_id` (const std::string &): n/a
  - `now` (std::chrono::system_clock::time_point): n/a

#### `const Config & getConfig() const`
- Source: `include/search/personalized_ranker.h`:193
- Brief: n/a
- Parameters: none

#### `std::vector< UserInteraction > getUserInteractions(const std::string &user_id) const`
- Source: `include/search/personalized_ranker.h`:175
- Brief: Return all recorded interactions for a user (most-recent first).
- Parameters:
  - `user_id` (const std::string &): n/a

#### `void recordInteraction(const UserInteraction &interaction)`
- Source: `include/search/personalized_ranker.h`:125
- Brief: Record a user interaction for future personalization.
- Parameters:
  - `interaction` (const UserInteraction &): Input parameter.
- Details: Record Interaction. If the per-user buffer is full, the oldest interaction is evicted. interaction Interaction to record. interaction Input parameter. Calls: lock(), size(), erase(), begin(), push_back(), THEMIS_DEBUG().

#### `double typeWeight(InteractionType type)`
- Source: `include/search/personalized_ranker.h`:201
- Brief: Type Weight.
- Parameters:
  - `type` (InteractionType): Input parameter.
- Return: Return value.
- Details: type Input parameter. Return value. Implements typeWeight without additional internal calls.

#### `size_t userCount() const`
- Source: `include/search/personalized_ranker.h`:181
- Brief: Return the number of distinct users with interaction history.
- Parameters: none

### themis::PersonalizedRanker::Config

#### `Config defaults()`
- Source: `include/search/personalized_ranker.h`:104
- Brief: n/a
- Parameters: none

### themis::QueryExpander

#### `QueryExpander(QueryExpander &&) noexcept=default`
- Source: `include/search/query_expander.h`:94
- Brief: n/a
- Parameters:
  - `<unnamed>` (QueryExpander &&): n/a

#### `QueryExpander(const Config &config=Config::defaults())`
- Source: `include/search/query_expander.h`:89
- Brief: Construct a QueryExpander with the given config.
- Parameters:
  - `config` (const Config &): n/a
- Throws:
  - std::invalid_argument: if max_edit_distance < 0 or max_expansions == 0.
- Details: std::invalid_argument if max_edit_distance < 0 or max_expansions == 0.

#### `QueryExpander(const QueryExpander &)=delete`
- Source: `include/search/query_expander.h`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (const QueryExpander &): n/a

#### `void addSynonyms(const std::string &term, const std::vector< std::string > &synonyms)`
- Source: `include/search/query_expander.h`:107
- Brief: Register synonyms for a term.
- Parameters:
  - `term` (const std::string &): Input parameter.
  - `synonyms` (const std::vector< std::string > &): Input parameter.
- Details: Add Synonyms. term The canonical term (will be lowercased). synonyms Equivalent terms/phrases for term. term Input parameter. synonyms Input parameter. Calls: toLower(), std::find(), begin(), end(), push_back().

#### `void addVocabulary(const std::vector< std::string > &words)`
- Source: `include/search/query_expander.h`:114
- Brief: Add words to the vocabulary used for spelling correction.
- Parameters:
  - `words` (const std::vector< std::string > &): Input parameter.
- Details: Add Vocabulary. Words already in the vocabulary are ignored (idempotent). words Input parameter. Calls: insert(), toLower().

#### `void addVocabularyWithFrequencies(const std::vector< std::pair< std::string, size_t > > &words_with_frequencies)`
- Source: `include/search/query_expander.h`:128
- Brief: Add words with associated frequency counts to the vocabulary.
- Parameters:
  - `words_with_frequencies` (const std::vector< std::pair< std::string, size_t > > &): Pairs of (word, frequency_count). Frequency counts must be > 0; zero values are ignored.
- Details: Registers each word in the spelling-correction vocabulary and records its corpus frequency. When multiple candidates share the same edit distance, higher-frequency words are ranked first and receive a higher confidence score. Calling this function multiple times for the same word accumulates the frequency counts. words_with_frequencies Pairs of (word, frequency_count). Frequency counts must be > 0; zero values are ignored.

#### `std::string correctSpelling(const std::string &word) const`
- Source: `include/search/query_expander.h`:153
- Brief: Return the best spelling correction for a single word.
- Parameters:
  - `word` (const std::string &): Single word to correct (must not contain spaces).
- Return: Corrected word, or word unchanged if no correction found.
- Details: Searches the registered vocabulary for the closest word within Config::max_edit_distance. Returns the original word if no close match is found. word Single word to correct (must not contain spaces). Corrected word, or word unchanged if no correction found.

#### `int editDistance(const std::string &a, const std::string &b)`
- Source: `include/search/query_expander.h`:232
- Brief: Edit Distance.
- Parameters:
  - `a` (const std::string &): Input parameter.
  - `b` (const std::string &): Input parameter.
- Return: Return value.
- Details: a Input parameter. b Input parameter. Return value. Calls: size(), std::abs(), prev(), curr(), std::min(), std::swap().

#### `ExpandedQuery expand(const std::string &query) const`
- Source: `include/search/query_expander.h`:141
- Brief: Expand a user query: tokenize, correct spelling, add synonyms.
- Parameters:
  - `query` (const std::string &): Raw user input.
- Return: Populated ExpandedQuery. Never throws.
- Details: query Raw user input. Populated ExpandedQuery. Never throws.

#### `const Config & getConfig() const`
- Source: `include/search/query_expander.h`:215
- Brief: n/a
- Parameters: none

#### `QueryExpander & operator=(QueryExpander &&) noexcept=default`
- Source: `include/search/query_expander.h`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (QueryExpander &&): n/a

#### `QueryExpander & operator=(const QueryExpander &)=delete`
- Source: `include/search/query_expander.h`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (const QueryExpander &): n/a

#### `std::string relaxQuery(const std::string &query) const`
- Source: `include/search/query_expander.h`:213
- Brief: Build a relaxed (OR-logic) query for zero-result fallback.
- Parameters:
  - `query` (const std::string &): Original query.
- Return: Relaxed query with one token fewer, or empty string for single-token input.
- Details: Drops the least-discriminative token from the query, returning a shorter query that is more likely to match. query Original query. Relaxed query with one token fewer, or empty string for single-token input.

#### `std::vector< std::string > suggestAlternatives(const std::string &query) const`
- Source: `include/search/query_expander.h`:202
- Brief: Suggest alternative phrasings for the whole query.
- Parameters:
  - `query` (const std::string &): Original query string.
- Return: List of alternative query strings (may be empty).
- Details: Returns up to Config::max_expansions alternative queries formed by replacing one token at a time with its synonyms. query Original query string. List of alternative query strings (may be empty).

#### `std::vector< SpellingCorrection > suggestQueryCorrections(const std::string &query, size_t max_suggestions=5) const`
- Source: `include/search/query_expander.h`:189
- Brief: Return ranked full-query spelling correction suggestions.
- Parameters:
  - `query` (const std::string &): Raw user query (may contain multiple tokens).
  - `max_suggestions` (size_t): Maximum number of full-query suggestions (default 5).
- Return: Ranked list of SpellingCorrection structs where suggestion is a full query string; may be empty.
- Details: Tokenizes the query, independently gathers up to max_suggestions corrections per token, and returns full-query strings formed by substituting one token at a time with its best correction. Results are sorted by total edit distance (ascending). An empty list is returned when no token needs correction. query Raw user query (may contain multiple tokens). max_suggestions Maximum number of full-query suggestions (default 5). Ranked list of SpellingCorrection structs where suggestion is a full query string; may be empty.

#### `std::vector< SpellingCorrection > suggestSpellingCorrections(const std::string &word, size_t max_suggestions=5) const`
- Source: `include/search/query_expander.h`:170
- Brief: Return ranked spelling correction candidates for a single word.
- Parameters:
  - `word` (const std::string &): Single word to find corrections for.
  - `max_suggestions` (size_t): Maximum number of candidates to return (default 5).
- Return: Ranked list of SpellingCorrection candidates; may be empty.
- Details: Returns up to max_suggestions candidates from the registered vocabulary, sorted by ascending edit distance then alphabetically for stability. Each candidate carries a normalized confidence score: the word with edit distance 1 has higher confidence than one with edit distance 2, etc. An empty list is returned when the word is already in the vocabulary, spelling correction is disabled, or the vocabulary is empty. word Single word to find corrections for. max_suggestions Maximum number of candidates to return (default 5). Ranked list of SpellingCorrection candidates; may be empty.

#### `std::string toLower(const std::string &s)`
- Source: `include/search/query_expander.h`:231
- Brief: To Lower.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: std::tolower().

#### `std::vector< std::string > tokenize(const std::string &text)`
- Source: `include/search/query_expander.h`:230
- Brief: Tokenize.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Calls: iss(), std::isalnum(), std::tolower(), empty(), push_back().

### themis::QueryExpander::Config

#### `Config defaults()`
- Source: `include/search/query_expander.h`:82
- Brief: n/a
- Parameters: none

### themis::SearchAnalytics

#### `SearchAnalytics(const Config &config=Config::defaults())`
- Source: `include/search/search_analytics.h`:91
- Brief: n/a
- Parameters:
  - `config` (const Config &): n/a

#### `void clear()`
- Source: `include/search/search_analytics.h`:155
- Brief: Clear all retained events.
- Parameters: none
- Details: Clear. Calls: lock().

#### `SearchMetrics computeMetrics() const`
- Source: `include/search/search_analytics.h`:145
- Brief: Compute aggregated metrics over all retained events.
- Parameters: none
- Details: Computing metrics is O(n) in the number of retained events.

#### `size_t eventCount() const`
- Source: `include/search/search_analytics.h`:150
- Brief: Return the number of currently retained events.
- Parameters: none

#### `const Config & getConfig() const`
- Source: `include/search/search_analytics.h`:157
- Brief: n/a
- Parameters: none

#### `std::vector< SearchEvent > getRecentEvents(size_t limit=100) const`
- Source: `include/search/search_analytics.h`:125
- Brief: Return the N most recent recorded events.
- Parameters:
  - `limit` (size_t): Number of events to return (most recent first).
- Details: limit Number of events to return (most recent first).

#### `std::vector< std::pair< std::string, size_t > > getTopQueries(size_t limit=20) const`
- Source: `include/search/search_analytics.h`:138
- Brief: Return the top queries ranked by frequency.
- Parameters:
  - `limit` (size_t): Maximum number of entries to return (default: 20).
- Return: Vector of (query, count) pairs, most frequent first.
- Details: Returns up to limit query strings with their occurrence counts, sorted by descending frequency (most popular first). This is a lightweight alternative to computeMetrics() when only the top-query list is needed. limit Maximum number of entries to return (default: 20). Vector of (query, count) pairs, most frequent first.

#### `std::vector< SearchEvent > getZeroResultQueries(size_t limit=100) const`
- Source: `include/search/search_analytics.h`:118
- Brief: Return the most recently recorded zero-result queries.
- Parameters:
  - `limit` (size_t): Maximum number of events to return (most recent first).
- Return: List of SearchEvent where result_count == 0.
- Details: limit Maximum number of events to return (most recent first). List of SearchEvent where result_count == 0.

#### `void record(const std::string &query, size_t result_count, double latency_ms)`
- Source: `include/search/search_analytics.h`:104
- Brief: Record a completed search event.
- Parameters:
  - `query` (const std::string &): Input parameter.
  - `result_count` (size_t): Input parameter.
  - `latency_ms` (double): Input parameter.
- Details: Record. query The original search query. result_count Number of results returned. latency_ms Search latency in milliseconds. query Input parameter. result_count Input parameter. latency_ms Input parameter. Calls: std::chrono::system_clock::now(), lock(), size(), erase(), begin(), push_back(), std::move(), THEMIS_WARN().

### themis::SearchAnalytics::Config

#### `Config defaults()`
- Source: `include/search/search_analytics.h`:88
- Brief: n/a
- Parameters: none

### themis::SearchHighlighter

#### `SearchHighlighter()`
- Source: `include/search/search_highlighter.h`:41
- Brief: n/a
- Parameters: none

#### `SearchHighlighter(Config config)`
- Source: `include/search/search_highlighter.h`:42
- Brief: n/a
- Parameters:
  - `config` (Config): n/a

#### `std::string applyHighlight(const std::string &text, const std::vector< std::pair< size_t, size_t > > &offsets, const std::string &open_tag, const std::string &close_tag)`
- Source: `include/search/search_highlighter.h`:105
- Brief: Apply highlight tags at the given byte offsets inside text.
- Parameters:
  - `text` (const std::string &): Source text.
  - `offsets` (const std::vector< std::pair< size_t, size_t > > &): Sorted, non-overlapping [start,end) byte ranges.
  - `open_tag` (const std::string &): Opening highlight tag.
  - `close_tag` (const std::string &): Closing highlight tag.
- Return: text with tags inserted at the specified ranges.
- Details: offsets is a list of [start, end) byte pairs describing the byte ranges to wrap. Ranges must be non-overlapping and sorted by start. text Source text. offsets Sorted, non-overlapping [start,end) byte ranges. open_tag Opening highlight tag. close_tag Closing highlight tag. text with tags inserted at the specified ranges.

#### `size_t bestWindowOffset(const std::string &text, const std::vector< std::string > &terms, size_t window_size)`
- Source: `include/search/search_highlighter.h`:124
- Brief: Find the byte offset that maximises term coverage in a window.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `terms` (const std::vector< std::string > &): Input parameter.
  - `window_size` (size_t): Input parameter.
- Return: Start byte offset of the best window, or 0 if text is shorter than window_size.
- Details: Best Window Offset. Scans text with a sliding window of window_size bytes and returns the start offset that covers the greatest number of distinct term occurrences. Ties are broken by preferring the earliest window. text Source text. terms Lowercase search terms. window_size Window width in bytes. Start byte offset of the best window, or 0 if text is shorter than window_size. text Input parameter. terms Input parameter. window_size Input parameter. Return value. Calls: empty(), size(), reserve(), std::tolower(), find(), push_back(), std::sort(), begin().

#### `std::vector< std::pair< size_t, size_t > > findMatchRanges(const std::string &text, const std::vector< std::string > &terms) const`
- Source: `include/search/search_highlighter.h`:133
- Brief: Return sorted, merged [start,end) match ranges for terms in text.
- Parameters:
  - `text` (const std::string &): n/a
  - `terms` (const std::vector< std::string > &): n/a

#### `std::string highlight(const std::string &text, const std::vector< std::string > &terms) const noexcept`
- Source: `include/search/search_highlighter.h`:58
- Brief: Highlight all occurrences of terms inside text.
- Parameters:
  - `text` (const std::string &): Source document text (UTF-8).
  - `terms` (const std::vector< std::string > &): Search terms to highlight.
- Return: text with matched terms wrapped in highlight tags, or an empty string when text is empty.
- Details: Each token in terms that appears in text is wrapped with the configured highlight_open / highlight_close tags. Overlapping matches are merged so that a single term never receives nested tags. text Source document text (UTF-8). terms Search terms to highlight. text with matched terms wrapped in highlight tags, or an empty string when text is empty.

#### `std::string snippet(const std::string &text, const std::vector< std::string > &terms, size_t window_size=0) const noexcept`
- Source: `include/search/search_highlighter.h`:74
- Brief: Extract the best-matching passage from text.
- Parameters:
  - `text` (const std::string &): Source document text (UTF-8).
  - `terms` (const std::vector< std::string > &): Search terms to locate.
  - `window_size` (size_t): Passage width in characters (default: Config::max_snippet_len).
- Return: Best matching snippet, or the first window_size characters of text when no terms are found.
- Details: A sliding window of window_size characters is scored by the number of distinct search terms it contains. The highest-scoring window is returned with matched terms highlighted and boundary ellipses appended. text Source document text (UTF-8). terms Search terms to locate. window_size Passage width in characters (default: Config::max_snippet_len). Best matching snippet, or the first window_size characters of text when no terms are found.

#### `std::vector< std::string > tokenize(const std::string &text, bool case_insensitive=true)`
- Source: `include/search/search_highlighter.h`:90
- Brief: Tokenise text into a vector of lowercase tokens.
- Parameters:
  - `text` (const std::string &): Input parameter.
  - `case_insensitive` (bool): Input parameter.
- Return: Vector of tokens in order of appearance.
- Details: Tokenize. Splits on whitespace and standard ASCII punctuation. Non-ASCII bytes are preserved inside tokens (multi-byte UTF-8 sequences are not split). text Input string. case_insensitive When true, tokens are lowercased (ASCII only). Vector of tokens in order of appearance. text Input parameter. case_insensitive Input parameter. Return value. Calls: std::isspace(), std::ispunct(), empty(), push_back(), std::move(), std::isupper(), std::tolower().

### themis::SearchResultStream

#### `SearchResultStream(HybridSearch *hybrid_search)`
- Source: `include/search/search_result_stream.h`:115
- Brief: Construct a SearchResultStream engine.
- Parameters:
  - `hybrid_search` (HybridSearch *): Non-owning pointer to the underlying HybridSearch. May be null; open() will return an empty stream.
- Throws:
  - std::invalid_argument: on invalid config values.
- Details: hybrid_search Non-owning pointer to the underlying HybridSearch. May be null; open() will return an empty stream. std::invalid_argument on invalid config values.

#### `SearchResultStream(HybridSearch *hybrid_search, const Config &config)`
- Source: `include/search/search_result_stream.h`:124
- Brief: Construct a SearchResultStream engine.
- Parameters:
  - `hybrid_search` (HybridSearch *): Non-owning pointer to the underlying HybridSearch. May be null; open() will return an empty stream.
  - `config` (const Config &): Engine configuration.
- Throws:
  - std::invalid_argument: on invalid config values.
- Details: hybrid_search Non-owning pointer to the underlying HybridSearch. May be null; open() will return an empty stream. config Engine configuration. std::invalid_argument on invalid config values.

#### `void close()`
- Source: `include/search/search_result_stream.h`:170
- Brief: Clear the buffered results and reset the cursor.
- Parameters: none
- Details: Close. After calling close(), hasMore() returns false until open() is called again. Calls: clear().

#### `size_t cursorPosition() const`
- Source: `include/search/search_result_stream.h`:196
- Brief: Current cursor position.
- Parameters: none

#### `void forEachResult(ResultCallback callback)`
- Source: `include/search/search_result_stream.h`:186
- Brief: Deliver each result to callback starting from the current cursor position.
- Parameters:
  - `callback` (ResultCallback): Input parameter.
- Details: For Each Result. Advances the cursor to the end of the result set (or stops early when the callback returns false). Never throws; callback exceptions are caught and iteration is stopped. callback Invoked for each result; return false to stop early. callback Input parameter. Calls: size(), callback(), THEMIS_ERROR(), what().

#### `const Config & getConfig() const`
- Source: `include/search/search_result_stream.h`:198
- Brief: n/a
- Parameters: none

#### `bool hasMore() const`
- Source: `include/search/search_result_stream.h`:157
- Brief: Return true when there are more results beyond the current cursor.
- Parameters: none

#### `std::vector< HybridSearch::Result > nextPage()`
- Source: `include/search/search_result_stream.h`:152
- Brief: Advance the cursor and return the next page of results.
- Parameters: none
- Return: Return value.
- Details: Next Page. Returns up to Config::page_size results starting at the current cursor position, then advances the cursor by the number of results returned. Returns an empty vector when hasMore() is false. Return value. Calls: hasMore(), std::min(), size(), page(), begin().

#### `void open(const std::string &query, const std::vector< float > &vector_query={})`
- Source: `include/search/search_result_stream.h`:142
- Brief: Open the stream for a new query.
- Parameters:
  - `query` (const std::string &): Input parameter.
  - `vector_query` (const std::vector< float > &): Optional semantic embedding vector.
- Details: Open. Executes HybridSearch::search() with k = Config::total_k, stores the full result vector internally, and resets the cursor to position 0. Any previously open stream is discarded. Never throws; index failures result in an empty stream. query Full-text query string. vector_query Optional semantic embedding vector. query Input parameter. param Input parameter. Calls: close(), empty(), getConfig(), setConfig(), std::chrono::steady_clock::now(), search(), count(), THEMIS_WARN().

#### `void reset()`
- Source: `include/search/search_result_stream.h`:162
- Brief: Rewind the cursor to position 0 without re-issuing the query.
- Parameters: none
- Details: Reset the modification detection flag. Implements reset without additional internal calls.

#### `void setConfig(const Config &config)`
- Source: `include/search/search_result_stream.h`:199
- Brief: Set Config.
- Parameters:
  - `config` (const Config &): Input parameter.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: config Input parameter. std::invalid_argument if an error occurs. Implements setConfig without additional internal calls.

#### `size_t totalResults() const`
- Source: `include/search/search_result_stream.h`:193
- Brief: Total results in the current stream (0 when not open).
- Parameters: none

### themis::search

#### `std::unique_ptr< ILlmReranker > createLlmReranker(const ILlmReranker::Config &cfg)`
- Source: `src/search/llm_reranker_factory_stub.cpp`:24
- Brief: Create Llm Reranker.
- Parameters:
  - `cfg` (const ILlmReranker::Config &): Input parameter.
- Return: Return value.
- Details: cfg Input parameter. Return value. Calls: g_reranker_factory().

#### `void registerLlmRerankerFactory(LlmRerankerFactory f)`
- Source: `src/search/llm_reranker_factory_stub.cpp`:16
- Brief: Register Llm Reranker Factory.
- Parameters:
  - `f` (LlmRerankerFactory): Input parameter.
- Details: f Input parameter. Calls: std::move().

### themis::search::LayeredRetrievalOrchestrator

#### `LayeredRetrievalOrchestrator(LayeredRetrievalConfig config=LayeredRetrievalConfig{})`
- Source: `include/search/layered_retrieval_orchestrator.h`:187
- Brief: Construct an orchestrator with the given configuration.
- Parameters:
  - `config` (LayeredRetrievalConfig): Layer enablement, timeouts, and guardrails.
- Details: config Layer enablement, timeouts, and guardrails.

#### `LayeredRetrievalResult execute(const LayeredRetrievalContext &context) const`
- Source: `include/search/layered_retrieval_orchestrator.h`:241
- Brief: Execute the configured four-layer retrieval chain.
- Parameters:
  - `context` (const LayeredRetrievalContext &): Query payload and layer-specific lookup keys.
- Return: Unified retrieval result with layer outputs and diagnostics.
- Details: The orchestrator never throws. Layer failures are surfaced through LayeredRetrievalResult::routing_decisions and LayeredRetrievalResult::diagnostics. context Query payload and layer-specific lookup keys. Unified retrieval result with layer outputs and diagnostics.

#### `const LayeredRetrievalConfig & getConfig() const noexcept`
- Source: `include/search/layered_retrieval_orchestrator.h`:229
- Brief: Return the active runtime configuration.
- Parameters: none

#### `void setAnnIndex(std::shared_ptr< AdvancedVectorIndex > index)`
- Source: `include/search/layered_retrieval_orchestrator.h`:194
- Brief: Inject the ANN backend used by the first retrieval layer.
- Parameters:
  - `index` (std::shared_ptr< AdvancedVectorIndex >): Input parameter.
- Details: Set Ann Index. index Shared AdvancedVectorIndex instance; null disables the ANN layer. index Input parameter. Calls: std::move().

#### `void setConfig(const LayeredRetrievalConfig &config)`
- Source: `include/search/layered_retrieval_orchestrator.h`:224
- Brief: Replace the current runtime configuration.
- Parameters:
  - `config` (const LayeredRetrievalConfig &): Input parameter.
- Details: Set Config. config New layer/time/guardrail configuration. config Input parameter. Implements setConfig without additional internal calls.

#### `void setGraphReasoner(std::shared_ptr< graph::KnowledgeGraphReasoner > reasoner)`
- Source: `include/search/layered_retrieval_orchestrator.h`:206
- Brief: Inject the knowledge-graph reasoner used by the provenance layer.
- Parameters:
  - `reasoner` (std::shared_ptr< graph::KnowledgeGraphReasoner >): Input parameter.
- Details: Set Graph Reasoner. reasoner Shared KnowledgeGraphReasoner instance; null disables the graph layer. reasoner Input parameter. Calls: std::move().

#### `void setLlmClient(std::shared_ptr< llm::LLMClient > client)`
- Source: `include/search/layered_retrieval_orchestrator.h`:212
- Brief: Inject the LLM client used by the final answer layer.
- Parameters:
  - `client` (std::shared_ptr< llm::LLMClient >): Input parameter.
- Details: Set Llm Client. client Shared LLMClient instance; null disables the LLM layer. client Input parameter. Calls: std::move().

#### `void setTensorGraph(std::shared_ptr< tensor::TensorFingerprintGraph > graph)`
- Source: `include/search/layered_retrieval_orchestrator.h`:200
- Brief: Inject the tensor fingerprint graph used by the tensor layer.
- Parameters:
  - `graph` (std::shared_ptr< tensor::TensorFingerprintGraph >): Input parameter.
- Details: Set Tensor Graph. graph Shared TensorFingerprintGraph instance; null disables the tensor layer. graph Input parameter. Calls: std::move().

#### `void setTracer(std::shared_ptr< core::concerns::ITracer > tracer)`
- Source: `include/search/layered_retrieval_orchestrator.h`:218
- Brief: Inject the tracer used for per-layer span emission.
- Parameters:
  - `tracer` (std::shared_ptr< core::concerns::ITracer >): Input parameter.
- Details: Set Tracer. tracer Shared tracer; null disables tracing. tracer Input parameter. Calls: std::move().

### themis::search::testing

#### `TEST(DistributedMergePhase2, P2_01_AllShardsSuccessful)`
- Source: `tests/search/test_search_distributed_merge_phase2.cpp`:35
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMergePhase2): n/a
  - `<unnamed>` (P2_01_AllShardsSuccessful): n/a
- Details: P2-01: Basic merge with all shards successful Verifies degradation flags remain false when no failures occur.

#### `TEST(DistributedMergePhase2, P2_02_MergeUnderflow)`
- Source: `tests/search/test_search_distributed_merge_phase2.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMergePhase2): n/a
  - `<unnamed>` (P2_02_MergeUnderflow): n/a
- Details: P2-02: Merge underflow detection (insufficient candidates) Verifies merge_underflow flag when result count < k.

#### `TEST(DistributedMergePhase2, P2_03_HighOverlapVariance)`
- Source: `tests/search/test_search_distributed_merge_phase2.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMergePhase2): n/a
  - `<unnamed>` (P2_03_HighOverlapVariance): n/a
- Details: P2-03: High overlap variance detection Verifies detection when same document appears in most shards.

#### `TEST(DistributedMergePhase2, P2_04_ShardFailureReasons)`
- Source: `tests/search/test_search_distributed_merge_phase2.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMergePhase2): n/a
  - `<unnamed>` (P2_04_ShardFailureReasons): n/a
- Details: P2-04: Shard failure with failed_shard_reasons tracking Verifies failed_shard_reasons populated for operator diagnostics.

#### `TEST(DistributedMergePhase2, P2_05_ExactKCandidates)`
- Source: `tests/search/test_search_distributed_merge_phase2.cpp`:198
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMergePhase2): n/a
  - `<unnamed>` (P2_05_ExactKCandidates): n/a
- Details: P2-05: K-limit edge case - exact k candidates Verifies no underflow when exactly k documents available.

#### `TEST(DistributedMergePhase2, P2_06_PartialResultDetection)`
- Source: `tests/search/test_search_distributed_merge_phase2.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMergePhase2): n/a
  - `<unnamed>` (P2_06_PartialResultDetection): n/a
- Details: P2-06: Partial result detection across degradation flags Verifies proper coordination of partial_result with failed shard reasons.

#### `TEST(DistributedMergePhase2, P2_07_BoundedResourceEnforcement)`
- Source: `tests/search/test_search_distributed_merge_phase2.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMergePhase2): n/a
  - `<unnamed>` (P2_07_BoundedResourceEnforcement): n/a
- Details: P2-07: Bounded resource enforcement - large result set cap Verifies k-limit enforcement with large shard result sets.

#### `TEST(DistributedMergePhase2, P2_08_ConcurrentShardFailures)`
- Source: `tests/search/test_search_distributed_merge_phase2.cpp`:313
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedMergePhase2): n/a
  - `<unnamed>` (P2_08_ConcurrentShardFailures): n/a
- Details: P2-08: Deterministic stress - concurrent shard failures Verifies stable behavior under multiple concurrent shard failures.

#### `TEST(HybridSearchDegradationPhase2, P2H_01_SearchStatsStructure)`
- Source: `tests/search/test_search_hybrid_degradation_phase2.cpp`:27
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridSearchDegradationPhase2): n/a
  - `<unnamed>` (P2H_01_SearchStatsStructure): n/a
- Details: P2H-01: SearchStats structure contains degradation flags Verifies that SearchStats has all Phase 2 fields.

#### `TEST(HybridSearchDegradationPhase2, P2H_02_ErrorCodeConstants)`
- Source: `tests/search/test_search_hybrid_degradation_phase2.cpp`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridSearchDegradationPhase2): n/a
  - `<unnamed>` (P2H_02_ErrorCodeConstants): n/a
- Details: P2H-02: Error code constants from search_error_codes.h Verifies that common error codes are defined and accessible.

#### `TEST(HybridSearchDegradationPhase2, P2H_03_PrimaryErrorCodeTracking)`
- Source: `tests/search/test_search_hybrid_degradation_phase2.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridSearchDegradationPhase2): n/a
  - `<unnamed>` (P2H_03_PrimaryErrorCodeTracking): n/a
- Details: P2H-03: Primary error code tracking Verifies that primary_error_code field can hold meaningful error codes.

#### `TEST(HybridSearchDegradationPhase2, P2H_04_DegradationScenarios)`
- Source: `tests/search/test_search_hybrid_degradation_phase2.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (HybridSearchDegradationPhase2): n/a
  - `<unnamed>` (P2H_04_DegradationScenarios): n/a
- Details: P2H-04: Degradation flag scenarios Verifies distinct degradation flag combinations.

#### `TEST(SearchEdgeCasesPhase3, DegradationFlagIndependence)`
- Source: `tests/search/test_search_edge_cases_phase3.cpp`:141
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchEdgeCasesPhase3): n/a
  - `<unnamed>` (DegradationFlagIndependence): n/a

#### `TEST(SearchEdgeCasesPhase3, ErrorCodeRangeFiltering)`
- Source: `tests/search/test_search_edge_cases_phase3.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchEdgeCasesPhase3): n/a
  - `<unnamed>` (ErrorCodeRangeFiltering): n/a

#### `TEST(SearchEdgeCasesPhase3, P3_01_EmptyResultSetHybridFusion)`
- Source: `tests/search/test_search_edge_cases_phase3.cpp`:18
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchEdgeCasesPhase3): n/a
  - `<unnamed>` (P3_01_EmptyResultSetHybridFusion): n/a

#### `TEST(SearchEdgeCasesPhase3, P3_02_ShardTimeoutDegradation)`
- Source: `tests/search/test_search_edge_cases_phase3.cpp`:32
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchEdgeCasesPhase3): n/a
  - `<unnamed>` (P3_02_ShardTimeoutDegradation): n/a

#### `TEST(SearchEdgeCasesPhase3, P3_03_KLimitUnderflowHighOverlap)`
- Source: `tests/search/test_search_edge_cases_phase3.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchEdgeCasesPhase3): n/a
  - `<unnamed>` (P3_03_KLimitUnderflowHighOverlap): n/a

#### `TEST(SearchEdgeCasesPhase3, P3_04_AllShardsFailed)`
- Source: `tests/search/test_search_edge_cases_phase3.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchEdgeCasesPhase3): n/a
  - `<unnamed>` (P3_04_AllShardsFailed): n/a

#### `TEST(SearchEdgeCasesPhase3, P3_05_RerankerFallback)`
- Source: `tests/search/test_search_edge_cases_phase3.cpp`:67
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchEdgeCasesPhase3): n/a
  - `<unnamed>` (P3_05_RerankerFallback): n/a

#### `TEST(SearchEdgeCasesPhase3, P3_06_ExpansionLimitExceeded)`
- Source: `tests/search/test_search_edge_cases_phase3.cpp`:79
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchEdgeCasesPhase3): n/a
  - `<unnamed>` (P3_06_ExpansionLimitExceeded): n/a

#### `TEST(SearchEdgeCasesPhase3, P3_07_FusionFailureWithFallback)`
- Source: `tests/search/test_search_edge_cases_phase3.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchEdgeCasesPhase3): n/a
  - `<unnamed>` (P3_07_FusionFailureWithFallback): n/a

#### `TEST(SearchEdgeCasesPhase3, P3_08_ConcurrentLayerFailures)`
- Source: `tests/search/test_search_edge_cases_phase3.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchEdgeCasesPhase3): n/a
  - `<unnamed>` (P3_08_ConcurrentLayerFailures): n/a

#### `TEST(SearchEdgeCasesPhase3, SearchResultStreamTimeout)`
- Source: `tests/search/test_search_edge_cases_phase3.cpp`:121
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchEdgeCasesPhase3): n/a
  - `<unnamed>` (SearchResultStreamTimeout): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_01_StreamBufferExhaustion)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_01_StreamBufferExhaustion): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_02_StreamBufferOverflowDropOldest)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_02_StreamBufferOverflowDropOldest): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_03_OpenEndedStreamTimeout)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_03_OpenEndedStreamTimeout): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_04_StreamTimeoutRecoveryPreservesState)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_04_StreamTimeoutRecoveryPreservesState): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_05_ConcurrentReaderSynchronization)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_05_ConcurrentReaderSynchronization): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_06_ConcurrentReaderDeadlockPrevention)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_06_ConcurrentReaderDeadlockPrevention): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_07_MetricAggregationBackpressure)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_07_MetricAggregationBackpressure): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_08_MetricAggregationOverflowDetection)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_08_MetricAggregationOverflowDetection): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_09_EventBatchingWindowBoundaryRace)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:311
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_09_EventBatchingWindowBoundaryRace): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_10_EventBatchingTimeoutFlush)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:349
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_10_EventBatchingTimeoutFlush): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_11_MetricCardinalityExplosion)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:378
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_11_MetricCardinalityExplosion): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_12_MetricCardinalityAutoPruning)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:411
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_12_MetricCardinalityAutoPruning): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_13_AnalyticsCorruptionChecksum)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:446
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_13_AnalyticsCorruptionChecksum): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_14_AnalyticsSequenceNumberIntegrity)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:481
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_14_AnalyticsSequenceNumberIntegrity): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_15_AnalyticsRecoveryPartialFlush)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:511
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_15_AnalyticsRecoveryPartialFlush): n/a

#### `TEST_F(AnalyticsEdgeCasesTest, ANL_16_AnalyticsStateConsistencyRecovery)`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:536
- Brief: n/a
- Parameters:
  - `<unnamed>` (AnalyticsEdgeCasesTest): n/a
  - `<unnamed>` (ANL_16_AnalyticsStateConsistencyRecovery): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_01_PartialShardFailureFirstShard)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:33
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_01_PartialShardFailureFirstShard): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_02_CascadingShardFailures)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_02_CascadingShardFailures): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_03_NetworkInducedUnderflow)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_03_NetworkInducedUnderflow): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_04_NetworkDelayAccumulation)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_04_NetworkDelayAccumulation): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_05_ResponseReordering)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_05_ResponseReordering): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_06_DeduplicationAfterReordering)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_06_DeduplicationAfterReordering): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_07_CascadingTimeoutPartialRecovery)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_07_CascadingTimeoutPartialRecovery): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_08_TimeoutRecoveryBackoff)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_08_TimeoutRecoveryBackoff): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_09_LoadBalancingUnevenLatency)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:164
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_09_LoadBalancingUnevenLatency): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_10_AdaptiveTimeoutAdjustment)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:178
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_10_AdaptiveTimeoutAdjustment): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_11_ShardStateCorruptionDetection)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_11_ShardStateCorruptionDetection): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_12_ShardStateTransitionConsistency)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:219
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_12_ShardStateTransitionConsistency): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_13_ConcurrentShardQueryIsolation)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_13_ConcurrentShardQueryIsolation): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_14_ConcurrentRaceConditionPrevention)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_14_ConcurrentRaceConditionPrevention): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_15_ReplicaFailoverAutomatic)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:275
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_15_ReplicaFailoverAutomatic): n/a

#### `TEST_F(DistributedSearchEdgeCasesTest, DIS_16_ReplicaConsistencyAfterFailover)`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:289
- Brief: n/a
- Parameters:
  - `<unnamed>` (DistributedSearchEdgeCasesTest): n/a
  - `<unnamed>` (DIS_16_ReplicaConsistencyAfterFailover): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_01_DistributedHybridWithAnalytics)`
- Source: `tests/search/test_search_integration_phase4.cpp`:62
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_01_DistributedHybridWithAnalytics): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_02_DistributedFailureAnalyticsReporting)`
- Source: `tests/search/test_search_integration_phase4.cpp`:95
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_02_DistributedFailureAnalyticsReporting): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_03_FederatedSearchMultipleBackends)`
- Source: `tests/search/test_search_integration_phase4.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_03_FederatedSearchMultipleBackends): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_04_FederatedSearchPartialFailure)`
- Source: `tests/search/test_search_integration_phase4.cpp`:195
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_04_FederatedSearchPartialFailure): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_05_ExpansionFacetRankingPipeline)`
- Source: `tests/search/test_search_integration_phase4.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_05_ExpansionFacetRankingPipeline): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_06_PipelineStageFallback)`
- Source: `tests/search/test_search_integration_phase4.cpp`:284
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_06_PipelineStageFallback): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_07_StreamingWithConcurrentHighlighting)`
- Source: `tests/search/test_search_integration_phase4.cpp`:323
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_07_StreamingWithConcurrentHighlighting): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_08_StreamingBackpressureFlowControl)`
- Source: `tests/search/test_search_integration_phase4.cpp`:364
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_08_StreamingBackpressureFlowControl): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_09_ErrorPropagationPipeline)`
- Source: `tests/search/test_search_integration_phase4.cpp`:416
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_09_ErrorPropagationPipeline): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_10_GracefulDegradationFallback)`
- Source: `tests/search/test_search_integration_phase4.cpp`:455
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_10_GracefulDegradationFallback): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_11_PerformanceRealisticLoad)`
- Source: `tests/search/test_search_integration_phase4.cpp`:498
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_11_PerformanceRealisticLoad): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_12_PerformanceStressLatency)`
- Source: `tests/search/test_search_integration_phase4.cpp`:533
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_12_PerformanceStressLatency): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_13_StateConsistencyAcrossStages)`
- Source: `tests/search/test_search_integration_phase4.cpp`:561
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_13_StateConsistencyAcrossStages): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_14_StateRollbackOnFailure)`
- Source: `tests/search/test_search_integration_phase4.cpp`:598
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_14_StateRollbackOnFailure): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_15_RecoveryReplicaFailover)`
- Source: `tests/search/test_search_integration_phase4.cpp`:636
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_15_RecoveryReplicaFailover): n/a

#### `TEST_F(SearchIntegrationPhase4Test, INT_16_FullSystemRecoveryCascading)`
- Source: `tests/search/test_search_integration_phase4.cpp`:667
- Brief: n/a
- Parameters:
  - `<unnamed>` (SearchIntegrationPhase4Test): n/a
  - `<unnamed>` (INT_16_FullSystemRecoveryCascading): n/a

#### `TEST_F(UtilityComponentEdgeCasesTest, UTL_01_QueryExpansionMaxLimit)`
- Source: `tests/search/test_search_edge_cases_utility.cpp`:20
- Brief: n/a
- Parameters:
  - `<unnamed>` (UtilityComponentEdgeCasesTest): n/a
  - `<unnamed>` (UTL_01_QueryExpansionMaxLimit): n/a

#### `TEST_F(UtilityComponentEdgeCasesTest, UTL_02_QueryExpansionOverflowProtection)`
- Source: `tests/search/test_search_edge_cases_utility.cpp`:33
- Brief: n/a
- Parameters:
  - `<unnamed>` (UtilityComponentEdgeCasesTest): n/a
  - `<unnamed>` (UTL_02_QueryExpansionOverflowProtection): n/a

#### `TEST_F(UtilityComponentEdgeCasesTest, UTL_05_FuzzyMatchingTimeout)`
- Source: `tests/search/test_search_edge_cases_utility.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (UtilityComponentEdgeCasesTest): n/a
  - `<unnamed>` (UTL_05_FuzzyMatchingTimeout): n/a

#### `TEST_F(UtilityComponentEdgeCasesTest, UTL_09_FilterComplexityCompilationLimit)`
- Source: `tests/search/test_search_edge_cases_utility.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (UtilityComponentEdgeCasesTest): n/a
  - `<unnamed>` (UTL_09_FilterComplexityCompilationLimit): n/a

#### `TEST_F(UtilityComponentEdgeCasesTest, UTL_10_FilterComplexityRejection)`
- Source: `tests/search/test_search_edge_cases_utility.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (UtilityComponentEdgeCasesTest): n/a
  - `<unnamed>` (UTL_10_FilterComplexityRejection): n/a

#### `TEST_F(UtilityComponentEdgeCasesTest, UTL_11_TokenizationEmptyTokens)`
- Source: `tests/search/test_search_edge_cases_utility.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (UtilityComponentEdgeCasesTest): n/a
  - `<unnamed>` (UTL_11_TokenizationEmptyTokens): n/a

#### `TEST_F(UtilityComponentEdgeCasesTest, UTL_15_ErrorCodeAggregation)`
- Source: `tests/search/test_search_edge_cases_utility.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (UtilityComponentEdgeCasesTest): n/a
  - `<unnamed>` (UTL_15_ErrorCodeAggregation): n/a

### themis::search::testing::AnalyticsEdgeCasesTest

#### `void SetUp() override`
- Source: `tests/search/test_search_edge_cases_analytics.cpp`:40
- Brief: n/a
- Parameters: none

### themis::search::testing::DistributedSearchEdgeCasesTest

#### `void SetUp() override`
- Source: `tests/search/test_search_edge_cases_distributed.cpp`:28
- Brief: n/a
- Parameters: none

### themis::search::testing::SearchIntegrationPhase4Test

#### `void SetUp() override`
- Source: `tests/search/test_search_integration_phase4.cpp`:46
- Brief: n/a
- Parameters: none

