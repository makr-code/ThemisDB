# AI DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\ai\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\ai\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 9
- Compounds: 45
- Classes/Structs: 17
- Namespaces: 11
- File Compounds: 9

## Namespaces
- @065030125130373254021066243216116030132101054042
- @316220140231134345342224221311367223142225367025
- std::chrono_literals
- testing
- themis
- themis::ai
- themis::ai::@225312325316033107054115365261302142323377361112
- themis::llm
- themis::plugins
- themis::plugins::ai
- themis::plugins::ai::@375377150326150001172234261301026133016020064223

## Types
### Classes
- AIDecisionAuditorTest
- AIPluginGenerator
- StubAIGenerationPipeline
- themis::ai::CAIEthicsIntegration
- themis::plugins::ai::AIPluginGenerator

### Structs
- AIPluginGenerator::Config
- AIPluginGenerator::Stats
- GeneratedPlugin
- GenerationResult
- PluginGenerationPrompt
- StubAIGenerationPipeline::Stats
- themis::ai::CAIEthicsConfig
- themis::ai::CAIEvaluationResult
- themis::plugins::ai::AIPluginGenerator::Config
- themis::plugins::ai::AIPluginGenerator::Stats
- themis::plugins::ai::GeneratedPlugin
- themis::plugins::ai::PluginGenerationPrompt

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 95

### AIDecisionAuditorTest

#### `void SetUp() override`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:24
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:47
- Brief: n/a
- Parameters: none

#### `AIDecisionAudit createTestAudit()`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:53
- Brief: n/a
- Parameters: none

### AIPluginGenerator

#### `AIPluginGenerator(const Config &config)`
- Source: `include/ai/ai_plugin_generator.h`:130
- Brief: AIPlugin Generator.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `void clearHttpPostFn()`
- Source: `include/ai/ai_plugin_generator.h`:172
- Brief: Clear Http Post Fn.
- Parameters: none

#### `Result< GeneratedPlugin > generatePlugin(const PluginGenerationPrompt &prompt)`
- Source: `include/ai/ai_plugin_generator.h`:149
- Brief: Generate Plugin.
- Parameters:
  - `prompt` (const PluginGenerationPrompt &): Input parameter.
- Return: Return value.
- Details: prompt Input parameter. Return value.

#### `Stats getStats() const`
- Source: `include/ai/ai_plugin_generator.h`:115
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void setHttpPostFn(HttpPostFn fn)`
- Source: `include/ai/ai_plugin_generator.h`:167
- Brief: Set Http Post Fn.
- Parameters:
  - `fn` (HttpPostFn): Input parameter.
- Details: fn Input parameter.

#### `void setLLMGenerateFn(LLMGenerateFn fn)`
- Source: `include/ai/ai_plugin_generator.h`:123
- Brief: Set LLMGenerate Fn.
- Parameters:
  - `fn` (LLMGenerateFn): Input parameter.
- Details: fn Input parameter.

#### `void setLlmHttpPostFn(LlmHttpPostFn fn)`
- Source: `include/ai/ai_plugin_generator.h`:142
- Brief: Set Llm Http Post Fn.
- Parameters:
  - `fn` (LlmHttpPostFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: std::move().

#### `Result< void > validatePrompt(const PluginGenerationPrompt &prompt)`
- Source: `include/ai/ai_plugin_generator.h`:155
- Brief: Validate Prompt.
- Parameters:
  - `prompt` (const PluginGenerationPrompt &): Input parameter.
- Return: Return value.
- Details: prompt Input parameter. Return value.

#### `~AIPluginGenerator()`
- Source: `include/ai/ai_plugin_generator.h`:131
- Brief: n/a
- Parameters: none

### StubAIGenerationPipeline

#### `StubAIGenerationPipeline(StubMode mode=StubMode::AlwaysSucceed, std::chrono::microseconds simulated_latency=500us)`
- Source: `tests/ai/test_ai_generation_soak_60min.cpp`:119
- Brief: n/a
- Parameters:
  - `mode` (StubMode): n/a
  - `simulated_latency` (std::chrono::microseconds): n/a

#### `GenerationResult call(std::size_t call_index)`
- Source: `tests/ai/test_ai_generation_soak_60min.cpp`:128
- Brief: Execute one simulated generation call.
- Parameters:
  - `call_index` (std::size_t): Sequential call index (used for MixedValidInvalid alternation).
- Return: GenerationResult with outcome and simulated latency.
- Details: call_index Sequential call index (used for MixedValidInvalid alternation). GenerationResult with outcome and simulated latency.

#### `Stats getStats() const`
- Source: `tests/ai/test_ai_generation_soak_60min.cpp`:180
- Brief: Return a snapshot of accumulated Stats.
- Parameters: none

#### `void setMode(StubMode mode)`
- Source: `tests/ai/test_ai_generation_soak_60min.cpp`:174
- Brief: Switch stub mode (thread-safe via atomic on the underlying integral type).
- Parameters:
  - `mode` (StubMode): n/a

### test_ai_decision_auditor.cpp

#### `TEST_F(AIDecisionAuditorTest, ExportForCompliance)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:286
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (ExportForCompliance): n/a

