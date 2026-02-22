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
#include "llm/ai_orchestrator.h"

using namespace themis::llm;

// ─── Helpers ────────────────────────────────────────────────────────────────

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
    reg.registerTool(spec, [](const json& args, const ModeSpec&) {
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

    orch.toolRegistry().registerTool(spec, [](const json& args, const ModeSpec&) {
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
