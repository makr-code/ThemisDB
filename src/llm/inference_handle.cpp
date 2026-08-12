/**
 * @file inference_handle.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
