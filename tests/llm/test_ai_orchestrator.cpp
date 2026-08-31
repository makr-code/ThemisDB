/**
 * @file test_ai_orchestrator.cpp
 * @brief Unit tests for AIOrchestrator, ModeSpecLoader and ToolRegistry.
 *
 * Tests cover:
 *  - ModeSpecLoader: valid YAML parsing, defaults injection, validation errors
 *  - ToolRegistry: registration, allowlist/denylist, invocation
 *  - AIOrchestrator: mode resolution, ask/rag pipelines without real LLM
 *  - modeIdFromString / modeIdToString round-trips
 */

#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include "llm/ai_orchestrator.h"
#include "llm/llm_plugin_interface.h"

using namespace themis::llm;

// ─── Helpers ────────────────────────────────────────────────────────────────

// Minimal ILLMPlugin stub that returns the raw prompt text as the response.
// Used so AIOrchestrator::runAsk() returns a predictable string (the prompt /
// query itself) without needing a real llama.cpp backend.
class EchoLLMPlugin : public ILLMPlugin {
public:
    bool loadModel(const std::string&, const json&) override { return true; }
    void unloadModel() override {}
    std::optional<ModelInfo> getModelInfo() const override {
        ModelInfo info{};
        info.model_id   = "echo";
        info.is_loaded  = true;
        return info;
    }
    bool isModelLoaded() const override { return true; }

    InferenceResponse generate(const InferenceRequest& request) override {
        InferenceResponse resp;
        resp.request_id       = request.request_id;
        resp.model_id         = "echo";
        // Return the raw prompt so that tests can control result.text by
        // setting ctx.query to the desired tool-call JSON.
        resp.text             = request.prompt;
        // Placeholder token estimate: ~4 characters per token (BPE heuristic).
        static constexpr int kAvgCharsPerToken = 4;
        resp.tokens_generated = static_cast<int>(request.prompt.size() / kAvgCharsPerToken);
        return resp;
    }
    InferenceResponse generateRAG(const RAGContext&,
                                  const InferenceRequest& request) override {
        return generate(request);
    }
    std::vector<float> embed([[maybe_unused]] const std::string& text) override {
        return std::vector<float>(8, 0.0f);
    }

    LLMCapabilities getCapabilities() const override { return {}; }
    json getMemoryStats() const override { return {}; }
    json getPerformanceStats() const override { return {}; }

    bool loadLoRA(const std::string&, const std::string&, float) override { return true; }
    bool unloadLoRA(const std::string&) override { return true; }
    std::vector<LoRAInfo> listLoRAs() const override { return {}; }
    std::vector<uint8_t> exportLoRA(const std::string&) override { return {}; }
    bool importLoRA(const std::string&, const std::vector<uint8_t>&) override { return true; }
};

class CaptureRagRequestLLMPlugin : public EchoLLMPlugin {
public:
    InferenceResponse generateRAG(const RAGContext&,
                                  const InferenceRequest& request) override {
        last_lora_adapter_id = request.lora_adapter_id;
        return EchoLLMPlugin::generate(request);
    }

    std::optional<std::string> last_lora_adapter_id;
};

class FixedAdapterCandidateProvider : public IAdapterCandidateProvider {
public:
    explicit FixedAdapterCandidateProvider(std::string selected)
        : selected_(std::move(selected)) {}

    [[nodiscard]] AdapterSelectionResult
    selectCandidates(const AdapterSelectionInput& input) const override {
        AdapterSelectionResult out;
        out.selected_adapter_id = selected_;
        out.reason = "fixed_provider";

        AdapterCandidate c;
        c.adapter_id = selected_;
        c.similarity = 0.93f;
        c.source_layer = "layer_0";
        c.tenant = input.tenant;
        out.candidates.push_back(std::move(c));
        return out;
    }

private:
    std::string selected_;
};

class ThrowingAdapterCandidateProvider : public IAdapterCandidateProvider {
public:
    [[nodiscard]] AdapterSelectionResult
    selectCandidates(const AdapterSelectionInput&) const override {
        throw std::runtime_error("provider_failed");
    }
};

class CapturingTopKAdapterCandidateProvider : public IAdapterCandidateProvider {
public:
    [[nodiscard]] AdapterSelectionResult
    selectCandidates(const AdapterSelectionInput& input) const override {
        last_top_k = static_cast<int>(input.top_k);
        AdapterSelectionResult out;
        out.reason = "capture_top_k";
        return out;
    }

    mutable int last_top_k = -1;
};

class RecordingAdapterApplyService : public IAdapterApplyService {
public:
    bool applyAdapter(const std::string& adapter_id,
                      const std::string&,
                      float) override {
        ++apply_calls;
        current_adapter = adapter_id;
        return apply_ok;
    }

    [[nodiscard]] std::string currentAdapter() const override {
        return current_adapter;
    }

    [[nodiscard]] bool canSwitch() const override {
        return allow_switch;
    }

    bool apply_ok = true;
    bool allow_switch = true;
    int apply_calls = 0;
    std::string current_adapter;
};

class AutoBindingLLMPlugin : public EchoLLMPlugin {
public:
    bool isModelLoaded() const override { return model_loaded; }

    bool loadLoRA(const std::string& lora_id,
                  const std::string& lora_path,
                  float) override {
        ++load_calls;
        last_loaded_id = lora_id;
        last_loaded_path = lora_path;
        if (!load_outcomes.empty()) {
            const bool outcome = load_outcomes.front();
            load_outcomes.erase(load_outcomes.begin());
            return outcome;
        }
        return true;
    }

    bool unloadLoRA(const std::string& lora_id) override {
        ++unload_calls;
        last_unloaded_id = lora_id;
        return unload_ok;
    }

    bool model_loaded = true;
    int load_calls = 0;
    int unload_calls = 0;
    std::string last_loaded_id;
    std::string last_loaded_path;
    std::string last_unloaded_id;
    bool unload_ok = true;
    std::vector<bool> load_outcomes;
};

class FixedRagCostModelService : public IRagCostModelService {
public:
    explicit FixedRagCostModelService(double total = 12.5)
        : total_cost_(total) {}

    [[nodiscard]] std::optional<RagCostEstimate>
    estimate(const RagCostModelInput& input) const override {
        last_tokens_generated = input.tokens_generated;
        RagCostEstimate est;
        est.total_cost = total_cost_;
        est.retrieval_cost = 3.0;
        est.inference_cost = 8.0;
        est.adapter_cost = 1.5;
        est.model = "test_cost_model";
        est.unit = "cost_units";
        est.extra = {{"checked", true}};
        return est;
    }

    mutable int last_tokens_generated = 0;

private:
    double total_cost_ = 12.5;
};

class LinearTopKRagCostModelService : public IRagCostModelService {
public:
    LinearTopKRagCostModelService(double base_cost, double per_doc_cost)
        : base_cost_(base_cost), per_doc_cost_(per_doc_cost) {}

    [[nodiscard]] std::optional<RagCostEstimate>
    estimate(const RagCostModelInput& input) const override {
        RagCostEstimate est;
        est.retrieval_cost =
            base_cost_ + per_doc_cost_ * static_cast<double>(input.retrieved_docs);
        est.inference_cost = 0.0;
        est.adapter_cost = 0.0;
        est.total_cost = est.retrieval_cost;
        est.model = "linear_topk_cost";
        est.unit = "cost_units";
        return est;
    }

private:
    double base_cost_ = 0.0;
    double per_doc_cost_ = 0.0;
};

static const char* kMinimalValidYaml = R"yaml(
apiVersion: themis.ai/v1
kind: ThemisModePack
metadata:
  name: test-pack
  version: "1.0.0"
