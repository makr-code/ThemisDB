/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            feature_flags.cpp                                  ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:18:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     64                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 5d88494041  2026-03-01  feat(performance): Implement io_uring zero-copy I/O path ... ║
    • 3fc5073575  2026-02-25  feat(performance/phase4): add PMU hardware counter integr... ║
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
            if (phase4.contains("pmu_enabled")) {
                set_pmu_enabled(phase4["pmu_enabled"].get<bool>());
            }
            if (phase4.contains("io_uring_enabled")) {
                set_io_uring_enabled(phase4["io_uring_enabled"].get<bool>());
            }
        }
    } catch (const std::exception&) {
        // Ignore JSON parsing errors, keep defaults
    }
}

} // namespace phase4
} // namespace performance
} // namespace themis
