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

    std::string getModelId() const override { return model_id_; }

private:
    bool               initialized_ = false;
    std::string        model_id_ = "inmemory";
    std::vector<uint8_t> next_px_;
    int                next_w_ = 512, next_h_ = 512;
    uint64_t           next_seed_ = 42;
};

} // namespace imggen
} // namespace themis
