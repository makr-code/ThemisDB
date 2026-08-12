/**
 * @file llm_process_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
// Multi-LLM descriptor adapters for process model semantic enrichment
#include <string>
#include <vector>
#include <utility>

namespace themis { namespace process {

enum class LLMDescriptorBackend { OPENAI, ANTHROPIC, GEMINI, LLAMA_CPP, THEMIS_NATIVE };

struct LLMDescriptorConfig {
    LLMDescriptorBackend backend = LLMDescriptorBackend::THEMIS_NATIVE;
    std::string model_id;
    std::string api_endpoint;
    std::string api_key_env_var;
    int max_tokens = 256;
    float temperature = 0.0f;
    std::string language = "en";
};

struct ProcessDescriptor {
    std::string activity_id;
    std::string generated_description;
    std::string suggested_role;
    std::vector<std::string> suggested_kpis;
    float confidence = 0.0f;
    LLMDescriptorBackend backend_used{LLMDescriptorBackend::THEMIS_NATIVE};
    double generation_time_ms = 0.0;
};

class ILLMProcessAdapter {
public:
    virtual ~ILLMProcessAdapter() = default;
    virtual ProcessDescriptor generateDescriptor(
        const std::string& activity_id,
        const std::string& activity_name,
        const std::string& process_context = "") = 0;
    virtual std::vector<ProcessDescriptor> generateBatch(
        const std::vector<std::pair<std::string, std::string>>& activity_name_pairs,
        const std::string& process_context = "") = 0;
    virtual LLMDescriptorBackend activeBackend() const = 0;
    virtual bool isAvailable() const = 0;
};

}} // namespace themis::process
