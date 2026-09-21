# LLAMA_CPP DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\llama_cpp\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\llama_cpp\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 12
- Compounds: 37
- Classes/Structs: 8
- Namespaces: 8
- File Compounds: 12

## Namespaces
- benchmark
- themis
- themis::llamacpp
- themis::llamacpp::@127237156063337153307251304233107314375357123362
- themis::llamacpp::@165270353314065163016077076374251231372042352256
- themis::llm
- themis::test
- themis::test::wave_d

## Types
### Classes
- LlamaCppBenchFixture
- themis::llamacpp::LlamaCppPlugin
- themis::llamacpp::LlamaCppPluginRegistrar
- themis::llamacpp::ModelLoadingGuard

### Structs
- themis::llamacpp::LlamaCppPlugin::LoRAEntry
- themis::test::wave_d::StubLlamaAccelGuard
- themis::test::wave_d::StubLlamaRegistry
- themis::test::wave_d::StubTokenStore

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 215

### LlamaCppBenchFixture

#### `void SetUp(const benchmark::State &) override`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:291
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `void TearDown(const benchmark::State &) override`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (const benchmark::State &): n/a

#### `bool ensureReady(benchmark::State &state)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:337
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

### bench_llama_cpp_inference.cpp

#### `Arg(1) -> Arg(4) ->Arg(8) ->Arg(16) ->Arg(32) ->Unit(benchmark::kMillisecond)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:462
- Brief: n/a
- Parameters:
  - `<unnamed>` (1): n/a

#### `BENCHMARK(BM_Generate_PromptSize) -> Arg(16) ->Arg(64) ->Arg(256) ->Arg(512)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:436
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_Generate_PromptSize): n/a

#### `BENCHMARK(BM_LlamaCpp_RealModel_GPUEvidence) -> Unit(benchmark::kMillisecond)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:666
- Brief: n/a
- Parameters:
  - `<unnamed>` (BM_LlamaCpp_RealModel_GPUEvidence): n/a

#### `BENCHMARK_F(LlamaCppBenchFixture, Embed_Latency)(benchmark`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:467
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppBenchFixture): n/a
  - `<unnamed>` (Embed_Latency): n/a

#### `BENCHMARK_F(LlamaCppBenchFixture, GenerateBatch_Throughput)(benchmark`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:440
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppBenchFixture): n/a
  - `<unnamed>` (GenerateBatch_Throughput): n/a

#### `BENCHMARK_F(LlamaCppBenchFixture, GenerateStream_Overhead)(benchmark`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:485
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppBenchFixture): n/a
  - `<unnamed>` (GenerateStream_Overhead): n/a

#### `BENCHMARK_F(LlamaCppBenchFixture, Generate_SingleRequest)(benchmark`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:363
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppBenchFixture): n/a
  - `<unnamed>` (Generate_SingleRequest): n/a

#### `BENCHMARK_F(LlamaCppBenchFixture, GetCapabilities)(benchmark`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:590
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppBenchFixture): n/a
  - `<unnamed>` (GetCapabilities): n/a

#### `BENCHMARK_F(LlamaCppBenchFixture, StatsQuery)(benchmark`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:574
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppBenchFixture): n/a
  - `<unnamed>` (StatsQuery): n/a

#### `void BM_ConcurrentInference(benchmark::State &state)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:506
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_Generate_PromptSize(benchmark::State &state)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:382
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LlamaCpp_RealModel_GPUEvidence(benchmark::State &state)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:603
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void LLCPG1_TTFT_Stub_Baseline(benchmark::State &state)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:675
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void LLCPG2_BatchEmbedding_Stub(benchmark::State &state)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:698
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void LLCPG3_LoRALoad_P99(benchmark::State &state)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:726
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void LLCPG4_RegressionBaseline(benchmark::State &state)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:751
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `Unit(benchmark::kMicrosecond) -> UseRealTime() ->Iterations(100)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:743
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

#### `Unit(benchmark::kMillisecond) -> UseRealTime() ->Iterations(20)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:692
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMillisecond): n/a

#### `std::optional< size_t > extractTokenCount(const themis::llm::GGUFMetadata &metadata)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:138
- Brief: n/a
- Parameters:
  - `metadata` (const themis::llm::GGUFMetadata &): n/a

#### `bool fileExists(const std::string &path)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:89
- Brief: n/a
- Parameters:
  - `path` (const std::string &): n/a

#### `const themis::llm::TensorMetadata * findTensorByName(const themis::llm::GGUFMetadata &metadata, std::string_view tensorName)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:156
- Brief: n/a
- Parameters:
  - `metadata` (const themis::llm::GGUFMetadata &): n/a
  - `tensorName` (std::string_view): n/a

#### `std::string getEnvOrEmpty(const char *key)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:46
- Brief: n/a
- Parameters:
  - `key` (const char *): n/a

#### `bool isGemmaArtifact(const std::string &modelPath, const themis::llm::GGUFMetadata &metadata)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:104
- Brief: n/a
- Parameters:
  - `modelPath` (const std::string &): n/a
  - `metadata` (const themis::llm::GGUFMetadata &): n/a

#### `InferenceRequest makeRequest(const std::string &prompt, int max_tokens=64, float temperature=0.0f)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:275
- Brief: Build an InferenceRequest with a given prompt and max_tokens budget.
- Parameters:
  - `prompt` (const std::string &): n/a
  - `max_tokens` (int): n/a
  - `temperature` (float): n/a

#### `std::optional< size_t > parseArrayLength(const std::string &encodedValue)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:110
- Brief: n/a
- Parameters:
  - `encodedValue` (const std::string &): n/a

#### `std::string resolveCompileTimeModelPath()`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:51
- Brief: n/a
- Parameters: none

#### `int resolveGpuLayers()`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:77
- Brief: n/a
- Parameters: none

#### `std::string resolveModelPath()`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:69
- Brief: n/a
- Parameters: none

#### `std::optional< std::string > runRealModelPreflight(const std::string &modelPath)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:166
- Brief: n/a
- Parameters:
  - `modelPath` (const std::string &): n/a

#### `void setLlamaGpuEvidenceCounters(benchmark::State &state, const std::string &modelPath, int requestedGpuLayers, bool warmupSucceeded, const nlohmann::json &memoryStats)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:216
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a
  - `modelPath` (const std::string &): n/a
  - `requestedGpuLayers` (int): n/a
  - `warmupSucceeded` (bool): n/a
  - `memoryStats` (const nlohmann::json &): n/a

#### `std::string toLowerCopy(std::string value)`
- Source: `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`:97
- Brief: n/a
- Parameters:
  - `value` (std::string): n/a

### llama_cpp_plugin.h

#### `THEMIS_LLM_PLUGIN()`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:254
- Brief: n/a
- Parameters: none

### llama_cpp_plugin_validation_gates.cpp

