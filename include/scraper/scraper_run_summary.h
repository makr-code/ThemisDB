// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file scraper_run_summary.h
 * @brief Operator-facing diagnostics triage interface for scraper run summaries.
 * @version 1.0.0
 *
 * ## §Purpose
 *
 * Provides @ref ScraperRunSummary (a value-type aggregate of per-run counters)
 * and @ref ScraperRunSummaryCollector (a sink listener that accumulates those
 * counters from @ref ListeningScraperDiagnosticSink events).
 *
 * ## §Design Constraints
 *
 * - Header-only; no separate translation unit required.
 * - All counter mutations are protected by @c std::mutex / @c std::lock_guard.
 * - No file I/O, no network; safe for unit-test and production use.
 *
 * ## §Usage
 *
 * ```cpp
 * ListeningScraperDiagnosticSink sink;
 * ScraperRunSummaryCollector     collector;
 * collector.attach(sink);
 *
 * // … run the scraper pipeline, emitting events into sink …
 *
 * collector.recordSuccess(pages_written);
 * collector.setRunStats(total_urls, duration_ms);
 *
 * ScraperRunSummary s = collector.summary();
 * if (!s.isHealthy()) {
 *     log::warn("{}", s.toLogLine());
 * }
 * ```
 *
 * @see include/scraper/scraper_diagnostics.h
 */

#pragma once

#include "scraper/scraper_diagnostics.h"

#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>

namespace themis {
namespace scraper {

// ============================================================================
// § 1  ScraperRunSummary
// ============================================================================

/**
 * @brief Plain value snapshot of a single scraper-run's outcome counters.
 *
 * Populated by @ref ScraperRunSummaryCollector after the run completes.
 * All fields are public for direct aggregate construction; callers should
 * treat an instance as a read-only snapshot once it is returned from
 * @c ScraperRunSummaryCollector::summary().
 * Suitable for structured logging, metrics export, and operator triage.
 */
struct ScraperRunSummary {
    uint32_t total_urls{0};      ///< Total URLs attempted in the run.
    uint32_t succeeded{0};       ///< URLs that completed successfully.
    uint32_t failed_fetch{0};    ///< kFetchPath (and kRenderPath) fault events.
    uint32_t failed_parse{0};    ///< kParsePath fault events.
    uint32_t failed_eval{0};     ///< kEvaluatorPath fault events (fail-closed).
    uint32_t failed_write{0};    ///< kWritePath fault events.
    uint32_t skipped_burst{0};   ///< kCrawlControl/kWarning events (burst throttle).
    uint32_t pages_written{0};   ///< Successfully written metadata records.
    uint64_t run_duration_ms{0}; ///< Total run duration in milliseconds.

    /**
     * @brief Returns true when at least one URL succeeded and no evaluation
     *        failures occurred.
     *
     * An evaluation failure (kEvaluatorPath) is fail-closed: content must not
     * be written when quality gating fails, so any such failure marks the run
     * as unhealthy for operator review.
     */
    [[nodiscard]] bool isHealthy() const noexcept {
        return succeeded > 0 && failed_eval == 0;
    }

