#pragma once

#include "llm/llm_plugin_interface.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

namespace themis {
namespace llamacpp {

using json = nlohmann::json;

/**
 * @brief Standalone llama_cpp plugin that wraps the existing LlamaWrapper
 *        and exposes it through the ILLMPlugin interface for dynamic loading.
 *
 * This plugin bridges the existing LlamaWrapper implementation with the
 * plugin system. It can be loaded dynamically via THEMIS_LLM_PLUGIN() and
 * registered in the PluginManager.
 *
 * Usage without model (test/stub mode):
 *   - initialize() returns true without loading any model
 *   - generate() returns an empty InferenceResponse
 *
 * Usage with model:
 *   - initialize() calls loadModel() with model_path from config
 */
class LlamaCppPlugin : public llm::ILLMPlugin {
public:
    LlamaCppPlugin();
    ~LlamaCppPlugin() override;

    // ── ILLMPlugin ────────────────────────────────────────────────────────
    bool loadModel(const std::string& model_path, const json& config) override;
    void unloadModel() override;
    std::optional<llm::ModelInfo> getModelInfo() const override;
    bool isModelLoaded() const override { return model_loaded_; }

    bool loadLoRA(const std::string& lora_path, const std::string& lora_id,
                  float scale) override;
    bool unloadLoRA(const std::string& lora_id) override;
    std::vector<llm::LoRAInfo> listLoRAs() const override;

    llm::InferenceResponse generate(const llm::InferenceRequest& request) override;
    llm::InferenceResponse generateRAG(const llm::InferenceRequest& request,
                                        const std::vector<std::string>& context_docs) override;
    std::vector<float> embed(const std::string& text) override;

    llm::LLMCapabilities getCapabilities() const override;
    json getMemoryStats() const override;
    json getPerformanceStats() const override;

    std::vector<uint8_t> exportLoRA(const std::string& lora_id) override;
    bool importLoRA(const std::string& lora_data_base64,
                    const std::string& lora_id) override;

    std::string getPluginVersion() const { return "2.0.0"; }
    std::string getModelId() const;

private:
    bool        model_loaded_ = false;
    std::string model_path_;
    std::string model_id_;
    size_t      context_length_ = 4096; ///< Model context window (tokens); set by loadModel()
    mutable std::mutex mutex_;

    uint64_t inference_count_   = 0;
    uint64_t error_count_       = 0;

    struct LoRAEntry {
        std::string id;
        std::string path;
        float       scale = 1.0f;
    };
    std::vector<LoRAEntry> loras_;
};

} // namespace llamacpp
} // namespace themis

// Export macro for dynamic loading
THEMIS_LLM_PLUGIN();
