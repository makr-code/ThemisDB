// Lightweight shim to provide an LLM reranker factory symbol for query/search
// paths when no concrete reranker backend is linked.

#include "themis/search/llm_reranker_factory.h"

namespace themis {
namespace search {

std::unique_ptr<ILlmReranker> createLlmReranker(const ILlmReranker::Config& cfg) {
    return nullptr;
}

} // namespace search
} // namespace themis
