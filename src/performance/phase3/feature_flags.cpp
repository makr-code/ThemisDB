/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feature_flags.cpp                                  ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     71                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "performance/phase3/feature_flags.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace themis {
namespace performance {
namespace phase3 {

void Phase3FeatureFlags::load_from_config(const std::string& config_path) {
    try {
        std::ifstream file(config_path);
        if (!file.is_open()) {
            return; // Config file doesn't exist, use defaults
        }
        
        nlohmann::json config;
        file >> config;
        
        if (config.contains("performance") && config["performance"].contains("phase3")) {
            auto phase3 = config["performance"]["phase3"];
            
            if (phase3.contains("diskann_enabled")) {
                set_diskann_enabled(phase3["diskann_enabled"]);
            }
            if (phase3.contains("bwtree_enabled")) {
                set_bwtree_enabled(phase3["bwtree_enabled"]);
            }
            if (phase3.contains("splinterdb_enabled")) {
                set_splinterdb_enabled(phase3["splinterdb_enabled"]);
            }
            if (phase3.contains("gunrock_enabled")) {
                set_gunrock_enabled(phase3["gunrock_enabled"]);
            }
            if (phase3.contains("bao_enabled")) {
                set_bao_enabled(phase3["bao_enabled"]);
            }
        }
    } catch (const std::exception&) {
        // Ignore JSON parsing errors, use defaults
    }
}

} // namespace phase3
} // namespace performance
} // namespace themis
