/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            phase2_feature_flags.h                             ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:26:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     121                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Phase 2 Performance Optimizations Feature Flags
// Based on scientific research from PR #156 (45+ peer-reviewed papers)
// Implementation framework from PR #157
//
// Expected gains (Phase 2): +100-200% overall performance
// Timeframe: 3-6 months implementation effort

#pragma once

#include <atomic>
#include <mutex>
#include <string>

namespace themis {
namespace performance {

/// Thread-safe singleton for Phase 2 performance feature flags
/// Allows runtime toggling of optimizations without recompilation
class Phase2FeatureFlags {
public:
    static Phase2FeatureFlags& instance() {
        static Phase2FeatureFlags instance;
        return instance;
    }

    // WiscKey: Key/Value Separation for LSM Trees (FAST'16)
    // Expected gain: +40-60% write throughput for values >1KB
    bool wisckey_enabled() const { return wisckey_enabled_.load(std::memory_order_relaxed); }
    void set_wisckey_enabled(bool enabled) { wisckey_enabled_.store(enabled, std::memory_order_relaxed); }

    // Dostoevsky: Adaptive LSM Tree Merging (SIGMOD'18)
    // Expected gain: +25-35% mixed workloads
    bool dostoevsky_enabled() const { return dostoevsky_enabled_.load(std::memory_order_relaxed); }
    void set_dostoevsky_enabled(bool enabled) { dostoevsky_enabled_.store(enabled, std::memory_order_relaxed); }

    // Cicada: Optimistic Concurrency Control (SIGMOD'17)
    // Expected gain: +100-150% transaction throughput
    bool cicada_enabled() const { return cicada_enabled_.load(std::memory_order_relaxed); }
    void set_cicada_enabled(bool enabled) { cicada_enabled_.store(enabled, std::memory_order_relaxed); }

    // Ligra: Parallel Graph Processing (PPoPP'13)
    // Expected gain: +200-300% graph operations
    bool ligra_enabled() const { return ligra_enabled_.load(std::memory_order_relaxed); }
    void set_ligra_enabled(bool enabled) { ligra_enabled_.store(enabled, std::memory_order_relaxed); }

    // RaBitQ: 2-bit Vector Quantization (SIGMOD'24)
    // Expected gain: 16x memory reduction, +50-80% throughput
    bool rabitq_enabled() const { return rabitq_enabled_.load(std::memory_order_relaxed); }
    void set_rabitq_enabled(bool enabled) { rabitq_enabled_.store(enabled, std::memory_order_relaxed); }

    // Load configuration from JSON file
    void load_from_config(const std::string& config_path);

private:
    Phase2FeatureFlags() = default;
    ~Phase2FeatureFlags() = default;
    Phase2FeatureFlags(const Phase2FeatureFlags&) = delete;
    Phase2FeatureFlags& operator=(const Phase2FeatureFlags&) = delete;

    std::atomic<bool> wisckey_enabled_{false};
    std::atomic<bool> dostoevsky_enabled_{false};
    std::atomic<bool> cicada_enabled_{false};
    std::atomic<bool> ligra_enabled_{false};
    std::atomic<bool> rabitq_enabled_{false};
};

// Macro helpers for compile-time + runtime checks
#ifdef THEMIS_ENABLE_WISCKEY
    #define THEMIS_PHASE2_WISCKEY_ENABLED() (::themis::performance::Phase2FeatureFlags::instance().wisckey_enabled())
#else
    #define THEMIS_PHASE2_WISCKEY_ENABLED() (false)
#endif

#ifdef THEMIS_ENABLE_DOSTOEVSKY
    #define THEMIS_PHASE2_DOSTOEVSKY_ENABLED() (::themis::performance::Phase2FeatureFlags::instance().dostoevsky_enabled())
#else
    #define THEMIS_PHASE2_DOSTOEVSKY_ENABLED() (false)
#endif

#ifdef THEMIS_ENABLE_CICADA
    #define THEMIS_PHASE2_CICADA_ENABLED() (::themis::performance::Phase2FeatureFlags::instance().cicada_enabled())
#else
    #define THEMIS_PHASE2_CICADA_ENABLED() (false)
#endif

#ifdef THEMIS_ENABLE_LIGRA
    #define THEMIS_PHASE2_LIGRA_ENABLED() (::themis::performance::Phase2FeatureFlags::instance().ligra_enabled())
#else
    #define THEMIS_PHASE2_LIGRA_ENABLED() (false)
#endif

#ifdef THEMIS_ENABLE_RABITQ
    #define THEMIS_PHASE2_RABITQ_ENABLED() (::themis::performance::Phase2FeatureFlags::instance().rabitq_enabled())
#else
    #define THEMIS_PHASE2_RABITQ_ENABLED() (false)
#endif

} // namespace performance
} // namespace themis
