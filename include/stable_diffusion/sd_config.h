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
    std::string model_path;
    int         width = 512;
    int         height = 512;
    int         steps = 20;
    float       cfg_scale = 7.0f;
    std::string sampler = "euler_a";
    int64_t     seed = -1;           // -1 = random
    std::string blocked_keywords_file;  // path to YAML with blocked prompt keywords
    std::string negative_prompt;

    static SDConfig fromJson(const json& j);
    json toJson() const;
};

} // namespace imggen
} // namespace themis
