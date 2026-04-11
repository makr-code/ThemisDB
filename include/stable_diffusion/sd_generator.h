#pragma once

#include "plugins/image_generation_interface.h"
#include "stable_diffusion/sd_config.h"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace themis {
namespace imggen {

/**
 * @brief Interface for the core image generation engine.
 *
 * Separates model-loading / inference from the plugin lifecycle so that
 * test doubles can be injected without linking stable-diffusion.cpp.
 */
class ISDGenerator {
public:
    virtual ~ISDGenerator() = default;

    virtual bool initialize(const SDConfig& cfg) = 0;
    virtual bool isInitialized() const = 0;

    /**
     * @brief Generate raw RGB pixel data for a given prompt.
     * @param prompt     Sanitized text prompt.
     * @param cfg        Per-request generation config.
     * @param out_width  Actual width of the generated image.
     * @param out_height Actual height of the generated image.
     * @param out_seed   Seed that was actually used (important for -1 random seeds).
     * @return Raw RGB byte buffer (width * height * 3 bytes).
     */
    virtual std::vector<uint8_t> generate(const std::string& prompt,
                                          const SDGenerationConfig& cfg,
                                          int& out_width, int& out_height,
                                          uint64_t& out_seed) = 0;

    /**
     * @brief Img2Img: generate raw RGB pixel data conditioned on an input image.
     *
     * Default implementation ignores the input image and delegates to generate().
     * SDCppGenerator will override this with real denoising inference.
     *
     * @param prompt         Sanitized text prompt.
     * @param cfg            Img2Img config (includes input_image_rgb, strength, mask).
     * @param out_width      Actual width of the generated image.
     * @param out_height     Actual height of the generated image.
     * @param out_seed       Seed actually used.
     * @return Raw RGB byte buffer (width * height * 3 bytes).
     */
    virtual std::vector<uint8_t> generateImg2Img(const std::string& prompt,
                                                  const Img2ImgConfig& cfg,
                                                  int& out_width, int& out_height,
                                                  uint64_t& out_seed) {
        // Default: fall back to regular text-to-image
        return generate(prompt, cfg, out_width, out_height, out_seed);
    }

    virtual std::string getModelId() const = 0;
};

// ---------------------------------------------------------------------------
// Stub generator – used when stable-diffusion.cpp is not linked
// ---------------------------------------------------------------------------

/**
 * @brief Stub generator that returns a solid-black 1×1 PNG without a model.
 *
 * Used in CI builds that do not have a model file.
 */
class SDStubGenerator : public ISDGenerator {
public:
    bool initialize(const SDConfig& cfg) override {
        model_id_    = cfg.model_path.empty() ? "stub" : cfg.model_path;
        initialized_ = true;
        return true;
    }
    bool isInitialized() const override { return initialized_; }

    std::vector<uint8_t> generate(const std::string&,
                                  const SDGenerationConfig& cfg,
                                  int& out_w, int& out_h,
                                  uint64_t& out_seed) override {
        out_w    = cfg.width;
        out_h    = cfg.height;
        out_seed = (cfg.seed < 0) ? 0u : static_cast<uint64_t>(cfg.seed);
        // Return a stub RGB buffer (all zeros = black)
        return std::vector<uint8_t>(static_cast<size_t>(cfg.width) * cfg.height * 3, 0);
    }

    std::string getModelId() const override { return model_id_; }

private:
    bool        initialized_ = false;
    std::string model_id_ = "stub";
};

// ---------------------------------------------------------------------------
// Test double
// ---------------------------------------------------------------------------

/**
 * @brief In-memory generator for unit tests.
 *
 * Callers pre-set the next pixel buffer and dimensions.
 * Supports both text-to-image and img2img paths; img2img records whether it
 * was invoked so tests can verify the correct path was taken.
 */
class InMemorySDGenerator : public ISDGenerator {
public:
    void setNextPixels(std::vector<uint8_t> px, int w, int h, uint64_t seed = 42) {
        next_px_   = std::move(px);
        next_w_    = w;
        next_h_    = h;
        next_seed_ = seed;
        initialized_ = true;
    }

    bool initialize(const SDConfig& cfg) override {
        model_id_    = cfg.model_path.empty() ? "inmemory" : cfg.model_path;
        initialized_ = true;
        return true;
    }
    bool isInitialized() const override { return initialized_; }

    std::vector<uint8_t> generate(const std::string&,
                                  const SDGenerationConfig&,
                                  int& out_w, int& out_h,
                                  uint64_t& out_seed) override {
        out_w    = next_w_;
        out_h    = next_h_;
        out_seed = next_seed_;
        return next_px_;
    }

    std::vector<uint8_t> generateImg2Img(const std::string& prompt,
                                          const Img2ImgConfig& cfg,
                                          int& out_w, int& out_h,
                                          uint64_t& out_seed) override {
        last_img2img_strength_ = cfg.strength;
        last_img2img_input_w_  = cfg.input_width;
        last_img2img_input_h_  = cfg.input_height;
        img2img_called_ = true;
        return generate(prompt, cfg, out_w, out_h, out_seed);
    }

    // Test inspection helpers
    bool   img2imgCalled()          const { return img2img_called_; }
    float  lastImg2ImgStrength()    const { return last_img2img_strength_; }
    int    lastImg2ImgInputWidth()  const { return last_img2img_input_w_; }
    int    lastImg2ImgInputHeight() const { return last_img2img_input_h_; }

    std::string getModelId() const override { return model_id_; }

private:
    bool               initialized_ = false;
    std::string        model_id_ = "inmemory";
    std::vector<uint8_t> next_px_;
    int                next_w_ = 512, next_h_ = 512;
    uint64_t           next_seed_ = 42;

    bool  img2img_called_           = false;
    float last_img2img_strength_    = 0.0f;
    int   last_img2img_input_w_     = 0;
    int   last_img2img_input_h_     = 0;
};

} // namespace imggen
} // namespace themis
