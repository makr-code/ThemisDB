/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inference_handle.h                                 ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:57:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     79                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea2e59ae9  2026-02-12  Add const-correctness to interfaces and eliminate unneces... ║
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