#### `const void * GetLlamaCppPluginValidator()`
- Source: `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`:405
- Brief: Get Llama Cpp Plugin Validator.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result. Calls: themis::llamacpp::getLlamaPluginValidator().

#### `int LlamaCppPluginValidationEnabled()`
- Source: `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`:415
- Brief: Llama Cpp Plugin Validation Enabled.
- Parameters: none
- Return: Return value.
- Details: Return value. Implements LlamaCppPluginValidationEnabled without additional internal calls.

### test_llama_cpp_inference_contract_focused.cpp

#### `TEST(LlamaCppInferenceContractFocusedTests, IC10_JsonSchema_Accepted)`
- Source: `src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppInferenceContractFocusedTests): n/a
  - `<unnamed>` (IC10_JsonSchema_Accepted): n/a

#### `TEST(LlamaCppInferenceContractFocusedTests, IC1_GenerateUnloaded_ReturnsError)`
- Source: `src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppInferenceContractFocusedTests): n/a
  - `<unnamed>` (IC1_GenerateUnloaded_ReturnsError): n/a

#### `TEST(LlamaCppInferenceContractFocusedTests, IC2_GenerateLoaded_ReturnsSuccess)`
- Source: `src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`:53
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppInferenceContractFocusedTests): n/a
  - `<unnamed>` (IC2_GenerateLoaded_ReturnsSuccess): n/a

#### `TEST(LlamaCppInferenceContractFocusedTests, IC3_GenerateSuccess_TextNotEmpty)`
- Source: `src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`:64
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppInferenceContractFocusedTests): n/a
  - `<unnamed>` (IC3_GenerateSuccess_TextNotEmpty): n/a

#### `TEST(LlamaCppInferenceContractFocusedTests, IC4_GenerateSuccess_ErrorMessageEmpty)`
- Source: `src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppInferenceContractFocusedTests): n/a
  - `<unnamed>` (IC4_GenerateSuccess_ErrorMessageEmpty): n/a

#### `TEST(LlamaCppInferenceContractFocusedTests, IC5_SystemPrompt_Accepted)`
- Source: `src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`:90
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppInferenceContractFocusedTests): n/a
  - `<unnamed>` (IC5_SystemPrompt_Accepted): n/a

#### `TEST(LlamaCppInferenceContractFocusedTests, IC6_LoRAAdapterHint_Accepted)`
- Source: `src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppInferenceContractFocusedTests): n/a
  - `<unnamed>` (IC6_LoRAAdapterHint_Accepted): n/a

#### `TEST(LlamaCppInferenceContractFocusedTests, IC7_StreamingCallback_IsInvoked)`
- Source: `src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`:112
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppInferenceContractFocusedTests): n/a
  - `<unnamed>` (IC7_StreamingCallback_IsInvoked): n/a

#### `TEST(LlamaCppInferenceContractFocusedTests, IC8_GenerateRAG_ReturnsResponse)`
- Source: `src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`:133
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppInferenceContractFocusedTests): n/a
  - `<unnamed>` (IC8_GenerateRAG_ReturnsResponse): n/a

#### `TEST(LlamaCppInferenceContractFocusedTests, IC9_GenerateRAG_EmptyContext_NocrASH)`
- Source: `src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`:151
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppInferenceContractFocusedTests): n/a
  - `<unnamed>` (IC9_GenerateRAG_EmptyContext_NocrASH): n/a

#### `std::unique_ptr< LlamaCppPlugin > make_loaded_plugin()`
- Source: `src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`:33
- Brief: ── helper ────────────────────────────────────────────────────────────────────
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: loadModel().

### test_llama_cpp_plugin.cpp

#### `TEST(LlamaCppPluginFocusedTests, A1_LoadModelReturnsTrue)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:28
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (A1_LoadModelReturnsTrue): n/a

#### `TEST(LlamaCppPluginFocusedTests, A2_DoubleLoadIsSafe)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (A2_DoubleLoadIsSafe): n/a

#### `TEST(LlamaCppPluginFocusedTests, A3_UnloadModel)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (A3_UnloadModel): n/a

#### `TEST(LlamaCppPluginFocusedTests, B1_GetModelInfoNulloptWhenNotLoaded)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (B1_GetModelInfoNulloptWhenNotLoaded): n/a

#### `TEST(LlamaCppPluginFocusedTests, B2_GetModelInfoPresentAfterLoad)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:55
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (B2_GetModelInfoPresentAfterLoad): n/a

#### `TEST(LlamaCppPluginFocusedTests, B3_GetModelInfoContainsModelId)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:61
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (B3_GetModelInfoContainsModelId): n/a

#### `TEST(LlamaCppPluginFocusedTests, C1_InitiallyNotLoaded)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:69
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (C1_InitiallyNotLoaded): n/a

#### `TEST(LlamaCppPluginFocusedTests, C2_LoadedAfterLoadModel)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (C2_LoadedAfterLoadModel): n/a

#### `TEST(LlamaCppPluginFocusedTests, C3_NotLoadedAfterUnload)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (C3_NotLoadedAfterUnload): n/a

#### `TEST(LlamaCppPluginFocusedTests, D1_GenerateUninitializedReturnsError)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (D1_GenerateUninitializedReturnsError): n/a

#### `TEST(LlamaCppPluginFocusedTests, D2_GenerateAfterLoadReturnsSuccess)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (D2_GenerateAfterLoadReturnsSuccess): n/a

#### `TEST(LlamaCppPluginFocusedTests, D3_GenerateTextNotEmpty)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:105
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (D3_GenerateTextNotEmpty): n/a

#### `TEST(LlamaCppPluginFocusedTests, E1_GenerateRAGReturnsSuccess)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (E1_GenerateRAGReturnsSuccess): n/a

#### `TEST(LlamaCppPluginFocusedTests, E2_GenerateRAGNoContextSameAsGenerate)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:126
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (E2_GenerateRAGNoContextSameAsGenerate): n/a

#### `TEST(LlamaCppPluginFocusedTests, E3_GenerateRAGUninitReturnsError)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (E3_GenerateRAGUninitReturnsError): n/a

#### `TEST(LlamaCppPluginFocusedTests, E4_GenerateRAGHonoursExplicitContextOverrideForMaxTokens)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (E4_GenerateRAGHonoursExplicitContextOverrideForMaxTokens): n/a

#### `TEST(LlamaCppPluginFocusedTests, F1_EmbedReturnsEmptyWhenNotLoaded)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (F1_EmbedReturnsEmptyWhenNotLoaded): n/a

#### `TEST(LlamaCppPluginFocusedTests, F2_EmbedReturnsVectorWhenLoaded)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (F2_EmbedReturnsVectorWhenLoaded): n/a

#### `TEST(LlamaCppPluginFocusedTests, F3_EmbedReturnsEmptyAfterUnload)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:191
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (F3_EmbedReturnsEmptyAfterUnload): n/a

