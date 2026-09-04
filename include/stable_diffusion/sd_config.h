/**
 * @file sd_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace themis {
namespace imggen {

using json = nlohmann::json;

/**
 * @brief Runtime configuration for the Stable Diffusion image generation plugin.
 */
struct SDConfig {
    std::string model_path = {};
    int         width = 512;
    int         height = 512;
    int         steps = 20;
    float       cfg_scale = 7.0f;
    std::string sampler = "euler_a";
    int64_t     seed = -1;           // -1 = random
    std::string blocked_keywords_file;  // path to YAML with blocked prompt keywords
    std::string negative_prompt;
    std::string model_sha256;           // optional expected SHA-256 for model_path

    static SDConfig fromJson(const json& j);
    json toJson() const;
};

} // namespace imggen
} // namespace themis
