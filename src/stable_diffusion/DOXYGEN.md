# STABLE_DIFFUSION DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\stable_diffusion\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\stable_diffusion\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 14
- Compounds: 45
- Classes/Structs: 13
- Namespaces: 9
- File Compounds: 14

## Namespaces
- @051141366136176200252373111272065044332216220126
- @240033151313032226017254161057352217122211045305
- @301124222063165063334052020124050116054212345140
- plugins
- themis
- themis::imggen
- themis::plugins
- themis::test
- themis::test::wave_d

## Types
### Classes
- LoRAFailingGenerator
- ThrowingGenerator
- themis::imggen::ISDGenerator
- themis::imggen::InMemorySDGenerator
- themis::imggen::SDPlugin
- themis::imggen::SDPluginAdapter
- themis::imggen::SDPluginRegistrar
- themis::imggen::SDPromptSanitizer
- themis::imggen::SDStubGenerator

### Structs
- themis::imggen::SDConfig
- themis::test::wave_d::StubAccelerationGuard
- themis::test::wave_d::StubInferenceStore
- themis::test::wave_d::StubModelRegistry

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 175

### LoRAFailingGenerator

#### `bool applyLoRA(const std::string &, float, std::string &error_out) override`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:49
- Brief: Optionally apply a LoRA adapter for subsequent generation calls.
- Parameters:
  - `lora_path` (const std::string &): Path to LoRA adapter (empty means clear LoRA state).
  - `scale` (float): Adapter scale factor.
  - `error_out` (std::string &): Detailed error text on failure.
- Return: true on success, false on validation/application failure.
- Details: Default implementation is a no-op that succeeds for generators that do not support LoRA updates. lora_path Path to LoRA adapter (empty means clear LoRA state). scale Adapter scale factor. error_out Detailed error text on failure. true on success, false on validation/application failure.

### ThrowingGenerator

#### `std::vector< uint8_t > generate(const std::string &, const SDGenerationConfig &, int &w, int &h, uint64_t &seed) override`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:34
- Brief: Generate raw RGB pixel data for a given prompt.
- Parameters:
  - `prompt` (const std::string &): Sanitized text prompt.
  - `cfg` (const SDGenerationConfig &): Per-request generation config.
  - `out_width` (int &): Actual width of the generated image.
  - `out_height` (int &): Actual height of the generated image.
  - `out_seed` (uint64_t &): Seed that was actually used (important for -1 random seeds).
- Return: Raw RGB byte buffer (width * height * 3 bytes).
- Details: prompt Sanitized text prompt. cfg Per-request generation config. out_width Actual width of the generated image. out_height Actual height of the generated image. out_seed Seed that was actually used (important for -1 random seeds). Raw RGB byte buffer (width * height * 3 bytes).

#### `std::string getModelId() const override`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:40
- Brief: n/a
- Parameters: none

#### `bool initialize(const SDConfig &cfg) override`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:30
- Brief: n/a
- Parameters:
  - `cfg` (const SDConfig &): n/a

#### `bool isInitialized() const override`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:33
- Brief: n/a
- Parameters: none

### sd_plugin.h

#### `THEMIS_IMGGEN_PLUGIN()`
- Source: `include/stable_diffusion/sd_plugin.h`:141
- Brief: n/a
- Parameters: none

### test_sd_plugin.cpp

#### `TEST(SDPluginFocusedTests, A1_FromJsonDefaults)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:57
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (A1_FromJsonDefaults): n/a

#### `TEST(SDPluginFocusedTests, A2_FromJsonCustomValues)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:66
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (A2_FromJsonCustomValues): n/a

#### `TEST(SDPluginFocusedTests, A3_FromJsonClampsInvalidDimensions)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (A3_FromJsonClampsInvalidDimensions): n/a

#### `TEST(SDPluginFocusedTests, B1_ToJsonRoundTrip)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (B1_ToJsonRoundTrip): n/a

#### `TEST(SDPluginFocusedTests, B2_ToJsonContainsAllKeys)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (B2_ToJsonContainsAllKeys): n/a

#### `TEST(SDPluginFocusedTests, B3_CfgScaleRoundTrip)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (B3_CfgScaleRoundTrip): n/a

#### `TEST(SDPluginFocusedTests, B4_ToJsonRoundTripNewFields)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:114
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (B4_ToJsonRoundTripNewFields): n/a

#### `TEST(SDPluginFocusedTests, C1_AllowsCleanPrompt)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (C1_AllowsCleanPrompt): n/a

#### `TEST(SDPluginFocusedTests, C2_BlocksMatchingKeyword)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (C2_BlocksMatchingKeyword): n/a

#### `TEST(SDPluginFocusedTests, C3_CaseInsensitiveBlocking)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (C3_CaseInsensitiveBlocking): n/a

#### `TEST(SDPluginFocusedTests, D1_SanitizeRemovesKeyword)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:143
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (D1_SanitizeRemovesKeyword): n/a

#### `TEST(SDPluginFocusedTests, D2_SanitizeMultipleOccurrences)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (D2_SanitizeMultipleOccurrences): n/a

#### `TEST(SDPluginFocusedTests, D3_SanitizeEmptyListReturnsUnchanged)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:155
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (D3_SanitizeEmptyListReturnsUnchanged): n/a

#### `TEST(SDPluginFocusedTests, E1_InitializeReturnsTrue)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:162
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (E1_InitializeReturnsTrue): n/a

#### `TEST(SDPluginFocusedTests, E2_GenerateReturnsPresetPixels)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:169
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (E2_GenerateReturnsPresetPixels): n/a

#### `TEST(SDPluginFocusedTests, E3_GenerateDefaultDimensions)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (E3_GenerateDefaultDimensions): n/a

#### `TEST(SDPluginFocusedTests, F1_InitializeViaDI)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (F1_InitializeViaDI): n/a

#### `TEST(SDPluginFocusedTests, F2_GenerateAfterInit)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (F2_GenerateAfterInit): n/a

