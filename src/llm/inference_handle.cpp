/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inference_handle.cpp                               ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     34                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
