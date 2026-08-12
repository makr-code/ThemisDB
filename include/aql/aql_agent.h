/**
 * @file aql_agent.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "aql/llm_error_codes.h"
#include "aql/llm_aql_handler.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace aql {

using json = nlohmann::json;

// ============================================================================
// Agent Framework Data Types
// ============================================================================

/**
 * @brief A callable tool that an agent can invoke during reasoning.
 *
 * Tools encapsulate side-effecting operations (database queries, calculations,
 * external service calls) that the LLM cannot perform on its own.  Each tool
 * has a name, a human-readable description, a JSON Schema describing its
 * parameters, and an executor function.
 *
 * Security note: executor functions must not access the filesystem or the
 * network unless explicitly permitted by the embedding application.
 */
struct AgentTool {
    /// Unique identifier used by the LLM to invoke the tool (e.g. "query_db").
    std::string name;

    /// Human-readable description shown to the LLM in the system prompt.
    std::string description;

    /**
     * @brief JSON Schema (draft-07) describing the tool's input parameters.
     *
     * Example:
     * @code{.json}
     * {
     *   "type": "object",
     *   "properties": {
     *     "collection": {"type": "string"},
     *     "limit":      {"type": "integer", "default": 10}
     *   },
     *   "required": ["collection"]
     * }
     * @endcode
     */
    json parameter_schema;

    /**
     * @brief Callable that executes the tool with validated JSON arguments.
     *
     * @param args  Tool arguments (pre-validated against @c parameter_schema).
     * @return      Tool output as JSON.
     * @throws std::runtime_error on execution failure.
     */
    std::function<json(const json& args)> executor;
};

/**
 * @brief Configuration for a ReActAgent instance.
 */
struct AgentConfig {
    /// LLM model alias to use for reasoning steps (empty = default model).
    std::string model_alias;

    /// Maximum number of Thought→Action→Observation cycles before giving up.
    int max_iterations = 10;

    /// LLM sampling temperature for reasoning (lower = more deterministic).
    float temperature = 0.3f;

    /// Maximum tokens to generate per reasoning step.
    int max_tokens_per_step = 512;

    /// When true, each reasoning step is logged via spdlog at DEBUG level.
    bool verbose = false;
};

/**
 * @brief A single Thought→Action→Observation step in the ReAct reasoning chain.
 */
struct ReasoningStep {
    /// LLM's internal reasoning text ("I should look up X because…").
    std::string thought;

    /// Name of the tool invoked (empty if no tool was called this step).
    std::optional<std::string> tool_name;

    /// Arguments passed to the tool (nullopt when no tool was invoked).
    std::optional<json> tool_input;

    /// Output returned by the tool (nullopt when no tool was invoked).
    std::optional<json> tool_output;

    /// Description of what was learned from the tool output (or "N/A").
    std::string observation;
};

/**
 * @brief The result of an agent's task execution.
 */
struct AgentResult {
    /// Final synthesised answer to the task.
    std::string final_answer;

    /// Ordered trace of every reasoning step taken to reach the answer.
    std::vector<ReasoningStep> reasoning_trace;

    /// Number of Thought→Action→Observation cycles consumed.
    int iterations_used = 0;

    /// true if the agent reached a final answer; false on max-iterations timeout.
    bool succeeded = false;
};

// ============================================================================
// IAgent – abstract interface
// ============================================================================

/**
 * @brief Abstract agent interface for multi-step LLM reasoning with tool use.
 *
 * Implementations must be thread-compatible (no concurrent execute() calls on
 * the same instance) but may be invoked sequentially from any thread.
 */
class IAgent {
public:
    virtual ~IAgent() = default;

    /**
     * @brief Execute a task using iterative LLM reasoning and tool invocation.
     *
     * @param task     Natural-language description of the goal (e.g. "Find the
     *                 top 5 users by order count and summarise their profiles").
     * @param context  Optional JSON context injected into the system prompt
     *                 (e.g. current user, session info).
     * @return         AgentResult containing the final answer and reasoning trace.
     * @throws LLMException on LLM errors.
     * @throws std::runtime_error on tool execution failures that are not caught
     *         internally.
     */
    virtual AgentResult execute(
        const std::string& task,
        const json& context = json::object()
    ) = 0;

    /**
     * @brief Register a tool that the agent may invoke during reasoning.
     *
     * @throws std::invalid_argument if a tool with the same name is already
     *         registered.
     */
    virtual void registerTool(const AgentTool& tool) = 0;

    /**
     * @brief Remove a previously registered tool by name.
     *
     * @throws std::invalid_argument if no tool with that name is registered.
     */
    virtual void removeTool(const std::string& name) = 0;

    /// Return all currently registered tools.
    virtual std::vector<AgentTool> getTools() const = 0;

    /// Return true if a tool with @p name is registered.
    virtual bool hasTool(const std::string& name) const = 0;
};

// ============================================================================
// ReActAgent – concrete ReAct implementation
// ============================================================================

/**
 * @brief Concrete agent implementing the ReAct (Reasoning + Acting) pattern.
 *
 * ReActAgent alternates between:
 *   1. **Thought** – the LLM reasons about the current state.
 *   2. **Action**  – the LLM selects and calls a registered tool.
 *   3. **Observation** – the tool output is fed back to the LLM.
 *
 * This continues until the LLM emits a "Final Answer:" prefix or
 * @c AgentConfig::max_iterations is reached.
 *
 * Example:
 * @code
 *   ReActAgent agent(std::make_shared<LLMAQLHandler>(), config);
 *   agent.registerTool({
 *       "count_users",
 *       "Count documents in a collection",
 *       json{{"type","object"},{"properties",{{"collection",{{"type","string"}}}}}},
 *       [](const json& args) -> json {
 *           return json{{"count", 42}};  // real impl would query the DB
 *       }
 *   });
 *   auto result = agent.execute("How many users are in the system?");
 *   std::cout << result.final_answer;
 * @endcode
 */
class ReActAgent : public IAgent {
public:
    /**
     * @brief Construct a ReActAgent.
     *
     * @param handler  Shared LLMAQLHandler used for LLM inference.
     * @param config   Agent configuration.
     */
    explicit ReActAgent(
        std::shared_ptr<LLMAQLHandler> handler,
        const AgentConfig& config = AgentConfig{}
    );

    ~ReActAgent() override;

    // Move-only (Pimpl pattern)
    ReActAgent(const ReActAgent&)            = delete;
    ReActAgent& operator=(const ReActAgent&) = delete;
    ReActAgent(ReActAgent&&)                 noexcept;
    ReActAgent& operator=(ReActAgent&&)      noexcept;

    // IAgent interface
    AgentResult execute(
        const std::string& task,
        const json& context = json::object()
    ) override;

    void registerTool(const AgentTool& tool) override;
    void removeTool(const std::string& name) override;
    std::vector<AgentTool> getTools() const override;
    bool hasTool(const std::string& name) const override;

    /// Update configuration without rebuilding the agent.
    void setConfig(const AgentConfig& config);

    /// Return a read-only reference to the current configuration.
    const AgentConfig& getConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