default_mode: ask
modes:
  - id: ask
    description: "Q&A"
    model: default
    budgets:
      max_tokens: 256
      timeout_ms: 5000
)yaml";

static const char* kRagYaml = R"yaml(
apiVersion: themis.ai/v1
kind: ThemisModePack
metadata:
  name: rag-pack
  version: "1.0.0"
default_mode: rag
models:
  - id: default
    path: ""
    gpu_layers: 0
tools:
  - name: docs_search
    description: "Search docs"
    timeout_ms: 3000
    schema: {}
modes:
  - id: ask
    budgets: {max_tokens: 256, timeout_ms: 5000}
  - id: rag
    model: default
    tools_allowed: [docs_search]
    retrieval:
      enabled: true
      strategy: hybrid
      top_k: 3
      threshold: 0.4
    budgets:
      max_tokens: 512
      timeout_ms: 10000
)yaml";

// ============================================================================
// ModeId helpers
// ============================================================================

TEST(ModeIdTest, RoundTrip_KnownModes) {
    EXPECT_EQ(modeIdFromString("ask"),        ModeId::Ask);
    EXPECT_EQ(modeIdFromString("edit"),       ModeId::Edit);
    EXPECT_EQ(modeIdFromString("rag"),        ModeId::Rag);
    EXPECT_EQ(modeIdFromString("agentic"),    ModeId::Agentic);
    EXPECT_EQ(modeIdFromString("multi_agent"),ModeId::MultiAgent);
    EXPECT_EQ(modeIdFromString("ethics"),     ModeId::Ethics);
    EXPECT_EQ(modeIdFromString("unknown_xyz"),ModeId::Custom);
}

TEST(ModeIdTest, CaseInsensitive) {
    EXPECT_EQ(modeIdFromString("ASK"),  ModeId::Ask);
    EXPECT_EQ(modeIdFromString("RAG"),  ModeId::Rag);
    EXPECT_EQ(modeIdFromString("Edit"), ModeId::Edit);
}

TEST(ModeIdTest, ToString_KnownModes) {
    EXPECT_EQ(modeIdToString(ModeId::Ask),        "ask");
    EXPECT_EQ(modeIdToString(ModeId::Edit),       "edit");
    EXPECT_EQ(modeIdToString(ModeId::Rag),        "rag");
    EXPECT_EQ(modeIdToString(ModeId::Agentic),    "agentic");
    EXPECT_EQ(modeIdToString(ModeId::MultiAgent), "multi_agent");
    EXPECT_EQ(modeIdToString(ModeId::Ethics),     "ethics");
    EXPECT_EQ(modeIdToString(ModeId::Custom),     "custom");
}

// ============================================================================
// ModeSpecLoader – valid YAML
// ============================================================================

TEST(ModeSpecLoaderTest, LoadFromString_MinimalValid) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kMinimalValidYaml, &res);

    EXPECT_TRUE(res.ok) << "Errors: " << (res.errors.empty() ? "" : res.errors[0]);
    EXPECT_EQ(pack.apiVersion,   "themis.ai/v1");
    EXPECT_EQ(pack.kind,         "ThemisModePack");
    EXPECT_EQ(pack.name,         "test-pack");
    EXPECT_EQ(pack.version,      "1.0.0");
    EXPECT_EQ(pack.default_mode, "ask");
    ASSERT_EQ(pack.modes.size(), 1u);
    EXPECT_EQ(pack.modes[0].id,  "ask");
}

TEST(ModeSpecLoaderTest, LoadFromString_RagPack) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);

    EXPECT_TRUE(res.ok) << "Errors: " << (res.errors.empty() ? "" : res.errors[0]);
    EXPECT_EQ(pack.modes.size(), 2u);
    EXPECT_EQ(pack.default_mode, "rag");

    const ModeSpec* rag = nullptr;
    for (const auto& m : pack.modes) {
        if (m.id == "rag") { rag = &m; break; }
    }
    ASSERT_NE(rag, nullptr);
    EXPECT_TRUE(rag->retrieval.enabled);
    EXPECT_EQ(rag->retrieval.top_k, 3);
    EXPECT_FLOAT_EQ(rag->retrieval.threshold, 0.4f);
    ASSERT_EQ(rag->tools_allowed.size(), 1u);
    EXPECT_EQ(rag->tools_allowed[0], "docs_search");
}

TEST(ModeSpecLoaderTest, Defaults_InjectedForMissingFields) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kMinimalValidYaml, &res);

    const ModeSpec& mode = pack.modes[0];
    // Budget defaults
    EXPECT_EQ(mode.budgets.max_tokens,  256);
    EXPECT_EQ(mode.budgets.timeout_ms,  5000);
    // Retrieval defaults
    EXPECT_FALSE(mode.retrieval.enabled);
    EXPECT_EQ(mode.retrieval.strategy,  "hybrid");
    EXPECT_EQ(mode.retrieval.top_k,     5);
    // Output defaults
    EXPECT_EQ(mode.output.format, "text");
}

// ============================================================================
// ModeSpecLoader – validation errors
// ============================================================================

TEST(ModeSpecLoaderTest, InvalidApiVersion_ReturnsError) {
    const char* yaml = R"yaml(
apiVersion: unknown/v99
kind: ThemisModePack
modes:
  - id: ask
    budgets: {max_tokens: 10, timeout_ms: 1000}
)yaml";
    ValidationResult res;
    ModeSpecLoader::loadFromString(yaml, &res);
    EXPECT_FALSE(res.ok);
    bool found = false;
    for (const auto& e : res.errors) {
        if (e.find("apiVersion") != std::string::npos || e.find("Unsupported") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Expected apiVersion error message";
}

TEST(ModeSpecLoaderTest, UnknownKind_ReturnsError) {
    const char* yaml = R"yaml(
apiVersion: themis.ai/v1
kind: SomethingElse
modes:
  - id: ask
    budgets: {max_tokens: 10, timeout_ms: 1000}
)yaml";
    ValidationResult res;
    ModeSpecLoader::loadFromString(yaml, &res);
    EXPECT_FALSE(res.ok);
    bool found = false;
    for (const auto& e : res.errors) {
        if (e.find("kind") != std::string::npos || e.find("Unknown") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Expected kind error message";
}

TEST(ModeSpecLoaderTest, EmptyModes_ReturnsError) {
    const char* yaml = R"yaml(
apiVersion: themis.ai/v1
kind: ThemisModePack
modes: []
)yaml";
    ValidationResult res;
    ModeSpecLoader::loadFromString(yaml, &res);
    EXPECT_FALSE(res.ok);
    bool found = false;
    for (const auto& e : res.errors) {
        if (e.find("modes") != std::string::npos || e.find("empty") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Expected empty modes error";
}

TEST(ModeSpecLoaderTest, DuplicateModeId_ReturnsError) {
    const char* yaml = R"yaml(
apiVersion: themis.ai/v1
kind: ThemisModePack
modes:
  - id: ask
    budgets: {max_tokens: 10, timeout_ms: 1000}
  - id: ask
    budgets: {max_tokens: 10, timeout_ms: 1000}
)yaml";
    ValidationResult res;
    ModeSpecLoader::loadFromString(yaml, &res);
    EXPECT_FALSE(res.ok);
    bool found = false;
    for (const auto& e : res.errors) {
        if (e.find("Duplicate") != std::string::npos || e.find("ask") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Expected duplicate mode id error";
}

TEST(ModeSpecLoaderTest, InvalidDefaultMode_ReturnsError) {
    const char* yaml = R"yaml(
apiVersion: themis.ai/v1
kind: ThemisModePack
default_mode: nonexistent
modes:
  - id: ask
    budgets: {max_tokens: 10, timeout_ms: 1000}
)yaml";
    ValidationResult res;
    ModeSpecLoader::loadFromString(yaml, &res);
    EXPECT_FALSE(res.ok);
    bool found = false;
    for (const auto& e : res.errors) {
        if (e.find("default_mode") != std::string::npos || e.find("nonexistent") != std::string::npos) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Expected default_mode error";
}

TEST(ModeSpecLoaderTest, MalformedYaml_ReturnsError) {
    const char* yaml = "apiVersion: [unclosed\n";
    ValidationResult res;
    ModeSpecLoader::loadFromString(yaml, &res);
    EXPECT_FALSE(res.ok);
    EXPECT_FALSE(res.errors.empty());
}

TEST(ModeSpecLoaderTest, InvalidBudgets_MaxTokensZero_ReturnsError) {
    const char* yaml = R"yaml(
apiVersion: themis.ai/v1
kind: ThemisModePack
modes:
  - id: ask
    budgets: {max_tokens: 0, timeout_ms: 1000}
)yaml";
    ValidationResult res;
    ModeSpecLoader::loadFromString(yaml, &res);
    EXPECT_FALSE(res.ok);
}

TEST(ModeSpecLoaderTest, NegativeRetrievalTopK_ReturnsError) {
    const char* yaml = R"yaml(
apiVersion: themis.ai/v1
kind: ThemisModePack
modes:
  - id: rag
    budgets: {max_tokens: 256, timeout_ms: 5000}
    retrieval:
      enabled: true
      top_k: -1
)yaml";
    ValidationResult res;
    ModeSpecLoader::loadFromString(yaml, &res);
    EXPECT_FALSE(res.ok);
}

// ============================================================================
// ModeSpecLoader – file loading
// ============================================================================

TEST(ModeSpecLoaderTest, LoadFromFile_NonExistentFile_ReturnsError) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromFile("/nonexistent/path/file.yaml", &res);
    EXPECT_FALSE(res.ok);
    EXPECT_FALSE(res.errors.empty());
}

// ============================================================================
// ToolRegistry
// ============================================================================

TEST(ToolRegistryTest, RegisterAndList) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name        = "my_tool";
    spec.description = "A test tool";
    reg.registerTool(spec, [](const json&, const ModeSpec&) { return json{}; });

    auto tools = reg.listTools();
    EXPECT_EQ(tools.size(), 1u);
    EXPECT_EQ(tools[0], "my_tool");
}

