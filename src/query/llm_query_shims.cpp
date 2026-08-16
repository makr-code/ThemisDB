// Lightweight shims to provide minimal LLM factory symbols for the query
// module in modular builds where the full LLM API module may not be
// linked into the final shared library. These return nullptr and are
// accepted by callers which handle missing LLM functionality gracefully.

#include "themis/llm/llm_factory.h"
#include "themis/llm/lora_orchestrator_interface.h"
#include "themis/search/llm_reranker_factory.h"

namespace themis {
namespace llm {

std::shared_ptr<ILLMPluginManager> createLLMPluginManager() {
    return nullptr;
}

std::shared_ptr<lora::ILoRAOrchestrator> createLoRAOrchestrator() {
    return nullptr;
}

} // namespace llm
} // namespace themis

namespace themis {
namespace search {

std::unique_ptr<ILlmReranker> createLlmReranker(const ILlmReranker::Config& cfg) {
    return nullptr;
}

} // namespace search
} // namespace themis
