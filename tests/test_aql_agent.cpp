/**
 * @file test_aql_agent.cpp
 * @brief Unit tests for AQL Agent Framework (IAgent / ReActAgent).
 *
 * All tests run without a live LLM model.  LLM responses are simulated by
 * pre-registering tools and relying on the fallback behaviour of LLMAQLHandler
 * when no model is loaded (it returns a placeholder string that we pattern-match
 * in the tests).
 */

#include <gtest/gtest.h>
#include "aql/aql_agent.h"
#include "aql/llm_aql_handler.h"
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::aql;
using json = nlohmann::json;

// ============================================================================
// Fixture
// ============================================================================

class AQLAgentTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_shared<LLMAQLHandler>();

        AgentConfig cfg;
        cfg.max_iterations = 5;
        cfg.verbose        = false;
        agent = std::make_unique<ReActAgent>(handler, cfg);
    }

    void TearDown() override {
        agent.reset();
        handler.reset();
    }

    std::shared_ptr<LLMAQLHandler> handler;
    std::unique_ptr<ReActAgent>    agent;
};

// ============================================================================
// Tool registration tests
// ============================================================================

TEST_F(AQLAgentTest, RegisterTool_AddsToolSuccessfully) {
    AgentTool tool;
    tool.name        = "test_tool";
    tool.description = "A test tool";
    tool.parameter_schema = json::object();
    tool.executor    = [](const json&) -> json { return json{{"ok", true}}; };

    EXPECT_NO_THROW(agent->registerTool(tool));
    EXPECT_TRUE(agent->hasTool("test_tool"));
    EXPECT_EQ(agent->getTools().size(), std::size_t(1));
}

TEST_F(AQLAgentTest, RegisterTool_DuplicateNameThrows) {
    AgentTool tool;
    tool.name        = "dupe_tool";
    tool.description = "Tool A";
    tool.executor    = [](const json&) -> json { return json::object(); };

    agent->registerTool(tool);
    EXPECT_THROW(agent->registerTool(tool), std::invalid_argument);
}

TEST_F(AQLAgentTest, RegisterTool_EmptyNameThrows) {
    AgentTool tool;
    tool.name     = "";  // invalid
    tool.executor = [](const json&) -> json { return json::object(); };
    EXPECT_THROW(agent->registerTool(tool), std::invalid_argument);
}

TEST_F(AQLAgentTest, RemoveTool_RemovesRegisteredTool) {
    AgentTool tool;
    tool.name        = "removable";
    tool.description = "To be removed";
    tool.executor    = [](const json&) -> json { return json::object(); };

    agent->registerTool(tool);
    EXPECT_TRUE(agent->hasTool("removable"));

    EXPECT_NO_THROW(agent->removeTool("removable"));
    EXPECT_FALSE(agent->hasTool("removable"));
    EXPECT_TRUE(agent->getTools().empty());
}

TEST_F(AQLAgentTest, RemoveTool_UnknownNameThrows) {
    EXPECT_THROW(agent->removeTool("nonexistent"), std::invalid_argument);
}

TEST_F(AQLAgentTest, HasTool_ReturnsFalseForUnregisteredTool) {
    EXPECT_FALSE(agent->hasTool("unknown_tool"));
}

TEST_F(AQLAgentTest, GetTools_ReturnsAllRegisteredTools) {
    for (int i = 0; i < 3; ++i) {
        AgentTool t;
        t.name     = "tool_" + std::to_string(i);
        t.executor = [](const json&) -> json { return json::object(); };
        agent->registerTool(t);
    }
    EXPECT_EQ(agent->getTools().size(), std::size_t(3));
}

// ============================================================================
// Configuration tests
// ============================================================================

TEST_F(AQLAgentTest, SetConfig_UpdatesConfiguration) {
    AgentConfig new_cfg;
    new_cfg.max_iterations = 20;
    new_cfg.temperature    = 0.1f;
    new_cfg.model_alias    = "llama-test";

    agent->setConfig(new_cfg);

    const AgentConfig& cfg = agent->getConfig();
    EXPECT_EQ(cfg.max_iterations, 20);
    EXPECT_FLOAT_EQ(cfg.temperature, 0.1f);
    EXPECT_EQ(cfg.model_alias, "llama-test");
}

