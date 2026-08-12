/**
 * @file image_generation_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "plugins/plugin_interface.h"
#include <string>
#include <vector>
#include <cstdint>
#include <optional>
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
    std::string prompt_hash;                // FNV-1a 64-bit hex fingerprint of the sanitised prompt
    std::optional<std::string> perceptual_hash; // Optional deterministic perceptual hash metadata
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
    std::vector<uint8_t> control_image_rgb; // Optional ControlNet conditioning image (RGB)
    int control_width = 0;           // Control image width; must match control_image_rgb
    int control_height = 0;          // Control image height; must match control_image_rgb
    std::string control_model_path;  // Optional ControlNet model path for this request
    float control_strength = 0.9f;   // ControlNet conditioning strength [0.0, 1.0]
    std::string lora_adapter_path;   // Optional LoRA adapter path for this request
    float lora_scale = 1.0f;         // LoRA scale multiplier
};

/**
 * @brief Configuration for image-to-image (img2img) generation.
 *
 * Extends SDGenerationConfig with an input image and denoising strength.
 */
struct Img2ImgConfig : public SDGenerationConfig {
    std::vector<uint8_t> input_image_rgb;   // Raw RGB byte buffer of the input image
    int         input_width  = 0;           // Width of the input image
    int         input_height = 0;           // Height of the input image
    float       strength = 0.75f;           // Denoising strength [0.0, 1.0]; 1.0 = fully redraw
    std::vector<uint8_t> mask_rgb;          // Optional binary mask (white = repaint); empty = no mask
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
     * @brief Generate multiple images from a list of prompts.
     *
     * Default implementation calls generate() for each prompt independently.
     * Backends may override with a parallelised implementation.
     * Content-policy screening is applied to each prompt independently.
     *
     * @param prompts  List of text prompts (one per image).
     * @param cfg      Shared generation config applied to every prompt.
     * @return         One GeneratedImage per prompt, in the same order.
     */
    virtual std::vector<GeneratedImage> generateBatch(
            const std::vector<std::string>& prompts,
            const SDGenerationConfig& cfg) {
        std::vector<GeneratedImage> results;
        results.reserve(prompts.size());
        for (const auto& p : prompts) {
            results.push_back(generate(p, cfg));
        }
        return results;
    }

    /**
     * @brief Modify an existing image guided by a text prompt (img2img).
     *
     * Default implementation falls back to generate(prompt, cfg) ignoring the
     * input image.  Backends with real model support override this method.
     *
     * @param prompt  Text prompt describing the desired output.
     * @param cfg     Img2Img config including the input image and denoising strength.
     * @return        GeneratedImage with the result.
     */
    virtual GeneratedImage generateImg2Img(const std::string& prompt,
                                           const Img2ImgConfig& cfg) {
        return generate(prompt, cfg);
    }

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
