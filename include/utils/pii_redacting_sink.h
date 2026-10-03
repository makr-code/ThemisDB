/**
 * @file pii_redacting_sink.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "security/pii_redaction_policy.h"
#include <spdlog/sinks/sink.h>
#include <array>
#include <memory>
#include <string>
#include <string_view>

namespace themis {
namespace utils {
namespace {

inline bool isStructuredTelemetryJson(std::string_view payload) {
    if (payload.empty() || payload.front() != '{') {
        return false;
    }
    constexpr std::array<std::string_view, 5> kTelemetryKeys = {
        "\"event\"",
        "\"layer_name\"",
        "\"correlation_id\"",
        "\"routing_reason_code\"",
        "\"resolved\""
    };

    for (const auto& key : kTelemetryKeys) {
        if (payload.find(key) != std::string_view::npos) {
            return true;
        }
    }
    return false;
}

} // namespace

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
        // Structured telemetry and already-normalized JSON must pass through the
        // sink untouched; these are audit records and must never be stripped by
        // generic PII redaction.
        if (msg.payload.size() > 0) {
            const std::string_view payload_view{msg.payload.data(), msg.payload.size()};
            if (isStructuredTelemetryJson(payload_view) || payload_view.front() == '{') {
                if (wrapped_) {
                    wrapped_->log(msg);
                }
                return;
            }
        }

        // Thread-local re-entrancy guard: prevents infinite recursion if a
        // redaction helper emits another log message while lazily initializing.
        if (in_redaction_) {
            if (wrapped_) {
                wrapped_->log(msg);
            }
            return;
        }

        in_redaction_ = true;
        try {
            // msg.payload is a string_view into a stack-allocated buffer; we need
            // a std::string to pass to redactForLog().
            std::string original(msg.payload.data(), msg.payload.size());
            std::string redacted = themis::security::PIIRedactionPolicy::get()
                                       .redactForLog(original);

            if (redacted == original) {
                if (wrapped_) {
                    wrapped_->log(msg);
                }
            } else {
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

                if (wrapped_) {
                    wrapped_->log(redacted_msg);
                }
            }
        } catch (...) {
            if (wrapped_) {
                wrapped_->log(msg);
            }
        }

        in_redaction_ = false;
    }

    void flush() override {
        if (wrapped_) {
          wrapped_->flush();
        }
    }

    void set_pattern(const std::string& pattern) override {
        if (wrapped_) {
          wrapped_->set_pattern(pattern);
        }
    }

    void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override {
        if (wrapped_) {
          wrapped_->set_formatter(std::move(sink_formatter));
        }
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
