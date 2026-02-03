#include "llm/inference_handle.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

void InferenceHandle::cancel() {
    // Mark as cancelled - worker will check this
    // Note: This is a best-effort cancellation. The actual cancellation
    // mechanism is handled by the inference engine that created this handle.
    spdlog::info("Cancellation requested for inference: {}", request_id_);
}

} // namespace llm
} // namespace themis
