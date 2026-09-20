# ETHICS_AI DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\ethics_ai\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\ethics_ai\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 57
- Compounds: 161
- Classes/Structs: 81
- Namespaces: 14
- File Compounds: 57

## Namespaces
- @147136314125312125006222003163313037060141041256
- @311203113052065240120300012173265236014131234252
- @351237257370334275070146252300136200335012070214
- std
- themis
- themis::plugins
- themis::plugins::ethics
- themis::plugins::ethics::@021330254147016311153210336045124176057014133351
- themis::plugins::ethics::@201037302134030316304077100234153050033211134142
- themis::plugins::ethics::@234004037025165274356166154134041021052211044264
- themis::plugins::ethics::@262313363104051226064104026164164351062126303172
- themis::plugins::ethics::@345307354332311024274044151003150335145004214252
- themis::plugins::ethics::LDM7Schools
- themis::query

## Types
### Classes
- themis::plugins::ethics::ArgumentStore
- themis::plugins::ethics::ChainVisualizer
- themis::plugins::ethics::ConvergenceMarkerEngine
- themis::plugins::ethics::CrossSchoolTensionResolver
- themis::plugins::ethics::DiscourseMemoryStore
- themis::plugins::ethics::DiscourseOrchestrator
- themis::plugins::ethics::DynamicClusteringEngine
- themis::plugins::ethics::EthicalDiscourseEngine
- themis::plugins::ethics::EthicsAIPlugin
- themis::plugins::ethics::EthicsAQLQueries
- themis::plugins::ethics::EthicsAuditLog
- themis::plugins::ethics::EthicsBaseEntityAdapter
- themis::plugins::ethics::EthicsEvaluator
- themis::plugins::ethics::EthicsProfileRegistry
- themis::plugins::ethics::EthicsSelectionRouter
- themis::plugins::ethics::IAdaLoRABiasCorrector
- themis::plugins::ethics::IEthicsAIPlugin
- themis::plugins::ethics::IEthicsProfileRegistry
- themis::plugins::ethics::ILlmCascadeRouter
- themis::plugins::ethics::IdentityAdaLoRABiasCorrector
- themis::plugins::ethics::LlmCascadeRouter
- themis::plugins::ethics::MetaVerdictBuilder
- themis::plugins::ethics::MirrorSchoolHandler
- themis::plugins::ethics::PhilosophyLoader
- themis::plugins::ethics::PositionAbstractValidator
- themis::plugins::ethics::PriorRoundCompressor
- themis::plugins::ethics::RAGContextEngine
- themis::plugins::ethics::ScalarAdaLoRABiasCorrector
- themis::plugins::ethics::SynthesisMatrixBuilder
- themis::plugins::ethics::TournamentModeSelector

### Structs
- themis::plugins::ethics::ArgumentChain
- themis::plugins::ethics::CascadeRoutingConfig
- themis::plugins::ethics::CascadeRoutingDecision
- themis::plugins::ethics::ClusterAssignment
- themis::plugins::ethics::ClusterPosition
- themis::plugins::ethics::CompressionConfig
- themis::plugins::ethics::CompressionResult
- themis::plugins::ethics::ConvergenceMarker
- themis::plugins::ethics::CrossSchoolTensionEdge
- themis::plugins::ethics::CrossSchoolTensionGraph
- themis::plugins::ethics::CulturalEthicsSchoolDescriptor
- themis::plugins::ethics::DebateInitialization
- themis::plugins::ethics::DebateRound
- themis::plugins::ethics::DiscourseMemoryConfig
- themis::plugins::ethics::DiscourseOrchestrator::Impl
- themis::plugins::ethics::DiscourseOrchestratorPlan
- themis::plugins::ethics::DiscourseRoundOutput
- themis::plugins::ethics::EpisodicMemoryEntry
- themis::plugins::ethics::EthicalArgument
- themis::plugins::ethics::EthicalDecision
- themis::plugins::ethics::EthicsAIPlugin::Metrics
- themis::plugins::ethics::EthicsError
- themis::plugins::ethics::EthicsEvaluationResult
- themis::plugins::ethics::EthicsEvaluator::Config
- themis::plugins::ethics::EthicsIndexQuery
- themis::plugins::ethics::EthicsProfileMeta
- themis::plugins::ethics::EthicsSelectionRouter::Impl
- themis::plugins::ethics::EthicsSelectionRouter::Impl::PrecedentEntry
- themis::plugins::ethics::InjectionDecision
- themis::plugins::ethics::LegalGrounding
- themis::plugins::ethics::MetaVerdict
- themis::plugins::ethics::MetaVerdictSchoolVote
- themis::plugins::ethics::MirrorSchoolPolicy
- themis::plugins::ethics::ModelTokenBudget
- themis::plugins::ethics::NormCitation
- themis::plugins::ethics::NormEvidence
- themis::plugins::ethics::PhilosophyProfile
- themis::plugins::ethics::PhilosophyThesis
- themis::plugins::ethics::PositionAbstractConfig
- themis::plugins::ethics::PositionAbstractSchemaError
- themis::plugins::ethics::RAGContext
- themis::plugins::ethics::RoundAuditEntry
- themis::plugins::ethics::RouterCandidate
- themis::plugins::ethics::RouterConfig
- themis::plugins::ethics::RouterResult
- themis::plugins::ethics::SchemaValidationError
- themis::plugins::ethics::SchoolPositionSummary
- themis::plugins::ethics::SchoolTension
- themis::plugins::ethics::Status
- themis::plugins::ethics::TournamentConfig
- themis::plugins::ethics::TournamentSelectionResult

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 464

### bench_ethics_ai_plugin.cpp

#### `BENCHMARK(BM_ArgumentStore_GetArgument)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:759
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ArgumentStore_GetArgument): n/a

#### `BENCHMARK(BM_ArgumentStore_GetArgumentsByPhilosophy) -> Arg(10) ->Arg(50) ->Arg(100) ->Arg(500)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:760
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ArgumentStore_GetArgumentsByPhilosophy): n/a

#### `BENCHMARK(BM_ArgumentStore_GetDebateTranscript) -> Arg(2) ->Arg(3) ->Arg(5)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:779
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ArgumentStore_GetDebateTranscript): n/a

#### `BENCHMARK(BM_ArgumentStore_StoreArgument)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:758
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ArgumentStore_StoreArgument): n/a

#### `BENCHMARK(BM_ArgumentStore_StoreDebateRound) -> Arg(2) ->Arg(5) ->Arg(10)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:778
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ArgumentStore_StoreDebateRound): n/a

#### `BENCHMARK(BM_ArgumentStore_StoreDecision)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:761
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_ArgumentStore_StoreDecision): n/a

#### `BENCHMARK(BM_DiscourseEngine_ContinueDebateRound)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:775
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_DiscourseEngine_ContinueDebateRound): n/a

#### `BENCHMARK(BM_DiscourseEngine_InitializeDebate)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:771
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_DiscourseEngine_InitializeDebate): n/a

#### `BENCHMARK(BM_DiscourseEngine_MakeDecisionFiveSchools)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:773
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_DiscourseEngine_MakeDecisionFiveSchools): n/a

#### `BENCHMARK(BM_DiscourseEngine_MakeDecisionSingleSchool)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:772
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_DiscourseEngine_MakeDecisionSingleSchool): n/a

#### `BENCHMARK(BM_DiscourseEngine_MakeDecisionWithRAG)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:774
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_DiscourseEngine_MakeDecisionWithRAG): n/a

#### `BENCHMARK(BM_EthicsEvaluator_EvaluateDecision)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:782
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_EthicsEvaluator_EvaluateDecision): n/a

#### `BENCHMARK(BM_EthicsEvaluator_EvaluateDecisionManyArgs) -> Arg(10) ->Arg(50) ->Arg(100)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:784
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_EthicsEvaluator_EvaluateDecisionManyArgs): n/a

#### `BENCHMARK(BM_EthicsEvaluator_EvaluateDecisionNoArgs)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:783
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_EthicsEvaluator_EvaluateDecisionNoArgs): n/a

#### `BENCHMARK(BM_EthicsEvaluator_GetMetricsText)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:786
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_EthicsEvaluator_GetMetricsText): n/a

#### `BENCHMARK(BM_EthicsEvaluator_RecordDecision)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:785
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_EthicsEvaluator_RecordDecision): n/a

#### `BENCHMARK(BM_PhilosophyLoader_GetProfile)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:753
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PhilosophyLoader_GetProfile): n/a

#### `BENCHMARK(BM_PhilosophyLoader_HasProfile)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:754
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PhilosophyLoader_HasProfile): n/a

#### `BENCHMARK(BM_PhilosophyLoader_ListSchools)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:755
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PhilosophyLoader_ListSchools): n/a

#### `BENCHMARK(BM_PhilosophyLoader_LoadSingleProfile)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:752
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_PhilosophyLoader_LoadSingleProfile): n/a

#### `BENCHMARK(BM_RAGContextEngine_BuildContext)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:764
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RAGContextEngine_BuildContext): n/a

#### `BENCHMARK(BM_RAGContextEngine_BuildContextBatch10)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:765
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RAGContextEngine_BuildContextBatch10): n/a

#### `BENCHMARK(BM_RAGContextEngine_FindSimilarDilemmas)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:766
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RAGContextEngine_FindSimilarDilemmas): n/a

#### `BENCHMARK(BM_RAGContextEngine_TraverseArgumentChain)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:767
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RAGContextEngine_TraverseArgumentChain): n/a

#### `BENCHMARK(BM_RAGContextEngine_VectorSemanticSearch512)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:768
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_RAGContextEngine_VectorSemanticSearch512): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:788
- Brief: n/a
- Parameters: none

