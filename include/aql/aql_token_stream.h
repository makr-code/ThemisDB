/**
 * @file aql_token_stream.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "aql/llm_error_codes.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <string>

namespace themis {
namespace aql {

class AQLTokenStream {
public:
    AQLTokenStream() = default;

    // Non-copyable; move is allowed so callers can transfer ownership.
    AQLTokenStream(const AQLTokenStream&)            = delete;
    AQLTokenStream& operator=(const AQLTokenStream&) = delete;
    AQLTokenStream(AQLTokenStream&&)                 noexcept = default;
    AQLTokenStream& operator=(AQLTokenStream&&)      noexcept = default;

    ~AQLTokenStream() {
        // Ensure any blocked consumer is unblocked.
        close();
    }

    // -------------------------------------------------------------------------
    // Producer API
    // -------------------------------------------------------------------------

    /**
     * @brief Push.
     * @param[in] token Input parameter.
     * @details Calls: lock(), notify_one().
     */
    void push(const std::string& token) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_ || cancelled_) {
              return;
            }
            queue_.push(token);
        }
        cv_.notify_one();
    }

    /**
     * @brief Close.
     * @details Calls: lock(), notify_all().
     */
    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        cv_.notify_all();
    }

    // -------------------------------------------------------------------------
    // Consumer API
    // -------------------------------------------------------------------------

    /**
     * @brief Next Token.
     * @return Return value.
     * @details Calls: lock(), wait(), empty(), front(), pop().
     */
    std::optional<std::string> nextToken() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] {
            return !queue_.empty() || closed_ || cancelled_;
        });
        if (!queue_.empty()) {
            auto token = queue_.front();
            queue_.pop();
            return token;
        }
        return std::nullopt;  // stream exhausted or cancelled
    }

    /**
     * @brief Cancel.
     * @details Calls: lock(), notify_all().
     */
    void cancel() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cancelled_ = true;
        }
        cv_.notify_all();
    }

    bool isCancelled() const {
        return cancelled_.load(std::memory_order_acquire);
    }

    bool isClosed() const {
        /**
         * @brief Lock.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    // -------------------------------------------------------------------------
    // Range-based for-loop support
    // -------------------------------------------------------------------------

    class Iterator {
    public:
        /**
         * @brief Iterator.
         * @param[in,out] stream Input/output parameter.
         * @return Return value.
         */
        explicit Iterator(AQLTokenStream* stream)
            : stream_(stream), current_(stream ? stream->nextToken() : std::nullopt) {}

        Iterator() : stream_(nullptr) {}

        const std::string& operator*() const { return *current_; }

        Iterator& operator++() {
            current_ = stream_->nextToken();
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            // End is signalled by an empty optional.
            if (other.stream_ == nullptr) {
                return current_.has_value();
            }
            return stream_ != other.stream_ || current_.has_value() != other.current_.has_value();
        }

    private:
        AQLTokenStream*          stream_;
        std::optional<std::string> current_;
    };

    /**
     * @brief Begin.
     * @return Return value.
     * @details Calls: Iterator().
     */
    Iterator begin() { return Iterator(this); }

    /**
     * @brief End.
     * @return Return value.
     * @details Calls: Iterator().
     */
    Iterator end()   { return Iterator();     }

private:
    mutable std::mutex              mutex_;
    std::condition_variable         cv_;
    std::queue<std::string>         queue_;
    bool                            closed_    = false;
    std::atomic<bool>               cancelled_ {false};
};

} // namespace aql
} // namespace themis