TEST(ToolRegistryTest, GetSpec_KnownTool) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name    = "tool_a";
    spec.timeout_ms = 1234;
    reg.registerTool(spec, [](const json&, const ModeSpec&) { return json{}; });

    auto got = reg.getSpec("tool_a");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->name, "tool_a");
    EXPECT_EQ(got->timeout_ms, 1234);
}

TEST(ToolRegistryTest, GetSpec_UnknownTool_ReturnsNullopt) {
    ToolRegistry reg;
    EXPECT_FALSE(reg.getSpec("no_such_tool").has_value());
}

TEST(ToolRegistryTest, IsAllowed_EmptyAllowlist_ReturnsFalse) {
    ToolRegistry reg;
    ModeSpec mode;
    mode.id             = "ask";
    mode.tools_allowed  = {};
    mode.tools_denied   = {};
    EXPECT_FALSE(reg.isAllowed("any_tool", mode));
}

TEST(ToolRegistryTest, IsAllowed_ExplicitAllow) {
    ToolRegistry reg;
    ModeSpec mode;
    mode.id            = "rag";
    mode.tools_allowed = {"docs_search"};
    mode.tools_denied  = {};
    EXPECT_TRUE(reg.isAllowed("docs_search", mode));
    EXPECT_FALSE(reg.isAllowed("other_tool", mode));
}

TEST(ToolRegistryTest, IsAllowed_WildcardAllowAll) {
    ToolRegistry reg;
    ModeSpec mode;
    mode.id            = "agentic";
    mode.tools_allowed = {"*"};
    mode.tools_denied  = {};
    EXPECT_TRUE(reg.isAllowed("any_tool", mode));
    EXPECT_TRUE(reg.isAllowed("docs_search", mode));
}

TEST(ToolRegistryTest, DenyOverridesAllow) {
    ToolRegistry reg;
    ModeSpec mode;
    mode.id            = "agentic";
    mode.tools_allowed = {"*"};
    mode.tools_denied  = {"dangerous_tool"};
    EXPECT_FALSE(reg.isAllowed("dangerous_tool", mode));
    EXPECT_TRUE(reg.isAllowed("safe_tool", mode));
}

TEST(ToolRegistryTest, InvokeTool_NotPermitted_ReturnsError) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name = "restricted";
    reg.registerTool(spec, [](const json&, const ModeSpec&) { return json{{"ok", true}}; });

    ModeSpec mode;
    mode.id            = "ask";
    mode.tools_allowed = {};   // no tools allowed

    json result = reg.invokeTool("restricted", {}, mode);
    EXPECT_TRUE(result.contains("error"));
}

TEST(ToolRegistryTest, InvokeTool_Permitted_CallsHandler) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name = "echo_tool";
    reg.registerTool(spec, []([[maybe_unused]] const json& args, const ModeSpec&) {
        return json{{"echoed", args}};
    });

    ModeSpec mode;
    mode.id            = "rag";
    mode.tools_allowed = {"echo_tool"};

    json args   = {{"query", "hello"}};
    json result = reg.invokeTool("echo_tool", args, mode);
    ASSERT_TRUE(result.contains("echoed"));
    EXPECT_EQ(result["echoed"]["query"], "hello");
}

TEST(ToolRegistryTest, InvokeTool_NotRegistered_ReturnsError) {
    ToolRegistry reg;
    ModeSpec mode;
    mode.id            = "agentic";
    mode.tools_allowed = {"*"};

    json result = reg.invokeTool("not_registered", {}, mode);
    EXPECT_TRUE(result.contains("error"));
}

TEST(ToolRegistryTest, RegisterTool_EmptyName_Throws) {
    ToolRegistry reg;
    ToolSpec spec;
    spec.name = "";
    EXPECT_THROW(
        reg.registerTool(spec, [](const json&, const ModeSpec&) { return json{}; }),
        std::invalid_argument
    );
}

// ============================================================================
// AIOrchestrator – no LLM plugin (stub mode)
// ============================================================================

TEST(AIOrchestrator_NoPluginTest, FindMode_ExistingMode) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kMinimalValidYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    EXPECT_NE(orch.findMode("ask"), nullptr);
    EXPECT_EQ(orch.findMode("nonexistent"), nullptr);
}

TEST(AIOrchestrator_NoPluginTest, DefaultMode_MatchesPackDefault) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kMinimalValidYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    const ModeSpec* def = orch.defaultMode();
    ASSERT_NE(def, nullptr);
    EXPECT_EQ(def->id, "ask");
}

TEST(AIOrchestrator_NoPluginTest, Run_UnknownMode_ReturnsError) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kMinimalValidYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    OrchestratorContext ctx;
    ctx.query   = "test query";
    ctx.mode_id = "nonexistent_mode";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

TEST(AIOrchestrator_NoPluginTest, Run_AskMode_NoPlugin_ReturnsPlaceholder) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kMinimalValidYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    OrchestratorContext ctx;
    ctx.query   = "What is ThemisDB?";
    ctx.mode_id = "ask";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.text.empty());
    EXPECT_EQ(result.metadata.mode_id, "ask");
}

TEST(AIOrchestrator_NoPluginTest, Run_DefaultMode_UsedWhenEmpty) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kMinimalValidYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    OrchestratorContext ctx;
    ctx.query   = "hello";
    ctx.mode_id = "";  // empty → use default

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.metadata.mode_id, "ask");
}