#### `TEST(SDPluginFocusedTests, F3_IsPromptAllowedDelegates)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:207
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (F3_IsPromptAllowedDelegates): n/a

#### `TEST(SDPluginFocusedTests, G1_GenerationTimestampPositive)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:215
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (G1_GenerationTimestampPositive): n/a

#### `TEST(SDPluginFocusedTests, G2_PluginVersionIs2_1_0)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (G2_PluginVersionIs2_1_0): n/a

#### `TEST(SDPluginFocusedTests, G3_PromptHashNotEmpty)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:229
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (G3_PromptHashNotEmpty): n/a

#### `TEST(SDPluginFocusedTests, H1_BlockedPromptReturnsError)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (H1_BlockedPromptReturnsError): n/a

#### `TEST(SDPluginFocusedTests, H2_BlockedCountIncrements)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:247
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (H2_BlockedCountIncrements): n/a

#### `TEST(SDPluginFocusedTests, H3_AllowedPromptDoesNotIncrementBlockedCount)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:255
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (H3_AllowedPromptDoesNotIncrementBlockedCount): n/a

#### `TEST(SDPluginFocusedTests, I1_StatisticsContainsRequiredKeys)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:264
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (I1_StatisticsContainsRequiredKeys): n/a

#### `TEST(SDPluginFocusedTests, I2_StatisticsPluginName)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (I2_StatisticsPluginName): n/a

#### `TEST(SDPluginFocusedTests, I3_GenerationCountIncrements)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (I3_GenerationCountIncrements): n/a

#### `TEST(SDPluginFocusedTests, J1_UninitializedGenerateReturnsError)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (J1_UninitializedGenerateReturnsError): n/a

#### `TEST(SDPluginFocusedTests, J2_GeneratorThrowsReturnsError)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:297
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (J2_GeneratorThrowsReturnsError): n/a

#### `TEST(SDPluginFocusedTests, J3_DoubleInitIsSafe)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:305
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (J3_DoubleInitIsSafe): n/a

#### `TEST(SDPluginFocusedTests, J4_InitializeFailsOnModelShaMismatch)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (J4_InitializeFailsOnModelShaMismatch): n/a

#### `TEST(SDPluginFocusedTests, J5_InitializeSucceedsOnModelShaMatch)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:334
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (J5_InitializeSucceedsOnModelShaMatch): n/a

#### `TEST(SDPluginFocusedTests, K1_BlockedNegativePromptReturnsError)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:361
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (K1_BlockedNegativePromptReturnsError): n/a

#### `TEST(SDPluginFocusedTests, K2_BlockedNegativePromptIncrementsBlockedCount)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:371
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (K2_BlockedNegativePromptIncrementsBlockedCount): n/a

#### `TEST(SDPluginFocusedTests, K3_EmptyNegativePromptDoesNotBlock)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:380
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (K3_EmptyNegativePromptDoesNotBlock): n/a

#### `TEST(SDPluginFocusedTests, K4_DimensionGuardRejectsInvalidSize)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (K4_DimensionGuardRejectsInvalidSize): n/a

#### `TEST(SDPluginFocusedTests, L1_BatchReturnsOneResultPerPrompt)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:404
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (L1_BatchReturnsOneResultPerPrompt): n/a

#### `TEST(SDPluginFocusedTests, L2_BatchBlockedPromptDoesNotBlockOthers)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:416
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (L2_BatchBlockedPromptDoesNotBlockOthers): n/a

#### `TEST(SDPluginFocusedTests, L3_BatchEmptyPromptsListReturnsEmpty)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:429
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (L3_BatchEmptyPromptsListReturnsEmpty): n/a

#### `TEST(SDPluginFocusedTests, M1_Img2ImgSuccessAfterInit)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:438
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (M1_Img2ImgSuccessAfterInit): n/a

#### `TEST(SDPluginFocusedTests, M2_Img2ImgBlockedPromptReturnsError)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:453
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (M2_Img2ImgBlockedPromptReturnsError): n/a

#### `TEST(SDPluginFocusedTests, M3_Img2ImgUninitializedReturnsError)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:463
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (M3_Img2ImgUninitializedReturnsError): n/a

#### `TEST(SDPluginFocusedTests, N1_Img2ImgGeneratorPathTaken)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:474
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (N1_Img2ImgGeneratorPathTaken): n/a

#### `TEST(SDPluginFocusedTests, N2_Img2ImgStrengthRecorded)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:487
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (N2_Img2ImgStrengthRecorded): n/a

#### `TEST(SDPluginFocusedTests, N3_Img2ImgInputDimensionsRecorded)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:500
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (N3_Img2ImgInputDimensionsRecorded): n/a

#### `TEST(SDPluginFocusedTests, N4_ControlNetAndLoRAFieldsPassedToGenerator)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:513
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (N4_ControlNetAndLoRAFieldsPassedToGenerator): n/a

#### `TEST(SDPluginFocusedTests, N5_LoRAApplyFailureReturnsError)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:541
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (N5_LoRAApplyFailureReturnsError): n/a

#### `TEST(SDPluginFocusedTests, N6_ControlNetAndLoRAFieldsPassedToImg2Img)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:556
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (N6_ControlNetAndLoRAFieldsPassedToImg2Img): n/a

#### `TEST(SDPluginFocusedTests, N7_EmptyLoraPathClearsPreviousStateOnGenerate)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:584
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (N7_EmptyLoraPathClearsPreviousStateOnGenerate): n/a

#### `TEST(SDPluginFocusedTests, N8_EmptyLoraPathClearsPreviousStateOnImg2Img)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:610
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (N8_EmptyLoraPathClearsPreviousStateOnImg2Img): n/a

#### `TEST(SDPluginFocusedTests, O1_Img2ImgConfigDefaultStrength)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:644
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (O1_Img2ImgConfigDefaultStrength): n/a

#### `TEST(SDPluginFocusedTests, O2_Img2ImgConfigInheritsSdGenerationConfig)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:652
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (O2_Img2ImgConfigInheritsSdGenerationConfig): n/a

