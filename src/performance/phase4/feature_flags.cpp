/**
 * @file feature_flags.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/phase4/feature_flags.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

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

