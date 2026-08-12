/**
 * @file pmu_counters.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <functional>

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
    using OpenFn = std::function<bool(uint32_t, uint64_t)>;
    using ReadFn = std::function<uint64_t()>;

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

    static void setOpenFn(OpenFn fn);
    static void setReadFn(ReadFn fn);

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
    using StopFn = std::function<CacheMissMetrics()>;
    using ProbeFn = std::function<bool()>;

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

    static void setStopFn(StopFn fn);
    static void setProbeFn(ProbeFn fn);

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