#### `TEST_F(AIDecisionAuditorTest, FlagForReview)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:135
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (FlagForReview): n/a

#### `TEST_F(AIDecisionAuditorTest, GenerateExplanation)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (GenerateExplanation): n/a

#### `TEST_F(AIDecisionAuditorTest, GetStatistics)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:260
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (GetStatistics): n/a

#### `TEST_F(AIDecisionAuditorTest, HighConfidenceNotFlagged)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:125
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (HighConfidenceNotFlagged): n/a

#### `TEST_F(AIDecisionAuditorTest, LogDecisionWithCompleteContext)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (LogDecisionWithCompleteContext): n/a

#### `TEST_F(AIDecisionAuditorTest, LowConfidenceAutoFlagsForReview)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (LowConfidenceAutoFlagsForReview): n/a

#### `TEST_F(AIDecisionAuditorTest, QueryByConfidenceRange)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:212
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (QueryByConfidenceRange): n/a

#### `TEST_F(AIDecisionAuditorTest, QueryByReviewFlag)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:238
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (QueryByReviewFlag): n/a

#### `TEST_F(AIDecisionAuditorTest, QueryByUserFilter)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:186
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (QueryByUserFilter): n/a

#### `TEST_F(AIDecisionAuditorTest, RecordHumanOverride)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (RecordHumanOverride): n/a

#### `TEST_F(AIDecisionAuditorTest, RetrieveDecisionById)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:101
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (RetrieveDecisionById): n/a

#### `TEST_F(AIDecisionAuditorTest, SerializationRoundTrip)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:323
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (SerializationRoundTrip): n/a

#### `TEST_F(AIDecisionAuditorTest, VerifyIntegrity)`
- Source: `tests/ai/test_ai_decision_auditor.cpp`:314
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIDecisionAuditorTest): n/a
  - `<unnamed>` (VerifyIntegrity): n/a

### test_ai_generation_soak_60min.cpp

#### `TEST(WaveD_AI_GenerationSoak, EndpointStress)`
- Source: `tests/ai/test_ai_generation_soak_60min.cpp`:213
- Brief: Soak test: sustained endpoint load, all calls succeed.
- Parameters:
  - `<unnamed>` (WaveD_AI_GenerationSoak): n/a
  - `<unnamed>` (EndpointStress): n/a
- Details: Models ~10 req/s over the soak duration. Verifies: Success rate remains 100% for always-succeed stub Stats counters remain internally consistent No exception or undefined behavior under sustained load

#### `TEST(WaveD_AI_GenerationSoak, RetryBudgetExhaustionAndRecovery)`
- Source: `tests/ai/test_ai_generation_soak_60min.cpp`:263
- Brief: Soak test: 100% transport failure window, then recovery.
- Parameters:
  - `<unnamed>` (WaveD_AI_GenerationSoak): n/a
  - `<unnamed>` (RetryBudgetExhaustionAndRecovery): n/a
