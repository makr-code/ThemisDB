/*
 * ThemisDB | File: inference_handle.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 73
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #3284 [llm] Implement prompt inje... (2026-03-12) | #3283 [llm] Propagate timeouts on... (2026-03-12) | #3282 [llm] Add tokens/sec and la... (2026-03-12) | #3281 [llm] Integrate single-mode... (2026-03-12) | #3270 [llm] Implement LoRA adapte... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include <future>
#include <string>
#include <atomic>
#include <memory>

namespace themis {
namespace llm {

/**
 * @file inference_handle.h
 * @brief Shared inference request handle for async operations
 * 
 * This handle is used by both AsyncInferenceEngine and InferenceEngineEnhanced
 * to track inference requests and retrieve results asynchronously.
 */

/**
 * @brief Inference request handle for tracking and cancellation
 * 
 * Provides a lightweight handle to track an inference request submitted
 * to an async inference engine. Supports:
 * - Blocking wait for result (get())
 * - Non-blocking status check (ready())
 * - Best-effort cancellation (cancel())
 *
 * Cancellation is propagated via a shared atomic<bool> cancel token that
 * is also held by the request in the inference engine. Setting the flag
 * causes the worker thread to skip queued requests and abort streaming
 * inference at the next token boundary.
 */
class InferenceHandle {
public:
    InferenceHandle(const std::string& request_id,
                    std::shared_future<InferenceResponse> future,
                    std::shared_ptr<std::atomic<bool>> cancel_token = nullptr)
        : request_id_(request_id), future_(future),
          cancel_token_(std::move(cancel_token)) {}
    
    // Wait for result (blocking)
    InferenceResponse get() { return future_.get(); }
    
    // Check if ready (non-blocking)
    bool ready() const {
        return future_.wait_for(std::chrono::seconds(0)) == 
               std::future_status::ready;
    }
    
    // Cancel request (best effort) — sets the shared cancel token so the
    // worker thread will stop processing at the next check point.
    void cancel();
    
    const std::string& requestId() const { return request_id_; }
    
private:
    std::string request_id_;
    std::shared_future<InferenceResponse> future_;
    std::shared_ptr<std::atomic<bool>> cancel_token_;
};

} // namespace llm
} // namespace themis