#### `TEST(LlamaCppPluginFocusedTests, G1_LoadLoRAReturnsTrue)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:200
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (G1_LoadLoRAReturnsTrue): n/a

#### `TEST(LlamaCppPluginFocusedTests, G2_ListLoRAsAfterLoad)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:205
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (G2_ListLoRAsAfterLoad): n/a

#### `TEST(LlamaCppPluginFocusedTests, G3_UnloadLoRARemovesEntry)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:214
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (G3_UnloadLoRARemovesEntry): n/a

#### `TEST(LlamaCppPluginFocusedTests, H1_DuplicateLoRAIdReplaces)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (H1_DuplicateLoRAIdReplaces): n/a

#### `TEST(LlamaCppPluginFocusedTests, H2_UnloadNonexistentReturnsFalse)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:232
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (H2_UnloadNonexistentReturnsFalse): n/a

#### `TEST(LlamaCppPluginFocusedTests, H3_MultipleLoRAsListedCorrectly)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:237
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (H3_MultipleLoRAsListedCorrectly): n/a

#### `TEST(LlamaCppPluginFocusedTests, I1_SupportsLoRA)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:246
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (I1_SupportsLoRA): n/a

#### `TEST(LlamaCppPluginFocusedTests, I2_SupportsEmbeddings)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (I2_SupportsEmbeddings): n/a

#### `TEST(LlamaCppPluginFocusedTests, I3_PluginVersion)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:256
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (I3_PluginVersion): n/a

#### `TEST(LlamaCppPluginFocusedTests, J1_MemoryStatsContainsRequiredKeys)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:263
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (J1_MemoryStatsContainsRequiredKeys): n/a

#### `TEST(LlamaCppPluginFocusedTests, J2_PerformanceStatsContainsInferenceCount)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:271
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (J2_PerformanceStatsContainsInferenceCount): n/a

#### `TEST(LlamaCppPluginFocusedTests, J3_PluginNameInMemoryStats)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:280
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (J3_PluginNameInMemoryStats): n/a

#### `TEST(LlamaCppPluginFocusedTests, K1_StreamCallbackInvokedOnSuccess)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:287
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (K1_StreamCallbackInvokedOnSuccess): n/a

#### `TEST(LlamaCppPluginFocusedTests, K2_GenerateStreamResponseTextNotEmpty)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (K2_GenerateStreamResponseTextNotEmpty): n/a

#### `TEST(LlamaCppPluginFocusedTests, K3_GenerateStreamUninitReturnsError)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:308
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (K3_GenerateStreamUninitReturnsError): n/a

#### `TEST(LlamaCppPluginFocusedTests, K4_StreamCallbackExceptionSwallowed)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:319
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (K4_StreamCallbackExceptionSwallowed): n/a

#### `TEST(LlamaCppPluginFocusedTests, K5_StreamCallbackTokenMatchesGenerateText)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:332
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (K5_StreamCallbackTokenMatchesGenerateText): n/a

#### `TEST(LlamaCppPluginFocusedTests, L1_BatchEmptyInputReturnsEmpty)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (L1_BatchEmptyInputReturnsEmpty): n/a

#### `TEST(LlamaCppPluginFocusedTests, L2_BatchSingleRequestReturnsOneResponse)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:353
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (L2_BatchSingleRequestReturnsOneResponse): n/a

#### `TEST(LlamaCppPluginFocusedTests, L3_BatchMultipleRequestsPreservesOrder)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:363
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (L3_BatchMultipleRequestsPreservesOrder): n/a

#### `TEST(LlamaCppPluginFocusedTests, L4_BatchErrorWhenNotLoaded)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (L4_BatchErrorWhenNotLoaded): n/a

#### `TEST(LlamaCppPluginFocusedTests, L5_BatchIncreasesInferenceCount)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:385
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (L5_BatchIncreasesInferenceCount): n/a

#### `TEST(LlamaCppPluginFocusedTests, M1_CapabilitiesSupportsStreaming)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:397
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (M1_CapabilitiesSupportsStreaming): n/a

#### `TEST(LlamaCppPluginFocusedTests, M2_CapabilitiesSupportsBatching)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:402
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (M2_CapabilitiesSupportsBatching): n/a

#### `TEST(LlamaCppPluginFocusedTests, M3_CapabilitiesPluginVersion210)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:407
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (M3_CapabilitiesPluginVersion210): n/a

#### `TEST(LlamaCppPluginFocusedTests, M4_GetPluginVersionMethod)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:412
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (M4_GetPluginVersionMethod): n/a

#### `TEST(LlamaCppPluginFocusedTests, N1_RegistrarCreatePluginStubMode)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:419
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (N1_RegistrarCreatePluginStubMode): n/a

#### `TEST(LlamaCppPluginFocusedTests, N2_RegistrarCreatePluginWithEmptyPath)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:426
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (N2_RegistrarCreatePluginWithEmptyPath): n/a

#### `TEST(LlamaCppPluginFocusedTests, N3_RegistrarDefaultReloadCallbackStubMode)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:434
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (N3_RegistrarDefaultReloadCallbackStubMode): n/a

#### `TEST(LlamaCppPluginFocusedTests, N4_RegistrarDefaultReloadCallbackWithPath)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:441
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (N4_RegistrarDefaultReloadCallbackWithPath): n/a

#### `TEST(LlamaCppPluginFocusedTests, N5_RegistrarCreatePluginSupportsGenerate)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:450
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (N5_RegistrarCreatePluginSupportsGenerate): n/a

#### `TEST(LlamaCppPluginFocusedTests, N6_InferenceResponseEchoesTraceContext)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:459
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (N6_InferenceResponseEchoesTraceContext): n/a

#### `TEST(LlamaCppPluginFocusedTests, O1_ModelNotLoadedReturnsSuccessFalse)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:482
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (O1_ModelNotLoadedReturnsSuccessFalse): n/a

#### `TEST(LlamaCppPluginFocusedTests, O2_StubModeLoadedReturnsSuccessTrue)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:491
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (O2_StubModeLoadedReturnsSuccessTrue): n/a

#### `TEST(LlamaCppPluginFocusedTests, O3_NotLoadedErrorMessageDescriptive)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:503
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (O3_NotLoadedErrorMessageDescriptive): n/a

#### `TEST(LlamaCppPluginFocusedTests, P1_ConcurrentGenerateNoRaceOrDeadlock)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:520
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (P1_ConcurrentGenerateNoRaceOrDeadlock): n/a

#### `TEST(LlamaCppPluginFocusedTests, P2_ConcurrentGenerateBatchCorrectResponseCount)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:557
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (P2_ConcurrentGenerateBatchCorrectResponseCount): n/a

#### `TEST(LlamaCppPluginFocusedTests, P3_ConcurrentLoraRegistryAndGenerate)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:593
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (P3_ConcurrentLoraRegistryAndGenerate): n/a

