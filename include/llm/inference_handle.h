/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inference_handle.h                                 ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:35:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     87                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