- Details: Models the "retry storm then recovery" scenario described in docs/operability/RUNBOOK_AI_GENERATION.md Type 3. Verifies: transport_errors grow during failure window Successes resume ≤ 1 s after endpoint recovery Stats invariant holds throughout

#### `TEST(WaveD_AI_GenerationSoak, StatConsistencyInvariant)`
- Source: `tests/ai/test_ai_generation_soak_60min.cpp`:364
- Brief: Soak test: Stats invariant holds across all call patterns.
- Parameters:
  - `<unnamed>` (WaveD_AI_GenerationSoak): n/a
  - `<unnamed>` (StatConsistencyInvariant): n/a
- Details: Runs a rapid burst of calls across multiple modes and verifies that the Stats invariant (total_calls == successes + all_errors) holds at the end regardless of call outcome distribution.

#### `TEST(WaveD_AI_GenerationSoak, ValidationRateStability)`
- Source: `tests/ai/test_ai_generation_soak_60min.cpp`:319
- Brief: Soak test: mixed valid/invalid prompts over soak duration.
- Parameters:
  - `<unnamed>` (WaveD_AI_GenerationSoak): n/a
  - `<unnamed>` (ValidationRateStability): n/a
- Details: Verifies: validation_errors equals the number of invalid prompts exactly successes equals the number of valid prompts exactly No cross-contamination between valid and invalid paths Stats invariant holds

#### `uint64_t soakDurationMs()`
- Source: `tests/ai/test_ai_generation_soak_60min.cpp`:68
- Brief: n/a
- Parameters: none

### test_ai_highcardinality_stress.cpp

#### `TEST(WaveD_AIFrameworkStress, ConcurrentKGReasonerStress)`
- Source: `tests/ai/test_ai_highcardinality_stress.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_AIFrameworkStress): n/a
  - `<unnamed>` (ConcurrentKGReasonerStress): n/a

#### `TEST(WaveD_AIFrameworkStress, HighCardinalityPluginDispatch)`
- Source: `tests/ai/test_ai_highcardinality_stress.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_AIFrameworkStress): n/a
  - `<unnamed>` (HighCardinalityPluginDispatch): n/a

#### `TEST(WaveD_AIFrameworkStress, InferenceBatchStress)`
- Source: `tests/ai/test_ai_highcardinality_stress.cpp`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (WaveD_AIFrameworkStress): n/a
  - `<unnamed>` (InferenceBatchStress): n/a

### test_ai_plugin_generator.cpp

#### `TEST(AIPluginGeneratorTest, APG01_ConstructionDoesNotThrow)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:128
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG01_ConstructionDoesNotThrow): n/a

#### `TEST(AIPluginGeneratorTest, APG02_ValidatePromptEmptyDescriptionFails)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:136
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG02_ValidatePromptEmptyDescriptionFails): n/a

#### `TEST(AIPluginGeneratorTest, APG03_ValidatePromptValidDescriptionSucceeds)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:146
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG03_ValidatePromptValidDescriptionSucceeds): n/a

#### `TEST(AIPluginGeneratorTest, APG04_ValidatePromptOversizedDescriptionFails)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:154
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG04_ValidatePromptOversizedDescriptionFails): n/a

#### `TEST(AIPluginGeneratorTest, APG05_GeneratePluginPropagatesValidationError)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:163
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG05_GeneratePluginPropagatesValidationError): n/a

#### `TEST(AIPluginGeneratorTest, APG06_GeneratePluginReturnsGeneratedPlugin)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:172
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG06_GeneratePluginReturnsGeneratedPlugin): n/a

#### `TEST(AIPluginGeneratorTest, APG07_GeneratePluginParsesEndpointResponse)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG07_GeneratePluginParsesEndpointResponse): n/a