#### `TEST(LlamaCppPluginFocusedTests, Q1_EmbedFn_InjectedFnCalled)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:639
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (Q1_EmbedFn_InjectedFnCalled): n/a

#### `TEST(LlamaCppPluginFocusedTests, Q2_EmbedFn_EmptyReturnFallsBackToStub)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:657
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (Q2_EmbedFn_EmptyReturnFallsBackToStub): n/a

#### `TEST(LlamaCppPluginFocusedTests, Q3_EmbedFn_ClearingRevertsToStub)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:673
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (Q3_EmbedFn_ClearingRevertsToStub): n/a

#### `TEST(LlamaCppPluginFocusedTests, Q4_EmbedFn_UsedEvenWithoutModel)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:692
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (Q4_EmbedFn_UsedEvenWithoutModel): n/a

#### `TEST(LlamaCppPluginFocusedTests, R1_GenerateFn_InjectedFnCalled)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:707
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (R1_GenerateFn_InjectedFnCalled): n/a

#### `TEST(LlamaCppPluginFocusedTests, R2_GenerateFn_ExceptionFailsClosed)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:729
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (R2_GenerateFn_ExceptionFailsClosed): n/a

#### `TEST(LlamaCppPluginFocusedTests, S1_CapabilitiesReportsFunctionCallSupport)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:746
- Brief: S1: getCapabilities() reports supports_function_call = true.
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (S1_CapabilitiesReportsFunctionCallSupport): n/a

#### `TEST(LlamaCppPluginFocusedTests, S2_StubMode_ToolCallSynthesized)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:753
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (S2_StubMode_ToolCallSynthesized): n/a
- Details: S2: generate() with tools synthesises a stub tool-call JSON response in THEMIS_LLAMA_CPP_STUB_MODE (the test binary always defines this macro).

#### `TEST(LlamaCppPluginFocusedTests, S3_BridgeFn_ToolCallsForwarded)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:776
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (S3_BridgeFn_ToolCallsForwarded): n/a
- Details: S3: generate() with tools via generate_fn_ bridge passes tools through and tool_calls populated by the bridge are forwarded unchanged.

#### `TEST(LlamaCppPluginFocusedTests, T1_CancellationToken_PreCancelledRejectsRequest)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:811
- Brief: T1: generate() returns cancelled error when token is pre-set to true.
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (T1_CancellationToken_PreCancelledRejectsRequest): n/a

#### `TEST(LlamaCppPluginFocusedTests, T2_CancellationToken_NotSetAllowsGeneration)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:837
- Brief: T2: generate() succeeds normally when token exists but is false.
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (T2_CancellationToken_NotSetAllowsGeneration): n/a

#### `TEST(LlamaCppPluginFocusedTests, U1_ConcurrentGenerateRAGAndLoadLoRA_NoCrashOrDeadlock)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:855
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (U1_ConcurrentGenerateRAGAndLoadLoRA_NoCrashOrDeadlock): n/a
- Details: U1: 8 threads: half call generateRAG(), half call loadLoRA() concurrently. No crash and all threads join within 10 s.

#### `TEST(LlamaCppPluginFocusedTests, U2_AtomicInferenceCountAccumulatesCorrectly)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:903
- Brief: U2: generate() called 100 times; inference_count in perf stats equals 100.
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (U2_AtomicInferenceCountAccumulatesCorrectly): n/a

#### `TEST(LlamaCppPluginFocusedTests, U3_GenerateRAGNoRagModeMetadata_SucceedsWithStubResponse)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:923
- Brief: U3: generateRAG() with no rag_mode key in metadata succeeds without crash.
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (U3_GenerateRAGNoRagModeMetadata_SucceedsWithStubResponse): n/a

#### `TEST(LlamaCppPluginFocusedTests, U4_FactoryRoundTrip_CreateLoadGenerateDestroy)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:948
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (U4_FactoryRoundTrip_CreateLoadGenerateDestroy): n/a
- Details: U4: Factory round-trip: create -> loadModel -> generate -> destroy. In THEMIS_TEST_BUILD the C-linkage factory is compiled out; we exercise the equivalent heap allocation / deletion path directly.

#### `TEST(LlamaCppPluginFocusedTests, V1_ImportLoRA_BadMagicReturnsFalse)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:972
- Brief: V1: importLoRA with bad magic bytes returns false.
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (V1_ImportLoRA_BadMagicReturnsFalse): n/a

#### `TEST(LlamaCppPluginFocusedTests, V2_ImportLoRA_ValidMagicStubModeReturnsTrue)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:982
- Brief: V2: importLoRA with valid GGUF magic and minimal size returns true in stub mode.
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (V2_ImportLoRA_ValidMagicStubModeReturnsTrue): n/a

#### `TEST(LlamaCppPluginFocusedTests, V3_ImportLoRA_TooSmallReturnsFalse)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:992
- Brief: V3: importLoRA with too-small data (< 8 bytes) returns false.
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (V3_ImportLoRA_TooSmallReturnsFalse): n/a

#### `TEST(LlamaCppPluginFocusedTests, V4_PolicyFn_DenyingFnBlocksGenerate)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:1002
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (V4_PolicyFn_DenyingFnBlocksGenerate): n/a
- Details: V4: setPolicyFn with a denying function causes generate() to return success=false with the denial reason in error_message.

#### `TEST(LlamaCppPluginFocusedTests, V5_PolicyFn_AllowingFnPermitsGenerate)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:1023
- Brief: V5: setPolicyFn with an allowing function lets generate() succeed normally.
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (V5_PolicyFn_AllowingFnPermitsGenerate): n/a

#### `TEST(LlamaCppPluginFocusedTests, V6_PolicyFn_ClearingRevertsToNormalGenerate)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:1041
- Brief: V6: Clearing the policy fn (nullptr) reverts to normal generate behaviour.
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (V6_PolicyFn_ClearingRevertsToNormalGenerate): n/a

#### `TEST(LlamaCppPluginFocusedTests, X1_StreamCallbackRetry_TransientException)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:1076
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (X1_StreamCallbackRetry_TransientException): n/a
- Details: X1: stream_callback that throws std::runtime_error on the first two invocations is retried and increments stream_retry_count_ by >= 1. Requires THEMIS_LLAMA_CPP_STUB_MODE so the callback is exercised in unit-test builds without a real model.

#### `TEST(LlamaCppPluginFocusedTests, X2_GetPerformanceStats_ContainsStreamRetryCount)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:1114
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (X2_GetPerformanceStats_ContainsStreamRetryCount): n/a
- Details: X2: getPerformanceStats() always exposes the "stream_retry_count" key, even when the counter is zero (regression guard).

#### `TEST(LlamaCppPluginFocusedTests, X3_RapidSuccessiveGenerate_NoThreadLeak)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin.cpp`:1128
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginFocusedTests): n/a
  - `<unnamed>` (X3_RapidSuccessiveGenerate_NoThreadLeak): n/a
