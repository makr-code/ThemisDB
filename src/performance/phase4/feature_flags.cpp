/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feature_flags.cpp                                  ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 07:55:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     33                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "performance/phase4/feature_flags.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace themis {
namespace performance {
namespace phase4 {

void Phase4FeatureFlags::load_from_config(const std::string& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            return; // Config file doesn't exist, use defaults
        }

        nlohmann::json config;
        file >> config;

        if (config.contains("performance") && config["performance"].contains("phase4")) {
            const auto& phase4 = config["performance"]["phase4"];

            if (phase4.contains("pmem_enabled")) {
                set_pmem_enabled(phase4["pmem_enabled"].get<bool>());
            }
        }
    } catch (const std::exception&) {
        // Ignore JSON parsing errors, keep defaults
    }
}

} // namespace phase4
} // namespace performance
} // namespace themis
