/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            phase2_feature_flags.cpp                           ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "performance/phase2_feature_flags.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace themis {
namespace performance {

void Phase2FeatureFlags::load_from_config(const std::string& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            return; // Config file doesn't exist, use defaults
        }
        
        nlohmann::json config;
        file >> config;
        
        if (config.contains("performance") && config["performance"].contains("phase2")) {
            auto phase2 = config["performance"]["phase2"];
            
            if (phase2.contains("wisckey_enabled")) {
                set_wisckey_enabled(phase2["wisckey_enabled"]);
            }
            if (phase2.contains("dostoevsky_enabled")) {
                set_dostoevsky_enabled(phase2["dostoevsky_enabled"]);
            }
            if (phase2.contains("cicada_enabled")) {
                set_cicada_enabled(phase2["cicada_enabled"]);
            }
            if (phase2.contains("ligra_enabled")) {
                set_ligra_enabled(phase2["ligra_enabled"]);
            }
            if (phase2.contains("rabitq_enabled")) {
                set_rabitq_enabled(phase2["rabitq_enabled"]);
            }
        }
    } catch (const std::exception&) {
        // Ignore JSON parsing errors, use defaults
    }
}

} // namespace performance
} // namespace themis