- Details: X3: Plugin survives 10 rapid successive generate() calls without thread leaks or crashes. This guards the stub generation path against resource leaks while keeping the regression scope focused on production code.

### test_llama_cpp_plugin_lifecycle_focused.cpp

#### `TEST(LlamaCppPluginLifecycleFocusedTests, LC1_InitialState_NotLoaded)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp`:27
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginLifecycleFocusedTests): n/a
  - `<unnamed>` (LC1_InitialState_NotLoaded): n/a

#### `TEST(LlamaCppPluginLifecycleFocusedTests, LC2_LoadModel_TransitionsToLoaded)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp`:34
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginLifecycleFocusedTests): n/a
  - `<unnamed>` (LC2_LoadModel_TransitionsToLoaded): n/a

#### `TEST(LlamaCppPluginLifecycleFocusedTests, LC3_UnloadModel_TransitionsToUnloaded)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp`:42
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginLifecycleFocusedTests): n/a
  - `<unnamed>` (LC3_UnloadModel_TransitionsToUnloaded): n/a

#### `TEST(LlamaCppPluginLifecycleFocusedTests, LC4_Reload_WorksAfterUnload)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginLifecycleFocusedTests): n/a
  - `<unnamed>` (LC4_Reload_WorksAfterUnload): n/a

#### `TEST(LlamaCppPluginLifecycleFocusedTests, LC5_DoubleLoad_ReplacesModel)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp`:63
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginLifecycleFocusedTests): n/a
  - `<unnamed>` (LC5_DoubleLoad_ReplacesModel): n/a

#### `TEST(LlamaCppPluginLifecycleFocusedTests, LC6_DoubleUnload_IsIdempotent)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginLifecycleFocusedTests): n/a
  - `<unnamed>` (LC6_DoubleUnload_IsIdempotent): n/a

#### `TEST(LlamaCppPluginLifecycleFocusedTests, LC7_UnloadNeverLoaded_IsNoOp)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginLifecycleFocusedTests): n/a
  - `<unnamed>` (LC7_UnloadNeverLoaded_IsNoOp): n/a

#### `TEST(LlamaCppPluginLifecycleFocusedTests, LC8_RegistrarCreatedPlugin_ConsistentState)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginLifecycleFocusedTests): n/a
  - `<unnamed>` (LC8_RegistrarCreatedPlugin_ConsistentState): n/a

#### `TEST(LlamaCppPluginLifecycleFocusedTests, LC9_ConcurrentLoadQuery_NoRace)`
- Source: `src/llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPluginLifecycleFocusedTests): n/a
  - `<unnamed>` (LC9_ConcurrentLoadQuery_NoRace): n/a

### test_llama_cpp_registrar_integration_focused.cpp

#### `TEST(LlamaCppRegistrarIntegrationTests, W1_InitFromServerConfig_NoLLMSection)`
- Source: `src/llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp`:25
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppRegistrarIntegrationTests): n/a
  - `<unnamed>` (W1_InitFromServerConfig_NoLLMSection): n/a

#### `TEST(LlamaCppRegistrarIntegrationTests, W2_InitFromServerConfig_EmptyModelPath)`
- Source: `src/llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp`:31
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppRegistrarIntegrationTests): n/a
  - `<unnamed>` (W2_InitFromServerConfig_EmptyModelPath): n/a

#### `TEST(LlamaCppRegistrarIntegrationTests, W3_InitFromServerConfig_NoModelPathKey)`
- Source: `src/llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppRegistrarIntegrationTests): n/a
  - `<unnamed>` (W3_InitFromServerConfig_NoModelPathKey): n/a

#### `TEST(LlamaCppRegistrarIntegrationTests, W4_DefaultReloadCallback_EmptyConfig)`
- Source: `src/llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp`:43
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppRegistrarIntegrationTests): n/a
  - `<unnamed>` (W4_DefaultReloadCallback_EmptyConfig): n/a

#### `TEST(LlamaCppRegistrarIntegrationTests, W5_RegisterWithLLMManager_StubMode)`
- Source: `src/llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp`:50
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppRegistrarIntegrationTests): n/a
  - `<unnamed>` (W5_RegisterWithLLMManager_StubMode): n/a

#### `TEST(LlamaCppRegistrarIntegrationTests, W6_RegisteredPlugin_CanGenerate)`
- Source: `src/llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp`:58
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppRegistrarIntegrationTests): n/a
  - `<unnamed>` (W6_RegisteredPlugin_CanGenerate): n/a

#### `TEST(LlamaCppRegistrarIntegrationTests, W7_InitFromServerConfig_WithModelPath)`
- Source: `src/llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp`:72
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppRegistrarIntegrationTests): n/a
  - `<unnamed>` (W7_InitFromServerConfig_WithModelPath): n/a

#### `TEST(LlamaCppRegistrarIntegrationTests, W8_InitFromServerConfig_Idempotent)`
- Source: `src/llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppRegistrarIntegrationTests): n/a
  - `<unnamed>` (W8_InitFromServerConfig_Idempotent): n/a

### test_llama_cpp_validation_gates_focused.cpp

#### `TEST(LlamaCppValidationGatesFocusedTests, VG1_ContextLength_BelowMin_UsesDefault)`
- Source: `src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp`:41
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppValidationGatesFocusedTests): n/a
  - `<unnamed>` (VG1_ContextLength_BelowMin_UsesDefault): n/a

#### `TEST(LlamaCppValidationGatesFocusedTests, VG2_ContextLength_AtMinBoundary)`
- Source: `src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppValidationGatesFocusedTests): n/a
  - `<unnamed>` (VG2_ContextLength_AtMinBoundary): n/a

#### `TEST(LlamaCppValidationGatesFocusedTests, VG3_ContextLength_AtMaxBoundary)`
- Source: `src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp`:65
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppValidationGatesFocusedTests): n/a
  - `<unnamed>` (VG3_ContextLength_AtMaxBoundary): n/a

#### `TEST(LlamaCppValidationGatesFocusedTests, VG4_MaxTokensZero_IsRejected)`
- Source: `src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppValidationGatesFocusedTests): n/a
  - `<unnamed>` (VG4_MaxTokensZero_IsRejected): n/a

#### `TEST(LlamaCppValidationGatesFocusedTests, VG5_MaxTokensNegative_IsRejected)`
- Source: `src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppValidationGatesFocusedTests): n/a
  - `<unnamed>` (VG5_MaxTokensNegative_IsRejected): n/a

