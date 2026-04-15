/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sd_plugin.h                                        ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 18:47:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     124                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 75af53c598  2026-04-11  feat(stable_diffusion): v2.1.0 — batch generation, img2im... ║
    • 1e348484ec  2026-04-07  feat(plugins): add stable_diffusion + llama_cpp plugins, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "plugins/image_generation_interface.h"
#include "stable_diffusion/sd_config.h"
#include "stable_diffusion/sd_generator.h"
#include "stable_diffusion/sd_prompt_sanitizer.h"
#include <memory>
#include <mutex>
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
 *  - plugin_version       "2.1.0"
 *
 * Thread safety: generate(), generateBatch(), and generateImg2Img() are
 * serialised by an internal mutex.  Statistics counters are updated atomically
 * under the same lock.
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

    /**
     * @brief Generate multiple images from a list of prompts (batch mode).
     *
     * Each prompt is screened independently by SDPromptSanitizer.
     * Results are returned in the same order as the input prompts.
     */
    std::vector<GeneratedImage> generateBatch(
            const std::vector<std::string>& prompts,
            const SDGenerationConfig& cfg) override;

    /**
     * @brief Modify an existing image guided by a text prompt (img2img).
     *
     * In v2.1.0 the stub/in-memory generators ignore the input image and
     * produce a normal generate() result.  SDCppGenerator will override this
     * with real denoising in a future release.
     *
     * Both the positive prompt and the negative_prompt in cfg are screened
     * by SDPromptSanitizer before reaching the generator.
     */
    GeneratedImage generateImg2Img(const std::string& prompt,
                                   const Img2ImgConfig& cfg) override;

    bool isPromptAllowed(const std::string& prompt) const override;

    std::string getModelId() const override;
    std::string getPluginVersion() const override { return "2.1.0"; }
    nlohmann::json getStatistics() const override;

private:
    mutable std::mutex            generate_mutex_;
    std::unique_ptr<ISDGenerator> generator_;
    SDPromptSanitizer             sanitizer_;
    bool     initialized_ = false;
    uint64_t generation_count_ = 0;
    uint64_t blocked_count_    = 0;
    uint64_t error_count_      = 0;
    std::string model_path_;

    // Internal helper: run generate without locking (caller holds generate_mutex_)
    GeneratedImage generateLocked(const std::string& prompt,
                                   const SDGenerationConfig& cfg);

    static std::string sha256Hex(const std::string& input);
    static std::vector<uint8_t> encodeMinimalPng(const std::vector<uint8_t>& rgb,
                                                  int width, int height);
};

} // namespace imggen
} // namespace themis

// Export symbols for dynamic loading
THEMIS_IMGGEN_PLUGIN();
