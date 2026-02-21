/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            api_version_config.h                               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:37:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     58                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2e740da75  2026-01-24  Add API versioning infrastructure with 24-month deprecati... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file api_version_config.h
 * @brief Central configuration for API versions
 * 
 * This file provides centralized version configuration for the entire system.
 * When updating versions, only modify this file.
 */

namespace themis::server {

/**
 * @brief Current API version configuration
 * 
 * These values should be updated when releasing new API versions.
 * They are synchronized with the VERSION file at build time.
 */
struct APIVersionConfig {
    // Current stable API version (from VERSION file: 1.4.1-dev)
    static constexpr uint32_t CURRENT_MAJOR = 1;
    static constexpr uint32_t CURRENT_MINOR = 4;
    static constexpr uint32_t CURRENT_PATCH = 1;
    
    // Minimum supported API version (oldest version still supported)
    static constexpr uint32_t MINIMUM_MAJOR = 1;
    static constexpr uint32_t MINIMUM_MINOR = 0;
    static constexpr uint32_t MINIMUM_PATCH = 0;
    
    // Deprecation policy duration (in days)
    static constexpr int DEPRECATION_PERIOD_DAYS = 730; // 24 months (~2 years)
};

} // namespace themis::server
