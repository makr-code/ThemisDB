/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sd_config.cpp                                      ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-15 04:20:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     60                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 1e348484ec  2026-04-07  feat(plugins): add stable_diffusion + llama_cpp plugins, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
        {"negative_prompt",       negative_prompt}
    };
}

} // namespace imggen
} // namespace themis
