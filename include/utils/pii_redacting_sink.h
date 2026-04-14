/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            pii_redacting_sink.h                               ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:45:02                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     164                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file pii_redacting_sink.h
 * @brief spdlog sink wrapper that automatically redacts PII from every log message.
 *
 * This header provides a delegating spdlog sink that intercepts each log message
 * before it reaches any underlying (real) sink and passes the formatted payload
 * through PIIRedactionPolicy::get().redactForLog().
 *
 * Design notes:
 * - The wrapper is zero-copy when no PII is detected (string comparison short-circuits
 *   the log_msg rebuild for the common PII-free case).
 * - A thread-local re-entrancy guard prevents infinite recursion in the unlikely
 *   event that PIIRedactionPolicy itself emits a log message during initialisation.
 * - The class is header-only to avoid adding a new translation unit and to stay
 *   consistent with how spdlog ships its sink adapters.
 * - Only the string-value `setAttribute` and `recordError` overloads need
 *   redaction; numeric/boolean overloads cannot contain PII.
 *
 * Coverage notes:
 * - Logger::init() installs this sink AND registers the resulting logger as the
 *   spdlog global default (spdlog::set_default_logger), so bare spdlog::info()
 *   calls made after Logger::init() are also covered.
 * - Bare spdlog::info() / spdlog::warn() calls that fire *before* Logger::init()
 *   is called (e.g., in static constructors) will use spdlog's built-in default
 *   logger which has no PII-redacting sink.  This is an inherent limitation of
 *   the global-logger pattern; mitigate by calling Logger::init() or Logger::get()
 *   as early as possible in the process lifecycle.
 *
 * Usage (see Logger::init() in logger.cpp):
 * @code
 * auto real_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
 * auto pii_sink  = std::make_shared<PIIRedactingSink>(real_sink);
 * logger_ = std::make_shared<spdlog::logger>("themis", pii_sink);
 * @endcode
 */

#include "security/pii_redaction_policy.h"
#include <spdlog/sinks/sink.h>
#include <memory>
#include <string>

namespace themis {
namespace utils {

/**
 * @brief Delegating spdlog sink that redacts PII before writing to a wrapped sink.
 *
 * Thread-safety: the wrapped sink is responsible for its own locking.
 * PIIRedactionPolicy is itself thread-safe.
 */
class PIIRedactingSink : public spdlog::sinks::sink {
public:
    /**
     * @brief Construct a PII-redacting wrapper around @p wrapped.
     * @param wrapped  The real destination sink (e.g. stdout_color_sink_mt, file_sink).
     */
    explicit PIIRedactingSink(spdlog::sink_ptr wrapped)
        : wrapped_(std::move(wrapped)) {}

    // -------------------------------------------------------------------------
    // spdlog::sinks::sink interface
    // -------------------------------------------------------------------------

    void log(const spdlog::details::log_msg& msg) override {
        // Thread-local re-entrancy guard: prevents infinite recursion within
        // the *same* thread if PIIRedactionPolicy or PIIDetector emits a log
        // message during lazy initialisation.  Each thread has its own copy of
        // the flag; concurrent calls from different threads are independent and
        // safe – the wrapped sink is responsible for its own thread-safety
        // (e.g., stdout_color_sink_mt uses its own mutex).
        if (in_redaction_) {
            if (wrapped_) wrapped_->log(msg);
            return;
        }

        in_redaction_ = true;

        // msg.payload is a string_view into a stack-allocated buffer; we need
        // a std::string to pass to redactForLog().
        std::string original(msg.payload.data(), msg.payload.size());
        std::string redacted = themis::security::PIIRedactionPolicy::get()
                                   .redactForLog(original);

        in_redaction_ = false;

        if (redacted == original) {
            // No PII detected – the string comparison is O(n) in the message
            // length but is worthwhile: it avoids constructing a new log_msg
            // (heap allocation + metadata copy) for the common case where no
            // PII is present.  Most log messages do not contain PII so this
            // branch is taken the overwhelming majority of the time.
            if (wrapped_) wrapped_->log(msg);
        } else {
            // Rebuild a log_msg with the redacted payload.  Keep `redacted`
            // alive on the stack for the entire duration of the wrapped log()
            // call so the string_view remains valid.
            spdlog::details::log_msg redacted_msg{
                msg.source,
                msg.logger_name,
                msg.level,
                spdlog::string_view_t{redacted.data(), redacted.size()}
            };
            redacted_msg.time = msg.time;
            redacted_msg.thread_id = msg.thread_id;
            redacted_msg.color_range_start = msg.color_range_start;
            redacted_msg.color_range_end = msg.color_range_end;
            // `redacted` (owning the data behind string_view_t) is still in
            // scope here; it outlives the wrapped_->log() call below.
            if (wrapped_) wrapped_->log(redacted_msg);
            // `redacted` is destroyed here, after wrapped_->log() returns.
        }
    }

    void flush() override {
        if (wrapped_) wrapped_->flush();
    }

    void set_pattern(const std::string& pattern) override {
        if (wrapped_) wrapped_->set_pattern(pattern);
    }

    void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override {
        if (wrapped_) wrapped_->set_formatter(std::move(sink_formatter));
    }

private:
    spdlog::sink_ptr wrapped_;

    // Thread-local re-entrancy guard to prevent infinite recursion if
    // PIIRedactionPolicy or PIIDetector emit log messages during lazy init.
    static thread_local bool in_redaction_;
};

// Out-of-line definition of the thread_local static member.
// The `inline` keyword ensures ODR-safety when this header is included in
// multiple translation units; no additional include guard is needed.
inline thread_local bool PIIRedactingSink::in_redaction_ = false;

} // namespace utils
} // namespace themis
