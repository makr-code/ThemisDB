/**
 * @file llm_process_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

/** @brief Illm process adapter component. */
class ILLMProcessAdapter {
public:
    /**
     * @brief TBD: Describe ~ILLMProcessAdapter.
     * @return Return value.
     */
    virtual ~ILLMProcessAdapter() = default;
    virtual ProcessDescriptor generateDescriptor(
        const std::string& activity_id,
        const std::string& activity_name,
        const std::string& process_context = "") = 0;
    virtual std::vector<ProcessDescriptor> generateBatch(
        const std::vector<std::pair<std::string, std::string>>& activity_name_pairs,
        const std::string& process_context = "") = 0;
    /**
     * @brief TBD: Describe activeBackend.
     * @return Return value.
     */
    virtual LLMDescriptorBackend activeBackend() const = 0;
    /**
     * @brief TBD: Describe isAvailable.
     * @return True on success.
     */
    virtual bool isAvailable() const = 0;
};

}} // namespace themis::process
