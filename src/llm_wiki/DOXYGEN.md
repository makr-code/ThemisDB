# LLM_WIKI DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\llm_wiki\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\llm_wiki\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 20
- Compounds: 62
- Classes/Structs: 21
- Namespaces: 12
- File Compounds: 20

## Namespaces
- @012216142030031076212065300174342203057014244155
- @033243363205005241247265350053112264112327153255
- @270142366144113220161013147076055043224053321102
- testing
- themis
- themis::llm_wiki
- themis::llm_wiki::@343207204233274270215210361202125047140313132347
- themis::llm_wiki::guardrail_patterns
- themis::plugins
- themis::plugins::llm_wiki
- themis::plugins::llm_wiki::@134323175143040353020153275305247335143314012041
- themis::plugins::llm_wiki::@205223110360050335272230121117015364325211274303

## Types
### Classes
- LlmWikiCoreTest
- LlmWikiEdgeCaseTest
- themis::llm_wiki::ProcessPolicyManager
- themis::llm_wiki::WikiGuardrails
- themis::llm_wiki::WorkspaceStateManager
- themis::plugins::llm_wiki::ILLMWikiPlugin

### Structs
- themis::llm_wiki::KnobBounds
- themis::llm_wiki::LLMWikiProcessPolicy
- themis::llm_wiki::ProcessPolicyStatus
- themis::llm_wiki::StagePolicy
- themis::llm_wiki::WorkspaceState
- themis::llm_wiki::WorkspaceStatus
- themis::plugins::llm_wiki::Status
- themis::plugins::llm_wiki::WikiDumpIngestOptions
- themis::plugins::llm_wiki::WikiIngestOptions
- themis::plugins::llm_wiki::WikiIngestResult
- themis::plugins::llm_wiki::WikiLintResult
- themis::plugins::llm_wiki::WikiQueryOptions
- themis::plugins::llm_wiki::WikiQueryResult
- themis::plugins::llm_wiki::WikiWorkspaceStats
- themis::plugins::llm_wiki::WikiWorkspaceStats::EvaluationStats

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 94

### LlmWikiCoreTest

#### `void SetUp() override`
- Source: `tests/llm_wiki/test_llm_wiki_core_focused.cpp`:20
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/llm_wiki/test_llm_wiki_core_focused.cpp`:26
- Brief: n/a
- Parameters: none

### LlmWikiEdgeCaseTest

#### `void SetUp() override`
- Source: `tests/llm_wiki/test_llm_wiki_edge_cases_focused.cpp`:20
- Brief: n/a
- Parameters: none

#### `void TearDown() override`
- Source: `tests/llm_wiki/test_llm_wiki_edge_cases_focused.cpp`:26
- Brief: n/a
- Parameters: none

### bench_llm_wiki_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp`:167
- Brief: n/a
- Parameters: none

#### `void BM_LW_BM_01_IndexQueryThroughput(benchmark::State &state)`
- Source: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp`:99
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LW_BM_02_ArticleSyncLatency(benchmark::State &state)`
- Source: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp`:118
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LW_BM_03_SemanticSearchLatency(benchmark::State &state)`
- Source: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp`:135
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_LW_BM_04_CacheLookupLatency(benchmark::State &state)`
- Source: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp`:153
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `MinTime(1.0) -> UseRealTime()`
- Source: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp`:128
- Brief: n/a
- Parameters:
  - `0` (1.): n/a