TEST(AIOrchestrator_NoPluginTest, Run_RagMode_NoPlugin_ReturnsPlaceholder) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    OrchestratorContext ctx;
    ctx.query   = "How do I configure sharding?";
    ctx.mode_id = "rag";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.text.empty());
    EXPECT_EQ(result.metadata.mode_id, "rag");
}

TEST(AIOrchestrator_NoPluginTest, Run_RagMode_PrePopulatedDocs_Filtered) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);

    // Pre-populate some documents
    OrchestratorContext ctx;
    ctx.query   = "sharding config";
    ctx.mode_id = "rag";

    RAGContext::Document d1;
    d1.content         = "Sharding is configured via shard.yaml";
    d1.source          = "doc1";
    d1.relevance_score = 0.8f;

    RAGContext::Document d2;
    d2.content         = "Irrelevant document";
    d2.source          = "doc2";
    d2.relevance_score = 0.1f;  // below threshold 0.4

    ctx.documents = {d1, d2};

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    // Only d1 should survive the threshold filter
    EXPECT_EQ(result.metadata.retrieved_docs, 1);
}

TEST(AIOrchestrator_RagAdapterSelectionTest, SelectedAdapterIsPropagatedToRequest) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<CaptureRagRequestLLMPlugin>();
    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_legal_v2"));

    OrchestratorContext ctx;
    ctx.query = "contract clause interpretation";
    ctx.mode_id = "rag";
    ctx.request_id = "req-1";
    ctx.extra["tenant"] = "tenant-a";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    ASSERT_TRUE(llm->last_lora_adapter_id.has_value());
    EXPECT_EQ(llm->last_lora_adapter_id.value(), "adapter_legal_v2");
    EXPECT_EQ(result.metadata.adapter_candidates, 1);
    ASSERT_TRUE(result.metadata.selected_adapter_id.has_value());
    EXPECT_EQ(result.metadata.selected_adapter_id.value(), "adapter_legal_v2");
}

TEST(AIOrchestrator_RagAdapterSelectionTest, ProviderFailureDoesNotBreakRag) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    orch.setLLMPlugin(std::make_shared<CaptureRagRequestLLMPlugin>());
    orch.setAdapterCandidateProvider(std::make_shared<ThrowingAdapterCandidateProvider>());

    OrchestratorContext ctx;
    ctx.query = "retry retrieval semantics";
    ctx.mode_id = "rag";
    ctx.request_id = "req-2";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.metadata.extra.contains("adapter_selection_error"));
}

TEST(AIOrchestrator_RagCostModelTest, CustomServicePopulatesRagCostEstimate) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    orch.setLLMPlugin(std::make_shared<CaptureRagRequestLLMPlugin>());
    auto cost_model = std::make_shared<FixedRagCostModelService>();
    orch.setRagCostModelService(cost_model);

    OrchestratorContext ctx;
    ctx.query = "rag cost estimate";
    ctx.mode_id = "rag";
    ctx.request_id = "req-cost-1";
    ctx.extra["tenant"] = "tenant-cost";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    ASSERT_TRUE(result.metadata.extra.contains("rag_cost_estimate"));

    const json estimate = result.metadata.extra["rag_cost_estimate"];
    EXPECT_DOUBLE_EQ(estimate.value("total_cost", 0.0), 12.5);
    EXPECT_DOUBLE_EQ(estimate.value("retrieval_cost", 0.0), 3.0);
    EXPECT_DOUBLE_EQ(estimate.value("inference_cost", 0.0), 8.0);
    EXPECT_DOUBLE_EQ(estimate.value("adapter_cost", 0.0), 1.5);
    EXPECT_EQ(estimate.value("model", std::string()), "test_cost_model");
    EXPECT_EQ(estimate.value("unit", std::string()), "cost_units");
    EXPECT_TRUE(estimate["extra"].value("checked", false));
    EXPECT_GT(cost_model->last_tokens_generated, 0);
}

TEST(AIOrchestrator_RagCostModelTest, CostBudgetGateBlocksAdapterApplyWhenExceeded) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<CaptureRagRequestLLMPlugin>();
    auto apply = std::make_shared<RecordingAdapterApplyService>();
    auto high_cost = std::make_shared<FixedRagCostModelService>(200.0);

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_budget_v1"));
    orch.setAdapterApplyService(apply);
    orch.setRagCostModelService(high_cost);

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    pol.enable_cost_budget_gate = true;
    pol.max_total_cost = 50.0;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "budget gate";
    ctx.mode_id = "rag";
    ctx.request_id = "req-budget-1";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(apply->apply_calls, 0);
    EXPECT_FALSE(result.metadata.extra.value("adapter_apply_attempted", true));
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_block_reason", std::string()),
              "cost_budget_exceeded");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_block_code", std::string()),
              "cost_budget_exceeded");
    EXPECT_EQ(result.raw_response.value("adapter_apply_block_code", std::string()),
              "cost_budget_exceeded");
    EXPECT_EQ(result.raw_response.value("adapter_apply_error_code", std::string()),
              "cost_budget_exceeded");
    EXPECT_EQ(result.raw_response.value("adapter_apply_error_class", std::string()),
              "non_retryable");
    ASSERT_TRUE(result.raw_response.contains("decision_summary"));
    const json summary = result.raw_response["decision_summary"];
    EXPECT_EQ(summary.value("adapter_apply_block_code", std::string()),
              "cost_budget_exceeded");
    EXPECT_EQ(summary.value("adapter_apply_error_class", std::string()),
              "non_retryable");
    EXPECT_EQ(summary.value("cost_gate_phase", std::string()),
              "pre_apply_switch");
    EXPECT_EQ(summary.value("cost_gate_trigger_count", -1), 1);
    EXPECT_DOUBLE_EQ(result.metadata.extra.value("rag_cost_budget_limit", 0.0), 50.0);
    EXPECT_GT(result.metadata.extra.value("rag_cost_budget_projected_total", 0.0), 50.0);

    json s = orch.stats();
    EXPECT_GE(s["rag_cost_gate_pre_apply_total"].get<int64_t>(), 1);
    EXPECT_EQ(s["rag_cost_gate_pre_retrieval_total"].get<int64_t>(), 0);
    EXPECT_EQ(s["rag_cost_gate_multi_total"].get<int64_t>(), 0);
}

TEST(AIOrchestrator_RagCostModelTest, CostBudgetCanAdaptEffectiveTopK) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    orch.setLLMPlugin(std::make_shared<CaptureRagRequestLLMPlugin>());

    auto topk_provider = std::make_shared<CapturingTopKAdapterCandidateProvider>();
    orch.setAdapterCandidateProvider(topk_provider);
    orch.setRagCostModelService(std::make_shared<LinearTopKRagCostModelService>(0.0, 10.0));

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    pol.enable_cost_budget_gate = true;
    pol.enable_cost_top_k_adaptation = true;
    pol.max_total_cost = 25.0;
    pol.min_top_k_under_budget = 1;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "topk budget adaptation";
    ctx.mode_id = "rag";
    ctx.request_id = "req-topk-budget-1";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(topk_provider->last_top_k, 2);
    EXPECT_EQ(result.metadata.extra.value("rag_cost_top_k_original", -1), 3);
    EXPECT_EQ(result.metadata.extra.value("rag_cost_top_k_effective", -1), 2);
    EXPECT_TRUE(result.metadata.extra.value("rag_cost_top_k_budget_adapted", false));
    ASSERT_TRUE(result.raw_response.contains("decision_summary"));
    const json summary = result.raw_response["decision_summary"];
    EXPECT_EQ(summary.value("cost_gate_phase", std::string()), "pre_retrieval_top_k");
    EXPECT_EQ(summary.value("cost_gate_trigger_count", -1), 1);

    json s = orch.stats();
    EXPECT_GE(s["rag_cost_gate_pre_retrieval_total"].get<int64_t>(), 1);
    EXPECT_EQ(s["rag_cost_gate_pre_apply_total"].get<int64_t>(), 0);
    EXPECT_EQ(s["rag_cost_gate_multi_total"].get<int64_t>(), 0);
}