#### `TEST(SDPluginFocusedTests, O3_GenerateBatchCountsGenerations)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:660
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (O3_GenerateBatchCountsGenerations): n/a

#### `TEST(SDPluginFocusedTests, P1_PngSignatureCorrect)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:764
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (P1_PngSignatureCorrect): n/a

#### `TEST(SDPluginFocusedTests, P2_PngContainsIdatChunk)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:784
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (P2_PngContainsIdatChunk): n/a

#### `TEST(SDPluginFocusedTests, P3_PngIhdrDimensionsMatchRequest)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:801
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (P3_PngIhdrDimensionsMatchRequest): n/a

#### `TEST(SDPluginFocusedTests, P4_PerceptualHashDeterministic)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:815
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (P4_PerceptualHashDeterministic): n/a

#### `TEST(SDPluginFocusedTests, P5_PerceptualHashNonFatalWhenUnavailable)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:833
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (P5_PerceptualHashNonFatalWhenUnavailable): n/a

#### `TEST(SDPluginFocusedTests, Q1_StubImg2ImgUsesInputDimensions)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:848
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (Q1_StubImg2ImgUsesInputDimensions): n/a

#### `TEST(SDPluginFocusedTests, Q2_StubImg2ImgReturnsInputData)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:863
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (Q2_StubImg2ImgReturnsInputData): n/a

#### `TEST(SDPluginFocusedTests, Q3_StubImg2ImgFallsBackWhenInputEmpty)`
- Source: `src/stable_diffusion/tests/test_sd_plugin.cpp`:878
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginFocusedTests): n/a
  - `<unnamed>` (Q3_StubImg2ImgFallsBackWhenInputEmpty): n/a

### test_sd_plugin_real_backend_e2e.cpp

#### `TEST(SDPluginRealBackendE2ETests, E2E_Text2ImgAndImg2Img_WithModelSha256Gate)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_real_backend_e2e.cpp`:33
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRealBackendE2ETests): n/a
  - `<unnamed>` (E2E_Text2ImgAndImg2Img_WithModelSha256Gate): n/a

#### `TEST(SDPluginRealBackendE2ETests, ParallelAudit_RealBackendGenerate)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_real_backend_e2e.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRealBackendE2ETests): n/a
  - `<unnamed>` (ParallelAudit_RealBackendGenerate): n/a

### test_sd_plugin_registrar.cpp

#### `TEST(SDPluginRegistrarTests, A1_CreatePluginStubMode)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:24
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (A1_CreatePluginStubMode): n/a

#### `TEST(SDPluginRegistrarTests, A2_CreatePluginEmptyModelPath)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:31
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (A2_CreatePluginEmptyModelPath): n/a

#### `TEST(SDPluginRegistrarTests, A3_CreatePluginWithModelPath)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:39
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (A3_CreatePluginWithModelPath): n/a

#### `TEST(SDPluginRegistrarTests, B1_CreateAdapterNotNull)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (B1_CreateAdapterNotNull): n/a

#### `TEST(SDPluginRegistrarTests, B2_AdapterTypeIsImageGeneration)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:54
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (B2_AdapterTypeIsImageGeneration): n/a

#### `TEST(SDPluginRegistrarTests, B3_AdapterGetInstanceReturnsSDPlugin)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (B3_AdapterGetInstanceReturnsSDPlugin): n/a

#### `TEST(SDPluginRegistrarTests, C1_AdapterNameAndVersion)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:71
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (C1_AdapterNameAndVersion): n/a

#### `TEST(SDPluginRegistrarTests, C2_AdapterCapabilitiesBatchingAndThreadSafe)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (C2_AdapterCapabilitiesBatchingAndThreadSafe): n/a

#### `TEST(SDPluginRegistrarTests, C3_AdapterInitializeAndShutdownCycle)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:87
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (C3_AdapterInitializeAndShutdownCycle): n/a

#### `TEST(SDPluginRegistrarTests, D1_DefaultReloadCallbackStubMode)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (D1_DefaultReloadCallbackStubMode): n/a

#### `TEST(SDPluginRegistrarTests, D2_DefaultReloadCallbackWithPath)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:117
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (D2_DefaultReloadCallbackWithPath): n/a

