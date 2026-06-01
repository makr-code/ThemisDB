/*
 * ThemisDB | File: sd_generator.h | Version: 0.0.10
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=9; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "plugins/image_generation_interface.h"
#include "stable_diffusion/sd_config.h"
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

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

    std::vector<uint8_t> generateImg2Img(const std::string& prompt,
                                          const Img2ImgConfig& cfg,
                                          int& out_w, int& out_h,
                                          uint64_t& out_seed) override {
        // Stub pass-through: return the input image unchanged.
        // If no valid input is provided, fall back to text-to-image.
        if (!cfg.input_image_rgb.empty()
                && cfg.input_width  > 0
                && cfg.input_height > 0) {
            out_w    = cfg.input_width;
            out_h    = cfg.input_height;
            out_seed = (cfg.seed < 0) ? 0u : static_cast<uint64_t>(cfg.seed);
            return cfg.input_image_rgb;
        }
        return generate(prompt, cfg, out_w, out_h, out_seed);
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

// ---------------------------------------------------------------------------
// SDCppGenerator – real stable-diffusion.cpp inference backend
// Compiled only when THEMIS_ENABLE_STABLE_DIFFUSION is defined (i.e. the
// stable-diffusion.cpp library was found and linked at build time).
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_STABLE_DIFFUSION
#include "stable-diffusion.h"  // NOLINT: from stable-diffusion.cpp submodule

/**
 * @brief Generator that delegates to the stable-diffusion.cpp C API.
 *
 * initialize() loads the GGUF/safetensors model via new_sd_ctx().
 * generate() calls txt2img(); generateImg2Img() calls img2img().
 * The ctx_ pointer is owned and freed by the destructor.
 */
class SDCppGenerator : public ISDGenerator {
public:
    ~SDCppGenerator() override {
        if (ctx_) {
            free_sd_ctx(ctx_);
            ctx_ = nullptr;
        }
    }

    bool initialize(const SDConfig& cfg) override {
        if (ctx_) {
            free_sd_ctx(ctx_);
            ctx_ = nullptr;
        }
        model_id_ = cfg.model_path;
        config_   = cfg;

        ctx_ = new_sd_ctx(
            cfg.model_path.c_str(),
            /*vae_path=*/"",
            /*taesd_path=*/"",
            /*control_net_path=*/"",
            /*lora_model_dir=*/"",
            /*embed_dir=*/"",
            /*stacked_id_embed_dir=*/"",
            /*vae_decode_only=*/true,
            /*vae_tiling=*/false,
            /*free_params_immediately=*/false,
            /*n_threads=*/-1,
            /*wtype=*/SD_TYPE_F32,
            /*rng_type=*/STD_DEFAULT_RNG,
            /*s=*/DEFAULT,
            /*keep_clip_on_cpu=*/false,
            /*keep_control_net_cpu=*/false,
            /*keep_vae_on_cpu=*/false
        );
        initialized_ = (ctx_ != nullptr);
        return initialized_;
    }

    bool isInitialized() const override { return initialized_; }

    std::vector<uint8_t> generate(const std::string& prompt,
                                   const SDGenerationConfig& cfg,
                                   int& out_w, int& out_h,
                                   uint64_t& out_seed) override {
        if (!ctx_)
            throw std::runtime_error("SDCppGenerator: not initialized");

        const int64_t actual_seed = (cfg.seed < 0)
            ? static_cast<int64_t>(
                  std::chrono::steady_clock::now().time_since_epoch().count()
                  & 0x7FFFFFFF)
            : cfg.seed;

        sd_image_t* images = txt2img(
            ctx_,
            prompt.c_str(),
            cfg.negative_prompt.c_str(),
            /*clip_skip=*/-1,
            cfg.cfg_scale,
            cfg.width,
            cfg.height,
            samplerFromString(cfg.sampler),
            cfg.steps,
            actual_seed,
            /*batch_count=*/1,
            /*control_cond=*/nullptr,
            /*control_strength=*/0.9f,
            /*style_strength=*/20.0f,
            /*normalize_input=*/false,
            /*input_id_images_path=*/""
        );

        if (!images || !images[0].data) {
            if (images) std::free(images);
            throw std::runtime_error("SDCppGenerator: txt2img returned null image");
        }

        out_w    = static_cast<int>(images[0].width);
        out_h    = static_cast<int>(images[0].height);
        out_seed = static_cast<uint64_t>(actual_seed);

        const size_t n = static_cast<size_t>(out_w) * out_h * images[0].channel;
        std::vector<uint8_t> rgb(images[0].data, images[0].data + n);
        std::free(images[0].data);
        std::free(images);
        return rgb;
    }

