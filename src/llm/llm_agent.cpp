#include "llm/llm_agent.h"
#include <chrono>
#include <random>
#include <sstream>

namespace themis {
namespace llm {

// AgentConfig methods
nlohmann::json LLMAgent::AgentConfig::toJson() const {
    return nlohmann::json{
        {"agent_id", agent_id},
        {"role", role},
        {"lora_adapter_id", lora_adapter_id},
        {"base_model", base_model},
        {"max_context_length", max_context_length},
        {"temperature", temperature},
        {"role_instructions", role_instructions},
        {"metadata", metadata}
    };
}

LLMAgent::AgentConfig LLMAgent::AgentConfig::fromJson(const nlohmann::json& j) {
    AgentConfig config;
    config.agent_id = j.value("agent_id", "");
    config.role = j.value("role", "");
    config.lora_adapter_id = j.value("lora_adapter_id", "");
    config.base_model = j.value("base_model", "mistral-7b");
    config.max_context_length = j.value("max_context_length", 4096);
    config.temperature = j.value("temperature", 0.7f);
    config.role_instructions = j.value("role_instructions", nlohmann::json::object());
    config.metadata = j.value("metadata", nlohmann::json::object());
    return config;
}

// AgentRequest methods
nlohmann::json LLMAgent::AgentRequest::toJson() const {
    return nlohmann::json{
        {"prompt", prompt},
        {"context", context},
        {"peer_responses", peer_responses},
        {"max_tokens", max_tokens},
        {"temperature", temperature}
    };
}

LLMAgent::AgentRequest LLMAgent::AgentRequest::fromJson(const nlohmann::json& j) {
    AgentRequest request;
    request.prompt = j.value("prompt", "");
    request.context = j.value("context", nlohmann::json::object());
    request.peer_responses = j.value("peer_responses", std::vector<std::string>{});
    request.max_tokens = j.value("max_tokens", 2048);
    request.temperature = j.value("temperature", 0.7f);
    return request;
}

// AgentResult methods
nlohmann::json LLMAgent::AgentResult::toJson() const {
    return nlohmann::json{
        {"response", response},
        {"reasoning_steps", reasoning_steps},
        {"confidence", confidence},
        {"metadata", metadata},
        {"token_count", token_count},
        {"latency_ms", latency_ms}
    };
}

LLMAgent::AgentResult LLMAgent::AgentResult::fromJson(const nlohmann::json& j) {
    AgentResult result;
    result.response = j.value("response", "");
    result.reasoning_steps = j.value("reasoning_steps", std::vector<std::string>{});
    result.confidence = j.value("confidence", 0.0f);
    result.metadata = j.value("metadata", nlohmann::json::object());
    result.token_count = j.value("token_count", 0);
    result.latency_ms = j.value("latency_ms", 0);
    return result;
}

// Constructor
LLMAgent::LLMAgent(const AgentConfig& config, rocksdb::TransactionDB* db)
    : config_(config), db_(db) {
}

// Main processing method
LLMAgent::AgentResult LLMAgent::processRequest(const AgentRequest& request) {
    auto start_time = std::chrono::steady_clock::now();
    
    // Build full prompt with system instructions and context
    std::string full_prompt = formatPromptWithContext(request);
    
    // Generate response (stub - will use llama.cpp in v1.5.0)
    std::string response = generateResponse(full_prompt, request);
    
    // Extract reasoning steps
    auto reasoning_steps = extractReasoningSteps(response);
    
    // Estimate confidence
    float confidence = estimateConfidence(response);
    
    // Calculate latency and tokens
    auto end_time = std::chrono::steady_clock::now();
    int latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    ).count();
    
    int token_count = response.length() / 4; // Rough estimate: 1 token ≈ 4 chars
    
    // Update statistics
    total_requests_++;
    total_tokens_ += token_count;
    total_latency_ms_ += latency_ms;
    
    AgentResult result;
    result.response = response;
    result.reasoning_steps = reasoning_steps;
    result.confidence = confidence;
    result.token_count = token_count;
    result.latency_ms = latency_ms;
    result.metadata = nlohmann::json{
        {"agent_id", config_.agent_id},
        {"role", config_.role},
        {"lora_adapter", config_.lora_adapter_id},
        {"timestamp_ms", getCurrentTimestampMs()}
    };
    