#### `TEST(AIPluginGeneratorTest, APG08_GeneratePluginRejectsMissingImplementationCode)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:211
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG08_GeneratePluginRejectsMissingImplementationCode): n/a

#### `TEST(AIPluginGeneratorTest, APG09_C1SafetyGateAcceptsWhenThresholdMet)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:227
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG09_C1SafetyGateAcceptsWhenThresholdMet): n/a

#### `TEST(AIPluginGeneratorTest, APG10_C1SafetyGateRejectsWhenThresholdMisses)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG10_C1SafetyGateRejectsWhenThresholdMisses): n/a

#### `TEST(AIPluginGeneratorTest, APG11_C2FederatedTelemetryReceivesMetrics)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:257
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG11_C2FederatedTelemetryReceivesMetrics): n/a

#### `TEST(AIPluginGeneratorTest, APG12_C1SafetyGateMissingCallbackFailsClosed)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:279
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG12_C1SafetyGateMissingCallbackFailsClosed): n/a

#### `TEST(AIPluginGeneratorTest, APG13_C1SafetyGateRejectsNonFiniteScore)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG13_C1SafetyGateRejectsNonFiniteScore): n/a

#### `TEST(AIPluginGeneratorTest, APG14_C2TelemetryMissingCallbackFailsClosed)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:304
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG14_C2TelemetryMissingCallbackFailsClosed): n/a

#### `TEST(AIPluginGeneratorTest, APG15_C2TelemetryFailurePropagates)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG15_C2TelemetryFailurePropagates): n/a

#### `TEST(AIPluginGeneratorTest, APG16_C2TelemetryIncludesC1SafetyScoreWhenEnabled)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:331
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG16_C2TelemetryIncludesC1SafetyScoreWhenEnabled): n/a

#### `TEST(AIPluginGeneratorTest, APG17_ValidatePromptRejectsInvalidCapabilityToken)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:354
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG17_ValidatePromptRejectsInvalidCapabilityToken): n/a

#### `TEST(AIPluginGeneratorTest, APG18_ValidatePromptRejectsDuplicateDependencies)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:365
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG18_ValidatePromptRejectsDuplicateDependencies): n/a

#### `TEST(AIPluginGeneratorTest, APG19_GeneratePluginRejectsEndpointOutsideAllowList)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:376
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG19_GeneratePluginRejectsEndpointOutsideAllowList): n/a

#### `TEST(AIPluginGeneratorTest, APG20_GeneratePluginRejectsOversizedEndpointResponse)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:391
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG20_GeneratePluginRejectsOversizedEndpointResponse): n/a

#### `TEST(AIPluginGeneratorTest, APG21_GeneratePluginRejectsOversizedSerializedRequest)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:405
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG21_GeneratePluginRejectsOversizedSerializedRequest): n/a

#### `TEST(AIPluginGeneratorTest, APG22_SandboxGateMaterializesArtifactsWithoutCallback)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:419
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG22_SandboxGateMaterializesArtifactsWithoutCallback): n/a

#### `TEST(AIPluginGeneratorTest, APG23_SandboxGateFailurePropagatesAndCountsRejection)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:466
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG23_SandboxGateFailurePropagatesAndCountsRejection): n/a

#### `TEST(AIPluginGeneratorTest, APG24_SandboxGateSuccessAppendsSecurityReport)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:487
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG24_SandboxGateSuccessAppendsSecurityReport): n/a

#### `TEST(AIPluginGeneratorTest, APG25_StatsCountersTrackOutcomes)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:509
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG25_StatsCountersTrackOutcomes): n/a

#### `TEST(AIPluginGeneratorTest, APG26_GeneratePluginRejectsOversizedCmakeCode)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:545
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG26_GeneratePluginRejectsOversizedCmakeCode): n/a

#### `TEST(AIPluginGeneratorTest, APG27_GeneratePluginRejectsOversizedSecurityReport)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:561
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG27_GeneratePluginRejectsOversizedSecurityReport): n/a

