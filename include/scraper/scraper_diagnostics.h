// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file scraper_diagnostics.h
 * @brief Unified diagnostics primitives for scraper pipeline fault classes.
 * @version 1.0.0
 *
 * ## §Purpose
 *
 * Provides a structured, machine-parseable diagnostic vocabulary that unifies
 * fault reporting across the crawler, evaluator, and metadata-writer stages of
 * the scraper pipeline.  All scraper diagnostic events are expressed as a
 * @ref ScraperDiagnosticEvent with a @ref ScraperFaultClass and a
 * @ref ScraperFaultSeverity.
 *
 * ## §Design Constraints
 *
 * - Diagnostic types are header-only value types; no allocation beyond
 *   std::string fields.
 * - All degraded and fallback paths that emit diagnostics MUST do so
 *   non-silently: the @ref ScraperDiagnosticEvent::message field is mandatory.
 * - The @ref IScraperDiagnosticSink interface is the sole channel for
 *   emitting events to operator-visible log/metrics sinks.
 * - Implementations must be thread-safe: concurrent emits from the bounded
 *   thread pool must not corrupt sink state.
 *
 * ## §Usage
 *
 * ```cpp
 * ScraperDiagnosticEvent ev;
 * ev.fault_class  = ScraperFaultClass::kFetchPath;
 * ev.severity     = ScraperFaultSeverity::kWarning;
 * ev.error        = ScraperError::kFetchFailed;
 * ev.source_url   = request.source_url;
 * ev.message      = "HTTP 429 – rate limit; backing off";
 * sink.emit(ev);
 * ```
 *
 * @see include/scraper/scraper_api_contract.h
 * @see src/scraper/ROADMAP.md — Phase 3 items
 */

#pragma once

#include "scraper/scraper_api_contract.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace scraper {

// ============================================================================
// § 1  Fault classification
// ============================================================================

/**
 * @brief Pipeline stage that produced the diagnostic event.
 *
 * Used for operator triage: each class maps to a distinct log channel and
 * metric counter prefix.
 */
enum class ScraperFaultClass : uint8_t {
    kFetchPath      = 0, ///< HTTP fetch / DNS / TLS errors.
    kRenderPath     = 1, ///< JS renderer timeout / crash.
    kParsePath      = 2, ///< HTML/DOM parse structural errors.
    kEvaluatorPath  = 3, ///< LLM evaluator failures or below-threshold.
    kWritePath      = 4, ///< Metadata / relational / graph / vector write.
    kCrawlControl   = 5, ///< Pagination limit, burst throttle, source catalog.
    kInternal       = 6, ///< Unclassified internal errors.
};

/**
 * @brief Operational severity for a diagnostic event.
 */
enum class ScraperFaultSeverity : uint8_t {
    kInfo    = 0, ///< Informational; no operator action required.
    kWarning = 1, ///< Degraded path taken; operator may want to inspect.
    kError   = 2, ///< Operation failed; run may continue with other sources.
    kFatal   = 3, ///< Hard failure; scrape run must be aborted.
};

// ============================================================================
// § 2  Diagnostic event
// ============================================================================

/**
 * @brief Structured diagnostic event emitted for every scraper fault.
 *
 * All fields except @c stage_hint are mandatory.  Implementations that omit
 * @c message are non-conformant and will be rejected in code review.
 */
struct ScraperDiagnosticEvent {
    ScraperFaultClass    fault_class{ScraperFaultClass::kInternal};
    ScraperFaultSeverity severity{ScraperFaultSeverity::kError};
    ScraperError         error{ScraperError::kInternalError};

    /// Source URL associated with this event (empty for catalog-level faults).
    std::string source_url;

    /// Human-readable diagnostic message.  Mandatory; must be non-empty.
    std::string message;

    /// Optional free-text hint describing the pipeline stage or call site.
    std::string stage_hint;

    /// Wall-clock timestamp of the event.
    std::chrono::system_clock::time_point timestamp{
        std::chrono::system_clock::now()};
};

// ============================================================================
// § 3  Diagnostic sink interface
// ============================================================================

/**
 * @brief Operator-visible sink for scraper diagnostic events.
 *
 * Implementations must be thread-safe.  The default @ref NullScraperDiagnosticSink
 * is a no-op suitable for unit tests.  The @ref ListeningScraperDiagnosticSink
 * supports injecting listener callbacks for integration tests and metrics.
 */
class IScraperDiagnosticSink {
public:
    virtual ~IScraperDiagnosticSink() = default;

    /**
     * @brief Emit a diagnostic event to the sink.
     *
     * @param event The structured event.  @c event.message must be non-empty.
     *
     * Implementations MUST NOT throw.  Any internal error during emit should
     * be swallowed and counted (not propagated to the scraper pipeline).
     */
    virtual void emit(const ScraperDiagnosticEvent& event) noexcept = 0;
};

