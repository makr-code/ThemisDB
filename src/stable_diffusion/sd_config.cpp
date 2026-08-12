/**
 * @file sd_config.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "stable_diffusion/sd_config.h"

namespace themis {
namespace imggen {

SDConfig SDConfig::fromJson(const json& j) {
    SDConfig cfg;
    if (j.contains("model_path"))            cfg.model_path            = j["model_path"].get<std::string>();
    if (j.contains("width"))                 cfg.width                 = j["width"].get<int>();
    if (j.contains("height"))                cfg.height                = j["height"].get<int>();
    if (j.contains("steps"))                 cfg.steps                 = j["steps"].get<int>();
    if (j.contains("cfg_scale"))             cfg.cfg_scale             = j["cfg_scale"].get<float>();
    if (j.contains("sampler"))               cfg.sampler               = j["sampler"].get<std::string>();
    if (j.contains("seed"))                  cfg.seed                  = j["seed"].get<int64_t>();
    if (j.contains("blocked_keywords_file")) cfg.blocked_keywords_file = j["blocked_keywords_file"].get<std::string>();
    if (j.contains("negative_prompt"))       cfg.negative_prompt       = j["negative_prompt"].get<std::string>();
    if (j.contains("model_sha256"))          cfg.model_sha256          = j["model_sha256"].get<std::string>();
    if (cfg.width  < 1) cfg.width  = 512;
    if (cfg.height < 1) cfg.height = 512;
    if (cfg.steps  < 1) cfg.steps  = 1;
    return cfg;
}

json SDConfig::toJson() const {
    return {
        {"model_path",            model_path},
        {"width",                 width},
        {"height",                height},
        {"steps",                 steps},
        {"cfg_scale",             cfg_scale},
        {"sampler",               sampler},
        {"seed",                  seed},
        {"blocked_keywords_file", blocked_keywords_file},
        {"negative_prompt",       negative_prompt},
        {"model_sha256",          model_sha256}
    };
}

} // namespace imggen
} // namespace themis