    /**
     * @brief Returns a single-line operator log string.
     *
     * Format:
     * @code
     * scrape-run: succeeded=N failed_fetch=N failed_parse=N failed_eval=N
     *             failed_write=N skipped_burst=N pages_written=N duration_ms=N
     * @endcode
     * (all on one line)
     */
    [[nodiscard]] std::string toLogLine() const {
        std::ostringstream oss;
        oss << "scrape-run:"
            << " succeeded="      << succeeded
            << " failed_fetch="   << failed_fetch
            << " failed_parse="   << failed_parse
            << " failed_eval="    << failed_eval
            << " failed_write="   << failed_write
            << " skipped_burst="  << skipped_burst
            << " pages_written="  << pages_written
            << " duration_ms="    << run_duration_ms;
        return oss.str();
    }
};

// ============================================================================
// § 2  ScraperRunSummaryCollector
// ============================================================================

/**
 * @brief Accumulates per-run fault counters by listening to a
 *        @ref ListeningScraperDiagnosticSink.
 *
 * Attach once before the run starts via @ref attach().  After the run
 * completes, call @ref recordSuccess() and @ref setRunStats(), then retrieve
 * the snapshot with @ref summary().
 *
 * Thread-safety: all public methods are thread-safe; the listener lambda
 * installed by @ref attach() may be invoked concurrently from the scraper
 * bounded thread pool.
 */
class ScraperRunSummaryCollector {
public:
    /**
     * @brief Attach this collector as a listener to the given sink.
     *
     * Registers a lambda that increments the appropriate counter for each
     * emitted @ref ScraperDiagnosticEvent.  The lambda holds a raw pointer to
     * @c *this; the collector must outlive the sink (or be reset/detached
     * before destruction).
     *
     * @param sink  The sink whose events should be counted.
     */
    void attach(ListeningScraperDiagnosticSink& sink) {
        sink.addListener([this](const ScraperDiagnosticEvent& e) {
            std::lock_guard<std::mutex> lk(mu_);
            switch (e.fault_class) {
                case ScraperFaultClass::kFetchPath:
                    ++failed_fetch_;
                    break;
                case ScraperFaultClass::kRenderPath:
                    // Render failures count as fetch failures for the summary.
                    ++failed_fetch_;
                    break;
                case ScraperFaultClass::kParsePath:
                    ++failed_parse_;
                    break;
                case ScraperFaultClass::kEvaluatorPath:
                    ++failed_eval_;
                    break;
                case ScraperFaultClass::kWritePath:
                    ++failed_write_;
                    break;
                case ScraperFaultClass::kCrawlControl:
                    if (e.severity == ScraperFaultSeverity::kWarning) {
                        ++skipped_burst_;
                    }
                    break;
                case ScraperFaultClass::kInternal:
                    // Internal faults are not surfaced in the run summary counters.
                    break;
            }
        });
    }

    /**
     * @brief Record a successful URL completion.
     *
     * Call once per URL that was successfully written.
     *
     * @param pages  Number of metadata records written for this URL (default 1).
     */
    void recordSuccess(uint32_t pages = 1) noexcept {
        std::lock_guard<std::mutex> lk(mu_);
        ++succeeded_;
        pages_written_ += pages;
    }

    /**
     * @brief Set aggregate run-level statistics after the run completes.
     *
     * @param total_urls      Total URLs that were attempted.
     * @param run_duration_ms Wall-clock run duration in milliseconds.
     */
    void setRunStats(uint32_t total_urls, uint64_t run_duration_ms) noexcept {
        std::lock_guard<std::mutex> lk(mu_);
        total_urls_      = total_urls;
        run_duration_ms_ = run_duration_ms;
    }

    /**
     * @brief Returns a snapshot of the accumulated counters.
     *
     * The returned @ref ScraperRunSummary is a value copy taken under the
     * internal mutex; it will not change after return.
     */
    [[nodiscard]] ScraperRunSummary summary() const noexcept {
        std::lock_guard<std::mutex> lk(mu_);
        ScraperRunSummary s;
        s.total_urls      = total_urls_;
        s.succeeded       = succeeded_;
        s.failed_fetch    = failed_fetch_;
        s.failed_parse    = failed_parse_;
        s.failed_eval     = failed_eval_;
        s.failed_write    = failed_write_;
        s.skipped_burst   = skipped_burst_;
        s.pages_written   = pages_written_;
        s.run_duration_ms = run_duration_ms_;
        return s;
    }

    /// Reset all counters to zero.
    void reset() noexcept {
        std::lock_guard<std::mutex> lk(mu_);
        total_urls_      = 0;
        succeeded_       = 0;
        failed_fetch_    = 0;
        failed_parse_    = 0;
        failed_eval_     = 0;
        failed_write_    = 0;
        skipped_burst_   = 0;
        pages_written_   = 0;
        run_duration_ms_ = 0;
    }

private:
    mutable std::mutex mu_;
    uint32_t total_urls_{0};
    uint32_t succeeded_{0};
    uint32_t failed_fetch_{0};
    uint32_t failed_parse_{0};
    uint32_t failed_eval_{0};
    uint32_t failed_write_{0};
    uint32_t skipped_burst_{0};
    uint32_t pages_written_{0};
    uint64_t run_duration_ms_{0};
};

} // namespace scraper
} // namespace themis
