/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inference_handle.cpp                               ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:32:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     39                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 99d0e82ce7  2026-02-22  Implement per-request timeout and cancellation propagatio... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/inference_handle.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

void InferenceHandle::cancel() {
    // Propagate cancellation via the shared token so the worker thread stops.
    if (cancel_token_) {
        cancel_token_->store(true, std::memory_order_release);
    }
    spdlog::info("Cancellation requested for inference: {}", request_id_);
}

} // namespace llm
} // namespace themis
