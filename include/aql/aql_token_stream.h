/**
 * @file aql_token_stream.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Thread-safe streaming interface for LLM-generated tokens.
 *
 * AQLTokenStream bridges a token producer (e.g. an LLM inference thread) and
 * a consumer (e.g. a rendering loop or SSE writer).  The producer pushes tokens
 * via push() and signals end-of-stream via close().  The consumer either
 * iterates via the range-based for-loop interface or calls nextToken() directly.
 *
 * Cancellation is cooperative: the consumer calls cancel() and the producer
 * should check isCancelled() between tokens to abort early.
 *
 * Thread-safety:
 *   - push() and close() may be called from any thread (typically the inference
 *     thread).
 *   - nextToken(), begin(), end(), and cancel() may be called from any thread
 *     (typically the consumer thread).
 *   - Only one consumer thread is supported at a time.
 *
 * Usage example (consumer):
 * @code
 *   auto stream = std::make_shared<AQLTokenStream>();
 *   // ... producer fills the stream in another thread ...
 *   for (const auto& token : *stream) {
 *       std::cout << token;
 *   }
 * @endcode
 *
 * Usage example (producer):
 * @code
 *   stream->push("Hello");
 *   stream->push(", world");
 *   if (stream->isCancelled()) { return; }  // cooperative check
 *   stream->close();                         // signal end-of-stream
 * @endcode
 */
class AQLTokenStream {
public:
    AQLTokenStream() = default;

    // Non-copyable; move is allowed so callers can transfer ownership.
    AQLTokenStream(const AQLTokenStream&)            = delete;
    AQLTokenStream& operator=(const AQLTokenStream&) = delete;
    AQLTokenStream(AQLTokenStream&&)                 = default;
    AQLTokenStream& operator=(AQLTokenStream&&)      = default;

    ~AQLTokenStream() {
        // Ensure any blocked consumer is unblocked.
        close();
    }

    // -------------------------------------------------------------------------
    // Producer API
    // -------------------------------------------------------------------------

    /**
     * @brief Push a token onto the stream.
     *
     * Notifies one waiting consumer.  If the stream has already been closed or
     * cancelled this call is a no-op (the token is silently discarded).
     *
     * @param token  Token string to enqueue.
     */
    void push(const std::string& token) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_ || cancelled_) return;
            queue_.push(token);
        }
        cv_.notify_one();
    }

    /**
     * @brief Signal end-of-stream.
     *
     * After close() returns the consumer will drain any remaining queued tokens
     * and then see an empty optional from nextToken(), indicating completion.
     * Calling close() more than once is safe (idempotent).
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
     * @brief Retrieve the next token, blocking until one is available.
     *
     * @return The next token, or std::nullopt when the stream is exhausted
     *         (closed with no more queued tokens) or cancelled.
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
     * @brief Request cooperative cancellation.
     *
     * Unblocks any waiting consumer (nextToken() returns nullopt) and sets the
     * cancelled flag so producers can detect and abort early via isCancelled().
     * Calling cancel() more than once is safe (idempotent).
     */
    void cancel() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cancelled_ = true;
        }
        cv_.notify_all();
    }

    /**
     * @brief Check whether cancellation has been requested.
     *
     * Producers should call this periodically to cooperate with cancellation.
     */
    bool isCancelled() const {
        return cancelled_.load(std::memory_order_acquire);
    }

    /**
     * @brief Check whether the stream has been closed by the producer.
     */
    bool isClosed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    // -------------------------------------------------------------------------
    // Range-based for-loop support
    // -------------------------------------------------------------------------

    /**
     * @brief Input iterator over AQLTokenStream tokens.
     *
     * Satisfies the minimum requirements for a range-based for-loop.
     * Advancing the iterator blocks until the next token is available.
     */
    class Iterator {
    public:
        explicit Iterator(AQLTokenStream* stream)
            : stream_(stream), current_(stream ? stream->nextToken() : std::nullopt) {}

        /// Sentinel iterator constructor (end-of-stream marker).
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

    /// Return an iterator pointing at the first (possibly blocking) token.
    Iterator begin() { return Iterator(this); }

    /// Return the sentinel end iterator.
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
