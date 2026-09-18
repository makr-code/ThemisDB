#pragma once

/**
 * @file inference_handle.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/llm_plugin_interface.h"

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <utility>

namespace themis {
namespace llm {

class InferenceHandle {
public:
    InferenceHandle(const std::string& request_id,
                    std::shared_future<InferenceResponse> future,
                    std::shared_ptr<std::atomic<bool>> cancel_token = nullptr)
        : request_id_(request_id), future_(future),
          cancel_token_(std::move(cancel_token)) {}
    
    /**
     * @brief Wait for result (blocking)
     * @return Return value.
     * @details Implements get without additional internal calls.
     */
    InferenceResponse get() { return future_.get(); }
    
    // Check if ready (non-blocking)
    bool ready() const {
        return future_.wait_for(std::chrono::seconds(0)) == 
               std::future_status::ready;
    }
    
    /**
     * @brief Cancel request (best effort) — sets the shared cancel token so the worker thread will stop processing at the next check point.
     */
    void cancel();
    
    const std::string& requestId() const { return request_id_; }
    
private:
    std::string request_id_;
    std::shared_future<InferenceResponse> future_;
    std::shared_ptr<std::atomic<bool>> cancel_token_;
};

} // namespace llm
} // namespace themis