#### `TEST(LlamaCppValidationGatesFocusedTests, VG6_MaxTokensExceedsHalfContext_NotFatal)`
- Source: `src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppValidationGatesFocusedTests): n/a
  - `<unnamed>` (VG6_MaxTokensExceedsHalfContext_NotFatal): n/a

#### `TEST(LlamaCppValidationGatesFocusedTests, VG7_TemperatureOutOfRange_NotFatal)`
- Source: `src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppValidationGatesFocusedTests): n/a
  - `<unnamed>` (VG7_TemperatureOutOfRange_NotFatal): n/a

#### `TEST(LlamaCppValidationGatesFocusedTests, VG8_TopPOutOfRange_NotFatal)`
- Source: `src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppValidationGatesFocusedTests): n/a
  - `<unnamed>` (VG8_TopPOutOfRange_NotFatal): n/a

#### `TEST(LlamaCppValidationGatesFocusedTests, VG9_EmptyPrompt_ReturnsError)`
- Source: `src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppValidationGatesFocusedTests): n/a
  - `<unnamed>` (VG9_EmptyPrompt_ReturnsError): n/a

#### `std::unique_ptr< LlamaCppPlugin > make_loaded()`
- Source: `src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp`:32
- Brief: ── helpers ───────────────────────────────────────────────────────────────────
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: loadModel().

### themis::llamacpp

#### `bool detectCudaAvailable()`
- Source: `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`:67
- Brief: ============================================================================ SECTION 2: CUDA Capability Detection with Caching ============================================================================
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Calls: std::getenv(), std::string(), THEMIS_INFO(), std::chrono::steady_clock::now(), count(), THEMIS_DEBUG(), THEMIS_WARN().

#### `LlamaPluginValidator getLlamaPluginValidator()`
- Source: `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`:353
- Brief: Get Llama Plugin Validator.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: validateModelInitialization(), THEMIS_ERROR(), contains(), is_number(), validateMemoryAllocation(), detectCudaAvailable(), THEMIS_WARN(), THEMIS_INFO().

#### `bool validateMemoryAllocation(size_t model_size, int gpu_layers, std::string &error_msg)`
- Source: `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`:258
- Brief: ============================================================================ SECTION 5: Memory Allocation Validation ============================================================================
- Parameters:
  - `model_size` (size_t): Input parameter.
  - `gpu_layers` (int): Input parameter.
  - `error_msg` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: model_size Input parameter. gpu_layers Input parameter. error_msg Input/output parameter. True when the operation succeeds.

#### `bool validateModelInitialization(const std::string &model_path, const nlohmann::json &config, std::string &error_msg)`
- Source: `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`:177
- Brief: ============================================================================ SECTION 4: Fail-Closed Model Initialization ============================================================================
- Parameters:
  - `model_path` (const std::string &): Path to the model.
  - `config` (const nlohmann::json &): Input parameter.
  - `error_msg` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: model_path Path to the model. config Input parameter. error_msg Input/output parameter. True when the operation succeeds.

#### `bool validateTokenLimits(const themis::llm::InferenceRequest &request, size_t context_length, std::string &error_msg)`
- Source: `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`:119
- Brief: ============================================================================ SECTION 3: Token Limit Validation ============================================================================
- Parameters:
  - `request` (const themis::llm::InferenceRequest &): Input parameter.
  - `context_length` (size_t): Input parameter.
  - `error_msg` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: request Input parameter. context_length Input parameter. error_msg Input/output parameter. True when the operation succeeds.

### themis::llamacpp::LlamaCppPlugin

#### `LlamaCppPlugin()`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:56
- Brief: n/a
- Parameters: none

#### `LlamaCppPlugin(LlamaCppPlugin &&) noexcept=default`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPlugin &&): n/a

#### `std::string computeFileDigest(const std::string &path)`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:241
- Brief: Compute a hex digest of the file at path. Uses FNV-64 as a CI-safe placeholder; swap for SHA-256 (OpenSSL EVP) in production deployments where libcrypto is available. Returns empty string on I/O error.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Details: ── computeFileDigest ───────────────────────────────────────────────────────── path Input parameter. Return value. Calls: f(), read(), gcount(), std::setw(), std::setfill(), str().

#### `std::vector< std::vector< float > > computeTargetLogitsForTokens(const llm::InferenceRequest &request, const std::vector< int > &draft_token_ids)`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:117
- Brief: Compute exact target-model logits for supplied speculative draft tokens.
- Parameters:
  - `request` (const llm::InferenceRequest &): Input parameter.
  - `draft_token_ids` (const std::vector< int > &): Input parameter.
- Return: (K+1) x vocab_size raw target-logit matrix.
- Throws:
  - std::runtime_error: if no live llama.cpp wrapper/model is available.
  - std::invalid_argument: if any supplied token is outside the loaded vocabulary range.
  - std::runtime_error: if an error occurs.
- Details: Compute Target Logits For Tokens. Forwards to the live LlamaWrapper when a real llama.cpp model is loaded. The returned matrix contains exactly draft_token_ids.size() + 1 rows and is directly compatible with SpeculativeDecoder::verify(). request Verification request whose prompt defines the current prefix. draft_token_ids Draft token IDs to evaluate in target-vocabulary space. (K+1) x vocab_size raw target-logit matrix. std::runtime_error if no live llama.cpp wrapper/model is available. std::invalid_argument if any supplied token is outside the loaded vocabulary range. request Input parameter. draft_token_ids Input parameter. Return value. std::runtime_error if an error occurs. Calls: lock(), else().

#### `std::vector< float > embed(const std::string &text) override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:79
- Brief: ── embed ─────────────────────────────────────────────────────────────────────
- Parameters:
  - `text` (const std::string &): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Calls: lock(), embed_fn_(), empty().

#### `std::vector< uint8_t > exportLoRA(const std::string &lora_id) override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:126
- Brief: ── LoRA import/export ────────────────────────────────────────────────────────
- Parameters:
  - `lora_id` (const std::string &): Identifier of the lora.
- Return: Return value.
- Details: lora_id Identifier of the lora. Return value. Calls: lock(), else().

#### `llm::InferenceResponse generate(const llm::InferenceRequest &request) override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:76
- Brief: ── generate ──────────────────────────────────────────────────────────────────
- Parameters:
  - `request` (const llm::InferenceRequest &): Input parameter.
- Return: Return value.
- Details: request Input parameter. Return value. Calls: stream_callback(), lock(), get(), policy_fn(), load(), what(), generate_fn(), empty().

#### `std::vector< llm::InferenceResponse > generateBatch(const std::vector< llm::InferenceRequest > &requests)`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:206
- Brief: Batch inference over multiple requests.
- Parameters:
  - `requests` (const std::vector< llm::InferenceRequest > &): Input parameter.
- Return: Vector of InferenceResponse, same size as requests.
- Details: ── generateBatch ───────────────────────────────────────────────────────────── Iterates over requests and calls generate() for each one. Order of responses mirrors order of requests. requests Vector of inference requests. Vector of InferenceResponse, same size as requests. requests Input parameter. Return value. Calls: reserve(), size(), push_back(), generate().

