#pragma once

#include "plugins/plugin_interface.h"
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace imggen {

using json = nlohmann::json;

/**
 * @brief Result of an image generation operation.
 *
 * Mandatory provenance fields allow auditing of every generated image
 * back to the model, sampler and configuration used.
 */
struct GeneratedImage {
    std::vector<uint8_t> png_data;          // Encoded PNG bytes
    int         width = 0;
    int         height = 0;
    std::string model_id;
    std::string plugin_version;
    std::string sampler;
    uint64_t    seed_used = 0;
    int64_t     generation_timestamp = 0;   // Unix epoch milliseconds; mandatory
    std::string prompt_hash;                // SHA-256 hex of the sanitised prompt
    bool        success = true;
    std::string error_message;
};

/**
 * @brief Per-request configuration for image generation.
 */
struct SDGenerationConfig {
    int     width = 512;
    int     height = 512;
    int     steps = 20;
    float   cfg_scale = 7.0f;
    std::string sampler = "euler_a";
    int64_t seed = -1;               // -1 = random
    std::string negative_prompt;
};

/**
 * @brief Pure-virtual interface for image-generation backends.
 *
 * Implementations: SDPlugin (stable-diffusion.cpp), stub/test doubles.
 */
class IImageGenerationBackend {
public:
    virtual ~IImageGenerationBackend() = default;

    /**
     * @brief Load model and apply configuration.
     * @param model_path  Path to the GGUF/safetensors model file (empty for stub).
     * @param config      JSON configuration (see sd_config.h).
     */
    virtual bool initialize(const std::string& model_path, const json& config) = 0;

    virtual bool isInitialized() const = 0;

    /**
     * @brief Generate an image from a text prompt.
     *
     * Implementations MUST check isPromptAllowed() before running inference.
     * If the prompt is blocked, they should return a GeneratedImage with
     * success=false and an appropriate error_message.
     */
    virtual GeneratedImage generate(const std::string& prompt,
                                    const SDGenerationConfig& cfg) = 0;

    /**
     * @brief Content-policy check.  Returns false for blocked prompts.
     */
    virtual bool isPromptAllowed(const std::string& prompt) const = 0;

    virtual std::string getModelId() const = 0;
    virtual std::string getPluginVersion() const = 0;
    virtual json        getStatistics() const = 0;
};

} // namespace imggen
} // namespace themis

/**
 * @brief Export macro for dynamic loading of image-generation plugins.
 *
 * Add this macro once in the .cpp file of your image-generation plugin.
 */
#define THEMIS_IMGGEN_PLUGIN()                                                              \
    extern "C" THEMIS_PLUGIN_EXPORT                                                        \
        themis::imggen::IImageGenerationBackend* themis_imggen_create();                   \
    extern "C" THEMIS_PLUGIN_EXPORT                                                        \
        void themis_imggen_destroy(themis::imggen::IImageGenerationBackend* p)
