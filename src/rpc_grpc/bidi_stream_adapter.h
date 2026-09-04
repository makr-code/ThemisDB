/**
 * @file bidi_stream_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.4
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <grpcpp/grpcpp.h>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <utility>

namespace themis {
namespace plugins {
namespace rpc {
namespace grpc_plugin {

/**
 * @brief Typed bidirectional streaming adapter for gRPC service handlers.
 *
 * @tparam Req    Inbound message type (client → server).
 * @tparam Resp   Outbound message type (server → client).
 * @tparam Stream Stream type that exposes `Read(Req*)` and `Write(const Resp&)`.
 *                Defaults to `grpc::ServerReaderWriter<Resp, Req>` (production).
 *                Can be overridden with a mock type for unit testing.
 */
template <typename Req, typename Resp,
          typename Stream = grpc::ServerReaderWriter<Resp, Req>>
class BidiStreamAdapter {
public:
    /// Callback type invoked for each inbound message.
    using MessageHandler = std::function<void(Req&&)>;

    /**
     * @brief Construct the adapter around a gRPC server reader/writer stream.
     *
     * @param stream         Pointer to the stream (not owned).
     * @param max_queue_depth Maximum number of outbound messages buffered before
     *                        `write()` blocks. Default: 100.
     */
    explicit BidiStreamAdapter(
        Stream* stream,
        std::size_t max_queue_depth = 100)
        : stream_(stream),
          max_queue_depth_(max_queue_depth),
          finished_(false) {
        if (!stream_) {
            throw std::invalid_argument("BidiStreamAdapter: stream must not be null");
        }
    }

    /// Non-copyable, non-movable.
    BidiStreamAdapter(const BidiStreamAdapter&) = delete;
    BidiStreamAdapter& operator=(const BidiStreamAdapter&) = delete;
    BidiStreamAdapter(BidiStreamAdapter&&) = delete;
    BidiStreamAdapter& operator=(BidiStreamAdapter&&) = delete;

    ~BidiStreamAdapter() = default;

    /**
     * @brief Register the inbound message handler.
     *
     * Must be called before `run()`.  The handler is invoked synchronously
     * from the `run()` loop in the calling thread for each received message.
     * It is safe to call `write()` from within the handler.
     *
     * @param handler Callable accepting `Req&&` for each received message.
     */
    void onMessage(MessageHandler handler) {
        handler_ = std::move(handler);
    }

    /**
     * @brief Read all inbound messages and dispatch them to the handler.
     *
     * Blocks until the client half-closes the stream (no more messages).
     * Call this from the gRPC service handler thread.
     */
    void run() {
        Req msg;
        while (!finished_ && stream_->Read(&msg)) {
            if (handler_) {
                handler_(std::move(msg));
            }
            msg = Req{};
        }
    }

    /**
     * @brief Enqueue an outbound message.
     *
     * Thread-safe.  Blocks if the outbound queue is at capacity until
     * space becomes available, or until `finish()` is called.
     *
     * @param response Message to send to the client.
     * @return `true` if the message was written; `false` if the stream is
     *         already finished.
     */
    bool write(Resp response) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            // Wait while queue is full and stream is still open
            queue_not_full_.wait(lock, [this] {
                return finished_ || static_cast<int>(queue_.size()) < max_queue_depth_;
            });

            if (finished_) {
                return false;
            }

            queue_.push(std::move(response));
        }
        // Flush the queue directly (synchronous write to gRPC stream)
        flush();
        return true;
    }

    /**
     * @brief Mark the stream as finished and unblock any waiting `write()` calls.
     *
     * After this call returns, further `write()` invocations return `false`
     * immediately without blocking.
     *
     * @param status Final gRPC status to associate with this stream end.
     *               Stored for introspection; actual `Finish()` must be called
     *               by the service handler returning the status.
     */
    void finish(grpc::Status status) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            finished_ = true;
            finish_status_ = std::move(status);
        }
        queue_not_full_.notify_all();
    }

    /**
     * @brief Return the status set by the last `finish()` call.
     *
     * Returns `grpc::Status::OK` if `finish()` has not been called yet.
     */
    grpc::Status finishStatus() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return finish_status_;
    }

    /**
     * @brief Return the current outbound queue depth.
     *
     * Primarily useful for testing and diagnostics.
     */
    std::size_t queueDepth() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return static_cast<int>(queue_.size());
    }

    /**
     * @brief Return whether `finish()` has been called.
     */
    bool isFinished() const {
        return finished_.load();
    }

private:
    /**
     * @brief Drain the outbound queue, writing each message to the gRPC stream.
     *
     * Called after each successful `write()` enqueue.  Protected against
     * concurrent draining by the queue mutex.
     */
    void flush() {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        while (!queue_.empty()) {
            Resp msg = std::move(queue_.front());
            queue_.pop();
            lock.unlock();

            // Write outside the lock; gRPC stream is thread-safe for Write().
            stream_->Write(msg);
            queue_not_full_.notify_one();

            lock.lock();
        }
    }

    Stream* stream_;  ///< Non-owning pointer.
    const std::size_t max_queue_depth_;
    std::atomic<bool> finished_;

    mutable std::mutex queue_mutex_;
    std::condition_variable queue_not_full_;
    std::queue<Resp> queue_;
    MessageHandler handler_;
    grpc::Status finish_status_{grpc::Status::OK};
};

} // namespace grpc_plugin
} // namespace rpc
} // namespace plugins
} // namespace themis
