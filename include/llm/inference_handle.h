/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inference_handle.h                                 ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:07:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     79                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include <future>
#include <string>

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
 */
class InferenceHandle {
public:
    InferenceHandle(const std::string& request_id,
                    std::shared_future<InferenceResponse> future)
        : request_id_(request_id), future_(future) {}
    
    // Wait for result (blocking)
    InferenceResponse get() { return future_.get(); }
    
    // Check if ready (non-blocking)
    bool ready() const {
        return future_.wait_for(std::chrono::seconds(0)) == 
               std::future_status::ready;
    }
    
    // Cancel request (best effort)
    void cancel();
    
    const std::string& requestId() const { return request_id_; }
    
private:
    std::string request_id_;
    std::shared_future<InferenceResponse> future_;
};

} // namespace llm
} // namespace themis