#### `TEST(AIPluginGeneratorTest, APG28_OversizedVersionDefaulted)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:577
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG28_OversizedVersionDefaulted): n/a

#### `TEST(AIPluginGeneratorTest, APG29_OversizedManifestDescriptionTruncated)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:592
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG29_OversizedManifestDescriptionTruncated): n/a

#### `TEST(AIPluginGeneratorTest, APG30_OversizedBuildDependencyEntryDropped)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:607
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG30_OversizedBuildDependencyEntryDropped): n/a

#### `TEST(AIPluginGeneratorTest, APG31_LlmHttpPostBridgeInvokedWhenConfigured)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:629
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG31_LlmHttpPostBridgeInvokedWhenConfigured): n/a

#### `TEST(AIPluginGeneratorTest, APG32_HttpStatusFailuresAreNotRetried)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:656
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APG32_HttpStatusFailuresAreNotRetried): n/a

#### `TEST(AIPluginGeneratorTest, APGINT01_DeterministicEndpointFixtureFullPath)`
- Source: `tests/ai/test_ai_plugin_generator.cpp`:675
- Brief: n/a
- Parameters:
  - `<unnamed>` (AIPluginGeneratorTest): n/a
  - `<unnamed>` (APGINT01_DeterministicEndpointFixtureFullPath): n/a

### themis::ai::CAIEthicsIntegration

#### `CAIEthicsIntegration(CAIEthicsIntegration &&) noexcept=default`
- Source: `include/ai/cai_ethics_integration.h`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (CAIEthicsIntegration &&): n/a

#### `CAIEthicsIntegration(const CAIEthicsConfig &config={})`
- Source: `include/ai/cai_ethics_integration.h`:70
- Brief: n/a
- Parameters:
  - `config` (const CAIEthicsConfig &): n/a

#### `CAIEthicsIntegration(const CAIEthicsIntegration &)=delete`
- Source: `include/ai/cai_ethics_integration.h`:75
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CAIEthicsIntegration &): n/a

#### `void addPrinciple(const llm::ConstitutionalPrinciple &principle)`
- Source: `include/ai/cai_ethics_integration.h`:95
- Brief: Add Principle.
- Parameters:
  - `principle` (const llm::ConstitutionalPrinciple &): Input parameter.
- Details: principle Input parameter.

#### `std::vector< plugins::ethics::EthicalArgument > buildArguments(const llm::ConstitutionalReasoningResult &cai_result) const`
- Source: `include/ai/cai_ethics_integration.h`:145
- Brief: Build Arguments.
- Parameters:
  - `cai_result` (const llm::ConstitutionalReasoningResult &): Input parameter.
- Return: Return value.
- Details: cai_result Input parameter. Return value.

#### `plugins::ethics::EthicalDecision buildDecision(const llm::ConstitutionalReasoningResult &cai_result, const std::string &query, const std::vector< std::string > &formalized_principles, const std::vector< std::string > &formalized_domains, const std::vector< std::string > &argument_chain_ids) const`
- Source: `include/ai/cai_ethics_integration.h`:132
- Brief: Build Decision.
- Parameters:
  - `cai_result` (const llm::ConstitutionalReasoningResult &): Input parameter.
  - `query` (const std::string &): Input parameter.
  - `formalized_principles` (const std::vector< std::string > &): Input parameter.
  - `formalized_domains` (const std::vector< std::string > &): Input parameter.
  - `argument_chain_ids` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: cai_result Input parameter. query Input parameter. formalized_principles Input parameter. formalized_domains Input parameter. argument_chain_ids Input parameter. Return value.

#### `const CAIEthicsConfig & config() const`
- Source: `include/ai/cai_ethics_integration.h`:113
- Brief: n/a
- Parameters: none

#### `CAIEvaluationResult evaluate(const std::string &response, const std::string &query, std::function< std::string(const std::string &)> llm_fn=nullptr)`
- Source: `include/ai/cai_ethics_integration.h`:84
- Brief: n/a
- Parameters:
  - `response` (const std::string &): n/a
  - `query` (const std::string &): n/a
  - `llm_fn` (std::function< std::string(const std::string &)>): n/a