#### `Threads(8) -> MinTime(1.0) ->UseRealTime()`
- Source: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp`:110
- Brief: n/a
- Parameters:
  - `<unnamed>` (8): n/a

#### `WikiIndexStub & sharedIndex()`
- Source: `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp`:86
- Brief: n/a
- Parameters: none

### test_llm_wiki_core_focused.cpp

#### `TEST_F(LlmWikiCoreTest, WS1_Load_NoFile_ReturnsFileNotFound)`
- Source: `tests/llm_wiki/test_llm_wiki_core_focused.cpp`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiCoreTest): n/a
  - `<unnamed>` (WS1_Load_NoFile_ReturnsFileNotFound): n/a

#### `TEST_F(LlmWikiCoreTest, WS2_Save_CreatesStateFile)`
- Source: `tests/llm_wiki/test_llm_wiki_core_focused.cpp`:46
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiCoreTest): n/a
  - `<unnamed>` (WS2_Save_CreatesStateFile): n/a

#### `TEST_F(LlmWikiCoreTest, WS3_SaveThenLoad_Roundtrip_PreservesWorkspaceRoot)`
- Source: `tests/llm_wiki/test_llm_wiki_core_focused.cpp`:59
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiCoreTest): n/a
  - `<unnamed>` (WS3_SaveThenLoad_Roundtrip_PreservesWorkspaceRoot): n/a

#### `TEST_F(LlmWikiCoreTest, WS4_SaveThenLoad_Roundtrip_PreservesLinks)`
- Source: `tests/llm_wiki/test_llm_wiki_core_focused.cpp`:74
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiCoreTest): n/a
  - `<unnamed>` (WS4_SaveThenLoad_Roundtrip_PreservesLinks): n/a

#### `TEST_F(LlmWikiCoreTest, WS5_SaveThenLoad_Roundtrip_PreservesTasks)`
- Source: `tests/llm_wiki/test_llm_wiki_core_focused.cpp`:91
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiCoreTest): n/a
  - `<unnamed>` (WS5_SaveThenLoad_Roundtrip_PreservesTasks): n/a

#### `TEST_F(LlmWikiCoreTest, WS6_MultipleSaves_AllSucceed)`
- Source: `tests/llm_wiki/test_llm_wiki_core_focused.cpp`:107
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiCoreTest): n/a
  - `<unnamed>` (WS6_MultipleSaves_AllSucceed): n/a

#### `TEST_F(LlmWikiCoreTest, WS7_MultipleSaves_LoadReturnsLatest)`
- Source: `tests/llm_wiki/test_llm_wiki_core_focused.cpp`:118
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiCoreTest): n/a
  - `<unnamed>` (WS7_MultipleSaves_LoadReturnsLatest): n/a

### test_llm_wiki_edge_cases_focused.cpp

#### `TEST_F(LlmWikiEdgeCaseTest, WE1_LoadCorruptJson_ReturnsErrorCode)`
- Source: `tests/llm_wiki/test_llm_wiki_edge_cases_focused.cpp`:37
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiEdgeCaseTest): n/a
  - `<unnamed>` (WE1_LoadCorruptJson_ReturnsErrorCode): n/a

#### `TEST_F(LlmWikiEdgeCaseTest, WE2_LoadChecksumMismatch_ReturnsChecksumMismatch)`
- Source: `tests/llm_wiki/test_llm_wiki_edge_cases_focused.cpp`:56
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiEdgeCaseTest): n/a
  - `<unnamed>` (WE2_LoadChecksumMismatch_ReturnsChecksumMismatch): n/a

#### `TEST_F(LlmWikiEdgeCaseTest, WE3_SaveLargeLinksMap_Succeeds)`
- Source: `tests/llm_wiki/test_llm_wiki_edge_cases_focused.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiEdgeCaseTest): n/a
  - `<unnamed>` (WE3_SaveLargeLinksMap_Succeeds): n/a

#### `TEST_F(LlmWikiEdgeCaseTest, WE4_ValidateChecksum_NonExistentFile_ReturnsError)`
- Source: `tests/llm_wiki/test_llm_wiki_edge_cases_focused.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiEdgeCaseTest): n/a
  - `<unnamed>` (WE4_ValidateChecksum_NonExistentFile_ReturnsError): n/a

#### `TEST_F(LlmWikiEdgeCaseTest, WE5_WorkspaceStatus_FactoryMethods_ConsistentState)`
- Source: `tests/llm_wiki/test_llm_wiki_edge_cases_focused.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiEdgeCaseTest): n/a
  - `<unnamed>` (WE5_WorkspaceStatus_FactoryMethods_ConsistentState): n/a

#### `TEST_F(LlmWikiEdgeCaseTest, WE6_CorruptThenValidSave_RecoversByLoad)`
- Source: `tests/llm_wiki/test_llm_wiki_edge_cases_focused.cpp`:111
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiEdgeCaseTest): n/a
  - `<unnamed>` (WE6_CorruptThenValidSave_RecoversByLoad): n/a

### test_llm_wiki_highcardinality_stress.cpp

