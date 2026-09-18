/**
 * @file bidi_stream_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.4
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
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

template <typename Req, typename Resp,
          typename Stream = grpc::ServerReaderWriter<Resp, Req>>
class BidiStreamAdapter {
public:
    using MessageHandler = std::function<void(Req&&)>;

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

    BidiStreamAdapter(const BidiStreamAdapter&) = delete;
    BidiStreamAdapter& operator=(const BidiStreamAdapter&) = delete;
    BidiStreamAdapter(BidiStreamAdapter&&) = delete;
    BidiStreamAdapter& operator=(BidiStreamAdapter&&) = delete;

    ~BidiStreamAdapter() = default;

    /**
     * @brief On Message.
     * @param[in] handler Input parameter.
     * @details Calls: std::move().
     */
    void onMessage(MessageHandler handler) {
        handler_ = std::move(handler);
    }

    /**
     * @brief Run.
     * @details Calls: Read(), handler_(), std::move().
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
     * @brief Write.
     * @param[in] response Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: lock(), wait(), size(), push(), std::move(), flush().
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
     * @brief Finish.
     * @param[in] status Input parameter.
     * @details Calls: lock(), std::move(), notify_all().
     */
    void finish(grpc::Status status) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            finished_ = true;
            finish_status_ = std::move(status);
        }
        queue_not_full_.notify_all();
    }

    grpc::Status finishStatus() const {
        /**
         * @brief Lock.
         * @param[in] queue_mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return finish_status_;
    }

    std::size_t queueDepth() const {
        /**
         * @brief Lock.
         * @param[in] queue_mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return static_cast<int>(queue_.size());
    }

    bool isFinished() const {
        return finished_.load();
    }

private:
    /**
     * @brief Flush.
     * @details Calls: lock(), empty(), std::move(), front(), pop(), unlock(), Write(), notify_one().
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
