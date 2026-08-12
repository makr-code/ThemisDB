/**
 * @file aql_agent.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=10, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "aql/aql_agent.h"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace aql {

// ============================================================================
// Pimpl implementation
// ============================================================================

/** @brief Pimpl implementation. */
class ReActAgent::Impl {
  public:
    explicit Impl(std::shared_ptr<LLMAQLHandler> handler, const AgentConfig &config)
        : handler_(std::move(handler)), config_(config) {}

    // -----------------------------------------------------------------------
    // Tool registry
    // -----------------------------------------------------------------------

    void registerTool(const AgentTool &tool) {
        if (tool.name.empty()) {
            throw std::invalid_argument("AgentTool name must not be empty");
        }
        if (tools_.count(tool.name)) {
            throw std::invalid_argument("Tool already registered: " + tool.name);
        }
        tools_[tool.name] = tool;
    }

    void removeTool(const std::string &name) {
        auto it = tools_.find(name);
        if (it == tools_.end()) {
            throw std::invalid_argument("Tool not registered: " + name);
        }
        tools_.erase(it);
    }

    std::vector<AgentTool> getTools() const {
        std::vector<AgentTool> result;
        result.reserve(tools_.size());
        for (const auto &kv : tools_) {
            result.push_back(kv.second);
        }
        return result;
    }

    bool hasTool(const std::string &name) const {
        return tools_.count(name) > 0;
    }

    void setConfig(const AgentConfig &config) {
        config_ = config;
    }
    const AgentConfig &getConfig() const {
        return config_;
    }

    // -----------------------------------------------------------------------
    // Core execution
    // -----------------------------------------------------------------------

    AgentResult execute(const std::string &task, const json &context) {
        AgentResult result;
        result.succeeded = false;

        // Build the initial system prompt describing available tools.
        std::string system_prompt = buildSystemPrompt(context);
        // Build the initial user message.
        std::string conversation = buildInitialUserMessage(task);

        for (int iter = 0; iter < config_.max_iterations; ++iter) {
            if (config_.verbose) {
                spdlog::debug("[ReActAgent] iteration {}/{}", iter + 1, config_.max_iterations);
            }

            // Ask the LLM for the next reasoning step.
            std::string llm_input = system_prompt + "\n\n" + conversation;
            std::string raw_response;
            try {
                std::unordered_map<std::string, std::string> opts;
                opts["max_tokens"]  = std::to_string(config_.max_tokens_per_step);
                opts["temperature"] = std::to_string(config_.temperature);
                raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
                                                             /*lora_id=*/"", opts);
            } catch (const std::exception &e) {
                // LLM inference unavailable (e.g. no model loaded).
                // Record an error step and continue so the max-iterations
                // fallback synthesises final_answer without propagating.
                ReasoningStep error_step;
                error_step.thought = std::string("LLM inference unavailable: ") + e.what();
                result.reasoning_trace.push_back(error_step);
                result.iterations_used = iter + 1;
                continue;
            }

            if (config_.verbose) {
                spdlog::debug("[ReActAgent] raw_response: {}", raw_response);
            }

            // Parse the LLM response into a reasoning step.
            auto step = parseStep(raw_response);
            result.reasoning_trace.push_back(step);
            result.iterations_used = iter + 1;

            // Append the assistant's reasoning to the conversation history.
            conversation += "\nAssistant:\n" + raw_response + "\n";

            // Check for final answer.
            if (isFinalAnswerStep(raw_response)) {
                result.final_answer = extractFinalAnswer(raw_response);
                result.succeeded    = true;
                break;
            }

            // If the LLM requested a tool, invoke it.
            if (step.tool_name.has_value()) {
                json tool_output            = invokeTool(*step.tool_name, step.tool_input.value_or(json::object()));
                ReasoningStep &mutable_step = result.reasoning_trace.back();
                mutable_step.tool_output    = tool_output;
                mutable_step.observation    = "Tool returned: " + tool_output.dump();

                // Feed the tool output back into the conversation.
                conversation += "Observation: " + tool_output.dump() + "\n";
            }
        }

        // If we exhausted iterations without a final answer, synthesise one.
        if (!result.succeeded && !result.reasoning_trace.empty()) {
            result.final_answer = "Agent reached maximum iterations (" + std::to_string(config_.max_iterations)
                                  + ") without a conclusive answer.";
        }