#### `TEST(LLMWikiHighCardinalityStress, ConcurrentSemanticQueryStress)`
- Source: `tests/llm_wiki/test_llm_wiki_highcardinality_stress.cpp`:116
- Brief: ConcurrentSemanticQueryStress.
- Parameters:
  - `<unnamed>` (LLMWikiHighCardinalityStress): n/a
  - `<unnamed>` (ConcurrentSemanticQueryStress): n/a
- Details: Issues 500 000 semantic search calls from 4 concurrent threads. Asserts all calls succeed.

#### `TEST(LLMWikiHighCardinalityStress, HighCardinalityArticleIndex)`
- Source: `tests/llm_wiki/test_llm_wiki_highcardinality_stress.cpp`:83
- Brief: HighCardinalityArticleIndex.
- Parameters:
  - `<unnamed>` (LLMWikiHighCardinalityStress): n/a
  - `<unnamed>` (HighCardinalityArticleIndex): n/a
- Details: Syncs 100 000 articles from 8 concurrent threads and validates all are indexed without loss.

#### `TEST(LLMWikiHighCardinalityStress, WikiSyncEdgeCaseStress)`
- Source: `tests/llm_wiki/test_llm_wiki_highcardinality_stress.cpp`:145
- Brief: WikiSyncEdgeCaseStress.
- Parameters:
  - `<unnamed>` (LLMWikiHighCardinalityStress): n/a
  - `<unnamed>` (WikiSyncEdgeCaseStress): n/a
- Details: Exercises repeated sync of the same article IDs from multiple threads to validate idempotency under concurrent writes.

### test_llm_wiki_llm_integration_focused.cpp

#### `TEST(LlmWikiLlmIntegrationFocusedTests, EG1_GetCurrentEdition_DoesNotCrash)`
- Source: `tests/llm_wiki/test_llm_wiki_llm_integration_focused.cpp`:22
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiLlmIntegrationFocusedTests): n/a
  - `<unnamed>` (EG1_GetCurrentEdition_DoesNotCrash): n/a

#### `TEST(LlmWikiLlmIntegrationFocusedTests, EG2_IsLLMWikiEnabled_DoesNotCrash)`
- Source: `tests/llm_wiki/test_llm_wiki_llm_integration_focused.cpp`:30
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiLlmIntegrationFocusedTests): n/a
  - `<unnamed>` (EG2_IsLLMWikiEnabled_DoesNotCrash): n/a

#### `TEST(LlmWikiLlmIntegrationFocusedTests, EG3_CommunityBuild_LLMWikiIsDisabled)`
- Source: `tests/llm_wiki/test_llm_wiki_llm_integration_focused.cpp`:40
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiLlmIntegrationFocusedTests): n/a
  - `<unnamed>` (EG3_CommunityBuild_LLMWikiIsDisabled): n/a

#### `TEST(LlmWikiLlmIntegrationFocusedTests, EG4_CommunityBuild_EnforcePluginGate_PermissionDenied)`
- Source: `tests/llm_wiki/test_llm_wiki_llm_integration_focused.cpp`:49
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiLlmIntegrationFocusedTests): n/a
  - `<unnamed>` (EG4_CommunityBuild_EnforcePluginGate_PermissionDenied): n/a

#### `TEST(LlmWikiLlmIntegrationFocusedTests, EG5_CommunityBuild_EnforceFeatureGate_PermissionDenied)`
- Source: `tests/llm_wiki/test_llm_wiki_llm_integration_focused.cpp`:60
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiLlmIntegrationFocusedTests): n/a
  - `<unnamed>` (EG5_CommunityBuild_EnforceFeatureGate_PermissionDenied): n/a

#### `TEST(LlmWikiLlmIntegrationFocusedTests, EG6_DisabledSubFeature_IsNotEnabled)`
- Source: `tests/llm_wiki/test_llm_wiki_llm_integration_focused.cpp`:70
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiLlmIntegrationFocusedTests): n/a
  - `<unnamed>` (EG6_DisabledSubFeature_IsNotEnabled): n/a

