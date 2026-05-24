/*
 * ThemisDB | File: inference_handle.cpp | Version: 0.0.47 | Last Modified: 2026-04-15 18:58:58
 * Author: ThemisDB Version Bot | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 24
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=8 | delta=5 | status=divergent
 * External Severity (v3): C=0, H=7, M=1
 * PR: #3629 [MODULE] llm â€“ build-system audit: register 16 missing sources, 2... (2026-03-12T07:39:34Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
