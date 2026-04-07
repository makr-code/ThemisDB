#pragma once

#include "plugins/image_generation_interface.h"
#include "stable_diffusion/sd_config.h"
#include "stable_diffusion/sd_generator.h"
#include "stable_diffusion/sd_prompt_sanitizer.h"
#include <memory>
#include <atomic>
#include <nlohmann/json.hpp>

namespace themis {
namespace imggen {

/**
 * @brief Top-level image-generation plugin implementing IImageGenerationBackend.
 *
 * Wires together:
 *  - ISDGenerator       (model inference – injected or SDStubGenerator)
 *  - SDPromptSanitizer  (content policy)
 *
 * All generated images receive mandatory provenance fields:
 *  - generation_timestamp (Unix epoch ms)
 *  - prompt_hash          (SHA-256 hex of the sanitised prompt)
 *  - plugin_version       "2.0.0"
 */
class SDPlugin : public IImageGenerationBackend {
public:
    /** Default constructor – uses SDStubGenerator (no model required). */
    SDPlugin();

    /** Injection constructor for tests. */
    SDPlugin(std::unique_ptr<ISDGenerator> generator, SDPromptSanitizer sanitizer);

    ~SDPlugin() override = default;

    // ── IImageGenerationBackend ────────────────────────────────────────────
    bool initialize(const std::string& model_path,
                    const nlohmann::json& config) override;

    bool isInitialized() const override { return initialized_; }

    GeneratedImage generate(const std::string& prompt,
                            const SDGenerationConfig& cfg) override;

    bool isPromptAllowed(const std::string& prompt) const override;

    std::string getModelId() const override;
    std::string getPluginVersion() const override { return "2.0.0"; }
    nlohmann::json getStatistics() const override;

private:
    std::unique_ptr<ISDGenerator> generator_;
    SDPromptSanitizer             sanitizer_;
    bool     initialized_ = false;
    uint64_t generation_count_ = 0;
    uint64_t blocked_count_    = 0;
    uint64_t error_count_      = 0;
    std::string model_path_;

    static std::string sha256Hex(const std::string& input);
    static std::vector<uint8_t> encodeMinimalPng(const std::vector<uint8_t>& rgb,
                                                  int width, int height);
};

} // namespace imggen
} // namespace themis

// Export symbols for dynamic loading
THEMIS_IMGGEN_PLUGIN();