#### `TEST(LlmWikiLlmIntegrationFocusedTests, EG7_StatusTypes_CarryCorrectCodes)`
- Source: `tests/llm_wiki/test_llm_wiki_llm_integration_focused.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiLlmIntegrationFocusedTests): n/a
  - `<unnamed>` (EG7_StatusTypes_CarryCorrectCodes): n/a

#### `TEST(LlmWikiLlmIntegrationFocusedTests, EG8_EditionEnum_CoversAllEditions)`
- Source: `tests/llm_wiki/test_llm_wiki_llm_integration_focused.cpp`:96
- Brief: n/a
- Parameters:
  - `<unnamed>` (LlmWikiLlmIntegrationFocusedTests): n/a
  - `<unnamed>` (EG8_EditionEnum_CoversAllEditions): n/a

### themis::llm_wiki

#### `std::string computeSHA256(const std::string &data) noexcept`
- Source: `src/llm_wiki/workspace_state_manager.cpp`:46
- Brief: n/a
- Parameters:
  - `data` (const std::string &): n/a

#### `WorkspaceStatus deserializeJsonToState(const json &j, WorkspaceState &out_state) noexcept`
- Source: `src/llm_wiki/workspace_state_manager.cpp`:95
- Brief: n/a
- Parameters:
  - `j` (const json &): n/a
  - `out_state` (WorkspaceState &): n/a

#### `PluginStatus enforceFeatureGate(const char *feature_name) noexcept`
- Source: `src/llm_wiki/edition_gate.cpp`:78
- Brief: Enforce edition gate for a specific LLM Wiki sub-feature.
- Parameters:
  - `feature_name` (const char *): Name of the sub-feature (e.g., "llm_wiki_wikipedia").
- Return: Status indicating whether access is allowed.
- Details: If the sub-feature is not available in the current edition, returns Status::PermissionDenied(). Otherwise, returns Status::Ok(). feature_name Name of the sub-feature (e.g., "llm_wiki_wikipedia"). Status indicating whether access is allowed.

#### `PluginStatus enforcePluginGate(const char *operation_name) noexcept`
- Source: `src/llm_wiki/edition_gate.cpp`:63
- Brief: Enforce edition gate for a plugin operation.
- Parameters:
  - `operation_name` (const char *): Name of the operation (for error message).
- Return: Status indicating whether access is allowed.
- Details: If the plugin is not enabled in the current edition, returns Status::PermissionDenied(). Otherwise, returns Status::Ok(). Use this in all public API entry points. operation_name Name of the operation (for error message). Status indicating whether access is allowed.

#### `Edition getCurrentEdition() noexcept`
- Source: `src/llm_wiki/edition_gate.cpp`:23
- Brief: Detect the current ThemisDB edition at runtime.
- Parameters: none
- Return: Current edition.
- Details: Checks compile-time defines and runtime configuration to determine which edition is active. Current edition.

#### `bool isLLMWikiEnabled() noexcept`
- Source: `src/llm_wiki/edition_gate.cpp`:34
- Brief: Check if the LLM Wiki plugin is enabled in the current edition.
- Parameters: none
- Return: True if plugin is available; false otherwise.
- Details: The plugin is available in enterprise, hyperscaler, and military editions. Returns false for community and minimal. True if plugin is available; false otherwise.

#### `bool isLLMWikiFeatureEnabled(const char *feature_name) noexcept`
- Source: `src/llm_wiki/edition_gate.cpp`:41
- Brief: Check if a specific LLM Wiki sub-feature is enabled.
- Parameters:
  - `feature_name` (const char *): Name of the sub-feature (case-sensitive).
- Return: True if available; false otherwise.
- Details: Sub-features include: "llm_wiki_wikipedia" — Wikipedia dump ingestion (enterprise+ only) "llm_wiki_workspace" — Persistent workspace (enterprise+ only) (Future) multi-tenant, RBAC, quality evaluation feature_name Name of the sub-feature (case-sensitive). True if available; false otherwise.

#### `std::string normalizeForGuardrailCheck(std::string_view text)`
- Source: `src/llm_wiki/guardrail_patterns.h`:101
- Brief: Normalize For Guardrail Check.
- Parameters:
  - `text` (std::string_view): Input parameter.
- Return: Return value.
- Details: text Input parameter. Return value. Calls: reserve(), size(), std::isspace(), push_back(), std::tolower(), empty(), back(), pop_back().

#### `json serializeStateToJson(const WorkspaceState &state) noexcept`
- Source: `src/llm_wiki/workspace_state_manager.cpp`:70
- Brief: n/a
- Parameters:
  - `state` (const WorkspaceState &): n/a

### themis::llm_wiki::ProcessPolicyManager

#### `ProcessPolicyStatus loadFromYaml(const std::filesystem::path &yaml_path, LLMWikiProcessPolicy &out_policy) noexcept`
- Source: `include/llm_wiki/process_policy_manager.h`:123
- Brief: Load policy from YAML file and validate invariants.
- Parameters:
  - `yaml_path` (const std::filesystem::path &): Path to YAML policy file.
  - `out_policy` (LLMWikiProcessPolicy &): Materialized policy on success.
- Return: Status code and error message if failed.
- Details: yaml_path Path to YAML policy file. out_policy Materialized policy on success. Status code and error message if failed.

#### `ProcessPolicyStatus validate(const LLMWikiProcessPolicy &policy) noexcept`
- Source: `include/llm_wiki/process_policy_manager.h`:133
- Brief: Validate a materialized policy.
- Parameters:
  - `policy` (const LLMWikiProcessPolicy &): Materialized policy.
- Return: Status code and validation message.
- Details: policy Materialized policy. Status code and validation message.

### themis::llm_wiki::ProcessPolicyStatus

#### `ProcessPolicyStatus InvalidFile(std::string msg)`
- Source: `include/llm_wiki/process_policy_manager.h`:42
- Brief: n/a
- Parameters:
  - `msg` (std::string): n/a

#### `ProcessPolicyStatus Ok()`
- Source: `include/llm_wiki/process_policy_manager.h`:38
- Brief: n/a
- Parameters: none

#### `ProcessPolicyStatus ParseError(std::string msg)`
- Source: `include/llm_wiki/process_policy_manager.h`:46
- Brief: n/a
- Parameters:
  - `msg` (std::string): n/a

#### `ProcessPolicyStatus ValidationError(std::string msg)`
- Source: `include/llm_wiki/process_policy_manager.h`:50
- Brief: n/a
- Parameters:
  - `msg` (std::string): n/a

#### `bool ok() const noexcept`
- Source: `include/llm_wiki/process_policy_manager.h`:36
- Brief: n/a
- Parameters: none

### themis::llm_wiki::WikiGuardrails

#### `WikiGuardrails()=default`
- Source: `src/llm_wiki/guardrail_patterns.h`:133
- Brief: n/a
- Parameters: none

#### `WikiGuardrails(WikiGuardrails &&)=delete`
- Source: `src/llm_wiki/guardrail_patterns.h`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (WikiGuardrails &&): n/a

#### `WikiGuardrails(const WikiGuardrails &)=delete`
- Source: `src/llm_wiki/guardrail_patterns.h`:137
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WikiGuardrails &): n/a

#### `bool checkPatterns(std::string_view text) const noexcept`
- Source: `src/llm_wiki/guardrail_patterns.h`:153
- Brief: n/a
- Parameters:
  - `text` (std::string_view): n/a

#### `bool isUnsafeContent(std::string_view chunk_text) const noexcept`
- Source: `src/llm_wiki/guardrail_patterns.h`:146
- Brief: n/a
- Parameters:
  - `chunk_text` (std::string_view): n/a

#### `bool isUnsafeQuery(std::string_view query_text) const noexcept`
- Source: `src/llm_wiki/guardrail_patterns.h`:142
- Brief: n/a
- Parameters:
  - `query_text` (std::string_view): n/a

#### `WikiGuardrails & operator=(WikiGuardrails &&)=delete`
- Source: `src/llm_wiki/guardrail_patterns.h`:140
- Brief: n/a
- Parameters:
  - `<unnamed>` (WikiGuardrails &&): n/a

#### `WikiGuardrails & operator=(const WikiGuardrails &)=delete`
- Source: `src/llm_wiki/guardrail_patterns.h`:138
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WikiGuardrails &): n/a

#### `~WikiGuardrails()=default`
- Source: `src/llm_wiki/guardrail_patterns.h`:134
- Brief: n/a
- Parameters: none

### themis::llm_wiki::WorkspaceStateManager

#### `WorkspaceStateManager(const WorkspaceStateManager &)=delete`
- Source: `include/llm_wiki/workspace_state_manager.h`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WorkspaceStateManager &): n/a

#### `WorkspaceStateManager(const std::filesystem::path &workspace_root)`
- Source: `include/llm_wiki/workspace_state_manager.h`:144
- Brief: n/a
- Parameters:
  - `workspace_root` (const std::filesystem::path &): n/a

#### `WorkspaceStatus load(WorkspaceState &out_state) noexcept`
- Source: `include/llm_wiki/workspace_state_manager.h`:165
- Brief: Load workspace state from disk.
- Parameters:
  - `out_state` (WorkspaceState &): n/a
- Return: OK if state loaded successfully, or error code otherwise.
- Details: Read state.json Validate SHA-256 checksum embedded in the file On checksum failure, attempt recovery from state.log Return error if both fail OK if state loaded successfully, or error code otherwise.

#### `WorkspaceStateManager & operator=(const WorkspaceStateManager &)=delete`
- Source: `include/llm_wiki/workspace_state_manager.h`:153
- Brief: n/a
- Parameters:
  - `<unnamed>` (const WorkspaceStateManager &): n/a

#### `WorkspaceStatus recoverFromLog(WorkspaceState &out_state) noexcept`
- Source: `include/llm_wiki/workspace_state_manager.h`:202
- Brief: Attempt recovery from the append-only transaction log.
- Parameters:
  - `out_state` (WorkspaceState &): Reconstructed state (best-effort).
- Return: OK if recovery succeeds, error otherwise.
- Details: If state.json is corrupted, try to reconstruct the state from state.log (which has one JSON object per line). out_state Reconstructed state (best-effort). OK if recovery succeeds, error otherwise.

#### `WorkspaceStatus save(const WorkspaceState &state) noexcept`
- Source: `include/llm_wiki/workspace_state_manager.h`:179
- Brief: Save workspace state to disk atomically.
- Parameters:
  - `state` (const WorkspaceState &): State to persist.
- Return: OK on success, error code otherwise.
- Details: Serialize state to JSON Compute SHA-256 checksum Write to temporary file Rename temp file to state.json (atomic on POSIX) Append to state.log for durability state State to persist. OK on success, error code otherwise.

#### `WorkspaceStatus validateChecksum(const std::filesystem::path &file_path) noexcept`
- Source: `include/llm_wiki/workspace_state_manager.h`:190
- Brief: Validate the checksum of a state file.
- Parameters:
  - `file_path` (const std::filesystem::path &): Path to state.json.
- Return: OK if checksum matches, ChecksumMismatch otherwise.
- Details: Reads the SHA-256 field from the JSON and compares it to the computed hash of the file contents (excluding the checksum field itself). file_path Path to state.json. OK if checksum matches, ChecksumMismatch otherwise.

#### `~WorkspaceStateManager()=default`
- Source: `include/llm_wiki/workspace_state_manager.h`:149
- Brief: n/a
- Parameters: none

### themis::llm_wiki::WorkspaceStatus

#### `WorkspaceStatus ChecksumMismatch(std::string expected, std::string actual)`
- Source: `include/llm_wiki/workspace_state_manager.h`:89
- Brief: n/a
- Parameters:
  - `expected` (std::string): n/a
  - `actual` (std::string): n/a

#### `WorkspaceStatus CorruptState(std::string msg)`
- Source: `include/llm_wiki/workspace_state_manager.h`:85
- Brief: n/a
- Parameters:
  - `msg` (std::string): n/a

#### `WorkspaceStatus Error(std::string msg)`
- Source: `include/llm_wiki/workspace_state_manager.h`:81
- Brief: n/a
- Parameters:
  - `msg` (std::string): n/a

#### `WorkspaceStatus Ok()`
- Source: `include/llm_wiki/workspace_state_manager.h`:77
- Brief: n/a
- Parameters: none

#### `bool ok() const noexcept`
- Source: `include/llm_wiki/workspace_state_manager.h`:75
- Brief: n/a
- Parameters: none

### themis::plugins::llm_wiki::ILLMWikiPlugin

#### `PluginType getType() const override`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:263
- Brief: n/a
- Parameters: none

#### `WikiIngestResult ingest(const std::string &source_path, const WikiIngestOptions &opts={})=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:314
- Brief: Ingest documents from source_path into the wiki index.
- Parameters:
  - `source_path` (const std::string &): File or directory path to ingest. Directories are traversed according to opts.recursive and opts.file_glob.
  - `opts` (const WikiIngestOptions &): Ingestion options; defaults are production-safe.
