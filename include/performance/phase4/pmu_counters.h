/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pmu_counters.h                                     ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:04:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     177                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB - Hardware Performance Counter (PMU) Integration
// Provides cache miss analysis via Linux perf_event_open (Phase 4).
//
// Key idea: modern CPUs expose hardware performance monitoring units (PMU)
// that count hardware events (cache misses, branch mispredictions, etc.)
// with negligible overhead (<1 ns per measurement point when amortised).
// This module wraps the Linux perf_event_open(2) syscall to expose:
//   • L1d read cache misses
//   • Last-level cache (LLC) misses
//   • Branch mispredictions
//
// On non-Linux platforms or when perf_event_open is unavailable (e.g.
// due to kernel parameter paranoid > 2) all counters return 0 and
// CacheMissAnalyzer::is_available() returns false.
//
// Compile-time gate: THEMIS_ENABLE_PMU_COUNTERS
// Runtime gate:      Phase4FeatureFlags::instance().pmu_enabled()

#pragma once

#include <cstdint>

namespace themis {
namespace performance {
namespace phase4 {

// ---------------------------------------------------------------------------
// CacheMissMetrics – plain data aggregate returned by CacheMissAnalyzer
// ---------------------------------------------------------------------------

/// Hardware cache-miss counters collected over a measurement interval.
struct CacheMissMetrics {
    uint64_t l1d_read_misses      = 0;  ///< L1 data-cache read misses
    uint64_t llc_misses           = 0;  ///< Last-level cache misses
    uint64_t branch_mispredictions = 0; ///< Branch mispredictions
    bool     available            = false; ///< false if PMU access failed
};

// ---------------------------------------------------------------------------
// PmuCounter – thin RAII wrapper around a single perf_event fd
// ---------------------------------------------------------------------------

/// Wraps a single Linux perf_event counter.
/// Not copyable; movable.
class PmuCounter {
public:
    PmuCounter() noexcept;
    ~PmuCounter() noexcept;

    PmuCounter(const PmuCounter&) = delete;
    PmuCounter& operator=(const PmuCounter&) = delete;

    PmuCounter(PmuCounter&& other) noexcept;
    PmuCounter& operator=(PmuCounter&& other) noexcept;

    /// Open a perf event counter.
    /// @param type   perf_event_attr.type  (e.g. PERF_TYPE_HARDWARE)
    /// @param config perf_event_attr.config (e.g. PERF_COUNT_HW_CACHE_MISSES)
    /// @return true on success
    bool open(uint32_t type, uint64_t config) noexcept;

    /// Reset and enable the counter.
    void enable() noexcept;

    /// Disable the counter (stops counting).
    void disable() noexcept;

    /// Read the current counter value.
    /// Returns 0 on error or if not opened.
    uint64_t read() const noexcept;

    /// Close the file descriptor.
    void close() noexcept;

    /// Returns true if the counter was successfully opened.
    bool is_open() const noexcept { return fd_ >= 0; }

private:
    int fd_{-1};
};

// ---------------------------------------------------------------------------
// CacheMissAnalyzer – high-level helper for cache-miss analysis
// ---------------------------------------------------------------------------

/// Collects L1d, LLC, and branch-misprediction counters.
/// Typical usage:
/// @code
///   CacheMissAnalyzer analyzer;
///   if (analyzer.is_available()) {
///       analyzer.start();
///       do_work();
///       CacheMissMetrics m = analyzer.stop();
///       // use m.llc_misses, m.l1d_read_misses, ...
///   }
/// @endcode
class CacheMissAnalyzer {
public:
    CacheMissAnalyzer() noexcept;
    ~CacheMissAnalyzer() noexcept = default;

    CacheMissAnalyzer(const CacheMissAnalyzer&) = delete;
    CacheMissAnalyzer& operator=(const CacheMissAnalyzer&) = delete;

    /// Returns true if all required PMU counters could be opened.
    bool is_available() const noexcept { return available_; }

    /// Reset and enable all counters.
    void start() noexcept;

    /// Disable counters and return accumulated metrics.
    CacheMissMetrics stop() noexcept;

    /// Static convenience check (tries to open a test counter).
    static bool pmu_accessible() noexcept;

private:
    PmuCounter l1d_misses_;
    PmuCounter llc_misses_;
    PmuCounter branch_mispredict_;
    bool       available_{false};
};

// ---------------------------------------------------------------------------
// ScopedCacheMissTimer – RAII measurement wrapper
// ---------------------------------------------------------------------------

/// RAII timer: calls analyzer.start() on construction,
/// analyzer.stop() on destruction and writes the result to *output.
class ScopedCacheMissTimer {
public:
    explicit ScopedCacheMissTimer(CacheMissAnalyzer& analyzer,
                                  CacheMissMetrics*  output) noexcept
        : analyzer_(analyzer), output_(output)
    {
        analyzer_.start();
    }

    ~ScopedCacheMissTimer() noexcept {
        if (output_) {
            *output_ = analyzer_.stop();
        }
    }

    ScopedCacheMissTimer(const ScopedCacheMissTimer&) = delete;
    ScopedCacheMissTimer& operator=(const ScopedCacheMissTimer&) = delete;

private:
    CacheMissAnalyzer& analyzer_;
    CacheMissMetrics*  output_;
};

} // namespace phase4
} // namespace performance
} // namespace themis