    std::vector<uint8_t> generateImg2Img(const std::string& prompt,
                                          const Img2ImgConfig& cfg,
                                          int& out_w, int& out_h,
                                          uint64_t& out_seed) override {
        if (!ctx_)
            throw std::runtime_error("SDCppGenerator: not initialized");

        const int64_t actual_seed = (cfg.seed < 0)
            ? static_cast<int64_t>(
                  std::chrono::steady_clock::now().time_since_epoch().count()
                  & 0x7FFFFFFF)
            : cfg.seed;

        // The stable-diffusion.cpp img2img() takes the input image by value;
        // we keep a local copy so the data pointer stays valid for the call.
        std::vector<uint8_t> input_copy = cfg.input_image_rgb;
        sd_image_t input_img{
            static_cast<uint32_t>(cfg.input_width),
            static_cast<uint32_t>(cfg.input_height),
            3u,
            input_copy.data()
        };

        const int out_width  = (cfg.width  > 0) ? cfg.width  : cfg.input_width;
        const int out_height = (cfg.height > 0) ? cfg.height : cfg.input_height;

        sd_image_t* images = img2img(
            ctx_,
            input_img,
            prompt.c_str(),
            cfg.negative_prompt.c_str(),
            /*clip_skip=*/-1,
            cfg.cfg_scale,
            out_width,
            out_height,
            samplerFromString(cfg.sampler),
            cfg.steps,
            cfg.strength,
            actual_seed,
            /*batch_count=*/1,
            /*control_cond=*/nullptr,
            /*control_strength=*/0.9f,
            /*style_strength=*/20.0f,
            /*normalize_input=*/false,
            /*input_id_images_path=*/""
        );

        if (!images || !images[0].data) {
            if (images) std::free(images);
            throw std::runtime_error("SDCppGenerator: img2img returned null image");
        }

        out_w    = static_cast<int>(images[0].width);
        out_h    = static_cast<int>(images[0].height);
        out_seed = static_cast<uint64_t>(actual_seed);

        const size_t n = static_cast<size_t>(out_w) * out_h * images[0].channel;
        std::vector<uint8_t> rgb(images[0].data, images[0].data + n);
        std::free(images[0].data);
        std::free(images);
        return rgb;
    }

    std::string getModelId() const override { return model_id_; }

private:
    static sample_method_t samplerFromString(const std::string& s) {
        if (s == "euler")       return EULER;
        if (s == "euler_a")     return EULER_A;
        if (s == "heun")        return HEUN;
        if (s == "dpm2")        return DPM2;
        if (s == "dpm++2s_a")   return DPMPP2S_A;
        if (s == "dpm++2m")     return DPMPP2M;
        if (s == "dpm++2mv2")   return DPMPP2Mv2;
        if (s == "lcm")         return LCM;
        return EULER_A;  // default
    }

    sd_ctx_t*   ctx_         = nullptr;
    bool        initialized_ = false;
    std::string model_id_;
    SDConfig    config_;
};
#endif  // THEMIS_ENABLE_STABLE_DIFFUSION

} // namespace imggen
} // namespace themis