TEST(AIOrchestrator_RagCostModelTest, CostGatePhaseCanBeMultiWithTwoTriggers) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<CaptureRagRequestLLMPlugin>();
    auto apply = std::make_shared<RecordingAdapterApplyService>();
    auto high_cost = std::make_shared<FixedRagCostModelService>(200.0);

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_multi_gate_v1"));
    orch.setAdapterApplyService(apply);
    orch.setRagCostModelService(high_cost);

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    pol.enable_cost_budget_gate = true;
    pol.enable_cost_top_k_adaptation = true;
    pol.max_total_cost = 50.0;
    pol.min_top_k_under_budget = 2;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "multi gate";
    ctx.mode_id = "rag";
    ctx.request_id = "req-multi-gate-1";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(apply->apply_calls, 0);

    ASSERT_TRUE(result.raw_response.contains("decision_summary"));
    const json summary = result.raw_response["decision_summary"];
    EXPECT_EQ(summary.value("cost_gate_phase", std::string()), "multi");
    EXPECT_EQ(summary.value("cost_gate_trigger_count", -1), 2);
    EXPECT_EQ(summary.value("top_k_effective", -1), 2);
    EXPECT_EQ(summary.value("adapter_apply_block_code", std::string()),
              "cost_budget_exceeded");

    json s = orch.stats();
    EXPECT_GE(s["rag_cost_gate_pre_retrieval_total"].get<int64_t>(), 1);
    EXPECT_GE(s["rag_cost_gate_pre_apply_total"].get<int64_t>(), 1);
    EXPECT_GE(s["rag_cost_gate_multi_total"].get<int64_t>(), 1);
}

TEST(AIOrchestrator_RagCostModelTest, TenantBudgetOverrideCanRelaxCostGate) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<CaptureRagRequestLLMPlugin>();
    auto apply = std::make_shared<RecordingAdapterApplyService>();
    auto high_cost = std::make_shared<FixedRagCostModelService>(200.0);

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_tenant_budget_v1"));
    orch.setAdapterApplyService(apply);
    orch.setRagCostModelService(high_cost);

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    pol.enable_cost_budget_gate = true;
    pol.max_total_cost = 50.0;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "tenant budget override";
    ctx.mode_id = "rag";
    ctx.request_id = "req-tenant-budget-1";
    ctx.extra["tenant"] = "tenant-enterprise";
    ctx.extra["tenant_budget_override"] = 250.0;

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(apply->apply_calls, 1);
    EXPECT_TRUE(result.metadata.extra.value("adapter_apply_attempted", false));
    EXPECT_DOUBLE_EQ(result.metadata.extra.value("rag_cost_budget_limit", 0.0), 250.0);
    EXPECT_DOUBLE_EQ(result.metadata.extra.value("rag_cost_budget_limit_effective", 0.0), 250.0);
    EXPECT_EQ(result.metadata.extra.value("rag_cost_budget_limit_source", std::string()),
              "tenant_budget_override");
}

TEST(AIOrchestrator_RagCostModelTest, DirectTenantBudgetOverrideHasPriorityOverTenantMap) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<CaptureRagRequestLLMPlugin>();
    auto apply = std::make_shared<RecordingAdapterApplyService>();
    auto high_cost = std::make_shared<FixedRagCostModelService>(200.0);

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_budget_priority_v1"));
    orch.setAdapterApplyService(apply);
    orch.setRagCostModelService(high_cost);

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    pol.enable_cost_budget_gate = true;
    pol.max_total_cost = 50.0;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "tenant budget priority";
    ctx.mode_id = "rag";
    ctx.request_id = "req-tenant-budget-prio-1";
    ctx.extra["tenant"] = "tenant-enterprise";
    ctx.extra["tenant_budget_override"] = 250.0;
    ctx.extra["tenant_budgets"] = {
        {"tenant-enterprise", 120.0}
    };

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(apply->apply_calls, 1);
    EXPECT_DOUBLE_EQ(result.metadata.extra.value("rag_cost_budget_limit_effective", 0.0), 250.0);
    EXPECT_EQ(result.metadata.extra.value("rag_cost_budget_limit_source", std::string()),
              "tenant_budget_override");
}

TEST(AIOrchestrator_RagCostModelTest, InvalidTenantBudgetOverrideIsIgnoredAndReported) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<CaptureRagRequestLLMPlugin>();
    auto apply = std::make_shared<RecordingAdapterApplyService>();
    auto high_cost = std::make_shared<FixedRagCostModelService>(200.0);

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_budget_invalid_v1"));
    orch.setAdapterApplyService(apply);
    orch.setRagCostModelService(high_cost);

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    pol.enable_cost_budget_gate = true;
    pol.max_total_cost = 50.0;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "tenant budget invalid";
    ctx.mode_id = "rag";
    ctx.request_id = "req-tenant-budget-invalid-1";
    ctx.extra["tenant"] = "tenant-enterprise";
    ctx.extra["tenant_budget_override"] = -1.0;

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(apply->apply_calls, 0);
    EXPECT_FALSE(result.metadata.extra.value("adapter_apply_attempted", true));
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_block_reason", std::string()),
              "cost_budget_exceeded");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_block_code", std::string()),
              "cost_budget_exceeded");

    EXPECT_TRUE(result.metadata.extra.value("rag_cost_budget_override_invalid", false));
    EXPECT_EQ(result.metadata.extra.value("rag_cost_budget_override_invalid_code", std::string()),
              "tenant_budget_override_non_positive");
    EXPECT_EQ(result.metadata.extra.value("rag_cost_budget_override_invalid_detail", std::string()),
              "tenant_budget_override");
    EXPECT_TRUE(result.raw_response.value("rag_cost_budget_override_invalid", false));
    EXPECT_EQ(result.raw_response.value("rag_cost_budget_override_invalid_code", std::string()),
              "tenant_budget_override_non_positive");
    EXPECT_EQ(result.raw_response.value("rag_cost_budget_override_invalid_detail", std::string()),
              "tenant_budget_override");
    EXPECT_DOUBLE_EQ(result.metadata.extra.value("rag_cost_budget_limit_effective", 0.0), 50.0);
    EXPECT_EQ(result.metadata.extra.value("rag_cost_budget_limit_source", std::string()), "policy");
}

TEST(AIOrchestrator_RagAdapterApplyTest, SelectedAdapterIsAppliedWhenAllowed) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<CaptureRagRequestLLMPlugin>();
    auto apply = std::make_shared<RecordingAdapterApplyService>();

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_finance_v1"));
    orch.setAdapterApplyService(apply);

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 2;
    pol.min_similarity_gain = 0.0f;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "balance sheet guidance";
    ctx.mode_id = "rag";
    ctx.request_id = "req-apply-1";
    ctx.extra["tenant"] = "tenant-fin";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(apply->apply_calls, 1);
    EXPECT_EQ(apply->current_adapter, "adapter_finance_v1");
    EXPECT_TRUE(result.metadata.extra.value("adapter_apply_attempted", false));
    EXPECT_TRUE(result.metadata.extra.value("adapter_apply_success", false));
    EXPECT_EQ(result.raw_response.value("adapter_apply_error_code", std::string()), "none");
    EXPECT_EQ(result.raw_response.value("adapter_apply_error_class", std::string()), "none");
    ASSERT_TRUE(result.raw_response.contains("decision_summary"));
    const json summary = result.raw_response["decision_summary"];
    EXPECT_TRUE(summary.value("adapter_apply_attempted", false));
    EXPECT_TRUE(summary.value("adapter_apply_success", false));
    EXPECT_EQ(summary.value("adapter_apply_block_code", std::string()), "none");
}

