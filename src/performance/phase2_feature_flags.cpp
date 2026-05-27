/*
 * ThemisDB | File: phase2_feature_flags.cpp | Version: 0.0.47 | Last Modified: 2026-04-15 18:58:58
 * Author: ThemisDB Version Bot | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 51
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=10 | delta=7 | status=divergent
 * External Severity (v3): C=1, H=6, M=3
 * PR: #1223 Reorganize config architecture into hierarchical structure with bac... (2026-03-11T17:48:01Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    } catch (...) {
        // Ignore JSON parsing errors, use defaults
    }
}

} // namespace performance
} // namespace themis