#### `llm::ILLMPlugin::DraftTokensResult generateDraftTokens(const llm::InferenceRequest &request, size_t k, size_t vocab_size_hint) override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:98
- Brief: Generate K draft tokens with per-token logit distributions.
- Parameters:
  - `request` (const llm::InferenceRequest &): Input parameter.
  - `k` (size_t): Input parameter.
  - `vocab_size_hint` (size_t): Input parameter.
- Return: DraftTokensResult with k tokens and k logit rows (empty when k==0).
- Details: ── generateDraftTokens ──────────────────────────────────────────────────────── Phase 2 Implementation: Real draft-logit pipeline using llama_get_logits() from the underlying llama.cpp context. Returns actual logit distributions for speculative decoding verification. When k is 0 the function returns an empty result immediately without acquiring any significant resources. vocab_size_hint values exceeding 65 536 are capped to bound memory allocation in stub/fallback mode. request Inference request (prompt + generation parameters). k Number of draft tokens to produce (0 is valid). vocab_size_hint Expected vocabulary size; 32000 used as fallback and capped in stub/fallback mode to bound memory usage. DraftTokensResult with k tokens and k logit rows (empty when k==0). request Input parameter. k Input parameter. vocab_size_hint Input parameter. Return value. Calls: lock(), std::min(), spdlog::warn(), push_back(), logits(), std::move(), empty(), size().

#### `llm::InferenceResponse generateRAG(const llm::RAGContext &rag_context, const llm::InferenceRequest &request) override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:77
- Brief: Generate RAG.
- Parameters:
  - `rag_context` (const llm::RAGContext &): Input parameter.
  - `request` (const llm::InferenceRequest &): Input parameter.
- Return: Return value.
- Details: rag_context Input parameter. request Input parameter. Return value. Calls: lock(), reserve(), size(), push_back(), std::move(), is_object(), find(), end().

#### `llm::InferenceResponse generateStream(llm::InferenceRequest request, std::function< void(const std::string &token)> token_callback)`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:193
- Brief: Streaming generation convenience wrapper.
- Parameters:
  - `request` (llm::InferenceRequest): Inference parameters (stream_callback field is overwritten).
  - `token_callback` (std::function< void(const std::string &token)>): Called once per token; must not throw.
- Return: Full InferenceResponse (same as generate()).
- Details: Calls generate() with the given request, injecting token_callback into InferenceRequest::stream_callback. When a real llama.cpp model is wired in, each sampled token is forwarded to the callback before the full response is returned. In stub mode the callback receives the stub response as a single "token" so that callers always get at least one callback invocation. request Inference parameters (stream_callback field is overwritten). token_callback Called once per token; must not throw. Full InferenceResponse (same as generate()).

#### `llm::LLMCapabilities getCapabilities() const override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:122
- Brief: n/a
- Parameters: none

#### `json getMemoryStats() const override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:123
- Brief: n/a
- Parameters: none

#### `std::string getModelId() const`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:210
- Brief: n/a
- Parameters: none

#### `std::optional< llm::ModelInfo > getModelInfo() const override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:65
- Brief: n/a
- Parameters: none

#### `json getPerformanceStats() const override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:124
- Brief: n/a
- Parameters: none

#### `std::string getPluginVersion() const`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:209
- Brief: n/a
- Parameters: none

#### `bool importLoRA(const std::string &lora_id, const std::vector< uint8_t > &data) override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:127
- Brief: Import Lo RA.
- Parameters:
  - `lora_id` (const std::string &): Identifier of the lora.
  - `data` (const std::vector< uint8_t > &): Input parameter.
- Return: True when the operation succeeds.
- Details: lora_id Identifier of the lora. data Input parameter. True when the operation succeeds. Calls: size(), lock(), else().

#### `bool isModelLoaded() const override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:66
- Brief: n/a
- Parameters: none

#### `std::vector< llm::LoRAInfo > listLoRAs() const override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:74
- Brief: n/a
- Parameters: none

#### `bool loadLoRA(const std::string &lora_id, const std::string &lora_path, float scale) override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:69
- Brief: n/a
- Parameters:
  - `lora_id` (const std::string &): Identifier of the lora.
  - `lora_path` (const std::string &): Path to the lora.
  - `scale` (float): Input parameter.
- Return: True when the operation succeeds.
- Details: ── LoRA management ─────────────────────────────────────────────────────────── lora_id Identifier of the lora. lora_path Path to the lora. scale Input parameter. True when the operation succeeds. Calls: lock(), erase(), std::remove_if(), begin(), end(), push_back().

#### `bool loadModel(const std::string &model_path, const json &config) override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:63
- Brief: ── loadModel / unloadModel ───────────────────────────────────────────────────
- Parameters:
  - `model_path` (const std::string &): Path to the model.
  - `config` (const json &): Input parameter.
- Return: True when the operation succeeds.
- Details: model_path Path to the model. config Input parameter. True when the operation succeeds. Calls: lock(), empty(), contains(), is_number(), reset(), std::move(), getModelInfo(), value().

#### `LlamaCppPlugin & operator=(LlamaCppPlugin &&) noexcept=default`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlamaCppPlugin &&): n/a

#### `void setEmbedFn(EmbedFn fn)`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:147
- Brief: ── setEmbedFn ────────────────────────────────────────────────────────────────
- Parameters:
  - `fn` (EmbedFn): Input parameter.
- Details: fn Input parameter. Calls: lock(), std::move().

#### `void setGenerateFn(GenerateFn fn)`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:158
- Brief: Set Generate Fn.
- Parameters:
  - `fn` (GenerateFn): Input parameter.
- Details: fn Input parameter. Calls: lock(), std::move().

#### `void setPolicyFn(PolicyFn fn)`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:178
- Brief: Set Policy Fn.
- Parameters:
  - `fn` (PolicyFn): Input parameter.
- Details: fn Input parameter. Calls: lock(), std::move().

#### `bool unloadLoRA(const std::string &lora_id) override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:72
- Brief: n/a
- Parameters:
  - `lora_id` (const std::string &): Identifier of the lora.
- Return: True when the operation succeeds.
- Details: Unload Lo RA. lora_id Identifier of the lora. True when the operation succeeds. Calls: lock(), size(), erase(), std::remove_if(), begin(), end().

#### `void unloadModel() override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:64
- Brief: Unload Model.
- Parameters: none
- Details: Calls: lock(), clear(), reset().

#### `~LlamaCppPlugin() override`
- Source: `include/llama_cpp/llama_cpp_plugin.h`:57
- Brief: n/a
- Parameters: none

### themis::llamacpp::LlamaCppPluginRegistrar

#### `LlamaCppPluginRegistrar()=delete`
- Source: `include/llama_cpp/llama_cpp_registrar.h`:161
- Brief: n/a
- Parameters: none