- Return: WikiIngestResult with counts, error list (failed_files), and per-file error details. A non-empty failed_files vector indicates partial-failure mode (some files succeeded, some failed).
- Details: Performs per-file ingestion with partial-failure semantics: Files are processed sequentially If a file fails to parse or chunk, the error is logged and processing continues The result includes a list of failed files and counts of successful/skipped chunks source_path File or directory path to ingest. Directories are traversed according to opts.recursive and opts.file_glob. opts Ingestion options; defaults are production-safe. WikiIngestResult with counts, error list (failed_files), and per-file error details. A non-empty failed_files vector indicates partial-failure mode (some files succeeded, some failed). Partial-failure means the result is non-fatal even if some files error. Check result.failed_files.empty() to distinguish complete success from partial success with errors.

#### `WikiIngestResult ingestWikipediaDump(const std::string &dump_path, const WikiDumpIngestOptions &opts={})=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:421
- Brief: Ingest a Wikipedia XML dump into the wiki index.
- Parameters:
  - `dump_path` (const std::string &): Local path or http(s):// URI of the compressed Wikipedia dump file (*-pages-articles.xml.bz2).
  - `opts` (const WikiDumpIngestOptions &): Dump ingestion options.
- Return: WikiIngestResult with article/chunk counts.
- Details: Delegates to the C++ Wikipedia ingestion pipeline (src/importers/wikipedia_pipeline.cpp). Resumable via checkpoint when opts.checkpoint_dir is set. Requires license sub-feature "llm_wiki_wikipedia". dump_path Local path or http(s):// URI of the compressed Wikipedia dump file (*-pages-articles.xml.bz2). opts Dump ingestion options. WikiIngestResult with article/chunk counts.