// ============================================================================
// § 4  Null sink (no-op)
// ============================================================================

/**
 * @brief No-op diagnostic sink for unit tests and default construction.
 */
class NullScraperDiagnosticSink final : public IScraperDiagnosticSink {
public:
    void emit(const ScraperDiagnosticEvent& /*event*/) noexcept override {}
};

// ============================================================================
// § 5  Listening sink (tests + integration)
// ============================================================================

/**
 * @brief Thread-safe diagnostic sink that records events and notifies listeners.
 *
 * Suitable for integration tests and metrics bridge adapters.
 */
class ListeningScraperDiagnosticSink final : public IScraperDiagnosticSink {
public:
    using Listener = std::function<void(const ScraperDiagnosticEvent&)>;

    /**
     * @brief Register a listener called synchronously on each emit.
     *
     * Listeners are called while the internal mutex is held; keep them short.
     */
    void addListener(Listener fn) {
        std::lock_guard<std::mutex> lk(mu_);
        listeners_.push_back(std::move(fn));
    }

    void emit(const ScraperDiagnosticEvent& event) noexcept override {
        std::lock_guard<std::mutex> lk(mu_);
        events_.push_back(event);
        for (const auto& fn : listeners_) {
            try { fn(event); } catch (...) {}
        }
    }

    /// Return a snapshot of all recorded events (thread-safe copy).
    std::vector<ScraperDiagnosticEvent> snapshot() const {
        std::lock_guard<std::mutex> lk(mu_);
        return events_;
    }

    /// Number of events recorded.
    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return events_.size();
    }

    /// Clear all recorded events and listeners.
    void clear() {
        std::lock_guard<std::mutex> lk(mu_);
        events_.clear();
    }

private:
    mutable std::mutex mu_;
    std::vector<ScraperDiagnosticEvent> events_;
    std::vector<Listener> listeners_;
};

// ============================================================================
// § 6  Fail-safe helpers
// ============================================================================

/**
 * @brief Returns the @ref ScraperFaultClass for the given @ref ScraperError.
 *
 * Used to route diagnostic events to the correct pipeline-stage counter without
 * requiring callers to maintain their own mapping table.
 */
[[nodiscard]] inline constexpr ScraperFaultClass faultClassOf(
    ScraperError e) noexcept {
    switch (e) {
        case ScraperError::kFetchFailed:         return ScraperFaultClass::kFetchPath;
        case ScraperError::kRenderTimeout:       return ScraperFaultClass::kRenderPath;
        case ScraperError::kParseError:          return ScraperFaultClass::kParsePath;
        case ScraperError::kEvaluationFailed:    return ScraperFaultClass::kEvaluatorPath;
        case ScraperError::kMetadataWriteFailed: return ScraperFaultClass::kWritePath;
        case ScraperError::kSourceNotFound:      return ScraperFaultClass::kCrawlControl;
        case ScraperError::kPaginationLimit:     return ScraperFaultClass::kCrawlControl;
        case ScraperError::kSuccess:             return ScraperFaultClass::kInternal;
        case ScraperError::kInternalError:       return ScraperFaultClass::kInternal;
    }
    return ScraperFaultClass::kInternal;
}

/**
 * @brief Returns the default @ref ScraperFaultSeverity for the given error.
 *
 * - Fatal errors (@ref isScraperFailClosed) map to kFatal.
 * - kSuccess maps to kInfo.
 * - All others map to kError.
 */
[[nodiscard]] inline constexpr ScraperFaultSeverity defaultSeverityOf(
    ScraperError e) noexcept {
    if (e == ScraperError::kSuccess) return ScraperFaultSeverity::kInfo;
    if (isScraperFailClosed(e))      return ScraperFaultSeverity::kFatal;
    return ScraperFaultSeverity::kError;
}

/**
 * @brief Build a minimal well-formed @ref ScraperDiagnosticEvent from an error.
 *
 * @param e       Error code that triggered the diagnostic.
 * @param url     Source URL (may be empty for catalog-level faults).
 * @param message Human-readable description.  Must be non-empty.
 * @return A fully-populated ScraperDiagnosticEvent ready for emit.
 */
[[nodiscard]] inline ScraperDiagnosticEvent makeDiagnosticEvent(
    ScraperError       e,
    const std::string& url,
    const std::string& message) {
    ScraperDiagnosticEvent ev;
    ev.fault_class = faultClassOf(e);
    ev.severity    = defaultSeverityOf(e);
    ev.error       = e;
    ev.source_url  = url;
    ev.message     = message;
    return ev;
}

} // namespace scraper
} // namespace themis
