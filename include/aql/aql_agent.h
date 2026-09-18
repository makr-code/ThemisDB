/**
 * @file aql_agent.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

struct AgentTool {
    std::string name;

    std::string description;

    json parameter_schema;

    std::function<json(const json& args)> executor;
};

struct AgentConfig {
    std::string model_alias;

    int max_iterations = 10;

    float temperature = 0.3f;

    int max_tokens_per_step = 512;

    bool verbose = false;
};

struct ReasoningStep {
    std::string thought;

    std::optional<std::string> tool_name;

    std::optional<json> tool_input;

    std::optional<json> tool_output;

    std::string observation;
};

struct AgentResult {
    std::string final_answer;

    std::vector<ReasoningStep> reasoning_trace;

    int iterations_used = 0;

    bool succeeded = false;
};

// ============================================================================
// IAgent – abstract interface
// ============================================================================

class IAgent {
public:
    /**
     * @brief IAgent.
     * @return Return value.
     */
    virtual ~IAgent() = default;

    virtual AgentResult execute(
        const std::string& task,
        const json& context = json::object()
    ) = 0;

    /**
     * @brief Register Tool.
     * @param[in] tool Input parameter.
     */
    virtual void registerTool(const AgentTool& tool) = 0;

    /**
     * @brief Remove Tool.
     * @param[in] name Input parameter.
     */
    virtual void removeTool(const std::string& name) = 0;

    /**
     * @brief Get Tools.
     * @return Return value.
     */
    virtual std::vector<AgentTool> getTools() const = 0;

    /**
     * @brief Has Tool.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool hasTool(const std::string& name) const = 0;
};

// ============================================================================
// ReActAgent – concrete ReAct implementation
// ============================================================================

class ReActAgent : public IAgent {
public:
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

    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     */
    void setConfig(const AgentConfig& config);

    /**
     * @brief Get Config.
     * @return Return value.
     */
    const AgentConfig& getConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace aql
} // namespace themis
