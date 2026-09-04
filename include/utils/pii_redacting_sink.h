/**
 * @file pii_redacting_sink.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
            if (wrapped_) {
              wrapped_->log(msg);
            }
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
            if (wrapped_) {
              wrapped_->log(msg);
            }
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
            if (wrapped_) {
              wrapped_->log(redacted_msg);
            }
            // `redacted` is destroyed here, after wrapped_->log() returns.
        }
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