#### `void BM_ArgumentStore_GetArgument(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:275
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ArgumentStore_GetArgumentsByPhilosophy(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:302
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ArgumentStore_GetDebateTranscript(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:649
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ArgumentStore_StoreArgument(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:258
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ArgumentStore_StoreDebateRound(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:632
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_ArgumentStore_StoreDecision(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:323
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DiscourseEngine_ContinueDebateRound(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:587
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DiscourseEngine_InitializeDebate(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:463
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DiscourseEngine_MakeDecisionFiveSchools(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:524
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DiscourseEngine_MakeDecisionSingleSchool(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:493
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_DiscourseEngine_MakeDecisionWithRAG(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:555
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EthicsEvaluator_EvaluateDecision(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:673
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EthicsEvaluator_EvaluateDecisionManyArgs(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:704
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EthicsEvaluator_EvaluateDecisionNoArgs(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:691
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EthicsEvaluator_GetMetricsText(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:733
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_EthicsEvaluator_RecordDecision(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:722
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PhilosophyLoader_GetProfile(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:212
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PhilosophyLoader_HasProfile(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:226
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PhilosophyLoader_ListSchools(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:240
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_PhilosophyLoader_LoadSingleProfile(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:193
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RAGContextEngine_BuildContext(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:343
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RAGContextEngine_BuildContextBatch10(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:427
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RAGContextEngine_FindSimilarDilemmas(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:370
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RAGContextEngine_TraverseArgumentChain(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:391
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_RAGContextEngine_VectorSemanticSearch512(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:408
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `const std::vector< std::string > & benchmarkFiveSchools()`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:114
- Brief: n/a
- Parameters: none

#### `const std::vector< std::string > & benchmarkTwoSchools()`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:106
- Brief: n/a
- Parameters: none

#### `EthicalArgument createBenchmarkArgument(const std::string &id, const std::string &school)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:58
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `school` (const std::string &): n/a

#### `DebateRound createBenchmarkDebateRound(const std::string &debate_id, int round_number, int argument_count)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:82
- Brief: n/a
- Parameters:
  - `debate_id` (const std::string &): n/a
  - `round_number` (int): n/a
  - `argument_count` (int): n/a

#### `EthicalDecision createBenchmarkDecision()`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:69
- Brief: n/a
- Parameters: none

#### `std::vector< float > createQueryEmbedding(size_t dimensions)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:181
- Brief: n/a
- Parameters:
  - `dimensions` (size_t): n/a

#### `std::string generateRandomId(int index)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:52
- Brief: n/a
- Parameters:
  - `index` (int): n/a

#### `const std::string & getBenchmarkPhilosophyDirectory()`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:145
- Brief: n/a
- Parameters: none

#### `bool initializeStore(benchmark::State &state, ArgumentStore &store)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:150
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
  - `store` (ArgumentStore &): n/a

#### `bool loadBenchmarkProfiles(benchmark::State &state, PhilosophyLoader &loader)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:159
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
  - `loader` (PhilosophyLoader &): n/a

#### `void populateStoreWithArguments(ArgumentStore &store, const std::vector< std::string > &schools, int arguments_per_school)`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:168
- Brief: n/a
- Parameters:
  - `store` (ArgumentStore &): n/a
  - `schools` (const std::vector< std::string > &): n/a
  - `arguments_per_school` (int): n/a

#### `std::string resolveBenchmarkPhilosophyDirectory()`
- Source: `benchmarks/ethics_ai/bench_ethics_ai_plugin.cpp`:125
- Brief: n/a
- Parameters: none

### bench_ldm.cpp

#### `Arg(100) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:383
- Brief: n/a
- Parameters:
  - `<unnamed>` (100): n/a

#### `Arg(1000) -> Unit(benchmark::kMicrosecond)`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:423
- Brief: n/a
- Parameters:
  - `<unnamed>` (1000): n/a

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:428
- Brief: n/a
- Parameters: none

#### `void BM_LDM_AuditLog_Append(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:347
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LDM_AuditLog_ExportOnly(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:393
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LDM_Ebene1_22Schools(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:163
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LDM_Ebene1_TimeoutFailsafe(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:195
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LDM_EndToEnd_LAYERED_FAST(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:310
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LDM_MetaVerdictAssembly(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:228
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LDM_MirrorSchool_Parallel4(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:282
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LDM_PlanGeneration(benchmark::State &state)`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:140
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Unit(benchmark::kMillisecond) -> Iterations(1000)`
- Source: `benchmarks/ethics_ai/bench_ldm.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

### ethics_ai_plugin.cpp

#### `THEMIS_PLUGIN_EXPORT themis::plugins::IThemisPlugin * createPlugin()`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:536
- Brief: Create Plugin.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result. Calls: themis::plugins::ethics::EthicsAIPlugin().

#### `THEMIS_PLUGIN_EXPORT void destroyPlugin(themis::plugins::IThemisPlugin *plugin)`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:548
- Brief: Destroy Plugin.
- Parameters:
  - `plugin` (themis::plugins::IThemisPlugin *): Input/output parameter.
- Return: Return value.
- Details: plugin Input/output parameter. Return value. Implements destroyPlugin without additional internal calls.

### test_ethics_ai_community_separability.cpp

#### `TEST(EthicsAiCommunitySeparability, CSEP01_PublicTypeHeaderStandaloneCompile)`
- Source: `tests/ethics_ai/test_ethics_ai_community_separability.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiCommunitySeparability): n/a
  - `<unnamed>` (CSEP01_PublicTypeHeaderStandaloneCompile): n/a

#### `TEST(EthicsAiCommunitySeparability, CSEP02_SelectionOnly_WorksWithoutPrivateSources)`
- Source: `tests/ethics_ai/test_ethics_ai_community_separability.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiCommunitySeparability): n/a
  - `<unnamed>` (CSEP02_SelectionOnly_WorksWithoutPrivateSources): n/a

#### `TEST(EthicsAiCommunitySeparability, CSEP03_AuditLog_FullyUsable_CommunityBuild)`
- Source: `tests/ethics_ai/test_ethics_ai_community_separability.cpp`:148
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiCommunitySeparability): n/a
  - `<unnamed>` (CSEP03_AuditLog_FullyUsable_CommunityBuild): n/a

#### `TEST(EthicsAiCommunitySeparability, CSEP04_EthicsErrorCode_AllCodesInRange)`
- Source: `tests/ethics_ai/test_ethics_ai_community_separability.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiCommunitySeparability): n/a
  - `<unnamed>` (CSEP04_EthicsErrorCode_AllCodesInRange): n/a

#### `TEST(EthicsAiCommunitySeparability, CSEP05_LegalGrounding_DefaultIsFailing)`
- Source: `tests/ethics_ai/test_ethics_ai_community_separability.cpp`:202
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiCommunitySeparability): n/a
  - `<unnamed>` (CSEP05_LegalGrounding_DefaultIsFailing): n/a

#### `TEST(EthicsAiCommunitySeparability, CSEP06_MetaVerdict_DefaultCarriesNoSchoolEntries)`
- Source: `tests/ethics_ai/test_ethics_ai_community_separability.cpp`:220
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiCommunitySeparability): n/a
  - `<unnamed>` (CSEP06_MetaVerdict_DefaultCarriesNoSchoolEntries): n/a

### test_ethics_ai_discourse_hardening_focused.cpp

#### `TEST(DiscourseHardening, BoundaryScoreAt0_60IsContested)`
- Source: `tests/ethics_ai/test_ethics_ai_discourse_hardening_focused.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiscourseHardening): n/a
  - `<unnamed>` (BoundaryScoreAt0_60IsContested): n/a
- Details: TestDH-06: score exactly 0.60 maps to CONTESTED (boundary check).

#### `TEST(DiscourseHardening, ConflictThresholdOf1_0IsAccepted)`
- Source: `tests/ethics_ai/test_ethics_ai_discourse_hardening_focused.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiscourseHardening): n/a
  - `<unnamed>` (ConflictThresholdOf1_0IsAccepted): n/a
- Details: TestDH-03: conflict_threshold_ratio of 1.0 is accepted.

#### `TEST(DiscourseHardening, DefaultConflictThresholdRatioIs0_6)`
- Source: `tests/ethics_ai/test_ethics_ai_discourse_hardening_focused.cpp`:40
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiscourseHardening): n/a
  - `<unnamed>` (DefaultConflictThresholdRatioIs0_6): n/a
- Details: TestDH-01: Default conflict_threshold_ratio is 0.6.

#### `TEST(DiscourseHardening, HighScoreYieldsClearConsensus)`
- Source: `tests/ethics_ai/test_ethics_ai_discourse_hardening_focused.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiscourseHardening): n/a
  - `<unnamed>` (HighScoreYieldsClearConsensus): n/a
- Details: TestDH-05: score > 0.75 produces CLEAR_CONSENSUS.

#### `TEST(DiscourseHardening, MetaVerdictThresholdIsDeterministic)`
- Source: `tests/ethics_ai/test_ethics_ai_discourse_hardening_focused.cpp`:108
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiscourseHardening): n/a
  - `<unnamed>` (MetaVerdictThresholdIsDeterministic): n/a
- Details: TestDH-08: MetaVerdictThreshold is deterministic for identical inputs.

#### `TEST(DiscourseHardening, RouterConfigWithHardeningFieldsCompiles)`
- Source: `tests/ethics_ai/test_ethics_ai_discourse_hardening_focused.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiscourseHardening): n/a
  - `<unnamed>` (RouterConfigWithHardeningFieldsCompiles): n/a
- Details: TestDH-04: RouterConfig with both LDM hardening fields compiles and is assignable.

#### `TEST(DiscourseHardening, ZeroConflictThresholdIsAccepted)`
- Source: `tests/ethics_ai/test_ethics_ai_discourse_hardening_focused.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiscourseHardening): n/a
  - `<unnamed>` (ZeroConflictThresholdIsAccepted): n/a
- Details: TestDH-02: Zero conflict_threshold_ratio is accepted.

#### `TEST(DiscourseHardening, ZeroScoreIsDissent)`
- Source: `tests/ethics_ai/test_ethics_ai_discourse_hardening_focused.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiscourseHardening): n/a
  - `<unnamed>` (ZeroScoreIsDissent): n/a
- Details: TestDH-07: score 0.0 maps to DISSENT.

### test_ethics_ai_eu_artifact_norms_focused.cpp

#### `TEST(EthicsAiComplianceFocused, EuArtifactNorms_SkippedForCurrentBuild)`
- Source: `tests/ethics_ai/test_ethics_ai_eu_artifact_norms_focused.cpp`:21
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiComplianceFocused): n/a
  - `<unnamed>` (EuArtifactNorms_SkippedForCurrentBuild): n/a

### test_ethics_ai_eu_compliance.cpp

#### `TEST(EthicsAiEuCompliance, EuCompliance_SkippedForCurrentBuild)`
- Source: `tests/ethics_ai/test_ethics_ai_eu_compliance.cpp`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiEuCompliance): n/a
  - `<unnamed>` (EuCompliance_SkippedForCurrentBuild): n/a

### test_ethics_ai_euai_compliance_focused.cpp

#### `TEST(EthicsAiEuAiCompliance, EuAiCompliance_SkippedForCurrentBuild)`
- Source: `tests/ethics_ai/test_ethics_ai_euai_compliance_focused.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiEuAiCompliance): n/a
  - `<unnamed>` (EuAiCompliance_SkippedForCurrentBuild): n/a

### test_ethics_ai_ldm6_clustering_focused.cpp

#### `TEST(ClusterAssignment, ClusterCountIsCorrect)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm6_clustering_focused.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (ClusterAssignment): n/a
  - `<unnamed>` (ClusterCountIsCorrect): n/a
- Details: TestLDM6-04: cluster_count reflects actual distinct indices.

#### `TEST(ClusterAssignment, SchoolsInClusterReturnsCorrectMembers)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm6_clustering_focused.cpp`:73
- Brief: n/a
- Parameters:
  - `<unnamed>` (ClusterAssignment): n/a
  - `<unnamed>` (SchoolsInClusterReturnsCorrectMembers): n/a
- Details: TestLDM6-03: schoolsInCluster() returns correct members.

#### `TEST(CrossSchoolTensionGraph, TensionIsSymmetric)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm6_clustering_focused.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossSchoolTensionGraph): n/a
  - `<unnamed>` (TensionIsSymmetric): n/a
- Details: TestLDM6-02: tensionBetween() is symmetric.

#### `TEST(CrossSchoolTensionGraph, UnknownPairReturnsZero)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm6_clustering_focused.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (CrossSchoolTensionGraph): n/a
  - `<unnamed>` (UnknownPairReturnsZero): n/a
- Details: TestLDM6-01: tensionBetween() returns 0.0 for an unknown pair.

#### `TEST(DynamicClusteringEngine, EmptyGraphProducesEmptyAssignment)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm6_clustering_focused.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (DynamicClusteringEngine): n/a
  - `<unnamed>` (EmptyGraphProducesEmptyAssignment): n/a
- Details: TestLDM6-05: Empty graph produces an empty ClusterAssignment.

#### `TEST(DynamicClusteringEngine, HighTensionPairSeparatedIntoDifferentClusters)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm6_clustering_focused.cpp`:120
- Brief: n/a
- Parameters:
  - `<unnamed>` (DynamicClusteringEngine): n/a
  - `<unnamed>` (HighTensionPairSeparatedIntoDifferentClusters): n/a
- Details: TestLDM6-06: High-tension pair lands in different clusters. Builds a 4-school graph where kant↔utilitarianism have maximum tension (1.0) and all other pairs are zero. With 2 target clusters the greedy algorithm must separate them.

### test_ethics_ai_ldm7_focused.cpp

#### `TEST(CulturalEthicsSchoolDescriptor, DefaultConstructed)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm7_focused.cpp`:48
- Brief: n/a
- Parameters:
  - `<unnamed>` (CulturalEthicsSchoolDescriptor): n/a
  - `<unnamed>` (DefaultConstructed): n/a
- Details: TestLDM7-02: CulturalEthicsSchoolDescriptor default-constructs correctly.

#### `TEST(CulturalEthicsSchoolDescriptor, LatinLiberationTheologyDescriptor)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm7_focused.cpp`:85
- Brief: n/a
- Parameters:
  - `<unnamed>` (CulturalEthicsSchoolDescriptor): n/a
  - `<unnamed>` (LatinLiberationTheologyDescriptor): n/a
- Details: TestLDM7-04: A Latin Liberation Theology descriptor can be constructed.

#### `TEST(CulturalEthicsSchoolDescriptor, MaoriEthicsDescriptor)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm7_focused.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (CulturalEthicsSchoolDescriptor): n/a
  - `<unnamed>` (MaoriEthicsDescriptor): n/a
- Details: TestLDM7-03: A Māori ethics descriptor can be constructed with required fields.

#### `TEST(LDM7Schools, ConstantsAreNonEmptyAndDistinct)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm7_focused.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (LDM7Schools): n/a
  - `<unnamed>` (ConstantsAreNonEmptyAndDistinct): n/a
- Details: TestLDM7-01: LDM7Schools constants are non-empty and distinct.

### test_ethics_ai_ldm8_focused.cpp

#### `TEST(AdaLoRABiasCorrector, IdentityHasNoAdapters)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm8_focused.cpp`:33
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaLoRABiasCorrector): n/a
  - `<unnamed>` (IdentityHasNoAdapters): n/a
- Details: TestLDM8-01: IdentityAdaLoRABiasCorrector has no adapters for any school.

#### `TEST(AdaLoRABiasCorrector, IdentityReturnsRawScoreUnchanged)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm8_focused.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaLoRABiasCorrector): n/a
  - `<unnamed>` (IdentityReturnsRawScoreUnchanged): n/a
- Details: TestLDM8-02: IdentityAdaLoRABiasCorrector returns raw_score unchanged.

#### `TEST(AdaLoRABiasCorrector, PolymorphicDispatchWorksCorrectly)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm8_focused.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaLoRABiasCorrector): n/a
  - `<unnamed>` (PolymorphicDispatchWorksCorrectly): n/a
- Details: TestLDM8 — Polymorphic dispatch through IAdaLoRABiasCorrector base pointer.

#### `TEST(AdaLoRABiasCorrector, ScalarClampsResultToUnitInterval)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm8_focused.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaLoRABiasCorrector): n/a
  - `<unnamed>` (ScalarClampsResultToUnitInterval): n/a
- Details: TestLDM8-04: Corrected score is clamped to [0.0, 1.0] and deterministic.

#### `TEST(AdaLoRABiasCorrector, ScalarHasAdapterOnlyForRegistered)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm8_focused.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (AdaLoRABiasCorrector): n/a
  - `<unnamed>` (ScalarHasAdapterOnlyForRegistered): n/a
- Details: TestLDM8-03: hasAdapter() returns true only for registered schools.

### test_ethics_ai_ldm_contract_focused.cpp

#### `TEST(EthicsAiLdmContract, EAL01_DiscourseMode_SelectionOnly_EmptyPlan)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm_contract_focused.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiLdmContract): n/a
  - `<unnamed>` (EAL01_DiscourseMode_SelectionOnly_EmptyPlan): n/a

#### `TEST(EthicsAiLdmContract, EAL02_EqualWeightContract_InitialWeight)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm_contract_focused.cpp`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiLdmContract): n/a
  - `<unnamed>` (EAL02_EqualWeightContract_InitialWeight): n/a

#### `TEST(EthicsAiLdmContract, EAL03_Ebene1_AbstainFailClosed_OnTimeout)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm_contract_focused.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiLdmContract): n/a
  - `<unnamed>` (EAL03_Ebene1_AbstainFailClosed_OnTimeout): n/a

#### `TEST(EthicsAiLdmContract, EAL04_MetaVerdict_AllAbstain_ProducesDissent)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm_contract_focused.cpp`:235
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiLdmContract): n/a
  - `<unnamed>` (EAL04_MetaVerdict_AllAbstain_ProducesDissent): n/a

#### `TEST(EthicsAiLdmContract, EAL05_MetaVerdict_ClearConsensus_ConvergenceAbove075)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm_contract_focused.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiLdmContract): n/a
  - `<unnamed>` (EAL05_MetaVerdict_ClearConsensus_ConvergenceAbove075): n/a

#### `TEST(EthicsAiLdmContract, EAL06_CrossCulturalFlag_MultiRegionConsensus)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm_contract_focused.cpp`:321
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiLdmContract): n/a
  - `<unnamed>` (EAL06_CrossCulturalFlag_MultiRegionConsensus): n/a

#### `TEST(EthicsAiLdmContract, EAL07_MirrorSchool_PresentInMinorityDissent_EvenForClearConsensus)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm_contract_focused.cpp`:357
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiLdmContract): n/a
  - `<unnamed>` (EAL07_MirrorSchool_PresentInMinorityDissent_EvenForClearConsensus): n/a

#### `TEST(EthicsAiLdmContract, EAL08_LegalGrounding_Unavailable_FlagObservable)`
- Source: `tests/ethics_ai/test_ethics_ai_ldm_contract_focused.cpp`:418
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAiLdmContract): n/a
  - `<unnamed>` (EAL08_LegalGrounding_Unavailable_FlagObservable): n/a

### test_ethics_ai_profile_reload_focused.cpp

#### `TEST(ProfileReloadGuard, CopiedConfigIsIndependent)`
- Source: `tests/ethics_ai/test_ethics_ai_profile_reload_focused.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfileReloadGuard): n/a
  - `<unnamed>` (CopiedConfigIsIndependent): n/a
- Details: TestPR-04: A copied RouterConfig is independent of the original.

#### `TEST(ProfileReloadGuard, DefaultConstructedConfigIsUsable)`
- Source: `tests/ethics_ai/test_ethics_ai_profile_reload_focused.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfileReloadGuard): n/a
  - `<unnamed>` (DefaultConstructedConfigIsUsable): n/a
- Details: TestPR-07: Default-constructed RouterConfig with snapshot=true is usable.

#### `TEST(ProfileReloadGuard, DefaultSnapshotFlagIsTrue)`
- Source: `tests/ethics_ai/test_ethics_ai_profile_reload_focused.cpp`:38
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfileReloadGuard): n/a
  - `<unnamed>` (DefaultSnapshotFlagIsTrue): n/a
- Details: TestPR-01: Default value of snapshot_profile_on_round_start is true.

#### `TEST(ProfileReloadGuard, FlagCanBeSetToFalse)`
- Source: `tests/ethics_ai/test_ethics_ai_profile_reload_focused.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfileReloadGuard): n/a
  - `<unnamed>` (FlagCanBeSetToFalse): n/a
- Details: TestPR-02: Flag can be set to false (legacy/test mode).

#### `TEST(ProfileReloadGuard, FlagPreservedOnCopy)`
- Source: `tests/ethics_ai/test_ethics_ai_profile_reload_focused.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfileReloadGuard): n/a
  - `<unnamed>` (FlagPreservedOnCopy): n/a
- Details: TestPR-03: Flag is preserved through copy construction.

#### `TEST(ProfileReloadGuard, IndependentConfigsDoNotInterfere)`
- Source: `tests/ethics_ai/test_ethics_ai_profile_reload_focused.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfileReloadGuard): n/a
  - `<unnamed>` (IndependentConfigsDoNotInterfere): n/a
- Details: TestPR-06: Two configs with different thresholds do not interfere.

#### `TEST(ProfileReloadGuard, SchoolBiasCopyIsDeep)`
- Source: `tests/ethics_ai/test_ethics_ai_profile_reload_focused.cpp`:123
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfileReloadGuard): n/a
  - `<unnamed>` (SchoolBiasCopyIsDeep): n/a
- Details: TestPR-08: Modifying school_bias on a copy does not affect the original.

#### `TEST(ProfileReloadGuard, SnapshotCopiesConflictThreshold)`
- Source: `tests/ethics_ai/test_ethics_ai_profile_reload_focused.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (ProfileReloadGuard): n/a
  - `<unnamed>` (SnapshotCopiesConflictThreshold): n/a
- Details: TestPR-05: Copied config carries the same conflict_threshold_ratio.

### themis::plugins::ethics

#### `MetaVerdict::ConvergenceVerdict MetaVerdictThreshold(double score) noexcept`
- Source: `include/ethics_ai/ethics_ai_types.h`:409
- Brief: n/a
- Parameters:
  - `score` (double): n/a

#### `const char * argumentStrengthToString(ArgumentStrength strength)`
- Source: `src/ethics_ai/ethics_ai_types.cpp`:77
- Brief: Argument Strength To String.
- Parameters:
  - `strength` (ArgumentStrength): Input parameter.
- Return: Pointer to the result.
- Details: strength Input parameter. Pointer to the result.

#### `const char * argumentTypeToString(ArgumentType type)`
- Source: `src/ethics_ai/ethics_ai_types.cpp`:25
- Brief: Argument Type To String.
- Parameters:
  - `type` (ArgumentType): Input parameter.
- Return: Pointer to the result.
- Details: type Input parameter. Pointer to the result.

#### `std::string computeSHA256(const uint8_t *data, size_t len)`
- Source: `src/ethics_ai/argument_store.cpp`:45
- Brief: Compute SHA256.
- Parameters:
  - `data` (const uint8_t *): Input parameter.
  - `len` (size_t): Input parameter.
- Return: Return value.
- Details: data Input parameter. len Input parameter. Return value. Calls: SHA256_Init(), SHA256_Update(), SHA256_Final(), std::setw(), std::setfill(), str().

#### `std::string makeIntegrityKey(const std::string &entity_key)`
- Source: `src/ethics_ai/argument_store.cpp`:65
- Brief: Make Integrity Key.
- Parameters:
  - `entity_key` (const std::string &): Input parameter.
- Return: Return value.
- Details: entity_key Input parameter. Return value. Implements makeIntegrityKey without additional internal calls.

#### `double strengthToScore(ArgumentStrength s)`
- Source: `src/ethics_ai/ethics_evaluator.cpp`:218
- Brief: Strength To Score.
- Parameters:
  - `s` (ArgumentStrength): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Implements strengthToScore without additional internal calls.

#### `ArgumentStrength stringToArgumentStrength(const std::string &str)`
- Source: `src/ethics_ai/ethics_ai_types.cpp`:99
- Brief: String To Argument Strength.
- Parameters:
  - `str` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: str Input parameter. Return value. str Input parameter. Return value. std::invalid_argument if an error occurs. Calls: std::transform(), begin(), end().

#### `ArgumentType stringToArgumentType(const std::string &str)`
- Source: `src/ethics_ai/ethics_ai_types.cpp`:51
- Brief: String To Argument Type.
- Parameters:
  - `str` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::invalid_argument: if an error occurs.
- Details: str Input parameter. Return value. str Input parameter. Return value. std::invalid_argument if an error occurs. Calls: std::transform(), begin(), end().

#### `bool verifyModelIntegrity(const std::vector< uint8_t > &blob, const std::string &entity_id, const std::optional< std::string > &expected_hash)`
- Source: `src/ethics_ai/argument_store.cpp`:77
- Brief: Verify Model Integrity.
- Parameters:
  - `blob` (const std::vector< uint8_t > &): Input parameter.
  - `entity_id` (const std::string &): Identifier of the entity.
  - `expected_hash` (const std::optional< std::string > &): Input parameter.
- Return: True when the operation succeeds.
- Details: blob Input parameter. entity_id Identifier of the entity. expected_hash Input parameter. True when the operation succeeds. Calls: computeSHA256(), data(), size(), spdlog::debug(), spdlog::error().

### themis::plugins::ethics::ArgumentChain

#### `ArgumentChain()`
- Source: `include/ethics_ai/ethics_ai_types.h`:69
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::ArgumentStore

#### `ArgumentStore()=default`
- Source: `src/ethics_ai/argument_store.h`:36
- Brief: n/a
- Parameters: none

#### `std::variant< EthicalArgument, Status > getArgument(const std::string &argument_id)`
- Source: `src/ethics_ai/argument_store.h`:55
- Brief: n/a
- Parameters:
  - `argument_id` (const std::string &): n/a

#### `std::variant< std::vector< EthicalArgument >, Status > getArgumentsByPhilosophy(const std::string &philosophy_school, const std::vector< ArgumentType > &argument_types, size_t limit)`
- Source: `src/ethics_ai/argument_store.h`:57
- Brief: n/a
- Parameters:
  - `philosophy_school` (const std::string &): n/a
  - `argument_types` (const std::vector< ArgumentType > &): n/a
  - `limit` (size_t): n/a

#### `std::variant< ArgumentChain, Status > getChain(const std::string &chain_id)`
- Source: `src/ethics_ai/argument_store.h`:88
- Brief: n/a
- Parameters:
  - `chain_id` (const std::string &): n/a

#### `std::variant< std::vector< DebateRound >, Status > getDebateTranscript(const std::string &debate_id)`
- Source: `src/ethics_ai/argument_store.h`:97
- Brief: n/a
- Parameters:
  - `debate_id` (const std::string &): n/a

#### `std::variant< EthicalDecision, Status > getDecision(const std::string &decision_id)`
- Source: `src/ethics_ai/argument_store.h`:70
- Brief: n/a
- Parameters:
  - `decision_id` (const std::string &): n/a

#### `std::variant< PhilosophyProfile, Status > getPhilosophyProfile(const std::string &school)`
- Source: `src/ethics_ai/argument_store.h`:79
- Brief: n/a
- Parameters:
  - `school` (const std::string &): n/a

#### `Status initialize(std::shared_ptr< RocksDBWrapper > storage, std::shared_ptr< query::QueryEngine > query_engine=nullptr)`
- Source: `src/ethics_ai/argument_store.h`:48
- Brief: Initialize.
- Parameters:
  - `storage` (std::shared_ptr< RocksDBWrapper >): Input parameter.
  - `query_engine` (std::shared_ptr< query::QueryEngine >): Input parameter.
- Return: Return value.
- Details: storage Input parameter. query_engine Input parameter. Return value. Calls: lock(), Status::Error(), Status::OK().

#### `void setVectorStoreFunction(VectorStoreFn fn)`
- Source: `src/ethics_ai/argument_store.h`:46
- Brief: Set Vector Store Function.
- Parameters:
  - `fn` (VectorStoreFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lock(), std::move().

#### `void setVectorWriter(std::shared_ptr< IVectorWriter > writer, std::function< std::vector< float >(const std::string &)> embedding_fn=nullptr)`
- Source: `src/ethics_ai/argument_store.h`:106
- Brief: n/a
- Parameters:
  - `writer` (std::shared_ptr< IVectorWriter >): n/a
  - `embedding_fn` (std::function< std::vector< float >(const std::string &)>): n/a

#### `void shutdown()`
- Source: `src/ethics_ai/argument_store.h`:104
- Brief: Shutdown.
- Parameters: none
- Details: Calls: lock(), reset(), clear().

#### `Status storeArgument(const EthicalArgument &argument, bool store_vector=true)`
- Source: `src/ethics_ai/argument_store.h`:53
- Brief: Store Argument.
- Parameters:
  - `argument` (const EthicalArgument &): Input parameter.
  - `store_vector` (bool): Input parameter.
- Return: Return value.
- Details: argument Input parameter. store_vector Input parameter. Return value. Calls: lock(), Status::Error(), empty(), Status::OK(), EthicsBaseEntityAdapter::toBaseEntity(), EthicsBaseEntityAdapter::makeArgumentKey(), serialize(), put().

#### `Status storeChain(const ArgumentChain &chain)`
- Source: `src/ethics_ai/argument_store.h`:86
- Brief: Store Chain.
- Parameters:
  - `chain` (const ArgumentChain &): Input parameter.
- Return: Return value.
- Details: chain Input parameter. Return value. chain Input parameter. Return value. Calls: lock(), Status::Error(), empty(), Status::OK().

#### `Status storeDebateRound(const DebateRound &round)`
- Source: `src/ethics_ai/argument_store.h`:95
- Brief: Store Debate Round.
- Parameters:
  - `round` (const DebateRound &): Input parameter.
- Return: Return value.
- Details: ============================================================================ v0. round Input parameter. Return value. round Input parameter. Return value. 2.0 — Debate Transcript Storage ============================================================================ Calls: lock(), Status::OK(), push_back(), std::sort(), begin(), end().

#### `Status storeDecision(const EthicalDecision &decision)`
- Source: `src/ethics_ai/argument_store.h`:68
- Brief: Store Decision.
- Parameters:
  - `decision` (const EthicalDecision &): Input parameter.
- Return: Return value.
- Details: decision Input parameter. Return value. decision Input parameter. Return value. Calls: lock(), Status::Error(), empty(), Status::OK(), EthicsBaseEntityAdapter::toBaseEntity(), EthicsBaseEntityAdapter::makeDecisionKey(), serialize(), put().

#### `Status storePhilosophyProfile(const PhilosophyProfile &profile)`
- Source: `src/ethics_ai/argument_store.h`:77
- Brief: Store Philosophy Profile.
- Parameters:
  - `profile` (const PhilosophyProfile &): Input parameter.
- Return: Return value.
- Details: profile Input parameter. Return value. profile Input parameter. Return value. Calls: lock(), Status::Error(), empty(), Status::OK(), EthicsBaseEntityAdapter::toBaseEntity(), EthicsBaseEntityAdapter::makeProfileKey(), serialize(), put().

#### `~ArgumentStore()=default`
- Source: `src/ethics_ai/argument_store.h`:37
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::CascadeRoutingConfig

#### `CascadeRoutingConfig defaultConfig()`
- Source: `include/ethics_ai/llm_cascade_router.h`:43
- Brief: Default Config.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements defaultConfig without additional internal calls.

### themis::plugins::ethics::ChainVisualizer

#### `std::string chainToDot(const ArgumentChain &chain, ArgumentStore &store, const std::string &graph_name="ethics_chain")`
- Source: `src/ethics_ai/chain_visualizer.h`:47
- Brief: Chain To Dot.
- Parameters:
  - `chain` (const ArgumentChain &): Input parameter.
  - `store` (ArgumentStore &): Input/output parameter.
  - `graph_name` (const std::string &): Name of the graph.
- Return: Return value.
- Details: chain Input parameter. store Input/output parameter. graph_name Name of the graph. Return value.

#### `std::string chainToMermaid(const ArgumentChain &chain, ArgumentStore &store)`
- Source: `src/ethics_ai/chain_visualizer.h`:59
- Brief: Chain To Mermaid.
- Parameters:
  - `chain` (const ArgumentChain &): Input parameter.
  - `store` (ArgumentStore &): Input/output parameter.
- Return: Return value.
- Details: chain Input parameter. store Input/output parameter. Return value.

#### `std::string dotEscape(const std::string &s)`
- Source: `src/ethics_ai/chain_visualizer.h`:84
- Brief: Escape a string for use inside DOT double-quoted attributes.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: Dot Escape. s Input parameter. Return value. s Input parameter. Return value. Calls: str().

#### `const char * dotFillColor(ArgumentType type)`
- Source: `src/ethics_ai/chain_visualizer.h`:70
- Brief: Node colour (DOT fillcolor) based on argument type.
- Parameters:
  - `type` (ArgumentType): Input parameter.
- Return: Pointer to the result.
- Details: Dot Fill Color. type Input parameter. Pointer to the result. type Input parameter. Pointer to the result. Implements dotFillColor without additional internal calls.

#### `std::string exportDot(const std::vector< std::string > &argument_ids, ArgumentStore &store, const std::string &graph_name="ethics_debate")`
- Source: `src/ethics_ai/chain_visualizer.h`:30
- Brief: Export Dot.
- Parameters:
  - `argument_ids` (const std::vector< std::string > &): Input parameter.
  - `store` (ArgumentStore &): Input/output parameter.
  - `graph_name` (const std::string &): Name of the graph.
- Return: Return value.
- Details: argument_ids Input parameter. store Input/output parameter. graph_name Name of the graph. Return value.

#### `std::string exportMermaid(const std::vector< std::string > &argument_ids, ArgumentStore &store)`
- Source: `src/ethics_ai/chain_visualizer.h`:42
- Brief: Export Mermaid.
- Parameters:
  - `argument_ids` (const std::vector< std::string > &): Input parameter.
  - `store` (ArgumentStore &): Input/output parameter.
- Return: Return value.
- Details: argument_ids Input parameter. store Input/output parameter. Return value.

#### `std::string makeLabel(const EthicalArgument &arg)`
- Source: `src/ethics_ai/chain_visualizer.h`:77
- Brief: Short label string: "<school>\n<type> | <strength>".
- Parameters:
  - `arg` (const EthicalArgument &): Input parameter.
- Return: Return value.
- Details: Make Label. arg Input parameter. Return value. arg Input parameter. Return value. Calls: argumentTypeToString(), argumentStrengthToString().

#### `std::string mermaidEscape(const std::string &s)`
- Source: `src/ethics_ai/chain_visualizer.h`:91
- Brief: Escape a string for use inside Mermaid node labels.
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: Mermaid Escape. s Input parameter. Return value. s Input parameter. Return value. Calls: str().

### themis::plugins::ethics::ClusterAssignment

#### `std::vector< std::string > schoolsInCluster(std::size_t cluster_index) const`
- Source: `include/ethics_ai/ethics_ai_types.h`:541
- Brief: n/a
- Parameters:
  - `cluster_index` (std::size_t): n/a

### themis::plugins::ethics::ConvergenceMarkerEngine

#### `ConvergenceMarkerEngine()=default`
- Source: `include/ethics_ai/convergence_marker_engine.h`:45
- Brief: n/a
- Parameters: none

#### `std::string buildConvergencePreamble(const std::vector< ConvergenceMarker > &markers, int max_tokens=250) const`
- Source: `include/ethics_ai/convergence_marker_engine.h`:51
- Brief: n/a
- Parameters:
  - `markers` (const std::vector< ConvergenceMarker > &): n/a
  - `max_tokens` (int): n/a

#### `std::string convergenceTypeLabel(ConvergenceType type) noexcept`
- Source: `include/ethics_ai/convergence_marker_engine.h`:61
- Brief: Convergence Type Label.
- Parameters:
  - `type` (ConvergenceType): Input parameter.
- Return: Return value.
- Details: type Input parameter. Return value. Exception safety: noexcept.

#### `int countTokens(const std::string &text) noexcept`
- Source: `include/ethics_ai/convergence_marker_engine.h`:70
- Brief: Count Tokens.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Exception safety: noexcept.

#### `std::vector< ConvergenceMarker > detectConvergences(const std::vector< DiscourseRoundOutput > &round_outputs, const std::vector< SchoolTension > &tensions={}) const`
- Source: `include/ethics_ai/convergence_marker_engine.h`:47
- Brief: n/a
- Parameters:
  - `round_outputs` (const std::vector< DiscourseRoundOutput > &): n/a
  - `tensions` (const std::vector< SchoolTension > &): n/a

### themis::plugins::ethics::CrossSchoolTensionGraph

#### `double tensionBetween(const std::string &a, const std::string &b) const noexcept`
- Source: `include/ethics_ai/ethics_ai_types.h`:526
- Brief: n/a
- Parameters:
  - `a` (const std::string &): n/a
  - `b` (const std::string &): n/a

### themis::plugins::ethics::CrossSchoolTensionResolver

#### `CrossSchoolTensionResolver()=default`
- Source: `include/ethics_ai/cross_school_tension_resolver.h`:40
- Brief: n/a
- Parameters: none

#### `std::vector< SchoolTension > loadTensions(const PhilosophyProfile &profile) const`
- Source: `include/ethics_ai/cross_school_tension_resolver.h`:55
- Brief: Load Tensions.
- Parameters:
  - `profile` (const PhilosophyProfile &): Input parameter.
- Return: Return value.
- Details: profile Input parameter. Return value.

#### `std::vector< InjectionDecision > resolveOpponentInjections(const std::string &own_school_id, const std::vector< std::string > &opponent_school_ids, const std::vector< EthicalArgument > &opponent_round_args, const std::vector< SchoolTension > &tensions, float full_injection_threshold=0.6f, int max_full_injections=2) const`
- Source: `include/ethics_ai/cross_school_tension_resolver.h`:42
- Brief: n/a
- Parameters:
  - `own_school_id` (const std::string &): n/a
  - `opponent_school_ids` (const std::vector< std::string > &): n/a
  - `opponent_round_args` (const std::vector< EthicalArgument > &): n/a
  - `tensions` (const std::vector< SchoolTension > &): n/a
  - `full_injection_threshold` (float): n/a
  - `max_full_injections` (int): n/a

### themis::plugins::ethics::DebateInitialization

#### `DebateInitialization()`
- Source: `include/ethics_ai/ethics_ai_types.h`:148
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::DebateRound

#### `DebateRound()`
- Source: `include/ethics_ai/ethics_ai_types.h`:134
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::DiscourseMemoryStore

#### `DiscourseMemoryStore(DiscourseMemoryConfig config=DiscourseMemoryConfig{})`
- Source: `include/ethics_ai/discourse_memory_store.h`:32
- Brief: n/a
- Parameters:
  - `config` (DiscourseMemoryConfig): n/a

#### `std::map< std::string, std::string > buildAllEpisodicContexts(const std::vector< std::string > &school_ids, int max_episodes=3) const`
- Source: `include/ethics_ai/discourse_memory_store.h`:55
- Brief: n/a
- Parameters:
  - `school_ids` (const std::vector< std::string > &): n/a
  - `max_episodes` (int): n/a

#### `std::string buildEpisodicContext(const std::string &school_id, int max_episodes=3) const`
- Source: `include/ethics_ai/discourse_memory_store.h`:51
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a
  - `max_episodes` (int): n/a

#### `void clear()`
- Source: `include/ethics_ai/discourse_memory_store.h`:62
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `std::string compressPosition(const std::string &position_abstract, int max_tokens) noexcept`
- Source: `include/ethics_ai/discourse_memory_store.h`:93
- Brief: Compress Position.
- Parameters:
  - `position_abstract` (const std::string &): Input parameter.
  - `max_tokens` (int): Input parameter.
- Return: Return value.
- Details: position_abstract Input parameter. max_tokens Input parameter. Return value. Exception safety: noexcept.

#### `const DiscourseMemoryConfig & config() const noexcept`
- Source: `include/ethics_ai/discourse_memory_store.h`:71
- Brief: n/a
- Parameters: none

#### `int countTokens(const std::string &text) noexcept`
- Source: `include/ethics_ai/discourse_memory_store.h`:85
- Brief: Count Tokens.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Exception safety: noexcept.

#### `size_t episodeCount(const std::string &school_id) const`
- Source: `include/ethics_ai/discourse_memory_store.h`:69
- Brief: Episode Count.
- Parameters:
  - `school_id` (const std::string &): Identifier of the school.
- Return: Return value.
- Details: school_id Identifier of the school. Return value.

#### `std::vector< EpisodicMemoryEntry > getEpisodesForSchool(const std::string &school_id, int max_episodes=3) const`
- Source: `include/ethics_ai/discourse_memory_store.h`:47
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a
  - `max_episodes` (int): n/a

#### `void storeEpisode(const DiscourseRoundOutput &output)`
- Source: `include/ethics_ai/discourse_memory_store.h`:39
- Brief: Store Episode.
- Parameters:
  - `output` (const DiscourseRoundOutput &): Input parameter.
- Details: output Input parameter. output Input parameter. Calls: compressPosition(), lock(), push_back(), std::move(), size(), erase(), begin().

#### `void storeEpisode(const EpisodicMemoryEntry &entry)`
- Source: `include/ethics_ai/discourse_memory_store.h`:45
- Brief: Store Episode.
- Parameters:
  - `entry` (const EpisodicMemoryEntry &): Input parameter.
- Details: entry Input parameter. entry Input parameter. Calls: compressPosition(), lock(), push_back(), std::move(), size(), erase(), begin().

### themis::plugins::ethics::DiscourseOrchestrator

#### `DiscourseOrchestrator(DiscourseOrchestrator &&) noexcept=default`
- Source: `include/ethics_ai/discourse_orchestrator.h`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiscourseOrchestrator &&): n/a

#### `DiscourseOrchestrator(IEthicsProfileRegistry *registry, const RouterConfig &config)`
- Source: `include/ethics_ai/discourse_orchestrator.h`:57
- Brief: Discourse Orchestrator.
- Parameters:
  - `registry` (IEthicsProfileRegistry *): Input/output parameter.
  - `config` (const RouterConfig &): Input parameter.
- Return: Return value.
- Details: registry Input/output parameter. config Input parameter. Return value.

#### `DiscourseOrchestrator(const DiscourseOrchestrator &)=delete`
- Source: `include/ethics_ai/discourse_orchestrator.h`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DiscourseOrchestrator &): n/a

#### `DiscourseOrchestrator & operator=(DiscourseOrchestrator &&) noexcept=default`
- Source: `include/ethics_ai/discourse_orchestrator.h`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (DiscourseOrchestrator &&): n/a

#### `DiscourseOrchestrator & operator=(const DiscourseOrchestrator &)=delete`
- Source: `include/ethics_ai/discourse_orchestrator.h`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (const DiscourseOrchestrator &): n/a

#### `std::vector< DiscourseRoundOutput > runEbene1(const DiscourseOrchestratorPlan &plan, const std::string &dilemma_text, const MirrorSchoolPolicy &mirror_policy)`
- Source: `include/ethics_ai/discourse_orchestrator.h`:81
- Brief: ============================================================================ runEbene1 — parallel equal-weight initial scoring ============================================================================
- Parameters:
  - `plan` (const DiscourseOrchestratorPlan &): Input parameter.
  - `dilemma_text` (const std::string &): Input parameter.
  - `mirror_policy` (const MirrorSchoolPolicy &): Input parameter.
- Return: Return value.
- Details: plan Input parameter. dilemma_text Input parameter. mirror_policy Input parameter. Return value.

#### `std::pair< std::vector< ClusterPosition >, std::vector< EpisodicMemoryEntry > > runEbene2(const DiscourseOrchestratorPlan &plan, const std::vector< DiscourseRoundOutput > &ebene1_results)`
- Source: `include/ethics_ai/discourse_orchestrator.h`:88
- Brief: n/a
- Parameters:
  - `plan` (const DiscourseOrchestratorPlan &): n/a
  - `ebene1_results` (const std::vector< DiscourseRoundOutput > &): n/a

#### `std::vector< DiscourseRoundOutput > runMirrorSchools(const MirrorSchoolPolicy &mirror_policy, const std::string &dilemma_text, const std::string &domain)`
- Source: `include/ethics_ai/discourse_orchestrator.h`:91
- Brief: ============================================================================ runMirrorSchools — lightweight parallel mirror step ============================================================================
- Parameters:
  - `mirror_policy` (const MirrorSchoolPolicy &): Input parameter.
  - `dilemma_text` (const std::string &): Input parameter.
  - `domain` (const std::string &): Input parameter.
- Return: Return value.
- Details: mirror_policy Input parameter. dilemma_text Input parameter. domain Input parameter. Return value.

#### `void setLLMInferenceFn(LLMInferenceFn fn)`
- Source: `include/ethics_ai/discourse_orchestrator.h`:72
- Brief: Set LLMInference Fn.
- Parameters:
  - `fn` (LLMInferenceFn): Input parameter.
- Details: fn Input parameter.

#### `void setSchoolTimeoutMs(int timeout_ms) noexcept`
- Source: `include/ethics_ai/discourse_orchestrator.h`:79
- Brief: Set School Timeout Ms.
- Parameters:
  - `timeout_ms` (int): Input parameter.
- Details: timeout_ms Input parameter. Exception safety: noexcept.

#### `~DiscourseOrchestrator()`
- Source: `include/ethics_ai/discourse_orchestrator.h`:60
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::DiscourseOrchestrator::Impl

#### `Impl(IEthicsProfileRegistry *reg, const RouterConfig &cfg)`
- Source: `src/ethics_ai/discourse_orchestrator.cpp`:132
- Brief: n/a
- Parameters:
  - `reg` (IEthicsProfileRegistry *): n/a
  - `cfg` (const RouterConfig &): n/a

#### `LLMInferenceFn effectiveInferenceFn() const`
- Source: `src/ethics_ai/discourse_orchestrator.cpp`:136
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::DiscourseOrchestratorPlan

#### `bool empty() const noexcept`
- Source: `include/ethics_ai/ethics_selection_router.h`:48
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::DynamicClusteringEngine

#### `DynamicClusteringEngine(std::size_t target_cluster_count=0) noexcept`
- Source: `include/ethics_ai/ethics_ai_types.h`:553
- Brief: n/a
- Parameters:
  - `target_cluster_count` (std::size_t): n/a

#### `ClusterAssignment cluster(const CrossSchoolTensionGraph &graph) const`
- Source: `include/ethics_ai/ethics_ai_types.h`:556
- Brief: n/a
- Parameters:
  - `graph` (const CrossSchoolTensionGraph &): n/a

### themis::plugins::ethics::EthicalArgument

#### `EthicalArgument()`
- Source: `include/ethics_ai/ethics_ai_types.h`:54
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::EthicalDecision

#### `EthicalDecision()`
- Source: `include/ethics_ai/ethics_ai_types.h`:112
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::EthicalDiscourseEngine

#### `EthicalDiscourseEngine(std::shared_ptr< PhilosophyLoader > philosophy_loader, std::shared_ptr< ArgumentStore > store, std::shared_ptr< RAGContextEngine > rag_engine)`
- Source: `src/ethics_ai/discourse_engine.h`:28
- Brief: n/a
- Parameters:
  - `philosophy_loader` (std::shared_ptr< PhilosophyLoader >): n/a
  - `store` (std::shared_ptr< ArgumentStore >): n/a
  - `rag_engine` (std::shared_ptr< RAGContextEngine >): n/a

#### `std::variant< DebateRound, Status > continueDebate(const std::string &debate_id, int round_number)`
- Source: `src/ethics_ai/discourse_engine.h`:54
- Brief: n/a
- Parameters:
  - `debate_id` (const std::string &): n/a
  - `round_number` (int): n/a

#### `EthicalArgument generateArgument(const PhilosophyProfile &profile, const std::string &dilemma, ArgumentType type)`
- Source: `src/ethics_ai/discourse_engine.h`:77
- Brief: Generate Argument.
- Parameters:
  - `profile` (const PhilosophyProfile &): Input parameter.
  - `dilemma` (const std::string &): Input parameter.
  - `type` (ArgumentType): Input parameter.
- Return: Return value.
- Details: profile Input parameter. dilemma Input parameter. type Input parameter. Return value. profile Input parameter. dilemma Input parameter. type Input parameter. Return value. Calls: std::chrono::system_clock::now(), std::chrono::system_clock::to_time_t(), rd(), str(), size(), empty(), find(), end().

#### `std::variant< DebateInitialization, Status > initializeDebate(const std::string &dilemma_description, const std::vector< std::string > &philosophy_schools, const std::string &category)`
- Source: `src/ethics_ai/discourse_engine.h`:35
- Brief: n/a
- Parameters:
  - `dilemma_description` (const std::string &): n/a
  - `philosophy_schools` (const std::vector< std::string > &): n/a
  - `category` (const std::string &): n/a

#### `std::variant< EthicalDecision, Status > makeDecision(const std::string &dilemma_description, const std::vector< std::string > &philosophy_schools, const std::string &category, bool use_rag)`
- Source: `src/ethics_ai/discourse_engine.h`:41
- Brief: n/a
- Parameters:
  - `dilemma_description` (const std::string &): n/a
  - `philosophy_schools` (const std::vector< std::string > &): n/a
  - `category` (const std::string &): n/a
  - `use_rag` (bool): n/a

#### `void setChainVisualizerOutputPath(const std::string &output_path)`
- Source: `src/ethics_ai/discourse_engine.h`:52
- Brief: Set Chain Visualizer Output Path.
- Parameters:
  - `output_path` (const std::string &): Path to the output.
- Details: output_path Path to the output. output_path Path to the output. Implements setChainVisualizerOutputPath without additional internal calls.

#### `std::string synthesizeDecision(const std::vector< EthicalArgument > &arguments, const std::string &primary_philosophy)`
- Source: `src/ethics_ai/discourse_engine.h`:89
- Brief: Synthesize Decision.
- Parameters:
  - `arguments` (const std::vector< EthicalArgument > &): Input parameter.
  - `primary_philosophy` (const std::string &): Input parameter.
- Return: Return value.
- Details: arguments Input parameter. primary_philosophy Input parameter. Return value. arguments Input parameter. primary_philosophy Input parameter. Return value. Calls: size(), str().

#### `~EthicalDiscourseEngine()=default`
- Source: `src/ethics_ai/discourse_engine.h`:33
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::EthicsAIPlugin

#### `EthicsAIPlugin()`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:60
- Brief: n/a
- Parameters: none

#### `std::variant< RAGContext, Status > buildRAGContext(const std::string &dilemma_description, const std::vector< std::string > &philosophy_schools, const std::string &category) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:266
- Brief: n/a
- Parameters:
  - `dilemma_description` (const std::string &): n/a
  - `philosophy_schools` (const std::vector< std::string > &): n/a
  - `category` (const std::string &): n/a

#### `std::variant< EthicsEvaluationResult, Status > evaluateDecision(const EthicalDecision &decision, const std::vector< EthicalArgument > &arguments) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:378
- Brief: n/a
- Parameters:
  - `decision` (const EthicalDecision &): n/a
  - `arguments` (const std::vector< EthicalArgument > &): n/a

#### `std::variant< std::vector< std::string >, Status > findSimilarDilemmas(const std::string &query_text, double threshold, size_t limit) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:280
- Brief: n/a
- Parameters:
  - `query_text` (const std::string &): n/a
  - `threshold` (double): n/a
  - `limit` (size_t): n/a

#### `std::variant< EthicalArgument, Status > getArgumentById(const std::string &argument_id) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:238
- Brief: n/a
- Parameters:
  - `argument_id` (const std::string &): n/a

#### `std::variant< ArgumentChain, Status > getArgumentChain(const std::string &chain_id) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:256
- Brief: n/a
- Parameters:
  - `chain_id` (const std::string &): n/a

#### `std::variant< std::vector< EthicalArgument >, Status > getArgumentsByPhilosophy(const std::string &philosophy_school, const std::vector< ArgumentType > &argument_types, size_t limit) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:224
- Brief: n/a
- Parameters:
  - `philosophy_school` (const std::string &): n/a
  - `argument_types` (const std::vector< ArgumentType > &): n/a
  - `limit` (size_t): n/a

#### `std::variant< std::vector< std::string >, Status > getBestPractices(const std::string &category, double min_satisfaction, size_t limit) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:292
- Brief: n/a
- Parameters:
  - `category` (const std::string &): n/a
  - `min_satisfaction` (double): n/a
  - `limit` (size_t): n/a

#### `PluginCapabilities getCapabilities() const override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:82
- Brief: n/a
- Parameters: none

#### `std::optional< std::string > getConfig(const std::string &key) const override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:510
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::string getDashboardJSON() const override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:467
- Brief: n/a
- Parameters: none

#### `std::variant< EthicalDecision, Status > getDecision(const std::string &decision_id) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:368
- Brief: n/a
- Parameters:
  - `decision_id` (const std::string &): n/a

#### `void * getInstance() override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:169
- Brief: n/a
- Parameters: none

#### `const char * getName() const override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:70
- Brief: n/a
- Parameters: none

#### `std::variant< PhilosophyProfile, Status > getPhilosophyProfile(const std::string &school_id) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:416
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a

#### `std::string getPrometheusMetrics() const override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:434
- Brief: n/a
- Parameters: none

#### `std::map< std::string, double > getStatistics() const override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:487
- Brief: n/a
- Parameters: none

#### `PluginType getType() const override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:78
- Brief: n/a
- Parameters: none

#### `const char * getVersion() const override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:74
- Brief: n/a
- Parameters: none

#### `bool initialize(const char *config_json) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:92
- Brief: n/a
- Parameters:
  - `config_json` (const char *): n/a

#### `std::variant< DebateInitialization, Status > initializeDebate(const std::string &dilemma_description, const std::vector< std::string > &philosophy_schools, const std::string &category) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:175
- Brief: n/a
- Parameters:
  - `dilemma_description` (const std::string &): n/a
  - `philosophy_schools` (const std::vector< std::string > &): n/a
  - `category` (const std::string &): n/a

#### `std::vector< std::string > listPhilosophySchools() const override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:426
- Brief: n/a
- Parameters: none

#### `std::variant< size_t, Status > loadPhilosophyProfiles(const std::string &philosophy_dir) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:406
- Brief: n/a
- Parameters:
  - `philosophy_dir` (const std::string &): n/a

#### `std::variant< EthicalDecision, Status > makeDecision(const std::string &dilemma_description, const std::vector< std::string > &philosophy_schools, const std::string &category, bool use_rag) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:333
- Brief: n/a
- Parameters:
  - `dilemma_description` (const std::string &): n/a
  - `philosophy_schools` (const std::vector< std::string > &): n/a
  - `category` (const std::string &): n/a
  - `use_rag` (bool): n/a

#### `Status setConfig(const std::string &key, const std::string &value) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:505
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

#### `void setEthicalGuidelinesManager(void *manager) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:518
- Brief: Set Ethical Guidelines Manager.
- Parameters:
  - `manager` (void *): Input/output parameter.
- Details: manager Input/output parameter.

#### `void shutdown() override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:151
- Brief: n/a
- Parameters: none

#### `Status storeArgument(const EthicalArgument &argument, bool store_vector) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:201
- Brief: n/a
- Parameters:
  - `argument` (const EthicalArgument &): n/a
  - `store_vector` (bool): n/a

#### `Status storeArgumentChain(const ArgumentChain &chain) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:248
- Brief: n/a
- Parameters:
  - `chain` (const ArgumentChain &): n/a

#### `Status storeDecision(const EthicalDecision &decision) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:360
- Brief: n/a
- Parameters:
  - `decision` (const EthicalDecision &): n/a

#### `std::variant< std::vector< std::string >, Status > traverseArgumentChain(const std::string &start_argument_id, size_t max_depth, const std::string &direction) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:319
- Brief: n/a
- Parameters:
  - `start_argument_id` (const std::string &): n/a
  - `max_depth` (size_t): n/a
  - `direction` (const std::string &): n/a

#### `std::variant< std::vector< std::pair< std::string, double > >, Status > vectorSemanticSearch(const std::vector< float > &query_embedding, const std::string &philosophy_school, size_t limit) override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:305
- Brief: n/a
- Parameters:
  - `query_embedding` (const std::vector< float > &): n/a
  - `philosophy_school` (const std::string &): n/a
  - `limit` (size_t): n/a

#### `~EthicsAIPlugin() override`
- Source: `src/ethics_ai/ethics_ai_plugin.cpp`:62
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::EthicsAQLQueries

#### `std::string buildRAGContext()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:188
- Brief: ========== RAG Context Queries ==========
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: VECTOR_COSINE_SIMILARITY().

#### `std::string findConsensusDecisions()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:140
- Brief: Find Consensus Decisions.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements findConsensusDecisions without additional internal calls.

#### `std::string findShortestPath()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:103
- Brief: Find Shortest Path.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements findShortestPath without additional internal calls.

#### `std::string findSimilarDilemmas()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:57
- Brief: Find Similar Dilemmas.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: VECTOR_COSINE_SIMILARITY().

#### `std::string getArgumentById()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:30
- Brief: ========== Argument Queries ==========
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getArgumentById without additional internal calls.

#### `std::string getArgumentsByPhilosophy()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:39
- Brief: Get Arguments By Philosophy.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getArgumentsByPhilosophy without additional internal calls.

#### `std::string getArgumentsByPhilosophyAndType()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:48
- Brief: Get Arguments By Philosophy And Type.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getArgumentsByPhilosophyAndType without additional internal calls.

#### `std::string getBestPractices()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:150
- Brief: ========== Best Practice Queries ==========
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getBestPractices without additional internal calls.

#### `std::string getCounteringArguments()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:94
- Brief: Get Countering Arguments.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getCounteringArguments without additional internal calls.

#### `std::string getDecisionById()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:113
- Brief: ========== Decision Queries ==========
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getDecisionById without additional internal calls.

#### `std::string getDecisionsByCategory()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:122
- Brief: Get Decisions By Category.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getDecisionsByCategory without additional internal calls.

#### `std::string getPhilosophyProfile()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:169
- Brief: ========== Philosophy Profile Queries ==========
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getPhilosophyProfile without additional internal calls.

#### `std::string getPhilosophyStatistics()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:159
- Brief: Get Philosophy Statistics.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getPhilosophyStatistics without additional internal calls.

#### `std::string getRecentDebates()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:131
- Brief: Get Recent Debates.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getRecentDebates without additional internal calls.

#### `std::string getSupportingArguments()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:85
- Brief: Get Supporting Arguments.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements getSupportingArguments without additional internal calls.

#### `std::string listPhilosophySchools()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:178
- Brief: List Philosophy Schools.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements listPhilosophySchools without additional internal calls.

#### `std::string searchArgumentsByContent()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:66
- Brief: Search Arguments By Content.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: CONTAINS(), LOWER().

#### `std::string traverseArgumentChain()`
- Source: `src/ethics_ai/ethics_aql_queries.h`:76
- Brief: ========== Graph Traversal Queries ==========
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: LENGTH().

### themis::plugins::ethics::EthicsAuditLog

#### `EthicsAuditLog()=default`
- Source: `include/ethics_ai/ethics_ai_types.h`:445
- Brief: n/a
- Parameters: none

#### `EthicsAuditLog(EthicsAuditLog &&) noexcept=default`
- Source: `include/ethics_ai/ethics_ai_types.h`:450
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAuditLog &&): n/a

#### `EthicsAuditLog(const EthicsAuditLog &)=delete`
- Source: `include/ethics_ai/ethics_ai_types.h`:448
- Brief: n/a
- Parameters:
  - `<unnamed>` (const EthicsAuditLog &): n/a

#### `size_t append(RoundAuditEntry entry)`
- Source: `include/ethics_ai/ethics_ai_types.h`:459
- Brief: Append.
- Parameters:
  - `entry` (RoundAuditEntry): Input parameter.
- Return: Return value.
- Details: entry Input parameter. Return value. Calls: lock(), size(), push_back(), std::move().

#### `bool empty() const noexcept`
- Source: `include/ethics_ai/ethics_ai_types.h`:497
- Brief: n/a
- Parameters: none

#### `std::vector< RoundAuditEntry > exportAuditLog() const`
- Source: `include/ethics_ai/ethics_ai_types.h`:477
- Brief: n/a
- Parameters: none

#### `EthicsAuditLog & operator=(EthicsAuditLog &&) noexcept=default`
- Source: `include/ethics_ai/ethics_ai_types.h`:451
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsAuditLog &&): n/a

#### `EthicsAuditLog & operator=(const EthicsAuditLog &)=delete`
- Source: `include/ethics_ai/ethics_ai_types.h`:449
- Brief: n/a
- Parameters:
  - `<unnamed>` (const EthicsAuditLog &): n/a

#### `size_t size() const noexcept`
- Source: `include/ethics_ai/ethics_ai_types.h`:487
- Brief: n/a
- Parameters: none

#### `AuditError tryErase(size_t index) const noexcept`
- Source: `include/ethics_ai/ethics_ai_types.h`:472
- Brief: n/a
- Parameters:
  - `index` (size_t): n/a

#### `AuditError tryOverwrite(size_t index, const RoundAuditEntry &replacement) const noexcept`
- Source: `include/ethics_ai/ethics_ai_types.h`:466
- Brief: n/a
- Parameters:
  - `index` (size_t): n/a
  - `replacement` (const RoundAuditEntry &): n/a

### themis::plugins::ethics::EthicsBaseEntityAdapter

#### `EthicalArgument fromBaseEntity(const BaseEntity &entity)`
- Source: `src/ethics_ai/ethics_base_entity_adapter.h`:71
- Brief: From Base Entity.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. Return value. Calls: getPrimaryKey(), getFieldAsString(), value_or(), stringToArgumentType(), stringToArgumentStrength(), getFieldAsInt(), std::chrono::system_clock::from_time_t(), nlohmann::json::parse().

#### `EthicalDecision fromBaseEntity(const BaseEntity &entity, bool is_decision)`
- Source: `src/ethics_ai/ethics_base_entity_adapter.h`:174
- Brief: From Base Entity.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
  - `is_decision` (bool): Input parameter.
- Return: Return value.
- Details: entity Input parameter. is_decision Input parameter. Return value. Calls: getPrimaryKey(), getFieldAsString(), value_or(), getFieldAsDouble(), getFieldAsInt(), std::chrono::system_clock::from_time_t(), nlohmann::json::parse(), THEMIS_WARN().

#### `PhilosophyProfile fromBaseEntityToProfile(const BaseEntity &entity)`
- Source: `src/ethics_ai/ethics_base_entity_adapter.h`:275
- Brief: From Base Entity To Profile.
- Parameters:
  - `entity` (const BaseEntity &): Input parameter.
- Return: Return value.
- Details: entity Input parameter. Return value. Calls: getPrimaryKey(), getFieldAsString(), value_or(), nlohmann::json::parse(), THEMIS_WARN(), what(), parse_string_vec(), parse_string_map().

#### `std::string makeArgumentKey(const std::string &argument_id)`
- Source: `src/ethics_ai/ethics_base_entity_adapter.h`:128
- Brief: Make Argument Key.
- Parameters:
  - `argument_id` (const std::string &): Identifier of the argument.
- Return: Return value.
- Details: argument_id Identifier of the argument. Return value. Implements makeArgumentKey without additional internal calls.

#### `std::string makeDebateKey(const std::string &debate_id)`
- Source: `src/ethics_ai/ethics_base_entity_adapter.h`:357
- Brief: Make Debate Key.
- Parameters:
  - `debate_id` (const std::string &): Identifier of the debate.
- Return: Return value.
- Details: debate_id Identifier of the debate. Return value. Implements makeDebateKey without additional internal calls.

#### `std::string makeDecisionKey(const std::string &decision_id)`
- Source: `src/ethics_ai/ethics_base_entity_adapter.h`:217
- Brief: Make Decision Key.
- Parameters:
  - `decision_id` (const std::string &): Identifier of the decision.
- Return: Return value.
- Details: decision_id Identifier of the decision. Return value. Implements makeDecisionKey without additional internal calls.

#### `std::string makeProfileKey(const std::string &school)`
- Source: `src/ethics_ai/ethics_base_entity_adapter.h`:319
- Brief: Make Profile Key.
- Parameters:
  - `school` (const std::string &): Input parameter.
- Return: Return value.
- Details: school Input parameter. Return value. Implements makeProfileKey without additional internal calls.

#### `BaseEntity toBaseEntity(const DebateInitialization &debate)`
- Source: `src/ethics_ai/ethics_base_entity_adapter.h`:330
- Brief: ========== Debate Initialization Conversion ==========
- Parameters:
  - `debate` (const DebateInitialization &): Input parameter.
- Return: Return value.
- Details: debate Input parameter. Return value. Calls: time_since_epoch(), count(), empty(), dump(), BaseEntity::fromFields().

#### `BaseEntity toBaseEntity(const EthicalArgument &argument)`
- Source: `src/ethics_ai/ethics_base_entity_adapter.h`:33
- Brief: ========== Ethical Argument Conversion ==========
- Parameters:
  - `argument` (const EthicalArgument &): Input parameter.
- Return: Return value.
- Details: argument Input parameter. Return value. Calls: argumentTypeToString(), argumentStrengthToString(), time_since_epoch(), count(), empty(), dump(), BaseEntity::fromFields().

#### `BaseEntity toBaseEntity(const EthicalDecision &decision)`
- Source: `src/ethics_ai/ethics_base_entity_adapter.h`:139
- Brief: ========== Ethical Decision Conversion ==========
- Parameters:
  - `decision` (const EthicalDecision &): Input parameter.
- Return: Return value.
- Details: decision Input parameter. Return value. Calls: time_since_epoch(), count(), empty(), dump(), BaseEntity::fromFields().

#### `BaseEntity toBaseEntity(const PhilosophyProfile &profile)`
- Source: `src/ethics_ai/ethics_base_entity_adapter.h`:228
- Brief: ========== Philosophy Profile Conversion ==========
- Parameters:
  - `profile` (const PhilosophyProfile &): Input parameter.
- Return: Return value.
- Details: profile Input parameter. Return value. Calls: dump(), BaseEntity::fromFields().

### themis::plugins::ethics::EthicsError

#### `bool isOk() const noexcept`
- Source: `include/ethics_ai/ethics_ai_types.h`:264
- Brief: n/a
- Parameters: none

#### `EthicsError ok() noexcept`
- Source: `include/ethics_ai/ethics_ai_types.h`:260
- Brief: n/a
- Parameters: none

#### `operator bool() const noexcept`
- Source: `include/ethics_ai/ethics_ai_types.h`:268
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::EthicsEvaluationResult

#### `EthicsEvaluationResult()`
- Source: `include/ethics_ai/ethics_ai_types.h`:162
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::EthicsEvaluator

#### `EthicsEvaluator()=default`
- Source: `src/ethics_ai/ethics_evaluator.h`:36
- Brief: n/a
- Parameters: none

#### `EthicsEvaluator(const Config &config)`
- Source: `src/ethics_ai/ethics_evaluator.h`:43
- Brief: Ethics Evaluator.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `double computeConfidence(const std::vector< EthicalArgument > &arguments)`
- Source: `src/ethics_ai/ethics_evaluator.h`:57
- Brief: Compute Confidence.
- Parameters:
  - `arguments` (const std::vector< EthicalArgument > &): Input parameter.
- Return: Return value.
- Details: arguments Input parameter. Return value. arguments Input parameter. Return value. Calls: empty(), strengthToScore(), size().

#### `double computeConsensus(const std::vector< EthicalArgument > &arguments)`
- Source: `src/ethics_ai/ethics_evaluator.h`:64
- Brief: Compute Consensus.
- Parameters:
  - `arguments` (const std::vector< EthicalArgument > &): Input parameter.
- Return: Return value.
- Details: arguments Input parameter. Return value. arguments Input parameter. Return value. Calls: empty(), size().

#### `double evaluateAlignment(const EthicalDecision &decision, const std::vector< EthicalArgument > &arguments)`
- Source: `src/ethics_ai/ethics_evaluator.h`:140
- Brief: Evaluate Alignment.
- Parameters:
  - `decision` (const EthicalDecision &): Input parameter.
  - `arguments` (const std::vector< EthicalArgument > &): Input parameter.
- Return: Return value.
- Details: decision Input parameter. arguments Input parameter. Return value. decision Input parameter. arguments Input parameter. Return value. Calls: empty(), size(), std::min().

#### `double evaluateConsistency(const EthicalDecision &decision, const std::vector< EthicalArgument > &arguments)`
- Source: `src/ethics_ai/ethics_evaluator.h`:118
- Brief: Evaluate Consistency.
- Parameters:
  - `decision` (const EthicalDecision &): Input parameter.
  - `arguments` (const std::vector< EthicalArgument > &): Input parameter.
- Return: Return value.
- Details: decision Input parameter. arguments Input parameter. Return value. decision Input parameter. arguments Input parameter. Return value. Calls: empty(), size(), std::min().

#### `std::variant< EthicsEvaluationResult, Status > evaluateDecision(const EthicalDecision &decision, const std::vector< EthicalArgument > &arguments)`
- Source: `src/ethics_ai/ethics_evaluator.h`:47
- Brief: n/a
- Parameters:
  - `decision` (const EthicalDecision &): n/a
  - `arguments` (const std::vector< EthicalArgument > &): n/a

#### `double evaluateDecisionQuality(const EthicalDecision &decision, const std::vector< EthicalArgument > &arguments)`
- Source: `src/ethics_ai/ethics_evaluator.h`:107
- Brief: Evaluate Decision Quality.
- Parameters:
  - `decision` (const EthicalDecision &): Input parameter.
  - `arguments` (const std::vector< EthicalArgument > &): Input parameter.
- Return: Return value.
- Details: decision Input parameter. arguments Input parameter. Return value. decision Input parameter. arguments Input parameter. Return value. Calls: empty(), std::min(), size().

#### `double evaluateFairness(const EthicalDecision &decision, const std::vector< EthicalArgument > &arguments)`
- Source: `src/ethics_ai/ethics_evaluator.h`:129
- Brief: Evaluate Fairness.
- Parameters:
  - `decision` (const EthicalDecision &): Input parameter.
  - `arguments` (const std::vector< EthicalArgument > &): Input parameter.
- Return: Return value.
- Details: decision Input parameter. arguments Input parameter. Return value. decision Input parameter. arguments Input parameter. Return value. Calls: size(), empty(), insert(), std::min().

#### `double evaluateTransparency(const EthicalDecision &decision, const std::vector< EthicalArgument > &arguments)`
- Source: `src/ethics_ai/ethics_evaluator.h`:151
- Brief: Evaluate Transparency.
- Parameters:
  - `decision` (const EthicalDecision &): Input parameter.
  - `arguments` (const std::vector< EthicalArgument > &): Input parameter.
- Return: Return value.
- Details: decision Input parameter. arguments Input parameter. Return value.

#### `std::string getMetricsText() const`
- Source: `src/ethics_ai/ethics_evaluator.h`:88
- Brief: Get Metrics Text.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void recordDecision(double confidence, bool rag_hit, uint64_t latency_ms)`
- Source: `src/ethics_ai/ethics_evaluator.h`:76
- Brief: Record Decision.
- Parameters:
  - `confidence` (double): Input parameter.
  - `rag_hit` (bool): Input parameter.
  - `latency_ms` (uint64_t): Input parameter.
- Details: confidence Input parameter. rag_hit Input parameter. latency_ms Input parameter. confidence Input parameter. rag_hit Input parameter. latency_ms Input parameter. Implements recordDecision without additional internal calls.

#### `void setArgumentStoreSize(uint64_t count)`
- Source: `src/ethics_ai/ethics_evaluator.h`:82
- Brief: Set Argument Store Size.
- Parameters:
  - `count` (uint64_t): Input parameter.
- Details: count Input parameter.

#### `~EthicsEvaluator()=default`
- Source: `src/ethics_ai/ethics_evaluator.h`:45
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::EthicsProfileMeta

#### `EthicsProfileMeta()=default`
- Source: `include/ethics_ai/ethics_profile_registry.h`:32
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::EthicsProfileRegistry

#### `EthicsProfileRegistry(size_t lru_capacity=20)`
- Source: `src/ethics_ai/ethics_profile_registry.h`:28
- Brief: n/a
- Parameters:
  - `lru_capacity` (size_t): n/a

#### `std::variant< PhilosophyProfile, Status > getProfile(const std::string &school_id) override`
- Source: `src/ethics_ai/ethics_profile_registry.h`:35
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a

#### `bool hasProfile(const std::string &school_id) const override`
- Source: `src/ethics_ai/ethics_profile_registry.h`:42
- Brief: Has Profile.
- Parameters:
  - `school_id` (const std::string &): Identifier of the school.
- Return: True when the operation succeeds.
- Details: school_id Identifier of the school. True when the operation succeeds.

#### `size_t indexSize() const override`
- Source: `src/ethics_ai/ethics_profile_registry.h`:41
- Brief: Index Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void lruEvict()`
- Source: `src/ethics_ai/ethics_profile_registry.h`:65
- Brief: Lru Evict.
- Parameters: none

#### `const PhilosophyProfile * lruGet(const std::string &id)`
- Source: `src/ethics_ai/ethics_profile_registry.h`:61
- Brief: Lru Get.
- Parameters:
  - `id` (const std::string &): Input parameter.
- Return: Pointer to the result.
- Details: id Input parameter. Pointer to the result.

#### `void lruPut(const std::string &id, const PhilosophyProfile &profile)`
- Source: `src/ethics_ai/ethics_profile_registry.h`:55
- Brief: Lru Put.
- Parameters:
  - `id` (const std::string &): Input parameter.
  - `profile` (const PhilosophyProfile &): Input parameter.
- Details: id Input parameter. profile Input parameter.

#### `std::vector< EthicsProfileMeta > queryIndex(const EthicsIndexQuery &query) const override`
- Source: `src/ethics_ai/ethics_profile_registry.h`:32
- Brief: Query Index.
- Parameters:
  - `query` (const EthicsIndexQuery &): Input parameter.
- Return: Return value.
- Details: query Input parameter. Return value.

#### `std::variant< size_t, Status > rebuildIndex(const std::string &directory) override`
- Source: `src/ethics_ai/ethics_profile_registry.h`:38
- Brief: n/a
- Parameters:
  - `directory` (const std::string &): n/a

#### `EthicsProfileMeta scanHeader(const std::string &filepath)`
- Source: `src/ethics_ai/ethics_profile_registry.h`:72
- Brief: ── Internal ─────────────────────────────────────────────────────────────
- Parameters:
  - `filepath` (const std::string &): Input parameter.
- Return: Return value.
- Details: Scan Header. filepath Input parameter. Return value.

#### `~EthicsProfileRegistry() override=default`
- Source: `src/ethics_ai/ethics_profile_registry.h`:29
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::EthicsSelectionRouter

#### `EthicsSelectionRouter(EthicsSelectionRouter &&) noexcept=default`
- Source: `include/ethics_ai/ethics_selection_router.h`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsSelectionRouter &&): n/a

#### `EthicsSelectionRouter(IEthicsProfileRegistry *registry, const RouterConfig &config={})`
- Source: `include/ethics_ai/ethics_selection_router.h`:105
- Brief: n/a
- Parameters:
  - `registry` (IEthicsProfileRegistry *): n/a
  - `config` (const RouterConfig &): n/a

#### `EthicsSelectionRouter(const EthicsSelectionRouter &)=delete`
- Source: `include/ethics_ai/ethics_selection_router.h`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (const EthicsSelectionRouter &): n/a

#### `const RouterConfig & config() const`
- Source: `include/ethics_ai/ethics_selection_router.h`:139
- Brief: Config.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `EthicsSelectionRouter & operator=(EthicsSelectionRouter &&) noexcept=default`
- Source: `include/ethics_ai/ethics_selection_router.h`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (EthicsSelectionRouter &&): n/a

#### `EthicsSelectionRouter & operator=(const EthicsSelectionRouter &)=delete`
- Source: `include/ethics_ai/ethics_selection_router.h`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (const EthicsSelectionRouter &): n/a

#### `DiscourseOrchestratorPlan planDiscourse(const std::string &domain_context={}) const`
- Source: `include/ethics_ai/ethics_selection_router.h`:116
- Brief: n/a
- Parameters:
  - `domain_context` (const std::string &): n/a

#### `void recordDecisionOutcome(const std::string &dilemma_type, const std::string &school_id, double dc_score)`
- Source: `include/ethics_ai/ethics_selection_router.h`:131
- Brief: Record Decision Outcome.
- Parameters:
  - `dilemma_type` (const std::string &): Input parameter.
  - `school_id` (const std::string &): Identifier of the school.
  - `dc_score` (double): Input parameter.
- Details: dilemma_type Input parameter. school_id Identifier of the school. dc_score Input parameter.

#### `RouterResult route(const std::string &dilemma_text, const std::string &dilemma_domain, const std::vector< std::string > &dilemma_tags={}, bool regulatory_context=false) const`
- Source: `include/ethics_ai/ethics_selection_router.h`:119
- Brief: n/a
- Parameters:
  - `dilemma_text` (const std::string &): n/a
  - `dilemma_domain` (const std::string &): n/a
  - `dilemma_tags` (const std::vector< std::string > &): n/a
  - `regulatory_context` (bool): n/a

#### `void setEmbeddingFn(EmbeddingFn fn)`
- Source: `include/ethics_ai/ethics_selection_router.h`:145
- Brief: Set Embedding Fn.
- Parameters:
  - `fn` (EmbeddingFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: std::move().

#### `void setPrecedentQueryFn(PrecedentQueryFn fn)`
- Source: `include/ethics_ai/ethics_selection_router.h`:151
- Brief: Set Precedent Query Fn.
- Parameters:
  - `fn` (PrecedentQueryFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: std::move().

#### `~EthicsSelectionRouter()`
- Source: `include/ethics_ai/ethics_selection_router.h`:108
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::EthicsSelectionRouter::Impl

#### `void loadTaxonomy(const std::string &yaml_path)`
- Source: `src/ethics_ai/ethics_selection_router.cpp`:182
- Brief: Load Taxonomy.
- Parameters:
  - `yaml_path` (const std::string &): Path to the yaml.
- Details: yaml_path Path to the yaml.

#### `std::set< std::string > stage1(const std::string &domain, const std::vector< std::string > &tags, bool regulatory_context) const`
- Source: `src/ethics_ai/ethics_selection_router.cpp`:190
- Brief: Stage1.
- Parameters:
  - `domain` (const std::string &): Input parameter.
  - `tags` (const std::vector< std::string > &): Input parameter.
  - `regulatory_context` (bool): Input parameter.
- Return: Return value.
- Details: domain Input parameter. tags Input parameter. regulatory_context Input parameter. Return value.

#### `std::vector< RouterCandidate > stage2(const std::string &dilemma_text, const std::set< std::string > &candidates) const`
- Source: `src/ethics_ai/ethics_selection_router.cpp`:200
- Brief: Stage2.
- Parameters:
  - `dilemma_text` (const std::string &): Input parameter.
  - `candidates` (const std::set< std::string > &): Input parameter.
- Return: Return value.
- Details: dilemma_text Input parameter. candidates Input parameter. Return value.

#### `void stage3(std::vector< RouterCandidate > &candidates, const std::string &dilemma_domain) const`
- Source: `src/ethics_ai/ethics_selection_router.cpp`:208
- Brief: Stage3.
- Parameters:
  - `candidates` (std::vector< RouterCandidate > &): Input/output parameter.
  - `dilemma_domain` (const std::string &): Input parameter.
- Details: candidates Input/output parameter. dilemma_domain Input parameter.

### themis::plugins::ethics::IAdaLoRABiasCorrector

#### `double applyBiasCorrection(const std::string &school_id, double raw_score) const noexcept=0`
- Source: `include/ethics_ai/ethics_ai_types.h`:591
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a
  - `raw_score` (double): n/a

#### `bool hasAdapter(const std::string &school_id) const noexcept=0`
- Source: `include/ethics_ai/ethics_ai_types.h`:594
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a

#### `~IAdaLoRABiasCorrector()=default`
- Source: `include/ethics_ai/ethics_ai_types.h`:589
- Brief: IAda Lo RABias Corrector.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::plugins::ethics::IEthicsAIPlugin

#### `std::variant< RAGContext, Status > buildRAGContext(const std::string &dilemma_description, const std::vector< std::string > &philosophy_schools, const std::string &category="general")=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:64
- Brief: n/a
- Parameters:
  - `dilemma_description` (const std::string &): n/a
  - `philosophy_schools` (const std::vector< std::string > &): n/a
  - `category` (const std::string &): n/a

#### `std::variant< EthicsEvaluationResult, Status > evaluateDecision(const EthicalDecision &decision, const std::vector< EthicalArgument > &arguments={})=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:112
- Brief: n/a
- Parameters:
  - `decision` (const EthicalDecision &): n/a
  - `arguments` (const std::vector< EthicalArgument > &): n/a

#### `std::variant< std::vector< std::string >, Status > findSimilarDilemmas(const std::string &query_text, double threshold=0.65, size_t limit=10)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:70
- Brief: n/a
- Parameters:
  - `query_text` (const std::string &): n/a
  - `threshold` (double): n/a
  - `limit` (size_t): n/a

#### `std::variant< EthicalArgument, Status > getArgumentById(const std::string &argument_id)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:52
- Brief: n/a
- Parameters:
  - `argument_id` (const std::string &): n/a

#### `std::variant< ArgumentChain, Status > getArgumentChain(const std::string &chain_id)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:58
- Brief: n/a
- Parameters:
  - `chain_id` (const std::string &): n/a

#### `std::variant< std::vector< EthicalArgument >, Status > getArgumentsByPhilosophy(const std::string &philosophy_school, const std::vector< ArgumentType > &argument_types={}, size_t limit=20)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:46
- Brief: n/a
- Parameters:
  - `philosophy_school` (const std::string &): n/a
  - `argument_types` (const std::vector< ArgumentType > &): n/a
  - `limit` (size_t): n/a

#### `std::variant< std::vector< std::string >, Status > getBestPractices(const std::string &category, double min_satisfaction=0.8, size_t limit=10)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:76
- Brief: n/a
- Parameters:
  - `category` (const std::string &): n/a
  - `min_satisfaction` (double): n/a
  - `limit` (size_t): n/a

#### `std::optional< std::string > getConfig(const std::string &key) const =0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:141
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a

#### `std::string getDashboardJSON() const =0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:133
- Brief: n/a
- Parameters: none

#### `std::variant< EthicalDecision, Status > getDecision(const std::string &decision_id)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:106
- Brief: n/a
- Parameters:
  - `decision_id` (const std::string &): n/a

#### `std::variant< PhilosophyProfile, Status > getPhilosophyProfile(const std::string &school_id)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:123
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a

#### `std::string getPrometheusMetrics() const =0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:131
- Brief: n/a
- Parameters: none

#### `std::map< std::string, double > getStatistics() const =0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:135
- Brief: n/a
- Parameters: none

#### `std::variant< DebateInitialization, Status > initializeDebate(const std::string &dilemma_description, const std::vector< std::string > &philosophy_schools, const std::string &category="general")=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:33
- Brief: n/a
- Parameters:
  - `dilemma_description` (const std::string &): n/a
  - `philosophy_schools` (const std::vector< std::string > &): n/a
  - `category` (const std::string &): n/a

#### `std::vector< std::string > listPhilosophySchools() const =0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:127
- Brief: n/a
- Parameters: none

#### `std::variant< size_t, Status > loadPhilosophyProfiles(const std::string &philosophy_dir)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:119
- Brief: n/a
- Parameters:
  - `philosophy_dir` (const std::string &): n/a

#### `std::variant< EthicalDecision, Status > makeDecision(const std::string &dilemma_description, const std::vector< std::string > &philosophy_schools, const std::string &category="general", bool use_rag=true)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:97
- Brief: n/a
- Parameters:
  - `dilemma_description` (const std::string &): n/a
  - `philosophy_schools` (const std::vector< std::string > &): n/a
  - `category` (const std::string &): n/a
  - `use_rag` (bool): n/a

#### `Status setConfig(const std::string &key, const std::string &value)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:139
- Brief: n/a
- Parameters:
  - `key` (const std::string &): n/a
  - `value` (const std::string &): n/a

#### `void setEthicalGuidelinesManager(void *manager)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:148
- Brief: Set Ethical Guidelines Manager.
- Parameters:
  - `manager` (void *): Input/output parameter.
- Details: manager Input/output parameter.

#### `Status storeArgument(const EthicalArgument &argument, bool store_vector=true)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:41
- Brief: n/a
- Parameters:
  - `argument` (const EthicalArgument &): n/a
  - `store_vector` (bool): n/a

#### `Status storeArgumentChain(const ArgumentChain &chain)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:56
- Brief: n/a
- Parameters:
  - `chain` (const ArgumentChain &): n/a

#### `Status storeDecision(const EthicalDecision &decision)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:104
- Brief: n/a
- Parameters:
  - `decision` (const EthicalDecision &): n/a

#### `std::variant< std::vector< std::string >, Status > traverseArgumentChain(const std::string &start_argument_id, size_t max_depth=5, const std::string &direction="both")=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:89
- Brief: n/a
- Parameters:
  - `start_argument_id` (const std::string &): n/a
  - `max_depth` (size_t): n/a
  - `direction` (const std::string &): n/a

#### `std::variant< std::vector< std::pair< std::string, double > >, Status > vectorSemanticSearch(const std::vector< float > &query_embedding, const std::string &philosophy_school="", size_t limit=20)=0`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:83
- Brief: n/a
- Parameters:
  - `query_embedding` (const std::vector< float > &): n/a
  - `philosophy_school` (const std::string &): n/a
  - `limit` (size_t): n/a

#### `~IEthicsAIPlugin()=default`
- Source: `include/ethics_ai/ethics_ai_plugin_interface.h`:29
- Brief: IEthics AIPlugin.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::plugins::ethics::IEthicsProfileRegistry

#### `std::variant< PhilosophyProfile, Status > getProfile(const std::string &school_id)=0`
- Source: `include/ethics_ai/ethics_profile_registry.h`:58
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a

#### `bool hasProfile(const std::string &school_id) const =0`
- Source: `include/ethics_ai/ethics_profile_registry.h`:75
- Brief: Has Profile.
- Parameters:
  - `school_id` (const std::string &): Identifier of the school.
- Return: True when the operation succeeds.
- Details: school_id Identifier of the school. True when the operation succeeds.

#### `size_t indexSize() const =0`
- Source: `include/ethics_ai/ethics_profile_registry.h`:68
- Brief: Index Size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< EthicsProfileMeta > queryIndex(const EthicsIndexQuery &query) const =0`
- Source: `include/ethics_ai/ethics_profile_registry.h`:55
- Brief: Query Index.
- Parameters:
  - `query` (const EthicsIndexQuery &): Input parameter.
- Return: Return value.
- Details: query Input parameter. Return value.

#### `std::variant< size_t, Status > rebuildIndex(const std::string &directory)=0`
- Source: `include/ethics_ai/ethics_profile_registry.h`:61
- Brief: n/a
- Parameters:
  - `directory` (const std::string &): n/a

#### `~IEthicsProfileRegistry()=default`
- Source: `include/ethics_ai/ethics_profile_registry.h`:48
- Brief: IEthics Profile Registry.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::plugins::ethics::ILlmCascadeRouter

#### `ModelTokenBudget budgetForRound(const std::string &round_role) const =0`
- Source: `include/ethics_ai/llm_cascade_router.h`:96
- Brief: Budget For Round.
- Parameters:
  - `round_role` (const std::string &): Input parameter.
- Return: Return value.
- Details: round_role Input parameter. Return value.

#### `CascadeRoutingDecision routeForRound(const std::string &round_role, size_t estimated_prompt_tokens) const =0`
- Source: `include/ethics_ai/llm_cascade_router.h`:87
- Brief: Route For Round.
- Parameters:
  - `round_role` (const std::string &): Input parameter.
  - `estimated_prompt_tokens` (size_t): Input parameter.
- Return: Return value.
- Details: round_role Input parameter. estimated_prompt_tokens Input parameter. Return value.

#### `CascadeModelTier tierForRound(const std::string &round_role) const noexcept=0`
- Source: `include/ethics_ai/llm_cascade_router.h`:105
- Brief: Tier For Round.
- Parameters:
  - `round_role` (const std::string &): Input parameter.
- Return: Return value.
- Details: round_role Input parameter. Return value. Exception safety: noexcept.

#### `~ILlmCascadeRouter()=default`
- Source: `include/ethics_ai/llm_cascade_router.h`:79
- Brief: ILlm Cascade Router.
- Parameters: none
- Return: Return value.
- Details: Return value.

### themis::plugins::ethics::IdentityAdaLoRABiasCorrector

#### `double applyBiasCorrection(const std::string &, double raw_score) const noexcept override`
- Source: `include/ethics_ai/ethics_ai_types.h`:599
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a
  - `raw_score` (double): n/a

#### `bool hasAdapter(const std::string &) const noexcept override`
- Source: `include/ethics_ai/ethics_ai_types.h`:605
- Brief: n/a
- Parameters:
  - `<unnamed>` (const std::string &): n/a

### themis::plugins::ethics::LlmCascadeRouter

#### `LlmCascadeRouter(CascadeRoutingConfig config=CascadeRoutingConfig::defaultConfig())`
- Source: `include/ethics_ai/llm_cascade_router.h`:111
- Brief: n/a
- Parameters:
  - `config` (CascadeRoutingConfig): n/a

#### `ModelTokenBudget budgetForRound(const std::string &round_role) const override`
- Source: `include/ethics_ai/llm_cascade_router.h`:117
- Brief: Budget For Round.
- Parameters:
  - `round_role` (const std::string &): Input parameter.
- Return: Return value.
- Details: round_role Input parameter. Return value.

#### `ModelTokenBudget budgetForTier(CascadeModelTier tier) const noexcept`
- Source: `include/ethics_ai/llm_cascade_router.h`:161
- Brief: Budget For Tier.
- Parameters:
  - `tier` (CascadeModelTier): Input parameter.
- Return: Return value.
- Details: tier Input parameter. Return value. Exception safety: noexcept.

#### `const CascadeRoutingConfig & config() const noexcept`
- Source: `include/ethics_ai/llm_cascade_router.h`:123
- Brief: n/a
- Parameters: none

#### `std::string invoke(const std::string &round_role, const std::string &prompt) const`
- Source: `include/ethics_ai/llm_cascade_router.h`:142
- Brief: Invoke.
- Parameters:
  - `round_role` (const std::string &): Input parameter.
  - `prompt` (const std::string &): Input parameter.
- Return: Return value.
- Details: round_role Input parameter. prompt Input parameter. Return value.

#### `CascadeModelTier resolveTier(const std::string &round_role) const noexcept`
- Source: `include/ethics_ai/llm_cascade_router.h`:154
- Brief: Resolve Tier.
- Parameters:
  - `round_role` (const std::string &): Input parameter.
- Return: Return value.
- Details: round_role Input parameter. Return value. Exception safety: noexcept.

#### `CascadeRoutingDecision routeForRound(const std::string &round_role, size_t estimated_prompt_tokens) const override`
- Source: `include/ethics_ai/llm_cascade_router.h`:113
- Brief: Route For Round.
- Parameters:
  - `round_role` (const std::string &): Input parameter.
  - `estimated_prompt_tokens` (size_t): Input parameter.
- Return: Return value.
- Details: round_role Input parameter. estimated_prompt_tokens Input parameter. Return value.

#### `void setLlmInvokeFn(LlmInvokeFn fn)`
- Source: `include/ethics_ai/llm_cascade_router.h`:134
- Brief: Set Llm Invoke Fn.
- Parameters:
  - `fn` (LlmInvokeFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: std::move().

#### `CascadeModelTier tierForRound(const std::string &round_role) const noexcept override`
- Source: `include/ethics_ai/llm_cascade_router.h`:120
- Brief: Tier For Round.
- Parameters:
  - `round_role` (const std::string &): Input parameter.
- Return: Return value.
- Details: round_role Input parameter. Return value. Exception safety: noexcept.

### themis::plugins::ethics::MetaVerdictBuilder

#### `MetaVerdictBuilder()=default`
- Source: `include/ethics_ai/meta_verdict_builder.h`:40
- Brief: n/a
- Parameters: none

#### `MetaVerdictBuilder(MetaVerdictBuilder &&) noexcept=default`
- Source: `include/ethics_ai/meta_verdict_builder.h`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetaVerdictBuilder &&): n/a

#### `MetaVerdictBuilder(const MetaVerdictBuilder &)=delete`
- Source: `include/ethics_ai/meta_verdict_builder.h`:44
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MetaVerdictBuilder &): n/a

#### `MetaVerdict buildMetaVerdict(const std::vector< DiscourseRoundOutput > &ebene1_results, const std::vector< ClusterPosition > &cluster_positions, const LegalGrounding &legal_grounding, DiscourseMode mode, const std::vector< DiscourseRoundOutput > &mirror_dissent) const`
- Source: `include/ethics_ai/meta_verdict_builder.h`:56
- Brief: n/a
- Parameters:
  - `ebene1_results` (const std::vector< DiscourseRoundOutput > &): n/a
  - `cluster_positions` (const std::vector< ClusterPosition > &): n/a
  - `legal_grounding` (const LegalGrounding &): n/a
  - `mode` (DiscourseMode): n/a
  - `mirror_dissent` (const std::vector< DiscourseRoundOutput > &): n/a

#### `std::string culturalRegion(const std::string &school_id) noexcept`
- Source: `include/ethics_ai/meta_verdict_builder.h`:66
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a

#### `MetaVerdictBuilder & operator=(MetaVerdictBuilder &&) noexcept=default`
- Source: `include/ethics_ai/meta_verdict_builder.h`:47
- Brief: n/a
- Parameters:
  - `<unnamed>` (MetaVerdictBuilder &&): n/a

#### `MetaVerdictBuilder & operator=(const MetaVerdictBuilder &)=delete`
- Source: `include/ethics_ai/meta_verdict_builder.h`:45
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MetaVerdictBuilder &): n/a

#### `void setLegalGrounding(LegalGrounding grounding) noexcept`
- Source: `include/ethics_ai/meta_verdict_builder.h`:54
- Brief: Set Legal Grounding.
- Parameters:
  - `grounding` (LegalGrounding): Input parameter.
- Details: grounding Input parameter. Exception safety: noexcept.

#### `~MetaVerdictBuilder()=default`
- Source: `include/ethics_ai/meta_verdict_builder.h`:41
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::MirrorSchoolHandler

#### `MirrorSchoolHandler()=default`
- Source: `include/ethics_ai/mirror_school_handler.h`:50
- Brief: n/a
- Parameters: none

#### `MirrorSchoolHandler(MirrorSchoolHandler &&) noexcept=default`
- Source: `include/ethics_ai/mirror_school_handler.h`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (MirrorSchoolHandler &&): n/a

#### `MirrorSchoolHandler(const MirrorSchoolHandler &)=delete`
- Source: `include/ethics_ai/mirror_school_handler.h`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MirrorSchoolHandler &): n/a

#### `MirrorSchoolHandler & operator=(MirrorSchoolHandler &&) noexcept=default`
- Source: `include/ethics_ai/mirror_school_handler.h`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (MirrorSchoolHandler &&): n/a

#### `MirrorSchoolHandler & operator=(const MirrorSchoolHandler &)=delete`
- Source: `include/ethics_ai/mirror_school_handler.h`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (const MirrorSchoolHandler &): n/a

#### `std::vector< DiscourseRoundOutput > runMirror(const std::vector< std::string > &mirror_school_ids, const std::string &dilemma_text, const std::string &domain)`
- Source: `include/ethics_ai/mirror_school_handler.h`:72
- Brief: Run Mirror.
- Parameters:
  - `mirror_school_ids` (const std::vector< std::string > &): Input parameter.
  - `dilemma_text` (const std::string &): Input parameter.
  - `domain` (const std::string &): Input parameter.
- Return: Return value.
- Details: mirror_school_ids Input parameter. dilemma_text Input parameter. domain Input parameter. Return value.

#### `void setLLMInferenceFn(LLMInferenceFn fn)`
- Source: `include/ethics_ai/mirror_school_handler.h`:63
- Brief: Set LLMInference Fn.
- Parameters:
  - `fn` (LLMInferenceFn): Input parameter.
- Details: fn Input parameter.

#### `void setSchoolTimeoutMs(int timeout_ms) noexcept`
- Source: `include/ethics_ai/mirror_school_handler.h`:70
- Brief: Set School Timeout Ms.
- Parameters:
  - `timeout_ms` (int): Input parameter.
- Details: timeout_ms Input parameter. Exception safety: noexcept.

#### `~MirrorSchoolHandler()=default`
- Source: `include/ethics_ai/mirror_school_handler.h`:51
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::MirrorSchoolPolicy

#### `bool isActiveFor(const std::string &domain) const noexcept`
- Source: `include/ethics_ai/ethics_ai_types.h`:294
- Brief: n/a
- Parameters:
  - `domain` (const std::string &): n/a

### themis::plugins::ethics::PhilosophyLoader

#### `PhilosophyLoader()=default`
- Source: `src/ethics_ai/philosophy_loader.h`:27
- Brief: n/a
- Parameters: none

#### `void addProfile(const PhilosophyProfile &profile)`
- Source: `src/ethics_ai/philosophy_loader.h`:66
- Brief: Add Profile.
- Parameters:
  - `profile` (const PhilosophyProfile &): Input parameter.
- Details: profile Input parameter. profile Input parameter. Calls: lock().

#### `void clear()`
- Source: `src/ethics_ai/philosophy_loader.h`:57
- Brief: Clear.
- Parameters: none
- Details: Calls: lock().

#### `size_t count() const`
- Source: `src/ethics_ai/philosophy_loader.h`:68
- Brief: n/a
- Parameters: none

#### `std::map< std::string, PhilosophyProfile > getAllProfiles() const`
- Source: `src/ethics_ai/philosophy_loader.h`:70
- Brief: n/a
- Parameters: none

#### `std::variant< PhilosophyProfile, Status > getProfile(const std::string &school_id) const`
- Source: `src/ethics_ai/philosophy_loader.h`:39
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a

#### `std::vector< std::string > getSchoolIds() const`
- Source: `src/ethics_ai/philosophy_loader.h`:52
- Brief: Get School Ids.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool hasProfile(const std::string &school_id) const`
- Source: `src/ethics_ai/philosophy_loader.h`:46
- Brief: Has Profile.
- Parameters:
  - `school_id` (const std::string &): Identifier of the school.
- Return: True when the operation succeeds.
- Details: school_id Identifier of the school. True when the operation succeeds.

#### `std::variant< size_t, Status > loadFromDirectory(const std::string &directory)`
- Source: `src/ethics_ai/philosophy_loader.h`:30
- Brief: n/a
- Parameters:
  - `directory` (const std::string &): n/a

#### `Status loadFromFile(const std::string &filepath)`
- Source: `src/ethics_ai/philosophy_loader.h`:37
- Brief: Load From File.
- Parameters:
  - `filepath` (const std::string &): Input parameter.
- Return: Return value.
- Details: filepath Input parameter. Return value. filepath Input parameter. Return value. Calls: YAML::LoadFile(), empty(), fs::path(), stem(), string(), IsScalar(), IsMap(), str().

#### `Status parseYAML(const std::string &content, PhilosophyProfile &profile)`
- Source: `src/ethics_ai/philosophy_loader.h`:82
- Brief: Helper to parse YAML content.
- Parameters:
  - `content` (const std::string &): Input parameter.
  - `profile` (PhilosophyProfile &): Input/output parameter.
- Return: Return value.
- Details: content Input parameter. profile Input/output parameter. Return value.

#### `std::variant< size_t, Status > reloadProfiles(const std::string &directory)`
- Source: `src/ethics_ai/philosophy_loader.h`:60
- Brief: n/a
- Parameters:
  - `directory` (const std::string &): n/a

#### `~PhilosophyLoader()=default`
- Source: `src/ethics_ai/philosophy_loader.h`:28
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::PositionAbstractSchemaError

#### `PositionAbstractSchemaError(const std::string &school, int round, const std::string &reason)`
- Source: `include/ethics_ai/position_abstract_validator.h`:33
- Brief: Position Abstract Schema Error.
- Parameters:
  - `school` (const std::string &): Input parameter.
  - `round` (int): Input parameter.
  - `reason` (const std::string &): Input parameter.
- Return: Return value.
- Details: school Input parameter. round Input parameter. reason Input parameter. Return value.

### themis::plugins::ethics::PositionAbstractValidator

#### `PositionAbstractValidator(PositionAbstractConfig config=PositionAbstractConfig{})`
- Source: `include/ethics_ai/position_abstract_validator.h`:53
- Brief: n/a
- Parameters:
  - `config` (PositionAbstractConfig): n/a

#### `bool autoRepair(DiscourseRoundOutput &output) const`
- Source: `include/ethics_ai/position_abstract_validator.h`:69
- Brief: Auto Repair.
- Parameters:
  - `output` (DiscourseRoundOutput &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: output Input/output parameter. True when the operation succeeds.

#### `std::string buildDefaultAbstract(const DiscourseRoundOutput &output)`
- Source: `include/ethics_ai/position_abstract_validator.h`:107
- Brief: Build Default Abstract.
- Parameters:
  - `output` (const DiscourseRoundOutput &): Input parameter.
- Return: Return value.
- Details: output Input parameter. Return value. output Input parameter. Return value. Calls: size(), str(), empty().

#### `std::string buildSchemaInstruction() const`
- Source: `include/ethics_ai/position_abstract_validator.h`:75
- Brief: Build Schema Instruction.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `const PositionAbstractConfig & config() const noexcept`
- Source: `include/ethics_ai/position_abstract_validator.h`:77
- Brief: n/a
- Parameters: none

#### `int countTokens(const std::string &text) noexcept`
- Source: `include/ethics_ai/position_abstract_validator.h`:95
- Brief: Count Tokens.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Exception safety: noexcept.

#### `std::string extractVerdictFromContent(const std::string &content)`
- Source: `include/ethics_ai/position_abstract_validator.h`:101
- Brief: Extract Verdict From Content.
- Parameters:
  - `content` (const std::string &): Input parameter.
- Return: Return value.
- Details: content Input parameter. Return value. content Input parameter. Return value. Calls: std::transform(), begin(), end(), std::toupper(), find(), std::string().

#### `bool isValidVerdict(const std::string &v) noexcept`
- Source: `include/ethics_ai/position_abstract_validator.h`:88
- Brief: Is Valid Verdict.
- Parameters:
  - `v` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: v Input parameter. True when the operation succeeds. Exception safety: noexcept.

#### `bool validate(DiscourseRoundOutput &output, bool strict=true) const`
- Source: `include/ethics_ai/position_abstract_validator.h`:56
- Brief: n/a
- Parameters:
  - `output` (DiscourseRoundOutput &): n/a
  - `strict` (bool): n/a

#### `void validateBatch(std::vector< DiscourseRoundOutput > &outputs) const`
- Source: `include/ethics_ai/position_abstract_validator.h`:62
- Brief: Validate Batch.
- Parameters:
  - `outputs` (std::vector< DiscourseRoundOutput > &): Input/output parameter.
- Details: outputs Input/output parameter.

### themis::plugins::ethics::PriorRoundCompressor

#### `PriorRoundCompressor()=default`
- Source: `include/ethics_ai/prior_round_compressor.h`:50
- Brief: n/a
- Parameters: none

#### `std::string buildPriorContext(const std::vector< std::vector< EthicalArgument > > &all_rounds, const CompressionConfig &config, int current_round, int max_total_tokens) const`
- Source: `include/ethics_ai/prior_round_compressor.h`:72
- Brief: Build Prior Context.
- Parameters:
  - `all_rounds` (const std::vector< std::vector< EthicalArgument > > &): Input parameter.
  - `config` (const CompressionConfig &): Input parameter.
  - `current_round` (int): Input parameter.
  - `max_total_tokens` (int): Input parameter.
- Return: Return value.
- Details: all_rounds Input parameter. config Input parameter. current_round Input parameter. max_total_tokens Input parameter. Return value.

#### `CompressionResult compressHeadline(const EthicalArgument &arg, const CompressionConfig &config) const`
- Source: `include/ethics_ai/prior_round_compressor.h`:138
- Brief: Compress Headline.
- Parameters:
  - `arg` (const EthicalArgument &): Input parameter.
  - `config` (const CompressionConfig &): Input parameter.
- Return: Return value.
- Details: arg Input parameter. config Input parameter. Return value.

#### `CompressionResult compressPrincipleCitationsOnly(const EthicalArgument &arg, const CompressionConfig &config) const`
- Source: `include/ethics_ai/prior_round_compressor.h`:128
- Brief: Compress Principle Citations Only.
- Parameters:
  - `arg` (const EthicalArgument &): Input parameter.
  - `config` (const CompressionConfig &): Input parameter.
- Return: Return value.
- Details: arg Input parameter. config Input parameter. Return value.

#### `CompressionResult compressPriorRound(const std::vector< EthicalArgument > &round_arguments, const CompressionConfig &config, int current_round) const`
- Source: `include/ethics_ai/prior_round_compressor.h`:59
- Brief: Compress Prior Round.
- Parameters:
  - `round_arguments` (const std::vector< EthicalArgument > &): Input parameter.
  - `config` (const CompressionConfig &): Input parameter.
  - `current_round` (int): Input parameter.
- Return: Return value.
- Details: round_arguments Input parameter. config Input parameter. current_round Input parameter. Return value.

#### `CompressionResult compressStructuredSummary(const EthicalArgument &arg, const CompressionConfig &config) const`
- Source: `include/ethics_ai/prior_round_compressor.h`:157
- Brief: Compress Structured Summary.
- Parameters:
  - `arg` (const EthicalArgument &): Input parameter.
  - `config` (const CompressionConfig &): Input parameter.
- Return: Return value.
- Details: arg Input parameter. config Input parameter. Return value.

#### `int countTokens(const std::string &text) noexcept`
- Source: `include/ethics_ai/prior_round_compressor.h`:103
- Brief: Count Tokens.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Exception safety: noexcept.

#### `std::vector< std::string > extractPrincipleCitations(const std::string &content)`
- Source: `include/ethics_ai/prior_round_compressor.h`:112
- Brief: Extract Principle Citations.
- Parameters:
  - `content` (const std::string &): Input parameter.
- Return: Return value.
- Details: content Input parameter. Return value. content Input parameter. Return value. Calls: re(), std::sregex_iterator(), begin(), end(), str(), insert(), push_back().

#### `std::string extractVerdict(const std::string &content)`
- Source: `include/ethics_ai/prior_round_compressor.h`:120
- Brief: Extract Verdict.
- Parameters:
  - `content` (const std::string &): Input parameter.
- Return: Return value.
- Details: content Input parameter. Return value. content Input parameter. Return value. Calls: std::transform(), begin(), end(), find().

#### `float measureDcLoss(const std::string &original_arg, const std::string &compressed_arg) const`
- Source: `include/ethics_ai/prior_round_compressor.h`:84
- Brief: Measure Dc Loss.
- Parameters:
  - `original_arg` (const std::string &): Input parameter.
  - `compressed_arg` (const std::string &): Input parameter.
- Return: Return value.
- Details: original_arg Input parameter. compressed_arg Input parameter. Return value.

#### `void setLlmSummaryFn(LlmSummaryFn fn)`
- Source: `include/ethics_ai/prior_round_compressor.h`:95
- Brief: Set Llm Summary Fn.
- Parameters:
  - `fn` (LlmSummaryFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: lock(), std::move().

### themis::plugins::ethics::RAGContextEngine

#### `RAGContextEngine(std::shared_ptr< ArgumentStore > store)`
- Source: `src/ethics_ai/rag_context_engine.h`:30
- Brief: RAGContext Engine.
- Parameters:
  - `store` (std::shared_ptr< ArgumentStore >): Input parameter.
- Return: Return value.
- Details: store Input parameter. Return value.

#### `std::variant< RAGContext, Status > buildContext(const std::string &dilemma_description, const std::vector< std::string > &philosophy_schools, const std::string &category)`
- Source: `src/ethics_ai/rag_context_engine.h`:33
- Brief: n/a
- Parameters:
  - `dilemma_description` (const std::string &): n/a
  - `philosophy_schools` (const std::vector< std::string > &): n/a
  - `category` (const std::string &): n/a

#### `double calculateTextSimilarity(const std::string &text1, const std::string &text2)`
- Source: `src/ethics_ai/rag_context_engine.h`:87
- Brief: Calculate Text Similarity.
- Parameters:
  - `text1` (const std::string &): Input parameter.
  - `text2` (const std::string &): Input parameter.
- Return: Return value.
- Details: text1 Input parameter. text2 Input parameter. Return value. text1 Input parameter. text2 Input parameter. Return value. Calls: empty(), iss(), std::transform(), begin(), end(), std::isalpha(), back(), pop_back().

#### `std::variant< std::vector< std::string >, Status > findSimilarDilemmas(const std::string &query_text, double threshold, size_t limit)`
- Source: `src/ethics_ai/rag_context_engine.h`:39
- Brief: n/a
- Parameters:
  - `query_text` (const std::string &): n/a
  - `threshold` (double): n/a
  - `limit` (size_t): n/a

#### `std::vector< float > generateEmbedding(const std::string &text)`
- Source: `src/ethics_ai/rag_context_engine.h`:93
- Brief: Generate Embedding.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. text Input parameter. Return value. Calls: emb(), empty(), std::sqrt().

#### `std::variant< std::vector< std::string >, Status > getBestPractices(const std::string &category, double min_satisfaction, size_t limit)`
- Source: `src/ethics_ai/rag_context_engine.h`:45
- Brief: n/a
- Parameters:
  - `category` (const std::string &): n/a
  - `min_satisfaction` (double): n/a
  - `limit` (size_t): n/a

#### `LegalGrounding retrieveLegalGrounding(const std::string &dilemma_description) const`
- Source: `src/ethics_ai/rag_context_engine.h`:64
- Brief: n/a
- Parameters:
  - `dilemma_description` (const std::string &): n/a

#### `void setLegalDbAvailable(bool available) noexcept`
- Source: `src/ethics_ai/rag_context_engine.h`:72
- Brief: Set Legal Db Available.
- Parameters:
  - `available` (bool): Input parameter.
- Details: available Input parameter. Exception safety: noexcept.

#### `std::variant< std::vector< std::string >, Status > traverseArgumentChain(const std::string &start_argument_id, size_t max_depth, const std::string &direction)`
- Source: `src/ethics_ai/rag_context_engine.h`:58
- Brief: n/a
- Parameters:
  - `start_argument_id` (const std::string &): n/a
  - `max_depth` (size_t): n/a
  - `direction` (const std::string &): n/a

#### `std::variant< std::vector< std::pair< std::string, double > >, Status > vectorSemanticSearch(const std::vector< float > &query_embedding, const std::string &philosophy_school, size_t limit)`
- Source: `src/ethics_ai/rag_context_engine.h`:52
- Brief: n/a
- Parameters:
  - `query_embedding` (const std::vector< float > &): n/a
  - `philosophy_school` (const std::string &): n/a
  - `limit` (size_t): n/a

#### `~RAGContextEngine()=default`
- Source: `src/ethics_ai/rag_context_engine.h`:31
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::ScalarAdaLoRABiasCorrector

#### `double applyBiasCorrection(const std::string &school_id, double raw_score) const noexcept override`
- Source: `include/ethics_ai/ethics_ai_types.h`:624
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a
  - `raw_score` (double): n/a

#### `bool hasAdapter(const std::string &school_id) const noexcept override`
- Source: `include/ethics_ai/ethics_ai_types.h`:647
- Brief: n/a
- Parameters:
  - `school_id` (const std::string &): n/a

#### `void registerAdapter(const std::string &school_id, double factor)`
- Source: `include/ethics_ai/ethics_ai_types.h`:619
- Brief: Register Adapter.
- Parameters:
  - `school_id` (const std::string &): Identifier of the school.
  - `factor` (double): Input parameter.
- Details: school_id Identifier of the school. factor Input parameter. Calls: lock().

### themis::plugins::ethics::SchemaValidationError

#### `SchemaValidationError(const std::string &msg)`
- Source: `include/ethics_ai/synthesis_matrix_builder.h`:37
- Brief: Schema Validation Error.
- Parameters:
  - `msg` (const std::string &): Input parameter.
- Return: Return value.
- Details: msg Input parameter. Return value.

### themis::plugins::ethics::Status

#### `Status Error(const std::string &msg, int code=-1)`
- Source: `include/ethics_ai/ethics_ai_types.h`:187
- Brief: n/a
- Parameters:
  - `msg` (const std::string &): n/a
  - `code` (int): n/a

#### `Status OK()`
- Source: `include/ethics_ai/ethics_ai_types.h`:186
- Brief: OK.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: Status().

#### `Status()`
- Source: `include/ethics_ai/ethics_ai_types.h`:177
- Brief: n/a
- Parameters: none

#### `Status(bool ok_, const std::string &msg="", int code_=0)`
- Source: `include/ethics_ai/ethics_ai_types.h`:178
- Brief: n/a
- Parameters:
  - `ok_` (bool): n/a
  - `msg` (const std::string &): n/a
  - `code_` (int): n/a

#### `bool isOK() const`
- Source: `include/ethics_ai/ethics_ai_types.h`:191
- Brief: n/a
- Parameters: none

#### `operator bool() const`
- Source: `include/ethics_ai/ethics_ai_types.h`:192
- Brief: n/a
- Parameters: none

### themis::plugins::ethics::SynthesisMatrixBuilder

#### `SynthesisMatrixBuilder()=default`
- Source: `include/ethics_ai/synthesis_matrix_builder.h`:43
- Brief: n/a
- Parameters: none

#### `std::string buildMatrix(const std::vector< SchoolPositionSummary > &positions, const std::vector< ConvergenceMarker > &convergences={}, int max_tokens=300) const`
- Source: `include/ethics_ai/synthesis_matrix_builder.h`:45
- Brief: n/a
- Parameters:
  - `positions` (const std::vector< SchoolPositionSummary > &): n/a
  - `convergences` (const std::vector< ConvergenceMarker > &): n/a
  - `max_tokens` (int): n/a

#### `int countTokens(const std::string &text) noexcept`
- Source: `include/ethics_ai/synthesis_matrix_builder.h`:71
- Brief: Count Tokens.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Exception safety: noexcept.

#### `SchoolPositionSummary extractSummary(const DiscourseRoundOutput &round_output) const`
- Source: `include/ethics_ai/synthesis_matrix_builder.h`:55
- Brief: Extract Summary.
- Parameters:
  - `round_output` (const DiscourseRoundOutput &): Input parameter.
- Return: Return value.
- Details: round_output Input parameter. Return value.

#### `bool isValidVerdict(const std::string &verdict) noexcept`
- Source: `include/ethics_ai/synthesis_matrix_builder.h`:78
- Brief: Is Valid Verdict.
- Parameters:
  - `verdict` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: verdict Input parameter. True when the operation succeeds. Exception safety: noexcept.

#### `void validateSummary(const SchoolPositionSummary &summary) const`
- Source: `include/ethics_ai/synthesis_matrix_builder.h`:62
- Brief: Validate Summary.
- Parameters:
  - `summary` (const SchoolPositionSummary &): Input parameter.
- Details: summary Input parameter.

### themis::plugins::ethics::TournamentModeSelector

#### `TournamentModeSelector()=default`
- Source: `include/ethics_ai/tournament_mode_selector.h`:47
- Brief: n/a
- Parameters: none

#### `std::string buildHeadline(const EthicalArgument &arg)`
- Source: `include/ethics_ai/tournament_mode_selector.h`:73
- Brief: Build Headline.
- Parameters:
  - `arg` (const EthicalArgument &): Input parameter.
- Return: Return value.
- Details: arg Input parameter. Return value. arg Input parameter. Return value. Calls: size(), substr(), str().

#### `std::map< std::string, TournamentSelectionResult > buildTournamentContext(const std::vector< EthicalArgument > &all_round_arguments, const std::map< std::string, std::vector< SchoolTension > > &tensions_per_school, const TournamentConfig &config=TournamentConfig{}) const`
- Source: `include/ethics_ai/tournament_mode_selector.h`:55
- Brief: n/a
- Parameters:
  - `all_round_arguments` (const std::vector< EthicalArgument > &): n/a
  - `tensions_per_school` (const std::map< std::string, std::vector< SchoolTension > > &): n/a
  - `config` (const TournamentConfig &): n/a

#### `int countTokens(const std::string &text) noexcept`
- Source: `include/ethics_ai/tournament_mode_selector.h`:67
- Brief: Count Tokens.
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Exception safety: noexcept.

#### `TournamentSelectionResult selectOpponents(const std::string &own_school_id, const std::vector< EthicalArgument > &opponent_arguments, const std::vector< SchoolTension > &tensions, const TournamentConfig &config=TournamentConfig{}) const`
- Source: `include/ethics_ai/tournament_mode_selector.h`:49
- Brief: n/a
- Parameters:
  - `own_school_id` (const std::string &): n/a
  - `opponent_arguments` (const std::vector< EthicalArgument > &): n/a
  - `tensions` (const std::vector< SchoolTension > &): n/a
  - `config` (const TournamentConfig &): n/a