        return result;
    }

  private:
    // -----------------------------------------------------------------------
    // Prompt construction helpers
    // -----------------------------------------------------------------------

    std::string buildSystemPrompt(const json &context) const {
        std::ostringstream oss;
        oss << "You are a helpful database assistant with access to the following tools:\n\n";

        for (const auto &kv : tools_) {
            const AgentTool &tool = kv.second;
            oss << "Tool: " << tool.name << "\n";
            oss << "Description: " << tool.description << "\n";
            oss << "Parameters: " << tool.parameter_schema.dump() << "\n\n";
        }

        oss << "To use a tool, output:\n"
               "Thought: <your reasoning>\n"
               "Action: <tool_name>\n"
               "Action Input: <JSON arguments>\n\n"
               "When you have the final answer, output:\n"
               "Thought: I now know the final answer.\n"
               "Final Answer: <your answer>\n";

        if (!context.empty()) {
            oss << "\nContext:\n" << context.dump(2) << "\n";
        }

        return oss.str();
    }

    std::string buildInitialUserMessage(const std::string &task) const {
        return "Human: " + task + "\nAssistant:";
    }

    // -----------------------------------------------------------------------
    // Response parsing helpers
    // -----------------------------------------------------------------------

    static bool isFinalAnswerStep(const std::string &response) {
        return response.find("Final Answer:") != std::string::npos;
    }

    static std::string extractFinalAnswer(const std::string &response) {
        const std::string marker = "Final Answer:";
        auto pos                 = response.find(marker);
        if (pos == std::string::npos) {
            return response;
        }
        std::string answer = response.substr(pos + marker.size());
        // Trim leading/trailing whitespace.
        auto start = answer.find_first_not_of(" \t\n\r");
        auto end   = answer.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) {
            return "";
        }
        return answer.substr(start, end - start + 1);
    }

    static ReasoningStep parseStep(const std::string &response) {
        ReasoningStep step;

        // Extract Thought:
        auto extract_field = [&](const std::string &field_prefix) -> std::string {
            auto pos = response.find(field_prefix);
            if (pos == std::string::npos) {
                return "";
            }
            pos += field_prefix.size();
            auto end = response.find('\n', pos);
            if (end == std::string::npos) {
                end = response.size();
            }
            std::string val = response.substr(pos, end - pos);
            // Trim
            auto s = val.find_first_not_of(" \t");
            auto e = val.find_last_not_of(" \t");
            if (s == std::string::npos) {
                return "";
            }
            return val.substr(s, e - s + 1);
        };

        step.thought     = extract_field("Thought:");
        step.observation = extract_field("Observation:");

        std::string action_name  = extract_field("Action:");
        std::string action_input = extract_field("Action Input:");

        if (!action_name.empty()) {
            step.tool_name = action_name;
            if (!action_input.empty()) {
                try {
                    step.tool_input = json::parse(action_input);
                } catch (const json::parse_error &) {
                    // If the input is not valid JSON, wrap it as a string argument.
                    step.tool_input = json{{"input", action_input}};
                }
            }
        }

        return step;
    }

    // -----------------------------------------------------------------------
    // Tool invocation
    // -----------------------------------------------------------------------

    json invokeTool(const std::string &name, const json &args) {
        auto it = tools_.find(name);
        if (it == tools_.end()) {
            return json{{"error", "Unknown tool: " + name}};
        }
        try {
            return it->second.executor(args);
        } catch (const std::exception &e) {
            return json{{"error", std::string("Tool execution failed: ") + e.what()}};
        }
    }

    // -----------------------------------------------------------------------
    // Members
    // -----------------------------------------------------------------------

    std::shared_ptr<LLMAQLHandler> handler_;
    AgentConfig config_;
    std::unordered_map<std::string, AgentTool> tools_;
};

// ============================================================================
// ReActAgent – public interface delegation
// ============================================================================

ReActAgent::ReActAgent(std::shared_ptr<LLMAQLHandler> handler, const AgentConfig &config)
    : impl_(std::make_unique<Impl>(std::move(handler), config)) {}

ReActAgent::~ReActAgent() = default;

ReActAgent::ReActAgent(ReActAgent &&) noexcept            = default;
ReActAgent &ReActAgent::operator=(ReActAgent &&) noexcept = default;

AgentResult ReActAgent::execute(const std::string &task, const json &context) {
    return impl_->execute(task, context);
}

void ReActAgent::registerTool(const AgentTool &tool) {
    impl_->registerTool(tool);
}

void ReActAgent::removeTool(const std::string &name) {
    impl_->removeTool(name);
}

std::vector<AgentTool> ReActAgent::getTools() const {
    return impl_->getTools();
}

bool ReActAgent::hasTool(const std::string &name) const {
    return impl_->hasTool(name);
}

void ReActAgent::setConfig(const AgentConfig &config) {
    impl_->setConfig(config);
}

const AgentConfig &ReActAgent::getConfig() const {
    return impl_->getConfig();
}

} // namespace aql
} // namespace themis