TEST(AIOrchestrator_RagAdapterApplyTest, DebounceSkipsSameAdapter) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<CaptureRagRequestLLMPlugin>();
    auto apply = std::make_shared<RecordingAdapterApplyService>();
    apply->current_adapter = "adapter_same_v1";

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_same_v1"));
    orch.setAdapterApplyService(apply);

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "debounce test";
    ctx.mode_id = "rag";
    ctx.request_id = "req-apply-2";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(apply->apply_calls, 0);
    EXPECT_FALSE(result.metadata.extra.value("adapter_apply_attempted", true));
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_block_reason", std::string()),
              "same_adapter");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_block_code", std::string()),
              "same_adapter");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_code", std::string()),
              "same_adapter");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_class", std::string()),
              "non_retryable");
}

TEST(AIOrchestrator_RagAdapterApplyTest, CooldownBlocksSecondSwitch) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<CaptureRagRequestLLMPlugin>();
    auto apply = std::make_shared<RecordingAdapterApplyService>();

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_cooldown_v1"));
    orch.setAdapterApplyService(apply);

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 60000;
    pol.max_switches_per_request = 5;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext first;
    first.query = "cooldown first";
    first.mode_id = "rag";
    first.request_id = "req-apply-3a";
    OrchestratorResult first_result = orch.run(first);
    EXPECT_TRUE(first_result.success);

    OrchestratorContext second;
    second.query = "cooldown second";
    second.mode_id = "rag";
    second.request_id = "req-apply-3b";
    OrchestratorResult second_result = orch.run(second);
    EXPECT_TRUE(second_result.success);

    EXPECT_EQ(apply->apply_calls, 1);
    EXPECT_EQ(second_result.metadata.extra.value("adapter_apply_block_reason", std::string()),
              "cooldown");
    EXPECT_EQ(second_result.metadata.extra.value("adapter_apply_block_code", std::string()),
              "cooldown");
    EXPECT_EQ(second_result.metadata.extra.value("adapter_apply_error_code", std::string()),
              "cooldown");
    EXPECT_EQ(second_result.metadata.extra.value("adapter_apply_error_class", std::string()),
              "retryable");
}

TEST(AIOrchestrator_RagAdapterApplyTest, ForceRollbackReappliesPreviousAdapter) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<CaptureRagRequestLLMPlugin>();
    auto apply = std::make_shared<RecordingAdapterApplyService>();
    apply->current_adapter = "adapter_prev_v1";

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_new_v2"));
    orch.setAdapterApplyService(apply);

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 2;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "rollback quality gate";
    ctx.mode_id = "rag";
    ctx.request_id = "req-apply-rollback";
    ctx.extra["force_adapter_rollback"] = true;

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(apply->apply_calls, 2);
    EXPECT_TRUE(result.metadata.extra.value("adapter_rollback_attempted", false));
    EXPECT_TRUE(result.metadata.extra.value("adapter_rollback_success", false));
}

TEST(AIOrchestrator_RagAdapterObservabilityTest, StatsExposeAdapterRagCounters) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<CaptureRagRequestLLMPlugin>();
    auto apply = std::make_shared<RecordingAdapterApplyService>();

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_metrics_v1"));
    orch.setAdapterApplyService(apply);

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 3;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "metrics path";
    ctx.mode_id = "rag";
    ctx.request_id = "req-metrics-1";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);

    json s = orch.stats();
    EXPECT_GE(s["rag_retrieval_trigger_total"].get<int64_t>(), 1);
    EXPECT_GE(s["rag_reretrieval_total"].get<int64_t>(), 1);
    EXPECT_GE(s["rag_adapter_candidates_total"].get<int64_t>(), 1);
    EXPECT_GE(s["rag_adapter_switch_total"].get<int64_t>(), 1);
    EXPECT_TRUE(s.contains("rag_adapter_switch_latency_ms"));
}

TEST(AIOrchestrator_RagAdapterApplyTest, AutoBindsPluginApplyServiceWhenUnset) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<AutoBindingLLMPlugin>();
    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_autobind_v1"));

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "autobind path";
    ctx.mode_id = "rag";
    ctx.request_id = "req-autobind-1";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(llm->load_calls, 1);
    EXPECT_EQ(llm->last_loaded_id, "adapter_autobind_v1");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_code", std::string()),
              "none");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_class", std::string()),
              "none");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_attempts", -1), 1);
    EXPECT_FALSE(result.metadata.extra.value("adapter_apply_retried", true));
}

TEST(AIOrchestrator_RagAdapterApplyTest, AutoBindingUsesConfiguredPathResolver) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<AutoBindingLLMPlugin>();
    orch.setAdapterPathResolver([](const std::string& adapter_id, const std::string& tenant)
                                    -> std::optional<std::string> {
        return "C:/adapters/" + tenant + "/" + adapter_id + ".safetensors";
    });
    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_path_v1"));

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "resolver path";
    ctx.mode_id = "rag";
    ctx.request_id = "req-path-1";
    ctx.extra["tenant"] = "tenant-a";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(llm->load_calls, 1);
    EXPECT_EQ(llm->last_loaded_path,
              "C:/adapters/tenant-a/adapter_path_v1.safetensors");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_code", std::string()),
              "none");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_class", std::string()),
              "none");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_attempts", -1), 1);
    EXPECT_FALSE(result.metadata.extra.value("adapter_apply_retried", true));
}

TEST(AIOrchestrator_RagAdapterApplyTest, EmptyResolvedPathFailsApplyGracefully) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<AutoBindingLLMPlugin>();
    orch.setAdapterPathResolver([](const std::string&, const std::string&)
                                    -> std::optional<std::string> {
        return std::nullopt;
    });
    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_missing"));

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "resolver miss";
    ctx.mode_id = "rag";
    ctx.request_id = "req-path-miss-1";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(llm->load_calls, 0);
    EXPECT_FALSE(result.metadata.extra.value("adapter_apply_success", true));
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_code", std::string()),
              "resolver_empty_path");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_class", std::string()),
              "non_retryable");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_attempts", -1), 1);
    EXPECT_FALSE(result.metadata.extra.value("adapter_apply_retried", true));
}

TEST(AIOrchestrator_RagAdapterApplyTest, UnloadFailureIsReportedAsErrorCode) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<AutoBindingLLMPlugin>();
    llm->unload_ok = false;

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_next_v2"));

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    orch.setAdapterSwitchPolicy(pol);

    // First run sets active adapter.
    OrchestratorContext first;
    first.query = "first adapter";
    first.mode_id = "rag";
    first.request_id = "req-unload-1";
    OrchestratorResult first_result = orch.run(first);
    EXPECT_TRUE(first_result.success);
    EXPECT_EQ(llm->load_calls, 1);

    // Second run tries to switch and fails during unload of previous adapter.
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_next_v3"));

    OrchestratorContext second;
    second.query = "second adapter";
    second.mode_id = "rag";
    second.request_id = "req-unload-2";
    OrchestratorResult second_result = orch.run(second);

    EXPECT_TRUE(second_result.success);
    EXPECT_EQ(llm->load_calls, 1);
    EXPECT_EQ(llm->unload_calls, 1);
    EXPECT_EQ(second_result.metadata.extra.value("adapter_apply_error_code", std::string()),
              "unload_failed");
    EXPECT_EQ(second_result.metadata.extra.value("adapter_apply_error_class", std::string()),
              "non_retryable");
    EXPECT_EQ(second_result.metadata.extra.value("adapter_apply_attempts", -1), 1);
    EXPECT_FALSE(second_result.metadata.extra.value("adapter_apply_retried", true));
}