#### `Status initialize(const std::string &config_json)=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:282
- Brief: Initialize the plugin from a JSON configuration blob.
- Parameters:
  - `config_json` (const std::string &): JSON object with plugin configuration.
- Return: Status::Ok() on success; error status otherwise.
- Details: Required fields (all have defaults): "embedding_provider" (string, default "hash") "embedding_dim" (int, default 128) "table_name" (string, default "wiki_chunks") "workspace_root" (string, optional) "rocksdb_dir" (string, optional — activates Phase B if set) "fail_open" (bool, default false; when rocksdb_dir is set, allows explicit test/degraded in-memory fallback on RocksDB init failure) config_json JSON object with plugin configuration. Status::Ok() on success; error status otherwise.

#### `const char * pluginId() const noexcept=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:260
- Brief: n/a
- Parameters: none
- Return: "llm_wiki"
- Details: "llm_wiki"

#### `const char * pluginVersion() const noexcept=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:262
- Brief: n/a
- Parameters: none
- Return: Human-readable name, e.g., "ThemisDB LLM Wiki Plugin v0.1.0"
- Details: Human-readable name, e.g., "ThemisDB LLM Wiki Plugin v0.1.0"

#### `WikiQueryResult query(const std::string &query_text, const WikiQueryOptions &opts={})=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:335
- Brief: Query the wiki index for chunks relevant to query_text.
- Parameters:
  - `query_text` (const std::string &): Natural-language query string.
  - `opts` (const WikiQueryOptions &): Query options.