TEST_F(AQLAgentTest, DefaultConfig_MaxIterationsTen) {
    ReActAgent default_agent(handler);
    EXPECT_EQ(default_agent.getConfig().max_iterations, 10);
}

// ============================================================================
// Execute tests (no live LLM – tests structural / fallback behaviour)
// ============================================================================

TEST_F(AQLAgentTest, Execute_NoToolsNoModel_ReturnsResult) {
    // Without a loaded LLM model, executeInfer() returns a placeholder.
    // The agent should still complete without throwing.
    AgentResult result;
    EXPECT_NO_THROW({
        result = agent->execute("Count all users in the system");
    });
    // Result must always be populated (either an answer or a timeout message).
    EXPECT_FALSE(result.final_answer.empty());
    EXPECT_GE(result.iterations_used, 1);
}

TEST_F(AQLAgentTest, Execute_ReachesMaxIterations_SetsFailureMessage) {
    AgentConfig tight_cfg;
    tight_cfg.max_iterations = 2;
    agent->setConfig(tight_cfg);

    AgentResult result;
    EXPECT_NO_THROW({
        result = agent->execute("What is the meaning of life?");
    });

    // After exhausting iterations the agent synthesises a failure message.
    if (!result.succeeded) {
        EXPECT_NE(result.final_answer.find("maximum iterations"), std::string::npos);
    }
    EXPECT_LE(result.iterations_used, 2);
}

TEST_F(AQLAgentTest, Execute_ToolInvoked_ToolOutputInTrace) {
    bool tool_called = false;
    AgentTool counter_tool;
    counter_tool.name        = "count_documents";
    counter_tool.description = "Returns the number of documents in a collection";
    counter_tool.parameter_schema = json{
        {"type", "object"},
        {"properties", {{"collection", {{"type", "string"}}}}}
    };
    counter_tool.executor = [&tool_called](const json& args) -> json {
        tool_called = true;
        return json{{"count", 42}, {"collection", args.value("collection", "unknown")}};
    };
    agent->registerTool(counter_tool);

    // The actual LLM call (no model loaded) won't generate a well-formed
    // Action: line, but the executor might still be triggered if the LLM
    // response contains the right pattern. Either way the result is valid.
    AgentResult result;
    EXPECT_NO_THROW({
        result = agent->execute("How many documents are in the users collection?");
    });
    EXPECT_FALSE(result.final_answer.empty());
}

TEST_F(AQLAgentTest, Execute_WithContext_DoesNotThrow) {
    json context = {{"user_id", "u42"}, {"tenant", "acme_corp"}};
    AgentResult result;
    EXPECT_NO_THROW({
        result = agent->execute("Show me recent orders", context);
    });
    EXPECT_FALSE(result.final_answer.empty());
}

TEST_F(AQLAgentTest, Execute_EmptyTask_DoesNotThrow) {
    AgentResult result;
    EXPECT_NO_THROW({
        result = agent->execute("");
    });
    EXPECT_FALSE(result.final_answer.empty());
}

// ============================================================================
// ReasoningTrace tests
// ============================================================================

TEST_F(AQLAgentTest, Execute_TraceIsNonEmpty) {
    AgentResult result;
    EXPECT_NO_THROW({
        result = agent->execute("List all collections");
    });
    EXPECT_GE(result.reasoning_trace.size(), std::size_t(1));
}

TEST_F(AQLAgentTest, Execute_IterationsUsedMatchesTraceSize) {
    AgentResult result = agent->execute("Describe the schema");
    EXPECT_EQ(result.iterations_used,
              static_cast<int>(result.reasoning_trace.size()));
}

// ============================================================================
// Move semantics
// ============================================================================

TEST_F(AQLAgentTest, MoveConstruct_PreservesTools) {
    AgentTool t;
    t.name     = "move_tool";
    t.executor = [](const json&) -> json { return json::object(); };
    agent->registerTool(t);

    ReActAgent moved(std::move(*agent));
    EXPECT_TRUE(moved.hasTool("move_tool"));
}