TEST(AIOrchestrator_RagAdapterApplyTest, RetryableLoadFailureRetriesAndSucceeds) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<AutoBindingLLMPlugin>();
    llm->load_outcomes = {false, true};

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_retry_v1"));

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    pol.max_retry_attempts = 1;
    pol.retry_backoff_ms = 0;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "retry load";
    ctx.mode_id = "rag";
    ctx.request_id = "req-retry-1";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(llm->load_calls, 2);
    EXPECT_TRUE(result.metadata.extra.value("adapter_apply_success", false));
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_code", std::string()),
              "none");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_class", std::string()),
              "none");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_attempts", -1), 2);
    EXPECT_TRUE(result.metadata.extra.value("adapter_apply_retried", false));
    EXPECT_FALSE(result.metadata.extra.value("adapter_apply_retry_exhausted", true));

    json s = orch.stats();
    EXPECT_GE(s["rag_adapter_retry_total"].get<int64_t>(), 1);
    EXPECT_GE(s["rag_adapter_retry_success_total"].get<int64_t>(), 1);
    EXPECT_EQ(s["rag_adapter_retry_exhausted_total"].get<int64_t>(), 0);
}

TEST(AIOrchestrator_RagAdapterApplyTest, RetryableLoadFailureCanExhaustRetries) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    auto llm = std::make_shared<AutoBindingLLMPlugin>();
    llm->load_outcomes = {false, false};

    orch.setLLMPlugin(llm);
    orch.setAdapterCandidateProvider(
        std::make_shared<FixedAdapterCandidateProvider>("adapter_retry_fail_v1"));

    AdapterSwitchPolicy pol;
    pol.min_switch_interval_ms = 0;
    pol.max_switches_per_request = 1;
    pol.max_retry_attempts = 1;
    pol.retry_backoff_ms = 0;
    orch.setAdapterSwitchPolicy(pol);

    OrchestratorContext ctx;
    ctx.query = "retry exhaust";
    ctx.mode_id = "rag";
    ctx.request_id = "req-retry-exhaust-1";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(llm->load_calls, 2);
    EXPECT_FALSE(result.metadata.extra.value("adapter_apply_success", true));
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_code", std::string()),
              "load_failed");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_error_class", std::string()),
              "retryable");
    EXPECT_EQ(result.metadata.extra.value("adapter_apply_attempts", -1), 2);
    EXPECT_TRUE(result.metadata.extra.value("adapter_apply_retried", false));
    EXPECT_TRUE(result.metadata.extra.value("adapter_apply_retry_exhausted", false));

    json s = orch.stats();
    EXPECT_GE(s["rag_adapter_retry_total"].get<int64_t>(), 1);
    EXPECT_EQ(s["rag_adapter_retry_success_total"].get<int64_t>(), 0);
    EXPECT_GE(s["rag_adapter_retry_exhausted_total"].get<int64_t>(), 1);
}

TEST(AIOrchestrator_NoPluginTest, Stats_IncreaseAfterRun) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kMinimalValidYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);

    OrchestratorContext ctx;
    ctx.query   = "test";
    ctx.mode_id = "ask";

    orch.run(ctx);
    orch.run(ctx);

    json s = orch.stats();
    EXPECT_GE(s["total_runs"].get<int64_t>(), 2);
}

TEST(AIOrchestrator_NoPluginTest, Run_MetadataContainsModeAndModel) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kMinimalValidYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    OrchestratorContext ctx;
    ctx.query   = "ping";
    ctx.mode_id = "ask";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_EQ(result.metadata.mode_id,  "ask");
    EXPECT_EQ(result.metadata.model_id, "default");
}

// ============================================================================
// ToolRegistry integration with AIOrchestrator
// ============================================================================

TEST(AIOrchestrator_ToolTest, RegisterAndInvoke_ViaOrchestrator) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kRagYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);

    // Register docs_search tool that returns canned docs
    ToolSpec spec;
    spec.name        = "docs_search";
    spec.description = "Test doc search";

    orch.toolRegistry().registerTool(spec, []([[maybe_unused]] const json& args, const ModeSpec&) {
        return json{
            {"documents", json::array({
                {{"content", "Sharding doc"}, {"source", "shard.md"}, {"relevance_score", 0.9f}},
                {{"content", "Replication doc"}, {"source", "repl.md"}, {"relevance_score", 0.5f}},
            })}
        };
    });

    OrchestratorContext ctx;
    ctx.query   = "how does sharding work?";
    ctx.mode_id = "rag";

    OrchestratorResult result = orch.run(ctx);
    EXPECT_TRUE(result.success);
    // docs_search was called
    EXPECT_FALSE(result.metadata.tool_calls_made.empty());
    EXPECT_EQ(result.metadata.tool_calls_made[0], "docs_search");
}

// ============================================================================
// Audit regression tests (fixes from code-audit session)
// ============================================================================

// ── Fix 1: Wildcard "*" in tools_allowed must NOT produce a validation warning ─

TEST(AuditRegressionTest, WildcardAllowlist_NoFalsePositiveWarning) {
    const char* yaml = R"yaml(
apiVersion: themis.ai/v1
kind: ThemisModePack
modes:
  - id: agentic
    budgets: {max_tokens: 256, timeout_ms: 5000}
    tools_allowed: ["*"]
)yaml";
    // No tools[] section → before the fix, "*" would trigger a warning
    // "tools_allowed references unknown tool '*'" (false positive).
    ValidationResult res;
    ModeSpecLoader::loadFromString(yaml, &res);
    // The pack has no apiVersion warning issues that cause ok=false, but
    // let's check that no error/warning mentions "*" as an unknown tool.
    for (const auto& w : res.warnings) {
        EXPECT_EQ(w.find("unknown tool '*'"), std::string::npos)
            << "False-positive warning for wildcard: " << w;
    }
    for (const auto& e : res.errors) {
        EXPECT_EQ(e.find("unknown tool '*'"), std::string::npos)
            << "False-positive error for wildcard: " << e;
    }
}

TEST(AuditRegressionTest, WildcardAllowlist_IsAllowed_AnyTool) {
    ToolRegistry reg;
    ModeSpec mode;
    mode.id            = "agentic";
    mode.tools_allowed = {"*"};
    mode.tools_denied  = {};
    EXPECT_TRUE(reg.isAllowed("any_arbitrary_tool", mode));
    EXPECT_TRUE(reg.isAllowed("docs_search",        mode));
    EXPECT_TRUE(reg.isAllowed("aql_execute",        mode));
}

TEST(AuditRegressionTest, WildcardAllowlist_DenyOverridesWildcard) {
    ToolRegistry reg;
    ModeSpec mode;
    mode.id            = "agentic";
    mode.tools_allowed = {"*"};
    mode.tools_denied  = {"dangerous_tool"};
    EXPECT_FALSE(reg.isAllowed("dangerous_tool", mode));
    EXPECT_TRUE(reg.isAllowed("safe_tool",       mode));
}

// ── Fix 2: ToolRegistry thread-safety ──────────────────────────────────────