#### `std::vector< llm::ConstitutionalPrinciple > getPrinciples() const`
- Source: `include/ai/cai_ethics_integration.h`:101
- Brief: Get Principles.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `CAIEthicsIntegration & operator=(CAIEthicsIntegration &&) noexcept=default`
- Source: `include/ai/cai_ethics_integration.h`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (CAIEthicsIntegration &&): n/a

#### `CAIEthicsIntegration & operator=(const CAIEthicsIntegration &)=delete`
- Source: `include/ai/cai_ethics_integration.h`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (const CAIEthicsIntegration &): n/a

#### `bool passesAcceptanceCriteria(const CAIEvaluationResult &result, double min_safety_score=0.80)`
- Source: `include/ai/cai_ethics_integration.h`:115
- Brief: Passes Acceptance Criteria.
- Parameters:
  - `result` (const CAIEvaluationResult &): Input parameter.
  - `min_safety_score` (double): Input parameter.
- Return: True when the operation succeeds.
- Details: result Input parameter. min_safety_score Input parameter. True when the operation succeeds.

#### `std::size_t principleCount() const`
- Source: `include/ai/cai_ethics_integration.h`:107
- Brief: Principle Count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `~CAIEthicsIntegration()=default`
- Source: `include/ai/cai_ethics_integration.h`:72
- Brief: n/a
- Parameters: none

### themis::ai::CAIEvaluationResult

#### `double safety_score() const`
- Source: `include/ai/cai_ethics_integration.h`:48
- Brief: n/a
- Parameters: none

### themis::plugins::ai::AIPluginGenerator

#### `AIPluginGenerator(const Config &config)`
- Source: `include/ai/ai_plugin_generator.h`:130
- Brief: AIPlugin Generator.
- Parameters:
  - `config` (const Config &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `void clearHttpPostFn()`
- Source: `include/ai/ai_plugin_generator.h`:172
- Brief: Clear Http Post Fn.
- Parameters: none

#### `Result< GeneratedPlugin > generatePlugin(const PluginGenerationPrompt &prompt)`
- Source: `include/ai/ai_plugin_generator.h`:149
- Brief: Generate Plugin.
- Parameters:
  - `prompt` (const PluginGenerationPrompt &): Input parameter.
- Return: Return value.
- Details: prompt Input parameter. Return value.

#### `Stats getStats() const`
- Source: `include/ai/ai_plugin_generator.h`:115
- Brief: Get Stats.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `void setHttpPostFn(HttpPostFn fn)`
- Source: `include/ai/ai_plugin_generator.h`:167
- Brief: Set Http Post Fn.
- Parameters:
  - `fn` (HttpPostFn): Input parameter.
- Details: fn Input parameter.

#### `void setLLMGenerateFn(LLMGenerateFn fn)`
- Source: `include/ai/ai_plugin_generator.h`:123
- Brief: Set LLMGenerate Fn.
- Parameters:
  - `fn` (LLMGenerateFn): Input parameter.
- Details: fn Input parameter.

#### `void setLlmHttpPostFn(LlmHttpPostFn fn)`
- Source: `include/ai/ai_plugin_generator.h`:142
- Brief: Set Llm Http Post Fn.
- Parameters:
  - `fn` (LlmHttpPostFn): Input parameter.
- Details: fn Input parameter. fn Input parameter. Calls: std::move().

#### `Result< void > validatePrompt(const PluginGenerationPrompt &prompt)`
- Source: `include/ai/ai_plugin_generator.h`:155
- Brief: Validate Prompt.
- Parameters:
  - `prompt` (const PluginGenerationPrompt &): Input parameter.
- Return: Return value.
- Details: prompt Input parameter. Return value.

#### `~AIPluginGenerator()`
- Source: `include/ai/ai_plugin_generator.h`:131
- Brief: n/a
- Parameters: none