#### `TEST(SDPluginRegistrarTests, D3_DefaultReloadCallbackEmptyPath)`
- Source: `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (SDPluginRegistrarTests): n/a
  - `<unnamed>` (D3_DefaultReloadCallbackEmptyPath): n/a

### themis::imggen::ISDGenerator

#### `bool applyLoRA(const std::string &lora_path, float scale, std::string &error_out)`
- Source: `include/stable_diffusion/sd_generator.h`:54
- Brief: Optionally apply a LoRA adapter for subsequent generation calls.
- Parameters:
  - `lora_path` (const std::string &): Path to LoRA adapter (empty means clear LoRA state).
  - `scale` (float): Adapter scale factor.
  - `error_out` (std::string &): Detailed error text on failure.
- Return: true on success, false on validation/application failure.
- Details: Default implementation is a no-op that succeeds for generators that do not support LoRA updates. lora_path Path to LoRA adapter (empty means clear LoRA state). scale Adapter scale factor. error_out Detailed error text on failure. true on success, false on validation/application failure.

#### `std::vector< uint8_t > generate(const std::string &prompt, const SDGenerationConfig &cfg, int &out_width, int &out_height, uint64_t &out_seed)=0`
- Source: `include/stable_diffusion/sd_generator.h`:72
- Brief: Generate raw RGB pixel data for a given prompt.
- Parameters:
  - `prompt` (const std::string &): Sanitized text prompt.
  - `cfg` (const SDGenerationConfig &): Per-request generation config.
  - `out_width` (int &): Actual width of the generated image.
  - `out_height` (int &): Actual height of the generated image.
  - `out_seed` (uint64_t &): Seed that was actually used (important for -1 random seeds).
- Return: Raw RGB byte buffer (width * height * 3 bytes).
- Details: prompt Sanitized text prompt. cfg Per-request generation config. out_width Actual width of the generated image. out_height Actual height of the generated image. out_seed Seed that was actually used (important for -1 random seeds). Raw RGB byte buffer (width * height * 3 bytes).

#### `std::vector< uint8_t > generateImg2Img(const std::string &prompt, const Img2ImgConfig &cfg, int &out_width, int &out_height, uint64_t &out_seed)`
- Source: `include/stable_diffusion/sd_generator.h`:90
- Brief: Img2Img: generate raw RGB pixel data conditioned on an input image.
- Parameters:
  - `prompt` (const std::string &): Sanitized text prompt.
  - `cfg` (const Img2ImgConfig &): Img2Img config (includes input_image_rgb, strength, mask).
  - `out_width` (int &): Actual width of the generated image.
  - `out_height` (int &): Actual height of the generated image.
  - `out_seed` (uint64_t &): Seed actually used.
- Return: Raw RGB byte buffer (width * height * 3 bytes).
- Details: Default implementation ignores the input image and delegates to generate(). SDCppGenerator will override this with real denoising inference. prompt Sanitized text prompt. cfg Img2Img config (includes input_image_rgb, strength, mask). out_width Actual width of the generated image. out_height Actual height of the generated image. out_seed Seed actually used. Raw RGB byte buffer (width * height * 3 bytes).

#### `std::string getModelId() const =0`
- Source: `include/stable_diffusion/sd_generator.h`:98
- Brief: n/a
- Parameters: none

#### `bool initialize(const SDConfig &cfg)=0`
- Source: `include/stable_diffusion/sd_generator.h`:40
- Brief: n/a
- Parameters:
  - `cfg` (const SDConfig &): n/a

#### `bool isInitialized() const =0`
- Source: `include/stable_diffusion/sd_generator.h`:41
- Brief: n/a
- Parameters: none

#### `~ISDGenerator()=default`
- Source: `include/stable_diffusion/sd_generator.h`:38
- Brief: n/a
- Parameters: none

### themis::imggen::InMemorySDGenerator

#### `bool applyLoRA(const std::string &lora_path, float scale, std::string &error_out) override`
- Source: `include/stable_diffusion/sd_generator.h`:214
- Brief: Optionally apply a LoRA adapter for subsequent generation calls.
- Parameters:
  - `lora_path` (const std::string &): Path to LoRA adapter (empty means clear LoRA state).
  - `scale` (float): Adapter scale factor.
  - `error_out` (std::string &): Detailed error text on failure.
- Return: true on success, false on validation/application failure.
- Details: Default implementation is a no-op that succeeds for generators that do not support LoRA updates. lora_path Path to LoRA adapter (empty means clear LoRA state). scale Adapter scale factor. error_out Detailed error text on failure. true on success, false on validation/application failure.

#### `std::vector< uint8_t > generate(const std::string &, const SDGenerationConfig &cfg, int &out_w, int &out_h, uint64_t &out_seed) override`
- Source: `include/stable_diffusion/sd_generator.h`:187
- Brief: Generate raw RGB pixel data for a given prompt.
- Parameters:
  - `prompt` (const std::string &): Sanitized text prompt.
  - `cfg` (const SDGenerationConfig &): Per-request generation config.
  - `out_width` (int &): Actual width of the generated image.
  - `out_height` (int &): Actual height of the generated image.
  - `out_seed` (uint64_t &): Seed that was actually used (important for -1 random seeds).
- Return: Raw RGB byte buffer (width * height * 3 bytes).
- Details: prompt Sanitized text prompt. cfg Per-request generation config. out_width Actual width of the generated image. out_height Actual height of the generated image. out_seed Seed that was actually used (important for -1 random seeds). Raw RGB byte buffer (width * height * 3 bytes).

#### `std::vector< uint8_t > generateImg2Img(const std::string &prompt, const Img2ImgConfig &cfg, int &out_w, int &out_h, uint64_t &out_seed) override`
- Source: `include/stable_diffusion/sd_generator.h`:203
- Brief: Img2Img: generate raw RGB pixel data conditioned on an input image.
- Parameters:
  - `prompt` (const std::string &): Sanitized text prompt.
  - `cfg` (const Img2ImgConfig &): Img2Img config (includes input_image_rgb, strength, mask).
  - `out_width` (int &): Actual width of the generated image.
  - `out_height` (int &): Actual height of the generated image.
  - `out_seed` (uint64_t &): Seed actually used.
- Return: Raw RGB byte buffer (width * height * 3 bytes).
- Details: Default implementation ignores the input image and delegates to generate(). SDCppGenerator will override this with real denoising inference. prompt Sanitized text prompt. cfg Img2Img config (includes input_image_rgb, strength, mask). out_width Actual width of the generated image. out_height Actual height of the generated image. out_seed Seed actually used. Raw RGB byte buffer (width * height * 3 bytes).

#### `std::string getModelId() const override`
- Source: `include/stable_diffusion/sd_generator.h`:243
- Brief: n/a
- Parameters: none

#### `bool img2imgCalled() const`
- Source: `include/stable_diffusion/sd_generator.h`:230
- Brief: n/a
- Parameters: none

#### `bool initialize(const SDConfig &cfg) override`
- Source: `include/stable_diffusion/sd_generator.h`:175
- Brief: n/a
- Parameters:
  - `cfg` (const SDConfig &): n/a

#### `bool isInitialized() const override`
- Source: `include/stable_diffusion/sd_generator.h`:180
- Brief: n/a
- Parameters: none

#### `const std::string & lastAppliedLoraPath() const`
- Source: `include/stable_diffusion/sd_generator.h`:240
- Brief: n/a
- Parameters: none

#### `float lastAppliedLoraScale() const`
- Source: `include/stable_diffusion/sd_generator.h`:241
- Brief: n/a
- Parameters: none

#### `int lastControlHeight() const`
- Source: `include/stable_diffusion/sd_generator.h`:235
- Brief: n/a
- Parameters: none

#### `const std::string & lastControlModelPath() const`
- Source: `include/stable_diffusion/sd_generator.h`:237
- Brief: n/a
- Parameters: none

#### `float lastControlStrength() const`
- Source: `include/stable_diffusion/sd_generator.h`:236
- Brief: n/a
- Parameters: none

#### `int lastControlWidth() const`
- Source: `include/stable_diffusion/sd_generator.h`:234
- Brief: n/a
- Parameters: none

#### `int lastImg2ImgInputHeight() const`
- Source: `include/stable_diffusion/sd_generator.h`:233
- Brief: n/a
- Parameters: none

#### `int lastImg2ImgInputWidth() const`
- Source: `include/stable_diffusion/sd_generator.h`:232
- Brief: n/a
- Parameters: none

#### `float lastImg2ImgStrength() const`
- Source: `include/stable_diffusion/sd_generator.h`:231
- Brief: n/a
- Parameters: none

#### `const std::string & lastLoraAdapterPath() const`
- Source: `include/stable_diffusion/sd_generator.h`:238
- Brief: n/a
- Parameters: none

#### `float lastLoraScale() const`
- Source: `include/stable_diffusion/sd_generator.h`:239
- Brief: n/a
- Parameters: none

#### `void setApplyLoraResult(bool succeed, std::string error_message={})`
- Source: `include/stable_diffusion/sd_generator.h`:182
- Brief: n/a
- Parameters:
  - `succeed` (bool): n/a
  - `error_message` (std::string): n/a

#### `void setNextPixels(std::vector< uint8_t > px, int w, int h, uint64_t seed=42)`
- Source: `include/stable_diffusion/sd_generator.h`:167
- Brief: n/a
- Parameters:
  - `px` (std::vector< uint8_t >): n/a
  - `w` (int): n/a
  - `h` (int): n/a
  - `seed` (uint64_t): n/a

### themis::imggen::SDConfig

#### `SDConfig fromJson(const json &j)`
- Source: `include/stable_diffusion/sd_config.h`:37
- Brief: From Json.
- Parameters:
  - `j` (const json &): Input parameter.
- Return: Return value.
- Details: j Input parameter. Return value. Calls: contains().

#### `json toJson() const`
- Source: `include/stable_diffusion/sd_config.h`:38
- Brief: n/a
- Parameters: none

### themis::imggen::SDPlugin

#### `SDPlugin()`
- Source: `include/stable_diffusion/sd_plugin.h`:44
- Brief: n/a
- Parameters: none
- Details: Default constructor – uses SDStubGenerator (no model required).

#### `SDPlugin(std::unique_ptr< ISDGenerator > generator, SDPromptSanitizer sanitizer)`
- Source: `include/stable_diffusion/sd_plugin.h`:53
- Brief: Construct the plugin with an injected generator and baseline sanitizer.
- Parameters:
  - `generator` (std::unique_ptr< ISDGenerator >): n/a
  - `sanitizer` (SDPromptSanitizer): n/a
- Details: The provided sanitizer remains the fallback policy across repeated initialize() calls unless a non-empty blocked_keywords_file is supplied in the runtime config.

#### `std::optional< std::string > computePerceptualHash(const std::vector< uint8_t > &rgb, int width, int height) noexcept`
- Source: `include/stable_diffusion/sd_plugin.h`:131
- Brief: n/a
- Parameters:
  - `rgb` (const std::vector< uint8_t > &): n/a
  - `width` (int): n/a
  - `height` (int): n/a

#### `std::vector< uint8_t > encodeMinimalPng(const std::vector< uint8_t > &rgb, int width, int height)`
- Source: `include/stable_diffusion/sd_plugin.h`:133
- Brief: ── encodeMinimalPng ──────────────────────────────────────────────────────────
- Parameters:
  - `rgb` (const std::vector< uint8_t > &): Input parameter.
  - `width` (int): Input parameter.
  - `height` (int): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: rgb Input parameter. width Input parameter. height Input parameter. Return value. std::runtime_error if an error occurs. Calls: validateGenerationDimensions(), push_back(), put_be32(), size(), insert(), end(), crc32_of(), data().

#### `GeneratedImage generate(const std::string &prompt, const SDGenerationConfig &cfg) override`
- Source: `include/stable_diffusion/sd_plugin.h`:75
- Brief: ── generate ──────────────────────────────────────────────────────────────────
- Parameters:
  - `prompt` (const std::string &): Input parameter.
  - `cfg` (const SDGenerationConfig &): Input parameter.
- Return: Return value.
- Details: prompt Input parameter. cfg Input parameter. Return value. Calls: lock(), generateLocked().

#### `std::vector< GeneratedImage > generateBatch(const std::vector< std::string > &prompts, const SDGenerationConfig &cfg) override`
- Source: `include/stable_diffusion/sd_plugin.h`:84
- Brief: Generate multiple images from a list of prompts (batch mode).
- Parameters:
  - `prompts` (const std::vector< std::string > &): Input parameter.
  - `cfg` (const SDGenerationConfig &): Input parameter.
- Return: Return value.
- Details: ── generateBatch ───────────────────────────────────────────────────────────── Each prompt is screened independently by SDPromptSanitizer. Results are returned in the same order as the input prompts. prompts Input parameter. cfg Input parameter. Return value. Calls: lock(), reserve(), size(), push_back(), generateLocked().

#### `GeneratedImage generateImg2Img(const std::string &prompt, const Img2ImgConfig &cfg) override`
- Source: `include/stable_diffusion/sd_plugin.h`:98
- Brief: Modify an existing image guided by a text prompt (img2img).
- Parameters:
  - `prompt` (const std::string &): Input parameter.
  - `cfg` (const Img2ImgConfig &): Input parameter.
- Return: Return value.
- Details: ── generateImg2Img ─────────────────────────────────────────────────────────── In v2.1.0 the stub/in-memory generators ignore the input image and produce a normal generate() result. SDCppGenerator will override this with real denoising in a future release. Both the positive prompt and the negative_prompt in cfg are screened by SDPromptSanitizer before reaching the generator. prompt Input parameter. cfg Input parameter. Return value. Calls: lock(), generateImg2ImgLocked().

#### `GeneratedImage generateImg2ImgLocked(const std::string &prompt, const Img2ImgConfig &cfg)`
- Source: `include/stable_diffusion/sd_plugin.h`:121
- Brief: Generate Img2 Img Locked.
- Parameters:
  - `prompt` (const std::string &): Input parameter.
  - `cfg` (const Img2ImgConfig &): Input parameter.
- Return: Return value.
- Details: prompt Input parameter. cfg Input parameter. Return value. Calls: getPluginVersion(), isPromptAllowed(), sha256Hex(), empty(), sanitize(), validateRgbBufferShape(), validateGenerationDimensions(), std::isfinite().

#### `GeneratedImage generateLocked(const std::string &prompt, const SDGenerationConfig &cfg)`
- Source: `include/stable_diffusion/sd_plugin.h`:119
- Brief: ── generateLocked (internal, called with generate_mutex_ held) ───────────────
- Parameters:
  - `prompt` (const std::string &): Input parameter.
  - `cfg` (const SDGenerationConfig &): Input parameter.
- Return: Return value.
- Details: prompt Input parameter. cfg Input parameter. Return value. Calls: getPluginVersion(), isPromptAllowed(), sha256Hex(), empty(), sanitize(), validateGenerationDimensions(), validateRgbBufferShape(), std::isfinite().

#### `std::string getModelId() const override`
- Source: `include/stable_diffusion/sd_plugin.h`:103
- Brief: n/a
- Parameters: none

#### `std::string getPluginVersion() const override`
- Source: `include/stable_diffusion/sd_plugin.h`:104
- Brief: n/a
- Parameters: none

#### `nlohmann::json getStatistics() const override`
- Source: `include/stable_diffusion/sd_plugin.h`:105
- Brief: n/a
- Parameters: none

#### `bool initialize(const std::string &model_path, const nlohmann::json &config) override`
- Source: `include/stable_diffusion/sd_plugin.h`:70
- Brief: Initialize the plugin for stub or real-backend generation.
- Parameters:
  - `model_path` (const std::string &): Path to the model.
  - `config` (const nlohmann::json &): Input parameter.
- Return: true when the active generator initializes successfully; false on integrity-check failure or generator init failure.
- Details: ── initialize ──────────────────────────────────────────────────────────────── model_path Optional backend model path. Empty paths keep the plugin in stub mode when the active generator supports it. config Runtime configuration. When blocked_keywords_file is present and non-empty, it overrides the constructor-provided sanitizer for this initialization cycle; otherwise the baseline sanitizer is restored. true when the active generator initializes successfully; false on integrity-check failure or generator init failure. model_path Path to the model. config Input parameter. True when the operation succeeds. Calls: SDConfig::fromJson(), empty(), SDPromptSanitizer::fromFile(), defined(), normalizeLowerHex(), themis::utils::calculateSHA256(), else().

#### `bool isInitialized() const override`
- Source: `include/stable_diffusion/sd_plugin.h`:73
- Brief: n/a
- Parameters: none

#### `bool isPromptAllowed(const std::string &prompt) const override`
- Source: `include/stable_diffusion/sd_plugin.h`:101
- Brief: n/a
- Parameters:
  - `prompt` (const std::string &): n/a

#### `std::string normalizeLowerHex(const std::string &hex)`
- Source: `include/stable_diffusion/sd_plugin.h`:128
- Brief: Normalize Lower Hex.
- Parameters:
  - `hex` (const std::string &): Input parameter.
- Return: Return value.
- Details: hex Input parameter. Return value. Calls: reserve(), size(), std::isspace(), push_back(), std::tolower().

#### `std::string sha256Hex(const std::string &input)`
- Source: `include/stable_diffusion/sd_plugin.h`:130
- Brief: ── sha256Hex (FNV-based hex fingerprint; name kept for API compatibility) ─────
- Parameters:
  - `input` (const std::string &): Input parameter.
- Return: Return value.
- Details: input Input parameter. Return value. Calls: std::setfill(), std::setw(), str().

#### `bool validateGenerationDimensions(int width, int height, std::string &error_out)`
- Source: `include/stable_diffusion/sd_plugin.h`:123
- Brief: Validate Generation Dimensions.
- Parameters:
  - `width` (int): Input parameter.
  - `height` (int): Input parameter.
  - `error_out` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: width Input parameter. height Input parameter. error_out Input/output parameter. True when the operation succeeds. Calls: max(), clear().

#### `bool validateRgbBufferShape(const std::vector< uint8_t > &rgb, int width, int height, std::string &error_out)`
- Source: `include/stable_diffusion/sd_plugin.h`:124
- Brief: Validate Rgb Buffer Shape.
- Parameters:
  - `rgb` (const std::vector< uint8_t > &): Input parameter.
  - `width` (int): Input parameter.
  - `height` (int): Input parameter.
  - `error_out` (std::string &): Input/output parameter.
- Return: True when the operation succeeds.
- Details: rgb Input parameter. width Input parameter. height Input parameter. error_out Input/output parameter. True when the operation succeeds. Calls: validateGenerationDimensions(), size(), clear().

#### `~SDPlugin() override=default`
- Source: `include/stable_diffusion/sd_plugin.h`:55
- Brief: n/a
- Parameters: none

### themis::imggen::SDPluginAdapter

#### `SDPluginAdapter(std::unique_ptr< SDPlugin > plugin)`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:56
- Brief: Construct from an existing SDPlugin instance.
- Parameters:
  - `plugin` (std::unique_ptr< SDPlugin >): Heap-allocated SDPlugin; adapter takes ownership.
- Details: plugin Heap-allocated SDPlugin; adapter takes ownership.

#### `plugins::PluginCapabilities getCapabilities() const override`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:66
- Brief: n/a
- Parameters: none

#### `void * getInstance() override`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:89
- Brief: Return a pointer to the underlying SDPlugin.
- Parameters: none
- Details: Callers should cast the return value to imggen::SDPlugin*.

#### `const char * getName() const override`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:59
- Brief: n/a
- Parameters: none

#### `SDPlugin * getSDPlugin()`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:94
- Brief: n/a
- Parameters: none
- Return: Non-owning pointer to the wrapped SDPlugin.
- Details: Non-owning pointer to the wrapped SDPlugin.

#### `const SDPlugin * getSDPlugin() const`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:95
- Brief: n/a
- Parameters: none

#### `plugins::PluginType getType() const override`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:62
- Brief: n/a
- Parameters: none

#### `const char * getVersion() const override`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:60
- Brief: n/a
- Parameters: none

#### `bool initialize(const char *config_json) override`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:77
- Brief: Initialize the underlying SDPlugin.
- Parameters:
  - `config_json` (const char *): Input parameter.
- Return: true on success; false when config is missing a non-empty model_path.
- Details: Initialize. Parses config_json and calls SDPlugin::initialize(). Expected JSON keys: "model_path" (string, optional). config_json JSON configuration string. true on success; false when config is missing a non-empty model_path. config_json Input parameter. True when the operation succeeds. Calls: nlohmann::json::parse(), contains(), is_string(), empty().

#### `void shutdown() override`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:82
- Brief: Shutdown: reset the inner plugin to its default stub state.
- Parameters: none
- Details: Shutdown. Calls: clear().

### themis::imggen::SDPluginRegistrar

#### `SDPluginRegistrar()=delete`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:201
- Brief: n/a
- Parameters: none

#### `std::unique_ptr< SDPluginAdapter > createAdapter(const json &config={})`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:154
- Brief: Create an SDPluginAdapter wrapping a new SDPlugin.
- Parameters:
  - `config` (const json &): Input parameter.
- Return: Heap-allocated SDPluginAdapter; caller owns.
- Details: Create Adapter. The adapter implements IThemisPlugin and can be handed directly to plugins::PluginManager. config Optional JSON configuration. Heap-allocated SDPluginAdapter; caller owns. config Input parameter. Return value. Calls: createPlugin(), std::move().

#### `std::unique_ptr< SDPlugin > createPlugin(const json &config={})`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:143
- Brief: Create a standalone SDPlugin instance.
- Parameters:
  - `config` (const json &): Input parameter.
- Return: Heap-allocated SDPlugin; caller owns.
- Details: ── SDPluginRegistrar — factory methods ────────────────────────────────────── config Optional JSON configuration. Key: "model_path" (string) — if present, calls SDPlugin::initialize(model_path, config). Heap-allocated SDPlugin; caller owns. config Input parameter. Return value. Calls: contains(), is_string(), empty(), initialize().

#### `ReloadCallback defaultReloadCallback()`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:177
- Brief: Default hot-plug reload callback.
- Parameters: none
- Return: Return value.
- Details: ── SDPluginRegistrar — hot-plug ────────────────────────────────────────────── Calls SDPlugin::initialize(config["model_path"], config) when "model_path" is present and non-empty. Missing or empty paths are treated as a successful stub-mode no-op so hot-plug reload can keep the plugin unloaded without failing the caller. Return value. Calls: contains(), is_string(), empty(), initialize().

#### `void disableHotPlug(plugins::PluginManager &manager)`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:198
- Brief: Disable PluginManager hot-plug monitoring.
- Parameters:
  - `manager` (plugins::PluginManager &): Input/output parameter.
- Details: Disable Hot Plug. manager PluginManager instance. manager Input/output parameter. Implements disableHotPlug without additional internal calls.

#### `bool enableHotPlug(plugins::PluginManager &manager, const std::string &directory)`
- Source: `include/stable_diffusion/sd_plugin_registrar.h`:189
- Brief: Enable PluginManager hot-plug monitoring for a directory.
- Parameters:
  - `manager` (plugins::PluginManager &): Input/output parameter.
  - `directory` (const std::string &): Input parameter.
- Return: true on success.
- Details: Enable Hot Plug. Calls manager.enableHotPlug(directory, config) with default options (auto_load=true, auto_reload=true, auto_unload=true). manager PluginManager instance. directory Directory to watch. true on success. manager Input/output parameter. directory Input parameter. True when the operation succeeds. Implements enableHotPlug without additional internal calls.

### themis::imggen::SDPromptSanitizer

#### `SDPromptSanitizer(const std::vector< std::string > &blocked_keywords={})`
- Source: `include/stable_diffusion/sd_prompt_sanitizer.h`:32
- Brief: n/a
- Parameters:
  - `blocked_keywords` (const std::vector< std::string > &): n/a

#### `size_t blockedCount() const`
- Source: `include/stable_diffusion/sd_prompt_sanitizer.h`:49
- Brief: n/a
- Parameters: none

#### `SDPromptSanitizer fromFile(const std::string &path)`
- Source: `include/stable_diffusion/sd_prompt_sanitizer.h`:38
- Brief: Load blocked keywords from a plain-text file (one keyword per line). Lines starting with '#' are treated as comments.
- Parameters:
  - `path` (const std::string &): Input parameter.
- Return: Return value.
- Throws:
  - std::runtime_error: if an error occurs.
- Details: From File. path Input parameter. Return value. std::runtime_error if an error occurs. Calls: f(), is_open(), std::getline(), find_first_not_of(), substr(), empty(), push_back(), SDPromptSanitizer().

#### `bool isAllowed(const std::string &prompt) const`
- Source: `include/stable_diffusion/sd_prompt_sanitizer.h`:41
- Brief: n/a
- Parameters:
  - `prompt` (const std::string &): n/a
- Return: false if the prompt contains any blocked keyword.
- Details: false if the prompt contains any blocked keyword.

#### `std::string sanitize(const std::string &prompt) const`
- Source: `include/stable_diffusion/sd_prompt_sanitizer.h`:47
- Brief: Remove all blocked keywords from the prompt.
- Parameters:
  - `prompt` (const std::string &): n/a
- Return: Sanitized prompt string.
- Details: Sanitized prompt string.

#### `std::string toLower(const std::string &s)`
- Source: `include/stable_diffusion/sd_prompt_sanitizer.h`:54
- Brief: ── helpers ───────────────────────────────────────────────────────────────────
- Parameters:
  - `s` (const std::string &): Input parameter.
- Return: Return value.
- Details: s Input parameter. Return value. Calls: std::transform(), begin(), end(), std::tolower().

### themis::imggen::SDStubGenerator

#### `std::vector< uint8_t > generate(const std::string &, const SDGenerationConfig &cfg, int &out_w, int &out_h, uint64_t &out_seed) override`
- Source: `include/stable_diffusion/sd_generator.h`:119
- Brief: Generate raw RGB pixel data for a given prompt.
- Parameters:
  - `prompt` (const std::string &): Sanitized text prompt.
  - `cfg` (const SDGenerationConfig &): Per-request generation config.
  - `out_width` (int &): Actual width of the generated image.
  - `out_height` (int &): Actual height of the generated image.
  - `out_seed` (uint64_t &): Seed that was actually used (important for -1 random seeds).
- Return: Raw RGB byte buffer (width * height * 3 bytes).
- Details: prompt Sanitized text prompt. cfg Per-request generation config. out_width Actual width of the generated image. out_height Actual height of the generated image. out_seed Seed that was actually used (important for -1 random seeds). Raw RGB byte buffer (width * height * 3 bytes).

#### `std::vector< uint8_t > generateImg2Img(const std::string &prompt, const Img2ImgConfig &cfg, int &out_w, int &out_h, uint64_t &out_seed) override`
- Source: `include/stable_diffusion/sd_generator.h`:130
- Brief: Img2Img: generate raw RGB pixel data conditioned on an input image.
- Parameters:
  - `prompt` (const std::string &): Sanitized text prompt.
  - `cfg` (const Img2ImgConfig &): Img2Img config (includes input_image_rgb, strength, mask).
  - `out_width` (int &): Actual width of the generated image.
  - `out_height` (int &): Actual height of the generated image.
  - `out_seed` (uint64_t &): Seed actually used.
- Return: Raw RGB byte buffer (width * height * 3 bytes).
- Details: Default implementation ignores the input image and delegates to generate(). SDCppGenerator will override this with real denoising inference. prompt Sanitized text prompt. cfg Img2Img config (includes input_image_rgb, strength, mask). out_width Actual width of the generated image. out_height Actual height of the generated image. out_seed Seed actually used. Raw RGB byte buffer (width * height * 3 bytes).

#### `std::string getModelId() const override`
- Source: `include/stable_diffusion/sd_generator.h`:147
- Brief: n/a
- Parameters: none

#### `bool initialize(const SDConfig &cfg) override`
- Source: `include/stable_diffusion/sd_generator.h`:112
- Brief: n/a
- Parameters:
  - `cfg` (const SDConfig &): n/a

#### `bool isInitialized() const override`
- Source: `include/stable_diffusion/sd_generator.h`:117
- Brief: n/a
- Parameters: none

### themis::test::wave_d

#### `TEST(WaveD_StableDiffusionStress, AccelerationFailClosedStress)`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_StableDiffusionStress): n/a
  - `<unnamed>` (AccelerationFailClosedStress): n/a

#### `TEST(WaveD_StableDiffusionStress, ConcurrentModelLoadStress)`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:161
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_StableDiffusionStress): n/a
  - `<unnamed>` (ConcurrentModelLoadStress): n/a

#### `TEST(WaveD_StableDiffusionStress, HighCardinalityInferenceBatch)`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:130
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_StableDiffusionStress): n/a
  - `<unnamed>` (HighCardinalityInferenceBatch): n/a

### themis::test::wave_d::StubAccelerationGuard

#### `StubAccelerationGuard(bool available)`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:103
- Brief: n/a
- Parameters:
  - `available` (bool): n/a

#### `long accelerated() const`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:117
- Brief: n/a
- Parameters: none

#### `long attempts() const`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:116
- Brief: n/a
- Parameters: none

#### `long fallbacks() const`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:118
- Brief: n/a
- Parameters: none

#### `bool handleRequest(uint64_t)`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:106
- Brief: Returns true if the request was handled (accelerated OR CPU fallback).
- Parameters:
  - `<unnamed>` (uint64_t): n/a

### themis::test::wave_d::StubInferenceStore

#### `bool fetch(const std::string &prompt, std::string *out) const`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:51
- Brief: n/a
- Parameters:
  - `prompt` (const std::string &): n/a
  - `out` (std::string *): n/a

#### `std::size_t size() const`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:59
- Brief: n/a
- Parameters: none

#### `bool submit(const std::string &prompt, const std::string &result)`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:44
- Brief: n/a
- Parameters:
  - `prompt` (const std::string &): n/a
  - `result` (const std::string &): n/a

#### `long writes() const`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:64
- Brief: n/a
- Parameters: none

### themis::test::wave_d::StubModelRegistry

#### `bool isLoaded(const std::string &model_id) const`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:87
- Brief: n/a
- Parameters:
  - `model_id` (const std::string &): n/a

#### `bool load(const std::string &model_id)`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:73
- Brief: n/a
- Parameters:
  - `model_id` (const std::string &): n/a

#### `long loads() const`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:92
- Brief: n/a
- Parameters: none

#### `bool unload(const std::string &model_id)`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:80
- Brief: n/a
- Parameters:
  - `model_id` (const std::string &): n/a

#### `long unloads() const`
- Source: `tests/stable_diffusion/test_stable_diffusion_highcardinality_stress.cpp`:93
- Brief: n/a
- Parameters: none