#### `std::unique_ptr< llm::LLMPluginAdapter > createAdapter(const json &config={})`
- Source: `include/llama_cpp/llama_cpp_registrar.h`:84
- Brief: Create an LLMPluginAdapter wrapping a LlamaCppPlugin.
- Parameters:
  - `config` (const json &): Input parameter.
- Return: Heap-allocated adapter, caller owns.
- Details: ── createAdapter ───────────────────────────────────────────────────────────── The adapter implements IThemisPlugin so it can be handed to the unified plugins::PluginManager. Internally it delegates to LlamaCppPlugin for all LLM operations. config Optional JSON configuration. Heap-allocated adapter, caller owns. config Input parameter. Return value. Calls: createPlugin(), std::move().

#### `std::unique_ptr< LlamaCppPlugin > createPlugin(const json &config={})`
- Source: `include/llama_cpp/llama_cpp_registrar.h`:72
- Brief: Create a standalone LlamaCppPlugin instance.
- Parameters:
  - `config` (const json &): Input parameter.
- Return: Heap-allocated plugin, caller owns.
- Details: ── createPlugin ───────────────────────────────────────────────────────────── config Optional JSON configuration forwarded to loadModel(). Keys: "model_path" (string), "n_ctx" / "context_length" (integer), "n_gpu_layers" (integer). Heap-allocated plugin, caller owns. config Input parameter. Return value. Calls: contains(), is_string(), empty(), loadModel().

#### `ReloadCallback defaultReloadCallback()`
- Source: `include/llama_cpp/llama_cpp_registrar.h`:124
- Brief: Default hot-plug reload callback.
- Parameters: none
- Details: Calls loadModel(config["model_path"], config) when "model_path" is present; otherwise returns false (or true in stub mode).

#### `bool initFromServerConfig(const json &server_config)`
- Source: `include/llama_cpp/llama_cpp_registrar.h`:158
- Brief: Initialize the LLM plugin subsystem from server configuration.
- Parameters:
  - `server_config` (const json &): Input parameter.
- Return: true if registration succeeded or no plugin was needed; false if plugin creation or model loading failed.
- Details: ── initFromServerConfig ────────────────────────────────────────────────────── Called during server startup. Reads the LLM configuration block from server_config, and if a model path is configured, registers a LlamaCppPlugin with the global LLMPluginManager singleton under the name "llama_cpp". This method is a no-op (returns true) when: server_config contains no "llm" key, or the "llm" object has no "model_path" key, or model_path is an empty string. In those cases the server starts in stub / CI mode without any model loaded. When a non-empty model_path is present the plugin is created, loadModel() is called, and the plugin is registered with LLMPluginManager::instance(). server_config Full server configuration JSON object. Expected key path: config["llm"]["model_path"] true if registration succeeded or no plugin was needed; false if plugin creation or model loading failed. jsoncfg=load_server_config("config.json"); if(!LlamaCppPluginRegistrar::initFromServerConfig(cfg)){ LOG_ERROR("LlamaCpppluginfailedtoinitialise"); } server_config Input parameter. True when the operation succeeds. Calls: contains(), value(), empty(), themis::llm::LLMPluginManager::instance(), registerWithLLMManager().

#### `bool registerWithLLMManager(llm::LLMPluginManager &manager, const std::string &plugin_name="llama_cpp", const json &config={})`
- Source: `include/llama_cpp/llama_cpp_registrar.h`:100
- Brief: Register a new LlamaCppPlugin instance with an LLMPluginManager.
- Parameters:
  - `manager` (llm::LLMPluginManager &): Input/output parameter.
  - `plugin_name` (const std::string &): Name of the plugin.
  - `config` (const json &): Input parameter.
- Return: true on success.
- Details: ── registerWithLLMManager ──────────────────────────────────────────────────── Creates a fresh plugin, optionally calls loadModel() when config["model_path"] is present, and hands ownership to the manager. manager Target LLMPluginManager. plugin_name Name under which the plugin is registered. config Optional JSON configuration. true on success. manager Input/output parameter. plugin_name Name of the plugin. config Input parameter. True when the operation succeeds. Calls: createPlugin(), registerPlugin(), std::move().

### themis::llamacpp::ModelLoadingGuard

#### `ModelLoadingGuard(const std::string &model_id, int timeout_seconds)`
- Source: `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`:303
- Brief: n/a
- Parameters:
  - `model_id` (const std::string &): n/a
  - `timeout_seconds` (int): n/a

#### `int getRemainingSeconds() const`
- Source: `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`:327
- Brief: n/a
- Parameters: none

#### `bool isTimedOut() const`
- Source: `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`:322
- Brief: n/a
- Parameters: none

#### `~ModelLoadingGuard()`
- Source: `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`:310
- Brief: n/a
- Parameters: none

### themis::test::wave_d

#### `TEST(WaveD_LlamaCppStress, AccelerationFailClosedStress)`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:176
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_LlamaCppStress): n/a
  - `<unnamed>` (AccelerationFailClosedStress): n/a

#### `TEST(WaveD_LlamaCppStress, ConcurrentModelLoadStress)`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_LlamaCppStress): n/a
  - `<unnamed>` (ConcurrentModelLoadStress): n/a

#### `TEST(WaveD_LlamaCppStress, HighCardinalityTokenGeneration)`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:113
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_LlamaCppStress): n/a
  - `<unnamed>` (HighCardinalityTokenGeneration): n/a

### themis::test::wave_d::StubLlamaAccelGuard

#### `StubLlamaAccelGuard(bool available)`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:90
- Brief: n/a
- Parameters:
  - `available` (bool): n/a

#### `long accelerated() const`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:100
- Brief: n/a
- Parameters: none

#### `long attempts() const`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:99
- Brief: n/a
- Parameters: none

#### `long fallbacks() const`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:101
- Brief: n/a
- Parameters: none

#### `bool handleRequest(uint64_t)`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:92
- Brief: n/a
- Parameters:
  - `<unnamed>` (uint64_t): n/a

### themis::test::wave_d::StubLlamaRegistry

#### `bool load(const std::string &model_path)`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:65
- Brief: n/a
- Parameters:
  - `model_path` (const std::string &): n/a

#### `long loads() const`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:79
- Brief: n/a
- Parameters: none

#### `bool unload(const std::string &model_path)`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:72
- Brief: n/a
- Parameters:
  - `model_path` (const std::string &): n/a

#### `long unloads() const`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:80
- Brief: n/a
- Parameters: none

### themis::test::wave_d::StubTokenStore

#### `bool generate(const std::string &prompt, const std::string &tokens)`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:44
- Brief: n/a
- Parameters:
  - `prompt` (const std::string &): n/a
  - `tokens` (const std::string &): n/a

#### `std::size_t size() const`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:51
- Brief: n/a
- Parameters: none

#### `long writes() const`
- Source: `tests/llama_cpp/test_llama_cpp_highcardinality_stress.cpp`:56
- Brief: n/a
- Parameters: none