- Return: WikiQueryResult with scored candidates and guardrail flags.
- Details: Retrieval strategy depends on the active backend: Phase A (JSON/in-memory): BM25 TF token overlap + cosine KNN. Phase B (RocksDB-native): WikiIndexStore BM25 + HNSW + RRF fusion. Guardrails: Query is screened for prompt-injection patterns before retrieval. Individual chunks are filtered if they match unsafe content patterns. query_text Natural-language query string. opts Query options. WikiQueryResult with scored candidates and guardrail flags.

#### `void shutdown() noexcept=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:290
- Brief: Gracefully shut down the plugin and flush pending writes.
- Parameters: none
- Details: Must be called before the shared library is unloaded or the plugin pointer is released.

#### `WikiWorkspaceStats stats(const std::string &workspace_root={})=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:434
- Brief: Return current index and workspace statistics.
- Parameters:
  - `workspace_root` (const std::string &): Optional workspace root path; when empty, only index-level stats are returned.
- Return: WikiWorkspaceStats.
- Details: workspace_root Optional workspace root path; when empty, only index-level stats are returned. WikiWorkspaceStats.

#### `WikiIngestResult wikiIngest(const std::string &source_path, const WikiIngestOptions &opts)=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:368
- Brief: Ingest a single source into an initialized workspace.
- Parameters:
  - `source_path` (const std::string &): Source file path.
  - `opts` (const WikiIngestOptions &): Ingestion options; opts.workspace_root is mandatory.