    return result;
}

// Validation
bool LLMAgent::validateResponse(const std::string& response) const {
    // Basic validation rules
    if (response.empty()) return false;
    if (response.length() < 10) return false;
    
    // Check for minimum quality indicators
    // (In production, this would be more sophisticated)
    return true;
}

// Configuration update
void LLMAgent::updateConfig(const AgentConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
}

// Statistics
nlohmann::json LLMAgent::getStats() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    size_t requests = total_requests_.load();
    double avg_latency = requests > 0 
        ? static_cast<double>(total_latency_ms_.load()) / requests
        : 0.0;
    
    return nlohmann::json{
        {"agent_id", config_.agent_id},
        {"role", config_.role},
        {"total_requests", requests},
        {"total_tokens", total_tokens_.load()},
        {"avg_latency_ms", avg_latency},
        {"lora_adapter", config_.lora_adapter_id}
    };
}

// Private helper methods
std::string LLMAgent::buildSystemPrompt() const {
    std::stringstream ss;
    
    // Add role-specific instructions
    if (config_.role_instructions.contains("system_prompt")) {
        ss << config_.role_instructions["system_prompt"].get<std::string>();
    } else {
        ss << "You are a " << config_.role << " agent.";
    }
    
    return ss.str();
}

std::string LLMAgent::formatPromptWithContext(const AgentRequest& request) const {
    std::stringstream ss;
    
    // System prompt
    ss << buildSystemPrompt() << "\n\n";
    
    // Add context if provided
    if (!request.context.empty()) {
        ss << "Context:\n" << request.context.dump(2) << "\n\n";
    }
    
    // Add peer responses for iterative refinement
    if (!request.peer_responses.empty()) {
        ss << "Previous responses from other agents:\n";
        for (size_t i = 0; i < request.peer_responses.size(); i++) {
            ss << "Agent " << (i+1) << ": " << request.peer_responses[i] << "\n";
        }
        ss << "\n";
    }
    
    // Main prompt
    ss << "Task:\n" << request.prompt << "\n\n";
    ss << "Response:";
    
    return ss.str();
}

std::vector<std::string> LLMAgent::extractReasoningSteps(const std::string& response) const {
    std::vector<std::string> steps;
    
    // Simple extraction: look for numbered steps or "Step X:" patterns
    // In production, this would use more sophisticated NLP
    std::istringstream iss(response);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.find("Step ") != std::string::npos ||
            line.find("1.") != std::string::npos ||
            line.find("2.") != std::string::npos) {
            steps.push_back(line);
        }
    }
    
    return steps;
}

float LLMAgent::estimateConfidence(const std::string& response) const {
    // Stub confidence estimation
    // In production, this would use:
    // - Response length
    // - Presence of hedging words ("maybe", "possibly")
    // - Model's logprobs
    // - Consistency checks
    
    float confidence = 0.8f; // Default confidence
    
    // Penalize very short responses
    if (response.length() < 50) {
        confidence -= 0.2f;
    }
    
    // Penalize uncertainty markers
    if (response.find("unsure") != std::string::npos ||
        response.find("unclear") != std::string::npos) {
        confidence -= 0.1f;
    }
    
    return std::max(0.1f, std::min(1.0f, confidence));
}

std::string LLMAgent::generateResponse(const std::string& prompt, const AgentRequest& request) {
    // STUB IMPLEMENTATION
    // In v1.5.0, this will use llama.cpp for actual inference
    
    std::stringstream ss;
    ss << "[" << config_.role << " analysis]\n\n";
    ss << "As a " << config_.role << ", I have analyzed the request.\n\n";
    
    // Add role-specific response template
    if (config_.role == "legal_expert") {
        ss << "Legal Considerations:\n";
        ss << "1. Compliance aspects need review\n";
        ss << "2. Risk factors should be assessed\n";
        ss << "3. Contractual terms require attention\n";
    } else if (config_.role == "technical_analyst") {
        ss << "Technical Analysis:\n";
        ss << "1. Architecture considerations\n";
        ss << "2. Performance implications\n";
        ss << "3. Implementation feasibility\n";
    } else if (config_.role == "business_strategist") {
        ss << "Business Analysis:\n";
        ss << "1. Strategic alignment\n";
        ss << "2. ROI considerations\n";
        ss << "3. Market positioning\n";
    } else {
        ss << "Analysis:\n";
        ss << "1. Key considerations identified\n";
        ss << "2. Recommendations developed\n";
        ss << "3. Action items proposed\n";
    }
    
    ss << "\n[Note: This is a stub response. Full LLM integration coming in v1.5.0]";
    
    return ss.str();
}

int64_t LLMAgent::getCurrentTimestampMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace llm
} // namespace themis
