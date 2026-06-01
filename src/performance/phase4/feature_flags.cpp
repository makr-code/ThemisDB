/*
 * ThemisDB | File: feature_flags.cpp | Version: 0.0.15 | Last Modified: 2026-05-24 14:31:17
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 48
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * PR History (last 5): #3339 feat(performance): io_uring... (2026-03-12) | #2616 feat(performance): Persiste... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
            if (phase4.contains("pmu_enabled")) {
                set_pmu_enabled(phase4["pmu_enabled"].get<bool>());
            }
            if (phase4.contains("io_uring_enabled")) {
                set_io_uring_enabled(phase4["io_uring_enabled"].get<bool>());
            }
        }
    } catch (...) {
        // Ignore JSON parsing errors, keep defaults
    }
}

} // namespace phase4
} // namespace performance
} // namespace themis