- Return: WikiIngestResult.
- Details: Appends a log entry, generates a summary page, and creates concept links in wiki/state.json. A raw copy is placed in raw_sources/. source_path Source file path. opts Ingestion options; opts.workspace_root is mandatory. WikiIngestResult.

#### `Status wikiInit(const std::string &workspace_root)=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:356
- Brief: Initialize a persistent LLM Wiki workspace.
- Parameters:
  - `workspace_root` (const std::string &): Absolute path for the workspace directory.
- Return: Status::Ok() on success; error if the directory cannot be created or is already incompatible.
- Details: Creates the canonical workspace directory structure: raw_sources/ — immutable source copies wiki/pages/ — LLM-maintained wiki pages wiki/index.md — content catalog wiki/log.md — append-only ingest/query/lint timeline wiki/schema.md — maintenance rules wiki/state.json — structured state (links, assertions, tasks) workspace_root Absolute path for the workspace directory. Status::Ok() on success; error if the directory cannot be created or is already incompatible.

#### `WikiLintResult wikiLint(const std::string &workspace_root, int max_staleness_days=30)=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:401
- Brief: Run lint checks on a workspace and return diagnostic findings.
- Parameters:
  - `workspace_root` (const std::string &): Absolute workspace root path.
  - `max_staleness_days` (int): Pages whose source refs are older than this are flagged as stale (default 30).
- Return: WikiLintResult with categorised findings.
- Details: Checks: Orphan pages (no inbound links) Missing backlinks Stale synthesis pages (source refs older than max_staleness_days) Unresolved contradiction-review tasks workspace_root Absolute workspace root path. max_staleness_days Pages whose source refs are older than this are flagged as stale (default 30). WikiLintResult with categorised findings.

#### `WikiQueryResult wikiQuery(const std::string &query_text, const WikiQueryOptions &opts)=0`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:383
- Brief: Query an initialized workspace and optionally persist the answer.
- Parameters:
  - `query_text` (const std::string &): Natural-language query string.
  - `opts` (const WikiQueryOptions &): Query options; opts.workspace_root is mandatory.
- Return: WikiQueryResult.
- Details: When opts.save_as_page = true a new page is written to wiki/pages/<slug>.md and indexed. The returned WikiQueryResult::saved_page_path contains the path. query_text Natural-language query string. opts Query options; opts.workspace_root is mandatory. WikiQueryResult.

#### `~ILLMWikiPlugin() override=default`
- Source: `include/llm_wiki/llm_wiki_plugin_interface.h`:255
- Brief: n/a
- Parameters: none

### themis::plugins::llm_wiki::Status

#### `Status Error(std::string msg)`
- Source: `include/llm_wiki/llm_wiki_status.h`:48
- Brief: n/a
- Parameters:
  - `msg` (std::string): n/a

#### `Status InvalidArgument(std::string msg)`
- Source: `include/llm_wiki/llm_wiki_status.h`:52
- Brief: n/a
- Parameters:
  - `msg` (std::string): n/a

#### `Status NotInitialized()`
- Source: `include/llm_wiki/llm_wiki_status.h`:54
- Brief: n/a
- Parameters: none

#### `Status Ok()`
- Source: `include/llm_wiki/llm_wiki_status.h`:47
- Brief: n/a
- Parameters: none

#### `Status PermissionDenied(std::string msg)`
- Source: `include/llm_wiki/llm_wiki_status.h`:50
- Brief: n/a
- Parameters:
  - `msg` (std::string): n/a

#### `bool ok() const noexcept`
- Source: `include/llm_wiki/llm_wiki_status.h`:45
- Brief: n/a
- Parameters: none

