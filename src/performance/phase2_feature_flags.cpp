/**
 * @file phase2_feature_flags.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: phase2_feature_flags.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 52
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * PR History (last 5): #1223 Reorganize config architect... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "performance/phase2_feature_flags.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

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
    } catch (...) {
        // Ignore JSON parsing errors, use defaults
    }
}

} // namespace performance
} // namespace themis