TEST(AuditRegressionTest, ToolRegistry_ConcurrentReads_NoDataRace) {
    // Register a tool once, then invoke it concurrently from multiple threads.
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(R"yaml(
apiVersion: themis.ai/v1
kind: ThemisModePack
modes:
  - id: ask
    budgets: {max_tokens: 64, timeout_ms: 5000}
    tools_allowed: ["counter_tool"]
)yaml", &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);

    std::atomic<int> call_count{0};
    ToolSpec spec;
    spec.name = "counter_tool";
    orch.toolRegistry().registerTool(spec,
        [&call_count](const json&, const ModeSpec&) -> json {
            ++call_count;
            return {{"ok", true}};
        });

    // Launch 8 concurrent readers
    constexpr int kThreads = 8;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    ModeSpec mode;
    mode.id            = "ask";
    mode.tools_allowed = {"counter_tool"};

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&orch, &mode]() {
            for (int j = 0; j < 10; ++j) {
                json r = orch.toolRegistry().invokeTool("counter_tool", {}, mode);
                EXPECT_FALSE(r.contains("error"));
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(call_count.load(), kThreads * 10);
}

TEST(AuditRegressionTest, ToolRegistry_ConcurrentRegisterAndRead_NoDataRace) {
    ToolRegistry reg;

    // Writer thread registers tools while reader threads query
    std::atomic<bool> stop{false};

    std::thread writer([&reg, &stop]() {
        int n = 0;
        while (!stop) {
            ToolSpec s;
            s.name = "tool_" + std::to_string(n++ % 5);
            reg.registerTool(s, [](const json&, const ModeSpec&) { return json{{"ok",1}}; });
        }
    });

    std::thread reader([&reg, &stop]() {
        for (int i = 0; i < 100; ++i) {
            (void)reg.listTools();
            (void)reg.getSpec("tool_0");
        }
        stop = true;
    });

    writer.join();
    reader.join();
    // Verify the registry contains the expected tools after concurrent ops.
    // This provides value even without sanitizers.
    for (int i = 0; i < 5; ++i) {
        auto spec = reg.getSpec("tool_" + std::to_string(i));
        EXPECT_TRUE(spec.has_value()) << "Expected tool_" << i << " to be registered";
    }
}

// ── Fix 3: Namespace correctness (compile-time; tested via inclusion) ────────

TEST(AuditRegressionTest, TypesInCorrectNamespace) {
    // Verify that key types live in themis::llm (not a nested themis::themis::...)
    static_assert(std::is_class_v<themis::llm::AIOrchestrator>,
                  "AIOrchestrator must be in themis::llm");
    static_assert(std::is_class_v<themis::llm::ToolRegistry>,
                  "ToolRegistry must be in themis::llm");
    static_assert(std::is_class_v<themis::llm::ModeSpecLoader>,
                  "ModeSpecLoader must be in themis::llm");
    SUCCEED();
}

// ============================================================================
// Agentic mode – tool call parsing (v1.8.0)
// ============================================================================

// YAML pack with an "agentic" mode and a registered tool.
static const char* kAgenticYaml = R"yaml(
apiVersion: themis.ai/v1
kind: ThemisModePack
metadata:
  name: agentic-pack
  version: "1.0.0"
default_mode: agentic
tools:
  - name: calc_tool
    description: "Simple calculator tool"
    timeout_ms: 3000
    schema: {}
modes:
  - id: agentic
    budgets: {max_tokens: 256, timeout_ms: 5000}
    tools_allowed: ["calc_tool"]
)yaml";

// When the LLM response is valid tool-call JSON the orchestrator should
// dispatch the tool and return its result as response.text.
TEST(AgenticToolCallTest, ValidToolCallJson_Dispatched) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kAgenticYaml, &res);
    ASSERT_TRUE(res.ok) << res.errors.front();

    AIOrchestrator orch(pack);
    // Inject a plugin that echoes the prompt back as response.text so that
    // the tool-call JSON we put in ctx.query reaches runAgentic() intact.
    orch.setLLMPlugin(std::make_shared<EchoLLMPlugin>());

    // Register the tool with a known return value.
    ToolSpec spec;
    spec.name = "calc_tool";
    std::atomic<int> call_count{0};
    orch.toolRegistry().registerTool(spec,
        [&call_count]([[maybe_unused]] const json& args, const ModeSpec&) -> json {
            ++call_count;
            return {{"result", args.value("x", 0) + args.value("y", 0)}};
        });

    // Set the query to a valid tool-call JSON: the EchoLLMPlugin will return
    // it verbatim so runAgentic() sees it as result.text.
    OrchestratorContext ctx;
    ctx.query   = R"({"name":"calc_tool","arguments":{"x":3,"y":4}})";
    ctx.mode_id = "agentic";

    OrchestratorResult result = orch.run(ctx);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(call_count.load(), 1) << "Tool should have been dispatched exactly once";
    EXPECT_FALSE(result.metadata.tool_calls_made.empty());
    EXPECT_EQ(result.metadata.tool_calls_made[0], "calc_tool");
    // The response text should be the serialised tool result.
    json tool_out = json::parse(result.text);
    EXPECT_EQ(tool_out.value("result", -1), 7);
    // raw_response should carry the tool name and result.
    EXPECT_EQ(result.raw_response.value("tool_name", ""), "calc_tool");
}

// When the LLM response is plain text (not JSON) the orchestrator should
// return it unchanged without throwing.
TEST(AgenticToolCallTest, PlainTextResponse_NoToolDispatched) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kAgenticYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);

    ToolSpec spec;
    spec.name = "calc_tool";
    std::atomic<int> call_count{0};
    orch.toolRegistry().registerTool(spec,
        [&call_count]([[maybe_unused]] const json&, const ModeSpec&) -> json {
            ++call_count;
            return {{"ok", true}};
        });

    OrchestratorContext ctx;
    ctx.query   = "This is a plain text answer, not a tool call.";
    ctx.mode_id = "agentic";

    OrchestratorResult result = orch.run(ctx);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(call_count.load(), 0) << "No tool should be dispatched for plain text";
    EXPECT_TRUE(result.metadata.tool_calls_made.empty());
    // The raw LLM echo text should be preserved.
    EXPECT_FALSE(result.text.empty());
}

// Malformed JSON in the response must not crash; the raw text is preserved.
TEST(AgenticToolCallTest, MalformedJson_GracefulFallback) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kAgenticYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);

    ToolSpec spec;
    spec.name = "calc_tool";
    orch.toolRegistry().registerTool(spec,
        []([[maybe_unused]] const json&, const ModeSpec&) -> json { return {{"ok", true}}; });

    OrchestratorContext ctx;
    ctx.query   = "{not valid json at all!!!";
    ctx.mode_id = "agentic";

    // Must not throw.
    OrchestratorResult result;
    EXPECT_NO_THROW(result = orch.run(ctx));
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.metadata.tool_calls_made.empty());
}

// Valid JSON that is NOT a tool call (missing "name" key) should be left alone.
TEST(AgenticToolCallTest, ValidJsonButNotToolCall_NoDispatch) {
    ValidationResult res;
    ModePack pack = ModeSpecLoader::loadFromString(kAgenticYaml, &res);
    ASSERT_TRUE(res.ok);

    AIOrchestrator orch(pack);
    // Inject echo plugin so result.text is the raw JSON (not a prefixed echo).
    orch.setLLMPlugin(std::make_shared<EchoLLMPlugin>());

    ToolSpec spec;
    spec.name = "calc_tool";
    std::atomic<int> call_count{0};
    orch.toolRegistry().registerTool(spec,
        [&call_count](const json&, const ModeSpec&) -> json {
            ++call_count;
            return {{"ok", true}};
        });

    OrchestratorContext ctx;
    ctx.query   = R"({"answer": "42", "confidence": 0.9})";
    ctx.mode_id = "agentic";

    OrchestratorResult result = orch.run(ctx);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(call_count.load(), 0);
    EXPECT_TRUE(result.metadata.tool_calls_made.empty());
}
