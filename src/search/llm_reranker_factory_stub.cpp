#include "themis/search/llm_reranker_interface.h"
#include "themis/search/llm_reranker_factory.h"

namespace themis {
namespace search {

using LlmRerankerFactory = std::function<std::unique_ptr<ILlmReranker>(const ILlmReranker::Config&)>;

static LlmRerankerFactory g_reranker_factory = nullptr;

/**
 * @brief Register Llm Reranker Factory.
 * @param[in] f Input parameter.
 * @details Calls: std::move().
 */
void registerLlmRerankerFactory(LlmRerankerFactory f) { g_reranker_factory = std::move(f); }

/**
 * @brief Create Llm Reranker.
 * @param[in] cfg Input parameter.
 * @return Return value.
 * @details Calls: g_reranker_factory().
 */
std::unique_ptr<ILlmReranker> createLlmReranker(const ILlmReranker::Config& cfg) {
    if (g_reranker_factory) {
      return g_reranker_factory(cfg);
    }
    return nullptr;
}

} // namespace search
} // namespace themis
